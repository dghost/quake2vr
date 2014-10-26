#include "include/r_vr_ovr.h"
#include "../vr/include/vr_ovr.h"
#include "include/r_local.h"
#include "../OVR_CAPI.h"
#include "../../backends/sdl2/sdl2quake.h"


void OVR_FrameStart(int32_t changeBackBuffers);
void OVR_Present(qboolean loading);
int32_t OVR_Enable(void);
void OVR_Disable(void);
int32_t OVR_Init(void);
void OVR_GetState(vr_param_t *state);
void OVR_PostPresent(void);

hmd_render_t vr_render_ovr = 
{
	HMD_RIFT,
	OVR_Init,
	OVR_Enable,
	OVR_Disable,
	OVR_FrameStart,
	OVR_GetState,
	OVR_Present,
	OVR_PostPresent
};

extern ovrHmd hmd;
extern ovrEyeRenderDesc eyeDesc[2];
extern ovrTrackingState trackingState;
extern ovrFrameTiming frameTime;
extern qboolean withinFrame;

extern void VR_OVR_GetFOV(float *fovx, float *fovy);
extern int32_t VR_OVR_RenderLatencyTest(vec4_t color);


static vr_param_t currentState;

// this should probably be rearranged
typedef struct {
	fbo_t eyeFBO;
	vbo_t eye;
	ovrSizei renderTarget;
	ovrFovPort eyeFov; 
	ovrVector2f UVScaleOffset[2];
} ovr_eye_info_t;

static ovr_eye_info_t renderInfo[2];

static qboolean useChroma;

static fbo_t offscreen[2];
static int currentFrame = 0;

r_attrib_t distAttribs[] = {
	{"Position",0},
	{"TexCoord",2},
	{"Color",4},
	{NULL,0}
};

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_norm = {
	0, 
	// vertex shader
	"vr/rift.vert",
	// fragment shader
	"vr/rift.frag",
	distAttribs
};

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_warp = {
	0, 
	// vertex shader
	"vr/rift_timewarp.vert",
	// fragment shader
	"vr/rift.frag",
	distAttribs
};


r_attrib_t chromaAttribs[] = {
	{"Position",0},
	{"TexCoord0",1},
	{"TexCoord1",2},
	{"TexCoord2",3},
	{"Color",4},
	{NULL,0}
};

// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t ovr_shader_chrm = {
	0, 
	// vertex shader
	"vr/rift_chromatic.vert",
	// fragment shader
	"vr/rift_chromatic.frag",
	chromaAttribs
};


// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t ovr_shader_chrm_warp = {
	0, 
	// vertex shader
	"vr/rift_chromatic_timewarp.vert",
	// fragment shader
	"vr/rift_chromatic.frag",
	chromaAttribs
};

typedef struct {
	ovrVector2f pos;
	ovrVector2f texR;
	ovrVector2f texG;
	ovrVector2f texB;
	GLubyte color[4];
} ovr_vert_t;


static attribs_t distortion_attribs[5] = {
	{0, 2, GL_FLOAT, GL_FALSE, sizeof(ovr_vert_t), 0},
	{1, 2, GL_FLOAT, GL_FALSE, sizeof(ovr_vert_t), sizeof(ovrVector2f)},
	{2, 2, GL_FLOAT, GL_FALSE, sizeof(ovr_vert_t), sizeof(ovrVector2f) * 2},
	{3, 2, GL_FLOAT, GL_FALSE, sizeof(ovr_vert_t), sizeof(ovrVector2f) * 3},
	{4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ovr_vert_t), sizeof(ovrVector2f) * 4},
};


typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint currentFrame;
		GLuint lastFrame;
		GLuint EyeToSourceUVScale;
		GLuint EyeToSourceUVOffset;
		GLuint EyeRotationStart;
		GLuint EyeRotationEnd;
		GLuint OverdriveScales;
		GLuint VignetteFade;
		GLuint InverseResolution;
	} uniform;
} r_ovr_shader_t;

static r_ovr_shader_t ovr_distortion_shaders[2];
static r_ovr_shader_t ovr_timewarp_shaders[2];

// util function
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{
	GLint texloc;
	if (!object->program)
		R_CompileShaderFromFiles(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);
	shader->uniform.EyeToSourceUVOffset = glGetUniformLocation(shader->shader->program,"EyeToSourceUVOffset");
	shader->uniform.EyeToSourceUVScale = glGetUniformLocation(shader->shader->program,"EyeToSourceUVScale");
	shader->uniform.EyeRotationStart = glGetUniformLocation(shader->shader->program,"EyeRotationStart");
	shader->uniform.EyeRotationEnd = glGetUniformLocation(shader->shader->program,"EyeRotationEnd");

	shader->uniform.OverdriveScales = glGetUniformLocation(shader->shader->program,"OverdriveScales");
	shader->uniform.VignetteFade = glGetUniformLocation(shader->shader->program,"VignetteFade");
	shader->uniform.InverseResolution = glGetUniformLocation(shader->shader->program,"InverseResolution");
	
	texloc = glGetUniformLocation(shader->shader->program,"currentFrame");
	glUniform1i(texloc,0);
	shader->uniform.currentFrame = texloc;

	texloc = glGetUniformLocation(shader->shader->program,"lastFrame");
	glUniform1i(texloc,1);
	shader->uniform.lastFrame = texloc;

	glUseProgram(0);
}


void OVR_CalculateState(vr_param_t *state)
{
	vr_param_t ovrState;
	float ovrScale = vr_ovr_supersample->value;
	int eye = 0;

	for (eye = 0; eye < 2; eye++) {
		ovrDistortionMesh meshData;
		ovr_vert_t *mesh = NULL;
		ovr_vert_t *v = NULL;
		ovrDistortionVertex *ov = NULL;
		int i = 0;

		if (vr_ovr_maxfov->value)
		{
			renderInfo[eye].eyeFov = hmd->MaxEyeFov[eye];
		} else
		{
			renderInfo[eye].eyeFov = hmd->DefaultEyeFov[eye];
		}

		ovrState.eyeFBO[eye] = &renderInfo[eye].eyeFBO;

		ovrState.renderParams[eye].projection.x.scale = 2.0f / ( renderInfo[eye].eyeFov.LeftTan + renderInfo[eye].eyeFov.RightTan );
		ovrState.renderParams[eye].projection.x.offset = ( renderInfo[eye].eyeFov.LeftTan - renderInfo[eye].eyeFov.RightTan ) * ovrState.renderParams[eye].projection.x.scale * 0.5f;
		ovrState.renderParams[eye].projection.y.scale = 2.0f / ( renderInfo[eye].eyeFov.UpTan + renderInfo[eye].eyeFov.DownTan );
		ovrState.renderParams[eye].projection.y.offset = ( renderInfo[eye].eyeFov.UpTan - renderInfo[eye].eyeFov.DownTan ) * ovrState.renderParams[eye].projection.y.scale * 0.5f;

		// set up rendering info
		eyeDesc[eye] = ovrHmd_GetRenderDesc(hmd,(ovrEyeType) eye,renderInfo[eye].eyeFov);

		VectorSet(ovrState.renderParams[eye].viewOffset,
			-eyeDesc[eye].HmdToEyeViewOffset.x,
			eyeDesc[eye].HmdToEyeViewOffset.y,
			eyeDesc[eye].HmdToEyeViewOffset.z);

		ovrHmd_CreateDistortionMesh(hmd, eyeDesc[eye].Eye, eyeDesc[eye].Fov, ovrDistortionCap_Chromatic | ovrDistortionCap_SRGB | ovrDistortionCap_TimeWarp | ovrDistortionCap_Vignette, &meshData);

		mesh = (ovr_vert_t *) malloc(sizeof(ovr_vert_t) * meshData.VertexCount);
		v = mesh;
		ov = meshData.pVertexData; 
		for (i = 0; i < meshData.VertexCount; i++)
		{
			v->pos.x = ov->ScreenPosNDC.x;
			v->pos.y = ov->ScreenPosNDC.y;

			v->texR = (*(ovrVector2f*)&ov->TanEyeAnglesR); 
			v->texG = (*(ovrVector2f*)&ov->TanEyeAnglesG);
			v->texB = (*(ovrVector2f*)&ov->TanEyeAnglesB); 
			v->color[0] = v->color[1] = v->color[2] = (GLubyte)( ov->VignetteFactor * 255.99f );
			v->color[3] = (GLubyte)( ov->TimeWarpFactor * 255.99f );
			v++; ov++;
		}

		R_BindIVBO(&renderInfo[eye].eye,NULL,0);
		R_VertexData(&renderInfo[eye].eye,sizeof(ovr_vert_t) * meshData.VertexCount, mesh);
		R_IndexData(&renderInfo[eye].eye,GL_TRIANGLES,GL_UNSIGNED_SHORT,meshData.IndexCount,sizeof(unsigned short) * meshData.IndexCount,meshData.pIndexData);
		R_ReleaseIVBO();
		free(mesh);
		ovrHmd_DestroyDistortionMesh( &meshData );
	}
	{
		// calculate this to give the engine a rough idea of the fov
		float combinedTanHalfFovHorizontal = max ( max ( renderInfo[0].eyeFov.LeftTan, renderInfo[0].eyeFov.RightTan ), max ( renderInfo[1].eyeFov.LeftTan, renderInfo[1].eyeFov.RightTan ) );
		float combinedTanHalfFovVertical = max ( max ( renderInfo[0].eyeFov.UpTan, renderInfo[0].eyeFov.DownTan ), max ( renderInfo[1].eyeFov.UpTan, renderInfo[1].eyeFov.DownTan ) );
		float horizontalFullFovInRadians = 2.0f * atanf ( combinedTanHalfFovHorizontal ); 
		float fovX = RAD2DEG(horizontalFullFovInRadians);
		float fovY = RAD2DEG(2.0 * atanf(combinedTanHalfFovVertical));
		ovrState.aspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
		ovrState.viewFovY = fovY;
		ovrState.viewFovX = fovX;
		ovrState.pixelScale = ovrScale * vid.width / (float) hmd->Resolution.w;
	}

	*state = ovrState;
}


void OVR_FrameStart(int32_t changeBackBuffers)
{
	if (vr_ovr_maxfov->modified)
	{
		int newValue =  vr_ovr_maxfov->value ? 1 : 0;
		if (newValue != vr_ovr_maxfov->value)
			Cvar_SetInteger("vr_ovr_maxfov",newValue);
		changeBackBuffers = 1;
		vr_ovr_maxfov->modified = (qboolean) false;
	}

	if (vr_ovr_supersample->modified)
	{
		if (vr_ovr_supersample->value < 1.0)
			Cvar_Set("vr_ovr_supersample", "1.0");
		else if (vr_ovr_supersample->value > 2.0)
			Cvar_Set("vr_ovr_supersample", "2.0");
		changeBackBuffers = 1;
		vr_ovr_supersample->modified = false;
	}
	if (useChroma != (qboolean) !!vr_chromatic->value)
	{
		useChroma = (qboolean) !!vr_chromatic->value;
	}

	if (vr_ovr_lumoverdrive->modified)
	{
		changeBackBuffers = 1;
		currentFrame = 0;
		vr_ovr_lumoverdrive->modified = false;
	}

	if (changeBackBuffers)
	{
		int i;
		float width, height;
		float ovrScale;

		OVR_CalculateState(&currentState);


		width = glConfig.screen_width / (float) hmd->Resolution.w;
		height = glConfig.screen_height / (float) hmd->Resolution.h;
		ovrScale = (width + height) / 2.0;
		ovrScale *= R_AntialiasGetScale() * vr_ovr_supersample->value;
		if (vr_ovr_debug->value)
			Com_Printf("VR_OVR: Set render target scale to %.2f\n",ovrScale);
		for (i = 0; i < 2; i++)
		{
			ovrRecti viewport = {0,0, 0,0};
			renderInfo[i].renderTarget = ovrHmd_GetFovTextureSize(hmd, (ovrEyeType) i, renderInfo[i].eyeFov, ovrScale);
			viewport.Size.w = renderInfo[i].renderTarget.w;
			viewport.Size.h = renderInfo[i].renderTarget.h;
			ovrHmd_GetRenderScaleAndOffset(renderInfo[i].eyeFov, renderInfo[i].renderTarget, viewport, (ovrVector2f*) renderInfo[i].UVScaleOffset);

			if (renderInfo[i].renderTarget.w != renderInfo[i].eyeFBO.width || renderInfo[i].renderTarget.h != renderInfo[i].eyeFBO.height)
			{
				if (vr_ovr_debug->value)
					Com_Printf("VR_OVR: Set buffer %i to size %i x %i\n",i,renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h);
				R_ResizeFBO(renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h, 1, GL_RGBA8, &renderInfo[i].eyeFBO);
				R_ClearFBO(&renderInfo[i].eyeFBO);
			}

		}
	}
}

void OVR_GetState(vr_param_t *state)
{
	*state = currentState;
	state->offscreen = &offscreen[currentFrame];
}

void R_Clear (void);
void OVR_Present(qboolean loading)
{
	float fade = vr_ovr_distortion_fade->value > 0.0 ? 1.0f : 0.0f;
	GL_ClearColor(0.0, 0.0, 0.0, 1.0);
	R_Clear();
	GL_SetDefaultClearColor();	
	{
		int i = 0;
		r_ovr_shader_t *currentShader;

		qboolean warp =(qboolean) (!loading && withinFrame && vr_ovr_timewarp->value);
		if (warp)
		{
			currentShader = &ovr_timewarp_shaders[useChroma];	
			ovr_WaitTillTime(frameTime.TimewarpPointSeconds);
		} else {
			currentShader = &ovr_distortion_shaders[useChroma];	
		}

		//WARNING!!! These matrices are transposed in SetUniform4x4f, before being used by the shader.

		glDisableClientState (GL_COLOR_ARRAY);
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		glDisableClientState (GL_VERTEX_ARRAY);
		glEnableVertexAttribArray (0);
		glEnableVertexAttribArray (1);
		glEnableVertexAttribArray (2);
		glEnableVertexAttribArray (3);
		glEnableVertexAttribArray (4);

		glUseProgram(currentShader->shader->program);

		if (hmd->Type >= ovrHmd_DK2 && vr_ovr_lumoverdrive->value)
		{
			int lastFrame = (currentFrame ? 0 : 1);
			static float overdriveScaleRegularRise = 0.1f;
			static float overdriveScaleRegularFall = 0.05f;	// falling issues are hardly visible

			GL_MBind(1,offscreen[lastFrame].texture);
			glUniform2f(currentShader->uniform.OverdriveScales,overdriveScaleRegularRise, overdriveScaleRegularFall);
		} else {
			glUniform2f(currentShader->uniform.OverdriveScales,0,0);
		}
		glUniform2f(currentShader->uniform.InverseResolution,1.0/glState.currentFBO->width,1.0/glState.currentFBO->height);
		glUniform1f(currentShader->uniform.VignetteFade,fade);

		for (i = 0; i < 2; i++)
		{
			// hook for rendering in different order
			int eye = i;
			GL_MBind(0,renderInfo[eye].eyeFBO.texture);
			R_BindIVBO(&renderInfo[eye].eye,distortion_attribs,5);

			glUniform2f(currentShader->uniform.EyeToSourceUVScale,
				renderInfo[eye].UVScaleOffset[0].x, renderInfo[eye].UVScaleOffset[0].y);

			glUniform2f(currentShader->uniform.EyeToSourceUVOffset,
				renderInfo[eye].UVScaleOffset[1].x, renderInfo[eye].UVScaleOffset[1].y);

			if (warp)
			{
				ovrPosef framePose = trackingState.HeadPose.ThePose;
				ovrMatrix4f timeWarpMatrices[2];
				ovrHmd_GetEyeTimewarpMatrices(hmd, (ovrEyeType)eye, framePose, timeWarpMatrices);
				glUniformMatrix4fv(currentShader->uniform.EyeRotationStart,1,GL_TRUE,(GLfloat *) timeWarpMatrices[0].M);
				glUniformMatrix4fv(currentShader->uniform.EyeRotationEnd,1,GL_TRUE,(GLfloat *) timeWarpMatrices[1].M);
			}

			R_DrawIVBO(&renderInfo[eye].eye);
			R_ReleaseIVBO();
		}

		if (vr_ovr_lumoverdrive->value)
		{
			GL_MBind(1,0);
			currentFrame = (currentFrame ? 0 : 1);
		}

		GL_MBind(0,0);
		glUseProgram(0);

		glDisableVertexAttribArray (0);
		glDisableVertexAttribArray (1);
		glDisableVertexAttribArray (2);
		glDisableVertexAttribArray (3);
		glDisableVertexAttribArray (4);

		glEnableClientState (GL_COLOR_ARRAY);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glEnableClientState (GL_VERTEX_ARRAY);

		//		glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
		//		glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);

	}

}

void OVR_PostPresent(void)
{
	vec4_t debugColor = {1.0,1.0,1.0,1.0};
	if (VR_OVR_RenderLatencyTest(debugColor))
	{
		//Com_Printf("VR_OVR: Debug color ( %.2f, %.2f, %.2f)\n",debugColor[0],debugColor[1],debugColor[2]);
		glColor4fv(debugColor);

		if (hmd->Type < ovrHmd_DK2)
		{
			glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(0.3, -0.4);
			glVertex2f(0.3, 0.4);
			glVertex2f(0.7, -0.4);
			glVertex2f(0.7, 0.4); 
			glEnd();

			glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(-0.3, -0.4);
			glVertex2f(-0.3, 0.4);
			glVertex2f(-0.7, -0.4);
			glVertex2f(-0.7, 0.4); 
			glEnd();

		} else {
			float resX = 2.0 / glConfig.screen_width;
			float resY = 2.0 / glConfig.screen_height;
			float x = 1.0 - 10 * resX;
			float y = 1.0 - 10 * resY;


			glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(x, y);
			glVertex2f(x, 1.0);
			glVertex2f(1.0, y);
			glVertex2f(1.0, 1.0); 
			glEnd();
		}
		glColor4f(1.0,1.0,1.0,1.0);
	}

}

int32_t OVR_Enable(void)
{
	int i;
	if (!glConfig.arb_texture_float)
		return 0;

	if (hmd && !(hmd->HmdCaps & ovrHmdCap_ExtendDesktop))
	{
#ifdef WIN32
		ovrHmd_AttachToWindow(hmd,mainWindowInfo.info.win.window,NULL,NULL);
#endif
	}

	for (i = 0; i < 2; i++)
	{
		if (renderInfo[i].eyeFBO.valid)
			R_DelFBO(&renderInfo[i].eyeFBO);
		if (offscreen[i].valid)
			R_DelFBO(&offscreen[i]);
	}

	R_CreateIVBO(&renderInfo[0].eye,GL_STATIC_DRAW);
	R_CreateIVBO(&renderInfo[1].eye,GL_STATIC_DRAW);

	//VR_FrameStart(1);

	VR_OVR_InitShader(&ovr_distortion_shaders[0],&ovr_shader_norm);
	VR_OVR_InitShader(&ovr_distortion_shaders[1],&ovr_shader_chrm);

	VR_OVR_InitShader(&ovr_timewarp_shaders[0],&ovr_shader_warp);
	VR_OVR_InitShader(&ovr_timewarp_shaders[1],&ovr_shader_chrm_warp);
	//OVR_FrameStart(true);
	Cvar_ForceSet("vr_hmdstring",(char *)hmd->ProductName);
	return true;
}

void OVR_Disable(void)
{
	int i;

	R_DelShaderProgram(&ovr_shader_norm);
	R_DelShaderProgram(&ovr_shader_chrm);
	R_DelShaderProgram(&ovr_shader_warp);
	R_DelShaderProgram(&ovr_shader_chrm_warp);


	for (i = 0; i < 2; i++)
	{
		if (renderInfo[i].eyeFBO.valid)
			R_DelFBO(&renderInfo[i].eyeFBO);
		if (offscreen[i].valid)
			R_DelFBO(&offscreen[i]);
		R_DelIVBO(&renderInfo[i].eye);
	}
}

int32_t OVR_Init(void)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		R_InitFBO(&renderInfo[i].eyeFBO);
		R_InitFBO(&offscreen[i]);
		R_InitIVBO(&renderInfo[i].eye);

	}

	return true;
}

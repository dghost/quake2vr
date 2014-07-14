#include "include/r_vr_ovr.h"
#include "../vr/include/vr_ovr.h"
#include "include/r_local.h"
#include "OVR_CAPI.h"



void OVR_FrameStart(int32_t changeBackBuffers);
void OVR_BindView(vr_eye_t eye);
void OVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect);
void OVR_Present(qboolean loading);
int32_t OVR_Enable();
void OVR_Disable();
int32_t OVR_Init();
void OVR_GetState(vr_param_t *state);

hmd_render_t vr_render_ovr = 
{
	HMD_RIFT,
	OVR_Init,
	OVR_Enable,
	OVR_Disable,
	OVR_BindView,
	OVR_GetViewRect,
	OVR_FrameStart,
	OVR_GetState,
	OVR_Present
};

extern ovrHmd hmd;
extern ovrHmdDesc hmdDesc;
extern ovrEyeRenderDesc eyeDesc[2];
extern ovrPosef framePose;
extern ovrFrameTiming frameTime;
extern qboolean withinFrame;


extern void VR_OVR_GetFOV(float *fovx, float *fovy);
extern int32_t VR_OVR_RenderLatencyTest(vec4_t color);


static vr_param_t currentState;

// this should probably be rearranged
static fbo_t eyeFBO[2];
static vbo_t eyes[2];
static ovrSizei renderTargets[2];
static ovrFovPort eyeFov[2]; 
static ovrVector2f UVScaleOffset[2][2];

static qboolean useChroma;

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
		GLuint tex;
		GLuint EyeToSourceUVScale;
		GLuint EyeToSourceUVOffset;
		GLuint EyeRotationStart;
		GLuint EyeRotationEnd;
	} uniform;

} r_ovr_shader_t;

static r_ovr_shader_t ovr_distortion_shaders[2];
static r_ovr_shader_t ovr_timewarp_shaders[2];

// util function
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{
	if (!object->program)
		R_CompileShaderFromFiles(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);
	shader->uniform.EyeToSourceUVOffset = glGetUniformLocation(shader->shader->program,"EyeToSourceUVOffset");
	shader->uniform.EyeToSourceUVScale = glGetUniformLocation(shader->shader->program,"EyeToSourceUVScale");
	shader->uniform.EyeRotationStart = glGetUniformLocation(shader->shader->program,"EyeRotationStart");
	shader->uniform.EyeRotationEnd = glGetUniformLocation(shader->shader->program,"EyeRotationEnd");

	shader->uniform.tex = glGetUniformLocation(shader->shader->program,"tex");
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
			eyeFov[eye] = hmdDesc.MaxEyeFov[eye];
		} else
		{
			eyeFov[eye] = hmdDesc.DefaultEyeFov[eye];
		}

		ovrState.eyeFBO[eye] = &eyeFBO[eye];

		ovrState.renderParams[eye].projection.x.scale = 2.0f / ( eyeFov[eye].LeftTan + eyeFov[eye].RightTan );
		ovrState.renderParams[eye].projection.x.offset = ( eyeFov[eye].LeftTan - eyeFov[eye].RightTan ) * ovrState.renderParams[eye].projection.x.scale * 0.5f;
		ovrState.renderParams[eye].projection.y.scale = 2.0f / ( eyeFov[eye].UpTan + eyeFov[eye].DownTan );
		ovrState.renderParams[eye].projection.y.offset = ( eyeFov[eye].UpTan - eyeFov[eye].DownTan ) * ovrState.renderParams[eye].projection.y.scale * 0.5f;

		// set up rendering info
		eyeDesc[eye] = ovrHmd_GetRenderDesc(hmd,(ovrEyeType) eye,eyeFov[eye]);

		VectorSet(ovrState.renderParams[eye].viewOffset,
			-eyeDesc[eye].ViewAdjust.x,
			eyeDesc[eye].ViewAdjust.y,
			eyeDesc[eye].ViewAdjust.z);

		ovrHmd_CreateDistortionMesh(hmd, eyeDesc[eye].Eye, eyeDesc[eye].Fov, ovrDistortionCap_Chromatic, &meshData);

		mesh = (ovr_vert_t *) malloc(sizeof(ovr_vert_t) * meshData.VertexCount);
		v = mesh;
		ov = meshData.pVertexData; 
		for (i = 0; i < meshData.VertexCount; i++)
		{
			v->pos.x = ov->Pos.x;
			v->pos.y = ov->Pos.y;

			v->texR = (*(ovrVector2f*)&ov->TexR); 
			v->texG = (*(ovrVector2f*)&ov->TexG);
			v->texB = (*(ovrVector2f*)&ov->TexB); 
			v->color[0] = v->color[1] = v->color[2] = (GLubyte)( ov->VignetteFactor * 255.99f );
			v->color[3] = (GLubyte)( ov->TimeWarpFactor * 255.99f );
			v++; ov++;
		}

		R_BindIVBO(&eyes[eye],NULL,0);
		R_VertexData(&eyes[eye],sizeof(ovr_vert_t) * meshData.VertexCount, mesh);
		R_IndexData(&eyes[eye],GL_TRIANGLES,GL_UNSIGNED_SHORT,meshData.IndexCount,sizeof(unsigned short) * meshData.IndexCount,meshData.pIndexData);
		R_ReleaseIVBO();
		free(mesh);
		ovrHmd_DestroyDistortionMesh( &meshData );
	}
	{
		// calculate this to give the engine a rough idea of the fov
		float combinedTanHalfFovHorizontal = max ( max ( eyeFov[0].LeftTan, eyeFov[0].RightTan ), max ( eyeFov[1].LeftTan, eyeFov[1].RightTan ) );
		float combinedTanHalfFovVertical = max ( max ( eyeFov[0].UpTan, eyeFov[0].DownTan ), max ( eyeFov[1].UpTan, eyeFov[1].DownTan ) );
		float horizontalFullFovInRadians = 2.0f * atanf ( combinedTanHalfFovHorizontal ); 
		float fovX = RAD2DEG(horizontalFullFovInRadians);
		float fovY = RAD2DEG(2.0 * atanf(combinedTanHalfFovVertical));
		ovrState.aspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
		ovrState.viewFovY = fovY;
		ovrState.viewFovX = fovX;
		ovrState.pixelScale = ovrScale * vid.width / (float) hmdDesc.Resolution.w;
	}

	ovrState.eyeFBO[1] = &eyeFBO[1];
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

	if (changeBackBuffers)
	{
		int i;
		float ovrScale;

		OVR_CalculateState(&currentState);

		ovrScale = (r_antialias->value == ANTIALIAS_4X_FSAA ? 2.0 : 1.0);
		ovrScale *= vr_ovr_supersample->value;
		for (i = 0; i < 2; i++)
		{
			ovrRecti viewport = {0,0, 0,0};
			renderTargets[i] = ovrHmd_GetFovTextureSize(hmd, (ovrEyeType) i, eyeFov[i], ovrScale);
			viewport.Size.w = renderTargets[i].w;
			viewport.Size.h = renderTargets[i].h;
			ovrHmd_GetRenderScaleAndOffset(eyeFov[i], renderTargets[i], viewport, (ovrVector2f*) UVScaleOffset[i]);

			if (renderTargets[i].w != eyeFBO[i].width || renderTargets[i].h != eyeFBO[i].height)
			{
				Com_Printf("VR_OVR: Set buffer %i to size %i x %i\n",i,renderTargets[i].w, renderTargets[i].h);
				R_ResizeFBO(renderTargets[i].w, renderTargets[i].h, 1, GL_RGBA8, &eyeFBO[i]);
			}

		}
	}
}


void OVR_BindView(vr_eye_t eye)
{	
	switch(eye)
	{
	case EYE_LEFT:
		vid.height = eyeFBO[0].height;
		vid.width = eyeFBO[0].width;
		R_BindFBO(&eyeFBO[0]);
		break;
	case EYE_RIGHT:
		vid.height = eyeFBO[1].height;
		vid.width = eyeFBO[1].width;
		R_BindFBO(&eyeFBO[1]);
		break;
	default:
		return;
	}
}

void OVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect)
{
	vr_rect_t result;
	result.x = 0;
	result.y = 0;
	
	if (eye == EYE_LEFT)
	{
		result.width = renderTargets[0].w;
		result.height = renderTargets[0].h;
	}
	else
	{
		result.width = renderTargets[1].w;
		result.height = renderTargets[1].h;
	}
	*rect = result;
}

void OVR_GetState(vr_param_t *state)
{
	*state = currentState;
}


void OVR_Present(qboolean loading)
{
	vec4_t debugColor;
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
		glUniform1i(currentShader->uniform.tex,0);

		for (i = 0; i < 2; i++)
		{
			// hook for rendering in different order
			int eye = i;

			glUniform2f(currentShader->uniform.EyeToSourceUVScale,
				UVScaleOffset[eye][0].x, UVScaleOffset[eye][0].y);

			glUniform2f(currentShader->uniform.EyeToSourceUVOffset,
				UVScaleOffset[eye][1].x, UVScaleOffset[eye][1].y);

			if (warp)
			{
				ovrMatrix4f timeWarpMatrices[2];
				ovrHmd_GetEyeTimewarpMatrices(hmd, (ovrEyeType)eye, framePose, timeWarpMatrices);
				glUniformMatrix4fv(currentShader->uniform.EyeRotationStart,1,GL_TRUE,(GLfloat *) timeWarpMatrices[0].M);
				glUniformMatrix4fv(currentShader->uniform.EyeRotationEnd,1,GL_TRUE,(GLfloat *) timeWarpMatrices[1].M);
			}
			GL_MBind(0,eyeFBO[eye].texture);
			R_BindIVBO(&eyes[eye],distortion_attribs,5);

			R_DrawIVBO(&eyes[eye]);
			R_ReleaseIVBO();
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
	if (VR_OVR_RenderLatencyTest(debugColor))
	{
		glColor4fv(debugColor);

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

		glColor4f(1.0,1.0,1.0,1.0);
	}

}


int32_t OVR_Enable()
{
	if (!glConfig.arb_texture_float)
		return 0;
	if (eyeFBO[0].valid)
		R_DelFBO(&eyeFBO[0]);
	if (eyeFBO[1].valid)
		R_DelFBO(&eyeFBO[1]);

	R_CreateIVBO(&eyes[0],GL_STATIC_DRAW);
	R_CreateIVBO(&eyes[1],GL_STATIC_DRAW);

	//VR_FrameStart(1);

	VR_OVR_InitShader(&ovr_distortion_shaders[0],&ovr_shader_norm);
	VR_OVR_InitShader(&ovr_distortion_shaders[1],&ovr_shader_chrm);

	VR_OVR_InitShader(&ovr_timewarp_shaders[0],&ovr_shader_warp);
	VR_OVR_InitShader(&ovr_timewarp_shaders[1],&ovr_shader_chrm_warp);
	//OVR_FrameStart(true);
	Cvar_ForceSet("vr_hmdstring",(char *)hmdDesc.ProductName);
	return true;
}

void OVR_Disable()
{

	R_DelShaderProgram(&ovr_shader_norm);
	R_DelShaderProgram(&ovr_shader_chrm);
	R_DelShaderProgram(&ovr_shader_warp);
	R_DelShaderProgram(&ovr_shader_chrm_warp);

	if (eyeFBO[0].valid)
		R_DelFBO(&eyeFBO[0]);
	if (eyeFBO[1].valid)
		R_DelFBO(&eyeFBO[1]);

	R_DelIVBO(&eyes[0]);
	R_DelIVBO(&eyes[1]);
}

int32_t OVR_Init()
{
	R_InitFBO(&eyeFBO[0]);
	R_InitFBO(&eyeFBO[1]);
	R_InitIVBO(&eyes[0]);
	R_InitIVBO(&eyes[1]);
	return true;
}
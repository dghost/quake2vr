#include "include/r_vr_ovr.h"
#include "../vr/include/vr_ovr.h"
#include "include/r_local.h"
#include "OVR_CAPI.h"



void OVR_FrameStart(int32_t changeBackBuffers);
void OVR_BindView(vr_eye_t eye);
void OVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect);
void OVR_Present();
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
ovrFovPort eyeFov[2]; 
ovrVector2f UVScaleOffset[2][2];

extern void VR_OVR_CalcRenderParam();
extern float VR_OVR_GetDistortionScale();
extern void VR_OVR_GetFOV(float *fovx, float *fovy);
extern int32_t VR_OVR_RenderLatencyTest(vec4_t color);


static vr_param_t currentState;

//static fbo_t world;
static fbo_t left, right;


static vbo_t eyes[2];

static ovrSizei renderTargets[2];

static qboolean useBilinear, useChroma;

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_dist_norm = {
	0, 
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/ovrdistort.frag",
	NULL
};

r_attrib_t chromaAttribs[] = {
	{"Position",0},
	{"TexCoord0",1},
	{"TexCoord1",2},
	{"TexCoord2",3},
	{"Color",4},
	{NULL,NULL}
	};

// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t ovr_shader_dist_chrm = {
	0, 
	// vertex shader (identity)
	"vr/libovr.vert",
	// fragment shader
	"vr/libovr.frag",
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
		GLuint scale;
		GLuint scale_in;
		GLuint lens_center;
		GLuint texture_size;
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
		GLuint tex;
		GLuint dist;
		GLuint EyeToSourceUVScale;
		GLuint EyeToSourceUVOffset;
	} uniform;

} r_ovr_shader_t;

typedef enum {
	OVR_FILTER_BILINEAR,
	OVR_FILTER_WEIGHTED_BILINEAR,
	OVR_FILTER_BICUBIC,
	NUM_OVR_FILTER_MODES
} ovr_filtermode_t;

static r_ovr_shader_t ovr_distortion_shaders[2];

// util function
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{
	if (!object->program)
		R_CompileShaderFromFiles(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);
	shader->uniform.EyeToSourceUVOffset = glGetUniformLocation(shader->shader->program,"EyeToSourceUVOffset");
	shader->uniform.EyeToSourceUVScale = glGetUniformLocation(shader->shader->program,"EyeToSourceUVScale");

	shader->uniform.scale = glGetUniformLocation(shader->shader->program, "scale");
	shader->uniform.scale_in = glGetUniformLocation(shader->shader->program, "scaleIn");
	shader->uniform.lens_center = glGetUniformLocation(shader->shader->program, "lensCenter");
	shader->uniform.hmd_warp_param = glGetUniformLocation(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = glGetUniformLocation(shader->shader->program, "chromAbParam");
	shader->uniform.texture_size = glGetUniformLocation(shader->shader->program,"textureSize");
	shader->uniform.tex = glGetUniformLocation(shader->shader->program,"tex");
	shader->uniform.dist = glGetUniformLocation(shader->shader->program,"dist");
	glUseProgram(0);
}

void OVR_FrameStart(int32_t changeBackBuffers)
{


	if (vr_ovr_distortion->modified)
	{
		if (vr_ovr_distortion->value > 3)
			Cvar_SetInteger("vr_ovr_distortion", 3);
		else if (vr_ovr_distortion->value < -1)
			Cvar_SetInteger("vr_ovr_distortion", -1);
		changeBackBuffers = 1;
		vr_ovr_distortion->modified = false;
	}

	if (vr_ovr_scale->modified)
	{
		if (vr_ovr_scale->value < 1.0)
			Cvar_Set("vr_ovr_scale", "1.0");
		else if (vr_ovr_scale->value > 2.0)
			Cvar_Set("vr_ovr_scale", "2.0");
		changeBackBuffers = 1;
		vr_ovr_scale->modified = false;
	}

	if (vr_ovr_autoscale->modified)
	{
		if (vr_ovr_autoscale->value < 0.0)
			Cvar_SetInteger("vr_ovr_autoscale", 0);
		else if (vr_ovr_autoscale->value > 2.0)
			Cvar_SetInteger("vr_ovr_autoscale",2);
		else
			Cvar_SetInteger("vr_ovr_autoscale", (int32_t) vr_ovr_autoscale->value);
		changeBackBuffers = 1;
		vr_ovr_autoscale->modified = false;
	}

	if (vr_ovr_lensdistance->modified)
	{
		if (vr_ovr_lensdistance->value < 0.0)
			Cvar_SetToDefault("vr_ovr_lensdistance");
		else if (vr_ovr_lensdistance->value > 100.0)
			Cvar_Set("vr_ovr_lensdistance", "100.0");
		changeBackBuffers = 1;
		VR_OVR_CalcRenderParam();
		vr_ovr_lensdistance->modified = false;
	}

	if (vr_ovr_autolensdistance->modified)
	{
		Cvar_SetValue("vr_ovr_autolensdistance",!!vr_ovr_autolensdistance->value);
		changeBackBuffers = 1;
		VR_OVR_CalcRenderParam();
		vr_ovr_autolensdistance->modified = false;
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
		changeBackBuffers = 1;
	}

	if (vr_ovr_filtermode->modified)
	{
		switch ((int32_t) vr_ovr_filtermode->value)
		{
		default:
		case OVR_FILTER_BILINEAR:
		case OVR_FILTER_BICUBIC:
			useBilinear = 1;
			break;
		case OVR_FILTER_WEIGHTED_BILINEAR:
			useBilinear = 0;
			break;
		}
//		Com_Printf("OVR: %s bilinear texture filtering\n", useBilinear ? "Using" : "Not using");
		R_SetFBOFilter(useBilinear,&left);
		R_SetFBOFilter(useBilinear,&right);
		vr_ovr_filtermode->modified = false;
	}

	if (changeBackBuffers)
	{
		ovrSizei recommendedTex0Size, recommendedTex1Size;
		float ovrScale = vr_ovr_supersample->value;

		recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmdDesc.DefaultEyeFov[0], 1.0); 
		recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmdDesc.DefaultEyeFov[1], 1.0);
		
		renderTargets[0].w = ovrScale * recommendedTex0Size.w;
		renderTargets[0].h = ovrScale  * recommendedTex0Size.h;

		renderTargets[1].w = ovrScale * recommendedTex1Size.w;
		renderTargets[1].h = ovrScale  * recommendedTex1Size.h;

	}
}


void OVR_BindView(vr_eye_t eye)
{
	
	switch(eye)
	{
	case EYE_LEFT:
		if (renderTargets[0].w != left.width || renderTargets[0].h != left.height)
		{
			R_ResizeFBO(renderTargets[0].w, renderTargets[0].h, useBilinear, GL_RGBA8, &left);
			Com_Printf("OVR: Set left render target size to %ux%u\n",left.width,left.height);
		}
		vid.height = left.height;
		vid.width = left.width;
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:
		if (renderTargets[1].w != right.width || renderTargets[1].h != right.height)
		{
			R_ResizeFBO(renderTargets[1].w, renderTargets[1].h, useBilinear, GL_RGBA8, &right);
			Com_Printf("OVR: Set right render target size to %ux%u\n",right.width, right.height);
		}
		vid.height = right.height;
		vid.width = right.width;
		R_BindFBO(&right);
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


void OVR_CalculateState(vr_param_t *state)
{
	vr_param_t ovrState;
	float ovrScale = vr_ovr_supersample->value;

	ovrFovPort fovLeft = eyeFov[0] = hmdDesc.DefaultEyeFov[ovrEye_Left]; 
	ovrFovPort fovRight = eyeFov[1] = hmdDesc.DefaultEyeFov[ovrEye_Right];
	int eye = 0;
	// calculate this to give the engine a rough idea of the fov
	float combinedTanHalfFovHorizontal = max ( max ( fovLeft.LeftTan, fovLeft.RightTan ), max ( fovRight.LeftTan, fovRight.RightTan ) );
	float combinedTanHalfFovVertical = max ( max ( fovLeft.UpTan, fovLeft.DownTan ), max ( fovRight.UpTan, fovRight.DownTan ) );
	float horizontalFullFovInRadians = 2.0f * atanf ( combinedTanHalfFovHorizontal ); 
	float fovY = RAD2DEG(horizontalFullFovInRadians);
	ovrState.aspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
	ovrState.viewFovX = fovY * ovrState.aspect;
	ovrState.viewFovY = fovY;

	ovrState.leftScaleOffset.x.scale = 2.0f / ( fovLeft.LeftTan + fovLeft.RightTan );
	ovrState.leftScaleOffset.x.offset = ( fovLeft.LeftTan - fovLeft.RightTan ) * ovrState.leftScaleOffset.x.scale * 0.5f;
    ovrState.leftScaleOffset.y.scale = 2.0f / ( fovLeft.UpTan + fovLeft.DownTan );
	ovrState.leftScaleOffset.y.offset = ( fovLeft.UpTan - fovLeft.DownTan ) * ovrState.leftScaleOffset.y.scale * 0.5f;

	ovrState.rightScaleOffset.x.scale = 2.0f / ( fovRight.LeftTan + fovRight.RightTan );
	ovrState.rightScaleOffset.x.offset = ( fovRight.LeftTan - fovRight.RightTan ) * ovrState.rightScaleOffset.x.scale * 0.5f;
    ovrState.rightScaleOffset.y.scale = 2.0f / ( fovRight.UpTan + fovRight.DownTan );
	ovrState.rightScaleOffset.y.offset = ( fovRight.UpTan - fovRight.DownTan ) * ovrState.rightScaleOffset.y.scale * 0.5f;	

	// set up rendering info
	eyeDesc[0] = ovrHmd_GetRenderDesc(hmd,ovrEye_Left,eyeFov[0]);
	eyeDesc[1] = ovrHmd_GetRenderDesc(hmd,ovrEye_Right,eyeFov[1]);

	VectorSet(ovrState.leftEyeAdjust,
		eyeDesc[ovrEye_Left].ViewAdjust.x,
		eyeDesc[ovrEye_Left].ViewAdjust.y,
		eyeDesc[ovrEye_Left].ViewAdjust.z);
	VectorSet(ovrState.rightEyeAdjust,
		eyeDesc[ovrEye_Right].ViewAdjust.x,
		eyeDesc[ovrEye_Right].ViewAdjust.y,
		eyeDesc[ovrEye_Right].ViewAdjust.z);

	ovrState.pixelScale = ovrScale * vid.width / (float) hmdDesc.Resolution.w;
	*state = ovrState;

	for (eye = 0; eye < 2; eye++)
	{
		ovrDistortionMesh meshData;
		ovr_vert_t *mesh = NULL;
		ovr_vert_t *v = NULL;
		ovrDistortionVertex *ov = NULL;

		int i = 0;
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
		R_ReleaseIVBO(&eyes[eye]);
		free(mesh);
		ovrHmd_DestroyDistortionMesh( &meshData );
	}

}

void OVR_GetState(vr_param_t *state)
{
	*state = currentState;
}


void OVR_Present()
{
	vec4_t debugColor;

	{

		ovrRecti viewport[] = {{ {0,0}, renderTargets[0].w, renderTargets[0].h},
								{ {0, 0 }, renderTargets[0].w, renderTargets[0].h}};
		ovrHmd_GetRenderScaleAndOffset(eyeDesc[0].Fov, renderTargets[0], viewport[0], (ovrVector2f*) UVScaleOffset[0]);
		ovrHmd_GetRenderScaleAndOffset(eyeDesc[1].Fov, renderTargets[1], viewport[1], (ovrVector2f*) UVScaleOffset[1]);
		glDisableClientState (GL_COLOR_ARRAY);
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		glDisableClientState (GL_VERTEX_ARRAY);

		glUseProgram(ovr_distortion_shaders[1].shader->program);
		glUniform2f(ovr_distortion_shaders[1].uniform.EyeToSourceUVScale,
			 UVScaleOffset[0][0].x, UVScaleOffset[0][0].y);
		
		glUniform2f(ovr_distortion_shaders[1].uniform.EyeToSourceUVOffset,
			 UVScaleOffset[0][1].x, UVScaleOffset[0][1].y);
		glUniform1i(ovr_distortion_shaders[1].uniform.tex,0);

		GL_MBind(0,left.texture);

		R_BindIVBO(&eyes[0],distortion_attribs,5);
		glEnableVertexAttribArray (0);
		glEnableVertexAttribArray (1);
		glEnableVertexAttribArray (2);
		glEnableVertexAttribArray (3);
		glEnableVertexAttribArray (4);
		R_DrawIVBO(&eyes[0]);

		GL_MBind(0,right.texture);
		glUniform2f(ovr_distortion_shaders[1].uniform.EyeToSourceUVScale,
			 UVScaleOffset[1][0].x, UVScaleOffset[1][0].y);
		
		glUniform2f(ovr_distortion_shaders[1].uniform.EyeToSourceUVOffset,
			 UVScaleOffset[1][1].x, UVScaleOffset[1][1].y);


		R_BindIVBO(&eyes[1],distortion_attribs, 5);
		glEnableVertexAttribArray (0);
		glEnableVertexAttribArray (1);
		glEnableVertexAttribArray (2);
		glEnableVertexAttribArray (3);
		glEnableVertexAttribArray (4);
		R_DrawIVBO(&eyes[1]);
		R_ReleaseIVBO(&eyes[1]);


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

		glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
		glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);

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
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

	R_CreateIVBO(&eyes[0],GL_STATIC_DRAW);
	R_CreateIVBO(&eyes[1],GL_STATIC_DRAW);

	OVR_CalculateState(&currentState);
	VR_OVR_InitShader(&ovr_distortion_shaders[0],&ovr_shader_dist_norm);
	VR_OVR_InitShader(&ovr_distortion_shaders[1],&ovr_shader_dist_chrm);
	//OVR_FrameStart(true);
	Cvar_ForceSet("vr_hmdstring",(char *)hmdDesc.ProductName);
	return true;
}

void OVR_Disable()
{

	R_DelShaderProgram(&ovr_shader_dist_norm);
	R_DelShaderProgram(&ovr_shader_dist_chrm);

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

	R_DelIVBO(&eyes[0]);
	R_DelIVBO(&eyes[1]);
}

int32_t OVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	R_InitIVBO(&eyes[0]);
	R_InitIVBO(&eyes[1]);
	return true;
}
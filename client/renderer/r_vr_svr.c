#include "include/r_local.h"
#include "include/r_vr.h"

#ifndef NO_STEAM

#include "include/r_vr_svr.h"
#include "../vr/include/vr_svr.h"
#include "../vr/include/vr_steamvr.h"

//static fbo_t world;
static fbo_t left, right;

static vr_rect_t renderTargetRect, origTargetRect;
static vr_rect_t leftRenderRect, rightRenderRect;

static GLuint leftDistortion[2], rightDistortion[2];

static qboolean chromatic;

extern svr_settings_t svr_settings;

extern vr_distort_shader_t vr_distort_shaders[2];
extern vr_distort_shader_t vr_bicubic_distort_shaders[2];

void SVR_BuildDistortionTextures()
{
	GLfloat *normalTexture, *chromaTexture;
	int width = glConfig.render_width; // width of the distortion map
	int height= glConfig.render_height; // height of the distortion map
	int bitsPerPixel = (glConfig.arb_texture_rg? 2 : 4);
	float scale = (1.0 / pow(2,vr_svr_distortion->value));
	
	width = (int) ((float) width * scale);
	height = (int) ((float) height * scale);
	
	Com_Printf("VR_SVR: Generating %dx%d distortion textures...\n",width,height);
	
	normalTexture = (GLfloat*)Z_TagMalloc(sizeof(GLfloat) * width * height * bitsPerPixel, TAG_RENDERER);
	chromaTexture = (GLfloat*)Z_TagMalloc(sizeof(GLfloat)* width * height * 4, TAG_RENDERER);

	if (SteamVR_GetDistortionTextures(SVR_Left,width,height,bitsPerPixel,normalTexture,chromaTexture))
	{

		GL_MBind(0,leftDistortion[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		if (bitsPerPixel == 2)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, normalTexture);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, normalTexture);

		GL_MBind(0,leftDistortion[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, chromaTexture);
	}

	if (SteamVR_GetDistortionTextures(SVR_Right,width,height,bitsPerPixel,normalTexture,chromaTexture))
	{
		GL_MBind(0,rightDistortion[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		if (bitsPerPixel == 2)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, normalTexture);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, normalTexture);

		GL_MBind(0,rightDistortion[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, chromaTexture);
	}
	GL_MBind(0,0);

	Z_Free(chromaTexture);
	Z_Free(normalTexture);
}

void SVR_SetOffscreenSize(uint32_t width, uint32_t height) {
	origTargetRect.x = 0;
	origTargetRect.y = 0;
	origTargetRect.width = width * svr_settings.scaleX;
	origTargetRect.height = height * svr_settings.scaleY;

	renderTargetRect = origTargetRect;

	if (renderTargetRect.width != left.width || renderTargetRect.height != left.height)
	{
		R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &left);
		R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &right);
	}

	Com_Printf("VR_SVR: Set render target size to %ux%u\n", renderTargetRect.width, renderTargetRect.height);
}


void SVR_FrameStart(void)
{


	if (vr_svr_distortion->modified)
	{
		if (vr_svr_distortion->value > 3)
			Cvar_SetInteger("vr_svr_distortion", 3);
		if (vr_svr_distortion->value <0)
			Cvar_SetInteger("vr_svr_distortion", 0);
		SVR_SetOffscreenSize(viddef.width, viddef.height);
		SVR_BuildDistortionTextures();
		vr_svr_distortion->modified = false;
	}

	if (chromatic != (qboolean) !!vr_chromatic->value)
	{
		chromatic = (qboolean) !!vr_chromatic->value;
	}
}

void SVR_GetState(vr_param_t *state)
{
	vr_param_t svrState;
	float ipd = svr_settings.ipd;
	int index = 0;

	svrState.aspect = svr_settings.aspect;
	svrState.viewFovX = svr_settings.viewFovX;
	svrState.viewFovY = svr_settings.viewFovY;
	
	for (index = 0; index < 2 ; index++)
	{
		int eyeSign = (-1 + index * 2);
		VectorSet(svrState.renderParams[index].viewOffset,ipd * eyeSign, 0.0, 0.0);

		svrState.renderParams[index].projection.y.scale = 1.0f / tanf((svrState.viewFovY / 2.0f) * M_PI / 180);
		svrState.renderParams[index].projection.y.offset = 0.0;
		svrState.renderParams[index].projection.x.scale = svrState.renderParams[0].projection.y.scale / svr_settings.aspect;
		svrState.renderParams[index].projection.x.offset = -eyeSign * svr_settings.projOffset;
	}

	svrState.pixelScale = 3.0;
	svrState.eyeFBO[0] = &left;
	svrState.eyeFBO[1] = &right;
	*state = svrState;
}

void R_Clear (void);
void SVR_Present(fbo_t *destination, qboolean loading)
{
    R_BindFBO(destination);

	if (!vr_svr_debug->value)
	{

		vr_distort_shader_t *current_shader = &vr_distort_shaders[chromatic];
 
		R_SetupQuadState();
		// draw left eye
		glUseProgram(current_shader->shader->program);
		glUniform1i(current_shader->uniform.texture,0);
		glUniform1i(current_shader->uniform.distortion,1);	

		GL_EnableMultitexture(true);
		GL_MBind(0,left.texture);
		GL_MBind(1,leftDistortion[chromatic]);
		glViewport(0,0,destination->width / 2.0, destination->height);
		
		R_DrawQuad();
		// draw right eye

		GL_MBind(0,right.texture);
		GL_MBind(1,rightDistortion[chromatic]);
		glViewport(destination->width / 2.0,0,destination->width / 2.0, destination->height);
		
		R_DrawQuad();

		glUseProgram(0);
		GL_MBind(1,0);
		GL_MBind(0,0);
		GL_EnableMultitexture(false);
		R_TeardownQuadState();

	} else {
		R_SetupBlit();

		glViewport(0,0,destination->width / 2.0, destination->height);
		R_BlitTextureToScreen(left.texture);
		glViewport(destination->width / 2.0,0,destination->width / 2.0, destination->height);

		R_BlitTextureToScreen(rightDistortion[chromatic]);
		R_TeardownBlit();
	}
}

int32_t SVR_Init(void)
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	return true;
}

void SVR_Disable(void)
{
	if (left.status)
		R_DelFBO(&left);
	if (right.status)
		R_DelFBO(&right);

	if (leftDistortion)
		glDeleteTextures(2,&leftDistortion[0]);
	if (rightDistortion)
		glDeleteTextures(2,&rightDistortion[0]);

	leftDistortion[0] = 0;
	leftDistortion[1] = 0;
	rightDistortion[0] = 0;
	rightDistortion[1] = 0;
}

int32_t SVR_Enable(void)
{
	char string[128];

	if (!glConfig.arb_texture_float)
		return 0;

	if (left.status)
		R_DelFBO(&left);
	if (right.status)
		R_DelFBO(&right);

	if (!leftDistortion[0])
		glGenTextures(2,&leftDistortion[0]);
	if (!rightDistortion[0])
		glGenTextures(2,&rightDistortion[0]);

	SteamVR_GetSettings(&svr_settings);
	
	SteamVR_GetEyeViewport(SVR_Left,&leftRenderRect.x,&leftRenderRect.y,&leftRenderRect.width,&leftRenderRect.height);	
	SteamVR_GetEyeViewport(SVR_Right,&rightRenderRect.x,&rightRenderRect.y,&rightRenderRect.width,&rightRenderRect.height);

	strncpy(string, va("SteamVR-%s", svr_settings.deviceName), sizeof(string));
	Cvar_ForceSet("vr_hmdstring",string);
	vr_svr_distortion->modified = true;
	return true;
}



hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	SVR_Init,
	SVR_Enable,
	SVR_Disable,
	SVR_FrameStart,
	SVR_SetOffscreenSize,
	SVR_GetState,
	SVR_Present,
};

#endif //NO_STEAM
#include "include/r_local.h"
#include "include/r_vr.h"

#ifndef NO_STEAM

#include "include/r_vr_svr.h"
#include "../vr/include/vr_svr.h"
#include "../vr/include/vr_steamvr.h"

//static fbo_t world;
static fbo_t left, right, offscreen;

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
	int width = glConfig.screen_width; // width of the distortion map
	int height= glConfig.screen_height; // height of the distortion map
	int bitsPerPixel = (glConfig.arb_texture_rg? 2 : 4);
	float scale = (1.0 / pow(2,vr_svr_distortion->value));
	
	width = (int) ((float) width * scale);
	height = (int) ((float) height * scale);
	
	Com_Printf("VR_SVR: Generating %dx%d distortion textures...\n",width,height);
	
	normalTexture = malloc(sizeof(GLfloat) * width * height * bitsPerPixel);
	chromaTexture = malloc(sizeof(GLfloat) * width * height * 4);

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

	free(chromaTexture);
	free(normalTexture);
}


void SVR_FrameStart(int32_t changeBackBuffers)
{


	if (vr_svr_distortion->modified)
	{
		if (vr_svr_distortion->value > 3)
			Cvar_SetInteger("vr_svr_distortion", 3);
		if (vr_svr_distortion->value <0)
			Cvar_SetInteger("vr_svr_distortion", 0);
		changeBackBuffers = 1;
		SVR_BuildDistortionTextures();		
		vr_svr_distortion->modified = false;
	}

	if (chromatic != (qboolean) !!vr_chromatic->value)
	{
		chromatic = (qboolean) !!vr_chromatic->value;
	}

	if (changeBackBuffers)
	{	
		float scale = R_AntialiasGetScale();
		origTargetRect.x = 0;
		origTargetRect.y = 0;
		origTargetRect.width = glConfig.screen_width * svr_settings.scaleX;
		origTargetRect.height = glConfig.screen_height * svr_settings.scaleY;

		renderTargetRect = origTargetRect;
	
		renderTargetRect.width *= scale;
		renderTargetRect.height *= scale;

		if (renderTargetRect.width != left.width || renderTargetRect.height != left.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &left);
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &right);
		}

		Com_Printf("VR_SVR: Set render target size to %ux%u\n",renderTargetRect.width,renderTargetRect.height);
	}
}

void SVR_GetState(vr_param_t *state)
{
	vr_param_t svrState;
	float ipd = (svr_settings.ipd / 2.0);
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
	svrState.offscreen = &offscreen;
	*state = svrState;
}

void R_Clear (void);
void SVR_Present(qboolean loading)
{
//	GL_SetIdentityOrtho(GL_PROJECTION, 0, svr_settings.width, svr_settings.height, 0, -1, 1);
	GL_ClearColor(0.0, 0.0, 0.0, 0.0);
	R_Clear();
	GL_SetDefaultClearColor();		

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
		glViewport(0,0,vid.width / 2.0, vid.height);
		
		R_DrawQuad();
		// draw right eye

		GL_MBind(0,right.texture);
		GL_MBind(1,rightDistortion[chromatic]);
		glViewport(vid.width / 2.0,0,vid.width / 2.0, vid.height);
		
		R_DrawQuad();

		glUseProgram(0);
		GL_MBind(1,0);
		GL_MBind(0,0);
		GL_EnableMultitexture(false);
		R_TeardownQuadState();

	} else {
		R_SetupBlit();

		glViewport(0,0,vid.width / 2.0, vid.height);
		R_BlitTextureToScreen(left.texture);
		glViewport(vid.width / 2.0,0,vid.width / 2.0, vid.height);

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
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (offscreen.valid)
		R_DelFBO(&offscreen);


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

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (offscreen.valid)
		R_DelFBO(&offscreen);

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
	SVR_GetState,
	SVR_Present,
	NULL
};

#else

hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

#endif //NO_STEAM
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

void SVR_BuildDistortionTextures()
{
	GLfloat *normalTexture, *chromaTexture;
	int width = origTargetRect.width; // width of the distortion map
	int height= origTargetRect.height; // height of the distortion map
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
		vr_svr_distortion->modified = false;
	}

	if (chromatic != (qboolean) !!vr_chromatic->value)
	{
		chromatic = (qboolean) !!vr_chromatic->value;
	}

	if (changeBackBuffers)
	{	
		origTargetRect.x = 0;
		origTargetRect.y = 0;
		origTargetRect.width = (uint32_t) Cvar_VariableValue("vid_width") * svr_settings.scaleX;
		origTargetRect.height = (uint32_t) Cvar_VariableValue("vid_height") * svr_settings.scaleY;

		renderTargetRect = origTargetRect;
		if (r_antialias->value == ANTIALIAS_4X_FSAA)
		{
			renderTargetRect.width *= 2;
			renderTargetRect.height *= 2;
		}
		if (renderTargetRect.width != left.width || renderTargetRect.height != left.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &left);
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, GL_RGBA8, &right);
		}

		Com_Printf("VR_SVR: Set render target size to %ux%u\n",renderTargetRect.width,renderTargetRect.height);
		SVR_BuildDistortionTextures();		
	}
}


void SVR_BindView(vr_eye_t eye)
{
	switch(eye)
	{
	case EYE_LEFT:


		vid.height = left.height;
		vid.width = left.width;
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:

		vid.height = right.height;
		vid.width = right.width;
		R_BindFBO(&right);
		break;
	default:
		return;
	}
}

void SVR_GetState(vr_param_t *state)
{
	vr_param_t svrState;
	float ipd = (svr_settings.ipd / 2.0);
	svrState.aspect = svr_settings.aspect;
	svrState.viewFovX = svr_settings.viewFovX;
	svrState.viewFovY = svr_settings.viewFovY;
	VectorSet(svrState.renderParams[0].viewOffset,ipd * EYE_LEFT, 0.0, 0.0);
	VectorSet(svrState.renderParams[1].viewOffset,ipd * EYE_RIGHT, 0.0, 0.0);

    svrState.renderParams[0].projection.y.scale = 1.0f / tanf((svrState.viewFovY / 2.0f) * M_PI / 180);
	svrState.renderParams[0].projection.y.offset = 0.0;
	svrState.renderParams[0].projection.x.scale = svrState.renderParams[0].projection.y.scale / svr_settings.aspect;
	svrState.renderParams[0].projection.x.offset = svr_settings.projOffset;


    svrState.renderParams[1].projection.y.scale = 1.0f / tanf((svrState.viewFovY / 2.0f) * M_PI / 180);
	svrState.renderParams[1].projection.y.offset = 0.0;
	svrState.renderParams[1].projection.x.scale = svrState.renderParams[1].projection.y.scale / svr_settings.aspect;
	svrState.renderParams[1].projection.x.offset = -svr_settings.projOffset;

	svrState.pixelScale = 3.0;
	svrState.eyeFBO[0] = &left;
	svrState.eyeFBO[1] = &right;
	*state = svrState;
}

void SVR_Present(qboolean loading)
{
	GL_SetIdentityOrtho(GL_PROJECTION, 0, svr_settings.width, svr_settings.height, 0, -1, 1);

	if (!vr_svr_debug->value)
	{

		vr_distort_shader_t *current_shader = &vr_distort_shaders[chromatic];
 
		
		// draw left eye
		glUseProgram(current_shader->shader->program);
		glUniform1i(current_shader->uniform.texture,0);
		glUniform1i(current_shader->uniform.distortion,1);	

		GL_EnableMultitexture(true);
		GL_MBind(0,left.texture);
		GL_MBind(1,leftDistortion[chromatic]);
		
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(leftRenderRect.x, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(leftRenderRect.x, leftRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y);
		glEnd();

		// draw right eye

		GL_MBind(0,right.texture);
		GL_MBind(1,rightDistortion[chromatic]);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(rightRenderRect.x, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(rightRenderRect.x, rightRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y);
		glEnd();
		glUseProgram(0);
		GL_MBind(1,0);
		GL_MBind(0,0);
		GL_EnableMultitexture(false);

	} else {
		GL_MBind(0,left.texture);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(leftRenderRect.x, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(leftRenderRect.x, leftRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y);
		glEnd();

		GL_MBind(0,rightDistortion[chromatic]);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(rightRenderRect.x, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(rightRenderRect.x, rightRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y);
		glEnd();
		GL_MBind(0,0);
	}
}
void SVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect)
{
	*rect = renderTargetRect;
}


int32_t SVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	return true;
}

void SVR_Disable()
{
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
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

void SVR_CalculateRenderParams()
{

}


int32_t SVR_Enable()
{
	char string[128];

	if (!glConfig.arb_texture_float)
		return 0;

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
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
	return true;
}



hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	SVR_Init,
	SVR_Enable,
	SVR_Disable,
	SVR_BindView,
	SVR_GetViewRect,
	SVR_FrameStart,
	SVR_GetState,
	SVR_Present
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
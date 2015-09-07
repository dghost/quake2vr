#include "include/r_vr_rift.h"
#include "../vr/include/vr_rift.h"
#include "include/r_local.h"
#ifdef OCULUS_DYNAMIC
#include "../vr/oculus_dynamic/oculus_dynamic.h"
#else
#include "OVR_CAPI_GL.h"
#endif
#include "../../backends/sdl2/sdl2quake.h"


void Rift_FrameStart(void);
void Rift_Present(fbo_t *destination, qboolean loading);
int32_t Rift_Enable(void);
void Rift_Disable(void);
int32_t Rift_Init(void);
void Rift_GetState(vr_param_t *state);
void Rift_PostPresent(void);
void Rift_SetOffscreenSize(uint32_t width, uint32_t height);

hmd_render_t vr_render_rift =
{
	HMD_RIFT,
	Rift_Init,
	Rift_Enable,
	Rift_Disable,
	Rift_FrameStart,
	Rift_SetOffscreenSize,
	Rift_GetState,
	Rift_Present,
};

rift_render_export_t renderExport;


extern ovrHmd hmd;
extern ovrHmdDesc hmdDesc;
extern ovrEyeRenderDesc eyeDesc[2];
extern ovrTrackingState trackingState;
extern ovrFrameTiming frameTime;
ovrVector3f      hmdToEyeViewOffset[2];

static vec4_t cameraFrustum[4];

extern void VR_Rift_GetFOV(float *fovx, float *fovy);
extern int32_t VR_Rift_RenderLatencyTest(vec4_t color);


static vr_param_t currentState;

static ovrSwapTextureSet *eyeTextures[2];
static ovrGLTexture *mirrorTexture = NULL;

static uint32_t currentFBO = 0;

ovrLayerEyeFov swapLayer;

// this should probably be rearranged
typedef struct {
	fbo_t eyeFBO;
	ovrSizei renderTarget;
	ovrFovPort eyeFov;
	ovrVector2f UVScaleOffset[2];

} ovr_eye_info_t;

static ovr_eye_info_t renderInfo[2];

static int currentFrame = 0;

int32_t R_GenFBOWithoutTexture(int32_t width, int32_t height, GLenum format, fbo_t *FBO)
{
	GLuint fbo, dep;
	int32_t err;
	glGetError();

	glGenFramebuffersEXT(1, &fbo);
	glGenRenderbuffersEXT(1, &dep);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, dep);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, width, height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: Depth buffer creation: glGetError() = 0x%x\n", err);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: FBO creation: glGetError() = 0x%x\n", err);

	FBO->framebuffer = fbo;
	FBO->texture = 0;
	FBO->depthbuffer = dep;
	FBO->width = width;
	FBO->height = height;
	FBO->format = format;
	FBO->status = FBO_VALID | FBO_GENERATED_DEPTH;
	if (format == GL_SRGB8 || format == GL_SRGB8_ALPHA8)
		FBO->status |= FBO_SRGB;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, glState.currentFBO->framebuffer);
	return 1;
}


void Rift_CalculateState(vr_param_t *state)
{
	vr_param_t ovrState;
	int eye = 0;
	for (eye = 0; eye < 2; eye++) {
		unsigned int i = 0;
		if (vr_rift_maxfov->value)
		{
			renderInfo[eye].eyeFov = hmdDesc.MaxEyeFov[eye];
		}
		else
		{
			renderInfo[eye].eyeFov = hmdDesc.DefaultEyeFov[eye];
		}

		ovrState.eyeFBO[eye] = &renderInfo[eye].eyeFBO;

		ovrState.renderParams[eye].projection.x.scale = 2.0f / (renderInfo[eye].eyeFov.LeftTan + renderInfo[eye].eyeFov.RightTan);
		ovrState.renderParams[eye].projection.x.offset = (renderInfo[eye].eyeFov.LeftTan - renderInfo[eye].eyeFov.RightTan) * ovrState.renderParams[eye].projection.x.scale * 0.5f;
		ovrState.renderParams[eye].projection.y.scale = 2.0f / (renderInfo[eye].eyeFov.UpTan + renderInfo[eye].eyeFov.DownTan);
		ovrState.renderParams[eye].projection.y.offset = (renderInfo[eye].eyeFov.UpTan - renderInfo[eye].eyeFov.DownTan) * ovrState.renderParams[eye].projection.y.scale * 0.5f;

		// set up rendering info
		eyeDesc[eye] = ovr_GetRenderDesc(hmd, (ovrEyeType) eye, renderInfo[eye].eyeFov);

		VectorSet(ovrState.renderParams[eye].viewOffset,
			-eyeDesc[eye].HmdToEyeViewOffset.x,
			eyeDesc[eye].HmdToEyeViewOffset.y,
			eyeDesc[eye].HmdToEyeViewOffset.z);
	}
	{
		// calculate this to give the engine a rough idea of the fov
		float combinedTanHalfFovHorizontal = max(max(renderInfo[0].eyeFov.LeftTan, renderInfo[0].eyeFov.RightTan), max(renderInfo[1].eyeFov.LeftTan, renderInfo[1].eyeFov.RightTan));
		float combinedTanHalfFovVertical = max(max(renderInfo[0].eyeFov.UpTan, renderInfo[0].eyeFov.DownTan), max(renderInfo[1].eyeFov.UpTan, renderInfo[1].eyeFov.DownTan));
		float horizontalFullFovInRadians = 2.0f * atanf(combinedTanHalfFovHorizontal);
		float fovX = RAD2DEG(horizontalFullFovInRadians);
		float fovY = RAD2DEG(2.0 * atanf(combinedTanHalfFovVertical));
		ovrState.aspect = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
		ovrState.viewFovY = fovY;
		ovrState.viewFovX = fovX;
		ovrState.pixelScale = vid.width / (float) hmdDesc.Resolution.w;
	}

	hmdToEyeViewOffset[0] = eyeDesc[0].HmdToEyeViewOffset;
	hmdToEyeViewOffset[1] = eyeDesc[1].HmdToEyeViewOffset;


	*state = ovrState;
}

void Rift_SetOffscreenSize(uint32_t width, uint32_t height) {
	int i;
	float w, h;
	float ovrScale;
	ovrResult result;

	Rift_CalculateState(&currentState);

	w = width / (float) hmdDesc.Resolution.w;
	h = height / (float) hmdDesc.Resolution.h;

	ovrScale = (w + h) / 2.0;
	if (vr_rift_debug->value)
		Com_Printf("VR_Rift: Set render target scale to %.2f\n", ovrScale);

	currentFBO = 0;

	if (mirrorTexture)
		ovr_DestroyMirrorTexture(hmd, mirrorTexture);

	result = ovr_CreateMirrorTextureGL(hmd, GL_SRGB8, width, height, &mirrorTexture);


	swapLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	swapLayer.Header.Type = ovrLayerType_EyeFov;

	for (i = 0; i < 2; i++)
	{

		renderInfo[i].renderTarget = ovr_GetFovTextureSize(hmd, (ovrEyeType) i, renderInfo[i].eyeFov, ovrScale);

		if (renderInfo[i].renderTarget.w != renderInfo[i].eyeFBO.width || renderInfo[i].renderTarget.h != renderInfo[i].eyeFBO.height)
		{
			ovrRecti viewport = { { 0, 0 }, { renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h } };
			swapLayer.Viewport[i] = viewport;


			if (vr_rift_debug->value)
				Com_Printf("VR_Rift: Set buffer %i to size %i x %i\n", i, renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h);

			R_DelFBO(&renderInfo[i].eyeFBO);
			if (eyeTextures[i])
				ovr_DestroySwapTextureSet(hmd, eyeTextures[i]);

			result = ovr_CreateSwapTextureSetGL(hmd, GL_SRGB8_ALPHA8, renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h, &eyeTextures[i]);
			swapLayer.ColorTexture[i] = eyeTextures[i];

			R_GenFBOWithoutTexture(renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h, GL_SRGB8_ALPHA8, &renderInfo[i].eyeFBO);
		}
		swapLayer.Fov[i] = renderInfo[i].eyeFov;

	}



}

void R_Clear();
void Rift_FrameStart()
{
	qboolean changed = false;
	int i;

	if (vr_rift_maxfov->modified)
	{
		int newValue = vr_rift_maxfov->value ? 1 : 0;
		if (newValue != (int) vr_rift_maxfov->value)
			Cvar_SetInteger("vr_rift_maxfov", newValue);
		changed = true;
		vr_rift_maxfov->modified = (qboolean) false;
	}

	if (changed) {
		float scale = R_AntialiasGetScale();
		float w = glConfig.render_width * scale;
		float h = glConfig.render_height * scale;
		Rift_SetOffscreenSize(w, h);
	}

	if (eyeTextures[0] != NULL) {
		currentFBO = (currentFBO + 1) % eyeTextures[0]->TextureCount;

		for (i = 0; i < 2; i++) {
			ovrGLTextureData *tex = (ovrGLTextureData *) &eyeTextures[i]->Textures[currentFBO];
			eyeTextures[i]->CurrentIndex = currentFBO;
			currentState.eyeFBO[i]->texture = tex->TexId;
			R_BindFBO(currentState.eyeFBO[i]);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex->TexId, 0);
			R_Clear();
		}
		R_BindFBO(&screenFBO);
	}


}

void Rift_GetState(vr_param_t *state)
{
	ovrPosef eyes[2];

	*state = currentState;

	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, hmdToEyeViewOffset, eyes);

//	Com_Printf("Left eye: %.2f %.2f %.2f\n", eyes[0].Position.x, eyes[0].Position.y, eyes[0].Position.z);
}

void R_Clear(void);

void VR_Rift_QuatToEuler(ovrQuatf q, vec3_t e);
void Rift_Present(fbo_t *destination, qboolean loading)
{
	float desaturate = 0.0;

	if (renderExport.positionTracked && trackingState.StatusFlags & ovrStatus_PositionConnected && vr_rift_trackingloss->value > 0) {
		if (renderExport.hasPositionLock) {
			float yawDiff = (fabsf(renderExport.cameraYaw) - 105.0f) * 0.04;
			float xBound, yBound, zBound;
			vec_t temp[4][4], fin[4][4];
			int i = 0;
			vec3_t euler;
			vec4_t pos = { 0.0, 0.0, 0.0, 1.0 };
			vec4_t out = { 0, 0, 0, 0 };
			ovrPosef camera, head;
			vec4_t quat;
			camera = trackingState.CameraPose;
			head = trackingState.HeadPose.ThePose;

			pos[0] = -(head.Position.x - camera.Position.x);
			pos[1] = head.Position.y - camera.Position.y;
			pos[2] = -(head.Position.z - camera.Position.z);

			VR_Rift_QuatToEuler(camera.Orientation, euler);
			EulerToQuat(euler, quat);
			QuatToRotation(quat, temp);
			MatrixMultiply(cameraFrustum, temp, fin);

			for (i = 0; i < 4; i++) {
				out[i] = fin[i][0] * pos[0] + fin[i][1] * pos[1] + fin[i][2] * pos[2] + fin[i][3] * pos[3];
			}

			xBound = (fabsf(out[0]) - 0.6f) * 6.25f;
			yBound = (fabsf(out[1]) - 0.45f) * 6.25f;
			zBound = (fabsf(out[2] - 0.5f) - 0.5f) * 10.0f;

			yawDiff = clamp(yawDiff, 0.0, 1.0);
			xBound = clamp(xBound, 0.0, 1.0);
			yBound = clamp(yBound, 0.0, 1.0);
			zBound = clamp(zBound, 0.0, 1.0);

			desaturate = max(max(max(xBound, yBound), zBound), yawDiff);
		}
		else {
			desaturate = 1.0;
		}
	}

	{
		int i;


		ovrLayerHeader* layers = &swapLayer.Header;
		ovrResult result;
		swapLayer.RenderPose[0] = trackingState.HeadPose.ThePose;
		swapLayer.RenderPose[1] = trackingState.HeadPose.ThePose;

		for (i = 0; i < 2; i++) {
			R_BindFBO(currentState.eyeFBO[i]);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 0, 0);
		}

		R_BindFBO(destination);
		R_Clear();

		result = ovr_SubmitFrame(hmd, 0, NULL, &layers, 1);

		R_BlitFlipped(mirrorTexture->OGL.TexId);
	}
}


int32_t Rift_Enable(void)
{
	int i;
	eyeScaleOffset_t camera;

	if (!glConfig.arb_texture_float)
		return 0;


	for (i = 0; i < 2; i++)
	{
		if (renderInfo[i].eyeFBO.status)
			R_DelFBO(&renderInfo[i].eyeFBO);
	}

	camera.x.offset = 0.0;
	camera.x.scale = 1.0 / tanf(hmdDesc.CameraFrustumHFovInRadians * 0.5);
	camera.y.offset = 0.0;
	camera.y.scale = 1.0 / tanf(hmdDesc.CameraFrustumVFovInRadians * 0.5);
	R_MakePerspectiveFromScale(camera, hmdDesc.CameraFrustumNearZInMeters, hmdDesc.CameraFrustumFarZInMeters, cameraFrustum);

	Cvar_ForceSet("vr_hmdstring", (char *) hmdDesc.ProductName);
	return VR_ENABLED | VR_DISABLE_VSYNC;
}

void Rift_Disable(void)
{
	int i;

	if (mirrorTexture) {
		ovr_DestroyMirrorTexture(hmd, mirrorTexture);
		mirrorTexture = NULL;
	}

	for (i = 0; i < 2; i++)
	{
		if (renderInfo[i].eyeFBO.status)
			R_DelFBO(&renderInfo[i].eyeFBO);
		if (eyeTextures[i])
			ovr_DestroySwapTextureSet(hmd, eyeTextures[i]);
		eyeTextures[i] = NULL;
	}
}

int32_t Rift_Init(void)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		R_InitFBO(&renderInfo[i].eyeFBO);
	}
	return true;
}

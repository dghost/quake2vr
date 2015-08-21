#include "include/r_vr_rift.h"
#include "../vr/include/vr_rift.h"
#include "include/r_local.h"
#ifdef OCULUS_DYNAMIC
#include "../vr/oculus_dynamic/oculus_dynamic.h"
#else
#define OVR_ALIGNAS(x)
#include "OVR_CAPI.h"
#endif
#include "../../backends/sdl2/sdl2quake.h"


void Rift_FrameStart(void);
void Rift_Present(qboolean loading);
int32_t Rift_Enable(void);
void Rift_Disable(void);
int32_t Rift_Init(void);
void Rift_GetState(vr_param_t *state);
void Rift_PostPresent(void);
fbo_t *Rift_GetScreenFBO(qboolean frameStart);
void Rift_SetOffscreenSize(uint32_t width, uint32_t height);

hmd_render_t vr_render_rift =
{
	HMD_RIFT,
	Rift_Init,
	Rift_Enable,
	Rift_Disable,
	Rift_FrameStart,
	Rift_GetState,
	Rift_Present,
	Rift_PostPresent,
	Rift_GetScreenFBO,
	Rift_SetOffscreenSize
};

rift_render_export_t renderExport;


extern ovrHmd hmd;
extern ovrEyeRenderDesc eyeDesc[2];
extern ovrTrackingState trackingState;
extern ovrFrameTiming frameTime;

static vec4_t cameraFrustum[4];

extern void VR_Rift_GetFOV(float *fovx, float *fovy);
extern int32_t VR_Rift_RenderLatencyTest(vec4_t color);


static vr_param_t currentState;

// this should probably be rearranged
typedef struct {
	fbo_t eyeFBO;
	ovrSizei renderTarget;
	ovrFovPort eyeFov; 
	ovrVector2f UVScaleOffset[2];
	
} ovr_eye_info_t;

static ovr_eye_info_t renderInfo[2];

static fbo_t offscreen;
static int currentFrame = 0;

void Rift_CalculateState(vr_param_t *state)
{
	vr_param_t ovrState;
	float ovrScale = vr_rift_supersample->value;
	int eye = 0;
	
	for (eye = 0; eye < 2; eye++) {
		unsigned int i = 0;
		if (vr_rift_maxfov->value)
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
	ovrState.enabled = true;
	ovrState.swapToScreen = true;
	*state = ovrState;
}

void Rift_SetOffscreenSize(uint32_t width, uint32_t height) {
	int i;
	float w, h;
	float ovrScale;

	Rift_CalculateState(&currentState);

	R_ResizeFBO(width, height, 1, GL_RGBA8, &offscreen);

	w = width / (float) hmd->Resolution.w;
	h = height / (float) hmd->Resolution.h;

	ovrScale = (w + h) / 2.0;
	ovrScale *= vr_rift_supersample->value;
	if (vr_rift_debug->value)
		Com_Printf("VR_Rift: Set render target scale to %.2f\n", ovrScale);
	for (i = 0; i < 2; i++)
	{
		ovrRecti viewport = { { 0, 0 }, { 0, 0 } };
		renderInfo[i].renderTarget = ovrHmd_GetFovTextureSize(hmd, (ovrEyeType) i, renderInfo[i].eyeFov, ovrScale);
		viewport.Size.w = renderInfo[i].renderTarget.w;
		viewport.Size.h = renderInfo[i].renderTarget.h;

		if (renderInfo[i].renderTarget.w != renderInfo[i].eyeFBO.width || renderInfo[i].renderTarget.h != renderInfo[i].eyeFBO.height)
		{
			if (vr_rift_debug->value)
				Com_Printf("VR_Rift: Set buffer %i to size %i x %i\n", i, renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h);
			R_ResizeFBO(renderInfo[i].renderTarget.w, renderInfo[i].renderTarget.h, 1, GL_RGBA8, &renderInfo[i].eyeFBO);
			R_ClearFBO(&renderInfo[i].eyeFBO);
		}

	}
}

void Rift_FrameStart()
{
	qboolean changed = false;
	if (vr_rift_maxfov->modified)
	{
		int newValue =  vr_rift_maxfov->value ? 1 : 0;
		if (newValue != (int)vr_rift_maxfov->value)
			Cvar_SetInteger("vr_rift_maxfov",newValue);
		changed = true;
		vr_rift_maxfov->modified = (qboolean) false;
	}

	if (vr_rift_supersample->modified)
	{
		if (vr_rift_supersample->value < 1.0)
			Cvar_Set("vr_rift_supersample", "1.0");
		else if (vr_rift_supersample->value > 2.0)
			Cvar_Set("vr_rift_supersample", "2.0");
		changed = true;
		vr_rift_supersample->modified = false;
	}

	if (changed) {
		float scale = R_AntialiasGetScale();
		float w = glConfig.render_width * scale;
		float h = glConfig.render_height * scale;
		Rift_SetOffscreenSize(w, h);
	}
}

void Rift_GetState(vr_param_t *state)
{
	*state = currentState;
}

void R_Clear (void);

void VR_Rift_QuatToEuler(ovrQuatf q, vec3_t e);
void Rift_Present(qboolean loading)
{
    int fade = vr_rift_distortion_fade->value != 0.0f;
	float desaturate = 0.0;
    
	if (renderExport.positionTracked && trackingState.StatusFlags & ovrStatus_PositionConnected && vr_rift_trackingloss->value > 0) {
		if (renderExport.hasPositionLock) {
			float yawDiff = (fabsf(renderExport.cameraYaw) - 105.0f) * 0.04;
			float xBound,yBound,zBound;
			vec_t temp[4][4], fin[4][4];
			int i = 0;
			vec3_t euler;
			vec4_t pos = {0.0,0.0,0.0,1.0};
			vec4_t out = {0,0,0,0};
			ovrPosef camera, head;
			vec4_t quat;
			camera = trackingState.CameraPose;
			head = trackingState.HeadPose.ThePose;

			pos[0] = -(head.Position.x - camera.Position.x);
			pos[1] = head.Position.y - camera.Position.y;
			pos[2] = -(head.Position.z - camera.Position.z);

			VR_Rift_QuatToEuler(camera.Orientation,euler);
			EulerToQuat(euler,quat);
			QuatToRotation(quat,temp);
			MatrixMultiply (cameraFrustum,temp,fin);

			for (i=0; i<4; i++) {
				out[i] = fin[i][0]*pos[0] + fin[i][1]*pos[1] + fin[i][2]*pos[2] + fin[i][3]*pos[3];
			}

			xBound = (fabsf(out[0]) - 0.6f) * 6.25f;
			yBound = (fabsf(out[1]) - 0.45f) * 6.25f;
			zBound = (fabsf(out[2] - 0.5f) - 0.5f) * 10.0f;

			yawDiff = clamp(yawDiff,0.0,1.0);
			xBound = clamp(xBound,0.0,1.0);
			yBound = clamp(yBound,0.0,1.0);
			zBound = clamp(zBound,0.0,1.0);

			desaturate = max(max(max(xBound,yBound),zBound),yawDiff);
		} else {
			desaturate = 1.0;
		}
	}
	R_BindFBO(&offscreen);
	GL_ClearColor(0.0, 0.0, 0.0, 1.0);
	R_Clear();
	GL_SetDefaultClearColor();	
	{
		R_SetupBlit();

		glViewport(0, 0, vid.width / 2.0, vid.height);
		R_BlitTextureToScreen(renderInfo[0].eyeFBO.texture);
		glViewport(vid.width / 2.0, 0, vid.width / 2.0, vid.height);

		R_BlitTextureToScreen(renderInfo[1].eyeFBO.texture);
		R_TeardownBlit();

	}
	R_BindFBO(&screenFBO);
}

void Rift_PostPresent(void)
{

}

fbo_t *Rift_GetScreenFBO(qboolean frameStart) {
	return &offscreen;
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
	if (offscreen.status)
		R_DelFBO(&offscreen);

	camera.x.offset = 0.0;
	camera.x.scale = 1.0 / tanf(hmd->CameraFrustumHFovInRadians * 0.5);
	camera.y.offset = 0.0;
	camera.y.scale = 1.0 / tanf(hmd->CameraFrustumVFovInRadians * 0.5);
	R_MakePerspectiveFromScale(camera,hmd->CameraFrustumNearZInMeters, hmd->CameraFrustumFarZInMeters, cameraFrustum);

	Cvar_ForceSet("vr_hmdstring",(char *)hmd->ProductName);
	return true;
}

void Rift_Disable(void)
{
	int i;

	for (i = 0; i < 2; i++)
	{
		if (renderInfo[i].eyeFBO.status)
			R_DelFBO(&renderInfo[i].eyeFBO);
	}
	if (offscreen.status)
		R_DelFBO(&offscreen);

}

int32_t Rift_Init(void)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		R_InitFBO(&renderInfo[i].eyeFBO);
	}
	R_InitFBO(&offscreen);
	return true;
}

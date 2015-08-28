#include "include/vr.h"
#include "include/vr_rift.h"
#include "OVR_CAPI.h"
#include <SDL.h>

void VR_Rift_GetHMDResolution(int32_t *width, int32_t *height);
void VR_Rift_FrameStart();
void VR_Rift_FrameEnd();

int32_t VR_Rift_Enable();
void VR_Rift_Disable();
int32_t VR_Rift_Init();
ovrBool VR_Rift_InitSensor();
void VR_Rift_Shutdown();
int32_t VR_Rift_getOrientation(float euler[3]);
void VR_Rift_ResetHMDOrientation();
int32_t VR_Rift_SetPredictionTime(float time);
int32_t VR_Rift_getPosition(float pos[3]);

cvar_t *vr_rift_debug;
cvar_t *vr_rift_maxfov;
cvar_t *vr_rift_enable;
cvar_t *vr_rift_autoprediction;
cvar_t *vr_rift_dk2_color_hack;
cvar_t *vr_rift_trackingloss;

ovrHmd hmd;
ovrHmdDesc hmdDesc;
ovrGraphicsLuid graphicsLuid;
ovrEyeRenderDesc eyeDesc[2];

ovrTrackingState trackingState;
ovrFrameTiming frameTime;
static ovrBool sensorEnabled = 0;

static ovrBool libovrInitialized = 0;

static double prediction_time;

rift_render_export_t renderExport;

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_Rift_Init,
	VR_Rift_Shutdown,
	VR_Rift_Enable,
	VR_Rift_Disable,
	VR_Rift_FrameStart,
	VR_Rift_FrameEnd,
	VR_Rift_ResetHMDOrientation,
	VR_Rift_getOrientation,
	VR_Rift_getPosition,
	VR_Rift_SetPredictionTime,
	NULL,
	VR_Rift_GetHMDResolution
};

static riftname_t hmdnames[255];


void VR_Rift_GetFOV(float *fovx, float *fovy)
{
	*fovx = hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan + hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan;
	*fovy = hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan + hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan;
}

void VR_Rift_GetHMDResolution(int32_t *width, int32_t *height)
{
	if (hmd)
	{
		*width = hmdDesc.Resolution.w;
		*height = hmdDesc.Resolution.h;
	}
}


void VR_Rift_QuatToEuler(ovrQuatf q, vec3_t e)
{
	vec_t w = q.w;
	vec_t Q[3] = { q.x, q.y, q.z };

	vec_t ww = w*w;
	vec_t Q11 = Q[1] * Q[1];
	vec_t Q22 = Q[0] * Q[0];
	vec_t Q33 = Q[2] * Q[2];

	vec_t s2 = -2.0f * (-w*Q[0] + Q[1] * Q[2]);

	if (s2 < -1.0f + 0.0000001f)
	{ // South pole singularity
		e[YAW] = 0.0f;
		e[PITCH] = -M_PI / 2.0f;
		e[ROLL] = atan2(2.0f*(-Q[1] * Q[0] + w*Q[2]),
			ww + Q22 - Q11 - Q33);
	}
	else if (s2 > 1.0f - 0.0000001f)
	{  // North pole singularity
		e[YAW] = 0.0f;
		e[PITCH] = M_PI / 2.0f;
		e[ROLL] = atan2(2.0f*(-Q[1] * Q[0] + w*Q[2]),
			ww + Q22 - Q11 - Q33);
	}
	else
	{
		e[YAW] = -atan2(-2.0f*(w*Q[1] - -Q[0] * Q[2]),
			ww + Q33 - Q11 - Q22);
		e[PITCH] = asin(s2);
		e[ROLL] = atan2(2.0f*(w*Q[2] - -Q[1] * Q[0]),
			ww + Q11 - Q22 - Q33);
	}

	e[PITCH] = -RAD2DEG(e[PITCH]);
	e[YAW] = RAD2DEG(e[YAW]);
	e[ROLL] = -RAD2DEG(e[ROLL]);
}

int32_t VR_Rift_getOrientation(float euler[3])
{
	double time = 0.0;

	if (!hmd)
		return 0;

	if (vr_rift_autoprediction->value > 0)
		time = frameTime.DisplayMidpointSeconds;
	else
		time = ovr_GetTimeInSeconds() + prediction_time;
	trackingState = ovr_GetTrackingState(hmd, time);
	if (trackingState.StatusFlags & ovrStatus_OrientationTracked)
	{
		vec3_t temp;
		VR_Rift_QuatToEuler(trackingState.HeadPose.ThePose.Orientation, euler);
		if (trackingState.StatusFlags & ovrStatus_PositionTracked) {
			VR_Rift_QuatToEuler(trackingState.LeveledCameraPose.Orientation, temp);
			renderExport.cameraYaw = euler[YAW] - temp[YAW];
			AngleClamp(&renderExport.cameraYaw);
		}
		else {
			renderExport.cameraYaw = 0.0;
		}
		return 1;
	}
	return 0;
}

void SCR_CenterAlert(char *str);
int32_t VR_Rift_getPosition(float pos[3])
{
	qboolean tracked = 0;
	if (!hmd)
		return 0;

	//	if (!sensorEnabled)
	//		VR_Rift_InitSensor();

	if (sensorEnabled && renderExport.positionTracked)
	{

		tracked = trackingState.StatusFlags & ovrStatus_PositionTracked ? 1 : 0;
		if (tracked)
		{
			ovrPosef pose = trackingState.HeadPose.ThePose;
			VectorSet(pos, -pose.Position.z, pose.Position.x, pose.Position.y);
			VectorScale(pos, (PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M), pos);
		}

		if (vr_rift_trackingloss->value > 0 && trackingState.StatusFlags & ovrStatus_PositionConnected)
		{
			if (tracked && !renderExport.hasPositionLock && vr_rift_debug->value)
				SCR_CenterAlert("Position tracking enabled");
			else if (!tracked && renderExport.hasPositionLock && vr_rift_debug->value)
				SCR_CenterAlert("Position tracking interrupted");
		}

		renderExport.hasPositionLock = tracked;
	}
	return tracked;
}


void VR_Rift_ResetHMDOrientation()
{
	if (hmd)
		ovr_RecenterPose(hmd);
}

ovrBool VR_Rift_InitSensor()
{
	uint32_t sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;

	if (sensorEnabled)
	{
		sensorEnabled = 0;
	}

	sensorCaps |= ovrTrackingCap_Position;

	sensorEnabled = ovr_ConfigureTracking(hmd, sensorCaps, ovrTrackingCap_Orientation);

	return sensorEnabled;
}

int32_t VR_Rift_SetPredictionTime(float time)
{
	prediction_time = time / 1000.0;
	if (vr_rift_debug->value)
		Com_Printf("VR_Rift: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}

void VR_Rift_FrameStart()
{

	if (!renderExport.withinFrame)
	{
		frameTime = ovr_GetFrameTiming(hmd, 0);
	}
	renderExport.withinFrame = true;
}

void VR_Rift_FrameEnd()
{
	renderExport.withinFrame = false;
}

float VR_Rift_GetGammaMin()
{
	float result = 0.0;
	if (hmd && hmdDesc.Type >= ovrHmd_DK2)
		result = 1.0 / 255.0;
	return result;
}

riftname_t *VR_Rift_GetNameList()
{
	return hmdnames;
}

int32_t VR_Rift_Enable()
{
	qboolean isDebug = false;
	uint32_t device = 0;
	ovrResult result = 0;

	if (!vr_rift_enable->value)
		return 0;

	if (!libovrInitialized)
	{
		result = ovr_Initialize(0);

		if (OVR_FAILURE(result))
		{
			Com_Printf("VR_Rift: Fatal error: could not initialize LibOVR!\n");
			libovrInitialized = 0;
			return 0;
		}
		else {
			Com_Printf("VR_Rift: %s initialized...\n", ovr_GetVersionString());
			libovrInitialized = 1;
		}
	}
	{

		qboolean found = false;


		Com_Printf("VR_Rift: Enumerating devices...\n");

		Com_Printf("VR_Rift: Initializing HMD: ");

		renderExport.withinFrame = false;
		result = ovr_Create(&hmd, &graphicsLuid);

		if (OVR_FAILURE(result))
		{
			Com_Printf("no HMD detected!\n");
		}
		else {
			Com_Printf("ok!\n");
		}

		hmdDesc = ovr_GetHmdDesc(hmd);

		ovr_SetEnabledCaps(hmd, hmdDesc.DefaultHmdCaps);

		if (hmdDesc.AvailableHmdCaps & ovrTrackingCap_Position) {
			renderExport.positionTracked = (qboolean) true;
			Com_Printf("...supports position tracking\n");
		}
		else {
			renderExport.positionTracked = (qboolean) false;
		}
		Com_Printf("...has type %s\n", hmdDesc.ProductName);
		Com_Printf("...has %ux%u native resolution\n", hmdDesc.Resolution.w, hmdDesc.Resolution.h);

		if (!VR_Rift_InitSensor())
		{
			Com_Printf("VR_Rift: Sensor initialization failed!\n");
		}
		return 1;
	}
}

void VR_Rift_Disable()
{
	renderExport.withinFrame = false;
	if (hmd)
	{
		ovr_Destroy(hmd);
	}
	hmd = NULL;
}


int32_t VR_Rift_Init()
{

	/*
	if (!libovrInitialized)
	{
	libovrInitialized = ovr_Initialize();
	if (!libovrInitialized)
	{
	Com_Printf("VR_Rift: Fatal error: could not initialize LibOVR!\n");
	return 0;
	} else {
	Com_Printf("VR_Rift: %s initialized...\n",ovr_GetVersionString());
	}
	}
	*/

	vr_rift_trackingloss = Cvar_Get("vr_rift_trackingloss", "1", CVAR_CLIENT);
	vr_rift_maxfov = Cvar_Get("vr_rift_maxfov", "0", CVAR_CLIENT);
	vr_rift_enable = Cvar_Get("vr_rift_enable", "1", CVAR_CLIENT);
	vr_rift_dk2_color_hack = Cvar_Get("vr_rift_dk2_color_hack", "1", CVAR_CLIENT);
	vr_rift_debug = Cvar_Get("vr_rift_debug", "0", CVAR_CLIENT);
	vr_rift_autoprediction = Cvar_Get("vr_rift_autoprediction", "1", CVAR_CLIENT);

	return 1;
}

void VR_Rift_Shutdown()
{
	if (libovrInitialized)
		ovr_Shutdown();
	libovrInitialized = 0;
}



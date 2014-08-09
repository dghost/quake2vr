#include "include/vr.h"
#include "include/vr_ovr.h"
#include "OVR_CAPI.h"
#include <SDL.h>

void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos);
void VR_OVR_FrameStart();
void VR_OVR_FrameEnd();

int32_t VR_OVR_Enable();
void VR_OVR_Disable();
int32_t VR_OVR_Init();
ovrBool VR_OVR_InitSensor();
void VR_OVR_Shutdown();
int32_t VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
int32_t VR_OVR_SetPredictionTime(float time);
int32_t VR_OVR_getPosition(float pos[3]);

cvar_t *vr_ovr_debug;
cvar_t *vr_ovr_maxfov;
cvar_t *vr_ovr_supersample;
cvar_t *vr_ovr_enable;
cvar_t *vr_ovr_autoprediction;
cvar_t *vr_ovr_timewarp;
cvar_t *vr_ovr_dk2_color_hack;
cvar_t *vr_ovr_lowpersistence;
cvar_t *vr_ovr_lumoverdrive;

unsigned char ovrLatencyColor[3];

ovrHmd hmd;
ovrEyeRenderDesc eyeDesc[2];

qboolean withinFrame = false;

ovrTrackingState trackingState;
ovrFrameTiming frameTime;
static ovrBool sensorEnabled = 0;

static ovrBool libovrInitialized = 0;

static qboolean positionTracked;

static double prediction_time;

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_OVR_Init,
	VR_OVR_Shutdown,
	VR_OVR_Enable,
	VR_OVR_Disable,
	VR_OVR_GetHMDPos,
	VR_OVR_FrameStart,
	VR_OVR_FrameEnd,
	VR_OVR_ResetHMDOrientation,
	VR_OVR_getOrientation,
	VR_OVR_getPosition,
	VR_OVR_SetPredictionTime
};



void VR_OVR_GetFOV(float *fovx, float *fovy)
{
	*fovx = hmd->DefaultEyeFov[ovrEye_Left].LeftTan + hmd->DefaultEyeFov[ovrEye_Left].RightTan;
	*fovy = hmd->DefaultEyeFov[ovrEye_Left].UpTan + hmd->DefaultEyeFov[ovrEye_Left].DownTan;
}


void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (hmd)
	{
		*xpos = hmd->WindowsPos.x;
		*ypos = hmd->WindowsPos.y;
	}
}

void VR_OVR_QuatToEuler(ovrQuatf q, vec3_t e)
{
	vec_t w = q.w;
	vec_t Q[3] = {q.x,q.y,q.z};

	vec_t ww  = w*w;
    vec_t Q11 = Q[1]*Q[1];
    vec_t Q22 = Q[0]*Q[0];
    vec_t Q33 = Q[2]*Q[2];

    vec_t psign = -1.0f;

    vec_t s2 = -2.0f * (-w*Q[0] + Q[1]*Q[2]);

    if (s2 < -1.0f + 0.0000001f)
    { // South pole singularity
        e[YAW] = 0.0f;
        e[PITCH] = -M_PI / 2.0f;
        e[ROLL] = atan2(2.0f*(-Q[1]*Q[0] + w*Q[2]),
		                ww + Q22 - Q11 - Q33 );
    }
    else if (s2 > 1.0f - 0.0000001f)
    {  // North pole singularity
        e[YAW] = 0.0f;
        e[PITCH] = M_PI / 2.0f;
        e[ROLL] = atan2(2.0f*(-Q[1]*Q[0] + w*Q[2]),
		                ww + Q22 - Q11 - Q33);
    }
    else
    {
        e[YAW] = -atan2(-2.0f*(w*Q[1] - -Q[0]*Q[2]),
		                ww + Q33 - Q11 - Q22);
        e[PITCH] = asin(s2);
        e[ROLL] = atan2(2.0f*(w*Q[2] - -Q[1]*Q[0]),
		                ww + Q11 - Q22 - Q33);
    }      

	e[PITCH] = -RAD2DEG(e[PITCH]);
	e[YAW] = RAD2DEG(e[YAW]);
	e[ROLL] = -RAD2DEG(e[ROLL]);

}

int32_t VR_OVR_getOrientation(float euler[3])
{
	double time = 0.0;

	if (!hmd)
		return 0;

	if (vr_ovr_autoprediction->value > 0)
		time = (frameTime.EyeScanoutSeconds[ovrEye_Left] + frameTime.EyeScanoutSeconds[ovrEye_Right]) / 2.0;
	else
		time = ovr_GetTimeInSeconds() + prediction_time;
	trackingState = ovrHmd_GetTrackingState(hmd, time);
	if (trackingState.StatusFlags & (ovrStatus_OrientationTracked ) )
	{
		VR_OVR_QuatToEuler(trackingState.HeadPose.ThePose.Orientation,euler);
		return 1;
	}
	return 0;
}

void SCR_CenterAlert (char *str);
int32_t VR_OVR_getPosition(float pos[3])
{
	qboolean tracked = 0;
	if (!hmd)
		return 0;

	if (!sensorEnabled)
		VR_OVR_InitSensor();

	if (sensorEnabled)
	{
		tracked = trackingState.StatusFlags & ovrStatus_PositionTracked ? 1 : 0;
		if (tracked)
		{
			ovrPosef pose = trackingState.HeadPose.ThePose;
			VectorSet(pos,-pose.Position.z,pose.Position.x,pose.Position.y);
			VectorScale(pos,(PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M),pos);
		}

		if (hmd->TrackingCaps && ovrTrackingCap_Position)
		{ 
			if (tracked && !positionTracked)
				SCR_CenterAlert("Position tracking enabled");
			else if (!tracked && positionTracked)
				SCR_CenterAlert("Position tracking interrupted");

		}

		positionTracked = tracked;
	}
	return tracked;
}


void VR_OVR_ResetHMDOrientation()
{
	if (hmd)
		ovrHmd_RecenterPose(hmd);
}

ovrBool VR_OVR_InitSensor()
{
	unsigned int sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;

	if (sensorEnabled)
	{
		sensorEnabled = 0;
	}

	sensorCaps |= ovrTrackingCap_Position;

	sensorEnabled = ovrHmd_ConfigureTracking(hmd,sensorCaps, ovrTrackingCap_Orientation);
	if (sensorEnabled)
	{
		ovrTrackingState ss;
		ss = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
		Com_Printf("VR_OVR: Successfully initialized sensors!\n");

		if (ss.StatusFlags & ovrStatus_PositionConnected)
			Com_Printf("...sensor has position tracking support\n");
		if (ss.StatusFlags & ovrStatus_OrientationTracked)
			Com_Printf("...orientation tracking enabled\n");
		if (ss.StatusFlags & ovrStatus_PositionTracked)
			Com_Printf("...position tracking enabled\n");
	}
	return sensorEnabled;
}

int32_t VR_OVR_RenderLatencyTest(vec4_t color) 
{
	qboolean use = (qboolean) ovrHmd_ProcessLatencyTest(hmd,ovrLatencyColor);
	color[0] = ovrLatencyColor[0] / 255.0f;
	color[1] = ovrLatencyColor[1] / 255.0f;
	color[2] = ovrLatencyColor[2] / 255.0f;
	color[3] = 1.0f;
	return use;
}


int32_t VR_OVR_SetPredictionTime(float time)
{
	prediction_time = time / 1000.0;
	Com_Printf("VR_OVR: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}

void VR_OVR_FrameStart()
{

	const char *results = ovrHmd_GetLatencyTestResult(hmd);
	if (results && strncmp(results,"",1))
	{
		static float lastms = 0;
		float ms;
		if (sscanf(results,"RESULT=%f ",&ms) && ms != lastms)
		{
			Cvar_SetInteger("vr_prediction",(int) ms);
			lastms = ms;
		}
	}

	if (vr_ovr_lowpersistence->modified)
	{
		unsigned int caps = 0;
		if (hmd->HmdCaps & ovrHmdCap_DynamicPrediction)
			caps |= ovrHmdCap_DynamicPrediction;

		if (hmd->HmdCaps & ovrHmdCap_LowPersistence && vr_ovr_lowpersistence->value)
			caps |= ovrHmdCap_LowPersistence;
		
		ovrHmd_SetEnabledCaps(hmd,caps);
		vr_ovr_lowpersistence->modified = false;
	}

	if (!withinFrame)
	{
		frameTime = ovrHmd_BeginFrameTiming(hmd,0);
	}
	else
	{
		ovrHmd_EndFrameTiming(hmd);
		ovrHmd_ResetFrameTiming(hmd,0);
		frameTime = ovrHmd_BeginFrameTiming(hmd,0);
	}
	withinFrame = true;
}

void VR_OVR_FrameEnd()
{
	ovrHmd_EndFrameTiming(hmd);
	withinFrame = false;
}

int32_t VR_OVR_Enable()
{
	int32_t failure = 0;
	unsigned int hmdCaps = 0;
	qboolean isDebug = false;

	if (!vr_ovr_enable->value)
		return 0;

	if (!libovrInitialized)
	{
		libovrInitialized = ovr_Initialize();
		if (!libovrInitialized)
		{
			Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
			return 0;
		} else {
			Com_Printf("VR_OVR: %s initialized...\n",ovr_GetVersionString());
		}
	}

	{
		int numDevices = ovrHmd_Detect();
		int i;
		Com_Printf("VR_OVR: Enumerating devices...\n");
		Com_Printf("VR_OVR: Found %i devices\n", numDevices);
		for (i = 0 ; i < numDevices ; i++)
		{
			ovrHmd hmd = ovrHmd_Create(i);
			if (hmd)
			{
				Com_Printf("VR_OVR: Found device '%s'\n",hmd->ProductName);
				ovrHmd_Destroy(hmd);
			}
		}
	}

	Com_Printf("VR_OVR: Initializing HMD: ");
	
	withinFrame = false;
	hmd = ovrHmd_Create(0);
	
	if (!hmd)
	{
		Com_Printf("no HMD detected!\n");
		failure = 1;
	}
	else {
		Com_Printf("ok!\n");
	}


	if (failure && vr_ovr_debug->value)
	{
		ovrHmdType temp = ovrHmd_None;
		switch((int) vr_ovr_debug->value)
		{
			default:
				temp = ovrHmd_DK1;
				break;
			case 2:
				temp = ovrHmd_DKHD;
				break;
			case 3:
				temp = ovrHmd_DK2;
				break;

		}
		hmd = ovrHmd_CreateDebug(temp);
		isDebug = true;
		Com_Printf("VR_OVR: Creating debug HMD...\n");
	}

	if (!hmd)
		return 0;

	if (hmd->HmdCaps & ovrHmdCap_ExtendDesktop)
	{
		Com_Printf("...running in extended desktop mode\n");
	} else if (!isDebug) {
		Com_Printf("...running in Direct HMD mode\n");
		Com_Printf("...Direct HMD mode is unsupported at this time\n");
		VR_OVR_Disable();
		VR_OVR_Shutdown();
		return 0;
	} else {
		Com_Printf("...debug device positioned at (%i,%i)\n",hmd->WindowsPos.x,hmd->WindowsPos.y);

	}

	if (hmd->HmdCaps & ovrHmdCap_Available)
		Com_Printf("...sensor is available\n");
	if (hmd->HmdCaps & ovrHmdCap_LowPersistence)
		Com_Printf("...supports low persistance\n");
	if (hmd->HmdCaps & ovrHmdCap_DynamicPrediction)
		Com_Printf("...supports dynamic motion prediction\n");
	if (hmd->TrackingCaps & ovrTrackingCap_Position)
		Com_Printf("...supports position tracking\n");

	Com_Printf("...has type %s\n", hmd->ProductName);
	Com_Printf("...has %ux%u native resolution\n", hmd->Resolution.w, hmd->Resolution.h);

	if (!VR_OVR_InitSensor())
	{
		Com_Printf("VR_OVR: Sensor initialization failed!\n");
	}

	ovrHmd_ResetFrameTiming(hmd,0);
	return 1;
}

void VR_OVR_Disable()
{
	withinFrame = false;
	if (hmd)
	{
		ovrHmd_Destroy(hmd);
	}
	hmd = NULL;
}


int32_t VR_OVR_Init()
{

	/*
	if (!libovrInitialized)
	{
		libovrInitialized = ovr_Initialize();
		if (!libovrInitialized)
		{
			Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
			return 0;
		} else {
			Com_Printf("VR_OVR: %s initialized...\n",ovr_GetVersionString());
		}
	}
	*/

	vr_ovr_timewarp = Cvar_Get("vr_ovr_timewarp","1",CVAR_ARCHIVE);
	vr_ovr_supersample = Cvar_Get("vr_ovr_supersample","1.0",CVAR_ARCHIVE);
	vr_ovr_maxfov = Cvar_Get("vr_ovr_maxfov","0",CVAR_ARCHIVE);
	vr_ovr_lowpersistence = Cvar_Get("vr_ovr_lowpersistence","1",CVAR_ARCHIVE);
	vr_ovr_lumoverdrive = Cvar_Get("vr_ovr_lumoverdrive","1",CVAR_ARCHIVE);

	vr_ovr_enable = Cvar_Get("vr_ovr_enable","1",CVAR_ARCHIVE);
	vr_ovr_dk2_color_hack = Cvar_Get("vr_ovr_dk2_color_hack","1",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_ARCHIVE);
	vr_ovr_autoprediction = Cvar_Get("vr_ovr_autoprediction","1",CVAR_ARCHIVE);

	return 1;
}

void VR_OVR_Shutdown()
{
	if (libovrInitialized)
		ovr_Shutdown();
	libovrInitialized = 0;
}



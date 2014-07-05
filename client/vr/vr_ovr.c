#include "include/vr.h"
#include "include/vr_ovr.h"
#include "OVR_CAPI.h"
#include <SDL.h>

void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos);
void VR_OVR_GetState(vr_param_t *state);
void VR_OVR_FrameStart();
void VR_OVR_FrameEnd();

int32_t VR_OVR_Enable();
void VR_OVR_Disable();
int32_t VR_OVR_Init();
void VR_OVR_Shutdown();
int32_t VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
int32_t VR_OVR_SetPredictionTime(float time);

cvar_t *vr_ovr_debug;
cvar_t *vr_ovr_maxfov;
cvar_t *vr_ovr_supersample;
cvar_t *vr_ovr_latencytest;
cvar_t *vr_ovr_enable;
cvar_t *vr_ovr_autoprediction;
cvar_t *vr_ovr_timewarp;

unsigned char ovrLatencyColor[3];
qboolean useLatencyColor = false;

ovr_attrib_t ovrConfig = {0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}};

vr_param_t ovrState = {0, 0, 0, 0, 0, 0};

ovrHmd hmd;
ovrHmdDesc hmdDesc; 
ovrEyeRenderDesc eyeDesc[2];
qboolean withinFrame = false;

ovrPosef framePose;
ovrFrameTiming frameTime;
static ovrBool sensorEnabled = 0;

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
	NULL,
	VR_OVR_SetPredictionTime
};



void VR_OVR_GetFOV(float *fovx, float *fovy)
{
	*fovx = hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan + hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan;
	*fovy = hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan + hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan;
}


void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (hmd)
	{
		*xpos = hmdDesc.WindowsPos.x;
		*ypos = hmdDesc.WindowsPos.y;
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
	if (!hmd)
		return 0;
	if (withinFrame && vr_ovr_autoprediction->value > 0)
	{
		// really, we should be checking both eyes
		// however, for the way we are using the SDK this should always be equal
		// mostly we are just letting the SDK have it's fun with timing
		framePose = ovrHmd_GetEyePose(hmd,ovrEye_Left);
	} else {
		// if not, do this by hand
		ovrSensorState ss;
		ss = ovrHmd_GetSensorState(hmd, ovr_GetTimeInSeconds() + prediction_time);
		if (ss.StatusFlags & (ovrStatus_OrientationTracked ) )
		{ 
			framePose = ss.Predicted.Pose;		
		}
	}
	VR_OVR_QuatToEuler(framePose.Orientation,euler);
	return 1;
}

void VR_OVR_ResetHMDOrientation()
{
	if (hmd)
		ovrHmd_ResetSensor(hmd);
}

void VR_OVR_InitSensor()
{
	unsigned int sensorCaps = ovrSensorCap_Orientation | ovrSensorCap_YawCorrection;

	if (sensorEnabled)
	{
		ovrHmd_StopSensor(hmd);
		sensorEnabled = 0;
	}

	Com_Printf("Initializing sensor: ");
	sensorEnabled = ovrHmd_StartSensor(hmd,sensorCaps, ovrSensorCap_Orientation);
	if (sensorEnabled)
	{
		ovrSensorState ss;
		ss = ovrHmd_GetSensorState(hmd, ovr_GetTimeInSeconds() + prediction_time);
		Com_Printf("ok!\n");

		if (ss.StatusFlags & ovrStatus_HmdConnected)
			Com_Printf("...sensor has HMD connected\n");
		if (ss.StatusFlags & ovrStatus_PositionConnected)
			Com_Printf("...sensor has position tracking support\n");
		if (ss.StatusFlags & ovrStatus_OrientationTracked)
			Com_Printf("...orientation tracking enabled\n");
		if (ss.StatusFlags & ovrStatus_PositionTracked)
			Com_Printf("...position tracking enabled\n");
	} else {
		Com_Printf("failed!\n");
	}
}

int32_t VR_OVR_RenderLatencyTest(vec4_t color) 
{
	qboolean use = (qboolean) ovrHmd_GetLatencyTestDrawColor(hmd,ovrLatencyColor);
	color[0] = ovrLatencyColor[0] / 255.0f;
	color[1] = ovrLatencyColor[1] / 255.0f;
	color[2] = ovrLatencyColor[2] / 255.0f;
	color[3] = 1.0f;
	return (vr_ovr_latencytest->value && use);
}


int32_t VR_OVR_SetPredictionTime(float time)
{
	prediction_time = time / 1000.0;
	Com_Printf("VR_OVR: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}

void SCR_CenterAlert (char *str);
void VR_OVR_FrameStart()
{
	if (vr_ovr_latencytest->value)
	{
			const char *results = ovrHmd_GetLatencyTestResult(hmd);
			if (results)
			{
				float ms;
				Com_Printf("VR_OVR: %s\n",results);		
				if (sscanf(results,"RESULT=%f ",&ms))
				{
					char buffer[64];
					SDL_snprintf(buffer,sizeof(buffer),"%.1fms latency",ms);
					SCR_CenterAlert(buffer);
					Cvar_SetInteger("vr_prediction",(int) ms);
				}
			}
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

	if (!vr_ovr_enable->value)
		return 0;
	Com_Printf("VR_OVR: Initializing HMD: ");
	
	
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
				temp = ovrHmd_CrystalCoveProto;
				break;
			case 4:
				temp = ovrHmd_DK2;
				break;

		}
		hmd = ovrHmd_CreateDebug(temp);
		Com_Printf("VR_OVR: Creating debug HMD...\n");
	}

	if (!hmd)
		return 0;

//	hmdCaps = ovrHmd_GetEnabledCaps(hmd);

	ovrHmd_SetEnabledCaps(hmd,ovrHmdCap_LatencyTest);
	ovrHmd_GetDesc(hmd, &hmdDesc);
	if (hmdDesc.HmdCaps & ovrHmdCap_Available)
		Com_Printf("...sensor is available\n");
	if (hmdDesc.HmdCaps & ovrHmdCap_LowPersistence)
		Com_Printf("...supports low persistance\n");
	if (hmdDesc.HmdCaps & ovrHmdCap_DynamicPrediction)
		Com_Printf("...supports dynamic motion prediction\n");
	if (hmdDesc.HmdCaps & ovrHmdCap_LatencyTest)
	{
		Com_Printf("...found latency tester\n");
		Cvar_SetInteger("vr_ovr_latencytest",1);
	}
	if (hmdDesc.SensorCaps & ovrSensorCap_Position)
		Com_Printf("...supports position tracking\n");

	Com_Printf("...has type %s\n", hmdDesc.ProductName);
	Com_Printf("...has %ux%u native resolution\n", hmdDesc.Resolution.w, hmdDesc.Resolution.h);

	VR_OVR_InitSensor();

	ovrHmd_ResetFrameTiming(hmd,0);
	return 1;
}

void VR_OVR_Disable()
{
	if (hmd)
	{
		ovrHmd_StopSensor(hmd);
		ovrHmd_Destroy(hmd);
	}
	hmd = NULL;
}


int32_t VR_OVR_Init()
{
	ovrBool init = ovr_Initialize();
	vr_ovr_timewarp = Cvar_Get("vr_ovr_timewarp","1",CVAR_ARCHIVE);
	vr_ovr_supersample = Cvar_Get("vr_ovr_supersample","1.0",CVAR_ARCHIVE);
	vr_ovr_maxfov = Cvar_Get("vr_ovr_maxfov","0",CVAR_ARCHIVE);
	vr_ovr_latencytest = Cvar_Get("vr_ovr_latencytest","0",0);
	vr_ovr_enable = Cvar_Get("vr_ovr_enable","1",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_ARCHIVE);
	vr_ovr_autoprediction = Cvar_Get("vr_ovr_autoprediction","1",CVAR_ARCHIVE);

	if (!init)
	{
		Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
		return 0;
	}

	Com_Printf("VR_OVR: Oculus Rift support initialized...\n");

	return 0;
}

void VR_OVR_Shutdown()
{
	ovr_Shutdown();
}



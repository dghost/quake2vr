#include "include/vr.h"
#include "include/vr_rift.h"
#ifdef OCULUS_DYNAMIC
#include "oculus_dynamic/oculus_dynamic.h"
#else
#define OVR_ALIGNAS(x)
#include "OVR_CAPI.h"
#endif
#include <SDL.h>

void VR_Rift_GetHMDPos(int32_t *xpos, int32_t *ypos);
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
cvar_t *vr_rift_device;
cvar_t *vr_rift_supersample;
cvar_t *vr_rift_enable;
cvar_t *vr_rift_autoprediction;
cvar_t *vr_rift_timewarp;
cvar_t *vr_rift_dk2_color_hack;
cvar_t *vr_rift_lowpersistence;
cvar_t *vr_rift_lumoverdrive;
cvar_t *vr_rift_distortion_fade;
cvar_t *vr_rift_latencytest;
cvar_t *vr_rift_trackingloss;

ovrHmd hmd;
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
	VR_Rift_GetHMDPos,
	VR_Rift_GetHMDResolution
};

static riftname_t hmdnames[255];


void VR_Rift_GetFOV(float *fovx, float *fovy)
{
	*fovx = hmd->DefaultEyeFov[ovrEye_Left].LeftTan + hmd->DefaultEyeFov[ovrEye_Left].RightTan;
	*fovy = hmd->DefaultEyeFov[ovrEye_Left].UpTan + hmd->DefaultEyeFov[ovrEye_Left].DownTan;
}


void VR_Rift_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (hmd)
	{
		*xpos = hmd->WindowsPos.x;
		*ypos = hmd->WindowsPos.y;
	}
}

void VR_Rift_GetHMDResolution(int32_t *width, int32_t *height)
{
	if (hmd)
	{
		*width = hmd->Resolution.w;
		*height = hmd->Resolution.h;
	}
}


void VR_Rift_QuatToEuler(ovrQuatf q, vec3_t e)
{
	vec_t w = q.w;
	vec_t Q[3] = {q.x,q.y,q.z};

	vec_t ww  = w*w;
    vec_t Q11 = Q[1]*Q[1];
    vec_t Q22 = Q[0]*Q[0];
    vec_t Q33 = Q[2]*Q[2];

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

int32_t VR_Rift_getOrientation(float euler[3])
{
	double time = 0.0;

	if (!hmd)
		return 0;

	if (vr_rift_autoprediction->value > 0)
		time = (frameTime.EyeScanoutSeconds[ovrEye_Left] + frameTime.EyeScanoutSeconds[ovrEye_Right]) / 2.0;
	else
		time = ovr_GetTimeInSeconds() + prediction_time;
	trackingState = ovrHmd_GetTrackingState(hmd, time);
	if (trackingState.StatusFlags & ovrStatus_OrientationTracked )
	{
		vec3_t temp;
		VR_Rift_QuatToEuler(trackingState.HeadPose.ThePose.Orientation,euler);
		if (trackingState.StatusFlags & ovrStatus_PositionTracked) {
			VR_Rift_QuatToEuler(trackingState.LeveledCameraPose.Orientation,temp);
			renderExport.cameraYaw = euler[YAW] - temp[YAW];
			AngleClamp(&renderExport.cameraYaw);
		} else {
			renderExport.cameraYaw = 0.0;
		}
		return 1;
	}
	return 0;
}

void SCR_CenterAlert (char *str);
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
			VectorSet(pos,-pose.Position.z,pose.Position.x,pose.Position.y);
			VectorScale(pos,(PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M),pos);
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
		ovrHmd_RecenterPose(hmd);
}

ovrBool VR_Rift_InitSensor()
{
	uint32_t sensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection;

	if (sensorEnabled)
	{
		sensorEnabled = 0;
	}

	sensorCaps |= ovrTrackingCap_Position;

	sensorEnabled = ovrHmd_ConfigureTracking(hmd,sensorCaps, ovrTrackingCap_Orientation);

	return sensorEnabled;
}

int32_t VR_Rift_RenderLatencyTest(vec4_t color) 
{
	uint8_t ovrLatencyColor[3] = {0, 0, 0};

	qboolean use = (qboolean) false;
	if (hmd->Type >= ovrHmd_DK2)
		use = (qboolean) ovrHmd_GetLatencyTest2DrawColor(hmd,ovrLatencyColor);
	else if (vr_rift_latencytest->value)
		use = (qboolean) ovrHmd_ProcessLatencyTest(hmd,ovrLatencyColor);

	color[0] = ovrLatencyColor[0] / 255.0f;
	color[1] = ovrLatencyColor[1] / 255.0f;
	color[2] = ovrLatencyColor[2] / 255.0f;
	color[3] = 1.0f;
	return use;
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

	if (hmd->Type < ovrHmd_DK2 && vr_rift_latencytest->value)
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
	}

	if (vr_rift_lowpersistence->modified)
	{
		uint32_t caps = 0;
		if (hmd->HmdCaps & ovrHmdCap_DynamicPrediction)
			caps |= ovrHmdCap_DynamicPrediction;

		if (hmd->HmdCaps & ovrHmdCap_LowPersistence && vr_rift_lowpersistence->value)
			caps |= ovrHmdCap_LowPersistence;
		
		ovrHmd_SetEnabledCaps(hmd,caps);
		vr_rift_lowpersistence->modified = false;
	}

	if (!renderExport.withinFrame)
	{
		frameTime = ovrHmd_BeginFrameTiming(hmd,0);
	}
	else
	{
		ovrHmd_EndFrameTiming(hmd);
		ovrHmd_ResetFrameTiming(hmd,0);
		frameTime = ovrHmd_BeginFrameTiming(hmd,0);
	}
	renderExport.withinFrame = true;
}

void VR_Rift_FrameEnd()
{
	ovrHmd_EndFrameTiming(hmd);
	renderExport.withinFrame = false;
}

float VR_Rift_GetGammaMin()
{
	float result = 0.0;
	if (hmd && hmd->Type >= ovrHmd_DK2)
		result = 1.0/255.0;
	return result;
}

riftname_t *VR_Rift_GetNameList()
{
	return hmdnames;
}

int32_t VR_Rift_Enable()
{
	int32_t failure = 0;
	qboolean isDebug = false;
	uint32_t device = 0;
	if (!vr_rift_enable->value)
		return 0;

	if (!libovrInitialized)
	{
#ifdef OCULUS_DYNAMIC
		const char* failed_function;
		static const char *dllnames[] = {"libovr", "libovr_043", 0};
		oculus_library_handle = Sys_FindLibrary(dllnames);
		if (!oculus_library_handle) {
			Com_Printf("VR_Rift: Fatal error: could not load Oculus library\n");
			return 0;
		}

		ovr_dynamic_load_result res = oculus_dynamic_load_handle(oculus_library_handle, &failed_function);
		if (res != OVR_DYNAMIC_RESULT_SUCCESS) {
			if (res == OVR_DYNAMIC_RESULT_LIBOVR_COULD_NOT_LOAD_FUNCTION) {
				Com_Printf("VR_Rift: Fatal error: function %s not found in oculus library\n", failed_function);
			} else {
				Com_Printf("VR_Rift: Fatal error: could not load Oculus library\n");
			}
			SDL_UnloadObject(oculus_library_handle);
			oculus_library_handle = NULL;
			return 0;
		}
#endif

#if OVR_MAJOR_VERSION >= 5
		libovrInitialized = ovr_Initialize(0);
#else
        libovrInitialized = ovr_Initialize();
#endif
		if (!libovrInitialized)
		{
			Com_Printf("VR_Rift: Fatal error: could not initialize LibOVR!\n");
#ifdef OCULUS_DYNAMIC
			SDL_UnloadObject(oculus_library_handle);
			oculus_library_handle = NULL;
#endif
			return 0;
		} else {
			Com_Printf("VR_Rift: %s initialized...\n",ovr_GetVersionString());
		}
	}

	{
		int numDevices = ovrHmd_Detect();
		int i;
		qboolean found = false;

		memset(hmdnames,0,sizeof(riftname_t) * 255);

		Com_Printf("VR_Rift: Enumerating devices...\n");
		Com_Printf("VR_Rift: Found %i devices\n", numDevices);
		for (i = 0 ; i < numDevices ; i++)
		{
			ovrHmd tempHmd = ovrHmd_Create(i);
			if (tempHmd)
			{
				Com_Printf("VR_Rift: Found device #%i '%s' s/n:%s\n",i,tempHmd->ProductName, tempHmd->SerialNumber);
				sprintf(hmdnames[i].label,"%i: %s",i + 1, tempHmd->ProductName);
				sprintf(hmdnames[i].serialnumber,"%s",tempHmd->SerialNumber);
				if (!strncmp(vr_rift_device->string,tempHmd->SerialNumber,strlen(tempHmd->SerialNumber)))
				{
					device = i;
					found = true;
				}
				ovrHmd_Destroy(tempHmd);
			}
		}
	}

	Com_Printf("VR_Rift: Initializing HMD: ");
	
	renderExport.withinFrame = false;
	hmd = ovrHmd_Create(device);
	
	if (!hmd)
	{
		Com_Printf("no HMD detected!\n");
		failure = 1;
	}
	else {
		Com_Printf("ok!\n");
	}


	if (failure && vr_rift_debug->value)
	{
		ovrHmdType temp = ovrHmd_None;
		switch((int) vr_rift_debug->value)
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
		Com_Printf("VR_Rift: Creating debug HMD...\n");
	}

	if (!hmd)
		return 0;

	if (hmd->HmdCaps & ovrHmdCap_ExtendDesktop)
	{
		Com_Printf("...running in extended desktop mode\n");
	} else if (!isDebug) {
		Com_Printf("...running in Direct HMD mode\n");
		Com_Printf("...Direct HMD mode is unsupported at this time\n");
		VR_Rift_Disable();
		VR_Rift_Shutdown();
		return 0;
	}

	Com_Printf("...device positioned at (%i,%i)\n",hmd->WindowsPos.x,hmd->WindowsPos.y);

	if (hmd->HmdCaps & ovrHmdCap_Available)
		Com_Printf("...sensor is available\n");
	if (hmd->HmdCaps & ovrHmdCap_LowPersistence)
		Com_Printf("...supports low persistance\n");
	if (hmd->HmdCaps & ovrHmdCap_DynamicPrediction)
		Com_Printf("...supports dynamic motion prediction\n");
	if (hmd->TrackingCaps & ovrTrackingCap_Position) {
		renderExport.positionTracked = (qboolean) true;
		Com_Printf("...supports position tracking\n");
	} else {
		renderExport.positionTracked = (qboolean) false;
	}
	Com_Printf("...has type %s\n", hmd->ProductName);
	Com_Printf("...has %ux%u native resolution\n", hmd->Resolution.w, hmd->Resolution.h);

	if (!VR_Rift_InitSensor())
	{
		Com_Printf("VR_Rift: Sensor initialization failed!\n");
	}

	ovrHmd_ResetFrameTiming(hmd,0);
	return 1;
}

void VR_Rift_Disable()
{
	renderExport.withinFrame = false;
	if (hmd)
	{
		ovrHmd_Destroy(hmd);
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
	vr_rift_timewarp = Cvar_Get("vr_rift_timewarp","1",CVAR_CLIENT);
	vr_rift_supersample = Cvar_Get("vr_rift_supersample","1.0",CVAR_CLIENT);
	vr_rift_maxfov = Cvar_Get("vr_rift_maxfov","0",CVAR_CLIENT);
	vr_rift_lumoverdrive = Cvar_Get("vr_rift_lumoverdrive","1",CVAR_CLIENT);
	vr_rift_lowpersistence = Cvar_Get("vr_rift_lowpersistence","1",CVAR_CLIENT);
	vr_rift_latencytest = Cvar_Get("vr_rift_latencytest","0",CVAR_CLIENT);
	vr_rift_enable = Cvar_Get("vr_rift_enable","1",CVAR_CLIENT);
	vr_rift_dk2_color_hack = Cvar_Get("vr_rift_dk2_color_hack","1",CVAR_CLIENT);
	vr_rift_device = Cvar_Get("vr_rift_device","",CVAR_CLIENT);
	vr_rift_debug = Cvar_Get("vr_rift_debug","0",CVAR_CLIENT);
	vr_rift_distortion_fade = Cvar_Get("vr_rift_distortion_fade","0",CVAR_CLIENT);
	vr_rift_autoprediction = Cvar_Get("vr_rift_autoprediction","1",CVAR_CLIENT);

	return 1;
}

void VR_Rift_Shutdown()
{
	if (libovrInitialized)
		ovr_Shutdown();
	libovrInitialized = 0;
}



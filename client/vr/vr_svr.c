#include "include/vr.h"
#ifndef NO_STEAM
#include "include/vr_svr.h"
#include "include/vr_steamvr.h"

svr_settings_t svr_settings;
static float predictionTime;
static qboolean hasBeenInitialized = 0;
static qboolean sensorThisFrame = false;

cvar_t *vr_svr_distortion;
cvar_t *vr_svr_debug;
cvar_t *vr_svr_enable;
cvar_t *vr_svr_positiontracking;

static float orientation[3];
static float position[3];

int32_t VR_SVR_Enable()
{
	if (!vr_svr_enable->value)
		return 0;

	if (!hasBeenInitialized)
	{
		hasBeenInitialized = SteamVR_Init();
		if (!hasBeenInitialized)
		{
			Com_Printf("VR_SVR: Fatal error initializing SteamVR support");
			return 0;
		}
		Com_Printf("VR_SVR: SteamVR support initialized...\n");
	}

	if (!SteamVR_Enable())
	{
		Com_Printf("VR_SVR: Error, no HMD detected!\n");
		return 0;
	}

	else {
		qboolean result = false;
		Com_Printf("VR_SVR: Initializing HMD:");
		result = (qboolean) SteamVR_GetSettings(&svr_settings);
		if (!result)
		{
			Com_Printf(" failed!\n");
			return 0;
		}
		Com_Printf(" ok!\n");
		Com_Printf("...found HMD of type: %s\n",svr_settings.deviceName);
		Com_Printf("...detected IPD %.1fmm\n",svr_settings.ipd * 1000.0f * 2.0f);
	}
	return 1;
}

void VR_SVR_Disable()
{
	svr_settings.initialized = 0;
	SteamVR_Disable();
}

int32_t VR_SVR_Init()
{
	if (!hasBeenInitialized)
	{
		hasBeenInitialized = SteamVR_Init();
		if (!hasBeenInitialized)
		{
			Com_Printf("VR_SVR: Fatal error initializing SteamVR support");
			return 0;
		}
		Com_Printf("VR_SVR: SteamVR support initialized...\n");
	}

	vr_svr_positiontracking = Cvar_Get("vr_svr_positiontracking","0",0);
	vr_svr_enable = Cvar_Get("vr_svr_enable","1",CVAR_CLIENT);
	vr_svr_distortion = Cvar_Get("vr_svr_distortion","2",CVAR_CLIENT);
	vr_svr_debug = Cvar_Get("vr_svr_debug","0",0);
	return 1;
}


void VR_SVR_Shutdown()
{
	if (hasBeenInitialized)
		SteamVR_Shutdown();
	hasBeenInitialized = 0;
}

void VR_SVR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (svr_settings.initialized)
	{
		*xpos = svr_settings.xPos;
		*ypos = svr_settings.yPos;
	}
}

int32_t VR_SVR_SetPredictionTime(float time)
{
	predictionTime = time / 1000.0f;
	Com_Printf("VR_SVR: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}


void VR_SVR_Frame()
{
	sensorThisFrame = false;

}



int32_t VR_SVR_GetOrientation(float euler[3])
{
	if (!sensorThisFrame)
		SteamVR_GetOrientationAndPosition(predictionTime,orientation,position);

	VectorCopy(orientation,euler);
	return 1;
}

int32_t VR_SVR_GetPosition(float pos[3])
{
	if (!vr_svr_positiontracking->value)
		return 0;

	if (!sensorThisFrame)
		SteamVR_GetOrientationAndPosition(predictionTime,orientation,position);

	VectorCopy(position,pos);
	return 1;
}

hmd_interface_t hmd_steam = {
	HMD_STEAM,
	VR_SVR_Init,
	VR_SVR_Shutdown,
	VR_SVR_Enable,
	VR_SVR_Disable,
	VR_SVR_Frame,
	NULL,
	SteamVR_ResetHMDOrientation,
	VR_SVR_GetOrientation,
	VR_SVR_GetPosition,
	VR_SVR_SetPredictionTime,
	VR_SVR_GetHMDPos,
	NULL
};

#endif //NO_STEAM
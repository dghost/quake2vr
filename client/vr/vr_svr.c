#include "include/vr.h"
#ifndef NO_STEAM
#include "include/vr_svr.h"
#include "include/vr_steamvr.h"

svr_settings_t svr_settings;
static float predictionTime;

cvar_t *vr_svr_distortion;
cvar_t *vr_svr_debug;
cvar_t *vr_svr_enable;

int32_t VR_SVR_Enable()
{
	if (!vr_svr_enable->value)
		return 0;
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
		Com_Printf("...detected IPD %.1fmm\n",svr_settings.ipd * 1000.0f);
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
	int32_t result = SteamVR_Init();
	if (result)
		Com_Printf("VR_SVR: SteamVR support initialized...\n");

	vr_svr_enable = Cvar_Get("vr_svr_enable","1",CVAR_ARCHIVE);
	vr_svr_distortion = Cvar_Get("vr_svr_distortion","2",CVAR_ARCHIVE);
	vr_svr_debug = Cvar_Get("vr_svr_debug","0",0);
	return result;
}


void VR_SVR_Shutdown()
{
	SteamVR_Shutdown();
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

int32_t VR_SVR_GetOrientation(float euler[3])
{
	vec3_t pos;
	SteamVR_GetOrientationAndPosition(predictionTime,euler,pos);
	return 1;
}

void VR_SVR_Frame()
{


}

hmd_interface_t hmd_steam = {
	HMD_STEAM,
	VR_SVR_Init,
	VR_SVR_Shutdown,
	VR_SVR_Enable,
	VR_SVR_Disable,
	VR_SVR_GetHMDPos,
	VR_SVR_Frame,
	SteamVR_ResetHMDOrientation,
	VR_SVR_GetOrientation,
	NULL,
	VR_SVR_SetPredictionTime
};

#else

hmd_interface_t hmd_steam = {
	HMD_STEAM,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


#endif //NO_STEAM
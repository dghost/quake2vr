#include "vr.h"
#ifndef NO_STEAM
#include "vr_svr.h"
#include "vr_steamvr.h"



Sint32 VR_SVR_Enable()
{

	return SteamVR_Enable();
}

void VR_SVR_Disable()
{

	SteamVR_Disable();
}

Sint32 VR_SVR_Init()
{
	Com_Printf("VR_SVR: SteamVR support initialized...\n");


	return SteamVR_Init();
}


void VR_SVR_Shutdown()
{
	SteamVR_Shutdown();

}


hmd_interface_t hmd_steam = {
	HMD_STEAM,
	VR_SVR_Init,
	VR_SVR_Shutdown,
	VR_SVR_Enable,
	VR_SVR_Disable,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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
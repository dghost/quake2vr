#ifndef NO_STEAM
#include "vr.h"
#include "vr_svr.h"
#include "vr_steamvr.h"


hmd_interface_t hmd_steam = {
	HMD_STEAM,
	VR_SVR_Init,
	VR_SVR_Shutdown,
	NULL,
	VR_SVR_Enable,
	VR_SVR_Disable,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


int VR_SVR_Enable()
{

	return SteamVR_Enable();
}

void VR_SVR_Disable()
{

	SteamVR_Disable();
}

int VR_SVR_Init()
{
	Com_Printf("VR_SVR: SteamVR support initialized...\n");


	return SteamVR_Init();
}


void VR_SVR_Shutdown()
{
	SteamVR_Shutdown();

}
#endif //NO_STEAM
#ifndef NO_STEAM

#include "vr_steamvr.h"
#include <steamvr.h>
#include <cstdlib>

static vr::IHmd *hmd;

Sint32 SteamVR_Init()
{
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
	return !hmd;
}

Sint32 SteamVR_Enable()
{
	vr::HmdError error;
	hmd = vr::VR_Init(&error);
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
	return 0;
}

void SteamVR_Disable()
{


}

void SteamVR_Shutdown()
{
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
}






#endif //NO_STEAM
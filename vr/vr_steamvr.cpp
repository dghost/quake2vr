#ifndef NO_STEAM

#include "vr_steamvr.h"
#include <steamvr.h>
#include <cstdlib>

static vr::IHmd *hmd;

#define M_PI 3.14159265358979323846f

int SteamVR_Init()
{
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
	return !hmd;
}

int SteamVR_Enable()
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
#ifndef NO_STEAM

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

#endif //NO_STEAM
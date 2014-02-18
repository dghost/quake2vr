#include "../renderer/r_local.h"
#include "r_vr.h"

#ifndef NO_STEAM

#include "r_vr_svr.h"
#include "../vr/vr_svr.h"



hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

#else

hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

#endif //NO_STEAM
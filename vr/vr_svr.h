#ifndef __VR_SVR_H
#define __VR_SVR_H


#ifndef NO_STEAM

#include "vr.h"


Sint32 VR_SVR_Enable();
void VR_SVR_Disable();
Sint32 VR_SVR_Init();
void VR_SVR_Shutdown();


extern cvar_t *vr_svr_distortion;
extern cvar_t *vr_svr_debug;

/*

void VR_SVR_SetFOV();
void VR_SVR_CalcRenderParam();
void VR_SVR_Frame();
Sint32 VR_SVR_isDeviceAvailable();
Sint32 VR_SVR_getOrientation(float euler[3]);
void VR_SVR_ResetHMDOrientation();
float VR_SVR_GetDistortionScale();
Sint32 VR_SVR_RenderLatencyTest(vec4_t color);
*/


#endif // NO_STEAM

extern hmd_interface_t hmd_steam;



#endif // __VR_SVR_H
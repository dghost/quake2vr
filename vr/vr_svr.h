#ifndef __VR_SVR_H
#define __VR_SVR_H
#ifndef NO_STEAM

#include "vr.h"

/*
void VR_SVR_SetFOV();
void VR_SVR_CalcRenderParam();
void VR_SVR_Frame();
int VR_SVR_Enable();
void VR_SVR_Disable();
int VR_SVR_Init();
void VR_SVR_Shutdown();
int VR_SVR_isDeviceAvailable();
int VR_SVR_getOrientation(float euler[3]);
void VR_SVR_ResetHMDOrientation();
float VR_SVR_GetDistortionScale();
int VR_SVR_RenderLatencyTest(vec4_t color);
*/
extern hmd_interface_t hmd_steam;



#endif // NO_STEAM
#endif // __VR_SVR_H
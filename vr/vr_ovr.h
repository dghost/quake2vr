#ifndef __VR_OVR_H
#define __VR_OVR_H

#include "vr.h"

typedef enum {
	RIFT_NONE,
	RIFT_DK1,
	RIFT_DKHD,
	NUM_RIFT
} rift_t;

extern cvar_t *vr_ovr_driftcorrection;
extern cvar_t *vr_ovr_scale;
extern cvar_t *vr_ovr_chromatic;
extern cvar_t *vr_ovr_prediction;
extern cvar_t *vr_ovr_distortion;
extern cvar_t *vr_ovr_lensdistance;

void VR_OVR_SetFOV();
void VR_OVR_CalcRenderParam();
void VR_OVR_Frame();
int VR_OVR_Enable();
void VR_OVR_Disable();
int VR_OVR_Init();
void VR_OVR_Shutdown();
int VR_OVR_isDeviceAvailable();
int VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
float VR_OVR_GetDistortionScale();
int VR_OVR_RenderLatencyTest(vec4_t color);

extern hmd_interface_t hmd_rift;

#endif
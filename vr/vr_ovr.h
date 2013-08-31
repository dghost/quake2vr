#ifndef __VR_OVR_H
#define __VR_OVR_H

#include "vr.h"
#include "vr_libovr.h"

extern cvar_t *vr_ovr_driftcorrection;
extern cvar_t *vr_ovr_scale;
extern cvar_t *vr_ovr_chromatic;

void VR_OVR_SetFOV();
void VR_OVR_CalcRenderParam();
void VR_OVR_Frame();
int VR_OVR_Enable();
int VR_OVR_Init();



extern hmd_interface_t hmd_rift;

#endif
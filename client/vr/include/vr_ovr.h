#ifndef __VR_OVR_H
#define __VR_OVR_H

#include "vr.h"

typedef struct {
	char label[50];
	char serialnumber[24];
} ovrname_t;

extern cvar_t *vr_ovr_device;
extern cvar_t *vr_ovr_prediction;
extern cvar_t *vr_ovr_maxfov;
extern cvar_t *vr_ovr_autoprediction;
extern cvar_t *vr_ovr_supersample;
extern cvar_t *vr_ovr_timewarp;
extern cvar_t *vr_ovr_dk2_color_hack;
extern cvar_t *vr_ovr_lowpersistence;
extern cvar_t *vr_ovr_lumoverdrive;
extern hmd_interface_t hmd_rift;

#endif
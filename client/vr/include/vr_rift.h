#ifndef __VR_RIFT_H
#define __VR_RIFT_H

#include "vr.h"

typedef struct {
	char label[50];
	char serialnumber[24];
} riftname_t;

typedef struct {
	qboolean withinFrame;
	float cameraYaw;
	qboolean positionTracked;
	qboolean hasPositionLock;
} rift_render_export_t;

extern cvar_t *vr_rift_device;
extern cvar_t *vr_rift_debug;
extern cvar_t *vr_rift_prediction;
extern cvar_t *vr_rift_maxfov;
extern cvar_t *vr_rift_autoprediction;
extern cvar_t *vr_rift_supersample;
extern cvar_t *vr_rift_timewarp;
extern cvar_t *vr_rift_trackingloss;
extern cvar_t *vr_rift_dk2_color_hack;
extern cvar_t *vr_rift_lowpersistence;
extern cvar_t *vr_rift_lumoverdrive;
extern cvar_t *vr_rift_distortion_fade;
extern hmd_interface_t hmd_rift;

#endif //__VR_RIFT_H
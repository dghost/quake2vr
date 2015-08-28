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

extern cvar_t *vr_rift_debug;
extern cvar_t *vr_rift_prediction;
extern cvar_t *vr_rift_maxfov;
extern cvar_t *vr_rift_autoprediction;
extern cvar_t *vr_rift_trackingloss;
extern cvar_t *vr_rift_dk2_color_hack;
extern hmd_interface_t hmd_rift;

#endif //__VR_RIFT_H
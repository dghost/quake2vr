#ifndef __VR_OVR_H
#define __VR_OVR_H

#include "vr.h"

typedef enum {
	RIFT_NONE,
	RIFT_DK1,
	RIFT_DKHD,
	NUM_RIFT
} rift_t;

typedef struct {
	uint32_t hmdHeight;
	uint32_t hmdWidth;
	float aspect;
	float ipd;
	float projOffset;
	float minScale;
	float maxScale;
	float dk[4];
	float chrm[4];
	char deviceString[32];
} ovr_attrib_t;

extern ovr_attrib_t ovrConfig;

extern cvar_t *vr_ovr_driftcorrection;
extern cvar_t *vr_ovr_scale;
extern cvar_t *vr_ovr_prediction;
extern cvar_t *vr_ovr_distortion;
extern cvar_t *vr_ovr_autoscale;
extern cvar_t *vr_ovr_filtermode;
extern cvar_t *vr_ovr_supersample;

extern hmd_interface_t hmd_rift;

#endif
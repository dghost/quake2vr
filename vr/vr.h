#ifndef __R_VR_H
#define __R_VR_H
#include "../qcommon/qcommon.h"

enum vr_eye_t {
	EYE_LEFT = -1,
	EYE_NONE = 0,
	EYE_RIGHT = 1
};

typedef struct {
	float viewOffset;
	float projOffset;
	float viewFovY;
	float viewFovX;
	float scale;
	float pixelScale;
	unsigned int stale;
	unsigned int viewHeight;
	unsigned int viewWidth;
	unsigned int vrWidth;
	unsigned int vrHalfWidth;
	unsigned int vrHeight;
	unsigned int hudWidth;
	unsigned int hudHeight;
	int eye;
} vr_param_t;

// struct for things that may change from frame to frame
extern vr_param_t vrState;

typedef struct {
	unsigned int xPos;
	unsigned int yPos;
	unsigned int hmdHeight;
	unsigned int hmdWidth;
	float ipd;
	float dk[4];
	float chrm[4];
	float aspect;
	float dist_scale;
	char deviceName[32];
} vr_attrib_t;

// struct for things that should never change after init
extern vr_attrib_t vrConfig;

extern cvar_t *vr_enabled;
extern cvar_t *vr_autoenable;
extern cvar_t *vr_ipd;
extern cvar_t *vr_ovr_scale;
extern cvar_t *vr_ovr_chromatic;
extern cvar_t *vr_hud_fov;
extern cvar_t *vr_hud_depth;
extern cvar_t *vr_hud_transparency;
extern cvar_t *vr_crosshair;
extern cvar_t *vr_crosshair_size;
extern cvar_t *vr_crosshair_brightness;
extern cvar_t *vr_aimmode;
extern cvar_t *vr_aimmode_deadzone;
extern cvar_t *vr_viewmove;
extern cvar_t *vr_hud_bounce;
extern cvar_t *vr_hud_bounce_falloff;

enum {
	VR_AIMMODE_DISABLE,
	VR_AIMMODE_HEAD_MYAW,
	VR_AIMMODE_HEAD_MYAW_MPITCH,
	VR_AIMMODE_MOUSE_MYAW,
	VR_AIMMODE_MOUSE_MYAW_MPITCH,
	VR_AIMMODE_BLENDED_FIXPITCH,
	VR_AIMMODE_BLENDED,
	NUM_VR_AIMMODE
} vr_aimmode_t;



void VR_Init();
void VR_Shutdown();
int VR_Enable();
void VR_Disable();
void VR_Frame();
void VR_OVR_SetFOV();
void VR_GetRenderParam(vr_param_t *settings);
void VR_GetOrientation(vec3_t angle);
void VR_GetOrientationDelta(vec3_t angle);
void VR_GetOrientationEMA(vec3_t angle);
void VR_ResetOrientation();

#endif
#ifndef __R_VR_H
#define __R_VR_H
#include "../qcommon/qcommon.h"

enum vr_eye_t {
	EYE_LEFT = -1,
	EYE_NONE = 0,
	EYE_RIGHT = 1
};

typedef struct {
	qboolean available;
	float viewOffset;
	float projOffset;
	float viewFovY;
	float viewFovX;
	float scale;
	float pixelScale;
	unsigned int viewHeight;
	unsigned int viewWidth;
	unsigned int vrWidth;
	unsigned int vrHalfWidth;
	unsigned int vrHeight;
	unsigned int hudWidth;
	unsigned int hudHeight;
	int eye;
} vr_param_t;

extern vr_param_t vrState;

typedef struct {
	unsigned int hmdHeight;
	unsigned int hmdWidth;
	float ipd;
	float dk[4];
	float chrm[4];
	float aspect;
} vr_attrib_t;

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

void VR_Init();
void VR_Shutdown();
void VR_Enable();
void VR_Disable();
void VR_Frame();
void VR_Set_OVRDistortion_Scale(float dist_scale);
void VR_GetRenderParam(vr_param_t *settings);
void VR_GetSensorOrientation(vec3_t angle);
void VR_GetSensorOrientationDelta(vec3_t angle);
void VR_ResetOrientation();

#endif
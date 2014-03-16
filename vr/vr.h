#ifndef __VR_H
#define __VR_H
#include "../qcommon/qcommon.h"

#define PLAYER_HEIGHT_UNITS 56.0f
#define PLAYER_HEIGHT_M 1.75f

typedef enum {
	EYE_LEFT = -1,
	EYE_HUD = 0,
	EYE_RIGHT = 1
} vr_eye_t;

typedef struct {
	float projOffset;
	float viewFovY;
	float viewFovX;
	float ipd;
	float pixelScale;
	float aspect;
} vr_param_t;

// struct for things that may change from frame to frame
extern vr_param_t vrState;

// prefer OVR native code path
typedef enum {
	HMD_NONE,
	HMD_RIFT,
	HMD_STEAM,
	NUM_HMD_TYPES
} hmd_t;

typedef struct {
	hmd_t type;
	Sint32 (*init)();
	void (*shutdown)();
	Sint32 (*enable)();
	void (*disable)();
	void (*getHMDPos)(Sint32 *xpos, Sint32 *ypos);
	void (*frame)();
	void (*resetOrientation)();
	Sint32 (*getOrientation)(float euler[3]);
	Sint32 (*getHeadOffset)(float offset[3]);
	Sint32 (*setPrediction)(float timeInMs);
} hmd_interface_t;


extern cvar_t *vr_enabled;
extern cvar_t *vr_autoenable;
extern cvar_t *vr_autofov;
extern cvar_t *vr_autofov_scale;
extern cvar_t *vr_autoipd;
extern cvar_t *vr_ipd;
extern cvar_t *vr_hud_fov;
extern cvar_t *vr_hud_depth;
extern cvar_t *vr_hud_transparency;
extern cvar_t *vr_chromatic;
extern cvar_t *vr_crosshair;
extern cvar_t *vr_crosshair_size;
extern cvar_t *vr_crosshair_brightness;
extern cvar_t *vr_aimmode;
extern cvar_t *vr_aimmode_deadzone_yaw;
extern cvar_t *vr_aimmode_deadzone_pitch;
extern cvar_t *vr_viewmove;
extern cvar_t *vr_hud_bounce;
extern cvar_t *vr_hud_bounce_falloff;
extern cvar_t *vr_nosleep;
extern cvar_t *vr_neckmodel;
extern cvar_t *vr_neckmodel_up;
extern cvar_t *vr_neckmodel_forward;

enum {
	VR_AIMMODE_DISABLE,
	VR_AIMMODE_HEAD_MYAW,
	VR_AIMMODE_HEAD_MYAW_MPITCH,
	VR_AIMMODE_MOUSE_MYAW,
	VR_AIMMODE_MOUSE_MYAW_MPITCH,
	VR_AIMMODE_TF2_MODE2,
	VR_AIMMODE_TF2_MODE3,
	VR_AIMMODE_TF2_MODE4,
	VR_AIMMODE_DECOUPLED,
	NUM_VR_AIMMODE
} vr_aimmode_t;

enum {
	VR_CROSSHAIR_NONE,
	VR_CROSSHAIR_DOT,
	VR_CROSSHAIR_LASER,
	NUM_VR_CROSSHAIR
} vr_crosshair_t;

enum {
	VR_HUD_BOUNCE_NONE,
	VR_HUD_BOUNCE_SES,
	VR_HUD_BOUNCE_LES,
	NUM_HUD_BOUNCE_MODES
} vr_bounce_mode_t;

void VR_Startup();
void VR_Teardown();
Sint32 VR_Enable();
void VR_Disable();
void VR_Frame();
void VR_GetRenderParam(vr_param_t *settings);
void VR_GetOrientation(vec3_t angle);
void VR_GetOrientationDelta(vec3_t angle);
void VR_GetOrientationEMA(vec3_t angle);
void VR_GetOrientationEMAQuat(vec3_t quat);
Sint32 VR_GetHeadOffset(vec3_t offset);
void VR_ResetOrientation();
void VR_GetHMDPos(Sint32 *xpos, Sint32 *ypos);

#endif
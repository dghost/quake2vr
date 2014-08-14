#ifndef __VR_H
#define __VR_H
#include "../../../qcommon/qcommon.h"

#define PLAYER_HEIGHT_UNITS 56.0f
#define PLAYER_HEIGHT_M 1.75f

typedef enum {
	EYE_LEFT = 0,
	EYE_RIGHT = 1,
	NUM_EYES = 2,
} vr_eye_t;

typedef struct {
	vr_eye_t eyes[NUM_EYES];
} eye_order_t;

// prefer OVR native code path
typedef enum {
	HMD_NONE,
	HMD_RIFT,
	HMD_STEAM,
	NUM_HMD_TYPES
} hmd_t;

extern const char *hmd_names[];

typedef struct {
	hmd_t type;
	int32_t (*init)();
	void (*shutdown)();
	int32_t (*enable)();
	void (*disable)();
	void (*getHMDPos)(int32_t *xpos, int32_t *ypos);
	void (*frameStart)();
	void (*frameEnd)();
	void (*resetOrientation)();
	int32_t (*getOrientation)(float euler[3]);
	int32_t (*getHeadOffset)(float offset[3]);
	int32_t (*setPrediction)(float timeInMs);
} hmd_interface_t;


extern cvar_t *vr_enabled;
extern cvar_t *vr_autoenable;
extern cvar_t *vr_autofov;
extern cvar_t *vr_autofov_scale;
extern cvar_t *vr_autoipd;
extern cvar_t *vr_ipd;
extern cvar_t *vr_hud_fov;
extern cvar_t *vr_hud_depth;
extern cvar_t *vr_hud_segments;
extern cvar_t *vr_hud_transparency;
extern cvar_t *vr_hud_width;
extern cvar_t *vr_hud_height;
extern cvar_t *vr_chromatic;
extern cvar_t *vr_aimlaser;
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
extern cvar_t *vr_walkspeed;
extern cvar_t *vr_force_fullscreen;

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
	VR_HUD_BOUNCE_NONE,
	VR_HUD_BOUNCE_SES,
	VR_HUD_BOUNCE_LES,
	NUM_HUD_BOUNCE_MODES
} vr_bounce_mode_t;

void VR_Startup();
void VR_Teardown();
int32_t VR_Enable();
void VR_Disable();
void VR_FrameStart();
void VR_FrameEnd();
void VR_GetOrientation(vec3_t angle);
void VR_GetOrientationDelta(vec3_t angle);
void VR_GetOrientationEMA(vec3_t angle);
void VR_GetOrientationEMAQuat(vec3_t quat);
int32_t VR_GetHeadOffset(vec3_t offset);
void VR_ResetOrientation();
void VR_GetHMDPos(int32_t *xpos, int32_t *ypos);

#endif
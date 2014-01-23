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
	float viewOffset;
	float projOffset;
	float viewFovY;
	float viewFovX;
	float scale;
	float pixelScale;
	unsigned int stale;
	unsigned int viewHeight;
	unsigned int viewWidth;
	unsigned int scaledViewHeight;
	unsigned int scaledViewWidth;
	unsigned int hudWidth;
	unsigned int hudHeight;
	vr_eye_t eye;
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
	float minScale;
	float maxScale;
	char deviceName[32];
} vr_attrib_t;

// struct for things that should never change after init
extern vr_attrib_t vrConfig;

typedef enum {
	HMD_NONE,
	HMD_RIFT,
	NUM_HMD_TYPES
} hmd_t;

typedef struct {
	hmd_t type;
	int (*init)();
	void (*shutdown)();
	int (*attached)();
	int (*enable)();
	void (*disable)();
	void (*setfov)();
	void (*frame)();
	void (*resetOrientation)();
	int (*getOrientation)(float euler[3]);
	int (*getHeadOffset)(float offset[3]);
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
extern cvar_t *vr_antialias;

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
	VR_ANTIALIAS_NONE,
	VR_ANTIALIAS_4X_FSAA,
	NUM_VR_ANTIALIAS
} vr_antialias_t;

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
void VR_GetOrientationEMAQuat(vec3_t quat);
int VR_GetHeadOffset(vec3_t offset);
void VR_ResetOrientation();

#endif
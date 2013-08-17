#ifndef __R_VR_H
#define __R_VR_H
#include "../qcommon/qcommon.h"

enum vr_eye_t {
	EYE_LEFT = -1,
	EYE_NONE = 0,
	EYE_RIGHT = 1
};

typedef struct {
	qboolean enabled;
	float viewOffset;
	float projOffset;
	float ipd;
	float viewAspect;
	float viewFovY;
	float viewFovX;
	float scale;
	float dk[4];
	float chrm[4];
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

void VR_Init();
void VR_Shutdown();
void VR_Frame();
void VR_Set_OVRDistortion_Scale(float dist_scale);
void VR_GetRenderParam(vr_param_t *settings);
void VR_GetSensorOrientation(vec3_t angle);
void VR_GetSensorOrientationDelta(vec3_t angle);
void VR_ResetOrientation();

#endif
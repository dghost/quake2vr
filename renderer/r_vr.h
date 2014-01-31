#ifndef __R_VR_H
#define __R_VR_H

#include "../vr/vr.h"

typedef struct {
	int x, y;
	int width, height;
} vr_rect_t;


typedef struct {
	hmd_t type;
	int (*init)();
	int (*enable)();
	void (*disable)();
	void (*bindView)(vr_eye_t eye);
	void (*getViewRect)(vr_eye_t eye, vr_rect_t *rect);
	void (*frameStart)(int changeBackBuffers);
	void (*getState)(vr_param_t *state);
	void (*present)();
} hmd_render_t;

void R_VR_Init();
void R_VR_Shutdown();
void R_VR_Enable();
void R_VR_Disable();
void R_VR_StartFrame();
void R_VR_BindView(vr_eye_t eye);
void R_VR_GetViewPos(vr_eye_t eye, int *x, int *y);
void R_VR_GetViewSize(vr_eye_t eye, int *width, int *height);
void R_VR_GetViewRect(vr_eye_t eye, vr_rect_t *rect);
void R_VR_Rebind();
void R_VR_CurrentViewPosition(int *x, int *y);
void R_VR_CurrentViewSize(int *width, int *height);
void R_VR_CurrentViewRect(vr_rect_t *rect);
void R_VR_Present();
void R_VR_EndFrame();
void R_VR_DrawHud(vr_eye_t eye);
void R_VR_Perspective(double fovy, double aspect, double zNear, double zFar);
void R_VR_GetFOV(float *fovx, float *fovy);
#endif //__R_VR_H
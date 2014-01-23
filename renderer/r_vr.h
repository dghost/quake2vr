#ifndef __R_VR_H
#define __R_VR_H

#include "../vr/vr.h"

typedef struct {
	hmd_t type;
	int (*init)();
	int (*enable)();
	void (*disable)();
	void (*bindView)(vr_eye_t eye);
	void (*getViewPos)(vr_eye_t eye, unsigned int pos[2]);
	void (*getViewSize)(vr_eye_t eye, unsigned int size[2]);
	void (*frameStart)(int changeBackBuffers);
	void (*present)();
} hmd_render_t;


void R_VR_Init();
void R_VR_Shutdown();
void R_VR_Enable();
void R_VR_Disable();
void R_VR_StartFrame();
void R_VR_BindView(vr_eye_t eye);
void R_VR_GetViewPos(vr_eye_t eye, unsigned int pos[2]);
void R_VR_GetViewSize(vr_eye_t eye, unsigned int size[2]);
void R_VR_Rebind();
void R_VR_Present();
void R_VR_EndFrame();
void R_VR_DrawHud(vr_eye_t eye);

#endif //__R_VR_H
#ifndef __R_VR_OVR_H
#define __R_VR_OVR_H

#include "r_vr.h"
#include "r_local.h"

typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint scale;
		GLuint scale_in;
		GLuint lens_center;
		GLuint screen_center;
		GLuint texture_size;
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
	} uniform;

} r_ovr_shader_t;

void R_VR_OVR_FrameStart(int changeBackBuffers);
void R_VR_OVR_BindView(vr_eye_t eye);
void R_VR_OVR_Present();
int R_VR_OVR_Enable();
void R_VR_OVR_Disable();
int R_VR_OVR_Init();

extern hmd_render_t vr_render_ovr;

#endif //__R_VR_OVR_H
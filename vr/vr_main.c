#include "vr.h"

#include "vr_ovr.h"


cvar_t *vr_enabled;
int vr_ovr_detected = false;

vr_param_t vrState = {false, 0, 0, 0, 0, 0};
vr_ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0,{ 0, 0, 0, 0,},  { 0, 0, 0, 0,}, ""};
vr_param_t vr_render_param;


void VR_Set_OVR_Distortion_Scale(float dist_scale)
{
	if (vr_ovr_settings.initialized)
	{
		float aspect = vr_ovr_settings.h_resolution / (2.0f * vr_ovr_settings.v_resolution);
		float fovy = 2 * atan2(vr_ovr_settings.v_screen_size * dist_scale, 2 * vr_ovr_settings.eye_to_screen_distance);
		float viewport_fov_y = fovy * 180 / M_PI;
		float viewport_fov_x = viewport_fov_y * aspect;
		vrState.viewAspect = aspect;
		vrState.viewFovY = viewport_fov_y;
		vrState.viewFovX = viewport_fov_x;
		vrState.scale = dist_scale;
	}

}
void VR_CalculateOVRRenderParam()
{
	if (vr_ovr_settings.initialized)
	{
		float *dk = vr_ovr_settings.distortion_k;

		float h = 1.0f - (2.0f * vr_ovr_settings.lens_separation_distance) / vr_ovr_settings.h_screen_size;
		float r = -1.0f - h;
		float dist_scale = (dk[0] + dk[1] * pow(r,2) + dk[2] * pow(r,4) + dk[3] * pow(r,6));
		VR_Set_OVR_Distortion_Scale(dist_scale);
		memcpy(vrState.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(vrState.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		vrState.projOffset = h;
		vrState.ipd = vr_ovr_settings.interpupillary_distance;
		Com_Printf("VR_OVR: Calculated %.2f FOV\n",vrState.viewFovY);
	}
}
void VR_Init()
{
	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_ovr_detected = VR_OVR_Init();
	if (vr_ovr_detected)
	{
		if (VR_OVR_GetSettings(&vr_ovr_settings))
		{

			Com_Printf("VR_OVR: Detected HMD %s\n",vr_ovr_settings.devName);
			Com_Printf("VR_OVR: Detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);
			VR_CalculateOVRRenderParam();
		}
		Cvar_ForceSet("vr_enabled","1");

	}
}

void VR_Shutdown()
{
	Cvar_SetInteger("vr_enabled",0);
	VR_OVR_Shutdown();
}
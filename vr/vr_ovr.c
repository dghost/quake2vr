#include "vr.h"
#include "vr_ovr.h"
#include "vr_libovr.h"

cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_chromatic;


ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}, "", ""};

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_OVR_Init,
	LibOVR_Shutdown,
	LibOVR_IsDeviceAvailable,
	VR_OVR_Enable,
	LibOVR_DeviceRelease,
	VR_OVR_SetFOV,
	VR_OVR_Frame,
	LibOVR_GetOrientation,
	LibOVR_ResetHMDOrientation,
	LibOVR_SetPredictionTime
};

void VR_OVR_SetFOV()
{
	if (vr_ovr_settings.initialized)
	{
		float fovy = 2 * atan2(vr_ovr_settings.v_screen_size * vr_ovr_scale->value, 2 * vr_ovr_settings.eye_to_screen_distance);
		float viewport_fov_y = fovy * 180 / M_PI * vr_fov_scale->value;
		float viewport_fov_x = viewport_fov_y * vrConfig.aspect;
		vrState.viewFovY = viewport_fov_y;
		vrState.viewFovX = viewport_fov_x;
	}
}

void VR_OVR_CalcRenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float h = 1.0f - (2.0f * vr_ovr_settings.lens_separation_distance) / vr_ovr_settings.h_screen_size;
		float r = -1.0f - h;

		vrConfig.dist_scale = (dk[0] + dk[1] * pow(r,2) + dk[2] * pow(r,4) + dk[3] * pow(r,6));
		vrConfig.ipd = vr_ovr_settings.interpupillary_distance;
		vrConfig.aspect = vr_ovr_settings.h_resolution / (2.0f * vr_ovr_settings.v_resolution);
		vrConfig.hmdWidth = vr_ovr_settings.h_resolution;
		vrConfig.hmdHeight = vr_ovr_settings.v_resolution;
		vrConfig.xPos = vr_ovr_settings.xPos;
		vrConfig.yPos = vr_ovr_settings.yPos;
		strncpy(vrConfig.deviceName,vr_ovr_settings.deviceName,31);
		memcpy(vrConfig.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(vrConfig.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		vrState.projOffset = h;

	}
}


void VR_OVR_Frame()
{

	if (vr_ovr_driftcorrection->modified)
	{
		if (vr_ovr_driftcorrection->value)
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",1);
			LibOVR_DeviceInitMagneticCorrection();
		}
		else
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",0);
			LibOVR_DisableMagneticCorrection();
		}
		vr_ovr_driftcorrection->modified = false;
	}

}

int VR_OVR_Enable()
{
	char string[6];

	if (!LibOVR_IsDeviceAvailable())
	{
		Com_Printf("VR_OVR: Error, no HMD detected!\n");
		return 0;
	}
	Com_Printf("VR_OVR: Initializing HMD:");

	// try to initialize LibOVR
	if (LibOVR_DeviceInit())
	{
		Com_Printf(" ok!\n");
	}else {
		Com_Printf(" failed!\n");
		return 0;
	}

	Com_Printf("VR_OVR: Getting HMD settings:");
	
	// get info from LibOVR
	if (LibOVR_GetSettings(&vr_ovr_settings))
	{
		
		Com_Printf(" ok!\n");


		Com_Printf("...detected HMD %s\n",vr_ovr_settings.deviceString);
		Com_Printf("...detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);

		VR_OVR_CalcRenderParam();

		strncpy(string, va("%.2f",vrConfig.ipd * 1000), sizeof(string));
		vr_ipd = Cvar_Get("vr_ipd",string, CVAR_ARCHIVE);


		if (vr_ipd->value < 0)
			Cvar_SetValue("vr_ipd",vrConfig.ipd * 1000);
		strncpy(string, va("%.2f",vrConfig.dist_scale), sizeof(string));
		vr_ovr_scale = Cvar_Get("vr_ovr_scale",string,CVAR_ARCHIVE);
		if (vr_ovr_scale->value < 0)
		{
			Cvar_Set("vr_ovr_scale",string);
		}

		VR_OVR_SetFOV();

		if (vr_ovr_driftcorrection->value > 0.0)
			LibOVR_DeviceInitMagneticCorrection();

		// TODO: use pixel density instead
		if (vr_ovr_settings.v_resolution > 800)
			Cvar_SetInteger("vr_hud_transparency",1);

		Com_Printf("...calculated %.2f FOV\n",vrState.viewFovY);
		Com_Printf("...calculated %.2f distortion scale\n", vr_ovr_scale->value);
	} else 
	{
		Com_Printf(" failed!\n");
		return 0;
	}


	return 1;
}

int VR_OVR_Init()
{
	if (!LibOVR_Init())
	{
		Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
		return 0;
	}

	vr_ovr_scale = Cvar_Get("vr_ovr_scale","-1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);
	Com_Printf("VR_OVR: Oculus Rift support initialized...\n");
	return 1;
}

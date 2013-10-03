#include "vr.h"
#include "vr_ovr.h"
#include "vr_libovr.h"

cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_chromatic;
cvar_t *vr_ovr_debug;

static qboolean debug_init = false;

ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}, "", ""};

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_OVR_Init,
	VR_OVR_Shutdown,
	VR_OVR_isDeviceAvailable,
	VR_OVR_Enable,
	VR_OVR_Disable,
	VR_OVR_SetFOV,
	VR_OVR_Frame,
	VR_OVR_getOrientation,
	VR_OVR_ResetHMDOrientation,
	VR_OVR_SetPredictionTime
};

void VR_OVR_SetFOV()
{
	if (vr_ovr_settings.initialized)
	{
		float scale = (vr_ovr_scale->value ? vr_ovr_scale->value : vrConfig.dist_scale);
		float fovy = 2 * atan2(vr_ovr_settings.v_screen_size * scale, 2 * vr_ovr_settings.eye_to_screen_distance);
		float viewport_fov_y = fovy * 180 / M_PI * vr_fov_scale->value;
		float viewport_fov_x = viewport_fov_y * vrConfig.aspect;
		vrState.viewFovY = viewport_fov_y;
		vrState.viewFovX = viewport_fov_x;
	}
}

int VR_OVR_SetPredictionTime(float time)
{
	int result = LibOVR_SetPredictionTime(time);
	if (!debug_init)
		return result;
	else
		return 1;
}

int VR_OVR_getOrientation(float euler[3])
{
	int result = LibOVR_GetOrientation(euler);
	if (result)
		return 1;
	else if (debug_init)
	{
		VectorSet(euler, 0, 0, 0);
		return 1;
	}
	else
		return 0;
}

void VR_OVR_ResetHMDOrientation()
{
	LibOVR_ResetHMDOrientation();
}

void VR_OVR_CalcRenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float h = 1.0f - (2.0f * vr_ovr_settings.lens_separation_distance) / vr_ovr_settings.h_screen_size;
		float r = -1.0f - h;
		float rsq = r * r;
		vrConfig.dist_scale = (dk[0] + dk[1] * rsq + dk[2] * rsq * rsq + dk[3] * rsq * rsq * rsq);
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
	if (debug_init)
		return;
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
int VR_OVR_GetSettings(ovr_settings_t *settings)
{
	int result = LibOVR_GetSettings(settings);
	Com_Printf("VR_OVR: Getting HMD settings:");
	if (result)
	{
		Com_Printf(" ok!\n");
		return 1;
	}

	Com_Printf(" failed!\n");
	if (debug_init)
	{
		float distk[4] = { 1.0f, 0.18f, 0.115f, 0.0f };
		float chrm[4] = { 0.996f, -0.004f, 1.014f, 0.0f };
		Com_Printf("VR_OVR: Falling back to debug parameters...\n");
		settings->initialized = 1;
		settings->h_resolution = 1920;
		settings->v_resolution = 1080;
		settings->h_screen_size = 0.12096f;
		settings->v_screen_size = 0.0756f;
		settings->xPos = 0;
		settings->yPos = 0;
		settings->interpupillary_distance = 0.064f;
		settings->lens_separation_distance = 0.0635f;
		settings->eye_to_screen_distance = 0.040f;
		memcpy(settings->distortion_k, distk, sizeof(float) * 4);
		memcpy(settings->chrom_abr, chrm, sizeof(float) * 4);
		strncpy(settings->deviceString, "Oculus Rift DK HD Emulator", 31);
		strncpy(settings->deviceName, "", 31);
		return 1;
	}

		return 0;
}


int VR_OVR_isDeviceAvailable()
{
	if (!debug_init)
		return LibOVR_IsDeviceAvailable();
	else
		return 1;
}

int VR_OVR_Enable()
{
	char string[6];
	int failure = 0;
	if (!LibOVR_IsDeviceAvailable())
	{
		Com_Printf("VR_OVR: Error, no HMD detected!\n");
		failure = 1;
	}
	else {

		Com_Printf("VR_OVR: Initializing HMD:");

		// try to initialize LibOVR
		if (LibOVR_DeviceInit())
		{
			Com_Printf(" ok!\n");
		}
		else {
			Com_Printf(" failed!\n");
			failure = 1;
		}
	}

	if (failure && !debug_init)
		return 0;


	// get info from LibOVR

	if (!VR_OVR_GetSettings(&vr_ovr_settings))
		return 0;

	Com_Printf("...detected HMD %s\n", vr_ovr_settings.deviceString);
	Com_Printf("...detected IPD %.1fmm\n", vr_ovr_settings.interpupillary_distance * 1000);

	VR_OVR_CalcRenderParam();

	strncpy(string, va("%.2f", vrConfig.ipd * 1000), sizeof(string));
	vr_ipd = Cvar_Get("vr_ipd", string, CVAR_ARCHIVE);


	if (vr_ipd->value < 0)
		Cvar_SetValue("vr_ipd", vrConfig.ipd * 1000);

	VR_OVR_SetFOV();

	if (vr_ovr_driftcorrection->value > 0.0)
		LibOVR_DeviceInitMagneticCorrection();

	/* 8/30/2013 - never auto enable this.
	// TODO: use pixel density instead
	if (vr_ovr_settings.v_resolution > 800)
		Cvar_SetInteger("vr_hud_transparency", 1);
	*/
	Com_Printf("...calculated %.2f FOV\n", vrState.viewFovY);
	Com_Printf("...calculated %.2f distortion scale\n", vrConfig.dist_scale);

	return 1;
}

void VR_OVR_Disable()
{
	LibOVR_DeviceRelease();
}


int VR_OVR_Init()
{
	qboolean init = LibOVR_Init();
	debug_init = false;

	vr_ovr_scale = Cvar_Get("vr_ovr_scale","0",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_NOSET);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);

	if (!init)
	{
		Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
		return 0;
	}

	if (vr_ovr_debug->value)
	{
		debug_init = true;
		Com_Printf("VR_OVR: Enabling debug fallback...\n");

	}
	Com_Printf("VR_OVR: Oculus Rift support initialized...\n");

	return 1;
}

void VR_OVR_Shutdown()
{
	LibOVR_Shutdown();
}

#include "include/vr.h"
#include "include/vr_ovr.h"
#include "include/vr_libovr.h"


void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos);
void VR_OVR_GetState(vr_param_t *state);
void VR_OVR_Frame();
int32_t VR_OVR_Enable();
void VR_OVR_Disable();
int32_t VR_OVR_Init();
void VR_OVR_Shutdown();
int32_t VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
int32_t VR_OVR_SetPredictionTime(float time);

void VR_OVR_CalcRenderParam();
float VR_OVR_GetDistortionScale();

cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_debug;
cvar_t *vr_ovr_distortion;
cvar_t *vr_ovr_lensdistance;
cvar_t *vr_ovr_autoscale;
cvar_t *vr_ovr_autolensdistance;
cvar_t *vr_ovr_filtermode;
cvar_t *vr_ovr_supersample;
cvar_t *vr_ovr_latencytest;
cvar_t *vr_ovr_enable;

static ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}, "", ""};


ovr_attrib_t ovrConfig = {0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}};

vr_param_t ovrState = {0, 0, 0, 0, 0, 0};


hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_OVR_Init,
	VR_OVR_Shutdown,
	VR_OVR_Enable,
	VR_OVR_Disable,
	VR_OVR_GetHMDPos,
	VR_OVR_Frame,
	VR_OVR_ResetHMDOrientation,
	VR_OVR_getOrientation,
	NULL,
	VR_OVR_SetPredictionTime
};


void VR_OVR_GetFOV(float *fovx, float *fovy)
{
	if (vr_ovr_settings.initialized)
	{
		float scale = VR_OVR_GetDistortionScale();
		float yfov = 2 * atan2(vr_ovr_settings.v_screen_size * scale, 2 * vr_ovr_settings.eye_to_screen_distance) * 180/ M_PI;
		*fovy = yfov;
		*fovx = yfov * ovrConfig.aspect;
	}
}


void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (vr_ovr_settings.initialized)
	{
		*xpos = vr_ovr_settings.xPos;
		*ypos = vr_ovr_settings.yPos;
	}
}


int32_t VR_OVR_getOrientation(float euler[3])
{
	if (vr_ovr_latencytest->value)
		LibOVR_ProcessLatencyInputs();
    return LibOVR_GetOrientation(euler);
}

void VR_OVR_ResetHMDOrientation()
{
	LibOVR_ResetHMDOrientation();
}

int32_t VR_OVR_RenderLatencyTest(vec4_t color) 
{
	return (vr_ovr_latencytest->value && LibOVR_GetLatencyTestColor(color));
}

void VR_OVR_CalcRenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float lsd = vr_ovr_autolensdistance->value ? vr_ovr_settings.lens_separation_distance : vr_ovr_lensdistance->value / 1000.0;
		float h = 1.0f - (2.0f * lsd) / vr_ovr_settings.h_screen_size;

		if (lsd > 0)
		{
			float le = (-1.0f - h) * (-1.0f - h);
			float re = (-1.0f + h) * (-1.0f + h);

			if (le > re)
			{
				ovrConfig.minScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
				ovrConfig.maxScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
			} else
			{
				ovrConfig.maxScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
				ovrConfig.minScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
			}
		} else {
			ovrConfig.maxScale = 2.0;
			ovrConfig.minScale = 1.0;
		}

		if (ovrConfig.maxScale >= 2.0)
			ovrConfig.maxScale = 2.0;

		ovrConfig.ipd = vr_ovr_settings.interpupillary_distance;
		ovrConfig.aspect = vr_ovr_settings.h_resolution / (2.0f * vr_ovr_settings.v_resolution);
		ovrConfig.hmdWidth = vr_ovr_settings.h_resolution;
		ovrConfig.hmdHeight = vr_ovr_settings.v_resolution;

		memcpy(ovrConfig.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(ovrConfig.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		strncpy(ovrConfig.deviceString,vr_ovr_settings.deviceString,sizeof(ovrConfig.deviceString));
		ovrConfig.projOffset = h;
	}
}


float VR_OVR_GetDistortionScale()
{
	if (vr_ovr_distortion->value < 0)
		return 1.0f;

	switch ((int32_t) vr_ovr_autoscale->value)
	{
	case 1:
		return ovrConfig.minScale;
	case 2:
		return ovrConfig.maxScale;
	default:
		return vr_ovr_scale->value;
	}
}

int32_t VR_OVR_SetPredictionTime(float time)
{
	int32_t result = !!LibOVR_SetPredictionTime(time);
	if (result)
		Com_Printf("VR_OVR: Set HMD Prediction time to %.1fms\n", time);
	return result;
}

void SCR_CenterAlert (char *str);
void VR_OVR_Frame()
{

	if (vr_ovr_driftcorrection->modified)
	{
		if (vr_ovr_driftcorrection->value)
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",1);
			if (!LibOVR_EnableDriftCorrection())
			{
				Com_Printf("VR_OVR: Cannot enable magnetic drift correction - device is not calibrated\n");
			}
		}
		else
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",0);
			LibOVR_DisableDriftCorrection();
		}
		vr_ovr_driftcorrection->modified = false;
	}

	if (vr_ovr_latencytest->value && LibOVR_IsLatencyTesterAvailable())
	{
			const char *results = LibOVR_ProcessLatencyResults();
			if (results)
			{
				float ms;
				Com_Printf("VR_OVR: %s\n",results);		
				if (sscanf(results,"RESULT=%f ",&ms))
				{
					char buffer[64];
					SDL_snprintf(buffer,sizeof(buffer),"%.1fms latency",ms);
					SCR_CenterAlert(buffer);
				}
			}
	} 
}

int32_t VR_OVR_GetSettings(ovr_settings_t *settings)
{
	int32_t result = LibOVR_GetSettings(settings);
	Com_Printf("VR_OVR: Getting HMD settings:");
	if (result)
	{
		Com_Printf(" ok!\n");
		return 1;
	}

	Com_Printf(" failed!\n");
	if (vr_ovr_debug->value)
	{
		int32_t riftType = (int32_t) vr_ovr_debug->value;
		Com_Printf("VR_OVR: Falling back to debug parameters...\n");

		switch (riftType)
		{
		case RIFT_DK1:
			{
				float distk[4] = { 1.0f, 0.22f, 0.24f, 0.0f };
				float chrm[4] = { 0.996f, -0.004f, 1.014f, 0.0f };
				settings->initialized = 1;
				settings->xPos = 0;
				settings->yPos = 0;
				settings->h_resolution = 1280;
				settings->v_resolution = 800;
				settings->h_screen_size = 0.14976f;
				settings->v_screen_size = 0.0936f;
				settings->interpupillary_distance = 0.064f;
				settings->lens_separation_distance = 0.0635f;
				settings->eye_to_screen_distance = 0.041f;
				memcpy(settings->distortion_k, distk, sizeof(float) * 4);
				memcpy(settings->chrom_abr, chrm, sizeof(float) * 4);
				strncpy(settings->deviceString, "Oculus Rift DK1 Emulator", 31);
			}
			break;
		default:
		case RIFT_DKHD:
			{
				float distk[4] = { 1.0f, 0.18f, 0.115f, 0.0f };
				float chrm[4] = { 0.996f, -0.004f, 1.014f, 0.0f };
				settings->initialized = 1;
				settings->xPos = 0;
				settings->yPos = 0;
				settings->h_resolution = 1920;
				settings->v_resolution = 1080;
				settings->h_screen_size = 0.12096f;
				settings->v_screen_size = 0.06804f;
				settings->interpupillary_distance = 0.064f;
				settings->lens_separation_distance = 0.0635f;
				settings->eye_to_screen_distance = 0.040f;
				memcpy(settings->distortion_k, distk, sizeof(float) * 4);
				memcpy(settings->chrom_abr, chrm, sizeof(float) * 4);
				strncpy(settings->deviceString, "Oculus Rift DK HD Emulator", 31);
			}
			break;
		}
		return 1;
	}
	return 0;
}

int32_t VR_OVR_Enable()
{
	char string[6];
	int32_t failure = 0;
	if (!vr_ovr_enable->value)
		return 0;
	if (!LibOVR_DeviceInit())
	{
		Com_Printf("VR_OVR: Error, no HMD detected!\n");
		failure = 1;
	}
	else {
		Com_Printf("VR_OVR: Initializing HMD:");
		Com_Printf(" ok!\n");
	}

	if (failure && !vr_ovr_debug->value)
		return 0;

	// get info from LibOVR
	if (!VR_OVR_GetSettings(&vr_ovr_settings))
		return 0;

	Com_Printf("...detected HMD %s with %ux%u resolution\n", vr_ovr_settings.deviceString, vr_ovr_settings.h_resolution, vr_ovr_settings.v_resolution);
	Com_Printf("...detected IPD %.1fmm\n", vr_ovr_settings.interpupillary_distance * 1000);
	strncpy(string, va("%.2f", vr_ovr_settings.lens_separation_distance * 1000), sizeof(string));
	vr_ovr_lensdistance = Cvar_Get("vr_ovr_lensdistance", string, CVAR_ARCHIVE);
	
	if (vr_ovr_scale->value < 1)
		Cvar_SetValue("vr_ovr_scale", 1.0);

	VR_OVR_CalcRenderParam();

	if (vr_ovr_driftcorrection->value > 0.0)
		if (!LibOVR_EnableDriftCorrection())
				Com_Printf("VR_OVR: Cannot enable magnetic drift correction - device is not calibrated\n");

	/* 8/30/2013 - never auto enable this.
	// TODO: use pixel density instead
	if (vr_ovr_settings.v_resolution > 800)
		Cvar_SetInteger("vr_hud_transparency", 1);
	*/
	Com_Printf("...calculated %.2f minimum distortion scale\n", ovrConfig.minScale);
	Com_Printf("...calculated %.2f maximum distortion scale\n", ovrConfig.maxScale);

	if (LibOVR_IsLatencyTesterAvailable())
	{
		Com_Printf("...found latency tester\n");
		Cvar_SetInteger("vr_ovr_latencytest",1);
	}
	
	return 1;
}

void VR_OVR_Disable()
{
	LibOVR_DeviceRelease();
}


int32_t VR_OVR_Init()
{
	int32_t init = LibOVR_Init();
	vr_ovr_supersample = Cvar_Get("vr_ovr_supersample","1.0",CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","1.0",CVAR_ARCHIVE);
	vr_ovr_lensdistance = Cvar_Get("vr_ovr_lensdistance","-1",CVAR_ARCHIVE);
	vr_ovr_latencytest = Cvar_Get("vr_ovr_latencytest","0",0);
	vr_ovr_enable = Cvar_Get("vr_ovr_enable","1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","1",CVAR_ARCHIVE);
	vr_ovr_distortion = Cvar_Get("vr_ovr_distortion","1",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_ARCHIVE);
	vr_ovr_filtermode = Cvar_Get("vr_ovr_filtermode","0",CVAR_ARCHIVE);
	vr_ovr_autoscale = Cvar_Get("vr_ovr_autoscale","1",CVAR_ARCHIVE);
	vr_ovr_autolensdistance = Cvar_Get("vr_ovr_autolensdistance","1",CVAR_ARCHIVE);

	if (!init)
	{
		Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
		return 0;
	}

	Com_Printf("VR_OVR: Oculus Rift support initialized...\n");

	return 1;
}

void VR_OVR_Shutdown()
{
	LibOVR_Shutdown();
}



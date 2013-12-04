#include "vr.h"
#include "vr_ovr.h"
#include "vr_libovr.h"

cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_chromatic;
cvar_t *vr_ovr_debug;
cvar_t *vr_ovr_prediction;
cvar_t *vr_ovr_distortion;
cvar_t *vr_ovr_lensdistance;
cvar_t *vr_ovr_autoscale;
cvar_t *vr_ovr_autolensdistance;
cvar_t *vr_ovr_bicubic;
cvar_t *vr_ovr_supersample;

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
	VR_OVR_ResetHMDOrientation
};

void VR_OVR_SetFOV()
{
	if (vr_ovr_settings.initialized)
	{
		float scale = VR_OVR_GetDistortionScale();
		float fovy = 2 * atan2(vr_ovr_settings.v_screen_size * scale, 2 * vr_ovr_settings.eye_to_screen_distance);
		float viewport_fov_y = fovy * 180 / M_PI * vr_autofov_scale->value;
		float viewport_fov_x = viewport_fov_y * vrConfig.aspect;
		vrState.viewFovY = viewport_fov_y;
		vrState.viewFovX = viewport_fov_x;
	}
}


int VR_OVR_getOrientation(float euler[3])
{
	if (vr_ovr_debug->value)
		LibOVR_ProcessLatencyInputs();
	return LibOVR_GetOrientation(euler);
}

void VR_OVR_ResetHMDOrientation()
{
	LibOVR_ResetHMDOrientation();
}

int VR_OVR_RenderLatencyTest(vec4_t color) 
{
	return (vr_ovr_debug->value && LibOVR_GetLatencyTestColor(color));
}

void VR_OVR_CalcRenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float lsd = vr_ovr_autolensdistance->value ? vr_ovr_settings.lens_separation_distance : vr_ovr_lensdistance->value / 1000.0;
		float h = 1.0f - (2.0f * lsd) / vr_ovr_settings.h_screen_size;
		float le = (-1.0f - h) * (-1.0f - h);
		float re = (-1.0f + h) * (-1.0f + h);

		if (le > re)
		{
			vrConfig.minScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
			vrConfig.maxScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
		} else 
		{
			vrConfig.maxScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
			vrConfig.minScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
		}

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


float VR_OVR_GetDistortionScale()
{
	if (!vr_ovr_distortion->value)
		return 1.0f;

	switch ((int) vr_ovr_autoscale->value)
	{
	case 1:
		return vrConfig.minScale;
	case 2:
		return vrConfig.maxScale;
	default:
		return vr_ovr_scale->value;
	}
}

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

	if (vr_ovr_prediction->modified)
	{
		if (vr_ovr_prediction->value < 0.0)
			Cvar_Set("vr_ovr_prediction", "0.0");
		else if (vr_ovr_prediction->value > 75.0f)
			Cvar_Set("vr_ovr_prediction", "75.0");
		vr_ovr_prediction->modified = false;
		if (LibOVR_SetPredictionTime(vr_ovr_prediction->value))
			Com_Printf("VR_OVR: Set HMD Prediction time to %.1fms\n", vr_ovr_prediction->value);

	}
	if (vr_ovr_debug->value)
	{
		const char *results = LibOVR_ProcessLatencyResults();
		if (results)
			Com_Printf("VR_OVR: %s\n",results);		
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
	if (vr_ovr_debug->value)
	{
		int riftType = (int) vr_ovr_debug->value;
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
				settings->v_screen_size = 0.0756f;
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


int VR_OVR_isDeviceAvailable()
{
	if (!vr_ovr_debug->value)
		return LibOVR_IsDeviceAvailable();
	else
		return 1;
}


void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{

	if (!object->program)
		R_CompileShaderProgram(object);

	shader->shader = object;
	qglUseProgramObjectARB(shader->shader->program);

	shader->uniform.scale = qglGetUniformLocationARB(shader->shader->program, "scale");
	shader->uniform.scale_in = qglGetUniformLocationARB(shader->shader->program, "scaleIn");
	shader->uniform.lens_center = qglGetUniformLocationARB(shader->shader->program, "lensCenter");
	shader->uniform.screen_center = qglGetUniformLocationARB(shader->shader->program, "screenCenter");
	shader->uniform.hmd_warp_param = qglGetUniformLocationARB(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = qglGetUniformLocationARB(shader->shader->program, "chromAbParam");
	shader->uniform.texture_size = qglGetUniformLocationARB(shader->shader->program,"textureSize");
	qglUseProgramObjectARB(0);
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

	if (failure && !vr_ovr_debug->value)
		return 0;


	// get info from LibOVR

	if (!VR_OVR_GetSettings(&vr_ovr_settings))
		return 0;

	Com_Printf("...detected HMD %s with %ux%u resolution\n", vr_ovr_settings.deviceString, vr_ovr_settings.h_resolution, vr_ovr_settings.v_resolution);
	Com_Printf("...detected IPD %.1fmm\n", vr_ovr_settings.interpupillary_distance * 1000);
	if (LibOVR_SetPredictionTime(vr_ovr_prediction->value))
		Com_Printf("...set HMD Prediction time to %.1fms\n", vr_ovr_prediction->value);
	vr_ovr_prediction->modified = false;

	strncpy(string, va("%.2f", vr_ovr_settings.lens_separation_distance * 1000), sizeof(string));
	vr_ovr_lensdistance = Cvar_Get("vr_ovr_lensdistance", string, CVAR_ARCHIVE);

	if (vr_ovr_scale->value < 1)
		Cvar_SetValue("vr_ovr_scale", 1.0);

	VR_OVR_CalcRenderParam();

	strncpy(string, va("%.2f", vrConfig.ipd * 1000), sizeof(string));
	vr_ipd = Cvar_Get("vr_ipd", string, CVAR_ARCHIVE);

	if (vr_ipd->value < 0)
		Cvar_SetValue("vr_ipd", vrConfig.ipd * 1000);

	VR_OVR_SetFOV();

	if (vr_ovr_driftcorrection->value > 0.0)
		if (!LibOVR_EnableDriftCorrection())
				Com_Printf("VR_OVR: Cannot enable magnetic drift correction - device is not calibrated\n");

	/* 8/30/2013 - never auto enable this.
	// TODO: use pixel density instead
	if (vr_ovr_settings.v_resolution > 800)
		Cvar_SetInteger("vr_hud_transparency", 1);
	*/
	Com_Printf("...calculated %.2f FOV\n", vrState.viewFovY);
	Com_Printf("...calculated %.2f minimum distortion scale\n", vrConfig.minScale);
	Com_Printf("...calculated %.2f maximum distortion scale\n", vrConfig.maxScale);

	return 1;
}

void VR_OVR_Disable()
{
	LibOVR_DeviceRelease();
}


int VR_OVR_Init()
{
	qboolean init = LibOVR_Init();

	vr_ovr_supersample = Cvar_Get("vr_ovr_supersample","1.0",CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","1.0",CVAR_ARCHIVE);
	vr_ovr_prediction = Cvar_Get("vr_ovr_prediction", "40", CVAR_ARCHIVE);
	vr_ovr_lensdistance = Cvar_Get("vr_ovr_lensdistance","-1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","1",CVAR_ARCHIVE);
	vr_ovr_distortion = Cvar_Get("vr_ovr_distortion","1",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_ARCHIVE);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);
	vr_ovr_bicubic = Cvar_Get("vr_ovr_bicubic","0",CVAR_ARCHIVE);
	vr_ovr_autoscale = Cvar_Get("vr_ovr_autoscale","2",CVAR_ARCHIVE);
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



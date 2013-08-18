#include "vr.h"

#include "vr_ovr.h"

cvar_t *vr_enabled;
cvar_t *vr_autoenable;
cvar_t *vr_motionprediction;
cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_enabled;
cvar_t *vr_ipd;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_chromatic;
cvar_t *vr_hud_fov;
cvar_t *vr_hud_depth;
cvar_t *vr_hud_transparency;
cvar_t *vr_crosshair;
cvar_t *vr_crosshair_size;

int vr_ovr_detected = false;

vr_param_t vrState;
vr_attrib_t vrConfig;

vr_ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0,{ 0, 0, 0, 0,},  { 0, 0, 0, 0,}, ""};
vr_param_t vr_render_param;

vec3_t vr_lastOrientation;
vec3_t vr_orientation;

void VR_Set_OVRDistortion_Scale(float dist_scale)
{
	if (vr_ovr_settings.initialized)
	{
		float fovy = 2 * atan2(vr_ovr_settings.v_screen_size * dist_scale, 2 * vr_ovr_settings.eye_to_screen_distance);
		float viewport_fov_y = fovy * 180 / M_PI;
		float viewport_fov_x = viewport_fov_y * vrConfig.aspect;
		vrState.viewFovY = viewport_fov_y;
		vrState.viewFovX = viewport_fov_x;
		vrState.scale = dist_scale;
	}
}

void VR_Calculate_OVR_RenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float h = 1.0f - (2.0f * vr_ovr_settings.lens_separation_distance) / vr_ovr_settings.h_screen_size;
		float r = -1.0f - h;
		float dist_scale = (dk[0] + dk[1] * pow(r,2) + dk[2] * pow(r,4) + dk[3] * pow(r,6));
		vrConfig.ipd = vr_ovr_settings.interpupillary_distance;
		vrConfig.aspect = vr_ovr_settings.h_resolution / (2.0f * vr_ovr_settings.v_resolution);
		vrConfig.hmdWidth = vr_ovr_settings.h_resolution;
		vrConfig.hmdHeight = vr_ovr_settings.v_resolution;
		memcpy(vrConfig.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(vrConfig.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		vrState.projOffset = h;

		VR_Set_OVRDistortion_Scale(dist_scale);

		Com_Printf("VR_OVR: Calculated %.2f FOV\n",vrState.viewFovY);
	}
}


void VR_GetSensorOrientation(vec3_t angle)
{
	if (!vr_enabled->value)
		return;

	VectorCopy(vr_orientation,angle);
}

void VR_GetSensorOrientationDelta(vec3_t angle)
{
	if (!vr_enabled->value)
		return;

	VectorSubtract(vr_orientation,vr_lastOrientation,angle);
}

// takes time in ms, sets appropriate prediction values
void VR_SetMotionPrediction(float time)
{
	float timeInSecs = time /1000.0;
	if (VR_OVR_SetPredictionTime(timeInSecs))
		Com_Printf("VR: Set HMD Prediction time to %.1fms\n",time);
	else
		Com_Printf("VR_OVR: Error setting HMD prediction time!\n");
}


void VR_OVR_Frame()
{
	float euler[3];
	VectorCopy(vr_orientation,vr_lastOrientation);
	VR_OVR_GetOrientation(euler);
	VectorSet(vr_orientation,euler[0],euler[1],euler[2]);

	if (vr_ovr_driftcorrection->modified)
	{
		if (vr_ovr_driftcorrection->value)
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",1);
			VR_OVR_EnableMagneticCorrection();
		}
		else
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",0);
			VR_OVR_DisableMagneticCorrection();
		}
		vr_ovr_driftcorrection->modified = false;
	}

	if (vr_hud_depth->modified)
	{
		if (vr_hud_depth->value < 0.25f)
			Cvar_SetValue("vr_hud_depth",0.25);
		else if (vr_hud_depth->value > 250)
			Cvar_SetValue("vr_hud_depth",250);

		vr_hud_depth->modified = false;
	}

	if (vr_hud_fov->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_fov->value < 30)
			Cvar_SetValue("vr_hud_fov",30.0f);
		else if (vr_hud_fov->value > vrState.viewFovY * 2.0)
			Cvar_SetValue("vr_hud_fov",vrState.viewFovY * 2.0);
		vr_hud_fov->modified = false;
	}

	//R_VR_StartFrame();
}

void VR_ResetOrientation()
{
	if (vr_ovr_detected)
		VR_OVR_ResetHMDOrientation();
	VR_Frame();
	VectorCopy(vr_orientation,vr_lastOrientation);
}

void VR_Frame()
{
	if (!vr_enabled->value)
		return;

	if (vr_motionprediction->modified)
	{
		if (vr_motionprediction->value < 0.0)
			Cvar_Set("vr_motionprediction","0.0");
		else if (vr_motionprediction->value > 75.0f)
			Cvar_Set("vr_motionprediction","75.0");
		vr_motionprediction->modified = false;
		VR_SetMotionPrediction(vr_motionprediction->value);
	}

	if (vr_ovr_detected)
		VR_OVR_Frame();
}

void VR_Enable()
{
	char string[6];

	if (!vrState.available)
		return;

	Com_Printf("VR: Initializing...\n");
	Cvar_ForceSet("vr_enabled","1");

	if (VR_OVR_GetSettings(&vr_ovr_settings))
	{

		Com_Printf("VR_OVR: Detected HMD %s\n",vr_ovr_settings.devName);
		Com_Printf("VR_OVR: Detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);

		VR_Calculate_OVR_RenderParam();

		strncpy(string, va("%.2f",vrConfig.ipd * 1000), sizeof(string));
		vr_ipd = Cvar_Get("vr_ipd",string, CVAR_ARCHIVE);


		if (vr_ipd->value < 0)
			Cvar_SetValue("vr_ipd",vrConfig.ipd * 1000);
		strncpy(string, va("%.2f",vrState.scale), sizeof(string));
		vr_ovr_scale = Cvar_Get("vr_ovr_scale",string,CVAR_ARCHIVE);
		if (vr_ovr_scale->value < 0)
			Cvar_SetValue("vr_ovr_scale",vrState.scale);

		if (vr_ovr_driftcorrection->value > 0.0)
			VR_OVR_EnableMagneticCorrection();

		// TODO: use pixel density instead
		if (vr_ovr_settings.v_resolution > 800)
			Cvar_SetInteger("vr_hud_transparency",1);
	}
	VR_SetMotionPrediction(vr_motionprediction->value);
	VR_ResetOrientation();
}

void VR_Disable()
{
	Cvar_ForceSet("vr_enabled","0");
}

void VR_Shutdown()
{
	if (vr_enabled->value)
		VR_Disable();

	VR_OVR_Shutdown();
}



void VR_Init()
{

	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_autoenable = Cvar_Get("vr_autoenable","1", CVAR_ARCHIVE);
	vr_ipd = Cvar_Get("vr_ipd","-1", CVAR_ARCHIVE);
	vr_motionprediction = Cvar_Get("vr_motionprediction","40",CVAR_ARCHIVE);
	vr_hud_fov = Cvar_Get("vr_hud_fov","60",CVAR_ARCHIVE);
	vr_hud_depth = Cvar_Get("vr_hud_depth","5",CVAR_ARCHIVE);
	vr_hud_transparency = Cvar_Get("vr_hud_transparency","0", CVAR_ARCHIVE);
	vr_crosshair = Cvar_Get("vr_crosshair","1", CVAR_ARCHIVE);
	vr_crosshair_size = Cvar_Get("vr_crosshair_size","3", CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","-1",CVAR_ARCHIVE);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	Cmd_AddCommand("vr_reset_home",VR_ResetOrientation);



	// this is a little overkill on the abstraction, but the point of it was to try to make it
	// flexible enough to handle other HMD's if possible
	// -dghost 08/17/13

	vr_ovr_detected = VR_OVR_Init();
	if (vr_ovr_detected )
	{
		vrState.available = true;
//`		if (vr_autoenable->value)
//			VR_Enable();
	} else {
		Com_Printf("VR: no HMD detected...\n");

	}
}
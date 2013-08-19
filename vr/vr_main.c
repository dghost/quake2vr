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
cvar_t *vr_crosshair_brightness;
cvar_t *vr_aimmode;
cvar_t *vr_aimmode_deadzone;

vr_param_t vrState;
vr_attrib_t vrConfig;

vr_ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}, "", ""};
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
		vrConfig.xPos = vr_ovr_settings.xPos;
		vrConfig.yPos = vr_ovr_settings.yPos;
		strncpy(vrConfig.deviceName,vr_ovr_settings.deviceName,31);

		memcpy(vrConfig.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(vrConfig.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		vrState.projOffset = h;

		VR_Set_OVRDistortion_Scale(dist_scale);
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
int VR_SetMotionPrediction(float time)
{
	float timeInSecs = time /1000.0;
	if (!VR_OVR_SetPredictionTime(timeInSecs))
	{
		Com_Printf("VR_OVR: Error setting HMD prediction time!\n");
		return 0;
	}
	return 1;
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
		if (VR_SetMotionPrediction(vr_motionprediction->value))
			Com_Printf("VR: Set HMD Prediction time to %.1fms\n",vr_motionprediction->value);

	}

	if (vr_aimmode->modified)
	{
		if (vr_aimmode->value < 0.0)
			Cvar_SetInteger("vr_aimmode",0);
		else if (vr_aimmode->value >= NUM_VR_AIMMODE)
			Cvar_SetInteger("vr_aimmode",NUM_VR_AIMMODE - 1);
		else
			Cvar_SetInteger("vr_aimmode",(int) vr_aimmode->value);
	}

	if (vr_aimmode_deadzone->modified)
	{
		if (vr_aimmode_deadzone->value < 0.0)
			Cvar_SetInteger("vr_aimmode_deadzone",0);
		else if (vr_aimmode_deadzone->value > 70)
			Cvar_SetInteger("vr_aimmode_deadzone",70);
		else
			Cvar_SetInteger("vr_aimmode_deadzone",(int) vr_aimmode_deadzone->value);
	}

	if (vr_crosshair->modified)
	{
		if (vr_crosshair->value < 0.0)
			Cvar_SetInteger("vr_crosshair",0);
		else if (vr_crosshair->value > 2)
			Cvar_SetInteger("vr_crosshair",2);
		else
			Cvar_SetInteger("vr_crosshair",(int) vr_crosshair->value);
	}

	if (vr_crosshair_brightness->modified)
	{
		if (vr_crosshair_brightness->value < 0.0)
			Cvar_SetInteger("vr_crosshair_brightness",0);
		else if (vr_crosshair_brightness->value > 100)
			Cvar_SetInteger("vr_crosshair_brightness",100);
		else
			Cvar_SetInteger("vr_crosshair_brightness",(int) vr_crosshair_brightness->value);
	}

	if (vr_crosshair_size->modified)
	{
		if (vr_crosshair_size->value < 0.0)
			Cvar_SetInteger("vr_crosshair_size",0);
		else if (vr_crosshair_size->value > 5)
			Cvar_SetInteger("vr_crosshair_size",5);
	}
	VR_OVR_Frame();
}

void VR_ResetOrientation()
{

	VR_OVR_ResetHMDOrientation();
	VR_Frame();
	VectorCopy(vr_orientation,vr_lastOrientation);
}


int VR_Enable()
{
	char string[6];

	Com_Printf("VR: Initializing HMD:");

	if (VR_OVR_Init())
	{
		Com_Printf(" ok!\n");
	}else {
		Com_Printf(" failed!\n");
		return 0;
	}

	Com_Printf("VR: Getting HMD settings:");
	
	if (VR_OVR_GetSettings(&vr_ovr_settings))
	{
		
		cvar_t *vid_xpos, *vid_ypos;

		Com_Printf(" ok!\n");


		Com_Printf("...detected HMD %s\n",vr_ovr_settings.deviceString);
		Com_Printf("...detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);

		VR_Calculate_OVR_RenderParam();

		Com_Printf("...calculated %.2f FOV\n",vrState.viewFovY);
		Com_Printf("...calculated %.2f distortion scale\n", vrState.scale);

		strncpy(string, va("%.2f",vrConfig.ipd * 1000), sizeof(string));
		vr_ipd = Cvar_Get("vr_ipd",string, CVAR_ARCHIVE);


		if (vr_ipd->value < 0)
			Cvar_SetValue("vr_ipd",vrConfig.ipd * 1000);
		strncpy(string, va("%.2f",vrState.scale), sizeof(string));
		vr_ovr_scale = Cvar_Get("vr_ovr_scale",string,CVAR_ARCHIVE);
		if (vr_ovr_scale->value < 0)
		{
			Cvar_Set("vr_ovr_scale",string);
		}
		if (vr_ovr_driftcorrection->value > 0.0)
			VR_OVR_EnableMagneticCorrection();

		// TODO: use pixel density instead
		if (vr_ovr_settings.v_resolution > 800)
			Cvar_SetInteger("vr_hud_transparency",1);

		vid_xpos = Cvar_Get ("vid_xpos", "0", 0);
		vid_ypos = Cvar_Get ("vid_ypos", "0", 0);
		vrState.viewXPos = vid_xpos->value;
		vrState.viewYPos = vid_ypos->value;
	} else 
	{
		Com_Printf(" failed!\n");
		return 0;
	}
	if (VR_SetMotionPrediction(vr_motionprediction->value))
		Com_Printf("...set HMD Prediction time to %.1fms\n",vr_motionprediction->value);
	vr_motionprediction->modified = false;
	VR_ResetOrientation();
	return 1;
}

void VR_Disable()
{
	VR_OVR_Shutdown();

	Cvar_SetInteger("vid_xpos",vrState.viewXPos);
	Cvar_SetInteger("vid_ypos",vrState.viewYPos);

}

void VR_Shutdown()
{
	if (vr_enabled->value)
		VR_Disable();
}

void VR_Enable_f()
{
	if (vr_enabled->value)
		return;

	if (VR_Enable())
	{
		Cvar_ForceSet("vr_enabled","1");
		Cmd_ExecuteString("vid_restart");
	}
}

void VR_Disable_f()
{
	if (!vr_enabled->value)
		return;

	VR_Disable();
	Cvar_ForceSet("vr_enabled","0");
	Cmd_ExecuteString("vid_restart");
}

void VR_Init()
{
	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_autoenable = Cvar_Get("vr_autoenable","1", CVAR_ARCHIVE);
	vr_aimmode = Cvar_Get("vr_aimmode","6",CVAR_ARCHIVE);
	vr_aimmode_deadzone = Cvar_Get("vr_aimmode_deadzone","30",CVAR_ARCHIVE);
	vr_ipd = Cvar_Get("vr_ipd","-1", CVAR_ARCHIVE);
	vr_motionprediction = Cvar_Get("vr_motionprediction","40",CVAR_ARCHIVE);
	vr_hud_fov = Cvar_Get("vr_hud_fov","60",CVAR_ARCHIVE);
	vr_hud_depth = Cvar_Get("vr_hud_depth","5",CVAR_ARCHIVE);
	vr_hud_transparency = Cvar_Get("vr_hud_transparency","0", CVAR_ARCHIVE);
	vr_crosshair = Cvar_Get("vr_crosshair","1", CVAR_ARCHIVE);
	vr_crosshair_size = Cvar_Get("vr_crosshair_size","3", CVAR_ARCHIVE);
	vr_crosshair_brightness = Cvar_Get("vr_crosshair_brightness","75",CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","-1",CVAR_ARCHIVE);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	Cmd_AddCommand("vr_reset_home",VR_ResetOrientation);
	Cmd_AddCommand("vr_disable",VR_Disable_f);
	Cmd_AddCommand("vr_enable",VR_Enable_f);



	// this is a little overkill on the abstraction, but the point of it was to try to make it
	// flexible enough to handle other HMD's if possible
	// -dghost 08/17/13

	if (vr_autoenable->value)
		if (VR_Enable())
			Cvar_ForceSet("vr_enabled","1");

}
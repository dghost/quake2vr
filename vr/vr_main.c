#include "vr.h"
#include "vr_ovr.h"

cvar_t *vr_enabled;
cvar_t *vr_autoenable;
cvar_t *vr_motionprediction;
cvar_t *vr_ovr_driftcorrection;
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
cvar_t *vr_viewmove;
cvar_t *vr_hud_bounce;
cvar_t *vr_hud_bounce_falloff;
cvar_t *vr_fov_scale;

vr_param_t vrState;
vr_attrib_t vrConfig;

static vec3_t vr_lastOrientation;
static vec3_t vr_orientation;
static vec3_t vr_emaOrientation;
static float vr_emaWeight = 0.25;

//
// Oculus Rift specific functions
//

vr_ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}, "", ""};


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

void VR_Calculate_OVR_RenderParam()
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
			VR_OVR_EnableMagneticCorrection();
		}
		else
		{
			Cvar_SetInteger("vr_ovr_driftcorrection",0);
			VR_OVR_DisableMagneticCorrection();
		}
		vr_ovr_driftcorrection->modified = false;
	}

}

//
// General functions
//

void VR_GetSensorOrientation()
{
		float euler[3];
		vec3_t temp;
		float emaWeight = 2 / (vr_hud_bounce_falloff->value + 1);
		VectorCopy(vr_orientation,vr_lastOrientation);
		VR_OVR_GetOrientation(euler);
		VectorSet(vr_orientation,euler[0],euler[1],euler[2]);
		VectorSubtract(vr_orientation,vr_lastOrientation,temp);
		VectorScale(temp,emaWeight,temp);
		VectorMA(temp,(1-emaWeight),vr_emaOrientation,vr_emaOrientation);

}

void VR_GetOrientation(vec3_t angle)
{
	if (!vr_enabled->value)
		return;
	if (vrState.stale)
	{
		VR_GetSensorOrientation();
		vrState.stale = 0;
	}

	VectorCopy(vr_orientation,angle);
}

void VR_GetOrientationDelta(vec3_t angle)
{
	if (!vr_enabled->value)
		return;

	if (vrState.stale)
	{
		VR_GetSensorOrientation();
		vrState.stale = 0;
	}

	VectorSubtract(vr_orientation,vr_lastOrientation,angle);
}

void VR_GetOrientationEMA(vec3_t angle) 
{

	if (!vr_enabled->value)
		return;

	if (vrState.stale)
	{
		VR_GetSensorOrientation();
		vrState.stale = 0;
	}

	VectorCopy(vr_emaOrientation,angle);
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

void VR_Frame()
{
	if (!vr_enabled->value)
		return;


	// check and clamp cvars
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
		vr_aimmode->modified = false;
	}

	if (vr_aimmode_deadzone->modified)
	{
		if (vr_aimmode_deadzone->value < 0.0)
			Cvar_SetInteger("vr_aimmode_deadzone",0);
		else if (vr_aimmode_deadzone->value > 70)
			Cvar_SetInteger("vr_aimmode_deadzone",70);
		else
			Cvar_SetInteger("vr_aimmode_deadzone",(int) vr_aimmode_deadzone->value);
		vr_aimmode_deadzone->modified = false;
	}

	if (vr_crosshair->modified)
	{
		if (vr_crosshair->value < 0.0)
			Cvar_SetInteger("vr_crosshair",0);
		else if (vr_crosshair->value > 2)
			Cvar_SetInteger("vr_crosshair",2);
		else
			Cvar_SetInteger("vr_crosshair",(int) vr_crosshair->value);
		vr_crosshair->modified = false;
	}

	if (vr_crosshair_brightness->modified)
	{
		if (vr_crosshair_brightness->value < 0.0)
			Cvar_SetInteger("vr_crosshair_brightness",0);
		else if (vr_crosshair_brightness->value > 100)
			Cvar_SetInteger("vr_crosshair_brightness",100);
		else
			Cvar_SetInteger("vr_crosshair_brightness",(int) vr_crosshair_brightness->value);
		vr_crosshair_brightness->modified = false;
	}

	if (vr_crosshair_size->modified)
	{
		if (vr_crosshair_size->value < 0.0)
			Cvar_SetInteger("vr_crosshair_size",0);
		else if (vr_crosshair_size->value > 5)
			Cvar_SetInteger("vr_crosshair_size",5);
		vr_crosshair_size->modified = false;
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

	if (vr_hud_bounce->modified)
	{
		if (vr_hud_bounce->value < 0.0)
			Cvar_SetInteger("vr_hud_bounce",0);
		else if (vr_hud_bounce->value > 3)
			Cvar_SetInteger("vr_hud_bounce",3);
		vr_hud_bounce->modified = false;
	}
	
	if (vr_hud_bounce_falloff->modified)
	{
		if (vr_hud_bounce_falloff->value < 0.0)
			Cvar_SetInteger("vr_hud_bounce_falloff",0);
		else if (vr_hud_bounce_falloff->value > 60)
			Cvar_SetInteger("vr_hud_bounce_falloff", 60);
		vr_hud_bounce_falloff->modified = false;
	}

	if (vr_fov_scale->modified)
	{
		if (vr_fov_scale->value > 2.0)
			Cvar_Set("vr_fov_scale","2.0");
		if (vr_fov_scale->value < 0.5)
			Cvar_Set("vr_fov_scale","0.5");
		vr_fov_scale->modified = false;
		VR_OVR_SetFOV();
		Com_Printf("VR: New vertical FOV is %3.2f degrees\n",vrState.viewFovX);
	}
	vrState.stale = 1;
	// pull an update from the motion tracker
	VR_OVR_Frame();
}

void VR_ResetOrientation()
{

	VR_OVR_ResetHMDOrientation();
	VR_Frame();
	VectorCopy(vr_orientation,vr_lastOrientation);
	VectorSet(vr_emaOrientation,0,0,0);
}

// this is a little overkill on the abstraction, but the point of it was to try to make it
// possible to support other HMD's in the future without having to rewrite other portions of the engine.
// -dghost 08/17/13

int VR_Enable()
{
	char string[6];

	Com_Printf("VR: Initializing HMD:");

	// try to initialize LibOVR
	if (VR_OVR_Init())
	{
		Com_Printf(" ok!\n");
	}else {
		Com_Printf(" failed!\n");
		return 0;
	}

	Com_Printf("VR: Getting HMD settings:");
	
	// get info from LibOVR
	if (VR_OVR_GetSettings(&vr_ovr_settings))
	{
		
		Com_Printf(" ok!\n");


		Com_Printf("...detected HMD %s\n",vr_ovr_settings.deviceString);
		Com_Printf("...detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);

		VR_Calculate_OVR_RenderParam();

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
			VR_OVR_EnableMagneticCorrection();

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

	if (VR_SetMotionPrediction(vr_motionprediction->value))
		Com_Printf("...set HMD Prediction time to %.1fms\n",vr_motionprediction->value);
	vr_motionprediction->modified = false;
	VR_ResetOrientation();
	vrState.stale = 1;
	return 1;
}

void VR_Disable()
{
	VR_OVR_Shutdown();

}

void VR_Shutdown()
{
	if (vr_enabled->value)
		VR_Disable();
}

// console command to enable Rift support
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

// console command to disable Rift support
void VR_Disable_f()
{
	if (!vr_enabled->value)
		return;

	VR_Disable();
	Cvar_ForceSet("vr_enabled","0");
	Cmd_ExecuteString("vid_restart");
}

// launch-time initialization for Rift support
void VR_Init()
{
	vr_viewmove = Cvar_Get("vr_viewmove","0",CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","-1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	vr_ovr_chromatic = Cvar_Get("vr_ovr_chromatic","1",CVAR_ARCHIVE);
	vr_motionprediction = Cvar_Get("vr_motionprediction","40",CVAR_ARCHIVE);
	vr_ipd = Cvar_Get("vr_ipd","-1", CVAR_ARCHIVE);
	vr_hud_transparency = Cvar_Get("vr_hud_transparency","0", CVAR_ARCHIVE);
	vr_hud_fov = Cvar_Get("vr_hud_fov","65",CVAR_ARCHIVE);
	vr_hud_depth = Cvar_Get("vr_hud_depth","0.5",CVAR_ARCHIVE);
	vr_fov_scale = Cvar_Get("vr_fov_scale","1.0",CVAR_ARCHIVE);
	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_crosshair_size = Cvar_Get("vr_crosshair_size","3", CVAR_ARCHIVE);
	vr_crosshair_brightness = Cvar_Get("vr_crosshair_brightness","75",CVAR_ARCHIVE);
	vr_crosshair = Cvar_Get("vr_crosshair","1", CVAR_ARCHIVE);
	vr_hud_bounce_falloff = Cvar_Get("vr_hud_bounce_falloff","10",CVAR_ARCHIVE);
	vr_hud_bounce = Cvar_Get("vr_hud_bounce","1",CVAR_ARCHIVE);
	vr_autoenable = Cvar_Get("vr_autoenable","1", CVAR_ARCHIVE);
	vr_aimmode_deadzone = Cvar_Get("vr_aimmode_deadzone","30",CVAR_ARCHIVE);
	vr_aimmode = Cvar_Get("vr_aimmode","6",CVAR_ARCHIVE);

	Cmd_AddCommand("vr_reset_home",VR_ResetOrientation);
	Cmd_AddCommand("vr_disable",VR_Disable_f);
	Cmd_AddCommand("vr_enable",VR_Enable_f);

	Com_Printf("RiftQuake II Version %4.2f\n",RIFTQUAKE2_VERSION);

	if (vr_autoenable->value)
		if (VR_Enable())
			Cvar_ForceSet("vr_enabled","1");

}
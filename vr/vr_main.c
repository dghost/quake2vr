#include "vr.h"
#include "vr_ovr.h"
#include "vr_libovr.h"
#include "../client/client.h"

cvar_t *vr_enabled;
cvar_t *vr_autoenable;
cvar_t *vr_motionprediction;
cvar_t *vr_ipd;
cvar_t *vr_hud_fov;
cvar_t *vr_hud_depth;
cvar_t *vr_hud_transparency;
cvar_t *vr_crosshair;
cvar_t *vr_crosshair_size;
cvar_t *vr_crosshair_brightness;
cvar_t *vr_aimmode;
cvar_t *vr_aimmode_deadzone_yaw;
cvar_t *vr_aimmode_deadzone_pitch;
cvar_t *vr_viewmove;
cvar_t *vr_hud_bounce;
cvar_t *vr_hud_bounce_falloff;
cvar_t *vr_fov_scale;
cvar_t *vr_nosleep;
cvar_t *vr_neckmodel;
cvar_t *vr_neckmodel_up;
cvar_t *vr_neckmodel_forward;

vr_param_t vrState;
vr_attrib_t vrConfig;

static vec3_t vr_lastOrientation;
static vec3_t vr_orientation;
static vec4_t vr_emaOrientation;

static hmd_interface_t *hmd;

static hmd_interface_t hmd_none = {
	HMD_NONE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

//
// General functions
//


extern ovr_settings_t vr_ovr_settings;

int VR_GetSensorOrientation()
{
	vec3_t euler;
	vec4_t quat;
	float emaWeight = 2 / (vr_hud_bounce_falloff->value + 1);

	if (!hmd)
		return 0;
	if (!hmd->getOrientation(euler))
		return 0;

	VectorCopy(vr_orientation, vr_lastOrientation);
	VectorCopy(euler,vr_orientation);

	// convert last position to quaternions for SLERP
	EulerToQuat(vr_lastOrientation,quat);

	// SLERP between previous EMA orientation and new EMA orientation
//	LerpQuat(vr_emaOrientation, quat, emaWeight, vr_emaOrientation);
	SlerpQuat(vr_emaOrientation, quat, emaWeight, vr_emaOrientation);
	return 1;
}

void VR_GetOrientation(vec3_t angle)
{
	if (!hmd)
		return;

	if (vrState.stale && VR_GetSensorOrientation())
		vrState.stale = 0;

	VectorCopy(vr_orientation,angle);
}

void VR_GetOrientationDelta(vec3_t angle)
{
	if (!hmd)
		return;

	if (vrState.stale && VR_GetSensorOrientation())
		vrState.stale = 0;

	VectorSubtract(vr_orientation,vr_lastOrientation,angle);
}

// unused as of 9/30/2013 - consider removing later
void VR_GetOrientationEMA(vec3_t angle) 
{
	vec4_t quat;
	if (!hmd)
		return;
	
	VR_GetOrientationEMAQuat(quat);
	QuatToEuler(quat,angle);
}

void VR_GetOrientationEMAQuat(vec3_t quat) 
{
	float bounce = vr_hud_bounce->value / vr_hud_bounce_falloff->value;
	vec4_t t1, t2, zero = { 1, 0, 0, 0 };
	if (!hmd)
		return;

	if (vrState.stale && VR_GetSensorOrientation())
		vrState.stale = 0;

	EulerToQuat(vr_orientation,t1);
	QuatCopy(vr_emaOrientation,t2);
	// fast conjugate
	t2[0] = -t2[0];
	QuatMultiply(t2, t1, t1);
	SlerpQuat(zero, t1, bounce, quat);

}

// takes time in ms, sets appropriate prediction values
int VR_SetMotionPrediction(float time)
{
	if (!hmd)
		return 0;

	if (!hmd->setprediction(time))
	{
		Com_Printf("VR: Error setting HMD prediction time!\n");
		return 0;
	}
	return 1;
}

void VR_Frame()
{
	if (!vr_enabled->value || !hmd)
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
		VectorAdd(cl.bodyangles,cl.viewangles,cl.bodyangles);
		VectorSet(cl.aimdelta,0,0,0);
		VectorSet(cl.viewangles,0,0,0);
		vr_aimmode->modified = false;
	}

	if (vr_aimmode_deadzone_pitch->modified)
	{
		if (vr_aimmode_deadzone_pitch->value < 0.0)
			Cvar_SetInteger("vr_aimmode_deadzone_pitch",0);
		else if (vr_aimmode_deadzone_pitch->value > 360)
			Cvar_SetInteger("vr_aimmode_deadzone_pitch",360);
		else
			Cvar_SetInteger("vr_aimmode_deadzone_pitch",(int) vr_aimmode_deadzone_pitch->value);
		vr_aimmode_deadzone_pitch->modified = false;
	}

	if (vr_aimmode_deadzone_yaw->modified)
	{
		if (vr_aimmode_deadzone_yaw->value < 0.0)
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",0);
		else if (vr_aimmode_deadzone_yaw->value > 360)
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",360);
		else
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",(int) vr_aimmode_deadzone_yaw->value);
		vr_aimmode_deadzone_yaw->modified = false;
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
			Cvar_SetInteger("vr_hud_bounce", 0);
		else if (vr_hud_bounce->value > 2)
			Cvar_SetInteger("vr_hud_bounce", 2);
		vr_hud_bounce->modified = false;
	}

	if (vr_hud_bounce_falloff->modified)
	{
		if (vr_hud_bounce_falloff->value < 1.0)
			Cvar_SetInteger("vr_hud_bounce_falloff",1);
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
		hmd->setfov();
		Com_Printf("VR: New vertical FOV is %3.2f degrees\n",vrState.viewFovY);
	}

	hmd->frame();

// 	only flag as stale after a frame is rendered
//	vrState.stale = 1;

}

void VR_ResetOrientation()
{
	if (hmd)
	{
		hmd->resetOrientation();
		VR_GetSensorOrientation();
		VectorCopy(vr_orientation, vr_lastOrientation);
		QuatSet(vr_emaOrientation, 1, 0, 0, 0);
	} else {
		QuatSet(vr_emaOrientation, 1, 0, 0, 0);
		VectorSet(vr_orientation, 0, 0, 0);
		VectorSet(vr_lastOrientation, 0, 0, 0);
	}

}

// this is a little overkill on the abstraction, but the point of it was to try to make it
// possible to support other HMD's in the future without having to rewrite other portions of the engine.
// -dghost 08/17/13

int VR_Enable()
{
	char hmd_type[3];
	hmd = &available_hmds[HMD_RIFT];

	if (!hmd->attached())
	{
		Com_Printf("VR: Error - no HMD attached.\n");
		return 0;
	}

	if (!hmd->enable())
		return 0;


	if (VR_SetMotionPrediction(vr_motionprediction->value))
		Com_Printf("...set HMD Prediction time to %.1fms\n",vr_motionprediction->value);
	vr_motionprediction->modified = false;
	VR_ResetOrientation();

	strncpy(hmd_type, va("%i", hmd->type), sizeof(hmd_type));
	Cvar_ForceSet("vr_enabled", hmd_type);

	vrState.stale = 1;
	return 1;
}

void VR_Disable()
{
	hmd->disable();
	hmd = NULL;
}

void VR_Shutdown()
{
	int i = 0;
	if (vr_enabled->value)
		VR_Disable();

	for (i = 1; i < NUM_HMD_TYPES; i++)
	{
		available_hmds[i].shutdown();
	}

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
	int i = 0;
	Com_Printf("RiftQuake II Version v%1i.%1i.%1i\n", RIFTQUAKE2_MAJOR, RIFTQUAKE2_MINOR, RIFTQUAKE2_MAINT);

	available_hmds[HMD_NONE] = hmd_none;
	available_hmds[HMD_RIFT] = hmd_rift;

	for (i = 1; i < NUM_HMD_TYPES; i++)
	{
		available_hmds[i].init();
	}

	vr_viewmove = Cvar_Get("vr_viewmove","0",CVAR_ARCHIVE);
	vr_nosleep = Cvar_Get("vr_nosleep", "1", CVAR_ARCHIVE);
	vr_neckmodel_up = Cvar_Get("vr_neckmodel_up","0.232",CVAR_ARCHIVE);
	vr_neckmodel_forward = Cvar_Get("vr_neckmodel_forward","0.09",CVAR_ARCHIVE);
	vr_neckmodel = Cvar_Get("vr_neckmodel","1",CVAR_ARCHIVE);
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
	vr_aimmode_deadzone_yaw = Cvar_Get("vr_aimmode_deadzone_yaw","30",CVAR_ARCHIVE);
	vr_aimmode_deadzone_pitch = Cvar_Get("vr_aimmode_deadzone_pitch","60",CVAR_ARCHIVE);
	vr_aimmode = Cvar_Get("vr_aimmode","6",CVAR_ARCHIVE);
	Cmd_AddCommand("vr_reset_home",VR_ResetOrientation);
	Cmd_AddCommand("vr_disable",VR_Disable_f);
	Cmd_AddCommand("vr_enable",VR_Enable_f);

	if (vr_autoenable->value)
		VR_Enable();
}
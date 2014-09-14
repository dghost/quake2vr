#include "../client.h"
#include "include/vr.h"
#include "include/vr_svr.h"
#include "include/vr_ovr.h"

cvar_t *vr_enabled;
cvar_t *vr_autoenable;
cvar_t *vr_autoipd;
cvar_t *vr_autofov;
cvar_t *vr_autofov_scale;
cvar_t *vr_ipd;
cvar_t *vr_hud_fov;
cvar_t *vr_hud_depth;
cvar_t *vr_hud_transparency;
cvar_t *vr_hud_segments;
cvar_t *vr_hud_width;
cvar_t *vr_hud_height;
cvar_t *vr_chromatic;
cvar_t *vr_aimlaser;
cvar_t *vr_aimmode;
cvar_t *vr_aimmode_deadzone_yaw;
cvar_t *vr_aimmode_deadzone_pitch;
cvar_t *vr_viewmove;
cvar_t *vr_walkspeed;
cvar_t *vr_hud_bounce;
cvar_t *vr_hud_bounce_falloff;
cvar_t *vr_nosleep;
cvar_t *vr_neckmodel;
cvar_t *vr_neckmodel_up;
cvar_t *vr_neckmodel_forward;
cvar_t *vr_hmdtype;
cvar_t *vr_prediction;
cvar_t *vr_hmdstring;
cvar_t *vr_positiontracking;
cvar_t *vr_force_fullscreen;
cvar_t *vr_force_resolution;

static vec3_t vr_lastOrientation;
static vec3_t vr_orientation;
static vec4_t vr_smoothSeries;
static vec4_t vr_doubleSmoothSeries;
static vec4_t vr_smoothOrientation;

static qboolean stale;

static hmd_interface_t available_hmds[NUM_HMD_TYPES];

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
	NULL,
	NULL,
};

const char *hmd_names[] = 
{
	"none",
	"Oculus Rift",
	"SteamVR",
	0
};

//
// General functions
//

int32_t VR_GetSensorOrientation()
{
	vec3_t euler;
	vec4_t currentPos;


	float emaWeight = 2 / (vr_hud_bounce_falloff->value + 1);

	if (!hmd)
		return 0;

	if (!hmd->getOrientation || !hmd->getOrientation(euler))
		VectorSet(euler,0,0,0);

	EulerToQuat(vr_lastOrientation,currentPos);
	VectorCopy(vr_orientation, vr_lastOrientation);
	VectorCopy(euler,vr_orientation);


	// convert last position to quaternions for SLERP
//	EulerToQuat(vr_orientation,currentPos);

	// SLERP between previous EMA orientation and new EMA orientation
//	LerpQuat(vr_smoothSeries, quat, emaWeight, vr_smoothSeries);
	SlerpQuat(vr_smoothSeries, currentPos, emaWeight, vr_smoothSeries);
	SlerpQuat(vr_doubleSmoothSeries, vr_smoothSeries, emaWeight, vr_doubleSmoothSeries);


	if (vr_hud_bounce->value == VR_HUD_BOUNCE_LES)
	{
		vec4_t level,trend, zero = { 1, 0, 0, 0 };
		vec4_t negDouble;
		// fast conjugate
		QuatCopy(vr_doubleSmoothSeries,negDouble);
		negDouble[0] = -negDouble[0];

		QuatMultiply(vr_smoothSeries,negDouble,trend);
		QuatMultiply(trend,vr_smoothSeries,level);

		SlerpQuat(zero,trend, emaWeight / ( 1 - emaWeight), trend);
		QuatMultiply(level,trend,vr_smoothOrientation);
	} else if (vr_hud_bounce->value <= VR_HUD_BOUNCE_SES) {
		QuatCopy(vr_smoothSeries,vr_smoothOrientation);
	}
	return 1;
}

void VR_GetOrientation(vec3_t angle)
{
	if (!hmd)
		return;

	if (stale && VR_GetSensorOrientation())
		stale = 0;

	VectorCopy(vr_orientation,angle);
}

void VR_GetOrientationDelta(vec3_t angle)
{
	if (!hmd)
		return;

	if (stale && VR_GetSensorOrientation())
		stale = 0;

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

// returns head offset using X forward, Y left, Z up
int32_t VR_GetHeadOffset(vec3_t offset)
{
	if (!hmd || !vr_positiontracking->value)
		return false;

	if (stale && VR_GetSensorOrientation())
		stale = 0;

	return (hmd->getHeadOffset && hmd->getHeadOffset(offset));
}

int32_t VR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (hmd && hmd->getHMDPos)
	{
		hmd->getHMDPos(xpos,ypos);
		return 1;
	}
	return 0;
}

int32_t VR_GetHMDResolution(int32_t *width, int32_t *height)
{
	if (hmd && hmd->getHMDResolution)
	{
		hmd->getHMDResolution(width,height);
		return 1;
	}
	return 0;
}

void VR_GetOrientationEMAQuat(vec3_t quat) 
{
	float bounce = (!!vr_hud_bounce->value) / vr_hud_bounce_falloff->value;
	vec4_t t1, t2, zero = { 1, 0, 0, 0 };
	
	if (!hmd)
		return;

	if (stale && VR_GetSensorOrientation())
		stale = 0;


	EulerToQuat(vr_orientation,t1);
	QuatCopy(vr_smoothOrientation,t2);
	// fast conjugate
	t2[0] = -t2[0];
	QuatMultiply(t2, t1, t1);
	SlerpQuat(zero, t1, bounce, quat);

}

void VR_FrameStart()
{
	if (!vr_enabled->value || !hmd)
		return;

	// check and clamp cvars
	if (vr_aimmode->modified)
	{
		if (vr_aimmode->value < 0.0)
			Cvar_SetInteger("vr_aimmode",0);
		else if (vr_aimmode->value >= NUM_VR_AIMMODE)
			Cvar_SetInteger("vr_aimmode",NUM_VR_AIMMODE - 1);
		else
			Cvar_SetInteger("vr_aimmode",(int32_t) vr_aimmode->value);
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
			Cvar_SetInteger("vr_aimmode_deadzone_pitch",(int32_t) vr_aimmode_deadzone_pitch->value);
		vr_aimmode_deadzone_pitch->modified = false;
	}

	if (vr_aimmode_deadzone_yaw->modified)
	{
		if (vr_aimmode_deadzone_yaw->value < 0.0)
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",0);
		else if (vr_aimmode_deadzone_yaw->value > 360)
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",360);
		else
			Cvar_SetInteger("vr_aimmode_deadzone_yaw",(int32_t) vr_aimmode_deadzone_yaw->value);
		vr_aimmode_deadzone_yaw->modified = false;
	}

	if (vr_aimlaser->modified)
	{
		Cvar_SetInteger("vr_aimlaser",!!vr_aimlaser->value);
		vr_aimlaser->modified = false;
	}

	if (vr_hud_bounce->modified)
	{
		if (vr_hud_bounce->value < 0.0)
			Cvar_SetInteger("vr_hud_bounce", 0);
		else if (vr_hud_bounce->value > NUM_HUD_BOUNCE_MODES)
			Cvar_SetInteger("vr_hud_bounce", NUM_HUD_BOUNCE_MODES - 1);
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

	if (vr_autofov_scale->modified)
	{
		if (vr_autofov_scale->value > 2.0)
			Cvar_Set("vr_autofov_scale","2.0");
		if (vr_autofov_scale->value < 0.5)
			Cvar_Set("vr_autofov_scale","0.5");
		vr_autofov_scale->modified = false;
	}
	
	if (vr_prediction->modified)
	{
		if (vr_prediction->value < 0.0)
			Cvar_Set("vr_prediction", "0.0");
		else if (vr_prediction->value > 75.0f)
			Cvar_Set("vr_prediction", "75.0");
		vr_prediction->modified = false;
		if (hmd->setPrediction)
			hmd->setPrediction(vr_prediction->value);
	}

	if (vr_walkspeed->modified)
	{
		if (vr_walkspeed->value < 0.25)
			Cvar_Set("vr_walkspeed","0.25");
		else if (vr_walkspeed->value > 1.5)
			Cvar_Set("vr_walkspeed","1.5");

		vr_walkspeed->modified = false;
	}

	hmd->frameStart();

// 	only flag as stale after a frame is rendered
	stale = 1;

}

void VR_FrameEnd()
{
	if (hmd && hmd->frameEnd)
		hmd->frameEnd();
}

void VR_ResetOrientation( void )
{
	if (hmd && hmd->resetOrientation)
	{
		hmd->resetOrientation();
		VR_GetSensorOrientation();
		VectorCopy(vr_orientation, vr_lastOrientation);
		QuatSet(vr_smoothSeries, 1, 0, 0, 0);
	} else {
		QuatSet(vr_smoothSeries, 1, 0, 0, 0);
		VectorSet(vr_orientation, 0, 0, 0);
		VectorSet(vr_lastOrientation, 0, 0, 0);
	}

}

// this is a little overkill on the abstraction, but the point of it was to try to make it
// possible to support other HMD's in the future without having to rewrite other portions of the engine.
// -dghost 08/17/13

int32_t VR_Enable()
{
	char hmd_type[3];
	int32_t i;

	hmd = NULL;
	for(i = 1 ; i < NUM_HMD_TYPES; i++)
	{
		Com_Printf("VR: Checking for %s HMD...\n",hmd_names[i]);
		if (available_hmds[i].enable && available_hmds[i].enable())
		{
			hmd = &available_hmds[i];
			break;
		} 
	}

	if (!hmd)
	{
		Com_Printf("VR: Error - no HMD attached.\n");
		return 0;
	}

	Com_Printf("VR: using %s HMD.\n",hmd_names[i]);

	if (hmd->setPrediction)
		hmd->setPrediction(vr_prediction->value);
	vr_prediction->modified = false;

	VR_ResetOrientation();

	strncpy(hmd_type, va("%i", hmd->type), sizeof(hmd_type));
	Cvar_ForceSet("vr_enabled", hmd_type);

	stale = 1;
	return 1;
}

void VR_Disable()
{
	hmd->disable();
	hmd = NULL;
	Cvar_ForceSet("vr_enabled","0");
	Cvar_ForceSet("vr_hmdstring","VR Disabled");
}

void VR_Teardown()
{
	int32_t i = 0;
	if (vr_enabled->value)
		VR_Disable();

	for (i = 0; i < NUM_HMD_TYPES; i++)
	{
		if (available_hmds[i].shutdown)
			available_hmds[i].shutdown();
	}

}

// console command to enable Rift support
void VR_Enable_f(void)
{
	if (vr_enabled->value)
		return;

	if (VR_Enable())
	{
		Cmd_ExecuteString("vid_restart");
	}
}

// console command to disable Rift support
void VR_Disable_f(void)
{
	if (!vr_enabled->value)
		return;

	VR_Disable();
	Cmd_ExecuteString("vid_restart");
}

// launch-time initialization for Rift support
void VR_Startup(void)
{
	int32_t i = 0;
	Com_Printf("Quake II VR Version v%s\n", VR_VER);

	available_hmds[HMD_NONE] = hmd_none;
	available_hmds[HMD_STEAM] = hmd_steam;
	available_hmds[HMD_RIFT] = hmd_rift;

	for (i = 0; i < NUM_HMD_TYPES; i++)
	{
		if (available_hmds[i].init)
			available_hmds[i].init();
	}
	
	vr_walkspeed = Cvar_Get("vr_walkspeed","1.0",CVAR_ARCHIVE);
	vr_viewmove = Cvar_Get("vr_viewmove","0",CVAR_ARCHIVE);
	vr_positiontracking = Cvar_Get("vr_positiontracking","1",CVAR_ARCHIVE);
	vr_prediction = Cvar_Get("vr_prediction","30",CVAR_ARCHIVE);
	vr_nosleep = Cvar_Get("vr_nosleep", "1", CVAR_ARCHIVE);
	vr_neckmodel_up = Cvar_Get("vr_neckmodel_up","0.232",CVAR_ARCHIVE);
	vr_neckmodel_forward = Cvar_Get("vr_neckmodel_forward","0.09",CVAR_ARCHIVE);
	vr_neckmodel = Cvar_Get("vr_neckmodel","1",CVAR_ARCHIVE);
	vr_ipd = Cvar_Get("vr_ipd","64", CVAR_ARCHIVE);
	vr_hud_width = Cvar_Get("vr_hud_width","640",CVAR_ARCHIVE);
	vr_hud_transparency = Cvar_Get("vr_hud_transparency","1", CVAR_ARCHIVE);
	vr_hud_segments = Cvar_Get("vr_hud_segments","15",CVAR_ARCHIVE);
	vr_hud_height = Cvar_Get("vr_hud_height","480",CVAR_ARCHIVE);
	vr_hud_fov = Cvar_Get("vr_hud_fov","58",CVAR_ARCHIVE);
	vr_hud_depth = Cvar_Get("vr_hud_depth","1.0",CVAR_ARCHIVE);
	vr_hud_bounce_falloff = Cvar_Get("vr_hud_bounce_falloff","15",CVAR_ARCHIVE);
	vr_hud_bounce = Cvar_Get("vr_hud_bounce","2",CVAR_ARCHIVE);
	vr_hmdstring = Cvar_Get("vr_hmdstring","VR Disabled",CVAR_NOSET);
	vr_force_resolution = Cvar_Get("vr_force_resolution","0",CVAR_ARCHIVE);
	vr_force_fullscreen = Cvar_Get("vr_force_fullscreen","1",CVAR_ARCHIVE);
	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_aimlaser = Cvar_Get("vr_aimlaser","0", CVAR_ARCHIVE);
	vr_chromatic = Cvar_Get("vr_chromatic","1",CVAR_ARCHIVE);
	vr_autoipd = Cvar_Get("vr_autoipd","1",CVAR_ARCHIVE);
	vr_autofov_scale = Cvar_Get("vr_autofov_scale", "1.0", CVAR_ARCHIVE);
	vr_autofov = Cvar_Get("vr_autofov", "1", CVAR_ARCHIVE);
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
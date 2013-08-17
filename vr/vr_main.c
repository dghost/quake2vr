#include "vr.h"

#include "vr_ovr.h"

cvar_t *vr_enabled;
cvar_t *vr_motionprediction;
cvar_t *vr_ovr_driftcorrection;

int vr_ovr_detected = false;

vr_param_t vrState;
vr_ovr_settings_t vr_ovr_settings = { 0, 0, 0, 0, 0, 0, 0, 0,{ 0, 0, 0, 0,},  { 0, 0, 0, 0,}, ""};
vr_param_t vr_render_param;

vec3_t vr_lastOrientation;
vec3_t vr_orientation;

void VR_Set_OVRDistortion_Scale(float dist_scale)
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

void VR_Calculate_OVR_RenderParam()
{
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float h = 1.0f - (2.0f * vr_ovr_settings.lens_separation_distance) / vr_ovr_settings.h_screen_size;
		float r = -1.0f - h;
		float dist_scale = (dk[0] + dk[1] * pow(r,2) + dk[2] * pow(r,4) + dk[3] * pow(r,6));
		VR_Set_OVRDistortion_Scale(dist_scale);
		memcpy(vrState.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(vrState.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		vrState.projOffset = h;
		vrState.ipd = vr_ovr_settings.interpupillary_distance;
		Com_Printf("VR_OVR: Calculated %.2f FOV\n",vrState.viewFovY);
	}
}


void VR_GetSensorOrientation(vec3_t angle)
{
	if (!vrState.enabled)
		return;

	VectorCopy(vr_orientation,angle);
}

void VR_GetSensorOrientationDelta(vec3_t angle)
{
	if (!vrState.enabled)
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
		if (vr_ovr_driftcorrection)
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

void VR_ResetOrientation()
{
	if (vr_ovr_detected)
		VR_OVR_ResetHMDOrientation();
	VR_Frame();
	VectorCopy(vr_orientation,vr_lastOrientation);
}

void VR_Frame()
{
	if (!vrState.enabled)
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



void VR_Init()
{
	vr_enabled = Cvar_Get("vr_enabled","0",CVAR_NOSET);
	vr_motionprediction = Cvar_Get("vr_motionprediction","40",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","0",CVAR_ARCHIVE);
	Cmd_AddCommand("vr_reset_home",VR_ResetOrientation);
	// this is a little overkill on the abstraction, but the point of it was to try to make it
	// flexible enough to handle other HMD's if possible
	// -dghost 08/17/13

	vr_ovr_detected = VR_OVR_Init();
	if (vr_ovr_detected)
	{
		if (VR_OVR_GetSettings(&vr_ovr_settings))
		{

			Com_Printf("VR_OVR: Detected HMD %s\n",vr_ovr_settings.devName);
			Com_Printf("VR_OVR: Detected IPD %.1fmm\n",vr_ovr_settings.interpupillary_distance * 1000);
			VR_Calculate_OVR_RenderParam();
		}
		Com_Printf("VR: Initializing...\n");
		Cvar_ForceSet("vr_enabled","1");
		VR_SetMotionPrediction(vr_motionprediction->value);
		if (vr_ovr_driftcorrection->value)
			VR_OVR_EnableMagneticCorrection();
	} else {
		Com_Printf("VR: no HMD detected...\n");

	}
}

void VR_Shutdown()
{
	Cvar_SetInteger("vr_enabled",0);
	VR_OVR_Shutdown();
}
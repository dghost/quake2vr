#include "include/vr.h"
#include "include/vr_rift.h"
#include "../client.h"
#include "OVR_CAPI.h"
#include <SDL.h>

void IN_DownDown(void);
void IN_DownUp(void);

static struct {
	int32_t repeatCount;
	int32_t lastSendTime;
}  gamepad_repeatstatus[K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP + 1];

static const int32_t InitialRepeatDelay = 220;
static const  int32_t RepeatDelay = 160;

void VR_Rift_GetHMDResolution(int32_t *width, int32_t *height);
void VR_Rift_FrameStart();
void VR_Rift_FrameEnd();

int32_t VR_Rift_Enable();
void VR_Rift_Disable();
int32_t VR_Rift_Init();
ovrBool VR_Rift_InitSensor();
void VR_Rift_Shutdown();
int32_t VR_Rift_getOrientation(float euler[3]);
void VR_Rift_ResetHMDOrientation();
int32_t VR_Rift_SetPredictionTime(float time);
int32_t VR_Rift_getPosition(float pos[3]);
int32_t VR_Rift_GetControllerOrientation(float euler[3]);
void VR_Rift_GetInputCommands();
void VR_Rift_GetControllerMoveInput(usercmd_t *cmd);

cvar_t *vr_rift_debug;
cvar_t *vr_rift_maxfov;
cvar_t *vr_rift_enable;
cvar_t *vr_rift_autoprediction;
cvar_t *vr_rift_dk2_color_hack;
cvar_t *vr_rift_trackingloss;
static cvar_t *gamepad_trigger_threshold;
static cvar_t *gamepad_stick_mode;
static cvar_t *gamepad_stick_toggle;

ovrSession hmd;
ovrHmdDesc hmdDesc;
ovrGraphicsLuid graphicsLuid;
ovrEyeRenderDesc eyeDesc[2];

ovrTrackingState trackingState;
ovrInputState lastInputState;
static qboolean gamepad_sticktoggle[2];
double frameTime;
long long riftFrameIndex;
static ovrBool sensorEnabled = 0;
static float ovrPlayerHeightMeters = 0;
static qboolean isAutoCrouching = false, wasAutoCrouching = false;

static ovrBool libovrInitialized = 0;

static double prediction_time;

rift_render_export_t renderExport;

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_Rift_Init,
	VR_Rift_Shutdown,
	VR_Rift_Enable,
	VR_Rift_Disable,
	VR_Rift_FrameStart,
	VR_Rift_FrameEnd,
	VR_Rift_ResetHMDOrientation,
	VR_Rift_getOrientation,
	VR_Rift_getPosition,
	VR_Rift_SetPredictionTime,
	NULL,
	VR_Rift_GetHMDResolution,
	VR_Rift_GetControllerOrientation,
	VR_Rift_GetInputCommands,
	VR_Rift_GetControllerMoveInput
};

static riftname_t hmdnames[255];


void VR_Rift_GetFOV(float *fovx, float *fovy)
{
	*fovx = hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan + hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan;
	*fovy = hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan + hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan;
}

void VR_Rift_GetHMDResolution(int32_t *width, int32_t *height)
{
	if (hmd)
	{
		*width = hmdDesc.Resolution.w;
		*height = hmdDesc.Resolution.h;
	}
}


void VR_Rift_QuatToEuler(ovrQuatf q, vec3_t e)
{
	vec_t w = q.w;
	vec_t Q[3] = { q.x, q.y, q.z };

	vec_t ww = w*w;
	vec_t Q11 = Q[1] * Q[1];
	vec_t Q22 = Q[0] * Q[0];
	vec_t Q33 = Q[2] * Q[2];

	vec_t s2 = -2.0f * (-w*Q[0] + Q[1] * Q[2]);

	if (s2 < -1.0f + 0.0000001f)
	{ // South pole singularity
		e[YAW] = 0.0f;
		e[PITCH] = -M_PI / 2.0f;
		e[ROLL] = atan2(2.0f*(-Q[1] * Q[0] + w*Q[2]),
			ww + Q22 - Q11 - Q33);
	}
	else if (s2 > 1.0f - 0.0000001f)
	{  // North pole singularity
		e[YAW] = 0.0f;
		e[PITCH] = M_PI / 2.0f;
		e[ROLL] = atan2(2.0f*(-Q[1] * Q[0] + w*Q[2]),
			ww + Q22 - Q11 - Q33);
	}
	else
	{
		e[YAW] = -atan2(-2.0f*(w*Q[1] - -Q[0] * Q[2]),
			ww + Q33 - Q11 - Q22);
		e[PITCH] = asin(s2);
		e[ROLL] = atan2(2.0f*(w*Q[2] - -Q[1] * Q[0]),
			ww + Q11 - Q22 - Q33);
	}

	e[PITCH] = -RAD2DEG(e[PITCH]);
	e[YAW] = RAD2DEG(e[YAW]);
	e[ROLL] = -RAD2DEG(e[ROLL]);
}

int32_t VR_Rift_getOrientation(float euler[3])
{
	double time = 0.0;
	ovrSessionStatus sessionStatus;

	if (!hmd)
		return 0;

	ovr_GetSessionStatus(hmd, &sessionStatus);

	if (sessionStatus.ShouldRecenter)
	{
		ovr_RecenterTrackingOrigin(hmd);
	}

	if (vr_rift_autoprediction->value > 0)
		time = frameTime;
	else
		time = ovr_GetTimeInSeconds() + prediction_time;
	trackingState = ovr_GetTrackingState(hmd, time, true);
	if (trackingState.StatusFlags & ovrStatus_OrientationTracked)
	{
		//vec3_t temp;
		VR_Rift_QuatToEuler(trackingState.HeadPose.ThePose.Orientation, euler);
		/*if (trackingState.StatusFlags & ovrStatus_PositionTracked) {
			VR_Rift_QuatToEuler(trackingState.LeveledCameraPose.Orientation, temp);
			renderExport.cameraYaw = euler[YAW] - temp[YAW];
			AngleClamp(&renderExport.cameraYaw);
		}
		else*/ {
			renderExport.cameraYaw = 0.0;
		}
		return 1;
	}
	return 0;
}

int32_t VR_Rift_GetControllerOrientation(float euler[3])
{
	double time = 0.0;
	int32_t vrHand = 0;

	if (!hmd)
		return 0;

	if (vr_controllermode->value)
	{
		vrHand = (int32_t)vr_controllermode->value;
	}
	if (vrHand == 0)
	{
		return 0;
	}
	vrHand = vrHand == 2 ? ovrHand_Left : ovrHand_Right;

	if (trackingState.HandStatusFlags[vrHand] & ovrStatus_OrientationTracked)
	{
		VR_Rift_QuatToEuler(trackingState.HandPoses[vrHand].ThePose.Orientation, euler);
		euler[ROLL] = -euler[ROLL];
		return 1;
	}

	return 0;
}

void SCR_CenterAlert(char *str);
int32_t VR_Rift_getPosition(float pos[3])
{
	qboolean tracked = 0;
	if (!hmd)
		return 0;

	//	if (!sensorEnabled)
	//		VR_Rift_InitSensor();

	if (sensorEnabled && renderExport.positionTracked)
	{

		tracked = trackingState.StatusFlags & ovrStatus_PositionTracked ? 1 : 0;
		if (tracked)
		{
			ovrPosef pose = trackingState.HeadPose.ThePose;
			VectorSet(pos, -pose.Position.z, pose.Position.x, pose.Position.y);

			if (vr_autocrouch->value)
			{
				if (pos[2] < ovrPlayerHeightMeters / -5.0)
				{
					isAutoCrouching = true;
					if (cl.frame.playerstate.pmove.pm_flags & PMF_DUCKED)
					{
						pos[2] += 0.75; // Viewheight for player +22 -> -2 in PM_CheckDuck(). 24*pHeightMeters/pHeightUnits = 0.75m
					}
				}
				else
				{
					isAutoCrouching = false;
				}
			}

			VectorScale(pos, (PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M), pos);
		}

		if (vr_rift_trackingloss->value > 0 && trackingState.StatusFlags & ovrTracker_Connected)
		{
			if (tracked && !renderExport.hasPositionLock && vr_rift_debug->value)
				SCR_CenterAlert("Position tracking enabled");
			else if (!tracked && renderExport.hasPositionLock && vr_rift_debug->value)
				SCR_CenterAlert("Position tracking interrupted");
		}

		renderExport.hasPositionLock = tracked;
	}
	return tracked;
}


void VR_Rift_ResetHMDOrientation()
{
	if (hmd)
	{
		ovrTrackingState state;

		ovr_RecenterTrackingOrigin(hmd);

		ovr_SetTrackingOriginType(hmd, ovrTrackingOrigin_FloorLevel);
		state = ovr_GetTrackingState(hmd, 0, false);
		ovr_SetTrackingOriginType(hmd, ovrTrackingOrigin_EyeLevel);
		ovrPlayerHeightMeters = state.HeadPose.ThePose.Position.y;
	}
}

void Rift_ParseThumbstick(ovrVector2f thumbstick, vec3_t out)
{
	float LX = thumbstick.x, LY = thumbstick.y;
	//determine how far the controller is pushed
	float mag = sqrt(LX*LX + LY*LY);

	//determine the direction the controller is pushed
	if (mag > 0)
	{
		out[0] = LX / mag;
		out[1] = LY / mag;
	}
	else
	{
		out[0] = 0;
		out[1] = 0;
	}
	out[2] = mag;
}

void VR_Rift_GetControllerMoveInput(usercmd_t *cmd)
{
	vec3_t leftPos, rightPos;

	Rift_ParseThumbstick(lastInputState.Thumbstick[ovrHand_Left], leftPos);
	Rift_ParseThumbstick(lastInputState.Thumbstick[ovrHand_Right], rightPos);

	ThumbstickMove(cmd, leftPos, rightPos, true);
}

static void RiftTouchInput_HandleRepeat(keynum_t key, int32_t delayMultiplier)
{
	int32_t index = key - K_GAMEPAD_LSTICK_UP;
	int32_t delay = 0;
	qboolean send = false;

	if (index < 0 || index >(K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP))
		return;

	if (gamepad_repeatstatus[index].repeatCount == 0)
		delay = InitialRepeatDelay * delayMultiplier;
	else
		delay = RepeatDelay * delayMultiplier;

	if (cl.time > gamepad_repeatstatus[index].lastSendTime + delay || cl.time < gamepad_repeatstatus[index].lastSendTime)
		send = true;

	if (send)
	{
		gamepad_repeatstatus[index].lastSendTime = cl.time;
		gamepad_repeatstatus[index].repeatCount++;
		Key_Event(key, true, 0);
	}

}

void VR_Rift_GetInputCommands()
{
	// NOTE: Gamepad / XBox controller isn't handled here because Q2 already supports & handles XInput devices separately
	int32_t i;
	int32_t handleThumbStickButtonsNormally = !gamepad_stick_toggle->value || cls.key_dest == key_menu;

	if (hmd && vr_autocrouch->value)
	{
		if (!wasAutoCrouching && isAutoCrouching)
		{
			Cmd_ExecuteString("+movedown");
		}
		if (wasAutoCrouching && !isAutoCrouching)
		{
			Cmd_ExecuteString("-movedown");
		}
		wasAutoCrouching = isAutoCrouching;
	}

	if (hmd && (ovr_GetConnectedControllerTypes(hmd) & ovrControllerType_Touch))
	{
		ovrInputState inputState;
		const float sq1over2 = sqrt(0.5);
		if (OVR_SUCCESS(ovr_GetInputState(hmd, ovrControllerType_Touch, &inputState)))
		{
			{
				qboolean leftTriggerOld, leftTrigger, rightTriggerOld, rightTrigger, leftGripOld, leftGrip, rightGripOld, rightGrip;
				float triggerThreshold = ClampCvar(0.04, 0.96, gamepad_trigger_threshold->value);

				leftTriggerOld = lastInputState.IndexTrigger[ovrHand_Left] > triggerThreshold;
				leftTrigger = inputState.IndexTrigger[ovrHand_Left] > triggerThreshold;
				if (!leftTriggerOld && leftTrigger) Key_Event(K_GAMEPAD_LT, true, 0);
				if (leftTriggerOld && !leftTrigger) Key_Event(K_GAMEPAD_LT, false, 0);

				rightTriggerOld = lastInputState.IndexTrigger[ovrHand_Right] > triggerThreshold;
				rightTrigger = inputState.IndexTrigger[ovrHand_Right] > triggerThreshold;
				if (!rightTriggerOld && rightTrigger) Key_Event(K_GAMEPAD_RT, true, 0);
				if (rightTriggerOld && !rightTrigger) Key_Event(K_GAMEPAD_RT, false, 0);

				leftGripOld = lastInputState.HandTrigger[ovrHand_Left] > triggerThreshold;
				leftGrip = inputState.HandTrigger[ovrHand_Left] > triggerThreshold;
				if (!leftGripOld && leftGrip) Key_Event(K_GAMEPAD_LS, true, 0);
				if (leftGripOld && !leftGrip) Key_Event(K_GAMEPAD_LS, false, 0);

				rightGripOld = lastInputState.HandTrigger[ovrHand_Right] > triggerThreshold;
				rightGrip = inputState.HandTrigger[ovrHand_Right] > triggerThreshold;
				if (!rightGripOld && rightGrip) Key_Event(K_GAMEPAD_RS, true, 0);
				if (rightGripOld && !rightGrip) Key_Event(K_GAMEPAD_RS, false, 0);
			}

			{
				int32_t aimStickHand = (int32_t)gamepad_stick_mode->value == 1 ? ovrHand_Left : ovrHand_Right;
				if (cls.key_dest != key_menu && vr_controllermode->value && (trackingState.HandStatusFlags[aimStickHand] & ovrStatus_OrientationTracked))
				{
					// For Oculus Touch, interpret aim/look thumbstick up/ down as gamepad up/down when VR controller tracking is on.
					// This is because pitch (aim up/ down) is tied to the controller, so thumbstick up/down would otherwise be unused.
					// And Oculus Touch has no DPad, so the user has fewer inputs to work with. This way, with the default bindings,
					// the user can access Next Item & Use Item when using VR controller input.
					vec3_t oldStick, newStick;
					int32_t oldDir = 0, newDir = 0;
					Rift_ParseThumbstick(lastInputState.Thumbstick[aimStickHand], oldStick);
					Rift_ParseThumbstick(inputState.Thumbstick[aimStickHand], newStick);
					if (oldStick[1] * oldStick[2] <= -sq1over2) oldDir = K_GAMEPAD_DOWN;
					if (oldStick[1] * oldStick[2] >= sq1over2) oldDir = K_GAMEPAD_UP;
					if (newStick[1] * newStick[2] <= -sq1over2) newDir = K_GAMEPAD_DOWN;
					if (newStick[1] * newStick[2] >= sq1over2) newDir = K_GAMEPAD_UP;
					if (newDir != oldDir)
					{
						if (oldDir != 0)
						{
							Key_Event(oldDir, false, 0);
						}
						if (newDir != 0)
						{
							RiftTouchInput_HandleRepeat(newDir, 3);
						}
					}
				}
			}

			if (cls.key_dest == key_menu)
			{
				for (i = 0; i < 2; i++)
				{
					vec3_t oldStick, newStick;
					int32_t oldDir = 0, newDir = 0;
					int32_t upKey = i == 0 ? K_GAMEPAD_LSTICK_UP : K_GAMEPAD_RSTICK_UP;
					Rift_ParseThumbstick(lastInputState.Thumbstick[i], oldStick);
					Rift_ParseThumbstick(inputState.Thumbstick[i], newStick);
					if (oldStick[1] * oldStick[2] <= -sq1over2) oldDir = upKey + 1; // Down
					if (oldStick[1] * oldStick[2] >= sq1over2) oldDir = upKey;      // Up
					if (oldStick[0] * oldStick[2] <= -sq1over2) oldDir = upKey + 2; // Left
					if (oldStick[0] * oldStick[2] >= sq1over2) oldDir = upKey + 3;  // Right
					if (newStick[1] * newStick[2] <= -sq1over2) newDir = upKey + 1;
					if (newStick[1] * newStick[2] >= sq1over2) newDir = upKey;
					if (newStick[0] * newStick[2] <= -sq1over2) newDir = upKey + 2;
					if (newStick[0] * newStick[2] >= sq1over2) newDir = upKey + 3;
					if (newDir != oldDir)
					{
						if (oldDir != 0)
						{
							Key_Event(oldDir, false, 0);
						}
						if (newDir != 0)
						{
							RiftTouchInput_HandleRepeat(newDir, 1);
						}
					}
				}
			}

			{
				int32_t TouchButtonCount = handleThumbStickButtonsNormally ? 7 : 5;
				ovrButton ovrButtons[] = { ovrButton_A, ovrButton_B, ovrButton_X, ovrButton_Y, ovrButton_Enter, ovrButton_LThumb, ovrButton_RThumb, };
				int32_t gamepadButtons[] = { K_GAMEPAD_A, K_GAMEPAD_B, K_GAMEPAD_X, K_GAMEPAD_Y, K_GAMEPAD_START, K_GAMEPAD_LEFT_STICK, K_GAMEPAD_RIGHT_STICK };
				for (i = 0; i < TouchButtonCount; i++)
				{
					qboolean oldState, newState;
					oldState = lastInputState.Buttons & ovrButtons[i];
					newState = inputState.Buttons & ovrButtons[i];
					if (!oldState && newState) Key_Event(gamepadButtons[i], true, 0);
					if (oldState && !newState) Key_Event(gamepadButtons[i], false, 0);
				}
			}

			if (handleThumbStickButtonsNormally)
			{
				memset(gamepad_sticktoggle, 0, sizeof(gamepad_sticktoggle));
			}
			else
			{
				if ((inputState.Buttons & ovrButton_LThumb) && !(lastInputState.Buttons & ovrButton_LThumb))
				{
					gamepad_sticktoggle[0] = !gamepad_sticktoggle[0];
					Key_Event(K_GAMEPAD_LEFT_STICK, gamepad_sticktoggle[0], 0);
				}
				if ((inputState.Buttons & ovrButton_RThumb) && !(lastInputState.Buttons & ovrButton_RThumb))
				{
					gamepad_sticktoggle[1] = !gamepad_sticktoggle[1];
					Key_Event(K_GAMEPAD_RIGHT_STICK, gamepad_sticktoggle[1], 0);
				}
			}

			memcpy(&lastInputState, &inputState, sizeof(ovrInputState));
			return;
		}
	}
	memset(&lastInputState, 0, sizeof(ovrInputState));
}

ovrBool VR_Rift_InitSensor()
{
	sensorEnabled = hmdDesc.AvailableTrackingCaps & ovrTrackingCap_Position;
	return hmdDesc.AvailableTrackingCaps & ovrTrackingCap_Orientation;
}

int32_t VR_Rift_SetPredictionTime(float time)
{
	prediction_time = time / 1000.0;
	if (vr_rift_debug->value)
		Com_Printf("VR_Rift: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}

void VR_Rift_FrameStart()
{
	if (!renderExport.withinFrame)
	{
		frameTime = ovr_GetPredictedDisplayTime(hmd, riftFrameIndex);
	}
	renderExport.withinFrame = true;
}

void VR_Rift_FrameEnd()
{
	renderExport.withinFrame = false;
}

float VR_Rift_GetGammaMin()
{
	float result = 0.0;
	if (hmd && hmdDesc.Type >= ovrHmd_DK2)
		result = 1.0 / 255.0;
	return result;
}

riftname_t *VR_Rift_GetNameList()
{
	return hmdnames;
}

int32_t VR_Rift_Enable()
{
	qboolean isDebug = false;
	uint32_t device = 0;
	ovrResult result = 0;

	if (!vr_rift_enable->value)
		return 0;

	if (!libovrInitialized)
	{
		result = ovr_Initialize(0);

		if (OVR_FAILURE(result))
		{
			Com_Printf("VR_Rift: Fatal error: could not initialize LibOVR!\n");
			libovrInitialized = 0;
			return 0;
		}
		else {
			Com_Printf("VR_Rift: %s initialized...\n", ovr_GetVersionString());
			libovrInitialized = 1;
		}
	}
	{

		qboolean found = false;


		Com_Printf("VR_Rift: Enumerating devices...\n");

		Com_Printf("VR_Rift: Initializing HMD: ");

		renderExport.withinFrame = false;
		result = ovr_Create(&hmd, &graphicsLuid);

		if (OVR_FAILURE(result))
		{
			Com_Printf("no HMD detected!\n");
		}
		else {
			Com_Printf("ok!\n");
		}

		hmdDesc = ovr_GetHmdDesc(hmd);

		if (hmdDesc.AvailableTrackingCaps & ovrTrackingCap_Position) {
			renderExport.positionTracked = (qboolean) true;
			Com_Printf("...supports position tracking\n");
		}
		else {
			renderExport.positionTracked = (qboolean) false;
		}
		Com_Printf("...has type %s\n", hmdDesc.ProductName);
		Com_Printf("...has %ux%u native resolution\n", hmdDesc.Resolution.w, hmdDesc.Resolution.h);

		if (!VR_Rift_InitSensor())
		{
			Com_Printf("VR_Rift: Sensor initialization failed!\n");
		}
		return 1;
	}
}

void VR_Rift_Disable()
{
	renderExport.withinFrame = false;
	if (hmd)
	{
		ovr_Destroy(hmd);
	}
	hmd = NULL;
}


int32_t VR_Rift_Init()
{

	/*
	if (!libovrInitialized)
	{
	libovrInitialized = ovr_Initialize();
	if (!libovrInitialized)
	{
	Com_Printf("VR_Rift: Fatal error: could not initialize LibOVR!\n");
	return 0;
	} else {
	Com_Printf("VR_Rift: %s initialized...\n",ovr_GetVersionString());
	}
	}
	*/

	vr_rift_trackingloss = Cvar_Get("vr_rift_trackingloss", "0", CVAR_CLIENT);
	vr_rift_maxfov = Cvar_Get("vr_rift_maxfov", "0", CVAR_CLIENT);
	vr_rift_enable = Cvar_Get("vr_rift_enable", "1", CVAR_CLIENT);
	vr_rift_dk2_color_hack = Cvar_Get("vr_rift_dk2_color_hack", "0", CVAR_CLIENT);
	vr_rift_debug = Cvar_Get("vr_rift_debug", "0", CVAR_CLIENT);
	vr_rift_autoprediction = Cvar_Get("vr_rift_autoprediction", "1", CVAR_CLIENT);
	gamepad_trigger_threshold = Cvar_Get("gamepad_trigger_threshold", "0.12", CVAR_ARCHIVE);
	gamepad_stick_toggle = Cvar_Get("gamepad_stick_toggle", "0", CVAR_ARCHIVE);
	gamepad_stick_mode = Cvar_Get("gamepad_stick_mode", "0", CVAR_ARCHIVE);

	memset(&lastInputState, 0, sizeof(ovrInputState));
	riftFrameIndex = 0;
	ovrPlayerHeightMeters = 1.75;
	isAutoCrouching = false;
	wasAutoCrouching = false;

	return 1;
}

void VR_Rift_Shutdown()
{
	if (libovrInitialized)
		ovr_Shutdown();
	libovrInitialized = 0;
}



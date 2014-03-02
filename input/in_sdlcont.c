#include "../client/client.h"
#include "../win32/winquake.h"
#include <Xinput.h>
#include "in_sdlcont.h"

/*
============================================================

XBOX 360 GAMEPAD CONTROL

============================================================
*/

enum _XboxStickModes
{
	Xbox_StickDefault = 0, Xbox_StickSwap
};

typedef enum _XboxDirection {
	Xbox_None,
	Xbox_Up,
	Xbox_Down,
	Xbox_Left,
	Xbox_Right
} xboxdir_t;


cvar_t  *autosensitivity;
cvar_t	*xbox_usernum;
cvar_t	*xbox_stick_mode;
cvar_t	*xbox_trigger_threshold;
cvar_t	*xbox_pitch_sensitivity;
cvar_t	*xbox_yaw_sensitivity;
cvar_t  *xbox_stick_toggle;

static XINPUT_GAMEPAD xbox_oldstate;
static int xbox_lastDevice = -1;
static struct {
	int repeatCount;
	int lastSendTime;
}  xbox_repeatstatus[12];

static qboolean xbox_sticktoggle[2];

#define XBOX_INITIAL_REPEAT_DELAY 220
#define XBOX_REPEAT_DELAY 160




void IN_XinputInit (void)
{
	DWORD i;
	cvar_t		*cv;


	// abort startup if user requests no xbox support
	cv = Cvar_Get ("in_initxbox", "1", CVAR_NOSET);
	if ( !cv->value ) 
		return; 
	autosensitivity			= Cvar_Get ("autosensitivity",			"1",		CVAR_ARCHIVE);
	xbox_yaw_sensitivity	= Cvar_Get ("xbox_yaw_sensitivity",		"1.75",		CVAR_ARCHIVE);
	xbox_pitch_sensitivity	= Cvar_Get ("xbox_pitch_sensitivity",	"1.75",		CVAR_ARCHIVE);
	xbox_trigger_threshold	= Cvar_Get ("xbox_trigger_threshold",	"0.12",		CVAR_ARCHIVE);
	xbox_stick_toggle		= Cvar_Get ("xbox_stick_toggle",		"0",		CVAR_ARCHIVE);
	xbox_stick_mode			= Cvar_Get ("xbox_stick_mode",			"0",		CVAR_ARCHIVE);
	xbox_usernum			= Cvar_Get ("xbox_usernum",				"0",		CVAR_ARCHIVE);


	for (i = 0; i < 4; i++)
	{
		XINPUT_STATE state;
		memset(&state, 0, sizeof(XINPUT_STATE));
		if (XInputGetState(i, &state) == ERROR_SUCCESS)
			Com_Printf("Found Xbox 360 Controller at %i\n",i);
	}

}

void Xbox_ParseThumbStick(float LX, float LY, float deadzone, vec3_t out)
{
	//determine how far the controller is pushed
	float mag = sqrt(LX*LX + LY*LY);

	//determine the direction the controller is pushed
	if (mag > 0)
	{
		out[0] = LX / mag;
		out[1] = LY / mag;
	} else 
	{
		out[0] = 0;
		out[1] = 0;
	}
	//check if the controller is outside a circular dead zone
	if (mag > deadzone)
	{
		//clip the magnitude at its expected maximum value
		if (mag > 32767) 
			mag = 32767;

		//adjust magnitude relative to the end of the dead zone
		mag -= deadzone;

		//optionally normalize the magnitude with respect to its expected range
		//giving a magnitude value of 0.0 to 1.0
		out[2] = mag / (32767 - deadzone);
	}
	else //if the controller is in the deadzone zero out the magnitude
	{
		out[2] = 0.0;
	}
}

void Xbox_ParseDirection(vec3_t dir, xboxdir_t *out)
{
	float x = dir[0] * dir[2];
	float y = dir[1] * dir[2];
	const float sq1over2 = sqrt(0.5);
	if (dir[2] == 0.0f)
		*out = Xbox_None;
	else if (x > sq1over2)
		*out = Xbox_Right;
	else if (x < -sq1over2)
		*out = Xbox_Left;
	else if (y > sq1over2)
		*out = Xbox_Up;
	else if (y < -sq1over2)
		*out = Xbox_Down;
	else
		*out = Xbox_None;
	return;
}

void Xbox_HandleRepeat(keynum_t key)
{
	int index = key - K_XBOX_LSTICK_UP;
	int delay = 0;
	qboolean send = false;

	if (index < 0 || index > 12)
		return;
	
	if (xbox_repeatstatus[index].repeatCount == 0)
		delay = XBOX_INITIAL_REPEAT_DELAY;
	else
		delay = XBOX_REPEAT_DELAY;
	
	if (cl.time > xbox_repeatstatus[index].lastSendTime + delay || cl.time < xbox_repeatstatus[index].lastSendTime)
		send = true;
	
	if (send)
	{
		xbox_repeatstatus[index].lastSendTime = cl.time;
		xbox_repeatstatus[index].repeatCount++;
		Key_Event (key, true, 0);
	}

}


void Xbox_SendKeyup(keynum_t key)
{
	int index = key - K_XBOX_LSTICK_UP;

	if (index < 0 || index > 12)
		return;
	xbox_repeatstatus[index].lastSendTime = 0;
	xbox_repeatstatus[index].repeatCount = 0;

	Key_Event (key, false, 0);
}

void IN_XinputCommands (void)
{
	XINPUT_STATE newState;
	XINPUT_GAMEPAD *n = &newState.Gamepad;
	XINPUT_GAMEPAD *o = &xbox_oldstate;
	unsigned int i = 0;
	unsigned int j = 1;
	unsigned int triggerThreshold = ClampCvar(0.04,0.96,xbox_trigger_threshold->value) * 255.0f;


	int device = (int) xbox_usernum->value - 1;

	if (device < 0)
	{
		if (xbox_lastDevice >= 0 && XInputGetState(xbox_lastDevice,&newState) != ERROR_SUCCESS)
		{
			char buffer[128];
			memset(buffer,0,sizeof(buffer));
			sprintf(buffer,"Xbox 360 Controller %i disconnected",xbox_lastDevice+1);
			SCR_CenterPrint(buffer);
			xbox_lastDevice = -1;
		} 

		if (xbox_lastDevice < 0)
		{
			memset(&newState,0,sizeof(newState));
			for (device = 0; device < 4 ; device++)
			{
				if (XInputGetState(device,&newState) == ERROR_SUCCESS)
				{
					char buffer[128];
					memset(buffer,0,sizeof(buffer));
					sprintf(buffer,"Using Xbox 360 Controller %i",device+1);
					SCR_CenterPrint(buffer);
					xbox_lastDevice = device;
					break;
				}
			}
		}	
	} else 
	{
		xbox_lastDevice = device;
		if (XInputGetState(device,&newState) != ERROR_SUCCESS)
		{
			memset(&newState,0,sizeof(newState));
			xbox_lastDevice = -1;
		}

	}

	if (cls.key_dest == key_menu)
	{
		vec3_t oldStick, newStick;
		xboxdir_t oldDir, newDir;
	


		Xbox_ParseThumbStick(n->sThumbLX,n->sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,newStick);
		Xbox_ParseThumbStick(o->sThumbLX,o->sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,oldStick);

		Xbox_ParseDirection(newStick,&newDir);
		Xbox_ParseDirection(oldStick,&oldDir);

		for (i = 0; i < 4; i++)
		{
			if (newDir == Xbox_Up + i)
				Xbox_HandleRepeat(K_XBOX_LSTICK_UP + i);
			if (newDir != Xbox_Up + i && oldDir == Xbox_Up + i)
				Xbox_SendKeyup(K_XBOX_LSTICK_UP + i);
		}

		Xbox_ParseThumbStick(n->sThumbRX,n->sThumbRY,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,newStick);
		Xbox_ParseThumbStick(o->sThumbRX,o->sThumbRY,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,oldStick);

		Xbox_ParseDirection(newStick,&newDir);
		Xbox_ParseDirection(oldStick,&oldDir);

		for (i = 0; i < 4; i++)
		{
			if (newDir == Xbox_Up + i)
				Xbox_HandleRepeat(K_XBOX_RSTICK_UP + i);
			if (newDir != Xbox_Up + i && oldDir == Xbox_Up + i)
				Xbox_SendKeyup(K_XBOX_RSTICK_UP + i);
		}

		for (i = 0; i < 4; i++)
		{

			if (n->wButtons & j)
				Xbox_HandleRepeat(K_XBOX_UP + i);

			if (!(n->wButtons & j) && (o->wButtons & j))
				Xbox_SendKeyup(K_XBOX_UP + i);
			
			
			j = j << 1;
		}
	} 


	for ( ; i< 6 ; i++)
	{

		if (n->wButtons & j && !(o->wButtons & j))
			Key_Event (K_XBOX_UP + i, true, 0);

		if (!(n->wButtons & j) && (o->wButtons & j))
			Key_Event (K_XBOX_UP + i, false, 0);

		j = j << 1;

	}

	if (!xbox_stick_toggle->value || cls.key_dest == key_menu)
	{
		memset(xbox_sticktoggle,0,sizeof(xbox_sticktoggle));
		j = XINPUT_GAMEPAD_LEFT_THUMB;
		for (i = 0 ; i< 2 ; i++)
		{

			if (n->wButtons & j && !(o->wButtons & j))
				Key_Event (K_XBOX_LEFT_STICK + i, true, 0);

			if (!(n->wButtons & j) && (o->wButtons & j))
				Key_Event (K_XBOX_LEFT_STICK + i, false, 0);


			j = j << 1;
		}
	} else {
		j = XINPUT_GAMEPAD_LEFT_THUMB;
		for (i = 0 ; i< 2 ; i++)
		{
			if (n->wButtons & j && !(o->wButtons & j))
			{
				xbox_sticktoggle[i] = !xbox_sticktoggle[i];
				Key_Event (K_XBOX_LEFT_STICK + i, xbox_sticktoggle[i], 0);
			}
			j = j << 1;
		}
	}

	j = XINPUT_GAMEPAD_LEFT_SHOULDER;
	for (i = 0 ; i< 8 ; i++)
	{

		if (n->wButtons & j && !(o->wButtons & j))
			Key_Event (K_XBOXLS + i, true, 0);

		if (!(n->wButtons & j) && (o->wButtons & j))
			Key_Event (K_XBOXLS + i, false, 0);

		if (K_XBOXLS + i != K_XBOXRS)
			j = j << 1;
		else
			j = j << 3;
	}

	/* left trigger */
	if (n->bLeftTrigger >= triggerThreshold && o->bLeftTrigger < triggerThreshold)
		Key_Event (K_XBOXLT, true, 0);

	if (n->bLeftTrigger < triggerThreshold && o->bLeftTrigger >= triggerThreshold)
		Key_Event (K_XBOXLT, false, 0);

	/* right trigger */
	if (n->bRightTrigger >= triggerThreshold && o->bRightTrigger < triggerThreshold)
		Key_Event (K_XBOXRT, true, 0);

	if (n->bRightTrigger < triggerThreshold && o->bRightTrigger >= triggerThreshold)
		Key_Event (K_XBOXRT, false, 0);

	xbox_oldstate = newState.Gamepad;

}

void IN_XinputMove (usercmd_t *cmd)
{
	vec3_t leftPos,rightPos;
	vec_t *view, *move;
	float speed, aspeed;

	float pitchInvert = (m_pitch->value < 0.0) ? -1 : 1;

	if (cls.key_dest == key_menu || xbox_lastDevice < 0)
		return;

	if ( (in_speed.state & 1) ^ (int)cl_run->value)
		speed = 2;
	else
		speed = 1;
	aspeed = cls.frametime;
	if (autosensitivity->value)
		aspeed *= (cl.refdef.fov_x/90.0);

	switch((int) xbox_stick_mode->value)
	{
	case Xbox_StickSwap:
		view = leftPos;
		move = rightPos;
		break;
	case Xbox_StickDefault:
	default:
		view = rightPos;
		move = leftPos;
		break;
	}

	Xbox_ParseThumbStick(xbox_oldstate.sThumbLX,xbox_oldstate.sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,leftPos);
	Xbox_ParseThumbStick(xbox_oldstate.sThumbRX,xbox_oldstate.sThumbRY,XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,rightPos);

	cmd->forwardmove += move[1] * move[2] * speed * cl_forwardspeed->value;
	cmd->sidemove += move[0] * move[2] * speed *  cl_sidespeed->value;

	cl.in_delta[PITCH] -= (view[1] * view[2] * pitchInvert * xbox_pitch_sensitivity->value) * aspeed * cl_pitchspeed->value;

	cl.in_delta[YAW] -= (view[0] * view[2] * xbox_yaw_sensitivity->value) * aspeed * cl_yawspeed->value;

}

#include "../client/client.h"
#include <SDL.h>
#include "in_sdlcont.h"

/*
============================================================

 360 GAMEPAD CONTROL

============================================================
*/

enum _GamepadStickModes
{
	Gamepad_StickDefault = 0, Gamepad_StickSwap
};

typedef enum _GamepadDirection {
	Gamepad_None,
	Gamepad_Up,
	Gamepad_Down,
	Gamepad_Left,
	Gamepad_Right
} dir_t;


cvar_t  *autosensitivity;
cvar_t	*gamepad_usernum;
cvar_t	*gamepad_stick_mode;
cvar_t	*gamepad_trigger_threshold;
cvar_t	*gamepad_pitch_sensitivity;
cvar_t	*gamepad_yaw_sensitivity;
cvar_t  *gamepad_stick_toggle;

static SDL_GameController *currentController;

static struct {
	int repeatCount;
	int lastSendTime;
}  gamepad_repeatstatus[K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP + 1];

Sint16 oldAxisState[SDL_CONTROLLER_AXIS_MAX];
Uint8 oldButtonState[SDL_CONTROLLER_BUTTON_MAX];

static qboolean gamepad_sticktoggle[2];

#define INITIAL_REPEAT_DELAY 220
#define REPEAT_DELAY 160

#define RIGHT_THUMB_DEADZONE 8689
#define LEFT_THUMB_DEADZONE 7849

void IN_ControllerInit (void)
{
	int i;
	cvar_t		*cv;


	// abort startup if user requests no  support
	cv = Cvar_Get ("in_initgamepad", "1", CVAR_NOSET);
	if ( !cv->value ) 
		return; 

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == 0) {
		if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER) < 0) {
			Com_Printf ("Couldn't init SDL gamepad: %s\n", SDL_GetError ());
			return;
		}
	} else if (SDL_WasInit(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER) < 0) {
			Com_Printf ("Couldn't init SDL gamepad: %s\n", SDL_GetError ());
			return;
		} 

	}
	SDL_GameControllerEventState(SDL_IGNORE);

	autosensitivity			= Cvar_Get ("autosensitivity",			"1",		CVAR_ARCHIVE);
	gamepad_yaw_sensitivity	= Cvar_Get ("gamepad_yaw_sensitivity",		"1.75",		CVAR_ARCHIVE);
	gamepad_pitch_sensitivity	= Cvar_Get ("gamepad_pitch_sensitivity",	"1.75",		CVAR_ARCHIVE);
	gamepad_trigger_threshold	= Cvar_Get ("gamepad_trigger_threshold",	"0.12",		CVAR_ARCHIVE);
	gamepad_stick_toggle		= Cvar_Get ("gamepad_stick_toggle",		"0",		CVAR_ARCHIVE);
	gamepad_stick_mode			= Cvar_Get ("gamepad_stick_mode",			"0",		CVAR_ARCHIVE);
	gamepad_usernum			= Cvar_Get ("gamepad_usernum",				"0",		CVAR_ARCHIVE);


	for (i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (SDL_IsGameController(i))
			Com_Printf("Found game controller %i named '%s'!\n", i, SDL_GameControllerNameForIndex(i));
	}

}

void Gamepad_ParseThumbStick(float LX, float LY, float deadzone, vec3_t out)
{
	//determine how far the controller is pushed
	float mag = sqrt(LX*LX + LY*LY);

	//determine the direction the controller is pushed
	if (mag > 0)
	{
		out[0] = LX / mag;
		out[1] = -LY / mag;
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

void Gamepad_ParseDirection(vec3_t dir, dir_t *out)
{
	float x = dir[0] * dir[2];
	float y = dir[1] * dir[2];
	const float sq1over2 = sqrt(0.5);
	if (dir[2] == 0.0f)
		*out = Gamepad_None;
	else if (x > sq1over2)
		*out = Gamepad_Right;
	else if (x < -sq1over2)
		*out = Gamepad_Left;
	else if (y > sq1over2)
		*out = Gamepad_Up;
	else if (y < -sq1over2)
		*out = Gamepad_Down;
	else
		*out = Gamepad_None;
	return;
}

void Gamepad_HandleRepeat(keynum_t key)
{
	int index = key - K_GAMEPAD_LSTICK_UP;
	int delay = 0;
	qboolean send = false;

	if (index < 0 || index > (K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP))
		return;
	
	if (gamepad_repeatstatus[index].repeatCount == 0)
		delay = INITIAL_REPEAT_DELAY;
	else
		delay = REPEAT_DELAY;
	
	if (cl.time > gamepad_repeatstatus[index].lastSendTime + delay || cl.time < gamepad_repeatstatus[index].lastSendTime)
		send = true;
	
	if (send)
	{
		gamepad_repeatstatus[index].lastSendTime = cl.time;
		gamepad_repeatstatus[index].repeatCount++;
		Key_Event (key, true, 0);
	}

}


void Gamepad_SendKeyup(keynum_t key)
{
	int index = key - K_GAMEPAD_LSTICK_UP;

	if (index < 0 || index > (K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP))
		return;
	gamepad_repeatstatus[index].lastSendTime = 0;
	gamepad_repeatstatus[index].repeatCount = 0;

	Key_Event (key, false, 0);
}

void IN_ControllerCommands (void)
{
	Sint16 newAxisState[SDL_CONTROLLER_AXIS_MAX];
	Uint8 newButtonState[SDL_CONTROLLER_BUTTON_MAX];

	unsigned int i = 0;
	
	unsigned int triggerThreshold = ClampCvar(0.04,0.96,gamepad_trigger_threshold->value) * 255.0f;


	if (currentController && SDL_GameControllerGetAttached(currentController) != SDL_TRUE)
	{
		char buffer[128];
		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"%s disconnected",SDL_GameControllerName(currentController));
		SCR_CenterPrint(buffer);

		currentController = NULL;
	}

	if (!currentController)
	{
		int i;

		for (i = 0; i < SDL_NumJoysticks(); i++)
		{
			if (SDL_IsGameController(i))
			{
				currentController = SDL_GameControllerOpen(i);
				if (currentController)
				{
					char buffer[128];
					memset(buffer,0,sizeof(buffer));
					sprintf(buffer,"%s connected",SDL_GameControllerName(currentController));
					SCR_CenterPrint(buffer);
					break;
				}

			}

		}

		memset(newAxisState,0,sizeof(newAxisState));
		memset(newButtonState,0,sizeof(newButtonState));
	}

	if (currentController)
	{
		SDL_GameControllerUpdate();


		for (i = 0 ; i < SDL_CONTROLLER_AXIS_MAX; i++)
		{
			newAxisState[i] = SDL_GameControllerGetAxis(currentController,(SDL_GameControllerAxis) i);
		}

		for (i = 0 ; i < SDL_CONTROLLER_BUTTON_MAX; i++)
		{
			newButtonState[i] = SDL_GameControllerGetButton(currentController,(SDL_GameControllerButton) i);
		}
	} 

	if (cls.key_dest == key_menu)
	{
		vec3_t oldStick, newStick;
		dir_t oldDir, newDir;
		unsigned int j = 0;
		Gamepad_ParseThumbStick(newAxisState[SDL_CONTROLLER_AXIS_LEFTX],newAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,newStick);
		Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_LEFTX],oldAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,oldStick);

		Gamepad_ParseDirection(newStick,&newDir);
		Gamepad_ParseDirection(oldStick,&oldDir);

		for (j = 0; j < 4; j++)
		{
			if (newDir == Gamepad_Up + j)
				Gamepad_HandleRepeat(K_GAMEPAD_LSTICK_UP + j);
			if (newDir != Gamepad_Up + j && oldDir == Gamepad_Up + j)
				Gamepad_SendKeyup(K_GAMEPAD_LSTICK_UP + j);
		}

		Gamepad_ParseThumbStick(newAxisState[SDL_CONTROLLER_AXIS_RIGHTX],newAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,newStick);
		Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_RIGHTX],oldAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,oldStick);


		Gamepad_ParseDirection(newStick,&newDir);
		Gamepad_ParseDirection(oldStick,&oldDir);

		for (j = 0; j < 4; j++)
		{
			if (newDir == Gamepad_Up + j)
				Gamepad_HandleRepeat(K_GAMEPAD_RSTICK_UP + j);
			if (newDir != Gamepad_Up + j && oldDir == Gamepad_Up + j)
				Gamepad_SendKeyup(K_GAMEPAD_RSTICK_UP + j);
		}

		for (j = SDL_CONTROLLER_BUTTON_DPAD_UP ; j <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT ; j++)
		{
			unsigned int k = j - SDL_CONTROLLER_BUTTON_DPAD_UP;

			if (newButtonState[j])
				Gamepad_HandleRepeat(K_GAMEPAD_UP + k);

			if (!(newButtonState[j]) && (oldButtonState[j]))
				Gamepad_SendKeyup(K_GAMEPAD_UP + k);
		}
	}  else {
		unsigned int j;
		for (j = SDL_CONTROLLER_BUTTON_DPAD_UP ; j <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT ; j++)
		{
			unsigned int k = j - SDL_CONTROLLER_BUTTON_DPAD_UP;


			if ((newButtonState[j]) && (!oldButtonState[j]))
				Key_Event(K_GAMEPAD_UP + k, true, 0);

			if (!(newButtonState[j]) && (oldButtonState[j]))
				Key_Event(K_GAMEPAD_UP + k, false, 0);
		}

	}



	for (i = SDL_CONTROLLER_BUTTON_A ; i <= SDL_CONTROLLER_BUTTON_START ; i++)
	{
		int j = i - SDL_CONTROLLER_BUTTON_A;
		if ((newButtonState[i]) && (!oldButtonState[i]))
			Key_Event (K_GAMEPAD_A + j, true, 0);

		if (!(newButtonState[i]) && (oldButtonState[i]))
			Key_Event (K_GAMEPAD_A + j, false, 0);
	}



	if (!gamepad_stick_toggle->value || cls.key_dest == key_menu)
	{
		memset(gamepad_sticktoggle,0,sizeof(gamepad_sticktoggle));

		for (i = SDL_CONTROLLER_BUTTON_LEFTSTICK ; i <= SDL_CONTROLLER_BUTTON_RIGHTSTICK ; i++)
		{
			int j = i - SDL_CONTROLLER_BUTTON_LEFTSTICK;
			if ((newButtonState[i]) && (!oldButtonState[i]))
				Key_Event (K_GAMEPAD_LEFT_STICK + j, true, 0);

			if (!(newButtonState[i]) && (oldButtonState[i]))
				Key_Event (K_GAMEPAD_LEFT_STICK + j, false, 0);
		}

	} else {

		for (i = SDL_CONTROLLER_BUTTON_LEFTSTICK ; i <= SDL_CONTROLLER_BUTTON_RIGHTSTICK ; i++)
		{
			int j = i - SDL_CONTROLLER_BUTTON_LEFTSTICK;
			if ((newButtonState[i]) && (!oldButtonState[i]))
			{
				gamepad_sticktoggle[j] = !gamepad_sticktoggle[j];
				Key_Event (K_GAMEPAD_LEFT_STICK + j, gamepad_sticktoggle[j], 0);
			}

		}

	}


	for (i = SDL_CONTROLLER_BUTTON_LEFTSHOULDER ; i <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ; i++)
	{
		int j = i - SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
		if ((newButtonState[i]) && (!oldButtonState[i]))
			Key_Event (K_GAMEPAD_LS + j, true, 0);

		if (!(newButtonState[i]) && (oldButtonState[i]))
			Key_Event (K_GAMEPAD_LS + j, false, 0);
	}



	/* left trigger */

	for (i = SDL_CONTROLLER_AXIS_TRIGGERLEFT ; i <= SDL_CONTROLLER_AXIS_TRIGGERRIGHT ; i++)
	{
		int j = i - SDL_CONTROLLER_AXIS_TRIGGERLEFT;
		if ((newAxisState[i] >= triggerThreshold) && (oldAxisState[i] < triggerThreshold))
			Key_Event (K_GAMEPAD_LT + j, true, 0);

		if ((newAxisState[i] < triggerThreshold) && (oldAxisState[i] >= triggerThreshold))
			Key_Event (K_GAMEPAD_LT + j, false, 0);
	}

	memcpy(oldButtonState,newButtonState,sizeof(oldButtonState));
	memcpy(oldAxisState,newAxisState,sizeof(oldAxisState));
}

void IN_ControllerMove (usercmd_t *cmd)
{
	vec3_t leftPos,rightPos;
	vec_t *view, *move;
	float speed, aspeed;

	float pitchInvert = (m_pitch->value < 0.0) ? -1 : 1;

	if (cls.key_dest == key_menu) 
		return;


	if ( (in_speed.state & 1) ^ (int)cl_run->value)
		speed = 2;
	else
		speed = 1;
	aspeed = cls.frametime;
	if (autosensitivity->value)
		aspeed *= (cl.refdef.fov_x/90.0);

	switch((int) gamepad_stick_mode->value)
	{
	case Gamepad_StickSwap:
		view = leftPos;
		move = rightPos;
		break;
	case Gamepad_StickDefault:
	default:
		view = rightPos;
		move = leftPos;
		break;
	}
	Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_RIGHTX],oldAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,rightPos);
	Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_LEFTX],oldAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,leftPos);

	cmd->forwardmove += move[1] * move[2] * speed * cl_forwardspeed->value;
	cmd->sidemove += move[0] * move[2] * speed *  cl_sidespeed->value;

	cl.in_delta[PITCH] -= (view[1] * view[2] * pitchInvert * gamepad_pitch_sensitivity->value) * aspeed * cl_pitchspeed->value;

	cl.in_delta[YAW] -= (view[0] * view[2] * gamepad_yaw_sensitivity->value) * aspeed * cl_yawspeed->value;
}

#include "../../client/client.h"
#include <SDL.h>
#include "in_sdlcont.h"

/*
============================================================

SDL GAMEPAD CONTROL

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


static cvar_t  *autosensitivity;
static cvar_t	*gamepad_usernum;
static cvar_t	*gamepad_stick_mode;
static cvar_t	*gamepad_trigger_threshold;
static cvar_t	*gamepad_pitch_sensitivity;
static cvar_t	*gamepad_yaw_sensitivity;
static cvar_t  *gamepad_stick_toggle;

static SDL_GameController *currentController;

static struct {
	int32_t repeatCount;
	int32_t lastSendTime;
}  gamepad_repeatstatus[K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP + 1];

static int16_t oldAxisState[SDL_CONTROLLER_AXIS_MAX];
static uint8_t oldButtonState[SDL_CONTROLLER_BUTTON_MAX];

static qboolean gamepad_sticktoggle[2];

#define INITIAL_REPEAT_DELAY 220
#define REPEAT_DELAY 160

#define RIGHT_THUMB_DEADZONE 8689
#define LEFT_THUMB_DEADZONE 7849

void IN_ControllerInit (void)
{
	int32_t i;
	int32_t size;
	char *name[] = {"gamecontrollerdb.txt","controllers.cfg",0};
	char *p = NULL;
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


	// load additional controller bindings
	// this will attempt to load them from 'controllers.cfg' and 'gamecontrollerdb.txt'
	// by default, this will ship with a copy of 'controllers.cfg' in the vrquake2.pk3 file
	// to add new bindings, the user will have to add a copy of the gamecontrollerdb.txt to the baseq2 dir
	// see https://github.com/gabomdq/SDL_GameControllerDB for info
	
	for (i = 0; name[i] != NULL ; i++)
	{
		void *buffer = NULL;
		SDL_RWops *rw;
		p = name[i];
		Com_Printf("Loading controller mappings from '%s':\n",p);
		size = FS_LoadFile(p,&buffer);		
		
		if (buffer)
		{
			int results;
			rw = SDL_RWFromMem(buffer,size);
			results = SDL_GameControllerAddMappingsFromRW(rw, 1);	
			if (results >= 0)
			{
				Com_Printf("...loaded %d additional mappings\n",results);
			} else {
				Com_Printf("...error: %s\n",SDL_GetError());
			}
			Z_Free(buffer);
		}
	}
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
	int32_t index = key - K_GAMEPAD_LSTICK_UP;
	int32_t delay = 0;
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
	int32_t index = key - K_GAMEPAD_LSTICK_UP;

	if (index < 0 || index > (K_GAMEPAD_RIGHT - K_GAMEPAD_LSTICK_UP))
		return;
	gamepad_repeatstatus[index].lastSendTime = 0;
	gamepad_repeatstatus[index].repeatCount = 0;

	Key_Event (key, false, 0);
}

void IN_ControllerCommands (void)
{
	int16_t newAxisState[SDL_CONTROLLER_AXIS_MAX];
	uint8_t newButtonState[SDL_CONTROLLER_BUTTON_MAX];

	uint32_t i = 0;

	int16_t triggerThreshold = ClampCvar(0.04,0.96,gamepad_trigger_threshold->value) * 255.0f;


	if (currentController && SDL_GameControllerGetAttached(currentController) != SDL_TRUE)
	{
		char buffer[128];
		SDL_memset(buffer,0,sizeof(buffer));
		SDL_snprintf(buffer,sizeof(buffer),"%s disconnected",SDL_GameControllerName(currentController));
		SCR_CenterPrint(buffer);

		currentController = NULL;
	}

	if (!currentController)
	{
		int32_t j;

		for (j = 0; j < SDL_NumJoysticks(); j++)
		{
			if (SDL_IsGameController(j))
			{
				currentController = SDL_GameControllerOpen(j);
				if (currentController)
				{
					char buffer[128];
					SDL_memset(buffer,0,sizeof(buffer));
					SDL_snprintf(buffer,sizeof(buffer),"%s connected",SDL_GameControllerName(currentController));
					SCR_CenterPrint(buffer);
					break;
				}

			}

		}

		SDL_memset(newAxisState,0,sizeof(newAxisState));
		SDL_memset(newButtonState,0,sizeof(newButtonState));
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
		uint32_t j = 0;
		Gamepad_ParseThumbStick(newAxisState[SDL_CONTROLLER_AXIS_LEFTX],newAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,newStick);
		Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_LEFTX],oldAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,oldStick);

		Gamepad_ParseDirection(newStick,&newDir);
		Gamepad_ParseDirection(oldStick,&oldDir);

		for (j = 0; j < 4; j++)
		{
			if (newDir == Gamepad_Up + j)
				Gamepad_HandleRepeat((keynum_t)(K_GAMEPAD_LSTICK_UP + j));
			if (newDir != Gamepad_Up + j && oldDir == Gamepad_Up + j)
				Gamepad_SendKeyup((keynum_t)(K_GAMEPAD_LSTICK_UP + j));
		}

		Gamepad_ParseThumbStick(newAxisState[SDL_CONTROLLER_AXIS_RIGHTX],newAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,newStick);
		Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_RIGHTX],oldAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,oldStick);


		Gamepad_ParseDirection(newStick,&newDir);
		Gamepad_ParseDirection(oldStick,&oldDir);

		for (j = 0; j < 4; j++)
		{
			if (newDir == Gamepad_Up + j)
				Gamepad_HandleRepeat((keynum_t)(K_GAMEPAD_RSTICK_UP + j));
			if (newDir != Gamepad_Up + j && oldDir == Gamepad_Up + j)
				Gamepad_SendKeyup((keynum_t)(K_GAMEPAD_RSTICK_UP + j));
		}

		for (j = SDL_CONTROLLER_BUTTON_DPAD_UP ; j <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT ; j++)
		{
			uint32_t k = j - SDL_CONTROLLER_BUTTON_DPAD_UP;

			if (newButtonState[j])
				Gamepad_HandleRepeat((keynum_t)(K_GAMEPAD_UP + k));

			if (!(newButtonState[j]) && (oldButtonState[j]))
				Gamepad_SendKeyup((keynum_t)(K_GAMEPAD_UP + k));
		}
	}  else {
		uint32_t j;
		for (j = SDL_CONTROLLER_BUTTON_DPAD_UP ; j <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT ; j++)
		{
			uint32_t k = j - SDL_CONTROLLER_BUTTON_DPAD_UP;


			if ((newButtonState[j]) && (!oldButtonState[j]))
				Key_Event(K_GAMEPAD_UP + k, true, 0);

			if (!(newButtonState[j]) && (oldButtonState[j]))
				Key_Event(K_GAMEPAD_UP + k, false, 0);
		}

	}



	for (i = SDL_CONTROLLER_BUTTON_A ; i <= SDL_CONTROLLER_BUTTON_START ; i++)
	{
		int32_t j = i - SDL_CONTROLLER_BUTTON_A;
		if ((newButtonState[i]) && (!oldButtonState[i]))
			Key_Event (K_GAMEPAD_A + j, true, 0);

		if (!(newButtonState[i]) && (oldButtonState[i]))
			Key_Event (K_GAMEPAD_A + j, false, 0);
	}



	if (!gamepad_stick_toggle->value || cls.key_dest == key_menu)
	{
		SDL_memset(gamepad_sticktoggle,0,sizeof(gamepad_sticktoggle));

		for (i = SDL_CONTROLLER_BUTTON_LEFTSTICK ; i <= SDL_CONTROLLER_BUTTON_RIGHTSTICK ; i++)
		{
			int32_t j = i - SDL_CONTROLLER_BUTTON_LEFTSTICK;
			if ((newButtonState[i]) && (!oldButtonState[i]))
				Key_Event (K_GAMEPAD_LEFT_STICK + j, true, 0);

			if (!(newButtonState[i]) && (oldButtonState[i]))
				Key_Event (K_GAMEPAD_LEFT_STICK + j, false, 0);
		}

	} else {

		for (i = SDL_CONTROLLER_BUTTON_LEFTSTICK ; i <= SDL_CONTROLLER_BUTTON_RIGHTSTICK ; i++)
		{
			int32_t j = i - SDL_CONTROLLER_BUTTON_LEFTSTICK;
			if ((newButtonState[i]) && (!oldButtonState[i]))
			{
				gamepad_sticktoggle[j] = !gamepad_sticktoggle[j];
				Key_Event (K_GAMEPAD_LEFT_STICK + j, gamepad_sticktoggle[j], 0);
			}

		}

	}


	for (i = SDL_CONTROLLER_BUTTON_LEFTSHOULDER ; i <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ; i++)
	{
		int32_t j = i - SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
		if ((newButtonState[i]) && (!oldButtonState[i]))
			Key_Event (K_GAMEPAD_LS + j, true, 0);

		if (!(newButtonState[i]) && (oldButtonState[i]))
			Key_Event (K_GAMEPAD_LS + j, false, 0);
	}



	/* left trigger */

	for (i = SDL_CONTROLLER_AXIS_TRIGGERLEFT ; i <= SDL_CONTROLLER_AXIS_TRIGGERRIGHT ; i++)
	{
		int32_t j = i - SDL_CONTROLLER_AXIS_TRIGGERLEFT;
		if ((newAxisState[i] >= triggerThreshold) && (oldAxisState[i] < triggerThreshold))
			Key_Event (K_GAMEPAD_LT + j, true, 0);

		if ((newAxisState[i] < triggerThreshold) && (oldAxisState[i] >= triggerThreshold))
			Key_Event (K_GAMEPAD_LT + j, false, 0);
	}

	SDL_memcpy(oldButtonState,newButtonState,sizeof(oldButtonState));
	SDL_memcpy(oldAxisState,newAxisState,sizeof(oldAxisState));
}

void IN_ControllerMove (usercmd_t *cmd)
{
	vec3_t leftPos,rightPos;
	Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_RIGHTX],oldAxisState[SDL_CONTROLLER_AXIS_RIGHTY],RIGHT_THUMB_DEADZONE,rightPos);
	Gamepad_ParseThumbStick(oldAxisState[SDL_CONTROLLER_AXIS_LEFTX],oldAxisState[SDL_CONTROLLER_AXIS_LEFTY],LEFT_THUMB_DEADZONE,leftPos);

	ThumbstickMove(cmd, leftPos, rightPos, false);
}

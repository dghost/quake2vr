/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_sdl2.c -- SDL2 mouse support
// 02/28/2014 - inital version - dghost

#include "../client/client.h"
#include "../sdl2/sdl2quake.h"
#include "../vr/vr.h"
#include "in_sdlcont.h"

extern	unsigned	sys_msg_time;

enum _ControllerType
{
	ControllerNone = 0, ControllerSDLController
};


cvar_t	*m_noaccel; //sul
cvar_t	*in_mouse;
cvar_t	*in_controller;
cvar_t	*autosensitivity;


extern cursor_t cursor;

qboolean	in_appactive;

/*
============================================================

MOUSE CONTROL

============================================================
*/

// mouse variables
cvar_t	*m_filter;

qboolean	mlooking;

void IN_MLookDown (void) { mlooking = true; }
void IN_MLookUp (void) {
	mlooking = false;
	if (!freelook->value && lookspring->value)
		IN_CenterView ();
}

Sint32			mouse_buttons;
Sint32			mouse_oldbuttonstate;
Sint32			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

Sint32			window_center_x, window_center_y;

qboolean	mouseactive;	// false when not focus app

qboolean	mouseinitialized;

qboolean useRelativeMode;

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void)
{
	
	Sint32		width, height;

	if (!mouseinitialized)
		return;
	if (!in_mouse->value)
	{
		mouseactive = false;
		return;
	}
	if (mouseactive)
		return;

	mouseactive = true;

	/*
	if (m_noaccel->value) 
		newmouseparms[2]=0; //sul XP fix?
	else 
		newmouseparms[2]=1;
		*/
	SDL_GetWindowSize(mainWindow,&width,&height);
	//SDL_SetWindowGrab(mainWindow,SDL_TRUE);

	SDL_ShowCursor(0);
	window_center_x = width/2;
	window_center_y = height/2;
	old_mouse_x = 0;
	old_mouse_y = 0;
	SDL_WarpMouseInWindow(mainWindow,window_center_x, window_center_y);
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void)
{
	if (!mouseinitialized)
		return;
	if (!mouseactive)
		return;


	mouseactive = false;

	//SDL_ShowCursor(1);
	//SDL_SetWindowGrab(mainWindow,SDL_FALSE);
}


/*
===========
IN_StartupMouse
===========
*/
void UI_RefreshCursorMenu (void);
void UI_RefreshCursorLink (void);
//void MW_Set_Hook (void);
//void MW_Shutdown (void);
void IN_StartupMouse (void)
{
	cvar_t		*cv;
	cv = Cvar_Get ("in_initmouse", "1", CVAR_NOSET);
	if ( !cv->value ) 
		return; 

	// Knightmare- added Psychospaz's menu mouse support
	UI_RefreshCursorMenu();
	UI_RefreshCursorLink();

	cursor.mouseaction = false;

	mouseinitialized = true;
	//MW_Set_Hook(); 	// Logitech mouse support
	mouse_buttons = 5; // was 3



}

/*
===========
IN_MouseEvent
===========
*/
void UI_Think_MouseCursor (void);
void IN_MouseEvent (Sint32 mstate)
{
	Sint32		i;

	if (!mouseinitialized)
		return;

	// perform button actions
	for (i=0 ; i<mouse_buttons ; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true, sys_msg_time);
		}

		if ( !(mstate & (1<<i)) &&
			(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, false, sys_msg_time);
		}
	}	

	//set menu cursor buttons
	if (cls.key_dest == key_menu)
	{
		Sint32 multiclicktime = 750;
		Sint32 max = mouse_buttons;
		if (max > MENU_CURSOR_BUTTON_MAX) max = MENU_CURSOR_BUTTON_MAX;

		for (i=0 ; i<max ; i++)
		{
			if ( (mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)))
			{	//mouse press down
				if (sys_msg_time-cursor.buttontime[i] < multiclicktime)
					cursor.buttonclicks[i] += 1;
				else
					cursor.buttonclicks[i] = 1;

				if (cursor.buttonclicks[i]>max)
					cursor.buttonclicks[i] = max;

				cursor.buttontime[i] = sys_msg_time;

				cursor.buttondown[i] = true;
				cursor.buttonused[i] = false;
				cursor.mouseaction = true;
			}
			else if ( !(mstate & (1<<i)) &&	(mouse_oldbuttonstate & (1<<i)) )
			{	//mouse let go
				cursor.buttondown[i] = false;
				cursor.buttonused[i] = false;
				cursor.mouseaction = true;
			}
		}			
	}	

	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	Sint32		mx, my;

	if (!autosensitivity)
		autosensitivity = Cvar_Get ("autosensitivity", "1", CVAR_ARCHIVE);

	if (!mouseactive)
		return;

	// find mouse movement
	SDL_GetMouseState(&mx,&my);
	mx -= window_center_x;
	my -= window_center_y;

#if 0
	if (!mx && !my)
		return;
#endif

	if (m_filter->value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	//now to set the menu cursor
	if (cls.key_dest == key_menu)
	{
		cursor.oldx = cursor.x;
		cursor.oldy = cursor.y;

		cursor.x += mouse_x * menu_sensitivity->value;
		cursor.y += mouse_y * menu_sensitivity->value;

		if (cursor.x!=cursor.oldx || cursor.y!=cursor.oldy)
			cursor.mouseaction = true;

		if (cursor.x < 0) cursor.x = 0;
		if (cursor.x > viddef.width) cursor.x = viddef.width;
		if (cursor.y < 0) cursor.y = 0;
		if (cursor.y > viddef.height) cursor.y = viddef.height;
	}
	else
	{
		cursor.oldx = 0;
		cursor.oldy = 0;

		//psychospaz - zooming in preserves sensitivity
		if (autosensitivity->value)
		{
			mouse_x *= sensitivity->value * (cl.refdef.fov_x/90.0);
			mouse_y *= sensitivity->value * (cl.refdef.fov_y/90.0);
		}
		else
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}

		// add mouse X/Y movement to cmd
		if ( (in_strafe.state & 1) || (lookstrafe->value && mlooking ))
			cmd->sidemove += m_side->value * mouse_x;
		else
			cl.in_delta[YAW] -= m_yaw->value * mouse_x;

		if ( (mlooking || freelook->value) && !(in_strafe.state & 1))
		{
			cl.in_delta[PITCH] += m_pitch->value * mouse_y;
		}
		else
		{
			cmd->forwardmove -= m_forward->value * mouse_y;
		}
	}

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SDL_WarpMouseInWindow (mainWindow, window_center_x, window_center_y);
}


/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// mouse variables
	autosensitivity			= Cvar_Get ("autosensitivity",			"1",		CVAR_ARCHIVE);
	m_noaccel				= Cvar_Get ("m_noaccel",				"0",		CVAR_ARCHIVE); //sul  enables mouse acceleration XP fix?
	m_filter				= Cvar_Get ("m_filter",					"0",		0);
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE);

	// joystick variables
	in_controller			= Cvar_Get ("in_controller",			"0",		CVAR_ARCHIVE);

	// centering
	v_centermove			= Cvar_Get ("v_centermove",				"0.15",		0);
	v_centerspeed			= Cvar_Get ("v_centerspeed",			"500",		0);

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);


	IN_StartupMouse ();
	IN_ControllerInit ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
	//MW_Shutdown(); // Logitech mouse support
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{
	in_appactive = active;
	mouseactive = !active;		// force a new window check or turn off
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!mouseinitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse ();
		return;
	}

	//	if ( !cl.refresh_prepped
	//		|| cls.key_dest == key_console
	//		|| cls.key_dest == key_menu)
	//Knightmare- added Psychospaz's mouse menu support
	if ( (!cl.refresh_prepped && cls.key_dest != key_menu) || cls.consoleActive) //mouse used in menus...
	{
		// temporarily deactivate if in fullscreen
		if (Cvar_VariableValue ("vid_fullscreen") == 0)
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	if (!ActiveApp)
		return;


	IN_MouseMove (cmd);

	// Knightmare- added Psychospaz's mouse support
	if (cls.key_dest == key_menu && !cls.consoleActive) // Knightmare added
		UI_Think_MouseCursor();


	switch((Sint32) in_controller->value)
	{
	case ControllerSDLController:
		IN_ControllerMove (cmd);
		return;
	default:
		IN_ControllerMove (cmd);
		return;
	}

}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{
	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}



/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
	switch((Sint32) in_controller->value)
	{
	case ControllerSDLController:
		IN_ControllerCommands ();
		return;
	default:
		IN_ControllerCommands ();
		return;
	}
}

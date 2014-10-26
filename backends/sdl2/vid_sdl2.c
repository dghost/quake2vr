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

// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.
#include <assert.h>
#include <float.h>

#include "../../client/client.h"
#include "sdl2quake.h"

//PGM
int32_t	vidref_val;
//PGM

// Console variables that we need to access from this module
cvar_t		*scanforcd; // Knightmare- just here to enable command line option without error
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_xpos;			// X coordinate of window position
cvar_t		*vid_ypos;			// Y coordinate of window position
cvar_t		*vid_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
qboolean	kmgl_active = 0;

static qboolean s_alttab_disabled;

extern	unsigned	sys_msg_time;


/*
==========================================================================

DLL GLUE

==========================================================================
*/

#define	MAXPRINTMSG	8192 // was 4096
void VID_Printf (int32_t print_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;

	va_start (argptr, fmt);
	//	vsprintf (msg, fmt, argptr);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if (print_level == PRINT_ALL)
	{
		Com_Printf ("%s", msg);
	}
	else if ( print_level == PRINT_DEVELOPER )
	{
		Com_DPrintf ("%s", msg);
	}
	else if ( print_level == PRINT_ALERT )
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"PRINT_ALERT",msg,NULL);
#ifdef _WIN32
		OutputDebugString( msg );
#endif
	}
}

void VID_Error (int32_t err_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;

	va_start (argptr, fmt);
	//	vsprintf (msg, fmt,argptr);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}

/*
=======
MapSDLKey

Map from SDL to quake keynums
=======
*/
int32_t MapSDLKey (SDL_Keysym key)
{
	int32_t result = 0;
	if (key.sym > 32 && key.sym < 127)
	{
		result = key.sym;
	} else 
	{
		switch (key.scancode)
		{
		case SDL_SCANCODE_PAGEUP:	 result = K_PGUP; break;
		case SDL_SCANCODE_PAGEDOWN: result = K_PGDN; break;
		case SDL_SCANCODE_HOME: result = K_HOME; break;
		case SDL_SCANCODE_END:  result = K_END; break;
		case SDL_SCANCODE_LEFT:	 result = K_LEFTARROW; break;
		case SDL_SCANCODE_RIGHT:	result = K_RIGHTARROW;		break;
		case SDL_SCANCODE_DOWN:	 result = K_DOWNARROW; break;
		case SDL_SCANCODE_UP:		 result = K_UPARROW;	 break;
		case SDL_SCANCODE_ESCAPE: result = K_ESCAPE;		break;
		case SDL_SCANCODE_RETURN: result = K_ENTER;		 break;
		case SDL_SCANCODE_RETURN2: result = K_ENTER;		 break;
		case SDL_SCANCODE_TAB:		result = K_TAB;			 break;
		case SDL_SCANCODE_F1:		 result = K_F1;				break;
		case SDL_SCANCODE_F2:		 result = K_F2;				break;
		case SDL_SCANCODE_F3:		 result = K_F3;				break;
		case SDL_SCANCODE_F4:		 result = K_F4;				break;
		case SDL_SCANCODE_F5:		 result = K_F5;				break;
		case SDL_SCANCODE_F6:		 result = K_F6;				break;
		case SDL_SCANCODE_F7:		 result = K_F7;				break;
		case SDL_SCANCODE_F8:		 result = K_F8;				break;
		case SDL_SCANCODE_F9:		 result = K_F9;				break;
		case SDL_SCANCODE_F10:		result = K_F10;			 break;
		case SDL_SCANCODE_F11:		result = K_F11;			 break;
		case SDL_SCANCODE_F12:		result = K_F12;			 break;
		case SDL_SCANCODE_BACKSPACE: result = K_BACKSPACE; break;
		case SDL_SCANCODE_DELETE: result = K_DEL; break;
		case SDL_SCANCODE_PAUSE:	result = K_PAUSE;		 break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:	result = K_SHIFT;		break;
		case SDL_SCANCODE_LCTRL: 
		case SDL_SCANCODE_RCTRL:	result = K_CTRL;		 break;
		case SDL_SCANCODE_LALT:	
		case SDL_SCANCODE_LGUI: 
		case SDL_SCANCODE_RALT:	
		case SDL_SCANCODE_RGUI: result = K_ALT;			break;
		case SDL_SCANCODE_SPACE: result = K_SPACE; break;

			//		case XK_KP_Begin: result = K_KP_5;	break;

		case SDL_SCANCODE_INSERT:result = K_INS; break;
			//		case XK_KP_Insert: result = K_KP_INS; break;

		case SDL_SCANCODE_KP_MULTIPLY: result = '*'; break;
		case SDL_SCANCODE_KP_PLUS:  result = K_KP_PLUS; break;
		case SDL_SCANCODE_KP_MINUS: result = K_KP_MINUS; break;
		case SDL_SCANCODE_KP_DIVIDE: result = K_KP_SLASH; break;
		case SDL_SCANCODE_KP_1: result = K_KP_END; break;
		case SDL_SCANCODE_KP_2: result = K_KP_DOWNARROW; break;
		case SDL_SCANCODE_KP_3: result = K_KP_PGDN; break;
		case SDL_SCANCODE_KP_4: result = K_KP_LEFTARROW; break;
		case SDL_SCANCODE_KP_5: result = K_KP_5; break;
		case SDL_SCANCODE_KP_6: result = K_KP_RIGHTARROW; break;
		case SDL_SCANCODE_KP_7: result = K_KP_HOME; break;
		case SDL_SCANCODE_KP_8: result = K_KP_UPARROW; break;
		case SDL_SCANCODE_KP_9: result = K_KP_PGUP; break;
		case SDL_SCANCODE_KP_0: result = K_KP_INS; break;
		case SDL_SCANCODE_KP_PERIOD: result = K_KP_DEL; break;
		case SDL_SCANCODE_KP_ENTER: result = K_KP_ENTER;	break;

		}

	}

	return result;
}

void AppActivate(SDL_bool fActive, SDL_bool minimize)
{
	Minimized = (SDL_bool) !!minimize;

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if (fActive && !Minimized)
		ActiveApp = SDL_TRUE;
	else
		ActiveApp = SDL_FALSE;

	if (!RelativeMouse)
	{
		SDL_SetWindowGrab(mainWindow,ActiveApp);
		SDL_ShowCursor(!ActiveApp);
	} else {
		SDL_SetRelativeMouseMode(ActiveApp);
	}
	IN_Activate ((qboolean) !!ActiveApp);
	S_Activate ((qboolean) !!ActiveApp);
}


/*
====================
SDL_ProcEvent

main window procedure
====================
*/
void SDL_ProcEvent (SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_MOUSEWHEEL:
		if (event->wheel.y > 0)
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		} else if (event->wheel.y < 0)
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
		break;

	case SDL_QUIT:
		Com_Quit ();
		break;

	case SDL_WINDOWEVENT:
		if (event->window.windowID != mainWindowID)
			break;
		switch (event->window.event)
		{
		case SDL_WINDOWEVENT_RESTORED:
		case SDL_WINDOWEVENT_SHOWN:
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			AppActivate(true,false);

			if ( kmgl_active )
				GLimp_AppActivate ( true );
			break;

		case SDL_WINDOWEVENT_MINIMIZED:
			AppActivate(false,true);
			if ( kmgl_active )
				GLimp_AppActivate ( false );
			break;
		case SDL_WINDOWEVENT_HIDDEN:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			AppActivate(false,false);
			if ( kmgl_active )
				GLimp_AppActivate ( false );
			break;
		case SDL_WINDOWEVENT_MOVED:
			if (!vid_fullscreen->value && !(vr_enabled->value && vr_force_fullscreen->value))
			{
				int32_t xPos = event->window.data1;    // horizontal position 
				int32_t yPos = event->window.data2;    // vertical position 

				Cvar_SetValue( "vid_xpos", xPos);
				Cvar_SetValue( "vid_ypos", yPos);
				vid_xpos->modified = false;
				vid_ypos->modified = false;
				if (ActiveApp)
					IN_Activate (true);
			}
			break;
		case SDL_WINDOWEVENT_RESIZED:
			break;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		{
			static int32_t mask = 0;			
			int32_t temp = 0;

			switch (event->button.button)
			{
			case SDL_BUTTON_LEFT:
				temp |= 1;
				break;
			case SDL_BUTTON_RIGHT:
				temp = 2;
				break;
			case SDL_BUTTON_MIDDLE:
				temp = 4;
				break;
			case SDL_BUTTON_X1:
				temp = 8;
				break;
			case SDL_BUTTON_X2:
				temp = 16;
				break;
			}
			if (event->button.type == SDL_MOUSEBUTTONDOWN)
				mask |= temp;
			else
				mask ^= temp;

			IN_MouseEvent (mask);
		}
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		// todo - trap alt-enter
		if (event->key.windowID != mainWindowID)
			break;
		{
			int32_t key = MapSDLKey(event->key.keysym);
			if (key > 0)
				Key_Event( key ,(qboolean) (event->key.type == SDL_KEYDOWN), sys_msg_time);
		}
		break;
	}

}

int altEnterFilter(void* unused, SDL_Event *event)
{
	if ( event->type == SDL_KEYDOWN ) {
		if ( (event->key.keysym.sym == SDLK_RETURN) &&
                     (event->key.keysym.mod & KMOD_ALT) )
		{
			Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
			return(0);
		}
	}
	return(1);
}



/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	vid_ref->modified = true;
}

void VID_Front_f( void )
{
	SDL_RaiseWindow(mainWindow);
#ifdef _WIN32
	if((vid_fullscreen->value || (vr_enabled->value && vr_force_fullscreen->value)) && mainWindowInfo.subsystem == SDL_SYSWM_WINDOWS)
	{
		SetWindowLong( mainWindowInfo.info.win.window, GWL_EXSTYLE, WS_EX_TOPMOST );
		SetForegroundWindow(mainWindowInfo.info.win.window);
	}
#endif
}

/*
** VID_GetModeInfo
*/

/*
typedef struct vidmode_s
{
const char *description;
int32_t         width, height;
int32_t         mode;
} vidmode_t;

// Knightmare- added 1280x1024, 1400x1050, 856x480, 1024x480 modes
vidmode_t vid_modes[] =
{
#include "../qcommon/vid_modes.h"
};

*/

/*
==============
VID_UpdateWindowPos
==============
*/
void VID_UpdateWindowPos ( int32_t x, int32_t y )
{
	SDL_SetWindowPosition(mainWindow,x,y);
	//SDL_SetWindowSize(mainWindow,viddef.width,viddef.height);
}

/*
==============
VID_NewWindow
==============
*/
void VID_NewWindow ( int32_t width, int32_t height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

void VID_FreeReflib (void)
{
	kmgl_active  = false;
}


extern	decalpolys_t	*active_decals;
static qboolean reclip_decals = false;
qboolean	vid_reloading; // Knightmare- flag to not unnecessarily drop console

/*
==============
UpdateVideoRef
==============
*/
void UpdateVideoRef (void)
{
	char	reason[128];

	if ( vid_ref->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();

		// unclip decals
		if (active_decals) {
			CL_UnclipDecals();
			reclip_decals = true;
		}
	}

	vid_reloading = false;

	while (vid_ref->modified)
	{	// refresh has changed
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		if (cl.cinematictime > 0) // Knightmare added
			cls.disable_screen = false;
		else
			cls.disable_screen = true;
		vid_reloading = true;
		// end Knightmare

		//==========================
		// compacted code from VID_LoadRefresh
		//==========================
		if ( kmgl_active )
		{
			R_Shutdown();
			VID_FreeReflib ();
		}

		Com_Printf( "\n------ Renderer Initialization ------\n");

		if ( !R_Init( reason ) )
		{
			R_Shutdown();
			VID_FreeReflib ();
			Com_Error (ERR_FATAL, "Couldn't initialize OpenGL renderer!\n%s", reason);
		}

		Com_Printf( "------------------------------------\n");

		kmgl_active = true;
		//==========================
	}

	// added to close loading screen
	if (cl.refresh_prepped && vid_reloading)
		cls.disable_screen = false;

	// re-clip decals
	if (cl.refresh_prepped && reclip_decals) {
		CL_ReclipDecals();
		reclip_decals = false;
	}

	vid_reloading = false;
}


/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	//update changed vid_ref
	UpdateVideoRef ();

	// update our window position
	if ( vid_xpos->modified || vid_ypos->modified )
	{
		if (!vid_fullscreen->value && !(vr_enabled->value && vr_force_fullscreen->value))
			VID_UpdateWindowPos( vid_xpos->value, vid_ypos->value );

		vid_xpos->modified = false;
		vid_ypos->modified = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_ref = Cvar_Get ("vid_ref", "gl", CVAR_NOSET );
	vid_xpos = Cvar_Get ("vid_xpos", "0", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get ("vid_ypos", "0", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "1", CVAR_ARCHIVE);

	// Knightmare- just here to enable command line option without error
	scanforcd = Cvar_Get( "scanforcd", "0", 0 );

	// force vid_ref to gl
	// older versions of Lazarus code check only vid_ref=gl for fadein effects
	Cvar_ForceSet( "vid_ref", "gl" );
	vidref_val = VIDREF_GL;	// this is always in GL mode

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_front", VID_Front_f);

	/* Disable the 3Dfx splash screen */
	//putenv("FX_GLIDE_NO_SPLASH=0");
	
	SDL_SetEventFilter(altEnterFilter,NULL);
		
		/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( kmgl_active )
	{
		R_Shutdown ();
		VID_FreeReflib ();
	}
}



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
// sys_sdl2.h

#include "../../qcommon/qcommon.h"
#include "sdl2quake.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef WIN32
#include <direct.h>
#include <io.h>
#include <conio.h>
#else
#include <unistd.h>
#endif
#include "../../client/vr/include/vr.h"

#ifdef _WIN32
#include "../win32/resource.h"
#include "../win32/conproc.h"
#endif

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

int32_t			starttime;
SDL_bool	ActiveApp;
SDL_bool	Minimized;
SDL_bool	RelativeMouse;

#ifdef WIN32
static HANDLE		hinput, houtput;
#endif

SDL_Window *mainWindow;
uint32_t mainWindowID;
SDL_SysWMinfo mainWindowInfo;

void SDL_ProcEvent(SDL_Event *event);
uint32_t	sys_msg_time;
uint32_t	sys_frame_time;


#define	MAX_NUM_ARGVS	128
int32_t			argc;
char		*argv[MAX_NUM_ARGVS];

/*
================
Sys_Milliseconds
================
*/
int32_t	curtime;
int32_t Sys_Milliseconds (void)
{
	static int32_t		base;
	static qboolean	initialized = false;

	if (!initialized)
	{
		base = SDL_GetTicks();
		initialized = true;
	}
	curtime = SDL_GetTicks () - base;
	return curtime;
}


#ifdef _WIN32
/*
===============================================================================

DEDICATED CONSOLE

===============================================================================
*/
static char	console_text[256];
static int32_t	console_textlen;

/*
================
Sys_InitConsole
================
*/
void Sys_InitConsole (void)
{
	if (!dedicated->value)
		return;

	if (!AllocConsole ())
		Sys_Error ("Couldn't create dedicated server console");
	hinput = GetStdHandle (STD_INPUT_HANDLE);
	houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	
	// let QHOST hook in
	InitConProc (argc, argv);
}


/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int32_t		dummy;
	int32_t		ch, numread, numevents;

	if (!dedicated || !dedicated->value)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}
						break;
				}
			}
		}
	}
	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	int32_t		dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		SDL_memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, SDL_strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}

//================================================================
#else

qboolean stdin_active = true;
cvar_t *nostdout;

char *Sys_ConsoleInput(void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

#endif

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		sys_msg_time = event.common.timestamp;
		SDL_ProcEvent(&event);
	}


	// grab frame time 
	sys_frame_time = Sys_Milliseconds();	// FIXME: should this be at start?
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *sdl_cliptext;

	sdl_cliptext = SDL_GetClipboardText();
	if (!sdl_cliptext)
		return NULL;
	data = SDL_strdup(sdl_cliptext);
	SDL_free(sdl_cliptext);
	return data;
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
//	vsprintf (text, error, argptr);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);
	if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",text,NULL))
	{
		printf("Error: %s\n",text);
	}

// shut down QHOST hooks if necessary
#ifdef _WIN32
	DeinitConProc ();
#endif
	exit (1);
}


void Sys_Quit (void)
{
#ifdef _WIN32
	timeEndPeriod( 1 );
#endif
	CL_Shutdown();
	Qcommon_Shutdown ();

#ifdef _WIN32
	if (dedicated && dedicated->value)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();
#endif

	exit (0);
}

//================================================================

/*
=================
Sys_DetectCPU
Fixed to not be so clever
=================
*/
static qboolean Sys_DetectCPU (char *cpuString, int32_t maxSize)
{

#ifdef _WIN32
	DWORD dwType = REG_SZ;
	HKEY hKey = 0;
	LONG succeeded = 0;
	const char* subkey = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
	succeeded = RegOpenKey(HKEY_LOCAL_MACHINE,subkey,&hKey);
	if (succeeded != ERROR_SUCCESS)
		return false;

	succeeded = RegQueryValueEx(hKey,"ProcessorNameString",NULL,&dwType,cpuString,&maxSize);
	RegCloseKey(hKey);
	return (succeeded == ERROR_SUCCESS);

#else
	int32_t numCores = SDL_GetCPUCount();
    char buffer[512];
    memset(buffer,0,sizeof(buffer));
    snprintf(cpuString, maxSize, "Unknown %d-core processor",numCores);
	return true;
#endif

}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	char string[64];
	SDL_version compiled;
	SDL_version linked;

	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0){
		Sys_Error("SDL_Init failed!");
	}
#ifdef _WIN32
	timeBeginPeriod( 1 );
#endif

	SDL_strlcpy(string,SDL_GetPlatform(),sizeof(string));
	Com_Printf("OS: %s\n", string);
	Cvar_Get("sys_osVersion", string, CVAR_NOSET|CVAR_LATCH);

	// Detect CPU
	Com_Printf("Detecting system...\n");
	if (Sys_DetectCPU(string, sizeof(string))) {
		Com_Printf("CPU: %s\n", string);
		Cvar_Get("sys_cpuString", string, CVAR_NOSET|CVAR_LATCH);
	}
	else {
		Com_Printf("CPU: Unknown CPU found\n");
		Cvar_Get("sys_cpuString", "Unknown", CVAR_NOSET|CVAR_LATCH);
	}

    
    SDL_snprintf(string,sizeof(string),"%u",SDL_GetCPUCacheLineSize());
    Com_Printf("Cache Line Size: %s bytes\n", string);
    Cvar_Get("sys_cacheLine", string, CVAR_NOSET|CVAR_LATCH);
    
    // Get physical memory
	SDL_snprintf(string,sizeof(string),"%u",SDL_GetSystemRAM());
	Com_Printf("Memory: %s MB\n", string);
	Cvar_Get("sys_ramMegs", string, CVAR_NOSET|CVAR_LATCH);
	// end Q2E detection


    SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	SDL_snprintf(string,sizeof(string),"%d.%d.%d",linked.major, linked.minor, linked.patch);
	Cvar_Get("sys_sdlString",string,CVAR_NOSET|CVAR_LATCH);
	Com_Printf("SDL Version: %d.%d.%d.\n", linked.major, linked.minor, linked.patch);

	if (compiled.major != linked.major || compiled.minor != linked.minor || compiled.patch != linked.patch)
	{
		Com_Printf("ERROR: The version of SDL in use differs from the intended version.\n");
	}

#ifdef _WIN32
	Sys_InitConsole (); // show dedicated console, moved to function
#endif
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	SDL_ShowWindow(mainWindow);
	SDL_RaiseWindow(mainWindow);

#ifdef _WIN32
		if(mainWindowInfo.subsystem == SDL_SYSWM_WINDOWS)
			SetForegroundWindow(mainWindowInfo.info.win.window);
#endif
	

}

/*
========================================================================

GAME DLL

========================================================================
*/

static void* game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	SDL_UnloadObject (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
#ifdef _WIN32
#define LIBEXT ".dll"
#else
#define LIBEXT ".so"
#endif

void* Sys_FindLibrary(const char *dllnames[])
{
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
	int32_t i = 0;
	const char *gamename = NULL;
	void	*library = NULL;

#ifndef _DEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

	// check the current debug directory first for development purposes
#ifdef WIN32
	_getcwd (cwd, sizeof(cwd));
#else
	getcwd (cwd, sizeof(cwd));
#endif

	for (i = 0; (dllnames[i] != 0) && (library == NULL); i++)
	{
		gamename = dllnames[i];
		Com_sprintf (name, sizeof(name), "%s/%s/%s%s", cwd, debugdir, gamename,LIBEXT);
		library = SDL_LoadObject ( name );
		if (!library)
			Com_DPrintf("failed to load %s: %s\n", name, SDL_GetError());
	}
	if (library)
	{
		Com_DPrintf ("SDL_LoadObject (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s%s", cwd, gamename,LIBEXT);
		library = SDL_LoadObject ( name );
		if (library)
		{
			Com_DPrintf ("SDL_LoadObject (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				for (i = 0; dllnames[i] != 0 && (library == NULL); i++)
				{
					gamename = dllnames[i];
					Com_sprintf (name, sizeof(name), "%s/%s%s", path, gamename,LIBEXT);
					library = SDL_LoadObject (name);
					if (!library) 
						Com_DPrintf("failed to load %s: %s\n", name, SDL_GetError());
				}
				if (library)
				{
					Com_DPrintf ("SDL_LoadObject (%s)\n",name);
					break;
				}
			}
		}
	}
	return library;
}


void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
//	int32_t i = 0;
	//Knightmare- changed DLL name for better cohabitation
#ifdef KMQUAKE2_ENGINE_MOD
	static const char *dllnames[] = {"vrgamex86", "kmq2gamex86",0};
#else
	static const char *dllnames[] = {"game", "gamex86",0};
#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	game_library = Sys_FindLibrary(dllnames);
    
	if (!game_library)
		return NULL;

	GetGameAPI = (void *(*)(void*)) SDL_LoadFunction (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

//=======================================================================

/*
==================
WinMain

==================
*/

int32_t main(int32_t argc, char *argv[])
{
	int32_t				time, oldtime, newtime;
//	qboolean		cdscan = false; // Knightmare added

		
#ifdef _WIN32
	SetProcessDPIAware();
#endif


	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();
#ifndef _WIN32
	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}
#endif
    /* main window message loop */
	while (1)
	{
		SDL_Event event;
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			SDL_Delay (1);
		}

		while (SDL_PollEvent(&event))
		{
			sys_msg_time = event.common.timestamp;
			SDL_ProcEvent(&event);
		}


		// DarkOne's CPU usage fix
		while (1)
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
			if (time > 0) break;
			if (!vr_enabled->value || !vr_nosleep->value)
				SDL_Delay(0); // may also use SDL_Delay(1); to free more CPU, but it can lower your fps
		}

		/*do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);*/
		//	Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

		//	_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );

#ifdef _WIN32
		// is this necessary? Linux seems well behaved without it
//		_controlfp( _PC_24, _MCW_PC );
#endif
		Qcommon_Frame (time);

		oldtime = newtime;
	}

	// never gets here
    return 0;
}

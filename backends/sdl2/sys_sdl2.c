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
#include <direct.h>
#include <io.h>
#include <conio.h>
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

static HANDLE		hinput, houtput;

SDL_Window *mainWindow;
uint32_t mainWindowID;
SDL_SysWMinfo mainWindowInfo;

void SDL_ProcEvent(SDL_Event *event);
unsigned	sys_msg_time;
unsigned	sys_frame_time;


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
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}

//================================================================
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
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) 
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
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
	DeinitConProc ();

	exit (1);
}


void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	if (dedicated && dedicated->value)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (0);
}


//================================================================

/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD (void)
{
	static char	cddir[MAX_OSPATH];
	static qboolean	done;
	char		drive[4];
	FILE		*f;
	char		test[MAX_QPATH];
	qboolean	missionpack = false; // Knightmare added
	int32_t			i; // Knightmare added

	if (done)		// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	Com_Printf("\nScanning for game CD data path...");

	done = true;

	// Knightmare- check if mission pack gamedir is set
	for (i=0; i<argc; i++)
		if (!strcmp(argv[i], "game") && (i+1<argc))
		{
			if (!strcmp(argv[i+1], "rogue") || !strcmp(argv[i+1], "xatrix"))
				missionpack = true;
			break; // game parameter only appears once in command line
		}

	// scan the drives
	for (drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++)
	{
		// where activision put the stuff...
		if (missionpack) // Knightmare- mission packs have cinematics in different path
		{
			sprintf (cddir, "%sdata\\max", drive);
			sprintf (test, "%sdata\\patch\\quake2.exe", drive);
		}
		else
		{
			sprintf (cddir, "%sinstall\\data", drive);
			sprintf (test, "%sinstall\\data\\quake2.exe", drive);
		}
		f = fopen(test, "r");
		if (f)
		{
			fclose (f);
			if (GetDriveType (drive) == DRIVE_CDROM) {
				Com_Printf(" found %s\n", cddir);
				return cddir;
			}
		}
	}

	Com_Printf(" could not find %s on any CDROM drive!\n", test);

	cddir[0] = 0;
	
	return NULL;
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
	Q_strncpyz(cpuString, "Unknown processor with %d logical cores\n",numCores);
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

	strncpy(string,SDL_GetPlatform(),sizeof(string));
	Com_Printf("OS: %s\n", string);
	Cvar_Get("sys_osVersion", string, CVAR_NOSET|CVAR_LATCH);

	// Detect CPU
	Com_Printf("Detecting CPU... ");
	if (Sys_DetectCPU(string, sizeof(string))) {
		Com_Printf("Found %s\n", string);
		Cvar_Get("sys_cpuString", string, CVAR_NOSET|CVAR_LATCH);
	}
	else {
		Com_Printf("Unknown CPU found\n");
		Cvar_Get("sys_cpuString", "Unknown", CVAR_NOSET|CVAR_LATCH);
	}

	// Get physical memory
	sprintf(string,"%u",SDL_GetSystemRAM());
	Com_Printf("Memory: %s MB\n", string);
	Cvar_Get("sys_ramMegs", string, CVAR_NOSET|CVAR_LATCH);
	// end Q2E detection

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	sprintf(string,"%d.%d.%d",linked.major, linked.minor, linked.patch);
	Cvar_Get("sys_sdlString",string,CVAR_NOSET|CVAR_LATCH);
	Com_Printf("SDL Version: %d.%d.%d.\n", linked.major, linked.minor, linked.patch);

	if (compiled.major != linked.major || compiled.minor != linked.minor || compiled.patch != linked.patch)
	{
		Com_Printf("ERROR: The version of SDL in use differs from the intended version.\n");
	}

	Sys_InitConsole (); // show dedicated console, moved to function
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
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
	int32_t i = 0;
	//Knightmare- changed DLL name for better cohabitation
#ifdef KMQUAKE2_ENGINE_MOD
	static const char *dllnames[] = {"vrgamex86", "kmq2gamex86",0};
#else
	static const char *dllnames[] = {"game", "gamex86",0};
	#endif
	const char *gamename = NULL;
	
#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");
	
	
	
	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof(cwd));
	
	for (i = 0; (dllnames[i] != 0) && (game_library == NULL); i++)
	{
		gamename = dllnames[i];
		Com_sprintf (name, sizeof(name), "%s/%s/%s%s", cwd, debugdir, gamename,LIBEXT);
		game_library = SDL_LoadObject ( name );
	}
	if (game_library)
	{
		Com_DPrintf ("SDL_LoadObject (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s%s", cwd, gamename,LIBEXT);
		game_library = SDL_LoadObject ( name );
		if (game_library)
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
				for (i = 0; dllnames[i] != 0 && (game_library == NULL); i++)
				{
					gamename = dllnames[i];
					Com_sprintf (name, sizeof(name), "%s/%s%s", path, gamename,LIBEXT);
					game_library = SDL_LoadObject (name);
				}
				if (game_library)
				{
					Com_DPrintf ("SDL_LoadObject (%s)\n",name);
					break;
				}
			}
		}
	}

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
	qboolean		cdscan = false; // Knightmare added

	
	/*
	int32_t				i; // Knightmare added
	char			*cddir;


	// Knightmare- scan for cd command line option
	for (i=0; i<argc; i++)
		if (!strcmp(argv[i], "scanforcd")) {
			cdscan = true;
			break;
		}

	// if we find the CD, add a +set cddir xxx command line
	if (cdscan)
	{
		cddir = Sys_ScanForCD ();
		if (cddir && argc < MAX_NUM_ARGVS - 3)
		{
			int32_t		i;

			// don't override a cddir on the command line
			for (i=0 ; i<argc ; i++)
				if (!strcmp(argv[i], "cddir"))
					break;
			if (i == argc)
			{
				argv[argc++] = "+set";
				argv[argc++] = "cddir";
				argv[argc++] = cddir;
			}
		}
	}
	*/
	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

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
    return TRUE;
}

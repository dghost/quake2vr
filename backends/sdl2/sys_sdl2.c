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
#include <SDL_main.h>
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

#include "../../qcommon/shared/game.h"

#ifdef _WIN32
#include "../win32/resource.h"
#include "../win32/conproc.h"
#endif

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

//static int32_t			starttime;
SDL_bool	ActiveApp;
SDL_bool	Minimized;
SDL_bool	RelativeMouse;

#ifdef WIN32
static HANDLE		hinput, houtput;
#endif

SDL_Window *mainWindow;
uint32_t mainWindowID;
SDL_SysWMinfo mainWindowInfo;

void SDL_ProcEvent(SDL_Event *event, uint32_t time);
static uint32_t	sys_msg_time;
uint32_t	sys_frame_time;
uint32_t    sys_cacheline;


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
#define	MAX_NUM_ARGVS	128
static int32_t			argc;
static char		*argv[MAX_NUM_ARGVS];

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
	unsigned long		dummy, numread, numevents;
	int32_t		ch;

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
	unsigned long		dummy;
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
static cvar_t *nostdout;

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
		SDL_ProcEvent(&event, sys_msg_time);
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
	return SDL_GetClipboardText();
}

void Sys_FreeClipboardData ( char *cliptext )
{
    if (cliptext)
        SDL_free(cliptext);
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

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
void cpuid(uint32_t info, uint32_t *regs)
{
#ifdef _WIN32
        __cpuid((int *)regs, (int)info);
        
#else
        asm volatile
        ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
         : "a" (info), "c" (0));
        // ECX is set to zero for CPUID function 4
#endif
}

qboolean cpu_string(char* buffer, int size)
{
    char namestring[49];
    int start;
    
    uint32_t *word = (uint32_t *) &namestring[0];

    memset(namestring,0,sizeof(namestring));
    
    cpuid(0x80000000,&word[0]);
    
    if (word[0] < 0x80000004)
        return false;
    
    cpuid(0x80000002,&word[0]);
    cpuid(0x80000003,&word[4]);
    cpuid(0x80000004,&word[8]);
    
    for (start = 0; start < 49 && namestring[start]==' '; start++);

    SDL_snprintf(buffer, size, "%s",&namestring[start]);
    return true;
}
#endif

/*
 =================
 Sys_DetectCPU
 =================
*/

static qboolean Sys_DetectCPU (char *cpuString, int32_t maxSize)
{
    memset(cpuString,0,maxSize);
    return cpu_string(cpuString,maxSize);
}

char* basepath = NULL;

char *Sys_GetBaseDir (void) {
    if (!basepath) {
        char* dir = SDL_GetBasePath();
        if (dir) {
            int i;
            basepath = (char*)Z_TagStrdup(dir, TAG_SYSTEM);
            SDL_free(dir);
            
            Com_Printf("Got base path %s\n", basepath);
            
            i = strlen(basepath) - 1;
            
            if (i > 0) {
                while(i >= 0 && (basepath[i] == '\\' || basepath[i] == '/')) {
                    basepath[i] = '\0';
                    i--;
                }
            }
            i = strlen(basepath);
            
            if (i >= 0) {
                i--;
                for (;i>=0;i--) {
                    if (basepath[i] == '\\') {
                        basepath[i] = '/';
                    }
                }
            } else {
                Z_Free(basepath);
                basepath = NULL;
            }
            
        }
        
        if (!basepath) {
            basepath = (char*)Z_TagStrdup(".", TAG_SYSTEM);
        }
        
        
        Com_Printf("Using base path %s\n", basepath);
    }
    return basepath;
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
	Cvar_Get("sys_os", string, CVAR_NOSET|CVAR_LATCH);

	// Detect CPU
	if (Sys_DetectCPU(string, sizeof(string))) {
		Cvar_Get("sys_cpu", string, CVAR_NOSET|CVAR_LATCH);
	}
	else {
        int cores = SDL_GetCPUCount();
        char string[50];
        memset(string, 0, sizeof(string));
		SDL_snprintf(string, sizeof(string),"Unknown %i-core CPU", cores);
		Cvar_Get("sys_cpu", string, CVAR_NOSET|CVAR_LATCH);
	}
    
    sys_cacheline = SDL_GetCPUCacheLineSize();
    
    // Get physical memory
	SDL_snprintf(string,sizeof(string),"%u",SDL_GetSystemRAM());
	Cvar_Get("sys_ram", string, CVAR_NOSET|CVAR_LATCH);
	// end Q2E detection

    SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	SDL_snprintf(string,sizeof(string),"%d.%d.%d",linked.major, linked.minor, linked.patch);
	Cvar_Get("sys_sdl",string,CVAR_NOSET|CVAR_LATCH);
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


void* Sys_LoadLibrariesInPath(const char *path, const char *dllnames[]) {
    int i;
    void	*library = NULL;
    const char *gamename = NULL;
    char	name[MAX_OSPATH];
    
    if (!path)
        return NULL;
    
    for (i = 0; dllnames[i] != 0 && (library == NULL); i++)
    {
        gamename = dllnames[i];
        Com_sprintf (name, sizeof(name), "%s/%s%s", path, gamename, LIBEXT);
        library = SDL_LoadObject (name);
        if (!library)
            Com_DPrintf("failed to load %s: %s\n", name, SDL_GetError());
    }
    
    if (library)
    {
        Com_DPrintf ("SDL_LoadObject (%s)\n",name);
    }
    return library;
}

#ifdef WIN32
#define getcwd _getcwd
#endif

void* Sys_FindLibrary(const char *dllnames[])
{
    char	name[MAX_OSPATH];
    char	cwd[MAX_OSPATH];
    const char *path = NULL;
    void	*library = NULL;
    
#ifndef _DEBUG
    const char *debugdir = "release";
#else
    const char *debugdir = "debug";
#endif
    
    // check the current debug directory first for development purposes
    getcwd (cwd, sizeof(cwd));
    
    Com_sprintf (name, sizeof(name), "%s/%s", cwd, debugdir);
    library = Sys_LoadLibrariesInPath(name, dllnames);
    
#ifdef _DEBUG
    if (!library)
        library = Sys_LoadLibrariesInPath(cwd, dllnames);
#endif
    while (!library && (path = FS_NextPath(path)) != NULL)
    {
        library = Sys_LoadLibrariesInPath(path, dllnames);
    }
    return library;
}


void *Sys_LoadGameAPI(const char *name, void *parms, qboolean setGameLibrary) {
    game_export_t	*ge = NULL;
    void *library = NULL;
    
    Com_DPrintf ("SDL_LoadObject (%s)\n",name);
    library = SDL_LoadObject (name);
    
    if (library) {
        void	*(*GetGameAPI) (void *);
        GetGameAPI = (void *(*)(void*)) SDL_LoadFunction (library, "GetGameAPI");
        if (GetGameAPI) {
            ge = GetGameAPI (parms);
            if (ge->apiversion & GAME_API_VERSION_MASK) {
                Com_DPrintf("Attempting to load q2vr game library...\n");
                if (ge->apiversion != GAME_API_VERSION) {
                    Com_DPrintf ("game is version %i, not %i\n", ge->apiversion, GAME_API_VERSION);
                    ge = NULL;
                }
                else
                    Com_DPrintf("loading q2vr game library...\n");
            } else if (sv_legacy_libraries->value) {
                Com_DPrintf("Attempting to load legacy game library...\n");
                if (ge->apiversion != LEGACY_API_VERSION) {
                    Com_DPrintf ("game is version %i, not %i\n", ge->apiversion,
                                 LEGACY_API_VERSION);
                    ge = NULL;
                }
                else
                    Com_DPrintf("loading legacy game library...\n");
                
            } else {
                Com_DPrintf ("game is version %i, not %i\n", ge->apiversion,
                            GAME_API_VERSION);
                ge = NULL;
            }
        }
        
        if (ge && setGameLibrary) {
            game_library = library;
        } else {
            Com_DPrintf ("SDL_UnloadObject (%s)\n",name);
            SDL_UnloadObject(library);
        }
    } else {
        Com_DPrintf("failed to load %s: %s\n", name, SDL_GetError());
    }
    return ge;
}

game_import_t SV_GetGameImport (void);

// prefer loading Q2VR, then KMQ2, then try to fallback to legacy
static const char *dllnames[] = {
#ifdef Q2VR_ENGINE_MOD
    "vrgame" CPUSTRING,
    "mpgame" CPUSTRING,
#endif
#ifdef KMQUAKE2_ENGINE_MOD
    "kmq2game" CPUSTRING,
#endif
    "game" CPUSTRING,
    "game",
    0};


void* Sys_LoadGameLibrariesInPath(const char *path, qboolean setGameLibrary) {
    int i;
    const char *gamename = NULL;
    char	name[MAX_OSPATH];
    game_import_t import = SV_GetGameImport();
	void *ge = NULL;    
    qboolean missionPack = (!Q_strcasecmp(fs_gamedirvar->string, "rogue")
                            || !Q_strcasecmp(fs_gamedirvar->string, "xatrix"));
    
    if (!path)
        return NULL;
    
    for (i = 0; dllnames[i] != 0; i++)
    {
        gamename = dllnames[i];
        if (missionPack && !strncmp(gamename, "vrgame", 6))
            continue;
        else if (!missionPack && !strncmp(gamename, "mpgame", 6))
            continue;
        else if (!sv_legacy_libraries->value && !strncmp(gamename, "game", 4))
            continue;

        Com_sprintf (name, sizeof(name), "%s/%s%s", path, gamename, LIBEXT);
        ge = Sys_LoadGameAPI(name, &import, setGameLibrary);
        if (ge) {
            return ge;
        }
    }
    return NULL;
}

void* Sys_LoadGameLibraryInBasePaths(const char *dllname, qboolean setGameLibrary) {
    char	name[MAX_OSPATH];
    game_import_t import = SV_GetGameImport();
    const char *path = NULL;
    void	*ge = NULL;
    
    if (!dllname)
        return NULL;
    
    if (setGameLibrary) {
        qboolean missionPack;

        if (!sv_legacy_libraries->value && !strncmp(dllname, "game", 4))
            return NULL;
        
        missionPack = (!Q_strcasecmp(fs_gamedirvar->string, "rogue") || !Q_strcasecmp(fs_gamedirvar->string, "xatrix"));
        
        if (missionPack && !strncmp(dllname, "vrgame", 6))
            return NULL;
        else if (!missionPack && !strncmp(dllname, "mpgame", 6))
            return NULL;
    }
    
    while (!ge && (path = FS_NextBasePath(path)) != NULL)
    {
        Com_sprintf (name, sizeof(name), "%s/%s%s", path, dllname, LIBEXT);
        ge = Sys_LoadGameAPI(name, &import, setGameLibrary);
        if (ge) {
            return ge;
        }
    }
    return NULL;
}

void *Sys_GetGameAPI ()
{
    char	name[MAX_OSPATH];
    char	cwd[MAX_OSPATH];
    const char *path = NULL;
    void	*ge = NULL;
    int i;
    
       
#ifndef _DEBUG
    const char *debugdir = "release";
#else
    const char *debugdir = "debug";
#endif
    
    
	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");


    // check the current debug directory first for development purposes
    getcwd (cwd, sizeof(cwd));
    
    Com_sprintf (name, sizeof(name), "%s/%s", cwd, debugdir);
    ge = Sys_LoadGameLibrariesInPath(name, true);
    
#ifdef _DEBUG
    if (!ge) {
        ge = Sys_LoadGameLibrariesInPath(cwd, true);
    }
#endif
    while (!ge && (path = FS_NextGamePath(path)) != NULL)
    {
        Com_DPrintf("Checking path: %s\n", path);
        ge = Sys_LoadGameLibrariesInPath(path, true);
    }
    
    for (i = 0; !ge && dllnames[i]; i++)
    {
        Com_DPrintf("Checking library: %s\n", dllnames[i]);
        ge = Sys_LoadGameLibraryInBasePaths(dllnames[i], true);
    }
    
    if (!ge)
        Com_Printf("Could not find suitable library\n");

    return ge;
}

//=======================================================================

/*
==================
main

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
			SDL_ProcEvent(&event, sys_msg_time);
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

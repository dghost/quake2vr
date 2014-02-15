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
// sys_win.h

#include "../qcommon/qcommon.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../win32/conproc.h"
#include "../vr/vr.h"

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

int			starttime;
int			ActiveApp;
qboolean	Minimized;

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


static HANDLE		qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];


#ifndef NEW_DED_CONSOLE
/*
===============================================================================

DEDICATED CONSOLE

===============================================================================
*/
static char	console_text[256];
static int	console_textlen;

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
	int		dummy;
	int		ch, numread, numevents;

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
	int		dummy;
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
#endif // NEW_DED_CONSOLE

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// grab frame time 
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
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

#ifndef NEW_DED_CONSOLE
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

	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}
#endif // NEW_DED_CONSOLE


void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	CloseHandle (qwclsemaphore);
	if (dedicated && dedicated->value)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

#ifdef NEW_DED_CONSOLE
	Sys_ShutdownConsole();
#endif

	exit (0);
}


void WinError (void)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
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
	int			i; // Knightmare added

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
l33t CPU detection
Borrowed from Q2E
=================
*/
static qboolean Sys_DetectCPU (char *cpuString, int maxSize)
{
#if defined _M_IX86

	char				vendor[16];
	int					stdBits, features, moreFeatures, extFeatures;
	int					family, extFamily, model, extModel;
	unsigned __int64	start, end, counter, stop, frequency;
	unsigned			speed;
	qboolean			hasMMX, hasMMXExt, has3DNow, has3DNowExt, hasSSE, hasSSE2, hasSSE3;

	// Check if CPUID instruction is supported
	__try {
		__asm {
			mov eax, 0
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER){
		return false;
	}

	// Get CPU info
	__asm {
		; // Get vendor identifier
		mov eax, 0
		cpuid
		mov dword ptr[vendor+0], ebx
		mov dword ptr[vendor+4], edx
		mov dword ptr[vendor+8], ecx
		mov dword ptr[vendor+12], 0

		; // Get standard bits and features
		mov eax, 1
		cpuid
		mov stdBits, eax
		mov moreFeatures, ecx ; // Knightmare added
		mov features, edx

		; // Check if extended functions are present
		mov extFeatures, 0
		mov eax, 80000000h
		cpuid
		cmp eax, 80000000h
		jbe NoExtFunction

		; // Get extended features
		mov eax, 80000001h
		cpuid
		mov extFeatures, edx

NoExtFunction:
	}

	// Get CPU name
	family = (stdBits >> 8) & 15;
	extFamily = (stdBits >> 20) & 255;
	model = (stdBits >> 4) & 15;
	extModel = (stdBits >> 16) & 15;

	if (!Q_stricmp(vendor, "AuthenticAMD"))
	{
		strncpy(cpuString, "AMD", maxSize);

		switch (family)
		{
		case 4:
			Q_strncatz(cpuString, " 5x86", maxSize);
			break;
		case 5:
			switch (model)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				Q_strncatz(cpuString, " K5", maxSize);
				break;
			case 6:
			case 7:
				Q_strncatz(cpuString, " K6", maxSize);
				break;
			case 8:
				Q_strncatz(cpuString, " K6-2", maxSize);
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				Q_strncatz(cpuString, " K6-III", maxSize);
				break;
			}
			break;
		case 6: // K7 family
			switch (model)
			{
			case 1:		// 250nm core
			case 2:		// 180nm core
			case 4:		// Thunderbird core
			case 6:		// Palomino core
			case 8:		// Thoroughbred core
			case 10:	// Barton core
				Q_strncatz(cpuString, " Athlon", maxSize);
				break;
			case 3:		// Spitfire core
			case 7:		// Morgan core
				Q_strncatz(cpuString, " Duron", maxSize);
				break;
			default:
				Q_strncatz(cpuString, " K7", maxSize);
				break;
			}
			break;
		case 15: // refer to extended family
			if (extFamily == 0) // K8 family
			{
				switch (model)
				{
			//	case 0:
			//	case 2:
			//	case 6:
				case 4:		// Clawhammer/Newark
				case 7:		// San Diego/Newcastle
				case 12:	// Newcastle/Albany
					Q_strncatz(cpuString, " Athlon 64", maxSize);
					break;
				case 3:		// Toledo
				case 11:	// Manchester/Brisbane
				case 15:	// Winchester/Venice
					Q_strncatz(cpuString, " Athlon 64 X2", maxSize);
					break;
				case 1:
				case 5:
					Q_strncatz(cpuString, " Athlon 64 FX / Opteron", maxSize);
					break;
				default:
					Q_strncatz(cpuString, " K8", maxSize);
					break;
				}
			}
			else if (extFamily == 1) // K10 family
			{
				switch (model)
				{
				case 0: // Barcelona A0-A2
				case 2: // Barcelona B0-B3
					Q_strncatz(cpuString, " Phenom / Opteron", maxSize);
					break;
				case 4: // Deneb / Shanghai
				case 10: // Thuban
					Q_strncatz(cpuString, " Phenom II / Opteron", maxSize);
					break;
				case 5: // Propus
				case 6: // Regor
					Q_strncatz(cpuString, " Athlon II", maxSize);
					break;
				default:
					Q_strncatz(cpuString, " K10", maxSize);
					break;
				}
			}
			else if (extFamily == 12) // Stars Fusion family
			{
				switch (model)
				{
				case 1:	// Llano
					Q_strncatz(cpuString, " A8 APU", maxSize);
					break;
				default:
					Q_strncatz(cpuString, " A series APU", maxSize);
					break;
				}
			}
			else if (extFamily == 14) // Bobcat family
			{
				switch (model)
				{
				case 1:	// Zacate
					Q_strncatz(cpuString, " E-350 APU", maxSize);
					break;
				default:
					Q_strncatz(cpuString, " E series APU", maxSize);
					break;
				}
			}
			else if (extFamily == 15) // Bulldozer family
			{
				switch (model)
				{
				case 0:	// Zambezi
					if (extModel == 10)
						Q_strncatz(cpuString, " A series APU", maxSize);
					else
						Q_strncatz(cpuString, " FX series", maxSize);
					break;
				case 2:
					if (extModel == 2) // Vishera
						Q_strncatz(cpuString, " FX series", maxSize);
					break;
				case 3:
					if (extModel == 13)	// Richland
						Q_strncatz(cpuString, " A series APU", maxSize);
					break;
				default:
					Q_strncatz(cpuString, " FX series", maxSize);
					break;
				}
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "CyrixInstead"))
	{
		strncpy(cpuString, "Cyrix", maxSize);

		switch (family)
		{
		case 4:
			Q_strncatz(cpuString, " 5x86", maxSize);
			break;
		case 5:
			switch (model)
			{
			case 2:
				Q_strncatz(cpuString, " 6x86", maxSize);
				break;
			case 4:
				Q_strncatz(cpuString, " MediaGX", maxSize);
				break;
			default:
				Q_strncatz(cpuString, " 6x86 / MediaGX", maxSize);
				break;
			}
			break;
		case 6:
			Q_strncatz(cpuString, " 6x86MX", maxSize);
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "CentaurHauls"))
	{
		strncpy(cpuString, "Centaur", maxSize);

		switch (family)
		{
		case 5:
			switch (model)
			{
			case 4:
				Q_strncatz(cpuString, " C6", maxSize);
				break;
			case 8:
				Q_strncatz(cpuString, " C2", maxSize);
				break;
			case 9:
				Q_strncatz(cpuString, " C3", maxSize);
				break;
			default: // unknown model
				break;
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "NexGenDriven"))
	{
		strncpy(cpuString, "NexGen", maxSize);

		switch (family)
		{
		case 5:
			switch (model)
			{
			case 0:
				Q_strncatz(cpuString, " Nx586 / Nx586FPU", maxSize);
				break;
			default: // unknown model
				break;
			}
			break;
		default: // unknown family
			break;
		}
	}
	else if (!Q_stricmp(vendor, "GenuineIntel"))
	{
		int intModel = model + (extModel << 4);
		strncpy(cpuString, "Intel", maxSize);
		switch (family)
		{
		case 5: // Pentium family
			switch (model)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 7:
			case 8:
			default:
				Q_strncatz(cpuString, " Pentium", maxSize);
				break;
			}
			break;
		case 6:
			if ((family + extFamily) == 6)	// newer CPUs
			{
				switch (model)
				{
				case 5:		// Clarkdale
					Q_strncatz(cpuString, " Core i5 6xx / Core i3 5xx", maxSize);
					break;
				case 7:		
					if (intModel == 0x17)	// Wolfdale / Yorkfield
						Q_strncatz(cpuString, " Core 2", maxSize);
					else // Katmai
						Q_strncatz(cpuString, " Pentium III", maxSize);
					break;
				case 8:		// Coppermine
					Q_strncatz(cpuString, " Pentium III", maxSize);
					break;

				case 10:	
					if (intModel == 0x2A)	// Sandy Bridge
						Q_strncatz(cpuString, "  Core i7 /i5 2xxx", maxSize);
					else if (intModel == 0x3A)	// Ivy Bridge
						Q_strncatz(cpuString, "  Core i7 /i5 3xxx", maxSize);
					else if (intModel == 0x1A)	// Bloomfield
						Q_strncatz(cpuString, " Core i7 9xx", maxSize);
					else if (intModel == 0x45)
						Q_strncatz(cpuString, " Core i7 /i5 4xxx", maxSize);
					else // Coppermine
						Q_strncatz(cpuString, " Pentium III", maxSize);
					break;

				case 11:	// Tualatin
					Q_strncatz(cpuString, " Pentium III", maxSize);
					break;
				case 12:
					if (intModel == 0x2C)	// Gulftown
						Q_strncatz(cpuString, " Core i7 9xx", maxSize);
					else if (extModel == 0x3C)	// Haswell
						Q_strncatz(cpuString, "  Core i7 /i5 4xxx", maxSize);
					else	// Silverthorne
						Q_strncatz(cpuString, " Atom", maxSize);
					break;
				case 13:	
					if (intModel == 0x2D)	// Sandy Bridge-E
						Q_strncatz(cpuString, " Core i7 39xx / 38xx", maxSize);
					else // Dothan
						Q_strncatz(cpuString, " Pentium M", maxSize);
					break;
				case 14:
					if (intModel == 0x1E)	// Lynnfield
						Q_strncatz(cpuString, " Core i7 8xx / Core i5 7xx", maxSize);
					else
						// Yonah
						Q_strncatz(cpuString, " Core", maxSize);

					break;
				case 15: // Conroe / Kentsfield / Wolfdale / Yorkfield	
					Q_strncatz(cpuString, " Core 2", maxSize);
					break;
				default:
					break;
				}
			}
			else if (extFamily != 6) // P6 family
			{
				switch (model)
				{
				case 0:
				case 1:
					Q_strncatz(cpuString, " Pentium Pro", maxSize);
					break;
				// Actual differentiation depends on cache settings
				case 3:		// Klamath
				case 5:		// Deschutes
					Q_strncatz(cpuString, " Pentium II", maxSize);
					break;
				case 6:
					Q_strncatz(cpuString, " Celeron", maxSize);
					break;
				// Actual differentiation depends on cache settings

				default:
					Q_strncatz(cpuString, " P6", maxSize);
					break;
				}
			}
			break;
		case 7: // Itanium family
			switch (model)
			{
			default:
				Q_strncatz(cpuString, " Itanium", maxSize);
				break;
			}
			break;
		case 15: // refer to extended family
			if (extFamily == 0) // NetBurst family
			{
				switch (model)
				{
				case 0:		// Williamette
				case 1:		// Williamette
				case 2:		// Northwood
				case 3:		// Prescott
				case 4:		// Smithfield
				case 6:		// Cedar Mill / Presler
					Q_strncatz(cpuString, " Pentium 4", maxSize);
					break;
			//	case 4:		// Smithfield
			//	case 6:		// Cedar Mill / Presler
			//		Q_strncatz(cpuString, " Pentium D", maxSize);
			//		break;
				default:
					Q_strncatz(cpuString, " NetBurst", maxSize);
					break;
				}
			}
			else if (extFamily == 1 || extFamily == 2) // Itanium 2 family
			{
				switch (model)
				{
				default:
					Q_strncatz(cpuString, " Itanium 2", maxSize);
					break;
				}
			}
			break;
		default: // unknown family
			break;
		}
	}
	else
		return false;

	// Check if RDTSC instruction is supported
	if ((features >> 4) & 1){
		// Measure CPU speed
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

		__asm {
			rdtsc
			mov dword ptr[start+0], eax
			mov dword ptr[start+4], edx
		}

		QueryPerformanceCounter((LARGE_INTEGER *)&stop);
		stop += frequency;

		do {
			QueryPerformanceCounter((LARGE_INTEGER *)&counter);
		} while (counter < stop);

		__asm {
			rdtsc
			mov dword ptr[end+0], eax
			mov dword ptr[end+4], edx
		}

		speed = (unsigned)((end - start) / 1000000);

		Q_strncatz(cpuString, va(" %u MHz", speed), maxSize);
	}

	// Get extended instruction sets supported
	hasMMX = (features >> 23) & 1;
	hasMMXExt = (extFeatures >> 22) & 1;
	has3DNow = (extFeatures >> 31) & 1;
	has3DNowExt = (extFeatures >> 30) & 1;
	hasSSE = (features >> 25) & 1;
	hasSSE2 = (features >> 26) & 1;
	hasSSE3 = (moreFeatures >> 0) & 1;

	if (hasMMX || has3DNow || hasSSE)
	{
		Q_strncatz(cpuString, " w/", maxSize);

		if (hasMMX){
			Q_strncatz(cpuString, " MMX", maxSize);
			if (hasMMXExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (has3DNow){
			Q_strncatz(cpuString, " 3DNow!", maxSize);
			if (has3DNowExt)
				Q_strncatz(cpuString, "+", maxSize);
		}
		if (hasSSE){
			Q_strncatz(cpuString, " SSE", maxSize);
			if (hasSSE3)
				Q_strncatz(cpuString, "3", maxSize);
			else if (hasSSE2)
				Q_strncatz(cpuString, "2", maxSize);
		}
	}

	return true;

#else

	Q_strncpyz(cpuString, "Alpha AXP", maxSize);
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
	OSVERSIONINFO	osInfo;
	MEMORYSTATUS	memStatus;	// Knightmare added
	char			string[64];	// Knightmare added

#if 0
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore already exists
    qwclsemaphore = CreateMutex(
        NULL,         /* Security attributes */
        0,            /* owner       */
        "qwcl"); /* Semaphore name      */
	if (!qwclsemaphore)
		Sys_Error ("QWCL is already running on this system");
	CloseHandle (qwclsemaphore);

    qwclsemaphore = CreateSemaphore(
        NULL,         /* Security attributes */
        0,            /* Initial count       */
        1,            /* Maximum count       */
        "qwcl"); /* Semaphore name      */
#endif

	timeBeginPeriod( 1 );

	osInfo.dwOSVersionInfoSize = sizeof(osInfo);

	if (!GetVersionEx (&osInfo))
		Sys_Error ("Couldn't get OS info");

	if ((osInfo.dwMajorVersion < 5 && osInfo.dwMinorVersion < 1) || (osInfo.dwPlatformId != VER_PLATFORM_WIN32_NT))
		Sys_Error ("KMQuake2 requires Windows XP or higher");

// from Q2E - OS & CPU detection
	if (osInfo.dwMajorVersion == 5 && osInfo.dwMinorVersion == 1)
		strncpy(string, "Windows XP", sizeof(string));
	if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 0)
		strncpy(string, "Windows Vista", sizeof(string));
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 1)
		strncpy(string, "Windows 7", sizeof(string));
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2)
		strncpy(string, "Windows 8", sizeof(string));
	else
		strncpy(string, va("Windows %i.%i", osInfo.dwMajorVersion, osInfo.dwMinorVersion), sizeof(string));


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
	GlobalMemoryStatus(&memStatus);
	strncpy(string, va("%u",memStatus.dwTotalPhys >> 20), sizeof(string));
	Com_Printf("Memory: %s MB\n", string);
	Cvar_Get("sys_ramMegs", string, CVAR_NOSET|CVAR_LATCH);
// end Q2E detection

#ifndef NEW_DED_CONSOLE
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
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
	int i = 0;
	//Knightmare- changed DLL name for better cohabitation
	const char *dllnames[2] = {"vrgamex86.dll", "kmq2gamex86.dll"};
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
	
	for (i = 0; (i < 2) && (game_library == NULL); i++)
	{
		gamename = dllnames[i];
		Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
		game_library = LoadLibrary ( name );
	}
	if (game_library)
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
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
				for (i = 0; (i < 2) && (game_library == NULL); i++)
				{
					gamename = dllnames[i];
					Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
					game_library = LoadLibrary (name);
				}
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
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
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}
}

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;
HWND		hwnd_dialog; // Knightmare added

int WINAPI WinMain (__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd)
{
    MSG				msg;
	int				time, oldtime, newtime;
	char			*cddir;
	int				i; // Knightmare added
	qboolean		cdscan = false; // Knightmare added

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;

	ParseCommandLine (lpCmdLine);

#ifndef NEW_DED_CONSOLE
	// Knightmare- startup logo, code from TomazQuake
	//if (!(dedicated && dedicated->value))
	if (false)
	{
		RECT			rect; // Knightmare added

		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

		if (hwnd_dialog)
		{
			if (GetWindowRect (hwnd_dialog, &rect))
			{
				if (rect.left > (rect.top * 2))
				{
					SetWindowPos (hwnd_dialog, 0, (rect.left/2) - ((rect.right - rect.left)/2), rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}

			ShowWindow (hwnd_dialog, SW_SHOWDEFAULT);
			UpdateWindow (hwnd_dialog);
			SetForegroundWindow (hwnd_dialog);
		}
	}
	// end Knightmare
#endif

#ifdef NEW_DED_CONSOLE // init debug console
	Sys_InitDedConsole ();
	Com_Printf("Quake II VR %4.2f %s %s %s\n", VERSION, CPUSTRING, BUILDSTRING, __DATE__);
#endif

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
			int		i;

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

	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

    /* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			Sleep (1);
		}

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_msg_time = msg.time;
			TranslateMessage (&msg);
   			DispatchMessage (&msg);
		}

		// DarkOne's CPU usage fix


		while (1)
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
			if (time > 0) break;
			if (!vr_enabled->value || !vr_nosleep->value)
				Sleep(0); // may also use Speep(1); to free more CPU, but it can lower your fps
		}

		/*do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);*/
		//	Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

		//	_controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
		_controlfp( _PC_24, _MCW_PC );
		Qcommon_Frame (time);

		oldtime = newtime;
	}

	// never gets here
    return TRUE;
}

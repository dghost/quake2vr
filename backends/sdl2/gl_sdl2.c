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
/*
** GL_SDL2.C
**
** This file contains ALL SDL2 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/
#include <assert.h>
//#include <windows.h>
#include "../../client/renderer/include/r_local.h"
#include "gl_sdl2.h"
#include "sdl2quake.h"
#include "../../client/vr/include/vr.h"

#ifdef _WIN32
#include "../win32/resource.h"
#include <SDL_syswm.h>
#endif

static qboolean GLimp_SwitchFullscreen( int32_t width, int32_t height );
qboolean GLimp_InitGL (void);

SDL_GLContext glcontext;


extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;
extern cvar_t *vid_refresh;

static qboolean VerifyDriver( void )
{
	char buffer[1024];

	strcpy( buffer, glGetString( GL_RENDERER ) );
	strlwr( buffer );
	if ( strcmp( buffer, "gdi generic" ) == 0 )
		return false;
	return true;
}

qboolean modType (char *name);

/*
** VID_CreateWindow
*/
#define	WINDOW_TITLE	"Quake II VR" // changed
#define	WINDOW_TITLE_XATRIX	"Quake II VR - The Reckoning" // changed
#define	WINDOW_TITLE_ROGUE	"Quake II VR - Ground Zero" // changed
#define WINDOW_ICON			"icons/q2.bmp"
#define WINDOW_ICON_XATRIX  "icons/q2mp1.bmp"
#define WINDOW_ICON_ROGUE	"icons/q2mp2.bmp"

SDL_Surface* GLimp_LoadIcon(const char *name)
{
	int32_t size;
	fileHandle_t handle;
	SDL_Surface *result = NULL;
	size = FS_FOpenFile(name,&handle,FS_READ);
	if (size > 0)
	{
		SDL_RWops *rw;
		void *buffer = NULL;
		
		buffer = malloc(size);
		if (buffer)
		{
			FS_Read(buffer,size,handle);
			rw = SDL_RWFromMem(buffer,size);
			if (rw)
			{
				result = SDL_LoadBMP_RW(rw,1);
			}
			free(buffer);
		}
		FS_FCloseFile(handle);
	}
	return result;
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode ( int32_t *pwidth, int32_t *pheight )
{
	int32_t width, height;
	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_BORDERLESS;
	SDL_DisplayMode targetMode;
	SDL_Surface *icon = NULL;

	const char *win_fs[] = { "W", "FS" };
	char *title = NULL;

	int32_t xpos = Cvar_VariableInteger("vid_xpos");
	int32_t ypos = Cvar_VariableInteger("vid_ypos");
	qboolean fullscreen = Cvar_VariableInteger("vid_fullscreen");

	if (vr_enabled->value)
	{
		VR_GetHMDPos(&xpos,&ypos);
	}


	width = Cvar_VariableInteger("vid_width");
	height = Cvar_VariableInteger("vid_height");

	VID_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	VID_Printf (PRINT_ALL, "...setting mode %dx%d %s\n", width, height, win_fs[fullscreen] );

	// destroy the existing window
	if (mainWindow)
	{
		GLimp_Shutdown ();
	}

	// do a CDS if needed
	if ( fullscreen )
	{
		SDL_Rect displayBounds;
		int32_t numDisplays = SDL_GetNumVideoDisplays();
		int32_t targetDisplay = 0;
		int32_t i = 0;
		int32_t refresh = vid_refresh->value;
		SDL_DisplayMode idealMode;



		for (i = 0; i < numDisplays; i++)
		{
			if (!SDL_GetDisplayBounds(i,&displayBounds) &&
				(xpos >= displayBounds.x && xpos < displayBounds.x + displayBounds.w) &&
				(ypos >= displayBounds.y && ypos < displayBounds.y + displayBounds.h))
			{
				xpos = displayBounds.x;
				ypos = displayBounds.y;
				targetDisplay = i;
			}
		}

		idealMode.format = SDL_PIXELFORMAT_RGBA8888;
		idealMode.w = width;
		idealMode.h = height;
		idealMode.refresh_rate = refresh;
		idealMode.driverdata = 0;
		if (!SDL_GetClosestDisplayMode(targetDisplay,&idealMode,&targetMode))
			return rserr_invalid_fullscreen;

		if (width != targetMode.w || height != targetMode.h || (refresh && refresh != targetMode.refresh_rate ))
		{
			return rserr_invalid_fullscreen;
		}
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if (modType("xatrix")) { // q2mp1
		icon = GLimp_LoadIcon(WINDOW_ICON_XATRIX);		
		title = WINDOW_TITLE_XATRIX;
	}
	else if (modType("rogue"))  { // q2mp2
		icon = GLimp_LoadIcon(WINDOW_ICON_ROGUE);
		title = WINDOW_TITLE_ROGUE;
	}
	else {
		icon = GLimp_LoadIcon(WINDOW_ICON);
		title = WINDOW_TITLE;
		//wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	}

	if ( r_bitdepth->value > 0 ) {
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, r_bitdepth->value);
		VID_Printf( PRINT_ALL, "...using r_bitdepth of %d\n", ( int32_t ) r_bitdepth->value );
	} else {
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);


	mainWindow = SDL_CreateWindow(title, xpos, ypos, width, height, flags);

	if (!mainWindow)
		return rserr_invalid_mode;

	if (icon)
	{
		SDL_SetWindowIcon(mainWindow,icon);
		SDL_FreeSurface(icon);
	} else

	if (fullscreen)
	{
		if ( SDL_SetWindowDisplayMode(mainWindow,&targetMode) < 0)
		{
			SDL_DestroyWindow(mainWindow);
			mainWindow = NULL;
			return rserr_invalid_fullscreen;
		}
	}
	SDL_VERSION(&mainWindowInfo.version); // initialize info structure with SDL version info
	SDL_GetWindowWMInfo(mainWindow,&mainWindowInfo);
#ifdef _WIN32
		if(mainWindowInfo.subsystem == SDL_SYSWM_WINDOWS)
			SetForegroundWindow(mainWindowInfo.info.win.window);
#endif
	

	SDL_RaiseWindow(mainWindow);
	SDL_SetWindowGrab(mainWindow,SDL_TRUE);
	mainWindowID = SDL_GetWindowID(mainWindow);

	if (!GLimp_InitGL())
	{
		VID_Printf( PRINT_ALL, "GLimp_SetMode() - GLimp_InitGL failed\n");
		return rserr_unknown;
	}
	SDL_GL_GetDrawableSize(mainWindow,&width,&height);
	printf("Set window to size %ix%i...\n",width,height);
	VID_NewWindow (width, height);
	*pwidth = width;
	*pheight = height;
	return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
float original_brightness;
void UpdateGammaRamp (qboolean enable);

void GLimp_Shutdown( void )
{
	//Knightmare- added Vic's hardware gamma ramp
	UpdateGammaRamp(false);

	//end Knightmare
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(mainWindow);
	mainWindow = NULL;
	mainWindowID = -1;
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean GLimp_Init( )
{
	return true;
}

qboolean GLimp_InitGL (void)
{


	glcontext = SDL_GL_CreateContext(mainWindow);
	if (!glcontext)
	{
		VID_Printf (PRINT_ALL, "GLimp_Init() - SDL_GL_CreateContext failed\n");

		goto fail;
	}

	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK)
	{
		VID_Printf( PRINT_ALL, "GLimp_Init() - glewInit failed\n");
		goto fail;
	}

	if ( !VerifyDriver() )
	{
		VID_Printf( PRINT_ALL, "GLimp_Init() - no hardware acceleration detected.\nPlease install drivers provided by your video card/GPU vendor.\n" );
		goto fail;
	}

	//Knightmare- Vic's hardware gamma stuff
	if ( !r_ignorehwgamma->value )
	{
		original_brightness = SDL_GetWindowBrightness(mainWindow);
		vid_gamma->modified = true;
	}


	//Knightmare- 12/24/2001- stencil buffer
	{
		VID_Printf( PRINT_ALL, "... Using stencil buffer\n" );
		glConfig.have_stencil = true; // Stencil shadows - MrG

	}

	/*	Moved to GL_SetDefaultState in r_glstate.c
	// Vertex arrays
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
	glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);
	glColorPointer (4, GL_FLOAT, sizeof(colorArray[0]), colorArray[0]);
	//glState.activetmu[0] = true;
	// end vertex arrays
	*/
	return true;

fail:

	GLimp_Shutdown();
	return false;
}

//Knightmare- added Vic's hardware gammaramp
void UpdateGammaRamp (qboolean enable)
{
	float gamma;
	int32_t ret;

	if (enable)
		gamma =  vid_gamma->value;
	else
		gamma = original_brightness;

	ret = SDL_SetWindowBrightness(mainWindow,gamma);

	if ( ret < 0 ) {
		// originally the idea of warning on failures seemed pretty rad
		// unfortunately, this tends to return false even if it succeeds
		Com_Printf( "SDL_SetWindowBrightness failed: %s\n", SDL_GetError() );
	}
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( )
{
	if ( r_bitdepth->modified )
	{
		r_bitdepth->modified = false;
	}


}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	SDL_GL_SwapWindow( mainWindow);
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
	if ( active )
	{
		if (mainWindow)
		{
#ifdef _WIN32
			if(vid_fullscreen->value && mainWindowInfo.subsystem == SDL_SYSWM_WINDOWS)
				SetForegroundWindow(mainWindowInfo.info.win.window);
#endif
			SDL_RaiseWindow(mainWindow);
			SDL_ShowWindow(mainWindow);

		}

	}	else
	{
		if ( vid_fullscreen->value )
			SDL_MinimizeWindow(mainWindow);
	}
}

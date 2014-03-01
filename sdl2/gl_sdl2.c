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
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
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
#include <windows.h>
#include "../renderer/r_local.h"
#include "gl_sdl2.h"
#include "sdl2quake.h"
#include "../vr/vr.h"
#include "SDL_syswm.h"

#ifdef _WIN32
#include "../win32/resource.h"
#endif

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

SDL_GLContext glcontext;


extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

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
#define	WINDOW_CLASS_NAME	"Quake II VR" // changed
#define	WINDOW_CLASS_NAME2	"Quake II VR - The Reckoning" // changed
#define	WINDOW_CLASS_NAME3	"Quake II VR - Ground Zero" // changed

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode ( int xpos, int ypos, int *pwidth, int *pheight, qboolean fullscreen )
{
	int width, height;
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED;

	const char *win_fs[] = { "W", "FS" };
	char *title = NULL;
	width = Cvar_VariableInteger("r_width");
	height = Cvar_VariableInteger("r_height");

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
		int numDisplays = SDL_GetNumVideoDisplays();
		int targetDisplay = 0;
		int i = 0;
		SDL_DisplayMode idealMode, targetMode;

		for (i = 0; i < numDisplays; i++)
		{
			if (!SDL_GetDisplayBounds(i,&displayBounds) &&
				(xpos >= displayBounds.x && xpos <= displayBounds.x + displayBounds.w) &&
				(ypos >= displayBounds.y && ypos <= displayBounds.y + displayBounds.h))
			{
				xpos = displayBounds.x;
				ypos = displayBounds.y;
				targetDisplay = 0;
			}
		}

		idealMode.format = SDL_PIXELFORMAT_RGBA8888;
		idealMode.w = width;
		idealMode.h = height;
		idealMode.refresh_rate = 0;
		idealMode.driverdata = 0;
		SDL_GetClosestDisplayMode(targetDisplay,&idealMode,&targetMode);
		if (width != targetMode.w || height != targetMode.h)
		{
			return rserr_invalid_fullscreen;
		}
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	
	if (modType("xatrix")) { // q2mp1
		//wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON2));
		title = WINDOW_CLASS_NAME2;
	}
	else if (modType("rogue"))  { // q2mp2
		//wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON3));
		title = WINDOW_CLASS_NAME3;
	}
	else {
		title = WINDOW_CLASS_NAME;
		//wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	}
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
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

#ifdef _WIN32
	{
		SDL_SysWMinfo info;
		if (SDL_GetWindowWMInfo(mainWindow,&info))
		{
			cl_hwnd = info.info.win.window;
		}
	}
#endif

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

void GLimp_Shutdown( void )
{
	//Knightmare- added Vic's hardware gamma ramp
	if ( !r_ignorehwgamma->value )
	{
	
		SDL_SetWindowBrightness (mainWindow, original_brightness);
	}
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
 
	cvar_t *stereo;

	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	/*
	** set PFD_STEREO if necessary
	*/
	if ( stereo->value != 0 )
	{
		VID_Printf( PRINT_ALL, "...attempting to use stereo\n" );
		glState.stereo_enabled = true;
	}
	else
	{
		glState.stereo_enabled = false;
	}

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
		r_gamma->modified = true;
	}

	// moved these to GL_SetDefaultState
	//glState.blend = false;
	//glState.alphaTest = false;
	//end Knightmare

	//Knightmare- 12/24/2001- stecil buffer
	{
		char buffer[1024];

		strcpy( buffer, glGetString( GL_RENDERER ) );
		strlwr( buffer );
		VID_Printf( PRINT_ALL, "... Using stencil buffer\n" );
		glConfig.have_stencil = true; // Stencil shadows - MrG

	}
	//if (pfd.cStencilBits)
	//	glConfig.have_stencil = true;
	//end Knightmare

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
void UpdateGammaRamp (void)
{
	float gamma =  r_gamma->value;
	int ret;

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
void GLimp_BeginFrame( float camera_separation )
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
			SDL_RaiseWindow(mainWindow);
			SDL_ShowWindow(mainWindow);
		}
		
	}	else
	{
		if ( vid_fullscreen->value )
			SDL_MinimizeWindow(mainWindow);
	}
}

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

// r_misc.c - particle image loading, and screenshots

#include "include/r_local.h"
#include "SDL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

//#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "include/stb_image_resize.h"

/*
==================
R_CreateNullTexture
==================
*/
#define NULLTEX_SIZE 16
image_t * R_CreateNullTexture (void)
{
	byte	nulltex[NULLTEX_SIZE][NULLTEX_SIZE][4];
	int32_t		x;

	memset (nulltex, 32, sizeof(nulltex));
	for (x=0; x<NULLTEX_SIZE; x++)
	{
		nulltex[0][x][0]=
		nulltex[0][x][1]=
		nulltex[0][x][2]=
		nulltex[0][x][3]= 255;

		nulltex[x][0][0]=
		nulltex[x][0][1]=
		nulltex[x][0][2]=
		nulltex[x][0][3]= 255;

		nulltex[NULLTEX_SIZE-1][x][0]=
		nulltex[NULLTEX_SIZE-1][x][1]=
		nulltex[NULLTEX_SIZE-1][x][2]=
		nulltex[NULLTEX_SIZE-1][x][3]= 255;

		nulltex[x][NULLTEX_SIZE-1][0]=
		nulltex[x][NULLTEX_SIZE-1][1]=
		nulltex[x][NULLTEX_SIZE-1][2]=
		nulltex[x][NULLTEX_SIZE-1][3]= 255;
	}
	return R_LoadPic ("***notexture***", (byte *)nulltex, NULLTEX_SIZE, NULLTEX_SIZE, it_wall, 32);
}


/*
==================
LoadPartImg
==================
*/
image_t *LoadPartImg (char *name, imagetype_t type)
{
	image_t *image = R_FindImage(name, type);
	if (!image) image = glMedia.notexture;
	return image;
}


/*
==================
R_SetParticlePicture
==================
*/
void R_SetParticlePicture (int32_t num, char *name)
{
	glMedia.particletextures[num] = LoadPartImg (name, it_part);
}


/*
==================
R_InitMedia
==================
*/
void R_InitMedia (void)
{
	int32_t		x;
	byte	whitetex[NULLTEX_SIZE][NULLTEX_SIZE][4];
#ifdef ROQ_SUPPORT
	byte	data2D[256*256*4]; // Raw texture
#endif // ROQ_SUPPORT

	glMedia.notexture = R_CreateNullTexture (); // Generate null texture

	memset (whitetex, 255, sizeof(whitetex));
	glMedia.whitetexture = R_LoadPic ("***whitetexture***", (byte *)whitetex, NULLTEX_SIZE, NULLTEX_SIZE, it_wall, 32);

#ifdef ROQ_SUPPORT
	memset(data2D, 255, 256*256*4);
	glMedia.rawtexture = R_LoadPic ("***rawtexture***", data2D, 256, 256, it_pic, 32);
#endif // ROQ_SUPPORT
	
	glMedia.envmappic = LoadPartImg ("gfx/effects/envmap.tga", it_wall);
	glMedia.spheremappic = LoadPartImg ("gfx/effects/spheremap.tga", it_skin);
	glMedia.shelltexture = LoadPartImg ("gfx/effects/shell_generic.tga", it_skin);
	glMedia.causticwaterpic = LoadPartImg ("gfx/water/caustic_water.tga", it_wall);
	glMedia.causticslimepic = LoadPartImg ("gfx/water/caustic_slime.tga", it_wall);
	glMedia.causticlavapic = LoadPartImg ("gfx/water/caustic_lava.tga", it_wall);
	glMedia.particlebeam = LoadPartImg ("gfx/particles/beam.tga", it_part);

	// Psychospaz's enhanced particles
	for (x=0; x<PARTICLE_TYPES; x++)
		glMedia.particletextures[x] = NULL;

	CL_SetParticleImages ();
}


/* 
============================================================================== 

SCREEN SHOTS 

============================================================================== 
*/ 

/* 
================== 
R_ScaledScreenshot
================== 
*/

byte	*saveshotdata;

void R_ScaledScreenshot (char *name)
{
	char							shotname[MAX_OSPATH];
	int32_t								saveshotWidth, saveshotHeight;
	byte							*pngdata;
	int result;

	if (!saveshotdata)	return;

	// Optional hi-res saveshots
	saveshotWidth = saveshotHeight = 256;
	if (r_saveshotsize->value)
	{
        float aspectRatio = glConfig.render_width / (float) glConfig.render_height;

        if (glConfig.render_height >= 1024)
            saveshotHeight = 1024;
        else if (glConfig.render_height >= 512)
            saveshotHeight = 512;

//		if (vid.width >= 1024)
//			saveshotWidth = 1024;
//		else if (vid.width >= 512)
//			saveshotWidth = 512;
        
        saveshotWidth = saveshotHeight * aspectRatio;
        
	}
	/*	if (r_saveshotsize->value && (vid.width >= 1024) && (vid.height >= 1024))
	saveshotsize = 1024;
	else if (r_saveshotsize->value && (vid.width >= 512) && (vid.height >= 512))
	saveshotsize = 512;
	else
	saveshotsize = 256;*/

	// Allocate room for reduced screenshot
	pngdata = Z_TagMalloc(saveshotWidth * saveshotHeight * 3, TAG_RENDERER);
	if (!pngdata)	return;

	// Resize grabbed screen
	if (!stbir_resize_uint8((uint8_t *) saveshotdata, (int) glConfig.render_width, (int) glConfig.render_height, 0, (uint8_t *) pngdata, (int) saveshotWidth, (int) saveshotHeight, 0, 3)) {
		VID_Printf (PRINT_ALL, "Menu_ScreenShot: Couldn't create %s\n", name); 
		return;
	}

	Com_sprintf (shotname, sizeof(shotname), "%s", name);

	result = stbi_write_png(shotname,saveshotWidth, saveshotHeight, 3, pngdata,0);

	if(!result)
	{
		VID_Printf (PRINT_ALL, "Menu_ScreenShot: Couldn't create %s\n", name); 
		return;
	}

	// Free Reduced screenshot
	Z_Free(pngdata);
    Z_Free(saveshotdata);
    saveshotdata = NULL;
}


/* 
================== 
R_GrabScreen
================== 
*/
void R_GrabScreen (void)
{	
	fbo_t *currentFBO = glState.currentFBO;
	fbo_t captureFBO;

    // Free saveshot buffer first
	if (saveshotdata)
		Z_Free(saveshotdata);
	// Allocate room for a copy of the framebuffer
	saveshotdata = Z_TagMalloc(  glConfig.render_width * glConfig.render_height * 3, TAG_RENDERER);
	R_InitFBO(&captureFBO);

    R_GenFBO(glConfig.render_width,glConfig.render_height,0,GL_RGB8,&captureFBO);
	R_BindFBO(&captureFBO);
	R_BlitFlipped(lastFrame->texture);
	R_BindFBO(currentFBO);

	GL_MBind(0,captureFBO.texture);
	// Read the framebuffer into our storage
	glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,saveshotdata);
	GL_MBind(0,0);
	R_DelFBO(&captureFBO);
}



/* 
================== 
R_ScreenShot_PNG
================== 
*/
void R_ScreenShot_PNG (qboolean silent)
{
	byte							*rgbdata;
	FILE							*file;
	char							picname[80], checkname[MAX_OSPATH];
	int32_t								i, result;
	fbo_t *currentFBO = glState.currentFBO;
	fbo_t captureFBO;
	qboolean srgb = (qboolean) (glConfig.srgb_framebuffer && Cvar_VariableInteger("vid_srgb"));

	// Create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

	// Knightmare- changed screenshot filenames, up to 1000 screenies
	// Find a file name to save it to 
	//strcpy(picname,"quake00.jpg");

	// rewrite this shit
	for (i=0; i<=999; i++) 
	{ 
		//picname[5] = i/10 + '0'; 
		//picname[6] = i%10 + '0'; 
		int32_t one, ten, hundred;

		hundred = i*0.01;
		ten = (i - hundred*100)*0.1;
		one = i - hundred*100 - ten*10;

		Com_sprintf (picname, sizeof(picname), "q2vr_%i%i%i.png", hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		file = fopen (checkname, "rb");
		if (file)
			fclose (file);
		else
			break;	// file doesn't exist
	} 
	if (i==1000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Couldn't create a file\n"); 
		return;
	}

	// Open the file for Binary Output

	// Allocate room for a copy of the framebuffer
	rgbdata = Z_TagMalloc(  glConfig.render_width * glConfig.render_height * 3, TAG_RENDERER);
	if(!rgbdata)
	{
		return;
	}
	R_InitFBO(&captureFBO);
	if (srgb)
	{
		glEnable(GL_FRAMEBUFFER_SRGB);
		R_GenFBO(glConfig.render_width,glConfig.render_height,0,GL_SRGB8,&captureFBO);
	} else {
		R_GenFBO(glConfig.render_width,glConfig.render_height,0,GL_RGB8,&captureFBO);
	}
	R_BindFBO(&captureFBO);
	R_BlitWithGammaFlipped(lastFrame->texture, vid_gamma);
	R_BindFBO(currentFBO);
	if (srgb)
	{
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
	GL_MBind(0,captureFBO.texture);
	// Read the framebuffer into our storage
	glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,rgbdata);

	GL_MBind(0,0);

	result = stbi_write_png(checkname,captureFBO.width, captureFBO.height, 3, rgbdata,0);

	// Free Temp Framebuffer
	Z_Free(rgbdata);
	R_DelFBO(&captureFBO);

	if(!result)
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_PNG: Couldn't create a file\n");
		return;
	}

	// Done!
	if (!silent)
		VID_Printf (PRINT_ALL, "Wrote %s\n", picname);
}


/* 
================== 
R_ScreenShot_f
================== 
*/  
void R_ScreenShot_f (void) 
{
	R_ScreenShot_PNG (false);
}


/* 
================== 
R_ScreenShot_Silent_f
================== 
*/  
void R_ScreenShot_Silent_f (void) 
{
	R_ScreenShot_PNG(true);
}


//============================================================================== 


/*
=================
GL_UpdateSwapInterval
=================
*/
void GL_UpdateSwapInterval (void)
{
	static qboolean registering;

	// don't swap interval if loading a map
	if (registering != registration_active)
		r_swapinterval->modified = true;

	if (r_adaptivevsync->value && !glConfig.ext_swap_control_tear)
		Cvar_SetInteger("r_adaptivevsync",0);

	if (r_adaptivevsync->modified)
	{

		Cvar_SetInteger("r_adaptivevsync",!!r_adaptivevsync->value);
		r_swapinterval->modified = true;
		r_adaptivevsync->modified = false;
	}
	if ( r_swapinterval->modified )
	{
		int32_t sync = 0;
		int32_t tear = (r_adaptivevsync->value ? -1 : 1);
		Cvar_SetInteger("r_swapinterval", abs((int)r_swapinterval->value));
		r_swapinterval->modified = false;
		
		sync = r_swapinterval->value;
		if (glConfig.ext_swap_control_tear)
			sync *= tear;

		registering = registration_active;
		if ( glConfig.ext_swap_control )
			SDL_GL_SetSwapInterval( (registration_active) ? 0 : sync );

		
	}
}


static struct {
	GLsync sync; 
	qboolean fenced;
	GLint64 timeout;
	int32_t timeStart;
} glFence;

void R_FrameFence (void)
{
	if (glConfig.arb_sync && !glFence.fenced && r_fencesync->value && r_swapinterval->value)
	{
		glFence.sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
//		glGetInteger64v(GL_MAX_SERVER_WAIT_TIMEOUT, &glFence.timeout);
//		glFence.timeout = 1000000;
		glFence.timeout = 0;
		glColor4f(0.0f,0.0f,0.0f,0.0f);
		GL_Disable(GL_DEPTH_TEST);
		GL_MBind(0,0);
		glBegin(GL_TRIANGLES);
		glVertex2f(0, 0);
		glVertex2f(0, 0);
		glVertex2f(0, 0);
		glEnd();
		glFence.fenced = true;
		glFence.timeStart = Sys_Milliseconds();
	} else if ((!glConfig.arb_sync && r_fencesync->value) || !r_swapinterval->value)
	{
		Cvar_SetInteger("r_fencesync",0);
	}
}

int32_t R_FrameSync (void)
{
	if (glConfig.arb_sync && glFence.fenced && r_fencesync->value)
	{
		GLenum result;
		result = glClientWaitSync(glFence.sync, GL_SYNC_FLUSH_COMMANDS_BIT, glFence.timeout);
		if (result == GL_TIMEOUT_EXPIRED)
		{
			return 0;
		} else {
			if (result == GL_WAIT_FAILED)
			{
				GLenum err = glGetError();
				if ( err != GL_NO_ERROR )
					VID_Printf (PRINT_ALL, "R_FrameSync: glGetError() = 0x%x\n", err);
			}
			glDeleteSync(glFence.sync);
			glFence.fenced = false;
			return max(Sys_Milliseconds()-glFence.timeStart,1);
		}
	}
	return -1;
}


void R_PerspectiveOffset(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar, GLfloat offset)
{

	GLfloat f = 1.0f / tanf((fovy / 2.0f) * M_PI / 180);
	GLfloat nf = 1.0f / (zNear - zFar);
	GLfloat out[4][4];

	out[0][0] = f / aspect;
	out[0][1] = 0;
	out[0][2] = 0;
	out[0][3] = 0;
	out[1][0] = 0;
	out[1][1] = f;
	out[1][2] = 0;
	out[1][3] = 0;
	out[2][0] = offset;
	out[2][1] = 0;
	out[2][2] = (zFar + zNear) * nf;
	out[2][3] = -1;
	out[3][0] = 0;
	out[3][1] = 0;
	out[3][2] = (2.0f * zFar * zNear) * nf;
	out[3][3] = 0;

	GL_LoadMatrix(GL_PROJECTION,out);
}

void R_MakePerspectiveFromScale(eyeScaleOffset_t eye, GLfloat zNear, GLfloat zFar, vec4_t out[4])
{
	GLfloat nf = 1.0f / (zNear - zFar);
	
	out[0][0] = eye.x.scale;
	out[0][1] = 0;
	out[0][2] = 0;
	out[0][3] = 0;

	out[1][0] = 0;
	out[1][1] = eye.y.scale;
	out[1][2] = 0;
	out[1][3] = 0;

	out[2][0] = -eye.x.offset;
	out[2][1] = eye.y.offset;
	out[2][2] = (zFar + zNear) * nf;
	out[2][3] = -1;

	out[3][0] = 0;
	out[3][1] = 0;
	out[3][2] = (2.0f * zFar * zNear) * nf;
	out[3][3] = 0;
}

void R_PerspectiveScale(eyeScaleOffset_t eye, GLfloat zNear, GLfloat zFar)
{
	vec4_t out[4];
	R_MakePerspectiveFromScale(eye,zNear,zFar,out);
	GL_LoadMatrix(GL_PROJECTION,out);
}

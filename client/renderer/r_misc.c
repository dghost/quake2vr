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
#include "include/jpeglib.h" // Heffo - JPEG Screenshots
#include "SDL.h"

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

typedef struct _TargaHeader {
	uint8_t 	id_length, colormap_type, image_type;
	uint16_t	colormap_index, colormap_length;
	uint8_t	colormap_size;
	uint16_t	x_origin, y_origin, width, height;
	uint8_t	pixel_size, attributes;
} TargaHeader;


/*
================
R_ResampleShotLerpLine
from DarkPlaces
================
*/
void R_ResampleShotLerpLine (byte *in, byte *out, int32_t inwidth, int32_t outwidth) 
{ 
	int32_t j, xi, oldx = 0, f, fstep, l1, l2, endx;

	fstep = (int32_t) (inwidth*65536.0f/outwidth); 
	endx = (inwidth-1); 
	for (j = 0,f = 0; j < outwidth; j++, f += fstep) 
	{ 
		xi = (int32_t) f >> 16; 
		if (xi != oldx) 
		{ 
			in += (xi - oldx) * 3; 
			oldx = xi; 
		} 
		if (xi < endx) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			*out++ = (byte) ((in[0] * l1 + in[3] * l2) >> 16);
			*out++ = (byte) ((in[1] * l1 + in[4] * l2) >> 16); 
			*out++ = (byte) ((in[2] * l1 + in[5] * l2) >> 16); 
		} 
		else // last pixel of the line has no pixel to lerp to 
		{ 
			*out++ = in[0]; 
			*out++ = in[1]; 
			*out++ = in[2]; 
		} 
	} 
}


/*
================
R_ResampleShot
================
*/
void R_ResampleShot (void *indata, int32_t inwidth, int32_t inheight, void *outdata, int32_t outwidth, int32_t outheight) 
{ 
	int32_t i, j, yi, oldy, f, fstep, l1, l2, endy = (inheight-1);
	
	byte *inrow, *out, *row1, *row2; 
	out = outdata; 
	fstep = (int32_t) (inheight*65536.0f/outheight); 

	row1 = malloc(outwidth*3); 
	row2 = malloc(outwidth*3); 
	inrow = indata; 
	oldy = 0; 
	R_ResampleShotLerpLine (inrow, row1, inwidth, outwidth); 
	R_ResampleShotLerpLine (inrow + inwidth*3, row2, inwidth, outwidth); 
	for (i = 0, f = 0; i < outheight; i++,f += fstep) 
	{ 
		yi = f >> 16; 
		if (yi != oldy) 
		{ 
			inrow = (byte *)indata + inwidth*3*yi; 
			if (yi == oldy+1) 
				memcpy(row1, row2, outwidth*3); 
			else 
				R_ResampleShotLerpLine (inrow, row1, inwidth, outwidth);

			if (yi < endy) 
				R_ResampleShotLerpLine (inrow + inwidth*3, row2, inwidth, outwidth); 
			else 
				memcpy(row2, row1, outwidth*3); 
			oldy = yi; 
		} 
		if (yi < endy) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
			} 
			row1 -= outwidth*3; 
			row2 -= outwidth*3; 
		} 
		else // last line has no pixels to lerp to 
		{ 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = *row1++; 
				*out++ = *row1++; 
				*out++ = *row1++; 
			} 
			row1 -= outwidth*3; 
		} 
	} 
	free(row1); 
	free(row2); 
} 


/* 
================== 
R_ScaledScreenshot
by Knightmare
================== 
*/

byte	*saveshotdata;

void R_ScaledScreenshot (char *name)
{
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	JSAMPROW						s[1];
	FILE							*file;
	char							shotname[MAX_OSPATH];
	int32_t								saveshotWidth, saveshotHeight, offset;
	byte							*jpgdata;

	if (!saveshotdata)	return;

	// Optional hi-res saveshots
	saveshotWidth = saveshotHeight = 256;
	if (r_saveshotsize->value)
	{
		if (vid.width >= 1024)
			saveshotWidth = 1024;
		else if (vid.width >= 512)
			saveshotWidth = 512;

		if (vid.height >= 1024)
			saveshotHeight = 1024;
		else if (vid.height >= 512)
			saveshotHeight = 512;
	}
/*	if (r_saveshotsize->value && (vid.width >= 1024) && (vid.height >= 1024))
		saveshotsize = 1024;
	else if (r_saveshotsize->value && (vid.width >= 512) && (vid.height >= 512))
		saveshotsize = 512;
	else
		saveshotsize = 256;*/

	// Allocate room for reduced screenshot
	jpgdata = malloc(saveshotWidth * saveshotHeight * 3);
	if (!jpgdata)	return;

	// Resize grabbed screen
	R_ResampleShot(saveshotdata, screenFBO.width, screenFBO.height, jpgdata, saveshotWidth, saveshotHeight);

	// Open the file for Binary Output
	Com_sprintf (shotname, sizeof(shotname), "%s", name);
	file = fopen(shotname, "wb");
	if (!file)
	{
		VID_Printf (PRINT_ALL, "Menu_ScreenShot: Couldn't create %s\n", name); 
		return;
 	}

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = saveshotWidth; //256;
	cinfo.image_height = saveshotHeight; //256;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 85, TRUE); // was 100

	// Start Compression
	jpeg_start_compress(&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &jpgdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress(&cinfo);

	// Close File
	fclose(file);

	// Free Reduced screenshot
	free(jpgdata);
}


/* 
================== 
R_GrabScreen
by Knightmare
================== 
*/
void R_GrabScreen (void)
{	
	fbo_t *currentFBO = glState.currentFBO;
	// Free saveshot buffer first
	if (saveshotdata)
		free(saveshotdata);
	// Allocate room for a copy of the framebuffer
	saveshotdata = malloc(screenFBO.width * screenFBO.height * 3);
	if (!saveshotdata)	return;

	R_BindFBO(&screenFBO);

	// Read the framebuffer into our storage
	glReadPixels(0, 0, screenFBO.width, screenFBO.height, GL_RGB, GL_UNSIGNED_BYTE, saveshotdata);
	R_BindFBO(currentFBO);
}


/* 
================== 
R_ScreenShot_JPG
By Robert 'Heffo' Heffernan
================== 
*/
void R_ScreenShot_JPG (qboolean silent)
{
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rgbdata;
	JSAMPROW						s[1];
	FILE							*file;
	char							picname[80], checkname[MAX_OSPATH];
	int32_t								i, offset;
	fbo_t *currentFBO = glState.currentFBO;

	// Create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

	// Knightmare- changed screenshot filenames, up to 1000 screenies
	// Find a file name to save it to 
	//strcpy(picname,"quake00.jpg");

	for (i=0; i<=999; i++) 
	{ 
		//picname[5] = i/10 + '0'; 
		//picname[6] = i%10 + '0'; 
		int32_t one, ten, hundred;

		hundred = i*0.01;
		ten = (i - hundred*100)*0.1;
		one = i - hundred*100 - ten*10;

		Com_sprintf (picname, sizeof(picname), "q2vr_%i%i%i.jpg", hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		file = fopen (checkname, "rb");
		if (!file)
			break;	// file doesn't exist
		fclose (file);
	} 
	if (i==1000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_JPG: Couldn't create a file\n"); 
		return;
 	}

	// Open the file for Binary Output
	file = fopen(checkname, "wb");
	if(!file)
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_JPG: Couldn't create a file\n"); 
		return;
 	}

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(screenFBO.width * screenFBO.height * 3);
	if(!rgbdata)
	{
		fclose(file);
		return;
	}

	R_BindFBO(&screenFBO);

	// Read the framebuffer into our storage
	glReadPixels(0, 0, screenFBO.width, screenFBO.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);

	R_BindFBO(currentFBO);

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = screenFBO.width;
	cinfo.image_height = screenFBO.height;

	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if ((r_screenshot_jpeg_quality->value >= 101) || (r_screenshot_jpeg_quality->value <= 0))
		Cvar_Set("r_screenshot_jpeg_quality", "85");
	jpeg_set_quality(&cinfo, r_screenshot_jpeg_quality->value, TRUE);

	// Start Compression
	jpeg_start_compress(&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while(cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress(&cinfo);

	// Close File
	fclose(file);

	// Free Temp Framebuffer
	free(rgbdata);


	// Done!
	if (!silent)
		VID_Printf (PRINT_ALL, "Wrote %s\n", picname);
}


/* 
================== 
R_ScreenShot_TGA
================== 
*/  
void R_ScreenShot_TGA (qboolean silent) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int32_t			i, c, temp;
	FILE		*f;
	fbo_t *currentFBO = glState.currentFBO;

/*	// Heffo - JPEG Screenshots
	if (r_screenshot_jpeg->value)
	{
		R_ScreenShot_JPG();
		return;
	}*/

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 

	// Knightmare- changed screenshot filenames, up to 100 screenies
	//strcpy(picname,"quake00.tga");

	for (i=0; i<=999; i++) 
	{ 
		//picname[5] = i/10 + '0'; 
		//picname[6] = i%10 + '0'; 
		int32_t one, ten, hundred;

		hundred = i*0.01;
		ten = (i - hundred*100)*0.1;
		one = i - hundred*100 - ten*10;

		Com_sprintf (picname, sizeof(picname), "q2vr_%i%i%i.tga", hundred, ten, one);
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==1000) 
	{
		VID_Printf (PRINT_ALL, "R_ScreenShot_TGA: Couldn't create a file\n"); 
		return;
 	}


	buffer = malloc(screenFBO.width*screenFBO.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = screenFBO.width&255;
	buffer[13] = screenFBO.width>>8;
	buffer[14] = screenFBO.height&255;
	buffer[15] = screenFBO.height>>8;
	buffer[16] = 24;	// pixel size

	
	R_BindFBO(&screenFBO);

	// Read the framebuffer into our storage
	glReadPixels (0, 0, screenFBO.width, screenFBO.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	R_BindFBO(currentFBO);


	// swap rgb to bgr
	c = 18+screenFBO.width*screenFBO.height*3;
	for (i=18; i<c; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);

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
	// Heffo - JPEG Screenshots
	if (r_screenshot_jpeg->value)
		R_ScreenShot_JPG (false);
	else 
		R_ScreenShot_TGA (false);
}


/* 
================== 
R_ScreenShot_Silent_f
================== 
*/  
void R_ScreenShot_Silent_f (void) 
{
	// Heffo - JPEG Screenshots
	if (r_screenshot_jpeg->value)
		R_ScreenShot_JPG (true);
	else 
		R_ScreenShot_TGA (true);

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
		Cvar_SetInteger("r_swapinterval", abs(r_swapinterval->value));
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
	} else if (!glConfig.arb_sync && r_fencesync->value || !r_swapinterval->value)
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


void R_PerspectiveScale(eyeScaleOffset_t eye, GLfloat zNear, GLfloat zFar)
{
	GLfloat nf = 1.0f / (zNear - zFar);
	GLfloat out[4][4];

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

	GL_LoadMatrix(GL_PROJECTION,out);
}
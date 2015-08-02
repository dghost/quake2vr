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

// r_image.c

#include "include/r_local.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
// strictly speaking these are unnecessary, but i'm paranoid
#define STBI_MALLOC(sz) malloc(sz)
#define STBI_REALLOC(p,sz) realloc(p,sz)
#define STBI_FREE(p) free(p)
#include "include/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "include/stb_image_resize.h"


image_t         gltextures[MAX_GLTEXTURES];
int32_t			numgltextures;
int32_t			base_textureid;		// gltextures[i] = base_textureid+i

static byte     intensitytable[256];
static uint8_t  gammatable[256];

// TODO: consider removing r_intensity
cvar_t		*r_intensity;

uint32_t	d_8to24table[256];
float		d_8to24tablef[256][3]; //Knightmare- MrG's Vertex array stuff

qboolean GL_Upload8 (byte *data, int32_t width, int32_t height,  qboolean mipmap, qboolean is_sky );
qboolean GL_Upload32 (uint32_t *data, int32_t width, int32_t height,  qboolean mipmap);

int32_t		gl_solid_format = GL_RGB;
int32_t		gl_alpha_format = GL_RGBA;
int32_t		gl_raw_tex_solid_format = GL_RGB;
int32_t		gl_raw_tex_alpha_format = GL_RGBA;
int32_t		gl_compressed_tex_solid_format = GL_RGB;
int32_t		gl_compressed_tex_alpha_format = GL_RGBA;
int32_t		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int32_t		gl_filter_max = GL_LINEAR;


// store the names of last images that failed to load
static stable_t failed_images = {0, 512};

// with PCX, TGA, JPG, PNG, and WAL it used 66 bytes
static stable_t supported_image_types = {0, 128};

int s_pcx;
int s_tga;
int s_jpg;
int s_png;
int s_wal;

void GL_SetTexturePalette( uint32_t palette[256] )
{
	int32_t i;
	uint8_t temptable[768];

	if ( glColorTableEXT )//&& gl_ext_palettedtexture->value )
	{
		for ( i = 0; i < 256; i++ )
		{
			temptable[i*3+0] = ( palette[i] >> 0 ) & 0xff;
			temptable[i*3+1] = ( palette[i] >> 8 ) & 0xff;
			temptable[i*3+2] = ( palette[i] >> 16 ) & 0xff;
		}

		glColorTableEXT( GL_SHARED_TEXTURE_PALETTE_EXT,
						   GL_RGB,
						   256,
						   GL_RGB,
						   GL_UNSIGNED_BYTE,
						   temptable );
	}
}


typedef struct
{
	char *name;
	int32_t	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int32_t mode;
} gltmode_t;

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string )
{
	int32_t		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_strcasecmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		VID_Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// clamp selected anisotropy
	if (glConfig.anisotropic)
	{
		if (r_anisotropic->value > glConfig.max_anisotropy)
			Cvar_SetValue("r_anisotropic", glConfig.max_anisotropy);
		else if (r_anisotropic->value < 1.0)
			Cvar_SetValue("r_anisotropic", 1.0);
	}

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky)
		{
			GL_Bind (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			// Set anisotropic filter if supported and enabled
			if (glConfig.anisotropic && r_anisotropic->value)
			{
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);
				//GLfloat largest_supported_anisotropy;
				//glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
				//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
			}
		}
	}
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f (void)
{
	int32_t		i;
	image_t	*image;
	int32_t		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	VID_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			VID_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			VID_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			VID_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			VID_Printf (PRINT_ALL, "P");
			break;
		case it_part:
			VID_Printf (PRINT_ALL, "P");
			break;
		default:
			VID_Printf (PRINT_ALL, " ");
			break;
		}

		VID_Printf (PRINT_ALL,  " %3i %3i %s: %s\n",
			image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	VID_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int32_t			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
qboolean	scrap_dirty;

// returns a texture number and the position inside it
int32_t Scrap_AllocBlock (int32_t w, int32_t h, int32_t *x, int32_t *y)
{
	int32_t		i, j;
	int32_t		best, best2;
	int32_t		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
//	Sys_Error ("Scrap_AllocBlock: full");
}

int32_t	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload8 (scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, false );
	scrap_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int32_t *width, int32_t *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int32_t		x, y;
	int32_t		len;
	int32_t		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		VID_Printf (PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480)
	{
		VID_Printf (PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = (byte*)Z_TagMalloc ( (pcx->ymax+1) * (pcx->xmax+1) , TAG_RENDERER);

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = (byte*)Z_TagMalloc(768, TAG_RENDERER);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		VID_Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		Z_Free (*pic);
		*pic = NULL;
	}

	FS_FreeFile (pcx);
}


/*
=========================================================

STB_IMAGE LOADING

=========================================================
*/
image_t *R_LoadSTB(char *filename, imagetype_t type)
{
    image_t		*image;
	byte      *data;
	int w, h, c;
	stbi_uc *rgbadata;
	int32_t		length;

	// load file
	length = FS_LoadFile( filename, (void **) &data );

	if( !data )
		return NULL;

	rgbadata = stbi_load_from_memory(data, length, &w, &h, &c, 4);

	FS_FreeFile( data );

	if (!rgbadata)	{
		VID_Printf(PRINT_DEVELOPER, "R_LoadSTB Failed on file %s: %s\n",filename, stbi_failure_reason());
		return NULL;
	}
    
	VID_Printf(PRINT_DEVELOPER, "R_LoadSTB Succeed: %s\n",filename);
    image = R_LoadPic(filename, rgbadata, w, h, type, 32);
    STBI_FREE(rgbadata);
    return image;
}


/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	int16_t		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int32_t skinwidth, int32_t skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int32_t					inpt = 0, outpt = 0;
	int32_t					filledcolor = -1;
	int32_t					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int32_t			x = fifo[outpt].x, y = fifo[outpt].y;
		int32_t			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture (uint32_t *in, int32_t inwidth, int32_t inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int32_t		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int32_t		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int32_t width, int32_t height)
{
	int32_t		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0; j<width; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}


/*
================
GL_BuildPalettedTexture
================
*/
void GL_BuildPalettedTexture (uint8_t *paletted_texture, uint8_t *scaled, int32_t scaled_width, int32_t scaled_height)
{
	int32_t i;

	for ( i = 0; i < scaled_width * scaled_height; i++ )
	{
		uint32_t r, g, b, c;

		r = ( scaled[0] >> 3 ) & 31;
		g = ( scaled[1] >> 2 ) & 63;
		b = ( scaled[2] >> 3 ) & 31;

		c = r | ( g << 5 ) | ( b << 11 );

		paletted_texture[i] = glState.d_16to8table[c];

		scaled += 4;
	}
}


/*
===============
here starts modified code
by Heffo/changes by Nexus
===============
*/

static	int32_t		upload_width, upload_height; //** DMP made local to module
static	qboolean uploaded_paletted;			//** DMP ditto

/*
static qboolean IsPowerOf2 (int32_t value)
{
	int32_t i = 1;
	while (1) {
		if (value == i)
			return true;
		if (i > value)
			return false;
		i <<= 1;
	}
}
*/

int32_t nearest_power_of_2 (int32_t size)
{
	int32_t i = 2;

	// NeVo - infinite loop bug-fix
	if (size == 1)
		return size; 

	while (1) 
	{
		i <<= 1;
		if (size == i)
			return i;
		if (size > i && size < (i <<1)) 
		{
			if (size >= ((i+(i<<1))/2))
				return i<<1;
			else
				return i;
		}
	};
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
qboolean GL_Upload32 (uint32_t *data, int32_t width, int32_t height, qboolean mipmap)
{
	int32_t			samples;
	uint32_t 	*scaled;
	int32_t			scaled_width, scaled_height;
	int32_t			i, c;
	byte		*scan;
	int32_t			comp;

	uploaded_paletted = false;

	//
	// scan the texture for any non-255 alpha
	//
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
	{
		comp = gl_compressed_tex_solid_format;
	}
	else if (samples == gl_alpha_format)
	{
		comp = gl_compressed_tex_alpha_format;
	}

	//
	// find sizes to scale to
	//
	if (!mipmap || r_nonpoweroftwo_mipmaps->value) {
		scaled_width = width;
		scaled_height = height;
	}
	else {
		scaled_width = nearest_power_of_2 (width);
		scaled_height = nearest_power_of_2 (height);
	}

	if (scaled_width > glConfig.max_texsize)
		scaled_width = glConfig.max_texsize;
	if (scaled_height > glConfig.max_texsize)
		scaled_height = glConfig.max_texsize;

	//
	// allow sampling down of the world textures for speed
	//
	if (mipmap && (int32_t)r_picmip->value > 0)
	{
		int32_t maxsize;

		if ((int32_t)r_picmip->value == 1)		// clamp to 512x512
			maxsize = 512;
		else if ((int32_t)r_picmip->value == 2) // clamp to 256x256
			maxsize = 256;
		else								// clamp to 128x128
			maxsize = 128;

		while (1) {
			if (scaled_width <= maxsize && scaled_height <= maxsize)
				break;
			scaled_width >>= 1;
			scaled_height >>= 1;
		}
		//scaled_width >>= (int32_t)r_picmip->value;
		//scaled_height >>= (int32_t)r_picmip->value;
	}

	//
	// resample texture if needed
	//
	if (scaled_width != width || scaled_height != height) 
	{
		scaled = (uint32_t*)Z_TagMalloc((scaled_width * scaled_height) * 4, TAG_RENDERER);
		if (!stbir_resize_uint8((uint8_t *) data, (int) width, (int) height, 0, (uint8_t *) scaled, (int) scaled_width, (int) scaled_height, 0, 4)) {
			scaled_width = width;
			scaled_height = height;
			scaled = data;
		}
	}
	else
	{
		scaled_width = width;
		scaled_height = height;
		scaled = data;
	}
	if (r_ignorehwgamma->value)
		GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );


	//
	// generate mipmaps and upload
	//
	if (mipmap)
	{
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	} 
	else
		glTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

	if (scaled_width != width || scaled_height != height)
		Z_Free(scaled);

	upload_width = scaled_width;	upload_height = scaled_height;

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? gl_filter_min : gl_filter_max);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	
	// Set anisotropic filter if supported and enabled
	if (mipmap && glConfig.anisotropic && r_anisotropic->value)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);

	return (samples == gl_alpha_format);
}

/*
===============
GL_Upload8

Returns has_alpha
===============
*/
qboolean GL_Upload8 (byte *data, int32_t width, int32_t height,  qboolean mipmap, qboolean is_sky )
{
	uint32_t	trans[512*256];
	int32_t			i, s;
	int32_t			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		VID_Error (ERR_DROP, "GL_Upload8: too large");

	/*if ( glColorTableEXT && 
		 gl_ext_palettedtexture->value && 
		 is_sky )
	{
		glTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  width,
					  height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  data );

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else*/
	{
		for (i=0 ; i<s ; i++)
		{
			p = data[i];
			trans[i] = d_8to24table[p];

			if (p == 255)
			{	// transparent, so scan around for another color
				// to avoid alpha fringes
				// FIXME: do a full flood fill so mips work...
				if (i > width && data[i-width] != 255)
					p = data[i-width];
				else if (i < s-width && data[i+width] != 255)
					p = data[i+width];
				else if (i > 0 && data[i-1] != 255)
					p = data[i-1];
				else if (i < s-1 && data[i+1] != 255)
					p = data[i+1];
				else
					p = 0;
				// copy rgb components
				((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
				((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
				((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
			}
		}

		return GL_Upload32 (trans, width, height, mipmap);
	}
}


/*
================
R_LoadPic

This is also used as an entry point for the generated notexture
Nexus  - changes for hires-textures
================
*/
image_t *R_LoadPic (char *name, byte *pic, int32_t width, int32_t height, imagetype_t type, int32_t bits)
{
	image_t		*image;
	int32_t			i;
	//Nexus'added vars
	int32_t len; 
	char s[128]; 
    char *ext;
    int token;
	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			VID_Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];
    len = strlen(name);
    ext = name + len - 4;
    token = Q_STLookup(&supported_image_types, ext);
	if (len + 1 >= sizeof(image->name))
		VID_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
    image->name[len-3] = 'i';
    image->name[len-2] = 'm';
    image->name[len-1] = 'g';

	image->hash = Hash32(image->name, strlen(image->name));	// Knightmare added
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;
	image->replace_scale_w = image->replace_scale_h = 1.0f; // Knightmare added

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

// replacement scaling hack for TGA/JPEG HUD images and skins
// NOTE: replace this with shaders as soon as they are supported
	len = strlen(name); 
	strcpy(s,name);
	// check if we have a tga/jpg/png pic
	if ((type == it_pic) && (token != s_pcx) && (token != s_wal))
	{ 
		byte	*pic, *palette;
		int32_t		pcxwidth, pcxheight;
		s[len-3]='p'; s[len-2]='c'; s[len-1]='x'; // replace extension 
		LoadPCX (s, &pic, &palette, &pcxwidth, &pcxheight); // load .pcx file 
		
		if (pic && (pcxwidth > 0) && (pcxheight > 0)) {
			image->replace_scale_w = (float)pcxwidth/image->width;
			image->replace_scale_h = (float)pcxheight/image->height;
		}
		if (pic) Z_Free(pic);
		if (palette) Z_Free(palette);
	}	

	// load little pics into the scrap
	if (image->type == it_pic && bits == 8
		&& image->width < 64 && image->height < 64)
	{
		int32_t		x, y;
		int32_t		i, j, k;
		int32_t		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x+0.01)/(float)BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8)
			image->has_alpha = GL_Upload8 (pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky );
		else
			image->has_alpha = GL_Upload32 ((uint32_t *)pic, width, height, (image->type != it_pic && image->type != it_sky) );
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}



/*
===============
R_InitFailedImgList
===============
*/
void R_InitFailedImgList (void)
{
    Q_STFree(&failed_images);
    Q_STInit(&failed_images, failed_images.size, 16, TAG_RENDERER);
}

/*
===============
R_CheckImgFailed
===============
*/
qboolean R_CheckImgFailed (char *name)
{
	return (Q_STLookup(&failed_images, name) != -1);
}

/*
===============
R_AddToFailedImgList
===============
*/
void R_AddToFailedImgList (char *name)
{
	if (!strncmp(name, "save/", 5)) // don't add saveshots
		return;

    Q_STAutoRegister(&failed_images, name);
}

/*
================
R_LoadWal
================
*/
image_t *R_LoadWal (char *name, imagetype_t type)
{
	miptex_t	*mt;
	int32_t			width, height, ofs;
	image_t		*image;

	FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		if (type == it_wall)
			VID_Printf (PRINT_ALL, "R_FindImage: can't load %s\n", name);
		//return glMedia.notexture;
		return NULL;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = R_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	FS_FreeFile ((void *)mt);

	return image;
}

image_t *R_LoadImage(char *name, imagetype_t type) {
    image_t	*image = NULL;
    int32_t	width, height;
    int32_t len = strlen(name);
    char *ext = name + len - 4;
    int token = Q_STLookup(&supported_image_types, ext);
    
    if (token == s_pcx)
    {
        byte	*pic = NULL;
        byte    *palette = NULL;
        
        LoadPCX (name, &pic, &palette, &width, &height);
        if (pic) {
            image = R_LoadPic (name, pic, width, height, type, 8);
            Z_Free(pic);
        }
        if (palette)
            Z_Free(palette);
    }
    else if (token == s_wal)
    {
        image = R_LoadWal (name, type);
    }
    else if (token != -1)
    {
        // use new loading code
        image = R_LoadSTB(name, type);
    }
    return image;
}

/*
===============
R_FindImage

Finds or loads the given image
===============
*/
image_t	*R_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
    int32_t		i, len = strlen(name);
	char	s[MAX_OSPATH];
	char	*tmp;
    hash32_t hash;
    char *ext = name + len - 4;
    int token;
    
	if (!name)
		return NULL;
    
	if (len<5 || len > MAX_OSPATH)
		return NULL;
    
    token = Q_STLookup(&supported_image_types, ext);
    
    // not an image file
    if (token == -1)
        return NULL;
    
	// fix up bad image paths
    tmp = name;
    while ( *tmp != 0 )
    {
        if ( *tmp == '\\' )
            *tmp = '/';
        tmp++;
    }
    strcpy(s, name);
    s[len-3] = 'i';
    s[len-2] = 'm';
    s[len-1] = 'g';
    
    
    // don't try again to load an image that just failed
    if (R_CheckImgFailed (s))
    {
        return NULL;
    }
    
    hash = Hash32(s, len);
    
	// look for it
	for (i=0, image=gltextures; i<numgltextures; i++,image++)
	{
		if (!HashEquals32(hash, image->hash) && !strcmp(s, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	// MrG's automatic JPG & TGA loading
	// search for TGAs, PNGs, and JPGs to replace .pcx and .wal images
    if (token == s_tga)
    {
        char fext[2][4] = {"png","jpg"};
        char temp[MAX_OSPATH];
        
        int i;
        strcpy(temp,name);
        for (i = 0; i < 2; i++) {
            char *e = fext[i];
            temp[len-3] = e[0]; temp[len-2] =e [1]; temp[len-1] = e[2];
            image = R_LoadImage(temp,type);
            if (image)
                return image;
        }
    } else	if ((token == s_pcx) || (token == s_wal))
    {
        char fext[3][4] = {"png","jpg","tga"};
        char temp[MAX_OSPATH];
        
        int i;
        strcpy(temp,name);
        for (i = 0; i < 3; i++) {
            char *e = fext[i];
            temp[len-3] = e[0]; temp[len-2] =e [1]; temp[len-1] = e[2];
            image = R_LoadImage(temp,type);
            if (image)
                return image;
        }
    }

    image = R_LoadImage(name, type);
    
	if (!image)
		R_AddToFailedImgList(s);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return R_FindImage (name, it_skin);
}


/*
================
R_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void R_FreeUnusedImages (void)
{
	int32_t		i;
	image_t	*image;

	// never free notexture or particle textures
	glMedia.notexture->registration_sequence = registration_sequence;
	glMedia.whitetexture->registration_sequence = registration_sequence;
#ifdef ROQ_SUPPORT
	glMedia.rawtexture->registration_sequence = registration_sequence;
#endif // ROQ_SUPPORT
	glMedia.envmappic->registration_sequence = registration_sequence;
	glMedia.spheremappic->registration_sequence = registration_sequence;
	glMedia.causticwaterpic->registration_sequence = registration_sequence;
	glMedia.causticslimepic->registration_sequence = registration_sequence;
	glMedia.causticlavapic->registration_sequence = registration_sequence;
	glMedia.shelltexture->registration_sequence = registration_sequence;
	glMedia.particlebeam->registration_sequence = registration_sequence;

	for (i=0; i<PARTICLE_TYPES; i++)
		if (glMedia.particletextures[i]) // dont mess with null ones silly :p
			glMedia.particletextures[i]->registration_sequence = registration_sequence;

	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics

		//Heffo - Free Cinematic
		//if(image->is_cin)
		//	CIN_FreeCin(image->texnum);

		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
    Q_STAutoPack(&failed_images);
}


/*
===============
Draw_GetPalette
===============
*/
int32_t Draw_GetPalette (void)
{
	int32_t		i;
	int32_t		r, g, b;
	uint32_t	v;
	byte	*pic, *pal;
	int32_t		width, height;
	//int32_t		avg, dr, dg, db, d1, d2, d3;	//** DMP
	//float	sat;							//** DMP

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		VID_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx");

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		
		//** DMP adjust saturation
		/*avg = (r + g + b + 2) / 3;			// first calc grey value we'll desaturate to
		dr = avg - r;						// calc distance from grey value to each gun
		dg = avg - g;
		db = avg - b;
		d1 = abs(r - g);					// find greatest distance between all the guns
		d2 = abs(g - b);
		d3 = abs(b - r);
		if (d1 > d2)						// we will use this for our existing saturation
			if (d1 > d3)
				sat = d1;
			else
				if (d2 > d3)
					sat = d2;
				else
					sat = d3;
		else
			if (d2 > d3)
				sat = d2;
			else
				sat = d3;
		sat /= 255.0;						// convert existing saturationn to ratio
		sat = 1.0 - sat;					// invert so the most saturation causes the desaturation to lessen
		sat *= (1.0 - r_saturation->value); // scale the desaturation value so our desaturation is non-linear (keeps lava looking good)
		dr *= sat;							// scale the differences by the amount we want to desaturate
		dg *= sat;
		db *= sat;
		r += dr;							// now move the gun values towards total grey by the amount we desaturated
		g += dg;
		b += db;*/
		//** DMP end saturation mod

		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	Z_Free (pic);
	Z_Free (pal);

	return 0;
}

/*
 ===============
 R_IsSupportedImage
 ===============
 */
qboolean R_IsSupportedImageType(const char *name) {
    if (supported_image_types.st)
        return (Q_STLookup(&supported_image_types, name) != -1);
    return false;
}

/*
===============
R_InitImages
===============
*/
void R_InitImages (void)
{
	int32_t		i, j;
	float	g = 1.0f / vid_gamma;

    Q_STInit(&supported_image_types, supported_image_types.size, 5, TAG_RENDERER);
    
    s_pcx = Q_STAutoRegister(&supported_image_types, ".pcx");
    s_wal = Q_STAutoRegister(&supported_image_types, ".wal");
    s_tga = Q_STAutoRegister(&supported_image_types, ".tga");
    s_jpg = Q_STAutoRegister(&supported_image_types, ".jpg");
    s_png = Q_STAutoRegister(&supported_image_types, ".png");
    
    Q_STAutoPack(&supported_image_types);
    
	// Knightmare- reinitialize these after a vid_restart
	// this is needed because the renderer is no longer a DLL
	gl_solid_format = GL_RGB;
	gl_alpha_format = GL_RGBA;
	gl_raw_tex_solid_format = GL_RGB;
	gl_raw_tex_alpha_format = GL_RGBA;

	if (r_texturecompression->value > 1 && glConfig.arb_texture_compression_bptc) 
	{
		gl_compressed_tex_solid_format = GL_COMPRESSED_RGBA_BPTC_UNORM;
		gl_compressed_tex_alpha_format = GL_COMPRESSED_RGBA_BPTC_UNORM;
	} else if (r_texturecompression->value && glConfig.ext_texture_compression_s3tc) {
		gl_compressed_tex_solid_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		gl_compressed_tex_alpha_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	} else {
		gl_compressed_tex_solid_format = GL_RGB;
		gl_compressed_tex_alpha_format = GL_RGBA;
	}
	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
	gl_filter_max = GL_LINEAR;

	registration_sequence = 1;

	// init intensity conversions
	//r_intensity = Cvar_Get ("r_intensity", "2", CVAR_ARCHIVE);

	// Knightmare- added Vic's RGB brightening
	r_intensity = Cvar_Get ("r_intensity", "1", 0);
	// end Knightmare

	if ( r_intensity->value <= 1 )
		Cvar_Set( "r_intensity", "1" );

	glState.inverse_intensity = 1 / r_intensity->value;

	R_InitFailedImgList (); // Knightmare added

	Draw_GetPalette ();

	if (glColorTableEXT)
	{
		FS_LoadFile( "pics/16to8.dat", (void **) &glState.d_16to8table );
		if ( !glState.d_16to8table )
			VID_Error( ERR_FATAL, "Couldn't load pics/16to8.pcx");
	}

	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;
			
			inf = (int32_t)(255 * pow( i/255.0f, g) + 0.5f);
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*r_intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}
}

/*
===============
R_FreePic
by Knightmare
Frees a single pic
===============
*/
void R_FreePic (char *name)
{
	int32_t		i;
    char s[MAX_OSPATH];
	image_t	*image;
    hash32_t hash;
    int32_t len = strlen(name);
    strcpy(s, name);
    s[len-3] = 'i';
    s[len-2] = 'm';
    s[len-1] = 'g';
    hash = Hash32(s, len);

    
   	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type != it_pic)
			continue;		// only free pics
		if (!HashEquals32(hash, image->hash) && !strcmp(s, image->name))
		{
			//Heffo - Free Cinematic
			//if (image->is_cin)
			//	CIN_FreeCin(image->texnum);

			// free it
			glDeleteTextures (1, &image->texnum);
			memset (image, 0, sizeof(*image));
			return; //we're done here
		}
	}
}

/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages (void)
{
	int32_t		i;
	image_t	*image;
    
    
	for (i=0, image=gltextures; i<numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		//Heffo - Free Cinematic
		//if (image->is_cin)
		//	CIN_FreeCin(image->texnum);

		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
    Q_STFree(&failed_images);
}


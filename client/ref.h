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
#ifndef __REF_H
#define __REF_H

#include "../qcommon/qcommon.h"

#define TWOTHIRDS 0.666666666666666666666666666666

#define DIV40 0.025
#define DIV64 0.015625
#define DIV254BY255 0.9960784313725490196078431372549
#define DIV255 0.003921568627450980392156862745098
#define DIV256 0.00390625
#define DIV512 0.001953125

#define Vector4Set(v, w, x, y, z)	(v[0]=(w), v[1]=(x), v[2]=(y), v[3]=(z))
#define Vector2Set(v, x, y)	(v[0]=(x), v[1]=(y))
#define Vector2Copy(v, w)	(w[0]=v[0], w[1]=v[1])

#define	MAX_DLIGHTS		64 // was 32
#define	MAX_ENTITIES	2048 // was 128
#define	MAX_PARTICLES	8192 // was 4096
#define	MAX_LIGHTSTYLES	256

#define MAX_DECAL_VERTS			256
#define MAX_VERTS_PER_FRAGMENT	8
#define MAX_FRAGMENTS_PER_DECAL	32
#define MAX_DECAL_FRAGS			8192

#define POWERSUIT_SCALE		1.0F
#define WEAPON_SHELL_SCALE	0.25F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
//#define SHELL_RB_COLOR		0x86
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7

typedef struct entity_s
{
	struct model_s		*model;			// opaque type outside refresh
	float				angles[3];
	//vec3_t				axis[3];		// Rotation vectors

	// most recent data
	float				origin[3];		// also used as RF_BEAM's "from"
	int32_t					frame;			// also used as RF_BEAM's diameter

	// previous data for lerping
	float				oldorigin[3];	// also used as RF_BEAM's "to"
	int32_t					oldframe;

	// misc
	float	backlerp;				// 0.0 = current, 1.0 = old
	int32_t		skinnum;				// also used as RF_BEAM's palette index

	int32_t		lightstyle;				// for flashing entities
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set

	float	scale;

	struct image_s	*skin;			// NULL for inline skin
	int32_t		flags;
	int32_t		renderfx;

} entity_t;

#define ENTITY_FLAGS  68

typedef struct
{
	qboolean spotlight;
	vec3_t	direction;
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
} dlight_t;

typedef struct
{
	float	strength;
	vec3_t	direction;
	vec3_t	color;
} m_dlight_t;


//***********************************
//psychospaz
//particles
//***********************************

#define PART_GRAVITY_LIGHT		0x00001
#define PART_GRAVITY_HEAVY		0x00002
#define PART_ANGLED				0x00004
#define PART_DIRECTION			0x00008
#define PART_TRANS				0x00010
#define PART_SATURATE			0x00020
#define PART_SHADED				0x00040
#define PART_LIGHTNING			0x00080
#define PART_BEAM				0x00100
#define PART_LENSFLARE			0x00200
#define PART_ADD_LIGHT			0x00400
#define PART_DECAL				0x00800
#define PART_INSTANT			0x01000
#define PART_OVERBRIGHT			0x02000
#define PART_ALPHACOLOR			0x04000

#define PART_DEPTHHACK_SHORT	0x08000 //32768
#define PART_DEPTHHACK_MID		0x10000 //65536
#define PART_DEPTHHACK_LONG		0x20000 //128K
#define PART_LEAVEMARK			0x40000 //256K
#define PART_SPARK				0x80000 //512K

//combo flags
#define PART_DEPTHHACK	(PART_DEPTHHACK_SHORT|PART_DEPTHHACK_MID|PART_DEPTHHACK_LONG)

typedef struct
{
	int32_t numPoints;
	int32_t	firstPoint;
	struct mnode_s	*node;
} markFragment_t;

typedef struct decalpolys_s decalpolys_t;
struct decalpolys_s
{
	decalpolys_t *next; // for allocation linked list
	decalpolys_t *nextpoly; // for linked list
	qboolean	clearflag;
	int32_t		numpolys;
	vec3_t	polys[MAX_VERTS_PER_FRAGMENT];
	vec2_t	coords[MAX_VERTS_PER_FRAGMENT];
	struct mnode_s	*node;
};

//Knightmare- Psychospaz's enhanced particle code
typedef struct
{
	vec3_t	angle;
	vec3_t	origin;
	int32_t		color;
	
	float	size;
	int32_t		flags;
	float	alpha;

	int32_t		blendfunc_src;
	int32_t		blendfunc_dst;

	float	red;
	float	green;
	float	blue;

	decalpolys_t *decal;

	int32_t		image;
} particle_t;

// end Knightmare

typedef struct
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;


// Knightmare- added Psychospaz's menu cursor
//cursor - psychospaz
#define MENU_CURSOR_BUTTON_MAX 2

#define MENUITEM_ACTION		1
#define MENUITEM_ROTATE		2
#define MENUITEM_SLIDER		3
#define MENUITEM_TEXT		4

typedef struct
{
	//only 2 buttons for menus
	float		buttontime[MENU_CURSOR_BUTTON_MAX];
	int32_t			buttonclicks[MENU_CURSOR_BUTTON_MAX];
	int32_t			buttonused[MENU_CURSOR_BUTTON_MAX];
	qboolean	buttondown[MENU_CURSOR_BUTTON_MAX];

	qboolean	mouseaction;

	//this is the active item that cursor is on.
	int32_t			menuitemtype;
	void		*menuitem;
	void		*menu;

	//coords
	int32_t		x;
	int32_t		y;

	int32_t		oldx;
	int32_t		oldy;
} cursor_t;

typedef struct
{
	int32_t			x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		aimangles[3];
	float		aimstart[3];
	float		aimend[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	int32_t			rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int32_t			num_entities;
	entity_t	*entities;

	int32_t			num_dlights;
	dlight_t	*dlights;

	int32_t			num_particles;
	particle_t	*particles;

	int32_t			num_decals;
	particle_t	*decals;
} refdef_t;

#endif // __REF_H

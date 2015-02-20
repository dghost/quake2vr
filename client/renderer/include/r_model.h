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

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

typedef struct
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	Sint32			headnode;
	Sint32			visleafs;		// not including the solid leaf 0
	Sint32			firstface, numfaces;
} mmodel_t;


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_LIGHTMAPPED	0x80
#define SURF_UNDERWATER		0x100
#define SURF_UNDERSLIME		0x200
#define SURF_UNDERLAVA		0x400
#define SURF_MASK_CAUSTIC	(SURF_UNDERWATER|SURF_UNDERSLIME|SURF_UNDERLAVA)
// Psychospaz's envmapping
#define SURF_ENVMAP			0x800

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	Uint16	v[2];
	Uint32	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];
	Sint32			texWidth;	// added Q2E hack
	Sint32			texHeight;	// added Q2E hack
	Sint32			flags;
	Sint32			numframes;
	struct mtexinfo_s	*next;		// animation chain
	image_t		*image;
	image_t		*glow;		// glow overlay
} mtexinfo_t;

/*
typedef struct
{
	vec3_t		xyz;
	vec2_t		texture_st;
	vec2_t		lightmap_st;
	byte		basecolor[4];
	byte		color[4];
} mpolyvertex_t;
*/

#define	VERTEXSIZE	7

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	Sint32		numverts;

	qboolean	vertexlightset;
	byte		*vertexlightbase;
	byte		*vertexlight;
	vec3_t		center;

	Sint32			flags;			// for SURF_UNDERWATER (not needed anymore?)
	float		verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
//	mpolyvertex_t	verts[4];			// variable sized (xyz s1t1 s2t2 rgb rgba)
} glpoly_t;

typedef struct msurface_s
{
	Sint32			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	Sint32			flags;

	Sint32			firstedge;	// look up in model->surfedges[], negative numbers
	Sint32			numedges;	// are backwards edges
	
	Sint16		texturemins[2];
	Sint16		extents[2];

	Sint32			light_s, light_t;	// gl lightmap coordinates
	Sint32			light_smax, light_tmax;
	Sint32			dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps

	glpoly_t	*polys;				// multiple if warped
	struct	msurface_s	*texturechain;
	struct  msurface_s	*lightmapchain;

	mtexinfo_t	*texinfo;
	
// lighting info
	Sint32			dlightframe;
	Sint32			dlightbits[(MAX_DLIGHTS+31)>>5];	// derived from MAX_DLIGHTS
	qboolean	cached_dlight;

	Sint32			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	byte		*samples;		// [numstyles*surfsize]
	byte		*stains;		// Knightmare- added stainmaps

	void		*chain_part;
	void		*chain_ent;

	Sint32			checkCount;
	entity_t	*entity;		// entity pointer
} msurface_t;


typedef struct malphasurface_s
{
	msurface_t	*surf;
	entity_t	*entity;		// entity pointer
	struct	malphasurface_s	*surfacechain;
} malphasurface_t;


typedef struct mnode_s
{
// common with leaf
	Sint32			contents;		// -1, to differentiate from leafs
	Sint32			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	Uint16		firstsurface;
	Uint16		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	Sint32			contents;		// wil be a negative contents number
	Sint32			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	Sint32			cluster;
	Sint32			area;

	msurface_t	**firstmarksurface;
	Sint32			nummarksurfaces;
} mleaf_t;


//===================================================================

//
// Whole model
//

//Harven++ MD3 added mod_md3
typedef enum {mod_bad, mod_brush, mod_sprite, mod_alias, mod_md2 } modtype_t;
//Harven-- MD3

typedef struct model_s
{
	char		name[MAX_QPATH];
    hash_t      hash;
    
	Sint32			registration_sequence;

	modtype_t	type;
	Sint32			numframes;
	
	Sint32			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	Sint32			firstmodelsurface, nummodelsurfaces;
	Sint32			lightmap;		// only for submodels

	Sint32			numsubmodels;
	mmodel_t	*submodels;

	Sint32			numplanes;
	cplane_t	*planes;

	Sint32			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	Sint32			numvertexes;
	mvertex_t	*vertexes;

	Sint32			numedges;
	medge_t		*edges;

	Sint32			numnodes;
	Sint32			firstnode;
	mnode_t		*nodes;

	Sint32			numtexinfo;
	mtexinfo_t	*texinfo;

	Sint32			numsurfaces;
	msurface_t	*surfaces;

	Sint32			numsurfedges;
	Sint32			*surfedges;

	Sint32			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	byte		*lightdata;

	// for alias models and skins
	// Echon's per-mesh skin support
	image_t      *skins[MD3_MAX_MESHES][MAX_MD2SKINS];
	//image_t		*skins[MAX_MD2SKINS];

	Sint32			extradatasize;
	void		*extradata;

	qboolean	hasAlpha; // if model has scripted transparency

#ifdef PROJECTION_SHADOWS // projection shadows from BeefQuake R6
	//Sint32	edge_tri[MAX_TRIANGLES][3]; // make this dynamically allocated?
	Sint32			*edge_tri;
#endif // end projection shadows from BeefQuake R6
} model_t;

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_ClusterPVS (Sint32 cluster, model_t *model);

void	Mod_Modellist_f (void);

void	*Hunk_Begin (Sint32 maxsize);
void	*Hunk_Alloc (Sint32 size);
Sint32		Hunk_End (void);
void	Hunk_Free (void *base);

void	Mod_FreeAll (void);
void	Mod_Free (model_t *mod);

extern qboolean	registration_active;	// map registration flag

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

// r_main.c

#include "include/r_local.h"
#include "include/vlights.h"
#include "include/r_vr.h"
#include "include/r_stereo.h"

void R_Clear (void);

viddef_t	vid;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t	glConfig;
glstate_t	glState;
glmedia_t	glMedia;

entity_t	*currententity;
int32_t			r_worldframe; // added for trans animations
model_t		*currentmodel;

cplane_t	frustum[4];

int32_t			r_visframecount;	// bumped when going to a new PVS
int32_t			r_framecount;		// used for dlight push checking

int32_t			c_brush_calls, c_brush_surfs, c_brush_polys, c_alias_polys, c_part_polys;

float		v_blend[4];			// final blending color

int32_t			maxsize;			// Nexus

void GL_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

//float	r_world_matrix[16];
float	r_base_world_matrix[16];


//
// screen size info
//
refdef_t	r_newrefdef;

int32_t		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t  *gl_driver;

cvar_t	*con_font; // Psychospaz's console font size option
cvar_t	*con_font_size;
cvar_t	*alt_text_color;

extern cvar_t	*scr_netgraph_pos;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_ignorehwgamma; // hardware gamma
cvar_t	*vid_refresh; // refresh rate control
cvar_t	*r_lefthand;
cvar_t	*r_waterwave;	// water waves
cvar_t  *r_waterquality;
cvar_t  *r_glows;		// texture glows
cvar_t	*r_saveshotsize;//  save shot size option

cvar_t	*vid_width;
cvar_t	*vid_height;
cvar_t	*r_dlights_normal; // lerped dlights on models
cvar_t	*r_model_shading;
cvar_t	*r_model_dlights;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*r_rgbscale; // Vic's RGB brightening

cvar_t	*r_nonpoweroftwo_mipmaps;		// Knightmare- non-power-of-two texture support

cvar_t	*r_trans_lighting; // disabling of lightmaps on trans surfaces
cvar_t	*r_warp_lighting; // allow disabling of lighting on warp surfaces
cvar_t	*r_solidalpha;			// allow disabling of trans33+trans66 surface flag combining
cvar_t	*r_entity_fliproll;		// allow disabling of backwards alias model roll

cvar_t	*r_glass_envmaps; // Psychospaz's envmapping
cvar_t	*r_trans_surf_sorting; // trans bmodel sorting
cvar_t	*r_shelltype; // entity shells: 0 = solid, 1 = warp, 2 = spheremap

cvar_t	*r_lightcutoff;	//** DMP - allow dynamic light cutoff to be user-settable

cvar_t	*r_bitdepth;
cvar_t	*r_lightmap;
cvar_t	*r_shadows;
cvar_t	*r_shadowalpha;
cvar_t	*r_shadowrange;
cvar_t	*r_shadowvolumes;
cvar_t	*r_stencil;
cvar_t	*r_transrendersort; // correct trasparent sorting
cvar_t	*r_particle_lighting;	// particle lighting
cvar_t	*r_particle_min;
cvar_t	*r_particle_max;

cvar_t	*r_particledistance;
cvar_t	*r_particle_overdraw;

cvar_t	*r_dynamic;

//TODO: consider removing r_modulate
cvar_t	*r_modulate;
cvar_t	*r_nobind;
cvar_t	*r_picmip;
cvar_t	*r_skymip;
cvar_t	*r_playermip;
cvar_t	*r_showtris;
cvar_t	*r_showbbox;	// show model bounding box
cvar_t	*r_finish;
cvar_t	*r_cull;
cvar_t	*r_polyblend;
cvar_t	*r_flashblend;
cvar_t	*r_swapinterval;
cvar_t  *r_adaptivevsync;
cvar_t	*r_texturemode;
cvar_t  *r_lodbias;
cvar_t	*r_anisotropic;
cvar_t	*r_anisotropic_avail;
cvar_t	*r_lockpvs;
cvar_t  *r_texturecompression;

//cvar_t  *r_bloom;	// BLOOMS

cvar_t	*r_skydistance; //Knightmare- variable sky range
cvar_t	*r_saturation;	//** DMP

cvar_t  *r_drawnullmodel;
cvar_t  *r_fencesync;
cvar_t  *r_lateframe_decay;
cvar_t  *r_lateframe_threshold;
cvar_t  *r_lateframe_ratio;
cvar_t  *r_directstate;

cvar_t  *r_driver_workarounds;

float vid_gamma = 1.0;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int32_t		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	float alpha = v_blend[3];
	if (!r_polyblend->value)
		return;
	if (!alpha)
		return;

	GL_Disable (GL_ALPHA_TEST);
	GL_Enable (GL_BLEND);
	GL_Disable (GL_DEPTH_TEST);
	GL_DisableTexture(0);

//	GL_MatrixMode (GL_MODELVIEW);

	GL_LoadMatrix(GL_MODELVIEW, glState.axisRotation);

	rb_vertex = rb_index = 0;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+1;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+3;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, 100);
	VA_SetElem4v(colorArray[rb_vertex], v_blend);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, 100);
	VA_SetElem4v(colorArray[rb_vertex], v_blend);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, -100);
	VA_SetElem4v(colorArray[rb_vertex], v_blend);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, -100);
	VA_SetElem4v(colorArray[rb_vertex], v_blend);
	rb_vertex++;
	RB_RenderMeshGeneric (false);

	GL_Disable (GL_BLEND);
	GL_EnableTexture(0);
	//GL_Enable (GL_ALPHA_TEST);

	glColor4f (1,1,1,1);
}

//=======================================================================

int32_t SignbitsForPlane (cplane_t *out)
{
	int32_t	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int32_t		i;
	float	fov_x = r_newrefdef.fov_x;
	float	fov_y = r_newrefdef.fov_y;

	// hack to keep objects from disappearing at the edges when projection matrix is offset
	if (vr_enabled->value)
	{
//		fov_y *= 1.30;
		fov_x += 20;
	}
	
#if 0
	//
	// this code is wrong, since it presume a 90 degree FOV both in the
	// horizontal and vertical plane
	//
	// front side is visible
	VectorAdd (vpn, vright, frustum[0].normal);
	VectorSubtract (vpn, vright, frustum[1].normal);
	VectorAdd (vpn, vup, frustum[2].normal);
	VectorSubtract (vpn, vup, frustum[3].normal);

	// we theoretically don't need to normalize these vectors, but I do it
	// anyway so that debugging is a little easier
	VectorNormalize( frustum[0].normal );
	VectorNormalize( frustum[1].normal );
	VectorNormalize( frustum[2].normal );
	VectorNormalize( frustum[3].normal );
#else
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - fov_y / 2 ) );
#endif

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int32_t i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);
// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_newrefdef.vieworg, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_newrefdef.vieworg, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_newrefdef.vieworg, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];


	c_brush_calls = 0;
	c_brush_surfs = 0;
	c_brush_polys = 0;
	c_alias_polys = 0;
	c_part_polys = 0;
}

void R_SetProjection(eyeScaleOffset_t scaleOffset)
{
	static GLfloat farz;
	GLfloat boxsize;
	// Knightmare- variable sky range
	// calc farz falue from skybox size
	if (r_skydistance->modified)
	{
		r_skydistance->modified = false;
		boxsize = r_skydistance->value;
		boxsize -= 252 * ceil(boxsize / 2300);
		farz = 1.0;
		while (farz < boxsize) //make this a power of 2
		{
			farz *= 2.0;
			if (farz >= 65536) //don't make it larger than this
				break;
		}
		farz *= 2.0; //double since boxsize is distance from camera to edge of skybox
		//not total size of skybox
		VID_Printf(PRINT_DEVELOPER, "farz now set to %g\n", farz);
	}
	// end Knightmare

	//
	// set up projection matrix
	//

	R_PerspectiveScale(scaleOffset, 1, farz); //was 4096
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL(void)
{
	//	float	yfov;
	vec3_t vieworigin;

	vec_t temp1[4][4], temp2[4][4], fin[4][4];

	// Knightmare- update r_modulate in real time
	if (r_modulate->modified && (r_worldmodel)) //Don't do this if no map is loaded
	{
		msurface_t *surf;
		int32_t i;

		for (i = 0, surf = r_worldmodel->surfaces; i < r_worldmodel->numsurfaces; i++, surf++)
			surf->cached_light[0] = 0;

		r_modulate->modified = 0;
	}

	RotationMatrix (-r_newrefdef.viewangles[2],  1, 0, 0, temp2);
	MatrixMultiply (temp2,glState.axisRotation,temp1);
	RotationMatrix (-r_newrefdef.viewangles[0],  0, 1, 0, temp2);
	MatrixMultiply (temp2,temp1,fin);
	RotationMatrix (-r_newrefdef.viewangles[1],  0, 0, 1, temp2);
	MatrixMultiply (temp2,fin,temp1);

	VectorCopy(r_newrefdef.vieworg,vieworigin);
	TranslationMatrix (-vieworigin[0],  -vieworigin[1],  -vieworigin[2],temp2);
	MatrixMultiply (temp2,temp1,fin);

	GL_LoadMatrix(GL_MODELVIEW, fin);

	GL_CullFace(GL_FRONT);

	//
	// set drawing parms
	//
	if (r_cull->value)
		GL_Enable(GL_CULL_FACE);
	else
		GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Enable(GL_DEPTH_TEST);

	rb_vertex = rb_index = 0;
}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	GLbitfield	clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;	// bitshifter's consolidation

	gldepthmin = 0;
	gldepthmax = 1;
	GL_DepthFunc (GL_LEQUAL);

	GL_DepthRange (gldepthmin, gldepthmax);

	// added stencil buffer
	if (glConfig.have_stencil)
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
//	GL_DepthRange (gldepthmin, gldepthmax);

	if (clearBits)	// bitshifter's consolidation
		glClear(clearBits);
}


/*
=============
R_Flash
=============
*/
void R_Flash (void)
{
	R_PolyBlend ();
}


/*
=============
R_DrawLastElements
=============
*/
void R_DrawLastElements (void)
{
	//if (parts_prerender)
	//	R_DrawParticles(parts_prerender);
	if (ents_trans)
		R_DrawEntitiesOnList(ents_trans);
}
//============================================

void SCR_DrawCrosshair();
void SCR_TouchPics ();
image_t	*R_DrawFindPic (char *name);
extern cvar_t *crosshair;
extern cvar_t *crosshair_scale;
extern cvar_t *crosshair_alpha;
extern cvar_t *crosshair_pulse;
extern char		crosshair_pic[MAX_QPATH];
extern qboolean scr_hidehud;

void VR_DrawCrosshair()
{
	GLenum src, dst;
	qboolean blend, depth, alphatest;

	float	scaledSize, alpha, pulsealpha;
	
	if (!vr_enabled->value || scr_hidehud)
		return;
	
	SCR_DrawCrosshair();
	
	scaledSize = crosshair_scale->value * 3;
	pulsealpha = crosshair_alpha->value * crosshair_pulse->value;	
	alpha = max(min(crosshair_alpha->value - pulsealpha + pulsealpha*sin(anglemod(r_newrefdef.time*5)), 1.0), 0.0);
	
	blend = glState.blend;
	depth = glState.depthTest;
	alphatest = glState.alphaTest;
	src = glState.blendSrc;
	dst = glState.blendDst;

	if (depth)
		GL_Disable(GL_DEPTH_TEST);
	if (alphatest)
		GL_Disable(GL_ALPHA_TEST);
	if (!blend)
		GL_Enable(GL_BLEND);

	GL_BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	if (crosshair->value)
	{
		float y,x, depth;
		image_t		*gl;
		vec3_t origin, up,right,temp;
		vec3_t forward, delta;

		gl = R_DrawFindPic (crosshair_pic);
		GL_MBind(0,gl->texnum);

		AngleVectors(r_newrefdef.aimangles,forward,right,up);
		VectorCopy(r_newrefdef.aimend,origin);
		VectorSubtract(r_newrefdef.aimend,r_newrefdef.vieworg,delta);

		// calculate coordinates for hud

		depth = fabsf(DotProduct(forward,delta));
		x = tanf(scaledSize * (M_PI/180.0f) * 0.5) * depth;
		y = x;

		VectorScale(up,y,up);
		VectorScale(right,x,right);

		glColor4f(1.0,1.0,1.0,alpha);
		glBegin(GL_TRIANGLE_STRIP);

		VectorSubtract(origin,up,temp);
		VectorSubtract(temp,right,temp);
		glTexCoord2f (0, 1); glVertex3f (temp[0],temp[1],temp[2]);

		VectorAdd(origin,up,temp);
		VectorSubtract(temp,right,temp);
		glTexCoord2f (0, 0); glVertex3f (temp[0],temp[1],temp[2]);

		VectorSubtract(origin,up,temp);
		VectorAdd(temp,right,temp);
		glTexCoord2f (1, 1); glVertex3f (temp[0],temp[1],temp[2]);

		VectorAdd(origin,up,temp);
		VectorAdd(temp,right,temp);
		glTexCoord2f (1, 0); glVertex3f (temp[0],temp[1],temp[2]);

		glEnd();
		GL_MBind(0,0);
	}

	if (vr_aimlaser->value)
	{
		GL_MBind(0,0);
		glColor4f(1.0,0.0,0.0,alpha);
		glLineWidth( crosshair_scale->value * vrState.pixelScale );
		glBegin (GL_LINES);
		glVertex3f(r_newrefdef.aimstart[0],r_newrefdef.aimstart[1],r_newrefdef.aimstart[2]);	
		glVertex3f(r_newrefdef.aimend[0],r_newrefdef.aimend[1],r_newrefdef.aimend[2]);
		glEnd ();
	}
	if (depth)
		GL_Enable(GL_DEPTH_TEST);
	if (alphatest)
		GL_Enable(GL_ALPHA_TEST);
	if (!blend)
		GL_Disable(GL_BLEND);

	GL_BlendFunc(src,dst);
}

/*
================
R_RenderCommon


================
*/
extern cvar_t *cl_paused;
void R_RenderCommon (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		VID_Error (ERR_DROP, "R_RenderCommon: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_calls = 0;
		c_brush_surfs = 0;
		c_brush_polys = 0;
		c_alias_polys = 0;
		c_part_polys = 0;
	}

	R_PushDlights ();

	if (r_finish->value)
		glFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_MarkLeaves ();	// done here so we know if we're in water
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
extern cvar_t *cl_paused;
void R_RenderView (refdef_t *fd)
{
	int32_t		x, x2, y2, y, w, h;
	eyeScaleOffset_t projection;

	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

	//
	// set up viewport
	//
	x = floorf(r_newrefdef.x * vid.width / vid.width);
	x2 = ceilf((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floorf(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceilf(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	glViewport(x, y2, w, h);


	R_SetupGL ();

    projection.y.scale = 1.0f / tanf((r_newrefdef.fov_y / 2.0f) * M_PI / 180);
	projection.y.offset = 0.0;
	projection.x.scale = 1.0f / tanf((r_newrefdef.fov_x / 2.0f) * M_PI / 180);
	projection.x.offset = 0;

	R_SetProjection(projection);
    R_SetFog();

	R_DrawWorld ();
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) // options menu
	{
		qboolean fog_on = glState.fog;
		//Knightmare- no fogging on menu/hud models
		if (fog_on) //check if fog is enabled
			GL_Disable(GL_FOG); //if so, disable it

		//R_DrawAllDecals();
		R_DrawAllEntities(false);
		R_DrawAllParticles();

		//re-enable fog if it was on
		if (fog_on)
			GL_Enable(GL_FOG);
	}
	else
	{
		GL_Disable (GL_ALPHA_TEST);

		R_RenderDlights();

		if (r_transrendersort->value) {
			//R_BuildParticleList();
			R_SortParticlesOnList();
			R_DrawAllDecals();
			//R_DrawAllEntityShadows();
			R_DrawSolidEntities();
			R_DrawEntitiesOnList(ents_trans);
		}
		else {
			R_DrawAllDecals();
			//R_DrawAllEntityShadows();
			R_DrawAllEntities(true);
		}

		R_DrawAllParticles ();

		VR_DrawCrosshair();
        
        R_DrawEntitiesOnList(ents_viewweaps);
        
        if (r_particle_overdraw->value)
        {
            R_ParticleStencil (1);
            
            R_DrawAlphaSurfaces ();
            
            R_ParticleStencil (2);
            R_DrawAllParticles ();
            R_ParticleStencil (3);
        } else {
            R_DrawAlphaSurfaces ();
        }
        
		// always draw vwep last...
		R_DrawEntitiesOnList(ents_viewweaps_trans);
		
		R_Flash();
	}
}

/*
================
R_RenderViewIntoFBO

r_newrefdef must be set before the first call
================
*/
extern cvar_t *cl_paused;
void R_DrawCameraEffect ();
void R_RenderViewIntoFBO (refdef_t *fd, eye_param_t parameters, fbo_t *destination, vrect_t *viewRect)
{
	uint32_t oldWidth, oldHeight;

	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
			VID_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

	if (!( r_newrefdef.rdflags & RDF_NOWORLDMODEL ))
	{
		vec3_t tmp;

		VectorScale( vright, parameters.viewOffset[0] , tmp );
		VectorAdd( r_newrefdef.vieworg, tmp, r_newrefdef.vieworg );

		VectorScale( vpn, parameters.viewOffset[1], tmp );
		VectorAdd( r_newrefdef.vieworg, tmp, r_newrefdef.vieworg );

		VectorScale(vup, parameters.viewOffset[2] , tmp );
		VectorAdd( r_newrefdef.vieworg, tmp, r_newrefdef.vieworg );
	}

	VectorCopy (r_newrefdef.vieworg, r_origin);


	oldWidth = vid.width;
	oldHeight = vid.height;
		
	R_BindFBO(destination);

	if (viewRect) {
		int32_t		x, x2, y2, y, w, h;

		r_newrefdef.x = viewRect->x;
		r_newrefdef.y = viewRect->y;
		r_newrefdef.width = viewRect->width;
		r_newrefdef.height = viewRect->height;
		x = floorf(r_newrefdef.x * vid.width / vid.width);
		x2 = ceilf((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
		y = floorf(vid.height - r_newrefdef.y * vid.height / vid.height);
		y2 = ceilf(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

		w = x2 - x;
		h = y - y2;

		glViewport(x, y2, w, h);
	}


	R_SetupGL ();
	R_SetProjection(parameters.projection);
    R_SetFog();

	R_DrawWorld ();
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) // options menu
	{
		qboolean fog_on = glState.fog;

		// re-execute these, because we're not rendering the world
		//R_PushDlights ();
		R_SetFrustum ();
		//R_MarkLeaves ();	
		
		//Knightmare- no fogging on menu/hud models
		if (fog_on) //check if fog is enabled
			GL_Disable(GL_FOG); //if so, disable it
		//R_DrawAllDecals();
		R_DrawAllEntities(false);
		R_DrawAllParticles();

		//re-enable fog if it was on
		if (fog_on)
			GL_Enable(GL_FOG);
	}
	else
	{
		GL_Disable (GL_ALPHA_TEST);

		R_RenderDlights();

		if (r_transrendersort->value) {
			//R_BuildParticleList();
			R_SortParticlesOnList();
			R_DrawAllDecals();
			//R_DrawAllEntityShadows();
			R_DrawSolidEntities();
			R_DrawEntitiesOnList(ents_trans);
		}
		else {
			R_DrawAllDecals();
			//R_DrawAllEntityShadows();
			R_DrawAllEntities(true);
		}

		R_DrawAllParticles ();

		VR_DrawCrosshair();

		R_DrawEntitiesOnList(ents_viewweaps);

        if (r_particle_overdraw->value)
        {
            R_ParticleStencil (1);
            
            R_DrawAlphaSurfaces ();
            
            R_ParticleStencil (2);
            R_DrawAllParticles ();
            R_ParticleStencil (3);
        } else {
            R_DrawAlphaSurfaces ();
        }

		// always draw vwep last...
		R_DrawEntitiesOnList(ents_viewweaps_trans);

		//GL_Disable(GL_ALPHA_TEST);

		GL_MBind(0,destination->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
		glGenerateMipmap(GL_TEXTURE_2D);
		GL_MBind(0,0);

		R_ApplyPostProcess(destination);

	}

	if ((r_newrefdef.rdflags & RDF_CAMERAEFFECT))
		R_DrawCameraEffect ();
	vid.width = oldWidth;
	vid.height = oldHeight;
}


void	Con_DrawString (int32_t x, int32_t y, char *string, int32_t alpha);
float	SCR_ScaledVideo (float param);
#define	FONT_SIZE		SCR_ScaledVideo(con_font_size->value)

void R_DrawSpeeds(fbo_t *destination)
{
	if (glState.currentFBO != destination)
		R_BindFBO(destination);
	// Knightmare- draw r_speeds (modified from Echon's tutorial)
	if (r_speeds->value && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) // don't do this for options menu
	{
		char	S[128];
		int32_t		lines, i, x, y, n = 0;

		lines = 5;

		for (i=0; i<lines; i++)
		{
			switch (i) {
				case 0:	n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i wcall", c_brush_calls); break;
				case 1:	n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i wsurf", c_brush_surfs); break;
				case 2:	n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i wpoly", c_brush_polys); break;
				case 3: n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i epoly", c_alias_polys); break;
				case 4: n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i ppoly", c_part_polys); break;
				case 5: n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i tex  ", c_visible_textures); break;
				case 6: n = sprintf (S, S_COLOR_ALT S_COLOR_SHADOW"%5i lmaps", c_visible_lightmaps); break;
				default: break;
			}
			if (scr_netgraph_pos->value)
				x = destination->width - (n*FONT_SIZE + FONT_SIZE/2);
			else
				x = FONT_SIZE/2;
			y = destination->height-(lines-i)*(FONT_SIZE+2);
			Con_DrawString (x, y, S, 255);
		}
	}
}

/*
================
R_SetGL2D
================
*/

void R_SetGL2D (void)
{
    // set 2D virtual screen size
	glViewport (0,0, vid.width, vid.height);
	GL_SetIdentityOrtho(GL_PROJECTION, 0, vid.width, vid.height, 0, -99999, 99999);
	GL_LoadIdentity(GL_MODELVIEW);

	GL_Disable (GL_DEPTH_TEST);
	GL_Disable (GL_CULL_FACE);
	GL_Disable (GL_BLEND);
	GL_Enable (GL_ALPHA_TEST);
    GL_Disable(GL_FOG);
    glColor4f (1,1,1,1);
    
}

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight, false);//true);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void V_RenderViewIntoFBO (fbo_t *fbo);
void R_RenderFrame (refdef_t *fd)
{
	eye_param_t params;
	vrect_t rect;
	memset(&params,0,sizeof(eye_param_t));
	params.projection.y.scale = 1.0f / tanf((fd->fov_y / 2.0f) * M_PI / 180);
	params.projection.y.offset = 0.0;
	params.projection.x.scale = 1.0f / tanf((fd->fov_x / 2.0f) * M_PI / 180);
	params.projection.x.offset = 0.0;

//	VectorSet(params.viewOffset,0,0,0);

	rect.x = fd->x;
	rect.y = fd->y;
	rect.width = fd->width;
	rect.height = fd->height;
	R_RenderViewIntoFBO(fd,params,glState.currentFBO,&rect);
//	V_RenderViewIntoFBO(hud);
	R_SetGL2D ();
}


void R_Register (void)
{
	// added Psychospaz's console font size option
	con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);
	con_font_size = Cvar_Get ("con_font_size", "8", CVAR_ARCHIVE);
	alt_text_color = Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);
	scr_netgraph_pos = Cvar_Get ("netgraph_pos", "0", CVAR_ARCHIVE);

	gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_NOSET );

	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_fullbright = Cvar_Get ("r_fullbright", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", CVAR_CHEAT);
	r_novis = Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_nocull = Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = Cvar_Get ("r_speeds", "0", 0);
	r_ignorehwgamma = Cvar_Get ("r_ignorehwgamma", "0", CVAR_ARCHIVE);	// hardware gamma
	vid_refresh = Cvar_Get ("vid_refresh", "0", CVAR_ARCHIVE); // refresh rate control
	Cvar_AssertRange(vid_refresh, 0, 150, true);

	vid_width = Cvar_Get( "vid_width", "1280", CVAR_ARCHIVE );
	vid_height = Cvar_Get( "vid_height", "800", CVAR_ARCHIVE );

	// lerped dlights on models
	r_dlights_normal = Cvar_Get("r_dlights_normal", "1", CVAR_ARCHIVE);
	r_model_shading = Cvar_Get( "r_model_shading", "3", CVAR_ARCHIVE );
	r_model_dlights = Cvar_Get( "r_model_dlights", "8", CVAR_ARCHIVE );

	r_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);
	// added Vic's RGB brightening
	r_rgbscale = Cvar_Get ("r_rgbscale", "1", CVAR_ARCHIVE);

	r_waterwave = Cvar_Get ("r_waterwave", "0", CVAR_ARCHIVE );
	r_waterquality = Cvar_Get ("r_waterquality","2",CVAR_ARCHIVE );
	r_glows = Cvar_Get ("r_glows", "1", CVAR_ARCHIVE );
	r_saveshotsize = Cvar_Get ("r_saveshotsize", "1", CVAR_ARCHIVE );

	// correct trasparent sorting
	r_transrendersort = Cvar_Get ("r_transrendersort", "1", CVAR_ARCHIVE );
	r_particle_lighting = Cvar_Get ("r_particle_lighting", "1.0", CVAR_ARCHIVE );
	r_particledistance = Cvar_Get ("r_particledistance", "0", CVAR_ARCHIVE );
	r_particle_overdraw = Cvar_Get ("r_particle_overdraw", "1", CVAR_ARCHIVE );
	r_particle_min = Cvar_Get ("r_particle_min", "0", CVAR_ARCHIVE );
	r_particle_max = Cvar_Get ("r_particle_max", "0", CVAR_ARCHIVE );

	r_modulate = Cvar_Get ("r_modulate", "1", CVAR_ARCHIVE );
	r_bitdepth = Cvar_Get( "r_bitdepth", "0", 0 );
	r_lightmap = Cvar_Get ("r_lightmap", "0", 0);
	r_shadows = Cvar_Get ("r_shadows", "0", CVAR_ARCHIVE );
	r_shadowalpha = Cvar_Get ("r_shadowalpha", "0.4", CVAR_ARCHIVE );
	r_shadowrange  = Cvar_Get ("r_shadowrange", "768", CVAR_ARCHIVE );
	r_shadowvolumes = Cvar_Get ("r_shadowvolumes", "0", CVAR_CHEAT );
	r_stencil = Cvar_Get ("r_stencil", "1", CVAR_ARCHIVE );

	r_dynamic = Cvar_Get ("r_dynamic", "1", 0);
	r_nobind = Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_picmip = Cvar_Get ("r_picmip", "0", 0);
	r_skymip = Cvar_Get ("r_skymip", "0", 0);
	r_showtris = Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showbbox = Cvar_Get ("r_showbbox", "0", CVAR_CHEAT); // show model bounding box
	r_finish = Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_cull = Cvar_Get ("r_cull", "1", 0);
	r_polyblend = Cvar_Get ("r_polyblend", "1", 0);
	r_flashblend = Cvar_Get ("r_flashblend", "0", 0);
	r_playermip = Cvar_Get ("r_playermip", "0", 0);
	// changed default texture mode to bilinear
	r_texturemode = Cvar_Get( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );

	r_lodbias = Cvar_Get("r_lodbias","0",CVAR_ARCHIVE);
	r_anisotropic = Cvar_Get( "r_anisotropic", "0", CVAR_ARCHIVE );
	r_anisotropic_avail = Cvar_Get( "r_anisotropic_avail", "0", 0 );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", 0 );
	r_texturecompression = Cvar_Get("r_texturecompression","2", CVAR_ARCHIVE);

	//gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "0", CVAR_ARCHIVE );
	//gl_ext_pointparameters = Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	r_nonpoweroftwo_mipmaps = Cvar_Get("r_nonpoweroftwo_mipmaps", "0", CVAR_ARCHIVE /*| CVAR_LATCH*/);

	// allow disabling of lightmaps on trans surfaces
	r_trans_lighting = Cvar_Get( "r_trans_lighting", "1", CVAR_ARCHIVE );

	// allow disabling of lighting on warp surfaces
	r_warp_lighting = Cvar_Get( "r_warp_lighting", "1", CVAR_ARCHIVE );

	// allow disabling of trans33+trans66 surface flag combining
	r_solidalpha = Cvar_Get( "r_solidalpha", "1", CVAR_ARCHIVE );
	
	// allow disabling of backwards alias model roll
	r_entity_fliproll = Cvar_Get( "r_entity_fliproll", "1", CVAR_ARCHIVE );	

	// added Psychospaz's envmapping
	r_glass_envmaps = Cvar_Get( "r_glass_envmaps", "1", CVAR_ARCHIVE );
	r_trans_surf_sorting = Cvar_Get( "r_trans_surf_sorting", "0", CVAR_ARCHIVE );
	r_shelltype = Cvar_Get( "r_shelltype", "1", CVAR_ARCHIVE );

	r_swapinterval = Cvar_Get( "r_swapinterval", "1", CVAR_ARCHIVE );
	r_adaptivevsync = Cvar_Get( "r_adaptivevsync", "1", CVAR_ARCHIVE );

	r_skydistance = Cvar_Get("r_skydistance", "10000", CVAR_ARCHIVE); // variable sky range
	r_saturation = Cvar_Get( "r_saturation", "1.0", CVAR_ARCHIVE );	//** DMP saturation setting (.89 good for nvidia)
	r_lightcutoff = Cvar_Get( "r_lightcutoff", "0", CVAR_ARCHIVE );	//** DMP dynamic light cutoffnow variable

	r_drawnullmodel = Cvar_Get("r_drawnullmodel","0", CVAR_ARCHIVE );
	r_fencesync = Cvar_Get("r_fencesync","0",CVAR_ARCHIVE );
	r_lateframe_decay = Cvar_Get("r_lateframe_decay","10",CVAR_ARCHIVE);
	r_lateframe_threshold = Cvar_Get("r_lateframe_threshold","20",CVAR_ARCHIVE);
	r_lateframe_ratio = Cvar_Get("r_lateframe_ratio","0.35",CVAR_ARCHIVE);
	r_directstate = Cvar_Get("r_directstate","1",CVAR_ARCHIVE);
	r_driver_workarounds = Cvar_Get("r_driver_workarounds","1",CVAR_ARCHIVE);

	Cmd_AddCommand ("imagelist", R_ImageList_f);
	Cmd_AddCommand ("screenshot", R_ScreenShot_f);
	Cmd_AddCommand ("screenshot_silent", R_ScreenShot_Silent_f);
	Cmd_AddCommand ("modellist", Mod_Modellist_f);
	Cmd_AddCommand ("gl_strings", GL_Strings_f);
//	Cmd_AddCommand ("resetvertexlights", R_ResetVertextLights_f);
}


/*
==================
R_SetMode
==================
*/
//extern void VR_GetHMDPos(int32_t *xpos, int32_t *ypos);
qboolean R_SetMode (void)
{
	rserr_t err;


	if ( vid_fullscreen->modified && !glConfig.allowCDS )
	{
		VID_Printf (PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n" );
		Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	r_skydistance->modified = true; // skybox size variable


	vid_fullscreen->modified = false;

	if (vid_height->value < 480 || vid_width->value < 640)
	{
		Cvar_SetToDefault("vid_width");
		Cvar_SetToDefault("vid_height");

		VID_Printf (PRINT_ALL, "R_SetMode() - Invalid resolution set, reverting to default resolution\n" );
	}

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height )) != rserr_ok )
	{
		if ( err == rserr_invalid_fullscreen )
		{
			Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			VID_Printf (PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			Cvar_SetValue("vid_width",640);
			Cvar_SetValue("vid_height",480);
			VID_Printf (PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode(  &vid.width, &vid.height ) ) != rserr_ok )
		{
			VID_Printf (PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}


/*
===============
R_Init
===============
*/
qboolean R_Init ( char *reason )
{	
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int32_t		err;
	int32_t		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	Draw_GetPalette ();
	R_Register();

	// place default error
	memcpy (reason, "Unknown failure on intialization!\0", 34);
	
    glConfig.allowCDS = true;
	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( ) )
	{
		memcpy (reason, "Init of OS-specific parts of OpenGL Failed!\0", 44);
		return false;
	}


	// create the window and set up the context
	if ( !R_SetMode () )
	{
        VID_Printf (PRINT_ALL, "R_Init() - could not R_SetMode()\n" );
		memcpy (reason, "Creation of the window/context set-up Failed!\0", 46);
		return false;
	}

	glConfig.render_height = vid.height;
	glConfig.render_width = vid.width;

	//
	// get our various GL strings
	//
	glConfig.vendor_string = glGetString (GL_VENDOR);
	glConfig.renderer_string = glGetString (GL_RENDERER);
	glConfig.version_string = glGetString (GL_VERSION);
	glConfig.shader_version_string = glGetString (GL_SHADING_LANGUAGE_VERSION);

	sscanf((const char *)glConfig.version_string, "%d.%d.%d", &glConfig.version_major, &glConfig.version_minor, &glConfig.version_release);
	sscanf((const char *)glConfig.shader_version_string, "%d.%d", &glConfig.shader_version_major, &glConfig.shader_version_minor);
	
	if (glConfig.version_major < 2)
	{
		memcpy (reason, "Could not get OpenGL 2.0 or higher context!\0", 44);
		return false;
	}


	VID_Printf (PRINT_ALL, "GL_VENDOR: %s\n", glConfig.vendor_string );
	VID_Printf (PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	VID_Printf (PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	VID_Printf (PRINT_ALL, "GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig.shader_version_string );

	// Knighmare- added max texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&glConfig.max_texsize);
	VID_Printf (PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.max_texsize );

	glConfig.extensions_string = glGetString (GL_EXTENSIONS);
//	VID_Printf (PRINT_DEVELOPER, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	if (developer->value > 0)	// print extensions 2 to a line
	{
		char		*extString, *extTok;
		uint32_t	line = 0;
		VID_Printf (PRINT_DEVELOPER, "GL_EXTENSIONS: " );
		extString = (char *)glConfig.extensions_string;
		while (1)
		{
			extTok = COM_Parse(&extString);
			if (!extTok[0])
				break;
			line++;
			if ((line % 2) == 0)
				VID_Printf (PRINT_DEVELOPER, "%s\n", extTok );
			else
				VID_Printf (PRINT_DEVELOPER, "%s ", extTok );
		}
		if ((line % 2) != 0)
			VID_Printf (PRINT_DEVELOPER, "\n" );
	}

	Q_strlcpy( renderer_buffer, (char *) glConfig.renderer_string, sizeof(renderer_buffer));
	Q_strlwr( renderer_buffer );

	Q_strlcpy( vendor_buffer, (char *) glConfig.vendor_string, sizeof(vendor_buffer) );
	Q_strlwr( vendor_buffer );

	// find out the renderer model
	if (strstr(vendor_buffer, "nvidia")) {
		glConfig.rendType = GLREND_NVIDIA;
		if (strstr(renderer_buffer, "geforce"))	glConfig.rendType |= GLREND_GEFORCE;
	}
	else if (strstr(vendor_buffer, "ati") || strstr(vendor_buffer, "amd")) {
		glConfig.rendType = GLREND_ATI;
		if (strstr(vendor_buffer, "radeon"))		glConfig.rendType |= GLREND_RADEON;
	}
	else if (strstr(vendor_buffer, "intel"))		glConfig.rendType = GLREND_INTEL;
	else											glConfig.rendType = GLREND_DEFAULT;


	
	Cvar_Set( "scr_drawall", "0" );

#ifdef __linux__
	Cvar_SetValue( "r_finish", 1 );
#else
	Cvar_SetValue( "r_finish", 0 );
#endif
	r_swapinterval->modified = true;	// force swapinterval update

	if (glConfig.allowCDS)
		VID_Printf (PRINT_ALL, "...allowing CDS\n" );
	else
		VID_Printf (PRINT_ALL, "...disabling CDS\n" );

	//
	// grab extensions
	//

	// GL_EXT_compiled_vertex_array
	glConfig.extCompiledVertArray = false;
	if ( GLEW_EXT_compiled_vertex_array )
	{
		VID_Printf (PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
		glConfig.extCompiledVertArray = true;
	}
	else
		VID_Printf (PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );

	// GL_ARB_vertex_buffer_object
	glConfig.vertexBufferObject = true;

	glConfig.ext_packed_depth_stencil = false;
	if ( GLEW_EXT_packed_depth_stencil )
	{
		VID_Printf (PRINT_ALL, "...using GL_EXT_packed_depth_stencil\n");
		glConfig.ext_packed_depth_stencil = true;

	} else {
		VID_Printf (PRINT_ALL, "...GL_EXT_packed_depth_stencil not found\n");
	}

	glConfig.ext_framebuffer_object = false;
	if ( glConfig.ext_packed_depth_stencil )
	{
		if (GLEW_EXT_framebuffer_object)
		{
			VID_Printf (PRINT_ALL, "...using GL_EXT_framebuffer_object\n");
			glConfig.ext_framebuffer_object = true;
			glState.currentFBO = 0;
		} else {
			VID_Printf (PRINT_ALL, "...GL_EXT_framebuffer_object not found\n");
		}
	}

	// GL_EXT_texture_filter_anisotropic - NeVo
	glConfig.anisotropic = false;
	if ( GLEW_EXT_texture_filter_anisotropic)
	{
		VID_Printf (PRINT_ALL,"...using GL_EXT_texture_filter_anisotropic\n" );
		glConfig.anisotropic = true;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_anisotropy);
		Cvar_SetValue("r_anisotropic_avail", glConfig.max_anisotropy);
	}
	else
	{
		VID_Printf (PRINT_ALL,"..GL_EXT_texture_filter_anisotropic not found\n" );
		glConfig.anisotropic = false;
		glConfig.max_anisotropy = 0.0;
		Cvar_SetValue("r_anisotropic_avail", 0.0);
	} 

	glConfig.arb_sync = false;
	if (GLEW_ARB_sync)
	{
		// This actually works on Intel GPUs
		// We are ignoring it because it hurts performance
		// and Intel GPUs need all the help they can get
		if ( !((glConfig.rendType & GLREND_INTEL) && r_driver_workarounds->value) )
		{
			VID_Printf (PRINT_ALL, "...using GL_ARB_sync\n" );
			glConfig.arb_sync = true;
		} else {
			VID_Printf (PRINT_ALL, "...ignoring GL_ARB_sync on Intel GPUs\n" );
			glConfig.arb_sync = false;
			Cvar_SetInteger("r_fencesync",0);
		}
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_sync not found\n" );
		glConfig.arb_sync = false;
		Cvar_SetInteger("r_fencesync",0);
	}

	if (GLEW_ARB_texture_float)
	{
		VID_Printf (PRINT_ALL, "...using GL_ARB_texture_float\n" );
		glConfig.arb_texture_float = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_texture_float not found\n" );
		glConfig.arb_texture_float = false;
	}

	if (GLEW_ARB_texture_rg)
	{
		VID_Printf (PRINT_ALL, "...using GL_ARB_texture_rg\n" );
		glConfig.arb_texture_rg = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_texture_rg not found\n" );
		glConfig.arb_texture_rg = false;
	}

	glConfig.ext_direct_state_access = false;
	if (GLEW_EXT_direct_state_access)
	{
		VID_Printf (PRINT_ALL, "...using GL_EXT_direct_state_access\n" );
		glConfig.ext_direct_state_access = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_EXT_direct_state_access not found\n" );
		glConfig.ext_direct_state_access = false;
		Cvar_SetInteger("r_directstate",0);
	}

	glConfig.ext_texture_srgb = false;
	if (GLEW_EXT_texture_sRGB)
	{
		VID_Printf (PRINT_ALL, "...using GL_EXT_texture_srgb\n" );
		glConfig.ext_texture_srgb = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_EXT_texture_srgb not found\n" );
		glConfig.ext_texture_srgb = false;
//		Cvar_SetInteger("r_srgb_gamma",0);
	}

	glConfig.ext_framebuffer_srgb = false;
	if (GLEW_EXT_framebuffer_sRGB)
	{
		VID_Printf (PRINT_ALL, "...using GL_EXT_framebuffer_srgb\n" );
		glConfig.ext_framebuffer_srgb = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_EXT_framebuffer_srgb not found\n" );
		glConfig.ext_framebuffer_srgb = false;
	}

	glConfig.arb_vertex_buffer_object = false;
	if (GLEW_ARB_vertex_buffer_object)
	{
		VID_Printf (PRINT_ALL, "...using GL_ARB_vertex_buffer_object\n" );
		glConfig.arb_vertex_buffer_object = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_vertex_buffer_object not found\n" );
		glConfig.arb_vertex_buffer_object = false;
	}

	glConfig.arb_vertex_array_object = false;
	if (GLEW_ARB_vertex_array_object)
	{
		VID_Printf (PRINT_ALL, "...using GL_ARB_vertex_array_object\n" );
		glConfig.arb_vertex_array_object = true;
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_vertex_array_object not found\n" );
		glConfig.arb_vertex_array_object = false;
	}

	glConfig.ext_texture_compression_s3tc = false;
	if (GLEW_EXT_texture_compression_s3tc)
	{
		// ignore GL_EXT_texture_compression_s3tc on Intel.
		// Why? Because it causes a tremendous hit in performance
		// 
		if ( !((glConfig.rendType & GLREND_INTEL) && r_driver_workarounds->value) )
		{
			VID_Printf (PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
			glConfig.ext_texture_compression_s3tc = true;
		} else {
			VID_Printf (PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc on Intel GPUs\n" );
			glConfig.ext_texture_compression_s3tc = false;
		}
	} else {
		VID_Printf (PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
		glConfig.ext_texture_compression_s3tc = false;
	}

	glConfig.arb_texture_compression_bptc = false;
	if (GLEW_ARB_texture_compression_bptc)
	{
		// ignore GL_ARB_texture_compression_bptc on Intel.
		// Does it suffer the same performance loss as s3tc? 
		// 
		if ( !((glConfig.rendType & GLREND_INTEL) && r_driver_workarounds->value) )
		{
			VID_Printf (PRINT_ALL, "...using GL_ARB_texture_compression_bptc\n" );
			glConfig.arb_texture_compression_bptc = true;
		} else {
			VID_Printf (PRINT_ALL, "...ignoring GL_ARB_texture_compression_bptc on Intel GPUs\n" );
			glConfig.arb_texture_compression_bptc = false;
		}
	} else {
		VID_Printf (PRINT_ALL, "...GL_ARB_texture_compression_bptc not found\n" );
		glConfig.arb_texture_compression_bptc = false;
	}

    
    
#ifdef _WIN32
    // WGL_EXT_swap_control / WGL_EXT_swap_control_tear
    glConfig.ext_swap_control = WGLEW_EXT_swap_control;
    glConfig.ext_swap_control_tear = WGLEW_EXT_swap_control_tear;
    
#elif __linux__
    // GLX_EXT_swap_control / GLX_EXT_swap_control_tear
    glConfig.ext_swap_control = GLXEW_EXT_swap_control;
    glConfig.ext_swap_control_tear = GLXEW_EXT_swap_control_tear;
    
#else
    glConfig.ext_swap_control = false;
    glConfig.ext_swap_control_tear = false;
#endif
    
    if ( glConfig.ext_swap_control )
    {
        VID_Printf (PRINT_ALL, "...using EXT_swap_control\n" );
    }
    else
    {
        VID_Printf (PRINT_ALL, "...EXT_swap_control not found\n" );
    }
    
    if ( glConfig.ext_swap_control_tear )
    {
#ifdef _WIN32
        // AMD likes to fuck up this extension
        // either by periodocially running faster than vsync to buffer frames
        // or simply by corrupting the image on the screen
        // this is why we cannot have nice things
        if ( !((glConfig.rendType & GLREND_ATI) && r_driver_workarounds->value) )
        {
#endif
            
            VID_Printf (PRINT_ALL, "...using EXT_swap_control_tear\n" );
#ifdef _WIN32
        } else
        {
            glConfig.ext_swap_control_tear = false;
            VID_Printf (PRINT_ALL, "...ignoring EXT_swap_control_tear on AMD GPUs\n" );
            Cvar_SetInteger("r_adaptivevsync",0);
        }
#endif
    }
    else
    {
        VID_Printf (PRINT_ALL, "...EXT_swap_control_tear not found\n" );
    }
    
	glGetIntegerv(GL_MAX_TEXTURE_UNITS , &glConfig.max_texunits);
	VID_Printf (PRINT_ALL, "...GL_MAX_TEXTURE_UNITS: %i\n", glConfig.max_texunits);
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_Init: glGetError() = 0x%x\n", err);


/*
	Com_Printf( "Size of dlights: %i\n", sizeof (dlight_t)*MAX_DLIGHTS );
	Com_Printf( "Size of entities: %i\n", sizeof (entity_t)*MAX_ENTITIES );
	Com_Printf( "Size of particles: %i\n", sizeof (particle_t)*MAX_PARTICLES );
	Com_Printf( "Size of decals: %i\n", sizeof (particle_t)*MAX_DECAL_FRAGS );
*/
	GL_SetDefaultState();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "GL_SetDefaultState: glGetError() = 0x%x\n", err);

	R_ShaderObjectsInit();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_ShaderObjectsInit: glGetError() = 0x%x\n", err);

	R_PostProcessInit();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_BlurInit: glGetError() = 0x%x\n", err);

	R_AntialiasInit();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_AntialiasInit: glGetError() = 0x%x\n", err);

	R_VR_Init();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_VR_Init: glGetError() = 0x%x\n", err);

	R_StereoInit();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_StereoInit: glGetError() = 0x%x\n", err);

	R_InitImages ();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_InitImages: glGetError() = 0x%x\n", err);

	Mod_Init ();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "Mod_Init: glGetError() = 0x%x\n", err);

	R_InitMedia ();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_InitMedia: glGetError() = 0x%x\n", err);

	R_DrawInitLocal ();
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_DrawInitLocal: glGetError() = 0x%x\n", err);

	R_InitDSTTex (); // init shader warp texture
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_InitDSTTex: glGetError() = 0x%x\n", err);

	R_InitFogVars (); // reset fog variables
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_InitFogVars: glGetError() = 0x%x\n", err);

	VLight_Init (); // Vic's bmodel lights
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "VLight_Init: glGetError() = 0x%x\n", err);

	RB_InitBackend(); // init mini-backend
	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "RB_InitBackend: glGetError() = 0x%x\n", err);

	return true;
}


/*
===============
R_ClearState
===============
*/
void R_ClearState (void)
{	
	R_SetFogVars (false, 0, 0, 0, 0, 0, 0, 0); // clear fog effets
	GL_EnableMultitexture (false);
	GL_SetDefaultState ();
}


/*
=================
GL_Strings_f
=================
*/
void GL_Strings_f (void)
{
	char		*extString, *extTok;
	uint32_t	line = 0;

	VID_Printf (PRINT_ALL, "GL_VENDOR: %s\n", glConfig.vendor_string );
	VID_Printf (PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	VID_Printf (PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	VID_Printf (PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.max_texsize );
//	VID_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	// print extensions 2 to a line
	VID_Printf (PRINT_ALL, "GL_EXTENSIONS: " );
	extString = (char *)glConfig.extensions_string;
	while (1)
	{
		extTok = COM_Parse(&extString);
		if (!extTok[0])
			break;
		line++;
		if ((line % 2) == 0)
			VID_Printf (PRINT_ALL, "%s\n", extTok );
		else
			VID_Printf (PRINT_ALL, "%s ", extTok );
	}
	if ((line % 2) != 0)
		VID_Printf (PRINT_ALL, "\n" );
}


/*
===============
R_Shutdown
===============
*/

fbo_t viewFBO;

void R_Shutdown (void)
{	
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("screenshot_silent");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("gl_strings");
//	Cmd_RemoveCommand ("resetvertexlights");

	// Knightmare- Free saveshot buffer
	if (saveshotdata)
		Z_Free(saveshotdata);
	saveshotdata = NULL;	// make sure this is null after a vid restart!

	Mod_FreeAll ();
	R_PostProcessShutdown();
	R_AntialiasShutdown();
	R_ShaderObjectsShutdown();
	R_ShutdownImages ();
	R_StereoShutdown();
	R_VR_Shutdown();
	R_DelFBO(&viewFBO);

	//
	// shut down OS specific OpenGL stuff like contexts, etc.
	//
	GLimp_Shutdown();

	//
	// shutdown our QGL subsystem
	//
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_GetViewFBO
@@@@@@@@@@@@@@@@@@@@@
*/

fbo_t* R_GetViewFBO(void)
{
	return &viewFBO;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void RefreshFont (void);
void GLimp_SetFullscreen(qboolean enable);

void R_BeginFrame()
{

	// Knightmare- added Psychospaz's console font size option
	if (con_font->modified)
		RefreshFont ();

	if (con_font_size->modified)
	{
		if (con_font_size->value < 4)
			Cvar_Set( "con_font_size", "4" );
		else if (con_font_size->value > 24)
			Cvar_Set( "con_font_size", "24" );

		con_font_size->modified = false;
	}
	// end Knightmare

	//
	// change modes if necessary
	//
	if ( vid_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		if (!vr_enabled->value || !vr_force_fullscreen->value)
			GLimp_SetFullscreen((qboolean) vid_fullscreen->value);
		vid_fullscreen->modified=false;
	}

	if (vid_srgb->modified)
	{
		vid_srgb->modified = false;
		vid_brightness->modified = true;
	}

#define BRIGHTNESS_MAX 1.0
#define BRIGHTNESS_MIN 0.0
#define BRIGHTNESS_DIFF (BRIGHTNESS_MAX - BRIGHTNESS_MIN)

	if (vid_brightness->modified)
	{
		float bright = 0.0;
		
		if (vid_brightness->value > BRIGHTNESS_MAX)
			Cvar_SetValue("vid_brightness", BRIGHTNESS_MAX);
		else if (vid_brightness->value < BRIGHTNESS_MIN)
			Cvar_SetValue("vid_brightness", BRIGHTNESS_MIN);
		vid_brightness->modified = false;
		// normalize from 0 to 1
		bright = (vid_brightness->value - BRIGHTNESS_MIN) / BRIGHTNESS_DIFF;
		
		if (glConfig.srgb_framebuffer && vid_srgb->value)
		{
			// compress gamma from 0.5 - 1.2
			vid_gamma = bright * 0.6  + 0.5;
		} else {
			// expand gamma from 1.0 to 2.5
			vid_gamma = bright * 1.3  + 1.0;
		}

	}

	GLimp_BeginFrame(  );
	R_BindFBO(&screenFBO);

	R_AntialiasStartFrame();


	if (vr_enabled->value)
	{
		R_VR_StartFrame();
	} else {
		R_AntialiasSetFBOSize(&viewFBO);
		R_ClearFBO(&viewFBO);
		R_StereoFrame(&viewFBO);

	}
	
	//
	// texturemode stuff
	//
	if ( r_texturemode->modified )
	{
		GL_TextureMode( r_texturemode->string );
		r_texturemode->modified = false;
	}


	if ( r_lodbias->modified )
	{
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS,r_lodbias->value);
		r_lodbias->modified = false;
	}

	//
	// swapinterval stuff
	//
	GL_UpdateSwapInterval();

	// clear screen if desired
	//

	
	if (glConfig.have_stencil)
	{
		if (r_shadows->value == 3) // BeefQuake R6 shadows
			glClearStencil(0);
		else
			glClearStencil(1);
	}
}

void R_EndFrame(void)
{
	int32_t		err;
	fbo_t *frame = &viewFBO;
	qboolean srgb = (qboolean) (glConfig.srgb_framebuffer && vid_srgb->value);
	qboolean vr = (qboolean) (vr_enabled->value != 0);
	err = glGetError();
	//	assert( err == GL_NO_ERROR );

	if (err != GL_NO_ERROR)	// Output error code instead
		VID_Printf (PRINT_DEVELOPER, "OpenGL Error %i\n", err);

	GL_SetIdentity(GL_PROJECTION);
	GL_SetIdentity(GL_MODELVIEW);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_ALPHA_TEST);
	R_VR_EndFrame();
	R_Stereo_EndFrame(frame);

	R_BindFBO(&screenFBO);
	R_Clear();
	glColor4f(1.0,1.0,1.0,1.0);

	if (vr)
	{
		frame = R_VR_GetFrameFBO();
	} 

	if (srgb)
	{
		glEnable(GL_FRAMEBUFFER_SRGB);
	}	

	R_BlitWithGamma(frame->texture,vid_gamma);

	if (srgb)
	{
		glDisable(GL_FRAMEBUFFER_SRGB);
	}	
	if (vr)
		R_VR_PostGammaPresent();

	lastFrame = frame;

	GLimp_EndFrame();
	
	R_FrameFence();
}

/*
=============
R_SetPalette
=============
*/
uint32_t r_rawpalette[256];

void R_SetPalette ( const uint8_t *palette)
{
	int32_t		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	//GL_SetTexturePalette( r_rawpalette );

	GL_ClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT);
	GL_SetDefaultClearColor();
}

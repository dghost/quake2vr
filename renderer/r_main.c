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

#include "r_local.h"
#include "vlights.h"
#include "../vr/vr.h"

void R_Clear (void);

viddef_t	vid;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t	glConfig;
glstate_t	glState;
glmedia_t	glMedia;

entity_t	*currententity;
int			r_worldframe; // added for trans animations
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_calls, c_brush_surfs, c_brush_polys, c_alias_polys, c_part_polys;

float		v_blend[4];			// final blending color

int			maxsize;			// Nexus

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

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*gl_allow_software;
cvar_t  *gl_driver;
cvar_t	*gl_clear;

cvar_t	*con_font; // Psychospaz's console font size option
cvar_t	*con_font_size;
cvar_t	*alt_text_color;
cvar_t	*scr_netgraph_pos;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_ignorehwgamma; // hardware gamma
cvar_t	*r_displayrefresh; // refresh rate control
cvar_t	*r_lefthand;
cvar_t	*r_waterwave;	// water waves
cvar_t  *r_caustics;	// Barnes water caustics
cvar_t  *r_glows;		// texture glows
cvar_t	*r_saveshotsize;//  save shot size option

cvar_t	*r_width;
cvar_t	*r_height;
cvar_t	*r_dlights_normal; // lerped dlights on models
cvar_t	*r_model_shading;
cvar_t	*r_model_dlights;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*r_rgbscale; // Vic's RGB brightening

cvar_t	*r_nosubimage;
cvar_t	*r_vertex_arrays;

cvar_t	*r_ext_swapinterval;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_nonpoweroftwo_mipmaps;		// Knightmare- non-power-of-two texture support
cvar_t	*r_newlightmapformat;			// Knightmare- whether to use new lightmap format

cvar_t	*r_arb_fragment_program;
cvar_t	*r_arb_vertex_program;
cvar_t	*r_arb_vertex_buffer_object;
cvar_t	*r_pixel_shader_warp; // allow disabling the nVidia water warp
cvar_t	*r_trans_lighting; // disabling of lightmaps on trans surfaces
cvar_t	*r_warp_lighting; // allow disabling of lighting on warp surfaces
cvar_t	*r_solidalpha;			// allow disabling of trans33+trans66 surface flag combining
cvar_t	*r_entity_fliproll;		// allow disabling of backwards alias model roll

cvar_t	*r_glass_envmaps; // Psychospaz's envmapping
cvar_t	*r_trans_surf_sorting; // trans bmodel sorting
cvar_t	*r_shelltype; // entity shells: 0 = solid, 1 = warp, 2 = spheremap
cvar_t	*r_ext_texture_compression; // Heffo - ARB Texture Compression
cvar_t	*r_screenshot_jpeg;			// Heffo - JPEG Screenshots
cvar_t	*r_screenshot_jpeg_quality;	// Heffo - JPEG Screenshots

//cvar_t	*r_motionblur;				// motionblur
cvar_t	*r_lightcutoff;	//** DMP - allow dynamic light cutoff to be user-settable

cvar_t	*r_bitdepth;
cvar_t	*r_drawbuffer;
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
cvar_t  *r_monolightmap;

cvar_t	*r_modulate;
cvar_t	*r_nobind;
cvar_t	*r_round_down;
cvar_t	*r_picmip;
cvar_t	*r_skymip;
cvar_t	*r_playermip;
cvar_t	*r_showtris;
cvar_t	*r_showbbox;	// show model bounding box
cvar_t	*r_ztrick;
cvar_t	*r_finish;
cvar_t	*r_cull;
cvar_t	*r_polyblend;
cvar_t	*r_flashblend;
cvar_t  *r_saturatelighting;
cvar_t	*r_swapinterval;
cvar_t  *r_adaptivevsync;
cvar_t	*r_texturemode;
cvar_t	*r_texturealphamode;
cvar_t	*r_texturesolidmode;
cvar_t	*r_anisotropic;
cvar_t	*r_anisotropic_avail;
cvar_t	*r_lockpvs;

cvar_t	*r_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

cvar_t  *r_bloom;	// BLOOMS

cvar_t	*r_skydistance; //Knightmare- variable sky range
cvar_t	*r_saturation;	//** DMP

cvar_t  *r_drawnullmodel;
cvar_t  *r_fencesync;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

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
	if (!r_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	GL_Disable (GL_ALPHA_TEST);
	GL_Enable (GL_BLEND);
	GL_Disable (GL_DEPTH_TEST);
	GL_DisableTexture(0);

    glLoadIdentity ();

	// FIXME: get rid of these
    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	rb_vertex = rb_index = 0;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+1;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+3;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, 100);
	VA_SetElem4(colorArray[rb_vertex], v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, 100);
	VA_SetElem4(colorArray[rb_vertex], v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, -100);
	VA_SetElem4(colorArray[rb_vertex], v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, -100);
	VA_SetElem4(colorArray[rb_vertex], v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
	rb_vertex++;
	RB_RenderMeshGeneric (false);

	GL_Disable (GL_BLEND);
	GL_EnableTexture(0);
	//GL_Enable (GL_ALPHA_TEST);

	glColor4f (1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

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
	int		i;
	float	fov_x = r_newrefdef.fov_x;
	float	fov_y = r_newrefdef.fov_y;

	// hack to keep objects from disappearing at the edges
	if (vr_enabled->value)
	{
		fov_y += 20;
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
	int i;
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
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
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

	// clear out the portion of the screen that the NOWORLDMODEL defines
	/*if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		GL_Enable( GL_SCISSOR_TEST );
		glClearColor( 0.3, 0.3, 0.3, 1 );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( 1, 0, 0.5, 0.5 );
		GL_Disable( GL_SCISSOR_TEST );
	}*/
}

void vrPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar, GLdouble offset)
{

	GLdouble f = 1.0f / tanf((fovy / 2.0f) * M_PI / 180);
	GLdouble nf = 1.0f / (zNear - zFar);
	GLdouble out[16];

	out[0] = f / aspect;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;
	out[4] = 0;
	out[5] = f;
	out[6] = 0;
	out[7] = 0;
	out[8] = offset;
	out[9] = 0;
	out[10] = (zFar + zNear) * nf;
	out[11] = -1;
	out[12] = 0;
	out[13] = 0;
	out[14] = (2.0f * zFar * zNear) * nf;
	out[15] = 0;

	glMultMatrixd(out);
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL(void)
{
	float	screenaspect;
	float	offset;
	//	float	yfov;
	int		x, x2, y2, y, w, h;
	vec3_t vieworigin;
	//Knightmare- variable sky range
	static GLdouble farz;
	GLdouble boxsize;
	//end Knightmare

	// Knightmare- update r_modulate in real time
	if (r_modulate->modified && (r_worldmodel)) //Don't do this if no map is loaded
	{
		msurface_t *surf;
		int i;

		for (i = 0, surf = r_worldmodel->surfaces; i < r_worldmodel->numsurfaces; i++, surf++)
			surf->cached_light[0] = 0;

		r_modulate->modified = 0;
	}

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	glViewport(x, y2, w, h);

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
	//	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	screenaspect = (float) r_newrefdef.width / r_newrefdef.height;

	if (vr_enabled->value)
	{
		offset = vrState.eye * vrState.projOffset;
		screenaspect = vrConfig.aspect;
	}
	else {
		offset = 0;
		screenaspect = (float) r_newrefdef.width / r_newrefdef.height;
	}

	//Knightmare- 12/26/2001- increase back clipping plane distance
	vrPerspective(r_newrefdef.fov_y, screenaspect, 1, farz, offset); //was 4096
	//end Knightmare

	GL_CullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
	glRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
	glRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
	

	VectorCopy(r_newrefdef.vieworg,vieworigin);
	/*
	// Neckmodel stuff
	if (vr_enabled->value && vr_neckmodel->value)
	{
		vec3_t forward, up, out;
		float eyeDist = vr_neckmodel_forward->value * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
		float neckLength = vr_neckmodel_up->value * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
		AngleVectors(r_newrefdef.viewangles,forward,NULL,up);
		VectorNormalize(forward);
		VectorNormalize(up);
		VectorScale(forward, eyeDist ,forward);
		VectorScale(up,neckLength,up);
		VectorAdd(forward,up,out);
		vieworigin[2] -= neckLength;
		VectorAdd(vieworigin,out,vieworigin); 
	}
	*/
    glTranslatef (-vieworigin[0],  -vieworigin[1],  -vieworigin[2]);

//	if ( glState.camera_separation != 0 && glState.stereo_enabled )
//		glTranslatef ( glState.camera_separation, 0, 0 );

	
	//glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

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
	GLbitfield	clearBits = 0;	// bitshifter's consolidation

	if (gl_clear->value)
		clearBits |= GL_COLOR_BUFFER_BIT;

	if (r_ztrick->value)
	{
		static int trickframe;

	//	if (gl_clear->value)
	//		glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			GL_DepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			GL_DepthFunc (GL_GEQUAL);
		}
	}
	else
	{
	//	if (gl_clear->value)
	//		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//	else
	//		glClear (GL_DEPTH_BUFFER_BIT);
		clearBits |= GL_DEPTH_BUFFER_BIT;

		gldepthmin = 0;
		gldepthmax = 1;
		GL_DepthFunc (GL_LEQUAL);
	}

	GL_DepthRange (gldepthmin, gldepthmax);

	// added stencil buffer
	if (glConfig.have_stencil)
	{
		if (r_shadows->value == 3) // BeefQuake R6 shadows
			glClearStencil(0);
		else
			glClearStencil(1);
	//	glClear(GL_STENCIL_BUFFER_BIT);
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

void VR_DrawCrosshair()
{

	if (!vr_enabled->value)
		return;

	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_ALPHA_TEST);
	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	GL_Bind(0);
	glColor4f(1.0,0.0,0.0,vr_crosshair_brightness->value / 100.0);
	if ((int) vr_crosshair->value == VR_CROSSHAIR_DOT)
	{
		glEnable(GL_POINT_SMOOTH);

		glPointSize(vr_crosshair_size->value * vrState.pixelScale);
		glBegin(GL_POINTS);
		glVertex3f(r_newrefdef.aimend[0],r_newrefdef.aimend[1],r_newrefdef.aimend[2]);
		glEnd();
		glDisable(GL_POINT_SMOOTH);
	} else if ((int) vr_crosshair->value == VR_CROSSHAIR_LASER)
	{

		glLineWidth( vr_crosshair_size->value * vrState.pixelScale );
		glBegin (GL_LINES);
		glVertex3f(r_newrefdef.aimstart[0],r_newrefdef.aimstart[1],r_newrefdef.aimstart[2]);	
		glVertex3f(r_newrefdef.aimend[0],r_newrefdef.aimend[1],r_newrefdef.aimend[2]);
		glEnd ();
	}

	GL_Enable(GL_DEPTH_TEST);
	GL_Disable(GL_BLEND);
}

/*
================
VR_RenderView

r_newrefdef must be set before the first call
================
*/
void VR_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		VID_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

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

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) // options menu
	{
		qboolean fog_on = false;
		//Knightmare- no fogging on menu/hud models
		if (glIsEnabled(GL_FOG)) //check if fog is enabled
		{
			fog_on = true;
			glDisable(GL_FOG); //if so, disable it
		}

		//R_DrawAllDecals();
		R_DrawAllEntities(false);
		R_DrawAllParticles();

		//re-enable fog if it was on
		if (fog_on)
			glEnable(GL_FOG);
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

		R_ParticleStencil (1);
		R_DrawAlphaSurfaces ();
		R_ParticleStencil (2);

		R_ParticleStencil (3);
		if (r_particle_overdraw->value) // redraw over alpha surfaces, those behind are occluded
			R_DrawAllParticles ();
		R_ParticleStencil (4);

		// always draw vwep last...
		R_DrawEntitiesOnList(ents_viewweaps_trans);
	}
	R_SetFog();
}

/*
================
VR_RenderScreenEffects

r_newrefdef must be set before the first call
================
*/
void VR_RenderScreenEffects (refdef_t *fd)
{
		r_newrefdef = *fd;
		R_BloomBlend (fd);	// BLOOMS
		R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		VID_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

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

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) // options menu
	{
		qboolean fog_on = false;
		//Knightmare- no fogging on menu/hud models
		if (glIsEnabled(GL_FOG)) //check if fog is enabled
		{
			fog_on = true;
			glDisable(GL_FOG); //if so, disable it
		}

		//R_DrawAllDecals();
		R_DrawAllEntities(false);
		R_DrawAllParticles();

		//re-enable fog if it was on
		if (fog_on)
			glEnable(GL_FOG);
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

		R_ParticleStencil (1);
		R_DrawAlphaSurfaces ();
		R_ParticleStencil (2);

		R_ParticleStencil (3);
		if (r_particle_overdraw->value) // redraw over alpha surfaces, those behind are occluded
			R_DrawAllParticles ();
		R_ParticleStencil (4);

		// always draw vwep last...
		R_DrawEntitiesOnList(ents_viewweaps_trans);
		
		R_BloomBlend (fd);	// BLOOMS
		R_Flash();
	}
	R_SetFog();
}


/*
================
R_SetGL2D
================
*/
void	Con_DrawString (int x, int y, char *string, int alpha);
float	SCR_ScaledVideo (float param);
#define	FONT_SIZE		SCR_ScaledVideo(con_font_size->value)

void R_SetGL2D (void)
{
	// set 2D virtual screen size
	glViewport (0,0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
	GL_Disable (GL_DEPTH_TEST);
	GL_Disable (GL_CULL_FACE);
	GL_Disable (GL_BLEND);
	GL_Enable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);

	// Knightmare- draw r_speeds (modified from Echon's tutorial)
	if (r_speeds->value && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) // don't do this for options menu
	{
		char	S[128];
		int		lines, i, x, y, n = 0;

		lines = (glConfig.multitexture)?5:7;

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
				x = r_newrefdef.width - (n*FONT_SIZE + FONT_SIZE/2);
			else
				x = FONT_SIZE/2;
			y = r_newrefdef.height-(lines-i)*(FONT_SIZE+2);
			Con_DrawString (x, y, S, 255);
		}
	}
}


#if 0
static void GL_DrawColoredStereoLinePair (float r, float g, float b, float y)
{
	glColor3f( r, g, b );
	glVertex2f( 0, y );
	glVertex2f( vid.width, y );
	glColor3f( 0, 0, 0 );
	glVertex2f( 0, y + 1 );
	glVertex2f( vid.width, y + 1 );
}


static void GL_DrawStereoPattern (void)
{
	int i;

	if ( !glState.stereo_enabled )
		return;

	R_SetGL2D();

	glDrawBuffer( GL_BACK_LEFT );

	for ( i = 0; i < 20; i++ )
	{
		glBegin( GL_LINES );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
			GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
			GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
			GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
			GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
		glEnd();
		
		GLimp_EndFrame();
	}
}
#endif

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
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();
}


void AssertCvarRange (cvar_t *var, float min, float max, qboolean isInteger)
{
	if (!var)
		return;

	if (isInteger && ((int)var->value != var->integer))
	{
		VID_Printf (PRINT_ALL, S_COLOR_YELLOW"Warning: cvar '%s' must be an integer (%f)\n", var->name, var->value);
		Cvar_Set (var->name, va("%d", var->integer));
	}

	if (var->value < min)
	{
		VID_Printf (PRINT_ALL, S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f < %f)\n", var->name, var->value, min);
		Cvar_Set (var->name, va("%f", min));
	}
	else if (var->value > max)
	{
		VID_Printf (PRINT_ALL, S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f > %f)\n", var->name, var->value, max);
		Cvar_Set (var->name, va("%f", max));
	}
}


void R_Register (void)
{
	// added Psychospaz's console font size option
	con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);
	con_font_size = Cvar_Get ("con_font_size", "8", CVAR_ARCHIVE);
	alt_text_color = Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);
	scr_netgraph_pos = Cvar_Get ("netgraph_pos", "0", CVAR_ARCHIVE);

	gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );
	gl_allow_software = Cvar_Get( "gl_allow_software", "0", 0 );
	gl_clear = Cvar_Get ("gl_clear", "0", 0);

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
	r_displayrefresh = Cvar_Get ("r_displayrefresh", "0", CVAR_ARCHIVE); // refresh rate control
	AssertCvarRange (r_displayrefresh, 0, 150, true);

	r_width = Cvar_Get( "r_width", "1280", CVAR_ARCHIVE );
	r_height = Cvar_Get( "r_height", "800", CVAR_ARCHIVE );

	// lerped dlights on models
	r_dlights_normal = Cvar_Get("r_dlights_normal", "1", CVAR_ARCHIVE);
	r_model_shading = Cvar_Get( "r_model_shading", "2", CVAR_ARCHIVE );
	r_model_dlights = Cvar_Get( "r_model_dlights", "8", CVAR_ARCHIVE );

	r_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);
	// added Vic's RGB brightening
	r_rgbscale = Cvar_Get ("r_rgbscale", "2", CVAR_ARCHIVE);

	r_waterwave = Cvar_Get ("r_waterwave", "0", CVAR_ARCHIVE );
	r_caustics = Cvar_Get ("r_caustics", "1", CVAR_ARCHIVE );
	r_glows = Cvar_Get ("r_glows", "1", CVAR_ARCHIVE );
	r_saveshotsize = Cvar_Get ("r_saveshotsize", "1", CVAR_ARCHIVE );
	r_nosubimage = Cvar_Get( "r_nosubimage", "0", 0 );

	// correct trasparent sorting
	r_transrendersort = Cvar_Get ("r_transrendersort", "1", CVAR_ARCHIVE );
	r_particle_lighting = Cvar_Get ("r_particle_lighting", "1.0", CVAR_ARCHIVE );
	r_particledistance = Cvar_Get ("r_particledistance", "0", CVAR_ARCHIVE );
	r_particle_overdraw = Cvar_Get ("r_particle_overdraw", "0", CVAR_ARCHIVE );
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
	r_round_down = Cvar_Get ("r_round_down", "1", 0);
	r_picmip = Cvar_Get ("r_picmip", "0", 0);
	r_skymip = Cvar_Get ("r_skymip", "0", 0);
	r_showtris = Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showbbox = Cvar_Get ("r_showbbox", "0", CVAR_CHEAT); // show model bounding box
	r_ztrick = Cvar_Get ("r_ztrick", "0", 0);
	r_finish = Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_cull = Cvar_Get ("r_cull", "1", 0);
	r_polyblend = Cvar_Get ("r_polyblend", "1", 0);
	r_flashblend = Cvar_Get ("r_flashblend", "0", 0);
	r_playermip = Cvar_Get ("r_playermip", "0", 0);
	r_monolightmap = Cvar_Get( "r_monolightmap", "0", 0 );
	// changed default texture mode to bilinear
	r_texturemode = Cvar_Get( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_texturealphamode = Cvar_Get( "r_texturealphamode", "default", CVAR_ARCHIVE );
	r_texturesolidmode = Cvar_Get( "r_texturesolidmode", "default", CVAR_ARCHIVE );
	r_anisotropic = Cvar_Get( "r_anisotropic", "0", CVAR_ARCHIVE );
	r_anisotropic_avail = Cvar_Get( "r_anisotropic_avail", "0", 0 );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", 0 );

	r_vertex_arrays = Cvar_Get( "r_vertex_arrays", "1", CVAR_ARCHIVE );

	//gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "0", CVAR_ARCHIVE );
	//gl_ext_pointparameters = Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	r_ext_swapinterval = Cvar_Get( "r_ext_swapinterval", "1", CVAR_ARCHIVE );
	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );
	r_nonpoweroftwo_mipmaps = Cvar_Get("r_nonpoweroftwo_mipmaps", "1", CVAR_ARCHIVE /*| CVAR_LATCH*/);

	r_newlightmapformat = Cvar_Get("r_newlightmapformat", "1", CVAR_ARCHIVE);	// whether to use new lightmap format

	r_arb_fragment_program = Cvar_Get ("r_arb_fragment_program", "1", CVAR_ARCHIVE);
	r_arb_vertex_program = Cvar_Get ("_arb_vertex_program", "1", CVAR_ARCHIVE);

	r_arb_vertex_buffer_object = Cvar_Get ("r_arb_vertex_buffer_object", "1", CVAR_ARCHIVE);

	// allow disabling the nVidia water warp
	r_pixel_shader_warp = Cvar_Get( "r_pixel_shader_warp", "1", CVAR_ARCHIVE );

	// allow disabling of lightmaps on trans surfaces
	r_trans_lighting = Cvar_Get( "r_trans_lighting", "2", CVAR_ARCHIVE );

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

	r_ext_texture_compression = Cvar_Get( "r_ext_texture_compression", "0", CVAR_ARCHIVE ); // Heffo - ARB Texture Compression

	r_screenshot_jpeg = Cvar_Get( "r_screenshot_jpeg", "1", CVAR_ARCHIVE );					// Heffo - JPEG Screenshots
	r_screenshot_jpeg_quality = Cvar_Get( "r_screenshot_jpeg_quality", "85", CVAR_ARCHIVE );	// Heffo - JPEG Screenshots

	//r_motionblur = Cvar_Get( "r_motionblur", "0", CVAR_ARCHIVE );	// motionblur

	r_drawbuffer = Cvar_Get( "r_drawbuffer", "GL_BACK", 0 );
	r_swapinterval = Cvar_Get( "r_swapinterval", "1", CVAR_ARCHIVE );
	r_adaptivevsync = Cvar_Get( "r_adaptivevsync", "1", CVAR_ARCHIVE );

	r_saturatelighting = Cvar_Get( "r_saturatelighting", "0", 0 );

	r_3dlabs_broken = Cvar_Get( "r_3dlabs_broken", "1", CVAR_ARCHIVE );

	vid_fullscreen = Cvar_Get( "vid_fullscreen", "1", CVAR_ARCHIVE );
	vid_gamma = Cvar_Get( "vid_gamma", "0.8", CVAR_ARCHIVE ); // was 1.0
	vid_ref = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );

	r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );	// BLOOMS

	r_skydistance = Cvar_Get("r_skydistance", "10000", CVAR_ARCHIVE); // variable sky range
	r_saturation = Cvar_Get( "r_saturation", "1.0", CVAR_ARCHIVE );	//** DMP saturation setting (.89 good for nvidia)
	r_lightcutoff = Cvar_Get( "r_lightcutoff", "0", CVAR_ARCHIVE );	//** DMP dynamic light cutoffnow variable

	r_drawnullmodel = Cvar_Get("r_drawnullmodel","0", CVAR_ARCHIVE );
	r_fencesync = Cvar_Get("r_fencesync","0",CVAR_ARCHIVE );

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
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	if ( vid_fullscreen->modified && !glConfig.allowCDS )
	{
		VID_Printf (PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n" );
		Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;
	r_skydistance->modified = true; // skybox size variable


	vid_fullscreen->modified = false;

	if (r_height->value < 480 || r_width->value < 640)
	{
		Cvar_SetToDefault("r_width");
		Cvar_SetToDefault("r_height");

		VID_Printf (PRINT_ALL, "R_SetMode() - Invalid resolution set, reverting to default resolution\n" );
	}

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height, fullscreen ) ) != rserr_ok )
	{
		if ( err == rserr_invalid_fullscreen )
		{
			Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			VID_Printf (PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			Cvar_SetValue("r_width",640);
			Cvar_SetValue("r_height",480);
			VID_Printf (PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode( &vid.width, &vid.height, false ) ) != rserr_ok )
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
qboolean R_Init ( void *hinstance, void *hWnd, char *reason )
{	
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	Draw_GetPalette ();
	R_Register();

	// place default error
	memcpy (reason, "Unknown failure on intialization!\0", 34);
	
#ifdef _WIN32
	// output system info
	VID_Printf (PRINT_ALL, "OS: %s\n", Cvar_VariableString("sys_osVersion"));
	VID_Printf (PRINT_ALL, "CPU: %s\n", Cvar_VariableString("sys_cpuString"));
	VID_Printf (PRINT_ALL, "RAM: %s MB\n", Cvar_VariableString("sys_ramMegs"));
#endif
	glConfig.allowCDS = true;
	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) )
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



	//
	// get our various GL strings
	//
	glConfig.vendor_string = glGetString (GL_VENDOR);
	glConfig.renderer_string = glGetString (GL_RENDERER);
	glConfig.version_string = glGetString (GL_VERSION);
	sscanf(glConfig.version_string, "%d.%d.%d", &glConfig.version_major, &glConfig.version_minor, &glConfig.version_release);	
	
	if (glConfig.version_major < 2 || (glConfig.version_major == 2 && glConfig.version_minor < 1))
	{
		memcpy (reason, "Could not get OpenGL 2.1 or higher context!\0", 44);
		return false;
	}


	VID_Printf (PRINT_ALL, "GL_VENDOR: %s\n", glConfig.vendor_string );
	VID_Printf (PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	VID_Printf (PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );

#ifdef Q2VR_REGAL
//	glEnable(GL_LOG_WARNING_REGAL);
//	glEnable(GL_LOG_ERROR_REGAL);
//	glDisable(GL_DRIVER_REGAL);
//	glDisable(GL_EMULATION_REGAL);
	VID_Printf (PRINT_ALL, "GL_EMULATION_REGAL: %s\n", glIsEnabled(GL_EMULATION_REGAL) ? "enabled" : "disabled");
#endif

	// Knighmare- added max texture size
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&glConfig.max_texsize);
	VID_Printf (PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.max_texsize );

	glConfig.extensions_string = glGetString (GL_EXTENSIONS);
//	VID_Printf (PRINT_DEVELOPER, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	if (developer->value > 0)	// print extensions 2 to a line
	{
		char		*extString, *extTok;
		unsigned	line = 0;
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

	strcpy( renderer_buffer, glConfig.renderer_string );
	strlwr( renderer_buffer );

	strcpy( vendor_buffer, glConfig.vendor_string );
	strlwr( vendor_buffer );

	// find out the renderer model
	if (strstr(vendor_buffer, "nvidia")) {
		glConfig.rendType = GLREND_NVIDIA;
		if (strstr(renderer_buffer, "geforce"))	glConfig.rendType |= GLREND_GEFORCE;
	}
	else if (strstr(vendor_buffer, "ati")) {
		glConfig.rendType = GLREND_ATI;
		if (strstr(vendor_buffer, "radeon"))		glConfig.rendType |= GLREND_RADEON;
	}
	else if (strstr(vendor_buffer, "intel"))		glConfig.rendType = GLREND_INTEL;
	else											glConfig.rendType = GLREND_DEFAULT;


	// wat?
	if ( toupper( r_monolightmap->string[1] ) != 'F' )
	{
		// dghost - there used to be a 3DLabs branch here
		Cvar_Set( "r_monolightmap", "0" );
	}

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

	// TODO: Consider removing all this hacked up shit
	// If using one of the mini-drivers, a Voodoo w/ WickedGL, or pre-1.2 driver,
	// use the texture formats determined by gl_texturesolidmode and gl_texturealphamode.
	if (!r_newlightmapformat || !r_newlightmapformat->value)
	{
		VID_Printf (PRINT_ALL, "...using legacy lightmap format\n" );
		glConfig.newLMFormat = false;
	}
	else
	{
		VID_Printf (PRINT_ALL, "...using new lightmap format\n" );
		glConfig.newLMFormat = true;
	}

	//
	// grab extensions
	//

	// GL_EXT_compiled_vertex_array
	glConfig.extCompiledVertArray = r_ext_compiled_vertex_array->value;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
	{
		if (r_ext_compiled_vertex_array->value) {
			if (!glLockArraysEXT || !glUnlockArraysEXT) {
				VID_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_EXT_compiled_vertex_array not properly supported!\n");
				glLockArraysEXT	= NULL;
				glUnlockArraysEXT	= NULL;
			}
			else {
				VID_Printf (PRINT_ALL, "...enabling GL_EXT_compiled_vertex_array\n" );
				glConfig.extCompiledVertArray = true;
			}
		}
		else
			VID_Printf (PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n");
	}
	else
		VID_Printf (PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );


#ifdef _WIN32
	// WGL_EXT_swap_control
	glConfig.ext_swap_control = false;
	
	if ( WGLEW_EXT_swap_control )
	{
		glConfig.ext_swap_control = true;
		VID_Printf (PRINT_ALL, "...enabling WGL_EXT_swap_control\n" );
	}
	else
		
	VID_Printf (PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	
	
	// WGL_EXT_swap_control_tear
	glConfig.ext_swap_control_tear = false;

	if ( WGLEW_EXT_swap_control_tear )
	{
		glConfig.ext_swap_control_tear = true;
		VID_Printf (PRINT_ALL, "...enabling WGL_EXT_swap_control_tear\n" );
	}
	else
		VID_Printf (PRINT_ALL, "...WGL_EXT_swap_control_tear not found\n" );
#endif


	// GL_ARB_vertex_buffer_object
	glConfig.vertexBufferObject = true;

	// GL_ARB_multitexture
	if (r_ext_multitexture->value)
	{
		glConfig.multitexture = true;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texunits);
		VID_Printf (PRINT_ALL, "...GL_MAX_TEXTURE_UNITS_ARB: %i\n", glConfig.max_texunits);
	} else {
		glConfig.multitexture = false;
		glConfig.max_texunits = 1;

	}


	// GL_ARB_fragment_program
	glConfig.arb_fragment_program = false;
	if (strstr(glConfig.extensions_string, "GL_ARB_fragment_program"))
	{
		if (r_arb_fragment_program->value)
		{
			if (!GLEW_ARB_fragment_program)
				VID_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_ARB_fragment_program not properly supported!\n");
			else {
				VID_Printf (PRINT_ALL, "...using GL_ARB_fragment_program\n");
				glConfig.arb_fragment_program = true;
			}
		}
		else
			VID_Printf (PRINT_ALL, "...ignoring GL_ARB_fragment_program\n");
	}
	else
		VID_Printf (PRINT_ALL, "...GL_ARB_fragment_program not found\n");

	// GL_ARB_vertex_program
	glConfig.arb_vertex_program = false;
	if (glConfig.arb_fragment_program)
	{
		if (strstr(glConfig.extensions_string, "GL_ARB_vertex_program"))
		{
			if (r_arb_vertex_program->value)
			{
				if (!glGetVertexAttribdvARB || !glGetVertexAttribfvARB
					|| !glGetVertexAttribivARB || !glGetVertexAttribPointervARB)
					VID_Printf (PRINT_ALL, "..." S_COLOR_RED "GL_ARB_vertex_program not properly supported!\n");
				else {
					VID_Printf (PRINT_ALL, "...using GL_ARB_vertex_program\n");
					glConfig.arb_vertex_program = true;
				}
			}
			else
				VID_Printf (PRINT_ALL, "...ignoring GL_ARB_vertex_program\n");
		}
		else
			VID_Printf (PRINT_ALL, "...GL_ARB_vertex_program not found\n");
	}

	R_Compile_ARB_Programs ();

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
		} else {
			VID_Printf (PRINT_ALL, "...GL_EXT_framebuffer_object not found\n");
		}
	}


	// GL_NV_texture_shader - MrG
	if ( strstr( glConfig.extensions_string, "GL_NV_texture_shader" ) )
	{
		VID_Printf (PRINT_ALL, "...using GL_NV_texture_shader\n" );
		glConfig.NV_texshaders = true;
	}
	else
	{
		VID_Printf (PRINT_ALL, "...GL_NV_texture_shader not found\n" );
		glConfig.NV_texshaders = false;
	}

	// GL_EXT_texture_filter_anisotropic - NeVo
	glConfig.anisotropic = false;
	if ( strstr(glConfig.extensions_string,"GL_EXT_texture_filter_anisotropic") )
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
			VID_Printf (PRINT_ALL, "...using GL_ARB_sync\n" );
			glConfig.arb_sync = true;
	} else {
			VID_Printf (PRINT_ALL, "...GL_ARB_sync not found\n" );
			glConfig.arb_sync = false;
			Cvar_SetInteger("vr_fencesync",0);
	}
	
	// dghost: guaranteed extensions from OpenGL 2.0
	// TODO: consider removing legacy code
	glState.texture_compression = (qboolean) r_ext_texture_compression->value;

/*
	Com_Printf( "Size of dlights: %i\n", sizeof (dlight_t)*MAX_DLIGHTS );
	Com_Printf( "Size of entities: %i\n", sizeof (entity_t)*MAX_ENTITIES );
	Com_Printf( "Size of particles: %i\n", sizeof (particle_t)*MAX_PARTICLES );
	Com_Printf( "Size of decals: %i\n", sizeof (particle_t)*MAX_DECAL_FRAGS );
*/

	GL_SetDefaultState();

	R_VR_Init();

	// draw our stereo patterns
#if 0 // commented out until H3D pays us the money they owe us
	GL_DrawStereoPattern();
#endif

	R_InitImages ();
	Mod_Init ();
	R_InitMedia ();
	R_DrawInitLocal ();

	R_InitDSTTex (); // init shader warp texture
	R_InitFogVars (); // reset fog variables
	VLight_Init (); // Vic's bmodel lights
	RB_InitBackend(); // init mini-backend

	err = glGetError();
	if ( err != GL_NO_ERROR )
		VID_Printf (PRINT_ALL, "R_Init: glGetError() = 0x%x\n", err);


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
	unsigned	line = 0;

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
		free(saveshotdata);
	saveshotdata = NULL;	// make sure this is null after a vid restart!

	Mod_FreeAll ();

	R_ShutdownImages ();
	R_VR_Shutdown();

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
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void UpdateGammaRamp (void); //Knightmare added
void RefreshFont (void);
void R_BeginFrame( float camera_separation )
{

	glState.camera_separation = camera_separation;

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
		cvar_t	*ref;
		ref = Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	//
	// update 3Dfx gamma -- it is expected that a user will do a vid_restart
	// after tweaking this value
	//
	if ( vid_gamma->modified )
	{
		vid_gamma->modified = false;
		UpdateGammaRamp ();
	}

	GLimp_BeginFrame( camera_separation );

	//
	// go into 2D mode
	//
	glViewport (0,0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

	GL_Disable (GL_DEPTH_TEST);
	GL_Disable (GL_CULL_FACE);
	GL_Disable (GL_BLEND);
	GL_Enable (GL_ALPHA_TEST);
	glColor4f (1,1,1,1);

	//
	// draw buffer stuff
	//
	if ( r_drawbuffer->modified )
	{
		r_drawbuffer->modified = false;

		if (!vr_enabled->value)
		{
			if ( glState.camera_separation == 0 || !glState.stereo_enabled )
			{
				if ( Q_stricmp( r_drawbuffer->string, "GL_FRONT" ) == 0 )
					glDrawBuffer( GL_FRONT );
				else
					glDrawBuffer( GL_BACK );
			}
		}
	}

	//
	// texturemode stuff
	//
	if ( r_texturemode->modified )
	{
		GL_TextureMode( r_texturemode->string );
		r_texturemode->modified = false;
	}

	if ( r_texturealphamode->modified )
	{
		GL_TextureAlphaMode( r_texturealphamode->string );
		r_texturealphamode->modified = false;
	}

	if ( r_texturesolidmode->modified )
	{
		GL_TextureSolidMode( r_texturesolidmode->string );
		r_texturesolidmode->modified = false;
	}

	//
	// swapinterval stuff
	//
	GL_UpdateSwapInterval();

	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

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

	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT);
	glClearColor (1,0, 0.5 , 0.5);
}

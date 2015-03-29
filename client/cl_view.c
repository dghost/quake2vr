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
// cl_view.c -- player rendering positioning

#include "client.h"
#include "renderer/include/r_vr.h"
#include "renderer/include/r_stereo.h"


//=============
//
// development tools for weapons
//
int32_t			gun_frame;
struct		model_s	*gun_model;

//=============

cvar_t		*cl_testparticles;
cvar_t		*cl_testentities;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;

extern cvar_t		*hand;

int32_t			r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int32_t			r_numentities;
entity_t	r_entities[MAX_ENTITIES];

int32_t			r_numparticles;
particle_t	r_particles[MAX_PARTICLES];

int32_t			r_numdecalfrags;
particle_t	r_decalfrags[MAX_DECAL_FRAGS];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

char		cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int32_t			num_cl_weaponmodels;

vec3_t clientOrg;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
	r_numdecalfrags = 0;
}

// Knightmare- added Psychospaz's chasecam
/*
=====================

3D Cam Stuff -psychospaz

=====================
*/
#define CAM_MAXALPHADIST 0.000111
float viewermodelalpha;

void ClipCam (vec3_t start, vec3_t end, vec3_t newpos)
{
	int32_t i;

	trace_t tr = CL_Trace (start, end, 5, MASK_SOLID);
	for (i=0;i<3;i++)
		newpos[i]=tr.endpos[i];
}

void AddViewerEntAlpha (entity_t *ent)
{
	if (viewermodelalpha == 1 || !cg_thirdperson_alpha->value)
		return;

	ent->alpha *= viewermodelalpha;
	if (ent->alpha < 1.0F)
		ent->flags |= RF_TRANSLUCENT;
}

#define ANGLEMAX 90.0
void CalcViewerCamTrans (float distance)
{
	float alphacalc = cg_thirdperson_dist->value;

	// no div by 0
	if (alphacalc < 1)
		alphacalc = 1;

	viewermodelalpha = distance/alphacalc;

	if (viewermodelalpha>1)
		viewermodelalpha = 1;
}


/*
=====================
V_AddEntity
=====================
*/
void V_AddEntity (entity_t *ent)
{
	// Knightmare- added Psychospaz's chasecam
	if (ent->flags & RF_VIEWERMODEL) //here is our client
	{	int32_t i; 

		// what was i thinking before!?
		for (i=0;i<3;i++)
			clientOrg[i] = ent->oldorigin[i] = ent->origin[i] = cl.predicted_origin[i];

		if (hand->value == 1) //lefthanded
			ent->flags |= RF_MIRRORMODEL;

		if (cg_thirdperson->value
			&& !(cl.attractloop && !(cl.cinematictime > 0 && cls.realtime - cl.cinematictime > 1000)))
		{
			AddViewerEntAlpha(ent);
			ent->flags &= ~RF_VIEWERMODEL;
			ent->renderfx |= RF2_CAMERAMODEL;
		}
	}
	// end Knightmare
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}


/*
=====================
V_AddParticle
=====================
*/
//Knightmare- Psychospaz's enhanced particle code
void V_AddParticle (vec3_t org, vec3_t angle, vec3_t color, float alpha,
				int32_t alpha_src, int32_t alpha_dst, float size, int32_t image, int32_t flags)
{
	int32_t i;
	particle_t	*p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];

	for (i=0;i<3;i++)
	{
		p->origin[i] = org[i];
		p->angle[i] = angle[i];
	}
	p->red = color[0];
	p->green = color[1];
	p->blue = color[2];
	p->alpha = alpha;
	p->image = image;
	p->flags = flags;
	p->size  = size;
	p->decal = NULL;
	p->blendfunc_src = alpha_src;
	p->blendfunc_dst = alpha_dst;
}
//end Knightmare

/*
=====================
V_AddDecal
=====================
*/
void V_AddDecal (vec3_t org, vec3_t angle, vec3_t color, float alpha,
				int32_t alpha_src, int32_t alpha_dst, float size, int32_t image, int32_t flags, decalpolys_t *decal)
{
	int32_t i;
	particle_t	*d;

	if (r_numdecalfrags >= MAX_DECAL_FRAGS)
		return;
	d = &r_decalfrags[r_numdecalfrags++];

	for (i=0;i<3;i++)
	{
		d->origin[i] = org[i];
		d->angle[i] = angle[i];
	}
	d->red = color[0];
	d->green = color[1];
	d->blue = color[2];
	d->alpha = alpha;
	d->image = image;
	d->flags = flags;
	d->size  = size;
	d->decal = decal;

	d->blendfunc_src = alpha_src;
	d->blendfunc_dst = alpha_dst;
}

/*
=====================
V_AddLight

=====================
*/
void V_AddLight (vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	VectorCopy (vec3_origin, dl->direction);
	dl->spotlight = false;
}

/*
=====================
V_AddSpotLight

=====================
*/
void V_AddSpotLight (vec3_t org, vec3_t direction, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	VectorCopy(direction, dl->direction);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;

	dl->spotlight=true;
}

/*
=====================
V_AddLightStyle

=====================
*/
void V_AddLightStyle (int32_t style, float r, float g, float b)
{
	lightstyle_t	*ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error (ERR_DROP, "Bad light style %i", style);
	ls = &r_lightstyles[style];

	ls->white = r+g+b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int32_t			i, j;
	float		d, r, u;

	r_numparticles = 4096;
	for (i=0 ; i<r_numparticles ; i++)
	{
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);
		p = &r_particles[i];

		for (j=0; j<3; j++)
			p->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*d +
			cl.v_right[j]*r + cl.v_up[j]*u;

		p->color = 8;
		p->alpha = cl_testparticles->value;
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities (void)
{
	int32_t			i, j;
	float		f, r;
	entity_t	*ent;

	r_numentities = 32;
	memset (r_entities, 0, sizeof(r_entities));

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &r_entities[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0; j<3; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;

		ent->model = cl.baseclientinfo.model;
		ent->skin = cl.baseclientinfo.skin;
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int32_t			i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = 32;
	memset (r_dlights, 0, sizeof(r_dlights));

	for (i=0 ; i<r_numdlights ; i++)
	{
		dl = &r_dlights[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0; j<3; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->intensity = 200;
		dl->spotlight = false;
	}
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
qboolean needLoadingPlaque (void);
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int32_t			i, max;
	char		pname[MAX_QPATH];
	float		rotate;
	vec3_t		axis;
	qboolean	newPlaque = needLoadingPlaque();

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	if (newPlaque)
		SCR_BeginLoadingPlaque();

	// Knightmare- for Psychospaz's map loading screen
	loadingMessage = true;
	Com_sprintf (loadingMessages, sizeof(loadingMessages), S_COLOR_ALT"loading %s", cl.configstrings[CS_MODELS+1]);
	loadingPercent = 0.0f;
	// end Knightmare

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	R_BeginRegistration (mapname);
	Com_Printf ("                                     \r");

	// Knightmare- for Psychospaz's map loading screen
	Com_sprintf (loadingMessages, sizeof(loadingMessages), S_COLOR_ALT"loading models...");
	loadingPercent += 20.0f;
	// end Knightmare

	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	CL_RegisterTEntModels ();

	num_cl_weaponmodels = 1;
	strcpy(cl_weaponmodels[0], "weapon.md2");

	// Knightmare- for Psychospaz's map loading screen
	for (i=1, max=0 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
		max++;

	for (i=1; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0]; i++)
	{
		strcpy (pname, cl.configstrings[CS_MODELS+i]);
		pname[37] = 0;	// never go beyond one line
		if (pname[0] != '*')
		{
			Com_Printf ("%s\r", pname); 
			// Knightmare- for Psychospaz's map loading screen
			//only make max of 40 chars long
			if (i > 1)
				Com_sprintf (loadingMessages, sizeof(loadingMessages),
					S_COLOR_ALT"loading %s", (strlen(pname)>40)? &pname[strlen(pname)-40]: pname);
		}

		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		if (pname[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		} 
		else
		{
			cl.model_draw[i] = R_RegisterModel (cl.configstrings[CS_MODELS+i]);
			if (pname[0] == '*')
				cl.model_clip[i] = CM_InlineModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_clip[i] = NULL;
		}
		if (pname[0] != '*')
			Com_Printf ("                                     \r");
		// Knightmare- for Psychospaz's map loading screen
		loadingPercent += 40.0f/(float)max;
	}
	// Knightmare- for Psychospaz's map loading screen
	Com_sprintf (loadingMessages, sizeof(loadingMessages), S_COLOR_ALT"loading pics...");

	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();

	// Knightmare- BIG UGLY HACK for connected to server using old protocol
	// Changed configstrings require different parsing
	if (LegacyProtocol() )
	{	// Knightmare- for Psychospaz's map loading screen
		for (i=1, max=0; i<OLD_MAX_IMAGES && cl.configstrings[OLD_CS_IMAGES+i][0]; i++)
			max++;
		for (i=1; i<OLD_MAX_IMAGES && cl.configstrings[OLD_CS_IMAGES+i][0]; i++)
		{
			cl.image_precache[i] = R_DrawFindPic (cl.configstrings[OLD_CS_IMAGES+i]);
			Sys_SendKeyEvents ();	// pump message loop
			// Knightmare- for Psychospaz's map loading screen
			loadingPercent += 20.0f/(float)max;
		}
	}
	else
	{	// Knightmare- for Psychospaz's map loading screen
		for (i=1, max=0; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0]; i++)
			max++;
		for (i=1; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0]; i++)
		{
			cl.image_precache[i] = R_DrawFindPic (cl.configstrings[CS_IMAGES+i]);
			Sys_SendKeyEvents ();	// pump message loop
			// Knightmare- for Psychospaz's map loading screen
			loadingPercent += 20.0f/(float)max;
		}
	}
	// Knightmare- for Psychospaz's map loading screen
	Com_sprintf (loadingMessages, sizeof(loadingMessages), S_COLOR_ALT"loading players...");

	Com_Printf ("                                     \r");

	// Knightmare- for Psychospaz's map loading screen
	for (i=1, max=0 ; i<MAX_CLIENTS ; i++)
		if ( LegacyProtocol() ) {
			if (cl.configstrings[OLD_CS_PLAYERSKINS+i][0])
				max++;
		} else {
			if (cl.configstrings[CS_PLAYERSKINS+i][0])
				max++;
		}

	for (i=0; i < MAX_CLIENTS; i++)
	{
		// Knightmare- BIG UGLY HACK for old connected to server using old protocol
		// Changed configstrings require different parsing
		if ( LegacyProtocol() ) {
			if (!cl.configstrings[OLD_CS_PLAYERSKINS+i][0])
				continue;
		} else {
			if (!cl.configstrings[CS_PLAYERSKINS+i][0])
				continue;
		}
		Com_Printf ("client %i\r", i); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");

		// Knightmare- for Psychospaz's map loading screen
		loadingPercent += 20.0f/(float)max;
	}
	// Knightmare- for Psychospaz's map loading screen
	Com_sprintf (loadingMessages, sizeof(loadingMessages), S_COLOR_ALT"loading players...done");
	//hack hack hack - psychospaz
	loadingPercent = 100.0f;

	// Knightmare - Vics fix to get rid of male/grunt flicker
	// CL_LoadClientinfo (&cl.baseclientinfo, "unnamed\\male/grunt");
	CL_LoadClientinfo (&cl.baseclientinfo, va("unnamed\\%s", skin->string));

	// Knightmare- refresh the player model/skin info
	userinfo_modified = true;

	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();
	rotate = atof (cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", 
		&axis[0], &axis[1], &axis[2]);
	R_SetSky (cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	R_EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start the cd track
	CL_PlayBackgroundTrack ();

	// Knightmare- for Psychospaz's map loading screen
	loadingMessage = false;
	// Knightmare- close loading screen as soon as done
	cls.disable_screen = false;

	// Knightmare- don't start map with game paused
	if (cls.key_dest != key_menu)
		Cvar_Set ("paused", "0");

	// dghost - reset HMD home position before starting new map
	VR_ResetOrientation();
}

/*
====================
CalcFov
====================
*/
extern qboolean sbsEnabled;
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

// gun frame debugging functions
void V_Gun_Next_f (void)
{
	gun_frame++;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	char	name[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		gun_model = NULL;
		return;
	}
	Com_sprintf (name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = R_RegisterModel (name);
}

//============================================================================

eyeScaleOffset_t R_ApplyWarpToProjection(eyeScaleOffset_t projection)
{
	eyeScaleOffset_t result = projection;
	// Barnes- Warp if underwater ala q3a :-)

	float fov_x = RAD2DEG(atan2f(1,projection.x.scale)) * 2.0;
	float fov_y = RAD2DEG(atan2f(1,projection.y.scale)) * 2.0;
	float f = sin(cl.time * 0.001 * 0.4 * (M_PI*2.7));
	fov_x += f * (fov_x/90.0); // Knightmare- scale to fov
	fov_y -= f * (fov_y/90.0); // Knightmare- scale to fov
	result.x.scale = 1.0f / tanf(DEG2RAD(fov_x / 2.0f));
	result.y.scale = 1.0f / tanf(DEG2RAD(fov_y / 2.0f));
	return result;
}

/*
==================
VR_RenderStereo
==================
*/
void R_RenderView (refdef_t *fd);
void R_RenderViewIntoFBO (refdef_t *fd, eye_param_t parameters, fbo_t *destination, vrect_t *viewRect);
void R_SetLightLevel ();
void R_SetGL2D ();
void R_RenderCommon (refdef_t *fd);
void R_Clear(void);
void VR_RenderStereo ()
{
	extern int32_t entitycmpfnc( const entity_t *, const entity_t * );
	vec3_t view,viewOrig, offset;
	int index;

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}


	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( (cl.frame.valid && (cl.force_refdef || !cl_paused->value)) || cl_paused->value )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		// FOV init

		if (vr_autofov->value)
		{
			R_VR_GetFOV(&cl.refdef.fov_x,&cl.refdef.fov_y);
			cl.refdef.fov_x *= vr_autofov_scale->value;
			cl.refdef.fov_y *= vr_autofov_scale->value;
		}
		else {
			float width = vrState.eyeFBO[0]->width;
			float height = vrState.eyeFBO[0]->height;
			//	float standardRatio, currentRatio;
			//	standardRatio = (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
			//	currentRatio = (float)cl.refdef.width/(float)cl.refdef.height;
			//	if (currentRatio > standardRatio)
			//		cl.refdef.fov_x *= (1 + (0.5 * (currentRatio / standardRatio - 1)));
			float aspectRatio = width/height;
			// changed to improved algorithm by Dopefish
			if (aspectRatio > STANDARD_ASPECT_RATIO)
				cl.refdef.fov_x = RAD2DEG( 2 * atan( (aspectRatio/ STANDARD_ASPECT_RATIO) * tan(DEG2RAD(cl.refdef.fov_x) * 0.5) ) );
			//	cl.refdef.fov_x *= (1 + (0.5 * (aspectRatio / STANDARD_ASPECT_RATIO - 1)));
			cl.refdef.fov_x = min(cl.refdef.fov_x, 160);

			cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, width, height);
		}

		cl.refdef.time = cl.time*0.001;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;

		cl.refdef.num_decals = r_numdecalfrags;
		cl.refdef.decals = r_decalfrags;

		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int32_t (*)(const void *, const void *))entitycmpfnc );
	}

		// Neckmodel stuff

	R_RenderCommon(&cl.refdef);

	VectorCopy(cl.refdef.vieworg, viewOrig);
	VectorCopy(cl.refdef.vieworg, view);

	// head and neck model stuff
	if (VR_GetHeadOffset(offset))
	{
		vec3_t orient;
		vec3_t out;
		vec3_t flatView,forward,right,up;
		float yaw;
		
		VR_GetOrientation(orient);

		yaw = cl.refdef.viewangles[YAW]- orient[YAW];

		// clamp yaw to [-180,180]
		yaw = yaw - floor((yaw + 180.0f) * (1.0f / 360.0f)) * 360.0f;

		if (yaw > 180.0f)
			yaw -= 360.0f;

		VectorSet(flatView,0,yaw,0);
		AngleVectors(flatView,forward,right,up);

		VectorNormalize(forward);
		VectorNormalize(up);
		VectorNormalize(right);

		// apply this using X forward, Y right, Z up
		VectorScale(forward, offset[0] ,forward);
		VectorScale(right,offset[1],right);
		VectorScale(up,offset[2],up);
		VectorAdd(forward,up,out);
		VectorAdd(out,right,out);
		VectorAdd(view,out,view);
	}
	else if (vr_neckmodel->value)
	{
		vec3_t forward, up, out;
		float eyeDist = vr_neckmodel_forward->value * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
		float neckLength = vr_neckmodel_up->value * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
		VectorCopy(cl.v_forward,forward);
		VectorCopy(cl.v_up,up);
		VectorNormalize(forward);
		VectorNormalize(up);
		VectorScale(forward, eyeDist ,forward);
		VectorScale(up,neckLength,up);
		VectorAdd(forward,up,out);
		out[2] -= neckLength;
		VectorAdd(view,out,view); 
	}

	VectorCopy(view,cl.refdef.vieworg);
	// left eye rendering
	for (index = 0; index < NUM_EYES; index++)
	{
		eye_param_t params;
		int eyeSign = (-1 + index * 2);

		if (vr_autofov->value)
		{
			params.projection = vrState.renderParams[index].projection;
			params.projection.x.scale *= vr_autofov_scale->value;
			params.projection.y.scale *= vr_autofov_scale->value;
		} else {
			float aspectRatio = (float)vrState.eyeFBO[index]->height/(float)vrState.eyeFBO[index]->width;
			
			params.projection.x.scale =  1.0f / tanf((cl.refdef.fov_x / 2.0f) * M_PI / 180);
			params.projection.x.offset = vrState.renderParams[index].projection.x.offset;
			params.projection.y.scale = params.projection.x.scale / aspectRatio;
			params.projection.y.offset = 0.0;
		}
		
		if (cl.refdef.rdflags & RDF_UNDERWATER)
			params.projection = R_ApplyWarpToProjection(params.projection);
		if (vr_autoipd->value)
		{
			VectorScale(vrState.renderParams[index].viewOffset,PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M, params.viewOffset);
		} else {
			float viewOffset = (vr_ipd->value / 2000.0) * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
			VectorSet(params.viewOffset,eyeSign * viewOffset ,0,0);
		}

		R_RenderViewIntoFBO( &cl.refdef, params,vrState.eyeFBO[index],NULL);	
	}

	// cleanup
	VectorCopy(viewOrig, cl.refdef.vieworg);

	R_SetLightLevel ();
	R_SetGL2D ();

	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);
}


/*
==================
R_RenderStereo
==================
*/

void R_RenderStereo ()
{
	extern int32_t entitycmpfnc( const entity_t *, const entity_t * );
	float f; // Barnes added
	eye_param_t params;
	int index;

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = 0;
		cl.refdef.y = 0;
		cl.refdef.width = stereo_fbo[0].width;
		cl.refdef.height = stereo_fbo[0].height;

		// adjust fov for wide aspect ratio
		if (cl_widescreen_fov->value)
		{
		//	float standardRatio, currentRatio;
		//	standardRatio = (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
		//	currentRatio = (float)cl.refdef.width/(float)cl.refdef.height;
		//	if (currentRatio > standardRatio)
		//		cl.refdef.fov_x *= (1 + (0.5 * (currentRatio / standardRatio - 1)));
			float aspectRatio = (float)(cl.refdef.width * 2.0)/(float)cl.refdef.height;
			// changed to improved algorithm by Dopefish
			if (aspectRatio > STANDARD_ASPECT_RATIO)
				cl.refdef.fov_x = RAD2DEG( 2 * atan( (aspectRatio/ STANDARD_ASPECT_RATIO) * tan(DEG2RAD(cl.refdef.fov_x) * 0.5) ) );
			//	cl.refdef.fov_x *= (1 + (0.5 * (aspectRatio / STANDARD_ASPECT_RATIO - 1)));
			cl.refdef.fov_x = min(cl.refdef.fov_x, 160);
		}
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, (cl.refdef.width * 2.0), cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		// Barnes- Warp if underwater ala q3a :-)
		if (cl.refdef.rdflags & RDF_UNDERWATER) {
			f = sin(cl.time * 0.001 * 0.4 * (M_PI*2.7));
			cl.refdef.fov_x += f * (cl.refdef.fov_x/90.0); // Knightmare- scale to fov
			cl.refdef.fov_y -= f * (cl.refdef.fov_y/90.0); // Knightmare- scale to fov
		} // end Barnes

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;

		cl.refdef.num_decals = r_numdecalfrags;
		cl.refdef.decals = r_decalfrags;

		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int32_t (*)(const void *, const void *))entitycmpfnc );
	}

	params.projection.y.scale = 1.0f / tanf((cl.refdef.fov_y / 2.0f) * M_PI / 180);
	params.projection.y.offset = 0.0;
	params.projection.x.scale = 1.0f / tanf((cl.refdef.fov_x / 2.0f) * M_PI / 180);

	params.projection.x.offset = 0.0;


	R_RenderCommon(&cl.refdef);

	
	// left eye rendering
	for (index = 0; index < 2; index++)
	{
		int eyeSign = (-1 + index * 2);
		float viewOffset = (r_stereo_separation->value / 2000.0) * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
		VectorSet(params.viewOffset,eyeSign * viewOffset ,0,0);
		params.projection.x.offset = eyeSign * r_stereo_separation->value / 2000.0;

		R_RenderViewIntoFBO( &cl.refdef, params,&stereo_fbo[index],NULL);	
	}

	// cleanup
	R_SetLightLevel ();
	R_SetGL2D ();

	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);
}

/*
==================
V_RenderView
==================
*/
void V_RenderView ()
{
	extern int32_t entitycmpfnc( const entity_t *, const entity_t * );
	float f; // Barnes added

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;

		// adjust fov for wide aspect ratio
		if (cl_widescreen_fov->value)
		{
		//	float standardRatio, currentRatio;
		//	standardRatio = (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
		//	currentRatio = (float)cl.refdef.width/(float)cl.refdef.height;
		//	if (currentRatio > standardRatio)
		//		cl.refdef.fov_x *= (1 + (0.5 * (currentRatio / standardRatio - 1)));
			float aspectRatio = (float)cl.refdef.width/(float)cl.refdef.height;
			// changed to improved algorithm by Dopefish
			if (aspectRatio > STANDARD_ASPECT_RATIO)
				cl.refdef.fov_x = RAD2DEG( 2 * atan( (aspectRatio/ STANDARD_ASPECT_RATIO) * tan(DEG2RAD(cl.refdef.fov_x) * 0.5) ) );
			//	cl.refdef.fov_x *= (1 + (0.5 * (aspectRatio / STANDARD_ASPECT_RATIO - 1)));
			cl.refdef.fov_x = min(cl.refdef.fov_x, 160);
		}
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		// Barnes- Warp if underwater ala q3a :-)
		if (cl.refdef.rdflags & RDF_UNDERWATER) {
			f = sin(cl.time * 0.001 * 0.4 * (M_PI*2.7));
			cl.refdef.fov_x += f * (cl.refdef.fov_x/90.0); // Knightmare- scale to fov
			cl.refdef.fov_y -= f * (cl.refdef.fov_y/90.0); // Knightmare- scale to fov
		} // end Barnes

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;

		cl.refdef.num_decals = r_numdecalfrags;
		cl.refdef.decals = r_decalfrags;

		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int32_t (*)(const void *, const void *))entitycmpfnc );
	}

	R_RenderCommon(&cl.refdef);

	R_RenderView( &cl.refdef );	

	R_SetLightLevel ();
	R_SetGL2D ();
	
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);
}

void V_RenderViewIntoFBO (fbo_t *fbo)
{
	extern int32_t entitycmpfnc( const entity_t *, const entity_t * );
	float f; // Barnes added
	eye_param_t params;

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = 0;
		cl.refdef.y = 0;
		cl.refdef.width = fbo->width;
		cl.refdef.height = fbo->height;

		// adjust fov for wide aspect ratio
		if (cl_widescreen_fov->value)
		{
		//	float standardRatio, currentRatio;
		//	standardRatio = (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
		//	currentRatio = (float)cl.refdef.width/(float)cl.refdef.height;
		//	if (currentRatio > standardRatio)
		//		cl.refdef.fov_x *= (1 + (0.5 * (currentRatio / standardRatio - 1)));
			float aspectRatio = (float)cl.refdef.width/(float)cl.refdef.height;
			// changed to improved algorithm by Dopefish
			if (aspectRatio > STANDARD_ASPECT_RATIO)
				cl.refdef.fov_x = RAD2DEG( 2 * atan( (aspectRatio/ STANDARD_ASPECT_RATIO) * tan(DEG2RAD(cl.refdef.fov_x) * 0.5) ) );
			//	cl.refdef.fov_x *= (1 + (0.5 * (aspectRatio / STANDARD_ASPECT_RATIO - 1)));
			cl.refdef.fov_x = min(cl.refdef.fov_x, 160);
		}
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		// Barnes- Warp if underwater ala q3a :-)
		if (cl.refdef.rdflags & RDF_UNDERWATER) {
			f = sin(cl.time * 0.001 * 0.4 * (M_PI*2.7));
			cl.refdef.fov_x += f * (cl.refdef.fov_x/90.0); // Knightmare- scale to fov
			cl.refdef.fov_y -= f * (cl.refdef.fov_y/90.0); // Knightmare- scale to fov
		} // end Barnes

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;

		cl.refdef.num_decals = r_numdecalfrags;
		cl.refdef.decals = r_decalfrags;

		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int32_t (*)(const void *, const void *))entitycmpfnc );
	}

	params.projection.y.scale = 1.0f / tanf((cl.refdef.fov_y / 2.0f) * M_PI / 180);
	params.projection.y.offset = 0.0;
	params.projection.x.scale = 1.0f / tanf((cl.refdef.fov_x / 2.0f) * M_PI / 180);

	params.projection.x.offset = 0.0;
	VectorSet(params.viewOffset,0,0,0);

	R_RenderCommon(&cl.refdef);

	R_RenderViewIntoFBO( &cl.refdef, params,fbo,NULL);	

	R_SetLightLevel ();
	R_SetGL2D ();
	
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);
}

/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : %i\n", (int32_t)cl.refdef.vieworg[0],
		(int32_t)cl.refdef.vieworg[1], (int32_t)cl.refdef.vieworg[2], 
		(int32_t)cl.refdef.viewangles[YAW]);
}

// Knightmare- diagnostic commands from Lazarus
/*
=============
V_Texture_f
=============
*/
void V_Texture_f (void)
{
	trace_t	tr;
	vec3_t	forward, start, end;

	if (!developer->value) // only works in developer mode
		return;

	VectorCopy(cl.refdef.vieworg, start);
	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(start, 8192, forward, end);
	tr = CL_PMSurfaceTrace(cl.playernum+1, start,NULL,NULL,end,MASK_ALL);
	if (!tr.ent)
		Com_Printf("Nothing hit?\n");
	else {
		if (!tr.surface)
			Com_Printf("Not a brush\n");
		else
			Com_Printf("Texture=%s, surface=0x%08x, value=%d\n",tr.surface->name,tr.surface->flags,tr.surface->value);
	}
}

/*
=============
V_Surf_f
=============
*/
void V_Surf_f (void)
{
	trace_t	tr;
	vec3_t	forward, start, end;
	int32_t		s;

	if (!developer->value) // only works in developer mode
		return;

	// Disable this in multiplayer
	if ( cl.configstrings[CS_MAXCLIENTS][0] &&
		strcmp(cl.configstrings[CS_MAXCLIENTS], "1") )
		return;

	if (Cmd_Argc() < 2)
	{
		Com_Printf("Syntax: surf <value>\n");
		return;
	}
	else
		s = atoi(Cmd_Argv(1));

	VectorCopy(cl.refdef.vieworg, start);
	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(start, 8192, forward, end);
	tr = CL_PMSurfaceTrace(cl.playernum+1, start,NULL,NULL,end,MASK_ALL);
	if (!tr.ent)
		Com_Printf("Nothing hit?\n");
	else
	{
		if (!tr.surface)
			Com_Printf("Not a brush\n");
		else
			tr.surface->flags = s;
	}
}


/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("gun_next", V_Gun_Next_f);
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);

	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	// Knightmare- diagnostic commands from Lazarus
	Cmd_AddCommand ("texture", V_Texture_f);
	Cmd_AddCommand ("surf", V_Surf_f);
	//Cmd_AddCommand ("bbox", V_BBox_f);

	hand = Cvar_Get ("hand", "0", CVAR_ARCHIVE);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", CVAR_CHEAT);

	cl_stats = Cvar_Get ("cl_stats", "0", 0);

}

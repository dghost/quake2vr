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
// r_warp.c -- sky and water polygons

#include "include/r_local.h"

extern	model_t	*loadmodel;
msurface_t	*warpface;

#define	SUBDIVIDE_SIZE	32

void BoundPoly (int32_t numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int32_t		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 999999;
	maxs[0] = maxs[1] = maxs[2] = -999999;
	v = verts;
	for (i=0; i<numverts; i++)
		for (j=0; j<3; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon (int32_t numverts, float *verts)
{
	int32_t		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[128], back[128];
	int32_t		f, b, size;
	float	dist[128];
	float	frac;
	glpoly_t	*poly;
	float	s, t;
	vec3_t	total;
	float	total_s, total_t;

	if (numverts > 124)
		VID_Error (ERR_DROP, "numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0; i<3; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor (m/SUBDIVIDE_SIZE + 0.5);
		if (maxs[i] - m < 1)
			continue;
		if (m - mins[i] < 1)
			continue;

		// cut it
		v = verts + i;
		for (j=0; j<numverts; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0; j<numverts; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0; k<3; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = (glpoly_t*)Hunk_Alloc (sizeof(glpoly_t) + ((numverts-4)+2) * VERTEXSIZE*sizeof(float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts+2;
	
	// alloc vertex light fields
	size = poly->numverts*3*sizeof(byte);
	poly->vertexlight = (byte*)Hunk_Alloc(size);
	poly->vertexlightbase = (byte*)Hunk_Alloc(size);
	memset(poly->vertexlight, 0, size);
	memset(poly->vertexlightbase, 0, size);
	poly->vertexlightset = false;
	

	VectorClear (total);
	total_s = 0;
	total_t = 0;
	for (i=0; i<numverts; i++, verts+=3)
	{
		VectorCopy (verts, poly->verts[i+1]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd (total, verts, total);

		poly->verts[i+1][3] = s;
		poly->verts[i+1][4] = t;
	}

	VectorScale (total, (1.0/(float)numverts), poly->verts[0]);
	VectorCopy(poly->verts[0], poly->center); // for vertex lighting
	poly->verts[0][3] = total_s/numverts;
	poly->verts[0][4] = total_t/numverts;

	// copy first vertex to last
	memcpy (poly->verts[i+1], poly->verts[1], sizeof(poly->verts[0]));
}

/*
================
R_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent warps
can be done reasonably.
================
*/
void R_SubdivideSurface (msurface_t *fa)
{
	vec3_t		verts[64];
	int32_t			numverts;
	int32_t			i;
	int32_t			lindex;
	float		*vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0; i<fa->numedges; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	r_turbsin[] =
{
	#include "include/warpsin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))



// MrG - texture shader stuffs
#define DST_SIZE 32
uint32_t dst_texture;

/*
===============
CreateDSTTex

Create the texture which warps texture shaders
===============
*/
void CreateDSTTex (void)
{
	uint8_t	dist[DST_SIZE][DST_SIZE][4];
	int32_t				x,y;

#ifdef WIN32
	srand(GetTickCount());
#else
	srand(time(NULL));
#endif
	for (x=0; x<DST_SIZE; x++)
		for (y=0; y<DST_SIZE; y++) {
			dist[x][y][0] = rand()%255;
			dist[x][y][1] = rand()%255;
			dist[x][y][2] = rand()%48;
			dist[x][y][3] = rand()%48;
		}

	glGenTextures(1,&dst_texture);
	GL_Bind(dst_texture);

	glTexImage2D (GL_TEXTURE_2D, 0, 4, DST_SIZE, DST_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, dist);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
}

/*
===============
R_InitDSTTex

Resets the texture which warps texture shaders.
Needed after a vid_restart.
===============
*/
void R_InitDSTTex (void)
{
	dst_texture = 0;
	CreateDSTTex ();
}
//end MrG


image_t *R_TextureAnimation (msurface_t *surf);

/*
=============
RB_RenderWarpSurface

backend for R_DrawWarpSurface
=============
*/
void RB_RenderWarpSurface (msurface_t *fa)
{
//	float		args[7] = {0,0.05,0,0,0.04,0,0};
//	float		alpha = colorArray[0][3];
	image_t		*image = R_TextureAnimation (fa);
	qboolean	light = r_warp_lighting->value && !(fa->texinfo->flags & SURF_NOLIGHTENV);
	qboolean	texShaderWarp = r_waterquality->value;

	if (rb_vertex == 0 || rb_index == 0) // nothing to render
		return;

	c_brush_calls++;

	// Psychospaz's vertex lighting
	if (light) {
		GL_ShadeModel (GL_SMOOTH);
		if (!texShaderWarp)
			R_SetVertexRGBScale (true);
	}

	/*
	Texture Shader waterwarp
	Damn this looks fantastic
	WHY texture shaders? because I can!
	- MrG
	*/
	if (texShaderWarp)
	{
        int fog = (r_fogenable) ? (r_fogmodel + 1) : 0;
        
		GLfloat param[4];
		float rdt = r_newrefdef.time;
		float scale[2] = {0.025,0.05};
		r_warpshader_t *warp;

		//GL_SelectTexture(0);
		GL_MBind(0, image->texnum);

		GL_EnableTexture(1);
		GL_MBind(1, dst_texture);

		Vector4Set(param,r_rgbscale->value,r_rgbscale->value,r_rgbscale->value,1.0);
		if ( !(fa->texinfo->flags & SURF_FLOWING)
			&& fa->plane->normal[2] > 0
			&& fa->plane->normal[2] > fa->plane->normal[0]
			&& fa->plane->normal[2] > fa->plane->normal[1] 
			&& r_waterwave->value > 0)
		{
			if (r_waterquality->value == 3)
				warp = &simplexwarpshader;
			else 
				warp = &warpshader;

			glUseProgram(warp->shader->program);
			glUniform4fv(warp->rgbscale_uniform,1,param);
			glUniform1f(warp->time_uniform,rdt);
			glUniform1f(warp->displacement_uniform,r_waterwave->value);
			glUniform2fv(warp->scale_uniform,1,scale);
            glUniform1i(warp->fogmodel_uniform, fog);
		} else {
			glUseProgram(causticshader.shader->program);
			glUniform4fv(causticshader.rgbscale_uniform,1,param);
            glUniform1i(causticshader.fogmodel_uniform, fog);
		}
	}
	else
		GL_MBind(0,image->texnum);

	RB_DrawArrays ();

	// MrG - texture shader waterwarp
	if (texShaderWarp)
	{
		glUseProgram(0);
		GL_DisableTexture(1);
		GL_SelectTexture(0);
	}

	// Psychospaz's vertex lighting
	if (light) {
		GL_ShadeModel (GL_FLAT);
		if (!texShaderWarp)
			R_SetVertexRGBScale (false); 
	}

	RB_DrawMeshTris ();
	rb_vertex = rb_index = 0;
}

/*
=============
R_DrawWarpSurface

Does a water warp on the pre-fragmented glpoly_t chain.
added Psychospaz's lightmaps on alpha surfaces
=============
*/
void R_DrawWarpSurface (msurface_t *fa, float alpha, qboolean render)
{
	glpoly_t	*p, *bp;
	float		*v, s, t, scroll, dstscroll, rdt = r_newrefdef.time;
	int32_t			i;
	qboolean	light = r_warp_lighting->value && !r_fullbright->value && !(fa->texinfo->flags & SURF_NOLIGHTENV);

	c_brush_surfs++;

	dstscroll = -64 * ( (r_newrefdef.time*0.15) - (int32_t)(r_newrefdef.time*0.15) );

	if (fa->texinfo->flags & SURF_FLOWING)
		scroll = -64 * ( (r_newrefdef.time*0.5) - (int32_t)(r_newrefdef.time*0.5) );
	else
		scroll = 0.0f;

//	rb_vertex = rb_index = 0;
	for (bp = fa->polys; bp; bp = bp->next)
	{
		c_brush_polys += (bp->numverts-2);
		p = bp;
		if (RB_CheckArrayOverflow (p->numverts, (p->numverts-2)*3))
			RB_RenderWarpSurface (fa);
		for (i = 0; i < p->numverts-2; i++) {
			indexArray[rb_index++] = rb_vertex;
			indexArray[rb_index++] = rb_vertex+i+1;
			indexArray[rb_index++] = rb_vertex+i+2;
		}
		for (i=0, v=p->verts[0]; i<p->numverts; i++, v+=VERTEXSIZE)
		{
		#if !id386
			s = v[3] + r_turbsin[(int32_t)((v[4]*0.125+rdt) * TURBSCALE) & 255];
			t = v[4] + r_turbsin[(int32_t)((v[3]*0.125+rdt) * TURBSCALE) & 255];
		#else
			s = v[3] + r_turbsin[Q_ftol( ((v[4]*0.125+rdt) * TURBSCALE) ) & 255];
			t = v[4] + r_turbsin[Q_ftol( ((v[3]*0.125+rdt) * TURBSCALE) ) & 255];
		#endif
			s += scroll;
			s *= DIV64;
			t *= DIV64;

			// MrG - texture shader waterwarp
			VA_SetElem2(texCoordArray[0][rb_vertex], s, t);
			VA_SetElem2(texCoordArray[1][rb_vertex], (v[3]+dstscroll)*DIV64, v[4]*DIV64);

			if (light && p->vertexlight && p->vertexlightset)
				VA_SetElem4(colorArray[rb_vertex],
					(float)(p->vertexlight[i*3+0]*DIV255),
					(float)(p->vertexlight[i*3+1]*DIV255),
					(float)(p->vertexlight[i*3+2]*DIV255), alpha);
			else
				VA_SetElem4(colorArray[rb_vertex], glState.inverse_intensity, glState.inverse_intensity, glState.inverse_intensity, alpha);

			VA_SetElem3v(vertexArray[rb_vertex], v);

			rb_vertex++;
		}
	}

	if (render)
		RB_RenderWarpSurface (fa);
}

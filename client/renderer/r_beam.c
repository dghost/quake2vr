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

// r_beam.c -- beam rendering
// moved from r_main.c

#include "include/r_local.h"


/*
=======================
R_RenderBeam
=======================
*/
void R_RenderBeam (vec3_t start, vec3_t end, float size, float red, float green, float blue, float alpha)
{
	float		len;
	vec3_t		vert[4], ang_up, ang_right, vdelta;
    static const vec2_t		texCoord[4] = {
        { 0, 1},
        { 0, 0},
        { 1, 0},
        { 1, 1}
    };
    const vec_t r = red*DIV255;
    const vec_t g = green*DIV255;
    const vec_t b = blue*DIV255;
    const vec_t a = alpha*DIV255;
    const vec4_t beamColor[4] = {
        {r, g, b, a},
        {r, g, b, a},
        {r, g, b, a},
        {r, g, b, a}
    };
    static const uint32_t indices[6] = {
        0, 1, 2, 0, 2, 3
    };
    
	c_alias_polys += 2;

	GL_TexEnv (GL_MODULATE);
	GL_DepthMask (false);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE); // this fixes the black background
	GL_Enable (GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);
	GL_Bind(glMedia.particlebeam->texnum);

	VectorSubtract(start, end, ang_up);
	len = VectorNormalize(ang_up);

	VectorSubtract(r_newrefdef.vieworg, start, vdelta);
	CrossProduct(ang_up, vdelta, ang_right);
	if (!VectorCompare(ang_right, vec3_origin))
		VectorNormalize(ang_right);

	VectorScale (ang_right, size*2, ang_right); // Knightmare- a little narrower, please

	VectorAdd(start, ang_right, vert[0]);
	VectorAdd(end, ang_right, vert[1]);
	VectorSubtract(end, ang_right, vert[2]);
	VectorSubtract(start, ang_right, vert[3]);

	rb_vertex = rb_index = 0;
    memcpy(&indexArray[0], indices, sizeof(indices));

    memcpy(texCoordArray[0], texCoord, sizeof(vec2_t) * 4);
    memcpy(vertexArray, vert, sizeof(vec3_t) * 4);
    memcpy(colorArray, beamColor, sizeof(vec4_t) * 4);

    rb_index += 6;
    rb_vertex += 4;
    
	RB_DrawArrays ();

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv (GL_MODULATE);
	GL_DepthMask (true);
	GL_Disable (GL_BLEND);

	RB_DrawMeshTris ();
	rb_vertex = rb_index = 0;
}


/*
=======================
R_DrawBeam
=======================
*/
void R_DrawBeam( entity_t *e )
{
	qboolean fog_on = glState.fog;

	// Knightmare- no fog on lasers
	if (fog_on) // check if fog is enabled
		GL_Disable(GL_FOG); // if so, disable it

	R_RenderBeam( e->origin, e->oldorigin, e->frame, 
		( d_8to24table[e->skinnum & 0xFF] ) & 0xFF,
		( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF,
		( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF,
		e->alpha*255 );

	// re-enable fog if it was on
	if (fog_on)
		GL_Enable(GL_FOG);
}

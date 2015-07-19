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

// r_alias_misc.c: shared model rendering functions

#include "include/r_local.h"
#include "../vr/include/vr.h"

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "include/anorms.h"
};

// precalculated dot products for quantized angles
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "include/anormtab.h"
;

float		*shadedots = r_avertexnormal_dots[0];

vec3_t		shadelight;

m_dlight_t	model_dlights[MAX_MODEL_DLIGHTS];
int32_t			model_dlights_num;

float		shellFlowH, shellFlowV;

/*
=================
mirrorValue
=================
*/
float  mirrorValue (float value, qboolean mirrormodel)
{
	if (mirrormodel)
	{
		if (value>1)
			return 0;
		else if (value<0)
			return 1;
		else
			return 1-value;
	}
	return value;
}


#if 0
/*
=================
R_CalcEntAlpha
=================
*/
float R_CalcEntAlpha (float alpha, vec3_t point)
{
	float	baseDist, newAlpha;
	vec3_t	vert_len;

	newAlpha = alpha;

	if (!(currententity->renderfx & RF2_CAMERAMODEL) || !(currententity->flags & RF_TRANSLUCENT))
	{
		newAlpha = max(min(newAlpha, 1.0f), 0.0f);
		return newAlpha;
	}

	baseDist = Cvar_VariableValue("cg_thirdperson_dist");
	if (baseDist < 1)	baseDist = 50;
	VectorSubtract(r_newrefdef.vieworg, point, vert_len);
	newAlpha *= VectorLength(vert_len) / baseDist;
	if (newAlpha > alpha)	newAlpha = alpha;

	newAlpha = max(min(newAlpha, 1.0f), 0.0f);

	return newAlpha;
}
#endif


/*
=================
R_CalcShadowAlpha
=================
*/
#define SHADOW_FADE_DIST 128
float R_CalcShadowAlpha (entity_t *e)
{
	vec3_t	vec;
	float	dist, minRange, maxRange, outAlpha;

	VectorSubtract(e->origin, r_origin, vec);
	dist = VectorLength(vec);

	if (r_newrefdef.fov_y >= 90)
		minRange = r_shadowrange->value;
	else // reduced FOV means longer range
		minRange = r_shadowrange->value * (90/r_newrefdef.fov_y); // this can't be zero
	maxRange = minRange + SHADOW_FADE_DIST;

	if (dist <= minRange) // in range
		outAlpha = r_shadowalpha->value;
	else if (dist >= maxRange) // out of range
		outAlpha = 0.0f;
	else // fade based on distance
		outAlpha = r_shadowalpha->value * (fabs(dist-maxRange)/SHADOW_FADE_DIST);

	return outAlpha;
}


/*
==============
R_ShadowBlend
Draws projection shadow(s)
from stenciled volume
==============
*/
void R_ShadowBlend (float shadowalpha)
{
    vec4_t color = {0, 0, 0, shadowalpha};
	if (r_shadows->value != 3)
		return;

	GL_PushMatrix(GL_MODELVIEW);

	GL_LoadMatrix(GL_MODELVIEW, glState.axisRotation);

	GL_Disable (GL_ALPHA_TEST);
	GL_Enable (GL_BLEND);
	GL_Disable (GL_DEPTH_TEST);
	GL_DisableTexture(0);

    GL_StencilFunc(GL_NOTEQUAL, 0, 255);
    glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
	GL_Enable(GL_STENCIL_TEST);

	rb_vertex = rb_index = 0;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+1;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+0;
	indexArray[rb_index++] = rb_vertex+2;
	indexArray[rb_index++] = rb_vertex+3;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, 100);
	VA_SetElem4v(colorArray[rb_vertex], color);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, 100);
	VA_SetElem4v(colorArray[rb_vertex], color);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, -100, -100);
	VA_SetElem4v(colorArray[rb_vertex], color);
	rb_vertex++;
	VA_SetElem3(vertexArray[rb_vertex], 10, 100, -100);
	VA_SetElem4v(colorArray[rb_vertex], color);
	rb_vertex++;
	RB_RenderMeshGeneric (false);

	GL_PopMatrix(GL_MODELVIEW);

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_Disable (GL_BLEND);
	GL_EnableTexture(0);
	GL_Enable (GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);
	//GL_Enable (GL_ALPHA_TEST);

	glColor4f(1,1,1,1);
}


/*
=================
capColorVec
=================
*/
void capColorVec (vec3_t color)
{
	int32_t i;

	for (i=0;i<3;i++)
		color[i] = max(min(color[i], 1.0f), 0.0f);
}

/*
=================
R_SetVertexRGBScale
=================
*/
void R_SetVertexRGBScale (qboolean toggle)
{
	if (!r_rgbscale->value)
		return;

	if (toggle) // turn on
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, r_rgbscale->value);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		
		GL_TexEnv(GL_COMBINE);

	}
	else // turn off
	{
		GL_TexEnv(GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);
	}
}


/*
=================
EnvMapShell
=================
*/
qboolean EnvMapShell (void)
{
	return ( (r_shelltype->value == 2)
		|| (r_shelltype->value == 1 && currententity->alpha == 1.0f) );
}


/*
=================
FlowingShell
=================
*/
qboolean FlowingShell (void)
{
	return (r_shelltype->value == 1 && currententity->alpha != 1.0f);
}


/*
=================
R_SetShellBlend
=================
*/
void R_SetShellBlend (qboolean toggle)
{
	// shells only
	if ( !(currententity->flags & RF_MASK_SHELL) )
		return;

	if (toggle) //turn on
	{
		// Psychospaz's envmapping
		if (EnvMapShell())
		{
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

			GL_Bind(glMedia.spheremappic->texnum);

			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
		}
		else if (FlowingShell())
			GL_Bind(glMedia.shelltexture->texnum);
		else
			GL_DisableTexture(0);
        
        GL_ClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        GL_StencilFunc(GL_EQUAL, 1, 2);
        glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
        GL_Enable(GL_STENCIL_TEST);
        
		shellFlowH =  0.25 * sin(r_newrefdef.time * 0.5 * M_PI);
		shellFlowV = -(r_newrefdef.time / 2.0); 
	}
	else // turn off
	{
		// Psychospaz's envmapping
		if (EnvMapShell())
		{
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
		}
		else if (FlowingShell())
		{ /*nothing*/ }
		else
			GL_EnableTexture(0);

        GL_Disable(GL_STENCIL_TEST);
	}
}

/*
=================
R_FlipModel
=================
*/
void R_FlipModel (qboolean on, qboolean cullOnly)
{

// TODO - optimize

	if (on)
	{
		if (!cullOnly)
			glScalef(1,-1,1);
		GL_CullFace( GL_BACK );

	}
	else
	{
		if (!cullOnly)
			glScalef(1,-1,1);
		GL_CullFace( GL_FRONT );

	}
}

/*
=================
R_SetBlendModeOn
=================
*/
void R_SetBlendModeOn (image_t *skin)
{
	GL_TexEnv( GL_MODULATE );

	if (skin)
		GL_Bind(skin->texnum);

	GL_ShadeModel (GL_SMOOTH);

	if (currententity->flags & RF_TRANSLUCENT)
	{
		GL_DepthMask (false);

		if (currententity->flags & RF_TRANS_ADDITIVE)
		{ 
			GL_BlendFunc   (GL_SRC_ALPHA, GL_ONE);	
			glColor4ub(255, 255, 255, 255);
			GL_ShadeModel (GL_FLAT);
		}
		else
			GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		GL_Enable (GL_BLEND);
	}
}

/*
=================
R_SetBlendModeOff
=================
*/
void R_SetBlendModeOff (void)
{
	if ( currententity->flags & RF_TRANSLUCENT )
	{
		GL_DepthMask (true);
		GL_Disable(GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

/*
=================
R_SetShadeLight
=================
*/
void R_SetShadeLight (void)
{
	int32_t		i;

	if (currententity->flags & RF_MASK_SHELL)
	{
		VectorClear (shadelight);
		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
			shadelight[0] = 0.56;
			shadelight[1] = 0.59;
			shadelight[2] = 0.45;
		}
		if ( currententity->flags & RF_SHELL_DOUBLE )
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ( currententity->flags & RF_SHELL_RED )
			shadelight[0] = 1.0;
		if ( currententity->flags & RF_SHELL_GREEN )
			shadelight[1] = 1.0;
		if ( currententity->flags & RF_SHELL_BLUE )
			shadelight[2] = 1.0;
	}
	else if ( currententity->flags & RF_FULLBRIGHT )
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		// Set up basic lighting...
		if (r_model_shading->value && r_model_dlights->value)
		{
			int32_t max = r_model_dlights->value;

			if (max<0) max=0;
			if (max>MAX_MODEL_DLIGHTS) max=MAX_MODEL_DLIGHTS;

			R_LightPointDynamics (currententity->origin, shadelight, model_dlights, 
				&model_dlights_num, max);
		}
		else
		{
			R_LightPoint (currententity->origin, shadelight, false);//true);
			model_dlights_num = 0;
		}

		// player lighting hack for communication back to server
		// big hack!
		if ( currententity->flags & RF_WEAPONMODEL )
		{
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
	
	}

	if ( currententity->flags & RF_MINLIGHT )
	{
		for (i=0; i<3; i++)
			if (shadelight[i] > 0.02)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.02;
			shadelight[1] = 0.02;
			shadelight[2] = 0.02;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.2 * sin(r_newrefdef.time*7);
		for (i=0 ; i<3 ; i++)
		{
			min = shadelight[i] * 0.8;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

	if (r_newrefdef.rdflags & RDF_IRGOGGLES)
	{
		if (currententity->flags & RF_IR_VISIBLE)
		{
			shadelight[0] = 1.0;
			shadelight[1] = 0.0;
			shadelight[2] = 0.0;
		}
	}
/*	if (r_newrefdef.rdflags & RDF_UVGOGGLES)
	{
		if (currententity->flags & RF_IR_VISIBLE)
		{
			shadelight[0] = 0.5;
			shadelight[1] = 1.0;
			shadelight[2] = 0.5;
		}
		else
		{
			shadelight[0] = 0.0;
			shadelight[1] = 1.0;
			shadelight[2] = 0.0;
		}
	}*/

	shadedots = r_avertexnormal_dots[((int32_t)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
}

/*
=================
R_DrawAliasModelBBox
=================
*/
void R_DrawAliasModelBBox (vec3_t bbox[8], entity_t *e)
{
	if (!r_showbbox->value)
		return;

	if (e->flags & RF_WEAPONMODEL || e->flags & RF_VIEWERMODEL || e->flags & RF_BEAM || e->renderfx & RF2_CAMERAMODEL)
		return;

	GL_Disable (GL_CULL_FACE);
	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	GL_DisableTexture (0);

	// Draw top and sides
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3fv( bbox[2] );
	glVertex3fv( bbox[1] );
	glVertex3fv( bbox[0] );
	glVertex3fv( bbox[1] );
	glVertex3fv( bbox[4] );
	glVertex3fv( bbox[5] );
	glVertex3fv( bbox[1] );
	glVertex3fv( bbox[7] );
	glVertex3fv( bbox[3] );
	glVertex3fv( bbox[2] );
	glVertex3fv( bbox[7] );
	glVertex3fv( bbox[6] );
	glVertex3fv( bbox[2] );
	glVertex3fv( bbox[4] );
	glVertex3fv( bbox[0] );
	glEnd();

  	// Draw bottom
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3fv( bbox[4] );
	glVertex3fv( bbox[6] );
	glVertex3fv( bbox[7] );
	glEnd();

	GL_EnableTexture (0);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	GL_Enable (GL_CULL_FACE);
}

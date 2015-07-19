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

// cl_effects.c -- particle and decal effects parsing/generation

#include "client.h"
#include "particles.h"

static vec3_t avelocities [NUMVERTEXNORMALS];

extern	uint32_t	d_8to24table[256];

//=================================================

/*
===============
CL_LightningBeam
===============
*/
/*void CL_LightningBeam(vec3_t start, vec3_t end, int32_t srcEnt, int32_t dstEnt, float size)
{
	cparticle_t *list;
	cparticle_t *p=NULL;
	
	for (list=active_particles; list; list=list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt )
		{
			p=list;
			p->time = cl.time;
			VectorCopy(start, p->angle);
			VectorCopy(end, p->org);

			if (p->link)
			{
				p->link->time = cl.time;
				VectorCopy(start, p->link->angle);
				VectorCopy(end, p->link->org);
			}
			else
			{
				p->link = CL_SetupParticle (
					start[0],	start[1],	start[2],
					end[0],		end[1],		end[2],
					0,	0,	0,
					0,		0,		0,
					150,	150,	255,
					0,	0,	0,
					1,		-1.0,
					size,		0,			
					particle_beam,
					PART_BEAM,
					NULL,0);
				
			}
			break;
		}

	if (p)
		return;

	p = CL_SetupParticle (
		start[0],	start[1],	start[2],
		end[0],		end[1],		end[2],
		0,	0,	0,
		0,		0,		0,
		115,	115,	255,
		0,	0,	0,
		1,		-1.0,
		size,		0,			
		particle_lightning,
		PART_LIGHTNING,
		NULL,0);

	p->src_ent=srcEnt;
	p->dst_ent=dstEnt;

	p->link = CL_SetupParticle (
		start[0],	start[1],	start[2],
		end[0],		end[1],		end[2],
		0,	0,	0,
		0,		0,		0,
		150,	150,	255,
		0,	0,	0,
		1,		-1.0,
		size,		0,			
		particle_beam,
		PART_BEAM,
		NULL,0);
}*/
void CL_LightningBeam (vec3_t start, vec3_t end, int32_t srcEnt, int32_t dstEnt, float size)
{
	cparticle_t *list;
	cparticle_t *p=NULL;

	for (list=active_particles; list; list=list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt && list->image == particle_lightning)
		{
			p=list;
			/*p->start =*/ p->time = cl.time;
			VectorCopy(start, p->angle);
			VectorCopy(end, p->org);

			return;
		}

	p = CL_SetupParticle (
		start[0],	start[1],	start[2],
		end[0],		end[1],		end[2],
		0,	0,	0,
		0,		0,		0,
		255,	255,	255,
		0,	0,	0,
		1,		-2,
		GL_SRC_ALPHA, GL_ONE,
		size,		0,		
		particle_lightning,
		PART_LIGHTNING,
		0, false);

	if (!p)
		return;

	p->src_ent=srcEnt;
	p->dst_ent=dstEnt;
}


static const vec3_t angles[6] = {
    { 1, 0, 0},
    { -1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1}
};

/*
===============
CL_Explosion_Decal
===============
*/
void CL_Explosion_Decal (vec3_t org, float size, int32_t decalnum)
{
	if (r_decals->value)
	{
		int32_t			i, j, offset=8;	//size/2
		cparticle_t	*p;
		vec3_t		ang;
		trace_t		trace1, trace2;
		vec3_t		end1, end2, normal, sorg, dorg;
		vec3_t		planenormals[6];



		for (i=0; i<6; i++)
		{
			VectorMA(org, -offset, angles[i], sorg); // move origin 8 units back
			VectorMA(sorg, size/2+offset, angles[i], end1);
            trace1 = CL_Trace (sorg, end1, 0, CONTENTS_SOLID);
            if (trace1.fraction < 1) // hit a surface
            {	// make sure we haven't hit this plane before
                qboolean done = 0;
                VectorCopy(trace1.plane.normal, planenormals[i]);
                for (j=0; j<i && !done; j++)
                    done |= (VectorCompare(planenormals[j],planenormals[i]));
                
                if (done) {
                    continue;
                }
                
                // try tracing directly to hit plane
				VectorNegate(trace1.plane.normal, normal);
				VectorMA(sorg, size/2, normal, end2);
				trace2 = CL_Trace (sorg, end2, 0, CONTENTS_SOLID);
				// if seond trace hit same plane
				if (trace2.fraction < 1 && VectorCompare(trace2.plane.normal, trace1.plane.normal))
					VectorCopy(trace2.endpos, dorg);
				else
					VectorCopy(trace1.endpos, dorg);
				//if (CM_PointContents(dorg,0) & MASK_WATER) // no scorch marks underwater
				//	continue;
				VecToAngleRolled(normal, rand()%360, ang);
				p = CL_SetupParticle (
					ang[0],	ang[1],	ang[2],
					dorg[0],dorg[1],dorg[2],
					0,		0,		0,
					0,		0,		0,
					255,	255,	255,
					0,		0,		0,
					1,		-1/r_decal_life->value,
					GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
					size,		0,			
					decalnum, // particle_burnmark
					PART_SHADED|PART_DECAL|PART_ALPHACOLOR,
					CL_DecalAlphaThink, true);
			}
			/*VecToAngleRolled(angle[i], rand()%360, ang);
			p = CL_SetupParticle (
				ang[0],	ang[1],	ang[2],
				org[0],	org[1],	org[2],
				0,		0,		0,
				0,		0,		0,
				255,	255,	255,
				0,		0,		0,
				1,		-0.001,
				GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
				size,		0,			
				particle_burnmark,
				PART_SHADED|PART_DECAL|PART_ALPHACOLOR,
				CL_DecalAlphaThink, true);*/
		}
	}
}

/*
===============
CL_ExplosionBubbleThink
===============
*/
void CL_ExplosionBubbleThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{

	if (CM_PointContents(org,0) & MASK_WATER)
		p->thinknext = true;
	else
	{
		p->think = NULL;
		p->alpha = 0;
	}
}

/*
===============
CL_ParticleExplosionSparksThink
===============
*/
void CL_ParticleExplosionSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	int32_t i;

	//setting up angle for sparks
	{
		float time1, time2;

		time1 = *time;
		time2 = time1*time1;

		for (i=0;i<2;i++)
			angle[i] = 0.25*(p->vel[i]*time1 + (p->accel[i])*time2);
		angle[2] = 0.25*(p->vel[2]*time1 + (p->accel[2]-PARTICLE_GRAVITY_DEFAULT)*time2);
	}

	p->thinknext = true;
}

/*
===============
CL_Explosion_Sparks
===============
*/
void CL_Explosion_Sparks (vec3_t org, int32_t size, int32_t count)
{
	int32_t	i;

	for (i=0; i < (count/cl_particle_scale->value); i++) // was 256
	{
		CL_SetupParticle (
			0,	0,	0,
			org[0] + ((rand()%size)-16),	org[1] + ((rand()%size)-16),	org[2] + ((rand()%size)-16),
			(rand()%150)-75,	(rand()%150)-75,	(rand()%150)-75,
			0,		0,		0,
			255,	100,	25,
			0,	0,	0,
			1,		-0.8 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			size,		size*-1.5f,		// was 6, -9
			particle_solid,
			PART_GRAVITY_LIGHT|PART_SPARK,
			CL_ParticleExplosionSparksThink, true);
	}
}


/*
=====================

Blood effects

=====================
*/
void CL_ParticleBloodThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time);
void CL_BloodPuff (vec3_t org, vec3_t dir, int32_t count);

#define MAXBLEEDSIZE 5
#define TIMEBLOODGROW 2.5f
#define BLOOD_DECAL_CHANCE 0.5F

/*
===============
CL_ParticleBloodDecalThink
===============
*/
void CL_ParticleBloodDecalThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{	// This REALLY slows things down
	/*if (*time<TIMEBLOODGROW)
	{
		vec3_t dir;

		*size *= sqrt(0.5 + 0.5*(*time/TIMEBLOODGROW));

		AngleVectors (angle, dir, NULL, NULL);
		VectorNegate(dir, dir);
		CL_ClipDecal(p, *size, angle[2], org, dir);
	}*/

	//now calc alpha
	CL_DecalAlphaThink (p, org, angle, alpha, size, image, time);
}

/*
===============
CL_ParticleBloodDropThink
===============
*/
void CL_ParticleBloodDropThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	float length;
	vec3_t len;

	VectorSubtract(p->angle, org, len);
	{
		CL_CalcPartVelocity(p, 0.2, time, angle);

		length = VectorNormalize(angle);
		if (length>MAXBLEEDSIZE) length = MAXBLEEDSIZE;
		VectorScale(angle, -length, angle);
	}

	//now to trace for impact...
	CL_ParticleBloodThink (p, org, angle, alpha, size, image, time);
}

/*
===============
CL_ParticleBloodPuffThink
===============
*/
void CL_ParticleBloodPuffThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	angle[2] =	angle[0] + *time*angle[1] + *time**time*angle[2];

	//now to trace for impact...
	CL_ParticleBloodThink (p, org, angle, alpha, size, image, time);
}

/*
===============
CL_ParticleBloodThink
===============
*/
void CL_ParticleBloodThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	trace_t	trace = CL_Trace (p->oldorg, org, 0, CONTENTS_SOLID); // was 0.1
	qboolean became_decal = false;

	if (trace.fraction < 1.0) // delete and stain...
	{
		if (r_decals->value && (p->flags & PART_LEAVEMARK)
			&& !VectorCompare(trace.plane.normal, vec3_origin)
			&& !(CM_PointContents(p->oldorg,0) & MASK_WATER)) // no blood splatters underwater...
		{
			vec3_t	normal, dir;
			int32_t		i;
			qboolean greenblood = false;
			qboolean timedout = false;
			if (p->color[1] > 0 && p->color[2] > 0)
				greenblood = true;
			// time cutoff for gib trails
			if (p->flags & PART_GRAVITY_LIGHT && !(p->flags & PART_DIRECTION))
			{	// gekk gibs go flyin faster...
				if ((greenblood) && (cl.time - p->time)*0.001 > 1.0F)
					timedout = true;
				if ((!greenblood) && (cl.time - p->time)*0.001 > 0.5F)
					timedout = true;
			}

			if (!timedout)
			{
				VectorNegate(trace.plane.normal, normal);
				VecToAngleRolled(normal, rand()%360, p->angle);
				
				VectorCopy(trace.endpos, p->org);
				VectorClear(p->vel);
				VectorClear(p->accel);
				p->image = CL_GetRandomBloodParticle();
				p->blendfunc_src = GL_SRC_ALPHA; //GL_ZERO
				p->blendfunc_dst = GL_ONE_MINUS_SRC_ALPHA; //GL_ONE_MINUS_SRC_COLOR
				p->flags = PART_DECAL|PART_SHADED|PART_ALPHACOLOR;
				p->alpha = *alpha;
				p->alphavel = -1/r_decal_life->value;
				if (greenblood)
					p->color[1] = 210;
				else
					for (i=0; i<3; i++)
						p->color[i] *= 0.5;
				p->start = CL_NewParticleTime();
				p->think = CL_ParticleBloodDecalThink;
				p->thinknext = true;
				p->size = MAXBLEEDSIZE*0.5*(random()*5.0+5);
				//p->size = *size*(random()*5.0+5);
				p->sizevel = 0;
				
				p->decalnum = 0;
				p->decal = NULL;
				AngleVectors (p->angle, dir, NULL, NULL);
				VectorNegate(dir, dir);
				CL_ClipDecal(p, p->size, -p->angle[2], p->org, dir);
				if (p->decalnum)
					became_decal = true;
				//else
				//	Com_Printf(S_COLOR_YELLOW"Blood decal not clipped!\n");
			}
		}
		if (!became_decal)
		{
			*alpha = 0;
			*size = 0;
			p->alpha = 0;
		}
	}
	VectorCopy(org, p->oldorg);

	p->thinknext = true;
}


/*
===============
CL_BloodSmack
===============
*/
void CL_BloodSmack (vec3_t org, vec3_t dir)
{
	cparticle_t *p;

	p = CL_SetupParticle (
		crand()*180, crand()*100, 0,
		org[0],	org[1],	org[2],
		dir[0],	dir[1],	dir[2],
		0,		0,		0,
		255,	0,		0,
		0,		0,		0,
		1.0,		-1 / (0.5 + frand()*0.3), //was -0.75
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		10,			0,			
		particle_redblood,
		PART_SHADED|PART_OVERBRIGHT,
		CL_ParticleRotateThink,true);

	CL_BloodPuff(org, dir, 1);
}


/*
===============
CL_BloodBleed
===============
*/
void CL_BloodBleed (vec3_t org, vec3_t dir, int32_t count)
{
	cparticle_t *p;
	vec3_t	pos;
	int32_t		i;

	VectorScale(dir, 10, pos);
	for (i=0; i<count; i++)
	{
		VectorSet(pos,
			dir[0]+random()*(cl_blood->value-2)*0.01,
			dir[1]+random()*(cl_blood->value-2)*0.01,
			dir[2]+random()*(cl_blood->value-2)*0.01);
		VectorScale(pos, 10 + (cl_blood->value-2)*0.0001*random(), pos);

		p = CL_SetupParticle (
			org[0], org[1], org[2],
			org[0] + ((rand()&7)-4) + dir[0],	org[1] + ((rand()&7)-4) + dir[1],	org[2] + ((rand()&7)-4) + dir[2],
			pos[0]*(random()*3+5),	pos[1]*(random()*3+5),	pos[2]*(random()*3+5),
			0,		0,		0,
			255,	0,		0,
			0,		0,		0,
			0.7,		-0.25 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			MAXBLEEDSIZE*0.5,		0,			
			particle_blooddrip,
			PART_SHADED|PART_DIRECTION|PART_GRAVITY_LIGHT|PART_OVERBRIGHT,
			CL_ParticleBloodDropThink,true);

		if (p && i == 0 && random() < BLOOD_DECAL_CHANCE)
			p->flags |= PART_LEAVEMARK;
	}

}

/*
===============
CL_BloodPuff
===============
*/
void CL_BloodPuff (vec3_t org, vec3_t dir, int32_t count)
{
	cparticle_t *p;
	int32_t		i;
	float	d;

	for (i=0; i<count; i++)
	{
		d = rand()&31;
		p = CL_SetupParticle (
			crand()*180, crand()*100, 0,
			org[0] + ((rand()&7)-4) + d*dir[0],	org[1] + ((rand()&7)-4) + d*dir[1],	org[2] + ((rand()&7)-4) + d*dir[2],
			dir[0]*(crand()*3+5),	dir[1]*(crand()*3+5),	dir[2]*(crand()*3+5),
			0,			0,			-100,
			255,		0,			0,
			0,			0,			0,
			1.0,		-1.0,
			GL_SRC_ALPHA, GL_ONE,
			10,			0,
			particle_blood,
			PART_SHADED,
			CL_ParticleBloodPuffThink,true);

		if (p && i == 0 && random() < BLOOD_DECAL_CHANCE)
			p->flags |= PART_LEAVEMARK;
	}
}

/*
===============
CL_BloodHit
===============
*/
void CL_BloodHit (vec3_t org, vec3_t dir)
{
	if (cl_blood->value < 1) // disable blood option
		return;
	if (cl_blood->value == 2) // splat
		CL_BloodSmack(org, dir);
	else if (cl_blood->value == 3) // bleed
		CL_BloodBleed (org, dir, 6);
	else if (cl_blood->value == 4) // gore
		CL_BloodBleed (org, dir, 16);
	else // 1 = puff
		CL_BloodPuff(org, dir, 5);
}

/*
==================
CL_GreenBloodHit

green blood spray
==================
*/
void CL_GreenBloodHit (vec3_t org, vec3_t dir)
{
	cparticle_t *p;
	int32_t		i;
	float	d;

	if (cl_blood->value < 1) // disable blood option
		return;

	for (i=0;i<5;i++)
	{
		d = rand()&31;
		p = CL_SetupParticle (
				crand()*180, crand()*100, 0,
				org[0] + ((rand()&7)-4) + d*dir[0],	org[1] + ((rand()&7)-4) + d*dir[1],	org[2] + ((rand()&7)-4) + d*dir[2],
				dir[0]*(crand()*3+5),	dir[1]*(crand()*3+5),	dir[2]*(crand()*3+5),
				0,		0,		-100,
				255,	180,	50,
				0,		0,		0,
				1,		-1.0,
				GL_SRC_ALPHA, GL_ONE,
				10,			0,			
				particle_blood,
				PART_SHADED|PART_OVERBRIGHT,
				CL_ParticleBloodPuffThink,true);

		if (p && i == 0 && random() < BLOOD_DECAL_CHANCE)
			p->flags |= PART_LEAVEMARK;
	}

}

/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
void CL_ParticleEffect (vec3_t org, vec3_t dir, int32_t color8, int32_t count)
{
	int32_t			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&31;
		CL_SetupParticle (
			0,		0,		0,
			org[0] + ((rand()&7)-4) + d*dir[0],	org[1] + ((rand()&7)-4) + d*dir[1],	org[2] + ((rand()&7)-4) + d*dir[2],
			crand()*20,			crand()*20,			crand()*20,
			0,		0,		0,
			color[0],		color[1],		color[2],
			0,	0,	0,
			1.0,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL,0);
	}
}

/*
===============
CL_ParticleEffect2
===============
*/

#define colorAdd 25
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int32_t color8, int32_t count)
{
	int32_t			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;
		CL_SetupParticle (
			0,	0,	0,
			org[0]+((rand()&7)-4)+d*dir[0],	org[1]+((rand()&7)-4)+d*dir[1],	org[2]+((rand()&7)-4)+d*dir[2],
			crand()*20,			crand()*20,			crand()*20,
			0,		0,		0,
			color[0] + colorAdd,		color[1] + colorAdd,		color[2] + colorAdd,
			0,	0,	0,
			1,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL,0);
	}
}

// RAFAEL
/*
===============
CL_ParticleEffect3
===============
*/

void CL_ParticleEffect3 (vec3_t org, vec3_t dir, int32_t color8, int32_t count)
{
	int32_t			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;
		CL_SetupParticle (
			0,	0,	0,
			org[0]+((rand()&7)-4)+d*dir[0],	org[1]+((rand()&7)-4)+d*dir[1],	org[2]+((rand()&7)-4)+d*dir[2],
			crand()*20,			crand()*20,			crand()*20,
			0,		0,		0,
			color[0] + colorAdd,		color[1] + colorAdd,		color[2] + colorAdd,
			0,	0,	0,
			1,		-0.25 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			2,			-0.25,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL, false);
	}
}


/*
===============
CL_ParticleSplashThink
===============
*/
#define SplashSize 7.5
void CL_ParticleSplashThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	int32_t i;
	vec3_t len;
	VectorSubtract(p->angle, org, len);
	
//	*size *= (float)(SplashSize/VectorLength(len)) * 0.5/((4-*size));
//	if (*size > SplashSize)
//		*size = SplashSize;

	//setting up angle for sparks
	{
		float time1, time2;

		time1 = *time;
		time2 = time1*time1;

		for (i=0;i<2;i++)
			angle[i] = 0.5*(p->vel[i]*time1 + (p->accel[i])*time2);
		angle[2] = 0.5*(p->vel[2]*time1 + (p->accel[2]-PARTICLE_GRAVITY_DEFAULT)*time2);
	}

	p->thinknext = true;
}

/*
===============
CL_ParticleEffectSplash

Water Splashing
===============
*/
void CL_ParticleEffectSplash (vec3_t org, vec3_t dir, int32_t color8, int32_t count)
{
	int32_t			i;
	float		d;
	vec3_t color = {color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&5;
		CL_SetupParticle (
			org[0],	org[1],	org[2],
			org[0]+d*dir[0],	org[1]+d*dir[1],	org[2]+d*dir[2],
			dir[0]*40 + crand()*10,	dir[1]*40 + crand()*10,	dir[2]*40 + crand()*10,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1,		-0.75 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			5,			-7,			
			particle_smoke,
			PART_GRAVITY_LIGHT|PART_DIRECTION   /*|PART_TRANS|PART_SHADED*/,
			CL_ParticleSplashThink,true);
	}
}
void CL_ParticleBlasterThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time);

void CL_ParticleEffectSplashSpark (vec3_t org, vec3_t dir, int32_t color8, int32_t count)
{
	int32_t			i;
	float		d;
	float speed = 0.75;
	vec3_t color = {color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&5;
		CL_SetupParticle (
			org[0],	org[1],	org[2],
			org[0]+d*dir[0],	org[1]+d*dir[1],	org[2]+d*dir[2],
			(dir[0]*75 + crand()*60)*speed,	(dir[1]*75 + crand()*60)*speed,	(dir[2]*75 + crand()*60)*speed,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1,		-0.75 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			2,	2*-0.125,		// was 4, -0.5
			particle_generic,
			PART_GRAVITY_HEAVY,
			CL_ParticleBlasterThink,true);
	}
}

	
/*
===============
CL_ParticleSparksThink
===============
*/
void CL_ParticleSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	//vec3_t dir;
	int32_t i;

	//setting up angle for sparks
	{
		float time1, time2;

		time1 = *time;
		time2 = time1*time1;

		for (i=0;i<2;i++)
			angle[i] = 0.25*(p->vel[i]*time1 + (p->accel[i])*time2);
		angle[2] = 0.25*(p->vel[2]*time1 + (p->accel[2]-PARTICLE_GRAVITY_DEFAULT)*time2);
	}

	p->thinknext = true;
}

/*
===============
CL_ParticleEffectSparks
===============
*/
void CL_ParticleEffectSparks (vec3_t org, vec3_t dir, vec3_t color, int32_t count)
{
	int32_t			i;
	float		d;
	cparticle_t *p = NULL;

	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;


		p = CL_SetupParticle (
			0,	0,	0,
			org[0]+((rand()&3)-2),	org[1]+((rand()&3)-2),	org[2]+((rand()&3)-2),
			crand()*20 + dir[0]*40,			crand()*20 + dir[1]*40,			crand()*20 + dir[2]*40,
			0,		0,		0,
			color[0],		color[1],		color[2],
			0,	0,	0,
			0.75,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			4,			0, //Knightmare- increase size
			particle_solid,
			PART_GRAVITY_LIGHT|PART_SPARK,
			CL_ParticleSparksThink,true);

	}
	if (p) // added light effect
		CL_AddParticleLight (p, (count>8)?130:65, 0, color[0]/255, color[1]/255, color[2]/255);
}


/*
===============
CL_ParticleBulletDecal
===============
*/
#define DECAL_OFFSET 0.5f
void CL_ParticleBulletDecal (vec3_t org, vec3_t dir, float size)
{
	cparticle_t	*p;
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;

	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);
	//tr = CL_Trace (origin, end, 1, 1);

	if (tr.fraction == 1)
	//if (!tr.allsolid)
		return;

	VectorNegate(tr.plane.normal, angle);
	//VectorNegate(dir, angle);
	VecToAngleRolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	p = CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		255,	255,	255,
		0,		0,		0,
		1,		-1/r_decal_life->value,
		GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		size,			0,			
		particle_bulletmark,
		PART_SHADED|PART_DECAL|PART_ALPHACOLOR, // was part_saturate
		CL_DecalAlphaThink, true);
}


/*
===============
CL_ParticleRailDecal
===============
*/
#define RAIL_DECAL_OFFSET 2.0f
void CL_ParticleRailDecal (vec3_t org, vec3_t dir, float size, int32_t color[3])
{
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;

	VectorMA(org, -RAIL_DECAL_OFFSET, dir, origin);
	VectorMA(org, 2*RAIL_DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	VecToAngleRolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		255,	255,	255,
		0,		0,		0,
		1,		-1/r_decal_life->value,
		GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		size,			0,			
		particle_bulletmark,
		PART_SHADED|PART_DECAL|PART_ALPHACOLOR,
		CL_DecalAlphaThink, true);

	CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		color[0],	color[1],	color[2],
		0,		0,		0,
		1,		-0.25,
		GL_SRC_ALPHA, GL_ONE,
		size,			0,			
		particle_generic,
		PART_DECAL,
		NULL, false);

	CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		255,	255,	255,
		0,		0,		0,
		1,		-0.25,
		GL_SRC_ALPHA, GL_ONE,
		size*0.67,		0,			
		particle_generic,
		PART_DECAL,
		NULL, false);
}


/*
===============
CL_ParticleBlasterDecal
===============
*/
void CL_ParticleBlasterDecal (vec3_t org, vec3_t dir, float size, int32_t red, int32_t green, int32_t blue)
{
	cparticle_t	*p;
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;
 
	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	VecToAngleRolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	p = CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		255,	255,	255,
		0,		0,		0,
		0.7,	-1/r_decal_life->value,
		GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		size,		0,			
		particle_shadow,
		PART_SHADED|PART_DECAL,
		NULL, false);

	p = CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		red,	green,	blue,
		0,		0,		0,
		1,		-0.3,
		GL_SRC_ALPHA, GL_ONE,
		size*0.4,	0,			
		particle_generic,
		PART_SHADED|PART_DECAL,
		NULL, false);

	p = CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		red,	green,	blue,
		0,		0,		0,
		1,		-0.6,
		GL_SRC_ALPHA, GL_ONE,
		size*0.3,	0,			
		particle_generic,
		PART_SHADED|PART_DECAL,
		NULL, false);
}


/*
===============
CL_ParticlePlasmaBeamDecal
===============
*/
void CL_ParticlePlasmaBeamDecal (vec3_t org, vec3_t dir, float size)
{
	cparticle_t	*p;
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;
 
	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	VecToAngleRolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	p = CL_SetupParticle (
		ang[0],	ang[1],	ang[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		255,	255,	255,
		0,		0,		0,
		0.85,	-1/r_decal_life->value,
		GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		size,		0,			
		particle_shadow,
		PART_SHADED|PART_DECAL,
		NULL, false);
}


/*
===============
CL_TeleporterParticles
===============
*/
void CL_TeleporterParticles (entity_state_t *ent)
{
	int32_t			i;

	for (i = 0; i < 8; i++)
	{
		CL_SetupParticle (
			0,	0,	0,
			ent->origin[0]-16+(rand()&31),	ent->origin[1]-16+(rand()&31),	ent->origin[2]-16+(rand()&31),
			crand()*14,		crand()*14,		80 + (rand()&7),
			0,		0,		0,
			230+crand()*25,	125+crand()*25,	25+crand()*25,
			0,		0,		0,
			1,		-0.5,
			GL_SRC_ALPHA, GL_ONE,
			2,		0,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL,0);
	}
}


/*
===============
CL_LogoutEffect
===============
*/
void CL_LogoutEffect (vec3_t org, int32_t type)
{
	int32_t			i;
	vec3_t	color;

	for (i=0 ; i<500 ; i++)
	{
		if (type == MZ_LOGIN)// green
		{
			color[0] = 20;
			color[1] = 200;
			color[2] = 20;
		}
		else if (type == MZ_LOGOUT)// red
		{
			color[0] = 200;
			color[1] = 20;
			color[2] = 20;
		}
		else// yellow
		{
			color[0] = 200;
			color[1] = 200;
			color[2] = 20;
		}
		
		CL_SetupParticle (
			0,	0,	0,
			org[0] - 16 + frand()*32,	org[1] - 16 + frand()*32,	org[2] - 24 + frand()*56,
			crand()*20,			crand()*20,			crand()*20,
			0,		0,		0,
			color[0],		color[1],		color[2],
			0,	0,	0,
			1,		-1.0 / (1.0 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL,0);
	}
}


/*
===============
CL_ItemRespawnParticles
===============
*/
void CL_ParticleBlasterThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time);
void CL_ItemRespawnParticles (vec3_t org)
{
	int32_t			i;

	for (i=0 ; i<64 ; i++)
	{
		CL_SetupParticle (
			0,	0,	0,
			org[0] + crand()*8,	org[1] + crand()*8,	org[2] + crand()*8,
			crand()*40,			crand()*40,			crand()*120,
			0,		0,		PARTICLE_GRAVITY_DEFAULT*0.2,
			0,		150+frand()*25,		0,
			0,	0,	0,
			1,		-1.0 / (1.0 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			PART_GRAVITY_HEAVY,
			CL_ParticleBlasterThink,true);
	}
}


/*
===============
CL_BigTeleportParticles
===============
*/
void CL_BigTeleportParticles (vec3_t org)
{
	int32_t			i, index;
	float		angle, dist;
	static int32_t colortable0[4] = {10,50,150,50};
	static int32_t colortable1[4] = {150,150,50,10};
	static int32_t colortable2[4] = {50,10,10,150};

	for (i=0; i<(1024/cl_particle_scale->value); i++) // was 4096
	{

		index = rand()&3;
		angle = M_PI*2*(rand()&1023)/1023.0;
		dist = rand()&31;
		CL_SetupParticle (
			0,	0,	0,
			org[0]+cos(angle)*dist,	org[1] + sin(angle)*dist,org[2] + 8 + (rand()%90),
			cos(angle)*(70+(rand()&63)),sin(angle)*(70+(rand()&63)),-100 + (rand()&31),
			-cos(angle)*100,	-sin(angle)*100,PARTICLE_GRAVITY_DEFAULT*4,
			colortable0[index],	colortable1[index],	colortable2[index],
			0,	0,	0,
			1,		-0.1 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			5,		0.15 / (0.5 + frand()*0.3),	 // was 2, 0.05	
			particle_generic,
			0,
			NULL,0);
	}
}


/*
===============
CL_ParticleBlasterThink

Wall impact puffs
===============
*/
#define pBlasterMaxVelocity 400
#define pBlasterMinSize 1.0
#define pBlasterMaxSize 5.0

void CL_ParticleBlasterThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	vec_t  length;
	vec3_t len;
	float clipsize = 1.0;
	VectorSubtract(p->angle, org, len);

	*size *= (float)(pBlasterMaxSize/VectorLength(len)) * 1.0/((4-*size));
	*size += *time * p->sizevel;

	if (*size > pBlasterMaxSize)
		*size = pBlasterMaxSize;
	if (*size < pBlasterMinSize)
		*size = pBlasterMinSize;


	CL_ParticleBounceThink (p, org, angle, alpha, &clipsize, image, time); // was size

	length = VectorNormalize(p->vel);
	if (length>pBlasterMaxVelocity)
		VectorScale(p->vel,	pBlasterMaxVelocity,	p->vel);
	else
		VectorScale(p->vel,	length, p->vel);

/*	vec3_t len;
	VectorSubtract(p->angle, org, len);
	
	*size *= (float)(pBlasterMaxSize/VectorLength(len)) * 1.0/((4-*size));
	if (*size > pBlasterMaxSize)
		*size = pBlasterMaxSize;
	
	p->thinknext = true;*/
}


/*
===============
CL_BlasterParticles

Wall impact puffs
===============
*/
void CL_BlasterParticles (vec3_t org, vec3_t dir, int32_t count, float size,
		int32_t red, int32_t green, int32_t blue, int32_t reddelta, int32_t greendelta, int32_t bluedelta)
{
	int32_t			i;
	float		speed = .75;
	cparticle_t *p = NULL;
	vec3_t		origin;

	for (i = 0; i < count; i++)
	{
		VectorSet(origin,
			org[0] + dir[0]*(1 + random()*3 + pBlasterMaxSize/2.0),
			org[1] + dir[1]*(1 + random()*3 + pBlasterMaxSize/2.0),
			org[2] + dir[2]*(1 + random()*3 + pBlasterMaxSize/2.0)
			);

		p = CL_SetupParticle (
			org[0],	org[1],	org[2],
			origin[0],	origin[1],	origin[2],
			(dir[0]*75 + crand()*40)*speed,	(dir[1]*75 + crand()*40)*speed,	(dir[2]*75 + crand()*40)*speed,
			0,		0,		0,
			red,		green,		blue,
			reddelta,	greendelta,	bluedelta,
			1,		-0.5 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			size,	size*-0.125,		// was 4, -0.5
			particle_generic,
			PART_GRAVITY_HEAVY,
			CL_ParticleBlasterThink,true);
	
	/*	d = rand()&5;
		p = CL_SetupParticle (
			org[0],	org[1],	org[2],
			org[0]+((rand()&5)-2)+d*dir[0],	org[1]+((rand()&5)-2)+d*dir[1],	org[2]+((rand()&5)-2)+d*dir[2],
			(dir[0]*50 + crand()*20)*speed,	(dir[1]*50 + crand()*20)*speed,	(dir[2]*50 + crand()*20)*speed,
			0,			0,			0,
			red,		green,		blue,
			reddelta,	greendelta,	bluedelta,
			1,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			4,		-1.0,
			particle_generic,
			PART_GRAVITY_LIGHT,
			CL_ParticleBlasterThink,true);*/
	}
	if (p) // added light effect
		CL_AddParticleLight (p, 150, 0, ((float)red)/255, ((float)green)/255, ((float)blue)/255);
	
}


/*
===============
CL_BlasterTrail
===============
*/
void CL_BlasterTrail (vec3_t start, vec3_t end, int32_t red, int32_t green, int32_t blue,
									int32_t reddelta, int32_t greendelta, int32_t bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			dec;

	VectorCopy (start, move);
//	VectorSubtract (end, start, vec);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec);
	VectorMA (move, -5.0f, vec, move);

	dec = 4 * cl_particle_scale->value; 
	VectorScale (vec, dec, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,			0,			0,
			move[0] + crand(),	move[1] + crand(),	move[2] + crand(),
			crand()*5,	crand()*5,	crand()*5,
			0,			0,			0,
			red,		green,		blue,
			reddelta,	greendelta,	bluedelta,
			1,			-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			3,			-7,	// was 4, -6;
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_HyperBlasterGlow

Hyperblaster particle glow effect
===============
*/
#if 1
void CL_HyperBlasterGlow (vec3_t start, vec3_t end, int32_t red, int32_t green, int32_t blue,
										int32_t reddelta, int32_t greendelta, int32_t bluedelta)
{
	vec3_t		move, vec;
	float		len, dec, size;
	int32_t			i;

	VectorCopy (start, move);
	VectorSubtract (start, end, vec);
	len = VectorNormalize (vec);
	VectorMA (move, -16.5f, vec, move);

	dec = 3.0f; // was 1, 5
	VectorScale (vec, dec, vec);

	for (i = 0; i < 12; i++) // was 18
	{
		size = 4.2f - (0.1f*(float)i);

		CL_SetupParticle (
			0,			0,			0,
			move[0] + 0.5*crand(),	move[1] + 0.5*crand(),	move[2] + 0.5*crand(),
			crand()*5,	crand()*5,	crand()*5,
			0,			0,			0,
			red,		green,		blue,
			reddelta,	greendelta,	bluedelta,
			1,			INSTANT_PARTICLE, // was -16.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			size,			0, // was 3, -36; 5, -60
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}
#else
void CL_HyperBlasterTrail (vec3_t start, vec3_t end, int32_t red, int32_t green, int32_t blue,
										int32_t reddelta, int32_t greendelta, int32_t bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			dec;
	int32_t			i;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	VectorMA (move, 0.25, vec, move); // was 0.5
	len = VectorNormalize (vec);

	dec = 5; // was 1
	VectorScale (vec, dec, vec);

	for (i = 0; i < 5; i++) // was 18
	{
		len -= dec;

		CL_SetupParticle (
			0,			0,			0,
			move[0] + 0.5*crand(),	move[1] + 0.5*crand(),	move[2] + 0.5*crand(),
			crand()*5,	crand()*5,	crand()*5,
			0,			0,			0,
			red,		green,		blue,
			reddelta,	greendelta,	bluedelta,
			1,			-16.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			5,			-60, // was 3, -36			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}
#endif

/*
===============
CL_BlasterTracer 
===============
*/
void CL_BlasterTracer (vec3_t origin, vec3_t angle, int32_t red, int32_t green, int32_t blue, float len, float size)
{
//	int32_t		i;
	vec3_t	dir;
	
	AngleVectors (angle, dir, NULL, NULL);
	VectorScale (dir, len,dir);

	//for (i=0; i<3; i++)
	CL_SetupParticle (
		dir[0],	dir[1],	dir[2],
		origin[0],	origin[1],	origin[2],
		0,		0,		0,
		0,		0,		0,
		red,	green,	blue,
		0,		0,		0,
		1,		INSTANT_PARTICLE,
		GL_ONE, GL_ONE, // was GL_SRC_ALPHA, GL_ONE
		size,		0,			
		particle_blasterblob, // was particle_generic
		PART_DIRECTION|PART_INSTANT|PART_OVERBRIGHT,
		NULL,0);
}

void CL_HyperBlasterEffect (vec3_t start, vec3_t end, vec3_t angle, int32_t red, int32_t green, int32_t blue,
										int32_t reddelta, int32_t greendelta, int32_t bluedelta, float len, float size)
{
	if (cl_particle_scale->value < 2)
	//	CL_HyperBlasterTrail (start, end, red, green, blue, reddelta, greendelta, bluedelta);
		CL_HyperBlasterGlow (start, end, red, green, blue, reddelta, greendelta, bluedelta);
	//else //if (cl_particle_scale->value >= 2)
		CL_BlasterTracer (end, angle, red, green, blue, len, size);
}


/*
===============
CL_QuadTrail
===============
*/
void CL_QuadTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0] + crand()*16,	move[1] + crand()*16,	move[2] + crand()*16,
			crand()*5,	crand()*5,	crand()*5,
			0,		0,		0,
			0,		0,		200,
			0,	0,	0,
			1,		-1.0 / (0.8+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_FlagTrail
===============
*/
void CL_FlagTrail (vec3_t start, vec3_t end, qboolean isred, qboolean isgreen)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0] + crand()*16, move[1] + crand()*16, move[2] + crand()*16,
			crand()*5,	crand()*5, crand()*5,
			0,		0,		0,
			(isred)?255:0, (isgreen)?255:0, (!isred && !isgreen)?255:0,
			0,	0,	0,
			1,		-1.0 / (0.8+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_DiminishingTrail
===============
*/
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int32_t flags)
{
	cparticle_t	*p;
	vec3_t		move;
	vec3_t		vec;
	float		len, oldlen;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = oldlen = VectorNormalize (vec);

	dec = (flags & EF_ROCKET) ? 10 : 2;
	dec *= cl_particle_scale->value;
	VectorScale (vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		if (flags & EF_ROCKET)
		{
			if (CM_PointContents(move,0) & MASK_WATER)
				CL_SetupParticle (
					0,	0,	crand()*360,
					move[0],	move[1],	move[2],
					crand()*9,	crand()*9,	crand()*9+5,
					0,		0,		0,
					255,	255,	255,
					0,	0,	0,
					0.75,		-0.2 / (1 + frand() * 0.2),
					GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					1+random()*3,	1,			
					particle_bubble,
					PART_TRANS|PART_SHADED,
					CL_ExplosionBubbleThink, true);
			else
				CL_SetupParticle (
					crand()*180, crand()*100, 0,
					move[0],	move[1],	move[2],
					crand()*5,	crand()*5,	crand()*10,
					0,		0,		5,
					255,	255,	255,
					-50,	-50,	-50,
					0.75,		-0.75,	// was 1, -0.5
					GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					5,			15,			
					particle_smoke,
					PART_TRANS|PART_SHADED,
					CL_ParticleRotateThink, true);
		}
		else
		{
			// drop less particles as it flies
			if ((rand()&1023) < old->trailcount)
			{
				if (flags & EF_GIB)
				{
					if (cl_blood->value > 1)
						p = CL_SetupParticle (
							0,	0,	random()*360,
							move[0] + crand()*orgscale,	move[1] + crand()*orgscale,	move[2] + crand()*orgscale,
							crand()*velscale,	crand()*velscale,	crand()*velscale,
							0,		0,		0,
							255,	0,		0,
							0,		0,		0,
							0.75,	-0.75 / (1+frand()*0.4),
							GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
							3 + random()*2,			0,			
							particle_blooddrop,
							PART_OVERBRIGHT|PART_GRAVITY_LIGHT|PART_SHADED,
							CL_ParticleBloodThink,true);
							//NULL,0);
					else
						p = CL_SetupParticle (
							0,	0,	0,
							move[0] + crand()*orgscale,	move[1] + crand()*orgscale,	move[2] + crand()*orgscale,
							crand()*velscale,	crand()*velscale,	crand()*velscale,
							0,		0,		0,
							255,	0,		0,
							0,		0,		0,
							1,		-1.0 / (1+frand()*0.4),
							GL_SRC_ALPHA, GL_ONE,
							5,			-1,			
							particle_blood,
							PART_GRAVITY_LIGHT|PART_SHADED,
							CL_ParticleBloodThink,true);
							//NULL,0);
					if ( p && (crand() < (double)0.0001F) )
						p->flags |= PART_LEAVEMARK;
				}
				else if (flags & EF_GREENGIB)
				{
					p = CL_SetupParticle (
						0,	0,	0,
						move[0] + crand()*orgscale,	move[1] + crand()*orgscale,	move[2] + crand()*orgscale,
						crand()*velscale,	crand()*velscale,	crand()*velscale,
						0,		0,		0,
						255,	180,	50,
						0,		0,		0,
						1,		-0.5 / (1+frand()*0.4),
						GL_SRC_ALPHA, GL_ONE,
						5,			-1,			
						particle_blood,
						PART_OVERBRIGHT|PART_GRAVITY_LIGHT|PART_SHADED,
						CL_ParticleBloodThink,true);
						//NULL,0);
					if ( p && (crand() < (double)0.0001F) ) 
						p->flags |= PART_LEAVEMARK;

				}
				else if (flags & EF_GRENADE) // no overbrights on grenade trails
				{
					if (CM_PointContents(move,0) & MASK_WATER)
						CL_SetupParticle (
							0,	0,	crand()*360,
							move[0],	move[1],	move[2],
							crand()*9,	crand()*9,	crand()*9+5,
							0,		0,		0,
							255,	255,	255,
							0,	0,	0,
							0.75,		-0.2 / (1 + frand() * 0.2),
							GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
							1+random()*3,	1,			
							particle_bubble,
							PART_TRANS|PART_SHADED,
							CL_ExplosionBubbleThink,true);
					else
						CL_SetupParticle (
							crand()*180, crand()*50, 0,
							move[0] + crand()*orgscale,	move[1] + crand()*orgscale,	move[2] + crand()*orgscale,
							crand()*velscale,	crand()*velscale,	crand()*velscale,
							0,		0,		20,
							255,	255,	255,
							0,		0,		0,
							0.5,		-0.5,
							GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
							5,			5,			
							particle_smoke,
							PART_TRANS|PART_SHADED,
							CL_ParticleRotateThink,true);
				}
				else
				{
					CL_SetupParticle (
						crand()*180, crand()*50, 0,
						move[0] + crand()*orgscale,	move[1] + crand()*orgscale,	move[2] + crand()*orgscale,
						crand()*velscale,	crand()*velscale,	crand()*velscale,
						0,		0,		20,
						255,		255,		255,
						0,	0,	0,
						0.5,		-0.5,
						GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
						5,			5,			
						particle_smoke,
						PART_OVERBRIGHT|PART_TRANS|PART_SHADED,
						CL_ParticleRotateThink,true);
				}
			}

			old->trailcount -= 5;
			if (old->trailcount < 100)
				old->trailcount = 100;
		}

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_RocketTrail
===============
*/
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, totallen;
	float		dec;

	// smoke
	CL_DiminishingTrail (start, end, old, EF_ROCKET);

	// fire
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	totallen = len = VectorNormalize (vec);

	dec = 1*cl_particle_scale->value;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		// falling particles
		if ( (rand()&7) == 0)
		{
			CL_SetupParticle (
				0,	0,	0,
				move[0] + crand()*5,	move[1] + crand()*5,	move[2] + crand()*5,
				crand()*20,	crand()*20,	crand()*20,
				0,		0,		20,
				255,	255,	200,
				0,	-50,	0,
				1,		-1.0 / (1+frand()*0.2),
				GL_SRC_ALPHA, GL_ONE,
				2,			-2,			
				particle_blaster,
				PART_GRAVITY_LIGHT,
				NULL,0);
		}
		VectorAdd (move, vec, move);
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	totallen = len = VectorNormalize (vec);
	dec = 1.5*cl_particle_scale->value;
	VectorScale (vec, dec, vec);
/*	len = totallen;
	VectorCopy (start, move);
	dec = 1.5;// *cl_particle_scale->value;
	VectorScale (vec, dec, vec);*/

	while (len > 0)
	{
		len -= dec;

		// flame
		CL_SetupParticle (
			crand()*180, crand()*100, 0,
			move[0],	move[1],	move[2],
			crand()*5,	crand()*5,	crand()*5,
			0,		0,		5,
			255,	225,	200,
			-50,	-50,	-50,
			0.5,		-2, // was 0.75, -3
			GL_SRC_ALPHA, GL_ONE,
			5,			5,
			particle_inferno,
			0,
			CL_ParticleRotateThink, true);

		VectorAdd (move, vec, move);
	}
}


/*
===============
FartherPoint
Returns true if the first vector
is farther from the viewpoint.
===============
*/
qboolean FartherPoint (vec3_t pt1, vec3_t pt2)
{
	vec3_t		distance1, distance2;

	VectorSubtract(pt1, cl.refdef.vieworg, distance1);
	VectorSubtract(pt2, cl.refdef.vieworg, distance2);
	return (VectorLength(distance1) > VectorLength(distance2));
}


/*
===============
CL_RailSpiral
===============
*/
#define DEVRAILSTEPS 2
//this is the length of each piece...
#define RAILTRAILSPACE 15

void CL_RailSpiral (vec3_t start, vec3_t end, int32_t color[3])
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	float			i;
	float		d, c, s;
	vec3_t		dir;
	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);
	len = min (len, cl_rail_length->value);  // cap length
	MakeNormalVectors (vec, right, up);

	VectorScale(vec, cl_rail_space->value*cl_particle_scale->value, vec);


	for (i=0; i<len; i += cl_rail_space->value*cl_particle_scale->value)
	{
		d = i * 0.05;
		c = cos(d);
		s = sin(d);

		VectorScale (right, c, dir);
		VectorMA (dir, s, up, dir);

		CL_SetupParticle (
			0,	0,	0,
			move[0] + dir[0]*3,	move[1] + dir[1]*3,	move[2] + dir[2]*3,
			dir[0]*6,	dir[1]*6,	dir[2]*6,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1,		-1.0,
			GL_SRC_ALPHA, GL_ONE,
			2,	-1,		
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}
/*
===============
CL_RailParticleSpiral
===============
*/

void CL_RailParticleSpiral (vec3_t start, vec3_t end, int32_t color[3])
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	float			i;
	float		d, c, s;
	vec3_t		dir;
	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);
	len = min (len, cl_rail_length->value);  // cap length
	MakeNormalVectors (vec, right, up);

	VectorScale(vec, 5.0* cl_rail_space->value*cl_particle_scale->value, vec);

	for (i=0; i<len; i += 5.0 * cl_rail_space->value*cl_particle_scale->value)
	{
		d = i * 0.05;
		c = cos(d);
		s = sin(d);

	
		VectorScale (right, c, dir);
		VectorMA (dir, s, up, dir);

		CL_SetupParticle (
			0,	0,	0,
			move[0] + dir[0]*3,	move[1] + dir[1]*3,	move[2] + dir[2]*3,
			dir[0]*6,	dir[1]*6,	dir[2]*6,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1.0,		-1.0,
			GL_SRC_ALPHA, GL_ONE,
			2, -1,			
			particle_generic,
			PART_TRANS|PART_OVERBRIGHT,
			NULL, 0);

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_RailCore
===============
*/
void CL_RailCore (vec3_t start, vec3_t end, int32_t color[3])
{
	vec3_t		move, last;
	vec3_t		vec, point;
	//vec3_t	right, up;
	int32_t			i;
	int32_t j;
	float		len;//, dec;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);
	len = min (len, cl_rail_length->value);  // cap length
	VectorCopy (vec, point);
	VectorScale (vec, RAILTRAILSPACE, vec);
	//MakeNormalVectors (vec, right, up);

	j = 0;
	while (len > 0)
	{
		VectorCopy (move, last);	
		VectorAdd (move, vec, move);

		len -= RAILTRAILSPACE;
		
		if (j % 5 == 0)
			CL_SetupParticle (
				last[0],	last[1],	last[2],
				move[0],	move[1],	move[2],
				0,	0,	0,
				0,	0,	0,
				color[0],	color[1],	color[2],
				0,	0,	0,
				1.0, -1.0,
				GL_SRC_ALPHA, GL_ONE,
				4, -1,			
				particle_generic,
				PART_TRANS | PART_OVERBRIGHT,
				NULL,0);

		for (i=0;i<3;i++)
			CL_SetupParticle (
				last[0],	last[1],	last[2],
				move[0],	move[1],	move[2],
				0,	0,	0,
				0,	0,	0,
				color[0],	color[1],	color[2],
				0,	0,	0,
				1.0,		-1.0,
				GL_SRC_ALPHA, GL_ONE,
				2, -1,			
				particle_beam2,
				PART_BEAM | PART_TRANS,
				NULL,0);
		j++;
	}
}

/*
===============
CL_ParticleDevRailThink
===============
*/
void CL_ParticleDevRailThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	int32_t i;
	vec3_t len;
	VectorSubtract(p->angle, org, len);
	
	*size *= (float)(SplashSize/VectorLength(len)) * 0.5/((4-*size));
	if (*size > SplashSize)
		*size = SplashSize;

	//setting up angle for sparks
	{
		float time1, time2;

		time1 = *time;
		time2 = time1*time1;

		for (i=0;i<2;i++)
			angle[i] = 3*(p->vel[i]*time1 + (p->accel[i])*time2);
		angle[2] = 3*(p->vel[2]*time1 + (p->accel[2]-PARTICLE_GRAVITY_DEFAULT)*time2);
	}

	p->thinknext = true;
}

/*
===============
CL_DevRailTrail
===============
*/
void CL_DevRailTrail (vec3_t start, vec3_t end, int32_t color[3])
{
	vec3_t		move;
	vec3_t		vec, point;
	float		len;
	int32_t			dec, i=0;
	
	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);
	len = min (len, cl_rail_length->value);  // cap length
	VectorCopy(vec, point);

	dec = 4;
	VectorScale (vec, dec, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;
		i++;

		if (i>=DEVRAILSTEPS)
		{
			for (i=3;i>0;i--)
			CL_SetupParticle (
				point[0],	point[1],	point[2],
				move[0],	move[1],	move[2],
				0,		0,		0,
				0,		0,		0,
				color[0],	color[1],	color[2],
				0,		-90,	-30,
				0.75,		-.75,
				GL_SRC_ALPHA, GL_ONE,
				dec*DEVRAILSTEPS*TWOTHIRDS,	0,			
				particle_beam2,
				PART_DIRECTION,
				NULL,0);
		}

		CL_SetupParticle (
			0,	0,	0,
			move[0],	move[1],	move[2],
			crand()*10,	crand()*10,	crand()*10+20,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1,		-0.75 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			2,			-0.25,			
			particle_solid,
			PART_GRAVITY_LIGHT|PART_SPARK,
			CL_ParticleDevRailThink,true);
		
		CL_SetupParticle (
			crand()*180, crand()*100, 0,
			move[0],	move[1],	move[2],
			crand()*10,	crand()*10,	crand()*10+20,
			0,		0,		5,
			255,	255,	255,
			0,	0,	0,
			0.25,		-0.25,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			5,			10,			
			particle_smoke,
			PART_TRANS|PART_GRAVITY_LIGHT|PART_OVERBRIGHT,
			CL_ParticleRotateThink, true);

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_ClassicRailTrail
===============
*/
void CL_ClassicRailTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec, point;
	float		len, dec;
	int32_t			i=0;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);


	len = min (len, cl_rail_length->value);  // cap length
	VectorCopy(vec, point);

	dec = 0.75 * cl_rail_space->value*cl_particle_scale->value;
	VectorScale (vec, dec, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		uint32_t temp = (rand() & 15);
		int32_t	beamred, beamgreen, beamblue;

		beamred = color8red(temp);
		beamgreen = color8green(temp);
		beamblue = color8blue(temp);

		len -= dec;
		i++;
		
		CL_SetupParticle (
			0, 0, 0,
			move[0] + crand() * 3, move[1] + crand() * 3, move[2] + crand() * 3,
			crand() * 3, crand() * 3, crand() * 3,
			0,		0,		0,
			beamred,	beamgreen,	beamblue,
			0,	0,	0,
			1.0, -1.0 / (0.6 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			0.75, 0.0,					
			particle_generic,
			PART_TRANS|PART_OVERBRIGHT,
			NULL, 0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_ClassicRailSprial
===============
*/

void CL_ClassicRailSprial (vec3_t start, vec3_t end, qboolean isRed)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	float			i;
	float		d, c, s;
	vec3_t		dir;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize (vec);
	len = min (len, cl_rail_length->value);  // cap length
	MakeNormalVectors (vec, right, up);

	VectorScale(vec, cl_rail_space->value*cl_particle_scale->value, vec);


	for (i=0; i<len; i += cl_rail_space->value*cl_particle_scale->value)
	{
		int32_t	beamred, beamgreen, beamblue;

		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		VectorScale (right, c, dir);
		VectorMA (dir, s, up, dir);

		if (isRed)
		{
			uint32_t temp = 0x42 + (rand() & 7);
			beamred = color8red(temp);
			beamgreen = color8green(temp);
			beamblue = color8blue(temp);

		} else 
		{
			uint32_t temp = 0x74 + (rand() & 7);
			beamred = color8red(temp);
			beamgreen = color8green(temp);
			beamblue = color8blue(temp);
		}

		CL_SetupParticle (
			0, 0, 0,
			move[0] + dir[0]*3,	move[1] + dir[1]*3,	move[2] + dir[2]*3,
			dir[0]*6,	dir[1]*6,	dir[2]*6,
			0,		0,		0,
			beamred,	beamgreen,	beamblue,
			0,	0,	0,
			1.0, -1.0 / (1.0 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			1, 0.0,			
			particle_generic,
			PART_TRANS|PART_OVERBRIGHT,
			NULL, 0);

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_RailTrail
===============
*/
void CL_RailTrail (vec3_t start, vec3_t end, qboolean isRed)
{

	vec3_t		vec;
    int32_t		coreColor[3] = {255,20,20};
    int32_t		spiralColor[3] = {191,191,191};

    if (!isRed)
    {
		ColorLookup(cl_railspiral_color->value,&spiralColor[0],&spiralColor[1],&spiralColor[2]);
		spiralColor[0] *= 0.75;
		spiralColor[1] *=0.75;
		spiralColor[2] *=0.75;
		ColorLookup(cl_railcore_color->value,&coreColor[0],&coreColor[1],&coreColor[2]);
		coreColor[0] *= 0.75;
		coreColor[1] *=0.75;
		coreColor[2] *=0.75;
	}

	VectorSubtract (end, start, vec);
	VectorNormalize(vec);
	
	switch((int32_t) cl_railtype->value)
	{
	default:
	case 0:
		{
			int32_t	splashColor[3];
			if (isRed)
			{
				uint32_t temp = 0x42 + (rand() & 7);
				splashColor[0] = color8red(temp);
				splashColor[1] = color8green(temp);
				splashColor[2] = color8blue(temp);

			} else 
			{
				uint32_t temp = 0x74 + (rand() & 7);
				splashColor[0] = color8red(temp);
				splashColor[1] = color8green(temp);
				splashColor[2] = color8blue(temp);
			}

			CL_ParticleRailDecal (end, vec, 7, splashColor);
			CL_ClassicRailTrail (start,end);
			CL_ClassicRailSprial (start, end, isRed);
		}
		break;
	case 1:
		CL_ParticleRailDecal (end, vec, 7, coreColor);
		CL_RailCore(start,end,coreColor);
		CL_RailParticleSpiral (start, end, spiralColor);
		break;
	case 2:
		CL_ParticleRailDecal (end, vec, 7, coreColor);
		CL_RailCore(start,end,coreColor);
		CL_RailSpiral (start, end, spiralColor);
		break;
	case 3:
		if (isRed)
			VectorCopy(spiralColor,coreColor);
		CL_ParticleRailDecal (end, vec, 7, coreColor);
		CL_RailCore(start,end,coreColor);
		break;
	case 4:
		if (isRed)
			VectorCopy(spiralColor,coreColor);
		CL_ParticleRailDecal (end, vec, 7, coreColor);
		CL_DevRailTrail (start, end, coreColor);
		break;

	}
}


// RAFAEL
/*
===============
CL_IonripperTrail
===============
*/
void CL_IonripperTrail (vec3_t start, vec3_t ent)
{
	vec3_t	move;
	vec3_t	vec;
	vec3_t  leftdir,up;
	float	len;
	int32_t		dec;

	VectorCopy (start, move);
	VectorSubtract (ent, start, vec);
	len = VectorNormalize (vec);

	MakeNormalVectors (vec, leftdir, up);

	dec = 3*cl_particle_scale->value;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0],	move[1],	move[2],
			0,	0,	0,
			0,		0,		0,
			255,	75,		0,
			0,	0,	0,
			0.75,		-1.0 / (0.3 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE,
			3,			0,			// was dec
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_BubbleTrail

===============
*/
void CL_BubbleTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			i;
	float		dec, size;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 32;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		size = (frand()>0.25)? 1 : (frand()>0.5) ? 2 : (frand()>0.75) ? 3 : 4;

		CL_SetupParticle (
			0,	0,	0,
			move[0]+crand()*2,	move[1]+crand()*2,	move[2]+crand()*2,
			crand()*5,	crand()*5,	crand()*5+6,
			0,		0,		0,
			255,	255,	255,
			0,	0,	0,
			0.75,		-0.5 / (1 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			size,	1,			
			particle_bubble,
			PART_TRANS|PART_SHADED,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_FlyParticles
===============
*/
#define	BEAMLENGTH			16


void CL_FlyParticles (vec3_t origin, int32_t count)
{
	int32_t			i;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;


	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.time / 1000.0;
	for (i=0 ; i<count ; i+=2)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		dist = sin(ltime + i)*64;

		CL_SetupParticle (
			0,	0,	0,
			origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH,origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH,
			origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			1,		-100,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			1+sin(i+ltime),	1,			
			particle_generic,
			PART_TRANS,
			NULL,0);
	}
}

/*
===============
CL_FlyEffect
===============
*/
void CL_FlyEffect (centity_t *ent, vec3_t origin)
{
	int32_t		n;
	int32_t		count;
	int32_t		starttime;

	if (ent->fly_stoptime < cl.time)
	{
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.time - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else
	{
		n = ent->fly_stoptime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CL_FlyParticles (origin, count);
}


/*
===============
CL_ParticleBFGThink
===============
*/
void CL_ParticleBFGThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	vec3_t len;
	VectorSubtract(p->angle, p->org, len);
	
	*size = (float)((300/VectorLength(len))*0.75);
}

#define	BEAMLENGTH			16

/*
===============
CL_BfgParticles
===============
*/
void CL_BfgParticles (entity_t *ent)
{
	int32_t			i;
	cparticle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64, dist2;
	vec3_t		v;
	float		ltime;
	
	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.time / 1000.0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		dist2 = dist;
		dist = sin(ltime + i)*64;

		p = CL_SetupParticle (
			ent->origin[0],	ent->origin[1],	ent->origin[2],
			ent->origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH,ent->origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH,
			ent->origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH,
			0,	0,		0,
			0,	0,		0,
			50,	200*dist2,	20,
			0,	0,	0,
			1,		-100,
			GL_SRC_ALPHA, GL_ONE,
			1,			1,			
			particle_generic,
			0,
			CL_ParticleBFGThink, true);
		
		if (!p)
			return;

		VectorSubtract (p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0;
	}
}


// RAFAEL
/*
===============
CL_TrapParticles
===============
*/
void CL_TrapParticles (entity_t *ent)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int32_t			dec;

	ent->origin[2]-=14;
	VectorCopy (ent->origin, start);
	VectorCopy (ent->origin, end);
	end[2]+=64;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0] + crand(),	move[1] + crand(),	move[2] + crand(),
			crand()*15,	crand()*15,	crand()*15,
			0,	0,	PARTICLE_GRAVITY_DEFAULT,
			230+crand()*25,	125+crand()*25,	25+crand()*25,
			0,		0,		0,
			1,		-1.0 / (0.3+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			3,			-3,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}

	{
	int32_t			i, j, k;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	
	ent->origin[2]+=14;
	VectorCopy (ent->origin, org);


	for (i=-2 ; i<=2 ; i+=4)
		for (j=-2 ; j<=2 ; j+=4)
			for (k=-2 ; k<=4 ; k+=4)
			{

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				VectorNormalize (dir);						
				vel = 50 + rand()&63;

				CL_SetupParticle (
					0,	0,	0,
					org[0] + i + ((rand()&23) * crand()), org[1] + j + ((rand()&23) * crand()),	org[2] + k + ((rand()&23) * crand()),
					dir[0]*vel,	dir[1]*vel,	dir[2]*vel,
					0,		0,		0,
					230+crand()*25,	125+crand()*25,	25+crand()*25,
					0,		0,		0,
					1,		-1.0 / (0.3+frand()*0.2),
					GL_SRC_ALPHA, GL_ONE,
					1,			1,			
					particle_generic,
					PART_GRAVITY_LIGHT,
					NULL,0);

			}
	}
}


/*
===============
CL_BFGExplosionParticles
===============
*/
//FIXME combined with CL_ExplosionParticles
void CL_BFGExplosionParticles (vec3_t org)
{
	int32_t			i;

	for (i=0 ; i<256 ; i++)
	{
		CL_SetupParticle (
			0,	0,	0,
			org[0] + ((rand()%32)-16), org[1] + ((rand()%32)-16),	org[2] + ((rand()%32)-16),
			(rand()%150)-75,	(rand()%150)-75,	(rand()%150)-75,
			0,	0,	0,
			50,	100+frand()*50,	0, //Knightmare- made more green
			0,	0,	0,
			1,		-0.8 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			10,			-10,			
			particle_generic,
			PART_GRAVITY_LIGHT,
			NULL,0);
	}
}


/*
===============
CL_TeleportParticles
===============
*/
void CL_TeleportParticles (vec3_t org)
{
	int32_t			i, j, k;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=-16 ; k<=32 ; k+=4)
			{
				dir[0] = j*16;
				dir[1] = i*16;
				dir[2] = k*16;

				VectorNormalize (dir);						
				vel = 150 + (rand()&63);

				CL_SetupParticle (
					0,	0,	0,
					org[0]+i+(rand()&3), org[1]+j+(rand()&3),	org[2]+k+(rand()&3),
					dir[0]*vel,	dir[1]*vel,	dir[2]*vel,
					0,		0,		0,
					200 + 55*frand(), 200 + 55*frand(), 200 + 55*frand(),
					0,		0,		0,
					1,		-1.0 / (0.3 + (rand()&7) * 0.02),
					GL_SRC_ALPHA, GL_ONE,
					1,			3,			
					particle_generic,
					PART_GRAVITY_LIGHT,
					NULL,0);
			}
}


/*
===============
CL_Flashlight
===============
*/
void CL_Flashlight (int32_t ent, vec3_t pos)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (ent);
	VectorCopy (pos,  dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}


/*
===============
CL_ColorFlash - flash of light
===============
*/
void CL_ColorFlash (vec3_t pos, int32_t ent, int32_t intensity, float r, float g, float b)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (ent);
	VectorCopy (pos,  dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
===============
CL_DebugTrail
===============
*/
void CL_DebugTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	float		dec;
	vec3_t		right, up;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	MakeNormalVectors (vec, right, up);

	dec = 8; // was 2
	VectorScale (vec, dec, vec);
	VectorCopy (start, move);

	while (len > 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0],	move[1],	move[2],
			0,	0,	0,
			0,		0,		0,
			50,	50,	255,
			0,	0,	0,
			1,		-0.75,
			GL_SRC_ALPHA, GL_ONE,
			7.5,			0,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_ForceWall
===============
*/
void CL_ForceWall (vec3_t start, vec3_t end, int32_t color8)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, 4, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= 4;
		
		if (frand() > 0.3)
			CL_SetupParticle (
				0,	0,	0,
				move[0] + crand()*3,	move[1] + crand()*3,	move[2] + crand()*3,
				0,	0,	-40 - (crand()*10),
				0,		0,		0,
				color[0]+5,	color[1]+5,	color[2]+5,
				0,	0,	0,
				1,		-1.0 / (3.0+frand()*0.5),
				GL_SRC_ALPHA, GL_ONE,
				5,			0,			
				particle_generic,
				0,
				NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_BubbleTrail2 (lets you control the # of bubbles by setting the distance between the spawns)
===============
*/
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int32_t dist)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			i;
	float		dec, size;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = dist;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		size = (frand()>0.25)? 1 : (frand()>0.5) ? 2 : (frand()>0.75) ? 3 : 4;
		CL_SetupParticle (
			0,	0,	0,
			move[0]+crand()*2,	move[1]+crand()*2,	move[2]+crand()*2,
			crand()*5,	crand()*5,	crand()*5+6,
			0,		0,		0,
			255,	255,	255,
			0,	0,	0,
			0.75,	-0.5 / (1 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			size,		1,			
			particle_bubble,
			PART_TRANS|PART_SHADED,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_HeatbeamParticles
===============
*/
void CL_HeatbeamParticles (vec3_t start, vec3_t forward)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	int32_t			i;
	float		c, s;
	vec3_t		dir;
	float		ltime;
	float		step, rstep;
	float		start_pt;
	float		rot;
	float		variance;
	float		size;
	int32_t			maxsteps;
	vec3_t		end;

	VectorMA (start, 4096, forward, end);

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// FIXME - pmm - these might end up using old values?
	//MakeNormalVectors (vec, right, up);
	VectorCopy (cl.v_right, right);
	VectorCopy (cl.v_up, up);

	if (cg_thirdperson->value) {
		ltime = (float) cl.time/250.0;
		step = 96;
		maxsteps = 10;
		variance = 1.2;
		size = 2;
	}
	else {
		ltime = (float) cl.time/1000.0;
		step = 32;
		maxsteps = 7;
		variance = 0.5;
		size = 1;
	}

	// this just misaligns rings with beam
	//VectorMA (move, -0.5, right, move);
	//VectorMA (move, -0.5, up, move);

	//ltime = (float) cl.time/1000.0;
	start_pt = fmod(ltime * 96.0f,step);
	VectorMA (move, start_pt, vec, move);

	VectorScale (vec, step, vec);

	//Com_Printf ("%f\n", ltime);
	rstep = M_PI/10.0*min(cl_particle_scale->value, 2);
	for (i=start_pt; i<len; i+=step)
	{
		if (i>step*maxsteps) // don't bother after the nth ring
			break;

		for (rot = 0; rot < M_PI*2; rot += rstep)
		{
		//	variance = 0.5;
			c = cos(rot)*variance;
			s = sin(rot)*variance;
			
			// trim it so it looks like it's starting at the origin
			if (i < 10)
			{
				VectorScale (right, c*(i/10.0), dir);
				VectorMA (dir, s*(i/10.0), up, dir);
			}
			else
			{
				VectorScale (right, c, dir);
				VectorMA (dir, s, up, dir);
			}

			CL_SetupParticle (
				0,	0,	0,
				move[0]+dir[0]*2,	move[1]+dir[1]*2,	move[2]+dir[2]*2, //Knightmare- decreased radius
				0,	0,	0,
				0,		0,		0,
				200+frand()*50,	200+frand()*25,	frand()*50,
				0,	0,	0,
				0.25,	-1000.0, // decreased alpha
				GL_SRC_ALPHA, GL_ONE,
				size,		1,		// shrunk size
				particle_blaster,
				0,
				NULL,0);
		}
		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int32_t red, int32_t green, int32_t blue,
							 int32_t reddelta, int32_t greendelta, int32_t bluedelta, int32_t count, int32_t magnitude)
{
	int32_t			i;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;
	//vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	//vectoangles2 (dir, angle_dir);
	//AngleVectors (angle_dir, f, r, u);

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<count ; i++)
	{
		p = CL_SetupParticle (
			0,	0,	0,
			org[0]+magnitude*0.1*crand(),	org[1]+magnitude*0.1*crand(),	org[2]+magnitude*0.1*crand(),
			0,	0,	0,
			0,		0,		0,
			red,	green,	blue,
			reddelta,	greendelta,	bluedelta,
			0.5,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			4,			-2,			
			particle_smoke,
			0,
			NULL,0);

		if (!p)
			return;

		VectorScale (dir, magnitude, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, u, p->vel);
	}
}


/*
===============
CL_ParticleSteamEffect2

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CL_ParticleSteamEffect2 (cl_sustain_t *self)
//vec3_t org, vec3_t dir, int32_t color, int32_t count, int32_t magnitude)
{
	int32_t			i;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;
	int32_t			color8 = self->color + (rand()&7);
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	//vectoangles2 (dir, angle_dir);
	//AngleVectors (angle_dir, f, r, u);

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0; i<self->count; i++)
	{
		p = CL_SetupParticle (
			0,		0,		0,
			self->org[0] + self->magnitude*0.1*crand(),	self->org[1] + self->magnitude*0.1*crand(),	self->org[2] + self->magnitude*0.1*crand(),
			0,		0,		0,
			0,		0,		0,
			color[0],	color[1],	color[2],
			0,		0,		0,
			1.0,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			4,			0,			
			particle_smoke,
			PART_GRAVITY_LIGHT,
			NULL,0);

		if (!p)
			return;

		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);
	}
	self->nextthink += self->thinkinterval;
}


/*
===============
CL_TrackerTrail
===============
*/
void CL_TrackerTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		forward,right,up,angle_dir;
	float		len;
	cparticle_t	*p;
	int32_t			dec;
	float		dist;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorCopy(vec, forward);
	vectoangles2 (forward, angle_dir);
	AngleVectors (angle_dir, forward, right, up);

	dec = 3*max(cl_particle_scale->value/2, 1);
	VectorScale (vec, 3*max(cl_particle_scale->value/2, 1), vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		p = CL_SetupParticle (
			0,	0,	0,
			0,	0,	0,
			0,	0,	5,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			1.0,	-2.0,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			2,		0,			
			particle_generic,
			PART_TRANS,
			NULL,0);

		if (!p)
			return;

		dist = DotProduct(move, forward);
		VectorMA(move, 8 * cos(dist), up, p->org);

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_TrackerShell
===============
*/
void CL_Tracker_Shell(vec3_t origin)
{
	vec3_t			dir;
	int32_t				i;
	cparticle_t		*p;

	for(i=0; i < (300/cl_particle_scale->value); i++)
	{
		p = CL_SetupParticle (
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			1.0,	-2.0,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			1,		0,	//Knightmare- changed size		
			particle_generic,
			PART_TRANS,
			NULL,0);

		if (!p)
			return;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorMA(origin, 40, dir, p->org);
	}
}


/*
======================
CL_MonsterPlasma_Shell
======================
*/
void CL_MonsterPlasma_Shell(vec3_t origin)
{
	vec3_t			dir;
	int32_t				i;
	cparticle_t		*p;

	for(i=0; i<40; i++)
	{
		p = CL_SetupParticle (
			0,		0,		0,
			0,		0,		0,
			0,		0,		0,
			0,		0,		0,
			220,	140,	50, //Knightmare- this was all black
			0,		0,		0,
			1.0,	INSTANT_PARTICLE,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			2,		0,			
			particle_generic,
			PART_TRANS|PART_INSTANT,
			NULL,0);

		if (!p)
			return;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
	
		VectorMA(origin, 10, dir, p->org);
	}
}


/*
===============
CL_Widowbeamout
===============
*/
void CL_Widowbeamout (cl_sustain_t *self)
{
	vec3_t			dir;
	int32_t				i;
	static int32_t colortable0[6] = {125,	255,	185,	125,	185,	255};
	static int32_t colortable1[6] = {185,	125,	255,	255,	125,	185};
	static int32_t colortable2[6] = {255,	185,	125,	185,	255,	125};
	cparticle_t		*p;
	float			ratio;
	int32_t				index;

	ratio = 1.0 - (((float)self->endtime - (float)cl.time)/2100.0);

	for(i=0; i<300; i++)
	{
		index = rand()&5;
		p = CL_SetupParticle (
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			colortable0[index],	colortable1[index],	colortable2[index],
			0,	0,	0,
			1.0,	INSTANT_PARTICLE,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			2,		0,			
			particle_generic,
			PART_TRANS|PART_INSTANT,
			NULL,0);

		if (!p)
			return;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
	
		VectorMA(self->org, (45.0 * ratio), dir, p->org);
	}
}


/*
============
CL_Nukeblast
============
*/
void CL_Nukeblast (cl_sustain_t *self)
{
	vec3_t			dir;
	int32_t				i;
	cparticle_t		*p;
	static int32_t colortable0[4] = {185,	155,	125,	95};
	static int32_t colortable1[4] = {185,	155,	125,	95};
	static int32_t colortable2[4] = {255,	255,	255,	255};
	float			ratio, size;
	int32_t				index;

	ratio = 1.0 - (((float)self->endtime - (float)cl.time)/1000.0);
	size = ratio*ratio;

	for(i=0; i<(700/cl_particle_scale->value); i++)
	{
		index = rand()&3;
		p = CL_SetupParticle (
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			colortable0[index],	colortable1[index],	colortable2[index],
			0,	0,	0,
			1-size,	INSTANT_PARTICLE,
			GL_SRC_ALPHA, GL_ONE,
			10*(0.5+ratio*0.5),	-1,			
			particle_generic,
			PART_INSTANT,
			NULL,0);

		if (!p)
			return;


		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorScale(dir, -1, p->angle);
		VectorMA(self->org, 200.0*size, dir, p->org); //was 100
		VectorMA(vec3_origin, 400.0*size, dir, p->vel); //was 100

	}
}


/*
==============
CL_WidowSplash
==============
*/
void CL_WidowSplash (vec3_t org)
{
	int32_t			i;
	cparticle_t	*p;
	vec3_t		dir;

	for (i=0; i<256; i++)
	{
		p = CL_SetupParticle (
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			0,	0,	0,
			rand()&255,	rand()&255,	rand()&255,
			0,	0,	0,
			1.0,		-0.8 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			3,			0,			
			particle_generic,
			0,
			NULL,0);

		if (!p)
			return;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorMA(org, 45.0, dir, p->org);
		VectorMA(vec3_origin, 40.0, dir, p->vel);
	}
}


/*
==================
CL_Tracker_Explode
==================
*/
void CL_Tracker_Explode (vec3_t	origin)
{
	vec3_t			dir, backdir;
	int32_t				i;
	cparticle_t		*p;

	for (i=0; i<(300/cl_particle_scale->value); i++)
	{
		p = CL_SetupParticle (
			0,		0,		0,
			0,		0,		0,
			0,		0,		0,
			0,		0,		-10, //was 20
			0,		0,		0,
			0,		0,		0,
			1.0,	-0.5,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			2,		0,			
			particle_generic,
			PART_TRANS,
			NULL,0);

		if (!p)
			return;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorScale(dir, -1, backdir);

		VectorCopy (origin, p->org); //Knightmare- start at center, not edge
	//	VectorMA(origin, 64, dir, p->org); 
		VectorScale(dir, (crand()*128), p->vel); //was backdir, 64
	}
	
}


/*
===============
CL_TagTrail
===============
*/
void CL_TagTrail (vec3_t start, vec3_t end, int32_t color8)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			dec;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len >= 0)
	{
		len -= dec;

		CL_SetupParticle (
			0,	0,	0,
			move[0] + crand()*16,	move[1] + crand()*16,	move[2] + crand()*16,
			crand()*5,	crand()*5,	crand()*5,
			0,		0,		20,
			color[0],	color[1],	color[2],
			0,	0,	0,
			1.0,		-1.0 / (0.8+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			1.5,			0,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}
}


/*
==========================
CL_ColorExplosionParticles
==========================
*/
void CL_ColorExplosionParticles (vec3_t org, int32_t color8, int32_t run)
{
	int32_t			i;
	vec3_t color = {color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<128 ; i++)
	{
		CL_SetupParticle (
			0,	0,	0,
			org[0] + ((rand()%32)-16),	org[1] + ((rand()%32)-16),	org[2] + ((rand()%32)-16),
			(rand()%256)-128,	(rand()%256)-128,	(rand()%256)-128,
			0,		0,		20,
			color[0] + (rand() % run),	color[1] + (rand() % run),	color[2] + (rand() % run),
			0,	0,	0,
			1.0,		-0.4 / (0.6 + frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			2,			1,			
			particle_generic,
			0,
			NULL,0);
	}
}



/*
=======================
CL_ParticleSmokeEffect - like the steam effect, but unaffected by gravity
=======================
*/
void CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, float size)
{
	float alpha = fabs(crand())*0.25 + 0.750;

	CL_SetupParticle (
		crand()*180, crand()*100, 0,
		org[0],	org[1],	org[2],
		dir[0],	dir[1],	dir[2],
		0,		0,		10,
		255,	255,	255,
		0,	0,	0,
		alpha,		-1.0,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		size,			5,			
		particle_smoke,
		PART_TRANS|PART_SHADED|PART_OVERBRIGHT,
		CL_ParticleRotateThink,true);
}


/*
===============
CL_ParticleElectricSparksThink
===============
*/
void CL_ParticleElectricSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int32_t *image, float *time)
{
	int32_t i;

	//setting up angle for sparks
	{
		float time1, time2;

		time1 = *time;
		time2 = time1*time1;

		for (i=0;i<2;i++)
			angle[i] = 0.25*(p->vel[i]*time1 + (p->accel[i])*time2);
		angle[2] = 0.25*(p->vel[2]*time1 + (p->accel[2]-PARTICLE_GRAVITY_DEFAULT)*time2);
	}

	p->thinknext = true;
}

/*
===============
CL_ElectricParticles

new sparks for Rogue turrets
===============
*/
void CL_ElectricParticles (vec3_t org, vec3_t dir, int32_t count)
{
	int32_t			i, j;
	vec3_t		start;
	float d;

	for (i = 0; i < 40; i++)
	{
		d = rand()&31;
		for (j = 0; j < 3; j++)
			start[j] = org[j] + ((rand()&7)-4) + d*dir[j];
		CL_SetupParticle (
			0,			0,			0,
			start[0],	start[1],	start[2],
			crand()*20,	crand()*20,	crand()*20,
			0,			0,			-PARTICLE_GRAVITY_DEFAULT,
			25,			100,		255,
			50,			50,			50,
			1,		-1.0 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			6,		-3,			
			particle_solid,
			PART_GRAVITY_LIGHT|PART_SPARK,
			CL_ParticleElectricSparksThink, true);
	}
}


//Knightmare- removed for Psychospaz's enhanced particle code
#if 0
/*
===============
CL_SmokeTrail
===============
*/
void CL_SmokeTrail (vec3_t start, vec3_t end, int32_t colorStart, int32_t colorRun, int32_t spacing)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int32_t			j;
	cparticle_t	*p;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, spacing, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= spacing;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.5);
		p->color = colorStart + (rand() % colorRun);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*3;
			p->accel[j] = 0;
		}
		p->vel[2] = 20 + crand()*5;

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_FlameEffects
===============
*/
void CL_FlameEffects (centity_t *ent, vec3_t origin)
{
	int32_t			n, count;
	int32_t			j;
	cparticle_t	*p;

	count = rand() & 0xF;

	for(n=0;n<count;n++)
	{
		if (!free_particles)
			return;
			
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		VectorClear (p->accel);
		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.2);
		p->color = 226 + (rand() % 4);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = origin[j] + crand()*5;
			p->vel[j] = crand()*5;
		}
		p->vel[2] = crand() * -10;
		p->accel[2] = -PARTICLE_GRAVITY_DEFAULT;
	}

	count = rand() & 0x7;

	for(n=0;n<count;n++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.5);
		p->color = 0 + (rand() % 4);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = origin[j] + crand()*3;
		}
		p->vel[2] = 20 + crand()*5;
	}

}


/*
===============
CL_GenericParticleEffect
===============
*/
void CL_GenericParticleEffect (vec3_t org, vec3_t dir, int32_t color, int32_t count, int32_t numcolors, int32_t dirspread, float alphavel)
{
	int32_t			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		if (numcolors > 1)
			p->color = color + (rand() & numcolors);
		else
			p->color = color;

		d = rand() & dirspread;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY_DEFAULT;
//		VectorCopy (accel, p->accel);
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*alphavel);
//		p->alphavel = alphavel;
	}
}
#endif
//end Knightmare
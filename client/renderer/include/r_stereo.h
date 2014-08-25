/*
====================================================================

r_stereo.c

====================================================================
*/

#ifndef __R_STEREO_H
#define __R_STEREO_H

#include "r_local.h"

extern cvar_t  *r_stereo;
extern cvar_t  *r_stereo_separation;
extern qboolean sbsEnabled;

extern fbo_t stereo_fbo[2];
extern fbo_t stereo_hud;

void R_StereoInit();
void R_StereoShutdown();
void R_StereoFrame(fbo_t *view);
void R_Stereo_EndFrame(fbo_t *view);

#endif
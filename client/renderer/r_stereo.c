/*

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
// r_stereo.c: side by side 3d support

#include "include/r_local.h"
#include "include/r_vr.h"

cvar_t  *r_stereo;
cvar_t  *r_stereo_separation;

fbo_t stereo_hud;
fbo_t stereo_fbo[2];
static qboolean firstInit = false;

static float screenWidth, screenHeight;
qboolean sbsEnabled;

void R_StereoFrame(fbo_t *view)
{
	if (sbsEnabled)
	{
		if (screenWidth != view->width || screenHeight != view->height)
		{
			screenWidth = view->width;
			screenHeight = view->height;


			R_ResizeFBO(screenWidth/2.0,screenHeight,1,GL_RGBA8,&stereo_fbo[0]);
			R_ResizeFBO(screenWidth/2.0,screenHeight,1,GL_RGBA8,&stereo_fbo[1]);
			R_ResizeFBO(screenWidth,screenHeight,1,GL_RGBA8,&stereo_hud);
		}

		GL_ClearColor(0.0, 0.0, 0.0, 0.0);
		R_ClearFBO(&stereo_fbo[0]);
		R_ClearFBO(&stereo_fbo[1]);
		R_ClearFBO(&stereo_hud);
		GL_SetDefaultClearColor();		

	} 
}

void R_Clear(void);
void R_Stereo_EndFrame(fbo_t *view)
{
	if (sbsEnabled)
	{
		float w, h;
		w = view->width / 2.0;
		h = view->height;
		R_SetupBlit();
		GL_BindFBO(view);
		glViewport(0, 0, w, h);
		R_BlitTextureToScreen(stereo_fbo[0].texture);
		glViewport(w, 0, w, h);
		R_BlitTextureToScreen(stereo_fbo[1].texture);
		// enable alpha testing so only pixels that have alpha info get written out
		// prevents black pixels from being rendered into the view
		GL_Enable(GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER, 0.0f);
		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glViewport(0, 0, w, h);
		R_BlitTextureToScreen(stereo_hud.texture);
		glViewport(w, 0, w, h);
		R_BlitTextureToScreen(stereo_hud.texture);
		R_TeardownBlit();
		GL_Disable(GL_BLEND);
		GL_Disable(GL_ALPHA_TEST);
	}
}

void R_StereoInit()
{
	if (!firstInit)
	{
		firstInit = true;
		r_stereo = Cvar_Get("r_stereo","0",CVAR_ARCHIVE);
		r_stereo_separation = Cvar_Get("r_stereo_separation","4",CVAR_ARCHIVE);
	}

	sbsEnabled = false;

	if (glConfig.ext_framebuffer_object && r_stereo->value)
	{
		sbsEnabled = true;
		R_InitFBO(&stereo_fbo[0]);
		R_InitFBO(&stereo_fbo[1]);
		R_InitFBO(&stereo_hud);
		screenWidth = 0;
		screenHeight = 0;
	}
}

void R_StereoShutdown()
{
	if (sbsEnabled)
	{
		R_DelFBO(&stereo_fbo[0]);
		R_DelFBO(&stereo_fbo[1]);
		R_DelFBO(&stereo_hud);
		sbsEnabled = false;
	}
}
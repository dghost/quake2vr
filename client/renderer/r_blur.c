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
// r_blur.c: post-processing blur effect

#include "include/r_local.h"
#include "include/r_vr.h"


cvar_t  *r_blur;

static fbo_t pongFBO;
static fbo_t pingFBO;
static fbo_t offscreenFBO;
static qboolean blurSupported;

void R_Blur(float blurScale)
{
	if (blurSupported && r_blur->value)
	{
		GLuint currentFBO = glState.currentFBO;
		GLuint width, height;
		float scale = r_blur->value * blurScale;
		if (scale < 1)
			scale = 1;

		if (vid.width != offscreenFBO.width || vid.height != offscreenFBO.height)
			R_ResizeFBO(vid.width,vid.height,TRUE, &offscreenFBO);

		glViewport(0,0,vid.width,vid.height);
		GL_MBind(0,offscreenFBO.texture);
		glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,0,0,vid.width,vid.height,0);

		
		width = (vid.width / scale);
		height = (vid.height / scale);

		if (width != pongFBO.width || height != pongFBO.height)
		{
			R_ResizeFBO(width ,height ,TRUE, &pongFBO);
			R_ResizeFBO(width ,height ,TRUE, &pingFBO);
		}

		GL_ClearColor(0,0,0,0);

		GL_LoadIdentity(GL_PROJECTION);
		GL_LoadIdentity(GL_MODELVIEW);

		glUseProgram(blurXshader.shader->program);
		glUniform2f(blurXshader.res_uniform,pongFBO.width,pongFBO.height);

		GL_MBind(0,offscreenFBO.texture);
		R_BindFBO(&pongFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();


		glUseProgram(blurYshader.shader->program);
		glUniform2f(blurYshader.res_uniform,pongFBO.width,pongFBO.height);

		GL_MBind(0,pongFBO.texture);
		R_BindFBO(&pingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();

		glUseProgram(0);

		GL_MBind(0,pingFBO.texture);
		GL_BindFBO(currentFBO);
		glViewport(0,0,vid.width,vid.height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();

		GL_MBind(0,0);
	}
}

static firstInit = false;
void R_BlurInit()
{
	if (!firstInit)
	{
		firstInit = true;
	}

	if (glConfig.shader_version_major > 1 || (glConfig.shader_version_major == 1 && glConfig.shader_version_minor >= 2))
	{
		blurSupported = true;
		R_InitFBO(&pongFBO);
		R_InitFBO(&pingFBO);
		R_InitFBO(&offscreenFBO);
	}
}

void R_BlurShutdown()
{
	R_DelFBO(&pongFBO);
	R_DelFBO(&pingFBO);
	R_DelFBO(&offscreenFBO);
	blurSupported = false;
}
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
// r_blur.c: post-processing blur effect

#include "include/r_local.h"
#include "include/r_vr.h"


cvar_t  *r_blur;
cvar_t	*r_blur_radius;
cvar_t  *r_blur_texscale;
cvar_t  *r_flashblur;
cvar_t  *r_bloom;
cvar_t	*r_bloom_radius;
cvar_t	*r_bloom_texscale;
cvar_t  *r_bloom_minthreshold;
cvar_t	*r_bloom_falloff;

static fbo_t pongFBO;
static fbo_t pingFBO;
static qboolean blitSupported;
static qboolean blurSupported;
static qboolean bloomSupported;

/*
=============
Full screen quad stuff
=============
*/

typedef struct {
	GLbyte x,y;
	GLubyte	s,t;
} quad_vert_t;

static attribs_t postprocess_attribs[2] = {
	{0, 2, GL_BYTE, GL_FALSE, sizeof(quad_vert_t), 0},
	{1, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(quad_vert_t), sizeof(GLbyte) * 2},
};

static quad_vert_t quad_verts[4] = {
	{-1, -1	,0, 0},
	{-1,  1	,0, 1},
	{ 1, -1 ,1, 0},	
	{ 1,  1	,1, 1}
};

buffer_t quad;


void R_SetupQuadState()
{
	glDisableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glDisableClientState (GL_VERTEX_ARRAY);
	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);
	R_BindBuffer(&quad);
	R_SetAttribsVBO(postprocess_attribs,2);
}

void R_TeardownQuadState()
{
	R_ReleaseBuffer(&quad);
	glDisableVertexAttribArray (0);
	glDisableVertexAttribArray (1);

	glEnableClientState (GL_COLOR_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
}

void R_DrawQuad()
{
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

/*
=============
Shader stuff
=============
*/


typedef struct {
	r_shaderobject_t *shader;
	GLint res_uniform;
	GLint scale_uniform;
	GLint weight_uniform;
} r_blurshader_t;

typedef struct {
	r_shaderobject_t *shader;
	GLint scale_uniform;
	GLint blend_uniform;
} r_blitshader_t;

typedef struct {
	r_shaderobject_t *shader;
	GLint scale_uniform;
	GLint brightpass_uniform;
	GLint conversion_uniform;
	GLint res_uniform;
	GLint weight_uniform;
	GLint falloff_uniform;
} r_bloomshader_t;


r_blurshader_t blurXshader;
r_blurshader_t blurYshader;
r_bloomshader_t bloomPass;
r_bloomshader_t brightPass;
r_blitshader_t passthrough;
r_blitshader_t colorBlend;

r_attrib_t postProcess[] = {
	{"Position",0},
	{"TexCoord",1},
	{NULL,0}
};


static r_shaderobject_t blurX_object = {
	0,
	// vertex shader (identity)
	"blurX.vert",
	// fragment shader
	"blur.frag",
	postProcess
};

static r_shaderobject_t blurY_object = {
	0,
	// vertex shader (identity)
	"blurY.vert",
	// fragment shader
	"blur.frag",
	postProcess
};

static r_shaderobject_t brightPass_object = {
	0,
	// vertex shader (identity)
	"blurX.vert",
	// fragment shader
	"lowpass.frag",
	postProcess
};


static r_shaderobject_t bloomPass_object = {
	0,
	// vertex shader (identity)
	"blurY.vert",
	// fragment shader
	"bloom.frag",
	postProcess
};

static r_shaderobject_t passthrough_object = {
	0,
	// vertex shader (identity)
	"quad.vert",
	// fragment shader
	"passthrough.frag",
	postProcess
};

static r_shaderobject_t colorBlend_object = {
	0,
	// vertex shader (identity)
	"quad.vert",
	// fragment shader
	"blend.frag",
	postProcess
};


void R_BlurFBO(float blurScale, float blendColor[4], fbo_t *source)
{
	if (blurSupported && r_blur->value && blurScale > 0.0)
	{
		GLuint currentFBO = glState.currentFBO;
		GLuint width, height;
		float scale = r_blur_texscale->value;
		float size = blurScale * r_blur_radius->value;
		float weights[5] = {0, 0, 0, 0, 0};
		float xCoord, yCoord;
		int i;

		if (size <= 0)
			return;

		if ((int) r_antialias->value == ANTIALIAS_4X_FSAA)
			scale /= 2.0;

		width = (source->width * scale);
		height = (source->height * scale);

		if (width > pongFBO.width || height > pongFBO.height)
		{
			R_ResizeFBO(width ,height ,TRUE, GL_RGBA8, &pongFBO);
			R_ResizeFBO(width ,height ,TRUE, GL_RGBA8, &pingFBO);
		}

		xCoord = width / (float) pongFBO.width;
		yCoord = height / (float) pongFBO.height;

		for (i = 0 ; i < 5 ; i++)
		{

			float two_oSq = 2.0 * size;
			float temp = (1.0 / sqrt(two_oSq * M_PI)) * exp(-((i * i) / (two_oSq)));

			if (temp > 1)
				temp = 1;
			else if (temp < 0)
				temp = 0;
			weights[i] = temp;
		}

		if (weights[0] == 1)
			return;

		//		Com_Printf("Weights: %f %f %f %f %f\n",weights[0],weights[1],weights[2],weights[3],weights[4]);


		GL_ClearColor(0,0,0,0);
		GL_LoadIdentity(GL_PROJECTION);
		GL_LoadIdentity(GL_MODELVIEW);
		GL_Disable(GL_DEPTH_TEST);
		glUseProgram(blurXshader.shader->program);
		glUniform2f(blurXshader.res_uniform,width,height);
		glUniform1fv(blurXshader.weight_uniform,5,weights);
		glUniform2f(blurXshader.scale_uniform,1.0,1.0);

		GL_MBind(0,source->texture);
		R_BindFBO(&pongFBO);
		//		GL_BindFBO(pongFBO.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);
		R_SetupQuadState();
		R_DrawQuad();

		glUseProgram(blurYshader.shader->program);
		glUniform2f(blurYshader.res_uniform,pingFBO.width,pingFBO.height);
		glUniform1fv(blurYshader.weight_uniform,5,weights);
		glUniform2f(blurYshader.scale_uniform,xCoord,yCoord);

		GL_MBind(0,pongFBO.texture);
		R_BindFBO(&pingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);


		R_DrawQuad();


		GL_MBind(0,pingFBO.texture);
		R_BindFBO(source);

		glUseProgram(colorBlend.shader->program);
		glUniform2f(colorBlend.scale_uniform,xCoord,yCoord);
		glUniform4fv(colorBlend.blend_uniform,1,blendColor);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		R_DrawQuad();

		glUseProgram(0);
		R_TeardownQuadState();

		GL_MBind(0,0);

		GL_Enable(GL_DEPTH_TEST);
	} 
}

void R_BloomFBO(fbo_t *source)
{
	if (bloomSupported && r_bloom->value)
	{
		GLuint currentFBO = glState.currentFBO;
		GLuint width, height;
		float scale = r_bloom_texscale->value;
		float size = r_bloom_radius->value;
		float weights[5] = {0, 0, 0, 0, 0};
		float xCoord, yCoord;
		float lowpass_threshold = r_bloom_minthreshold->value;
		float bloom_falloff = r_bloom_falloff->value;

		int i;

		if (size < 0)
			return;

		if ((int) r_antialias->value == ANTIALIAS_4X_FSAA)
			scale /= 2.0;

		width = (source->width * scale);
		height = (source->height * scale);

		if (width > pongFBO.width || height > pongFBO.height)
		{
			R_ResizeFBO(width ,height ,TRUE, GL_RGBA8, &pongFBO);
			R_ResizeFBO(width ,height ,TRUE, GL_RGBA8, &pingFBO);
		}

		xCoord = width / (float) pongFBO.width;
		yCoord = height / (float) pongFBO.height;

		for (i = 0 ; i < 5 ; i++)
		{

			float two_oSq = 2.0 * size;
			float temp = (1.0 / sqrt(two_oSq * M_PI)) * exp(-((i * i) / (two_oSq)));

			if (temp > 1)
				temp = 1;
			else if (temp < 0)
				temp = 0;
			weights[i] = temp;
		}

		if (weights[0] == 1)
			return;

		//		Com_Printf("Weights: %f %f %f %f %f\n",weights[0],weights[1],weights[2],weights[3],weights[4]);


		GL_ClearColor(0,0,0,0);
		R_SetupQuadState();

		GL_LoadIdentity(GL_PROJECTION);
		GL_LoadIdentity(GL_MODELVIEW);
		GL_Disable(GL_DEPTH_TEST);


		R_BindFBO(&pongFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);

		GL_MBind(0,source->texture);		

		glUseProgram(brightPass.shader->program);
		glUniform2f(brightPass.res_uniform,width,height);
		glUniform1fv(brightPass.weight_uniform,5,weights);
		glUniform1f(brightPass.brightpass_uniform,lowpass_threshold);
		glUniform2f(brightPass.scale_uniform,1.0,1.0);
		glUniform1f(brightPass.falloff_uniform,bloom_falloff);

		R_DrawQuad();

		glUseProgram(bloomPass.shader->program);
		glUniform2f(bloomPass.res_uniform,pingFBO.width,pingFBO.height);
		glUniform1fv(bloomPass.weight_uniform,5,weights);
		glUniform2f(bloomPass.scale_uniform,xCoord,yCoord);


		GL_MBind(0,pongFBO.texture);
		R_BindFBO(&pingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);

		R_DrawQuad();

		GL_MBind(0,pingFBO.texture);
		R_BindFBO(source);

		glUseProgram(passthrough.shader->program);
		glUniform2f(passthrough.scale_uniform,xCoord,yCoord);

		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

		R_DrawQuad();

		glUseProgram(0);
		R_TeardownQuadState();
		GL_MBind(0,0);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_Disable(GL_BLEND);

		GL_Enable(GL_DEPTH_TEST);
	} 
}

qboolean R_InitPostsprocessShaders()
{
	qboolean success = true;

	if (glConfig.shader_version_major > 1 || (glConfig.shader_version_major == 1 && glConfig.shader_version_minor >= 2))
	{
		Com_Printf("...loading blit shaders: ");

		if (R_CompileShaderFromFiles(&passthrough_object))
		{
			GLint texloc;
			passthrough.shader = &passthrough_object;
			glUseProgram(passthrough.shader->program);
			texloc = glGetUniformLocation(passthrough.shader->program,"tex");
			glUniform1i(texloc,0);
			passthrough.blend_uniform = glGetUniformLocation(passthrough.shader->program,"blendColor");
			passthrough.scale_uniform = glGetUniformLocation(passthrough.shader->program,"texScale");			
			glUseProgram(0);

			success = success && true;
		}
		else {
			success = false;
		}

		if (R_CompileShaderFromFiles(&colorBlend_object))
		{
			GLint texloc;
			colorBlend.shader = &colorBlend_object;
			glUseProgram(colorBlend.shader->program);
			texloc = glGetUniformLocation(colorBlend.shader->program,"tex");
			glUniform1i(texloc,0);
			colorBlend.blend_uniform = glGetUniformLocation(colorBlend.shader->program,"blendColor");
			colorBlend.scale_uniform = glGetUniformLocation(colorBlend.shader->program,"texScale");
			glUseProgram(0);
			success = success && true;
		}
		else {
			success = false;
		}

		if (success)
		{
			Com_Printf("success!\n");
			blitSupported = true;

		} else {
			Com_Printf("failed!\n");
		}

		if (blitSupported)
		{
			Com_Printf("...loading blur shaders: ");
			if (R_CompileShaderFromFiles(&blurX_object))
			{
				GLint texloc;
				blurXshader.shader = &blurX_object;
				glUseProgram(blurXshader.shader->program);
				texloc = glGetUniformLocation(blurXshader.shader->program,"tex");
				glUniform1i(texloc,0);
				blurXshader.res_uniform = glGetUniformLocation(blurXshader.shader->program,"resolution");
				blurXshader.weight_uniform = glGetUniformLocation(blurXshader.shader->program,"weight");
				blurXshader.scale_uniform = glGetUniformLocation(blurXshader.shader->program,"texScale");			
				glUseProgram(0);

				success = success && true;
			}
			else {
				success = false;
			}

			if (R_CompileShaderFromFiles(&blurY_object))
			{
				GLint texloc;
				blurYshader.shader = &blurY_object;
				glUseProgram(blurYshader.shader->program);
				texloc = glGetUniformLocation(blurYshader.shader->program,"tex");
				glUniform1i(texloc,0);
				blurYshader.res_uniform = glGetUniformLocation(blurYshader.shader->program,"resolution");
				blurYshader.weight_uniform = glGetUniformLocation(blurYshader.shader->program,"weight");
				blurYshader.scale_uniform = glGetUniformLocation(blurYshader.shader->program,"texScale");
				glUseProgram(0);
				success = success && true;
			}
			else {
				success = false;
			}

			if (success)
			{
				Com_Printf("success!\n");
				blurSupported = true;

			} else {
				Com_Printf("failed!\n");
			}


			Com_Printf("...loading bloom shaders: ");


			if (R_CompileShaderFromFiles(&bloomPass_object))
			{
				GLint texloc;
				bloomPass.shader = &bloomPass_object;
				glUseProgram(bloomPass.shader->program);
				texloc = glGetUniformLocation(bloomPass.shader->program,"blurTex");
				glUniform1i(texloc,0);
				texloc = glGetUniformLocation(bloomPass.shader->program,"sourceTex");
				glUniform1i(texloc,1);
				bloomPass.brightpass_uniform = glGetUniformLocation(bloomPass.shader->program,"minThreshold");
				bloomPass.conversion_uniform = glGetUniformLocation(bloomPass.shader->program,"lumConversion");
				bloomPass.res_uniform = glGetUniformLocation(bloomPass.shader->program,"resolution");
				bloomPass.weight_uniform = glGetUniformLocation(bloomPass.shader->program,"weight");
				bloomPass.scale_uniform = glGetUniformLocation(bloomPass.shader->program,"texScale");
				bloomPass.falloff_uniform = glGetUniformLocation(bloomPass.shader->program,"falloff");
				glUseProgram(0);
				success = success && true;
			}
			else {
				success = false;
			}
			if (R_CompileShaderFromFiles(&brightPass_object))
			{
				GLint texloc;
				brightPass.shader = &brightPass_object;
				glUseProgram(brightPass.shader->program);
				texloc = glGetUniformLocation(brightPass.shader->program,"tex");
				glUniform1i(texloc,0);
				texloc = glGetUniformLocation(brightPass.shader->program,"sourceTex");
				glUniform1i(texloc,1);
				brightPass.brightpass_uniform = glGetUniformLocation(brightPass.shader->program,"minThreshold");
				brightPass.conversion_uniform = glGetUniformLocation(brightPass.shader->program,"lumConversion");
				brightPass.res_uniform = glGetUniformLocation(brightPass.shader->program,"resolution");
				brightPass.weight_uniform = glGetUniformLocation(brightPass.shader->program,"weight");
				brightPass.scale_uniform = glGetUniformLocation(brightPass.shader->program,"texScale");
				brightPass.falloff_uniform = glGetUniformLocation(brightPass.shader->program,"falloff");
				glUseProgram(0);
				success = success && true;
			}
			else {
				success = false;
			}

			if (success)
				bloomSupported = true;

			if (success)
			{
				Com_Printf("success!\n");
				bloomSupported = true;

			} else {
				Com_Printf("failed!\n");
			}
		}
	} else {
		success = false;
		Com_Printf("success!\n");

	}
	return success;
}

void R_TeardownPostprocessShaders()
{
	R_DelShaderProgram(&blurX_object);
	R_DelShaderProgram(&blurY_object);
	R_DelShaderProgram(&brightPass_object);
	R_DelShaderProgram(&bloomPass_object);
	R_DelShaderProgram(&passthrough_object);
	R_DelShaderProgram(&colorBlend_object);
	blitSupported = false;
	blurSupported = false;
	bloomSupported = false;
}

static qboolean firstInit = false;
void R_PostProcessInit()
{
	if (!firstInit)
	{
		firstInit = true;
		r_flashblur = Cvar_Get("r_flashblur","1",CVAR_ARCHIVE);
		r_bloom_texscale = Cvar_Get("r_bloom_texscale","0.5",CVAR_ARCHIVE);
		r_bloom_radius = Cvar_Get("r_bloom_radius","5.0",CVAR_ARCHIVE);
		r_bloom_minthreshold = Cvar_Get( "r_bloom_minthreshold", "0.15", CVAR_ARCHIVE );	// was 0.08
		r_bloom_falloff = Cvar_Get("r_bloom_falloff","1.5",CVAR_ARCHIVE);
		r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );	// BLOOMS
		r_blur_texscale = Cvar_Get("r_blur_texscale","1.0",CVAR_ARCHIVE);
		r_blur_radius = Cvar_Get("r_blur_radius","5.0",CVAR_ARCHIVE);
		r_blur = Cvar_Get("r_blur", "5", CVAR_ARCHIVE );
	}
	if (glConfig.ext_framebuffer_object && glConfig.arb_vertex_buffer_object)
	{
		R_InitFBO(&pongFBO);
		R_InitFBO(&pingFBO);

		R_InitBuffer(&quad);
		R_CreateBuffer(&quad, GL_ARRAY_BUFFER,GL_STATIC_DRAW);
		R_BindBuffer(&quad);
		R_SetBuffer(&quad, sizeof(quad_vert_t) * 4,(void *)&quad_verts[0]);
		R_ReleaseBuffer(&quad);
		R_InitPostsprocessShaders();
	}
}

void R_PostProcessShutdown()
{
	R_DelFBO(&pongFBO);
	R_DelFBO(&pingFBO);
	R_DelBuffer(&quad);
	R_TeardownPostprocessShaders();
}
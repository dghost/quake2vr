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
static fbo_t fxaaFBO;
static qboolean blitSupported;
static qboolean blurSupported;
static qboolean bloomSupported;
static qboolean fxaaSupported;

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
	{1, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(quad_vert_t), (GLvoid *) (sizeof(GLbyte) * 2)},
};

static quad_vert_t quad_verts[4] = {
	{-1, -1	,0, 0},
	{-1,  1	,0, 1},
	{ 1, -1 ,1, 0},	
	{ 1,  1	,1, 1}
};

static quad_vert_t inv_quad_verts[4] = {
	{-1, -1	,0, 1},
	{-1,  1	,0, 0},
	{ 1, -1 ,1, 1},	
	{ 1,  1	,1, 0}
};
buffer_t quad;
buffer_t inv_quad;


static buffer_t *currentBuffer = NULL;
void R_SetupQuadState()
{
    if (currentBuffer == &quad)
        return;
	currentBuffer = &quad;
	glDisableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glDisableClientState (GL_VERTEX_ARRAY);
	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);
	R_BindBuffer(currentBuffer);
	R_SetAttribsVBO(postprocess_attribs,2);
}

void R_SetupInvQuadState()
{
    if (currentBuffer == &inv_quad)
        return;
	currentBuffer = &inv_quad;
	glDisableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glDisableClientState (GL_VERTEX_ARRAY);
	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);
	R_BindBuffer(currentBuffer);
	R_SetAttribsVBO(postprocess_attribs,2);
}

void R_TeardownQuadState()
{
	if (currentBuffer == NULL)
		return;
	R_ReleaseBuffer(&quad);
	glDisableVertexAttribArray (0);
	glDisableVertexAttribArray (1);
	glEnableClientState (GL_COLOR_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	currentBuffer = NULL;
}

void R_DrawQuad()
{
	qboolean needsSetup = !(currentBuffer == &quad);
    qboolean needsTeardown = (currentBuffer == NULL);
	if (needsSetup)
		R_SetupQuadState();
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	if (needsTeardown)
		R_TeardownQuadState();
}

void R_DrawQuadFlipped()
{
    qboolean needsSetup = !(currentBuffer == &inv_quad);
    qboolean needsTeardown = (currentBuffer == NULL);
    if (needsSetup)
        R_SetupInvQuadState();
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    if (needsTeardown)
        R_TeardownQuadState();
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
	GLint gamma_uniform;
	GLint mincolor_uniform;
} r_blitshader_t;

typedef struct {
	r_shaderobject_t *shader;
	GLint scale_uniform;
	GLint res_uniform;
} r_fxaashader_t;

typedef struct {
	r_shaderobject_t *shader;
	GLint scale_uniform;
	GLint lowpass_uniform;
	GLint conversion_uniform;
	GLint res_uniform;
	GLint weight_uniform;
	GLint falloff_uniform;
} r_bloomshader_t;


r_blurshader_t blurXshader;
r_blurshader_t blurYshader;
r_bloomshader_t lowpassFilter;
r_blitshader_t passthrough;
r_blitshader_t colorBlend;
r_blitshader_t gammaAdjust;
r_fxaashader_t fxaa;

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

static r_shaderobject_t lowpassFilter_object = {
	0,
	// vertex shader (identity)
	"blurX.vert",
	// fragment shader
	"lowpass.frag",
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

static r_shaderobject_t gammaAdjust_object = {
	0,
	// vertex shader (identity)
	"quad.vert",
	// fragment shader
	"gamma.frag",
	postProcess
};

static r_shaderobject_t FXAA_object = {
	0,
	// vertex shader (identity)
	"quad.vert",
	// fragment shader
	"fxaa.frag",
	postProcess
};

static qboolean setupForBlit = false;
void R_SetupBlit()
{
	if (setupForBlit)
		return;
	glUseProgram(passthrough.shader->program);
	glUniform2f(passthrough.scale_uniform,1.0,1.0);	
	setupForBlit = true;
}


void R_TeardownBlit()
{
	if (!setupForBlit)
		return;
	glUseProgram(0);
	setupForBlit = false;
}

void R_BlitTextureToScreen(GLuint texture)
{
	qboolean alreadySetup = setupForBlit;
	GL_MBind(0,texture);

	if (!alreadySetup)
		R_SetupBlit();
	R_DrawQuad();
	GL_MBind(0,0);
	if (!alreadySetup)
		R_TeardownBlit();

}

void R_BlitFlipped(GLuint texture)
{
    qboolean alreadySetup = setupForBlit;
    GL_MBind(0,texture);
    
    if (!alreadySetup)
        R_SetupBlit();
    R_DrawQuadFlipped();
    GL_MBind(0,0);
    if (!alreadySetup)
        R_TeardownBlit();

}



void R_SetupGamma(float gamma) {
	glUseProgram(gammaAdjust.shader->program);
	glUniform1f(gammaAdjust.gamma_uniform, gamma);
	glUniform2f(gammaAdjust.scale_uniform, 1.0, 1.0);
	glUniform3f(gammaAdjust.mincolor_uniform, 0.0, 0.0, 0.0);
}

void R_TeardownGamma() {
	glUseProgram(0);
}

void R_BlitWithGamma(GLuint texture, float gamma)
{
	GL_MBind(0,texture);
	R_SetupGamma(gamma);
	R_DrawQuad();
	GL_MBind(0,0);
	R_TeardownGamma();
}

void R_BlitWithGammaFlipped(GLuint texture, float gamma)
{
	GL_MBind(0,texture);
	R_SetupGamma(gamma);
	R_SetupInvQuadState();
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	R_TeardownQuadState();
	GL_MBind(0,0);
	R_TeardownGamma();
}



void R_BlurFBO(float blurScale, float blendColor[4], fbo_t *source)
{
	if (blurSupported && r_blur->value && blurScale > 0.0)
	{
		GLuint width, height;
		float scale = r_blur_texscale->value;
		float size = blurScale * r_blur_radius->value;
		float weights[5] = {0, 0, 0, 0, 0};
		float xCoord, yCoord;
		int i;

		if (size <= 0)
			return;

		scale /=  R_AntialiasGetScale();

		size *= scale;

		for (i = 0 ; i < 5 ; i++)
		{

			float two_oSq = 2.0 * size;
			float temp = (1.0 / sqrt(two_oSq * M_PI)) * exp(-((i * i) / (two_oSq)));

			//			if (temp > 1)
			//				temp = 1;
			if (temp < 0)
				temp = 0;
			weights[i] = temp;
		}

		if (weights[0] >= 1)
			return;

		width = (source->width * scale);
		height = (source->height * scale);

		xCoord = width / (float) pongFBO.width;
		yCoord = height / (float) pongFBO.height;

		//		Com_Printf("Weights: %f %f %f %f %f\n",weights[0],weights[1],weights[2],weights[3],weights[4]);


		glUseProgram(blurXshader.shader->program);
		glUniform2f(blurXshader.res_uniform,width,height);
		glUniform1fv(blurXshader.weight_uniform,5,weights);
		glUniform2f(blurXshader.scale_uniform,1.0,1.0);

		GL_MBind(0,source->texture);
		GL_BindFBO(&pongFBO);
		//		GL_BindFBO(pongFBO.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);

		R_DrawQuad();

		glUseProgram(blurYshader.shader->program);
		glUniform2f(blurYshader.res_uniform,pingFBO.width,pingFBO.height);
		glUniform1fv(blurYshader.weight_uniform,5,weights);
		glUniform2f(blurYshader.scale_uniform,xCoord,yCoord);

		GL_MBind(0,pongFBO.texture);
		GL_BindFBO(&pingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);


		R_DrawQuad();


		GL_MBind(0,pingFBO.texture);

		GL_BindFBO(source);
		glViewport(0,0,source->width,source->height);

		glUseProgram(colorBlend.shader->program);
		glUniform2f(colorBlend.scale_uniform,xCoord,yCoord);
		glUniform4fv(colorBlend.blend_uniform,1,blendColor);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		R_DrawQuad();

	} 
}

void R_BloomFBO(fbo_t *source)
{
	if (bloomSupported && r_bloom->value)
	{
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

		scale /=  R_AntialiasGetScale();

		size *= scale;

		for (i = 0 ; i < 5 ; i++)
		{

			float two_oSq = 2.0 * size;
			float temp = (1.0 / sqrt(two_oSq * M_PI)) * exp(-((i * i) / (two_oSq)));

			//			if (temp > 1)
			//				temp = 1;
			if (temp < 0)
				temp = 0;
			weights[i] = temp;
		}

		if (weights[0] >= 1)
			return;

		width = (source->width * scale);
		height = (source->height * scale);

		xCoord = width / (float) pongFBO.width;
		yCoord = height / (float) pongFBO.height;



		GL_BindFBO(&pongFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);

		GL_MBind(0,source->texture);		

		glUseProgram(lowpassFilter.shader->program);
		glUniform2f(lowpassFilter.res_uniform,width,height);
		glUniform1fv(lowpassFilter.weight_uniform,5,weights);
		glUniform1f(lowpassFilter.lowpass_uniform,lowpass_threshold);
		glUniform2f(lowpassFilter.scale_uniform,1.0,1.0);
		glUniform1f(lowpassFilter.falloff_uniform,bloom_falloff);

		R_DrawQuad();

		glUseProgram(blurYshader.shader->program);
		glUniform2f(blurYshader.res_uniform,pingFBO.width,pingFBO.height);
		glUniform1fv(blurYshader.weight_uniform,5,weights);
		glUniform2f(blurYshader.scale_uniform,xCoord,yCoord);


		GL_MBind(0,pongFBO.texture);
		GL_BindFBO(&pingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,width,height);

		R_DrawQuad();

		GL_MBind(0,pingFBO.texture);

		GL_BindFBO(source);
		glViewport(0,0,source->width,source->height);

		glUseProgram(passthrough.shader->program);
		glUniform2f(passthrough.scale_uniform,xCoord,yCoord);

		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

		R_DrawQuad();

		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_Disable(GL_BLEND);

	} 
}

extern	cvar_t	*cl_paused;
extern  float	v_blend[4];			// final blending color
void R_PolyBlend ();

void R_ApplyPostProcess(fbo_t *source)
{
	fbo_t *currentFBO = glState.currentFBO;
    qboolean fog_on = glState.fog;
    
    if (fog_on)
        GL_Disable(GL_FOG);
    
	GL_ClearColor(0,0,0,0);
	R_SetupQuadState();
	GL_Disable(GL_DEPTH_TEST);

	if (source->width > pongFBO.width || source->height > pongFBO.height)
	{
		R_ResizeFBO(source->width ,source->height ,TRUE, GL_RGBA8, &pongFBO);
		R_ResizeFBO(source->width ,source->height ,TRUE, GL_RGBA8, &pingFBO);
	}



	if (r_bloom->value)
	{
		R_BloomFBO(source);
	}

    if (blurSupported && r_blur->value && r_flashblur->value) {
        if (r_flashblur->value && v_blend[3] > 0.0)
        {
            float color[4] = {v_blend[0],v_blend[1],v_blend[2],v_blend[3]*0.5};
            float weight = (cl_paused->value || (r_newrefdef.rdflags & RDF_UNDERWATER)) ? 1.0 : v_blend[3] * 0.5;
            R_BlurFBO(weight,color,source);
            
        } else if (cl_paused->value || (r_newrefdef.rdflags & RDF_UNDERWATER))
        {
            float color[4] = {0.0,0.0,0.0,0.0};
            R_BlurFBO(1,color,source);
        }
    } else if (r_polyblend->value && v_blend[3] > 0.0) {
        R_TeardownQuadState();
        R_PolyBlend();
        R_SetupQuadState();
    }
    
	if (fxaaSupported && (int) r_antialias->value >= ANTIALIAS_FXAA)
	{
		if (source->width != fxaaFBO.width || source->height != fxaaFBO.height)
		{
			R_ResizeFBO(source->width ,source->height ,TRUE, GL_RGBA8, &fxaaFBO);
		}

		glViewport(0,0,source->width,source->height);

		GL_MBind(0,source->texture);		
		glUseProgram(fxaa.shader->program);
		glUniform2f(fxaa.res_uniform,source->width,source->height);
		glUniform2f(fxaa.scale_uniform,1.0,1.0);

		GL_BindFBO(&fxaaFBO);
		GL_ClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		R_DrawQuad();

		glUseProgram(passthrough.shader->program);
		glUniform2f(passthrough.scale_uniform,1.0,1.0);
		GL_MBind(0,fxaaFBO.texture);
        
		GL_BindFBO(source);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		R_DrawQuad();

	} else {
		R_DelFBO(&fxaaFBO);
	}



	GL_Enable(GL_DEPTH_TEST);
	R_TeardownQuadState();
	R_BindFBO(currentFBO);
	GL_MBind(0,0);
	glUseProgram(0);
    if (fog_on)
        GL_Enable(GL_FOG);
}


qboolean R_InitPostsprocessShaders()
{
	qboolean success = true;
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
		glUniform2f(passthrough.scale_uniform,1.0,1.0);	
		passthrough.gamma_uniform = glGetUniformLocation(passthrough.shader->program,"gamma");			
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
		glUniform2f(colorBlend.scale_uniform,1.0,1.0);	
		colorBlend.gamma_uniform = glGetUniformLocation(colorBlend.shader->program,"gamma");
		glUseProgram(0);
		success = success && true;
	}
	else {
		success = false;
	}

	if (R_CompileShaderFromFiles(&gammaAdjust_object))
	{
		GLint texloc;
		gammaAdjust.shader = &gammaAdjust_object;
		glUseProgram(gammaAdjust.shader->program);
		texloc = glGetUniformLocation(gammaAdjust.shader->program,"tex");
		glUniform1i(texloc,0);
		gammaAdjust.blend_uniform = glGetUniformLocation(gammaAdjust.shader->program,"blendColor");
		gammaAdjust.scale_uniform = glGetUniformLocation(gammaAdjust.shader->program,"texScale");
		gammaAdjust.gamma_uniform = glGetUniformLocation(gammaAdjust.shader->program,"gamma");
		gammaAdjust.mincolor_uniform = glGetUniformLocation(gammaAdjust.shader->program,"minColor");
		glUseProgram(0);
		success = success && true;
	}
	else {
		success = false;
	}

	blitSupported = success;

	if (blitSupported)
	{
		Com_Printf("success!\n");
	} else {
		Com_Printf("failed!\n");
	}


	if (glConfig.shader_version_major > 1 || (glConfig.shader_version_major == 1 && glConfig.shader_version_minor >= 2))
	{
		if (blitSupported)
		{
			Com_Printf("...loading FXAA shader: ");
			success = true;
			if (R_CompileShaderFromFiles(&FXAA_object))
			{
				GLint texloc;
				fxaa.shader = &FXAA_object;
				glUseProgram(fxaa.shader->program);
				texloc = glGetUniformLocation(fxaa.shader->program,"Texture0");
				glUniform1i(texloc,0);
				fxaa.res_uniform = glGetUniformLocation(fxaa.shader->program,"buffersize");
				fxaa.scale_uniform = glGetUniformLocation(fxaa.shader->program,"texScale");
				glUniform2f(fxaa.scale_uniform,1.0,1.0);	
				glUseProgram(0);
				success = success && true;
			}
			else {
				success = false;
			}

			fxaaSupported = success;
			if (fxaaSupported)
			{
				Com_Printf("success!\n");
			} else {
				Com_Printf("failed!\n");
			}

			if ( !((glConfig.rendType & GLREND_INTEL) && r_driver_workarounds->value) )
			{

				Com_Printf("...loading blur shaders: ");
				success = true;
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

				blurSupported = success;
				if (blurSupported)
				{
					Com_Printf("success!\n");
				} else {
					Com_Printf("failed!\n");
				}


				Com_Printf("...loading bloom shaders: ");
				success = true;

				if (R_CompileShaderFromFiles(&lowpassFilter_object))
				{
					GLint texloc;
					lowpassFilter.shader = &lowpassFilter_object;
					glUseProgram(lowpassFilter.shader->program);
					texloc = glGetUniformLocation(lowpassFilter.shader->program,"tex");
					glUniform1i(texloc,0);
					lowpassFilter.lowpass_uniform = glGetUniformLocation(lowpassFilter.shader->program,"minThreshold");
					lowpassFilter.conversion_uniform = glGetUniformLocation(lowpassFilter.shader->program,"lumConversion");
					lowpassFilter.res_uniform = glGetUniformLocation(lowpassFilter.shader->program,"resolution");
					lowpassFilter.weight_uniform = glGetUniformLocation(lowpassFilter.shader->program,"weight");
					lowpassFilter.scale_uniform = glGetUniformLocation(lowpassFilter.shader->program,"texScale");
					lowpassFilter.falloff_uniform = glGetUniformLocation(lowpassFilter.shader->program,"falloff");
					glUseProgram(0);
					success = success && true;
				}
				else {
					success = false;
				}

				bloomSupported = success;
				if (bloomSupported)
				{
					Com_Printf("success!\n");
				} else {
					Com_Printf("failed!\n");
				}
			}
			success = bloomSupported && blurSupported && fxaaSupported;
		}
	}
	return success;
}

void R_TeardownPostprocessShaders()
{
	R_DelShaderProgram(&blurX_object);
	R_DelShaderProgram(&blurY_object);
	R_DelShaderProgram(&lowpassFilter_object);
	R_DelShaderProgram(&passthrough_object);
	R_DelShaderProgram(&colorBlend_object);
	R_DelShaderProgram(&gammaAdjust_object);
	R_DelShaderProgram(&FXAA_object);

	blitSupported = false;
	blurSupported = false;
	bloomSupported = false;
	fxaaSupported = false;
}

static qboolean firstInit = false;
void R_PostProcessInit()
{
	if (!firstInit)
	{
		firstInit = true;
		r_flashblur = Cvar_Get("r_flashblur","1",CVAR_ARCHIVE);
		r_bloom_texscale = Cvar_Get("r_bloom_texscale","0.5",CVAR_ARCHIVE);
		r_bloom_radius = Cvar_Get("r_bloom_radius","15.0",CVAR_ARCHIVE);
		r_bloom_minthreshold = Cvar_Get( "r_bloom_minthreshold", "0.15", CVAR_ARCHIVE );	// was 0.08
		r_bloom_falloff = Cvar_Get("r_bloom_falloff","1.5",CVAR_ARCHIVE);
		r_bloom = Cvar_Get( "r_bloom", "1", CVAR_ARCHIVE );	// BLOOMS
		r_blur_texscale = Cvar_Get("r_blur_texscale","1.0",CVAR_ARCHIVE);
		r_blur_radius = Cvar_Get("r_blur_radius","5.0",CVAR_ARCHIVE);
		r_blur = Cvar_Get("r_blur", "1", CVAR_ARCHIVE );
	}
	if (glConfig.ext_framebuffer_object && glConfig.arb_vertex_buffer_object)
	{
		R_InitFBO(&pongFBO);
		R_InitFBO(&pingFBO);
		R_InitFBO(&fxaaFBO);
		R_InitBuffer(&quad);
		R_CreateBuffer(&quad, GL_ARRAY_BUFFER,GL_STATIC_DRAW);
		R_BindBuffer(&quad);
		R_SetBuffer(&quad, sizeof(quad_vert_t) * 4,(void *)&quad_verts[0]);
		R_ReleaseBuffer(&quad);
		R_InitBuffer(&inv_quad);
		R_CreateBuffer(&inv_quad, GL_ARRAY_BUFFER,GL_STATIC_DRAW);
		R_BindBuffer(&inv_quad);
		R_SetBuffer(&inv_quad, sizeof(quad_vert_t) * 4,(void *)&inv_quad_verts[0]);
		R_ReleaseBuffer(&inv_quad);
		R_InitPostsprocessShaders();
	}
}

void R_PostProcessShutdown()
{
	R_DelFBO(&pongFBO);
	R_DelFBO(&pingFBO);
	R_DelFBO(&fxaaFBO);
	R_DelBuffer(&quad);
	R_TeardownPostprocessShaders();
}
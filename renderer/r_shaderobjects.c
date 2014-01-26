#include "r_local.h"

//
// Utility Functions
//


r_warpshader_t warpshader;

static r_shaderobject_t warpshader_object = {
	0,
	// vertex shader (identity)
	"varying vec4 color;"
	"void main(void) {\n"
		"color = gl_Color;\n"
		"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
		"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"gl_TexCoord[1] = gl_MultiTexCoord1;\n"
	"}\n",
	// fragment shader
	"uniform sampler2D texImage;\n"
	"uniform sampler2D texDistort;\n"
	"uniform vec4 rgbscale;\n"
	"varying vec4 color;\n"
	"void main() {\n"
		"vec2 offset;\n"
		"vec4 dist;\n"
		"vec2 coord;\n"
		"offset = texture2D(texDistort, gl_TexCoord[1].xy).zw * 0.5;\n"
		"coord = gl_TexCoord[0].xy + offset;\n"
		"dist = texture2D(texImage, coord);\n"
		"gl_FragColor = color * dist * rgbscale;\n"
	"}\n"
};


qboolean R_CompileShader(GLhandleARB shader, const char *source)
{
	GLint status;

	glShaderSourceARB(shader, 1, &source, NULL);
	glCompileShaderARB(shader);
	glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (status == 0)
	{
		GLint length;
		char *info;

		glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		info = (char *) malloc(sizeof(char) * length+1);
		glGetInfoLogARB(shader, length, NULL, info);
		VID_Printf(PRINT_ALL,S_COLOR_RED "Failed to compile shader:\n%s\n%s", source, info);
		free(info);
	}

	return !!status;
}

qboolean R_CompileShaderProgram(r_shaderobject_t *shader)
{
	int		err;
	glGetError();

	if (shader)
	{
		GLhandleARB vert_shader,frag_shader;
		shader->program = glCreateProgramObjectARB();

		vert_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		if (!R_CompileShader(vert_shader, shader->vert_source)) {
			glDeleteObjectARB(vert_shader);
			return false;
		}

		frag_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		if (!R_CompileShader(frag_shader, shader->frag_source)) {
			glDeleteObjectARB(vert_shader);
			glDeleteObjectARB(frag_shader);
			return false;
		}

		glAttachObjectARB(shader->program, vert_shader);
		glAttachObjectARB(shader->program, frag_shader);
		glLinkProgramARB(shader->program); 
		glDeleteObjectARB(vert_shader);
		glDeleteObjectARB(frag_shader);

		err = glGetError();
		if (err != GL_NO_ERROR)
		{
			VID_Printf(PRINT_ALL, "R_CompileShaderProgram: glGetError() = 0x%x\n", err);
			glDeleteObjectARB(shader->program);
			shader->program = 0;
			return false;
		}
	}

	return true;
}

void R_DelShaderProgram(r_shaderobject_t *shader)
{
	if (shader->program)
	{
		glDeleteObjectARB(shader->program);
		shader->program = 0;
	}
}


void R_ShaderObjectsInit()
{
	Com_Printf("...loading shaders: ");
	if (R_CompileShaderProgram(&warpshader_object))
	{
		GLint texloc;
		Com_Printf("success!\n");
		warpshader.shader = &warpshader_object;
		glUseProgramObjectARB(warpshader.shader->program);
		texloc = glGetUniformLocationARB(warpshader.shader->program,"texImage");
		glUniform1iARB(texloc,0);
		texloc = glGetUniformLocationARB(warpshader.shader->program,"texDistort");
		glUniform1iARB(texloc,1);
		warpshader.scale_uniform = glGetUniformLocationARB(warpshader.shader->program,"rgbscale");
		glUseProgramObjectARB(0);
	} else {
		Com_Printf("failed!\n");
	}
}

void R_ShaderObjectsShutdown()
{
	R_DelShaderProgram(&warpshader_object);
}
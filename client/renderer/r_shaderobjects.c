#include "include/r_local.h"

//
// Utility Functions
//


r_warpshader_t warpshader;

static r_shaderobject_t warpshader_object = {
	0,
	// vertex shader (identity)
	"varying vec4 color;\n"
	"varying vec4 coords;\n"
	"uniform float displacement;\n"
	"uniform vec2 scale;\n"
	"uniform float time;\n"
	"float deform(vec2 pos) {\n"
	"return sin(pos.x) * cos(pos.y);\n"
	"}\n"
	"void main(void) {\n"
		"color = gl_Color;\n"
		"vec4 vertex = gl_Vertex;\n"
		"vec2 position = vertex.xy * scale + time;\n"
		"vertex.z += displacement * deform(position);\n"
		"gl_Position = gl_ModelViewProjectionMatrix * vertex;\n"
		"coords = vec4(gl_MultiTexCoord0.xy,gl_MultiTexCoord1.xy);\n"
	"}\n",
	// fragment shader
	"uniform sampler2D texImage;\n"
	"uniform sampler2D texDistort;\n"
	"uniform vec4 rgbscale;\n"
	"varying vec4 color;\n"
	"varying vec4 coords;\n"
	"void main() {\n"
		"vec2 offset;\n"
		"vec4 dist;\n"
		"offset = texture2D(texDistort, coords.zw).zw * 0.5;\n"
		"dist = texture2D(texImage, coords.xy + offset);\n"
		"gl_FragColor = color * dist * rgbscale;\n"
	"}\n"
};

qboolean R_CompileShader(GLuint shader, const char *source)
{
	GLint status;
	int32_t		err;
	glGetError();

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == 0)
	{
		GLint length;
		char *info;
		
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		info = (char *) malloc(sizeof(char) * length+1);
		glGetShaderInfoLog(shader, length, NULL, info);
		VID_Printf(PRINT_ALL,S_COLOR_RED "Failed to compile shader:\n%s\n%s", source, info);
		free(info);
	}
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		VID_Printf(PRINT_ALL, "R_CompileShader: glGetError() = 0x%x\n", err);
	}

	return !!status;
}

qboolean R_CompileShaderProgram(r_shaderobject_t *shader)
{
	int32_t		err;
	qboolean success = false;
	glGetError();

	if (shader)
	{
		GLuint vert_shader,frag_shader, program;
		GLint status;

		program = glCreateProgram();
		vert_shader = glCreateShader(GL_VERTEX_SHADER);
		frag_shader = glCreateShader(GL_FRAGMENT_SHADER);


		success = R_CompileShader(vert_shader, shader->vert_source);
		success = success && R_CompileShader(frag_shader, shader->frag_source);

		if (success)
		{
			glAttachShader(program, vert_shader);
			glAttachShader(program, frag_shader);
			glLinkProgram(program); 

			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				VID_Printf(PRINT_ALL, "R_CompileShaderProgram: glGetError() = 0x%x\n", err);
			}

			glGetProgramiv(program, GL_LINK_STATUS, &status);
			if (status == 0)
			{
				GLint length;
				char *info;

				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
				info = (char *) malloc(sizeof(char) * length+1);
				glGetProgramInfoLog(program, length, NULL, info);
				VID_Printf(PRINT_ALL,S_COLOR_RED "Failed to link program:\n%s", info);
				free(info);		
				success = false;
			} 		
		}

		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);

		if (success)
			shader->program = program;
		else
			glDeleteProgram(program);

	}

	return success;
}

void R_DelShaderProgram(r_shaderobject_t *shader)
{
	if (shader->program)
	{
		
		glDeleteProgram(shader->program);
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
		glUseProgram(warpshader.shader->program);
		texloc = glGetUniformLocation(warpshader.shader->program,"texImage");
		glUniform1i(texloc,0);
		texloc = glGetUniformLocation(warpshader.shader->program,"texDistort");
		glUniform1i(texloc,1);
		warpshader.rgbscale_uniform = glGetUniformLocation(warpshader.shader->program,"rgbscale");
		warpshader.scale_uniform = glGetUniformLocation(warpshader.shader->program,"scale");
		warpshader.time_uniform = glGetUniformLocation(warpshader.shader->program,"time");
		warpshader.displacement_uniform = glGetUniformLocation(warpshader.shader->program,"displacement");
		
		glUseProgram(0);
	} else {
		Com_Printf("failed!\n");
	}
}

void R_ShaderObjectsShutdown()
{
	R_DelShaderProgram(&warpshader_object);
}
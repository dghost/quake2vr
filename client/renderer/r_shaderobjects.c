#include "include/r_local.h"

//
// Utility Functions
//

#define SHADER_DIR "shaders"

r_warpshader_t warpshader;
r_causticshader_t causticshader;

static r_shaderobject_t warpshader_object = {
	0,
	// vertex shader (identity)
	"warp.vert",
	// fragment shader
	"warp.frag"
};

static r_shaderobject_t causticshader_object = {
	0,
	// vertex shader (identity)
	"caustic.vert",
	// fragment shader
	"warp.frag"
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

char* R_LoadShader(char*path)
{
	char *buffer;
	int size;
	size = FS_LoadFile(path,(void **) &buffer);
	if (buffer)
	{
		char *temp = (void *) Z_Malloc(size + 2);
		memcpy(temp,buffer,size);
		temp[size]='\n';
		temp[size+1]= 0;
		Z_Free(buffer);
		return temp;
	}
	return NULL;
}

qboolean R_CompileShaderFromFiles(r_shaderobject_t *shader)
{
	int32_t		err;
	qboolean success = false;
	glGetError();


	if (shader)
	{
		GLuint vert_shader,frag_shader, program;
		GLint status;
		char buffer[MAX_OSPATH];
		char *fragBuffer = NULL,*vertBuffer = NULL;
		int size = 0;
		program = glCreateProgram();
		vert_shader = glCreateShader(GL_VERTEX_SHADER);
		frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

		Com_sprintf (buffer, sizeof(buffer), "%s/%s", SHADER_DIR, shader->vert_source);
		vertBuffer = R_LoadShader(buffer);

		if (vertBuffer)
		{
			success = R_CompileShader(vert_shader, (const char *) vertBuffer);
			Z_Free(vertBuffer);
		}

		Com_sprintf (buffer, sizeof(buffer), "%s/%s", SHADER_DIR, shader->frag_source);
		fragBuffer = R_LoadShader(buffer);
		
		if (fragBuffer)
		{
			success = success && R_CompileShader(frag_shader, (const char *) fragBuffer);
			Z_Free(fragBuffer);
		}

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
	qboolean success = false;
	Com_Printf("...loading shaders: ");

	if (R_CompileShaderFromFiles(&warpshader_object))
	{
		GLint texloc;
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

		success = true;
	}

	if (R_CompileShaderFromFiles(&causticshader_object))
	{
		GLint texloc;
		causticshader.shader = &warpshader_object;
		glUseProgram(causticshader.shader->program);
		texloc = glGetUniformLocation(causticshader.shader->program,"texImage");
		glUniform1i(texloc,0);
		texloc = glGetUniformLocation(causticshader.shader->program,"texDistort");
		glUniform1i(texloc,1);
		causticshader.rgbscale_uniform = glGetUniformLocation(causticshader.shader->program,"rgbscale");
		glUseProgram(0);
		success = success && true;
	}


	if (success)
	{
		Com_Printf("success!\n");
	} else {
		Com_Printf("failed!\n");
	}
}

void R_ShaderObjectsShutdown()
{
	R_DelShaderProgram(&warpshader_object);
}
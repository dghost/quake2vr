#include "r_local.h"

//
// Utility Functions
//

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
		shader->program = glCreateProgramObjectARB();

		shader->vert_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		if (!R_CompileShader(shader->vert_shader, shader->vert_source)) {
			return false;
		}

		shader->frag_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		if (!R_CompileShader(shader->frag_shader, shader->frag_source)) {
			return false;
		}

		glAttachObjectARB(shader->program, shader->vert_shader);
		glAttachObjectARB(shader->program, shader->frag_shader);
		glLinkProgramARB(shader->program); 
	}
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_CompileShaderProgram: glGetError() = 0x%x\n", err);


	return true;
}

void R_DelShaderProgram(r_shaderobject_t *shader)
{
	if (shader->program)
	{
		glDeleteObjectARB(shader->program);
		shader->program = 0;
	}
	if (shader->frag_shader)
	{
		glDeleteObjectARB(shader->frag_shader);
		shader->frag_shader = 0;
	}

	if (shader->vert_shader)
	{
		glDeleteObjectARB(shader->vert_shader);
		shader->vert_shader = 0;
	}
}

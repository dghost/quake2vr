#include "r_local.h"

//
// Utility Functions
//

qboolean R_CompileShader(GLhandleARB shader, const char *source)
{
	GLint status;

	qglShaderSourceARB(shader, 1, &source, NULL);
	qglCompileShaderARB(shader);
	qglGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (status == 0)
	{
		GLint length;
		char *info;

		qglGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		info = (char *) malloc(sizeof(char) * length+1);
		qglGetInfoLogARB(shader, length, NULL, info);
		VID_Printf(PRINT_ALL,S_COLOR_RED "Failed to compile shader:\n%s\n%s", source, info);
		free(info);
	}

	return !!status;
}

qboolean R_CompileShaderProgram(r_shaderobject_t *shader)
{
	qglGetError();
	if (shader)
	{
		shader->program = qglCreateProgramObjectARB();

		shader->vert_shader = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		if (!R_CompileShader(shader->vert_shader, shader->vert_source)) {
			return false;
		}

		shader->frag_shader = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		if (!R_CompileShader(shader->frag_shader, shader->frag_source)) {
			return false;
		}

		qglAttachObjectARB(shader->program, shader->vert_shader);
		qglAttachObjectARB(shader->program, shader->frag_shader);
		qglLinkProgramARB(shader->program); 
	}
	return (qglGetError() == GL_NO_ERROR);
}

void R_DelShaderProgram(r_shaderobject_t *shader)
{
	if (shader->program)
	{
		qglDeleteObjectARB(shader->program);
		shader->program = 0;
	}
	if (shader->frag_shader)
	{
		qglDeleteObjectARB(shader->frag_shader);
		shader->frag_shader = 0;
	}

	if (shader->vert_shader)
	{
		qglDeleteObjectARB(shader->vert_shader);
		shader->vert_shader = 0;
	}
}

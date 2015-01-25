#include "include/r_local.h"

void R_SetAttribsVBO(attribs_t *attribs, uint32_t count)
{
	uint32_t i;
	for (i = 0; i < count; i++)
	{
		attribs_t *a = &attribs[i];

		glVertexAttribPointer(a->index, a->size,  a->type,  a->normalized,  a->stride,  (GLvoid *) a->offset);
	}
}


int32_t R_CreateBuffer(buffer_t *buffer, GLenum target, GLenum usage)
{
	if (!buffer->handle)
	{
		glGenBuffers(1,&buffer->handle);
	}
	buffer->target = target;
	buffer->usage = usage;
	return 1;
}

void R_SetBuffer(buffer_t *buffer, GLsizeiptr size,  const GLvoid * data)
{
	if (buffer->handle)
	{
		glBufferData(buffer->target,  size, data,  buffer->usage);
	}
}

void R_BindBuffer(buffer_t *buffer)
{
	glBindBuffer(buffer->target, buffer->handle);
}

void R_ReleaseBuffer(buffer_t *buffer)
{
	glBindBuffer(buffer->target, 0);
}

void R_DelBuffer(buffer_t *buffer)
{
	if (buffer->handle)
	{
		glDeleteBuffers(1,&buffer->handle);
	}
	memset(buffer,0,sizeof(buffer_t));
}

void R_InitBuffer(buffer_t *buffer)
{
	memset(buffer,0,sizeof(buffer_t));
}


void R_InitIVBO(vbo_t *buffer)
{
	memset(buffer,0,sizeof(vbo_t));
}

void R_CreateIVBO(vbo_t *buffer, GLenum usage)
{
	if (!buffer->handles[0])
	{
		glGenBuffers(2,buffer->handles);
	}
	buffer->usage = usage;
}

void R_VertexData(vbo_t *buffer, GLsizeiptr size,  const GLvoid * data)
{
	if (buffer->handles[0])
	{
		glBufferData(GL_ARRAY_BUFFER, size, data,  buffer->usage);
	}
}

void R_IndexData(vbo_t *buffer, GLenum mode, GLenum type, GLsizei count, GLsizeiptr size, const GLvoid * data)
{
	if (buffer->handles[1])
	{
		buffer->mode = mode;
		buffer->type = type;
		buffer->count = count;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data,  buffer->usage);
	}
}

void R_BindIVBO(vbo_t *buffer, attribs_t *attribs, uint32_t attribCount)
{
	if (buffer->handles[0])
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffer->handles[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->handles[1]);
		if (attribs && attribCount)
			R_SetAttribsVBO(attribs,attribCount);
	}
}

void R_ReleaseIVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

void R_DrawIVBO(vbo_t *buffer)
{
	if (buffer->handles[0])
	{
		glDrawElements(buffer->mode,buffer->count,buffer->type,(void*)0);
	}
}

void R_DrawRangeIVBO(vbo_t *buffer, GLvoid *offset, GLsizei count)
{
	if (buffer->handles[0])
	{
		glDrawElements(buffer->mode,count,buffer->type,offset);
	}
}

void R_DelIVBO(vbo_t *buffer)
{
	if (buffer->handles[0])
		glDeleteBuffers(2,buffer->handles);
	memset(buffer,0,sizeof(vbo_t));
}
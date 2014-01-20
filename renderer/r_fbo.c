#include "r_local.h"

int R_GenFBO(int width, int height, fbo_t *FBO)
{
	GLuint fbo, tex, dep;
	int err;
	glGetError();

	glGenFramebuffersEXT(1, &fbo);
	glGenTextures(1, &tex);
	glGenRenderbuffersEXT(1, &dep);
	GL_SelectTexture(0);
	GL_Bind(tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_Bind(0);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: Texture creation: glGetError() = 0x%x\n", err);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, dep);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, width, height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: Depth buffer creation: glGetError() = 0x%x\n", err);
	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("ERROR: Creation of %i x %i FBO failed!\n", width, height);

		glDeleteTextures(1, &tex);
		glDeleteRenderbuffersEXT(1, &dep);
		glDeleteFramebuffersEXT(1, &fbo);
		return 0;
	}
	else {
		int err;
		err = glGetError();
		if (err != GL_NO_ERROR)
			VID_Printf(PRINT_ALL, "R_GenFBO: glGetError() = 0x%x\n", err);

		FBO->framebuffer = fbo;
		FBO->texture = tex;
		FBO->depthbuffer = dep;
		FBO->width = width;
		FBO->height = height;
		FBO->valid = 1;
		return 1;
	}
}

int R_ResizeFBO(int width, int height, fbo_t *FBO)
{
	int err;
	
	
	if (!FBO->valid)
	{
		return R_GenFBO(width, height, FBO);
	}

	glGetError();

	GL_SelectTexture(0);
	GL_Bind(FBO->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_Bind(0);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_ResizeFBO: Texture resize: glGetError() = 0x%x\n", err);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, FBO->depthbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, width, height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	err = glGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_ResizeFBO: Depth buffer resize: glGetError() = 0x%x\n", err);
	FBO->width = width;
	FBO->height = height;

	/*
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, FBO->framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("ERROR: Creation of %i x %i FBO failed!\n", width, height);

		glDeleteTextures(1, &tex);
		glDeleteRenderbuffers(1, &dep);
		glDeleteFramebuffers(1, &fbo);
		return 0;
	}
	else {
		FBO->framebuffer = fbo;
		FBO->texture = tex;
		FBO->depthbuffer = dep;
		FBO->width = width;
		FBO->height = height;
		FBO->valid = 1;
		return 1;
	}
	*/

	return 1;
}

void R_DelFBO(fbo_t *FBO)
{
	glDeleteFramebuffersEXT(1, &FBO->framebuffer);
	glDeleteTextures(1, &FBO->texture);
	glDeleteRenderbuffersEXT(1, &FBO->depthbuffer);
	
	FBO->framebuffer = 0;
	FBO->texture = 0;
	FBO->depthbuffer = 0;

	FBO->height = 0;
	FBO->width = 0;
	FBO->valid = 0;

}

void R_InitFBO(fbo_t *FBO)
{
	FBO->framebuffer = 0;
	FBO->texture = 0;
	FBO->depthbuffer = 0;
	FBO->height = 0;
	FBO->width = 0;
	FBO->valid = 0;

}

GLuint R_BindFBO(fbo_t *FBO)
{
	GLint currentFrameBuffer = 0;
	//glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFrameBuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FBO->framebuffer);
	glViewport(0, 0, FBO->width, FBO->height);
	return (GLuint) currentFrameBuffer;
}

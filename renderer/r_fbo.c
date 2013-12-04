#include "r_local.h"

int R_GenFBO(int width, int height, fbo_t *FBO)
{
	GLuint fbo, tex, dep;
	int err;
	qglGetError();

	qglGenFramebuffers(1, &fbo);
	qglGenTextures(1, &tex);
	qglGenRenderbuffers(1, &dep);
	GL_SelectTexture(0);
	GL_Bind(tex);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_Bind(0);
	err = qglGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: Texture creation: glGetError() = 0x%x\n", err);
	qglBindRenderbuffer(GL_RENDERBUFFER_EXT, dep);
	qglRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, width, height);
	qglBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);

	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
	qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	err = qglGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_GenFBO: Depth buffer creation: glGetError() = 0x%x\n", err);
	if (qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("ERROR: Creation of %i x %i FBO failed!\n", width, height);

		qglDeleteTextures(1, &tex);
		qglDeleteRenderbuffers(1, &dep);
		qglDeleteFramebuffers(1, &fbo);
		return 0;
	}
	else {
		int err;
		err = qglGetError();
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

	qglGetError();

	GL_SelectTexture(0);
	GL_Bind(FBO->texture);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GL_Bind(0);
	err = qglGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_ResizeFBO: Texture resize: glGetError() = 0x%x\n", err);

	qglBindRenderbuffer(GL_RENDERBUFFER_EXT, FBO->depthbuffer);
	qglRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, width, height);
	qglBindRenderbuffer(GL_RENDERBUFFER_EXT, 0);
	err = qglGetError();
	if (err != GL_NO_ERROR)
		VID_Printf(PRINT_ALL, "R_ResizeFBO: Depth buffer resize: glGetError() = 0x%x\n", err);
	FBO->width = width;
	FBO->height = height;
	/*
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, FBO->framebuffer);
	qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, dep);

	if (qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("ERROR: Creation of %i x %i FBO failed!\n", width, height);

		qglDeleteTextures(1, &tex);
		qglDeleteRenderbuffers(1, &dep);
		qglDeleteFramebuffers(1, &fbo);
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
	qglDeleteFramebuffers(1, &FBO->framebuffer);
	qglDeleteTextures(1, &FBO->texture);
	qglDeleteRenderbuffers(1, &FBO->depthbuffer);
	
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
	//qglGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFrameBuffer);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, FBO->framebuffer);
	qglViewport(0, 0, FBO->width, FBO->height);
	return (GLuint) currentFrameBuffer;
}

#include "r_local.h"

int R_GenFBO(int width, int height, fbo_t *FBO)
{


	GLuint fbo, tex, dep;

	qglGenFramebuffers(1, &fbo);
	qglGenTextures(1, &tex);
	qglGenRenderbuffers(1, &dep);
	GL_SelectTexture(0);
	GL_Bind(tex);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	GL_Bind(0);

	qglBindRenderbuffer(GL_RENDERBUFFER, dep);
	qglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	qglBindRenderbuffer(GL_RENDERBUFFER, 0);

	qglBindFramebuffer(GL_FRAMEBUFFER, fbo);
	qglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dep);


	if (qglCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
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
}


void R_DelFBO(fbo_t *FBO)
{
	qglDeleteFramebuffers(1, &FBO->framebuffer);
	qglDeleteTextures(1, &FBO->texture);
	qglDeleteRenderbuffers(1, &FBO->depthbuffer);

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
	qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFrameBuffer);
	qglBindFramebuffer(GL_FRAMEBUFFER, FBO->framebuffer);
	qglViewport(0, 0, FBO->width, FBO->height);
	return (GLuint) currentFrameBuffer;
}

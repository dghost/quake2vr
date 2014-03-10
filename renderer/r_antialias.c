#include "r_local.h"

cvar_t *r_antialias;
fbo_t offscreen;
viddef_t offscreenBounds;
viddef_t screenBounds;

static Sint32 offscreenStale = 1;

void R_AntialiasStartFrame (void)
{
	if (r_antialias->modified)
	{
		Sint32 aa = r_antialias->value;
		if (aa >= NUM_ANTIALIAS_MODES)
			aa = NUM_ANTIALIAS_MODES - 1;
		else if (aa < 0)
			aa = 0;

		Cvar_SetInteger("r_antialias", aa);

		r_antialias->modified = false;
		
		if (!glConfig.ext_framebuffer_object)
		{
			Cvar_SetInteger("r_antialias",ANTIALIAS_NONE);
			return;
		}

		switch (aa)
		{
		case ANTIALIAS_4X_FSAA:
			offscreenBounds.width = screenBounds.width * 2;
			offscreenBounds.height = screenBounds.height * 2;
			R_ResizeFBO(offscreenBounds.width, offscreenBounds.height, 1, &offscreen);
			break;
		default:
		case ANTIALIAS_NONE:
			offscreenBounds.width = screenBounds.width;
			offscreenBounds.height = screenBounds.height;
			R_DelFBO(&offscreen);
			break;
		}
		Com_Printf("Antialias render target size %ux%u\n",offscreenBounds.width, offscreenBounds.height);

		VID_NewWindow(offscreenBounds.width,offscreenBounds.height);
	}

	offscreenStale = 1;
	vid.width = offscreenBounds.width;
	vid.height = offscreenBounds.height;
}

void R_AntialiasBind(void)
{
	if (r_antialias->value)
	{
		R_BindFBO(&offscreen);
		vid.width = offscreen.width;
		vid.height = offscreen.height;
		if (offscreenStale)
		{
			offscreenStale = 0;
			glClearColor(0.0,0.0,0.0,0.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glClearColor (1,0, 0.5, 0.5);
		}
	} 
}

void R_AntialiasEndFrame(void)
{

	if (r_antialias->value)
	{
		GL_BindFBO(0);
		glViewport(0,0,screenBounds.width,screenBounds.height);
		//GL_SelectTexture(0);

		GL_Disable(GL_DEPTH_TEST);
		GL_Disable(GL_ALPHA_TEST);

		GL_SetIdentity(GL_PROJECTION);
		GL_SetIdentity(GL_MODELVIEW);

		GL_Bind(offscreen.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();

		GL_Bind(0);
	}
}


void R_InitAntialias(void)
{
	r_antialias = Cvar_Get("r_antialias","0",CVAR_ARCHIVE);
	R_InitFBO(&offscreen);
	screenBounds.width = vid.width;
	screenBounds.height = vid.height;
	Com_Printf("Screen size %ux%u\n",screenBounds.width, screenBounds.height);
	r_antialias->modified = true; 
	R_AntialiasStartFrame();
}
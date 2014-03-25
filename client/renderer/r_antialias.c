#include "include/r_local.h"

cvar_t *r_antialias;
static fbo_t offscreen;
static viddef_t offscreenBounds;
static viddef_t screenBounds;

static int32_t offscreenStale = 1;
static GLuint currentFBO;
static qboolean changed = false;

void R_AntialiasStartFrame (void)
{
	if (r_antialias->modified)
	{
		int32_t aa = r_antialias->value;
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
		changed = true;
	}
}

void R_AntialiasBind(void)
{

	currentFBO = glState.currentFBO;
	if (changed || vid.width != screenBounds.width || vid.height != screenBounds.height)
	{
		screenBounds.width = vid.width;
		screenBounds.height = vid.height;
		switch ((int) r_antialias->value)
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
		changed = false;
	}

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
		GL_BindFBO(currentFBO);
		glViewport(0,0,screenBounds.width,screenBounds.height);
		//GL_SelectTexture(0);

		GL_Disable(GL_DEPTH_TEST);
		GL_Disable(GL_ALPHA_TEST);

		GL_SetIdentity(GL_PROJECTION);
		GL_SetIdentity(GL_MODELVIEW);

		GL_MBind(0,offscreen.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();

		GL_MBind(0,0);
		vid.width = screenBounds.width;
		vid.height = screenBounds.height;
		
	}

	offscreenStale = 1;
}


void R_AntialiasInit(void)
{
	r_antialias = Cvar_Get("r_antialias","0",CVAR_ARCHIVE);
	R_InitFBO(&offscreen);
	screenBounds.width = 0;
	screenBounds.height = 0;
	r_antialias->modified = true; 
	R_AntialiasStartFrame();
}

void R_AntialiasShutdown(void)
{
	if (offscreen.valid)
		R_DelFBO(&offscreen);

	vid.width = screenBounds.width;
	vid.height = screenBounds.height;
}
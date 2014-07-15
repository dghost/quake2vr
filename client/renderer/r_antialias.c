#include "include/r_local.h"

cvar_t *r_antialias;
static viddef_t offscreenBounds;
static viddef_t screenBounds;

static GLuint currentFBO;
static qboolean changed = false;

void R_AntialiasStartFrame (void)
{
	if (r_antialias->modified)
	{
		int32_t aa = r_antialias->value;
		if (!glConfig.ext_framebuffer_object)
			aa = ANTIALIAS_NONE;
		else if (aa >= NUM_ANTIALIAS_MODES)
			aa = NUM_ANTIALIAS_MODES - 1;
		else if (aa < 0)
			aa = 0;

		Cvar_SetInteger("r_antialias", aa);

		r_antialias->modified = false;
		
		changed = true;
	}

	if (changed || glConfig.screen_width != screenBounds.width || glConfig.screen_height != screenBounds.height)
	{
		screenBounds.width = glConfig.screen_width;
		screenBounds.height = glConfig.screen_height;
		switch ((int) r_antialias->value)
		{
		case ANTIALIAS_4X_FSAA:
			offscreenBounds.width = screenBounds.width * 2;
			offscreenBounds.height = screenBounds.height * 2;
			break;
		default:
		case ANTIALIAS_NONE:
			offscreenBounds.width = screenBounds.width;
			offscreenBounds.height = screenBounds.height;
			break;
		}
		VID_NewWindow(offscreenBounds.width, offscreenBounds.height);
		changed = false;
	}
}

void R_AntialiasSetFBOSize(fbo_t *fbo)
{
	if ( fbo->width != offscreenBounds.width || fbo->height != offscreenBounds.height)
	{
		Com_Printf("Antialias render target size %ux%u\n",offscreenBounds.width, offscreenBounds.height);
		R_ResizeFBO(offscreenBounds.width, offscreenBounds.height, 1, GL_RGBA8, fbo);
	}
}

void R_AntialiasInit(void)
{
	r_antialias = Cvar_Get("r_antialias","0",CVAR_ARCHIVE);
	r_antialias->modified = true; 
	R_AntialiasStartFrame();
}

void R_AntialiasShutdown(void)
{

}
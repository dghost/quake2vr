#include "r_local.h"
#include "r_vr_ovr.h"
#include "../vr/vr.h"


static fbo_t offscreen, hud; 

static hmd_render_t vr_render_none = 
{
	HMD_NONE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static hmd_render_t available_hmds[NUM_HMD_TYPES];

static hmd_render_t *hmd;

static int leftStale, rightStale, hudStale;

static vr_rect_t hudRect;
//
// Rendering related functions
//

// executed once per frame
void R_VR_StartFrame()
{
	float ipd = vr_autoipd->value ? vrConfig.ipd / 2.0 : (vr_ipd->value / 2000.0);
	int resolutionChanged = 0;
	
	if (!hmd)
		return;
	
	vrState.viewOffset = ipd * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;

	if (vid.width != vrState.viewWidth || vid.height != vrState.viewHeight)
	{
		vrState.viewHeight = vid.height;
		vrState.viewWidth = vid.width;
		resolutionChanged = true;		
	}

	hmd->frameStart(resolutionChanged);

	if (resolutionChanged)
	{
		Com_Printf("VR: Calculated %.2f FOV\n", vrState.viewFovY);
	}

	leftStale = 1;
	rightStale = 1;
	hudStale = 1;

}

void R_VR_BindView(vr_eye_t eye)
{
	int clear = 0;
	vrState.eye = eye;
	if (eye == EYE_HUD)
	{
		R_BindFBO(&hud);
		vid.height = hud.height;
		vid.width = hud.width;
		clear = hudStale;
		hudStale = 0;
	}
	else if (hmd) 
	{
		hmd->bindView(eye);
		if (eye == EYE_LEFT)
		{
			clear = leftStale;
			leftStale = 0;
		} else if (eye == EYE_RIGHT)
		{
			clear = rightStale;
			rightStale = 0;
		}
	}

	if (clear)
	{
		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClearColor (1,0, 0.5, 0.5);
	}
}

void R_VR_GetViewPos(vr_eye_t eye, int *x, int *y)
{
	if (eye == EYE_HUD)
	{
		*x = 0;
		*y = 0;
	} else if (hmd)
	{
		vr_rect_t view;
		hmd->getViewRect(eye,&view);
		*x = view.x;
		*y = view.y;
	}
}

void R_VR_GetViewSize(vr_eye_t eye, int *width, int *height)
{
	if (eye == EYE_HUD)
	{
		*width = hud.width;
		*height = hud.height;
	} else if (hmd)
	{
		vr_rect_t view;
		hmd->getViewRect(eye,&view);
		*width = view.width;
		*height = view.height;
	}
}

void R_VR_GetViewRect(vr_eye_t eye, vr_rect_t *rect)
{
	if (eye == EYE_HUD)
	{
		*rect = hudRect;
	} else if (hmd)
		hmd->getViewRect(eye,rect);
}

void R_VR_CurrentViewPosition(int *x, int *y)
{
	R_VR_GetViewPos(vrState.eye,x,y);
}

void R_VR_CurrentViewSize(int *width, int *height)
{
	R_VR_GetViewSize(vrState.eye,width,height);
}

void R_VR_CurrentViewRect(vr_rect_t *rect)
{
	R_VR_GetViewRect(vrState.eye,rect);
}

void R_VR_Rebind()
{
	if (hmd) 
		hmd->bindView(vrState.eye);
}

void R_VR_EndFrame()
{
	if (vr_enabled->value)
	{
		GL_Disable(GL_DEPTH_TEST);
		GL_Enable (GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER,0.0f);
		GL_SelectTexture(0);


		R_VR_BindView(EYE_LEFT);
		R_VR_DrawHud(EYE_LEFT);

		R_VR_BindView(EYE_RIGHT);
		R_VR_DrawHud(EYE_RIGHT);

//		GL_Bind(0);

		GL_Disable(GL_ALPHA_TEST);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		glViewport(0,0,vrState.viewWidth,vrState.viewHeight);
		vid.width = vrState.viewWidth;
		vid.height = vrState.viewHeight;
	}
}

extern void vrPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar, GLdouble offset);

void R_VR_DrawHud(vr_eye_t eye)
{
	float fov = vr_hud_fov->value;
	float y,x;
	float depth = vr_hud_depth->value;
	float ipd = vr_autoipd->value ? vrConfig.ipd / 2.0 : (vr_ipd->value / 2000.0);

	extern int scr_draw_loading;

	if (!vr_enabled->value)
		return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	vrPerspective(vrState.viewFovY, vrConfig.aspect, 0.24, 251.0, eye * vrState.projOffset);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// disable this for the loading screens since they are not at 60fps
	if ((vr_hud_bounce->value > 0) && !scr_draw_loading && ((int) vr_aimmode->value > 0))
	{
		// load the quaternion directly into a rotation matrix
		vec4_t q;
		vec4_t mat[4];
		VR_GetOrientationEMAQuat(q);
		q[2] = -q[2];
		QuatToRotation(q,mat);
		glMultMatrixf((const GLfloat *) mat);


		/* depriciated as of 9/30/2013 - consider removing later
		// convert to euler angles and rotate
		vec3_t orientation;
		VR_GetOrientationEMA(orientation);
		glRotatef (orientation[0],  1, 0, 0);
		// y axis is inverted between Quake II and OpenGL
		glRotatef (-orientation[1],  0, 1, 0);
		glRotatef (orientation[2],  0, 0, 1);
		*/
	}

	glTranslatef(eye * -ipd,0,0);
	// calculate coordinates for hud
	x = tanf(fov * (M_PI/180.0f) * 0.5) * (depth);
	y = x / ((float) hud.width / hud.height);



	if (vr_hud_transparency->value)
	{
		GL_Enable(GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	} 
	GL_Bind(hud.texture);


	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f (0, 0); glVertex3f (-x, -y,-depth);
	glTexCoord2f (0, 1); glVertex3f (-x, y,-depth);		
	glTexCoord2f (1, 0); glVertex3f (x, -y,-depth);
	glTexCoord2f (1, 1); glVertex3f (x, y,-depth);
	glEnd();

	GL_Disable(GL_BLEND);
}

// takes the various FBO's and renders them to the framebuffer
void R_VR_Present()
{
	if (!hmd)
		return;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// tell the HMD renderer to draw composited scene
	hmd->present();

	// flag HMD to refresh next frame
	vrState.stale = 1;
}



// enables renderer support for the Rift
void R_VR_Enable()
{
	qboolean success = false;
	vrState.viewHeight = 0;
	vrState.viewWidth = 0;

	if (hud.valid)
		R_DelFBO(&hud);
	if (offscreen.valid)
		R_DelFBO(&offscreen);

	leftStale = 1;
	rightStale = 1;
	hudStale = 1;

	hudRect.x = 0;
	hudRect.y = 0;
	hudRect.width = 640;
	hudRect.height = 480;

	Com_Printf("VR: Initializing renderer:");

	success = (qboolean) R_GenFBO(hudRect.width, hudRect.height, 1, &hud);

	success = success && hmd->enable();
	if (!success)
	{
		Com_Printf(" failed!\n");
		Cmd_ExecuteString("vr_disable");
	} else {
		Com_Printf(" ok!\n");
	}
	R_VR_StartFrame();
}

// disables renderer support for the Rift
void R_VR_Disable()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	glViewport(0,0,vrState.viewWidth,vrState.viewHeight);

	vid.width = vrState.viewWidth;
	vid.height = vrState.viewHeight;

	vrState.pixelScale = 1.0;

	hmd->disable();
	if (hud.valid)
		R_DelFBO(&hud);
}



// launch-time initialization for Rift support
void R_VR_Init()
{
	if (glConfig.ext_framebuffer_object && glConfig.ext_packed_depth_stencil)
	{

		R_InitFBO(&hud);
		R_InitFBO(&offscreen);

		// TODO: conditional this shit up

		available_hmds[HMD_NONE] = vr_render_none;
		available_hmds[HMD_RIFT] = vr_render_ovr;

		hmd = &available_hmds[HMD_RIFT];

		hmd->init();
		vrState.eye = EYE_HUD;

		if (vr_enabled->value)
			R_VR_Enable();

	} else {
		if (vr_enabled->value)
		{
			VR_Disable();
			Cvar_ForceSet("vr_enabled","0");
		}
	}
}

// called when the renderer is shutdown
void R_VR_Shutdown()
{
	if (vr_enabled->value)
		R_VR_Disable();


	if (hud.valid)
		R_DelFBO(&hud);
}
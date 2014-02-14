#include "r_local.h"
#include "../vr/vr.h"
#include "r_vr_ovr.h"
#include "r_vr_svr.h"

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

static vr_eye_t currenteye;

vr_rect_t screen;

//
// Rendering related functions
//

// executed once per frame
void R_VR_StartFrame()
{
	int resolutionChanged = 0;
	
	if (!hmd)
		return;
	
	if (vid.width != screen.width || vid.height != screen.height)
	{
		screen.height = vid.height;
		screen.width = vid.width;
		resolutionChanged = true;		
	}


	hmd->frameStart(resolutionChanged);
	hmd->getState(&vrState);

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
	currenteye = eye;
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
		rect->x = 0;
		rect->y = 0;
		rect->width = hud.width;
		rect->height = hud.height;
	} else if (hmd)
		hmd->getViewRect(eye,rect);
}

void R_VR_CurrentViewPosition(int *x, int *y)
{
	R_VR_GetViewPos(currenteye,x,y);
}

void R_VR_CurrentViewSize(int *width, int *height)
{
	R_VR_GetViewSize(currenteye,width,height);
}

void R_VR_CurrentViewRect(vr_rect_t *rect)
{
	R_VR_GetViewRect(currenteye,rect);
}

void R_VR_Rebind()
{
	if (hmd) 
		hmd->bindView(currenteye);
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

		GL_BindFBO(0);
		glViewport(0,0,screen.width,screen.height);
		vid.width = screen.width ;
		vid.height = screen.height;
	}
}

void R_VR_Perspective(double fovy, double aspect, double zNear, double zFar)
{
	// if the current eye is set to either left or right use the HMD aspect ratio, otherwise use what was passed to it
	R_PerspectiveOffset(fovy, (currenteye) ? vrState.aspect : aspect, zNear, zFar, currenteye * vrState.projOffset);
}


void R_VR_GetFOV(float *fovx, float *fovy)
{
	if (!hmd)
		return;

	*fovx = vrState.viewFovX * vr_autofov_scale->value;
	*fovy = vrState.viewFovY * vr_autofov_scale->value;
}


void R_VR_DrawHud(vr_eye_t eye)
{
	float fov = vr_hud_fov->value;
	float y,x;
	float depth = vr_hud_depth->value;
	float ipd = vr_autoipd->value ? vrState.ipd / 2.0 : (vr_ipd->value / 2000.0);

	extern int scr_draw_loading;

	if (!vr_enabled->value)
		return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	R_PerspectiveOffset(vrState.viewFovY, vrState.aspect, 0.24, 251.0, eye * vrState.projOffset);

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

}



// enables renderer support for the Rift
void R_VR_Enable()
{
	qboolean success = false;
	screen.width = 0;
	screen.height = 0;

	leftStale = 1;
	rightStale = 1;
	hudStale = 1;

	if (glConfig.ext_framebuffer_object && glConfig.ext_packed_depth_stencil)
	{
		Com_Printf("VR: Initializing renderer:");

		// TODO: conditional this shit up
		if (hud.valid)
			R_DelFBO(&hud);
		if (offscreen.valid)
			R_DelFBO(&offscreen);

		hmd = &available_hmds[(int) vr_enabled->value];

		success = (qboolean) R_GenFBO(640, 480, 1, &hud);
		success = success && hmd->enable();

		if (!success)
		{
			Com_Printf(" failed!\n");
			Cmd_ExecuteString("vr_disable");
		} else {
			Com_Printf(" ok!\n");
		}

	} else {
		Com_Printf("VR: Cannot initialize renderer due to missing OpenGL extensions\n");
		if (vr_enabled->value)
		{
			Cmd_ExecuteString("vr_disable");
		}
	}



	R_VR_StartFrame();
}

// disables renderer support for the Rift
void R_VR_Disable()
{
	GL_BindFBO(0);
	glViewport(0,0,screen.width,screen.height);

	vid.width = screen.width;
	vid.height = screen.height;

	vrState.pixelScale = 1.0;
	if (hmd)
		hmd->disable();
	if (hud.valid)
		R_DelFBO(&hud);
}

// launch-time initialization for VR support
void R_VR_Init()
{
	int i;
	currenteye = EYE_HUD;
	available_hmds[HMD_NONE] = vr_render_none;
	available_hmds[HMD_STEAM] = vr_render_svr;
	available_hmds[HMD_RIFT] = vr_render_ovr;

	for (i = 0; i < NUM_HMD_TYPES; i++)
	{
		if (available_hmds[i].init)
			available_hmds[i].init();
	}

	R_InitFBO(&hud);
	R_InitFBO(&offscreen);

	if (vr_enabled->value)
		R_VR_Enable();
}

// called when the renderer is shutdown
void R_VR_Shutdown()
{
	if (vr_enabled->value)
		R_VR_Disable();
}
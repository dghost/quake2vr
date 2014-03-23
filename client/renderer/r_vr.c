#include "include/r_local.h"
#include "../vr/include/vr.h"
#include "include/r_vr_ovr.h"
#include "include/r_vr_svr.h"

#define MAX_SEGMENTS 20
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

static int32_t leftStale, rightStale, hudStale;

static vr_eye_t currenteye;

vr_rect_t screen;

/*
typedef struct {
	GLfloat position[3];
	GLfloat texCoords[2];
} vert_t;

vert_t hudverts[MAX_VERTS];
*/
//
// Rendering related functions
//

// executed once per frame
void R_VR_StartFrame()
{
	int32_t resolutionChanged = 0;
	
	if (!hmd || !hmd->frameStart || !hmd->getState)
		return;
	
	if (vid.width != screen.width || vid.height != screen.height)
	{
		screen.height = vid.height;
		screen.width = vid.width;
		resolutionChanged = true;		
	}


	hmd->frameStart(resolutionChanged);
	hmd->getState(&vrState);


	if (vr_hud_segments->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_segments->value < 1)
			Cvar_SetInteger("vr_hud_segments",1);
		else if (vr_hud_segments->value > MAX_SEGMENTS)
			Cvar_SetValue("vr_hud_segments",MAX_SEGMENTS);
		vr_hud_segments->modified = false;
	
		// build VAO here
	}

	if (vr_hud_fov->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_fov->value < 30)
			Cvar_SetValue("vr_hud_fov",30.0f);
		else if (vr_hud_fov->value > vrState.viewFovY * 2.0)
			Cvar_SetValue("vr_hud_fov",vrState.viewFovY * 2.0);
		vr_hud_fov->modified = false;
		resolutionChanged = true;
	}

	if (resolutionChanged)
	{
		Com_Printf("VR: Calculated %.2f FOV\n", vrState.viewFovY);
	}

	leftStale = 1;
	rightStale = 1;
	hudStale = 1;

}

void R_Clear (void);
void R_VR_BindView(vr_eye_t eye)
{
	int32_t clear = 0;
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

	if (clear && eye != EYE_HUD)
	{
		glClearColor(0.0,0.0,0.0,0.0);
		R_Clear();
		glClearColor (1,0, 0.5, 0.5);
	} else if (clear) {
		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClearColor (1,0, 0.5, 0.5);
	}
}

void R_VR_GetViewPos(vr_eye_t eye, int32_t *x, int32_t *y)
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

void R_VR_GetViewSize(vr_eye_t eye, int32_t *width, int32_t *height)
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

void R_VR_CurrentViewPosition(int32_t *x, int32_t *y)
{
	R_VR_GetViewPos(currenteye,x,y);
}

void R_VR_CurrentViewSize(int32_t *width, int32_t *height)
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
		GL_MBind(0,0);

		R_VR_BindView(EYE_LEFT);
		R_VR_DrawHud(EYE_LEFT);

		R_VR_BindView(EYE_RIGHT);
		R_VR_DrawHud(EYE_RIGHT);

		GL_Disable(GL_ALPHA_TEST);

		GL_BindFBO(0);
		glViewport(0,0,screen.width,screen.height);
		vid.width = screen.width ;
		vid.height = screen.height;
	}
}

void R_VR_Perspective(float fovy, float aspect, float zNear, float zFar)
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
	float depth = vr_hud_depth->value;
	int numsegments = vr_hud_segments->value;
	float ipd = vr_autoipd->value ? vrState.ipd / 2.0 : (vr_ipd->value / 2000.0);
	vec_t mat[4][4],temp[4][4];

	extern int32_t scr_draw_loading;

	if (!vr_enabled->value)
		return;

	R_PerspectiveOffset(vrState.viewFovY, vrState.aspect, 0.24, 251.0, eye * vrState.projOffset);

	TranslationMatrix(0,0,0,mat);
	// disable this for the loading screens since they are not at 60fps
	if ((vr_hud_bounce->value > 0) && !scr_draw_loading && ((int32_t) vr_aimmode->value > 0))
	{
		// load the quaternion directly into a rotation matrix
		vec4_t q;
		VR_GetOrientationEMAQuat(q);
		q[2] = -q[2];
		QuatToRotation(q,mat);

	}

	TranslationMatrix(eye * -ipd,0,0,temp);
	MatrixMultiply(temp,mat,mat);
	GL_LoadMatrix(GL_MODELVIEW, mat);
	


	if (vr_hud_transparency->value)
	{
		GL_Enable(GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	} 
	GL_MBind(0,hud.texture);

	glBegin(GL_TRIANGLE_STRIP);

	if (numsegments > 1) {

		int i = 0;
		float interval = fov / (float) numsegments;
		float pos = fov * -0.5f;
		float texpos = 0;
		qboolean dir = false;
		float y = (tanf(abs(pos) * (M_PI/180.0f)) * depth) / ((float) hud.width / hud.height);

		for (i = 0; i <= numsegments; i++)
		{
			float z = depth * cosf(pos * (M_PI/180.0f));
			float x = tanf((pos ) * (M_PI/180.0f)) * (depth);

			glTexCoord2f (texpos, 0); glVertex3f (x, -y,-z);
			glTexCoord2f (texpos, 1); glVertex3f (x, y,-z);
			texpos += (1.0f / (float) numsegments);
			pos += interval;
		}
	} else {
		float y,x;
		// calculate coordinates for hud
		x = tanf(fov * (M_PI/180.0f) * 0.5) * (depth);
		y = x / ((float) hud.width / hud.height);
		glTexCoord2f (0, 0); glVertex3f (-x, -y,-depth);
		glTexCoord2f (0, 1); glVertex3f (-x, y,-depth);		
		glTexCoord2f (1, 0); glVertex3f (x, -y,-depth);
		glTexCoord2f (1, 1); glVertex3f (x, y,-depth);
	}

	glEnd();


	GL_Disable(GL_BLEND);
}

// takes the various FBO's and renders them to the framebuffer
void R_VR_Present()
{
	if (!hmd)
		return;

	GL_SetIdentity(GL_PROJECTION);
	GL_SetIdentity(GL_MODELVIEW);

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

		hmd = &available_hmds[(int32_t) vr_enabled->value];

		success = (qboolean) R_GenFBO(640, 480, 1, &hud);
		success = success && hmd->enable && hmd->enable();

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
	if (hmd && hmd->disable)
		hmd->disable();
	if (hud.valid)
		R_DelFBO(&hud);
}

// launch-time initialization for VR support
void R_VR_Init()
{
	int32_t i;
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
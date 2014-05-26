#include "include/r_local.h"
#include "../vr/include/vr.h"
#include "include/r_vr_ovr.h"
#include "include/r_vr_svr.h"

#define MAX_SEGMENTS 25
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


typedef struct {
	vec3_t position;
	vec2_t texCoords;
} vert_t;

static vert_t *hudverts;
static uint32_t hudNumVerts;

static GLuint currentFBO = 0;

// Default Lens Warp Shader
static r_shaderobject_t vr_shader_distort_norm = {
	0,
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/texdistort.frag"
};

// Default Lens Warp Shader
static r_shaderobject_t vr_shader_distort_bicubic_norm = {
	0,
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/texdistort_fastbicubic.frag"
};


// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t vr_shader_distort_chrm = {
	0,
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/texdistort_chromatic.frag"
};

static r_shaderobject_t vr_shader_distort_bicubic_chrm = {
	0,
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/texdistort_chromatic_fastbicubic.frag"
};

vr_distort_shader_t vr_distort_shaders[2];
vr_distort_shader_t vr_bicubic_distort_shaders[2];

//
// Rendering related functions
//

void sphereProject(vec3_t pos, vec3_t scale, vec3_t out)
{
	vec3_t temp;
	vec_t x2 = pos[0] * pos[0];
	vec_t y2 = pos[1] * pos[1];
	vec_t z2 = pos[2] * pos[2];

	temp[0] = pos[0] * sqrt(1 - y2 / 2.0 - z2 / 2.0 + (y2 * z2) / 3.0) * scale[0];
	temp[1] = pos[1] * sqrt(1 - z2 / 2.0 - x2 / 2.0 + (x2 * z2) / 3.0) * scale[1];
	temp[2] = pos[2] * sqrt(1 - x2 / 2.0 - y2 / 2.0 + (y2 * x2) / 3.0) * scale[2];
	VectorCopy(temp, out);
}

// executed once per frame
void R_VR_StartFrame()
{
	qboolean resolutionChanged = 0;
	qboolean hudChanged = 0;

	if (!hmd || !hmd->frameStart || !hmd->getState)
		return;

	if (vid.width != screen.width || vid.height != screen.height)
	{
		screen.height = vid.height;
		screen.width = vid.width;
		resolutionChanged = true;
	}

	currentFBO = glState.currentFBO;
	hmd->frameStart(resolutionChanged);
	hmd->getState(&vrState);

	if (vr_hud_segments->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_segments->value < 0)
			Cvar_SetInteger("vr_hud_segments", 0);
		else if (vr_hud_segments->value > MAX_SEGMENTS)
			Cvar_SetValue("vr_hud_segments", MAX_SEGMENTS);
		vr_hud_segments->modified = false;
		hudChanged = true;
	}

	if (vr_hud_depth->modified)
	{
		if (vr_hud_depth->value < 0.25f)
			Cvar_SetValue("vr_hud_depth", 0.25);
		else if (vr_hud_depth->value > 250)
			Cvar_SetValue("vr_hud_depth", 250);
		vr_hud_depth->modified = false;
		hudChanged = true;
	}

	if (vr_hud_fov->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_fov->value < 30)
			Cvar_SetValue("vr_hud_fov", 30.0f);
		else if (vr_hud_fov->value > vrState.viewFovY * 2.0)
			Cvar_SetValue("vr_hud_fov", vrState.viewFovY * 2.0);
		vr_hud_fov->modified = false;
		hudChanged = true;
	}

	if (hudChanged)
	{
		int i, j;
		float numsegments = floor(vr_hud_segments->value);
		float horizFOV = vr_hud_fov->value;
		float depth = vr_hud_depth->value;
		float horizInterval = 2.0 / numsegments;
		float vertBounds = (float) hud.height / (float) hud.width;
		float vertInterval = horizInterval * vertBounds;

		int numverts = (numsegments) * (numsegments + 1) * 2 + (numsegments * 2);


		// calculate coordinates for hud
		float xoff = tanf(horizFOV * (M_PI / 180.0f) * 0.5) * (depth);
		float zoff = depth * cosf(horizFOV * (M_PI / 180.0f) * 0.5);
		vec3_t offsetScale;
		VectorSet(offsetScale, xoff, xoff, zoff);

		hudNumVerts = 0;

		if (hudverts)
			free(hudverts);

		hudverts = malloc(sizeof(vert_t) * numverts);
		memset(hudverts, 0, sizeof(vert_t) * numverts);

		for (j = 0; j < numsegments; j++)
		{
			float ypos = j * vertInterval - vertBounds;
			float ypos1 = (j + 1) * vertInterval - vertBounds;

			for (i = 0; i <= numsegments; i++)
			{
				float xpos = i * horizInterval - 1;
				float xpos1 = (i + 1) * horizInterval - 1;

				vert_t vert1, vert2;
	
				VectorSet(vert1.position, xpos, ypos, -1);
				sphereProject(vert1.position, offsetScale, vert1.position);
				vert1.texCoords[0] = (float) i / (float) (numsegments);
				vert1.texCoords[1] = (float) j / (float) (numsegments);

				VectorSet(vert2.position, xpos, ypos1, -1);
				sphereProject(vert2.position, offsetScale, vert2.position);
				vert2.texCoords[0] = (float) i / (float) (numsegments);
				vert2.texCoords[1] = (float) (j + 1) / (float) (numsegments);

				if (j > 0 && i == 0)
				{
					memcpy(&hudverts[hudNumVerts++], &vert1, sizeof(vert_t));
				}

				memcpy(&hudverts[hudNumVerts++], &vert1, sizeof(vert_t));

				memcpy(&hudverts[hudNumVerts++], &vert2, sizeof(vert_t));

				if (i == numsegments && j < (numsegments - 1))
				{
					memcpy(&hudverts[hudNumVerts++], &vert2, sizeof(vert_t));
				}
			}
		}
	}

	if (resolutionChanged)
	{
		Com_Printf("VR: Calculated %.2f FOV\n", vrState.viewFovY);
	}

	leftStale = 1;
	rightStale = 1;
	hudStale = 1;

}

void R_Clear(void);
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
		}
		else if (eye == EYE_RIGHT)
		{
			clear = rightStale;
			rightStale = 0;
		}
	}

	if (clear && eye != EYE_HUD)
	{
		GL_ClearColor(0.0, 0.0, 0.0, 0.0);
		R_Clear();
		GL_SetDefaultClearColor();
	}
	else if (clear) {
		GL_ClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		GL_SetDefaultClearColor();
	}
}

void R_VR_GetViewPos(vr_eye_t eye, int32_t *x, int32_t *y)
{
	if (eye == EYE_HUD)
	{
		*x = 0;
		*y = 0;
	}
	else if (hmd)
	{
		vr_rect_t view;
		hmd->getViewRect(eye, &view);
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
	}
	else if (hmd)
	{
		vr_rect_t view;
		hmd->getViewRect(eye, &view);
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
	}
	else if (hmd)
		hmd->getViewRect(eye, rect);
}

void R_VR_CurrentViewPosition(int32_t *x, int32_t *y)
{
	R_VR_GetViewPos(currenteye, x, y);
}

void R_VR_CurrentViewSize(int32_t *width, int32_t *height)
{
	R_VR_GetViewSize(currenteye, width, height);
}

void R_VR_CurrentViewRect(vr_rect_t *rect)
{
	R_VR_GetViewRect(currenteye, rect);
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
		GL_Enable(GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER, 0.0f);

		GL_MBind(0, hud.texture);

		R_VR_BindView(EYE_LEFT);
		R_VR_DrawHud(EYE_LEFT);

		R_VR_BindView(EYE_RIGHT);
		R_VR_DrawHud(EYE_RIGHT);

		GL_MBind(0, 0);

		GL_Disable(GL_ALPHA_TEST);

		glViewport(0, 0, screen.width, screen.height);
		vid.width = screen.width;
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
	vec_t mat[4][4], temp[4][4];

	extern int32_t scr_draw_loading;

	if (!vr_enabled->value || !hudverts)
		return;

	R_PerspectiveOffset(vrState.viewFovY, vrState.aspect, 0.24, 251.0, eye * vrState.projOffset);

	TranslationMatrix(0, 0, 0, mat);
	// disable this for the loading screens since they are not at 60fps
	if ((vr_hud_bounce->value > 0) && !scr_draw_loading && ((int32_t) vr_aimmode->value > 0))
	{
		// load the quaternion directly into a rotation matrix
		vec4_t q;
		VR_GetOrientationEMAQuat(q);
		q[2] = -q[2];
		QuatToRotation(q, mat);

	}

	TranslationMatrix(eye * -ipd, 0, 0, temp);
	MatrixMultiply(temp, mat, mat);
	GL_LoadMatrix(GL_MODELVIEW, mat);

	if (vr_hud_transparency->value)
	{
		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	}

	/*
	// debug rendering of HUD wireframe
	GL_MBind(0, 0);
	glColor3f(0.25, 0.25, 0.25);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBegin(GL_TRIANGLE_STRIP);
	if (hudNumVerts >= 4) {
		int i = 0;
		for (i = 0; i < hudNumVerts; i++)
		{
			glTexCoord2fv(hudverts[i].texCoords);
			glVertex3fv(hudverts[i].position);
		}
	}
	else {
		float y, x, z;
		// calculate coordinates for hud
		x = tanf(fov * (M_PI / 180.0f) * 0.5) * (depth);
		y = x / ((float) hud.width / hud.height);
		z = depth * cosf(fov * (M_PI / 180.0f) * 0.5);

		glTexCoord2f(0, 0); glVertex3f(-x, -y, -z);
		glTexCoord2f(0, 1); glVertex3f(-x, y, -z);
		glTexCoord2f(1, 0); glVertex3f(x, -y, -z);
		glTexCoord2f(1, 1); glVertex3f(x, y, -z);
	}

	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(1.0, 1.0, 1.0);
	GL_MBind(0, hud.texture);
	*/

	glBegin(GL_TRIANGLE_STRIP);

	if (hudNumVerts >= 4) {
		int i = 0;
		for (i = 0; i < hudNumVerts; i++)
		{
			glTexCoord2fv(hudverts[i].texCoords);
			glVertex3fv(hudverts[i].position);
		}
	}
	else {
		float y, x, z;
		// calculate coordinates for hud
		x = tanf(fov * (M_PI / 180.0f) * 0.5) * (depth);
		y = x / ((float) hud.width / hud.height);
		z = depth * cosf(fov * (M_PI / 180.0f) * 0.5);

		glTexCoord2f(0, 0); glVertex3f(-x, -y, -z);
		glTexCoord2f(0, 1); glVertex3f(-x, y, -z);
		glTexCoord2f(1, 0); glVertex3f(x, -y, -z);
		glTexCoord2f(1, 1); glVertex3f(x, y, -z);
	}

	glEnd();

	GL_Disable(GL_BLEND);
}

// takes the various FBO's and renders them to the framebuffer
void R_VR_Present()
{
	if (!hmd)
		return;
	GL_BindFBO(currentFBO);

	GL_SetIdentity(GL_PROJECTION);
	GL_SetIdentity(GL_MODELVIEW);

	// tell the HMD renderer to draw composited scene
	hmd->present();

}

// util function
void R_VR_InitDistortionShader(vr_distort_shader_t *shader, r_shaderobject_t *object)
{
	if (!object->program)
		R_CompileShaderFromFiles(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);
	shader->uniform.texture = glGetUniformLocation(shader->shader->program, "tex");
	shader->uniform.distortion = glGetUniformLocation(shader->shader->program, "dist");
	shader->uniform.texture_size = glGetUniformLocation(shader->shader->program, "textureSize");
	glUseProgram(0);
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


		R_VR_InitDistortionShader(&vr_distort_shaders[0], &vr_shader_distort_norm);
		R_VR_InitDistortionShader(&vr_distort_shaders[1], &vr_shader_distort_chrm);
		R_VR_InitDistortionShader(&vr_bicubic_distort_shaders[0], &vr_shader_distort_bicubic_norm);
		R_VR_InitDistortionShader(&vr_bicubic_distort_shaders[1], &vr_shader_distort_bicubic_chrm);

		if (!success)
		{
			Com_Printf(" failed!\n");
			Cmd_ExecuteString("vr_disable");
		}
		else {
			char string[6];
			Com_Printf(" ok!\n");

			hmd->getState(&vrState);


			strncpy(string, va("%.2f", vrState.ipd * 1000), sizeof(string));
			vr_ipd = Cvar_Get("vr_ipd", string, CVAR_ARCHIVE);

			if (vr_ipd->value < 0)
				Cvar_SetToDefault("vr_ipd");

		}

	}
	else {
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
	glViewport(0, 0, screen.width, screen.height);

	vid.width = screen.width;
	vid.height = screen.height;

	vrState.pixelScale = 1.0;
	if (hmd && hmd->disable)
		hmd->disable();
	if (hud.valid)
		R_DelFBO(&hud);

	R_DelShaderProgram(&vr_shader_distort_norm);
	R_DelShaderProgram(&vr_shader_distort_chrm);
	R_DelShaderProgram(&vr_shader_distort_bicubic_norm);
	R_DelShaderProgram(&vr_shader_distort_bicubic_chrm);

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
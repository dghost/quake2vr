#include "include/r_local.h"
#include "../vr/include/vr.h"
#include "include/r_vr_ovr.h"
#include "include/r_vr_svr.h"

#define MAX_SEGMENTS 25
static fbo_t hud;

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

vr_rect_t screen;

static qboolean loadingScreen;

typedef struct {
	vec3_t position;
	vec2_t texCoords;
} vert_t;

static vbo_t hudVBO;

r_attrib_t distort[] = {
	{"Position",0},
	{"TexCoord",1},
	{NULL,0}
};

// Default Lens Warp Shader
static r_shaderobject_t vr_shader_distort_norm = {
	0,
	// vertex shader (identity)
	"vr/texdistort.vert",
	// fragment shader
	"vr/texdistort.frag",
	distort
};


// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t vr_shader_distort_chrm = {
	0,
	// vertex shader (identity)
	"vr/texdistort.vert",
	// fragment shader
	"vr/texdistort_chromatic.frag",
	distort
};

vr_distort_shader_t vr_distort_shaders[2];
vr_param_t vrState;

vec_t vr_hud_yaw = 0;

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

void R_VR_GenerateHud()
{
	int i, j;
	float numsegments = floor(vr_hud_segments->value);
	float horizFOV = vr_hud_fov->value;
	float depth = vr_hud_depth->value;
	float horizInterval = 2.0 / numsegments;
	float vertBounds = (float) hud.height / (float) hud.width;
	float vertInterval = horizInterval * vertBounds;

	int numindices = (numsegments) * (numsegments + 1) * 2 + (numsegments * 2);
	int numverts = (numsegments) * (numsegments + 1) * 2;


	vert_t *hudverts = NULL;
	GLushort *indices = NULL;
	uint32_t hudNumVerts = 0;
	GLushort currIndex = 0;
	uint32_t iboSize = sizeof(GLushort) * numindices;
	uint32_t vboSize = sizeof(vert_t) * numverts;

	// calculate coordinates for hud
	float xoff = tanf(horizFOV * (M_PI / 180.0f) * 0.5) * (depth);
	float zoff = depth * cosf(horizFOV * (M_PI / 180.0f) * 0.5);
	vec3_t offsetScale;
	VectorSet(offsetScale, xoff, xoff, zoff);

	hudverts = (vert_t *) malloc(vboSize);
	memset(hudverts, 0, vboSize);

	indices = (GLushort *) malloc(iboSize);
	memset(indices, 0, iboSize);

	for (j = 0; j < numsegments; j++)
	{
		float ypos, ypos1;
		qboolean verticalHalf = (j >= numsegments / 2);
		ypos = j * vertInterval - vertBounds;
		ypos1 = (j + 1) * vertInterval - vertBounds;

		for (i = 0; i <= numsegments; i++)
		{
			float xpos;
			vert_t vert1, vert2;
			GLushort vertNum1, vertNum2;
			
			xpos = i * horizInterval - 1;

			VectorSet(vert1.position, xpos, ypos, -1);
			sphereProject(vert1.position, offsetScale, vert1.position);
			vert1.texCoords[0] = (float) i / (float) (numsegments);
			vert1.texCoords[1] = (float) j / (float) (numsegments);
				

			VectorSet(vert2.position, xpos, ypos1, -1);
			sphereProject(vert2.position, offsetScale, vert2.position);
			vert2.texCoords[0] = (float) i / (float) (numsegments);
			vert2.texCoords[1] = (float) (j + 1) / (float) (numsegments);

			vertNum1 = hudNumVerts++;
			vertNum2 = hudNumVerts++;

			if (verticalHalf)
			{
				hudverts[vertNum2] = vert1;
				hudverts[vertNum1] = vert2;
			} else {
				hudverts[vertNum1] = vert1;
				hudverts[vertNum2] = vert2;
			}

			if (j > 0 && i == 0)
			{
				indices[currIndex++] = vertNum1;
			}

			indices[currIndex++] = vertNum1;
			indices[currIndex++] = vertNum2;


			if (i == numsegments && j < (numsegments - 1))
			{
				indices[currIndex++] = vertNum2;
			}
		}
	}

	R_BindIVBO(&hudVBO,NULL,0);
	R_VertexData(&hudVBO,hudNumVerts * sizeof(vert_t),hudverts);
	R_IndexData(&hudVBO,GL_TRIANGLE_STRIP,GL_UNSIGNED_SHORT,currIndex,currIndex * sizeof(GLushort),indices);
	R_ReleaseIVBO();
	free(hudverts);
	free(indices);
}

// executed once per frame
void R_Clear(void);

void R_VR_StartFrame()
{
	qboolean resolutionChanged = 0;
	qboolean hudChanged = 0;
	extern int32_t scr_draw_loading;


	if (!hmd || !hmd->frameStart || !hmd->getState)
		return;
	
	R_AntialiasSetFBOSize(vrState.offscreen);
	if (vrState.offscreen->width != screen.width || vrState.offscreen->height != screen.height)
	{
		screen.height = vrState.offscreen->height;
		screen.width = vrState.offscreen->width;
		resolutionChanged = true;
	}

	hmd->frameStart(resolutionChanged);
	hmd->getState(&vrState);

	if (vr_hud_width->modified || vr_hud_height->modified)
	{
		int width, height;

		width = vr_hud_width->value;
		height = vr_hud_height->value;

		if (width < 320)
			width = 320;

		if (height < 240)
			height = 240;

		Cvar_SetInteger("vr_hud_width",width);
		Cvar_SetInteger("vr_hud_height",height);
		vr_hud_width->modified = false;
		vr_hud_height->modified = false;
		Com_Printf("VR: New HUD resolution %ix%i\n",width,height);
		if (hud.width != width || hud.height != height)
			R_ResizeFBO(width,height,1,GL_RGBA8,&hud);

	}

	if (vr_hud_segments->modified)
	{
		// clamp value from 30-90 degrees
		if (vr_hud_segments->value < 0)
			Cvar_SetInteger("vr_hud_segments", 1);
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
		R_VR_GenerateHud();

	if (resolutionChanged)
	{
		Com_Printf("VR: Calculated %.2f FOV\n", vrState.viewFovY);
	}


	GL_ClearColor(0.0, 0.0, 0.0, 0.0);
	R_BindFBO(vrState.eyeFBO[0]);
	R_Clear();
	R_BindFBO(vrState.eyeFBO[1]);
	R_Clear();
	R_BindFBO(&hud);
	R_Clear();
	R_BindFBO(&screenFBO);
	GL_SetDefaultClearColor();		

	hudStale = 1;

	loadingScreen = (qboolean) (scr_draw_loading > 0 ? 1 : 0);

}

void R_VR_GetFOV(float *fovx, float *fovy)
{
	if (!hmd)
		return;

	*fovx = vrState.viewFovX * vr_autofov_scale->value;
	*fovy = vrState.viewFovY * vr_autofov_scale->value;
}

fbo_t* R_VR_GetHUDFBO()
{
	if (!hmd)
		return NULL;
	
	return &hud;
}

fbo_t* R_VR_GetFrameFBO()
{
	if (!vr_enabled->value)
		return NULL;
	return vrState.offscreen;	
}


void R_VR_DrawHud()
{
	int index = 0;
	vec_t mat[4][4], temp[4][4], counter[4][4];
	vec3_t rot, pos;

	if (!vr_enabled->value)
		return;

	// force pumping of these values;
	VR_GetOrientation(rot);
	VR_GetHeadOffset(pos);

	// enable alpha testing so only pixels that have alpha info get written out
	// prevents black pixels from being rendered into the view
	GL_Enable(GL_ALPHA_TEST);
	GL_AlphaFunc(GL_GREATER, 0.0f);

	// if hud transparency is enabled, enable blending
	if (vr_hud_transparency->value)
	{
		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}


	// bind the texture
	GL_MBind(0, hud.texture);

	// disable this for the loading screens since they are not at 60fps
	if ((vr_hud_bounce->value > 0) && !loadingScreen && ((int32_t) vr_aimmode->value > 0))
	{
		// load the quaternion directly into a rotation matrix
		vec4_t q;
		VR_GetOrientationEMAQuat(q);
		q[2] = -q[2];
		QuatToRotation(q, mat);
	} else {
		// identity matrix
		TranslationMatrix(0,0,0,mat);
	}

	TranslationMatrix(0,0,0,counter);

	if (!vr_hud_fixed->value && !loadingScreen && ((int32_t) vr_aimmode->value > 0))
	{
		vec_t tmat[4][4];
		vec_t angle_diff;
		vec3_t e;
		VR_GetOrientation(e);

		angle_diff = e[1] - vr_hud_yaw;
		AngleClamp(&angle_diff);
		if (angle_diff < -vr_hud_deadzone_yaw->value)
		{
			vr_hud_yaw = e[1] + vr_hud_deadzone_yaw->value;
		}
		else if (angle_diff > vr_hud_deadzone_yaw->value)
		{
			vr_hud_yaw = e[1] - vr_hud_deadzone_yaw->value;
		}

		RotationMatrix(vr_hud_yaw - e[1], 0.0, 1.0, 0.0, tmat);
		MatrixMultiply(counter, tmat, counter);

		RotationMatrix(e[0], 1.0, 0.0, 0.0, tmat);
		MatrixMultiply(counter, tmat, counter);

		RotationMatrix(e[2], 0.0, 0.0, 1.0, tmat);
		MatrixMultiply(counter, tmat, counter);
	}

	// set proper mode
	glDisableClientState (GL_COLOR_ARRAY);

	// bind vertex buffer and set tex coord parameters
	R_BindIVBO(&hudVBO,NULL,0);
	glTexCoordPointer(2,GL_FLOAT,sizeof(vert_t),(void *)( sizeof(GL_FLOAT) * 3));
	glVertexPointer(3,GL_FLOAT,sizeof(vert_t),NULL);

	for (index = 0; index < 2; index++)
	{
		// bind the eye FBO
		R_BindFBO(vrState.eyeFBO[index]);

		// set the perspective matrix for that eye
		R_PerspectiveScale(vrState.renderParams[index].projection, 0.01, 251.0);

		// build the eye translation matrix
		if (vr_autoipd->value)
		{
			TranslationMatrix(-vrState.renderParams[index].viewOffset[0], vrState.renderParams[index].viewOffset[1], vrState.renderParams[index].viewOffset[2], temp);
		} else {
			float viewOffset = (vr_ipd->value / 2000.0);
			TranslationMatrix((-1 + index * 2) * -viewOffset, 0, 0, temp);
		}

		// load the view matrix
		MatrixMultiply(temp, mat, temp);
		MatrixMultiply(counter, temp, temp);
		GL_LoadMatrix(GL_MODELVIEW, temp);

		// draw the hud for that eye
		R_DrawIVBO(&hudVBO);
	}

	// teardown 
	R_ReleaseIVBO();

	GL_MBind(0, 0);

	glEnableClientState (GL_COLOR_ARRAY);
	glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
	glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
}


void R_VR_EndFrame()
{
	if (hmd && vr_enabled->value)
	{
		GL_Disable(GL_DEPTH_TEST);

		//draw the HUD into specific views
		R_VR_DrawHud();

		// draw the HUD 
		R_BindFBO(vrState.offscreen);
		GL_SetIdentity(GL_PROJECTION);
		GL_SetIdentity(GL_MODELVIEW);

		// tell the HMD renderer to draw composited scene
		hmd->present((qboolean) (loadingScreen || (vr_aimmode->value == 0)));
	}
}

void R_VR_PostGammaPresent()
{
	if (hmd && hmd->postPresent)
		hmd->postPresent();
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

		hmd = &available_hmds[(int32_t) vr_enabled->value];

		success = R_GenFBO(640,480,1,GL_RGBA8,&hud);
		success = success && hmd->enable && hmd->enable();

		// shader init
		R_VR_InitDistortionShader(&vr_distort_shaders[0], &vr_shader_distort_norm);
		R_VR_InitDistortionShader(&vr_distort_shaders[1], &vr_shader_distort_chrm);

		if (!success)
		{
			Com_Printf(" failed!\n");
			Cmd_ExecuteString("vr_disable");
		}
		else {
			char string[6];
			Com_Printf(" ok!\n");

			hmd->getState(&vrState);

			//strncpy(string, va("%.2f", vrState.ipd * 1000), sizeof(string));
			vr_ipd = Cvar_Get("vr_ipd", string, CVAR_ARCHIVE);

			if (vr_ipd->value < 0)
				Cvar_SetToDefault("vr_ipd");

		}

		R_CreateIVBO(&hudVBO, GL_STATIC_DRAW);
		R_VR_GenerateHud();

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
	R_DelIVBO(&hudVBO);

	R_BindFBO(&screenFBO);
	
	vrState.pixelScale = 1.0;

	if (hmd && hmd->disable)
		hmd->disable();
	if (hud.valid)
		R_DelFBO(&hud);

	R_DelShaderProgram(&vr_shader_distort_norm);
	R_DelShaderProgram(&vr_shader_distort_chrm);
}

// launch-time initialization for VR support
void R_VR_Init()
{
	int32_t i;
	available_hmds[HMD_NONE] = vr_render_none;
	available_hmds[HMD_STEAM] = vr_render_svr;
	available_hmds[HMD_RIFT] = vr_render_ovr;

	for (i = 0; i < NUM_HMD_TYPES; i++)
	{
		if (available_hmds[i].init)
			available_hmds[i].init();
	}

	R_InitFBO(&hud);
	R_InitIVBO(&hudVBO);

	if (vr_enabled->value)
		R_VR_Enable();
}

// called when the renderer is shutdown
void R_VR_Shutdown()
{
	R_VR_Disable();
}

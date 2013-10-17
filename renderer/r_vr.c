#include "r_local.h"
#include "../vr/vr.h"
#include "../vr/vr_ovr.h"


typedef struct {
	GLhandleARB program;
	GLhandleARB vert_shader;
	GLhandleARB frag_shader;
	const char *vert_source;
	const char *frag_source;
} r_shaderobject_t;

typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint scale;
		GLuint scale_in;
		GLuint lens_center;
		GLuint screen_center;
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
	} uniform;

} r_shader_t;

static r_shader_t ovr_shaders[2];

static fbo_t world, hud; 

static GLint defaultFBO;

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_norm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 theta;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scaleIn;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"theta = (vec2(gl_MultiTexCoord0) - lensCenter) * scaleIn;\n"
	"}\n",

	// fragment shader
	"varying vec2 theta;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform sampler2D texture;\n"
	"void main()\n"
	"{\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"vec2 tc = (lensCenter + scale * rvector);\n"
		"if (any(bvec2(clamp(tc, screenCenter - vec2(0.25,0.5), screenCenter + vec2(0.25,0.5))-tc)))\n"
			"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"else\n"
			"gl_FragColor = texture2D(texture, tc);\n"
	"}\n"
};

// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t ovr_shader_chrm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 theta;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scaleIn;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"theta = (vec2(gl_MultiTexCoord0) - lensCenter) * scaleIn;\n"
	"}\n",

	// fragment shader
	"varying vec2 theta;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform vec4 chromAbParam;\n"
	"uniform sampler2D texture;\n"

	// Scales input texture coordinates for distortion.
	// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
	// larger due to aspect ratio.
	"void main()\n"
	"{\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 theta1 = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"

		// Detect whether blue texture coordinates are out of range since these will scaled out the furthest.
		"vec2 thetaBlue = theta1 * (chromAbParam.z + chromAbParam.w * rSq);\n"
		"vec2 tcBlue = lensCenter + scale * thetaBlue;\n"

		"if (!all(equal(clamp(tcBlue, screenCenter - vec2(0.25,0.5), screenCenter + vec2(0.25,0.5)),tcBlue)))\n"
		"{\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
			"return;\n"
		"}\n"

		// Now do blue texture lookup.
		"float blue = texture2D(texture, tcBlue).b;\n"

		// Do green lookup (no scaling).
		"vec2  tcGreen = lensCenter + scale * theta1;\n"

		"float green = texture2D(texture, tcGreen).g;\n"

		// Do red scale and lookup.
		"vec2  thetaRed = theta1 * (chromAbParam.x + chromAbParam.y * rSq);\n"
		"vec2  tcRed = lensCenter + scale * thetaRed;\n"

		"float red = texture2D(texture, tcRed).r;\n"

		"gl_FragColor = vec4(red, green, blue, 1.0);\n"
	"}\n"
};

//
// Utility Functions
//

qboolean R_CompileShader(GLhandleARB shader, const char *source)
{
	GLint status;

	qglShaderSourceARB(shader, 1, &source, NULL);
	qglCompileShaderARB(shader);
	qglGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (status == 0)
	{
		GLint length;
		char *info;

		qglGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		info = (char *) malloc(sizeof(char) * length+1);
		qglGetInfoLogARB(shader, length, NULL, info);
		VID_Printf(PRINT_ALL,S_COLOR_RED "Failed to compile shader:\n%s\n%s", source, info);
		free(info);
	}

	return !!status;
}

qboolean R_CompileShaderProgram(r_shaderobject_t *shader)
{
	qglGetError();
	if (shader)
	{
		shader->program = qglCreateProgramObjectARB();

		shader->vert_shader = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		if (!R_CompileShader(shader->vert_shader, shader->vert_source)) {
			return false;
		}

		shader->frag_shader = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		if (!R_CompileShader(shader->frag_shader, shader->frag_source)) {
			return false;
		}

		qglAttachObjectARB(shader->program, shader->vert_shader);
		qglAttachObjectARB(shader->program, shader->frag_shader);
		qglLinkProgramARB(shader->program); 
	}
	return (qglGetError() == GL_NO_ERROR);
}

void R_DelShaderProgram(r_shaderobject_t *shader)
{
	if (shader->program)
	{
		qglDeleteObjectARB(shader->program);
		shader->program = 0;
	}
	if (shader->frag_shader)
	{
		qglDeleteObjectARB(shader->frag_shader);
		shader->frag_shader = 0;
	}

	if (shader->vert_shader)
	{
		qglDeleteObjectARB(shader->vert_shader);
		shader->vert_shader = 0;
	}
}

//
// Rendering related functions
//

// executed once per frame
void R_VR_StartFrame()
{
	int hmd = (int) vr_enabled->value;

	if (!hmd)
		return;

	vrState.viewOffset = (vr_ipd->value / 2000.0) * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
	
	if (hmd == HMD_RIFT)
	{
		if (vr_ovr_scale->modified)
		{
			float scale;
			if (vr_ovr_scale->value < -1.0)
				Cvar_SetInteger("vr_ovr_scale", -1);
			else if (vr_ovr_scale->value < 1.0)
				Cvar_SetInteger("vr_ovr_scale", (int) vr_ovr_scale->value);
			else if (vr_ovr_scale->value > 2.0)
				Cvar_Set("vr_ovr_scale", "2.0");
			scale = VR_OVR_GetDistortionScale();
			
			R_DelFBO(&world);
		
			vrState.vrWidth = scale * vrState.viewWidth;
			vrState.vrHalfWidth = scale  * vrState.viewWidth / 2.0;
			vrState.vrHeight = scale  * vrState.viewHeight;
			R_GenFBO(vrState.vrWidth, vrState.vrHeight, &world);

			VR_OVR_SetFOV();
			vrState.pixelScale = (float) vrState.vrWidth / (float) vrConfig.hmdWidth;
			Com_Printf("VR: Calculated %.2f FOV\n", vrState.viewFovY);
			Com_Printf("VR: Using %u x %u backbuffer\n", vrState.vrWidth, vrState.vrHeight);
			vr_ovr_scale->modified = false;

		}



	}


	qglClearColor(0.0,0.0,0.0,0.0);

	R_VR_BindWorld();

	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	R_VR_BindHud();

	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	qglClearColor (1,0, 0.5, 0.5);

}

void R_VR_BindHud()
{
	if (vr_enabled->value)
	{
		R_BindFBO(&hud);

		vid.height = hud.height;
		vid.width = hud.width;
		vrState.eye = EYE_NONE;
	}
}

void R_VR_BindLeft()
{
	if (vr_enabled->value)
	{
		R_BindFBO(&world);
		qglViewport(0,0,vrState.vrHalfWidth,vrState.vrHeight);
		vid.height = vrState.vrHeight;
		vid.width = vrState.vrHalfWidth;
		vrState.eye = EYE_LEFT;
	}
}

void R_VR_BindRight()
{
	if (vr_enabled->value)
	{
		R_BindFBO(&world);
		qglViewport(vrState.vrHalfWidth,0,vrState.vrHalfWidth,vrState.vrHeight);
		vid.height = vrState.vrHeight;
		vid.width = vrState.vrHalfWidth;
		vrState.eye = EYE_RIGHT;

	}
}

void R_VR_BindWorld()
{
	if (vr_enabled->value)
	{
		R_BindFBO(&world);
		qglViewport(0,0,vrState.vrWidth,vrState.vrHeight);
		vid.height = vrState.vrHeight;
		vid.width = vrState.vrWidth;
		vrState.eye = EYE_NONE;
	}
}


void R_VR_Rebind()
{
	switch (vrState.eye) {
	case EYE_LEFT:
		R_VR_BindLeft();
		break;
	case EYE_RIGHT:
		R_VR_BindRight();
		break;
	case EYE_NONE:
		R_VR_BindHud();
		break;
	}
}

void R_VR_EndFrame()
{
	if (vr_enabled->value)
	{
		qglBindFramebuffer(GL_FRAMEBUFFER,defaultFBO);
		qglViewport(0,0,vrState.viewWidth,vrState.viewHeight);
		vid.width = vrState.viewWidth;
		vid.height = vrState.viewHeight;
	}
}

extern void vrPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar, GLdouble offset);

void R_VR_DrawHud()
{
	float fov = vr_hud_fov->value;
	float y,x;
	float depth = vr_hud_depth->value;

	extern int scr_draw_loading;

	if (!vr_enabled->value)
		return;

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	vrPerspective(vrState.viewFovY, vrConfig.aspect, 0.24, 251.0, vrState.eye * vrState.projOffset);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	// disable this for the loading screens since they are not at 60fps
	if ((vr_hud_bounce->value > 0) && !scr_draw_loading && ((int) vr_aimmode->value > 0))
	{
		// load the quaternion directly into a rotation matrix
		vec4_t q;
		vec4_t mat[4];
		VR_GetOrientationEMAQuat(q);
		q[2] = -q[2];
		QuatToRotation(q,mat);
		qglMultMatrixf((const GLfloat *) mat);
		
	
		/* depriciated as of 9/30/2013 - consider removing later
		// convert to euler angles and rotate
		vec3_t orientation;
		VR_GetOrientationEMA(orientation);
		qglRotatef (orientation[0],  1, 0, 0);
		// y axis is inverted between Quake II and OpenGL
		qglRotatef (-orientation[1],  0, 1, 0);
		qglRotatef (orientation[2],  0, 0, 1);
		*/
	}

	qglTranslatef(vrState.eye * -vr_ipd->value / 2000.0,0,0);
	// calculate coordinates for hud
	x = tanf(fov * (M_PI/180.0f) * 0.5) * (depth);
	y = x / ((float) hud.width / hud.height);



	if (vr_hud_transparency->value)
	{
		GL_Enable(GL_BLEND);
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	} else {
		GL_Enable (GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER,0.0f);
	}
	GL_Bind(hud.texture);


	qglBegin(GL_TRIANGLE_STRIP);
	qglTexCoord2f (0, 0); qglVertex3f (-x, -y,-depth);
	qglTexCoord2f (0, 1); qglVertex3f (-x, y,-depth);		
	qglTexCoord2f (1, 0); qglVertex3f (x, -y,-depth);
	qglTexCoord2f (1, 1); qglVertex3f (x, y,-depth);
	qglEnd();

//	GL_Disable(GL_BLEND);
}

// takes the various FBO's and renders them to the framebuffer
void R_VR_Present()
{

	int current_hmd = (int) vr_enabled->value;
	if (!current_hmd)
		return;

	if (current_hmd == HMD_RIFT)
	{

	}
	GL_Disable(GL_DEPTH_TEST);

	GL_SelectTexture(0);
	GL_Bind(hud.texture);
	R_BindFBO(&world);
	qglViewport(0,0,vrState.vrHalfWidth,vrState.vrHeight);
	vid.height = vrState.vrHeight;
	vid.width = vrState.vrHalfWidth;
	vrState.eye = EYE_LEFT;
	R_VR_DrawHud();

	qglViewport(vrState.vrHalfWidth,0,vrState.vrHalfWidth,vrState.vrHeight);
	vrState.eye = EYE_RIGHT;

	R_VR_DrawHud();
	GL_Bind(0);
	R_VR_EndFrame();

	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();



	if (current_hmd == HMD_RIFT)
	{
		if (vr_ovr_distortion->value)
		{

			float scale = VR_OVR_GetDistortionScale();
			r_shader_t *current_shader;
			vec4_t debugColor;
			current_shader = &ovr_shaders[!!(int) vr_ovr_chromatic->value];
			
		

			GL_Bind(world.texture);

			qglUseProgramObjectARB(current_shader->shader->program);

			qglUniform4fvARB(current_shader->uniform.chrom_ab_param, 1, vrConfig.chrm);
			qglUniform4fvARB(current_shader->uniform.hmd_warp_param, 1, vrConfig.dk);
			qglUniform2fARB(current_shader->uniform.scale_in, 4.0f, 2.0f / vrConfig.aspect);
			qglUniform2fARB(current_shader->uniform.scale, 0.25f / scale, 0.5f * vrConfig.aspect / scale);


			qglUniform2fARB(current_shader->uniform.lens_center, 0.25 + vrState.projOffset * 0.25, 0.5);
			qglUniform2fARB(current_shader->uniform.screen_center, 0.25 , 0.5);

			qglBegin(GL_TRIANGLE_STRIP);
			qglTexCoord2f(0, 0); qglVertex2f(-1, -1);
			qglTexCoord2f(0, 1); qglVertex2f(-1, 1);
			qglTexCoord2f(0.5, 0); qglVertex2f(0, -1);
			qglTexCoord2f(0.5, 1); qglVertex2f(0, 1);
			qglEnd();

			qglUniform2fARB(current_shader->uniform.lens_center, 0.75 -vrState.projOffset * 0.25, 0.5 );
			qglUniform2fARB(current_shader->uniform.screen_center, 0.75 , 0.5);

			qglBegin(GL_TRIANGLE_STRIP);
			qglTexCoord2f(0.5, 0); qglVertex2f(0, -1);
			qglTexCoord2f(0.5, 1); qglVertex2f(0, 1);
			qglTexCoord2f(1, 0); qglVertex2f(1, -1);
			qglTexCoord2f(1, 1); qglVertex2f(1, 1);
			qglEnd();
			qglUseProgramObjectARB(0);


			if (VR_OVR_RenderLatencyTest(debugColor))
			{
				qglColor4fv(debugColor);
				GL_Bind(0);
				qglBegin(GL_TRIANGLE_STRIP);
				qglVertex2f(0.3, -0.4);
				qglVertex2f(0.3, 0.4);
				qglVertex2f(0.7, -0.4);
				qglVertex2f(0.7, 0.4); 
				qglEnd();

				qglBegin(GL_TRIANGLE_STRIP);
				qglVertex2f(-0.3, -0.4);
				qglVertex2f(-0.3, 0.4);
				qglVertex2f(-0.7, -0.4);
				qglVertex2f(-0.7, 0.4); 
				qglEnd();
			}

		} else {
			qglBegin(GL_TRIANGLE_STRIP);
			qglTexCoord2f(0, 0); qglVertex2f(-1, -1);
			qglTexCoord2f(0, 1); qglVertex2f(-1, 1);
			qglTexCoord2f(1, 0); qglVertex2f(1, -1);
			qglTexCoord2f(1, 1); qglVertex2f(1, 1);
			qglEnd();
		}
	}

	GL_Bind(0);

	// flag HMD to refresh next frame

	vrState.stale = 1;
}


void R_VR_InitShader(r_shader_t *shader, r_shaderobject_t *object)
{

	if (!object->program)
		R_CompileShaderProgram(object);

	shader->shader = object;
	qglUseProgramObjectARB(shader->shader->program);

	shader->uniform.scale = qglGetUniformLocationARB(shader->shader->program, "scale");
	shader->uniform.scale_in = qglGetUniformLocationARB(shader->shader->program, "scaleIn");
	shader->uniform.lens_center = qglGetUniformLocationARB(shader->shader->program, "lensCenter");
	shader->uniform.screen_center = qglGetUniformLocationARB(shader->shader->program, "screenCenter");
	shader->uniform.hmd_warp_param = qglGetUniformLocationARB(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = qglGetUniformLocationARB(shader->shader->program, "chromAbParam");

	qglUseProgramObjectARB(0);
}

// enables renderer support for the Rift
void R_VR_Enable()
{
	qboolean success = false;
	float scale = VR_OVR_GetDistortionScale();
	vrState.viewHeight = vid.height;
	vrState.viewWidth = vid.width;
	vrState.vrHalfWidth = vid.width;
	vrState.vrWidth = vid.width;
	vrState.vrHeight = vid.height;


	if (world.valid)
		R_DelFBO(&world);
	if (hud.valid)
		R_DelFBO(&hud);

	Com_Printf("VR: Initializing renderer:");
	
	vrState.vrWidth = scale * vrState.viewWidth;
	vrState.vrHalfWidth = scale * vrState.viewWidth / 2.0;
	vrState.vrHeight = scale * vrState.viewHeight;
	vrState.pixelScale = (float) vrState.vrWidth / (float) vrConfig.hmdWidth;
	success = R_GenFBO(vrState.hudWidth,vrState.hudHeight,&hud);
	success = success && R_GenFBO(vrState.vrWidth,vrState.vrHeight,&world);

	if (!success)
	{
		Com_Printf(" failed!\n");
		Cmd_ExecuteString("vr_disable");
	} else {
		Com_Printf(" ok!\n");
	}
	Com_Printf("VR: Using %u x %u backbuffer\n",vrState.vrWidth,vrState.vrHeight);
}

// disables renderer support for the Rift
void R_VR_Disable()
{
	qglBindFramebuffer(GL_FRAMEBUFFER,defaultFBO);

	qglViewport(0,0,vrState.viewWidth,vrState.viewHeight);

	vid.width = vrState.viewWidth;
	vid.height = vrState.viewHeight;

	vrState.vrWidth = vrState.viewWidth;
	vrState.vrHalfWidth = vrState.viewWidth;
	vrState.vrHeight = vrState.viewHeight;
	vrState.pixelScale = 1.0;

	if (world.valid)
		R_DelFBO(&world);
	if (hud.valid)
		R_DelFBO(&hud);
}



// launch-time initialization for Rift support
void R_VR_Init()
{
	if (glConfig.arb_framebuffer_object && glConfig.arb_shader_objects)
	{
		vrState.hudHeight = 480;
		vrState.hudWidth = 640;

		qglGetIntegerv(GL_FRAMEBUFFER_BINDING,&defaultFBO);
		R_InitFBO(&world);
		R_InitFBO(&hud);

		R_VR_InitShader(&ovr_shaders[0],&ovr_shader_norm);
		R_VR_InitShader(&ovr_shaders[1],&ovr_shader_chrm);


		vrState.eye = EYE_NONE;

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

	R_DelShaderProgram(&ovr_shader_norm);
	R_DelShaderProgram(&ovr_shader_chrm);

	if (world.valid)
		R_DelFBO(&world);
	if (hud.valid)
		R_DelFBO(&hud);
}
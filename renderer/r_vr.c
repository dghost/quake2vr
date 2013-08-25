#include "r_local.h"
#include "../vr/vr.h"

typedef struct {
	GLuint framebuffer;
	GLuint texture;
	GLuint depthbuffer;
	int width, height;
	int valid;
} r_fbo_t;

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
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
	} uniform;

} r_shader_t;

r_shader_t lens_warp_shaders[2];

r_shader_t *lens_warp;

r_fbo_t left, right, hud; 

GLint defaultFBO;

// Default Lens Warp Shader
static r_shaderobject_t lens_warp_shader_norm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 vUv;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"vUv = vec2(gl_MultiTexCoord0);\n"
	"}\n",

	// fragment shader
	"varying vec2 vUv;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform sampler2D texture;\n"
	"void main()\n"
	"{\n"
		"vec2 uv = (vUv*2.0)-1.0;\n" // range from [0,1] to [-1,1]
		"vec2 theta = (uv-lensCenter)*scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"vec2 tc = (lensCenter + scale * rvector);\n"
		"tc = (tc+1.0)/2.0;\n" // range from [-1,1] to [0,1]
		"if (any(bvec2(clamp(tc, vec2(0.0,0.0), vec2(1.0,1.0))-tc)))\n"
			"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"else\n"
			"gl_FragColor = texture2D(texture, tc);\n"
	"}\n"
};

// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t lens_warp_shader_chrm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 vUv;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"vUv = vec2(gl_MultiTexCoord0);\n"
	"}\n",

	// fragment shader
	"varying vec2 vUv;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform vec4 chromAbParam;\n"
	"uniform sampler2D texture;\n"

	// Scales input texture coordinates for distortion.
	// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
	// larger due to aspect ratio.
	"void main()\n"
	"{\n"
		"vec2 uv = (vUv*2.0)-1.0;\n" // range from [0,1] to [-1,1]
		"vec2 theta = (uv-lensCenter)*scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 theta1 = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"

		// Detect whether blue texture coordinates are out of range since these will scaled out the furthest.
		"vec2 thetaBlue = theta1 * (chromAbParam.z + chromAbParam.w * rSq);\n"
		"vec2 tcBlue = lensCenter + scale * thetaBlue;\n"
		"tcBlue = (tcBlue+1.0)/2.0;\n" // range from [-1,1] to [0,1]
		"if (any(bvec2(clamp(tcBlue, vec2(0.0,0.0), vec2(1.0,1.0))-tcBlue)))\n"
		"{\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
			"return;\n"
		"}\n"

		// Now do blue texture lookup.
		"float blue = texture2D(texture, tcBlue).b;\n"

		// Do green lookup (no scaling).
		"vec2  tcGreen = lensCenter + scale * theta1;\n"
		"tcGreen = (tcGreen+1.0)/2.0;\n" // range from [-1,1] to [0,1]
		"vec4  center = texture2D(texture, tcGreen);\n"

		// Do red scale and lookup.
		"vec2  thetaRed = theta1 * (chromAbParam.x + chromAbParam.y * rSq);\n"
		"vec2  tcRed = lensCenter + scale * thetaRed;\n"
		"tcRed = (tcRed+1.0)/2.0;\n" // range from [-1,1] to [0,1]
		"float red = texture2D(texture, tcRed).r;\n"

		"gl_FragColor = vec4(red, center.g, blue, center.a);\n"
	"}\n"
};

#define PLAYER_HEIGHT_UNITS 56.0f
#define PLAYER_HEIGHT_M 1.75f

//
// Utility Functions
//

int R_GenFBO(int width, int height, r_fbo_t *FBO)
{


	GLuint fbo, tex, dep;

	qglGenFramebuffers(1,&fbo);
	qglGenTextures(1,&tex);
	qglGenRenderbuffers(1,&dep);
	GL_SelectTexture(0);
	GL_Bind(tex);
    qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	qglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
	GL_Bind(0);

	qglBindRenderbuffer(GL_RENDERBUFFER,dep);
	qglRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,width,height);
	qglBindRenderbuffer(GL_RENDERBUFFER,0);
	
	qglBindFramebuffer(GL_FRAMEBUFFER,fbo);
	qglFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
	qglFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,dep);


	if (qglCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Com_Printf("ERROR: Creation of %i x %i FBO failed!\n", width, height);

		qglDeleteTextures(1,&tex);
		qglDeleteRenderbuffers(1,&dep);
		
		qglDeleteFramebuffers(1,&fbo);
		return 0;
	} else {
		FBO->framebuffer = fbo;
		FBO->texture = tex;
		FBO->depthbuffer = dep;
		FBO->width = width;
		FBO->height = height;
		FBO->valid = 1;
		return 1;
	}
}


void R_DelFBO(r_fbo_t *FBO)
{
	qglDeleteFramebuffers(1,&FBO->framebuffer);
	qglDeleteTextures(1,&FBO->texture);
	qglDeleteRenderbuffers(1,&FBO->depthbuffer);

	FBO->height = 0;
	FBO->width = 0;
	FBO->valid = 0;

}

void R_InitFBO(r_fbo_t *FBO)
{
	FBO->framebuffer = 0;
	FBO->texture = 0;
	FBO->depthbuffer = 0;
	FBO->height = 0;
	FBO->width = 0;
	FBO->valid = 0;

}

GLuint R_BindFBO(r_fbo_t *FBO)
{
	GLint currentFrameBuffer = 0;
	qglGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFrameBuffer);
	qglBindFramebuffer(GL_FRAMEBUFFER,FBO->framebuffer);
	qglViewport(0,0,FBO->width,FBO->height);
	return (GLuint) currentFrameBuffer;
}

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
	vrState.viewOffset = (vr_ipd->value / 2000.0) * PLAYER_HEIGHT_UNITS / PLAYER_HEIGHT_M;
	
	
	if (vr_ovr_scale->modified)
	{
		if (vr_ovr_scale->value < 1.0)
			Cvar_Set("vr_ovr_scale","1.0");
		if (vr_ovr_scale->value > 2.0)
			Cvar_Set("vr_ovr_scale","2.0");

		R_DelFBO(&left);
		R_DelFBO(&right);

		vrState.vrWidth = vr_ovr_scale->value * vrState.viewWidth;
		vrState.vrHalfWidth = vr_ovr_scale->value  * vrState.viewWidth / 2.0;
		vrState.vrHeight = vr_ovr_scale->value  * vrState.viewHeight;
		R_GenFBO(vrState.vrHalfWidth,vrState.vrHeight,&left);
		R_GenFBO(vrState.vrHalfWidth,vrState.vrHeight,&right);
		VR_OVR_SetFOV();
		vrState.pixelScale = (vrState.vrWidth) / vrConfig.hmdWidth;
		Com_Printf("VR: Calculated %.2f FOV\n",vrState.viewFovY);
		Com_Printf("VR: Using %u x %u backbuffer\n",vrState.vrWidth,vrState.vrHeight);
		vr_ovr_scale->modified = false;

	}


	if (vr_ovr_chromatic->value)
		lens_warp = &lens_warp_shaders[1];
	else
		lens_warp = &lens_warp_shaders[0];


	qglClearColor(0.0,0.0,0.0,0.0);
	R_VR_BindLeft();

	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	R_VR_BindRight();

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
		R_BindFBO(&left);

		vid.height = left.height;
		vid.width = left.width;
		vrState.eye = EYE_LEFT;
	}
}

void R_VR_BindRight()
{
	if (vr_enabled->value)
	{
		R_BindFBO(&right);

		vid.height = right.height;
		vid.width = right.width;
		vrState.eye = EYE_RIGHT;

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

void R_VR_DrawHud()
{
	float fov = vr_hud_fov->value;
	float y,x;
	float depth = vr_hud_depth->value;
	float farz = 300.0;
	float nearz = 0.01;
	float f = 1.0f / tanf((vrState.viewFovY / 2.0f) * M_PI / 180);
	float nf = 1.0f / (nearz - farz);
	float out[16];
	float bounce = vr_hud_bounce->value;
	vec3_t orientation;
	extern int scr_draw_loading;
	out[0] = f / vrConfig.aspect;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;
	out[4] = 0;
	out[5] = f;
	out[6] = 0;
	out[7] = 0;
	out[8] = vrState.eye * vrState.projOffset;
	out[9] = 0;
	out[10] = (farz + nearz) * nf;
	out[11] = -1;
	out[12] = 0;
	out[13] = 0;
	out[14] = (2.0f * farz * nearz) * nf;
	out[15] = 0;

	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(out);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	// disable this for the loading screens since they are not at 60fps
	if (vr_hud_bounce->value && !scr_draw_loading)
	{
		VR_GetOrientationEMA(orientation);
		qglRotatef (orientation[0] * bounce,  1, 0, 0);
		qglRotatef (orientation[1] * bounce,  0, -1, 0);
		qglRotatef (orientation[2] * bounce,  0, 0, 1);
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

	qglBegin(GL_QUADS);
	qglTexCoord2f (0, 0); qglVertex3f (-x, -y,-depth);
	qglTexCoord2f (1, 0); qglVertex3f (x, -y,-depth);
	qglTexCoord2f (1, 1); qglVertex3f (x, y,-depth);
	qglTexCoord2f (0, 1); qglVertex3f (-x, y,-depth);		
	qglEnd();

	GL_Bind(0);
	GL_Disable(GL_BLEND);
}

// takes the various FBO's and renders them to the framebuffer
void R_VR_Present()
{
	if (vr_enabled->value)
	{
		float scale = vr_ovr_scale->value;
	
		GL_Disable(GL_DEPTH_TEST);
		GL_SelectTexture(0);

		R_VR_BindLeft();
		R_VR_DrawHud();

		R_VR_BindRight();
		R_VR_DrawHud();

		R_VR_EndFrame();

		GL_Disable(GL_BLEND);
		GL_Disable(GL_ALPHA_TEST);
		GL_Disable(GL_DEPTH_TEST);

		qglMatrixMode(GL_PROJECTION);
		qglLoadIdentity ();

		qglMatrixMode(GL_MODELVIEW);
		qglLoadIdentity ();

		qglUseProgramObjectARB(lens_warp->shader->program);
		qglUniform2fARB(lens_warp->uniform.lens_center, vrState.projOffset , 0);
		qglUniform2fARB(lens_warp->uniform.scale, 1.0f/scale, 1.0f * vrConfig.aspect/scale);
		qglUniform4fvARB(lens_warp->uniform.chrom_ab_param,1,vrConfig.chrm);
		qglUniform4fvARB(lens_warp->uniform.hmd_warp_param,1,vrConfig.dk);
		qglUniform2fARB(lens_warp->uniform.scale_in, 1.0f, 1.0f/vrConfig.aspect);

		GL_Bind(left.texture);

		qglBegin(GL_QUADS);			
		qglTexCoord2f (0, 0); qglVertex2f (-1, -1);
		qglTexCoord2f (1, 0); qglVertex2f (0, -1);
		qglTexCoord2f (1, 1); qglVertex2f (0, 1);
		qglTexCoord2f (0, 1); qglVertex2f (-1, 1);	
		qglEnd();

		qglUniform2fARB(lens_warp->uniform.lens_center, -vrState.projOffset , 0);
		qglUniform2fARB(lens_warp->uniform.scale, 1.0f/scale, 1.0f * vrConfig.aspect/scale);
		qglUniform4fvARB(lens_warp->uniform.chrom_ab_param,1,vrConfig.chrm);
		qglUniform4fvARB(lens_warp->uniform.hmd_warp_param,1,vrConfig.dk);
		qglUniform2fARB(lens_warp->uniform.scale_in, 1.0f, 1.0f/vrConfig.aspect);

		GL_Bind(right.texture);

		qglBegin(GL_QUADS);		
		qglTexCoord2f (0, 0); qglVertex2f (0, -1);
		qglTexCoord2f (1, 0); qglVertex2f (1, -1);
		qglTexCoord2f (1, 1); qglVertex2f (1, 1);
		qglTexCoord2f (0, 1); qglVertex2f (0, 1);		
		qglEnd();
	
		GL_Bind(0);
		qglUseProgramObjectARB(0);	
	}
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
	shader->uniform.hmd_warp_param = qglGetUniformLocationARB(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = qglGetUniformLocationARB(shader->shader->program, "chromAbParam");

	qglUseProgramObjectARB(0);
}

// enables renderer support for the Rift
void R_VR_Enable()
{
	qboolean success = false;
//	if (!vr_enabled->value)
//		VR_Enable();

	vrState.viewHeight = vid.height;
	vrState.viewWidth = vid.width;
	vrState.vrHalfWidth = vid.width;
	vrState.vrWidth = vid.width;
	vrState.vrHeight = vid.height;


	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (hud.valid)
		R_DelFBO(&hud);

	Com_Printf("VR: Initializing renderer:");
	
	vrState.vrWidth = vr_ovr_scale->value * vrState.viewWidth;
	vrState.vrHalfWidth = vr_ovr_scale->value * vrState.viewWidth / 2.0;
	vrState.vrHeight = vr_ovr_scale->value * vrState.viewHeight;
	vrState.pixelScale = (vrState.vrWidth) / vrConfig.hmdWidth;	 
	success = R_GenFBO(vrState.hudWidth,vrState.hudHeight,&hud);
	success = success && R_GenFBO(vrState.vrHalfWidth,vrState.vrHeight,&left);
	success = success && R_GenFBO(vrState.vrHalfWidth,vrState.vrHeight,&right);

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
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
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
		R_InitFBO(&left);
		R_InitFBO(&right);
		R_InitFBO(&hud);

		R_VR_InitShader(&lens_warp_shaders[0],&lens_warp_shader_norm);
		R_VR_InitShader(&lens_warp_shaders[1],&lens_warp_shader_chrm);

		lens_warp = &lens_warp_shaders[0];

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

	R_DelShaderProgram(&lens_warp_shader_norm);
	R_DelShaderProgram(&lens_warp_shader_chrm);

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (hud.valid)
		R_DelFBO(&hud);
}
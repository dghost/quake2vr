#include "include/r_vr_ovr.h"
#include "../vr/include/vr_ovr.h"
#include "include/r_local.h"


void OVR_FrameStart(int32_t changeBackBuffers);
void OVR_BindView(vr_eye_t eye);
void OVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect);
void OVR_Present();
int32_t OVR_Enable();
void OVR_Disable();
int32_t OVR_Init();
void OVR_GetState(vr_param_t *state);

hmd_render_t vr_render_ovr = 
{
	HMD_RIFT,
	OVR_Init,
	OVR_Enable,
	OVR_Disable,
	OVR_BindView,
	OVR_GetViewRect,
	OVR_FrameStart,
	OVR_GetState,
	OVR_Present
};


extern void VR_OVR_CalcRenderParam();
extern float VR_OVR_GetDistortionScale();
extern void VR_OVR_GetFOV(float *fovx, float *fovy);
extern int32_t VR_OVR_RenderLatencyTest(vec4_t color);


//static fbo_t world;
static fbo_t left, right;
static fbo_t leftDistortion, rightDistortion;

static vr_rect_t renderTargetRect;

static qboolean useBilinear, useChroma;

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_dist_norm = {
	0, 
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/ovrdistort.frag"

};

// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t ovr_shader_dist_chrm = {
	0, 
	// vertex shader (identity)
	"vr/common.vert",
	// fragment shader
	"vr/ovrdistort_chromatic.frag"
};




typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint scale;
		GLuint scale_in;
		GLuint lens_center;
		GLuint texture_size;
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
		GLuint tex;
		GLuint dist;
	} uniform;

} r_ovr_shader_t;

typedef enum {
	OVR_FILTER_BILINEAR,
	OVR_FILTER_WEIGHTED_BILINEAR,
	OVR_FILTER_BICUBIC,
	NUM_OVR_FILTER_MODES
} ovr_filtermode_t;

static r_ovr_shader_t ovr_distortion_shaders[2];

// util function
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{

	if (!object->program)
		R_CompileShaderFromFiles(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);

	shader->uniform.scale = glGetUniformLocation(shader->shader->program, "scale");
	shader->uniform.scale_in = glGetUniformLocation(shader->shader->program, "scaleIn");
	shader->uniform.lens_center = glGetUniformLocation(shader->shader->program, "lensCenter");
	shader->uniform.hmd_warp_param = glGetUniformLocation(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = glGetUniformLocation(shader->shader->program, "chromAbParam");
	shader->uniform.texture_size = glGetUniformLocation(shader->shader->program,"textureSize");
	shader->uniform.tex = glGetUniformLocation(shader->shader->program,"tex");
	shader->uniform.dist = glGetUniformLocation(shader->shader->program,"dist");
	glUseProgram(0);
}



void OVR_RenderDistortion()
{
	float resScale = 1.0;
	uint32_t width;
	uint32_t height;
	GLuint currentFBO;
	float scale = VR_OVR_GetDistortionScale();
	r_ovr_shader_t *current = &ovr_distortion_shaders[useChroma];
	// draw left eye

	currentFBO = glState.currentFBO;
	if (vr_ovr_distortion->value > 0)
	{
		resScale = (1.0 / pow(2,vr_ovr_distortion->value - 1 ));
		width = ovrConfig.hmdWidth * 0.5 * resScale;
		height = ovrConfig.hmdHeight * resScale;
	} else {
		width = Cvar_VariableInteger("vid_width") * 0.5;
		height = Cvar_VariableInteger("vid_height");
	}
	Com_Printf("VR_OVR: Generating %dx%d distortion textures.\n",width,height);
	glUseProgram(current->shader->program);

	glUniform4fv(current->uniform.chrom_ab_param, 1, ovrConfig.chrm);
	glUniform4fv(current->uniform.hmd_warp_param, 1, ovrConfig.dk);
	glUniform2f(current->uniform.scale_in, 2.0f, 2.0f / ovrConfig.aspect);
	glUniform2f(current->uniform.scale, 0.5f / scale, 0.5f * ovrConfig.aspect / scale);
	glUniform4f(current->uniform.texture_size, width, height, 1.0 / width, 1.0 / height);
	glUniform2f(current->uniform.lens_center, 0.5 + ovrConfig.projOffset * 0.5, 0.5);
	
	R_ResizeFBO(width,height,true,&leftDistortion);
	GL_MBind(0,leftDistortion.texture);
	if (!useChroma && glConfig.arb_texture_rg)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	GL_MBind(0,0);
	R_BindFBO(&leftDistortion);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, 0); glVertex2f(-1, -1);
	glTexCoord2f(0, 1); glVertex2f(-1, 1);
	glTexCoord2f(1, 0); glVertex2f(1, -1);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glEnd();

	// draw right eye
	glUniform2f(current->uniform.lens_center, 0.5 - ovrConfig.projOffset * 0.5, 0.5 );
	
	R_ResizeFBO(width,height,true,&rightDistortion);
	
	GL_MBind(0,rightDistortion.texture);
	if (!useChroma && glConfig.arb_texture_rg)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	GL_MBind(0,0);


	R_BindFBO(&rightDistortion);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, 0); glVertex2f(-1, -1);
	glTexCoord2f(0, 1); glVertex2f(-1, 1);
	glTexCoord2f(1, 0); glVertex2f(1, -1);
	glTexCoord2f(1, 1); glVertex2f(1, 1);
	glEnd();
	glUseProgram(0);

	GL_BindFBO(currentFBO);
}

void OVR_FrameStart(int32_t changeBackBuffers)
{


	if (vr_ovr_distortion->modified)
	{
		if (vr_ovr_distortion->value > 3)
			Cvar_SetInteger("vr_ovr_distortion", 3);
		else if (vr_ovr_distortion->value < -1)
			Cvar_SetInteger("vr_ovr_distortion", -1);
		changeBackBuffers = 1;
		vr_ovr_distortion->modified = false;
	}

	if (vr_ovr_scale->modified)
	{
		if (vr_ovr_scale->value < 1.0)
			Cvar_Set("vr_ovr_scale", "1.0");
		else if (vr_ovr_scale->value > 2.0)
			Cvar_Set("vr_ovr_scale", "2.0");
		changeBackBuffers = 1;
		vr_ovr_scale->modified = false;
	}

	if (vr_ovr_autoscale->modified)
	{
		if (vr_ovr_autoscale->value < 0.0)
			Cvar_SetInteger("vr_ovr_autoscale", 0);
		else if (vr_ovr_autoscale->value > 2.0)
			Cvar_SetInteger("vr_ovr_autoscale",2);
		else
			Cvar_SetInteger("vr_ovr_autoscale", (int32_t) vr_ovr_autoscale->value);
		changeBackBuffers = 1;
		vr_ovr_autoscale->modified = false;
	}

	if (vr_ovr_lensdistance->modified)
	{
		if (vr_ovr_lensdistance->value < 0.0)
			Cvar_SetToDefault("vr_ovr_lensdistance");
		else if (vr_ovr_lensdistance->value > 100.0)
			Cvar_Set("vr_ovr_lensdistance", "100.0");
		changeBackBuffers = 1;
		VR_OVR_CalcRenderParam();
		vr_ovr_lensdistance->modified = false;
	}

	if (vr_ovr_autolensdistance->modified)
	{
		Cvar_SetValue("vr_ovr_autolensdistance",!!vr_ovr_autolensdistance->value);
		changeBackBuffers = 1;
		VR_OVR_CalcRenderParam();
		vr_ovr_autolensdistance->modified = false;
	}

	if (vr_ovr_supersample->modified)
	{
		if (vr_ovr_supersample->value < 1.0)
			Cvar_Set("vr_ovr_supersample", "1.0");
		else if (vr_ovr_supersample->value > 2.0)
			Cvar_Set("vr_ovr_supersample", "2.0");
		changeBackBuffers = 1;
		vr_ovr_supersample->modified = false;
	}
	if (useChroma != (qboolean) !!vr_chromatic->value)
	{
		useChroma = (qboolean) !!vr_chromatic->value;
		changeBackBuffers = 1;
	}

	if (vr_ovr_filtermode->modified)
	{
		switch ((int32_t) vr_ovr_filtermode->value)
		{
		default:
		case OVR_FILTER_BILINEAR:
		case OVR_FILTER_BICUBIC:
			useBilinear = 1;
			break;
		case OVR_FILTER_WEIGHTED_BILINEAR:
			useBilinear = 0;
			break;
		}
//		Com_Printf("OVR: %s bilinear texture filtering\n", useBilinear ? "Using" : "Not using");
		R_SetFBOFilter(useBilinear,&left);
		R_SetFBOFilter(useBilinear,&right);
		vr_ovr_filtermode->modified = false;
	}

	if (changeBackBuffers)
	{
		float ovrScale = VR_OVR_GetDistortionScale() *  vr_ovr_supersample->value;
		renderTargetRect.width = ovrScale * vid.width * 0.5;
		renderTargetRect.height = ovrScale  * vid.height;
		Com_Printf("VR_OVR: Set render target size to %ux%u\n",renderTargetRect.width,renderTargetRect.height);
		OVR_RenderDistortion();		
	}
}


void OVR_BindView(vr_eye_t eye)
{
	
	switch(eye)
	{
	case EYE_LEFT:
		if (renderTargetRect.width != left.width || renderTargetRect.height != left.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, useBilinear, &left);
	//		Com_Printf("OVR: Set left render target size to %ux%u\n",left.width,left.height);
		}
		vid.height = left.height;
		vid.width = left.width;
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:
		if (renderTargetRect.width != right.width || renderTargetRect.height != right.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, useBilinear, &right);
	//		Com_Printf("OVR: Set right render target size to %ux%u\n",right.width, right.height);
		}
		vid.height = right.height;
		vid.width = right.width;
		R_BindFBO(&right);
		break;
	default:
		return;
	}
}

void OVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect)
{
	*rect = renderTargetRect;
}


void OVR_GetState(vr_param_t *state)
{
		vr_param_t ovrState;
		float ovrScale = VR_OVR_GetDistortionScale() *  vr_ovr_supersample->value;
		VR_OVR_GetFOV(&ovrState.viewFovX, &ovrState.viewFovY);
		ovrState.projOffset = ovrConfig.projOffset;
		ovrState.ipd = ovrConfig.ipd;
		ovrState.pixelScale = ovrScale * vid.width / (int32_t) ovrConfig.hmdWidth;
		ovrState.aspect = ovrConfig.aspect;
		*state = ovrState;
}

void OVR_Present()
{
	vec4_t debugColor;

	if (vr_ovr_distortion->value >= 0)
	{

		float superscale = vr_ovr_supersample->value;
		vr_distort_shader_t *current_shader;
		if (vr_ovr_filtermode->value)
			current_shader = &vr_bicubic_distort_shaders[!!(int32_t) vr_chromatic->value];
		else
			current_shader = &vr_distort_shaders[!!(int32_t) vr_chromatic->value];

		// draw left eye
		glUseProgram(current_shader->shader->program);

		glUniform4f(current_shader->uniform.texture_size, renderTargetRect.width / superscale, renderTargetRect.height / superscale, superscale / renderTargetRect.width, superscale / renderTargetRect.height);
		glUniform1i(current_shader->uniform.texture,0);
		glUniform1i(current_shader->uniform.distortion,1);

		GL_MBind(0,left.texture);
		GL_MBind(1,leftDistortion.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(0, -1);
		glTexCoord2f(1, 1); glVertex2f(0, 1);
		glEnd();

		// draw right eye
		GL_MBind(0,right.texture);
		GL_MBind(1,rightDistortion.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(0, -1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();
		glUseProgram(0);
		
		GL_MBind(1,0);
		GL_MBind(0,0);

	} else {
		GL_MBind(0,left.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(0, -1);
		glTexCoord2f(1, 1); glVertex2f(0, 1);
		glEnd();

		GL_MBind(0,right.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(0, -1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();
		GL_MBind(0,0);
	}
	if (VR_OVR_RenderLatencyTest(debugColor))
	{
		glColor4fv(debugColor);

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(0.3, -0.4);
		glVertex2f(0.3, 0.4);
		glVertex2f(0.7, -0.4);
		glVertex2f(0.7, 0.4); 
		glEnd();

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(-0.3, -0.4);
		glVertex2f(-0.3, 0.4);
		glVertex2f(-0.7, -0.4);
		glVertex2f(-0.7, 0.4); 
		glEnd();

		glColor4f(1.0,1.0,1.0,1.0);
	}

}


int32_t OVR_Enable()
{
	if (!glConfig.arb_texture_float)
		return 0;
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (leftDistortion.valid)
		R_DelFBO(&leftDistortion);
	if (rightDistortion.valid)
		R_DelFBO(&rightDistortion);
	
	VR_OVR_InitShader(&ovr_distortion_shaders[0],&ovr_shader_dist_norm);
	VR_OVR_InitShader(&ovr_distortion_shaders[1],&ovr_shader_dist_chrm);
	//OVR_FrameStart(true);
	Cvar_ForceSet("vr_hmdstring",ovrConfig.deviceString);
	return true;
}

void OVR_Disable()
{

	R_DelShaderProgram(&ovr_shader_dist_norm);
	R_DelShaderProgram(&ovr_shader_dist_chrm);

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
	if (leftDistortion.valid)
		R_DelFBO(&leftDistortion);
	if (rightDistortion.valid)
		R_DelFBO(&rightDistortion);
}

int32_t OVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	R_InitFBO(&leftDistortion);
	R_InitFBO(&rightDistortion);
	return true;
}
#include "r_vr_ovr.h"
#include "../vr/vr_ovr.h"
#include "../renderer/r_local.h"

hmd_render_t vr_render_ovr = 
{
	HMD_RIFT,
	R_VR_OVR_Init,
	R_VR_OVR_Enable,
	R_VR_OVR_Disable,
	R_VR_OVR_BindView,
	R_VR_OVR_FrameStart,
	R_VR_OVR_Present
};


static r_ovr_shader_t ovr_shaders[2];
static r_ovr_shader_t ovr_bicubic_shaders[2];

//static fbo_t world;
static fbo_t left, right;

// util function
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object)
{

	if (!object->program)
		R_CompileShaderProgram(object);

	shader->shader = object;
	glUseProgramObjectARB(shader->shader->program);

	shader->uniform.scale = glGetUniformLocationARB(shader->shader->program, "scale");
	shader->uniform.scale_in = glGetUniformLocationARB(shader->shader->program, "scaleIn");
	shader->uniform.lens_center = glGetUniformLocationARB(shader->shader->program, "lensCenter");
	shader->uniform.screen_center = glGetUniformLocationARB(shader->shader->program, "screenCenter");
	shader->uniform.hmd_warp_param = glGetUniformLocationARB(shader->shader->program, "hmdWarpParam");
	shader->uniform.chrom_ab_param = glGetUniformLocationARB(shader->shader->program, "chromAbParam");
	shader->uniform.texture_size = glGetUniformLocationARB(shader->shader->program,"textureSize");
	glUseProgramObjectARB(0);
}



void R_VR_OVR_FrameStart(int changeBackBuffers)
{


	if (vr_ovr_distortion->modified)
	{
		Cvar_SetInteger("vr_ovr_distortion", !!(int) vr_ovr_distortion->value);
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
			Cvar_SetInteger("vr_ovr_autoscale", (int) vr_ovr_autoscale->value);
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

	if (changeBackBuffers)
	{
		float fsaaScale = vr_antialias->value == VR_ANTIALIAS_FSAA? 2.0f : 1.0f;
		float worldScale = VR_OVR_GetDistortionScale() * fsaaScale * vr_ovr_supersample->value;
		//R_DelFBO(&world);

		vrState.vrWidth = worldScale * vrState.viewWidth;
		vrState.vrHalfWidth = vrState.vrWidth / 2.0;
		vrState.vrHeight = worldScale  * vrState.viewHeight;
		R_ResizeFBO(vrState.vrHalfWidth, vrState.vrHeight, &left);
		R_ResizeFBO(vrState.vrHalfWidth, vrState.vrHeight, &right);
	
		VR_OVR_SetFOV();
		vrState.pixelScale = (float) vrState.vrWidth / (float) vrConfig.hmdWidth;
	}
	R_BindFBO(&left);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	R_BindFBO(&right);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

}


void R_VR_OVR_BindView(vr_eye_t eye)
{


	vid.height = vrState.vrHeight;
	vid.width = vrState.vrHalfWidth;

	switch(eye)
	{
	case EYE_LEFT:
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:
		R_BindFBO(&right);
		break;
	case EYE_HUD:
		break;

	}
}

void R_VR_OVR_Present()
{
	if (vr_ovr_distortion->value)
	{

		float scale = VR_OVR_GetDistortionScale();
		//float superscale = (vr_antialias->value == VR_ANTIALIAS_FSAA ? 2.0f : 1.0f);
		float superscale = vr_ovr_supersample->value;
		r_ovr_shader_t *current_shader;
		vec4_t debugColor;
		if (vr_ovr_bicubic->value)
			current_shader = &ovr_bicubic_shaders[!!(int) vr_ovr_chromatic->value];
		else
			current_shader = &ovr_shaders[!!(int) vr_ovr_chromatic->value];

		// draw left eye

		glUseProgramObjectARB(current_shader->shader->program);

		glUniform4fvARB(current_shader->uniform.chrom_ab_param, 1, vrConfig.chrm);
		glUniform4fvARB(current_shader->uniform.hmd_warp_param, 1, vrConfig.dk);
		glUniform2fARB(current_shader->uniform.scale_in, 2.0f, 2.0f / vrConfig.aspect);
		glUniform2fARB(current_shader->uniform.scale, 0.5f / scale, 0.5f * vrConfig.aspect / scale);
		glUniform4fARB(current_shader->uniform.texture_size, vrState.vrWidth / superscale, vrState.vrHeight / superscale, superscale / vrState.vrWidth, superscale / vrState.vrHeight);
		//glUniform2fARB(current_shader->uniform.texture_size, vrState.viewWidth, vrState.viewHeight);

		glUniform2fARB(current_shader->uniform.lens_center, 0.5 + vrState.projOffset * 0.5, 0.5);
		glUniform2fARB(current_shader->uniform.screen_center, 0.5 , 0.5);
		GL_Bind(left.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(0, -1);
		glTexCoord2f(1, 1); glVertex2f(0, 1);
		glEnd();

		// draw right eye
		glUniform2fARB(current_shader->uniform.lens_center, 0.5 - vrState.projOffset * 0.5, 0.5 );
		glUniform2fARB(current_shader->uniform.screen_center, 0.5 , 0.5);
		GL_Bind(right.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(0, -1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();
		glUseProgramObjectARB(0);
					GL_Bind(0);

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
		}


	} else {
		GL_Bind(left.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(0, 1); glVertex2f(-1, 1);
		glTexCoord2f(1, 0); glVertex2f(0, -1);
		glTexCoord2f(1, 1); glVertex2f(0, 1);
		glEnd();

		GL_Bind(right.texture);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(0, -1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glTexCoord2f(1, 0); glVertex2f(1, -1);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glEnd();
		GL_Bind(0);
	}

}

int R_VR_OVR_Enable()
{
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

	R_VR_OVR_FrameStart(true);

	return (left.framebuffer && right.framebuffer);
}

void R_VR_OVR_Disable()
{
	R_DelShaderProgram(&ovr_shader_norm);
	R_DelShaderProgram(&ovr_shader_chrm);
	R_DelShaderProgram(&ovr_shader_bicubic_norm);
	R_DelShaderProgram(&ovr_shader_bicubic_chrm);

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);
}

int R_VR_OVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	VR_OVR_InitShader(&ovr_shaders[0],&ovr_shader_norm);
	VR_OVR_InitShader(&ovr_shaders[1],&ovr_shader_chrm);
	VR_OVR_InitShader(&ovr_bicubic_shaders[0],&ovr_shader_bicubic_norm);
	VR_OVR_InitShader(&ovr_bicubic_shaders[1],&ovr_shader_bicubic_chrm);
	return true;

}
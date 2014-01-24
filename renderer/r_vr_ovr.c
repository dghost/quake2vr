#include "r_vr_ovr.h"
#include "../vr/vr_ovr.h"
#include "../renderer/r_local.h"

hmd_render_t vr_render_ovr = 
{
	HMD_RIFT,
	OVR_Init,
	OVR_Enable,
	OVR_Disable,
	OVR_BindView,
	OVR_GetViewPos,
	OVR_GetViewSize,
	OVR_FrameStart,
	OVR_Present
};


static r_ovr_shader_t ovr_shaders[2];
static r_ovr_shader_t ovr_bicubic_shaders[2];

//static fbo_t world;
static fbo_t left, right;

static unsigned int renderTargetSize[2];

static unsigned useBilinear;

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_norm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"texCoords = gl_MultiTexCoord0.xy;\n"
	"}\n",
	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform sampler2D texture;\n"
	"vec2 warp(vec2 uv)\n"
	"{\n"
		"vec2 theta = (vec2(uv) - lensCenter) * scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"return (lensCenter + scale * rvector);\n"
	"}\n"
	"void main()\n"
	"{\n"
		"vec2 tc = warp(texCoords);\n"
		"if (any(bvec2(clamp(tc, screenCenter - vec2(0.5,0.5), screenCenter + vec2(0.5,0.5))-tc)))\n"
			"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"else\n"
			"gl_FragColor = texture2D(texture, tc);\n"
	"}\n"

};

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_bicubic_norm = {
	0, 0, 0,

	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
	"gl_Position = gl_Vertex;\n"
	"texCoords = gl_MultiTexCoord0.xy;\n"
	"}\n",

	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec4 textureSize;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform sampler2D texture;\n"
	// hack to fix artifacting with AMD cards
	"vec3 sampleClamp(sampler2D tex, vec2 uv)\n"
	"{\n"
		"return texture2D(tex,clamp(uv,vec2(0.0),vec2(1.0))).rgb;\n"
	"}\n"
	"vec2 warp(vec2 uv)\n"
	"{\n"
		"vec2 theta = (vec2(uv) - lensCenter) * scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"return (lensCenter + scale * rvector);\n"
	"}\n"
	"vec3 filter(sampler2D texture, vec2 texCoord)\n"
	"{\n"
		// calculate the center of the texel
		"texCoord *= textureSize.xy;\n"
		"vec2 texelCenter   = floor( texCoord - 0.5 ) + 0.5;\n"
		"vec2 fracOffset    = texCoord - texelCenter;\n"
		"vec2 fracOffset_x2 = fracOffset * fracOffset;\n"
		"vec2 fracOffset_x3 = fracOffset * fracOffset_x2;\n"
		// calculate bspline weight function
		"vec2 weight0 = fracOffset_x2 - 0.5 * ( fracOffset_x3 + fracOffset );\n"
		"vec2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;\n"
		"vec2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );\n"
		"vec2 weight2 = 1.0 - weight0 - weight1 - weight3;\n"
		// calculate texture coordinates
		"vec2 scalingFactor0 = weight0 + weight1;\n"
		"vec2 scalingFactor1 = weight2 + weight3;\n"
		"vec2 f0 = weight1 / ( weight0 + weight1 );\n"
		"vec2 f1 = weight3 / ( weight2 + weight3 );\n"
		"vec2 texCoord0 = texelCenter - 1.0 + f0;\n"
		"vec2 texCoord1 = texelCenter + 1.0 + f1;\n"
		"texCoord0 *= textureSize.zw;\n"
		"texCoord1 *= textureSize.zw;\n"
		// sample texture and apply weighting
		"return sampleClamp( texture, (vec2( texCoord0.x, texCoord0.y )) ) * scalingFactor0.x * scalingFactor0.y +\n"
			   "sampleClamp( texture, (vec2( texCoord1.x, texCoord0.y )) ) * scalingFactor1.x * scalingFactor0.y +\n"
			   "sampleClamp( texture, (vec2( texCoord0.x, texCoord1.y )) ) * scalingFactor0.x * scalingFactor1.y +\n"
			   "sampleClamp( texture, (vec2( texCoord1.x, texCoord1.y )) ) * scalingFactor1.x * scalingFactor1.y;\n"
	"}\n"
	"void main()\n"
	"{\n"
		"vec2 tc = warp(texCoords);\n"
		"if (!all(equal(clamp(tc, screenCenter - vec2(0.5,0.5), screenCenter + vec2(0.5,0.5)),tc)))\n"
			"gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"else\n"
			// have to reapply warping for the bicubic sampling
			"gl_FragColor = vec4(filter(texture, tc),1.0);\n"
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

		"if (!all(equal(clamp(tcBlue, screenCenter - vec2(0.5,0.5), screenCenter + vec2(0.5,0.5)),tcBlue)))\n"
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

static r_shaderobject_t ovr_shader_bicubic_chrm = {
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
	"uniform vec4 textureSize;\n"
	"uniform sampler2D texture;\n"
	// hack to fix artifacting with AMD cards
	// not necessary for chromatic
	"vec3 sampleClamp(sampler2D tex, vec2 uv)\n"
	"{\n"
		"return texture2D(tex,clamp(uv,vec2(0.0),vec2(1.0))).rgb;\n"
	"}\n"
	"vec3 filter(sampler2D texture, vec2 texCoord)\n"
	"{\n"
		// calculate the center of the texel
		"texCoord *= textureSize.xy;\n"
		"vec2 texelCenter   = floor( texCoord - 0.5 ) + 0.5;\n"
		"vec2 fracOffset    = texCoord - texelCenter;\n"
		"vec2 fracOffset_x2 = fracOffset * fracOffset;\n"
		"vec2 fracOffset_x3 = fracOffset * fracOffset_x2;\n"
		// calculate bspline weight function
		"vec2 weight0 = fracOffset_x2 - 0.5 * ( fracOffset_x3 + fracOffset );\n"
		"vec2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;\n"
		"vec2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );\n"
		"vec2 weight2 = 1.0 - weight0 - weight1 - weight3;\n"
		// calculate texture coordinates
		"vec2 scalingFactor0 = weight0 + weight1;\n"
		"vec2 scalingFactor1 = weight2 + weight3;\n"
		"vec2 f0 = weight1 / ( weight0 + weight1 );\n"
		"vec2 f1 = weight3 / ( weight2 + weight3 );\n"
		"vec2 texCoord0 = texelCenter - 1.0 + f0;\n"
		"vec2 texCoord1 = texelCenter + 1.0 + f1;\n"
		"texCoord0 *= textureSize.zw;\n"
		"texCoord1 *= textureSize.zw;\n"
		// sample texture and apply weighting
		"return sampleClamp( texture, (vec2( texCoord0.x, texCoord0.y )) ) * scalingFactor0.x * scalingFactor0.y +\n"
			   "sampleClamp( texture, (vec2( texCoord1.x, texCoord0.y )) ) * scalingFactor1.x * scalingFactor0.y +\n"
			   "sampleClamp( texture, (vec2( texCoord0.x, texCoord1.y )) ) * scalingFactor0.x * scalingFactor1.y +\n"
			   "sampleClamp( texture, (vec2( texCoord1.x, texCoord1.y )) ) * scalingFactor1.x * scalingFactor1.y;\n"
	"}\n"

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

		"if (!all(equal(clamp(tcBlue, screenCenter - vec2(0.5,0.5), screenCenter + vec2(0.5,0.5)),tcBlue)))\n"
		"{\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
			"return;\n"
		"}\n"

		// Now do blue texture lookup.
		"float blue = filter(texture, tcBlue).b;\n"

		// Do green lookup (no scaling).
		"vec2  tcGreen = lensCenter + scale * theta1;\n"

		"float green = filter(texture, tcGreen).g;\n"

		// Do red scale and lookup.
		"vec2  thetaRed = theta1 * (chromAbParam.x + chromAbParam.y * rSq);\n"
		"vec2  tcRed = lensCenter + scale * thetaRed;\n"

		"float red = filter(texture, tcRed).r;\n"

		"gl_FragColor = vec4(red, green, blue, 1.0);\n"
	"}\n"

};

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



void OVR_FrameStart(int changeBackBuffers)
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

	if (vr_ovr_filtermode->modified)
	{
		switch ((int) vr_ovr_filtermode->value)
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

		renderTargetSize[0] = ovrScale * vrState.scaledViewWidth * 0.5;
		renderTargetSize[1] = ovrScale  * vrState.scaledViewHeight;
		Com_Printf("OVR: Set render target size to %ux%u\n",renderTargetSize[0],renderTargetSize[1]);
		VR_OVR_SetFOV();
		vrState.pixelScale = (int) ovrScale * vrState.scaledViewWidth / (int) vrConfig.hmdWidth;
	}
}


void OVR_BindView(vr_eye_t eye)
{
	
	switch(eye)
	{
	case EYE_LEFT:
		if (renderTargetSize[0] != left.width || renderTargetSize[1] != left.height)
		{
			R_ResizeFBO(renderTargetSize[0], renderTargetSize[1], useBilinear, &left);
	//		Com_Printf("OVR: Set left render target size to %ux%u\n",left.width,left.height);
		}
		vid.height = left.height;
		vid.width = left.width;
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:
		if (renderTargetSize[0] != right.width || renderTargetSize[1] != right.height)
		{
			R_ResizeFBO(renderTargetSize[0], renderTargetSize[1], useBilinear, &right);
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

void OVR_GetViewPos(vr_eye_t eye, unsigned int pos[2])
{
		pos[0] = 0;
		pos[1] = 0;
}

void OVR_GetViewSize(vr_eye_t eye, unsigned int size[2])
{
	size[0] = renderTargetSize[0];
	size[1] = renderTargetSize[1];
}

void OVR_Present()
{
	if (vr_ovr_distortion->value)
	{

		float scale = VR_OVR_GetDistortionScale();
		//float superscale = (vr_antialias->value == VR_ANTIALIAS_FSAA ? 2.0f : 1.0f);
		float superscale = vr_ovr_supersample->value;
		r_ovr_shader_t *current_shader;
		vec4_t debugColor;
		if (vr_ovr_filtermode->value)
			current_shader = &ovr_bicubic_shaders[!!(int) vr_ovr_chromatic->value];
		else
			current_shader = &ovr_shaders[!!(int) vr_ovr_chromatic->value];

		// draw left eye

		glUseProgramObjectARB(current_shader->shader->program);

		glUniform4fvARB(current_shader->uniform.chrom_ab_param, 1, vrConfig.chrm);
		glUniform4fvARB(current_shader->uniform.hmd_warp_param, 1, vrConfig.dk);
		glUniform2fARB(current_shader->uniform.scale_in, 2.0f, 2.0f / vrConfig.aspect);
		glUniform2fARB(current_shader->uniform.scale, 0.5f / scale, 0.5f * vrConfig.aspect / scale);
		glUniform4fARB(current_shader->uniform.texture_size, renderTargetSize[0] / superscale, renderTargetSize[1] / superscale, superscale / renderTargetSize[0], superscale / renderTargetSize[1]);
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

int OVR_Enable()
{
	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

//	OVR_FrameStart(true);

	return true;
}

void OVR_Disable()
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

int OVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	VR_OVR_InitShader(&ovr_shaders[0],&ovr_shader_norm);
	VR_OVR_InitShader(&ovr_shaders[1],&ovr_shader_chrm);
	VR_OVR_InitShader(&ovr_bicubic_shaders[0],&ovr_shader_bicubic_norm);
	VR_OVR_InitShader(&ovr_bicubic_shaders[1],&ovr_shader_bicubic_chrm);
	return true;

}
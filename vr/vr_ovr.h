#ifndef __VR_OVR_H
#define __VR_OVR_H

#include "../renderer/r_local.h"
#include "vr.h"

typedef enum {
	RIFT_NONE,
	RIFT_DK1,
	RIFT_DKHD,
	NUM_RIFT
} rift_t;

typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint scale;
		GLuint scale_in;
		GLuint lens_center;
		GLuint screen_center;
		GLuint texture_size;
		GLuint hmd_warp_param;
		GLuint chrom_ab_param;
	} uniform;

} r_ovr_shader_t;

// Default Lens Warp Shader
static r_shaderobject_t ovr_shader_norm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"texCoords = gl_MultiTexCoord0;\n"
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
		"if (any(bvec2(clamp(tc, screenCenter - vec2(0.25,0.5), screenCenter + vec2(0.25,0.5))-tc)))\n"
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
		"texCoords = gl_MultiTexCoord0;\n"
	"}\n",

	// fragment shader
	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec2 textureSize;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform sampler2D texture;\n"
	"vec2 warp(vec2 uv)\n"
	"{\n"
		"vec2 theta = (vec2(uv) - lensCenter) * scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"return (lensCenter + scale * rvector);\n"
	"}\n"
	"vec4 filter(sampler2D texture, vec2 texCoord)\n"
	"{\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the center of the texel to avoid any filtering\n"
		"vec2 texelSize = 1.0 / textureSize;\n"
		"texCoord *= textureSize;\n"
		"vec2 texelCenter   = floor( texCoord - 0.5 ) + 0.5;\n"
		"vec2 fracOffset    = texCoord - texelCenter;\n"
		"vec2 fracOffset_x2 = fracOffset * fracOffset;\n"
		"vec2 fracOffset_x3 = fracOffset * fracOffset_x2;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the filter weights (B-Spline Weighting Function)\n"
		"vec2 weight0 = fracOffset_x2 - 0.5 * ( fracOffset_x3 + fracOffset );\n"
		"vec2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;\n"
		"vec2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );\n"
		"vec2 weight2 = 1.0 - weight0 - weight1 - weight3;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the texture coordinates\n"
		"vec2 scalingFactor0 = weight0 + weight1;\n"
		"vec2 scalingFactor1 = weight2 + weight3;\n"
		"vec2 f0 = weight1 / ( weight0 + weight1 );\n"
		"vec2 f1 = weight3 / ( weight2 + weight3 );\n"
		"vec2 texCoord0 = texelCenter - 1.0 + f0;\n"
		"vec2 texCoord1 = texelCenter + 1.0 + f1;\n"
		"texCoord0 *= texelSize;\n"
		"texCoord1 *= texelSize;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Sample the texture\n"
		"return texture2D( texture, warp(vec2( texCoord0.x, texCoord0.y )) ) * scalingFactor0.x * scalingFactor0.y +\n"
			   "texture2D( texture, warp(vec2( texCoord1.x, texCoord0.y )) ) * scalingFactor1.x * scalingFactor0.y +\n"
			   "texture2D( texture, warp(vec2( texCoord0.x, texCoord1.y )) ) * scalingFactor0.x * scalingFactor1.y +\n"
			   "texture2D( texture, warp(vec2( texCoord1.x, texCoord1.y )) ) * scalingFactor1.x * scalingFactor1.y;\n"
	"}\n"
	"void main()\n"
	"{\n"
		"vec2 tc = warp(texCoords);\n"
		"if (any(bvec2(clamp(tc, screenCenter - vec2(0.25,0.5), screenCenter + vec2(0.25,0.5))-tc)))\n"
			"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"else\n"
			"gl_FragColor = filter(texture, texCoords);\n"
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

static r_shaderobject_t ovr_shader_bicubic_chrm = {
	0, 0, 0,
	
	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
		"gl_Position = gl_Vertex;\n"
		"texCoords = gl_MultiTexCoord0;\n"
	"}\n",

	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform vec2 lensCenter;\n"
	"uniform vec2 scale;\n"
	"uniform vec2 screenCenter;\n"
	"uniform vec2 scaleIn;\n"
	"uniform vec2 textureSize;\n"
	"uniform vec4 hmdWarpParam;\n"
	"uniform vec4 chromAbParam;\n"
	"uniform sampler2D texture;\n"
		"vec2 warp(vec2 uv, vec2 chromAdjust)\n"
	"{\n"
		"vec2 theta = (vec2(uv) - lensCenter) * scaleIn;\n"
		"float rSq = theta.x*theta.x + theta.y*theta.y;\n"
		"vec2 rvector = theta*(hmdWarpParam.x + hmdWarpParam.y*rSq + hmdWarpParam.z*rSq*rSq + hmdWarpParam.w*rSq*rSq*rSq);\n"
		"rvector *= (chromAdjust.x + chromAdjust.y * rSq);\n"
		"return (lensCenter + scale * rvector);\n"
	"}\n"
	"vec4 filter(sampler2D texture, vec2 texCoord, vec2 warpAdjust)\n"
	"{\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the center of the texel to avoid any filtering\n"
		"vec2 texelSize = 1.0 / textureSize;\n"
		"texCoord *= textureSize;\n"
		"vec2 texelCenter   = floor( texCoord - 0.5 ) + 0.5;\n"
		"vec2 fracOffset    = texCoord - texelCenter;\n"
		"vec2 fracOffset_x2 = fracOffset * fracOffset;\n"
		"vec2 fracOffset_x3 = fracOffset * fracOffset_x2;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the filter weights (B-Spline Weighting Function)\n"
		"vec2 weight0 = fracOffset_x2 - 0.5 * ( fracOffset_x3 + fracOffset );\n"
		"vec2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;\n"
		"vec2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );\n"
		"vec2 weight2 = 1.0 - weight0 - weight1 - weight3;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Calculate the texture coordinates\n"
		"vec2 scalingFactor0 = weight0 + weight1;\n"
		"vec2 scalingFactor1 = weight2 + weight3;\n"
		"vec2 f0 = weight1 / ( weight0 + weight1 );\n"
		"vec2 f1 = weight3 / ( weight2 + weight3 );\n"
		"vec2 texCoord0 = texelCenter - 1.0 + f0;\n"
		"vec2 texCoord1 = texelCenter + 1.0 + f1;\n"
		"texCoord0 *= texelSize;\n"
		"texCoord1 *= texelSize;\n"
		"//--------------------------------------------------------------------------------------\n"
		"// Sample the texture\n"
		"return texture2D( texture, warp(vec2( texCoord0.x, texCoord0.y ), warpAdjust) ) * scalingFactor0.x * scalingFactor0.y +\n"
			   "texture2D( texture, warp(vec2( texCoord1.x, texCoord0.y ), warpAdjust) ) * scalingFactor1.x * scalingFactor0.y +\n"
			   "texture2D( texture, warp(vec2( texCoord0.x, texCoord1.y ), warpAdjust) ) * scalingFactor0.x * scalingFactor1.y +\n"
			   "texture2D( texture, warp(vec2( texCoord1.x, texCoord1.y ), warpAdjust) ) * scalingFactor1.x * scalingFactor1.y;\n"
	"}\n"
	// Scales input texture coordinates for distortion.
	// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
	// larger due to aspect ratio.
	"void main()\n"
	"{\n"
	"vec2 tcBlue = warp(texCoords,vec2(chromAbParam.z,chromAbParam.w));\n"

		"if (!all(equal(clamp(tcBlue, screenCenter - vec2(0.25,0.5), screenCenter + vec2(0.25,0.5)),tcBlue)))\n"
		"{\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
			"return;\n"
		"}\n"

		// Now do blue texture lookup.
		"float blue = filter(texture, texCoords,vec2(chromAbParam.z,chromAbParam.w)).b;\n"
		"float green = filter(texture, texCoords,vec2(1.0,0.0)).g;\n"
		"float red = filter(texture, texCoords,vec2(chromAbParam.x,chromAbParam.y)).r;\n"

		"gl_FragColor = vec4(red, green, blue, 1.0);\n"
	"}\n"
};
extern cvar_t *vr_ovr_driftcorrection;
extern cvar_t *vr_ovr_scale;
extern cvar_t *vr_ovr_chromatic;
extern cvar_t *vr_ovr_prediction;
extern cvar_t *vr_ovr_distortion;
extern cvar_t *vr_ovr_lensdistance;
extern cvar_t *vr_ovr_autoscale;
extern cvar_t *vr_ovr_autolensdistance;
extern cvar_t *vr_ovr_bicubic;

void VR_OVR_SetFOV();
void VR_OVR_CalcRenderParam();
void VR_OVR_Frame();
int VR_OVR_Enable();
void VR_OVR_Disable();
int VR_OVR_Init();
void VR_OVR_Shutdown();
int VR_OVR_isDeviceAvailable();
int VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
float VR_OVR_GetDistortionScale();
int VR_OVR_RenderLatencyTest(vec4_t color);
void VR_OVR_InitShader(r_ovr_shader_t *shader, r_shaderobject_t *object);

extern hmd_interface_t hmd_rift;

#endif
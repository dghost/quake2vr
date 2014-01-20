#ifndef __R_VR_OVR_H
#define __R_VR_OVR_H

#include "r_vr.h"
#include "r_local.h"

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
		"if (any(lessThan(uv,vec2(0.0)))||any(greaterThan(uv,vec2(1.0))))\n"
			"return vec3(0.0);\n"
		"else\n"
		"return texture2D(tex,uv).rgb;\n"
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
		"if (any(lessThan(uv,vec2(0.0)))||any(greaterThan(uv,vec2(1.0))))\n"
			"return vec3(0.0);\n"
		"else\n"
			"return texture2D(tex,uv).rgb;\n"
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

void R_VR_OVR_FrameStart(int changeBackBuffers);
void R_VR_OVR_BindView(vr_eye_t eye);
void R_VR_OVR_Present();
int R_VR_OVR_Enable();
void R_VR_OVR_Disable();
int R_VR_OVR_Init();

extern hmd_render_t vr_render_ovr;

#endif //__R_VR_OVR_H
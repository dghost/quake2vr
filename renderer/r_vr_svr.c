#include "../renderer/r_local.h"
#include "r_vr.h"

#ifndef NO_STEAM

#include "r_vr_svr.h"
#include "../vr/vr_svr.h"
#include "../vr/vr_steamvr.h"

//static fbo_t world;
static fbo_t left, right;

static vr_rect_t renderTargetRect, origTargetRect;
static vr_rect_t leftRenderRect, rightRenderRect;

static GLuint leftDistortion, rightDistortion;

static qboolean chromatic;

extern svr_settings_t svr_settings;

// Default Lens Warp Shader
static r_shaderobject_t svr_shader_norm = {
	0, 
	
	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
		"gl_Position = gl_ProjectionMatrix * gl_Vertex;\n"
		"texCoords = gl_MultiTexCoord0.xy;\n"
	"}\n",
	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform sampler2D tex;\n"
	"uniform sampler2D dist;\n"
	"void main()\n"
	"{\n"
		"vec2 tc = texture2D(dist,texCoords).xy;\n"
		"if (!all(equal(clamp(tc, vec2(0.0,0.0), vec2(1.0,1.0)),tc)))\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
		"else\n"
			"gl_FragColor = texture2D(tex, tc);\n"
	"}\n"

};


// Lens Warp Shader with Chromatic Aberration 
static r_shaderobject_t svr_shader_chrm = {
	0, 
	
	// vertex shader (identity)
	"varying vec2 texCoords;\n"
	"void main(void) {\n"
		"gl_Position = gl_ProjectionMatrix * gl_Vertex;\n"
		"texCoords = gl_MultiTexCoord0.xy;\n"
	"}\n",
	// fragment shader
	"varying vec2 texCoords;\n"
	"uniform sampler2D tex;\n"
	"uniform sampler2D dist;\n"
	"void main()\n"
	"{\n"
		"vec4 vRead = texture2D(dist,texCoords);\n"
		"vec2 tcGreen = vec2(vRead.r + vRead.b, vRead.g + vRead.a) / 2.0;\n"
		"if (!all(equal(clamp(tcGreen, vec2(0.0,0.0), vec2(1.0,1.0)),tcGreen)))\n"
		"{\n"
			"gl_FragColor = vec4(0.0,0.0,0.0,1.0);\n"
		"}\n"
		"else\n"
		"{\n"
			"vec4 final;"
			"final.r = texture2D(tex, vRead.rg).r;"
			"final.ga = texture2D(tex, tcGreen).ga;"
			"final.b = texture2D(tex, vRead.ba).b;"
			"gl_FragColor = final;\n"
		"}\n"
	"}\n"
};

typedef struct {
	r_shaderobject_t *shader;
	struct {
		GLuint texture;
		GLuint distortion;
	} uniform;

} r_svr_shader_t;

r_svr_shader_t svr_shaders[2];

// util function
void SVR_InitShader(r_svr_shader_t *shader, r_shaderobject_t *object)
{
	if (!object->program)
		R_CompileShaderProgram(object);

	shader->shader = object;
	glUseProgram(shader->shader->program);
	shader->uniform.texture = glGetUniformLocation(shader->shader->program, "tex");
	shader->uniform.distortion = glGetUniformLocation(shader->shader->program, "dist");
	glUseProgram(0);
}

void SVR_BuildDistortionTextures()
{
    GLfloat *rnDistortionMapData[ 2 ]; // the memory to put the map into
	int width = origTargetRect.width; // width of the distortion map
	int height= origTargetRect.height; // height of the distortion map
	int i;
	int bitsPerPixel = (!chromatic && glConfig.arb_texture_rg? 2 : 4);
	float scale = (1.0 / pow(2,vr_svr_distortion->value));
	
	width = (int) ((float) width * scale);
	height = (int) ((float) height * scale);
	
	Com_Printf("VR_SVR: Generating %dx%d distortion textures...\n",width,height);
	for ( i = 0; i < 2; i++ )
	{
		eye_t eye = (eye_t) i;
		GLfloat *pData;
	
		int x, y;	
		rnDistortionMapData[i] = (GLfloat *) malloc(sizeof(GLfloat) * width * height * bitsPerPixel);
		memset(rnDistortionMapData[i],0,sizeof(GLfloat) * width * height * bitsPerPixel);
		pData = rnDistortionMapData[ i ];

		for( y = 0; y < height; y++ )
		{
			for( x = 0; x < width; x++ )
			{
				int offset = bitsPerPixel * ( x + y * width );
				distcoords_t coords;

				float u = ( (float)x + 0.5f) / (float)width;
				float v = ( (float)y + 0.5f) / (float)height;
				assert( offset < width * height * bitsPerPixel );


				coords = SteamVR_GetDistortionCoords( eye, u, v );

				// Put red and green UVs into the texture. We'll estimate the green UV in the shader from
				// these two.

				if (chromatic)
				{
					pData[offset + 0] = (GLfloat)( coords.rfRed[0] );
					pData[offset + 1] = (GLfloat)( coords.rfRed[1] );
					pData[offset + 2] = (GLfloat)( coords.rfBlue[0] );
					pData[offset + 3] = (GLfloat)( coords.rfBlue[1] );
				} else {
					pData[offset + 0] = (GLfloat)( coords.rfGreen[0] );
					pData[offset + 1] = (GLfloat)( coords.rfGreen[1] );
				}
			}
		}
	}

	GL_MBind(0,leftDistortion);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	if (bitsPerPixel == 2)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, rnDistortionMapData[0]);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, rnDistortionMapData[0]);
	
	GL_MBind(0,rightDistortion);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	if (bitsPerPixel == 2)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, rnDistortionMapData[1]);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, rnDistortionMapData[1]);
	
	GL_MBind(0,0);

	for (i = 0; i < 2; i++)
	{
		free(rnDistortionMapData[i]);
	}
}


void SVR_FrameStart(Sint32 changeBackBuffers)
{


	if (vr_svr_distortion->modified)
	{
		if (vr_svr_distortion->value > 3)
			Cvar_SetInteger("vr_svr_distortion", 3);
		if (vr_svr_distortion->value <1)
			Cvar_SetInteger("vr_svr_distortion", 1);
		changeBackBuffers = 1;
		vr_svr_distortion->modified = false;
	}

	if (chromatic != (qboolean) !!vr_chromatic->value)
	{
		chromatic = (qboolean) !!vr_chromatic->value;
		changeBackBuffers = 1;
	}

	if (changeBackBuffers)
	{	
		origTargetRect.x = 0;
		origTargetRect.y = 0;
		origTargetRect.width = (Uint32) Cvar_VariableValue("vid_width") * svr_settings.scaleX;
		origTargetRect.height = (Uint32) Cvar_VariableValue("vid_height") * svr_settings.scaleY;

		renderTargetRect = origTargetRect;
		if (r_antialias->value == ANTIALIAS_4X_FSAA)
		{
			renderTargetRect.width *= 2;
			renderTargetRect.height *= 2;
		}
		Com_Printf("VR_SVR: Set render target size to %ux%u\n",renderTargetRect.width,renderTargetRect.height);
		SVR_BuildDistortionTextures();		
	}
}


void SVR_BindView(vr_eye_t eye)
{
	switch(eye)
	{
	case EYE_LEFT:

		if (renderTargetRect.width != left.width || renderTargetRect.height != left.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, &left);
		}
		vid.height = left.height;
		vid.width = left.width;
		R_BindFBO(&left);
		break;
	case EYE_RIGHT:
		if (renderTargetRect.width != right.width || renderTargetRect.height != right.height)
		{
			R_ResizeFBO(renderTargetRect.width, renderTargetRect.height, true, &right);
		}
		vid.height = right.height;
		vid.width = right.width;
		R_BindFBO(&right);
		break;
	default:
		return;
	}
}

void SVR_GetState(vr_param_t *state)
{
	vr_param_t svrState;
	svrState.aspect = svr_settings.aspect;
	svrState.projOffset = svr_settings.projOffset;
	svrState.viewFovX = svr_settings.viewFovX;
	svrState.viewFovY = svr_settings.viewFovY;
	svrState.ipd = svr_settings.ipd;
	svrState.pixelScale = 3.0;
	*state = svrState;
}

void SVR_Present()
{
	GL_SetIdentityOrtho(GL_PROJECTION, 0, svr_settings.width, svr_settings.height, 0, -1, 1);

	if (!vr_svr_debug->value)
	{

		r_svr_shader_t *current_shader = &svr_shaders[chromatic];
 
		
		// draw left eye
		glUseProgram(current_shader->shader->program);
		glUniform1i(current_shader->uniform.texture,0);
		glUniform1i(current_shader->uniform.distortion,1);	

		GL_EnableMultitexture(true);
		GL_MBind(0,left.texture);
		GL_MBind(1,leftDistortion);
		
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(leftRenderRect.x, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(leftRenderRect.x, leftRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y);
		glEnd();

		// draw right eye

		GL_MBind(0,right.texture);
		GL_MBind(1,rightDistortion);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(rightRenderRect.x, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(rightRenderRect.x, rightRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y);
		glEnd();
		glUseProgram(0);
		GL_MBind(1,0);
		GL_MBind(0,0);

	} else {
		GL_MBind(0,left.texture);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(leftRenderRect.x, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(leftRenderRect.x, leftRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y + leftRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(leftRenderRect.x + leftRenderRect.width, leftRenderRect.y);
		glEnd();

		GL_MBind(0,rightDistortion);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0); glVertex2f(rightRenderRect.x, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(0, 1); glVertex2f(rightRenderRect.x, rightRenderRect.y);
		glTexCoord2f(1, 0); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y + rightRenderRect.height);
		glTexCoord2f(1, 1); glVertex2f(rightRenderRect.x + rightRenderRect.width, rightRenderRect.y);
		glEnd();
		GL_MBind(0,0);
	}
}
void SVR_GetViewRect(vr_eye_t eye, vr_rect_t *rect)
{
	*rect = renderTargetRect;
}


Sint32 SVR_Init()
{
	R_InitFBO(&left);
	R_InitFBO(&right);
	return true;
}

void SVR_Disable()
{
	R_DelShaderProgram(&svr_shader_norm);
	R_DelShaderProgram(&svr_shader_chrm);

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

	if (leftDistortion)
		glDeleteTextures(1,&leftDistortion);
	if (rightDistortion)
		glDeleteTextures(1,&rightDistortion);

	leftDistortion = 0;
	rightDistortion = 0;
}

void SVR_CalculateRenderParams()
{

}


Sint32 SVR_Enable()
{
	char string[128];

	if (!glConfig.arb_texture_float)
		return 0;

	if (left.valid)
		R_DelFBO(&left);
	if (right.valid)
		R_DelFBO(&right);

	if (!leftDistortion)
		glGenTextures(1,&leftDistortion);
	if (!rightDistortion)
		glGenTextures(1,&rightDistortion);

	SteamVR_GetSettings(&svr_settings);
	
	SteamVR_GetEyeViewport(SVR_Left,&leftRenderRect.x,&leftRenderRect.y,&leftRenderRect.width,&leftRenderRect.height);	
	SteamVR_GetEyeViewport(SVR_Right,&rightRenderRect.x,&rightRenderRect.y,&rightRenderRect.width,&rightRenderRect.height);

	SVR_InitShader(&svr_shaders[0],&svr_shader_norm);
	SVR_InitShader(&svr_shaders[1],&svr_shader_chrm);
	strncpy(string, va("SteamVR-%s", svr_settings.deviceName), sizeof(string));
	Cvar_ForceSet("vr_hmdstring",string);
	return true;
}



hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	SVR_Init,
	SVR_Enable,
	SVR_Disable,
	SVR_BindView,
	SVR_GetViewRect,
	SVR_FrameStart,
	SVR_GetState,
	SVR_Present
};

#else

hmd_render_t vr_render_svr = 
{
	HMD_STEAM,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

#endif //NO_STEAM
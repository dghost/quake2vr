/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// r_glstate.c - OpenGL state manager from Q2E

#include "include/r_local.h"

fbo_t screenFBO;
fbo_t *lastFrame;

/*
=================
GL_Enable
=================
*/
void GL_Enable (GLenum cap)
{
	switch (cap)
	{
	case GL_CULL_FACE:
		if (glState.cullFace)
			return;
		glState.cullFace = true;
		break;
	case GL_POLYGON_OFFSET_FILL:
		if (glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = true;
		break;
	case GL_ALPHA_TEST:
		if (glState.alphaTest)
			return;
		glState.alphaTest = true;
		break;
	case GL_BLEND:
		if (glState.blend)
			return;
		glState.blend = true;
		break;
	case GL_DEPTH_TEST:
		if (glState.depthTest)
			return;
		glState.depthTest = true;
		break;
	case GL_STENCIL_TEST:
		if (glState.stencilTest)
			return;
		glState.stencilTest = true;
		break;
	case GL_SCISSOR_TEST:
		if (glState.scissorTest)
			return;
		glState.scissorTest = true;
        break;
    case GL_FOG:
        if (glState.fog)
            return;
        glState.fog = true;
        break;
	}
	glEnable(cap);
}


/*
=================
GL_Disable
=================
*/
void GL_Disable (GLenum cap)
{
	switch (cap)
	{
	case GL_CULL_FACE:
		if (!glState.cullFace)
			return;
		glState.cullFace = false;
		break;
	case GL_POLYGON_OFFSET_FILL:
		if (!glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = false;
		break;
	case GL_ALPHA_TEST:
		if (!glState.alphaTest)
			return;
		glState.alphaTest = false;
		break;
	case GL_BLEND:
		if (!glState.blend)
			return;
		glState.blend = false;
		break;
	case GL_DEPTH_TEST:
		if (!glState.depthTest)
			return;
		glState.depthTest = false;
		break;
	case GL_STENCIL_TEST:
		if (!glState.stencilTest)
			return;
		glState.stencilTest = false;
		break;
	case GL_SCISSOR_TEST:
		if (!glState.scissorTest)
			return;
		glState.scissorTest = false;
        break;
    case GL_FOG:
        if (!glState.fog)
            return;
        glState.fog = false;
        break;
	}
	glDisable(cap);
}


void GL_ClearStencil (GLint value) {
    if (value != glState.stencilClear) {
        glState.stencilClear = value;
        glClearStencil(value);
    }
}


void GL_StencilFunc (GLenum func, GLint ref, GLuint mask) {
    if (func != glState.stencilFunc || ref != glState.stencilFuncRef || mask != glState.stencilFuncMask) {
        glState.stencilFunc = func;
        glState.stencilFuncRef = ref;
        glState.stencilFuncMask = mask;
        glStencilFunc(func, ref, mask);
    }
}

qboolean GL_HasStencil (void)
{
	return (glConfig.have_stencil && r_stencil->value);
}


/*
=================
GL_Envmap
setting up envmap
=================
*/
#define GLSTATE_DISABLE_TEXGEN		if (glState.texgen) { glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); glDisable(GL_TEXTURE_GEN_R); glState.texgen=false; }
#define GLSTATE_ENABLE_TEXGEN		if (!glState.texgen) { glEnable(GL_TEXTURE_GEN_S); glEnable(GL_TEXTURE_GEN_T); glEnable(GL_TEXTURE_GEN_R); glState.texgen=true; }
void GL_Envmap (qboolean enable)
{

	if (enable)
	{
		glTexGenf(GL_S, GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
		glTexGenf(GL_T, GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
		GLSTATE_ENABLE_TEXGEN
	}
	else
	{
		GLSTATE_DISABLE_TEXGEN
	}
}


/*
=================
GL_ShadeModel
=================
*/
void GL_ShadeModel (GLenum mode)
{
	if (glState.shadeModelMode == mode)
		return;
	glState.shadeModelMode = mode;
	glShadeModel(mode);
}


/*
=================
GL_TexEnv
=================
*/
void GL_TexEnv (GLenum mode)
{
	static int32_t lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[glState.currenttmu] )
	{
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[glState.currenttmu] = mode;
	}
}


/*
=================
GL_CullFace
=================
*/
void GL_CullFace (GLenum mode)
{
	if (glState.cullMode == mode)
		return;
	glState.cullMode = mode;
	glCullFace(mode);
}


/*
=================
GL_PolygonOffset
=================
*/
void GL_PolygonOffset (GLfloat factor, GLfloat units)
{
	if (glState.offsetFactor == factor && glState.offsetUnits == units)
		return;
	glState.offsetFactor = factor;
	glState.offsetUnits = units;
	glPolygonOffset(factor, units);
}


/*
=================
GL_AlphaFunc
=================
*/
void GL_AlphaFunc (GLenum func, GLclampf ref)
{
	if (glState.alphaFunc == func && glState.alphaRef == ref)
		return;
	glState.alphaFunc = func;
	glState.alphaRef = ref;
	glAlphaFunc(func, ref);
}


/*
=================
GL_BlendFunc
=================
*/
void GL_BlendFunc (GLenum src, GLenum dst)
{
	if (glState.blendSrc == src && glState.blendDst == dst)
		return;
	glState.blendSrc = src;
	glState.blendDst = dst;
	glBlendFunc(src, dst);
}


/*
=================
GL_DepthFunc
=================
*/
void GL_DepthFunc (GLenum func)
{
	if (glState.depthFunc == func)
		return;
	glState.depthFunc = func;
	glDepthFunc(func);
}


/*
=================
GL_DepthMask
=================
*/
void GL_DepthMask (GLboolean mask)
{
	if (glState.depthMask == mask)
		return;
	glState.depthMask = mask;
	glDepthMask(mask);
}

/*
=================
GL_DepthRange
=================
*/
void GL_DepthRange (GLfloat rMin, GLfloat rMax)
{
	if (glState.depthMin == rMin && glState.depthMax == rMax)
		return;
	glState.depthMin = rMin;
	glState.depthMax = rMax;
	glDepthRange (rMin, rMax);
}


/*
=============
GL_LockArrays
=============
*/
void GL_LockArrays (int32_t numVerts)
{
	if (!glConfig.extCompiledVertArray)
		return;
	if (glState.arraysLocked)
		return;

	glLockArraysEXT (0, numVerts);
	glState.arraysLocked = true;
}


/*
=============
GL_UnlockArrays
=============
*/
void GL_UnlockArrays (void)
{
	if (!glConfig.extCompiledVertArray)
		return;
	if (!glState.arraysLocked)
		return;

	glUnlockArraysEXT ();
	glState.arraysLocked = false;
}

/*
=================
GL_EnableTexture
=================
*/
void GL_EnableTexture (uint32_t tmu)
{
	if (tmu >= MAX_TEXTURE_UNITS || tmu >= glConfig.max_texunits)
		return;

	GL_SelectTexture(tmu);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(texCoordArray[tmu][0]), texCoordArray[tmu][0]);
	glState.activetmu[tmu] = true;
}

/*
=================
GL_DisableTexture
=================
*/
void GL_DisableTexture (uint32_t tmu)
{
	if (tmu >= MAX_TEXTURE_UNITS || tmu >= glConfig.max_texunits)
		return;

	GL_SelectTexture(tmu);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glState.activetmu[tmu] = false;
}

/*
=================
GL_EnableMultitexture
Only used for world drawing
=================
*/
void GL_EnableMultitexture (qboolean enable)
{
	if (enable)
	{
		GL_EnableTexture(1);
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_DisableTexture(1);
		GL_TexEnv(GL_REPLACE);
	}
	GL_SelectTexture(0);
	GL_TexEnv(GL_REPLACE);
}

/*
=================
GL_SelectTexture
=================
*/
void GL_SelectTexture (uint32_t tmu)
{
	if (tmu >= MAX_TEXTURE_UNITS || tmu >= glConfig.max_texunits)
		return;

	if (tmu == glState.currenttmu)
		return;

	glState.currenttmu = tmu;

	glActiveTexture(GL_TEXTURE0+tmu);
	glClientActiveTexture(GL_TEXTURE0+tmu);
}

/*
=================
GL_Bind
=================
*/
void GL_Bind (int32_t texnum)
{
	extern	image_t	*draw_chars;

	if (r_nobind->value && draw_chars)		// performance evaluation option
		texnum = draw_chars->texnum;
	if (glState.currenttextures[glState.currenttmu] == texnum)
		return;

	glState.currenttextures[glState.currenttmu] = texnum;
	glBindTexture (GL_TEXTURE_2D, texnum);
}

/*
=================
GL_MBind
=================
*/
void GL_MBind (uint32_t tmu, int32_t texnum)
{
	if (tmu >= MAX_TEXTURE_UNITS || tmu >= glConfig.max_texunits)
		return;

	GL_SelectTexture(tmu);

	if (glState.currenttextures[tmu] == texnum)
		return;

	GL_Bind(texnum);
}

/*
=================
GL_BindFBO
=================
*/

void GL_BindFBO (fbo_t *fbo)
{
	if (!glConfig.ext_framebuffer_object)
		return;

	if (!fbo || glState.currentFBO == fbo)
		return;

	glState.currentFBO = fbo;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo->framebuffer);
}



void GL_MatrixMode(GLenum matrixMode)
{
	if (glState.matrixMode == matrixMode)
		return;

	glMatrixMode(matrixMode);
	glState.matrixMode = matrixMode;
}

void GL_SetIdentity(GLenum matrixMode)
{
	if (glState.matrixMode == matrixMode)
	{
		glLoadIdentity();
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixLoadIdentityEXT(matrixMode);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glLoadIdentity();
		GL_MatrixMode(currentMode);
	}
}

void GL_LoadIdentity(GLenum matrixMode)
{
	GL_MatrixMode(matrixMode);
	glLoadIdentity();
}

void GL_SetIdentityOrtho(GLenum matrixMode, float l, float r, float b, float t, float n, float f)
{
	if (glState.matrixMode == matrixMode)
	{
		glLoadIdentity();
		glOrtho(l, r, b, t, n ,f);
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixLoadIdentityEXT(matrixMode);
		glMatrixOrthoEXT(matrixMode, l, r, b, t, n ,f);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glLoadIdentity();
		glOrtho(l, r, b, t, n ,f);
		GL_MatrixMode(currentMode);
	}
}

void GL_LoadMatrix(GLenum matrixMode, const float m[4][4])
{
	if (glState.matrixMode == matrixMode)
	{
		glLoadMatrixf((GLfloat*) m);
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixLoadfEXT(matrixMode, (GLfloat*) m);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glLoadMatrixf((GLfloat*) m);
		GL_MatrixMode(currentMode);
	}
}

void GL_MultiplyMatrix(GLenum matrixMode, const float m[4][4])
{
	if (glState.matrixMode == matrixMode)
	{
		glMultMatrixf((GLfloat*) m);
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixMultfEXT(matrixMode, (GLfloat*) m);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glMultMatrixf((GLfloat*) m);
		GL_MatrixMode(currentMode);
	}
}

void GL_PushMatrix(GLenum matrixMode)
{
	if (glState.matrixMode == matrixMode)
	{
		glPushMatrix();
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixPushEXT(matrixMode);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glPushMatrix();
		GL_MatrixMode(currentMode);
	}
}

void GL_PopMatrix(GLenum matrixMode)
{
	if (glState.matrixMode == matrixMode)
	{
		glPopMatrix();
	} else if (glConfig.ext_direct_state_access && r_directstate->value)
	{
		glMatrixPopEXT(matrixMode);
	} else {
		GLenum currentMode = glState.matrixMode;
		GL_MatrixMode(matrixMode);
		glPopMatrix();
		GL_MatrixMode(currentMode);
	}
}

void GL_ClearColor(float r, float g, float b, float a)
{
	if (glState.clearColor[0] != r || glState.clearColor[1] != g ||  
		glState.clearColor[2] != b || glState.clearColor[3] != a )
	{
		Vector4Set(glState.clearColor,r,g,b,a);
		glClearColor(r,g,b,a);
	}

}
void GL_SetDefaultClearColor(void)
{
	if (developer->value)
		GL_ClearColor (1,0, 0.5, 0.5);
	else
		GL_ClearColor (0.0,0.0,0.0,0.0);
}

/*
=================
GL_SetDefaultState
=================
*/
void GL_SetDefaultState (void)
{
	int32_t		i;
	vec_t rot[4][4], temp[4][4];

	// Reset the state manager
	glState.texgen = false;
	glState.cullFace = false;
	glState.polygonOffsetFill = false;
	glState.alphaTest = false;
	glState.blend = false;
	glState.stencilTest = false;
	glState.depthTest = false;
	glState.scissorTest = false;
	glState.arraysLocked = false;

	glState.cullMode = GL_FRONT;
	glState.shadeModelMode = GL_FLAT;
	glState.depthMin = gldepthmin;
	glState.depthMax = gldepthmax;
	glState.offsetFactor = -1;
	glState.offsetUnits = -2;
	glState.alphaFunc = GL_GREATER;
	glState.alphaRef = 0.666;
	glState.blendSrc = GL_SRC_ALPHA;
	glState.blendDst = GL_ONE_MINUS_SRC_ALPHA;
	glState.depthFunc = GL_LEQUAL;
	glState.depthMask = GL_TRUE;
	glState.matrixMode = GL_MODELVIEW;

    glState.stencilClear = 0; // OpenGL says 0 is default
    glState.stencilFunc = GL_ALWAYS;
    glState.stencilFuncRef = 0;
    glState.stencilFuncMask = ~0; // default should be filled with all 1's

    
	RotationMatrix(-90, 1, 0, 0, rot);
	RotationMatrix( 90, 0, 0, 1, temp);
	MatrixMultiply(temp, rot, glState.axisRotation);



	// Set default state
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FOG);
    
	glCullFace(glState.cullMode);
	glShadeModel(glState.shadeModelMode);
	glPolygonOffset(glState.offsetFactor, glState.offsetUnits);
	glAlphaFunc(glState.alphaFunc, glState.alphaRef);
	glBlendFunc(glState.blendSrc, glState.blendDst);
	glDepthFunc(glState.depthFunc);
	glDepthMask(glState.depthMask);

	glMatrixMode(glState.matrixMode);

	// force it to change the clear color
	Vector4Set(glState.clearColor,-1,-1,-1,-1);
	GL_SetDefaultClearColor();
	glClearDepth(1.0);
    
    glClearStencil(glState.stencilClear);
    glStencilFunc(glState.stencilFunc, glState.stencilFuncRef, glState.stencilFuncMask);

	glColor4f (1,1,1,1);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	GL_TextureMode( r_texturemode->string );

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS,r_lodbias->value);

	// Vertex arrays
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
	glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);
	glColorPointer (4, GL_FLOAT, sizeof(colorArray[0]), colorArray[0]);
	// end vertex arrays

	glState.activetmu[0] = true;
	for (i=1; i<MAX_TEXTURE_UNITS; i++)
		glState.activetmu[i] = false;

	GL_TexEnv (GL_REPLACE);
	
	GL_UpdateSwapInterval();

	memset(&screenFBO,0,sizeof(fbo_t));
	screenFBO.width = glConfig.window_width;
	screenFBO.height = glConfig.window_height;
	R_BindFBO(&screenFBO);
}

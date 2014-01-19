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
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#define GLEW_STATIC

#ifdef Q2VR_REGAL
#define REGAL_NO_HTTP
#include "include/GL/RegalGLEW.h"
//#include <GL/gl.h>

#ifdef __linux__
//#include <GL/fxmesa.h>
#include "include/GL/RegalGLXEW.h"
//#include <GL/glx.h>
#endif

#ifdef _WIN32
#include "include/GL/RegalWGLEW.h"
#endif

#else

#include "include/GL/glew.h"
//#include <GL/gl.h>

#ifdef __linux__
//#include <GL/fxmesa.h>
#include "include/GL/glxew.h"
//#include <GL/glx.h>
#endif

#ifdef _WIN32
#include "include/GL/wglew.h"
#endif

#endif // Q2VR_REGAL

#endif

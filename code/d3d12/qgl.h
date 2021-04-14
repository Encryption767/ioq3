/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#if defined( _WIN32 )
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#pragma warning (disable: 4514)
#pragma warning (disable: 4032)
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#define NOMINMAX
#include <windows.h>
#include <gl/gl.h>
#endif

/*
** multitexture extension definitions
*/
#define GL_MAX_ACTIVE_TEXTURES_ARB          0x84E2

#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1

/*
** extension constants
*/

// S3TC compression constants
#define GL_RGB4_S3TC						0x83A1

// extensions will be function pointers on all platforms
extern	void ( WINAPIV * qglActiveTextureARB )( GLenum texture );
extern	void ( WINAPIV * qglClientActiveTextureARB )( GLenum texture );

extern	void ( WINAPIV * qglLockArraysEXT) (GLint, GLint);
extern	void ( WINAPIV * qglUnlockArraysEXT) (void);

//===========================================================================
extern  void ( WINAPIV * qglAlphaFunc )(GLenum func, GLclampf ref);
extern  void ( WINAPIV * qglBegin )(GLenum mode);
extern  void ( WINAPIV * qglBindTexture )(GLenum target, GLuint texture);
extern  void ( WINAPIV * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern  void ( WINAPIV * qglClear )(GLbitfield mask);
extern  void ( WINAPIV * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern  void ( WINAPIV * qglClipPlane )(GLenum plane, const GLdouble *equation);
extern  void ( WINAPIV * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
extern  void ( WINAPIV * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern  void ( WINAPIV * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( WINAPIV * qglCullFace )(GLenum mode);
extern  void ( WINAPIV * qglDeleteTextures )(GLsizei n, const GLuint *textures);
extern  void ( WINAPIV * qglDepthFunc )(GLenum func);
extern  void ( WINAPIV * qglDepthMask )(GLboolean flag);
extern  void ( WINAPIV * qglDepthRange )(GLclampd zNear, GLclampd zFar);
extern  void ( WINAPIV * qglDisable )(GLenum cap);
extern  void ( WINAPIV * qglDisableClientState )(GLenum array);
extern  void ( WINAPIV * qglDrawBuffer )(GLenum mode);
extern  void ( WINAPIV * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern  void ( WINAPIV * qglEnable )(GLenum cap);
extern  void ( WINAPIV * qglEnableClientState )(GLenum array);
extern  void ( WINAPIV * qglEnd )(void);
extern  void ( WINAPIV * qglFinish )(void);
extern  GLenum (WINAPIV * qglGetError )(void);
extern  void ( WINAPIV * qglGetIntegerv )(GLenum pname, GLint *params);
extern  const GLubyte * ( WINAPIV * qglGetString )(GLenum name);
extern  void ( WINAPIV * qglLineWidth )(GLfloat width);
extern  void ( WINAPIV * qglLoadIdentity )(void);
extern  void ( WINAPIV * qglLoadMatrixf )(const GLfloat *m);
extern  void ( WINAPIV * qglMatrixMode )(GLenum mode);
extern  void ( WINAPIV * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  void ( WINAPIV * qglPolygonMode )(GLenum face, GLenum mode);
extern  void ( WINAPIV * qglPolygonOffset )(GLfloat factor, GLfloat units);
extern  void ( WINAPIV * qglPopMatrix )(void);
extern  void ( WINAPIV * qglPushMatrix )(void);
extern  void ( WINAPIV * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern  void ( WINAPIV * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( WINAPIV * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern  void ( WINAPIV * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern  void ( WINAPIV * qglTexCoord2f )(GLfloat s, GLfloat t);
extern  void ( WINAPIV * qglTexCoord2fv )(const GLfloat *v);
extern  void ( WINAPIV * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( WINAPIV * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
extern  void ( WINAPIV * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( WINAPIV * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
extern  void ( WINAPIV * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
extern  void ( WINAPIV * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( WINAPIV * qglVertex2f )(GLfloat x, GLfloat y);
extern  void ( WINAPIV * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( WINAPIV * qglVertex3fv )(const GLfloat *v);
extern  void ( WINAPIV * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( WINAPIV * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

#if defined( _WIN32 )
extern HGLRC ( WINAPI * qwglCreateContext)(HDC);
extern BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
extern PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern BOOL  ( WINAPI * qwglSwapIntervalEXT)( int interval );
#endif

#endif

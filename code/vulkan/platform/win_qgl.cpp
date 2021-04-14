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
** QGL_WIN.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake3 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/
#include "../tr_local.h"

extern FILE *log_fp;

static HINSTANCE hinstOpenGL; // HINSTANCE for the OpenGL library

void QGL_EnableLogging(qboolean enable);

HGLRC(WINAPI *qwglCreateContext)( HDC );
BOOL(WINAPI *qwglDeleteContext)( HGLRC );
PROC(WINAPI *qwglGetProcAddress)( LPCSTR );
BOOL(WINAPI *qwglMakeCurrent)( HDC, HGLRC );
int   ( WINAPI *qwglSwapIntervalEXT )( int interval );

void ( WINAPIV *qglAlphaFunc )( GLenum func, GLclampf ref );
void ( WINAPIV *qglBegin )( GLenum mode );
void ( WINAPIV *qglBindTexture )( GLenum target, GLuint texture );
void ( WINAPIV *qglBlendFunc )( GLenum sfactor, GLenum dfactor );
void ( WINAPIV *qglClear )( GLbitfield mask );
void ( WINAPIV *qglClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
void ( WINAPIV *qglClipPlane )( GLenum plane, const GLdouble *equation );
void ( WINAPIV *qglColor3f )( GLfloat red, GLfloat green, GLfloat blue );
void ( WINAPIV *qglColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
void ( WINAPIV *qglColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void ( WINAPIV *qglCullFace )( GLenum mode );
void ( WINAPIV *qglDeleteTextures )( GLsizei n, const GLuint *textures );
void ( WINAPIV *qglDepthFunc )( GLenum func );
void ( WINAPIV *qglDepthMask )( GLboolean flag );
void ( WINAPIV *qglDepthRange )( GLclampd zNear, GLclampd zFar );
void ( WINAPIV *qglDisable )( GLenum cap );
void ( WINAPIV *qglDisableClientState )( GLenum array );
void ( WINAPIV *qglDrawBuffer )( GLenum mode );
void ( WINAPIV *qglDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
void ( WINAPIV *qglEnable )( GLenum cap );
void ( WINAPIV *qglEnableClientState )( GLenum array );
void ( WINAPIV *qglEnd )( void );
void ( WINAPIV *qglFinish )( void );
GLenum(WINAPIV *qglGetError)( void );
void ( WINAPIV *qglGetIntegerv )( GLenum pname, GLint *params );
const GLubyte *( WINAPIV *qglGetString )( GLenum name );
void ( WINAPIV *qglLineWidth )( GLfloat width );
void ( WINAPIV *qglLoadIdentity )( void );
void ( WINAPIV *qglLoadMatrixf )( const GLfloat *m );
void ( WINAPIV *qglMatrixMode )( GLenum mode );
void ( WINAPIV *qglOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
void ( WINAPIV *qglPolygonMode )( GLenum face, GLenum mode );
void ( WINAPIV *qglPolygonOffset )( GLfloat factor, GLfloat units );
void ( WINAPIV *qglPopMatrix )( void );
void ( WINAPIV *qglPushMatrix )( void );
void ( WINAPIV *qglReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
void ( WINAPIV *qglScissor )( GLint x, GLint y, GLsizei width, GLsizei height );
void ( WINAPIV *qglStencilFunc )( GLenum func, GLint ref, GLuint mask );
void ( WINAPIV *qglStencilOp )( GLenum fail, GLenum zfail, GLenum zpass );
void ( WINAPIV *qglTexCoord2f )( GLfloat s, GLfloat t );
void ( WINAPIV *qglTexCoord2fv )( const GLfloat *v );
void ( WINAPIV *qglTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void ( WINAPIV *qglTexEnvf )( GLenum target, GLenum pname, GLfloat param );
void ( WINAPIV *qglTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void ( WINAPIV *qglTexParameterf )( GLenum target, GLenum pname, GLfloat param );
void ( WINAPIV *qglTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params );
void ( WINAPIV *qglTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
void ( WINAPIV *qglVertex2f )( GLfloat x, GLfloat y );
void ( WINAPIV *qglVertex3f )( GLfloat x, GLfloat y, GLfloat z );
void ( WINAPIV *qglVertex3fv )( const GLfloat *v );
void ( WINAPIV *qglVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
void ( WINAPIV *qglViewport )( GLint x, GLint y, GLsizei width, GLsizei height );

static void ( WINAPIV *dllAlphaFunc )( GLenum func, GLclampf ref );
static void ( WINAPIV *dllBegin )( GLenum mode );
static void ( WINAPIV *dllBindTexture )( GLenum target, GLuint texture );
static void ( WINAPIV *dllBlendFunc )( GLenum sfactor, GLenum dfactor );
static void ( WINAPIV *dllClear )( GLbitfield mask );
static void ( WINAPIV *dllClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
static void ( WINAPIV *dllClipPlane )( GLenum plane, const GLdouble *equation );
static void ( WINAPIV *dllColor3f )( GLfloat red, GLfloat green, GLfloat blue );
static void ( WINAPIV *dllColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
static void ( WINAPIV *dllColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( WINAPIV *dllCullFace )( GLenum mode );
static void ( WINAPIV *dllDeleteTextures )( GLsizei n, const GLuint *textures );
static void ( WINAPIV *dllDepthFunc )( GLenum func );
static void ( WINAPIV *dllDepthMask )( GLboolean flag );
static void ( WINAPIV *dllDepthRange )( GLclampd zNear, GLclampd zFar );
static void ( WINAPIV *dllDisable )( GLenum cap );
static void ( WINAPIV *dllDisableClientState )( GLenum array );
static void ( WINAPIV *dllDrawBuffer )( GLenum mode );
static void ( WINAPIV *dllDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
static void ( WINAPIV *dllEnable )( GLenum cap );
static void ( WINAPIV *dllEnableClientState )( GLenum array );
static void ( WINAPIV *dllEnd )( void );
static void ( WINAPIV *dllFinish )( void );
GLenum(WINAPIV *dllGetError)( void );
static void ( WINAPIV *dllGetIntegerv )( GLenum pname, GLint *params );
const GLubyte *( WINAPIV *dllGetString )( GLenum name );
static void ( WINAPIV *dllLineWidth )( GLfloat width );
static void ( WINAPIV *dllLoadIdentity )( void );
static void ( WINAPIV *dllLoadMatrixf )( const GLfloat *m );
static void ( WINAPIV *dllMatrixMode )( GLenum mode );
static void ( WINAPIV *dllOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
static void ( WINAPIV *dllPolygonMode )( GLenum face, GLenum mode );
static void ( WINAPIV *dllPolygonOffset )( GLfloat factor, GLfloat units );
static void ( WINAPIV *dllPopMatrix )( void );
static void ( WINAPIV *dllPushMatrix )( void );
static void ( WINAPIV *dllReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
static void ( WINAPIV *dllScissor )( GLint x, GLint y, GLsizei width, GLsizei height );
static void ( WINAPIV *dllStencilFunc )( GLenum func, GLint ref, GLuint mask );
static void ( WINAPIV *dllStencilOp )( GLenum fail, GLenum zfail, GLenum zpass );
static void ( WINAPIV *dllTexCoord2f )( GLfloat s, GLfloat t );
static void ( WINAPIV *dllTexCoord2fv )( const GLfloat *v );
static void ( WINAPIV *dllTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( WINAPIV *dllTexEnvf )( GLenum target, GLenum pname, GLfloat param );
static void ( WINAPIV *dllTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
static void ( WINAPIV *dllTexParameterf )( GLenum target, GLenum pname, GLfloat param );
static void ( WINAPIV *dllTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params );
static void ( WINAPIV *dllTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
static void ( WINAPIV *dllVertex2f )( GLfloat x, GLfloat y );
static void ( WINAPIV *dllVertex3f )( GLfloat x, GLfloat y, GLfloat z );
static void ( WINAPIV *dllVertex3fv )( const GLfloat *v );
static void ( WINAPIV *dllVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( WINAPIV *dllViewport )( GLint x, GLint y, GLsizei width, GLsizei height );

//
// Placeholder functions to replace OpenGL calls when Vulkan renderer is active.
//
static HGLRC nowglCreateContext(HDC) {
	return NULL;
}
static BOOL  nowglDeleteContext(HGLRC) {
	return FALSE;
}
static PROC  nowglGetProcAddress(LPCSTR) {
	return NULL;
}
static BOOL  nowglMakeCurrent(HDC, HGLRC) {
	return FALSE;
}
static int   nowglSwapIntervalEXT(int interval) {
	return -1;
}

static void noglActiveTextureARB(GLenum texture) {}
static void noglClientActiveTextureARB(GLenum texture) {}
static void noglLockArraysEXT(GLint, GLint) {}
static void noglUnlockArraysEXT(void) {}
static void noglAlphaFunc(GLenum func, GLclampf ref) {}
static void noglBegin(GLenum mode) {}
static void noglBindTexture(GLenum target, GLuint texture) {}
static void noglBlendFunc(GLenum sfactor, GLenum dfactor) {}
static void noglClear(GLbitfield mask) {}
static void noglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}
static void noglClipPlane(GLenum plane, const GLdouble *equation) {}
static void noglColor3f(GLfloat red, GLfloat green, GLfloat blue) {}
static void noglColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
static void noglColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglCullFace(GLenum mode) {}
static void noglDeleteTextures(GLsizei n, const GLuint *textures) {}
static void noglDepthFunc(GLenum func) {}
static void noglDepthMask(GLboolean flag) {}
static void noglDepthRange(GLclampd zNear, GLclampd zFar) {}
static void noglDisable(GLenum cap) {}
static void noglDisableClientState(GLenum array) {}
static void noglDrawBuffer(GLenum mode) {}
static void noglDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {}
static void noglEnable(GLenum cap) {}
static void noglEnableClientState(GLenum array) {}
static void noglEnd(void) {}
static void noglFinish(void) {}
static GLenum noglGetError(void) {
	return GL_NO_ERROR;
}
static void noglGetIntegerv(GLenum pname, GLint *params) {}
static const GLubyte *noglGetString(GLenum name) {
	static char *s = ""; return (GLubyte *)s;
}
static void noglLineWidth(GLfloat width) {}
static void noglLoadIdentity(void) {}
static void noglLoadMatrixf(const GLfloat *m) {}
static void noglMatrixMode(GLenum mode) {}
static void noglOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {}
static void noglPolygonMode(GLenum face, GLenum mode) {}
static void noglPolygonOffset(GLfloat factor, GLfloat units) {}
static void noglPopMatrix(void) {}
static void noglPushMatrix(void) {}
static void noglReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {}
static void noglScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
static void noglStencilFunc(GLenum func, GLint ref, GLuint mask) {}
static void noglStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
static void noglTexCoord2f(GLfloat s, GLfloat t) {}
static void noglTexCoord2fv(const GLfloat *v) {}
static void noglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglTexEnvf(GLenum target, GLenum pname, GLfloat param) {}
static void noglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {}
static void noglTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
static void noglTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {}
static void noglTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {}
static void noglVertex2f(GLfloat x, GLfloat y) {}
static void noglVertex3f(GLfloat x, GLfloat y, GLfloat z) {}
static void noglVertex3fv(const GLfloat *v) {}
static void noglVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static void noglViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}

static const char *BooleanToString(GLboolean b) {
	if( b == GL_FALSE )
		return "GL_FALSE";
	else if( b == GL_TRUE )
		return "GL_TRUE";
	else
		return "OUT OF RANGE FOR BOOLEAN";
}

static const char *FuncToString(GLenum f) {
	switch( f ) 	{
		case GL_ALWAYS:
			return "GL_ALWAYS";
		case GL_NEVER:
			return "GL_NEVER";
		case GL_LEQUAL:
			return "GL_LEQUAL";
		case GL_LESS:
			return "GL_LESS";
		case GL_EQUAL:
			return "GL_EQUAL";
		case GL_GREATER:
			return "GL_GREATER";
		case GL_GEQUAL:
			return "GL_GEQUAL";
		case GL_NOTEQUAL:
			return "GL_NOTEQUAL";
		default:
			return "!!! UNKNOWN !!!";
	}
}

static const char *PrimToString(GLenum mode) {
	static char prim[1024];

	if( mode == GL_TRIANGLES )
		strcpy(prim, "GL_TRIANGLES");
	else if( mode == GL_TRIANGLE_STRIP )
		strcpy(prim, "GL_TRIANGLE_STRIP");
	else if( mode == GL_TRIANGLE_FAN )
		strcpy(prim, "GL_TRIANGLE_FAN");
	else if( mode == GL_QUADS )
		strcpy(prim, "GL_QUADS");
	else if( mode == GL_QUAD_STRIP )
		strcpy(prim, "GL_QUAD_STRIP");
	else if( mode == GL_POLYGON )
		strcpy(prim, "GL_POLYGON");
	else if( mode == GL_POINTS )
		strcpy(prim, "GL_POINTS");
	else if( mode == GL_LINES )
		strcpy(prim, "GL_LINES");
	else if( mode == GL_LINE_STRIP )
		strcpy(prim, "GL_LINE_STRIP");
	else if( mode == GL_LINE_LOOP )
		strcpy(prim, "GL_LINE_LOOP");
	else
		sprintf(prim, "0x%x", mode);

	return prim;
}

static const char *CapToString(GLenum cap) {
	static char buffer[1024];

	switch( cap ) 	{
		case GL_TEXTURE_2D:
			return "GL_TEXTURE_2D";
		case GL_BLEND:
			return "GL_BLEND";
		case GL_DEPTH_TEST:
			return "GL_DEPTH_TEST";
		case GL_CULL_FACE:
			return "GL_CULL_FACE";
		case GL_CLIP_PLANE0:
			return "GL_CLIP_PLANE0";
		case GL_COLOR_ARRAY:
			return "GL_COLOR_ARRAY";
		case GL_TEXTURE_COORD_ARRAY:
			return "GL_TEXTURE_COORD_ARRAY";
		case GL_VERTEX_ARRAY:
			return "GL_VERTEX_ARRAY";
		case GL_ALPHA_TEST:
			return "GL_ALPHA_TEST";
		case GL_STENCIL_TEST:
			return "GL_STENCIL_TEST";
		default:
			sprintf(buffer, "0x%x", cap);
	}

	return buffer;
}

static const char *TypeToString(GLenum t) {
	switch( t ) 	{
		case GL_BYTE:
			return "GL_BYTE";
		case GL_UNSIGNED_BYTE:
			return "GL_UNSIGNED_BYTE";
		case GL_SHORT:
			return "GL_SHORT";
		case GL_UNSIGNED_SHORT:
			return "GL_UNSIGNED_SHORT";
		case GL_INT:
			return "GL_INT";
		case GL_UNSIGNED_INT:
			return "GL_UNSIGNED_INT";
		case GL_FLOAT:
			return "GL_FLOAT";
		case GL_DOUBLE:
			return "GL_DOUBLE";
		default:
			return "!!! UNKNOWN !!!";
	}
}

static void WINAPIV logAlphaFunc(GLenum func, GLclampf ref) {
	fprintf(log_fp, "glAlphaFunc( 0x%x, %f )\n", func, ref);
	dllAlphaFunc(func, ref);
}
static void WINAPIV logBegin(GLenum mode) {
	fprintf(log_fp, "glBegin( %s )\n", PrimToString(mode));
	dllBegin(mode);
}
static void WINAPIV logBindTexture(GLenum target, GLuint texture) {
	fprintf(log_fp, "glBindTexture( 0x%x, %u )\n", target, texture);
	dllBindTexture(target, texture);
}
static void BlendToName(char *n, GLenum f) {
	switch( f ) 	{
		case GL_ONE:
			strcpy(n, "GL_ONE");
			break;
		case GL_ZERO:
			strcpy(n, "GL_ZERO");
			break;
		case GL_SRC_ALPHA:
			strcpy(n, "GL_SRC_ALPHA");
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			strcpy(n, "GL_ONE_MINUS_SRC_ALPHA");
			break;
		case GL_DST_COLOR:
			strcpy(n, "GL_DST_COLOR");
			break;
		case GL_ONE_MINUS_DST_COLOR:
			strcpy(n, "GL_ONE_MINUS_DST_COLOR");
			break;
		case GL_DST_ALPHA:
			strcpy(n, "GL_DST_ALPHA");
			break;
		default:
			sprintf(n, "0x%x", f);
	}
}
static void WINAPIV logBlendFunc(GLenum sfactor, GLenum dfactor) {
	char sf[128], df[128];

	BlendToName(sf, sfactor);
	BlendToName(df, dfactor);

	fprintf(log_fp, "glBlendFunc( %s, %s )\n", sf, df);
	dllBlendFunc(sfactor, dfactor);
}
static void WINAPIV logClear(GLbitfield mask) {
	fprintf(log_fp, "glClear( 0x%x = ", mask);

	if( mask & GL_COLOR_BUFFER_BIT )
		fprintf(log_fp, "GL_COLOR_BUFFER_BIT ");
	if( mask & GL_DEPTH_BUFFER_BIT )
		fprintf(log_fp, "GL_DEPTH_BUFFER_BIT ");
	if( mask & GL_STENCIL_BUFFER_BIT )
		fprintf(log_fp, "GL_STENCIL_BUFFER_BIT ");
	if( mask & GL_ACCUM_BUFFER_BIT )
		fprintf(log_fp, "GL_ACCUM_BUFFER_BIT ");

	fprintf(log_fp, ")\n");
	dllClear(mask);
}
static void WINAPIV logClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	fprintf(log_fp, "glClearColor\n");
	dllClearColor(red, green, blue, alpha);
}
static void WINAPIV logClipPlane(GLenum plane, const GLdouble *equation) {
	fprintf(log_fp, "glClipPlane\n");
	dllClipPlane(plane, equation);
}
static void WINAPIV logColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	fprintf(log_fp, "glColor3f\n");
	dllColor3f(red, green, blue);
}

#define SIG( x ) fprintf( log_fp, x "\n" )

static void WINAPIV logColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	SIG("glColorMask");
	dllColorMask(red, green, blue, alpha);
}
static void WINAPIV logColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
	fprintf(log_fp, "glColorPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	dllColorPointer(size, type, stride, pointer);
}
static void WINAPIV logCullFace(GLenum mode) {
	fprintf(log_fp, "glCullFace( %s )\n", ( mode == GL_FRONT ) ? "GL_FRONT" : "GL_BACK");
	dllCullFace(mode);
}
static void WINAPIV logDeleteTextures(GLsizei n, const GLuint *textures) {
	SIG("glDeleteTextures");
	dllDeleteTextures(n, textures);
}
static void WINAPIV logDepthFunc(GLenum func) {
	fprintf(log_fp, "glDepthFunc( %s )\n", FuncToString(func));
	dllDepthFunc(func);
}
static void WINAPIV logDepthMask(GLboolean flag) {
	fprintf(log_fp, "glDepthMask( %s )\n", BooleanToString(flag));
	dllDepthMask(flag);
}
static void WINAPIV logDepthRange(GLclampd zNear, GLclampd zFar) {
	fprintf(log_fp, "glDepthRange( %f, %f )\n", (float)zNear, (float)zFar);
	dllDepthRange(zNear, zFar);
}
static void WINAPIV logDisable(GLenum cap) {
	fprintf(log_fp, "glDisable( %s )\n", CapToString(cap));
	dllDisable(cap);
}
static void WINAPIV logDisableClientState(GLenum array) {
	fprintf(log_fp, "glDisableClientState( %s )\n", CapToString(array));
	dllDisableClientState(array);
}
static void WINAPIV logDrawBuffer(GLenum mode) {
	SIG("glDrawBuffer");
	dllDrawBuffer(mode);
}
static void WINAPIV logDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
	fprintf(log_fp, "glDrawElements( %s, %d, %s, MEM )\n", PrimToString(mode), count, TypeToString(type));
	dllDrawElements(mode, count, type, indices);
}
static void WINAPIV logEnable(GLenum cap) {
	fprintf(log_fp, "glEnable( %s )\n", CapToString(cap));
	dllEnable(cap);
}

static void WINAPIV logEnableClientState(GLenum array) {
	fprintf(log_fp, "glEnableClientState( %s )\n", CapToString(array));
	dllEnableClientState(array);
}

static void WINAPIV logEnd(void) {
	SIG("glEnd");
	dllEnd();
}
static void WINAPIV logFinish(void) {
	SIG("glFinish");
	dllFinish();
}
static GLenum WINAPIV logGetError(void) {
	SIG("glGetError");
	return dllGetError();
}
static void WINAPIV logGetIntegerv(GLenum pname, GLint *params) {
	SIG("glGetIntegerv");
	dllGetIntegerv(pname, params);
}
static const GLubyte *WINAPIV logGetString(GLenum name) {
	SIG("glGetString");
	return dllGetString(name);
}
static void WINAPIV logLineWidth(GLfloat width) {
	SIG("glLineWidth");
	dllLineWidth(width);
}
static void WINAPIV logLoadIdentity(void) {
	SIG("glLoadIdentity");
	dllLoadIdentity();
}
static void WINAPIV logLoadMatrixf(const GLfloat *m) {
	SIG("glLoadMatrixf");
	dllLoadMatrixf(m);
}
static void WINAPIV logMatrixMode(GLenum mode) {
	SIG("glMatrixMode");
	dllMatrixMode(mode);
}
static void WINAPIV logOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	SIG("glOrtho");
	dllOrtho(left, right, bottom, top, zNear, zFar);
}
static void WINAPIV logPolygonMode(GLenum face, GLenum mode) {
	fprintf(log_fp, "glPolygonMode( 0x%x, 0x%x )\n", face, mode);
	dllPolygonMode(face, mode);
}
static void WINAPIV logPolygonOffset(GLfloat factor, GLfloat units) {
	SIG("glPolygonOffset");
	dllPolygonOffset(factor, units);
}
static void WINAPIV logPopMatrix(void) {
	SIG("glPopMatrix");
	dllPopMatrix();
}
static void WINAPIV logPushMatrix(void) {
	SIG("glPushMatrix");
	dllPushMatrix();
}
static void WINAPIV logReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	SIG("glReadPixels");
	dllReadPixels(x, y, width, height, format, type, pixels);
}
static void WINAPIV logScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	fprintf(log_fp, "glScissor( %d, %d, %d, %d )\n", x, y, width, height);
	dllScissor(x, y, width, height);
}
static void WINAPIV logStencilFunc(GLenum func, GLint ref, GLuint mask) {
	SIG("glStencilFunc");
	dllStencilFunc(func, ref, mask);
}
static void WINAPIV logStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
	SIG("glStencilOp");
	dllStencilOp(fail, zfail, zpass);
}
static void WINAPIV logTexCoord2f(GLfloat s, GLfloat t) {
	SIG("glTexCoord2f");
	dllTexCoord2f(s, t);
}
static void WINAPIV logTexCoord2fv(const GLfloat *v) {
	SIG("glTexCoord2fv");
	dllTexCoord2fv(v);
}
static void WINAPIV logTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
	fprintf(log_fp, "glTexCoordPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	dllTexCoordPointer(size, type, stride, pointer);
}

static void WINAPIV logTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	fprintf(log_fp, "glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param);
	dllTexEnvf(target, pname, param);
}
static void WINAPIV logTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	SIG("glTexImage2D");
	dllTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
static void WINAPIV logTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	fprintf(log_fp, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param);
	dllTexParameterf(target, pname, param);
}
static void WINAPIV logTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
	SIG("glTexParameterfv");
	dllTexParameterfv(target, pname, params);
}
static void WINAPIV logTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	SIG("glTexSubImage2D");
	dllTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
static void WINAPIV logVertex2f(GLfloat x, GLfloat y) {
	SIG("glVertex2f");
	dllVertex2f(x, y);
}
static void WINAPIV logVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	SIG("glVertex3f");
	dllVertex3f(x, y, z);
}
static void WINAPIV logVertex3fv(const GLfloat *v) {
	SIG("glVertex3fv");
	dllVertex3fv(v);
}
static void WINAPIV logVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
	fprintf(log_fp, "glVertexPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	dllVertexPointer(size, type, stride, pointer);
}
static void WINAPIV logViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	fprintf(log_fp, "glViewport( %d, %d, %d, %d )\n", x, y, width, height);
	dllViewport(x, y, width, height);
}

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown(void) {
	if( hinstOpenGL ) 	{
		ri.Printf(PRINT_ALL, "...shutting down QGL\n");
		ri.Printf(PRINT_ALL, "...unloading OpenGL DLL\n");
		FreeLibrary(hinstOpenGL);
		hinstOpenGL = NULL;
	}

	qglAlphaFunc = NULL;
	qglBegin = NULL;
	qglBindTexture = NULL;
	qglBlendFunc = NULL;
	qglClear = NULL;
	qglClearColor = NULL;
	qglClipPlane = NULL;
	qglColor3f = NULL;
	qglColorMask = NULL;
	qglColorPointer = NULL;
	qglCullFace = NULL;
	qglDeleteTextures = NULL;
	qglDepthFunc = NULL;
	qglDepthMask = NULL;
	qglDepthRange = NULL;
	qglDisable = NULL;
	qglDisableClientState = NULL;
	qglDrawBuffer = NULL;
	qglDrawElements = NULL;
	qglEnable = NULL;
	qglEnableClientState = NULL;
	qglEnd = NULL;
	qglFinish = NULL;
	qglGetError = NULL;
	qglGetIntegerv = NULL;
	qglGetString = NULL;
	qglLineWidth = NULL;
	qglLoadIdentity = NULL;
	qglLoadMatrixf = NULL;
	qglMatrixMode = NULL;
	qglOrtho = NULL;
	qglPolygonMode = NULL;
	qglPolygonOffset = NULL;
	qglPopMatrix = NULL;
	qglPushMatrix = NULL;
	qglReadPixels = NULL;
	qglScissor = NULL;
	qglStencilFunc = NULL;
	qglStencilOp = NULL;
	qglTexCoord2f = NULL;
	qglTexCoord2fv = NULL;
	qglTexCoordPointer = NULL;
	qglTexEnvf = NULL;
	qglTexImage2D = NULL;
	qglTexParameterf = NULL;
	qglTexParameterfv = NULL;
	qglTexSubImage2D = NULL;
	qglVertex2f = NULL;
	qglVertex3f = NULL;
	qglVertex3fv = NULL;
	qglVertexPointer = NULL;
	qglViewport = NULL;

	qwglCreateContext = NULL;
	qwglDeleteContext = NULL;
	qwglGetProcAddress = NULL;
	qwglMakeCurrent = NULL;
}

#	pragma warning (disable : 4113 4133 4047 )
#	define GPA( a ) (dllname ? (void*)GetProcAddress(hinstOpenGL, #a) : (void*)(&no ## a))

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
*/
qboolean QGL_Init(const char *dllname) {
	if( dllname != nullptr ) {
		assert(hinstOpenGL == 0);

		ri.Printf(PRINT_ALL, "...initializing QGL\n");
		ri.Printf(PRINT_ALL, "...calling LoadLibrary('%s'): ", dllname);

		if( ( hinstOpenGL = LoadLibrary(dllname) ) == 0 ) 		{
			ri.Printf(PRINT_ALL, "failed\n");
			return qfalse;
		}
		ri.Printf(PRINT_ALL, "succeeded\n");
	}

	qglAlphaFunc = dllAlphaFunc = (decltype( dllAlphaFunc ))GPA(glAlphaFunc);
	qglBegin = dllBegin = (decltype( dllBegin ))GPA(glBegin);
	qglBindTexture = dllBindTexture = (decltype( dllBindTexture ))GPA(glBindTexture);
	qglBlendFunc = dllBlendFunc = (decltype( dllBlendFunc ))GPA(glBlendFunc);
	qglClear = dllClear = (decltype( dllClear ))GPA(glClear);
	qglClearColor = dllClearColor = (decltype( dllClearColor ))GPA(glClearColor);
	qglClipPlane = dllClipPlane = (decltype( dllClipPlane ))GPA(glClipPlane);
	qglColor3f = dllColor3f = (decltype( dllColor3f ))GPA(glColor3f);
	qglColorMask = dllColorMask = (decltype( dllColorMask ))GPA(glColorMask);
	qglColorPointer = dllColorPointer = (decltype( dllColorPointer ))GPA(glColorPointer);
	qglCullFace = dllCullFace = (decltype( dllCullFace ))GPA(glCullFace);
	qglDeleteTextures = dllDeleteTextures = (decltype( dllDeleteTextures ))GPA(glDeleteTextures);
	qglDepthFunc = dllDepthFunc = (decltype( dllDepthFunc ))GPA(glDepthFunc);
	qglDepthMask = dllDepthMask = (decltype( dllDepthMask ))GPA(glDepthMask);
	qglDepthRange = dllDepthRange = (decltype( dllDepthRange ))GPA(glDepthRange);
	qglDisable = dllDisable = (decltype( dllDisable ))GPA(glDisable);
	qglDisableClientState = dllDisableClientState = (decltype( dllDisableClientState ))GPA(glDisableClientState);
	qglDrawBuffer = dllDrawBuffer = (decltype( dllDrawBuffer ))GPA(glDrawBuffer);
	qglDrawElements = dllDrawElements = (decltype( dllDrawElements ))GPA(glDrawElements);
	qglEnable = dllEnable = (decltype( dllEnable ))GPA(glEnable);
	qglEnableClientState = dllEnableClientState = (decltype( dllEnableClientState ))GPA(glEnableClientState);
	qglEnd = dllEnd = (decltype( dllEnd ))GPA(glEnd);
	qglFinish = dllFinish = (decltype( dllFinish ))GPA(glFinish);
	qglGetError = dllGetError = ( GLenum(WINAPIV *)( void ) ) GPA(glGetError);
	qglGetIntegerv = dllGetIntegerv = (decltype( dllGetIntegerv ))GPA(glGetIntegerv);
	qglGetString = dllGetString = (decltype( dllGetString ))GPA(glGetString);
	qglLineWidth = dllLineWidth = (decltype( dllLineWidth ))GPA(glLineWidth);
	qglLoadIdentity = dllLoadIdentity = (decltype( dllLoadIdentity ))GPA(glLoadIdentity);
	qglLoadMatrixf = dllLoadMatrixf = (decltype( dllLoadMatrixf ))GPA(glLoadMatrixf);
	qglMatrixMode = dllMatrixMode = (decltype( dllMatrixMode ))GPA(glMatrixMode);
	qglOrtho = dllOrtho = (decltype( dllOrtho ))GPA(glOrtho);
	qglPolygonMode = dllPolygonMode = (decltype( dllPolygonMode ))GPA(glPolygonMode);
	qglPolygonOffset = dllPolygonOffset = (decltype( dllPolygonOffset ))GPA(glPolygonOffset);
	qglPopMatrix = dllPopMatrix = (decltype( dllPopMatrix ))GPA(glPopMatrix);
	qglPushMatrix = dllPushMatrix = (decltype( dllPushMatrix ))GPA(glPushMatrix);
	qglReadPixels = dllReadPixels = (decltype( dllReadPixels ))GPA(glReadPixels);
	qglScissor = dllScissor = (decltype( dllScissor ))GPA(glScissor);
	qglStencilFunc = dllStencilFunc = (decltype( dllStencilFunc ))GPA(glStencilFunc);
	qglStencilOp = dllStencilOp = (decltype( dllStencilOp ))GPA(glStencilOp);
	qglTexCoord2f = dllTexCoord2f = (decltype( dllTexCoord2f ))GPA(glTexCoord2f);
	qglTexCoord2fv = dllTexCoord2fv = (decltype( dllTexCoord2fv ))GPA(glTexCoord2fv);
	qglTexCoordPointer = dllTexCoordPointer = (decltype( dllTexCoordPointer ))GPA(glTexCoordPointer);
	qglTexEnvf = dllTexEnvf = (decltype( dllTexEnvf ))GPA(glTexEnvf);
	qglTexImage2D = dllTexImage2D = (decltype( dllTexImage2D ))GPA(glTexImage2D);
	qglTexParameterf = dllTexParameterf = (decltype( dllTexParameterf ))GPA(glTexParameterf);
	qglTexParameterfv = dllTexParameterfv = (decltype( dllTexParameterfv ))GPA(glTexParameterfv);
	qglTexSubImage2D = dllTexSubImage2D = (decltype( dllTexSubImage2D ))GPA(glTexSubImage2D);
	qglVertex2f = dllVertex2f = (decltype( dllVertex2f ))GPA(glVertex2f);
	qglVertex3f = dllVertex3f = (decltype( dllVertex3f ))GPA(glVertex3f);
	qglVertex3fv = dllVertex3fv = (decltype( dllVertex3fv ))GPA(glVertex3fv);
	qglVertexPointer = dllVertexPointer = (decltype( dllVertexPointer ))GPA(glVertexPointer);
	qglViewport = dllViewport = (decltype( dllViewport ))GPA(glViewport);

	qwglCreateContext = (decltype( qwglCreateContext ))GPA(wglCreateContext);
	qwglDeleteContext = (decltype( qwglDeleteContext ))GPA(wglDeleteContext);
	qwglGetProcAddress = (decltype( qwglGetProcAddress ))GPA(wglGetProcAddress);
	qwglMakeCurrent = (decltype( qwglMakeCurrent ))GPA(wglMakeCurrent);

	qwglSwapIntervalEXT = 0;
	qglActiveTextureARB = 0;
	qglClientActiveTextureARB = 0;
	qglLockArraysEXT = 0;
	qglUnlockArraysEXT = 0;

	if( dllname != nullptr ) {

		// check logging
		QGL_EnableLogging((qboolean)r_logFile->integer);
	}

	return qtrue;
}

void QGL_EnableLogging(qboolean enable) {
	static qboolean isEnabled;

	// return if we're already active
	if( isEnabled && enable ) {

		// decrement log counter and stop if it has reached 0
		ri.Cvar_Set("r_logFile", va("%d", r_logFile->integer - 1));
		if( r_logFile->integer ) {
			return;
		}
		enable = qfalse;
	}

	// return if we're already disabled
	if( !enable && !isEnabled )
		return;

	isEnabled = enable;

	if( enable ) 	{
		if( !log_fp ) 		{
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];
			cvar_t *basedir;

			time(&aclock);
			newtime = localtime(&aclock);

			asctime(newtime);

			basedir = ri.Cvar_Get("fs_basepath", "", 0);
			Com_sprintf(buffer, sizeof(buffer), "%s/gl.log", basedir->string);
			log_fp = fopen(buffer, "wt");

			fprintf(log_fp, "%s\n", asctime(newtime));
		}

		qglAlphaFunc = logAlphaFunc;
		qglBegin = logBegin;
		qglBindTexture = logBindTexture;
		qglBlendFunc = logBlendFunc;
		qglClear = logClear;
		qglClearColor = logClearColor;
		qglClipPlane = logClipPlane;
		qglColor3f = logColor3f;
		qglColorMask = logColorMask;
		qglColorPointer = logColorPointer;
		qglCullFace = logCullFace;
		qglDeleteTextures = logDeleteTextures;
		qglDepthFunc = logDepthFunc;
		qglDepthMask = logDepthMask;
		qglDepthRange = logDepthRange;
		qglDisable = logDisable;
		qglDisableClientState = logDisableClientState;
		qglDrawBuffer = logDrawBuffer;
		qglDrawElements = logDrawElements;
		qglEnable = logEnable;
		qglEnableClientState = logEnableClientState;
		qglEnd = logEnd;
		qglFinish = logFinish;
		qglGetError = logGetError;
		qglGetIntegerv = logGetIntegerv;
		qglGetString = logGetString;
		qglLineWidth = logLineWidth;
		qglLoadIdentity = logLoadIdentity;
		qglLoadMatrixf = logLoadMatrixf;
		qglMatrixMode = logMatrixMode;
		qglOrtho = logOrtho;
		qglPolygonMode = logPolygonMode;
		qglPolygonOffset = logPolygonOffset;
		qglPopMatrix = logPopMatrix;
		qglPushMatrix = logPushMatrix;
		qglReadPixels = logReadPixels;
		qglScissor = logScissor;
		qglStencilFunc = logStencilFunc;
		qglStencilOp = logStencilOp;
		qglTexCoord2f = logTexCoord2f;
		qglTexCoord2fv = logTexCoord2fv;
		qglTexCoordPointer = logTexCoordPointer;
		qglTexEnvf = logTexEnvf;
		qglTexImage2D = logTexImage2D;
		qglTexParameterf = logTexParameterf;
		qglTexParameterfv = logTexParameterfv;
		qglTexSubImage2D = logTexSubImage2D;
		qglVertex2f = logVertex2f;
		qglVertex3f = logVertex3f;
		qglVertex3fv = logVertex3fv;
		qglVertexPointer = logVertexPointer;
		qglViewport = logViewport;
	} 	else 	{
		if( log_fp ) {
			fprintf(log_fp, "*** CLOSING LOG ***\n");
			fclose(log_fp);
			log_fp = NULL;
		}
		qglAlphaFunc = dllAlphaFunc;
		qglBegin = dllBegin;
		qglBindTexture = dllBindTexture;
		qglBlendFunc = dllBlendFunc;
		qglClear = dllClear;
		qglClearColor = dllClearColor;
		qglClipPlane = dllClipPlane;
		qglColor3f = dllColor3f;
		qglColorMask = dllColorMask;
		qglColorPointer = dllColorPointer;
		qglCullFace = dllCullFace;
		qglDeleteTextures = dllDeleteTextures;
		qglDepthFunc = dllDepthFunc;
		qglDepthMask = dllDepthMask;
		qglDepthRange = dllDepthRange;
		qglDisable = dllDisable;
		qglDisableClientState = dllDisableClientState;
		qglDrawBuffer = dllDrawBuffer;
		qglDrawElements = dllDrawElements;
		qglEnable = dllEnable;
		qglEnableClientState = dllEnableClientState;
		qglEnd = dllEnd;
		qglFinish = dllFinish;
		qglGetError = dllGetError;
		qglGetIntegerv = dllGetIntegerv;
		qglGetString = dllGetString;
		qglLineWidth = dllLineWidth;
		qglLoadIdentity = dllLoadIdentity;
		qglLoadMatrixf = dllLoadMatrixf;
		qglMatrixMode = dllMatrixMode;
		qglOrtho = dllOrtho;
		qglPolygonMode = dllPolygonMode;
		qglPolygonOffset = dllPolygonOffset;
		qglPopMatrix = dllPopMatrix;
		qglPushMatrix = dllPushMatrix;
		qglReadPixels = dllReadPixels;
		qglScissor = dllScissor;
		qglStencilFunc = dllStencilFunc;
		qglStencilOp = dllStencilOp;
		qglTexCoord2f = dllTexCoord2f;
		qglTexCoord2fv = dllTexCoord2fv;
		qglTexCoordPointer = dllTexCoordPointer;
		qglTexEnvf = dllTexEnvf;
		qglTexImage2D = dllTexImage2D;
		qglTexParameterf = dllTexParameterf;
		qglTexParameterfv = dllTexParameterfv;
		qglTexSubImage2D = dllTexSubImage2D;
		qglVertex2f = dllVertex2f;
		qglVertex3f = dllVertex3f;
		qglVertex3fv = dllVertex3fv;
		qglVertexPointer = dllVertexPointer;
		qglViewport = dllViewport;
	}
}

#pragma warning (default : 4113 4133 4047 )

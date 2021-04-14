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
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL/Vulkan refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_LogComment
** GLimp_Shutdown
**
** vk_imp_init
** vk_imp_shutdown
** vk_imp_create_surface
*/
#include "../tr_local.h"
#include "resource.h"
#include "win_local.h"

#include <SDL.h>

SDL_Window *SDL_window = NULL;

#define	MAIN_WINDOW_CLASS_NAME	"Quake 3: Arena"
#define	TWIN_WINDOW_CLASS_NAME	"Quake 3: Arena [Twin]"

static bool s_main_window_class_registered = false;
static bool s_twin_window_class_registered = false;

static HDC gl_hdc; // handle to device context
static HGLRC gl_hglrc; // handle to GL rendering context

WinVars_t	g_wv;

FILE *log_fp;

qboolean QGL_Init(const char *dllname);
void QGL_Shutdown();
void QGL_EnableLogging(qboolean enable);
void WG_CheckHardwareGamma();
void WG_RestoreGamma();

static int GetDesktopCaps(int index) {
	HDC hdc = GetDC(GetDesktopWindow());
	int value = GetDeviceCaps(hdc, index);
	ReleaseDC(GetDesktopWindow(), hdc);
	return value;
}
static int GetDesktopColorDepth() {
	return GetDesktopCaps(BITSPIXEL);
}
static int GetDesktopWidth() {
	return GetDesktopCaps(HORZRES);
}
static int GetDesktopHeight() {
	return GetDesktopCaps(VERTRES);
}

/*
** ChoosePFD
**
** Helper function that replaces ChoosePixelFormat.
*/
static int GLW_ChoosePixelFormat(HDC hDC, const PIXELFORMATDESCRIPTOR *pPFD) {
	const int MAX_PFDS = 512;
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS + 1];

	ri.Printf(PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n", (int)pPFD->cColorBits, (int)pPFD->cDepthBits, (int)pPFD->cStencilBits);

	// count number of PFDs
	int maxPFD = DescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[0]);
	if( maxPFD > MAX_PFDS ) {
		ri.Printf(PRINT_WARNING, "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS);
		maxPFD = MAX_PFDS;
	}

	ri.Printf(PRINT_ALL, "...%d PFDs found\n", maxPFD);

	// grab information
	for( int i = 1; i <= maxPFD; i++ ) {
		DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);
	}

	// look for a best match
	int bestMatch = 0;
	for( int i = 1; i <= maxPFD; i++ ) {
		if( bestMatch != 0 &&
		   ( pfds[bestMatch].dwFlags & PFD_STEREO ) == ( pPFD->dwFlags & PFD_STEREO ) &&
		   pfds[bestMatch].cColorBits == pPFD->cColorBits &&
		   pfds[bestMatch].cDepthBits == pPFD->cDepthBits &&
		   pfds[bestMatch].cStencilBits == pPFD->cStencilBits )         {
			break;
		}

		//
		// make sure this has hardware acceleration
		//
		if( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
			if( r_verbose->integer ) {
				ri.Printf(PRINT_ALL, "...PFD %d rejected, software acceleration\n", i);
			}
			continue;
		}

		// verify pixel type
		if( pfds[i].iPixelType != PFD_TYPE_RGBA ) {
			if( r_verbose->integer ) {
				ri.Printf(PRINT_ALL, "...PFD %d rejected, not RGBA\n", i);
			}
			continue;
		}

		// verify proper flags
		if( ( pfds[i].dwFlags & pPFD->dwFlags ) != pPFD->dwFlags ) {
			if( r_verbose->integer ) {
				ri.Printf(PRINT_ALL, "...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags);
			}
			continue;
		}

		// verify enough bits
		if( pfds[i].cDepthBits < 15 ) {
			continue;
		}
		if( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) ) {
			continue;
		}

		if( !bestMatch ) {
			bestMatch = i;
			continue;
		}

		//
		// selection criteria (in order of priority):
		//
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//
		bool same_stereo = ( pfds[i].dwFlags & PFD_STEREO ) == ( pfds[bestMatch].dwFlags & PFD_STEREO );
		bool better_stereo = ( pfds[i].dwFlags & PFD_STEREO ) == ( pPFD->dwFlags & PFD_STEREO ) &&
			( pfds[bestMatch].dwFlags & PFD_STEREO ) != ( pPFD->dwFlags & PFD_STEREO );

		bool same_color = pfds[i].cColorBits == pfds[bestMatch].cColorBits;
		bool better_color = ( pfds[bestMatch].cColorBits >= pPFD->cColorBits )
			? pfds[i].cColorBits >= pPFD->cColorBits && pfds[i].cColorBits < pfds[bestMatch].cColorBits
			: pfds[i].cColorBits > pfds[bestMatch].cColorBits;

		bool same_depth = pfds[i].cDepthBits == pfds[bestMatch].cDepthBits;
		bool better_depth = ( pfds[bestMatch].cDepthBits >= pPFD->cDepthBits )
			? pfds[i].cDepthBits >= pPFD->cDepthBits && pfds[i].cDepthBits < pfds[bestMatch].cDepthBits
			: pfds[i].cDepthBits > pfds[bestMatch].cDepthBits;

		bool better_stencil;
		if( pPFD->cStencilBits == 0 )
			better_stencil = pfds[i].cStencilBits == 0 && pfds[bestMatch].cStencilBits != 0;
		else
			better_stencil = ( pfds[bestMatch].cStencilBits >= pPFD->cStencilBits )
			? pfds[i].cStencilBits >= pPFD->cStencilBits && pfds[i].cStencilBits < pfds[bestMatch].cStencilBits
			: pfds[i].cStencilBits > pfds[bestMatch].cStencilBits;

		if( better_stereo )
			bestMatch = i;
		else if( same_stereo ) {
			if( better_color )
				bestMatch = i;
			else if( same_color ) {
				if( better_depth )
					bestMatch = i;
				else if( same_depth ) {
					if( better_stencil )
						bestMatch = i;
				}
			}
		}
	}

	if( !bestMatch )
		return 0;

	if( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 || ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED ) != 0 ) {
		ri.Printf(PRINT_ALL, "...no hardware acceleration found\n");
		return 0;
	}

	ri.Printf(PRINT_ALL, "...hardware acceleration found\n");
	return bestMatch;
}

static bool GLW_SetPixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *pPFD, int colorbits, int depthbits, int stencilbits, qboolean stereo) {
	const PIXELFORMATDESCRIPTOR pfd_base =
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		0,	                			// color bits
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		0,	                			// z-buffer	bits
		0,              	    		// stencil buffer bits
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};

	*pPFD = pfd_base;

	pPFD->cColorBits = (BYTE)colorbits;
	pPFD->cDepthBits = (BYTE)depthbits;
	pPFD->cStencilBits = (BYTE)stencilbits;

	if( stereo )     {
		ri.Printf(PRINT_ALL, "...attempting to use stereo\n");
		pPFD->dwFlags |= PFD_STEREO;
	}

	int pixelformat = GLW_ChoosePixelFormat(hdc, pPFD);
	if( pixelformat == 0 ) {
		ri.Printf(PRINT_ALL, "...GLW_ChoosePixelFormat failed\n");
		return false;
	}
	ri.Printf(PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelformat);

	DescribePixelFormat(hdc, pixelformat, sizeof(*pPFD), pPFD);
	if( SetPixelFormat(hdc, pixelformat, pPFD) == FALSE ) {
		ri.Printf(PRINT_ALL, "...SetPixelFormat failed\n", hdc);
		return false;
	}
	return true;
}

// Sets pixel format and creates opengl context for the given window.
static qboolean GLW_InitDriver(HWND hwnd) {
	ri.Printf(PRINT_ALL, "Initializing OpenGL driver\n");

	//
	// get a DC for our window
	//
	ri.Printf(PRINT_ALL, "...getting DC: ");
	gl_hdc = GetDC(hwnd);
	if( gl_hdc == NULL ) {
		ri.Printf(PRINT_ALL, "failed\n");
		return qfalse;
	}
	ri.Printf(PRINT_ALL, "succeeded\n");

	//
	// set pixel format
	//
	int colorbits = GetDesktopColorDepth();
	int depthbits = ( r_depthbits->integer == 0 ) ? 24 : r_depthbits->integer;
	int stencilbits = r_stencilbits->integer;

	PIXELFORMATDESCRIPTOR pfd;
	if( !GLW_SetPixelFormat(gl_hdc, &pfd, colorbits, depthbits, stencilbits, (qboolean)r_stereo->integer) ) {
		ReleaseDC(hwnd, gl_hdc);
		gl_hdc = NULL;
		ri.Printf(PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n");
		return qfalse;
	}

	// report if stereo is desired but unavailable
	if( !( pfd.dwFlags & PFD_STEREO ) && ( r_stereo->integer != 0 ) ) {
		ri.Printf(PRINT_ALL, "...failed to select stereo pixel format\n");
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	ri.Printf(PRINT_ALL, "...creating GL context: ");
	gl_hglrc = qwglCreateContext(gl_hdc);
	if( gl_hglrc == NULL ) {
		ReleaseDC(hwnd, gl_hdc);
		gl_hdc = NULL;
		ri.Printf(PRINT_ALL, "failed\n");
		return qfalse;
	}
	ri.Printf(PRINT_ALL, "succeeded\n");

	ri.Printf(PRINT_ALL, "...making context current: ");
	if( !qwglMakeCurrent(gl_hdc, gl_hglrc) ) {
		qwglDeleteContext(gl_hglrc);
		gl_hglrc = NULL;
		ReleaseDC(hwnd, gl_hdc);
		gl_hdc = NULL;
		ri.Printf(PRINT_ALL, "failed\n");
		return qfalse;
	}
	ri.Printf(PRINT_ALL, "succeeded\n");

	glConfig.colorBits = (int)pfd.cColorBits;
	glConfig.depthBits = (int)pfd.cDepthBits;
	glConfig.stencilBits = (int)pfd.cStencilBits;
	glConfig.stereoEnabled = ( pfd.dwFlags & PFD_STEREO ) ? qtrue : qfalse;
	return qtrue;
}

LRESULT __stdcall WndProcFull(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if( uMsg == WM_DESTROY ) {
		PostQuitMessage(0);
		return 0;
	} else if( uMsg == WM_CREATE ) {
		ShowCursor(FALSE);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static HWND create_main_window(int width, int height, qboolean fullscreen) {

	// We are a dll now so lets get this since sys main is different in ioQuakeIII
	g_wv.hInstance = GetModuleHandleA("renderer_d3d12_x86.dll"); //

		//
		// register the window class if necessary
		//
	if( !s_main_window_class_registered ) {

		WNDCLASS wc{};

		//memset( &wc, 0, sizeof( wc ) );

		wc.style = 0;
		wc.lpfnWndProc = WndProcFull; // wndproc
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = g_wv.hInstance;
		wc.hIcon = LoadIcon(g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(void *)COLOR_GRAYTEXT;
		wc.lpszMenuName = 0;
		wc.lpszClassName = MAIN_WINDOW_CLASS_NAME;

		if( !GetClassInfo(g_wv.hInstance, MAIN_WINDOW_CLASS_NAME, &wc) ) {
			if( !RegisterClass(&wc) ) {
				ri.Error(ERR_FATAL, "create_main_window: could not register window class");
			}
		}
		s_main_window_class_registered = true;
		ri.Printf(PRINT_ALL, "...registered window class\n");
	}

	//
	// compute width and height
	//
	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = width;
	r.bottom = height;

	int	stylebits;
	if( fullscreen ) 	{
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	} 	else 	{
		stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
		AdjustWindowRect(&r, stylebits, FALSE);
	}

	int w = r.right - r.left;
	int h = r.bottom - r.top;

	int x, y;

	if( fullscreen ) 	{
		x = 0;
		y = 0;
	} 	else 	{
		cvar_t *vid_xpos = ri.Cvar_Get("vid_xpos", "", 0);
		cvar_t *vid_ypos = ri.Cvar_Get("vid_ypos", "", 0);
		x = vid_xpos->integer;
		y = vid_ypos->integer;

		// adjust window coordinates if necessary
		// so that the window is completely on screen
		if( x < 0 )
			x = 0;
		if( y < 0 )
			y = 0;

		int desktop_width = GetDesktopWidth();
		int desktop_height = GetDesktopHeight();

		if( w < desktop_width && h < desktop_height ) 		{
			if( x + w > desktop_width )
				x = ( desktop_width - w );
			if( y + h > desktop_height )
				y = ( desktop_height - h );
		}
	}

	char window_name[1024];

	strcpy(window_name, MAIN_WINDOW_CLASS_NAME);

	HWND hwnd = CreateWindowEx(
		0,
		TEXT(MAIN_WINDOW_CLASS_NAME),
		TEXT(window_name),
		stylebits,
		x, y, w, h,
		NULL,
		NULL,
		g_wv.hInstance,
		NULL);

	if( !hwnd ) 	{
		ri.Error(ERR_FATAL, "create_main_window() - Couldn't create window");
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	ri.Printf(PRINT_ALL, "...created window@%d,%d (%dx%d)\n", x, y, w, h);

	return hwnd;
}

static void SetMode(int mode, qboolean fullscreen) {
	if( fullscreen ) {
		ri.Printf(PRINT_ALL, "...setting fullscreen mode:");
		glConfig.vidWidth = GetDesktopWidth();
		glConfig.vidHeight = GetDesktopHeight();
		glConfig.windowAspect = 1.0f;
	} else {
		ri.Printf(PRINT_ALL, "...setting mode %d:", mode);
		if( !R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode) ) {
			ri.Printf(PRINT_ALL, " invalid mode\n");
			ri.Error(ERR_FATAL, "SetMode - could not set the given mode (%d)\n", mode);
		}

		// Ensure that window size does not exceed desktop size.
		// CreateWindow Win32 API does not allow to create windows larger than desktop.
		int desktop_width = GetDesktopWidth();
		int desktop_height = GetDesktopHeight();

		if( glConfig.vidWidth > desktop_width || glConfig.vidHeight > desktop_height ) {
			int default_mode = 4;
			ri.Printf(PRINT_WARNING, "\nMode %d specifies width that is larger than desktop width: using default mode %d\n", mode, default_mode);

			ri.Printf(PRINT_ALL, "...setting mode %d:", default_mode);
			if( !R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, default_mode) ) {
				ri.Printf(PRINT_ALL, " invalid mode\n");
				ri.Error(ERR_FATAL, "SetMode - could not set the given mode (%d)\n", default_mode);
			}
		}
	}
	glConfig.isFullscreen = fullscreen;
	ri.Printf(PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, fullscreen ? "FS" : "W");
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions(void) {
	ri.Printf(PRINT_ALL, "Initializing OpenGL extensions\n");

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	if( strstr(glConfig.extensions_string, "GL_S3_s3tc") ) 	{
		if( r_ext_compressed_textures->integer ) 		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf(PRINT_ALL, "...using GL_S3_s3tc\n");
		} 		else 		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf(PRINT_ALL, "...ignoring GL_S3_s3tc\n");
		}
	} 	else 	{
		ri.Printf(PRINT_ALL, "...GL_S3_s3tc not found\n");
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if( strstr(glConfig.extensions_string, "EXT_texture_env_add") ) 	{
		if( r_ext_texture_env_add->integer ) 		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf(PRINT_ALL, "...using GL_EXT_texture_env_add\n");
		} 		else 		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n");
		}
	} 	else 	{
		ri.Printf(PRINT_ALL, "...GL_EXT_texture_env_add not found\n");
	}

	// WGL_EXT_swap_control
	qwglSwapIntervalEXT = ( BOOL(WINAPI *)( int ) ) qwglGetProcAddress("wglSwapIntervalEXT");
	if( qwglSwapIntervalEXT ) 	{
		ri.Printf(PRINT_ALL, "...using WGL_EXT_swap_control\n");
		r_swapInterval->modified = qtrue;	// force a set next frame
	} 	else 	{
		ri.Printf(PRINT_ALL, "...WGL_EXT_swap_control not found\n");
	}

	// GL_ARB_multitexture
	{
		if( !strstr(glConfig.extensions_string, "GL_ARB_multitexture") )
			ri.Error(ERR_FATAL, "GL_ARB_multitexture not found");

		qglActiveTextureARB = ( void ( WINAPIV * ) ( GLenum target ) ) qwglGetProcAddress("glActiveTextureARB");
		qglClientActiveTextureARB = ( void ( WINAPIV * ) ( GLenum target ) ) qwglGetProcAddress("glClientActiveTextureARB");

		if( !qglActiveTextureARB || !qglClientActiveTextureARB )
			ri.Error(ERR_FATAL, "GL_ARB_multitexture: could not initialize function pointers");

		qglGetIntegerv(GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.numTextureUnits);

		if( glConfig.numTextureUnits < 2 )
			ri.Error(ERR_FATAL, "GL_ARB_multitexture: < 2 texture units");

		ri.Printf(PRINT_ALL, "...using GL_ARB_multitexture\n");
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if( strstr(glConfig.extensions_string, "GL_EXT_compiled_vertex_array") ) 	{
		if( r_ext_compiled_vertex_array->integer ) 		{
			ri.Printf(PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n");
			qglLockArraysEXT = ( void ( WINAPIV * )( int, int ) ) qwglGetProcAddress("glLockArraysEXT");
			qglUnlockArraysEXT = ( void ( WINAPIV * )( void ) ) qwglGetProcAddress("glUnlockArraysEXT");
			if( !qglLockArraysEXT || !qglUnlockArraysEXT ) {
				ri.Error(ERR_FATAL, "bad getprocaddress");
			}
		} 		else 		{
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n");
		}
	} 	else 	{
		ri.Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n");
	}
}

void GLimp_LogComment(char *comment) {
	if( log_fp ) {
		fprintf(log_fp, "%s", comment);
	}
}

void dx_imp_init() {
	ri.Printf(PRINT_ALL, "Initializing DX12 subsystem\n");

	// This will set qgl pointers to no-op placeholders.
	QGL_Init(nullptr);
	qglActiveTextureARB = [](GLenum) {};
	qglClientActiveTextureARB = [](GLenum) {};

	// Create window.
	SetMode(r_mode->integer, (qboolean)r_fullscreen->integer);
	g_wv.hWnd_dx = create_main_window(glConfig.vidWidth, glConfig.vidHeight, (qboolean)r_fullscreen->integer);
	g_wv.hWnd = g_wv.hWnd_dx;
	SetForegroundWindow(g_wv.hWnd);
	SetFocus(g_wv.hWnd);
	WG_CheckHardwareGamma();

}

void dx_imp_shutdown() {
	ri.Printf(PRINT_ALL, "Shutting down DX12 subsystem\n");

	if( g_wv.hWnd_dx ) {
		ri.Printf(PRINT_ALL, "...destroying DX12 window\n");
		DestroyWindow(g_wv.hWnd_dx);

		if( g_wv.hWnd == g_wv.hWnd_dx ) {
			g_wv.hWnd = NULL;
		}
		g_wv.hWnd_dx = NULL;
	}

	if( g_wv.hInstance ) {
		g_wv.hInstance = NULL;
	}

	// For DX12 mode we still have qgl pointers initialized with placeholder values.
	// Reset them the same way as we do in opengl mode.
	QGL_Shutdown();

	WG_RestoreGamma();

	memset(&glConfig, 0, sizeof(glConfig));
	memset(&glState, 0, sizeof(glState));

	if( log_fp ) {
		fclose(log_fp);
		log_fp = 0;
	}
}

/*
===========================================================

SDL

	Call SDL_Init and pass an SDL_window from our HWnd
	so we can have input.

===========================================================
*/

/*
===============
GLimp_SwitchSDL
===============
*/

void GLimp_SwitchSDL(void) {

	if( SDL_window == NULL ) {
		SDL_Init(SDL_INIT_VIDEO);
		SetForegroundWindow(g_wv.hWnd);
		SetFocus(g_wv.hWnd);
		SDL_window = SDL_CreateWindowFrom(g_wv.hWnd);
		ri.IN_Init(SDL_window);
	} else {
		SDL_RaiseWindow(SDL_window);
		ri.IN_Restart();
	}

}

/*
===============
GLimp_ShutdownSDL
===============
*/
void GLimp_ShutdownSDL(bool destroyWindow) {

	if( destroyWindow ) {
		SDL_DestroyWindow(SDL_window);
		SDL_window = NULL;
		ri.IN_Shutdown();
		SDL_Quit();
	}

}

/*
===========================================================

SMP acceleration

===========================================================
*/

HANDLE	renderCommandsEvent;
HANDLE	renderCompletedEvent;
HANDLE	renderActiveEvent;

void ( *glimpRenderThread )( void );

void GLimp_RenderThreadWrapper(void) {
	glimpRenderThread();

	// unbind the context before we die
	qwglMakeCurrent(gl_hdc, NULL);
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
HANDLE	renderThreadHandle;
int		renderThreadId;
qboolean GLimp_SpawnRenderThread(void ( *function )( void )) {

	renderCommandsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	renderCompletedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	renderActiveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
		NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		0,		// DWORD cbStack,
		(LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
		0,			// LPVOID lpvThreadParm,
		0,			//   DWORD fdwCreate,
		(LPDWORD)&renderThreadId);

	if( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

static	void *smpData;
static	int		wglErrors;

void *GLimp_RendererSleep(void) {
	void *data;

	if( !qwglMakeCurrent(gl_hdc, NULL) ) {
		wglErrors++;
	}

	ResetEvent(renderActiveEvent);

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent(renderCompletedEvent);

	WaitForSingleObject(renderCommandsEvent, INFINITE);

	if( !qwglMakeCurrent(gl_hdc, gl_hglrc) ) {
		wglErrors++;
	}

	ResetEvent(renderCompletedEvent);
	ResetEvent(renderCommandsEvent);

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent(renderActiveEvent);

	return data;
}

void GLimp_FrontEndSleep(void) {
	WaitForSingleObject(renderCompletedEvent, INFINITE);

	if( !qwglMakeCurrent(gl_hdc, gl_hglrc) ) {
		wglErrors++;
	}
}

void GLimp_WakeRenderer(void *data) {
	smpData = data;

	if( !qwglMakeCurrent(gl_hdc, NULL) ) {
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent(renderCommandsEvent);

	WaitForSingleObject(renderActiveEvent, INFINITE);
}

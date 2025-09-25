/*
 * OSMesa BGL - Off-screen Mesa implementation for Haiku using BGLView
 * 
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * A single-header implementation of OSMesa API that provides off-screen OpenGL
 * rendering capabilities on Haiku operating system via native BGLView.
 * 
 * USAGE:
 *   #define OSMESA_BGL_IMPLEMENTATION
 *   #include "osmesa_bgl.h"
 * 
 * FEATURES:
 *   - Off-screen OpenGL rendering
 *   - Compatible with standard OSMesa API
 *   - Native implementation via BGLView
 *   - Thread-safe context management
 *   - Support for BGRA pixel format
 * 
 * EXAMPLE:
 *   OSMesaContext ctx = OSMesaCreateContext(OSMESA_BGRA, NULL);
 *   void* buffer = malloc(width * height * 4);
 *   OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, width, height);
 *   // ... OpenGL rendering calls ...
 *   OSMesaSwapBuffers(ctx);
 *   OSMesaDestroyContext(ctx);
 * 
 */

#ifndef OSMESA_BGL_H
#define OSMESA_BGL_H

#define GL_VERSION_4_6 1
#define GL_GLEXT_PROTOTYPES 1

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSMESA_MAJOR_VERSION 11
#define OSMESA_MINOR_VERSION 2  
#define OSMESA_PATCH_VERSION 0

#define OSMESA_RGBA		GL_RGBA
#define OSMESA_BGRA		0x1
#define OSMESA_ARGB		0x2

#define OSMESA_ROW_LENGTH	0x10
#define OSMESA_Y_UP		0x11

#define OSMESA_WIDTH		0x20
#define OSMESA_HEIGHT		0x21
#define OSMESA_FORMAT		0x22
#define OSMESA_TYPE		0x23
#define OSMESA_MAX_WIDTH	0x24
#define OSMESA_MAX_HEIGHT	0x25

#define OSMESA_DEPTH_BITS            0x30
#define OSMESA_STENCIL_BITS          0x31
#define OSMESA_ACCUM_BITS            0x32
#define OSMESA_PROFILE               0x33
#define OSMESA_CORE_PROFILE          0x34
#define OSMESA_COMPAT_PROFILE        0x35
#define OSMESA_CONTEXT_MAJOR_VERSION 0x36
#define OSMESA_CONTEXT_MINOR_VERSION 0x37

typedef struct osmesa_context *OSMesaContext;
typedef void (*OSMESAproc)();

GLAPI OSMesaContext GLAPIENTRY OSMesaCreateContext( GLenum format, OSMesaContext sharelist );
GLAPI OSMesaContext GLAPIENTRY OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist);
GLAPI OSMesaContext GLAPIENTRY OSMesaCreateContextAttribs( const int *attribList, OSMesaContext sharelist );
GLAPI void GLAPIENTRY OSMesaDestroyContext( OSMesaContext ctx );
GLAPI GLboolean GLAPIENTRY OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height );
GLAPI OSMesaContext GLAPIENTRY OSMesaGetCurrentContext( void );
GLAPI void GLAPIENTRY OSMesaPixelStore( GLint pname, GLint value );
GLAPI void GLAPIENTRY OSMesaGetIntegerv( GLint pname, GLint *value );
GLAPI GLboolean GLAPIENTRY OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
GLAPI GLboolean GLAPIENTRY OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer );
GLAPI OSMESAproc GLAPIENTRY OSMesaGetProcAddress( const char *funcName );
GLAPI void GLAPIENTRY OSMesaColorClamp(GLboolean enable);
GLAPI void GLAPIENTRY OSMesaPostprocess(OSMesaContext osmesa, const char *filter, unsigned enable_value);
GLAPI void GLAPIENTRY OSMesaSwapBuffers(OSMesaContext ctx);

#ifdef __cplusplus
}
#endif

#ifdef OSMESA_BGL_IMPLEMENTATION

#ifdef __cplusplus

#include <GLView.h>
#include <Autolock.h>
#include <OS.h>
#include <Screen.h>
#include <map>
#include <cstring>
#include <algorithm>

struct OSMesaContextRec {
	BGLView* glView;
	void* userBuffer;
	GLenum format;
	GLsizei width;
	GLsizei height;
	GLint depthBits;
	GLint stencilBits;
	GLint accumBits;
	GLboolean yUp;
	GLboolean contextLocked;
	GLsizei maxWidth;
	GLsizei maxHeight;
	
	OSMesaContextRec() {
		glView = NULL;
		userBuffer = NULL;
		format = OSMESA_BGRA;
		width = 0;
		height = 0;
		depthBits = 16;
		stencilBits = 0;
		accumBits = 0;
		yUp = GL_TRUE;
		contextLocked = GL_FALSE;
		
		BScreen screen(B_MAIN_SCREEN_ID);
		BRect frame = screen.Frame();
		maxWidth = (GLsizei)(frame.Width() + 1);
		maxHeight = (GLsizei)(frame.Height() + 1);
		
		if (maxWidth < 512) maxWidth = 512;
		if (maxHeight < 512) maxHeight = 512;
	}
						
	~OSMesaContextRec() {
		if (contextLocked && glView) {
			glView->UnlockGL();
			contextLocked = GL_FALSE;
		}
		delete glView;
		glView = NULL;
	}
};

static std::map<OSMesaContext, OSMesaContextRec*> g_contexts;
static OSMesaContext g_currentContext = NULL;
static BLocker g_contextLock("OSMesa Context Lock");

static ulong GetBGLOptions(GLint depthBits, GLint stencilBits, GLint accumBits, GLboolean shareContext) {
	ulong options = BGL_SINGLE | BGL_RGB | BGL_ALPHA;
	
	if (depthBits > 0) options |= BGL_DEPTH;
	if (stencilBits > 0) options |= BGL_STENCIL;
	if (accumBits > 0) options |= BGL_ACCUM;
	
	if (shareContext) {
		options |= BGL_SHARE_CONTEXT;
	}
	
	return options;
}

extern "C" {

#endif

GLAPI OSMesaContext GLAPIENTRY  
OSMesaCreateContextExt(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist) {
#ifdef __cplusplus
	if (format != OSMESA_BGRA) {
		return NULL;
	}
	
	BAutolock lock(g_contextLock);
	
	OSMesaContextRec* ctx = new OSMesaContextRec();
	ctx->format = format;
	ctx->depthBits = depthBits;
	ctx->stencilBits = stencilBits;  
	ctx->accumBits = accumBits;
	
	GLboolean hasShare = (sharelist != NULL) ? GL_TRUE : GL_FALSE;
	ulong options = GetBGLOptions(depthBits, stencilBits, accumBits, hasShare);
	
	ctx->glView = new BGLView(BRect(0, 0, ctx->maxWidth - 1, ctx->maxHeight - 1), 
							 "OSMesa", B_FOLLOW_NONE, B_WILL_DRAW, options);
	
	if (!ctx->glView) {
		delete ctx;
		return NULL;
	}
	
	OSMesaContext handle = (OSMesaContext)(ctx);
	g_contexts[handle] = ctx;
	
	return handle;
#else
	return NULL;
#endif
}

GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContext(GLenum format, OSMesaContext sharelist) {
	return OSMesaCreateContextExt(format, 16, 0, 0, sharelist);
}

GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContextAttribs(const int *attribList, OSMesaContext sharelist) {
	GLenum format = OSMESA_BGRA;
	GLint depthBits = 16;
	GLint stencilBits = 0;
	GLint accumBits = 0;
	
	if (attribList) {
		for (int i = 0; attribList[i] != 0; i += 2) {
			switch (attribList[i]) {
				case OSMESA_FORMAT:
					format = attribList[i + 1];
					break;
				case OSMESA_DEPTH_BITS:
					depthBits = attribList[i + 1];
					break;
				case OSMESA_STENCIL_BITS:
					stencilBits = attribList[i + 1];
					break;
				case OSMESA_ACCUM_BITS:
					accumBits = attribList[i + 1];
					break;
				case OSMESA_PROFILE:
				case OSMESA_CONTEXT_MAJOR_VERSION:
				case OSMESA_CONTEXT_MINOR_VERSION:
					break;
			}
		}
	}
	
	return OSMesaCreateContextExt(format, depthBits, stencilBits, accumBits, sharelist);
}

GLAPI void GLAPIENTRY
OSMesaDestroyContext(OSMesaContext ctx) {
#ifdef __cplusplus
	if (!ctx) return;
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(ctx);
	if (it != g_contexts.end()) {
		if (g_currentContext == ctx) {
			g_currentContext = NULL;
		}
		delete it->second;
		g_contexts.erase(it);
	}
#endif
}

GLAPI GLboolean GLAPIENTRY
OSMesaMakeCurrent(OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height) {
#ifdef __cplusplus
	if (!ctx || !buffer || width < 1 || height < 1 || type != GL_UNSIGNED_BYTE) {
		return GL_FALSE;
	}
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(ctx);
	if (it == g_contexts.end()) {
		return GL_FALSE;
	}
	
	OSMesaContextRec* context = it->second;
	
	if (width > context->maxWidth || height > context->maxHeight) {
		return GL_FALSE;
	}
	
	if (context->contextLocked && context->glView) {
		context->glView->UnlockGL();
		context->contextLocked = GL_FALSE;
	}
	
	context->userBuffer = buffer;
	context->width = width;
	context->height = height;
	
	if (!context->glView) {
		return GL_FALSE;
	}
	
	context->glView->LockGL();
	context->contextLocked = GL_TRUE;
	g_currentContext = ctx;
	
	glViewport(0, 0, width, height);
	
	return GL_TRUE;
#else
	return GL_FALSE;
#endif
}

GLAPI OSMesaContext GLAPIENTRY
OSMesaGetCurrentContext(void) {
#ifdef __cplusplus
	return g_currentContext;
#else
	return NULL;
#endif
}

GLAPI void GLAPIENTRY
OSMesaPixelStore(GLint pname, GLint value) {
#ifdef __cplusplus
	if (!g_currentContext) return;
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(g_currentContext);
	if (it != g_contexts.end()) {
		OSMesaContextRec* context = it->second;
		switch (pname) {
			case OSMESA_ROW_LENGTH:
				break;
			case OSMESA_Y_UP:
				context->yUp = value ? GL_TRUE : GL_FALSE;
				break;
		}
	}
#endif
}

GLAPI void GLAPIENTRY
OSMesaGetIntegerv(GLint pname, GLint *value) {
#ifdef __cplusplus
	if (!value || !g_currentContext) return;
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(g_currentContext);
	if (it != g_contexts.end()) {
		OSMesaContextRec* context = it->second;
		switch (pname) {
			case OSMESA_WIDTH:
				*value = context->width;
				break;
			case OSMESA_HEIGHT:
				*value = context->height;
				break;
			case OSMESA_FORMAT:
				*value = context->format;
				break;
			case OSMESA_TYPE:
				*value = GL_UNSIGNED_BYTE;
				break;
			case OSMESA_ROW_LENGTH:
				*value = 0;
				break;
			case OSMESA_Y_UP:
				*value = context->yUp;
				break;
			case OSMESA_MAX_WIDTH:
				*value = context->maxWidth;
				break;
			case OSMESA_MAX_HEIGHT:
				*value = context->maxHeight;
				break;
		}
	}
#endif
}

GLAPI GLboolean GLAPIENTRY
OSMesaGetDepthBuffer(OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer) {
	return GL_FALSE;
}

GLAPI GLboolean GLAPIENTRY
OSMesaGetColorBuffer(OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer) {
#ifdef __cplusplus
	if (!c) return GL_FALSE;
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(c);
	if (it != g_contexts.end()) {
		OSMesaContextRec* context = it->second;
		if (width) *width = context->width;
		if (height) *height = context->height;
		if (format) *format = context->format;
		if (buffer) *buffer = context->userBuffer;
		return GL_TRUE;
	}
#endif
	return GL_FALSE;
}

GLAPI OSMESAproc GLAPIENTRY
OSMesaGetProcAddress(const char *funcName) {
#ifdef __cplusplus
	if (!g_currentContext) return NULL;
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(g_currentContext);
	if (it != g_contexts.end() && it->second->glView) {
		return (OSMESAproc)it->second->glView->GetGLProcAddress(funcName);
	}
#endif
	return NULL;
}

GLAPI void GLAPIENTRY
OSMesaColorClamp(GLboolean enable) {
}

GLAPI void GLAPIENTRY
OSMesaPostprocess(OSMesaContext osmesa, const char *filter, unsigned enable_value) {
}

GLAPI void GLAPIENTRY
OSMesaSwapBuffers(OSMesaContext ctx) {
#ifdef __cplusplus
	if (!ctx) return;
	
	BAutolock lock(g_contextLock);
	
	std::map<OSMesaContext, OSMesaContextRec*>::iterator it = g_contexts.find(ctx);
	if (it == g_contexts.end()) return;
	
	OSMesaContextRec* context = it->second;
	
	if (!context->userBuffer || context->width <= 0 || context->height <= 0) {
		return;
	}
	
	if (g_currentContext != ctx || !context->contextLocked || !context->glView) {
		return;
	}
	
	glReadBuffer(GL_FRONT);
	
	glReadPixels(0, 0, context->width, context->height,
			   GL_BGRA, GL_UNSIGNED_BYTE, context->userBuffer);
	
	if (context->yUp) {
		int rowBytes = context->width * 4;
		unsigned char* temp = (unsigned char*)malloc(rowBytes);
		if (temp) {
			unsigned char* pixels = (unsigned char*)context->userBuffer;
			
			for (int y = 0; y < context->height / 2; y++) {
				unsigned char* row1 = pixels + y * rowBytes;
				unsigned char* row2 = pixels + (context->height - 1 - y) * rowBytes;
				memcpy(temp, row1, rowBytes);
				memcpy(row1, row2, rowBytes);
				memcpy(row2, temp, rowBytes);
			}
			free(temp);
		}
	}
#endif
}

#ifdef __cplusplus
}
#endif

#endif

#endif

#ifndef QGL_H
#define QGL_H
#ifdef _WIN32
#pragma once
#endif

// OpenGL entry points. The Dreamcast renders through Direct3D, so these resolve
// to direct stubs (see qgl.c) rather than the loaded function pointers a real GL
// build uses. They are kept for the 2D drawing paths that were written against GL.

#include <GL/gl.h>

#ifndef APIENTRY
#define APIENTRY
#endif

#ifdef __cplusplus
extern "C" {
#endif

void APIENTRY qglBegin( GLenum mode );
void APIENTRY qglEnd( void );
void APIENTRY qglVertex2f( GLfloat x, GLfloat y );
void APIENTRY qglTexCoord2f( GLfloat s, GLfloat t );
void APIENTRY qglColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void APIENTRY qglEnable( GLenum cap );
void APIENTRY qglDisable( GLenum cap );
void APIENTRY qglTexEnvf( GLenum target, GLenum pname, GLfloat param );

#ifdef __cplusplus
}
#endif

#endif // QGL_H

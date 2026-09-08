// qgl.c -- OpenGL entry points for the Dreamcast.
//
// The renderer is Direct3D, so the immediate-mode GL calls that some drawing
// paths were written against are placeholders here until those paths are ported.
#include "quakedef.h"
#include "qgl.h"

void APIENTRY qglBegin( GLenum mode )
{
	Sys_Error("NYI");
}

void APIENTRY qglBlendFunc( GLenum sfactor, GLenum dfactor )
{
	Sys_Error("NYI");
}

void APIENTRY qglColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
	Sys_Error("NYI");
}

void APIENTRY qglColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
	Sys_Error("NYI");
}

void APIENTRY qglDepthMask( GLboolean flag )
{
	Sys_Error("NYI");
}

void APIENTRY qglDisable( GLenum cap )
{
	Sys_Error("NYI");
}

void APIENTRY qglEnable( GLenum cap )
{
	Sys_Error("NYI");
}

void APIENTRY qglEnd( void )
{
	Sys_Error("NYI");
}

void APIENTRY qglShadeModel( GLenum mode )
{
	Sys_Error("NYI");
}

void APIENTRY qglTexCoord2f( GLfloat s, GLfloat t )
{
	Sys_Error("NYI");
}

void APIENTRY qglTexParameterf( GLenum target, GLenum pname, GLfloat param )
{
	Sys_Error("NYI");
}

void APIENTRY qglTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
	Sys_Error("NYI");
}

void APIENTRY qglVertex2f( GLfloat x, GLfloat y )
{
	Sys_Error("NYI");
}

void APIENTRY qglVertex3fv( const GLfloat* v )
{
	Sys_Error("NYI");
}

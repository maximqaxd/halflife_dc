// r_triangle.h
#if !defined( R_TRIANGLE_H )
#define R_TRIANGLE_H
#ifdef _WIN32
#pragma once
#endif

typedef enum
{
	TRI_FRONT = 0,
	TRI_NONE = 1,
} TRICULLSTYLE;


#define TRI_TRIANGLES		0
#define TRI_TRIANGLE_FAN	1
#define TRI_QUADS			2
#define TRI_POLYGON			3
#define TRI_LINES			4	
#define TRI_TRIANGLE_STRIP	5
#define TRI_QUAD_STRIP		6

// GL
void tri_GL_Color4f( float x, float y, float z, float w );
void tri_GL_Color4ub( byte r, byte g, byte b, byte a );
void tri_GL_Brightness( float x );
void tri_GL_RenderMode( int mode );
void tri_GL_CullFace( TRICULLSTYLE style );

// Software
void tri_Soft_Begin( int primitiveCode );
void tri_Soft_Color4f( float r, float g, float b, float a );
void tri_Soft_Color4ub( byte r, byte g, byte b, byte a );
void tri_Soft_TexCoord2f( float u, float v );
void tri_Soft_Vertex3f(float x, float y, float z);
void tri_Soft_Vertex3fv( float* worldPnt );
void tri_Soft_Brightness( float brightness );
void tri_Soft_RenderMode( int mode );
void tri_Soft_CullFace( int style );
void tri_Soft_End( void );

void R_TriangleSetTexture( byte* pTexture, short width, short height, unsigned short* pPalette );
int R_TriangleSpriteTexture( model_t* pSpriteModel, int frame );

#define tri_Begin(mode)				NULL
#define tri_Color4f(x, y, z, w)		tri_GL_Color4f(x, y, z, w)
#define tri_Color4ub(r, g, b, a)	tri_GL_Color4ub(r, g, b, a)
#define tri_TexCoord2f(u, v)		NULL
#define tri_Vertex3f(x, y, z)		NULL
#define tri_Vertex3fv(v)			NULL
#define tri_Brightness(brightness)	tri_GL_Brightness(brightness)
#define tri_RenderMode(mode)		tri_GL_RenderMode(mode)
#define tri_CullFace(style)			tri_GL_CullFace(style)
#define tri_End()					NULL

#endif // R_TRIANGLE_H
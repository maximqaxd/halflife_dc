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

// Dreamcast
void tri_DC_Color4f( float x, float y, float z, float w );
void tri_DC_Color4ub( byte r, byte g, byte b, byte a );
void tri_DC_Brightness( float x );
void tri_DC_RenderMode( int mode );
void tri_DC_CullFace( TRICULLSTYLE style );

int R_TriangleSpriteTexture( model_t* pSpriteModel, int frame );

#define tri_Begin(mode)				NULL
#define tri_Color4f(x, y, z, w)		tri_DC_Color4f(x, y, z, w)
#define tri_Color4ub(r, g, b, a)	tri_DC_Color4ub(r, g, b, a)
#define tri_TexCoord2f(u, v)		NULL
#define tri_Vertex3f(x, y, z)		NULL
#define tri_Vertex3fv(v)			NULL
#define tri_Brightness(brightness)	tri_DC_Brightness(brightness)
#define tri_RenderMode(mode)		tri_DC_RenderMode(mode)
#define tri_CullFace(style)			tri_DC_CullFace(style)
#define tri_End()					NULL

#endif // R_TRIANGLE_H

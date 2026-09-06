#include "quakedef.h"
#include "dc_accum.h"
#include "text_draw.h"

void R_ApplyViewModelProjection( float zn );

/*
=================
GLBeginHud

=================
*/
void GLBeginHud( void )
{
	DCV_SetHudDepth( 1.0f );
	Font_SetScale( 1.0f, 1.0f );
	DCV_SetPackedColor( 0xFFFFFFFF );
}

/*
=================
DrawWedge
=================
*/
void DrawWedge( float centerx, float centery, float angle1, float angle2, float radius )
{
#if 0
	qglBegin(GL_TRIANGLES);
	qglVertex2f(centerx, centery);
	qglVertex2f(centerx - cos(angle1 * (M_PI / 180.0)) * radius, centery - sin(angle1 * (M_PI / 180.0)) * radius);
	qglVertex2f(centerx - cos(angle2 * (M_PI / 180.0)) * radius, centery - sin(angle2 * (M_PI / 180.0)) * radius);
	qglEnd();
#endif
}

/*
=================
GLFinishHud
=================
*/
void GLFinishHud( void )
{
	R_ApplyViewModelProjection( 0.0f );
}

#include "quakedef.h"
#include "dc_accum.h"
#include "text_draw.h"
#include "qgl.h"

/*
=================
GLBeginHud

=================
*/
void GLBeginHud( void )
{
	DCV_SetHudDepth(1.0f);
	Font_SetScale(1.0f, 1.0f);
	DCV_SetPackedColor(0xFFFFFFFF);
}

/*
=================
DrawWedge
=================
*/
void DrawWedge( float centerx, float centery, float angle1, float angle2, float radius )
{
	angle1 *= (float)M_PI / 180.0f;
	angle2 *= (float)M_PI / 180.0f;
	qglBegin(GL_TRIANGLES);
	qglVertex2f(centerx, centery);
	qglVertex2f(centerx - sin(angle1) * radius, centery - cos(angle1) * radius);
	qglVertex2f(centerx - sin(angle2) * radius, centery - cos(angle2) * radius);
	qglEnd();
}

/*
=================
GLFinishHud
=================
*/
void GLFinishHud( void )
{
	R_ApplyViewModelProjection(0.0f);
}

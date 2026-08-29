#include "quakedef.h"
#include "dc_accum.h"

static D3DMATRIX s_savedProjMatrix;
static D3DMATRIX s_savedViewMatrix;
static D3DMATRIX s_savedWorldMatrix;
static D3DVIEWPORT2 s_savedViewport2;
static qboolean    s_savedViewport2Valid;

/*
=================
GLBeginHud

=================
*/
void GLBeginHud( void )
{
	LPDIRECT3DDEVICE3 dev;
	D3DMATRIX identity;
	float fw;
	float fh;

	dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();

	if (!dev || !dev->lpVtbl)
		return;

	DCV_FlushInline();

	dev->lpVtbl->GetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &s_savedProjMatrix);
	dev->lpVtbl->GetTransform(dev, D3DTRANSFORMSTATE_VIEW,       &s_savedViewMatrix);
	dev->lpVtbl->GetTransform(dev, D3DTRANSFORMSTATE_WORLD,      &s_savedWorldMatrix);

	memset(&identity, 0, sizeof(identity));
	identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;

	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_WORLD, &identity);
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_VIEW,  &identity);

	fw = (float)glwidth;
	fh = (float)glheight;

	if (fw <= 0.0f)
		fw = 640.0f;

	if (fh <= 0.0f)

		fh = 480.0f;
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &identity);
	DCV_Ortho(D3DTRANSFORMSTATE_PROJECTION, 0.0f, fw, fh, 0.0f, -99999.0f, 99999.0f, 1.0f);
	DCV_SetNoClip();

	DCV_SetColor(255, 255, 255, 255);
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
	LPDIRECT3DDEVICE3 dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();

	if (!dev || !dev->lpVtbl)
		return;

	DCV_FlushInline();

	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &s_savedProjMatrix);
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_VIEW,       &s_savedViewMatrix);
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_WORLD,      &s_savedWorldMatrix);

	DCV_SetColor(255, 255, 255, 255);
	DCV_SetClipRequired();
}

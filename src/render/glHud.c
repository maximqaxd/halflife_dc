#include "quakedef.h"
#include "dc_accum.h"

extern void* Sys_GetD3DDevice3( void );
extern void* Sys_GetD3DViewport( void );
extern int glx, gly, glwidth, glheight;

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

	DCV_Flush();

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
	DCV_MatrixMode(D3DTRANSFORMSTATE_PROJECTION);
	DCV_Ortho(0.0f, fw, fh, 0.0f, -99999.0f, 99999.0f);
	DCV_MatrixMode(D3DTRANSFORMSTATE_WORLD);
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

	DCV_Flush();

	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &s_savedProjMatrix);
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_VIEW,       &s_savedViewMatrix);
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_WORLD,      &s_savedWorldMatrix);
	DCV_MatrixMode(D3DTRANSFORMSTATE_WORLD);

	DCV_SetColor(255, 255, 255, 255);
	DCV_SetClipRequired();
}

/*
=========================================================
DCV_TexState Helpers
=========================================================
*/

void DCV_FlushApplyRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
	LPDIRECT3DDEVICE3 dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();

	if (!dev || !dev->lpVtbl)
		return;

	DCV_Flush();
	dev->lpVtbl->SetRenderState(dev, state, value);
}



/*
=================
DCV_TexState_Additive
=================
*/
void DCV_TexState_Additive( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_ONE);      
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE,        FALSE);
}

/*
=================
DCV_TexState_AlphaTest

=================
*/
void DCV_TexState_AlphaTest( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHAREF,  (DWORD)(gl_alphamin.value * 255.0f));
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

/*
=================
DCV_TexState_Blend
=================
*/
void DCV_TexState_Blend( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE,        FALSE);
}

/*
=================
DCV_TexState_VertColor
=================
*/
void DCV_TexState_VertColor( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
}

/*
=================
DCV_TexState_Modulate
=================
*/
void DCV_TexState_Modulate( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
}

/*
=================
DCV_TexState_Opaque
=================
*/
void DCV_TexState_Opaque( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA);
}

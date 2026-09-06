// d3dmath.c -- Direct3D transform helpers: matrix stacks, viewport and
// GL-style frustum/ortho/rotate/translate on top of the D3D device

#include "quakedef.h"
#include "winquake.h"
#include "dc_accum.h"


D3DMATRIX	g_matWorld;
D3DMATRIX	g_matView;
D3DMATRIX	g_matProjection;

D3DMATRIX	g_matWorldStack[32];
int			g_nWorldStackDepth;
D3DMATRIX	g_matProjectionStack[2];
int			g_nProjectionStackDepth;

/*
================
DCV_PushMatrix
================
*/
void DCV_PushMatrix( int state )
{
	D3DMATRIX current;

	g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, &current);

	if (state == D3DTRANSFORMSTATE_WORLD)
	{
		if (g_nWorldStackDepth == 32)
			Sys_Error("World matrix stack overflowed!\n");
		memcpy(&g_matWorldStack[g_nWorldStackDepth], &current, sizeof(D3DMATRIX));
		g_nWorldStackDepth++;
	}
	else
	{
		if (g_nProjectionStackDepth == 2)
			Sys_Error("Projection matrix stack overflowed!\n");
		memcpy(&g_matProjectionStack[g_nProjectionStackDepth], &current, sizeof(D3DMATRIX));
		g_nProjectionStackDepth++;
	}
}

/*
================
DCV_PopMatrix
================
*/
void DCV_PopMatrix( int state )
{
	if (g_nAccumVertCount)
		DCV_FlushInline();

	if (state == D3DTRANSFORMSTATE_WORLD)
	{
		if (g_nWorldStackDepth == 0)
			Sys_Error("World matrix stack underflowed!\n");
		g_nWorldStackDepth--;
		memcpy(&g_matWorld, &g_matWorldStack[g_nWorldStackDepth], sizeof(D3DMATRIX));
		g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, D3DTRANSFORMSTATE_WORLD, &g_matWorld);
	}
	else
	{
		if (g_nProjectionStackDepth == 0)
			Sys_Error("Projection matrix stack underflowed!\n");
		g_nProjectionStackDepth--;
		memcpy(&g_matProjection, &g_matProjectionStack[g_nProjectionStackDepth], sizeof(D3DMATRIX));
		g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, state, &g_matProjection);
	}
}

/*
================
DCV_SetViewport

Flush the pending triangle batch, then program the D3D viewport to a screen
rect. dwX/dwWidth/dwHeight are used directly; dwY is flipped to a bottom-left
origin. The clip volume is the fixed (-1,1)..(1,-1) 2x2 square.
================
*/
void DCV_SetViewport( int x, int y, int width, int height )
{
	if (g_nAccumVertCount)
		DCV_FlushInline();

	g_viewportDesc.dwSize = sizeof(D3DVIEWPORT2);
	g_pViewport->lpVtbl->GetViewport2(g_pViewport, &g_viewportDesc);
	g_viewportDesc.dwX          = x;
	g_viewportDesc.dwY          = 480 - (y + height);
	g_viewportDesc.dwWidth      = width;
	g_viewportDesc.dwHeight     = height;
	g_viewportDesc.dvClipX      = -1.0f;
	g_viewportDesc.dvClipY      = 1.0f;
	g_viewportDesc.dvClipWidth  = 2.0f;
	g_viewportDesc.dvClipHeight = 2.0f;
	g_pViewport->lpVtbl->SetViewport2(g_pViewport, &g_viewportDesc);
	g_pD3DDevice->lpVtbl->SetCurrentViewport(g_pD3DDevice, g_pViewport);
}

/*
================
DCV_SetProjectionDepthRange

Derive the viewport depth range from the projection near/far planes.
================
*/
void DCV_SetProjectionDepthRange( float znear, float zfar )
{
	if (g_nAccumVertCount)
		DCV_FlushInline();

	g_viewportDesc.dwSize = sizeof(D3DVIEWPORT2);
	g_pViewport->lpVtbl->GetViewport2(g_pViewport, &g_viewportDesc);
	g_viewportDesc.dvMinZ = -(znear + zfar) / (zfar - znear);
	g_viewportDesc.dvMaxZ = -((znear + zfar) - 2.0f) / (zfar - znear);
	g_pViewport->lpVtbl->SetViewport2(g_pViewport, &g_viewportDesc);
	g_pD3DDevice->lpVtbl->SetCurrentViewport(g_pD3DDevice, g_pViewport);
}

/*
================
DCV_SetViewportDepthRange
================
*/
void DCV_SetViewportDepthRange( float minz, float maxz )
{
	if (g_nAccumVertCount)
		DCV_FlushInline();

	g_viewportDesc.dwSize = sizeof(D3DVIEWPORT2);
	g_pViewport->lpVtbl->GetViewport2(g_pViewport, &g_viewportDesc);
	g_viewportDesc.dvMinZ = minz;
	g_viewportDesc.dvMaxZ = maxz;
	g_pViewport->lpVtbl->SetViewport2(g_pViewport, &g_viewportDesc);
	g_pD3DDevice->lpVtbl->SetCurrentViewport(g_pD3DDevice, g_pViewport);
}

/*
================
DCV_FlushApplyRenderState

Out-of-line render-state setter for callers outside the batch core.
================
*/
void DCV_FlushApplyRenderState( D3DRENDERSTATETYPE state, DWORD value )
{
	DWORD current;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, state, &current);
	if (current != value)
	{
		if (g_nAccumVertCount)
			DCV_FlushInline();
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, state, value);
	}
}

/*
================
DCV_SetTransform

Set a device transform and keep the world/view/projection shadow copies.
================
*/
void DCV_SetTransform( int state, const D3DMATRIX* matrix )
{
	if (g_nAccumVertCount)
		DCV_FlushInline();

	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, state, (LPD3DMATRIX)matrix);

	if (state == D3DTRANSFORMSTATE_WORLD)
		memcpy(&g_matWorld, matrix, sizeof(D3DMATRIX));
	else if (state == D3DTRANSFORMSTATE_VIEW)
		memcpy(&g_matView, matrix, sizeof(D3DMATRIX));
	else
		memcpy(&g_matProjection, matrix, sizeof(D3DMATRIX));
}

void DCV_GetTransform( int state, D3DMATRIX* matrix )
{
	g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, matrix);
}

/*
================
DCV_Frustum

Build a GL-style perspective frustum into the world or projection shadow
matrix, scale it, and set it on the device.
================
*/
void DCV_Frustum( int state, float lf, float rt, float bt, float tp, float zn, float zf, float scale )
{
	float *m;

	if (g_nAccumVertCount)
		DCV_FlushInline();

	if (state == D3DTRANSFORMSTATE_PROJECTION)
		m = (float *)&g_matProjection;
	else
		m = (float *)&g_matWorld;

	m[0]  = (zn + zn) / (rt - lf);
	m[4]  = 0.0f;
	m[8]  = (lf + rt) / (rt - lf);
	m[12] = 0.0f;
	m[1]  = 0.0f;
	m[5]  = (zn + zn) / (tp - bt);
	m[9]  = (bt + tp) / (tp - bt);
	m[13] = 0.0f;
	m[2]  = 0.0f;
	m[6]  = 0.0f;
	m[10] = -(zn + zf) / (zf - zn);
	m[14] = (zf * -2.0f * zn) / (zf - zn);
	m[3]  = 0.0f;
	m[7]  = 0.0f;
	m[11] = -1.0f;
	m[15] = 0.0f;

	m[0]  = m[0] * scale;
	m[8]  = m[8] * scale;
	m[5]  = m[5] * scale;
	m[9]  = m[9] * scale;
	m[10] = m[10] * scale;
	m[14] = m[14] * scale;
	m[11] = m[11] * scale;

	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, state, (LPD3DMATRIX)m);
}

/*
================
DCV_Ortho
================
*/
void DCV_Ortho( int state, float lf, float rt, float bt, float tp, float zn, float zf, float scale )
{
	float *m;

	if (g_nAccumVertCount)
		DCV_FlushInline();

	if (state == D3DTRANSFORMSTATE_PROJECTION)
		m = (float *)&g_matProjection;
	else
		m = (float *)&g_matWorld;

	m[0]  = 2.0f / (rt - lf);
	m[4]  = 0.0f;
	m[8]  = 0.0f;
	m[12] = -(lf + rt) / (rt - lf);
	m[1]  = 0.0f;
	m[5]  = 2.0f / (tp - bt);
	m[9]  = 0.0f;
	m[13] = -(bt + tp) / (tp - bt);
	m[2]  = 0.0f;
	m[6]  = 0.0f;
	m[10] = -2.0f / (zf - zn);
	m[14] = -(zn + zf) / (zf - zn);
	m[3]  = 0.0f;
	m[7]  = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;

	m[0]  = m[0] * scale;
	m[12] = m[12] * scale;
	m[5]  = m[5] * scale;
	m[13] = m[13] * scale;
	m[10] = m[10] * scale;
	m[14] = m[14] * scale;
	m[15] = m[15] * scale;

	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, state, (LPD3DMATRIX)m);
}

/*
================
DCV_Translate

Multiply a translation into the current transform and read the result back
into the shadow matrix.
================
*/
void DCV_Translate( int state, float x, float y, float z )
{
	float *m;

	if (g_nAccumVertCount)
		DCV_FlushInline();

	if (state == D3DTRANSFORMSTATE_PROJECTION)
		m = (float *)&g_matProjection;
	else if (state == D3DTRANSFORMSTATE_VIEW)
		m = (float *)&g_matView;
	else
		m = (float *)&g_matWorld;

	m[0]  = 1.0f;
	m[4]  = 0.0f;
	m[8]  = 0.0f;
	m[12] = x;
	m[1]  = 0.0f;
	m[5]  = 1.0f;
	m[9]  = 0.0f;
	m[13] = y;
	m[2]  = 0.0f;
	m[6]  = 0.0f;
	m[10] = 1.0f;
	m[14] = z;
	m[3]  = 0.0f;
	m[7]  = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;

	g_pD3DDevice->lpVtbl->MultiplyTransform(g_pD3DDevice, state, (LPD3DMATRIX)m);
	g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, (LPD3DMATRIX)m);
}

/*
================
DCV_Rotate

Multiply an axis rotation into the current transform; only the three
cardinal axes are supported.
================
*/
void DCV_Rotate( int state, float angle, float x, float y, float z )
{
	D3DMATRIX rot;
	float     *m = (float *)&rot;
	float     rad, c, s;

	if (g_nAccumVertCount)
		DCV_FlushInline();

	rad = angle * 3.141592654f / 180.0f;
	c = cos(rad);
	s = sin(rad);

	m[5]  = c;
	m[10] = c;
	if (x == 1.0f)
	{
		m[9]  = -s;
		m[0]  = 1.0f;
		m[4]  = 0.0f;
		m[8]  = 0.0f;
		m[1]  = 0.0f;
		m[2]  = 0.0f;
		m[6]  = s;
	}
	else
	{
		m[0] = c;
		if (y == 1.0f)
		{
			m[4]  = 0.0f;
			m[2]  = -s;
			m[1]  = 0.0f;
			m[5]  = 1.0f;
			m[9]  = 0.0f;
			m[6]  = 0.0f;
			m[8]  = s;
		}
		else if (z == 1.0f)
		{
			m[4]  = -s;
			m[8]  = 0.0f;
			m[9]  = 0.0f;
			m[2]  = 0.0f;
			m[6]  = 0.0f;
			m[10] = 1.0f;
			m[1]  = s;
		}
		else
		{
			return;
		}
	}
	m[15] = 1.0f;
	m[14] = 0.0f;
	m[13] = 0.0f;
	m[12] = 0.0f;
	m[11] = 0.0f;
	m[7]  = 0.0f;
	m[3]  = 0.0f;

	g_pD3DDevice->lpVtbl->MultiplyTransform(g_pD3DDevice, state, &rot);
	if (state == D3DTRANSFORMSTATE_PROJECTION)
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, (LPD3DMATRIX)&g_matProjection);
	else if (state == D3DTRANSFORMSTATE_VIEW)
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, (LPD3DMATRIX)&g_matView);
	else
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice, state, (LPD3DMATRIX)&g_matWorld);
}

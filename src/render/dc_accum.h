/*
 * dc_accum.h – AccumVerts/AccumIndex batch
 *
 *
 */
#ifndef DC_ACCUM_H
#define DC_ACCUM_H

#include "glquake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUAD_TABLE_MAX_VERTS  50
#define FLUSH_THRESHOLD       224

void          DCV_AccumInit( void );
void          DCV_SetPackedColor( DWORD diffuse );
DWORD         DCV_GetCurrentDiffuse( void );
void          DCV_SetColorFloat( float r, float g, float b, float a );
qboolean      DCV_EnsureSpace( int add_verts, int add_indices );
int           DCV_GetVertCount( void );
void          DCV_AddLVertex( const D3DLVERTEX* v );
void          DCV_AddPolyIndices( int base, int numverts );
void          DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 );
void          DCV_AddIndicesStrip( int base, int count );
void          DCV_AddIndicesFan( int base, int count );
void          DCV_SetClipRequired( void );
void          DCV_SetNoClip( void );
void          DCV_SetTexStateFromRenderMode( int rendermode );

void          DCV_FlushApplyRenderState( D3DRENDERSTATETYPE state, DWORD value );

/*
 * The PowerVR is a deferred tile renderer: a render-state change also affects the
 * triangles already queued in the current tile, so the pending batch has to be
 * flushed before the state can change. These setters do the flush; they are inline
 * because they are called from a lot of places. The batch itself lives in
 * dc_accum.c and the device in dc_d3d.c.
 */
extern int               g_nAccumVertCount;
extern int               g_nAccumIndexCount;
extern int               g_nAccumMaxVertsSeen;
extern int               g_nAccumMaxIndicesSeen;
extern D3DLVERTEX       *g_pAccumVerts;
extern WORD             *g_pAccumIndex;
extern DWORD             g_dwAccumFlushFlags;
extern DWORD             g_dwAccumCurrentDiffuse;
extern LPDIRECT3DDEVICE3 g_pD3DDevice;

static __inline void DCV_Flush( void )
{
	if (g_nAccumVertCount != 0)
	{
		if (g_nAccumMaxVertsSeen < g_nAccumVertCount)
			g_nAccumMaxVertsSeen = g_nAccumVertCount;
		if (g_nAccumMaxIndicesSeen < g_nAccumIndexCount)
			g_nAccumMaxIndicesSeen = g_nAccumIndexCount;
		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice,
			D3DPT_TRIANGLELIST, D3DFVF_LVERTEX,
			g_pAccumVerts, g_nAccumVertCount,
			g_pAccumIndex, g_nAccumIndexCount,
			g_dwAccumFlushFlags | D3DDP_DONOTLIGHT);
		g_nAccumVertCount = 0;
		g_nAccumIndexCount = 0;
	}
}

static __inline void DCV_FlushIfLarge( void )
{
	if (g_nAccumVertCount > FLUSH_THRESHOLD)
		DCV_Flush();
}

static __inline void DCV_SetRenderState( D3DRENDERSTATETYPE state, DWORD value )
{
	DWORD current;
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, state, &current);
	if (current != value)
	{
		if (g_nAccumVertCount)
			DCV_Flush();
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, state, value);
	}
}

static __inline void DCV_SetTextureStageState( DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value )
{
	DWORD current;
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, stage, type, &current);
	if (current != value)
	{
		if (g_nAccumVertCount)
			DCV_Flush();
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, stage, type, value);
	}
}

static __inline void DCV_SetColor( int r, int g, int b, int a )
{
	g_dwAccumCurrentDiffuse = (a << 24) | (r << 16) | (g << 8) | b;
}

static __inline int DCV_AddVertex( float x, float y, float z, float tu, float tv )
{
	D3DLVERTEX *pVert = &g_pAccumVerts[g_nAccumVertCount];

	pVert->x = x;
	pVert->y = y;
	pVert->z = z;
	pVert->color = g_dwAccumCurrentDiffuse;
	pVert->tu = tu;
	pVert->tv = tv;
	return g_nAccumVertCount++;
}

#ifdef __cplusplus
}
#endif

#endif /* DC_ACCUM_H */

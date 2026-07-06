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
void          DCV_SetColor( int r, int g, int b, int a );
void          DCV_SetPackedColor( DWORD diffuse );
DWORD         DCV_GetCurrentDiffuse( void );
void          DCV_SetColorFloat( float r, float g, float b, float a );
void          DCV_FlushIfLarge( void );
qboolean      DCV_EnsureSpace( int add_verts, int add_indices );
int           DCV_GetVertCount( void );
int           DCV_AddVertex( float x, float y, float z, float tu, float tv );
void          DCV_AddLVertex( const D3DLVERTEX* v );
void          DCV_AddPolyIndices( int base, int numverts );
void          DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 );
void          DCV_AddIndicesStrip( int base, int count );
void          DCV_AddIndicesFan( int base, int count );
void          DCV_SetClipRequired( void );
void          DCV_SetNoClip( void );
void          DCV_SetTexStateFromRenderMode( int rendermode );
void          DCV_SetDefaultRenderStates( void );
void          DCV_SetTextRenderStates( void );

void          DCV_FlushApplyRenderState( D3DRENDERSTATETYPE state, DWORD value );

/* d3dmath.c -- matrix stacks, viewport, GL-style transforms on the D3D device */
void          DCV_PushMatrix( int state );
void          DCV_PopMatrix( int state );
void          DCV_SetViewport( int x, int y, int width, int height );
void          DCV_SetProjectionDepthRange( float znear, float zfar );
void          DCV_SetViewportDepthRange( float minz, float maxz );
void          DCV_SetTransform( int state, const D3DMATRIX* matrix );
void          DCV_GetTransform( int state, D3DMATRIX* matrix );
void          DCV_Frustum( float lf, float rt, float bt, float tp, float zn, float zf, float scale, int state );
void          DCV_Ortho( float lf, float rt, float bt, float tp, float zn, float zf, float scale, int state );
void          DCV_Translate( float x, float y, float z, int state );
void          DCV_Rotate( float angle, float x, float y, float z, int state );

extern D3DMATRIX g_matWorld;
extern D3DMATRIX g_matView;
extern D3DMATRIX g_matProjection;

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
extern LPDIRECT3DDEVICE3   g_pD3DDevice;
extern LPDIRECT3DVIEWPORT3 g_pViewport;
extern D3DVIEWPORT2        g_viewportDesc;

/* The batch flush exists in two forms in the binary. DCV_Flush is a real
   out-of-line function (dc_accum.c, 0x128428) that direct callers such as
   GL_EndRendering invoke. The state-change setters below instead carry an inlined
   copy (DCV_FlushInline) so they fold into the render path exactly as the binary
   does (e.g. inside DCV_Flip), rather than emitting an out-of-line call. */
void DCV_Flush( void );

static __inline void DCV_FlushInline( void )
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

static __inline void DCV_SetRenderState( D3DRENDERSTATETYPE state, DWORD value )
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

static __inline void DCV_SetTextureStageState( DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value )
{
	DWORD current;
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, stage, type, &current);
	if (current != value)
	{
		if (g_nAccumVertCount)
			DCV_FlushInline();
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, stage, type, value);
	}
}

#ifdef __cplusplus
}
#endif

#endif /* DC_ACCUM_H */

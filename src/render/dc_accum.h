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
#define MAX_D3D_LIGHTS        4

void          DCV_AccumInit( void );
void          DCV_SetColor( int r, int g, int b, int a );
void          DCV_SetPackedColor( DWORD diffuse );
DWORD         DCV_GetCurrentDiffuse( void );
void          DCV_SetColorFloat( float r, float g, float b, float a );
void          DCV_FlushIfLarge( void );
void          DCV_SubmitBatchCopy( void );
void          DCV_SubmitBatchGuarded( void );
void          DCV_SetupStudioLighting( const float (*boneMatrices)[4][4], int count );
int           DCV_GetVertCount( void );
int           DCV_AddVertex( float x, float y, float z, float tu, float tv );
void          DCV_AddLVertex( const D3DLVERTEX* v );
void          DCV_PushVertexLit( const vec_t *pos, float tu, float tv );
void          DCV_AddVertexIndexed( float x, float y, float z, float tu, float tv );
void          DCV_AddStudioMesh( int count, const short *pCmds, const byte *pVertices, const byte *pNormals );
void          DCV_AddStudioMeshChrome( int count, const short *pCmds, const byte *pVertices, const byte *pNormals );
void          DCV_AddStudioMeshTagged( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const byte *pVertTag );
void          DCV_AddStudioMeshChromeTagged( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const byte *pVertTag );
void          DCV_AddPolyIndices( int base, int numverts );
void          DCV_AddIndicesFan( int base, int numverts );
void          DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 );
void          DCV_AddIndicesStrip( int base, int count );
void          DCV_AddIndicesFan( int base, int count );
void          DCV_BuildStudioIndexList( const short *pCmds );
void          DCV_AddIndicesStripRestart( int base, int count );
void          DCV_AddIndicesFanRestart( short base, int count );
void          DCV_AssembleStudioIndexListRestart( const short *pCmds );
void          DCV_AccumSolidPoly( const void *poly );
void          DCV_AccumColoredPoly( const void *poly );
void          DCV_AccumLightmapBatch( const void *poly );
void          DCV_AccumScrollPoly( const void *poly );
void          DCV_SetTextureClamp( void );
void          DCV_SetTextureWrap( void );
extern float  g_flScrollOffset;
extern float  g_flStudioTexScaleS;
extern float  g_flStudioTexScaleT;
void          DCV_SetClipRequired( void );
void          DCV_SetNoClip( void );
void          DCV_SetTexStateFromRenderMode( int rendermode );
void          DCV_SetDlight( int index, float *origin, float *color, float radius );
void          DCV_DisableDlight( int index );
void          DCV_SetWorldLight( float *dir, float *color, float *ambient );

void          DCV_FlushApplyRenderState( D3DRENDERSTATETYPE state, DWORD value );

/* d3dmath.c -- matrix stacks, viewport, GL-style transforms on the D3D device */
void          DCV_PushMatrix( int state );
void          DCV_PopMatrix( int state );
void          DCV_SetViewport( int x, int y, int width, int height );
void          DCV_SetProjectionDepthRange( float znear, float zfar );
void          DCV_SetViewportDepthRange( float minz, float maxz );
void          DCV_SetTransform( int state, const D3DMATRIX* matrix );
void          DCV_GetTransform( int state, D3DMATRIX* matrix );
void          DCV_Frustum( int state, float lf, float rt, float bt, float tp, float zn, float zf, float scale );
void          DCV_Ortho( int state, float lf, float rt, float bt, float tp, float zn, float zf, float scale );
void          DCV_Translate( int state, float x, float y, float z );
void          DCV_Rotate( int state, float angle, float x, float y, float z );

extern D3DMATRIX g_matWorld;
extern D3DMATRIX g_matView;
extern D3DMATRIX g_matProjection;
extern D3DLIGHT2 g_lightData[MAX_D3D_LIGHTS];

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
extern D3DMATERIAL         g_backgroundMaterialData;

/* Draw the accumulated vertex batch. DCV_Flush is the shared out-of-line entry
   point for direct callers such as GL_EndRendering; the render-state setters below
   use the inline DCV_FlushInline so any pending batch is drawn before a state
   change takes effect. */
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

/* 2D screen-space quad drawing (built on the qgl entry points); not yet ported. */
void DCV_Begin2D( int mode, int flags );
void DCV_2D_SetupStates( void );

void DCV_TexState_Additive( void );
void DCV_SetHudDepth( float depth );
void DCV_ScreenFade( int r, int g, int b, int a, int layer, qboolean modulate );
void DCV_SetFog( int enable, int r, int g, int b, int amount );
void DCV_UpdateTextureFiltering( void );
void DCV_DisableMultitexture( void );

#ifdef __cplusplus
}
#endif

#endif /* DC_ACCUM_H */

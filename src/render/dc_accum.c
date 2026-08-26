/*
 * dc_accum.c - AccumVerts / AccumIndex batch core.
 */

#include "quakedef.h"
#include "dc_accum.h"

extern void DCV_SetupFog( void );

#define QUAD_TABLE_ROWS     0x32
#define ROW_STRIDE_SHORTS   6

#define ACCUM_VERTS_SIZE    65536
#define ACCUM_INDEX_SIZE    16384
#define MULTI_MTX_0_SIZE    5120
#define MULTI_MTX_1_SIZE    2048

static short    *g_pQuadTable[QUAD_TABLE_ROWS + 1];
static short     g_QuadIndexData[7500 / sizeof(short)];

// shared with the inline render-state helpers in dc_accum.h
D3DLVERTEX      *g_pAccumVerts;
WORD            *g_pAccumIndex;
int              g_nAccumVertCount;
int              g_nAccumIndexCount;
DWORD            g_dwAccumFlushFlags = D3DDP_DONOTUPDATEEXTENTS;
int              g_nAccumMaxVertsSeen;
int              g_nAccumMaxIndicesSeen;

static void     *g_pMultiMtx0;
static void     *g_pMultiMtx1;
DWORD            g_dwAccumCurrentDiffuse = 0xFFFFFFFFu;

static int DCV_GetMaxVertCount( void )
{
	return ACCUM_VERTS_SIZE / (int)sizeof(D3DLVERTEX);
}

static int DCV_GetMaxIndexCount( void )
{
	return ACCUM_INDEX_SIZE / (int)sizeof(WORD);
}

static void DCV_ClearBatch( void )
{
	g_nAccumVertCount = 0;
	g_nAccumIndexCount = 0;
}

void DCV_AccumInit( void )
{
	int row;
	int row_stride;
	short *row_start;

	row_start = g_QuadIndexData;
	row_stride = 0;
	for (row = 0; row < QUAD_TABLE_ROWS; ++row)
	{
		short *out = row_start;
		short base = 0;
		int high = row;
		int k;

		g_pQuadTable[row] = row_start;
		for (k = 0; k < row; k += 2)
		{
			high--;
			*out++ = (short)high;
			*out++ = base;
			*out++ = (short)(high - 1);
			*out++ = base;
			*out++ = base + 1;
			*out++ = (short)(high - 1);
			base++;
		}

		row_start = (short *)((byte *)row_start + row_stride);
		row_stride += ROW_STRIDE_SHORTS;
	}
	g_pQuadTable[QUAD_TABLE_ROWS] = NULL;

	g_pAccumVerts = (D3DLVERTEX *)MnemoAlloc(ACCUM_VERTS_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumVerts");
	g_pAccumIndex = (WORD *)MnemoAlloc(ACCUM_INDEX_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumIndex");

	/* Reserve for multi matrixes for future use in r_studio.c */
	g_pMultiMtx0 = MnemoAlloc(MULTI_MTX_0_SIZE, MNEMO_FLAG_MALLOC, 0, "MultiMtx");
	g_pMultiMtx1 = MnemoAlloc(MULTI_MTX_1_SIZE, MNEMO_FLAG_MALLOC, 0, "MultiMtx");
}

void DCV_Flush( void )
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

/* Flush without updating the max-seen high-water marks. */
void DCV_FlushBatchCopy( void )
{
	if (g_nAccumVertCount != 0)
	{
		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice,
			D3DPT_TRIANGLELIST, D3DFVF_LVERTEX,
			g_pAccumVerts, g_nAccumVertCount,
			g_pAccumIndex, g_nAccumIndexCount,
			g_dwAccumFlushFlags);
		g_nAccumVertCount = 0;
		g_nAccumIndexCount = 0;
	}
}

/* Flush a single synthesized guard vertex/index pair, ignoring the rest of
   the batch. Payload source globals not yet identified. */
static DWORD s_dcGuardVertLo[2];
static void *s_pDcGuardVertHi;

void DCV_FlushBatchGuarded( void )
{
	if (g_nAccumVertCount != 0 && g_nAccumIndexCount != 0)
	{
		DWORD guardVert[4];

		guardVert[1] = s_dcGuardVertLo[0];
		guardVert[2] = s_dcGuardVertLo[1];
		guardVert[3] = (DWORD)(size_t)s_pDcGuardVertHi;
		guardVert[0] = g_dwAccumCurrentDiffuse;

		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice,
			D3DPT_TRIANGLELIST, D3DFVF_LVERTEX,
			guardVert, g_nAccumVertCount,
			g_pAccumIndex, g_nAccumIndexCount,
			D3DDP_DONOTCLIP);
		g_nAccumVertCount = 0;
		g_nAccumIndexCount = 0;
	}
}

void DCV_SetColor( int r, int g, int b, int a )
{
	g_dwAccumCurrentDiffuse = (a << 24) | (r << 16) | (g << 8) | b;
}

void DCV_SetPackedColor( DWORD diffuse )
{
	g_dwAccumCurrentDiffuse = diffuse;
}

DWORD DCV_GetCurrentDiffuse( void )
{
	return g_dwAccumCurrentDiffuse;
}

void DCV_SetColorFloat( float r, float g, float b, float a )
{
	g_dwAccumCurrentDiffuse = (int)(a * 255.0f) << 24 | (int)(r * 255.0f) << 16 |
	                          (int)(g * 255.0f) << 8  | (int)(b * 255.0f);
}

void DCV_FlushIfLarge( void )
{
	if (g_nAccumVertCount > FLUSH_THRESHOLD)
		DCV_FlushInline();
}

/* ---------------------------------------------------------------------------
 * Texture-stage / blend presets. Each sets stage 0 to modulate the texture by
 * the vertex color, then selects a blend mode.
 * --------------------------------------------------------------------------- */

/* Additive: texture * color, added to the frame buffer (dst = ONE). */
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

/* Standard alpha blending, for glyph quads and translucent pics. */
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

/* Modulate: texture * frame buffer (dst = SRCCOLOR, src = ZERO). */
void DCV_TexState_Modulate( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_ZERO);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_SRCCOLOR);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE,        FALSE);
}

/* Opaque: no blending; also re-applies the current fog settings. */
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

	DCV_SetupFog();
}

/* Vertex color drawn directly (COLOROP selects the diffuse arg), alpha blended. */
void DCV_TexState_VertColor( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
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

/* Turn stage 1 back off after a lightmapped multitexture draw. */
void DCV_DisableMultitexture( void )
{
	DCV_SetTextureStageState(1, D3DTSS_COLOROP,       D3DTOP_DISABLE);
	DCV_SetTextureStageState(1, D3DTSS_ALPHAOP,       D3DTOP_DISABLE);
	DCV_SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	DCV_SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
}

void DCV_SetClipRequired( void )
{
	g_dwAccumFlushFlags = D3DDP_DONOTUPDATEEXTENTS;
}

void DCV_SetNoClip( void )
{
	g_dwAccumFlushFlags = D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP;
}

qboolean DCV_EnsureSpace( int add_verts, int add_indices )
{
	if (add_verts < 0 || add_indices < 0)
		return FALSE;

	if (add_verts > DCV_GetMaxVertCount() || add_indices > DCV_GetMaxIndexCount())
		return FALSE;

	if (g_nAccumVertCount + add_verts > DCV_GetMaxVertCount() ||
		g_nAccumIndexCount + add_indices > DCV_GetMaxIndexCount())
	{
		DCV_FlushInline();
	}

	return TRUE;
}

int DCV_GetVertCount( void )
{
	return g_nAccumVertCount;
}

/* The four DCV_Accum*Poly variants below all take a glpoly_t-shaped source
   (next, chain, numverts, verts[][8] -- one field narrower than this fork's
   own glpoly_t, which also carries a flags field before verts; that gap is
   not yet reconciled, so the vertex array is reached by raw offset here
   rather than through the glpoly_t type). Each copies numverts vertices into
   the batch using the quad table for the triangle indices, then flushes if
   the batch has grown past FLUSH_THRESHOLD. */
#define DC_POLY_NUMVERTS( poly )  (*(const short *)((const byte *)(poly) + 8))
#define DC_POLY_VERTS( poly )     ((const float *)((const byte *)(poly) + 12))

/* Solid textured poly: current diffuse, base texture UV (verts[][4],[5]). */
void DCV_AccumSolidPoly( const void *poly )
{
	int numverts = DC_POLY_NUMVERTS(poly);
	const float *pVert = DC_POLY_VERTS(poly);
	const short *pSrc = g_pQuadTable[numverts];
	int needed = (numverts - 2) * 3;
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	D3DLVERTEX *pOut = &g_pAccumVerts[g_nAccumVertCount];
	int i;

	for (i = needed; i != 0; --i)
		*pIdx++ = (short)g_nAccumVertCount + *pSrc++;

	for (i = numverts; i != 0; --i)
	{
		pOut->x = pVert[0];
		pOut->y = pVert[1];
		pOut->z = pVert[2];
		pOut->color = g_dwAccumCurrentDiffuse;
		pOut->tu = pVert[4];
		pOut->tv = pVert[5];
		pVert += 8;
		++pOut;
	}

	g_nAccumVertCount += numverts;
	g_nAccumIndexCount += needed;

	if (g_nAccumVertCount > FLUSH_THRESHOLD && g_nAccumVertCount != 0)
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

/* Per-vertex packed color (verts[][3]) instead of the current diffuse. */
void DCV_AccumColoredPoly( const void *poly )
{
	int numverts = DC_POLY_NUMVERTS(poly);
	const float *pVert = DC_POLY_VERTS(poly);
	const short *pSrc = g_pQuadTable[numverts];
	int needed = (numverts - 2) * 3;
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	D3DLVERTEX *pOut = &g_pAccumVerts[g_nAccumVertCount];
	int i;

	for (i = needed; i != 0; --i)
		*pIdx++ = (short)g_nAccumVertCount + *pSrc++;

	for (i = numverts; i != 0; --i)
	{
		pOut->x = pVert[0];
		pOut->y = pVert[1];
		pOut->z = pVert[2];
		pOut->color = *(const DWORD *)(pVert + 3);
		pOut->tu = pVert[4];
		pOut->tv = pVert[5];
		pVert += 8;
		++pOut;
	}

	g_nAccumVertCount += numverts;
	g_nAccumIndexCount += needed;

	if (g_nAccumVertCount > FLUSH_THRESHOLD && g_nAccumVertCount != 0)
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

/* Lightmap pass: current diffuse, lightmap UV (verts[][6],[7]). */
void DCV_AccumLightmapBatch( const void *poly )
{
	int numverts = DC_POLY_NUMVERTS(poly);
	const float *pVert = DC_POLY_VERTS(poly);
	const short *pSrc = g_pQuadTable[numverts];
	int needed = (numverts - 2) * 3;
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	D3DLVERTEX *pOut = &g_pAccumVerts[g_nAccumVertCount];
	int i;

	for (i = needed; i != 0; --i)
		*pIdx++ = (short)g_nAccumVertCount + *pSrc++;

	for (i = numverts; i != 0; --i)
	{
		pOut->x = pVert[0];
		pOut->y = pVert[1];
		pOut->z = pVert[2];
		pOut->color = g_dwAccumCurrentDiffuse;
		pOut->tu = pVert[6];
		pOut->tv = pVert[7];
		pVert += 8;
		++pOut;
	}

	g_nAccumVertCount += numverts;
	g_nAccumIndexCount += needed;

	if (g_nAccumVertCount > FLUSH_THRESHOLD && g_nAccumVertCount != 0)
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

/* Scrolling texture: current diffuse, base UV with a running U offset. */
/* The U scroll offset is not a parameter -- the binary reads it from a
   global (a pooled float load, not the FR5 register a second param would
   use), set by a not-yet-reconstructed caller. Real identity unconfirmed. */
static float s_dcScrollOffset;

void DCV_AccumScrollPoly( const void *poly )
{
	float s_offset = s_dcScrollOffset;
	int numverts = DC_POLY_NUMVERTS(poly);
	const float *pVert = DC_POLY_VERTS(poly);
	const short *pSrc = g_pQuadTable[numverts];
	int needed = (numverts - 2) * 3;
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	D3DLVERTEX *pOut = &g_pAccumVerts[g_nAccumVertCount];
	int i;

	for (i = needed; i != 0; --i)
		*pIdx++ = (short)g_nAccumVertCount + *pSrc++;

	for (i = numverts; i != 0; --i)
	{
		pOut->x = pVert[0];
		pOut->y = pVert[1];
		pOut->z = pVert[2];
		pOut->color = g_dwAccumCurrentDiffuse;
		pOut->tu = pVert[4] + s_offset;
		pOut->tv = pVert[5];
		pVert += 8;
		++pOut;
	}

	g_nAccumVertCount += numverts;
	g_nAccumIndexCount += needed;

	if (g_nAccumVertCount > FLUSH_THRESHOLD && g_nAccumVertCount != 0)
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

void DCV_AddLVertex( const D3DLVERTEX* v )
{
	if (!v || !DCV_EnsureSpace(1, 0))
		return;

	g_pAccumVerts[g_nAccumVertCount++] = *v;
}

int DCV_AddVertex( float x, float y, float z, float tu, float tv )
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

/* Same as DCV_AddVertex, but the position comes from a vec3_t and there's
   no return value -- used by callers that already have a packed vector. */
void DCV_AddVertexLit( float tu, float tv, const vec_t *pos )
{
	D3DLVERTEX *pVert = &g_pAccumVerts[g_nAccumVertCount];

	pVert->x = pos[0];
	pVert->y = pos[1];
	pVert->z = pos[2];
	pVert->color = g_dwAccumCurrentDiffuse;
	pVert->tu = tu;
	pVert->tv = tv;
	g_nAccumVertCount++;
}

/* Adds a vertex AND its own index in one call (point-per-vertex indexing),
   for batches that don't share vertices between primitives. */
void DCV_AddVertexIndexed( float x, float y, float z, float tu, float tv )
{
	D3DLVERTEX *pVert = &g_pAccumVerts[g_nAccumVertCount];

	pVert->x = x;
	pVert->y = y;
	pVert->z = z;
	pVert->color = g_dwAccumCurrentDiffuse;
	pVert->tu = tu;
	pVert->tv = tv;

	g_pAccumIndex[g_nAccumIndexCount] = (WORD)g_nAccumVertCount;
	g_nAccumVertCount++;
	g_nAccumIndexCount++;
}

/* Texture-coordinate scale for the studio mesh writers below (1/width,
   1/height of the currently bound skin texture; set by a not-yet-
   reconstructed caller before a mesh batch). */
static float s_studioTexScaleS = 1.0f;
static float s_studioTexScaleT = 1.0f;

/* Studio mesh vertices share the same 32-byte accumulator slot as the lit
   D3DLVERTEX path above, but carry a vertex normal (floats at offset 3-5)
   instead of a packed color -- these meshes are hardware-lit, not tinted by
   the current diffuse. A flush of this data must use a normal-vertex FVF,
   not D3DFVF_LVERTEX. pCmds is a run of (vertindex, normindex, s, t) shorts,
   one group per vertex; pVertices/pNormals are the studio model's raw
   vec3_t arrays indexed by those (vertindex, normindex) pairs. */
void DCV_AddStudioMesh( int count, const short *pCmds, const byte *pVertices, const byte *pNormals )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	for (; count != 0; --count)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		{
			int s = pCmds[2];
			int t = pCmds[3];
			pOut[6] = (float)s * s_studioTexScaleS;
			pOut[7] = (float)t * s_studioTexScaleT;
		}
		pCmds += 4;
		pOut += 8;
	}
}

/* Chrome (environment-mapped) variant: the texture coordinate comes from a
   per-normal-index reflection table instead of the mesh's own s/t. */
void DCV_AddStudioMeshChrome( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const float *pChromeUV )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	for (; count != 0; --count)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		pOut[6] = pChromeUV[pCmds[1] * 2] * s_studioTexScaleS;
		pOut[7] = pChromeUV[pCmds[1] * 2 + 1] * s_studioTexScaleT;
		pCmds += 4;
		pOut += 8;
	}
}

/* Same as DCV_AddStudioMesh, plus a per-vertex byte (indexed by vertex, not
   sequential) packed into the low byte of the normal.x float's slot. */
void DCV_AddStudioMeshTagged( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const byte *pVertTag )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	for (; count != 0; --count)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		{
			int s = pCmds[2];
			int t = pCmds[3];
			pOut[6] = (float)s * s_studioTexScaleS;
			pOut[7] = (float)t * s_studioTexScaleT;
		}
		*(byte *)&pOut[3] = pVertTag[pCmds[0]];
		pCmds += 4;
		pOut += 8;
	}
}

/* Chrome + tagged: chrome UV table lookup, plus the per-vertex tag byte. */
void DCV_AddStudioMeshChromeTagged( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const float *pChromeUV, const byte *pVertTag )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	for (; count != 0; --count)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		pOut[6] = pChromeUV[pCmds[1] * 2] * s_studioTexScaleS;
		pOut[7] = pChromeUV[pCmds[1] * 2 + 1] * s_studioTexScaleT;
		*(byte *)&pOut[3] = pVertTag[pCmds[0]];
		pCmds += 4;
		pOut += 8;
	}
}

void DCV_AddPolyIndices( short base, int numverts )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	int n = numverts - 2;
	int loops = n + 1;

	if (loops < 0)
		loops = numverts;

	for (loops >>= 1; loops != 0; --loops)
	{
		*p++ = base;
		*p++ = base + 1;
		*p++ = base + 2;
		*p++ = base + 1;
		*p++ = base + 3;
		*p++ = base + 2;
		base += 2;
	}
	g_nAccumIndexCount += n * 3;
}

void DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];

	*p++ = (WORD)i0;
	*p++ = (WORD)i1;
	*p++ = (WORD)i3;
	*p++ = (WORD)i1;
	*p++ = (WORD)i2;
	*p++ = (WORD)i3;
	g_nAccumIndexCount += 6;
}

/* Studio batching helpers */

void DCV_AddIndicesStrip( int base, int count )
{
	int i;
	int n;

	if (count < 3)
		return;

	n = (count - 2) * 3;

	if (!DCV_EnsureSpace(0, n))
		return;

	for (i = 0; i < count - 2; ++i)
	{
		WORD *idx = &g_pAccumIndex[g_nAccumIndexCount];
		if (i & 1)
		{
			idx[0] = (WORD)(base + i + 1);
			idx[1] = (WORD)(base + i + 2);
			idx[2] = (WORD)(base + i);
		}
		else
		{
			idx[0] = (WORD)(base + i);
			idx[1] = (WORD)(base + i + 1);
			idx[2] = (WORD)(base + i + 2);
		}
		g_nAccumIndexCount += 3;
	}
}

void DCV_AddIndicesFan( short base, int count )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	short next = base + 1;
	int n = count - 2;

	for (; n != 0; --n)
	{
		*p++ = next;
		++next;
		*p++ = next;
		*p++ = base;
	}
	g_nAccumIndexCount += (count - 2) * 3;
}

/* A studio mesh's triangle command list: a run of (count, then count's
   vertex-index/normal-index/s/t groups) blocks, terminated by a zero count.
   A negative count is a fan of -count vertices (hub is the first one, then
   a sliding window of pairs); a positive count is a strip of count vertices,
   split into paired quads the same way DCV_AddPolyIndices splits one, with a
   trailing single triangle when the strip has an odd number of them. Indices
   here are the mesh's own vertex-index values, not accumulator-relative --
   studio mesh vertices are written in the same order this list references
   them, by DCV_AddStudioMesh and friends. */
void DCV_BuildStudioIndexList( const short *pCmds )
{
	WORD *pOut = &g_pAccumIndex[g_nAccumIndexCount];
	int total = 0;
	short count;

	while ((count = *pCmds) != 0)
	{
		if (count < 0)
		{
			short hub = pCmds[1];
			int tris = -count - 2;
			const short *p = pCmds + 2;
			int i;

			total += tris;
			for (i = tris; i != 0; --i)
			{
				*pOut++ = p[0];
				*pOut++ = p[1];
				*pOut++ = hub;
				++p;
			}
			pCmds = p + 1;
		}
		else
		{
			int tris = count - 2;
			int pairs = tris - 1;
			const short *p = pCmds + 1;
			int i;

			if (pairs < 0)
				pairs = tris;
			pairs >>= 1;

			total += tris;

			for (i = pairs; i != 0; --i)
			{
				*pOut++ = p[0];
				*pOut++ = p[1];
				*pOut++ = p[2];
				*pOut++ = p[1];
				*pOut++ = p[3];
				*pOut++ = p[2];
				p += 2;
			}

			if ((tris & 1) == 0)
				pCmds = p + 2;
			else
			{
				/* Odd triangle count: the last paired-quad iteration wrote
				   one triangle too many from data past the strip's end;
				   rewind over it so the next block's write overwrites it. */
				pCmds = p + 1;
				pOut -= 3;
			}
		}
	}

	g_nAccumIndexCount += total * 3;
}

/* Same quad-strip split as DCV_AddPolyIndices, but each pair of triangles is
   followed by a 0xFFFF strip-restart marker instead of running straight into
   the next pair. */
void DCV_AddIndicesStripRestart( short base, int count )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	short next = base + 1;
	int n = count - 2;
	int pairs = (n < 0) ? (count - 1) : n;
	int tailTri = n & 1;
	int i;

	if (n < 0 && tailTri != 0)
		tailTri -= 2;

	pairs >>= 1;

	for (i = pairs; i != 0; --i)
	{
		*p++ = next;
		*p++ = next + 1;
		*p++ = base;
		next += 2;
		*p++ = next;
		*p++ = (short)0xFFFF;
	}

	if (tailTri != 0)
	{
		*p++ = next;
		*p++ = next + 1;
		*p++ = base;
		*p = (short)0xFFFF;
	}

	g_nAccumIndexCount += pairs * 5 + tailTri * 4;
}

/* Same fan as DCV_AddIndicesFan, followed by a 0xFFFF strip-restart marker. */
void DCV_AddIndicesFanRestart( short base, int count )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	int i;

	for (i = count; i != 0; --i)
		*p++ = base++;
	*p = (short)0xFFFF;

	g_nAccumIndexCount += count + 1;
}

void DCV_SetTextureClamp( void )
{
	DCV_SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	DCV_SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
}

void DCV_SetTextureWrap( void )
{
	DCV_SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
}

/*
 * DCV_SetTexStateFromRenderMode
 *
 */
void DCV_SetTexStateFromRenderMode( int rendermode )
{
	switch (rendermode)
	{
	case kRenderNormal:
		DCV_TexState_Opaque();
		break;

	case kRenderTransColor:
	case kRenderTransTexture:
		DCV_TexState_Blend();
		break;

	case kRenderGlow:
	case kRenderTransAdd:
		DCV_TexState_Additive();
		break;

	case kRenderTransAlpha:
		DCV_TexState_Blend();
		break;

	default:
		DCV_TexState_Blend();
		break;
	}
}

/*
=============================================================================

	2D drawing

=============================================================================
*/

/* Begin a 2D drawing batch. */
void DCV_Begin2D( int mode, int flags )
{
	Sys_Error("NYI");
}

/* Program the render/texture-stage state for 2D drawing: modulate the
   texture by the vertex color, alpha-test cutout instead of blending. */
void DCV_2D_SetupStates( void )
{
	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHAFUNC,        D3DCMP_GREATER);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHAREF,         192);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA);

	DCV_SetupFog();
}

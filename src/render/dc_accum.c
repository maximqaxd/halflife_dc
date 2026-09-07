/*
 * dc_accum.c - AccumVerts / AccumIndex batch core.
 */

#include <shintr.h>

#include "quakedef.h"
#include "dc_accum.h"

#define QUAD_TABLE_ROWS     0x32
#define ROW_STRIDE_SHORTS   6

#define ACCUM_VERTS_SIZE    65536
#define ACCUM_INDEX_SIZE    16384
#define MULTI_MTX_0_SIZE    5120
#define MULTI_MTX_1_SIZE    1280

static WORD     *g_pQuadTable[QUAD_TABLE_ROWS + 1];
static WORD      g_QuadIndexData[7500 / sizeof(WORD)];

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
static D3DCOLOR  g_studioLightTable[128];
#define COLOR_OPAQUE_WHITE	0xFFFFFFFFu
#define STRIP_RESTART_INDEX	0xFFFF

DWORD            g_dwAccumCurrentDiffuse = COLOR_OPAQUE_WHITE;

static int DCV_GetMaxVertCount( void )
{
	return ACCUM_VERTS_SIZE / (int)sizeof(D3DLVERTEX);
}

static int DCV_GetMaxIndexCount( void )
{
	return ACCUM_INDEX_SIZE / (int)sizeof(WORD);
}

void DCV_AccumInit( void )
{
	int row;
	int row_stride;
	WORD *row_start;

	row_start = g_QuadIndexData;
	row_stride = 0;
	for (row = 0; row < QUAD_TABLE_ROWS; ++row)
	{
		WORD *out = row_start;
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

		row_start = (WORD *)((byte *)row_start + row_stride);
		row_stride += ROW_STRIDE_SHORTS;
	}
	g_pQuadTable[QUAD_TABLE_ROWS] = NULL;

	g_pAccumVerts = (D3DLVERTEX *)MnemoAlloc(ACCUM_VERTS_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumVerts");
	g_pAccumIndex = (WORD *)MnemoAlloc(ACCUM_INDEX_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumIndex");

	/* Reserve the matrix and lighting data used by studio rendering. */
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

/* Draw the accumulated studio batch. Studio meshes carry a vertex normal and
   are lit by the device, so they go out with the normal-vertex format rather
   than the pre-lit one the world uses, and skip the high-water bookkeeping. */
void DCV_SubmitBatchCopy( void )
{
	if (g_nAccumVertCount != 0)
	{
		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice,
			D3DPT_TRIANGLELIST, D3DFVF_VERTEX,
			g_pAccumVerts, g_nAccumVertCount,
			g_pAccumIndex, g_nAccumIndexCount,
			g_dwAccumFlushFlags);
		g_nAccumVertCount = 0;
		g_nAccumIndexCount = 0;
	}
}

/* Draw the accumulated studio batch through the hardware's multi-matrix path.
   Each vertex names its bone in the low byte of its normal, and the device
   skins it against the bone matrices, light directions and shade table that
   DCV_SetupStudioLighting built for this model. */
void DCV_SubmitBatchGuarded( void )
{
	if (g_nAccumVertCount != 0 && g_nAccumIndexCount != 0)
	{
		D3DMULTIMATRIX mm;

		mm.lpd3dMatrices = (LPD3DMATRIX)g_pMultiMtx0;
		mm.lpvLightDirs = g_pMultiMtx1;
		mm.lpLightTable = g_studioLightTable;
		mm.lpvVertices = g_pAccumVerts;
		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice,
			D3DPT_TRIANGLELIST, D3DFVF_VERTEX,
			&mm, g_nAccumVertCount,
			g_pAccumIndex, g_nAccumIndexCount,
			D3DDP_MULTIMATRIX);
		g_nAccumVertCount = 0;
		g_nAccumIndexCount = 0;
	}
}

/* Multiply two row-major D3D matrices with the SH-4 matrix/vector unit. */
static __inline void DCV_MultiplyMatrixSH4( D3DMATRIX *out, const D3DMATRIX *left, const D3DMATRIX *right )
{
	__asm(
		"frchg\n"
		"fmov.s @r5+, fr0\n"  "fmov.s @r5+, fr4\n"
		"fmov.s @r5+, fr8\n"  "fmov.s @r5+, fr12\n"
		"fmov.s @r5+, fr1\n"  "fmov.s @r5+, fr5\n"
		"fmov.s @r5+, fr9\n"  "fmov.s @r5+, fr13\n"
		"fmov.s @r5+, fr2\n"  "fmov.s @r5+, fr6\n"
		"fmov.s @r5+, fr10\n" "fmov.s @r5+, fr14\n"
		"fmov.s @r5+, fr3\n"  "fmov.s @r5+, fr7\n"
		"fmov.s @r5+, fr11\n" "fmov.s @r5, fr15\n"
		"frchg\n"

		"fmov.s @r6, fr0\n" "add #16, r6\n"
		"fmov.s @r6, fr1\n" "add #16, r6\n"
		"fmov.s @r6, fr2\n" "add #16, r6\n"
		"fmov.s @r6, fr3\n" "ftrv xmtrx, fv0\n"
		"fmov.s fr0, @r4\n" "add #16, r4\n"
		"fmov.s fr1, @r4\n" "add #16, r4\n"
		"fmov.s fr2, @r4\n" "add #16, r4\n"
		"fmov.s fr3, @r4\n"

		"add #-44, r6\n" "fmov.s @r6, fr0\n" "add #16, r6\n"
		"fmov.s @r6, fr1\n" "add #16, r6\n"
		"fmov.s @r6, fr2\n" "add #16, r6\n"
		"fmov.s @r6, fr3\n" "ftrv xmtrx, fv0\n"
		"add #-44, r4\n" "fmov.s fr0, @r4\n" "add #16, r4\n"
		"fmov.s fr1, @r4\n" "add #16, r4\n"
		"fmov.s fr2, @r4\n" "add #16, r4\n"
		"fmov.s fr3, @r4\n"

		"add #-44, r6\n" "fmov.s @r6, fr0\n" "add #16, r6\n"
		"fmov.s @r6, fr1\n" "add #16, r6\n"
		"fmov.s @r6, fr2\n" "add #16, r6\n"
		"fmov.s @r6, fr3\n" "ftrv xmtrx, fv0\n"
		"add #-44, r4\n" "fmov.s fr0, @r4\n" "add #16, r4\n"
		"fmov.s fr1, @r4\n" "add #16, r4\n"
		"fmov.s fr2, @r4\n" "add #16, r4\n"
		"fmov.s fr3, @r4\n"

		"add #-44, r6\n" "fmov.s @r6, fr0\n" "add #16, r6\n"
		"fmov.s @r6, fr1\n" "add #16, r6\n"
		"fmov.s @r6, fr2\n" "add #16, r6\n"
		"fmov.s @r6, fr3\n" "ftrv xmtrx, fv0\n"
		"add #-44, r4\n" "fmov.s fr0, @r4\n" "add #16, r4\n"
		"fmov.s fr1, @r4\n" "add #16, r4\n"
		"fmov.s fr2, @r4\n" "add #16, r4\n"
		"fmov.s fr3, @r4\n",
		out, left, right);
}

static __inline void DCV_TransformStudioLightSH4( float *out, const D3DMATRIX *matrix,
	const D3DVECTOR *light )
{
	__asm(
		"frchg\n"
		"fmov.s @r5+, fr0\n"  "fmov.s @r5+, fr4\n"
		"fmov.s @r5+, fr8\n"  "fmov.s @r5+, fr12\n"
		"fmov.s @r5+, fr1\n"  "fmov.s @r5+, fr5\n"
		"fmov.s @r5+, fr9\n"  "fmov.s @r5+, fr13\n"
		"fmov.s @r5+, fr2\n"  "fmov.s @r5+, fr6\n"
		"fmov.s @r5+, fr10\n" "fmov.s @r5+, fr14\n"
		"fmov.s @r5+, fr3\n"  "fmov.s @r5+, fr7\n"
		"fmov.s @r5+, fr11\n" "fmov.s @r5, fr15\n"
		"frchg\n"
		"frchg\n"
		"fldi0 fr12\n"
		"fldi0 fr13\n"
		"fldi0 fr14\n"
		"fldi0 fr15\n"
		"frchg\n"
		"fldi0 fr3\n"
		"fmov.s @r6+, fr0\n"
		"fmov.s @r6+, fr1\n"
		"fmov.s @r6, fr2\n"
		"ftrv xmtrx, fv0\n"
		"add #12, r4\n"
		"fmov.s fr2, @-r4\n"
		"fmov.s fr1, @-r4\n"
		"fmov.s fr0, @-r4\n",
		out, matrix, light);
}

void DCV_SetupStudioLighting( const float (*boneMatrices)[4][4], int count )
{
	static D3DMATRIX projection;
	static D3DMATRIX view;
	static D3DMATRIX world;
	static D3DMATRIX clip;
	static D3DMATRIX worldView;
	static D3DMATRIX worldViewProjection;
	static D3DMATRIX studioTransform;
	static D3DMATRIX boneMatrix;
	static D3DMATRIX transformedBone;
	D3DLIGHT2 *light;
	DWORD xOffset;
	int i;

	if (count != 0)
	{
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice,
			D3DTRANSFORMSTATE_PROJECTION, &projection);
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice,
			D3DTRANSFORMSTATE_VIEW, &view);
		g_pD3DDevice->lpVtbl->GetTransform(g_pD3DDevice,
			D3DTRANSFORMSTATE_WORLD, &world);

		clip = g_identityMatrix;
		clip._11 = 0.5f;
		clip._22 = 0.5f;
		clip._41 = 0.5f;
		clip._42 = 0.5f;

		DCV_MultiplyMatrixSH4(&worldView, &world, &view);
		DCV_MultiplyMatrixSH4(&worldViewProjection, &worldView, &projection);
		DCV_MultiplyMatrixSH4(&studioTransform, &worldViewProjection, &clip);

		xOffset = 0;
		for (i = 0; i < count; i++)
		{
			DWORD matrixMarker = 0xe0001000;
			const D3DMATRIX *source = (const D3DMATRIX *)&boneMatrices[i];
			D3DMATRIX *matrix = &((D3DMATRIX *)g_pMultiMtx0)[i];

			boneMatrix = *source;
			boneMatrix._11 = source->_11;
			boneMatrix._12 = source->_21;
			boneMatrix._13 = source->_31;
			boneMatrix._14 = source->_41;
			boneMatrix._21 = source->_12;
			boneMatrix._22 = source->_22;
			boneMatrix._23 = source->_32;
			boneMatrix._24 = source->_42;
			boneMatrix._31 = source->_13;
			boneMatrix._32 = source->_23;
			boneMatrix._33 = source->_33;
			boneMatrix._34 = source->_43;
			boneMatrix._41 = source->_14;
			boneMatrix._42 = source->_24;
			boneMatrix._43 = source->_34;
			boneMatrix._44 = 1.0f;

			DCV_MultiplyMatrixSH4(&transformedBone, &boneMatrix, &studioTransform);

			matrix->_11 = *(float *)&xOffset;
			matrix->_12 = transformedBone._11 * 640.0f + transformedBone._14 * *(float *)&xOffset;
			matrix->_13 = transformedBone._12 * -480.0f + transformedBone._14 * 480.0f;
			matrix->_14 = transformedBone._14;
			matrix->_21 = *(float *)&xOffset;
			matrix->_22 = transformedBone._21 * 640.0f + transformedBone._24 * *(float *)&xOffset;
			matrix->_23 = transformedBone._22 * -480.0f + transformedBone._24 * 480.0f;
			matrix->_24 = transformedBone._24;
			matrix->_31 = *(float *)&xOffset;
			matrix->_32 = transformedBone._31 * 640.0f + transformedBone._34 * *(float *)&xOffset;
			matrix->_33 = transformedBone._32 * -480.0f + transformedBone._34 * 480.0f;
			matrix->_34 = transformedBone._34;
			matrix->_41 = *(float *)&matrixMarker;
			matrix->_42 = transformedBone._41 * 640.0f + transformedBone._44 * *(float *)&xOffset;
			matrix->_43 = transformedBone._42 * -480.0f + transformedBone._44 * 480.0f;
			matrix->_44 = transformedBone._44;

			DCV_TransformStudioLightSH4(&((float *)g_pMultiMtx1)[i * 4 + 1], &boneMatrix,
				&g_lightData[0].dvDirection);
			((float *)g_pMultiMtx1)[i * 4 + 0] = 0.0f;
			((float *)g_pMultiMtx1)[i * 4 + 1] *= -251.0f;
			((float *)g_pMultiMtx1)[i * 4 + 2] *= -251.0f;
			((float *)g_pMultiMtx1)[i * 4 + 3] *= -251.0f;
		}
		light = &g_lightData[0];
	}
	else
	{
		light = &g_lightData[0];
	}

	for (i = 0; i < 64; i++)
	{
		float level = ((float)i * 255.0f) / 64.0f;
		g_studioLightTable[i] =
			(((int)(g_backgroundMaterialData.diffuse.g * level * light->dcvColor.g +
				g_backgroundMaterialData.emissive.g * 255.0f) |
			(int)(g_backgroundMaterialData.diffuse.r * level * light->dcvColor.r +
				g_backgroundMaterialData.emissive.r * 255.0f) << 8) << 8) |
			(int)(g_backgroundMaterialData.diffuse.b * level * light->dcvColor.b +
				g_backgroundMaterialData.emissive.b * 255.0f);
	}
	for (i = 64; i < 128; i++)
		g_studioLightTable[i] = g_studioLightTable[0];
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
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
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

int DCV_GetVertCount( void )
{
	return g_nAccumVertCount;
}

/* Surface records carry the vertex count followed by eight floats per vertex.
   The accumulation passes copy those vertices and emit the matching indices. */
#define DC_POLY_NUMVERTS( poly )  (*(const short *)((const byte *)(poly) + 8))
#define DC_POLY_VERTS( poly )     ((const float *)((const byte *)(poly) + 12))

/* Solid textured poly: current diffuse, base texture UV (verts[][4],[5]). */
void DCV_AccumSolidPoly( const void *poly )
{
	int numverts = DC_POLY_NUMVERTS(poly);
	int needed = (numverts - 2) * 3;
	const WORD *pSrc = g_pQuadTable[numverts];
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	const float *pVert;
	D3DLVERTEX *pOut;
	int i;

	i = needed;
	while (i--)
		*pIdx++ = (WORD)(g_nAccumVertCount + *pSrc++);

	pVert = DC_POLY_VERTS(poly);
	pOut = &g_pAccumVerts[g_nAccumVertCount];
	i = numverts;
	while (i--)
	{
		__prefetch((unsigned long *)(pVert + 8));
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
	int needed = (numverts - 2) * 3;
	const WORD *pSrc = g_pQuadTable[numverts];
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	const float *pVert;
	D3DLVERTEX *pOut;
	int i;

	i = needed;
	while (i--)
		*pIdx++ = (WORD)(g_nAccumVertCount + *pSrc++);

	pVert = DC_POLY_VERTS(poly);
	pOut = &g_pAccumVerts[g_nAccumVertCount];
	i = numverts;
	while (i--)
	{
		__prefetch((unsigned long *)(pVert + 8));
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
	int needed = (numverts - 2) * 3;
	const WORD *pSrc = g_pQuadTable[numverts];
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	const float *pVert;
	D3DLVERTEX *pOut;
	int i;

	i = needed;
	while (i--)
		*pIdx++ = (WORD)(g_nAccumVertCount + *pSrc++);

	pVert = DC_POLY_VERTS(poly);
	pOut = &g_pAccumVerts[g_nAccumVertCount];
	i = numverts;
	while (i--)
	{
		__prefetch((unsigned long *)(pVert + 8));
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
/* The U scroll offset is stored in renderer state rather than passed in. */
// Set by ScrollOffset() for the surface currently being accumulated.
float g_flScrollOffset;

void DCV_AccumScrollPoly( const void *poly )
{
	int numverts = DC_POLY_NUMVERTS(poly);
	int needed = (numverts - 2) * 3;
	const WORD *pSrc = g_pQuadTable[numverts];
	WORD *pIdx = &g_pAccumIndex[g_nAccumIndexCount];
	const float *pVert;
	D3DLVERTEX *pOut;
	int i;

	i = needed;
	while (i--)
		*pIdx++ = (WORD)(g_nAccumVertCount + *pSrc++);

	pVert = DC_POLY_VERTS(poly);
	pOut = &g_pAccumVerts[g_nAccumVertCount];
	i = numverts;
	while (i--)
	{
		__prefetch((unsigned long *)(pVert + 8));
		pOut->x = pVert[0];
		pOut->y = pVert[1];
		pOut->z = pVert[2];
		pOut->color = g_dwAccumCurrentDiffuse;
		pOut->tu = pVert[4] + g_flScrollOffset;
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
	if (!v)
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
void DCV_PushVertexLit( const vec_t *pos, float tu, float tv )
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
float g_flStudioTexScaleS = 1.0f;
float g_flStudioTexScaleT = 1.0f;
extern float chrome[][2];

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

	while (count--)
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
			pOut[6] = (float)s * g_flStudioTexScaleS;
			pOut[7] = (float)t * g_flStudioTexScaleT;
		}
		pCmds += 4;
		pOut += 8;
	}
}

/* Chrome (environment-mapped) variant: the texture coordinate comes from a
   per-normal-index reflection table instead of the mesh's own s/t. */
void DCV_AddStudioMeshChrome( int count, const short *pCmds, const byte *pVertices, const byte *pNormals )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	while (count--)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		pOut[6] = chrome[pCmds[1]][0] * g_flStudioTexScaleS;
		pOut[7] = chrome[pCmds[1]][1] * g_flStudioTexScaleT;
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

	while (count--)
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
			pOut[6] = (float)s * g_flStudioTexScaleS;
			pOut[7] = (float)t * g_flStudioTexScaleT;
		}
		*(byte *)&pOut[3] = pVertTag[pCmds[0]];
		pCmds += 4;
		pOut += 8;
	}
}

/* Chrome + tagged: chrome UV table lookup, plus the per-vertex tag byte. */
void DCV_AddStudioMeshChromeTagged( int count, const short *pCmds, const byte *pVertices, const byte *pNormals, const byte *pVertTag )
{
	float *pOut = (float *)&g_pAccumVerts[g_nAccumVertCount];

	g_nAccumVertCount += count;

	while (count--)
	{
		const float *pV = (const float *)(pVertices + pCmds[0] * 12);
		const float *pN = (const float *)(pNormals + pCmds[1] * 12);

		pOut[0] = pV[0];
		pOut[1] = pV[1];
		pOut[2] = pV[2];
		pOut[3] = pN[0];
		pOut[4] = pN[1];
		pOut[5] = pN[2];
		pOut[6] = chrome[pCmds[1]][0] * g_flStudioTexScaleS;
		pOut[7] = chrome[pCmds[1]][1] * g_flStudioTexScaleT;
		*(byte *)&pOut[3] = pVertTag[pCmds[0]];
		pCmds += 4;
		pOut += 8;
	}
}

void DCV_AddPolyIndices( int base, int numverts )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	int n = numverts - 2;
	int loops = (n + 1) / 2;

	while (loops--)
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

void DCV_AddIndicesFan( int base, int count )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	int next = base + 1;
	int n = count - 2;
	int i = n;

	while (i--)
	{
		*p++ = next;
		++next;
		*p++ = next;
		*p++ = base;
	}
	g_nAccumIndexCount += n * 3;
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

	while ((count = *pCmds++) != 0)
	{
		if (count < 0)
		{
			short hub = *pCmds++;
			int tris = -count - 2;
			int i;

			total += tris;
			i = tris;
			while (i--)
			{
				*pOut++ = *pCmds++;
				*pOut++ = *pCmds;
				*pOut++ = hub;
			}
			pCmds++;
		}
		else
		{
			int tris = count - 2;
			int pairs = (tris + 1) / 2;
			int i;

			total += tris;

			i = pairs;
			while (i--)
			{
				*pOut++ = pCmds[0];
				*pOut++ = pCmds[1];
				*pOut++ = pCmds[2];
				*pOut++ = pCmds[1];
				*pOut++ = pCmds[3];
				*pOut++ = pCmds[2];
				pCmds += 2;
			}

			if (tris & 1)
			{
				/* Odd triangle count: the last paired-quad iteration wrote
				   one triangle too many from data past the strip's end;
				   rewind over it so the next block's write overwrites it. */
				pCmds += 1;
				pOut -= 3;
			}
			else
			{
				pCmds += 2;
			}
		}
	}

	g_nAccumIndexCount += total * 3;
}

/* Same quad-strip split as DCV_AddPolyIndices, but each pair of triangles is
   followed by a strip-restart marker instead of running straight into
   the next pair. */
void DCV_AddIndicesFanRestart( short base, int count )
{
	WORD *p;
	int next = base + 1;
	int tailTri;
	int pairs;
	int i;

	count -= 2;
	pairs = count / 2;
	tailTri = count % 2;
	p = &g_pAccumIndex[g_nAccumIndexCount];

	i = pairs;
	while (i--)
	{
		*p++ = next;
		++next;
		*p++ = next;
		*p++ = base;
		++next;
		*p++ = next;
		*p++ = (short)STRIP_RESTART_INDEX;
	}

	if (tailTri != 0)
	{
		*p++ = next;
		++next;
		*p++ = next;
		*p++ = base;
		*p = (short)STRIP_RESTART_INDEX;
	}

	g_nAccumIndexCount += pairs * 5 + tailTri * 4;
}

/* Same strip as DCV_AddIndicesStrip, followed by a strip-restart marker. */
void DCV_AddIndicesStripRestart( int base, int count )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];
	int i = count;

	while (i--)
	{
		*p = base;
		base++;
		p++;
	}
	*p = (short)STRIP_RESTART_INDEX;

	g_nAccumIndexCount += count + 1;
}

/* Same as DCV_BuildStudioIndexList, but emits strips and fans the hardware can
   take back to back: each run ends with a restart marker instead of
   being expanded into separate triangles. */
void DCV_AssembleStudioIndexListRestart( const short *pCmds )
{
	WORD *pOut = &g_pAccumIndex[g_nAccumIndexCount];
	int written = 0;
	int markers = 0;
	int command;

	while ((command = *pCmds++) != 0)
	{
		if (command < 0)
		{
			int tris = -command - 2;
			int pairs = tris / 2;
			int tail = tris % 2;
			unsigned short hub;
			int i;

			hub = *pCmds++;
			written += pairs * 4 + tail * 3;
			markers += pairs + tail;

			i = pairs;
			while (i--)
			{
				*pOut++ = *pCmds++;
				*pOut++ = *pCmds++;
				*pOut++ = hub;
				*pOut++ = *pCmds;
				*pOut++ = (short)STRIP_RESTART_INDEX;
			}

			if (tail != 0)
			{
				*pOut++ = *pCmds++;
				*pOut++ = *pCmds;
				*pOut++ = hub;
				*pOut++ = (short)STRIP_RESTART_INDEX;
			}

			++pCmds;
		}
		else
		{
			int i;

			written += command;
			markers++;
			i = command;
			while (i--)
				*pOut++ = *pCmds++;
			*pOut++ = (short)STRIP_RESTART_INDEX;
		}
	}

	g_nAccumIndexCount += written + markers;
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

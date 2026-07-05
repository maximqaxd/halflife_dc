/*
 * dc_accum.c - AccumVerts / AccumIndex batch core.
 */

#include "quakedef.h"
#include "dc_accum.h"

extern void* Sys_GetD3DDevice3( void );

#define QUAD_TABLE_ROWS     0x32
#define ROW_STRIDE_SHORTS   6

#define ACCUM_VERTS_SIZE    65536
#define ACCUM_INDEX_SIZE    16384
#define MULTI_MTX_0_SIZE    5120
#define MULTI_MTX_1_SIZE    2048

static short    *g_pQuadTable[QUAD_TABLE_ROWS + 1];
static short     g_QuadIndexData[7350 / sizeof(short)];

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
static qboolean  g_bAccumInitialized = FALSE;

static int DCV_GetMaxVertCount( void )
{
	return ACCUM_VERTS_SIZE / (int)sizeof(D3DLVERTEX);
}

static int DCV_GetMaxIndexCount( void )
{
	return ACCUM_INDEX_SIZE / (int)sizeof(WORD);
}

static BYTE DCV_ClampColorFloat( float value )
{
	if (value < 0.0f)
		value = 0.0f;
	else if (value > 1.0f)
		value = 1.0f;
		
	return (BYTE)(value * 255.0f);
}

static void DCV_ClearBatch( void )
{
	g_nAccumVertCount = 0;
	g_nAccumIndexCount = 0;
}

static const short* DCV_GetQuadIndices( int numverts, int *out_count )
{
	if (numverts < 3 || numverts > QUAD_TABLE_ROWS)
		return NULL;

	if (out_count)
		*out_count = (numverts - 2) * 3;
	return g_pQuadTable[numverts];
}

void DCV_AccumInit( void )
{
	int row;
	int row_stride;
	short *row_start;

	if (g_bAccumInitialized)
		return;

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
			out[0] = (short)high;
			out[1] = base;
			out[2] = (short)(high - 1);
			out[3] = base;
			out[4] = base + 1;
			out[5] = (short)(high - 1);
			out += ROW_STRIDE_SHORTS;
			base++;
		}

		row_start = (short *)((byte *)row_start + row_stride);
		row_stride += ROW_STRIDE_SHORTS * (int)sizeof(short);
	}
	g_pQuadTable[QUAD_TABLE_ROWS] = NULL;

	g_pAccumVerts = (D3DLVERTEX *)MnemoAlloc(ACCUM_VERTS_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumVerts");
	g_pAccumIndex = (WORD *)MnemoAlloc(ACCUM_INDEX_SIZE, MNEMO_FLAG_MALLOC, 0, "AccumIndex");

	/* Reserve for multi matrixes for future use in r_studio.c */
	g_pMultiMtx0 = MnemoAlloc(MULTI_MTX_0_SIZE, MNEMO_FLAG_MALLOC, 0, "MultiMtx");
	g_pMultiMtx1 = MnemoAlloc(MULTI_MTX_1_SIZE, MNEMO_FLAG_MALLOC, 0, "MultiMtx");

	DCV_ClearBatch();
	g_bAccumInitialized = TRUE;
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
	DCV_SetColor(
		(int)DCV_ClampColorFloat(r),
		(int)DCV_ClampColorFloat(g),
		(int)DCV_ClampColorFloat(b),
		(int)DCV_ClampColorFloat(a));
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
	if (!g_bAccumInitialized)
		return FALSE;

	if (add_verts < 0 || add_indices < 0)
		return FALSE;

	if (add_verts > DCV_GetMaxVertCount() || add_indices > DCV_GetMaxIndexCount())
		return FALSE;

	if (g_nAccumVertCount + add_verts > DCV_GetMaxVertCount() ||
		g_nAccumIndexCount + add_indices > DCV_GetMaxIndexCount())
	{
		DCV_Flush();
	}

	return TRUE;
}

int DCV_GetVertCount( void )
{
	return g_nAccumVertCount;
}

void DCV_AddLVertex( const D3DLVERTEX* v )
{
	if (!v || !DCV_EnsureSpace(1, 0))
		return;

	g_pAccumVerts[g_nAccumVertCount++] = *v;
}

void DCV_AddPolyIndices( int base, int numverts )
{
	const short *quad_indices;
	int needed_indices;
	int i;

	quad_indices = DCV_GetQuadIndices(numverts, &needed_indices);

	if (!quad_indices || !DCV_EnsureSpace(0, needed_indices))
		return;

	for (i = 0; i < needed_indices; ++i)
		g_pAccumIndex[g_nAccumIndexCount++] = (WORD)(base + quad_indices[i]);
}

void DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 )
{
	WORD *p = &g_pAccumIndex[g_nAccumIndexCount];

	p[0] = (WORD)i0;
	p[1] = (WORD)i1;
	p[2] = (WORD)i3;
	p[3] = (WORD)i1;
	p[4] = (WORD)i2;
	p[5] = (WORD)i3;
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

void DCV_AddIndicesFan( int base, int count )
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
		idx[0] = (WORD)(base + 1 + i);
		idx[1] = (WORD)(base + 2 + i);
		idx[2] = (WORD)(base);
		g_nAccumIndexCount += 3;
	}
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
		DCV_TexState_AlphaTest();
		break;

	default:
		DCV_TexState_Blend();
		break;
	}
}

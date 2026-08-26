// dc_surf.c: surface-related refresh code

#include "quakedef.h"
#include "cmodel.h"
#include "decal.h"
#include "draw.h"
#include "pr_cmds.h"
#include "gl_water.h"
#include "dc_draw.h"
#include "dc_accum.h"
#include "view.h"
#include "input.h"
#include <floatmathlib.h>

#define MAX_DECALSURFS		500

int		lightmap_bytes;		// 1, 2, or 4
int	lightmap_used;
#define MAX_BLOCK_LIGHTS	(18 * 18)
colorVec blocklights[MAX_BLOCK_LIGHTS];

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	MAX_LIGHTMAPS	64
int			active_lightmaps;
int			gl_lightmap_format;

// Only one lightmap block is filled in at a time; once it is full it gets
// uploaded as a texture and the allocation watermarks start over.
int			allocated[BLOCK_WIDTH];
byte		lightmaps[BLOCK_WIDTH * BLOCK_HEIGHT * 4];

static glpoly_t*  lightmap_polys[MAX_LIGHTMAPS];
static short      lightmap_modified[MAX_LIGHTMAPS];
static int        lm_texnum[MAX_LIGHTMAPS];

msurface_t* gDecalSurfs[MAX_DECALSURFS];
int gDecalSurfCount;

// The hash-deduped table of unique plane normals mplane_t.normalindex refers
// to. Built at BSP load time (world.c/dc_model.c, not yet reconstructed);
// left NULL here as a forward reference until that loader code is written.
planenormal_t* g_planeNormalTable;

// Set by R_SetRenderMode: non-zero while an entity is being drawn with the
// alpha-tested 2D state (kRenderTransAlpha). Those surfaces carry their own
// per-vertex colour and are skipped by the lightmap blend pass.
int r_alphatestmode;

// Texture upload target/type constants and the entry point itself; the
// Direct3D back end services the lightmap page directly, so the call is inert.
#define GL_TEXTURE_2D		0x0DE1
#define GL_UNSIGNED_BYTE	0x1401
#define GL_RGBA				0x1908
void glTexSubImage2D( int target, int level, int xoffset, int yoffset,
	int width, int height, int format, int type, const void* pixels );

// How far the near plane is pulled in for the lightmap pass so it lands on top
// of the base pass instead of z-fighting with it.
#define LIGHTMAP_DEPTH_NUDGE	0.0005f

extern float g_frustum_zn;
void R_ApplyViewModelProjection( float zn );

extern int numgltextures;
int DC_LoadTexture( char* identifier, int texture_type, int width, int height,
	void* data, short mipmap, int tex_type, unsigned char* pPal );

void R_RenderDynamicLightmaps( msurface_t* fa );
void DrawGLPolyScroll( msurface_t* psurface, cl_entity_t* pEntity );
void DrawGLSolidPoly( glpoly_t* p );
void DrawLightmapWaterPoly( glpoly_t* p );
void DrawGLWaterPoly( glpoly_t* p );
void DrawGLPoly( glpoly_t* p );

float ScrollOffset( msurface_t* psurface, cl_entity_t* pEntity );

#define SURF_NEAR_CLIP_DIST   4.0f
#define SURF_CLIPPED_POLY_MAX_VERTS 64

// -----------------------------------------------------------------------------
// D3D L-vertex batch (world + studio)
// -----------------------------------------------------------------------------

#define SURF_MAX_VERTS       2048
#define SURF_MAX_INDICES     8192

static float DCV_SurfNearClipDistance( const float *pfVert )
{
	vec3_t vecToVert;

	VectorSubtract(pfVert, r_origin, vecToVert);
	return DotProduct(vecToVert, vpn) - SURF_NEAR_CLIP_DIST;
}

static void DCV_LerpClippedPolyVertex( const float *pfStart, const float *pfEnd, float flFrac, float *pfOut )
{
	int i;
	DWORD cs, ce, co;
	unsigned rs, gs, bs, re, ge, be, ro, go, bo;
	for (i = 0; i < VERTEXSIZE; ++i)
	{
		if (i == 3)
		{
			/* Slot [3] is a packed 0xAARRGGBB colour; interpolating the raw bits as a float
			 * produces garbage.  Interpolate each channel in [0..255] separately. */
			memcpy(&cs, &pfStart[3], sizeof(DWORD));
			memcpy(&ce, &pfEnd[3],   sizeof(DWORD));
			rs = (cs >> 16) & 0xFF;
			re = (ce >> 16) & 0xFF;
			gs = (cs >> 8) & 0xFF;
			ge = (ce >> 8) & 0xFF;
			bs = cs & 0xFF;
			be = ce & 0xFF;
			ro = (unsigned)((float)rs + ((float)re - (float)rs) * flFrac);
			go = (unsigned)((float)gs + ((float)ge - (float)gs) * flFrac);
			bo = (unsigned)((float)bs + ((float)be - (float)bs) * flFrac);
			co = 0xFF000000u | (ro << 16) | (go << 8) | bo;
			memcpy(&pfOut[3], &co, sizeof(DWORD));
		}
		else
		{
			pfOut[i] = pfStart[i] + (pfEnd[i] - pfStart[i]) * flFrac;
		}
	}
}

static int DCV_ClipPolyToNearPlane( const float pfInVerts[][VERTEXSIZE], int nInVerts, float pfOutVerts[][VERTEXSIZE] )
{
	const float *pfPrev;
	float flPrevDist;
	int nOutVerts;
	int i;

	if (nInVerts < 3)
		return 0;

	pfPrev = pfInVerts[nInVerts - 1];
	flPrevDist = DCV_SurfNearClipDistance(pfPrev);
	nOutVerts = 0;

	for (i = 0; i < nInVerts; ++i)
	{
		const float *pfCur;
		float flCurDist;
		qboolean bPrevInside;
		qboolean bCurInside;

		pfCur = pfInVerts[i];
		flCurDist = DCV_SurfNearClipDistance(pfCur);
		bPrevInside = (flPrevDist >= 0.0f) ? TRUE : FALSE;
		bCurInside = (flCurDist >= 0.0f) ? TRUE : FALSE;

		if (bPrevInside != bCurInside)
		{
			float flFrac;

			if (nOutVerts >= SURF_CLIPPED_POLY_MAX_VERTS)
				return 0;

			flFrac = flPrevDist / (flPrevDist - flCurDist);
			DCV_LerpClippedPolyVertex(pfPrev, pfCur, flFrac, pfOutVerts[nOutVerts]);
			++nOutVerts;
		}

		if (bCurInside)
		{
			if (nOutVerts >= SURF_CLIPPED_POLY_MAX_VERTS)
				return 0;

			memcpy(pfOutVerts[nOutVerts], pfCur, sizeof(pfOutVerts[nOutVerts]));
			++nOutVerts;
		}

		pfPrev = pfCur;
		flPrevDist = flCurDist;
	}

	return nOutVerts;
}

/*
 * DCV_AccumGLPoly - Copy verts to AccumVerts, indices from quad table to AccumIndex
 * - Increment AccumVertCount, AccumIndexCount
 * - If AccumVertCount > SURF_MAX_VERTS, flush
 */
void DCV_AccumGLPoly( float verts[][VERTEXSIZE], int numverts, DWORD diffuse, float s_offset )
{
	float clipped[SURF_CLIPPED_POLY_MAX_VERTS][VERTEXSIZE];
	int clip_n;
	int i, base, needed_indices;

	if (numverts < 3)
		return;

	clip_n = DCV_ClipPolyToNearPlane((const float (*)[VERTEXSIZE])verts, numverts, clipped);
	if (clip_n < 3)
		return;

	needed_indices = (clip_n - 2) * 3;
	if (clip_n > SURF_MAX_VERTS || needed_indices > SURF_MAX_INDICES)
		return;

	if (!DCV_EnsureSpace(clip_n, needed_indices))
		return;

	base = DCV_GetVertCount();
	for (i = 0; i < clip_n; i++)
	{
		/* Binary DrawGLPoly uses g_dwAccumCurrentDiffuse per draw, not per-vertex [3];
		 * world lighting is applied in R_BlendLightmaps. */
		DCV_SetPackedColor(diffuse);
		DCV_AddVertex(
			clipped[i][0],
			clipped[i][1],
			clipped[i][2],
			clipped[i][4] + s_offset,
			clipped[i][5]);
	}
	DCV_AddPolyIndices(base, clip_n);
	DCV_FlushIfLarge();
}



void DCV_BindTexture( int texnum )
{
	GL_Bind(texnum, 0);
}

static DWORD DCV_SurfColorFromEntity( const cl_entity_t* ent )
{
	BYTE r, g, b, a;
	if (!ent)
		return 0xFFFFFFFFu;
	r = (BYTE)ent->rendercolor.r;
	g = (BYTE)ent->rendercolor.g;
	b = (BYTE)ent->rendercolor.b;
	if (r == 0 && g == 0 && b == 0)
		return 0xFFFFFFFFu;
	a = (BYTE)(r_blend * 255.0f);
	return ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
}

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights( msurface_t* surf )
{
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			smax, tmax;
	mtexinfo_t* tex;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	tex = surf->texinfo;

	for (lnum = 0; lnum < MAX_DLIGHTS; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
			continue;		// not lit by this light

		VectorSubtract(cl_dlights[lnum].origin, currententity->origin, impact);

		rad = cl_dlights[lnum].radius;
		dist = DotProduct(impact, surf->plane->normal)
			- surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0; t < tmax; t++)
		{
			td = local[1] - t * 16;
			if (td < 0)
				td = -td;
			for (s = 0; s < smax; s++)
			{
				sd = local[0] - s * 16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);
				if (dist < minlight)
				{
					unsigned delta;
					delta = (rad - dist) * 256;

					blocklights[t * smax + s].r += (delta * cl_dlights[lnum].color.r) >> 8;
					blocklights[t * smax + s].g += (delta * cl_dlights[lnum].color.g) >> 8;
					blocklights[t * smax + s].b += (delta * cl_dlights[lnum].color.b) >> 8;
				}
			}
		}
	}
}


/*
===============
LT2 LERP 'a' decode
Decode one grid; returns bytes consumed or 0 on error.
===============
*/
int DCV_LT2Decode( const byte* in, int in_size, color24* out, int w, int h )
{
	int gw, gh, need, y, x, cy, cx;
	const byte* p;
	color24* grid;
	float u, v, fx, fy;
	int r, g, b;

	if (in_size < 1) return 0;
	gh = ((in[0] >> 4) & 0x0F) + 2;
	gw = (in[0] & 0x0F) + 2;
	need = 1 + gw * gh * 3;
	if (need > in_size) return 0;

	p = in + 1;
	/* Max grid 17x17 from opcode 0xFF */
	if (gw > 17 || gh > 17) return 0;
	{
		color24 grid_storage[17 * 17];
		grid = grid_storage;

	for (y = 0; y < gh; y++)
	{
		for (x = 0; x < gw; x++)
		{
			grid[y * gw + x].r = p[0];
			grid[y * gw + x].g = p[1];
			grid[y * gw + x].b = p[2];
			p += 3;
		}
	}

	for (y = 0; y < h; y++)
	{
		v = (h == 1) ? 0.0f : ((float)y / (float)(h - 1)) * (float)(gh - 1);
		cy = (gh > 1) ? (int)floor(v) : 0;
		if (cy < 0) cy = 0;
		if (cy > gh - 2) cy = gh - 2;
		fy = (gh > 1) ? (v - (float)cy) : 0.0f;

		for (x = 0; x < w; x++)
		{
			u = (w == 1) ? 0.0f : ((float)x / (float)(w - 1)) * (float)(gw - 1);
			cx = (gw > 1) ? (int)floor(u) : 0;
			if (cx < 0) cx = 0;
			if (cx > gw - 2) cx = gw - 2;
			fx = (gw > 1) ? (u - (float)cx) : 0.0f;

			/* bilerp */
			r = (int)(grid[cy * gw + cx].r + (grid[cy * gw + cx + 1].r - grid[cy * gw + cx].r) * fx + 0.5f);
			g = (int)(grid[cy * gw + cx].g + (grid[cy * gw + cx + 1].g - grid[cy * gw + cx].g) * fx + 0.5f);
			b = (int)(grid[cy * gw + cx].b + (grid[cy * gw + cx + 1].b - grid[cy * gw + cx].b) * fx + 0.5f);
			r = (int)(r + (grid[(cy + 1) * gw + cx].r + (grid[(cy + 1) * gw + cx + 1].r - grid[(cy + 1) * gw + cx].r) * fx - r) * fy + 0.5f);
			g = (int)(g + (grid[(cy + 1) * gw + cx].g + (grid[(cy + 1) * gw + cx + 1].g - grid[(cy + 1) * gw + cx].g) * fx - g) * fy + 0.5f);
			b = (int)(b + (grid[(cy + 1) * gw + cx].b + (grid[(cy + 1) * gw + cx + 1].b - grid[(cy + 1) * gw + cx].b) * fx - b) * fy + 0.5f);
			if (r < 0) r = 0; if (r > 255) r = 255;
			if (g < 0) g = 0; if (g > 255) g = 255;
			if (b < 0) b = 0; if (b > 255) b = 255;
			out[y * w + x].r = (byte)r;
			out[y * w + x].g = (byte)g;
			out[y * w + x].b = (byte)b;
		}
	}
	}
	return need;
}

/*
===============
DC_FullbrightBlockLights

Fill blocklights with full bright (255).
===============
*/
static void DC_FullbrightBlockLights( int smax, int tmax )
{
	int i, size = smax * tmax;

	if (smax * tmax > MAX_BLOCK_LIGHTS)
		Sys_Error("Oversized surface in DC_FullbrightBlockLights");

	for (i = 0; i < size; i++)
	{
		blocklights[i].r = 255 * 256;
		blocklights[i].g = 255 * 256;
		blocklights[i].b = 255 * 256;
	}
}

/*
===============
DC_ClearBlockLights

Zero blocklights. 
===============
*/
static void DC_ClearBlockLights( int smax, int tmax )
{
	int i, size = smax * tmax;

	if (smax * tmax > MAX_BLOCK_LIGHTS)
		Sys_Error("Oversized surface in DC_ClearBlockLights");
		
	for (i = 0; i < size; i++)
	{
		blocklights[i].r = 0;
		blocklights[i].g = 0;
		blocklights[i].b = 0;
	}
}

/*
===============
DC_SumBlockLights

Accumulate samples into blocklights. cl.worldmodel->lightmap_mode selects the
on-disk encoding: 1 = packed-delta 16-bit samples, 2 = LT2 row-run bilinear,
3 = LT2 LERP 'a' grid bilinear.
===============
*/
#define LT2_LIGHTGAMMA(j) ((unsigned)(lightgammatable[(unsigned)(j) << 2]) >> 2)

// floatmathlib.h only fast-paths `floor`/`fceil`, not `ceil` -- so a bare
// `ceil()` call falls through to the real double-precision libm routine.
// Declare it so the compiler emits a proper double-returning call for it
// instead of assuming an int-returning implicit declaration.
extern double ceil( double x );

/* Mode 1 texel encoding: a stream of unsigned shorts, one per texel, row-major.
 * Bit 15 clear -> an absolute RGB555 sample (5 bits/channel, R:14-10 G:9-5 B:4-0,
 * each widened to 0-248 by <<3). Bit 15 set -> a signed delta from the *previous*
 * texel's decoded R/G/B: R has its own sign (bit14) and 4-bit magnitude (13-10);
 * G and B share one sign bit (9) with their own 4-bit magnitudes (8-5 and 3-0). */
#define LT2D_DELTA_FLAG   0x8000
#define LT2D_R_MASK_ABS   0x7c00
#define LT2D_G_MASK_ABS   0x03e0
#define LT2D_B_MASK_ABS   0x001f
#define LT2D_R_SIGN       0x4000
#define LT2D_R_MAG_MASK   0x3c00
#define LT2D_GB_SIGN      0x0200
#define LT2D_G_MAG_MASK   0x01e0
#define LT2D_B_MAG_MASK   0x000f

static void DC_SumBlockLights( msurface_t* psurf, int smax, int tmax )
{
	int i, maps, size = smax * tmax;
	color24* lightmap;
	unsigned scale;

	if (smax * tmax > MAX_BLOCK_LIGHTS)
		Sys_Error("Oversized surface in DC_SumBlockLights");

	// R_BuildLightMap only calls us when psurf->samples is non-NULL; no need
	// to check it again here.
	lightmap = psurf->samples;

	if (cl.worldmodel->lightmap_mode == 1)
	{
		const unsigned short* p = (const unsigned short*)lightmap;
		int r, g, b;

		for (maps = 0; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255; maps++)
		{
			scale = d_lightstylevalue[psurf->styles[maps]];
			psurf->cached_light[maps] = (short)scale;

			for (i = 0; i < size; i++)
			{
				unsigned short v = *p++;

				if (!(v & LT2D_DELTA_FLAG))
				{
					r = (v & LT2D_R_MASK_ABS) >> 7;
					g = (v & LT2D_G_MASK_ABS) >> 2;
					b = (v & LT2D_B_MASK_ABS) << 3;
				}
				else
				{
					int d;

					d = (v & LT2D_R_MAG_MASK) >> 10;
					r += (v & LT2D_R_SIGN) ? -d : d;
					d = (v & LT2D_G_MAG_MASK) >> 5;
					g += (v & LT2D_GB_SIGN) ? -d : d;
					d = (v & LT2D_B_MAG_MASK);
					b += (v & LT2D_GB_SIGN) ? -d : d;
				}

				blocklights[i].r += r * scale;
				blocklights[i].g += g * scale;
				blocklights[i].b += b * scale;
			}
		}
	}
	else if (cl.worldmodel->lightmap_mode == 2)
	{
		/* LT2 row-run bilinear: each row is a run of `n` RGB triples, resampled
		 * across smax columns. `floor`/`ceil` are called as real library
		 * routines here (not folded to a truncate), matching the binary
		 * exactly -- `floor` has a fast single-precision path via
		 * floatmathlib's `floors`, but `ceil` doesn't (only `fceil` does), so
		 * it falls through to the plain double-precision libm routine. */
		const byte* lt2ptr = (const byte*)lightmap;
		float recip = 1.0f / (float)(smax - 1);

		for (maps = 0; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255; maps++)
		{
			int t, s;

			scale = d_lightstylevalue[psurf->styles[maps]];
			psurf->cached_light[maps] = (short)scale;

			for (t = 0; t < tmax; t++)
			{
				int n;
				const byte* row;
				float step, pos;

				n = (int)*lt2ptr;
				row = lt2ptr + 1;
				step = (float)(n - 1) * recip;
				pos = 0.0f;

				for (s = 0; s < smax; s++, pos += step)
				{
					int fi = (int)floor(pos);
					int ci = (int)ceil(pos);
					float frac = pos - (float)fi;
					int fi3 = fi * 3, ci3 = ci * 3;
					int rf, rc, gf, gc, bf, bc;
					unsigned r, g, b;

					rf = row[fi3+0]; rc = row[ci3+0];
					r = texgammatable[(int)((float)rf * (1.0f - frac) + (float)rc * frac + 0.5f)];
					gf = row[fi3+1]; gc = row[ci3+1];
					g = texgammatable[(int)((float)gf * (1.0f - frac) + (float)gc * frac + 0.5f)];
					bf = row[fi3+2]; bc = row[ci3+2];
					b = texgammatable[(int)((float)bf * (1.0f - frac) + (float)bc * frac + 0.5f)];

					blocklights[t * smax + s].r += r * scale;
					blocklights[t * smax + s].g += g * scale;
					blocklights[t * smax + s].b += b * scale;
				}

				lt2ptr += 1 + n * 3;
			}
		}
	}
	else if (cl.worldmodel->lightmap_mode == 3)
	{
		/* LT2 LERP 'a': decode each style's grid, apply lightgamma, accumulate */
		static color24 lt2_tmp[MAX_BLOCK_LIGHTS];
		byte* lt2ptr = (byte*)lightmap;

		for (maps = 0; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255; maps++)
		{
			int consumed;

			scale = d_lightstylevalue[psurf->styles[maps]];
			psurf->cached_light[maps] = (short)scale;

			consumed = DCV_LT2Decode(lt2ptr,
				(int)((cl.worldmodel->lightpayload + cl.worldmodel->lightBytes) - lt2ptr),
				lt2_tmp, smax, tmax);

			if (consumed == 0)
				break;

			for (i = 0; i < size; i++)
			{
				blocklights[i].r += LT2_LIGHTGAMMA(lt2_tmp[i].r) * scale;
				blocklights[i].g += LT2_LIGHTGAMMA(lt2_tmp[i].g) * scale;
				blocklights[i].b += LT2_LIGHTGAMMA(lt2_tmp[i].b) * scale;
			}
			lt2ptr += consumed;
		}
	}
}

/*
===============
DC_PackBlockLights

Pack blocklights to RGB565 and write into this surface's lightmap page,
at the block it was allocated.
===============
*/
static void DC_PackBlockLights( msurface_t* surf )
{
	unsigned short* dest16;
	byte*	dest;
	int*	src;
	int		smax, tmax, i, j, k, stride;
	int		c[3];
	colorVec* bl;

	dest = lightmaps
		+ surf->light_t * BLOCK_WIDTH * lightmap_bytes
		+ surf->light_s * lightmap_bytes;
	stride = BLOCK_WIDTH * lightmap_bytes;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	if (smax * tmax > MAX_BLOCK_LIGHTS)
		Sys_Error("Oversized surface in DC_SumBlockLights");

	bl = blocklights;

	for (i = 0; i < tmax; i++, dest += stride)
	{
		dest16 = (unsigned short*)dest;

		for (j = 0; j < smax; j++, bl++)
		{
			src = (int*)bl;
			for (k = 0; k < 3; k++, src++)
			{
				c[k] = *src >> 8;
				if (c[k] < 0)
					c[k] = 0;
				if (c[k] > 255)
					c[k] = 255;
			}

			c[0] >>= 3;
			c[1] >>= 2;
			c[2] >>= 3;

			*dest16++ = (unsigned short)(c[0] << 11 | c[1] << 5 | c[2]);
		}
	}
}

/*
===============
DC_SurfacePolyApplyBlockLights

Bake the current blocklights[] array into per-vertex diffuse colours stored at
poly->verts[i][3] (packed ARGB DWORD).
===============
*/
static void DC_SurfacePolyApplyBlockLights( msurface_t* surf )
{
	glpoly_t* p;
	int       i, smax, tmax, light_s16, light_t16;
	float*    v;
	unsigned r, g, b;

	if (!surf || !surf->polys)
		return;

	if (!surf->samples)
	{
		for (p = surf->polys; p; p = p->next)
			for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE)
				*(DWORD*)&v[3] = 0xFFFFFFFFu;
		return;
	}

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	light_s16 = (int)surf->light_s << 4;
	light_t16 = (int)surf->light_t << 4;

	for (p = surf->polys; p; p = p->next)
	{
		for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE)
		{
			int si = ((int)(v[6] * (BLOCK_WIDTH  * 16.0f) - (float)light_s16)) >> 4;
			int ti = ((int)(v[7] * (BLOCK_HEIGHT * 16.0f) - (float)light_t16)) >> 4;

			if (si < 0) si = 0; else if (si >= smax) si = smax - 1;
			if (ti < 0) ti = 0; else if (ti >= tmax) ti = tmax - 1;

			{
				colorVec* c = &blocklights[ti * smax + si];
				r = c->r >> 8; if (r > 255) r = 255;
				g = c->g >> 8; if (g > 255) g = 255;
				b = c->b >> 8; if (b > 255) b = 255;
				*(DWORD*)&v[3] = 0xFF000000u | (r << 16) | (g << 8) | b;
			}
		}
	}
}

/*
===============
R_BuildLightMap

Build the blocklights array for a given surface, pack it into the surface's
lightmap page, bake it into the poly's per-vertex colours, and reset any
decal tints cached on the surface back to their default.
===============
*/
void R_BuildLightMap( msurface_t* psurf )
{
	int smax, tmax;
	decal_t* pdecal;

	psurf->cached_dlight = (byte)(psurf->dlightbits & r_dlightactive);
	psurf->dlightbits &= r_dlightactive;

	smax = (psurf->extents[0] >> 4) + 1;
	tmax = (psurf->extents[1] >> 4) + 1;

	if (!r_fullbright.value && cl.worldmodel->lightdata)
	{
		DC_ClearBlockLights(smax, tmax);

		if (psurf->samples)
			DC_SumBlockLights(psurf, smax, tmax);

		if (psurf->dlightframe == (byte)r_framecount)
			R_AddDynamicLights(psurf);
	}
	else
	{
		DC_FullbrightBlockLights(smax, tmax);
	}

	DC_PackBlockLights(psurf);
	DC_SurfacePolyApplyBlockLights(psurf);

	pdecal = psurf->pdecals;
	while (pdecal && pdecal->psurface == psurf)
	{
		pdecal->color = 0xFAAA;
		pdecal = pdecal->pnext;
	}
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t* R_TextureAnimation( msurface_t* s )
{
	texture_t* base;
	int		reletive;
	int		count;
	static int rtable[20][20];
	int		tu, tv;

	base = s->texinfo->texture;

	if (!rtable[0][0])
	{
		for (tu = 0; tu < 20; tu++)
		{
			for (tv = 0; tv < 20; tv++)
			{
				rtable[tu][tv] = RandomLong(0, 0x7FFF);
			}
		}
	}

	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	if (base->name[0] == '-')
	{
		tu = (int)((s->texturemins[0] + (base->width << 16)) / base->width) % 20;
		tv = (int)((s->texturemins[1] + (base->height << 16)) / base->height) % 20;
		reletive = rtable[tu][tv] % base->anim_total;
	}
	else
	{
		reletive = (int)(cl.time * 10.0f) % base->anim_total;
	}

	count = 0;
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error("R_TextureAnimation: infinite cycle");
	}

	return base;
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

void GL_DisableMultitexture( void )
{
	DCV_DisableMultitexture();
}
/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly( glpoly_t* p )
{
	int		i;
	float*	v;
	vec3_t	nv;

	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		nv[0] = v[0] + 8 * sin(v[1] * 0.05f + realtime) * sin(v[2] * 0.05f + realtime);
		nv[1] = v[1] + 8 * sin(v[0] * 0.05f + realtime) * sin(v[2] * 0.05f + realtime);
		nv[2] = v[2];
	}
}

/*
================
DrawLightmapWaterPoly

The lightmap-pass counterpart of DrawGLWaterPoly: same warp, taken from the
second set of texture coordinates.
================
*/
void DrawLightmapWaterPoly( glpoly_t* p )
{
	int		i;
	float*	v;
	vec3_t	nv;

	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		nv[0] = v[0] + 8 * sin(v[1] * 0.05f + realtime) * sin(v[2] * 0.05f + realtime);
		nv[1] = v[1] + 8 * sin(v[0] * 0.05f + realtime) * sin(v[2] * 0.05f + realtime);
		nv[2] = v[2];
	}
}


/*
================
DrawGLPoly
================
*/
void DrawGLPoly( glpoly_t* p )
{
	DWORD diffuse = DCV_GetCurrentDiffuse();
	for (; p; p = p->next)
		if (p->numverts >= 3)
			DCV_AccumGLPoly(p->verts, p->numverts, diffuse, 0.0f);
}

/*
================
DCV_AccumLightmapPoly
================
*/
static void DCV_AccumLightmapPoly( glpoly_t* p )
{
	float clipped[SURF_CLIPPED_POLY_MAX_VERTS][VERTEXSIZE];
	int   clip_n, base, i, needed_indices;

	if (p->numverts < 3)
		return;

	clip_n = DCV_ClipPolyToNearPlane(
	            (const float (*)[VERTEXSIZE])p->verts, p->numverts, clipped);
	if (clip_n < 3)
		return;

	needed_indices = (clip_n - 2) * 3;
	if (!DCV_EnsureSpace(clip_n, needed_indices))
		return;

	base = DCV_GetVertCount();
	for (i = 0; i < clip_n; i++)
	{
		DCV_SetPackedColor(0xFFFFFFFFu);
		/* Use lightmap UVs ([6],[7]) not base texture UVs ([4],[5]). */
		DCV_AddVertex(clipped[i][0], clipped[i][1], clipped[i][2],
		              clipped[i][6], clipped[i][7]);
	}
	DCV_AddPolyIndices(base, clip_n);
	DCV_FlushIfLarge();
}

/*
================
R_BlendLightmaps

================
*/
void R_BlendLightmaps( void )
{
	int                i, j;
	glpoly_t          *p;
	glpoly_t          *p2;

	if (!gl_texsort || r_fullbright.value)
		return;

	// Nudge the near plane towards the eye so the lightmap pass lands exactly
	// on top of the base pass instead of z-fighting with it.
	R_ApplyViewModelProjection(g_frustum_zn - LIGHTMAP_DEPTH_NUDGE);

	if (gl_monolights.value)
		DCV_TexState_Blend();
	else
		DCV_TexState_Modulate();

	DCV_FlushApplyRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_EQUAL);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		p = lightmap_polys[i];
		if (!p)
			continue;

		GL_Bind(lm_texnum[i], 0);

		// Upload fresh lightmap data for surfaces with dynamic lights.
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = 0;
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BLOCK_WIDTH, BLOCK_HEIGHT,
				gl_lightmap_format, GL_UNSIGNED_BYTE,
				lightmaps + BLOCK_HEIGHT * BLOCK_WIDTH * lightmap_bytes);
		}

		for (; p; p = p->chain)
		{
			if (p->flags & SURF_DRAWBACKGROUND)
			{
				DrawLightmapWaterPoly(p);
			}
			else if (p->flags & SURF_DRAWTURB)
			{
				// Turbulent surfaces get their own warped pass in
				// R_DrawWaterChain, so there is nothing to submit here.
				for (p2 = p; p2; p2 = p2->next)
					for (j = 0; j < p2->numverts; j++)
						;
			}
			else if (!r_alphatestmode)
			{
				DCV_AccumLightmapBatch(p);
			}
		}

		DCV_Flush();
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);

	R_ApplyViewModelProjection(g_frustum_zn + LIGHTMAP_DEPTH_NUDGE);
}

/*
================
ScrollOffset
================
*/
float ScrollOffset( msurface_t* psurface, cl_entity_t* pEntity )
{
	float speed;
	float sOffset;

	sOffset = (float)(pEntity->rendercolor.b + (pEntity->rendercolor.g << 8)) / 16.0f;
	if (!pEntity->rendercolor.r)
		sOffset = -sOffset;

	speed = (1.0f / psurface->texinfo->texture->width) * sOffset * cl.time;

	if (speed < 0.0f)
		return fmod(speed, -1);
	else
		return fmod(speed, 1);
}

/*
================
DrawGLPolyScroll
================
*/
void DrawGLPolyScroll( msurface_t* psurface, cl_entity_t* pEntity )
{
	float sOffset;
	glpoly_t* p;

	sOffset = ScrollOffset(psurface, pEntity);
	{
		DWORD diffuse = DCV_GetCurrentDiffuse();

		if (psurface->texinfo && psurface->texinfo->texture &&
			psurface->texinfo->texture->anim_total == 1)
		{
			DCV_TexState_VertColor();
		}

		for (p = psurface->polys; p; p = p->next)
			if (p->numverts >= 3)
				DCV_AccumGLPoly(p->verts, p->numverts, diffuse, sOffset);
	}
}

/*
================
DrawGLSolidPoly
================
*/
void DrawGLSolidPoly( glpoly_t* p )
{
	DWORD diffuse = DCV_SurfColorFromEntity(currententity);
	for (; p; p = p->next)
		if (p->numverts >= 3)
			DCV_AccumGLPoly(p->verts, p->numverts, diffuse, 0.0f);
}


/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly( msurface_t* fa )
{
	texture_t*	t;
	int			maps;

	c_brush_polys++;

	if (fa->flags & SURF_DRAWSKY)
		return;

	t = R_TextureAnimation(fa);
	DCV_BindTexture(t->gl_texturenum);

	if (fa->flags & SURF_DRAWTURB)
	{
		EmitWaterPolys(fa, SIDE_FRONT);
		return;
	}

	if (fa->flags & SURF_UNDERWATER)
		DrawGLWaterPoly(fa->polys);
	else if (currententity && currententity->rendermode == kRenderTransColor)
	{
		GL_Bind(-1, 1);
		DrawGLSolidPoly(fa->polys);
	}
	else if (fa->flags & SURF_DRAWTILED)
	{
		DrawGLPolyScroll(fa, currententity);
	}
	else
	{
		DrawGLPoly(fa->polys);
	}

	if (fa->pdecals)
	{
		gDecalSurfs[gDecalSurfCount] = fa;
		gDecalSurfCount++;
		if (gDecalSurfCount > MAX_DECALSURFS)
			Sys_Error("Too many decal surfaces!\n");
	}

	if (fa->polys && !(fa->flags & (SURF_DRAWTURB | SURF_DRAWSKY | SURF_DRAWTILED)))
	{
		fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
		lightmap_polys[fa->lightmaptexturenum] = fa->polys;
	}

	// check for lightmap modification
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;
	}

	if (fa->dlightframe == (byte)r_framecount || fa->cached_dlight)
	{
dynamic:
		if (r_dynamic.value)
		{
			R_BuildLightMap(fa);
			lightmap_modified[fa->lightmaptexturenum] = 1;
		}
	}
}

/*
================
R_MirrorChain
================
*/
void R_MirrorChain( msurface_t* s )
{
	if (mirror)
		return;
	mirror = TRUE;
	mirror_plane = s->plane;
}

/*
================
R_DrawWaterChain
================
*/
void R_DrawWaterChain( msurface_t* pChain, int direction )
{
	msurface_t* s;
	glpoly_t*   p;
	texture_t*  t;
	float*      v;
	int         i, base;
	float       scale;
	float       os, ot, s_out, t_out, warp;
	vec3_t      tempVert;

	D_SetFadeColor(pChain->texinfo->texture->fade_r, pChain->texinfo->texture->fade_g,
		pChain->texinfo->texture->fade_b, pChain->texinfo->texture->fade_fog);

	t = R_TextureAnimation(pChain);
	DCV_BindTexture(t->gl_texturenum);

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_NONE);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);

	for (s = pChain; s; s = s->texturechain)
	{
		if (s->polys->verts[0][2] >= r_refdef.vieworg[2])
			scale = -currententity->scale;
		else
			scale = currententity->scale;

		DCV_FlushIfLarge();

		for (p = s->polys; p; p = p->next)
		{
			base = DCV_GetVertCount();
			DCV_AddIndicesFan(base, p->numverts);

			if (direction)
				v = p->verts[p->numverts - 1];
			else
				v = p->verts[0];

			for (i = 0; i < p->numverts; i++)
			{
				os = v[4];
				ot = v[5];

				VectorCopy(v, tempVert);
				warp = turbsin[(int)(cl.time * 160.0f + v[0] + v[1]) & 255] + 8.0f;
				warp += (turbsin[(int)(cl.time * 171.0f + v[0] * 5.0f - v[1]) & 255] + 8.0f) * 0.8f;
				tempVert[2] += warp * scale;

				s_out = (turbsin[(int)((ot * 0.125f + cl.time) * TURBSCALE) & 255] + os) * (1.0f / 64);
				t_out = (turbsin[(int)((os * 0.125f + cl.time) * TURBSCALE) & 255] + ot) * (1.0f / 64);

				DCV_AddVertexLit(s_out, t_out, tempVert);

				if (direction)
					v -= VERTEXSIZE;
				else
					v += VERTEXSIZE;
			}
		}
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);
}

/*
================
R_DrawSequentialPoly

Batch-draws one texture's whole surface chain. A chain that is uniformly
sky, water, or a scrolling (SURF_DRAWTILED) texture gets one specialised
pass; a chain mixing those flags falls back to the slow, unbatched
R_RenderBrushPoly path. Otherwise the chain is split into runs that share
the same animated texture frame, each accumulated with one bound texture
and flushed as a batch.
================
*/
void R_DrawSequentialPoly( msurface_t* chain )
{
	msurface_t* s;
	msurface_t* cur;
	msurface_t* next;
	msurface_t* deferred;
	texture_t*  t;
	byte        flagsOr, flagsAnd, special;
	void        (*pfnAccum)( const void* poly );
	int         maps;

	flagsOr = 0;
	flagsAnd = 0xFF;
	for (s = chain; s; s = s->texturechain)
	{
		flagsAnd &= s->flags;
		flagsOr  |= s->flags;
	}

	pfnAccum = r_alphatestmode ? DCV_AccumColoredPoly : DCV_AccumSolidPoly;

	special = flagsOr & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_DRAWTILED | SURF_DRAWBACKGROUND);
	if (special)
	{
		if ((flagsAnd & SURF_DRAWSKY) && (flagsOr & SURF_DRAWSKY))
			return;

		if ((flagsAnd & 0x74) == SURF_DRAWTURB && special == SURF_DRAWTURB)
		{
			R_DrawWaterChain(chain, 0);
			return;
		}

		if ((flagsAnd & 0x74) != SURF_DRAWTILED || special != SURF_DRAWTILED)
		{
			Con_Printf("Slow path of RenderBrushPoly %x %x\n", flagsAnd, special);
			for (s = chain; s; s = s->texturechain)
				R_RenderBrushPoly(s);
			return;
		}

		ScrollOffset(chain, currententity);
		pfnAccum = DCV_AccumScrollPoly;
	}

	if (currententity && currententity->rendermode == kRenderTransColor)
	{
		DCV_TexState_VertColor();
		DCV_SetColor(currententity->rendercolor.r, currententity->rendercolor.g,
			currententity->rendercolor.b, (int)(r_blend * 255.0f));
	}

	for (s = chain; s; )
	{
		t = R_TextureAnimation(s);
		DCV_BindTexture(t->gl_texturenum);

		deferred = NULL;
		for (cur = s; cur; cur = next)
		{
			next = cur->texturechain;

			if (!cur->texinfo->texture->anim_total || R_TextureAnimation(cur) == t)
			{
				c_brush_polys++;
				pfnAccum(cur->polys);

				if (gl_texsort)
				{
					cur->polys->chain = lightmap_polys[cur->lightmaptexturenum];
					lightmap_polys[cur->lightmaptexturenum] = cur->polys;
				}

				if (cur->pdecals)
				{
					gDecalSurfs[gDecalSurfCount] = cur;
					gDecalSurfCount++;
					if (gDecalSurfCount > MAX_DECALSURFS)
						Sys_Error("Too many decal surfaces!\n");
				}

				if (r_dynamic.value)
				{
					for (maps = 0; maps < MAXLIGHTMAPS && cur->styles[maps] != 255; maps++)
					{
						if (d_lightstylevalue[cur->styles[maps]] != cur->cached_light[maps])
							goto dynamic;
					}

					if (cur->dlightframe == (byte)r_framecount || cur->cached_dlight)
					{
dynamic:
						lightmap_modified[cur->lightmaptexturenum] = 1;
						R_BuildLightMap(cur);
					}
				}
			}
			else
			{
				cur->texturechain = deferred;
				deferred = cur;
			}
		}

		DCV_Flush();
		s = deferred;
	}
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains( void )
{
	int		i;
	msurface_t* s;
	texture_t* t;
	int iSounds;

	currententity = cl_entities;

	iSounds = 100;

	for (i = 0; i < cl.worldmodel->numtextures; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;

		if (i == skytexturenum)
		{
			R_DrawSkyChain(t->texturechain);
		}
		else
		{
			if ((s->flags & SURF_DRAWTURB) && r_wateralpha.value != 1.0f)
				goto skipped;	// draw translucent water later

			R_DrawSequentialPoly(t->texturechain);
		}

		t->texturechain = NULL;

		if (iSounds-- == 0)
		{
			S_UpdateAmbient();
			IN_Accumulate();
			iSounds = 100;
		}
skipped:
		;
	}

}

/*
=================
R_SetRenderMode
=================
*/
void R_SetRenderMode( cl_entity_t* pEntity )
{
	int rendermode;

	r_alphatestmode = 0;

	rendermode = pEntity->rendermode;

	if (rendermode == kRenderNormal)
	{
		DCV_SetColor(255, 255, 255, 255);
		DCV_TexState_Opaque();
	}
	else if (rendermode == kRenderTransColor)
	{
		DCV_TexState_Blend();
	}
	else if (rendermode == kRenderTransAdd)
	{
		DCV_TexState_Additive();
		DCV_SetColor((int)(r_blend * 255.0f), (int)(r_blend * 255.0f),
			(int)(r_blend * 255.0f), 255);
	}
	else if (rendermode == kRenderTransAlpha)
	{
		r_alphatestmode = 1;
		DCV_SetColor(255, 255, 255, 255);
		DCV_2D_SetupStates();
	}
	else
	{
		DCV_TexState_Blend();
		DCV_SetColor(255, 255, 255, (int)(r_blend * 255.0f));
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel( cl_entity_t* e )
{
	int			i;
	int			k;
	vec3_t		mins, maxs;
	msurface_t* psurf;
	float		dot;
	mplane_t*	pplane;
	model_t*	clmodel;
	qboolean	rotated;

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = TRUE;
		for (i = 0; i < 3; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = FALSE;
		VectorAdd(e->origin, clmodel->mins, mins);
		VectorAdd(e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;


	VectorSubtract(r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

// calculate dynamic lighting for bmodel if it's not an
// instanced model
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		for (k = 0; k < MAX_DLIGHTS; k++)
		{
			vec3_t saveOrigin;

			if ((cl_dlights[k].die < cl.time) ||
				(!cl_dlights[k].radius))
				continue;

			VectorCopy(cl_dlights[k].origin, saveOrigin);
			VectorSubtract(cl_dlights[k].origin, e->origin, cl_dlights[k].origin);

			R_MarkLights(&cl_dlights[k], 1 << k,
				clmodel->nodes + clmodel->hulls[0].firstclipnode);

			VectorCopy(saveOrigin, cl_dlights[k].origin);
		}
	}

	DCV_PushMatrix(D3DTRANSFORMSTATE_WORLD);
	DCV_SetClipRequired();

	memset(lightmap_polys, 0, sizeof(lightmap_polys));

	R_RotateForEntity(e);
	R_SetRenderMode(e);

	//
	// draw texture
	//
	{
#define MAX_BMODEL_CHAINS 50
		msurface_t* chains[MAX_BMODEL_CHAINS];
		int nchains = 0;
		int c;

		for (i = 0; i < clmodel->nummodelsurfaces; i++, psurf++)
		{
			qboolean bPass;

			pplane = psurf->plane;

			if (psurf->flags & SURF_DRAWTURB)
			{
				bPass = FALSE;
				if ((pplane->type == PLANE_Z || gl_watersides.value) &&
					(mins[2] + 1.0f < pplane->dist))
					bPass = TRUE;
			}
			else
			{
				dot = DotProduct(modelorg, g_planeNormalTable[pplane->normalindex].normal) - pplane->dist;

				bPass = (psurf->flags & SURF_PLANEBACK) ?
					(dot <= -BACKFACE_EPSILON) : (dot >= BACKFACE_EPSILON);
			}

			if (bPass && psurf->texinfo->texture)
			{
				for (c = 0; c < nchains; c++)
				{
					if (chains[c]->texinfo->texture == psurf->texinfo->texture)
					{
						psurf->texturechain = chains[c];
						chains[c] = psurf;
						goto next_surf;
					}
				}

				psurf->texturechain = NULL;
				if (nchains > MAX_BMODEL_CHAINS - 1)
					Sys_Error("Too many chains in brush model\n");
				chains[nchains++] = psurf;
			}
next_surf:;
		}

		for (c = 0; c < nchains; c++)
			R_DrawSequentialPoly(chains[c]);
	}

	if (e->rendermode == kRenderTransAdd)
	{
		if (!r_fullbright.value)
			R_BlendLightmaps();
	}
	else
	{
		R_DrawDecals();
		if (e->rendermode == kRenderNormal)
			R_BlendLightmaps();
	}

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,     TRUE);

	DCV_PopMatrix(D3DTRANSFORMSTATE_WORLD);
	DCV_SetNoClip();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/


/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode( mnode_t* node )
{
	int			c, side;
	mplane_t* plane;
	msurface_t* surf, ** mark;
	mleaf_t* pleaf;
	double		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBoxShort(node->minmaxs, node->minmaxs + 3))
		return;

	// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mleaf_t*)node;

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				*(volatile byte *)&((*mark)->visframe) = (byte)r_framecount;
				mark++;
			} while (--c);
		}

		// deal with model fragments in this leaf
		if (pleaf->efrags)
			R_StoreEfrags(&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(modelorg, g_planeNormalTable[plane->normalindex].normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode(node->children[side]);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = cl.worldmodel->surfaces + node->firstsurface;

		if (dot < 0 - BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			for (; c; c--, surf++)
			{
				if (surf->visframe != (byte)r_framecount)
					continue;

				// don't backface underwater surfaces, because they warp
				if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
					continue;		// wrong side

				// if sorting by texture, just store it out
				if (gl_texsort)
				{
					if (!mirror
						|| surf->texinfo->texture != cl.worldmodel->textures[mirrortexturenum])
					{
						surf->texturechain = surf->texinfo->texture->texturechain;
						surf->texinfo->texture->texturechain = surf;
					}
				}
			}
		}

	}

// recurse down the back side
	R_RecursiveWorldNode(node->children[!side]);
}



/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld( void )
{
	cl_entity_t ent;

	memset(&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy(r_refdef.vieworg, modelorg);

	currententity = &ent;

	ent.rendercolor.r = gWaterColor.r;
	ent.rendercolor.g = gWaterColor.g;
	ent.rendercolor.b = gWaterColor.b;

	DCV_SetColor(255, 255, 255, 255);

	memset(lightmap_polys, 0, sizeof(lightmap_polys));

	R_ClearSkyBox();

	// The world is drawn with wrapping texture coordinates; everything after it
	// wants them clamped again.
	DCV_SetTextureWrap();

	R_RecursiveWorldNode(cl.worldmodel->nodes);
	gDecalSurfCount = 0;

	DrawTextureChains();

	DCV_SetTextureClamp();

	S_UpdateAmbient();
	IN_Accumulate();

	R_DrawDecals();

	R_BlendLightmaps();
}

/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves( void )
{
	byte* vis;
	mnode_t* node;
	int		i;
	byte	solid[4096];
	int		marked_leafs = 0;

	if (r_oldviewleaf == r_viewleaf && !r_novis.value)
		return;

	if (mirror)
		return;

	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value)
	{
		vis = solid;
		memset(solid, 0xff, (cl.worldmodel->numleafs + 7) >> 3);
	}
	else
		vis = Mod_LeafPVS(r_viewleaf, cl.worldmodel);

	for (i = 0; i < cl.worldmodel->numleafs; i++)
	{
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			node = (mnode_t*)&cl.worldmodel->leafs[i + 1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				*(volatile int *)&node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

/*
========================
LM_UploadBlock

Hand the block that has just been filled in to the texture manager and move on
to the next one.
========================
*/
static void LM_UploadBlock( void )
{
	char	name[36];
	int		texnum;
	int		texture;

	texnum = active_lightmaps++;

	if (!gl_texsort)
		return;

	sprintf(name, "lightmap.%d.%d", numgltextures, active_lightmaps);

	if (lm_texnum[texnum])
		return;

	texture = DC_LoadTexture(name, GLT_WORLD, BLOCK_WIDTH, BLOCK_HEIGHT,
		lightmaps, 0, TEX_TYPE_RGB565_RAW, NULL);

	if (texnum && !texture)
		lm_texnum[texnum] = lm_texnum[0];

	lm_texnum[texnum] = texture;
}

/*
========================
AllocBlock

Returns a texture number and the position inside it.
========================
*/
static int AllocBlock( int w, int h, int* x, int* y )
{
	int		i, j;
	int		best, best2;

	for ( ; ; )
	{
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++)
		{
			best2 = 0;

			for (j = 0; j < w; j++)
			{
				if (allocated[i + j] >= best)
					break;
				if (allocated[i + j] > best2)
					best2 = allocated[i + j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h <= BLOCK_HEIGHT)
			break;

		// No room left in this block -- upload it and start a fresh one.
		LM_UploadBlock();
		memset(allocated, 0, sizeof(allocated));
	}

	for (i = 0; i < w; i++)
		allocated[*x + i] = best + h;

	return active_lightmaps;
}


mvertex_t* r_pcurrentvertbase;
model_t* currentmodel;

int	nColinElim;

/*
================
DC_BuildSurfaceDisplayList
================
*/
static void DC_BuildSurfaceDisplayList( msurface_t* fa )
{
	int			i, lindex, lnumverts;
	medge_t*	pedges, * r_pedge;
	int			vertpage;
	float*		vec;
	float		s, t;
	glpoly_t* poly;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	//
	// draw texture
	//
	poly = (glpoly_t*)Hunk_Alloc(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy(vec, poly->verts[i]);
		*(DWORD*)&poly->verts[i][3] = 0xFFFFFFFFu;
		poly->verts[i][4] = s;
		poly->verts[i][5] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

		poly->verts[i][6] = s;
		poly->verts[i][7] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER))
	{
		for (i = 0; i < lnumverts; ++i)
		{
			vec3_t v1, v2;
			float* prev, * thisPoint, * next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			thisPoint = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract(thisPoint, prev, v1);
			VectorNormalize(v1);
			VectorSubtract(next, prev, v2);
			VectorNormalize(v2);

			// skip co-linear points
#define COLINEAR_EPSILON 0.001f
			if ((fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
				(fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) &&
				(fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;
}

/*
========================
DC_CreateSurfaceLightmap
========================
*/
static void DC_CreateSurfaceLightmap( msurface_t* surf )
{
	int		smax, tmax;
	int ls, lt;
	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	if ((surf->flags & SURF_DRAWTILED) && (surf->texinfo->flags & TEX_SPECIAL))
		return;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;


	surf->lightmaptexturenum = (byte)AllocBlock(smax, tmax, &ls, &lt);
	surf->light_s = (byte)ls;
	surf->light_t = (byte)lt;

	R_BuildLightMap(surf);
}

/*
==================
DC_BuildLightmaps
==================
*/
void DC_BuildLightmaps( void )
{
	int		i, j;
	model_t* m;

	active_lightmaps = 0;
	memset(lm_texnum, 0, sizeof(lm_texnum));
	memset(allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	gl_lightmap_format = GL_RGBA;
	lightmap_bytes = 2;

	for (j = 1; j < MAX_MODELS; j++)
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i = 0; i < m->numsurfaces; i++)
		{
			DC_CreateSurfaceLightmap(m->surfaces + i);
			if (m->surfaces[i].flags & SURF_DRAWTURB)
				continue;
			DC_BuildSurfaceDisplayList(m->surfaces + i);
			DC_SurfacePolyApplyBlockLights(m->surfaces + i);
		}
	}

	// Upload whatever is left in the block that was being filled in.
	LM_UploadBlock();
}
//-----------------------------------------------------------------------------
//
// Decal system
//
//-----------------------------------------------------------------------------

// UNDONE: Compress this???  256K here?
static decal_t			gDecalPool[MAX_DECALS];
static int				gDecalCount;					// Pool index
static vec3_t			gDecalPos;

static model_t*			gDecalModel = NULL;
static texture_t*		gDecalTexture = NULL;
static int				gDecalSize, gDecalIndex;
static int				gDecalFlags, gDecalEntity;

int R_DecalUnProject( decal_t* pdecal, vec_t* position );
void R_DecalCreate( msurface_t* psurface, int textureIndex, float scale, float x, float y );
void R_DecalShoot( int textureIndex, int entity, int modelIndex, vec_t* position, int flags );
void R_InvalidateSurface( msurface_t* surface );

#define DECAL_DISTANCE			4

// Empirically determined constants for minimizing overalpping decals
#define MAX_OVERLAP_DECALS		4
#define DECAL_OVERLAP_DIST		8


// Init the decal pool
void R_DecalInit( void )
{
	memset(gDecalPool, 0, sizeof(gDecalPool));
	gDecalCount = 0;
}


// Unlink pdecal from any surface it's attached to
void R_DecalUnlink( decal_t* pdecal )
{
	decal_t* tmp;

	if (pdecal->psurface)
	{
		if (pdecal->psurface->pdecals == pdecal)
		{
			pdecal->psurface->pdecals = pdecal->pnext;
		}
		else
		{
			tmp = pdecal->psurface->pdecals;
			if (!tmp)
			{
				Sys_Error("Bad decal list");
				return;
			}

			while (tmp->pnext)
			{
				if (tmp->pnext == pdecal)
				{
					tmp->pnext = pdecal->pnext;
					break;
				}

				tmp = tmp->pnext;
			}
		}
	}

	pdecal->psurface = NULL;
}


// Just reuse next decal in list
// A decal that spans multiple surfaces will use multiple decal_t pool entries, as each surface needs
// it's own.
decal_t* R_DecalAlloc( decal_t* pdecal )
{
	if (!pdecal)
	{
		int count;

		count = 0;		// Check for the odd possiblity of infinte loop
		do
		{
			gDecalCount++;
			if (gDecalCount >= MAX_DECALS)
				gDecalCount = 0;
			pdecal = gDecalPool + gDecalCount;	// reuse next decal
			count++;
		} while ((pdecal->flags & FDECAL_PERMANENT) && count < MAX_DECALS);
	}

	// If decal is already linked to a surface, unlink it.
	R_DecalUnlink(pdecal);

	return pdecal;
}


// remove all decals
void R_DecalRemoveAll( int textureIndex )
{
	int i;
	decal_t* pDecal;

	for (i = 0; i < MAX_DECALS; i++)
	{
		pDecal = &gDecalPool[i];

		if (pDecal->texture == textureIndex)
		{
			R_DecalUnlink(pDecal);
			memset(pDecal, 0, sizeof(decal_t));
		}
	}
}

// iterate over all surfaces on a node, looking for surfaces to decal
void R_DecalNode( mnode_t* node )
{
	mplane_t* splitplane;
	float		dist;

	if (!node)
		return;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct(gDecalPos, g_planeNormalTable[splitplane->normalindex].normal) - splitplane->dist;

	// This is arbitrarily set to 10 right now.  In an ideal world we'd have the 
	// exact surface but we don't so, this tells me which planes are "sort of 
	// close" to the gunshot -- the gunshot is actually 4 units in front of the 
	// wall (see dlls\weapons.cpp). We also need to check to see if the decal 
	// actually intersects the texture space of the surface, as this method tags
	// parallel surfaces in the same node always.
	// JAY: This still tags faces that aren't correct at edges because we don't 
	// have a surface normal

	if (dist > gDecalSize)
	{
		R_DecalNode(node->children[0]);
	}
	else if (dist < -gDecalSize)
	{
		R_DecalNode(node->children[1]);
	}
	else
	{
		if (dist < DECAL_DISTANCE && dist > -DECAL_DISTANCE)
		{
			int			w, h;
			float		s, t, scale;
			msurface_t* surf;
			int			i;
			mtexinfo_t* tex;

			surf = gDecalModel->surfaces + node->firstsurface;

			// iterate over all surfaces in the node
			for (i = 0; i < node->numsurfaces; i++, surf++)
			{
				if (surf->flags & (SURF_DRAWTILED | SURF_DRAWTURB))
					continue;

				tex = surf->texinfo;

				scale = VectorLength(tex->vecs[0]);
				if (scale == 0)
					continue;

				// project decal center into the texture space of the surface
				s = DotProduct(gDecalPos, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
				t = DotProduct(gDecalPos, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

				w = gDecalTexture->width * scale;
				h = gDecalTexture->height * scale;

				// move s,t to upper left corner
				s -= (w * 0.5f);
				t -= (h * 0.5f);

				if (s <= -w || t <= -h ||
					s > (surf->extents[0] + w) || t > (surf->extents[1] + h))
				{
					continue; // nope
				}

				scale = 1.0f / scale;
				s = (surf->texturemins[0] + s) / (float)tex->texture->width;
				t = (surf->texturemins[1] + t) / (float)tex->texture->height;

				// stamp it
				R_DecalCreate(surf, gDecalIndex, scale, s, t);
			}
		}

		R_DecalNode(node->children[0]);
		R_DecalNode(node->children[1]);
	}
}

int DecalListAdd( DECALLIST* pList, int count )
{
	int			i;
	vec3_t		tmp;
	DECALLIST* pdecal;

	pdecal = pList + count;
	for (i = 0; i < count; i++)
	{
		if (pdecal->name == pList[i].name &&
			pdecal->entityIndex == pList[i].entityIndex)
		{
			VectorSubtract(pdecal->position, pList[i].position, tmp);	// Merge
			if (VectorLength(tmp) < 2)	// UNDONE: Tune this '2' constant
				return count;
		}
	}

	// This is a new decal
	return count + 1;
}


typedef int (*qsortFunc_t)( const void *, const void * );

int DecalDepthCompare( const DECALLIST* elem1, const DECALLIST* elem2 )
{
	if (elem1->depth > elem2->depth)
		return -1;
	if (elem1->depth < elem2->depth)
		return 1;

	return 0;
}

int DecalListCreate( DECALLIST* pList )
{
	int total = 0;
	int i, depth;

	for (i = 0; i < MAX_DECALS; i++)
	{
		decal_t* decal = &gDecalPool[i];
		decal_t* pdecals;
		texture_t* ptexture;

		// Decal is in use and is not a custom decal
		if (!decal->psurface || (decal->flags & FDECAL_CUSTOM))
			continue;

		// compute depth
		pdecals = decal->psurface->pdecals;
		depth = 0;
		while (pdecals && pdecals != decal)
		{
			depth++;
			pdecals = pdecals->pnext;
		}
		pList[total].depth = depth;
		pList[total].flags = decal->flags;

		pList[total].entityIndex = R_DecalUnProject(decal, pList[total].position);

		ptexture = Draw_DecalTexture(decal->texture);
		pList[total].name = ptexture->name;

		// Check to see if the decal should be addedo
		total = DecalListAdd(pList, total);
	}

	// Sort the decals lowest depth first, so they can be re-applied in order
	qsort(pList, total, sizeof(DECALLIST), (qsortFunc_t)DecalDepthCompare);

	return total;
}
// ---------------------------------------------------------

int R_DecalUnProject( decal_t* pdecal, vec_t* position )
{
	float s, t;
	float scale;
	float inverseScale;
	mtexinfo_t* pTexinfo;
	texture_t* ptexture;
	int entityIndex = 0;

	if (!pdecal || !pdecal->psurface)
		return -1;

	pTexinfo = pdecal->psurface->texinfo;

	s = (float)pTexinfo->texture->width * pdecal->dx - (float)pdecal->psurface->texturemins[0];
	t = (float)pTexinfo->texture->height * pdecal->dy - (float)pdecal->psurface->texturemins[1];

	scale = VectorLength(pTexinfo->vecs[0]) * 0.5f;
	ptexture = Draw_DecalTexture(pdecal->texture);

	s = (float)ptexture->width * scale + s + (float)pdecal->psurface->texturemins[0] - pTexinfo->vecs[0][3];
	t = (float)ptexture->height * scale + t + (float)pdecal->psurface->texturemins[1] - pTexinfo->vecs[1][3];

	inverseScale = fabs(VectorLength(pTexinfo->vecs[0]));

	if (inverseScale != 0.0f)
		inverseScale = (1.0f / inverseScale) * (1.0f / inverseScale);

	VectorScale(pTexinfo->vecs[0], s * inverseScale, position);

	VectorMA(position, t * inverseScale, pTexinfo->vecs[1], position);
	VectorMA(position, pdecal->psurface->plane->dist, pdecal->psurface->plane->normal, position);

	entityIndex = pdecal->entityIndex;

	if (pdecal->entityIndex)
	{
		hull_t* phull;
		vec3_t temp;
		edict_t* pEdict;
		model_t* pModel = NULL;

		pEdict = &sv.edicts[entityIndex];
		if (pEdict->v.modelindex)
			pModel = sv.models[pEdict->v.modelindex];

		// Make sure it's a brush model
		if (!pModel || pModel->type != mod_brush)
			return 0;

		if (pEdict->v.angles[0] || pEdict->v.angles[1] || pEdict->v.angles[2])
		{
			vec3_t forward, right, up;
			AngleVectorsTranspose(pEdict->v.angles, forward, right, up);

			VectorCopy(position, temp);
			position[0] = DotProduct(temp, forward);
			position[1] = DotProduct(temp, right);
			position[2] = DotProduct(temp, up);
		}

		if (pModel->firstmodelsurface)
		{
			phull = &pModel->hulls[0]; // always use #0 hull
			VectorAdd(pEdict->v.origin, phull->clip_mins, temp);
			VectorAdd(temp, position, position);
		}
	}

	return entityIndex;
}


// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
void R_DecalShoot_( texture_t* ptexture, int index, int entity, int modelIndex, vec_t* position, int flags )
{
	mnode_t* pnodes;
	cl_entity_t* pent;

	VectorCopy(position, gDecalPos);	// Pass position in global

	pent = &cl_entities[entity];

	// Try all ways to get the model
	if (pent)
	{
		gDecalModel = pent->model;
		if (!gDecalModel)
		{
			if (modelIndex)
				gDecalModel = cl.model_precache[modelIndex];

			if (!gDecalModel)
			{
				if (sv.active)
					gDecalModel = sv.models[sv.edicts[entity].v.modelindex];
			}
		}
	}
	else
	{
		gDecalModel = NULL;
	}

	if (!pent || !gDecalModel || gDecalModel->type != mod_brush || !ptexture)
	{
		Con_DPrintf("Decals must hit mod_brush!\n");
		return;
	}

	pnodes = gDecalModel->nodes;

	if (entity)
	{
		hull_t* phull;
		vec3_t temp;

		if (gDecalModel->firstmodelsurface)
		{
			phull = &gDecalModel->hulls[0]; // always use #0 hull

			VectorSubtract(position, phull->clip_mins, temp);
			VectorSubtract(temp, pent->origin, gDecalPos);
			pnodes = pnodes + phull->firstclipnode;
		}

		if (pent->angles[0] || pent->angles[1] || pent->angles[2])
		{
			vec3_t forward, right, up;
			AngleVectors(pent->angles, forward, right, up);
			VectorCopy(gDecalPos, temp);

			gDecalPos[0] = DotProduct(temp, forward);
			gDecalPos[1] = -DotProduct(temp, right);
			gDecalPos[2] = DotProduct(temp, up);
		}
	}

	// More state used by R_DecalNode()
	gDecalEntity = entity;
	gDecalTexture = ptexture;
	gDecalIndex = index;
	gDecalFlags = flags;
	gDecalSize = ptexture->width >> 1;

	if (gDecalSize < (int)(ptexture->height >> 1))
		gDecalSize = ptexture->height >> 1;

	R_DecalNode(pnodes);
}

// Shoots a decal onto the surface of the BSP.  position is the center of the decal in world coords
// This is called from cl_parse.c, cl_tent.c
void R_DecalShoot( int textureIndex, int entity, int modelIndex, vec_t* position, int flags )
{
	texture_t* ptexture;

	ptexture = Draw_DecalTexture(textureIndex);
	R_DecalShoot_(ptexture, textureIndex, entity, modelIndex, position, flags);
}

void R_CustomDecalShoot( texture_t* ptexture, int playernum, int entity, int modelIndex, vec_t* position, int flags )
{
	int plindex = ~playernum;
	R_DecalShoot_(ptexture, plindex, entity, modelIndex, position, flags);
}

// Check for intersecting decals on this surface
decal_t* R_DecalIntersect( msurface_t* psurf, int* pcount, float x, float y )
{
	decal_t* plist;
	decal_t* plast;
	int			dist;
	int			lastDist;
	int			dx, dy;
	texture_t* ptexture;
	float		w, h;
	float		maxWidth;
	qboolean	bPermanent;

	plast = NULL;

	lastDist = 0xFFFF;
	*pcount = 0;

	maxWidth = (float)(gDecalTexture->width) * 1.5f;

	plist = psurf->pdecals;
	while (plist)
	{
		ptexture = Draw_DecalTexture(plist->texture);

		// Don't steal bigger decals and replace them with smaller decals
		// Don't steal permanent decals
		bPermanent = (plist->flags & FDECAL_PERMANENT);
		if (!bPermanent)
		{
			if (maxWidth >= (float)ptexture->width)
			{
				w = abs((int)((gDecalTexture->width >> 1) + psurf->texinfo->texture->width * x
					- (psurf->texinfo->texture->width * plist->dx + (ptexture->width >> 1))));
				h = abs((int)((gDecalTexture->height >> 1) + psurf->texinfo->texture->height * y
					- (psurf->texinfo->texture->height * plist->dy + (ptexture->height >> 1))));

				// Now figure out the part of the projection that intersects plist's
				// clip box [0,0,1,1].
				if (h <= w)
				{
					dx = w;
					dy = h;
				}
				else
				{
					dx = h;
					dy = w;
				}

				// Figure out how much of this intersects the (0,0) - (1,1) bbox
				dist = (float)dx + (float)dy * 0.5f;
				if ((dist * plist->scale) < 8)
				{
					*pcount += 1;

					if (!plast || dist <= lastDist)
					{
						lastDist = dist;
						plast = plist;
					}
				}
			}
		}

		plist = plist->pnext;
	}

	return plast;
}

// Allocate and initialize a decal from the pool, on surface with offsets x, y
void R_DecalCreate( msurface_t* psurface, int textureIndex, float scale, float x, float y )
{
	decal_t* pdecal;
	decal_t* pold;
	int				count;

	pold = R_DecalIntersect(psurface, &count, x, y);

	if (count < MAX_OVERLAP_DECALS)
		pold = NULL;

	pdecal = R_DecalAlloc(pold);
	pdecal->texture = textureIndex;
	pdecal->flags = gDecalFlags;
	pdecal->dx = x;
	pdecal->dy = y;
	pdecal->pnext = NULL;

	if (psurface->pdecals)
	{
		pold = psurface->pdecals;

		while (pold->pnext)
			pold = pold->pnext;

		pold->pnext = pdecal;
	}
	else
	{
		psurface->pdecals = pdecal;
	}

	// Tag surface
	pdecal->psurface = psurface;

	// Set scaling
	pdecal->scale = scale;
	pdecal->entityIndex = gDecalEntity;	
	pdecal->color = 0xFFFF;

	R_InvalidateSurface(psurface);
}

// clip edges
#define LEFT_EDGE			0
#define RIGHT_EDGE			1
#define TOP_EDGE			2
#define BOTTOM_EDGE			3

// Quick and dirty sutherland Hodgman clipper
// Clip polygon to decal in texture space
// JAY: This code is lame, change it later.  It does way too much work per frame
// It can be made to recursively call the clipping code and only copy the vertex list once
int Inside( float* vert, int edge )
{
	switch (edge)
	{
		case LEFT_EDGE:
			if (vert[4] > 0.0f)
				return 1;
			return 0;

		case RIGHT_EDGE:
			if (vert[4] < 1.0f)
				return 1;
			return 0;

		case TOP_EDGE:
			if (vert[5] > 0.0f)
				return 1;
			return 0;

		case BOTTOM_EDGE:
			if (vert[5] < 1.0f)
				return 1;
			return 0;
	}

	return 0;
}

void Intersect( float* one, float* two, int edge, float* out )
{
	float t;

	// vert[4] is decal u, vert[5] is decal v
	// vert[0..2] is X, Y, Z

	if (edge < TOP_EDGE)
	{
		if (edge == LEFT_EDGE)
		{
			t = ((one[4] - 0) / (one[4] - two[4]));
			out[4] = 0;
		}
		else
		{
			t = ((one[4] - 1) / (one[4] - two[4]));
			out[4] = 1;
		}

		out[5] = one[5] + (two[5] - one[5]) * t;
	}
	else
	{
		if (edge == TOP_EDGE)
		{
			t = ((one[5] - 0) / (one[5] - two[5]));
			out[5] = 0;
		}
		else
		{
			t = ((one[5] - 1) / (one[5] - two[5]));
			out[5] = 1;
		}

		out[4] = one[4] + (two[4] - one[4]) * t;
	}

	out[0] = one[0] + (two[0] - one[0]) * t;
	out[1] = one[1] + (two[1] - one[1]) * t;
	out[2] = one[2] + (two[2] - one[2]) * t;
}

int SHClip( float* vert, int vertCount, float* out, int edge )
{
	int j, outCount;
	float* s, * p;


	outCount = 0;

	s = &vert[(vertCount - 1) * VERTEXSIZE];
	for (j = 0; j < vertCount; j++)
	{
		p = &vert[j * VERTEXSIZE];
		if (Inside(p, edge))
		{
			if (Inside(s, edge))
			{
				memcpy(out, p, sizeof(*out) * VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
			else
			{
				Intersect(s, p, edge, out);
				out += VERTEXSIZE;
				outCount++;
				memcpy(out, p, sizeof(*out) * VERTEXSIZE);
				outCount++;
				out += VERTEXSIZE;
			}
		}
		else
		{
			if (Inside(s, edge))
			{
				Intersect(p, s, edge, out);
				out += VERTEXSIZE;
				outCount++;
			}
		}
		s = p;
	}

	return outCount;
}

#define MAX_DECALCLIPVERT		32
#define MAX_DECAL_CHAINS		50

extern void R_ApplySceneProjection( void );

/*
==================
R_DecalComputeVertices

Compute decal vertices from surface polygon, project UV, SH-clip to [0,1].
Returns vertex count after clipping. Output in vert[].
==================
*/
static int R_DecalComputeVertices(
	decal_t* plist,
	msurface_t* psurf,
	texture_t* ptexture,
	float vert[MAX_DECALCLIPVERT][VERTEXSIZE],
	float outvert[MAX_DECALCLIPVERT][VERTEXSIZE] )
{
	float  scalex, scaley;
	float* v;
	int    j, outCount;

	scalex = (psurf->texinfo->texture->width * plist->scale) / (float)ptexture->width;
	scaley = (psurf->texinfo->texture->height * plist->scale) / (float)ptexture->height;

	v = psurf->polys->verts[0];
	for (j = 0; j < psurf->polys->numverts; j++, v += VERTEXSIZE)
	{
		VectorCopy(v, vert[j]);
		vert[j][4] = (v[4] - plist->dx) * scalex;
		vert[j][5] = (v[5] - plist->dy) * scaley;

		if (plist->flags & FDECAL_HFLIP)
			vert[j][4] = 1.0f - vert[j][4];

		if (plist->flags & FDECAL_VFLIP)
			vert[j][5] = 1.0f - vert[j][5];
	}

	outCount = SHClip(vert[0], psurf->polys->numverts, outvert[0], LEFT_EDGE);
	outCount = SHClip(outvert[0], outCount, vert[0], RIGHT_EDGE);
	outCount = SHClip(vert[0], outCount, outvert[0], TOP_EDGE);
	outCount = SHClip(outvert[0], outCount, vert[0], BOTTOM_EDGE);

	/* Check if decal can be flagged FDECAL_NOCLIP (full quad, skip future clipping) */
	if (outCount && (plist->flags & FDECAL_CLIPTEST) && outCount == 4)
	{
		int kk;
		qboolean isFullQuad = 1;
		plist->flags &= ~FDECAL_CLIPTEST;
		for (kk = 0; kk < 4 && isFullQuad; kk++)
		{
			float u = vert[kk][4];
			float vv = vert[kk][5];
			if ((u != 0.0f && u != 1.0f) || (vv != 0.0f && vv != 1.0f))
				isFullQuad = 0;
		}
		if (isFullQuad)
			plist->flags |= FDECAL_NOCLIP;
	}

	return outCount;
}

/*
==================
R_DecalColor4444to32

==================
*/
static DWORD R_DecalColor4444to32( unsigned short color )
{
	int a = ((color >> 12) & 0xF);
	int r = ((color >> 8) & 0xF);
	int g = ((color >> 4) & 0xF);
	int b = ((color) & 0xF);

	a |= (a << 4);
	r |= (r << 4);
	g |= (g << 4);
	b |= (b << 4);

	return (DWORD)((a << 24) | (r << 16) | (g << 8) | b);
}

/*
==================
R_DrawDecals

==================
*/
void R_DrawDecals( void )
{
	float vert[MAX_DECALCLIPVERT][VERTEXSIZE];
	float outvert[MAX_DECALCLIPVERT][VERTEXSIZE];

	decal_t* chains[MAX_DECAL_CHAINS];
	int      numChains = 0;
	decal_t* plist;
	int      i, j, k, outCount;
	texture_t* ptexture;
	msurface_t* psurf;

	if (gDecalSurfCount == 0)
		return;

	DCV_TexState_Blend();
	R_ApplySceneProjection();

	for (i = 0; i < gDecalSurfCount; i++)
	{
		psurf = gDecalSurfs[i];

		for (plist = psurf->pdecals; plist; plist = plist->pnext)
		{
			qboolean found = 0;

			for (j = 0; j < numChains; j++)
			{
				if (chains[j]->texture == plist->texture)
				{
					/* Insert at head via chain_next overlay */
					plist->chain_next = chains[j];
					chains[j] = plist;
					found = 1;
					break;
				}
			}

			if (!found)
			{
				plist->chain_next = NULL;
				chains[numChains] = plist;
				numChains++;
				if (numChains > MAX_DECAL_CHAINS)
					Sys_Error("Too many chains in R_DrawDecals");
			}
		}
	}

	for (i = 0; i < numChains; i++)
	{
		plist = chains[i];

		ptexture = Draw_DecalTexture(plist->texture);
		GL_Bind(ptexture->gl_texturenum, 0);
		DCV_FlushIfLarge();

		for (; plist; plist = plist->chain_next)
		{
			outCount = R_DecalComputeVertices(
				plist, plist->psurface, ptexture,
				vert, outvert);

			if (outCount)
			{
				int   base;
				float* vlist;
				DWORD  color32 = R_DecalColor4444to32(plist->color);

				DCV_SetPackedColor(color32);

				base = DCV_GetVertCount();
				DCV_AddIndicesFan(base, outCount);

				vlist = vert[0];
				for (k = 0; k < outCount; k++, vlist += VERTEXSIZE)
				{
					DCV_AddVertex(vlist[0], vlist[1], vlist[2],
					              vlist[4], vlist[5]);
				}
			}
		}
	}

	gDecalSurfCount = 0;
	R_ApplySceneProjection();
	DCV_SetPackedColor(0xFFFFFFFF);
}



void R_InvalidateSurface( msurface_t* surface )
{
}
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

#pragma intrinsic(fabsf)

#define MAX_DECALSURFS		200

int		lightmap_bytes;		// 1, 2, or 4
int	lightmap_used;
#define MAX_BLOCK_LIGHTS	(18 * 18)
colorVec blocklights[MAX_BLOCK_LIGHTS];

// Horizontal resample weights for one row of a surface's lightmap; the same
// for every row, so they are only worked out once per light style.
#define MAX_LIGHTMAP_WIDTH	20

typedef struct
{
	int		index;
	float	w0;
	float	w1;
} lmcolumn_t;

static lmcolumn_t lm_column[MAX_LIGHTMAP_WIDTH];

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

// Projection-scale offsets that put lightmaps and decals just in front of the
// base pass without changing the shared clip planes.
#define LIGHTMAP_DEPTH_NUDGE	0.0005f

// Brush models cycle through this many projection-scale steps so that
// co-planar ones do not fight each other for depth.
#define BMODEL_DEPTH_SLOTS	64
#define BMODEL_DEPTH_STEP	0.005f

// Decals sit just in front of the surface they are stuck to.
#define DECAL_DEPTH_NUDGE	0.00025f

extern int r_depthslot;

extern float g_frustum_zn;
void R_ApplyViewModelProjection( float zn );

extern int numgltextures;
extern int nada_texture;

void R_RenderDynamicLightmaps( msurface_t* fa );
void DrawGLSolidPoly( glpoly_t* p );
void DrawLightmapWaterPoly( glpoly_t* p );
void DrawGLWaterPoly( glpoly_t* p );

float ScrollOffset( msurface_t* psurface, cl_entity_t* pEntity );

#define SURF_NEAR_CLIP_DIST   4.0f
#define SURF_CLIPPED_POLY_MAX_VERTS 64

// -----------------------------------------------------------------------------
// D3D L-vertex batch (world + studio)
// -----------------------------------------------------------------------------

#define SURF_MAX_VERTS       2048
#define SURF_MAX_INDICES     8192

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
		dist = DotProduct(impact, g_planeNormalTable[surf->plane->normalindex].normal)
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
DC_FullbrightBlockLights

Fill blocklights with full bright (255).
===============
*/
static void DC_FullbrightBlockLights( int size )
{
	int i;

	if (size > MAX_BLOCK_LIGHTS)
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
static void DC_ClearBlockLights( int size )
{
	int i;

	if (size > MAX_BLOCK_LIGHTS)
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
#define LT2_LIGHTGAMMA(j) ((unsigned)g_GammaTable256[(unsigned)(j)])

// floatmathlib.h only fast-paths `floor`/`fceil`, not `ceil` -- so a bare
// `ceil()` call falls through to the real double-precision libm routine.
// Declare it so the compiler emits a proper double-returning call for it
// instead of assuming an int-returning implicit declaration.
extern double ceil( double x );

static void DC_SumBlockLights( msurface_t* psurf, int smax, int tmax )
{
	int i, maps, s, t;
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

			i = 0;
			for (t = 0; t < tmax; t++)
			{
				for (s = 0; s < smax; s++)
				{
					unsigned short v = *p++;

					if (v & LT2D_DELTA_FLAG)
					{
						int d;

						if (v & LT2D_R_SIGN)
							r -= (v & LT2D_R_MAG_MASK) >> 10;
						else
							r += (v & LT2D_R_MAG_MASK) >> 10;

						d = v & LT2D_GB_SIGN;

						if (d)
							g -= (v & LT2D_G_MAG_MASK) >> 5;
						else
							g += (v & LT2D_G_MAG_MASK) >> 5;

						if (d)
							b -= v & LT2D_B_MAG_MASK;
						else
							b += v & LT2D_B_MAG_MASK;
					}
					else
					{
						r = (v & LT2D_R_MASK_ABS) >> 7;
						g = (v & LT2D_G_MASK_ABS) >> 2;
						b = (v & LT2D_B_MASK_ABS) << 3;
					}

					blocklights[i].r += r * scale;
					blocklights[i].g += g * scale;
					blocklights[i].b += b * scale;
					i++;
				}
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

			scale = d_lightstylevalue[psurf->styles[maps]];
			psurf->cached_light[maps] = (short)scale;

			i = 0;
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
					r = g_GammaTable256[(int)((float)rf * (1.0f - frac) + (float)rc * frac + 0.5f)];
					gf = row[fi3+1]; gc = row[ci3+1];
					g = g_GammaTable256[(int)((float)gf * (1.0f - frac) + (float)gc * frac + 0.5f)];
					bf = row[fi3+2]; bc = row[ci3+2];
					b = g_GammaTable256[(int)((float)bf * (1.0f - frac) + (float)bc * frac + 0.5f)];

					blocklights[i].r += r * scale;
					blocklights[i].g += g * scale;
					blocklights[i].b += b * scale;
					i++;
				}

				lt2ptr += 1 + n * 3;
			}
		}
	}
	else if (cl.worldmodel->lightmap_mode == 3)
	{
		/* LT2 grid: each style stores a small ncols x nrows grid of RGB
		 * triples that gets resampled up to the surface's smax x tmax
		 * lightmap. The horizontal weights are the same for every row, so
		 * they are worked out once per style up front. */
		const byte* lt2ptr = (const byte*)lightmap;
		float sRecip = 1.0f / (float)(smax - 1);
		float tRecip = 1.0f / (float)(tmax - 1);

		for (maps = 0; maps < MAXLIGHTMAPS && psurf->styles[maps] != 255; maps++)
		{
			int   hdr, ncols, nrows;
			float sStep, tStep, spos, tpos;

			scale = d_lightstylevalue[psurf->styles[maps]];
			psurf->cached_light[maps] = (short)scale;

			hdr = *lt2ptr++;
			ncols = (hdr >> 4) + 2;
			nrows = (hdr & 15) + 2;

			tpos = 0.0f;
			tStep = (float)(nrows - 1) * tRecip;

			spos = 0.0f;
			sStep = (float)(ncols - 1) * sRecip;

			for (s = 0; s < smax; s++)
			{
				int fi = (int)floor(spos);

				lm_column[s].index = fi;
				lm_column[s].w1 = spos - (float)fi;
				spos += sStep;
				lm_column[s].w0 = 1.0f - lm_column[s].w1;
			}

			i = 0;

			for (t = 0; t < tmax; t++)
			{
				int   ti = (int)floor(tpos);
				float tw1 = tpos - (float)ti;
				float tw0 = 1.0f - tw1;
				int   row0 = ti * ncols;
				int   row1 = (ti + 1) * ncols;

				for (s = 0; s < smax; s++)
				{
					int   si = lm_column[s].index;
					float w1 = lm_column[s].w1;
					float w0 = lm_column[s].w0;
					int   i00 = (row0 + si) * 3;
					int   i01 = (row0 + si + 1) * 3;
					int   i10 = (row1 + si) * 3;
					int   i11 = (row1 + si + 1) * 3;
					short r, g, b;

					r = lt2ptr[i00] * tw0 * w0 + lt2ptr[i10] * tw1 * w0
						+ lt2ptr[i01] * tw0 * w1 + lt2ptr[i11] * tw1 * w1 + 0.5f;
					g = lt2ptr[i00 + 1] * tw0 * w0 + lt2ptr[i10 + 1] * tw1 * w0
						+ lt2ptr[i01 + 1] * tw0 * w1 + lt2ptr[i11 + 1] * tw1 * w1 + 0.5f;
					b = lt2ptr[i00 + 2] * tw0 * w0 + lt2ptr[i10 + 2] * tw1 * w0
						+ lt2ptr[i01 + 2] * tw0 * w1 + lt2ptr[i11 + 2] * tw1 * w1 + 0.5f;

					blocklights[i].r += g_GammaTable256[r] * scale;
					blocklights[i].g += g_GammaTable256[g] * scale;
					blocklights[i].b += g_GammaTable256[b] * scale;
					i++;
				}

				tpos += tStep;
			}

			lt2ptr += nrows * ncols * 3;
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
	int       i;
	unsigned r, g, b;

	if (!surf || !surf->polys)
		return;

	for (i = 0; i < surf->polys->numverts; i++)
	{
		if (!surf->samples)
		{
			*(DWORD*)&surf->polys->verts[i][3] = 0xFFFFFFFFu;
		}
		else
		{
			colorVec* c = &blocklights[
				((int)(surf->polys->verts[i][7] * (BLOCK_HEIGHT * 16.0f) - 8.0f
					- (float)(surf->light_t << 4)) >> 4) * ((surf->extents[0] >> 4) + 1)
				+ ((int)(surf->polys->verts[i][6] * (BLOCK_WIDTH * 16.0f) - 8.0f
					- (float)(surf->light_s << 4)) >> 4)];

			r = c->r >> 8; if (r > 255) r = 255;
			g = c->g >> 8; if (g > 255) g = 255;
			b = c->b >> 8; if (b > 255) b = 255;
			*(DWORD*)&surf->polys->verts[i][3] = 0xFF000000u | (r << 16) | (g << 8) | b;
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
	int smax, tmax, size;
	decal_t* pdecal;

	psurf->cached_dlight = (byte)(psurf->dlightbits & r_dlightactive);
	psurf->dlightbits &= r_dlightactive;

	smax = (psurf->extents[0] >> 4) + 1;
	tmax = (psurf->extents[1] >> 4) + 1;
	size = smax * tmax;

	if (r_fullbright.value || !cl.worldmodel->lightdata)
	{
		DC_FullbrightBlockLights(size);
	}
	else
	{
		DC_ClearBlockLights(size);

		if (psurf->samples)
			DC_SumBlockLights(psurf, smax, tmax);

		if (psurf->dlightframe == (char)r_framecount)
			R_AddDynamicLights(psurf);
	}

	DC_PackBlockLights(psurf);
	DC_SurfacePolyApplyBlockLights(psurf);

	if (psurf && psurf->pdecals)
	{
		pdecal = psurf->pdecals;

		while (pdecal->psurface == psurf)
		{
			pdecal->color = 0xFAAA;
			pdecal = pdecal->pnext;
			if (!pdecal)
				return;
		}
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

	if (currententity->frame && base->alternate_anims)
		base = base->alternate_anims;

	if (base->anim_total)
	{
		if (base->name[0] == '-' && base->anim_total)
		{
			tu = (int)((s->texturemins[0] + (base->width << 16)) / base->width);
			tv = (int)((s->texturemins[1] + (base->height << 16)) / base->height);
			reletive = rtable[tu % 20][tv % 20] % base->anim_total;

			count = 0;
			while (base->anim_min > reletive || base->anim_max <= reletive)
			{
				base = base->anim_next;
				if (!base)
					Sys_Error("R_TextureAnimation: broken cycle");
				if (++count > 100)
					Sys_Error("R_TextureAnimation: infinite cycle");
			}
		}
		else
		{
			reletive = (int)(cl.time * 10.0f) % base->anim_total;

			count = 0;
			while (base->anim_min > reletive || base->anim_max <= reletive)
			{
				base = base->anim_next;
				if (!base)
					Sys_Error("R_TextureAnimation: broken cycle");
				if (++count > 100)
					Sys_Error("R_TextureAnimation: infinite cycle");
			}
		}
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

	// Apply the binary's small projection-scale bias so this pass lands exactly
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

	sOffset = (float)(pEntity->rendercolor.g * 256 + pEntity->rendercolor.b) * (1.0f / 16.0f);
	if (!pEntity->rendercolor.r)
		sOffset = -sOffset;

	speed = cl.time * sOffset * (1.0f / psurface->texinfo->texture->width);
	g_flScrollOffset = speed;

	if (speed < 0.0f)
		g_flScrollOffset = fmod(speed, -1.0f);
	else
		g_flScrollOffset = fmod(speed, 1.0f);

	return g_flScrollOffset;
}

/*
================
DrawGLSolidPoly
================
*/
void DrawGLSolidPoly( glpoly_t* p )
{
	Sys_Error("Use DCV_AccumGLPoly");
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
	GL_Bind(t->gl_texturenum, 0);

	if (fa->flags & SURF_DRAWTURB)
	{
		EmitWaterPolys(fa, SIDE_FRONT);
		return;
	}

	if (fa->flags & SURF_DRAWBACKGROUND)
	{
		DrawGLWaterPoly(fa->polys);
	}
	else if (currententity->rendermode == kRenderTransColor)
	{
		DCV_TexState_VertColor();
		DCV_SetColor(currententity->rendercolor.r, currententity->rendercolor.g,
			currententity->rendercolor.b, (int)(r_blend * 255.0f));
		DCV_AccumSolidPoly(fa->polys);
	}
	else if (fa->flags & SURF_DRAWTILED)
	{
		ScrollOffset(fa, currententity);
		DCV_AccumScrollPoly(fa->polys);
	}
	else
	{
		if (r_alphatestmode)
			DCV_AccumColoredPoly(fa->polys);
		else
			DCV_AccumSolidPoly(fa->polys);
	}

	if (gl_texsort)
	{
		fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
		lightmap_polys[fa->lightmaptexturenum] = fa->polys;
	}

	if (fa->pdecals)
	{
		gDecalSurfs[gDecalSurfCount] = fa;
		if (gDecalSurfCount >= MAX_DECALSURFS)
			return;
		gDecalSurfCount++;
	}

	// check for lightmap modification
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;
	}

	if (fa->dlightframe == (char)r_framecount || fa->cached_dlight)
	{
dynamic:
		if (r_dynamic.value)
		{
			lightmap_modified[fa->lightmaptexturenum] = 1;
			R_BuildLightMap(fa);

			if (lm_texnum[fa->lightmaptexturenum] != nada_texture)
				DCV_UpdateTextureSubRect(lm_texnum[fa->lightmaptexturenum],
					fa->light_s, fa->light_t,
					(fa->extents[0] >> 4) + 1, (fa->extents[1] >> 4) + 1,
					(const unsigned short*)lightmaps, BLOCK_WIDTH);
		}
	}
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
	byte        flagsOr, flagsAnd;
	void        (*pfnAccum)( const void* poly );
	int         maps;

	flagsOr = 0;
	flagsAnd = 0xFF;

	pfnAccum = r_alphatestmode ? DCV_AccumColoredPoly : DCV_AccumSolidPoly;

	for (s = chain; s; s = s->texturechain)
	{
		flagsAnd &= s->flags;
		flagsOr  |= s->flags;
	}

	flagsOr  &= (SURF_DRAWSKY | SURF_DRAWTURB | SURF_DRAWTILED | SURF_DRAWBACKGROUND);
	flagsAnd &= (SURF_DRAWSKY | SURF_DRAWTURB | SURF_DRAWTILED | SURF_DRAWBACKGROUND);

	if (flagsOr)
	{
		if ((flagsAnd & SURF_DRAWSKY) && (flagsOr & SURF_DRAWSKY))
			return;

		if (flagsAnd == SURF_DRAWTURB && flagsOr == SURF_DRAWTURB)
		{
			R_DrawWaterChain(chain, 0);
			return;
		}

		if (flagsAnd != SURF_DRAWTILED || flagsOr != SURF_DRAWTILED)
		{
			Con_Printf("Slow path of RenderBrushPoly %x %x\n");
			for (s = chain; s; s = s->texturechain)
				R_RenderBrushPoly(s);
			return;
		}

		ScrollOffset(chain, currententity);
		pfnAccum = DCV_AccumScrollPoly;
	}

	if (currententity->rendermode == kRenderTransColor)
	{
		DCV_TexState_VertColor();
		DCV_SetColor(currententity->rendercolor.r, currententity->rendercolor.g,
			currententity->rendercolor.b, (int)(r_blend * 255.0f));
	}

	for (s = chain; s; )
	{
		t = R_TextureAnimation(s);
		GL_Bind(t->gl_texturenum, 0);

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
					if (gDecalSurfCount >= MAX_DECALSURFS)
						Sys_Error("Too many decal surfaces!\n");
					gDecalSurfCount++;
				}

				if (r_dynamic.value)
				{
					for (maps = 0; maps < MAXLIGHTMAPS && cur->styles[maps] != 255; maps++)
					{
						if (d_lightstylevalue[cur->styles[maps]] != cur->cached_light[maps])
							goto dynamic;
					}

					if (cur->dlightframe == (char)r_framecount || cur->cached_dlight)
					{
dynamic:
						lightmap_modified[cur->lightmaptexturenum] = 1;
						R_BuildLightMap(cur);

						if (lm_texnum[cur->lightmaptexturenum] != nada_texture)
							DCV_UpdateTextureSubRect(lm_texnum[cur->lightmaptexturenum],
								cur->light_s, cur->light_t,
								(cur->extents[0] >> 4) + 1, (cur->extents[1] >> 4) + 1,
								(const unsigned short*)lightmaps, BLOCK_WIDTH);
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
			S_ExtraUpdate();
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

	if (rendermode != kRenderNormal)
	{
		if (rendermode == kRenderTransColor)
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
	else
	{
		DCV_SetColor(255, 255, 255, 255);
		DCV_TexState_Opaque();
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
	mclipplane_t*	pplane;
	model_t*	clmodel;
	qboolean	rotated;

	currententity = e;

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

	DCV_SetTextureWrap();
	DCV_SetPackedColor(0xFFFFFFFFu);

	memset(lightmap_polys, 0, sizeof(lightmap_polys));

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

	// Hand out a slightly different near plane to each brush model so that
	// models sharing a wall do not fight each other for depth.
	R_ApplyViewModelProjection((float)(r_depthslot % BMODEL_DEPTH_SLOTS)
		* BMODEL_DEPTH_STEP / (float)BMODEL_DEPTH_SLOTS);
	r_depthslot++;

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
					(modelorg[2] + 1.0f < pplane->dist))
					bPass = TRUE;
			}
			else
			{
				dot = DotProduct(modelorg, g_planeNormalTable[pplane->normalindex].normal) - pplane->dist;

				bPass = FALSE;
				if (((psurf->flags & SURF_PLANEBACK) && dot < -BACKFACE_EPSILON)
					|| (!(psurf->flags & SURF_PLANEBACK) && dot > BACKFACE_EPSILON))
					bPass = TRUE;
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
				chains[nchains++] = psurf;
				if (nchains >= MAX_BMODEL_CHAINS)
					Sys_Error("Too many chains in brush model\n");
			}
next_surf:;
		}

		for (c = 0; c < nchains; c++)
			R_DrawSequentialPoly(chains[c]);
	}

	if (currententity->rendermode == kRenderTransAlpha)
	{
		if (gl_lightholes.value)
			R_BlendLightmaps();
	}
	else
	{
		R_DrawDecals();
		if (currententity->rendermode == kRenderNormal)
			R_BlendLightmaps();
	}

	DCV_PopMatrix(D3DTRANSFORMSTATE_WORLD);
	DCV_SetTextureClamp();

	r_alphatestmode = 0;
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
	mclipplane_t* plane;
	msurface_t* surf, ** mark;
	mleaf_t* pleaf;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_TestPackedBoundsAgainstFrustum(node->minmaxs, node->minmaxs + 3))
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
		for (; c; c--, surf++)
		{
			// warped surfaces are never backfaced, because they move off
			// their own plane
			if (surf->visframe == (byte)r_framecount
				&& ((surf->flags & SURF_DRAWBACKGROUND)
					|| ((dot < 0) == !!(surf->flags & SURF_PLANEBACK))))
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

	S_ExtraUpdate();
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
				*(volatile byte *)&node->visframe = (byte)r_visframecount;
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
	poly = (glpoly_t*)Hunk_AllocName(sizeof(glpoly_t) + (lnumverts - 4) * VERTEXSIZE * sizeof(float), "surf polys");
#if HLDC_FIXES
	// Test the allocation before writing through it, or a full hunk takes the
	// renderer down before it can report what ran out.
	if (!poly)
		Sys_Error("NULL poly in BuildSurfaceDisplayList");
	poly->next = fa->polys;
	poly->flags = fa->flags;
#else
	poly->next = fa->polys;
	poly->flags = fa->flags;
	if (!poly)
		Sys_Error("NULL poly in BuildSurfaceDisplayList");
#endif
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
#if HLDC_FIXES
	// Warped surfaces move off their own plane every frame, so they have to
	// keep their co-linear points or the water tears open along its edges.
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER))
#else
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_DRAWBACKGROUND))
#endif
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

	if (gl_texsort == 1)
	{
		surf->lightmaptexturenum = (byte)AllocBlock(smax, tmax, &ls, &lt);
		surf->light_s = (byte)ls;
		surf->light_t = (byte)lt;

		R_BuildLightMap(surf);
	}
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
#define MAX_DECALCLIPVERT		32
#define MAX_DECAL_CHAINS		50

static decal_t			gDecalPool[MAX_DECALS];
// Scratch the polygon clipper ping-pongs between while trimming a decal to its
// surface.
static float gDecalClipA[MAX_DECALCLIPVERT][VERTEXSIZE];
static float gDecalClipB[MAX_DECALCLIPVERT][VERTEXSIZE];

// A decal that came out of the clipper as a whole, unclipped quad keeps its
// four vertices here so they only have to be built once.
#define DECAL_CACHE_ENTRIES		256

typedef struct
{
	int		decalIndex;
	float	verts[4][VERTEXSIZE];
} decalcache_t;

static decalcache_t gDecalCache[DECAL_CACHE_ENTRIES];

static void R_DecalSetupLightmapCoords( float (*pverts)[VERTEXSIZE], msurface_t* psurf, int count );
static int				gDecalCount;					// Pool index
static vec3_t			gDecalPos;

// Where the decal would have come to rest had it kept going through the
// surface. Set up alongside every decal that gets stamped.
static vec3_t			gDecalOrigin;

static model_t*			gDecalModel = NULL;
static texture_t*		gDecalTexture = NULL;
static int				gDecalSize, gDecalIndex;
static int				gDecalFlags, gDecalEntity;

int R_DecalUnProject( decal_t* pdecal, vec_t* position );
void R_DecalCreate( msurface_t* psurface, int textureIndex, float scale, float x, float y );
void R_DecalShoot( int textureIndex, int entity, int modelIndex, vec_t* position, int flags );

#define DECAL_DISTANCE			4

// Empirically determined constants for minimizing overalpping decals
#define MAX_OVERLAP_DECALS		4
#define DECAL_OVERLAP_DIST		8


// Init the decal pool
void R_DecalInit( void )
{
	int i;

	memset(gDecalPool, 0, sizeof(gDecalPool));
	gDecalCount = 0;

	// Nothing in the vertex cache belongs to a decal any more
	for (i = 0; i < DECAL_CACHE_ENTRIES; i++)
		gDecalCache[i].decalIndex = -1;
}


// Unlink pdecal from any surface it's attached to
void R_DecalUnlink( decal_t* pdecal )
{
	decal_t* tmp;
	decal_t* next;
	int      index;

	index = pdecal - gDecalPool;
	if (gDecalCache[index & (DECAL_CACHE_ENTRIES - 1)].decalIndex == index)
		gDecalCache[index & (DECAL_CACHE_ENTRIES - 1)].decalIndex = -1;

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
				Sys_Error("Bad decal list");

			for (next = tmp->pnext; next; next = next->pnext)
			{
				if (next == pdecal)
				{
					tmp->pnext = pdecal->pnext;
					pdecal->psurface = NULL;
					pdecal->pnext = NULL;
					return;
				}
				tmp = next;
			}

			Sys_Error("Bad decal list B");
		}

		pdecal->psurface = NULL;
		pdecal->pnext = NULL;
	}
}


// Just reuse next decal in list
// A decal that spans multiple surfaces will use multiple decal_t pool entries, as each surface needs
// it's own.
decal_t* R_DecalAlloc( decal_t* pdecal )
{
	int limit;

	limit = MAX_DECALS;
	if (r_decals.value < (float)MAX_DECALS)
		limit = (int)r_decals.value;

	// Decals are switched off
	if (!limit)
		return NULL;

	if (!pdecal)
	{
		int count;

		count = 0;		// Check for the odd possiblity of infinte loop
		do
		{
			gDecalCount++;
			if (gDecalCount >= limit)
				gDecalCount = 0;
			pdecal = gDecalPool + gDecalCount;	// reuse next decal
			count++;
		} while ((pdecal->flags & FDECAL_PERMANENT) && count < limit);
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
	mclipplane_t* splitplane;
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
			float		s, t, scale, d;
			vec3_t		normal, tmp;
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
				s = (s + surf->texturemins[0]) / (float)tex->texture->width;
				t = (t + surf->texturemins[1]) / (float)tex->texture->height;

				// the surface's outward normal
				if (surf->flags & SURF_PLANEBACK)
				{
					normal[0] = g_planeNormalTable[surf->plane->normalindex].normal[0] * -1.0f;
					normal[1] = g_planeNormalTable[surf->plane->normalindex].normal[1] * -1.0f;
					normal[2] = g_planeNormalTable[surf->plane->normalindex].normal[2] * -1.0f;
				}
				else
				{
					normal[0] = g_planeNormalTable[surf->plane->normalindex].normal[0];
					normal[1] = g_planeNormalTable[surf->plane->normalindex].normal[1];
					normal[2] = g_planeNormalTable[surf->plane->normalindex].normal[2];
				}

				gDecalOrigin[0] = normal[0] * -2048.0f;
				gDecalOrigin[1] = normal[1] * -2048.0f;
				gDecalOrigin[2] = normal[2] * -2048.0f;
				VectorAdd(gDecalOrigin, gDecalPos, gDecalOrigin);

				// A logo has to read the right way round no matter which way
				// the surface's texture axes happen to run.
				if (gDecalFlags & FDECAL_CUSTOM)
				{
					CrossProduct(surf->texinfo->vecs[0], normal, tmp);
					if (DotProduct(tmp, surf->texinfo->vecs[1]) < 0.0f)
						gDecalFlags |= FDECAL_HFLIP;
					else
						gDecalFlags &= ~FDECAL_HFLIP;

					CrossProduct(surf->texinfo->vecs[1], normal, tmp);
					d = DotProduct(tmp, surf->texinfo->vecs[0]);

					if (gDecalFlags & FDECAL_HFLIP)
						d = -d;

					if (d > 0.0f)
						gDecalFlags |= FDECAL_VFLIP;
					else
						gDecalFlags &= ~FDECAL_VFLIP;
				}

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
	int i;
	decal_t* decal;

	if (cl.worldmodel)
	{
		decal = gDecalPool;

		for (i = 0; i < MAX_DECALS; i++, decal++)
		{
			msurface_t* psurf = decal->psurface;
			decal_t* pdecals;
			texture_t* ptexture;
			int depth;

			// Decal is in use and is not a custom decal
			if (psurf && !(decal->flags & FDECAL_CUSTOM))
			{
				// compute depth
				pdecals = psurf->pdecals;
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

				// Check to see if the decal should be added
				total = DecalListAdd(pList, total);
			}
		}
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
	int entityIndex;

	if (!pdecal || !pdecal->psurface)
		return -1;

	pTexinfo = pdecal->psurface->texinfo;

	s = pdecal->dx * pTexinfo->texture->width - pdecal->psurface->texturemins[0];
	t = pdecal->dy * pTexinfo->texture->height - pdecal->psurface->texturemins[1];

	scale = VectorLength(pTexinfo->vecs[0]) * 0.5f;
	ptexture = Draw_DecalTexture(pdecal->texture);

	s = s + ptexture->width * scale + pdecal->psurface->texturemins[0] - pTexinfo->vecs[0][3];
	t = t + ptexture->height * scale + pdecal->psurface->texturemins[1] - pTexinfo->vecs[1][3];

	scale = fabsf(VectorLength(pTexinfo->vecs[0]));

	if (scale != 0.0f)
	{
		inverseScale = 1.0f / scale;
		inverseScale = inverseScale * inverseScale;
	}

	VectorScale(pTexinfo->vecs[0], s * inverseScale, position);

	VectorMA(position, t * inverseScale, pTexinfo->vecs[1], position);
	VectorMA(position, pdecal->psurface->plane->dist,
		g_planeNormalTable[pdecal->psurface->plane->normalindex].normal, position);

	entityIndex = pdecal->entityIndex;

	if (entityIndex)
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
			VectorAdd(position, phull->clip_mins, position);
			VectorAdd(position, pEdict->v.origin, position);
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
	pdecal->flags = gDecalFlags;
	pdecal->dx = x;
	pdecal->dy = y;
	pdecal->texture = textureIndex;
	pdecal->color = 0xFAAA;
	pdecal->pnext = NULL;

	if (!psurface->pdecals)
	{
		psurface->pdecals = pdecal;
	}
	else
	{
		pold = psurface->pdecals;

		while (pold->pnext)
			pold = pold->pnext;

		pold->pnext = pdecal;
	}

	// Tag surface
	pdecal->psurface = psurface;

	// Set scaling
	pdecal->scale = FloatToShort(scale);
	pdecal->entityIndex = gDecalEntity;
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



/*
==================
R_DecalComputeVertices

Compute decal vertices from surface polygon, project UV, SH-clip to [0,1].
Returns vertex count after clipping. Output in vert[].
==================
*/
static float (*R_DecalComputeVertices(
	float (*pout)[VERTEXSIZE],
	decal_t* plist,
	msurface_t* psurf,
	texture_t* ptexture,
	int* pOutCount ))[VERTEXSIZE]
{
	float  scalex, scaley;
	float* v;
	int    j, outCount;

	scalex = (ShortToFloat(plist->scale) * (float)psurf->texinfo->texture->width)
		/ (float)ptexture->width;
	scaley = (ShortToFloat(plist->scale) * (float)psurf->texinfo->texture->height)
		/ (float)ptexture->height;

	if (!pout)
		pout = gDecalClipA;

	v = psurf->polys->verts[0];
	for (j = 0; j < psurf->polys->numverts; j++, v += VERTEXSIZE)
	{
		VectorCopy(v, gDecalClipA[j]);
		gDecalClipA[j][4] = (v[4] - plist->dx) * scalex;
		gDecalClipA[j][5] = (v[5] - plist->dy) * scaley;

		if (plist->flags & FDECAL_HFLIP)
			gDecalClipA[j][4] = 1.0f - gDecalClipA[j][4];

		if (plist->flags & FDECAL_VFLIP)
			gDecalClipA[j][5] = 1.0f - gDecalClipA[j][5];
	}

	outCount = SHClip(gDecalClipA[0], psurf->polys->numverts, gDecalClipB[0], LEFT_EDGE);
	outCount = SHClip(gDecalClipB[0], outCount, gDecalClipA[0], RIGHT_EDGE);
	outCount = SHClip(gDecalClipA[0], outCount, gDecalClipB[0], TOP_EDGE);
	outCount = SHClip(gDecalClipB[0], outCount, pout[0], BOTTOM_EDGE);

	// A decal that survived the clipper whole can be cached and never clipped
	// again.
	if (outCount && (plist->flags & FDECAL_CLIPTEST))
	{
		plist->flags &= ~FDECAL_CLIPTEST;

		if (outCount == 4)
		{
			qboolean clipped = FALSE;

			for (j = 0; j < 4 && !clipped; j++)
			{
				float u = gDecalClipA[j][4];
				float w = gDecalClipA[j][5];

				if ((u != 0.0f && u != 1.0f) || (w != 0.0f && w != 1.0f))
					clipped = TRUE;
			}

			if (!clipped)
				plist->flags |= FDECAL_NOCLIP;
		}
	}

	*pOutCount = outCount;
	return pout;
}

/*
================
R_DecalSetupLightmapCoords

Give each decal vertex the lightmap coordinate of the surface underneath it, so
the decal picks up the same lighting as the wall it is stuck to.
================
*/
static void R_DecalSetupLightmapCoords( float (*pverts)[VERTEXSIZE], msurface_t* psurf, int count )
{
	int i;
	float s, t;

	for (i = 0; i < count; i++, pverts++)
	{
		s = ((DotProduct((*pverts), psurf->texinfo->vecs[0]) + psurf->texinfo->vecs[0][3])
			- psurf->texturemins[0] + (psurf->light_s << 4) + 8.0f) * (1.0f / 2048.0f);
		t = ((DotProduct((*pverts), psurf->texinfo->vecs[1]) + psurf->texinfo->vecs[1][3])
			- psurf->texturemins[1] + (psurf->light_t << 4) + 8.0f) * (1.0f / 2048.0f);

		(*pverts)[6] = s;
		(*pverts)[7] = t;
	}
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
	decal_t* chains[MAX_DECAL_CHAINS];
	int      numChains = 0;
	decal_t* plist;
	float  (*pverts)[VERTEXSIZE];
	int      i, j, k, outCount;
	texture_t* ptexture;
	msurface_t* psurf;

	if (gDecalSurfCount == 0)
		return;

	DCV_TexState_Blend();
	R_ApplyViewModelProjection(g_frustum_zn - DECAL_DEPTH_NUDGE);

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
			if (!(plist->flags & FDECAL_NOCLIP))
			{
				pverts = R_DecalComputeVertices(NULL, plist, plist->psurface,
					ptexture, &outCount);
				R_DecalSetupLightmapCoords(pverts, plist->psurface, outCount);
			}

			if (plist->flags & FDECAL_NOCLIP)
			{
				int index = plist - gDecalPool;
				decalcache_t* pcache = &gDecalCache[index & (DECAL_CACHE_ENTRIES - 1)];

				if (pcache->decalIndex == index)
				{
					pverts = pcache->verts;
				}
				else
				{
					pcache->decalIndex = index;
					pverts = R_DecalComputeVertices(pcache->verts, plist,
						plist->psurface, ptexture, &outCount);
				}

				outCount = 4;
			}

			if (outCount)
			{
				int   base;
				float* vlist;

				DCV_SetPackedColor(R_DecalColor4444to32(plist->color));

				base = DCV_GetVertCount();
				DCV_AddIndicesFan(base, outCount);

				vlist = pverts[0];
				for (k = 0; k < outCount; k++, vlist += VERTEXSIZE)
					DCV_PushVertexLit(vlist, vlist[4], vlist[5]);
			}
		}
	}

	gDecalSurfCount = 0;
	R_ApplyViewModelProjection(g_frustum_zn + DECAL_DEPTH_NUDGE);
	DCV_SetPackedColor(0xFFFFFFFF);
}



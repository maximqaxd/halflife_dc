// gl_warp.c -- sky and water polygons

#include "quakedef.h"
#include "pr_cmds.h"
#include "gl_water.h"
#include "dc_accum.h"

#ifdef _WIN32_WCE
#pragma optimize("", off)
#endif

#pragma intrinsic(fabsf)
#pragma intrinsic(_Dot3dVW0)

extern model_t* loadmodel;
float floors( float value );

extern cshift_t	cshift_water;

int		skytexturenum;

int		solidskytexture;
int		alphaskytexture;
float	speedscale;		// for top sky and bottom sky
colorVec gWaterColor;

msurface_t* warpface;

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define SUBDIVIDE_SIZE	64.0f
#define WARP_TURBSCALE	40.743665f

void R_DrawSkyBox( void );

void R_CalcBoundingBox( int numverts, float* verts, vec_t* mins, vec_t* maxs )
{
	int		i, j;
	float* v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	i = 0;
	if (numverts > 0)
	{
		do
		{
			j = 0;
			do
			{
				if (*v < mins[j])
					mins[j] = *v;
				if (*v > maxs[j])
					maxs[j] = *v;
				j++;
				v++;
			} while (j < 3);
			i++;
		} while (i < numverts);
	}
}

void SubdividePolygon( int numverts, float* verts )
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float* v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t* poly;
	float	s, t;

	if (numverts > 60)
		Sys_Error("numverts = %i", numverts);

	R_CalcBoundingBox(numverts, verts, mins, maxs);

	i = 0;
	while (i < 3)
	{
		m = (mins[i] + maxs[i]) * 0.5f;
		m = SUBDIVIDE_SIZE * floors(m / SUBDIVIDE_SIZE + 0.5f);
		if (maxs[i] - m < 8)
		{
			i++;
			continue;
		}
		if (m - mins[i] < 8)
		{
			i++;
			continue;
		}

		// cut it
		v = verts + i;
		j = 0;
		if (numverts > 0)
		{
			do
			{
				dist[j] = *v - m;
				j++;
				v += 3;
			} while (j < numverts);
		}

		// wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy(verts, v);

		f = b = 0;
		v = verts;
		j = 0;
		if (numverts > 0)
		{
			do
			{
				if (dist[j] >= 0)
				{
					VectorCopy(v, front[f]);
					f++;
				}
				if (dist[j] <= 0)
				{
					VectorCopy(v, back[b]);
					b++;
				}
				if (dist[j] != 0 && dist[j + 1] != 0 &&
					((dist[j] > 0) != (dist[j + 1] > 0)))
				{
					// clip point
					frac = dist[j] / (dist[j] - dist[j + 1]);
					k = 0;
					do
					{
						front[f][k] = back[b][k] = v[k] + frac * (v[3 + k] - v[k]);
						k++;
					} while (k < 3);
					f++;
					b++;
				}
				j++;
				v += 3;
			} while (j < numverts);
		}

		SubdividePolygon(f, front[0]);
		SubdividePolygon(b, back[0]);
		return;
	}

	poly = (glpoly_t*)Hunk_AllocName(sizeof(glpoly_t) + (numverts - 4) * VERTEXSIZE * sizeof(float), "warp polys");
	poly->next = warpface->polys;
	poly->flags = warpface->flags;
	warpface->polys = poly;
	poly->numverts = numverts;
	i = 0;
	if (numverts > 0)
	{
		do
		{
			VectorCopy(verts, poly->verts[i]);
			s = DotProduct(verts, warpface->texinfo->vecs[0]);
			t = DotProduct(verts, warpface->texinfo->vecs[1]);
			poly->verts[i][4] = s;
			poly->verts[i][5] = t;
			i++;
			verts += 3;
		} while (i < numverts);
	}
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface( msurface_t* fa )
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float* vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i = 0; i < fa->numedges; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy(vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon(numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	turbsin[] =
{
	#include "gl_warp_sin.h"
};

/*
=============
D_SetFadeColor

Set the color and fog parameters for a water surface
=============
*/
void D_SetFadeColor( int r, int g, int b, int fog )
{
	gWaterColor.r = r;
	gWaterColor.g = g;
	gWaterColor.b = b;

	cshift_water.destcolor[0] = r;
	cshift_water.destcolor[1] = g;
	cshift_water.destcolor[2] = b;
	cshift_water.percent = fog;
}

void EmitWaterPolys( msurface_t* fa, int direction )
{
	glpoly_t* p;
	float* v;
	int			i, base;
	float		s, t;
	volatile float	os, ot;
	float		scale;
	vec3_t		tempVert;

	{
		texture_t* ptexture = fa->texinfo->texture;
		D_SetFadeColor(ptexture->fade_r, ptexture->fade_g, ptexture->fade_b, ptexture->fade_fog);
	}

	/* the fan can be walked in either winding direction (see below), so cull
	 * nothing while the water surfaces are on screen */
	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_NONE);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);

	if (fa->polys->verts[0][2] >= r_refdef.vieworg[2])
		scale = -currententity->scale;
	else
		scale = currententity->scale;

	for (p = fa->polys; p; p = p->next)
	{
		DCV_FlushIfLarge();
		base = DCV_GetVertCount();
		DCV_AddIndicesFan(base, p->numverts);

		if (direction)
			v = p->verts[p->numverts - 1];
		else
			v = p->verts[0];

		i = 0;
		if (p->numverts > 0)
		{
			do
			{
				os = v[4];
				ot = v[5];
				VectorCopy(v, tempVert);
				s = turbsin[(int)(cl.time * 160.0f + v[0] + v[1]) & 255] + 8.0f;
				s += (turbsin[(int)(cl.time * 171.0f + v[0] * 5.0f - v[1]) & 255] + 8.0f) * 0.8f;
				tempVert[2] += s * scale;

				s = os + turbsin[(int)((ot * 0.125f + cl.time) * WARP_TURBSCALE) & 255];
				s *= 1.0f / 64.0f;
				t = ot + turbsin[(int)((os * 0.125f + cl.time) * WARP_TURBSCALE) & 255];
				t *= 1.0f / 64.0f;
				DCV_PushVertexLit(tempVert, s, t);

				if (direction)
					v -= VERTEXSIZE * 2;
				v += VERTEXSIZE;
				i++;
			} while (i < p->numverts);
		}
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);
}

/*
================
R_DrawWaterChain
================
*/
void R_DrawWaterChain( msurface_t* pChain, int direction )
{
	glpoly_t*   p;
	texture_t*  ptexture;
	float*      v;
	int         i, base;
	float       s, t;
	volatile float os, ot;
	float       scale;
	vec3_t      tempVert;

	ptexture = pChain->texinfo->texture;
	D_SetFadeColor(ptexture->fade_r, ptexture->fade_g, ptexture->fade_b, ptexture->fade_fog);

	ptexture = R_TextureAnimation(pChain);
	GL_Bind(ptexture->gl_texturenum, 0);

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_NONE);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);

	for (; pChain; pChain = pChain->texturechain)
	{
		if (pChain->polys->verts[0][2] >= r_refdef.vieworg[2])
			scale = -currententity->scale;
		else
			scale = currententity->scale;

		DCV_FlushIfLarge();

		for (p = pChain->polys; p; p = p->next)
		{
			base = DCV_GetVertCount();
			DCV_AddIndicesFan(base, p->numverts);

			if (direction)
				v = p->verts[p->numverts - 1];
			else
				v = p->verts[0];

			i = 0;
			if (p->numverts > 0)
			{
				do
				{
					os = v[4];
					ot = v[5];
					VectorCopy(v, tempVert);
					s = turbsin[(int)(cl.time * 160.0f + v[0] + v[1]) & 255] + 8.0f;
					s += (turbsin[(int)(cl.time * 171.0f + v[0] * 5.0f - v[1]) & 255] + 8.0f) * 0.8f;
					tempVert[2] += s * scale;

					s = os + turbsin[(int)((ot * 0.125f + cl.time) * WARP_TURBSCALE) & 255];
					s *= 1.0f / 64.0f;
					t = ot + turbsin[(int)((os * 0.125f + cl.time) * WARP_TURBSCALE) & 255];
					t *= 1.0f / 64.0f;
					DCV_PushVertexLit(tempVert, s, t);

					if (direction)
						v -= VERTEXSIZE * 2;
					v += VERTEXSIZE;
					i++;
					} while (i < p->numverts);
			}
		}
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);
}

#if 0
/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitBothSkyLayers( msurface_t* fa )
{
	GL_DisableMultitexture();

	GL_Bind(solidskytexture, 0);
	speedscale = realtime * 8;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	glEnable(GL_BLEND);
	GL_Bind(alphaskytexture, 0);
	speedscale = realtime * 16;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	glDisable(GL_BLEND);
}


#define	SKY_TEX		2000

/*
=================================================================

  PCX Loading

=================================================================
*/

typedef struct
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin, ymin, xmax, ymax;
	unsigned short	hres, vres;
	unsigned char	palette[48];
	char	reserved;
	char	color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char	filler[58];
	unsigned 	data;			// unbounded
} pcx_t;

byte* pcx_rgb;

/*
============
LoadPCX
============
*/
void LoadPCX( FILE* f )
{
	pcx_t* pcx, pcxbuf;
	byte	palette[768];
	byte* pix;
	int		x, y;
	int		dataByte, runLength;
	int		count;

//
// parse the PCX file
//
	fread(&pcxbuf, 1, sizeof(pcxbuf), f);

	pcx = &pcxbuf;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 320
		|| pcx->ymax >= 256)
	{
		Con_Printf("Bad pcx file\n");
		return;
	}

	// seek to palette
	fseek(f, -768, SEEK_END);
	fread(palette, 1, 768, f);

	fseek(f, sizeof(pcxbuf) - 4, SEEK_SET);

	count = (pcx->xmax + 1) * (pcx->ymax + 1);
	pcx_rgb = malloc(count * 4);

	for (y = 0; y <= pcx->ymax; y++)
	{
		pix = pcx_rgb + 4 * y * (pcx->xmax + 1);
		for (x = 0; x <= pcx->ymax; )
		{
			dataByte = fgetc(f);

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = fgetc(f);
			}
			else
				runLength = 1;

			while (runLength-- > 0)
			{
				pix[0] = palette[dataByte * 3];
				pix[1] = palette[dataByte * 3 + 1];
				pix[2] = palette[dataByte * 3 + 2];
				pix[3] = 255;
				pix += 4;
				x++;
			}
		}
	}
}

#endif

/*
==================
R_LoadSkys
==================
*/
char* suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

int g_bLoadSkys = FALSE;
int gSkyTexNumber[6];

void R_LoadSkys( void )
{
	int i;
	int length;
	byte* buffer;
	char name[MAX_QPATH];

	i = 0;
	if (i < 6)
	{
		do
		{
			sprintf(name, "sky%d", i);
			DC_FreeTextureByName(name);
		} while (++i < 6);
	}

	if (g_bLoadSkys)
	{
		i = 0;
		if (i < 6)
		{
			do
			{
				gSkyTexNumber[i] = 0;
				sprintf(name, "gfx/env/%s%s.pvr", cl_skyname.string, suf[i]);
				buffer = COM_LoadTempFile(name, &length);
				sprintf(name, "sky%d", i);
				gSkyTexNumber[i] = DC_LoadTexture(name, GLT_WORLD, 512, 512,
					buffer, FALSE, TEX_TYPE_GBIX, NULL);
				COM_FreeFile();
			} while (++i < 6);
		}

		g_bLoadSkys = FALSE;
	}
}

vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};
int	c_sky;

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

float	skymins[2][6], skymaxs[2][6];

void DrawSkyPolygon( int nump, vec_t* vecs )
{
	union
	{
		struct
		{
			int axis;
			int i;
			float* vp;
			vec3_t v;
			vec3_t av;
		} face;
		struct
		{
			int axis;
			int i;
			union
			{
				int j;
				float t;
			} coord;
			float unused1[2];
			float s;
			float unused2[2];
			float dv;
		} projection;
	} work;

	c_sky++;
#if 0
	qglBegin(GL_POLYGON);
	for (i = 0; i < nump; i++, vecs += 3)
	{
		VectorAdd(vecs, r_origin, v);
		qglVertex3fv(v);
	}
	qglEnd();
	return;
#endif
	// decide which face it maps to
	VectorCopy(vec3_origin, work.face.v);
	work.face.i = 0;
	work.face.vp = vecs;
	if (nump > 0)
	{
		do
		{
			VectorAdd(work.face.vp, work.face.v, work.face.v);
			work.face.vp += 3;
		} while (++work.face.i < nump);
	}
	work.face.av[0] = fabs(work.face.v[0]);
	work.face.av[1] = fabs(work.face.v[1]);
	work.face.av[2] = fabs(work.face.v[2]);
	if (work.face.av[0] > work.face.av[1] && work.face.av[0] > work.face.av[2])
	{
		if (work.face.v[0] < 0)
			work.face.axis = 1;
		else
			work.face.axis = 0;
	}
	else if (work.face.av[1] > work.face.av[2] && work.face.av[1] > work.face.av[0])
	{
		if (work.face.v[1] < 0)
			work.face.axis = 3;
		else
			work.face.axis = 2;
	}
	else
	{
		if (work.face.v[2] < 0)
			work.face.axis = 5;
		else
			work.face.axis = 4;
	}

	// project new texture coords
	work.projection.i = 0;
	if (nump > 0)
	{
		do
		{
			work.projection.coord.j = vec_to_st[work.projection.axis][2];
			if (work.projection.coord.j > 0)
				work.projection.dv = vecs[work.projection.coord.j - 1];
			else
				work.projection.dv = -vecs[-work.projection.coord.j - 1];

			work.projection.coord.j = vec_to_st[work.projection.axis][0];
			if (work.projection.coord.j < 0)
				work.projection.s = -vecs[-work.projection.coord.j - 1] / work.projection.dv;
			else
				work.projection.s = vecs[work.projection.coord.j - 1] / work.projection.dv;
			work.projection.coord.j = vec_to_st[work.projection.axis][1];
			if (work.projection.coord.j < 0)
				work.projection.coord.t = -vecs[-work.projection.coord.j - 1] / work.projection.dv;
			else
				work.projection.coord.t = vecs[work.projection.coord.j - 1] / work.projection.dv;

			if (work.projection.s < skymins[0][work.projection.axis])
				skymins[0][work.projection.axis] = work.projection.s;
			if (work.projection.coord.t < skymins[1][work.projection.axis])
				skymins[1][work.projection.axis] = work.projection.coord.t;
			if (work.projection.s > skymaxs[0][work.projection.axis])
				skymaxs[0][work.projection.axis] = work.projection.s;
			if (work.projection.coord.t > skymaxs[1][work.projection.axis])
				skymaxs[1][work.projection.axis] = work.projection.coord.t;
			vecs += 3;
		} while (++work.projection.i < nump);
	}
}

#define	MAX_CLIP_VERTS	64
void ClipSkyPolygon( int nump, vec_t* vecs, int stage )
{
	float* norm;
	float* v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS - 2)
		Sys_Error("ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		DrawSkyPolygon(nump, vecs);
		return;
	}

	front = back = FALSE;
	norm = skyclip[stage];
	i = 0;
	v = vecs;
	if (nump > 0)
	{
		do
		{
			d = DotProduct(v, norm);
			if (d > 0.1f)
			{
				front = TRUE;
				sides[i] = SIDE_FRONT;
			}
			else if (d < 0.1f)
			{
				back = TRUE;
				sides[i] = SIDE_BACK;
			}
			else
				sides[i] = SIDE_ON;
			dists[i] = d;
			v += 3;
		} while (++i < nump);
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon(nump, vecs, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 3)));
	newc[0] = newc[1] = 0;

	i = 0;
	v = vecs;
	if (nump > 0)
	{
		do
		{
			switch (sides[i])
			{
			case SIDE_FRONT:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				break;
			case SIDE_BACK:
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			case SIDE_ON:
				VectorCopy(v, newv[0][newc[0]]);
				newc[0]++;
				VectorCopy(v, newv[1][newc[1]]);
				newc[1]++;
				break;
			}

			if (sides[i] != SIDE_ON && sides[i + 1] != SIDE_ON && sides[i + 1] != sides[i])
			{
				d = dists[i] / (dists[i] - dists[i + 1]);
				j = 0;
				do
				{
					e = v[j] + d * (v[j + 3] - v[j]);
					newv[0][newc[0]][j] = e;
					newv[1][newc[1]][j] = e;
				} while (++j < 3);
				newc[0]++;
				newc[1]++;
			}
			v += 3;
		} while (++i < nump);
	}

	// continue
	ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
	ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain( msurface_t* s )
{
	msurface_t* fa;

	int		i;
	vec3_t	verts[MAX_CLIP_VERTS];
	glpoly_t* p;

	c_sky = 0;

	// calculate vertex values for sky box

	fa = s;
	if (fa)
	{
		do
		{
			p = fa->polys;
			if (p)
			{
				do
				{
					i = 0;
					if (p->numverts > 0)
					{
						do
						{
							VectorSubtract(p->verts[i], r_origin, verts[i]);
						} while (++i < p->numverts);
					}

					ClipSkyPolygon(p->numverts, verts[0], 0);
				} while ((p = p->next) != NULL);
			}
		} while ((fa = fa->texturechain) != NULL);
	}

	R_DrawSkyBox();
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox( void )
{
	int		i;

	i = 0;
	while (i < 6)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
		i++;
	}
}


#define SQRT3INV		(0.57735f)		// a little less than 1 / sqrt(3)

void MakeSkyVec( float s, float t, int axis )
{
	vec3_t		v, b;
	int			j, k;
	float		width;

	width = gl_zmax.value * SQRT3INV;

	if (s < -1.0f)
		s = -1.0f;
	else if (s > 1.0f)
		s = 1.0f;
	if (t < -1.0f)
		t = -1.0f;
	else if (t > 1.0f)
		t = 1.0f;

	b[0] = s * width;
	b[1] = t * width;
	b[2] = width;

	j = 0;
	if (j < 3)
	{
		do
		{
			k = st_to_vec[axis][j];
			if (k < 0)
				v[j] = -b[-k - 1];
			else
				v[j] = b[k - 1];
			v[j] += r_origin[j];
		} while (++j < 3);
	}

	// avoid bilerp seam
	s = (s + 1.0f) * 0.5f;
	t = (t + 1.0f) * 0.5f;

	if (s < 1.0f / 512.0f)
		s = 1.0f / 512.0f;
	else if (s > 511.0f / 512.0f)
		s = 511.0f / 512.0f;
	if (t < 1.0f / 512.0f)
		t = 1.0f / 512.0f;
	else if (t > 511.0f / 512.0f)
		t = 511.0f / 512.0f;

	t = 1.0f - t;
	DCV_PushVertexLit(v, s, t);
}

/*
=================
R_DrawSkyBox
=================
*/
int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
#define SIGN(d)				((d)<0?-1:1)
static int	gFakePlaneType[6] = { 1, -1, 2, -2, 3, -3 };
void R_DrawSkyBox( void )
{
	int		i;
	int		base;
	vec3_t	normal;

	GL_DisableMultitexture();

	for (i = 0; i < 6; i++)
	{
		if (skymins[0][i] >= skymaxs[0][i] || skymins[1][i] >= skymaxs[1][i])
			continue;

		VectorCopy(vec3_origin, normal);
		switch (gFakePlaneType[i])
		{
		case 1:
			normal[0] = 1.0f;
			break;

		case -1:
			normal[0] = -1.0f;
			break;

		case 2:
			normal[1] = 1.0f;
			break;

		case -2:
			normal[1] = -1.0f;
			break;

		case 3:
			normal[2] = 1.0f;
			break;

		case -3:
			normal[2] = -1.0f;
			break;
		}
		// Hack, try to find backfacing surfaces on the inside of the cube to avoid binding their texture
		if (DotProduct(vpn, normal) < (-1.0f + 0.70710678f))
			continue;

		GL_Bind(gSkyTexNumber[skytexorder[i]], 0);
		DCV_SetPackedColor(0xFFFFFFFFu);
		DCV_FlushIfLarge();
		base = DCV_GetVertCount();
		DCV_AddPolyIndices(base, 4);
		MakeSkyVec(skymins[0][i], skymins[1][i], i);
		MakeSkyVec(skymins[0][i], skymaxs[1][i], i);
		MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i);
		MakeSkyVec(skymaxs[0][i], skymins[1][i], i);
	}
}

//===============================================================

/*
==================
R_ForceLoadSkys
==================
*/
void R_ForceLoadSkys( void )
{
	g_bLoadSkys = TRUE;
}

#ifdef _WIN32_WCE
#pragma optimize("", on)
#endif

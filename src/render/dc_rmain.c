// r_main.c

#include "quakedef.h"
#include "r_studio.h"
#include "r_trans.h"
#include "shake.h"
#include "dc_draw.h"
#include "dc_accum.h"
#include <shintr.h>

#pragma intrinsic(fabsf)

cl_entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
cl_entity_t* currententity;

signed char	r_visframecount;
int			r_framecount;

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture
int			currenttexture = -1;	// to avoid unnecessary texture sets
int			cnttextures[2] = { -1, -1 };     // cached

int			particletexture;	// little dot for particles
int			particlepufftexture;
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
mplane_t*	mirror_plane;

alight_t	r_viewlighting;
vec3_t		r_plightvec; // light vector in model reference frame

//
// view origin
//
vec3_t		vup;
vec3_t		vpn;
vec3_t		vright;
vec3_t		r_origin;

float		r_world_matrix[16];
float		r_base_world_matrix[16];
float		gProjectionMatrix[16];
float		gWorldToScreen[16];

/* Current D3D view matrix */
D3DMATRIX	gViewMatrix;

/* Projection scale offset used for coplanar passes; this is not a Z-near
   distance.  The base world projection keeps it at zero. */
float		g_frustum_zn;

// Rotating projection-scale slot handed out to brush models by R_DrawBrushModel.
int			r_depthslot;

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t*	r_viewleaf, * r_oldviewleaf;

texture_t*	r_notexture_mip;

int			d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves( void );

extern cshift_t cshift_water;

/* Effect sprites whose projection is biased toward the camera on Dreamcast. */
extern short g_sModelIndexLaserDot;
extern short g_sModelIndexFireball;
extern short g_sModelIndexSmoke;
extern short g_sModelIndexWExplosion;
extern short g_sModelIndexEgonFlare;

/*-----------------------------------------------------------------*/

#define DEG2RAD( a ) (((a) * (float)M_PI) / 180.0f)

/*
================
DCV matrix stack / helpers

================
*/
/*
======================
DCV_SetViewportDepthRange

======================
*/
/*
===========================
DCV_BuildProjectionAndSetTransform
===========================
*/
void DCV_BuildProjectionAndSetTransform(
	float		right, float left,
	float		top,   float bottom,
	float		zn,    float zf,
	float		scale,
	D3DTRANSFORMSTATETYPE state )
{
	LPDIRECT3DDEVICE3 dev;
	D3DMATRIX	m;
	float		xRange, yRange, zRange;

	DCV_FlushInline();

	dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();
	if (!dev || !dev->lpVtbl)
		return;

	xRange = right - left;
	yRange = top   - bottom;
	zRange = zf    - zn;

	if (xRange == 0.0f || yRange == 0.0f || zRange == 0.0f)
		return;

	memset(&m, 0, sizeof(m));

	m._11 = (2.0f * zn) / xRange;
	m._31 = (right + left) / xRange;
	m._22 = (2.0f * zn) / yRange;
	m._32 = (top + bottom) / yRange;
	m._33 = -zf / zRange;
	m._34 = -1.0f;
	m._43 = -(zn * zf) / zRange;

	/* Post-multiply by scale. */
	m._11 *= scale;
	m._31 *= scale;
	m._22 *= scale;
	m._32 *= scale;
	m._33 *= scale;
	m._34 *= scale;
	m._43 *= scale;

	dev->lpVtbl->SetTransform(dev, state, &m);

	if (state == D3DTRANSFORMSTATE_WORLD)
		memcpy(r_world_matrix,    &m, sizeof(m));
	else if ( state == D3DTRANSFORMSTATE_VIEW )
		memcpy( r_base_world_matrix, &m, sizeof(m) );
	else
		memcpy( gProjectionMatrix, &m, sizeof(m) );
}


void ProjectPointOnPlane( vec_t* dst, const vec_t* p, const vec_t* normal )
{
	float		d;
	vec3_t		n;
	float		inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec_t* dst, const vec_t* src )
{
	int			pos;
	int			i;
	float		minelem = 1.0F;
	vec3_t		tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabsf(src[i]) < minelem)
		{
			pos = i;
			minelem = fabsf(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}


void RotatePointAroundAxisByDegrees( vec_t* dst, const vec_t* dir, const vec_t* point, float degrees )
{
	float		m[3][3];
	float		im[3][3];
	float		zrot[3][3];
	float		tmpmat[3][3];
	float		rot[3][3];
	int			i;
	vec3_t		vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos(DEG2RAD(degrees));
	zrot[0][1] = sin(DEG2RAD(degrees));
	zrot[1][0] = -sin(DEG2RAD(degrees));
	zrot[1][1] = cos(DEG2RAD(degrees));

	R_ConcatRotations(m, zrot, tmpmat);
	R_ConcatRotations(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox( vec_t* mins, vec_t* maxs )
{
	int			i;

	for (i = 0; i < 4; i++)
		if (BoxOnPlaneSide(mins, maxs, &frustum[i]) == 2)
			return TRUE;

	return FALSE;
}


// Same test as R_CullBox, but works directly off a node's short-packed
// bounding box (mnode_t.minmaxs) instead of paying for a float conversion first.
qboolean R_TestPackedBoundsAgainstFrustum( short* mins, short* maxs )
{
	int			i;

	for (i = 0; i < 4; i++)
		if (BoxOnPlaneSide_short(mins, maxs, &frustum[i]) == 2)
			return TRUE;

	return FALSE;
}


void R_RotateForEntity( cl_entity_t* e )
{
	int			i;
	vec3_t		angles;
	vec3_t		modelpos;

	VectorCopy(e->origin, modelpos);
	VectorCopy(e->angles, angles);

	if (e->movetype != MOVETYPE_NONE)
	{
		float		f = 0.0f;
		float		d;
		if (cl.time < e->animtime + 0.2f)
		{
			if (e->animtime != e->prevanimtime)
				f = (cl.time - e->animtime) / (e->animtime - e->prevanimtime);
		}

		for (i = 0; i < 3; i++)
		{
			d = e->prevorigin[i] - e->origin[i];
			modelpos[i] -= d * f;
		}

		if (f > 0.0f && f < 1.5f)
		{
			f = 1.0f - f;

			for (i = 0; i < 3; i++)
			{
				d = e->prevangles[i] - e->angles[i];
				if (d > 180.0f)
					d -= 360.0f;
				else if (d < -180.0f)
					d += 360.0f;

				angles[i] += d * f;
			}
		}
	}

	DCV_Translate(D3DTRANSFORMSTATE_WORLD, modelpos[0], modelpos[1], modelpos[2]);
	DCV_Rotate(D3DTRANSFORMSTATE_WORLD, angles[1], 0.0f, 0.0f, 1.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_WORLD, angles[0], 0.0f, 1.0f, 0.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_WORLD, angles[2], 1.0f, 0.0f, 0.0f);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*


/*
==================
R_ApplyViewModelProjection

Rebuild the fixed 2-unit-near projection with a small scale offset.  The
offset separates coplanar passes (brush entities, decals, lightmaps) without
changing their clip planes.
==================
*/
#define VIEWMODEL_ZNEAR		2.0f

void R_ApplyViewModelProjection( float zn )
{
	extern int	glwidth, glheight;
	float		aspect, fov, fovy, xmin, xmax, ymin, ymax, msw, zf, znear;

	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	aspect = (float)glwidth / (float)glheight;

	fov = scr_fov_value;
	fovy = CalcFov(fov, (float)glwidth, (float)glheight);

	msw = dc_msw.value * (zn + 1.0f);
	g_frustum_zn = zn;
	zf = gl_zmax.value;

	znear = VIEWMODEL_ZNEAR;
	ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
	ymin = -ymax;

	xmin = aspect * ymin;
	xmax = aspect * ymax;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, xmin, xmax, ymin, ymax,
		znear, zf, msw);

	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
}

/*
==================
R_ApplyViewProjection

Rebuild the scene projection at the fixed 2-unit near plane with the view
scale.  Used by the passes that draw after the world has been submitted.
==================
*/
void R_ApplyViewProjection( void )
{
	extern int	glwidth, glheight;
	float		aspect, fov, fovy, xmin, xmax, ymin, ymax, zf, znear, scale;

	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	aspect = (float)glwidth / (float)glheight;

	fov = scr_fov_value;
	fovy = CalcFov(fov, (float)glwidth, (float)glheight);

	zf = gl_zmax.value;
	scale = dc_msv.value;

	znear = VIEWMODEL_ZNEAR;
	ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
	ymin = -ymax;

	xmin = aspect * ymin;
	xmax = aspect * ymax;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, xmin, xmax, ymin, ymax,
		znear, zf, scale);

	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
}

void R_ApplyDecalProjection( void )
{
	extern int	glwidth, glheight;
	float		aspect, fov, fovy, xmin, xmax, ymin, ymax, zf, znear, scale;

	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	aspect = (float)glwidth / (float)glheight;

	fov = scr_fov_value;
	fovy = CalcFov(fov, (float)glwidth, (float)glheight);

	zf = gl_zmax.value;
	scale = dc_msd.value;

	znear = VIEWMODEL_ZNEAR;
	ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
	ymin = -ymax;

	xmin = aspect * ymin;
	xmax = aspect * ymax;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, xmin, xmax, ymin, ymax,
		znear, zf, scale);

	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
}

/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel( cl_entity_t* e )
{
	vec3_t		point, forward, right, up;
	mspriteframe_t* frame;
	register float	scale;
	int			depthHack;
	int			projectionChanged;
	msprite_t*	psprite;
	colorVec	color;
	int			i0, i1, i2, i3;

	depthHack = FALSE;
	projectionChanged = FALSE;
	psprite = (msprite_t*)(((unsigned)e->model->cache.data & 1) ? NULL : e->model->cache.data);

	frame = R_GetSpriteFrame(psprite, e->frame);

	if (e->scale > 0.0f)
		scale = e->scale;
	else
		scale = 1.0f;

	if (e->rendermode == kRenderNormal)
		r_blend = 1.0f;

	if (e->model == cl.model_precache[g_sModelIndexLaserDot])
		depthHack = TRUE;
	else if (e->model == cl.model_precache[g_sModelIndexFireball])
		depthHack = TRUE;
	else if (e->model == cl.model_precache[g_sModelIndexSmoke])
		depthHack = TRUE;
	else if (e->model == cl.model_precache[g_sModelIndexWExplosion])
		depthHack = TRUE;
	else if (e->model == cl.model_precache[g_sModelIndexEgonFlare])
		depthHack = TRUE;

	R_SpriteColor(&color, e, (int)(r_blend * 255.0f));

	if (gl_spriteblend.value || e->rendermode != kRenderNormal)
	{
		if (e->rendermode == kRenderTransColor)
		{
			DCV_TexState_Blend();
			DCV_SetColor(color.r, color.g, color.b, (int)(r_blend * 255.0f));
		}
		else if (e->rendermode == kRenderTransAdd)
		{
			DCV_TexState_Additive();
			DCV_SetColor(color.r, color.g, color.b, 255);
			if (depthHack)
			{
				extern int	glwidth, glheight;
				float		aspect, fov, fovy, xmax, ymax, msw, zf, znear;

				DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
				aspect = (float)glwidth / (float)glheight;
				fov = scr_fov_value;
				fovy = CalcFov(fov, (float)glwidth, (float)glheight);
				msw = dc_msw.value * 0.75f;
				g_frustum_zn = -0.25f;
				zf = gl_zmax.value;
				znear = VIEWMODEL_ZNEAR;
				ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
				xmax = aspect * ymax;
				DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
					znear, zf, msw);
				DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
				projectionChanged = TRUE;
			}
		}
		else if (e->rendermode == kRenderGlow)
		{
			DCV_TexState_Additive();
			DCV_SetColor(color.r, color.g, color.b, 255);
			DCV_FlushApplyRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
			DCV_FlushApplyRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
			if (depthHack)
			{
				extern int	glwidth, glheight;
				float		aspect, fov, fovy, xmax, ymax, msw, zf, znear;

				DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
				aspect = (float)glwidth / (float)glheight;
				fov = scr_fov_value;
				fovy = CalcFov(fov, (float)glwidth, (float)glheight);
				g_frustum_zn = -0.5f;
				msw = dc_msw.value * 0.5f;
				zf = gl_zmax.value;
				znear = VIEWMODEL_ZNEAR;
				ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
				xmax = aspect * ymax;
				DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
					znear, zf, msw);
				DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
				projectionChanged = TRUE;
			}
		}
		else if (e->rendermode == kRenderTransAlpha)
		{
			DCV_TexState_Blend();
			DCV_SetColor(color.r, color.g, color.b, (int)(r_blend * 255.0f));
			if (depthHack)
			{
				extern int	glwidth, glheight;
				float		aspect, fov, fovy, xmax, ymax, msw, zf, znear;

				DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
				aspect = (float)glwidth / (float)glheight;
				fov = scr_fov_value;
				fovy = CalcFov(fov, (float)glwidth, (float)glheight);
				zf = gl_zmax.value;
				msw = dc_msw.value * 0.75f;
				g_frustum_zn = -0.25f;
				znear = VIEWMODEL_ZNEAR;
				ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
				xmax = aspect * ymax;
				DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
					znear, zf, msw);
				DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
				projectionChanged = TRUE;
			}
		}
		else
		{
			DCV_TexState_Blend();
			DCV_SetColor(color.r, color.g, color.b, (int)(r_blend * 255.0f));
		}
	}
	else
	{
		DCV_SetColor(color.r, color.g, color.b, 255);
		DCV_TexState_Opaque();
	}

	R_GetSpriteAxes(e, psprite->type, forward, right, up);
	GL_DisableMultitexture();
	GL_Bind(frame->gl_texturenum, 0);

	DCV_FlushIfLarge();

	/* v0: bottom-left */
	VectorMA(r_entorigin, frame->down * scale, up,    point);
	VectorMA(point,       frame->left * scale, right, point);
	i0 = DCV_AddVertex(point[0], point[1], point[2], 0.0f, 1.0f);

	/* v1: top-left */
	VectorMA(r_entorigin, frame->up   * scale, up,    point);
	VectorMA(point,       frame->left * scale, right, point);
	i1 = DCV_AddVertex(point[0], point[1], point[2], 0.0f, 0.0f);

	/* v2: top-right */
	VectorMA(r_entorigin, frame->up    * scale, up,    point);
	VectorMA(point,       frame->right * scale, right, point);
	i2 = DCV_AddVertex(point[0], point[1], point[2], 1.0f, 0.0f);

	/* v3: bottom-right */
	VectorMA(r_entorigin, frame->down  * scale, up,    point);
	VectorMA(point,       frame->right * scale, right, point);
	i3 = DCV_AddVertex(point[0], point[1], point[2], 1.0f, 1.0f);

	DCV_AddIndicesQuad(i0, i1, i2, i3);

	DCV_FlushApplyRenderState(D3DRENDERSTATE_ZENABLE,      D3DZB_TRUE);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_ZWRITEENABLE,  TRUE);

	if (projectionChanged)
	{
		extern int	glwidth, glheight;
		float		aspect, fov, fovy, xmax, ymax, msw, zf, znear;

		DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
		aspect = (float)glwidth / (float)glheight;
		fov = scr_fov_value;
		fovy = CalcFov(fov, (float)glwidth, (float)glheight);
		g_frustum_zn = 0.0f;
		msw = dc_msw.value;
		zf = gl_zmax.value;
		znear = VIEWMODEL_ZNEAR;
		ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
		xmax = aspect * ymax;
		DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
			znear, zf, msw);
		DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
	}
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162
float		r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t		shadevector;
float		shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float		r_avertexnormal_dots[SHADEDOT_QUANT][256] = {
#include "anorm_dots.h"
};

float*		shadedots = r_avertexnormal_dots[0];

int			lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void R_DrawAliasFrame( aliashdr_t* paliashdr, int posenum )
{
	Sys_Error("R_DrawAliasFrame should be obsolete\n");
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern vec3_t lightspot;

void R_DrawAliasShadow( aliashdr_t* paliashdr, int posenum )
{
	Sys_Error("R_DrawAliasShadow should be obsolete\n");
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame( int frame, aliashdr_t* paliashdr )
{
	Sys_Error("R_SetupAliasFrame should be obsolete\n");
}



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel( cl_entity_t* e )
{
	Sys_Error("R_DrawAliasModel should be obsolete\n");
}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList( void )
{
	int			i, j;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i = 0; i < cl_numvisedicts; i++)
	{
		currententity = &cl_visedicts[i];

		if (currententity->rendermode != kRenderNormal)
		{
			AddTEntity(currententity);
			continue;
		}

		switch (currententity->model->type)
		{
		case mod_brush:
			R_DrawBrushModel(currententity);
			break;

		case mod_alias:
			Sys_Error("R_DrawAliasModel should be obsolete\n");
			Sys_Error("We have alias models???");
			break;

		case mod_studio:
			if (currententity->index > 0 && currententity->index <= cl.maxclients)
			{
				R_StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS,
					&cl.frames[cl.parsecount & UPDATE_MASK].playerstate[currententity->index - 1]);
			}
			else
			{
				if (currententity->movetype == MOVETYPE_FOLLOW)
				{
					for (j = 0; j < cl_numvisedicts; j++)
					{
						if (cl_visedicts[j].index != currententity->aiment)
							continue;

						currententity = &cl_visedicts[j];
						if (currententity->index > 0 && currententity->index <= cl.maxclients)
						{
							R_StudioDrawPlayer(0,
								&cl.frames[cl.parsecount & UPDATE_MASK].playerstate[currententity->index - 1]);
						}
						else
						{
							R_StudioDrawModel(0, TRUE);
						}

						currententity = &cl_visedicts[i];
						R_StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS, TRUE);
						break;
					}
				}
				else
				{
					R_StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS, TRUE);
				}
			}
			break;

		default:
			break;
		}
	}

	r_blend = 1.0f;

	for (i = 0; i < cl_numvisedicts; i++)
	{
		currententity = &cl_visedicts[i];

		if (currententity->rendermode != kRenderNormal)
			continue;

		switch (currententity->model->type)
		{
		case mod_sprite:
			if (currententity->body)
			{
				float*		pAttachment;

				pAttachment = R_GetAttachmentPoint(currententity->skin, currententity->body);
				VectorCopy(pAttachment, r_entorigin);
			}
			else
			{
				VectorCopy(currententity->origin, r_entorigin);
			}

			R_DrawSpriteModel(currententity);
			break;

		default:
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel( void )
{
	extern int	glwidth, glheight;
	float		lightvec[3];
	colorVec	c;
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add, oldShadows;
	float		screenaspect, yfov, ymax, znear;
	dlight_t*	dl;

	lightvec[0] = -1;
	lightvec[1] = 0;
	lightvec[2] = 0;

	currententity = &cl.viewent;

	if (!r_drawviewmodel.value || cam_thirdperson || chase_active.value || envmap ||
		!r_drawentities.value || cl.stats[STAT_HEALTH] <= 0 ||
		!currententity->model || cl.viewentity > cl.maxclients || cl.spectator)
	{
		c = R_LightPoint(currententity->origin);
		cl.light_level = (c.r + c.g + c.b) / 3;
		return;
	}

		DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
		screenaspect = (float)glwidth / (float)glheight;
		yfov = CalcFov(scr_fov_value, (float)glwidth, (float)glheight);
		znear = VIEWMODEL_ZNEAR;
		ymax = znear * (float)tan((yfov * (float)M_PI) / 360.0f);
		DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION,
			-screenaspect * ymax, screenaspect * ymax, -ymax, ymax,
			znear, gl_zmax.value, dc_msv.value);
		DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
		DCV_SetTextureWrap();

		switch (currententity->model->type)
		{
		case mod_brush:
			R_DrawBrushModel(currententity);
			break;

		case mod_alias:
			c = R_LightPoint(currententity->origin);

			j = (c.r + c.g + c.b) / 3;
			if (j < 24)
				j = 24;
			r_viewlighting.ambientlight = j;
			r_viewlighting.shadelight = j;

			for (lnum = 0; lnum < MAX_DLIGHTS; lnum++)
			{
				dl = &cl_dlights[lnum];
				if (!dl->radius)
					continue;
				if (!dl->radius)
					continue;
			if (dl->die < cl.time)
					continue;

				VectorSubtract(currententity->origin, dl->origin, dist);
				add = dl->radius - VectorLength(dist);
				if (add > 0.0f)
					r_viewlighting.ambientlight += add;
			}

			if (r_viewlighting.ambientlight > 128)
				r_viewlighting.ambientlight = 128;
			if (r_viewlighting.ambientlight + r_viewlighting.shadelight > 192)
				r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;

			r_viewlighting.plightvec = lightvec;
			R_DrawAliasModel(currententity);
			break;

		case mod_studio:
			if (cl.weaponstarttime == 0.0f)
				cl.weaponstarttime = cl.time;
			
			currententity->sequence = cl.weaponsequence;
			currententity->frame = 0.0f;
			currententity->framerate = FloatToShort(1.0f);
			currententity->animtime = cl.weaponstarttime;
			currententity->colormap = cl.players[cl.playernum].translations[0xf0];
			currententity->colormap |= cl.players[cl.playernum].translations[0xf4] << 8;

			cl.light_level = 128;

			oldShadows = r_shadows.value;
			r_shadows.value = 0.0f;
			R_StudioDrawModel(STUDIO_RENDER, FALSE);
			r_shadows.value = oldShadows;
			break;
		}

		DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);
		screenaspect = (float)glwidth / (float)glheight;
		yfov = CalcFov(scr_fov_value, (float)glwidth, (float)glheight);
		r_depthslot = 0;
		znear = VIEWMODEL_ZNEAR;
		ymax = znear * (float)tan((yfov * (float)M_PI) / 360.0f);
		DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION,
			-screenaspect * ymax, screenaspect * ymax, -ymax, ymax,
			znear, gl_zmax.value, dc_msw.value);
		DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);
		DCV_SetTextureClamp();
}

void R_DispatchViewModelEvents( void )
{
	cl_entity_t* viewent;

	viewent = &cl.viewent;
	currententity = viewent;

	// Don't draw if it's disabled.
	if (!r_drawviewmodel.value)
		return;

	// Don't draw if we are in a third person mode.
	if (cam_thirdperson || chase_active.value || envmap || !r_drawentities.value)
		return;

	if (cl.stats[STAT_HEALTH] <= 0 || !viewent->model || cl.viewentity > cl.maxclients || cl.spectator)
		return;

	if (viewent->model->type == mod_brush ||
		viewent->model->type == mod_alias ||
		viewent->model->type != mod_studio)
		return;

	if (cl.weaponstarttime == 0.0f)
		cl.weaponstarttime = cl.time;

	viewent->sequence = cl.weaponsequence;
	viewent->frame = 0.0f;
	viewent->framerate = FloatToShort(1.0f);
	viewent->animtime = cl.weaponstarttime;

	R_StudioDrawModel(STUDIO_EVENTS, FALSE);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend( void )
{
	unsigned char color[4];
	int			alpha;

	alpha = V_FadeAlpha();
	if (!alpha)
		return;

	if (cl.sf.fadeFlags & FFADE_MODULATE)
	{
		int			remainder = (255 - alpha) * 255;

		color[0] = (alpha * cl.sf.fader + remainder) >> 8;
		color[1] = (alpha * cl.sf.fadeg + remainder) >> 8;
		color[2] = (alpha * cl.sf.fadeb + remainder) >> 8;
		color[3] = 255;
	}
	else
	{
		color[0] = cl.sf.fader;
		color[1] = cl.sf.fadeg;
		color[2] = cl.sf.fadeb;
		color[3] = alpha;
	}

	DCV_ScreenFade(color[0], color[1], color[2], color[3], 0,
		(cl.sf.fadeFlags & FFADE_MODULATE) != 0);
}


int SignbitsForPlane( mplane_t* out )
{
	int			bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1 << j;
	}
	return bits;
}

void R_SetFrustum( void )
{
	int			i;
	float		fovx, fovy;

	fovx = scr_fov_value;
	fovy = CalcFov(fovx, (float)glwidth, (float)glheight);
	fovx *= 0.5f;
	fovy *= 0.5f;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundAxisByDegrees(frustum[0].normal, vup, vpn, -(90.0f - fovx));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundAxisByDegrees(frustum[1].normal, vup, vpn, 90.0f - fovx);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundAxisByDegrees(frustum[2].normal, vright, vpn, 90.0f - fovy);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundAxisByDegrees(frustum[3].normal, vright, vpn, -(90.0f - fovy));

	for (i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame( void )
{
	extern cvar_t forcefog;
	int			fogEnabled;

	DCV_SetTextureFilterDefault();

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set("r_fullbright", "0");

	R_AnimateLight();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy(r_refdef.vieworg, r_origin);

	AngleVectors(r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);
	if (cl.waterlevel > 2)
	{
		DCV_SetFog(TRUE, cshift_water.destcolor[0], cshift_water.destcolor[1],
			cshift_water.destcolor[2], cshift_water.percent);
	}
	else
	{
		fogEnabled = TRUE;
		if (forcefog.value <= 0.0f)
			fogEnabled = FALSE;
		DCV_SetFog(fogEnabled, 64, 64, 64, 128);
	}

	V_CalcBlend();

	r_cache_thrash = FALSE;

	c_brush_polys = 0;
	c_alias_polys = 0;
}

void MYgluPerspective( float fovy, float aspect,
	float		zNear, float zFar, float scale )
{
	float		xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * (float)M_PI / 360.0f);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, xmin, xmax, ymin, ymax, zNear, zFar, scale);
}

/*
====================
CalcFov
====================
*/
float CalcFov( float fov_x, float width, float height )
{
	float		a;
	float		x;

	if (fov_x < 1.0f || fov_x > 179.0f)
		fov_x = 90.0f;	// error, set to 90

	x = width / tan((fov_x / 360.0f) * (float)M_PI);

	a = atan(height / x);

	a = (a * 360.0f) / (float)M_PI;

	return a;
}

float		g_flHudDepth;		// current HUD depth sublayer

/*
================
DCV_SetHudDepth

Set up a 2D orthographic projection for the HUD and place it at one of the depth
sublayers between dc_msh and dc_msh2, so overlapping HUD elements sort correctly.
================
*/
void DCV_SetHudDepth( float layer )
{
	float		depth;

	DCV_SetViewport(glx, gly, glwidth, glheight);
	DCV_SetTransform(3, &g_identityMatrix);

	depth = dc_msh.value + (dc_msh2.value - dc_msh.value) * layer;

	g_flHudDepth = layer;

	DCV_Ortho(3, 0.0f, (float)glwidth, (float)glheight, 0.0f, 10.0f, -10.0f, depth);

	DCV_SetTransform(2, &g_identityMatrix);
	DCV_SetTransform(1, &g_identityMatrix);

	DCV_SetViewportDepthRange(dc_depthminhud.value, dc_depthmaxhud.value);
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL( void )
{
	extern int	glwidth, glheight;
	int			x, x2, y2, y, w, h;
	D3DMATRIX	view;
	D3DMATRIX	worldView;
	D3DMATRIX	world;

	float		screenaspect;
	float		yfov;
	float		ymax;

	DCV_Flush();

	x = r_refdef.vrect.x;
	x2 = r_refdef.vrect.x + r_refdef.vrect.width;
	y = glheight - r_refdef.vrect.y;
	y2 = glheight - (r_refdef.vrect.y + r_refdef.vrect.height);

	if (x > 0) x--;
	if (x2 < glwidth) x2++;
	if (y2 < 0) y2--;
	if (y < glheight) y++;

	w = x2 - x;
	h = y - y2;
	if (envmap)
	{
		x = y2 = 0;
		glwidth = glheight = w = h = gl_envmapsize.value;
	}

	DCV_SetViewport(glx + x, gly + y2, w, h);

	g_frustum_zn = 0.0f;
	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	screenaspect = (float)glwidth / (float)glheight;
	yfov = CalcFov(scr_fov_value, (float)glwidth, (float)glheight);
	r_depthslot = 0;
	ymax = VIEWMODEL_ZNEAR * (float)tan((yfov * (float)M_PI) / 360.0f);

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION,
		-screenaspect * ymax, screenaspect * ymax, -ymax, ymax,
		VIEWMODEL_ZNEAR, gl_zmax.value, dc_msw.value);
	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);

	if (gl_cull.value)
	{
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);
	}
	else
	{
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_NONE);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);
	}

	DCV_GetTransform(D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX *)gProjectionMatrix);
	DCV_SetTransform(D3DTRANSFORMSTATE_WORLD, &g_identityMatrix);
	DCV_SetTransform(D3DTRANSFORMSTATE_VIEW, &g_identityMatrix);

	DCV_Rotate(D3DTRANSFORMSTATE_VIEW, -90.0f, 1.0f, 0.0f, 0.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_VIEW, 90.0f, 0.0f, 0.0f, 1.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_VIEW, -r_refdef.viewangles[2], 1.0f, 0.0f, 0.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_VIEW, -r_refdef.viewangles[0], 0.0f, 1.0f, 0.0f);
	DCV_Rotate(D3DTRANSFORMSTATE_VIEW, -r_refdef.viewangles[1], 0.0f, 0.0f, 1.0f);
	DCV_Translate(D3DTRANSFORMSTATE_VIEW, -r_refdef.vieworg[0], -r_refdef.vieworg[1],
		-r_refdef.vieworg[2]);

	DCV_TexState_Opaque();
	DCV_SetTextureClamp();

	DCV_GetTransform(D3DTRANSFORMSTATE_VIEW, &view);
	DCV_GetTransform(D3DTRANSFORMSTATE_WORLD, &world);
	_Multiply4dM((float *)&worldView, (float *)&world, (float *)&view);
	_Multiply4dM(gWorldToScreen, (float *)&worldView, gProjectionMatrix);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene( void )
{
	float		aspect;
	float		fov;
	float		fovy;
	float		xmax;
	float		ymax;
	float		zfar;
	float		msw;
	float		znear;

	key_count++;

	DCV_AddMeterValue(0xff808080, 0.0f);
	DCV_AddMeterValue(0xff808080, 0.016666667f);
	DCV_AddMeterValue(0xff808080, 0.033333333f);
	DCV_AddMeterValue(0xff808080, 0.05f);
	DCV_AddMeterValue(0xff808080, 0.066666667f);
	DCV_AddMeterValue(0xff808080, 0.083333333f);

	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	R_MarkLeaves();	// done here so we know if we're in water
	DCV_AddMeterTimed(0xffff0000);

	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	aspect = (float)glwidth / (float)glheight;
	fov = scr_fov_value;
	fovy = CalcFov(fov, (float)glwidth, (float)glheight);
	msw = dc_msw.value;
	msw *= 1.0005f;
	g_frustum_zn = 0.0005f;
	zfar = gl_zmax.value;
	znear = VIEWMODEL_ZNEAR;
	ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
	xmax = aspect * ymax;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
		znear, zfar, msw);
	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);

	R_DrawWorld();		// adds static entities to the list

	DCV_SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	aspect = (float)glwidth / (float)glheight;
	fov = scr_fov_value;
	fovy = CalcFov(fov, (float)glwidth, (float)glheight);
	msw = dc_msw.value;
	g_frustum_zn = 0.0f;
	zfar = gl_zmax.value;
	znear = VIEWMODEL_ZNEAR;
	ymax = znear * tan((fovy * (float)M_PI) / 360.0f);
	xmax = aspect * ymax;

	DCV_Frustum(D3DTRANSFORMSTATE_PROJECTION, -xmax, xmax, -ymax, ymax,
		znear, zfar, msw);
	DCV_SetViewportDepthRange(dc_depthmin.value, dc_depthmax.value);

	DCV_AddMeterTimed(0xffffff00);

	S_ExtraUpdate();	// don't let sound get messed up if going slow
	IN_Accumulate();

	R_DrawEntitiesOnList();

	DCV_FlushApplyRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	DCV_FlushApplyRenderState(D3DRENDERSTATE_RANGEFOGENABLE, FALSE);
	
	R_DrawTEntitiesOnList();
	DCV_AddMeterTimed(0xff00ff00);

	S_ExtraUpdate();
	IN_Accumulate();

	R_RenderDlights();

	GL_DisableMultitexture();

	R_DrawParticles();
	DCV_AddMeterTimed(0xff0000ff);
}


void R_SetStackBase( void )
{
	// get stack position so we can guess if we are going to overflow
	//r_stack_start = (byte*)&dummy;
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView( void )
{
	float		time1, time2;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		time1 = Sys_FloatTime();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = FALSE;
	gldepthmin = dc_depthmin.value;
	gldepthmax = dc_depthmax.value;

	R_DispatchViewModelEvents();

	R_RenderScene();

	R_DrawViewModel();

	R_PolyBlend();

	S_ExtraUpdate();
	IN_Accumulate();

	if (r_speeds.value)
	{
		float		framerate = cl.time - cl.oldtime;

		if (framerate > 0.0f)
			framerate = 1.0f / framerate;

		time2 = Sys_FloatTime();
		Con_Printf("%3ifps %3i ms  %4i wpoly %4i epoly\n", (int)(framerate + 0.5f), (int)((time2 - time1) * 1000.0f), c_brush_polys, c_alias_polys);
	}
}

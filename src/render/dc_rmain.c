// r_main.c

#include "quakedef.h"
#include "r_studio.h"
#include "r_trans.h"
#include "shake.h"
#include "dc_draw.h"
#include "dc_accum.h"

extern void* Sys_GetD3DDevice3( void );
extern void* Sys_GetD3DViewport( void );

cl_entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
cl_entity_t* currententity;

int			r_visframecount;
int			r_framecount;

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture
int			currenttexture = -1;	// to avoid unnecessary texture sets
int			cnttextures[2] = { -1, -1 };     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
mplane_t*	mirror_plane;

alight_t	r_viewlighting;
vec3_t		r_plightvec; // light vector in model reference frame

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];
float	gProjectionMatrix[16];
float	gWorldToScreen[16];

/* Current D3D view matrix */
D3DMATRIX gViewMatrix;

static float g_frustum_xmax, g_frustum_ymax, g_frustum_zn;

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t*	r_viewleaf, * r_oldviewleaf;

texture_t*	r_notexture_mip;

int			d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves( void );

extern cshift_t	cshift_water;

/*-----------------------------------------------------------------*/

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

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
	float right, float left,
	float top,   float bottom,
	float zn,    float zf,
	float scale,
	D3DTRANSFORMSTATETYPE state )
{
	LPDIRECT3DDEVICE3 dev;
	D3DMATRIX         m;
	float             xRange, yRange, zRange;

	DCV_FlushInline();

	dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();
	if ( !dev || !dev->lpVtbl )
		return;

	xRange = right - left;
	yRange = top   - bottom;
	zRange = zf    - zn;

	if ( xRange == 0.0f || yRange == 0.0f || zRange == 0.0f )
		return;

	memset( &m, 0, sizeof(m) );

	m._11 = ( 2.0f * zn ) / xRange;
	m._31 = ( right + left ) / xRange;
	m._22 = ( 2.0f * zn ) / yRange;
	m._32 = ( top + bottom ) / yRange;
	m._33 = -zf / zRange;
	m._34 = -1.0f;
	m._43 = -( zn * zf ) / zRange;

	/* Post-multiply by scale (binary-accurate) */
	m._11 *= scale;
	m._31 *= scale;
	m._22 *= scale;
	m._32 *= scale;
	m._33 *= scale;
	m._34 *= scale;
	m._43 *= scale;

	dev->lpVtbl->SetTransform( dev, state, &m );

	if ( state == D3DTRANSFORMSTATE_WORLD )
		memcpy( r_world_matrix,    &m, sizeof(m) );
	else if ( state == D3DTRANSFORMSTATE_VIEW )
		memcpy( r_base_world_matrix, &m, sizeof(m) );
	else
		memcpy( gProjectionMatrix, &m, sizeof(m) );
}


void ProjectPointOnPlane( vec_t* dst, const vec_t* p, const vec_t* normal )
{
	float d;
	vec3_t n;
	float inv_denom;

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
	int pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
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


void RotatePointAroundVector( vec_t* dst, const vec_t* dir, const vec_t* point, float degrees )
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;

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
	int		i;

	for (i = 0; i < 4; i++)
		if (BoxOnPlaneSide(mins, maxs, &frustum[i]) == 2)
			return TRUE;

	return FALSE;
}


void R_RotateForEntity( cl_entity_t* e )
{
	int		i;
	vec3_t	angles;
	vec3_t	modelpos;
	LPDIRECT3DDEVICE3 dev;

	VectorCopy(e->origin, modelpos);
	VectorCopy(e->angles, angles);

	if (e->movetype != MOVETYPE_NONE)
	{
		float f = 0.0;
		float d;
		if (e->animtime + 0.2 > cl.time && e->animtime != e->prevanimtime)
			f = (cl.time - e->animtime) / (e->animtime - e->prevanimtime);

		for (i = 0; i < 3; i++)
		{
			d = e->prevorigin[i] - e->origin[i];
			modelpos[i] -= d * f;
		}

		if (f > 0.0 && f < 1.5)
		{
			f = 1.0 - f;

			for (i = 0; i < 3; i++)
			{
				d = e->prevangles[i] - e->angles[i];
				if (d > 180.0)
					d -= 360.0;
				else if (d < -180.0)
					d += 360.0;

				angles[i] += d * f;
			}
		}
	}

	dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();

	if (dev)
	{
		D3DMATRIX world;
		vec3_t forward, right, up;
			
		DCV_FlushInline();

		AngleVectors(angles, forward, right, up);

		memset(&world, 0, sizeof(world));
		world._11 = forward[0];  world._12 = forward[1];  world._13 = forward[2];
		world._21 = -right[0];   world._22 = -right[1];   world._23 = -right[2];
		world._31 = up[0];       world._32 = up[1];       world._33 = up[2];
		world._41 = modelpos[0]; world._42 = modelpos[1]; world._43 = modelpos[2];
		world._44 = 1.0f;

		dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_WORLD, &world);
	}
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*


/*
==================
R_ApplySceneProjection
==================
*/
void R_ApplySceneProjection( void )
{
	DCV_BuildProjectionAndSetTransform(
		g_frustum_xmax, -g_frustum_xmax,
		g_frustum_ymax, -g_frustum_ymax,
		g_frustum_zn, gl_zmax.value,
		dc_msw.value * 0.75f,
		D3DTRANSFORMSTATE_PROJECTION );

	DCV_SetViewportDepthRange( dc_depthmin.value, dc_depthmax.value );
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel( cl_entity_t* e )
{
	vec3_t	point, forward, right, up;
	mspriteframe_t* frame;
	float			scale;
	msprite_t* psprite;
	colorVec		color;
	int             alpha;
	int             base;

	psprite = (msprite_t*)e->model->cache.data;

	frame = R_GetSpriteFrame( psprite, e->frame );

	scale = ( e->scale > 0.0f ) ? e->scale : 1.0f;

	if ( e->rendermode == kRenderNormal )
		r_blend = 1.0f;

	alpha = (int)( (float)e->renderamt * scale );
	if ( alpha < 0 )   alpha = 0;
	if ( alpha > 255 ) alpha = 255;

	R_SpriteColor( &color, e, (int)( r_blend * 255.0f ) );

	switch ( e->rendermode )
	{
	case kRenderNormal:
		DCV_SetColor( 255, 255, 255, 255 );
		DCV_TexState_Opaque();
		break;

	case kRenderTransColor:
	case kRenderTransTexture:
		DCV_TexState_Blend();
		DCV_SetColor( color.r, color.g, color.b, alpha );
		R_ApplySceneProjection();
		break;

	case kRenderGlow:
		DCV_TexState_Additive();
		DCV_SetColor( color.r, color.g, color.b, 255 );
		R_ApplySceneProjection();
		break;

	case kRenderTransAdd:
		DCV_TexState_Additive();
		DCV_SetColor( color.r, color.g, color.b, 255 );
		DCV_FlushApplyRenderState( D3DRENDERSTATE_ZENABLE,       FALSE );
		DCV_FlushApplyRenderState( D3DRENDERSTATE_ZWRITEENABLE,   FALSE );
		R_ApplySceneProjection();
		break;

	case kRenderTransAlpha:
		DCV_TexState_AlphaTest();
		DCV_SetColor( color.r, color.g, color.b, alpha );
		break;

	default:
		DCV_TexState_Blend();
		DCV_SetColor( color.r, color.g, color.b, alpha );
		break;
	}

	R_GetSpriteAxes( e, psprite->type, forward, right, up );
	DCV_FlushInline();
	GL_Bind(frame->gl_texturenum, 0);

	if ( !DCV_EnsureSpace( 4, 6 ) )
		return;

	base = DCV_GetVertCount();

	/* v0: bottom-left */
	VectorMA( r_entorigin, frame->down * scale, up,    point );
	VectorMA( point,       frame->left * scale, right, point );
	DCV_AddVertex( point[0], point[1], point[2], 0.0f, 1.0f );

	/* v1: top-left */
	VectorMA( r_entorigin, frame->up   * scale, up,    point );
	VectorMA( point,       frame->left * scale, right, point );
	DCV_AddVertex( point[0], point[1], point[2], 0.0f, 0.0f );

	/* v2: top-right */
	VectorMA( r_entorigin, frame->up    * scale, up,    point );
	VectorMA( point,       frame->right * scale, right, point );
	DCV_AddVertex( point[0], point[1], point[2], 1.0f, 0.0f );

	/* v3: bottom-right */
	VectorMA( r_entorigin, frame->down  * scale, up,    point );
	VectorMA( point,       frame->right * scale, right, point );
	DCV_AddVertex( point[0], point[1], point[2], 1.0f, 1.0f );

	DCV_AddIndicesQuad( base, base + 1, base + 2, base + 3 );
	DCV_FlushIfLarge();

	DCV_FlushApplyRenderState( D3DRENDERSTATE_ZENABLE,      D3DZB_TRUE );
	DCV_FlushApplyRenderState( D3DRENDERSTATE_ZWRITEENABLE,  TRUE );

	if ( e->rendermode == kRenderGlow ||
	     e->rendermode == kRenderTransAdd ||
	     e->rendermode == kRenderTransColor ||
	     e->rendermode == kRenderTransTexture )
	{
		DCV_BuildProjectionAndSetTransform(
			g_frustum_xmax, -g_frustum_xmax,
			g_frustum_ymax, -g_frustum_ymax,
			g_frustum_zn, gl_zmax.value,
			dc_msw.value,
			D3DTRANSFORMSTATE_PROJECTION );
		DCV_SetViewportDepthRange( dc_depthmin.value, dc_depthmax.value );
		DCV_FlushInline();
	}
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162
float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
float	shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] = {
#include "anorm_dots.h"
};

float* shadedots = r_avertexnormal_dots[0];

int	lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void R_DrawAliasFrame( aliashdr_t* paliashdr, int posenum )
{
	Sys_Error ("R_DrawAliasFrame should be obsolete\n");
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void R_DrawAliasShadow( aliashdr_t* paliashdr, int posenum )
{
	Sys_Error ("R_DrawAliasShadow should be obsolete\n");
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame( int frame, aliashdr_t* paliashdr )
{
	Sys_Error ("R_SetupAliasFrame should be obsolete\n");
}



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel( cl_entity_t* e )
{
	Sys_Error ("R_DrawAliasModel should be obsolete\n");
}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList( void )
{
	int		i;

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
			R_DrawAliasModel(currententity);
			break;

		case mod_studio:
			if (currententity->index > 0 && currententity->index <= cl.maxclients)
			{
				R_StudioDrawPlayer(STUDIO_RENDER | STUDIO_EVENTS,
					&cl.frames[cl.parsecount & UPDATE_MASK].playerstate[currententity->index - 1]);
			}
			else
			{
				R_StudioDrawModel(STUDIO_RENDER | STUDIO_EVENTS);
			}
			break;

		default:
			break;
		}
	}

	r_blend = 1.0;

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
				float* pAttachment;

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
	float		lightvec[3];
	colorVec	c;
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add, oldShadows;
	dlight_t* dl;

	lightvec[0] = -1;
	lightvec[1] = 0;
	lightvec[2] = 0;

	currententity = &cl.viewent;

	if (!r_drawviewmodel.value)
	{
		c = R_LightPoint(currententity->origin);
		cl.light_level = (c.r + c.g + c.b) / 3;
		return;
	}

	// Don't draw if we are in a third person mode
	if (cam_thirdperson || chase_active.value || envmap || !r_drawentities.value)
	{
		c = R_LightPoint(currententity->origin);
		cl.light_level = (c.r + c.g + c.b) / 3;
		return;
	}

	if (cl.stats[STAT_HEALTH] <= 0 || !currententity->model || cl.viewentity > cl.maxclients || cl.spectator)
	{
		c = R_LightPoint(currententity->origin);
		cl.light_level = (c.r + c.g + c.b) / 3;
		return;
	}


		DCV_BuildProjectionAndSetTransform(
			g_frustum_xmax, -g_frustum_xmax,
			g_frustum_ymax, -g_frustum_ymax,
			g_frustum_zn, gl_zmax.value,
			dc_msv.value,
			D3DTRANSFORMSTATE_PROJECTION );

		DCV_SetViewportDepthRange( dc_depthmin.value, dc_depthmax.value );
		DCV_FlushInline();

		switch (currententity->model->type)
		{
		case mod_brush:
			R_DrawBrushModel(currententity);
			break;

		case mod_alias:
			R_DrawAliasModel(currententity);
			break;

		case mod_studio:
			if (cl.weaponstarttime == 0.0)
				cl.weaponstarttime = cl.time;
			
			currententity->sequence = cl.weaponsequence;
			currententity->frame = 0.0;
			currententity->framerate = 1.0;
			currententity->animtime = cl.weaponstarttime;

			cl.light_level = 128;

			oldShadows = r_shadows.value;
			r_shadows.value = 0.0;
			R_StudioDrawModel(STUDIO_RENDER);
			r_shadows.value = oldShadows;
			break;
		}

		DCV_FlushInline();

		DCV_BuildProjectionAndSetTransform(
			g_frustum_xmax, -g_frustum_xmax,
			g_frustum_ymax, -g_frustum_ymax,
			g_frustum_zn, gl_zmax.value,
			dc_msw.value,
			D3DTRANSFORMSTATE_PROJECTION );

		DCV_SetViewportDepthRange( dc_depthmin.value, dc_depthmax.value );
		DCV_FlushInline();
}

void R_PreDrawViewModel( void )
{
	currententity = &cl.viewent;

	// Don't draw if it's disabled
	if (!r_drawviewmodel.value)
		return;

	// Don't draw if we are in a third person mode
	if (cam_thirdperson || chase_active.value || envmap || !r_drawentities.value)
		return;

	if (cl.stats[STAT_HEALTH] <= 0 || !currententity->model || cl.viewentity > cl.maxclients || cl.spectator)
		return;

	if (cl.viewent.model->type != mod_studio)
		return;

	if (cl.weaponstarttime == 0.0)
		cl.weaponstarttime = cl.time;

	cl.viewent.frame = 0.0;
	cl.viewent.framerate = 1.0;
	cl.viewent.sequence = cl.weaponsequence;
	cl.viewent.animtime = cl.weaponstarttime;

	R_StudioDrawModel(STUDIO_EVENTS);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend( void )
{
	unsigned char color[4];
	int alpha;

	alpha = V_FadeAlpha();
	if (!alpha)
		return;
#if 0
	GL_DisableMultitexture();
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_TEXTURE_2D);
	if (cl.sf.fadeFlags & FFADE_MODULATE)
	{
		qglBlendFunc(GL_ZERO, GL_SRC_COLOR);
		color[0] = (alpha * (cl.sf.fader - 255) - 511) >> 8;
		color[1] = (alpha * (cl.sf.fadeg - 255) - 511) >> 8;
		color[2] = (alpha * (cl.sf.fadeb - 255) - 511) >> 8;
		color[3] = 255;
	}
	else
	{
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		color[0] = cl.sf.fader;
		color[1] = cl.sf.fadeg;
		color[2] = cl.sf.fadeb;
		color[3] = alpha;
	}

	qglLoadIdentity();

	qglRotatef(-90, 1, 0, 0);	    // put Z going up
	qglRotatef(90, 0, 0, 1);	    // put Z going up

	qglColor4ubv(color);

	qglBegin(GL_QUADS);

	qglVertex3f(10, 10, 10);
	qglVertex3f(10, -10, 10);
	qglVertex3f(10, -10, -10);
	qglVertex3f(10, 10, -10);
	qglEnd();

	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	qglEnable(GL_ALPHA_TEST);
#endif
}


int SignbitsForPlane( mplane_t* out )
{
	int bits, j;

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
	int		i;
	float	viewWidth;
	float	viewHeight;
	float	fovx, fovy;

	fovx = scr_fov_value;
	viewWidth = (float)r_refdef.vrect.width;
	viewHeight = (float)r_refdef.vrect.height;
	if (viewWidth <= 0.0f)
		viewWidth = (float)glwidth;
	if (viewHeight <= 0.0f)
		viewHeight = (float)glheight;

	fovy = CalcFov(fovx, viewWidth, viewHeight);

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - fovx / 2));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - fovx / 2);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - fovy / 2);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - fovy / 2));

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
#if 0
	if (cl.waterlevel > 2)
	{
		float fogColor[4];

		// Calculate fog color
		fogColor[0] = cshift_water.destcolor[0] / 255.0;
		fogColor[1] = cshift_water.destcolor[1] / 255.0;
		fogColor[2] = cshift_water.destcolor[2] / 255.0;
		fogColor[3] = 1.0;

		qglFogi(GL_FOG_MODE, GL_LINEAR);
		qglFogfv(GL_FOG_COLOR, fogColor);
		qglFogf(GL_FOG_START, GL_ZERO);
		qglFogf(GL_FOG_END, 1536 - 4 * cshift_water.percent);
		qglEnable(GL_FOG);
	}
#endif

	V_CalcBlend();

	r_cache_thrash = FALSE;

	c_brush_polys = 0;
	c_alias_polys = 0;
}

#if 0
void MYgluPerspective( GLdouble fovy, GLdouble aspect,
	GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	qglFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}
#endif

/*
====================
CalcFov
====================
*/
float CalcFov( float fov_x, float width, float height )
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		fov_x = 90;	// error, set to 90

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL( void )
{
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	LPDIRECT3DDEVICE3 dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();
	LPDIRECT3DVIEWPORT3 vp3 = (LPDIRECT3DVIEWPORT3)Sys_GetD3DViewport();
	D3DVIEWPORT2 vp;
	D3DMATRIX proj, view, world, mtx;

	float	msw;
	float	msv;
	float	zn;
	float	zf;
	float	fovx;
	float	fovy_deg;
	float	fovy_rad;
	float	aspect;
	float	ymax, ymin, xmin, xmax;

	if (!dev)
		return;

	msw = dc_msw.value;
	if (msw < 0.03125f)
		msw = 0.03125f;
	else if (msw > 64.0f)
		msw = 64.0f;

	msv = dc_msv.value;
	if (msv < 0.05f)
		msv = 0.05f;
	else if (msv > 2.0f)
		msv = 2.0f;

	zn = 2.0f;
	zf = gl_zmax.value;
	if (zf < zn + 1.0f)
		zf = zn + 1.0f;

	fovx = scr_fov_value;

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

	memset(&vp, 0, sizeof(vp));
	vp.dwSize = sizeof(vp);
	vp.dwX = (DWORD)(glx + x);
	vp.dwY = (DWORD)(gly + y2);
	vp.dwWidth = (DWORD)w;
	vp.dwHeight = (DWORD)h;
	vp.dvClipX      = -1.0f;
	vp.dvClipY      =  1.0f;
	vp.dvClipWidth  =  2.0f;
	vp.dvClipHeight =  2.0f;
	vp.dvMinZ = dc_depthmin.value;
	vp.dvMaxZ = dc_depthmax.value;

	if (vp3 && vp3->lpVtbl)
	{
		vp3->lpVtbl->SetViewport2(vp3, &vp);
		dev->lpVtbl->SetCurrentViewport(dev, vp3);
	}

	DCV_SetViewportDepthRange( dc_depthmin.value, dc_depthmax.value );

	aspect = (glheight > 0) ? ((float)glwidth / (float)glheight) : 1.0f;
	fovy_deg = CalcFov(fovx, (float)glwidth, (float)glheight);
	fovy_rad = (fovy_deg * (float)M_PI) / 180.0f;
	fovy_rad *= (0.3f / msv);

	ymax = zn * (float)tan(fovy_rad * 0.5f);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	DCV_BuildProjectionAndSetTransform(
		xmax, xmin,
		ymax, ymin,
		zn, zf,
		msw,
		D3DTRANSFORMSTATE_PROJECTION );

	g_frustum_xmax = xmax;
	g_frustum_ymax = ymax;
	g_frustum_zn   = zn;

	memset(&world, 0, sizeof(world));
	world._11 = world._22 = world._33 = world._44 = 1.0f;
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_WORLD, &world);

	memset(&view, 0, sizeof(view));
	view._11 = view._22 = view._33 = view._44 = 1.0f;
	dev->lpVtbl->SetTransform(dev, D3DTRANSFORMSTATE_VIEW, &view);

	DCV_Rotate(-90.0f, 1.0f, 0.0f, 0.0f, D3DTRANSFORMSTATE_VIEW);
	DCV_Rotate(90.0f, 0.0f, 0.0f, 1.0f, D3DTRANSFORMSTATE_VIEW);
	DCV_Rotate(-r_refdef.viewangles[2], 1.0f, 0.0f, 0.0f, D3DTRANSFORMSTATE_VIEW);
	DCV_Rotate(-r_refdef.viewangles[0], 0.0f, 1.0f, 0.0f, D3DTRANSFORMSTATE_VIEW);
	DCV_Rotate(-r_refdef.viewangles[1], 0.0f, 0.0f, 1.0f, D3DTRANSFORMSTATE_VIEW);
	memset(&mtx, 0, sizeof(mtx));
	mtx._11 = 1.0f; mtx._22 = 1.0f; mtx._33 = 1.0f; mtx._44 = 1.0f;
	mtx._41 = -r_refdef.vieworg[0];
	mtx._42 = -r_refdef.vieworg[1];
	mtx._43 = -r_refdef.vieworg[2];
	dev->lpVtbl->MultiplyTransform(dev, D3DTRANSFORMSTATE_VIEW, &mtx);

	dev->lpVtbl->GetTransform(dev, D3DTRANSFORMSTATE_VIEW, &view);
	gViewMatrix = view;

	DCV_SetRenderState(D3DRENDERSTATE_ZENABLE,     D3DZB_TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);

	memcpy(r_world_matrix, &view, sizeof(view));

	DCV_TexState_Opaque();
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene( void )
{
	R_SetupFrame();

	R_SetFrustum();

	R_SetupGL();

	R_MarkLeaves();	// done here so we know if we're in water

	R_DrawWorld();		// adds static entities to the list

	S_ExtraUpdate();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList();

	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	
	R_DrawTEntitiesOnList();

	S_ExtraUpdate();

	R_RenderDlights();

	GL_DisableMultitexture();

	R_DrawParticles();
}


/*
=============
R_Clear
=============
*/
void R_Clear( void )
{
	LPDIRECT3DVIEWPORT3 vp3 = (LPDIRECT3DVIEWPORT3)Sys_GetD3DViewport();
	LPDIRECT3DDEVICE3 dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();
	DWORD clear_flags;
	D3DCOLOR clear_color = 0xFF000000; /* black */
	D3DVALUE clear_z = 1.0f;

	if (!vp3 || !vp3->lpVtbl)
		return;


	clear_flags = (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);

	if (r_mirroralpha.value != 1.0f)
	{
		gldepthmin = 0.0f;
		gldepthmax = 0.5f;
	}
	else
	{
		gldepthmin = 0.0f;
		gldepthmax = 1.0f;
	}

	if (dev && dev->lpVtbl)
		dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);

	if (clear_flags)
		vp3->lpVtbl->Clear2(vp3, 0, NULL, clear_flags, clear_color, clear_z, 0);
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
	double	time1 = 0, time2;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error("	 NULL worldmodel");

	if (r_speeds.value)
	{
		time1 = Sys_FloatTime();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = FALSE;

	R_Clear();

	R_PreDrawViewModel();

	R_RenderScene();

	R_DrawViewModel();

	R_PolyBlend();

	S_ExtraUpdate();

	if (r_speeds.value)
	{
		static double oldrealtime = 0;
		float framerate = Sys_FloatTime() - oldrealtime;
		oldrealtime = Sys_FloatTime();

		if (framerate > 0.0)
			framerate = 1.0 / framerate;

		time2 = Sys_FloatTime();
		Con_Printf("%3ifps %3i ms  %4i wpoly %4i epoly\n", (int)(framerate + 0.5), (int)((time2 - time1) * 1000.0), c_brush_polys, c_alias_polys);
	}
}
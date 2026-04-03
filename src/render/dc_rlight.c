// gl_rlight.c

#include "quakedef.h"

extern int DCV_LT2Decode( const byte* in, int in_size, color24* out, int w, int h );

/* Maximum lightmap grid dimensions: extents capped at 2040 world units → (2040>>4)+1 = 128+1 */
#define MAX_LM_AXIS 129
#define MAX_LM_SAMPLE (MAX_LM_AXIS * MAX_LM_AXIS)

int	r_dlightframecount;
int	r_dlightchanged;
int r_dlightactive;


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight( void )
{
	int			i, j, k;

//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl.time * 10);
	for (j = 0; j < MAX_LIGHTSTYLES; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		k = k * 22;
		d_lightstylevalue[j] = k;
	}
}

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend( float r, float g, float b, float a2 )
{
	float	a;

	v_blend[3] = a = v_blend[3] + a2 * (1 - v_blend[3]);

	a2 = a2 / a;

	v_blend[0] = v_blend[1] * (1 - a2) + r * a2;
	v_blend[1] = v_blend[1] * (1 - a2) + g * a2;
	v_blend[2] = v_blend[2] * (1 - a2) + b * a2;
}

void R_RenderDlight( dlight_t* light )
{
	int		i, j;
	float	a;
	vec3_t	v;
	float	rad;

	rad = light->radius * 0.35;

	VectorSubtract(light->origin, r_origin, v);
	if (Length(v) < rad)
	{	// view is inside the dlight
		AddLightBlend(1, 0.5, 0, light->radius * 0.0003);
		return;
	}

#if 0
	qglBegin(GL_TRIANGLE_FAN);
	qglColor3f(0.2, 0.1, 0.0);
	for (i = 0; i < 3; i++)
		v[i] = light->origin[i] - vpn[i] * rad;
	qglVertex3fv(v);
	qglColor3f(0, 0, 0);
	for (i = 16; i >= 0; i--)
	{
		a = i / 16.0 * M_PI * 2;
		for (j = 0; j < 3; j++)
			v[j] = light->origin[j] + vright[j] * cos(a) * rad
				+ vup[j] * sin(a) * rad;
		qglVertex3fv(v);
	}
	qglEnd();
#endif
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights( void )
{
	int		i;
	dlight_t* l;

	if (!gl_flashblend.value)
		return;

	r_dlightframecount = r_framecount + 1; // because the count hasn't
										//  advanced yet for this frame
#if 0
	qglDepthMask(GL_FALSE);
	qglDisable(GL_TEXTURE_2D);
	qglShadeModel(GL_SMOOTH);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);

	l = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_RenderDlight(l);
	}

	qglColor3f(1, 1, 1);
	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask(GL_TRUE);
#endif
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights( dlight_t* light, int bit, mnode_t* node )
{
	mplane_t*	splitplane;
	float		dist;
	msurface_t* surf;
	int			i;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		R_MarkLights(light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		R_MarkLights(light, bit, node->children[1]);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		float rad, minlight;
		int s, t;
		int smax, tmax;
		mtexinfo_t* tex;

		rad = light->radius - fabs(dist);
		if (light->minlight > rad)
			continue;

		tex = surf->texinfo;

		minlight = rad - light->minlight;

		// Project light center into texture coordinates
		s = DotProduct(light->origin, tex->vecs[0])
			+ tex->vecs[0][3] - surf->texturemins[0];
		t = DotProduct(light->origin, tex->vecs[1])
			+ tex->vecs[1][3] - surf->texturemins[1];

		smax = surf->extents[0];
		tmax = surf->extents[1];

		if (s <= -minlight || t <= -minlight || s > minlight + smax || t > minlight + tmax)
		{
			continue;
		}

		if (surf->dlightframe != (byte)r_dlightframecount)
		{
			surf->dlightframe = (byte)r_dlightframecount;
			surf->dlightbits = 0;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights(light, bit, node->children[0]);
	R_MarkLights(light, bit, node->children[1]);
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights( void )
{
	int		i;
	dlight_t* l;

	if (gl_flashblend.value)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_MarkLights(l, 1 << i, cl.worldmodel->nodes);
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t* lightplane;
vec3_t			lightspot;

colorVec RecursiveLightPoint( mnode_t* node, vec_t* start, vec_t* end )
{
	colorVec	c;
	float		front, back, frac;
	int			side;
	mplane_t* plane;
	vec3_t		mid;
	msurface_t* surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t* tex;
	color24* lightmap;
	unsigned	scale;
	int			maps;

	if (node->contents < 0)		// didn't hit anything
	{
	// clear to no light
		c.r = c.g = c.b = c.a = 0;
		return c;
	}

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct(start, plane->normal) - plane->dist;
	back = DotProduct(end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint(node->children[side], start, end);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

// go down front side
	c = RecursiveLightPoint(node->children[side], start, mid);
	if (c.r != 0 || c.g != 0 || c.b != 0)
		return c;		// hit something

	if ((back < 0) == side)		// didn't hit anuthing
	{
	// clear to no light
		c.r = c.g = c.b = c.a = 0;
		return c;
	}

// check for impact on this node
	VectorCopy(mid, lightspot);
	lightplane = plane;

	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];

		if (s < surf->texturemins[0] ||
			t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
		{
		// clear to no light
			c.r = c.g = c.b = c.a = 0;
			return c;
		}

		ds >>= 4;
		dt >>= 4;

	// clear to no light
		c.r = c.g = c.b = c.a = 0;

		if (cl.worldmodel->lightmap_mode >= 2)
		{
			/* LT2 mode: surf->samples points into lightpayload; decode style 0 only for speed. */
			int smax = (surf->extents[0] >> 4) + 1;
			int tmax = (surf->extents[1] >> 4) + 1;
			static color24 lt2_tmp[MAX_LM_SAMPLE];
			int consumed = DCV_LT2Decode((byte*)surf->samples,
				(int)((cl.worldmodel->lightpayload + cl.worldmodel->lightBytes) - (byte*)surf->samples),
				lt2_tmp, smax, tmax);
			if (consumed > 0 && surf->styles[0] != 255)
			{
				int idx = dt * smax + ds;
				scale = d_lightstylevalue[surf->styles[0]];
				c.r = (lt2_tmp[idx].r * scale) >> 8;
				c.g = (lt2_tmp[idx].g * scale) >> 8;
				c.b = (lt2_tmp[idx].b * scale) >> 8;
				if (c.r > 255) c.r = 255;
				if (c.g > 255) c.g = 255;
				if (c.b > 255) c.b = 255;
			}
		}
		else
		{
			/* Standard BSP color24 lightdata */
			lightmap = surf->samples;
			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				c.r += lightmap->r * scale;
				c.g += lightmap->g * scale;
				c.b += lightmap->b * scale;
				lightmap += ((surf->extents[0] >> 4) + 1) *
					((surf->extents[1] >> 4) + 1);
			}

			c.r >>= 8;
			c.g >>= 8;
			c.b >>= 8;
		}

		return c;
	}

// go down back side
	return RecursiveLightPoint(node->children[!side], mid, end);
}

colorVec R_LightVec( vec_t* start, vec_t* end )
{
	colorVec	c;

	if (!cl.worldmodel->lightdata && !cl.worldmodel->lightpayload)
	{
		c.r = c.g = c.b = 255;
		c.a = 0;
		return c;
	}

	c = RecursiveLightPoint(cl.worldmodel->nodes, start, end);

	c.r += r_refdef.ambientlight.r;
	c.g += r_refdef.ambientlight.g;
	c.b += r_refdef.ambientlight.b;

	if (c.r > 255)
		c.r = 255;
	if (c.g > 255)
		c.g = 255;
	if (c.b > 255)
		c.b = 255;

	return c;
}

colorVec R_LightPoint( vec_t* p )
{
	vec3_t		end;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	return R_LightVec(p, end);
}
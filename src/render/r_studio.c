// r_studio.c: routines for setting up to draw 3DStudio models

#include <shintr.h>

#include "quakedef.h"
#include "dreamcast_crt.h"
#include "pr_cmds.h"
#include "view.h"
#include "CL_TENT.H"
#include "customentity.h"
#include "r_triangle.h"
#include "r_studio.h"
#include "dc_draw.h"
#include "dc_accum.h"

// Hulls & planes
#define STUDIO_NUM_HULLS	128
#define STUDIO_NUM_PLANES	(STUDIO_NUM_HULLS * 6)

// Pointer to header block for studio model data
studiohdr_t* pstudiohdr;

vec3_t		r_colormix;
colorVec	r_icolormix;
vec3_t		r_blightvec[MAXSTUDIOBONES];	// light vectors in bone reference frames

// Model to world transformation
float		rotationmatrix[3][4];

// Concatenated bone and light transforms
float		bonetransform[MAXSTUDIOBONES][4][4];
float		lighttransform[MAXSTUDIOBONES][3][4];
int			cached_numbones;
char		cached_bonename[MAXSTUDIOBONES * 32];
float		cached_bonetransform[STUDIO_NUM_HULLS][4][4];
float		cached_lighttransform[STUDIO_NUM_HULLS][3][4];

// Vert data, position and lighting
auxvert_t	auxverts[MAXSTUDIOVERTS];
vec3_t		lightvalues[MAXSTUDIOVERTS];

// Global studio cache data, hulls and planes
int			cache_hull_hitgroup[STUDIO_NUM_HULLS];
hull_t		cache_hull[STUDIO_NUM_HULLS];
mplane_t	cache_planes[STUDIO_NUM_PLANES];
int			nCurrentHull;
int			nCurrentPlane;

// Caching
// Studio cache data
typedef struct
{
	float		frame;
	int			sequence;

	vec3_t		angles;
	vec3_t		origin;
	vec3_t		size;

	unsigned char controller[4]; // bone controller
	unsigned char blending[2];

	model_t*	pModel;	// model instance

	int			nStartHull;
	int			nStartPlane;

	int			numhulls;
} r_studiocache_t;


#define STUDIO_CACHE_SIZE	16
#define STUDIO_CACHEMASK	(STUDIO_CACHE_SIZE - 1)

r_studiocache_t rgStudioCache[STUDIO_CACHE_SIZE];
int			r_cachecurrent;

// Do interpolation?
int			r_dointerp = 1;

//
// Global studio hull/clipnode/plane data to
// copy the cached ones
int			studio_hull_hitgroup[STUDIO_NUM_HULLS];
hull_t		studio_hull[STUDIO_NUM_HULLS];
dclipnode_t	studio_clipnodes[6];
mplane_t	studio_planes[STUDIO_NUM_PLANES];

void R_StudioTransformAuxVert( auxvert_t* av, int bone, vec_t* vert );
void StudioTransformVerts( auxvert_t* out, const char* bones, vec3_t* verts, int count );
void R_StudioTransformChromeVerts( vec3_t* out, const char* bones, vec3_t* verts, int count );
void R_StudioRenderMeshChrome( void );
void R_StudioBuildTriangleStrips( short* commands );
studiohdr_t* R_StudioGetTextureHeader( model_t* model );
void R_StudioSetupPlayerSkin( studiohdr_t* textureHeader, int textureIndex );
void R_StudioRemapPaletteRange( byte* palette, int color, int first, int last );
void R_StudioChromeForMesh( int count, int normalIndex, const char* pnormbone, const vec3_t* pstudionorms );
void R_StudioSetupSkin( mstudiotexture_t* ptexture );
void R_LightStrength( int bone, float* vert, float(*light)[4] );
void R_StudioLighting( float* lv, int bone, int flags, vec_t* normal );
void R_StudioSaveBones( void );
void R_StudioCalcAttachments( void );
void R_StudioEstimateGait( player_state_t* player );
void R_StudioProcessGait( player_state_t* player );
void R_StudioResetPlayerModel( void );
static void R_StudioPlayerBlend( mstudioseqdesc_t* sequence, int* blend, float* pitch );

static studiohdr_t* (* volatile g_pStudioGetTextureHeader)(model_t*) = R_StudioGetTextureHeader;
static void(* volatile g_pStudioSetupPlayerSkin)(studiohdr_t*, int) = R_StudioSetupPlayerSkin;

extern vec3_t shadevector;

// Pointers to current body part and submodel
mstudiobodyparts_t* pbodypart;
mstudiomodel_t* psubmodel;
mstudiomesh_t* pmesh;
model_t*	r_studio_model;
player_info_t* r_playerinfo;
int			r_playerindex;
float		r_gaitmovement;
int			r_topcolor;
int			r_bottomcolor;

typedef struct studio_player_model_s
{
	char		name[MAX_OSPATH];
	char		modelName[MAX_OSPATH];
	model_t*	model;
} studio_player_model_t;

/* Preserve the player entity while drawing an attached weapon. */
typedef struct studio_entity_snapshot_s
{
	int					data[sizeof(cl_entity_t) / sizeof(int)];
} studio_entity_snapshot_t;

studio_player_model_t g_studioPlayerModels[MAX_CLIENTS];

typedef struct studio_skin_cache_s
{
	int			playerIndex;
	int			topColor;
	int			bottomColor;
	model_t*	model;
	char		textureName[STUDIO_SKIN_CACHE_NAME_LENGTH];
	byte		skinState[STUDIO_SKIN_STATE_BYTES];
	int			textureIndex;
	int			textureState;
	int			width;
	int			height;
	cache_user_t pixels;
	int			glTexture;
} studio_skin_cache_t;

static studio_skin_cache_t g_studioSkinCache[STUDIO_SKIN_CACHE_COUNT];
static studio_skin_cache_t* g_studioSkinByPlayer[STUDIO_SKIN_PLAYER_SLOTS];
static int	g_studioSkinCacheCursor;
byte		g_studioTranslatedPalette[STUDIO_PALETTE_RGB_BYTES];

studio_skin_cache_t* R_StudioGetPlayerSkinCache( int playerIndex );
void R_StudioLoadPlayerSkin( model_t* model, int textureIndex, studio_skin_cache_t* cache );


// Chrome and light data

float		chrome[MAXSTUDIOVERTS][2];		// texture coords for surface normals
int			g_NormalIndex[MAXSTUDIOVERTS];
int			chromeage[MAXSTUDIOBONES];		// last time chrome vectors were updated
vec3_t		r_chromeup[MAXSTUDIOBONES];		// chrome vector "up" in bone reference frames
vec3_t		r_chromeright[MAXSTUDIOBONES];	// chrome vector "right" in bone reference frames
vec3_t		g_ChromeOrigin;
#define MAXLOCALLIGHTS 3
int			numlights;
dlight_t*	locallight[MAXLOCALLIGHTS];
int			locallinearlight[MAXLOCALLIGHTS][3];
float		locallightR2[MAXLOCALLIGHTS];
float		lightpos[MAXSTUDIOVERTS][3][4];
vec_t		lightbonepos[MAXSTUDIOBONES][3][3];
int			lightage[MAXSTUDIOBONES];					// last time lights were updated

auxvert_t*	pauxverts = auxverts;

// Software's drawstyle for debugging
// the studio model
int			drawstyle;

extern vec3_t* pvlightvalues;

int			r_ambientlight;					// ambient world light
float		r_shadelight;					// direct world light

int			r_amodels_drawn;
int			r_smodels_total;				// cookie
int			r_studio_clip_required;
int			g_ForcedFaceFlags;


void R_StudioTransformVector( vec_t* in, vec_t* out );
int SignbitsForPlane( mplane_t* out );

void StudioRotateBonesX( float matrix[4][4], float angle )
{
	float		s, c;
	float		x0, x1, x2, x3;
	float		y0, y1, y2, y3;

	angle *= 0.017453292f;
	s = sin(angle);
	c = cos(angle);

	x0 = matrix[0][1];
	y0 = matrix[0][2];
	x1 = matrix[1][1];
	y1 = matrix[1][2];
	x2 = matrix[2][1];
	y2 = matrix[2][2];
	x3 = matrix[3][1];
	y3 = matrix[3][2];
	matrix[0][1] = x0 * c + y0 * s;
	matrix[1][1] = x1 * c + y1 * s;
	matrix[2][1] = x2 * c + y2 * s;
	matrix[3][1] = x3 * c + y3 * s;
	s = -s;
	matrix[0][2] = x0 * s + y0 * c;
	matrix[1][2] = x1 * s + y1 * c;
	matrix[2][2] = x2 * s + y2 * c;
	matrix[3][2] = x3 * s + y3 * c;
}

void StudioRotateBonesY( float matrix[4][4], float angle )
{
	float		s, c, ns;
	float		y0, y1, y2, y3;
	float		z0, z1, z2, z3;

	angle *= 0.017453292f;
	s = sin(angle);
	c = cos(angle);

	y0 = matrix[0][0];
	z0 = matrix[0][2];
	y1 = matrix[1][0];
	z1 = matrix[1][2];
	y2 = matrix[2][0];
	z2 = matrix[2][2];
	y3 = matrix[3][0];
	z3 = matrix[3][2];
	ns = -s;
	matrix[0][0] = y0 * c + z0 * ns;
	matrix[1][0] = y1 * c + z1 * ns;
	matrix[2][0] = y2 * c + z2 * ns;
	matrix[3][0] = y3 * c + z3 * ns;
	matrix[0][2] = y0 * s + z0 * c;
	matrix[1][2] = y1 * s + z1 * c;
	matrix[2][2] = y2 * s + z2 * c;
	matrix[3][2] = y3 * s + z3 * c;
}

void StudioRotateBonesZ( float matrix[4][4], float angle )
{
	float		s, c;
	float		z0, z1, z2, z3;
	float		x0, x1, x2, x3;

	angle *= 0.017453292f;
	s = sin(angle);
	c = cos(angle);

	z0 = matrix[0][0];
	x0 = matrix[0][1];
	z1 = matrix[1][0];
	x1 = matrix[1][1];
	z2 = matrix[2][0];
	x2 = matrix[2][1];
	z3 = matrix[3][0];
	x3 = matrix[3][1];
	matrix[0][0] = z0 * c + x0 * s;
	matrix[1][0] = z1 * c + x1 * s;
	matrix[2][0] = z2 * c + x2 * s;
	matrix[3][0] = z3 * c + x3 * s;
	s = -s;
	matrix[0][1] = z0 * s + x0 * c;
	matrix[1][1] = z1 * s + x1 * c;
	matrix[2][1] = z2 * s + x2 * c;
	matrix[3][1] = z3 * s + x3 * c;
}

void StudioIdentityMatrix( float matrix[4][4] )
{
	matrix[0][0] = 1.0f;
	matrix[1][0] = 0.0f;
	matrix[2][0] = 0.0f;
	matrix[3][0] = 0.0f;
	matrix[0][1] = 0.0f;
	matrix[1][1] = 1.0f;
	matrix[2][1] = 0.0f;
	matrix[3][1] = 0.0f;
	matrix[0][2] = 0.0f;
	matrix[1][2] = 0.0f;
	matrix[2][2] = 1.0f;
	matrix[3][2] = 0.0f;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
	matrix[3][3] = 1.0f;
}

void StudioComputeBBox( vec3_t mins, vec3_t maxs, const vec3_t angles )
{
	float		matrix[4][4];
	float		axes[3][4];
	vec3_t		points[8];
	vec3_t		transformed[8];
	int			i, j;

	points[0][0] = mins[0];
	points[0][1] = mins[1];
	points[0][2] = mins[2];
	points[1][0] = mins[0];
	points[1][1] = mins[1];
	points[1][2] = maxs[2];
	points[2][0] = mins[0];
	points[2][1] = maxs[1];
	points[2][2] = mins[2];
	points[3][0] = mins[0];
	points[3][1] = maxs[1];
	points[3][2] = maxs[2];
	points[4][0] = maxs[0];
	points[4][1] = mins[1];
	points[4][2] = mins[2];
	points[5][0] = maxs[0];
	points[5][1] = mins[1];
	points[5][2] = maxs[2];
	points[6][0] = maxs[0];
	points[6][1] = maxs[1];
	points[6][2] = mins[2];
	points[7][0] = maxs[0];
	points[7][1] = maxs[1];
	points[7][2] = maxs[2];

	StudioIdentityMatrix(matrix);
	StudioRotateBonesZ(matrix, angles[YAW]);
	StudioRotateBonesY(matrix, angles[PITCH]);
	StudioRotateBonesX(matrix, angles[ROLL]);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			axes[i][j] = matrix[i][j];
	}

	transformed[0][0] = _Dot3dVW0(points[0], axes[0]);
	transformed[0][1] = _Dot3dVW0(points[0], axes[1]);
	transformed[0][2] = _Dot3dVW0(points[0], axes[2]);
	transformed[1][0] = _Dot3dVW0(points[1], axes[0]);
	transformed[1][1] = _Dot3dVW0(points[1], axes[1]);
	transformed[1][2] = _Dot3dVW0(points[1], axes[2]);
	transformed[2][0] = _Dot3dVW0(points[2], axes[0]);
	transformed[2][1] = _Dot3dVW0(points[2], axes[1]);
	transformed[2][2] = _Dot3dVW0(points[2], axes[2]);
	transformed[3][0] = _Dot3dVW0(points[3], axes[0]);
	transformed[3][1] = _Dot3dVW0(points[3], axes[1]);
	transformed[3][2] = _Dot3dVW0(points[3], axes[2]);
	transformed[4][0] = _Dot3dVW0(points[4], axes[0]);
	transformed[4][1] = _Dot3dVW0(points[4], axes[1]);
	transformed[4][2] = _Dot3dVW0(points[4], axes[2]);
	transformed[5][0] = _Dot3dVW0(points[5], axes[0]);
	transformed[5][1] = _Dot3dVW0(points[5], axes[1]);
	transformed[5][2] = _Dot3dVW0(points[5], axes[2]);
	transformed[6][0] = _Dot3dVW0(points[6], axes[0]);
	transformed[6][1] = _Dot3dVW0(points[6], axes[1]);
	transformed[6][2] = _Dot3dVW0(points[6], axes[2]);
	transformed[7][0] = _Dot3dVW0(points[7], axes[0]);
	transformed[7][1] = _Dot3dVW0(points[7], axes[1]);
	transformed[7][2] = _Dot3dVW0(points[7], axes[2]);

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 3; j++)
		{
			if (i == 0 || transformed[i][j] < mins[j])
				mins[j] = transformed[i][j];
			if (i == 0 || transformed[i][j] > maxs[j])
				maxs[j] = transformed[i][j];
		}
	}
}


/*
===========
R_StudioCheckBBox

Checks if entity's bbox is in the view frustum
===========
*/
qboolean R_StudioCheckBBox( void )
{
	mplane_t	plane;
	vec3_t		mins, maxs;
	int			side;
	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + currententity->sequence;
	VectorCopy(pseqdesc->bbmin, mins);
	VectorCopy(pseqdesc->bbmax, maxs);
	StudioComputeBBox(mins, maxs, currententity->angles);
	VectorAdd(mins, currententity->origin, mins);
	VectorAdd(maxs, currententity->origin, maxs);

	if (currententity->model &&
		!memcmp(currententity->model->name, "models/barnacle.mdl", 20))
		mins[2] -= 1024.0f;

	if (R_CullBox(mins, maxs))
		return FALSE;

	VectorCopy(vpn, plane.normal);
	plane.dist = DotProduct(plane.normal, r_origin);
	plane.type = PLANE_ANYZ;
	plane.signbits = SignbitsForPlane(&plane);

	side = BoxOnPlaneSide(mins, maxs, &plane);
	if (side == 3)
	{
		DCV_SetClipRequired();
		r_studio_clip_required = TRUE;
	}
	else
	{
		DCV_SetNoClip();
		r_studio_clip_required = FALSE;
	}

	return side != 2;
}


// Get number of body variations
int R_StudioBodyVariations( model_t* model )
{
	studiohdr_t* pstudiohdr;
	mstudiobodyparts_t* pbodypart;
	int			i, count;

	if (model->type != mod_studio)
		return 0;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(model);
	if (!pstudiohdr)
		return 0;
	if (Mod_IsStudioNeoModel(pstudiohdr))
		return R_StudioBodyVariations_Neo(model);

	count = 1;

	pbodypart = (mstudiobodyparts_t*)((byte*)pstudiohdr + pstudiohdr->bodypartindex);
	for (i = 0; i < pstudiohdr->numbodyparts; i++, pbodypart++)
	{
		count *= pbodypart->nummodels;
	}

	return count;
}

/*
================
R_StudioTransformVector
================
*/
void R_StudioTransformVector( vec_t* in, vec_t* out )
{
	out[0] = DotProduct(in, rotationmatrix[0]) + rotationmatrix[0][3];
	out[1] = DotProduct(in, rotationmatrix[1]) + rotationmatrix[1][3];
	out[2] = DotProduct(in, rotationmatrix[2]) + rotationmatrix[2][3];
}

float		h_scale = 1.5;

/*
================
R_StudioSetUpTransform
================
*/
void R_StudioSetUpTransform( int trivial_accept )
{
	int			i;
	vec3_t		angles;
	vec3_t		modelpos;

	// tweek model origin
		//for (i = 0; i < 3; i++)
		//	modelpos[i] = currententity->origin[i];

	VectorCopy(currententity->origin, modelpos);

	// TODO: should really be stored with the entity instead of being reconstructed
	// TODO: should use a look-up table
	// TODO: could cache lazily, stored in the entity
	angles[ROLL] = currententity->angles[ROLL];
	angles[PITCH] = currententity->angles[PITCH];
	angles[YAW] = currententity->angles[YAW];

	//Con_DPrintf("Angles %4.2f prev %4.2f for %i\n", angles[PITCH], currententity->index);
	//Con_DPrintf("movetype %d %d\n", currententity->movetype, currententity->aiment);
	if (currententity->movetype != MOVETYPE_NONE)
	{
		float		f = 0;
		float		d;

		// don't do it if the goalstarttime hasn't updated in a while.

		// NOTE:  Because we need to interpolate multiplayer characters, the interpolation time limit
		//  was increased to 1.0 s., which is 2x the max lag we are accounting for.

		if (cl.time < (currententity->animtime + 1.0f) &&
			(currententity->animtime != currententity->prevanimtime))
		{
			f = (cl.time - currententity->animtime) / (currententity->animtime - currententity->prevanimtime);
			//Con_DPrintf("%4.2f %.2f %.2f\n", f, currententity->animtime, cl.time);
		}

		if (r_dointerp)
		{
			// ugly hack to interpolate angle, position. current is reached 0.1 seconds after being set
			f = f - 1.0f;
		}
		else
		{
			f = 0;
		}

		for (i = 0; i < 3; i++)
		{
			modelpos[i] += (currententity->origin[i] - currententity->prevorigin[i]) * f;
		}

		// NOTE:  Because multiplayer lag can be relatively large, we don't want to cap
		//  f at 1.5 anymore.
		//if (f > -1.0 && f < 1.5) {}

//			Con_DPrintf("%.0f %.0f\n", currententity->msg_angles[0][YAW], currententity->msg_angles[1][YAW]);
		for (i = 0; i < 3; i++)
		{
			float		ang1, ang2;

			ang1 = currententity->angles[i];
			ang2 = currententity->prevangles[i];

			d = ang1 - ang2;
			if (d > 180)
			{
				d -= 360;
			}
			else if (d < -180)
			{
				d += 360;
			}

			angles[i] += d * f;
		}
		//Con_DPrintf("%.3f \n", f);
	}

	//Con_DPrintf("%.0f %0.f %0.f\n", modelpos[0], modelpos[1], modelpos[2]);
	//Con_DPrintf("%.0f %0.f %0.f\n", angles[0], angles[1], angles[2]);

	angles[PITCH] = -angles[PITCH];
	AngleMatrix(angles, rotationmatrix);

	rotationmatrix[0][3] = modelpos[0];
	rotationmatrix[1][3] = modelpos[1];
	rotationmatrix[2][3] = modelpos[2];
}

// rotate by the inverse of the matrix
void VectorIRotate( vec_t* in1, float(*in2)[4], vec_t* out )
{
	out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
	out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
	out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}

void AngleQuaternion( vec_t* angles, vec_t* quaternion )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[1] * 0.5f;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[0] * 0.5f;
	sr = sin(angle);
	cr = cos(angle);

	quaternion[0] = sr * cp * cy - cr * sp * sy; // X
	quaternion[1] = cr * sp * cy + sr * cp * sy; // Y
	quaternion[2] = cr * cp * sy - sr * sp * cy; // Z
	quaternion[3] = cr * cp * cy + sr * sp * sy; // W
}

void QuaternionMatrix( vec_t* quaternion, float(*matrix)[4] )
{
	float		x = quaternion[0];
	float		y = quaternion[1];
	float		z = quaternion[2];
	float		w = quaternion[3];
	float		x2 = x + x;
	float		y2 = y + y;
	float		z2 = z + z;
	float		xx = x * x2;
	float		xy = x * y2;
	float		xz = x * z2;
	float		yy = y * y2;
	float		yz = y * z2;
	float		zz = z * z2;
	float		wx = w * x2;
	float		wy = w * y2;
	float		wz = w * z2;

	matrix[0][0] = 1.0f - (yy + zz);
	matrix[0][1] = xy - wz;
	matrix[0][2] = wy + xz;
	matrix[1][0] = wz + xy;
	matrix[1][1] = 1.0f - (xx + zz);
	matrix[1][2] = yz - wx;
	matrix[2][0] = xz - wy;
	matrix[2][1] = wx + yz;
	matrix[2][2] = 1.0f - (xx + yy);
}

float		omega, cosom, sinom, sclp, sclq;
void QuaternionSlerp( vec_t* p, vec_t* q, float t, vec_t* qt )
{
	int			i;

	// decide if one of the quaternions is backwards
	float		a = 0;
	float		b = 0;

	for (i = 0; i < 4; i++)
	{
		a += (p[i] - q[i]) * (p[i] - q[i]);
		b += (p[i] + q[i]) * (p[i] + q[i]);
	}
	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if ((1.0f + cosom) > 0.001f)
	{
		if ((1.0f - cosom) > 0.001f)
		{
			omega = acos(cosom);
			sinom = sin(omega);
			sclp = sin((1.0f - t) * omega) / sinom;
			sclq = sin(omega * t) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++)
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin((1.0f - t) * (0.5f * (float)M_PI));
		sclq = sin(t * (0.5f * (float)M_PI));
		for (i = 0; i < 3; i++)
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

/*
====================
R_StudioCalcBoneAdj

Compute bone adjustments ( bone controllers )
====================
*/
void R_StudioCalcBoneAdj( float dadt, float* adj, const unsigned char* pcontroller1, const unsigned char* pcontroller2, unsigned char mouthopen )
{
	int			i, j;
	float		value;
	mstudiobonecontroller_t* pbonecontroller;

	pbonecontroller = (mstudiobonecontroller_t*)((byte*)pstudiohdr + pstudiohdr->bonecontrollerindex);

	for (j = 0; j < pstudiohdr->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int			a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = ((a * dadt) + (b * (1.0f - dadt)) - 128.0f) * (360.0f / 256.0f) + pbonecontroller[j].start;
				}
				else
				{
					value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0f - dadt)) * (360.0f / 256.0f) + pbonecontroller[j].start;
				}
			}
			else
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0f - dadt)) * (1.0f / 255.0f);
				if (value < 0.0f) value = 0.0f;
				if (value > 1.0f) value = 1.0f;
				value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			//Con_DPrintf("%d %d %f : %f\n", currententity->controller[j], currententity->prevcontroller[j], value, dadt);
		}
		else
		{
			// mouth hardcoded at controller 4
			value = mouthopen * (1.0f / 64.0f);
			if (value > 1.0f) value = 1.0f;
			value = (1.0f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			//Con_DPrintf("%d %f\n", mouthopen, value);
		}
		switch (pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = value * (3.141592654f / 180.0f);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}

void R_StudioCalcBoneQuaterion( int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* q )
{
	int			j, k;
	vec4_t		q1, q2;
	vec3_t		angle1, angle2;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j + 3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j + 3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t*)((byte*)panim + panim->offset[j + 3]);
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k + 1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k + 2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
			angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
		}

		if (pbone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j + 3]];
			angle2[j] += adj[pbone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	}
	else
	{
		AngleQuaternion(angle1, q);
	}
}

void R_StudioCalcBonePosition( int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* pos )
{
	int			j, k;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t*)((byte*)panim + panim->offset[j]);
			/*
			if (i == 0 && j == 0)
				Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s);
			*/

			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k + 1].value * (1.0f - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k + 1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1)
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}
}

float CL_StudioEstimateInterpolant( void )
{
	float		dadt;

	if (currententity->animtime >= currententity->prevanimtime + 0.01f)
	{
		dadt = (cl.time - currententity->animtime) / 0.1f;
		if (dadt > 2.0f)
		{
			dadt = 2.0f;
		}
	}
	else
	{
		dadt = 1.0f;
	}
	return dadt;
}

/*
====================
R_StudioCalcRotations

====================
*/
void R_StudioCalcRotations( vec3_t* pos, vec4_t* q, mstudioseqdesc_t* pseqdesc, mstudioanim_t* panim, float f )
{
	int			i;
	int			frame;
	mstudiobone_t* pbone;

	float		s;
	float		adj[MAXSTUDIOCONTROLLERS];
	float		dadt;

	if (f > pseqdesc->numframes - 1)
	{
		f = 0;	// bah, fix this bug with changing sequences too fast
	}

	frame = (int)f;

	// Con_DPrintf("%d %.4f %.4f %.4f %.4f %d\n", currententity->sequence, cl.time, currententity->animtime, currententity->frame, f, frame);

	// Con_DPrintf("%f %f %f\n", currententity->angles[ROLL], currententity->angles[PITCH], currententity->angles[YAW]);

	// Con_DPrintf("frame %d %d\n", frame1, frame2);


	dadt = CL_StudioEstimateInterpolant();
	s = (f - frame);

	// add in programtic controllers
	pbone = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);

	R_StudioCalcBoneAdj(dadt, adj, currententity->controller, currententity->prevcontroller, currententity->mouth.mouthopen);

	for (i = 0; i < pstudiohdr->numbones; i++, pbone++, panim++)
	{
		R_StudioCalcBoneQuaterion(frame, s, pbone, panim, adj, q[i]);

		R_StudioCalcBonePosition(frame, s, pbone, panim, adj, pos[i]);
		// if (0 && i == 0)
		//	Con_DPrintf("%d %d %d %d\n", currententity->sequence, frame, j, k);
	}

	if (pseqdesc->motiontype & STUDIO_X)
	{
		pos[pseqdesc->motionbone][0] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Y)
	{
		pos[pseqdesc->motionbone][1] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Z)
	{
		pos[pseqdesc->motionbone][2] = 0.0;
	}
}

/*
====================
R_GetAnim

====================
*/
mstudioanim_t* R_GetAnim( model_t* psubmodel, mstudioseqdesc_t* pseqdesc )
{
	mstudioseqgroup_t* pseqgroup;
	cache_user_t* paSequences;
	unsigned int data;

	pseqgroup = (mstudioseqgroup_t*)((byte*)pstudiohdr + pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;

	if (!pseqdesc->seqgroup)
		return (mstudioanim_t*)((byte*)pstudiohdr + pseqdesc->animindex + pseqgroup->data);

	paSequences = (cache_user_t*)psubmodel->submodels;
	if (!paSequences)
	{
		paSequences = (cache_user_t*)calloc(16, sizeof(cache_user_t));
		psubmodel->submodels = (dmodel_t*)paSequences;
	}

	if (!Cache_Check(&paSequences[pseqdesc->seqgroup]))
	{
		Cache_Lock(&psubmodel->cache);
		COM_LoadCacheFile(pseqgroup->name, &paSequences[pseqdesc->seqgroup]);
		Cache_Unlock(&psubmodel->cache);
	}

	data = (unsigned int)paSequences[pseqdesc->seqgroup].data;
	if (data & 1)
		data = 0;
	return (mstudioanim_t*)(data + pseqdesc->animindex);
}

/*
====================
R_StudioSlerpBones

====================
*/
void R_StudioSlerpBones( vec4_t* q1, vec3_t* pos1, vec4_t* q2, vec3_t* pos2, float s )
{
	int			i;
	vec4_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0f) s = 1.0f;

	s1 = 1.0f - s;

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3);
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

float StudioEstimateFrame( mstudioseqdesc_t* pseqdesc )
{
	float		dfdt;
	float		f;

	dfdt = (cl.time - currententity->animtime) *
		ShortToFloat(currententity->framerate) * pseqdesc->fps;

	if (pseqdesc->numframes <= 1)
	{
		f = 0.0f;
	}
	else
	{
		f = currententity->frame * (pseqdesc->numframes - 1) * (1.0f / 256.0f);
	}

	f += dfdt;

	if (pseqdesc->flags & STUDIO_LOOPING)
	{
		if (pseqdesc->numframes > 1)
		{
			f -= (int)(f / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
		}
		if (f < 0.0f)
		{
			f += (pseqdesc->numframes - 1);
		}
	}
	else
	{
		if (f >= pseqdesc->numframes - 1.001f)
		{
			f = pseqdesc->numframes - 1.001f;
		}
		if (f < 0.0f)
		{
			f = 0.0f;
		}
	}
	return f;
}

void R_StudioSetupBones( void )
{
	int			i;
	float		frame;
	mstudiobone_t* bones;
	mstudioseqdesc_t* sequence;
	mstudioanim_t* animation;
	static float position[MAXSTUDIOBONES][3];
	static vec4_t quaternion[MAXSTUDIOBONES];
	float		boneMatrix[3][4];
	static float position2[MAXSTUDIOBONES][3];
	static vec4_t quaternion2[MAXSTUDIOBONES];
	static float position3[MAXSTUDIOBONES][3];
	static vec4_t quaternion3[MAXSTUDIOBONES];
	static float position4[MAXSTUDIOBONES][3];
	static vec4_t quaternion4[MAXSTUDIOBONES];

	if (currententity->sequence >= pstudiohdr->numseq)
		currententity->sequence = 0;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;
	frame = StudioEstimateFrame(sequence);
	animation = R_GetAnim(r_studio_model, sequence);
	R_StudioCalcRotations(position, quaternion, sequence, animation, frame);

	if (sequence->numblends > 1)
	{
		float		s;
		float		dadt;

		animation += pstudiohdr->numbones;
		R_StudioCalcRotations(position2, quaternion2, sequence, animation, frame);
		dadt = CL_StudioEstimateInterpolant();
		s = (currententity->blending[0] * dadt +
			currententity->prevblending[0] * (1.0f - dadt)) * (1.0f / 255.0f);
		R_StudioSlerpBones(quaternion, position, quaternion2, position2, s);

		if (sequence->numblends == 4)
		{
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations(position3, quaternion3, sequence, animation, frame);
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations(position4, quaternion4, sequence, animation, frame);

			s = (currententity->blending[0] * dadt +
				currententity->prevblending[0] * (1.0f - dadt)) * (1.0f / 255.0f);
			R_StudioSlerpBones(quaternion3, position3, quaternion4, position4, s);
			s = (currententity->blending[1] * dadt +
				currententity->prevblending[1] * (1.0f - dadt)) * (1.0f / 255.0f);
			R_StudioSlerpBones(quaternion, position, quaternion3, position3, s);
		}
	}

	if (currententity->sequencetime &&
		currententity->sequencetime + 0.2f > cl.time &&
		(currententity->prevsequence < pstudiohdr->numseq))
	{
		static float previousPosition[MAXSTUDIOBONES][3];
		static vec4_t previousQuaternion[MAXSTUDIOBONES];
		float		s;

		sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
			currententity->prevsequence;
		animation = R_GetAnim(r_studio_model, sequence);
		R_StudioCalcRotations(previousPosition, previousQuaternion, sequence,
			animation, currententity->prevframe);

		if (sequence->numblends > 1)
		{
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations(position2, quaternion2, sequence, animation, frame);
			s = currententity->prevseqblending[0] * (1.0f / 255.0f);
			R_StudioSlerpBones(previousQuaternion, previousPosition,
				quaternion2, position2, s);

			if (sequence->numblends == 4)
			{
				animation += pstudiohdr->numbones;
				R_StudioCalcRotations(position3, quaternion3, sequence,
					animation, frame);
				animation += pstudiohdr->numbones;
				R_StudioCalcRotations(position4, quaternion4, sequence,
					animation, frame);
				s = currententity->prevseqblending[0] * (1.0f / 255.0f);
				R_StudioSlerpBones(quaternion3, position3,
					quaternion4, position4, s);
				s = currententity->prevseqblending[1] * (1.0f / 255.0f);
				R_StudioSlerpBones(previousQuaternion, previousPosition,
					quaternion3, position3, s);
			}
		}

		s = 1.0f - (cl.time - currententity->sequencetime) / 0.2f;
		R_StudioSlerpBones(quaternion, position, previousQuaternion,
			previousPosition, s);
	}
	else
		currententity->prevframe = frame;

	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	if (r_playerinfo && r_playerinfo->gaitsequence != 0)
	{
		sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
			r_playerinfo->gaitsequence;
		animation = R_GetAnim(r_studio_model, sequence);
		R_StudioCalcRotations(position2, quaternion2, sequence, animation,
			r_playerinfo->gaitframe);

		for (i = 0; i < pstudiohdr->numbones; i++)
		{
			if (!strcmp(bones[i].name, "Bip01 Spine"))
				break;
			memcpy(position[i], position2[i], sizeof(position[i]));
			memcpy(quaternion[i], quaternion2[i], sizeof(quaternion[i]));
		}
	}

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		QuaternionMatrix(quaternion[i], boneMatrix);
		boneMatrix[0][3] = position[i][0];
		boneMatrix[1][3] = position[i][1];
		boneMatrix[2][3] = position[i][2];

		if (bones[i].parent == -1)
		{
			R_ConcatTransforms(rotationmatrix, boneMatrix, bonetransform[i]);
			R_ConcatTransforms(rotationmatrix, boneMatrix, lighttransform[i]);
			CL_FxTransform(currententity, bonetransform[i][0]);
		}
		else
		{
			R_ConcatTransforms(bonetransform[bones[i].parent], boneMatrix,
				bonetransform[i]);
			R_ConcatTransforms(lighttransform[bones[i].parent], boneMatrix,
				lighttransform[i]);
		}
	}
}

void MatrixCopy( float(*in)[4], float(*out)[4] )
{
	memcpy(out, in, sizeof(float) * 3 * 4);
}

void R_StudioSaveBones( void )
{
	int			i;
	mstudiobone_t* bones;

	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	cached_numbones = pstudiohdr->numbones;
	if (cached_numbones > MAXSTUDIOBONES)
		Sys_Error("Too damn many bones: %d", cached_numbones);

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		strcpy(&cached_bonename[i * 32], bones[i].name);
		memcpy(cached_bonetransform[i], bonetransform[i], 0x40);
		memcpy(cached_lighttransform[i], lighttransform[i], 0x40);
	}
}

/*
====================
R_StudioMergeBones

Merge bones of a child model with current one
====================
*/
void R_StudioMergeBones( model_t* model )
{
	int			i, j;
	float		f;

	mstudiobone_t* pbones;
	mstudioseqdesc_t* pseqdesc;
	mstudioanim_t* panim;

	static float pos[MAXSTUDIOBONES][3];
	float		bonematrix[3][4];
	static vec4_t q[MAXSTUDIOBONES];

	if (currententity->sequence >= pstudiohdr->numseq)
	{
		currententity->sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + currententity->sequence;

	f = StudioEstimateFrame(pseqdesc);

	panim = R_GetAnim(model, pseqdesc);
	R_StudioCalcRotations(pos, q, pseqdesc, panim, f);

	pbones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		for (j = 0; j < cached_numbones; j++)
		{
			if (_stricmp(pbones[i].name, &cached_bonename[j * 32]) == 0)
			{
				memcpy(bonetransform[i], cached_bonetransform[j], 0x40);
				memcpy(lighttransform[i], cached_lighttransform[j], 0x40);
				break;
			}
		}
		if (j >= cached_numbones)
		{
			QuaternionMatrix(q[i], bonematrix);

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1)
			{
				R_ConcatTransforms(rotationmatrix, bonematrix, bonetransform[i]);
				R_ConcatTransforms(rotationmatrix, bonematrix, lighttransform[i]);

				// Apply client-side effects to the transformation matrix
				CL_FxTransform(currententity, bonetransform[i][0]);
			}
			else
			{
				R_ConcatTransforms(bonetransform[pbones[i].parent], bonematrix, bonetransform[i]);
				R_ConcatTransforms(lighttransform[pbones[i].parent], bonematrix, lighttransform[i]);
			}
		}
	}
}

/*
====================
SV_StudioSetupBones

Server-side setup of studio bones
====================
*/
void SV_StudioSetupBones( model_t* pModel, float frame, int sequence, const vec_t* angles, const vec_t* origin,
	const unsigned char* pcontroller, const unsigned char* pblending, int iBone )
{
	int			i, j;
	float		f;
	float		s;
	float		adj[MAXSTUDIOCONTROLLERS];
	mstudiobone_t* pbones;
	mstudioseqdesc_t* pseqdesc;
	mstudioanim_t* panim;

	static float pos[MAXSTUDIOBONES][3];
	float		bonematrix[3][4];
	static vec4_t q[MAXSTUDIOBONES];

	int			chain[MAXSTUDIOBONES];
	int			chainlength = 0;

	// Bound sequence number
	if (sequence < 0 || sequence >= pstudiohdr->numseq)
	{
		Con_DPrintf("sequence %d out of range for model %s\n", sequence, pstudiohdr->name);
		sequence = 0;
	}

	pbones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + sequence;
	panim = R_GetAnim(pModel, pseqdesc);

	if (iBone < -1 || iBone >= pstudiohdr->numbones)
		iBone = 0;

	if (iBone == -1)
	{
		chainlength = pstudiohdr->numbones;
		for (i = 0; i < chainlength; i++)
			chain[(chainlength - i) - 1] = i;
	}
	else
	{
		// only the parent bones
		for (i = iBone; i != -1; i = pbones[i].parent)
			chain[chainlength++] = i;
	}

	if (pseqdesc->numframes > 1)
	{
		f = (float)(pseqdesc->numframes - 1) * frame / 256.0f;
	}
	else
	{
		f = 0.0f;
	}

	s = f - (int)f;
	R_StudioCalcBoneAdj(0.0f, adj, pcontroller, pcontroller, 0);

	for (i = chainlength - 1; i >= 0; i--)
	{
		j = chain[i];
		R_StudioCalcBoneQuaterion(f, s, &pbones[j], &panim[j], adj, q[j]);
		R_StudioCalcBonePosition(f, s, &pbones[j], &panim[j], adj, pos[j]);
	}

	if (pseqdesc->numblends > 1)
	{
		static vec3_t pos2[MAXSTUDIOBONES];
		static vec4_t q2[MAXSTUDIOBONES];
		float		b;

		panim = R_GetAnim(pModel, pseqdesc) + pstudiohdr->numbones;

		for (i = chainlength - 1; i >= 0; i--)
		{
			j = chain[i];
			R_StudioCalcBoneQuaterion(f, s, &pbones[j], &panim[j], adj, q2[j]);
			R_StudioCalcBonePosition(f, s, &pbones[j], &panim[j], adj, pos2[j]);
		}

		b = *pblending / 255.0f;
		R_StudioSlerpBones(q, pos, q2, pos2, b);
	}

	AngleMatrix(angles, rotationmatrix);
	rotationmatrix[0][3] = origin[0];
	rotationmatrix[1][3] = origin[1];
	rotationmatrix[2][3] = origin[2];

	for (i = chainlength - 1; i >= 0; i--)
	{
		j = chain[i];
		QuaternionMatrix(q[j], bonematrix);
		bonematrix[0][3] = pos[j][0];
		bonematrix[1][3] = pos[j][1];
		bonematrix[2][3] = pos[j][2];

		if (pbones[j].parent == -1)
			R_ConcatTransforms(rotationmatrix, bonematrix, bonetransform[j]);
		else
			R_ConcatTransforms(bonetransform[pbones[j].parent], bonematrix, bonetransform[j]);
	}
}

void AnimationAutomove( const edict_t* pEdict, float flTime )
{
}

void GetBonePosition( const edict_t* pEdict, int iBone, float* rgflOrigin, float* rgflAngles )
{
	pstudiohdr = (studiohdr_t*)Mod_Extradata(sv.models[pEdict->v.modelindex]);

	SV_StudioSetupBones(sv.models[pEdict->v.modelindex], pEdict->v.frame, pEdict->v.sequence, pEdict->v.angles, pEdict->v.origin,
		pEdict->v.controller, pEdict->v.blending, iBone);

	if (rgflOrigin)
	{
		rgflOrigin[0] = bonetransform[iBone][0][3];
		rgflOrigin[1] = bonetransform[iBone][1][3];
		rgflOrigin[2] = bonetransform[iBone][2][3];
	}
}

/*
====================
GetAttachment

Get the attachment origin and angles
====================
*/
void GetAttachment( const edict_t* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles )
{
	mstudioattachment_t	*pattachment;
	vec3_t		angles;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(sv.models[pEdict->v.modelindex]);

	VectorCopy(pEdict->v.angles, angles);
	angles[PITCH] = -pEdict->v.angles[PITCH]; // stupid quake bug

	pattachment = (mstudioattachment_t*)((byte*)pstudiohdr + pstudiohdr->attachmentindex) + iAttachment;

	SV_StudioSetupBones(sv.models[pEdict->v.modelindex], pEdict->v.frame, pEdict->v.sequence, angles, pEdict->v.origin,
		pEdict->v.controller, pEdict->v.blending, pattachment->bone);

	if (rgflOrigin)
	{
		VectorTransform(pattachment->org, bonetransform[pattachment->bone], rgflOrigin);
	}
}

/*
====================
SV_InitStudioHull

Initialize studio clipnodes and hulls
====================
*/
void SV_InitStudioHull( void )
{
	int			i;
	int			side;

	if (studio_hull[0].planes) // already initailized
		return;

	for (i = 0; i < 6; i++)
	{
		side = i & 1;
		studio_clipnodes[i].planenum = i;
		studio_clipnodes[i].children[side] = CONTENTS_EMPTY;

		if (i == 5)
			studio_clipnodes[i].children[side ^ 1] = CONTENTS_SOLID;
		else
			studio_clipnodes[i].children[side ^ 1] = i + 1;
	}

	for (i = 0; i < STUDIO_NUM_HULLS; i++)
	{
		studio_hull[i].planes = &studio_planes[i * 6];
		studio_hull[i].clipnodes = &studio_clipnodes[0];
		studio_hull[i].firstclipnode = 0;
		studio_hull[i].lastclipnode = 5;
	}
}

/*
====================
SV_SetStudioHullPlane

Initialize studio hull plane
====================
*/
void SV_SetStudioHullPlane( mplane_t* pplane, int iBone, int k, float dist )
{
	pplane->type = PLANE_ANYZ;

	pplane->normal[0] = bonetransform[iBone][0][k];
	pplane->normal[1] = bonetransform[iBone][1][k];
	pplane->normal[2] = bonetransform[iBone][2][k];

	pplane->dist = pplane->normal[0] * bonetransform[iBone][0][3] +
		pplane->normal[1] * bonetransform[iBone][1][3] +
		pplane->normal[2] * bonetransform[iBone][2][3] +
		dist;
}

/*
====================
SV_HullForStudioModel

====================
*/
hull_t* SV_HullForStudioModel( const edict_t* pEdict, const vec_t* mins, const vec_t* maxs, vec_t* offset, int* pNumHulls )
{
	qboolean	useComplexHull;
	vec3_t		size;
	float		factor;

	useComplexHull = FALSE;
	factor = 0.5;

	VectorSubtract(maxs, mins, size);
	if (VectorCompare(vec3_origin, size))
	{
		if (!(gGlobalVariables.trace_flags & FTRACE_SIMPLEBOX))
		{
			useComplexHull = TRUE;

			if (pEdict->v.flags & FL_CLIENT)
			{
				if (!sv_clienttrace.value)
				{
					useComplexHull = FALSE;
				}
				else
				{
					factor = sv_clienttrace.value * 0.5f;
					size[0] = 1.0f;
					size[1] = 1.0f;
					size[2] = 1.0f;
				}
			}
		}
	}

	if ((sv.models[pEdict->v.modelindex]->flags & FL_ONGROUND) || useComplexHull)
	{
		VectorScale(size, factor, size);
		VectorCopy(vec3_origin, offset);

		if (pEdict->v.flags & FL_CLIENT)
		{
			mstudioseqdesc_t* sequence;
			vec3_t		angles;
			int			blend;
			byte		blending[2];
			byte		controller[4];

			pstudiohdr = (studiohdr_t*)Mod_Extradata(sv.models[pEdict->v.modelindex]);
			VectorCopy(pEdict->v.angles, angles);
			sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
				pEdict->v.sequence;
			if (Mod_IsStudioNeoModel(pstudiohdr))
				R_StudioPlayerBlend_Neo(sequence, &blend, &angles[PITCH]);
			else
				R_StudioPlayerBlend(sequence, &blend, &angles[PITCH]);

			blending[0] = (byte)blend;
			blending[1] = 0;
			controller[0] = 127;
			controller[1] = 127;
			controller[2] = 127;
			controller[3] = 127;

			return R_StudioHull(sv.models[pEdict->v.modelindex],
				pEdict->v.frame, pEdict->v.sequence,
				angles, pEdict->v.origin, size, controller, blending, pNumHulls);
		}

		return R_StudioHull(sv.models[pEdict->v.modelindex],
			pEdict->v.frame, pEdict->v.sequence,
			pEdict->v.angles, pEdict->v.origin, size, pEdict->v.controller,
			pEdict->v.blending, pNumHulls);
	}
	else
	{
		*pNumHulls = 1;
		return SV_HullForEntity((edict_t*)pEdict, mins, maxs, offset);
	}
}

/*
====================
R_InitStudioCache

====================
*/
void R_InitStudioCache( void )
{
	memset(rgStudioCache, 0, sizeof(rgStudioCache));

	r_cachecurrent = 0;
	nCurrentHull = 0;
	nCurrentPlane = 0;
}

/*
====================
R_CheckStudioCache

Check if a specified studio cache does exist
====================
*/
r_studiocache_t* R_CheckStudioCache( model_t* pModel, float frame, int sequence,
	const vec_t* angles, const vec_t* origin, const vec_t* size, const unsigned char* controller, const unsigned char* blending )
{
	int			i;
	r_studiocache_t* pCached;

	// Check if the cache exists
	for (i = 0; i < STUDIO_CACHE_SIZE; i++)
	{
		pCached = &rgStudioCache[(r_cachecurrent - i) & STUDIO_CACHEMASK];

		// All parameters in cache data must match,
		// so check everything to figure out that there is a cache we are looking for
		if (pCached->pModel != pModel)
			continue;

		if (pCached->frame != frame)
			continue;

		if (pCached->sequence != sequence)
			continue;

		if (!VectorCompare(pCached->angles, angles))
			continue;

		if (!VectorCompare(pCached->origin, origin))
			continue;

		if (!VectorCompare(pCached->size, size))
			continue;

		if (!memcmp(pCached->controller, (void*)controller, sizeof(pCached->controller)) &&
			!memcmp(pCached->blending, (void*)blending, sizeof(pCached->blending)))
		{
			// Found it
			return pCached;
		}
	}

	return NULL;
}

/*
====================
R_AddToStudioCache

Add studio model data to studio cache
====================
*/
void R_AddToStudioCache( float frame, int sequence, const vec_t* angles, const vec_t* origin, const vec_t* size,
	const unsigned char* controller, const unsigned char* pblending, model_t* pModel, hull_t* pHulls, int numhulls )
{
	r_studiocache_t* p;

	if (numhulls + nCurrentHull >= MAXSTUDIOBONES)
	{
		R_FlushStudioCache();
	}

	r_cachecurrent++;
	p = &rgStudioCache[r_cachecurrent & STUDIO_CACHEMASK];
	p->frame = frame;
	p->sequence = sequence;

	VectorCopy(angles, p->angles);
	VectorCopy(origin, p->origin);
	VectorCopy(size, p->size);

	memcpy(p->controller, controller, sizeof(p->controller));
	memcpy(p->blending, pblending, sizeof(p->blending));

	p->pModel = pModel;
	p->nStartHull = nCurrentHull;
	p->nStartPlane = nCurrentPlane;

	memcpy(&cache_hull[nCurrentHull], pHulls, sizeof(hull_t) * numhulls);
	memcpy(&cache_planes[nCurrentPlane], studio_planes, sizeof(mplane_t) * 6 * numhulls);
	memcpy(&cache_hull_hitgroup[nCurrentHull], studio_hull_hitgroup, sizeof(int) * numhulls);

	nCurrentHull += numhulls;
	nCurrentPlane += numhulls * 6;

	p->numhulls = numhulls;
}

/*
====================
R_FlushStudioCache

====================
*/
void R_FlushStudioCache( void )
{
	R_InitStudioCache();
}

/*
====================
R_StudioHull

====================
*/
hull_t* R_StudioHull( model_t* pModel, float frame, int sequence, const vec_t* angles, const vec_t* origin, const vec_t* size,
	const byte*	pcontroller, const byte* pblending, int* pNumHulls )
{
	int			i, j;
	mstudiobbox_t* pbbox;
	vec3_t		angles2;
	r_studiocache_t* pCached;
	int			numHitBoxes;

	SV_InitStudioHull();

	if (r_cachestudio.value)
	{
		pCached = R_CheckStudioCache(pModel, frame, sequence, angles, origin, size, pcontroller, pblending);
		if (pCached)
		{
			memcpy(studio_planes, &cache_planes[pCached->nStartPlane], sizeof(mplane_t) * 6 * pCached->numhulls);
			memcpy(studio_hull, &cache_hull[pCached->nStartHull], sizeof(hull_t) * pCached->numhulls);
			memcpy(studio_hull_hitgroup, &cache_hull_hitgroup[pCached->nStartHull], sizeof(int) * pCached->numhulls);
			*pNumHulls = pCached->numhulls;
			return studio_hull;
		}
	}

	pstudiohdr = (studiohdr_t*)Mod_Extradata(pModel);

	VectorCopy(angles, angles2);
	angles2[PITCH] = -angles[PITCH]; // stupid quake bug
	SV_StudioSetupBones(pModel, frame, sequence, angles2, origin, pcontroller, pblending, -1);

	pbbox = (mstudiobbox_t*)((byte*)pstudiohdr + pstudiohdr->hitboxindex);

	numHitBoxes = pstudiohdr->numhitboxes;
	for (i = 0; i < numHitBoxes; i++)
	{
		studio_hull_hitgroup[i] = pbbox[i].group;

		for (j = 0; j < 3; j++)
		{
			mplane_t*	p0, * p1;

			p0 = &studio_planes[i * 6 + j * 2 + 0];
			p1 = &studio_planes[i * 6 + j * 2 + 1];

			SV_SetStudioHullPlane(p0, pbbox[i].bone, j, pbbox[i].bbmax[j]);
			SV_SetStudioHullPlane(p1, pbbox[i].bone, j, pbbox[i].bbmin[j]);

			p0->dist += fabs(p0->normal[0] * size[0]) + fabs(p0->normal[1] * size[1]) + fabs(p0->normal[2] * size[2]);
			p1->dist -= fabs(p1->normal[0] * size[0]) + fabs(p1->normal[1] * size[1]) + fabs(p1->normal[2] * size[2]);
		}
	}

	*pNumHulls = pstudiohdr->numhitboxes;

	if (r_cachestudio.value)
		R_AddToStudioCache(frame, sequence, angles, origin, size, pcontroller, pblending, pModel, studio_hull, *pNumHulls);

	return &studio_hull[0];
}

int SV_HitgroupForStudioHull( int index )
{
	return studio_hull_hitgroup[index];
}

int			boxpnt[6][4] =
{
	{ 0, 4, 6, 2 }, // +X
	{ 0, 1, 5, 4 }, // +Y
	{ 0, 2, 3, 1 }, // +Z
	{ 7, 5, 1, 3 }, // -X
	{ 7, 3, 2, 6 }, // -Y
	{ 7, 6, 4, 5 }, // -Z
};

vec_t		hullcolor[8][3] =
{
	{ 1.0, 1.0, 1.0 },
	{ 1.0, 0.5, 0.5 },
	{ 0.5, 1.0, 0.5 },
	{ 1.0, 1.0, 0.5 },
	{ 0.5, 0.5, 1.0 },
	{ 1.0, 0.5, 1.0 },
	{ 0.5, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 }
};

void R_StudioDrawHulls( void )
{
	int			i, j;
	float		lv;
	vec3_t		tmp;
	vec3_t		p[8];
	mstudiobbox_t* pbbox;

	pbbox = (mstudiobbox_t*)((byte*)pstudiohdr + pstudiohdr->hitboxindex);

	R_TriangleSpriteTexture(cl_sprite_white, 0);

	for (i = 0; i < pstudiohdr->numhitboxes; i++)
	{
		for (j = 0; j < 8; j++)
		{
			tmp[0] = (j & 1) ? pbbox[i].bbmin[0] : pbbox[i].bbmax[0];
			tmp[1] = (j & 2) ? pbbox[i].bbmin[1] : pbbox[i].bbmax[1];
			tmp[2] = (j & 4) ? pbbox[i].bbmin[2] : pbbox[i].bbmax[2];

			VectorTransform(tmp, lighttransform[pbbox[i].bone], p[j]);
		}

		j = (pbbox[i].group % (MAXSTUDIOGROUPS / 2));

		tri_Begin(TRI_QUADS);
		tri_Color4f(hullcolor[j][0], hullcolor[j][1], hullcolor[j][2], 1.0);
		tri_TexCoord2f(0, 0);

		for (j = 0; j < 6; j++)
		{
			tmp[0] = tmp[1] = tmp[2] = 0;
			tmp[j % 3] = (j < 3) ? 1.0 : -1.0;
			R_StudioLighting(&lv, pbbox[i].bone, 0, tmp);

			tri_Brightness(lv);
			tri_Vertex3fv(p[boxpnt[j][0]]);
			tri_Vertex3fv(p[boxpnt[j][1]]);
			tri_Vertex3fv(p[boxpnt[j][2]]);
			tri_Vertex3fv(p[boxpnt[j][3]]);
		}

		tri_End();
	}
}

void R_StudioAbsBB( void )
{
	int			j;
	float		lv;
	vec3_t		tmp;
	vec3_t		p[8];
	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + currententity->sequence;

	tri_RenderMode(kRenderTransAdd);

	R_TriangleSpriteTexture(cl_sprite_white, 0);

	for (j = 0; j < 8; j++)
	{
		p[j][0] = (j & 1) ? pseqdesc->bbmin[0] : pseqdesc->bbmax[0];
		p[j][1] = (j & 2) ? pseqdesc->bbmin[1] : pseqdesc->bbmax[1];
		p[j][2] = (j & 4) ? pseqdesc->bbmin[2] : pseqdesc->bbmax[2];

		VectorAdd(p[j], currententity->origin, p[j]);
	}

	tri_Begin(TRI_QUADS);
	tri_Color4f(0.5, 0.5, 1, 1);

	for (j = 0; j < 6; j++)
	{
		tmp[0] = tmp[1] = tmp[2] = 0;
		tmp[j % 3] = (j < 3) ? 1.0 : -1.0;
		R_StudioLighting(&lv, -1, 0, tmp);

		tri_Brightness(lv);
		tri_Vertex3fv(p[boxpnt[j][0]]);
		tri_Vertex3fv(p[boxpnt[j][1]]);
		tri_Vertex3fv(p[boxpnt[j][2]]);
		tri_Vertex3fv(p[boxpnt[j][3]]);
	}

	tri_End();
	tri_RenderMode(kRenderNormal);
}

void R_StudioDrawBones( void )
{
	int			i, j, k;
	float		lv;
	vec3_t		tmp;
	vec3_t		p[8];
	vec3_t		up, right, forward;
	vec3_t		a1;
	mstudiobone_t* pbones;

	pbones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);

	R_TriangleSpriteTexture(cl_sprite_white, 0);

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		if (pbones[i].parent == -1)
			continue;

		k = pbones[i].parent;

		a1[0] = a1[1] = a1[2] = 1.0;
		up[0] = lighttransform[i][0][3] - lighttransform[k][0][3];
		up[1] = lighttransform[i][1][3] - lighttransform[k][1][3];
		up[2] = lighttransform[i][2][3] - lighttransform[k][2][3];
		if (up[0] > up[1])
		{
			if (up[0] > up[2])
				a1[0] = 0.0;
			else
				a1[2] = 0.0;
		}
		else
		{
			if (up[1] > up[2])
				a1[1] = 0.0;
			else
				a1[2] = 0.0;
		}

		CrossProduct(up, a1, right);
		VectorNormalize(right);
		CrossProduct(up, right, forward);
		VectorNormalize(forward);
		VectorScale(right, 2.0f, right);
		VectorScale(forward, 2.0f, forward);

		for (j = 0; j < 8; j++)
		{
			p[j][0] = lighttransform[k][0][3];
			p[j][1] = lighttransform[k][1][3];
			p[j][2] = lighttransform[k][2][3];

			if (j & 1)
			{
				VectorSubtract(p[j], right, p[j]);
			}
			else
			{
				VectorAdd(p[j], right, p[j]);
			}

			if (j & 2)
			{
				VectorSubtract(p[j], forward, p[j]);
			}
			else
			{
				VectorAdd(p[j], forward, p[j]);
			}

			if (j & 4)
			{
			}
			else
			{
				VectorAdd(p[j], up, p[j]);
			}
		}

		VectorNormalize(up);
		VectorNormalize(right);
		VectorNormalize(forward);

		tri_Begin(TRI_QUADS);
		tri_Color4f(1, 1, 1, 1);
		tri_TexCoord2f(0, 0);

		for (j = 0; j < 6; j++)
		{
			switch (j)
			{
			case 0:	VectorCopy(right, tmp); break;
			case 1:	VectorCopy(forward, tmp); break;
			case 2:	VectorCopy(up, tmp); break;
			case 3:	VectorScale(right, -1, tmp); break;
			case 4:	VectorScale(forward, -1, tmp); break;
			case 5:	VectorScale(up, -1, tmp); break;
			}

			R_StudioLighting(&lv, -1, 0, tmp);

			tri_Brightness(lv);
			tri_Vertex3fv(p[boxpnt[j][0]]);
			tri_Vertex3fv(p[boxpnt[j][1]]);
			tri_Vertex3fv(p[boxpnt[j][2]]);
			tri_Vertex3fv(p[boxpnt[j][3]]);
		}

		tri_End();
	}
}

/*
====================
R_StudioTransformAuxVert

====================
*/
void R_StudioTransformAuxVert( auxvert_t* av, int bone, vec_t* vert )
{
	av->fv[0] = DotProduct(bonetransform[bone][0], vert)
		+ bonetransform[bone][0][3];
	av->fv[1] = DotProduct(bonetransform[bone][1], vert)
		+ bonetransform[bone][1][3];
	av->fv[2] = DotProduct(bonetransform[bone][2], vert)
		+ bonetransform[bone][2][3];
}

/*
================
StudioTransformVerts

Bulk skinning path used by the Dreamcast renderer.  The bone matrix is kept in
the SH-4 alternate floating-point bank and only reloaded when the bone index
changes; ftrv then transforms each position through xmtrx.
================
*/
void StudioTransformVerts( auxvert_t* out, const char* bones, vec3_t* verts, int count )
{
	int			lastbone = -1;

	while (count--)
	{
		int			bone = *bones;

		if (bone != lastbone)
		{
			float*		matrix = &bonetransform[bone][0][0];

			lastbone = bone;
			__asm(
				"frchg\n"
				"fmov.s @r4+, fr0\n"
				"fmov.s @r4+, fr4\n"
				"fmov.s @r4+, fr8\n"
				"fmov.s @r4+, fr12\n"
				"fmov.s @r4+, fr1\n"
				"fmov.s @r4+, fr5\n"
				"fmov.s @r4+, fr9\n"
				"fmov.s @r4+, fr13\n"
				"fmov.s @r4+, fr2\n"
				"fmov.s @r4+, fr6\n"
				"fmov.s @r4+, fr10\n"
				"fmov.s @r4+, fr14\n"
				"fmov.s @r4+, fr3\n"
				"fmov.s @r4+, fr7\n"
				"fmov.s @r4+, fr11\n"
				"fmov.s @r4, fr15\n"
				"frchg\n",
				matrix);
		}

		__asm(
			"frchg\n"
			"fldi0 fr12\n"
			"fldi0 fr13\n"
			"fldi0 fr14\n"
			"fldi0 fr15\n"
			"frchg\n"
			"add #12, r4\n"
			"fmov.s @r5+, fr0\n"
			"fmov.s @r5+, fr1\n"
			"fmov.s @r5+, fr2\n"
			"fldi0 fr3\n"
			"ftrv xmtrx, fv0\n"
			"fmov.s fr2, @-r4\n"
			"fmov.s fr1, @-r4\n"
			"fmov.s fr0, @-r4\n",
			out->fv, *verts);

		out->fv[0] += bonetransform[bone][0][3];
		out->fv[1] += bonetransform[bone][1][3];
		out->fv[2] += bonetransform[bone][2][3];
		out++;
		bones++;
		verts++;
	}
}

void R_StudioTransformChromeVerts( vec3_t* out, const char* bones, vec3_t* verts, int count )
{
	int			lastbone = -1;

	while (count--)
	{
		int			bone = *bones++;

		if (bone != lastbone)
		{
			float*		matrix = &lighttransform[bone][0][0];

			lastbone = bone;
			__asm(
				"frchg\n"
				"fmov.s @r4+, fr0\n"
				"fmov.s @r4+, fr4\n"
				"fmov.s @r4+, fr8\n"
				"fmov.s @r4+, fr12\n"
				"fmov.s @r4+, fr1\n"
				"fmov.s @r4+, fr5\n"
				"fmov.s @r4+, fr9\n"
				"fmov.s @r4+, fr13\n"
				"fmov.s @r4+, fr2\n"
				"fmov.s @r4+, fr6\n"
				"fmov.s @r4+, fr10\n"
				"fmov.s @r4+, fr14\n"
				"fmov.s @r4+, fr3\n"
				"fmov.s @r4+, fr7\n"
				"fmov.s @r4+, fr11\n"
				"fmov.s @r4, fr15\n"
				"frchg\n",
				matrix);
		}

		__asm(
			"frchg\n"
			"fldi0 fr12\n"
			"fldi0 fr13\n"
			"fldi0 fr14\n"
			"fldi0 fr15\n"
			"frchg\n"
			"fldi0 fr3\n"
			"add #12, r4\n"
			"fmov.s @r5+, fr0\n"
			"fmov.s @r5+, fr1\n"
			"fmov.s @r5+, fr2\n"
			"ftrv xmtrx, fv0\n"
			"fmov.s fr2, @-r4\n"
			"fmov.s fr1, @-r4\n"
			"fmov.s fr0, @-r4\n",
			*out, *verts);

		out++;
		verts++;
	}
}

/*
====================
R_StudioLighting

====================
*/
void R_StudioLighting( float* lv, int bone, int flags, vec_t* normal )
{
	float		illum;
	float		lightcos;

	illum = r_ambientlight;

	if ((flags & STUDIO_NF_FLATSHADE) && drawstyle != 1)
	{
		illum += r_shadelight * 0.8f;
	}
	else
	{
		float		r;
		if (bone == -1)
			lightcos = DotProduct(normal, r_plightvec);
		else
			lightcos = DotProduct(normal, r_blightvec[bone]); // -1 colinear, 1 opposite

		if (lightcos > 1)
			lightcos = 1;

		r = v_lambert.value;

		if (r < 1.0f)
		{
			lightcos = (r - lightcos) / (r + 1.0f);

			if (lightcos > 0.0f)
				illum += r_shadelight * lightcos;
		}
		else
		{
			illum += r_shadelight;
			lightcos = (lightcos + (r - 1.0f)) / r; 		// do modified hemispherical lighting

			if (lightcos > 0.0f)
				illum -= r_shadelight * lightcos;
		}

		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255)
		illum = 255;

	*lv = lightgammatable[(int)illum * 4] / 1023.0f;	// Light from 0 to 1.0
}

/*
================
R_LightStrength

================
*/
void R_LightStrength( int bone, float* vert, float(*light)[4] )
{
	int			i;

	if (lightage[bone] != r_smodels_total)
	{
		for (i = 0; i < numlights; i++)
		{
			vec3_t		lpos;
			lpos[0] = locallight[i]->origin[0] - lighttransform[bone][0][3];
			lpos[1] = locallight[i]->origin[1] - lighttransform[bone][1][3];
			lpos[2] = locallight[i]->origin[2] - lighttransform[bone][2][3];
			VectorIRotate(lpos, lighttransform[bone], lightbonepos[bone][i]);
		}

		lightage[bone] = r_smodels_total;
	}

	for (i = 0; i < numlights; i++)
	{
		VectorSubtract(vert, lightbonepos[bone][i], light[i]);
		light[i][3] = 0.0;
	}
}

/*
====================
R_LightLambert

Lambert studio lighting

Designed to prevent the rear of a studio model losing its shape and looking too flat
The Lambert lighting is completely non-physical, it gives a purely percieved visual
enhancement and is an example of a forgiving lighting model
====================
*/
// GL Lambert lighting
void R_LightLambert( float(*light)[4], float* normal, float* src, float* lambert )
{
	int			i;
	float		adjr, adjg, adjb;
	float		c;
	int			j;

	adjr = 0.0;
	adjg = 0.0;
	adjb = 0.0;

	for (i = 0; i < numlights; i++)
	{
		float		r2, r;

		r = -DotProduct(normal, light[i]);
		if (r > 0.0f)
		{
			if (light[i][3] == 0.0f)
			{
				r2 = DotProduct(light[i], light[i]);
				if (r2 > 0.0f)
					light[i][3] = locallightR2[i] / (r2 * sqrt(r2));
				else
					light[i][3] = 1.0f;
			}

			c = r * light[i][3];
			adjr += locallinearlight[i][0] * c;
			adjg += locallinearlight[i][1] * c;
			adjb += locallinearlight[i][2] * c;
		}
	}

	// No light at all
	if (adjr == 0.0f && adjg == 0.0f && adjb == 0.0f)
	{
		lambert[0] = src[0];
		lambert[1] = src[1];
		lambert[2] = src[2];
		return;
	}

	//
	// Apply light effect
	//
	j = adjr + lineargammatable[(int)(src[0] * 1023.0f)];
	if (j > 1023)
		lambert[0] = 1.0f;
	else
		lambert[0] = screengammatable[j] / 1023.0f;

	j = adjg + lineargammatable[(int)(src[1] * 1023.0f)];
	if (j > 1023)
		lambert[1] = 1.0f;
	else
		lambert[1] = screengammatable[j] / 1023.0f;

	j = adjb + lineargammatable[(int)(src[2] * 1023.0f)];
	if (j > 1023)
		lambert[2] = 1.0f;
	else
		lambert[2] = screengammatable[j] / 1023.0f;
}
/*
================
R_StudioChrome

================
*/
void R_StudioChromeForMesh( int count, int normalIndex, const char* pnormbone, const vec3_t* pstudionorms )
{
	while (count-- != 0)
	{
		int			bone = pnormbone[normalIndex];
		float		n;

		if (chromeage[bone] != r_smodels_total)
		{
			vec3_t		chromeupvec;
			vec3_t		chromerightvec;
			vec3_t		tmp;

			VectorScale(g_ChromeOrigin, -1.0f, tmp);
			tmp[0] += lighttransform[bone][0][3];
			tmp[1] += lighttransform[bone][1][3];
			tmp[2] += lighttransform[bone][2][3];

			VectorNormalize(tmp);
			CrossProduct(tmp, vright, chromeupvec);
			VectorNormalize(chromeupvec);
			CrossProduct(chromeupvec, tmp, chromerightvec);
			VectorNormalize(chromerightvec);

			VectorIRotate(chromeupvec, lighttransform[bone], r_chromeup[bone]);
			VectorIRotate(chromerightvec, lighttransform[bone], r_chromeright[bone]);
			chromeage[bone] = r_smodels_total;
		}

		n = DotProduct(pstudionorms[normalIndex], r_chromeright[bone]);
		chrome[normalIndex][0] = (n + 1.0f) * 32.0f;

		n = DotProduct(pstudionorms[normalIndex], r_chromeup[bone]);
		chrome[normalIndex][1] = (n + 1.0f) * 32.0f;
		normalIndex++;
	}
}


/*
===========
R_StudioSetupLighting
Applies lighting effects to model
set some global variables based on entity position
inputs:
outputs:
	r_ambientlight
	r_shadelight
===========
*/
void R_StudioSetupLighting( alight_t* plighting )
{
	int			i;
	vec3_t		lightColor;
	vec3_t		ambientColor;

	r_ambientlight = plighting->ambientlight;
	r_shadelight = plighting->shadelight;

	VectorCopy(plighting->plightvec, r_plightvec);

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		VectorIRotate(r_plightvec, lighttransform[i], r_blightvec[i]);
	}

	// the colorVec accepts 0-FFFFF range of colors, scale it with 192 and 255 respectively (C0 and FF)
	r_icolormix.r = (int)(plighting->color[0] * 0xC0FF) & 0xFF00;
	r_icolormix.g = (int)(plighting->color[1] * 0xC0FF) & 0xFF00;
	r_icolormix.b = (int)(plighting->color[2] * 0xC0FF) & 0xFF00;

	r_colormix[0] = plighting->color[0];
	r_colormix[1] = plighting->color[1];
	r_colormix[2] = plighting->color[2];

	lightColor[0] = plighting->color[0] * r_shadelight * (1.0f / 255.0f);
	lightColor[1] = plighting->color[1] * r_shadelight * (1.0f / 255.0f);
	lightColor[2] = plighting->color[2] * r_shadelight * (1.0f / 255.0f);
	ambientColor[0] = plighting->color[0] * r_ambientlight * (1.0f / 255.0f);
	ambientColor[1] = plighting->color[1] * r_ambientlight * (1.0f / 255.0f);
	ambientColor[2] = plighting->color[2] * r_ambientlight * (1.0f / 255.0f);
	DCV_SetWorldLight(plighting->plightvec, lightColor, ambientColor);
}

/*
====================
R_StudioSetupModel

Based on the body part, figure out which mesh it should be using
====================
*/
#pragma auto_inline(off)
void R_StudioSetupModel( int bodypart )
{
	int			index;

	if (bodypart > pstudiohdr->numbodyparts)
	{
		bodypart = 0;
	}

	pbodypart = (mstudiobodyparts_t*)((byte*)pstudiohdr + pstudiohdr->bodypartindex) + bodypart;

	index = currententity->body / pbodypart->base;
	index = index % pbodypart->nummodels;

	psubmodel = (mstudiomodel_t*)((byte*)pstudiohdr + pbodypart->modelindex) + index;
}
#pragma auto_inline(on)

void R_StudioRenderModel( void )
{
	VectorCopy(r_origin, g_ChromeOrigin);
	g_ForcedFaceFlags = 0;
	R_StudioRenderFinal();
}

/*
====================
R_StudioDrawModel

====================
*/
int R_StudioDrawModel( int flags, int checkBBox )
{
	alight_t	lighting;
	vec3_t		dir;
	int			result;

	if (currententity->renderfx == kRenderFxDeadPlayer)
	{
		player_state_t deadPlayer;
		int			savedInterpolation;

		if (currententity->renderamt <= 0 || currententity->renderamt > cl.maxclients)
			return 0;
		deadPlayer = cl.frames[cl.parsecount & UPDATE_MASK].playerstate[currententity->renderamt - 1];
		deadPlayer.number = (byte)(currententity->renderamt - 1);
		deadPlayer.weaponmodel = 0;
		deadPlayer.gaitsequence = 0;
		deadPlayer.movetype = MOVETYPE_NONE;
		VectorCopy(currententity->angles, deadPlayer.viewangles);
		VectorCopy(currententity->origin, deadPlayer.origin);
		savedInterpolation = r_dointerp;
		r_dointerp = 0;
		result = R_StudioDrawPlayer(flags, &deadPlayer);
		r_dointerp = savedInterpolation;
		return result;
	}

	pstudiohdr = (studiohdr_t*)Mod_Extradata(currententity->model);
	r_studio_model = currententity->model;

	R_StudioSetUpTransform(0);

	if (flags & STUDIO_RENDER)
	{
		if (!checkBBox)
			r_studio_clip_required = TRUE;
		else if (!R_StudioCheckBBox())
		{
			DCV_SetClipRequired();
			return 0;
		}

		r_amodels_drawn++;
		r_smodels_total++; // render data cache cookie

		if (pstudiohdr->numbodyparts == 0)
		{
			DCV_SetClipRequired();
			return 1;
		}
	}

	if (currententity->movetype == MOVETYPE_FOLLOW)
		R_StudioMergeBones(r_studio_model);
	else
		R_StudioSetupBones();
	R_StudioSaveBones();

	if (flags & STUDIO_EVENTS)
	{
		R_StudioCalcAttachments();
		R_StudioClientEvents();

		// copy attachments into global entity array
		if (currententity->index > 0)
			memcpy(cl_entities[currententity->index].attachment, currententity->attachment, sizeof(vec3_t) * 4);
	}

	if (flags & STUDIO_RENDER)
	{
		lighting.plightvec = dir;
		R_StudioDynamicLight(currententity, &lighting);

		R_StudioEntityLight(&lighting);

		// model and frame independant
		R_StudioSetupLighting(&lighting);

		r_topcolor = currententity->colormap & 0xFF;
		r_bottomcolor = (currententity->colormap >> 8) & 0xFF;
		R_StudioRenderModel();
	}

	DCV_SetClipRequired();
	return 1;
}

static void R_StudioPlayerBlend( mstudioseqdesc_t* sequence, int* blend, float* pitch )
{
	*blend = (int)(*pitch * 3.0f);
	if (*blend < sequence->blendstart[0])
	{
		*pitch -= sequence->blendstart[0] / 3.0f;
		*blend = 0;
	}
	else if (*blend > sequence->blendend[0])
	{
		*pitch -= sequence->blendend[0] / 3.0f;
		*blend = 255;
	}
	else if (sequence->blendend[0] - sequence->blendstart[0] < 0.1f)
	{
		*blend = 127;
	}
	else
	{
		*blend = (int)(255.0f * ((float)*blend - sequence->blendstart[0]) /
			(sequence->blendend[0] - sequence->blendstart[0]));
		*pitch = 0.0f;
	}
}

void R_StudioEstimateGait( player_state_t* player )
{
	float		dt;
	vec3_t		velocity;
	float		yaw;

	dt = cl.time - cl.oldtime;
	if (dt < 0.0f)
		dt = 0.0f;
	else if (dt > 1.0f)
		dt = 1.0f;

	if (dt == 0.0f || r_playerinfo->renderframe == r_framecount)
	{
		r_gaitmovement = 0.0f;
		return;
	}

	if (cl_gaitestimation.value != 0.0f)
	{
		VectorSubtract(currententity->origin, r_playerinfo->prevgaitorigin, velocity);
		VectorCopy(currententity->origin, r_playerinfo->prevgaitorigin);
		r_gaitmovement = VectorLength(velocity);
		if (dt <= 0.0f || r_gaitmovement / dt < 5.0f)
		{
			r_gaitmovement = 0.0f;
			velocity[0] = 0.0f;
			velocity[1] = 0.0f;
		}
	}
	else
	{
		VectorCopy(player->velocity, velocity);
		r_gaitmovement = VectorLength(velocity) * dt;
	}

	if (velocity[0] == 0.0f && velocity[1] == 0.0f)
	{
		yaw = currententity->angles[YAW] - r_playerinfo->gaityaw;
		yaw -= (int)(yaw / 360.0f) * 360.0f;
		if (yaw > 180.0f)
			yaw -= 360.0f;
		if (yaw < -180.0f)
			yaw += 360.0f;
		if (dt < 0.25f)
			yaw *= dt * 4.0f;
		else
			yaw *= dt;
		r_playerinfo->gaityaw += yaw;
		r_playerinfo->gaityaw -= (int)(r_playerinfo->gaityaw / 360.0f) * 360.0f;
		r_gaitmovement = 0.0f;
	}
	else
	{
		r_playerinfo->gaityaw = (float)atan2(velocity[1], velocity[0]) *
			(180.0f / 3.14159265358979323846f);
		if (r_playerinfo->gaityaw > 180.0f)
			r_playerinfo->gaityaw = 180.0f;
		if (r_playerinfo->gaityaw < -180.0f)
			r_playerinfo->gaityaw = -180.0f;
	}
}

void R_StudioProcessGait( player_state_t* player )
{
	mstudioseqdesc_t* sequence;
	float		dt;
	float		yaw;
	int			blend;
	byte		controller;

	if (currententity->sequence >= pstudiohdr->numseq)
		currententity->sequence = 0;
	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + currententity->sequence;

	R_StudioPlayerBlend(sequence, &blend, &currententity->angles[PITCH]);
	currententity->prevangles[PITCH] = currententity->angles[PITCH];
	currententity->blending[0] = (byte)blend;
	currententity->prevblending[0] = currententity->blending[0];
	currententity->prevseqblending[0] = currententity->blending[0];

	dt = cl.time - cl.oldtime;
	if (dt < 0.0f)
		dt = 0.0f;
	else if (dt > 1.0f)
		dt = 1.0f;

	R_StudioEstimateGait(player);
	yaw = currententity->angles[YAW] - r_playerinfo->gaityaw;
	yaw -= (int)(yaw / 360.0f) * 360.0f;
	if (yaw < -180.0f)
		yaw += 360.0f;
	if (yaw > 180.0f)
		yaw -= 360.0f;

	if (yaw > 120.0f)
	{
		r_playerinfo->gaityaw -= 180.0f;
		r_gaitmovement = -r_gaitmovement;
		yaw -= 180.0f;
	}
	else if (yaw < -120.0f)
	{
		r_playerinfo->gaityaw += 180.0f;
		r_gaitmovement = -r_gaitmovement;
		yaw += 180.0f;
	}

	controller = (byte)(((yaw * 0.25f) + 30.0f) * (255.0f / 60.0f));
	currententity->controller[0] = controller;
	currententity->controller[1] = controller;
	currententity->controller[2] = controller;
	currententity->controller[3] = controller;
	memcpy(currententity->prevcontroller, currententity->controller, 4);

	currententity->angles[YAW] = r_playerinfo->gaityaw;
	if (currententity->angles[YAW] < 0.0f)
		currententity->angles[YAW] += 360.0f;
	currententity->prevangles[YAW] = currententity->angles[YAW];

	if (player->gaitsequence >= pstudiohdr->numseq)
		player->gaitsequence = 0;
	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + player->gaitsequence;
	if (sequence->linearmovement[0] > 0.0f)
		r_playerinfo->gaitframe += (r_gaitmovement / sequence->linearmovement[0]) * sequence->numframes;
	else
		r_playerinfo->gaitframe += sequence->fps * dt;
	r_playerinfo->gaitframe -= (int)(r_playerinfo->gaitframe / sequence->numframes) * sequence->numframes;
	if (r_playerinfo->gaitframe < 0.0f)
		r_playerinfo->gaitframe += sequence->numframes;
}

void R_StudioResetPlayerModel( void )
{
	studio_skin_cache_t* cache;

	cache = R_StudioGetPlayerSkinCache(currententity->index);
	cache->topColor = -1;
	cache->bottomColor = -1;

	if (Cache_Check(&cache->pixels) != NULL)
		Cache_Free(&cache->pixels, 0);
}

/*
====================
R_StudioDrawPlayer

====================
*/
int R_StudioDrawPlayer( int flags, player_state_t* pplayer )
{
	alight_t	lighting;
	vec3_t		dir;
	vec3_t		savedAngles;

	r_playerindex = pplayer->number;
	if (r_playerindex < 0 || r_playerindex >= cl.maxclients)
		return 0;

	if ((developer.value != 0.0f || !SV_Active()) &&
		cl.players[r_playerindex].model[0] != 0)
	{
		if (strcmp(g_studioPlayerModels[r_playerindex].name, cl.players[r_playerindex].model) != 0)
		{
			strcpy(g_studioPlayerModels[r_playerindex].name, cl.players[r_playerindex].model);

			strcpy(g_studioPlayerModels[r_playerindex].modelName, "models/player/");
			strcat(g_studioPlayerModels[r_playerindex].modelName, cl.players[r_playerindex].model);
			strcat(g_studioPlayerModels[r_playerindex].modelName, "/");
			strcat(g_studioPlayerModels[r_playerindex].modelName, cl.players[r_playerindex].model);
			strcat(g_studioPlayerModels[r_playerindex].modelName, ".mdl");

			g_studioPlayerModels[r_playerindex].model =
				Mod_ForName(g_studioPlayerModels[r_playerindex].modelName, FALSE);
			if (g_studioPlayerModels[r_playerindex].model == NULL)
				g_studioPlayerModels[r_playerindex].model = currententity->model;

			R_StudioResetPlayerModel();
		}
	}
	else
	{
		g_studioPlayerModels[r_playerindex].name[0] = 0;
		if (g_studioPlayerModels[r_playerindex].model != currententity->model)
		{
			g_studioPlayerModels[r_playerindex].model = currententity->model;
			R_StudioResetPlayerModel();
		}
	}

	r_studio_model = g_studioPlayerModels[r_playerindex].model;
	if (r_studio_model == NULL)
		return 0;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(r_studio_model);
	if (Mod_IsStudioNeoModel(pstudiohdr))
		return R_StudioDrawPlayer_Neo(flags, pplayer);

	if (pplayer->gaitsequence != 0)
	{
		r_playerinfo = &cl.players[r_playerindex];
		VectorCopy(currententity->angles, savedAngles);
		R_StudioProcessGait(pplayer);
		r_playerinfo->gaitsequence = pplayer->gaitsequence;
		r_playerinfo = NULL;
		R_StudioSetUpTransform(0);
		VectorCopy(savedAngles, currententity->angles);
	}
	else
	{
		currententity->controller[0] = 127;
		currententity->controller[1] = 127;
		currententity->controller[2] = 127;
		currententity->controller[3] = 127;
		memcpy(currententity->prevcontroller, currententity->controller, 4);
		r_playerinfo = &cl.players[r_playerindex];
		r_playerinfo->gaitsequence = 0;
		R_StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!R_StudioCheckBBox())
		{
			DCV_SetClipRequired();
			return 0;
		}

		r_amodels_drawn++;
		r_smodels_total++; // render data cache cookie

		if (pstudiohdr->numbodyparts == 0)
		{
			DCV_SetClipRequired();
			return 1;
		}
	}

	r_playerinfo = &cl.players[r_playerindex];
	R_StudioSetupBones();
	R_StudioSaveBones();
	r_playerinfo->renderframe = r_framecount;
	r_playerinfo = NULL;

	if (flags & STUDIO_EVENTS)
	{
		R_StudioCalcAttachments();
		R_StudioClientEvents();

		// copy attachments into global entity array
		if (currententity->index > 0)
			memcpy(cl_entities[currententity->index].attachment, currententity->attachment, sizeof(vec3_t) * 4);
	}

	if (flags & STUDIO_RENDER)
	{
		if (cl_himodels.value != 0.0f &&
			g_studioPlayerModels[r_playerindex].model != currententity->model)
			currententity->body = 255;
		if (!((developer.value == 0.0f) && SV_Active()) &&
			g_studioPlayerModels[r_playerindex].name[0] != 0 &&
			g_studioPlayerModels[r_playerindex].model == currententity->model)
			currententity->body = 1;

		lighting.plightvec = dir;
		R_StudioDynamicLight(currententity, &lighting);

		R_StudioEntityLight(&lighting);

		// model and frame independant
		R_StudioSetupLighting(&lighting);

		r_playerinfo = &cl.players[r_playerindex];
		r_topcolor = r_playerinfo->color;
		r_bottomcolor = r_playerinfo->bottomcolor;
		if (r_topcolor < 0)
			r_topcolor = 0;
		if (r_topcolor > 360)
			r_topcolor = 360;
		if (r_bottomcolor < 0)
			r_bottomcolor = 0;
		if (r_bottomcolor > 360)
			r_bottomcolor = 360;
		R_StudioRenderModel();
		r_playerinfo = NULL;

		if (pplayer->weaponmodel)
		{
			studio_entity_snapshot_t saveEntity;
			model_t*	pweaponmodel;

			saveEntity = *(studio_entity_snapshot_t*)currententity;
			pweaponmodel = cl.model_precache[pplayer->weaponmodel];

			pstudiohdr = (studiohdr_t*)Mod_Extradata(pweaponmodel);
			r_studio_model = pweaponmodel;

			R_StudioMergeBones(pweaponmodel);
			R_StudioSetupLighting(&lighting);

			R_StudioRenderModel();
			R_StudioCalcAttachments();
			*(studio_entity_snapshot_t*)currententity = saveEntity;
		}
	}

	DCV_SetClipRequired();
	return 1;
}

/*
====================
R_StudioDynamicLight

Apply lighting effects to a model
====================
*/
void R_StudioDynamicLight( cl_entity_t* ent, alight_t* plight )
{
	int			lnum;
	vec3_t		dist; // distance between dlight and entity origin
	colorVec	down;
	vec3_t		light;
	float		total;
	float		r, add;
	vec3_t		uporigin, upend;
	float		floor;
	vec3_t		color;

	// fullbright mode, set max brightness and go away
	if (r_fullbright.value == 1.0f)
	{
		plight->shadelight = 0;
		plight->ambientlight = 192;
		plight->plightvec[0] = 0.0f;
		plight->plightvec[1] = 0.0f;
		plight->plightvec[2] = -1.0f;
		plight->color[0] = 1.0f;
		plight->color[1] = 1.0f;
		plight->color[2] = 1.0f;
		return;
	}

	//
	// setup ambient lighting
	light[1] = 0.0f;
	light[0] = 0.0f;
	light[2] = 1.0f;

	if (!(ent->effects & EF_INVLIGHT))
		light[2] = -1.0f;

	VectorCopy(ent->origin, uporigin);
	uporigin[2] -= light[2] * 8.0f;

	down.r = down.g = down.b = down.a = 0;

	// check sky color values
	if ((cl_skycolor_r.value + cl_skycolor_g.value + cl_skycolor_b.value) != 0)
	{
		vec3_t		end;
		msurface_t*	psurf;

		end[0] = ent->origin[0] - cl_skyvec_x.value * 8192.0f;
		end[1] = ent->origin[1] - cl_skyvec_y.value * 8192.0f;
		end[2] = ent->origin[2] - cl_skyvec_z.value * 8192.0f;

		psurf = SurfaceAtPoint(cl.worldmodel, cl.worldmodel->nodes, uporigin, end);
		if ((ent->model->flags & STUDIO_FORCE_SKYLIGHT) || (psurf && (psurf->flags & SURF_DRAWSKY)))
		{
			down.r = cl_skycolor_r.value;
			down.g = cl_skycolor_g.value;
			down.b = cl_skycolor_b.value;
			light[0] = cl_skyvec_x.value;
			light[1] = cl_skyvec_y.value;
			light[2] = cl_skyvec_z.value;
		}
	}

	// see if the model is not illuminated by the sky
	if ((down.r + down.g + down.b) == 0)
	{
		colorVec	gcolor;
		float		grad[4];

		VectorScale(light, 2048.0f, upend);
		VectorAdd(upend, uporigin, upend);

		down = R_LightVec(uporigin, upend);

		uporigin[0] -= 16.0f;
		uporigin[1] -= 16.0f;
		upend[0] -= 16.0f;
		upend[1] -= 16.0f;
		gcolor = R_LightVec(uporigin, upend);
		grad[0] = (float)(gcolor.r + gcolor.g + gcolor.b) * (1.0f / 768.0f);

		uporigin[0] += 32.0f;
		upend[0] += 32.0f;
		gcolor = R_LightVec(uporigin, upend);
		grad[1] = (float)(gcolor.r + gcolor.g + gcolor.b) * (1.0f / 768.0f);

		uporigin[1] += 32.0f;
		upend[1] += 32.0f;
		gcolor = R_LightVec(uporigin, upend);
		grad[2] = (float)(gcolor.r + gcolor.g + gcolor.b) * (1.0f / 768.0f);

		uporigin[0] -= 32.0f;
		upend[0] -= 32.0f;
		gcolor = R_LightVec(uporigin, upend);
		grad[3] = (float)(gcolor.r + gcolor.g + gcolor.b) * (1.0f / 768.0f);

		// calc light direction
		light[0] = grad[0] - grad[1] - grad[2] + grad[3];
		light[1] = grad[0] + grad[1] - grad[2] - grad[3];
		VectorNormalize(light);
	}

	// Set floor light
	currententity->cvFloorColor = PutRGB(&down);

	color[0] = down.r;
	color[1] = down.g;
	color[2] = down.b;

	// intentsity
	floor = max(max(color[0], color[1]), color[2]);
	if (floor == 0.0f)
		floor = 1.0f;

	VectorScale(light, floor, light);

	//
	// add dynamic lights

	for (lnum = 0; lnum < MAX_DLIGHTS; lnum++)
	{
		dlight_t*	dl;

		dl = &cl_dlights[lnum];

		// it's dead already, so just skip
		if (dl->die < cl.time)
			continue;

		VectorSubtract(ent->origin, dl->origin, dist);

		r = VectorLength(dist);
		add = (dl->radius - r); // squared radius
		if (add > 0.0f)
		{
			floor += add;

			if (r > 1.0f)
			{
				VectorScale(dist, add / r, dist);
			}
			else
			{
				VectorScale(dist, add, dist);
			}

			VectorAdd(light, dist, light);

			color[0] += dl->color.r * (add * (1.0f / 256.0f));
			color[1] += dl->color.g * (add * (1.0f / 256.0f));
			color[2] += dl->color.b * (add * (1.0f / 256.0f));
		}
	}

	if (ent->model->flags & STUDIO_DYNAMIC_LIGHT)
		total = 0.6f;
	else
		total = v_direct.value;

	VectorScale(light, total, light);

	plight->shadelight = VectorLength(light);
	plight->ambientlight = (floor - plight->shadelight);

	floor = max(max(color[0], color[1]), color[2]);
	if (floor == 0.0f)
	{
		plight->color[0] = 1.0f;
		plight->color[1] = 1.0f;
		plight->color[2] = 1.0f;
	}
	else
	{
		plight->color[0] = color[0] * (1.0f / floor);
		plight->color[1] = color[1] * (1.0f / floor);
		plight->color[2] = color[2] * (1.0f / floor);
	}

	// clamp the lighting, so it doesn't "overbright" too much
	if (plight->ambientlight > 128)
		plight->ambientlight = 128;

	if ((plight->ambientlight + plight->shadelight) > 255)
		plight->shadelight = 255 - plight->ambientlight;

	VectorNormalize(light);
	VectorCopy(light, plight->plightvec);
}

// Apply entity lighting
void R_StudioEntityLight( alight_t* plight )
{
	int			i, k;
	dlight_t*	el;
	vec3_t		mid, pos;
	float		dist2, f;
	float		radius;
	float		lstrength[MAXLOCALLIGHTS];
	float		minstrength;

	VectorCopy(currententity->origin, pos);

	lstrength[0] = 0.0f;
	lstrength[1] = 0.0f;
	lstrength[2] = 0.0f;

	minstrength = 1000000.0f;

	numlights = 0; // clear previous elights

	for (i = 0; i < MAX_ELIGHTS; i++)
	{
		el = &cl_elights[i];

		if (el->die <= cl.time || el->radius <= 0.0f)
			continue;

		// Beam entities
		if (BEAMENT_ENTITY(el->key) == currententity->index)
		{
			int			attachment = BEAMENT_ATTACHMENT(el->key);

			if (attachment)
			{
				VectorCopy(currententity->attachment[attachment - 1], el->origin);
			}
			else
			{
				VectorCopy(currententity->origin, el->origin);
			}
		}

		VectorSubtract(pos, el->origin, mid);

		f = DotProduct(mid, mid);
		radius = el->radius * el->radius; // squared radius

		if (f > radius)
			dist2 = (radius / f);
		else
			dist2 = 1.0f;

		if (dist2 > 0.004f)
		{
			int			att;
			if (numlights < 3)
			{
				att = numlights;
			}
			else
			{
				att = -1;
				for (k = 0; k < numlights; k++)
				{
					if (lstrength[k] < minstrength && lstrength[k] < dist2)
					{
						att = k;
						minstrength = lstrength[k];
					}
				}
			}

			if (att != -1)
			{
				lstrength[att] = dist2;
				locallight[att] = el;
				locallightR2[att] = radius;

				locallinearlight[att][0] = el->color.r * 4;
				locallinearlight[att][1] = el->color.g * 4;
				locallinearlight[att][2] = el->color.b * 4;

				if (numlights <= att)
					numlights = att + 1;
			}
		}
	}

	for (i = 0; i < MAXLOCALLIGHTS; i++)
	{
		if (i < numlights)
		{
			vec3_t		color;

			color[0] = locallight[i]->color.r * (1.0f / 255.0f);
			color[1] = locallight[i]->color.g * (1.0f / 255.0f);
			color[2] = locallight[i]->color.b * (1.0f / 255.0f);
			DCV_SetDlight(i + 1, locallight[i]->origin, color, locallight[i]->radius);
		}
		else
		{
			DCV_DisableDlight(i + 1);
		}
	}
}

void R_StudioCalcAttachments( void )
{
	int			i;
	mstudioattachment_t* pattachment;

	if (pstudiohdr->numattachments > 4)
		Sys_Error("Too many attachments on %s", currententity->model->name);

	pattachment = (mstudioattachment_t*)((byte*)pstudiohdr + pstudiohdr->attachmentindex);
	for (i = 0; i < pstudiohdr->numattachments; i++)
		VectorTransform(pattachment[i].org, lighttransform[pattachment[i].bone], currententity->attachment[i]);
}

/*
=========================
R_StudioClientEvents

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void R_StudioClientEvents( void )
{
	int			i;
	mstudioevent_t* event;
	mstudioseqdesc_t* sequence;
	float		frameStart;
	float		frameEnd;
	static float currentTime;
	static float lastTime;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;

	if (cl.time == cl.oldtime)
		return;

	if (currentTime != cl.time)
	{
		lastTime = currentTime;
		currentTime = cl.time;
	}

	if (currententity->effects & EF_MUZZLEFLASH)
	{
		dlight_t*	light;

		light = CL_AllocElight(0);
		VectorCopy(currententity->attachment[0], light->origin);
		light->radius = 16.0f;
		light->decay = light->radius / 0.05f;
		light->die = cl.time + 0.05f;
		light->color.r = 255;
		light->color.g = 192;
		light->color.b = 64;
		currententity->effects &= ~EF_MUZZLEFLASH;
	}

	if (!sequence->numevents)
		return;

	event = (mstudioevent_t*)((byte*)pstudiohdr + sequence->eventindex);
	frameEnd = StudioEstimateFrame(sequence);
	frameStart = frameEnd - (currentTime - lastTime) *
		ShortToFloat(currententity->framerate) * sequence->fps;

	if (currententity->sequencetime == currententity->animtime &&
		!(sequence->flags & STUDIO_LOOPING))
		frameStart = -0.01f;

	for (i = 0; i < sequence->numevents; i++, event++)
	{
		if (event->event < 5000)
			continue;
		if (event->frame <= frameStart || event->frame > frameEnd)
			continue;

		switch (event->event)
		{
		case 5001:
			R_MuzzleFlash(currententity->attachment[0], atoi(event->options));
			break;
		case 5011:
			R_MuzzleFlash(currententity->attachment[1], atoi(event->options));
			break;
		case 5021:
			R_MuzzleFlash(currententity->attachment[2], atoi(event->options));
			break;
		case 5031:
			R_MuzzleFlash(currententity->attachment[3], atoi(event->options));
			break;
		case 5002:
			R_SparkEffect(currententity->attachment[0], atoi(event->options), -100, 100);
			break;
		case 5004:
		{
			int			soundIndex;
			sfx_t*		sound;

			sound = NULL;
			for (soundIndex = 0; soundIndex < MAX_SOUNDS; soundIndex++)
			{
				sound = cl.sound_precache[soundIndex];
				if (sound && !strcmp(event->options, (char*)sound + 8))
					break;
			}
			if (soundIndex == MAX_SOUNDS)
				sound = NULL;

			if (sound)
			{
				S_StartDynamicSound(cl.viewentity, CHAN_AUTO, sound,
					currententity->attachment[0], 1.0f, 1.0f, 0, PITCH_NORM);
			}
			break;
		}
		}
	}
}


vec3_t*		pvlightvalues = lightvalues;

/*
================
R_StudioRenderFinal

Finilize studio model rendering
================
*/
#pragma auto_inline(off)
void R_StudioRenderFinal( void )
{
	int			i;
	int			rendermode;
	qboolean	translucent;
	void(* volatile setupModel)(int) = R_StudioSetupModel;

	GL_DisableMultitexture();
	DCV_PushMatrix(D3DTRANSFORMSTATE_WORLD);

	if (gl_smoothmodels.value)
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

	rendermode = currententity->rendermode;
	if (g_ForcedFaceFlags)
		rendermode = kRenderTransAdd;

	if (r_drawentities.value != 2.0f && r_drawentities.value != 3.0f)
	{
		translucent = FALSE;
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);

		if (rendermode != kRenderNormal)
		{
			DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);
			translucent = TRUE;

			if (rendermode == kRenderTransColor)
			{
				DCV_TexState_Blend();
				DCV_SetColor(255, 255, 255, (int)(r_blend * 255.0f));
			}
			else if (rendermode == kRenderTransAdd)
			{
				int			color = (int)(r_blend * 255.0f);

				DCV_SetColor(color, color, color, 255);
				DCV_TexState_Additive();
			}
			else
			{
				DCV_TexState_Blend();
				DCV_SetColor(255, 255, 255, (int)(r_blend * 255.0f));
			}
		}
		else
		{
			DCV_TexState_Opaque();
		}

		for (i = 0; i < pstudiohdr->numbodyparts; i++)
		{
			setupModel(i);
			if (r_studio_clip_required || translucent)
				R_StudioDrawPoints();
			else
				R_StudioRenderMeshChrome();
		}
	}

	if (r_drawentities.value == 4.0f)
	{
		DCV_SetTexStateFromRenderMode(kRenderTransAdd);
		DCV_SetTexStateFromRenderMode(kRenderNormal);
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
	DCV_PopMatrix(D3DTRANSFORMSTATE_WORLD);
}
#pragma auto_inline(on)

#pragma auto_inline(off)

/*
====================
R_StudioBuildTriangleStrips

Convert the original four-short studio command records in place.  If sharing
enough records saves space, the stream becomes a unique-vertex table followed
by indexed fan/strip commands.  Otherwise the first command is tagged so the
Dreamcast restart-index path can consume it directly on the next pass.
====================
*/
void R_StudioBuildTriangleStrips( short* commands )
{
	unsigned short* records;
	short*		hashTable;
	unsigned short* uniqueSlots;
	unsigned short* indices;
	unsigned short* indexOut;
	unsigned short* source;
	unsigned short* destination;
	unsigned short* uniqueSlotOut;
	int			originalShorts;
	int			groupCount;
	int			vertexCount;
	int			uniqueCount;
	int			count;
	int			i;

	originalShorts = 0;
	groupCount = 0;
	vertexCount = 0;
	uniqueCount = 0;
	source = (unsigned short*)commands + 1;
	records = (unsigned short*)Hunk_TempAlloc(0xAE20);
	hashTable = (short*)((byte*)records + 0x4000);
	uniqueSlots = (unsigned short*)((byte*)hashTable + 0x1000);
	indices = (unsigned short*)((byte*)uniqueSlots + 0x1000);

	count = commands[0];
	if (count < 0)
		count = -count;
	while (count != 0)
	{
		originalShorts += count * 4 + 1;
		vertexCount += count;
		groupCount++;
		source += count * 4;
		count = (short)*source++;
		if (count < 0)
			count = -count;
	}

	memset(hashTable, 0xFF, 0x1000);

	source = (unsigned short*)commands + 1;
	indexOut = indices;
	count = commands[0];
	for (;;)
	{
		unsigned short hash;

		if (count == 0)
			break;

		*indexOut++ = (unsigned short)count;
		if (count < 0)
			count = -count;

		uniqueSlotOut = uniqueSlots + uniqueCount;
		for (i = 0; i < count; i++, source += 4)
		{
			unsigned short existing;

			hash = (unsigned short)(source[0] ^ source[1] ^ (source[2] & source[3]));
			for (;;)
			{
				hash &= 0x07FF;
				existing = (unsigned short)hashTable[hash];
				if ((short)existing < 0)
					break;
				if (records[hash * 4 + 0] == source[0] &&
					records[hash * 4 + 1] == source[1] &&
					records[hash * 4 + 2] == source[2] &&
					records[hash * 4 + 3] == source[3])
				{
					*indexOut++ = existing;
					break;
				}
				hash++;
			}

			if ((short)existing < 0)
			{
				records[hash * 4 + 0] = source[0];
				records[hash * 4 + 1] = source[1];
				records[hash * 4 + 2] = source[2];
				records[hash * 4 + 3] = source[3];
				hashTable[hash] = (short)uniqueCount;
				*uniqueSlotOut++ = hash;
				*indexOut++ = (unsigned short)uniqueCount;
				uniqueCount++;
			}
		}

		count = (short)*source++;
	}

	*indexOut = 0x7FFE;
	if (uniqueCount * 4 + groupCount + vertexCount + 3 < originalShorts)
	{
		commands[0] = 0;
		commands[1] = (short)uniqueCount;
		destination = (unsigned short*)commands + 2;

		for (i = 0; i < uniqueCount; i++)
		{
			unsigned short slot = uniqueSlots[i];

			*destination++ = records[slot * 4 + 0];
			*destination++ = records[slot * 4 + 1];
			*destination++ = records[slot * 4 + 2];
			*destination++ = records[slot * 4 + 3];
		}

		indexOut = indices;
		while (*indexOut != 0x7FFE)
			*destination++ = *indexOut++;
		*destination = 0;
	}
	else if (commands[0] < 0)
	{
		commands[0] -= 100;
	}
	else
	{
		commands[0] = -200 - commands[0];
	}
}

void R_StudioRenderMeshChrome( void )
{
	studiohdr_t* ptexturehdr;
	mstudiotexture_t* ptexture;
	mstudiomesh_t* mesh;
	short*		pskinref;
	byte*		pvertbone;
	byte*		pnormbone;
	byte*		pstudioverts;
	byte*		pstudionorms;
	int			i;
	int			normalIndex;

	pvertbone = (byte*)pstudiohdr + psubmodel->vertinfoindex;
	pnormbone = (byte*)pstudiohdr + psubmodel->norminfoindex;
	pstudioverts = (byte*)pstudiohdr + psubmodel->vertindex;
	pstudionorms = (byte*)pstudiohdr + psubmodel->normindex;
	mesh = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex);

	ptexturehdr = g_pStudioGetTextureHeader(r_studio_model);
	ptexture = (mstudiotexture_t*)((byte*)ptexturehdr + ptexturehdr->textureindex);
	pskinref = (short*)((byte*)ptexturehdr + ptexturehdr->skinindex);
	if (currententity->skin != 0 && currententity->skin < ptexturehdr->numskinfamilies)
		pskinref += currententity->skin * ptexturehdr->numskinref;

	DCV_Flush();

	normalIndex = 0;
	for (i = 0; i < psubmodel->nummesh; i++)
	{
		int			textureIndex = pskinref[mesh[i].skinref];
		int			textureFlags = ptexture[textureIndex].flags | g_ForcedFaceFlags;

		if (r_fullbright.value >= 2.0f)
			textureFlags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
		if (textureFlags & STUDIO_NF_CHROME)
			R_StudioChromeForMesh(mesh[i].numnorms, normalIndex, (const char*)pnormbone, (const vec3_t*)pstudionorms);
		normalIndex += mesh[i].numnorms;
	}

	DCV_SetupStudioLighting(bonetransform, pstudiohdr->numbones);

	for (i = 0; i < psubmodel->nummesh; i++)
	{
		short*		commands;
		int			textureIndex = pskinref[mesh[i].skinref];
		int			textureFlags = ptexture[textureIndex].flags | g_ForcedFaceFlags;

		g_flStudioTexScaleS = 1.0f / (float)ptexture[textureIndex].width;
		g_flStudioTexScaleT = 1.0f / (float)ptexture[textureIndex].height;

		if (r_fullbright.value < 2.0f)
		{
			g_pStudioSetupPlayerSkin(ptexturehdr, textureIndex);
		}
		else
		{
			textureFlags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
			R_TriangleSpriteTexture(cl_sprite_white, 0);
		}

		c_alias_polys += mesh[i].numtris;
		commands = (short*)((byte*)pstudiohdr + mesh[i].triindex);

		while (*commands != 0)
		{
			if (*commands == (short)0x7FFD)
				goto submit_mesh;

			if (*commands < -99)
			{
				int			count = *commands;

				while (count != 0)
				{
					commands++;
					if (count < 1)
					{
						if (count > -200)
						{
							if (count < -99)
								count += 100;
							count = -count;
							DCV_AddIndicesFanRestart((short)DCV_GetVertCount(), count);
						}
						else
						{
							count = -200 - count;
							DCV_AddIndicesStripRestart((short)DCV_GetVertCount(), count);
						}
					}
					else
					{
						DCV_AddIndicesStripRestart((short)DCV_GetVertCount(), count);
					}

					if (textureFlags & STUDIO_NF_CHROME)
						DCV_AddStudioMeshChromeTagged(count, commands, pstudioverts, pstudionorms,
							pvertbone);
					else
						DCV_AddStudioMeshTagged(count, commands, pstudioverts, pstudionorms, pvertbone);
					commands += count * 4;
					count = *commands;
				}
				goto submit_mesh;
			}

			R_StudioBuildTriangleStrips(commands);
		}

		{
			int			count = commands[1];

			commands += 2;
			if (textureFlags & STUDIO_NF_CHROME)
				DCV_AddStudioMeshChromeTagged(count, commands, pstudioverts, pstudionorms,
					pvertbone);
			else
				DCV_AddStudioMeshTagged(count, commands, pstudioverts, pstudionorms, pvertbone);
			DCV_AssembleStudioIndexListRestart(commands + count * 4);
		}

	submit_mesh:
		DCV_SubmitBatchGuarded();
	}
}

studiohdr_t* R_StudioGetTextureHeader( model_t* model )
{
	studiohdr_t* header;
	model_t*	textureModel;
	char		textureName[MAX_OSPATH];
	unsigned int data;
	int			length;

	header = pstudiohdr;
	if (header->textureindex != 0)
		return header;

	textureModel = (model_t*)model->texinfo;
	Cache_Lock(&model->cache);
	if (!textureModel || !Cache_Check(&textureModel->cache))
	{
		strcpy(textureName, model->name);
		length = strlen(textureName);
		memcpy(textureName + length - 4, "t.mdl", 6);
		textureModel = Mod_ForName(textureName, TRUE);
		model->texinfo = (mtexinfo_t*)textureModel;

		data = (unsigned int)textureModel->cache.data;
		if (data & 1)
			data = 0;
		strcpy(((studiohdr_t*)data)->name, textureName);
	}
	Cache_Unlock(&model->cache);

	data = (unsigned int)textureModel->cache.data;
	if (data & 1)
		data = 0;
	return (studiohdr_t*)data;
}

void R_StudioSetupPlayerSkin( studiohdr_t* textureHeader, int textureIndex )
{
	mstudiotexture_t* texture;
	studio_skin_cache_t* cache;
	byte*		pixels;
	char		textureName[MAX_OSPATH];
	int			playerIndex;

	if (g_ForcedFaceFlags & STUDIO_NF_CHROME)
		return;

	texture = (mstudiotexture_t*)((byte*)textureHeader + textureHeader->textureindex) + textureIndex;
	playerIndex = currententity->index;
	if (playerIndex > 0 && !Q_stricmp(texture->name, "DM_Base.bmp"))
	{
		cache = R_StudioGetPlayerSkinCache(playerIndex);

		if (cache->model != r_studio_model || cache->topColor != r_topcolor ||
			cache->bottomColor != r_bottomcolor)
		{
			R_StudioLoadPlayerSkin(r_studio_model, textureIndex, cache);

			sprintf(textureName, "%s%d", texture->name, playerIndex);

			pixels = (byte*)cache->pixels.data;
			if ((unsigned int)pixels & 1)
				pixels = NULL;
			memcpy(g_studioTranslatedPalette, pixels + cache->width * cache->height,
				STUDIO_PALETTE_RGB_BYTES);

			cache->model = r_studio_model;
			cache->topColor = r_topcolor;
			cache->bottomColor = r_bottomcolor;
			R_StudioRemapPaletteRange(g_studioTranslatedPalette, r_topcolor,
				STUDIO_TOP_COLOR_START, STUDIO_TOP_COLOR_END);
			R_StudioRemapPaletteRange(g_studioTranslatedPalette, cache->bottomColor,
				STUDIO_BOTTOM_COLOR_START, STUDIO_BOTTOM_COLOR_END);

			GL_UnloadTexture(textureName);
			pixels = (byte*)cache->pixels.data;
			if ((unsigned int)pixels & 1)
				pixels = NULL;

			cache->glTexture = GL_LoadTexture(textureName, GLT_STUDIO,
				cache->width, cache->height, pixels, FALSE, TEX_TYPE_NONE,
				g_studioTranslatedPalette);
		}

		if (cache->glTexture != 0)
		{
			GL_Bind(cache->glTexture, 0);
			return;
		}
	}

	GL_Bind(texture->index, 0);
}
#pragma auto_inline(on)

/*
================
R_StudioDrawPoints

================
*/
#if 0
void R_StudioDrawPoints_Legacy( void )
{
	int			i, j;
	byte*		pvertbone;
	byte*		pnormbone;
	vec3_t*		pstudioverts;
	vec3_t*		pstudionorms;
	mstudiotexture_t* ptexture;
	auxvert_t*	av;
	float*		lv;
	vec3_t		fl;
	float		lv_tmp;
	short*		pskinref;
	int			flags;

	pvertbone = ((byte*)pstudiohdr + psubmodel->vertinfoindex);
	pnormbone = ((byte*)pstudiohdr + psubmodel->norminfoindex);
	ptexture = (mstudiotexture_t*)((byte*)pstudiohdr + pstudiohdr->textureindex);

	pmesh = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex);

	pstudioverts = (vec3_t*)((byte*)pstudiohdr + psubmodel->vertindex);
	pstudionorms = (vec3_t*)((byte*)pstudiohdr + psubmodel->normindex);

	pskinref = (short*)((byte*)pstudiohdr + pstudiohdr->skinindex);
	if (currententity->skin != 0 && currententity->skin < pstudiohdr->numskinfamilies)
		pskinref += (currententity->skin * pstudiohdr->numskinref);

	StudioTransformVerts(pauxverts, pvertbone, pstudioverts, psubmodel->numverts);

	pstudioverts = (vec3_t*)((byte*)pstudiohdr + psubmodel->vertindex);
	pvertbone = ((byte*)pstudiohdr + psubmodel->vertinfoindex);
	for (i = 0; i < psubmodel->numverts; i++)
	{
		R_LightStrength(pvertbone[i], pstudioverts[i], lightpos[i]);
	}

//
// clip and draw all triangles
//
	lv = (float*)pvlightvalues;
	for (j = 0; j < psubmodel->nummesh; j++)
	{
		int			k;
		int			normalIndex = (int)(((vec3_t*)lv) - pvlightvalues);
		flags = ptexture[pskinref[pmesh[j].skinref]].flags;
		if (r_fullbright.value >= 2)
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
		if (flags & STUDIO_NF_CHROME)
			R_StudioChromeForMesh(pmesh[j].numnorms, normalIndex, (const char*)pnormbone, pstudionorms);

		if (currententity->rendermode == kRenderTransAdd)
		{
			for (k = 0; k < pmesh[j].numnorms; k++, lv += 3)
			{
				for (k = 0; k < pmesh[j].numnorms; k++, lv += 3)
				{
					lv[0] = r_blend;
					lv[1] = r_blend;
					lv[2] = r_blend;

				}
			}
		}
		else
		{
			for (k = 0; k < pmesh[j].numnorms; k++, lv += 3, pstudionorms++, pnormbone++)
			{
				R_StudioLighting(&lv_tmp, *pnormbone, flags, (float*)pstudionorms);

				lv[0] = lv_tmp * r_colormix[0];
				lv[1] = lv_tmp * r_colormix[1];
				lv[2] = lv_tmp * r_colormix[2];
			}
		}
	}

	DCV_FlushInline();
	GL_Bind(-1, 1);

	pstudionorms = (vec3_t*)((byte*)pstudiohdr + psubmodel->normindex);
	for (j = 0; j < psubmodel->nummesh; j++)
	{
		float		s, t;
		short*		ptricmds;

		pmesh = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex) + j;
		ptricmds = (short*)((byte*)pstudiohdr + pmesh->triindex);

		c_alias_polys += pmesh->numtris;


		flags = ptexture[pskinref[pmesh->skinref]].flags;
		if (r_fullbright.value >= 2)
		{
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
			s = 1.0f / 256.0f;
			t = 1.0f / 256.0f;
		}
		else
		{
			s = 1.0f / ptexture[pskinref[pmesh->skinref]].width;
			t = 1.0f / ptexture[pskinref[pmesh->skinref]].height;
		}

		/* DCV: bind texture for D3D batch */
		if (r_fullbright.value >= 2 && cl_sprite_white && cl_sprite_white->cache.data)
		{
			mspriteframe_t* pFrame = R_GetSpriteFrame((msprite_t*)cl_sprite_white->cache.data, 0);
			if (pFrame)
				GL_Bind(pFrame->gl_texturenum, 0);
			else
				GL_Bind(ptexture[pskinref[pmesh->skinref]].index, 0);
		}
		else
			GL_Bind(ptexture[pskinref[pmesh->skinref]].index, 0);

		if (flags & STUDIO_NF_CHROME)
		{
			s *= 1.0f / 1024.0f;
			t *= 1.0f / 1024.0f;

			/* DCV: chrome meshes → D3D L-vertex batch */
			{
				short*		pc = ptricmds;

				while ((i = *(pc++)) != 0)
				{
					int			n = (i < 0) ? -i : i;
					int			is_fan = (i < 0);
					int			numtris = (n >= 3) ? (n - 2) : 0;
					int			base, v;
					D3DLVERTEX	lv_out;
					DWORD		diffuse;

					DCV_FlushIfLarge();
					base = DCV_GetVertCount();

					for (v = 0; v < n; v++, pc += 4)
					{
						av = &pauxverts[pc[0]];
						lv_out.x = av->fv[0];
						lv_out.y = av->fv[1];
						lv_out.z = av->fv[2];
						lv_out.dwReserved = 0;
						lv = pvlightvalues[pc[1]];
						R_LightLambert(lightpos[pc[0]], pstudionorms[pc[1]], lv, fl);
						diffuse = (DWORD)((int)(r_blend * 255) << 24)
							| (DWORD)((int)((fl[0] > 1 ? 1 : (fl[0] < 0 ? 0 : fl[0])) * 255) << 16)
							| (DWORD)((int)((fl[1] > 1 ? 1 : (fl[1] < 0 ? 0 : fl[1])) * 255) << 8)
							| (DWORD)((int)((fl[2] > 1 ? 1 : (fl[2] < 0 ? 0 : fl[2])) * 255));
						lv_out.color = diffuse;
						lv_out.specular = 0;
						lv_out.tu = chrome[pc[1]][0] * s;
						lv_out.tv = chrome[pc[1]][1] * t;
						DCV_AddLVertex(&lv_out);
					}

					if (numtris > 0)
					{
						if (is_fan)
							DCV_AddIndicesFan(base, n);
						else
							DCV_AddIndicesStrip(base, n);
					}
				}
			}
		}
		else
		{
			/* DCV: normal meshes → D3D L-vertex batch */
			{
				short*		pc = ptricmds;

				while ((i = *(pc++)) != 0)
				{
					int			n = (i < 0) ? -i : i;
					int			is_fan = (i < 0);
					int			numtris = (n >= 3) ? (n - 2) : 0;
					int			base, v;
					D3DLVERTEX	lv_out;
					DWORD		diffuse;

					DCV_FlushIfLarge();
					base = DCV_GetVertCount();

					for (v = 0; v < n; v++, pc += 4)
					{
						av = &pauxverts[pc[0]];
						lv_out.x = av->fv[0];
						lv_out.y = av->fv[1];
						lv_out.z = av->fv[2];
						lv_out.dwReserved = 0;
						lv = pvlightvalues[pc[1]];
						R_LightLambert(lightpos[pc[0]], pstudionorms[pc[1]], lv, fl);
						diffuse = (DWORD)((int)(r_blend * 255) << 24)
							| (DWORD)((int)((fl[0] > 1 ? 1 : (fl[0] < 0 ? 0 : fl[0])) * 255) << 16)
							| (DWORD)((int)((fl[1] > 1 ? 1 : (fl[1] < 0 ? 0 : fl[1])) * 255) << 8)
							| (DWORD)((int)((fl[2] > 1 ? 1 : (fl[2] < 0 ? 0 : fl[2])) * 255));
						lv_out.color = diffuse;
						lv_out.specular = 0;
						lv_out.tu = pc[2] * s;
						lv_out.tv = pc[3] * t;
						DCV_AddLVertex(&lv_out);
					}

					if (numtris > 0)
					{
						if (is_fan)
							DCV_AddIndicesFan(base, n);
						else
							DCV_AddIndicesStrip(base, n);
					}
				}
			}
		}
	}
}
#endif

void R_StudioDrawPoints( void )
{
	studiohdr_t* textureHeader;
	mstudiotexture_t* textures;
	mstudiomesh_t* meshes;
	short*		skinref;
	byte*		vertBones;
	byte*		normBones;
	vec3_t*		studioVerts;
	vec3_t*		studioNorms;
	int			normalIndex;
	int			i;

	vertBones = (byte*)pstudiohdr + psubmodel->vertinfoindex;
	normBones = (byte*)pstudiohdr + psubmodel->norminfoindex;
	studioVerts = (vec3_t*)((byte*)pstudiohdr + psubmodel->vertindex);
	studioNorms = (vec3_t*)((byte*)pstudiohdr + psubmodel->normindex);
	meshes = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex);

	textureHeader = g_pStudioGetTextureHeader(r_studio_model);
	textures = (mstudiotexture_t*)((byte*)textureHeader + textureHeader->textureindex);
	skinref = (short*)((byte*)textureHeader + textureHeader->skinindex);
	if (currententity->skin != 0 && currententity->skin < textureHeader->numskinfamilies)
		skinref += currententity->skin * textureHeader->numskinref;

	DCV_Flush();

	normalIndex = 0;
	for (i = 0; i < psubmodel->nummesh; i++)
	{
		int			textureIndex = skinref[meshes[i].skinref];
		int			flags = textures[textureIndex].flags | g_ForcedFaceFlags;

		if (r_fullbright.value >= 2.0f)
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
		if (flags & STUDIO_NF_CHROME)
			R_StudioChromeForMesh(meshes[i].numnorms, normalIndex, (const char*)normBones, studioNorms);
		normalIndex += meshes[i].numnorms;
	}

	R_StudioTransformChromeVerts(pvlightvalues, (const char*)normBones, studioNorms, normalIndex);
	StudioTransformVerts(pauxverts, (const char*)vertBones, studioVerts, psubmodel->numverts);

	for (i = 0; i < psubmodel->nummesh; i++)
	{
		mstudiomesh_t* mesh = &meshes[i];
		short*		commands = (short*)((byte*)pstudiohdr + mesh->triindex);
		int			textureIndex = skinref[mesh->skinref];
		int			flags = textures[textureIndex].flags | g_ForcedFaceFlags;

		g_flStudioTexScaleS = 1.0f / (float)textures[textureIndex].width;
		g_flStudioTexScaleT = 1.0f / (float)textures[textureIndex].height;

		if (r_fullbright.value < 2.0f)
		{
			g_pStudioSetupPlayerSkin(textureHeader, textureIndex);
		}
		else
		{
			flags = 0;
			R_TriangleSpriteTexture(cl_sprite_white, 0);
		}

		c_alias_polys += mesh->numtris;
		while (*commands != 0)
		{
			if (*commands == (short)0x7FFD)
				goto submit_points_mesh;

			if (*commands < -99)
			{
				int			count = *commands;

				while (count != 0)
				{
					commands++;
					if (count < 1)
					{
						if (count > -200)
						{
							if (count < -99)
								count += 100;
							count = -count;
							DCV_AddIndicesFan((short)DCV_GetVertCount(), count);
						}
						else
						{
							count = -200 - count;
							DCV_AddPolyIndices((short)DCV_GetVertCount(), count);
						}
					}
					else
					{
						DCV_AddPolyIndices((short)DCV_GetVertCount(), count);
					}

					if (flags & STUDIO_NF_CHROME)
						DCV_AddStudioMeshChrome(count, commands, (const byte*)pauxverts,
							(const byte*)pvlightvalues);
					else
						DCV_AddStudioMesh(count, commands, (const byte*)pauxverts,
							(const byte*)pvlightvalues);
					commands += count * 4;
					count = *commands;
				}
				goto submit_points_mesh;
			}

			R_StudioBuildTriangleStrips(commands);
		}

		{
			int			count = commands[1];

			commands += 2;
			if (flags & STUDIO_NF_CHROME)
				DCV_AddStudioMeshChrome(count, commands, (const byte*)pauxverts,
					(const byte*)pvlightvalues);
			else
				DCV_AddStudioMesh(count, commands, (const byte*)pauxverts,
					(const byte*)pvlightvalues);
			DCV_BuildStudioIndexList(commands + count * 4);
		}

	submit_points_mesh:
		DCV_SubmitBatchCopy();
	}
}

void R_StudioRemapPaletteRange( byte* palette, int color, int first, int last )
{
	int			i;
	float		red, blue, green;
	float		maxColor, minColor;
	float		hue, value, saturation;

	hue = (float)color * (360.0f / 255.0f);

	for (i = first; i <= last; i++)
	{
		red = palette[i * 3];
		green = palette[i * 3 + 1];
		blue = palette[i * 3 + 2];

		maxColor = max(max(red, green), blue) * (1.0f / 255.0f);
		minColor = min(min(red, green), blue) * (1.0f / 255.0f);

		value = maxColor;
		saturation = (maxColor - minColor) / maxColor;
		minColor = value * (1.0f - saturation);

		if (hue <= 120.0f)
		{
			blue = minColor;
			if (hue < 60.0f)
			{
				red = value;
				green = minColor + hue * (value - minColor) / (120.0f - hue);
			}
			else
			{
				green = value;
				red = minColor + (120.0f - hue) * (value - minColor) / hue;
			}
		}
		else if (hue <= 240.0f)
		{
			red = minColor;
			if (hue < 180.0f)
			{
				green = value;
				blue = minColor + (hue - 120.0f) * (value - minColor) / (240.0f - hue);
			}
			else
			{
				blue = value;
				green = minColor + (240.0f - hue) * (value - minColor) / (hue - 120.0f);
			}
		}
		else
		{
			green = minColor;
			if (hue < 300.0f)
			{
				blue = value;
				red = minColor + (hue - 240.0f) * (value - minColor) / (360.0f - hue);
			}
			else
			{
				red = value;
				blue = minColor + (360.0f - hue) * (value - minColor) / (hue - 240.0f);
			}
		}

		palette[i * 3] = (byte)(red * 255.0f);
		palette[i * 3 + 1] = (byte)(green * 255.0f);
		palette[i * 3 + 2] = (byte)(blue * 255.0f);
	}
}

studio_skin_cache_t* R_StudioGetPlayerSkinCache( int playerIndex )
{
	studio_skin_cache_t* cache;
	int			cursor;

	cache = g_studioSkinByPlayer[playerIndex];
	if (cache == NULL || cache->playerIndex != playerIndex)
	{
		cursor = g_studioSkinCacheCursor;
		g_studioSkinCacheCursor = (cursor + 1) % STUDIO_SKIN_CACHE_COUNT;
		cache = &g_studioSkinCache[cursor];
		g_studioSkinByPlayer[playerIndex] = cache;
		cache->playerIndex = playerIndex;
		cache->topColor = -1;
		cache->bottomColor = -1;
		if (Cache_Check(&cache->pixels) != NULL)
			Cache_Free(&cache->pixels, 0);
	}
	return cache;
}

void R_StudioLoadPlayerSkin( model_t* model, int textureIndex, studio_skin_cache_t* cache )
{
	studiohdr_t* header;
	mstudiotexture_t* texture;
	byte*		fileData;
	byte*		pixels;
	unsigned int pixelData;
	int			dataSize;

	if (Cache_Check(&cache->pixels) != NULL)
	{
		if (cache->model == model)
			return;
		Cache_Free(&cache->pixels, 0);
	}

	fileData = COM_LoadFile(model->name, 5, NULL);
	header = (studiohdr_t*)fileData;
	texture = (mstudiotexture_t*)(fileData + header->textureindex) + textureIndex;

	cache->textureIndex = textureIndex;
	cache->width = texture->width;
	cache->height = texture->height;

	dataSize = cache->width * cache->height + STUDIO_PALETTE_RGB_BYTES;
	Cache_Alloc(&cache->pixels, dataSize, cache->textureName);

	pixelData = (unsigned int)cache->pixels.data;
	if (pixelData & 1)
		pixelData = 0;

	pixels = (byte*)pixelData;
	memcpy(pixels, fileData + texture->index, dataSize);

	COM_FreeFile(fileData);
}

extern vec3_t lightspot;

void GLR_StudioDrawShadow( void )
{
#if 0
	int			i, k;
	vec3_t		point;
	float		height;
	auxvert_t*	av;

	height = lightspot[2] + 1.0;

	for (i = 0; i < psubmodel->nummesh; i++)
	{
		short*		ptricmds;

		pmesh = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex) + i;
		c_alias_polys += pmesh->numtris;

		ptricmds = (short*)((byte*)pstudiohdr + pmesh->triindex);

		while (1)
		{
			// get the vertex count and primitive type
			k = *(ptricmds++);
			if (!k)
				break;		// done
			if (k < 0)
			{
				k = -k;
				qglBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				qglBegin(GL_TRIANGLE_STRIP);
			}

			for (; k > 0; k--, ptricmds += 4)
			{
				av = &pauxverts[ptricmds[0]];
				VectorCopy(av->fv, point);
				point[0] -= shadevector[0] * (av->fv[2] - lightspot[2]);
				point[1] -= shadevector[1] * (av->fv[2] - lightspot[2]);
				point[2] = height;
				qglVertex3fv(point);
			}

			qglEnd();
		}
	}
#endif
}

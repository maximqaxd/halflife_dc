// gl_model.h
#if !defined( GL_MODEL_H )
#define GL_MODEL_H
#if defined( _WIN32 )
#pragma once
#endif

#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

#include "studio.h"

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

#define MAX_CLIENTS			1
#define	MAX_EDICTS			900

#define MAX_MODEL_NAME		64
#define MAX_MAP_HULLS		4
#define	MIPLEVELS			4
#define	NUM_AMBIENTS		4		// automatic ambient sounds
#define	MAXLIGHTMAPS		4
#define	PLANE_ANYZ			5

#define ALIAS_Z_CLIP_PLANE	5

// flags in finalvert_t.flags
#define ALIAS_LEFT_CLIP				0x0001
#define ALIAS_TOP_CLIP				0x0002
#define ALIAS_RIGHT_CLIP			0x0004
#define ALIAS_BOTTOM_CLIP			0x0008
#define ALIAS_Z_CLIP				0x0010
#define ALIAS_ONSEAM				0x0020
#define ALIAS_XY_CLIP_MASK			0x000F

#define	ZISCALE	((float)0x8000)

#define CACHE_SIZE	32		// used to align key data structures

#define ALIAS_MODEL_VERSION		0x006

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
// normalizing factor so player model works out to about
//  1 pixel per triangle
#define MAX_LBM_HEIGHT			480
#define MAX_ALIAS_MODEL_VERTS	2000

typedef struct msurface_s msurface_t;
typedef struct decal_s decal_t;

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define	SIDE_CROSS	-2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	float dist;
	byte type;			// for texture axis selection and fast side tests
	byte signbits;		// signx + signy<<1 + signz<<1
	unsigned short pad;
	vec3_t normal;
} mplane_t;

// Compact planes used by BSP nodes and surfaces.  The normal vector is shared
// through the model's normal table.
typedef struct mclipplane_s
{
	float dist;
	byte type;
	byte signbits;
	unsigned short normalindex;
} mclipplane_t;

// One slot of the plane-normal dedup table; see mclipplane_t.normalindex above.
typedef struct
{
	vec3_t	normal;
	float	unused;		// hash-table occupancy/sentinel field, unused after lookup
} planenormal_t;

extern planenormal_t* g_planeNormalTable;

unsigned short Mod_AddNormalToTable( vec_t* normal, unsigned int hash );
void Mod_InitNormalTable( void );

typedef struct texture_s
{
	char		name[16];
	unsigned short width, height;
	short		gl_texturenum;
	short		reserved;
	struct msurface_s* texturechain;
	short		anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s*	anim_next;		// in the animation sequence
	struct texture_s*	alternate_anims;	// bmodels in frame 1 use these
	unsigned int offsets[MIPLEVELS];		// four mip maps stored
	byte		fade_r, fade_g, fade_b, fade_fog;	// water fade color/fog, from the WAD palette's tail
} texture_t;

#define SURF_PLANEBACK			2
#define SURF_DRAWSKY			4
#define SURF_DRAWSPRITE			8
#define SURF_DRAWTURB			0x10
#define SURF_DRAWTILED			0x20
#define SURF_DRAWBACKGROUND		0x40
#define SURF_UNDERWATER			0x80
#define SURF_DONTWARP			0x100

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct medge_s
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];		// [s/t] unit vectors in world space. 
								// [i][3] is the s/t offset relative to the origin.
								// s or t = dot(3Dpoint,vecs[i])+vecs[i][3]
	texture_t*	texture;
	short		flags;			// sky or slime, no lightmap or 256 subdivision
} mtexinfo_t;

#define	VERTEXSIZE	8

typedef struct glpoly_s
{
	struct glpoly_s*	next;
	struct glpoly_s*	chain;
	short		numverts;
	short		flags;					// surface flags, for SURF_DRAWTURB / SURF_DRAWBACKGROUND
	float		verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

// JAY: Compress this as much as possible
struct decal_s
{
	short		texture;		 // Decal texture
	short		flags;			 // Decal flags
	short		entityIndex;	 // Entity this is attached to
	unsigned short color;		 // ARGB4444 tint 
	unsigned short scale;		 // scale, packed by FloatToShort
	unsigned short reserved;
	float		dx;				 // Offsets into surface texture (texture coordinates)
	float		dy;				
	struct decal_s*	pnext;		 // linked list for each surface
	struct msurface_s* psurface; // Surface id for persistence / unlinking
	struct decal_s*	chain_next;	 // texture-sorted chain, rebuilt every frame
};

struct msurface_s
{
	byte		flags;				// see SURF_ #defines
	byte		cached_dlight;		// true if dynamic light in cache
	byte		numedges;			// are backwards edges
	byte		lightmaptexturenum;	


	byte		light_s;			// lightmap s coordinate 
	byte		light_t;			// lightmap t coordinate 
	byte		visframe;			// should be drawn when node is crossed (byte, wraps at 256)
	char		dlightframe;		

	byte		styles[MAXLIGHTMAPS]; // index into d_lightstylevalue[] for animated lights 
									  // no one surface can be effected by more than 4 
									  // animated lights.

	short		extents[2];			// s/t texture size, 1..256 for all non-sky surfaces
	short		texturemins[2];		// smallest s/t position on the surface

	short		cached_light[MAXLIGHTMAPS]; // values currently used in lightmap


	int			firstedge;			// look up in model->surfedges[], negative numbers
	int			dlightbits;			


	mclipplane_t*	plane;			// pointer to shared plane
	glpoly_t*	polys;				// multiple if warped
	msurface_t*	texturechain;		
	mtexinfo_t*	texinfo;			
	decal_t*	pdecals;			
	color24*	samples;			//[numstyles*surfsize]
};

// Nodes and leaves share compact traversal fields so a leaf can be walked as a
// node during BSP traversal.
typedef struct mnode_s
{
// node specific
	unsigned short	firstsurface;
	unsigned short	numsurfaces;
// common with leaf
	signed char	contents;		// 0, to differentiate from leafs.
	signed char	visframe;		// Node needs to be traversed if current.
	short		minmaxs[6];		// For bounding box culling.
	struct mnode_s*	parent;
// node specific
	mclipplane_t*	plane;
	struct mnode_s*	children[2];
} mnode_t;

typedef struct mleaf_s
{
// leaf specific
	byte		ambient_sound_level[NUM_AMBIENTS];
// common with node
	signed char	contents;		// Will be a negative contents number.
	signed char	visframe;		// Node needs to be traversed if current.
	short		minmaxs[6];		// For bounding box culling.
	struct mnode_s*	parent;
// leaf specific
	byte*		compressed_vis;
	struct efrag_s*	efrags;
	msurface_t** firstmarksurface;
	unsigned short	nummarksurfaces;
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct hull_s
{
	dclipnode_t*	clipnodes;
	mclipplane_t*	boxplanes;		// compact BSP planes
	mplane_t*		planes;
	int				firstclipnode;
	int				lastclipnode;
	vec3_t			clip_mins;
	vec3_t			clip_maxs;
} hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int		width;
	int		height;
	float	up, down, left, right;
	int		gl_texturenum;
} mspriteframe_t;

typedef struct mspritegroup_s
{
	int				numframes;
	float*			intervals;
	mspriteframe_t* frames[1];
} mspritegroup_t;

typedef struct mspriteframedesc_s
{
	spriteframetype_t	type;
	mspriteframe_t*		frameptr;
} mspriteframedesc_t;

typedef struct msprite_s
{
	short				type;
	short				texFormat;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	int					paloffset;
	float				beamlength;		// remove?
	void*				cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct
{
	int					firstpose;
	int					numposes;
	float				interval;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
	int					facesfront;
	int					vertindex[3];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct {
	int					ident;
	int					version;
	vec3_t				scale;
	vec3_t				scale_origin;
	float				boundingradius;
	vec3_t				eyeposition;
	int					numskins;
	int					skinwidth;
	int					skinheight;
	int					numverts;
	int					numtris;
	int					numframes;
	synctype_t			synctype;
	int					flags;
	float				size;

	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t
	int					commands;	// gl command list with embedded s/t
	int					gl_texturenum[MAX_SKINS];
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS		1024
#define	MAXALIASFRAMES		256
#define	MAXALIASTRIS		2048
extern	aliashdr_t* pheader;
extern	stvert_t	stverts[MAXALIASVERTS];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	trivertx_t* poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum {
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail

// values for model_t's needload
#define NL_PRESENT		0
#define NL_NEEDS_LOADED	1
#define NL_UNREFERENCED	2
#define NL_CLIENT		3

#if !defined( CACHE_USER ) && !defined( QUAKEDEF_H )
#define CACHE_USER
typedef struct cache_user_s
{
	void	*data;
} cache_user_t;
#endif

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

typedef struct model_s
{
	char		name[48];		// Compact model-name buffer.
	short		needload;		// bmodels and sprites don't cache normally
	short		reserved;

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;

	int			flags;

//
// volume occupied by the model graphics
//	
	vec3_t		mins, maxs;
	float		radius;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	dmodel_t*	submodels;

	int			numplanes;
	mclipplane_t*	planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t*	leafs;

	int			numvertexes;
	mvertex_t*	vertexes;

	int			numedges;
	medge_t*	edges;

	int			numnodes;
	mnode_t*	nodes;

	int			numtexinfo;
	mtexinfo_t* texinfo;

	int			numsurfaces;
	msurface_t* surfaces;

	int			numsurfedges;
	int*		surfedges;

	int			numclipnodes;
	dclipnode_t* clipnodes;

	int			nummarksurfaces;
	msurface_t** marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_t** textures;

	byte*		visdata;

	/* LT2 lighting: mode 0=BSP raw, 2=LT2 raw, 3=LT2 LERP 'a'. In LT2 modes
	   lightdata holds the packed payload and lightsurfs the per-face offsets
	   into it; the table is thrown away once the faces are loaded. */
	int			lightmap_mode;
	int*		lightsurfs;
	int			lightBytes;

	color24*	lightdata;
	char*		entities;

//
// additional model data
//
	cache_user_t cache;		// only access through Mod_Extradata
} model_t;

void Mod_UnloadSpriteTextures( model_t* mod );

//============================================================================

typedef struct
{
	char*		name;
	short		entityIndex;
	byte		depth;
	byte		flags;
	vec3_t		position;
} DECALLIST;

//============================================================================

void SW_Mod_Init( void );

void Mod_ClearAll( void );
model_t* Mod_ForName( char* name, qboolean crash );
model_t* Mod_ForNameDefer( char* name, qboolean crash );
model_t* Mod_FindName( char* name );
void* Mod_Extradata( model_t* mod );	// handles caching
void Mod_TouchModel( char* name );
void Mod_MarkClient( model_t* pModel );
void DC_PrecacheMap( char* mapName );

mleaf_t* Mod_PointInLeaf( vec_t* p, model_t* model );

model_t* Mod_LoadModel( model_t* mod, qboolean crash, qboolean bDefer );

void Mod_Print( void );

#endif // GL_MODEL_H

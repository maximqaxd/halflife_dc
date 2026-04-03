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
	vec3_t normal;
	float dist;
	byte type;			// for texture axis selection and fast side tests
	byte signbits;		// signx + signy<<1 + signz<<1
	byte pad[2];
} mplane_t;

typedef struct texture_s
{
	char		name[16];
	unsigned	width, height;
	int			gl_texturenum;
	struct msurface_s* texturechain;
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s*	anim_next;		// in the animation sequence
	struct texture_s*	alternate_anims;	// bmodels in frame 1 use these
	unsigned int offsets[MIPLEVELS];		// four mip maps stored
	byte*		pPal;
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
	float		mipadjust;		// ?? mipmap limits for very small surfaces
	texture_t*	texture;
	int			flags;			// sky or slime, no lightmap or 256 subdivision
} mtexinfo_t;

#define	VERTEXSIZE	8

typedef struct glpoly_s
{
	struct glpoly_s*	next;
	struct glpoly_s*	chain;
	int			numverts;
	int			flags;					// for SURF_UNDERWATER
	float		verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

// JAY: Compress this as much as possible
struct decal_s
{
	short		texture;		 // Decal texture
	short		flags;			 // Decal flags
	short		reserved;		
	unsigned short color;		 // ARGB4444 tint 
	float		scale;			 // scale
	float		dx;				 // Offsets into surface texture (texture coordinates)
	float		dy;				
	struct decal_s*	pnext;		 // linked list for each surface
	struct msurface_s* psurface; // Surface id for persistence / unlinking
	union {
		struct {
			short		entityIndex;	// Entity this is attached to 
			short		_pad1;			// padding to 32 bytes
		};
		struct decal_s*	chain_next;		
	};
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
	byte		dlightframe;		

	byte		styles[MAXLIGHTMAPS]; // index into d_lightstylevalue[] for animated lights 
									  // no one surface can be effected by more than 4 
									  // animated lights.

	short		extents[2];			// s/t texture size, 1..256 for all non-sky surfaces
	short		texturemins[2];		// smallest s/t position on the surface

	short		cached_light[MAXLIGHTMAPS]; // values currently used in lightmap


	int			firstedge;			// look up in model->surfedges[], negative numbers
	int			dlightbits;			


	mplane_t*	plane;				// pointer to shared plane
	glpoly_t*	polys;				// multiple if warped
	msurface_t*	texturechain;		
	mtexinfo_t*	texinfo;			
	decal_t*	pdecals;			
	color24*	samples;			//[numstyles*surfsize]
};

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling


	struct mnode_s*	parent;

// node specific
	mplane_t*	plane;
	struct mnode_s*	children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
// common with node
	int			contents;		// will be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s*	parent;

// leaf specific
	byte*		compressed_vis;
	struct efrag_s*	efrags;

	msurface_t** firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct hull_s
{
	dclipnode_t*	clipnodes;
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

#if !defined( CACHE_USER ) && !defined( QUAKEDEF_H )
#define CACHE_USER
typedef struct cache_user_s
{
	void	*data;
} cache_user_t;
#endif

typedef struct model_s
{
	char		name[MAX_QPATH];
	qboolean	needload;		// bmodels and sprites don't cache normally

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
	mplane_t*	planes;

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
	color24*	lightdata;
	char*		entities;

	/* LT2: mode 0=BSP raw, 2=LT2 raw, 3=LT2 LERP 'a' */
	int			lightmap_mode;
	int			lightBytes;
	int			lightSurfCount;
	int*		lightsurfs;
	byte*		lightpayload;

//
// additional model data
//
	cache_user_t cache;		// only access through Mod_Extradata
} model_t;

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
model_t* Mod_FindName( char* name );
void* Mod_Extradata( model_t* mod );	// handles caching
void Mod_TouchModel( char* name );
void Mod_MarkClient( model_t* pModel );

mleaf_t* Mod_PointInLeaf( vec_t* p, model_t* model );

model_t* Mod_LoadModel( model_t* mod, qboolean crash );

void Mod_Print( void );

#endif // GL_MODEL_H
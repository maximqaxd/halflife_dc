// disable data conversion warnings

#ifndef GLQUAKE_H
#define GLQUAKE_H
#pragma once

#pragma pack(push)
#include <ddraw.h>
#include <d3d.h>
#pragma pack(pop)

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#pragma warning(disable : 4305)     // truncation from 'const double ' to 'float '

#ifdef __cplusplus
extern "C" {
#endif

// RGB565 colors for the fatal-error screen background (Sys_ErrorColor).
// Memory/allocation failures show red; missing or unreadable files show green.
#define RGB565_RED		0xf800
#define RGB565_GREEN	0x07e0

typedef struct auxvert_s
{
	float fv[3];		// viewspace x, y
} auxvert_t;

typedef struct alight_s
{
	int			ambientlight;	// clip at 128
	int			shadelight;		// clip at 192 - ambientlight
	vec3_t		color;
	float* plightvec;
} alight_t;

void GL_BeginRendering( int* x, int* y, int* width, int* height );
void GL_EndRendering( void );

extern	int texture_extension_number;
extern	int	texture_mode;

extern	float	gldepthmin, gldepthmax;

typedef enum
{
	GLT_SYSTEM,
	GLT_DECAL,
	GLT_HUDSPRITE,
	GLT_STUDIO,
	GLT_WORLD,
	GLT_SPRITE
} GL_TEXTURETYPE;

int GL_LoadTexture( char* identifier, GL_TEXTURETYPE textureType, int width, int height, unsigned char* data, int mipmap, int iType, unsigned char* pPal );
void GL_UnloadTexture( char* identifier );
int GL_LoadPicTexture( qpic_t* pic, char* pszName );
int GL_UnloadTextures( void );
void GL_SelectTexture( unsigned int target );

// dc_draw.c -- Dreamcast texture-cache backend
int DC_LoadTexture( char* identifier, int texture_type, int width, int height, void* data, short mipmap, int tex_type, unsigned char* pPal );
int DC_FreeTextureByName( char* name );

void GL_PaletteClearSky( void );
short GL_PaletteAdd( unsigned char* pPal, qboolean isSky );

void GLBeginHud( void );
void GLFinishHud( void );
void DCV_MatrixInit( void );
void DCV_MatrixMode( D3DTRANSFORMSTATETYPE state );
void DCV_BuildProjectionAndSetTransform( float right, float left, float top, float bottom, float zn, float zf, float scale, D3DTRANSFORMSTATETYPE state );
void DrawWedge( float centerx, float centery, float angle1, float angle2, float radius );

void DCV_TexState_Additive( void );
void DCV_TexState_Blend( void );
void DCV_TexState_VertColor( void );
void DCV_TexState_Modulate( void );
void DCV_TexState_Opaque( void );
void DCV_SetTexStateFromRenderMode( int rendermode );

extern	int glx, gly, glwidth, glheight;

#define BACKFACE_EPSILON	0.01f

#pragma pack(push, 1)
typedef struct gbix_s
{
	unsigned int version;           /* "GBIX" in ASCII */
	unsigned int nextTagOffset;     /* bytes to next tag */
	unsigned __int64 globalIndex;
} gbix_t;

typedef struct pvrt_s
{
	unsigned int version;          /* "PVRT" in ASCII */
	unsigned int textureDataSize; /* size of rest of chunk */
	unsigned char colorFormat;     /* 0x01=RGB565, 0x02=ARGB4444 */
	unsigned char imageFormat;     /* 0x01=twiddled, 0x03=VQ, 0x09=rectangle */
	unsigned short zeroes;         /* always 0 */
	unsigned short width;
	unsigned short height;
} pvrt_t;
#pragma pack(pop)

#define GBIXHEADER  (('X'<<24)+('I'<<16)+('B'<<8)+'G')  /* "GBIX" little-endian */
#define PVRTSIGN    (('T'<<24)+('R'<<16)+('V'<<8)+'P')  /* "PVRT" little-endian */

#define PVR_TWIDDLE           0x01
#define PVR_TWIDDLED_MIPMAP   0x02
#define PVR_VQ                0x03
#define PVR_VQ_MIPMAP         0x04
#define PVR_CLUT8_TWIDDLED    0x05
#define PVR_CLUT4_TWIDDLED    0x06
#define PVR_DIRECT8_TWIDDLED  0x07
#define PVR_DIRECT4_TWIDDLED  0x08
#define PVR_RECT              0x09
#define PVR_RECTANGULAR_STRIDE 0x0B
#define PVR_RECTANGULAR_TWIDDLED 0x0D
#define PVR_SMALL_VQ          0x10
#define PVR_SMALL_VQ_MIPMAP   0x11
#define PVR_TWIDDLED_MIPMAP2  0x12

#define PVR_ARGB1555 0x00
#define PVR_RGB565   0x01
#define PVR_ARGB4444 0x02

/* On-disk header of a .PVR texture: a GBIX chunk followed by a PVRT chunk, with
   the encoded pixel data starting right after (at offset 0x20). */
typedef struct pvrheader_s
{
	unsigned char	gbixMagic[4];	/* +0x00 "GBIX" */
	int				gbixLength;		/* +0x04 bytes of global index that follow */
	unsigned int	globalIndex[2];	/* +0x08 texture global index */
	unsigned char	pvrtMagic[4];	/* +0x10 "PVRT" */
	int				dataSize;		/* +0x14 pixel data size + 8 */
	unsigned char	pixelFormat;	/* +0x18 PVR_ARGB1555 / PVR_RGB565 / PVR_ARGB4444 */
	unsigned char	dataFormat;		/* +0x19 PVR_TWIDDLE / PVR_VQ / PVR_RECT / ... */
	unsigned short	reserved;		/* +0x1a */
	short			width;			/* +0x1c */
	short			height;			/* +0x1e */
} pvrheader_t;

#define TEX_TYPE_NONE				0
#define TEX_TYPE_ALPHA				1
#define TEX_TYPE_LUM				2
#define TEX_TYPE_ALPHA_GRADIENT		3
#define TEX_TYPE_RGBA				4
#define TEX_TYPE_RGB565				5
#define TEX_TYPE_PAL8_6				6	/* 8-bit paletted, palette pool class 6 */
#define TEX_TYPE_PAL8				7	/* 8-bit paletted, default palette pool */
#define TEX_TYPE_GBIX				8	/* .PVR file with GBIX/PVRT header */
#define TEX_TYPE_PAL8_9				9	/* 8-bit paletted, palette pool class 9 */
#define TEX_TYPE_RGB565_RAW			10	/* pre-encoded 16-bit RGB565, uploaded verbatim */
#define TEX_TYPE_FONT				11

#define TEX_IS_ALPHA(type)		((type) == TEX_TYPE_ALPHA || (type) == TEX_TYPE_ALPHA_GRADIENT || (type) == TEX_TYPE_RGBA)

void R_TimeRefresh_f( void );
void R_ReadPointFile_f( void );
void DC_TexDump_f( void );
texture_t* R_TextureAnimation( msurface_t* s );


//====================================================

extern cl_entity_t	r_worldentity;
extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern cl_entity_t* currententity;
extern	signed char	r_visframecount;
extern	int			r_framecount;
extern	mplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;


//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t	r_refdef;
extern mleaf_t* r_viewleaf, * r_oldviewleaf;
extern texture_t* r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean envmap;
extern	int	currenttexture;
extern	int	cnttextures[2];

extern	int	particletexture;
extern	int	particlepufftexture;
extern	int	playertextures;

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	cvar_t	r_cachestudio;
extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_decals;
extern	cvar_t	mp_decals;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_shadows;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;
extern	cvar_t	r_mmx;
extern	cvar_t	r_traceglow;
extern	cvar_t	r_wadtextures;

extern	cvar_t	gl_clear;
extern	cvar_t	gl_cull;
extern	int		gl_texsort;
extern	cvar_t	gl_monolights;
extern	cvar_t	gl_smoothmodels;
extern	cvar_t	gl_affinemodels;
extern	cvar_t	gl_flashblend;
extern	cvar_t	gl_playermip;
extern	cvar_t	gl_nocolors;
extern	cvar_t	gl_keeptjunctions;
extern	cvar_t	gl_reporttjunctions;
extern	cvar_t	gl_wateramp;
extern	cvar_t	gl_dither;
extern	cvar_t	gl_spriteblend;
extern	cvar_t	gl_polyoffset;
extern	cvar_t	gl_lightholes;
extern	cvar_t	gl_zmax;
extern	cvar_t	gl_alphamin;
extern	cvar_t	gl_overdraw;
extern	cvar_t	gl_watersides;
extern	cvar_t	gl_overbright;
extern	cvar_t	gl_envmapsize;
extern	cvar_t	gl_flipmatrix;

extern	cvar_t	fogrange;
extern	cvar_t	fogscale;
extern	cvar_t	progress;
extern	cvar_t	profilescale;
extern	cvar_t	profilemeter;

extern	cvar_t	dc_light_min;
extern	cvar_t	dc_light_max;
extern	cvar_t	dc_light_alpha;
extern	cvar_t	dc_light_beta;

extern	cvar_t	dc_depthhud;
extern	cvar_t	dc_depthminhud;
extern	cvar_t	dc_depthmaxhud;
extern	cvar_t	dc_depthmin;
extern	cvar_t	dc_depthmax;
extern	cvar_t	dc_msw;
extern	cvar_t	dc_msv;
extern	cvar_t	dc_msd;
extern	cvar_t	dc_msh;
extern	cvar_t	dc_msh2;

extern	cvar_t	gl_ztrick;

extern	cvar_t	gl_palette_tex;

extern	int		gl_lightmap_format;

extern	int		mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean mirror;
extern	mplane_t* mirror_plane;

extern	vec3_t		r_plightvec;

extern	float	r_world_matrix[16];
extern	float	r_base_world_matrix[16];
extern	float	gProjectionMatrix[16];
extern	float	gWorldToScreen[16];

extern	const char* gl_vendor;
extern	const char* gl_renderer;
extern	const char* gl_version;
extern	const char* gl_extensions;

void GL_Bind( int stage, int texnum );

// Multitexture
#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F

extern qboolean gl_mtexable;

extern int r_dlightframecount;
extern int r_dlightchanged;
extern int r_dlightactive;

void GL_DisableMultitexture( void );
void GL_EnableMultitexture( void );

//
// gl_rmain.c
//
qboolean R_CullBox( vec_t* mins, vec_t* maxs );
qboolean R_TestPackedBoundsAgainstFrustum( short* mins, short* maxs );
void R_RotateForEntity( cl_entity_t* e );
float CalcFov( float fov_x, float width, float height );

//
// gl_rlight.c
//
void R_MarkLights( dlight_t* light, int bit, mnode_t* node );
void R_AnimateLight( void );
void R_RenderDlights( void );
colorVec R_LightPoint( vec_t* p );
colorVec R_LightVec( vec_t* start, vec_t* end );

//
// gl_refrag.c
//
void R_StoreEfrags( efrag_t** ppefrag );

//
// gl_mesh.c
//
void GL_MakeAliasModelDisplayLists( model_t* m, aliashdr_t* hdr );

//
// dc_rsurf.c
//
void R_DrawBrushModel( cl_entity_t* e );
void R_DrawWorld( void );
void DC_BuildLightmaps( void );
void R_DecalInit( void );
void R_DrawDecals( void );
int DecalListCreate( DECALLIST* pList );

//
// dc_d3d.c -- DirectDraw/Direct3D display surfaces + texture pixel-format state
//
extern LPDIRECTDRAW4		g_pDD4;			// primary IDirectDraw4
extern LPDIRECT3DDEVICE3	g_pD3DDevice;	// active HAL device
extern D3DMATRIX		g_identityMatrix;
extern DDPIXELFORMAT	g_pfRGB565;
extern DDPIXELFORMAT	g_pfARGB1555;
extern DDPIXELFORMAT	g_pfARGB4444;
extern DDPIXELFORMAT	g_pfScreenRGB565;
extern DDPIXELFORMAT	g_pfPalette8;

void*		Sys_GetDirectDraw4( void );
void*		Sys_GetPrimarySurface4( void );
void*		Sys_GetBackBuffer4( void );
void*		Sys_GetD3DDevice3( void );
void*		Sys_GetD3DViewport( void );

qboolean	DCV_DDError( HRESULT hr, TCHAR* op );
void		DCV_SetHudDepth( float layer );
void		DCV_Flip( void );
void		DCV_InvalidateTextureCache( void );
void		DCV_SetupFog( void );

//
// dc_rmain.c / dc_warp.c -- world view transform + water shading
//
extern D3DMATRIX	gViewMatrix;
extern colorVec		gWaterColor;
void			R_SetRenderMode( cl_entity_t* pEntity );

//
// text_draw.cpp / dc_draw.c -- console character sheet (the Font_* text API is
// declared per-caller: it takes an internal dcfont_t*, not qfont_t*)
//
extern qfont_t*	draw_chars;
extern int		char_texture;

// shared palette-pool indices: the gray ramp, 6x6x6 color cube, and padded
// gray ramp built once in DCV_GammaRefresh_f (dc_d3d.c), consumed by DC_LoadTexture
extern short	g_iPalIdxDefault;
extern short	g_iPalIdxClass6;
extern short	g_iPalIdxClass9;
extern byte	g_GammaTable256[256];
int		DC_GetPaletteIndex( unsigned char* pPal );

//
// dc_vidnt.c -- render test cvars
//
extern cvar_t	r_testlight;
extern cvar_t	mipbias;

//
// dc_draw.c / l_studio.c -- texture VRAM management, used by the cache
// last-chance reclaim path when the arena is full
//
int		DC_ReclaimTextureSlot( void );
int		DC_ReleaseTexture( int texnum );
void		DC_TouchTexture( int texnum );
void	Mod_FreeStudioTextures( void* buf );
void	Mod_TouchStudioTextures( void* buf );
void	Mod_TouchStudioNeoTextures( void* buf );

void R_ApplyViewModelProjection( float zn );
void R_ApplyViewProjection( void );
void R_ApplyDecalProjection( void );

//
// dc_debug.c -- on-screen memory profiling meters
//
void	DCV_ClearMeters( int flag );
void	DCV_MeterText( unsigned int color, int x, int y, const char* text );
void	DCV_AddMeterTimed( unsigned int color );
void	DCV_AddMeterValue( unsigned int color, float value );
extern cvar_t	profilemeter;

#ifdef __cplusplus
}
#endif

#endif // GLQUAKE_H

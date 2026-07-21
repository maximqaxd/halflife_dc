// decal.h
#if !defined( DECAL_H )
#define DECAL_H
#ifdef _WIN32
#pragma once
#endif

#ifndef DRAW_H
#include "draw.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DECALS				4096		// MAX decals in world

#if defined ( GLQUAKE )
#define MIP_EXTRASIZE		28
#else
#define MIP_EXTRASIZE		24
#endif

// Decal flags
#define FDECAL_PERMANENT	0x01		// This decal should not be removed in favor of any new decals
#define FDECAL_REFERENCE	0x02		// This is a decal that's been moved from another level
#define FDECAL_CUSTOM		0x04        // This is a custom clan logo and should not be saved/restored
#define FDECAL_HFLIP		0x08		// Flip horizontal (U/S) axis
#define FDECAL_VFLIP		0x10		// Flip vertical (V/T) axis
#define FDECAL_CLIPTEST		0x20		// Decal needs to be clip-tested
#define FDECAL_NOCLIP		0x40		// Decal is not clipped by containing polygon

typedef struct
{
	char name[16];
	unsigned char ucFlags;
} decalname_t;

extern decalname_t			sv_decalnames[MAX_BASE_DECALS];
extern int					sv_decalnamecount;

extern char					decal_names[MAX_BASE_DECALS][16];

extern void					R_DecalShoot( int textureIndex, int entity, int modelIndex, vec_t* position, int flags );
extern void					R_CustomDecalShoot( texture_t* ptexture, int playernum, int entity, int modelIndex, vec_t* position, int flags );
extern void					R_DecalRemoveAll( int textureIndex );

extern void					Draw_MiptexTexture( cachewad_t* wad, byte* data );
extern void					Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad );
extern void					Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize );
extern void					Draw_DecalSetName( int decal, char* name );
extern int					Draw_DecalIndex( int id );
extern int					Draw_CacheIndex( cachewad_t* wad, char* path );
extern int					Draw_DecalSize( int number );
texture_t*					Draw_DecalTexture( int index );
extern int					Draw_DecalIndexFromName( char* name );

#if defined ( GLQUAKE )
extern qboolean				Draw_CacheReload( cachewad_t* wad, int index, lumpinfo_t* pLump, cacheentry_t* pic, char* clean, char* path );
extern qboolean				Draw_CacheLoadFromCustom( char* clean, cachewad_t* wad, void* raw, cacheentry_t* pic );
#else
extern qboolean				Draw_CacheReload( cachewad_t* wad, lumpinfo_t* pLump, cachepic_t* pic, char* clean, char* path );
extern qboolean				Draw_CacheLoadFromCustom( char* clean, cachewad_t* wad, void* raw, cachepic_t* pic );
#endif

extern void*				Draw_CacheGet( cachewad_t* wad, int index );
extern void*				Draw_CustomCacheGet( cachewad_t* wad, void* raw, int index );
extern void					CustomDecal_Init( cachewad_t* wad, void* raw, int nFileSize );
extern void					Draw_CustomCacheWadInit( int cacheMax, cachewad_t* wad, void* raw, int nFileSize );
extern int					Draw_CacheByIndex( cachewad_t* wad, int nIndex );

#ifdef __cplusplus
}
#endif

#endif // DECAL_H
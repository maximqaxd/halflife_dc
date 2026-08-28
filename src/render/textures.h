// textures.h
#if !defined( TEXTURES_H )
#define TEXTURES_H
#if defined( _WIN32 )
#pragma once
#endif

qboolean	TEX_InitFromWad( char* path );
qboolean	TEX_BuildPerMapWadPath( const char* mapPath, char* outPath );
void		TEX_CleanupWadInfo( void );
int			TEX_LoadLump( char* name, byte* dest );
void		TEX_AddAnimatingTextures( void );

// Drop every texture so a level change starts with an empty VRAM pool
int GL_UnloadTextures( void );

#endif // TEXTURES_H
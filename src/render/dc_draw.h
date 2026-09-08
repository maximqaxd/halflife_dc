// Shared Dreamcast D3D helpers (2D/3D render paths)
#ifndef DC_DRAW_H
#define DC_DRAW_H

#pragma pack(push)
#include <d3d.h>
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

extern LPDIRECTDRAWSURFACE4 g_pddsPrimary;

// Upload 16-bit subrect into an existing texture slot (for lightmap updates).
int DCV_UpdateTextureSubRect( int texnum, int x, int y, int w, int h, const unsigned short* src, int src_pitch );
int DC_FreeStaleTextureSlots( void );
int DC_ReclaimTextureSlot( void );
int DC_ForceFreeTextureByName( char* name );
void DC_TexDump_f( void );


#ifdef __cplusplus
}
#endif

#endif // DC_DRAW_H

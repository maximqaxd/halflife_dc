/*
 * dc_accum.h – AccumVerts/AccumIndex batch 
 *
 * 
 */
#ifndef DC_ACCUM_H
#define DC_ACCUM_H

#include "glquake.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUAD_TABLE_MAX_VERTS  50
#define FLUSH_THRESHOLD       224

void          DCV_AccumInit( void );
void          DCV_Flush( void );
void          DCV_FlushIfLarge( void );
void          DCV_SetColor( int r, int g, int b, int a );
void          DCV_SetPackedColor( DWORD diffuse );
DWORD         DCV_GetCurrentDiffuse( void );
void          DCV_SetColorFloat( float r, float g, float b, float a );
qboolean      DCV_EnsureSpace( int add_verts, int add_indices );
int           DCV_GetVertCount( void );
int           DCV_AddVertex( float x, float y, float z, float u, float v );
void          DCV_AddLVertex( const D3DLVERTEX* v );
void          DCV_AddPolyIndices( int base, int numverts );
void          DCV_AddIndicesQuad( int i0, int i1, int i2, int i3 );
void          DCV_AddIndicesStrip( int base, int count );
void          DCV_AddIndicesFan( int base, int count );
void          DCV_SetClipRequired( void );
void          DCV_SetNoClip( void );
void          DCV_SetTexStateFromRenderMode( int rendermode );

void          DCV_SetRenderState( D3DRENDERSTATETYPE state, DWORD value );
void          DCV_FlushApplyRenderState( D3DRENDERSTATETYPE state, DWORD value );
void          DCV_SetTextureStageState( DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value );

#ifdef __cplusplus
}
#endif

#endif /* DC_ACCUM_H */

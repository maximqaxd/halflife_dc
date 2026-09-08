/*
 * dc_debug.h -- fatal text drawing on framebuffer (Sys_Error path).
 */

#ifndef DC_FRAMEBUFFER_H
#define DC_FRAMEBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "winquake.h"

#define MAX_FB_METERS 32
#define FBMETER_DATA_BYTES 64

/* Profiling/status meters drawn over the framebuffer each flip (76 bytes). */
typedef struct fbmeter_s
{
	short type;
	short value;
	short y;
	short reserved;
	DWORD color;
	char  text[FBMETER_DATA_BYTES];
} fbmeter_t;

extern fbmeter_t g_FBMeters[MAX_FB_METERS];

void DCV_FB_BackgroundRect( unsigned short wColor );   /* implemented in dc_d3d.c */
void DCV_FB_Text( const char* text );
void DCV_MeterText( unsigned int color, int x, int y, const char* text );
void DCV_AddMeterTimed( unsigned int color );
void DCV_AddMeterValue( unsigned int color, float value );
void DCV_DrawMeters( void );
void DCV_FB_TextPixel( DDSURFACEDESC2 *desc, int x, int y, unsigned short color );
fbmeter_t *DCV_AllocMeter( void );
int DCV_MeterElapsed( void );
void DCV_UpdateMeters( void );
void DCV_AddMeterMarker( unsigned int color, float marker );
void DCV_AddMeterCount( unsigned int color, short value );

#endif /* DC_FRAMEBUFFER_H */

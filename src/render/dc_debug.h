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

/* Profiling/status meters drawn over the framebuffer each flip. */
typedef struct fbmeter_s
{
	short type;
	short value;
	int   frame;
	DWORD color;
	char  pad[FBMETER_DATA_BYTES];
} fbmeter_t;

extern fbmeter_t g_FBMeters[MAX_FB_METERS];

void DCV_FB_TextOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int x, int y, const char* text );

void DCV_FB_BackgroundRect( unsigned short wColor );   /* implemented in dc_d3d.c */
void DCV_FB_Text( const char* text );
void DCV_MeterText( unsigned int color, int x, int y, const char* text );
void DCV_AddMeterTimed( unsigned int color );
void DCV_AddMeterValue( unsigned int color, float value );
void DCV_DrawMeters( void );
int  DCV_FB_LoadImage( byte* rgb, int cache );

#endif /* DC_FRAMEBUFFER_H */

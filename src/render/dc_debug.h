/*
 * dc_debug.h -- fatal text drawing on framebuffer (Sys_Error path).
 */

#ifndef DC_FRAMEBUFFER_H
#define DC_FRAMEBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "winquake.h"

/* Profiling/status meters drawn over the framebuffer each flip. 0x4c bytes. */
typedef struct fbmeter_s
{
	short type;
	short value;
	int   frame;
	DWORD color;
	char  pad[64];
} fbmeter_t;

extern fbmeter_t g_FBMeters[32];

void DCV_FB_TextOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int x, int y, const char* text );

void DCV_FB_BackgroundRect( unsigned short wColor );   /* implemented in dc_d3d.c */
void DCV_FB_Text( const char* text );
void DCV_MeterText( unsigned int color, int x, int y, const char* text );
void DCV_AddMeterTimed( unsigned int color );
void DCV_AddMeterValue( unsigned int color, float value );
void DCV_DrawMeters( void );
int  DCV_FB_LoadImage( byte* rgb, int cache );

#endif /* DC_FRAMEBUFFER_H */

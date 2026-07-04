/*
 * dc_framebuffer.h -- fatal text drawing on framebuffer (Sys_Error path).
 */

#ifndef DC_FRAMEBUFFER_H
#define DC_FRAMEBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "winquake.h"

void DCV_FB_BackgroundRectOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int y, unsigned short wColor );
void DCV_FB_TextOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int x, int y, const char* text );

void DCV_FB_BackgroundRect( unsigned short wColor );
void DCV_FB_Text( const char* text );

#endif /* DC_FRAMEBUFFER_H */

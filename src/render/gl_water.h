// gl_water.h
#ifndef GL_WATER_H
#define GL_WATER_H
#ifdef _WIN32
#pragma once
#endif

void GL_SubdivideSurface( msurface_t* fa );
void EmitWaterPolys( msurface_t* fa, int direction );
void R_DrawSkyChain( msurface_t* s );
void D_SetFadeColor( int r, int g, int b, int fog );

extern float turbsin[];
#define TURBSCALE (256.0f / (2 * M_PI))

#endif // GL_WATER_H
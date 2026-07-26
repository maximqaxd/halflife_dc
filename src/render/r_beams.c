// r_beams.c -- beams, tracers and the segment drawing they share

#include "quakedef.h"

extern float gWorldToScreen[16];

/*
================
R_WorldToScreen

Project a world point onto the screen, in normalized -1..1 coordinates.
Returns TRUE when the point is behind the viewer and the result is unusable.
================
*/
int R_WorldToScreen( vec_t* point, vec_t* screen )
{
	float w;

	screen[0] = gWorldToScreen[0] * point[0] + gWorldToScreen[4] * point[1] + gWorldToScreen[8] * point[2] + gWorldToScreen[12];
	screen[1] = gWorldToScreen[1] * point[0] + gWorldToScreen[5] * point[1] + gWorldToScreen[9] * point[2] + gWorldToScreen[13];
	w         = gWorldToScreen[3] * point[0] + gWorldToScreen[7] * point[1] + gWorldToScreen[11] * point[2] + gWorldToScreen[15];

	if (0.0f != w)
	{
		w = 1.0f / w;
		screen[0] = screen[0] * w;
		screen[1] = screen[1] * w;
	}

	return w <= 0.0f;
}

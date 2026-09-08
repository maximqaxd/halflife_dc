/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

//
// neoanim.h: packs studio animation for a Neo model
//

#ifndef NEOANIM_H
#define NEOANIM_H

// Rewrites one bone axis of one animation into the packed form a Neo model
// carries, and returns the byte after it.  Rotations are quantized on the way
// in, which is what makes them worth packing.
byte *NeoPackAnimation( byte *pData, mstudioanimvalue_t *panimvalue, int numanim, int rotation );

#endif // NEOANIM_H

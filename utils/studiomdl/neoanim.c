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
// neoanim.c: packs studio animation for a Neo model
//

#include <stdio.h>
#include <stdlib.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "..\..\src\engine\studio.h"
#include "neoanim.h"

//=============================================================================
//
// Packed animation
//
// A Neo model keeps the same run structure as a normal one - so many values
// followed by so many frames that repeat the last of them - but the values
// themselves are written as a bit stream instead of a word each.  Each run
// starts with four bits saying how wide its deltas are, then the first value in
// full, then the rest as differences from the one before.  Rotations drop their
// bottom four bits on the way in, which nothing can see and which makes the
// differences small enough to be worth packing.
//
//=============================================================================

#define NEO_MAX_DELTA_BITS	15		// four bits say how wide a run's deltas are
#define NEO_ROTATION_SHIFT	4		// rotations are kept to this many bits fewer
#define NEO_MAX_ANIMVALUES	2048	// values one bone axis can hold

byte	*neo_bitstream;
int		neo_bitsused;

void NeoStartBits( byte *pData )
{
	neo_bitstream = pData;
	neo_bitsused = 0;
	*neo_bitstream = 0;
}

byte *NeoEndBits( void )
{
	// the stream ends on whatever byte it was in the middle of
	return neo_bitstream + (neo_bitsused ? 1 : 0);
}

void NeoWriteBits( int value, int bitcount )
{
	int		room;
	int		take;

	while (bitcount > 0)
	{
		room = 8 - neo_bitsused;
		take = bitcount < room ? bitcount : room;

		*neo_bitstream |= (byte)((value & ((1 << take) - 1)) << neo_bitsused);
		value >>= take;
		bitcount -= take;
		neo_bitsused += take;

		if (neo_bitsused == 8)
		{
			neo_bitstream++;
			neo_bitsused = 0;
			*neo_bitstream = 0;
		}
	}
}

// how many bits a difference needs, signed
int NeoDeltaBits( int delta )
{
	int		bits;

	for (bits = 0; bits <= NEO_MAX_DELTA_BITS; bits++)
	{
		int limit = 1 << bits;
		if (delta >= -(limit / 2) && delta < limit / 2)
			return bits;
	}

	return NEO_MAX_DELTA_BITS + 1;
}

/*
============
NeoPackAnimation

Rewrites one bone axis of one animation into the packed form and returns the
byte after it.
============
*/
byte *NeoPackAnimation( byte *pData, mstudioanimvalue_t *panimvalue, int numanim, int rotation )
{
	static int	value[NEO_MAX_ANIMVALUES];
	static byte	runvalid[NEO_MAX_ANIMVALUES];
	static byte	runtotal[NEO_MAX_ANIMVALUES];
	static int	runfirst[NEO_MAX_ANIMVALUES];
	int			numruns;
	int			numvalues;
	int			n, i, j;

	// unpack the runs, splitting any whose differences are too wide to pack
	numruns = 0;
	numvalues = 0;
	n = 0;
	while (n < numanim)
	{
		int		valid = panimvalue[n].num.valid;
		int		total = panimvalue[n].num.total;
		int		first = numvalues;
		int		used = 0;

		for (i = 0; i < valid; i++)
		{
			int v = panimvalue[n + 1 + i].value;
			if (rotation)
				v >>= NEO_ROTATION_SHIFT;

			if (used && NeoDeltaBits( v - value[numvalues - 1] ) > NEO_MAX_DELTA_BITS)
			{
				// start a fresh run so this one can carry its own first value
				runvalid[numruns] = (byte)used;
				runtotal[numruns] = (byte)used;
				runfirst[numruns] = first;
				numruns++;
				total -= used;
				first = numvalues;
				used = 0;
			}

			if (numvalues >= NEO_MAX_ANIMVALUES || numruns >= NEO_MAX_ANIMVALUES)
				Error("too much animation on one bone to pack\n");

			value[numvalues++] = v;
			used++;
		}

		runvalid[numruns] = (byte)used;
		runtotal[numruns] = (byte)total;
		runfirst[numruns] = first;
		numruns++;

		n += valid + 1;
	}

	// the run table comes first, so the engine can walk the frames without
	// touching the bit stream
	for (i = 0; i < numruns; i++)
	{
		*pData++ = runvalid[i];
		*pData++ = runtotal[i];
	}

	NeoStartBits( pData );
	for (i = 0; i < numruns; i++)
	{
		int		*values = &value[runfirst[i]];
		int		bits = 0;

		for (j = 1; j < runvalid[i]; j++)
		{
			int need = NeoDeltaBits( values[j] - values[j - 1] );
			if (need > bits)
				bits = need;
		}

		NeoWriteBits( bits, 4 );

		if (rotation)
			NeoWriteBits( values[0] & 0xFFF, 12 );
		else
			NeoWriteBits( values[0] & 0xFFFF, 16 );

		for (j = 1; j < runvalid[i]; j++)
			NeoWriteBits( values[j] - values[j - 1], bits );
	}

	return NeoEndBits();
}

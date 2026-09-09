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
// neomesh.c: mesh geometry for a Neo model
//
// A normal model writes each run of triangles with its own copy of the
// vertices it uses, so a vertex shared between runs is stored again every time.
// A Neo model keeps one table of vertices for the whole mesh and follows it
// with the runs, which now hold indices into that table.  Normals are named
// out of a shared table of directions rather than written out in full.
//

#pragma warning( disable : 4244 )

#include <stdio.h>
#include <string.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "neomesh.h"

// how many vertices one mesh can name, and the table that finds them again
#define NEO_MAX_UNIQUE		2048
#define NEO_HASH_MASK		(NEO_MAX_UNIQUE - 1)
#define NEO_MAX_INDICES		16384

// only the directions the engine can name with a sign extended index
#define NEO_MAX_NORMAL		128

static const vec3_t neo_normals[] =
{
#include "..\..\src\engine\anorms.h"
};

/*
============
NeoNormalIndex
============
*/
int NeoNormalIndex( const vec3_t normal )
{
	vec3_t	unit;
	float	best;
	float	length;
	int		bestindex;
	int		i;

	VectorCopy( normal, unit );
	length = VectorLength( unit );
	if (length > 0.0f)
		VectorScale( unit, 1.0f / length, unit );

	best = -2.0f;
	bestindex = 0;
	for (i = 0; i < NEO_MAX_NORMAL; i++)
	{
		float dot = DotProduct( neo_normals[i], unit );
		if (dot > best)
		{
			best = dot;
			bestindex = i;
		}
	}

	return bestindex;
}

/*
============
NeoMeshCommandBound
============
*/
int NeoMeshCommandBound( int numcommandbytes )
{
	// nothing is ever added, so the packed form fits in the original plus the
	// indices and the two shorts of header
	return numcommandbytes / (int)sizeof( short ) + NEO_MAX_INDICES + 4;
}

/*
============
NeoPackMeshCommands
============
*/
int NeoPackMeshCommands( const short *commands, short *out, int maxshorts )
{
	static short	records[NEO_MAX_UNIQUE][4];
	static short	slot[NEO_MAX_UNIQUE];
	static short	order[NEO_MAX_UNIQUE];
	static short	indices[NEO_MAX_INDICES];
	const short		*source;
	short			*dest;
	int				numunique;
	int				numindices;
	int				count;
	int				i;

	for (i = 0; i < NEO_MAX_UNIQUE; i++)
		slot[i] = -1;

	numunique = 0;
	numindices = 0;
	source = commands;

	count = *source++;
	while (count != 0)
	{
		int run = count < 0 ? -count : count;

		if (numindices + run + 1 > NEO_MAX_INDICES)
			Error( "too many triangles in one mesh to pack\n" );

		// the run keeps its own sign, so a fan stays a fan
		indices[numindices++] = (short)count;

		for (i = 0; i < run; i++, source += 4)
		{
			unsigned int hash;

			hash = (unsigned int)((unsigned short)source[0] ^ (unsigned short)source[1] ^
				((unsigned short)source[2] & (unsigned short)source[3]));
			for (;;)
			{
				hash &= NEO_HASH_MASK;
				if (slot[hash] < 0)
					break;
				if (records[hash][0] == source[0] && records[hash][1] == source[1] &&
					records[hash][2] == source[2] && records[hash][3] == source[3])
					break;
				hash++;
			}

			if (slot[hash] < 0)
			{
				if (numunique >= NEO_MAX_UNIQUE)
					Error( "too many vertices in one mesh to pack\n" );

				records[hash][0] = source[0];
				records[hash][1] = source[1];
				records[hash][2] = source[2];
				records[hash][3] = source[3];
				slot[hash] = (short)numunique;
				order[numunique] = (short)hash;
				numunique++;
			}

			indices[numindices++] = slot[hash];
		}

		count = *source++;
	}

	if (numunique * 4 + numindices + 3 > maxshorts)
		Error( "packed mesh does not fit the room it was given\n" );

	dest = out;
	*dest++ = 0;
	*dest++ = (short)numunique;
	for (i = 0; i < numunique; i++)
	{
		short *record = records[order[i]];

		*dest++ = record[0];
		*dest++ = record[1];
		*dest++ = record[2];
		*dest++ = record[3];
	}
	for (i = 0; i < numindices; i++)
		*dest++ = indices[i];
	*dest++ = 0;

	return (dest - out) * sizeof( short );
}

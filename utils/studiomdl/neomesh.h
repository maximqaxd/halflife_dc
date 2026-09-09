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
// neomesh.h: mesh geometry for a Neo model
//

#ifndef NEOMESH_H
#define NEOMESH_H

// The direction in the shared table that points most nearly the same way.  The
// engine reads the index back with a sign extending load, so only the first
// half of the table can be named.
int NeoNormalIndex( const vec3_t normal );

// Rewrites a mesh's commands the way a Neo model holds them: one table of
// vertices with the runs indexing into it, rather than each run carrying its
// own copies.  Returns the number of bytes written.
int NeoPackMeshCommands( const short *commands, short *out, int maxshorts );

// The most shorts NeoPackMeshCommands can write for a mesh of this many
// commands, so a caller can hand it somewhere big enough.
int NeoMeshCommandBound( int numcommandbytes );

#endif // NEOMESH_H

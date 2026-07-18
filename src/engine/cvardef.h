/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef CVARDEF_H
#define CVARDEF_H

#define	FCVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
#define	FCVAR_USERINFO		2	// changes the client's info string
#define	FCVAR_SERVER		4	// notifies players when changed
#define FCVAR_EXTDLL		8	// defined by external DLL
#define FCVAR_CLIENTDLL     16  // defined by the client dll
#define FCVAR_PROTECTED     32  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY        64  // This cvar cannot be changed by clients connected to a multiplayer server.
#define FCVAR_PRINTABLEONLY 128 // This cvar's string cannot contain unprintable characters (e.g. player name, etc).

typedef struct cvar_s
{
	char	*name;
	char	*string;
	int		flags;
	float	value;
	struct cvar_s *next;
} cvar_t;
#endif

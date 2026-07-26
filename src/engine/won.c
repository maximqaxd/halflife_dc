// won.c -- WON (World Opponent Network) authentication and protocol negotiation

#include "quakedef.h"
#include "won.h"

/*
==================
Protocol_Init

Installs the delta descriptor tables that match the negotiated protocol version.
==================
*/
void Protocol_Init( int version )
{
}

/*
==================
SV_CheckProtocol
==================
*/
qboolean SV_CheckProtocol( netadr_t* from, int version )
{
	return TRUE;
}

/*
==================
SV_ClearAuthRequest
==================
*/
void SV_ClearAuthRequest( int slot )
{
}

/*
==================
SV_FindAuthRequest
==================
*/
void* SV_FindAuthRequest( netadr_t* from, qboolean create )
{
	return NULL;
}

/*
==================
WON_ParseAuthenticationMessage
==================
*/
void WON_ParseAuthenticationMessage( void )
{
}

/*
==================
CL_ParseAuthenticationMessage
==================
*/
void CL_ParseAuthenticationMessage( void )
{
	Sys_Error("NYI");
}

/*
==================
WON_InitAuthentication

Clears the server and client authentication request tables.
==================
*/
void WON_InitAuthentication( void )
{
}

/*
==================
WON_ShutdownAuthentication
==================
*/
void WON_ShutdownAuthentication( void )
{
}

/*
==================
WON_RemoveUser
==================
*/
void WON_RemoveUser( int slot )
{
}

/*
==================
WON_HandleServerAuthMsgs

Retransmits any pending server authentication requests to the master.
==================
*/
void WON_HandleServerAuthMsgs( void )
{
}

/*
==================
WON_HandleClientAuthMsgs
==================
*/
void WON_HandleClientAuthMsgs( void )
{
}

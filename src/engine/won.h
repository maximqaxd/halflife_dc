// won.h -- WON (World Opponent Network) authentication and protocol negotiation
#ifndef WON_H
#define WON_H
#ifdef _WIN32
#pragma once
#endif

// Selects the delta descriptor tables for the active network protocol version.
void		Protocol_Init( int version );

// Server-side authentication request tracking.
qboolean	SV_CheckProtocol( netadr_t* from, int version );
void		SV_ClearAuthRequest( int slot );
void*		SV_FindAuthRequest( netadr_t* from, qboolean create );

// Authentication message parsing.
void		WON_ParseAuthenticationMessage( void );
void		CL_ParseAuthenticationMessage( void );

// Authentication lifetime + per-frame servicing.
void		WON_InitAuthentication( void );
void		WON_ShutdownAuthentication( void );
void		WON_RemoveUser( int slot );
void		WON_HandleServerAuthMsgs( void );
void		WON_HandleClientAuthMsgs( void );

#endif // WON_H

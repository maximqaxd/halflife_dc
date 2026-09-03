// won.h -- WON (World Opponent Network) authentication and protocol negotiation
#ifndef WON_H
#define WON_H
#ifdef _WIN32
#pragma once
#endif

typedef struct
{
	qboolean active;
	netadr_t address;
	double nexttime;
	int retries;
	int message;
	int packetlength;
	byte* packet;
	int reserved;
	int userid;
} authrequest_t;

extern authrequest_t cl_authrequest;

extern int g_PF_VELOCITYXY;
extern int g_PF_SPECTATOR;
extern int g_PF_GAITSEQUENCE;
extern int g_PF_MODEL_LONG;
extern int g_PF_ONTRAIN;
extern int g_PF_MODEL;
extern int g_PF_FRAMERATE;
extern int g_PF_BODY;
extern int g_PF_DEAD;
extern int g_PF_NOGRAV;
extern int g_PF_VELOCITY3;
extern int g_PF_VELOCITY2;
extern int g_PF_VELOCITY1;
extern int g_PF_GIB;
extern int g_PF_EFFECTS;
extern int g_PF_PING;
extern int g_PF_RENDER;
extern int g_PF_MSEC;
extern int g_PF_MOREBITS;
extern int g_PF_WATERJUMP;
extern int g_PF_MOVETYPE;
extern int g_PF_WEAPONMODEL;
extern int g_PF_SEQUENCE;
extern int g_PF_CONTROLLER1;
extern int g_PF_CONTROLLER2;
extern int g_PF_CONTROLLER3;
extern int g_PF_CONTROLLER4;
extern int g_PF_FROZEN;
extern int g_PF_COMMAND;
extern int g_PF_YETMOREBITS;
extern int g_PF_SKINNUM;
extern int g_PF_FRICTION;
extern int g_PF_BLENDING1;
extern int g_PF_BLENDING2;
extern int g_PF_BASEVELOCITY;
extern int g_PF_DUCKING;
extern int g_PF_EVENMOREBITS;
extern int g_PF_WEAPONMODEL_LONG;

// Selects the delta descriptor tables for the active network protocol version.
void		Protocol_Init( int version );

// Server-side authentication request tracking.
qboolean	WON_IsValidAuthMessage( int type );
void		SV_ClearAuthRequest( netadr_t* from );
int			SV_FindAuthRequest( qboolean create, netadr_t* from );
qboolean	SV_AuthenticateClient( netadr_t* from, char* certificate, int* userid );

// Authentication message parsing.
void		WON_ParseAuthenticationMessage( int type );
void		CL_ParseAuthenticationMessage( int type );
void		WON_RequestCertificate( void );

// Authentication lifetime + per-frame servicing.
void		WON_InitAuthentication( void );
void		WON_ShutdownAuthentication( void );
void		WON_RemoveUser( authrequest_t* request );
void		WON_HandleServerAuthMsgs( void );
void		WON_HandleClientAuthMsgs( void );

#endif // WON_H

// won.c -- WON (World Opponent Network) authentication and protocol negotiation

#include "quakedef.h"
#include "won.h"

#define MAX_AUTHREQUESTS 128

authrequest_t sv_authrequests[MAX_AUTHREQUESTS];
authrequest_t cl_authrequest;

int g_PF_VELOCITYXY;
int g_PF_SPECTATOR;
int g_PF_GAITSEQUENCE;
int g_PF_MODEL_LONG;
int g_PF_ONTRAIN;
int g_PF_MODEL;
int g_PF_FRAMERATE;
int g_PF_BODY;
int g_PF_DEAD;
int g_PF_NOGRAV;
int g_PF_VELOCITY3;
int g_PF_VELOCITY2;
int g_PF_VELOCITY1;
int g_PF_GIB;
int g_PF_EFFECTS;
int g_PF_PING;
int g_PF_RENDER;
int g_PF_MSEC;
int g_PF_MOREBITS;
int g_PF_WATERJUMP;
int g_PF_MOVETYPE;
int g_PF_WEAPONMODEL;
int g_PF_SEQUENCE;
int g_PF_CONTROLLER1;
int g_PF_CONTROLLER2;
int g_PF_CONTROLLER3;
int g_PF_CONTROLLER4;
int g_PF_FROZEN;
int g_PF_COMMAND;
int g_PF_YETMOREBITS;
int g_PF_SKINNUM;
int g_PF_FRICTION;
int g_PF_BLENDING1;
int g_PF_BLENDING2;
int g_PF_BASEVELOCITY;
int g_PF_DUCKING;
int g_PF_EVENMOREBITS;
int g_PF_WEAPONMODEL_LONG;

/*
==================
Protocol_Init

Installs the delta descriptor tables that match the negotiated protocol version.
==================
*/
void Protocol_Init( int version )
{
	if (version == PROTOCOL_VERSION_OLD)
	{
		g_PF_MSEC = 0x00000001;
		g_PF_COMMAND = 0x00000002;
		g_PF_VELOCITY1 = 0x00000004;
		g_PF_VELOCITY2 = 0x00000008;
		g_PF_VELOCITY3 = 0x00000010;
		g_PF_MODEL = 0x00000020;
		g_PF_SKINNUM = 0x00000040;
		g_PF_EFFECTS = 0x00000080;
		g_PF_WEAPONMODEL = 0x00000100;
		g_PF_DEAD = 0x00000200;
		g_PF_GIB = 0x00000400;
		g_PF_NOGRAV = 0x00000800;
		g_PF_MOVETYPE = 0x00001000;
		g_PF_SEQUENCE = 0x00002000;
		g_PF_RENDER = 0x00004000;
		g_PF_FRAMERATE = 0x00008000;
		g_PF_BODY = 0x00010000;
		g_PF_CONTROLLER1 = 0x00020000;
		g_PF_CONTROLLER2 = 0x00040000;
		g_PF_CONTROLLER3 = 0x00080000;
		g_PF_CONTROLLER4 = 0x00100000;
		g_PF_BLENDING1 = 0x00200000;
		g_PF_BLENDING2 = 0x00400000;
		g_PF_BASEVELOCITY = 0x00800000;
		g_PF_FRICTION = 0x01000000;
		g_PF_PING = 0x02000000;
		g_PF_GAITSEQUENCE = 0x04000000;
	}
	else
	{
		g_PF_ONTRAIN = 0x00000001;
		g_PF_DUCKING = 0x00000002;
		g_PF_FROZEN = 0x00000004;
		g_PF_SPECTATOR = 0x00000008;
		g_PF_WATERJUMP = 0x00000010;

		g_PF_COMMAND = 0x00000001;
		g_PF_MODEL = 0x00000002;
		g_PF_SEQUENCE = 0x00000004;
		g_PF_GAITSEQUENCE = 0x00000008;
		g_PF_MSEC = 0x00000010;
		g_PF_VELOCITYXY = 0x00000020;
		g_PF_WEAPONMODEL = 0x00000040;
		g_PF_MOREBITS = 0x00000080;
		g_PF_MODEL_LONG = 0x00000100;
		g_PF_WEAPONMODEL_LONG = 0x00000200;
		g_PF_VELOCITY3 = 0x00000400;
		g_PF_PING = 0x00000800;
		g_PF_EFFECTS = 0x00001000;
		g_PF_FRAMERATE = 0x00002000;
		g_PF_BASEVELOCITY = 0x00004000;
		g_PF_EVENMOREBITS = 0x00008000;
		g_PF_CONTROLLER1 = 0x00010000;
		g_PF_CONTROLLER2 = 0x00020000;
		g_PF_CONTROLLER3 = 0x00040000;
		g_PF_CONTROLLER4 = 0x00080000;
		g_PF_BLENDING1 = 0x00100000;
		g_PF_BLENDING2 = 0x00200000;
		g_PF_BODY = 0x00400000;
		g_PF_YETMOREBITS = 0x00800000;
		g_PF_SKINNUM = 0x01000000;
		g_PF_FRICTION = 0x02000000;
		g_PF_RENDER = 0x04000000;
		g_PF_DEAD = 0x08000000;
		g_PF_MOVETYPE = 0x10000000;
	}
}

/*
==================
WON_IsValidAuthMessage
==================
*/
qboolean WON_IsValidAuthMessage( int type )
{
	switch (type)
	{
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
		return TRUE;
	default:
		return FALSE;
	}
}

/*
==================
SV_ClearAuthRequest
==================
*/
void SV_ClearAuthRequest( netadr_t* from )
{
	int slot;

	slot = SV_FindAuthRequest(FALSE, from);
	if (slot != -1)
		memset(&sv_authrequests[slot], 0, sizeof(authrequest_t));
}

/*
==================
SV_FindAuthRequest
==================
*/
int SV_FindAuthRequest( qboolean create, netadr_t* from )
{
	authrequest_t* request;
	int i;

	for (i = 0, request = sv_authrequests; i < MAX_AUTHREQUESTS; i++, request++)
	{
		if (request->active && NET_CompareAdr(request->address, *from))
			return i;
	}

	if (!create)
		return -1;

	for (i = 0, request = sv_authrequests; i < MAX_AUTHREQUESTS; i++, request++)
	{
		if (!request->active)
			break;
	}

	if (i == MAX_AUTHREQUESTS)
	{
		Con_Printf("No more authentication slots available.\n");
		return -1;
	}

	memset(request, 0, sizeof(authrequest_t));
	request->nexttime = realtime + (3 - request->retries) * 5.0f;
	request->message = 0x33;
	request->retries = 3;
	request->address = *from;
	request->active = TRUE;
	request->userid = -1;
	return i;
}

qboolean SV_AuthenticateClient( netadr_t* from, char* certificate, int* userid )
{
	Sys_Error("NYI");
	return TRUE;
}

/*
==================
WON_ParseAuthenticationMessage
==================
*/
void WON_ParseAuthenticationMessage( int type )
{
	Sys_Error("NYI");
}

/*
==================
CL_ParseAuthenticationMessage
==================
*/
void CL_ParseAuthenticationMessage( int type )
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
	memset(sv_authrequests, 0, sizeof(sv_authrequests));
	memset(&cl_authrequest, 0, sizeof(cl_authrequest));
}

/*
==================
WON_ShutdownAuthentication
==================
*/
void WON_ShutdownAuthentication( void )
{
	Sys_Error("NYI");
}

/*
==================
WON_RemoveUser
==================
*/
void WON_RemoveUser( authrequest_t* request )
{
	memset(request, 0, sizeof(authrequest_t));
}

/*
==================
WON_HandleServerAuthMsgs

Retransmits any pending server authentication requests to the master.
==================
*/
void WON_HandleServerAuthMsgs( void )
{
	authrequest_t* request;
	byte packet[32];
	int i;

	for (i = 0, request = sv_authrequests; i < MAX_AUTHREQUESTS; request++, i++)
	{
		if (!sv_authrequests[i].active)
			continue;

		if (request->nexttime < realtime && request->packet && request->retries >= 0)
		{
			request->retries--;
			request->nexttime = realtime + (3 - request->retries) * 5.0f;

			if (request->packetlength < 32)
			{
				memset(packet, 1, 32);
				memcpy(packet, request->packet, request->packetlength);
				NET_SendPacket(NS_SERVER, 32, packet, request->address);
			}
			else
			{
				NET_SendPacket(NS_SERVER, request->packetlength, request->packet, request->address);
			}

			continue;
		}

		if (!request->packet || request->retries < 0)
		{
			memset(request, 0, sizeof(authrequest_t));
			return;
		}
	}
}

/*
==================
WON_HandleClientAuthMsgs
==================
*/
void WON_HandleClientAuthMsgs( void )
{
	if (!cl_authrequest.active)
		return;

	if (cl_authrequest.message == 0x37)
	{
		if (cl_authrequest.nexttime < realtime && cl_authrequest.retries >= 0)
		{
			cl_authrequest.retries--;
			cl_authrequest.nexttime = realtime + (3 - cl_authrequest.retries) * 5.0f;
			CL_SendConnectPacket();
			return;
		}

		if (cl_authrequest.retries < 0)
		{
			memset(&cl_authrequest, 0, sizeof(cl_authrequest));
			Con_Printf("Connection failed after auth response\n");
		}

		return;
	}

	if (cl_authrequest.nexttime < realtime &&
		cl_authrequest.packet && cl_authrequest.retries >= 0)
	{
		cl_authrequest.retries--;
		cl_authrequest.nexttime = realtime + (3 - cl_authrequest.retries) * 5.0f;
		NET_SendPacket(NS_CLIENT, cl_authrequest.packetlength,
			cl_authrequest.packet, cl_authrequest.address);
		return;
	}

	if (!cl_authrequest.packet || cl_authrequest.retries < 0)
	{
		if (cl_authrequest.packet)
			Con_Printf("Server validation failed after %i retries", 3);

		memset(&cl_authrequest, 0, sizeof(cl_authrequest));
		CL_Disconnect_f();
	}
}

/*
==================
WON_RequestCertificate
==================
*/
void WON_RequestCertificate( void )
{
	Sys_Error("NYI");
}

int SV_GetAuthUserID( netadr_t *from )
{
	int slot = SV_FindAuthRequest(FALSE, from);
	if (slot == -1)
		return -1;
	return sv_authrequests[slot].userid;
}

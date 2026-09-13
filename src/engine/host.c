// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"
#include "winquake.h"
#include "cmodel.h"
#include "cl_servercache.h"
#include "profile.h"
#include "hashpak.h"
#include "won.h"
#include "ui.h"
#include "dc_accum.h"
#include "info.h"
#include "kzap.h"
#include "mnemo.h"
#include "sys.h"
#include "text_draw.h"
#include "vmu.h"

#ifdef _WIN32_WCE
#pragma optimize("", off)
#endif

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution

float		host_frametime;
float		host_time;
float		realtime;			// without any filtering or bounding
float		oldrealtime;		// last frame run

int			host_framecount;
int			host_hunklevel;

int			minimum_memory;

client_t* host_client;			// current client

jmp_buf 	host_abortserver;
jmp_buf		host_enddemo;

unsigned short* host_basepal;
unsigned char* host_colormap;

// Master server
qboolean	gfNoMasterServer = FALSE;
float		gfLastHearbeat;				// Time we sent last heartbeat
qboolean	gfHeartbeatWaiting;			// Challenge request sent to master
float		gfHeartbeatWaitingTime;		// Challenge request send time
int			gHeartbeatSequence;			// # of heartbeat sequence
int			gHeartbeatChallenge;		// Last one is Main master
char		gszMasterAddress[128];
netadr_t	master_adr;					// Master server address

char		gszDefaultRoom[64];

master_t	*valvemaster_adr = NULL;	// linked list of WON master servers

extern cvar_t	sv_lan;

char		gpszVersionString[32];		// patch version read from sierra.inf

extern kbutton_t	in_jlook;

cvar_t	fps_single = { "fps_single", "66.0" };
cvar_t	fps_lan    = { "fps_lan",    "66.0" };
cvar_t	fps_modem  = { "fps_modem",  "66.0" };

float g_fFrameTime = 0.0f;

cvar_t	host_framerate = { "host_framerate", "0" };
cvar_t	host_speeds = { "host_speeds", "0" };			// set for running times
cvar_t	host_killtime = { "host_killtime", "0" };
cvar_t	serverprofile = { "serverprofile", "0" };

cvar_t	mp_logfile = { "mp_logfile", "1", FCVAR_SERVER };
cvar_t	mp_logecho = { "mp_logecho", "1", FCVAR_SERVER };

cvar_t	developer = { "developer", "0" };

cvar_t	skill = { "skill", "1" };						// 0 - 3
cvar_t	deathmatch = { "deathmatch", "0", FCVAR_SERVER };			// 0, 1, or 2
cvar_t	coop = { "coop", "0", FCVAR_SERVER };

cvar_t	pausable = { "pausable", "1", FCVAR_SERVER };

/*
================
COM_EntsForPlayerSlots

Returns the appropriate size of the edicts array to allocate
based on the stated # of max players
================
*/
int COM_EntsForPlayerSlots( int nPlayers )
{
	return 15 * (nPlayers - 1) + 800;
}


/*
================
Host_EndGame
================
*/
void Host_EndGame( char* message, ... )
{
	int oldn;
	va_list		argptr;
	char		string[1024];

	va_start(argptr, message);
	vsprintf(string, message, argptr);
	va_end(argptr);
	Con_DPrintf("Host_EndGame: %s\n", string);

	oldn = cls.demonum;

	scr_disabled_for_loading = TRUE;

	if (sv.active)
		Host_ShutdownServer(FALSE);

	cls.demonum = oldn;

	if (cls.state == ca_dedicated)
		Sys_Error("Host_EndGame: %s\n", string);	// dedicated servers exit

	if (cls.demonum != -1)
	{
		CL_Disconnect_f();
		cls.demonum = oldn;
		CL_NextDemo();
		longjmp(host_enddemo, 1);
	}

	scr_disabled_for_loading = FALSE;

	CL_Disconnect();

	Cbuf_AddText("cd stop\n");
	Cbuf_Execute();
	longjmp(host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error( char* error, ... )
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = FALSE;

	if (inerror)
		Sys_Error("Host_Error: recursively entered");
	inerror = TRUE;

	SCR_EndLoadingPlaque();		// reenable screen updates

	va_start(argptr, error);
	vsprintf(string, error, argptr);
	va_end(argptr);
	Con_Printf("Host_Error: %s\n", string);

	if (sv.active)
		Host_ShutdownServer(FALSE);

	if (cls.state == ca_dedicated)
		Sys_Error("Host_Error: %s\n", string);	// dedicated servers exit

	CL_Disconnect();
	cls.demonum = -1;

	inerror = FALSE;

	longjmp(host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void Host_FindMaxClients( void )
{
	int		i;
	client_t* cl;

	svs.maxclients = 1;

	// Check for command line override
	i = COM_CheckParm("-maxplayers");
	if (i)
	{
		svs.maxclients = Q_atoi(com_argv[i + 1]);
	}
	else if (isDedicated)
	{
		svs.maxclients = MAX_CLIENTS;
	}

	if (isDedicated)
		cls.state = ca_dedicated;
	else
		cls.state = ca_disconnected;

	if (svs.maxclients < 1)
	{
		svs.maxclients = DEFAULT_SERVER_CLIENTS;
	}
	else if (svs.maxclients > MAX_CLIENTS)
	{
		svs.maxclients = MAX_CLIENTS;
	}

	// Determine absolute limit
	svs.maxclientslimit = 32;

	// If we're a listen server and we're low on memory, reduce maximum player limit
	if (host_parms.memsize <= 0x1000000)
	{
		svs.maxclientslimit = 4;
	}

	// Single player only needs a shallow frame history
	if (svs.maxclients == 1)
		SV_UPDATE_BACKUP = SINGLEPLAYER_BACKUP;
	else
		SV_UPDATE_BACKUP = MULTIPLAYER_BACKUP;
	SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

	svs.clients = (client_t*)Hunk_AllocName(sizeof(client_t) * svs.maxclientslimit, "clients");

	for (i = 0, cl = svs.clients; i < svs.maxclientslimit; i++, cl++)
	{
		memset(cl, 0, sizeof(client_t));

		cl->resourcesneeded.pPrev = &cl->resourcesneeded;
		cl->resourcesneeded.pNext = &cl->resourcesneeded;
		cl->resourcesonhand.pPrev = &cl->resourcesonhand;
		cl->resourcesonhand.pNext = &cl->resourcesonhand;
	}

	if (svs.maxclients > 1)
		Cvar_SetValue("deathmatch", 1.0);
	else
		Cvar_SetValue("deathmatch", 0.0);

	SV_AllocClientFrames();

	if (svs.maxclientslimit < svs.maxclients)
		svs.maxclients = svs.maxclientslimit;
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal( void )
{
	Host_InitCommands();

	Cvar_RegisterVariable(&fps_single);
	Cvar_RegisterVariable(&fps_lan);
	Cvar_RegisterVariable(&fps_modem);
	Cvar_RegisterVariable(&host_framerate);
	Cvar_RegisterVariable(&host_speeds);
	Cvar_RegisterVariable(&serverprofile);
	Cvar_RegisterVariable(&mp_logfile);
	Cvar_RegisterVariable(&mp_logecho);
	Cvar_RegisterVariable(&host_killtime);
	Cvar_RegisterVariable(&developer);
	Cvar_RegisterVariable(&deathmatch);
	Cvar_RegisterVariable(&coop);
	Cvar_RegisterVariable(&pausable);
	Cvar_RegisterVariable(&skill);

	Host_FindMaxClients();

	host_time = 1.0;		// so a think at time 0 will not get called
}

/*
===============
Info_WriteVars

Cvar_WriteVariables already wrote every archived cvar, so this only needs to
save the userinfo keys that a mod set by hand and never backed with a cvar
("*" keys are server-assigned and never saved).
===============
*/
void Info_WriteVars( void* f )
{
	char	key[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char	*s;
	char	*o;
	cvar_t	*var;

	s = cls.userinfo;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = key;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		var = Cvar_FindVar(key);
		if (!var && key[0] != '*')
			Sys_FPrintf(f, "setinfo \"%s\" \"%s\"\n", key, value);

		if (!*s)
			return;
		s++;
	}
}

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration( void )
{
	void	*f;
	int		nbinds;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	if (host_initialized && cls.state != ca_dedicated)
	{
		nbinds = Key_CountBindings();
		if (nbinds < 2)
		{
			Con_Printf("skipping config.cfg output, no keys bound\n");
			return;
		}

		f = (void *)Bopen("halflife.cfg", "w");
		if (!f)
		{
			Con_Printf("Couldn't write config.cfg.\n");
			return;
		}

		Sys_FPrintf(f, "unbindall\n");
		Key_WriteBindings(f);
		Cvar_WriteVariables(f);
		Info_WriteVars(f);
#if HLDC_MP
		CL_WriteServerList(f);
#endif

		if (in_mlook.state & 1)
			Sys_FPrintf(f, "+mlook\n");
		if (in_jlook.state & 1)
			Sys_FPrintf(f, "+jlook\n");

		Sys_CloseHandle(f);

		GDROM_SetDoorBehavior();
		VMU_SaveGameHL4("halflife.cfg");
		GDROM_ConfigureDoorBehavior();
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf( char* fmt, ... )
{
	va_list		argptr;
	char		string[1024];

	if (!host_client->fakeclient)
	{
		va_start(argptr, fmt);
		vsprintf(string, fmt, argptr);
		va_end(argptr);

		MSG_WriteByte(&host_client->netchan.message, svc_print);
		MSG_WriteString(&host_client->netchan.message, string);
	}
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf( char* fmt, ... )
{
	va_list		argptr;
	char		string[1024];
	int			i;

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	for (i = 0; i < svs.maxclients; i++)
	{
		if ((svs.clients[i].active || svs.clients[i].spawned) && !svs.clients[i].fakeclient)
		{
			MSG_WriteByte(&svs.clients[i].netchan.message, svc_print);
			MSG_WriteString(&svs.clients[i].netchan.message, string);
		}
	}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands( char* fmt, ... )
{
	va_list		argptr;
	char		string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
	MSG_WriteString(&host_client->netchan.message, string);
}

/*
=================
SV_DropClient

Drops client from server, with explanation
if (crash = true), don't bother sending signofs
=================
*/
void SV_DropClient( client_t *cl, qboolean crash )
{
	byte	final[20];

	if (!crash)
	{
		// add the disconnect
		if (!cl->fakeclient)
		{
			MSG_WriteByte(&cl->netchan.message, svc_disconnect);
			final[0] = svc_disconnect;
		}

		if (cl->edict && cl->spawned)
		{
			if (cl->spectator)
				gEntityInterface.pfnSpectatorDisconnect(cl->edict);
			else
				gEntityInterface.pfnClientDisconnect(cl->edict);
		}

		if (cl->download)
		{
			COM_FreeFile(cl->download);
			cl->download = NULL;
		}

		if (cl->upload)
		{
			Sys_CloseHandle(cl->upload);
			cl->upload = NULL;
		}

		// flush the final disconnect message out immediately
		Netchan_Transmit(&cl->netchan, 1, final);
	}

// free the client (the body stays around)
	cl->active = FALSE;
	cl->spawned = FALSE;
	cl->connected = FALSE;
	cl->connection_started = realtime;
	memset(cl->name, 0, sizeof(cl->name));

// send notification to all other clients
	SV_FullClientUpdate(cl, &sv.reliable_datagram);
}

/*
==================
Host_ClearClients

==================
*/
void Host_ClearClients( qboolean bFramesOnly )
{
	int		i, j;
	client_frame_t* frame;

	host_client = svs.clients;

	for (i = 0; i < svs.maxclients; i++, host_client++)
	{
		if (host_client->frames)
		{
			for (j = 0; j < SV_UPDATE_BACKUP; j++)
			{
				frame = &host_client->frames[j];

				// Must clear out all dynamic data for each frame
				SV_ClearPacketEntities(frame);

				frame->senttime = 0;
				frame->ping_time = -1;
			}
		}

		if (host_client->netchan.remote_address.type != NA_UNUSED)
		{
			netadr_t save;
			memcpy(&save, &host_client->netchan.remote_address, sizeof(netadr_t));
			memset(&host_client->netchan, 0, sizeof(netchan_t));
			Netchan_Setup(NS_SERVER, &host_client->netchan, save);
		}
	}

	if (!bFramesOnly)
	{
		host_client = svs.clients;
		for (i = 0; i < svs.maxclientslimit; i++, host_client++)
			SV_ClearFrames(&host_client->frames);

		memset(svs.clients, 0, sizeof(client_t) * svs.maxclientslimit);

		SV_AllocClientFrames();
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer( qboolean crash )
{
	int		i;

	if (!sv.active)
		return;

	sv.active = FALSE;

	if (cls.state == ca_connecting
		|| cls.state == ca_connected
		|| cls.state == ca_uninitialized
		|| cls.state == ca_active)
	{
		CL_Disconnect();
	}

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if ((host_client->active || host_client->connected) && !host_client->fakeclient)
			SV_DropClient(host_client, crash);
	}

	// Clear all entities
	SV_ClearEntities();

	memset(&sv, 0, sizeof(server_t));

	//
	// clear structures
	//

	CL_ClearClientState();
	SV_ClearClientStates();
	Host_ClearClients(FALSE);

	for (i = 0, host_client = svs.clients; i < svs.maxclientslimit; i++, host_client++)
		SV_ClearFrames(&host_client->frames);

	memset(svs.clients, 0, sizeof(client_t) * svs.maxclientslimit);

	Master_Shutdown(FALSE);

	Log_Printf("Server shutdown.\n");
	Log_Close();
}

void SV_ClearClientStates( void )
{
	int		i;
	client_t* cl;

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		SV_ClearResourceLists(cl);
	}
}

/*
================
Host_CheckDynamicStructures

Release any per-client frame data still allocated across all client slots
================
*/
void Host_CheckDynamicStructures( void )
{
	int		i;
	client_t* cl;

	if (svs.clients)
	{
		for (i = 0, cl = svs.clients; i < svs.maxclientslimit; i++, cl++)
		{
			if (!cl)
				return;

			if (cl->frames)
				SV_ClearFrames(&cl->frames);
		}
	}
}

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory( qboolean bQuiet )
{
	CM_FreePAS();
	SV_ClearEntities();

	D_FlushCaches();
	Mod_ClearAll();

	if (host_hunklevel)
	{
		Host_CheckDynamicStructures();
		Hunk_FreeToLowMark(host_hunklevel);
	}

	cls.signon = 0;
	memset(&sv, 0, sizeof(server_t));

	CL_ClearClientState();
	SV_ClearClientStates();
}


//============================================================================


/*
===============
Host_FilterTime

Computes simulation time (FPS value)
===============
*/
qboolean Host_FilterTime( float time )
{
	float fps;

	realtime += time;

	if (!isDedicated)
	{
		fps = fps_single.value;

		if (!sv.active)
		{
			fps = fps_modem.value;
			if (rate.value > 5000.0f)
				fps = fps_lan.value;
		}

		if (fps != 0.0f)
		{
			// Clamp the requested framerate to a sane range.
			if (fps >= 0.1f)
			{
				if (fps > 72.0f)
					fps = 72.0f;
			}
			else
			{
				fps = 0.1f;
			}

			if ((realtime - oldrealtime) < 1.0f / fps)
				return FALSE;	/* framerate is too high */
		}
	}

	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_framerate.value <= 0.0f || !IsSinglePlayerGame())
	{
		if (host_frametime > 0.1f)
			host_frametime = 0.1f;
		if (host_frametime < 0.001f)
			host_frametime = 0.001f;
	}
	else
	{
		host_frametime = host_framerate.value;
	}

	return TRUE;
}

/*
==================
Host_ServerFrame

==================
*/
void Host_ServerFrame( void )
{
	float	time1 = 0;
	float	time2 = 0;
	float	time3 = 0;
	float	time4 = 0;
	float	time5 = 0;

	if (host_speeds.value)
		time1 = Sys_FloatTime();

// run the world state
	gGlobalVariables.frametime = host_frametime;

// read client messages
	SV_ReadPackets();

	if (host_speeds.value)
		time2 = Sys_FloatTime();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game && (cls.state == ca_active || cls.state == ca_dedicated)))
		SV_Physics();

	if (host_speeds.value)
		time3 = Sys_FloatTime();

	// Send the results of movement and physics to the clients
	SV_QueryMovevarsChanged();
	SV_SendClientMessages();

	if (host_speeds.value)
		time4 = Sys_FloatTime();

// send a heartbeat to the master if needed
	Master_Heartbeat();

	if (host_speeds.value)
		time5 = Sys_FloatTime();
}

//============================================================================

/*
==================
Master_RequestHeartbeat

Sends a heartbeat to the master server
==================
*/
#if HLDC_MP
void Master_RequestHeartbeat( void )
{
	static char	string[2048];    // Buffer for sending heartbeat
	static char	szChannels[256];
	int			active;          // Number of active client connections
	int			numChannels;
	svchannel_t* pChannel;

	if (!NET_StringToAdr(gszMasterAddress, &master_adr))
		return;

	// Still waiting on challenge response?
	if (gfHeartbeatWaiting)
		return;

	// Waited too long
	if ((realtime - gfHeartbeatWaitingTime) >= HB_TIMEOUT)
		return;

	//
	// count active users
	//
	SV_CountPlayers(&active, NULL);

	numChannels = 0;

	gHeartbeatSequence++;

	pChannel = svchannels;
	while (pChannel)
	{
		numChannels++;
		pChannel = pChannel->pNext;
	}

	memset(string, 0, sizeof(string));
	memset(szChannels, 0, sizeof(szChannels));

	pChannel = svchannels;
	while (pChannel)
	{
		strcat(szChannels, pChannel->szServerChannel);
		strcat(szChannels, "\n");
		pChannel = pChannel->pNext;
	}

	// Send to master
	sprintf(string, "%c\n%i\n%i\n%i\n%i\n%s", S2M_HEARTBEAT, gHeartbeatChallenge,
		gHeartbeatSequence, active, numChannels, szChannels);

	NET_SendPacket(NS_SERVER, strlen(string), string, master_adr);
}
#endif

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat( void )
{
	master_t		*p;
	unsigned char	c;

	if (gfNoMasterServer ||      // We are ignoring heartbeats
		gfUseLANAuthentication ||
		sv_lan.value ||
		svs.maxclients <= 1 ||   // not a multiplayer server
		!sv.active)
		return;

	Master_Init();

	for (p = valvemaster_adr; p != NULL; p = p->next)
	{
		if (HEARTBEAT_SECONDS <= realtime - p->heartbeattime)
		{
			// Resend a challenge request only if the last one has timed out.
			if (p->state == 0 || HB_TIMEOUT <= realtime - p->challengetime)
			{
				p->state = 1;
				p->challengetime = realtime;
				p->heartbeattime = realtime;

				// Send to master asking for a challenge #
				c = A2A_GETCHALLENGE;
				NET_SendPacket(NS_SERVER, 1, &c, p->adr);
			}
		}
	}
}

/*
==================
Master_Shutdown

Server is shutting down, unload master servers list, tell masters that we are closing the server
==================
*/
void Master_Shutdown( qboolean bFree )
{
	master_t	*p;
	master_t	*next;
	char		string[2048];

	if (gfNoMasterServer ||      // We are ignoring heartbeats
		gfUseLANAuthentication ||
		sv_lan.value ||
		svs.maxclients <= 1)     // not a multiplayer server
		return;

	Master_Init();

	sprintf(string, "%c\n", S2M_SHUTDOWN);

	for (p = valvemaster_adr; p != NULL; p = p->next)
		NET_SendPacket(NS_SERVER, strlen(string), string, p->adr);

	// unload the master list
	if (bFree)
	{
		for (p = valvemaster_adr; p != NULL; p = next)
		{
			next = p->next;
			free(p);
		}

		valvemaster_adr = NULL;
	}
}

/*
==================
Master_AddServer

Add a master server to the list, if it isn't already present
==================
*/
void Master_AddServer( netadr_t *adr )
{
	master_t	*p;

	Master_Init();

	for (p = valvemaster_adr; p != NULL; p = p->next)
	{
		if (NET_CompareAdr(p->adr, *adr))
			break;
	}

	if (p == NULL)
	{
		p = (master_t *)MnemoAllocDbg(sizeof(master_t), __FILE__, __LINE__);
		if (p == NULL)
			Sys_ErrorColor(RGB565_RED, "Error allocating %i bytes for master address.", sizeof(master_t));

		memset(p, 0, sizeof(master_t));

		p->adr = *adr;
		p->heartbeattime = -99999.0f;

		p->next = valvemaster_adr;
		valvemaster_adr = p;
	}
}

/*
==================
Master_SetMaster_f

setmaster < add | remove | enable | disable > < IP:port >
==================
*/
void Master_SetMaster_f( void )
{
	int			argc;
	char		*cmd;
	char		*addr;
	char		*pszPort;
	int			port;
	char		portstring[32];
	char		string[256];
	netadr_t	adr;
	master_t	*p;
	master_t	*prev;
	int			i;

	port = PORT_MASTER;
	sprintf(portstring, "%i", PORT_MASTER);

	argc = Cmd_Argc();
	if (argc < 2 || argc > 4)
	{
		Con_Printf("Usage:\nSetmaster <add | remove | enable | disable> <IP:port>\n");

		if (valvemaster_adr == NULL)
		{
			Con_Printf("Current:  None\n");
		}
		else
		{
			i = 1;
			Con_Printf("Current:\n");
			for (p = valvemaster_adr; p != NULL; p = p->next)
			{
				Con_Printf("  %i:  %s\n", i, NET_AdrToString(p->adr));
				i++;
			}
		}

		return;
	}

	cmd = Cmd_Argv(1);
	if (!cmd || !cmd[0])
		return;

	if (!Q_stricmp(cmd, "disable"))
	{
		gfNoMasterServer = TRUE;
		return;
	}

	if (!Q_stricmp(cmd, "enable"))
	{
		gfNoMasterServer = FALSE;
		return;
	}

	if (Q_stricmp(cmd, "add") && Q_stricmp(cmd, "remove"))
	{
		Con_Printf("Setmaster:  Unknown command %s\n", cmd);
		return;
	}

	addr = Cmd_Argv(2);

	if (argc == 4)
	{
		pszPort = Cmd_Argv(3);
		if (!pszPort || !pszPort[0])
		{
			pszPort = portstring;
		}
		else
		{
			port = atoi(pszPort);
			if (!port)
				port = PORT_MASTER;
		}
	}

	sprintf(string, "%s:%i", addr, port);

	if (!NET_StringToAdr(string, &adr))
	{
		Con_Printf(" Invalid address \"%s\", setmaster command ignored\n", string);
		return;
	}

	if (!Q_stricmp(cmd, "add"))
	{
		Master_Init();
		Master_AddServer(&adr);
		gfNoMasterServer = FALSE;
		Con_Printf("Adding master at %s\n", string);
	}
	else
	{
		Master_Init();

		if (valvemaster_adr == NULL)
		{
			Con_Printf("Can't remove master, list is empty\n");
			return;
		}

		for (p = valvemaster_adr; p != NULL; p = p->next)
		{
			if (NET_CompareAdr(p->adr, adr))
				break;
		}

		if (p == NULL)
		{
			Con_Printf("Can't remove master %s, not in list\n", string);
		}
		else if (p == valvemaster_adr)
		{
			valvemaster_adr = valvemaster_adr->next;
			free(p);
		}
		else
		{
			for (prev = valvemaster_adr; prev != NULL && prev->next != p; prev = prev->next)
				;

			if (prev != NULL)
			{
				prev->next = p->next;
				free(p);
			}
		}
	}
}

/*
==================
Master_Heartbeat_f

Force a heartbeat to be sent to all masters on the next server frame
==================
*/
void Master_Heartbeat_f( void )
{
	master_t	*p;

	for (p = valvemaster_adr; p != NULL; p = p->next)
		p->heartbeattime = -99999.0f;
}

/*
==================
Master_UseDefault

No masters were parsed from the list; fall back to the built-in one
==================
*/
void Master_UseDefault( void )
{
	netadr_t	adr;
	char		string[256];

	sprintf(string, "half-life.east.won.net:27010");

	if (NET_StringToAdr(string, &adr))
		Master_AddServer(&adr);
}

/*
==================
Master_RequestMOTD_f

Request for MOTD from Server Master
==================
*/
void Master_RequestMOTD_f( void )
{
	master_t	*p;
	char		string[2048];

	if (!gfNoMasterServer && valvemaster_adr)
	{
		NET_Config(TRUE);

		sprintf(string, "%c", A2M_GET_MOTD);

		for (p = valvemaster_adr; p != NULL; p = p->next)
			NET_SendPacket(NS_CLIENT, strlen(string), string, p->adr);
	}
}

/*
==================
Master_Init

Build the list of master servers from woncomm.lst, or fall back to the default
==================
*/
void Master_Init( void )
{
	static qboolean	initialized = FALSE;
	char		address[256];
	char		filename[256];
	char		*data;
	char		*p;
	int			comm;
	int			count;
	int			port;
	netadr_t	adr;

	if (gfNoMasterServer ||
		gfUseLANAuthentication ||
		sv_lan.value ||
		svs.maxclients <= 1 ||
		initialized)
		return;

	initialized = TRUE;

	sprintf(address, "half-life.east.won.net:27010");
	sprintf(filename, "%s", "woncomm.lst");

	comm = COM_CheckParm("-comm");
	if (comm && comm < com_argc - 1)
		strcpy(filename, com_argv[comm + 1]);

	data = (char *)COM_LoadFile(filename, 5, NULL);
	if (!data)
	{
		Con_Printf("Couldn't load woncomm.lst\nUsing default master\n");
		Master_UseDefault();
		return;
	}

	count = 0;

	for (p = data; ; )
	{
		p = COM_Parse(p);
		if (!strlen(com_token))
			break;

		if (!Q_stricmp(com_token, "Master"))
			port = PORT_MASTER;

		p = COM_Parse(p);
		if (!strlen(com_token))
			break;
		if (Q_stricmp(com_token, "{"))
			break;

		while (1)
		{
			p = COM_Parse(p);
			if (!strlen(com_token))
				break;
			if (!Q_stricmp(com_token, "}"))
				break;

			sprintf(address, "%s", com_token);

			p = COM_Parse(p);
			if (!strlen(com_token))
				break;
			if (Q_stricmp(com_token, ":"))
				break;

			p = COM_Parse(p);
			if (!strlen(com_token))
				break;

			port = atoi(com_token);
			if (!port)
				port = PORT_MASTER;

			sprintf(filename, "%s:%i", address, port);
			if (NET_StringToAdr(filename, &adr))
			{
				Con_Printf("Adding master server %s\n", NET_AdrToString(adr));
				Master_AddServer(&adr);
				count++;
			}
		}
	}

	if (count == 0)
	{
		Con_Printf("No masters parsed from woncomm.lst\nUsing default master\n");
		Master_UseDefault();
	}

	free(data);
}

/*
==================
Host_PostFrameRate
==================
*/
void Host_PostFrameRate( float frameTime )
{
	g_fFrameTime = frameTime;
}

/*
==================
Host_GetHostInfo
==================
*/
DLL_EXPORT void Host_GetHostInfo( float* fps, int* nActive, int* nSpectators, int* nMaxPlayers, char* pszMap )
{
	int		clients;
	int		spectators;

	*fps = g_fFrameTime;

	spectators = 0;
	clients = 0;

	// Count clients, report 
	SV_CountPlayers(&clients, &spectators);
	clients -= spectators;

	*nActive = clients;
	*nSpectators = spectators;

	if (pszMap)
	{
		if (sv.name && sv.name[0])
			strcpy(pszMap, sv.name);
		else
			*pszMap = '\0';
	}

	*nMaxPlayers = svs.maxclients;
}

/*
===================
Host_Speeds

Print and update the running frame timings
===================
*/
void Host_Speeds( float time0, float time1, float time2, float time3 )
{
	float	frameTime;
	float	fps;
	int		ent_count;
	int		i;

	frameTime = (time1 - time0) * 1000.0f + (time2 - time1) * 1000.0f + (time3 - time2) * 1000.0f;

	if (frameTime >= 1.0f)
		fps = 1000.0f / frameTime;
	else
		fps = 100.0f;

	Host_PostFrameRate(fps);

	if (host_speeds.value)
	{
		ent_count = 0;
		for (i = 0; i < sv.num_edicts; i++)
		{
			if (!sv.edicts[i].free)
				ent_count++;
		}
	}
}

/*
===================
Host_HandleRconPacket

A connectionless packet arrived on the server socket; run it if it's an rcon command
===================
*/
void Host_HandleRconPacket( void )
{
	MSG_BeginReading();
	MSG_ReadLong();

	Cmd_TokenizeString( MSG_ReadStringLine() );

	if (!strcmp( Cmd_Argv( 0 ), "rcon" ))
		Host_RemoteCommand( &net_from );
}

/*
===================
Host_CheckForRcon

Pull any pending packets off the server socket and handle bans / rcon
===================
*/
void Host_CheckForRcon( void )
{
	while (NET_GetPacket( NS_SERVER ))
	{
		if (SV_FilterPacket())
			SV_SendBan();
		else if (*(int *)net_message.data == -1)
			Host_HandleRconPacket();
	}
}

/*
===================
Host_UpdateScreen
===================
*/
void Host_UpdateScreen( void )
{
	Mnemo_ProfileMeter();
	DC_PrintFileCounts();
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame( float time )
{
	static float		time0 = 0;
	static float		time1 = 0;
	static float		time2 = 0;
	static float		time3 = 0;

	Host_UpdateScreen();

	if (setjmp(host_enddemo))
		return;			// demo finished.

// decide the simulation time
	if (!Host_FilterTime(time))
		return;			// don't run too fast, or packets will flood out

	R_SetStackBase();

	if ((cls.state == ca_connected || cls.state == ca_uninitialized) && cls.signon == SIGNONS)
	{
		cls.state = ca_active;
	}

	Sys_SendKeyEvents();

	if (g_bInactive)
		return;

// allow mice or other external controllers to add commands
	IN_Commands();

// process console commands
	Cbuf_Execute();

	if (cls.state == ca_active)
		ClientDLL_UpdateClientData();

// if running the server locally, make intentions now
	if (sv.active)
		CL_SendCmd();

//-------------------
//
// server operations
//
//-------------------

	if (sv.active)
		Host_ServerFrame();

	if (!sv.active && cls.state == ca_disconnected)
		Host_CheckForRcon();

//-------------------
//
// client operations
//
//-------------------

// if running the server remotely, send intentions now after
// the incoming messages have been read
	if (!sv.active)
		CL_SendCmd();

	// fetch results from server
	CL_ReadPackets();

	if (cls.state == ca_active)
	{
		CL_SetUpPlayerPrediction(FALSE);
		CL_PredictMove();
		CL_SetUpPlayerPrediction(TRUE);
	}

	CL_EmitEntities();

	// Resend connection request if needed.
	CL_CheckForResend();

	while (CL_RequestMissingResources());

	R_UpdateAdaptive();

	WON_HandleClientAuthMsgs();

// check timeouts
	SV_CheckTimeouts();

	WON_HandleServerAuthMsgs();

	host_time += host_frametime;

	CAM_Think();

// update video
	time1 = Sys_FloatTime();
	if (!gfBackground)
	{
		// Refresh the screen
		SCR_UpdateScreen();

		// If recording movie and the console is totally up, then write out this frame to movie file.
		if (cl_inmovie && !scr_con_current)
		{
			VID_WriteBuffer(NULL);
		}
	}
	time2 = Sys_FloatTime();

	CL_UpdateSoundFade();

// update audio
	if (!gfBackground)
	{
		if (cls.signon == SIGNONS)
		{
			S_Update(r_origin, vpn, vright, vup);
			CL_DecayLights();
		}
		else
		{
			S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);
		}
	}

	time0 = time3;
	time3 = Sys_FloatTime();

	Host_Speeds(time0, time1, time2, time3);

	if ((giSubState & 4) && cls.state == ca_disconnected)
	{
		giActive = DLL_PAUSED;
	}

	host_framecount++;
}

/*
==============================
Host_Frame

==============================
*/
DLL_EXPORT int Host_Frame( float time, int iState, int* stateInfo )
{
	float	time1, time2;
	static float	timetotal = 0;
	static int		timecount = 0;
	int		i, c, m;

	if (setjmp(host_abortserver))
		return giActive;			// something bad happened, or the server disconnected

	if (cls.state == ca_active && g_bForceReloadOnCA_Active)
	{
		Host_ExecConfig();
		g_bForceReloadOnCA_Active = FALSE;
		memset(g_szProfileName, 0, sizeof(g_szProfileName));
	}

	giActive = iState;
	if (!serverprofile.value)
	{
		_Host_Frame(time);
		if (giStateInfo)
		{
			*stateInfo = giStateInfo;
			giStateInfo = 0;
			Cbuf_Execute();
		}

		return giActive;
	}

	time1 = Sys_FloatTime();
	_Host_Frame(time);
	time2 = Sys_FloatTime();

	if (giStateInfo)
	{
		*stateInfo = giStateInfo;
		giStateInfo = 0;
		Cbuf_Execute();
	}

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return 0;

	m = timetotal * 1000 / timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;
	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active)
			c++;
	}
	Con_Printf("serverprofile: %2i clients %2i msec\n", c, m);

	return 0;
}

/*
==================
CheckGore

The Dreamcast build always ships with violence enabled
==================
*/
void CheckGore( void )
{
	Cvar_SetValue("violence_hblood", 1.0);
	Cvar_SetValue("violence_hgibs", 1.0);
	Cvar_SetValue("violence_ablood", 1.0);
	Cvar_SetValue("violence_agibs", 1.0);
}

void Log_PrintServerVars( void )
{
	cvar_t* var;

	if (svs.log.active)
	{
		Log_Printf("server cvars start\n");
		for (var = cvar_vars; var; var = var->next)
		{
			if (var->flags & FCVAR_SERVER)
				Log_Printf("\"%s\" = \"%s\"\n", var->name, var->string);
		}
		Log_Printf("server cvars end\n");
	}
}

void Log_Printf( char* fmt, ... )
{
}

void Log_Close( void )
{
	if (svs.log.file)
	{
		Log_Printf("Log closed.\n");
		Sys_CloseHandle(svs.log.file);
	}

	svs.log.file = NULL;
}

void Log_Open( void )
{
	char szFileBase[MAX_PATH];
	char szTestFile[MAX_PATH];
	long ltime;
	struct tm* today;
	void* file;
	int i;

	file = NULL;
	if (!svs.log.active)
		return;

	Log_Close();
	time(&ltime);
	today = localtime(&ltime);
	sprintf(szFileBase, "%s/logs/L%02i%02i", com_gamedir, today->tm_mon + 1, today->tm_mday);

	for (i = 0; i < 1000; i++)
	{
		sprintf(szTestFile, "%s%03i.log", szFileBase, i);
		COM_FixSlashes(szTestFile);
		COM_CreatePath(szTestFile);
		file = Sys_OpenHandle(szTestFile, "r");
		if (!file)
		{
			COM_CreatePath(szTestFile);
			file = Sys_OpenHandle(szTestFile, "wt");
			if (!file)
				i = 1000;
			else
				Con_Printf("Server logging data to file %s\n", szTestFile);
			break;
		}
		Sys_CloseHandle(file);
	}

	if (i == 1000)
	{
		Con_Printf("Unable to open logfiles under %s\nLogging disabled\n", szFileBase);
		svs.log.active = FALSE;
	}
	else
	{
		if (file)
			svs.log.file = file;
		Log_Printf("Log file started.\n");
	}
}

/*
==================
Host_Version

Read the patch version out of sierra.inf and print the build banner
==================
*/
void Host_Version( void )
{
	char	name[256];
	void	*fp;
	int		length;
	char	*data;
	char	*p;

	strcpy(gpszVersionString, "1.0.1.3");

	sprintf(name, "sierra.inf");

	fp = (void *)Sys_OpenHandle(name, "r");
	if (fp)
	{
		DC_fseek(fp, 0, SEEK_END);
		length = DC_ftell(fp);
		DC_fseek(fp, 0, SEEK_SET);

		data = (char *)MnemoAllocDbg(length + 1, __FILE__, __LINE__);
		DC_fread(data, length, 1, fp);
		Sys_CloseHandle(fp);
		data[length] = 0;

		p = data;
		while (1)
		{
			p = COM_Parse(p);
			if (!p)
				break;
			if (!strlen(com_token))
				break;

			if (!Q_strnicmp(com_token, "PatchVersion=", strlen("PatchVersion=")))
			{
				strcpy(gpszVersionString, com_token + strlen("PatchVersion="));
				break;
			}
		}

		if (data)
			free(data);
	}

	if (cls.state == ca_dedicated)
	{
		Con_Printf("Protocol version %i\nExe version %s\n", PROTOCOL_VERSION, gpszVersionString);
		Con_Printf("Exe build: Dreamcast " __TIME__ " " __DATE__ " (Dreamcast %i)\n", build_number());
	}
}

/*
====================
Host_Init
====================
*/
int Host_Init( quakeparms_t* parms )
{
	int		i;
	int		protocol;

	if (standard_quake)
		minimum_memory = MINIMUM_MEMORY;
	else
		minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm("-minmemory"))
		parms->memsize = minimum_memory;

	host_parms = *parms;

	com_argc = parms->argc;
	com_argv = parms->argv;

	realtime = 0.0;

	// Allow the network protocol to be forced back to the legacy version.
	i = COM_CheckParm("-protocol");
	protocol = 0;
	if (i && (protocol = Q_atoi(com_argv[i + 1])) == PROTOCOL_VERSION_OLD)
		PROTOCOL_VERSION = PROTOCOL_VERSION_OLD;

	Protocol_Init(PROTOCOL_VERSION);

	Memory_Init(parms->membase, parms->memsize);

	// Fill the save/restore export registry from the statically linked game.
	GameDLL_RegisterModules();

	// Initialize command system
	Cbuf_Init();
	Cmd_Init();
	Cvar_CmdInit();

	V_Init();
	DCV_AccumInit();
	Chase_Init();

	COM_Init(parms->basedir);

	Host_InitLocal();

	W_LoadWadFile("gfx.wad");

	Key_Init();

	Con_Init();

	Decal_Init();
	Mod_Init();

	NET_Init();
	// Sequenced message stream layer.
	Netchan_Init();

	SV_Init();

	Text_LoadLangTags();

	Host_Version();

	R_InitTextures();		// needed even for dedicated servers

	if (cls.state != ca_dedicated)
	{
		char* disk_basepal;

		disk_basepal = (char*)COM_LoadHunkFile("gfx/palette.lmp");
		if (!disk_basepal)
			Sys_ErrorColor(RGB565_RED, "Couldn't load gfx/palette.lmp", 0);

		host_basepal = Hunk_AllocName(sizeof(PackedColorVec) * 256, "palette.lmp");

		for (i = 0; i < 256; i++)
		{
			host_basepal[i * 4 + 0] = disk_basepal[i * 3 + 2];
			host_basepal[i * 4 + 1] = disk_basepal[i * 3 + 1];
			host_basepal[i * 4 + 2] = disk_basepal[i * 3 + 0];
			host_basepal[i * 4 + 3] = 0;
		}

		ClientDLL_Init();

		if (!VID_Init(host_basepal))
			return 0;

		Draw_Init();

		SCR_Init();

		R_Init();

		S_Init();

		CL_Init();

		IN_Init();

		Host_UpdateScreenSaver(0);

		UI_Init();
	}

	// Execute the startup configs
	Cbuf_InsertText("exec preset_a.cfg\n");
	Cbuf_InsertText("exec valve.rc\n");

	if (FileExists("/CD-ROM/valve/halflife.cfg"))
		Cbuf_AddText("exec halflife.cfg\n");

	GL_Init();

	WON_InitAuthentication();

	// Mark hunklevel at end of startup
	host_hunklevel = Hunk_LowMark();

	// Mark DLL as active
	giActive = DLL_ACTIVE;
	// Enable rendering
	scr_skipupdate = FALSE;

	Cvar_SetValue("sv_cheats", 1.0);

	if (COM_CheckParm("-dev"))
		Cvar_SetValue("developer", 1.0);

	CheckGore();

	host_initialized = TRUE;

	return 1;
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown( void )
{
	static qboolean isdown = FALSE;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = TRUE;

	Master_Shutdown(TRUE);

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = TRUE;

	Host_WriteConfiguration();

	HPAK_FlushHostQueue();

	SV_ClearChannels(FALSE);

	NET_Shutdown();
	S_ShutdownDevice();
	IN_Shutdown();

	Con_Shutdown();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();

		UI_Shutdown();
	}
}

#ifdef _WIN32_WCE
#pragma optimize("", on)
#endif

// cl_main.c  -- client main loop

#include "quakedef.h"
#include "winquake.h"
#include "crc.h"
#include "clientid.h"
#include "pmove.h"
#include "decal.h"
#include "hashpak.h"
#include "cl_demo.h"
#include "cl_tent.h"
#include "cl_servercache.h"
#include "tmessage.h"
#include "won.h"

#pragma optimize("", off)
#pragma inline_depth(0)

// Only send this many requests before timing out.
#define CL_CONNECTION_RETRIES		4

// Current long-running task, for the crash screen.
static char g_szTaskName[64];
#if HLDC_MP
static int cl_serverload_progress = -1;
#endif

void Cmd_fullserverinfo_f( void );

void CL_HudMessage( const char* pMessage )
{
	DispatchDirectUserMsg("HudText", strlen(pMessage), (void*)pMessage);
}

void Sys_SetTaskName( char *name )
{
	CL_UpdateProgressBar();
	strncpy(g_szTaskName, name, 63);
	g_szTaskName[63] = 0;
}

cvar_t	password = { "password", "", FCVAR_USERINFO };
cvar_t	spectator = { "spectator", "", FCVAR_USERINFO };
cvar_t	name = { "name", "Player", FCVAR_USERINFO | FCVAR_PRINTABLEONLY };
cvar_t	team = { "team", "", FCVAR_USERINFO };
cvar_t	skin = { "skin", "", FCVAR_USERINFO };
cvar_t	model = { "model", "", FCVAR_USERINFO };
cvar_t	topcolor = { "topcolor", "0", FCVAR_USERINFO };
cvar_t	bottomcolor = { "bottomcolor", "0", FCVAR_USERINFO };

cvar_t	cl_timeout = { "cl_timeout", "305", TRUE };
cvar_t	cl_shownet = { "cl_shownet", "0" };	// can be 0, 1, or 2
cvar_t	cl_showsizes = { "cl_showsizes", "0" };
cvar_t	cl_nolerp = { "cl_nolerp", "0" };
cvar_t	cl_stats = { "cl_stats", "0" };
cvar_t	cl_spectator_password = { "cl_spectator_password", "0" };

cvar_t	lookspring = { "lookspring", "0", TRUE };
cvar_t	lookstrafe = { "lookstrafe", "0", TRUE };
cvar_t	sensitivity = { "sensitivity", "3", TRUE };
float gMouseSensitivity;

cvar_t	cl_skyname = { "cl_skyname", "desert", TRUE };
cvar_t	cl_skycolor_r = { "cl_skycolor_r", "0" };
cvar_t	cl_skycolor_g = { "cl_skycolor_g", "0" };
cvar_t	cl_skycolor_b = { "cl_skycolor_b", "0" };
cvar_t	cl_skyvec_x = { "cl_skyvec_x", "0" };
cvar_t	cl_skyvec_y = { "cl_skyvec_y", "0" };
cvar_t	cl_skyvec_z = { "cl_skyvec_z", "0" };

cvar_t	cl_predict_players = { "cl_predict_players", "1" };
cvar_t	cl_pred_link = { "cl_pred_link", "1" };
cvar_t	cl_pred_maxtime = { "cl_pred_maxtime", "255" };
cvar_t	cl_pred_fraction = { "cl_pred_fraction", "0.5" };
cvar_t	cl_solid_players = { "cl_solid_players", "1" };
cvar_t	cl_nodelta = { "cl_nodelta", "0" };
cvar_t	cl_printplayers = { "cl_printplayers", "0" };
cvar_t	cl_himodels = { "cl_himodels", "0" };
cvar_t	cl_gaitestimation = { "cl_gaitestimation", "1" };

cvar_t	m_pitch = { "m_pitch", "0.022", TRUE };
cvar_t	m_yaw = { "m_yaw", "0.022", TRUE };
cvar_t	m_forward = { "m_forward", "1", TRUE };
cvar_t	m_side = { "m_side", "0.8", TRUE };

cvar_t	cl_pitchup = { "cl_pitchup", "89" };
cvar_t	cl_pitchdown = { "cl_pitchdown", "89" };

cvar_t	rcon_password = { "rcon_password", "" };
cvar_t	rcon_address = { "rcon_address", "" };
cvar_t	rcon_port = { "rcon_port", "0" };

cvar_t	cl_resend = { "cl_resend", "6.0" };
cvar_t	cl_downloadinterval = { "cl_downloadinterval", "1.0" };

void CL_RecordDownloadStats (void)
{
	downloadtime_t *sample;

	if (!cl_downloadinterval.value)
		return;
	if (cl_downloadinterval.value < 0)
		Cvar_SetValue ("cl_downloadinterval", 1.0f);
	if (realtime - cls.fLastDownloadTime < cl_downloadinterval.value)
		return;

	cls.fLastDownloadTime = realtime;
	sample = &cls.rgDownloads[cls.downloadnumber & (MAX_DL_STATS - 1)];
	sample->bUsed = true;
	sample->fTime = realtime;
	sample->nBytesRemaining = cls.nRemainingToTransfer;
	cls.downloadnumber++;
}
cvar_t	cl_slisttimeout = { "cl_slist", "10.0" };
cvar_t	cl_allowdownload = { "cl_allowdownload", "0" };
#if HLDC_MP
cvar_t	cl_allowupload = { "cl_allowupload", "1", FCVAR_ARCHIVE };
#else
cvar_t	cl_allowupload = { "cl_allowupload", "0" };
#endif
cvar_t	cl_upload_max = { "cl_upload_max", "0" };
cvar_t	cl_download_max = { "cl_download_max", "0" };
cvar_t	cl_download_ingame = { "cl_download_ingame", "0" };

cvar_t 	rate = { "rate", "2500", FCVAR_USERINFO };

client_static_t	cls;
client_state_t cl;

static server_cache_t	cached_servers[MAX_LOCAL_SERVERS];
#if HLDC_MP
static int server_pings[MAX_LOCAL_SERVERS];
#endif

// FIXME: put these on hunk?
efrag_t			cl_efrags[MAX_EFRAGS];
cl_entity_t*	cl_entities = NULL;
cl_entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];
dlight_t		cl_elights[MAX_ELIGHTS];

// refresh list
// this is double buffered so the last frame
// can be scanned for oldorigins of trailing objects
int				cl_numvisedicts, cl_oldnumvisedicts, cl_numbeamentities;
cl_entity_t* cl_visedicts, * cl_oldvisedicts;
cl_entity_t	cl_visedicts_list[2][MAX_VISEDICTS];

qboolean cl_inmovie;

qboolean g_bSkipDownload = FALSE;
qboolean g_bSkipUpload = FALSE;

int playerbitcounts[32];  // # of bytes of player data for this slot

/*
=================
CL_UpdateSoundFade

Modulates sound volume on the client.
=================
*/
void CL_UpdateSoundFade( void )
{
	int		nTotal;
	float	fGap;
	float	f;
	// Determine current fade value.

	// Assume no fading remains
	cls.soundfade.nClientSoundFadePercent = 0;

	nTotal = cls.soundfade.soundFadeOutTime + cls.soundfade.soundFadeInTime + cls.soundfade.soundFadeHoldTime;

	fGap = realtime - cls.soundfade.soundFadeStartTime;

	// Clock wrapped or reset (BUG) or we've gone far enough
	if (fGap >= nTotal)
	{
		return;
	}

	// We are in the fade time, so determine amount of fade.
	if (cls.soundfade.soundFadeOutTime && (fGap < cls.soundfade.soundFadeOutTime))
	{
		// Ramp up
		f = fGap / cls.soundfade.soundFadeOutTime;
	}
	else if (fGap < (cls.soundfade.soundFadeOutTime + cls.soundfade.soundFadeHoldTime))
	{
		// Stay
		f = 1.0f;
	}
	else
	{
		// Ramp down
		f = (fGap - (cls.soundfade.soundFadeOutTime + cls.soundfade.soundFadeHoldTime)) / cls.soundfade.soundFadeOutTime;
		// backward interpolated...
		f = 1.0f - f;
	}

	cls.soundfade.nClientSoundFadePercent = cls.soundfade.nStartPercent * f;
}

/*
=================
CL_ParseMOTD

Parses MOTD from master server
=================
*/
void CL_ParseMOTD( void )
{
	char line[40];
	char* p, * string, * start;
	qboolean isMasterServer = FALSE;

	if (NET_StringToAdr(gszMasterAddress, &master_adr))
		isMasterServer = NET_CompareClassBAdr(net_from, master_adr) != FALSE;

	if (!isMasterServer)
		return;

	Con_Printf("---- MOTD -----\n");
	while (1)
	{
		string = MSG_ReadString();
		start = string;
		p = line;
		while (*string)
		{
			if (*string != '\r' || start >= string - 1 || *(string - 1) == '\n')
			{
				*p++ = *string++;
			}
			else
			{
				// convert to CRLF
				*p++ = '\n';
				*p++ = '\r';
				string++;
			}
		}
		*p = 0; // null-terminate the line

		if (line[0] == '\0')
			break; // stop if we got empty line

		Con_Printf(line);
	}
	Con_Printf("\n---------------\n");
}

/*
=================
CL_ParseServerInfoResponse

=================
*/
void CL_ParseServerData( qboolean detailed )
{
	char name[80];
	char map[16];
	char desc[256];
	char gamedir[256];
	char info[32];
	char info_url[256];
	char download_url[256];
	int active;
	int maxplayers;
	int version;
	int size;
	char type;
	char os;
	char password;
	short mod;
	short secure;
	short dll;

	MSG_ReadString(); // address string

	strncpy(name, MSG_ReadString(), sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;

	strncpy(map, MSG_ReadString(), sizeof(map) - 1);
	map[sizeof(map) - 1] = 0;

	strncpy(gamedir, MSG_ReadString(), sizeof(gamedir) - 1);
	gamedir[sizeof(gamedir) - 1] = 0;

	strncpy(desc, MSG_ReadString(), sizeof(desc) - 1);
	desc[sizeof(desc) - 1] = 0;

	active = MSG_ReadByte();
	maxplayers = MSG_ReadByte();

	MSG_ReadByte();

	if (!detailed)
	{
		type = '?';
		os = '?';
		password = FALSE;
		mod = FALSE;
		secure = FALSE;
		dll = FALSE;
		info[0] = 0;
		info_url[0] = 0;
		download_url[0] = 0;
		version = 0;
		size = 0;
	}
	else
	{
		type = (char)MSG_ReadByte();
		os = (char)MSG_ReadByte();
		password = (char)MSG_ReadByte();
		mod = MSG_ReadByte() ? TRUE : FALSE;

		if (mod)
		{
			strcpy(info_url, MSG_ReadString());
			strcpy(download_url, MSG_ReadString());
			strcpy(info, MSG_ReadString());
			version = MSG_ReadLong();
			size = MSG_ReadLong();
			secure = MSG_ReadByte() ? TRUE : FALSE;
			dll = MSG_ReadByte() ? TRUE : FALSE;
		}
		else
		{
			info[0] = 0;
			info_url[0] = 0;
			download_url[0] = 0;
			version = 0;
			size = 0;
			secure = FALSE;
			dll = FALSE;
		}
	}

	CL_AddToServerCache(net_from, name, map, desc, gamedir, active, maxplayers,
		type, os, password, mod, secure, dll, info_url, download_url, version, size, info);
}

/*
=================
CL_Slist_f (void)

Populates the client's server_cache_t structrue
Replaces Slist command
=================
*/
#if HLDC_MP
int CL_ServerListCount( void )
{
	return num_servers;
}

const server_cache_t* CL_ServerListEntry( int index )
{
	if (index < 0 || index >= num_servers)
		return NULL;
	return &cached_servers[index];
}

int CL_ServerListPing( int index )
{
	if (index < 0 || index >= num_servers)
		return 0;
	return server_pings[index];
}

qboolean CL_RefreshServerList( char* address )
{
	netadr_t adr;

	memset(&adr, 0, sizeof(adr));
	if (address[0])
	{
		if (!NET_StringToAdr(address, &adr) || adr.type != NA_IP)
			return FALSE;
		if (!adr.port)
			adr.port = BigShort((unsigned short)atoi(PORT_SERVER));
	}
	num_servers = 0;
	memset(cached_servers, 0, sizeof(cached_servers));
	memset(server_pings, 0, sizeof(server_pings));
	CL_PingServers_f();
	if (address[0])
		Netchan_OutOfBandPrint(NS_CLIENT, adr, "details");
	return TRUE;
}
#endif

void CL_Slist_f( void )
{
	int i;
	server_cache_t* p;

	num_servers = 0;
	memset(cached_servers, 0, MAX_LOCAL_SERVERS * sizeof(server_cache_t));

	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
	{
		p = &cached_servers[i];
		strcpy(p->name, "empty slot");
	}

	// send out info packets
	// FIXME:  Record realtime when sent and determine ping times for responsive servers.
	CL_PingServers_f();
}

/*
=================
CL_ClearCachedServers_f (void)

=================
*/
void CL_ClearCachedServers_f( void )
{
	int i;
	server_cache_t* p;

	num_servers = 0;
	memset(cached_servers, 0, MAX_LOCAL_SERVERS * sizeof(server_cache_t));

	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
	{
		p = &cached_servers[i];
		strcpy(p->name, "empty slot");
	}

	Con_Printf("Server list cleared.\n");
}

/*
=================
CL_PingServers_f

Broadcast pings to any servers that we can see on our LAN
=================
*/
void CL_PingServers_f( void )
{
	netadr_t	adr;

	// send a broadcast packet
	Con_Printf("Searching for local servers...\n");

	cls.slist_time = Sys_FloatTime();
	NET_Config(TRUE);

	if (!noip)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort((unsigned short)atoi(PORT_SERVER));
		if (PROTOCOL_VERSION < PROTOCOL_VERSION_CURRENT)
			Netchan_OutOfBandPrint(NS_CLIENT, adr, "info");
		else
			Netchan_OutOfBandPrint(NS_CLIENT, adr, "details");
	}

#ifdef _WIN32
	if (!noipx)
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort((unsigned short)atoi(PORT_SERVER));
		if (PROTOCOL_VERSION < PROTOCOL_VERSION_CURRENT)
			Netchan_OutOfBandPrint(NS_CLIENT, adr, "info");
		else
			Netchan_OutOfBandPrint(NS_CLIENT, adr, "details");
	}
#endif
}

/*
=================
CL_AddToServerCache

Adds the address, name to the server cache
=================
*/
void CL_AddToServerCache( netadr_t adr, char* name, char* map, char* desc, char* gamedir,
	int active, int maxplayers, char type, char os, char password, short mod,
	short secure, short dll, char* info_url, char* download_url, int version, int size, char* info )
{
	int i;
	float fCurrentTime;
	server_cache_t* p;

	// Too many servers.
	if (num_servers >= MAX_LOCAL_SERVERS)
	{
		return;
	}

	if (cl_slisttimeout.value > 0)
	{
		fCurrentTime = Sys_FloatTime();
		if ((fCurrentTime - cls.slist_time) >= cl_slisttimeout.value)
		{
			return;
		}
	}

	// Already cached
	for (i = 0; i < num_servers; i++)
	{
		p = &cached_servers[i];
		if (NET_CompareAdr(p->adr, adr))
			return;
	}

	// Display it.
	Con_Printf("------------------\n");
	Con_Printf("%i %s %s %i/%i\nAdr:  %s - Dir:  %s\n%s\n", num_servers + 1, name, map,
		active, maxplayers, NET_AdrToString(adr), gamedir, desc);

	cached_servers[num_servers].adr = adr;
#if HLDC_MP
	server_pings[num_servers] = (int)((Sys_FloatTime() - cls.slist_time) * 1000.0f);
#endif

	strncpy(cached_servers[num_servers].name, name, sizeof(cached_servers[num_servers].name) - 1);
	cached_servers[num_servers].name[sizeof(cached_servers[num_servers].name) - 1] = 0;

	strncpy(cached_servers[num_servers].map, map, sizeof(cached_servers[num_servers].map));
	cached_servers[num_servers].map[sizeof(cached_servers[num_servers].map) - 1] = 0;

	strncpy(cached_servers[num_servers].desc, desc, sizeof(cached_servers[num_servers].desc) - 1);
	cached_servers[num_servers].desc[sizeof(cached_servers[num_servers].desc) - 1] = 0;
	strncpy(cached_servers[num_servers].gamedir, gamedir, sizeof(cached_servers[num_servers].gamedir) - 1);
	cached_servers[num_servers].gamedir[sizeof(cached_servers[num_servers].gamedir) - 1] = 0;

	cached_servers[num_servers].inuse = active;
	cached_servers[num_servers].maxplayers = maxplayers;
	cached_servers[num_servers].type = type;
	cached_servers[num_servers].os = os;
	cached_servers[num_servers].password = password;
	cached_servers[num_servers].mod = mod;
	cached_servers[num_servers].secure = secure;
	cached_servers[num_servers].dll = dll;
	if (type == 'd')
		Con_Printf("  dedicated - ");
	else if (type == 'l')
		Con_Printf("  listen - ");
	else
		Con_Printf("  type (%c) - ", type);

	if (os == 'w')
		Con_Printf("win32 - ");
	else if (os == 'l')
		Con_Printf("linux - ");
	else
		Con_Printf("os (%c) - ", os);

	if (password == TRUE)
		Con_Printf(" password ");
	else if (password == FALSE)
		Con_Printf(" nopasswd ");
	else
		Con_Printf("pw (%c) - ", password);

	if (secure)
		Con_Printf(" +serverside");
	if (dll)
		Con_Printf(" +client.dll");
	Con_Printf("\n");
	strcpy(cached_servers[num_servers].info_url, info_url);
	strcpy(cached_servers[num_servers].download_url, download_url);
	cached_servers[num_servers].version = version;
	cached_servers[num_servers].size = size;
	strncpy(cached_servers[num_servers].info, info, sizeof(cached_servers[num_servers].info) - 1);
	cached_servers[num_servers].info[sizeof(cached_servers[num_servers].info) - 1] = 0;
	num_servers++;
}

/*
===================
CL_ListCachedServers_f()

===================
*/
void CL_ListCachedServers_f( void )
{
	int i;
	server_cache_t* p;

	if (num_servers == 0)
	{
		Con_Printf("No local servers in list.\nTry 'slist' to search again.\n");
		return;
	}

	for (i = 0; i < num_servers; i++)
	{
		p = &cached_servers[i];
		if (!_stricmp(p->name, "empty slot"))
			continue;

		Con_Printf("------------------\n");
		Con_Printf("%i %s %s %i/%i\nAdr:  %s - Dir:  %s\n%s\n", i + 1, p->name, p->map,
			p->inuse, p->maxplayers, NET_AdrToString(p->adr), p->gamedir, p->desc);
		if (p->type == 'd')
			Con_Printf("  dedicated - ");
		else if (p->type == 'l')
			Con_Printf("  listen - ");
		else
			Con_Printf("  type (%c) - ", p->type);
		if (p->os == 'w')
			Con_Printf("win32 - ");
		else if (p->os == 'l')
			Con_Printf("linux - ");
		else
			Con_Printf("os (%c) - ", p->os);
		if (p->password == TRUE)
			Con_Printf(" password ");
		else if (p->password == FALSE)
			Con_Printf(" nopasswd ");
		else
			Con_Printf("pw (%c) - ", p->password);
		Con_Printf("\n");
		if (p->mod)
			Con_Printf("Mod info:\nInfo URL %s\nDL URL %s\nVer. %i, size %.2f MB\n",
				p->info_url, p->download_url, p->version, (float)p->size / 1048576.0f);
	}
}

void CL_ParseServerList( void )
{
	char address[128];
	byte ip[4];
	int i;
	int j;
	int count;
	int port;

	MSG_ReadByte();
	count = (net_message.cursize - 6) / 6;
	for (i = 0; i < count; i++)
	{
		memset(address, 0, sizeof(address));
		for (j = 0; j < 4; j++)
			ip[j] = MSG_ReadByte();
		sprintf(address, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
		port = BigShort(MSG_ReadShort());
		if (i + 1 <= 100)
			Con_Printf("%4i:  %s:%i\n", i + 1, address, port);
	}
	Con_Printf("%i total servers\n", count + 1);
}

void CL_ParseMasterServerList( void )
{
	char address[128];
	byte ip[4];
	byte request[5];
	int i;
	int j;
	int count;
	int port;
	int sequence;

	MSG_ReadByte();
	sequence = MSG_ReadLong();
	count = (net_message.cursize - 6) / 6;
	for (i = 0; i < count; i++)
	{
		memset(address, 0, sizeof(address));
		for (j = 0; j < 4; j++)
			ip[j] = MSG_ReadByte();
		sprintf(address, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
		port = BigShort(MSG_ReadShort());
	}
	Con_Printf("%i servers\n", count + 1);

	if (!sequence)
	{
		Con_Printf("Done.\n");
		return;
	}

	NET_Config(TRUE);
	request[0] = 'e';
	*(int*)&request[1] = sequence;
	NET_SendPacket(NS_CLIENT, sizeof(request), request, net_from);
}

void CL_ServerListInfo( void )
{
	char modname[128];
	char request[260];
	char* name;
	int players;
	int servers;
	int count = 0;

	MSG_ReadByte();
	modname[0] = 0;
	while (1)
	{
		name = MSG_ReadString();
		if (!name || !name[0] || !Q_strcasecmp(name, "end-of-list") || !Q_strcasecmp(name, "more-in-list"))
			break;
		strcpy(modname, name);
		players = atoi(MSG_ReadString());
		servers = atoi(MSG_ReadString());
		count++;
		Con_Printf("%3i %5i %s\n", players, servers, modname);
	}
	Con_Printf("%i servers\n", count);

	if (name && name[0] && !Q_strcasecmp(name, "more-in-list"))
	{
		sprintf(request, "%c%s", 'x', modname);
		NET_SendPacket(NS_CLIENT, strlen(request) + 1, request, net_from);
	}
	else
		Con_Printf("Done.\n");
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( void )
{
	int		c, i;
	char*	s;
	byte	data[6];

	MSG_BeginReading();
	MSG_ReadLong();        // skip the -1 marker

	c = MSG_ReadByte();
	if (c == S2C_CONNECTION)
	{
		for (i = 0; i < 16; i++)
			MSG_ReadByte();

		WON_RemoveUser(&cl_authrequest);

		// Already connected?
		if (cls.state != ca_connected)
		{
			// Initiate the network channel
			Netchan_Setup(NS_CLIENT, &cls.netchan, net_from);

			// Signon process will commence now that server ok'd connection.
			MSG_WriteChar(&cls.netchan.message, clc_stringcmd);
			MSG_WriteString(&cls.netchan.message, "new");

			// Report connection success.
			if (Q_strcasecmp("loopback", NET_AdrToString(net_from)))
				Con_Printf("Connection accepted by %s\n", NET_AdrToString(net_from));

			// Mark client as connected
			cls.state = ca_connected;

			// Not in the demo loop now
			cls.demonum = -1;
			// Need all the signon messages before playing ( or drawing first frame )
			cls.signon = 0;

			// Bump connection time to now so we don't resend a connection
			// Request
			cls.connect_time = realtime;
#if HLDC_MP
			CL_BeginServerLoad();
#endif
		}
	}
	else if (c == S2C_CHALLENGE)
	{
		if (cls.state != ca_disconnected)
		{
			for (i = 0; i < 16; i++)
				MSG_ReadByte();

			cls.challenge = BigLong(MSG_ReadLong());
			cls.authprotocol = MSG_ReadByte();
			if (cls.authprotocol == 0xFF)
				cls.authprotocol = PROTOCOL_HASHEDCDKEY;

			if (cls.authprotocol == PROTOCOL_AUTHCERTIFICATE)
			{
				COM_CheckAuthenticationType();
				if (!gfUseLANAuthentication)
					WON_RequestCertificate();
				else
				{
					Con_Printf("The server requires that you be validated through WON.net.\n"
						"Could not obtain WON authentication.\n");
					CL_Disconnect_f();
				}
			}
			else
				CL_SendConnectPacket();
		}
	}
	else
	{
		if (WON_IsValidAuthMessage(c))
			CL_ParseAuthenticationMessage(c);
		else if (c == A2C_PRINT)
		{
			Con_Printf(MSG_ReadString());
		}
		else if (c == S2C_BADPASSWORD)
	{
		if (cls.state == ca_connecting)
		{
			s = MSG_ReadString();
			if (!Q_strncasecmp(s, "BADPASSWORD", strlen("BADPASSWORD")))
				s += strlen("BADPASSWORD");

			Con_Printf(s);
			COM_ExplainDisconnection(FALSE, "BADPASSWORD");
			Con_Printf("Invalid server password.\n");
			CL_Disconnect();
			WON_RemoveUser(&cl_authrequest);
		}
	}
	else if (c == S2C_CONNREJECT)
	{
		if (cls.state == ca_connecting)
		{
			s = MSG_ReadString();
			COM_ExplainDisconnection(TRUE, s);
			CL_Disconnect();
			WON_RemoveUser(&cl_authrequest);
		}
	}
	else if (c == A2A_PING)
	{
		data[0] = 0xFF;
		data[1] = 0xFF;
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = A2A_ACK;
		data[5] = 0;
		NET_SendPacket(NS_CLIENT, sizeof(data), data, net_from);
	}
	else if (c == A2A_ACK)
	{
		if (cls.slist_time != 0.0f)
		{
			Con_Printf("Ping took %.3f ms.\n", (realtime - cls.slist_time) * 1000.0f);
			cls.slist_time = 0.0f;
		}
	}
	else if (c == S2A_INFO)
	{
		CL_ParseServerData(FALSE);
	}
	else if (c == S2A_INFO_DETAILED)
	{
		CL_ParseServerData(TRUE);
	}
	else if (c == M2A_MOTD)
	{
		CL_ParseMOTD();
	}
	else if (c == M2A_SERVERS)
	{
		CL_ParseServerList();
	}
	else if (c == 'f')
	{
		CL_ParseMasterServerList();
	}
	else if (c == M2A_ACTIVEMODS)
	{
		CL_ServerListInfo();
	}
	else
	{
		Con_Printf("Unknown command:\n%c\n", c);
	}
	}
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
qboolean CL_GetMessage( void )
{
	if (!NET_GetPacket(NS_CLIENT))
		return FALSE;

	return TRUE;
}

/*
=================
CL_ReadPackets

Updates the local time and reads/handles messages on client net connection.
=================
*/
void CL_ReadPackets( void )
{
	cl.oldtime = cl.time;
	cl.time += host_frametime;

	while (CL_GetMessage())
	{
		if (*(int*)net_message.data == -1)
		{
			CL_ConnectionlessPacket();
			continue;
		}

		if (cls.state == ca_disconnected ||
			cls.state == ca_connecting)
			continue;

		if (net_message.cursize < 8)
		{
			Con_Printf("%s: Runt packet\n", NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server, verify source IP address.
		//
		if (!NET_CompareAdr(net_from, cls.netchan.remote_address))
		{
			Con_Printf("%s:sequenced packet without connection\n", NET_AdrToString(net_from));
			continue;
		}

		if (Netchan_Process(&cls.netchan))
		{
			// Parse out the commands.
			CL_ParseServerMessage();
			continue;
		}
	}

	// check timeout, but not if running _DEBUG engine
#if !defined( _DEBUG )
	// Only check on final frame because that's when the server might send us a packet in single player.  This avoids
	//  a bug where if you sit in the game code in the debugger then you get a timeout here on resuming the engine
	//  because the timestep is > 1 tick because of the debugging delay but the server hasn't sent the next packet yet.
	if ((cls.state >= ca_connected) &&
		((realtime - cls.netchan.last_received) > cl_timeout.value))
	{
		Con_Printf("Server connection timed out.\n");
		CL_Disconnect();
		return;
	}
#endif

	if (cl_shownet.value)
		Con_Printf("\n");
}

/*
================
CL_PrintCustomizations_f

================
*/
void CL_PrintCustomizations_f( void )
{
	int	i, j;
	customization_t* pCust;

	if (cls.state != ca_active)
	{
		Con_Printf("Can't cl_print_custom, not connected\n");
		return;
	}

	for (i = 0; i < 1; i++)
	{
		pCust = cl.players[i].customdata.pNext;
		if (pCust)
		{
			j = 1;
			while (pCust)
			{
				if (pCust->bInUse)
				{
					CL_PrintResource(j, &pCust->resource);
					j++;
				}
				pCust = pCust->pNext;
			}
		}
	}
}

void CL_ClearClientState( void )
{
	int i;
	packet_entities_t* pClientPack;

	if (cl.frames)
	{
		for (i = 0; i < cl_update_backup; i++)
		{
			pClientPack = &cl.frames[i].packet_entities;
			if (pClientPack->entities)
				free(pClientPack->entities);

			pClientPack->entities = NULL;
			pClientPack->num_entities = 0;
		}
	}

	CL_ClearResourceLists();
	if (cl.frames)
		free(cl.frames);
	cl.frames = NULL;

	Q_memset(&cl, 0, (int)((byte*)&cl.frames - (byte*)&cl));

	cl.resourcesneeded.pPrev = &cl.resourcesneeded;
	cl.resourcesneeded.pNext = &cl.resourcesneeded;
	cl.resourcesonhand.pPrev = &cl.resourcesonhand;
	cl.resourcesonhand.pNext = &cl.resourcesonhand;

	cl.frames = (frame_t*)MnemoAllocDbg(sizeof(frame_t) * cl_update_backup, __FILE__, __LINE__);
	if (!cl.frames)
		Sys_Error("Unable to allocate %i client frames", cl_update_backup);
	memset(cl.frames, 0, sizeof(frame_t) * cl_update_backup);
	CL_CreateResourceList();
}

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState( qboolean bQuiet )
{
	int			i;

	if (!sv.active)
		Host_ClearMemory(bQuiet);

	CL_ClearClientState();

	SZ_Clear(&cls.netchan.message);

// clear other arrays
	memset(cl_efrags, 0, sizeof(cl_efrags));
	memset(cl_dlights, 0, sizeof(cl_dlights));
	memset(cl_elights, 0, sizeof(cl_elights));
	memset(cl_lightstyle, 0, sizeof(cl_lightstyle));

	CL_TempEntInit();

//
// allocate the efrags and chain together into a free list
//	
	cl.free_efrags = cl_efrags;
	for (i = 0; i < MAX_EFRAGS - 1; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i + 1];

	cl.free_efrags[i].entnext = NULL;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( void )
{
#if HLDC_MP
	if (CL_IsServerLoading())
	{
		SCR_EndLoadingPlaque();
		CL_StopProgressBar();
	}
#endif
	cls.connect_time = -99999.0;
	cls.connect_retry = 0;

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);

	// if running a local server, shut it down
	if (cls.state == ca_active
		|| cls.state == ca_connected
		|| cls.state == ca_uninitialized
		|| cls.state == ca_connecting)
	{
		if (cls.netchan.remote_address.type != NA_UNUSED)
		{
			byte	final[20];

			// Send a drop command.
			final[0] = clc_stringcmd;
			strcpy((char*)(final + 1), "dropclient\n");
			Netchan_Transmit(&cls.netchan, 13, final);
			Netchan_Transmit(&cls.netchan, 13, final);
			Netchan_Transmit(&cls.netchan, 13, final);
		}

		cls.state = ca_disconnected;

		if (sv.active)
			Host_ShutdownServer(FALSE);
	}

	cls.signon = 0;

	if (cls.download)
	{
		fclose(cls.download);
		cls.download = NULL;
	}

	if (cls.upload)
	{
		COM_FreeFile(cls.upload);
		cls.upload = NULL;
	}

	CL_ClearState(TRUE);

	CL_DeallocateDynamicData();

	Cam_Reset();
}

/*
=====================
CL_Disconnect_f

Disconnects user from the server
=====================
*/
void CL_Disconnect_f( void )
{
	CL_Disconnect();
	if (sv.active)
		Host_ShutdownServer(FALSE);
}

// HL1 CD Key
#define GUID_LEN 13

/*
=================
CL_GetCDKeyHash

Connections will now use a hashed cd key value
A LAN server will know not to allows more then xxx users with the same CD Key
=================
*/
char* CL_GetCDKeyHash( void )
{
	char szKeyBuffer[36] = "2534835307254"; // Keys are about 13 chars long.
	static char szHashedKeyBuffer[36];
	int nKeyLength = GUID_LEN;
	qboolean bDedicated = FALSE;
	MD5Context_t ctx;
	unsigned char digest[16]; // The MD5 Hash
#if 0
	// Get the cd key.
	Launcher_GetCDKey(szKeyBuffer, &nKeyLength, &bDedicated);
#endif
	if (bDedicated)
	{
		Con_Printf("Key has no meaning on dedicated server...\n");
		return "";
	}

	if (nKeyLength < 1 || nKeyLength > 35)
	{
		Con_Printf("Bogus key length on CD Key...\n");
		return "";
	}

	szKeyBuffer[nKeyLength] = 0;

	// Now get the md5 hash of the key
	memset(&ctx, 0, sizeof(ctx));
	memset(digest, 0, sizeof(digest));

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)szKeyBuffer, nKeyLength);
	MD5Final(digest, &ctx);
	memset(szHashedKeyBuffer, 0, sizeof(szHashedKeyBuffer));
	strcpy(szHashedKeyBuffer, MD5_Print(digest));
	return szHashedKeyBuffer;
}

/*
=================
CL_SendConnectPacket

called by CL_Connect and CL_CheckResend
If we are in ca_connecting state and we have gotten a challenge
  response before the timeout, send another "connect" request.
=================
*/
void CL_SendConnectPacket( void )
{
	netadr_t	adr;
	char		data[2048];
	char		szServerName[MAX_OSPATH];
	char		szRawCertificate[1024];
	char		szModelCRC[32];
	CRC32_t	modelCRC;
	int		nUserID = -1;
	int		nCDKeyLength = 0;

	strncpy(szServerName, cls.servername, sizeof(szServerName));

	// Deal with local connection
	if (!_stricmp(cls.servername, "local"))
	{
		sprintf(szServerName, "%s", "localhost");
	}

	if (!NET_StringToAdr(szServerName, &adr))
	{
		Con_Printf("Bad server address\n");
		cls.connect_time = -99999.0;
		cls.connect_retry = 0;
		cls.state = ca_disconnected;
		return;
	}

	if (adr.port == (unsigned short)0)
	{
		adr.port = BigShort((unsigned short)atoi(PORT_SERVER));
	}

	if (cls.authprotocol != PROTOCOL_AUTHCERTIFICATE)
	{
		if (cls.authprotocol != PROTOCOL_HASHEDCDKEY)
			cls.authprotocol = PROTOCOL_HASHEDCDKEY;

		nCDKeyLength = strlen(CL_GetCDKeyHash());
		strcpy(szRawCertificate, CL_GetCDKeyHash());
	}

	CRC32_Init(&modelCRC);
	if (!CRC_File(&modelCRC, "models/player/gordon/gordon.mdl"))
	{
		CL_Disconnect_f();
		Con_Printf("could not find models/player/gordon/gordon.mdl\n");
		return;
	}

	sprintf(szModelCRC, "%d", modelCRC);
	Info_SetValueForStarKey(cls.userinfo, "*modelcrc", szModelCRC, MAX_INFO_STRING);

	sprintf(data, "%c%c%c%cconnect %i %i %i %i %i \"%s\" \"%s\"\n",
		255, 255, 255, 255, PROTOCOL_VERSION, cls.challenge,
		cls.authprotocol, nUserID, nCDKeyLength, szRawCertificate, cls.userinfo);

	NET_SendPacket(NS_CLIENT, strlen(data), data, adr);
}


/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend( void )
{
	netadr_t	adr;
	char	data[2048];
	char szServerName[128];

	if (cls.state == ca_disconnected && sv.active)
	{
		cls.state = ca_connecting;
		strncpy(cls.servername, "localhost", sizeof(cls.servername) - 1);
		CL_SendConnectPacket();
		return;
	}

	// resend if we haven't gotten a reply yet
	// We only resend during the connection process.
	if (cls.state != ca_connecting)
		return;

	if (cl_resend.value < 1.5f)
		Cvar_SetValue("cl_resend", 1.5f);
	else if (cl_resend.value > 20.0f)
		Cvar_SetValue("cl_resend", 20.0f);

	// Wait at least the resend # of seconds.
	if ((realtime - cls.connect_time) < cl_resend.value)
		return;

	strncpy(szServerName, cls.servername, sizeof(szServerName));

	// Deal with local connection.
	if (!_stricmp(cls.servername, "local"))
		sprintf(szServerName, "%s", "localhost");

	if (!NET_StringToAdr(szServerName, &adr))
	{
		Con_Printf("Bad server address\n");
		cls.state = ca_disconnected;
		return;
	}

#if HLDC_MP
	if (!adr.port && adr.type == NA_IP)
		adr.port = BigShort((unsigned short)atoi(PORT_SERVER));
#endif

	// Only retry so many times before failure.
	if (cls.connect_retry >= CL_CONNECTION_RETRIES)
	{
		Con_Printf("Connection failed after %i retries.\n", cls.connect_retry);
		cls.connect_time = -99999.0;
		cls.connect_retry = 0;
		cls.state = ca_disconnected;
		return;
	}

	// Mark time of this attempt.
	cls.connect_time = realtime;	// for retransmit requests

	// Display appropriate message
	if (_stricmp(szServerName, "localhost"))
	{
		if (cls.connect_retry == 0)
			Con_Printf("Connecting to %s...\n", szServerName);			
		else
			Con_Printf("Retrying %s...\n", szServerName);
	}

	cls.connect_retry++;

	// Request another challenge value.
	sprintf(data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);

	// Send the request.
	NET_SendPacket(NS_CLIENT, strlen(data), data, adr);
}

void CL_Retry_f( void )
{
	char command[256];

	if (!cls.trueaddress || !cls.trueaddress[0])
	{
		Con_Printf("Can't retry, no previous connection\n");
		return;
	}

	sprintf(command, "connect %s\n", cls.trueaddress);
	Cbuf_AddText(command);
	Con_Printf("Commencing connection retry to %s\n", cls.trueaddress);
}

/*
=====================
CL_Connect_f

User command to connect to server
=====================
*/
void CL_Connect_f( void )
{
	char* server;
	char name[MAX_OSPATH];
	int i, num;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("usage: connect <server>\n");
		return;
	}

#if HLDC_MP
	server = Cmd_Args();
	if (server && server[0] == '"')
		server = Cmd_Argv(1);
#else
	server = Cmd_Args();
#endif
	if (!server)
		return;

	strcpy(cls.trueaddress, server);

	// Disconnect from current server
	// Don't call Host_Disconnect, because we don't want to shutdown the listen server!
	CL_Disconnect();

	// Get new server name
	memset(name + 4, 0, sizeof(name) - 4);
	strncpy(name, server, sizeof(name));

	num = atoi(server);  // In case it's an index.

	// Check the cached_servers list for a text match or index match:
	if (!strstr(server, ".") && (num > 0) && (num <= num_servers))
	{
		strncpy(name, NET_AdrToString(cached_servers[num - 1].adr), sizeof(name));
	}
	else
	{
		// Try to find server in cache
		for (i = 0; i < num_servers; i++)
		{
		if (!Q_strncasecmp(server, cached_servers[i].name, strlen(cached_servers[i].name)))
			{
				strncpy(name, NET_AdrToString(cached_servers[i].adr), sizeof(name));
				break;
			}
		}
	}

	memset(msg_buckets, 0, sizeof(msg_buckets));
	memset(total_data, 0, sizeof(total_data));

	strncpy(cls.servername, name, sizeof(cls.servername) - 1);

	// For the check for resend timer to fire a connection / getchallenge request.
	cls.state = ca_connecting;
	// Force connection request to fire.
	cls.connect_time = -99999.0f;

	cls.connect_retry = 0;
	gfExtendedError = FALSE;

	if (Q_strncasecmp(cls.servername, "local", 5))
		NET_Config(TRUE);
}

/*
=====================
CL_Spectate_f

User command to connect to server as spectator
=====================
*/
void CL_Spectate_f( void )
{
	char* server;
	char name[128], * p;
	int i, num;

	cls.spectator = TRUE;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("usage: spectate <server> [server password]\n");
		return;
	}

	server = Cmd_Args();
	if (!server)
		return;

	// Disconnect from current server
	// Don't call Host_Disconnect, because we don't want to shutdown the listen server!
	CL_Disconnect();

	// Get new server name
	memset(name, 0, sizeof(name));
	strncpy(name, server, sizeof(name));

	p = name;
	while (*p && *p != ' ')
		p++;

	if (p[0] && p[1])
		strcpy(cls.trueaddress, p + 1);
	else
		strcpy(cls.trueaddress, "0");

	num = atoi(server);

	// Check the cached_servers list for a text match or index match:
	if (!strstr(server, ".") && (num > 0) && (num <= num_servers))
	{
		strncpy(name, NET_AdrToString(cached_servers[num - 1].adr), sizeof(name));
	}
	else
	{
		// Try to find server in cache
		for (i = 0; i < num_servers; i++)
		{
			if (!_strnicmp(server, cached_servers[i].name, strlen(cached_servers[i].name)))
			{
				strncpy(name, NET_AdrToString(cached_servers[i].adr), sizeof(name));
				break;
			}
		}
	}

	memset(msg_buckets, 0, sizeof(msg_buckets));
	memset(total_data, 0, sizeof(total_data));

	strncpy(cls.servername, name, sizeof(cls.servername) - 1);

	// For the check for resend timer to fire a connection / getchallenge request.
	cls.state = ca_connecting;
	// Force connection request to fire.
	cls.connect_time = -99999;

	cls.connect_retry = 0;
}

/*
=================
CL_SignonReply

A svc_signonnum has been received, perform a client side setup
=================
*/
void CL_SignonReply( void )
{
	char 	str[8192];

#if HLDC_MP
	CL_SetServerLoadProgress(cls.signon == 1 ? 95 : cls.signon == 2 ? 98 : 100);
#endif
	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		sprintf(str, "spawn %i %s", cl.servercount, cls.spawnparms);
		MSG_WriteString(&cls.netchan.message, str);
		Sys_SetTaskName("CL_Signon 1");
		break;

	case 2:
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, "begin");
		Cache_Report();
		Sys_SetTaskName("CL_Signon 2");
		break;

	case 3:
		SCR_EndLoadingPlaque();		// allow normal screen updates
		Sys_SetTaskName("CL_Signon 3");
		CL_StopProgressBar();
		break;
	}
}

/*
==================
CL_NextDemo

Called to play the next demo in the demo loop
==================
*/
void CL_NextDemo( void )
{
	char    str[1024];

	if (cls.demonum == -1)
		return; // don't play demos

	SCR_BeginLoadingPlaque();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			scr_disabled_for_loading = FALSE;

			Con_Printf("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}	
	}

	sprintf(str, "playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText(str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f( void )
{
	cl_entity_t* ent;
	int		i;

	for (i = 0; i < cl.num_entities; i++)
	{
		Con_Printf("%3i:", i);

		ent = &cl_entities[i];
		if (!ent->model)
		{
			Con_Printf("EMPTY\n");
			continue;
		}

		Con_Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
			ent->model,
			ent->frame,
			ent->origin[0], ent->origin[1], ent->origin[2],
			ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}

/*
===============
CL_TakeSnapshot_f

Generates a filename and calls the vidwin.c function to write a .bmp file
===============
*/
static int cl_snapshotnum;
void CL_TakeSnapshot_f( void )
{
	char	base[MAX_QPATH];
	char	filename[MAX_QPATH];

	if (cl.num_entities && cl_entities->model)
		COM_FileBase(cl_entities->model->name, base);
	else
		strcpy(base, "Snapshot");

	sprintf(filename, "%s%04d.bmp", base, cl_snapshotnum);
	cl_snapshotnum++;

	VID_TakeSnapshot(filename);
}

/*
===============
CL_StartMovie_f

Sets the engine up to dump frames
===============
*/
void CL_StartMovie_f( void )
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("startmovie <filename>\n");
		return;
	}

	cl_inmovie = TRUE;
	VID_WriteBuffer(Cmd_Argv(1));
	Con_Printf("Started recording movie...\n");
}


/*
===============
CL_EndMovie_f

Ends frame dumping
===============
*/

void CL_EndMovie_f( void )
{
	if (!cl_inmovie)
	{
		Con_Printf("No movie started.\n");
	}
	else
	{
		cl_inmovie = FALSE;
		Con_Printf("Stopped recording movie...\n");
	}
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f( void )
{
	char	message[1024];
	char	command[256];
	int		i;
	netadr_t	to;
	unsigned short nPort;

	if (!rcon_password.string)
	{
		Con_Printf("You must set 'rcon_password' before\nissuing an rcon command.\n");
		return;
	}

	NET_Config(TRUE);

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_Printf("You must either be connected,\n"
					   "or set the 'rcon_address' cvar\n"
					   "to issue rcon commands\n");

			return;
		}
		if (!NET_StringToAdr(rcon_address.string, &to))
		{
			Con_Printf("Unable to resolve rcon address %s\n", rcon_address.string);
			return;
		}
	}

	nPort = (unsigned short)rcon_port.value;  // ?? BigShort
	if (!nPort)
		nPort = atoi(PORT_SERVER); //DEFAULTnet_hostport;

	to.port = BigShort(nPort);

	sprintf(message, "rcon \"%s\" ", rcon_password.string);

	for (i = 1; i < Cmd_Argc(); i++)
	{
		sprintf(command, "\"%s\"", Cmd_Argv(i));
		strcat(message, command);
		if (i != Cmd_Argc() - 1)
			strcat(message, " ");
	}

	Netchan_OutOfBandPrint(NS_CLIENT, to, "%s", message);
}

/*
==============
CL_View_f

Debugging changes the view entity to the specified index
===============
*/
void CL_View_f( void )
{
	int i;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("cl_view entity#\nCurrent %i\n", cl.viewentity);
		return;
	}

	i = atoi(Cmd_Argv(1));
	if (i == 0 || i >= cl.num_entities)
		return;

	cl.viewentity = i;
	vid.recalc_refdef = TRUE;
	Con_Printf("View entity set to %i\n", i);
}

/*
===============
SetPal

Debugging tool, just flashes the screen
===============
*/
void SetPal( int i )
{
#if 0
	static int old;
	byte	pal[768];
	int		c;

	if (i == old)
		return;
	old = i;

	if (i == 0)
		VID_SetPalette(host_basepal);
	else if (i == 1)
	{
		for (c = 0; c < 768; c += 3)
		{
			pal[c] = 0;
			pal[c + 1] = 255;
			pal[c + 2] = 0;
		}
		VID_SetPalette(pal);
	}
	else
	{
		for (c = 0; c < 768; c += 3)
		{
			pal[c] = 0;
			pal[c + 1] = 0;
			pal[c + 2] = 255;
		}
		VID_SetPalette(pal);
	}
#endif
}

/*
=================
CL_AllocDlight

=================
*/
dlight_t* CL_AllocDlight( int key )
{
	int	i;
	dlight_t* dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				r_dlightchanged |= (1 << i);
				r_dlightactive |= (1 << i);
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			r_dlightchanged |= (1 << i);
			r_dlightactive |= (1 << i);
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	r_dlightchanged |= (1 << 0);
	r_dlightactive |= (1 << 0);
	return dl;
}

/*
=================
CL_AllocElight

=================
*/
dlight_t* CL_AllocElight( int key )
{
	int	i;
	dlight_t* el;

// first look for an exact key match
	if (key)
	{
		el = cl_elights;
		for (i = 0; i < MAX_ELIGHTS; i++, el++)
		{
			if (el->key == key)
			{
				memset(el, 0, sizeof(*el));
				el->key = key;
				return el;
			}
		}
	}

// then look for anything else
	el = cl_elights;
	for (i = 0; i < MAX_ELIGHTS; i++, el++)
	{
		if (el->die < cl.time)
		{
			memset(el, 0, sizeof(*el));
			el->key = key;
			return el;
		}
	}

	el = &cl_elights[0];
	memset(el, 0, sizeof(*el));
	el->key = key;
	return el;
}

/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights( void )
{
	int			i;
	dlight_t* dl;
	float		time;

	r_dlightchanged = 0;
	r_dlightactive = 0;

	time = cl.time - cl.oldtime;

	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->radius != 0)
		{
			if (dl->die < cl.time)
			{
				r_dlightchanged |= (1 << i);
				dl->radius = 0;
			}
			else if (dl->decay)
			{
				r_dlightchanged |= (1 << i);

				dl->radius -= time * dl->decay;
				if (dl->radius < 0)
					dl->radius = 0;
			}
			if (dl->radius != 0)
			{
				r_dlightactive |= (1 << i);
			}
		}
	}

	dl = cl_elights;
	for (i = 0; i < MAX_ELIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
		{
			dl->radius = 0;
			continue;
		}

		dl->radius -= time * dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}

/*
===============
CL_TouchLight

===============
*/
void CL_TouchLight( dlight_t* dl )
{
	int i;

	i = dl - cl_dlights;
	if (i >= 0 && i < 32)
		r_dlightchanged |= 1 << i;
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float CL_LerpPoint( void )
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cl_nolerp.value || sv.active && !fakelag.value)
	{
		cl.time = cl.mtime[0];
		return 1.0f;
	}

	if (f > 0.1f)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1f;
		f = 0.1f;
	}
	frac = (cl.time - cl.mtime[1]) / f;
//Con_Printf("frac: %f\n", frac);
	if (frac < 0.0f)
	{
		if (frac < -0.01f)
		{
			SetPal(1);
			cl.time = cl.mtime[1];
//				Con_Printf("low frac\n");
		}
		frac = 0.0f;
	}
	else if (frac > 1.0f)
	{
		if (frac > 1.01f)
		{
			SetPal(2);
			cl.time = cl.mtime[0];
//				Con_Printf("high frac\n");
		}
		frac = 1.0f;
	}
	else
		SetPal(0);

	return frac;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd( void )
{
	sizebuf_t	buf;
	byte		data[128];
	int			i;
	usercmd_t* cmd, * oldcmd, nullcmd;
	int			checksumIndex;
	int			seq_hash;

	if (cls.state != ca_dedicated &&
		cls.state != ca_disconnected &&
		cls.state != ca_connecting &&
		cl.frames)
	{

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & cl_update_mask;
	cmd = &cl.frames[i].cmd;
	cl.frames[i].senttime = realtime;
	cl.frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	memset(cmd, 0, sizeof(*cmd));

//	seq_hash = (cls.netchan.outgoing_sequence & 0xffff) ; // ^ QW_CHECK_HASH;
	seq_hash = cls.netchan.outgoing_sequence;

	// get basic movement from keyboard
	if (cls.signon == SIGNONS)
	{
		// get basic movement from keyboard
		CL_BaseMove(cmd);

		// allow mice or other external controllers to add to the move
		IN_Move(cmd);
	}

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	buf.maxsize = 128;
	buf.cursize = 0;
	buf.data = data;

	MSG_WriteByte(&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte(&buf, 0);

	for (i = 0; i < 3; i++)
		cmd->angles[i] = cl.viewangles[i];

	i = (int)(host_frametime * 1000.0f);
	if (i > 250)
		i = 100;
	cmd->msec = i;

	cmd->buttons = CL_ButtonBits(1);
	if (in_klook.state)
	{
		if (cmd->forwardmove > 0.0f)
			cmd->buttons |= IN_FORWARD;
		else if (cmd->forwardmove < 0.0f)
			cmd->buttons |= IN_BACK;
	}
	cmd->impulse = in_impulse;

	// if we are spectator, try autocam
	if (cl.spectator)
	{
		Cam_Track(cmd);
		Cam_FinishMove(cmd);
	}

	memset(&nullcmd, 0, sizeof(nullcmd));

	i = (cls.netchan.outgoing_sequence - 2) & cl_update_mask;
	cmd = &cl.frames[i].cmd;
	MSG_WriteUsercmdByProtocol(&buf, cmd, &nullcmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence - 1) & cl_update_mask;
	cmd = &cl.frames[i].cmd;
	MSG_WriteUsercmdByProtocol(&buf, cmd, oldcmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & cl_update_mask;
	cmd = &cl.frames[i].cmd;
	MSG_WriteUsercmdByProtocol(&buf, cmd, oldcmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		seq_hash);

	memcpy(&cl.cmd, cmd, sizeof(cl.cmd));

	in_impulse = 0;

	// request delta compression of entities
	if (cls.netchan.outgoing_sequence - cl.validsequence >= cl_update_backup - 1)
		cl.validsequence = 0;

	if (cl.validsequence && !cl_nodelta.value && cls.state == ca_active)
	{
		cl.frames[cls.netchan.outgoing_sequence & cl_update_mask].delta_sequence = cl.validsequence;
		MSG_WriteByte(&buf, clc_delta);
		MSG_WriteByte(&buf, cl.validsequence & 255);
	}
	else
		cl.frames[cls.netchan.outgoing_sequence & cl_update_mask].delta_sequence = -1;


//
// deliver the message
//
	Netchan_Transmit(&cls.netchan, buf.cursize, buf.data);
	}
}

/*
=================
CL_ParseNextUpload

Sends next file chunk to server
=================
*/
void CL_ParseNextUpload( void )
{
	int		r;
	int		percent;
	int		size;

	if (!cls.upload)
		return;

	if (g_bSkipUpload)
	{
		g_bSkipUpload = FALSE;
		COM_FreeFile(cls.upload);
		cls.upload = NULL;
		Con_Printf("Skipping upload...\n");
		return;
	}

	r = cls.uploadsize - cls.uploadpos;
	if (r > 1024)
		r = 1024;
	CRC32_ProcessBuffer(&cls.uploadCRC, (byte*)cls.upload + cls.uploadpos, r);

	MSG_WriteByte(&cls.netchan.message, clc_upload);
	MSG_WriteShort(&cls.netchan.message, r);
	MSG_WriteShort(&cls.netchan.message, cls.uploadpos / 1024);
	MSG_WriteLong(&cls.netchan.message, cls.uploadCRC);

	cls.uploadpos += r;
	size = cls.uploadsize;
	if (!size)
		size = 1;
	percent = cls.uploadpos * 100 / size;
	MSG_WriteByte(&cls.netchan.message, percent);
	SZ_Write(&cls.netchan.message, (byte*)cls.upload + cls.uploadpos - r, r);

	if (cls.uploadpos != cls.uploadsize)
		return;

	COM_FreeFile(cls.upload);
	cls.upload = NULL;
}

/*
==================
CL_SetupResume

==================
*/
void CL_SetupResume( int size, CRC32_t crc )
{
	CRC32_t crcFile;

	if (size < 0)
		return;

	size *= 1024;
	if (cls.uploadsize < size)
		return;

	CRC32_Init(&crcFile);
	CRC32_ProcessBuffer(&crcFile, cls.upload, size);
	crcFile = CRC32_Final(crcFile);
	if (crcFile == crc)
	{
		cls.uploadpos = size;
		cls.uploadCRC = crc;
		cls.uploading = TRUE;
	}
}

/*
=================
CL_BeginUpload_f

Starts file upload to server, handles both normal files and MD5-hashed resources
=================
*/
#if HLDC_MP
static byte* cl_spraydata;
static int cl_spraysize;
static byte cl_sprayhash[16];

static byte* CL_BuildSprayWad( byte* data, int length, int* size )
{
	miptex_t source, *mip;
	wadinfo_t* header;
	lumpinfo_t* lump;
	byte* wad;
	byte* palette;
	byte* pixels;
	int w, h, x, y, level, offset, count, paletteOffset;
	unsigned short colors;

	if (length < sizeof(source))
		return NULL;
	memcpy(&source, data, sizeof(source));
	if (source.width < 16 || source.height < 16 || source.width > 256 || source.height > 256 ||
		(source.width & 15) || (source.height & 15))
		return NULL;
	count = source.width * source.height;
	for (level = 0; level < 4; level++)
	{
		if (source.offsets[level] < sizeof(source) || source.offsets[level] > length ||
			(count >> (level * 2)) > length - source.offsets[level])
			return NULL;
	}
	paletteOffset = source.offsets[3] + (count >> 6);
	if (paletteOffset > length - 770)
		return NULL;
	memcpy(&colors, data + paletteOffset, 2);
	if (colors != 256)
		return NULL;
	palette = data + paletteOffset + 2;
	w = source.width;
	h = source.height;
	while (w > 64 || h > 64)
	{
		w = max(16, w / 2);
		h = max(16, h / 2);
	}
	w = (w + 15) & ~15;
	h = (h + 15) & ~15;
	count = w * h;
	offset = sizeof(miptex_t) + count + (count >> 2) + (count >> 4) + (count >> 6) + 770;
	offset = (offset + 3) & ~3;
	*size = sizeof(wadinfo_t) + offset + sizeof(lumpinfo_t);
	wad = (byte*)malloc(*size);
	if (!wad)
		return NULL;
	memset(wad, 0, *size);
	header = (wadinfo_t*)wad;
	memcpy(header->identification, "WAD3", 4);
	header->numlumps = 1;
	header->infotableofs = sizeof(wadinfo_t) + offset;
	mip = (miptex_t*)(wad + sizeof(wadinfo_t));
	strcpy(mip->name, "{logo");
	mip->width = w;
	mip->height = h;
	offset = sizeof(miptex_t);
	for (level = 0; level < 4; level++)
	{
		mip->offsets[level] = offset;
		pixels = (byte*)mip + offset;
		for (y = 0; y < (h >> level); y++)
			for (x = 0; x < (w >> level); x++)
				*pixels++ = data[source.offsets[0] + (y * source.height / (h >> level)) * source.width +
					x * source.width / (w >> level)];
		offset += count >> (level * 2);
	}
	memcpy((byte*)mip + offset, &colors, 2);
	memcpy((byte*)mip + offset + 2, palette, 768);
	lump = (lumpinfo_t*)(wad + header->infotableofs);
	lump->filepos = sizeof(wadinfo_t);
	lump->disksize = lump->size = header->infotableofs - sizeof(wadinfo_t);
	lump->type = 67;
	strcpy(lump->name, "{logo");
	return wad;
}

static byte* CL_ReadSpray( const char* name, int* spraySize )
{
	FileList_t *list, *file;
	wadinfo_t header;
	lumpinfo_t lump;
	byte *raw, *wad;
	char clean[16];
	int i, size, length, candidateSize;

	wad = NULL;
	size = 0;
	list = NULL;
	if (!name[0])
		return NULL;
	COM_BuildFileList("decals.wad", &list);
	for (file = list; file; file = file->next)
	{
		COM_FileSeek(file->handles[0], file->handles[1], file->handles[2], 0);
		if (Sys_FileRead(file->handles[2], &header, sizeof(header)) != sizeof(header) ||
			memcmp(header.identification, "WAD3", 4) || header.numlumps < 0 ||
			header.infotableofs < sizeof(header) || header.infotableofs > file->fileLen ||
			header.numlumps > (file->fileLen - header.infotableofs) / sizeof(lump))
			continue;
		for (i = 0; i < header.numlumps; i++)
		{
			COM_FileSeek(file->handles[0], file->handles[1], file->handles[2], header.infotableofs + i * sizeof(lump));
			if (Sys_FileRead(file->handles[2], &lump, sizeof(lump)) != sizeof(lump))
				break;
			memcpy(clean, lump.name, sizeof(clean));
			clean[15] = 0;
			if (Q_strcasecmp(clean, name) || lump.compression || lump.filepos < sizeof(header) ||
				lump.filepos > file->fileLen || lump.disksize < sizeof(miptex_t) ||
				lump.disksize > 100000 || lump.disksize > file->fileLen - lump.filepos)
				continue;
			raw = (byte*)malloc(lump.disksize);
			if (!raw)
				break;
			COM_FileSeek(file->handles[0], file->handles[1], file->handles[2], lump.filepos);
			length = Sys_FileRead(file->handles[2], raw, lump.disksize);
			if (length == lump.disksize)
			{
				byte* candidate = CL_BuildSprayWad(raw, length, &candidateSize);
				if (candidate)
				{
					free(wad);
					wad = candidate;
					size = candidateSize;
				}
			}
			free(raw);
			break;
		}
	}
	COM_CloseUnusedFiles(list);
	COM_DestroyMultipleFileList(&list);
	*spraySize = size;
	return wad;
}

qboolean CL_CanUploadSpray( const char* name )
{
	byte* data;
	int size;

	data = CL_ReadSpray(name, &size);
	if (!data)
		return FALSE;
	free(data);
	return TRUE;
}

static void CL_LoadSpray( const char* name )
{
	MD5Context_t context;

	free(cl_spraydata);
	cl_spraydata = NULL;
	cl_spraysize = 0;
	cl_spraydata = CL_ReadSpray(name, &cl_spraysize);
	if (cl_spraydata)
	{
		MD5Init(&context);
		MD5Update(&context, cl_spraydata, cl_spraysize);
		MD5Final(cl_sprayhash, &context);
	}
}

const byte* CL_GetSprayData( byte* hash, int size )
{
	if (size != cl_spraysize || memcmp(hash, cl_sprayhash, sizeof(cl_sprayhash)))
		return NULL;
	return cl_spraydata;
}
#endif

void CL_BeginUpload_f( void )
{
#if HLDC_MP
	char* name;
	int error;

	if (cls.state == ca_disconnected || cls.state == ca_dedicated || Cmd_Argc() < 2)
		return;
	name = Cmd_Argv(1);
	error = -1;
	if (cl_allowupload.value && cl_spraydata && strlen(name) == 36 &&
		!Q_strncasecmp(name, "!MD5", 4) && !Q_strcasecmp(name + 4, COM_BinPrintf(cl_sprayhash, 16)))
	{
		if (!cl_upload_max.value || cl_spraysize <= cl_upload_max.value)
		{
			if (cls.upload)
				COM_FreeFile(cls.upload);
			cls.upload = (FILE*)malloc(cl_spraysize);
			if (cls.upload)
			{
				memcpy(cls.upload, cl_spraydata, cl_spraysize);
				cls.uploadsize = cl_spraysize;
				cls.uploadpos = 0;
				cls.uploading = FALSE;
				g_bSkipUpload = FALSE;
				CRC32_Init(&cls.uploadCRC);
				if (Cmd_Argc() == 4)
					CL_SetupResume(atoi(Cmd_Argv(2)), atol(Cmd_Argv(3)));
				CL_ParseNextUpload();
				return;
			}
		}
		error = -2;
	}
	MSG_WriteByte(&cls.netchan.message, clc_upload);
	MSG_WriteShort(&cls.netchan.message, -1);
	MSG_WriteShort(&cls.netchan.message, -1);
	MSG_WriteLong(&cls.netchan.message, error);
	MSG_WriteByte(&cls.netchan.message, 0);
#endif
}

/*
=================
CL_SendResourceListBlock

=================
*/
void CL_SendResourceListBlock( void )
{
	int		i;
	int		arg, size;
    unsigned short u;

	if (cls.state == ca_dedicated || cls.state == ca_disconnected)
	{
		Con_Printf("custom resource list not valid -- not connected\n");
		return;
	}

	arg = MSG_ReadLong();
	if (arg != cl.servercount)
	{
		Con_Printf("CL_SendResourceListBlock_f from different level\n");
		return;
	}

	i = MSG_ReadLong();
	if (i < 0 || i > cl.num_resources)
	{
		Con_Printf("custom resource list request out of range\n");
		return;
	}

	// Send resource list
	MSG_WriteByte(&cls.netchan.message, clc_resourcelist);
	MSG_WriteShort(&cls.netchan.message, cl.num_resources);
	MSG_WriteShort(&cls.netchan.message, i);
	size = cls.netchan.message.cursize;

	MSG_WriteShort(&cls.netchan.message, 0);

	for (; i < cl.num_resources; i++)
	{
		if (cls.netchan.message.cursize + 128 >= cls.netchan.message.maxsize)
			break;

		MSG_WriteByte(&cls.netchan.message, cl.resourcelist[i].type);
		MSG_WriteString(&cls.netchan.message, cl.resourcelist[i].szFileName);
		MSG_WriteShort(&cls.netchan.message, cl.resourcelist[i].nIndex);
#if HLDC_MP
		MSG_WriteLong(&cls.netchan.message, cl.resourcelist[i].ucFlags & RES_CUSTOM ? cl_spraysize : 1000);
#else
		MSG_WriteLong(&cls.netchan.message, 1000);
#endif
		MSG_WriteByte(&cls.netchan.message, cl.resourcelist[i].ucFlags);
#if HLDC_MP
		if (cl.resourcelist[i].ucFlags & RES_CUSTOM)
			SZ_Write(&cls.netchan.message, cl_sprayhash, sizeof(cl_sprayhash));
#endif
	}

    u = (unsigned short)i;
	memcpy(&cls.netchan.message.data[size], &u, sizeof(u));

	if (i < cl.num_resources)
	{
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, va("customrsrclist %i %i\n", cl.servercount, i));
	}
}

/*
====================
CL_AddResource

Adds a new resource to the client resource list
====================
*/
resource_t* CL_AddResource( resourcetype_t type, char* name, int size, qboolean bFatalIfMissing, int index )
{
	resource_t* r;

	r = &cl.resourcelist[cl.num_resources];
	if (cl.num_resources >= MAX_RESOURCES)
		Sys_Error("Too many resources on client.");
	cl.num_resources++;

	r->type = type;
	strcpy(r->szFileName, name);
	r->nIndex = index;

	if (bFatalIfMissing)
	{
		r->ucFlags |= RES_FATALIFMISSING;
	}

	return r;
}

/*
====================
CL_CreateResourceList

====================
*/
void CL_CreateResourceList( void )
{
#if HLDC_MP
	resource_t* resource;

	cl.num_resources = 0;
	CL_LoadSpray(sv.active ? "" : Cvar_VariableString("mp_spray"));
	if (!cl_spraydata)
		return;
	resource = &cl.resourcelist[0];
	memset(resource, 0, sizeof(*resource));
	strcpy(resource->szFileName, "pldecal.wad");
	resource->type = t_decal;
	resource->ucFlags = RES_CUSTOM;
	cl.num_resources = 1;
#else
	FILE* fp;
	int			nSize;
	char		szFileName[128];
	unsigned char	rgucMD5_hash[16];

	resource_t* pNewResource;

	cl.num_resources = 0;

	sprintf(szFileName, "pldecal.wad");

	memset(rgucMD5_hash, 0, sizeof(rgucMD5_hash));

	nSize = COM_FOpenFile(szFileName, &fp);
	if (nSize == -1)
	{
		nSize = 0;
	}
	else
	{
		MD5_Hash_File(rgucMD5_hash, szFileName);
	}

	if (nSize)
	{
		pNewResource = CL_AddResource(t_decal, szFileName, nSize, FALSE, 0);
		if (pNewResource)
		{
			HPAK_AddLump(HASHPAK_FILENAME, pNewResource, NULL, fp);
		}
	}

	if (fp)
		Sys_CloseHandle(fp);
#endif
}

/*
==================
CL_SkipDownload_f

Skip current download
==================
*/
void CL_SkipDownload_f( void )
{
	if (!cls.download)
		return;

	g_bSkipDownload = TRUE;
}

/*
==================
CL_SkipUpload_f

Skip current upload
==================
*/
void CL_SkipUpload_f( void )
{
	if (!cls.upload)
		return;

	g_bSkipUpload = TRUE;
}

/*
==================
CL_AllowDownload_f

==================
*/
void CL_AllowDownload_f( void )
{
	cl_allowdownload.value = !cl_allowdownload.value;

	if (!cl_allowdownload.value)
		Con_Printf("Client downloading disabled.\n");
	else
		Con_Printf("Client downloading enabled.\n");
}

/*
==================
CL_AllowUpload_f

==================
*/
void CL_AllowUpload_f( void )
{
	cl_allowupload.value = !cl_allowupload.value;

	if (!cl_allowupload.value)
		Con_Printf("Client uploading disabled.\n");
	else
		Con_Printf("Client uploading enabled.\nMax. upload size is %i", (unsigned int)(__int64)cl_upload_max.value);
}

cvar_t	cl_adaptive = { "cl_adaptive", "0" };

typedef struct adaptive_sample_s
{
	short	valid;
	short	pad;
	float	time;
	float	loss;
	float	latency;
	int		updates;
	int		lost;
	float	load;
	int		bytes;
	float	rate;
	float	requested_rate;
	float	fps;
} adaptive_sample_t;

#define ADAPTIVE_SAMPLES 16
#define ADAPTIVE_MASK (ADAPTIVE_SAMPLES - 1)

static adaptive_sample_t adaptive_samples[ADAPTIVE_SAMPLES];
static int adaptive_sample_index;
static float adaptive_lastupdate;

extern cvar_t fps_lan;
extern cvar_t fps_modem;

void R_DrawAdaptiveGraph( void )
{
	int i;
	int valid = 0;
	int x = vid.width;
	int y = vid.height - 10;
	float average = 0.0f;
	float height;
	adaptive_sample_t* sample;

	for (i = 0; i < ADAPTIVE_SAMPLES; i++)
	{
		sample = &adaptive_samples[(adaptive_sample_index - i) & ADAPTIVE_MASK];
		if (sample->valid)
		{
			height = sample->load * 100.0f / 1024.0f;
			average += height;
			valid++;
			Draw_FillRGBA(x - 6 - i, y - (int)(height * 2.0f), 1, (int)(height * 2.0f), 100, 100, 200, 128);
		}
	}

	if (valid)
	{
		average /= (float)valid;
		Draw_FillRGBA(x - 22, y - (int)(average * 2.0f), 16, 1, 255, 255, 255, 255);
	}
}

void R_PrintNetStats( void )
{
	adaptive_sample_t* sample = &adaptive_samples[adaptive_sample_index];

	if (!sample->valid)
		return;

	Con_Printf("--------------------------\n");
	Con_Printf("Time %.3f\n", sample->time);
	Con_Printf("Drop %.2f%%\n", sample->loss * 100.0f);
	Con_Printf("Avg. Latency %.2f\n", sample->latency * 1000.0f);
	Con_Printf("Load K/s %.2f\n", sample->load / 1024.0f);
	Con_Printf("Upd/s %.2f\n", sample->rate);
	Con_Printf("Rate %.2f\n", sample->requested_rate);
	Con_Printf("cl fps %.1f\n", sample->fps);
}

/*
==================
R_DrawAdaptive

Draw the adaptive frame-timing bars in the corner of the screen.
==================
*/
void R_DrawAdaptive( void )
{
	vrect_t rect;
	byte color[3];
	int i;

	if (!cl_adaptive.value)
		return;

	color[0] = 0;
	color[1] = 0;
	color[2] = 0;
	rect.x = vid.width - 6;
	rect.width = 5;
	rect.height = 1;

	for (i = 0; i < 10; i++)
	{
		rect.y = vid.height - 10 - i * 20;
		if (rect.y > 10)
			D_FillRect(&rect, color);
	}

	R_DrawAdaptiveGraph();
}

void R_UpdateAdaptive( void )
{
	int i;
	int samples = 0;
	qboolean got_sample = FALSE;
	frame_t* frame;
	adaptive_sample_t* sample;

	if (!cl_adaptive.value)
		return;
	if (sv.active && svs.maxclients <= 1)
		return;
	if (cl_update_backup > cls.netchan.incoming_sequence)
		return;
	if (realtime - adaptive_lastupdate < 1.0f)
		return;

	adaptive_lastupdate = realtime;
	adaptive_sample_index = (adaptive_sample_index + 1) & ADAPTIVE_MASK;
	sample = &adaptive_samples[adaptive_sample_index];
	memset(sample, 0, sizeof(*sample));
	sample->requested_rate = rate.value;
	sample->fps = rate.value < 2000.0f ? fps_modem.value : fps_lan.value;

	for (i = 0; i < cl_update_backup; i++)
	{
		frame = &cl.frames[(cls.netchan.incoming_sequence - i) & cl_update_mask];
		samples++;
		if (!got_sample && frame->receivedtime >= 0.0f)
		{
			got_sample = TRUE;
			sample->time = frame->receivedtime;
		}
		else if (frame->receivedtime >= 0.0f)
		{
			sample->time = sample->time - frame->receivedtime;
		}

		if (frame->receivedtime < 0.0f)
			sample->lost++;
		else
		{
			sample->updates++;
			sample->bytes += (unsigned short)frame->packet_entities.num_entities;
			sample->latency += frame->receivedtime - frame->senttime;
		}
	}

	if (sample->updates)
		sample->latency /= (float)sample->updates;
	else
		sample->latency = 0.0f;

	sample->loss = (float)sample->lost / (float)samples;
	if (sample->time > 0.0f)
	{
		sample->load = (float)sample->bytes / sample->time;
		sample->rate = (float)samples / sample->time;
	}
	else
	{
		sample->load = 0.0f;
		sample->rate = 0.0f;
	}
	sample->valid = TRUE;
	R_PrintNetStats();
}

char* CL_HashedClientID( unsigned char* hash, int size )
{
	static char szReturn[128];
	unsigned char c;
	char szChunk[10];
	int i;

	memset(szReturn, 0, sizeof(szReturn));

	for (i = 0; i < size; i++)
	{
		c = (unsigned char)hash[i];
		sprintf(szChunk, "%2x", c);
		strcat(szReturn, szChunk);
	}

	return szReturn;
}

/*
=================
CL_PrintCDKey_f

Print the CD key to the console
=================
*/
void CL_PrintCDKey_f( void )
{
	char szKeyBuffer[256]; // Keys are about 13 chars long.	
	char szHashedKeyBuffer[256];
	char szHashedClientID[2048];
	int nKeyLength = GUID_LEN;
	int bDedicated = 0;
	MD5Context_t ctx;
	clientid_t clientid;
	unsigned char digest[16]; // The MD5 Hash

	// A dedicated server
	if (bDedicated)
	{
		Con_Printf("Key has no meaning on dedicated server...\n");
		return;
	}

	if (nKeyLength <= 0 ||
		nKeyLength >= 256)
	{
		Con_Printf("Bogus key length on CD Key...\n");
		return;
	}

	szKeyBuffer[nKeyLength] = 0;

	// Now get the md5 hash of the key
	memset(&ctx, 0, sizeof(ctx));
	memset(digest, 0, sizeof(digest));

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)szKeyBuffer, nKeyLength);
	MD5Final(digest, &ctx);
	memset(szHashedKeyBuffer, 0, sizeof(szHashedKeyBuffer));
	strcpy(szHashedKeyBuffer, MD5_Print(digest));

	memset(szHashedClientID, 0, sizeof(szHashedClientID));

	{
		strcpy(szHashedClientID, "Unset");
	}

	Con_Printf("CD Key:  %s\nMD5 Hash:  %s\nClient ID:  %s\n\n", szKeyBuffer, szHashedKeyBuffer, szHashedClientID);
}

/*
=================
CL_Init
=================
*/
void CL_Init( void )
{
	Info_SetValueForKey(cls.userinfo, "name", "unnamed", MAX_INFO_STRING);
	Info_SetValueForKey(cls.userinfo, "topcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey(cls.userinfo, "bottomcolor", "0", MAX_INFO_STRING);
	Info_SetValueForKey(cls.userinfo, "rate", "2500", MAX_INFO_STRING);

	CL_InitInput();
	CL_InitTEnts();

	TextMessageInit();

	ClientDLL_Init();
	ClientDLL_HudInit();

//
// register our commands
//
	Cvar_RegisterVariable(&name);
	Cvar_RegisterVariable(&password);
	Cvar_RegisterVariable(&spectator);
	Cvar_RegisterVariable(&team);
	Cvar_RegisterVariable(&model);
	Cvar_RegisterVariable(&skin);
	Cvar_RegisterVariable(&topcolor);
	Cvar_RegisterVariable(&bottomcolor);
	Cvar_RegisterVariable(&rate);
	Cvar_RegisterVariable(&cl_himodels);
	Cvar_RegisterVariable(&cl_gaitestimation);
	Cvar_RegisterVariable(&cl_upspeed);
	Cvar_RegisterVariable(&cl_forwardspeed);
	Cvar_RegisterVariable(&cl_backspeed);
	Cvar_RegisterVariable(&cl_sidespeed);
	Cvar_RegisterVariable(&cl_movespeedkey);
	Cvar_RegisterVariable(&cl_yawspeed);
	Cvar_RegisterVariable(&cl_pitchspeed);
	Cvar_RegisterVariable(&cl_anglespeedkey);
	Cvar_RegisterVariable(&cl_nolerp);
	Cvar_RegisterVariable(&cl_skyname);
	Cvar_RegisterVariable(&cl_skycolor_r);
	Cvar_RegisterVariable(&cl_skycolor_g);
	Cvar_RegisterVariable(&cl_skycolor_b);
	Cvar_RegisterVariable(&cl_skyvec_x);
	Cvar_RegisterVariable(&cl_skyvec_y);
	Cvar_RegisterVariable(&cl_skyvec_z);
	Cvar_RegisterVariable(&lookspring);
	Cvar_RegisterVariable(&lookstrafe);
	Cvar_RegisterVariable(&sensitivity);

	Cvar_RegisterVariable(&m_pitch);
	Cvar_RegisterVariable(&m_yaw);
	Cvar_RegisterVariable(&m_forward);
	Cvar_RegisterVariable(&m_side);

	Cvar_RegisterVariable(&cl_pitchup);
	Cvar_RegisterVariable(&cl_pitchdown);
	Cvar_RegisterVariable(&cl_resend);
	Cvar_RegisterVariable(&cl_timeout);
	Cvar_RegisterVariable(&cl_shownet);
	Cvar_RegisterVariable(&cl_showsizes);
	Cvar_RegisterVariable(&rcon_address);
	Cvar_RegisterVariable(&rcon_port);
	Cvar_RegisterVariable(&cl_spectator_password);
	Cvar_RegisterVariable(&cl_pred_link);
	Cvar_RegisterVariable(&cl_predict_players);
	Cvar_RegisterVariable(&cl_solid_players);
	Cvar_RegisterVariable(&cl_nodelta);
	Cvar_RegisterVariable(&cl_slisttimeout);
	Cvar_RegisterVariable(&cl_downloadinterval);
	Cvar_RegisterVariable(&cl_upload_max);
	Cvar_RegisterVariable(&cl_download_max);
	Cvar_RegisterVariable(&cl_download_ingame);
	Cvar_RegisterVariable(&cl_allowdownload);
	Cvar_RegisterVariable(&cl_allowupload);
	Cvar_RegisterVariable(&cl_pred_maxtime);
	Cvar_RegisterVariable(&cl_pred_fraction);
	Cvar_RegisterVariable(&cl_adaptive);

	Cmd_AddCommand("fullserverinfo", Cmd_fullserverinfo_f);
	Cmd_AddCommand("retry", CL_Retry_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("snapshot", CL_TakeSnapshot_f);
	Cmd_AddCommand("startmovie", CL_StartMovie_f);
	Cmd_AddCommand("endmovie", CL_EndMovie_f);
	Cmd_AddCommand("entities", CL_PrintEntities_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("cl_view", CL_View_f);
	Cmd_AddCommand("cl_messages", CL_DumpMessageLoad_f);
	Cmd_AddCommand("cl_bitcounts", CL_BitCounts_f);
	Cmd_AddCommand("cl_usr", CL_UserMsgs_f);
	Cmd_AddCommand("pingservers", CL_PingServers_f);
	Cmd_AddCommand("slist", CL_Slist_f);
	Cmd_AddCommand("list", CL_ListCachedServers_f);
	Cmd_AddCommand("clearlist", CL_ClearCachedServers_f);
	Cmd_AddCommand("resources", CL_PrintResourceLists_f);
	Cmd_AddCommand("cl_allow_upload", CL_AllowUpload_f);
	Cmd_AddCommand("cl_allow_download", CL_AllowDownload_f);
	Cmd_AddCommand("upload", CL_BeginUpload_f);
	Cmd_AddCommand("allowupload", CL_AllowUpload_f);
	Cmd_AddCommand("skipdl", CL_SkipDownload_f);
	Cmd_AddCommand("skipul", CL_SkipUpload_f);
	Cmd_AddCommand("cl_print_custom", CL_PrintCustomizations_f);
	
	CL_InitPrediction();
	CL_InitCam();

	Pmove_Init();

	memset(bitcounts, 0, sizeof(bitcounts));
	memset(playerbitcounts, 0, sizeof(playerbitcounts));
	memset(custombitcounts, 0, sizeof(custombitcounts));

	memset(&cl, 0, sizeof(client_state_t));

	cl.resourcesneeded.pNext = cl.resourcesneeded.pPrev = &cl.resourcesneeded;
	cl.resourcesonhand.pNext = cl.resourcesonhand.pPrev = &cl.resourcesonhand;

	cl.frames = (frame_t*)MnemoAllocDbg(sizeof(frame_t) * cl_update_backup, __FILE__, __LINE__);
	if (!cl.frames)
		Sys_Error("Unable to allocate %i client frames", cl_update_backup);
	memset(cl.frames, 0, sizeof(frame_t) * cl_update_backup);
}
// Realtime the current load began, so the bar can pace itself
float	cl_progress_start;

// Elapsed time (from cl_progress_start) the bar was last redrawn at
static float	cl_progress_lastupdate;

#if HLDC_MP
qboolean CL_IsServerLoading( void )
{
	return cl_serverload_progress >= 0;
}

void CL_BeginServerLoad( void )
{
	if (cls.netchan.remote_address.type != NA_IP || sv.active ||
		CL_IsServerLoading())
		return;

	cl_serverload_progress = 0;
	cl_progress_lastupdate = 0;
	CL_StartProgressBar();
	SCR_BeginLoadingPlaque();
	CL_SetServerLoadProgress(1);
}

void CL_SetServerLoadProgress( int percent )
{
	if (!CL_IsServerLoading())
		return;
	percent = min(100, max(0, percent));
	if (percent <= cl_serverload_progress)
		return;
	cl_serverload_progress = percent;
	DCV_SetProgress(percent);
}
#endif

/*
=================
CL_StartProgressBar

Reset the loading bar and note when the wait began.
=================
*/
void CL_StartProgressBar( void )
{
	cl_progress_start = Sys_FloatTime();

	DCV_SetProgress(0);
	Sys_SetTaskName("start");
}

/*
=================
CL_StopProgressBar

Loading is over; put the bar away.
=================
*/
void CL_StopProgressBar( void )
{
	Sys_SetTaskName("end");
	cl_progress_start = 0;
#if HLDC_MP
	cl_serverload_progress = -1;
#endif
	DCV_SetProgress(0);
	Host_UpdateScreenSaver(FALSE);
}

/*
=================
CL_UpdateProgressBar

Redraws the loading bar a few times a second while a long load is in
progress, and pumps the ambient sound and input state so neither one goes
stale during the wait.
=================
*/
#pragma optimize("", off)
qboolean CL_UpdateProgressBar( void )
{
	float	now;
	float	elapsed;
	float	sinceupdate;

	if (cl_progress_start == 0)
		return FALSE;

	now = Sys_FloatTime();
	elapsed = now - cl_progress_start;
	sinceupdate = elapsed - cl_progress_lastupdate;

	if (sinceupdate > 0.1f)
	{
		S_ExtraUpdate();
#if HLDC_MP
		if (CL_IsServerLoading())
			DCV_SetProgress(cl_serverload_progress);
		else
#endif
		DCV_SetProgress((int)(elapsed * 5.0f));
		IN_Accumulate();
	}
	cl_progress_lastupdate = elapsed;

	if (sinceupdate <= 0.7f || sinceupdate >= 5.0f)
		return FALSE;

	return TRUE;
}
#pragma optimize("", on)

/*
=================
CL_PollProgressBar

Called from long-running load steps (studio model textures, etc.) so the
loading bar keeps moving even while nothing else is servicing the frame.
=================
*/
void CL_PollProgressBar( void )
{
	CL_UpdateProgressBar();
}

/*
=================
CL_PrintLogoList

Dump the spray logos the client has seen this session.
=================
*/
void CL_PrintLogoList( void )
{
	int i;
	char text[1024];
	customization_t* customization;

	Con_Printf("Client Rep. of Player Logos ===\n");
	for (i = 0; i < cl.maxclients; i++)
	{
		customization = cl.players[i].customdata.pNext;

		if (&cl.players[i] == &cl.players[cl.playernum])
			Con_Printf("SELF =====================\n");

		while (customization)
		{
			Con_Printf(text);
			customization = customization->pNext;
		}

		if (&cl.players[i] == &cl.players[cl.playernum])
			Con_Printf("SELF =====================\n");
	}
	Con_Printf("==========================\n");
}

void Cmd_fullserverinfo_f( void )
{
	if (Cmd_Argc() == 2)
	{
		strcpy(cl.serverinfo, Cmd_Argv(1));
	}
	else
	{
		Con_Printf("usage: fullserverinfo <complete info string>\n");
	}
}

#include "quakedef.h"
#include "winquake.h"
#include "pr_cmds.h"
#include "cl_demo.h"
#include "decal.h"
#include "hashpak.h"
#include "r_studio.h"
#include "pr_edict.h"
#include "kzap.h"

int	current_skill;
int	gHostSpawnCount = 0;

// Slack at the end of a save block. The token list is walked one step past its
// last terminator and the data buffer is rounded up to four bytes, so the block
// needs a little more room than the sizes stored in the file account for.
#define SAVE_HEAPSLACK	32

// Savegame file I/O goes through the GD-ROM aware handle helpers rather than
// stdio, so a save can be read straight out of a pack or the memory card.
void*			Sys_OpenHandle( const char* path, const char* mode );
int				Sys_CloseHandle( void* hFile );
unsigned int	DC_fread( void* buffer, unsigned int size, unsigned int count, void* hFile );
unsigned int	DC_fwrite( void* buffer, unsigned int size, unsigned int count, void* hFile );
int				DC_fseek( void* hFile, int offset, int whence );
int				DC_ftell( void* hFile );
unsigned long	DC_fsize( void* hFile );

extern	cvar_t*	sv_allow_download;
extern	cvar_t*	sv_allow_upload;

// Memory card save storage
extern int	gSaveGameSize;
extern char	vmuSaveComment[64];
extern char	vmuSaveTitle[16];
extern char* VMU_MarkSlotSaved( void );

extern int	g_Language;

typedef struct
{
	char*	pBSPName;
	char*	pTitleName;
} TITLECOMMENT;

TITLECOMMENT gTitleComments[] =
{
	{ "T0A0", "T0A0TITLE" },
	{ "C0A0", "C0A0TITLE" },
	{ "C1A0", "C0A1TITLE" },
	{ "C1A1", "C1A1TITLE" },
	{ "C1A2", "C1A2TITLE" },
	{ "C1A3", "C1A3TITLE" },
	{ "C1A4", "C1A4TITLE" },
	{ "C2A1", "C2A1TITLE" },
	{ "C2A2", "C2A2TITLE" },
	{ "C2A3", "C2A3TITLE" },
	{ "C2A4D", "C2A4TITLE2" },
	{ "C2A4E", "C2A4TITLE2" },
	{ "C2A4F", "C2A4TITLE2" },
	{ "C2A4G", "C2A4TITLE2" },
	{ "C2A4", "C2A4TITLE1" },
	{ "C2A5", "C2A5TITLE" },
	{ "C3A1", "C3A1TITLE" },
	{ "C3A2", "C3A2TITLE" },
	{ "C4A1A", "C4A1ATITLE" },
	{ "C4A1B", "C4A1ATITLE" },
	{ "C4A1C", "C4A1ATITLE" },
	{ "C4A1D", "C4A1ATITLE" },
	{ "C4A1E", "C4A1ATITLE" },
	{ "C4A1", "C4A1TITLE" },
	{ "C4A2", "C4A2TITLE" },
	{ "C4A3", "C4A3TITLE" },
	{ "C5A1", "C5TITLE" },
};

// Game Desription
TYPEDESCRIPTION gGameHeaderDescription[] =
{
	DEFINE_FIELD(GAME_HEADER, mapCount, FIELD_INTEGER),
	DEFINE_ARRAY(GAME_HEADER, mapName, FIELD_CHARACTER, 32), // sizeof(.mapName)
	DEFINE_ARRAY(GAME_HEADER, comment, FIELD_CHARACTER, 80), // sizeof(.comment)
};

// The proper way to extend the file format (add a new data chunk) is to add a field here, and use it to determine
// whether your new data chunk is in the file or not.  If the file was not saved with your new field, the chunk 
// won't be there either.
// Structure members can be added/deleted without any problems, new structures must be reflected in an existing struct
// and not read unless actually in the file.  New structure members will be zeroed out when reading 'old' files.

// Header Desription
TYPEDESCRIPTION gSaveHeaderDescription[] =
{
	DEFINE_FIELD(SAVE_HEADER, skillLevel, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, entityCount, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, connectionCount, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, lightStyleCount, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, time, FIELD_TIME),
	DEFINE_ARRAY(SAVE_HEADER, mapName, FIELD_CHARACTER, 32), // sizeof(.mapName)
	DEFINE_ARRAY(SAVE_HEADER, skyName, FIELD_CHARACTER, 32), // sizeof(.skyName)

	DEFINE_FIELD(SAVE_HEADER, skyColor_r, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, skyColor_g, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, skyColor_b, FIELD_INTEGER),
	DEFINE_FIELD(SAVE_HEADER, skyVec_x, FIELD_FLOAT),
	DEFINE_FIELD(SAVE_HEADER, skyVec_y, FIELD_FLOAT),
	DEFINE_FIELD(SAVE_HEADER, skyVec_z, FIELD_FLOAT),
};

// Adjacency Data
// Store landmark information
TYPEDESCRIPTION gAdjacencyDescription[] =
{
	DEFINE_ARRAY(LEVELLIST, mapName, FIELD_CHARACTER, 32), // sizeof(.mapName)
	DEFINE_ARRAY(LEVELLIST, landmarkName, FIELD_CHARACTER, 32), // sizeof(.landmarkName)
	DEFINE_FIELD(LEVELLIST, pentLandmark, FIELD_EDICT),
	DEFINE_FIELD(LEVELLIST, vecLandmarkOrigin, FIELD_VECTOR),
};

TYPEDESCRIPTION gEntityTableDescription[] =
{
	DEFINE_FIELD(ENTITYTABLE, id, FIELD_INTEGER),
	DEFINE_FIELD(ENTITYTABLE, location, FIELD_INTEGER),
	DEFINE_FIELD(ENTITYTABLE, size, FIELD_INTEGER),
	DEFINE_FIELD(ENTITYTABLE, flags, FIELD_INTEGER),
	DEFINE_FIELD(ENTITYTABLE, classname, FIELD_STRING),
};

TYPEDESCRIPTION gLightstyleDescription[] =
{
	DEFINE_FIELD(SAVELIGHTSTYLE, index, FIELD_INTEGER),
	DEFINE_ARRAY(SAVELIGHTSTYLE, style, FIELD_CHARACTER, MAX_LIGHTSTYLES),
};

cvar_t gHostMap = { "HostMap", "C1A0" };

/*
====================
SV_InactivateClients

Prepare for level transition, etc.
====================
*/
void SV_InactivateClients( void )
{
	int i;
	client_t* cl;

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if (!cl->active && !cl->connected && !cl->spawned)
			continue;

		SZ_Clear(&cl->netchan.message);

		cl->active = FALSE;
		cl->connected = TRUE;
		cl->spawned = FALSE;

		COM_ClearCustomizationList(&cl->customdata, FALSE);
		cl->maxspeed = 0.0;
	}
}

/*
=============================================================================

Con_Printf redirection

=============================================================================
*/

char	outputbuf[8000];

redirect_t	sv_redirected;

netadr_t sv_redirectto;

/*
==================
Host_FlushRedirect
==================
*/
void Host_FlushRedirect( void )
{
	sizebuf_t	buf;
	byte		data[1425];

	if (sv_redirected == RD_PACKET)
	{
		buf.maxsize = sizeof(data);
		buf.cursize = 0;
		buf.data = data;

		MSG_WriteLong(&buf, 0xffffffff); // -1 -1 -1 -1 signal
		MSG_WriteByte(&buf, A2C_PRINT);
		MSG_WriteString(&buf, outputbuf);
		MSG_WriteByte(&buf, 0);

		NET_SendPacket(NS_SERVER, buf.cursize, buf.data, sv_redirectto);
	}
	else if (sv_redirected == RD_CLIENT)   // Send to client on message stream.
	{
		MSG_WriteByte(&host_client->netchan.message, svc_print);
		MSG_WriteString(&host_client->netchan.message, outputbuf);
	}

	// clear it
	outputbuf[0] = 0;
}


/*
==================
Host_BeginRedirect

  Send Con_Printf data to the remote client
  instead of the console
==================
*/
void Host_BeginRedirect( redirect_t rd, netadr_t* addr )
{
	sv_redirected = rd;
	sv_redirectto = *addr;
	outputbuf[0] = 0;
}

void Host_EndRedirect( void )
{
	Host_FlushRedirect();
	sv_redirected = RD_NONE;
}

static int Rcon_Validate( void )
{
	if (!strlen(rcon_password.string))
		return 0;

	if (strcmp(Cmd_Argv(1), rcon_password.string))
		return 0;

	return 1;
}

/*
===============
Host_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void Host_RemoteCommand( netadr_t* net_from )
{
	int		i;
	int		valid;
	char	remaining[1024];

	// Verify this user has access rights.
	valid = Rcon_Validate();

	if (valid)
	{
		Con_Printf("Rcon from %s:\n%s\n", NET_AdrToString(*net_from), net_message.data + 4);
		Log_Printf("Rcon from \"%s\": \"%s\"\n", NET_AdrToString(*net_from), net_message.data + 4);
	}
	else
	{
		Con_Printf("Bad rcon from %s:\n%s\n", NET_AdrToString(*net_from), net_message.data + 4);
		Log_Printf("Bad Rcon from \"%s\": \"%s\"\n", NET_AdrToString(*net_from), net_message.data + 4);
	}

	Host_BeginRedirect(RD_PACKET, net_from);

	if (valid)
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			strcat(remaining, Cmd_Argv(i));
			strcat(remaining, " ");
		}

		Cmd_ExecuteString(remaining, src_command);
	}
	else
	{
		Con_Printf("Bad rcon_password.\n");
	}

	Host_EndRedirect();
}

/*
==================
Host_Quit_f
==================
*/
void Host_Quit_f( void )
{
	if (Cmd_Argc() == 1)
	{
		giActive = DLL_CLOSE;

		if (cls.state != ca_dedicated)
			CL_Disconnect();

		Host_ShutdownServer(FALSE);
		Sys_Quit();
	}

	giActive = DLL_PAUSED;
	giStateInfo = 4;
}

/*
==================
Host_Status_PrintClient

Print client info to console
==================
*/
void Host_Status_PrintClient( char* pszState, qboolean fromcbuf, void (*print) ( char* fmt, ... ), int playernum, client_t* client )
{
	int			seconds;
	int			minutes;
	int			hours = 0;

	seconds = (int)(realtime - client->netchan.connect_time);
	minutes = seconds / 60;
	if (minutes)
	{
		seconds %= 60;
		hours = minutes / 60;
		if (hours)
			minutes %= 60;
	}
	else
		hours = 0;

	print("#%-2u %-12.12s\n", playernum + 1, client->name);
	print("   frags:  %3i\n", client->edict->v.frags);

	if (hours)
		print("   time :  %2i:%02i:%02i\n", hours, minutes, seconds);
	else
		print("   time :  %02i:%02i\n", minutes, seconds);

	print("   frame rate :  %4i\n", (int)(1000.0f * client->netchan.frame_rate));
	print("   frame latency :  %4i\n", (int)(1000.0f * client->netchan.frame_latency));
	print("   ping :  %4i\n", SV_CalcPing(client));
	print("   drop :  %5.2f %%\n", 100.0f * client->netchan.drop_count / client->netchan.incoming_sequence);

	if (pszState && pszState[0])
		print("   %s\n", pszState);

	if (client->spectator)
	{
		print("  (spectator) %s\n", NET_BaseAdrToString(client->netchan.remote_address));
	}
	else if (client->fakeclient)
	{
		print("  (fake)\n");
	}
	else if (fromcbuf)
	{
		print("   %s\n", NET_BaseAdrToString(client->netchan.remote_address));
	}
	print("\n");
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f( void )
{
	client_t* client;
	int			j;
	int			players, spectators;
	void		(*print) ( char* fmt, ... );

	if (cmd_source == src_command)
	{
		if (sv.active == FALSE)
		{
			Cmd_ForwardToServer();
			return;
		}
		print = Con_Printf;
	}
	else
		print = SV_ClientPrintf;

	// ============================================================
	// Server status information.
	print("hostname:  %s\n", Cvar_VariableString("hostname"));
	print("build   :  %d\n", build_number());
	print("tcp/ip  :  %s\n", NET_AdrToString(net_local_adr));
	print(" map     :  %s\n", sv.name);

	SV_CountPlayers(&players, &spectators);

	if (spectators)
		print(" players: %i active (%i spectators) (%i max)\n\n", players, spectators, svs.maxclients);
	else
		print(" players: %i active (%i max)\n\n", players, svs.maxclients);

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active)
		{
			if (client->connected)
				Host_Status_PrintClient("CONNECTING", cmd_source == src_command, print, j, client);		
			continue;
		}

		Host_Status_PrintClient(NULL, cmd_source == src_command, print, j, client);
	}
}

/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (gGlobalVariables.deathmatch && !host_client->privileged)
		return;

	if (!allow_cheats)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE))
		SV_ClientPrintf("godmode OFF\n");
	else
		SV_ClientPrintf("godmode ON\n");
}

void Host_Notarget_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (gGlobalVariables.deathmatch && !host_client->privileged)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
	if (!((int)sv_player->v.flags & FL_NOTARGET))
		SV_ClientPrintf("notarget OFF\n");
	else
		SV_ClientPrintf("notarget ON\n");
}

/*
==================
FindPassableSpace

Searches along the direction ray in steps of "step" to see if
the entity position is passible
Used for putting the player in valid space when toggling off noclip mode
==================
*/
int FindPassableSpace( edict_t* pEdict, vec_t* direction, float step )
{
    int		i;

    for (i = 0; i < 100; i++)
    {
        VectorMA(pEdict->v.origin, step, direction, pEdict->v.origin);

		if (!SV_TestEntityPosition(pEdict))
        {
            // Store old origin
            VectorCopy(pEdict->v.origin, pEdict->v.oldorigin);
            return TRUE;
        }
    }

	return FALSE;
}

qboolean noclip_anglehack;

void Host_Noclip_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (gGlobalVariables.deathmatch && !host_client->privileged)
		return;

	if (!allow_cheats)
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
    {
		noclip_anglehack = TRUE;
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf("noclip ON\n");
    }
    else
    {
		noclip_anglehack = FALSE;
        sv_player->v.movetype = MOVETYPE_WALK;
        VectorCopy(sv_player->v.origin, sv_player->v.oldorigin);
		SV_ClientPrintf("noclip OFF\n");

		if (SV_TestEntityPosition(sv_player))
        {
            vec3_t forward, right, up;
			AngleVectors(sv_player->v.v_angle, forward, right, up);

			if (!FindPassableSpace(sv_player, forward, 1.0)
                && !FindPassableSpace(sv_player, right, 1.0)
                && !FindPassableSpace(sv_player, right, -1.0)		// left
                && !FindPassableSpace(sv_player, up, 1.0)			// up
                && !FindPassableSpace(sv_player, up, -1.0)			// down
                && !FindPassableSpace(sv_player, forward, -1.0))	// back
            {
				Con_DPrintf("Can't find the world\n");
            }

            VectorCopy(sv_player->v.oldorigin, sv_player->v.origin);
        }
    }
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (gGlobalVariables.deathmatch && !host_client->privileged)
		return;
	
	if (sv_player->v.movetype != MOVETYPE_FLY)
	{
		sv_player->v.movetype = MOVETYPE_FLY;
		SV_ClientPrintf("flymode ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf("flymode OFF\n");
	}
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f( void )
{
	int		i;
	client_t* client;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	SV_ClientPrintf("Client ping times:\n");
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;
		SV_ClientPrintf("%4i %s\n", SV_CalcPing(client), client->name);
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map
======================
*/
void Host_Map( qboolean bIsDemo, char* mapstring, char* mapName, qboolean loadGame )
{
	int		i;

	CL_Disconnect();
	Host_ShutdownServer(FALSE);

	key_dest = key_game;			// remove console or menu
	SCR_BeginLoadingPlaque();

	if (!loadGame)
	{
		Host_ClearGameState();
		SV_InactivateClients();
		svs.serverflags = 0;			// haven't completed an episode yet
	}

	strcpy(cls.mapstring, mapstring);

	if (!SV_SpawnServer(bIsDemo, mapName, NULL))
		return;

	if (loadGame)
	{
		if (!LoadGamestate(mapName, TRUE))
		{
			SV_LoadEntities();
		}

		sv.paused = TRUE;		// pause until all clients connect
		sv.loadgame = TRUE;
		SV_ActivateServer(FALSE);
	}
	else
	{
		SV_LoadEntities();
		SV_ActivateServer(TRUE);

		if (!sv.active)
			return;

		if (cls.state != ca_dedicated)
		{
			strcpy(cls.spawnparms, "");

			for (i = 2; i < Cmd_Argc(); i++)
			{
				strcat(cls.spawnparms, Cmd_Argv(i));
				strcat(cls.spawnparms, " ");
			}
		}
	}

	// Link usermsgs
	if (sv_gpNewUserMsgs)
	{
		UserMsg* pMsg = sv_gpUserMsgs;
		if (pMsg)
		{
			while (pMsg->next)
				pMsg = pMsg->next;
			pMsg->next = sv_gpNewUserMsgs;
		}
		else
		{
			sv_gpUserMsgs = sv_gpNewUserMsgs;
		}
		sv_gpNewUserMsgs = NULL;
	}

	// Connect the local client when a "map" command is issued.
	if (cls.state != ca_dedicated)
	{
		Cmd_ExecuteString("connect local", src_command);
	}
}

/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f( void )
{
	int		i, len;
	char	mapstring[MAX_QPATH];
	char	name[MAX_QPATH];

	if (cmd_source != src_command)
		return;

	mapstring[0] = 0;
	for (i = 0; i < Cmd_Argc(); i++)
	{
		strcat(mapstring, Cmd_Argv(i));
		strcat(mapstring, " ");
	}
	strcat(mapstring, "\n");

	strcpy(name, Cmd_Argv(1));

	len = strlen(name);
	if (len > 4 && !_stricmp(&name[len - 4], ".bsp"))
		name[len - 4] = 0;

	if (!PF_IsMapValid_I(name))
	{
		Con_Printf("map change failed: '");
		Con_Printf(name);
		Con_Printf("' not found on server\n");
		return;
	}

	Cvar_Set("HostMap", name);

	Host_Map(FALSE, mapstring, name, FALSE);
}

/*
======================
Host_Maps_f

======================
*/
void Host_Maps_f( void )
{
	char	szMapName[MAX_QPATH];
	char* pszSubString;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("Usage:  maps <substring>\nmaps * for full listing\n");
		return;
	}

	pszSubString = Cmd_Argv(1);
	if (!pszSubString || !pszSubString[0])
		return;

	if (pszSubString[0] == '*')
		pszSubString = NULL;

	COM_ListMaps(pszSubString);
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f( void )
{
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char*   startspot;


	if (Cmd_Argc() < 2)
	{
		Con_Printf("changelevel <levelname> : continue game on a new level\n");
		return;
	}

	if (!sv.active)
	{
		Con_Printf("Only the server may changelevel\n");
		return;
	}

	if (!PF_IsMapValid_I(Cmd_Argv(1)))
	{
		Con_Printf("changelevel failed: '");
		Con_Printf(Cmd_Argv(1));
		Con_Printf("' not found on server\n");
		return;
	}

	SCR_BeginLoadingPlaque();

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);

	strcpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		strcpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_InactivateClients();

	SV_SpawnServer(FALSE, level, startspot);
	SV_LoadEntities();
	SV_ActivateServer(TRUE);
}

char* Host_FindRecentSave( char* pNameBuf )
{
	HANDLE		findfn;
	BOOL		nextfile;
	WIN32_FIND_DATAA ffd;
	int	        found;
	FILETIME	newest;

	sprintf(pNameBuf, "%s*.sav", Host_SaveGameDirectory());

	findfn = FindFirstFile(pNameBuf, &ffd);
	if (findfn == INVALID_HANDLE_VALUE)
		return NULL;

	found = 0;

	do
	{
		// Don't load HLSave.sav -- it's a temporary file used by the launcher when switching video modes
		if (_stricmp(ffd.cFileName, "HLSave.sav"))
		{
			// Should we use the matche?
			if (!found || CompareFileTime(&newest, &ffd.ftLastWriteTime) < 0)
			{
				newest = ffd.ftLastWriteTime;
				strcpy(pNameBuf, ffd.cFileName);
				found = 1;
			}
		}

		// Any more save files
		nextfile = FindNextFile(findfn, &ffd);
	} while (nextfile);

	FindClose(findfn);

	if (found)
		return pNameBuf;

	return NULL;
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f( void )
{
	char	name[MAX_PATH];

	if (!sv.active)
		return;

	if (cmd_source != src_command)
		return;

	Host_ClearGameState();
	SV_InactivateClients();

	strcpy(name, sv.name);	// must copy out, because it gets cleared
							// in sv_spawnserver
	SV_SpawnServer(FALSE, name, NULL);
	SV_LoadEntities();
	SV_ActivateServer(TRUE);
}

/*
==================
Host_Reload_f

Restarts the current server for a dead player
==================
*/
void Host_Reload_f( void )
{
	char* pSaveName;
	char name[MAX_PATH];

	if (!sv.active)
		return;

	if (cmd_source != src_command)
		return;

	Host_ClearGameState();
	SV_InactivateClients();

	// See if there is a most recently saved game
	// Restart that game if there is
	// Otherwise, restart the starting game map
	pSaveName = Host_FindRecentSave(name);
	if (pSaveName && Host_Load(pSaveName))
		return;

	SV_SpawnServer(FALSE, gHostMap.string, NULL);
	SV_LoadEntities();
	SV_ActivateServer(TRUE);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f( void )
{
	if (cmd_source == src_command)
	{
		Con_Printf("reconnect is not valid from the console\n");
		return;
	}

	if (cls.state == ca_dedicated ||
		cls.state == ca_disconnected ||
		cls.state == ca_connecting)
		return;

	SCR_BeginLoadingPlaque();

	cls.signon = 0;		// need new connection messages
	cls.state = ca_connected;	// not active anymore, but not disconnected

	// Write a "new" command into client's buffer.
	MSG_WriteChar(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, "new");
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f( void )
{
	char	name[MAX_QPATH];

	if (cmd_source == src_command)
	{
		Con_Printf("connect is not valid from the console\n");
		return;
	}

	if (Cmd_Argc() < 2 || !Cmd_Args())
	{
		Con_Printf("Usage:  connect <server>\n");
		return;
	}

	strcpy(name, Cmd_Args());
	strncpy(cls.servername, name, sizeof(cls.servername) - 1);
	CL_Connect_f();
}

/*
=====================
Host_Spectate_f

User command to connect to server as spectator
=====================
*/
void Host_Spectate_f( void )
{
	char	name[MAX_QPATH];

	cls.demonum = -1;		// stop demo loop in case this fails
	if (Cmd_Argc() < 2 || !Cmd_Args())
	{
		Con_Printf("Usage:  spectate <server>\n");
		return;
	}

	strcpy(name, Cmd_Args());
	strncpy(cls.servername, name, sizeof(cls.servername) - 1);
	CL_Spectate_f();
}

/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

/*
==================
Host_SaveGameDirectory

Return the save directory
==================
*/
char* Host_SaveGameDirectory( void )
{
	static char szDirectory[MAX_PATH];
	memset(szDirectory, 0, sizeof(szDirectory));

	sprintf(szDirectory, "%s/SAVE/", com_gamedir);

	// Our save dir
	return szDirectory;
}

/*
===============
Host_SavegameComment

===============
*/
void Host_SavegameComment( char* pszBuffer )
{
	int		i;
	int		nChars;
	int		nHour;
	char*	pName;
	char*	pStr;
	char*	pszAMPM;
	char*	pszMapName;
	char	szTitle[64];
	SYSTEMTIME	systemTime;
	client_textmessage_t* pMessage;

	pName = NULL;
	pszMapName = &pr_strings[gGlobalVariables.mapname];

	for (i = 0; i < ARRAYSIZE(gTitleComments) && !pName; i++)
	{
		// Setup the comment from the titles.txt file
		if (!Q_strnicmp(pszMapName, gTitleComments[i].pBSPName, strlen(gTitleComments[i].pBSPName)))
		{
			pMessage = TextMessageGet(gTitleComments[i].pTitleName);
			if (pMessage)
			{
				strncpy(szTitle, pMessage->pMessage, 64);
				pName = szTitle;

				// Strip out the line feeds
				nChars = 0;
				pStr = szTitle;
				while (nChars < 64 && *pStr)
				{
					if (*pStr == '\n' || *pStr == '\r')
						*pStr = 0;
					else
					{
						nChars++;
						pStr++;
					}
				}
			}
		}
	}

	if (!pName)
	{
		if (cl.levelname && strlen(cl.levelname))
			pName = cl.levelname;
		else
			pName = pszMapName;
	}

	sprintf(pszBuffer, "%-64.64s %02d:%02d", pName, (int)(sv.time / 60.0f), (int)fmod(sv.time, 60.0f));

	// Stamp the memory card entry with the local date and time
	GetLocalTime(&systemTime);

	if (g_Language)
	{
		sprintf(vmuSaveComment, "%s %d/%d %02d:%02d\n", pName, systemTime.wDay, systemTime.wMonth, systemTime.wHour, systemTime.wMinute);
	}
	else
	{
		nHour = systemTime.wHour % 12;
		if (!nHour)
			nHour = 12;

		if (systemTime.wHour > 11)
			pszAMPM = "pm";
		else
			pszAMPM = "am";

		sprintf(vmuSaveComment, "%s %d/%d %d:%02d%s\n", pName, systemTime.wMonth, systemTime.wDay, nHour, systemTime.wMinute, pszAMPM);
	}

	sprintf(vmuSaveTitle, "%-15.15s", pszMapName);
}

/*
==================
Host_AgeSaveList

Roll the numbered save slots down one, dropping the oldest, so the newest save
can take the unnumbered name
==================
*/
void Host_AgeSaveList( const char* pName, int count )
{
	char	newName[MAX_PATH], oldName[MAX_PATH];

	// The last one falls off the end of the list
	sprintf(newName, "%s%s%02d.sav", Host_SaveGameDirectory(), pName, count);
	COM_FixSlashes(newName);
	Bremove_path(newName);

	while (count > 0)
	{
		if (count == 1)
			sprintf(oldName, "%s%s.sav", Host_SaveGameDirectory(), pName);
		else
			sprintf(oldName, "%s%s%02d.sav", Host_SaveGameDirectory(), pName, count - 1);
		COM_FixSlashes(oldName);

		sprintf(newName, "%s%s%02d.sav", Host_SaveGameDirectory(), pName, count);
		COM_FixSlashes(newName);

		Brename_path(oldName, newName);
		count--;
	}
}

int Host_ValidSave( void )
{
	if (cmd_source != src_command)
		return 0;

	if (!sv.active)
	{
		Con_Printf("Not playing a local game.\n");
		return 0;
	}

	if (cl.intermission || cls.state != ca_active)
	{
		Con_Printf("Can't save in intermission.\n");
		return 0;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf("Can't save multiplayer games.\n");
		return 0;
	}

	if (svs.clients->active && svs.clients->edict->v.health <= 0.0f)
	{
		Con_Printf("Can't savegame with a dead player\n");
		return 0;
	}

	// Passed all checks, it's ok to save
	return 1;
}

/*
==================
SaveInit

Initialize Save/Restore Data
==================
*/
SAVERESTOREDATA* SaveInit( int size )
{
	SAVERESTOREDATA* pSaveData;
	int i;
	edict_t* pEdict = NULL;

	if (size <= 0)
		size = 0x80000;		// Reserve 512K for now, UNDONE: Shrink this after compressing strings

	pSaveData = (SAVERESTOREDATA*)calloc(sizeof(SAVERESTOREDATA) + (sizeof(ENTITYTABLE) * sv.num_edicts) + size, sizeof(char));
	pSaveData->pTable = (ENTITYTABLE*)(pSaveData + 1); // skip the save structure
	pSaveData->tokenSize = 0;
	pSaveData->tokenCount = 0xfff; // Assume a maximum of 4K-1 symbol table entries(each of some length)
	pSaveData->pTokens = (char**)calloc(pSaveData->tokenCount, sizeof(char*));

	for (i = 0; i < sv.num_edicts; i++)
	{
		pEdict = &sv.edicts[i];

		pSaveData->pTable[i].id = i;
		pSaveData->pTable[i].pent = pEdict;
		pSaveData->pTable[i].flags = 0;
		pSaveData->pTable[i].location = 0;
		pSaveData->pTable[i].size = 0;
		pSaveData->pTable[i].classname = 0;
	}

	pSaveData->tableCount = sv.num_edicts;
	pSaveData->size = 0;
	pSaveData->time = gGlobalVariables.time; // Use DLL time
	pSaveData->fUseLandmark = FALSE;
	pSaveData->bufferSize = size;
	pSaveData->connectionCount = 0;

	pSaveData->pBaseData = (char*)(pSaveData->pTable + sv.num_edicts); // skip the save structure
	pSaveData->pCurrentData = (char*)(pSaveData->pTable + sv.num_edicts); // reset the pointer

	VectorCopy(vec3_origin, pSaveData->vecLandmarkOffset);

	// share with dlls
	gGlobalVariables.pSaveData = pSaveData;

	return pSaveData;
}

/*
==================
SaveExit

Frees save tokens and save/restore data
==================
*/
void SaveExit( SAVERESTOREDATA* save )
{
	if (save->pTokens)
	{
		free(save->pTokens);
		save->pTokens = NULL;

		save->tokenCount = 0;
	}

	if (save)
		free(save);

	gGlobalVariables.pSaveData = NULL;
}

/*
==================
SaveGameSlot

Do a save game
==================
*/
BOOL SaveGameSlot( const char* pSaveName, const char* pSaveComment, int fake )
{
	char			hlPath[256], name[256], * pTokenData;
	int				tag, i;
	bfile_t* pFile;
	SAVERESTOREDATA* pSaveData;
	GAME_HEADER		gameHeader;

	pSaveData = SaveGamestate();

	if (!pSaveData)
		return FALSE;

	SaveExit(pSaveData);
	pSaveData = SaveInit(0);

	sprintf(hlPath, "%s*.HL?", Host_SaveGameDirectory());
	COM_FixSlashes(hlPath);

	gameHeader.mapCount = DirectoryCount(hlPath);
	strcpy(gameHeader.mapName, sv.name);
	strcpy(gameHeader.comment, pSaveComment);

	SaveWriteFields(pSaveData, "GameHeader", &gameHeader, gGameHeaderDescription, Q_ARRAYSIZE(gGameHeaderDescription));
	SaveGlobalState(pSaveData);

	// Write entity string token table
	pTokenData = pSaveData->pCurrentData;
	if (pSaveData->pTokens)
	{
		// Make sure the token strings pointed to by the pToken hashtable.
		for (i = 0; i < pSaveData->tokenCount; i++)
		{
			char* pszToken = pSaveData->pTokens[i];

			if (pszToken)
			{
				pSaveData->size += strlen(pszToken) + 1;

				if (pSaveData->size > pSaveData->bufferSize)
				{
					Con_Printf("Token Table Save/Restore overflow!");
					pSaveData->size = pSaveData->bufferSize;
					break;
				}

				do
				{
					*pSaveData->pCurrentData++ = *pszToken;
				} while (*pszToken++);
			}
			else
			{
				pSaveData->size += 1;

				if (pSaveData->size > pSaveData->bufferSize)
				{
					Con_Printf("Token Table Save/Restore overflow!");
					pSaveData->size = pSaveData->bufferSize;
					break;
				}

				// Write the term
				*pSaveData->pCurrentData++ = '\0';
			}
		}
	}

	pSaveData->tokenSize = pSaveData->pCurrentData - pTokenData;
	if (pSaveData->size < pSaveData->bufferSize)
		pSaveData->size -= pSaveData->tokenSize;

	sprintf(name, "%s%s", Host_SaveGameDirectory(), pSaveName);
	COM_DefaultExtension(name, ".sav");
	COM_FixSlashes(name);
	Con_DPrintf("Saving game to %s...\n", name);

	pFile = Bopen(name, "wb");
	// Write the header -- THIS SHOULD NEVER CHANGE STRUCTURE, USE SAVE_HEADER FOR NEW HEADER INFORMATION
	// THIS IS ONLY HERE TO IDENTIFY THE FILE AND GET IT'S SIZE.
	tag = SAVEGAME_HEADER;
	Bwrite(&tag, sizeof(int), 1, pFile);   // Write header
	tag = SAVEGAME_VERSION;
	Bwrite(&tag, sizeof(int), 1, pFile);   // Write version
	Bwrite(&pSaveData->size, sizeof(int), 1, pFile);   // Does not include token table

	// Write out the tokens first so we can load them before we load the entities
	Bwrite(&pSaveData->tokenCount, sizeof(int), 1, pFile);
	Bwrite(&pSaveData->tokenSize, sizeof(int), 1, pFile);
	Bwrite(pTokenData, pSaveData->tokenSize, 1, pFile);

	Bwrite(pSaveData->pBaseData, pSaveData->size, 1, pFile);

	DirectoryCopy(hlPath, pFile);
	Bclose(pFile);
	SaveExit(pSaveData);

	return TRUE;
}

/*
=================
CL_HudMessage

Let the engine execute HUD message from client
=================
*/
void CL_HudMessage( const char* pMessage )
{
	DispatchDirectUserMsg("HudText", strlen(pMessage), (void*)pMessage);
}

/*
==================
Host_Savegame_f

Save the game
==================
*/
void Host_Savegame_f( void )
{
	char	szComment[80];
	int		fake;

	fake = 0;

	if (!Host_ValidSave())
		return;

	if (Cmd_Argc() < 2)
		return;

	if (strstr(Cmd_Argv(1), ".."))
		return;

	// "save <name> fake" writes the slot without committing it to the memory card
	if (Cmd_Argc() > 2)
	{
		if (!strcmp(Cmd_Argv(2), "fake"))
		{
			gSaveGameSize = 0;
			fake = 1;
		}
	}

	Host_SavegameComment(szComment);
	SaveGameSlot(Cmd_Argv(1), szComment, fake);

	if (fake)
		Host_SetMessage(VMU_MarkSlotSaved());
}

/*
==================
Host_AutoSave_f

Autosave the game
==================
*/
void Host_AutoSave_f( void )
{
	char szComment[80];

	if (!Host_ValidSave())
		return;
	
	Host_SavegameComment(szComment);
	SaveGameSlot("autosave", szComment, 0);
}

DLL_EXPORT BOOL SaveGame( char* pszSlot, char* pszComment )
{
	qboolean qret;
	qboolean q;

	q = scr_skipupdate;
	scr_skipupdate = TRUE;
	qret = SaveGameSlot(pszSlot, pszComment, 0);
	scr_skipupdate = q;
	return qret;
}

int SaveReadHeader( void* pFile, GAME_HEADER* pHeader, int readGlobalState )
{
	int             i, tag, size, tokenCount, tokenSize;
	char* pszTokenList;
	SAVERESTOREDATA* pSaveData;

	DC_fread(&tag, sizeof(int), 1, pFile);
	if (tag != SAVEGAME_HEADER)
	{
		Sys_CloseHandle(pFile);
		return 0;
	}

	DC_fread(&tag, sizeof(int), 1, pFile);
	if (tag != SAVEGAME_VERSION) // Enforce version for now
	{
		Sys_CloseHandle(pFile);
		return 0;
	}

	DC_fread(&size, sizeof(int), 1, pFile);
	DC_fread(&tokenCount, sizeof(int), 1, pFile); // These two ints are the token list
	DC_fread(&tokenSize, sizeof(int), 1, pFile);

	pSaveData = (SAVERESTOREDATA*)calloc(sizeof(SAVERESTOREDATA) + tokenSize + size + SAVE_HEAPSLACK, sizeof(char));
	pSaveData->tableCount = 0;
	pSaveData->pTable = NULL;
	pSaveData->connectionCount = 0;

	// Parse the symbol table
	pszTokenList = (char*)(pSaveData + 1);

	if (tokenSize > 0)
	{
		pSaveData->tokenCount = tokenCount;
		pSaveData->tokenSize = tokenSize;

		DC_fread(pszTokenList, tokenSize, 1, pFile);

		if (!pSaveData->pTokens)
			pSaveData->pTokens = (char**)calloc(tokenCount + 8, sizeof(char*));

		// Make sure the token strings pointed to by the pToken hashtable.
		for (i = 0; i < tokenCount; i++)
		{
			if (*pszTokenList)
				pSaveData->pTokens[i] = pszTokenList;
			else
				pSaveData->pTokens[i] = NULL;

			while (*pszTokenList++)
				;
		}
	}

	pSaveData->size = 0;
	pSaveData->bufferSize = size;
	pSaveData->fUseLandmark = FALSE;
	pSaveData->time = 0;

	pSaveData->pCurrentData = pszTokenList;
	pSaveData->pBaseData = pszTokenList;

	DC_fread(pSaveData->pBaseData, size, 1, pFile);

	SaveReadFields(pSaveData, "GameHeader", pHeader, gGameHeaderDescription, Q_ARRAYSIZE(gGameHeaderDescription));
	if (readGlobalState)
		RestoreGlobalState(pSaveData);
	SaveExit(pSaveData);

	return 1;
}

void SaveReadComment( void* f, char* name )
{
	GAME_HEADER gameHeader;

	if (!SaveReadHeader(f, &gameHeader, FALSE))
		return;

	strcpy(name, gameHeader.comment);
}

/*
==================
Host_Loadgame_f

Load saved game
==================
*/
void Host_Loadgame_f( void )
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("load <savename> : load a game\n");
		return;
	}
	
	if (!Host_Load(Cmd_Argv(1)))
	{
		Con_Printf("Error loading saved game\n");
		return;
	}
}

DLL_EXPORT int LoadGame( char* pName )
{
	int iRet = 0;
	qboolean q;

	q = scr_skipupdate;
	scr_skipupdate = TRUE;
	iRet = Host_Load(pName);
	scr_skipupdate = q;
	return iRet;
}

int Host_Load( const char* pName )
{
	void* pFile;
	GAME_HEADER     gameHeader;
	char			name[256];
	int             c;
	char* pTempNumber;
	char            szNumber[5] = { 0 };
	int             nSlot;
	
	if (!pName || !pName[0])
		return FALSE;

	// Handle quick-save slots (_1 to _12)
	if (pName[0] == '_')
	{
		pTempNumber = (char*)(pName + 1);
		c = 0;

		// Extract up to 5 digits from slot number
		while (*pTempNumber && c < sizeof(szNumber))
		{
			if (!isdigit(*pTempNumber))
				break;

			szNumber[c++] = *pTempNumber++;
		}
		szNumber[c] = 0;

		nSlot = atoi(szNumber);
		if (nSlot < 1 || nSlot > 12)
			return FALSE;

		sprintf(name, "%sHalf-Life-%i", Host_SaveGameDirectory(), nSlot);
	}
	else
	{
		sprintf(name, "%s%s", Host_SaveGameDirectory(), pName);
	}

	COM_DefaultExtension(name, ".sav");
	COM_FixSlashes(name);
	Con_Printf("Loading game from %s...\n", name);

	pFile = Sys_OpenHandle(name, "rb");
	if (!pFile)
		return FALSE;

	Host_ClearGameState();

	if (!SaveReadHeader(pFile, &gameHeader, TRUE))
	{
		giStateInfo = 1;
		Cbuf_AddText("\ndisconnect\n");
		return FALSE;
	}

	cls.demonum = -1;
	SV_InactivateClients();
	SCR_BeginLoadingPlaque();

	DirectoryExtract(pFile, gameHeader.mapCount);
	Sys_CloseHandle(pFile);

	Cvar_SetValue("deathmatch", 0.0);
	Cvar_SetValue("coop", 0.0);
	Cvar_SetValue("mp_teamplay", 0.0);

	sprintf(name, "map %s\n", gameHeader.mapName);
	Host_Map(FALSE, name, gameHeader.mapName, TRUE);

	return TRUE;
}

/*
==================
SaveGamestate

Saves Game State, when it's called
==================
*/
SAVERESTOREDATA* SaveGamestate( void )
{
	char            name[256];
	char* pTableData, * pTokenData;
	bfile_t* pFile;
	int             i, id, version;
	int			    dataSize, tableSize;
	edict_t* pent;
	SAVE_HEADER     header;
	SAVERESTOREDATA* pSaveData;
	SAVELIGHTSTYLE  light;

	if (!ParmsChangeLevel)
		return NULL;

	pSaveData = SaveInit(0);

	sprintf(name, "%s%s.HL1", Host_SaveGameDirectory(), sv.name);
	COM_FixSlashes(name);

	ParmsChangeLevel();

	// Write global data
	header.version = build_number();
	header.skillLevel = skill.value;	// This is created from an int even though it's a float
	header.entityCount = pSaveData->tableCount;
	header.connectionCount = pSaveData->connectionCount;
	header.time = sv.time;

	strcpy(header.skyName, sv_skyname.string);
	header.skyColor_r = cl_skycolor_r.value;
	header.skyColor_g = cl_skycolor_g.value;
	header.skyColor_b = cl_skycolor_b.value;
	header.skyVec_x = cl_skyvec_x.value;
	header.skyVec_y = cl_skyvec_y.value;
	header.skyVec_z = cl_skyvec_z.value;

	// prohibits rebase of header.time
	pSaveData->time = 0.0;

	strcpy(header.mapName, sv.name);
	header.lightStyleCount = 0;
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (sv.lightstyles[i])
			header.lightStyleCount++;
	}

	// Write the main header
	SaveWriteFields(pSaveData, "Save Header", &header, gSaveHeaderDescription, Q_ARRAYSIZE(gSaveHeaderDescription));
	pSaveData->time = header.time;

	// Write adjacency list
	for (i = 0; i < pSaveData->connectionCount; i++)
		SaveWriteFields(pSaveData, "ADJACENCY", &pSaveData->levelList[i], gAdjacencyDescription, Q_ARRAYSIZE(gAdjacencyDescription));

	// Write the lightstyles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (sv.lightstyles[i])
		{
			light.index = i;
			strcpy(light.style, sv.lightstyles[i]);
			SaveWriteFields(pSaveData, "LIGHTSTYLE", &light, gLightstyleDescription, Q_ARRAYSIZE(gLightstyleDescription));
		}
	}

	for (i = 0; i < sv.num_edicts; i++)
	{
		pent = &sv.edicts[i];
		pSaveData->currentIndex = i;
		pSaveData->pTable[i].location = pSaveData->size;
		pSaveData->pTable[i].size = 0;

		if (pent->free)
			continue;

		DispatchSave(pent, pSaveData);

		if (i > 0 && i < svs.maxclients + 1)
			pSaveData->pTable[i].flags |= FENTTABLE_PLAYER;
	}

	dataSize = pSaveData->size;
	pTableData = pSaveData->pCurrentData;

	// Write entity table
	for (i = 0; i < sv.num_edicts; i++)
		SaveWriteFields(pSaveData, "ETABLE", &pSaveData->pTable[i], gEntityTableDescription, Q_ARRAYSIZE(gEntityTableDescription));

	tableSize = pSaveData->size - dataSize;
	pTokenData = pSaveData->pCurrentData;

	// Write entity string token table
	if (pSaveData->pTokens)
	{
		for (i = 0; i < pSaveData->tokenCount; i++)
		{
			char* pszToken = pSaveData->pTokens[i];

			if (pszToken)
			{
				do
				{
					*pSaveData->pCurrentData++ = *pszToken;
				} while (*pszToken++);
			}
			else
			{
				// Write the term
				*pSaveData->pCurrentData++ = '\0';
			}
		}
	}

	pSaveData->tokenSize = pSaveData->pCurrentData - pTokenData;

	// Output to disk
	COM_CreatePath(name);
	pFile = Bopen(name, "wb");
	if (!pFile)
	{
		Con_Printf("Unable to open save game file %s.", name);
		return NULL;
	}

	// Write the header -- THIS SHOULD NEVER CHANGE STRUCTURE, USE SAVE_HEADER FOR NEW HEADER INFORMATION
	// THIS IS ONLY HERE TO IDENTIFY THE FILE AND GET IT'S SIZE.
	id = SAVEFILE_HEADER;
	version = SAVEGAME_VERSION;

	// Write the header
	Bwrite(&id, sizeof(int), 1, pFile);
	Bwrite(&version, sizeof(int), 1, pFile);

	// Write out the tokens first so we can load them before we load the entities
	Bwrite(&pSaveData->size, sizeof(int), 1, pFile);		// total size of all data to initialize read buffer
	Bwrite(&pSaveData->tableCount, sizeof(int), 1, pFile);	// entities count to right initialize entity table
	Bwrite(&pSaveData->tokenCount, sizeof(int), 1, pFile);	// num hash tokens to prepare token table
	Bwrite(&pSaveData->tokenSize, sizeof(int), 1, pFile);	// total size of hash tokens
	Bwrite(pTokenData, pSaveData->tokenSize, 1, pFile);		// write tokens into the file
	Bwrite(pTableData, tableSize, 1, pFile);				// dump ETABLE structures
	Bwrite(pSaveData->pBaseData, dataSize, 1, pFile);		// and finally store all the other data
	Bclose(pFile);

	EntityPatchWrite(pSaveData, sv.name);

	sprintf(name, "%s%s.HL2", Host_SaveGameDirectory(), sv.name);
	COM_FixSlashes(name);
	// Let the client see the server entity to id lookup tables, etc.
	CL_Save(name);

	return pSaveData;
}

void CL_Save( char* name )
{
	DECALLIST       decalList[MAX_DECALS];
	int				i, decalCount;
	int             temp;
	bfile_t* pFile;

	decalCount = DecalListCreate(decalList);
	pFile = Bopen(name, "wb");
	if (pFile)
	{
		temp = SAVEFILE_HEADER;
		Bwrite(&temp, sizeof(int), 1, pFile);
		temp = SAVEGAME_VERSION;
		Bwrite(&temp, sizeof(int), 1, pFile);

		Bwrite(&decalCount, sizeof(int), 1, pFile);

		for (i = 0; i < decalCount; i++)
		{
			Bwrite(decalList[i].name, sizeof(char), 16, pFile);
			Bwrite(&decalList[i].entityIndex, sizeof(short), 1, pFile);
			Bwrite(&decalList[i].depth, sizeof(byte), 1, pFile);
			Bwrite(&decalList[i].flags, sizeof(byte), 1, pFile);
			Bwrite(decalList[i].position, sizeof(vec3_t), 1, pFile);
		}

		Bclose(pFile);
		Bcompress_path(name);
	}
}

/*
==================
EntityInit

==================
*/
void EntityInit( edict_t* pEdict, int className )
{
	ENTITYINIT pEntityInit;

	if (!className)
		Sys_Error("Bad class!!\n");

	ReleaseEntityDLLFields(pEdict);
	InitEntityDLLFields(pEdict);
	pEdict->v.classname = className;

	pEntityInit = GetEntityInit(&pr_strings[className]);
	if (pEntityInit)
		pEntityInit(&pEdict->v);
}

/*
==================
LoadSaveData

Parses save files and loads the data
==================
*/
SAVERESTOREDATA* LoadSaveData( const char* level )
{
	char			name[128];
	char* pszTokenList;
	void* pFile;
	int             i, tag;
	int             size;
	int		        tokenCount, tokenSize;
	int		        tableCount;
	SAVERESTOREDATA* pSaveData;

	sprintf(name, "%s%s.HL1", Host_SaveGameDirectory(), level);
	COM_FixSlashes(name);
	Con_Printf("Loading game from %s...\n", name);

	pFile = Sys_OpenHandle(name, "rb");
	if (!pFile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return NULL;
	}

	//---------------------------------
	// Read the header
	DC_fread(&tag, sizeof(int), 1, pFile);
	// Is this a valid save?
	if (tag != SAVEFILE_HEADER)
		return NULL;

	DC_fread(&tag, sizeof(int), 1, pFile);
	if (tag > SAVEGAME_VERSION)
		return NULL;

	// Read the sections info and the data
	DC_fread(&size, sizeof(int), 1, pFile);		// total size of all data to initialize read buffer
	DC_fread(&tableCount, sizeof(int), 1, pFile);	// entities count to right initialize entity table
	DC_fread(&tokenCount, sizeof(int), 1, pFile);	// num hash tokens to prepare token table
	DC_fread(&tokenSize, sizeof(int), 1, pFile);	// total size of hash tokens

	pSaveData = (SAVERESTOREDATA*)calloc(sizeof(SAVERESTOREDATA) + tokenSize + size + (sizeof(ENTITYTABLE) * tableCount) + SAVE_HEAPSLACK, sizeof(char));
	pSaveData->tableCount = tableCount;
	pSaveData->tokenCount = tokenCount;
	pSaveData->tokenSize = tokenSize;
	strcpy(pSaveData->szCurrentMapName, level);

	//---------------------------------
	// Parse the symbol table
	pszTokenList = (char*)(pSaveData + 1);// Skip past the CSaveRestoreData structure

	if (tokenSize > 0)
	{
		DC_fread(pszTokenList, pSaveData->tokenSize, 1, pFile);

		if (!pSaveData->pTokens)
			pSaveData->pTokens = (char**)calloc(tokenCount, sizeof(char*));

		for (i = 0; i < tokenCount; i++)
		{
			if (*pszTokenList)
				pSaveData->pTokens[i] = pszTokenList;
			else
				pSaveData->pTokens[i] = NULL;

			pszTokenList += strlen(pszTokenList) + 1;
		}
	}

	pSaveData->pTable = (ENTITYTABLE*)pszTokenList;
	pSaveData->connectionCount = 0;
	pSaveData->size = 0;

	//---------------------------------
	// Set up the restore basis
	pSaveData->pBaseData = (char*)(pszTokenList + (sizeof(ENTITYTABLE) * pSaveData->tableCount));
	pSaveData->pCurrentData = pSaveData->pBaseData;

	pSaveData->fUseLandmark = TRUE;
	pSaveData->bufferSize = size;
	pSaveData->time = 0.0;
	VectorCopy(vec3_origin, pSaveData->vecLandmarkOffset);
	gGlobalVariables.pSaveData = pSaveData;

	DC_fread(pSaveData->pBaseData, size, 1, pFile);
	Sys_CloseHandle(pFile);

	return pSaveData;
}

/*
==================
ParseSaveTables

==================
*/
void ParseSaveTables( SAVERESTOREDATA* pSaveData, SAVE_HEADER* pHeader, int updateGlobals )
{
	int				i;
	SAVELIGHTSTYLE	light;

	for (i = 0; i < pSaveData->tableCount; i++)
	{
		SaveReadFields(pSaveData, "ETABLE", &(pSaveData->pTable[i]), gEntityTableDescription, Q_ARRAYSIZE(gEntityTableDescription));
		pSaveData->pTable[i].pent = NULL;
	}

	pSaveData->pBaseData = pSaveData->pCurrentData;
	pSaveData->size = 0;

	// Process SAVE_HEADER
	SaveReadFields(pSaveData, "Save Header", pHeader, gSaveHeaderDescription, Q_ARRAYSIZE(gSaveHeaderDescription));

	pSaveData->connectionCount = pHeader->connectionCount;
	pSaveData->time = pHeader->time;
	pSaveData->fUseLandmark = TRUE;
	VectorCopy(vec3_origin, pSaveData->vecLandmarkOffset);

	// Read adjacency list
	for (i = 0; i < pSaveData->connectionCount; i++)
		SaveReadFields(pSaveData, "ADJACENCY", &(pSaveData->levelList[i]), gAdjacencyDescription, Q_ARRAYSIZE(gAdjacencyDescription));

	if (updateGlobals)
	{
		for (i = 0; i < MAX_LIGHTSTYLES; i++)
			sv.lightstyles[i] = NULL;
	}
	for (i = 0; i < pHeader->lightStyleCount; i++)
	{
		SaveReadFields(pSaveData, "LIGHTSTYLE", &light, gLightstyleDescription, Q_ARRAYSIZE(gLightstyleDescription));
		if (updateGlobals)
		{
			sv.lightstyles[light.index] = (char*)Hunk_Alloc(strlen(light.style) + 1);
			strcpy(sv.lightstyles[light.index], light.style);
		}
	}
}

/*
==================
EntityPatchWrite

Write out the list of entities that are no longer in the save file for this level
(they've been moved to another level)
==================
*/
void EntityPatchWrite( SAVERESTOREDATA* pSaveData, const char* level )
{
	char			name[128];
	bfile_t*		pFile;
	int				i, size;

	sprintf(name, "%s%s.HL3", Host_SaveGameDirectory(), level);
	COM_FixSlashes(name);

	pFile = (bfile_t*)Bopen(name, "wb");
	if (pFile)
	{
		size = 0;
		for (i = 0; i < pSaveData->tableCount; i++)
		{
			if (pSaveData->pTable[i].flags & FENTTABLE_REMOVED)
				size++;
		}
		// Patch count
		Bwrite(&size, sizeof(int), 1, pFile);
		for (i = 0; i < pSaveData->tableCount; i++)
		{
			if (pSaveData->pTable[i].flags & FENTTABLE_REMOVED)
				Bwrite(&i, sizeof(int), 1, pFile);
		}
		Bclose(pFile);
		Bcompress_path(name);
	}
}

/*
==================
EntityPatchRead

Read the list of entities that are no longer in the save file for this level (they've been moved to another level)
 and correct the table
==================
*/
void EntityPatchRead( SAVERESTOREDATA* pSaveData, const char* level )
{
	char			name[128];
	void*			pFile;
	int				i, size, entityId;

	sprintf(name, "%s%s.HL3", Host_SaveGameDirectory(), level);
	COM_FixSlashes(name);

	pFile = Sys_OpenHandle(name, "rb");
	if (pFile)
	{
		// Patch count
		DC_fread(&size, sizeof(int), 1, pFile);
		for (i = 0; i < size; i++)
		{
			DC_fread(&entityId, sizeof(int), 1, pFile);
			pSaveData->pTable[entityId].flags = FENTTABLE_REMOVED;
		}
		Sys_CloseHandle(pFile);
	}
}

/*
==================
LoadGamestate

Loads game state
==================
*/
int LoadGamestate( char* level, int createPlayers )
{
	int             i;
	SAVE_HEADER		header;
	SAVERESTOREDATA* pSaveData;
	edict_t* pent;

	pSaveData = LoadSaveData(level);
	if (!pSaveData)		// Couldn't load the file
		return 0;

	ParseSaveTables(pSaveData, &header, TRUE);
	EntityPatchRead(pSaveData, level);
	Cvar_SetValue("skill", header.skillLevel);
	strcpy(sv.name, header.mapName);

	Cvar_Set("sv_skyname", header.skyName);
	Cvar_SetValue("cl_skycolor_r", header.skyColor_r);
	Cvar_SetValue("cl_skycolor_g", header.skyColor_g);
	Cvar_SetValue("cl_skycolor_b", header.skyColor_b);
	Cvar_SetValue("cl_skyvec_x", header.skyVec_x);
	Cvar_SetValue("cl_skyvec_y", header.skyVec_y);
	Cvar_SetValue("cl_skyvec_z", header.skyVec_z);

	// Create entity list
	for (i = 0; i < pSaveData->tableCount; i++)
	{
		ENTITYTABLE* table;

		table = &pSaveData->pTable[i];
		pent = NULL;

		if (table->classname && table->size && !(table->flags & FENTTABLE_REMOVED))
		{
			if (table->id)
			{
				if (table->id < svs.maxclients + 1)
				{
					if (!(table->flags & FENTTABLE_PLAYER))
						Sys_Error("ENTITY IS NOT A PLAYER: %d\n", i);

					pent = svs.clients[table->id - 1].edict;
					if (createPlayers && pent)
						EntityInit(pent, table->classname);
					else
						pent = NULL;
				}
				else
				{
					pent = CreateNamedEntity(table->classname);
				}
			}
			else
			{
				pent = sv.edicts;
				EntityInit(pent, table->classname);
			}
		}

		table->pent = pent;
	}

	for (i = 0; i < pSaveData->tableCount; i++)
	{
		ENTITYTABLE* table;

		table = &pSaveData->pTable[i];

		pSaveData->currentIndex = i;
		pSaveData->size = table->location;
		pSaveData->pCurrentData = pSaveData->pBaseData + table->location;

		if (table->pent)
		{
			if (DispatchRestore(table->pent, pSaveData, FALSE) < 0)
			{
				ED_Free(table->pent);
				table->pent = NULL;
			}
			else
			{
				// force the entity to be relinked
				SV_LinkEdict(table->pent, FALSE);
			}
		}
	}

	SaveExit(pSaveData);
	sv.time = header.time;
	// SUCCESS!
	return 1;
}

/*
==================
EntryInTable

Find all occurances of the map in the adjacency table
==================
*/
int EntryInTable( SAVERESTOREDATA* pSaveData, const char* pMapName, int index )
{
    int i;

	index++;
    for (i = index; i < pSaveData->connectionCount; i++)
    {
        if (!strcmp(pSaveData->levelList[i].mapName, pMapName))
			return i;
    }

	return -1;
}

void LandmarkOrigin( SAVERESTOREDATA* pSaveData, vec_t* output, const char* pLandmarkName )
{
	int i;

    for (i = 0; i < pSaveData->connectionCount; i++)
    {
        if (!strcmp(pSaveData->levelList[i].landmarkName, pLandmarkName))
        {
            VectorCopy(pSaveData->levelList[i].vecLandmarkOrigin, output);
            return;
        }
    }

	VectorCopy(vec3_origin, output);
}

/*
==================
EntityInSolid

Some moved edicts on the next level can stuck outside the world, find and remove them
==================
*/
int EntityInSolid( edict_t* pent )
{
    vec3_t point;

    // always go through if we are attached to the client
	if (pent->v.movetype == MOVETYPE_FOLLOW && pent->v.aiment && (pent->v.aiment->v.flags & FL_CLIENT))
		return 0;

    point[0] = (pent->v.absmin[0] + pent->v.absmax[0]) * 0.5f;
    point[1] = (pent->v.absmin[1] + pent->v.absmax[1]) * 0.5f;
    point[2] = (pent->v.absmin[2] + pent->v.absmax[2]) * 0.5f;

	if (SV_PointContents(point) == CONTENTS_SOLID)
		return TRUE;

	return FALSE;
}

int CreateEntityList( SAVERESTOREDATA* pSaveData, int levelMask )
{
	int         i;
	int         movedCount = 0;
	int         active;
	edict_t* pent;
	ENTITYTABLE* table;

	for (i = 0; i < pSaveData->tableCount; i++)
	{
		pent = NULL;
		table = &pSaveData->pTable[i];

		if (table->classname && table->size && table->id > 0)
		{
			active = (table->flags & levelMask) != 0;

			if (table->id < svs.maxclients + 1)
			{
				pent = svs.clients[table->id - 1].edict;

				if (active)
				{
					if (!(table->flags & FENTTABLE_PLAYER))
						Sys_Error("ENTITY IS NOT A PLAYER: %d\n", i);

					if (svs.clients[table->id - 1].active && pent)
						EntityInit(pent, table->classname);
					else
						pent = NULL;
				}
			}
			else
			{
				if (active)
					pent = CreateNamedEntity(table->classname);
			}
		}

		table->pent = pent;
	}

	// Now spawn entities
	for (i = 0; i < pSaveData->tableCount; i++)
	{
		table = &pSaveData->pTable[i];

		pSaveData->currentIndex = i;
		pSaveData->size = table->location;
		pSaveData->pCurrentData = pSaveData->pBaseData + table->location;

		if (table->pent)
		{
			active = (table->flags & levelMask) != 0;

			if (active)
			{
				if (table->flags & FENTTABLE_GLOBAL)
				{
					Con_DPrintf("Merging changes for global: %s\n", &pr_strings[table->classname]);

					// Pass the "global" flag to the DLL to indicate this entity should only override
					// a matching entity, not be spawned
					DispatchRestore(table->pent, pSaveData, TRUE);
					ED_Free(table->pent);
				}
				else
				{
					Con_DPrintf("Transferring %s (%d)\n", &pr_strings[table->classname], NUM_FOR_EDICT(table->pent));

					if (DispatchRestore(table->pent, pSaveData, FALSE) < 0)
					{
						ED_Free(table->pent);
					}
					else
					{
						SV_LinkEdict(table->pent, FALSE);

						if (!(table->flags & FENTTABLE_PLAYER) && EntityInSolid(table->pent))
						{
							// this can happen during normal processing - PVS is just a guess,
							// some map areas won't exist in the new map
							Con_DPrintf("Suppressing %s\n", &pr_strings[table->classname]);
							ED_Free(table->pent);
						}
						else
						{
							table->flags = FENTTABLE_REMOVED;
							movedCount++;
						}
					}
				}
			}
		}
	}

	return movedCount;
}

void LoadAdjacentEntities( const char* pOldLevel, const char* pLandmarkName )
{
	SAVERESTOREDATA currentLevelData, * pSaveData;
	int				i, test, flags, index, movedCount = 0;
	SAVE_HEADER		header;
	vec3_t			landmarkOrigin;

	memset(&currentLevelData, 0, sizeof(currentLevelData));
	gGlobalVariables.pSaveData = &currentLevelData;
	ParmsChangeLevel();

	for (i = 0; i < currentLevelData.connectionCount; i++)
	{
		for (test = 0; test < i; test++)
		{
			// Only do maps once
			if (!strcmp(currentLevelData.levelList[i].mapName, currentLevelData.levelList[test].mapName))
				break;
		}
		// Map was already in the list
		if (test < i)
			continue;

//		Con_Printf("Merging entities from %s ( at %s )\n", currentLevelData.levelList[i].mapName, currentLevelData.levelList[i].landmarkName);
		pSaveData = LoadSaveData(currentLevelData.levelList[i].mapName);

		if (pSaveData)
		{
			ParseSaveTables(pSaveData, &header, FALSE);
			EntityPatchRead(pSaveData, currentLevelData.levelList[i].mapName);
			pSaveData->time = sv.time;// - header.time;
			pSaveData->fUseLandmark = TRUE;
			flags = 0;
			LandmarkOrigin(&currentLevelData, landmarkOrigin, pLandmarkName);
			LandmarkOrigin(pSaveData, pSaveData->vecLandmarkOffset, pLandmarkName);
			VectorSubtract(landmarkOrigin, pSaveData->vecLandmarkOffset, pSaveData->vecLandmarkOffset);
			if (!strcmp(currentLevelData.levelList[i].mapName, pOldLevel))
				flags = FENTTABLE_PLAYER;

			index = -1;
			while (1)
			{
				index = EntryInTable(pSaveData, sv.name, index);
				if (index < 0)
					break;
				flags |= (1 << index);
			}

			if (flags)
				movedCount = CreateEntityList(pSaveData, flags);

			// If ents were moved, rewrite entity table to save file
			if (movedCount)
				EntityPatchWrite(pSaveData, currentLevelData.levelList[i].mapName);

			SaveExit(pSaveData);
		}
	}
	gGlobalVariables.pSaveData = NULL;
}

int FileSize( void* pFile )
{
	int pos1, pos2;

	if (!pFile)
		return 0;

	pos1 = DC_ftell(pFile);
	DC_fseek(pFile, 0, SEEK_END);
	pos2 = DC_ftell(pFile);
	DC_fseek(pFile, pos1, SEEK_SET);
	return pos2;
}

#define FILECOPYBUFSIZE 1024

void FileCopy( void* pOutput, void* pInput, int fileSize )
{
	char	buf[FILECOPYBUFSIZE];		// A small buffer for the copy
	int		size;

	while (fileSize > 0)
	{
		if (fileSize > FILECOPYBUFSIZE)
			size = FILECOPYBUFSIZE;
		else
			size = fileSize;
		DC_fread(buf, size, 1, pInput);
		DC_fwrite(buf, size, 1, pOutput);

		fileSize -= size;
	}
}

void DirectoryCopy( const char* pPath, void* pFile )
{
	char*			pFound;
	int				fileSize;
	void*			pCopy;
	char			szName[MAX_PATH];
	char			fileName[MAX_PATH];

	pFound = Bfind_first((char*)pPath, fileName);
	while (pFound)
	{
		// The gamestate for the level being saved is written out on its own
		if (!strstr(pFound, ".hl1"))
		{
			sprintf(szName, "%s%s", Host_SaveGameDirectory(), pFound);
			COM_FixSlashes(szName);
			pCopy = Sys_OpenHandle(szName, "rb");
			fileSize = Bfilesize_path(szName);
			DC_fwrite(pFound, sizeof(char), MAX_PATH, pFile);		// Filename can only be as long as a map name + extension
			DC_fwrite(&fileSize, sizeof(int), 1, pFile);
			FileCopy(pFile, pCopy, fileSize);
			Sys_CloseHandle(pCopy);
		}

		// Any more save files?
		pFound = Bfind_next(fileName);
	}

	Bfind_reset();
}

void DirectoryExtract( void* pFile, int fileCount )
{
    int				i, fileSize;
    void* pCopy;
    char			szName[MAX_PATH], fileName[MAX_PATH];

	for (i = 0; i < fileCount; i++)
	{
		DC_fread(fileName, sizeof(char), MAX_PATH, pFile);		// Filename can only be as long as a map name + extension
		DC_fread(&fileSize, sizeof(int), 1, pFile);
		sprintf(szName, "%s%s", Host_SaveGameDirectory(), fileName);
		COM_FixSlashes(szName);
		pCopy = (void*)Bopen(szName, "wb");
		FileCopy(pCopy, pFile, fileSize);
		Sys_CloseHandle(pCopy);

		// The landmark table has to stay readable for the level transition
		if (!strstr(szName, ".hl4"))
			Bcompress_path(szName);
	}
}

int DirectoryCount( const char* pPath )
{
	int		count;
	char*	pFound;

	count = 0;
	pFound = Bfind_first((char*)pPath, NULL);
	while (pFound)
	{
		count++;
		// Any more save files
		pFound = Bfind_next(NULL);
	}
	Bfind_reset();

	return count;
}

void DirectoryClear( const char* pPath )
{
	char	szName[MAX_PATH];
	char*	pFound;

	pFound = Bfind_first((char*)pPath, NULL);
	while (pFound)
	{
		sprintf(szName, "%s%s", Host_SaveGameDirectory(), pFound);
		Bremove_path(szName);

		// Any more save files
		pFound = Bfind_next(NULL);
	}
	Bfind_reset();
}

/*
==================
Host_ClearSaveDirectory

==================
*/
void Host_ClearSaveDirectory( void )
{
	char			szName[128];

	sprintf(szName, "%s", Host_SaveGameDirectory());
	COM_FixSlashes(szName);
	// Create save directory if it doesn't exist
	Sys_mkdir(szName);

	strcat(szName, "*.HL?");
	DirectoryClear(szName);
}

void Host_ClearGameState( void )
{
	S_StopAllSounds(TRUE);
	S_ClearBuffer(TRUE);
	Host_ClearSaveDirectory();

	ResetGlobalState();
}

/*
==================
Host_Changelevel_f

Changing levels within a unit, uses save/restore
==================
*/
void Host_Changelevel2_f( void )
{
	char	level[MAX_QPATH];
	char    oldlevel[MAX_QPATH];
	char    _startspot[MAX_QPATH];
	char* startspot;
	SAVERESTOREDATA* pSaveData;
	qboolean newUnit;

	giActive = DLL_TRANS;

	newUnit = FALSE;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}

	if (!sv.active || sv.paused)
	{
		Con_Printf("Only the server may changelevel\n");
		return;
	}

	SCR_BeginLoadingPlaque();

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);

	strcpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		strcpy(_startspot, Cmd_Argv(2));
		if (_startspot[0] == '\0')
			startspot = NULL;
		else
			startspot = _startspot;
	}

	strcpy(oldlevel, sv.name);

	// save the current level's state
	pSaveData = SaveGamestate();

	if (!SV_SpawnServer(FALSE, level, startspot))
	{
		Sys_Error("Couldn't load map %s\n", level);
		return;
	}

	SaveExit(pSaveData);

	// try to restore the new level
	if (!LoadGamestate(level, FALSE))
	{
		newUnit = TRUE;
		SV_LoadEntities();
	}

	LoadAdjacentEntities(oldlevel, startspot);

	sv.paused = TRUE;
	sv.loadgame = TRUE;
	gGlobalVariables.time = sv.time;

	if (newUnit && sv_newunit.value)
		Host_ClearSaveDirectory();

	SV_ActivateServer(FALSE);
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f( void )
{
	char* newName;

	if (Cmd_Argc() == 1)
	{
		Con_Printf("\"name\" is \"%s\"\n", cl_name.string);
		return;
	}
	if (Cmd_Argc() == 2)
		newName = Cmd_Argv(1);
	else
		newName = Cmd_Args();

	if (!newName || !newName[0])
	{
		Con_Printf("Usage:  name <name>\n");
		return;
	}

	newName[15] = 0;

	if (cmd_source == src_command)
	{
		if (!Q_strcmp(cl_name.string, newName))
			return;

		Cvar_Set("_cl_name", newName);

		Host_ClearSaveDirectory();

		if (cls.state == ca_connected || cls.state == ca_uninitialized || cls.state == ca_active)
			Cmd_ForwardToServer();
		return;
	}

	if (host_client->name[0] && strcmp(host_client->name, "unconnected"))
	{
		if (Q_strcmp(host_client->name, newName) != 0)
			Con_Printf("%s renamed to %s\n", host_client->name, newName);
	}

	Q_strcpy(host_client->name, newName);
	host_client->edict->v.netname = host_client->name - pr_strings;

// send notification to all clients

	MSG_WriteByte(&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteString(&sv.reliable_datagram, host_client->name);
}

void Host_Version_f( void )
{
	Con_Printf("Protocol version %i\nExe version %s\n", PROTOCOL_VERSION, gpszVersionString);
	Con_Printf("Exe build: " __TIME__ " " __DATE__ "\n", build_number());
}

void Host_Say( qboolean teamonly )
{
	client_t* client;
	client_t* save;
	int			j;
	char* p;
	char		text[64];

	if (cls.state != ca_dedicated)
	{
		if (cmd_source == src_command)
			Cmd_ForwardToServer();  // this will only happen if the game engine handles the say command instead of the entity dll;  that shouldn't happen

		return;
	}

	if (Cmd_Argc() < 2)
		return;

	p = Cmd_Args();
	if (!p)
		return;

	save = host_client;

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p) - 1] = 0;
	}

	sprintf(text, "%c<%s> ", 1, host_name.string);

	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;

		host_client = client;
		PF_MessageBegin_I(MSG_ONE, RegUserMsg("SayText", -1), NULL, &sv.edicts[j + 1]);
		PF_WriteByte_I(0);
		PF_WriteString_I(text);
		PF_MessageEnd_I();
	}
	host_client = save;

	Sys_Printf("%s", &text[1]);
}


void Host_Say_f( void )
{
	Host_Say(FALSE);
}


void Host_Say_Team_f( void )
{
	Host_Say(TRUE);
}


void Host_Tell_f( void )
{
	client_t* client;
	client_t* save;
	int		j;
	char* p;
	char	text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (Cmd_Argc() < 3)
		return;

	Q_strcpy(text, host_client->name);
	Q_strcat(text, ": ");

	p = Cmd_Args();
	if (!p)
		return;

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p) - 1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
			continue;
		if (Q_strcasecmp(client->name, Cmd_Argv(1)))
			continue;
		host_client = client;
		SV_ClientPrintf("%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f( void )
{
	int		top, bottom;
	int		playercolor;

	if (Cmd_Argc() == 1)
	{
		Con_Printf("\"color\" is \"%i %i\"\n", ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
		Con_Printf("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;

	playercolor = top * 16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValue("_cl_color", playercolor);
		if (cls.state == ca_connected || cls.state == ca_uninitialized || cls.state == ca_active)
			Cmd_ForwardToServer();
		return;
	}

	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;

// send notification to all clients
	MSG_WriteByte(&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte(&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintf("Can't suicide -- allready dead!\n");
		return;
	}

	gGlobalVariables.time = sv.time;
	ClientKill(sv_player);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f( void )
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!pausable.value)
		SV_ClientPrintf("Pause not allowed.\n");
	else
	{
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf("%s paused the game\n", pr_strings + sv_player->v.netname);
		}
		else
		{
			SV_BroadcastPrintf("%s unpaused the game\n", pr_strings + sv_player->v.netname);
		}

	// send notification to all clients
		MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
		MSG_WriteByte(&sv.reliable_datagram, sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f( void )
{
	int i;

	if (cmd_source == src_command)
	{
		Con_Printf("prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf("prespawn not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		Con_Printf("SV_PreSpawn_f from different level\n");
		SV_New_f();
		return;
	}

	i = atoi(Cmd_Argv(2));
	if (i >= sv.num_signon_buffers)
		i = 0;

	SZ_Write(&host_client->netchan.message, sv.signon_buffers[i], sv.signon_buffer_size[i]);
	i++;

	if (i == sv.num_signon_buffers)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_signonnum);
		MSG_WriteByte(&host_client->netchan.message, 1);
		host_client->sendsignon = TRUE;
	}
	else
	{
		MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
		MSG_WriteString(&host_client->netchan.message, va("cmd prespawn %i %i\n", svs.spawncount, i));
	}
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f( void )
{
	int		i;
	client_t* client;
	edict_t* ent;
	char	name[256];
	SAVERESTOREDATA currentLevelData;

	if (cmd_source == src_command)
	{
		Con_Printf("spawn is not valid from the console\n");
		return;
	}

	if (!host_client->connected)
	{
		Con_Printf("Spawn not valid -- already spawned\n");
		return;
	}

	if (atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		Con_Printf("SV_Spawn_f from different level\n");
		SV_New_f();
		return;
	}

// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited already
		// if this is the last client to be connected, unpause
		sv.paused = FALSE;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;

		// we're spawning
		sv.state = ss_loading;

		ReleaseEntityDLLFields(ent);
		memset(&ent->v, 0, sizeof(ent->v));
		InitEntityDLLFields(ent);
		ent->v.colormap = NUM_FOR_EDICT(ent);
		ent->v.team = (host_client->colors & 15) + 1;
		ent->v.netname = host_client->name - pr_strings;

		// make sure the time is set
		gGlobalVariables.time = sv.time;

		// call the spawn function
		ClientPutInServer(sv_player);

		// all setup is completed, any further precache statements are errors
		sv.state = ss_active;
	}

// send all current names, colors, and frag counts
	SZ_Clear(&host_client->netchan.message);

// send time of update
	MSG_WriteByte(&host_client->netchan.message, svc_time);
	MSG_WriteFloat(&host_client->netchan.message, sv.time);

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_updatename);
		MSG_WriteByte(&host_client->netchan.message, i);
		MSG_WriteString(&host_client->netchan.message, client->name);
		MSG_WriteByte(&host_client->netchan.message, svc_updatecolors);
		MSG_WriteByte(&host_client->netchan.message, i);
		MSG_WriteByte(&host_client->netchan.message, client->colors);

		if (host_client->maxspeed)
		{
			MSG_WriteByte(&host_client->netchan.message, svc_clientmaxspeed);
			MSG_WriteByte(&host_client->netchan.message, i);
			MSG_WriteByte(&host_client->netchan.message, client->maxspeed);
		}
	}

// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_lightstyle);
		MSG_WriteByte(&host_client->netchan.message, (char)i);
		MSG_WriteString(&host_client->netchan.message, sv.lightstyles[i]);
	}

//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = &sv.edicts[1 + (host_client - svs.clients)];
	MSG_WriteByte(&host_client->netchan.message, svc_setangle);
	for (i = 0; i < 2; i++)
		MSG_WriteHiresAngle(&host_client->netchan.message, ent->v.v_angle[i]);
	MSG_WriteHiresAngle(&host_client->netchan.message, 0);

	SV_WriteClientdataToMessage(host_client, &host_client->netchan.message);

	MSG_WriteByte(&host_client->netchan.message, svc_signonnum);
	MSG_WriteByte(&host_client->netchan.message, 2);
	host_client->sendsignon = TRUE;

	if (sv.loadgame)
	{
		memset(&currentLevelData, 0, sizeof(currentLevelData));
		gGlobalVariables.pSaveData = &currentLevelData;

		ParmsChangeLevel();

		MSG_WriteByte(&host_client->netchan.message, svc_restore);
		sprintf(name, "%s%s.HL2", Host_SaveGameDirectory(), sv.name);
		COM_FixSlashes(name);
		MSG_WriteString(&host_client->netchan.message, name);
		MSG_WriteByte(&host_client->netchan.message, currentLevelData.connectionCount);
		for (i = 0; i < currentLevelData.connectionCount; i++)
		{
			MSG_WriteString(&host_client->netchan.message, currentLevelData.levelList[i].mapName);
		}

		// Reset
		sv.loadgame = FALSE;
		gGlobalVariables.pSaveData = NULL;
	}
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f( void )
{
	if (cmd_source == src_command)
	{
		Con_Printf("begin is not valid from the console\n");
		return;
	}

	host_client->active = TRUE;
	host_client->connected = FALSE;
	host_client->netchan.frame_latency = 0.0;
	host_client->netchan.frame_rate = 0.0;
	host_client->netchan.drop_count = 0;
	host_client->netchan.good_count = 0;
	host_client->spawned = TRUE;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f( void )
{
	char* who;
	char* message = NULL;
	char* pName;
	client_t* save;
	int			i;
	int			userid;
	qboolean	byNumber;

	if (Cmd_Argc() <= 1)
	{
		Con_Printf("usage:  kick < name > | < # userid >\n");
		return;
	}

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer();
			return;
		}
	}
	else if (host_client->netchan.remote_address.type != NA_LOOPBACK)
	{
		SV_ClientPrintf("You can't 'kick' because you are not a server operator\n");
		return;
	}

	save = host_client;

	pName = Cmd_Argv(1);
	if (pName && *pName == '#')
	{
		if (Cmd_Argc() > 2)
			userid = Q_atoi(Cmd_Argv(2));
		else
			userid = Q_atoi(pName + 1);

		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active && !host_client->connected)
				continue;
			if (host_client->userid == userid)
				break;
		}
		byNumber = TRUE;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active && !host_client->connected)
				continue;
			if (Q_strcasecmp(host_client->name, Cmd_Argv(1)) == 0)
				break;
		}
		byNumber = FALSE;
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
		{
			if (cls.state == ca_dedicated)
				who = "Console";
			else
				who = cl_name.string;
		}
		else
			who = save->name;

		// the person running the game can't be dropped
		if (host_client->netchan.remote_address.type == NA_LOOPBACK)
		{
			Con_Printf("The local player cannot be kicked!\n");
			return;
		}

		if (Cmd_Argc() > 2)
		{
			message = COM_Parse(Cmd_Args());
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += Q_strlen(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_ClientPrintf("Kicked by %s: %s\n", who, message);
		else
			SV_ClientPrintf("Kicked by %s\n", who);
		SV_DropClient(host_client, FALSE);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

edict_t* FindViewthing( void )
{
    int		i;
    edict_t* e;

    for (i = 0; i < sv.num_edicts; i++)
    {
		e = &sv.edicts[i];
		if (!strcmp(pr_strings + e->v.classname, "viewthing"))
			return e;
    }
	Con_Printf("No viewthing on map\n");
    return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f( void )
{
	edict_t* e;
	model_t* m;

	e = FindViewthing();
	if (!e)
		return;

	m = Mod_ForName(Cmd_Argv(1), FALSE);
	if (!m)
	{
		Con_Printf("Can't load %s\n", Cmd_Argv(1));
		return;
	}
	
	e->v.frame = 0;
	cl.model_precache[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f( void )
{
	edict_t* e;
	int		f;
	model_t* m;

	e = FindViewthing();
	if (!e)
		return;	
	m = cl.model_precache[(int)e->v.modelindex];

	f = atoi(Cmd_Argv(1));
	if (f >= m->numframes)
		f = m->numframes - 1;

	e->v.frame = f;
}


void PrintFrameName( model_t* m, int frame )
{
    aliashdr_t* hdr;
    maliasframedesc_t* pframedesc;

	hdr = (aliashdr_t*)Mod_Extradata(m);
    if (!hdr)
        return;
    pframedesc = &hdr->frames[frame];

	Con_Printf("frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f( void )
{
	edict_t* e;
	model_t* m;

	e = FindViewthing();
	if (!e)
		return;
	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame + 1;
	if (e->v.frame >= m->numframes)
		e->v.frame = m->numframes - 1;

	PrintFrameName(m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f( void )
{
	edict_t* e;
	model_t* m;

	e = FindViewthing();
	if (!e)
		return;

	m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame - 1;
	if (e->v.frame < 0)
		e->v.frame = 0;

	PrintFrameName(m, e->v.frame);
}

/*
==================
Host_Interp_f

Enable frame interpolation
==================
*/
void Host_Interp_f( void )
{
	r_dointerp ^= 1;

	if (!r_dointerp)
		Con_Printf("Frame Interpolation OFF\n");
	else
		Con_Printf("Frame Interpolation ON\n");	
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f( void )
{
	int		i, c;

	if (cls.state == ca_dedicated)
	{
		if (!sv.active)
			Con_Printf("Cannot play demos on a dedicated server.\n");
		return;
	}

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		c = MAX_DEMOS;
		Con_Printf("Max %i demos in demoloop\n", MAX_DEMOS);
	}
	Con_Printf("%i demo(s) in loop\n", c);

	for (i = 1; i < c + 1; i++)
		strncpy(cls.demos[i - 1], Cmd_Argv(i), sizeof(cls.demos[0]) - 1);

	if (!sv.active && cls.demonum != -1)
	{
		cls.demonum = 0;
		CL_NextDemo();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f( void )
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 0;
	CL_Disconnect_f();
	CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f( void )
{
	if (cls.state == ca_dedicated)
		return;
}

//=============================================================================

svchannel_t* svchannels;

/*
==================
SV_CheckChannel

Check if the server room exists
==================
*/
qboolean SV_CheckChannel( char* pszChannel )
{
	svchannel_t* pChannel;

	if (!pszChannel || !pszChannel[0])
		return FALSE;

	pChannel = svchannels;
	while (pChannel)
	{
		if (!_stricmp(pChannel->szServerChannel, pszChannel))
			return TRUE;

		pChannel = pChannel->pNext;
	}

	return FALSE;
}

/*
==================
SV_AddChannel_f

Add server room
==================
*/
void SV_AddChannel_f( void )
{
	int i;
	qboolean bFound;
	svchannel_t* pChannel;

	if (Cmd_Argc() == 2)
	{
		bFound = FALSE;

		// See if this channel already exists
		i = 0;
		pChannel = svchannels;
		while (pChannel)
		{
			i++;
			if (!_stricmp(pChannel->szServerChannel, Cmd_Argv(1)))
			{
				bFound = TRUE;
				strncpy(pChannel->szServerChannel, Cmd_Argv(1), sizeof(pChannel->szServerChannel));
				pChannel->szServerChannel[sizeof(pChannel->szServerChannel) - 1] = 0;
				break;
			}

			pChannel = pChannel->pNext;
		}

		if (!bFound)
		{
			if (i >= MAX_SVCHANNELS)
				return;

			pChannel = (svchannel_t*)malloc(sizeof(svchannel_t));
			if (!pChannel)
				Sys_Error("Failed to allocate channel!");
			memset(pChannel, 0, 64);
			strncpy(pChannel->szServerChannel, Cmd_Argv(1), sizeof(pChannel->szServerChannel));
			pChannel->szServerChannel[sizeof(pChannel->szServerChannel) - 1] = 0;
			pChannel->bIsDefault = FALSE;
			pChannel->pNext = svchannels;
			svchannels = pChannel;
		}

		gfLastHearbeat = -99999;
	}
	else
	{
		Con_Printf("svaddchannel:  Adds server room (16 chars max)\ncurrent:  \n");

		if (!svchannels)
		{
			Con_Printf("none\n");
			return;
		}

		i = 0;
		pChannel = svchannels;
		while (pChannel)
		{
			i++;
			if (pChannel->bIsDefault)
				Con_Printf("  %i : %s (default)\n", i, pChannel);
			else
				Con_Printf("  %i : %s\n", i, pChannel);

			pChannel = pChannel->pNext;
		}
	}
}

/*
==================
SV_RemoveChannel_f

Remove server room
==================
*/
void SV_RemoveChannel_f( void )
{
	qboolean bFound;
	svchannel_t* pChannel, * pPrev;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("svremovechannel:  Removes server room (16 chars max)\n");
		return;
	}

	bFound = FALSE;

	pChannel = svchannels;
	while (pChannel)
	{
		if (!_stricmp(pChannel->szServerChannel, Cmd_Argv(1)))
		{
			bFound = TRUE;
			break;
		}

		pChannel = pChannel->pNext;
	}

	if (!bFound || !pChannel)
	{
		Con_Printf("Unknown server:  %s\n", Cmd_Argv(1));
		return;
	}

	if (pChannel->bIsDefault)
	{
		Con_Printf("Can't delete default server:  %s\n", Cmd_Argv(1));
		return;
	}

	// Remove from linked list
	if (pChannel == svchannels)
	{
		svchannels = svchannels->pNext;
	}
	else
	{
		pPrev = svchannels;
		while (pPrev->pNext != pChannel)
		{
			pPrev = pPrev->pNext;
		}
		pPrev->pNext = pChannel->pNext;
	}
	free(pChannel);

	gfLastHearbeat = -99999;
}

/*
==================
SV_ClearChannels

==================
*/
void SV_ClearChannels( qboolean bLeaveDefault )
{
	svchannel_t* pChannel;
	svchannel_t* pNext;
	svchannel_t* pNewList = NULL;

	pChannel = svchannels;
	while (pChannel)
	{
		pNext = pChannel->pNext;

		if (pChannel->bIsDefault && bLeaveDefault)
		{
			pChannel->pNext = pNewList;
			pNewList = pChannel;
		}
		else
		{
			free(pChannel);
		}

		pChannel = pNext;
	}

	if (pNewList)
		svchannels = pNewList;
	else
		svchannels = NULL;
}

/*
==================
SV_ClearChannels_f

==================
*/
void SV_ClearChannels_f( void )
{
	if (Cmd_Argc() != 1)
	{
		Con_Printf("svclearchannels:  Clears all server room associations (except default)\n");
		return;
	}
	
	SV_ClearChannels(TRUE);
	gfLastHearbeat = -99999;
}

/*
==================
SV_NextDownload_f

Sends next file chunk to client. Called automatically during downloads.
Packets contain: file data (1024b chunks), progress %, and CRC checksum.
==================
*/
void SV_NextDownload_f( void )
{
	int		r;
	int		percent;
	int		size;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!host_client->download)
		return;

	r = host_client->downloadsize - host_client->downloadpos;
	if (r > 1024)
		r = 1024;
	CRC32_ProcessBuffer(&host_client->downloadCRC, host_client->download + host_client->downloadpos, r);

	// Send download info packet
	MSG_WriteByte(&host_client->netchan.message, svc_download);
	MSG_WriteShort(&host_client->netchan.message, r);
	MSG_WriteShort(&host_client->netchan.message, host_client->downloadpos / 1024);
	MSG_WriteLong(&host_client->netchan.message, host_client->downloadCRC);

	host_client->downloadpos += r;
	size = host_client->downloadsize;
	if (!size)
		size = 1;
	percent = host_client->downloadpos * 100 / size;
	MSG_WriteByte(&host_client->netchan.message, percent);
	SZ_Write(&host_client->netchan.message, &host_client->download[host_client->downloadpos - r], r);

	if (host_client->downloadpos != host_client->downloadsize)
		return;

	COM_FreeFile(host_client->download);
	host_client->download = NULL;
}

/*
==================
SV_SetupResume

==================
*/
void SV_SetupResume( int size, CRC32_t crc )
{
	CRC32_t crcFile;

	if (size < 0)
		return;

	size *= 1024;
	if (host_client->downloadsize < size)
		return;

	CRC32_Init(&crcFile);
	CRC32_ProcessBuffer(&crcFile, host_client->download, size);
	crcFile = CRC32_Final(crcFile);
	if (crcFile == crc)
	{
		host_client->downloadpos = size;
		host_client->downloadCRC = crc;
		host_client->downloading = TRUE;
	}
}

/*
==================
SV_AllowDownload_f

==================
*/
void SV_AllowDownload_f( void )
{
	sv_allow_download->value = !sv_allow_download->value;

	if (sv_allow_download->value)
		Con_Printf("Server downloading enabled.\n");
	else
		Con_Printf("Server downloading disabled.\n");
}

/*
==================
SV_AllowUpload_f

==================
*/
void SV_AllowUpload_f( void )
{
	sv_allow_upload->value = !sv_allow_upload->value;

	if (sv_allow_upload->value)
		Con_Printf("Server uploading enabled.\nMax. upload size is %i", sv_upload_maxsize.name);
	else
		Con_Printf("Server uploading disabled.\n");
}

/*
==================
COM_Nibble

Returns the 4 bit nibble for a hex character
==================
*/
unsigned char COM_Nibble( char c )
{
	if ((c >= '0') && (c <= '9'))
		return (unsigned char)(c - '0');

	if ((c >= 'A') && (c <= 'F'))
		return (unsigned char)(c - 'A' + 0x0a);

	if ((c >= 'a') && (c <= 'f'))
		return (unsigned char)(c - 'a' + 0x0a);

	return c;
}

/*
==================
SV_BeginDownload_f

Starts file download to client, handles both normal files and MD5-hashed resources
==================
*/
void SV_BeginDownload_f( void )
{
	char* name;
	FILE* file;

	name = Cmd_Argv(1);

	if (cmd_source == src_command)
	{
		CL_CheckOrDownloadFile(name);
		return;
	}

	if (strstr(name, "..") || !sv_allow_download->value)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_download);
		MSG_WriteShort(&host_client->netchan.message, -1);
		MSG_WriteShort(&host_client->netchan.message, -1);
		MSG_WriteLong(&host_client->netchan.message, -1);
		MSG_WriteByte(&host_client->netchan.message, 0);
		return;
	}

	if (host_client->download)
	{
		COM_FreeFile(host_client->download);
		host_client->download = NULL;
	}

	file = NULL;

	// Handle customizations
	if (strlen(name) == 36 && !_strnicmp(name, "!MD5", 4))
	{
		resource_t resource;
		unsigned char rgucMD5_hash[16];

		memset(&resource, 0, sizeof(resource));

		COM_HexConvert(name + 4, 32, rgucMD5_hash);

		if (HPAK_ResourceForHash(HASHPAK_FILENAME, rgucMD5_hash, &resource))
		{
			HPAK_GetDataPointer(HASHPAK_FILENAME, &resource, (void**)&host_client->download, &host_client->downloadsize);
		}
	}
	else
	{
		host_client->downloadsize = COM_FindFile(name, NULL, &file);
		if (host_client->downloadsize != -1 && file)
		{
			host_client->download = COM_LoadFile(name, 5, NULL);
			fclose(file);
			file = NULL;
		}
	}

	host_client->downloadpos = 0;

	if (host_client->downloadsize == -1 || !host_client->download)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_download);
		MSG_WriteShort(&host_client->netchan.message, -1);
		MSG_WriteShort(&host_client->netchan.message, -1);
		MSG_WriteLong(&host_client->netchan.message, -2);
		MSG_WriteByte(&host_client->netchan.message, 0);
		return;
	}

	host_client->downloading = FALSE;

	CRC32_Init(&host_client->downloadCRC);

	if (Cmd_Argc() == 4)
	{
		SV_SetupResume(atoi(Cmd_Argv(2)), atol(Cmd_Argv(3)));
	}

	SV_NextDownload_f();
	Con_DPrintf("Downloading %s to %s\n", name, host_client->name);
}

void Host_Reactivate_f( void )
{
	
}

/*
==============================
Host_EndSection

Signals the engine that a section has ended
Possible values:
	_oem_end_training
	_oem_end_logo
	_oem_end_demo
==============================
*/
void Host_EndSection( const char* pszSection )
{
	Sleep(500);
	Cbuf_AddText("\ndisconnect\nmenu main\n");
}

/*
==================
Host_WC_f

Switch the main window to worldcraft
==================
*/
void Host_WC_f( void )
{
}

/*
==================
Host_Soundfade_f

==================
*/
void Host_Soundfade_f( void )
{
	int percent;
	int inTime, holdTime, outTime;

	if (Cmd_Argc() != 3 && Cmd_Argc() != 5)
	{
		Con_Printf("soundfade <percent> <hold> [<out> <int>]\n");
		return;
	}

	percent = atoi(Cmd_Argv(1));
	percent = min(percent, 100);
	percent = max(percent, 0);

	holdTime = atoi(Cmd_Argv(2));
	if (holdTime > 255)
		holdTime = 255;

	inTime = 0;
	outTime = 0;
	if (Cmd_Argc() == 5)
	{
		outTime = atoi(Cmd_Argv(3));
		if (outTime > 255)
			outTime = 255;

		inTime = atoi(Cmd_Argv(4));
		if (inTime > 255)
			inTime = 255;
	}

	cls.soundfade.nStartPercent = percent;
	cls.soundfade.soundFadeStartTime = realtime;
	cls.soundfade.soundFadeOutTime = outTime;
	cls.soundfade.soundFadeHoldTime = holdTime;
	cls.soundfade.soundFadeInTime = inTime;
}

/*
==================
Host_KillServer_f

==================
*/
void Host_KillServer_f( void )
{
	qboolean active;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	active = cls.state != ca_dedicated;

	if (active)
	{
		CL_Disconnect_f();
		return;
	}

	if (sv.active)
	{
		Host_ShutdownServer(FALSE);
		active = cls.state != ca_dedicated;
	}

	if (active)
		NET_Config(FALSE);
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands( void )
{
	Cmd_AddCommand("killserver", Host_KillServer_f);
	Cmd_AddCommand("soundfade", Host_Soundfade_f);
	Cmd_AddCommand("wc", Host_WC_f);
	Cmd_AddCommand("status", Host_Status_f);
	Cmd_AddCommand("quit", Host_Quit_f);
	Cmd_AddCommand("exit", Host_Quit_f);
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("maps", Host_Maps_f);
	Cmd_AddCommand("restart", Host_Restart_f);
	Cmd_AddCommand("reload", Host_Reload_f);
	Cmd_AddCommand("changelevel", Host_Changelevel_f);
	Cmd_AddCommand("changelevel2", Host_Changelevel2_f);
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("version", Host_Version_f);
	Cmd_AddCommand("say", Host_Say_f);
	Cmd_AddCommand("say_team", Host_Say_Team_f);
	Cmd_AddCommand("tell", Host_Tell_f);
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
	Cmd_AddCommand("spawn", Host_Spawn_f);
	Cmd_AddCommand("begin", Host_Begin_f);
	Cmd_AddCommand("prespawn", Host_PreSpawn_f);
	Cmd_AddCommand("kick", Host_Kick_f);
	Cmd_AddCommand("ping", Host_Ping_f);
	Cmd_AddCommand("load", Host_Loadgame_f);
	Cmd_AddCommand("save", Host_Savegame_f);
	Cmd_AddCommand("autosave", Host_AutoSave_f);

	Cmd_AddCommand("startdemos", Host_Startdemos_f);
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand("reactivate", Host_Reactivate_f);
	Cmd_AddCommand("ptrack", SV_PTrack_f);
	Cmd_AddCommand("customrsrclist", SV_RequestResourceList_f);
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
	Cmd_AddCommand("fly", Host_Fly_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
	Cmd_AddCommand("spectate", Host_Spectate_f);

	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);

	Cmd_AddCommand("mcache", Mod_Print);

	Cmd_AddCommand("interp", Host_Interp_f);
	Cmd_AddCommand("setmaster", Master_SetMaster_f);
	Cmd_AddCommand("heartbeat", Master_Heartbeat_f);
	Cmd_AddCommand("svaddchannel", SV_AddChannel_f);
	Cmd_AddCommand("motd", Master_RequestMOTD_f);
	Cmd_AddCommand("svremovechannel", SV_RemoveChannel_f);
	Cmd_AddCommand("svclearchannels", SV_ClearChannels_f);
	Cmd_AddCommand("sv_print_custom", SV_PrintCusomizations_f);
	Cmd_AddCommand("addip", SV_AddIP_f);
	Cmd_AddCommand("removeip", SV_RemoveIP_f);
	Cmd_AddCommand("listip", SV_ListIP_f);
	Cmd_AddCommand("writeip", SV_WriteIP_f);
	Cmd_AddCommand("mem_prediction", SV_MemPrediction_f);
	Cmd_AddCommand("download", SV_BeginDownload_f);
	Cmd_AddCommand("nextdl", SV_NextDownload_f);
	Cmd_AddCommand("sv_allow_download", SV_AllowDownload_f);
	Cmd_AddCommand("sv_allow_upload", SV_AllowUpload_f);
	Cmd_AddCommand("resourcelist", SV_SendResourceListBlock_f);

	Cvar_RegisterVariable(&rcon_password);
	Cvar_RegisterVariable(&filterban);

	Cmd_AddCommand("new", SV_New_f);
	Cmd_AddCommand("dropclient", SV_Drop_f);

	Cvar_RegisterVariable(&gHostMap);

	Cmd_AddCommand("keys", SV_Keys_f);

	Host_ClearSaveDirectory();
}

//=============================================================================

// Controller-status message drawn over the HUD.
static char		hostMessage[80];
static float	hostMessageTime;

/*
==================
Host_SetMessage
==================
*/
void Host_SetMessage( char* pszMessage )
{
	strcpy(hostMessage, Text_FindString(pszMessage));
	hostMessageTime = 0;
}

/*
==================
Host_CheckController

Pause the game and post a message while the controller is unplugged.
==================
*/
void Host_CheckController( void )
{
	// TODO: pause and post "#check_controller"/"#no_controller" while the
	// controller is missing, unpause once it returns
}

/*
==================
Host_DrawMessage

Draw the host message centered above the status bar, fading it out after a
few seconds.
==================
*/
void Host_DrawMessage( void )
{
	// TODO: draw hostMessage with fade-out
}
#include "quakedef.h"
#include "winquake.h"
#include "pr_cmds.h"
#include "cl_demo.h"
#include "decal.h"
#include "hashpak.h"
#include "r_studio.h"
#include "pr_edict.h"
#include "tmessage.h"
#include "kzap.h"
#include "vmu.h"
#include "ui.h"
#include "text_draw.h"
#include "won.h"

int	current_skill;
int	gHostSpawnCount = 0;


// Cheat page of the debug menu. Each of these points at the menu item's own
// value, so the poller can see when the player flips one and issue the command
// that brings the game back in step.
int*		gpCheatFly;
int*		gpCheatNoclip;
int*		gpCheatNotarget;
int*		gpCheatGod;
int*		gpCheatSlomo;
int*		gpCheatAllies;
int*		gpCheatAmmo;
int*		gpCheatWeapons;
int*		gpCheatEverything;
int*		gpCheatHealth;
int*		gpCheatUnused;
int*		gpCheatGravity;

// What the give lists still owe the player, and where the weapon list is up to
int			gGiveHealth;
int			gGiveItems;
int			gGiveWeapon;
int			gGiving;

extern int		gAlliesFriendly;
extern cvar_t	host_framerate;

// Whether the slow-motion debug framerate is on.
int		gSlomo;

// Save the skill-select screen is about to load, and whether the game resumed
// into the middle of a chapter rather than its opening.
char	gszPreloadSkill[64];
int		gResumedSave;



extern cvar_t	sv_lan;

extern cvar_t*	sv_allow_download;
extern cvar_t*	sv_allow_upload;

// Slack at the end of a save block. The token list is walked one step past its
// last terminator and the data buffer is rounded up to four bytes, so the block
// needs a little more room than the sizes stored in the file account for.
#define SAVE_HEAPSLACK	32
#define SAVE_TOKEN_COUNT	0xfff
#define SAVE_TOKEN_EXTRA	8

// Scratch path a packed save is expanded into before it is read back
#define UNZIP_TEMP_FILE	"\\CD-ROM\\valve\\SAVE\\UnzipTmp.sdj"

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

cvar_t gHostMap = { "HostMap", "C0A0" };

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

	return !strcmp(Cmd_Argv(1), rcon_password.string);
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
		Log_Printf("Rcon from \"%s\":\"(%s)\"\n", NET_AdrToString(*net_from), net_message.data + 4);
	}
	else
	{
		Con_Printf("Bad rcon from %s:\n%s\n", NET_AdrToString(*net_from), net_message.data + 4);
		Log_Printf("Bad Rcon from \"%s\":\"(%s)\"\n", NET_AdrToString(*net_from), net_message.data + 4);
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
Host_Status_f
==================
*/
void Host_Status_f( void )
{
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
	print("version :  %s\n", gpszVersionString);
	print("build   :  Dreamcast %d\n", build_number());

	if (!noip)
		print("tcp/ip  :  %s\n", NET_AdrToString(net_local_adr));

	if (!noipx)
		print("ipx     :  %s\n", NET_AdrToString(net_local_ipx_adr));

	print(" map     :  %s at: %d x, %d y, %d z\n", sv.name,
		(int)r_origin[0], (int)r_origin[1], (int)r_origin[2]);

	SV_CountPlayers(&players, &spectators);

	if (spectators)
		print(" players: %i active (%i spectators) (%i max)\n\n", players, spectators, svs.maxclients);
	else
		print(" players: %i active (%i max)\n\n", players, svs.maxclients);
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

/*
==================
Cmd_slomo_f

Toggle the slow-motion debug framerate
==================
*/
void Cmd_slomo_f( void )
{
	gSlomo = !gSlomo;

	if (gSlomo)
		Cbuf_AddText("host_framerate 0.007\n");
	else
		Cbuf_AddText("host_framerate 0\n");
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
		pEdict->v.origin[0] += direction[0] * step;
		pEdict->v.origin[1] += direction[1] * step;
		pEdict->v.origin[2] += direction[2] * step;

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
				&& !FindPassableSpace(sv_player, right, -1.0)	// left
				&& !FindPassableSpace(sv_player, up, 1.0)		// up
				&& !FindPassableSpace(sv_player, up, -1.0))		// down
			{
				FindPassableSpace(sv_player, forward, -1.0);	// back
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
The cheat page of the debug menu keeps its toggles where the menu can reach
them; these hand the engine the address of each one as the page is built.
==================
*/
void Host_BindCheatFly( int* pValue )
{
	gpCheatFly = pValue;
}

void Host_BindCheatNoclip( int* pValue )
{
	gpCheatNoclip = pValue;
}

void Host_BindCheatNotarget( int* pValue )
{
	gpCheatNotarget = pValue;
}

void Host_BindCheatGod( int* pValue )
{
	gpCheatGod = pValue;
}

void Host_BindCheatSlomo( int* pValue )
{
	gpCheatSlomo = pValue;
}

void Host_BindCheatAmmo( int* pValue )
{
	gpCheatAmmo = pValue;
}

void Host_BindCheatWeapons( int* pValue )
{
	gpCheatWeapons = pValue;
}

void Host_BindCheatEverything( int* pValue )
{
	gpCheatEverything = pValue;
}

void Host_BindCheatHealth( int* pValue )
{
	gpCheatHealth = pValue;
}

void Host_BindCheatGravity( int* pValue )
{
	gpCheatGravity = pValue;
}

void Host_BindCheatAllies( int* pValue )
{
	gpCheatAllies = pValue;
}

/*
==================
Host_ValidGame

Keep the cheat page of the debug menu in step with the game: whenever a toggle
and the state it controls disagree, run the command that reconciles them. The
give lists are handed out one entry per call so a full loadout arrives over
several frames instead of all at once.
==================
*/
void Host_ValidGame( void )
{
	if (gpCheatFly)
	{
		if (*gpCheatFly && sv_player->v.movetype != MOVETYPE_FLY)
			Cbuf_AddText("fly\n");

		if (!*gpCheatFly && sv_player->v.movetype == MOVETYPE_FLY)
			Cbuf_AddText("fly\n");
	}

	if (gpCheatNoclip)
	{
		if (*gpCheatNoclip && sv_player->v.movetype != MOVETYPE_NOCLIP)
			Cbuf_AddText("noclip\n");

		if (!*gpCheatNoclip && sv_player->v.movetype == MOVETYPE_NOCLIP)
			Cbuf_AddText("noclip\n");
	}

	if (gpCheatNotarget)
	{
		if (*gpCheatNotarget && !((int)sv_player->v.flags & FL_NOTARGET))
			Cbuf_AddText("notarget\n");

		if (!*gpCheatNotarget && ((int)sv_player->v.flags & FL_NOTARGET))
			Cbuf_AddText("notarget\n");
	}

	if (gpCheatGod)
	{
		if (*gpCheatGod && !((int)sv_player->v.flags & FL_GODMODE))
			Cbuf_AddText("god\n");

		if (!*gpCheatGod && ((int)sv_player->v.flags & FL_GODMODE))
			Cbuf_AddText("god\n");
	}

	if (gpCheatSlomo)
	{
		if (*gpCheatSlomo && host_framerate.value == 0)
			Cbuf_AddText("slomo\n");

		if (!*gpCheatSlomo && host_framerate.value != 0)
			Cbuf_AddText("slomo\n");
	}

	if (gpCheatAllies)
	{
		if (*gpCheatAllies && !gAlliesFriendly)
			Cbuf_AddText("impulse 222\n");

		if (!*gpCheatAllies && gAlliesFriendly)
			Cbuf_AddText("impulse 222\n");
	}

	if (gpCheatGravity)
	{
		if (*gpCheatGravity && sv_gravity.value > 400)
			Cbuf_AddText("sv_gravity 200\n");

		if (!*gpCheatGravity && sv_gravity.value < 400)
			Cbuf_AddText("sv_gravity 800\n");
	}

	// Health arrives a kit at a time
	if (gpCheatHealth && *gpCheatHealth)
	{
		gGiveHealth += 8;
		*gpCheatHealth = 0;
	}

	if (gGiveHealth)
	{
		gGiving = TRUE;
		gGiveHealth--;
		Cbuf_AddText("give item_healthkit\n");
	}
	else
	{
		gGiving = FALSE;
	}

	if (gpCheatAmmo && *gpCheatAmmo)
	{
		gGiveItems += 8;
		*gpCheatAmmo = 0;
	}

	if (gpCheatWeapons && *gpCheatWeapons)
	{
		gGiveItems += 16;
		*gpCheatWeapons = 0;
	}

	if (gpCheatEverything && *gpCheatEverything)
	{
		gGiveItems += 24;
		*gpCheatEverything = 0;
	}

	if (!gGiveItems)
	{
		gGiving = FALSE;
		return;
	}

	gGiving = TRUE;

	if (gGiveItems >= 8)
		Cbuf_AddText("give item_suit\n");

	gGiveItems--;

	switch (gGiveWeapon % 9)
	{
	case 0:
		Cbuf_AddText("give item_battery\n");
		break;
	case 1:
		Cbuf_AddText("give weapon_9mmhandgun\n");
		Cbuf_AddText("give ammo_9mmclip\n");
		break;
	case 2:
		Cbuf_AddText("give weapon_shotgun\n");
		Cbuf_AddText("give ammo_buckshot\n");
		break;
	case 3:
		Cbuf_AddText("give weapon_9mmAR\n");
		Cbuf_AddText("give ammo_9mmAR\n");
		break;
	case 4:
		Cbuf_AddText("give weapon_9mmAR\n");
		Cbuf_AddText("give ammo_ARgrenades\n");
		break;
	case 5:
		Cbuf_AddText("give weapon_357\n");
		Cbuf_AddText("give ammo_357\n");
		break;
	case 6:
		Cbuf_AddText("give weapon_rpg\n");
		Cbuf_AddText("give ammo_rpgclip\n");
		break;
	case 7:
		Cbuf_AddText("give weapon_satchel\n");
		break;
	case 8:
		Cbuf_AddText("give weapon_snark\n");
		break;
	}

	// The second campaign never sees the Xen weapons
	if (strncmp(sv.name, "ba_", 3))
	{
		gGiveItems--;

		switch (gGiveWeapon % 4)
		{
		case 0:
			Cbuf_AddText("give weapon_tripmine\n");
			break;
		case 1:
			Cbuf_AddText("give weapon_crossbow\n");
			Cbuf_AddText("give ammo_crossbow\n");
			break;
		case 2:
			Cbuf_AddText("give weapon_gauss\n");
			Cbuf_AddText("give weapon_egon\n");
			Cbuf_AddText("give ammo_gaussclip\n");
			break;
		case 3:
			Cbuf_AddText("give weapon_hornetgun\n");
			break;
		}
	}

	gGiveWeapon++;
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
==================
Cmd_menu_f

Open a menu page by name; "main" becomes the in-game menu once a level is up
==================
*/
void Cmd_menu_f( void )
{
	if (!strcmp("main", Cmd_Argv(1)) && cls.state == ca_active)
		UI_OpenMenu("gamemenu");
	else
		UI_OpenMenu(Cmd_Argv(1));
}

/*
==================
Cmd_c0dez_f

Unlock every menu entry
==================
*/
void Cmd_c0dez_f( void )
{
	M_EnableAllItems();
}

/*
==================
Cmd_screensaver_f
==================
*/
void Cmd_screensaver_f( void )
{
	g_bScreenSaverActive = atoi(Cmd_Argv(1));
}

/*
==================
Cmd_startgame_f

Switch to the named campaign and set up the map it starts on
==================
*/
void Cmd_startgame_f( void )
{
	COM_ChangeGameDir(Cmd_Argv(1));
	Cache_FreeAllLRU();
	CL_Disconnect_f();
	GL_UnloadTextures();

	if (!strcmp(Cmd_Argv(1), "barney"))
		Cvar_Set("HostMap", "ba_tram1");
	else
		Cvar_Set("HostMap", "c0a0");

	S_Init();
	HUD_Reset();
}

/*
==================
Host_AllowChangelevel

A single player game walks from level to level as the story goes; a server with
other people on it only does it when deathmatch is running the map rotation.
==================
*/
static __inline qboolean Host_AllowChangelevel( void )
{
	if (svs.maxclients > 1)
	{
		if (deathmatch.value)
			return TRUE;

		return FALSE;
	}

	return TRUE;
}

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

	con_backscroll = 0;
	key_dest = key_game;			// remove console or menu
	Sys_SetTaskName("client disconnected and server shut down");
	SCR_BeginLoadingPlaque();

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);
	S_ClearBuffer(TRUE);

	if (!loadGame)
	{
		Host_ClearGameState();
		SV_InactivateClients();
		svs.serverflags = 0;			// haven't completed an episode yet
	}

	strcpy(cls.mapstring, mapstring);

	if (!Host_AllowChangelevel())
		return;

	// Give the level all the memory that is going spare
	Cache_FreeAll();
	Bshrink_all();
	CompactAllHeaps();

	if (!SV_SpawnServer(bIsDemo, mapName, NULL))
		return;

	Sys_SetTaskName("SV_SpawnServer complete");

	if (loadGame)
	{
		if (!LoadGamestate(mapName, TRUE))
		{
			SV_LoadEntities();
		}

		Sys_SetTaskName("entities loaded");

		sv.paused = TRUE;		// pause until all clients connect
		sv.loadgame = TRUE;
		SV_ActivateServer(FALSE);
		Sys_SetTaskName("server activated");
	}
	else
	{
		SV_LoadEntities();
		Sys_SetTaskName("entities loaded");
		SV_ActivateServer(TRUE);
		Sys_SetTaskName("server activated");

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
		Cmd_ExecuteString("connect local", src_client);
	}

	Sys_SetTaskName("local client connected");
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
	char	mapstring[48];
	char	name[48];

	CL_StartProgressBar();

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
	COM_StringToLower(name);

	// Starting a map from the console: the game rules have to be up before the
	// level can spawn into them
	if (!svs.dll_initialized)
	{
		Cbuf_Execute();
		gpGlobals = &gGlobalVariables;
		GameDLLInit();
		Cbuf_Execute();
	}

	len = strlen(name);
	if (len > 4 && !Q_strcasecmp(&name[len - 4], ".bsp"))
		name[len - 4] = 0;

	if (!PF_IsMapValid_I(name))
	{
		Con_Printf("map change failed: '");
		Con_Printf(name);
		Con_Printf("' not found on server\n");
		return;
	}

	// A server with other people on it has to know how it is authenticating them
	if (svs.maxclients > 1 && !sv_lan.value)
		COM_CheckAuthenticationType();

	GL_UnloadTextures();
	Cvar_Set("HostMap", name);

	Sys_SetTaskName("About to Host_Map");
	Host_Map(FALSE, mapstring, name, FALSE);
	Sys_SetTaskName("Host_Map complete");
}

/*
======================
Host_Maps_f

======================
*/
void Host_Maps_f( void )
{
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
	char	level[48];
	char	_startspot[48];
	char*   startspot;

	CL_StartProgressBar();

	if (Cmd_Argc() < 2 || !Host_AllowChangelevel())
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
	S_ClearBuffer(TRUE);

	strcpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		strcpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_InactivateClients();

	// Make room for the new level
	Cache_FlushToDisk();
	Cache_FreeAll();
	Bshrink_all();
	CompactAllHeaps();

	SV_SpawnServer(FALSE, level, startspot);
	SV_LoadEntities();
	SV_ActivateServer(TRUE);

	// And drop everything the old level left behind
	Cache_FreeStale();
	Cache_FlushToDisk();
	Cache_FreeAll();
	Cache_FlushUnlocked();
	GL_UnloadTextures();
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

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);
	S_ClearBuffer(TRUE);

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

	if (!sv.active)
		return;

	if (cmd_source != src_command)
		return;

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);
	S_ClearBuffer(TRUE);

	Host_ClearSaveDirectory();
	Host_ClearGameState();
	SV_InactivateClients();

	// See if there is a most recently saved game
	// Restart that game if there is
	// Otherwise, restart the starting game map
	pSaveName = Host_FindRecentSave();
	if (pSaveName && Host_Load(pSaveName))
		return;

	CL_StartProgressBar();
	SCR_BeginLoadingPlaque();

	// Make room for the new level
	Cache_FlushToDisk();
	Cache_FreeAll();
	Bshrink_all();
	CompactAllHeaps();

	SV_SpawnServer(FALSE, gHostMap.string, NULL);
	SV_LoadEntities();
	SV_ActivateServer(TRUE);

	// And drop everything the old level left behind
	Cache_FreeStale();
	Cache_FlushToDisk();
	Cache_FreeAll();
	Cache_FlushUnlocked();
	GL_UnloadTextures();
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
#define DEFAULT_SAVE_BUFFER_SIZE	0x80000

SAVERESTOREDATA* SaveInit( int size )
{
	SAVERESTOREDATA* pSaveData;
	int i;
	edict_t* pEdict = NULL;

	if (size <= 0)
		size = DEFAULT_SAVE_BUFFER_SIZE;		// Reserve 512K for now, UNDONE: Shrink this after compressing strings

	pSaveData = (SAVERESTOREDATA*)calloc(sizeof(SAVERESTOREDATA) + (sizeof(ENTITYTABLE) * sv.num_edicts) + size, sizeof(char));
	pSaveData->pTable = (ENTITYTABLE*)(pSaveData + 1); // skip the save structure
	pSaveData->tokenSize = 0;
	pSaveData->tokenCount = SAVE_TOKEN_COUNT;
	pSaveData->pTokens = (char**)calloc(pSaveData->tokenCount + SAVE_TOKEN_EXTRA, sizeof(char*));

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
	pSaveData->connectionCount = 0;

	pSaveData->pBaseData = (char*)(pSaveData->pTable + sv.num_edicts); // skip the save structure
	pSaveData->pCurrentData = pSaveData->pBaseData; // reset the pointer
	pSaveData->size = 0;
	pSaveData->bufferSize = size;

	pSaveData->time = gGlobalVariables.time; // Use DLL time
	VectorCopy(vec3_origin, pSaveData->vecLandmarkOffset);
	pSaveData->fUseLandmark = FALSE;

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
	char			hlPath[MAX_PATH], name[MAX_PATH], * pTokenData;
	char			exportName[64];
	int				tag, i;
	bfile_t* pFile;
	SAVERESTOREDATA* pSaveData;
	GAME_HEADER		gameHeader;

	// The save block is big, so give the cache back its memory first
	Cache_FlushToDisk();
	Cache_FreeAll();

	pSaveData = SaveGamestate();

	if (!pSaveData)
		return FALSE;

	SaveExit(pSaveData);
	Host_SaveGameSize();
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
	pSaveData->size = (pSaveData->size + 3) & ~3;

	sprintf(name, "%s%s", Host_SaveGameDirectory(), pSaveName);
	COM_DefaultExtension(name, ".sav");
	COM_FixSlashes(name);

	// The quicksave and the autosave each keep a single slot; everything else
	// keeps a short history
	if (Q_stricmp(pSaveName, "quick") || Q_stricmp(pSaveName, "autosave"))
		Host_AgeSaveList(pSaveName, 1);

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

	// Settle the file back into the file table before it is compressed
	pFile = Bopen(name, "rb");
	Bclose(pFile);

	if (!Zip_CompressFile(name, 9))
	{
		// Out of room - send the player back to the memory card screen
		VMU_SetSaveResult(VMU_SAVE_QUIET);
	}
	else
	{
		if (exportsaves.value > 0)
		{
			sprintf(exportName, "%s.sav", sv.name);
			Bexport_path(name, exportName);
		}

		if (!fake)
		{
			VMU_SaveGameHL1(name);
			VMU_FormatSlotName(name);
			GDROM_ConfigureDoorBehavior();
		}
		else
		{
			Host_SaveGameSizeHL1(name);
		}
	}

	SaveExit(pSaveData);

	return TRUE;
}

// The save commands fire once, when the player asks for them, so there is no
// reason to expand the validity check into every one of them.
#pragma inline_depth( 0 )

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

#pragma inline_depth()

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

/*
==================
Cmd_preloadskill_f

Remember which save the skill-select screen is about to load
==================
*/
void Cmd_preloadskill_f( void )
{
	strcpy(gszPreloadSkill, Cmd_Argv(1));
}

/*
==================
Cmd_loadskill_f

Load the save that goes with the skill the player just chose
==================
*/
void Cmd_loadskill_f( void )
{
	char	name[64];
	char*	pFormat;

	if (cmd_source != src_command)
		return;

	switch ((int)skill.value)
	{
	case 1:
		pFormat = "%s_easy.sav";
		break;
	case 2:
		pFormat = "%s.sav";
		break;
	case 3:
		pFormat = "%s_hard.sav";
		break;
	default:
		pFormat = "%s.sav";
		break;
	}

	if (Cmd_Argc() == 2)
	{
		sprintf(name, pFormat, Cmd_Argv(1));
	}
	else
	{
		if (Cmd_Argc() != 1)
		{
			Con_Printf("loadskill <savename> : load a game\n");
			return;
		}

		sprintf(name, pFormat, gszPreloadSkill);
	}

	if (!Host_Load(name))
	{
		Con_Printf("Error loading saved game\n");
	}
	else
	{
		// The chapter openings play their own intro instead
		if (strncmp(gszPreloadSkill, "c0a0", 4)
		 && strncmp(gszPreloadSkill, "c1a0", 4)
		 && strncmp(gszPreloadSkill, "ba_security", 11)
		 && strncmp(gszPreloadSkill, "ba_tram", 7))
			gResumedSave = 1;
	}
}

int Host_Load( const char* pName )
{
	void* pFile;
	GAME_HEADER     gameHeader;
	char			name[256];
	int             c;
	char* pTempNumber;
	char            szNumber[5];
	int             nSlot;

	// The loaded level brings its own set in, and the save block needs the room
	GL_UnloadTextures();

	if (!pName || !pName[0])
		return FALSE;

	// Handle quick-save slots (_1 to _12)
	if (pName[0] == '_')
	{
		pTempNumber = (char*)(pName + 1);
		c = 0;

		// Extract up to 5 digits from slot number
		while (*pTempNumber && isdigit(*pTempNumber) && c < 5)
			szNumber[c++] = *pTempNumber++;

		szNumber[c] = 0;

		nSlot = atoi(szNumber);
		if (nSlot >= 1 && nSlot <= 12)
			sprintf(name, "%sHalf-Life-%i", Host_SaveGameDirectory(), nSlot);
		else
			return FALSE;
	}
	else
	{
		sprintf(name, "%s%s", Host_SaveGameDirectory(), pName);
	}

	// Loading straight off the title screen: the game rules have to be up
	// before there is anything to restore into
	if (!svs.dll_initialized)
	{
		Cbuf_Execute();
		gpGlobals = &gGlobalVariables;
		GameDLLInit();
		Cbuf_Execute();
	}

	COM_DefaultExtension(name, ".sav");
	COM_FixSlashes(name);

	// A save the player picked off the memory card isn't resident yet
	if (!FileExists(name))
		Bfetch_disc(name);

	if (Zip_DecompressFile(name, UNZIP_TEMP_FILE) != 1)
		return FALSE;

	pFile = Sys_OpenHandle(UNZIP_TEMP_FILE, "rb");
	if (!pFile)
		return FALSE;

	CL_StartProgressBar();

	// stop sounds (especially looping!)
	S_StopAllSounds(TRUE);
	S_ClearBuffer(TRUE);

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
	Bremove_path(UNZIP_TEMP_FILE);

	// The card holds the copy now, so drop the one on disc
	VMU_RemoveSave(name);

	Cvar_SetValue("deathmatch", 0.0);
	Cvar_SetValue("coop", 0.0);

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
	int             i;
	int			    dataSize, tableSize;
	edict_t* pent;
	SAVE_HEADER     header;
	SAVERESTOREDATA* pSaveData;
	SAVELIGHTSTYLE  light;

	if (!ParmsChangeLevel)
		return NULL;

	pSaveData = SaveInit(0);
	if (!pSaveData)
		return NULL;

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

	tableSize = (pSaveData->size - dataSize + 3) & ~3;
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

	pSaveData->tokenSize = (pSaveData->pCurrentData - pTokenData + 3) & ~3;
	pSaveData->pCurrentData = pTokenData + pSaveData->tokenSize;

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
	i = SAVEFILE_HEADER;
	Bwrite(&i, sizeof(int), 1, pFile);

	i = SAVEGAME_VERSION;
	Bwrite(&i, sizeof(int), 1, pFile);

	// Write out the tokens first so we can load them before we load the entities
	Bwrite(&pSaveData->size, sizeof(int), 1, pFile);		// total size of all data to initialize read buffer
	Bwrite(&pSaveData->tableCount, sizeof(int), 1, pFile);	// entities count to right initialize entity table
	Bwrite(&pSaveData->tokenCount, sizeof(int), 1, pFile);	// num hash tokens to prepare token table
	Bwrite(&pSaveData->tokenSize, sizeof(int), 1, pFile);	// total size of hash tokens
	Bwrite(pTokenData, pSaveData->tokenSize, 1, pFile);		// write tokens into the file
	Bwrite(pTableData, tableSize, 1, pFile);				// dump ETABLE structures
	Bwrite(pSaveData->pBaseData, dataSize, 1, pFile);		// and finally store all the other data

	if (exportdicts.value > 0)
		Bexport_handle(pFile);

	Bclose(pFile);

	ZipSaveGame(Host_SaveGameDirectory(), sv.name);
	Zip_CompressFile(name, 5);

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

	pFile = Sys_OpenHandle(name, "rb");
	if (!pFile)
	{
		// Not unpacked yet - pull it back out of the save bundle
		UnzipSaveGame(Host_SaveGameDirectory(), level);

		pFile = Sys_OpenHandle(name, "rb");
		if (!pFile)
		{
			Con_Printf("ERROR: couldn't open %s.\n", name);
			return NULL;
		}
	}
	Sys_CloseHandle(pFile);

	if (Zip_DecompressFile(name, UNZIP_TEMP_FILE) != 1)
	{
		Con_Printf("ERROR: couldn't open %s.\n", name);
		return NULL;
	}

	pFile = Sys_OpenHandle(UNZIP_TEMP_FILE, "rb");
	if (!pFile)
	{
		Con_Printf("ERROR: couldn't open %s.\n", UNZIP_TEMP_FILE);
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
			pSaveData->pTokens = (char**)calloc(tokenCount + 8, sizeof(char*));

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

	pSaveData->pTable = (ENTITYTABLE*)(((unsigned int)pszTokenList + 3) & ~3);
	pSaveData->currentIndex = 0;

	//---------------------------------
	// Set up the restore basis
	pSaveData->pBaseData = (char*)pSaveData->pTable + (sizeof(ENTITYTABLE) * pSaveData->tableCount);
	pSaveData->pCurrentData = pSaveData->pBaseData;

	pSaveData->size = 0;
	pSaveData->bufferSize = size;
	pSaveData->fUseLandmark = TRUE;
	pSaveData->time = 0.0;
	VectorCopy(vec3_origin, pSaveData->vecLandmarkOffset);
	gGlobalVariables.pSaveData = pSaveData;

	DC_fread(pSaveData->pBaseData, size + 3, 1, pFile);
	Sys_CloseHandle(pFile);
	Bremove_path(UNZIP_TEMP_FILE);

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

	// The entity table is padded out to a long boundary when it is written, so
	// step over that before the header and the sections behind it are read
	pSaveData->pCurrentData = (char *)(((unsigned int)pSaveData->pCurrentData + 3) & ~3);
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

		if (!table->classname || !table->size || (table->flags & FENTTABLE_REMOVED))
		{
			table->pent = NULL;
		}
		else
		{
			if (!table->id)
			{
				pent = sv.edicts;
				EntityInit(pent, table->classname);
			}
			else if (table->id < svs.maxclients + 1)
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

			table->pent = pent;
		}
	}

	for (i = 0; i < pSaveData->tableCount; i++)
	{
		ENTITYTABLE* table;

		table = &pSaveData->pTable[i];
		pent = table->pent;

		pSaveData->currentIndex = i;
		pSaveData->size = table->location;
		pSaveData->pCurrentData = pSaveData->pBaseData + table->location;

		if (pent)
		{
			if (DispatchRestore(pent, pSaveData, FALSE) < 0)
			{
				table->pent = NULL;
				ED_Free(pent);
			}
			else
			{
				// force the entity to be relinked
				SV_LinkEdict(pent, FALSE);
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

	for (i = 0; i < pSaveData->tableCount; i++)
	{
		pent = NULL;
		if (pSaveData->pTable[i].classname && pSaveData->pTable[i].size && pSaveData->pTable[i].id > 0)
		{
			active = (pSaveData->pTable[i].flags & levelMask) != 0;

			if (pSaveData->pTable[i].id < svs.maxclients + 1)
			{
				pent = svs.clients[pSaveData->pTable[i].id - 1].edict;

				if (active)
				{
					if (!(pSaveData->pTable[i].flags & FENTTABLE_PLAYER))
						Sys_Error("ENTITY IS NOT A PLAYER: %d\n", i);

					if (svs.clients[pSaveData->pTable[i].id - 1].active && pent)
						EntityInit(pent, pSaveData->pTable[i].classname);
					else
						pent = NULL;
				}
			}
			else
			{
				if (active)
					pent = CreateNamedEntity(pSaveData->pTable[i].classname);
			}
		}

		pSaveData->pTable[i].pent = pent;
	}

	// Now spawn entities
	for (i = 0; i < pSaveData->tableCount; i++)
	{
		pent = pSaveData->pTable[i].pent;
		pSaveData->currentIndex = i;
		pSaveData->size = pSaveData->pTable[i].location;
		pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->pTable[i].location;

		if (pent)
		{
			active = (pSaveData->pTable[i].flags & levelMask) != 0;

			if (active)
			{
				if (pSaveData->pTable[i].flags & FENTTABLE_GLOBAL)
				{
					// Pass the "global" flag to the DLL to indicate this entity should only override
					// a matching entity, not be spawned
					DispatchRestore(pent, pSaveData, TRUE);
					ED_Free(pent);
				}
				else
				{
					if (DispatchRestore(pent, pSaveData, FALSE) < 0)
					{
						ED_Free(pent);
					}
					else
					{
						SV_LinkEdict(pent, FALSE);

						if (!(pSaveData->pTable[i].flags & FENTTABLE_PLAYER) && EntityInSolid(pent))
						{
							// this can happen during normal processing - PVS is just a guess,
							// some map areas won't exist in the new map
							ED_Free(pent);
						}
						else
						{
							movedCount++;
							pSaveData->pTable[i].flags = FENTTABLE_REMOVED;
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
		// The gamestate for the level being saved isn't part of the bundle
		if (!strstr(pFound, ".hl1"))
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
	char	level[48];
	char	_startspot[48];
	char	oldlevel[48];
	char*	startspot;
	SAVERESTOREDATA* pSaveData;
	qboolean restored;

	CL_StartProgressBar();

	giActive = DLL_TRANS;

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
	S_CloseAllSounds();
	S_ClearBuffer(TRUE);

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

	// Give the save block room to build
	Cache_FlushToDisk();
	Cache_FreeAll();

	// save the current level's state
	pSaveData = SaveGamestate();
	if (pSaveData)
		SaveExit(pSaveData);

	// And give what is left to the level coming in
	Cache_FlushToDisk();
	Bshrink_all();
	CompactAllHeaps();

	if (!SV_SpawnServer(FALSE, level, startspot))
	{
		Sys_Error("Couldn't load map %s\n", level);
		return;
	}

	// try to restore the new level
	restored = LoadGamestate(level, FALSE);
	if (!restored)
		SV_LoadEntities();

	LoadAdjacentEntities(oldlevel, startspot);

	sv.paused = TRUE;
	sv.loadgame = TRUE;
	gGlobalVariables.time = sv.time;

	// Walking into a fresh unit leaves nothing worth carrying over
	if (!restored && sv_newunit.value)
		Host_ClearSaveDirectory();

	SV_ActivateServer(FALSE);

	Cache_FreeStale();
	Cache_FlushToDisk();
	Cache_FreeAll();
	Cache_FlushUnlocked();
	GL_UnloadTextures();
}

//============================================================================

void Host_Version_f( void )
{
	Con_Printf("Protocol version %i\nExe version %s\n", PROTOCOL_VERSION, gpszVersionString);
	Con_Printf("Exe build: " __TIME__ " " __DATE__ "\n", build_number());
}

/*
==================
Host_FullInfo_f

Allow clients to change userinfo
==================
*/
void Host_FullInfo_f( void )
{
	char	key[512];
	char	value[512];
	char*	o;
	char*	s;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;

	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_Printf("MISSING VALUE\n");
			return;
		}

		s++;
		o = value;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (cmd_source == src_command)
		{
			Info_SetValueForKey(cls.userinfo, key, value, MAX_INFO_STRING);
			Cmd_ForwardToServer();
			return;
		}

		Info_SetValueForKey(host_client->userinfo, key, value, MAX_INFO_STRING);
		host_client->sendinfo = TRUE;
	}
}

/*
==================
Host_SetInfo_f
==================
*/
void Host_SetInfo_f( void )
{
	if (Cmd_Argc() == 1)
	{
		Info_Print(cls.userinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf("usage: setinfo [ <key> <value> ]\n");
		return;
	}

	if (cmd_source == src_command)
	{
		Info_SetValueForKey(cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
		Cmd_ForwardToServer();
		return;
	}

	Info_SetValueForKey(host_client->userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
	host_client->sendinfo = TRUE;
}

void Host_Say( qboolean teamonly )
{
	client_t* client;
	client_t* save;
	int			j;
	char* p;
	char		text[128];

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

	// Keep what the player typed short enough to leave room for the name
	if (Q_strlen(p) > 63)
		p[63] = 0;

	j = sizeof(text) - 2 - Q_strlen(text);  // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned || client->fakeclient)
			continue;

		host_client = client;
		PF_MessageBegin_I(MSG_ONE, RegUserMsg("SayText", -1), NULL, &sv.edicts[j + 1]);
		PF_WriteByte_I(0);
		PF_WriteString_I(text);
		PF_MessageEnd_I();
	}
	host_client = save;
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
		SV_FullClientUpdate(client, &host_client->netchan.message);

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
	host_client->spawned = TRUE;
	host_client->connected = FALSE;

	// The connect handshake gives a bogus picture of the data rate, so throw
	// the statistics away now that the client is really playing
	host_client->netchan.frame_latency = 0.0;
	host_client->netchan.frame_rate = 0.0;
	host_client->netchan.drop_count = 0;
	host_client->netchan.good_count = 0;
	memset(host_client->netchan.flow, 0, sizeof(host_client->netchan.flow));
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
				who = name.string;
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
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


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
	int		chunk;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!host_client->download)
		return;

	r = host_client->downloadsize - host_client->downloadpos;

	chunk = 1024;
	if (host_client->downloadcustom)
		chunk = host_client->downloadchunk;

	percent = host_client->downloadpos / chunk;
	if (r > chunk)
		r = chunk;
	CRC32_ProcessBuffer(&host_client->downloadCRC, host_client->download + host_client->downloadpos, r);

	// Send download info packet
	MSG_WriteByte(&host_client->netchan.message, svc_download);
	MSG_WriteShort(&host_client->netchan.message, r);
	MSG_WriteShort(&host_client->netchan.message, percent);
	MSG_WriteLong(&host_client->netchan.message, host_client->downloadCRC);

	host_client->downloadpos += r;
	size = host_client->downloadsize;
	if (!size)
		size = 1;
	MSG_WriteByte(&host_client->netchan.message, host_client->downloadpos * 100 / size);
	SZ_Write(&host_client->netchan.message, &host_client->download[host_client->downloadpos - r], r);

	if (host_client->downloadpos != host_client->downloadsize)
		return;

	COM_FreeFile(host_client->download);
	host_client->download = NULL;
	host_client->downloadcustom = FALSE;
	host_client->downloadchunk = 1024;
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
SV_BeginDownload_f

Starts file download to client, handles both normal files and MD5-hashed resources
==================
*/
void SV_BeginDownload_f( void )
{
	char* name;

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

	// Handle customizations
	if (strlen(name) == 36 && !Q_strnicmp(name, "!MD5", 4))
	{
		resource_t resource;
		unsigned char rgucMD5_hash[16];

		memset(&resource, 0, sizeof(resource));

		COM_HexConvert(name + 4, 32, rgucMD5_hash);

		if (HPAK_ResourceForHash(HASHPAK_FILENAME, rgucMD5_hash, &resource))
		{
			HPAK_GetDataPointer(HASHPAK_FILENAME, &resource, (void**)&host_client->download, &host_client->downloadsize);
		}

		// A player's own resource goes out in pieces the client's rate can take
		host_client->downloadcustom = TRUE;

		if (!host_client->active)
			host_client->downloadchunk = 128;
		else if (host_client->netchan.rate == 0 || host_client->netchan.rate <= 3400)
			host_client->downloadchunk = 64;
		else if (host_client->netchan.rate <= 4900)
			host_client->downloadchunk = 128;
		else if (host_client->netchan.rate <= 9900)
			host_client->downloadchunk = 192;
		else
			host_client->downloadchunk = 256;
	}
	else
	{
		host_client->downloadsize = COM_FileSize(name);
		if (host_client->downloadsize != -1)
			host_client->download = COM_LoadFile(name, 5, NULL);

		host_client->downloadcustom = FALSE;
		host_client->downloadchunk = 1024;
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

	cls.soundfade.soundFadeStartTime = realtime;
	cls.soundfade.nStartPercent = percent;
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
/*
==================
Host_CRC_f

Print the CRC of a map, so a server and client can be compared
==================
*/
void Host_CRC_f( void )
{
	char		name[128];
	CRC32_t		crc;
	char*		pMapName;

	pMapName = Cmd_Argv(1);
	if (!pMapName)
		return;

	sprintf(name, "maps\\%s.bsp", pMapName);

	CRC32_Init(&crc);
	if (CRC_MapFile(&crc, name))
		Con_Printf("CRC of %s = %i\n", name, crc);
	else
		Con_Printf("Couldn't CRC %s\n", name);
}

/*
==================
Cmd_logos_f
==================
*/
void Cmd_logos_f( void )
{
	if (sv.active)
		SV_PrintLogos();

	if (cls.state != ca_dedicated && cls.state != ca_disconnected)
		CL_PrintLogoList();
}

/*
==================
Host_Protocol_f

Report or force the network protocol version
==================
*/
void Host_Protocol_f( void )
{
	int		version;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("protocol is %i\n", PROTOCOL_VERSION);
		return;
	}

	if (sv.active)
	{
		Con_Printf("protocol cannot be changed while a server is running.\n");
		return;
	}

	if (cls.state != ca_dedicated && cls.state != ca_disconnected)
	{
		Con_Printf("protocol cannot be changed while in a game.\n");
		return;
	}

	version = Q_atoi(Cmd_Argv(1));
	if (version < PROTOCOL_VERSION_OLD || version > PROTOCOL_VERSION_CURRENT)
	{
		Con_Printf("invalid protocol, valid protocols are 35 through %i\n", PROTOCOL_VERSION_CURRENT);
		return;
	}

	PROTOCOL_VERSION = version;
	Protocol_Init(version);
	Con_Printf("protocol set to %i\n", PROTOCOL_VERSION);
}

/*
==================
Host_GetServerList_f
==================
*/
void Host_GetServerList_f( void )
{
	master_t*	p;
	char		msg[5];

	NET_Config(TRUE);

	if (gfNoMasterServer || !valvemaster_adr)
		return;

	msg[0] = 'c';

	for (p = valvemaster_adr; p != NULL; p = p->next)
	{
		Con_Printf("Requesting server list from %s\n", NET_AdrToString(p->adr));
		NET_SendPacket(NS_CLIENT, 1, msg, p->adr);
	}
}

/*
==================
Host_GetBatchServerList_f
==================
*/
void Host_GetBatchServerList_f( void )
{
	master_t*	p;
	char		msg[5];

	NET_Config(TRUE);

	if (!gfNoMasterServer && valvemaster_adr)
	{
		msg[0] = 'e';
		*(int*)&msg[1] = 0;

		for (p = valvemaster_adr; p != NULL; p = p->next)
		{
			Con_Printf("Requesting batch server list from %s\n", NET_AdrToString(p->adr));
			NET_SendPacket(NS_CLIENT, sizeof(char) + sizeof(int), msg, p->adr);
		}
	}
}

/*
==================
Host_GetBatchModList_f
==================
*/
void Host_GetBatchModList_f( void )
{
	master_t*	p;
	char		msg[256];

	NET_Config(TRUE);

	if (gfNoMasterServer || !valvemaster_adr)
		return;

	sprintf(msg, "%c\r\nstart-of-list\r\n", 'x');

	for (p = valvemaster_adr; p != NULL; p = p->next)
	{
		Con_Printf("Requesting batch mod status from %s\n", NET_AdrToString(p->adr));
		NET_SendPacket(NS_CLIENT, strlen(msg) + 1, msg, p->adr);
	}
}

/*
==================
Cmd_pingsv_f
==================
*/
void Cmd_pingsv_f( void )
{
}

/*
==================
Cmd_notify_f
==================
*/
void Cmd_notify_f( void )
{
	UI_Activate();
}

/*
==================
Cmd_getcertificate_f
==================
*/
void Cmd_getcertificate_f( void )
{
}

void Host_InitCommands( void )
{
	Cmd_AddCommand("menu", Cmd_menu_f);
	Cmd_AddCommand("startgame", Cmd_startgame_f);
	Cmd_AddCommand("c0dez", Cmd_c0dez_f);
	Cmd_AddCommand("screensaver", Cmd_screensaver_f);
	Cmd_AddCommand("getcertificate", Cmd_getcertificate_f);
	Cmd_AddCommand("notify", Cmd_notify_f);
	Cmd_AddCommand("getsv", Host_GetServerList_f);
	Cmd_AddCommand("bgetsv", Host_GetBatchServerList_f);
	Cmd_AddCommand("bgetmod", Host_GetBatchModList_f);
	Cmd_AddCommand("pingsv", Cmd_pingsv_f);
	Cmd_AddCommand("protocol", Host_Protocol_f);
	Cmd_AddCommand("logos", Cmd_logos_f);
	Cmd_AddCommand("crc", Host_CRC_f);
	Cmd_AddCommand("killserver", Host_KillServer_f);
	Cmd_AddCommand("soundfade", Host_Soundfade_f);
	Cmd_AddCommand("wc", Host_WC_f);
	Cmd_AddCommand("status", Host_Status_f);
	Cmd_AddCommand("changelevel2", Host_Changelevel2_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("maps", Host_Maps_f);
	Cmd_AddCommand("restart", Host_Restart_f);
	Cmd_AddCommand("reload", Host_Reload_f);
	Cmd_AddCommand("changelevel", Host_Changelevel_f);
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("say", Host_Say_f);
	Cmd_AddCommand("say_team", Host_Say_Team_f);
	Cmd_AddCommand("tell", Host_Tell_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
	Cmd_AddCommand("spawn", Host_Spawn_f);
	Cmd_AddCommand("begin", Host_Begin_f);
	Cmd_AddCommand("prespawn", Host_PreSpawn_f);
	Cmd_AddCommand("kick", Host_Kick_f);
	Cmd_AddCommand("ping", Host_Ping_f);
	Cmd_AddCommand("load", Host_Loadgame_f);
	Cmd_AddCommand("loadskill", Cmd_loadskill_f);
	Cmd_AddCommand("preloadskill", Cmd_preloadskill_f);
	Cmd_AddCommand("save", Host_Savegame_f);
	Cmd_AddCommand("autosave", Host_AutoSave_f);
	Cmd_AddCommand("dumpvmu", Cmd_dumpvmu_f);
	Cmd_AddCommand("setinfo", Host_SetInfo_f);
	Cmd_AddCommand("fullinfo", Host_FullInfo_f);
	Cmd_AddCommand("ptrack", SV_PTrack_f);
	Cmd_AddCommand("customrsrclist", SV_RequestResourceList_f);
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
	Cmd_AddCommand("fly", Host_Fly_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);
	Cmd_AddCommand("slomo", Cmd_slomo_f);
	Cmd_AddCommand("mcache", Mod_Print);
	Cmd_AddCommand("setmaster", Master_SetMaster_f);
	Cmd_AddCommand("heartbeat", Master_Heartbeat_f);
	Cmd_AddCommand("motd", Master_RequestMOTD_f);
	Cmd_AddCommand("sv_print_custom", SV_PrintCusomizations_f);
	Cmd_AddCommand("addip", SV_AddIP_f);
	Cmd_AddCommand("removeip", SV_RemoveIP_f);
	Cmd_AddCommand("listip", SV_ListIP_f);
	Cmd_AddCommand("writeip", SV_WriteIP_f);
	Cmd_AddCommand("download", SV_BeginDownload_f);
	Cmd_AddCommand("nextdl", SV_NextDownload_f);
	Cmd_AddCommand("sv_allow_download", SV_AllowDownload_f);
	Cmd_AddCommand("sv_allow_upload", SV_AllowUpload_f);
	Cmd_AddCommand("resourcelist", SV_SendResourceListBlock_f);

	Cvar_RegisterVariable(&rcon_password);
	Cvar_RegisterVariable(&filterban);

	Cmd_AddCommand("new", SV_New_f);
	Cmd_AddCommand("dropclient", SV_Drop_f);
	Cmd_AddCommand("info", SV_Info_f);

	Cvar_RegisterVariable(&gHostMap);

	Cmd_AddCommand("keys", SV_Keys_f);

	Cvar_RegisterVariable(&sv_language);

	Host_ClearSaveDirectory();
}
//=============================================================================

// Controller-status message drawn over the HUD.
static char		hostMessage[80];
static float	hostMessageTime;

// Time the controller was last seen, and when the last warning went up
static float	hostControllerTime;
static float	hostControllerWarned;

#define HOSTMESSAGE_HOLD	2.0f	// seconds at full brightness
#define HOSTMESSAGE_GONE	2.5f	// seconds until it is off the screen
#define CONTROLLER_GRACE	3.0f	// seconds before the controller counts as gone

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
	float	time;

	time = Sys_FloatTime();

	if (!IN_ControllerPresent())
	{
		if (time - hostControllerWarned > CONTROLLER_GRACE)
		{
			if (!sv.paused)
				Cbuf_AddText("pause\n");

			if (time - hostControllerTime >= CONTROLLER_GRACE)
				Host_SetMessage("%no_controller");
			else
				Host_SetMessage("%check_controller");

			hostControllerWarned = time;
		}
	}
	else
	{
		if (sv.paused)
			Cbuf_AddText("pause\n");

		hostControllerTime = time;
	}
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
	float	elapsed;
	int		level;
	int		width;
	int		x, y;

	if (hostMessageTime == 0)
		hostMessageTime = Sys_FloatTime();

	elapsed = Sys_FloatTime() - hostMessageTime;

	if (elapsed > HOSTMESSAGE_GONE || !strlen(hostMessage))
		return;

	level = 200;
	if (elapsed > HOSTMESSAGE_HOLD)
	{
		level = 200 - (int)((elapsed - HOSTMESSAGE_HOLD) * 400);
		if (level < 0)
			return;
		if (level > 200)
			level = 200;
	}

	Font_SetScale(1.0f, 1.3333f);
	width = Font_StringWidth((dcfont_t *)draw_chars, (byte *)hostMessage);

	// Centred across the screen, sitting just inside the title-safe area
	x = 320 - width / 2;
	y = 416 - scr_safe_y;

	DCV_TexState_Additive();
	DCV_SetHudDepth(3.0f);
	Text_DrawString(1.0f, 1.3333f, hostMessage, x, y, level, level, level);
}

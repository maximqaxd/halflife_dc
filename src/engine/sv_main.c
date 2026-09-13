// sv_main.c -- server main program

#include "quakedef.h"
#include "pr_cmds.h"
#include "pr_edict.h"
#include "pmove.h"
#include "decal.h"
#include "cmodel.h"
#include "customentity.h"
#include "won.h"
#include "sound.h"

// This many players on a Lan with same key, is ok.
#define MAX_IDENTICAL_CDKEYS	5
#define MAX_PACKET_ENTITIES_MESSAGE 4000
#define MAX_CLIENT_DLL_PATH 52

server_t		sv;
server_static_t	svs;
char			localinfo[MAX_LOCALINFO];

char* pr_strings = NULL, * gNullString = "";
globalvars_t gGlobalVariables;

qboolean	allow_cheats;

decalname_t	sv_decalnames[MAX_BASE_DECALS];

// netusage info
int		nReliables = 0;
int		nDatagrams = 0;
int		nReliableBytesSent = 0;
int		nDatagramBytesSent = 0;
qboolean bUnreliableOverflow = FALSE;

int		num_servers;
int		gPacketSuppressed = 0;
int		sv_decalnamecount = 0;
int		g_userid = 1;
char	localmodels[MAX_MODELS][5];			// inline model names for precache

// User messages
UserMsg* sv_gpNewUserMsgs = NULL;
UserMsg* sv_gpUserMsgs = NULL;
int giNextUserMsg = 64;

float g_fLastPingUpdateTime = 0.0f;
qboolean g_bShouldUpdatePing = FALSE;

int		SV_UPDATE_BACKUP = 0;		// depth of the per-client frame history ring
int		SV_UPDATE_MASK = 0;

cvar_t	sv_lan = { "sv_lan", "0" };
cvar_t	sv_logrelay = { "sv_logrelay", "0" };
cvar_t	sv_smartdelta = { "sv_smartdelta", "1" };
cvar_t	sv_minrate = { "sv_minrate", "0", FCVAR_SERVER };
cvar_t	sv_maxrate = { "sv_maxrate", "0", FCVAR_SERVER };
cvar_t	sv_language = { "sv_language", "0" };
cvar_t	violence_hblood = { "violence_hblood", "1" };
cvar_t	violence_ablood = { "violence_ablood", "1" };
cvar_t	violence_hgibs = { "violence_hgibs", "1" };
cvar_t	violence_agibs = { "violence_agibs", "1" };

cvar_t	sv_newunit = { "sv_newunit", "0" };

cvar_t	showtriggers = { "showtriggers", "0" };
cvar_t	laddermode = { "laddermode", "0" };
cvar_t	sv_clienttrace = { "sv_clienttrace", "1", FCVAR_SERVER };

cvar_t	timeout = { "sv_timeout", "65", FCVAR_SERVER };
cvar_t	sv_challengetime = { "sv_challengetime", "15.0" };

cvar_t	sv_cheats = { "sv_cheats", "1", FCVAR_SERVER };

cvar_t	spectator_password = { "sv_spectator_password", "" };	// password for entering as a sepctator
cvar_t	max_spectators = { "sv_maxspectators", "8", FCVAR_SERVER };
cvar_t	sv_spectalk = { "sv_spectalk", "1", FCVAR_SERVER };
cvar_t	sv_password = { "sv_password", "", FCVAR_SERVER | FCVAR_PROTECTED };	// password for entering the game

cvar_t	sv_netsize = { "sv_netsize", "0" };

cvar_t	sv_allow_download = { "sv_allowdownload", "1", FCVAR_SERVER };
cvar_t	sv_allow_upload = { "sv_allowupload", "1", FCVAR_SERVER };
cvar_t	sv_upload_maxsize = { "sv_upload_maxsize", "0", FCVAR_SERVER };

cvar_t	sv_showcmd = { "sv_showcmd", "0" };

// Mirror the gamestate and the save file out to the development host as they
// are written, so a save can be inspected off the machine
cvar_t	exportdicts = { "exportdicts", "0" };
cvar_t	exportsaves = { "exportsaves", "0" };
cvar_t	terminator = { "terminator", "0" };

int sv_playermodel;

qboolean IsSinglePlayerGame( void );

static sizebuf_t sv_packet_entities_message;
static sizebuf_t sv_packet_entities_delta_message;
static byte sv_packet_entities_buffer[MAX_PACKET_ENTITIES_MESSAGE];
static byte sv_packet_entities_delta_buffer[MAX_PACKET_ENTITIES_MESSAGE];

void SV_Serverinfo_f( void )
{
	cvar_t* var;

	if (Cmd_Argc() == 1)
	{
		Con_Printf("Server info settings:\n");
		Info_Print(Info_Serverinfo());
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf("Star variables cannot be changed.\n");
		return;
	}

	Info_SetValueForKey(Info_Serverinfo(), Cmd_Argv(1), Cmd_Argv(2), 512);

	var = Cvar_FindVar(Cmd_Argv(1));
	if (var)
	{
		Z_Free(var->string);
		var->string = CopyString(Cmd_Argv(2));
		var->value = atof(var->string);
	}

	SV_BroadcastCommand("fullserverinfo \"%s\"\n", Info_Serverinfo());
}

void SV_Localinfo_f( void )
{
	if (Cmd_Argc() == 1)
	{
		Con_Printf("Local info settings:\n");
		Info_Print(localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf("Star variables cannot be changed.\n");
		return;
	}

	Info_SetValueForKey(localinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_LOCALINFO);
}

void SV_User_f( void )
{
	client_t* cl;
	int i;
	int uid;

	if (!sv.active)
	{
		Con_Printf("Can't 'user', not running a server\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));
	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((cl->active || cl->spawned || cl->connected) &&
			!cl->fakeclient && cl->name[0] &&
			(cl->userid == uid || !strcmp(cl->name, Cmd_Argv(1))))
		{
			Info_Print(cl->userinfo);
			return;
		}
	}

	Con_Printf("User not in server.\n");
}

void SV_Users_f( void )
{
	client_t* cl;
	int i;
	int count;

	if (!sv.active)
	{
		Con_Printf("Can't 'users', not running a server\n");
		return;
	}

	count = 0;
	Con_Printf("userid : uniqueid : name\n");
	Con_Printf("------ : ---------: ----\n");

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((cl->active || cl->spawned || cl->connected) &&
			!cl->fakeclient && cl->name[0])
		{
			Con_Printf("%6i : %u : %s\n", cl->userid, cl->network_userid, cl->name);
			count++;
		}
	}

	Con_Printf("%i users\n", count);
}

void SV_ShowServerinfo_f( void )
{
	if (cmd_source == src_command)
		Cmd_ForwardToServer();
	else
		Info_Print(Info_Serverinfo());
}

//=============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init( void )
{
	int i;
	client_t* client;

	Cmd_AddCommand("banid", SV_BanId_f);
	Cmd_AddCommand("removeid", SV_RemoveId_f);
	Cmd_AddCommand("listid", SV_ListId_f);
	Cmd_AddCommand("writeid", SV_WriteId_f);
	Cmd_AddCommand("logaddress", SV_SetLogAddress_f);
	Cmd_AddCommand("log", SV_ServerLog_f);
	Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand("localinfo", SV_Localinfo_f);
	Cmd_AddCommand("showinfo", SV_ShowServerinfo_f);
	Cmd_AddCommand("user", SV_User_f);
	Cmd_AddCommand("users", SV_Users_f);
	Cmd_AddCommand("profile", PR_Profile_f);

	Cvar_RegisterVariable(&sv_logrelay);
	Cvar_RegisterVariable(&sv_smartdelta);
	Cvar_RegisterVariable(&sv_lan);
	Cvar_RegisterVariable(&sv_password);
	Cvar_RegisterVariable(&sv_idealpitchscale);
	Cvar_RegisterVariable(&sv_aim);
	Cvar_RegisterVariable(&sv_language);
	Cvar_RegisterVariable(&violence_hblood);
	Cvar_RegisterVariable(&violence_ablood);
	Cvar_RegisterVariable(&violence_hgibs);
	Cvar_RegisterVariable(&violence_agibs);
	Cvar_RegisterVariable(&sv_newunit);
	Cvar_RegisterVariable(&sv_gravity);
	Cvar_RegisterVariable(&sv_friction);
	Cvar_RegisterVariable(&sv_edgefriction);
	Cvar_RegisterVariable(&sv_stopspeed);
	Cvar_RegisterVariable(&sv_maxspeed);
	Cvar_RegisterVariable(&sv_accelerate);
	Cvar_RegisterVariable(&sv_stepsize);
	Cvar_RegisterVariable(&sv_clipmode);
	Cvar_RegisterVariable(&sv_bounce);
	Cvar_RegisterVariable(&sv_airmove);
	Cvar_RegisterVariable(&sv_airaccelerate);
	Cvar_RegisterVariable(&sv_wateraccelerate);
	Cvar_RegisterVariable(&sv_waterfriction);
	Cvar_RegisterVariable(&sv_challengetime);
	Cvar_RegisterVariable(&timeout);
	Cvar_RegisterVariable(&sv_clienttrace);
	Cvar_RegisterVariable(&sv_zmax);
	Cvar_RegisterVariable(&sv_wateramp);
	Cvar_RegisterVariable(&sv_skyname);
	Cvar_RegisterVariable(&sv_maxvelocity);
	Cvar_RegisterVariable(&showtriggers);
	Cvar_RegisterVariable(&sv_cheats);
	Cvar_RegisterVariable(&spectator_password);
	Cvar_RegisterVariable(&max_spectators);
	Cvar_RegisterVariable(&sv_spectalk);
	Cvar_RegisterVariable(&sv_spectatormaxspeed);
	Cvar_RegisterVariable(&sv_netsize);
	Cvar_RegisterVariable(&sv_showcmd);

	Cvar_RegisterVariable(&pm_pushfix);

	Cvar_RegisterVariable(&sv_upload_maxsize);
	Cvar_RegisterVariable(&sv_allow_download);
	Cvar_RegisterVariable(&sv_allow_upload);
	Cvar_RegisterVariable(&sv_minrate);
	Cvar_RegisterVariable(&sv_maxrate);

	Cvar_RegisterVariable(&exportdicts);
	Cvar_RegisterVariable(&exportsaves);

	Pmove_Init();

	for (i = 0; i < MAX_MODELS; i++)
	{
		sprintf(localmodels[i], "*%i", i);
	}

	for (i = 0; i < svs.maxclientslimit; i++)
	{
		client = &svs.clients[i];
		SV_ClearFrames(&client->frames);
		memset(client, 0, sizeof(client_t));

		client->resourcesonhand.pPrev = &client->resourcesonhand;
		client->resourcesonhand.pNext = &client->resourcesonhand;
		client->resourcesneeded.pPrev = &client->resourcesneeded;
		client->resourcesneeded.pNext = &client->resourcesneeded;
	}

	SV_AllocClientFrames();
}

/*
==================
void SV_CountPlayers

Counts number of connections.  Clients includes regular connections
==================
*/
void SV_CountPlayers( int* clients, int* spectators )
{
	int			i;
	client_t* cl;

	*clients = 0;

	if (spectators)
		*spectators = 0;

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if (cl->active || cl->spawned || cl->connected)
		{
			(*clients)++;

			if (cl->spectator)
			{
				if (spectators)
					(*spectators)++;
			}
		}
	}
}

void SV_FindModelNumbers( void )
{
	int		i;

	sv_playermodel = -1;

	for (i = 0; i < MAX_MODELS; i++)
	{
		if (!sv.model_precache[i])
			break;
		if (!strcmp(sv.model_precache[i], "models/player.mdl"))
			sv_playermodel = i;
	}
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle( const vec_t* org, const vec_t* dir, int color, int count )
{
	int		i, v;

	if (sv.datagram.cursize > MAX_DATAGRAM - 16)
		return;
	MSG_WriteByte(&sv.datagram, svc_particle);
	MSG_WriteCoord(&sv.datagram, org[0]);
	MSG_WriteCoord(&sv.datagram, org[1]);
	MSG_WriteCoord(&sv.datagram, org[2]);
	for (i = 0; i < 3; i++)
	{
		v = dir[i] * 16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar(&sv.datagram, v);
	}
	MSG_WriteByte(&sv.datagram, count);
	MSG_WriteByte(&sv.datagram, color);
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

Pitch should be PITCH_NORM (100) for no pitch shift. Values over 100 (up to 255)
shift pitch higher, values lower than 100 lower the pitch.
==================
*/
qboolean SV_BuildSoundMsg( edict_t* entity, int channel, const char* sample, int volume,
	float attenuation, int fFlags, int pitch, vec3_t origin, sizebuf_t* buffer )
{
	char		name[MAX_QPATH];
	int		sound_num;
	int			i;
	int			ent;
	byte		value;
	short		coord;
	int			field_mask;
	unsigned short	short_value;

	if (volume < 0 || volume > 255)
		Sys_Error("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error("SV_StartSound: attenuation = %f", attenuation);

	if (channel < CHAN_AUTO || channel > CHAN_NETWORKVOICE_BASE)
		Sys_Error("SV_StartSound: channel = %i", channel);

	if (pitch < 0 || pitch > 255)
		Sys_Error("SV_StartSound: pitch = %i", pitch);

	if (sample && sample[0] == CHAR_SENTENCE)
	{
		fFlags |= SND_SENTENCE;
		sound_num = atoi(sample + 1);
		if (sound_num >= CVOXFILESENTENCEMAX)
		{
			Con_Printf("invalid sentence number: %s", sample + 1);
			return FALSE;
		}
	}
	else
	{
		name[0] = gNullString[0];
		memset(name + 1, 0, sizeof(name) - 1);
		if (sample[0] == '*')
			strcpy(name, sample + 1);
		else
			strcpy(name, sample);

		sound_num = SV_LookupSoundIndex(name);
		if (sound_num < 0)
			return FALSE;
	}

	ent = NUM_FOR_EDICT(entity);
	field_mask = fFlags;

	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;
	if (pitch != DEFAULT_SOUND_PACKET_PITCH)
		field_mask |= SND_PITCH;
	if (sound_num > 255)
		field_mask |= SND_LARGE_INDEX;

// directed messages go only to the entity the are targeted on
	MSG_WriteByte(buffer, svc_sound);
	MSG_StartBitWriting(buffer);
	short_value = (unsigned short)field_mask;
	MSG_WriteBitShort(&short_value, 9);
	if (field_mask & SND_VOLUME)
	{
		value = (byte)volume;
		MSG_WriteBitByte(&value, 8);
	}
	if (field_mask & SND_ATTENUATION)
	{
		value = (byte)(attenuation * 64.0f);
		MSG_WriteBitByte(&value, 8);
	}

	value = (byte)channel;
	MSG_WriteBitByte(&value, 3);
	short_value = (unsigned short)ent;
	MSG_WriteBitShort(&short_value, 10);

	if (sound_num > 255)
	{
		short_value = (unsigned short)sound_num;
		MSG_WriteBitShort(&short_value, 16);
	}
	else
	{
		value = (byte)sound_num;
		MSG_WriteBitByte(&value, 8);
	}

	for (i = 0; i < 3; i++)
	{
		coord = (short)(origin[i] * 8.0f);
		MSG_WriteSBitShort(&coord, 16);
	}

	if (field_mask & SND_PITCH)
	{
		value = (byte)pitch;
		MSG_WriteBitByte(&value, 8);
	}
	MSG_EndBitWriting(buffer);
	return TRUE;
}

void SV_StartSound( edict_t* entity, int channel, const char* sample, int volume, float attenuation, int fFlags, int pitch )
{
	int		i;
	vec3_t	origin;
	qboolean	spatial;

	for (i = 0; i < 3; i++)
		origin[i] = entity->v.origin[i] + (entity->v.mins[i] + entity->v.maxs[i]) * 0.5f;

	if (!SV_BuildSoundMsg(entity, channel, sample, volume, attenuation, fFlags, pitch,
		origin, &sv.multicast))
		return;

	if (channel == CHAN_STATIC || (fFlags & SND_STOP))
		spatial = FALSE;
	else
		spatial = TRUE;

	if (spatial)
	{
		SV_Multicast(origin, MSG_ALL, FALSE);
	}
	else if (IsSinglePlayerGame())
	{
		SV_Multicast(origin, MSG_BROADCAST, FALSE);
	}
	else
	{
		SV_Multicast(origin, MSG_BROADCAST, TRUE);
	}
}

/*
=============================================================================

MULTICAST MESSAGE

=============================================================================
*/

qboolean IsSinglePlayerGame( void )
{
	if (!sv.active)
		return cl.maxclients == 1;

	return svs.maxclients == 1;
}

/*
================
SV_ValidClientMulticast

Checks if a client should receive a multicast message
================
*/
qboolean SV_ValidClientMulticast( client_t* client, int soundLeaf, int to )
{
	unsigned char* mask;
	int bitNumber;

	mask = NULL;

	if (IsSinglePlayerGame())
		return TRUE;

	if (to == MSG_FL_NONE)
	{
	}
	else if (to == MSG_FL_PVS)
	{
		mask = CM_LeafPVS(soundLeaf);
	}
	else if (to == MSG_FL_PAS)
	{
		mask = CM_LeafPAS(soundLeaf);
	}
	else
	{
		Con_Printf("MULTICAST: Error %d!\n", to);
		return FALSE;
	}

	if (!mask)
		return TRUE;

	// Get the leaf number the client is on.
	bitNumber = SV_PointLeafnum(client->edict->v.origin);

	if (mask[(bitNumber - 1) >> 3] & (1 << ((bitNumber - 1) & 7)))
		return TRUE;

	return FALSE;
}

/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.
=================
*/
void SV_Multicast( vec_t* origin, int to, qboolean reliable )
{
	client_t* client;
	int		leafnum;
	int		j;
	sizebuf_t* pBuffer;

	leafnum = SV_PointLeafnum(origin);

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active)
			continue;

		if (SV_ValidClientMulticast(client, leafnum, to))
		{
			if (reliable)
			{
				pBuffer = &client->netchan.message;
			}
			else
			{
				pBuffer = &client->datagram;
			}

			if (pBuffer->maxsize - pBuffer->cursize > sv.multicast.cursize)
				SZ_Write(pBuffer, sv.multicast.data, sv.multicast.cursize);
		}
		else
		{
			gPacketSuppressed += sv.multicast.cursize;
		}
	}

	SZ_Clear(&sv.multicast);
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
==================
SV_WriteMovevarsToClient

==================
*/
void SV_WriteMovevarsToClient( sizebuf_t* message )
{
	MSG_WriteByte(message, svc_newmovevars);
	MSG_WriteFloat(message, movevars.gravity);
	MSG_WriteFloat(message, movevars.stopspeed);
	MSG_WriteFloat(message, movevars.maxspeed);
	MSG_WriteFloat(message, movevars.spectatormaxspeed);
	MSG_WriteFloat(message, movevars.accelerate);
	MSG_WriteFloat(message, movevars.airaccelerate);
	MSG_WriteFloat(message, movevars.wateraccelerate);
	MSG_WriteFloat(message, movevars.friction);
	MSG_WriteFloat(message, movevars.edgefriction);
	MSG_WriteFloat(message, movevars.waterfriction);
	MSG_WriteFloat(message, movevars.entgravity);
	MSG_WriteFloat(message, movevars.bounce);
	MSG_WriteFloat(message, movevars.stepsize);
	MSG_WriteFloat(message, movevars.maxvelocity);
	MSG_WriteFloat(message, movevars.zmax);
	MSG_WriteFloat(message, movevars.waveHeight);
	MSG_WriteString(message, movevars.skyName);
}

/*
==================
SV_QueryMovevarsChanged

Check for changes in movevars and update them if necessary
==================
*/
void SV_QueryMovevarsChanged( void )
{
	int i;
	client_t* cl;

	if (movevars.maxspeed			== sv_maxspeed.value &&
		movevars.gravity			== sv_gravity.value &&
		movevars.stopspeed			== sv_stopspeed.value &&
		sv_spectatormaxspeed.value	== movevars.spectatormaxspeed &&
		movevars.accelerate			== sv_accelerate.value &&
		movevars.airaccelerate		== sv_airaccelerate.value &&
		movevars.wateraccelerate	== sv_wateraccelerate.value &&
		movevars.friction			== sv_friction.value &&
		movevars.edgefriction		== sv_edgefriction.value &&
		movevars.waterfriction		== sv_waterfriction.value &&
		movevars.entgravity			== 1.0f &&
		movevars.bounce				== sv_bounce.value &&
		movevars.stepsize			== sv_stepsize.value &&
		movevars.maxvelocity		== sv_maxvelocity.value &&
		movevars.zmax				== sv_zmax.value &&
		movevars.waveHeight			== sv_wateramp.value &&
		!strcmp(sv_skyname.string, movevars.skyName))
		return;

	SV_SetMoveVars();

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if (!cl->fakeclient && (cl->active || cl->spawned || cl->connected))
			SV_WriteMovevarsToClient(&cl->netchan.message);
	}
}

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo( client_t *client )
{
	char			message[2048];
	int				playernum;

	// Only send this message to developer console, or multiplayer clients.
	if ((developer.value) || (svs.maxclients > 1))
	{
		MSG_WriteByte(&client->netchan.message, svc_print);
		sprintf(message, "%c\nBUILD %d SERVER (%i CRC)\nServer # %i\n", 0x2, build_number(), 0, svs.spawncount);
		MSG_WriteString(&client->netchan.message, message);
	}

	MSG_WriteByte(&client->netchan.message, svc_serverinfo);
	MSG_WriteLong(&client->netchan.message, PROTOCOL_VERSION);
	MSG_WriteLong(&client->netchan.message, svs.spawncount);
	MSG_WriteLong(&client->netchan.message, sv.worldmapCRC);       // To prevent cheating with hacked maps
	MSG_WriteLong(&client->netchan.message, sv.clientSideDllCRC);  // To prevent cheating with hacked client dlls
	MSG_WriteByte(&client->netchan.message, svs.maxclients);

	playernum = NUM_FOR_EDICT(client->edict) - 1;
	if (client->spectator)
		playernum |= PN_SPECTATOR;
	MSG_WriteByte(&client->netchan.message, playernum);

	if (!coop.value && deathmatch.value)
		MSG_WriteByte(&client->netchan.message, GAME_DEATHMATCH);
	else
		MSG_WriteByte(&client->netchan.message, GAME_COOP);

	COM_FileBase(com_gamedir, message);
	MSG_WriteString(&client->netchan.message, message);
	MSG_WriteString(&client->netchan.message, sv.name);
	sprintf(message, "%s", pr_strings + sv.edicts->v.message);
	MSG_WriteString(&client->netchan.message, message);

	SV_WriteMovevarsToClient(&host_client->netchan.message);

// send music
	MSG_WriteByte(&client->netchan.message, svc_cdtrack);
	MSG_WriteByte(&client->netchan.message, gGlobalVariables.cdAudioTrack);
	MSG_WriteByte(&client->netchan.message, gGlobalVariables.cdAudioTrack);

// set view
	MSG_WriteByte(&client->netchan.message, svc_setview);
	MSG_WriteShort(&client->netchan.message, NUM_FOR_EDICT(client->edict));

// send resource request
	MSG_WriteByte(&client->netchan.message, svc_resourcerequest);
	MSG_WriteLong(&client->netchan.message, svs.spawncount);
	MSG_WriteLong(&client->netchan.message, 0);

	client->connected = TRUE;
	client->sendsignon = TRUE;
	client->spawned = FALSE;
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f( void )
{
	edict_t* ent;

	if (cmd_source == src_command)
		return;

	if (host_client->spawned && !host_client->active)
		return;

	ent = host_client->edict;

	host_client->connected = TRUE;
	host_client->connection_started = realtime;

	SZ_Clear(&host_client->netchan.message);

	SV_SendServerinfo(host_client);

	// Send user messages
	if (sv_gpUserMsgs)
	{
		UserMsg* pTemp;

		pTemp = sv_gpNewUserMsgs;
		sv_gpNewUserMsgs = sv_gpUserMsgs;
		SV_SendUserReg(&host_client->netchan.message);
		sv_gpNewUserMsgs = pTemp;
	}

	if ((host_client->active || host_client->spawned) && ent && !ent->free && ent->pvPrivateData)
	{
		if (host_client->spectator)
			SpectatorDisconnect(ent);
		else
			ClientDisconnect(ent);
	}

	if (host_client->spectator)
	{
		SV_SpawnSpectator();

		gGlobalVariables.time = sv.time;
		gEntityInterface.pfnParmsNewLevel();
		gEntityInterface.pfnSpectatorConnect(ent);
	}
	else
	{
		char	szName[32];
		char	szAddress[64];
		char	szRejectReason[128];

		sprintf(szName, "%s", host_client->name);
		sprintf(szAddress, "%s", NET_AdrToString(host_client->netchan.remote_address));
		sprintf(szRejectReason, "Connection rejected by game\n");

		// let the game refuse the connection and tell the client why
		if (!gEntityInterface.pfnClientConnect(ent, szName, szAddress, szRejectReason))
		{
			MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
			MSG_WriteString(&host_client->netchan.message, va("echo %s", szRejectReason));
			SV_DropClient(host_client, FALSE);
			return;
		}
	}

	MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
	MSG_WriteString(&host_client->netchan.message, va("fullserverinfo \"%s\"", Info_Serverinfo()));

}

/*
=================
SV_PTrack_f

Change the bandwidth estimate for a client
=================
*/
void SV_PTrack_f( void )
{
	int i;

	if (Cmd_Argc() != 2)
	{
		// turn off tracking
		host_client->spec_track = 0;
		return;
	}

	i = atoi(Cmd_Argv(1));
	if (i < 0 || i >= svs.maxclientslimit || !svs.clients[i].active ||
		svs.clients[i].spectator)
	{
		SV_ClientPrintf("Invalid client to track\n");
		host_client->spec_track = 0;
		return;
	}
	host_client->spec_track = i + 1; // now tracking
}

/*
================
SV_RejectConnection

Rejects connection request and sends back a message
================
*/
void SV_RejectConnection( netadr_t* adr, char* text )
{
	SZ_Clear(&net_message);
	MSG_WriteLong(&net_message, -1); // -1 -1 -1 -1 signal
	MSG_WriteByte(&net_message, S2C_CONNREJECT);
	MSG_WriteString(&net_message, text);
	NET_SendPacket(NS_SERVER, net_message.cursize, net_message.data, *adr);
	SZ_Clear(&net_message);
	SV_ClearAuthRequest(adr);
}

void SV_RejectConnectionForPassword( netadr_t* adr )
{
	SZ_Clear(&net_message);
	MSG_WriteLong(&net_message, -1);
	MSG_WriteByte(&net_message, S2C_BADPASSWORD);
	MSG_WriteString(&net_message, "BADPASSWORD");
	NET_SendPacket(NS_SERVER, net_message.cursize, net_message.data, *adr);
	SZ_Clear(&net_message);
	SV_ClearAuthRequest(adr);
}

/*
==================
SV_SpawnSpectator
==================
*/
void SV_SpawnSpectator( void )
{
	int		i;
	edict_t* e;

	VectorCopy(vec3_origin, sv_player->v.origin);
	VectorCopy(vec3_origin, sv_player->v.view_ofs);
	sv_player->v.view_ofs[2] = 22;

	// search for an info_playerstart to spawn the spectator at
	for (i = MAX_CLIENTS - 1; i < sv.num_edicts; i++)
	{
		e = EDICT_NUM(i);
		if (!strcmp(pr_strings + e->v.classname, "info_player_start"))
		{
			VectorCopy(e->v.origin, sv_player->v.origin);
			return;
		}
	}
}


/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient( void )
{
	edict_t* ent;
	client_t* client;
	client_t* checkClient;
	netadr_t		adr;
	int				edictnum;
	int				i;
	char* s;
	char			name[128];
	char			szRawCertificate[512];
	char			userinfo[1024];
	int				clients, spectators;
	int				nAuthProtocol;
	int				nNetworkUserID = -1;
	int				nCDKeyLength;
	int				nHashCount = 0;
	int				nAuthenticatedUserID = -1;
	int				version;
	int				challenge;

	adr = net_from;

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		SV_RejectConnection(&adr, "Bad protocol\n");
		return;
	}

	if (Cmd_Argc() < 7)
	{
		SV_RejectConnection(&adr, "Insufficient connection info\n");
		return;
	}

	challenge = atoi(Cmd_Argv(2));

	// see if the challenge is valid
	if (!NET_IsLocalAddress(adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (NET_CompareBaseAdr(net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				SV_RejectConnection(&adr, "Bad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			SV_RejectConnection(&adr, "No challenge for your address.\n");
			return;
		}
	}

	if ((gfUseLANAuthentication || sv_lan.value) && net_from.type == NA_IP)
	{
		if (!NET_CompareClassBAdr(net_from, net_local_adr) &&
			!NET_IsReservedAdr(net_from))
		{
			SV_RejectConnection(&adr, "LAN servers are restricted to local clients (class C).\n");
			return;
		}
	}

	// Client connection has passed challenge response.
	// if there is allready a slot for this ip, drop it
	for (i = 0, checkClient = svs.clients; i < svs.maxclientslimit; i++, checkClient++)
	{
		if (!checkClient->active && !checkClient->spawned && !checkClient->connected)
			continue;

		if (NET_CompareAdr(adr, checkClient->netchan.remote_address))
		{
			SV_DropClient(checkClient, FALSE);
		}
	}

	s = Cmd_Argv(3);
	if (!s || !s[0])
	{
		SV_RejectConnection(&adr, "Invalid connection.\n");
		return;
	}

	// Check protocol ID
	nAuthProtocol = atoi(s);
	if ((nAuthProtocol <= 0) || (nAuthProtocol > PROTOCOL_LASTVALID))
	{
		SV_RejectConnection(&adr, "Invalid connection.\n");
		return;
	}

	s = Cmd_Argv(4);
	if (!s || !s[0])
	{
		SV_RejectConnection(&adr, "Invalid connection.\n");
		return;
	}

	nNetworkUserID = atoi(s);
	if (nNetworkUserID != -1 && SV_FilterUser(nNetworkUserID))
	{
		SV_RejectConnection(&adr, "Banned.\n");
		return;
	}

	s = Cmd_Argv(5);
	if (!s || !s[0])
	{
		SV_RejectConnection(&adr, "Invalid connection.\n");
		return;
	}

	nCDKeyLength = atoi(s);
	if ((nCDKeyLength <= 0) || (nCDKeyLength > 1024))
	{
		SV_RejectConnection(&adr, "Invalid connection.\n");
		return;
	}

	// Now check auth information
	s = Cmd_Argv(6);
	if (!s)
	{
		SV_RejectConnection(&adr, "Invalid connection protocol.\n");
		return;
	}

	memset(szRawCertificate, 0, sizeof(szRawCertificate));
	strcpy(szRawCertificate, s);

	memset(userinfo, 0, sizeof(userinfo));
	strncpy(userinfo, Cmd_Argv(7), sizeof(userinfo) - 1);

	s = Info_ValueForKey(userinfo, "spectator");
	if (s[0])
		strcmp(s, "1");

	if (!NET_IsLocalAddress(adr) && sv_password.string[0] &&
		Q_stricmp(sv_password.string, "none") &&
		strcmp(sv_password.string, Info_ValueForKey(userinfo, "password")))
	{
		Con_Printf("%s:  password failed\n", NET_AdrToString(adr));
		SV_RejectConnectionForPassword(&adr);
		return;
	}

	Info_RemoveKey(userinfo, "password");
	s = Info_ValueForKey(userinfo, "name");
	memset(name, 0, sizeof(name));
	if (!s)
		strcpy(name, "unconnected");
	else
	{
		strncpy(name, s, sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;
	}

	// count up the clients and spectators
	spectators = 0;
	clients = 0;
	SV_CountPlayers(&clients, &spectators);
	clients -= spectators;

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active && !client->spawned && !client->connected)
			break;
	}

	if (i == svs.maxclients)
	{
		SV_RejectConnection(&net_from, "Server is full.");
		return;
	}

	// Store off the potential client number
	edictnum = i + 1;
	ent = EDICT_NUM(edictnum);

	if (nAuthProtocol == PROTOCOL_AUTHCERTIFICATE)
	{
		if (!SV_AuthenticateClient(&adr, szRawCertificate, &nAuthenticatedUserID))
		{
			SV_RejectConnection(&adr, "Invalid authentication information.\n");
			return;
		}

		if (nAuthenticatedUserID != nNetworkUserID)
		{
			SV_RejectConnection(&adr, "Invalid authentication userid.\n");
			return;
		}

		if (nAuthenticatedUserID != -1 && SV_FilterUser(nAuthenticatedUserID))
		{
			SV_RejectConnection(&adr, "Banned.\n");
			return;
		}
	}
	else
	{
		if (!gfUseLANAuthentication)
		{
			SV_RejectConnection(&adr, "Invalid authentication message.\n");
			return;
		}

		if (strlen(szRawCertificate) != 32 || nCDKeyLength != 32)
		{
			SV_RejectConnection(&adr, "Invalid CD Key.\n");
			return;
		}

		for (i = 0, checkClient = svs.clients; i < svs.maxclients; i++, checkClient++)
		{
			if ((!checkClient->active && !checkClient->spawned && !checkClient->connected) ||
				Q_strnicmp(szRawCertificate, checkClient->hashedcdkey, 32))
				continue;

			nHashCount++;
		}

		if (nHashCount >= MAX_IDENTICAL_CDKEYS)
		{
			SV_RejectConnection(&adr, "CD Key already in use.\n");
			return;
		}
	}

	host_client = client;

	SV_ClearResourceLists(client);
	SV_ClearFrames(&client->frames);
	memset(client, 0, sizeof(client_t));

	client->resourcesneeded.pPrev = &client->resourcesneeded;
	client->resourcesneeded.pNext = &client->resourcesneeded;
	client->resourcesonhand.pPrev = &client->resourcesonhand;
	client->resourcesonhand.pNext = &client->resourcesonhand;

	client->frames = (client_frame_t*)MnemoAllocDbg(sizeof(client_frame_t) * SV_UPDATE_BACKUP, __FILE__, __LINE__);
	memset(client->frames, 0, sizeof(client_frame_t) * SV_UPDATE_BACKUP);

	Netchan_Setup(NS_SERVER, &client->netchan, adr);
	Netchan_OutOfBandPrint(NS_SERVER, adr, "%c0000000000000000", S2C_CONNECTION);
	Q_stricmp(NET_AdrToString(client->netchan.remote_address), "loopback");

	if (nAuthProtocol == PROTOCOL_HASHEDCDKEY)
	{
		strncpy(client->hashedcdkey, szRawCertificate, SIGNED_GUID_LEN);
		client->hashedcdkey[SIGNED_GUID_LEN] = 0;
	}

	client->active = FALSE;
	client->spawned = FALSE;
	client->connected = TRUE;
	client->uploading = FALSE;

	client->edict = ent;
	client->maxspeed = 0;
	client->userid = g_userid++;
	client->network_userid = nNetworkUserID;

	Log_Printf("\"%s<%i><WON:%u>\" connected, address \"%s\"\n",
		name, client->userid, client->network_userid,
		NET_AdrToString(client->netchan.remote_address));

	strncpy(client->userinfo, userinfo, MAX_INFO_STRING);
	SV_ExtractFromUserinfo(client);
	client->sendinfo = FALSE;

	client->datagram.allowoverflow = TRUE;
	client->datagram.data = client->datagram_buf;
	client->datagram.maxsize = MAX_DATAGRAM;
	client->privileged = FALSE;

	if (!sv.loadgame)
		gEntityInterface.pfnParmsNewLevel();
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
#if HLDC_MP
void SVC_Ping( void )
{
	char	data;

	data = A2A_ACK;

	NET_SendPacket(NS_SERVER, 1, &data, net_from);
}
#else
void SVC_Ping( void )
{
	char data[6];
	data[0] = -1;
	data[1] = -1;
	data[2] = -1;
	data[3] = -1;
	data[4] = A2A_ACK;
	data[5] = 0;
	NET_SendPacket(NS_SERVER, sizeof(data), data, net_from);
}
#endif

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/
void SVC_Status( void )
{
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge( void )
{
	int		i;
	int		oldest;
	int		oldestTime;
	char	data[128];
	int 	ch;
	int		len;
	
	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (NET_CompareClassBAdr(net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = (RandomLong(0, 0xFFFF) << 16 | RandomLong(0, 0xFFFF));
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = realtime;
		i = oldest;
	}

	// send it back
	sprintf(data, "%c%c%c%c%c0000000000000000", 255, 255, 255, 255, S2C_CHALLENGE);
	len = 21;
	ch = BigLong(LoadUnalignedLong(&svs.challenges[i].challenge));
	memcpy(data + len, &ch, sizeof(ch));
	len += sizeof(ch);

	if (sv.active && svs.maxclients > 1)
	{
		if (!sv_lan.value)
			COM_CheckAuthenticationType();
		else
			gfUseLANAuthentication = TRUE;
	}
	else
		gfUseLANAuthentication = TRUE;

	if (gfUseLANAuthentication)
		data[len++] = PROTOCOL_HASHEDCDKEY;
	else
		data[len++] = PROTOCOL_AUTHCERTIFICATE;

	NET_SendPacket(NS_SERVER, len, data, net_from);
}

typedef struct modinfo_s
{
	qboolean bIsMod;
	char szInfo[256];
	char szDL[256];
	char szHLVersion[32];
	int version;
	int size;
	qboolean svonly;
	qboolean cldll;
} modinfo_t;

modinfo_t gmodinfo;

void SV_InitModInfo( void )
{
	memset(&gmodinfo, 0, sizeof(gmodinfo));
	gmodinfo.version = 1;
	gmodinfo.svonly = TRUE;
}

qboolean SV_GetModInfo( char* pszInfo, char* pszDL, int* version, int* size,
	int* svonly, int* cldll, char* pszHLVersion )
{
	if (gmodinfo.bIsMod)
	{
		strcpy(pszInfo, gmodinfo.szInfo);
		strcpy(pszDL, gmodinfo.szDL);
		strcpy(pszHLVersion, gmodinfo.szHLVersion);
		*version = gmodinfo.version;
		*size = gmodinfo.size;
		*svonly = gmodinfo.svonly;
		*cldll = gmodinfo.cldll;
	}
	else
	{
		memcpy(pszInfo, "", 1);
		memcpy(pszDL, "", 1);
		memcpy(pszHLVersion, "", 1);
		*version = 1;
		*size = 0;
		*svonly = TRUE;
		*cldll = FALSE;
	}

	return gmodinfo.bIsMod;
}

/*
================
SVC_Info

Responds with short or long info for broadcast scans
================
*/
void SVC_Info( qboolean bDetailed )
{
	int i;
	int count;
	sizebuf_t buf;
	byte data[MAX_ROUTEABLE_PACKET];
	char szModURL_Info[512];
	char szModURL_DL[512];
	int mod_version;
	int mod_size;
	char gd[260];
	int cldll;
	int svonly;
	char szHLVersion[32];

	buf.data = data;
	buf.maxsize = sizeof(data);
	buf.cursize = 0;

	if (!sv.active)
		return;

	if (svs.maxclients <= 1)
		return;

	count = 0;
	for (i = 0; i < svs.maxclients; i++)
	{
		client_t* client = &svs.clients[i];

		if (client->active || client->spawned || client->connected)
			count++;
	}

	MSG_WriteLong(&buf, -1);
	MSG_WriteByte(&buf, bDetailed ? S2A_INFO_DETAILED : S2A_INFO);

	if (!noip)
		MSG_WriteString(&buf, NET_AdrToString(net_local_adr));
	else if (!noipx)
		MSG_WriteString(&buf, NET_AdrToString(net_local_ipx_adr));
	else
		MSG_WriteString(&buf, "LOOPBACK");

	MSG_WriteString(&buf, host_name.string);
	MSG_WriteString(&buf, sv.name);
	COM_FileBase(com_gamedir, gd);
	MSG_WriteString(&buf, gd);
	MSG_WriteString(&buf, gEntityInterface.pfnGetGameDescription());

	MSG_WriteByte(&buf, count);
	MSG_WriteByte(&buf, svs.maxclients);
	MSG_WriteByte(&buf, PROTOCOL_VERSION);

	if (bDetailed)
	{
		MSG_WriteByte(&buf, cls.state != ca_dedicated ? A2A_PRINT : M2A_SERVERS);
		MSG_WriteByte(&buf, M2A_MASTERSERVERS);

		if (strlen(sv_password.string) && Q_stricmp(sv_password.string, "none"))
			MSG_WriteByte(&buf, 1);
		else
			MSG_WriteByte(&buf, 0);

		if (SV_GetModInfo(szModURL_Info, szModURL_DL, &mod_version, &mod_size,
			&svonly, &cldll, szHLVersion))
		{
			MSG_WriteByte(&buf, 1);
			MSG_WriteString(&buf, szModURL_Info);
			MSG_WriteString(&buf, szModURL_DL);
			MSG_WriteString(&buf, szHLVersion);
			MSG_WriteLong(&buf, mod_version);
			MSG_WriteLong(&buf, mod_size);
			MSG_WriteByte(&buf, svonly);
			MSG_WriteByte(&buf, cldll);
		}
		else
		{
			MSG_WriteByte(&buf, 0);
		}
	}

	NET_SendPacket(NS_SERVER, buf.cursize, buf.data, net_from);
}

/*
=================
SVC_PlayerInfo

Returns info about requested player.
=================
*/
void SVC_PlayerInfo( void )
{
	int		i, count;
	client_t* client;
	sizebuf_t buf;
	byte	data[2048];

	buf.data = data;
	buf.maxsize = sizeof(data);
	buf.cursize = 0;

	if (!sv.active)            // Must be running a server.
		return;

	if (svs.maxclients <= 1)   // ignore in single player
		return;

	// Find Player
	MSG_WriteLong(&buf, -1);
	MSG_WriteByte(&buf, S2A_PLAYER);

	count = 0;
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!svs.clients[i].active)
			continue;

		count++;
	}

	MSG_WriteByte(&buf, count);
	count = 0;
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!svs.clients[i].active)
			continue;

		count++;

		MSG_WriteByte(&buf, count);
		MSG_WriteString(&buf, client->name);
		MSG_WriteLong(&buf, client->edict->v.frags);
		MSG_WriteFloat(&buf, realtime - client->netchan.connect_time);
	}

	NET_SendPacket(NS_SERVER, buf.cursize, buf.data, net_from);
}

/*
=================
SVC_RuleInfo

More detailed server information
=================
*/
void SVC_RuleInfo( void )
{
	int		nNumRules;
	cvar_t* var;
	sizebuf_t buf;
	byte	data[2048];

	buf.data = data;
	buf.maxsize = sizeof(data);
	buf.cursize = 0;

	if (!sv.active)            // Must be running a server.
		return;
	
	if (svs.maxclients <= 1)   // ignore in single player
		return;

	nNumRules = Cvar_CountServerVariables();
	if (nNumRules <= 0)        // No server rules active.
		return;

	// Find Player
	MSG_WriteLong(&buf, -1);
	MSG_WriteByte(&buf, S2A_RULES);  // All players coming now.
	MSG_WriteShort(&buf, nNumRules);

	// Need to respond with game directory, game name, and any server variables that have been set that
	//  effect rules.  Also, probably need a hook into the .dll to respond with additional rule information.
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & FCVAR_SERVER))
			continue;

		MSG_WriteString(&buf, var->name);   // Cvar Name
		if (var->flags & FCVAR_PROTECTED)
		{
			if (!strlen(var->string) || !Q_stricmp(var->string, "none"))
				MSG_WriteString(&buf, "0");
			else
				MSG_WriteString(&buf, "1");
		}
		else
		{
			MSG_WriteString(&buf, var->string); // Value
		}
	}

	NET_SendPacket(NS_SERVER, buf.cursize, buf.data, net_from);
}

/*
=================
SV_SetMasterPeeringMessage

Set master peering message
=================
*/
#if HLDC_MP
char g_szMasterMsg[1024];
int g_iMasterMsgSize;
void SV_SetMasterPeeringMessage( qboolean skipHeader )
{
	char	pBuf[1024];
	int		nSize;

	if (skipHeader)
	{
		msg_readcount = 0;
		MSG_ReadLong();
		MSG_ReadByte();
		MSG_ReadByte();
	}

	nSize = MSG_ReadLong();
	if (nSize <= 0 || nSize >= sizeof(pBuf))
		return;

	if (MSG_ReadBuf(nSize, pBuf) == -1)
		return;

	g_iMasterMsgSize = nSize;
	memcpy(g_szMasterMsg, pBuf, g_iMasterMsgSize);
}

/*
=================
SVC_Heartbeat

=================
*/
void SVC_Heartbeat( void )
{
	gfHeartbeatWaiting = FALSE;                   // // Kill timer
	gHeartbeatChallenge = MSG_ReadLong();

	if (MSG_ReadByte() == M2A_ACTIVEMODS)
		SV_SetMasterPeeringMessage(FALSE);

	// Send the actual heartbeat request to this master server.
	Master_RequestHeartbeat();
}

/*
=================
SVC_MasterPrint

Master message
=================
*/
float gfLastMasterPrintTime = 0.0f;
void SVC_MasterPrint( void )
{
	char	pBuf[1024];
	byte	pSign[1024];
	int		nSize, nSignSize;
	char* pMsg;

	if (!g_iMasterMsgSize)
		return;

	msg_readcount = 0;
	MSG_ReadLong();
	MSG_ReadByte();
	MSG_ReadByte();

	nSize = MSG_ReadLong();
	if (nSize <= 0 || nSize >= sizeof(pBuf))
		return;

	if (MSG_ReadBuf(nSize, pBuf) == -1)
		return;

	nSignSize = MSG_ReadLong();
	if (nSignSize <= 0 || nSignSize >= sizeof(pSign))
		return;

	if (MSG_ReadBuf(nSignSize, pSign) == -1)
		return;

	// Verify the message
#if 0
	pMsg = Launcher_VerifyMessage(g_iMasterMsgSize, g_szMasterMsg, nSize, pBuf, nSignSize, pSign);
#endif
	if (!pMsg)
		return;

	if ((realtime - gfLastMasterPrintTime) >= 5.0f)
	{
		gfLastMasterPrintTime = realtime;
		Con_Printf("%s\n", pMsg);
	}
}
#endif

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( void )
{
	char* s;
	char* c;
	char data[6];

	MSG_BeginReading();
	MSG_ReadLong();		// skip the -1 marker

	s = MSG_ReadStringLine();
	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") ||
		(c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')))
	{
		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;
		NET_SendPacket(NS_SERVER, sizeof(data), data, net_from);
	}
	else if (c[0] == A2A_PRINT && (c[1] == 0 || c[1] == '\n'))
	{
		MSG_BeginReading();
		MSG_ReadLong();
		MSG_ReadByte();
		MSG_ReadString();
	}
	else if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n'))
	{
		Con_Printf("A2A_ACK from %s\n", NET_AdrToString(net_from));
	}
	else if (c[0] == M2A_CHALLENGE && (c[1] == 0 || c[1] == '\n'))
	{
		Sys_Error("RAB deleted this and hopes it isn't needed.\n");
	}
	else if (!_stricmp(c, "log"))
	{
		if (sv_logrelay.value && s && strlen(s) > 4)
		{
			s = s + strlen("log ");
			if (s && s[0])
				Con_Printf("%s\n", s);
		}
	}
	else if (WON_IsValidAuthMessage((unsigned char)c[0]))
	{
		WON_ParseAuthenticationMessage((unsigned char)c[0]);
	}
	else if (!strcmp(c, "status"))
	{
		SVC_Status();
	}
	else if (!strcmp(c, "getchallenge"))
	{
		SVC_GetChallenge();
	}
	else if (!_stricmp(c, "info"))
	{
		SVC_Info(FALSE);
	}
	else if (!_stricmp(c, "details"))
	{
		SVC_Info(TRUE);
	}
	else if (!_stricmp(c, "players")) // Player info request.
	{
		SVC_PlayerInfo();
	}
	else if (!_stricmp(c, "rules"))   // Rule info request.
	{
		SVC_RuleInfo();
	}
	else if (!strcmp(c, "connect"))  // Must include correct challenge #
	{
		SV_ConnectClient();
	}
	else if (!strcmp(c, "rcon"))
	{
		Host_RemoteCommand(&net_from);
	}
	else
	{
		// Just ignore it.
		Con_Printf("bad connectionless packet from %s:\n%s\n",
			NET_AdrToString(net_from), s);
	}
}

//============================================================================

/*
=================
SV_ReadPackets

Read's packets from clients and executes messages as appropriate.
=================
*/
void SV_ReadPackets( void )
{
	int			i;
	client_t* cl;
	float		start, end;
	float		time1, time2, time3, time4, time5, time6;
	float		packettime, exectime, processtime;

	SV_SetClientTimes();

	packettime = 0;
	exectime = 0;
	processtime = 0;

	if (host_speeds.value == 2)
		start = Sys_FloatTime();

	while (NET_GetPacket(NS_SERVER))
	{
		time1 = Sys_FloatTime();

		if (SV_FilterPacket())
		{
			SV_SendBan();	// tell them we aren't listening...
			continue;
		}

		time2 = Sys_FloatTime();

		if (host_speeds.value == 2)
			packettime += time2 - time1;

		// check for connectionless packet (0xffffffff) first
		if (*(int*)net_message.data == -1)
		{
			SV_ConnectionlessPacket();
			continue;
		}

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < svs.maxclientslimit; i++, cl++)
		{
			if (!cl->connected && !cl->active && !cl->spawned)
				continue;
			if (!NET_CompareAdr(net_from, cl->netchan.remote_address))
				continue;

			time3 = Sys_FloatTime();

			if (Netchan_Process(&cl->netchan))
			{	// this is a valid, sequenced packet, so process it
#if HLDC_MP
				svs.stats.packets++;
#endif
				cl->send_message = TRUE;	// reply at end of frame

				time4 = Sys_FloatTime();
				SV_ExecuteClientMessage(cl);
				time5 = Sys_FloatTime();

				if (host_speeds.value == 2)
					exectime += time5 - time4;
			}

			time6 = Sys_FloatTime();

			if (host_speeds.value == 2)
				processtime += time6 - time3;
		}

		if (i != MAX_CLIENTS)
			continue;

		// packet is not from a known client
		//	Con_Printf("%s:sequenced packet without connection\n"
		// , NET_AdrToString(net_from));
	}

	if (host_speeds.value == 2)
		end = Sys_FloatTime();
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in sv_timeout.GetFloat()
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts( void )
{
	int		    i;
	client_t* cl;
	float	    droptime;

	droptime = realtime - timeout.value;

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if (cl->fakeclient)
			continue;

		if ((cl->connected || cl->active || cl->spawned) &&
			(cl->netchan.last_received < droptime))
		{
			SV_BroadcastPrintf("%s timed out\n", cl->name);
			SV_DropClient(cl, FALSE);
		}
	}
}

int SV_CalcPing( client_t* cl )
{
	float		ping;
	int			i;
	int			count;
	client_frame_t* frame;

	if (cl->fakeclient)
		return 5;

	ping = 0;
	count = 0;

	frame = cl->frames;
	for (i = 0; i < SV_UPDATE_BACKUP; i++, frame++)
	{
		if (frame->ping_time > 0)
		{
			ping += frame->ping_time;
			count++;
		}
	}

	if (!count)
		return 9999;
	ping /= count;
	return (ping * 1000);
}

int SV_CalcPacketLoss( client_t* cl )
{
	int i;
	int count;
	int lost;
	int num_packets;
	int sequence;

	if (cl->fakeclient)
		return 0;

	count = 0;
	lost = 0;
	num_packets = SV_UPDATE_BACKUP / 2;
	sequence = cl->netchan.incoming_sequence - 1;

	for (i = 0; i < num_packets; i++, sequence--)
	{
		count++;
		if (cl->frames[sequence & SV_UPDATE_MASK].ping_time == -1.0f)
			lost++;
	}

	if (!count)
		return 100;

	return (float)lost * 100.0f / count;
}

/*
===================
SV_FullClientUpdate

sends all the info about *cl to *sb
===================
*/
void SV_FullClientUpdate( client_t* cl, sizebuf_t* sb )
{
	int		playernum;
	char info[MAX_INFO_STRING];

	playernum = cl - svs.clients;
	strcpy(info, cl->userinfo);
	Info_RemovePrefixedKeys(info, '_');

	MSG_WriteByte(sb, svc_updateuserinfo);
	MSG_WriteByte(sb, playernum);
	MSG_WriteLong(sb, cl->userid);
	MSG_WriteString(sb, info);

	if (cl->maxspeed != 0.0f)
	{
		MSG_WriteByte(sb, svc_clientmaxspeed);
		MSG_WriteByte(sb, playernum);
		MSG_WriteFloat(sb, cl->maxspeed);
	}
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram( void )
{
	SZ_Clear(&sv.datagram);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[MAX_MAP_LEAFS / 8];
void SV_AddToFatPVS( vec_t* org, mnode_t* node )
{
	int		i;
	byte* pvs;
	mclipplane_t* plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS((mleaf_t*)node, sv.worldmodel);
				for (i = 0; i < fatbytes; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}

		plane = node->plane;
		d = DotProduct(org, g_planeNormalTable[plane->normalindex].normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS(org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte* SV_FatPVS( float* org )
{
	fatbytes = (sv.worldmodel->numleafs + 31) >> 3;
	Q_memset(fatpvs, 0, fatbytes);
	SV_AddToFatPVS(org, sv.worldmodel->nodes);
	return fatpvs;
}

//=============================================================================

int SV_PointLeafnum( vec_t* p )
{
	mleaf_t* pLeaf;

	pLeaf = Mod_PointInLeaf(p, sv.worldmodel);
	if (pLeaf)
		return pLeaf - sv.worldmodel->leafs;

	return 0;
}

/*
==================
SV_WriteCustomEntityDeltaToClient

Writes part of a custom entity message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void SV_WriteCustomEntityDeltaToClient( entity_state_t* from, entity_state_t* to, sizebuf_t* msg, qboolean force )
{
	int		bits;
	int		i;
	int		rendermode;
	float	miss;
	byte	bytevalue;
	short	shortvalue;
	unsigned short ushortvalue;

	(void)msg;

// send an update
	bits = U_MOREBITS | U_EVENMOREBITS | U_CUSTOM;
	if (to->rendermode != from->rendermode)
		bits |= U_BEAM_TYPE;

	rendermode = (to->rendermode & 15);
	if (rendermode == kRenderNormal || rendermode == kRenderTransColor)
	{
		for (i = 0; i < 3; i++)
		{
			miss = to->origin[i] - from->origin[i];
			if (miss < -0.1f || miss > 0.1f)
				bits |= U_BEAM_STARTX << i;
		}

		if (rendermode == kRenderNormal)
		{
			for (i = 0; i < 3; i++)
			{
				miss = to->angles[i] - from->angles[i];
				if (miss < -0.1f || miss > 0.1f)
					bits |= U_BEAM_ENDX << i;
			}
		}
	}

	if ((rendermode == kRenderTransColor || rendermode == kRenderTransTexture) &&
		(to->sequence != from->sequence || to->skin != from->skin))
		bits |= U_BEAM_ENTS;

	if (from->modelindex != to->modelindex)
		bits |= U_BEAM_MODEL;

	if (from->scale != to->scale)
		bits |= U_BEAM_WIDTH;

	if (from->body != to->body)
		bits |= U_BEAM_NOISE;

	if (to->rendercolor.r != from->rendercolor.r ||
		to->rendercolor.g != from->rendercolor.g ||
		to->rendercolor.b != from->rendercolor.b ||
		to->renderfx != from->renderfx)
	{
		bits |= U_BEAM_RENDER;
	}

	if (from->renderamt != to->renderamt)
		bits |= U_BEAM_BRIGHTNESS;

	if (from->frame != to->frame)
		bits |= U_BEAM_FRAME;

	if ((int)from->animtime != (int)to->animtime)
		bits |= U_BEAM_SCROLL;

	if (to->number > 255)
		bits |= U_LONGENTITY;

	if (bits > 0xFF)
		bits |= U_MOREBITS;
	if (bits > 0xFFFF)
		bits |= U_EVENMOREBITS;
	if (bits > 0xFFFFFF)
		bits |= U_YETMOREBITS;

	//
	// write the message
	//
	if (!to->number)
		Sys_Error("Unset entity number");

	if (!bits && !force)
		return;		// nothing to send!

	bytevalue = bits;
	MSG_WriteBitByte(&bytevalue, 8);

	if (bits & U_MOREBITS)
	{
		bytevalue = bits >> 8;
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_EVENMOREBITS)
	{
		bytevalue = bits >> 16;
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_YETMOREBITS)
	{
		bytevalue = bits >> 24;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_LONGENTITY)
	{
		ushortvalue = to->number;
		MSG_WriteBitShort(&ushortvalue, 10);
	}
	else
	{
		bytevalue = to->number;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_STARTX)
	{
		shortvalue = to->origin[0] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_BEAM_STARTY)
	{
		shortvalue = to->origin[1] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_BEAM_STARTZ)
	{
		shortvalue = to->origin[2] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_BEAM_ENDX)
	{
		shortvalue = to->angles[0] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_BEAM_ENDY)
	{
		shortvalue = to->angles[1] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_BEAM_ENDZ)
	{
		shortvalue = to->angles[2] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}

	if (bits & U_BEAM_ENTS)
	{
		ushortvalue = to->sequence;
		MSG_WriteBitShort(&ushortvalue, 16);
		ushortvalue = to->skin;
		MSG_WriteBitShort(&ushortvalue, 16);
	}

	if (bits & U_BEAM_TYPE)
	{
		bytevalue = to->rendermode;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_MODEL)
	{
		ushortvalue = to->modelindex;
		MSG_WriteBitShort(&ushortvalue, 16);
	}

	if (bits & U_BEAM_WIDTH)
	{
		bytevalue = to->scale;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_NOISE)
	{
		bytevalue = to->body;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_RENDER)
	{
		bytevalue = to->rendercolor.r;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->rendercolor.g;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->rendercolor.b;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->renderfx;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_BRIGHTNESS)
	{
		bytevalue = to->renderamt;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_FRAME)
	{
		bytevalue = to->frame;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BEAM_SCROLL)
	{
		bytevalue = to->animtime;
		MSG_WriteBitByte(&bytevalue, 8);
	}
}

/*
==================
SV_WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void SV_WriteDelta( entity_state_t* from, entity_state_t* to, sizebuf_t* msg, qboolean force )
{
	int     bits, bboxbits;
	int		i;
	float   miss;
	byte	bytevalue;
	char	sbytevalue;
	short	shortvalue;
	unsigned short ushortvalue;
	qboolean aimentchanged;

	(void)msg;

	if (to->entityType != ENTITY_NORMAL)
	{
		SV_WriteCustomEntityDeltaToClient(from, to, msg, force);
		return;
	}

// send an update
	bits = 0;
	bboxbits = 0;

	for (i = 0; i < 3; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if (miss < -0.1f || miss > 0.1f)
			bits |= (U_ORIGIN1 << i);
	}

	if (to->angles[0] != from->angles[0])
		bits |= U_ANGLE1;

	if (to->angles[1] != from->angles[1])
		bits |= U_ANGLE2;

	if (to->angles[2] != from->angles[2])
		bits |= U_ANGLE3;

	if (to->movetype != from->movetype)
		bits |= U_MOVETYPE;

	if (to->colormap != from->colormap)
		bits |= U_COLORMAP;

	if (to->skin != from->skin || to->solid != from->solid)
		bits |= U_CONTENTS;

	if (to->frame != from->frame)
		bits |= U_FRAME;

	if (to->scale != from->scale)
		bits |= U_SCALE;

	if (to->effects != from->effects)
		bits |= U_EFFECTS;

	if (to->modelindex != from->modelindex )
		bits |= U_MODELINDEX;

	if (to->animtime != from->animtime || to->sequence != from->sequence)
		bits |= U_SEQUENCE;

	if (to->framerate != from->framerate)
		bits |= U_FRAMERATE;

	if (to->aiment != from->aiment)
		bits |= U_AIMENT;

	if (to->body != from->body)
		bits |= U_BODY;

	if (to->controller[0] != from->controller[0])
		bits |= U_CONTROLLER1;
	if (to->controller[1] != from->controller[1])
		bits |= U_CONTROLLER2;
	if (to->controller[2] != from->controller[2])
		bits |= U_CONTROLLER3;
	if (to->controller[3] != from->controller[3])
		bits |= U_CONTROLLER4;

	if (to->blending[0] != from->blending[0])
		bits |= U_BLENDING1;
	if (to->blending[1] != from->blending[1])
		bits |= U_BLENDING2;

	if (to->rendermode != from->rendermode ||
		to->renderamt != from->renderamt ||
		to->renderfx != from->renderfx ||
		to->rendercolor.r != from->rendercolor.r ||
		to->rendercolor.g != from->rendercolor.g ||
		to->rendercolor.b != from->rendercolor.b)
	{
		bits |= U_RENDER;
	}

	if (to->animtime != 0.0f && to->velocity[0] == 0 && to->velocity[1] == 0 && to->velocity[2] == 0)
		bits |= U_MONSTERMOVE;

	for (i = 0; i < 3; i++)
	{
		miss = to->mins[i] - from->mins[i];
		if (miss < -0.1f || miss > 0.1f)
			bboxbits |= (U_BBOXMINS1 << i);

		miss = to->maxs[i] - from->maxs[i];
		if (miss < -0.1f || miss > 0.1f)
			bboxbits |= (U_BBOXMAXS1 << i);
	}

	aimentchanged = to->aiment != from->aiment;
	if (aimentchanged)
		bboxbits |= U_BBOXAIMENT;

	if (to->movetype == MOVETYPE_FOLLOW && to->aiment)
		bits &= ~(U_ORIGIN1 | U_ORIGIN2 | U_ORIGIN3);
	else if (aimentchanged)
		bits |= U_ORIGIN1 | U_ORIGIN2 | U_ORIGIN3;

	if (to->number > 255)
		bits |= U_LONGENTITY;

	if (bits > 0xFF)
		bits |= U_MOREBITS;
	if (bits > 0xFFFF)
		bits |= U_EVENMOREBITS;
	if (bits > 0xFFFFFF)
		bits |= U_YETMOREBITS;

	if (bboxbits)
		bits |= (U_YETMOREBITS | U_EVENMOREBITS | U_MOREBITS | U_BBOXBITS);

	//
	// write the message
	//
	if (!to->number)
		Sys_Error("Unset entity number");

	if (!bits && !bboxbits && !force)
		return;		// nothing to send!

	bytevalue = bits;
	MSG_WriteBitByte(&bytevalue, 8);

	if (bits & U_MOREBITS)
	{
		bytevalue = bits >> 8;
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_EVENMOREBITS)
	{
		bytevalue = bits >> 16;
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_YETMOREBITS)
	{
		bytevalue = bits >> 24;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BBOXBITS)
	{
		bytevalue = bboxbits;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_LONGENTITY)
	{
		ushortvalue = to->number;
		MSG_WriteBitShort(&ushortvalue, 10);
	}
	else
	{
		bytevalue = to->number;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_MODELINDEX)
	{
		if (to->modelindex > 255)
		{
			bytevalue = 1;
			MSG_WriteBitByte(&bytevalue, 1);
			ushortvalue = to->modelindex;
			MSG_WriteBitShort(&ushortvalue, 10);
		}
		else
		{
			bytevalue = 0;
			MSG_WriteBitByte(&bytevalue, 1);
			bytevalue = to->modelindex;
			MSG_WriteBitByte(&bytevalue, 8);
		}
	}
	if (bits & U_FRAME)
	{
		bytevalue = to->frame;
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_MOVETYPE)
	{
		bytevalue = to->movetype;
		MSG_WriteBitByte(&bytevalue, 4);
	}
	if (bits & U_COLORMAP)
	{
		ushortvalue = to->colormap;
		MSG_WriteBitShort(&ushortvalue, 16);
	}

	if (bits & U_CONTENTS)
	{
		bytevalue = to->skin;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->solid;
		MSG_WriteBitByte(&bytevalue, 3);
	}

	if (bits & U_SCALE)
	{
		ushortvalue = to->scale * 256.0f;
		MSG_WriteBitShort(&ushortvalue, 16);
	}

	if (bits & U_EFFECTS)
	{
		bytevalue = to->effects;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_ORIGIN1)
	{
		shortvalue = to->origin[0] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_ANGLE1)
		MSG_WriteBitAngle(to->angles[0], 11);
	if (bits & U_ORIGIN2)
	{
		shortvalue = to->origin[1] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_ANGLE2)
		MSG_WriteBitAngle(to->angles[1], 13);
	if (bits & U_ORIGIN3)
	{
		shortvalue = to->origin[2] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bits & U_ANGLE3)
		MSG_WriteBitAngle(to->angles[2], 11);

	if (bits & U_SEQUENCE)
	{
		bytevalue = to->sequence;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->animtime * 100.0f;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_FRAMERATE)
	{
		sbytevalue = to->framerate * 16.0f;
		MSG_WriteSBitByte(&sbytevalue, 8);
	}

	if (bits & U_CONTROLLER1)
	{
		bytevalue = to->controller[0];
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_CONTROLLER2)
	{
		bytevalue = to->controller[1];
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_CONTROLLER3)
	{
		bytevalue = to->controller[2];
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_CONTROLLER4)
	{
		bytevalue = to->controller[3];
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BLENDING1)
	{
		bytevalue = to->blending[0];
		MSG_WriteBitByte(&bytevalue, 8);
	}
	if (bits & U_BLENDING2)
	{
		bytevalue = to->blending[1];
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_BODY)
	{
		bytevalue = to->body;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bits & U_RENDER)
	{
		bytevalue = to->rendermode;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->renderamt;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->rendercolor.r;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->rendercolor.g;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->rendercolor.b;
		MSG_WriteBitByte(&bytevalue, 8);
		bytevalue = to->renderfx;
		MSG_WriteBitByte(&bytevalue, 8);
	}

	if (bboxbits & U_BBOXMINS1)
	{
		shortvalue = to->mins[0] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bboxbits & U_BBOXMINS2)
	{
		shortvalue = to->mins[1] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bboxbits & U_BBOXMINS3)
	{
		shortvalue = to->mins[2] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bboxbits & U_BBOXMAXS1)
	{
		shortvalue = to->maxs[0] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bboxbits & U_BBOXMAXS2)
	{
		shortvalue = to->maxs[1] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}
	if (bboxbits & U_BBOXMAXS3)
	{
		shortvalue = to->maxs[2] * 8.0f;
		MSG_WriteSBitShort(&shortvalue, 16);
	}

	if (bboxbits & U_BBOXAIMENT)
	{
		ushortvalue = to->aiment;
		MSG_WriteBitShort(&ushortvalue, 10);
	}
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of a packet_entities_t to the message.
=============
*/
int SV_CreatePacketEntities( qboolean delta, client_t* client, packet_entities_t* to, sizebuf_t* msg )
{
	edict_t* ent;
	client_frame_t* fromframe;
	packet_entities_t* from;

	int oldnum, newnum;
	int oldindex;
	int newindex;
	int oldmax;
	unsigned int bits;
	unsigned int endmarker;
	byte bytevalue;
	unsigned short ushortvalue;

	// this is the frame that we are going to delta update from
	if (delta)
	{
		fromframe = &client->frames[client->delta_sequence & SV_UPDATE_MASK];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		MSG_WriteByte(msg, svc_deltapacketentities);
		MSG_WriteShort(msg, to->num_entities);
		MSG_WriteByte(msg, client->delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		MSG_WriteByte(msg, svc_packetentities);
		MSG_WriteShort(msg, to->num_entities);
	}

	newindex = 0;
	oldindex = 0;
	MSG_StartBitWriting(msg);

	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{
			SV_WriteDelta(&from->entities[oldindex], &to->entities[newindex], msg, FALSE);
			newindex++;
			oldindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			ent = EDICT_NUM(newnum);
			SV_WriteDelta(&ent->baseline, &to->entities[newindex], msg, TRUE);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			bits = U_REMOVE;

			if (oldnum > 255)
				bits |= U_LONGENTITY;

			if (bits > 0xFF)
				bits |= U_MOREBITS;
			if (bits > 0xFFFF)
				bits |= U_EVENMOREBITS;
			if (bits > 0xFFFFFF)
				bits |= U_YETMOREBITS;

			bytevalue = bits;
			MSG_WriteBitByte(&bytevalue, 8);

			if (bits & U_MOREBITS)
			{
				bytevalue = bits >> 8;
				MSG_WriteBitByte(&bytevalue, 8);
			}
			if (bits & U_EVENMOREBITS)
			{
				bytevalue = bits >> 16;
				MSG_WriteBitByte(&bytevalue, 8);
			}
			if (bits & U_YETMOREBITS)
			{
				bytevalue = bits >> 24;
				MSG_WriteBitByte(&bytevalue, 8);
			}

			oldindex++;

			if (bits & U_LONGENTITY)
			{
				ushortvalue = oldnum;
				MSG_WriteBitShort(&ushortvalue, 10);
			}
			else
			{
				bytevalue = oldnum;
				MSG_WriteBitByte(&bytevalue, 8);
			}

			continue;
		}
	}

	endmarker = 0;
	MSG_WriteBitLong(&endmarker, 32);
	MSG_EndBitWriting(msg);

	return msg->cursize;
}

void SV_EmitPacketEntities( client_t* client, packet_entities_t* to, sizebuf_t* msg )
{
	int deltasize;
	int fullsize;

	sv_packet_entities_message.data = sv_packet_entities_buffer;
	sv_packet_entities_message.maxsize = MAX_PACKET_ENTITIES_MESSAGE;
	sv_packet_entities_message.cursize = 0;
	sv_packet_entities_message.allowoverflow = TRUE;
	sv_packet_entities_message.overflowed = FALSE;

	sv_packet_entities_delta_message.data = sv_packet_entities_delta_buffer;
	sv_packet_entities_delta_message.maxsize = MAX_PACKET_ENTITIES_MESSAGE;
	sv_packet_entities_delta_message.cursize = 0;
	sv_packet_entities_delta_message.allowoverflow = TRUE;
	sv_packet_entities_delta_message.overflowed = FALSE;

	if (client->delta_sequence == -1)
	{
		SV_CreatePacketEntities(FALSE, client, to, msg);
	}
	else if (!sv_smartdelta.value || svs.maxclients < 2)
	{
		SV_CreatePacketEntities(TRUE, client, to, msg);
	}
	else
	{
		deltasize = SV_CreatePacketEntities(TRUE, client, to, &sv_packet_entities_message);
		fullsize = SV_CreatePacketEntities(FALSE, client, to, &sv_packet_entities_delta_message);

		if (deltasize < fullsize)
			MSG_WriteBuf(msg, sv_packet_entities_message.cursize, sv_packet_entities_message.data);
		else
			MSG_WriteBuf(msg, sv_packet_entities_delta_message.cursize, sv_packet_entities_delta_message.data);
	}
}

/*
=============
SV_WritePlayersToClient

=============
*/
qboolean SV_ShouldUpdatePing( int clientIndex )
{
	if (svs.maxclientslimit == 1)
		return host_framecount % 30 == 0;
	return clientIndex == host_framecount % svs.maxclientslimit;
}


void SV_WritePlayersToClient( client_t* client, byte* pvs, sizebuf_t* msg )
{
	int i, j;
	int playernum;
	int pflags;
	int msec;
	int value;
	short shortvalue;
	byte bytevalue;
	byte packetloss;
	byte physflags;
	qboolean visible;
	qboolean updateping;
	qboolean localplayer;
	client_t* cl;
	edict_t* ent;
	edict_t* clent;
	usercmd_t cmd;
	usercmd_t nullcmd;

	clent = client->edict;

	for (j = 0, cl = svs.clients; j < svs.maxclients; j++, cl++)
	{
		if (!cl->spawned && !cl->active)
			continue;

		ent = cl->edict;
		visible = TRUE;
		updateping = SV_ShouldUpdatePing(j);

		if (ent != clent &&
			!(client->spec_track && client->spec_track - 1 == j))
		{
			if (!cl->spectator)
			{
				for (i = 0; i < ent->num_leafs; i++)
				{
					if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7)))
						break;
				}

				if (i == ent->num_leafs)
					visible = FALSE;
			}
			else
				visible = FALSE;
		}

		if (!visible && !updateping)
			continue;

		pflags = g_PF_MSEC | g_PF_COMMAND;

		if (ent->v.modelindex != sv_playermodel)
		{
			if (ent->v.modelindex < 256)
				pflags |= g_PF_MODEL;
			else
				pflags |= g_PF_MODEL_LONG;
		}

		if (ent->v.weaponmodel)
			pflags |= g_PF_WEAPONMODEL_LONG;
		if (ent->v.sequence || ent->v.animtime)
			pflags |= g_PF_SEQUENCE;
		if (ent->v.gaitsequence)
			pflags |= g_PF_GAITSEQUENCE;
		if (ent->v.velocity[0] || ent->v.velocity[1])
			pflags |= g_PF_VELOCITYXY;
		if (ent->v.velocity[2])
			pflags |= g_PF_VELOCITY3;
		if (ent->v.effects)
			pflags |= g_PF_EFFECTS;
		if (ent->v.framerate != 1.0f)
			pflags |= g_PF_FRAMERATE;
		if (ent->v.clbasevelocity[0] || ent->v.clbasevelocity[1] || ent->v.clbasevelocity[2])
			pflags |= g_PF_BASEVELOCITY;

		for (i = 0; i < 4; i++)
		{
			if (ent->v.controller[i])
				pflags |= g_PF_CONTROLLER1 << i;
		}

		for (i = 0; i < 2; i++)
		{
			if (ent->v.blending[i])
				pflags |= g_PF_BLENDING1 << i;
		}

		if (ent->v.body)
			pflags |= g_PF_BODY;
		if (ent->v.skin)
			pflags |= g_PF_SKINNUM;
		if (ent->v.friction != 1.0f)
			pflags |= g_PF_FRICTION;
		if (ent->v.rendermode || ent->v.renderamt != 0.0f || ent->v.renderfx ||
			ent->v.rendercolor[0] != 0.0f || ent->v.rendercolor[1] != 0.0f ||
			ent->v.rendercolor[2] != 0.0f)
			pflags |= g_PF_RENDER;
		if (ent->v.health <= 0.0f)
			pflags |= g_PF_DEAD;
		if (ent->v.movetype != MOVETYPE_WALK)
			pflags |= g_PF_MOVETYPE;

		if (!cl->spectator)
		{
			localplayer = ent == clent;
			if (localplayer)
				pflags &= ~g_PF_MSEC;
		}
		else
		{
			localplayer = ent == clent;
			pflags &= g_PF_VELOCITYXY | g_PF_VELOCITY3;
		}

		if (localplayer && sv.active && cls.state == ca_active &&
			client->netchan.remote_address.type == NA_LOOPBACK && !fakelag.value)
		{
			pflags |= g_PF_VELOCITYXY | g_PF_VELOCITY3;
		}

		if (updateping && visible)
			pflags |= g_PF_PING;
		else if (updateping && !visible)
			pflags = g_PF_PING;

		physflags = 0;
		if (ent->v.flags & FL_ONTRAIN)
			physflags |= g_PF_ONTRAIN;
		if (ent->v.flags & FL_DUCKING)
			physflags |= g_PF_DUCKING;
		if (ent->v.flags & FL_FROZEN)
			physflags |= g_PF_FROZEN;
		if (ent->v.flags & FL_SPECTATOR)
			physflags |= g_PF_SPECTATOR;
		if (ent->v.flags & FL_WATERJUMP)
			physflags |= g_PF_WATERJUMP;

		MSG_WriteByte(msg, svc_playerinfo);
		if (pflags >= 0x100)
			pflags |= g_PF_MOREBITS;
		if (pflags >= 0x10000)
			pflags |= g_PF_EVENMOREBITS;
		if (pflags >= 0x1000000)
			pflags |= g_PF_YETMOREBITS;

		MSG_WriteByte(msg, pflags);
		if (pflags & g_PF_MOREBITS)
			MSG_WriteByte(msg, pflags >> 8);
		if (pflags & g_PF_EVENMOREBITS)
			MSG_WriteByte(msg, pflags >> 16);
		if (pflags & g_PF_YETMOREBITS)
			MSG_WriteByte(msg, pflags >> 24);

		MSG_StartBitWriting(msg);
		playernum = j;
		if (cl->spectator)
			playernum |= 0x40;
		MSG_WriteBitByte((byte*)&playernum, 6);
		MSG_WriteBitByte(&physflags, 5);

		for (i = 0; i < 3; i++)
		{
			value = ent->v.origin[i] * 32.0f;
			MSG_WriteSBitLong(&value, 18);
		}

		bytevalue = ent->v.frame;
		MSG_WriteBitByte(&bytevalue, 8);

		if (pflags & g_PF_COMMAND)
		{
			cmd = cl->lastcmd;
			if (ent->v.health <= 0.0f)
			{
				cmd.angles[0] = 0.0f;
				cmd.angles[1] = ent->v.angles[1];
				cmd.angles[0] = 0.0f;
			}
			cmd.buttons = 0;
			cmd.impulse = 0;
			memset(&nullcmd, 0, sizeof(nullcmd));
			MSG_WriteBitUsercmd(&cmd, &nullcmd);
		}

		if (pflags & g_PF_MSEC)
		{
			msec = (sv.time - cl->localtime) * 1000.0f;
			if (msec > 255)
				msec = 255;
			bytevalue = msec;
			MSG_WriteBitByte(&bytevalue, 8);
		}

		if (pflags & g_PF_VELOCITYXY)
		{
			shortvalue = ent->v.velocity[0] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 14);
			shortvalue = ent->v.velocity[1] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 14);
		}
		if (pflags & g_PF_VELOCITY3)
		{
			shortvalue = ent->v.velocity[2] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 14);
		}

		if (pflags & g_PF_MODEL)
		{
			bytevalue = ent->v.modelindex;
			MSG_WriteBitByte(&bytevalue, 8);
		}
		else if (pflags & g_PF_MODEL_LONG)
		{
			shortvalue = ent->v.modelindex;
			MSG_WriteBitShort((unsigned short*)&shortvalue, 10);
		}

		if (pflags & g_PF_SKINNUM)
		{
			bytevalue = ent->v.skin;
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_EFFECTS)
		{
			bytevalue = ent->v.effects;
			MSG_WriteBitByte(&bytevalue, 8);
		}

		if (pflags & (g_PF_WEAPONMODEL_LONG | g_PF_WEAPONMODEL))
		{
			value = SV_ModelIndex(pr_strings + ent->v.weaponmodel);
			if (pflags & g_PF_WEAPONMODEL_LONG)
			{
				shortvalue = value;
				MSG_WriteBitShort((unsigned short*)&shortvalue, 10);
			}
			else
			{
				bytevalue = value;
				MSG_WriteBitByte(&bytevalue, 8);
			}
		}

		if (pflags & g_PF_MOVETYPE)
		{
			bytevalue = ent->v.movetype;
			MSG_WriteBitByte(&bytevalue, 4);
		}
		if (pflags & g_PF_SEQUENCE)
		{
			bytevalue = ent->v.sequence;
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.animtime * 100.0f;
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_RENDER)
		{
			bytevalue = ent->v.rendermode;
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.renderamt;
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.rendercolor[0];
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.rendercolor[1];
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.rendercolor[2];
			MSG_WriteBitByte(&bytevalue, 8);
			bytevalue = ent->v.renderfx;
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_FRAMERATE)
		{
			bytevalue = ent->v.framerate * 16.0f;
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_BODY)
		{
			bytevalue = ent->v.body;
			MSG_WriteBitByte(&bytevalue, 8);
		}

		if (pflags & g_PF_CONTROLLER1)
		{
			bytevalue = ent->v.controller[0];
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_CONTROLLER2)
		{
			bytevalue = ent->v.controller[1];
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_CONTROLLER3)
		{
			bytevalue = ent->v.controller[2];
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_CONTROLLER4)
		{
			bytevalue = ent->v.controller[3];
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_BLENDING1)
		{
			bytevalue = ent->v.blending[0];
			MSG_WriteBitByte(&bytevalue, 8);
		}
		if (pflags & g_PF_BLENDING2)
		{
			bytevalue = ent->v.blending[1];
			MSG_WriteBitByte(&bytevalue, 8);
		}

		if (pflags & g_PF_BASEVELOCITY)
		{
			shortvalue = ent->v.clbasevelocity[0] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 16);
			shortvalue = ent->v.clbasevelocity[1] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 16);
			shortvalue = ent->v.clbasevelocity[2] * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 16);
		}
		if (pflags & g_PF_FRICTION)
		{
			shortvalue = ent->v.friction * 8.0f;
			MSG_WriteSBitShort(&shortvalue, 16);
		}
		if (pflags & g_PF_PING)
		{
			shortvalue = SV_CalcPing(cl);
			MSG_WriteBitShort((unsigned short*)&shortvalue, 12);
			packetloss = SV_CalcPacketLoss(cl);
			if (packetloss > 63)
				packetloss = 63;
			if (packetloss < 0)
				packetloss = 0;
			bytevalue = packetloss;
			MSG_WriteBitByte(&bytevalue, 6);
		}
		if (pflags & g_PF_GAITSEQUENCE)
		{
			bytevalue = ent->v.gaitsequence;
			MSG_WriteBitByte(&bytevalue, 8);
		}

		MSG_EndBitWriting(msg);
	}
}

typedef struct full_packet_entities_s
{
	int num_entities;
	entity_state_t entities[MAX_PACKET_ENTITIES];
} full_packet_entities_t;

/*
==================
SV_AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
==================
*/
void SV_AddToFullPack( full_packet_entities_t* pack, int e, unsigned char* pSet )
{
	int					i;
	edict_t* ent;
	entity_state_t*	state;

	ent = &sv.edicts[e];

	// don't send if flagged for NODRAW
	if (ent->v.effects == EF_NODRAW)
		return;

	// Ignore ents without valid / visible models
	if (!ent->v.modelindex || !*(pr_strings + ent->v.model))
		return;

	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if (pSet)
	{
		if (ent->num_leafs < 0)
		{
			if (!CM_HeadnodeVisible(&sv.worldmodel->nodes[ent->leafnums[0]], pSet))
				return;
		}
		else
		{
			for (i = 0; i < ent->num_leafs; i++)
				if (pSet[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7)))
					break;
			if (i == ent->num_leafs)
				return;
		}
	}

	if (pack->num_entities >= MAX_PACKET_ENTITIES)
	{
		Con_DPrintf("Too many entities in visible packet list.\n");
		return;
	}
	state = &pack->entities[pack->num_entities];
	pack->num_entities++;

	state->number = e;
	state->flags = 0;
	state->entityType = ENTITY_NORMAL;

	// flag custom entities
	if (ent->v.flags & FL_CUSTOMENTITY)
		state->entityType = ENTITY_BEAM;
		
	VectorCopy(ent->v.origin, state->origin);
	VectorCopy(ent->v.angles, state->angles);
	VectorCopy(ent->v.mins, state->mins);
	VectorCopy(ent->v.maxs, state->maxs);
	VectorCopy(ent->v.velocity, state->velocity);

	state->modelindex = ent->v.modelindex;
	state->frame = ent->v.frame;
	state->skin = ent->v.skin;
	state->colormap = ent->v.colormap;
	state->solid = ent->v.solid;
	state->effects = ent->v.effects;
	state->scale = ent->v.scale;
	state->movetype = ent->v.movetype;
	state->animtime = ent->v.animtime;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;
	if (ent->v.aiment)
		state->aiment = NUM_FOR_EDICT(ent->v.aiment);
	else
		state->aiment = 0;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i] = ent->v.blending[i];
	}

	state->rendermode = ent->v.rendermode;
	state->renderamt = ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor[0];
	state->rendercolor.g = ent->v.rendercolor[1];
	state->rendercolor.b = ent->v.rendercolor[2];
}

/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a svc_packetentities messages and
svc_playerinfo messages
=============
*/

void SV_WriteEntitiesToClient( client_t* client, sizebuf_t* msg )
{
	int		i;
	byte* pvs;
	vec3_t	org;
	packet_entities_t* pack;
	int		entsinpacket;
	const edict_t* clent;
	client_frame_t* frame;
	full_packet_entities_t fullpack;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.incoming_sequence & SV_UPDATE_MASK];
	clent = client->edict;

	// find the client's PVS
	if (client->pViewEntity)
	{
		VectorAdd(client->pViewEntity->v.origin, client->pViewEntity->v.view_ofs, org);
	}
	else
	{
		VectorAdd(clent->v.origin, clent->v.view_ofs, org);
	}
	pvs = SV_FatPVS(org);

	// send over the players in the PVS
	SV_WritePlayersToClient(client, pvs, msg);

	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;


	fullpack.num_entities = 0;
	for (i = svs.maxclients + 1; i < sv.num_edicts; i++)
	{
		// add to the packetentities
		SV_AddToFullPack(&fullpack, i, pvs);
	}

	entsinpacket = fullpack.num_entities;
	pack->num_entities = entsinpacket;

	if (entsinpacket == 0)
		entsinpacket = 1;

	SV_AllocPacketEntities(frame, entsinpacket);
	if (fullpack.num_entities == 0)
		pack->num_entities = 0;

	if (pack->num_entities)
		memcpy(pack->entities, fullpack.entities, sizeof(entity_state_t) * pack->num_entities);
	else
		memset(pack->entities, 0, sizeof(entity_state_t));

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities(client, pack, msg);
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts( void )
{
	int		e;
	edict_t* ent;

	for (e = 1; e < sv.num_edicts; e++)
	{
		ent = &sv.edicts[e];

		ent->v.effects &= ~EF_MUZZLEFLASH;
		ent->v.effects &= ~EF_NOINTERP;
	}
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage( client_t* client, sizebuf_t* msg )
{
	int		bits;
	int		i;
	edict_t* ent;

	ent = client->edict;

	// send the chokecount for r_netgraph
	if (client->chokecount)
	{
		MSG_WriteByte(msg, svc_chokecount);
		MSG_WriteByte(msg, client->chokecount);
		client->chokecount = 0;
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if (ent->v.fixangle)
	{
		if (ent->v.fixangle == 2)
		{
			MSG_WriteByte(msg, svc_addangle);
			MSG_WriteHiresAngle(msg, ent->v.avelocity[1]);
			ent->v.avelocity[1] = 0;
		}
		else
		{
			MSG_WriteByte(msg, svc_setangle);
			for (i = 0; i < 3; i++)
				MSG_WriteHiresAngle(msg, ent->v.angles[i]);
		}
		ent->v.fixangle = 0;
	}

	bits = 0;

	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;

	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

	if (ent->v.weapons)
		bits |= SU_WEAPONS;

	if (ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;

	if (ent->v.waterlevel >= 2)
		bits |= SU_INWATER;

	if (ent->v.waterlevel >= 3)
		bits |= SU_FULLYINWATER;

	if (ent->v.viewmodel)
		bits |= SU_ITEMS;

	for (i = 0; i < 3; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1 << i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1 << i);
	}

// send the data

	MSG_WriteByte(msg, svc_clientdata);
	MSG_WriteShort(msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar(msg, ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar(msg, ent->v.idealpitch);

	for (i = 0; i < 3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
			MSG_WriteHiresAngle(msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1 << i))
			MSG_WriteChar(msg, ent->v.velocity[i] / 16);
	}

	if (bits & SU_WEAPONS)
		MSG_WriteLong(msg, ent->v.weapons);

	if (bits & SU_ITEMS)
		MSG_WriteShort(msg, SV_ModelIndex(pr_strings + ent->v.viewmodel));

	MSG_WriteShort(msg, ent->v.health);
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram( client_t* client )
{
	byte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;
	msg.allowoverflow = TRUE;
	msg.overflowed = FALSE;

	MSG_WriteByte(&msg, svc_time);
	MSG_WriteFloat(&msg, sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage(client, &msg);

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	SV_WriteEntitiesToClient(client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
		Con_Printf("WARNING: datagram overflowed for %s\n", client->name);
	else
		SZ_Write(&msg, client->datagram.data, client->datagram.cursize);
	SZ_Clear(&client->datagram);

	if (scr_graphmean.value)
	{
		nDatagrams++;
		nDatagramBytesSent += msg.cursize;
	}
	else
	{
		if (nDatagramBytesSent < msg.cursize)
			nDatagramBytesSent = msg.cursize;
	}

	if (msg.overflowed)
	{
		Con_Printf("WARNING: msg overflowed for %s\n", client->name);
		SZ_Clear(&msg);
	}

// send the datagram
	Netchan_Transmit(&client->netchan, msg.cursize, buf);

	return TRUE;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages( void )
{
	int			i, j;
	client_t* client;

// check for changes to be sent over the reliable streams to all clients
	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (!host_client->edict)
			continue;

		if (host_client->sendinfo && host_client->sendinfo_time <= sv.time)
		{
			host_client->sendinfo = FALSE;
			host_client->sendinfo_time = sv.time + 1.0f;
			SV_ExtractFromUserinfo(host_client);
			SV_FullClientUpdate(host_client, &sv.reliable_datagram);
		}

		if (host_client->fakeclient)
			continue;
		if (!host_client->active && !host_client->connected)
			continue;
		if (!sv_gpNewUserMsgs)
			continue;

		SV_SendUserReg(&host_client->netchan.message);
	}

	// Link new user messages to the global messages chain
	if (sv_gpNewUserMsgs)
	{
		if (sv_gpUserMsgs)
		{
			UserMsg* pMsg = sv_gpUserMsgs;
			while (pMsg->next)
			{
				pMsg = pMsg->next;
			}
			pMsg->next = sv_gpNewUserMsgs;
		}
		else
		{
			sv_gpUserMsgs = sv_gpNewUserMsgs;
		}
		sv_gpNewUserMsgs = NULL;
	}

	if (sv.datagram.overflowed)
	{
		Con_DPrintf("sv.datagram overflowed!\n");
		SZ_Clear(&sv.datagram);
	}

	// append the broadcast messages to each client messages
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || client->fakeclient)
			continue;

		SZ_Write(&client->netchan.message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);
		SZ_Write(&client->datagram, sv.datagram.data, sv.datagram.cursize);
	}

	SZ_Clear(&sv.reliable_datagram);
	SZ_Clear(&sv.datagram);
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages( void )
{
	int			i;

// update frags, names, etc
	SV_UpdateToReliableMessages();

	nReliableBytesSent = 0;
	nDatagramBytesSent = 0;
	nReliables = 0;
	nDatagrams = 0;
	bUnreliableOverflow = FALSE;

// build individual updates
	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (!host_client->active && !(host_client->connected || host_client->spawned))
			continue;

		if (host_client->fakeclient)
			continue;

		// if the reliable message overflowed,
		// drop the client
		if (host_client->netchan.message.overflowed)
		{
			SZ_Clear(&host_client->netchan.message);
			SZ_Clear(&host_client->datagram);
			SV_BroadcastPrintf("%s overflowed\n", host_client->name);
			Con_Printf("WARNING: reliable overflow for %s\n", host_client->name);
			SV_DropClient(host_client, FALSE);
			host_client->send_message = TRUE;
			host_client->netchan.cleartime = 0;	// don't choke this message
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!host_client->send_message)
			continue;

		host_client->send_message = FALSE;	// try putting this after choke?

		if (!Netchan_CanPacket(&host_client->netchan))
		{
			host_client->chokecount++;
			continue;		// bandwidth choke
		}

		if (host_client->active)
			SV_SendClientDatagram(host_client);
		else
			Netchan_Transmit(&host_client->netchan, 0, NULL);
	}

	SCR_UpdateNetUsage(nReliableBytesSent, nReliables, FALSE);

	SCR_UpdateNetUsage(nDatagramBytesSent,
		bUnreliableOverflow ? MAX_MSGLEN + 1 : nDatagrams, TRUE);

// clear muzzle flashes
	SV_CleanupEnts();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
=======================
SV_ExtractFromUserinfo
=======================
*/
void SV_ExtractFromUserinfo( client_t* client )
{
	client_t* other;
	char* value;
	char* p;
	char* q;
	char newname[80];
	int duplicate;
	int i;

	gEntityInterface.pfnClientUserInfoChanged(client->edict, client->userinfo);

	value = Info_ValueForKey(client->userinfo, "name");
	strncpy(newname, value, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = 0;

	p = newname;
	while (*p == ' ')
		p++;
	if (p != newname && *p)
	{
		q = newname;
		while (*p)
			*q++ = *p++;
		*q = 0;
	}

	p = newname + strlen(newname) - 1;
	if (p != newname)
	{
		while (*p == ' ' && --p != newname)
			;
	}
	p[1] = 0;

	if (strcmp(value, newname))
	{
		Info_SetValueForKey(client->userinfo, "name", newname, MAX_INFO_STRING);
		value = Info_ValueForKey(client->userinfo, "name");
	}

	if (!value[0] || !Q_stricmp(value, "console"))
	{
		Info_SetValueForKey(client->userinfo, "name", "unnamed", MAX_INFO_STRING);
		value = Info_ValueForKey(client->userinfo, "name");
	}

	duplicate = 1;
	for (;;)
	{
		for (i = 0, other = svs.clients; i < svs.maxclients; i++, other++)
		{
			if (other->active && other->spawned && other != client &&
				!Q_stricmp(other->name, value))
				break;
		}

		if (i == svs.maxclients)
			break;

		if (strlen(value) > 31)
			value[28] = 0;

		if (value[0] == '(')
		{
			if (value[2] == ')')
				value += 3;
			else if (value[3] == ')')
				value += 4;
		}

		sprintf(newname, "(%d)%-0.40s", duplicate, value);
		Info_SetValueForKey(client->userinfo, "name", newname, MAX_INFO_STRING);
		value = Info_ValueForKey(client->userinfo, "name");
		duplicate++;
	}

	strncpy(client->name, value, sizeof(client->name) - 1);

	value = Info_ValueForKey(client->userinfo, "rate");
	if (strlen(value))
	{
		i = atoi(value);
		if (i < 500)
			i = 500;
		else if (i > 10000)
			i = 10000;
		client->netchan.rate = (float)i;
	}

	value = Info_ValueForKey(client->userinfo, "topcolor");
	if (strlen(value))
		client->topcolor = atoi(value);

	value = Info_ValueForKey(client->userinfo, "bottomcolor");
	if (strlen(value))
		client->bottomcolor = atoi(value);
}

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex( char* name )
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i = 0; i < MAX_MODELS && sv.model_precache[i]; i++)
		if (!strcmp(sv.model_precache[i], name))
			return i;
	if (i == MAX_MODELS || !sv.model_precache[i])
		Sys_Error("SV_ModelIndex: model %s not precached", name);
	return i;
}

/*
================
SV_FlushSignon

Moves to the next signon buffer if needed
================
*/
void SV_FlushSignon( void )
{
	if (sv.signon.cursize < sv.signon.maxsize - 100)
		return;

	if (sv.num_signon_buffers == MAX_SIGNON_BUFFERS - 1)
		Sys_Error("sv.num_signon_buffers == MAX_SIGNON_BUFFERS-1");

	sv.signon_buffer_size[sv.num_signon_buffers - 1] = sv.signon.cursize;
	sv.signon.data = sv.signon_buffers[sv.num_signon_buffers];
	sv.num_signon_buffers++;
	sv.signon.cursize = 0;
}

/*
================
SV_SendResourceListBlock_f

================
*/
void SV_SendResourceListBlock_f( void )
{
	int		n;
	int		savepos;
	unsigned short u;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!host_client->connected)
	{
		Con_Printf("resourcelist not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (atoi(Cmd_Argv(1)) != svs.spawncount)
	{
		Con_Printf("SV_SendResourceListBlock_f from different level\n");
		SV_New_f();
		return;
	}

	n = atoi(Cmd_Argv(2));

	MSG_WriteByte(&host_client->netchan.message, svc_resourcelist);
	MSG_WriteShort(&host_client->netchan.message, sv.num_resources);
	MSG_WriteShort(&host_client->netchan.message, n);

	savepos = host_client->netchan.message.cursize;
	MSG_WriteShort(&host_client->netchan.message, 0);

	for (n; n < sv.num_resources; n++)
	{
		if (host_client->netchan.message.cursize >= host_client->netchan.message.maxsize - 512)
			break;

		MSG_WriteByte(&host_client->netchan.message, sv.resources[n].type);
		MSG_WriteString(&host_client->netchan.message, sv.resources[n].szFileName);
		MSG_WriteShort(&host_client->netchan.message, sv.resources[n].nIndex);
		MSG_WriteLong(&host_client->netchan.message, 1000);
		MSG_WriteByte(&host_client->netchan.message, sv.resources[n].ucFlags);
	}

	u = (unsigned short)n;
	memcpy(&host_client->netchan.message.data[savepos], &u, sizeof(u));

	if (sv.num_resources > n)
	{
		MSG_WriteByte(&host_client->netchan.message, svc_stufftext);
		MSG_WriteString(&host_client->netchan.message, va("cmd resourcelist %i %i\n", svs.spawncount, n));
	}
}

/*
================
SV_AddResource

Adds a new resource to the server resource list
================
*/
void SV_AddResource( resourcetype_t type, const char *name, int size, int flags, int index )
{
	resource_t*	r;

	r = &sv.resources[sv.num_resources];
	if (sv.num_resources >= MAX_RESOURCES)
		Sys_Error("Too many resources on server.");
	sv.num_resources++;

	r->type = type;
	strcpy(r->szFileName, name);
	r->ucFlags |= (byte)(flags ? RES_FATALIFMISSING : 0);
	r->nIndex = index;
}

/*
================
SV_CreateResourceList

Creates a common list of all server resources
================
*/
void SV_Customization( client_t* pPlayer, resource_t* pResource, qboolean bSkipPlayer )
{
	Sys_Error("Customizations");
}

void SV_CreateResourceList( void )
{
	char** s;
	int		ffirstsent = 0;
	int		i;

	sv.num_resources = 0;

	// Add sound files to svc_resourcelist
	i = 1;
	s = &sv.sound_precache[i];
	while (*s)
	{
		if (**s != CHAR_SENTENCE)
		{
			if (svs.maxclients > 1)
				COM_FileSize(va("sound/%s", *s));

			SV_AddResource(t_sound, *s, 0, 0, i);
		}
		else if (!ffirstsent)
		{
			ffirstsent = 1;
			SV_AddResource(t_sound, "!", 0, RES_FATALIFMISSING, i);
		}

		i++;
		s++;
	}

	// Add model files to svc_resourcelist
	i = 1;
	s = &sv.model_precache[i];
	while (*s)
	{
		if (svs.maxclients > 1)
			COM_FileSize(*s);

		SV_AddResource(t_model, *s, 0, RES_FATALIFMISSING, i);

		i++;
		s++;
	}

	// Add decals to svc_resourcelist
	for (i = 0; i < sv_decalnamecount; i++)
	{
		SV_AddResource(t_decal, sv_decalnames[i].name, Draw_DecalSize(i), 0, i);
	}

	// Add generic files to svc_resourcelist
	i = 1;
	s = &sv.generic_precache[i];
	while (*s)
	{
		if (svs.maxclients > 1)
			COM_FileSize(*s);

		SV_AddResource(t_generic, *s, 0, RES_FATALIFMISSING, i);

		i++;
		s++;
	}
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline( void )
{
	int			i;
	edict_t* svent;
	int			entnum;

	for (entnum = 0; entnum < sv.num_edicts; entnum++)
	{
	// get the current server version
		svent = &sv.edicts[entnum];
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

		svent->baseline.entityType = ENTITY_NORMAL;
		if (svent->v.flags & FL_CUSTOMENTITY)
			svent->baseline.entityType = ENTITY_BEAM;

	//
	// create entity baseline
	//
		VectorCopy(svent->v.origin, svent->baseline.origin);
		VectorCopy(svent->v.angles, svent->baseline.angles);
		VectorCopy(svent->v.mins, svent->baseline.mins);
		VectorCopy(svent->v.maxs, svent->baseline.maxs);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		svent->baseline.scale = svent->v.scale;
		svent->baseline.solid = svent->v.solid;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("models/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(pr_strings + svent->v.model);
		}

		svent->baseline.rendermode = svent->v.rendermode;
		svent->baseline.renderamt = svent->v.renderamt;
		svent->baseline.rendercolor.r = svent->v.rendercolor[0];
		svent->baseline.rendercolor.g = svent->v.rendercolor[1];
		svent->baseline.rendercolor.b = svent->v.rendercolor[2];
		svent->baseline.renderfx = svent->v.renderfx;

		SV_FlushSignon();

	//
	// add to the message
	//
		MSG_WriteByte(&sv.signon, svc_spawnbaseline);
		MSG_WriteShort(&sv.signon, entnum);

		MSG_WriteByte(&sv.signon, svent->baseline.entityType);
		MSG_WriteShort(&sv.signon, svent->baseline.modelindex);
		MSG_WriteByte(&sv.signon, svent->baseline.sequence);
		MSG_WriteByte(&sv.signon, svent->baseline.frame);

		if (svent->baseline.entityType == ENTITY_NORMAL)
			MSG_WriteWord(&sv.signon, svent->baseline.scale * 256);
		else
			MSG_WriteByte(&sv.signon, svent->baseline.scale);

		MSG_WriteByte(&sv.signon, svent->baseline.colormap);
		MSG_WriteShort(&sv.signon, svent->baseline.skin);
		MSG_WriteByte(&sv.signon, svent->baseline.solid);

		for (i = 0; i < 3; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
			MSG_WriteFloat(&sv.signon, svent->baseline.angles[i]);
			MSG_WriteCoord(&sv.signon, svent->baseline.mins[i]);
			MSG_WriteCoord(&sv.signon, svent->baseline.maxs[i]);
		}

		MSG_WriteByte(&sv.signon, svent->v.rendermode);
		MSG_WriteByte(&sv.signon, svent->v.renderamt);
		MSG_WriteByte(&sv.signon, svent->v.rendercolor[0]);
		MSG_WriteByte(&sv.signon, svent->v.rendercolor[1]);
		MSG_WriteByte(&sv.signon, svent->v.rendercolor[2]);
		MSG_WriteByte(&sv.signon, svent->v.renderfx);
	}
}

void SV_SetLogAddress_f( void )
{
	char* address;
	int port;
	char string[MAX_PATH];
	netadr_t adr;

	if (Cmd_Argc() != 3)
	{
		Con_Printf("logaddress:  usage\nlogaddress ip port\n");
		if (svs.log.active)
			Con_Printf("current:  %s\n", NET_AdrToString(svs.log.net_address));
		return;
	}

	address = Cmd_Argv(1);
	port = atoi(Cmd_Argv(2));
	if (!port)
	{
		Con_Printf("logaddress:  must specify a valid port\n");
		return;
	}

	if (!address || !address[0])
	{
		Con_Printf("logaddress:  unparseable address\n");
		return;
	}

	sprintf(string, "%s:%i", address, port);
	if (!NET_StringToAdr(string, &adr))
	{
		Con_Printf("logaddress:  unable to resolve %s\n", string);
		return;
	}

	svs.log.net_log = TRUE;
	svs.log.net_address = adr;
	Con_Printf("logaddress:  %s\n", NET_AdrToString(adr));
}

void SV_ServerLog_f( void )
{
	char* arg;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("usage:  log < on | off >\n");
		if (svs.log.active)
			Con_Printf("currently logging\n");
		else
			Con_Printf("not currently logging\n");
		return;
	}

	arg = Cmd_Argv(1);
	if (!_stricmp(arg, "off"))
	{
		if (svs.log.active)
		{
			if (svs.log.file)
				Log_Close();
			Con_Printf("Server logging disabled.\n");
			svs.log.active = FALSE;
		}
	}
	else if (!_stricmp(Cmd_Argv(1), "on"))
	{
		svs.log.active = TRUE;
		Log_Open();
	}
	else
	{
		Con_Printf("log:  unknown parameter %s, 'on' and 'off' are valid\n", Cmd_Argv(1));
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand( char* fmt, ... )
{
	va_list		argptr;
	char		string[1024];
	char		data[128];
	sizebuf_t	msg;
	int			i;
	client_t* cl;

	msg.data = data;
	msg.maxsize = sizeof(data);
	msg.cursize = 0;

	if (!sv.active)
		return;

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&msg, svc_stufftext);
	MSG_WriteString(&msg, string);

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((!cl->active && !cl->connected && !cl->spawned) || cl->fakeclient)
			continue;

		SZ_Write(&cl->netchan.message, msg.data, msg.cursize);
	}
}

/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect( void )
{
	SV_BroadcastCommand("reconnect\n");

	/*if (cls.state != ca_dedicated)
		Cbuf_InsertText("reconnect\n");
	*/
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms( SAVERESTOREDATA* pSaveData )
{
	int		i;

	svs.serverflags = gGlobalVariables.serverflags;

	gEntityInterface.pfnParmsChangeLevel();

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (!host_client->active && !host_client->connected)
			continue;

	// call the progs to get default spawn parms for the new client
		host_client->saveSize = pSaveData->size;
		pSaveData->currentIndex = NUM_FOR_EDICT(host_client->edict);
		gEntityInterface.pfnSave(host_client->edict, pSaveData);
	}
}

/*
================
SV_ActivateServer

================
*/
void SV_ActivateServer( qboolean runPhysics )
{
	int i;

	Cvar_Set("sv_newunit", "0");

	// Activate the DLL server code
	gEntityInterface.pfnServerActivate(sv.edicts, sv.num_edicts, svs.maxclients);

	sv.active = TRUE;
	// all setup is completed, any further precache statements are errors
	sv.state = ss_active;

	if (runPhysics)
	{
		host_frametime = 0.1f;
		SV_Physics();
	}
	else
	{
		host_frametime = 0.001f;
	}

	SV_Physics();

// create a baseline for more efficient communications
	SV_CreateBaseline();

	// update signon buffer size
	sv.signon_buffer_size[sv.num_signon_buffers - 1] = sv.signon.cursize;

	SV_CreateResourceList();

// send serverinfo to all connected clients
	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
	{
		if (host_client->active || host_client->connected)
		{
			if (svs.maxclients < 2)
				SV_SendServerinfo(host_client);

			// Send usermsgs
			if (sv_gpUserMsgs && !host_client->fakeclient)
			{
				UserMsg* pTemp = sv_gpNewUserMsgs;
				sv_gpNewUserMsgs = sv_gpUserMsgs;
				SV_SendUserReg(&host_client->netchan.message);
				sv_gpNewUserMsgs = pTemp;
			}
		}
	}
}

/*
================
SV_Active

True when a server we drive the timestep for is running
================
*/
qboolean SV_Active( void )
{
	return sv.active;
}

/*
================
SV_ReallocateDynamicData

Allocate the per-edict scratch buffers used by the push physics
================
*/
void SV_ReallocateDynamicData( void )
{
	int max_edicts;

	max_edicts = sv.max_edicts;
	if (!max_edicts)
		return;

	if (g_moved_edict)
		Con_Printf("Reallocate on moved_edict\n");
	g_moved_edict = (edict_t**)MnemoAllocDbg(sizeof(edict_t*) * max_edicts, __FILE__, __LINE__);
	memset(g_moved_edict, 0, sizeof(edict_t*) * max_edicts);

	if (g_moved_from)
		Con_Printf("Reallocate on moved_from\n");
	g_moved_from = (vec3_t*)MnemoAllocDbg(sizeof(vec3_t) * max_edicts, __FILE__, __LINE__);
	memset(g_moved_from, 0, sizeof(vec3_t) * max_edicts);

	if (g_playertouch)
		Con_Printf("Reallocate on playertouch\n");
	g_playertouch = (byte*)MnemoAllocDbg((max_edicts + 7) / 8, __FILE__, __LINE__);
	memset(g_playertouch, 0, (max_edicts + 7) / 8);
}

/*
================
Host_DeallocateDynamicData

Release the per-edict scratch buffers before changing their capacity
================
*/
void Host_DeallocateDynamicData( void )
{
	if (g_moved_edict)
		free(g_moved_edict);
	g_moved_edict = NULL;

	if (g_moved_from)
		free(g_moved_from);
	g_moved_from = NULL;

	if (g_playertouch)
		free(g_playertouch);
	g_playertouch = NULL;
}

/*
================
SV_ClearPacketEntities

Release the delta entity buffer attached to a single client frame
================
*/
void SV_ClearPacketEntities( client_frame_t* frame )
{
	if (frame)
	{
		if (frame->entities.entities)
			free(frame->entities.entities);

		frame->entities.entities = NULL;
		frame->entities.num_entities = 0;
	}
}

/*
================
SV_AllocPacketEntities

Grow and clear the delta entity buffer attached to a client frame
================
*/
void SV_AllocPacketEntities( client_frame_t* frame, int count )
{
	packet_entities_t* entities;
	int size;

	if (frame)
	{
		entities = &frame->entities;

		if (!entities->entities || entities->max_entities < count)
		{
			size = sizeof(entity_state_t) * count;
			entities->entities = (entity_state_t*)DebugRealloc(entities->entities, size, __FILE__, __LINE__);
			if (!entities->entities)
				Sys_Error("Failed to allocate space for %i packet entities\n", count);

			entities->max_entities = count;
		}
		else
		{
			size = sizeof(entity_state_t) * count;
		}

		memset(entities->entities, 0, size);
		entities->num_entities = count;
	}
}

/*
================
SV_ClearFrames

Free a client's frame ring and everything hanging off it
================
*/
void SV_ClearFrames( client_frame_t** frames )
{
	client_frame_t* pframe;
	int i;

	pframe = *frames;
	if (pframe)
	{
		for (i = 0; i < SV_UPDATE_BACKUP; i++)
			SV_ClearPacketEntities(&pframe[i]);

		free(*frames);
		*frames = NULL;
	}
}

/*
================
SV_AllocClientFrames

Allocate the frame ring for every client slot
================
*/
void SV_AllocClientFrames( void )
{
	client_t* cl;
	int i;

	for (i = 0, cl = svs.clients; i < svs.maxclientslimit; i++, cl++)
	{
		cl->frames = (client_frame_t*)MnemoAllocDbg(sizeof(client_frame_t) * SV_UPDATE_BACKUP, __FILE__, __LINE__);
		memset(cl->frames, 0, sizeof(client_frame_t) * SV_UPDATE_BACKUP);
	}
}

/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
int SV_SpawnServer( qboolean bIsDemo, char* server, char* startspot )
{
	edict_t* ent;
	client_t* client;
	int			i;
	int			edictBytes;
	int			edictReserveSize;

//
// tell all connected clients that we are going to a new level
//
	if (sv.active && svs.maxclients > 1)
	{
		SV_BroadcastCommand("reconnect\n");

		for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
		{
			if ((client->active || client->spawned || client->connected) &&
				client->edict && !client->edict->free)
			{
				if (!client->edict->pvPrivateData)
				{
					Con_Printf("Skipping reconnect on %s, no pvPrivateData\n", client->name);
				}
				else if (client->spectator)
				{
					SpectatorDisconnect(client->edict);
				}
				else
				{
					ClientDisconnect(client->edict);
				}
			}
		}
	}

	Log_Open();
	Log_Printf("Spawning server \"%s\"\n", server);
	Log_PrintServerVars();
	if (svs.maxclients > 1)
		NET_Config(TRUE);
	else
		NET_Config(FALSE);

	// let's not have any servers with no name
	if (host_name.string[0] == 0)
		Cvar_Set("hostname", "Half-Life");
	scr_centertime_off = 0;

	// Any partially connected client will be restarted if the spawncount is not matched.
	// in "spawn" command
	gHostSpawnCount = ++svs.spawncount;

//
// make cvars consistant
//
	if (coop.value)
		Cvar_SetValue("deathmatch", 0);
	current_skill = (int)(skill.value + 0.5f);
	if (current_skill < 0)
		current_skill = 0;
	if (current_skill > 3)
		current_skill = 3;

	Cvar_SetValue("skill", (float)current_skill);

//
// set up the new server
//
	Host_ClearMemory(FALSE);

	// Release each client's old frame ring, re-derive the frame history depth for
	// this spawn (single vs multiplayer client count can change between spawns),
	// and allocate a fresh ring sized to it.
	for (i = 0; i < svs.maxclientslimit; i++)
		SV_ClearFrames(&svs.clients[i].frames);

	SV_UPDATE_BACKUP = (svs.maxclients == 1) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

	for (i = 0; i < svs.maxclientslimit; i++)
	{
		svs.clients[i].frames = (client_frame_t*)MnemoAllocDbg(sizeof(client_frame_t) * SV_UPDATE_BACKUP, __FILE__, __LINE__);
		memset(svs.clients[i].frames, 0, sizeof(client_frame_t) * SV_UPDATE_BACKUP);
	}

	memset(&sv, 0, sizeof(sv));

	strcpy(sv.name, server);
	if (startspot)
		strcpy(sv.startspot, startspot);
	else
		sv.startspot[0] = 0;

// load progs to get entity field count
	pr_strings = gNullString;
	gGlobalVariables.pStringBase = gNullString;

	// Force normal player collisions for single player
	if (svs.maxclients == 1)
		Cvar_SetValue("sv_clienttrace", 1);

// allocate server memory
	sv.max_edicts = COM_EntsForPlayerSlots(svs.maxclients);

	Host_DeallocateDynamicData();
	SV_ReallocateDynamicData();

	// Assume no entities beyond world and client slots
	gGlobalVariables.maxEntities = sv.max_edicts;

	edictBytes = sizeof(edict_t) * sv.max_edicts;
	edictReserveSize = Mnemo_BlockSize(g_edict_reserve);
	if (edictReserveSize < edictBytes)
	{
		MnemoFree(g_edict_reserve);
		g_edict_reserve = MnemoAlloc(edictBytes, MNEMO_FLAG_HUNK, 0, "edict reserve");
	}
	else
	{
		memset(g_edict_reserve, 0, edictReserveSize);
	}

	sv.edicts = (edict_t*)g_edict_reserve;

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;

	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;

#if HLDC_MP
	sv.master.maxsize = sizeof(sv.master_buf);
	sv.master.data = sv.master_buf;
#endif

	sv.signon.maxsize = sizeof(sv.signon_buffers[0]);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buffers[0];
	sv.num_signon_buffers = 1;

	sv.multicast.maxsize = sizeof(sv.multicast_buf);
	sv.multicast.data = sv.multicast_buf;
	sv.multicast.cursize = 0;
	sv.multicast.allowoverflow = TRUE;

// leave slots at start for clients only
	sv.num_edicts = svs.maxclients + 1;
	for (i = 0; i < svs.maxclients; i++)
	{
		ent = &sv.edicts[i + 1];
		svs.clients[i].edict = ent;
	}

	gGlobalVariables.maxClients = svs.maxclients;

	sv.state = ss_loading;
	sv.paused = FALSE;

	// Set initial time values.
	sv.time = 1.0f;
	gGlobalVariables.time = 1.0f;

	strcpy(sv.name, server);
	S_TouchSound(server);
	DC_PrecacheMap(server);
	Cvar_SetValue("sv_wateramp", 0.0f);
	sprintf(sv.modelname, "maps/%s.bsp", server);
	sv.worldmodel = Mod_ForName(sv.modelname, FALSE);
	if (!sv.worldmodel)
	{
		Con_Printf("Couldn't load world %s\n", sv.modelname);
		sv.active = FALSE;
		return FALSE;
	}

	if (svs.maxclients > 1)
	{
		char szDllName[MAX_CLIENT_DLL_PATH];

		// Server map CRC check.
		CRC32_Init(&sv.worldmapCRC);
		if (!CRC_MapFile(&sv.worldmapCRC, sv.modelname))
		{
			Con_Printf("Couldn't CRC server map:  %s\n", sv.modelname);
			sv.active = FALSE;
			return FALSE;
		}

		// DLL CRC check.
		sprintf(szDllName, "cl_dlls\\client.dll");
		CRC32_Init(&sv.clientSideDllCRC);

		if (!CRC_File(&sv.clientSideDllCRC, szDllName))
		{
			Con_Printf("Couldn't CRC client side dll:  %s\n", szDllName);
			sv.active = FALSE;
			return FALSE;
		}

		CM_CalcPAS(sv.worldmodel);
	}
	else
	{
		sv.worldmapCRC = 0;
		sv.clientSideDllCRC = 0;
	}

	sv.models[1] = sv.worldmodel;

//
// clear world interaction links
//
	SV_ClearWorld();

	sv.sound_precache[0] = pr_strings;

	sv.model_precache[0] = pr_strings;
	sv.model_precache[1] = sv.modelname;
	for (i = 1; i < sv.worldmodel->numsubmodels; i++)
	{
		sv.model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = Mod_ForName(localmodels[i], FALSE);
	}

//
// load the rest of the entities
//
	ent = sv.edicts;
	memset(&ent->v, 0, sizeof(entvars_t));
	ent->free = FALSE;
	ent->v.model = sv.worldmodel->name - pr_strings;
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (coop.value)
		gGlobalVariables.coop = coop.value;
	else
		gGlobalVariables.deathmatch = deathmatch.value;

	gGlobalVariables.mapname = sv.name - pr_strings;
	gGlobalVariables.startspot = sv.startspot - pr_strings;

// serverflags are for cross level information (sigils)
	gGlobalVariables.serverflags = svs.serverflags;

	allow_cheats = sv_cheats.value;

	SV_SetMoveVars();
	Log_Printf("Map CRC: %i\n", sv.worldmapCRC);

	return TRUE;
}

void SV_LoadEntities( void )
{
	ED_LoadFromFile(sv.worldmodel->entities);
}

// Clears all entities
void SV_ClearEntities( void )
{
	int i;

	for (i = 0; i < sv.num_edicts; i++)
	{
		edict_t* pEdict = &sv.edicts[i];
		if (pEdict->free)
			continue;

		ReleaseEntityDLLFields(pEdict);
	}
}

/*
=================
RegUserMsg

Registers a user message
=================
*/
int RegUserMsg( const char* pszName, int iSize )
{
	UserMsg* pUserMsgs;
	UserMsg* pNewMsg;
	int iFound = 0;

	if (giNextUserMsg > MAX_USERMSGS)
		return 0;
	if (!pszName)
		return 0;
	if (strlen(pszName) > 11)
		return 0;
	if (iSize > MAX_USER_MSG_DATA)
		return 0;

	pUserMsgs = sv_gpUserMsgs;
	while (pUserMsgs)
	{
		if (!strcmp(pszName, pUserMsgs->szName))
		{
			iFound = 1;
			break;
		}
		pUserMsgs = pUserMsgs->next;
	}

	if (iFound)
		return pUserMsgs->iMsg;

	pNewMsg = (UserMsg*)malloc(sizeof(UserMsg));
	pNewMsg->iSize = iSize;
	pNewMsg->iMsg = giNextUserMsg;
	giNextUserMsg++;

	strcpy(pNewMsg->szName, pszName);

	if (pNewMsg)
	{
		pNewMsg->next = sv_gpNewUserMsgs;
		sv_gpNewUserMsgs = pNewMsg;
		return pNewMsg->iMsg;
	}

	return 0;
}

/*
==================
SV_SendUserReg

==================
*/
void SV_SendUserReg( sizebuf_t* sb )
{
	UserMsg* pMsg;

	pMsg = sv_gpNewUserMsgs;
	while (pMsg)
	{
		MSG_WriteByte(sb, svc_newusermsg);
		MSG_WriteByte(sb, pMsg->iMsg);
		MSG_WriteByte(sb, pMsg->iSize);
		MSG_WriteLong(sb, *(int*)(pMsg->szName + 0));
		MSG_WriteLong(sb, *(int*)(pMsg->szName + 4));
		MSG_WriteLong(sb, *(int*)(pMsg->szName + 8));
		MSG_WriteLong(sb, (int)pMsg->next);
		pMsg = pMsg->next;
	}
}

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/


typedef struct
{
	unsigned	mask;
	unsigned	compare;
	float		banEndTime; // 0 for permanent ban
	float		banTime;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int			numipfilters;
userfilter_t userfilters[MAX_USERFILTERS];
int			numuserfilters;

cvar_t	filterban = { "filterban", "1" };

/*
=================
SV_SendBan
=================
*/
void SV_SendBan( void )
{
	char		data[16];

	sprintf(data, "banned.\n");

	SZ_Clear(&net_message);
	MSG_WriteLong(&net_message, 0xFFFFFFFF); // -1 -1 -1 -1 signal
	MSG_WriteByte(&net_message, A2C_PRINT);
	MSG_WriteString(&net_message, data);
	NET_SendPacket(NS_SERVER, net_message.cursize, net_message.data, net_from);
	SZ_Clear(&net_message);
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket( void )
{
	int		i, j;
	unsigned	in;

	in = *(unsigned*)net_from.ip;

	// Handle timeouts 
	for (i = numipfilters - 1; i >= 0; i--)
	{
		if ((ipfilters[i].compare != 0xffffffff) &&
			 (ipfilters[i].banEndTime != 0.0f) &&
			 (ipfilters[i].banEndTime <= realtime))
		{
			for (j = i + 1; j < numipfilters; j++)
			{
				ipfilters[j - 1] = ipfilters[j];
			}
			numipfilters--;
			continue;
		}

		// Only get here if ban is still in effect.
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterban.value;
	}
	return !filterban.value;
}

qboolean SV_FilterUser( int userid )
{
	int i, j;

	for (i = numuserfilters - 1; i >= 0; i--)
	{
		if (userfilters[i].banEndTime != 0.0f &&
			userfilters[i].banEndTime <= realtime)
		{
			for (j = i + 1; j < numuserfilters; j++)
				userfilters[j - 1] = userfilters[j];
			numuserfilters--;
			continue;
		}

		if (userid == userfilters[i].userid)
			return filterban.value;
	}

	return !filterban.value;
}

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter( char* s, ipfilter_t* f )
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];

	for (i = 0; i < 4; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++)
	{
		if (*s < '0' || *s > '9')
		{
			Con_Printf("Bad filter address: %s\n", s);
			return FALSE;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = *(unsigned*)m;
	f->compare = *(unsigned*)b;

	return TRUE;
}

void SV_BanId_f( void )
{
	char reason[128];
	char* idstring;
	client_t* save;
	client_t* cl;
	int id;
	int search;
	int i;
	float banTime;

	if (Cmd_Argc() < 3 || Cmd_Argc() > 5)
	{
		Con_Printf("Usage:  banid <minutes> <uniqueid or #userid > { kick }\nUse 0 minutes for permanent\n");
		return;
	}

	idstring = Cmd_Argv(2);
	if (idstring && idstring[0] == '#')
	{
		if (strlen(idstring) == 1)
		{
			if (Cmd_Argc() < 3)
			{
				Con_Printf("Insufficient arguments to banid\n");
				return;
			}
			search = Q_atoi(Cmd_Argv(3));
		}
		else
		{
			search = Q_atoi(idstring + 1);
		}

		for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
		{
			if ((cl->active || cl->connected || cl->spawned) &&
				!cl->fakeclient && cl->userid == search)
			{
				id = cl->network_userid;
				break;
			}
		}

		if (id == -1)
		{
			Con_Printf("Couldn't find userid %u, or user is a LAN user and has no unique id\n", id);
			return;
		}
	}
	else
	{
		id = Q_atoi(Cmd_Argv(2));
		if (id == -1)
		{
			Con_Printf("Can't ban LAN user by unique id, use addip instead\n");
			return;
		}
	}

	for (i = 0; i < numuserfilters; i++)
	{
		if (userfilters[i].userid == id)
			break;
	}

	if (i == numuserfilters)
	{
		if (numuserfilters >= MAX_USERFILTERS)
		{
			Con_Printf("User filter list is full\n");
			return;
		}
		numuserfilters++;
	}

	banTime = Q_atof(Cmd_Argv(1));
	if (banTime < 0.01f)
		banTime = 0.0f;

	userfilters[i].banTime = banTime;
	userfilters[i].banEndTime = banTime == 0.0f ? 0.0f : realtime + banTime * 60.0f;
	userfilters[i].userid = id;

	if (Cmd_Argc() > 3 && !Q_strcasecmp(Cmd_Argv(Cmd_Argc() - 1), "kick"))
	{
		save = host_client;
		for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
		{
			if ((cl->active || cl->connected || cl->spawned) &&
				!cl->fakeclient && cl->network_userid == id)
			{
				host_client = cl;
				if (banTime == 0.0f)
					sprintf(reason, "permanently");
				else
					sprintf(reason, "for %.2f minutes", banTime);
				SV_ClientPrintf("You have been kicked and banned %s by the server op.\n", reason);
				SV_DropClient(host_client, FALSE);
				break;
			}
		}
		host_client = save;
	}
}

void SV_RemoveId_f( void )
{
	int id;
	int i, j;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("Ussage:  removeid <uniqueid>\n");
		return;
	}

	id = Q_atoi(Cmd_Argv(1));
	for (i = 0; i < numuserfilters; i++)
	{
		if (userfilters[i].userid == id)
		{
			for (j = i + 1; j < numuserfilters; j++)
				userfilters[j - 1] = userfilters[j];
			numuserfilters--;
			Con_Printf("removeid:  filter removed for %u.\n", id);
			return;
		}
	}

	Con_Printf("removeid:  not such userid:  %u.\n", id);
}

void SV_WriteId_f( void )
{
	void* file;
	char name[MAX_OSPATH];
	int i;

	sprintf(name, "%s/banned.cfg", com_gamedir);
	Con_Printf("Writing %s.\n", name);
	file = Sys_OpenHandle(name, "wb");
	if (!file)
	{
		Con_Printf("Couldn't open %s\n", name);
		return;
	}

	for (i = 0; i < numuserfilters; i++)
	{
		if (userfilters[i].banTime == 0.0f)
			Sys_FPrintf(file, "banid 0.0f %u\n", userfilters[i].userid);
	}

	Sys_CloseHandle(file);
}

void SV_ListId_f( void )
{
	int i;

	Con_Printf("User filter list:\n");
	for (i = 0; i < numuserfilters; i++)
	{
		if (userfilters[i].banTime == 0.0f)
			Con_Printf("%u : permanent\n", userfilters[i].userid);
		else
			Con_Printf("%u : %.3f min\n", userfilters[i].userid, userfilters[i].banTime);
	}
}

/*
=================
SV_AddIP_f
=================
*/
void SV_AddIP_f( void )
{
	int		i;
	float banTime;

	if (Cmd_Argc() != 3)
	{
		Con_Printf("Usage:  addip <minutes> <ipaddress>\nUse 0 minutes for permanent\n");
		return;
	}

	for (i = 0; i < numipfilters; i++)
	{
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	}

	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Con_Printf("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	banTime = atof(Cmd_Argv(1));
	if (banTime < 0.01f)
		banTime = 0.0f;

	ipfilters[i].banTime = banTime;

	if (banTime)
	{
		banTime *= 60.0f;
		banTime += realtime; // Time when we are done banning.
	}

	// Time to unban.
	ipfilters[i].banEndTime = banTime;

	if (!StringToFilter(Cmd_Argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void SV_RemoveIP_f( void )
{
	ipfilter_t	f;
	int			i, j;

	if (!StringToFilter(Cmd_Argv(1), &f))
		return;

	for (i = 0; i < numipfilters; i++)
	{
		if ((ipfilters[i].mask == f.mask) &&
			 (ipfilters[i].compare == f.compare))
		{
			for (j = i + 1; j < numipfilters; j++)
			{
				ipfilters[j - 1] = ipfilters[j];
			}
			numipfilters--;
			Con_Printf("Filter removed.\n");
			return;
		}
	}
	Con_Printf("Didn't find %s.\n", Cmd_Argv(1));
}

/*
=================
SV_ListIP_f
=================
*/
void SV_ListIP_f( void )
{
	int		i;
	byte	b[4];

	Con_Printf("Filter list:\n");
	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		if (ipfilters[i].banTime != 0.0f)
			Con_Printf("%3i.%3i.%3i.%3i : %.3f min\n", b[0], b[1], b[2], b[3], ipfilters[i].banTime);
		else
			Con_Printf("%3i.%3i.%3i.%3i : permanent\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SV_WriteIP_f( void )
{
	FILE* f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;
	float banTime;

	sprintf(name, "%s/listip.cfg", com_gamedir);

	Con_Printf("Writing %s.\n", name);

	f = fopen(name, "wb");
	if (!f)
	{
		Con_Printf("Couldn't open %s\n", name);
		return;
	}

	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;

		// Only store out the permanent bad guys from this server.
		banTime = ipfilters[i].banTime;
		if (banTime != 0.0f)
			continue;

		fprintf(f, "addip 0.0 %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	fclose(f);
}

//============================================================================

// Print CD keys
void SV_Keys_f( void )
{
	int		i;
	char	szKey[1024];
	client_t* cl;

	if (!sv.active)
	{
		Con_Printf("Not running a server\n");
		return;
	}

	Con_Printf("CD Keys =================\n");
	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((cl->active || cl->connected || cl->spawned) && !cl->fakeclient)
		{
			sprintf(szKey, "%3i %s : %s\n", i + 1, cl->name, cl->hashedcdkey);
			Con_Printf(szKey);
		}
	}
	Con_Printf("==========================\n");
}
/*
=================
SV_PrintLogos

Report which players still owe the server their spray logo.
=================
*/
void SV_PrintLogos( void )
{
	int i;
	char szKey[1024];
	client_t* cl;
	customization_t* pCust;

	Con_Printf("Server Rep. of Player Logos ===\n");

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if ((cl->active || cl->connected || cl->spawned) && !cl->fakeclient)
		{
			for (pCust = cl->customdata.pNext; pCust; pCust = pCust->pNext)
				Con_Printf(szKey);
		}
	}

	Con_Printf("==========================\n");
}

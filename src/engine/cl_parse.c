// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "pmove.h"
#include "decal.h"
#include "r_trans.h"
#include "cl_demo.h"
#include "cl_draw.h"
#include "hashpak.h"
#include "tmessage.h"
#include "vmu.h"

// Message parsing is built unoptimized and never inlined. It runs a handful of
// times a frame at most, and when a server sends something the client does not
// expect this is the first place anyone puts a breakpoint.
#pragma optimize( "", off )
#pragma inline_depth( 0 )

int		last_data[MAX_DATA_HISTORY];
int		msg_buckets[MAX_DATA_HISTORY];

UserMsg* gClientUserMsgs = NULL;

char* svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",				// [long] server version
	"svc_setview",				// [short] entity number
	"svc_sound",				// <see code>
	"svc_time",					// [float] server time
	"svc_print",				// [string] null terminated string
	"svc_stufftext",			// [string] stuffed into client's console buffer
									// the string should be \n terminated
	"svc_setangle",				// [vec3] set the view angle to this absolute value

	"svc_serverinfo",			// [long] version
									// [string] signon string
									// [string]..[0]model cache [string]...[0]sounds cache
									// [string]..[0]item cache
	"svc_lightstyle",			// [byte] [string]
	"svc_updateuserinfo",		// [byte] [long] [string]
	"svc_updatefrags",			// [byte] [short]
	"svc_clientdata",			// <shortbits + data>
	"svc_stopsound",			// <see code>
	"svc_updatecolors",			// [byte] [byte]
	"svc_particle",				// [vec3] <variable>
	"svc_damage",				// [byte] impact [byte] blood [vec3] from

	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",

	"svc_temp_entity",			// <variable>
	"svc_setpause",				// [byte] on / off
	"svc_signonnum",			// [byte]  used for the signon sequence
	"svc_centerprint",			// [string] to put in center of the screen
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",		// [coord3] [byte] samp [byte] vol [byte] aten
	"svc_intermission",			// [string] music
	"svc_finale",				// [string] music [string] text
	"svc_cdtrack",				// [byte] track [byte] looptrack
	"svc_restore",
	"svc_cutscene",
	"svc_weaponanim",
	"svc_decalname",			// [byte] index [string] name
	"svc_roomtype",				// [byte] roomtype (dsp effect)
	"svc_addangle",				// [angle3] set the view angle to this absolute value
	"svc_newusermsg",
	"svc_download",
	"svc_packetentities",		// [...]  Non-delta compressed entities
	"svc_deltapacketentities",	// [...]  Delta compressed entities
	"svc_playerinfo",
	"svc_choke",				// # of packets held back on channel because too much data was flowing.
	"svc_resourcelist",
	"svc_newmovevars",
	"svc_nextupload",
	"svc_resourcerequest",
	"svc_customization",
	"svc_crosshairangle",		// [char] pitch * 5 [char] yaw * 5
	"svc_soundfade",			// char percent, char holdtime, char fadeouttime, char fadeintime
	"svc_clientmaxspeed",
	"svc_skippedupdate"
};

int	oldparsecountmod;
int	parsecountmod;
float	parsecounttime;
resource_t currentresource;

//=============================================================================

/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
cl_entity_t* CL_EntityNum( int num )
{
	if (num >= cl.num_entities)
	{
		if (num >= cl.max_edicts)
			Host_Error("CL_EntityNum: %i is an invalid number, cl.max_edicts is %i", num, cl.max_edicts);
		while (cl.num_entities <= num)
		{
			cl.num_entities++;
		}
	}

	return &cl_entities[num];
}

/*
=====================
AddNewUserMsg

Registers a new user message on the client
=====================
*/
void AddNewUserMsg( void )
{
	UserMsg* pList;
	UserMsg umsg;

	int	i;
	int	fFound = 0;

	umsg.iMsg = MSG_ReadByte();
	umsg.iSize = MSG_ReadByte();
	umsg.pfn = NULL;

	if (umsg.iSize == 255)
		umsg.iSize = -1;

	i = MSG_ReadLong();
	strncpy(&umsg.szName[0], (char*)&i, sizeof(i));
	i = MSG_ReadLong();
	strncpy(&umsg.szName[4], (char*)&i, sizeof(i));
	i = MSG_ReadLong();
	strncpy(&umsg.szName[8], (char*)&i, sizeof(i));
	i = MSG_ReadLong();
	strncpy((char*)&umsg.next, (char*)&i, sizeof(i));

	// Scan all user messages
	for (pList = gClientUserMsgs; pList; pList = pList->next)
	{
		if (!_stricmp(pList->szName, umsg.szName))
		{
			fFound = 1;
			pList->iMsg = umsg.iMsg;
			pList->iSize = umsg.iSize;
		}
	}

	if (!fFound)
	{
		UserMsg* pumsg;

		pumsg = (UserMsg*)Z_Malloc(sizeof(UserMsg));
		memcpy(pumsg, &umsg, sizeof(UserMsg));
		pumsg->next = gClientUserMsgs;
		gClientUserMsgs = pumsg;
	}
}

/*
=================
DispatchUserMsg
=================
*/
void DispatchUserMsg( int iMsg )
{
	static char buf[MAX_USER_MSG_DATA];
	int MsgSize = 0;
	int fFound = 0;

	UserMsg* pList;

	if (iMsg <= svc_lastmsg || iMsg >= MAX_USERMSGS)
	{
		Con_DPrintf("Illegal User Msg %d\n", iMsg);
		return;
	}

	for (pList = gClientUserMsgs; pList; pList = pList->next)
	{
		if (pList->iMsg == iMsg)
		{
			MsgSize = pList->iSize;
			if (!fFound)
			{
				if (MsgSize == -1)
					MsgSize = MSG_ReadByte();

				MSG_ReadBuf(MsgSize, buf);
			}

			fFound = 1;

			if (pList->pfn)
				pList->pfn(pList->szName, MsgSize, buf);
			else
				Con_DPrintf("UserMsg: No pfn %s %d\n", pList->szName, iMsg);
		}
	}

	if (!fFound)
	{
		Con_DPrintf("UserMsg: Not Present on Client %d\n", iMsg);
	}
}

/*
===============
DispatchDirectUserMsg

Returns true on success
===============
*/
int DispatchDirectUserMsg( const char* pszName, int iSize, void* pBuf )
{
	int	fFound;
	UserMsg* pList;
	pfnUserMsgHook pfnRet;
	int	iMsgSize;

	fFound = 0;

	for (pList = gClientUserMsgs; pList; pList = pList->next)
	{
		if (!_stricmp(pszName, pList->szName))
		{
			fFound = 1;

			iMsgSize = pList->iSize;
			if (iMsgSize == -1)
				iMsgSize = iSize;

			pfnRet = pList->pfn;
			if (!pfnRet)
			{
				Con_DPrintf("UserMsg: No pfn %s %d\n", pList->szName, pList->iMsg);
				continue;
			}

			pfnRet(pList->szName, iMsgSize, pBuf);
		}
	}

	return fFound;
}

pfnUserMsgHook CL_HookUserMsg( char* pszName, pfnUserMsgHook pfn )
{
	UserMsg* pList, * pLastMatch;
	pfnUserMsgHook pfnRet;

	pLastMatch = NULL;

	for (pList = gClientUserMsgs; pList; pList = pList->next)
	{
		if (!_stricmp(pszName, pList->szName))
		{
			pfnRet = pList->pfn;
			if (pfnRet == pfn)
				return pfnRet;

			pLastMatch = pList;
		}
	}

	pList = (UserMsg*)malloc(sizeof(UserMsg));
	memset(pList, 0, sizeof(UserMsg));

	if (pLastMatch)
	{
		memcpy(pList, pLastMatch, sizeof(*pList));
	}
	else
	{
		strcpy(pList->szName, pszName);
	}

	pList->pfn = pfn;
	pList->next = gClientUserMsgs;
	gClientUserMsgs = pList;

	return NULL;
}

void CL_ClearUserMessages( void )
{
	UserMsg* pMsg;
	UserMsg* pNext;

	if (!gClientUserMsgs)
		return;

	pMsg = gClientUserMsgs;
	while (pMsg)
	{
		pNext = pMsg->next;
		free(pMsg);
		pMsg = pNext;
	}

	gClientUserMsgs = NULL;
}

void CL_UserMsgs_f( void )
{
	UserMsg* pMsg;

	pMsg = gClientUserMsgs;
	while (pMsg)
	{
		Con_Printf("%i:%s Sz %i\n", pMsg->iMsg, pMsg->szName, pMsg->iSize);
		pMsg = pMsg->next;
	}
}

///////////////////////////////////////////////////
//
// SOUND
//
//

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket( void )
{
	vec3_t	pos;
	int		channel, ent;
	int		sound_num;
	float	volume;
	int		field_mask;
	float 	attenuation;
	int		pitch;
	int		i;
	sfx_t* sfx;
	sfx_t sfxsentence;

	// Sentences use a temporary sound record built from the sentence name.
	memset(&sfxsentence, 0, sizeof(sfxsentence));

	MSG_StartBitReading(&net_message);

	field_mask = MSG_ReadBitField16(9);

	if (field_mask & SND_VOLUME)
		volume = MSG_ReadBitField8(8) * (1.0f / 255.0f);	// reduce back to 0.0 - 1.0 range
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME * (1.0f / 255.0f);

	if (field_mask & SND_ATTENUATION)
		attenuation = MSG_ReadBitField8(8) * (1.0f / 64.0f);
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	channel = MSG_ReadBitField8(3);
	ent = MSG_ReadBitField16(10);

	if (field_mask & SND_LARGE_INDEX)
		sound_num = MSG_ReadBitField16(16);
	else
		sound_num = MSG_ReadBitField8(8);

	for (i = 0; i < 3; i++)
		pos[i] = MSG_ReadSignMagnitude16(16) / 8.0f;

	if (field_mask & SND_PITCH)
		pitch = MSG_ReadBitField8(8);
	else
		pitch = DEFAULT_SOUND_PACKET_PITCH;

	MSG_EndBitReading(&net_message);

	if (ent < cl.max_edicts)
	{
		if (field_mask & SND_SENTENCE)
		{
			sfx = &sfxsentence;
			strcpy(sfx->name, "!");
			strcat(sfx->name, rgpszrawsentence[sound_num]);
		}
		else
		{
#if HLDC_MP
			if (cls.netchan.remote_address.type == NA_IP)
			{
				if (sound_num < 0 || sound_num >= MAX_SOUNDS)
					Host_Error("CL_ParseStartSoundPacket: sound = %i", sound_num);
				sfx = cl.sound_precache[sound_num];
				if (!sfx)
					return;
			}
			else
#endif
				sfx = S_GetSfxByIndex(sound_num);
		}

		if (channel == CHAN_STATIC)
		{
			S_StartStaticSound(ent, CHAN_STATIC, sfx, pos, volume, attenuation, field_mask, pitch);
		}
		else
		{
			S_StartDynamicSound(ent, channel, sfx, pos, volume, attenuation, field_mask, pitch);
		}
	}
	else
	{
		Host_Error("CL_ParseStartSoundPacket: ent = %i", ent);
	}
}

/*
==================
CL_CheckOrDownloadFile

Checks if the file exists or if we can download it
==================
*/
qboolean CL_CheckOrDownloadFile( char* filename )
{
	if (!strstr(filename, "..") && !strstr(filename, "server.cfg"))
	{
		Con_Printf("Download refused, cl_allow_download is 0\n");
		return TRUE;
	}

	Con_Printf("Refusing to download a path with '..'\n");
	return TRUE;
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload( void )
{
	Sys_Error("Got to CL_ParseDownload");
}

/*
==================
CL_PrintResource

==================
*/
void CL_PrintResource( int index, resource_t* pResource )
{
	static char type[16];
	static char fatal[4];

	if (pResource->ucFlags & RES_FATALIFMISSING)
		sprintf(fatal, "Y");
	else
		sprintf(fatal, "N");

	switch (pResource->type)
	{
	case t_sound:
		sprintf(type, "sound");
		break;
	case t_skin:
		sprintf(type, "skin");
		break;
	case t_model:
		sprintf(type, "model");
		break;
	case t_decal:
		sprintf(type, "decal");
		break;
	case t_generic:
		sprintf(type, "generic");
		break;
	default:
		sprintf(type, "unknown");
		break;
	}

	Con_Printf("%3i %s:%15s %i %s\n", index, type, pResource->szFileName, pResource->nIndex, fatal);
}

/*
===============
CL_PrintResourceLists_f

===============
*/
void CL_PrintResourceLists_f( void )
{
	int i;
	resource_t* pResource;

	i = 1;

	Con_Printf("- Needed -------------------------------------------\n");
	Con_Printf("#   Name                  Size Type Index Fatal\n");

	for (pResource = cl.resourcesneeded.pNext; pResource != &cl.resourcesneeded; pResource = pResource->pNext)
	{
		CL_PrintResource(i, pResource);
		i++;
	}

	Con_Printf("- On hand ------------------------------------------\n");
	Con_Printf("#   Name                  Size Type Index Fatal\n");

	for (pResource = cl.resourcesonhand.pNext; pResource != &cl.resourcesonhand; pResource = pResource->pNext)
	{
		CL_PrintResource(i, pResource);
		i++;
	}

	Con_Printf("--------------------------------------------\n\n");
}

/*
===============
CL_ClearResourceLists

===============
*/
void CL_ClearResourceLists( void )
{
	CL_ClearResourceList(&cl.resourcesneeded);
	CL_ClearResourceList(&cl.resourcesonhand);
}

qboolean GetGameInfo( char* mapName )
{
	CRC32_t	mapCRC;
	CRC32_t	clientdllCRC;
	FILE*	file;
	char	szDllName[MAX_QPATH];

	if (sv.active)
		return TRUE;

	CRC32_Init(&mapCRC);
	if (!CRC_MapFile(&mapCRC, mapName))
	{
		file = NULL;
		if (COM_FOpenFile(mapName, &file) != -1)
		{
			if (file)
				Sys_CloseHandle(file);
			COM_ExplainDisconnection(TRUE, "Couldn't CRC map %s, disconnecting\n", mapName);
			Host_Error("Disconnected");
			return FALSE;
		}

		if (!cl_allowdownload.value)
		{
			COM_ExplainDisconnection(TRUE,
				"Refusing to download map %s, (cl_allowdownload is 0 ) disconnecting\n", mapName);
			Host_Error("Disconnected");
			return FALSE;
		}

		Con_Printf("Couldn't find map %s, server will download the map\n", mapName);
		mapCRC = cl.serverCRC;
	}

	sprintf(szDllName, "cl_dlls\\client.dll");
	CRC32_Init(&clientdllCRC);
	if (!CRC_File(&clientdllCRC, szDllName))
	{
		COM_ExplainDisconnection(TRUE, "Couldn't CRC client side dll %s.\n", szDllName);
		Host_Error("Disconnected");
		return FALSE;
	}

	if (cl.clientdllCRC != clientdllCRC)
		Con_Printf("Mismatched client.dll, proceeding...\n");

	return TRUE;
}

/*
==================
CL_RegisterResources

Clean up and move to next part of sequence.
==================
*/
void CL_RegisterResources( void )
{
	float	time1, time2, time3, time4;

	if (cls.custom)
	{
		cls.custom = FALSE;
		return;
	}

	cl.worldmodel = cl.model_precache[1];

	cl_entities->model = cl.worldmodel;

	if (!cl.worldmodel)
		Sys_Error("Client world model is NULL\n");

	time1 = Sys_FloatTime();
	R_NewMap();
	time2 = Sys_FloatTime();
	Hunk_Check();
	time3 = Sys_FloatTime();

	noclip_anglehack = FALSE;
	if (!GetGameInfo(cl.worldmodel->name))
		return;

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, va("prespawn %i 0", cl.servercount));
	time4 = Sys_FloatTime();
}

void CL_MoveToOnHandList( resource_t* pResource )
{
	char* name;

	if (!pResource)
	{
		return;
	}

	name = pResource->szFileName;
	if (*name == '/' || *name == '\\')
	{
		do
		{
			*name = name[1];
			name++;
		} while (*name);
	}
	while (*name)
	{
		*name = tolower(*name);
		name++;
	}

	switch (pResource->type)
	{
	case t_sound:
		if (pResource->ucFlags & RES_WASMISSING)
		{
			cl.sound_precache[pResource->nIndex] = NULL;
		}
		else
		{
			S_EndPrecaching();
			cl.sound_precache[pResource->nIndex] = S_FindName(pResource->szFileName);
			S_BeginPrecaching();
			if (!cl.sound_precache[pResource->nIndex] && (pResource->ucFlags & RES_FATALIFMISSING))
			{
				COM_ExplainDisconnection(TRUE, "Cannot continue without sound %s, disconnecting\n", pResource->szFileName);
				CL_Disconnect();
				return;
			}
		}
		break;
	case t_skin:
		break;
	case t_model:
		cl.model_precache[pResource->nIndex] = Mod_ForNameDefer(pResource->szFileName, FALSE);
		if (!cl.model_precache[pResource->nIndex])
		{
			Con_Printf("Model %s not found\n", pResource->szFileName);
			if (pResource->ucFlags & RES_FATALIFMISSING)
			{
				COM_ExplainDisconnection(TRUE, "Cannot continue without model %s, disconnecting\n", pResource->szFileName);
				CL_Disconnect();
				return;
			}
		}
		break;
	case t_decal:
		Draw_DecalSetName(pResource->nIndex, pResource->szFileName);
		break;
	}

	CL_RemoveFromResourceList(pResource);
	CL_AddToResourceList(pResource, &cl.resourcesonhand);
}

/*
===============
COM_SizeofResourceList

===============
*/
int COM_SizeofResourceList( resource_t* pList, int* nWorldSize, int* nModelsSize, int* nDecalsSize, int* nSoundsSize, int* nSkinsSize, int* nGenericSize )
{
	resource_t* p;
	int nSize;

	nSize = 0;

	*nModelsSize = 0;
	*nWorldSize = 0;
	*nDecalsSize = 0;
	*nSoundsSize = 0;
	*nSkinsSize = 0;
	*nGenericSize = 0;

	for (p = pList->pNext; p != pList; p = p->pNext)
	{
		nSize += 1000;

		switch (p->type)
		{
		case t_sound:
			*nSoundsSize += 1000;
			break;
		case t_skin:
			*nSkinsSize += 1000;
			break;
		case t_model:
			if (p->nIndex == 1) // worldmodel always take 1 slot
			{
				*nWorldSize = 1000;
			}
			else
			{
				*nModelsSize += 1000;
			}
			break;
		case t_decal:
			*nDecalsSize += 1000;
			break;
		case t_generic:
			*nGenericSize += 1000;
			break;
		}
	}

	return nSize;
}

void CL_AddToResourceList( resource_t* pResource, resource_t* pList )
{
	if (pResource->pPrev || pResource->pNext)
	{
		Con_Printf("Resource already linked\n");
		return;
	}

	if (!pList->pPrev || !pList->pNext)
	{
		Sys_Error("Resource list corrupted.\n");
	}

	pResource->pPrev = pList->pPrev;
	pList->pPrev->pNext = pResource;
	pList->pPrev = pResource;
	pResource->pNext = pList;
}

/*
===============
CL_ClearResourceList
===============
*/
void CL_ClearResourceList( resource_t* pList )
{
	resource_t* p, * n = NULL;

	for (p = pList->pNext; p && p != pList; p = n)
	{
		n = p->pNext;

		CL_RemoveFromResourceList(p);
		free(p);
	}

	pList->pPrev = pList;
	pList->pNext = pList;
}

void CL_RemoveFromResourceList( resource_t* pResource )
{
	if (!pResource->pPrev || !pResource->pNext)
	{
		Sys_Error("Mislinked resource in CL_RemoveFromResourceList");
	}

	if (pResource->pNext == pResource || pResource->pPrev == pResource)
	{
		Sys_Error("Attempt to free last entry in list.");
	}

	pResource->pPrev->pNext = pResource->pNext;
	pResource->pNext->pPrev = pResource->pPrev;
	pResource->pPrev = NULL;
	pResource->pNext = NULL;
}

/*
===============
CL_EstimateNeededResources

Returns the size of needed resources to download
===============
*/
int CL_EstimateNeededResources( void )
{
	resource_t* p;

	// Everything the game needs already ships on the disc, so nothing is ever
	// downloaded. Walk the list but report no download size.
	for (p = cl.resourcesneeded.pNext; p != &cl.resourcesneeded; p = p->pNext)
	{
	}

	return 0;
}

/*
================
CL_RequestMissingResources

This is used to perform repeated checks on the local player to see
if it has loaded all the required resources
================
*/
qboolean CL_RequestMissingResources( void )
{
	resource_t* p;

	if (cls.download || cls.downloadinprogress)
		return FALSE;

	if (!cls.custom && cls.state != ca_uninitialized)
		return FALSE;

	if (cls.doneregistering)
		return FALSE;

	p = cl.resourcesneeded.pNext;
	cls.downloadresource = p;
	currentresource = *p;

	if (p == &cl.resourcesneeded)
	{
		cls.downloadresource = NULL;
		Sys_SetTaskName("Resource propagation complete");
		CL_RegisterResources();
		cls.doneregistering = TRUE;
		Sys_SetTaskName("Resources registered");
		return FALSE;
	}

	CL_MoveToOnHandList(p);
	return TRUE;
}

/*
===============
CL_StartResourceDownloading

Begin resource downloading, set incoming transfer data
===============
*/
void CL_StartResourceDownloading( char* pszMessage, qboolean bCustom )
{
	int		worldSize, modelsSize, decalsSize, soundsSize, skinsSize, genericSize;

	cls.nTotalSize = COM_SizeofResourceList(&cl.resourcesneeded, &worldSize, &modelsSize, &decalsSize, &soundsSize, &skinsSize, &genericSize);
	cls.nTotalToTransfer = CL_EstimateNeededResources();

	if (worldSize > 0)
	{
	}
	if (modelsSize > 0)
	{
	}
	if (soundsSize > 0)
	{
	}
	if (decalsSize > 0)
	{
	}
	if (skinsSize > 0)
	{
	}
	if (genericSize > 0)
	{
	}

	if (!bCustom)
	{
		cls.state = ca_uninitialized;
		cls.custom = FALSE;
	}
	else
	{
		cls.custom = TRUE;
	}

	cls.doneregistering = FALSE;
	cls.downloadinprogress = FALSE;

	cls.fLastStatusUpdate = realtime;
	cls.fLastDownloadTime = realtime;

	cls.nRemainingToTransfer = cls.nTotalToTransfer;

	memset(cls.rgDownloads, 0, sizeof(cls.rgDownloads));
	cls.downloadnumber = 0;
}

int CL_CountResourceList( resource_t* pList )
{
	resource_t* p;
	int count;

	count = 0;
	for (p = pList->pNext; p != pList; p = p->pNext)
		count++;

	return count;
}

/*
===============
CL_ParseResourceList

Parse the list of resources received from the server
===============
*/
void CL_ParseResourceList( void )
{
	int		i, total;
	int		totalsize;
	resource_t* resource;

	totalsize = MSG_ReadShort();
	i = MSG_ReadShort();
	total = MSG_ReadShort();

	for (; i < total; i++)
	{
		resource = (resource_t*)MnemoAllocDbg(sizeof(resource_t), __FILE__, __LINE__);
		memset(resource, 0, sizeof(resource_t));

		resource->type = MSG_ReadByte();
		strcpy(resource->szFileName, MSG_ReadString());
		resource->nIndex = MSG_ReadShort();
		MSG_ReadLong();
		resource->ucFlags = MSG_ReadByte();
		resource->pNext = resource->pPrev = NULL;
		resource->ucFlags &= ~RES_WASMISSING;

		// Add new entry in the linked list
		CL_AddToResourceList(resource, &cl.resourcesneeded);
	}

	if (CL_CountResourceList(&cl.resourcesneeded) < totalsize)
	{
		cls.state = ca_connected;
	}
	else
	{
		CL_StartResourceDownloading("Verifying and downloading resources...\n", FALSE);
	}
}

/*
================
CL_PlayerHasCustomization

Sees if the specified customization type exists for the nPlayerNum
================
*/
customization_t* CL_PlayerHasCustomization( int nPlayerNum, resourcetype_t type )
{
	customization_t* pList;

	pList = cl.players[nPlayerNum].customdata.pNext;
	while (pList)
	{
		if (pList->resource.type == type)
			return pList;

		pList = pList->pNext;
	}

	return NULL;
}

/*
================
CL_RemoveCustomization

Removes the specified customization for the nPlayerNum
================
*/
void CL_RemoveCustomization( int nPlayerNum, customization_t* pRemove )
{
	customization_t* pList;
	int	i;
	customization_t* pNext;

	pList = cl.players[nPlayerNum].customdata.pNext;
	while (pList)
	{
		pNext = pList->pNext;

		if (pRemove == pList)
		{
			if (pList->bInUse && pList->pBuffer)
				free(pList->pBuffer);

			if (pList->bInUse && pList->pInfo)
			{
				if (pRemove->resource.type == t_decal)
				{
					cachewad_t* pWad;

					if (cls.state == ca_active)
						R_DecalRemoveAll(-1 - nPlayerNum);

					pWad = (cachewad_t*)pRemove->pInfo;
					free(pWad->lumps);

					for (i = 0; i < pWad->cacheCount; i++)
					{
#if defined ( GLQUAKE )
						cacheentry_t* pic = &pWad->cache[i];
#else
						cachepic_t* pic = &pWad->cache[i];
#endif
						if (Cache_Check(&pic->cache))
							Cache_Free(&pic->cache, 0);
					}

					free(pWad->cache);
				}

				free(pRemove->pInfo);
			}

			free(pRemove);
			cl.players[nPlayerNum].customdata.pNext = pNext;
		}

		pList = pNext;
	}
}

/*
================
CL_DeallocateDynamicData

================
*/
void CL_DeallocateDynamicData( void )
{
	if (cl_entities)
	{
		free(cl_entities);
		cl_entities = NULL;
	}

	R_DestroyObjects();
}

/*
================
CL_ReallocateDynamicData

================
*/
void CL_ReallocateDynamicData( int nMaxClients )
{
	cl.max_edicts = COM_EntsForPlayerSlots(nMaxClients);
	if (cl.max_edicts <= 0)
		Sys_Error("CL_ReallocateDynamicData allocating 0 entities");

	if (cl_entities)
		Con_Printf("CL_Reallocate cl_entities\n");

	cl_entities = (cl_entity_t*)MnemoAlloc(sizeof(cl_entity_t) * cl.max_edicts, MNEMO_FLAG_MALLOC, 0, "cl_entities");
	memset(cl_entities, 0, (sizeof(cl_entity_t) * cl.max_edicts));

	R_AllocObjects(cl.max_edicts);

	if (nMaxClients == 1)
		cl_update_backup = SINGLEPLAYER_BACKUP;
	else
		cl_update_backup = MULTIPLAYER_BACKUP;
	cl_update_mask = cl_update_backup - 1;

	// the frame history just changed size, so the two cursors into it have to
	// come back inside the new one
	parsecountmod &= cl_update_mask;
	oldparsecountmod &= cl_update_mask;

	if (cl.frames)
		free(cl.frames);

	cl.frames = (frame_t*)MnemoAllocDbg(sizeof(frame_t) * cl_update_backup, __FILE__, __LINE__);
	if (!cl.frames)
		Sys_Error("CL_ReallocateDynamicData failed to allocate %i frames", cl_update_backup);
	memset(cl.frames, 0, sizeof(frame_t) * cl_update_backup);
}

void CL_ParseChangeGame( char* gameDir )
{
	char gamedir[MAX_OSPATH];

	if (!gameDir || !gameDir[0])
	{
		Con_Printf("Server didn't specify a gamedir\n");
		return;
	}

	COM_FileBase(com_gamedir, gamedir);
	if (Q_stricmp(gamedir, gameDir))
	{
		Host_WriteConfiguration();
		COM_ChangeGameDir(gameDir);
		Decal_Init();
		Draw_Init();
		TextMessageInit();
		ClientDLL_Init();
		ClientDLL_HudInit();
		ClientDLL_HudVidInit();
		Cbuf_AddText("exec config.cfg\n");
		Cbuf_AddText("exec preset_a.cfg\n");
		if (FileExists("/CD-ROM/valve/halflife.cfg"))
			Cbuf_AddText("exec halflife.cfg\n");
		Con_Printf("Changed to game %s\n", gameDir);
	}
	else
	{
		TextMessageInit();
	}
}

/*
=================
CL_Parse_ServerInfo

Read in server info packet.
=================
*/
void CL_ParseServerInfo( void )
{
	char* str;
	int		i;

	Sys_SetTaskName("CL_ParseServerInfo");
//
// wipe the client_state_t struct
//
	CL_ClearState(FALSE);

	// Re-init hud video, especially if we changed game directories
	ClientDLL_HudVidInit();

	// parse protocol version number
	i = MSG_ReadLong();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf("Server returned version %i, not %i\n", i, PROTOCOL_VERSION);
		return;
	}

	// Parse servercount (i.e., # of servers spawned since server .exe started)
	// So that we can detect new server startup during download, etc.
	cl.servercount = MSG_ReadLong();

	// The CRC of the server map must match the CRC of the client map. or else
	//  the client is probably cheating.
	cl.serverCRC = MSG_ReadLong();
	cl.clientdllCRC = MSG_ReadLong();

	cl.maxclients = MSG_ReadByte();

	if (cl.maxclients < 1 || cl.maxclients > MAX_CLIENTS)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}

	if (cl.maxclients > 1 && mp_decals.value < r_decals.value)
		Cvar_SetValue("r_decals", mp_decals.value);

	CL_DeallocateDynamicData();
	CL_ReallocateDynamicData(cl.maxclients);

	cl.playernum = MSG_ReadByte();
	if (cl.playernum & PN_SPECTATOR)
	{
		cl.spectator = TRUE;
		cl.playernum &= ~PN_SPECTATOR;
	}

	// parse gametype
	cl.gametype = MSG_ReadByte();

	CL_ParseChangeGame(MSG_ReadString());

	cls.changelevel = FALSE;
	str = MSG_ReadString();
	if (str && str[0])
	{
		cls.changelevel = TRUE;
		if (!GetGameInfo(str))
			return;
	}

	str = MSG_ReadString();
	strncpy(cl.levelname, str, sizeof(cl.levelname) - 1);

	Sys_SetTaskName("Request resourcelist");
	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, va("resourcelist %i 0", cl.servercount));

	// During a level transition the client remained active which could cause problems.
	// knock it back down to 'connected'
	cls.state = ca_connected;

	gHostSpawnCount = cl.servercount;
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline( cl_entity_t* ent )
{
	int			i;

	ent->baseline.entityType = MSG_ReadByte();
	ent->baseline.modelindex = MSG_ReadShort();
	ent->baseline.sequence = MSG_ReadByte();
	ent->baseline.frame = MSG_ReadByte();

	if (ent->baseline.entityType == ENTITY_NORMAL)
		ent->baseline.scale = MSG_ReadWord() * (1.0f / 256.0f);
	else
		ent->baseline.scale = MSG_ReadByte() * (1.0f / 10.0f);

#if HLDC_MP
	if (cls.netchan.remote_address.type == NA_IP && PROTOCOL_VERSION == PROTOCOL_VERSION_CURRENT)
		ent->baseline.colormap = MSG_ReadWord();
	else
#endif
		ent->baseline.colormap = MSG_ReadByte();
	ent->baseline.skin = MSG_ReadShort();
	ent->baseline.solid = MSG_ReadByte();

	for (i = 0; i < 3; i++)
	{
		ent->baseline.origin[i] = MSG_ReadCoord();
		ent->baseline.angles[i] = MSG_ReadFloat();
		ent->baseline.mins[i] = MSG_ReadCoord();
		ent->baseline.maxs[i] = MSG_ReadCoord();
	}

	ent->baseline.rendermode = MSG_ReadByte();
	ent->baseline.renderamt = MSG_ReadByte();
	ent->baseline.rendercolor.r = MSG_ReadByte();
	ent->baseline.rendercolor.g = MSG_ReadByte();
	ent->baseline.rendercolor.b = MSG_ReadByte();
	ent->baseline.renderfx = MSG_ReadByte();
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata( int bits )
{
	int				i;
	float		latency;
	frame_t* frame;

// calculate simulated time of message
	oldparsecountmod = parsecountmod;

	i = cls.netchan.incoming_acknowledged;
	cl.parsecount = i;
	i &= cl_update_mask;
	parsecountmod = i;
	frame = &cl.frames[i];
	parsecounttime = cl.frames[i].senttime;

	frame->receivedtime = realtime;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0f)
	{
//		Con_Printf("Odd latency: %5.2f\n", latency);
	}
	else
	{
		// drift the average latency towards the observed latency

		if (latency < cls.latency)
			cls.latency = latency;
		else
			cls.latency += 0.001f;	// drift up, so correction are needed
	}

	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = MSG_ReadChar();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar();
	else
		cl.idealpitch = 0;

	VectorCopy(cl.mvelocity[0], cl.mvelocity[1]);
	for (i = 0; i < 3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
			cl.punchangle[i] = MSG_ReadHiresAngle();
		else
			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1 << i))
			cl.mvelocity[0][i] = MSG_ReadChar() * 16;
		else
			cl.mvelocity[0][i] = 0;
	}

	if (bits & SU_WEAPONS)
		cl.weapons = MSG_ReadLong();

	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (cl.inwater)
	{
		if (bits & SU_FULLYINWATER)
			cl.waterlevel = 3;
		else
			cl.waterlevel = 2;
	}
	else
	{
		cl.waterlevel = 0;
	}

	if (bits & SU_ITEMS)
		cl.stats[STAT_WEAPON] = MSG_ReadShort();
	else
		cl.stats[STAT_WEAPON] = 0;

	i = MSG_ReadShort();
	if (i != cl.stats[STAT_HEALTH])
	{
		cl.stats[STAT_HEALTH] = i;
	}
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic( void )
{
	cl_entity_t* ent;
	int		i;

	i = cl.num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		Host_Error("Too many static entities");
	ent = &cl_static_entities[i];
	cl.num_statics++;
	CL_ParseBaseline(ent);

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->skin = ent->baseline.skin;
	ent->effects = ent->baseline.effects;
	ent->scale = ent->baseline.scale;
	ent->rendermode = ent->baseline.rendermode;
	ent->renderamt = ent->baseline.renderamt;
	ent->rendercolor.r = ent->baseline.rendercolor.r;
	ent->rendercolor.g = ent->baseline.rendercolor.g;
	ent->rendercolor.b = ent->baseline.rendercolor.b;
	ent->renderfx = ent->baseline.renderfx;

	VectorCopy(ent->baseline.origin, ent->origin);
	VectorCopy(ent->baseline.angles, ent->angles);
	R_AddEfrags(ent);
}

/*
===================
CL_ParseStaticSound

===================
*/
void CL_ParseStaticSound( void )
{
	vec3_t		org;
	int			sound_num;
	float		vol, atten;
	int			i;
	int			ent;
	int			pitch;
	int			flags;
	sfx_t* sfx;
	sfx_t sfxsentence;

	// a sentence is played from a name built on the stack, so it has to start
	// out as a blank record -- everything downstream keys off the buffer
	memset(&sfxsentence, 0, sizeof(sfxsentence));

	for (i = 0; i < 3; i++)
		org[i] = MSG_ReadCoord();
	sound_num = MSG_ReadShort();
	vol = MSG_ReadByte() / 255.0f;		// reduce back to 0.0 - 1.0 range
	atten = MSG_ReadByte() / 64.0f;
	ent = MSG_ReadShort();
	pitch = MSG_ReadByte();
	flags = MSG_ReadByte();
	if (flags & SND_SENTENCE)
	{
		// make dummy sfx for sentences
		sfx = &sfxsentence;
		strcpy(sfx->name, "!");
		strcat(sfx->name, rgpszrawsentence[sound_num]);
	}
	else
	{
		sfx = cl.sound_precache[sound_num];
	}

	S_StartStaticSound(ent, CHAN_STATIC, sfx, org, vol, atten, flags, pitch);
}

/*
===================
CL_ParseMovevars
===================
*/
void CL_ParseMovevars( void )
{
	movevars.gravity			= MSG_ReadFloat();
	movevars.stopspeed			= MSG_ReadFloat();
	movevars.maxspeed			= MSG_ReadFloat();
	movevars.spectatormaxspeed	= MSG_ReadFloat();
	movevars.accelerate			= MSG_ReadFloat();
	movevars.airaccelerate		= MSG_ReadFloat();
	movevars.wateraccelerate	= MSG_ReadFloat();
	movevars.friction			= MSG_ReadFloat();
	movevars.edgefriction		= MSG_ReadFloat();
	movevars.waterfriction		= MSG_ReadFloat();
	movevars.entgravity			= MSG_ReadFloat();
	movevars.bounce				= MSG_ReadFloat();
	movevars.stepsize			= MSG_ReadFloat();
	movevars.maxvelocity		= MSG_ReadFloat();
	movevars.zmax				= MSG_ReadFloat();
	movevars.waveHeight			= MSG_ReadFloat();

	strcpy(movevars.skyName, MSG_ReadString());

	if (strcmp(movevars.skyName, cl_skyname.string))
		Cvar_Set("cl_skyname", movevars.skyName);

#if defined( GLQUAKE )
	if (movevars.zmax != gl_zmax.value)
		Cvar_SetValue("gl_zmax", movevars.zmax);
	if (gl_wateramp.value != movevars.waveHeight)
		Cvar_SetValue("gl_wateramp", movevars.waveHeight);

	cl_entities->scale = gl_wateramp.value;
#endif
}

/*
===============
CL_ParseSoundFade

===============
*/
void CL_ParseSoundFade( void )
{
	int percent;
	int inTime, holdTime, outTime;

	percent = MSG_ReadByte();
	holdTime = MSG_ReadByte();
	inTime = MSG_ReadByte();
	outTime = MSG_ReadByte();

	cls.soundfade.soundFadeStartTime = realtime;
	cls.soundfade.nStartPercent = percent;
	cls.soundfade.soundFadeHoldTime = holdTime;
	cls.soundfade.soundFadeInTime = inTime;
	cls.soundfade.soundFadeOutTime = outTime;
}

/*
===============
CL_ParseRestoreDecals

Restores a saved game.
===============
*/
void CL_ParseRestoreDecals( char* fileName )
{
	DECALLIST decalList;
	int i, decalCount, tag, temp, mapCount;
	void* pFile;
	char name[16];

	pFile = Sys_OpenHandle(fileName, "rb");
	if (pFile)
	{
		DC_SetFileBuffering(pFile, NULL, _IOFBF, 0x1000);
		DC_fread(&tag, sizeof(int), 1, pFile);
		DC_fread(&temp, sizeof(int), 1, pFile);

		if (tag == SAVEFILE_HEADER)
		{
			DC_fread(&decalCount, sizeof(int), 1, pFile);

			for (i = 0; i < decalCount; i++)
			{
				DC_fread(name, sizeof(char), 16, pFile);
				DC_fread(&decalList.entityIndex, sizeof(short), 1, pFile);
				DC_fread(&decalList.depth, sizeof(byte), 1, pFile);
				DC_fread(&decalList.flags, sizeof(byte), 1, pFile);
				DC_fread(decalList.position, sizeof(vec3_t), 1, pFile);

				if (r_decals.value)
				{
					temp = Draw_DecalIndexFromName(name);

					// Spawn decals
					R_DecalShoot(Draw_DecalIndex(temp), decalList.entityIndex, 0, decalList.position, decalList.flags);
				}
			}
		}

		Sys_CloseHandle(pFile);
	}

	mapCount = MSG_ReadByte();

	for (i = 0; i < mapCount; i++)
	{
		MSG_ReadString();
	}
}

void CL_ParseCustomization( void )
{
	Sys_Error("Customization\n");
}

void CL_PlayerDropped( int nPlayerNumber )
{
	COM_ClearCustomizationList(&cl.players[nPlayerNumber].customdata, TRUE);
}

void CL_ParseUpdateUserInfo( void )
{
	player_info_t* player;
	int slot;

	slot = MSG_ReadByte();
	if (slot >= MAX_CLIENTS)
		Host_EndGame("CL_ParseServerMessage: svc_updateuserinfo > MAX_CLIENTS");

	player = &cl.players[slot];
	player->userid = MSG_ReadLong();
	strncpy(player->userinfo, MSG_ReadString(), sizeof(player->userinfo) - 1);
	strncpy(player->name, Info_ValueForKey(player->userinfo, "name"), sizeof(player->name) - 1);
	strncpy(player->model, Info_ValueForKey(player->userinfo, "model"), sizeof(player->model) - 1);
	player->color = atoi(Info_ValueForKey(player->userinfo, "topcolor"));
	player->bottomcolor = atoi(Info_ValueForKey(player->userinfo, "bottomcolor"));
	if (*Info_ValueForKey(player->userinfo, "*spectator"))
		player->spectator = TRUE;
	else
		player->spectator = FALSE;

	if (!player->userinfo[0] || !player->name[0])
		CL_PlayerDropped(slot);
}

int total_data[MAX_DATA_HISTORY];

/*
=================
CL_DumpMessageLoad_f
=================
*/
void CL_DumpMessageLoad_f( void )
{
	int		i, total;

	total = 0;

	Con_Printf("-------- Message Load ---------\n");

	for (i = 0; i < MAX_DATA_HISTORY - 1; i++)
	{
		if (i > svc_lastmsg)
		{
			Con_Printf("%i:%s: %i msgs:%.2fK\n", i, "bogus #", msg_buckets[i], total_data[i] / 1024.0f);
		}
		else
		{
			Con_Printf("%i:%s: %i msgs:%.2fK\n", i, svc_strings[i], msg_buckets[i], total_data[i] / 1024.0f);
		}

		total += msg_buckets[i];
	}

	Con_Printf("User messages:  %i:%.2fK\n", msg_buckets[MAX_DATA_HISTORY - 1], total_data[MAX_DATA_HISTORY - 1] / 1024.0f);
	Con_Printf("------ End:  %i Total----\n", msg_buckets[MAX_DATA_HISTORY - 1] + total);
}

/*
=================
CL_BitCounts_f
=================
*/
void CL_BitCounts_f( void )
{
	int		i, bits;

	bits = 0;

	Con_Printf("------- Bit Counts -------\n");
	Con_Printf("Bit    Delta   Player  Custom\n");

	for (i = 0; i < (32 + 8); i++)
	{
		if (i >= 32)
		{
			Con_Printf("(1<<%2i) %6.6i\n", bits, bitcounts[i]);
		}
		else
		{
			Con_Printf("(1<<%2i) %6.6i  %6.6i  %6.6i\n", bits, bitcounts[i], playerbitcounts[i], custombitcounts[i]);
		}
		bits++;
	}

	Con_Printf("--------------------------\n");
}

/*
=================
CL_TransferMessageData
=================
*/
void CL_TransferMessageData( void )
{
	int i;
	int* pTotal;

	i = 0;
	while (i < MAX_DATA_HISTORY)
	{
		pTotal = &total_data[i];
		*pTotal += last_data[i];
		i++;
	}
}

/*
=================
CL_ShowSizes

=================
*/
void CL_ShowSizes( void )
{
}

#define SHOWNET(x) \
	if (cl_shownet.value == 2.0f && strlen(x) > 1) \
		Con_Printf("%3i:%s\n", msg_readcount - 1, x);

/*
=================
CL_ParseServerMessage

Parse incoming message from server.
=================
*/
void CL_ParseServerMessage( void )
{
	int	cmd;
	int	i, j;
	int bufStart, bufEnd;

	Sys_SetTaskName("CL_ParseServerMessage\n");

	if (cl_shownet.value == 1.0f)
	{
		Con_Printf("%i ", net_message.cursize);
	}
	else if (cl_shownet.value == 2.0f)
	{
		Con_Printf("------------------\n");
	}

	cl.onground = FALSE;	// unless the server says otherwise

	memset(last_data, 0, sizeof(last_data));

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
			Host_Error("CL_ParseServerMessage: Bad server message");

		// Mark start position
		bufStart = msg_readcount;

		cmd = MSG_ReadByte();

		// Bogus message?
		if (cmd == -1)
			break;

		if (cmd > svc_lastmsg)
		{
			msg_buckets[63]++;
			DispatchUserMsg(cmd);

			// Mark end position
			bufEnd = msg_readcount;
			last_data[63] += bufEnd - bufStart;
			continue;
		}

		SHOWNET(svc_strings[cmd]);

		if (cmd <= 63)
			msg_buckets[cmd]++;

	// other commands
		switch (cmd)
		{
		default:
			Host_Error("CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
//			Con_Printf("svc_nop\n");
			break;

		case svc_disconnect:
			SCR_EndLoadingPlaque();
			Host_EndGame("Server disconnected\n");

		case svc_updatestat:
			i = MSG_ReadByte();
			if (i >= MAX_CL_STATS)
				Sys_Error("svc_updatestat: %i is invalid", i);
			cl.stats[i] = MSG_ReadLong();
			break;

		case svc_version:
			i = MSG_ReadLong();
			if (i != PROTOCOL_VERSION)
				Host_Error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;

		case svc_setview:
			cl.viewentity = MSG_ReadShort();
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = MSG_ReadFloat();
			break;

		case svc_print:
			Con_Printf("%s", MSG_ReadString());
			break;

		case svc_stufftext:
			Cbuf_AddText(MSG_ReadString());
			break;

		case svc_setangle:
			for (i = 0; i < 3; i++)
			{
				cl.viewangles[i] = MSG_ReadHiresAngle();
			}
			break;

		case svc_serverinfo:
			CL_ParseServerInfo();
			vid.recalc_refdef = TRUE;	// leave intermission full screen
			break;

		case svc_lightstyle:
			i = MSG_ReadByte();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error("svc_lightstyle > MAX_LIGHTSTYLES");
			Q_strcpy(cl_lightstyle[i].map, MSG_ReadString());
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			break;

		case svc_updateuserinfo:
			CL_ParseUpdateUserInfo();
			break;

		case svc_clientdata:
			i = MSG_ReadShort();
			CL_ParseClientdata(i);
			break;

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i >> 3, i & 7);
			break;

		case svc_particle:
			R_ParseParticleEffect();
			break;

		case svc_damage:
			break;

		case svc_spawnstatic:
			CL_ParseStatic();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline(CL_EntityNum(i));
			break;

		case svc_tempentity:
			CL_ParseTEnt();
			break;

		case svc_setpause:
			cl.paused = MSG_ReadByte();
			break;

		case svc_signonnum:
			i = MSG_ReadByte();
			if (i <= cls.signon)
				Host_Error("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply();
			break;

		case svc_centerprint:
			SCR_CenterPrint(MSG_ReadString());
			break;

		case svc_killedmonster:
			break;

		case svc_foundsecret:
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound();
			break;

		case svc_intermission:
			cl.intermission = 1;
			vid.recalc_refdef = TRUE;	// go to full screen
			cl.completed_time = cl.time;
			break;

		case svc_finale:
			cl.intermission = 2;
			vid.recalc_refdef = TRUE;	// go to full screen
			cl.completed_time = cl.time;
			SCR_CenterPrint(MSG_ReadString());
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte();
			cl.looptrack = MSG_ReadByte();

			CDAudio_PlayTrack(cl.cdtrack, TRUE);
			break;

		case svc_restore:
			CL_ParseRestoreDecals(MSG_ReadString());
			break;

		case svc_cutscene:
			cl.intermission = 3;
			vid.recalc_refdef = TRUE;
			cl.completed_time = cl.time;
			SCR_CenterPrint(MSG_ReadString());
			break;

		case svc_weaponanim:
			cl.weaponstarttime = 0.0f;
			cl.weaponsequence = MSG_ReadByte();
			cl.viewent.baseline.body = MSG_ReadByte();
			break;

		case svc_decalname:
			i = MSG_ReadByte();
			Draw_DecalSetName(i, MSG_ReadString());
			break;

		case svc_roomtype:
			Cvar_SetValue("room_type", MSG_ReadShort());
			break;

		case svc_addangle:
			cl.viewangles[YAW] += MSG_ReadHiresAngle();
			break;

		case svc_newusermsg:
			AddNewUserMsg();
			break;

		case svc_download:
			CL_ParseDownload();
			break;

		case svc_packetentities:
			CL_ParsePacketEntities(FALSE);
			CL_SetSolidEntities();
			break;

		case svc_deltapacketentities:
			CL_ParsePacketEntities(TRUE);
			CL_SetSolidEntities();
			break;

		case svc_playerinfo:
			CL_ParsePlayerinfo();
			break;

		case svc_chokecount:
			i = MSG_ReadByte();
			for (j = 0; j < i; j++)
			cl.frames[(cls.netchan.incoming_acknowledged - 1 - j) & cl_update_mask].receivedtime = -2.0f;
			break;

		case svc_resourcelist:
			CL_ParseResourceList();
			break;

		case svc_newmovevars:
			CL_ParseMovevars();
			break;

		case svc_nextupload:
			CL_ParseNextUpload();
			break;

		case svc_resourcerequest:
			CL_SendResourceListBlock();
			break;

		case svc_crosshairangle:
			cl.crosshairangle[PITCH] = MSG_ReadChar() * 0.2f;
			cl.crosshairangle[YAW] = MSG_ReadChar() * 0.2f;
			break;

		case svc_soundfade:
			CL_ParseSoundFade();
			break;

		case svc_clientmaxspeed:
			i = MSG_ReadByte();
			if (i >= cl.maxclients)
				Host_Error("CL_ParseServerMessage: svc_clientmaxspeed >= cl.maxclients");

			cl.players[i].maxspeed = MSG_ReadFloat();
			if (cl.players[i].maxspeed > movevars.maxspeed)
				cl.players[i].maxspeed = movevars.maxspeed;
			break;

		case svc_skippedupdate:
			i = MSG_ReadByte();
			if (cl.frames[i & cl_update_mask].receivedtime == -1.0f ||
				cl.frames[i & cl_update_mask].receivedtime == -2.0f)
			{
				cl.frames[i & cl_update_mask].receivedtime = -3.0f;
			}
			break;
		}

		// Mark end position
		bufEnd = msg_readcount;
		last_data[cmd] += bufEnd - bufStart;

		if (cmd == svc_packetentities || cmd == svc_deltapacketentities)
		{
			cl.frames[parsecountmod].packet_entities_bytes += bufEnd - bufStart;
		}
		else if (cmd == svc_playerinfo)
		{
			cl.frames[parsecountmod].playerinfo_bytes += bufEnd - bufStart;
		}
		else if (cmd == svc_tempentity)
		{
			cl.frames[parsecountmod].temp_entity_bytes += bufEnd - bufStart;
		}
		else if (cmd == svc_sound)
		{
			cl.frames[parsecountmod].sound_bytes += bufEnd - bufStart;
		}
	}

	// end of message
	SHOWNET("END OF MESSAGE");
	cl.frames[parsecountmod].message_bytes += net_message.cursize;

	CL_TransferMessageData();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	CL_SetSolidEntities();
}

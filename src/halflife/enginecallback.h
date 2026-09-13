/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef ENGINECALLBACK_H
#define ENGINECALLBACK_H
// Engine services are linked directly into the Dreamcast image.
#include "eng_builtins.h"

// The actual engine callbacks
#define GETPLAYERUSERID PF_GetPlayerUserId
#define PRECACHE_MODEL	PF_precache_model_I
#define PRECACHE_SOUND	PF_precache_sound_I
#define PRECACHE_GENERIC	PF_precache_generic_I
#define SET_MODEL		PF_setmodel_I
#define MODEL_INDEX		PF_modelindex
#define MODEL_FRAMES	ModelFrames
#define SET_SIZE		PF_setsize_I
#define CHANGE_LEVEL	PF_changelevel_I
#define GET_SPAWN_PARMS	PF_setspawnparms_I
#define SAVE_SPAWN_PARMS SaveSpawnParms
#define VEC_TO_YAW		PF_vectoyaw_I
#define VEC_TO_ANGLES	PF_vectoangles_I
#define MOVE_TO_ORIGIN  SV_MoveToOrigin_I
#define oldCHANGE_YAW		PF_changeyaw_I
#define CHANGE_PITCH	PF_changepitch_I
#define MAKE_VECTORS	PF_makevectors_I
#define CREATE_ENTITY	PF_Spawn_I
#define REMOVE_ENTITY	PF_Remove_I
#define CREATE_NAMED_ENTITY		CreateNamedEntity
#define MAKE_STATIC		PF_makestatic_I
#define ENT_IS_ON_FLOOR	PF_checkbottom_I
#define DROP_TO_FLOOR	PF_droptofloor_I
#define WALK_MOVE		PF_walkmove_I
#define SET_ORIGIN		PF_setorigin_I
#define EMIT_SOUND_DYN2 PF_sound_I
#define BUILD_SOUND_MSG PF_BuildSoundMsg
#define TRACE_LINE		PF_traceline_DLL
#define TRACE_TOSS		PF_TraceToss_DLL
#define TRACE_MONSTER_HULL		TraceMonsterHull
#define TRACE_HULL		TraceHull
#define GET_AIM_VECTOR	PF_aim_I
#define SERVER_COMMAND	PF_localcmd_I
#define SERVER_EXECUTE	PF_localexec_I
#define CLIENT_COMMAND	PF_stuffcmd_I
#define PARTICLE_EFFECT	PF_particle_I
#define LIGHT_STYLE		PF_lightstyle_I
#define DECAL_INDEX		PF_DecalIndex
#define POINT_CONTENTS	PF_pointcontents_I
#define CRC32_INIT           CRC32_Init
#define CRC32_PROCESS_BUFFER CRC32_ProcessBuffer
#define CRC32_PROCESS_BYTE   CRC32_ProcessByte
#define CRC32_FINAL          CRC32_Final
#define RANDOM_LONG		RandomLong
#define RANDOM_FLOAT	RandomFloat

inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin = NULL, edict_t *ed = NULL ) {
	PF_MessageBegin_I(msg_dest, msg_type, pOrigin, ed);
}
#define MESSAGE_END		PF_MessageEnd_I
#define WRITE_BYTE		PF_WriteByte_I
#define WRITE_CHAR		PF_WriteChar_I
#define WRITE_SHORT		PF_WriteShort_I
#define WRITE_LONG		PF_WriteLong_I
#define WRITE_ANGLE		PF_WriteAngle_I
#define WRITE_COORD		PF_WriteCoord_I
#define WRITE_STRING	PF_WriteString_I
#define WRITE_ENTITY	PF_WriteEntity_I
#define CVAR_REGISTER	CVarRegister
#define CVAR_GET_FLOAT	CVarGetFloat
#define CVAR_GET_STRING	CVarGetString
#define CVAR_GET_POINTER CVarGetPointer
#define CVAR_SET_FLOAT	CVarSetFloat
#define CVAR_SET_STRING	CVarSetString
#define ALERT			AlertMessage
#define ENGINE_FPRINTF	EngineFprintf
#define ALLOC_PRIVATE	PvAllocEntPrivateData
inline void *GET_PRIVATE( edict_t *pent )
{
	if ( pent )
		return pent->pvPrivateData;
	return NULL;
}

#define FREE_PRIVATE	FreeEntPrivateData
//#define STRING			SzFromIndex
#define ALLOC_STRING	AllocEngineString
#define FIND_ENTITY_BY_STRING	FindEntityByString
#define GETENTITYILLUM	GetEntityIllum
#define FIND_ENTITY_IN_SPHERE		FindEntityInSphere
#define FIND_CLIENT_IN_PVS			PF_checkclient_I
#define EMIT_AMBIENT_SOUND			PF_ambientsound_I
#define GET_MODEL_PTR				GetModelPtr
#define REG_USER_MSG				RegUserMsg
#define GET_BONE_POSITION			PF_GetBonePosition
#define FUNCTION_FROM_NAME			FunctionFromName
#define NAME_FOR_FUNCTION			NameForFunction
#define TRACE_TEXTURE				TraceTexture
#define CLIENT_PRINTF				ClientPrintf
#define CMD_ARGS					Cmd_Args
#define CMD_ARGC					Cmd_Argc
#define CMD_ARGV					Cmd_Argv
#define GET_ATTACHMENT			PF_GetAttachment
#define SET_VIEW				PF_setview_I
#define SET_CROSSHAIRANGLE		PF_crosshairangle_I
#define LOAD_FILE_FOR_ME		COM_LoadFileForMe
#define FREE_FILE				COM_FreeFile
#define COMPARE_FILE_TIME		COM_CompareFileTime
#define GET_GAME_DIR			COM_GetGameDir
#define IS_MAP_VALID			PF_IsMapValid_I
#define NUMBER_OF_ENTITIES		PF_NumberOfEntities_I
#define IS_DEDICATED_SERVER		PF_IsDedicatedServer

#endif		//ENGINECALLBACK_H

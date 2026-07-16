/***
*
*  Prototypes for the engine service routines the game DLL calls through
*  g_engfuncs.  On the Dreamcast the engine and game link into one image,
*  so these resolve to the real engine functions at link time and each
*  g_engfuncs call folds to a direct call.
*
****/
#ifndef ENG_BUILTINS_H
#define ENG_BUILTINS_H

#ifdef __cplusplus
extern "C" {
#endif

extern int PF_precache_model_I (char* s);
extern int PF_precache_sound_I (char* s);
extern void PF_setmodel_I (edict_t *e, const char *m);
extern int PF_modelindex (const char *m);
extern int ModelFrames (int modelIndex);
extern void PF_setsize_I (edict_t *e, const float *rgflMin, const float *rgflMax);
extern void PF_changelevel_I (char* s1, char* s2);
extern void PF_setspawnparms_I (edict_t *ent);
extern void SaveSpawnParms (edict_t *ent);
extern float PF_vectoyaw_I (const float *rgflVector);
extern void PF_vectoangles_I (const float *rgflVectorIn, float *rgflVectorOut);
extern void SV_MoveToOrigin_I (edict_t *ent, const float *pflGoal, float dist, int iMoveType);
extern void PF_changeyaw_I (edict_t* ent);
extern void PF_changepitch_I (edict_t* ent);
extern edict_t* FindEntityByString (edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue);
extern int GetEntityIllum (edict_t* pEnt);
extern edict_t* FindEntityInSphere (edict_t *pEdictStartSearchAfter, const float *org, float rad);
extern edict_t* PF_checkclient_I (edict_t *pEdict);
extern edict_t* PVSFindEntities (edict_t *pplayer);
extern void PF_makevectors_I (const float *rgflVector);
extern void AngleVectors (const float *rgflVector, float *forward, float *right, float *up);
extern edict_t* PF_Spawn_I (void);
extern void PF_Remove_I (edict_t* e);
extern edict_t* CreateNamedEntity (int className);
extern void PF_makestatic_I (edict_t *ent);
extern int PF_checkbottom_I (edict_t *e);
extern int PF_droptofloor_I (edict_t* e);
extern int PF_walkmove_I (edict_t *ent, float yaw, float dist, int iMode);
extern void PF_setorigin_I (edict_t *e, const float *rgflOrigin);
extern void PF_sound_I (edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch);
extern void PF_ambientsound_I (edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch);
extern void PF_traceline_DLL (const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
extern void PF_TraceToss_DLL (edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr);
extern int TraceMonsterHull (edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);
extern void TraceHull (const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr);
extern void TraceModel (const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr);
extern const char * TraceTexture (edict_t *pTextureEntity, const float *v1, const float *v2 );
extern void TraceSphere (const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr);
extern void PF_aim_I (edict_t* ent, float speed, float *rgflReturn);
extern void PF_localcmd_I (char* str);
extern void PF_localexec_I (void);
extern void PF_stuffcmd_I (edict_t* pEdict, char* szFmt, ...);
extern void PF_particle_I (const float *org, const float *dir, float color, float count);
extern void PF_lightstyle_I (int style, char* val);
extern int PF_DecalIndex (const char *name);
extern int PF_pointcontents_I (const float *rgflVector);
extern void PF_MessageBegin_I (int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
extern void PF_MessageEnd_I (void);
extern void PF_WriteByte_I (int iValue);
extern void PF_WriteChar_I (int iValue);
extern void PF_WriteShort_I (int iValue);
extern void PF_WriteLong_I (int iValue);
extern void PF_WriteAngle_I (float flValue);
extern void PF_WriteCoord_I (float flValue);
extern void PF_WriteString_I (const char *sz);
extern void PF_WriteEntity_I (int iValue);
extern void CVarRegister (cvar_t *pCvar);
extern float CVarGetFloat (const char *szVarName);
extern const char* CVarGetString (const char *szVarName);
extern void CVarSetFloat (const char *szVarName, float flValue);
extern void CVarSetString (const char *szVarName, const char *szValue);
extern void AlertMessage (ALERT_TYPE atype, char *szFmt, ...);
extern void EngineFprintf (FILE *pfile, char *szFmt, ...);
extern void* PvAllocEntPrivateData (edict_t *pEdict, long cb);
extern void* PvEntPrivateData (edict_t *pEdict);
extern void FreeEntPrivateData (edict_t *pEdict);
extern const char* SzFromIndex (int iString);
extern int AllocEngineString (const char *szValue);
extern entvars_t* GetVarsOfEnt (edict_t *pEdict);
extern edict_t* PEntityOfEntOffset (int iEntOffset);
extern int EntOffsetOfPEntity (const edict_t *pEdict);
extern int IndexOfEdict (const edict_t *pEdict);
extern edict_t* PEntityOfEntIndex (int iEntIndex);
extern edict_t* FindEntityByVars (entvars_t* pvars);
extern void* GetModelPtr (edict_t* pEdict);
extern int RegUserMsg (const char *pszName, int iSize);
extern void PF_AnimationAutomove (const edict_t* pEdict, float flTime);
extern void PF_GetBonePosition (const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles );
extern unsigned long FunctionFromName ( const char *pName );
extern const char * NameForFunction ( unsigned long function );
extern void ClientPrintf ( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg );
extern void ServerPrint ( const char *szMsg );
extern const char * Cmd_Args ( void );
extern const char * Cmd_Argv ( int argc );
extern int Cmd_Argc ( void );
extern void PF_GetAttachment (const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles );
extern void CRC32_Init (CRC32_t *pulCRC);
extern void CRC32_ProcessBuffer (CRC32_t *pulCRC, void *p, int len);
extern void CRC32_ProcessByte (CRC32_t *pulCRC, unsigned char ch);
extern CRC32_t CRC32_Final (CRC32_t pulCRC);
extern long RandomLong (long  lLow,  long  lHigh);
extern float RandomFloat (float flLow, float flHigh);
extern void PF_setview_I (const edict_t *pClient, const edict_t *pViewent );
extern float PF_Time ( void );
extern void PF_crosshairangle_I (const edict_t *pClient, float pitch, float yaw);
extern byte * COM_LoadFileForMe (char *filename, int *pLength);
extern void COM_FreeFile (void *buffer);
extern void Host_EndSection (const char *pszSectionName);
extern int COM_CompareFileTime (char *filename1, char *filename2, int *iCompare);
extern void COM_GetGameDir (char *szGetGameDir);
extern void Cvar_RegisterVariable (cvar_t *variable);
extern void PF_FadeVolume (const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
extern void PF_SetClientMaxspeed (const edict_t *pEdict, float fNewMaxspeed);
extern edict_t * PF_CreateFakeClient_I (const char *netname);
extern void PF_RunPlayerMove_I (edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec );
extern int PF_NumberOfEntities_I (void);
extern char* PF_GetInfoKeyBuffer_I (edict_t *e);
extern char* PF_InfoKeyValue (char *infobuffer, char *key);
extern void PF_SetKeyValue (char *infobuffer, char *key, char *value);
extern void PF_SetClientKeyValue (int clientIndex, char *infobuffer, char *key, char *value);
extern int PF_IsMapValid_I (char *filename);
extern void PF_StaticDecal ( const float *origin, int decalIndex, int entityIndex, int modelIndex );
extern int PF_precache_generic_I (char* s);
extern int PF_GetPlayerUserId (edict_t *e );
extern void PF_BuildSoundMsg (edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
extern int PF_IsDedicatedServer (void);

#ifdef __cplusplus
}
#endif

#endif // ENG_BUILTINS_H

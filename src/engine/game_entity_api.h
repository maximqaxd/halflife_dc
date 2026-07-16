/***
*
*  DLL_FUNCTIONS entity dispatch routines the engine calls in the game.
*  The Dreamcast links engine and game into one image, so the dispatch
*  table is fixed at link time and each call folds to a direct call.
*
****/
#ifndef GAME_ENTITY_API_H
#define GAME_ENTITY_API_H

#ifdef __cplusplus
extern "C" {
#endif

extern void GameDLLInit ( void );
extern int DispatchSpawn ( edict_t *pent );
extern void DispatchThink ( edict_t *pent );
extern void DispatchUse ( edict_t *pentUsed, edict_t *pentOther );
extern void DispatchTouch ( edict_t *pentTouched, edict_t *pentOther );
extern void DispatchBlocked ( edict_t *pentBlocked, edict_t *pentOther );
extern void DispatchKeyValue ( edict_t *pentKeyvalue, KeyValueData *pkvd );
extern void DispatchSave ( edict_t *pent, SAVERESTOREDATA *pSaveData );
extern int DispatchRestore ( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity );
extern void DispatchObjectCollsionBox ( edict_t *pent );
extern void SaveWriteFields ( SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int );
extern void SaveReadFields ( SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int );
extern void SaveGlobalState ( SAVERESTOREDATA * );
extern void RestoreGlobalState ( SAVERESTOREDATA * );
extern void ResetGlobalState ( void );
extern int ClientConnect ( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
extern void ClientDisconnect ( edict_t *pEntity );
extern void ClientKill ( edict_t *pEntity );
extern void ClientPutInServer ( edict_t *pEntity );
extern void ClientCommand ( edict_t *pEntity );
extern void ClientUserInfoChanged ( edict_t *pEntity, char *infobuffer );
extern void ServerActivate ( edict_t *pEdictList, int edictCount, int clientMax );
extern void PlayerPreThink ( edict_t *pEntity );
extern void PlayerPostThink ( edict_t *pEntity );
extern void StartFrame ( void );
extern void ParmsNewLevel ( void );
extern void ParmsChangeLevel ( void );
extern const char     * GetGameDescription ( void );
extern void PlayerCustomization ( edict_t *pEntity, customization_t *pCustom );
extern void SpectatorConnect ( edict_t *pEntity );
extern void SpectatorDisconnect ( edict_t *pEntity );
extern void SpectatorThink ( edict_t *pEntity );

static const DLL_FUNCTIONS gEntityInterface =
{
	GameDLLInit,
	DispatchSpawn,
	DispatchThink,
	DispatchUse,
	DispatchTouch,
	DispatchBlocked,
	DispatchKeyValue,
	DispatchSave,
	DispatchRestore,
	DispatchObjectCollsionBox,
	SaveWriteFields,
	SaveReadFields,
	SaveGlobalState,
	RestoreGlobalState,
	ResetGlobalState,
	ClientConnect,
	ClientDisconnect,
	ClientKill,
	ClientPutInServer,
	ClientCommand,
	ClientUserInfoChanged,
	ServerActivate,
	PlayerPreThink,
	PlayerPostThink,
	StartFrame,
	ParmsNewLevel,
	ParmsChangeLevel,
	GetGameDescription,
	PlayerCustomization,
	SpectatorConnect,
	SpectatorDisconnect,
	SpectatorThink,
};

#ifdef __cplusplus
}
#endif

#endif // GAME_ENTITY_API_H

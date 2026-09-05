#if !defined( HUD_HANDLERS_H )
#define HUD_HANDLERS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

int		GetScreenInfo( SCREENINFO* pscrinfo );

// cvar handling
int		CVAR_CREATE( char* szName, char* szValue, int flags );
float	CVAR_GET_FLOAT( char* szName );
char*	CVAR_GET_STRING( char* szName );

int		HOOK_COMMAND( char* cmd_name, void (*function)(void) );

// message handling
int		ServerCmd( char* pszCmdString );
int		ClientCmd( char* pszCmdString );

int		HookUserMsg( char* szMsgName, pfnUserMsgHook pfn );
void	HUD_GetPlayerInfo( int ent_num, hud_player_info_t* pinfo );

#ifdef PlaySound
#undef PlaySound
#endif

void	PlaySound( char* szSound, float volume );

void	GetConsoleStringSize( const char* string, int* length, int* height );
void	ConsolePrint( const char* string );
void	CenterPrint( const char* string );

#ifdef __cplusplus
}
#endif

#endif // HUD_HANDLERS_H

//
// eng_cdll_exp.c
//
// 4-23-98  JOHN
//  this file implements the functions exported the the client-side (HUD-drawing) DLL
//

#include "quakedef.h"
#include "cdll_int.h"

extern pfnUserMsgHook CL_HookUserMsg( char* pszName, pfnUserMsgHook pfn );
extern int Font_CharHeight( qfont_t* font );

#ifdef PlaySound
#undef PlaySound
#endif

int GetScreenInfo( SCREENINFO* pscrinfo )
{
	if (!pscrinfo)
		return 0;

	if (pscrinfo->iSize != sizeof(SCREENINFO))
		return 0;

	pscrinfo->iWidth = vid.width;
	pscrinfo->iHeight = vid.height;
#if defined ( GLQUAKE )
	pscrinfo->iFlags = SCRINFO_SCREENFLASH;
#else
	pscrinfo->iFlags = 0;
	if (vid_stretched)
		pscrinfo->iFlags |= SCRINFO_STRETCHED;
#endif
	pscrinfo->iCharHeight = Draw_MessageFontInfo(pscrinfo->charWidths);
	return sizeof(SCREENINFO);
}

int CVAR_CREATE( char* szName, char* szValue, int flags )
{
	cvar_t* cv;

	cv = (cvar_t*)Z_Malloc(sizeof(cvar_t));
	cv->name = szName;
	cv->string = szValue;
	cv->flags = flags | FCVAR_CLIENTDLL;

	Cvar_RegisterVariable(cv);

	return 1;
}

float CVAR_GET_FLOAT( char* szName )
{
	cvar_t* cv;

	cv = Cvar_FindVar(szName);
	if (cv)
		return cv->value;

	return 0.0f;
}

char* CVAR_GET_STRING( char* szName )
{
	cvar_t* cv;

	cv = Cvar_FindVar(szName);
	if (cv)
		return cv->string;

	return NULL;
}

int HOOK_COMMAND( char* cmd_name, void (*function)(void) )
{
	Cmd_AddHUDCommand(cmd_name, function);
	return 1;
}

int ServerCmd( char* pszCmdString )
{
	char buf[256];
	// just like the client typed "cmd xxxxx" at the console

	strcpy(buf, "cmd ");
	strncat(buf, pszCmdString, 250);

	Cmd_TokenizeString(buf);
	Cmd_ForwardToServer();

	return TRUE;
}

int ClientCmd( char* pszCmdString )
{
	if (!pszCmdString)
		return 0;

	Cbuf_AddText(pszCmdString);
	Cbuf_AddText("\n");
	return 1;
}

int HookUserMsg( char* szMsgName, pfnUserMsgHook pfn )
{
	return CL_HookUserMsg(szMsgName, pfn) != NULL;
}

void HUD_GetPlayerInfo( int ent_num, hud_player_info_t* pinfo )
{
	ent_num--;

	if (ent_num >= cl.maxclients || ent_num < 0 || !cl.players[ent_num].name[0])
	{
		pinfo->name = NULL;
		return;
	}

	pinfo->name = cl.players[ent_num].name;
	pinfo->ping = cl.players[ent_num].ping;
	pinfo->thisplayer = ent_num == cl.playernum;
	pinfo->spectator = cl.players[ent_num].spectator;
	pinfo->packetloss = 0;
}

void PlaySound( char* szSound, float volume )
{
	sfx_t* sfx;

	if (volume <= 0.0f)
		volume = 0.0f;
	if (volume >= 1.0f)
		volume = 1.0f;

	sfx = S_FindName(szSound);
	if (sfx)
		S_StartDynamicSound(cl.viewentity, CHAN_ITEM, sfx, r_origin, volume, 1.0f, 0, PITCH_NORM);
}

// Gets the length in pixels of a string if it were drawn onscreen
void GetConsoleStringSize( const char* string, int* length, int* height )
{
	*length = Draw_StringLen((char*)string);
	*height = Font_CharHeight(draw_chars);
}

void ConsolePrint( const char* string )
{
	Con_Print((char*)string);
}

void CenterPrint( const char* string )
{
	SCR_CenterPrint((char*)string);
}

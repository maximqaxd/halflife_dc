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
//
// util.h
//

#include <windows.h>

#include "../engine/cvardef.h"

#ifdef PlaySound
#undef PlaySound
#endif

extern "C"
{
	int GetScreenInfo( SCREENINFO* pscrinfo );
	int CVAR_CREATE( char* szName, char* szValue, int flags );
	float CVAR_GET_FLOAT( char* szName );
	char* CVAR_GET_STRING( char* szName );
	int HOOK_COMMAND( char* cmd_name, void (*function)(void) );
	int ServerCmd( char* pszCmdString );
	int ClientCmd( char* pszCmdString );
	int HookUserMsg( char* szMsgName, pfnUserMsgHook pfn );
	void PlaySound( char* szSound, float volume );
	void GetConsoleStringSize( const char* string, int* length, int* height );
	void ConsolePrint( const char* string );
	void CenterPrint( const char* string );
	void HUD_GetPlayerInfo( int ent_num, hud_player_info_t* pinfo );

	HSPRITE_t SPR_Load( const char* pTextureName );
	client_sprite_t* SPR_GetList( char* psz, int* piCount );
	int SPR_Frames( HSPRITE_t hSprite );
	int SPR_Width( HSPRITE_t hSprite, int frame );
	int SPR_Height( HSPRITE_t hSprite, int frame );
	void SPR_Set( HSPRITE_t hSprite, int r, int g, int b );
	void SPR_Draw( int frame, int x, int y, const wrect_t* prc );
	void SPR_DrawHoles( int frame, int x, int y, const wrect_t* prc );
	void SPR_DrawAdditive( int frame, int x, int y, const wrect_t* prc );
	void SPR_EnableScissor( int x, int y, int width, int height );
	void SPR_DisableScissor( void );
	void SetCrosshair( HSPRITE_t hspr, wrect_t rc, int r, int g, int b );

	void Draw_FillRGBA( int x, int y, int width, int height, int r, int g, int b, int a );
	void AngleVectors( const vec_t* angles, vec_t* forward, vec_t* right, vec_t* up );
	client_textmessage_t* TextMessageGet( const char* pName );
	int TextMessageDrawCharacter( int x, int y, int number, int r, int g, int b );
	int Draw_String( int x, int y, char* str );
}

// Macros to hook function calls into the HUD object
#define HOOK_MESSAGE(x) HookUserMsg(#x, __MsgFunc_##x );

#define DECLARE_MESSAGE(y, x) int __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) \
							{ \
							return gHUD.##y.MsgFunc_##x(pszName, iSize, pbuf ); \
							}


#define HOOK_COMMAND(x, y) (HOOK_COMMAND)( x, __CmdFunc_##y );
#define DECLARE_COMMAND(y, x) void __CmdFunc_##x( void ) \
							{ \
								gHUD.##y.UserCmd_##x( ); \
							}

#define CVAR_GET_FLOAT(x) (CVAR_GET_FLOAT)((char*)(x))
#define CVAR_GET_STRING(x) (CVAR_GET_STRING)((char*)(x))
#define CVAR_CREATE(cv, val, flags) (CVAR_CREATE)((char*)(cv), (char*)(val), (flags))

#define FillRGBA Draw_FillRGBA


// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight (gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth (gHUD.m_scrinfo.iWidth)

inline int DrawConsoleString( int x, int y, const char *string )
{ 
	return Draw_String(x, y, (char*)string);
}

inline int TextMessageDrawChar( int x, int y, int number, int r, int g, int b )
{
	return TextMessageDrawCharacter(x, y, number, r, g, b);
}

#define GetConsoleStringSize(string, width, height) (GetConsoleStringSize)((string), (width), (height))

inline int ConsoleStringLen( const char *string )
{
	int _width, _height;
	GetConsoleStringSize( string, &_width, &_height );
	return _width;
}

#define ConsolePrint(string) (ConsolePrint)((string))
#define CenterPrint(string) (CenterPrint)((string))

#define GetPlayerInfo HUD_GetPlayerInfo

#define PlaySound(szSound, vol) (PlaySound)((char*)(szSound), (vol))

#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))

void ScaleColors( int &r, int &g, int &b, int a );

#ifndef DotProduct
#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#endif
#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];}

// disable 'possible loss of data converting float to int' warning message
#pragma warning( disable: 4244 )
// disable 'truncation from 'const double' to 'float' warning message
#pragma warning( disable: 4305 )

inline void UnpackRGB(int &r, int &g, int &b, unsigned long ulRGB)\
{\
	r = (ulRGB & 0xFF0000) >>16;\
	g = (ulRGB & 0xFF00) >> 8;\
	b = ulRGB & 0xFF;\
}

HSPRITE LoadSprite(const char *pszName);

//
//  eng_cdll_int.c
//
// 4-23-98  
// JOHN:  implementation of interface between client-side DLL and game engine.
//  The cdll shouldn't have to know anything about networking or file formats.
//  This file is Win32-dependant
//


#include "quakedef.h"
#include "winquake.h"
#include "screen.h"
#include "cl_demo.h"
#include "cl_draw.h"
#include "tmessage.h"
#include "hud_handlers.h"

client_sprite_t* SPR_GetList( char* psz, int* piCount );

extern int HUD_VidInit( void );
extern int HUD_Init( void );
extern int HUD_Redraw( float flTime, int intermission );
extern int HUD_UpdateClientData( client_data_t *cdata, float flTime );
extern void SPR_Shutdown( void );
extern void Cvar_RemoveHudCvars( void );
extern void Cmd_RemoveHudCmds( void );
extern void CL_ClearUserMessages( void );
extern int COM_ExpandFilename( char* filename );
extern pfnUserMsgHook CL_HookUserMsg( char* pszName, pfnUserMsgHook pfn );

// Pointers to the exported client functions themselves
typedef struct
{
	int  (*pInitFunc)( struct cl_enginefuncs_s* pEnginefuncs, int iVersion );
	void (*pHudInitFunc)( void );
	int  (*pHudVidInitFunc)( void );
	int  (*pHudRedrawFunc)( float, int );
	int  (*pHudUpdateClientDataFunc)( client_data_t* , float );
	void (*pHudResetFunc)( void );
} cldll_func_t;

cldll_func_t cl_funcs;
static HMODULE cl_dllhandle;

/*
==============
ClientDLL_Init

Loads the client .dll
==============
*/
void ClientDLL_Init( void )
{
	char	szDllName[512];

	if (cl_dllhandle)
	{
		SPR_Shutdown();
		FreeLibrary(cl_dllhandle);
		cl_dllhandle = NULL;
		cl_funcs.pHudResetFunc = NULL;
		cl_funcs.pHudUpdateClientDataFunc = NULL;
		cl_funcs.pHudRedrawFunc = NULL;
		cl_funcs.pHudVidInitFunc = NULL;
		cl_funcs.pHudInitFunc = NULL;
		cl_funcs.pInitFunc = NULL;
		Cvar_RemoveHudCvars();
		Cmd_RemoveHudCmds();
		CL_ClearUserMessages();
	}

	sprintf(szDllName, "cl_dlls\\client.dll");
	COM_ExpandFilename(szDllName);
	CL_HookUserMsg("ScreenShake", V_ScreenShake);
	CL_HookUserMsg("ScreenFade", V_ScreenFade);
}

void ClientDLL_Shutdown( void )
{
	SPR_Shutdown();
	FreeLibrary(cl_dllhandle);
	cl_dllhandle = NULL;
	cl_funcs.pHudResetFunc = NULL;
	cl_funcs.pHudUpdateClientDataFunc = NULL;
	cl_funcs.pHudRedrawFunc = NULL;
	cl_funcs.pHudVidInitFunc = NULL;
	cl_funcs.pHudInitFunc = NULL;
	cl_funcs.pInitFunc = NULL;
	Cvar_RemoveHudCvars();
	Cmd_RemoveHudCmds();
	CL_ClearUserMessages();
}

/*
==============
ClientDLL_HudVidInit

Called when the game initializes and whenever the vid_mode is changed
 so the HUD can reinitialize itself.
==============
*/
void ClientDLL_HudVidInit( void )
{
	SPR_Init();
	HUD_VidInit();
}

/*
==============
ClientDLL_HudInit

Called to initialize the client library
This occurs after the engine has loaded the client and has initialized all other systems
==============
*/
void ClientDLL_HudInit( void )
{
	HUD_Init();
}

/*
==============
ClientDLL_HudRedraw

Called to redraw the HUD
==============
*/
void ClientDLL_HudRedraw( int intermission )
{
	float time;

	time = cl.time;
	HUD_Redraw(time, intermission);
}

/*
==============
ClientDLL_UpdateClientData

Called every frame while running a map
==============
*/
void ClientDLL_UpdateClientData( void )
{
	client_data_t cdat;

	if (!cl.spectator)
	{
		memset(&cdat, 0, sizeof(cdat));

		cdat.viewheight = cl.viewheight;
		cdat.maxspeed = cl.maxspeed;

		VectorCopy(cl.viewangles, cdat.viewangles);
		VectorCopy(cl.punchangle, cdat.punchangle);
		VectorCopy(cl_entities[cl.viewentity].origin, cdat.origin);
		
		cdat.iKeyBits = CL_ButtonBits(0);

		cdat.fov = scr_fov_value;
		cdat.iWeaponBits = cl.weapons;
		cdat.view_idlescale = v_idlescale;
		cdat.mouse_sensitivity = sensitivity.value;

		if (HUD_UpdateClientData(&cdat, cl.time))
		{
			cl.viewheight = cdat.viewheight;
			cl.maxspeed = cdat.maxspeed;

			VectorCopy(cdat.viewangles, cl.viewangles);
			VectorCopy(cdat.punchangle, cl.punchangle);

			scr_fov_value = cdat.fov;
			sensitivity.value = cdat.mouse_sensitivity;
			v_idlescale = cdat.view_idlescale;

			CL_ResetButtonBits(cdat.iKeyBits);
		}
	}
}

/*
=================
SPR_GetList

Loads a sprite list. This is a text file defining a list of HUD elements
Free the returned list with COM_FreeFile
=================
*/
client_sprite_t* SPR_GetList( char* psz, int* piCount )
{
	char* pfile;
	int		iCount, i;
	client_sprite_t* ps, * pret;

	pfile = (char*)COM_LoadFile(psz, 2, NULL);
	if (!pfile)
	{
		COM_FreeFile();
		return NULL;
	}

	pfile = COM_Parse(pfile);
	iCount = atoi(com_token);
	if (!iCount)
	{
		COM_FreeFile();
		return NULL;
	}
	
	pret = (client_sprite_t*)calloc(sizeof(client_sprite_t) * iCount, 1);
	if (pret)
	{
		ps = pret;

		for (i = 0; i < iCount; i++, ps++)
		{
			pfile = COM_Parse(pfile);
			strcpy(ps->szName, com_token);

			pfile = COM_Parse(pfile);
			ps->iRes = atoi(com_token);

			pfile = COM_Parse(pfile);
			strcpy(ps->szSprite, com_token);

			pfile = COM_Parse(pfile);
			ps->rc.left = atoi(com_token);
			pfile = COM_Parse(pfile);
			ps->rc.top = atoi(com_token);
			pfile = COM_Parse(pfile);
			ps->rc.right = ps->rc.left + atoi(com_token);
			pfile = COM_Parse(pfile);
			ps->rc.bottom = ps->rc.top + atoi(com_token);
		}

		if (piCount)
			*piCount = iCount;
	}

	COM_FreeFile();

	return pret;
}

// ui.c -- in-game user interface (menu) system

#include "quakedef.h"
#include "ui.h"

int		gfDrawMenu;			// draw the menu instead of the 3D view this frame
void*	gpActiveMenu;		// menu currently open, NULL if none

int		g_bScreenSaverActive;	// attract mode is running

/*
==================
UI_OpenMenu

Open a menu page by name. The cache is handed back first so the page has room
for its own artwork.
==================
*/
void UI_OpenMenu( char* pszMenu )
{
	if (cls.state == ca_active || cls.state == ca_disconnected)
	{
		Cache_FlushToDisk();
		Cache_FreeAll();

		// TODO: build the named page and make it the active menu
		gpActiveMenu = NULL;
	}
}

/*
==================
M_EnableAllItems

Unlock every entry on the current page.
==================
*/
void M_EnableAllItems( void )
{
	// TODO: clear the disabled flag on the active menu's items
}

/*
==================
M_DecodeStateFlags

==================
*/
void M_DecodeStateFlags( void )
{
	// TODO: resync menu/UI state on screen-saver wakeup
}

/*
==================
UI_Draw

Draw the open menu. Called from SCR_UpdateScreen when gfDrawMenu is set.
==================
*/
void UI_Draw( void )
{
	if (!gpActiveMenu)
		return;

	// TODO: draw the active menu's items
}

/*
==================
UI_Update

Run menu input and tear the menu down once it has been closed.
==================
*/
void UI_Update( void )
{
	if (!gpActiveMenu)
		return;

	// TODO: dispatch controller input to the active menu; free it once
	// gfDrawMenu has been cleared
}

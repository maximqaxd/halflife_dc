// ui.c -- in-game user interface (menu) system

#include "quakedef.h"

int		gfDrawMenu;			// draw the menu instead of the 3D view this frame
void*	gpActiveMenu;		// menu currently open, NULL if none

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

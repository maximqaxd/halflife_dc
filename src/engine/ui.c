// ui.c -- engine side of the front-end user interface
//
// The front end draws its own pages, but it leans on the engine for three
// things, and they all live here:
//
//   - a border drawer, so a page can frame a rectangle without going through
//     the picture cache;
//   - a list of elements, each pairing a lump of the caller's data with
//     whoever owns it. They go on the end of the list and come back off
//     the front, so they are handled in the order they were added;
//   - the keyboard hand-off. While a page is up the keys have to stop going
//     to the game, so the page takes the keyboard on the way in and gives
//     it back on the way out.
//
// None of this knows anything about the pages themselves; the front end
// owns their artwork and their layout.

#include "quakedef.h"
#include "ui.h"

// Elements the front end is showing, oldest first
static uielement_t*	ui_elements;

// Where the keys were going before the UI took the keyboard away
static keydest_t	ui_savedkeydest;

/*
==============
UI_DrawOutlineRect

Frame the rectangle with a one pixel border.
==============
*/
void UI_DrawOutlineRect( vrect_t* r, byte* color )
{
	vrect_t	rcFill;

	rcFill.x = r->x;
	rcFill.y = r->y;
	rcFill.width = r->width;
	rcFill.height = 1;
	D_FillRect(&rcFill, color);			// top

	rcFill.y = r->y + r->height;
	D_FillRect(&rcFill, color);			// bottom

	rcFill.x = r->x;
	rcFill.y = r->y;
	rcFill.width = 1;
	rcFill.height = r->height;
	D_FillRect(&rcFill, color);			// left

	rcFill.x = r->x + r->width;
	D_FillRect(&rcFill, color);			// right
}

/*
==============
UI_LinkElement
==============
*/
void UI_LinkElement( uielement_t* element )
{
	uielement_t*	last;

	for (last = ui_elements; last; last = last->next)
	{
		if (!last->next)
			break;
	}

	if (last)
		last->next = element;
	else
		ui_elements = element;
}

/*
==============
UI_AddElement

Build an element around the caller's data and queue it up behind whatever is
already on the list.
==============
*/
void UI_AddElement( void* data, void* owner )
{
	uielement_t*	element;

	element = (uielement_t*)MnemoAllocDbg(sizeof(uielement_t), __FILE__, __LINE__);
	memset(element, 0, sizeof(uielement_t));

	element->flags |= UI_ELEM_ACTIVE;
	element->state = 0;
	element->data = data;
	element->owner = owner;
	element->next = NULL;

	UI_LinkElement(element);
}

/*
==============
UI_Activate

A page is coming up, so take the keyboard away from the game. The frame count
is reset so that nothing which waits on it fires while the page is opening.
==============
*/
void UI_Activate( void )
{
	ui_savedkeydest = key_dest;
	r_framecount = 0;
	key_dest = key_ui;
}

/*
==============
UI_Deactivate

The page is gone; give the keyboard back to whoever had it.
==============
*/
void UI_Deactivate( void )
{
	r_framecount = 0;
	key_dest = ui_savedkeydest;
}

/*
==============
UI_UnlinkElement

Take the oldest element off the list.
==============
*/
uielement_t* UI_UnlinkElement( void )
{
	uielement_t*	element;

	if (!ui_elements)
		return NULL;

	element = ui_elements;
	ui_elements = element->next;
	return element;
}

/*
==============
UI_FreeElement
==============
*/
void UI_FreeElement( uielement_t* element )
{
	free(element);
}

/*
==============
UI_KeyEvent

A key the front end asked for. The pages read the keyboard themselves, so
there is nothing left to do here.
==============
*/
void UI_KeyEvent( int key )
{
}

/*
==============
UI_Free
==============
*/
void UI_Free( void* buffer )
{
	free(buffer);
}

/*
==============
UI_Init
==============
*/
void UI_Init( void )
{
#if HLDC_MP
	UI_MultiplayerInit();
#endif
}

/*
==============
UI_Shutdown
==============
*/
void UI_Shutdown( void )
{
}

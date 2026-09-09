// ui.h -- the front-end user interface: the menu pages and the elements the
// engine puts up over them.

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

// Element flags
#define UI_ELEM_ACTIVE		2		// element is up and taking part in the layout

// One thing the front end is showing. Elements are kept on a single list in
// the order they were added.
typedef struct uielement_s
{
	int					flags;
	void*				data;
	void*				owner;
	int					state;
	struct uielement_s*	next;
} uielement_t;

// Frame the given rectangle with a one pixel border
void UI_DrawOutlineRect( vrect_t* r, byte* color );

// Element list: hook one on the end, build a new one, take the oldest back off
void UI_LinkElement( uielement_t* element );
void UI_AddElement( void* data, void* owner );
uielement_t* UI_UnlinkElement( void );
void UI_FreeElement( uielement_t* element );
void UI_Free( void* buffer );

// Hand the keyboard to the UI, and give it back to whoever had it
void UI_Activate( void );
void UI_Deactivate( void );

void UI_KeyEvent( int key );
#if HLDC_MP
void UI_MultiplayerInit( void );
qboolean UI_MultiplayerKeyEvent( int key );
qboolean UI_OpenChatKeyboard( void );
qboolean UI_ChatKeyboardActive( void );
qboolean UI_ChatKeyboardKeyEvent( int key );
void UI_DrawChatKeyboard( void );
#endif

void UI_Init( void );
void UI_Shutdown( void );

// Bring up a named menu page, and re-enable every item on the current one
void UI_OpenMenu( char* pszMenu );
void M_EnableAllItems( void );

// Draw the open menu, and run its input
void UI_Draw( void );
void UI_Update( void );

// Set while the attract-mode screen saver is running
extern int g_bScreenSaverActive;

// Rebuild the HUD when a new campaign starts from the menu
int HUD_Reset( void );

#ifdef __cplusplus
}
#endif

#endif // UI_H

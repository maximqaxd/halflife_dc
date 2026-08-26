// ui.h -- the front-end menus and the screen saver that sits behind them.

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

// Bring up a named menu page, and re-enable every item on the current one
void UI_OpenMenu( char* pszMenu );
void M_EnableAllItems( void );

// Called on screen-saver wakeup to resync menu/UI state
void M_DecodeStateFlags( void );

// Set while the attract-mode screen saver is running
extern int g_bScreenSaverActive;

// Rebuild the HUD when a new campaign starts from the menu
int HUD_Reset( void );

#ifdef __cplusplus
}
#endif

#endif // UI_H

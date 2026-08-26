// winquake.h: Win32-specific Quake header file

#pragma warning( disable : 4229 )  // mgraph gets this
#pragma warning( disable : 4996 )  // GetVersionExA

#include <windows.h>
#ifndef _WIN32_WCE
#include <direct.h>
#endif
#define CINTERFACE

#include <dsound.h>

extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;

extern DWORD gSndBufSize;
//#define SNDBUFSIZE 65536

extern HWND* pmainwindow;
extern qboolean	Win32AtLeastV4;
extern int gHasMMXTechnology;

DLL_EXPORT void IN_ShowMouse( void );
DLL_EXPORT void IN_DeactivateMouse( void );
DLL_EXPORT void IN_HideMouse( void );
DLL_EXPORT void IN_ActivateMouse( void );
void IN_SetQuakeMouseState( void );
DLL_EXPORT void IN_MouseEvent( int mstate );

#ifdef __cplusplus
extern "C" {
#endif
extern int		window_center_x, window_center_y;
#ifdef __cplusplus
}
#endif
extern RECT		window_rect;

DLL_EXPORT void IN_UpdateClipCursor( void );


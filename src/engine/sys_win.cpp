// sys_win.cpp -- process entry point and the message pump
//
// Brings up the display, hands the heap over to GameInit and then sits in the
// message loop, translating key presses for the engine and running one frame
// per pass until the window goes away.

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dll_state.h"

#include <windows.h>
#include <tchar.h>

extern "C" {
qboolean GameInit( void );
qboolean DCV_CreateWindow( void );
void     GDROM_ConfigureDoorBehavior( void );
int      Sys_Frame( float time, int forceRun );
void     Dispatch_Substate( int iSubState );
void     Key_Event( int key, qboolean down );
void     Sys_ExecCmd( int iState, char *fmt, ... );
void     Host_GetHostInfo( float *fps, int *nActive, int *nSpectators, int *nMaxPlayers, char *pszMap );

// Scan-code -> Quake key translation lives in in_dc.c.
int MapKey( int key );

extern HINSTANCE g_hInstance;
extern HINSTANCE g_hPrevInstance;
extern qboolean  host_initialized;
}

LPTSTR g_lpCmdLine;
int    g_nCmdShow;

static void *g_pStartupMem;

// Accumulate frame times and rebuild the fps/players status line twice a second.
static float g_flStatusTime;
static float g_flFrameAccum;
static int   g_nFrameCount;

void UpdateStatus( void )
{
	float now, fps;
	int   nActive, nSpectators, nMaxPlayers;
	char  szMap[32];
	char  szStatus[80];

	now = Sys_FloatTime();
	Host_GetHostInfo(&fps, &nActive, &nSpectators, &nMaxPlayers, szMap);

	g_flFrameAccum += fps;
	g_nFrameCount++;

	if (now - g_flStatusTime >= 0.5f)
	{
		g_flStatusTime = now;
		if (g_nFrameCount < 1)
			fps = 0;
		else
			fps = g_flFrameAccum / (float)g_nFrameCount;

		sprintf(szStatus, "%.1f fps %2i(%2i spec)/%2i on %16s", (double)fps, nActive, nSpectators, nMaxPlayers, szMap);

		g_flFrameAccum = 0;
		g_nFrameCount = 0;
	}
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	MSG      msg;
	qboolean quit = FALSE;
	int      key;

	host_initialized = FALSE;

	GDROM_ConfigureDoorBehavior();

	g_pStartupMem = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	g_hInstance     = hInstance;
	g_hPrevInstance = hPrevInstance;
	g_lpCmdLine     = lpCmdLine;
	g_nCmdShow      = nCmdShow;

	DCV_CreateWindow();

	if (!GameInit())
	{
		return -1;
	}

	Sys_ExecCmd(0, "menu splash\n");
	Dispatch_Substate(1);

	while (!quit)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
			{
				quit = TRUE;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_KEYDOWN)
			{
				key = MapKey(msg.wParam);
				if (key)
					Key_Event(key, 1);
			}
			else if (msg.message == WM_KEYUP)
			{
				key = MapKey(msg.wParam);
				if (key)
					Key_Event(key, 0);
			}
		}

		if (quit)
			break;

		Sys_Frame(0.0f, 0);
		UpdateStatus();
	}

	return 1;
}

qboolean g_bInStartup = FALSE;
qboolean g_bInactive  = FALSE;


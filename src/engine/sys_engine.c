// sys_engine.c -- Dreamcast bootstrap: WinMain, GameInit and the frame loop
//
// WinMain brings up the display, allocates the game heap in GameInit and starts
// the engine, then drives the frame loop through Sys_Frame -> Host_Frame.

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dll_state.h"

#include <windows.h>
#include <segagdrm.h>
#include <ceddcdrm.h>
#include <wdm.h>
#include <tchar.h>

// Shared engine globals defined in sys_dc.c.
extern HINSTANCE g_hInstance;
extern HINSTANCE g_hPrevInstance;
extern qboolean  isDedicated;
extern int       giActive;
extern int       giStateInfo;

extern qboolean DCV_CreateWindow( void );
extern void     GDROM_ConfigureDoorBehavior( void );
extern void     Sys_InitFloatTime( void );
extern int      Host_Frame( float time, int iState, int *stateInfo );
extern int      Sys_SampleCount( void );
extern void     Sys_NotifyState( int state );

// Heap chosen by GameInit.
static unsigned char *g_pEngineMem     = NULL;
static int            g_iEngineMemSize = 0;

// Sys_Frame state machine.
static int      g_engineState = DLL_INACTIVE;   // current DLL state
static float    g_lastFrameTime;                // timestamp of last frame
static int      g_engineStateInfo;              // Host_Frame stateInfo out
static float    g_frameSampleTime;              // accum minus sample cost
static float    g_minFrameTime;                 // min seconds per frame
static int      g_sampleBase;                   // sample-count baseline
static float    g_frameAccum;                   // accumulated frame time
static HMODULE  g_hGameDll;                      // loaded game DLL handle
static int      g_pauseCounter;                 // pause debounce counter
static int      g_pauseFlag;                    // pause pending flag
static int      g_closeFlag;                    // killserver-once guard

/*
==================
GameInit

 - queries total physical RAM and chooses a heap size (at least 8 MiB, otherwise (totalRam - ~1.7 MiB));
 - maps either physically-mapped memory at 0x8C800000 via MmMapIoSpace or allocates a heap block via the WinCE allocator;
 - if the mapping/allocation fails, returns FALSE;
 - runs Sys_InitFloatTime/low-level init, then mounts "/CD-ROM" as the base game directory and builds search paths using a DC helper.
==================
*/
qboolean GameInit( char* lpCmdLine )
{
	MEMORYSTATUS ms;
	DWORD        heapSize;
	DWORD        lastErr = 0;
	void        *pHeap = NULL;
	quakeparms_t parms;
	static char  basedir[] = "/CD-ROM";
	static char* argv[1] = { "" };

	memset(&ms, 0, sizeof(ms));
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatus(&ms);

	// Heap target from currently available RAM with Dreamcast headroom.
	if (ms.dwAvailPhys > 1700000)
		heapSize = ms.dwAvailPhys - 1700000;
	else
		heapSize = ms.dwAvailPhys;
	heapSize &= ~0xFFF; // page-align for VM APIs

	{
		TCHAR dbg[256];
		wsprintf(dbg, TEXT("GameInit: total=%lu avail=%lu trying heap=%lu\n"), ms.dwTotalPhys, ms.dwAvailPhys, heapSize);
		OutputDebugString(dbg);
	}
	if (ms.dwAvailPhys < 0x800000)
	{
		PHYSICAL_ADDRESS pa;
		pa.QuadPart = 0x8c800000;
		if (heapSize > ms.dwAvailPhys)
			heapSize = ms.dwAvailPhys & ~0xFFF;
		if (heapSize < 0x100000)
			heapSize = 0x100000;
		pHeap = MmMapIoSpace(pa, heapSize, TRUE);
		if (!pHeap)
			lastErr = GetLastError();
	}
	else
	{
		pHeap = VirtualAlloc(NULL, heapSize, MEM_COMMIT, PAGE_READWRITE);
		if (!pHeap)
			lastErr = GetLastError();
	}

	if (!pHeap)
	{
		Sys_Error("GameInit: failed to allocate heap (%lu bytes). total=%lu avail=%lu gle=0x%08lx",
		          heapSize, ms.dwTotalPhys, ms.dwAvailPhys, lastErr);
		return FALSE;
	}

	g_pEngineMem     = (unsigned char *)pHeap;
	g_iEngineMemSize = (int)heapSize;
	{
		TCHAR dbg[256];
		wsprintf(dbg, TEXT("GameInit: selected heap=%lu at %p\n"), heapSize, pHeap);
		OutputDebugString(dbg);
	}

	// Dedicated is not used on Dreamcast.
	isDedicated = FALSE;

	memset(&parms, 0, sizeof(parms));
	parms.membase = g_pEngineMem;
	parms.memsize = g_iEngineMemSize;
	parms.cachedir = NULL;

	parms.basedir = basedir;
	parms.argc = 1;
	parms.argv = argv;

	// Set up timer scale and start time, then bring up the engine.
	Sys_InitFloatTime();

	return Host_Init(&parms) ? TRUE : FALSE;
}

/*
==================
Sys_Frame

Rate-limits to g_minFrameTime with a Sys_FloatTime busy-wait, runs one Host_Frame,
then drives the DLL_STATE machine (pause/trans/close).
==================
*/
int Sys_Frame( float time, int forceRun )
{
	int   state = g_engineState;
	int   ret;
	float now;

	if (state != DLL_ACTIVE && forceRun == 0)
		return DLL_INACTIVE;

	if (state != DLL_INACTIVE)
	{
		now = Sys_FloatTime();
		time = now - g_lastFrameTime;
		while (time < g_minFrameTime)
		{
			now = Sys_FloatTime();
			time = now - g_lastFrameTime;
		}

		g_frameAccum += time;
		g_frameSampleTime = g_frameAccum - (float)(Sys_SampleCount() - g_sampleBase) / 10.0f;

		g_engineStateInfo = 0;
		ret = Host_Frame(time, g_engineState, &g_engineStateInfo);

		switch (g_engineStateInfo)
		{
		case STATE_TRAINING:
		case STATE_ENDLOGO:
			break;
		case DLL_QUIT:
			PostQuitMessage(0);
			FreeLibrary(g_hGameDll);
			g_hGameDll = NULL;
			g_engineState = DLL_INACTIVE;
			g_engineStateInfo = 0;
			break;
		}

		if (g_pauseCounter != 0)
		{
			g_pauseCounter--;
			if (ret == DLL_PAUSED)
			{
				g_pauseFlag = 1;
				g_engineState = DLL_ACTIVE;
				Sys_NotifyState(DLL_ACTIVE);
				ret = DLL_ACTIVE;
			}
			if (g_pauseCounter == 0 && g_pauseFlag != 0)
			{
				g_engineState = DLL_ACTIVE;
				g_pauseFlag = 0;
				ret = DLL_PAUSED;
			}
		}

		if (ret == DLL_TRANS)
		{
			g_pauseCounter = 5;
			ret = DLL_ACTIVE;
			g_engineState = DLL_ACTIVE;
			Sys_NotifyState(DLL_ACTIVE);
		}

		if (ret != g_engineState)
		{
			g_engineState = ret;
			Sys_NotifyState(ret);
		}

		g_lastFrameTime = now;
	}

	ret = g_engineState;
	if (g_engineState == DLL_CLOSE)
	{
		if (g_closeFlag == 0)
		{
			g_closeFlag = 1;
			Cbuf_AddText("killserver\n");
			Sys_Frame(time, 1);
			Sleep(100);
			Sys_Frame(time, 1);
			Sleep(100);
			ret = g_engineState;
		}
		else
		{
			PostQuitMessage(1);
			FreeLibrary(g_hGameDll);
			g_hGameDll = NULL;
			g_engineState = DLL_INACTIVE;
			g_engineStateInfo = 0;
			ret = g_engineState;
		}
	}
	return ret;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	double time, oldtime, newtime;
	host_initialized = FALSE;
	g_hInstance = hInstance;
	g_hPrevInstance = hPrevInstance;

	isDedicated = FALSE;
	GDROM_ConfigureDoorBehavior();
	Sys_InitFloatTime();

	if ( !DCV_CreateWindow() )
	{
		Sys_Error("DCV_CreateWindow failed");
		return -1;
	}

	if ( !GameInit( (char*)lpCmdLine ) )
	{
		Sys_Error("GameInit failed");
		return -1;
	}

	giActive = DLL_ACTIVE;
	oldtime = Sys_FloatTime();
	{
		int quit = 0;
		while (!quit)
		{
			do
			{
				newtime = Sys_FloatTime();
				time = newtime - oldtime;
			} while (time < 0.015f);

			oldtime = newtime;

			Sys_Frame((float)time, 1);
		}
	}
	Host_Shutdown();
	return 0;
}

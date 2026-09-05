// sys_engine.c -- the engine frame loop
//
// One pass of the engine: hold the frame rate down to the configured minimum,
// run a single Host_Frame, then act on whatever state the host came back with.
// Pausing, level transitions and shutdown all land here as DLL_STATE changes.

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dll_state.h"

#include <windows.h>

extern int  Host_Frame( float time, int iState, int *stateInfo );
extern int  Sys_SampleCount( void );
extern void GameSetState( int state );

int             giState = DLL_INACTIVE;    // current DLL state

static float    g_lastFrameTime;           // timestamp of last frame
static int      giStateInfo;               // Host_Frame stateInfo out
static float    g_frameSampleTime;         // accum minus sample cost
static float    g_minFrameTime;            // min seconds per frame
static int      g_sampleBase;              // sample-count baseline
static float    g_frameAccum;              // accumulated frame time
static HMODULE  g_hGameDll;                // loaded game DLL handle
static int      g_pauseCounter;            // pause debounce counter
static int      g_pauseFlag;               // pause pending flag
static int      g_closeFlag;               // killserver-once guard

/*
==================
Sys_Frame

Rate-limits to g_minFrameTime with a Sys_FloatTime busy-wait, runs one Host_Frame,
then drives the DLL_STATE machine (pause/trans/close).
==================
*/
int Sys_Frame( float time, int forceRun )
{
	int   state = giState;
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

		giStateInfo = 0;
		ret = Host_Frame(time, giState, &giStateInfo);

		switch (giStateInfo)
		{
		case STATE_TRAINING:
		case STATE_ENDLOGO:
			break;
		case DLL_QUIT:
			PostQuitMessage(0);
			FreeLibrary(g_hGameDll);
			g_hGameDll = NULL;
			giState = DLL_INACTIVE;
			giStateInfo = 0;
			break;
		}

		if (g_pauseCounter != 0)
		{
			g_pauseCounter--;
			if (ret == DLL_PAUSED)
			{
				g_pauseFlag = 1;
				giState = DLL_ACTIVE;
				GameSetState(DLL_ACTIVE);
				ret = DLL_ACTIVE;
			}
			if (g_pauseCounter == 0 && g_pauseFlag != 0)
			{
				giState = DLL_ACTIVE;
				g_pauseFlag = 0;
				ret = DLL_PAUSED;
			}
		}

		if (ret == DLL_TRANS)
		{
			g_pauseCounter = 5;
			ret = DLL_ACTIVE;
			giState = DLL_ACTIVE;
			GameSetState(DLL_ACTIVE);
		}

		if (ret != giState)
		{
			giState = ret;
			GameSetState(ret);
		}

		g_lastFrameTime = now;
	}

	ret = giState;
	if (giState == DLL_CLOSE)
	{
		if (g_closeFlag == 0)
		{
			g_closeFlag = 1;
			Cbuf_AddText("killserver\n");
			Sys_Frame(time, 1);
			Sleep(100);
			Sys_Frame(time, 1);
			Sleep(100);
			ret = giState;
		}
		else
		{
			PostQuitMessage(1);
			FreeLibrary(g_hGameDll);
			g_hGameDll = NULL;
			giState = DLL_INACTIVE;
			giStateInfo = 0;
			ret = giState;
		}
	}
	return ret;
}

// Forward the engine state to the game as well as our own frame loop.
void Sys_NotifyState( int iState )
{
	giState = iState;
	GameSetState(iState);
}

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
void     DCV_FB_BackgroundRect( unsigned short color );
void     DCV_FB_Text( const char *text );

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

typedef struct dc_exception_s
{
	int              reserved;
	int              outOfMemory;
	EXCEPTION_RECORD record;
	CONTEXT          context;
	const char      *name;
	DWORD            code;
	void            *address;
	void            *accessAddress;
	int              arenaPageInFailed;
	const char      *access;
} dc_exception_t;

dc_exception_t g_dcException;
char           g_dcExceptionText[2048];
char           g_dcExceptionScratch[32];

#ifdef _WIN32_WCE
void* operator new( unsigned int size )
{
	return MnemoAlloc(size, 0x20, 0, "op new");
}

void* operator new[]( unsigned int size )
{
	return MnemoAlloc(size, 0x20, 0, "op new[]");
}

void operator delete( void* ptr )
{
	MnemoFree(ptr);
}

void operator delete[]( void* ptr )
{
	MnemoFree(ptr);
}
#endif

HANDLE houtput;
int console_textlen;
char console_text[256];

void Sys_ConsoleOutput( char* text )
{
	char blank[256];
	DWORD written;

	if (console_textlen)
	{
		blank[0] = '\r';
		memset(blank + 1, ' ', console_textlen);
		blank[console_textlen + 1] = '\r';
		blank[console_textlen + 2] = 0;
		WriteFile(houtput, blank, console_textlen + 2, &written, NULL);
	}
	WriteFile(houtput, text, strlen(text), &written, NULL);
	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &written, NULL);
}

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

static LONG Sys_ExceptionFilter( EXCEPTION_POINTERS *exceptionInfo, DWORD exceptionCode )
{
	g_dcException.accessAddress     = NULL;
	g_dcException.name              = "Unknown";
	g_dcException.access            = "n/a";
	g_dcException.arenaPageInFailed = FALSE;
	g_dcException.code              = exceptionCode;

	g_dcException.record  = *exceptionInfo->ExceptionRecord;
	g_dcException.context = *exceptionInfo->ContextRecord;
	g_dcException.address = (void *)g_dcException.context.Fir;

	switch (g_dcException.code)
	{
	case STATUS_SUCCESS:
		g_dcException.name = "SUCCESS/WAIT_0";
		break;
	case STATUS_UNSUCCESSFUL:
		g_dcException.name = "UNSUCCESSFUL";
		break;
	case STATUS_ABANDONED_WAIT_0:
		g_dcException.name = "ABANDONED_WAIT_0";
		break;
	case STATUS_USER_APC:
		g_dcException.name = "USER_AP";
		break;
	case STATUS_TIMEOUT:
		g_dcException.name = "TIMEOUT";
		break;
	case STATUS_PENDING:
		g_dcException.name = "PENDING";
		break;
	case STATUS_GUARD_PAGE_VIOLATION:
		g_dcException.name = "GUARD_PAGE_VIOLATION ";
		break;
	case STATUS_DATATYPE_MISALIGNMENT:
		g_dcException.name = "DATATYPE_MISALIGNMENT";
		break;
	case STATUS_BREAKPOINT:
		g_dcException.name = "BREAKPOINT";
		break;
	case STATUS_SINGLE_STEP:
		g_dcException.name = "SINGLE_STEP";
		break;
	case STATUS_ACCESS_VIOLATION:
		g_dcException.name = "ACCESS_VIOLATION";
		g_dcException.access = g_dcException.record.ExceptionInformation[0] ? "write" : "read";
		g_dcException.accessAddress = (void *)g_dcException.record.ExceptionInformation[1];
		g_dcException.arenaPageInFailed = Mnemo_IsInArena(g_dcException.accessAddress);
		break;
	case STATUS_IN_PAGE_ERROR:
		g_dcException.name = "IN_PAGE_ERROR";
		break;
	case STATUS_INVALID_PARAMETER:
		g_dcException.name = "INVALID_PARAMETER";
		break;
	case STATUS_NO_MEMORY:
		g_dcException.name = "NO_MEMORY";
		break;
	case STATUS_INVALID_SYSTEM_SERVICE:
		g_dcException.name = "INVALID_SYSTEM_SERVICE";
		break;
	case STATUS_ILLEGAL_INSTRUCTION:
		g_dcException.name = "ILLEGAL_INSTRUCTION";
		break;
	case STATUS_NONCONTINUABLE_EXCEPTION:
		g_dcException.name = "NONCONTINUABLE_EXCEPTION";
		break;
	case STATUS_INVALID_DISPOSITION:
		g_dcException.name = "INVALID_DISPOSITION";
		break;
	case STATUS_ARRAY_BOUNDS_EXCEEDED:
		g_dcException.name = "ARRAY_BOUNDS_EXCEEDED";
		break;
	case STATUS_FLOAT_DENORMAL_OPERAND:
		g_dcException.name = "FLOAT_DENORMAL_OPERAND";
		break;
	case STATUS_FLOAT_DIVIDE_BY_ZERO:
		g_dcException.name = "FLOAT_DIVIDE_BY_ZERO";
		break;
	case STATUS_FLOAT_INEXACT_RESULT:
		g_dcException.name = "FLOAT_INEXACT_RESULT";
		break;
	case STATUS_FLOAT_INVALID_OPERATION:
		g_dcException.name = "FLOAT_INVALID_OPERATION";
		break;
	case STATUS_FLOAT_OVERFLOW:
		g_dcException.name = "FLOAT_OVERFLOW";
		break;
	case STATUS_FLOAT_STACK_CHECK:
		g_dcException.name = "FLOAT_STACK_CHECK";
		break;
	case STATUS_FLOAT_UNDERFLOW:
		g_dcException.name = "FLOAT_UNDERFLOW";
		break;
	case STATUS_INTEGER_DIVIDE_BY_ZERO:
		g_dcException.name = "INTEGER_DIVIDE_BY_ZERO";
		break;
	case STATUS_INTEGER_OVERFLOW:
		g_dcException.name = "INTEGER_OVERFLOW";
		break;
	case STATUS_PRIVILEGED_INSTRUCTION:
		g_dcException.name = "PRIVILEGED_INSTRUCTION";
		break;
	case STATUS_STACK_OVERFLOW:
		g_dcException.name = "STACK_OVERFLOW";
		break;
	case STATUS_USER_BREAK:
		g_dcException.name = "USER_BREAK";
		break;
	case STATUS_CONTROL_C_EXIT:
		g_dcException.name = "CONTROL_C_EXIT";
		break;
	case STATUS_INSTRUCTION_MISALIGNMENT:
		g_dcException.name = "INSTRUCTION_MISALIGNMENT";
		break;
	}

	if (g_dcException.arenaPageInFailed == TRUE &&
		VirtualAlloc(g_dcException.accessAddress, 0x20, MEM_COMMIT, PAGE_READWRITE))
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	MSG      msg;
	qboolean quit = FALSE;
	int      key;

	host_initialized = FALSE;

	// The door has not been touched yet, so the drive is not owed a reset
	g_gdDoorOpened = 0;
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

	__try
	{
		while (!quit)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if (!GetMessage(&msg, NULL, 0, 0))
				{
					quit = TRUE;
					break;
				}
#if HLDC_MP
				if (NET_DialMessage(msg.message, msg.wParam, msg.lParam))
					continue;
#endif
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
	}
	__except (Sys_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()))
	{
		DWORD *stack;
		DWORD  i;

		sprintf(g_dcExceptionText,
			"%s exception at %p\naccess: %s at %p\nparms: ",
			g_dcException.name, g_dcException.address,
			g_dcException.access, g_dcException.accessAddress);

		for (i = 0; i < g_dcException.record.NumberParameters; i++)
		{
			sprintf(g_dcExceptionScratch, "[%p] ", g_dcException.record.ExceptionInformation[i]);
			strcat(g_dcExceptionText, g_dcExceptionScratch);
		}

		stack = (DWORD *)g_dcException.context.R15;
		sprintf(g_dcExceptionScratch, "\nstack @ %p:", stack);
		strcat(g_dcExceptionText, g_dcExceptionScratch);

		for (i = 0; i < 8; i++)
		{
			if ((i & 3) == 0)
				strcat(g_dcExceptionText, "\n");

			sprintf(g_dcExceptionScratch, "%p ", *stack++);
			strcat(g_dcExceptionText, g_dcExceptionScratch);
		}

		sprintf(g_dcExceptionScratch, "\nPR: %p  GBR: %p  SR: %p\n",
			g_dcException.context.PR, g_dcException.context.GBR,
			g_dcException.context.Psr);
		strcat(g_dcExceptionText, g_dcExceptionScratch);

		sprintf(g_dcExceptionScratch, "DC Release Build %d\nDate: %s\n",
			build_number(), __DATE__);
		strcat(g_dcExceptionText, g_dcExceptionScratch);

		if (g_dcException.outOfMemory)
			strcat(g_dcExceptionText, "(probably out of memory)\n");

		if (g_dcException.arenaPageInFailed)
			strcat(g_dcExceptionText, "(Mnemo arena page in failed)\n");

		DCV_FB_BackgroundRect(0x7bef);
		DCV_FB_Text(g_dcExceptionText);

		VirtualFree(g_pStartupMem, 0, MEM_RELEASE);
		Mnemo_ReportToFile();

		for (;;)
		{
		}
	}

	return 1;
}

qboolean g_bInStartup = FALSE;
qboolean g_bInactive  = FALSE;

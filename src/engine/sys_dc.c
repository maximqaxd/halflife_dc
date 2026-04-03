#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dll_state.h"
#include "pr_cmds.h"
#include "pr_edict.h"
#include "r_studio.h"
#include "server.h"
#include "draw.h"
#include "dc_framebuffer.h"

#include <windows.h>
#ifdef _WIN32_WCE
#include <segagdrm.h>
#include <ceddcdrm.h>
#include <wdm.h>
#endif
#include <tchar.h>

HWND*				pmainwindow;
static HINSTANCE       g_hInstance     = NULL;
static HINSTANCE       g_hPrevInstance = NULL;
static HWND           g_hwndAppDC;
static LPDIRECTDRAW        g_pDD           = NULL;
static LPDIRECTDRAW4       g_pDD4          = NULL;
static LPDIRECTDRAWSURFACE4 g_pddsPrimary  = NULL;
static LPDIRECTDRAWSURFACE4 g_pddsBack     = NULL;
static LPDIRECT3D3         g_pD3D          = NULL;
static LPDIRECT3DDEVICE3   g_pD3DDevice    = NULL;
static LPDIRECT3DVIEWPORT3 g_pD3DViewport  = NULL;
static DDPIXELFORMAT       g_dc_backbuffer_fmt;

void* Sys_GetDirectDraw4( void )
{
	return (void*)g_pDD4;
}

void* Sys_GetBackBuffer4( void )
{
	return (void*)g_pddsBack;
}

void* Sys_GetPrimarySurface4( void )
{
	return (void*)g_pddsPrimary;
}

void* Sys_GetD3D3( void )
{
	return (void*)g_pD3D;
}

void* Sys_GetD3DDevice3( void )
{
	return (void*)g_pD3DDevice;
}

void* Sys_GetD3DViewport( void )
{
	return (void*)g_pD3DViewport;
}

void Sys_PresentFrame( void )
{
	HRESULT hr;

	if (!g_pddsPrimary)
		return;

	hr = g_pddsPrimary->lpVtbl->Flip(g_pddsPrimary, NULL, DDFLIP_WAIT);
	if (hr == DDERR_SURFACELOST)
	{
		g_pddsPrimary->lpVtbl->Restore(g_pddsPrimary);
		if (g_pddsBack)
			g_pddsBack->lpVtbl->Restore(g_pddsBack);
		hr = g_pddsPrimary->lpVtbl->Flip(g_pddsPrimary, NULL, DDFLIP_WAIT);
	}

	if (FAILED(hr) && g_pddsBack)
	{
#ifdef _WIN32_WCE
		hr = g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, NULL, g_pddsBack, NULL, DDBLT_WAIT, NULL);
#else
		{
			RECT rc;
			if (g_hwndAppDC && GetClientRect(g_hwndAppDC, &rc))
			{
				ClientToScreen(g_hwndAppDC, (POINT*)&rc.left);
				ClientToScreen(g_hwndAppDC, (POINT*)&rc.right);
				hr = g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, &rc, g_pddsBack, NULL, DDBLT_WAIT, NULL);
			}
			else
				hr = g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, NULL, g_pddsBack, NULL, DDBLT_WAIT, NULL);
		}
#endif
		if (hr == DDERR_SURFACELOST)
		{
			g_pddsPrimary->lpVtbl->Restore(g_pddsPrimary);
			g_pddsBack->lpVtbl->Restore(g_pddsBack);
#ifdef _WIN32_WCE
			g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, NULL, g_pddsBack, NULL, DDBLT_WAIT, NULL);
#else
			{
				RECT rcRestore;
				if (g_hwndAppDC && GetClientRect(g_hwndAppDC, &rcRestore))
				{
					ClientToScreen(g_hwndAppDC, (POINT*)&rcRestore.left);
					ClientToScreen(g_hwndAppDC, (POINT*)&rcRestore.right);
					g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, &rcRestore, g_pddsBack, NULL, DDBLT_WAIT, NULL);
				}
				else
					g_pddsPrimary->lpVtbl->Blt(g_pddsPrimary, NULL, g_pddsBack, NULL, DDBLT_WAIT, NULL);
			}
#endif
		}
	}
}

static unsigned char *g_pEngineMem     = NULL;
static int            g_iEngineMemSize = 0;

qboolean g_bInStartup = FALSE;
qboolean g_bInactive  = FALSE;
qboolean gfUseLANAuthentication = TRUE;
qboolean gfBackground = FALSE;
qboolean			isDedicated;
char				g_szProfileName[MAX_QPATH];
qboolean			g_bForceReloadOnCA_Active = FALSE;
qboolean			Win32AtLeastV4;
int					gHasMMXTechnology;
DLL_FUNCTIONS		gEntityInterface;

int giActive    = DLL_INACTIVE;
int giStateInfo = 1;
int giSubState  = 0;
extern cvar_t sys_ticrate;
// -----------------------------------------------------------------------------

/*
================
Sys_GetProfileRegKeyValue

Gets profile settings from the registry
================
*/
void Sys_GetProfileRegKeyValue( char* pszName, char* pszPath, char* pszSetting, char* pszElement, char* pszReturnString, int nReturnLength, char* pszDefaultValue )
{

}


void ExecuteProfileSettings( char* pszName )
{
	(void)pszName;
}


static DWORD g_dwYUV420StagingBufferSize   = 0x1000;
static DWORD g_dwCommandPolygonBufferSize  = 0x2ce20;
static DWORD g_dwCommandVertexBufferSize   = 0xdbba0;
static DWORD g_dwSmallestPolygon           = 0;



qboolean GameInit( char* lpCmdLine );
qboolean Sys_InitDisplayAndWindow( void );
void     GDROM_ConfigureDoorBehavior( void );
void     Sys_InitFloatTime( void );

#define MAX_HANDLES 10
static HANDLE sys_handles[MAX_HANDLES];

// -----------------------------------------------------------------------------
// ASYNC FILE IO 
// -----------------------------------------------------------------------------

#define MAX_ASYNC 16

struct sys_async_s
{
	HANDLE       hFile;
	unsigned int requested;
	unsigned int completed;
	unsigned int startTick;
	unsigned int error;
	OVERLAPPED   ov;
	HANDLE       hEvent;
};

typedef struct dc_async_slot_s
{
	int         handle;  // engine-level handle id (Sys_File* index)
	sys_async_t status;  // async status block 
} dc_async_slot_t;

static dc_async_slot_t g_AsyncSlots[MAX_ASYNC];

static dc_async_slot_t* DC_FindAsyncSlot( int handle )
{
	int i;

	for (i = 0; i < MAX_ASYNC; ++i)
	{
		if (g_AsyncSlots[i].handle == handle)
			return &g_AsyncSlots[i];
	}

	return NULL;
}

// DC internal open helper
// - normalizes '/' -> '\\'
// - builds a full path relative to the current CWD
// - forbids write/append opens on the GD filesystem (\CD-ROM\*)
// - returns a raw OS handle or INVALID_HANDLE_VALUE on failure
static HANDLE Sys_OpenHandle( const char *path, const char *mode, DWORD *outLength )
{
	char   szPath[MAX_PATH];
	char   szPathUpper[MAX_PATH];
	char  *p;
	DWORD  access = GENERIC_READ;
	DWORD  creation;
	DWORD  share = FILE_SHARE_READ;
	DWORD  attrs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
	HANDLE hFile;
	int    retryUpper = 0;

	if (!path || !mode)
		return INVALID_HANDLE_VALUE;

	// Guard: do not allow writes to the GD-ROM filesystem.
	if ((strchr(mode, 'w') || strchr(mode, 'a')) && strstr(path, "CD-ROM"))
	{
		Sys_Error("Attempted open-for-write on GD filesystem");
		return INVALID_HANDLE_VALUE;
	}

	memset(szPath, 0, sizeof(szPath));
	strncpy(szPath, path, sizeof(szPath) - 1);

	// Normalize slashes to the WinCE-style backslashes used in the DC EXE.
	for (p = szPath; *p; ++p)
	{
		if (*p == '/')
			*p = '\\';
	}

	// Normalize common CD root spellings to "\CD-ROM\..."
	if (!strncmp(szPath, "\\\\CD-ROM\\\\", 8))
	{
		// already fine
	}
	else if (!strncmp(szPath, "\\CD-ROM\\", 8))
	{
		// already fine
	}
	else if (!strncmp(szPath, "CD-ROM\\", 7))
	{
		// add leading backslash
		memmove(szPath + 1, szPath, strlen(szPath) + 1);
		szPath[0] = '\\';
	}
	else if (!strncmp(szPath, "\\CD-ROM", 7) && szPath[7] == 0)
	{
		// "\CD-ROM" -> "\CD-ROM\"
		strncat(szPath, "\\", sizeof(szPath) - strlen(szPath) - 1);
	}

	// Map stdio-style mode to Win32 access/creation flags.
	if (strchr(mode, 'w'))
	{
		access = GENERIC_WRITE;
		creation = CREATE_ALWAYS;
	}
	else if (strchr(mode, 'a'))
	{
		access = GENERIC_WRITE;
		creation = OPEN_ALWAYS;
	}
	else
	{
		access = GENERIC_READ;
		creation = OPEN_EXISTING;
	}

#ifdef _WIN32_WCE
	/* WinCE: path is narrow, convert to wide and call CreateFile (CreateFileW). */
	{
		TCHAR wszPath[MAX_PATH];

		wszPath[0] = 0;
		MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));

		hFile = CreateFile(wszPath, access, share, NULL, creation, attrs, NULL);
	}

	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (!strncmp(szPath, "\\CD-ROM\\", 8))
			retryUpper = 1;
		else if (strstr(szPath, "CD-ROM"))
			retryUpper = 1;

		if (retryUpper)
		{
			memset(szPathUpper, 0, sizeof(szPathUpper));
			strncpy(szPathUpper, szPath, sizeof(szPathUpper) - 1);

			for (p = szPathUpper + 8; *p; ++p)
			{
				if (*p >= 'a' && *p <= 'z')
					*p = (char)(*p - ('a' - 'A'));
			}

			{
				TCHAR wszPath[MAX_PATH];

				wszPath[0] = 0;
				MultiByteToWideChar(CP_ACP, 0, szPathUpper, -1, wszPath, ARRAYSIZE(wszPath));
				hFile = CreateFile(wszPath, access, share, NULL, creation, attrs, NULL);
			}
		}
	}
#else
	/* Win32 desktop: use narrow path with CreateFileA (no CD-ROM retry). */
	hFile = CreateFileA(szPath, access, share, NULL, creation, attrs, NULL);
#endif

	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (outLength)
			*outLength = (DWORD)-1;
		return INVALID_HANDLE_VALUE;
	}

	// If requested, report current file length (for read-only opens).
	if (outLength)
	{
		DWORD size = GetFileSize(hFile, NULL);
		*outLength = (size == INVALID_FILE_SIZE) ? (DWORD)-1 : size;
	}

	// For append mode, seek to the end.
	if (strchr(mode, 'a'))
	{
		SetFilePointer(hFile, 0, NULL, FILE_END);
	}

	return hFile;
}

static int findhandle( void )
{
	int i;

	for (i = 1; i < MAX_HANDLES; i++)
	{
		if (!sys_handles[i])
			return i;
	}

	Sys_Error("out of handles");
	return -1;
}

int Sys_FileOpenRead( char *path, int *hndl )
{
	DWORD  length = (DWORD)-1;
	HANDLE hFile;
	int    slot;

	slot = findhandle();
	hFile = Sys_OpenHandle(path, "rb", &length);
	if (hFile == INVALID_HANDLE_VALUE || length == (DWORD)-1)
	{
		*hndl = -1;
		return -1;
	}

	sys_handles[slot] = hFile;
	*hndl = slot;
	return (int)length;
}

FILE* Sys_FOpenReadSeek( const char* path, int offset )
{
	HANDLE h;
	DWORD  len;

	if (!path)
		return NULL;

	h = Sys_OpenHandle(path, "rb", &len);

	if (h == INVALID_HANDLE_VALUE)
		return NULL;

	SetFilePointer(h, offset, NULL, FILE_BEGIN);
	CloseHandle(h);

	return NULL; 
}

int Sys_FileOpenWrite( char *path )
{
	HANDLE hFile;
	int    slot;

	slot = findhandle();
	hFile = Sys_OpenHandle(path, "wb", NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Sys_Error("Error opening %s", path);
		return -1;
	}

	sys_handles[slot] = hFile;
	return slot;
}

void Sys_FileClose( int handle )
{
	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return;

	CloseHandle(sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek( int handle, int position )
{
	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return;

	SetFilePointer(sys_handles[handle], position, NULL, FILE_BEGIN);
}

int Sys_FileRead( int handle, void *dest, int count )
{
	DWORD bytesRead = 0;
	DWORD pos;
	OVERLAPPED ov;
	HANDLE hEvent;

	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return 0;
	if (!dest || count <= 0)
		return 0;

	pos = SetFilePointer(sys_handles[handle], 0, NULL, FILE_CURRENT);
	memset(&ov, 0, sizeof(ov));
	ov.Offset = pos;
	ov.OffsetHigh = 0;

	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ov.hEvent = hEvent;

	if (!ReadFile(sys_handles[handle], dest, (DWORD)count, &bytesRead, &ov))
	{
		DWORD gle = GetLastError();
		if (gle == ERROR_IO_PENDING || gle == ERROR_IO_INCOMPLETE)
		{
			if (!GetOverlappedResult(sys_handles[handle], &ov, &bytesRead, TRUE))
			{
				Sys_Error("Sys_FileRead: read failed, probably out of WinCE memory (pointer beyond current end?)\n");
				if (hEvent)
					CloseHandle(hEvent);
				return 0;
			}
		}
		else
		{
			Sys_Error("Sys_FileRead: read failed, probably out of WinCE memory (pointer beyond current end?)\n");
			if (hEvent)
				CloseHandle(hEvent);
			return 0;
		}
	}

	SetFilePointer(sys_handles[handle], pos + bytesRead, NULL, FILE_BEGIN);
	if (hEvent)
		CloseHandle(hEvent);
	return (int)bytesRead;
}

int Sys_FileWrite( int handle, void *data, int count )
{
	DWORD bytesWritten = 0;
	DWORD pos;
	OVERLAPPED ov;
	HANDLE hEvent;

	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return 0;

	pos = SetFilePointer(sys_handles[handle], 0, NULL, FILE_CURRENT);
	memset(&ov, 0, sizeof(ov));
	ov.Offset = pos;
	ov.OffsetHigh = 0;

	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ov.hEvent = hEvent;

	if (!WriteFile(sys_handles[handle], data, (DWORD)count, &bytesWritten, &ov))
	{
		DWORD gle = GetLastError();
		if (gle == ERROR_IO_PENDING || gle == ERROR_IO_INCOMPLETE)
		{
			if (!GetOverlappedResult(sys_handles[handle], &ov, &bytesWritten, TRUE))
			{
				if (hEvent)
					CloseHandle(hEvent);
				return 0;
			}
		}
		else
		{
			if (hEvent)
				CloseHandle(hEvent);
			return 0;
		}
	}

	SetFilePointer(sys_handles[handle], pos + bytesWritten, NULL, FILE_BEGIN);
	if (hEvent)
		CloseHandle(hEvent);
	return (int)bytesWritten;
}

int Sys_FileTime( char *path )
{
	DWORD  length = (DWORD)-1;
	HANDLE hFile;

	hFile = Sys_OpenHandle(path, "rb", &length);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;

	CloseHandle(hFile);
	return (length == (DWORD)-1) ? -1 : 1;
}

void Sys_mkdir( char *path )
{
	TCHAR wszPath[MAX_PATH];

	if (!path || !path[0])
		return;

	wszPath[0] = 0;
	MultiByteToWideChar(CP_ACP, 0, path, -1, wszPath, ARRAYSIZE(wszPath));
	CreateDirectory(wszPath, NULL);
}

int Sys_FileTell( int i )
{
	if (i <= 0 || i >= MAX_HANDLES || !sys_handles[i])
		return -1;

	return (int)SetFilePointer(sys_handles[i], 0, NULL, FILE_CURRENT);
}

// -----------------------------------------------------------------------------
// ASYNC HELPERS (Sys_RegisterAsync / Sys_AsyncBusy / Sys_FileReadAsync)
// -----------------------------------------------------------------------------

qboolean Sys_RegisterAsync( int handle, sys_async_t **ppAsync )
{
	dc_async_slot_t *slot;
	int              i;

	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return FALSE;

	// Check for double-register.
	slot = DC_FindAsyncSlot(handle);
	if (slot)
	{
		Sys_Error("Sys_RegisterAsync: handle already registered\n");
		return FALSE;
	}

	// Find a free slot.
	for (i = 0; i < MAX_ASYNC; ++i)
	{
		if (g_AsyncSlots[i].handle == 0)
		{
			if (g_AsyncSlots[i].status.hEvent)
			{
				CloseHandle(g_AsyncSlots[i].status.hEvent);
			}

			memset(&g_AsyncSlots[i].status, 0, sizeof(g_AsyncSlots[i].status));
			g_AsyncSlots[i].status.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			g_AsyncSlots[i].handle       = handle;
			g_AsyncSlots[i].status.hFile = sys_handles[handle];

			if (ppAsync)
				*ppAsync = &g_AsyncSlots[i].status;

			return TRUE;
		}
	}

	Sys_Error("Sys_RegisterAsync: too many async handles\n");
	return FALSE;
}

qboolean Sys_AsyncBusy( int handle, sys_async_t *pStatus )
{
	dc_async_slot_t *slot = DC_FindAsyncSlot(handle);
	sys_async_t     *st;
	DWORD            bytes;

	if (!slot)
	{
		Sys_Error("Sys_AsyncBusy on unregistered sync handle\n");
		return FALSE;
	}

	st = pStatus ? pStatus : &slot->status;

	if (!st->requested || !st->hFile || !st->hEvent)
		return FALSE;

	if (!GetOverlappedResult(st->hFile, &st->ov, &bytes, FALSE))
	{
		DWORD err = GetLastError();

		if (err == ERROR_IO_INCOMPLETE)
		{
			return TRUE;
		}

		st->error     = err;
		st->completed = 0;
		st->requested = 0;
		return FALSE;
	}

	st->completed = bytes;
	st->requested = 0;
	return FALSE;
}

int Sys_FileReadAsync( int handle, void *buffer, int count, sys_async_t *pStatus )
{
	dc_async_slot_t *slot = DC_FindAsyncSlot(handle);
	sys_async_t     *st;
	DWORD            bytesRead = 0;

	if (handle <= 0 || handle >= MAX_HANDLES || !sys_handles[handle])
		return 0;

	if (!slot)
	{
		Sys_Error("Sys_FileReadAsync on unregistered sync handle\n");
		return 0;
	}

	st = pStatus ? pStatus : &slot->status;

	st->hFile     = sys_handles[handle];
	st->requested = (unsigned int)count;
	st->completed = 0;
	st->error     = 0;
	st->startTick = GetTickCount();
	memset(&st->ov, 0, sizeof(st->ov));
	st->ov.hEvent = st->hEvent;

	if (!ReadFile(sys_handles[handle], buffer, (DWORD)count, &bytesRead, &st->ov))
	{
		DWORD err = GetLastError();

		if (err != ERROR_IO_PENDING)
		{
			st->error     = err;
			st->completed = 0;
			st->requested = 0;
			return 0;
		}

		// I/O is pending; Sys_AsyncBusy will observe completion later.
		return count;
	}

	// Completed synchronously; update status now.
	st->completed = bytesRead;
	st->requested = 0;
	return count;
}

// -----------------------------------------------------------------------------
// MEMORY PROTECTION
// -----------------------------------------------------------------------------

void Sys_MakeCodeWriteable( unsigned long startaddr, unsigned long length )
{
	DWORD flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
		Sys_Error("Protection change failed");
}

// -----------------------------------------------------------------------------
// ERROR/PRINT/QUIT
// -----------------------------------------------------------------------------


void Sys_Error( char *error, ... )
{
	va_list argptr;
	char    text[1024];
	TCHAR   wtext[1024];
	unsigned short wFatalBarColor;

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

#ifdef _WIN32_WCE
	MultiByteToWideChar( CP_ACP, 0, text, -1, wtext, ARRAYSIZE( wtext ) );
	OutputDebugString( wtext );
#else
	OutputDebugString(text);
#endif

	wFatalBarColor = 0xF800;
	DCV_FB_BackgroundRect(0x18, wFatalBarColor);
	DCV_FB_Text(8, 0x18, text);
	Sys_PresentFrame();
	giActive = DLL_INACTIVE;
	for (;;)
	{
		
	}
}

void Sys_Warning( char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	Con_Printf("WARNING: %s\n", text);
	giActive = DLL_PAUSED;
}

void Sys_Printf( char *fmt, ... )
{
	va_list argptr;
	char    text[1024];
	TCHAR   wtext[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

#ifdef _WIN32_WCE
	MultiByteToWideChar( CP_ACP, 0, text, -1, wtext, ARRAYSIZE( wtext ) );
	OutputDebugString( wtext );
#else
	OutputDebugString(text);
#endif
}

void Sys_Quit( void )
{
	Host_Shutdown();
	giActive = DLL_CLOSE;
	longjmp(host_abortserver, 1);
}

// -----------------------------------------------------------------------------
// TIMING 
// -----------------------------------------------------------------------------

static double g_pfreq       = 0.0;
static double g_curtime     = 0.0;
static double g_lastcurtime = 0.0;
static int    g_lowshift    = 0;

void Sys_Init( void )
{
	LARGE_INTEGER perfFreq;
	unsigned int  lowpart, highpart;

	if (!QueryPerformanceFrequency(&perfFreq))
		Sys_Error("No hardware timer available");

	lowpart = (unsigned int)perfFreq.LowPart;
	highpart = (unsigned int)perfFreq.HighPart;
	g_lowshift = 0;

	while (highpart || (lowpart > 2000000u))
	{
		g_lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	g_pfreq = 1.0 / (double)lowpart;
	Sys_InitFloatTime();
}

DLL_EXPORT double Sys_FloatTime( void )
{
	static int          sametimecount;
	static unsigned int oldtime;
	static int          first = 1;
	LARGE_INTEGER       perfCount;
	unsigned int        temp, t2;
	double              time;

	QueryPerformanceCounter(&perfCount);

	temp = ((unsigned int)perfCount.LowPart >> g_lowshift) |
	       ((unsigned int)perfCount.HighPart << (32 - g_lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;
		}
		else
		{
			t2 = temp - oldtime;
			time = (double)t2 * g_pfreq;
			oldtime = temp;

			g_curtime += time;

			if (g_curtime == g_lastcurtime)
			{
				sametimecount++;
				if (sametimecount > 100000)
				{
					g_curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			g_lastcurtime = g_curtime;
		}
	}

	return g_curtime;
}

void Sys_InitFloatTime( void )
{
	int j;

	Sys_FloatTime();

	j = COM_CheckParm("-starttime");
	if (j)
		g_curtime = (double)(Q_atof(com_argv[j + 1]));
	else
		g_curtime = 0.0;

	g_lastcurtime = g_curtime;
}

void Sys_Sleep( void )
{
	Sleep(1);
}

void Sys_SendKeyEvents( void )
{
	MSG msg;

	if (g_bInStartup)
		return;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		scr_skipupdate = 0;
		if (g_bInactive)
			break;

		if (!GetMessage(&msg, NULL, 0, 0))
			Sys_Quit();

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (!g_bInactive)
		return;

}

// -----------------------------------------------------------------------------
// Engine/DLL interface helpers (static-link path for DC)
//
// Win32 uses LoadLibrary/GetProcAddress and BuildExportTable; DC uses a
// single "virtual" extension with a static name->address table and no
// LoadLibrary.
// -----------------------------------------------------------------------------

extern globalvars_t gGlobalVariables;
extern int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion );
extern void DLLEXPORT GiveFnptrsToDll( enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals );

typedef struct hl_link_export_s
{
	const char *name;
	void *func;
} hl_link_export_t;
extern hl_link_export_t lib_hl_exports[];

static enginefuncs_t g_engfuncsExportedToDlls =
{
	PF_precache_model_I,
	PF_precache_sound_I,
	PF_setmodel_I,
	PF_modelindex,
	ModelFrames,
	PF_setsize_I,
	PF_changelevel_I,
	PF_setspawnparms_I,
	SaveSpawnParms,
	PF_vectoyaw_I,
	PF_vectoangles_I,
	SV_MoveToOrigin_I,
	PF_changeyaw_I,
	PF_changepitch_I,
	FindEntityByString,
	GetEntityIllum,
	FindEntityInSphere,
	PF_checkclient_I,
	PVSFindEntities,
	PF_makevectors_I,
	AngleVectors,
	PF_Spawn_I,
	PF_Remove_I,
	CreateNamedEntity,
	PF_makestatic_I,
	PF_checkbottom_I,
	PF_droptofloor_I,
	PF_walkmove_I,
	PF_setorigin_I,
	PF_sound_I,
	PF_ambientsound_I,
	PF_traceline_DLL,
	PF_TraceToss_DLL,
	TraceMonsterHull,
	TraceHull,
	TraceModel,
	TraceTexture,
	TraceSphere,
	PF_aim_I,
	PF_localcmd_I,
	PF_stuffcmd_I,
	PF_particle_I,
	PF_lightstyle_I,
	PF_DecalIndex,
	PF_pointcontents_I,
	PF_MessageBegin_I,
	PF_MessageEnd_I,
	PF_WriteByte_I,
	PF_WriteChar_I,
	PF_WriteShort_I,
	PF_WriteLong_I,
	PF_WriteAngle_I,
	PF_WriteCoord_I,
	PF_WriteString_I,
	PF_WriteEntity_I,
	CVarGetFloat,
	CVarGetString,
	CVarSetFloat,
	CVarSetString,
	AlertMessage,
	EngineFprintf,
	PvAllocEntPrivateData,
	PvEntPrivateData,
	FreeEntPrivateData,
	SzFromIndex,
	AllocEngineString,
	GetVarsOfEnt,
	PEntityOfEntOffset,
	EntOffsetOfPEntity,
	IndexOfEdict,
	PEntityOfEntIndex,
	FindEntityByVars,
	GetModelPtr,
	RegUserMsg,
	AnimationAutomove,
	GetBonePosition,
	FunctionFromName,
	NameForFunction,
	ClientPrintf,
	Cmd_Args,
	Cmd_Argv,
	Cmd_Argc,
	GetAttachment,
	CRC32_Init,
	CRC32_ProcessBuffer,
	CRC32_ProcessByte,
	CRC32_Final,
	RandomLong,
	RandomFloat,
	PF_setview_I,
	PF_Time,
	PF_crosshairangle_I,
	COM_LoadFileForMe,
	COM_FreeFile,
	Host_EndSection,
	COM_CompareFileTime,
	COM_GetGameDir,
	Cvar_RegisterVariable,
	PF_FadeVolume,
	PF_SetClientMaxspeed,
	PF_CreateFakeClient_I,
	PF_RunPlayerMove_I,
	PF_NumberOfEntities_I,
	PF_IsMapValid_I
};

extensiondll_t g_rgextdll[MAX_EXT_DLLS];
int g_iextdllMac;

static functiontable_t *static_game_exports;
static int static_game_export_count;

static qboolean AddStaticExport( extensiondll_t *pextdll, const char *pName, uint32 function );

static qboolean AddStaticExport( extensiondll_t *pextdll, const char *pName, uint32 function )
{
	functiontable_t *pNewTable;
	char *pNameCopy;
	int i;

	if (!pName || !function)
		return FALSE;

	for (i = 0; i < pextdll->functionCount; i++)
	{
		if (!strcmp(pextdll->functionTable[i].pFunctionName, pName))
			return TRUE;
	}

	pNewTable = (functiontable_t *)realloc(pextdll->functionTable,
	                                       sizeof(functiontable_t) * (pextdll->functionCount + 1));
	if (!pNewTable)
		return FALSE;
	pextdll->functionTable = pNewTable;

	pNameCopy = (char *)malloc(strlen(pName) + 1);
	if (!pNameCopy)
		return FALSE;
	strcpy(pNameCopy, pName);

	pextdll->functionTable[pextdll->functionCount].pFunctionName = pNameCopy;
	pextdll->functionTable[pextdll->functionCount].pFunction = function;
	pextdll->functionCount++;
	return TRUE;
}

static void FreeStaticExportTable( extensiondll_t *pextdll )
{
	int i;

	if (!pextdll->functionTable)
		return;

	for (i = 0; i < pextdll->functionCount; i++)
	{
		if (pextdll->functionTable[i].pFunctionName)
			free(pextdll->functionTable[i].pFunctionName);
	}

	free(pextdll->functionTable);
	pextdll->functionTable = NULL;
	pextdll->functionCount = 0;
}

static qboolean BuildStaticExportTable( extensiondll_t *pextdll )
{
	int i;

	pextdll->lDLLHandle = NULL;
	pextdll->functionTable = NULL;
	pextdll->functionCount = 0;

	if (!AddStaticExport(pextdll, "GiveFnptrsToDll", (uint32)(size_t)GiveFnptrsToDll))
		return FALSE;
	if (!AddStaticExport(pextdll, "GetEntityAPI", (uint32)(size_t)GetEntityAPI))
		return FALSE;

	/* Populate class/entity dispatch directly from halflife/link_helper.cpp. */
	for (i = 0; lib_hl_exports[i].name; i++)
	{
		if (!AddStaticExport(pextdll, lib_hl_exports[i].name, (uint32)(size_t)lib_hl_exports[i].func))
			return FALSE;
	}

	static_game_exports = pextdll->functionTable;
	static_game_export_count = pextdll->functionCount;
	Con_Printf("Static export table: %i entries (GiveFnptrsToDll + GetEntityAPI + %i from link_helper)\n",
	           pextdll->functionCount,
	           pextdll->functionCount > 2 ? pextdll->functionCount - 2 : 0); // TODO - remove that 
	return TRUE;
}

static char *FindAddressInTable( extensiondll_t *pDll, uint32 function )
{
	int i;

	for (i = 0; i < pDll->functionCount; i++)
	{
		if (pDll->functionTable[i].pFunction == function)
			return pDll->functionTable[i].pFunctionName;
	}
	return NULL;
}

static uint32 FindNameInTable( extensiondll_t *pDll, char *pName )
{
	int i;

	for (i = 0; i < pDll->functionCount; i++)
	{
		if (!strcmp(pName, pDll->functionTable[i].pFunctionName))
			return pDll->functionTable[i].pFunction;
	}
	return 0;
}

DISPATCHFUNCTION GetDispatch( char *pname )
{
	int i;
	uint32 fn;

	for (i = 0; i < g_iextdllMac; i++)
	{
		fn = FindNameInTable(&g_rgextdll[i], pname);
		if (fn)
			return (DISPATCHFUNCTION)(size_t)fn;
	}
	return NULL;
}

ENTITYINIT GetEntityInit( char *pClassName )
{
	return (ENTITYINIT)GetDispatch(pClassName);
}

FIELDIOFUNCTION GetIOFunction( char *pName )
{
	return (FIELDIOFUNCTION)GetDispatch(pName);
}

uint32 FunctionFromName( char *pName )
{
	int i;
	uint32 function;

	for (i = 0; i < g_iextdllMac; i++)
	{
		function = FindNameInTable(&g_rgextdll[i], pName);
		if (function)
			return function;
	}
	Con_Printf("Can't find proc: %s\n", pName);
	return 0;
}

char *NameForFunction( uint32 function )
{
	int i;
	char *pName;

	for (i = 0; i < g_iextdllMac; i++)
	{
		pName = FindAddressInTable(&g_rgextdll[i], function);
		if (pName)
			return pName;
	}
	Con_Printf("Can't find address: %08lx\n", (unsigned long)function);
	return NULL;
}

void LoadEntityDLLs( char *szBaseDir )
{
	typedef void (DLLEXPORT *PFN_GiveFnptrsToDll)(enginefuncs_t *, globalvars_t *);
	PFN_GiveFnptrsToDll pfnGiveFnptrsToDll;
	APIFUNCTION pfnGetAPI;
	int interface_version;

	(void)szBaseDir;

	g_iextdllMac = 0;
	memset(g_rgextdll, 0, sizeof(g_rgextdll));

	if (!BuildStaticExportTable(&g_rgextdll[0]))
		Sys_Error("Unable to build static export table");

	g_iextdllMac = 1;

	pfnGiveFnptrsToDll = (PFN_GiveFnptrsToDll)GetDispatch("GiveFnptrsToDll");

	if (!pfnGiveFnptrsToDll)
		Sys_Error("Can't get GiveFnptrsToDll!");

	pfnGiveFnptrsToDll(&g_engfuncsExportedToDlls, &gGlobalVariables);

	pfnGetAPI = (APIFUNCTION)GetDispatch("GetEntityAPI");

	if (!pfnGetAPI)
		Sys_Error("Can't get DLL API!");

	interface_version = INTERFACE_VERSION;
	if (!pfnGetAPI(&gEntityInterface, interface_version))
		Sys_Error("Invalid DLL version!");

	Con_Printf("----------------------\n");
	Con_Printf("Dlls loaded for game:\n%s\n", gEntityInterface.pfnGetGameDescription());
	Con_Printf("----------------------\n");
}

void LoadThisDll( char *szDllFilename )
{
	(void)szDllFilename;
	/* DC does not load DLLs from disk. */
}

void ReleaseEntityDlls( void )
{
	if (g_iextdllMac > 0)
		FreeStaticExportTable(&g_rgextdll[0]);
	static_game_exports = NULL;
	static_game_export_count = 0;
	g_iextdllMac = 0;
	memset(g_rgextdll, 0, sizeof(g_rgextdll));
}

void EngineFprintf( void *pFile, char *szFmt, ... )
{
	va_list argptr;

	va_start(argptr, szFmt);
	vfprintf((FILE *)pFile, szFmt, argptr);
	va_end(argptr);
}

void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list argptr;
	char    szOut[1024];

	if (!developer.value)
		return;

	va_start(argptr, szFmt);
	vsprintf(szOut, szFmt, argptr);
	va_end(argptr);

	switch (atype)
	{
	case at_notice:
		Con_Printf("NOTE:  %s", szOut);
		break;
	case at_console:
		Con_Printf("%s", szOut);
		break;
	case at_aiconsole:
		if (developer.value < 2)
			return;
		Con_Printf("%s", szOut);
		break;
	case at_warning:
		Con_Printf("WARNING:  %s", szOut);
		break;
	case at_error:
		Con_Printf("ERROR:  %s", szOut);
		break;
	}
}


/*
==================
GDROM_ConfigureDoorBehavior

Sys_Init‑time GD‑ROM setup:
 - opens "\\Device\\CDROM0"
 - issues IOCTL to configure door / media behaviour
 - on failure, calls Sys_Error with the GD‑ROM error string.
==================
*/
#ifdef _WIN32_WCE
void GDROM_ConfigureDoorBehavior( void )
{
	HANDLE              hGDROM;
	SEGACD_DOOR_BEHAVIOR doorbehavior;
	DWORD               dwReturned;

	// Create a handle to the GD‑ROM drive
	hGDROM = CreateFile(TEXT("\\Device\\CDROM0"),
	                    GENERIC_READ,
	                    0,
	                    NULL,
	                    OPEN_EXISTING,
	                    0,
	                    NULL);
	if (hGDROM == INVALID_HANDLE_VALUE)
	{
		Sys_Error("Error opening GD‑ROM");
		return;
	}

	// Request "notify app" behaviour instead of reboot on door open.
	doorbehavior.dwBehavior = SEGACD_DOOR_NOTIFY_APP;
	if (!DeviceIoControl(hGDROM,
	                     IOCTL_SEGACD_SET_DOOR_BEHAVIOR,
	                     &doorbehavior,
	                     sizeof(doorbehavior),
	                     NULL,
	                     0,
	                     &dwReturned,
	                     NULL))
	{
		DWORD nError = GetLastError();

		if (nError == ERROR_NO_MEDIA_IN_DRIVE)
		{
			Sys_Error("There is no media in the GD‑ROM drive.  Please insert the Half‑Life disc and restart.");
		}
		else
		{
			Sys_Error("Error setting GD‑ROM door behavior (0x%08x).", nError);
		}
	}

	CloseHandle(hGDROM);
}
#endif

/*
==================
GameInit

 - queries total physical RAM and chooses a heap size (at least 8 MiB, otherwise (totalRam - ~1.7 MiB));
 - maps either physically‑mapped memory at 0x8C800000 via MmMapIoSpace or allocates a heap block via the WinCE allocator;
 - if the mapping/allocation fails, returns FALSE;
 - runs Sys_InitFloatTime/low‑level init, then mounts "/CD-ROM" as the base game directory and builds search paths using a DC helper.
==================
*/
qboolean GameInit( char* lpCmdLine )
{
	MEMORYSTATUS ms;
	DWORD        heapSize;
	DWORD        lastErr = 0;
	void        *pHeap = NULL;
	quakeparms_t parms;
#ifdef _WIN32_WCE
	static char  basedir[] = "/CD-ROM";
	static char* argv[1] = { "" };
#else
	static char  cwd[MAX_OSPATH];
	static char  cmdline[256];
	static char* argv[MAX_NUM_ARGVS];
#endif

	memset(&ms, 0, sizeof(ms));
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatus(&ms);

	// Heap target from currently available RAM with Dreamcast headroom.
	if (ms.dwAvailPhys > 1700000)
		heapSize = ms.dwAvailPhys - 1700000;
	else
		heapSize = ms.dwAvailPhys;
#ifndef _WIN32_WCE
	/* Win32: cap heap to ~8.7 MB to simulate Dreamcast target. */
	{
		const DWORD dc_heap_cap = (DWORD)(8.7 * 1024.0 * 1024.0);
		if (heapSize > dc_heap_cap)
			heapSize = dc_heap_cap;
	}
#endif
	heapSize &= ~0xFFF; // page-align for VM APIs

	{
		TCHAR dbg[256];
		wsprintf(dbg, TEXT("GameInit: total=%lu avail=%lu trying heap=%lu\n"), ms.dwTotalPhys, ms.dwAvailPhys, heapSize);
		OutputDebugString(dbg);
	}
#ifdef _WIN32_WCE
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
#endif
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

#ifdef _WIN32_WCE
	parms.basedir = basedir;
	parms.argc = 1;
	parms.argv = argv;
#else
	if (!GetCurrentDirectory(sizeof(cwd), cwd))
		Sys_Error("Couldn't determine current directory");
	{
		size_t len = strlen(cwd);
		if (len > 0 && (cwd[len - 1] == '/' || cwd[len - 1] == '\\'))
			cwd[len - 1] = '\0';
	}
	parms.basedir = cwd;

	argv[0] = "";
	parms.argc = 1;
	if (lpCmdLine)
	{
		char* p;
		strncpy(cmdline, lpCmdLine, sizeof(cmdline) - 1);
		cmdline[sizeof(cmdline) - 1] = '\0';
		p = cmdline;
		while (*p && parms.argc < MAX_NUM_ARGVS)
		{
			while (*p && ((*p <= 32) || (*p > 126)))
				p++;
			if (*p)
			{
				argv[parms.argc++] = p;
				while (*p && (*p > 32) && (*p <= 126))
					p++;
				if (*p)
				{
					*p = '\0';
					p++;
				}
			}
		}
	}
	parms.argv = argv;
	COM_InitArgv(parms.argc, parms.argv);
	parms.argc = com_argc;
	parms.argv = com_argv;
#endif

	// Set up timer scale and start time, then bring up the engine.
	Sys_Init();

	return Host_Init(&parms) ? TRUE : FALSE;
}

/*
==================
DCV_InitDirect3D

Dreamcast display / window / PVR bootstrap
 - reads HKLM\\...\\DisplaySettings values like
   YUV420StagingBufferSize, CommandPolygonBufferSize,
   CommandVertexBufferSize, SmallestPolygon;
 - registers a WinCE window class "Halflife" with a custom WndProc;
 - creates the main 640x480 window (CreateWindowEx‑style call);
 - creates a DirectDraw object, sets cooperative level to that window and
   switches the display to 640x480x16;
 - creates and configures the initial PVR / D3D device, viewport, and a
   small set of fixed‑function state blocks and vertex buffers;
==================
*/
#ifndef _WIN32_WCE
static LRESULT CALLBACK MainWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

qboolean Sys_InitDisplay( void )
{
	WNDCLASS cls;
	HKEY     hKey = NULL;
	DWORD    dwType;
	DWORD    dwSize;

	// Cache WinMain-style globals if we haven't been given them by a launcher.
	if (!g_hInstance)
		g_hInstance = GetModuleHandle(NULL);

	// Register a simple window class for the main application window, if needed.
	if (!g_hPrevInstance)
	{
		memset(&cls, 0, sizeof(cls));
		cls.hCursor       = NULL;
		cls.hIcon         = NULL;
		cls.lpszMenuName  = NULL;
		cls.hbrBackground = NULL;
		cls.hInstance     = (HINSTANCE)g_hInstance;
		cls.lpszClassName = TEXT("Halflife");
#ifdef _WIN32_WCE
		cls.lpfnWndProc   = (WNDPROC)DefWindowProc;
#else
		cls.lpfnWndProc   = MainWndProc;
#endif
		cls.style         = 0;
		cls.cbWndExtra    = 0;
		cls.cbClsExtra    = 0;

		if (!RegisterClass(&cls))
		{
			// WinCE will return ERROR_CLASS_ALREADY_EXISTS if someone registered it first.
			if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
				return FALSE;
		}
	}

#ifdef _WIN32_WCE
	g_hwndAppDC = CreateWindowEx(0,
	                             TEXT("Halflife"),
	                             TEXT("Halflife"),
	                             WS_VISIBLE,
	                             0, 0,
	                             640, 480,
	                             NULL,
	                             NULL,
	                             (HINSTANCE)g_hInstance,
	                             NULL);
#else
	{
		RECT rc = { 0, 0, 640, 480 };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		g_hwndAppDC = CreateWindowEx(0,
		                             TEXT("Halflife"),
		                             TEXT("Half-Life"),
		                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		                             CW_USEDEFAULT, CW_USEDEFAULT,
		                             rc.right - rc.left, rc.bottom - rc.top,
		                             NULL,
		                             NULL,
		                             (HINSTANCE)g_hInstance,
		                             NULL);
	}
#endif
	if (!g_hwndAppDC)
	{
		return FALSE;
	}

	/* Set PVR/driver tuning parameters in HKLM\DisplaySettings. */
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("DisplaySettings"), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dwSize = sizeof(DWORD);
		dwType = 0x1000;
		RegSetValueEx(hKey, TEXT("YUV420StagingBufferSize"), 0, REG_DWORD, (LPBYTE)&dwType, dwSize);
		dwType = 0x2ce20;
		RegSetValueEx(hKey, TEXT("CommandPolygonBufferSize"), 0, REG_DWORD, (LPBYTE)&dwType, dwSize);
		dwType = 0xdbba0;
		RegSetValueEx(hKey, TEXT("CommandVertexBufferSize"), 0, REG_DWORD, (LPBYTE)&dwType, dwSize);
		dwType = 0;
		RegSetValueEx(hKey, TEXT("SmallestPolygon"), 0, REG_DWORD, (LPBYTE)&dwType, dwSize);
		RegCloseKey(hKey);
	}

	// Initialize DirectDraw and switch to 640x480x16.
	if (FAILED(DirectDrawCreate(NULL, &g_pDD, NULL)))
		return FALSE;

	// Get an IDirectDraw4 interface.
	if (FAILED(g_pDD->lpVtbl->QueryInterface(g_pDD, &IID_IDirectDraw4, (LPVOID*)&g_pDD4)))
	{
		g_pDD->lpVtbl->Release(g_pDD);
		g_pDD = NULL;
		return FALSE;
	}

#ifdef _WIN32_WCE
	// Set cooperative level to full-screen exclusive on our main window.
	if (FAILED(g_pDD4->lpVtbl->SetCooperativeLevel(g_pDD4, g_hwndAppDC, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE)))
	{
		g_pDD4->lpVtbl->Release(g_pDD4);
		g_pDD4 = NULL;
		g_pDD->lpVtbl->Release(g_pDD);
		g_pDD = NULL;
		return FALSE;
	}

	// Switch to 640x480x16 display mode.
	if (FAILED(g_pDD4->lpVtbl->SetDisplayMode(g_pDD4, 640, 480, 16, 0, 0)))
	{
		g_pDD4->lpVtbl->Release(g_pDD4);
		g_pDD4 = NULL;
		g_pDD->lpVtbl->Release(g_pDD);
		g_pDD = NULL;
		return FALSE;
	}

	// Create primary surface with 1 backbuffer (flip chain).
	{
		DDSURFACEDESC2 ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount = 1;

		if (FAILED(g_pDD4->lpVtbl->CreateSurface(g_pDD4, &ddsd, &g_pddsPrimary, NULL)))
			return FALSE;

		ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

		if (FAILED(g_pddsPrimary->lpVtbl->GetAttachedSurface(g_pddsPrimary, &ddsd.ddsCaps, &g_pddsBack)))
			return FALSE;
	}
#else
	// Win32 windowed: normal cooperative level, no display mode change.
	if (FAILED(g_pDD4->lpVtbl->SetCooperativeLevel(g_pDD4, g_hwndAppDC, DDSCL_NORMAL)))
	{
		g_pDD4->lpVtbl->Release(g_pDD4);
		g_pDD4 = NULL;
		g_pDD->lpVtbl->Release(g_pDD);
		g_pDD = NULL;
		return FALSE;
	}

	// Create primary surface (windowed — tied to our window's client area).
	{
		DDSURFACEDESC2 ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		if (FAILED(g_pDD4->lpVtbl->CreateSurface(g_pDD4, &ddsd, &g_pddsPrimary, NULL)))
			return FALSE;
	}

	// Create offscreen back buffer for rendering (640x480, 3D device).
	{
		DDSURFACEDESC2 ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		ddsd.dwWidth = 640;
		ddsd.dwHeight = 480;

		if (FAILED(g_pDD4->lpVtbl->CreateSurface(g_pDD4, &ddsd, &g_pddsBack, NULL)))
			return FALSE;
	}
#endif

	// Create the Direct3D device and a full-screen viewport tied to the back buffer.
	if (!DCV_InitDirect3D())
		return FALSE;


	return TRUE;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	double time, oldtime, newtime;
	host_initialized = FALSE;
	g_hInstance = hInstance;
	g_hPrevInstance = hPrevInstance;

	isDedicated = FALSE;
#ifdef _WIN32_WCE
	GDROM_ConfigureDoorBehavior();
#endif 
	Sys_Init();

	if ( !Sys_InitDisplay() )
	{
		Sys_Error("Sys_InitDisplay failed"); // remove that later
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
#ifndef _WIN32_WCE
			/* Win32: pump messages so closing the window posts WM_QUIT and we exit. */
			{
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						quit = 1;
						break;
					}
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if (quit)
				break;
#endif
			do
			{
				newtime = Sys_FloatTime();
				time = newtime - oldtime;
			} while (time < 0.015f); 

			oldtime = newtime;

			Host_Frame(time, giActive, &giStateInfo);
		}
	}
	Host_Shutdown();
	return 0;
}

/*
================
DCV_InitDirect3D

Create an IDirect3D3/IDirect3DDevice3 on the back buffer and set a 640x480
fullscreen D3DVIEWPORT2. 
================
*/
static qboolean DCV_InitDirect3D( void ) // move in dc_vidnt.c? 
{
	HRESULT        hr;
	DDSURFACEDESC2 ddsd;
	D3DVIEWPORT2   vp;
	LPDIRECT3DDEVICE3 dev;

	if (!g_pDD4 || !g_pddsBack)
		return FALSE;

	/* Create the Direct3D3 object if we don't have one yet. */
	if (!g_pD3D)
	{
		hr = g_pDD4->lpVtbl->QueryInterface(g_pDD4, &IID_IDirect3D3, (LPVOID*)&g_pD3D);
		if (FAILED(hr) || !g_pD3D)
			return FALSE;
	}

	/* Create a HAL device on the back buffer (fallback to RGB if HAL unavailable). */
	if (!g_pD3DDevice)
	{
		hr = g_pD3D->lpVtbl->CreateDevice(g_pD3D, &IID_IDirect3DHALDevice, g_pddsBack, &g_pD3DDevice, NULL);
		if (FAILED(hr) || !g_pD3DDevice)
		{
			hr = g_pD3D->lpVtbl->CreateDevice(g_pD3D, &IID_IDirect3DRGBDevice, g_pddsBack, &g_pD3DDevice, NULL);
			if (FAILED(hr) || !g_pD3DDevice)
				return FALSE;
		}
	}


	/* Create a viewport once and bind it as current. */
	if (!g_pD3DViewport)
	{
		hr = g_pD3D->lpVtbl->CreateViewport(g_pD3D, &g_pD3DViewport, NULL);
		if (FAILED(hr) || !g_pD3DViewport)
			return FALSE;

		hr = g_pD3DDevice->lpVtbl->AddViewport(g_pD3DDevice, g_pD3DViewport);
		if (FAILED(hr))
			return FALSE;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if (FAILED(g_pddsBack->lpVtbl->GetSurfaceDesc(g_pddsBack, &ddsd)))
		return FALSE;

	/* Configure viewport to cover entire back buffer with canonical clip space. */
	memset(&vp, 0, sizeof(vp));
	vp.dwSize   = sizeof(D3DVIEWPORT2);
	vp.dwX      = 0;
	vp.dwY      = 0;
	vp.dwWidth  = ddsd.dwWidth;
	vp.dwHeight = ddsd.dwHeight;
	vp.dvClipWidth  = 2.0f;
	vp.dvClipHeight = 2.0f;
	vp.dvClipX      = -1.0f;
	vp.dvClipY      = 1.0f;
	vp.dvMinZ       = 0.0f;
	vp.dvMaxZ       = 1.0f;

	hr = g_pD3DViewport->lpVtbl->SetViewport2(g_pD3DViewport, &vp);
	if (FAILED(hr))
		return FALSE;

	hr = g_pD3DDevice->lpVtbl->SetCurrentViewport(g_pD3DDevice, g_pD3DViewport);
	if (FAILED(hr))
		return FALSE;

	/* Cache the backbuffer pixel format for use by the renderer when
	 * creating texture surfaces, so we can match channel layout. */

	memset(&g_dc_backbuffer_fmt, 0, sizeof(g_dc_backbuffer_fmt));
	g_dc_backbuffer_fmt.dwSize = sizeof(g_dc_backbuffer_fmt);

	if ((ddsd.ddpfPixelFormat.dwSize == sizeof(DDPIXELFORMAT)) &&
		(ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB))
	{
		g_dc_backbuffer_fmt = ddsd.ddpfPixelFormat;
	}

	hr = g_pD3DDevice->lpVtbl->SetRenderTarget(g_pD3DDevice, g_pddsBack, 0);

	dev = g_pD3DDevice;


	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_SPECULARENABLE,     FALSE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_DITHERENABLE,       TRUE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ZENABLE,            D3DZB_TRUE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ZWRITEENABLE,       TRUE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ZFUNC,              D3DCMP_LESSEQUAL);


	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_CULLMODE,  D3DCULL_NONE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);


	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);


	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);  
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA);
	dev->lpVtbl->SetRenderState(dev, D3DRENDERSTATE_FOGENABLE, FALSE);


	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);

	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	dev->lpVtbl->SetTextureStageState(dev, 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
	dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	dev->lpVtbl->SetTextureStageState(dev, 1, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
	return TRUE;
}

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
short giSubState = 0;
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

// -----------------------------------------------------------------------------
// ASYNC FILE IO 
// -----------------------------------------------------------------------------

#define MAX_ASYNC 16

/* Binary DC file model: a 16-entry table shadowing each open HANDLE with a
 * duplicate handle + OVERLAPPED for async reads. nId == the primary HANDLE. */
typedef struct dc_syncslot_s
{
	int        nId;    // primary HANDLE this slot shadows (-1 = free)
	void      *pFile;  // duplicate HANDLE opened for async reads
	OVERLAPPED ov;
} dc_syncslot_t;

static dc_syncslot_t g_AsyncHandles[MAX_ASYNC];
int g_filesOpened;
int g_filesClosed;

/* GD-ROM filesystem hooks: the DC routes reads under \CD-ROM\ through its own
 * GD driver instead of CreateFileW. GDROM_Open returns a driver handle (0 if the
 * path isn't on the GD filesystem); Sys_IsGDPath tests a live handle. */
extern int  Sys_IsGDPath( void *hFile );
extern int  GDROM_Open( const char *path, const char *mode );
extern int  GDROM_Close( int hGDROM );
extern int  GDROM_FileSize( void *hFile );
extern int  GDROM_Read( void *buffer, int size, int count, void *hFile );
extern int  GDROM_Write( void *buffer, int size, int count, void *hFile );
extern int  GDROM_Seek( void *hFile, int offset, int whence );
extern int  GDROM_Tell( void *hFile );

extern void DCV_MeterText( unsigned int color, int x, int y, const char *text );

void Sys_RegisterFileHandle( const char *path, int hFile );
unsigned int DC_fwrite( void *buffer, unsigned int size, unsigned int count, void *hFile );

/* Shared narrow->wide staging buffer for every CreateFileW open (DAT_0020a630),
 * plus the byte the port clears immediately before each open (DAT_0020b62e). */
static WCHAR g_wOpenPath[MAX_PATH];
static BYTE  g_gdOpenFlag;

/* Sys_FPrintf coalescing buffer: writes to the same file accumulate here and
 * flush when the target file changes or the buffer would pass 0x400 bytes. */
static int  g_fprintfFile;
static char g_fprintfBuffer[0x400 + 4];
static int  g_fprintfLen;

// On-screen open/close counters ("%d opened %d closed", profilemeter overlay).
void DC_PrintFileCounts( void )
{
	char szText[68];

	sprintf(szText, "%d opened %d closed", g_filesOpened, g_filesClosed);
	DCV_MeterText(0x8000ff00, 0, (g_filesOpened - g_filesClosed) + 1, szText);
}

void Host_ExecConfig( void )
{
	Cbuf_AddText("exec config.cfg\n");
	Cbuf_Execute();
}

// GD-aware file size of an open handle.
DWORD DC_fsize( void *hFile )
{
	if (Sys_IsGDPath(hFile))
		return GDROM_FileSize(hFile);

	return GetFileSize(hFile, NULL);
}

// DC internal open helper (Sys_OpenHandle @ 0x122254). Read mode tries the GD-ROM
// driver first, then CreateFileW OPEN_EXISTING. Write/append refuses \CD-ROM\ paths
// and uses CREATE_ALWAYS (w) / OPEN_ALWAYS (a, seeks to end). Normalizes '/'->'\\',
// bumps g_filesOpened, returns the raw HANDLE or NULL on failure.
static HANDLE Sys_OpenHandle( const char *path, const char *mode )
{
	char   szPath[MAX_PATH];
	char  *p;
	DWORD  access;
	DWORD  creation;
	HANDLE hFile;

	if (!strchr(mode, 'w') && !strchr(mode, 'a'))
	{
		hFile = (HANDLE)GDROM_Open(path, mode);
		if (hFile != NULL)
			return hFile;

		creation = OPEN_EXISTING;
		access   = GENERIC_READ;
	}
	else
	{
		if (strstr(path, "CD-ROM"))
			return NULL;

		creation = strchr(mode, 'a') ? OPEN_ALWAYS : CREATE_ALWAYS;
		access   = GENERIC_WRITE;
	}

	strcpy(szPath, path);
	for (p = szPath; *p; ++p)
	{
		if (*p == '/')
			*p = '\\';
	}

	MultiByteToWideChar(CP_ACP, 0, szPath, -1, g_wOpenPath, ARRAYSIZE(g_wOpenPath));
	g_gdOpenFlag = 0;
	hFile = CreateFileW(g_wOpenPath, access, FILE_SHARE_READ, NULL, creation,
	                    FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
	}
	else
	{
		g_filesOpened++;
		if (creation == OPEN_ALWAYS)
			SetFilePointer(hFile, 0, NULL, FILE_END);
	}

	return hFile;
}

// Low-level close: flush any pending Sys_FPrintf output for this file, release
// the async shadow slot, then close the handle (GD-ROM or Win32).
int Sys_CloseHandle( void *hFile )
{
	dc_syncslot_t *slot = NULL;
	int i;

	if (hFile == (void *)g_fprintfFile)
	{
		DC_fwrite(g_fprintfBuffer, g_fprintfLen, 1, hFile);
		g_fprintfLen = 0;
	}

	if (Sys_IsGDPath(hFile) != 0)
		return GDROM_Close((int)hFile);

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if ((void *)g_AsyncHandles[i].nId == hFile)
		{
			slot = &g_AsyncHandles[i];
			break;
		}
	}

	if (slot != NULL)
	{
		g_filesClosed++;
		CloseHandle(slot->pFile);
		slot->nId = -1;
		slot->pFile = INVALID_HANDLE_VALUE;
	}

	if (!CloseHandle(hFile))
		return -1;

	g_filesClosed++;
	return 0;
}

// Buffered file printf: coalesce writes to the same file, flush when the target
// changes or the buffer would pass 0x400 bytes.
void Sys_FPrintf( int fileid, char *fmt, ... )
{
	va_list argptr;
	char    text[1024];
	int     len;

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	len = strlen(text);
	if (fileid != g_fprintfFile || g_fprintfLen + len > 0x400)
	{
		DC_fwrite(g_fprintfBuffer, g_fprintfLen, 1, (void *)g_fprintfFile);
		g_fprintfLen = 0;
	}

	g_fprintfFile = fileid;
	memcpy(g_fprintfBuffer + g_fprintfLen, text, len);
	g_fprintfLen += len;
}

// GD-aware stdio-style transfer helpers.  The Win32 path reads/writes one
// record at a time so short transfers report how many whole records landed.
unsigned int DC_fread( void *buffer, unsigned int size, unsigned int count, void *hFile )
{
	unsigned int n;
	DWORD        bytes;

	if (size == 0 || count == 0)
		return 0;

	if (Sys_IsGDPath(hFile) != 0)
		return GDROM_Read(buffer, size, count, hFile);

	for (n = 0; n < count; n++)
	{
		bytes = 0;
		if (!ReadFile(hFile, buffer, size, &bytes, NULL) || bytes != size)
			return n;
		buffer = (char *)buffer + size;
	}

	return n;
}

unsigned int DC_fwrite( void *buffer, unsigned int size, unsigned int count, void *hFile )
{
	unsigned int n;
	DWORD        bytes;

	if (size == 0 || count == 0)
		return 0;

	if (Sys_IsGDPath(hFile) != 0)
		return GDROM_Write(buffer, size, count, hFile);

	for (n = 0; n < count; n++)
	{
		bytes = 0;
		if (!WriteFile(hFile, buffer, size, &bytes, NULL) || bytes != size)
			return n;
		buffer = (char *)buffer + size;
	}

	return n;
}

int DC_fseek( void *hFile, int offset, int whence )
{
	DWORD method;

	if (Sys_IsGDPath(hFile) != 0)
		return GDROM_Seek(hFile, offset, whence);

	if (whence == SEEK_CUR)
		method = FILE_CURRENT;
	if (whence == SEEK_SET)
		method = FILE_BEGIN;
	if (whence == SEEK_END)
		method = FILE_END;

	SetFilePointer(hFile, offset, NULL, method);
	return 0;
}

int DC_ftell( void *hFile )
{
	if (Sys_IsGDPath(hFile) != 0)
		return GDROM_Tell(hFile);

	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}

// Shadow an open HANDLE with a second read handle for async I/O: find a free
// slot (nId == -1) and open a duplicate of path into it.
void Sys_RegisterFileHandle( const char *path, int hFile )
{
	HANDLE hDup;
	int    i;

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if (g_AsyncHandles[i].nId == -1)
		{
			MultiByteToWideChar(CP_ACP, 0, path, -1, g_wOpenPath, ARRAYSIZE(g_wOpenPath));
			g_gdOpenFlag = 0;
			hDup = CreateFileW(g_wOpenPath, GENERIC_READ, FILE_SHARE_READ, NULL,
			                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hDup != NULL)
				g_filesOpened++;
			if (hDup != INVALID_HANDLE_VALUE)
			{
				g_AsyncHandles[i].nId   = hFile;
				g_AsyncHandles[i].pFile = hDup;
				return;
			}
		}
	}
}

// Sys_FileOpenRead @ 0x122b3c. Opens path for reading (GD-ROM or CreateFileW),
// writes the raw HANDLE to *pHandle, optionally registers it for async reads, and
// returns the file size (-1 on failure).
int Sys_FileOpenRead( char *path, int *pHandle, int bRegisterAsync )
{
	HANDLE hFile;
	DWORD  size;
	char  *p;

	hFile = (HANDLE)GDROM_Open(path, "rb");
	if (hFile == NULL)
	{
		for (p = path; *p; ++p)
		{
			if (*p == '/')
				*p = '\\';
		}

		MultiByteToWideChar(CP_ACP, 0, path, -1, g_wOpenPath, ARRAYSIZE(g_wOpenPath));
		g_gdOpenFlag = 0;
		hFile = CreateFileW(g_wOpenPath, GENERIC_READ, FILE_SHARE_READ, NULL,
		                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			*pHandle = -1;
			return -1;
		}

		if (bRegisterAsync)
			Sys_RegisterFileHandle(path, (int)hFile);
	}
	else
	{
		*pHandle = (int)hFile;
	}

	g_filesOpened++;
	*pHandle = (int)hFile;

	if (Sys_IsGDPath(hFile))
		size = GDROM_FileSize(hFile);
	else
		size = GetFileSize(hFile, NULL);

	return size;
}

FILE* Sys_FOpenReadSeek( const char* path, int offset )
{
	HANDLE h;

	if (!path)
		return NULL;

	h = Sys_OpenHandle(path, "rb");

	if (h == NULL)
		return NULL;

	SetFilePointer(h, offset, NULL, FILE_BEGIN);
	CloseHandle(h);

	return NULL;
}

int Sys_FileOpenWrite( char *path )
{
	(void)path;
	Sys_Error("File write not supported on Dreamcast");
	return -1;
}

void Sys_FileClose( void *hFile )
{
	dc_syncslot_t *slot = NULL;
	int i;

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if ((void *)g_AsyncHandles[i].nId == hFile)
		{
			slot = &g_AsyncHandles[i];
			break;
		}
	}

	if (slot != NULL)
	{
		g_filesClosed++;
		CloseHandle(slot->pFile);
		slot->nId = -1;
		slot->pFile = INVALID_HANDLE_VALUE;
	}
	CloseHandle(hFile);
	g_filesClosed++;
}

/* DC file I/O uses raw Win32 HANDLEs with a GD-ROM path abstraction: Sys_IsGDPath
 * (declared above) routes I/O through the GD-ROM driver layer instead of CreateFile. */
extern int GD_Read( void *buffer, int size, int count, void *hFile );
extern int GD_Seek( void *hFile, int offset, int whence );

void Sys_FileSeek( void *hFile, int position )
{
	if (Sys_IsGDPath(hFile) != 0)
		GD_Seek(hFile, position, 0);
	else
		SetFilePointer(hFile, position, NULL, FILE_BEGIN);
}

int Sys_FileRead( void *hFile, void *dest, int count )
{
	DWORD bytesRead = 0;

	if (Sys_IsGDPath(hFile) != 0)
		bytesRead = GD_Read(dest, 1, count, hFile);
	else
		ReadFile(hFile, dest, count, &bytesRead, NULL);

	return bytesRead;
}

/* The DC is read-only from the GD-ROM, so writes never reach a real device
 * (Sys_FileOpenWrite is a hard error). The handle passed in is the raw HANDLE
 * that Sys_FileOpenRead stored in *pHandle. */
int Sys_FileWrite( int handle, void *data, int count )
{
	DWORD bytesWritten = 0;

	WriteFile((HANDLE)handle, data, (DWORD)count, &bytesWritten, NULL);
	return (int)bytesWritten;
}

// Sys_FileTime @ 0x122d44. Returns the file's high modification-time dword, or -1
// if it can't be opened; GD-ROM files always report present (1).
int Sys_FileTime( char *path )
{
	HANDLE   hFile;
	int      hGDROM;
	DWORD    ftime = (DWORD)-1;
	FILETIME mtime;
	char    *p;

	hGDROM = GDROM_Open(path, "rb");
	if (hGDROM == 0)
	{
		for (p = path; *p; ++p)
		{
			if (*p == '/')
				*p = '\\';
		}

		MultiByteToWideChar(CP_ACP, 0, path, -1, g_wOpenPath, ARRAYSIZE(g_wOpenPath));
		g_gdOpenFlag = 0;
		hFile = CreateFileW(g_wOpenPath, GENERIC_READ, FILE_SHARE_READ, NULL,
		                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			if (GetFileTime(hFile, NULL, NULL, &mtime))
				ftime = mtime.dwHighDateTime;
			CloseHandle(hFile);
		}
	}
	else
	{
		GDROM_Close(hGDROM);
		ftime = 1;
	}

	return ftime;
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
	return (int)SetFilePointer((HANDLE)i, 0, NULL, FILE_CURRENT);
}

// -----------------------------------------------------------------------------
// ASYNC HELPERS (Sys_AsyncBusy / Sys_FileReadAsync)
// -----------------------------------------------------------------------------

qboolean Sys_AsyncBusy( int id, LPOVERLAPPED pov )
{
	dc_syncslot_t *slot = NULL;
	DWORD          bytes = 0;
	int            i;

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if (g_AsyncHandles[i].nId == id)
		{
			slot = &g_AsyncHandles[i];
			break;
		}
	}

	if (slot == NULL)
		Sys_Error("Sys_AsyncBusy on unregistered sync handle\n");

	if (pov == NULL)
		pov = &slot->ov;

	return GetOverlappedResult(slot->pFile, pov, &bytes, FALSE) == 0;
}

int Sys_FileReadAsync( void *hFile, void *buffer, int count, struct _OVERLAPPED *pov )
{
	dc_syncslot_t *slot = NULL;
	DWORD          bytes = 0;
	DWORD          pos;
	void          *hRealFile;
	int            i;

	pos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if ((void *)g_AsyncHandles[i].nId == hFile)
		{
			slot = &g_AsyncHandles[i];
			break;
		}
	}

	if (slot == NULL)
		Sys_Error("Sys_FileReadAsync on unregistered sync handle\n");

	hRealFile = slot->pFile;
	if (pov == NULL)
		pov = &slot->ov;

	GetOverlappedResult(hRealFile, pov, &bytes, TRUE);
	pov->hEvent     = NULL;
	pov->Offset     = pos;
	pov->OffsetHigh = 0;
	ReadFile(hRealFile, buffer, count, &bytes, pov);
	return count;
}

// -----------------------------------------------------------------------------
// MEMORY PROTECTION
// -----------------------------------------------------------------------------

void Sys_MakeCodeWriteable( unsigned long startaddr, unsigned long length )
{
	Sys_Error("And WHY would you want to make code writable?");
}

// -----------------------------------------------------------------------------
// ERROR/PRINT/QUIT
// -----------------------------------------------------------------------------

// Fatal error with a caller-chosen RGB565 background bar.  C89 varargs can't
// be forwarded, so this and Sys_Error carry duplicate bodies.
void Sys_ErrorColor( int wColor, char *error, ... )
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	strcat(text, "\n");

	DCV_FB_BackgroundRect(wColor);
	DCV_FB_Text(text);

	giActive = DLL_INACTIVE;
	Mnemo_ReportToFile();

	for (;;)
	{
	}
}

void Sys_Error( char *error, ... )
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	strcat(text, "\n");

	DCV_FB_BackgroundRect(0xFFFF);
	DCV_FB_Text(text);

	giActive = DLL_INACTIVE;
	Mnemo_ReportToFile();

	for (;;)
	{
	}
}

void Sys_WinError( void )
{
	char text[1024];

	sprintf(text, "Windows error %d; sorry for the numeric cop-out, but FormatMessage isn't available.", GetLastError());
	//Sys_Error
	("%s", text);
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
	Sys_Error("Sys_Quit");
}

// -----------------------------------------------------------------------------
// TIMING 
// -----------------------------------------------------------------------------

static float  g_pfreq       = 0.0f;
static float  g_curtime     = 0.0f;
static float  g_lastcurtime = 0.0f;
static int    g_lowshift    = 0;
static DWORD  g_baseTick    = 0;

DLL_EXPORT float Sys_FloatTime( void )
{
	static DWORD s_base = 0;
	DWORD        base = s_base;

	if (base == 0)
	{
		s_base = GetTickCount();
		return 0.0f;
	}

	return (GetTickCount() - base) * 0.001f;
}

// Sys_InitFloatTime @ 0x122fec. Reads the perf-counter frequency (Sys_Error if none),
// normalizes it to a ~1us-resolution float so pfreq = 1/freq, latches g_baseTick, honors
// -starttime, and clears the async handle table. pfreq/lowshift are vestigial on the DC:
// the port's Sys_FloatTime uses the GetTickCount()*0.001f path instead.
void Sys_InitFloatTime( void )
{
	LARGE_INTEGER  perfFreq;
	unsigned int   lowpart, highpart;
	float          freq;
	int            j;
	int            i;
	dc_syncslot_t *slot;

	if (!QueryPerformanceFrequency(&perfFreq))
		Sys_Error("No hardware timer available");

	lowpart  = (unsigned int)perfFreq.LowPart;
	highpart = (unsigned int)perfFreq.HighPart;
	g_lowshift = 0;

	while (highpart || (float)lowpart > 2000000.0f)
	{
		g_lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	freq = (float)lowpart;
	g_pfreq = 1.0f / freq;

	if (g_baseTick == 0)
		g_baseTick = GetTickCount();
	else
		GetTickCount();

	j = COM_CheckParm("-starttime");
	if (j)
		g_curtime = (float)Q_atof(com_argv[j + 1]);
	else
		g_curtime = 0.0f;
	g_lastcurtime = g_curtime;

	slot = g_AsyncHandles;
	for (i = 0; i < MAX_ASYNC; i++)
	{
		slot->nId   = -1;
		slot->pFile = INVALID_HANDLE_VALUE;
		slot++;
	}
}

void Sys_ShutdownFloatTime( void )
{
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
// Engine/DLL interface (static link)
//
// The DC links the game DLL statically, so both interface directions were
// devirtualized: the engine calls the game's Dispatch* functions directly,
// and the game calls the PF_* engine functions directly.  What remains is
// the export registry used by save/restore to map function pointers to
// names: cbase.cpp fills a 512-entry {function, name} table at startup
// (GameDLL_RegisterModules, called from Host_Init) through the typed
// registration entry points below, and FunctionFromName / NameForFunction
// (also cbase.cpp) search it.
// -----------------------------------------------------------------------------

extern void Sys_RegisterExport( char *pName, unsigned int function );	// cbase.cpp

void Sys_RegisterExportA( char *pName, unsigned int function )
{
	Sys_RegisterExport(pName, function);
}

void Sys_RegisterExportB( char *pName, unsigned int function )
{
	Sys_RegisterExport(pName, function);
}

void Sys_RegisterExportC( char *pName, unsigned int function )
{
	Sys_RegisterExport(pName, function);
}

void Sys_RegisterExportD( char *pName, unsigned int function )
{
	Sys_RegisterExport(pName, function);
}

void Sys_RegisterExportE( char *pName, unsigned int function )
{
	Sys_RegisterExport(pName, function);
}

void LoadThisDll( char *szDllFilename )
{
}

// Returns entity initialization functions, generated by LINK_ENTITY_TO_CLASS
ENTITYINIT GetEntityInit( char *pClassName )
{
	return (ENTITYINIT)GetDispatch(pClassName);
}

void EngineFprintf( void *pFile, char *szFmt, ... )
{
	va_list argptr;

	va_start(argptr, szFmt);
	vfprintf((FILE *)pFile, szFmt, argptr);
	va_end(argptr);
}

void GameSetState( int iState )
{
	giActive = iState;
}

// AlertMessage @ 0x123248. Gated on the developer cvar: formats into a shared global
// buffer, echoes it to the console verbatim, and escalates to Sys_Error on at_error.
static char g_szAlertMsg[1024];

void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list argptr;

	if (developer.value)
	{
		va_start(argptr, szFmt);
		vsprintf(g_szAlertMsg, szFmt, argptr);
		va_end(argptr);

		Con_Printf("%s", g_szAlertMsg);
		if (atype == at_error)
			Sys_Error(g_szAlertMsg);
	}
}

void Dispatch_Substate( int iSubState )
{
	giSubState = iSubState;
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
	Sys_InitFloatTime();

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

static qboolean DCV_InitDirect3D( void );

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

/* ------------------------------------------------------------------------- *
 * Sys_Frame - launcher-side frame pump above the exported Host_Frame.
 * Rate-limits to g_minFrameTime via a Sys_FloatTime busy-wait, runs one
 * Host_Frame, then drives the DLL_STATE machine (pause/trans/close).
 * ------------------------------------------------------------------------- */
static int      g_engineState = DLL_INACTIVE;   /* launcher DLL state      */
static float    g_lastFrameTime;                /* timestamp of last frame */
static int      g_engineStateInfo;              /* Host_Frame stateInfo out*/
static float    g_frameSampleTime;              /* accum minus sample cost */
static float    g_minFrameTime;                 /* min seconds per frame   */
static int      g_sampleBase;                   /* sample-count baseline   */
static float    g_frameAccum;                   /* accumulated frame time  */
static HMODULE  g_hGameDll;                      /* loaded game DLL handle  */
static int      g_pauseCounter;                 /* pause debounce counter  */
static int      g_pauseFlag;                    /* pause pending flag      */
static int      g_closeFlag;                    /* killserver-once guard   */

extern int  Host_Frame( float time, int iState, int *stateInfo );
extern int  Sys_SampleCount( void );   /* 0x176ef0 - perf sample count  */
extern void Sys_NotifyState( int state );   /* 0x123240 - state-change hook */

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
#ifdef _WIN32_WCE
	GDROM_ConfigureDoorBehavior();
#endif 
	Sys_InitFloatTime();

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

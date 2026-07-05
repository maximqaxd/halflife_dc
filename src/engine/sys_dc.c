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
#include <segagdrm.h>
#include <ceddcdrm.h>
#include <wdm.h>
#include <tchar.h>

HWND*				pmainwindow;
HINSTANCE       g_hInstance     = NULL;
HINSTANCE       g_hPrevInstance = NULL;


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

/* A 16-entry table shadowing each open HANDLE with a duplicate handle +
 * OVERLAPPED for async reads. nId == the primary HANDLE. */
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
 * GD driver instead of CreateFileW. Bopen returns a driver handle (0 if the
 * path isn't on the GD filesystem); IsBfile tests a live handle. */
extern int  IsBfile( void *hFile );
extern int  Bopen( const char *path, const char *mode );
extern int  Bclose( int hGDROM );
extern int  Bsize( void *hFile );
extern int  Bread( void *buffer, int size, int count, void *hFile );
extern int  Bwrite( void *buffer, int size, int count, void *hFile );
extern int  Bseek( void *hFile, int offset, int whence );
extern int  Btell( void *hFile );
extern int  Beof( void *hFile );


void Sys_RegisterFileHandle( const char *path, int hFile );
unsigned int DC_fwrite( void *buffer, unsigned int size, unsigned int count, void *hFile );

/* Shared narrow->wide staging buffer for every CreateFileW open, plus the flag
 * byte cleared immediately before each open. */
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
	if (IsBfile(hFile))
		return Bsize(hFile);

	return GetFileSize(hFile, NULL);
}

// Internal open helper. Read mode tries the GD-ROM driver first, then CreateFileW
// OPEN_EXISTING. Write/append refuses \CD-ROM\ paths
// and uses CREATE_ALWAYS (w) / OPEN_ALWAYS (a, seeks to end). Normalizes '/'->'\\',
// bumps g_filesOpened, returns the raw HANDLE or NULL on failure.
HANDLE Sys_OpenHandle( const char *path, const char *mode )
{
	char   szPath[MAX_PATH];
	char  *p;
	DWORD  access;
	DWORD  creation;
	HANDLE hFile;

	if (!strchr(mode, 'w') && !strchr(mode, 'a'))
	{
		hFile = (HANDLE)Bopen(path, mode);
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

	if (IsBfile(hFile) != 0)
		return Bclose((int)hFile);

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
	BOOL         ok;

	if (size == 0 || count == 0)
		return 0;

	if (IsBfile(hFile) != 0)
		return Bread(buffer, size, count, hFile);

	for (n = 0; n < count; n++)
	{
		bytes = 0;
		ok = ReadFile(hFile, buffer, size, &bytes, NULL);
		if (bytes != size || !ok)
			return n;
		buffer = (char *)buffer + size;
	}

	return n;
}

unsigned int DC_fwrite( void *buffer, unsigned int size, unsigned int count, void *hFile )
{
	unsigned int n;
	DWORD        bytes;
	BOOL         ok;

	if (size == 0 || count == 0)
		return 0;

	if (IsBfile(hFile) != 0)
		return Bwrite(buffer, size, count, hFile);

	for (n = 0; n < count; n++)
	{
		bytes = 0;
		ok = WriteFile(hFile, buffer, size, &bytes, NULL);
		if (bytes != size || !ok)
			return n;
		buffer = (char *)buffer + size;
	}

	return n;
}

int DC_fseek( void *hFile, int offset, int whence )
{
	DWORD method;

	if (IsBfile(hFile) != 0)
		return Bseek(hFile, offset, whence);

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
	if (IsBfile(hFile) != 0)
		return Btell(hFile);

	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}

// Shadow an open HANDLE with a second read handle for async I/O: find a free
// slot (nId == -1) and open a duplicate of path into it.
// GD-aware feof of an open handle.
int DC_feof( void *hFile )
{
	if (IsBfile(hFile))
		return Beof(hFile);

	return DC_fsize(hFile) <= (DWORD)DC_ftell(hFile);
}

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

// Opens path for reading (GD-ROM or CreateFileW),
// writes the raw HANDLE to *pHandle, optionally registers it for async reads, and
// returns the file size (-1 on failure).
int Sys_FileOpenRead( char *path, int *pHandle, int bRegisterAsync )
{
	HANDLE hFile;
	DWORD  size;
	char  *p;

	hFile = (HANDLE)Bopen(path, "rb");
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

	if (IsBfile(hFile))
		size = Bsize(hFile);
	else
		size = GetFileSize(hFile, NULL);

	return size;
}

int Sys_FileOpenWrite( char *path )
{
	(void)path;
	Sys_Error("File write not supported on Dreamcast");
	return -1;
}

void Sys_FileClose( int hFile )
{
	dc_syncslot_t *slot = NULL;
	int i;

	for (i = 0; i < MAX_ASYNC; i++)
	{
		if (g_AsyncHandles[i].nId == hFile)
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
	CloseHandle((HANDLE)hFile);
	g_filesClosed++;
}

void Sys_FileSeek( int hFile, int position )
{
	if (IsBfile((void *)hFile) != 0)
		Bseek((void *)hFile, position, 0);
	else
		SetFilePointer((HANDLE)hFile, position, NULL, FILE_BEGIN);
}

int Sys_FileRead( int hFile, void *dest, int count )
{
	DWORD bytesRead = 0;

	if (IsBfile((void *)hFile) != 0)
		bytesRead = Bread(dest, 1, count, (void *)hFile);
	else
		ReadFile((HANDLE)hFile, dest, count, &bytesRead, NULL);

	return bytesRead;
}

// Returns the file's high modification-time dword, or -1
// if it can't be opened; GD-ROM files always report present (1).
int Sys_FileTime( char *path )
{
	HANDLE   hFile;
	int      hGDROM;
	DWORD    ftime = (DWORD)-1;
	FILETIME mtime;
	char    *p;

	hGDROM = Bopen(path, "rb");
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
		Bclose(hGDROM);
		ftime = 1;
	}

	return ftime;
}

void Sys_mkdir( char *path )
{
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

void Sys_InitFloatTime( void )
{
	int j;

	Sys_FloatTime();

	j = COM_CheckParm("-starttime");
	if (j)
		g_curtime = (float)Q_atof(com_argv[j + 1]);
	else
		g_curtime = 0.0f;

	g_lastcurtime = g_curtime;
}

// Reads the perf-counter frequency (Sys_Error if none) and normalizes it to a
// ~1us-resolution float so pfreq = 1/freq; pfreq/lowshift end up unused since
// Sys_FloatTime uses the GetTickCount()*0.001f path.
void Sys_Init( void )
{
	LARGE_INTEGER  perfFreq;
	unsigned int   lowpart, highpart;
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

	g_pfreq = 1.0f / (float)lowpart;

	Sys_InitFloatTime();

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

// -----------------------------------------------------------------------------
// Engine/DLL interface (static link)
//
// The game DLL is linked statically, so the engine calls the game's Dispatch*
// functions directly and the game calls the PF_* engine functions directly.
// What remains is
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

int COM_CompareFileTime( int *ft1, int *ft2 )
{
	int iCompare = 0;

	if (ft1 && ft2)
	{
		if (*ft1 < *ft2)
			iCompare = -1;
		else if (*ft2 < *ft1)
			iCompare = 1;
	}

	return iCompare;
}

void GameSetSubState( int iSubState )
{
	if (iSubState & 2)
		giStateInfo = 1;
	else if (iSubState != 1)
		giStateInfo = iSubState;
}

void GameSetState( int iState )
{
	giActive = iState;
}

// Gated on the developer cvar: formats into a shared global
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
// Door state from the low-level driver.
int g_gdDoorOpened;
int g_gdDoorPending;

extern void GDROM_DoorReset( void );

void GDROM_ConfigureDoorBehavior( void )
{
	HANDLE hGDROM;
	DWORD  dwBehavior;
	DWORD  dwReturned;

	hGDROM = CreateFile(TEXT("\\Device\\CDROM0"),
	                    GENERIC_READ,
	                    0,
	                    NULL,
	                    OPEN_EXISTING,
	                    0,
	                    NULL);
	if (hGDROM != INVALID_HANDLE_VALUE)
	{
		// Request "notify app" behavior instead of reboot on door open. A
		// missing disc is fine here; the door flow below deals with it.
		dwBehavior = 0;
		if (!DeviceIoControl(hGDROM,
		                     IOCTL_SEGACD_SET_DOOR_BEHAVIOR,
		                     &dwBehavior,
		                     sizeof(dwBehavior),
		                     NULL,
		                     0,
		                     &dwReturned,
		                     NULL)
			&& GetLastError() != ERROR_NO_MEDIA_IN_DRIVE)
		{
			Sys_Error("Error setting GD-ROM door behavior (0x%08x).\n", GetLastError());
		}

		CloseHandle(hGDROM);
	}

	g_gdDoorPending = 0;
	if (g_gdDoorOpened)
		GDROM_DoorReset();
}


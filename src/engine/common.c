// common.c -- engine bootstrap
//
// Picks the heap the engine runs out of, then hands it to Host_Init. On a
// machine with room to spare the heap comes from the WinCE allocator; on a
// tight one we map a fixed physical block instead.

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"

#include <windows.h>
#include <wdm.h>

static char          *g_GameArgv[1];
static unsigned char *g_pEngineMem     = NULL;
static int            g_iEngineMemSize = 0;

#define ENGINE_MEMORY_RESERVE		1700000
#define MIN_ENGINE_HEAP_BYTES		0x800000
#define FIXED_ENGINE_HEAP_ADDRESS	0x8c800000

/*
==================
GameInit

Reserves the game heap and starts the engine on it. Everything below 8 MiB of
free physical memory falls back to the mapped block; anything larger keeps
1.7 MB back for the system and commits the rest.
==================
*/
qboolean GameInit( void )
{
	MEMORYSTATUS stat;
	quakeparms_t parms;

	stat.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&stat);

	g_iEngineMemSize = stat.dwAvailPhys - ENGINE_MEMORY_RESERVE;
	if (stat.dwAvailPhys < MIN_ENGINE_HEAP_BYTES)
	{
		PHYSICAL_ADDRESS pa;

		g_iEngineMemSize = MIN_ENGINE_HEAP_BYTES;
		pa.LowPart = FIXED_ENGINE_HEAP_ADDRESS;
		g_pEngineMem = (unsigned char *)MmMapIoSpace(pa, g_iEngineMemSize, TRUE);
	}
	else
	{
		g_pEngineMem = (unsigned char *)VirtualAlloc(NULL, g_iEngineMemSize, MEM_COMMIT, PAGE_READWRITE);
	}

	if (!g_pEngineMem)
		return FALSE;

	Sys_Init();

	parms.argv = g_GameArgv;
	parms.basedir = "/CD-ROM";
	parms.cachedir = NULL;
	parms.argc = 1;
	g_GameArgv[0] = "";
	parms.membase = g_pEngineMem;
	parms.memsize = g_iEngineMemSize;

	return Host_Init(&parms) != 0;
}

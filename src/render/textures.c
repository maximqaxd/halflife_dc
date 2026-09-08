// textures.c

#include "quakedef.h"
#include "textures.h"
#include "kzap.h"
#include <windows.h>

#define TEX_MAX_WADS			40
#define TEX_MAX_MIPTEX_NAME		64

int			nummiptex = 0;
char		miptex[MAX_MAP_TEXTURES][TEX_MAX_MIPTEX_NAME];

// Lump data
typedef struct
{
	lumpinfo_t	lump;
	int			iTexFile;	// index of the wad this texture is located in
} texlumpinfo_t;

void*		texfiles[TEX_MAX_WADS];
char		texpaths[TEX_MAX_WADS][MAX_PATH];
int			nTexFiles = 0;
texlumpinfo_t* lumpinfo = NULL;
int			nTexLumps = 0;
int			currentTexFile = -1;

void SafeRead( void* f, void* buffer, int count )
{
	if (DC_fread(buffer, count, 1, f) != 1)
		Sys_Error("File read failure");
}

void CleanupName( char* in, char* out )
{
	int			i;

	for (i = 0; i < 16; i++)
	{
		if (!in[i])
			break;

		out[i] = toupper(in[i]);
	}

	for (; i < 16; i++)
		out[i] = 0;
}

/*
=================
lump_sorter
=================
*/

int
lump_sorter( const void* lump1, const void* lump2 )
{
	texlumpinfo_t* plump1 = (texlumpinfo_t*)lump1;
	texlumpinfo_t* plump2 = (texlumpinfo_t*)lump2;
	return strcmp(plump1->lump.name, plump2->lump.name);
}

// Convert \\ to /
void ForwardSlashes( char* pname )
{
	while (*pname)
	{
		if (*pname == '\\')
			*pname = '/';
		pname++;
	}
}

/*
=================
TEX_InitFromWad
=================
*/
qboolean TEX_InitFromWad( char* path )
{
	int			i;
	int			dstOffset;
	int			srcOffset;
	wadinfo_t	wadinfo;
	char		szTmpPath[1024];
	char*		pszWadFile;

	strcpy(szTmpPath, path);

	// temporary kludge so we don't have to deal with no occurances of a semicolon
	//  in the path name ..
	if (strchr(szTmpPath, ';') == NULL)
		strcat(szTmpPath, ";");

	pszWadFile = strtok(szTmpPath, ";");

	while (pszWadFile)
	{
		void*		texfile;
		char		wadPath[MAX_PATH];
		char		wadName[MAX_PATH];

		ForwardSlashes(pszWadFile);

		COM_FileBase(pszWadFile, wadName);
		sprintf(wadPath, "%s/%s", com_gamedir, wadName);
		COM_DefaultExtension(wadPath, ".wad");

		if (strstr(wadName, "pldecal"))
		{
			pszWadFile = strtok(NULL, ";");
			continue;
		}

		strcpy(texpaths[nTexFiles], wadPath);
		texfile = Sys_OpenHandle(wadPath, "rb");
		texfiles[nTexFiles] = texfile;
		if (!texfile)
		{
			COM_FileBase(pszWadFile, wadName);
			sprintf(wadPath, "/CD-ROM/valve/%s", wadName);
			COM_DefaultExtension(wadPath, ".wad");

			strcpy(texpaths[nTexFiles], wadPath);
			texfile = Sys_OpenHandle(wadPath, "rb");
			texfiles[nTexFiles] = texfile;
		}

		if (!texfile)
		{
			Sys_ErrorColor(RGB565_GREEN, "ERROR: couldn't open %s\n", wadPath);
			return FALSE;
		}

		nTexFiles++;

		SafeRead(texfile, &wadinfo, sizeof(wadinfo));
		if (strncmp(wadinfo.identification, "WAD2", 4) &&
			strncmp(wadinfo.identification, "WAD3", 4))
		{
			Sys_Error("TEX_InitFromWad: %s isn't a wadfile - probably got built incorrectly due to a renegade 4-bit BMP.", wadPath);
		}

		wadinfo.numlumps = LittleLong(wadinfo.numlumps);
		wadinfo.infotableofs = LittleLong(wadinfo.infotableofs);
		DC_fseek(texfile, wadinfo.infotableofs, SEEK_SET);

		//
		// WAD lumps are packed without the texture file index.
		//
		// The runtime table adds that index to every record.
		//
		// Grow the table to its expanded stride before reading
		// the packed records into it.
		//
		lumpinfo = (texlumpinfo_t*)DebugRealloc(lumpinfo,
			sizeof(texlumpinfo_t) * (nTexLumps + wadinfo.numlumps), __FILE__, __LINE__);

		SafeRead(texfile, &lumpinfo[nTexLumps], wadinfo.numlumps * sizeof(lumpinfo_t));

		i = wadinfo.numlumps;
		if (i)
		{
			i--;
			dstOffset = i * sizeof(texlumpinfo_t);
			srcOffset = i * sizeof(lumpinfo_t);

			do
			{
				byte*		base = (byte*)&lumpinfo[nTexLumps];

				memmove(base + dstOffset, base + srcOffset, sizeof(lumpinfo_t));
				srcOffset -= sizeof(lumpinfo_t);
				dstOffset -= sizeof(texlumpinfo_t);
			}
			while (i--);
		}

		for (i = 0; i < wadinfo.numlumps; i++)
		{
			CleanupName(lumpinfo[nTexLumps].lump.name, lumpinfo[nTexLumps].lump.name);

			lumpinfo[nTexLumps].lump.filepos = LittleLong(lumpinfo[nTexLumps].lump.filepos);
			lumpinfo[nTexLumps].lump.disksize = LittleLong(lumpinfo[nTexLumps].lump.disksize);
			lumpinfo[nTexLumps].iTexFile = nTexFiles - 1;

			nTexLumps++;
		}

		pszWadFile = strtok(NULL, ";");
	}

	qsort(lumpinfo, nTexLumps, sizeof(texlumpinfo_t), lump_sorter);

	return TRUE;
}

qboolean TEX_FileExists( const char* path )
{
	void*		file;

	file = Sys_OpenHandle(path, "rb");
	if (file)
		Sys_CloseHandle(file);
	return file != NULL;
}

int TEX_FindPerMapWads( const char* mapPath, char* outPath )
{
	int			i;
	int			found;
	char		wadProbe[MAX_PATH];
	char		wadList[MAX_PATH];

	i = 1;
	found = 0;
	strcpy(wadList, "");
	while (1)
	{
		Sys_SetTaskName("Looking for small per level wads");
		sprintf(wadProbe, "%s/%d_%s.wad", com_gamedir, i, mapPath);
		if (!TEX_FileExists(wadProbe))
			break;

		sprintf(wadProbe, "%d_%s.wad", i, mapPath);
		strcat(wadList, wadProbe);
		strcat(wadList, ";");
		found++;
		i++;
	}

	if (found)
		strcpy(outPath, wadList);
	return found;
}

/*
=================
TEX_BuildPerMapWadPath
=================
*/
qboolean TEX_BuildPerMapWadPath( const char* mapPath, char* outPath )
{
	void*		file;
	char		mapName[MAX_PATH];
	char		mapPathLocal[MAX_PATH];

	strcpy(mapPathLocal, mapPath);
	ForwardSlashes(mapPathLocal);
	COM_FileBase(mapPathLocal, mapName);
	sprintf(outPath, "%s/%s", com_gamedir, mapName);
	COM_DefaultExtension(outPath, ".wad");

	if (TEX_FindPerMapWads(mapPath, outPath))
		return TRUE;

	Sys_SetTaskName("Looking for monolithic per level wad");
	file = Sys_OpenHandle(outPath, "rb");
	if (!file)
	{
		COM_FileBase(mapPathLocal, mapName);
		sprintf(outPath, "valve/%s", mapName);
		COM_DefaultExtension(outPath, ".wad");
		file = Sys_OpenHandle(outPath, "rb");
	}

	if (!file)
		return FALSE;

	Sys_CloseHandle(file);
	strcpy(outPath, mapName);
	return TRUE;
}

qboolean TEX_IsLevelWad( const char* name )
{
	const char*	path;

	for (path = name; *path; path++)
	{
		if (*path >= '0' && *path <= '9' && path[1] == '_')
			return TRUE;
	}
	return FALSE;
}

/*
=================
TEX_SelectLevelWad
=================
*/
void TEX_SelectLevelWad( int wadIndex )
{
	qboolean	isLevelWad;

	isLevelWad = wadIndex >= 0;
	if (isLevelWad && !TEX_IsLevelWad(texpaths[wadIndex]))
		return;

	if (wadIndex == currentTexFile)
		return;

	if (currentTexFile >= 0)
		Bremove_path(texpaths[currentTexFile]);

	if (isLevelWad)
	{
		if (texfiles[wadIndex])
			Sys_CloseHandle(texfiles[wadIndex]);

		Bfetch_disc(texpaths[wadIndex]);
		Sys_SetTaskName("accelerating per-level WAD");
		texfiles[wadIndex] = Sys_OpenHandle(texpaths[wadIndex], "rb");
		currentTexFile = wadIndex;
	}
}

/*
=================
TEX_CleanupWadInfo
=================
*/
void TEX_CleanupWadInfo( void )
{
	int			i;

	if (lumpinfo)
	{
		free(lumpinfo);
		lumpinfo = NULL;
	}

	for (i = 0; i < nTexFiles; i++)
	{
		Sys_CloseHandle(texfiles[i]);
		texfiles[i] = NULL;
	}

	nTexLumps = 0;
	nTexFiles = 0;
	TEX_SelectLevelWad(-1);
}

/*
=================
TEX_LoadLump
=================
*/
int TEX_LoadLump( char* name, byte* dest )
{
	texlumpinfo_t key;
	texlumpinfo_t* found;

	CleanupName(name, key.lump.name);

	// Find the lump
	found = (texlumpinfo_t*)bsearch(&key, lumpinfo, nTexLumps, sizeof(key), lump_sorter);
	if (found)
	{
		TEX_SelectLevelWad(found->iTexFile);
		DC_fseek(texfiles[found->iTexFile], found->lump.filepos, SEEK_SET);
		SafeRead(texfiles[found->iTexFile], dest, found->lump.disksize);
		return found->lump.disksize;
	}

	Con_SafePrintf("WARNING: texture lump \"%s\" not found\n", name);
	return 0;
}

__inline int FindMiptex( char* name )
{
	int			i;

	for (i = 0; i < nummiptex; i++)
	{
		if (!Q_strcasecmp(name, miptex[i]))
			return i;
	}

	if (nummiptex == MAX_MAP_TEXTURES)
		Sys_Error("Exceeded MAX_MAP_TEXTURES");
	strcpy(miptex[i], name);
	nummiptex++;
	return i;
}

/*
==================
TEX_AddAnimatingTextures
==================
*/
void TEX_AddAnimatingTextures( void )
{
	int			base;
	int			i, j, k;
	char		name[32];

	base = nummiptex;

	for (i = 0; i < base; i++)
	{
		if (miptex[i][0] != '+' && miptex[i][0] != '-')
			continue;
		strcpy(name, miptex[i]);

		for (j = 0; j < 20; j++)
		{
			if (j < 10)
				name[1] = '0' + j;
			else
				name[1] = 'A' + j - 10;		// alternate animation


			// see if this name exists in the wadfile
			for (k = 0; k < nTexLumps; k++)
			{
				if (!strcmp(name, lumpinfo[k].lump.name))
				{
					FindMiptex(name);	// add to the miptex list
					break;
				}
			}
		}
	}
}

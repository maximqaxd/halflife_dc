// textures.c

#include "quakedef.h"
#include "textures.h"
#include <windows.h>

#define TEX_MAX_WADS			128
#define TEX_MAX_MIPTEX_NAME		64
#define TEX_MAX_PER_LEVEL_WADS	7

int nummiptex = 0;
char miptex[MAX_MAP_TEXTURES][TEX_MAX_MIPTEX_NAME];

// Lump data
typedef struct
{
	lumpinfo_t	lump;
	int			iTexFile;	// index of the wad this texture is located in
} texlumpinfo_t;

typedef struct
{
	int filepos;
	int filelen;
	int handle;
} texfile_t;

texfile_t texfiles[TEX_MAX_WADS];
int nTexFiles = 0;
texlumpinfo_t* lumpinfo = NULL;
int nTexLumps = 0;

void SafeRead( texfile_t* f, void* buffer, int count )
{
	if (Sys_FileRead(f->handle, buffer, count) != count)
		Sys_Error("File read failure");
}

void CleanupName( char* in, char* out )
{
	int		i;

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
TEX_BuildPerMapWadPath
=================
*/
qboolean TEX_BuildPerMapWadPath( const char* mapPath, char* outPath )
{
	int i, h[3], wadLen, outLen;
	int foundSmallWad = 0;
	char mapPathLocal[MAX_PATH];
	char mapName[MAX_PATH];
	char wadProbe[MAX_PATH];
	char wadToken[MAX_PATH];

	if (!outPath)
		return FALSE;

	outPath[0] = '\0';

	if (!mapPath || !mapPath[0])
		return FALSE;

	strncpy(mapPathLocal, mapPath, sizeof(mapPathLocal) - 1);
	mapPathLocal[sizeof(mapPathLocal) - 1] = '\0';
	ForwardSlashes(mapPathLocal);
	COM_FileBase(mapPathLocal, mapName);

	Con_Printf("Looking for small per level wads\n");
	for (i = 1; i <= TEX_MAX_PER_LEVEL_WADS; i++)
	{
		sprintf(wadProbe, "%s/%d_%s.wad", com_gamedir, i, mapName);
		h[2] = -1;
		wadLen = COM_OpenFile(wadProbe, h);
		/* Stop at first missing file; max 7 per-level wads */
		if (h[2] == -1 || wadLen <= 0)
			break;

		COM_CloseFile(h[0], h[1], h[2]);
		sprintf(wadToken, "%d_%s.wad", i, mapName);

		outLen = strlen(outPath);
		if (outLen + (int)strlen(wadToken) + 2 >= MAX_OSPATH)
			break;
		if (outLen > 0)
			strcat(outPath, ";");
		strcat(outPath, wadToken);
		foundSmallWad++;
	}

	if (foundSmallWad > 0)
		return TRUE;

	Con_Printf("Looking for monolithic per level wad\n");
	sprintf(wadProbe, "%s/%s", com_gamedir, mapName);
	COM_DefaultExtension(wadProbe, ".wad");
	wadLen = COM_OpenFile(wadProbe, h);
	(void)wadLen;
	if (h[2] != -1)
	{
		COM_CloseFile(h[0], h[1], h[2]);
		strncpy(outPath, wadProbe, MAX_OSPATH - 1);
		outPath[MAX_OSPATH - 1] = '\0';
		return TRUE;
	}

	sprintf(wadProbe, "valve/%s", mapName);
	COM_DefaultExtension(wadProbe, ".wad");
	wadLen = COM_OpenFile(wadProbe, h);
	(void)wadLen;
	if (h[2] != -1)
	{
		COM_CloseFile(h[0], h[1], h[2]);
		strncpy(outPath, wadProbe, MAX_OSPATH - 1);
		outPath[MAX_OSPATH - 1] = '\0';
		return TRUE;
	}

	return FALSE;
}

/*
=================
TEX_InitFromWad
=================
*/
qboolean TEX_InitFromWad( char* path )
{
	int			i;
	int			loadedWads = 0;
	wadinfo_t	wadinfo;
	char		szTmpPath[1024];
	char* pszWadFile;

	strcpy(szTmpPath, path); 

	// temporary kludge so we don't have to deal with no occurances of a semicolon
	//  in the path name ..
	if (strchr(szTmpPath, ';') == NULL)
		strcat(szTmpPath, ";");

	pszWadFile = strtok(szTmpPath, ";");

	while (pszWadFile)
	{
		texfile_t texfile;
		int h[3];
		int wadLen;
		char wadPath[MAX_PATH];
		char wadName[MAX_PATH];
		char wadSearch[MAX_PATH];

		/* Skip empty tokens (e.g. from trailing ";") */
		if (!pszWadFile[0])
		{
			pszWadFile = strtok(NULL, ";");
			continue;
		}

		ForwardSlashes(pszWadFile);

		COM_FileBase(pszWadFile, wadName);
		strcpy(wadSearch, wadName);
		COM_DefaultExtension(wadSearch, ".wad");
		wadLen = COM_OpenFile(wadSearch, h);
		strcpy(wadPath, wadSearch);
		if (h[2] == -1)
		{
			// Try the full token from the BSP WAD list as a secondary lookup.
			strcpy(wadSearch, pszWadFile);
			COM_DefaultExtension(wadSearch, ".wad");
			wadLen = COM_OpenFile(wadSearch, h);
			strcpy(wadPath, wadSearch);
			if (h[2] == -1)
			{
				Con_SafePrintf("WARNING: couldn't open %s\n", wadPath);
				pszWadFile = strtok(NULL, ";");
				continue;
			}
		}

		texfile.filepos = h[0];
		texfile.filelen = h[1];
		texfile.handle = h[2];
		texfiles[nTexFiles] = texfile;
		nTexFiles++;
		loadedWads++;

		Con_SafePrintf("Using WAD File: %s\n", wadPath);

		COM_FileSeek(texfile.filepos, texfile.filelen, texfile.handle, 0);
		SafeRead(&texfile, &wadinfo, sizeof(wadinfo));
		if (strncmp(wadinfo.identification, "WAD2", 4) &&
			strncmp(wadinfo.identification, "WAD3", 4))
			Sys_Error("TEX_InitFromWad: %s isn't a wadfile", wadPath);

		wadinfo.numlumps = LittleLong(wadinfo.numlumps);
		wadinfo.infotableofs = LittleLong(wadinfo.infotableofs);
		COM_FileSeek(texfile.filepos, texfile.filelen, texfile.handle, wadinfo.infotableofs);
		lumpinfo = (texlumpinfo_t*)realloc(lumpinfo, sizeof(texlumpinfo_t) * (nTexLumps + wadinfo.numlumps));

		for (i = 0; i < wadinfo.numlumps; i++)
		{
			SafeRead(&texfile, &lumpinfo[nTexLumps], sizeof(lumpinfo_t));
			CleanupName(lumpinfo[nTexLumps].lump.name, lumpinfo[nTexLumps].lump.name);

			lumpinfo[nTexLumps].lump.filepos = LittleLong(lumpinfo[nTexLumps].lump.filepos);
			lumpinfo[nTexLumps].lump.disksize = LittleLong(lumpinfo[nTexLumps].lump.disksize);
			lumpinfo[nTexLumps].iTexFile = nTexFiles - 1;

			nTexLumps++;
		}

		// next wad file
		pszWadFile = strtok(NULL, ";");
	}

	if (nTexLumps > 0)
		qsort(lumpinfo, nTexLumps, sizeof(texlumpinfo_t), lump_sorter);

	return loadedWads > 0 ? TRUE : FALSE;
}

/*
=================
TEX_CleanupWadInfo
=================
*/
void TEX_CleanupWadInfo( void )
{
	int i;

	if (lumpinfo)
	{
		free(lumpinfo);
		lumpinfo = NULL;
	}

	for (i = 0; i < nTexFiles; i++)
	{
		COM_CloseFile(texfiles[i].filepos, texfiles[i].filelen, texfiles[i].handle);
		texfiles[i].handle = -1;
		texfiles[i].filepos = 0;
		texfiles[i].filelen = 0;
	}

	nTexLumps = 0;
	nTexFiles = 0;
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
		COM_FileSeek(texfiles[found->iTexFile].filepos, texfiles[found->iTexFile].filelen, texfiles[found->iTexFile].handle, found->lump.filepos);
		SafeRead(&texfiles[found->iTexFile], dest, found->lump.disksize);
		return found->lump.disksize;
	}

	Con_SafePrintf("WARNING: texture lump \"%s\" not found\n", name);
	return 0;
}

int FindMiptex( char* name )
{
	int		i;

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
	int		base;
	int		i, j, k;
	char	name[32];

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
				if (!strcmp(name, lumpinfo[k].lump.name))
				{
					FindMiptex(name);	// add to the miptex list
					break;
				}
		}
	}

	if (nummiptex != base)
		Con_SafePrintf("added %i texture frames\n", nummiptex - base);
}

/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char	**ppszFiles = NULL;
int		nFiles = 0;
int		nMaxFiles = 0;

int
string_comparator( const void *string1, const void *string2 )
{
	char	*s1 = *(char **)string1;
	char	*s2 = *(char **)string2;
	return strcmp( s1, s2 );
}

void PrintUsage(char *pname)
{
	printf("\n\tusage:%s <source directory> <wadfile name> <script name> \n",pname);
	printf("\t      %s -bsp <map.bsp> <source directory> <output directory> [-maxsize n]\n\n",pname);
	printf("\t%s.exe is used to generate a bitmap name sorted 'qlumpy script'.\n",pname);
	printf("\tThe -bsp form writes one script per level wad, holding only the\n");
	printf("\ttextures that level actually uses.\n");
}


//=============================================================================
//
// Level wads
//
// Only one level wad is held in memory at a time, so a level's textures are
// cut into pieces that fit and the engine pulls in whichever piece holds the
// texture it is looking for.  The pieces follow the order the textures appear
// in the bsp, which keeps textures used near each other in the same wad.
//
//=============================================================================

#define LEVEL_WAD_BUDGET	(128 * 1024)
#define LUMP_TEXTURES		2

int		pvrmaxsize = 128;		// world textures don't get any larger than this

// what the same texture will come to once it is a Dreamcast texture image
int NearestSize( int value )
{
	int		size;

	for (size = 8; size < 1024; size = size * 2)
	{
		if (value < size + size / 2)
			break;
	}

	if (size > pvrmaxsize)
		size = pvrmaxsize;

	return size;
}

int EncodedSize( int width, int height )
{
	int		size;
	int		level;

	if (width != height || width < 64)
	{
		// too small or too odd a shape to be worth quantizing
		size = 32 + width * height * 2;
	}
	else
	{
		size = 32 + 2048;
		for (level = 1; level <= width; level = level * 2)
		{
			int blocks = (level / 2) * (level / 2);
			size += blocks < 1 ? 1 : blocks;
		}
	}

	return (size + 31) & ~31;
}

// Only square textures can be quantized, so a lopsided one is worth squaring
// up whenever the quantized copy comes out smaller than the flat one would.
void ChooseSize( int srcwidth, int srcheight, int *pwidth, int *pheight )
{
	int		width, height, square;

	width = NearestSize( srcwidth );
	height = NearestSize( srcheight );
	square = width > height ? width : height;

	if (EncodedSize( square, square ) < EncodedSize( width, height ))
		width = height = square;

	*pwidth = width;
	*pheight = height;
}

byte *LoadWholeFile( const char *name, int *plength )
{
	FILE	*f;
	byte	*buffer;
	int		length;

	f = fopen( name, "rb" );
	if (!f)
		return NULL;

	fseek( f, 0, SEEK_END );
	length = ftell( f );
	fseek( f, 0, SEEK_SET );

	buffer = (byte *)malloc( length );
	fread( buffer, 1, length, f );
	fclose( f );

	*plength = length;
	return buffer;
}

// Pulls the texture names out of a bsp, in the order the map stores them.
int LoadMapTextures( const char *bspname, char names[][32], int maxnames )
{
	byte	*bsp;
	int		length;
	int		lumpofs, nummiptex;
	int		count, i;

	bsp = LoadWholeFile( bspname, &length );
	if (!bsp)
	{
		printf("\n---------- ERROR ------------------\n");
		printf(" Could not open the map: %s\n", bspname);
		exit(EXIT_FAILURE);
	}

	lumpofs = *(int *)(bsp + 4 + LUMP_TEXTURES * 8);
	nummiptex = *(int *)(bsp + lumpofs);

	count = 0;
	for (i = 0; i < nummiptex && count < maxnames; i++)
	{
		int dataofs = *(int *)(bsp + lumpofs + 4 + i * 4);
		if (dataofs < 0)
			continue;

		strncpy( names[count], (char *)(bsp + lumpofs + dataofs), 16 );
		names[count][16] = 0;
		strupr( names[count] );
		count++;
	}

	free( bsp );
	return count;
}

int MakeLevelWadScripts( char *bspname, char *srcdir, char *outdir )
{
	static char	names[4096][32];
	char		mapname[MAX_PATH];
	char		bmppath[MAX_PATH];
	char		scriptpath[MAX_PATH];
	char		*p;
	FILE		*script;
	int			sizes[4096];
	int			order[4096];
	int			numtextures;
	int			first, last, wad;
	int			width, height;
	int			total, i, j;

	// the wads are named after the map, not after the bsp's path
	strcpy( mapname, bspname );
	p = strrchr( mapname, '\\' );
	if (!p)
		p = strrchr( mapname, '/' );
	if (p)
		memmove( mapname, p + 1, strlen(p) );
	p = strrchr( mapname, '.' );
	if (p)
		*p = 0;
	strupr( mapname );

	numtextures = LoadMapTextures( bspname, names, 4096 );
	printf( "%s uses %d textures\n", mapname, numtextures );

	// work out how big each one lands up, so the wads can be filled to the
	// budget without encoding anything twice
	for (i = 0; i < numtextures; i++)
	{
		BITMAPFILEHEADER	filehdr;
		BITMAPINFOHEADER	infohdr;
		FILE				*bmp;

		sprintf( bmppath, "%s\\%s.BMP", srcdir, names[i] );
		bmp = fopen( bmppath, "rb" );
		if (!bmp)
		{
			printf( "  missing %s.BMP, skipping\n", names[i] );
			sizes[i] = 0;
			continue;
		}

		fread( &filehdr, sizeof(filehdr), 1, bmp );
		fread( &infohdr, sizeof(infohdr), 1, bmp );
		fclose( bmp );

		ChooseSize( infohdr.biWidth, abs( infohdr.biHeight ), &width, &height );
		sizes[i] = EncodedSize( width, height ) + 32;	// plus its directory entry
	}

	wad = 0;
	first = 0;
	while (first < numtextures)
	{
		total = 0;
		for (last = first; last < numtextures; last++)
		{
			total += sizes[last];
			if (total >= LEVEL_WAD_BUDGET)
			{
				last++;
				break;
			}
		}

		// qlumpy wants the lumps sorted, but the split stays in map order
		for (i = first; i < last; i++)
			order[i] = i;
		for (i = first; i < last; i++)
		{
			for (j = i + 1; j < last; j++)
			{
				if (strcmp( names[order[j]], names[order[i]] ) < 0)
				{
					int swap = order[i];
					order[i] = order[j];
					order[j] = swap;
				}
			}
		}

		wad++;
		sprintf( scriptpath, "%s\\%d_%s.LS", outdir, wad, mapname );
		script = fopen( scriptpath, "wb" );
		if (!script)
		{
			printf("\n---------- ERROR ------------------\n");
			printf(" Could not open the script file: %s\n", scriptpath);
			exit(EXIT_FAILURE);
		}

		fprintf( script, "$DEST    \"%d_%s.WAD\"\r\n", wad, mapname );
		fprintf( script, "$PVRMAXSIZE %d\r\n\r\n", pvrmaxsize );
		for (i = first; i < last; i++)
		{
			if (!sizes[order[i]])
				continue;
			fprintf( script, "$loadbmp    \"%s\\%s.BMP\"\r\n", srcdir, names[order[i]] );
			fprintf( script, "%s  pvr -1 -1 -1 -1\r\n\r\n", names[order[i]] );
		}
		fclose( script );

		printf( "  %d_%s.LS: %d textures, about %d bytes\n", wad, mapname, last - first, total );
		first = last;
	}

	return 0;
}

int main(int argc, void **argv)
{
	char *pszdir;
	char *pszWadName;
	char *pszScriptName;
	char szBuf[1024];
	HANDLE hFile, hScriptFile;
	WIN32_FIND_DATA FindData;
	BOOL fWrite;
	BOOL fContinue = TRUE;
	DWORD dwWritten;

	printf("makels Copyright (c) 1998 Valve L.L.C., %s\n", __DATE__ );

	pszdir = (char *)argv[1];

	if (argc >= 5 && !stricmp( pszdir, "-bsp" ))
	{
		if (argc >= 7 && !stricmp( (char *)argv[5], "-maxsize" ))
			pvrmaxsize = atoi( (char *)argv[6] );
		return MakeLevelWadScripts( (char *)argv[2], (char *)argv[3], (char *)argv[4] );
	}

	if ((argc != 4) || (pszdir[0] == '/') || (pszdir[0] == '-'))
	{
		PrintUsage((char *)argv[0]);
		exit(1);
	}

	pszdir = (char *)malloc(strlen((char *)argv[1]) + 7);
	strcpy(pszdir, (char *)argv[1]);
	strcat(pszdir, "\\*.bmp");

	pszWadName = (char *)malloc(strlen((char *)argv[2]) + 5);
	strcpy(pszWadName, (char *)argv[2]);
	strcat(pszWadName, ".WAD");

	pszScriptName = (char *)malloc(strlen((char *)argv[3]));
	strcpy(pszScriptName, (char *)argv[3]);
	hScriptFile = CreateFile(pszScriptName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, NULL);

	if (hScriptFile == INVALID_HANDLE_VALUE)
	{
		printf("\n---------- ERROR ------------------\n");
		printf(" Could not open the script file: %s\n", pszScriptName);
		Beep(800,500);
		exit(EXIT_FAILURE);
	}

	sprintf(szBuf, "$DEST    \"%s\"\r\n\r\n", pszWadName);
	fWrite = WriteFile(hScriptFile, szBuf, strlen(szBuf), &dwWritten, NULL);
	if (!fWrite || (dwWritten != strlen(szBuf)))
	{
write_error:
		printf("\n---------- ERROR ------------------\n");
		printf(" Could not write to the script file: %s\n", pszScriptName);
		Beep(800,500);
		CloseHandle(hScriptFile);
		exit(EXIT_FAILURE);
	}
	
	
	hFile = FindFirstFile(pszdir, &FindData);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		while (fContinue)
		{
			if (!(FindData.dwFileAttributes &
					(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN)))
			{
				char szShort[MAX_PATH];

				// ignore N_ and F_ files
				strcpy(szShort, FindData.cFileName);
				strupr(szShort);

				if ((szShort[1] == '_') && ((szShort[0] == 'N') || (szShort[0] == 'F')))
				{

					printf("Skipping %s.\n", FindData.cFileName);

				} else {
				
					if ( nFiles >= nMaxFiles )
					{
						nMaxFiles += 1000;
						ppszFiles = (char **)realloc( ppszFiles, nMaxFiles * sizeof(*ppszFiles) );
						if ( !ppszFiles )
						{
							printf("\n---------- ERROR ------------------\n");
							printf(" Could not realloc more filename pointer storage\n");
							Beep(800,500);
							exit(EXIT_FAILURE);
						}
					}
					ppszFiles[nFiles++] = strdup( szShort );
				}
			}
			fContinue = FindNextFile(hFile, &FindData);
		}	
	}


	if (nFiles > 0)
	{
		qsort( ppszFiles, nFiles, sizeof(char*), string_comparator );

		for( int i = 0; i < nFiles; i++ )
		{
			char *p;
			char szShort[MAX_PATH];
			char szFull[MAX_PATH];

			strcpy(szShort, pszdir);
			p = strchr(szShort, '*');
			*p = '\0';
			strcat(szShort, ppszFiles[i]);
			GetFullPathName(szShort, MAX_PATH, szFull, NULL);

			sprintf(szBuf, "$loadbmp    \"%s\"\r\n", szFull);
			fWrite = WriteFile(hScriptFile, szBuf, strlen(szBuf), &dwWritten, NULL);
			if (!fWrite || (dwWritten != strlen(szBuf)))
				goto write_error;


			p = strchr(ppszFiles[i], '.');
			*p = '\0';

			sprintf(szBuf, "%s  miptex -1 -1 -1 -1\r\n\r\n", ppszFiles[i]);
			fWrite = WriteFile(hScriptFile, szBuf, strlen(szBuf), &dwWritten, NULL);
			if (!fWrite || (dwWritten != strlen(szBuf)))
				goto write_error;

			free( ppszFiles[i] );
		}
	}
	
	printf("Processed %d files specified by %s\n", nFiles, pszdir );

	CloseHandle(hScriptFile);
	free(pszdir);
	exit(0);
	return 0;
}
// decals.c -- wad-backed decal and cached-picture management

#include "quakedef.h"
#include "winquake.h"
#include "cl_draw.h"
#include "decal.h"
#include "wad.h"

typedef unsigned char byte;

cachewad_t	*decal_wad;
cachewad_t	custom_wad;
cachewad_t	*menu_wad;

char		decal_names[MAX_BASE_DECALS][16];

short		m_bDrawInitialized;

short		custom_decal;
char		custom_decal_name[16];

extern int DC_LoadTexture( char *identifier, int texture_type, int width, int height, void *data, short mipmap, int tex_type, unsigned char *pPal );
extern int DC_FreeTextureByName( char *name );

void Draw_FreeWad( cachewad_t* pWad );
void Draw_CacheWadInitFromFile( int *h, int len, char *name, int cacheMax, cachewad_t *wad );

extern cvar_t violence_hblood;

typedef struct lumplist_s
{
	lumpinfo_t			*lump;
	qboolean			breplaced;
	struct lumplist_s	*next;
} lumplist_t;

void Draw_Shutdown( void )
{
	if (m_bDrawInitialized)
	{
		m_bDrawInitialized = FALSE;
		Draw_FreeWad(menu_wad);
		menu_wad = NULL;
	}
}

void Draw_DecalShutdown( void )
{
	Draw_FreeWad(decal_wad);
	decal_wad = NULL;
}

void Draw_CacheWadInitFromFile( int *h, int len, char *name, int cacheMax, cachewad_t *wad )
{
	lumpinfo_t* lump_p;
	wadinfo_t header;
	int		i;

	Sys_FileRead(h[2], &header, sizeof(header));

	if (header.identification[0] != 'W'
	  || header.identification[1] != 'A'
	  || header.identification[2] != 'D'
	  || header.identification[3] != '3')
	{
		Sys_Error("Wad file %s doesn't have WAD3 id\n", name);
	}

	wad->lumps = (lumpinfo_t*)MnemoAllocDbg(len - header.infotableofs, __FILE__, __LINE__);
	COM_FileSeek(h[0], h[1], h[2], header.infotableofs);
	Sys_FileRead(h[2], wad->lumps, len - header.infotableofs);

	for (i = 0, lump_p = wad->lumps; i < header.numlumps; i++, lump_p++)
	{
		W_CleanupName(lump_p->name, lump_p->name);
	}

	wad->lumpCount = header.numlumps;
	wad->cacheCount = 0;
	wad->cacheMax = cacheMax;
	wad->name = name;
	wad->cache = (cacheentry_t*)MnemoAllocDbg(cacheMax * sizeof(cacheentry_t), __FILE__, __LINE__);
	memset(wad->cache, 0, cacheMax * sizeof(cacheentry_t));
	wad->tempWad = FALSE;
	wad->pfnCacheBuild = NULL;
	wad->cacheExtra = 0;
}

void Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad )
{
	int		h[3];
	int		nFileSize;

	nFileSize = COM_OpenFile(name, h);
	if (h[2] == -1)
		Sys_Error("Draw_LoadWad: Couldn't open %s\n", name);

	Draw_CacheWadInitFromFile(h, nFileSize, name, cacheMax, wad);

	COM_CloseFile(h[0], h[1], h[2]);
}

void Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize )
{
	wad->pfnCacheBuild = fn;
	wad->cacheExtra = extraDataSize;
}

void Draw_FreeWad( cachewad_t* pWad )
{
	int i;
	cacheentry_t* pic;

	if (!pWad)
		return;

	if (pWad->lumps)
	{
		MnemoFree(pWad->lumps);
		pWad->lumps = NULL;
	}

	if (pWad->numpaths)
	{
		for (i = 0; i < pWad->numpaths; i++)
		{
			MnemoFree(pWad->basedirs[i]);
			pWad->basedirs[i] = NULL;
		}

		MnemoFree(pWad->basedirs);
		pWad->basedirs = NULL;
	}

	if (pWad->lumppathindices)
	{
		MnemoFree(pWad->lumppathindices);
		pWad->lumppathindices = NULL;
	}

	if (pWad->cache)
	{
		if (!pWad->tempWad && pWad->cacheCount > 0)
		{
			for (i = 0, pic = pWad->cache; i < pWad->cacheCount; i++, pic++)
			{
				if (Cache_Check(&pic->cache))
					Cache_Free(&pic->cache, 0);
			}
		}

		MnemoFree(pWad->cache);
	}
}

void Draw_DecalSetName( int decal, char* name )
{
	if (decal < MAX_BASE_DECALS)
	{
		strncpy(decal_names[decal], name, sizeof(decal_names[0]) - 1);
		decal_names[decal][sizeof(decal_names[0]) - 1] = 0;
	}
}

int Draw_DecalIndex( int id )
{
	char	tmpName[32];
	char*	pName;

	pName = decal_names[id];
	if (!pName[0])
		Sys_Error("Used decal #%d without a name\n", id);

	/* Never draw human blood on a censored build. */
	if (!sv.active && violence_hblood.value == 0.0f && !strncmp(pName, "{blood", 6))
	{
		sprintf(tmpName, "{yblood%s", pName + 6);
		pName = tmpName;
	}

	return Draw_CacheIndex(decal_wad, pName);
}

int Draw_DecalSize( int number )
{
	if (!decal_wad)
		return 0;

	if (number >= decal_wad->lumpCount)
		return 0;

	return decal_wad->lumps[number].size;
}

texture_t* Draw_DecalTexture( int index )
{
	int		playernum;
	customization_t* pCust;

	// Player decal
	if (index < 0)
	{
		playernum = -1 - index;
		pCust = cl.players[playernum].customdata.pNext;

		if (!pCust || !pCust->bInUse || !pCust->pInfo || !pCust->pBuffer)
		{
			Sys_Error("Failed to load custom decal for player #%i:%s using default decal 0.\n", playernum, cl.players[playernum].name);
			return NULL;
		}

		return (texture_t*)Draw_CustomCacheGet((cachewad_t*)pCust->pInfo, pCust->pBuffer, pCust->nUserData1);
	}

	// Just a regular decal
	return (texture_t*)Draw_CacheGet(decal_wad, index);
}

// called from cl_parse.c
// find the server side decal id given it's name.
// used for save/restore
int Draw_DecalIndexFromName( char* name )
{
	char tmpName[16];
	int i;

	strcpy(tmpName, name);

	if (tmpName[0] == '}')
		tmpName[0] = '{';

	for (i = 0; i < MAX_BASE_DECALS; i++)
	{
		if (decal_names[i][0] && !strcmp(tmpName, decal_names[i]))
			return i;
	}

	return 0;
}

void Decal_ReplaceOrAppendLump( lumplist_t **ppList, lumpinfo_t *lump, qboolean bsecondlump )
{
	lumplist_t *p;

	for (p = *ppList; p != NULL; p = p->next)
	{
		if (!Q_strcasecmp(lump->name, p->lump->name))
		{
			MnemoFree(p->lump);
			p->lump = (lumpinfo_t *)MnemoAlloc(sizeof(lumpinfo_t), 0x20, 0, "decallump");
			memcpy(p->lump, lump, sizeof(lumpinfo_t));
			p->breplaced = bsecondlump;
			return;
		}
	}

	p = (lumplist_t *)MnemoAlloc(sizeof(lumplist_t), 0x20, 0, "decallist");
	memset(p, 0, sizeof(lumplist_t));
	p->lump = (lumpinfo_t *)MnemoAlloc(sizeof(lumpinfo_t), 0x20, 0, "decallump");
	memcpy(p->lump, lump, sizeof(lumpinfo_t));
	p->breplaced = bsecondlump;
	p->next = *ppList;
	*ppList = p;
}

/* Merge the lumps of a decal wad over the ones already loaded so later wads
   replace earlier ones by name, then rebuild decal_wad. */
void Decal_MergeInDecals( char *name, const char *pathID, cachewad_t *pwad )
{
	lumplist_t	*lumplist;
	lumplist_t	*p;
	lumplist_t	*next;
	cachewad_t	*newwad;
	int			i;
	int			lumpcount;

	lumplist = NULL;

	if (!pwad)
	{
		Sys_Error("Decal_MergeInDecals called with NULL wadfile\n");
		return;
	}

	if (!decal_wad)
	{
		// The first wad found becomes the base to merge onto.
		decal_wad = pwad;
		pwad->numpaths = 1;
		decal_wad->basedirs = (char **)MnemoAllocDbg(decal_wad->numpaths * sizeof(char *), __FILE__, __LINE__);
		decal_wad->basedirs[0] = _strdup(pathID);
		decal_wad->lumppathindices = (int *)MnemoAllocDbg(decal_wad->cacheMax * sizeof(int), __FILE__, __LINE__);
		memset(decal_wad->lumppathindices, 0, decal_wad->cacheMax * sizeof(int));
		return;
	}

	newwad = (cachewad_t *)MnemoAllocDbg(sizeof(cachewad_t), __FILE__, __LINE__);
	memset(newwad, 0, sizeof(cachewad_t));

	for (i = 0; i < decal_wad->lumpCount; i++)
		Decal_ReplaceOrAppendLump(&lumplist, &decal_wad->lumps[i], FALSE);

	for (i = 0; i < pwad->lumpCount; i++)
		Decal_ReplaceOrAppendLump(&lumplist, &pwad->lumps[i], TRUE);

	lumpcount = 0;
	for (p = lumplist; p != NULL; p = p->next)
		lumpcount++;

	newwad->lumpCount = lumpcount;
	newwad->cacheCount = 0;
	newwad->cacheMax = decal_wad->cacheMax;
	newwad->name = _strdup(decal_wad->name);
	newwad->cache = (cacheentry_t *)MnemoAllocDbg(newwad->cacheMax * sizeof(cacheentry_t), __FILE__, __LINE__);
	memset(newwad->cache, 0, newwad->cacheMax * sizeof(cacheentry_t));
	newwad->tempWad = 0;
	newwad->pfnCacheBuild = decal_wad->pfnCacheBuild;
	newwad->cacheExtra = decal_wad->cacheExtra;
	newwad->lumppathindices = (int *)MnemoAllocDbg(newwad->cacheMax * sizeof(int), __FILE__, __LINE__);
	memset(newwad->lumppathindices, 0, newwad->cacheMax * sizeof(int));
	newwad->numpaths = 2;
	newwad->basedirs = (char **)MnemoAllocDbg(newwad->numpaths * sizeof(char *), __FILE__, __LINE__);
	newwad->basedirs[0] = _strdup(decal_wad->basedirs[0]);
	newwad->basedirs[1] = _strdup(pathID);

	lumpcount = 0;
	for (p = lumplist; p != NULL; p = p->next)
		lumpcount++;

	newwad->lumps = (lumpinfo_t *)MnemoAllocDbg(lumpcount * sizeof(lumpinfo_t), __FILE__, __LINE__);

	for (i = 0, p = lumplist; p != NULL; p = next, i++)
	{
		next = p->next;
		memcpy(&newwad->lumps[i], p->lump, sizeof(lumpinfo_t));
		p->lump = NULL;
		newwad->lumppathindices[i] = (p->breplaced != 0);
		free(p);
	}

	lumplist = NULL;
	Draw_FreeWad(decal_wad);
	decal_wad = newwad;
}

// This is called to reset all loaded decals
// called from cl_parse.c and host.c
void Decal_Init( void )
{
	FileList_t*	fileList;
	FileList_t*	pfile;
	cachewad_t*	wad;
	int			i;

	fileList = NULL;

	Draw_FreeWad(decal_wad);
	decal_wad = NULL;

	if (COM_BuildFileList("decals.wad", &fileList) < 1)
	{
		Sys_Error("Couldn't find '%s' in search path\n", "decals.wad");
	}
	else
	{
		for (pfile = fileList; pfile; pfile = pfile->next)
		{
			wad = (cachewad_t *)MnemoAllocDbg(sizeof(cachewad_t), __FILE__, __LINE__);
			memset(wad, 0, sizeof(cachewad_t));
			Draw_CacheWadInitFromFile(pfile->handles, pfile->fileLen, "decals.wad", MAX_BASE_DECALS, wad);
			wad->pfnCacheBuild = Draw_MiptexTexture;
			wad->cacheExtra = MIP_EXTRASIZE;
			Decal_MergeInDecals("decals.wad", pfile->pathID, wad);
		}

		COM_CloseUnusedFiles(fileList);
		COM_DestroyMultipleFileList(&fileList);
	}

	sv_decalnamecount = decal_wad ? decal_wad->lumpCount : 0;
	if (MAX_BASE_DECALS < sv_decalnamecount)
		Sys_Error("Too many decals: %d / %d\n", sv_decalnamecount, MAX_BASE_DECALS);

	for (i = 0; i < sv_decalnamecount; i++)
	{
		char* name;

		memset(&sv_decalnames[i], 0, sizeof(decalname_t));

		if (decal_wad && i < decal_wad->lumpCount)
			name = decal_wad->lumps[i].name;
		else
			name = NULL;

		strncpy(sv_decalnames[i].name, name, sizeof(sv_decalnames[i].name) - 1);
	}
}

void* Draw_CacheGet( cachewad_t* wad, int index )
{
	cacheentry_t* pic;
	int i;
	void* dat = NULL;

	if (index >= wad->cacheCount)
		Sys_Error("Cache wad indexed before load %s: %d", wad->name, index);

	pic = &wad->cache[index];
	if (wad->tempWad || (dat = Cache_Check(&pic->cache)) == NULL)
	{
		char name[16];
		char clean[16];
		lumpinfo_t* pLump;

		COM_FileBase(pic->name, name);
		W_CleanupName(name, clean);

		for (i = 0, pLump = wad->lumps; i < wad->lumpCount; i++, pLump++)
		{
			if (!strcmp(clean, pLump->name))
				break;
		}

		if (!Draw_CacheReload(wad, i, pLump, pic, clean, pic->name))
			return NULL;

		dat = NULL;
		if (!((int)pic->cache.data & 1))
			dat = pic->cache.data;
		if (!dat)
			Sys_Error("Draw_CacheGet: failed to load %s", pic->name);
	}

	return dat;
}

void* Draw_CustomCacheGet( cachewad_t* wad, void* raw, int index )
{
	cacheentry_t* pic;
	void* dat;

	if (index >= wad->cacheCount)
		Sys_Error("Cache wad indexed before load %s: %d", wad->name, index);

	pic = &wad->cache[index];
	dat = Cache_Check(&pic->cache);
	if (dat == NULL)
	{
		char name[16];
		char clean[16];
		COM_FileBase(pic->name, name);
		W_CleanupName(name, clean);

		if (!Draw_CacheLoadFromCustom(clean, wad, raw, pic))
			return NULL;

		dat = NULL;
		if (!((int)pic->cache.data & 1))
			dat = pic->cache.data;
		if (!dat)
			Sys_Error("Draw_CacheGet: failed to load %s", pic->name);
	}

	return dat;
}

qboolean Draw_CacheReload( cachewad_t* wad, int index, lumpinfo_t* pLump, cacheentry_t* pic, char* clean, char* path )
{
	byte* buf;
	int		h[3];

	if (wad->numpaths == 2)
		COM_OpenFileByName(wad->basedirs[wad->lumppathindices[index]], wad->name, h);
	else
		COM_OpenFile(wad->name, h);

	if (h[2] == -1)
		return FALSE;

	if (wad->tempWad)
	{
		buf = (byte*)Hunk_TempAlloc(pLump->size + wad->cacheExtra + 1);
		pic->cache.data = buf;
	}
	else
	{
		buf = (byte*)Cache_Alloc(&pic->cache, pLump->size + wad->cacheExtra + 1, clean);
	}

	if (!buf)
		Sys_Error("Draw_CacheGet: not enough space for %s in %s", path, wad->name);

	buf[pLump->size + wad->cacheExtra] = 0;

	COM_FileSeek(h[0], h[1], h[2], pLump->filepos);
	Sys_FileRead(h[2], &buf[wad->cacheExtra], pLump->size);
	COM_CloseFile(h[0], h[1], h[2]);

	if (wad->pfnCacheBuild)
		wad->pfnCacheBuild(wad, buf);

	return TRUE;
}

qboolean Draw_CacheLoadFromCustom( char* clean, cachewad_t* wad, void* raw, cacheentry_t* pic )
{
	int		idx;
	byte* buf;
	lumpinfo_t* pLump;

	idx = 0;
	if (strlen(clean) < 5 || ((idx = atoi(clean + 3)) >= 0 && idx < wad->lumpCount))
	{
		pLump = &wad->lumps[idx];
		buf = (byte*)Cache_Alloc(&pic->cache, pLump->size + wad->cacheExtra + 1, clean);
		if (!buf)
			Sys_Error("Draw_CacheGet: not enough space for %s in %s", clean, wad->name);

		buf[pLump->size + wad->cacheExtra] = 0;

		memcpy(&buf[wad->cacheExtra], (char*)raw + pLump->filepos, pLump->size);

		custom_decal = 1;
		sprintf(custom_decal_name, "T%s", clean);
		custom_decal_name[6] = 0;

		if (wad->pfnCacheBuild)
			wad->pfnCacheBuild(wad, buf);

		custom_decal = 0;

		return TRUE;
	}

	return FALSE;
}

int Draw_CacheIndex( cachewad_t* wad, char* path )
{
	cacheentry_t* pic;
	int i;

	for (i = 0, pic = wad->cache; i < wad->cacheCount; i++, pic++)
	{
		if (!strcmp(path, pic->name))
			break;
	}

	if (i == wad->cacheCount)
	{
		if (wad->cacheCount == wad->cacheMax)
			Sys_Error("Cache wad (%s) out of %d entries", wad->name, wad->cacheMax);
		wad->cacheCount++;
		strcpy(pic->name, path);
	}
	return i;
}

/*
===============
Draw_MiptexTexture
===============
*/
void Draw_MiptexTexture( cachewad_t* wad, byte* data )
{
	texture_t* tex;
	miptex_t* mip, tmp;
	int			i, pix, paloffset, palettesize;
	byte* pal, * bitmap;

	if (wad->cacheExtra != MIP_EXTRASIZE)
		Sys_Error("Draw_MiptexTexture: Bad cached wad %s\n", wad->name);

	tex = (texture_t*)data;
	mip = (miptex_t*)(data + wad->cacheExtra);
	tmp = *mip;
	strcpy(tex->name, tmp.name);

	tex->width = LittleLong(tmp.width);
	tex->height = LittleLong(tmp.height);
	tex->anim_min = 0;
	tex->anim_max = 0;
	tex->anim_total = 0;
	tex->alternate_anims = NULL;
	tex->anim_next = NULL;

	for (i = 0; i < MIPLEVELS; i++)
		tex->offsets[i] = LittleLong(tmp.offsets[i]) + wad->cacheExtra;

	pix = tex->width * tex->height;
	palettesize = tex->offsets[0];
	paloffset = palettesize + pix + (pix >> 2) + (pix >> 4) + (pix >> 6) + 2;
	pal = (byte*)tex + paloffset;
	bitmap = (byte*)tex + palettesize;

	if (custom_decal)
		strcpy(tex->name, custom_decal_name);

	if (pal[765] == 0 && pal[766] == 0 && pal[767] == 255)
	{
		tex->name[0] = '{';
		tex->gl_texturenum = DC_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, TRUE, TEX_TYPE_ALPHA, pal);
	}
	else
	{
		tex->name[0] = '}';
		if (custom_decal)
			DC_FreeTextureByName(tex->name);
		tex->gl_texturenum = DC_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, TRUE, TEX_TYPE_ALPHA_GRADIENT, pal);
	}
}

void CustomDecal_Init( cachewad_t* wad, void* raw, int nFileSize )
{
	int i;

	Draw_CustomCacheWadInit(16, wad, raw, nFileSize);
	Draw_CacheWadHandler(wad, Draw_MiptexTexture, MIP_EXTRASIZE);

	for (i = 0; i < wad->lumpCount; i++)
	{
		Draw_CacheByIndex(wad, i);
	}
}

void Draw_CustomCacheWadInit( int cacheMax, cachewad_t* wad, void* raw, int nFileSize )
{
	lumpinfo_t* lump_p;
	wadinfo_t header;
	int		i;
	int		size;

	header = *(wadinfo_t*)raw;

	if (header.identification[0] != 'W'
	  || header.identification[1] != 'A'
	  || header.identification[2] != 'D'
	  || header.identification[3] != '3')
	{
		Sys_Error("Custom file doesn't have WAD3 id\n");
	}

	nFileSize -= header.infotableofs;
	wad->lumps = (lumpinfo_t*)MnemoAllocDbg(nFileSize, __FILE__, __LINE__);
	memcpy(wad->lumps, (char*)raw + header.infotableofs, nFileSize);

	for (i = 0, lump_p = wad->lumps; i < header.numlumps; i++, lump_p++)
	{
		W_CleanupName(lump_p->name, lump_p->name);
	}

	wad->lumpCount = header.numlumps;
	wad->cacheCount = 0;
	wad->cacheMax = cacheMax;
	wad->name = "pldecal.wad";
	size = cacheMax * sizeof(cacheentry_t);
	wad->cache = (cacheentry_t*)MnemoAllocDbg(size, __FILE__, __LINE__);
	memset(wad->cache, 0, size);
	wad->tempWad = FALSE;
	wad->pfnCacheBuild = NULL;
	wad->cacheExtra = 0;
}

int Draw_CacheByIndex( cachewad_t* wad, int nIndex )
{
	cacheentry_t* pic;
	int i;

	for (i = 0, pic = wad->cache; i < wad->cacheCount; i++, pic++)
	{
		if (atoi(pic->name) == nIndex)
			break;
	}

	if (i == wad->cacheCount)
	{
		if (wad->cacheCount == wad->cacheMax)
			Sys_Error("Cache wad (%s) out of %d entries", wad->name, wad->cacheMax);

		wad->cacheCount++;
		sprintf(pic->name, "%i", nIndex);
	}

	return i;
}

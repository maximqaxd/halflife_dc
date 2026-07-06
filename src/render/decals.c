// decals.c -- wad-backed decal and cached-picture management

#include "quakedef.h"
#include "winquake.h"
#include "cl_draw.h"
#include "decal.h"
#include "wad.h"

typedef unsigned char byte;

cachewad_t	decal_wad;
cachewad_t	custom_wad;
cachewad_t	menu_wad;

char		decal_names[MAX_BASE_DECALS][16];

static int	m_bDrawInitialized;

void Draw_FreeWad( cachewad_t* pWad );
void Draw_CacheWadInitFromFile( int h0, int h1, int h2, int len, char *name, int cacheMax, cachewad_t *wad );

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
		Draw_FreeWad(&menu_wad);
	}
}

void Draw_DecalShutdown( void )
{
	Draw_FreeWad(&decal_wad);
}

void Draw_CacheWadInitFromFile( int h0, int h1, int h2, int len, char *name, int cacheMax, cachewad_t *wad )
{
	lumpinfo_t* lump_p;
	wadinfo_t header;
	int		i;

	Sys_FileRead(h2, &header, sizeof(header));

	if (header.identification[0] != 'W'
	  || header.identification[1] != 'A'
	  || header.identification[2] != 'D'
	  || header.identification[3] != '3')
	{
		Sys_Error("Wad file %s doesn't have WAD3 id\n", name);
	}

	wad->lumps = (lumpinfo_t*)MnemoAlloc(len - header.infotableofs, 0x20, 0, "wadlumps");

	COM_FileSeek(h0, h1, h2, header.infotableofs);
	Sys_FileRead(h2, wad->lumps, len - header.infotableofs);

	for (i = 0, lump_p = wad->lumps; i < header.numlumps; i++, lump_p++)
	{
		W_CleanupName(lump_p->name, lump_p->name);
	}

	wad->name = name;
	wad->lumpCount = header.numlumps;
	wad->cacheCount = 0;
	wad->cacheMax = cacheMax;
	wad->cache = (cacheentry_t*)MnemoAlloc(sizeof(cacheentry_t) * cacheMax, 0x20, 0, "wadcache");
	memset(wad->cache, 0, sizeof(cacheentry_t) * cacheMax);
	wad->cacheExtra = 0;
	wad->pfnCacheBuild = NULL;

	wad->tempWad = FALSE;
}

void Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad )
{
	int		h[3];
	int		nFileSize;

	nFileSize = COM_OpenFile(name, h);
	if (h[2] == -1)
		Sys_Error("Draw_LoadWad: Couldn't open %s\n", name);

	Draw_CacheWadInitFromFile(h[0], h[1], h[2], nFileSize, name, cacheMax, wad);

	COM_CloseFile(h[0], h[1], h[2]);
}

void Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize )
{
	wad->cacheExtra = extraDataSize;
	wad->pfnCacheBuild = fn;
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

	if (pWad->cache)
	{
		if (!pWad->tempWad)
		{
			for (i = 0, pic = pWad->cache; i < pWad->cacheCount; i++, pic++)
			{
				if (Cache_Check(&pic->cache))
					Cache_Free(&pic->cache);
			}
		}

		MnemoFree(pWad->cache);
		pWad->cache = NULL;
	}
}

void Draw_DecalSetName( int decal, char* name )
{
	if (decal >= MAX_BASE_DECALS)
		return;

	strncpy(decal_names[decal], name, sizeof(decal_names[0]) - 1);
	decal_names[decal][sizeof(decal_names[0]) - 1] = 0;
}

int Draw_DecalIndex( int id )
{
	char	tmpName[32];
	char*	pName;

	pName = decal_names[id];
	if (!pName[0])
		Sys_Error("Used decal #%d without a name\n", id);

	/* Never draw human blood on a censored build. */
	if (!violence_hblood.value && !strncmp(pName, "{blood", 6))
	{
		sprintf(tmpName, "{yblood%s", pName + 6);
		pName = tmpName;
	}

	return Draw_CacheIndex(&decal_wad, pName);
}

int Draw_DecalSize( int number )
{
	if (number >= decal_wad.lumpCount)
		return 0;

	return decal_wad.lumps[number].size;
}

texture_t* Draw_DecalTexture( int index )
{
	int		playernum;
	customization_t* pCust;

	// Just a regular decal
	if (index >= 0)
		return (texture_t*)Draw_CacheGet(&decal_wad, index);

	// Player decal
	playernum = ~index;
	pCust = cl.players[playernum].customdata.pNext;
	if (pCust && pCust->bInUse)
	{
		cachewad_t* pWad;

		pWad = (cachewad_t*)pCust->pInfo;
		if (pWad && pCust->pBuffer)
			return (texture_t*)Draw_CustomCacheGet(pWad, pCust->pBuffer, pCust->nUserData1);
	}

	Sys_Error("Failed to load custom decal for player #%i:%s using default decal 0.\n", playernum, cl.players[playernum].name);
	return NULL;
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

static int Decal_CountLumps( lumplist_t *plist )
{
	int c = 0;
	lumplist_t *p = plist;

	while (p != NULL)
	{
		p = p->next;
		c++;
	}
	return c;
}

/* Merge the lumps of custom.wad over decals.wad so custom decals replace
   stock ones by name. */
void Decal_MergeInDecals( cachewad_t *pwad, const char *pathID )
{
	int i;
	int lumpcount;
	lumpinfo_t *lump;
	lumplist_t *lumplist;
	lumplist_t *p;
	lumplist_t *next;
	cachewad_t custom;

	if (!pwad)
		return;

	lumplist = NULL;

	for (i = 0, lump = pwad->lumps; i < pwad->lumpCount; i++, lump++)
		Decal_ReplaceOrAppendLump(&lumplist, lump, FALSE);

	memset(&custom, 0, sizeof(custom));
	Draw_CacheWadInit("custom.wad", MAX_BASE_DECALS, &custom);

	for (i = 0, lump = custom.lumps; i < custom.lumpCount; i++, lump++)
		Decal_ReplaceOrAppendLump(&lumplist, lump, TRUE);

	lumpcount = Decal_CountLumps(lumplist);

	MnemoFree(pwad->lumps);
	pwad->lumps = (lumpinfo_t *)MnemoAlloc(sizeof(lumpinfo_t) * lumpcount, 0x20, 0, "decallumps");

	for (i = 0, p = lumplist; p != NULL; p = p->next, i++)
		memcpy(&pwad->lumps[i], p->lump, sizeof(lumpinfo_t));

	pwad->lumpCount = lumpcount;

	for (p = lumplist; p != NULL; p = next)
	{
		next = p->next;
		MnemoFree(p->lump);
		MnemoFree(p);
	}

	Draw_FreeWad(&custom);
}

// This is called to reset all loaded decals
// called from cl_parse.c and host.c
void Decal_Init( void )
{
	int i;

	Draw_CacheWadInit("decals.wad", MAX_BASE_DECALS, &decal_wad);

	sv_decalnamecount = Draw_DecalCount();
	if (sv_decalnamecount > MAX_BASE_DECALS)
		Sys_Error("Too many decals: %d / %d\n", sv_decalnamecount, MAX_BASE_DECALS);

	for (i = 0; i < sv_decalnamecount; i++)
	{
		memset(&sv_decalnames[i], 0, sizeof(decalname_t));
		strncpy(sv_decalnames[i].name, Draw_DecalName(i), sizeof(sv_decalnames[i].name) - 1);
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

		if (i >= wad->lumpCount)
			return NULL;

		if (!Draw_CacheReload(wad, pLump, pic, clean, pic->name))
			return NULL;

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

		dat = pic->cache.data;
		if (!dat)
			Sys_Error("Draw_CacheGet: failed to load %s", pic->name);
	}

	return dat;
}

qboolean Draw_CacheReload( cachewad_t* wad, lumpinfo_t* pLump, cacheentry_t* pic, char* clean, char* path )
{
	byte* buf;
	int		h[3];

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

	idx = atoi(clean);
	if (idx < 0 || idx >= wad->lumpCount)
		return FALSE;

	pLump = &wad->lumps[idx];
	buf = (byte*)Cache_Alloc(&pic->cache, wad->cacheExtra + pLump->size + 1, clean);
	if (!buf)
		Sys_Error("Draw_CacheGet: not enough space for %s in %s", clean, wad->name);

	buf[pLump->size + wad->cacheExtra] = 0;

	memcpy(&buf[wad->cacheExtra], (char*)raw + pLump->filepos, pLump->size);

	if (wad->pfnCacheBuild)
		wad->pfnCacheBuild(wad, buf);

	return TRUE;
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

/* ---------------------------------------------------------------------------
 * Helpers still referenced from files that have not been reworked yet.
 * --------------------------------------------------------------------------- */

int Draw_DecalCount( void )
{
	return decal_wad.lumpCount;
}

char* Draw_DecalName( int number )
{
	if (number >= decal_wad.lumpCount)
		return 0;

	return decal_wad.lumps[number].name;
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

	if (pal[765] != 0 || pal[766] != 0 || pal[767] != 255)
	{
		tex->name[0] = '}';
		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, TRUE, TEX_TYPE_ALPHA_GRADIENT, pal);
	}
	else
	{
		tex->name[0] = '{';
		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, TRUE, TEX_TYPE_ALPHA, pal);
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

	header = *(wadinfo_t*)raw;

	if (header.identification[0] != 'W'
	  || header.identification[1] != 'A'
	  || header.identification[2] != 'D'
	  || header.identification[3] != '3')
	{
		Sys_Error("Custom file doesn't have WAD3 id\n");
	}

	wad->lumps = (lumpinfo_t*)MnemoAlloc(nFileSize - header.infotableofs, 0x20, 0, "customlumps");
	memcpy(wad->lumps, (char*)raw + header.infotableofs, nFileSize - header.infotableofs);

	for (i = 0, lump_p = wad->lumps; i < header.numlumps; i++, lump_p++)
	{
		W_CleanupName(lump_p->name, lump_p->name);
	}

	wad->name = "pldecal.wad";
	wad->lumpCount = header.numlumps;
	wad->cacheCount = 0;
	wad->cacheMax = cacheMax;
	wad->cache = (cacheentry_t*)MnemoAlloc(sizeof(cacheentry_t) * cacheMax, 0x20, 0, "customcache");
	memset(wad->cache, 0, sizeof(cacheentry_t) * cacheMax);
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

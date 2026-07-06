// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "winquake.h"
#include "cl_draw.h"
#include "decal.h"
#include "pr_cmds.h"
#include "wad.h"
#include "dc_accum.h"

typedef unsigned char byte;

cvar_t		gl_nobind = { "gl_nobind", "0" };
cvar_t		gl_max_size = { "gl_max_size", "256" };
cvar_t		gl_round_down = { "gl_round_down", "3" };
cvar_t		gl_picmip = { "gl_picmip", "0" };
cvar_t		gl_palette_tex = { "gl_palette_tex", "1" };

int		gTexLog = 0;

int		font_texture;
qfont_t* draw_creditsfont;
qfont_t* draw_chars;
qpic_t* draw_disc;
qpic_t* draw_backtile;

int			translate_texture;
int			char_texture;

typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;

byte		conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t* conback = (qpic_t*)&conback_buffer;

int		texels;

extern cachewad_t	decal_wad;
extern cachewad_t	custom_wad;
extern cachewad_t	menu_wad;

int			numgltextures;


#define DC_MAXTEXTURES   1140
typedef struct dc_texture_s
{
	short   iPalette;       /* +0x00 */
	short   nScaledWidth;   /* +0x02 */
	short   nScaledHeight;  /* +0x04 */
	short   nSrcWidth;      /* +0x06 */
	short   nSrcHeight;     /* +0x08 */
	short   nUsageCount;    /* +0x0a */
	short   nServerCount;   /* +0x0c, -1 = free */
	short   pad0e;          /* +0x0e */
	int     iTextureKey;    /* +0x10 */
	int     iTextureType;   /* +0x14 */
	void   *pddsSurface;    /* +0x18 */
	void   *pd3dtTexture;   /* +0x1c */
	char   *pszName;        /* +0x20 */
	int     cbData;         /* +0x24 */
	void   *pCacheBlock;    /* +0x28 */
	unsigned char *pbCacheData; /* +0x2c */
	int     bCached;        /* +0x30 */
	void   *pPrev;          /* +0x34 */
	void   *pNext;          /* +0x38 */
} dctexture_t;

/* Palette pool: DDraw palettes are a scarce resource, so paletted textures
   share from a small pool keyed by a tag of the palette bytes. */
typedef struct
{
	int					tag;			/* -1 = free */
	int					referenceCount;
	LPDIRECTDRAWPALETTE	lpPalette;
} dcpalette_t;

#define DC_MAXPALETTES	4

static dcpalette_t	gGLPalette[DC_MAXPALETTES];

static dctexture_t	s_texSlots[DC_MAXTEXTURES];
static dctexture_t	s_dcTextureLruHead;
static dctexture_t	s_dcTextureLruTail;
static int			s_nCached;
static LPDIRECTDRAWSURFACE4 s_pCurrentTextureSurface;
static int			s_slotServercount[DC_MAXTEXTURES];
static int			s_slotUse4444[DC_MAXTEXTURES];
static int			s_slotIsSystem[DC_MAXTEXTURES];  /* GLT_SYSTEM: never cache/decache */
static int			s_bTexReclaimGuard;
static int			s_bUncacheGuard;
static int			s_nD3dCurrentTexnum = -1;

extern void* Sys_GetDirectDraw4( void );
extern void* Sys_GetBackBuffer4( void );
extern void* Sys_GetPrimarySurface4( void );
extern void* Sys_GetD3DDevice3( void );

qpic_t* LoadTransBMP( char* pszName );
qpic_t* LoadTransPic( char* pszName, qpic_t* ppic );

/* Shared localized text/font renderer (dc_text.c). */
extern void	Font_SetScale( float sx, float sy );
extern int	Font_StringWidth( qfont_t* font, char* str );
extern int	Font_CharHeight( qfont_t* font );
extern int	Font_DrawCharI( qfont_t* font, int x, int y, int num );
extern void	DCV_SetTextRenderStates( void );
extern void	DCV_SetDefaultRenderStates( void );
void	DCV_SetHudDepth( float layer );

static LPDIRECTDRAWSURFACE4 DCV_CreateTextureSurface16(int w, int h, int color_fmt, int allow_sysmem_fallback);
void DC_FreeTextureSlot(dctexture_t *slot);
int DC_ReclaimTextureSlot( void );
static int DC_DecacheTextureSlot( dctexture_t *slot );
static int DC_CacheTextureToRam( dctexture_t *slot );
static void DCV_UnlinkTextureSlot( dctexture_t *slot );
static void DCV_LinkTextureSlotFront( dctexture_t *slot );
static void DCV_TouchTextureSlot( dctexture_t *slot );
static int DCV_AttachTextureInterface( dctexture_t *slot, LPDIRECTDRAWSURFACE4 surf );
int DC_FreeStaleTextureSlots( void );
int GL_PaletteTag( byte* pPal );
void GL_UnloadTextures( void );
void GL_BindStage( int texnum, int stage );

static void DCV_UnlinkTextureSlot( dctexture_t *slot )
{
	dctexture_t *prev;
	dctexture_t *next;

	if (!slot)
		return;

	prev = (dctexture_t *)slot->pPrev;
	next = (dctexture_t *)slot->pNext;

	if (prev)
		prev->pNext = next;

	if (next)
		next->pPrev = prev;

	slot->pPrev = NULL;
	slot->pNext = NULL;
}

static void DCV_LinkTextureSlotFront( dctexture_t *slot )
{
	dctexture_t *next;

	if (!slot)
		return;

	DCV_UnlinkTextureSlot(slot);

	next = (dctexture_t *)s_dcTextureLruHead.pNext;

	slot->pPrev = &s_dcTextureLruHead;
	slot->pNext = next;

	s_dcTextureLruHead.pNext = slot;

	if (next)
		next->pPrev = slot;
}

static void DCV_TouchTextureSlot( dctexture_t *slot )
{
	if (!slot || slot->nServerCount == -1)
		return;

	DCV_LinkTextureSlotFront(slot);
}

static int DCV_AttachTextureInterface( dctexture_t *slot, LPDIRECTDRAWSURFACE4 surf )
{
	LPDIRECT3DTEXTURE2 tex;

	if (!slot || !surf || !surf->lpVtbl || !surf->lpVtbl->QueryInterface)
		return 0;

	if (slot->pd3dtTexture)
	{
		((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
		slot->pd3dtTexture = NULL;
	}

	tex = NULL;

	if (FAILED(surf->lpVtbl->QueryInterface(surf, &IID_IDirect3DTexture2, (void **)&tex)))
		return 0;

	slot->pd3dtTexture = tex;
	return 1;
}

static void DCV_ClearCachedFlag( void )
{
	dctexture_t *s;

	for (s = (dctexture_t *)s_dcTextureLruHead.pNext;
	     s != NULL && s != &s_dcTextureLruTail;
	     s = (dctexture_t *)s->pNext)
		s->bCached = 0;
}

/* ---------------------------------------------------------------------------
 * Shared palette pool
 * --------------------------------------------------------------------------- */

int GL_PaletteEqual( dcpalette_t* entry, byte* pPal, int tag )
{
	PALETTEENTRY entries[256];
	LPDIRECTDRAWPALETTE pal;
	int i;

	if (tag != entry->tag)
		return 0;

	pal = entry->lpPalette;
	if (!pal)
		return 0;

	pal->lpVtbl->GetEntries(pal, 0, 0, 256, entries);

	for (i = 0; i < 256; i++)
	{
		if (pPal[i*3+0] != entries[i].peRed)
			return 0;
		if (pPal[i*3+1] != entries[i].peGreen)
			return 0;
		if (pPal[i*3+2] != entries[i].peBlue)
			return 0;
	}

	return 1;
}

static void DC_PaletteEntryClear( dcpalette_t* entry );
static void DC_PaletteEntryRelease( dcpalette_t* entry );

int DC_CreatePalette( dcpalette_t* entry, byte* pPal, int tag )
{
	PALETTEENTRY entries[256];
	LPDIRECTDRAW4 pDD4;
	LPDIRECTDRAWPALETTE pal;
	int i;

	pDD4 = (LPDIRECTDRAW4)Sys_GetDirectDraw4();
	if (!pDD4)
		return 0;

	DC_PaletteEntryRelease(entry);
	DC_PaletteEntryClear(entry);

	for (i = 0; i < 256; i++)
	{
		entries[i].peRed = pPal[i*3+0];
		entries[i].peGreen = pPal[i*3+1];
		entries[i].peBlue = pPal[i*3+2];
		entries[i].peFlags = 0;
	}

	pal = NULL;
	if (FAILED(pDD4->lpVtbl->CreatePalette(pDD4, DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, &pal, NULL)))
		return 0;

	entry->tag = tag;
	entry->referenceCount = 0;
	entry->lpPalette = pal;
	return 1;
}

int DC_GetPaletteIndex( byte* pPal )
{
	int tag;
	int i;

	tag = GL_PaletteTag(pPal);

	for (i = 0; i < DC_MAXPALETTES; i++)
	{
		if (gGLPalette[i].tag < 0)
		{
			if (!DC_CreatePalette(&gGLPalette[i], pPal, tag))
				return -1;

			gGLPalette[i].referenceCount++;
			return i;
		}

		if (GL_PaletteEqual(&gGLPalette[i], pPal, tag))
		{
			gGLPalette[i].referenceCount++;
			return i;
		}
	}

	return -1;
}

static void DC_ReleaseTextureSlot( dctexture_t *slot );

static void DC_CacheRamCallback( void )
{
	DC_ReleaseTextureSlot(&s_dcTextureLruHead);
}

void DC_FreeTextureSlot(dctexture_t *slot)
{
	int slot_index;
	LPDIRECT3DDEVICE3 dev;
	if (!slot) return;
	s_pCurrentTextureSurface = NULL;

	slot_index = (int)(slot - s_texSlots);

	if (slot_index >= 0 && slot_index < DC_MAXTEXTURES)
	{
		dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();
		if (dev && dev->lpVtbl && dev->lpVtbl->SetTexture)
		{
			if (currenttexture == slot_index)
			{
				dev->lpVtbl->SetTexture(dev, 0, NULL);
				currenttexture = -1;
				s_nD3dCurrentTexnum = -1;
			}

		}
		s_slotServercount[slot_index] = -1;
		s_slotIsSystem[slot_index] = 0;
	}
	DCV_UnlinkTextureSlot(slot);

	if (slot->pddsSurface)
	{
		((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Release((LPDIRECTDRAWSURFACE4)slot->pddsSurface);
		slot->pddsSurface = NULL;
	}
	if (slot->pd3dtTexture)
	 {
		((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
	}

	slot->pd3dtTexture = NULL;

	if (slot->pCacheBlock)
	 {
		MnemoFree(slot->pCacheBlock);

		slot->pCacheBlock = NULL;
		slot->pbCacheData = NULL;
		slot->bCached = 0;

		if (s_nCached > 0)
			s_nCached--;
	}

	if (slot->pszName)
	{
		MnemoFree(slot->pszName);
		slot->pszName = NULL;
	}

	slot->iTextureType = 0;
	slot->nServerCount = -1;
}

static int DC_CacheTextureToRam( dctexture_t *slot )
{
	DDSURFACEDESC2 *pdd;
	LPDIRECTDRAWSURFACE4 surf;
	int cbCopy;
	int total;
	DWORD dwSavedLinearSize;
	const char *dbgname;

	if (!slot || !slot->pddsSurface || slot->pCacheBlock || slot->pbCacheData || slot->cbData <= 0)
		return 0;

	dbgname = slot->pszName ? slot->pszName : "texture";

	if (gTexLog)
		Con_DPrintf("Attempting to cache texture %s\n", dbgname);

	if (slot->bCached)
		Sys_Error("Reattempted texture-to-RAM cache!\n");

	if (Mnemo_LastChanceActive())
		return 0;

	surf = (LPDIRECTDRAWSURFACE4)slot->pddsSurface;
	total = (int)sizeof(DDSURFACEDESC2) + slot->cbData;
	slot->pCacheBlock = MnemoAlloc(total, 0xA0, 0, dbgname);

	if (!slot->pCacheBlock)
		return 0;

	pdd = (DDSURFACEDESC2 *)slot->pCacheBlock;
	memset(pdd, 0, sizeof(*pdd));
	pdd->dwSize = sizeof(*pdd);

	if (FAILED(surf->lpVtbl->GetSurfaceDesc(surf, pdd)))
		goto fail_free;

	/* Save dwLinearSize from GetSurfaceDesc; DDLOCK_WAIT on WinCE PVR may overwrite
	   the lPitch/dwLinearSize union with the row pitch, losing the compressed size. */
	dwSavedLinearSize = pdd->dwLinearSize;

	if (FAILED(surf->lpVtbl->Lock(surf, NULL, pdd, DDLOCK_WAIT, NULL)))
	{
		if (gTexLog)
			Con_DPrintf("Couldn't lock surface to cache it.\n");
		goto fail_free;
	}

	/* Restore dwLinearSize in case Lock overwrote it via the lPitch union. */
	pdd->dwLinearSize = dwSavedLinearSize;

	cbCopy = slot->cbData;
	if (cbCopy < 0)
		cbCopy = 0;

	slot->pbCacheData = (unsigned char *)slot->pCacheBlock + sizeof(DDSURFACEDESC2);
	if (cbCopy > 0 && pdd->lpSurface)
		memcpy(slot->pbCacheData, pdd->lpSurface, (size_t)cbCopy);

	surf->lpVtbl->Unlock(surf, NULL);

	/* Strip DDSD_LPSURFACE and zero lpSurface in the stored descriptor.
	   DC_DecacheTextureSlot passes this exact DDSURFACEDESC2 to CreateSurface;
	   if DDSD_LPSURFACE is still set with the now-freed VRAM address, the
	   WinCE PVR driver will attempt to read/validate that stale address -> AV. */
	pdd->dwFlags &= ~DDSD_LPSURFACE;
	pdd->lpSurface = NULL;

	if (slot->pd3dtTexture)
	{
		((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
		slot->pd3dtTexture = NULL;
	}

	((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Release((LPDIRECTDRAWSURFACE4)slot->pddsSurface);
	slot->pddsSurface = NULL;

	DCV_UnlinkTextureSlot(slot);
	slot->bCached = 1;
	s_nCached++;

	if (gTexLog)
	{
		Con_DPrintf("Cached texture %s\n", dbgname);
		Con_DPrintf("Cached texture from video to main RAM.\n");
	}
	return 1;

fail_free:
	MnemoFree(slot->pCacheBlock);
	slot->pCacheBlock = NULL;
	slot->pbCacheData = NULL;
	return 0;
}

static int DC_DecacheTextureSlot( dctexture_t *slot )
{
	DDSURFACEDESC2 *pdd;
	LPDIRECTDRAWSURFACE4 surf;
	LPDIRECTDRAW4 pDD4;
	int cbCopy;
	const char *dbgname;

	if (!slot)
		return 0;

	if (slot->pddsSurface && slot->pbCacheData)
		Sys_Error("Cache data was non-null but so was surface.\n");

	if (slot->pddsSurface)
		return 1;

	if (!slot->pCacheBlock || !slot->pbCacheData)
		return 0;

	dbgname = slot->pszName ? slot->pszName : "texture";
	if (gTexLog)
		Con_DPrintf("Attempting to decache texture %s\n", dbgname);

	pDD4 = (LPDIRECTDRAW4)Sys_GetDirectDraw4();
	if (!pDD4)
		return 0;

	DCV_ClearCachedFlag();

	pdd = (DDSURFACEDESC2 *)slot->pCacheBlock;

	surf = NULL;
	for (;;)
	{
		if (!FAILED(pDD4->lpVtbl->CreateSurface(pDD4, pdd, &surf, NULL)))
			break;

		{
			int reclaimed = 0;

			if (!s_bTexReclaimGuard)
			{
				s_bTexReclaimGuard = 1;
				reclaimed = DC_ReclaimTextureSlot();
				s_bTexReclaimGuard = 0;
			}
			if (!reclaimed)
			{
				if (gTexLog)
					Con_DPrintf("Couldn't recreate decached surface.\n");
				return 0;
			}
		}
	}

	/* Reuse pCacheBlock as the Lock descriptor so pCacheBlock->lpSurface gets the
	   new locked VRAM pointer. */
	for (;;)
	{
		if (!FAILED(surf->lpVtbl->Lock(surf, NULL, pdd, DDLOCK_WAIT, NULL)))
			break;

		{
			int reclaimed = 0;
			if (!s_bTexReclaimGuard)
			{
				s_bTexReclaimGuard = 1;
				reclaimed = DC_ReclaimTextureSlot();
				s_bTexReclaimGuard = 0;
			}
			if (!reclaimed)
			{
				if (gTexLog)
					Con_DPrintf("Couldn't lock surface to decache it.\n");
				surf->lpVtbl->Release(surf);
				return 0;
			}
		}
	}

	cbCopy = slot->cbData;
	if (cbCopy < 0)
		cbCopy = 0;

	if (cbCopy > 0 && slot->pbCacheData && pdd->lpSurface)
		memcpy(pdd->lpSurface, slot->pbCacheData, (size_t)cbCopy);

	surf->lpVtbl->Unlock(surf, NULL);
	slot->pddsSurface = surf;

	if (!DCV_AttachTextureInterface(slot, surf))
	{
		surf->lpVtbl->Release(surf);
		slot->pddsSurface = NULL;
		return 0;
	}

	DCV_TouchTextureSlot(slot);

	MnemoFree(slot->pCacheBlock);
	slot->pCacheBlock = NULL;
	slot->pbCacheData = NULL;
	slot->bCached = 0;

	if (s_nCached > 0)
		s_nCached--;

	if (gTexLog)
	{
		Con_DPrintf("Decached texture %s\n", dbgname);
		Con_DPrintf("Decached texture back to VRAM.\n");
	}
	return 1;
}

static dctexture_t *DC_ClearTextureSlot( dctexture_t *slot )
{
	slot->pddsSurface = NULL;
	slot->pd3dtTexture = NULL;
	slot->pszName = NULL;
	slot->iPalette = -1;
	slot->nScaledWidth = 0;
	slot->nScaledHeight = 0;
	slot->nSrcWidth = 0;
	slot->nSrcHeight = 0;
	slot->nServerCount = -1;
	slot->pbCacheData = NULL;
	slot->pCacheBlock = NULL;
	slot->bCached = 0;
	slot->pPrev = NULL;
	slot->pNext = NULL;
	return slot;
}

static void DC_ReleaseTextureSlot( dctexture_t *slot )
{
	if (!slot)
		return;

	if (slot->pd3dtTexture)
	{
		((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
		slot->pd3dtTexture = NULL;
	}
}

static void DC_SetupTextureSlot( dctexture_t *slot, char *name, short palIndex, short scaledWidth, short scaledHeight, short srcWidth, short srcHeight )
{
	if (!slot)
		return;

	slot->iPalette = palIndex;
	slot->nScaledWidth = scaledWidth;
	slot->nScaledHeight = scaledHeight;
	slot->nSrcWidth = srcWidth;
	slot->nSrcHeight = srcHeight;

	slot->pszName = NULL;
	if (name && name[0])
	{
		slot->pszName = (char *)MnemoAlloc(strlen(name) + 1, 0x20, 0, "texname");
		if (slot->pszName)
			strcpy(slot->pszName, name);
	}

	DCV_LinkTextureSlotFront(slot);

	if (gTexLog)
		Con_DPrintf("Set up texture %s\n", slot->pszName ? slot->pszName : "<nameless>");
}

void DC_TexCache( void )
{
	int i;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		if (slot->nServerCount == -1)
			continue;
		if (slot->pbCacheData || s_slotIsSystem[i])
			continue;

		DC_CacheTextureToRam(slot);
	}
}

void DC_TexDump_f( void )
{
	int i, count, max_used, hi;

	count = 0;
	hi = -1;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		if (s_texSlots[i].nServerCount != -1)
		{
			count++;
			if (i > hi)
				hi = i;
		}
	}
	max_used = (hi >= 0) ? (hi + 1) : 0;

	Con_Printf("%d textures loaded, max %d\n", count, max_used);
}

int DC_FindTextureSlot(char *name, int width, int height, unsigned int key)
{
	int i;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		if (s_texSlots[i].nServerCount == -1)
			continue;

		if (key && (unsigned int)s_texSlots[i].iTextureKey == key)
			return i;

		if (s_texSlots[i].pszName && strcmp(name, s_texSlots[i].pszName) == 0)
		{
			if (s_texSlots[i].nSrcWidth == width && s_texSlots[i].nSrcHeight == height)
				return i;
		}
	}
	return -1;
}

int DCV_GetSlot( void )
{
	int i;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		if (s_texSlots[i].nServerCount == -1)
			return i;
	}

	Sys_Error("DCV_GetSlot: Too many textures!");
	return -1;
}

static const int pot_sizes_desc[] = { 1024, 512, 256, 128, 64, 32, 16, 8, 0 };

/* Scale w/h to power-of-two texture dimensions. Returns 0 if the result equals
   the requested size, 1 if the texture must be resampled. round_down forces the
   smaller axis to the larger axis' POT; round_up allows a one-level mip drop when
   the smaller axis is less than half the larger. */
int DCV_ComputeScaledSize(int tex_type, int *out_w, int *out_h, int w, int h, int round_down, int round_up, void *pvrt)
{
	int mx, mn, rmax, rmin, axis_min, orient;
	const int *p;

	orient = (w > h);
	mx = orient ? w : h;
	mn = (h > w) ? w : h;

	if (tex_type == TEX_TYPE_GBIX)
	{
		*out_w = *(short *)((byte *)pvrt + 0x1c);
		*out_h = *(short *)((byte *)pvrt + 0x1e);
		return (*out_w == w && *out_h == h) ? 0 : 1;
	}

	if (mx < 8) mx = 8;
	if (mn < 8) mn = 8;

	p = pot_sizes_desc;
	rmax = *p;
	while (rmax != 0 && mx < rmax)
		rmax = *++p;

	p = pot_sizes_desc;
	rmin = *p;
	while (rmin != 0 && mn < rmin)
		rmin = *++p;

	axis_min = rmin;
	if (round_down != 0)
		axis_min = rmax;
	else if (round_up != 0 && (rmax >> 1) > rmin)
		axis_min = rmax >> 1;

	if (rmax < 8) rmax = 8;
	if (axis_min < 8) axis_min = 8;

	if (!orient)
	{
		*out_w = axis_min;
		*out_h = rmax;
	}
	else
	{
		*out_w = rmax;
		*out_h = axis_min;
	}

	return (*out_w == w && *out_h == h) ? 0 : 1;
}

void DCV_BuildPalette1555(unsigned char *pPal, unsigned short *out_256)
{
	unsigned short *out = out_256;
	int i = 0;

	do {
		byte r = pPal[0], g = pPal[1], b = pPal[2];
		pPal += 3;
		*out++ = (unsigned short)(((r>>3)<<10) | 0x8000 | ((g>>3)<<5) | (b>>3));
	} while (++i < 256);

	/* Index 255 is the fully transparent entry. */
	out_256[255] = 0;
}

void DCV_BuildPalette565(unsigned char *pPal, unsigned short *out_256)
{
	int i;

	for (i = 0; i < 256; i++)
		out_256[i] = (unsigned short)(((pPal[i*3+0]>>3)<<11) | ((pPal[i*3+1]>>2)<<5) | (pPal[i*3+2]>>3));
}

void DCV_BuildAlphaGradient4444(unsigned char *pPal, unsigned short *out_256)
{
	unsigned short r4, g4, b4, rgb;
	int alpha_accum;
	int i;

	r4 = (unsigned short)(pPal[255*3+0] >> 4);
	g4 = (unsigned short)(pPal[255*3+1] >> 4);
	b4 = (unsigned short)(pPal[255*3+2] >> 4);
	rgb = (r4 << 8) | (g4 << 4) | b4;

	alpha_accum = 0;
	for (i = 0; i < 256; i++)
	{
		out_256[i] = (unsigned short)(((unsigned short)alpha_accum & 0xF000) | rgb);
		alpha_accum += 0x100;
	}
}

void DCV_BuildMaxChannelAlpha(unsigned char *pPal, unsigned short *out_256)
{
	int i = 0;

	do {
		int r = pPal[0];
		int g = pPal[1];
		int mx = r;
		int a;

		if (r <= g)
			mx = g;			/* mx = max(r, g) */

		a = pPal[2];			/* b */
		if (a < mx)
		{
			a = g;
			if (r > g)
				a = r;
		}
		/* a = max(r, g, b): white RGB, alpha from brightest channel. */
		*out_256++ = (unsigned short)(((a>>4)<<12) | 0x0FFF);
		pPal += 3;
	} while (++i < 256);
}

void DCV_BuildLumaRemap(unsigned char *pPal, byte *out_256)
{
	int i = 0;

	do {
		byte r = pPal[0], g = pPal[1], b = pPal[2];
		pPal += 3;
		*out_256++ = (byte)(((unsigned)r + g + b) / 3);
	} while (++i < 256);
}

int DC_UncacheOneTexture( void )
{
	dctexture_t *slot;

	GL_UnloadTextures();

	if (s_bUncacheGuard)
		return 0;

	s_bUncacheGuard = 1;

	if (s_nCached)
	{
		slot = (dctexture_t *)s_dcTextureLruTail.pPrev;

		while (slot && !slot->bCached)
			slot = (dctexture_t *)slot->pPrev;

		if (slot)
			DC_DecacheTextureSlot(slot);
	}

	s_bUncacheGuard = 0;
	return 0;
}

int DC_ReclaimTextureSlot( void )
{
	dctexture_t *slot;
	const char *dbgname;

	slot = (dctexture_t *)s_dcTextureLruTail.pPrev;

	for (;;)
	{
		if (slot == &s_dcTextureLruHead || slot == NULL)
			return GL_UnloadTextures(), DC_FreeStaleTextureSlots();

		if (gTexLog)
		{
			dbgname = slot->pszName ? slot->pszName : "<nameless>";
			Con_DPrintf("Walking texture %s\n", dbgname);
		}

		if (!slot->bCached && !slot->pbCacheData)
			break;

		slot = (dctexture_t *)slot->pPrev;
	}

	if (slot->nServerCount == 0 ||
	    gHostSpawnCount <= (int)slot->nServerCount ||
	    slot->nUsageCount != 0)
	{
		DC_CacheTextureToRam(slot);
	}
	else
	{
		DC_FreeTextureSlot(slot);
	}

	return 1;
}

/* Create a 16-bit texture surface. */
static LPDIRECTDRAWSURFACE4 DCV_CreateTextureSurface16(int w, int h, int color_fmt, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAW4 pDD4;
	LPDIRECTDRAWSURFACE4 pSurf = NULL;

	pDD4 = (LPDIRECTDRAW4)Sys_GetDirectDraw4();

	if (!pDD4)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwRGBBitCount = 16;

	if (color_fmt == PVR_ARGB4444)
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddsd.ddpfPixelFormat.dwRBitMask = 0x0f00;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x00f0;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x000f;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
	}
	else if (color_fmt == PVR_ARGB1555)
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddsd.ddpfPixelFormat.dwRBitMask = 0x7c00;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x03e0;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x001f;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
	}
	else /* RGB565 */
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x07E0;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000;
	}

	while (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
	{
		if (DC_ReclaimTextureSlot())
			continue;

		if (!allow_sysmem_fallback)
			return NULL;

		/* retry to create surface in system memory for selected texture classes. */
		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
		if (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
			return NULL;
	}

	return pSurf;
}

/* DCV_PrepSurfaceTrueColor: convert RGBA bytes to 565 or 4444 and lock/copy/unlock. */
void *DCV_PrepSurfaceTrueColor(int w, int h, byte *rgbax, int use_565, dctexture_t *slot, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	int i, n;

	pSurf = DCV_CreateTextureSurface16(w, h, !use_565, allow_sysmem_fallback);

	if (!pSurf) return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	dst = (unsigned short *)ddsd.lpSurface;
	n = w * h;

	for (i = 0; i < n; i++, rgbax += 4)
	{
		if (use_565)
			*dst++ = (unsigned short)(((rgbax[0]>>3)<<11) | ((rgbax[1]>>2)<<5) | (rgbax[2]>>3));
		else
			*dst++ = (unsigned short)(((rgbax[0]>>4)<<8) | ((rgbax[1]>>4)<<4) | (rgbax[2]>>4) | ((rgbax[3]>>4)<<12));
	}

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += n;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = w * h * 2;
	}

	return pSurf;
}

/* DCV_UpdateTextureSubRect: copy 16-bit subrect into existing texture (lightmap updates). */
void DCV_UpdateTextureSubRect( int texnum, int x, int y, int w, int h, const unsigned short* src, int src_pitch )
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 surf;
	dctexture_t *slot;
	int slot_index;
	int row, dst_pitch;
	unsigned short *dst;

	slot_index = (texnum & 0xFFFF);

	if (slot_index < 0 || slot_index >= DC_MAXTEXTURES)
		return;

	slot = &s_texSlots[slot_index];

	if (slot->nServerCount == -1)
		return;

	/* if surface null but ram_cache exists, decache first (skip GLT_SYSTEM). */
	if (!slot->pddsSurface && slot->pCacheBlock && !s_slotIsSystem[slot_index])
	{
		if (!DC_DecacheTextureSlot(slot))
			return;
	}

	if (!slot->pddsSurface)
		return;

	surf = (LPDIRECTDRAWSURFACE4)slot->pddsSurface;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(surf->lpVtbl->Lock(surf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
		return;

	dst = (unsigned short *)ddsd.lpSurface;
	dst_pitch = (int)(ddsd.lPitch / 2);

	if (dst_pitch <= 0)
		dst_pitch = w;

	dst += y * dst_pitch + x;

	for (row = 0; row < h; row++)
	{
		memcpy(dst, src, (size_t)w * sizeof(unsigned short));
		dst += dst_pitch;
		src += src_pitch;
	}

	surf->lpVtbl->Unlock(surf, NULL);
}

/* DCV_PrepSurface16: copy 16-bit words into a new texture surface. */
void *DCV_PrepSurface16(int w, int h, unsigned short *src, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	int n;

	pSurf = DCV_CreateTextureSurface16(w, h, color_fmt, allow_sysmem_fallback);

	if (!pSurf)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	dst = (unsigned short *)ddsd.lpSurface;
	n = w * h;

	while (n--)
		*dst++ = *src++;

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += w * h;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = w * h * 2;
	}

	return pSurf;
}

/* DCV_PrepSurfaceIndexed: 8-bit indices -> 16-bit via palette table.
   color_fmt selects the PVR pixel format: PVR_RGB565, PVR_ARGB1555, or PVR_ARGB4444. */
void *DCV_PrepSurfaceIndexed(int w, int h, byte *idx, unsigned short *palette, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	int i, n;

	pSurf = DCV_CreateTextureSurface16(w, h, color_fmt, allow_sysmem_fallback);

	if (!pSurf)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	dst = (unsigned short *)ddsd.lpSurface;
	n = w * h;

	for (i = 0; i < n; i++)
		*dst++ = palette[idx[i]];

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += n;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = w * h * 2;
	}

	return pSurf;
}

/* ---------------------------------------------------------------------------
 * Twiddled (morton-order) blitters. PowerVR square textures store texels in
 * recursive quadrant order; the destination pointer advances linearly while
 * the source is walked by subdividing.
 * --------------------------------------------------------------------------- */
static int				twiddle_size;
static byte				*twiddle_src;
static unsigned short	*twiddle_dst16;
static byte				*twiddle_dst8;
static unsigned short	*twiddle_pal16;
static byte				*twiddle_remap8;
static int				twiddle_active;

void DCV_TwiddleBlit16Recurse( int x, int y, int size )
{
	while (size != 1)
	{
		size >>= 1;
		DCV_TwiddleBlit16Recurse(x, y, size);
		DCV_TwiddleBlit16Recurse(x, y + size, size);
		x += size;
		DCV_TwiddleBlit16Recurse(x, y, size);
		y += size;
	}

	*twiddle_dst16++ = twiddle_pal16[twiddle_src[y * twiddle_size + x]];
}

void DCV_TwiddleBlit16( int size, unsigned short *dst, byte *src, unsigned short *pal16 )
{
	twiddle_pal16 = pal16;
	twiddle_size = size;
	twiddle_dst16 = dst;
	twiddle_src = src;
	twiddle_active = 1;

	if (size == 1)
	{
		*twiddle_dst16++ = pal16[src[0]];
		*twiddle_dst16++ = pal16[src[0]];
		*twiddle_dst16++ = pal16[src[0]];
		*twiddle_dst16++ = pal16[src[0]];
	}
	else
	{
		DCV_TwiddleBlit16Recurse(0, 0, size);
	}
}

void DCV_TwiddleBlit8Recurse( int x, int y, int size )
{
	while (size != 2)
	{
		size >>= 1;
		DCV_TwiddleBlit8Recurse(x, y, size);
		DCV_TwiddleBlit8Recurse(x, y + size, size);
		x += size;
		DCV_TwiddleBlit8Recurse(x, y, size);
		y += size;
	}

	twiddle_dst8[0] = twiddle_remap8[twiddle_src[y * twiddle_size + x]];
	twiddle_dst8[1] = twiddle_remap8[twiddle_src[(y + 1) * twiddle_size + x]];
	twiddle_dst8 += 2;
	twiddle_dst8[0] = twiddle_remap8[twiddle_src[y * twiddle_size + x + 1]];
	twiddle_dst8[1] = twiddle_remap8[twiddle_src[(y + 1) * twiddle_size + x + 1]];
	twiddle_dst8 += 2;
}

void DCV_TwiddleBlit8( int size, byte *dst, byte *src, byte *remap )
{
	twiddle_remap8 = remap;
	twiddle_size = size;
	twiddle_dst8 = dst;
	twiddle_src = src;
	twiddle_active = 1;

	DCV_TwiddleBlit8Recurse(0, 0, size);
}

/* DCV_PrepSurfacePaletted: square 8-bit source into a P8 palettized surface,
   twiddled, sharing a DDraw palette from the pool. */
void *DCV_PrepSurfacePaletted(int side, byte *idx, byte *pPal, dctexture_t *slot, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAW4 pDD4;
	LPDIRECTDRAWSURFACE4 pSurf;
	static byte identity[256];
	int i, pal_index;

	pDD4 = (LPDIRECTDRAW4)Sys_GetDirectDraw4();
	if (!pDD4)
		return NULL;

	pal_index = DC_GetPaletteIndex(pPal);
	if (pal_index < 0)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwWidth = side;
	ddsd.dwHeight = side;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

	pSurf = NULL;
	while (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
	{
		if (DC_ReclaimTextureSlot())
			continue;

		if (!allow_sysmem_fallback)
			return NULL;

		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
		if (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
			return NULL;
	}

	pSurf->lpVtbl->SetPalette(pSurf, gGLPalette[pal_index].lpPalette);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	if (!identity[1])
	{
		for (i = 0; i < 256; i++)
			identity[i] = (byte)i;
	}

	DCV_TwiddleBlit8(side, (byte *)ddsd.lpSurface, idx, identity);

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += side * side;

	if (slot)
	{
		slot->iPalette = (short)pal_index;
		slot->pddsSurface = pSurf;
		slot->cbData = side * side;
	}

	return pSurf;
}

/* DCV_ResampleBlit: box-filter an 8-bit indexed source into a locked 16-bit
   destination of different dimensions. Each destination texel averages every
   source texel it covers, per channel, using the destination pixel format's
   channel masks. */
int DCV_ResampleBlit( DDPIXELFORMAT *pf, LPDIRECTDRAWSURFACE4 surf, int dst_w, int dst_h, byte *src, int src_w, int src_h, unsigned short *pal16 )
{
	DDSURFACEDESC2 ddsd;
	unsigned short *dstrow;
	float xstep, ystep;
	int x, y;
	int sx, sy, sx0, sy0, sx1, sy1;
	int count;
	int accum_r, accum_g, accum_b, accum_a;
	unsigned int texel;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(surf->lpVtbl->Lock(surf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
		return 0;

	xstep = (float)src_w / (float)dst_w;
	ystep = (float)src_h / (float)dst_h;

	for (y = 0; y < dst_h; y++)
	{
		dstrow = (unsigned short *)((byte *)ddsd.lpSurface + y * ddsd.lPitch);

		for (x = 0; x < dst_w; x++)
		{
			sx0 = (int)(xstep * x);
			sx1 = (int)(xstep * (x + 1));
			sy0 = (int)(ystep * y);
			sy1 = (int)(ystep * (y + 1));

			count = 0;
			accum_r = accum_g = accum_b = accum_a = 0;

			for (sy = sy0; sy < sy1; sy++)
			{
				for (sx = sx0; sx < sx1; sx++)
				{
					texel = pal16[src[sy * src_w + sx]];
					accum_r += (int)(texel & pf->dwRBitMask);
					accum_g += (int)(texel & pf->dwGBitMask);
					accum_b += (int)(texel & pf->dwBBitMask);
					accum_a += (int)(texel & pf->dwRGBAlphaBitMask);
					count++;
				}
			}

			if (!count)
				Sys_Error("Destination texel at %d, %d had no source texels!\n", x, y);

			dstrow[x] = (unsigned short)(
				((accum_r / count) & pf->dwRBitMask) |
				((accum_g / count) & pf->dwGBitMask) |
				((accum_b / count) & pf->dwBBitMask) |
				((accum_a / count) & pf->dwRGBAlphaBitMask));
		}
	}

	surf->lpVtbl->Unlock(surf, NULL);
	return 1;
}

/* DCV_PrepSurfaceTwiddled: square source uploaded in twiddled texel order. */
void *DCV_PrepSurfaceTwiddled(int side, byte *idx, unsigned short *pal16, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;

	pSurf = DCV_CreateTextureSurface16(side, side, color_fmt, allow_sysmem_fallback);

	if (!pSurf)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	DCV_TwiddleBlit16(side, (unsigned short *)ddsd.lpSurface, idx, pal16);

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += side * side;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = side * side * 2;
	}

	return pSurf;
}

/* DCV_PrepSurfaceResampled: create a texture surface at the rounded dimensions
   and box-filter the source into it. */
void *DCV_PrepSurfaceResampled(int w, int h, byte *src, int src_w, int src_h, unsigned short *pal16, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
{
	DDPIXELFORMAT pf;
	LPDIRECTDRAWSURFACE4 pSurf;

	DCV_ClearCachedFlag();

	pSurf = DCV_CreateTextureSurface16(w, h, color_fmt, allow_sysmem_fallback);

	if (!pSurf)
		return NULL;

	memset(&pf, 0, sizeof(pf));
	pf.dwSize = sizeof(pf);
	pSurf->lpVtbl->GetPixelFormat(pSurf, &pf);

	if (!DCV_ResampleBlit(&pf, pSurf, w, h, src, src_w, src_h, pal16))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	texels += w * h;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = w * h * 2;
	}

	return pSurf;
}

static LPDIRECTDRAWSURFACE4 DCV_CreateTextureSurfaceCompressedVQ(int w, int h, int color_fmt, int image_fmt, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	DDPIXELFORMAT pf;
	LPDIRECTDRAW4 pDD4 = (LPDIRECTDRAW4)Sys_GetDirectDraw4();
	LPDIRECTDRAWSURFACE4 pSurf = NULL;

	if (!pDD4)
		return NULL;

	memset(&pf, 0, sizeof(pf));
	pf.dwSize = sizeof(pf);
	pf.dwFlags = DDPF_RGB | DDPF_COMPRESSED;
	pf.dwRGBBitCount = 16;

	switch (color_fmt)
	{
	default:
	case PVR_RGB565:
		pf.dwRBitMask = 0x0000F800;
		pf.dwGBitMask = 0x000007E0;
		pf.dwBBitMask = 0x0000001F;
		break;
	case PVR_ARGB4444:
		pf.dwFlags |= DDPF_ALPHAPIXELS;
		pf.dwRBitMask = 0x00000F00;
		pf.dwGBitMask = 0x000000F0;
		pf.dwBBitMask = 0x0000000F;
		pf.dwRGBAlphaBitMask = 0x0000F000;
		break;
	case PVR_ARGB1555:
		pf.dwFlags |= DDPF_ALPHAPIXELS;
		pf.dwRBitMask = 0x00007C00;
		pf.dwGBitMask = 0x000003E0;
		pf.dwBBitMask = 0x0000001F;
		pf.dwRGBAlphaBitMask = 0x00008000;
		break;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddpfPixelFormat = pf;

	if (image_fmt == PVR_VQ_MIPMAP)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX;

	while (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
	{
		if (DC_ReclaimTextureSlot())
			continue;

		if (!allow_sysmem_fallback)
			return NULL;

		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_SYSTEMMEMORY;

		if (image_fmt == PVR_VQ_MIPMAP)
			ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX;

		if (FAILED(pDD4->lpVtbl->CreateSurface(pDD4, &ddsd, &pSurf, NULL)))
			return NULL;
	}

	return pSurf;
}

/* DCV_PrepSurfacePVR: supports RECT/TWIDDLED/VQ(+mipmap top-level extraction). */
void *DCV_PrepSurfacePVR(int w, int h, void *pvr_data, unsigned int *fmt_table, char *name, dctexture_t *slot, int allow_sysmem_fallback)
{
	int payload;
	int copyBytes;
	int dstBytes;
	unsigned short *src16;
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	gbix_t *gbix;
	pvrt_t *pvrt;
	byte *payloadPtr;
	unsigned char color_fmt;
	unsigned char image_fmt;

	if (!pvr_data)
		return NULL;

	gbix = (gbix_t *)pvr_data;
	pvrt = (pvrt_t *)((byte *)pvr_data + 8 + gbix->nextTagOffset);

	payload = pvrt->textureDataSize - 8;

	if (payload <= 0)
		 return NULL;

	color_fmt = pvrt->colorFormat;
	image_fmt = pvrt->imageFormat;

	if (color_fmt > PVR_ARGB4444)
	{
		Sys_Error("%s: No support for PVR YUV or bump textures yet\n", name ? name : "<null>");
	}

	if (pvrt->width > 0 && pvrt->height > 0 && pvrt->width <= 1024 && pvrt->height <= 1024)
	{
		w = pvrt->width;
		h = pvrt->height;
	}

	payloadPtr = (byte *)pvrt + 16;

	if (image_fmt == PVR_CLUT8_TWIDDLED || image_fmt == PVR_CLUT4_TWIDDLED ||
		image_fmt == PVR_DIRECT8_TWIDDLED || image_fmt == PVR_DIRECT4_TWIDDLED)
	{
		Sys_Error("%s: Palettized PVR texture?\n", name ? name : "<null>");
	}

	if (image_fmt == PVR_SMALL_VQ || image_fmt == PVR_SMALL_VQ_MIPMAP)
	{
		Sys_Error("%s: D3D doesn't support small VQ textures\n", name ? name : "<null>");
	}

	/* VQ compressed path. Layout: codebook (2048) + indices. For VQ_MIPMAP: codebook +
	 * base indices + mip indices down to 8x8. Single copy to top surface; driver parses chain. */
	if (image_fmt == PVR_VQ || image_fmt == PVR_VQ_MIPMAP)
	{
		pSurf = DCV_CreateTextureSurfaceCompressedVQ(w, h, color_fmt, image_fmt, allow_sysmem_fallback);
		if (!pSurf)
			return NULL;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

#if defined(_WIN32_WCE)
		if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_COMPRESSED, NULL)))
#else
		if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)))
#endif
		{
			pSurf->lpVtbl->Release(pSurf);
			return NULL;
		}

		dstBytes = (int)(ddsd.dwLinearSize ? ddsd.dwLinearSize : payload);
		copyBytes = payload & ~3;

		if (copyBytes > 0 && ddsd.lpSurface)
		{
			unsigned int i, ndw = copyBytes >> 2;
			unsigned int *dst = (unsigned int *)ddsd.lpSurface;
			unsigned int *src = (unsigned int *)payloadPtr;

			for (i = 0; i < ndw; i++)
				dst[i] = src[i];
		}
		pSurf->lpVtbl->Unlock(pSurf, NULL);

		texels += (w * h);

		if (slot)
		{
			slot->pddsSurface = pSurf;
			slot->cbData = copyBytes;
		}
		return pSurf;
	}

	/* Uncompressed path (RECT = linear layout). */
	if (image_fmt == PVR_RECT)
	{
		src16 = (unsigned short *)payloadPtr;
		pSurf = DCV_CreateTextureSurface16(w, h, color_fmt, allow_sysmem_fallback);

		if (!pSurf)
			return NULL;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
		{
			pSurf->lpVtbl->Release(pSurf);
			return NULL;
		}

		dstBytes = (int)(ddsd.lPitch * h);

		if (dstBytes < 0)
			dstBytes = -dstBytes;
		copyBytes = w * h * 2;

		if (copyBytes > payload)
			copyBytes = payload;
		if (copyBytes > dstBytes)
			copyBytes = dstBytes;

		if (copyBytes > 0)
			memcpy(ddsd.lpSurface, src16, (size_t)copyBytes);

		pSurf->lpVtbl->Unlock(pSurf, NULL);

		texels += (w * h);

		if (slot)
		{
 			slot->pddsSurface = pSurf;
			slot->cbData = copyBytes;
		}
		return pSurf;
	}

	return NULL;
}

static const unsigned int key_sizes[] = { 8, 16, 32, 64, 128, 256, 512, 0 };

unsigned int DCV_ComputeTextureKey(int w, int h, byte *data, int mip_count, int tex_type)
{
	unsigned int shift;
	unsigned int key;
	int sample_bytes;
	int levels;
	int i;

	if (!data)
		return 0;

	levels = mip_count ? 4 : 1;
	switch (tex_type)
	{
	case TEX_TYPE_RGBA:
	case TEX_TYPE_RGB565:
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h * 4;
			w >>= 1;
			h >>= 1;
		}
		break;
	case 10:
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h * 2;
			w >>= 1;
			h >>= 1;
		}
		break;
	case TEX_TYPE_GBIX:
		sample_bytes = *(int *)(data + 0x14);
		data += 0x20;
		break;
	default:
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h;
			w >>= 1;
			h >>= 1;
		}
		break;
	}

	if (sample_bytes > 8000)
		sample_bytes = 8000;

	key = 0;
	shift = 0;
	for (i = 0; i < sample_bytes; ++i)
	{
		key ^= ((unsigned int)data[i]) << shift;
		shift = (shift + 1) & 0xF;
	}

	return key;
}

int DCV_PaletteIsMonochromeQuantized(unsigned char *pPal)
{
	int i = 0;

	do {
		byte r = pPal[0], g = pPal[1], b = pPal[2];
		pPal += 3;

		if ((r & 0xF8) != (g & 0xF8))
			return 0;
		if ((r & 0xF8) != (b & 0xF8))
			return 0;
	} while (++i < 256);

	return 1;
}

/* Indexed paletted upload into 4-4-4-4 texture surface. */
static void *DC_UploadIndexed4444(int w, int h, byte *idx, unsigned short *palette, dctexture_t *slot, int allow_sysmem_fallback)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	int i, n;

	pSurf = DCV_CreateTextureSurface16(w, h, PVR_ARGB4444, allow_sysmem_fallback);
	if (!pSurf)
		return NULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (FAILED(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL)))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	dst = (unsigned short *)ddsd.lpSurface;
	n = w * h;

	for (i = 0; i < n; i++)
		*dst++ = palette[idx[i]];

	pSurf->lpVtbl->Unlock(pSurf, NULL);
	texels += n;

	if (slot)
	{
		slot->pddsSurface = pSurf;
		slot->cbData = w * h * 2;
	}

	return pSurf;
}

/* Resample 8-bit indices to a new size, then upload as 4-4-4-4. */
static void *DC_UploadResampledIndexed4444(int out_w, int out_h, int src_w, int src_h, byte *idx, unsigned short *palette, dctexture_t *slot, int allow_sysmem_fallback)
{
	int x, y;
	byte *tmp;
	void *ret;

	tmp = (byte *)malloc(out_w * out_h);

	if (!tmp)
		return NULL;

	for (y = 0; y < out_h; y++)
	{
		int sy = (y * src_h) / out_h;
		for (x = 0; x < out_w; x++)
		{
			int sx = (x * src_w) / out_w;
			tmp[y * out_w + x] = idx[sy * src_w + sx];
		}
	}

	ret = DC_UploadIndexed4444(out_w, out_h, tmp, palette, slot, allow_sysmem_fallback);
	free(tmp);

	return ret;
}

/* DC_UploadResampledIndexed16: nearest-neighbor resample indices to new size. */
static void *DC_UploadResampledIndexed16(int out_w, int out_h, int in_w, int in_h, byte *data, unsigned short *palette, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
{
	byte *tmp;
	int x, y;
	int src_x, src_y;
	void *ret;

	if (out_w <= 0 || out_h <= 0 || in_w <= 0 || in_h <= 0)
		return NULL;

	tmp = (byte *)malloc((size_t)(out_w * out_h));
	if (!tmp)
		return NULL;

	for (y = 0; y < out_h; y++)
	{
		for (x = 0; x < out_w; x++)
		{
			src_x = x * in_w / out_w;
			src_y = y * in_h / out_h;

			if (src_x < 0)
				src_x = 0;

			if (src_x >= in_w)
				src_x = in_w - 1;

			if (src_y < 0)
				src_y = 0;

			if (src_y >= in_h)
				src_y = in_h - 1;


			tmp[y * out_w + x] = data[src_y * in_w + src_x];
		}
	}

	ret = DCV_PrepSurfaceIndexed(out_w, out_h, tmp, palette, slot, color_fmt, allow_sysmem_fallback);
	free(tmp);

	return ret;
}

static void DCV_BuildPalette4444Alpha(unsigned char *pPal, unsigned short *out_256)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		int r = pPal[i*3+0];
		int g = pPal[i*3+1];
		int b = pPal[i*3+2];
		int a = r;

		if (g > a)
			 a = g;
		if (b > a)
			a = b;

		/* Keep white RGB (glyph tinted by vertex color), but preserve
		 * 4-bit alpha gradient from source palette to avoid dotted edges. */
		out_256[i] = (unsigned short)(((a >> 4) << 12) | 0x0FFF);
	}
}

int DC_LoadTexture(char *identifier, int texture_type, int width, int height, void *data, short mipmap, int tex_type, unsigned char *pPal)
{
	int slot_index, out_w, out_h;
	unsigned int key;
	unsigned short pal_565[256];
	int allow_sysmem_fallback;
	dctexture_t *slot;
	LPDIRECTDRAWSURFACE4 pSurf;
	LPDIRECT3DTEXTURE2 pTex;

	if (tex_type == TEX_TYPE_LUM)
	 {
		Sys_Error("Lum textures not supported.");
		return -1;
	}

	allow_sysmem_fallback = 1;

	key = DCV_ComputeTextureKey(width, height, (byte *)data, mipmap ? 4 : 1, tex_type);

	if (!identifier || !identifier[0])
		return 0;

	slot_index = DC_FindTextureSlot(identifier, width, height, key);

	if (slot_index >= 0)
	{
		if (s_slotServercount[slot_index] > 0)
		{
			s_slotServercount[slot_index] = gHostSpawnCount;
			s_texSlots[slot_index].nServerCount = (short)gHostSpawnCount;
		}
		return slot_index;
	}

	slot_index = DCV_GetSlot();

	if (slot_index < 0)
		return -1;

	slot = &s_texSlots[slot_index];
	slot->pddsSurface = NULL;
	slot->pd3dtTexture = NULL;
	slot->pCacheBlock = NULL;
	slot->pbCacheData = NULL;
	slot->bCached = 0;
	slot->iTextureType = texture_type;
	s_slotServercount[slot_index] = (texture_type == GLT_WORLD) ? gHostSpawnCount : 0;
	slot->nServerCount = (short)((texture_type == GLT_WORLD) ? gHostSpawnCount : 0);
	s_slotUse4444[slot_index] = 0;
	s_slotIsSystem[slot_index] = (texture_type == GLT_SYSTEM) ? 1 : 0;
	slot->iPalette = -1;
	slot->iTextureKey = (int)key;
	slot->pszName = (char *)MnemoAlloc(strlen(identifier) + 1, 0x20, 0, "texname");

	if (!slot->pszName)
	{
		slot->nServerCount = -1;
		return -1;
	}

	strcpy(slot->pszName, identifier);

	DCV_ComputeScaledSize(tex_type, &out_w, &out_h, width, height, 0, 0, data);

	pSurf = NULL;

	switch (tex_type)
	{
		case TEX_TYPE_ALPHA:
		if (texture_type == GLT_SYSTEM)
		{
			s_slotUse4444[slot_index] = 1;
			DCV_BuildPalette4444Alpha(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DC_UploadIndexed4444(out_w, out_h, (byte *)data, pal_565, slot, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed4444(out_w, out_h, width, height, (byte *)data, pal_565, slot, allow_sysmem_fallback);
		}
		else
		{
			DCV_BuildPalette1555(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DCV_PrepSurfaceIndexed(out_w, out_h, (byte *)data, pal_565, slot, PVR_ARGB1555, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed16(out_w, out_h, width, height, (byte *)data, pal_565, slot, PVR_ARGB1555, allow_sysmem_fallback);
		}
		break;
		case TEX_TYPE_ALPHA_GRADIENT:
			s_slotUse4444[slot_index] = 1;
			DCV_BuildAlphaGradient4444(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DC_UploadIndexed4444(out_w, out_h, (byte *)data, pal_565, slot, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed4444(out_w, out_h, width, height, (byte *)data, pal_565, slot, allow_sysmem_fallback);
			break;
		case TEX_TYPE_NONE:
		default:
			DCV_BuildPalette565(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DCV_PrepSurfaceIndexed(out_w, out_h, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed16(out_w, out_h, width, height, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			break;
		case 5:
		case 10:
			pSurf = DCV_PrepSurface16(out_w, out_h, (unsigned short *)data, slot, s_slotUse4444[slot_index] ? 2 : 1, allow_sysmem_fallback);
			break;
		case 6:
		case 7:
		case 9:
			DCV_BuildPalette565(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DCV_PrepSurfaceIndexed(out_w, out_h, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed16(out_w, out_h, width, height, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			break;
		case TEX_TYPE_GBIX:
			if (data && *(unsigned int *)data == GBIXHEADER)
			{
				gbix_t *gbix = (gbix_t *)data;
				pvrt_t *pvrt = (pvrt_t *)((byte *)data + 8 + gbix->nextTagOffset);
				if (pvrt->version == PVRTSIGN)
					s_slotUse4444[slot_index] = (pvrt->colorFormat == PVR_ARGB4444);
			}
			pSurf = DCV_PrepSurfacePVR(out_w, out_h, data, NULL, identifier, slot, allow_sysmem_fallback);
			break;
		case 11:
			s_slotUse4444[slot_index] = 1;
			DCV_BuildMaxChannelAlpha(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DCV_PrepSurfaceIndexed(out_w, out_h, (byte *)data, pal_565, slot, PVR_ARGB4444, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed16(out_w, out_h, width, height, (byte *)data, pal_565, slot, PVR_ARGB4444, allow_sysmem_fallback);
			break;
	}

	if (!pSurf)
	{
		DC_FreeTextureSlot(slot);
		return -1;
	}

	pTex = NULL;

	if (FAILED(pSurf->lpVtbl->QueryInterface(pSurf, &IID_IDirect3DTexture2, (void **)&pTex)))
	{
		DC_FreeTextureSlot(slot);
		return -1;
	}

	slot->pddsSurface = pSurf;
	slot->pd3dtTexture = pTex;
	DCV_TouchTextureSlot(slot);
	slot->nSrcWidth = (short)width;
	slot->nSrcHeight = (short)height;
	texels += (out_w * out_h);
	return slot_index;
}

static LPDIRECT3DTEXTURE2 GetTextureForBind( int slot_index )
{
	dctexture_t *slot;

	if (slot_index < 0 || slot_index >= DC_MAXTEXTURES)
		return NULL;

	slot = &s_texSlots[slot_index];

	if (slot->nServerCount == -1)
		return NULL;

	if (!slot->pddsSurface && slot->pCacheBlock)
	{
		if (!DC_DecacheTextureSlot(slot))
		{
			Sys_Error("Couldn't reconstruct surface!\n");
			return NULL;
		}
	}

	if (!slot->pddsSurface || !slot->pd3dtTexture)
		return NULL;

	DCV_TouchTextureSlot(slot);
	return (LPDIRECT3DTEXTURE2)slot->pd3dtTexture;
}

void GL_BindStage( int texnum, int stage )
{
	int slot_index;
	LPDIRECT3DDEVICE3 dev;
	LPDIRECT3DTEXTURE2 tex;

	texnum = (texnum & 0xFFFF);
	if (gl_nobind.value)
		texnum = char_texture;

	if (stage == 0 && currenttexture == texnum)
		return;

	DCV_FlushInline();

	dev = (LPDIRECT3DDEVICE3)Sys_GetD3DDevice3();

	if (!dev || !dev->lpVtbl || !dev->lpVtbl->SetTexture)
		return;

	slot_index = texnum;

	if (slot_index < 0 || slot_index >= DC_MAXTEXTURES || s_texSlots[slot_index].nServerCount == -1)
	{
		dev->lpVtbl->SetTexture(dev, stage, NULL);
		s_pCurrentTextureSurface = NULL;

		if (stage == 0)
		{
			s_nD3dCurrentTexnum = -1;
			currenttexture = -1;
		}

		return;
	}

	tex = GetTextureForBind(slot_index);

	if (tex)
		dev->lpVtbl->SetTexture(dev, stage, tex);

	s_pCurrentTextureSurface = s_texSlots[slot_index].pddsSurface ? (LPDIRECTDRAWSURFACE4)s_texSlots[slot_index].pddsSurface : NULL;

	if (stage == 0)
	{
		s_nD3dCurrentTexnum = slot_index;
		currenttexture = slot_index;
	}
}

/*
================
GL_UnloadTextures

Unload all loaded textures
We do this every time we load the map
================
*/
void GL_UnloadTextures( void )
{
	int i;
	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];
		if (slot->nServerCount == -1)
			continue;
		if (s_slotServercount[i] > 0 && s_slotServercount[i] != gHostSpawnCount)
			DC_FreeTextureSlot(slot);
	}
}

int DC_FreeTextureByName( char *name )
{
	int i;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		if (slot->nServerCount == -1)
			continue;

		if (slot->pszName && strcmp(name, slot->pszName) == 0)
		{
			if (slot->nUsageCount == 0)
				DC_FreeTextureSlot(slot);
			return 1;
		}
	}

	return 0;
}

void DC_TouchTexture( int texnum )
{
	dctexture_t *slot;

	if (texnum < 0 || texnum >= DC_MAXTEXTURES)
		return;

	slot = &s_texSlots[texnum];

	if (slot->nServerCount == -1 || slot->nServerCount == 0)
		return;

	DCV_LinkTextureSlotFront(slot);

	if (slot->nServerCount != 0)
		slot->nServerCount = (short)gHostSpawnCount;
}

int DC_ForceFreeTextureByName( char *name )
{
	int i;
	const char *slotname;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		if (slot->nServerCount == -1)
			continue;

		slotname = slot->pszName ? slot->pszName : "<nameless>";

		if (strcmp(slotname, name) == 0)
		{
			DC_FreeTextureSlot(slot);
			return 1;
		}
	}

	return 0;
}

int DC_FreeTextureByIndex( int texnum )
{
	dctexture_t *slot;

	if (texnum < 0 || texnum >= DC_MAXTEXTURES)
		return 0;

	slot = &s_texSlots[texnum];

	if (slot->nServerCount == -1)
		return 0;

	DC_FreeTextureSlot(slot);
	return 1;
}

int DC_ReleaseTexture( int texnum )
{
	dctexture_t *slot;

	if (texnum < 0 || texnum >= DC_MAXTEXTURES)
		return 0;

	slot = &s_texSlots[texnum];

	if (slot->nServerCount == -1)
		return 0;

	slot->nUsageCount--;
	if (slot->nUsageCount == 0)
		DC_FreeTextureSlot(slot);

	return 1;
}

int DC_FreeStaleTextureSlots( void )
{
	int i, freed = 0;
	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];
		if (slot->nServerCount != -1 &&
		    slot->nServerCount > 0 &&
		    slot->nServerCount < (short)gHostSpawnCount &&
		    slot->nUsageCount == 0)
		{
			DC_FreeTextureSlot(slot);
			freed++;
		}
	}
	return freed;
}

void DC_InitTextureList( void )
{
	int i;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		DC_ClearTextureSlot(&s_texSlots[i]);
		s_slotServercount[i] = -1;
		s_slotUse4444[i] = 0;
		s_slotIsSystem[i] = 0;
	}

	memset(&s_dcTextureLruHead, 0, sizeof(s_dcTextureLruHead));
	memset(&s_dcTextureLruTail, 0, sizeof(s_dcTextureLruTail));
	s_dcTextureLruHead.pNext = &s_dcTextureLruTail;
	s_dcTextureLruTail.pPrev = &s_dcTextureLruHead;

	for (i = 0; i < DC_MAXPALETTES; i++)
	{
		gGLPalette[i].tag = -1;
		gGLPalette[i].referenceCount = 0;
		gGLPalette[i].lpPalette = NULL;
	}

	s_nCached = 0;
	texels = 0;
	s_pCurrentTextureSurface = NULL;
	s_nD3dCurrentTexnum = -1;
}

static void DC_PaletteEntryClear( dcpalette_t* entry )
{
	entry->tag = -1;
	entry->referenceCount = 0;
	entry->lpPalette = NULL;
}

static void DC_PaletteEntryRelease( dcpalette_t* entry )
{
	if (entry->lpPalette)
		entry->lpPalette->lpVtbl->Release(entry->lpPalette);
}

void GL_Bind( int texnum, int stage )
{
	GL_BindStage(texnum, stage);
}

void GL_Texels_f( void )
{
	Con_Printf("Current uploaded texels: %i\n", texels);
}

/* ---------------------------------------------------------------------------
 * 2D vertex helper: adds to the shared D3DFVF_LVERTEX accumulator with the
 * current HUD depth. HUD ortho projection (set by GLBeginHud) transforms
 * pixel coords to NDC.
 * --------------------------------------------------------------------------- */
static void DCV_2D_AddVertex( float x, float y, float u, float v )
{
	DCV_AddVertex(x, y, dc_depthhud.value, u, v);
}

/*
===============
Draw_StringLen
===============
*/
int Draw_StringLen( char* psz )
{
	Font_SetScale(1.0f, 1.0f);
	return Font_StringWidth(draw_chars, psz);
}

int Draw_MessageFontInfo( short* pWidth )
{
	int i;

	if (!draw_chars)
		return 0;

	if (pWidth)
	{
		for (i = 0; i < 256; i++)
			*pWidth++ = draw_chars->fontinfo[i].charwidth;
	}

	return Font_CharHeight(draw_chars);
}

int Draw_MessageCharacterAdd( int x, int y, int num, int rr, int gg, int bb )
{
	DCV_SetColor(rr, gg, bb, 255);
	Font_SetScale(1.0f, 1.0f);
	DCV_SetDefaultRenderStates();
	return Font_DrawCharI(draw_chars, x, y, num);
}

/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f( void )
{
}

/*
===============
Draw_Init
===============
*/
void Draw_Init( void )
{
	qpic_t*	cb;
	glpic_t* gl;
	unsigned char* pPal;
	float			prev;
	int				i;

	DC_InitTextureList();

	Draw_CacheWadInit("cached.wad", 16, &menu_wad);
	menu_wad.tempWad = TRUE;

	Draw_CacheWadHandler(&decal_wad, Draw_MiptexTexture, MIP_EXTRASIZE);
	Draw_CacheWadHandler(&custom_wad, Draw_MiptexTexture, MIP_EXTRASIZE);

	Cvar_RegisterVariable(&gl_nobind);
	Cvar_RegisterVariable(&gl_max_size);
	Cvar_RegisterVariable(&gl_round_down);
	Cvar_RegisterVariable(&gl_picmip);
	Cvar_RegisterVariable(&gl_palette_tex);

	Cmd_AddCommand("gl_texturemode", Draw_TextureMode_f);
	Cmd_AddCommand("gl_texels", GL_Texels_f);

	for (i = 0; i < 256; i++)
		texgammatable[i] = i;

	draw_chars = (qfont_t*)W_GetLumpName("conchars");
	draw_creditsfont = (qfont_t*)W_GetLumpName("creditsfont");

	if (con_loading)
		cb = (qpic_t*)Draw_CachePic("gfx/loading.lmp");
	else
		cb = (qpic_t*)Draw_CachePic("gfx/conback.lmp");
	if (!cb)
		Sys_Error("Couldn't load conback.lmp");

	SwapPic(cb);

	conback->width = cb->width;
	conback->height = cb->height;

	gl = (glpic_t*)conback->data;
	pPal = &cb->data[cb->width * cb->height + 2];
	gl->texnum = GL_LoadTexture("conback", GLT_SYSTEM, cb->width, cb->height, cb->data, FALSE, TEX_TYPE_NONE, pPal);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	prev = gl_round_down.value;
	gl_round_down.value = 0.0;

	// now turn them into textures
	pPal = &draw_chars->data[256 * draw_chars->height + 2];
	char_texture = GL_LoadTexture("conchars", GLT_SYSTEM, 256, draw_chars->height, draw_chars->data, FALSE, TEX_TYPE_ALPHA, pPal);
	pPal = &draw_creditsfont->data[256 * draw_creditsfont->height + 2];
	font_texture = GL_LoadTexture("creditsfont", GLT_SYSTEM, 256, draw_creditsfont->height, draw_creditsfont->data, FALSE, TEX_TYPE_ALPHA, pPal);

	gl_round_down.value = prev;

	// Publish the glyph-sheet UV scale into each font header. Font_DrawChar reads
	// usize (+0x414) and vsize (+0x418) to map glyph cells to texture coords;
	// these overlap the first pixels of the sheet, which is safe now that the
	// sheets have been uploaded to textures.
	*(float *)(draw_chars->data + 4)       = 1.0f / 256.0f;
	*(float *)(draw_chars->data + 8)       = 1.0f / (float)draw_chars->height;
	*(float *)(draw_creditsfont->data + 4) = 1.0f / 256.0f;
	*(float *)(draw_creditsfont->data + 8) = 1.0f / (float)draw_creditsfont->height;

	draw_chars   = draw_creditsfont;
	char_texture = font_texture;

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	draw_disc = (qpic_t*)LoadTransBMP("lambda");
}

/*
================
Draw_Character

Draws a single character
================
*/
int Draw_Character( int x, int y, int num )
{
	DCV_SetTextRenderStates();
	Font_SetScale(1.0f, 1.0f);
	return Font_DrawCharI(draw_chars, x, y, num);
}

/*
================
Draw_String
================
*/
int Draw_String( int x, int y, char* str )
{
	DCV_SetHudDepth(3.0f);
	DCV_SetPackedColor(0xFFFF9000);

	while (*str)
	{
		DCV_SetTextRenderStates();
		Font_SetScale(1.0f, 1.0f);
		x += Font_DrawCharI(draw_chars, x, y, *str);
		str++;
	}
	return x;
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic( int x, int y, qpic_t *pic )
{
	glpic_t* gl;
	int base;
	float uleft, uright, vtop, vbottom;

	if (!pic)
		return;

	gl = (glpic_t*)pic->data;

	uleft = gl->sl;
	uright = gl->sh;
	vtop = gl->tl;
	vbottom = gl->th;

	DCV_SetColor(255, 255, 255, 255);
	GL_Bind(gl->texnum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,                   (float)y,                    uleft,  vtop);
	DCV_2D_AddVertex((float)(x + pic->width),    (float)y,                    uright, vtop);
	DCV_2D_AddVertex((float)(x + pic->width),    (float)(y + pic->height),    uright, vbottom);
	DCV_2D_AddVertex((float)x,                   (float)(y + pic->height),   uleft,  vbottom);

}

/*
=============
Draw_AlphaSubPic
=============
*/
void Draw_AlphaSubPic( int xDest, int yDest, int xSrc, int ySrc, int iWidth, int iHeight, qpic_t* pic, colorVec* pc, int iAlpha )
{
	glpic_t* gl;
	int base;
	float uleft, uright, vtop, vbottom;
	float du, dv;

	if (!pic)
		return;

	gl = (glpic_t*)pic->data;

	du = (gl->sh - gl->sl) / (float)pic->width;
	dv = (gl->th - gl->tl) / (float)pic->height;

	uleft = gl->sl + xSrc * du;
	uright = uleft + iWidth * du;
	vtop = gl->tl + ySrc * dv;
	vbottom = vtop + iHeight * dv;

	DCV_TexState_Blend();
	DCV_SetColor(pc->r, pc->g, pc->b, iAlpha);
	GL_Bind(gl->texnum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)xDest,            (float)yDest,             uleft,  vtop);
	DCV_2D_AddVertex((float)(xDest + iWidth), (float)yDest,             uright, vtop);
	DCV_2D_AddVertex((float)(xDest + iWidth), (float)(yDest + iHeight), uright, vbottom);
	DCV_2D_AddVertex((float)xDest,            (float)(yDest + iHeight), uleft,  vbottom);

	DCV_FlushInline();
	DCV_TexState_Opaque();
}

/*
=============
Draw_Pic2
=============
*/
void Draw_Pic2( int x, int y, int w, int h, qpic_t* pic )
{
	glpic_t* gl;
	int base;
	float uleft, uright, vtop, vbottom;

	if (!pic)
		return;

	gl = (glpic_t*)pic->data;

	uleft = gl->sl;
	uright = gl->sh;
	vtop = gl->tl;
	vbottom = gl->th;

	DCV_SetColor(255, 255, 255, 255);
	GL_Bind(gl->texnum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,        (float)y,         uleft,  vtop);
	DCV_2D_AddVertex((float)(x + w),  (float)y,         uright, vtop);
	DCV_2D_AddVertex((float)(x + w),  (float)(y + h),   uright, vbottom);
	DCV_2D_AddVertex((float)x,        (float)(y + h),   uleft,  vbottom);
}

/*
=============
Draw_ConsoleBackground

Draw the console background image, scrolled down from the top so that its
bottom edge sits at the bottom of the visible console (y = lines).
=============
*/
void Draw_ConsoleBackground( int lines )
{
	DCV_SetHudDepth(2.0f);
	DCV_SetPackedColor(0xFFFFFFFF);
	DCV_SetDefaultRenderStates();
	Draw_Pic2(0, lines - glheight, glwidth, glheight + 1, conback);
	DCV_SetHudDepth(3.0f);
}

// Sprites are clipped to this rectangle (x,y,width,height) if ScissorTest is enabled
int scissor_x = 0, scissor_y = 0, scissor_width = 0, scissor_height = 0;
qboolean giScissorTest = FALSE;

/*
===============
EnableScissorTest

Set the scissor
 the coordinate system for gl is upsidedown (inverted-y) as compared to software, so the
 specified clipping rect must be flipped
===============
*/
void EnableScissorTest( int x, int y, int width, int height )
{
	// Added casts to int because these warnings are so annoying
	x = clamp(x, 0, (int)vid.width);
	y = clamp(y, 0, (int)vid.height);
	width = clamp(width, 0, (int)vid.width - x);
	height = clamp(height, 0, (int)vid.height - y);

	scissor_x = x;
	scissor_y = y;
	scissor_width = width;
	scissor_height = height;

	giScissorTest = TRUE;
}

/*
===============
DisableScissorTest
===============
*/
void DisableScissorTest( void )
{
	scissor_x = 0;
	scissor_y = 0;
	scissor_width = 0;
	scissor_height = 0;

	giScissorTest = FALSE;
}

/*
===============
ValidateWRect

Verify that this is a valid, properly ordered rectangle.
===============
*/
static int ValidateWRect( const wrect_t* prc )
{
	if (!prc)
		return FALSE;

	if ((prc->left >= prc->right) || (prc->top >= prc->bottom))
	{
		//!!!UNDONE Dev only warning msg
		return FALSE;
	}

	return TRUE;
}

/*
===============
IntersectWRect

classic interview question
===============
*/
int IntersectWRect( const wrect_t* prc1, const wrect_t* prc2, wrect_t* prc )
{
	wrect_t rc;

	if (!prc)
		prc = &rc;

	prc->left = max(prc1->left, prc2->left);
	prc->right = min(prc1->right, prc2->right);

	if (prc->left < prc->right)
	{
		prc->top = max(prc1->top, prc2->top);
		prc->bottom = min(prc1->bottom, prc2->bottom);

		if (prc->top < prc->bottom)
			return TRUE;
	}

	return FALSE;
}

/*
===============
AdjustSubRect
===============
*/
void AdjustSubRect( mspriteframe_t* pFrame, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom, int* pw, int* ph, const wrect_t* prcSubRect )
{
	wrect_t rc;

	if (!ValidateWRect(prcSubRect))
		return;

	// clip sub rect to sprite

	rc.top = rc.left = 0;
	rc.right = *pw;
	rc.bottom = *ph;

	if (!IntersectWRect(prcSubRect, &rc, &rc))
		return;

	*pw = rc.right - rc.left;
	*ph = rc.bottom - rc.top;

	*pfLeft = rc.left / (float)pFrame->width;
	*pfRight = rc.right / (float)pFrame->width;
	*pfTop = rc.top / (float)pFrame->height;
	*pfBottom = rc.bottom / (float)pFrame->height;
}

/*
===============
Draw_Frame
===============
*/
void Draw_Frame( mspriteframe_t* pFrame, int x, int y, const wrect_t* prcSubRect )
{
	float	fLeft = 0;
	float	fRight = 1;
	float	fTop = 0;
	float	fBottom = 1;
	int		iWidth;
	int		iHeight;
	int		base;

	iWidth = pFrame->width;
	iHeight = pFrame->height;

	if (prcSubRect)
		AdjustSubRect(pFrame, &fLeft, &fRight, &fTop, &fBottom, &iWidth, &iHeight, prcSubRect);

	GL_Bind(pFrame->gl_texturenum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,            (float)y,             fLeft,  fTop);
	DCV_2D_AddVertex((float)(x + iWidth), (float)y,             fRight, fTop);
	DCV_2D_AddVertex((float)(x + iWidth), (float)(y + iHeight), fRight, fBottom);
	DCV_2D_AddVertex((float)x,            (float)(y + iHeight), fLeft,  fBottom);
}

void Draw_SpriteFrame( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	DCV_TexState_Additive();
	DCV_SetColor(gSpriteColorR, gSpriteColorG, gSpriteColorB, 255);
	Draw_Frame(pFrame, x, y, prcSubRect);
	DCV_TexState_Opaque();
}

void Draw_SpriteFrameHoles( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	DCV_TexState_AlphaTest();
	if (gl_spriteblend.value)
		DCV_TexState_Blend();
	DCV_SetColor(gSpriteColorR, gSpriteColorG, gSpriteColorB, 255);
	Draw_Frame(pFrame, x, y, prcSubRect);
	DCV_TexState_Opaque();
}

void Draw_SpriteFrameAdditive( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	DCV_TexState_Additive();
	DCV_SetColor(gSpriteColorR, gSpriteColorG, gSpriteColorB, 255);
	Draw_Frame(pFrame, x, y, prcSubRect);
	DCV_TexState_Opaque();
}

void Draw_SpriteFrameGeneric( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height )
{
	DCV_TexState_Blend();
	DCV_SetColor(gSpriteColorR, gSpriteColorG, gSpriteColorB, 255);
	Draw_Frame(pFrame, x, y, prcSubRect);
	DCV_TexState_Opaque();
}

/*
===============
Draw_FillRGBA

Fills the given rectangle with a given color.
===============
*/
void Draw_FillRGBA( int x, int y, int w, int h, int r, int g, int b, int a )
{
	int base;

	DCV_TexState_VertColor();
	DCV_SetColor(r, g, b, a);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex((float)x,         (float)y,         1.0f, 0.0f, 0.0f);
	DCV_AddVertex((float)(x + w),   (float)y,         1.0f, 1.0f, 0.0f);
	DCV_AddVertex((float)(x + w),   (float)(y + h),   1.0f, 1.0f, 1.0f);
	DCV_AddVertex((float)x,         (float)(y + h),   1.0f, 0.0f, 1.0f);
	DCV_SetPackedColor(0xFFFFFFFF);
	DCV_FlushApplyRenderState(0x1b, 0);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear( int x, int y, int w, int h )
{
	Draw_FillRGBA(x, y, w, h, 0, 0, 0, 255);
}

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen( void )
{
	int base;

	DCV_TexState_VertColor();
	DCV_SetColor(0, 0, 0, 204);
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex(0.0f,           0.0f,            0.0f, 0.0f);
	DCV_2D_AddVertex((float)glwidth, 0.0f,            1.0f, 0.0f);
	DCV_2D_AddVertex((float)glwidth, (float)glheight, 1.0f, 1.0f);
	DCV_2D_AddVertex(0.0f,           (float)glheight, 0.0f, 1.0f);
	DCV_FlushInline();
	DCV_SetPackedColor(0xFFFFFFFF);
	DCV_TexState_Opaque();
}

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc( void )
{
	if (!draw_disc)
		return;

	Draw_CenterPic(draw_disc);
}

/*
================
GL_FindTexture
================
*/
int GL_FindTexture( char* identifier )
{
	Sys_Error("NYI - go through DCV textures");
	return -1;
}

void GL_UnloadTexture( char* identifier )
{
	DC_FreeTextureByName(identifier);
}

int GL_PaletteTag( byte* pPal )
{
	int tag;
	int i;

	tag = *pPal;

	for (i = 0; i < 768; i++)
	{
		tag = (tag + pPal[1]) ^ *pPal;
		pPal++;
	}

	if (tag < 0)
		tag = -tag;

	return tag;
}

int DC_GetSlotPaletteIndex(int slot_index)
{
	if (slot_index < 0 || slot_index >= DC_MAXTEXTURES) return -1;
	return (int)s_texSlots[slot_index].iPalette;
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture( char* identifier, GL_TEXTURETYPE textureType, int width, int height, unsigned char* data, int mipmap, int iType, unsigned char* pPal )
{
	int slot_index, pal_index;
	slot_index = DC_LoadTexture(identifier, (int)textureType, width, height, data, (short)mipmap, iType, pPal);

	if (slot_index < 0)
		return 0;

	pal_index = DC_GetSlotPaletteIndex(slot_index);

	if (pal_index >= 0)
		return slot_index | ((pal_index + 1) << 16);

	return slot_index;
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture( qpic_t* pic, char* pszName )
{
	unsigned char* pPal;

	pPal = &pic->data[pic->width * pic->height + 2];

	return GL_LoadTexture(pszName, GLT_SYSTEM, pic->width, pic->height, pic->data, FALSE, TEX_TYPE_ALPHA, pPal);
}

qpic_t* LoadTransPic( char* pszName, qpic_t* ppic )
{
	static int	trans_pic_loaded;
	glpic_t* gl;
	qpic_t* ppicNew;
	byte* pPal;
	int slot_index;

	if (trans_pic_loaded)
		Sys_Error("LoadTransPic called multiple times.\n");
	trans_pic_loaded = 1;

	if (!ppic)
		return NULL;

	ppicNew = (qpic_t*)MnemoAlloc(sizeof(qpic_t) + sizeof(glpic_t), 0x20, 0, "LoadTransPic");
	gl = (glpic_t*)ppicNew->data;

	ppicNew->width = ppic->width;
	ppicNew->height = ppic->height;

	pPal = &ppic->data[ppic->width * ppic->height + 2];
	slot_index = DC_LoadTexture(pszName, GLT_SYSTEM, ppic->width, ppic->height, ppic->data, FALSE, TEX_TYPE_ALPHA, pPal);
	if (slot_index < 0)
		slot_index = 0;

	gl->texnum = (slot_index & 0xFFFF);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return ppicNew;
}

/* ---------------------------------------------------------------------------
 * Helpers still referenced from files that have not been reworked yet.
 * --------------------------------------------------------------------------- */

qpic_t* LoadTransBMP( char* pszName )
{
	return LoadTransPic(pszName, (qpic_t*)W_GetLumpName(pszName));
}

qpic_t* Draw_CachePic( char* path )
{
	qpic_t* ret;
	int idx;

	idx = Draw_CacheIndex(&menu_wad, path);
	ret = (qpic_t*)Draw_CacheGet(&menu_wad, idx);
	return ret;
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill( int x, int y, int w, int h, int c )
{
	Draw_FillRGBA(x, y, w, h,
		host_basepal[c * 4],
		host_basepal[c * 4 + 1],
		host_basepal[c * 4 + 2],
		255);
}

void GL_PaletteClearSky( void )
{
	Sys_Error("GL_PaletteClearSky no longer used\n");
}

float	g_flHudDepth;		// current HUD depth sublayer

/*
================
DCV_SetHudDepth

Point the HUD transform at one of the 2D depth sublayers between dc_msh and
dc_msh2.
================
*/
void DCV_SetHudDepth( float layer )
{
	g_flHudDepth = layer;
}

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
typedef struct
{
	char name[64];
	qpic_t pic;
	byte padding[32];
} cachepic_t;

#define	MAX_CACHED_PICS		16
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

int		pic_texels;
int		pic_count;

cachewad_t	decal_wad;
cachewad_t	custom_wad;
cachewad_t	menu_wad;

int			numgltextures;

float		chars_xsize, chars_ysize;
float		creditsfont_ysize;

char		decal_names[MAX_BASE_DECALS][16];

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

static dctexture_t	s_texSlots[DC_MAXTEXTURES];
static dctexture_t	s_dcTextureLruHead;
static int			s_nCached;
static LPDIRECTDRAWSURFACE4 s_pCurrentTextureSurface;
static int			s_slotServercount[DC_MAXTEXTURES];
static int			s_slotUse4444[DC_MAXTEXTURES];
static int			s_slotIsSystem[DC_MAXTEXTURES];  /* GLT_SYSTEM: never cache/decache */
static int			s_bTexReclaimGuard;
static int			s_nD3dCurrentTexnum = -1;

static int DCV_GetTextureSlotIndex( dctexture_t* slot )
{
	int i;

	if (!slot)
		return -1;

	for (i = 0; i < DC_MAXTEXTURES; ++i)
	{
		if (&s_texSlots[i] == slot)
			return i;
	}

	return -1;
}


extern void* Sys_GetDirectDraw4( void );
extern void* Sys_GetBackBuffer4( void );
extern void* Sys_GetPrimarySurface4( void );
extern void* Sys_GetD3DDevice3( void );

void	GL_PaletteInit( void );
void	GL_PaletteSelect( int paletteIndex );

qpic_t* LoadTransBMP( char* pszName );
qpic_t* LoadTransPic( char* pszName, qpic_t* ppic );

static LPDIRECTDRAWSURFACE4 DCV_CreateTextureSurface16(int w, int h, int color_fmt, int allow_sysmem_fallback);
void DC_FreeTextureSlot(dctexture_t *slot);
static int DC_DecacheTextureSlot( dctexture_t *slot );
static void DCV_UnlinkTextureSlot( dctexture_t *slot );
static void DCV_LinkTextureSlotFront( dctexture_t *slot );
static void DCV_TouchTextureSlot( dctexture_t *slot );
static int DCV_AttachTextureInterface( dctexture_t *slot, LPDIRECTDRAWSURFACE4 surf );
static int DC_FreeStaleTextureSlots( void );

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
	     s != NULL && s != &s_dcTextureLruHead;
	     s = (dctexture_t *)s->pNext)
		s->bCached = 0;
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

	/* Binary uses GetSurfaceDesc result directly as Lock desc with DDLOCK_WAIT only. */
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

	/* Binary uses pCacheBlock as Lock descriptor so pCacheBlock->lpSurface gets the
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

static int DC_FreeStaleTextureSlots( void )
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

int DC_ReclaimTextureSlot( void )
{
	dctexture_t *slot, *tail;

	/* Walk to the LRU tail (pNext chain ends at NULL = least-recently-used).
	   head.pPrev is not maintained, so we find the tail the long way. */
	tail = NULL;
	for (slot = (dctexture_t *)s_dcTextureLruHead.pNext;
	     slot && slot != &s_dcTextureLruHead;
	     slot = (dctexture_t *)slot->pNext)
		tail = slot;

	/* Now walk from LRU toward MRU via pPrev and evict the first uncached slot. */
	for (slot = tail; slot && slot != &s_dcTextureLruHead;
	     slot = (dctexture_t *)slot->pPrev)
	{
		if (slot->pbCacheData || slot->bCached)
			continue;

		if (slot->nServerCount == 0 ||
		    gHostSpawnCount <= (int)slot->nServerCount ||
		    slot->nUsageCount != 0)
		{
			DC_CacheTextureToRam(slot);
			return 1;
		}

		DC_FreeTextureSlot(slot);
		return 1;
	}

	return DC_FreeStaleTextureSlots();
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

/* ---------------------------------------------------------------------------
 * 2D vertex helper: adds to the shared D3DFVF_LVERTEX accumulator with z=0.
 * HUD ortho projection (set by GLBeginHud) transforming pixel coords to NDC.
 * --------------------------------------------------------------------------- */
static void DCV_2D_AddVertex( float x, float y, float u, float v )
{
	DCV_AddVertex(x, y, dc_depthhud.value, u, v);
}


static const unsigned int pot_sizes[] = { 8, 16, 32, 64, 128, 256, 512, 0 };

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
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h * 4;
			w >>= 1;
			h >>= 1;
		}
		break;
	case TEX_TYPE_RGB565:
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

int DC_FindTextureSlotByName(char *name, int width, int height, unsigned int key)
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

void DCV_GetRoundedDimensions(int tex_type, int *out_w, int *out_h, int w, int h, int round_down, int round_up, char *name, void *data)
{
	int i, max_sz, min_sz;
	const unsigned int *p;
	pvrt_t *pvrt;

	if (tex_type == TEX_TYPE_GBIX && data && *(unsigned int *)data == GBIXHEADER)
	{
		gbix_t *gbix = (gbix_t *)data;
		pvrt = (pvrt_t *)((byte *)data + 8 + gbix->nextTagOffset);

		if (pvrt && pvrt->version == PVRTSIGN && pvrt->width > 0 && pvrt->height > 0)
		{
			*out_w = (int)pvrt->width;
			*out_h = (int)pvrt->height;
			return;
		}
	}

	max_sz = (w >= h) ? w : h;
	min_sz = (w <= h) ? w : h;

	if (max_sz < 8) max_sz = 8;
	if (min_sz < 8) min_sz = 8;

	p = pot_sizes;

	while (*p && max_sz > (int)*p)
		 p++;

	*out_w = *p ? (int)*p : max_sz;

	p = pot_sizes;

	while (*p && min_sz > (int)*p) 
		p++;

	*out_h = *p ? (int)*p : min_sz;

	if (w <= h) 
	{
 		i = *out_w;
		*out_w = *out_h; 
		*out_h = i; 
	}
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

int DC_GetSlotPaletteIndex(int slot_index)
{
	if (slot_index < 0 || slot_index >= DC_MAXTEXTURES) return -1;
	return (int)s_texSlots[slot_index].iPalette;
}

/* Create a 16-bit texture surface.*/
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

/* DC_UploadRGBA16: convert RGBA bytes to 565 or 4444 and lock/copy/unlock. */
static void *DC_UploadRGBA16(int w, int h, byte *rgbax, int use_565, dctexture_t *slot, int allow_sysmem_fallback)
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

/* DC_UploadRaw16: copy 16-bit words into a new texture surface. */
static void *DC_UploadRaw16(int w, int h, unsigned short *src, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
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

/* DC_UploadSubRect16: copy 16-bit subrect into existing texture (lightmap updates). */
void DC_UploadSubRect16( int texnum, int x, int y, int w, int h, const unsigned short* src, int src_pitch )
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

/* DC_UploadIndexed16: 8-bit indices -> 16-bit via palette table.
   color_fmt selects the PVR pixel format: PVR_RGB565, PVR_ARGB1555, or PVR_ARGB4444. */
static void *DC_UploadIndexed16(int w, int h, byte *idx, unsigned short *palette, dctexture_t *slot, int color_fmt, int allow_sysmem_fallback)
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

	ret = DC_UploadIndexed16(out_w, out_h, tmp, palette, slot, color_fmt, allow_sysmem_fallback);
	free(tmp);

	return ret;
}

static void DCV_BuildPalette565(unsigned char *pPal, unsigned short *out_256)
{
	int i;

	for (i = 0; i < 256; i++)
		out_256[i] = (unsigned short)(((pPal[i*3+0]>>3)<<11) | ((pPal[i*3+1]>>2)<<5) | (pPal[i*3+2]>>3));
}
static void DCV_BuildPalette1555(unsigned char *pPal, unsigned short *out_256)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		unsigned short c =
			(unsigned short)(((pPal[i*3+0]>>3)<<10) |
			                 ((pPal[i*3+1]>>3)<<5)  |
			                 (pPal[i*3+2]>>3));
		/* Treat index 255 as fully transparent; others opaque. */
		if (i == 255)
			out_256[i] = c;
		else
			out_256[i] = (unsigned short)(c | 0x8000);
	}
}
static void DCV_BuildAlphaGradient4444(unsigned char *pPal, unsigned short *out_256)
{
	int i;
	unsigned short r4, g4, b4, rgb;
	int alpha_accum;

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
static void DCV_BuildMaxChannelAlpha(unsigned char *pPal, unsigned short *out_256)
{
	int i, a;

	for (i = 0; i < 256; i++) 
	{
		a = pPal[i*3]; 

		if (pPal[i*3+1] > a) 
			a = pPal[i*3+1]; 

		if (pPal[i*3+2] > a) 
			a = pPal[i*3+2];

		out_256[i] = (unsigned short)(((pPal[i*3]>>4)<<8) | ((pPal[i*3+1]>>4)<<4) | (pPal[i*3+2]>>4) | ((a>>4)<<12));
	}
}
static int DCV_PaletteIsMonochromeQuantized(unsigned char *pPal)
{
	// TODO: do we need this?
	int i; (void)pPal;
	(void)i;
	return 0;
}
static void DCV_BuildLumaRemap(unsigned char *pPal, unsigned short *out_256)
{
	DCV_BuildPalette565(pPal, out_256);
}

/* DC_UploadPalettedTexture: square palettized (calls internal fill). */
static void *DC_UploadPalettedTexture(int side, int _w, int _h, byte *data, unsigned short *palette, dctexture_t *slot, int allow_sysmem_fallback)
{
	byte *idx = data;
	int i, n = side * side;
	void *ret;

	unsigned short *conv = (unsigned short *)malloc((size_t)(n * 2));

	if (!conv) 
		return NULL;

	for (i = 0; i < n; i++)
		conv[i] = palette[idx[i]];

	ret = DC_UploadRaw16(side, side, conv, slot, PVR_RGB565, allow_sysmem_fallback);
	free(conv);
	
	return ret;
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

/* DC_UploadPVRTexture: supports RECT/TWIDDLED/VQ(+mipmap top-level extraction). */
static void *DC_UploadPVRTexture(int w, int h, void *pvr_data, unsigned int *fmt_table, char *name, dctexture_t *slot, int allow_sysmem_fallback)
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

	slot_index = DC_FindTextureSlotByName(identifier, width, height, key);

	if (slot_index >= 0)
	{
		if (s_slotServercount[slot_index] > 0)
		{
			s_slotServercount[slot_index] = gHostSpawnCount;
			s_texSlots[slot_index].nServerCount = (short)gHostSpawnCount;
		}
		return slot_index;
	}

	slot_index = 0;

	while (slot_index < DC_MAXTEXTURES && s_texSlots[slot_index].nServerCount != -1)
		slot_index++;

	if (slot_index >= DC_MAXTEXTURES)
	{
		Sys_Error("DCV_GetSlot: Too many textures.");
		return -1;
	}

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

	DCV_GetRoundedDimensions(tex_type, &out_w, &out_h, width, height, 0, 0, identifier, data);

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
				pSurf = DC_UploadIndexed16(out_w, out_h, (byte *)data, pal_565, slot, PVR_ARGB1555, allow_sysmem_fallback);
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
				pSurf = DC_UploadIndexed16(out_w, out_h, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			else
				pSurf = DC_UploadResampledIndexed16(out_w, out_h, width, height, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
			break;
		case 5:
		case 10:
			pSurf = DC_UploadRaw16(out_w, out_h, (unsigned short *)data, slot, s_slotUse4444[slot_index] ? 2 : 1, allow_sysmem_fallback);
			break;
		case 6:
		case 7:
		case 9:
			DCV_BuildPalette565(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DC_UploadIndexed16(out_w, out_h, (byte *)data, pal_565, slot, PVR_RGB565, allow_sysmem_fallback);
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
			pSurf = DC_UploadPVRTexture(out_w, out_h, data, NULL, identifier, slot, allow_sysmem_fallback);
			break;
		case 11:
			s_slotUse4444[slot_index] = 1;
			DCV_BuildMaxChannelAlpha(pPal, pal_565);
			if (out_w == width && out_h == height)
				pSurf = DC_UploadIndexed16(out_w, out_h, (byte *)data, pal_565, slot, PVR_ARGB4444, allow_sysmem_fallback);
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

void GL_Bind( int stage, int texnum )
{
	int slot_index;
	LPDIRECT3DDEVICE3 dev;
	LPDIRECT3DTEXTURE2 tex;

	texnum = (texnum & 0xFFFF);
	if (gl_nobind.value)
		texnum = char_texture;

	if (stage == 0 && currenttexture == texnum)
		return;

	DCV_Flush();

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

void GL_Texels_f( void )
{
	Con_Printf("Current uploaded texels: %i\n", texels);
}

/****************************************/


void GL_SelectTexture( unsigned int target )
{
	Sys_Error("Use GL_Bind stage argument instead of GL_SelectTexture\n");
}

//=============================================================================
/* Support Routines */

qpic_t* Draw_PicFromWad( char* name )
{
	qpic_t* p;
	glpic_t* gl;

	p = (qpic_t*)W_GetLumpName(name);
	gl = (glpic_t*)p->data;

	gl->texnum = GL_LoadPicTexture(p, name);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return p;
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
===============
Draw_StringLen
===============
*/
int Draw_StringLen( char* psz )
{
	int totalWidth = 0;

	while (psz && *psz)
	{
		totalWidth += draw_chars->fontinfo[*psz].charwidth;
		psz++;
	}
	return totalWidth;
}

int Draw_MessageFontInfo( short* pWidth )
{
	int i;

	if (!draw_creditsfont)
		return 0;

	if (pWidth)
	{
		for (i = 0; i < 256; i++)
			*pWidth++ = draw_creditsfont->fontinfo[i].charwidth;
	}

	return draw_creditsfont->rowheight;
}

int Draw_MessageCharacterAdd( int x, int y, int num, int rr, int gg, int bb )
{
	int				row, col;
	int				rowheight, charwidth;
	float			frow, fcol, xsize, ysize;
	float			uleft, uright, vtop, vbottom;
	int				base;
	LPDIRECT3DDEVICE3 dev;

	num &= 255;

	rowheight = draw_creditsfont->rowheight;
	if (y <= -rowheight)
		return 0;			// totally off screen

	charwidth = draw_creditsfont->fontinfo[num].charwidth;

	col = draw_creditsfont->fontinfo[num].startoffset & 255;
	fcol = col * chars_xsize;
	row = (draw_creditsfont->fontinfo[num].startoffset & ~255) >> 8;
	frow = row * creditsfont_ysize;

	xsize = charwidth * chars_xsize;
	ysize = rowheight * creditsfont_ysize;

	uleft = fcol;
	uright = fcol + xsize;
	vtop = frow;
	vbottom = frow + ysize;

	DCV_TexState_AlphaTest();

	DCV_SetColor(rr, gg, bb, 255);
	GL_DisableMultitexture();
	GL_Bind(0, font_texture);
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,               (float)y,                uleft,  vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)y,                uright, vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)(y + rowheight),  uright, vbottom);
	DCV_2D_AddVertex((float)x,               (float)(y + rowheight),  uleft,  vbottom);

	DCV_Flush();
	DCV_TexState_Opaque();

	return charwidth;
}

void Draw_CharToConback( int num, byte* dest )
{
}

typedef struct
{
	char* name;
	int	minimize, maximize;
} quake_mode_t;

#if 0
quake_mode_t modes[] = {
	{ "GL_NEAREST", GL_NEAREST, GL_NEAREST },
	{ "GL_LINEAR", GL_LINEAR, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }
};
#endif
/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f( void )
{
#if 0
	int		i;
	gltexture_t* glt;

	if (Cmd_Argc() == 1)
	{
		for (i = 0; i < 6; i++)
		{
			if (gl_filter_min == modes[i].minimize)
			{
				Con_Printf("%s\n", modes[i].name);
				return;
			}
		}
		Con_Printf("current filter is unknown???\n");
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (!Q_strcasecmp(modes[i].name, Cmd_Argv(1)))
			break;
	}
	if (i == 6)
	{
		Con_Printf("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->mipmap)
		{
			GL_Bind(0, glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
#endif
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

/*
===============
Draw_Init
===============
*/
void Draw_Init( void )
{
	int				i;
	qpic_t*	cb;
	glpic_t* gl;
	unsigned char* pPal;
	float			prev;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		s_texSlots[i].nServerCount = -1;
		s_texSlots[i].pPrev = NULL;
		s_texSlots[i].pNext = NULL;
		s_texSlots[i].pd3dtTexture = NULL;
		s_slotServercount[i] = -1;
		s_slotUse4444[i] = 0;
		s_slotIsSystem[i] = 0;
	}

	memset(&s_dcTextureLruHead, 0, sizeof(s_dcTextureLruHead));
	s_dcTextureLruHead.pPrev = &s_dcTextureLruHead;
	s_dcTextureLruHead.pNext = &s_dcTextureLruHead;
	s_nCached = 0;
	texels = 0;
	s_pCurrentTextureSurface = NULL;
	s_nD3dCurrentTexnum = -1;

	Draw_CacheWadInit("cached.wad", 16, &menu_wad);
	menu_wad.tempWad = TRUE;

	Draw_CacheWadHandler(&decal_wad, Draw_MiptexTexture, MIP_EXTRASIZE);
	Draw_CacheWadHandler(&custom_wad, Draw_MiptexTexture, MIP_EXTRASIZE);

	Cvar_RegisterVariable(&gl_nobind);
	Cvar_RegisterVariable(&gl_max_size);
	Cvar_RegisterVariable(&gl_round_down);
	Cvar_RegisterVariable(&gl_picmip);
	Cvar_RegisterVariable(&gl_palette_tex);

	memset(decal_names, 0, sizeof(decal_names));

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

	chars_xsize = 1.0 / 256;
	chars_ysize = 1.0 / draw_chars->height;

	creditsfont_ysize = 1.0 / draw_creditsfont->height;

	draw_chars   = draw_creditsfont;
	char_texture = font_texture;
	chars_ysize  = creditsfont_ysize;

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
	int				row, col;
	int				rowheight, charwidth;
	float			frow, fcol, xsize, ysize;
	float			uleft, uright, vtop, vbottom;
	int				base;

	num &= 255;

	rowheight = draw_chars->rowheight;
	if (y <= -rowheight)
		return 0;			// totally off screen

	charwidth = draw_chars->fontinfo[num].charwidth;
	if (y < 0 || num == 32)
		return charwidth;		// space

	col = draw_chars->fontinfo[num].startoffset & 255;
	fcol = col * chars_xsize;
	row = (draw_chars->fontinfo[num].startoffset & ~255) >> 8;
	frow = row * chars_ysize;

	xsize = charwidth * chars_xsize;
	ysize = rowheight * chars_ysize;
	
	uleft = fcol;
	uright = fcol + xsize;
	vtop = frow;
	vbottom = frow + ysize;

	DCV_TexState_Blend();
	DCV_SetColor(255, 128, 0, 255);
	GL_Bind(0, char_texture);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,               (float)y,                uleft,  vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)y,                uright, vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)(y + rowheight),  uright, vbottom);
	DCV_2D_AddVertex((float)x,               (float)(y + rowheight),  uleft,  vbottom);

	return charwidth;
}

/*
================
Draw_CharacterSlant

Draws one character with bottom edge shifted right by slant pixels (italic).
================
*/
int Draw_CharacterSlant( int x, int y, int num, int slant )
{
	int				row, col;
	int				rowheight, charwidth;
	float			frow, fcol, xsize, ysize;
	float			uleft, uright, vtop, vbottom;
	int				base;

	num &= 255;

	rowheight = draw_chars->rowheight;
	if (y <= -rowheight)
		return 0;

	charwidth = draw_chars->fontinfo[num].charwidth;
	if (y < 0 || num == 32)
		return charwidth;

	col = draw_chars->fontinfo[num].startoffset & 255;
	fcol = col * chars_xsize;
	row = (draw_chars->fontinfo[num].startoffset & ~255) >> 8;
	frow = row * chars_ysize;

	xsize = charwidth * chars_xsize;
	ysize = rowheight * chars_ysize;

	uleft = fcol;
	uright = fcol + xsize;
	vtop = frow;
	vbottom = frow + ysize;

	DCV_TexState_Blend();
	DCV_SetColor(255, 128, 0, 255);
	GL_Bind(0, char_texture);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	/* Shear: top edge normal, bottom edge shifted right by slant */
	DCV_2D_AddVertex((float)x,                (float)y,                uleft,  vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)y,                uright, vtop);
	DCV_2D_AddVertex((float)(x + charwidth + slant), (float)(y + rowheight), uright, vbottom);
	DCV_2D_AddVertex((float)(x + slant),            (float)(y + rowheight), uleft,  vbottom);

	return charwidth;
}

/*
================
Draw_CharacterSlantColor

Draw_CharacterSlant but with explicit RGB 
================
*/
int Draw_CharacterSlantColor( int x, int y, int num, int slant, int r, int g, int b )
{
	int				row, col;
	int				rowheight, charwidth;
	float			frow, fcol, xsize, ysize;
	float			uleft, uright, vtop, vbottom;
	int				base;

	num &= 255;

	rowheight = draw_chars->rowheight;
	if (y <= -rowheight)
		return 0;

	charwidth = draw_chars->fontinfo[num].charwidth;
	if (y < 0 || num == 32)
		return charwidth;

	col = draw_chars->fontinfo[num].startoffset & 255;
	fcol = col * chars_xsize;
	row = (draw_chars->fontinfo[num].startoffset & ~255) >> 8;
	frow = row * chars_ysize;

	xsize = charwidth * chars_xsize;
	ysize = rowheight * chars_ysize;

	uleft = fcol;
	uright = fcol + xsize;
	vtop = frow;
	vbottom = frow + ysize;

	DCV_TexState_Blend();
	DCV_SetColor(r, g, b, 255);
	GL_Bind(0, char_texture);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,                (float)y,                uleft,  vtop);
	DCV_2D_AddVertex((float)(x + charwidth), (float)y,                uright, vtop);
	DCV_2D_AddVertex((float)(x + charwidth + slant), (float)(y + rowheight), uright, vbottom);
	DCV_2D_AddVertex((float)(x + slant),            (float)(y + rowheight), uleft,  vbottom);

	return charwidth;
}

int Draw_StringSlantColor( int x, int y, int slant, int r, int g, int b, char* str )
{
	while (str && *str)
	{
		x += Draw_CharacterSlantColor(x, y, *str, slant, r, g, b);
		str++;
	}
	return x;
}

int Draw_StringSlantColorScale( int x, int y, int slant, int r, int g, int b, float sx, float sy, char* str )
{
	int				row, col, num, base;
	int				rowheight, charwidth, adv;
	float			frow, fcol, xsize, ysize;
	float			uleft, uright, vtop, vbottom;
	float			w, h;

	if (!str)
		return x;

	while (*str)
	{
		num = ((unsigned char)*str) & 255;
		rowheight = draw_chars->rowheight;
		charwidth = draw_chars->fontinfo[num].charwidth;

		if (y > -rowheight && num != 32)
		{
			col = draw_chars->fontinfo[num].startoffset & 255;
			fcol = col * chars_xsize;
			row = (draw_chars->fontinfo[num].startoffset & ~255) >> 8;
			frow = row * chars_ysize;

			xsize = charwidth * chars_xsize;
			ysize = rowheight * chars_ysize;
			uleft = fcol;
			uright = fcol + xsize;
			vtop = frow;
			vbottom = frow + ysize;

			w = charwidth * sx;
			h = rowheight * sy;
			DCV_SetColor(r, g, b, 255);
			GL_Bind(0, char_texture);
			DCV_FlushIfLarge();
			base = DCV_GetVertCount();
			DCV_AddPolyIndices(base, 4);
			DCV_2D_AddVertex((float)x,              (float)y,       uleft,  vtop);
			DCV_2D_AddVertex((float)(x + w),        (float)y,       uright, vtop);
			DCV_2D_AddVertex((float)(x + w + slant),(float)(y + h), uright, vbottom);
			DCV_2D_AddVertex((float)(x + slant),    (float)(y + h), uleft,  vbottom);
		}

		adv = (int)(charwidth * sx + 0.5f);
		if (adv < 1)
			adv = 1;
		x += adv;
		str++;
	}
	return x;
}

/*
================
Draw_GetRowHeight

Current font line height for menu step 
================
*/
int Draw_GetRowHeight( void )
{
	return draw_chars ? draw_chars->rowheight : 16;
}

/*
================
Draw_StringSlant
================
*/
int Draw_StringSlant( int x, int y, int slant, char* str )
{
	while (str && *str)
	{
		x += Draw_CharacterSlant(x, y, *str, slant);
		str++;
	}
	return x;
}

/*
================
Draw_String
================
*/
int Draw_String( int x, int y, char* str )
{
	while (*str)
	{
		x += Draw_Character(x, y, *str);
		str++;
	}
	return x;
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar( char num )
{
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
	GL_Bind(0, gl->texnum);
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
	GL_Bind(0, gl->texnum);
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
Draw_PicByTexnum
=============
Draw a 2D quad by texture slot (full UV 0..1). Used for menu PVR textures.
*/
void Draw_PicByTexnum( int x, int y, int w, int h, int texnum )
{
	int base;

	if (texnum < 0)
		return;

	DCV_SetColor(255, 255, 255, 255);
	GL_Bind(0, texnum);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,        (float)y,         0.0f, 0.0f);
	DCV_2D_AddVertex((float)(x + w),  (float)y,         1.0f, 0.0f);
	DCV_2D_AddVertex((float)(x + w),  (float)(y + h),  1.0f, 1.0f);
	DCV_2D_AddVertex((float)x,        (float)(y + h),  0.0f, 1.0f);
}

/*
=============
Draw_PicByTexnumMirrorV

Same as Draw_PicByTexnum but V flipped (mirror below). For title logo reflection.
Optional alpha 0..255; 0 = use 255.
=============
*/
void Draw_PicByTexnumMirrorV( int x, int y, int w, int h, int texnum, int alpha )
{
	int base;

	if (texnum < 0)
		return;
	if (alpha <= 0)
		alpha = 255;

	DCV_SetColor(255, 255, 255, alpha);
	GL_Bind(0, texnum);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	/* V flipped: top pixel row = bottom of texture */
	DCV_2D_AddVertex((float)x,        (float)y,         0.0f, 1.0f);
	DCV_2D_AddVertex((float)(x + w),  (float)y,         1.0f, 1.0f);
	DCV_2D_AddVertex((float)(x + w),  (float)(y + h),  1.0f, 0.0f);
	DCV_2D_AddVertex((float)x,        (float)(y + h),  0.0f, 0.0f);
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic( int x, int y, qpic_t* pic )
{
	if (!pic)
		return;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		(unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error("Draw_TransPic: bad coordinates");
	}

	Draw_Pic(x, y, pic);
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
int ValidateWRect( const wrect_t* prc )
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

	GL_Bind(0, pFrame->gl_texturenum);
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

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground( int lines )
{
	char ver[100];
	int x;

	Draw_Pic2(0, lines - glheight, glwidth, glheight + 1, conback);

	sprintf(ver, "Half-Life 39/1.0.1.3 (build DC %d)", build_number()); // TODO actual string should be from Host_Version

	x = vid.conwidth - Draw_StringLen(ver);
	if (!con_loading && !(giSubState & 4))
	{
		Draw_String(x, 0, ver);
	}
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

	if (w <= 0 || h <= 0)
		return;

	DCV_TexState_VertColor();
	DCV_SetColor(r, g, b, a);
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_2D_AddVertex((float)x,         (float)y,         0.0f, 0.0f);
	DCV_2D_AddVertex((float)(x + w),   (float)y,         1.0f, 0.0f);
	DCV_2D_AddVertex((float)(x + w),   (float)(y + h),   1.0f, 1.0f);
	DCV_2D_AddVertex((float)x,         (float)(y + h),   0.0f, 1.0f);
	DCV_Flush();
	DCV_SetPackedColor(0xFFFFFFFF);
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
//=============================================================================

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen( void )
{
#if 0
	qglEnable(GL_BLEND);
	qglDisable(GL_TEXTURE_2D);
	qglColor4f(0, 0, 0, 0.8);
	qglBegin(GL_QUADS);

	qglVertex2f(0, 0);
	qglVertex2f(glwidth, 0);
	qglVertex2f(glwidth, glheight);
	qglVertex2f(0, glheight);

	qglEnd();
	qglColor4f(1, 1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
#endif
}

//=============================================================================

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
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc( void )
{
}

void ComputeScaledSize( int* wscale, int* hscale, int width, int height )
{
	int scaled_width, scaled_height;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;

	if (gl_round_down.value > 0 && 
		width < scaled_width && 
		(gl_round_down.value == 1 || (scaled_width - width) > (scaled_width >> (int)gl_round_down.value)))
		scaled_width >>= 1;

	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;

	if (gl_round_down.value > 0 && 
		height < scaled_height && 
		(gl_round_down.value == 1 || (scaled_height - height) > (scaled_height >> (int)gl_round_down.value)))
		scaled_height >>= 1;

	if (wscale)
		*wscale = min(scaled_width >> (int)gl_picmip.value, (int)gl_max_size.value);
	if (hscale)
		*hscale = min(scaled_height >> (int)gl_picmip.value, (int)gl_max_size.value);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
int GL_FindTexture( char* identifier )
{
	int i;

	if (!identifier || !identifier[0])
		return -1;

	for (i = 0; i < DC_MAXTEXTURES; i++)
	{
		if (s_texSlots[i].nServerCount == -1)
			continue;
		if (s_texSlots[i].pszName && strcmp(identifier, s_texSlots[i].pszName) == 0)
			return i;
	}
	return -1;
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

void GL_PaletteClearSky( void )
{
	Sys_Error("GL_PaletteClearSky no longer used\n");
}


void GL_PaletteSelect( int paletteIndex )
{
	Sys_Error("GL_PaletteSelect no longer used\n");
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


qpic_t* LoadTransBMP( char* pszName )
{
	return LoadTransPic(pszName, (qpic_t*)W_GetLumpName(pszName));
}

qpic_t* LoadTransPic( char* pszName, qpic_t* ppic )
{
	int		i, slot_index;
	byte* pPal;
	glpic_t* gl;
	qpic_t* ppicNew;

	if (!ppic)
		return NULL;

	ppicNew = (qpic_t*)MnemoAlloc(sizeof(qpic_t) + sizeof(glpic_t), 0x20, 0, "LoadTransPic");
	gl = (glpic_t*)ppicNew->data;

	ppicNew->width = ppic->width;
	ppicNew->height = ppic->height;

	/* See if the texture is already present in DC slot table */
	if (pszName[0])
	{
		slot_index = GL_FindTexture(pszName);
		if (slot_index >= 0)
		{
			if (s_texSlots[slot_index].nSrcWidth == ppic->width && s_texSlots[slot_index].nSrcHeight == ppic->height)
			{
				gl->texnum = slot_index;
				gl->sl = 0; gl->sh = 1; gl->tl = 0; gl->th = 1;
				return ppicNew;
			}
			pszName[3]++;
			/* retry with modified name */
			return LoadTransPic(pszName, ppic);
		}
	}

	pPal = &ppic->data[ppic->width * ppic->height + 2];
	slot_index = GL_LoadTexture(pszName, GLT_SYSTEM, ppic->width, ppic->height, ppic->data, FALSE, TEX_TYPE_ALPHA, pPal);
	if (slot_index < 0)
		slot_index = 0;

	gl->texnum = (slot_index & 0xFFFF);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return ppicNew;
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

void Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad )
{
	int		h[3];
	int		nFileSize;
	lumpinfo_t* lump_p;
	wadinfo_t header;
	int		i;

	nFileSize = COM_OpenFile(name, h);
	if (h[2] == -1)
		Sys_Error("Draw_LoadWad: Couldn't open %s\n", name);

	Sys_FileRead(h[2], &header, sizeof(header));

	if (header.identification[0] != 'W'
	  || header.identification[1] != 'A'
	  || header.identification[2] != 'D'
	  || header.identification[3] != '3')
	{
		Sys_Error("Wad file %s doesn't have WAD3 id\n", name);
	}

	wad->lumps = (lumpinfo_t*)MnemoAlloc(nFileSize - header.infotableofs, 0x20, 0, "wadlumps");

	COM_FileSeek(h[0], h[1], h[2], header.infotableofs);
	Sys_FileRead(h[2], wad->lumps, nFileSize - header.infotableofs);
	COM_CloseFile(h[0], h[1], h[2]);

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

void Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize )
{
	wad->cacheExtra = extraDataSize;
	wad->pfnCacheBuild = fn;
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
	char* pName;

	pName = decal_names[id];
	if (!pName[0])
		Sys_Error("Used decal #%d without no name\n", id);

	return Draw_CacheIndex(&decal_wad, pName);
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

int Draw_DecalCount( void )
{
	return decal_wad.lumpCount;
}

int Draw_DecalSize( int number )
{
	if (number >= decal_wad.lumpCount)
		return 0;

	return decal_wad.lumps[number].size;
}

char* Draw_DecalName( int number )
{
	if (number >= decal_wad.lumpCount)
		return 0;
	
	return decal_wad.lumps[number].name;
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
/*
================
DCV_SetHudDepth

Point the HUD transform at one of the 2D depth sublayers between dc_msh and
dc_msh2. TODO: reconstruct the transform/depth-range setup (FUN_00140d94); the
screen fade and progress overlays still draw at dc_depthhud without it.
================
*/
void DCV_SetHudDepth( float layer )
{
}

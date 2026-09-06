// dc_draw.cpp -- 2D drawing and the DirectDraw/Direct3D texture cache.
// CINTERFACE selects the C-style (lpVtbl) DirectX interfaces.
#define CINTERFACE

#include "quakedef.h"
#include "winquake.h"
#include "cl_draw.h"
#include "decal.h"
#include "pr_cmds.h"
#include "wad.h"
#include "dc_accum.h"
#include "qgl.h"

typedef unsigned char byte;

cvar_t		gl_nobind = { "gl_nobind", "0" };
cvar_t		gl_max_size = { "gl_max_size", "256" };
cvar_t		gl_round_down = { "gl_round_down", "3" };
cvar_t		gl_picmip = { "gl_picmip", "0" };
cvar_t		gl_palette_tex = { "gl_palette_tex", "1" };

void*		g_bTextureLog = NULL;	/* open log-file handle; nonzero => trace texture cache (set by dc_texdump) */

qfont_t* draw_chars;
qpic_t* draw_disc;
qpic_t* draw_backtile;

int			translate_texture;
int			char_texture;

/* Draw_Init runs again on every level change; only register commands and load
   the one-time pics the first time through. */
static short	s_bFirstTime = TRUE;

typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;

byte		conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t* conback = (qpic_t*)&conback_buffer;

int		texels;

/* Expand a four-bit color component to the full byte range. */
static const unsigned int s_color4To8[16] =
{
	0x00, 0x11, 0x22, 0x33,
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB,
	0xCC, 0xDD, 0xEE, 0xFF
};

#ifdef __cplusplus
extern "C" {
#endif

unsigned short PutRGB( colorVec* pcv )
{
	byte* color = (byte*)pcv;
	unsigned int alpha = color[12];
	unsigned int red = color[0];
	unsigned int green = color[4];
	unsigned int blue = color[8];

	alpha >>= 4;
	red >>= 4;
	green >>= 4;
	blue >>= 4;

	return (unsigned short)((alpha << 12) | (red << 8) |
		(green << 4) | blue);
}

void GetRGB( unsigned short color, colorVec* pcv )
{
	unsigned int alpha;
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	const unsigned int* color4To8 = s_color4To8;

	alpha = color4To8[(color & 0xF000) >> 12];
	red = color4To8[(color & 0x0F00) >> 8];
	green = color4To8[(color & 0x00F0) >> 4];
	blue = color4To8[color & 0x000F];

	pcv->a = alpha;
	pcv->r = red;
	pcv->g = green;
	pcv->b = blue;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern cachewad_t	custom_wad;
extern cachewad_t	*menu_wad;
extern short		m_bDrawInitialized;

int			numgltextures;
int			nada_texture;
#ifdef __cplusplus
}
#endif


#define MAX_D3D_TEXTURES 1140
#define SURFACE_CAPS_PALETTE8 0x80007000
class dc_texture_s
{
public:
	dc_texture_s();
	~dc_texture_s();

	short   iPalette;
	short   nScaledWidth;
	short   nScaledHeight;
	short   nSrcWidth;
	short   nSrcHeight;
	short   nUsageCount;
	short   nServerCount;		/* -1 = free */
	short   reserved;
	int     iTextureKey;
	int     iTextureType;
	void   *pddsSurface;
	void   *pd3dtTexture;
	char   *pszName;
	int     cbData;
	void   *pCacheBlock;
	unsigned char *pbCacheData;
	int     bCached;
	void   *pPrev;
	void   *pNext;
};
typedef dc_texture_s dctexture_t;

/* Palette pool: DDraw palettes are a scarce resource, so paletted textures
   share from a small pool keyed by a tag of the palette bytes. */
class dcpalette_t
{
public:
	dcpalette_t();
	~dcpalette_t();

	int					tag;			/* -1 = free */
	int					referenceCount;
	LPDIRECTDRAWPALETTE	lpPalette;
};

#define MAX_D3D_PALETTES	4

static dcpalette_t	gGLPalette[MAX_D3D_PALETTES];

static dctexture_t	s_texSlots[MAX_D3D_TEXTURES];
static dctexture_t	s_dcTextureLruHead;
static dctexture_t	s_dcTextureLruTail;
static int			s_nCached;
static LPDIRECTDRAWSURFACE4 s_pCurrentTextureSurface;
static int			s_slotServercount[MAX_D3D_TEXTURES];
static int			s_slotUse4444[MAX_D3D_TEXTURES];
static int			s_slotIsSystem[MAX_D3D_TEXTURES];  /* GLT_SYSTEM: never cache/decache */
static int			s_bTexReclaimGuard;
static int			s_bUncacheGuard;
static int			s_nD3dCurrentTexnum = -1;
static int			s_stageTexnum[2] = { -1, -1 };	/* last texnum bound per D3D texture stage */

/* Localized text/font renderer (text_draw.cpp). The font object is an internal
   dcfont_t; callers here pass draw_chars (qfont_t*), matched by cast at the def. */
#ifdef __cplusplus
extern "C" {
#endif
void Draw_Shutdown( void );
void DCV_GammaRefresh_f( void );
qpic_t* LoadTransPic( char* pszName, qpic_t* ppic );
extern void	Font_SetScale( float sx, float sy );
extern int	Font_StringWidth( qfont_t* font, char* str );
extern int	Font_CharHeight( qfont_t* font );
extern int	Font_DrawCharI( qfont_t* font, int x, int y, int num );
void DC_FreeTextureSlot(dctexture_t *slot);
int DC_ReclaimTextureSlot( void );
int DC_ReleaseTexture( int texnum );
int DC_FreeStaleTextureSlots( void );
int GL_PaletteTag( byte* pPal );
int GL_UnloadTextures( void );
void GL_BindStage( int texnum, int stage );
void DC_InitTextureList( void );
int DCV_UpdateTextureSubRect( int texnum, int x, int y, int w, int h, const unsigned short* src, int src_pitch );
void DC_TouchTexture( int texnum );
extern float g_flHudDepth;
#ifdef __cplusplus
}
#endif

static void DC_DecacheTextureSlot( dctexture_t *slot );
static void DC_CacheTextureToRam( dctexture_t *slot );
static __inline void DCV_UnlinkTextureSlot( dctexture_t *slot );
static __inline void DCV_LinkTextureSlotFront( dctexture_t *slot );
static void DCV_TouchTextureSlot( dctexture_t *slot );
static int DCV_AttachTextureInterface( dctexture_t *slot, LPDIRECTDRAWSURFACE4 surf );


/* Remove a slot from the LRU list. Callers have already validated the slot. */
static __inline void DCV_UnlinkTextureSlot( dctexture_t *slot )
{
	if (slot->pPrev)
		((dctexture_t *)slot->pPrev)->pNext = slot->pNext;
	if (slot->pNext)
		((dctexture_t *)slot->pNext)->pPrev = slot->pPrev;
	slot->pNext = NULL;
	slot->pPrev = NULL;
}

static __inline void DCV_LinkTextureSlotFront( dctexture_t *slot )
{
	dctexture_t *first;

	DCV_UnlinkTextureSlot(slot);

	first = (dctexture_t *)s_dcTextureLruHead.pNext;
	slot->pNext = first;
	slot->pPrev = &s_dcTextureLruHead;
	first->pPrev = slot;
	((dctexture_t *)slot->pPrev)->pNext = slot;
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

	if (FAILED(surf->lpVtbl->QueryInterface(surf, IID_IDirect3DTexture2, (void **)&tex)))
		return 0;

	slot->pd3dtTexture = tex;
	return 1;
}

static __inline void DCV_ClearCachedFlag( void )
{
	dctexture_t *s;

	for (s = (dctexture_t *)s_dcTextureLruTail.pPrev; s != NULL; s = (dctexture_t *)s->pPrev)
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

int DC_CreatePalette( dcpalette_t* entry, byte* pPal, int tag )
{
	PALETTEENTRY entries[256];
	PALETTEENTRY *pe;
	int i;

	if (entry->lpPalette)
	{
		entry->lpPalette->lpVtbl->Release(entry->lpPalette);
		entry->lpPalette = NULL;
	}

	pe = entries;
	i = 0;
	do {
		pe->peRed = pPal[0];
		pe->peGreen = pPal[1];
		pe->peBlue = pPal[2];
		pPal += 3;
		pe++;
	} while (++i < 256);

	if (DCV_DDError(g_pDD4->lpVtbl->CreatePalette(g_pDD4,
			DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, &entry->lpPalette, NULL),
			TEXT("CreatePalette")))
		return 0;

	entry->tag = tag;
	return 1;
}

int DC_GetPaletteIndex( byte* pPal )
{
	int tag;
	int i;

	tag = GL_PaletteTag(pPal);

	for (i = 0; i < MAX_D3D_PALETTES; i++)
	{
		if (gGLPalette[i].tag < 0)
		{
			if (!DC_CreatePalette(&gGLPalette[i], pPal, tag))
				return -1;

			gGLPalette[i].referenceCount++;
			return (short)i;
		}

		if (GL_PaletteEqual(&gGLPalette[i], pPal, tag))
		{
			gGLPalette[i].referenceCount++;
			return (short)i;
		}
	}

	return -1;
}

void DC_FreeTextureSlot(dctexture_t *slot)
{
	GL_BindStage(0, 0);

	DCV_UnlinkTextureSlot(slot);

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Freeing texture %s\n",
			slot->pszName ? slot->pszName : "<nameless>");

	if (slot->iPalette != -1 && slot->pddsSurface)
		DCV_DDError(((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->SetPalette(
				(LPDIRECTDRAWSURFACE4)slot->pddsSurface, NULL),
			TEXT("Palette unlink"));

	if (slot->pbCacheData)
	{
		MnemoFree(slot->pCacheBlock);
		slot->pbCacheData = NULL;
		slot->pCacheBlock = NULL;
		slot->bCached = 0;
		s_nCached--;
	}
	else
	{
		if (slot->pd3dtTexture)
			((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
		slot->pd3dtTexture = NULL;

		if (slot->pddsSurface)
			DCV_DDError(((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Release(
					(LPDIRECTDRAWSURFACE4)slot->pddsSurface),
				TEXT("SurfaceRelease"));
		slot->pddsSurface = NULL;
	}

	if (slot->pszName)
		::operator delete(slot->pszName);
	slot->pszName = NULL;
	slot->nServerCount = -1;
}

static void DC_CacheTextureToRam( dctexture_t *slot )
{
	DDSURFACEDESC2 *pdd;
	unsigned short *src, *dst;
	int i;

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Attempting to cache texture %s\n",
			slot->pszName ? slot->pszName : "<nameless>");

	if (slot->bCached)
		Sys_Error("Reattempted texture to RAM cache!\n");

	slot->bCached = 1;

	if (slot->pbCacheData == NULL && slot->cbData > 0 && !Mnemo_LastChanceActive())
	{
		slot->pCacheBlock = MnemoAlloc(slot->cbData + sizeof(DDSURFACEDESC2), 0x8000A0, 0,
			Bmakename(slot->pszName ? slot->pszName : "<nameless>", 3));
		if (slot->pCacheBlock)
		{
			pdd = (DDSURFACEDESC2 *)slot->pCacheBlock;
			slot->pbCacheData = (unsigned char *)slot->pCacheBlock + sizeof(DDSURFACEDESC2);

			((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->GetSurfaceDesc(
				(LPDIRECTDRAWSURFACE4)slot->pddsSurface, pdd);

			if (((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Lock(
					(LPDIRECTDRAWSURFACE4)slot->pddsSurface, NULL, pdd, DDLOCK_WAIT, NULL) == DD_OK)
			{
				src = (unsigned short *)pdd->lpSurface;
				dst = (unsigned short *)slot->pbCacheData;
				i = 0;
				if (slot->cbData > 0)
					do {
						*dst = *src;
						src++;
						dst++;
						i += 2;
					} while (i < slot->cbData);

				((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Unlock(
					(LPDIRECTDRAWSURFACE4)slot->pddsSurface, NULL);

				if (slot->iPalette != -1 && slot->pddsSurface)
					DCV_DDError(((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->SetPalette(
						(LPDIRECTDRAWSURFACE4)slot->pddsSurface, NULL), TEXT("Palette unlink"));

				((LPDIRECT3DTEXTURE2)slot->pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)slot->pd3dtTexture);
				((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->Release((LPDIRECTDRAWSURFACE4)slot->pddsSurface);
				slot->pd3dtTexture = NULL;
				slot->pddsSurface = NULL;

				if (g_bTextureLog)
					Sys_FPrintf(g_bTextureLog, "Cached texture %s\n",
						slot->pszName ? slot->pszName : "<nameless>");

				s_nCached++;
			}
		}
	}
}

static void DC_DecacheTextureSlot( dctexture_t *slot )
{
	DDSURFACEDESC2 *pdd;
	LPDIRECTDRAWSURFACE4 surf;
	unsigned short *src, *dst;
	int i;
	int reclaimed;

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Attempting to decache texture %s\n",
			slot->pszName ? slot->pszName : "<nameless>");

	if (!slot->pbCacheData)
		return;

	if (slot->pddsSurface)
		Sys_Error("Cache data was nonnull but so was surface.\n");

	pdd = (DDSURFACEDESC2 *)slot->pCacheBlock;

	DCV_ClearCachedFlag();

	/* Recreate the surface from the cached DDSURFACEDESC2, reclaiming VRAM on failure. */
	{
		LPDIRECTDRAWSURFACE4 _out;
		do {
			if (g_pDD4->lpVtbl->CreateSurface(g_pDD4, pdd, &_out, NULL) == DD_OK)
			{
				surf = _out;
				goto got_surface;
			}
			reclaimed = 0;
			if (!s_bTexReclaimGuard)
			{
				s_bTexReclaimGuard = 1;
				reclaimed = DC_ReclaimTextureSlot();
				s_bTexReclaimGuard = 0;
			}
		} while (reclaimed);
		surf = NULL;
	got_surface:;
	}

	slot->pddsSurface = surf;
	if (!surf)
		return;

	if (surf->lpVtbl->Lock(surf, NULL, pdd, DDLOCK_WAIT, NULL) != DD_OK)
	{
		surf->lpVtbl->Release(surf);
		slot->pddsSurface = NULL;
		return;
	}

	if (slot->iPalette != -1 && slot->pddsSurface)
		((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->SetPalette(
			(LPDIRECTDRAWSURFACE4)slot->pddsSurface,
			gGLPalette[slot->iPalette].lpPalette);

	src = (unsigned short *)slot->pbCacheData;
	dst = (unsigned short *)pdd->lpSurface;
	i = 0;
	if (slot->cbData > 0)
		do {
			*dst = *src;
			src++;
			dst++;
			i += 2;
		} while (i < slot->cbData);

	surf->lpVtbl->Unlock(surf, NULL);

	if (DCV_DDError(((LPDIRECTDRAWSURFACE4)slot->pddsSurface)->lpVtbl->QueryInterface(
			(LPDIRECTDRAWSURFACE4)slot->pddsSurface, IID_IDirect3DTexture2,
			(void **)&slot->pd3dtTexture), TEXT("Get texture from surface")))
		Sys_Error("Ugh! Texture from surface failed.\n");

	MnemoFree(slot->pCacheBlock);
	slot->pbCacheData = NULL;
	slot->pCacheBlock = NULL;
	slot->bCached = 0;

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Decached texture %s\n",
			slot->pszName ? slot->pszName : "<nameless>");

	s_nCached--;
}

dc_texture_s::dc_texture_s()
{
	pddsSurface = NULL;
	pd3dtTexture = NULL;
	pszName = NULL;
	iPalette = -1;
	nScaledWidth = 0;
	nScaledHeight = 0;
	nSrcWidth = 0;
	nSrcHeight = 0;
	nServerCount = -1;
	pbCacheData = NULL;
	pCacheBlock = NULL;
	bCached = 0;
	pPrev = NULL;
	pNext = NULL;
}

dc_texture_s::~dc_texture_s()
{
	if (pd3dtTexture)
		((LPDIRECT3DTEXTURE2)pd3dtTexture)->lpVtbl->Release((LPDIRECT3DTEXTURE2)pd3dtTexture);
	if (pddsSurface)
		((LPDIRECTDRAWSURFACE4)pddsSurface)->lpVtbl->Release((LPDIRECTDRAWSURFACE4)pddsSurface);
	if (pszName)
	{
		::operator delete(pszName);
		pszName = NULL;
	}
	nServerCount = -1;
}

static void DC_SetupTextureSlot( dctexture_t *slot, LPDIRECTDRAWSURFACE4 pSurf, LPDIRECT3DTEXTURE2 pTex, char *name, short palIndex, short scaledWidth, short scaledHeight, short srcWidth, short srcHeight )
{
	slot->pddsSurface = pSurf;
	slot->pd3dtTexture = pTex;
	slot->nUsageCount = 0;

	if (slot->pszName)
	{
		::operator delete(slot->pszName);
		slot->pszName = NULL;
	}

	if (!name)
	{
		slot->pszName = (char *)::operator new(3);
		strcpy(slot->pszName, "?!?");
	}
	else
	{
		slot->pszName = (char *)::operator new(strlen(name) + 1);
		strcpy(slot->pszName, name);
	}

	slot->iPalette = palIndex;
	slot->nScaledWidth = scaledWidth;
	slot->nScaledHeight = scaledHeight;
	slot->nSrcWidth = srcWidth;
	slot->nSrcHeight = srcHeight;

	slot->bCached = 0;
	slot->cbData = 0;
	slot->pbCacheData = NULL;
	slot->pCacheBlock = NULL;

	DCV_LinkTextureSlotFront(slot);

	if (slot->nServerCount)
		slot->nServerCount = (short)gHostSpawnCount;

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Set up texture %s\n",
			slot->pszName ? slot->pszName : "<nameless>");
}

extern "C" void DC_TexCache( char *name )
{
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		if (slot->nServerCount != -1)
		{
			char *slotname = slot->pszName ? slot->pszName : "<nameless>";

			if (strcmp(slotname, name) == 0)
				DC_CacheTextureToRam(slot);
		}
	}
}

void DC_TexDump( void )
{
	int i, count;

	count = 0;
	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		if (s_texSlots[i].nServerCount != -1)
			count++;
	}

	Con_Printf("%d textures loaded, max %d\n", count, numgltextures + 1);

	if (g_bTextureLog)
	{
		Sys_FPrintf(g_bTextureLog, "Texture log end.\n");
		Sys_CloseHandle(g_bTextureLog);
		g_bTextureLog = NULL;
	}
}

void DC_TexDump_f( void )
{
	DC_TexDump();
}

int DC_FindTextureSlot(char *name, int width, int height, unsigned int key)
{
	int i;

	for (;;)
	{
		for (i = 0; i < MAX_D3D_TEXTURES; i++)
		{
			dctexture_t *slot = &s_texSlots[i];

			if (slot->nServerCount != -1)
			{
				char *slotname;

				if ((unsigned int)slot->iTextureKey == key && key != 0)
					return i;

				slotname = slot->pszName ? slot->pszName : "<nameless>";
				if (strcmp(name, slotname) == 0)
					break;
			}
		}

		if (i >= MAX_D3D_TEXTURES)
			return -1;

		if (s_texSlots[i].nSrcWidth == width && s_texSlots[i].nSrcHeight == height)
			return i;

		/* Same name, different size: mangle the requested name and try again. */
		name[3]++;
	}
}

int DCV_GetSlot( void )
{
	int i;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
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
		*out_w = ((pvrheader_t *)pvrt)->width;
		*out_h = ((pvrheader_t *)pvrt)->height;
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
	unsigned i = 0;

	do {
		unsigned r = *pPal++;
		unsigned g = *pPal++;
		unsigned b = *pPal++;
		*out++ = (unsigned short)(((r>>3)<<10) | 0x8000 | ((g>>3)<<5) | (b>>3));
	} while (++i < 256);

	/* Index 255 is the fully transparent entry. */
	out_256[255] = 0;
}

void DCV_BuildPalette565(unsigned char *pPal, unsigned short *out_256)
{
	unsigned short *out = out_256;
	unsigned i = 0;

	do {
		unsigned r = *pPal++;
		unsigned g = *pPal++;
		unsigned b = *pPal++;
		*out++ = (unsigned short)(((r>>3)<<11) | ((g>>2)<<5) | (b>>3));
	} while (++i < 256);
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
	unsigned i = 0;

	do {
		unsigned r = pPal[0];
		unsigned g = pPal[1];
		unsigned mx = r;
		unsigned a;

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
	unsigned i = 0;

	do {
		unsigned r = *pPal++;
		unsigned g = *pPal++;
		unsigned b = *pPal++;
		*out_256++ = (byte)((r + g + b) / 3);
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

		if (slot)
		{
			while (!slot->pbCacheData && (slot = (dctexture_t *)slot->pPrev) != NULL)
				;
		}

		if (slot)
			DC_DecacheTextureSlot(slot);
	}

	s_bUncacheGuard = 0;
	return 0;
}

int DC_ReclaimTextureSlot( void )
{
	dctexture_t *slot;

	slot = (dctexture_t *)s_dcTextureLruTail.pPrev;

	for (;;)
	{
		if (slot == &s_dcTextureLruHead || slot == NULL)
		{
			GL_UnloadTextures();
			return 0;
		}

		if (g_bTextureLog)
			Sys_FPrintf(g_bTextureLog, "Walking texture %s\n",
				slot->pszName ? slot->pszName : "<nameless>");

		if (!slot->pbCacheData && !slot->bCached)
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
/* Pixel-format descriptors (g_pf*), g_pDD4, and DCV_DDError are declared in
   glquake.h and defined in dc_d3d.c. DC_LoadTexture selects a format per texture
   class and passes it by address to the prep helpers. */

/* Byte count of the last surface upload; DC_LoadTexture stores it into the slot. */
int			g_nLastUploadBytes;

/* Shared palette-pool slot indices used by the paletted texture classes (init -1). */
short		g_iPalIdxDefault = -1;
short		g_iPalIdxClass6  = -1;
short		g_iPalIdxClass9  = -1;

/* Create a video-memory 3D texture surface with pixel format *fmt into the
   caller's local ddsd (reused for the Lock that follows), evicting cached
   textures on out-of-memory. A macro so it expands over the caller's ddsd. */
#define DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, caps)                      \
	do {                                                                      \
		int _reclaimed;                                                       \
		LPDIRECTDRAWSURFACE4 _out;                                            \
		memset(&(ddsd), 0, sizeof(ddsd));                                     \
		(ddsd).dwSize = sizeof(ddsd);                                         \
		(ddsd).dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT; \
		(ddsd).ddsCaps.dwCaps = (caps);                                       \
		(ddsd).ddpfPixelFormat = *(fmt);                                      \
		(ddsd).dwWidth = (w);                                                 \
		(ddsd).dwHeight = (h);                                                \
		DCV_ClearCachedFlag();                                                \
		do {                                                                  \
			if (g_pDD4->lpVtbl->CreateSurface(g_pDD4, &(ddsd), &_out, NULL) == DD_OK) \
			{                                                                 \
				(pSurf) = _out;                                               \
				goto _dcv_got_surface;                                        \
			}                                                                 \
			_reclaimed = 0;                                                   \
			if (!s_bTexReclaimGuard) {                                        \
				s_bTexReclaimGuard = 1;                                       \
				_reclaimed = DC_ReclaimTextureSlot();                        \
				s_bTexReclaimGuard = 0;                                       \
			}                                                                 \
		} while (_reclaimed);                                                 \
		(pSurf) = NULL;                                                       \
	_dcv_got_surface:;                                                        \
	} while (0)  /* NB: one expansion per function (fixed goto label) */

/* DCV_PrepSurfaceTrueColor: convert RGBA bytes to 565 or 4444 and lock/copy/unlock. */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfaceTrueColor(int w, int h, void *data, DDPIXELFORMAT *fmt)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	int n;
	byte *rgbax = (byte *)data;
	int use_565 = (fmt == &g_pfRGB565);

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY);
	if (!pSurf)
		return NULL;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	n = h * w;
	dst = (unsigned short *)ddsd.lpSurface;
	g_nLastUploadBytes = n * 2;

	if (n)
	{
		do
		{
			byte r = rgbax[0], g = rgbax[1], b = rgbax[2], a = rgbax[3];
			rgbax += 4;
			n--;
			if (use_565)
				*dst++ = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
			else if (fmt == &g_pfARGB4444)
				*dst++ = (unsigned short)(((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4) | ((a >> 4) << 12));
			else
				Sys_Error("DCV_PrepSurfaceTrueColor isn't prepared for this format\n");
		} while (n);
	}

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	return pSurf;
}

/* DCV_UpdateTextureSubRect: copy 16-bit subrect into existing texture (lightmap updates). */
int DCV_UpdateTextureSubRect( int texnum, int x, int y, int w, int h, const unsigned short* src, int src_pitch )
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 surf;
	dctexture_t *slot;
	int row, col;
	unsigned short *dst;
	const unsigned short *s;

	slot = &s_texSlots[texnum];

	if (slot->nServerCount != -1)
	{
		surf = (LPDIRECTDRAWSURFACE4)slot->pddsSurface;
		if (!surf)
		{
			DC_DecacheTextureSlot(slot);
			surf = (LPDIRECTDRAWSURFACE4)slot->pddsSurface;
		}

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (!DCV_DDError(surf->lpVtbl->Lock(surf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
		{
			for (row = 0; row < h; row++)
			{
				s = src + (src_pitch * y + x);
				dst = (unsigned short *)ddsd.lpSurface + ((int)(ddsd.lPitch / 2) * y + x);
				for (col = 0; col < w; col++)
					*dst++ = *s++;
				y++;
			}
			DCV_DDError(surf->lpVtbl->Unlock(surf, NULL), TEXT("Unlock texture"));
		}
	}
	return 0;
}

/* DCV_PrepSurface16: copy 16-bit words into a new texture surface. */
LPDIRECTDRAWSURFACE4 DCV_PrepSurface16(int w, int h, void *data, DDPIXELFORMAT *fmt)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	unsigned short *src = (unsigned short *)data;
	int n;

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY);
	if (!pSurf)
		return NULL;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	n = h * w;
	dst = (unsigned short *)ddsd.lpSurface;
	g_nLastUploadBytes = n * 2;

	for (; n != 0; n--)
	{
		unsigned short v = *src;
		src++;
		*dst = v;
		dst++;
	}

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	return pSurf;
}

/* DCV_PrepSurfaceIndexed: 8-bit indices -> 16-bit via palette table.
   color_fmt selects the PVR pixel format: PVR_RGB565, PVR_ARGB1555, or PVR_ARGB4444. */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfaceIndexed(int w, int h, void *data, unsigned short *palette, DDPIXELFORMAT *fmt)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *dst;
	byte *idx = (byte *)data;
	int n;

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY);
	if (!pSurf)
		return NULL;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	n = h * w;
	dst = (unsigned short *)ddsd.lpSurface;
	g_nLastUploadBytes = n * 2;

	for (; n != 0; n--)
	{
		byte b = *idx;
		idx++;
		*dst = palette[b];
		dst++;
	}

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
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
		size /= 2;
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
		*twiddle_dst16++ = twiddle_pal16[src[0]];
		*twiddle_dst16++ = twiddle_pal16[src[0]];
		*twiddle_dst16++ = twiddle_pal16[src[0]];
		*twiddle_dst16++ = twiddle_pal16[src[0]];
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
		size /= 2;
		DCV_TwiddleBlit8Recurse(x, y, size);
		DCV_TwiddleBlit8Recurse(x, y + size, size);
		x += size;
		DCV_TwiddleBlit8Recurse(x, y, size);
		y += size;
	}

	/* Innermost 2x2 block, stored as two twiddled column pairs. */
	*(unsigned short *)twiddle_dst8 =
		(unsigned short)(twiddle_remap8[twiddle_src[y * twiddle_size + x]]
		| (twiddle_remap8[twiddle_src[(y + 1) * twiddle_size + x]] << 8));
	twiddle_dst8 += 2;
	*(unsigned short *)twiddle_dst8 =
		(unsigned short)(twiddle_remap8[twiddle_src[y * twiddle_size + x + 1]]
		| (twiddle_remap8[twiddle_src[(y + 1) * twiddle_size + x + 1]] << 8));
	twiddle_dst8 += 2;
}

static __inline void DCV_TwiddleBlit8( int size, byte *dst, byte *src, byte *remap )
{
	twiddle_remap8 = remap;
	twiddle_size = size;
	twiddle_dst8 = dst;
	twiddle_src = src;
	twiddle_active = 1;

	DCV_TwiddleBlit8Recurse(0, 0, size);
}

/* DCV_PrepSurfacePaletted: square 8-bit source uploaded twiddled into a P8
   palettized surface. The palette itself is bound by the caller (DC_LoadTexture). */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfacePaletted(int w, int h, void *data, byte *pPal)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	(void)h;

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, &g_pfPalette8, SURFACE_CAPS_PALETTE8);
	if (!pSurf)
		return NULL;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	g_nLastUploadBytes = w * w;
	DCV_TwiddleBlit8(w, (byte *)ddsd.lpSurface, (byte *)data, pPal);

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	return pSurf;
}

/* DCV_ResampleBlit: box-filter an 8-bit indexed source into a locked 16-bit
   destination of different dimensions. Each destination texel averages every
   source texel it covers, per channel, using the destination pixel format's
   channel masks. */
int DCV_ResampleBlit( DDPIXELFORMAT *pf, LPDIRECTDRAWSURFACE4 surf, RECT *prc, byte *src, int src_w, int src_h, unsigned short *pal16 )
{
	DDSURFACEDESC2 ddsd;
	unsigned short *dst;
	byte *srcp;
	float xstep, ystep;
	float fx, fx1, fy, fy1;
	int x, y;
	int sx, sy;
	int count;
	int accum_r, accum_g, accum_b, accum_a;
	unsigned int texel;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (DCV_DDError(surf->lpVtbl->Lock(surf, NULL, &ddsd, DDLOCK_WAIT, NULL), TEXT("Lock texture")))
		return 0;

	g_nLastUploadBytes = prc->right * prc->bottom * 2;

	xstep = (float)src_w / (float)prc->right;
	ystep = (float)src_h / (float)prc->bottom;

	for (y = 0; y < prc->bottom; y++)
	{
		fy = ystep * (float)y;
		fy1 = ystep * (float)(y + 1);
		dst = (unsigned short *)((byte *)ddsd.lpSurface + prc->right * y * 2);

		for (x = 0; x < prc->right; x++)
		{
			fx = xstep * (float)x;
			fx1 = xstep * (float)(x + 1);

			count = 0;
			accum_r = accum_g = accum_b = accum_a = 0;

			/* Average every source texel this destination texel covers. Always
			   sample at least one texel per axis, so a shrunken axis can never
			   leave a destination texel empty. */
			sy = (int)floor(fy);
			do
			{
				sx = (int)floor(fx);
				srcp = src + sx + sy * src_w;
				do
				{
					count++;
					texel = pal16[*srcp++];
					accum_r += (int)(texel & pf->dwRBitMask);
					accum_g += (int)(texel & pf->dwGBitMask);
					accum_b += (int)(texel & pf->dwBBitMask);
					accum_a += (int)(texel & pf->dwRGBAlphaBitMask);
					sx++;
				} while (sx < (int)floor(fx1));
				sy++;
			} while (sy < (int)floor(fy1));

			if (!count)
				Sys_Error("Destination texel at %d, %d had no source texels!\n", x, y);
			else
				*dst = (unsigned short)(
					((accum_r / count) & pf->dwRBitMask) |
					((accum_g / count) & pf->dwGBitMask) |
					((accum_b / count) & pf->dwBBitMask) |
					((accum_a / count) & pf->dwRGBAlphaBitMask));
			dst++;
		}
	}

	surf->lpVtbl->Unlock(surf, NULL);
	return 1;
}

/* DCV_PrepSurfaceTwiddled: square source uploaded in twiddled texel order. */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfaceTwiddled(int w, int h, void *data, unsigned short *pal16, DDPIXELFORMAT *fmt, int mipmap)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	byte *idx = (byte *)data;
	(void)mipmap;

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_OPTIMIZED);
	if (!pSurf)
		return NULL;

	g_nLastUploadBytes = w * w;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, DDLOCK_WAIT | DDLOCK_OPTIMIZED, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	DCV_TwiddleBlit16(w, (unsigned short *)ddsd.lpSurface, idx, pal16);

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	return pSurf;
}

/* DCV_PrepSurfaceResampled: create a texture surface at the rounded dimensions
   and box-filter the source into it. */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfaceResampled(int w, int h, int src_w, int src_h, void *data, unsigned short *pal16, DDPIXELFORMAT *fmt)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	RECT rc;
	byte *src = (byte *)data;

	DCV_CREATE_SURFACE(pSurf, ddsd, w, h, fmt, DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY);
	if (!pSurf)
		return NULL;

	rc.left = 0;
	rc.right = w;
	rc.top = 0;
	rc.bottom = h;
	DCV_ResampleBlit(fmt, pSurf, &rc, src, src_w, src_h, pal16);

	return pSurf;
}

/* DCV_PrepSurfacePVR: supports RECT/TWIDDLED/VQ(+mipmap top-level extraction). */
LPDIRECTDRAWSURFACE4 DCV_PrepSurfacePVR(int w, int h, int src_w, int src_h, void *data, DDPIXELFORMAT *fmt, char *name)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE4 pSurf;
	unsigned short *src16, *dst;
	int texDataSize, payload, n, i;
	unsigned int fmtword;
	int lockFlags;
	int reclaimed;

	(void)src_w;
	(void)src_h;

	texDataSize = ((pvrheader_t *)data)->dataSize;
	payload = texDataSize - 8;
	g_nLastUploadBytes = payload;
	src16 = (unsigned short *)((pvrheader_t *)data + 1);
	fmtword = *(unsigned int *)&((pvrheader_t *)data)->pixelFormat;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
	ddsd.ddpfPixelFormat = *fmt;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;

	if ((fmtword & 0xf) == PVR_ARGB1555)
	{
		ddsd.ddpfPixelFormat = g_pfARGB1555;
	}
	else if ((fmtword & 0xf) == PVR_RGB565)
	{
		ddsd.ddpfPixelFormat = g_pfRGB565;
	}
	else
	{
		if ((fmtword & 0xf) != PVR_ARGB4444)
		{
			Sys_Error("%s: No support for PVR YUV or bump textures\n", name);
			return NULL;
		}
		ddsd.ddpfPixelFormat = g_pfARGB4444;
	}

	lockFlags = DDLOCK_WAIT;
	fmtword &= 0x1f00;

	if (fmtword == (PVR_TWIDDLE << 8))
	{
		ddsd.ddsCaps.dwCaps |= DDSCAPS_OPTIMIZED;
		lockFlags = DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_OPTIMIZED;
	}
	else if (fmtword == (PVR_TWIDDLED_MIPMAP << 8))
	{
		ddsd.ddsCaps.dwCaps |= (DDSCAPS_OPTIMIZED | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX);
		lockFlags = DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_OPTIMIZED;
	}
	else if (fmtword == (PVR_VQ << 8))
	{
		ddsd.ddpfPixelFormat = g_pfScreenRGB565;
		lockFlags = DDLOCK_WAIT | DDLOCK_COMPRESSED;
	}
	else if (fmtword == (PVR_VQ_MIPMAP << 8))
	{
		ddsd.ddpfPixelFormat = g_pfScreenRGB565;
		ddsd.ddsCaps.dwCaps |= (DDSCAPS_MIPMAP | DDSCAPS_COMPLEX);
		lockFlags = DDLOCK_WAIT | DDLOCK_COMPRESSED;
	}
	else if (fmtword == (PVR_CLUT8_TWIDDLED << 8) || fmtword == (PVR_CLUT4_TWIDDLED << 8) ||
	         fmtword == (PVR_DIRECT8_TWIDDLED << 8) || fmtword == (PVR_DIRECT4_TWIDDLED << 8))
	{
		Sys_Error("%s: Palettized PVR texture?\n", name);
		return NULL;
	}
	else if (fmtword != (PVR_RECT << 8) && fmtword != (PVR_RECTANGULAR_STRIDE << 8))
	{
		if (fmtword == (PVR_RECTANGULAR_TWIDDLED << 8))
		{
			ddsd.ddsCaps.dwCaps |= DDSCAPS_OPTIMIZED;
			lockFlags = DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_OPTIMIZED;
		}
		else if (fmtword != (0x0e << 8) &&
		         (fmtword == (0x0f << 8) || fmtword == (PVR_SMALL_VQ << 8)))
		{
			Sys_Error("%s: D3D don't do small VQ\n", name);
			return NULL;
		}
	}

	DCV_ClearCachedFlag();

	do {
		if (g_pDD4->lpVtbl->CreateSurface(g_pDD4, &ddsd, &pSurf, NULL) == DD_OK)
			break;
		reclaimed = 0;
		if (!s_bTexReclaimGuard)
		{
			s_bTexReclaimGuard = 1;
			reclaimed = DC_ReclaimTextureSlot();
			s_bTexReclaimGuard = 0;
		}
	} while (reclaimed);

	if (!pSurf)
		return NULL;

	if (DCV_DDError(pSurf->lpVtbl->Lock(pSurf, NULL, &ddsd, lockFlags, NULL), TEXT("Lock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	if (payload < 0)
		payload = texDataSize - 7;

	dst = (unsigned short *)ddsd.lpSurface;
	n = payload >> 1;
	for (i = 0; i < n; i++)
		dst[i] = src16[i];

	if (DCV_DDError(pSurf->lpVtbl->Unlock(pSurf, NULL), TEXT("Unlock texture")))
	{
		pSurf->lpVtbl->Release(pSurf);
		return NULL;
	}

	return pSurf;
}
static const unsigned int key_sizes[] = { 8, 16, 32, 64, 128, 256, 512, 0 };

/* Rolling-XOR hash over the texture's raw texels (capped at 8000 bytes), used as
   the cache key that DC_FindTextureSlot matches against. The byte count spans all
   mip levels and scales with the source's bytes-per-texel. */
unsigned int DCV_ComputeTextureHash(int w, int h, byte *data, int mip_count, int tex_type)
{
	unsigned int shift;
	unsigned int key;
	int sample_bytes;
	int levels;
	int i;

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
	case TEX_TYPE_PAL8_6:
	case TEX_TYPE_PAL8:
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h;
			w >>= 1;
			h >>= 1;
		}
		break;
	case TEX_TYPE_GBIX:
		sample_bytes = ((pvrheader_t *)data)->dataSize;
		data += sizeof(pvrheader_t);
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
	case TEX_TYPE_RGB565_RAW:
		sample_bytes = 0;
		for (i = 0; i < levels; ++i)
		{
			sample_bytes += w * h * 2;
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
	unsigned int i = 0;

	while (1)
	{
		unsigned r = *pPal++;
		unsigned g = *pPal++;
		unsigned b = *pPal++;

		if ((r & 0xF8) != (g & 0xF8))
			return 0;
		if ((r & 0xF8) != (b & 0xF8))
			break;
		if (++i >= 256)
			return 1;
	}

	return 0;
}

int DC_LoadTexture(char *identifier, int texture_type, int width, int height, void *data, short mipmap, int tex_type, unsigned char *pPal)
{
	int slot_index, out_w, out_h;
	unsigned int key;
	unsigned short pal_565[256];
	int monochrome, bPalIndexed, bDirect16, scaled;
	short sPalIndex;
	dctexture_t *slot;
	DDPIXELFORMAT *fmt;
	LPDIRECTDRAWSURFACE4 pSurf;
	LPDIRECT3DTEXTURE2 pTex;

	g_nLastUploadBytes = 0;
	monochrome = 0;
	bPalIndexed = 0;
	bDirect16 = 0;
	sPalIndex = -1;

	key = DCV_ComputeTextureHash(width, height, (byte *)data, mipmap, tex_type);

	if (tex_type == TEX_TYPE_LUM)
		Sys_Error("Lum textures not supported.");

	if (!identifier[0])
		return 0;

	slot_index = DC_FindTextureSlot(identifier, width, height, key);

	if (slot_index != -1)
	{
		if (texture_type == GLT_STUDIO)
			s_texSlots[slot_index].nUsageCount++;
		if (s_texSlots[slot_index].nServerCount > 0)
			s_texSlots[slot_index].nServerCount = (short)gHostSpawnCount;
		return slot_index;
	}

	/* Claim the first free slot (nServerCount == -1). */
	slot_index = 0;
	while (s_texSlots[slot_index].nServerCount != -1)
	{
		if (++slot_index >= MAX_D3D_TEXTURES)
		{
			Sys_Error("DCV_GetSlot: Too many textures.");
			slot_index = -1;
			break;
		}
	}

	if (slot_index == -1)
		return 0;

	scaled = DCV_ComputeScaledSize(tex_type, &out_w, &out_h, width, height, 0, 0, data);

	fmt = &g_pfRGB565;

	switch (tex_type)
	{
	default:				/* TEX_TYPE_NONE / TEX_TYPE_LUM */
		DCV_BuildPalette565(pPal, pal_565);
		monochrome = DCV_PaletteIsMonochromeQuantized(pPal);
		fmt = &g_pfRGB565;
		break;
	case TEX_TYPE_ALPHA:			/* 1 */
		DCV_BuildPalette1555(pPal, pal_565);
		fmt = &g_pfARGB1555;
		break;
	case TEX_TYPE_ALPHA_GRADIENT:		/* 3 */
		DCV_BuildAlphaGradient4444(pPal, pal_565);
		fmt = &g_pfARGB4444;
		break;
	case TEX_TYPE_RGBA:			/* 4 */
		bDirect16 = 1;
		fmt = &g_pfARGB4444;
		break;
	case TEX_TYPE_RGB565:			/* 5 */
	case TEX_TYPE_RGB565_RAW:		/* 10 */
		bDirect16 = 1;
		fmt = &g_pfRGB565;
		break;
	case TEX_TYPE_PAL8_6:
	case TEX_TYPE_PAL8:
	case TEX_TYPE_PAL8_9:
		bPalIndexed = 1;
		break;
	case TEX_TYPE_GBIX:			/* 8 */
		fmt = &g_pfScreenRGB565;
		break;
	case 11:
		DCV_BuildMaxChannelAlpha(pPal, pal_565);
		fmt = &g_pfARGB4444;
		break;
	}

	if (tex_type == TEX_TYPE_GBIX)
	{
		pSurf = DCV_PrepSurfacePVR(out_w, out_h, width, height, data, fmt, identifier);
	}
	else if (!bPalIndexed)
	{
		if (out_w == width && out_h == height && width == height && mipmap == 0 && pPal && monochrome)
		{
			/* square, unscaled, monochrome-quantized: share a pool palette. */
			DCV_BuildLumaRemap(pPal, (byte *)pal_565);
			pSurf = DCV_PrepSurfacePaletted(out_w, out_h, data, (byte *)pal_565);
			sPalIndex = g_iPalIdxDefault;
			if (pSurf)
				pSurf->lpVtbl->SetPalette(pSurf, gGLPalette[sPalIndex].lpPalette);
		}
		else if (!bDirect16)
		{
			if (scaled == 0)
			{
				if (out_w == out_h && mipmap)
					pSurf = DCV_PrepSurfaceTwiddled(out_w, out_h, data, pal_565, fmt, mipmap);
				else
					pSurf = DCV_PrepSurfaceIndexed(out_w, out_h, data, pal_565, fmt);
			}
			else
			{
				pSurf = DCV_PrepSurfaceResampled(out_w, out_h, width, height, data, pal_565, fmt);
			}
		}
		else
		{
			if (out_w != out_h || out_w != width || out_h != height || mipmap)
				return 0;
			if (tex_type == TEX_TYPE_RGB565_RAW)
				pSurf = DCV_PrepSurface16(out_w, out_h, data, fmt);
			else
				pSurf = DCV_PrepSurfaceTrueColor(out_w, out_h, data, fmt);
		}
	}
	else						/* bPalIndexed: classes 6/7/9 */
	{
		int i;

		if (tex_type == TEX_TYPE_PAL8_9)
			sPalIndex = g_iPalIdxClass9;
		else if (tex_type == TEX_TYPE_PAL8_6)
			sPalIndex = g_iPalIdxClass6;
		else
			sPalIndex = g_iPalIdxDefault;

		for (i = 0; i < 256; i++)
			((byte *)pal_565)[i] = (byte)i;

		pSurf = DCV_PrepSurfacePaletted(out_w, out_h, data, (byte *)pal_565);
		if (pSurf)
			pSurf->lpVtbl->SetPalette(pSurf, gGLPalette[sPalIndex].lpPalette);
	}

	if (!pSurf)
		return 0;

	pTex = NULL;
	if (DCV_DDError(pSurf->lpVtbl->QueryInterface(pSurf, IID_IDirect3DTexture2, (void **)&pTex),
			TEXT("Get texture from surface")))
		return 0;

	slot = &s_texSlots[slot_index];
	DC_SetupTextureSlot(slot, pSurf, pTex, identifier, sPalIndex,
			(short)out_w, (short)out_h, (short)width, (short)height);

	slot->nServerCount = (texture_type == GLT_WORLD || texture_type == GLT_SPRITE)
			? (short)gHostSpawnCount : 0;
	if (texture_type == GLT_STUDIO)
		slot->nUsageCount++;

	slot->iTextureKey = key;
	slot->cbData = g_nLastUploadBytes;
	slot->iTextureType = texture_type;

	if (numgltextures < slot_index)
		numgltextures = slot_index;

	return slot_index;
}

void GL_BindStage( int texnum, int stage )
{
	dctexture_t *slot;

	// A texnum of -1 is the "no texture" sentinel used to unbind a stage.
	if ((unsigned)texnum >= MAX_D3D_TEXTURES)
		return;

	if (texnum == s_nD3dCurrentTexnum)
		return;

	DCV_Flush();

	slot = &s_texSlots[texnum & 0xFFFF];
	if (slot->nServerCount == -1)
		return;

	DCV_LinkTextureSlotFront(slot);

	if (slot->nServerCount != 0)
		slot->nServerCount = (short)gHostSpawnCount;

	if (slot->pd3dtTexture == NULL)
		DC_DecacheTextureSlot(slot);

	DCV_DDError(g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, stage,
			(LPDIRECT3DTEXTURE2)slot->pd3dtTexture),
		TEXT("Set texture"));

	s_nD3dCurrentTexnum = texnum;
	s_stageTexnum[stage] = texnum;
}

/*
================
GL_UnloadTextures

Unload all loaded textures
We do this every time we load the map
================
*/
int GL_UnloadTextures( void )
{
	int i, freed = 0;
	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];
		if (slot->nServerCount != -1 &&
		    slot->nServerCount > 0 &&
		    slot->nServerCount < gHostSpawnCount &&
		    slot->nUsageCount == 0)
		{
			DC_FreeTextureSlot(slot);
			freed++;
		}
	}
	return freed;
}

int DC_FreeTextureByName( char *name )
{
	int i;
	int servercount = 0;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		servercount = slot->nServerCount;
		if (servercount != -1)
		{
			char *slotname = slot->pszName ? slot->pszName : "<nameless>";
			if (slotname)
			{
				slotname = slot->pszName ? slot->pszName : "<nameless>";
				if (strcmp(slotname, name) == 0 && slot->nServerCount >= 0 && slot->nUsageCount == 0)
				{
					DC_FreeTextureSlot(slot);
					return 1;
				}
			}
		}
	}

	return servercount;
}

void DC_TouchTexture( int texnum )
{
	dctexture_t *slot;

	if (texnum < 0 || texnum >= MAX_D3D_TEXTURES)
		return;

	slot = &s_texSlots[texnum];

	if (slot->nServerCount == -1 || slot->nServerCount == 0)
		return;

	DCV_LinkTextureSlotFront(slot);

	if (slot->nServerCount != 0)
		slot->nServerCount = (short)gHostSpawnCount;
}

extern "C" int DC_ForceFreeTextureByName( char *name )
{
	int i;
	int servercount = 0;

	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];

		servercount = slot->nServerCount;
		if (servercount != -1)
		{
			char *slotname = slot->pszName ? slot->pszName : "<nameless>";
			if (slotname)
			{
				slotname = slot->pszName ? slot->pszName : "<nameless>";
				if (strcmp(slotname, name) == 0)
				{
					DC_FreeTextureSlot(slot);
					return 1;
				}
			}
		}
	}

	return servercount;
}

int DC_FreeTextureByIndex( int texnum )
{
	if (s_texSlots[texnum].nServerCount == -1)
		return 0;

	DC_FreeTextureSlot(&s_texSlots[texnum]);
	return 1;
}

int DC_ReleaseTexture( int texnum )
{
	dctexture_t *slot = &s_texSlots[texnum];

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
	for (i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		dctexture_t *slot = &s_texSlots[i];
		if (slot->nServerCount != -1 &&
		    slot->iTextureType == 5 &&
		    slot->nServerCount > 0 &&
		    slot->nServerCount < gHostSpawnCount)
		{
			DC_FreeTextureSlot(slot);
			freed++;
		}
	}
	return freed;
}

void DC_InitTextureList( void )
{
	s_dcTextureLruHead.pPrev = NULL;
	s_dcTextureLruHead.pNext = &s_dcTextureLruTail;
	s_dcTextureLruTail.pPrev = &s_dcTextureLruHead;
	s_dcTextureLruTail.pNext = NULL;

	if (g_bTextureLog)
		Sys_FPrintf(g_bTextureLog, "Texture log start.\n");
}

dcpalette_t::dcpalette_t()
{
	tag = -1;
	referenceCount = 0;
	lpPalette = NULL;
}

dcpalette_t::~dcpalette_t()
{
	if (lpPalette)
		lpPalette->lpVtbl->Release(lpPalette);
}

void GL_Bind( int texnum, int stage )
{
	GL_BindStage(texnum, stage);
}

void GL_Texels_f( void )
{
	Con_Printf("Current uploaded texels: %i\n", texels);
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
	DCV_TexState_Additive();
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
	unsigned int	filler[256];
	float			prev;
	int				i;

	Draw_Shutdown();
	m_bDrawInitialized = TRUE;

	if (s_bFirstTime)
	{
		Cmd_AddCommand("gl_texturemode", Draw_TextureMode_f);
		Cmd_AddCommand("gl_texels", GL_Texels_f);

		Cvar_RegisterVariable(&gl_nobind);
		Cvar_RegisterVariable(&gl_max_size);
		Cvar_RegisterVariable(&gl_round_down);
		Cvar_RegisterVariable(&gl_picmip);
		Cvar_RegisterVariable(&gl_palette_tex);
	}

	menu_wad = (cachewad_t *)MnemoAllocDbg(sizeof(cachewad_t), __FILE__, __LINE__);
	memset(menu_wad, 0, sizeof(cachewad_t));
	Draw_CacheWadInit("cached.wad", 16, menu_wad);
	menu_wad->tempWad = TRUE;

	Draw_CacheWadHandler(&custom_wad, Draw_MiptexTexture, MIP_EXTRASIZE);

	memset(decal_names, 0, sizeof(decal_names));

	DCV_GammaRefresh_f();

	if (s_bFirstTime)
	{
		// a fallback texture for models with no skin
		for (i = 0; i < 256; i++)
			filler[i] = 0x800000FF;
		nada_texture = DC_LoadTexture("nada", GLT_SYSTEM, 16, 16, filler, FALSE, TEX_TYPE_RGBA, NULL);

		draw_disc = LoadTransPic("lambda", (qpic_t*)W_GetLumpinfo("lambda"));
	}

	draw_chars = (qfont_t*)W_GetLumpinfo("creditsfont");

	cb = (qpic_t*)Draw_CacheGet(menu_wad, Draw_CacheIndex(menu_wad, "gfx/conback.lmp"));
	if (!cb)
		Sys_Error("Couldn't load conback.lmp");

	SwapPic(cb);

	conback->width = cb->width;
	conback->height = cb->height;

	gl = (glpic_t*)conback->data;
	pPal = &cb->data[cb->width * cb->height + 2];
	if (!s_bFirstTime)
		DC_FreeTextureByName("conback");
	gl->texnum = DC_LoadTexture("conback", GLT_SYSTEM, cb->width, cb->height, cb->data, FALSE, TEX_TYPE_NONE, pPal);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	prev = gl_round_down.value;
	gl_round_down.value = 0.0;
	translate_texture = 0;

	// now turn the charset into a texture
	pPal = &draw_chars->data[256 * draw_chars->height + 2];
	char_texture = DC_LoadTexture("creditsfont", GLT_SYSTEM, 256, draw_chars->height, draw_chars->data, FALSE, TEX_TYPE_FONT, pPal);

	gl_round_down.value = prev;

	// Publish the glyph-sheet UV scale into the font header. Font_DrawChar reads
	// usize and vsize to map glyph cells to texture coords; these overlap the
	// first pixels of the sheet, which is safe now that the sheet has been
	// uploaded to a texture.
	*(float *)(draw_chars->data + 4) = 1.0f / 256.0f;
	*(float *)(draw_chars->data + 8) = 1.0f / (float)draw_chars->height;

	s_bFirstTime = FALSE;
}

/*
================
Draw_Character

Draws a single character
================
*/
int Draw_Character( int x, int y, int num )
{
	DCV_TexState_Blend();
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
	int c;

	DCV_SetHudDepth(3.0f);
	DCV_SetPackedColor(0xFFFF9000);

	c = *str;
	if (c)
	{
		do {
			DCV_TexState_Blend();
			Font_SetScale(1.0f, 1.0f);
			x += Font_DrawCharI(draw_chars, x, y, c);
			str++;
			c = *str;
		} while (c);
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

	if (!pic)
		return;

	gl = (glpic_t*)pic->data;

	DCV_2D_SetupStates();
	DCV_SetPackedColor(0xFFFFFFFF);
	GL_BindStage(gl->texnum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex((float)x,                   (float)y,                   dc_depthhud.value - 0.1f, gl->sl, gl->tl);
	DCV_AddVertex((float)(x + pic->width),    (float)y,                   dc_depthhud.value - 0.1f, gl->sh, gl->tl);
	DCV_AddVertex((float)x,                   (float)(y + pic->height),   dc_depthhud.value - 0.1f, gl->sl, gl->th);
	DCV_AddVertex((float)(x + pic->width),    (float)(y + pic->height),   dc_depthhud.value - 0.1f, gl->sh, gl->th);
}

/*
=============
Draw_AlphaSubPic
=============
*/
void Draw_AlphaSubPic( int xDest, int yDest, int xSrc, int ySrc, int iWidth, int iHeight, qpic_t* pic, colorVec* pc, int iAlpha )
{
	glpic_t* gl;
	float uleft, uright, vtop, vbottom, du;
	float alpha;

	if (!pic)
		return;

	DCV_Begin2D(1, 1);
	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglEnable(GL_BLEND);
	qglEnable(GL_ALPHA_TEST);
	DCV_2D_SetupStates();

	gl = (glpic_t*)pic->data;

	du    = (float)iWidth / (float)pic->width;
	uleft = (float)xSrc / (float)pic->width;
	vtop  = (float)ySrc / (float)pic->height;
	alpha = (float)iAlpha / 255.0f;

	qglColor4f(alpha * (float)pc->r / 256.0f,
		alpha * (float)pc->g / 256.0f,
		alpha * (float)pc->b / 256.0f, 1.0f);
	GL_BindStage(gl->texnum, 0);
	qglBegin(GL_QUADS);

	qglTexCoord2f(uleft, vtop);
	qglVertex2f((float)xDest, (float)yDest);
	uright = uleft + du;
	qglTexCoord2f(uright, vtop);
	qglVertex2f((float)(xDest + iWidth), (float)yDest);
	vbottom = vtop + (float)iHeight / (float)pic->height;
	qglTexCoord2f(uright, vbottom);
	qglVertex2f((float)(xDest + iWidth), (float)(yDest + iHeight));
	qglTexCoord2f(uleft, vbottom);
	qglVertex2f((float)xDest, (float)(yDest + iHeight));

	qglEnd();
	qglDisable(GL_BLEND);
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

	if (!pic)
		return;

	gl = (glpic_t*)pic->data;

	DCV_2D_SetupStates();
	GL_BindStage(gl->texnum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex((float)x,        (float)y,        dc_depthhud.value - 0.1f, gl->sl, gl->tl);
	DCV_AddVertex((float)(x + w),  (float)y,        dc_depthhud.value - 0.1f, gl->sh, gl->tl);
	DCV_AddVertex((float)x,        (float)(y + h),  dc_depthhud.value - 0.1f, gl->sl, gl->th);
	DCV_AddVertex((float)(x + w),  (float)(y + h),  dc_depthhud.value - 0.1f, gl->sh, gl->th);
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
	DCV_TexState_Opaque();
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
	x = clamp(x, 0, (int)vid.width);
	y = clamp(y, 0, (int)vid.height);
	width = clamp(width, 0, (int)vid.width - x);
	height = clamp(height, 0, (int)vid.height - y);

	scissor_x = x;
	scissor_width = width;
	scissor_y = (int)vid.height - y - height;
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
static __inline int ValidateWRect( const wrect_t* prc )
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
	float invw, invh;

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

	/* Sample pixel centers: nudge the edges inward by half a texel. */
	invw = 1.0f / (float)pFrame->width;
	*pfLeft   = ((float)rc.left   + 0.5f) * invw;
	*pfRight  = ((float)rc.right  - 0.5f) * invw;
	invh = 1.0f / (float)pFrame->height;
	*pfTop    = ((float)rc.top    + 0.5f) * invh;
	*pfBottom = ((float)rc.bottom - 0.5f) * invh;
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

	GL_BindStage(pFrame->gl_texturenum, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex((float)x,            (float)y,             dc_depthhud.value - 0.1f, fLeft,  fTop);
	DCV_AddVertex((float)(x + iWidth), (float)y,             dc_depthhud.value - 0.1f, fRight, fTop);
	DCV_AddVertex((float)x,            (float)(y + iHeight), dc_depthhud.value - 0.1f, fLeft,  fBottom);
	DCV_AddVertex((float)(x + iWidth), (float)(y + iHeight), dc_depthhud.value - 0.1f, fRight, fBottom);
}

void Draw_SpriteFrame( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	Draw_Frame(pFrame, x, y, prcSubRect);
}

void Draw_SpriteFrameHoles( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	DCV_2D_SetupStates();
	if (gl_spriteblend.value)
		DCV_TexState_Blend();
	Draw_Frame(pFrame, x, y, prcSubRect);
	DCV_TexState_Opaque();
}

void Draw_SpriteFrameAdditive( mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect )
{
	DCV_TexState_Additive();
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
	DCV_AddVertex((float)x,         (float)(y + h),   1.0f, 0.0f, 1.0f);
	DCV_AddVertex((float)(x + w),   (float)(y + h),   1.0f, 1.0f, 1.0f);
	DCV_SetPackedColor(0xFFFFFFFF);
	DCV_FlushApplyRenderState((D3DRENDERSTATETYPE)0x1b, 0);
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
	qglEnable(GL_BLEND);
	qglDisable(GL_TEXTURE_2D);
	qglColor4f(0.0f, 0.0f, 0.0f, 0.8f);
	qglBegin(GL_QUADS);
	qglVertex2f(0.0f, 0.0f);
	qglVertex2f((float)glwidth, 0.0f);
	qglVertex2f((float)glwidth, (float)glheight);
	qglVertex2f(0.0f, (float)glheight);
	qglEnd();
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
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

	tag = pPal[0];

	for (i = 1; i < 768; i++)
		tag = (tag + pPal[i]) ^ pPal[i - 1];

	if (tag < 0)
		tag = -tag;

	return tag;
}

int DC_GetSlotPaletteIndex(int slot_index)
{
	if (slot_index < 0 || slot_index >= MAX_D3D_TEXTURES) return -1;
	return (int)s_texSlots[slot_index].iPalette;
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture( char* identifier, GL_TEXTURETYPE textureType, int width, int height, unsigned char* data, int mipmap, int iType, unsigned char* pPal )
{
	return DC_LoadTexture(identifier, (int)textureType, width, height, data, (short)mipmap, iType, pPal);
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture( qpic_t* pic, char* pszName )
{
	unsigned char* pPal = &pic->data[pic->width * pic->height + 2];

	return GL_LoadTexture(pszName, GLT_SYSTEM, pic->width, pic->height, pic->data, FALSE, TEX_TYPE_ALPHA, pPal);
}

qpic_t* LoadTransPic( char* pszName, qpic_t* ppic )
{
	static int	trans_pic_loaded = 1;
	glpic_t* gl;
	qpic_t* ppicNew;
	byte* pPal;

	if (!trans_pic_loaded)
		Sys_Error("LoadTransPic called multiple times.\n");
	trans_pic_loaded = 0;

	if (!ppic)
		return NULL;

	ppicNew = (qpic_t*)MnemoAllocDbg(sizeof(qpic_t) + sizeof(glpic_t), __FILE__, __LINE__);
	gl = (glpic_t*)ppicNew->data;

	ppicNew->width = ppic->width;
	ppicNew->height = ppic->height;

	pPal = &ppic->data[ppic->width * ppic->height + 2];
	gl->texnum = DC_LoadTexture(pszName, GLT_SYSTEM, ppic->width, ppic->height, ppic->data, FALSE, TEX_TYPE_ALPHA, pPal);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return ppicNew;
}

qpic_t* Draw_CachePic( char* path )
{
	qpic_t* ret;
	int idx;

	idx = Draw_CacheIndex(menu_wad, path);
	ret = (qpic_t*)Draw_CacheGet(menu_wad, idx);
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

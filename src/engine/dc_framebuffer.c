/*
 * dc_framebuffer.c -- fatal text drawing on framebuffer (Sys_Error path).
 * Renders 5x5 procedural font directly to primary/backbuffer surfaces.
 */

#include "quakedef.h"
#include "winquake.h"

extern void* Sys_GetPrimarySurface4( void );
extern void* Sys_GetBackBuffer4( void );

#define GLYPH_W          5
#define GLYPH_H          5
#define GLYPH_XSCALE     2
#define GLYPH_YSCALE     3
#define GLYPH_ADVANCE    13
#define LINE_ADVANCE     32
#define TAB_WIDTH        80
#define START_Y_OFFSET   42
#define SHADOW_OFFSET    2
#define BANNER_X         256
#define BANNER_W         128
#define BANNER_H         0x20
#define TEXT_COLOR       0x7BEF
#define SHADOW_COLOR     0x0000

/*
 * Font blob at 0x001be764 used for rendering glyphs to FB for Sys_Error:
 * 96 glyphs (0x20-0x7f), 5 rows each, 6 bytes per row including NUL.
 * We keep the same logical row layout here and stamp 2x3 pixel blocks
 * directly into the primary surface.
 */
static const char s_szGlyphRows[96][GLYPH_H][GLYPH_W + 1] =
{
	{ "-----", "-----", "-----", "-----", "-----" },
	{ "--x--", "--x--", "--x--", "-----", "--x--" },
	{ "-x-x-", "-x-x-", "-----", "-----", "-----" },
	{ "-x-x-", "xxxxx", "-x-x-", "xxxxx", "-x-x-" },
	{ "-xxx-", "x-x--", "-xxx-", "--x-x", "-xxx-" },
	{ "x---x", "--x--", "-x---", "x---x", "-----" },
	{ "-xx--", "x--x-", "-xx-x", "x--x-", "-xxx-" },
	{ "--x--", "--x--", "-----", "-----", "-----" },
	{ "--x--", "-x---", "-x---", "-x---", "--x--" },
	{ "-x---", "--x--", "--x--", "--x--", "-x---" },
	{ "-x-x-", "--x--", "xxxxx", "--x--", "-x-x-" },
	{ "-----", "--x--", "xxxxx", "--x--", "-----" },
	{ "-----", "-----", "-----", "--x--", "-x---" },
	{ "-----", "-----", "xxxxx", "-----", "-----" },
	{ "-----", "-----", "-----", "--x--", "-----" },
	{ "----x", "---x-", "--x--", "-x---", "x----" },
	{ "-xxx-", "x---x", "x---x", "x---x", "-xxx-" },
	{ "--x--", "-xx--", "--x--", "--x--", "xxxxx" },
	{ "-xxx-", "----x", "-xxx-", "x----", "xxxxx" },
	{ "-xxx-", "----x", "--xx-", "----x", "-xxx-" },
	{ "x---x", "x---x", "xxxxx", "----x", "----x" },
	{ "xxxxx", "x----", "xxxx-", "----x", "xxxx-" },
	{ "-xxx-", "x----", "xxxx-", "x---x", "-xxx-" },
	{ "xxxxx", "----x", "---x-", "--x--", "-x---" },
	{ "-xxx-", "x---x", "-xxx-", "x---x", "-xxx-" },
	{ "-xxx-", "x---x", "-xxxx", "----x", "-xxx-" },
	{ "-----", "--x--", "-----", "--x--", "-----" },
	{ "-----", "--x--", "-----", "--x--", "-x---" },
	{ "---x-", "--x--", "-x---", "--x--", "---x-" },
	{ "-----", "xxxxx", "-----", "xxxxx", "-----" },
	{ "-x---", "--x--", "---x-", "--x--", "-x---" },
	{ "-xxx-", "----x", "--xx-", "-----", "--x--" },
	{ "-xxx-", "x-xxx", "x-x-x", "x----", "-xxx-" },
	{ "--x--", "-x-x-", "x---x", "xxxxx", "x---x" },
	{ "xxxx-", "x---x", "xxxx-", "x---x", "xxxx-" },
	{ "-xxx-", "x----", "x----", "x----", "-xxx-" },
	{ "xxxx-", "x---x", "x---x", "x---x", "xxxx-" },
	{ "xxxxx", "x----", "xxxx-", "x----", "xxxxx" },
	{ "xxxxx", "x----", "xxxx-", "x----", "x----" },
	{ "-xxx-", "x----", "x-xx-", "x---x", "-xxx-" },
	{ "x---x", "x---x", "xxxxx", "x---x", "x---x" },
	{ "xxxxx", "--x--", "--x--", "--x--", "xxxxx" },
	{ "----x", "----x", "----x", "x---x", "-xxx-" },
	{ "x---x", "x--x-", "xxx--", "x--x-", "x---x" },
	{ "x----", "x----", "x----", "x----", "xxxxx" },
	{ "x---x", "xx-xx", "x-x-x", "x---x", "x---x" },
	{ "x---x", "xx--x", "x-x-x", "x--xx", "x---x" },
	{ "-xxx-", "x---x", "x---x", "x---x", "-xxx-" },
	{ "xxxx-", "x---x", "xxxx-", "x----", "x----" },
	{ "-xxx-", "x---x", "x---x", "x-x-x", "-xx-x" },
	{ "xxxx-", "x---x", "xxxx-", "x--x-", "x---x" },
	{ "-xxx-", "x----", "-xxx-", "----x", "xxxx-" },
	{ "xxxxx", "--x--", "--x--", "--x--", "--x--" },
	{ "x---x", "x---x", "x---x", "x---x", "-xxx-" },
	{ "x---x", "x---x", "x---x", "-x-x-", "--x--" },
	{ "x---x", "x---x", "x-x-x", "xx-xx", "x---x" },
	{ "x---x", "-x-x-", "--x--", "-x-x-", "x---x" },
	{ "x---x", "-x-x-", "--x--", "--x--", "--x--" },
	{ "xxxxx", "---x-", "--x--", "-x---", "xxxxx" },
	{ "xxxx-", "x----", "x----", "x----", "xxxx-" },
	{ "x----", "-x---", "--x--", "---x-", "----x" },
	{ "-xxxx", "----x", "----x", "----x", "-xxxx" },
	{ "--x--", "-x-x-", "x---x", "-----", "-----" },
	{ "-----", "-----", "-----", "-----", "xxxxx" },
	{ "-x---", "--x--", "-----", "-----", "-----" },
	{ "-----", "-xxx-", "----x", "-xxxx", "x---x" },
	{ "x----", "xxxx-", "x---x", "x---x", "xxxx-" },
	{ "-----", "-xxx-", "x----", "x----", "-xxx-" },
	{ "----x", "-xxxx", "x---x", "x---x", "-xxxx" },
	{ "-----", "-xxx-", "xxxxx", "x----", "-xxx-" },
	{ "--xx-", "-x---", "xxxx-", "-x---", "-x---" },
	{ "-----", "-xxx-", "x---x", "-xxxx", "--xx-" },
	{ "x----", "xxxx-", "x---x", "x---x", "x---x" },
	{ "--x--", "-----", "-xx--", "--x--", "xxxxx" },
	{ "----x", "-----", "---x-", "x--x-", "-xx--" },
	{ "x----", "x--x-", "xx---", "x--x-", "x---x" },
	{ "-xx--", "--x--", "--x--", "--x--", "xxxxx" },
	{ "-----", "xx-xx", "x-x-x", "x---x", "x---x" },
	{ "-----", "xxxx-", "x---x", "x---x", "x---x" },
	{ "-----", "-xxx-", "x---x", "x---x", "-xxx-" },
	{ "-----", "xxxx-", "x---x", "xxxx-", "x----" },
	{ "-----", "-xxxx", "x---x", "-xxxx", "----x" },
	{ "-----", "x-xx-", "xx---", "x----", "x----" },
	{ "-----", "-xxx-", "xx---", "---xx", "xxxx-" },
	{ "-x---", "xxxx-", "-x---", "-x---", "--xx-" },
	{ "-----", "x---x", "x---x", "x---x", "-xxx-" },
	{ "-----", "x---x", "x---x", "-x-x-", "--x--" },
	{ "-----", "x---x", "x-x-x", "xx-xx", "-x-x-" },
	{ "-----", "x---x", "-x-x-", "-x-x-", "x---x" },
	{ "-----", "x---x", "-x-x-", "--x--", "-x---" },
	{ "-----", "xxxxx", "--x--", "-x---", "xxxxx" },
	{ "--xx-", "-x---", "x----", "-x---", "--xx-" },
	{ "--x--", "--x--", "--x--", "--x--", "--x--" },
	{ "-xx--", "---x-", "----x", "---x-", "-xx--" },
	{ "-x---", "x-x--", "-----", "-----", "-----" },
	{ "xxxxx", "xxxxx", "xxxxx", "xxxxx", "xxxxx" }
};

static void DCV_FB_TextPixel( DDSURFACEDESC2 *pDDSD, int x, int y, unsigned short wColor )
{
	unsigned short *pwSurface;
	int nPitchWords;
	int nWidth;
	int nHeight;

	if (!pDDSD || !pDDSD->lpSurface)
		return;

	nPitchWords = pDDSD->lPitch / (int)sizeof(unsigned short);
	nWidth = pDDSD->dwWidth ? (int)pDDSD->dwWidth : 640;
	nHeight = pDDSD->dwHeight ? (int)pDDSD->dwHeight : 480;
	if (x < 0 || y < 0 || x >= nWidth || y >= nHeight)
		return;

	pwSurface = (unsigned short *)pDDSD->lpSurface;
	pwSurface[y * nPitchWords + x] = wColor;
}

static int DCV_FB_Glyph( DDSURFACEDESC2 *pDDSD, int x, int y, int iGlyph, unsigned short wColor )
{
	int iRow;
	int iCol;
	int iXScale;
	int iYScale;

	for (iRow = 0; iRow < GLYPH_H; ++iRow)
	{
		const char *pRow = (const char *)s_szGlyphRows + (iGlyph * 5 + iRow) * 6;
		int xCol = x;
		for (iCol = 0; iCol < GLYPH_W; ++iCol)
		{
			if (pRow[iCol] != '-')
			{
				int xPix = xCol;
				for (iXScale = 0; iXScale < GLYPH_XSCALE; ++iXScale)
				{
					int yPix = y;
					for (iYScale = 0; iYScale < GLYPH_YSCALE; ++iYScale)
					{
						((unsigned short *)pDDSD->lpSurface)[(pDDSD->lPitch / 2) * yPix + xPix] = wColor;
						++yPix;
					}
					++xPix;
				}
			}
			xCol += GLYPH_XSCALE;
		}
		y += GLYPH_YSCALE;
	}

	return x + GLYPH_ADVANCE;
}

static qboolean DCV_FB_LockSurface( LPDIRECTDRAWSURFACE4 pddsSurface, DDSURFACEDESC2 *pDDSD )
{
	HRESULT hr;

	if (!pddsSurface || !pddsSurface->lpVtbl || !pDDSD)
		return FALSE;

	memset(pDDSD, 0, sizeof(*pDDSD));
	pDDSD->dwSize = sizeof(*pDDSD);

	hr = pddsSurface->lpVtbl->Lock(pddsSurface, NULL, pDDSD, DDLOCK_WAIT, NULL);

	if (hr == DDERR_SURFACELOST)
	{
		pddsSurface->lpVtbl->Restore(pddsSurface);
		hr = pddsSurface->lpVtbl->Lock(pddsSurface, NULL, pDDSD, DDLOCK_WAIT, NULL);
	}

	return !FAILED(hr);
}

void DCV_FB_BackgroundRectOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int y, unsigned short wColor )
{
	DDSURFACEDESC2 ddsd;
	int iRow;
	int iCol;

	if (!DCV_FB_LockSurface(pddsSurface, &ddsd))
		return;

	for (iRow = 0; iRow < BANNER_H; ++iRow)
	{
		for (iCol = 0; iCol < BANNER_W; ++iCol)
			DCV_FB_TextPixel(&ddsd, BANNER_X + iCol, y + iRow, wColor);
	}

	pddsSurface->lpVtbl->Unlock(pddsSurface, NULL);
}

void DCV_FB_TextOnSurface( LPDIRECTDRAWSURFACE4 pddsSurface, int x, int y, const char* text )
{
	DDSURFACEDESC2 ddsd;
	int xHome;
	int xPos;
	int yPos;
	int nWidth;
	int iGlyph;
	unsigned char ch;

	if (!text || !DCV_FB_LockSurface(pddsSurface, &ddsd))
		return;

	xHome = x;
	xPos = x;
	yPos = y + START_Y_OFFSET;
	nWidth = ddsd.dwWidth ? (int)ddsd.dwWidth : 640;

	while ((ch = (unsigned char)*text++) != 0)
	{
		if (ch >= 'a' && ch <= 'z')
			ch = (unsigned char)(ch - ('a' - 'A'));

		iGlyph = (int)ch - 0x20;

		if (iGlyph < 0)
		{
			switch (ch)
			{
			case '\b':
				xPos -= GLYPH_ADVANCE;
				if (xPos < xHome)
					xPos = xHome;
				break;
			case '\t':
				xPos = ((xPos + TAB_WIDTH) / TAB_WIDTH) * TAB_WIDTH;
				break;
			case '\n':
				yPos += LINE_ADVANCE;
				xPos = xHome;
				break;
			case '\r':
				xPos = xHome;
				break;
			default:
				break;
			}
			continue;
		}

		if (iGlyph > 0x5F)
			iGlyph = 0x5F;

		DCV_FB_Glyph(&ddsd, xPos + SHADOW_OFFSET, yPos + SHADOW_OFFSET, iGlyph, SHADOW_COLOR);
		xPos = DCV_FB_Glyph(&ddsd, xPos, yPos, iGlyph, TEXT_COLOR);

		if (nWidth - xHome <= xPos)
		{
			yPos += LINE_ADVANCE;
			xPos = xHome;
		}
	}

	pddsSurface->lpVtbl->Unlock(pddsSurface, NULL);
}

/*
 * DCV_FB_BackgroundRect: fills a centered 128x32 RGB565 bar.
 * Draws to primary and backbuffer when distinct.
 */
void DCV_FB_BackgroundRect( unsigned short wColor )
{
	LPDIRECTDRAWSURFACE4 pddsPrimary;
	LPDIRECTDRAWSURFACE4 pddsBack;
	int y = 0x18;   /* binary reads a fixed y offset from a global (DAT_001be734) */

	pddsPrimary = (LPDIRECTDRAWSURFACE4)Sys_GetPrimarySurface4();
	pddsBack = (LPDIRECTDRAWSURFACE4)Sys_GetBackBuffer4();

	DCV_FB_BackgroundRectOnSurface(pddsPrimary, y, wColor);
	if (pddsBack && pddsBack != pddsPrimary)
		DCV_FB_BackgroundRectOnSurface(pddsBack, y, wColor);
}

/*
 * DCV_FB_Text: lock primary/backbuffer and stamp built-in 5x5 font.
 * Bypasses normal 2D batch so fatal errors still render when frame pipeline is bad.
 */
void DCV_FB_Text( const char* text )
{
	LPDIRECTDRAWSURFACE4 pddsPrimary;
	LPDIRECTDRAWSURFACE4 pddsBack;
	int x = 8;      /* binary reads fixed x/y offsets from globals (DAT_001be730/734) */
	int y = 0x18;

	pddsPrimary = (LPDIRECTDRAWSURFACE4)Sys_GetPrimarySurface4();
	pddsBack = (LPDIRECTDRAWSURFACE4)Sys_GetBackBuffer4();

	DCV_FB_TextOnSurface(pddsPrimary, x, y, text);
	if (pddsBack && pddsBack != pddsPrimary)
		DCV_FB_TextOnSurface(pddsBack, x, y, text);
}

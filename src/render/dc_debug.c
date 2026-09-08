/*
 * dc_debug.c -- fatal text drawing on framebuffer (Sys_Error path).
 * Renders the built-in 5x5 font directly to the primary surface.
 */

#include "quakedef.h"
#include "winquake.h"
#include "dc_debug.h"
#include "dc_draw.h"
#include "dc_accum.h"
#include "text_draw.h"
#include <platutil.h>

#define GLYPH_W          5
#define GLYPH_H          5
#define GLYPH_COUNT		95
#define MAX_GLYPH_INDEX	95
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
 * The font contains the printable ASCII glyphs, five rows each. We stamp 2x3 pixel blocks
 * directly into the primary surface.
 */
static const char s_szGlyphRows[GLYPH_COUNT][GLYPH_H][GLYPH_W + 1] =
{
	{ "-----", "-----", "-----", "-----", "-----" },
	{ "--x--", "--x--", "--x--", "-----", "--x--" },
	{ "-x-x-", "-x-x-", "-----", "-----", "-----" },
	{ "-x-x-", "xxxxx", "-x-x-", "xxxxx", "-x-x-" },
	{ "-xxx-", "x-x--", "-xxx-", "--x-x", "-xxx-" },
	{ "x---x", "---x-", "--x--", "-x---", "x---x" },
	{ "-xx--", "x-x--", "-xx--", "x-x-x", "-xxx-" },
	{ "---x-", "--x--", "-----", "-----", "-----" },
	{ "--xx-", "-x---", "-x---", "-x---", "--xx-" },
	{ "-xx--", "---x-", "---x-", "---x-", "-xx--" },
	{ "-----", "-xxx-", "-xxx-", "-xxx-", "-----" },
	{ "-----", "--x--", "-xxx-", "--x--", "-----" },
	{ "-----", "-----", "-----", "--x--", "-x---" },
	{ "-----", "-----", "-xxx-", "-----", "-----" },
	{ "-----", "-----", "-----", "-----", "--x--" },
	{ "----x", "---x-", "--x--", "-x---", "x----" },
	{ "-xxx-", "x---x", "x-x-x", "x---x", "-xxx-" },
	{ "--x--", "-xx--", "--x--", "--x--", "-xxx-" },
	{ "-xxx-", "----x", "--xx-", "-x---", "xxxxx" },
	{ "-xxx-", "x---x", "--xx-", "x---x", "-xxx-" },
	{ "---x-", "--xx-", "-x-x-", "xxxxx", "---x-" },
	{ "xxxx-", "x----", "xxxx-", "----x", "xxxx-" },
	{ "--xx-", "-x---", "xxxx-", "x---x", "-xxx-" },
	{ "xxxxx", "---x-", "--x--", "--x--", "--x--" },
	{ "-xxx-", "x---x", "-xxx-", "x---x", "-xxx-" },
	{ "-xxx-", "x---x", "-xxxx", "---x-", "-xx--" },
	{ "-----", "--x--", "-----", "--x--", "-----" },
	{ "-----", "--x--", "-----", "--x--", "-x---" },
	{ "---x-", "--x--", "-x---", "--x--", "---x-" },
	{ "-----", "-xxx-", "-----", "-xxx-", "-----" },
	{ "-x---", "--x--", "---x-", "--x--", "-x---" },
	{ "-xxx-", "----x", "--xx-", "-----", "--x--" },
	{ "-xxx-", "----x", "-xx-x", "x-x-x", "-xxx-" },
	{ "--x--", "-x-x-", "xxxxx", "x---x", "x---x" },
	{ "xxxx-", "x---x", "xxxx-", "x---x", "xxxx-" },
	{ "-xxx-", "x---x", "x----", "x---x", "-xxx-" },
	{ "xxxx-", "x---x", "x---x", "x---x", "xxxx-" },
	{ "xxxxx", "x----", "xxx--", "x----", "xxxxx" },
	{ "xxxxx", "x----", "xxx--", "x----", "x----" },
	{ "-xxx-", "x----", "x-xxx", "x---x", "-xxx-" },
	{ "x---x", "x---x", "xxxxx", "x---x", "x---x" },
	{ "-xxx-", "--x--", "--x--", "--x--", "-xxx-" },
	{ "--xxx", "---x-", "---x-", "---x-", "-xx--" },
	{ "x---x", "x--x-", "xxx--", "x--x-", "x---x" },
	{ "x----", "x----", "x----", "x----", "xxxxx" },
	{ "x---x", "xx-xx", "x-x-x", "x---x", "x---x" },
	{ "x---x", "xx--x", "x-x-x", "x--xx", "x---x" },
	{ "-xxx-", "x---x", "x---x", "x---x", "-xxx-" },
	{ "xxxx-", "x---x", "xxxx-", "x----", "x----" },
	{ "-xxx-", "x---x", "x-x-x", "x--xx", "-xxxx" },
	{ "xxxx-", "x---x", "xxxx-", "x--x-", "x---x" },
	{ "-xxxx", "x----", "-xxx-", "----x", "xxxx-" },
	{ "xxxxx", "--x--", "--x--", "--x--", "--x--" },
	{ "x---x", "x---x", "x---x", "x---x", "-xxx-" },
	{ "x---x", "x---x", "x---x", "-x-x-", "--x--" },
	{ "x---x", "x---x", "x-x-x", "xx-xx", "x---x" },
	{ "x---x", "-x-x-", "--x--", "-x-x-", "x---x" },
	{ "x---x", "-x-x-", "--x--", "--x--", "--x--" },
	{ "xxxxx", "---x-", "--x--", "-x---", "xxxxx" },
	{ "-xxx-", "-x---", "-x---", "-x---", "-xxx-" },
	{ "x----", "-x---", "--x--", "---x-", "----x" },
	{ "-xxx-", "---x-", "---x-", "---x-", "-xxx-" },
	{ "--x--", "-x-x-", "-----", "-----", "-----" },
	{ "-----", "-----", "-----", "-----", "xxxxx" },
	{ "-x---", "--x--", "-----", "-----", "-----" },
	{ "--x--", "-x-x-", "xxxxx", "x---x", "x---x" },
	{ "xxxx-", "x---x", "xxxx-", "x---x", "xxxx-" },
	{ "-xxx-", "x---x", "x----", "x---x", "-xxx-" },
	{ "xxxx-", "x---x", "x---x", "x---x", "xxxx-" },
	{ "xxxxx", "x----", "xxx--", "x----", "xxxxx" },
	{ "xxxxx", "x----", "xxx--", "x----", "x----" },
	{ "-xxx-", "x----", "x-xxx", "x---x", "-xxx-" },
	{ "x---x", "x---x", "xxxxx", "x---x", "x---x" },
	{ "-xxx-", "--x--", "--x--", "--x--", "-xxx-" },
	{ "--xxx", "---x-", "---x-", "---x-", "-xx--" },
	{ "x---x", "x--x-", "xxx--", "x--x-", "x---x" },
	{ "x----", "x----", "x----", "x----", "xxxxx" },
	{ "x---x", "xx-xx", "x-x-x", "x---x", "x---x" },
	{ "x---x", "xx--x", "x-x-x", "x--xx", "x---x" },
	{ "-xxx-", "x---x", "x---x", "x---x", "-xxx-" },
	{ "xxxx-", "x---x", "xxxx-", "x----", "x----" },
	{ "-xxx-", "x---x", "x-x-x", "x--xx", "-xxxx" },
	{ "xxxx-", "x---x", "xxxx-", "x--x-", "x---x" },
	{ "-xxxx", "x----", "-xxx-", "----x", "xxxx-" },
	{ "xxxxx", "--x--", "--x--", "--x--", "--x--" },
	{ "x---x", "x---x", "x---x", "x---x", "-xxx-" },
	{ "x---x", "x---x", "x---x", "-x-x-", "--x--" },
	{ "x---x", "x---x", "x-x-x", "xx-xx", "x---x" },
	{ "x---x", "-x-x-", "--x--", "-x-x-", "x---x" },
	{ "x---x", "-x-x-", "--x--", "--x--", "--x--" },
	{ "xxxxx", "---x-", "--x--", "-x---", "xxxxx" },
	{ "--xx-", "--x--", "-xx--", "--x--", "--xx-" },
	{ "--x--", "--x--", "-----", "--x--", "--x--" },
	{ "-xx--", "--x--", "--xx-", "--x--", "-xx--" },
	{ "-----", "--x--", "-x-x-", "x---x", "xxxxx" }
};

void DCV_FB_TextPixel( DDSURFACEDESC2 *desc, int x, int y, unsigned short color )
{
	unsigned short *pixels = (unsigned short *)desc->lpSurface;
	pixels[y * (desc->lPitch / 2) + x] = color;
}

static int DCV_FB_Glyph( DDSURFACEDESC2 *pDDSD, int x, int y, int iGlyph, unsigned short wColor )
{
	int			iRow;
	int			iCol;
	int			iXScale;
	int			iYScale;

	for (iRow = 0; iRow < GLYPH_H; ++iRow)
	{
		const char *pRow = (const char *)s_szGlyphRows + (iGlyph * 5 + iRow) * 6;
		int			xCol = x;
		for (iCol = 0; iCol < GLYPH_W; ++iCol)
		{
			if (pRow[iCol] != '-')
			{
				int			xPix = xCol;
				for (iXScale = 0; iXScale < GLYPH_XSCALE; ++iXScale)
				{
					int			yPix = y;
					for (iYScale = 0; iYScale < GLYPH_YSCALE; ++iYScale)
					{
						DCV_FB_TextPixel(pDDSD, xPix, yPix, wColor);
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

fbmeter_t	g_FBMeters[MAX_FB_METERS];

#define METER_TIMED 400
#define METER_VALUE 420

extern DWORD g_dwFlipTick;

fbmeter_t *DCV_AllocMeter( void )
{
	int			i;

	for (i = 0; i < 32; i++)
	{
		if (g_FBMeters[i].type == 0)
			return &g_FBMeters[i];
	}

	return NULL;
}

void DCV_MeterText( unsigned int color, int x, int y, const char* text )
{
	fbmeter_t*	meter;

	if (profilemeter.value >= 1.0f)
	{
		meter = DCV_AllocMeter();
		if (meter)
		{
			meter->type = -11;
			meter->color = color;
			meter->value = (short)x;
			meter->y = (short)y;
			strncpy(meter->text, text, sizeof(meter->text));
			meter->text[sizeof(meter->text) - 1] = 0;
		}
	}
}

void DCV_ClearMeters( int flag )
{
	fbmeter_t*	meter = g_FBMeters;
	int			i;

	for (i = 0; i < MAX_FB_METERS; i++, meter++)
		meter->type = 0;
}

void DCV_AddMeterTimed( unsigned int color )
{
	fbmeter_t *meter;
	float		elapsed;
	int			value;

	if (profilescale.value < 1.0f)
		return;

	elapsed = (float)(GetTickCount() - g_dwFlipTick) * 0.001f;
	if (elapsed < 0.0f || elapsed > 0.2f)
		value = 0;
	else
		value = (int)(profilescale.value * elapsed + 20.0f);

	if (value <= 0)
		return;

	meter = DCV_AllocMeter();
	if (meter)
	{
		meter->type = METER_TIMED;
		meter->color = color;
		meter->value = (short)value;
	}
}

void DCV_AddMeterValue( unsigned int color, float value )
{
	fbmeter_t *meter;

	if (profilescale.value < 1.0f)
		return;

	meter = DCV_AllocMeter();
	if (meter)
	{
		meter->type = METER_VALUE;
		meter->color = color;
		meter->value = (short)(profilescale.value * value + 20.0f);
	}
}

void DCV_DrawMeters( void )
{
	fbmeter_t*	meter;
	int			i, x, right, y, textx;

	y = 465 - scr_safe_y;
	DCV_SetHudDepth(5.5f);
	DCV_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

	for (i = 0, meter = g_FBMeters; i < MAX_FB_METERS; i++, meter++)
	{
		if (!meter->type)
			return;

		x = (int)((float)meter->value + 0.5f);
		if (meter->type == -11)
		{
			right = (int)((float)meter->y + 0.5f);
			DCV_SetHudDepth(5.5f);
			DCV_SetPackedColor(meter->color);
			DCV_TexState_VertColorOpaque();
			DCV_FlushIfLarge();
			DCV_AddPolyIndices(g_nAccumVertCount, 4);
			DCV_AddVertex((float)x, (float)y, dc_depthhud.value, 0.0f, 0.0f);
			DCV_AddVertex((float)right, (float)y, dc_depthhud.value, 1.0f, 0.0f);
			DCV_AddVertex((float)x, (float)(y + 7), dc_depthhud.value, 0.0f, 1.0f);
			DCV_AddVertex((float)right, (float)(y + 7), dc_depthhud.value, 1.0f, 1.0f);

			Font_SetScale(0.5f, 0.5f);
			textx = (x + right - Font_StringWidth((dcfont_t*)draw_chars, (byte*)meter->text)) / 2;
			if (textx < 0)
				textx = 0;
			DCV_TexState_Blend();
			DCV_SetHudDepth(6.5f);
			Text_DrawString(0.5f, 0.5f, meter->text, textx - 1, y - 3, 255, 255, 255);
			y -= 10;
		}
		else
		{
			DCV_SetPackedColor(meter->color);
			DCV_FlushIfLarge();
			DCV_AddPolyIndices(g_nAccumVertCount, 3);
			DCV_AddVertex((float)x, (float)meter->type, dc_depthhud.value, 0.0f, 0.0f);
			DCV_AddVertex((float)x, (float)(meter->type + 7), dc_depthhud.value, 1.0f, 0.0f);
			DCV_AddVertex((float)(x - 7), (float)(meter->type + 7), dc_depthhud.value, 0.0f, 1.0f);
		}
		meter->type = 0;
	}
}

/*
================
DCV_FB_Text

Draw fatal error text directly on the primary surface.
================
*/
void DCV_FB_Text( const char* text )
{
	DDSURFACEDESC2 ddsd;
	int			x, y, xHome, glyph;
	char		ch;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	g_pddsPrimary->lpVtbl->Lock(g_pddsPrimary, NULL, &ddsd, DDLOCK_WAIT, NULL);
	xHome = scr_safe_x;
	x = xHome;
	y = scr_safe_y + START_Y_OFFSET;

	while ((ch = *text++) != 0)
	{
		glyph = ch - 0x20;
		if (glyph > MAX_GLYPH_INDEX)
			glyph = MAX_GLYPH_INDEX;

		if (glyph < 0)
		{
			switch (ch)
			{
			case '\b':
				x -= GLYPH_ADVANCE;
				if (x < xHome)
					x = xHome;
				break;
			case '\t':
				x = ((x + TAB_WIDTH) / TAB_WIDTH) * TAB_WIDTH;
				break;
			case '\n':
				y += LINE_ADVANCE;
				x = xHome;
				break;
			case '\r':
				x = xHome;
				break;
			}
		}
		else
		{
			DCV_FB_Glyph(&ddsd, x + SHADOW_OFFSET, y + SHADOW_OFFSET, glyph, SHADOW_COLOR);
			x = DCV_FB_Glyph(&ddsd, x, y, glyph, TEXT_COLOR);
			xHome = scr_safe_x;
			if (x >= 640 - xHome - 32)
			{
				y += LINE_ADVANCE;
				x = xHome;
			}
		}
	}
	g_pddsPrimary->lpVtbl->Unlock(g_pddsPrimary, NULL);
}

// Hand the console back to the firmware after the drive door opens.
void GDROM_DoorReset( void )
{
	ResetToFirmware();
}

int DCV_MeterElapsed( void )
{
	float		elapsed = (float)(GetTickCount() - g_dwFlipTick) * 0.001f;
	if (elapsed < 0.0f || elapsed > 0.2f)
		return 0;
	return (int)(profilescale.value * elapsed + 20.0f);
}

void DCV_UpdateMeters( void )
{
	if (profilescale.value > 0.0f)
	{
		if (profilescale.value >= 1.0f)
		{
			int			value = DCV_MeterElapsed();
			if (value > 0)
			{
				fbmeter_t *meter = DCV_AllocMeter();
				if (meter)
				{
					meter->type = METER_TIMED;
					meter->color = 0xffffffff;
					meter->value = (short)value;
				}
			}
		}
	}
	DCV_DrawMeters();
	g_dwFlipTick = GetTickCount();
}

void DCV_AddMeterMarker( unsigned int color, float marker )
{
	fbmeter_t *meter;
	int			value;
	if (profilescale.value < 1.0f)
		return;
	value = DCV_MeterElapsed();
	if (value <= 0)
		return;
	meter = DCV_AllocMeter();
	if (meter)
	{
		meter->type = (short)(marker * -10.0f + 400.0f);
		meter->color = color;
		meter->value = (short)value;
	}
}

void DCV_AddMeterCount( unsigned int color, short value )
{
	fbmeter_t *meter;
	if (profilescale.value < 1.0f)
		return;
	meter = DCV_AllocMeter();
	if (meter)
	{
		meter->type = METER_VALUE;
		meter->color = color;
		meter->value = value;
	}
}

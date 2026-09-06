// text_draw.cpp -- localized on-screen text and font rendering for the
// Dreamcast port. Draws HUD, console, loading and menu text through a single
// scaled glyph renderer, with %tag substitution driven by langtags.txt so the
// same message strings work across the supported languages.

// Use C-style (lpVtbl) COM interfaces so the DDraw/D3D headers match the rest
// of the engine when this translation unit is compiled as C++.
#define CINTERFACE

// The engine headers are C; include them (and define everything here) with C
// linkage so the symbols match the rest of the statically linked build.
extern "C" {

#include "quakedef.h"
#include "winquake.h"
#include "dc_accum.h"
#include "text_draw.h"

// A %tag -> localized string pair parsed from langtags.txt.
// Index of the language the disc was built for; 0 is English.
int			g_Language;

langtag_t	*g_pLangTags;
int			g_nLangTags;

// Fonts scale down for languages whose strings run longer; sv_language
// selects the reduced set. The active scale is latched here and applied by
// the glyph renderer.
float	g_flTextScaleX = 1.0f;
float	g_flTextScaleY = 1.0f;

// Extra texels of spacing added to the right edge of every glyph quad.
int		g_nTextCharGap;

// Character codes at or above this index hold the localized (high-range)
// glyphs; they are shifted down into the accented block before lookup.
#define FONT_HIGHCHAR		0xC0
#define FONT_HIGHCHAR_SHIFT	0x40

// Per-glyph advance is clamped to this pixel range so proportional fonts
// stay readable at any scale.
#define FONT_ADV_MIN		2
#define FONT_ADV_MAX		0x28

// Languages other than the default use a slightly smaller font so longer
// translated strings still fit.
#define LANG_FONT_SCALE		0.83f

// Rounding/spacing added to the scaled metrics before truncating to pixels.
#define FONT_HEIGHT_BIAS	0.9f
#define FONT_WIDTH_BIAS		1.4f

// Half-texel insets that keep glyph cells from bleeding into their neighbours
// in the font sheet.
#define GLYPH_U_BIAS		0.1f
#define GLYPH_V_BIAS		0.2f
#define GLYPH_U_INSET		0.5f

// The scale is reduced by this fraction each pass until a string fits.
#define FIT_SCALE_STEP		0.01f

// HUD sublayers the shadowed text passes draw into.
#define TEXT_DEPTH_SHADOW	3.0f
#define TEXT_DEPTH_FRONT	4.0f
#define TEXT_DEPTH_RESET	2.0f

// Highlighted lines use a cool tint; normal lines an amber one. The channel
// values are brightness scaled by these 0..255 numerators.
#define TINT_AMBER_GREEN	0xA0
#define TINT_COOL		0x5F
#define COLOR_MAX		255

// Controller-status overlay: default pulse brightness and how long the
// "controller ok" state lingers before the warning appears.
#define STATUS_BRIGHTNESS	220
#define CONTROLLER_GRACE	3.0f

// Longest tag or value string langtags.txt may hold.
#define LANGTAG_MAXLEN		0x7f

void	GL_BindStage( int texnum, int stage );
void	DCV_SetHudDepth( float layer );
int		IN_ControllerPresent( void );

static float	g_flControllerOkTime;

/*
================
Font_SetScale

Latch the glyph scale, reducing it for languages that need longer strings.
================
*/
void Font_SetScale( float sx, float sy )
{
	if (sv_language.value != 0.0f)
	{
		sx = sx * LANG_FONT_SCALE;
		sy = sy * LANG_FONT_SCALE;
	}

	g_flTextScaleX = sx;
	g_flTextScaleY = sy;
}

/*
================
Font_GetCharWide

Scaled advance width of one character.
================
*/
int Font_GetCharWide( dcfont_t *font, int ch )
{
	if (ch >= FONT_HIGHCHAR)
		ch -= FONT_HIGHCHAR_SHIFT;

	return (int)((float)(int)font->fontinfo[ch].charwidth * g_flTextScaleX + FONT_WIDTH_BIAS);
}

/*
================
Font_CharHeight

Scaled pixel height of one text row in this font.
================
*/
int Font_CharHeight( dcfont_t *font )
{
	return (int)((float)font->rowheight * g_flTextScaleY + FONT_HEIGHT_BIAS);
}

/*
================
Font_StringWidth

Total advance width of a string at the current scale. Per-glyph advance is
clamped to a sane range so proportional fonts stay readable.
================
*/
int Font_StringWidth( dcfont_t *font, byte *str )
{
	int	total = 0;
	byte	c;
	int	num;
	int	adv;

	if (!str)
		return 0;

	c = *str;
	if (c != 0)
	{
		do {
			num = c;
			if (num >= FONT_HIGHCHAR)
				num -= FONT_HIGHCHAR_SHIFT;

			adv = (int)((float)(int)font->fontinfo[num].charwidth * g_flTextScaleX + FONT_WIDTH_BIAS);
			if (adv < FONT_ADV_MIN)
				adv = FONT_ADV_MIN;
			if (adv > FONT_ADV_MAX)
				adv = FONT_ADV_MAX;

			total += adv;
			str++;
			c = *str;
		} while (c != 0);
	}

	return total;
}

/*
================
Font_DrawChar

Emit one scaled glyph quad at (x,y) and return its advance width. Spaces
advance without drawing.
================
*/
float Font_DrawChar( float x, float y, dcfont_t *font, int ch )
{
	charinfo_t	*ci;
	int			startoffset, charwidth, rowheight;
	float		advance, height;
	float		u0, v0, u1, v1;
	float		x2, ybot;
	int			base;

	ch &= 255;
	if (ch >= FONT_HIGHCHAR)
		ch -= FONT_HIGHCHAR_SHIFT;

	ci = &font->fontinfo[ch];
	charwidth = ci->charwidth;
	if (!charwidth)
		return 0.0f;

	startoffset = (unsigned short)ci->startoffset;

	height = (float)draw_chars->rowheight * g_flTextScaleY + FONT_HEIGHT_BIAS;
	advance = (float)(int)((float)charwidth * g_flTextScaleX + FONT_WIDTH_BIAS);
	rowheight = font->rowheight;

	u0 = ((float)(startoffset & 255) + GLYPH_U_BIAS) * font->usize;
	v0 = ((float)(startoffset >> 8) + GLYPH_V_BIAS) * font->vsize;

	if (ch != ' ')
	{
		GL_DisableMultitexture();
		GL_BindStage(char_texture, 0);
		DCV_FlushIfLarge();

		base = DCV_GetVertCount();
		DCV_AddPolyIndices(base, 4);

		u1 = u0 + ((float)charwidth - GLYPH_U_INSET) * font->usize;
		v1 = v0 + (float)(rowheight - 1) * font->vsize;
		x2 = x + advance + g_flTextScaleX * (float)g_nTextCharGap;
		ybot = y + (float)(int)height;

		// DCV_AddPolyIndices splits the quad along the vertex-1/vertex-2 diagonal
		// (triangles {0,1,2} and {1,3,2}), which needs the corners added in
		// row-major order -- TL, TR, BL, BR -- not a perimeter walk.
		DCV_AddVertex(x,  y,    dc_depthhud.value, u0, v0);
		DCV_AddVertex(x2, y,    dc_depthhud.value, u1, v0);
		DCV_AddVertex(x,  ybot, dc_depthhud.value, u0, v1);
		DCV_AddVertex(x2, ybot, dc_depthhud.value, u1, v1);
	}

	return advance;
}

int Font_DrawCharI( dcfont_t *font, int x, int y, int ch )
{
	return (int)Font_DrawChar((float)x, (float)y, font, ch);
}

/*
================
Text_FindString

If psz names a %tag, return its localized string; otherwise return psz.
================
*/
char *Text_FindString( char *psz )
{
	int i;

	for (i = 0; i < g_nLangTags; i++)
	{
		if (!strcmp(psz, g_pLangTags[i].tag))
			return g_pLangTags[i].string;
	}

	return psz;
}

/*
================
Text_DrawString

Draw a (possibly %tag) string left-aligned at (x,y) in the given color.
================
*/
void Text_DrawString( float sx, float sy, char *str, int x, int y, int r, int g, int b )
{
	int		i, ch;

	if (str != 0 && *str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(str, g_pLangTags[i].tag))
			{
				str = g_pLangTags[i].string;
				break;
			}
		}
	}

	if (sv_language.value != 0.0f)
	{
		sx = sx * LANG_FONT_SCALE;
		sy = sy * LANG_FONT_SCALE;
	}

	g_flTextScaleX = sx;
	g_flTextScaleY = sy;

	DCV_SetColor(r, g, b, 255);

	ch = *str;
	if (ch != 0)
	{
		do {
			str++;
			x += (int)Font_DrawChar((float)x, (float)y, (dcfont_t *)draw_chars, ch);
			ch = *str;
		} while (ch != 0);
	}
}

/*
================
Text_DrawStringShadow

Draw a string twice: a black drop shadow two pixels off on the layer behind,
then the tinted text. brightness ramps the tint; blue selects the cool tint
used for highlighted lines instead of the default amber.
================
*/
void Text_DrawStringShadow( char *str, int x, int y, int brightness, int blue )
{
	char	*psz;
	int		i, cx, ch;
	int		r, g, b;

	DCV_SetHudDepth(TEXT_DEPTH_SHADOW);

	psz = str;
	if (str != 0 && *str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
			if (!strcmp(str, g_pLangTags[i].tag)) { psz = g_pLangTags[i].string; break; }
	}

	cx = x + 2;
	DCV_SetColor(0, 0, 0, 255);

	ch = *psz;
	if (ch != 0)
	{
		do {
			psz++;
			cx += (int)Font_DrawChar((float)cx, (float)(y + 2), (dcfont_t *)draw_chars, ch);
			ch = *psz;
		} while (ch != 0);
	}

	DCV_SetHudDepth(TEXT_DEPTH_FRONT);

	psz = str;
	if (str != 0 && *str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
			if (!strcmp(str, g_pLangTags[i].tag)) { psz = g_pLangTags[i].string; break; }
	}

	if (blue == 0)
	{
		r = brightness;
		g = brightness * TINT_AMBER_GREEN / COLOR_MAX;
		b = 0;
	}
	else
	{
		r = brightness * TINT_COOL / COLOR_MAX;
		g = brightness * TINT_COOL / COLOR_MAX;
		b = brightness;
	}
	DCV_SetColor(r, g, b, 255);

	ch = *psz;
	if (ch != 0)
	{
		do {
			psz++;
			x += (int)Font_DrawChar((float)x, (float)y, (dcfont_t *)draw_chars, ch);
			ch = *psz;
		} while (ch != 0);
	}

	DCV_SetHudDepth(TEXT_DEPTH_RESET);
}

/*
================
Font_FitScale

Shrink the scale in 0.01 steps until the string fits within maxwidth,
keeping the aspect ratio; returns the chosen scale through pfx/pfy.
================
*/
void Font_FitScale( float sx, float sy, float maxwidth, byte *str, float *pfx, float *pfy )
{
	float	step, fx, fy;
	byte	c, *p;
	int		width, num, adv;

	g_flTextScaleX = sx;
	g_flTextScaleY = sy;
	step = 0.01f;
	width = 0;
	fx = sx;
	fy = sy;

	if (str != 0)
	{
		if (*str != 0)
		{
			c = *str;
			p = str;
			do {
				num = c;
				if (num >= FONT_HIGHCHAR)
					num -= FONT_HIGHCHAR_SHIFT;
				adv = (int)((float)(int)((dcfont_t *)draw_chars)->fontinfo[num].charwidth * sx + FONT_WIDTH_BIAS);
				if (adv < FONT_ADV_MIN) adv = FONT_ADV_MIN;
				if (adv > FONT_ADV_MAX) adv = FONT_ADV_MAX;
				width += adv;
				p++;
				c = *p;
			} while (c != 0);
		}
	}
	else
	{
		width = 0;
	}

	while (maxwidth < (float)width)
	{
		fx = fx - step;
		fy = (sy * fx) / sx;
		g_flTextScaleX = fx;
		g_flTextScaleY = fy;

		width = 0;
		if (str != 0)
		{
			if (*str != 0)
			{
				c = *str;
				p = str;
				do {
					num = c;
					if (num >= FONT_HIGHCHAR)
						num -= FONT_HIGHCHAR_SHIFT;
					adv = (int)((float)(int)((dcfont_t *)draw_chars)->fontinfo[num].charwidth * fx + FONT_WIDTH_BIAS);
					if (adv < FONT_ADV_MIN) adv = FONT_ADV_MIN;
					if (adv > FONT_ADV_MAX) adv = FONT_ADV_MAX;
					width += adv;
					p++;
					c = *p;
				} while (c != 0);
			}
		}
		else
		{
			width = 0;
		}
	}

	*pfx = fx;
	*pfy = fy;
}

/*
================
Text_DrawStringLeft

Localize, shrink to fit maxwidth, then shadow-draw left-aligned at x.
================
*/
void Text_DrawStringLeft( float sx, float sy, char *str, int x, int y, int brightness, int blue, int maxwidth )
{
	char	*psz;
	float	fx, fy;
	int		i;

	psz = str;
	if (*str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(str, g_pLangTags[i].tag))
			{
				psz = g_pLangTags[i].string;
				break;
			}
		}
	}

	Font_FitScale(sx, sy, (float)maxwidth, (byte *)psz, &fx, &fy);
	g_flTextScaleX = fx;
	g_flTextScaleY = fy;

	Text_DrawStringShadow(psz, x, y, brightness, blue);
}

/*
================
Text_DrawStringRight

As Text_DrawStringLeft, but x names the right edge of the text.
================
*/
void Text_DrawStringRight( float sx, float sy, char *str, int x, int y, int brightness, int blue, int maxwidth )
{
	char	*psz;
	float	fx, fy;
	int		i, width, num, adv;
	byte	c, *p;

	psz = str;
	if (*str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(str, g_pLangTags[i].tag))
			{
				psz = g_pLangTags[i].string;
				break;
			}
		}
	}

	Font_FitScale(sx, sy, (float)maxwidth, (byte *)psz, &fx, &fy);
	g_flTextScaleX = fx;
	g_flTextScaleY = fy;

	width = 0;
	if (psz != 0 && *psz != 0)
	{
		c = *(byte *)psz;
		p = (byte *)psz;
		do {
			num = c;
			if (num >= FONT_HIGHCHAR)
				num -= FONT_HIGHCHAR_SHIFT;
			adv = (int)((float)(int)((dcfont_t *)draw_chars)->fontinfo[num].charwidth * fx + FONT_WIDTH_BIAS);
			if (adv < FONT_ADV_MIN) adv = FONT_ADV_MIN;
			if (adv > FONT_ADV_MAX) adv = FONT_ADV_MAX;
			width += adv;
			p++;
			c = *p;
		} while (c != 0);
	}

	Text_DrawStringShadow(psz, x - width, y, brightness, blue);
}

/*
================
Text_DrawStringCentered

As Text_DrawStringLeft, but x names the center of the text.
================
*/
void Text_DrawStringCentered( float sx, float sy, char *str, int x, int y, int brightness, int blue, int maxwidth )
{
	char	*psz;
	float	fx, fy;
	int		i, width, num, adv;
	byte	c, *p;

	psz = str;
	if (*str == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(str, g_pLangTags[i].tag))
			{
				psz = g_pLangTags[i].string;
				break;
			}
		}
	}

	Font_FitScale(sx, sy, (float)maxwidth, (byte *)psz, &fx, &fy);
	g_flTextScaleX = fx;
	g_flTextScaleY = fy;

	width = 0;
	if (psz != 0 && *psz != 0)
	{
		c = *(byte *)psz;
		p = (byte *)psz;
		do {
			num = c;
			if (num >= FONT_HIGHCHAR)
				num -= FONT_HIGHCHAR_SHIFT;
			adv = (int)((float)(int)((dcfont_t *)draw_chars)->fontinfo[num].charwidth * fx + FONT_WIDTH_BIAS);
			if (adv < FONT_ADV_MIN) adv = FONT_ADV_MIN;
			if (adv > FONT_ADV_MAX) adv = FONT_ADV_MAX;
			width += adv;
			p++;
			c = *p;
		} while (c != 0);
	}

	Text_DrawStringShadow(psz, x - width / 2, y, brightness, blue);
}

/*
================
Text_DrawCenteredStatus

Draw a centered status line. With no controller plugged in, the message is
replaced by the controller warning (a short grace period shows "check"
before "no controller") and the text pulses.
================
*/
void Text_DrawCenteredStatus( float sx, float sy, byte *str, int color )
{
	byte	*psz;
	int		i, width;
	int		brightness;
	float	fx, fy;

	DCV_SetHudDepth(TEXT_DEPTH_FRONT);
	DCV_TexState_Additive();

	brightness = STATUS_BRIGHTNESS;

	if (!IN_ControllerPresent())
	{
		if (Sys_FloatTime() - g_flControllerOkTime < 3.0f)
			str = (byte *)"%check_controller";
		else
			str = (byte *)"%no_controller";

		brightness = (int)((coss(Sys_FloatTime() * 4.23f) + 1.0f) * 127.0f);
	}
	else
	{
		g_flControllerOkTime = Sys_FloatTime();
	}

	psz = str;
	if (str && *str == '%')
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp((char *)str, g_pLangTags[i].tag))
			{
				psz = (byte *)g_pLangTags[i].string;
				break;
			}
		}
	}

	Font_FitScale(sx, sy, 430.0f, psz, &fx, &fy);

	if (sv_language.value)
	{
		fx = sx * LANG_FONT_SCALE;
		fy = sy * LANG_FONT_SCALE;
	}

	g_flTextScaleX = fx;
	g_flTextScaleY = fy;

	width = Font_StringWidth((dcfont_t *)draw_chars, psz);

	DCV_SetColor(color, color, color, brightness);

	i = 320 - width / 2;
	while (*psz)
	{
		i += Font_DrawCharI((dcfont_t *)draw_chars, i, 416, *psz);
		psz++;
	}
}

/*
================
Text_LoadLangTags

Read langtags.txt and build the %tag -> string table. Each entry is written as
	%tagname:localized text;
================
*/
void Text_LoadLangTags( void )
{
	char	*buf;
	char	tmp[LANGTAG_MAXLEN + 1];
	int		len;
	int		i, n, count;
	qboolean	invalue;
	char	*p;

	len = 0;
	buf = (char *)COM_LoadFileForMe("langtags.txt", &len);

	g_nLangTags = 0;
	for (i = 0; i < len; i++)
	{
		if (buf[i] == '%')
			g_nLangTags++;
	}

	g_pLangTags = (langtag_t *) new char[(g_nLangTags + 1) * sizeof(langtag_t)];

	invalue = false;
	count = 0;
	n = 0;

	if (len > 0)
	{
		p = buf;
		i = 0;
		do {
			char c = *p;

			if (c == '%')
			{
				count = 1;
				tmp[0] = '%';
			}
			else if (c == ':' && !invalue)
			{
				tmp[count] = 0;
				g_pLangTags[n].tag = new char[count + 1];
				strncpy(g_pLangTags[n].tag, tmp, count + 1);
				count = 0;
				invalue = true;
			}
			else if ((c == ';' || c == '\n') && invalue)
			{
				tmp[count] = 0;
				g_pLangTags[n].string = new char[count + 1];
				strncpy(g_pLangTags[n].string, tmp, count + 1);
				n++;
				invalue = false;
			}
			else if (count < LANGTAG_MAXLEN)
			{
				tmp[count] = c;
				count++;
			}

			i++;
			p++;
		} while (i < len);
	}

	if (buf)
		COM_FreeFile();
}

/*
================
Text_ParseToken

Copy one token, honoring quotes; stops at whitespace outside quotes or at
end of line. Returns the read position.
================
*/
char *Text_ParseToken( char *in, char *out )
{
	char	c;
	qboolean	outside;

	outside = true;

	while (1)
	{
		c = *in;

		if (c == '\0' || c == '\n')
		{
			*out = '\0';
			return in;
		}

		if (outside && (c == ' ' || c == '\t'))
			break;

		if (c == '\"')
			outside = !outside;
		else
			*out++ = c;

		in++;
	}

	*out = '\0';
	return in;
}

} // extern "C"

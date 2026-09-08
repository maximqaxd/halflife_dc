#ifndef TEXT_DRAW_H
#define TEXT_DRAW_H

#define FONT_GLYPH_COUNT	256

// One glyph's placement in the font sheet.
typedef struct
{
	short	startoffset;	// packed row<<8 | column into the sheet
	short	charwidth;		// advance width in source pixels
} charinfo_t;

// Font sheet header. fontinfo[] is indexed by character code; the loader
// appends the sheet's texel scale so glyph cells convert straight to UVs.
typedef struct
{
	int			width;
	int			height;
	int			rowcount;
	int			rowheight;			// source pixel height of a text row
	charinfo_t	fontinfo[FONT_GLYPH_COUNT];
	byte		data[4];
	float		usize;				// 1.0 / sheet width
	float		vsize;				// 1.0 / sheet height
} dcfont_t;

// A %tag and the string it stands for in the language the disc was built for
typedef struct
{
	char*	tag;
	char*	string;
} langtag_t;

#ifdef __cplusplus
extern "C" {
#endif

// Text is laid out for the language the disc was built for
extern cvar_t sv_language;
extern int    g_Language;

extern langtag_t*	g_pLangTags;
extern int			g_nLangTags;

// The scale the glyph renderer draws at
extern float	g_flTextScaleX;
extern float	g_flTextScaleY;
extern int		g_nTextCharGap;

static __forceinline void Font_ApplyScale( float sx, float sy )
{
	if (sv_language.value != 0.0f)
	{
		sx *= 0.83f;
		sy *= 0.83f;
	}
	g_flTextScaleX = sx;
	g_flTextScaleY = sy;
}

static __forceinline int Font_MeasureString( dcfont_t *font, byte *str )
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
			if (num >= 0xC0)
				num -= 0x40;

			adv = (int)((float)(int)font->fontinfo[num].charwidth * g_flTextScaleX + 1.4f);
			if (adv < 2)
				adv = 2;
			if (adv > 40)
				adv = 40;

			total += adv;
			str++;
			c = *str;
		} while (c != 0);
	}

	return total;
}

static __forceinline char *Text_LocalizeString( char *psz )
{
	int i;

	for (i = 0; i < g_nLangTags; i++)
	{
		if (!strcmp(psz, g_pLangTags[i].tag))
			return g_pLangTags[i].string;
	}

	return psz;
}

// Glyph metrics and drawing
void	Font_SetScale( float sx, float sy );
int		Font_GetCharWide( dcfont_t* font, int ch );
int		Font_CharHeight( dcfont_t* font );
int		Font_StringWidth( dcfont_t* font, byte* str );
float	Font_DrawChar( float x, float y, dcfont_t* font, int ch );
int		Font_DrawCharI( dcfont_t* font, int x, int y, int ch );
void	Font_FitScale( float sx, float sy, float maxwidth, byte* str, float* pfx, float* pfy );

// Localized strings and the text passes that use them
char*	Text_FindString( char* psz );
void	Text_DrawString( float sx, float sy, char* str, int x, int y, int r, int g, int b );
char* Text_SkipSpace( char* in );
void Text_DrawScaledStringShadow( float sx, float sy, char* str, int x, int y, int brightness, int blue );
void	Text_DrawStringShadow( char* str, int x, int y, int brightness, int blue );
void	Text_DrawStringLeft( float sx, float sy, char* str, int x, int y, int brightness, int blue, int maxwidth );
void	Text_DrawStringRight( float sx, float sy, char* str, int x, int y, int brightness, int blue, int maxwidth );
void	Text_DrawStringCentered( float sx, float sy, char* str, int x, int y, int brightness, int blue, int maxwidth );
void	Text_DrawCenteredStatus( byte* str, float time, int color, float sx, float sy );
void	Text_LoadLangTags( void );
char*	Text_ParseToken( char* in, char* out );
void	Text_LoadAliases( char*** pppAliases, int* pnAliases, char* pszFile );

#ifdef __cplusplus
}
#endif

#endif // TEXT_DRAW_H

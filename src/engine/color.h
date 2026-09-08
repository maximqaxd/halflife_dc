#ifndef COLOR_H
#define COLOR_H
#pragma once

/*
	color.h

	Color depth info
*/

// Screen bit depth
#define _16BIT_
//#define _8BIT_

#define ColorSubtract(a,d,c) { c.r=a.r-d.r; c.g=a.g-d.g; c.b=a.b-d.b; }
#define ColorAdd(a,d,c) { c.r=a.r+d.r; c.g=a.g+d.g; c.b=a.b+d.b; }
#define ColorCopy(a,d) { d.r=a.r; d.g=a.g; d.b=a.b; }
#define ColorShift(a,n) { a.r>>=n; a.g>>=n; a.b>>=n; }

// TEMP
// NOTE amt is [0, 255]
#define ScaleToColor( oldColor, newColor, mask, amt ) ( (oldColor&mask) + ( ( (long) ( (newColor&mask) - (oldColor&mask) ) * (byte) amt ) >> 8  ) )
#define MixColor( oldColor, newColor, mask, amt ) ( (long)(oldColor&mask) + (((long)(newColor&mask) * (byte)amt) >> 8) )
#define AddColor( oldColor, newColor, mask )	((long)(oldColor&mask) + (long)(newColor&mask))

#define PACKEDRGB565(r,g,b)	(unsigned short) ( (((r)<<8) & 0xF800) | (((g)<<3) & 0x07E0) | (((b)>>3)) )
#define PACKEDRGB555(r,g,b)	(unsigned short) ( (((r)<<7) & 0x7C00) | (((g)<<2) & 0x03E0) | (((b)>>3)) )
#define PALETTE24( dest, p, i )			( (dest)->r = (p)[ (i)*4 + 2 ], (dest)->g = (p)[ (i)*4 + 1 ], (dest)->b = (p)[ (i)*4 + 0 ] )
#define RGB_RED565( c )			( ((c) & 0xF800) >> 11 )
#define RGB_GREEN565( c )		( ((c) & 0x07E0) >> 5 )
#define RGB_BLUE565( c )		( ((c) & 0x001F) )

#define RGB_RED555( c )			( ((c) & 0x7C00) >> 10 )
#define RGB_GREEN555( c )		( ((c) & 0x03E0) >> 5 )
#define RGB_BLUE555( c )		( ((c) & 0x001F) )

#define RGBPAL565(p,i)			( ( ( (short) *(((p)+((i)*4)) + 2) << 8 ) & 0xf800 ) | ( ( (short) *(((p)+((i)*4)) + 1) << 3 ) & 0x07e0 ) | ( (short) *(((p)+((i)*4)) + 0) >> 3 ) )
#define RGBPAL555(p,i)			( ( ( (short) *(((p)+((i)*4)) + 2) << 7 ) & 0x7c00 ) | ( ( (short) *(((p)+((i)*4)) + 1) << 2 ) & 0x03e0 ) | ( (short) *(((p)+((i)*4)) + 0) >> 3 ) )

extern qboolean is15bit;
extern word* r_palette;
extern byte r_lut[65536];
extern byte t_lut[65536];
extern word red_64klut[65536];
extern word green_64klut[65536];
extern word blue_64klut[65536];

extern short hlRGB( word* p, int i );
extern short PackedRGB( byte* p, int i );
#ifdef __cplusplus
extern "C" {
#endif
unsigned short PutRGB( colorVec* pcv );
void GetRGB( unsigned short color, colorVec* pcv );
/* Four 16-bit components, sizeof(colorVec16) == 8. */
typedef struct
{
	unsigned short r, g, b, a;
} colorVec16;

unsigned short Color8888To4444( int color );
unsigned int R_DecalColor4444to32( int color );
unsigned short Color24To565( color24* color );
void Color565To24( int color, color24* out );
unsigned short ColorVecTo4444( colorVec* pcv );
void GetRGB16( int color, colorVec16* out );
#ifdef __cplusplus
}
#endif

#endif	// COLOR_H

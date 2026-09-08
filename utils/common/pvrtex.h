/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

//
// pvrtex.h: PowerVR texture images for the Dreamcast
//

#ifndef PVRTEX_H
#define PVRTEX_H

// pixel formats
#define PVR_ARGB1555			0x00
#define PVR_RGB565				0x01
#define PVR_ARGB4444			0x02

// data formats
#define PVR_TWIDDLE				0x01
#define PVR_TWIDDLED_MIPMAP		0x02
#define PVR_VQ					0x03
#define PVR_VQ_MIPMAP			0x04
#define PVR_RECT				0x09

// A texture image is a global index block followed by the image itself.  The
// hardware wants the pixel data untouched, so the tools hand the whole thing
// to the engine and it goes straight into video memory.
typedef struct
{
	char			gbixMagic[4];		// "GBIX"
	int				gbixLength;			// bytes of global index that follow
	unsigned int	globalIndex[2];
	char			pvrtMagic[4];		// "PVRT"
	int				dataSize;			// pixel data size plus eight
	unsigned char	pixelFormat;
	unsigned char	dataFormat;
	unsigned short	reserved;
	short			width;
	short			height;
} pvrheader_t;

#define PVR_HEADER_BYTES		sizeof( pvrheader_t )
#define PVR_CODEBOOK_BYTES		2048		// 256 entries of a 2x2 texel block

// what PVR_EncodeIndexed should build
#define PVR_NO_MIPMAPS			0			// models: one level, sampled flat
#define PVR_WITH_MIPMAPS		1			// world textures: the whole chain

// Reads a .pvr image off disk.  Returns the whole file, including its header,
// ready to be copied into a model or a wad lump.  Errors out if the file is
// not something the Dreamcast can upload.
void *PVR_LoadFile( const char *filename, int *psize, int *pwidth, int *pheight );

// Reads the dimensions and total size of an image already in memory.  Returns
// zero if the block does not start with a global index and an image header.
int PVR_GetImageInfo( const void *data, int *psize, int *pwidth, int *pheight );

// Converts an 8 bit image and its palette into a texture image.  Square
// textures of 64 or more come out vector quantized, everything else stays
// uncompressed.  The result is malloced and carries its own header.
void *PVR_EncodeIndexed( const byte *pixels, const byte *palette, int width, int height,
						 int mipmaps, int *psize );

// How large PVR_EncodeIndexed's answer will be, without doing the work.  The
// size only depends on the dimensions, so a caller can budget ahead of time.
int PVR_EncodedSize( int width, int height, int mipmaps );

// True if the dimensions can be uploaded at all: powers of two from 8 to 1024.
int PVR_ValidSize( int width, int height );

// The power of two the value is closest to, clamped to what the hardware takes.
int PVR_NearestSize( int value );

#endif // PVRTEX_H

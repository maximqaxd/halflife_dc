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
// pvrtex.c: PowerVR texture images for the Dreamcast
//
// The hardware reads two kinds of image the tools care about.  A rectangular
// image is plain 16 bit pixels in scanline order.  A vector quantized image is
// a codebook of 256 two by two texel blocks followed by one byte per block
// picking an entry out of it, which is a quarter of the memory.  Square images
// store their blocks twiddled: the address is the bits of x and y interleaved,
// so a block and its neighbours in both directions land close together.
//
// A mipmapped image keeps one codebook for the whole chain and stores the
// levels smallest first, each one starting on a long boundary, with the top
// level last.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdlib.h"
#include "pvrtex.h"

#define VQ_CODEBOOK_ENTRIES		256
#define VQ_BLOCK_TEXELS			4
#define VQ_BLOCK_VALUES			(VQ_BLOCK_TEXELS * 3)	// r, g and b per texel
#define VQ_REFINE_PASSES		8
#define VQ_MIN_SIZE				64		// smaller skins lose too much to quantizing
#define PVR_SIZE_ALIGN			32		// textures are padded out to a whole header

#define MAX_MIP_LEVELS			11

typedef struct
{
	float	value[VQ_BLOCK_VALUES];
} vqblock_t;

/*
================
PVR_TwiddleIndex

Interleaves the bits of x and y, y first.
================
*/
static int PVR_TwiddleIndex( int x, int y )
{
	int		index;
	int		bit;

	index = 0;
	for (bit = 0; bit < 16; bit++)
	{
		index |= ((y >> bit) & 1) << (bit * 2);
		index |= ((x >> bit) & 1) << (bit * 2 + 1);
	}

	return index;
}

/*
================
PVR_PackRGB565
================
*/
static unsigned short PVR_PackRGB565( int r, int g, int b )
{
	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;

	return (unsigned short)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/*
================
PVR_ValidSize
================
*/
int PVR_ValidSize( int width, int height )
{
	if (width < 8 || height < 8 || width > 1024 || height > 1024)
		return 0;
	if (width & (width - 1))
		return 0;
	if (height & (height - 1))
		return 0;

	return 1;
}

/*
================
PVR_NearestSize
================
*/
int PVR_NearestSize( int value )
{
	int		size;

	for (size = 8; size < 1024; size = size * 2)
	{
		if (value < size + size / 2)
			break;
	}

	return size;
}

/*
================
PVR_UseVQ

Vector quantizing only pays for itself on the larger square textures.
================
*/
static int PVR_UseVQ( int width, int height )
{
	return width == height && width >= VQ_MIN_SIZE;
}

/*
================
PVR_MipIndexBytes

How much room the block indices take.  The levels sit one after another with
no padding between them, smallest first.
================
*/
static int PVR_MipIndexBytes( int width )
{
	int		bytes;
	int		size;

	bytes = 0;
	for (size = 1; size <= width; size = size * 2)
	{
		int blocks = (size / 2) * (size / 2);
		if (blocks < 1)
			blocks = 1;
		bytes += blocks;
	}

	return bytes;
}

/*
================
PVR_EncodedSize
================
*/
int PVR_EncodedSize( int width, int height, int mipmaps )
{
	int		size;

	if (!PVR_UseVQ( width, height ))
		size = PVR_HEADER_BYTES + width * height * 2;
	else if (mipmaps)
		size = PVR_HEADER_BYTES + PVR_CODEBOOK_BYTES + PVR_MipIndexBytes( width );
	else
		size = PVR_HEADER_BYTES + PVR_CODEBOOK_BYTES + (width / 2) * (height / 2);

	// the image is rounded out so the next one starts on a header boundary
	return (size + PVR_SIZE_ALIGN - 1) & ~(PVR_SIZE_ALIGN - 1);
}

/*
================
PVR_GetImageInfo
================
*/
int PVR_GetImageInfo( const void *data, int *psize, int *pwidth, int *pheight )
{
	const pvrheader_t	*header;

	header = (const pvrheader_t *)data;
	if (strncmp( header->gbixMagic, "GBIX", 4 ) || strncmp( header->pvrtMagic, "PVRT", 4 ))
		return 0;

	if (psize)
		*psize = PVR_HEADER_BYTES + header->dataSize - 8;
	if (pwidth)
		*pwidth = header->width;
	if (pheight)
		*pheight = header->height;

	return 1;
}

/*
================
PVR_LoadFile
================
*/
void *PVR_LoadFile( const char *filename, int *psize, int *pwidth, int *pheight )
{
	pvrheader_t	*header;
	int			length;
	int			expected;

	length = LoadFile( (char *)filename, (void **)&header );

	if (length < (int)PVR_HEADER_BYTES)
		Error( "\"%s\" is too small to be a PVR texture\n", filename );

	if (strncmp( header->gbixMagic, "GBIX", 4 ))
		Error( "\"%s\" has no GBIX global index\n", filename );

	if (header->gbixLength != 8)
		Error( "\"%s\" has a %d byte global index, expected 8\n", filename, header->gbixLength );

	if (strncmp( header->pvrtMagic, "PVRT", 4 ))
		Error( "\"%s\" has no PVRT image header\n", filename );

	if (!PVR_ValidSize( header->width, header->height ))
		Error( "\"%s\" is %dx%d; PVR textures must be powers of two from 8 to 1024\n",
			filename, header->width, header->height );

	if (header->pixelFormat != PVR_RGB565 && header->pixelFormat != PVR_ARGB1555 &&
		header->pixelFormat != PVR_ARGB4444)
		Error( "\"%s\" is in pixel format %d; only 1555, 565 and 4444 can be uploaded\n",
			filename, header->pixelFormat );

	switch (header->dataFormat)
	{
	case PVR_RECT:
	case PVR_TWIDDLE:
	case PVR_TWIDDLED_MIPMAP:
	case PVR_VQ:
	case PVR_VQ_MIPMAP:
		break;
	default:
		Error( "\"%s\" is in data format %d; only rectangle, twiddled and VQ can be uploaded\n",
			filename, header->dataFormat );
	}

	expected = PVR_HEADER_BYTES + header->dataSize - 8;
	if (expected > length)
		Error( "\"%s\" claims %d bytes of pixels but the file is only %d bytes\n",
			filename, header->dataSize - 8, length - (int)PVR_HEADER_BYTES );

	if (psize)
		*psize = expected;
	if (pwidth)
		*pwidth = header->width;
	if (pheight)
		*pheight = header->height;

	return header;
}

/*
================
PVR_WriteHeader
================
*/
static void PVR_WriteHeader( pvrheader_t *header, int width, int height, int dataFormat, int payload )
{
	memset( header, 0, sizeof( *header ) );
	memcpy( header->gbixMagic, "GBIX", 4 );
	header->gbixLength = 8;
	header->globalIndex[0] = 0;
	header->globalIndex[1] = 0x20202020;	// the rest of the index is left blank
	memcpy( header->pvrtMagic, "PVRT", 4 );
	header->dataSize = payload + 8;
	header->pixelFormat = PVR_RGB565;
	header->dataFormat = (unsigned char)dataFormat;
	header->width = (short)width;
	header->height = (short)height;
}

/*
================
PVR_ExpandPalette

Turns the 8 bit image into loose colour so the mip chain can be averaged.
================
*/
static byte *PVR_ExpandPalette( const byte *pixels, const byte *palette, int width, int height )
{
	byte		*colour;
	const byte	*entry;
	int			i;

	colour = malloc( width * height * 3 );
	for (i = 0; i < width * height; i++)
	{
		entry = palette + pixels[i] * 3;
		colour[i * 3 + 0] = entry[0];
		colour[i * 3 + 1] = entry[1];
		colour[i * 3 + 2] = entry[2];
	}

	return colour;
}

/*
================
PVR_HalveImage

Boxes four texels down into one for the next level of the chain.
================
*/
static byte *PVR_HalveImage( const byte *colour, int width, int height )
{
	byte	*out;
	int		half, x, y, c;
	int		total;

	half = width / 2;
	if (half < 1)
		half = 1;

	out = malloc( half * half * 3 );
	for (y = 0; y < half; y++)
	{
		for (x = 0; x < half; x++)
		{
			for (c = 0; c < 3; c++)
			{
				total  = colour[((y * 2 + 0) * width + x * 2 + 0) * 3 + c];
				total += colour[((y * 2 + 0) * width + x * 2 + 1) * 3 + c];
				total += colour[((y * 2 + 1) * width + x * 2 + 0) * 3 + c];
				total += colour[((y * 2 + 1) * width + x * 2 + 1) * 3 + c];
				out[(y * half + x) * 3 + c] = (byte)(total / 4);
			}
		}
	}

	return out;
}

/*
================
PVR_GatherBlocks

Unpacks a level into one vector per two by two texel block.  A level smaller
than that is spread across the whole block so it still quantizes sensibly.
================
*/
static int PVR_GatherBlocks( const byte *colour, int size, vqblock_t *blocks )
{
	int		x, y, texel, c;
	int		count;

	if (size < 2)
	{
		for (texel = 0; texel < VQ_BLOCK_TEXELS; texel++)
		{
			for (c = 0; c < 3; c++)
				blocks[0].value[texel * 3 + c] = (float)colour[c];
		}
		return 1;
	}

	count = 0;
	for (y = 0; y < size; y += 2)
	{
		for (x = 0; x < size; x += 2)
		{
			for (texel = 0; texel < VQ_BLOCK_TEXELS; texel++)
			{
				// the hardware stores a block down its columns
				const byte *entry = colour + ((y + (texel & 1)) * size + x + (texel >> 1)) * 3;
				for (c = 0; c < 3; c++)
					blocks[count].value[texel * 3 + c] = (float)entry[c];
			}
			count++;
		}
	}

	return count;
}

/*
================
PVR_ClosestEntry
================
*/
static int PVR_ClosestEntry( const vqblock_t *block, const vqblock_t *book, int numentries )
{
	float	best, distance, delta;
	int		bestentry;
	int		i, j;

	best = 0.0f;
	bestentry = 0;

	for (i = 0; i < numentries; i++)
	{
		distance = 0.0f;
		for (j = 0; j < VQ_BLOCK_VALUES; j++)
		{
			delta = block->value[j] - book[i].value[j];
			distance += delta * delta;
		}

		if (i == 0 || distance < best)
		{
			best = distance;
			bestentry = i;
			if (distance == 0.0f)
				break;
		}
	}

	return bestentry;
}

/*
================
PVR_BuildCodebook

Grows a codebook by repeatedly splitting every entry in two and letting the
blocks settle around the new centres.  Every level of a mipmapped texture
shares the one codebook, so they all go in together.
================
*/
static void PVR_BuildCodebook( const vqblock_t *blocks, int numblocks, vqblock_t *book )
{
	vqblock_t	*totals;
	int			*counts;
	int			numentries;
	int			closest;
	int			i, j, pass;

	totals = malloc( VQ_CODEBOOK_ENTRIES * sizeof( vqblock_t ) );
	counts = malloc( VQ_CODEBOOK_ENTRIES * sizeof( int ) );

	// start with a single entry sitting on the average block
	memset( book, 0, VQ_CODEBOOK_ENTRIES * sizeof( vqblock_t ) );
	for (i = 0; i < numblocks; i++)
	{
		for (j = 0; j < VQ_BLOCK_VALUES; j++)
			book[0].value[j] += blocks[i].value[j];
	}
	for (j = 0; j < VQ_BLOCK_VALUES; j++)
		book[0].value[j] /= numblocks;
	numentries = 1;

	while (numentries < VQ_CODEBOOK_ENTRIES)
	{
		// split every entry, nudging the copy off to one side
		for (i = 0; i < numentries && numentries + i < VQ_CODEBOOK_ENTRIES; i++)
		{
			for (j = 0; j < VQ_BLOCK_VALUES; j++)
				book[numentries + i].value[j] = book[i].value[j] + 2.0f;
		}
		numentries += i;

		for (pass = 0; pass < VQ_REFINE_PASSES; pass++)
		{
			memset( totals, 0, numentries * sizeof( vqblock_t ) );
			memset( counts, 0, numentries * sizeof( int ) );

			for (i = 0; i < numblocks; i++)
			{
				closest = PVR_ClosestEntry( &blocks[i], book, numentries );
				for (j = 0; j < VQ_BLOCK_VALUES; j++)
					totals[closest].value[j] += blocks[i].value[j];
				counts[closest]++;
			}

			for (i = 0; i < numentries; i++)
			{
				if (!counts[i])
					continue;
				for (j = 0; j < VQ_BLOCK_VALUES; j++)
					book[i].value[j] = totals[i].value[j] / counts[i];
			}
		}
	}

	free( totals );
	free( counts );
}

/*
================
PVR_EncodeRect

Plain 16 bit pixels, one scanline after another.
================
*/
static void *PVR_EncodeRect( const byte *pixels, const byte *palette, int width, int height, int *psize )
{
	pvrheader_t		*header;
	unsigned short	*out;
	const byte		*entry;
	int				payload, total;
	int				i;

	total = PVR_EncodedSize( width, height, PVR_NO_MIPMAPS );
	payload = total - PVR_HEADER_BYTES;
	header = malloc( total );
	memset( header, 0, total );
	PVR_WriteHeader( header, width, height, PVR_RECT, payload );

	out = (unsigned short *)(header + 1);
	for (i = 0; i < width * height; i++)
	{
		entry = palette + pixels[i] * 3;
		out[i] = PVR_PackRGB565( entry[0], entry[1], entry[2] );
	}

	*psize = total;
	return header;
}

/*
================
PVR_EncodeVQ
================
*/
static void *PVR_EncodeVQ( const byte *pixels, const byte *palette, int width, int height,
						   int mipmaps, int *psize )
{
	pvrheader_t		*header;
	vqblock_t		*blocks;
	vqblock_t		*book;
	byte			*levels[MAX_MIP_LEVELS];
	int				levelsize[MAX_MIP_LEVELS];
	int				levelblocks[MAX_MIP_LEVELS];
	int				levelfirst[MAX_MIP_LEVELS];
	unsigned short	*codebook;
	byte			*out;
	int				numlevels, numblocks;
	int				payload, total, size;
	int				i, x, y, texel;

	// build the chain of levels this image needs, smallest last
	levels[0] = PVR_ExpandPalette( pixels, palette, width, height );
	levelsize[0] = width;
	numlevels = 1;

	if (mipmaps)
	{
		for (size = width / 2; size >= 1; size = size / 2)
		{
			levels[numlevels] = PVR_HalveImage( levels[numlevels - 1], levelsize[numlevels - 1],
				levelsize[numlevels - 1] );
			levelsize[numlevels] = size;
			numlevels++;
		}
	}

	// every level quantizes against the one shared codebook
	numblocks = 0;
	for (i = 0; i < numlevels; i++)
	{
		int count = (levelsize[i] / 2) * (levelsize[i] / 2);
		if (count < 1)
			count = 1;
		levelblocks[i] = count;
		levelfirst[i] = numblocks;
		numblocks += count;
	}

	blocks = malloc( numblocks * sizeof( vqblock_t ) );
	for (i = 0; i < numlevels; i++)
		PVR_GatherBlocks( levels[i], levelsize[i], blocks + levelfirst[i] );

	book = malloc( VQ_CODEBOOK_ENTRIES * sizeof( vqblock_t ) );
	PVR_BuildCodebook( blocks, numblocks, book );

	total = PVR_EncodedSize( width, height, mipmaps );
	payload = total - PVR_HEADER_BYTES;

	header = malloc( total );
	memset( header, 0, total );
	PVR_WriteHeader( header, width, height, mipmaps ? PVR_VQ_MIPMAP : PVR_VQ, payload );

	codebook = (unsigned short *)(header + 1);
	for (i = 0; i < VQ_CODEBOOK_ENTRIES; i++)
	{
		for (texel = 0; texel < VQ_BLOCK_TEXELS; texel++)
		{
			codebook[i * VQ_BLOCK_TEXELS + texel] = PVR_PackRGB565(
				(int)(book[i].value[texel * 3 + 0] + 0.5f),
				(int)(book[i].value[texel * 3 + 1] + 0.5f),
				(int)(book[i].value[texel * 3 + 2] + 0.5f) );
		}
	}

	out = (byte *)(codebook + VQ_CODEBOOK_ENTRIES * VQ_BLOCK_TEXELS);
	if (mipmaps)
	{
		// the chain is stored smallest first, so walk the levels backwards
		for (i = numlevels - 1; i >= 0; i--)
		{
			int levelwidth = levelsize[i] / 2;

			if (levelwidth < 1)
				out[0] = (byte)PVR_ClosestEntry( &blocks[levelfirst[i]], book, VQ_CODEBOOK_ENTRIES );
			else
			{
				for (y = 0; y < levelwidth; y++)
				{
					for (x = 0; x < levelwidth; x++)
						out[PVR_TwiddleIndex( x, y )] = (byte)PVR_ClosestEntry(
							&blocks[levelfirst[i] + y * levelwidth + x], book, VQ_CODEBOOK_ENTRIES );
				}
			}

			out += levelblocks[i];
		}
	}
	else
	{
		int levelwidth = width / 2;

		for (y = 0; y < levelwidth; y++)
		{
			for (x = 0; x < levelwidth; x++)
				out[PVR_TwiddleIndex( x, y )] = (byte)PVR_ClosestEntry(
					&blocks[y * levelwidth + x], book, VQ_CODEBOOK_ENTRIES );
		}
	}

	for (i = 0; i < numlevels; i++)
		free( levels[i] );
	free( blocks );
	free( book );

	*psize = total;
	return header;
}

/*
================
PVR_EncodeIndexed
================
*/
void *PVR_EncodeIndexed( const byte *pixels, const byte *palette, int width, int height,
						 int mipmaps, int *psize )
{
	if (!PVR_ValidSize( width, height ))
		Error( "can't make a %dx%d PVR texture; sizes must be powers of two from 8 to 1024\n",
			width, height );

	if (PVR_UseVQ( width, height ))
		return PVR_EncodeVQ( pixels, palette, width, height, mipmaps, psize );

	return PVR_EncodeRect( pixels, palette, width, height, psize );
}

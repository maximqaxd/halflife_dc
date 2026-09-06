// afile.cpp -- resources parked in audio RAM

#include "quakedef.h"
#include "audio.h"
#include "afile.h"

static afile_t	*afiles[MAX_AFILES];
static int	afile_serial;

static WAVEFORMATEX	afile_format;
static DSBUFFERDESC	afile_bufferdesc;

/*
==================
AFile_BlockData

The sample memory a static buffer was handed by the driver. Blocks are filled
and read back directly instead of locking around every transfer.
==================
*/
static byte *AFile_BlockData (LPDIRECTSOUNDBUFFER block)
{
	return (byte *)(*(dsbufmem_t **)((byte *)block + DSBUF_MEMOFS))->data;
}

/*
==================
AFile_CopyWords

Longword copy. Both ends live in audio RAM, which does not take kindly to byte
sized accesses, so the tail is written as one more whole word.
==================
*/
void AFile_CopyWords (void *dest, void *src, int count)
{
	int		*out, *in;
	int		words, rem;

	out = (int *)dest;
	in = (int *)src;

	words = count / 4;
	rem = count % 4;

	while (words--)
		*out++ = *in++;

	if (rem)
		*out = *in;
}

/*
==================
AFile_Init
==================
*/
void AFile_Init (void)
{
	memset (afiles, 0, sizeof(afiles));
}

/*
==================
AFile_FreeSoundRam
==================
*/
int AFile_FreeSoundRam (void)
{
	DSCAPS	caps;
	HRESULT	hr;

	memset (&caps, 0, sizeof(caps));

	hr = lpDS->GetCaps (&caps);
	if (hr != DS_OK)
		S_DSoundError (hr);

	return caps.dwFreeHwMemBytes;
}

/*
==================
AFile_HasRoomFor
==================
*/
qboolean AFile_HasRoomFor (int size)
{
	DSCAPS	caps;
	HRESULT	hr;

	memset (&caps, 0, sizeof(caps));

	hr = lpDS->GetCaps (&caps);
	if (hr != DS_OK)
		S_DSoundError (hr);

	return size <= (int)caps.dwFreeHwMemBytes - AFILE_RESERVE;
}

/*
==================
AFile_TotalCachedBytes
==================
*/
int AFile_TotalCachedBytes (void)
{
	int	i, total;

	total = 0;
	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (afiles[i])
			total += afiles[i]->size;
	}

	return total;
}

/*
==================
AFile_FindByName
==================
*/
afile_t *AFile_FindByName (char *name)
{
	int	i;

	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (!afiles[i])
			continue;
		if (!Q_stricmp (afiles[i]->name, name))
			return afiles[i];
	}

	return NULL;
}

/*
==================
Cmd_afilelist_f
==================
*/
void Cmd_afilelist_f (void)
{
	int	i;

	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (afiles[i])
			Con_Printf ("%d: %s %d\n", i, afiles[i]->name, afiles[i]->size);
	}
}

/*
==================
AFile_PrintList
==================
*/
void AFile_PrintList (void* fileid)
{
	int	i;

	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (afiles[i])
			Sys_FPrintf (fileid, "Afile %d: %s %d\n", i, afiles[i]->name, afiles[i]->size);
	}
}

/*
==================
AFile_LoadOrCreate

Park size bytes of data in audio RAM under name. If the resource is already
resident it is returned untouched.
==================
*/
afile_t *AFile_LoadOrCreate (char *name, byte *data, int size, int usage)
{
	afile_t		*af;
	int		i, block;
	int		blocksize;
	void		*ptr1, *ptr2;
	DWORD		bytes1, bytes2;
	HRESULT		hr;

	af = AFile_FindByName (name);
	if (af)
		return NULL;			// already in the afile system

	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (!afiles[i])
			break;
	}

	if (afiles[i] || i >= MAX_AFILES)
		return NULL;			// out of AFile slots

	af = (afile_t *)MnemoAlloc (sizeof(afile_t), MNEMO_FLAG_MALLOC, 0, "AFile record");
	if (!af)
		return NULL;

	memset (af, 0, sizeof(afile_t));
	afiles[i] = af;

	af->id = afile_serial++;
	af->usage = usage;
	strcpy (af->name, name);

	block = 0;
	while (af->size < size)
	{
		blocksize = size - af->size;
		if (blocksize > AFILE_BLOCKSIZE)
			blocksize = AFILE_BLOCKSIZE;

		// audio RAM is handed out in whole words
		if (blocksize & 1)
			blocksize++;
		if (blocksize & 2)
			blocksize += 2;

		memset (&afile_format, 0, sizeof(afile_format));
		afile_format.wFormatTag = WAVE_FORMAT_PCM;
		afile_format.nChannels = 1;
		afile_format.nSamplesPerSec = AFILE_RATE;
		afile_format.nAvgBytesPerSec = AFILE_RATE;
		afile_format.nBlockAlign = 1;
		afile_format.wBitsPerSample = 8;
		afile_format.cbSize = 0;

		memset (&afile_bufferdesc, 0, sizeof(afile_bufferdesc));
		afile_bufferdesc.dwSize = sizeof(DSBUFFERDESC);
		afile_bufferdesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_LOCHARDWARE;
		afile_bufferdesc.dwBufferBytes = blocksize;
		afile_bufferdesc.lpwfxFormat = &afile_format;

		hr = lpDS->CreateSoundBuffer (&afile_bufferdesc, &af->blocks[block], NULL);
		if (hr != DS_OK)
		{
			S_DSoundError (hr);
			AFile_Free (af);
			return NULL;
		}

		ptr1 = ptr2 = NULL;
		bytes1 = bytes2 = 0;

		hr = af->blocks[block]->Lock (0, 0, &ptr1, &bytes1, &ptr2, &bytes2, DSBLOCK_ENTIREBUFFER);
		if (hr != DS_OK)
		{
			S_DSoundError (hr);
			AFile_Free (af);
			return NULL;
		}

		hr = af->blocks[block]->Unlock (ptr1, bytes1, ptr2, bytes2);
		if (hr != DS_OK)
		{
			S_DSoundError (hr);
			AFile_Free (af);
			return NULL;
		}

		if (ptr1 && bytes1)
		{
			AFile_CopyWords (AFile_BlockData (af->blocks[block]), data, bytes1);
			af->size += bytes1;
			data += bytes1;
		}

		block++;
	}

	return af;
}

/*
==================
AFile_ReadBlocks
==================
*/
int AFile_ReadBlocks (afile_t *af, byte *dest, int size)
{
	int	i, count;
	int	remaining;

	if (!af)
		return 0;

	remaining = size;
	for (i = 0 ; i < AFILE_BLOCKS ; i++)
	{
		if (!remaining || !af->blocks[i])
			continue;

		count = remaining;
		if (count > AFILE_BLOCKSIZE)
			count = AFILE_BLOCKSIZE;

		if (count)
		{
			AFile_CopyWords (dest, AFile_BlockData (af->blocks[i]), count);
			dest += count;
			remaining -= count;
		}
	}

	return size - remaining;
}

/*
==================
AFile_ReadBlocksOffset

Same, but starting offset bytes into the resource.
==================
*/
int AFile_ReadBlocksOffset (afile_t *af, byte *dest, int offset, int size)
{
	int	i, count;
	int	remaining;

	if (!af)
		return 0;

	// walk forward to the block holding offset
	i = 0;
	while (offset > AFILE_BLOCKSIZE)
	{
		offset -= AFILE_BLOCKSIZE;
		i++;
	}

	remaining = size;
	while (remaining && i < AFILE_BLOCKS)
	{
		count = AFILE_BLOCKSIZE - offset;
		if (count > remaining)
			count = remaining;

		if (count)
		{
			AFile_CopyWords (dest, AFile_BlockData (af->blocks[i]) + offset, count);
			dest += count;
			remaining -= count;
		}

		offset = 0;
		i++;
	}

	return size - remaining;
}

/*
==================
AFile_GetSize
==================
*/
int AFile_GetSize (afile_t *af)
{
	if (!af)
		return 0;

	return af->size;
}

/*
==================
AFile_Free
==================
*/
void AFile_Free (afile_t *af)
{
	int	i;
	HRESULT	hr;

	for (i = 0 ; i < MAX_AFILES ; i++)
	{
		if (afiles[i] == af)
			afiles[i] = NULL;
	}

	for (i = 0 ; i < AFILE_BLOCKS ; i++)
	{
		if (!af->blocks[i])
			continue;

		hr = af->blocks[i]->Release ();
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	MnemoFree (af);
}

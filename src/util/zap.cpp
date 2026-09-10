// Zap.cpp -- Bfile in-memory buffer files (+ Zap/zlib compression)

#include "quakedef.h"

#include <string.h>
#include "kzap.h"

// Bfile handle / table entry (bfile_t) is declared in kzap.h. Game files are
// unpacked (and decompressed) from the disc image into RAM Bfile buffers; writable
// ones (saves) compress back into RAM, which is why Bwrite exists even though the
// disc itself is read-only.
#define MAX_BFILES 128

static bfile_t g_bfiles[MAX_BFILES];

#define BFILE_MIN_GROW 2048
#define BFILE_FETCH_CHUNK 16384

static int  g_bBfileAlloc;      // set while a Bfile owns a Mnemo allocation
static int  g_bFetchActive;     // set while a disc fetch is in flight
static char g_bfileName[64];    // scratch for the Mnemo tag

// Wildcard search state shared by Bfind_first/Bfind_next.
static char g_findPattern[256]; // pattern, normalized once by Bfind_first
static char g_findName[256];    // basename of the slot the search is sitting on
static int  g_findSlot;         // that slot, -1 once the search has been reset

// Build a Mnemo allocation tag from the file's basename.
char *Bmakename( char *path, unsigned int type )
{
	char *p;

	p = strrchr(path, '\\');
	if (!p)
		p = strrchr(path, '/');
	if (p)
		path = p + 1;

	switch (type)
	{
	case 0:
		sprintf(g_bfileName, "|%s|", path);
		break;
	case 1:
		sprintf(g_bfileName, "}%s{", path);
		break;
	case 2:
		sprintf(g_bfileName, "(%s)", path);
		break;
	case 3:
		sprintf(g_bfileName, ">%s<", path);
		break;
	case ' ':
		sprintf(g_bfileName, "%s", path);
		break;
	default:
		sprintf(g_bfileName, "%c:%s", type, path);
		break;
	}
	return g_bfileName;
}

// Compare two paths ignoring case, treating '\' and '/' as the same.
int Bpathcmp( const char *s1, const char *s2 )
{
	int c1, c2;

	while (*s1 && *s2)
	{
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
		if (c1 == '\\')
			c1 = '/';
		if (c2 == '\\')
			c2 = '/';
		if (c1 != c2)
			return c1 - c2;
	}
	return *s1 - *s2;
}

void Bnormalize_path( char *dest, const char *src )
{
	while (*src)
	{
		char c = *src++;
		*dest++ = c == '\\' ? '/' : (char)tolower(c);
	}
	*dest = '\0';
}

extern "C" void MnemoShrink( void *ptr, int newsize );

// Compress a Bfile's buffer in place (save files, on Bclose).
static int Bcompress( bfile_t *h )
{
	byte *buf;
	int   alloc, csize;
	float t0;

	t0 = Sys_FloatTime();
	alloc = (int)((float)h->size * 1.0625f) + 1;
	if (alloc < 0x40)
		alloc = 0x40;

	buf = (byte *)MnemoAlloc(alloc + 4, MNEMO_FLAG_MALLOC, 0, Bmakename(h->path, 1));
	if (!buf)
		return 0;

	csize = ZlibCompress(h->data, buf, h->size);
	//Con_DPrintf
	("Compressed a Bfile", Sys_FloatTime() - t0);
	MnemoFree(h->data);
	h->data = buf;
	MnemoShrink(buf, csize);
	h->capacity = csize;
	h->flags |= 1;
	return h->flags;
}

// Grow a writable Bfile's buffer by at least BFILE_MIN_GROW bytes.
static int Bgrow( bfile_t *h, int grow )
{
	void *newbuf;
	int   saved;

	if (grow < BFILE_MIN_GROW)
		grow = BFILE_MIN_GROW;

	saved = g_bBfileAlloc;
	g_bBfileAlloc = 1;
	newbuf = MnemoAlloc(h->capacity + grow, MNEMO_FLAG_MALLOC, 0, Bmakename(h->path, 0));
	g_bBfileAlloc = saved;

	if (newbuf)
	{
		memcpy(newbuf, h->data, h->capacity);
		MnemoFree(h->data);
		h->data = (byte *)newbuf;
		h->capacity += grow;
	}
	return newbuf != 0;
}

extern "C" {

int IsBfile( bfile_t *h )
{
	int i;

	if (h < g_bfiles || &g_bfiles[MAX_BFILES - 1] < h)
		return 0;

	i = 0;
	do
	{
		if (h == &g_bfiles[i])
			return g_bfiles[i].open;
		i++;
	} while (i < MAX_BFILES);
	return 0;
}

extern void Sys_ErrorColor( int wColor, char *fmt, ... );

int Bdecompress( bfile_t *h )
{
	byte *buffer = (byte *)MnemoAlloc(h->size, MNEMO_FLAG_MALLOC, 0, Bmakename(h->path, 0));

	if (buffer)
	{
		ZlibDecompress(h->data, buffer);
		MnemoFree(h->data);
		h->data = buffer;
	}
	else
	{
		Sys_ErrorColor(RGB565_RED, "Insufficient memory to decompress a compressed memory file. This is BAD.\n");
	}
	return buffer != NULL;
}

bfile_t *Bopen( char *path, char *mode )
{
	bfile_t *e;
	int    i, saved;

	// Reopen an existing entry whose path matches, decompressing it if needed.
	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used == 0 || g_bfiles[i].open != 0)
			continue;
		if (Bpathcmp(g_bfiles[i].path, path) != 0)
			continue;

		e = &g_bfiles[i];
		strcpy(e->mode, mode);
		if (e->flags & 1)
		{
			if (!Bdecompress(e))
				return 0;
#if HLDC_FIXES
			// The buffer that was just handed out is size bytes long, so say so;
			// capacity still described the compressed copy that was thrown away.
			e->capacity = e->size;
#endif
		}
#if HLDC_FIXES
		// Opening for writing starts the file over. Without this the length only
		// ever grows: a save that writes fewer bytes than the last one leaves the
		// file at its old length, and everything past the new data - stale records,
		// or uninitialised buffer once the file has been through a compress and a
		// decompress - is carried into the save bundle and out to the memory card.
		// That is what makes a saved game cost several times the blocks it should,
		// and what makes the cost drift with whatever the engine last did.
		if (strchr(mode, 'w'))
			e->size = 0;
#endif
		e->position = 0;
		e->open = 1;
		return e;
	}

	// No match. A read of a missing file fails; a write creates it.
	if (strchr(mode, 'r'))
		return 0;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used != 0)
			continue;

		e = &g_bfiles[i];
		Bnormalize_path(e->path, path);
		strcpy(e->mode, mode);

		saved = g_bBfileAlloc;
		g_bBfileAlloc = 1;
		e->data = (byte *)MnemoAlloc(2048, MNEMO_FLAG_MALLOC, 0, Bmakename(e->path, 0));
		g_bBfileAlloc = saved;
		if (e->data != 0)
		{
			e->used = 1;
			e->capacity = 2048;
			e->size = 0;
			e->position = 0;
			e->open = 1;
			return e;
		}
		return 0;
	}
	return 0;
}

int Bclose( bfile_t *h )
{
	bfile_t *e = g_bfiles;
	int i = 0;

	while (e != h || e->used == 0)
	{
		i++;
		e++;
		if (i >= MAX_BFILES)
			return -1;
	}

	g_bfiles[i].open = 0;
	g_bfiles[i].position = 0;
	if (g_bfiles[i].flags & 1)
	{
		g_bfiles[i].flags &= ~1;
		Bcompress(&g_bfiles[i]);
	}
	return 0;
}

int Bsize( bfile_t *h )
{
	return h->size;
}

int Bread( void *buffer, int size, int count, bfile_t *h )
{
	int nread = 0;
	int pos, remaining;

	while (1)
	{
		if (count == 0)
			return nread;
		count--;
		pos = h->position;
		remaining = h->size - pos;
		if (remaining < size)
			break;
		memcpy(buffer, h->data + pos, size);
		h->position += size;
		buffer = (char *)buffer + size;
		nread++;
	}

	if (remaining == 0)
		return nread;

	memcpy(buffer, h->data + pos, remaining);
	h->position += remaining;
	return nread;
}

int Bwrite( void *buffer, int size, int count, bfile_t *h )
{
	int nwritten = 0;
	int pos, grow;

	if (strchr(h->mode, 'w') || strchr(h->mode, '+'))
	{
		while (count != 0)
		{
			count--;
			pos = h->position;
			grow = size - (h->capacity - pos);
			if (grow > 0)
			{
				if (!Bgrow(h, grow))
					return nwritten;
				pos = h->position;
			}
			memcpy(h->data + pos, buffer, size);
			pos = h->position + size;
			h->position = pos;
			buffer = (char *)buffer + size;
			if (h->size < pos)
			{
				h->size = pos;
				pos = h->position;
			}
			if (h->capacity < pos)
				Sys_Error("Bfile write overflow!\n");
			nwritten++;
		}
	}
	return nwritten;
}

int Bseek( void *hFile, int offset, int whence )
{
	bfile_t *h = (bfile_t *)hFile;
	int pos;

	if (whence == 1)
	{
		pos = h->position + offset;
		h->position = pos;
		if (pos < 0)
		{
			pos = 0;
			h->position = 0;
		}
		if (h->size < pos)
			h->position = h->size;
		return 0;
	}
	if (whence == 0)
	{
		h->position = offset;
		if (offset < 0)
		{
			offset = 0;
			h->position = 0;
		}
		if (h->size < offset)
			h->position = h->size;
		return 0;
	}
	if (whence == 2)
	{
		pos = h->size + offset;
		h->position = pos;
		if (pos < 0)
		{
			pos = 0;
			h->position = 0;
		}
		if (h->size < pos)
			h->position = h->size;
		return 0;
	}
	return -1;
}

int Btell( bfile_t *h )
{
	return h->position;
}

int Beof( bfile_t *h )
{
	if (h->position >= h->size)
		return -1;
	return 0;
}

// Zero one Bfile slot's bookkeeping fields.
void Bclear_slot( int slot )
{
	bfile_t *e = &g_bfiles[slot];

	e->data     = NULL;
	e->capacity = 0;
	e->position = 0;
	e->size     = 0;
	e->path[0]  = '\0';
	e->mode[0]  = '\0';
	e->used     = 0;
	e->open     = 0;
	e->flags    = 0;
}

static void (*volatile Bclear_slot_call)(int) = Bclear_slot;

void Binit_slots( void )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
		Bclear_slot_call(i);
}

void Bfree_all( void )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].data)
			MnemoFree(g_bfiles[i].data);
		Bclear_slot_call(i);
	}
}

// Shrink every open Bfile's buffer down to its current size.
void Bshrink_all( void )
{
	bfile_t *e;
	byte    *buf;
	int      i, keep;

	if (g_bBfileAlloc)
		return;

	g_bBfileAlloc = 1;

	for (i = 0; i < MAX_BFILES; i++)
	{
		e = &g_bfiles[i];
		if (!e->used)
			continue;

		keep = e->capacity;
		if (e->size <= e->capacity)
			keep = e->size;

		// The replacement must not be reclaimed out from under the copy
		buf = (byte *)MnemoAlloc(keep, MNEMO_FLAG_MALLOC | MNEMO_FLAG_NO_RECLAIM, 0,
			Bmakename(e->path, 0));
		if (!buf)
			return;

		memcpy(buf, e->data, keep);
		MnemoFree(e->data);
		e->data = buf;
		e->capacity = keep;
	}

	g_bBfileAlloc = 0;
}

// Remove the in-memory Bfile with the given path.
int Bremove_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (Bpathcmp(path, g_bfiles[i].path) != 0)
			continue;

		if (g_bfiles[i].data)
			MnemoFree(g_bfiles[i].data);

		Bclear_slot(i);
		return 0;
	}

	return -1;
}

// Return the size of the in-memory Bfile with the given path.
int Bfilesize_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used)
			continue;

		if (Bpathcmp(path, g_bfiles[i].path) == 0)
			return g_bfiles[i].size;
	}

	return 0;
}

int Bstoredsize_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used && !g_bfiles[i].open && !Bpathcmp(g_bfiles[i].path, path))
		{
			if (g_bfiles[i].flags | 1)
				return g_bfiles[i].capacity;
			return g_bfiles[i].size;
		}
	}
	return 0;
}

// Grow a Bfile's buffer so it can hold at least the requested size.
void Bensure_capacity( bfile_t *h, int needed )
{
	needed -= h->capacity;
	if (needed > 0)
		Bgrow(h, needed);
}

// Fetch a file from the disc image into an in-memory Bfile.
int Bfetch_disc( char *path )
{
	char      block[BFILE_FETCH_CHUNK];
	void     *pFile;
	bfile_t  *h;
	int       i, resident, size, chunk, grow, ok;

	ok = 1;

	pFile = Sys_OpenHandle(path, "rb");
	if (!pFile)
		return 0;

	// The file table is searched ahead of the disc, so a file that is already
	// resident comes back as one of our own entries and needs no fetching
	resident = 0;
	if ((char *)pFile >= (char *)g_bfiles &&
		(char *)pFile <= (char *)g_bfiles + sizeof(g_bfiles))
	{
		for (i = 0; i < MAX_BFILES; i++)
		{
			if (pFile == &g_bfiles[i])
			{
				resident = g_bfiles[i].open;
				break;
			}
		}
	}

	if (resident)
		return 1;

	size = DC_fsize(pFile);

	h = (bfile_t *)Bopen(path, "wb");
	if (!h)
		return 0;

	grow = size - h->capacity;
	if (grow > 0)
		Bgrow(h, grow);

	while (size)
	{
		chunk = (size < BFILE_FETCH_CHUNK) ? size : BFILE_FETCH_CHUNK;

		if (DC_fread(block, chunk, 1, pFile) != 1)
		{
			ok = 0;
			break;
		}

		if (Bwrite(block, chunk, 1, h) != 1)
		{
			ok = 0;
			break;
		}

		size -= chunk;
	}

	Bclose(h);
	Sys_CloseHandle(pFile);

	// A short read leaves a truncated file behind, so drop it again
	if (!ok)
	{
		for (i = 0; i < MAX_BFILES; i++)
		{
			if (Bpathcmp(path, g_bfiles[i].path) != 0)
				continue;

			if (g_bfiles[i].data)
				MnemoFree(g_bfiles[i].data);

			Bclear_slot(i);
			break;
		}
	}

	g_bFetchActive = 0;
	return ok;
}

// Match a name against a wildcard pattern ('*' and '?').
int Bwild_match( char *pattern, char *text )
{
	for (; *pattern; pattern++)
	{
		if (*pattern == '*')
		{
			if (pattern[1] == '\0')
				return 1;

			// Try every tail the star could stand for
			while (*text)
			{
				if (Bwild_match(pattern + 1, text))
					return 1;
				text++;
			}

			return Bwild_match(pattern + 1, text);
		}

		if (*pattern == '?')
		{
			if (*text == '\0')
				return 0;
		}
		else if (*text != *pattern)
		{
			return 0;
		}

		text++;
	}

	return (*text == '\0');
}

// Begin a wildcard search over the open Bfiles; returns the first basename.
// Begin a wildcard search over the open Bfiles; returns the first basename.
char *Bfind_first( char *pattern, char *nameOut )
{
	char *p, *q;

	if (nameOut)
		*nameOut = '\0';

	Bnormalize_path(g_findPattern, pattern);

	g_findSlot = 0;

	while (!Bwild_match(g_findPattern, g_bfiles[g_findSlot].path))
	{
		g_findSlot++;
		if (g_findSlot >= MAX_BFILES)
			return NULL;
	}

	{
		q = strrchr(g_bfiles[g_findSlot].path, '/');
		if (q)
			q = q + 1;
		else
			q = g_bfiles[g_findSlot].path;
		p = g_findName;
		for (; *q != '\0'; q++)
			*p++ = (*q == '\\') ? '/' : (char)tolower(*q);
		*p = '\0';

		if (nameOut)
			COM_FileBase(g_findName, nameOut);

		return g_findName;
	}
}

// Continue a wildcard search started by Bfind_first.
char *Bfind_next( char *nameOut )
{
	char *p, *q;

	if (nameOut)
		*nameOut = '\0';

	g_findSlot++;

	while (g_findSlot < MAX_BFILES)
	{
		if (Bwild_match(g_findPattern, g_bfiles[g_findSlot].path))
		{
			q = strrchr(g_bfiles[g_findSlot].path, '/');
			if (q)
				q = q + 1;
			else
			q = g_bfiles[g_findSlot].path;
			p = g_findName;
			for (; *q != '\0'; q++)
				*p++ = (*q == '\\') ? '/' : (char)tolower(*q);
			*p = '\0';
			if (nameOut)
				COM_FileBase(g_findName, nameOut);
			return g_findName;
		}

		g_findSlot++;
	}

	return NULL;
}

// Reset the wildcard search cursor.
void Bfind_reset( void )
{
	g_findSlot = -1;
}

// Rename an in-memory Bfile, failing if the destination already exists.
int Brename_path( char *oldpath, char *newpath )
{
	char  normalized[256];
	int   i;

	// Nothing may be renamed on top of a file that is already resident
	Bnormalize_path(normalized, newpath);

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used)
			continue;

		if (Bpathcmp(normalized, g_bfiles[i].path) == 0)
			return -1;
	}

	Bnormalize_path(normalized, oldpath);

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used)
			continue;

		if (Bpathcmp(normalized, g_bfiles[i].path) != 0)
			continue;

		Bnormalize_path(g_bfiles[i].path, newpath);
		return 0;
	}

	return -1;
}

// Compress the in-memory Bfile with the given path in place.
int Bcompress_path( char *path )
{
	int i, result;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used || g_bfiles[i].open)
			continue;

		if (Bpathcmp(path, g_bfiles[i].path) != 0)
			continue;

		if (g_bfiles[i].flags & 1)
			return 1;

		result = Bcompress(&g_bfiles[i]);
		CL_SetProgressName("Compressed a Bfile");
		return result;
	}

	return 0;
}

int Bexport_named_handle( bfile_t *h, char *path )
{
	char name[256];
	char *base = strrchr(path, '/');
	void *file;

	if (base)
		path = base + 1;
	sprintf(name, "\\PC\\%s", path);
	file = Sys_OpenHandle(name, "wb");
	if (file)
	{
		DC_fwrite(h->data, h->size, 1, file);
		Sys_CloseHandle(file);
	}
	return file != NULL;
}

// Write a Bfile's contents out to the PC-side host under \PC\.
int Bexport_handle( bfile_t *h )
{
	return Bexport_named_handle(h, h->path);
}

// Write the Bfile with the given path out to the PC-side host.
int Bexport_path( char *path, char *exportName )
{
	char *p;
	char name[256];
	void *pFile;
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used || g_bfiles[i].open)
			continue;

		if (Bpathcmp(path, g_bfiles[i].path) == 0)
		{
			p = strrchr(exportName, '/');
			if (p)
				p++;
			else
				p = exportName;

			sprintf(name, "\\PC\\%s", p);
			pFile = Sys_OpenHandle(name, "wb");
			if (!pFile)
				return 0;

			DC_fwrite(g_bfiles[i].data, g_bfiles[i].size, 1, pFile);
			Sys_CloseHandle(pFile);
			return 1;
		}
	}

	return 0;
}

int Bexport_default_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used && !g_bfiles[i].open && !Bpathcmp(g_bfiles[i].path, path))
			return Bexport_named_handle(&g_bfiles[i], path);
	}
	return 0;
}

// Return a pointer to a Bfile's data if it is uncompressed, else NULL.
void *Bfileptr_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (!g_bfiles[i].used || g_bfiles[i].open)
			continue;

		if (Bpathcmp(path, g_bfiles[i].path) == 0 && !(g_bfiles[i].flags & 1))
			return g_bfiles[i].data;
	}

	return NULL;
}

void *Bcompressedptr_path( char *path )
{
	int i;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used && !g_bfiles[i].open &&
			!Bpathcmp(g_bfiles[i].path, path) && (g_bfiles[i].flags & 1))
			return g_bfiles[i].data;
	}
	return NULL;
}

int Bwrite_compressed( char *path, void *data, int size )
{
	bfile_t *file = Bopen(path, "wb");

	if (file)
	{
		Bwrite(data, size, 1, file);
		Bclose(file);
		file->flags |= 1;
	}
	return file != NULL;
}

} // extern "C"

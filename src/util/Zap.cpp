// Zap.cpp -- Bfile in-memory buffer files (+ Zap/zlib compression)

#include "quakedef.h"

#include <string.h>
#include "kzap.h"

// Bfile handle / table entry. Game files are unpacked (and decompressed) from the
// disc image into RAM Bfile buffers; writable ones (saves) compress back into RAM,
// which is why Bwrite exists even though the disc itself is read-only.
typedef struct bfile_s
{
	byte         *data;        // in-memory file data
	int           capacity;    // allocated capacity
	int           position;    // current read/write position
	int           size;        // current file size
	char          path[256];   // normalized path
	char          mode[8];     // open mode string
	int           used;        // slot in use
	int           open;        // currently open
	unsigned int  flags;       // bit 0 = compressed
} bfile_t;

#define MAX_BFILES 128

static bfile_t g_bfiles[MAX_BFILES];

#define BFILE_MIN_GROW 2048

static int  g_bBfileAlloc;      // set while a Bfile owns a Mnemo allocation
static char g_bfileName[64];    // scratch for the Mnemo tag

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
static int Bpathcmp( const char *s1, const char *s2 )
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

int Bopen( char *path, char *mode )
{
	bfile_t *e;
	byte  *buf;
	char  *p, *q;
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
			buf = (byte *)MnemoAlloc(e->size, MNEMO_FLAG_MALLOC, 0, Bmakename(e->path, 0));
			if (buf == 0)
			{
				Sys_ErrorColor(RGB565_RED, "Insufficient memory to decompress a compressed memory file. This is BAD.\n");
				return 0;
			}
			ZlibDecompress(e->data, buf);
			MnemoFree(e->data);
			e->data = buf;
		}
		e->position = 0;
		e->open = 1;
		return (int)e;
	}

	// No match. A read of a missing file fails; a write creates it.
	if (strchr(mode, 'r'))
		return 0;

	for (i = 0; i < MAX_BFILES; i++)
	{
		if (g_bfiles[i].used != 0)
			continue;

		e = &g_bfiles[i];
		p = e->path;
		for (q = path; *q != '\0'; q++)
			*p++ = (*q == '\\') ? '/' : (char)tolower(*q);
		*p = '\0';
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
			return (int)e;
		}
		return 0;
	}
	return 0;
}

int Bclose( bfile_t *h )
{
	bfile_t *e = g_bfiles;
	int i = 0;

	while (e != h || e->open == 0)
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
void Bclear_slot( bfile_t *h )
{
}

// Shrink every open Bfile's buffer down to its current size.
void Bshrink_all( void )
{
}

// Remove the in-memory Bfile with the given path.
int Bremove_path( char *path )
{
	return 0;
}

// Return the size of the in-memory Bfile with the given path.
int Bfilesize_path( char *path )
{
	return 0;
}

// Grow a Bfile's buffer so it can hold at least the requested size.
int Bensure_capacity( bfile_t *h, int needed )
{
	return 0;
}

// Fetch a file from the disc image into an in-memory Bfile.
int Bfetch_disc( char *path )
{
	return 0;
}

// Match a name against a wildcard pattern ('*' and '?').
int Bwild_match( char *pattern, char *text )
{
	return 0;
}

// Begin a wildcard search over the open Bfiles; returns the first basename.
char *Bfind_first( char *pattern, char *nameOut )
{
	return NULL;
}

// Continue a wildcard search started by Bfind_first.
char *Bfind_next( char *nameOut )
{
	return NULL;
}

// Reset the wildcard search cursor.
void Bfind_reset( void )
{
}

// Rename an in-memory Bfile, failing if the destination already exists.
int Brename_path( char *oldpath, char *newpath )
{
	return 0;
}

// Compress the in-memory Bfile with the given path in place.
int Bcompress_path( char *path )
{
	return 0;
}

// Write a Bfile's contents out to the PC-side host under \PC\.
int Bexport_handle( bfile_t *h )
{
	return 0;
}

// Write the Bfile with the given path out to the PC-side host.
int Bexport_path( char *path )
{
	return 0;
}

// Return a pointer to a Bfile's data if it is uncompressed, else NULL.
void *Bfileptr_path( char *path )
{
	return NULL;
}

} // extern "C"

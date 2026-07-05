// Zap.cpp -- Bfile in-memory buffer files (+ Zap/zlib compression)

#include "quakedef.h"

#include <string.h>

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
static char *Bmakename( char *path, unsigned int type )
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

/*
=============================================================================

KZAP COMPRESSION

The opcode's top 3 bits select one of eight codecs; the low 5 bits are the
codec's argument. Decompression dispatches per opcode; compression asks every
codec for the cost of encoding at the current position and takes the cheapest,
falling back to plain "dump" literals.

=============================================================================
*/

#define ZAP_OPMASK 0xE0

static float g_zapNoFit = 10.0f;   // cost that never beats a plain literal

extern "C" void Sys_SetTaskName( char *name );

class KZapCodec
{
public:
	int         code;       // opcode high bits
	int         mask;       // opcode mask
	int         maxarg;     // largest argument a single op can carry
	const char *name;

	KZapCodec( int c, int m, int mx, const char *n )
	{
		code = c;
		mask = m;
		maxarg = mx;
		name = n;
	}
	virtual ~KZapCodec() {}
	virtual float Measure( byte *start, byte *cur, int remaining ) { return 1e30f; }
	virtual int   Encode( byte **src, byte **dst, int count )      { return 0; }
	virtual int   Decode( byte **src, byte **dst )                 { return 0; }
	virtual int   Finish( byte **dst )                             { return 0; }
};

// Plain literal run; a zero-length dump terminates the stream.
class KZapDumpCodec : public KZapCodec
{
public:
	KZapDumpCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		return ((float)remaining + 1.0f) / (float)remaining;
	}
	virtual int Encode( byte **src, byte **dst, int count )
	{
		int i, chunk, k;

		for (i = count; i > 0; i -= chunk)
		{
			chunk = i;
			if (chunk > 0x1f)
				chunk = 0x1f;
			*(*dst)++ = (byte)(code | chunk);
			for (k = 0; k < chunk; k++)
				*(*dst)++ = *(*src)++;
		}
		return count;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		int n = *(*src)++ & 0x1f;

		if (n == 0)
			return 0;
		do
		{
			*(*dst)++ = *(*src)++;
		} while (--n != 0);
		return 1;
	}
	virtual int Finish( byte **dst )
	{
		*(*dst)++ = (byte)code;
		return 1;
	}
};

// Run of one byte value.
class KZapRunCodec : public KZapCodec
{
public:
	int  count;
	char value;

	KZapRunCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		int n = 0;

		value = *cur;
		if (*cur == value && remaining > 0)
		{
			for (;;)
			{
				cur++;
				n++;
				if (*cur != value)
					break;
				if (n > 0x1e || n >= remaining)
					break;
			}
		}
		if (n == 0)
			return g_zapNoFit;
		count = n;
		return 2.0f / (float)n;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | count);
		*(*dst)++ = **src;
		*src += count;
		return count;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		count = *(*src)++ & 0x1f;
		value = *(*src)++;
		while (count-- != 0)
			*(*dst)++ = value;
		return 1;
	}
};

// Back-reference copy from earlier output.
class KZapStringCodec : public KZapCodec
{
public:
	int count;
	int matchoff;

	KZapStringCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		byte *scan;
		int   len, best;
		byte  c = *cur;
		float cost = g_zapNoFit;

		if (cur - start < 2)
			return g_zapNoFit;
		if (remaining < 2)
			return g_zapNoFit;

		best = 1;
		if (maxarg < cur - start)
			start = cur - maxarg;
		scan = cur;
		while (start < scan)
		{
			scan--;
			if (*scan == c)
			{
				len = 1;
				if (scan[1] == cur[1])
				{
					do
					{
						len++;
						if (len >= remaining || len > 0x1e || cur <= scan + len)
							break;
					} while (scan[len] == cur[len]);
				}
				if (best < len)
				{
					count = len;
					matchoff = cur - scan;
					cost = 2.0f / (float)len;
					best = len;
				}
			}
		}
		return cost;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | count);
		*(*dst)++ = (byte)matchoff;
		*src += count;
		return count;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		byte *from;

		count = *(*src)++ & 0x1f;
		from = *dst - *(*src)++;
		while (count-- != 0)
			*(*dst)++ = *from++;
		return 1;
	}
};

// Small long value, encoded in one to three bytes.
class KZapTinyCodec : public KZapCodec
{
public:
	int nbytes;
	int b1;
	int b2;
	int value;

	KZapTinyCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		if (remaining < 4)
			return g_zapNoFit;
		nbytes = 1;
		value = *(unsigned int *)cur;
		if ((unsigned int)value > 0x801d)
			return g_zapNoFit;
		value += 1;
		if ((unsigned int)value > 0x1f)
		{
			value -= 0x1f;
			if ((unsigned int)value < 0x80)
			{
				nbytes = 2;
				b1 = value;
				b2 = 0;
			}
			else
			{
				nbytes = 3;
				if ((unsigned int)value > 0x7fff)
				{
					Sys_Error("We were supposed to catch this earlier!\n");
					return g_zapNoFit;
				}
				b1 = (value & 0x7f) | 0x80;
				b2 = (value >> 7) & 0xff;
			}
		}
		return (float)nbytes / 4.0f;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		if (nbytes == 1)
		{
			*(*dst)++ = (byte)(code | value);
		}
		else if (nbytes == 2)
		{
			*(*dst)++ = (byte)code;
			*(*dst)++ = (byte)b1;
		}
		else if (nbytes == 3)
		{
			*(*dst)++ = (byte)code;
			*(*dst)++ = (byte)b1;
			*(*dst)++ = (byte)b2;
		}
		*src += 4;
		return 4;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		byte *p;

		value = *(*src)++ & 0x1f;
		if (value == 0)
		{
			value = *(*src)++;
			if (value & 0x80)
			{
				value &= 0x7f;
				value |= *(*src)++ << 7;
			}
			value += 0x1f;
		}
		value -= 1;

		p = (byte *)&value;
		(*dst)[0] = p[0];
		(*dst)[1] = p[1];
		(*dst)[2] = p[2];
		(*dst)[3] = p[3];
		*dst += 4;
		return 1;
	}
};

// Single byte copied from the recent output window.
class KZapCopyCodec : public KZapCodec
{
public:
	int dist;
	int value;

	KZapCopyCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		byte  b = *cur;
		byte *w = cur - 0x20;

		if (w < start)
			w = start;
		for (;;)
		{
			if (cur <= w)
				return g_zapNoFit;
			if (*w == b)
				break;
			w++;
		}
		dist = cur - 1 - w;
		value = b;
		return 1.0f;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | dist);
		*src += 1;
		return 1;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		int op = *(*src)++;

		**dst = (*dst)[-((op & 0x1f) + 1)];
		*dst += 1;
		return 1;
	}
};

// Repeat the last output byte.
class KZapRepeatCodec : public KZapCodec
{
public:
	int count;

	KZapRepeatCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		byte *prev;
		int   n = 0;

		if (cur == start)
			return g_zapNoFit;
		prev = cur - 1;
		if (*cur == *prev && remaining > 0)
		{
			for (;;)
			{
				cur++;
				n++;
				if (*cur != *prev)
					break;
				if (n > 0x1e || n >= remaining)
					break;
			}
		}
		if (n == 0)
			return g_zapNoFit;
		count = n;
		return 1.0f / (float)n;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | count);
		*src += count;
		return count;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		byte value = (*dst)[-1];
		int  n = *(*src)++ & 0x1f;

		count = n - 1;
		if (n != 0)
		{
			do
			{
				*(*dst)++ = value;
			} while (count-- != 0);
		}
		return 1;
	}
};

// Previous long with one shifted byte flipped.
class KZapModLongCodec : public KZapCodec
{
public:
	int modbyte;
	int shift;

	KZapModLongCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		unsigned int prev, val, mask;
		int s;

		if (remaining < 4)
			return g_zapNoFit;
		if (cur - start < 4)
			return g_zapNoFit;

		prev = ((unsigned int *)cur)[-1];
		val = *(unsigned int *)cur;
		for (s = 0; s < 0x18; s++)
		{
			mask = 0xff << s;
			if ((prev & ~mask) == (val & ~mask))
			{
				modbyte = ((prev & mask) ^ (val & mask)) >> s;
				shift = s;
				return 0.5f;
			}
		}
		return g_zapNoFit;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | shift);
		*(*dst)++ = (byte)modbyte;
		*src += 4;
		return 4;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		unsigned int prev = *(unsigned int *)(*dst - 4);
		unsigned int val;

		shift = *(*src)++ & 0x1f;
		modbyte = *(*src)++;
		val = prev ^ (modbyte << shift);
		*(*dst)++ = (byte)val;
		*(*dst)++ = (byte)(val >> 8);
		*(*dst)++ = (byte)(val >> 16);
		*(*dst)++ = (byte)(val >> 24);
		return 1;
	}
};

// Two bytes copied from the recent output window.
class KZapCopyWordCodec : public KZapCodec
{
public:
	int dist;
	int b1;
	int b2;

	KZapCopyWordCodec( int c, int m, int mx, const char *n ) : KZapCodec(c, m, mx, n) {}
	virtual float Measure( byte *start, byte *cur, int remaining )
	{
		byte  c1, c2;
		byte *w;

		if (remaining < 2)
			return g_zapNoFit;
		c1 = cur[0];
		c2 = cur[1];
		w = cur - 0x20;
		if (w < start)
			w = start;
		while (w < cur - 1)
		{
			if (w[0] == c1 && w[1] == c2)
			{
				dist = cur - 1 - w;
				b1 = c1;
				b2 = c2;
				return 0.5f;
			}
			w++;
		}
		return g_zapNoFit;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | dist);
		*src += 2;
		return 2;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		byte *from = *dst - ((*(*src)++ & 0x1f) + 1);

		*(*dst)++ = from[0];
		*(*dst)++ = from[1];
		return 1;
	}
};

static KZapCodec *g_zapCodecs[8];
static int        g_bZapReady;
static short      g_zapChunks[9];
static int        g_zapInBytes[9];
static int        g_zapOutBytes[9];

void KZapInitCodecs( int maxarg )
{
	if (maxarg == 0)
		maxarg = 0xff;

	g_zapCodecs[0] = new KZapDumpCodec(0x00, ZAP_OPMASK, maxarg, "dump");
	g_zapCodecs[1] = new KZapRunCodec(0x20, ZAP_OPMASK, maxarg, "run");
	g_zapCodecs[2] = new KZapStringCodec(0x40, ZAP_OPMASK, maxarg, "string");
	g_zapCodecs[3] = new KZapTinyCodec(0x60, ZAP_OPMASK, maxarg, "tiny");
	g_zapCodecs[4] = new KZapCopyCodec(0x80, ZAP_OPMASK, maxarg, "copy");
	g_zapCodecs[5] = new KZapRepeatCodec(0xA0, ZAP_OPMASK, maxarg, "repeat");
	g_zapCodecs[6] = new KZapModLongCodec(0xC0, ZAP_OPMASK, maxarg, "mod-long");
	g_zapCodecs[7] = new KZapCopyWordCodec(0xE0, ZAP_OPMASK, maxarg, "copy-word");
	g_bZapReady = 1;
}

static void KZapReportStats( void )
{
	int i;

	for (i = 0; i < 8; i++)
	{
		//Con_Printf("%9s: %5d chunks, %6d in, %6d out\n",
		//	g_zapCodecs[i]->name, g_zapChunks[i], g_zapInBytes[i], g_zapOutBytes[i]);
	}
}

int KZapCompress( byte *src, byte *dst, int len )
{
	byte *cur, *out, *dumpStart, *prevOut;
	int   i, k, n, pending, watch, best;
	int   origLen = len;
	float cost, bestCost;

	if (!g_bZapReady)
		KZapInitCodecs(0);

	for (i = 0; i < 9; i++)
	{
		g_zapChunks[i] = 0;
		g_zapInBytes[i] = 0;
		g_zapOutBytes[i] = 0;
	}

	cur = src;
	out = dst;
	pending = 0;
	watch = 0;
	best = 0;

	while (len > 0)
	{
		bestCost = g_zapCodecs[0]->Measure(src, cur, len);
		best = 0;
		for (k = 1; k < 8; k++)
		{
			KZapCodec *codec = g_zapCodecs[k];
			if (codec && (cost = codec->Measure(src, cur, len)) < bestCost)
			{
				bestCost = cost;
				best = k;
			}
		}

		if (best == 0)
		{
			// nothing beats a literal; extend the pending dump run
			if (pending == 0)
				dumpStart = cur;
			cur++;
			pending++;
			len--;
		}
		else
		{
			if (pending != 0)
			{
				g_zapCodecs[0]->Encode(&dumpStart, &out, pending);
				g_zapChunks[0]++;
				g_zapInBytes[0] += pending;
				g_zapOutBytes[0] += pending + 1;
				pending = 0;
			}
			prevOut = out;
			n = g_zapCodecs[best]->Encode(&cur, &out, pending);
			len -= n;
			g_zapChunks[best]++;
			g_zapInBytes[best] += n;
			g_zapOutBytes[best] += out - prevOut;
		}

		if (watch++ % 100 == 0)
			Sys_SetTaskName("KZapCompress");
	}

	if (len < 0)
		Sys_Error("Remaining length underflow in KZapCompress, last codec: %s\n", g_zapCodecs[best]->name);

	if (pending != 0)
	{
		g_zapCodecs[0]->Encode(&dumpStart, &out, pending);
		g_zapChunks[0]++;
		g_zapInBytes[0] += pending;
		g_zapOutBytes[0] += pending + 1;
	}

	if (cur - src != origLen)
		Sys_Error("Whoops!\n");

	for (i = 0; i < 8; i++)
	{
		if (g_zapCodecs[i]->Finish(&out))
			break;
	}
	KZapReportStats();
	return out - dst;
}

int KZapDecompress( byte *src, byte *dst )
{
	byte *s = src;
	byte *d = dst;
	int   i, cont;

	if (!g_bZapReady)
		KZapInitCodecs(0);

	i = 0;
	do
	{
		KZapCodec *codec = g_zapCodecs[(*s & ZAP_OPMASK) >> 5];
		if (codec == 0)
		{
			Sys_Error("Sucker was here a second ago...\n");
			cont = 0;
		}
		else
		{
			cont = codec->Decode(&s, &d);
		}
		if (i % 100 == 0)
			Sys_SetTaskName("KZapDecompress");
		i++;
	} while (cont != 0);

	return d - dst;
}

extern "C" int ZlibCompress( byte *src, byte *dst, int len )
{
	return KZapCompress(src, dst, len);
}

extern "C" int ZlibDecompress( byte *src, byte *dst )
{
	return KZapDecompress(src, dst);
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
				Sys_ErrorColor(0xf800, "Insufficient memory to decompress a compressed memory file. This is BAD.\n");
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

} // extern "C"

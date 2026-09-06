// kzap.cpp -- KZap stream compression. A table of tiny opcode codecs (dump, tiny,
// repeat, run, string, copy) each measures and encodes the next run of bytes, and
// the compressor picks the cheapest one at every step. Used to pack save buffers.

#include "quakedef.h"
#include <string.h>
#include "kzap.h"

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
		value = (unsigned int)cur[0] | ((unsigned int)cur[1] << 8) |
			((unsigned int)cur[2] << 16) | ((unsigned int)cur[3] << 24);
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

		prev = (unsigned int)cur[-4] | ((unsigned int)cur[-3] << 8) |
			((unsigned int)cur[-2] << 16) | ((unsigned int)cur[-1] << 24);
		val = (unsigned int)cur[0] | ((unsigned int)cur[1] << 8) |
			((unsigned int)cur[2] << 16) | ((unsigned int)cur[3] << 24);
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
		byte *previous = *dst - 4;
		unsigned int prev = (unsigned int)previous[0] | ((unsigned int)previous[1] << 8) |
			((unsigned int)previous[2] << 16) | ((unsigned int)previous[3] << 24);
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

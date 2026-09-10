// kzap.cpp -- KZap stream compression. A table of tiny opcode codecs (dump, tiny,
// repeat, run, string, copy) each measures and encodes the next run of bytes, and
// the compressor picks the cheapest one at every step. Used to pack save buffers.

#include "quakedef.h"
#include <string.h>
#include "kzap.h"
extern "C" {
#include "pr_cmds.h"
}

#define ZAP_OPMASK 0xE0

static float g_zapNoFit = 10.0f;   // cost that never beats a plain literal

extern "C" void CL_SetProgressName( char *name );

unsigned short g_zapPending;
unsigned short g_zapBestOp;
unsigned short g_zapBestCount;
unsigned short g_zapBestValue;
float g_zapBestCost;
float g_zapTrialCost;
int g_zapWindow;

unsigned short KZapDecodeCompactHeader( byte *src, unsigned short *value )
{
	if (*src >> 3)
	{
		*value = *src >> 3;
		return 1;
	}
	if (src[1] & 0x80)
	{
		*value = (src[1] & 0x7f) + src[2] * 0x80 + 0x9e;
		return 3;
	}
	*value = src[1] + 0x1f;
	return 2;
}

unsigned short KZapDecodeWideHeader( byte *src, unsigned short *value )
{
	if (*src >> 3)
	{
		*value = *src >> 3;
		return 1;
	}
	if (src[1] == 0)
	{
		*value = src[2] + src[3] * 0x100;
		return 4;
	}
	*value = src[1] + 0x1f;
	return 2;
}

unsigned short KZapEncodeWideHeader( byte *dst, unsigned short op, unsigned short value, int direct )
{
	if (direct)
	{
		*dst = (byte)((value << 3) + op);
		return 1;
	}
	if (!value)
		return 0xffff;
	if (value < 32)
	{
		*dst = (byte)((value << 3) + op);
		return 1;
	}
	if (value >= 0x11f)
	{
		dst[0] = (byte)op;
		dst[1] = 0;
		dst[2] = (byte)value;
		dst[3] = (byte)(value >> 8);
		return 4;
	}
	dst[0] = (byte)op;
	dst[1] = (byte)(value - 31);
	return 2;
}

unsigned short KZapEncodeCompactHeader( byte *dst, unsigned short op, unsigned short value )
{
	if (!value || value > 0x809d)
		return 0xffff;
	if (value < 32)
	{
		*dst = (byte)((value << 3) + op);
		return 1;
	}
	if (value >= 0x9e)
	{
		value -= 0x9e;
		dst[0] = (byte)op;
		dst[1] = (byte)((value & 0x7f) | 0x80);
		dst[2] = (byte)(value >> 7);
		return 3;
	}
	dst[0] = (byte)op;
	dst[1] = (byte)(value - 31);
	return 2;
}

unsigned short KZapFlushLiteral( byte *src, byte *dst )
{
	unsigned short size;

	if (!g_zapPending)
		return 0;
	size = KZapEncodeWideHeader(dst, 0, g_zapPending, 0);
	memcpy(dst + size, src, g_zapPending);
	size += g_zapPending;
	g_zapPending = 0;
	return size;
}

void KZapMeasureRun( byte *src, int remaining )
{
	byte *end = src;
	int count;

	do
	{
		end++;
		if (end == src + remaining)
			break;
	} while (*end == *src);
	count = (byte)(end - src);
	if (count < 2)
		return;
	g_zapTrialCost = 2.0f / (float)count;
	if (g_zapTrialCost < g_zapBestCost)
	{
		g_zapBestCost = g_zapTrialCost;
		g_zapBestOp = 1;
		g_zapBestCount = count;
		g_zapBestValue = *src;
	}
}

unsigned short KZapEncodeRun( byte *dst )
{
	unsigned short size = KZapEncodeWideHeader(dst, g_zapBestOp, g_zapBestCount, 0);
	dst[size] = (byte)g_zapBestValue;
	return size + 1;
}

void KZapMeasureTiny( unsigned int *src, unsigned int remaining )
{
	unsigned int value;
	float cost;

	if (remaining > 3 && (value = *src + 1) != 0 && value <= 0x809d)
	{
		if (value < 32)
			cost = 1.0f;
		else if (value > 0x9e)
			cost = 3.0f;
		else
			cost = 2.0f;
		cost /= 4.0f;
		g_zapTrialCost = cost;
		if (cost < g_zapBestCost)
		{
			g_zapBestCost = cost;
			g_zapBestCount = 4;
			g_zapBestValue = (unsigned short)value;
			g_zapBestOp = 4;
		}
	}
}

unsigned short KZapEncodeTiny( byte *dst )
{
	return KZapEncodeCompactHeader(dst, g_zapBestOp, g_zapBestValue);
}

void KZapMeasureString( byte *start, byte *cur, unsigned int remaining )
{
	unsigned short best = 1;
	unsigned short count;
	byte *scan;
	int distance = 0;

	if (cur - start <= 1 || remaining <= 1)
		return;
	if (g_zapWindow < cur - start)
		start = cur - g_zapWindow;
	scan = cur;
	while (start < scan)
	{
		scan--;
		distance++;
		if (*scan == *cur)
		{
			count = 1;
			if (scan[1] == cur[1])
			{
				do
				{
					count++;
					if (count >= remaining || cur <= scan + count)
						break;
				} while (scan[count] == cur[count]);
			}
			if (best < count)
			{
				g_zapTrialCost = (distance > 256 ? 3.0f : 2.0f) / (float)count;
				best = count;
				if (g_zapTrialCost < g_zapBestCost)
				{
					g_zapBestCost = g_zapTrialCost;
					g_zapBestCount = count;
					g_zapBestValue = (unsigned short)distance;
					g_zapBestOp = g_zapBestValue < 256 ? 2 : 3;
				}
			}
		}
	}
}

unsigned short KZapEncodeString( byte *dst )
{
	unsigned short size = KZapEncodeWideHeader(dst, g_zapBestOp, g_zapBestCount, 0);

	if (g_zapBestOp == 2)
	{
		dst[size] = (byte)g_zapBestValue;
		return size + 1;
	}
	dst[size] = (byte)g_zapBestValue;
	dst[size + 1] = (byte)(g_zapBestValue >> 8);
	return size + 2;
}

void KZapMeasureColor( byte *start, byte *cur, unsigned int remaining )
{
	unsigned short value, previous;
	int r, g, b;

	if (cur - start < 2 || remaining < 2)
		return;
	value = cur[0] + cur[1] * 256;
	previous = cur[-2] + cur[-1] * 256;
	r = (value & 31) - (previous & 31);
	g = ((value >> 5) & 31) - ((previous >> 5) & 31);
	b = (value >> 10) - (previous >> 10);
	if (g_zapBestCost > 0.5f && r > -2 && r < 2 && g > -2 && g < 2 && b > -2 && b < 2)
	{
		g_zapBestCost = 0.5f;
		g_zapBestOp = 5;
		g_zapBestValue = (unsigned short)(r + g * 3 + b * 9 + 13);
	}
}

void KZapMeasureCopy( byte *start, byte *cur )
{
	byte *scan;

	if (!g_zapPending && cur != start)
	{
		scan = cur;
		do
		{
			scan--;
			if (scan == start || cur - scan > 31)
				break;
		} while (*scan != *cur);
		if (*scan == *cur)
		{
			g_zapTrialCost = 1.0f;
			if (g_zapBestCost >= 1.0f)
			{
				g_zapBestCost = 1.0f;
				g_zapBestValue = (unsigned short)(cur - scan - 1);
				g_zapBestOp = 7;
			}
		}
	}
}

unsigned short KZapEncodeColor( byte *dst )
{
	KZapEncodeWideHeader(dst, g_zapBestOp, g_zapBestValue, 1);
	return 1;
}

unsigned short KZapEncodeCopy( byte *dst )
{
	KZapEncodeWideHeader(dst, g_zapBestOp, g_zapBestValue, 1);
	return 1;
}

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
	virtual float Measure( byte *start, byte *cur, int remaining, int pending ) { return g_zapNoFit; }
	virtual int   Encode( byte **src, byte **dst, int count )      { return 0; }
	virtual int   Decode( byte **src, byte **dst )                 { return 0; }
	virtual int   Finish( byte **dst )                             { return 0; }
};

// Plain literal run; a zero-length dump terminates the stream.
class KZapDumpCodec : public KZapCodec
{
public:
	KZapDumpCodec( int c, int m, int mx, const char *n );
	virtual ~KZapDumpCodec();
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
	{
		float count = (float)(remaining + pending);
		return (count + 1.0f) / count;
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
	byte value;

	KZapRunCodec( int c, int m, int mx, const char *n );
	virtual ~KZapRunCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapStringCodec( int c, int m, int mx, const char *n );
	virtual ~KZapStringCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapTinyCodec( int c, int m, int mx, const char *n );
	virtual ~KZapTinyCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapCopyCodec( int c, int m, int mx, const char *n );
	virtual ~KZapCopyCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapRepeatCodec( int c, int m, int mx, const char *n );
	virtual ~KZapRepeatCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapModLongCodec( int c, int m, int mx, const char *n );
	virtual ~KZapModLongCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

	KZapCopyWordCodec( int c, int m, int mx, const char *n );
	virtual ~KZapCopyWordCodec();
	virtual int Finish( byte **dst ) { return 0; }
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
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

KZapDumpCodec::KZapDumpCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapDumpCodec::~KZapDumpCodec()
{
}

KZapRunCodec::KZapRunCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapRunCodec::~KZapRunCodec()
{
}

KZapStringCodec::KZapStringCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapStringCodec::~KZapStringCodec()
{
}

KZapTinyCodec::KZapTinyCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapTinyCodec::~KZapTinyCodec()
{
}

KZapCopyCodec::KZapCopyCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapCopyCodec::~KZapCopyCodec()
{
}

KZapRepeatCodec::KZapRepeatCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapRepeatCodec::~KZapRepeatCodec()
{
}

KZapModLongCodec::KZapModLongCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapModLongCodec::~KZapModLongCodec()
{
}

KZapCopyWordCodec::KZapCopyWordCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapCopyWordCodec::~KZapCopyWordCodec()
{
}

// Runs of two- or four-byte values.
class KZapMultiRunCodec : public KZapCodec
{
public:
	int mode;
	int words;
	int longs;
	byte word[2];
	byte value[4];

	KZapMultiRunCodec( int c, int m, int mx, const char *n );
	virtual ~KZapMultiRunCodec();
	virtual float Measure( byte *start, byte *cur, int remaining, int pending );
	virtual int Encode( byte **src, byte **dst, int pending );
	virtual int Decode( byte **src, byte **dst );
	virtual int Finish( byte **dst ) { return 0; }
};

KZapMultiRunCodec::KZapMultiRunCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapMultiRunCodec::~KZapMultiRunCodec()
{
}

float KZapMultiRunCodec::Measure( byte *start, byte *cur, int remaining, int pending )
{
	float wordCost = g_zapNoFit;
	float longCost = g_zapNoFit;
	int limit, i;

	mode = 0;
	if (remaining < 2)
		return g_zapNoFit;
	limit = remaining / 2;
	if (limit > 15)
		limit = 15;
	word[0] = cur[0];
	word[1] = cur[1];
	for (i = 0; i < limit; i++)
	{
		if (cur[i * 2] != word[0] || cur[i * 2 + 1] != word[1])
			break;
	}
	if (i > 1)
	{
		words = i;
		wordCost = 3.0f / (float)(i * 2);
	}
	if (remaining < 4)
		return wordCost;
	limit = remaining / 4;
	if (limit > 15)
		limit = 15;
	value[0] = cur[0];
	value[1] = cur[1];
	value[2] = cur[2];
	value[3] = cur[3];
	for (i = 0; i < limit; i++)
	{
		if (cur[i * 4] != word[0] || cur[i * 4 + 1] != word[1] ||
			cur[i * 4 + 2] != value[0] || cur[i * 4 + 3] != value[1])
			break;
	}
	if (i > 1)
	{
		longs = i;
		longCost = 5.0f / (float)(i * 4);
	}
	if (wordCost <= longCost)
		return wordCost;
	mode = 1;
	return longCost;
}

int KZapMultiRunCodec::Encode( byte **src, byte **dst, int pending )
{
	if (mode)
	{
		*(*dst)++ = (byte)(code | 0x10 | longs);
		*(*dst)++ = (*src)[0];
		*(*dst)++ = (*src)[1];
		*(*dst)++ = (*src)[2];
		*(*dst)++ = (*src)[3];
		*src += longs * 4;
		return longs * 4;
	}
	*(*dst)++ = (byte)(code | words);
	*(*dst)++ = (*src)[0];
	*(*dst)++ = (*src)[1];
	*src += words * 2;
	return words * 2;
}

int KZapMultiRunCodec::Decode( byte **src, byte **dst )
{
	int op = *(*src)++;
	unsigned int count = op & 15;

	if (op & 0x10)
	{
		value[0] = *(*src)++;
		value[1] = *(*src)++;
		value[2] = *(*src)++;
		value[3] = *(*src)++;
		while (count--)
		{
			*(*dst)++ = value[0];
			*(*dst)++ = value[1];
			*(*dst)++ = value[2];
			*(*dst)++ = value[3];
		}
	}
	else
	{
		word[0] = *(*src)++;
		word[1] = *(*src)++;
		while (count--)
		{
			*(*dst)++ = word[0];
			*(*dst)++ = word[1];
		}
	}
	return 1;
}

class KZapEndCodec : public KZapCodec
{
public:
	KZapEndCodec( int c, int m, int mx, const char *n );
	virtual ~KZapEndCodec();
	virtual float Measure( byte *start, byte *cur, int remaining, int pending ) { return g_zapNoFit; }
	virtual int Encode( byte **src, byte **dst, int pending ) { return 0; }
	virtual int Decode( byte **src, byte **dst ) { return 0; }
	virtual int Finish( byte **dst )
	{
		*(*dst)++ = (byte)code;
		return 1;
	}
};

KZapEndCodec::KZapEndCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapEndCodec::~KZapEndCodec()
{
}

static byte g_zapDictionary[32] =
{
	'i', 'a', 't', 'n', 'r', '_', 'l', 'c',
	'e', 'o', 'u', 's', 'm', 'p', 'g', 'd',
	'f', 'v', 'b', 'h', 'S', 'C', 'k', 'y',
	1, 'T', '\n', 'L', 'A', 'w', 'q', 14
};

class KZapDictionaryCodec : public KZapCodec
{
public:
	int index;

	KZapDictionaryCodec( int c, int m, int mx, const char *n );
	virtual ~KZapDictionaryCodec();
	virtual float Measure( byte *start, byte *cur, int remaining, int pending )
	{
		for (int i = 0; i < 32; i++)
		{
			if (g_zapDictionary[i] == *cur)
			{
				index = i;
				return 1.0f;
			}
		}
		return g_zapNoFit;
	}
	virtual int Encode( byte **src, byte **dst, int pending )
	{
		*(*dst)++ = (byte)(code | index);
		*src += 1;
		return 1;
	}
	virtual int Decode( byte **src, byte **dst )
	{
		index = *(*src)++ & 31;
		*(*dst)++ = g_zapDictionary[index];
		return 1;
	}
	virtual int Finish( byte **dst ) { return 0; }
};

KZapDictionaryCodec::KZapDictionaryCodec( int c, int m, int mx, const char *n )
	: KZapCodec(c, m, mx, n)
{
}

KZapDictionaryCodec::~KZapDictionaryCodec()
{
}

static KZapCodec *g_zapCodecs[8];
static int        g_bZapReady;
static unsigned short g_zapChunks[9];
static int        g_zapInBytes[9];
static int        g_zapOutBytes[9];

void KZapClearStats( void )
{
	for (int i = 0; i < 9; i++)
	{
		g_zapChunks[i] = 0;
		g_zapInBytes[i] = 0;
		g_zapOutBytes[i] = 0;
	}
}

void KZapLegacyReportStats( void )
{
	int i = 0;
	do
	{
		i++;
	} while (i < 9);
}

void KZapAddStats( int codec, int input, int output )
{
	g_zapChunks[codec]++;
	g_zapInBytes[codec] += input;
	g_zapOutBytes[codec] += output;
}

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

	KZapClearStats();

	cur = src;
	out = dst;
	pending = 0;
	watch = 0;
	best = 0;

	while (len > 0)
	{
		bestCost = g_zapCodecs[0]->Measure(src, cur, len, pending);
		best = 0;
		for (k = 1; k < 8; k++)
		{
			KZapCodec *codec = g_zapCodecs[k];
			if (codec && (cost = codec->Measure(src, cur, len, pending)) < bestCost)
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
				KZapAddStats(0, pending, pending + 1);
				pending = 0;
			}
			prevOut = out;
			n = g_zapCodecs[best]->Encode(&cur, &out, pending);
			len -= n;
			KZapAddStats(best, n, out - prevOut);
		}

		if (watch++ % 100 == 0)
			CL_SetProgressName("KZapCompress");
	}

	if (len < 0)
		Sys_Error("Remaining length underflow in KZapCompress, last codec: %s\n", g_zapCodecs[best]->name);

	if (pending != 0)
	{
		g_zapCodecs[0]->Encode(&dumpStart, &out, pending);
		KZapAddStats(0, pending, pending + 1);
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
			CL_SetProgressName("KZapDecompress");
		i++;
	} while (cont != 0);

	return d - dst;
}

void KZapTestBuffer( int size )
{
	byte *input = (byte *)calloc(1, size * 2);
	byte *packed = (byte *)calloc(1, size * 2);
	byte *output = (byte *)calloc(1, size * 2);
	byte *cur = input;
	int remaining = size;
	int count, i, value;

	while (remaining)
	{
		if (remaining < 0)
			Sys_Error("FEH\n");
		switch (RandomLong(0, 3))
		{
		case 0:
			count = RandomLong(1, 20);
			if (count > remaining)
				count = remaining;
			while (count--)
			{
				*cur++ = (byte)RandomLong(0, 255);
				remaining--;
			}
			break;
		case 1:
			count = RandomLong(1, 20);
			if (count > remaining)
				count = remaining;
			value = RandomLong(0, 255);
			while (count--)
			{
				*cur++ = (byte)value;
				remaining--;
			}
			break;
		case 2:
			count = RandomLong(1, 20);
			if (remaining < count * 2)
				count = remaining / 2;
			for (i = 0; i < count; i++)
			{
				*cur++ = (byte)(count + i);
				remaining--;
			}
			for (i = 0; i < count; i++)
			{
				*cur++ = (byte)(count + i);
				remaining--;
			}
			break;
		case 3:
			if (remaining > 3)
			{
				value = RandomLong(0, 31);
				if (!value)
					value = RandomLong(32, 0x7fff);
				cur[0] = (byte)value;
				cur[1] = (byte)(value >> 8);
				cur[2] = (byte)(value >> 16);
				cur[3] = (byte)(value >> 24);
				cur += 4;
				remaining -= 4;
			}
			break;
		}
	}
	KZapCompress(input, packed, size);
	KZapDecompress(packed, output);
	for (i = 0; i < size; i++)
	{
		if (input[i] != output[i])
			Sys_Error("Compress mismatch\n");
	}
	free(input);
	free(packed);
	free(output);
}

void KZapTest( int count, int size )
{
	for (int i = 0; i < count; i++)
		KZapTestBuffer(size);
}

extern "C" int ZlibCompress( byte *src, byte *dst, int len )
{
	return KZapCompress(src, dst, len);
}

extern "C" int ZlibDecompress( byte *src, byte *dst )
{
	return KZapDecompress(src, dst);
}

// common.c -- misc functions used in client and server

#include "quakedef.h"
#include "winquake.h"
#include "pr_cmds.h"
#include "decal.h"
#include "info.h"
#include "color.h"

#define NUM_SAFE_ARGVS  7

static char* largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char* argvdummy = " ";

static char* safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"};

qboolean com_ignorecolons = FALSE;  // YWB:  Ignore colons as token separators in COM_Parse

cvar_t  registered = { "registered","0" };
cvar_t  cmdline = { "cmdline","0", FCVAR_SERVER };

qboolean        com_modified;   // set true if using non-id files

qboolean		proghack;

int             static_registered = 1;  // only for startup check, then set

int             PROTOCOL_VERSION = PROTOCOL_VERSION_CURRENT;  // active protocol; -protocol can force PROTOCOL_VERSION_OLD

qboolean		msg_suppress_1 = 0;
qboolean		gfExtendedError = FALSE;
char			gszDisconnectReason[256];

void COM_InitFilesystem( void );

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT              339
#define PAK0_CRC                32981

char	com_token[1024];
int		com_argc;
char** com_argv;

#define CMDLINE_LENGTH	256
char	com_cmdline[CMDLINE_LENGTH];

qboolean		standard_quake = TRUE, rogue, hipnotic;

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.

*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink( link_t* l )
{
	l->prev = l->next = l;
}

void RemoveLink( link_t* l )
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore( link_t* l, link_t* before )
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter( link_t* l, link_t* after )
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Q_memset( void* dest, int fill, int count )
{
	int             i;

	if ((((long)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill << 8) | (fill << 16) | (fill << 24);
		for (i = 0; i < count; i++)
			((int*)dest)[i] = fill;
	}
	else
		for (i = 0; i < count; i++)
			((byte*)dest)[i] = fill;
}

void Q_memcpy( void* dest, void* src, int count )
{
	int             i;

	if ((((long)dest | (long)src | count) & 3) == 0)
	{
		count >>= 2;
		for (i = 0; i < count; i++)
			((int*)dest)[i] = ((int*)src)[i];
	}
	else
		for (i = 0; i < count; i++)
			((byte*)dest)[i] = ((byte*)src)[i];
}

int Q_memcmp( void* m1, void* m2, int count )
{
	while (count)
	{
		count--;
		if (((byte*)m1)[count] != ((byte*)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy( char* dest, char* src )
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy( char* dest, char* src, int count )
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen( const char* str )
{
	int             count;
	
	count = 0;
	while (str[count])
		count++;

	return count;
}

char* Q_strrchr( char* s, char c )
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
		if (*--s == c) return s;
	return 0;
}

void Q_strcat( char* dest, char* src )
{
	int count;

	count = 0;
	while (dest[count])
		count++;
	dest += count;

	while (*src)
		*dest++ = *src++;
	*dest++ = 0;
}

int Q_strcmp( char* s1, char* s2 )
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp( char* s1, char* s2, int count )
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncasecmp( const char* s1, const char* s2, int n )
{
	int             c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}

	return -1;
}

int Q_strcasecmp( const char* s1, const char* s2 )
{
	return Q_strncasecmp(s1, s2, 99999);
}

int Q_atoi( char* str )
{
	int		val;
	int		sign;
	int		c;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = val * 16 + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = val * 16 + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = val * 16 + c - 'A' + 10;
			else
				return val * sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return str[1] * sign;
	}

//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c < '0' || c > '9')
			return val * sign;
		val = val * 10 + c - '0';
	}

	return 0;
}


float Q_atof( char* str )
{
	float	val;
	int		sign;
	int		c;
	int		decimal, total;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val * 16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val * 16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val * 16) + c - 'A' + 10;
			else
				return val * sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c < '0' || c > '9')
			break;
		val = val * 10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val * sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val * sign;
}

/*
==============================
Q_FileNameCmp

this is a specific string compare to use for filenames.
 It accepts a forward slash in the first path in place of a backslash
in the second path.
NOTE: this function uses the same return value conventions as strcmp.
so 0 signals a match.
NOTE: HLDC pak has forward slashes internaly 
==============================
*/
int Q_FileNameCmp( char* file1, char* file2 )
{
	do
	{
		if (*file1 == '/' && *file2 == '\\')
			continue;
		if (tolower(*file1) != tolower(*file2))
			return -1;
		if (!*file1)
			return 0;
	} while (*file1++ && *file2++);
	return 0; // fixed bogus warning
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean bigendien;

short	(*BigShort) (short l);
short	(*LittleShort) (short l);
int		(*BigLong) (int l);
int		(*LittleLong) (int l);
float	(*BigFloat) (float l);
float	(*LittleFloat) (float l);

short ShortSwap( short l )
{
	byte    b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

short ShortNoSwap( short l )
{
	return l;
}

int LongSwap( int l )
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

int LongNoSwap( int l )
{
	return l;
}

float FloatSwap( float f )
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap( float f )
{
	return f;
}

/*
============
COM_ExplainDisconnection

Stores an extended reason for a failed connection.
============
*/
void COM_ExplainDisconnection( qboolean bPrint, char* format, ... )
{
	va_list	argptr;
	char	string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	strncpy(gszDisconnectReason, string, 255);
	gszDisconnectReason[255] = 0;
	gfExtendedError = TRUE;

	if (bPrint)
		Con_Printf("%s\n", gszDisconnectReason);
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar( sizebuf_t* sb, int c )
{
	byte* buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error("MSG_WriteChar: range error");
#endif

	buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteByte( sizebuf_t* sb, int c )
{
	byte* buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error("MSG_WriteByte: range error");
#endif
	buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteShort( sizebuf_t* sb, int c )
{
	byte* buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c >(short)0x7fff)
		Sys_Error("MSG_WriteShort: range error");
#endif

	buf = SZ_GetSpace(sb, 2);
	buf[0] = c;
	buf[1] = c >> 8;
}

void MSG_WriteWord( sizebuf_t* sb, int c )
{
	byte* buf;

	buf = SZ_GetSpace(sb, 2);
	buf[0] = c & 0xff;
	buf[1] = (c >> 8) & 0xff;
}

void MSG_WriteLong( sizebuf_t* sb, int c )
{
	byte* buf;

	buf = (byte*)SZ_GetSpace(sb, 4);
	buf[0] = c;
	buf[1] = c >> 8;
	buf[2] = c >> 16;
	buf[3] = c >> 24;
}

void MSG_WriteFloat( sizebuf_t* sb, float f )
{
	union
	{
		float   f;
		int     l;
	} dat;

	byte* buf;

	dat.f = f;
	dat.l = LittleLong(dat.l);

	buf = SZ_GetSpace(sb, 4);
	Q_memcpy(buf, &dat.l, 4);
}

void MSG_WriteString( sizebuf_t* sb, const char* s )
{
	if (!s)
		SZ_Write(sb, "", 1);
	else
		SZ_Write(sb, (void *)s, Q_strlen(s) + 1);
}

void MSG_WriteBuf( sizebuf_t* sb, int iSize, void* buf )
{
	if (!buf)
		return;

	SZ_Write(sb, buf, iSize);
}

void MSG_WriteCoord( sizebuf_t* sb, float f )
{
	byte* buf;
	int c;

	c = (int)(f * 8);
	buf = SZ_GetSpace(sb, 2);
	buf[0] = c;
	buf[1] = c >> 8;
}

void MSG_WriteAngle( sizebuf_t* sb, float f )
{
	byte* buf;
	int c;

	c = (int)(f * 256 / 360);
	buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteHiresAngle( sizebuf_t* sb, float f )
{
	byte* buf;
	int c;

	c = (int)(f * 65536 / 360) & 0xffff;
	buf = SZ_GetSpace(sb, 2);
	buf[0] = c;
	buf[1] = c >> 8;
}

void MSG_WriteDeltaUsercmd( sizebuf_t* buf, usercmd_t* from, usercmd_t* cmd )
{
	int		bits;

//
// send the movement message
//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

	MSG_WriteByte(buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteHiresAngle(buf, from->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteHiresAngle(buf, from->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteAngle(buf, from->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteFloat(buf, from->forwardmove);
	if (bits & CM_SIDE)
		MSG_WriteFloat(buf, from->sidemove);
	if (bits & CM_UP)
		MSG_WriteFloat(buf, from->upmove);

	if (bits & CM_BUTTONS)
		MSG_WriteShort(buf, from->buttons);
	if (bits & CM_IMPULSE)
		MSG_WriteByte(buf, from->impulse);
	MSG_WriteByte(buf, from->lightlevel);
	MSG_WriteByte(buf, from->msec);
}

/*
============
MSG_WriteUsercmdGold

Delta-compress a movement command against a baseline.
Movement values are sent full precision.
============
*/
void MSG_WriteUsercmdGold( sizebuf_t* buf, usercmd_t* cmd, usercmd_t* from )
{
	int		bits;
	int		i;

	bits = 0;
	for (i = 0; i < 3; i++)
	{
		if (cmd->angles[i] != from->angles[i])
			bits |= 1 << i;
	}
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

	MSG_WriteByte(buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteHiresAngle(buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteHiresAngle(buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteAngle(buf, cmd->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteFloat(buf, cmd->forwardmove);
	if (bits & CM_SIDE)
		MSG_WriteFloat(buf, cmd->sidemove);
	if (bits & CM_UP)
		MSG_WriteFloat(buf, cmd->upmove);

	if (bits & CM_BUTTONS)
		MSG_WriteShort(buf, cmd->buttons);
	if (bits & CM_IMPULSE)
		MSG_WriteByte(buf, cmd->impulse);

	MSG_WriteByte(buf, cmd->lightlevel);
	MSG_WriteByte(buf, cmd->msec);
}

/*
============
MSG_WriteUsercmd36

Delta-compress a movement command for the 36 protocol.  Movement values
are sent as scaled shorts rather than full precision.
============
*/
void MSG_WriteUsercmd36( sizebuf_t* buf, usercmd_t* cmd, usercmd_t* from )
{
	int		bits;
	int		i;

	bits = 0;
	for (i = 0; i < 3; i++)
	{
		if (cmd->angles[i] != from->angles[i])
			bits |= 1 << i;
	}
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

	MSG_WriteByte(buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteHiresAngle(buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteHiresAngle(buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteAngle(buf, cmd->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteShort(buf, (int)cmd->forwardmove);
	if (bits & CM_SIDE)
		MSG_WriteShort(buf, (int)cmd->sidemove);
	if (bits & CM_UP)
		MSG_WriteShort(buf, (int)cmd->upmove);

	if (bits & CM_BUTTONS)
		MSG_WriteShort(buf, cmd->buttons);
	if (bits & CM_IMPULSE)
		MSG_WriteByte(buf, cmd->impulse);

	MSG_WriteByte(buf, cmd->lightlevel);
	MSG_WriteByte(buf, cmd->msec);
}

/*
============
MSG_WriteBitUsercmd

Bit-level delta encoder for a movement command, used by the low-bandwidth
protocol.  Movement values are packed to the minimum number of bits.
============
*/
void MSG_WriteBitUsercmd( usercmd_t* cmd, usercmd_t* from )
{
	byte	bits;
	int		i;
	short	value;

	bits = 0;
	for (i = 0; i < 3; i++)
	{
		if (cmd->angles[i] != from->angles[i])
			bits |= 1 << i;
	}
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

// header
	for (i = 7; i >= 0; i--)
		MSG_WriteOneBit(bits & (1 << i));

// yaw / pitch as 10-bit fixed-point
	if (bits & CM_ANGLE1)
	{
		value = (short)((int)(cmd->angles[0] * 1024 / 360) & 0x3ff);
		for (i = 9; i >= 0; i--)
			MSG_WriteOneBit(value & (1 << i));
	}
	if (bits & CM_ANGLE2)
	{
		value = (short)((int)(cmd->angles[1] * 1024 / 360) & 0x3ff);
		for (i = 9; i >= 0; i--)
			MSG_WriteOneBit(value & (1 << i));
	}
	if (bits & CM_ANGLE3)
	{
		value = (short)(int)(cmd->angles[2] * 64 / 360);
		for (i = 5; i >= 0; i--)
			MSG_WriteOneBit(value & 0x3f & (1 << i));
	}

// movement values as sign + 6-bit magnitude
	if (bits & CM_FORWARD)
	{
		value = (short)(int)(cmd->forwardmove / 4);
		MSG_WriteOneBit(value < 0);
		if (value < 0)
			value = -value;
		for (i = 6; i >= 0; i--)
			MSG_WriteOneBit(value & 0xffff & (1 << i));
	}
	if (bits & CM_SIDE)
	{
		value = (short)(int)(cmd->sidemove / 4);
		MSG_WriteOneBit(value < 0);
		if (value < 0)
			value = -value;
		for (i = 6; i >= 0; i--)
			MSG_WriteOneBit(value & 0xffff & (1 << i));
	}
	if (bits & CM_UP)
	{
		value = (short)(int)(cmd->upmove / 4);
		MSG_WriteOneBit(value < 0);
		if (value < 0)
			value = -value;
		for (i = 6; i >= 0; i--)
			MSG_WriteOneBit(value & 0xffff & (1 << i));
	}

	if (bits & CM_BUTTONS)
	{
		for (i = 15; i >= 0; i--)
			MSG_WriteOneBit(cmd->buttons & (1 << i));
	}
	if (bits & CM_IMPULSE)
	{
		for (i = 7; i >= 0; i--)
			MSG_WriteOneBit(cmd->impulse & (1 << i));
	}

	for (i = 5; i >= 0; i--)
		MSG_WriteOneBit((cmd->lightlevel >> 2) & (1 << i));

	for (i = 7; i >= 0; i--)
		MSG_WriteOneBit(cmd->msec & (1 << i));
}

/*
============
MSG_WriteUsercmdByProtocol

Pick the movement encoder that matches the negotiated protocol version.
============
*/
void MSG_WriteUsercmdByProtocol( sizebuf_t* buf, usercmd_t* cmd, usercmd_t* from )
{
	if (PROTOCOL_VERSION == PROTOCOL_VERSION_OLD)
		MSG_WriteUsercmdGold(buf, cmd, from);
	else
		MSG_WriteUsercmd36(buf, cmd, from);
}

//
// reading functions
//
int				msg_readcount;
qboolean		msg_badread;

void MSG_BeginReading( void )
{
	msg_readcount = 0;
	msg_badread = FALSE;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar( void )
{
	int     c;

	if (msg_readcount + 1 > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadByte( void )
{
	int     c;

	if (msg_readcount + 1 > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadShort( void )
{
	int     c;

	if (msg_readcount + 2 > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount + 1] << 8));

	msg_readcount += 2;

	return c;
}

int MSG_ReadWord( void )
{
	int     c;

	if (msg_readcount + 2 > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount + 1] << 8);

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong( void )
{
	int     c;

	if (msg_readcount + 4 > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount + 1] << 8)
	+ (net_message.data[msg_readcount + 2] << 16)
	+ (net_message.data[msg_readcount + 3] << 24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat( void )
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	dat.b[0] = net_message.data[msg_readcount];
	dat.b[1] = net_message.data[msg_readcount + 1];
	dat.b[2] = net_message.data[msg_readcount + 2];
	dat.b[3] = net_message.data[msg_readcount + 3];
	msg_readcount += 4;

	dat.l = LittleLong(dat.l);

	return dat.f;
}

int MSG_ReadBuf( int iSize, void* pbuf )
{
	if (msg_readcount + iSize > net_message.cursize)
	{
		msg_badread = TRUE;
		return -1;
	}

	memcpy(pbuf, &net_message.data[msg_readcount], iSize);
	msg_readcount += iSize;

	return 1;
}

char* MSG_ReadString( void )
{
	static char     string[2048];
	int             l, c;

	l = 0;
	do
	{
		c = MSG_ReadChar();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

char* MSG_ReadStringUntil( int terminator )
{
	static char string[1024];
	int l, c;

	l = 0;
	do
	{
		c = MSG_ReadChar();
		if (c == -1 || c == 0 || c == terminator)
			break;
		string[l++] = c;
	} while (l < sizeof(string) - 1);
	string[l] = 0;
	return string;
}

char* MSG_ReadStringLine( void )
{
	static char     string[2048];
	int             l, c;

	l = 0;
	do
	{
		c = MSG_ReadChar();
		if (c == -1 || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord( void )
{
	int c;

	if (msg_readcount + 2 > net_message.cursize)
	{
		msg_badread = TRUE;
		c = -1;
	}
	else
	{
		c = (short)(net_message.data[msg_readcount]
		+ (net_message.data[msg_readcount + 1] << 8));
		msg_readcount += 2;
	}

	return c * (1.0f / 8);
}

float MSG_ReadAngle( void )
{
	int c;

	if (msg_readcount + 1 > net_message.cursize)
	{
		msg_badread = TRUE;
		c = -1;
	}
	else
	{
		c = (signed char)net_message.data[msg_readcount];
		msg_readcount++;
	}

	return c * (360.0f / 256);
}

float MSG_ReadHiresAngle( void )
{
	int c;

	if (msg_readcount + 2 > net_message.cursize)
	{
		msg_badread = TRUE;
		c = -1;
	}
	else
	{
		c = (short)(net_message.data[msg_readcount]
		+ (net_message.data[msg_readcount + 1] << 8));
		msg_readcount += 2;
	}

	return c * (360.0f / 65536);
}

void MSG_ReadDeltaUsercmd( usercmd_t* move, usercmd_t* from )
{
	int bits;

	memcpy(move, from, sizeof(usercmd_t));

	bits = MSG_ReadByte();

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadHiresAngle();
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadHiresAngle();
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadAngle();

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadFloat();
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadFloat();
	if (bits & CM_UP)
		move->upmove = MSG_ReadFloat();

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadShort();

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte();

// read lightlevel
	move->lightlevel = MSG_ReadByte();

// read time to run command
	move->msec = MSG_ReadByte();
}

/*
============
MSG_ReadUsercmdGold

Decode a delta-compressed movement command.
============
*/
void MSG_ReadUsercmdGold( usercmd_t* move, usercmd_t* from )
{
	int		bits;

	memcpy(move, from, sizeof(usercmd_t));

	bits = MSG_ReadByte();

	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort() * (360.0f / 65536);
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort() * (360.0f / 65536);
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadChar() * (360.0f / 256);

	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadFloat();
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadFloat();
	if (bits & CM_UP)
		move->upmove = MSG_ReadFloat();

	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadShort();
	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte();

	move->lightlevel = MSG_ReadByte();
	move->msec = MSG_ReadByte();
}

/*
============
MSG_ReadUsercmd36

Decode a delta-compressed movement command for the 1.0.3.6 protocol, where
movement values arrive as scaled shorts.
============
*/
void MSG_ReadUsercmd36( usercmd_t* move, usercmd_t* from )
{
	int		bits;

	memcpy(move, from, sizeof(usercmd_t));

	bits = MSG_ReadByte();

	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort() * (360.0f / 65536);
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort() * (360.0f / 65536);
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadChar() * (360.0f / 256);

	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort();
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort();
	if (bits & CM_UP)
		move->upmove = MSG_ReadShort();

	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadShort();
	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte();

	move->lightlevel = MSG_ReadByte();
	move->msec = MSG_ReadByte();
}

/*
============
MSG_ReadBitUsercmd

Bit-level decoder matching MSG_WriteBitUsercmd.
============
*/
void MSG_ReadBitUsercmd( usercmd_t* move, usercmd_t* from )
{
	int		bits;
	int		i;
	int		sign;
	short	value;

	memcpy(move, from, sizeof(usercmd_t));

	bits = 0;
	for (i = 7; i >= 0; i--)
	{
		if (MSG_ReadOneBit())
			bits |= (1 << i) & 0xff;
	}

	if (bits & CM_ANGLE1)
	{
		value = 0;
		for (i = 9; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		move->angles[0] = (short)value * (360.0f / 1024);
	}
	if (bits & CM_ANGLE2)
	{
		value = 0;
		for (i = 9; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		move->angles[1] = (short)value * (360.0f / 1024);
	}
	if (bits & CM_ANGLE3)
	{
		value = 0;
		for (i = 5; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xff;
		}
		move->angles[2] = (short)value * (360.0f / 64);
	}

// wrap the recovered angles back into [-180, 180)
	for (i = 0; i < 3; i++)
	{
		if (move->angles[i] > 180.0f)
			move->angles[i] -= 360.0f;
		if (move->angles[i] < -180.0f)
			move->angles[i] += 360.0f;
	}

	if (bits & CM_FORWARD)
	{
		sign = MSG_ReadOneBit();
		value = 0;
		for (i = 6; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		if (sign)
			value = -value;
		move->forwardmove = (short)value * 4.0f;
	}
	if (bits & CM_SIDE)
	{
		sign = MSG_ReadOneBit();
		value = 0;
		for (i = 6; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		if (sign)
			value = -value;
		move->sidemove = (short)value * 4.0f;
	}
	if (bits & CM_UP)
	{
		sign = MSG_ReadOneBit();
		value = 0;
		for (i = 6; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		if (sign)
			value = -value;
		move->upmove = (short)value * 4.0f;
	}

	if (bits & CM_BUTTONS)
	{
		value = 0;
		for (i = 15; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xffff;
		}
		move->buttons = value;
	}
	if (bits & CM_IMPULSE)
	{
		value = 0;
		for (i = 7; i >= 0; i--)
		{
			if (MSG_ReadOneBit())
				value |= (1 << i) & 0xff;
		}
		move->impulse = (char)value;
	}

	value = 0;
	for (i = 5; i >= 0; i--)
	{
		if (MSG_ReadOneBit())
			value |= (1 << i) & 0xff;
	}
	move->lightlevel = (char)(value << 2);

	value = 0;
	for (i = 7; i >= 0; i--)
	{
		if (MSG_ReadOneBit())
			value |= (1 << i) & 0xff;
	}
	move->msec = (char)value;
}

/*
============
MSG_ReadUsercmd

Pick the movement decoder that matches the negotiated protocol version.
============
*/
void MSG_ReadUsercmd( usercmd_t* move, usercmd_t* from )
{
	if (PROTOCOL_VERSION == PROTOCOL_VERSION_OLD)
		MSG_ReadUsercmdGold(move, from);
	else
		MSG_ReadUsercmd36(move, from);
}

//
// bit-level reading, used by the delta player/entity decoders
//
typedef struct bitread_s
{
	int			bytecount;		// index of the next byte to consume
	sizebuf_t*	buf;			// message being read
	int			startbit;		// byte offset the read started at
	int			numbits;		// bits consumed so far
	int			curbit;			// bit position within the current byte
	byte*		curbyte;		// pointer to the current byte
} bitread_t;

bitread_t	bitread;

//
// bit-level writing state
//
typedef struct bitwrite_s
{
	int			curbit;			// bit position within the current byte
	byte*		curbyte;		// pointer to the byte being filled
	sizebuf_t*	buf;			// message being written
} bitwrite_t;

bitwrite_t	bitwrite;

void MSG_StartBitReading( sizebuf_t* buf )
{
	int start;

	start = msg_readcount;

	bitread.bytecount = start;
	bitread.startbit = start;
	bitread.curbit = 0;
	bitread.numbits = 0;
	bitread.curbyte = buf->data + start;
	bitread.buf = buf;
	bitread.bytecount = start + 1;

	if (buf->cursize < start + 1)
		msg_badread = TRUE;
}

void MSG_EndBitReading( sizebuf_t* buf )
{
	if (buf->cursize < bitread.bytecount)
		msg_badread = TRUE;

	msg_readcount = bitread.bytecount;

	bitread.startbit = 0;
	bitread.curbit = 0;
	bitread.numbits = 0;
	bitread.curbyte = 0;
	bitread.buf = 0;
}

qboolean MSG_ReadOneBit( void )
{
	int mask;

	if (msg_badread)
		return TRUE;

	if (bitread.curbit > 7)
	{
		bitread.bytecount++;
		bitread.curbit = 0;
		bitread.numbits++;
		bitread.curbyte++;
	}

	if (bitread.buf->cursize < bitread.bytecount)
	{
		msg_badread = TRUE;
		return TRUE;
	}

	mask = 1 << (7 - bitread.curbit);
	bitread.curbit++;

	return (*bitread.curbyte & mask & 0xff) != 0;
}

unsigned char MSG_ReadBitField8( unsigned int numbits )
{
	unsigned int value;

	value = 0;
	while ((int)numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= (1 << numbits) & 0xff;
	}

	return value;
}

unsigned short MSG_ReadBitField16( unsigned int numbits )
{
	unsigned int value;

	value = 0;
	while ((int)numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= (1 << numbits) & 0xffff;
	}

	return value;
}

unsigned int MSG_ReadBitField32( unsigned int numbits )
{
	unsigned int value;

	value = 0;
	while ((int)numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= 1 << numbits;
	}

	return value;
}

//
// bit-level writing, used by the delta player/entity encoders
//
void MSG_StartBitWriting( sizebuf_t* buf )
{
	bitwrite.curbit = 0;
	bitwrite.curbyte = buf->data + buf->cursize;
	bitwrite.buf = buf;
}

void MSG_WriteOneBit( int value )
{
	int	shift;

	if (bitwrite.buf->overflowed)
		return;

	if (bitwrite.curbit > 7)
	{
		SZ_GetSpace(bitwrite.buf, 1);
		bitwrite.curbit = 0;
		bitwrite.curbyte++;
		if (bitwrite.buf->overflowed)
			return;
	}

	shift = 7 - bitwrite.curbit;
	if (value)
		*bitwrite.curbyte |= 1 << shift;
	else
		*bitwrite.curbyte &= ~(1 << shift);

	bitwrite.curbit++;
}

void MSG_EndBitWriting( sizebuf_t* buf )
{
	int		i;
	byte	mask;

	if (bitwrite.buf->overflowed)
		return;

// pad the remainder of the current byte with zero bits
	mask = 0xff;
	for (i = 0; i < 8 - bitwrite.curbit; i++)
		mask &= ~(1 << i);

	*bitwrite.curbyte &= mask;

	SZ_GetSpace(bitwrite.buf, 1);
	bitwrite.curbyte++;

	bitwrite.curbit = 0;
	bitwrite.curbyte = 0;
	bitwrite.buf = 0;
}

void MSG_WriteBitByte( byte* data, int numbits )
{
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(*data & (1 << numbits));
	}
}

void MSG_WriteBitShort( unsigned short* data, int numbits )
{
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(*data & (1 << numbits));
	}
}

void MSG_WriteBitLong( unsigned int* data, int numbits )
{
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(*data & (1 << numbits));
	}
}

void MSG_WriteSBitByte( char* data, int numbits )
{
	byte	value;

	if (*data < 0)
		MSG_WriteOneBit(1);
	else
		MSG_WriteOneBit(0);

	value = *data;
	if ((char)value < 0)
		value = 0U - value;

	numbits--;
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(value & (1 << numbits));
	}
}

void MSG_WriteSBitShort( short* data, int numbits )
{
	unsigned short	value;

	if (*data < 0)
		MSG_WriteOneBit(1);
	else
		MSG_WriteOneBit(0);

	value = *data;
	if ((short)value < 0)
		value = 0U - value;

	numbits--;
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(value & (1 << numbits));
	}
}

void MSG_WriteSBitLong( int* data, int numbits )
{
	unsigned int	value;

	if (*data < 0)
		MSG_WriteOneBit(1);
	else
		MSG_WriteOneBit(0);

	value = *data;
	if ((int)value < 0)
		value = 0U - value;

	numbits--;
	while (numbits > 0)
	{
		numbits--;
		MSG_WriteOneBit(value & (1 << numbits));
	}
}

void MSG_WriteBitAngle( float angle, int numbits )
{
	unsigned int	shift;
	unsigned int	value;

	shift = 1 << numbits;
	value = (int)(angle * (float)(int)shift) / 360;
	value &= shift - 1;

	if (numbits > 16)
	{
		unsigned int	v = value;
		while (numbits > 0)
		{
			numbits--;
			MSG_WriteOneBit(v & (1 << numbits));
		}
	}
	else if (numbits > 8)
	{
		unsigned short	v = value;
		while (numbits > 0)
		{
			numbits--;
			MSG_WriteOneBit(v & (1 << numbits));
		}
	}
	else
	{
		unsigned char	v = value;
		while (numbits > 0)
		{
			numbits--;
			MSG_WriteOneBit(v & (1 << numbits));
		}
	}
}

float MSG_ReadScaledBitValue( unsigned int numbits )
{
	unsigned int	value;
	float			scale;

	scale = (float)(1 << numbits);
	value = 0;

	if ((int)numbits > 16)
	{
		while ((int)numbits > 0)
		{
			numbits--;
			if (MSG_ReadOneBit())
				value |= 1 << numbits;
		}
		return (int)value * (360.0f / scale);
	}
	else if ((int)numbits > 8)
	{
		while ((int)numbits > 0)
		{
			numbits--;
			if (MSG_ReadOneBit())
				value |= (1 << numbits) & 0xffff;
		}
		return (short)value * (360.0f / scale);
	}
	else
	{
		while ((int)numbits > 0)
		{
			numbits--;
			if (MSG_ReadOneBit())
				value |= (1 << numbits) & 0xff;
		}
		return (char)value * (360.0f / scale);
	}
}

unsigned int MSG_PeekByteBits( unsigned int numbits )
{
	bitread_t saved;
	unsigned int value;

	saved = bitread;
	value = 0;
	while ((int)numbits-- > 0)
	{
		if (MSG_ReadOneBit())
			value |= (1 << numbits) & 0xff;
	}
	bitread = saved;
	return value;
}

unsigned int MSG_PeekBits( unsigned int numbits )
{
	bitread_t		saved;
	unsigned int	value;

	saved = bitread;

	value = 0;
	while ((int)numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= 1 << numbits;
	}

	bitread = saved;

	return value;
}

int MSG_ReadSignMagnitude8( int numbits )
{
	int				sign;
	unsigned int	value;
	char			result;

	sign = MSG_ReadOneBit();

	value = 0;
	result = 0;
	numbits--;
	while (numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= (1 << numbits) & 0xff;
		result = value;
	}

	if (sign)
		result = -result;

	return result;
}

short MSG_ReadSignMagnitude16( int numbits )
{
	int				sign;
	unsigned int	value;
	short			result;

	sign = MSG_ReadOneBit();

	value = 0;
	result = 0;
	numbits--;
	while (numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= (1 << numbits) & 0xffff;
		result = value;
	}

	if (sign)
		result = -result;

	return result;
}

unsigned int MSG_ReadSignMagnitude32( int numbits )
{
	int				sign;
	unsigned int	value;

	sign = MSG_ReadOneBit();

	value = 0;
	numbits--;
	while (numbits > 0)
	{
		numbits--;
		if (MSG_ReadOneBit())
			value |= 1 << numbits;
	}

	if (sign)
		value = 0U - value;

	return value;
}


//===========================================================================

void SZ_Alloc( sizebuf_t* buf, int startsize )
{
	if (startsize < 256)
		startsize = 256;
	buf->data = Hunk_AllocName(startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}

void SZ_Clear( sizebuf_t* buf )
{
	buf->cursize = 0;
	buf->overflowed = FALSE;
}

void* SZ_GetSpace( sizebuf_t* buf, int length )
{
	void* data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
		{
			if (!buf->maxsize)
				Sys_Error("SZ_GetSpace:  Tried to write to an uninitialized sizebuf_t");

			Sys_Error("SZ_GetSpace: overflow without allowoverflow set");
		}

		if (length > buf->maxsize)
			Sys_Error("SZ_GetSpace: %i is > full buffer size", length);

		Con_Printf("SZ_GetSpace: overflow\n");

		buf->cursize = 0;
		buf->overflowed = TRUE;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write( sizebuf_t* buf, void* data, int length )
{
	Q_memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print( sizebuf_t* buf, char* data )
{
	int             len;

	len = Q_strlen(data) + 1;

// byte * cast to keep VC++ happy
	if (buf->data[buf->cursize - 1])
		Q_memcpy((byte*)SZ_GetSpace(buf, len), data, len); // no trailing 0
	else
		Q_memcpy((byte*)SZ_GetSpace(buf, len - 1) - 1, data, len); // write over trailing 0
}


//============================================================================


/*
============
COM_SkipPath
============
*/
char* COM_SkipPath( char* pathname )
{
	char* last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( char* in, char* out )
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FileExtension
============
*/
char* COM_FileExtension( char* in )
{
	static char exten[8];
	int		i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i = 0; i < 7 && *in; i++, in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase( char* in, char* out )
{
	int len, start, end;

	if (!in)
		Sys_Error("COM_FileBase passed a null string: %s", in);

	len = strlen(in) - 1;

	// scan backward for '.'
	end = len;
	while (end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.')		// no '.', copy to end
		end = len;
	else
		end--;					// Found '.', copy to left of '.'

	// scan backward for a path separator
	start = len;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new string
	len = end - start + 1;

	if (!in)
		Sys_Error("COM_FileBase passed a null string: %s", in);

	// Copy partial string
	strncpy(out, &in[start], len);
	out[len] = '\0';
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char* path, char* extension )
{
	char* src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat(path, extension);
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char* COM_Parse( char* data )
{
	int             c;
	int             len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ' || c == ',')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || (!com_ignorecolons && c == ':'))
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || (!com_ignorecolons && c == ':'))
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

/*
================
COM_HexConvert

Convert a string of hex characters into the equivalent bytes.
================
*/
unsigned char COM_Nibble( char c )
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return '0';
}

void COM_HexConvert( char* pszInput, int nInputLength, byte* pOutput )
{
	int		i;
	byte*	out;

	out = pOutput;
	for (i = 0; i < nInputLength; i += 2)
	{
		*out = (COM_Nibble(pszInput[0]) << 4) | COM_Nibble(pszInput[1]);
		out++;
		pszInput += 2;
	}
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm( char* parm )
{
	int             i;

	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp(parm, com_argv[i]))
			return i;
	}

	return 0;
}

DLL_EXPORT int COM_CheckParmEx( char* parm, char** argv )
{
	int             i;

	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp(parm, com_argv[i]))
		{
			if (argv)
			{
				if (i < com_argc - 1)
				{
					*argv = com_argv[i + 1];
					return i;
				}
				*argv = 0;
			}

			return i;
		}
	}

	return 0;
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered( void )
{
/*
	int             h;
	unsigned short  check[128];
	int                     i;

	COM_OpenFile("gfx/pop.lmp", &h);
	static_registered = 0;

	if (h == -1)
	{
#if WINDED
		Sys_Error("This dedicated server requires a full registered copy of Quake");
#endif
		Con_Printf("Playing shareware version.\n");
		if (com_modified)
			Sys_Error("You must have the registered version to use modified games");
		return;
	}

	Sys_FileRead(h, check, sizeof(check));
	COM_CloseFile(h);

	for (i = 0; i < 128; i++)
		if (pop[i] != (unsigned short)BigShort(check[i]))
			Sys_Error("Corrupted data file.");

	Cvar_Set("cmdline", com_cmdline);
	Cvar_Set("registered", "1");
	static_registered = 1;
	Con_Printf("Playing registered version.\n");
*/
}


void COM_Path_f( void );


/*
================
COM_InitArgv
================
*/
void COM_InitArgv( int argc, char** argv )
{
	qboolean        safe;
	int             i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j = 0; (j < MAX_NUM_ARGVS) && (j < argc); j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	com_cmdline[n] = 0;

	safe = FALSE;

	for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc);
		com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp("-safe", argv[com_argc]))
			safe = TRUE;
	}

	if (safe)
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i = 0; i < NUM_SAFE_ARGVS; i++)
		{
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

	if (COM_CheckParm("-rogue"))
	{
		rogue = TRUE;
		standard_quake = FALSE;
	}

	if (COM_CheckParm("-hipnotic"))
	{
		hipnotic = TRUE;
		standard_quake = FALSE;
	}
}

byte	swaptest[2] = { 1, 0 };

int		filelog_level;
HANDLE	filelog_handle;
static int filelog_lastend;

void COM_SetFileLog( int level )
{
	filelog_level = level;
	if (level == 0)
	{
		Sys_CloseHandle(filelog_handle);
		filelog_handle = 0;
	}
	else
	{
		filelog_handle = Sys_OpenHandle("\\PC\\action.log", "wt");
	}
}

void COM_LogFileOpen( char* source, char* filename, int offset, int size )
{
	int seek;

	if (filelog_level && filelog_handle)
	{
		seek = 0;
		if (offset && filelog_lastend)
			seek = offset - filelog_lastend;
		filelog_lastend = offset + size;
		Sys_FPrintf(filelog_handle, "Opened %s from %s: offset %d, size %d, seek %d\n",
			filename, source, offset, size, seek);
	}
}

void Cmd_filelog_f( void )
{
	filelog_level = atoi(Cmd_Argv(1));

	if (filelog_level == 0)
	{
		Sys_CloseHandle(filelog_handle);
		filelog_handle = 0;
	}
	else
	{
		filelog_handle = Sys_OpenHandle("\\PC\\action.log", "wt");
	}
}

/*
================
COM_Init
================
*/
void COM_Init( char* basedir )
{
// set the byte swapping variables in a portable manner
	if (*(short*)swaptest == 1)
	{
		bigendien = FALSE;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = TRUE;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	Cvar_RegisterVariable(&registered);
	Cvar_RegisterVariable(&cmdline);
	Cmd_AddCommand("path", COM_Path_f);
	Cmd_AddCommand("filelog", Cmd_filelog_f);

	memset(&bitwrite, 0, sizeof(bitwrite));
	memset(&bitread, 0, sizeof(bitread));

	COM_InitFilesystem();
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char* va( char* format, ... )
{
	va_list         argptr;
	static char            string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

/*
============
vstr

prints a vector into a temporary string
bufffer.
============
*/
char* vstr( float* v )
{
	static char string[1024];

	sprintf(string, "%.2f %.2f %.2f", v[0], v[1], v[2]);
	return string;
}


/// just for debugging
int     memsearch( byte* start, int count, int search )
{
	int             i;

	for (i = 0; i < count; i++)
		if (start[i] == search)
			return i;
	return -1;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

int     com_filesize;


//
// in memory
//

// in-memory directory entry: the file's base name, sorted so it can be
// binary-searched.
typedef struct
{
	char    name[24];
	int             filepos, filelen;
} packfile_t;

typedef struct
{
	char    filename[MAX_OSPATH];
	int             handle;
	int             numfiles;
	packfile_t* files;
} pack_t;

//
// on disk
//
typedef struct dpackfile_s
{
	char    name[56];
	int             filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int             dirofs;
	int             dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK       4096

char    com_cachedir[MAX_OSPATH];
char    com_gamedir[MAX_OSPATH];
char    com_gamedirname[64] = GAMENAME;

typedef struct searchpath_s
{
	char    filename[MAX_OSPATH];
	char    gamedir[MAX_OSPATH];
	pack_t* pack;          // only one of filename / pack will be used
	int     filetime;
	short   flags;
	short   pad;
	struct searchpath_s* next;
} searchpath_t;

searchpath_t* com_searchpaths;

void COM_FreeSearchPathEntry( searchpath_t* search )
{
	if (search)
	{
		if (search->pack)
		{
			Sys_FileClose(search->pack->handle);
			if (search->pack->files)
				MnemoFree(search->pack->files);
			MnemoFree(search->pack);
		}
		free(search);
	}
}

void COM_FreeSearchPath( void )
{
	searchpath_t* search;
	searchpath_t* next;
	for (search = com_searchpaths; search; search = next)
	{
		next = search->next;
		COM_FreeSearchPathEntry(search);
	}
	com_searchpaths = NULL;
}

/*
============
COM_Path_f

============
*/
void COM_Path_f( void )
{
	searchpath_t* s;

	Con_Printf("Current search path:\n");
	for (s = com_searchpaths; s; s = s->next)
	{
		if (s->flags)
			Con_Printf("(d) ");

		if (!s->pack)
			Con_Printf("%s\n", s->filename);
		else
			Con_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
	}
}

/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void COM_CreatePath( char* path )
{
	char* ofs;
	char old;

	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{       // create the directory
			old = *ofs;
			*ofs = 0;
			Sys_mkdir(path);
			*ofs = old;
		}
	}
}


/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
FILETIME gFileTime;
int COM_PackFileCompare( const void* a, const void* b );
void COM_GetShortName( char* out, char* in, int maxlen );

/*
===========
COM_FindFileSearch

Core file finder: walks the search path (optionally continuing after a previous
result via pRestart, and optionally restricted to a single gamedir) and opens the
file either on disk or from a pak, into a handle set or a FILE*.
===========
*/
int COM_FindFileSearch( searchpath_t** pRestart, char* gamedir, char* filename, int* phFile, FILE** file )
{
	searchpath_t*	search;
	char			netpath[MAX_OSPATH];
	pack_t*			pak;
	packfile_t*		entry;
	packfile_t		key;
	int				i = -1;
	int				findtime;

	if (file && phFile)
		Sys_Error("COM_FindFile: both phFile and file set");
	if (!file && !phFile)
		Sys_Error("COM_FindFile: neither phFile or file set");

	search = com_searchpaths;

// continue a previous search from where it left off
	if (pRestart && *pRestart)
	{
		for (; search && search != *pRestart; search = search->next)
			;
		if (search)
			search = search->next;
	}

// restrict to a single gamedir if one was requested
	if (gamedir)
		for (; search && Q_stricmp(gamedir, search->gamedir); search = search->next)
			;

	for (; search; search = search->next)
	{
		if (gamedir && Q_stricmp(gamedir, search->gamedir))
			break;

	// check a file in the directory tree first
		sprintf(netpath, "%s/%s", search->filename, filename);
		findtime = Sys_FileTime(netpath);
		*(int*)&gFileTime = findtime;
		if (findtime > 0)
		{
			com_filesize = Sys_FileOpenRead(netpath, &i, 0);
			if (!phFile)
			{
				Sys_FileClose(i);
				*file = (FILE*)Sys_OpenHandle(netpath, "rb");
			}
			else
			{
				phFile[2] = i;
				phFile[0] = 0;
			}
			if (pRestart)
				*pRestart = search;
			if (filelog_level && filelog_handle)
				Sys_FPrintf(filelog_handle, "Opened %s from %s, offset %d, size %d\n", filename, "File", 0, com_filesize);
			return com_filesize;
		}

	// is the element a pak file?
		pak = search->pack;
		if (pak)
		{
			char* name = filename;
			if (*name && (*name == '\\' || *name == '/'))
				name++;
			COM_GetShortName(key.name, name, sizeof(key.name) - 1);
			entry = (packfile_t*)bsearch(&key, pak->files, pak->numfiles, sizeof(packfile_t), COM_PackFileCompare);
			if (entry)
			{	// found it!
				if (!phFile)
				{
					*(int*)&gFileTime = search->filetime;
					*file = (FILE*)Sys_OpenHandle(pak->filename, "rb");
					Sys_FileSeek((int)*file, entry->filepos);
					if (*file)
						DC_fseek((void*)*file, entry->filepos, 0);
				}
				else
				{
					phFile[2] = pak->handle;
					phFile[0] = entry->filepos;
					phFile[1] = entry->filelen;
					Sys_FileSeek(pak->handle, entry->filepos);
				}
				com_filesize = entry->filelen;
				if (pRestart)
					*pRestart = search;
				if (filelog_level && filelog_handle)
					Sys_FPrintf(filelog_handle, "Opened %s from %s, offset %d, size %d\n", filename, "Pak", entry->filepos, entry->filelen);
				return com_filesize;
			}
		}
	}

	if (phFile)
		phFile[2] = -1;
	else
		*file = NULL;
	com_filesize = -1;
	return -1;
}

int COM_FindFile( char* filename, int* phFile, FILE** file )
{
	return COM_FindFileSearch(NULL, NULL, filename, phFile, file);
}

int COM_CompareFileTime( char* filename1, char* filename2, int* iCompare )
{
	FILE* file;
	int filetime1;
	int filetime2;
	int result = 0;

	*iCompare = 0;

	if (!filename1 || !filename2)
		goto done;

	COM_FindFileSearch(NULL, NULL, filename1, NULL, &file);
	if (!file)
		goto done;

	Sys_CloseHandle(file);
	filetime1 = *(int*)&gFileTime;

	COM_FindFileSearch(NULL, NULL, filename2, NULL, &file);
	if (!file)
		goto done;

	Sys_CloseHandle(file);
	filetime2 = *(int*)&gFileTime;

	*iCompare = Sys_CompareFileTime(&filetime1, &filetime2);
	result = 1;

done:
	return result;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int COM_OpenFile( char* filename, int* handle )
{
	return COM_FindFile(filename, handle, NULL);
}

/*
===========
COM_LoadFileChunk

Reads one slice of a file straight into memory the caller already owns, so a
level can be brought in a lump at a time instead of all at once.
===========
*/
void COM_LoadFileChunk( char* path, byte* dest, int offset, int length )
{
	int		hnd[3];
	int		len;

	if (!dest || !length)
		return;

	len = COM_FindFileSearch(NULL, NULL, path, hnd, NULL);
	if (hnd[2] == -1)
		return;

	if (len < offset + length)
		length = len - offset;

	Sys_FileSeek(hnd[2], hnd[0] + offset);
	Sys_FileRead(hnd[2], dest, length);

	COM_CloseFile(hnd[0], hnd[1], hnd[2]);
}

/*
===========
COM_OpenFileByName

Like COM_OpenFile, but restricts the search to a single gamedir.
===========
*/
int COM_OpenFileByName( char* gamedir, char* filename, int* handle )
{
	return COM_FindFileSearch(NULL, gamedir, filename, handle, NULL);
}

/*
===========
COM_FileSize

Length of a file without keeping it open.
===========
*/
int COM_FileSize( char* filename )
{
	int	hFile = 0;
	int	len;

	len = COM_FindFileSearch(NULL, NULL, filename, &hFile, NULL);
	if (hFile)
		Sys_CloseHandle((void*)hFile);

	return len;
}

/*
===========
COM_ChangeGameDir

Drop every search path the previous campaign added and bring the new one's
directory in behind it.
===========
*/
void COM_ChangeGameDir( char* pszDir )
{
	searchpath_t*	search;
	searchpath_t*	next;
	searchpath_t*	keep;
	char			dir[MAX_OSPATH];

// throw away everything the last campaign brought in, keeping the paths that
// were added at startup and are shared by every game directory
	keep = NULL;
	for (search = com_searchpaths; search; search = next)
	{
		next = search->next;

		if (search->flags)
		{
			search->next = keep;
			keep = search;
			continue;
		}

		if (search->pack)
		{
			Sys_FileClose(search->pack->handle);
			if (search->pack->files)
				MnemoFree(search->pack->files);
			MnemoFree(search->pack);
		}

		free(search);
	}

// the survivors came off the list backwards, so put them back in order
	com_searchpaths = NULL;
	while (keep)
	{
		next = keep->next;
		keep->next = com_searchpaths;
		com_searchpaths = keep;
		keep = next;
	}

	strcpy(com_gamedirname, pszDir);

	if (!Q_stricmp(pszDir, GAMENAME))
	{
		// the base game is always on the path, only the current directory moves
		sprintf(dir, "%s/%s", host_parms.basedir, pszDir);
		Info_SetValueForStarKey(serverinfo, "*gamedir", pszDir, MAX_INFO_STRING);
		strcpy(com_gamedir, dir);
	}
	else
	{
		COM_AddGameDirectory(0, host_parms.basedir, pszDir);
	}
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile( char* filename, FILE** file )
{
	return COM_FindFile(filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile( int filepos, int filelen, int handle )
{
	searchpath_t* s;

	for (s = com_searchpaths; s; s = s->next)
		if (s->pack && s->pack->handle == handle)
			return;

	Sys_FileClose(handle);
}


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
cache_user_t* loadcache;
byte* loadbuf;
int             loadsize;
byte* COM_LoadFile( char* path, int usehunk, int* pLength )
{
	int             h[3];
	byte* buf;
	char    base[32];
	int             len;
	int             allocsize;
	qboolean        rounded = FALSE;

	buf = NULL;     // quiet compiler warning

	if (pLength)
		*pLength = 0;

// look for it in the filesystem or pack files
	len = COM_OpenFile(path, h);
	if (h[2] == -1)
		return NULL;

// extract the filename base name for hunk tag
	COM_FileBase(path, base);

	switch (usehunk)
	{
	case 0:
		buf = Z_Malloc(len + 1);
		break;
	case 1:
		buf = Hunk_AllocName(len + 1, base);
		break;
	case 2:
		allocsize = (len + 0x7ff) & ~0x7ff;
		rounded = TRUE;
		buf = Hunk_TempAlloc(allocsize + 1);
		break;
	case 3:
		buf = Cache_Alloc(loadcache, len + 1, base);
		break;
	case 4:
		if (len + 1 > loadsize)
		{
			allocsize = (len + 0x7ff) & ~0x7ff;
			rounded = TRUE;
			buf = Hunk_TempAlloc(allocsize + 1);
		}
		else
			buf = loadbuf;
		break;
	case 5:
		buf = MnemoAllocDbg(len + 1, __FILE__, __LINE__);
		break;
	default:
		Sys_Error("COM_LoadFile: bad usehunk");
	}

	if (!buf)
	{
		Sys_ErrorColor(RGB565_RED, "COM_LoadFile: not enough space for %s", path);
		COM_CloseFile(h[0], h[1], h[2]);
		return NULL;
	}

	buf[len] = 0;

	if (rounded)
		Sys_FileRead(h[2], buf, allocsize);
	else
		Sys_FileRead(h[2], buf, len);
	COM_CloseFile(h[0], h[1], h[2]);

	if (pLength)
		*pLength = len;

	return buf;
}

/*
============
COM_FreeFile

Hand back a buffer COM_LoadFileForMe put on the heap.
============
*/
void COM_FreeFile( void* buffer )
{
	if (buffer)
		free(buffer);
}

/*
============
COM_FreeTempFile

Drop the scratch buffer the last COM_LoadTempFile read into. There is only
ever one of them, so it does not need naming.
============
*/
void COM_FreeTempFile( void )
{
	_FreeBlock();
}

byte* COM_LoadFileLimit( char* path, int pos, int cbmax, int* pcbread, int* phFile )
{
	int             h[3];
	byte* buf;
	char    base[32];
	int             len;

	if (phFile[2] == -1)
	{
	// look for it in the filesystem or pack files
		len = COM_OpenFile(path, h);
		h[1] = com_filesize;
	}
	else
	{
		h[0] = phFile[0];
		h[1] = phFile[1];
		h[2] = phFile[2];
		len = h[1];
	}

	if (h[2] == -1)
		return 0;

	if (pos > len)
		Sys_Error("COM_LoadFileLimit: invalid seek position for %s", path);

	COM_FileSeek(h[0], h[1], h[2], pos);

	if (len > cbmax)
		len = cbmax;

	*pcbread = len;

	if (path)
		COM_FileBase(path, base);

	buf = Hunk_TempAlloc(len + 1);
	if (!buf)
	{
		if (path)
			Sys_Error("COM_LoadFileLimit: not enough space for %s", path);
		COM_CloseFile(h[0], h[1], h[2]);
		return NULL;
	}

	buf[len] = 0;

	len = Sys_FileRead(h[2], buf, len);
	phFile[0] = h[0];
	phFile[1] = h[1];
	phFile[2] = h[2];
	*pcbread = len;
	return buf;
}

/*
============
COM_LoadFileLimitAsync

As COM_LoadFileLimit, but the caller supplies the destination and the read is
started in the background. The data is not there until the overlapped record
says the read has finished -- poll it with COM_OpenFileAsync.
============
*/
byte* COM_LoadFileLimitAsync( char* path, int pos, int cbmax, int* pcbread, int* phFile, byte* dest, LPOVERLAPPED pov )
{
	int             h[3];
	char    base[32];
	int             len;

	if (phFile[2] == -1)
	{
	// look for it in the filesystem or pack files
		len = COM_OpenFile(path, h);
		h[1] = com_filesize;
	}
	else
	{
		h[0] = phFile[0];
		h[1] = phFile[1];
		h[2] = phFile[2];
		len = h[1];
	}

	if (h[2] == -1)
		return NULL;

	if (pos > len)
		Sys_Error("COM_LoadFileLimit: invalid seek position for %s", path);

	COM_FileSeek(h[0], h[1], h[2], pos);

	if (len > cbmax)
		len = cbmax;

	*pcbread = len;

	if (path)
		COM_FileBase(path, base);

	if (!dest)
	{
		if (path)
			Sys_Error("COM_LoadFileLimit: not enough space for %s", path);
		COM_CloseFile(h[0], h[1], h[2]);
		return NULL;
	}

	Sys_FileReadAsync((void *)h[2], dest, len, pov);
	phFile[0] = h[0];
	phFile[1] = h[1];
	phFile[2] = h[2];
	*pcbread = len;
	return dest;
}

/*
============
COM_OpenFileAsync

Make sure the file is open and report whether the read started for it is still
running. Returns 0 once the data has landed.
============
*/
int COM_OpenFileAsync( char* path, int* phFile, LPOVERLAPPED pov )
{
	int             h[3];

	if (phFile[2] == -1)
	{
		COM_OpenFile(path, h);
	}
	else
	{
		h[0] = phFile[0];
		h[1] = phFile[1];
		h[2] = phFile[2];
	}

	if (h[2] == -1)
		return 0;

	return Sys_AsyncBusy(h[2], pov);
}

byte* COM_LoadHunkFile( char* path )
{
	return COM_LoadFile(path, 1, NULL);
}

byte* COM_LoadTempFile( char* path, int* pLength )
{
	return COM_LoadFile(path, 2, pLength);
}

byte* COM_LoadCacheFile( char* path, struct cache_user_s* cu )
{
	loadcache = cu;
	return COM_LoadFile(path, 3, NULL);
}

// uses temp hunk if larger than bufsize
byte* COM_LoadStackFile( char* path, void* buffer, int bufsize )
{
	loadbuf = buffer;
	loadsize = bufsize;
	return COM_LoadFile(path, 4, NULL);
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
int COM_PackFileCompare( const void* a, const void* b )
{
	return strcmp(((packfile_t*)a)->name, ((packfile_t*)b)->name);
}

int COM_PackFileSortCompare( const void* a, const void* b )
{
	return strcmp(((packfile_t*)a)->name, ((packfile_t*)b)->name);
}

void COM_SortPackFiles( packfile_t* files, int count )
{
	qsort(files, count, sizeof(packfile_t), COM_PackFileSortCompare);
}

void COM_GetShortName( char* out, char* in, int maxlen )
{
	char*	src;
	char*	o;
	char	c;

	src = strchr(in, 0) - maxlen;
	if (src < in)
		src = in;

	strncpy(out, src, maxlen);

	if (*out)
	{
		o = out;
		c = *o;
		do
		{
			c = tolower(c);
			*o = c;
			if (c == '\\')
				*o = '/';
			o++;
			c = *o;
		} while (c);
	}

	out[maxlen] = 0;
}

pack_t* COM_LoadPackFile( char* packfile )
{
	dpackheader_t   header;
	int                             i;
	dpackfile_t* ondisk;
	int                             numpackfiles;
	pack_t* pack;
	int                             packhandle;
	CRC32_t					crc;

	if (Sys_FileOpenRead(packfile, &packhandle, 1) == -1)
		return NULL;

	Sys_FileRead(packhandle, (void*)&header, sizeof(header));
	if (header.id[0] != 'P' || header.id[1] != 'A'
		|| header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error("%s is not a packfile", packfile);
	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error("%s has %i files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = TRUE;    // not the original file

// read the on-disk directory into a temporary buffer
	ondisk = (dpackfile_t*)MnemoAlloc(numpackfiles * sizeof(dpackfile_t), MNEMO_FLAG_MALLOC, 0, packfile);
	Sys_FileSeek(packhandle, header.dirofs);
	Sys_FileRead(packhandle, ondisk, header.dirlen);

// crc the directory to check for modifications
	CRC32_Init(&crc);
	CRC32_ProcessBuffer(&crc, ondisk, header.dirlen);
	if (crc != PAK0_CRC)
		com_modified = TRUE;

	for (i = 0; i < numpackfiles; i++)
	{
		ondisk[i].filepos = LittleLong(ondisk[i].filepos);
		ondisk[i].filelen = LittleLong(ondisk[i].filelen);
	}

	pack = (pack_t*)MnemoAlloc(sizeof(pack_t), MNEMO_FLAG_MALLOC, 0, Bmakename(packfile, '^'));
	strcpy(pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = (packfile_t*)MnemoAlloc(numpackfiles * sizeof(packfile_t), MNEMO_FLAG_MALLOC, 0, Bmakename(packfile, ' '));

// build the compact, filebase-only directory and sort it for binary search
	for (i = 0; i < numpackfiles; i++)
	{
		COM_GetShortName(pack->files[i].name, ondisk[i].name, sizeof(pack->files[i].name) - 1);
		pack->files[i].filepos = ondisk[i].filepos;
		pack->files[i].filelen = ondisk[i].filelen;
	}

	MnemoFree(ondisk);
	qsort(pack->files, numpackfiles, sizeof(packfile_t), COM_PackFileSortCompare);

	return pack;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
void COM_AddGameDirectory( int flags, char* basedir, char* gamedir )
{
	int						i;
	searchpath_t*			search;
	pack_t*					pak;
	char                    pakfile[MAX_OSPATH];
	char					dir[MAX_OSPATH];
	char*					p;

	sprintf(dir, "%s/%s", basedir, gamedir);
	Info_SetValueForStarKey(serverinfo, "*gamedir", gamedir, MAX_INFO_STRING);
	strcpy(com_gamedir, dir);

//
// add any pak files in the format pak0.pak pak1.pak, ...
//
	for (i = 0; ; i++)
	{
		sprintf(pakfile, "%s/pak%i.pak", dir, i);

		for (p = pakfile; *p; p++)
			if (*p == '/')
				*p = '\\';

		pak = COM_LoadPackFile(pakfile);
		if (!pak)
			break;

		search = MnemoAllocDbg(sizeof(searchpath_t), __FILE__, __LINE__);
		memset(search, 0, sizeof(searchpath_t));
		strcpy(search->gamedir, gamedir);
		search->flags = flags;
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;
		search->filetime = Sys_FileTime(pakfile);
	}

//
// add the directory to the search path
//
	search = MnemoAllocDbg(sizeof(searchpath_t), __FILE__, __LINE__);
	memset(search, 0, sizeof(searchpath_t));
	strcpy(search->gamedir, gamedir);
	search->flags = flags;
	strcpy(search->filename, dir);
	search->next = com_searchpaths;
	com_searchpaths = search;
}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem( void )
{
	int             i, j;
	char    basedir[MAX_OSPATH];
	searchpath_t* search;

//
// -basedir <path>
// Overrides the system supplied base directory (under GAMENAME)
//
	i = COM_CheckParm("-basedir");
	if (i && i < com_argc - 1)
		strcpy(basedir, com_argv[i + 1]);
	else
		strcpy(basedir, host_parms.basedir);

	j = strlen(basedir);

	if (j > 0)
	{
		if ((basedir[j - 1] == '\\') || (basedir[j - 1] == '/'))
			basedir[j - 1] = 0;
	}

//
// -cachedir <path>
// Overrides the system supplied cache directory (NULL or /qcache)
// -cachedir - will disable caching.
//
	i = COM_CheckParm("-cachedir");
	if (i && i < com_argc - 1)
	{
		if (com_argv[i + 1][0] == '-')
			com_cachedir[0] = 0;
		else
			strcpy(com_cachedir, com_argv[i + 1]);
	}
	else if (host_parms.cachedir)
		strcpy(com_cachedir, host_parms.cachedir);
	else
		com_cachedir[0] = 0;

//
// start up with GAMENAME by default (id1)
//
	COM_AddGameDirectory(1, basedir, GAMENAME);

//
// -game <gamedir>
// Adds basedir/gamedir as an override game
//
	i = COM_CheckParm("-game");
	if (i && i < com_argc - 1)
	{
		com_modified = TRUE;
		COM_AddGameDirectory(0, basedir, com_argv[i + 1]);
	}

//
// -path <dir or packfile> [<dir or packfile>] ...
// Fully specifies the exact serach path, overriding the generated one
//
	i = COM_CheckParm("-path");
	if (i)
	{
		com_modified = TRUE;
		com_searchpaths = NULL;
		while (++i < com_argc)
		{
			if (!com_argv[i] || com_argv[i][0] == '+' || com_argv[i][0] == '-')
				break;

			search = (searchpath_t*)MnemoAllocDbg(sizeof(searchpath_t), __FILE__, __LINE__);
			memset(search, 0, sizeof(searchpath_t));
			if (!strcmp(COM_FileExtension(com_argv[i]), "pak"))
			{
				search->pack = (pack_t*)COM_LoadPackFile(com_argv[i]);
				if (!search->pack)
					Sys_Error("Couldn't load packfile: %s", com_argv[i]);
			}
			else
				strcpy(search->filename, com_argv[i]);
			search->next = com_searchpaths;
			com_searchpaths = search;
		}
	}

	if (COM_CheckParm("-proghack"))
		proghack = TRUE;
}

void COM_FileSeek( int filepos, int filelen, int handle, int pos )
{
	Sys_FileSeek(handle, filepos + pos);
}

int COM_FileTell( int filepos, int filelen, int handle, ... )
{
	return DC_ftell((void*)handle) - filepos;
}

/*
============
COM_LoadBMP
============
*/
qpic_t* COM_LoadBMP( char* filename, int textureMode, int firstRow, int endRow )
{
	int h[3], i, rc = 0;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD rgrgbPalette[256];
	ULONG cbBmpBits, cbPalBytes, biTrueWidth;
	byte *palette, *pb, *pixels, *saved;
	qpic_t *pic, *result = NULL;
	typedef struct { int texnum; float sl, tl, sh, th; } glpic_t;
	glpic_t* gl;
	void (*volatile closeFile)(int, int, int) = COM_CloseFile;

	if (COM_FindFileSearch(NULL, NULL, filename, h, NULL) == -1)
		goto GetOut;

	// Read file header
	if (Sys_FileRead(h[2], &bmfh, sizeof(bmfh)) != sizeof(bmfh))
	{
		rc = -2; goto GetOut;
	}

	// Bogus file header check
	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))
	{
		rc = -2000; goto GetOut;
	}

	// Read info header
	if (Sys_FileRead(h[2], &bmih, sizeof(bmih)) != sizeof(bmih))
	{
		rc = -4; goto GetOut;
	}

	// Bogus info header check
	if (!(bmih.biSize == sizeof(bmih) && bmih.biPlanes == 1))
	{
		rc = -4000; goto GetOut;
	}

	// Bogus bit depth? Only 8-bit supported
	if (bmih.biBitCount != 8)
	{
		rc = -5; goto GetOut;
	}

	// Bogus compression? Only non-compressed supported
	if (bmih.biCompression != BI_RGB)
	{
		rc = -6; goto GetOut;
	}

	// Figure out how many entires are actually in the table
	if (bmih.biClrUsed == 0)
	{
		bmih.biClrUsed = 256;
		cbPalBytes = sizeof(RGBQUAD) * (1 << bmih.biBitCount);
	}
	else
		cbPalBytes = sizeof(RGBQUAD) * bmih.biClrUsed;

	if (Sys_FileRead(h[2], rgrgbPalette, cbPalBytes) != cbPalBytes)
	{
		rc = -8; goto GetOut;
	}

	// convert to a packed 768 byte palette
	palette = MnemoAllocDbg(256 * 3, __FILE__, __LINE__);
	if (palette == NULL)
	{
		rc = -10; goto GetOut;
	}

	pb = palette;
	memset(palette, 0, 256 * 3);

	// Copy over used entries
	for (i = 0; i < (int)bmih.biClrUsed; i++)
	{
		*pb++ = rgrgbPalette[i].rgbRed;
		*pb++ = rgrgbPalette[i].rgbGreen;
		*pb++ = rgrgbPalette[i].rgbBlue;
	}

	cbBmpBits = bmfh.bfSize - COM_FileTell(h[0], h[1], h[2]);
	pb = MnemoAllocDbg(cbBmpBits, __FILE__, __LINE__);
	if (Sys_FileRead(h[2], pb, cbBmpBits) != cbBmpBits)
		return NULL;

	biTrueWidth = (bmih.biWidth + 3) & ~3;
	if (firstRow == 0 && endRow == -1)
	{
		pic = MnemoAllocDbg(cbBmpBits + sizeof(qpic_t) + 2 + 768, __FILE__, __LINE__);
		pic->width = biTrueWidth;
		pic->height = bmih.biHeight;
		pixels = pic->data;
		pb += biTrueWidth * (bmih.biHeight - 1);
		for (i = 0; i < bmih.biHeight; i++)
		{
			memmove(pixels + biTrueWidth * i, pb, biTrueWidth);
			pb -= biTrueWidth;
		}
		free(pb + biTrueWidth);
	}
	else
	{
		int rows = endRow - firstRow;
		saved = pb;
		pic = MnemoAllocDbg(rows * biTrueWidth + sizeof(qpic_t) + 2 + 768, __FILE__, __LINE__);
		pic->width = biTrueWidth;
		pic->height = rows;
		pixels = pic->data;
		pb += biTrueWidth * (bmih.biHeight - firstRow - 1);
		for (i = 0; i < rows; i++)
		{
			memmove(pixels + biTrueWidth * i, pb, biTrueWidth);
			pb -= biTrueWidth;
		}
		free(saved);
	}
	pixels[biTrueWidth * pic->height] = 0;
	pixels[biTrueWidth * pic->height + 1] = 1;
	memcpy(pixels + biTrueWidth * pic->height + 2, palette, 768);
	result = MnemoAllocDbg(sizeof(qpic_t) + sizeof(glpic_t), __FILE__, __LINE__);
	*result = *pic;
	gl = (glpic_t*)result->data;
	gl->texnum = GL_LoadTexture(filename, GLT_SYSTEM, pic->width, pic->height,
		pic->data, FALSE, textureMode, palette);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;
	free(pic);
	free(palette);
GetOut:
	if (h[2] != -1)
		closeFile(h[0], h[1], h[2]);
	return result;
}

void LoadBMP8( int* phFile, byte** pPalette, int* nPalette, byte** pImage )
{
	int i, rc = 0;
	void (*volatile closeFile)(int, int, int) = COM_CloseFile;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD rgrgbPalette[256];
	ULONG cbBmpBits;
	BYTE* pbBmpBits;
	byte* pb;
	ULONG cbPalBytes;
	ULONG biTrueWidth;

	*pImage = NULL;
	*pPalette = NULL;

	// Read file header
	if (Sys_FileRead(phFile[2], &bmfh, sizeof(bmfh)) != sizeof(bmfh))
	{
		rc = -2; goto GetOut;
	}

	// Bogus file header check
	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))
	{
		rc = -2000; goto GetOut;
	}

	// Read info header
	if (Sys_FileRead(phFile[2], &bmih, sizeof(bmih)) != sizeof(bmih))
	{
		rc = -4; goto GetOut;
	}

	// Bogus info header check
	if (!(bmih.biSize == sizeof(bmih) && bmih.biPlanes == 1))
	{
		rc = -4000; goto GetOut;
	}

	// Bogus bit depth? Only 8-bit supported
	if (bmih.biBitCount != 8)
	{
		rc = -5; goto GetOut;
	}

	// Bogus compression? Only non-compressed supported
	if (bmih.biCompression != BI_RGB)
	{
		rc = -6; goto GetOut;
	}

	// Figure out how many entires are actually in the table
	if (bmih.biClrUsed == 0)
	{
		bmih.biClrUsed = 256;
		cbPalBytes = sizeof(RGBQUAD) * (1 << bmih.biBitCount);
	}
	else
		cbPalBytes = sizeof(RGBQUAD) * bmih.biClrUsed;

	if (Sys_FileRead(phFile[2], rgrgbPalette, cbPalBytes) != cbPalBytes)
	{
		rc = -8; goto GetOut;
	}

	// convert to a packed 768 byte palette
	*pPalette = MnemoAllocDbg(256 * 3, __FILE__, __LINE__);
	if (*pPalette == NULL)
	{
		rc = -10; goto GetOut;
	}

	pb = *pPalette;
	memset(*pPalette, 0, 256 * 3);

	// Copy over used entries
	for (i = 0; i < (int)bmih.biClrUsed; i++)
	{
		*pb++ = rgrgbPalette[i].rgbRed;
		*pb++ = rgrgbPalette[i].rgbGreen;
		*pb++ = rgrgbPalette[i].rgbBlue;
	}

	cbBmpBits = bmfh.bfSize - COM_FileTell(phFile[0], phFile[1], phFile[2]);

	// Read bitmap bits (remainder of file)
	pb = MnemoAllocDbg(cbBmpBits, __FILE__, __LINE__);
	if (Sys_FileRead(phFile[2], pb, cbBmpBits) != cbBmpBits)
	{
		return;
	}

	biTrueWidth = (bmih.biWidth + 3) & ~3;
	pbBmpBits = MnemoAllocDbg(cbBmpBits, __FILE__, __LINE__);
	*pImage = pbBmpBits;

	// reverse the order of the data
	pb += (bmih.biHeight - 1) * biTrueWidth;
	for (i = 0; i < bmih.biHeight; i++)
	{
		memmove(&pbBmpBits[biTrueWidth * i], pb, biTrueWidth);
		pb -= biTrueWidth;
	}

	pb += biTrueWidth;
	free(pb);

GetOut:
	if (phFile[2] != -1)
		closeFile(phFile[0], phFile[1], phFile[2]);

	//return rc;
}

/*
============
LoadBMP16
============
*/
byte* LoadBMP16( void* fin, qboolean is15bit )
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	byte *pImage16;

	pImage16 = NULL;

	if (DC_fread(&bmfh, sizeof(bmfh), 1, fin) != 1)
	{
		goto GetOut;
	}

	if (bmfh.bfType != 0x4d42)
	{
		goto GetOut;
	}

	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))
	{
		goto GetOut;
	}

	if (DC_fread(&bmih, sizeof(bmih), 1, fin) != 1)
	{
		goto GetOut;
	}

	if (!(bmih.biSize >= sizeof(bmih) && bmih.biPlanes == 1))
	{
		goto GetOut;
	}

	if (bmih.biCompression != BI_RGB)
	{
		goto GetOut;
	}

	if (bmih.biBitCount == 16)
	{
		DC_fseek(fin, 0, bmfh.bfOffBits);
		pImage16 = Hunk_AllocName(bmih.biSizeImage, "SKYBOX");
		if (pImage16)
		{
			if (DC_fread(pImage16, bmih.biSizeImage, 1, fin) != 1)
			{
				free(pImage16);
				pImage16 = NULL;
			}
		}
	}
	else if (bmih.biBitCount == 8)
	{
		int nPalette;
		byte* pPalette;
		int nImage8;
		byte* pImage8;
		int SizeOfRow8;
		int SizeOfRow16;
		int TrueHeight;
		int nImage16;

		if (bmih.biClrUsed)
			nPalette = bmih.biClrUsed;
		else
			nPalette = 256;

		pPalette = MnemoAllocDbg(nPalette * 4, __FILE__, __LINE__);
		if (!pPalette || DC_fread(pPalette, nPalette * 4, 1, fin) != 1)
		{
			goto GetOut;
		}


		nImage8 = bmih.biSizeImage;

		pImage8 = MnemoAllocDbg(nImage8, __FILE__, __LINE__);

		DC_fseek(fin, 0, bmfh.bfOffBits);

		if (!pImage8)
		{
			goto GetOut;
		}

		if (DC_fread(pImage8, nImage8, 1, fin) != 1)
		{
			free(pImage8);
			free(pPalette);
			goto GetOut;
		}

		SizeOfRow8 = ((bmih.biWidth + 3) / 4) * 4;
		SizeOfRow16 = (bmih.biWidth * 2 + 3) & ~3;
		TrueHeight = abs(bmih.biHeight);
		nImage16 = SizeOfRow16 * TrueHeight;

		pImage16 = Hunk_AllocName(nImage16, "SKYBOX");
		if (!pImage16)
		{
			goto GetOut;
		}

		if (is15bit)
		{

			if (bmih.biHeight <= 0)
			{

				byte* row16 = pImage16;
				byte* row8 = pImage8;
				int nRows = TrueHeight;

				while (nRows--)
				{
					unsigned short* p16 = (unsigned short*)row16;
					byte* p8 = row8;
					int nPixels = bmih.biWidth;

					while (nPixels--)
					{

						short r = pPalette[*p8 * 4 + 2] + RandomLong(0, 3);
						short g = pPalette[*p8 * 4 + 1] + RandomLong(0, 3);
						short b = pPalette[*p8 * 4 + 0] + RandomLong(0, 3);

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						*p16 = PACKEDRGB555(r, g, b);

						p16++;
						p8++;

					}

					row16 += SizeOfRow16;
					row8 += SizeOfRow8;

				}
			}

			else
			{
				byte* row16 = pImage16;
				byte* row8 = &pImage8[nImage8 - SizeOfRow8];
				int nRows = TrueHeight;

				while (nRows--)
				{
					unsigned short* p16 = (unsigned short*)row16;
					byte* p8 = row8;
					int nPixels = bmih.biWidth;

					while (nPixels--)
					{

						short r = pPalette[*p8 * 4 + 2] + RandomLong(0, 3);
						short g = pPalette[*p8 * 4 + 1] + RandomLong(0, 3);
						short b = pPalette[*p8 * 4 + 0] + RandomLong(0, 3);

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						*p16 = PACKEDRGB555(r, g, b);

						p16++;
						p8++;

					}

					row16 += SizeOfRow16;
					row8 -= SizeOfRow8;

				}
			}
		}
		else
		{

			if (bmih.biHeight <= 0)
			{

				byte* row16 = pImage16;
				byte* row8 = pImage8;
				int nRows = TrueHeight;

				while (nRows--)
				{
					unsigned short* p16 = (unsigned short*)row16;
					byte* p8 = row8;
					int nPixels = bmih.biWidth;

					while (nPixels--)
					{

						short r = pPalette[*p8 * 4 + 2] + RandomLong(0, 3);
						short g = pPalette[*p8 * 4 + 1] + RandomLong(0, 3);
						short b = pPalette[*p8 * 4 + 0] + RandomLong(0, 3);

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						*p16 = PACKEDRGB565(r, g, b);

						p16++;
						p8++;

					}

					row16 += SizeOfRow16;
					row8 += SizeOfRow8;

				}
			}

			else
			{
				byte* row16 = pImage16;
				byte* row8 = &pImage8[nImage8 - SizeOfRow8];
				int nRows = TrueHeight;

				while (nRows--)
				{
					unsigned short* p16 = (unsigned short*)row16;
					byte* p8 = row8;
					int nPixels = bmih.biWidth;

					while (nPixels--)
					{

						short r = pPalette[*p8 * 4 + 2] + RandomLong(0, 3);
						short g = pPalette[*p8 * 4 + 1] + RandomLong(0, 3);
						short b = pPalette[*p8 * 4 + 0] + RandomLong(0, 3);

						if (r > 255)
							r = 255;
						if (g > 255)
							g = 255;
						if (b > 255)
							b = 255;

						*p16 = PACKEDRGB565(r, g, b);

						p16++;
						p8++;

					}

					row16 += SizeOfRow16;
					row8 -= SizeOfRow8;

				}
			}

		}
	}

	GetOut:
	Sys_CloseHandle(fin);
	return pImage16;
}

/*
============
COM_Log
============
*/
void COM_Log( char* pszFile, char* fmt, ... )
{
	char string[1024];
	va_list va;
	char* pszFileName;
	void* hFile;

	if (!pszFile)
		pszFileName = "c:\\hllog.txt";
	else
		pszFileName = pszFile;

	va_start(va, fmt);
	vsprintf(string, fmt, va);
	va_end(va);

	hFile = Sys_OpenHandle(pszFileName, "a+t");
	if (hFile)
	{
		Sys_FPrintf(hFile, "%s", string);
		Sys_CloseHandle(hFile);
	}
}

/*
================
COM_ListMaps

Lists all maps matching the substring
If the substring is empty, or "*", then lists all maps
================
*/
void COM_ListMaps( char* pszSubString )
{
	//Con_Printf
	("COM_ListMaps is, um, non-trivial when abbreviated pak-names are involved.\n");
}

/*
===============
COM_ClearCustomizationList

===============
*/
void COM_ClearCustomizationList( customization_t* pHead, qboolean bCleanDecals )
{
	Sys_Error("Customization\n");
}

qboolean COM_CreateCustomization( customization_t* pListHead, resource_t* pResource,
	int playernumber, int flags, customization_t** pCustomization, int* nLumps )
{
	Sys_Error("Customization\n");
	return TRUE;
}

byte* COM_LoadFileForMe( char* path, int* pLength )
{
	return COM_LoadFile(path, 5, pLength);
}

FILE* Sys_FOpenReadSeek( const char* path, int offset )
{
	HANDLE h;

	if (!path)
		return NULL;

	h = Sys_OpenHandle(path, "rb");

	if (h == NULL)
		return NULL;

	SetFilePointer(h, offset, NULL, FILE_BEGIN);
	CloseHandle(h);

	return NULL;
}

void COM_GetGameDir( char* szGameDir )
{
	if (!szGameDir)
		return;

	strcpy(szGameDir, com_gamedir);
}

/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
void COM_FixSlashes( char* pname )
{
#ifdef _WIN32
	while (*pname)
	{
		if (*pname == '/')
			*pname = '\\';
		pname++;
	}
#else
	while (*pname)
	{
		if (*pname == '\\')
			*pname = '/';
		pname++;
	}
#endif
}

/*
============
COM_StringToLower

Lowercases a string in place, returning a pointer to its terminator.
============
*/
char* COM_StringToLower( char* string )
{
	while (*string)
	{
		*string = tolower(*string);
		string++;
	}
	return string;
}

/*
============
COM_BuildFileList

Collect every copy of a file across the search paths into a linked list,
opening each and recording its length.  Returns the number found.
============
*/
int COM_BuildFileList( char* filename, FileList_t** ppList )
{
	int				count;
	int				len;
	int*			handles;
	searchpath_t*	restart;
	FileList_t*		entry;

	count = 0;
	restart = NULL;

	if (!ppList)
		return 0;

	*ppList = NULL;

	while (1)
	{
		handles = (int*)MnemoAllocDbg(3 * sizeof(int), __FILE__, __LINE__);
		memset(handles, 0, 3 * sizeof(int));

		len = COM_FindFileSearch(&restart, NULL, filename, handles, NULL);
		if (len == -1)
			break;

		entry = (FileList_t*)MnemoAllocDbg(sizeof(FileList_t), __FILE__, __LINE__);
		memset(entry, 0, sizeof(FileList_t));
		entry->fileLen = len;
		entry->handles = handles;
		strcpy(entry->pathID, restart->gamedir);
		count++;
		entry->next = *ppList;
		*ppList = entry;
	}

	free(handles);
	return count;
}

/*
============
COM_CloseUnusedFiles

Close the handle of every listed file that isn't held open by a mounted pack.
============
*/
void COM_CloseUnusedFiles( FileList_t* list )
{
	FileList_t*		cur;
	searchpath_t*	search;
	qboolean		inuse;

	if (!list)
		return;

	for (cur = list; cur; cur = cur->next)
	{
		inuse = FALSE;

		for (search = com_searchpaths; search; search = search->next)
		{
			if (search->pack && search->pack->handle == cur->handles[2])
			{
				inuse = TRUE;
				break;
			}
		}

		if (!inuse)
			Sys_FileClose(cur->handles[2]);
	}
}

/*
============
COM_DestroyMultipleFileList

Release a directory listing and reset the caller's head pointer.
============
*/
void COM_DestroyMultipleFileList( FileList_t** ppList )
{
	FileList_t*	cur;
	FileList_t*	next;

	if (!ppList)
		return;

	for (cur = *ppList; cur; cur = next)
	{
		next = cur->next;
		if (cur->handles)
			free(cur->handles);
		free(cur);
	}

	*ppList = NULL;
}

/*
============
COM_ExpandFilename

Look for a file across the search paths.  If found, rewrites the caller's
buffer with the full path and returns true.
============
*/
int COM_ExpandFilename( char* filename )
{
	searchpath_t*	search;
	char			netpath[MAX_OSPATH];

	for (search = com_searchpaths; search; search = search->next)
	{
		if (!search->filename[0])
			continue;

		sprintf(netpath, "%s/%s", search->filename, filename);

		if (Sys_FileTime(netpath) > 0)
		{
			strcpy(filename, netpath);
			return TRUE;
		}
	}

	return FALSE;
}

int Q_FileNameSuffixCmp( char* suffix, char* filename )
{
	char name[260];
	char* out = name;
	char* tail;
	int c;

	while (*filename)
	{
		c = tolower(*filename++);
		if (c == '\\')
			c = '/';
		*out++ = c;
	}
	*out = 0;
	tail = strchr(name, 0) - strlen(suffix);
	if (tail < name)
		tail = name;
	return strcmp(suffix, tail);
}

char* COM_BinPrintf( byte* buf, int nLen )
{
	static char text[128];
	char chunk[10];
	int i;

	memset(text, 0, sizeof(text));
	for (i = 0; i < nLen; i++)
	{
		sprintf(chunk, "%02x", buf[i]);
		strcat(text, chunk);
	}
	return text;
}

byte* COM_LoadFileLimitIntoBuffer( char* path, int pos, int cbmax, int* pcbread, int* phFile, byte* dest )
{
	int             h[3];
	void (*volatile seekFile)(int, int, int, int) = COM_FileSeek;
	void (*volatile closeFile)(int, int, int) = COM_CloseFile;
	char    base[32];
	int             len;

	if (phFile[2] == -1)
	{
	// look for it in the filesystem or pack files
		len = COM_OpenFile(path, h);
		h[1] = com_filesize;
	}
	else
	{
		h[0] = phFile[0];
		h[1] = phFile[1];
		h[2] = phFile[2];
		len = h[1];
	}

	if (h[2] == -1)
		return NULL;

	if (pos > len)
		Sys_Error("COM_LoadFileLimit: invalid seek position for %s", path);

	seekFile(h[0], h[1], h[2], pos);

	if (len > cbmax)
		len = cbmax;

	*pcbread = len;

	if (path)
		COM_FileBase(path, base);

	if (!dest)
	{
		if (path)
			Sys_ErrorColor(0xf800, "COM_LoadFileLimit: not enough space for %s", path);
		closeFile(h[0], h[1], h[2]);
		return NULL;
	}

	dest[len] = 0;
	len = Sys_FileRead(h[2], dest, len);
	phFile[0] = h[0];
	phFile[1] = h[1];
	phFile[2] = h[2];
	*pcbread = len;
	return dest;
}

void COM_WriteFile( char* filename, void* data, int length )
{
	char name[MAX_OSPATH];
	int handle;

	sprintf(name, "%s/%s", com_gamedir, filename);
	handle = Sys_FileOpenWriteLegacy(name);
	if (handle != -1)
	{
		Sys_FileWrite(handle, data, length);
		Sys_FileClose(handle);
	}
}

void COM_CopyFile( char* netpath, char* cachepath )
{
	int in, out;
	unsigned int remaining, count;
	char buffer[4096];

	remaining = Sys_FileOpenRead(netpath, &in, FALSE);
	COM_CreatePath(cachepath);
	out = Sys_FileOpenWriteLegacy(cachepath);
	while (remaining)
	{
		count = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
		Sys_FileRead(in, buffer, count);
		Sys_FileWrite(out, buffer, count);
		remaining -= count;
	}
	Sys_FileClose(in);
	Sys_FileClose(out);
}

void COM_CopyFileChunk( void* dest, void* source, int size )
{
	char buffer[1024];

	while (size > 1024)
	{
		DC_fread(buffer, sizeof(buffer), 1, source);
		DC_fwrite(buffer, sizeof(buffer), 1, dest);
		size -= sizeof(buffer);
	}
	DC_fread(buffer, size, 1, source);
	DC_fwrite(buffer, size, 1, dest);
	Sys_Unimplemented("fflush");
	Sys_Unimplemented("fflush");
}

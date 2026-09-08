// Minimal CRT shims for Dreamcast WinCE builds.
// Only functions actually missing from the WinCE C runtime should live here.
#ifdef _WIN32_WCE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "dreamcast_crt.h"

extern "C" void* MnemoAllocDbg( int size, const char* srcFile, int srcLine );

#define MNEMO_FLAG_MALLOC 0x0020

extern "C" {

int _strnicmp( const char* s1, const char* s2, unsigned int n )
{
	if ( !s1 || !s2 || n == 0 )
		return 0;

	const unsigned char* p1 = (const unsigned char*)s1;
	const unsigned char* p2 = (const unsigned char*)s2;

	while ( n-- )
	{
		int c1 = tolower( *p1++ );
		int c2 = tolower( *p2++ );

		if ( c1 != c2 || c1 == 0 || c2 == 0 )
			return c1 - c2;
	}

	return 0;
}

int _stricmp( const char* s1, const char* s2 )
{
	if ( !s1 || !s2 )
		return 0;

	const unsigned char* p1 = (const unsigned char*)s1;
	const unsigned char* p2 = (const unsigned char*)s2;

	for ( ;; )
	{
		int c1 = tolower( *p1++ );
		int c2 = tolower( *p2++ );

		if ( c1 != c2 || c1 == 0 || c2 == 0 )
			return c1 - c2;
	}
}


// The C heap free() overrides coredll's and routes through the arena allocator.
// (The exe link uses /FORCE:MULTIPLE so this wins over coredll's free; per-module
// symbol resolution means other DLLs keep their own free.)
extern void MnemoFree( void* ptr );
extern void* MnemoAlloc( int size, unsigned int flags, int allocClass, const char* tag );
extern int Mnemo_BlockSize( void* ptr );
extern void Sys_Error( char* error, ... );
char* strrchr( const char* s, int c );

void free( void* ptr )
{
	MnemoFree( ptr );
}

#pragma inline_depth(0)
void* DebugRealloc( void* oldPtr, unsigned int sizeBytes, const char* srcFile, int srcLine )
{
	static char tag[MAX_PATH];
	const char* base;
	unsigned int copyBytes;
	void* newPtr;

	if ( sizeBytes == 0 )
	{
		MnemoFree( oldPtr );
		return NULL;
	}

	if ( oldPtr == NULL )
	{
		base = strrchr( srcFile, '\\' );
		if ( !base )
			base = strrchr( srcFile, '/' );
		if ( base )
			base++;
		else
			base = srcFile;

		sprintf( tag, "%d %s", srcLine, base );
		return MnemoAlloc( sizeBytes, MNEMO_FLAG_MALLOC, 0, tag );
	}

	base = strrchr( srcFile, '\\' );
	if ( !base )
		base = strrchr( srcFile, '/' );
	if ( base )
		base++;
	else
		base = srcFile;

	sprintf( tag, "%d %s", srcLine, base );
	newPtr = MnemoAlloc( sizeBytes, MNEMO_FLAG_MALLOC, 0, tag );
	copyBytes = Mnemo_BlockSize( oldPtr );

	if ( !newPtr )
	{
		Sys_Error( "Realloc failed." );
	}
	else
	{
		if ( copyBytes > sizeBytes )
			copyBytes = sizeBytes;

		memcpy( newPtr, oldPtr, copyBytes );
		MnemoFree( oldPtr );
	}

	return newPtr;
}
#pragma inline_depth(255)

// Case-insensitive
// Simple bsearch implementation for environments without a CRT bsearch.
void* bsearch( const void* key, const void* base, unsigned int num, unsigned int width,
                int (__cdecl *compare)(const void*, const void*) )
{
	unsigned int cur;
	const char* p;
	int r;

	while ( 1 )
	{
		do
		{
			cur = num;
			if ( cur == 0 )
				return NULL;
			num = cur >> 1;
			p = (const char*)base + num * width;
			r = compare( key, p );
		} while ( r < 0 );

		if ( r == 0 )
			return (void*)p;

		base = p + width;
		num = cur - ( num + 1 );
	}
}

// Character classification implementations.
int isalpha( int c )
{
	if ( c >= 'A' && c <= 'Z' )
		return 1;
	if ( c >= 'a' && c <= 'z' )
		return 1;
	return 0;
}

int isdigit( int c )
{
	if ( c >= '0' && c <= '9' )
		return 1;
	return 0;
}

int isspace( int c )
{
	if ( ' ' != c && '\f' != c && '\n' != c && '\r' != c && '\t' != c && '\v' != c )
		return 0;
	return 1;
}

int isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7e )
		return 1;
	return 0;
}

// String shims.
char* _strdup( const char* s )
{
	int len = strlen( s );
	char* out = (char*)MnemoAllocDbg( len + 1, __FILE__, __LINE__ );
	if ( out )
		strcpy( out, s );
	return out;
}

int Sys_SampleCount( void )
{
	return -1;
}

int DC_SetFileBuffering( void* stream, char* buffer, int mode, int size )
{
	return 0;
}

char* strrchr( const char* s, int c )
{
	const char ch = (char)c;
	char* last = NULL;

	for ( ;; s++ )
	{
		if ( *s == ch )
			last = (char*)s;
		if ( *s == '\0' )
			return last;
	}
}


void DC_ExtractFilename( const char *path, char *name )
{
	const char *slash = strrchr(path, '/');
	const char *drive = strrchr(path, ':');
	const char *ext = strrchr(path, '.');

	if (!ext)
		ext = strchr(path, 0);
	if (slash)
		path = slash + 1;
	else if (drive)
		path = drive + 1;
	while (path < ext)
		*name++ = *path++;
	*name = 0;
}

void _splitpath( const char *path, char *drive, char *dir, char *name, char *ext )
{
	char normalized[MAX_PATH];
	char *out = normalized;
	char *first, *last;

	while (*path)
	{
		*out++ = *path == '\\' ? '/' : *path;
		path++;
	}
	if (drive)
	{
		first = strchr(normalized, ':');
		if (first)
		{
			drive[0] = first[-1];
			drive[1] = first[0];
			drive[2] = 0;
		}
		else
			strcpy(drive, "");
	}
	if (dir)
	{
		first = strchr(normalized, '/');
		last = strrchr(normalized, '/');
		if (first)
		{
			do
			{
				*dir++ = *first++;
			} while (first <= last);
			*dir = 0;
		}
		else
			strcpy(dir, "");
	}
	if (name)
		DC_ExtractFilename(normalized, name);
	if (ext)
	{
		last = strrchr(normalized, '.');
		if (last)
			strcpy(ext, last);
		else
			strcpy(ext, "");
	}
}

// Single-precision floating point remainder.
float fmodf( float x, float y )
{
	return x - (float)(int)(x / y) * y;
}

// Time shim.
long time( long* t )
{
	return 0;
}

struct tm* localtime( const long* t )
{
	static struct tm result;

	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = 1;
	result.tm_mon = 0;
	result.tm_year = 70;
	result.tm_wday = 4;
	result.tm_yday = 0;
	result.tm_isdst = 0;
	return &result;
}

// Case-insensitive string compares (WinCE lacks these); tolower is resolved
// through the CRT so the DC build shares one implementation.
int Q_stricmp( const char* s1, const char* s2 )
{
	while (*s1)
	{
		if (tolower(*s1) != tolower(*s2))
			break;
		s1++;
		s2++;
	}

	if (tolower(*s1) < tolower(*s2))
		return -1;
	return (tolower(*s2) < tolower(*s1));
}

int Q_strnicmp( char* s1, char* s2, int n )
{
	while (*s1)
	{
		if (tolower(*s1) != tolower(*s2))
			break;
		if (--n == 0)
			break;
		s1++;
		s2++;
	}

	if (tolower(*s1) < tolower(*s2))
		return -1;
	return (tolower(*s2) < tolower(*s1));
}

} // extern "C"

#endif // _WIN32_WCE

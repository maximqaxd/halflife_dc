// Minimal CRT shims for Dreamcast WinCE builds.
// Only functions actually missing from the WinCE C runtime should live here.
#ifdef _WIN32_WCE
#include <windows.h>
#include <string.h>

extern "C" void* MnemoAllocDbg( int size, const char* srcFile, int srcLine );

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
void free( void* ptr )
{
	MnemoFree( ptr );
}

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


// Time shim.
long time( long* t )
{
	// Approximate time as seconds since boot; good enough for seeding, etc.
	unsigned long ms = GetTickCount();
	long secs = (long)(ms / 1000UL);
	if ( t )
		*t = secs;
	return secs;
}

// Case-insensitive string compares (WinCE lacks these); tolower is resolved
// through the CRT so the DC build shares one implementation.
int Q_stricmp( char* s1, char* s2 )
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

// Route all dynamic allocation through the game's memory manager instead of the
// WinCE C runtime heap, so engine, game, and these CRT shims share one allocator.
extern "C" void* MnemoAlloc( int size, unsigned int flags, int allocClass, const char* tag );
extern "C" void  MnemoFree( void* ptr );

void* operator new( unsigned int size )
{
	return MnemoAlloc( (int)size, 0x20, 0, "op new" );
}

void operator delete( void* ptr )
{
	MnemoFree( ptr );
}

#endif // _WIN32_WCE
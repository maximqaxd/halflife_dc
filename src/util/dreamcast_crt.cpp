// Minimal CRT shims for Dreamcast WinCE builds.
// Only functions actually missing from the WinCE C runtime should live here.
#ifdef _WIN32_WCE
#include <windows.h>
#include <string.h>

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

// WinCE file shims

int _unlink( const char* path )
{
	if ( !path )
		return -1;

	TCHAR wszPath[MAX_PATH];
	wszPath[0] = 0;
	MultiByteToWideChar( CP_ACP, 0, path, -1, wszPath, MAX_PATH );

	if ( DeleteFile( wszPath ) )
		return 0;

	return -1;
}

int rename( const char* oldname, const char* newname )
{
	if ( !oldname || !newname )
		return -1;

	TCHAR wszOld[MAX_PATH];
	TCHAR wszNew[MAX_PATH];

	wszOld[0] = 0;
	wszNew[0] = 0;

	MultiByteToWideChar( CP_ACP, 0, oldname, -1, wszOld, MAX_PATH );
	MultiByteToWideChar( CP_ACP, 0, newname, -1, wszNew, MAX_PATH );

	if ( MoveFile( wszOld, wszNew ) )
		return 0;

	return -1;
}

void* calloc( unsigned int num, unsigned int size )
{
	const unsigned int total = num * size;
	if ( !total )
		return 0;

	// Rely on existing malloc; zero the block ourselves.
	extern void* malloc( unsigned int );

	void* p = malloc( total );
	if ( p )
	{
		memset( p, 0, total );
	}
	return p;
}

// Case-insensitive 
int _strcmpi( const char* s1, const char* s2 )
{
	return _stricmp( s1, s2 );
}

// Simple bsearch implementation for environments without a CRT bsearch.
void* bsearch( const void* key, const void* base, unsigned int num, unsigned int width,
                int (__cdecl *compare)(const void*, const void*) )
{
	const unsigned char* lo = (const unsigned char*)base;
	const unsigned char* hi = lo + (num ? (num - 1) * width : 0);

	while ( num && lo <= hi )
	{
		const unsigned char* mid = lo + ((hi - lo) / width / 2) * width;
		int cmp = compare( key, mid );
		if ( cmp == 0 )
			return (void*)mid;
		if ( cmp < 0 )
		{
			if ( mid == lo )
				break;
			hi = mid - width;
		}
		else
		{
			lo = mid + width;
		}
	}

	return NULL;
}

// Character classification implementations.
int isalnum( int c )
{
	return ( (c >= '0' && c <= '9') ||
	         (c >= 'A' && c <= 'Z') ||
	         (c >= 'a' && c <= 'z') );
}

int isalpha( int c )
{
	return ( (c >= 'A' && c <= 'Z') ||
	         (c >= 'a' && c <= 'z') );
}

int isdigit( int c )
{
	return (c >= '0' && c <= '9');
}

int isspace( int c )
{
	return (c == ' '  || c == '\t' ||
	        c == '\n' || c == '\r' ||
	        c == '\f' || c == '\v');
}

int isprint( int c )
{
	return (c >= 0x20 && c <= 0x7e);
}

// String shims.
char* _strdup( const char* s )
{
	if ( !s )
		return NULL;

	const size_t len = strlen( s ) + 1;
	char* out = (char*)malloc( len );
	if ( out )
		memcpy( out, s, len );
	return out;
}

char* _strlwr( char* s )
{
	if ( !s )
		return NULL;

	for ( char* p = s; *p; ++p )
		*p = (char)tolower( (unsigned char)*p );

	return s;
}


void _splitpath( const char* path,
                 char* drive, char* dir, char* fname, char* ext )
{
	// We ignore drive on Dreamcast (\Device\CDROM0 etc.), so always empty.
	if ( drive )
		drive[0] = '\0';

	if ( !path )
	{
		if ( dir )   dir[0] = '\0';
		if ( fname ) fname[0] = '\0';
		if ( ext )   ext[0] = '\0';
		return;
	}

	const char* lastSlash = NULL;
	const char* lastDot   = NULL;

	for ( const char* p = path; *p; ++p )
	{
		if ( *p == '\\' || *p == '/' )
			lastSlash = p;
		else if ( *p == '.' )
			lastDot = p;
	}

	const char* nameStart = lastSlash ? lastSlash + 1 : path;
	const char* extStart  = (lastDot && lastDot > nameStart) ? lastDot : NULL;

	if ( dir )
	{
		if ( lastSlash )
		{
			size_t len = (size_t)(lastSlash - path + 1); // include trailing slash
			memcpy( dir, path, len );
			dir[len] = '\0';
		}
		else
		{
			dir[0] = '\0';
		}
	}

	if ( fname )
	{
		const char* end = extStart ? extStart : path + strlen( path );
		size_t len = (size_t)(end - nameStart);
		memcpy( fname, nameStart, len );
		fname[len] = '\0';
	}

	if ( ext )
	{
		if ( extStart )
			strcpy( ext, extStart );
		else
			ext[0] = '\0';
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

FILE* tmpfile( void )
{

	return NULL;
}

int setvbuf( void* stream, char* buffer, int mode, unsigned int size )
{
	(void)stream;
	(void)buffer;
	(void)mode;
	(void)size;
	return 0;
}

// Generic float-to-long converter used via quick_ftol macro / compiler helper.
long ftol( double f )
{
	return (long)f;
}

unsigned long timeGetTime( void )
{
	return GetTickCount();
}

// TODO: remove that
unsigned int joyGetNumDevs( void )
{
	return 0;
}

// Simple integer to string conversion; supports at least base 10 for sound.cpp usage.
char* itoa( int value, char* str, int base )
{
	static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char buf[32];
	int  i = 0;
	int  negative = 0;
	unsigned int v;

	if ( !str || base < 2 || base > 36 )
		return str;

	if ( value < 0 && base == 10 )
	{
		negative = 1;
		v = (unsigned int)(-value);
	}
	else
	{
		v = (unsigned int)value;
	}

	do
	{
		buf[i++] = digits[v % (unsigned int)base];
		v /= (unsigned int)base;
	} while ( v && i < (int)sizeof(buf) - 1 );

	if ( negative )
		buf[i++] = '-';

	// reverse into output
	{
		int j;
		int k = 0;
		for ( j = i - 1; j >= 0; --j )
			str[k++] = buf[j];
		str[k] = '\0';
	}

	return str;
}

} // extern "C"

#endif // _WIN32_WCE
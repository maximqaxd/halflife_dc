#ifndef DREAMCAST_CRT_H
#define DREAMCAST_CRT_H

#ifndef HLDC_MP
#define HLDC_MP 0
#endif

//
// Opt-in corrections for defects that shipped in the Dreamcast game.
//
// The engine reproduces the original behaviour by default, so a build with
// HLDC_FIXES left at 0 behaves exactly like the released game. Set it to 1 to
// get a corrected engine instead.
//
// Every guarded site names the symptom it produces in the game, so a fix can
// be turned off again when tracking down a difference in behaviour.
//
#ifndef HLDC_FIXES
#define HLDC_FIXES 0
#endif

#if HLDC_MP
#undef HLDC_FIXES
#define HLDC_FIXES 1
#endif

#ifdef _WIN32_WCE

// Pull the C runtime headers in first. The shims below deliberately shadow a
// few of their declarations, and that only works if the originals are already
// in scope - otherwise the runtime's copy arrives second and collides.
#include <stdlib.h>

// The FPU runs fixed in single precision. floatmathlib redirects the math entry
// points at the single-precision routines, so a call costs a handful of float
// instructions instead of dragging in the software double helpers.
#include <floatmathlib.h>

#ifndef _INC_STDLIB
typedef unsigned int size_t;
#endif

#define ARRAYSIZE(a)         (sizeof(a)/sizeof(a[0]))
#define offsetof(s,m)        ((size_t)&(((s*)0)->m))
#define stricmp _stricmp
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define _IOFBF 0

#ifdef __cplusplus
extern "C" {
#endif

/* WinCE/Dreamcast CRT shims */

int _strnicmp( const char *s1, const char *s2, unsigned int n );
int _stricmp( const char *s1, const char *s2 );
int Q_stricmp( const char* s1, const char* s2 );
int Q_strnicmp( char* s1, char* s2, int n );

// The Dreamcast heap allocators tag each block with its call site and route
// through the Mnemo arena. Declaring them here replaces the C runtime's
// imported entry points, so a call lands on the arena directly instead of
// going out through coredll; calloc also takes the call site, so each block
// records where it came from.
void* calloc( unsigned int num, unsigned int size, const char* file, int line );
#define calloc( n, s )	calloc( (n), (s), __FILE__, __LINE__ )

#pragma warning( push )
#pragma warning( disable : 4273 )
void  free( void* ptr );
#pragma warning( pop )

void* bsearch( const void* key, const void* base, unsigned int num, unsigned int width,
               int (__cdecl *compare)(const void*, const void*) );

int isalpha( int c );
int isdigit( int c );
int isspace( int c );
int isprint( int c );

char* _strdup( const char *s );
char* strrchr( const char *s, int c );
void _splitpath( const char *path, char *drive, char *dir, char *name, char *ext );

int DC_SetFileBuffering( void* stream, char* buffer, int mode, int size );
int _unlink( const char* path );

long time( long* t );

#ifndef _TM_DEFINED
struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};
#define _TM_DEFINED
#endif

struct tm* localtime( const long* t );

// floatmathlib spells fabs as a compare and a negate; the single-precision
// instruction does it in one, so take that instead.
#undef fabs
float fabsf( float x );
#define fabs( x )		fabsf( (x) )

#ifdef __cplusplus
}
#endif

#endif /* WIN32_WCE */

#endif /* DREAMCAST_CRT_H */

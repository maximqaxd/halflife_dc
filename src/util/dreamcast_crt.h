#ifndef DREAMCAST_CRT_H
#define DREAMCAST_CRT_H


#ifdef _WIN32_WCE

// Pull the C runtime headers in first. The shims below deliberately shadow a
// few of their declarations, and that only works if the originals are already
// in scope - otherwise the runtime's copy arrives second and collides.
#include <stdlib.h>

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

// The Dreamcast heap allocators tag each block with its call site and route
// through the Mnemo arena. Declaring them here replaces the C runtime's
// imported entry points, so a call lands on the arena directly instead of
// going out through coredll; calloc also takes the call site, so each block
// records where it came from.
void* calloc( unsigned int num, unsigned int size, const char* file, int line );
#define calloc( n, s )	calloc( (n), (s), __FILE__, __LINE__ )

void  free( void* ptr );

void* bsearch( const void* key, const void* base, unsigned int num, unsigned int width,
               int (__cdecl *compare)(const void*, const void*) );

int isalpha( int c );
int isdigit( int c );
int isspace( int c );
int isprint( int c );

char* _strdup( const char *s );
char* strrchr( const char *s, int c );

long time( long* t );

// The FPU runs fixed in single precision, but <stdlib.h> only declares the double
// form of fmod - calling that drags in the software double-precision helpers for
// what is a handful of float instructions. Route it through the single version.
float fmodf( float x, float y );
#define fmod( x, y )	fmodf( (x), (y) )

#ifdef __cplusplus
}
#endif

#endif /* WIN32_WCE */

#endif /* DREAMCAST_CRT_H */


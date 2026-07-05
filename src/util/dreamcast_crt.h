#ifndef DREAMCAST_CRT_H
#define DREAMCAST_CRT_H


#ifdef _WIN32_WCE

typedef unsigned int size_t;

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
int _strcmpi( const char *s1, const char *s2 );

void* calloc( unsigned int num, unsigned int size );
void* bsearch( const void* key, const void* base, unsigned int num, unsigned int width,
               int (__cdecl *compare)(const void*, const void*) );

int isalnum( int c );
int isalpha( int c );
int isdigit( int c );
int isspace( int c );
int isprint( int c );

char* _strdup( const char *s );
char* _strlwr( char *s );
char* strrchr( const char *s, int c );
void  _splitpath( const char* path, char* drive, char* dir, char* fname, char* ext );

long time( long* t );
unsigned long timeGetTime( void );
unsigned int  joyGetNumDevs( void );

char* itoa( int value, char* str, int base );
#ifdef __cplusplus
}
#endif

#endif /* WIN32_WCE */

#endif /* DREAMCAST_CRT_H */


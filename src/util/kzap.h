#ifndef KZAP_H
#define KZAP_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void KZapInitCodecs( int maxarg );
int  KZapCompress( byte *src, byte *dst, int len );
int  KZapDecompress( byte *src, byte *dst );
int  ZlibCompress( byte *src, byte *dst, int len );
int  ZlibDecompress( byte *src, byte *dst );

// In-memory buffered file used by the Dreamcast asset/save I/O (util/Zap.cpp).
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

int   Bopen( char *path, char *mode );
int   Bclose( bfile_t *h );
int   Bread( void *buffer, int size, int count, bfile_t *h );
int   Bwrite( void *buffer, int size, int count, bfile_t *h );
int   Bremove_path( char *path );
int   Bfilesize_path( char *path );
int   Brename_path( char *oldpath, char *newpath );
int   Bcompress_path( char *path );
int   Bexport_path( char *path );

// Wildcard search over the resident files. Each call returns the next matching
// name, or NULL when the list runs out.
char *Bfind_first( char *pattern, char *nameOut );
char *Bfind_next( char *nameOut );
void  Bfind_reset( void );

// heap wrappers shared with the zlib glue (util/zapsave.c)
void *mallocx( int size );
void  freex( void *ptr );

// shrink every resident Zap/BFile texture to reclaim arena space
void Bshrink_all( void );

#ifdef __cplusplus
}
#endif

#endif // KZAP_H

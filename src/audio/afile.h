// afile.h -- AFile: sound resources cached in DirectSound hardware buffers.
//
// An AFile record holds a chain of DirectSound secondary buffers (audio RAM).
// A resource is streamed into those buffers in chunks and copied back out on
// demand. The cache system juggles sprite/model VQ data through this same audio
// memory, so the cache-free path stages a resource out via AFile.

#ifndef AFILE_H
#define AFILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct afile_s afile_t;

// Find a cached AFile by resource name, or NULL if not resident.
afile_t* AFile_FindByName( const char* name );

// Total byte size of the resource.
int AFile_GetSize( afile_t* af );

// Copy the resource into dest, walking its buffer chain. Returns bytes read.
int AFile_Read( afile_t* af, void* dest, int size );

// Copy size bytes starting at offset into dest. Returns bytes read.
int AFile_ReadOffset( afile_t* af, void* dest, int offset, int size );

// Release the AFile and its DirectSound buffers.
void AFile_Free( afile_t* af );

// Find an AFile by name, or create one and stream size bytes of data into its
// DirectSound buffers. Used to juggle cache data out of the arena. Returns the
// AFile, or NULL if it could not be created.
afile_t* AFile_FindOrCreate( const char* name, void* data, int size, int create );

#ifdef __cplusplus
}
#endif

#endif // AFILE_H

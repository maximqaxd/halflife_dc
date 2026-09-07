// afile.h -- resources parked in audio RAM
//
// The Dreamcast has far more sound RAM than the cache system can spare in main
// memory, so cache blocks that are not being touched get streamed out into a
// chain of static DirectSound buffers and read back on demand. Each resource
// parked this way is described by an afile_t.

#ifndef AFILE_H
#define AFILE_H

// Slots in the AFile table.
#define MAX_AFILES		64

// A resource is split over this many DirectSound buffers, so the largest
// resource that fits is AFILE_BLOCKS * AFILE_BLOCKSIZE bytes.
#define AFILE_BLOCKS		12
#define AFILE_BLOCKSIZE		50000

// Never fill audio RAM completely -- leave this much for real sounds.
#define AFILE_RESERVE		204800

struct IDirectSoundBuffer;

typedef struct afile_s
{
	int							id;		// serial number, for the afilelist report
	int							size;		// bytes actually written into the blocks
	int							usage;		// carried for the cache system
	struct IDirectSoundBuffer*	blocks[AFILE_BLOCKS];
	char						name[MAX_SOUND_NAME];
} afile_t;

#ifdef __cplusplus
extern "C" {
#endif

void		AFile_Init( void );
void		AFile_FlushCache( void );
void		AFile_EvictCache( void );

// Free audio RAM as reported by the driver, and whether a resource of the
// given size still leaves the reserve intact.
int			AFile_FreeSoundRam( void );
int			AFile_MaxContiguousSoundRam( void );
qboolean	AFile_HasRoomFor( int size );

// Total bytes of all resident resources.
int			AFile_TotalCachedBytes( void );

afile_t*	AFile_FindByName( char* name );
void		AFile_PrintList( void* fileid );

// Find the resource, or park a copy of it in audio RAM. Returns NULL if the
// table is full or the buffers could not be created.
afile_t*	AFile_LoadOrCreate( char* name, byte* data, int size, int usage );

// Read the resource back out. Returns the number of bytes copied.
int			AFile_ReadBlocks( afile_t* af, byte* dest, int size );
int			AFile_ReadBlocksOffset( afile_t* af, byte* dest, int offset, int size );

int			AFile_GetSize( afile_t* af );
void		AFile_Free( afile_t* af );
void		AFile_Touch( afile_t* af );

void		AFile_CopyWords( void* dest, void* src, int count );

void		Cmd_afilelist_f( void );

#ifdef __cplusplus
}
#endif

#endif // AFILE_H

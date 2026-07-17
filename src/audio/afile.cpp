// afile.cpp -- AFile: packed sound files resident in SPU/CE memory
//
// Reconstruction in progress: functions are added here as they are
// identified in the reference binary. The block-chain internals live in the
// audio subsystem; for now these are placeholders so the cache system links.

#include "quakedef.h"
#include "afile.h"

afile_t* AFile_FindByName( const char* name )
{
	return NULL;
}

int AFile_GetSize( afile_t* af )
{
	if (!af)
		return 0;

	return 0;
}

int AFile_Read( afile_t* af, void* dest, int size )
{
	return 0;
}

int AFile_ReadOffset( afile_t* af, void* dest, int offset, int size )
{
	return 0;
}

void AFile_Free( afile_t* af )
{
}

afile_t* AFile_FindOrCreate( const char* name, void* data, int size, int create )
{
	return NULL;
}

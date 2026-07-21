// hashpak.c - HPAK system, handles resources in a compressed form

#include "quakedef.h"
#include "custom.h"
#include "hashpak.h"

/*
=================
HPAK_GetDataPointer

=================
*/
qboolean HPAK_GetDataPointer( char* pakname, resource_t* pResource, void** ppbuffer, int* pnsize )
{
	Sys_Error("Customization\n");
	return FALSE;
}

/*
=================
HPAK_FlushHostQueue

=================
*/
void HPAK_FlushHostQueue( void )
{
	Sys_Error("Customization\n");
}

/*
=================
HPAK_AddLump

=================
*/
void HPAK_AddLump( char* pakname, resource_t* pResource, void* pData, FILE* fpSource )
{
	Sys_Error("Customization\n");
}

/*
=================
HPAK_RemoveLump

=================
*/
void HPAK_RemoveLump( char* pakname, resource_t* pResource )
{
	Sys_Error("Customization\n");
}

/*
=================
HPAK_ResourceForIndex

=================
*/
qboolean HPAK_ResourceForIndex( char* pakname, int nIndex, resource_t* pResource )
{
	Sys_Error("Customization\n");
	return FALSE;
}

/*
=================
HPAK_ResourceForHash

=================
*/
qboolean HPAK_ResourceForHash( char* pakname, byte* hash, resource_t* pResourceEntry )
{
	Sys_Error("Customization\n");
	return FALSE;
}

// Z_zone.c

#include "quakedef.h"
#include "winquake.h"
#include "studio.h"

#define	DYNAMIC_SIZE	0xc000

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64


/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

memzone_t* mainzone;

void Z_ClearZone( memzone_t* zone, int size );


/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t* zone, int size )
{
	memblock_t* block;

// set the entire zone to one free block
	zone->size = size;

	zone->blocklist.next = zone->blocklist.prev = block =
		(memblock_t*)((byte*)zone + sizeof(memzone_t));
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}


/*
========================
Z_Free
========================
*/
void Z_Free( void* ptr )
{
	memblock_t* block, * other;

	if (!ptr)
		Sys_Error("Z_Free: NULL pointer");

	block = (memblock_t*)((byte*)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
		Sys_Error("Z_Free: freed a pointer without ZONEID");
	if (block->tag == 0)
		Sys_Error("Z_Free: freed a freed pointer");

	block->tag = 0;		// mark as free

	other = block->prev;
	if (!other->tag)
	{	// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == mainzone->rover)
			mainzone->rover = other;
		block = other;
	}

	other = block->next;
	if (!other->tag)
	{	// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == mainzone->rover)
			mainzone->rover = block;
	}
}


/*
========================
Z_Malloc
========================
*/
void* Z_Malloc( int size )
{
	void* buf;
	Z_CheckHeap();	// DEBUG
	buf = Z_TagMalloc(size, 1);
	if (!buf)
		Sys_Error("Z_Malloc: failed on allocation of %i bytes", size);
	Q_memset(buf, 0, size);

	return buf;
}

void* Z_TagMalloc( int size, int tag )
{
	int		extra;
	memblock_t* start, * rover, * newmem, * base;

	if (!tag)
		Sys_Error("Z_TagMalloc: tried to use a 0 tag");

//
// scan through the block list looking for the first free block
// of sufficient size
//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 7) & ~7;		// align to 8-byte boundary

	base = rover = mainzone->rover;
	start = base->prev;

	do
	{
		if (rover == start)	// scaned all the way around the list
			return NULL;
		if (rover->tag)
			base = rover = rover->next;
		else
			rover = rover->next;
	} while (base->tag || base->size < size);

//
// found a block big enough
//
	extra = base->size - size;
	if (extra > MINFRAGMENT)
	{	// there will be a free fragment after the allocated block
		newmem = (memblock_t*)((byte*)base + size);
		newmem->size = extra;
		newmem->tag = 0;			// free block
		newmem->prev = base;
		newmem->id = ZONEID;
		newmem->next = base->next;
		newmem->next->prev = newmem;
		base->next = newmem;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block

	mainzone->rover = base->next;	// next allocation will start looking here

	base->id = ZONEID;

// marker for memory trash testing
	*(int*)((byte*)base + base->size - 4) = ZONEID;

	return (void*)((byte*)base + sizeof(memblock_t));
}


/*
========================
Z_Print
========================
*/
void Z_Print( memzone_t* zone )
{
	memblock_t* block;

	Con_Printf("zone size: %i  location: %p\n", mainzone->size, mainzone);

	for (block = zone->blocklist.next; ; block = block->next)
	{
		Con_Printf("block:%p    size:%7i    tag:%3i\n",
			block, block->size, block->tag);

		if (block->next == &zone->blocklist)
			break;			// all blocks have been hit
		if ((byte*)block + block->size != (byte*)block->next)
			Con_Printf("ERROR: block size does not touch the next block\n");
		if (block->next->prev != block)
			Con_Printf("ERROR: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Con_Printf("ERROR: two consecutive free blocks\n");
	}
}


/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap( void )
{
	memblock_t* block;

	for (block = mainzone->blocklist.next; ; block = block->next)
	{
		if (block->next == &mainzone->blocklist)
			break;			// all blocks have been hit	
		if ((byte*)block + block->size != (byte*)block->next)
			Sys_Error("Z_CheckHeap: block size does not touch the next block\n");
		if (block->next->prev != block)
			Sys_Error("Z_CheckHeap: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Sys_Error("Z_CheckHeap: two consecutive free blocks\n");
	}
}


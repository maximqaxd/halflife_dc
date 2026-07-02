// Z_zone.c

#include "quakedef.h"
#include "winquake.h"
#include "studio.h"

#define	DYNAMIC_SIZE	0xc000

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

typedef struct memblock_s
{
	int		size;           // including the header and possibly tiny fragments
	int     tag;            // a tag of 0 is a free block
	int     id;        		// should be ZONEID
	struct memblock_s* next, * prev;
	int		pad;			// pad to 64 bit boundary
} memblock_t;

typedef struct
{
	int		size;		// total bytes malloced, including header
	memblock_t	blocklist;		// start / end cap for linked list
	memblock_t* rover;
} memzone_t;

void Cache_FreeLow( int new_low_hunk );
void Cache_FreeHigh( int new_high_hunk );
void Cache_Free( cache_user_t* c );
void Cache_Compact( void );
void Cache_Flush( void );
void MnemoFree( void* ptr );

cvar_t mem_dbgfile = { "mem_dbgfile", ".\\mem.txt" };
cvar_t mnemo_cache = { "mnemo_cache", "1" };
cvar_t mnemo_report_file = { "mnemo_report_file", "\\PC\\mnemo_report.txt" };

/*
==============================================================================

						MNEMO MEMORY ALLOCATION

Mnemo is a single committed arena backed by one contiguous region (see
Mnemo_InitArena). Blocks are doubly-linked mnemo_header_t nodes; free blocks
coalesce on MnemoFree. There is no implicit gap between user payloads and
headers — each allocation carries size, flags, alloc class, and an optional
tag string.

Allocation does not use one rover only: several strategies (generic scan,
hunk-neighbour, cache-neighbour, malloc-neighbour) pick a free block to reduce
fragmentation next to the hunk, cache, or heap. Small fixed-size requests can
use pez pools (bucketed slabs with a free stack) instead of splitting the main
list.

Flags (temp, cache, hunk, zone, malloc, pez, etc.) drive purge policy and
reporting. On failure, Mnemo can run a last-chance path (e.g. texture VRAM
reclaim via callback) before giving up. Large or long-lived data still belongs
on the hunk where appropriate;

==============================================================================
*/

#define MNEMO_FLAG_USED  0x0001
#define MNEMO_SPLIT_MIN 0x120
#define MNEMO_PAGE_SIZE 4096

typedef struct mnemo_header_s
{
	struct mnemo_header_s* prev;
	struct mnemo_header_s* next;
	int payload_size;
	unsigned short flags;
	short alloc_class;
	int sequence;
	char tag[12];
} mnemo_header_t;

typedef struct
{
	int alloc_calls;
	int free_calls;
	int failed_allocs;
	int live_allocs;
	int live_bytes;
	int peak_live_bytes;
	int live_arena_allocs;
	int live_pez_allocs;
} mnemo_stats_t;

typedef struct mnemo_pez_pool_s
{
	unsigned int size;
	unsigned int count;
	byte* slab;
	void** stack;
	unsigned int top;
	struct mnemo_pez_pool_s* next;
	int _pad0;
	int _pad1;
} mnemo_pez_pool_t;

static mnemo_header_t mnemo_head;
static mnemo_stats_t mnemo_stats;
static mnemo_pez_pool_t* mnemo_pez_buckets[8];
static int mnemo_sequence;
static byte* mnemo_arena_base;
static int mnemo_arena_size;
static qboolean mnemo_arena_precommitted;
static unsigned int cs_epoch_frame;
static int          cs_epoch_bytes;
static int mnemo_alloc_attempts;
static mnemo_header_t* mnemo_last_temp;
static int mnemo_last_chance_active;
static int mnemo_temp_danger;
static void* mnemo_zone_sloppy_ptr;
static int mnemo_zone_sloppy_left;
static int hunk_alloc_class;
static mnemo_purge_callback_t mnemo_purge_callback;
static mnemo_header_t* mnemo_rover_cache;
static mnemo_header_t* mnemo_rover_hunk;
static mnemo_header_t* mnemo_rover_malloc;
static mnemo_header_t* mnemo_rover_generic;

extern byte* hunk_base;
extern int hunk_size;
extern int hunk_low_used;
extern int hunk_high_used;

static void Mnemo_PezReset( void );
static qboolean Mnemo_PezEligible( int alignedSize, unsigned int flags, int allocClass );
static int Mnemo_PezBucketIndex( int alignedSize );
static mnemo_pez_pool_t* Mnemo_PezCreatePool( int elemSize, int elemCount );
static void* Mnemo_PezAlloc( int alignedSize );
static qboolean Mnemo_PezFreePayload( void* payload );
static void Mnemo_SetTag( mnemo_header_t* hdr, const char* tag );
static void Mnemo_InitArena( void *buf, int size );
static int Mnemo_SelectAllocMode( unsigned int flags, int allocClass );
static mnemo_header_t* Mnemo_PickFreeBlock( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* MnemoSelectByHunkNeighbor( int payload_size, int allocClass );
static mnemo_header_t* MnemoSelectByCacheNeighbor( int payload_size );
static mnemo_header_t* MnemoSelectByMallocNeighbor( int payload_size );
static mnemo_header_t* MnemoSelectGeneric( int payload_size );
static void* MnemoAllocFromFreeBlock( int payload_size, int request_size, unsigned int flags, int allocClass, const char* tag, int allocMode );
static void Mnemo_UpdateRovers( mnemo_header_t* hdr );
static void Mnemo_FreeArenaBlock( mnemo_header_t* hdr );
static mnemo_header_t* MnemoCoalesceFreeBlock( mnemo_header_t* hdr );
static void* MnemoAllocInternal( int payload_size, int request_size, unsigned int flags, int allocClass, const char* tag, int allocMode );
extern int DC_ReclaimTextureSlot( void );
static qboolean Mnemo_CommitRange( byte* base, int size );
static qboolean Mnemo_CommitBlockHeader( mnemo_header_t* hdr );
static void Mnemo_DecommitPages( mnemo_header_t* hdr );
static const char* Mnemo_FlagsToString( unsigned short flags );
static void Mnemo_Summary_f( void );
void Mnemo_ReportToFile( qboolean verbose );
static void Mnemo_FreeByClass( int allocClass );
static int MnemoPurge( int aggressive );

static int Mnemo_AlignSize( int size )
{
	if (size <= 0)
		return 0;

	return (size + 31) & ~31;
}

static const char* Mnemo_FlagsToString( unsigned short flags )
{
	static char buffer[64];
	int len;

	buffer[0] = 0;
	if ((flags & ~MNEMO_FLAG_USED) == 0)
		return "free";

	if (flags & MNEMO_FLAG_PEZ)
		strcat(buffer, "pez ");
	if (flags & MNEMO_FLAG_TEMP)
		strcat(buffer, "temp ");
	if (flags & MNEMO_FLAG_CACHE)
		strcat(buffer, "cache ");
	if (flags & MNEMO_FLAG_HUNK)
		strcat(buffer, "hunk ");
	if (flags & MNEMO_FLAG_ZONE)
		strcat(buffer, "zone ");
	if (flags & MNEMO_FLAG_MALLOC)
		strcat(buffer, "malloc ");
	if (flags & MNEMO_FLAG_ROOT)
		strcat(buffer, "root ");
	if (flags & MNEMO_FLAG_NO_RECLAIM)
		strcat(buffer, "noreclaim ");

	len = Q_strlen(buffer);
	if (len > 0 && buffer[len - 1] == ' ')
		buffer[len - 1] = 0;

	return buffer;
}

void Mnemo_ReportToFile( qboolean verbose )
{
	FILE* f;
	mnemo_header_t* node;
	const char* path;

	(void)verbose;

	path = mnemo_report_file.string;
	if (!path || !path[0])
		return;

	f = fopen(path, "at");
	if (!f)
		f = fopen(path, "wt");

	if (f)
		fprintf(f, "Mnemo report in map %s:\n", sv.name[0] ? sv.name : "<none>");

	for (node = mnemo_head.next; node != &mnemo_head; node = node->next)
	{
		if (f)
		{
			fprintf(f, "Node [0x%p] seq [%8d]: ", node, node->sequence);
			fprintf(f, "%8d [0x%6.6x] byte ", node->payload_size, node->payload_size);
			fprintf(f, "%11s block: %s\n", Mnemo_FlagsToString(node->flags), node->tag);
		}

		Sleep(10);
	}

	if (f)
	{
		fprintf(f, "\n");
		fclose(f);
	}

	Mnemo_Summary_f();
}


static void Mnemo_Sizes_f( void )
{
	Con_Printf("sizeof entity_state_t: %d\n", sizeof(entity_state_t));
	Con_Printf("sizeof entity_t: %d\n", sizeof(entity_t));
	Con_Printf("sizeof entvars_t: %d\n", sizeof(entvars_t));
	Con_Printf("sizeof edict_t: %d\n", sizeof(edict_t));
	Con_Printf("sizeof msurface_t: %d\n", sizeof(msurface_t));
	Con_Printf("sizeof mnode_t: %d\n", sizeof(mnode_t));
	Con_Printf("sizeof texture_t: %d\n", sizeof(texture_t));
	Con_Printf("sizeof decal_t: %d\n", sizeof(decal_t));
	Con_Printf("sizeof mleaf_t: 	%d\n", sizeof(mleaf_t));
	Con_Printf("sizeof model_t: %d\n", sizeof(model_t));
	Con_Printf("sizeof resource_t: %d\n", sizeof(resource_t));
	Con_Printf("sizeof glpoly_t: %d\n", sizeof(glpoly_t));
	Con_Printf("sizeof mtexinfo_t: %d\n", sizeof(mtexinfo_t));
}

static void Mnemo_Summary_f( void )
{
	mnemo_header_t* node;
	int totalUsed;
	int totalFree;
	int biggestUsed;
	int biggestFree;
	int totalBlocks;
	int lastSeq;
	int usedCount;
	int freeCount;

	totalUsed = 0;
	totalFree = 0;
	biggestUsed = 0;
	biggestFree = 0;
	totalBlocks = 0;
	lastSeq = 0;
	usedCount = 0;
	freeCount = 0;

	for (node = mnemo_head.next; node != &mnemo_head; node = node->next)
	{
		totalBlocks++;
		if (node->sequence > lastSeq)
			lastSeq = node->sequence;

		if (node->flags & MNEMO_FLAG_USED)
		{
			totalUsed += node->payload_size;
			if (node->payload_size > biggestUsed)
				biggestUsed = node->payload_size;
			usedCount++;
		}
		else
		{
			totalFree += node->payload_size;
			if (node->payload_size > biggestFree)
				biggestFree = node->payload_size;
			freeCount++;
		}
	}

	Con_Printf("Arena size: %d\n", mnemo_arena_size);
	Con_Printf("Total blocks: %d Total used: %d bytes Total free: %d bytes\n",
		totalBlocks, totalUsed, totalFree);
	Con_Printf("Biggest used block: %d Biggest free block: %d\n",
		biggestUsed, biggestFree);
	Con_Printf("Last sequence number: %d\n", lastSeq);
}

void Mnemo_SetPurgeCallback( mnemo_purge_callback_t callback )
{
	mnemo_purge_callback = callback;
}

static void Mnemo_SetTag( mnemo_header_t* hdr, const char* tag )
{
	if (tag && tag[0])
	{
		Q_strncpy(hdr->tag, tag, sizeof(hdr->tag));
		hdr->tag[sizeof(hdr->tag) - 1] = 0;
	}
	else
	{
		hdr->tag[0] = 0;
	}
}

static qboolean Mnemo_CommitRange( byte* base, int size )
{
	byte* arenaStart;
	byte* arenaEnd;
	byte* commitStart;
	int commitSize;

	if (!base || size <= 0)
		return TRUE;

	if (mnemo_arena_precommitted)
		return TRUE;

	arenaStart = mnemo_arena_base;
	arenaEnd = mnemo_arena_base + mnemo_arena_size;
	commitStart = (byte*)((DWORD)base & ~(MNEMO_PAGE_SIZE - 1));

	if (commitStart < arenaStart)
		commitStart = arenaStart;

	commitSize = (int)(((unsigned int)(size + 0x1ffeU)) & ~(MNEMO_PAGE_SIZE - 1));

	if (commitSize <= 0)
		return TRUE;

	if (commitStart + commitSize > arenaEnd)
		commitSize = (int)(arenaEnd - commitStart);

	if (commitSize <= 0)
		return TRUE;

	if (!VirtualAlloc(commitStart, commitSize, MEM_COMMIT, PAGE_READWRITE))
	{
		Sys_Error("(Mnemo arena page in failed)\n"); // TODO, in binary this msg is printed by CE Exception Handler, but we don't have crash handler impl yet.
		return FALSE;
	}

	return TRUE;
}

static qboolean Mnemo_CommitBlockHeader( mnemo_header_t* hdr )
{
	return Mnemo_CommitRange((byte*)hdr, (int)sizeof(mnemo_header_t));
}

static void Mnemo_DecommitPages( mnemo_header_t* hdr )
{
	byte* payload;
	byte* end;
	byte* decommitStart;
	byte* decommitEnd;
	int decommitSize;

	if (!hdr || (hdr->flags & MNEMO_FLAG_USED) || (hdr->flags & MNEMO_FLAG_ROOT))
		return;

	if (mnemo_arena_precommitted)
		return;

	payload = (byte*)(hdr + 1);
	end = payload + hdr->payload_size;
	decommitStart = (byte*)(((DWORD)(payload + MNEMO_PAGE_SIZE - 1)) & ~(MNEMO_PAGE_SIZE - 1));
	decommitEnd = (byte*)((DWORD)end & ~(MNEMO_PAGE_SIZE - 1));
	decommitSize = (int)(decommitEnd - decommitStart);

	if (decommitSize > 0)
		VirtualFree(decommitStart, decommitSize, MEM_DECOMMIT);
}

static void Mnemo_InitArena( void *buf, int size )
{
	byte* alignedBase;
	byte* alignedEnd;
	int remain;
	int i;
	mnemo_header_t* first;

	mnemo_arena_base = NULL;
	mnemo_arena_size = 0;
	mnemo_arena_precommitted = FALSE;
	mnemo_last_temp = NULL;
	cs_epoch_frame = (unsigned int)host_framecount;
	cs_epoch_bytes = 0;
	mnemo_alloc_attempts = 0;
	mnemo_rover_cache = mnemo_rover_hunk = mnemo_rover_malloc = mnemo_rover_generic = NULL;

	for (i = 0; i < 8; i++)
		mnemo_pez_buckets[i] = NULL;

	/* Initialize sentinel: circular self-reference, flags mark it as root+used. */
	mnemo_head.prev = &mnemo_head;
	mnemo_head.next = &mnemo_head;
	mnemo_head.payload_size = 0;
	mnemo_head.flags = MNEMO_FLAG_ROOT | MNEMO_FLAG_USED;
	mnemo_head.alloc_class = 0;
	Q_strncpy(mnemo_head.tag, "Root Node", sizeof(mnemo_head.tag));
	mnemo_head.tag[sizeof(mnemo_head.tag) - 1] = 0;

	alignedBase = (byte*)(((DWORD)buf + 0x1f) & ~31);
	alignedEnd   = (byte*)(((DWORD)((byte*)buf + size)) & ~31);

	if (alignedEnd <= alignedBase)
		return;

	mnemo_arena_base = alignedBase;
	mnemo_arena_size = (int)(alignedEnd - alignedBase);
	remain = mnemo_arena_size - (int)sizeof(mnemo_header_t);

	if (remain <= 0)
		return;

	first = (mnemo_header_t*)mnemo_arena_base;
	first->prev = &mnemo_head;
	first->next = &mnemo_head;
	first->payload_size = remain;
	first->flags = 0;
	first->alloc_class = 0;
	first->sequence = ++mnemo_sequence;
	Q_strncpy(first->tag, "_arena_", sizeof(first->tag));
	first->tag[sizeof(first->tag) - 1] = 0;
	Mnemo_DecommitPages(first);

	/* Link into the circular sentinel list. */
	mnemo_head.next = first;
	mnemo_head.prev = first;

	/* Rover pointers start at the first (and only) free block. */
	mnemo_rover_cache = mnemo_rover_hunk = mnemo_rover_malloc = mnemo_rover_generic = first;

	Mnemo_PezCreatePool(0x20, 0x400);
	Mnemo_PezCreatePool(0x40, 0x100);
	Mnemo_PezCreatePool(0x80, 0x100);
}

static qboolean Mnemo_PezEligible( int alignedSize, unsigned int flags, int allocClass )
{
	if (alignedSize <= 0 || alignedSize >= 0x101)
		return FALSE;

	if (flags & MNEMO_FLAG_CACHE)
		return FALSE;

	if (flags & MNEMO_FLAG_TEMP)
		return FALSE;

	if (allocClass >= 2)
		return FALSE;

	return TRUE;
}

static int Mnemo_SelectAllocMode( unsigned int flags, int allocClass )
{
	if (flags & 0x100)
		return 2;

	if (flags & 0x200)
		return 1;

	if (mnemo_sequence < 100)
		return 2;

	if (flags & MNEMO_FLAG_TEMP)
		return 1;

	if (flags & MNEMO_FLAG_HUNK)
		return (allocClass < 3) ? 2 : 1;

	if (flags & MNEMO_FLAG_CACHE)
		return 1;
	return 2;
}

static mnemo_header_t* MnemoSelectByHunkNeighbor( int payload_size, int allocClass )
{
	mnemo_header_t* node;
	mnemo_header_t* next;
	mnemo_header_t* prev;
	mnemo_header_t* best;
	int bestSize;

	best = NULL;
	bestSize = 10000000;

	for (node = mnemo_head.next; node != &mnemo_head; node = node->next)
	{
		if (node->flags & MNEMO_FLAG_USED)
			continue;

		if (node->payload_size < payload_size)
			continue;

		next = node->next;

		if (next != &mnemo_head &&
		    (next->flags & MNEMO_FLAG_HUNK) &&
		    next->alloc_class == (short)allocClass &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}

		prev = node->prev;

		if (prev != &mnemo_head &&
		    (prev->flags & MNEMO_FLAG_HUNK) &&
		    prev->alloc_class == (short)allocClass &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}
	}

	return best;
}

static mnemo_header_t* MnemoSelectByCacheNeighbor( int payload_size )
{
	mnemo_header_t* node;
	mnemo_header_t* next;
	mnemo_header_t* prev;
	mnemo_header_t* best;
	int bestSize;

	best = NULL;
	bestSize = 10000000;

	for (node = mnemo_head.next; node != &mnemo_head; node = node->next)
	{
		if (node->flags & MNEMO_FLAG_USED)
			continue;

		if (node->payload_size < payload_size)
			continue;

		next = node->next;

		if (next != &mnemo_head &&
		    (next->flags & MNEMO_FLAG_CACHE) &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}

		prev = node->prev;

		if (prev != &mnemo_head &&
		    (prev->flags & MNEMO_FLAG_CACHE) &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}
	}

	return best;
}

static mnemo_header_t* MnemoSelectByMallocNeighbor( int payload_size )
{
	mnemo_header_t* node;
	mnemo_header_t* next;
	mnemo_header_t* prev;
	mnemo_header_t* best;
	int bestSize;

	best = NULL;
	bestSize = 10000000;

	for (node = mnemo_head.next; node != &mnemo_head; node = node->next)
	{
		if (node->flags & MNEMO_FLAG_USED)
			continue;

		if (node->payload_size < payload_size)
			continue;

		next = node->next;

		if (next != &mnemo_head &&
		    (next->flags & MNEMO_FLAG_MALLOC) &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}
		prev = node->prev;

		if (prev != &mnemo_head &&
		    (prev->flags & MNEMO_FLAG_MALLOC) &&
		    node->payload_size < bestSize)
		{
			best = node;
			bestSize = node->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return node;
		}
	}

	return best;
}

static mnemo_header_t* MnemoSelectGeneric( int payload_size )
{
	mnemo_header_t* n;
	mnemo_header_t* best;
	int bestSize;

	best = NULL;
	bestSize = 10000000;

	for (n = mnemo_head.next; n != &mnemo_head; n = n->next)
	{
		if (n->flags & MNEMO_FLAG_USED)
			continue;

		if (n->payload_size < payload_size)
			continue;

		if (n->payload_size < bestSize)
		{
			best = n;
			bestSize = n->payload_size;
			if (bestSize <= payload_size + 0x1020)
				return best;
		}
	}

	return best;
}

static mnemo_header_t* Mnemo_PickFreeBlock( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* best;

	if (flags & MNEMO_FLAG_CACHE)
	{
		best = MnemoSelectByCacheNeighbor(payload_size);

		if (best)
			return best;
	}
	if (flags & MNEMO_FLAG_HUNK)
	{
		best = MnemoSelectByHunkNeighbor(payload_size, allocClass);

		if (best)
			return best;
	}
	if (flags & MNEMO_FLAG_MALLOC)
	{
		best = MnemoSelectByMallocNeighbor(payload_size);

		if (best)
			return best;
	}

	return MnemoSelectGeneric(payload_size);
}


static void* MnemoAllocFromFreeBlock( int payload_size, int request_size, unsigned int flags, int allocClass, const char* tag, int allocMode )
{
	mnemo_header_t* block;
	mnemo_header_t* split;
	mnemo_header_t* used;
	int extra;
	int needsSplit;
	int fromEnd;

	block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);

	if (!block)
		return NULL;

	extra = block->payload_size - payload_size;
	needsSplit = (extra >= MNEMO_SPLIT_MIN) ? 1 : 0;
	fromEnd = (allocMode == 2 && needsSplit) ? 1 : 0;

	if (fromEnd)
	{
		/* New allocation carved from the END of the free block. */
		used = (mnemo_header_t*)((byte*)(block + 1) + (extra - (int)sizeof(mnemo_header_t)));

		if (!Mnemo_CommitBlockHeader(used) ||
		    !Mnemo_CommitRange((byte*)(used + 1), payload_size))
			return NULL;

		memset(used, 0, sizeof(*used));

		used->payload_size = payload_size;
		block->payload_size = extra - (int)sizeof(mnemo_header_t);
		used->prev = block;
		used->next = block->next;
		block->next->prev = used;
		block->next = used;
		block = used;
	}
	else if (needsSplit)
	{
		/* Free split header is carved after the payload. */
		split = (mnemo_header_t*)((byte*)(block + 1) + payload_size);

		if (!Mnemo_CommitBlockHeader(split) ||
		    !Mnemo_CommitRange((byte*)(block + 1), payload_size))
			return NULL;

		memset(split, 0, sizeof(*split));

		split->payload_size = extra - (int)sizeof(mnemo_header_t);
		split->sequence = block->sequence;
		Q_strncpy(split->tag, block->tag, sizeof(split->tag));
		split->tag[sizeof(split->tag) - 1] = 0;
		split->prev = block;
		split->next = block->next;
		block->next->prev = split;
		block->next = split;
		block->payload_size = payload_size;

		/* Move rover pointers from block to split (split is now the free remnant). */
		if (mnemo_rover_cache   == block) mnemo_rover_cache   = split;
		if (mnemo_rover_hunk    == block) mnemo_rover_hunk    = split;
		if (mnemo_rover_malloc  == block) mnemo_rover_malloc  = split;
		if (mnemo_rover_generic == block) mnemo_rover_generic = split;
	}
	else
	{
		/* No split: commit payload only (block header already committed). */
		if (!Mnemo_CommitRange((byte*)(block + 1), payload_size))
			return NULL; 
	}

	block->flags = (unsigned short)(flags | MNEMO_FLAG_USED);
	block->alloc_class = (short)allocClass;
	block->sequence = mnemo_sequence++;
	Mnemo_SetTag(block, tag);

	mnemo_stats.alloc_calls++;
	mnemo_stats.live_allocs++;
	mnemo_stats.live_arena_allocs++;
	mnemo_stats.live_bytes += payload_size;
	if (mnemo_stats.live_bytes > mnemo_stats.peak_live_bytes)
		mnemo_stats.peak_live_bytes = mnemo_stats.live_bytes;

	return (void*)(block + 1);
}


static void Mnemo_UpdateRovers( mnemo_header_t* hdr )
{
	if (hdr->flags & MNEMO_FLAG_USED)
		Sys_Error("Mnemo_UpdateRovers: block still allocated");

	if (mnemo_last_temp == hdr)
		mnemo_last_temp = NULL;
	/* malloc rover: update when NULL, or freed block size is between hunk and cache sizes. */
	if (!mnemo_rover_malloc ||
	    (mnemo_rover_cache != NULL && hdr->payload_size <= mnemo_rover_cache->payload_size) ||
	    (mnemo_rover_hunk  != NULL && mnemo_rover_hunk->payload_size <= hdr->payload_size))
		mnemo_rover_malloc = hdr;
	/* cache rover: tracks the largest known free block. */
	if (!mnemo_rover_cache || mnemo_rover_cache->payload_size < hdr->payload_size)
		mnemo_rover_cache = hdr;
	/* hunk rover: tracks the smallest known free block. */
	if (!mnemo_rover_hunk || hdr->payload_size < mnemo_rover_hunk->payload_size)
		mnemo_rover_hunk = hdr;
}


static mnemo_header_t* MnemoCoalesceFreeBlock( mnemo_header_t* hdr )
{
	mnemo_header_t* next;
	mnemo_header_t* prev;

	if (!hdr || (hdr->flags & MNEMO_FLAG_USED))
		return hdr;

	/* Absorb next block if free. */
	next = hdr->next;
	if (next != &mnemo_head && next->flags == 0)
	{
		hdr->payload_size += (int)sizeof(mnemo_header_t) + next->payload_size;
		if (mnemo_rover_cache   == next) mnemo_rover_cache   = NULL;
		if (mnemo_rover_hunk    == next) mnemo_rover_hunk    = NULL;
		if (mnemo_rover_malloc  == next) mnemo_rover_malloc  = NULL;
		if (mnemo_last_temp     == next) mnemo_last_temp     = NULL;
		/* Unlink next. */
		next->prev->next = next->next;
		next->next->prev = next->prev;
	}

	/* Absorb into prev block if free. */
	prev = hdr->prev;
	if (prev != &mnemo_head && prev->flags == 0)
	{
		prev->payload_size += (int)sizeof(mnemo_header_t) + hdr->payload_size;
		if (mnemo_rover_cache   == hdr) mnemo_rover_cache   = NULL;
		if (mnemo_rover_hunk    == hdr) mnemo_rover_hunk    = NULL;
		if (mnemo_rover_malloc  == hdr) mnemo_rover_malloc  = NULL;
		if (mnemo_last_temp     == hdr) mnemo_last_temp     = NULL;
		/* Unlink hdr. */
		hdr->prev->next = hdr->next;
		hdr->next->prev = hdr->prev;
		hdr = prev;
	}

	return hdr;
}

static void Mnemo_FreeArenaBlock( mnemo_header_t* hdr )
{
	mnemo_header_t* merged;
	char freeTag[12 + 6];

	hdr->flags = 0;
	sprintf(freeTag, "free %s", hdr->tag);
	Q_strncpy(hdr->tag, freeTag, sizeof(hdr->tag));
	hdr->tag[sizeof(hdr->tag) - 1] = 0;
	hdr->sequence = mnemo_sequence++;
	merged = MnemoCoalesceFreeBlock(hdr);
	Mnemo_DecommitPages(merged);
	Mnemo_UpdateRovers(merged);
}

static int Mnemo_PezBucketIndex( int alignedSize )
{
	int bucketSize = 32;
	int bucketIdx = 0;

	while (bucketSize < alignedSize)
	{
		bucketSize <<= 1;
		bucketIdx++;
	}

	if (bucketIdx < 0 || bucketIdx > 7)
		Sys_Error("Bad pez bucket");

	return bucketIdx;
}

static mnemo_pez_pool_t* Mnemo_PezCreatePool( int elemSize, int elemCount )
{
	mnemo_pez_pool_t* pool;
	unsigned int alignedSize;
	char tag[MAX_QPATH];
	int slabOffset;
	int bucketIdx;
	int i;

	alignedSize = Mnemo_AlignSize(elemSize);

	_snprintf(tag, sizeof(tag), "%d-Pez", elemSize);
	tag[sizeof(tag) - 1] = '\0';

	pool = MnemoAlloc(elemCount * alignedSize + elemCount * sizeof(int) + 56, MNEMO_FLAG_PEZ, 0, tag); // todo fix hardcode

	if (!pool)
		return NULL;

	memset(pool, 0, sizeof(*pool));

	pool->size  = alignedSize;
	pool->count = (unsigned int)elemCount;
	pool->stack = (void**)((byte*)pool + (int)sizeof(mnemo_pez_pool_t));
	slabOffset = (int)sizeof(mnemo_pez_pool_t) + elemCount * (int)sizeof(void*);
	pool->slab  = (byte*)pool + slabOffset;
	pool->top   = (unsigned int)elemCount;

	for (i = 0; i < elemCount; i++)
		pool->stack[i] = pool->slab + pool->size * i;

	bucketIdx = Mnemo_PezBucketIndex((int)pool->size);

	pool->next = mnemo_pez_buckets[bucketIdx];
	mnemo_pez_buckets[bucketIdx] = pool;
	return pool;
}

static void* Mnemo_PezAlloc( int alignedSize )
{
	int bucketIdx;
	mnemo_pez_pool_t* pool;
	void* payload;
	int elemSize;
	int elemCount;

	bucketIdx = Mnemo_PezBucketIndex(alignedSize);

	if (bucketIdx < 0)
		return NULL;

	for (pool = mnemo_pez_buckets[bucketIdx]; pool; pool = pool->next)
	{
		if (pool->top > 0)
			break;
	}

	if (!pool)
	{
		elemSize = 32 << bucketIdx;
		elemCount = MNEMO_PAGE_SIZE / elemSize;

		if (elemCount < 1)
			elemCount = 1;

		pool = Mnemo_PezCreatePool(elemSize, elemCount);
	}

	if (!pool || pool->top <= 0)
		return NULL;

	if (pool->top > pool->count)
		Sys_Error("Mnemo_PezAlloc: bad top %d > %d", pool->top, pool->count);

	payload = pool->stack[--pool->top];

	if (!payload)
		Sys_Error("Mnemo_PezAlloc: null payload");

	pool->stack[pool->top] = NULL;

	return payload;
}

static qboolean Mnemo_PezFreePayload( void* payload )
{
	mnemo_pez_pool_t* pool;
	byte* p;
	byte* begin;
	byte* end;
	int i;

	if (!payload)
		return FALSE;

	p = (byte*)payload;

	for (i = 0; i < 8; i++)
	{
		for (pool = mnemo_pez_buckets[i]; pool; pool = pool->next)
		{
			begin = pool->slab;
			end = pool->slab + pool->size * pool->count;
			if (p >= begin && p < end)
			{
				/* Extra safety: only accept exact element addresses. */
				if (((p - begin) % pool->size) != 0)
					return FALSE;

				if (pool->top >= pool->count)
					Sys_Error("MnemoFree: pez stack overflow");

				pool->stack[pool->top++] = payload;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void Mnemo_PezReset( void )
{
	int i;
	mnemo_pez_pool_t* pool;
	mnemo_pez_pool_t* next;

	for (i = 0; i < 8; i++)
	{
		pool = mnemo_pez_buckets[i];

		while (pool)
		{
			next = pool->next;
			MnemoFree(pool);
			pool = next;
		}

		mnemo_pez_buckets[i] = NULL;
	}
}

int Mnemo_LastChanceActive( void )
{
	return mnemo_last_chance_active;
}

void* MnemoAlloc( int size, unsigned int flags, int allocClass, const char* tag )
{
	int payload_size;
	int allocMode;
	void* p;
	void* pez;
	mnemo_header_t* hdr;

	if (size <= 0)
		return NULL;

	if (size > 4000000)
		Sys_Error("Absurd MnemoAlloc(%d, 0x%x, %d, %s)", size, flags, allocClass, tag ? tag : "<null>");

	mnemo_alloc_attempts++;

	if (flags & MNEMO_FLAG_TEMP)
	{
		if (mnemo_last_temp && (mnemo_last_temp->flags & MNEMO_FLAG_USED))
		{
			void* oldTemp = (void*)(mnemo_last_temp + 1);
			mnemo_last_temp = NULL;
			MnemoFree(oldTemp);
		}
		else
		{
			mnemo_last_temp = NULL;
		}
	}

	payload_size = Mnemo_AlignSize(size);

	if (Mnemo_PezEligible(payload_size, flags, allocClass))
	{
		pez = Mnemo_PezAlloc(payload_size);

		if (pez)
		{
			mnemo_stats.alloc_calls++;
			mnemo_stats.live_allocs++;
			mnemo_stats.live_pez_allocs++;

			if ((flags & MNEMO_FLAG_MALLOC) == 0)
				memset(pez, 0, payload_size);

			return pez;
		}
	}

	allocMode = Mnemo_SelectAllocMode(flags, allocClass);
	p = MnemoAllocInternal(payload_size, size, flags, allocClass, tag, allocMode);

	mnemo_last_chance_active = 0;

	if (!p)
	{
		const char* flagsText;
		mnemo_stats.failed_allocs++;

		if ((flags & MNEMO_FLAG_NO_RECLAIM) == 0)
		{
			flagsText = Mnemo_FlagsToString((unsigned short)flags);
			Con_DPrintf("Mnemo couldn't allocate %d bytes for a %s block named %s.\n",
			            size, flagsText, tag ? tag : "<null>");

			if (mnemo_cache.value > 0.0f)
				Mnemo_ReportToFile(FALSE);
		}
		return NULL;
	}

	{
		/* clear rovers that point to the just-allocated block. */
		mnemo_header_t* allocHdr = ((mnemo_header_t*)p) - 1;
		if (mnemo_rover_cache   == allocHdr) mnemo_rover_cache   = NULL;
		if (mnemo_rover_hunk    == allocHdr) mnemo_rover_hunk    = NULL;
		if (mnemo_rover_malloc  == allocHdr) mnemo_rover_malloc  = NULL;
	}

	if ((flags & MNEMO_FLAG_MALLOC) == 0)
		memset(p, 0, payload_size);

	if (flags & MNEMO_FLAG_TEMP)
	{
		hdr = ((mnemo_header_t*)p) - 1;
		mnemo_last_temp = hdr;
	}
	return p;
}

static int MnemoDbgOldSize( void* ptr )
{
	mnemo_header_t* hdr;

	if (!ptr)
		return 0;

	hdr = ((mnemo_header_t*)ptr) - 1;

	if (!(hdr->flags & MNEMO_FLAG_USED))
		return 0;

	return hdr->payload_size;
}

void* MnemoReallocDbg( void* oldPtr, int sizeBytes, const char* srcFile, int srcLine )
{
	char tag[MAX_OSPATH];
	const char* base;
	const char* slash;
	const char* p;
	int copyBytes;
	void* newPtr;

	if (sizeBytes <= 0)
	{
		MnemoFree(oldPtr);
		return NULL;
	}

	base = srcFile ? srcFile : "unknown";
	slash = NULL;

	for (p = base; *p; p++)
		if (*p == '\\' || *p == '/')
			slash = p;

	base = slash ? slash + 1 : base;

	sprintf(tag, "%d %s", srcLine, base);

	newPtr = MnemoAlloc(sizeBytes, MNEMO_FLAG_MALLOC, 0, tag);

	if (!newPtr)
	{
		if (oldPtr)
			Sys_Error("Realloc failed.");
		return NULL;
	}

	if (!oldPtr)
		return newPtr;

	copyBytes = MnemoDbgOldSize(oldPtr);

	if (copyBytes > sizeBytes)
		copyBytes = sizeBytes;

	if (copyBytes > 0)
		memcpy(newPtr, oldPtr, copyBytes);

	MnemoFree(oldPtr);
	return newPtr;
}

void MnemoFreeDbg( void* ptr )
{
	MnemoFree(ptr);
}

void MnemoFree( void* ptr )
{
	mnemo_header_t* hdr;

	if (!ptr)
		return;

	if (Mnemo_PezFreePayload(ptr))
	{
		mnemo_stats.free_calls++;

		if (mnemo_stats.live_allocs > 0)
			mnemo_stats.live_allocs--;

		if (mnemo_stats.live_pez_allocs > 0)
			mnemo_stats.live_pez_allocs--;

		return;
	}

	hdr = ((mnemo_header_t*)ptr) - 1;

	if (!(hdr->flags & MNEMO_FLAG_USED))
		Sys_Error("MnemoFree: double free or invalid pointer");

	if (hdr->flags & MNEMO_FLAG_ROOT)
		Sys_Error("_FreeBlock: attempt to free root block");

	if (hdr == mnemo_last_temp)
		mnemo_last_temp = NULL;

	mnemo_stats.free_calls++;
	mnemo_stats.live_allocs--;
	mnemo_stats.live_bytes -= hdr->payload_size;

	if (mnemo_stats.live_bytes < 0)
		mnemo_stats.live_bytes = 0;

	if (mnemo_stats.live_arena_allocs > 0)
		mnemo_stats.live_arena_allocs--;

	Mnemo_FreeArenaBlock(hdr);
}

static void Mnemo_FreeByClass( int allocClass )
{
	mnemo_header_t* hdr;
	void* payload;
	qboolean freedOne;

	do
	{
		freedOne = FALSE;
		for (hdr = mnemo_head.next; hdr != &mnemo_head; hdr = hdr->next)
		{
			if (!(hdr->flags & MNEMO_FLAG_USED))
				continue;

			if (hdr->flags & MNEMO_FLAG_ROOT)
				continue;

			if (hdr->alloc_class != allocClass)
				continue;

			payload = (void*)(hdr + 1);
			MnemoFree(payload);
			freedOne = TRUE;

			break;
		}
	} while (freedOne);
}


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

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

#define HUNK_NAME_LEN 64

typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNK_NAME_LEN];
} hunk_t;

byte* hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

qboolean	hunk_tempactive;
int		hunk_tempmark;

void R_FreeTextures( void );


/*
===================
Hunk_AllocName
===================
*/
void* Hunk_AllocName(int size, char* name)
{
	int aligned;
	int chunk;
	void* result;
	hunk_t* h;

	if (!mnemo_arena_base || mnemo_arena_size <= 0 || !mnemo_head.next)
	{
#ifdef PARANOID
		Hunk_Check();
#endif
		if (size < 0)
			Sys_Error("Hunk_Alloc: bad size: %i", size);

		size = sizeof(hunk_t) + ((size + 15) & ~15);
		if (hunk_size - hunk_low_used - hunk_high_used < size)
			Sys_Error("Hunk_Alloc: failed on %i bytes", size);

		h = (hunk_t*)(hunk_base + hunk_low_used);
		hunk_low_used += size;
		memset(h, 0, size);
		h->size = size;
		h->sentinal = HUNK_SENTINAL;
		Q_strncpy(h->name, name, sizeof(h->name));
		h->name[sizeof(h->name) - 1] = 0;
		return (void*)(h + 1);
	}

	if (size <= 0)
		return NULL;

	if (size < 1000)
	{
		/* Small path: sloppy bump (4-byte align). Same pool as zone when zonesize < 1000. */
		aligned = (size + 3) & ~3;
		if (mnemo_zone_sloppy_left < aligned || !mnemo_zone_sloppy_ptr)
		{
			chunk = 0x1000;
			if (chunk < aligned)
				chunk = aligned;
			mnemo_zone_sloppy_ptr = MnemoAlloc(chunk, MNEMO_FLAG_HUNK, hunk_alloc_class, "sloppy");
			if (!mnemo_zone_sloppy_ptr)
			{
				mnemo_zone_sloppy_left = 0;
				return NULL;
			}
			mnemo_zone_sloppy_left = chunk;
		}
		result = mnemo_zone_sloppy_ptr;
		mnemo_zone_sloppy_ptr = (byte*)mnemo_zone_sloppy_ptr + aligned;
		mnemo_zone_sloppy_left -= aligned;
		return result;
	}

	return MnemoAlloc(size, MNEMO_FLAG_HUNK, hunk_alloc_class, name ? name : "unknown");
}

/*
===================
Hunk_Alloc
===================
*/
void* Hunk_Alloc( int size )
{
	return Hunk_AllocName(size, "unknown");
}

int	Hunk_LowMark( void )
{
	mnemo_zone_sloppy_ptr = NULL;
	mnemo_zone_sloppy_left = 0;
	hunk_alloc_class++;
	return hunk_alloc_class;
}

void Hunk_FreeToLowMark( int mark )
{
	int cls;

	if (mark < 0 || mark > hunk_alloc_class)
		Sys_Error("Hunk_FreeToLowMark: bad mark %i", mark);

	for (cls = mark; cls <= hunk_alloc_class; cls++)
		Mnemo_FreeByClass(cls);

	hunk_alloc_class = mark;
	mnemo_zone_sloppy_ptr = NULL;
	mnemo_zone_sloppy_left = 0;
	hunk_low_used = 0;
}

int	Hunk_HighMark( void )
{
	if (hunk_tempactive)
	{
		hunk_tempactive = FALSE;
		Hunk_FreeToHighMark(hunk_tempmark);
	}

	return hunk_high_used;
}

void Hunk_FreeToHighMark( int mark )
{
	if (hunk_tempactive)
	{
		hunk_tempactive = FALSE;
		Hunk_FreeToHighMark(hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
		Sys_Error("Hunk_FreeToHighMark: bad mark %i", mark);


	hunk_high_used = mark;
}


/*
===================
Hunk_HighAllocName
===================
*/
void* Hunk_HighAllocName( int size, char* name )
{
	Sys_Error("We shouldn't be using Hunk_HighAllocName directly");
	return NULL;
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void* Hunk_TempAlloc( int size )
{
	return MnemoAlloc(size, MNEMO_FLAG_TEMP, hunk_alloc_class, "temp");
}

/*
===============================================================================

CACHE MEMORY

All cache entries are Mnemo-arena-backed (MnemoAlloc/MnemoFree).
No hunk-space dependency; no sentinel cache_head node.

===============================================================================
*/

typedef struct cache_system_s
{
	char                   name[16];
	unsigned int           timestamp;
	unsigned int           frame;
	unsigned int           flags;
	int                    size;
	void**                 user;
	struct cache_system_s* lru_prev;
	struct cache_system_s* lru_next;
} cache_system_t;

static cache_system_t* cs_mru_head;
static cache_system_t* cs_lru_tail;
static int cs_live_bytes;

static void Cache_MakeLRU( cache_system_t* cs )
{
	cs->lru_next = NULL;
	cs->lru_prev = cs_mru_head;        /* old head is now one step older */

	if (cs_mru_head)
		cs_mru_head->lru_next = cs;    /* old head points toward cs (newer) */

	if (!cs_lru_tail)
		cs_lru_tail = cs;              /* first entry: init tail */

	cs_mru_head = cs;
}

static void Cache_UnlinkLRU( cache_system_t* cs )
{
	if (cs_mru_head == cs)
		cs_mru_head = cs->lru_prev;    /* new head = older neighbor */

	if (cs_lru_tail == cs)
		cs_lru_tail = cs->lru_next;    /* new tail = newer neighbor */

	if (cs->lru_next)
		cs->lru_next->lru_prev = cs->lru_prev;

	if (cs->lru_prev)
		cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

/*
 * Strip the "moved" LSB from user->data to get the real payload pointer.
 * user->data LSB=1 means block was relocated by MnemoCacheMove (DC juggling).
 */
static void* Cache_UserPayload( cache_user_t* user )
{
	if (!user || !user->data)
		return NULL;

	return (void*)(((unsigned int)user->data) & ~1u);
}

static qboolean Cache_UserLocked( cache_user_t* user )
{
	void* payload;
	cache_system_t* cs;

	payload = Cache_UserPayload(user);

	if (!payload)
		return FALSE;

	cs = ((cache_system_t*)payload) - 1;

	return (cs->flags & 1u) ? TRUE : FALSE;
}

/*
============
Cache_LockBlock / Cache_UnlockBlock

Lock a cache entry against eviction (sets cs->flags bit 0).
Returns 0 if block has been moved (must call Cache_Check first).
============
*/
int Cache_LockBlock( cache_user_t* c )
{
	cache_system_t* cs;
	unsigned int raw;

	if (!c || !c->data) 
		return 0;

	raw = (unsigned int)c->data;

	if (raw & 1u) 
		return 0;             /* moved: must re-resolve first */

	cs = ((cache_system_t*)(void*)raw) - 1;
	cs->flags |= 1u;

	return 1;
}

int Cache_UnlockBlock( cache_user_t* c )
{
	cache_system_t* cs;
	unsigned int raw;

	if (!c || !c->data) 
		return 0;

	raw = (unsigned int)c->data;

	if (raw & 1u) 
		return 0;
		
	cs = ((cache_system_t*)(void*)raw) - 1;
	cs->flags &= ~1u;

	return 1;
}

/*
============
Cache_Flush

Free all unlocked cache blocks.
Traverses from LRU tail toward MRU head (lru_next direction).
Skips "moved" blocks (user->data LSB=1) -> their cs is stale.
============
*/
void Cache_Flush( void )
{
	cache_system_t* cs;
	cache_system_t* next_cs;

	cs = cs_lru_tail;
	while (cs)
	{
		next_cs = cs->lru_next;

		if (!((unsigned int)(*cs->user) & 1u))  /* skip moved blocks */
			Cache_Free((cache_user_t*)cs->user);

		cs = next_cs;
	}
}

/*
============
Cache_FreeOldFrame

Free LRU-tail blocks whose frame stamp doesn't match the current frame.
============
*/
void Cache_FreeOldFrame( void )
{
	cache_system_t* cs;
	cache_system_t* next_cs;

	cs = cs_lru_tail;

	while (cs && cs->frame != (unsigned int)host_framecount)
	{
		next_cs = cs->lru_next;
		Cache_Free((cache_user_t*)cs->user);
		cs = next_cs;
	}
}

/*
============
MnemoPurge

Pass 1: LRU tail -> MRU head (lru_next).
  Find unlocked block where timestamp != gHostSpawnCount OR frame != host_framecount.
Pass 2: MRU head -> LRU tail (lru_prev).
  Find any unlocked block regardless of stamp 
Returns cs->size of freed block, or 0 if nothing purgeable.
============
*/
static int MnemoPurge( int aggressive )
{
	cache_system_t* cs;
	int bytes;

	/* Pass 1: from oldest toward newest; skip blocks that are current
	 * (same spawn-count AND same host_framecount). */
	for (cs = cs_lru_tail; cs != NULL; cs = cs->lru_next)
	{
		if (cs->flags & 1u)
			continue;  /* locked */

		if (cs->timestamp == (unsigned int)gHostSpawnCount &&
		    cs->frame == (unsigned int)host_framecount)
			continue;  /* current map, current frame — keep */

		if (!aggressive && cs->size <= 0x800)
			continue;

		bytes = cs->size;
		Cache_Free((cache_user_t*)cs->user);
		return bytes;
	}

	/* Pass 2: from newest toward oldest; any unlocked block */
	for (cs = cs_mru_head; cs != NULL; cs = cs->lru_prev)
	{
		if (cs->flags & 1u)
			continue;  /* locked */

		if (!aggressive && cs->size <= 0x800)
			continue;

		bytes = cs->size;
		Cache_Free((cache_user_t*)cs->user);
		return bytes;
	}

	return 0;
}

/*
 * Mnemo_TryCacheMoveBlock
 *
 * 
 * Given a mnemo_header that holds a CACHE block, attempts to move it
 * to a new arena location (reducing fragmentation near free holes).
 *
 * Binary MnemoCacheMove only relocates the cs header and leaves payload
 * in place (VQ textures stay in VRAM, moving the RAM tracking struct).
 * For now, so we do a full copy: alloc new block, copy everything,
 * update user->data, free old.
 *
 * Returns 1 on successful move, 0 otherwise.
 */
static int Mnemo_TryCacheMoveBlock( mnemo_header_t* hdr )
{
	cache_system_t* cs;
	cache_system_t* newcs;
	int size;

	if (!(hdr->flags & MNEMO_FLAG_USED)) return 0;
	if (!(hdr->flags & MNEMO_FLAG_CACHE)) return 0;

	cs = (cache_system_t*)((byte*)hdr + sizeof(mnemo_header_t));

	if (cs->flags & 1u) 
		return 0;                      /* locked */

	if ((unsigned int)(*cs->user) & 1u) 
		return 0;      /* already marked moved */

	size = cs->size;

	if (size <= 0)
		 return 0;

	/*
	 * Allocate a new block. MNEMO_FLAG_NO_RECLAIM prevents re-entrant
	 * compaction loops; if there's no room now, bail out.
	 */
	newcs = (cache_system_t*)MnemoAlloc(size,
	                                    MNEMO_FLAG_CACHE | MNEMO_FLAG_NO_RECLAIM,
	                                    0, cs->name);
	if (!newcs) 
		return 0;

	Q_memcpy(newcs, cs, size);

	/* Fix up cross-references in the new cs. */
	newcs->user     = cs->user;
	newcs->lru_prev = NULL;
	newcs->lru_next = NULL;

	*newcs->user = (byte*)newcs + sizeof(cache_system_t);

	Cache_UnlinkLRU(cs);
	cs_live_bytes -= size;
	MnemoFree(cs);   /* coalesces arena + updates rovers */

	Cache_MakeLRU(newcs);
	cs_live_bytes += size;

	return 1;
}

/*
 * MnemoCacheMoveEnds
 *
 * Examines the blocks immediately adjacent (in arena order)
 * to the largest known free block (mnemo_rover_cache) and tries to move each
 * one that is a CACHE block, repeating until no further moves are possible.
 * Goal: consolidate free space near the biggest hole.
 * Returns 1 if at least one block was moved, 0 otherwise.
 */
int MnemoCacheMoveEnds( void )
{
	mnemo_header_t* rover;
	mnemo_header_t* adj;
	int moved = 0;
	int this_pass;

	do {
		this_pass = 0;
		rover = mnemo_rover_cache;
		if (!rover) break;

		/* check rover->next (block after the largest free gap). */
		adj = rover->next;
		if (adj != &mnemo_head &&
		    (adj->flags & MNEMO_FLAG_USED) &&
		    (adj->flags & MNEMO_FLAG_CACHE))
		{
			if (Mnemo_TryCacheMoveBlock(adj))
			{ this_pass = 1; moved = 1; }
		}

		/* Re-read rover: MnemoFree/alloc may have changed it. */
		rover = mnemo_rover_cache;
		if (!rover) break;

		/* check rover->prev (block before the largest free gap). */
		adj = rover->prev;
		if (adj != &mnemo_head &&
		    (adj->flags & MNEMO_FLAG_USED) &&
		    (adj->flags & MNEMO_FLAG_CACHE))
		{
			if (Mnemo_TryCacheMoveBlock(adj))
			{ this_pass = 1; moved = 1; }
		}
	} while (this_pass);

	return moved;
}

/*
 * MnemoCacheMoveSweep
 *
 * Walks the entire arena circular list. For each FREE block,
 * examines both neighboring (prev/next in arena order) blocks: if either is a
 * CACHE block it tries to move it, restarting the walk from the beginning on
 * each successful move. Stops when a full pass yields no moves.
 * Returns 1 if at least one block was moved, 0 otherwise.
 */
int MnemoCacheMoveSweep( void )
{
	mnemo_header_t* hdr;
	mnemo_header_t* adj;
	int moved = 0;

	hdr = mnemo_head.next;
	while (hdr != &mnemo_head)
	{
		mnemo_header_t* next = hdr->next;  /* save before possible coalesce */

		/* only examine blocks that are FREE (flags == 0). */
		if ((hdr->flags & MNEMO_FLAG_USED) == 0)
		{
			/* Check block immediately after this free gap in the arena. */
			adj = hdr->next;
			if (adj != &mnemo_head &&
			    (adj->flags & MNEMO_FLAG_USED) &&
			    (adj->flags & MNEMO_FLAG_CACHE))
			{
				if (Mnemo_TryCacheMoveBlock(adj))
				{
					moved = 1;
					/* restarts from mnemo_head.next on each move. */
					hdr = mnemo_head.next;
					continue;
				}
			}

			/* Check block immediately before this free gap in the arena. */
			adj = hdr->prev;
			if (adj != &mnemo_head &&
			    (adj->flags & MNEMO_FLAG_USED) &&
			    (adj->flags & MNEMO_FLAG_CACHE))
			{
				if (Mnemo_TryCacheMoveBlock(adj))
				{
					moved = 1;
					hdr = mnemo_head.next;
					continue;
				}
			}
		}

		hdr = next;
	}

	return moved;
}

/*
 * MnemoAllocInternal
 *
 *
 * Phase 1: initial attempt.
 * Phase 2: texture-slot reclaim loop -> retry.
 * Phase 3: MnemoCacheMoveEnds (compact ends) -> retry.
 * Phase 4: MnemoCacheMoveSweep (full sweep) -> retry.
 * Phase 5: MnemoPurge(0) loop -> retry after each victim.
 * Phase 6: last-chance flag + purge callback(1) -> retry.
 * Phase 7: sacrifice dangling temp block ("Danger" warning) -> final retry.
 */
static void* MnemoAllocInternal( int payload_size, int request_size, unsigned int flags, int allocClass, const char* tag, int allocMode )
{
	void* p;
	int purged;
	mnemo_header_t* tempHdr;

	/* Phase 1: initial attempt. */
	p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
	if (p)
		return p;

	if (flags & MNEMO_FLAG_NO_RECLAIM)
		return NULL;

	/* Phase 2: DC texture-slot reclaim  */
	while (DC_ReclaimTextureSlot() != 0)
		;
	p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
	if (p)
		return p;

	/* Phase 3: compact cache ends  */
	if (MnemoCacheMoveEnds())
	{
		p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
		if (p)
			return p;
	}

	/* Phase 4: full cache sweep  */
	if (MnemoCacheMoveSweep())
	{
		p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
		if (p)
			return p;
	}

	/* Phase 5: purge-one loop  */
	do
	{
		purged = MnemoPurge(0);
		p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
		if (p)
			return p;
	} while (purged != 0);

	/* Phase 6: last-chance pass. */
	mnemo_last_chance_active = 1;

	if (mnemo_purge_callback)
		mnemo_purge_callback(1);

	p = MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
	if (p)
		return p;

	/* Phase 7: temp block sacrifice. */
	tempHdr = mnemo_last_temp;

	if (!tempHdr)
		return NULL;

	mnemo_temp_danger = 1;
	Con_DPrintf("Mnemo: Danger, discarding temp block prematurely.\n");
	mnemo_last_temp = NULL;
	Mnemo_FreeArenaBlock(tempHdr);

	return MnemoAllocFromFreeBlock(payload_size, request_size, flags, allocClass, tag, allocMode);
}




/*
============
CacheSystemCompare

Compares the names of two cache_system_t structs.
Used with qsort()
============
*/

//needed for OutputDebugString
//#include <windows.h>
static int CacheSystemCompare( const void* ppcs1, const void* ppcs2 )
{
	cache_system_t* pcs1 = *(cache_system_t**)ppcs1;
	cache_system_t* pcs2 = *(cache_system_t**)ppcs2;
//	char buf[400];
//	sprintf(buf, "Comparing \"%s\" and \"%s\"\n", pcs1->name, pcs2->name);
//	OutputDebugString(buf);


	return _stricmp(pcs1->name, pcs2->name);
}




/*
============
ComparePath1

compares the first directory of two paths...
(so  "foo/bar" will match "foo/fred"
============
*/
int ComparePath1( char* path1, char* path2 )
{
	while (*path1 != '/' && *path1 != '\\' && *path1)
	{
		if (*path1 != *path2)
			return 0;
		else
		{
			path1++;
			path2++;
		}
	}
	return 1;
}

/*
============
CommatizeNumber

takes a number, and creates a string of that with commas in the
appropriate places.
============
*/
char* CommatizeNumber( int num, char* pout )
{

	//this is probably more complex than it needs to be.
	int len = 0;
	int i;
	char outbuf[50];
	memset(outbuf, 0, sizeof(outbuf));
	while (num)
	{
		char tempbuf[50];
		int temp = num % 1000;
		num = num / 1000;
		strcpy(tempbuf, outbuf);

		sprintf(outbuf, ",%03i%s", temp, tempbuf);
	}

	len = strlen(outbuf);

	for (i = 0; i < len; i++)				//find first significant digit
	{
		if (outbuf[i] != '0' && outbuf[i] != ',')
			break;
	}

	if (i == len)
		strcpy(pout, "0");
	else
		strcpy(pout, &outbuf[i]);	//copy from i to get rid of the first comma and leading zeros

	return pout;
}

/*
============
Cache_Compact

Try to move unlocked cache blocks to coalesce free space.
Sweeps from LRU tail toward MRU head (saved at start), up to 50 successful moves.
For each unlocked, non-moved block, allocate a replacement and copy.
============
*/
void Cache_Compact( void )
{
	cache_system_t* cs;
	cache_system_t* stop;
	cache_system_t* newcs;
	void* payload;
	int moved_count = 0;

	stop = cs_mru_head;

	cs = cs_lru_tail;
	while (cs != NULL && moved_count < 0x32)
	{
		cache_system_t* next = cs->lru_next;

		/* skip locked or moved blocks */
		if (cs->flags & 1u)
			goto next_cs;
		if ((unsigned int)(*cs->user) & 1u)
			goto next_cs;

		payload = (byte*)cs + sizeof(cache_system_t);

		/*
		 * Attempt to allocate a new mnemo block at a better location.
		 * Use NO_RECLAIM to avoid recursive compaction pressure.
		 */
		newcs = (cache_system_t*)MnemoAlloc(
			cs->size,
			(cs->size == (int)sizeof(cache_system_t)) ? MNEMO_FLAG_MALLOC : (MNEMO_FLAG_CACHE | MNEMO_FLAG_NO_RECLAIM),
			0, cs->name);

		if (newcs)
		{
			void* newpayload = (byte*)newcs + sizeof(cache_system_t);

			/* Copy header and payload to new location. */
			Q_memcpy(newcs, cs, sizeof(cache_system_t));
			Q_memcpy(newpayload, payload, cs->size - (int)sizeof(cache_system_t));

			/* Redirect user pointer. */
			newcs->user = cs->user;
			*newcs->user = newpayload;

			/* Re-init LRU links and insert new cs at MRU head. */
			newcs->lru_prev = newcs->lru_next = NULL;

			/* Free old block (param_2=1: internal move, no user pointer clear). */
			Cache_UnlinkLRU(cs);
			cs_live_bytes -= cs->size;
			MnemoFree(cs);

			/* Insert new block at MRU head. */
			Cache_MakeLRU(newcs);
			cs_live_bytes += newcs->size;

			moved_count++;
		}

next_cs:
		if (cs == stop) break;
		cs = next;
	}
}

/*
==============
Cache_Free  (= binary MnemoCacheFree, normal path)

Frees a cache entry: clears user->data, unlinks from LRU, frees mnemo block.

TODO: path that moves sprite VQ data between VRAM and RAM (user->data LSB=1)
==============
*/
void Cache_Free( cache_user_t* c )
{
	cache_system_t* cs;
	void* payload;

	if (!c->data)
		Sys_Error("MnemoCacheFree: not allocated");

	payload = Cache_UserPayload(c);
	if (!payload)
		Sys_Error("MnemoCacheFree: null payload");

	cs = ((cache_system_t*)payload) - 1;

	if (cs->flags & 1u)
		Sys_Error("Tried to free locked cache block!");

	cs_live_bytes -= cs->size;
	*cs->user = NULL;

	Cache_UnlinkLRU(cs);
	MnemoFree(cs);
}

int Cache_TotalUsed( void )
{
	return cs_live_bytes;
}

/*
==============
Cache_Check

- If user->data LSB=1 (moved block): treat as invalid, clear to NULL.
- If user->data valid: bring to MRU head, update frame stamp, return payload.
==============
*/
void* Cache_Check( cache_user_t* c )
{
	cache_system_t* cs;
	unsigned int raw;
	void* payload;

	if (!c->data)
		return NULL;

	raw = (unsigned int)c->data;
	if (raw & 1u)
	{
		c->data = NULL;
		return NULL;
	}

	payload = (void*)(raw);
	cs = ((cache_system_t*)payload) - 1;

	/* Move to MRU head and refresh frame stamp. */
	Cache_UnlinkLRU(cs);
	Cache_MakeLRU(cs);

	cs->timestamp = (unsigned int)gHostSpawnCount; /* DAT_0019af88 = server spawn count */
	cs->frame     = (unsigned int)host_framecount;

	return payload;
}

/*
==============
Cache_Alloc

Allocates a Mnemo-arena-backed cache entry with a purge loop on failure.
size = user data bytes (header added internally).
Returns payload pointer, or NULL (after calling Sys_Error) if OOM.
==============
*/
void* Cache_Alloc( cache_user_t* c, int size, char* name )
{
	cache_system_t* cs;
	unsigned int flags;
	int total;
	int purged;
	int nameLen;
	int nameOfs;

	if (c->data)
		Sys_Error("Cache_Alloc: already allocated");

	if (size <= 0)
		Sys_Error("Cache_Alloc: size %i", size);

	total = size + (int)sizeof(cache_system_t);
	flags = (total == (int)sizeof(cache_system_t)) ? MNEMO_FLAG_MALLOC : MNEMO_FLAG_CACHE;

	for (;;)
	{
		cs = (cache_system_t*)MnemoAlloc(total, flags, 0, name);
		if (cs)
			break;
		purged = MnemoPurge(0);
		if (purged == 0)	
		{
			Sys_Error("Out of cache memory.\n");
			return NULL;
		}
	}

	nameLen = Q_strlen(name);
	nameOfs = nameLen > 15 ? nameLen - 15 : 0;
	Q_strncpy(cs->name, name + nameOfs, 15);
	cs->name[15] = '\0';
	cs->timestamp = (unsigned int)gHostSpawnCount;
	cs->frame     = (unsigned int)host_framecount;
	cs->flags     = 0;
	cs->size      = total;
	cs->user      = (void**)c;

	cs->lru_prev = cs->lru_next = NULL;

	cs_live_bytes += total;
	cs_epoch_bytes += total;

	if (cs_epoch_frame != (unsigned int)host_framecount)
	{
		cs_epoch_frame = (unsigned int)host_framecount;
		cs_epoch_bytes = 0;
	}

	/* Set user->data = payload, then bring to MRU head. */
	c->data = (byte*)cs + sizeof(cache_system_t);
	Cache_MakeLRU(cs);

	return Cache_Check(c);
}

//============================================================================


/*
========================
Memory_Init
========================
*/
void Memory_Init( void* buf, int size )
{
	int p;
	int zonesize = DYNAMIC_SIZE;
	void* zonebuf;

	hunk_base = (byte*)buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	memset(&mnemo_stats, 0, sizeof(mnemo_stats));
	memset(&mnemo_head, 0, sizeof(mnemo_head));
	mnemo_sequence = 0;
	mnemo_last_chance_active = 0;
	mnemo_temp_danger = 0;
	mnemo_zone_sloppy_ptr = NULL;
	mnemo_zone_sloppy_left = 0;
	hunk_alloc_class = 2;
	mnemo_head.next = mnemo_head.prev = &mnemo_head;
	Mnemo_InitArena(buf, size);

	p = COM_CheckParm("-zone");
	if (p)
	{
		if (p < com_argc - 1)
			zonesize = Q_atoi(com_argv[p + 1]) * 1024;
		else
			Sys_Error("Memory_Init: you must specify a size in KB after -zone");
	}

	Cmd_AddCommand("report", Mnemo_ReportToFile);
	Cmd_AddCommand("sizes", Mnemo_Sizes_f);
	Cmd_AddCommand("summary", Mnemo_Summary_f);

	if (zonesize < 1000)
	{
		int allocSize = (zonesize + 3) & ~3;
		if (mnemo_zone_sloppy_left < allocSize || !mnemo_zone_sloppy_ptr)
		{
			int chunk = 0x1000;
			if (chunk < allocSize)
				chunk = allocSize;
			mnemo_zone_sloppy_ptr = MnemoAlloc(chunk, MNEMO_FLAG_HUNK, hunk_alloc_class, "sloppy");
			mnemo_zone_sloppy_left = chunk;
		}
		if (!mnemo_zone_sloppy_ptr)
			Sys_Error("Memory_Init: failed to allocate sloppy zone buffer");

		zonebuf = mnemo_zone_sloppy_ptr;
		mnemo_zone_sloppy_ptr = (byte*)mnemo_zone_sloppy_ptr + allocSize;
		mnemo_zone_sloppy_left -= allocSize;
	}
	else
	{
		zonebuf = MnemoAlloc(zonesize, MNEMO_FLAG_HUNK, hunk_alloc_class, "zone");
	}

	mainzone = (memzone_t*)zonebuf;
	Z_ClearZone(mainzone, zonesize);

	Cvar_RegisterVariable(&mnemo_cache);
	Cvar_RegisterVariable(&mnemo_report_file);
}

void Cache_Print_Models_And_Totals( void )
{
	char buf[50];
	cache_system_t* cd;
	cache_system_t* sortarray[512];
	long i = 0, j = 0;
	long totalbytes = 0;
	FILE* file = fopen(mem_dbgfile.string, "a");
	int subtot = 0;

	if (!file)
		return;



	memset(sortarray, 0, sizeof(sortarray));

	for (cd = cs_lru_tail; cd != NULL; cd = cd->lru_next)
	{
		if (strstr(cd->name, ".mdl") && i < 512)
			sortarray[i++] = cd;
	}

	qsort(sortarray, i, sizeof(cache_system_t*), CacheSystemCompare);

	fprintf(file, "\nCACHED MODELS:\n");

	for (j = 0; j < i; j++)
	{
		int k;
		mstudiotexture_t* ptexture;
		/* cs->user = &cache_user_t.data; payload = *cs->user (if not moved). */
		studiohdr_t* phdr = (studiohdr_t*)(*sortarray[j]->user);


		ptexture = (mstudiotexture_t*)(((char*)phdr) + phdr->textureindex);

		subtot = 0;
		for (k = 0; k < phdr->numtextures; k++)
			subtot += ptexture[k].width * ptexture[k].height + 768; // (256*3 for the palette)

		fprintf(file, "\t%16.16s : %s\n", CommatizeNumber(sortarray[j]->size, buf), sortarray[j]->name);
		totalbytes += sortarray[j]->size;
	}


	fprintf(file, "Total bytes in cache used by models:  %s\n", CommatizeNumber(totalbytes, buf));
	fclose(file);
}



/*
============
Cache_Print_Sounds_And_Totals

Prints out which sounds are in the cache, how much space they take, and the
directories that they're in.
============
*/

void Cache_Print_Sounds_And_Totals( void )
{
	char buf[50];
	cache_system_t* cd;
	cache_system_t* sortarray[MAX_SFX];
	long i = 0, j = 0;
	long totalsndbytes = 0;
	FILE* file = fopen(mem_dbgfile.string, "a");
	int subtot = 0;

	if (!file)
		return;

	memset(sortarray, 0, sizeof(sortarray));

	for (cd = cs_lru_tail; cd != NULL; cd = cd->lru_next)
	{
		if (strstr(cd->name, ".wav") && i < MAX_SFX)
			sortarray[i++] = cd;
	}

	qsort(sortarray, i, sizeof(cache_system_t*), CacheSystemCompare);

	fprintf(file, "\nCACHED SOUNDS:\n");


	//now process the sorted list.  (totals by directory)
	for (j = 0; j < i; j++)
	{

		fprintf(file, "\t%16.16s : %s\n", CommatizeNumber(sortarray[j]->size, buf), sortarray[j]->name);
		totalsndbytes += sortarray[j]->size;

		if (j + 1 == i || ComparePath1(sortarray[j]->name, sortarray[j + 1]->name) == 0)
		{
			char pathbuf[512];
			_splitpath(sortarray[j]->name, NULL, pathbuf, NULL, NULL);
			fprintf(file, "\tTotal Bytes used in \"%s\": %s\n", pathbuf, CommatizeNumber(totalsndbytes - subtot, buf));
			subtot = totalsndbytes;
		}

	}



	fprintf(file, "Total bytes in cache used by sound:  %s\n", CommatizeNumber(totalsndbytes, buf));
	fclose(file);
}
// mnemo.c -- the Mnemo memory arena: a single committed region carved into
// doubly-linked blocks with Pez slab pools, an LRU cache, and the hunk built on
// top of it. Z_zone.c keeps the small classic zone allocator; everything that
// touches the arena lives here.

#include "quakedef.h"
#include "winquake.h"
#include "studio.h"
#include "afile.h"
#include "kzap.h"
#include "pr_cmds.h"
#include "decal.h"
#include "r_studio.h"

#define	DYNAMIC_SIZE	0xc000

void Cache_FreeLow( int new_low_hunk );
void Cache_FreeHigh( int new_high_hunk );
void Cache_Free( cache_user_t* c, int keep );
void Cache_Compact( void );
void Cache_Flush( void );
void MnemoFree( void* ptr );

cvar_t mem_dbgfile = { "mem_dbgfile", ".\\mem.txt" };
cvar_t mnemo_cache = { "mnemo_cache", "5000000" };

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

#define MNEMO_MAX_ALLOC    4000000	// sanity cap on a single allocation
#define MNEMO_PEZ_MAX      0x100	// largest allocation served from a pez pool
#define MNEMO_TAG_LEN      11		// chars copied into a block tag (last byte terminates)
#define MNEMO_SLOPPY_MAX   1000		// smaller hunk/zone allocations bump off the sloppy pool
#define MNEMO_SLOPPY_CHUNK 0x1000	// sloppy pool replenish size
#define CACHE_FREE_MIN     0x800	// Cache_FreeLRU skips smaller blocks unless aggressive
#define CACHE_FLUSH_MAX    50	// Cache_FlushToDisk writes back at most this many blocks
#define CACHE_NAME_LEN     15		// chars kept of a resource name in a cache block

#define MNEMO_FIT_SLACK    0x1020	// a free block within this of the request is "good enough"
#define MNEMO_HUGE_SIZE    10000000	// best-fit seed: larger than any real block
#define MNEMO_PEZ_OVERHEAD 0x38		// per-pool bytes beyond the slab + free-stack arrays
#define MNEMO_PEZ_SLAB_PAD 0x18		// free-stack size padding before the 32-aligned slab

/* Every arena payload is 32-byte aligned. */
#define MNEMO_ALIGN( x )   (((x) + 0x1f) & ~0x1f)

/* ARGB colors and bar scales for the profilemeter overlay. */
#define METER_ARGB_TOTAL  0x80b03e80
#define METER_ARGB_USED   0x800000ff
#define METER_ARGB_BIG    0x80b00000
#define METER_ARGB_CLASS  0x80b000ff
#define METER_ARGB_TAG    0x8000ff00
#define METER_ARGB_SOUND  0x80ff8000
#define METER_ARGB_SPU    0x80ff00ff
#define METER_SCALE_TOTAL 0x03ff3e80
#define METER_SCALE_CLASS 15
#define METER_SCALE_TAG   5

typedef struct mnemo_header_s
{
	struct mnemo_header_s* prev;
	struct mnemo_header_s* next;
	int payload_size;
	short flags;
	short alloc_class;
	int sequence;
	char tag[12];
} mnemo_header_t;

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

// The whole allocator state is one struct so field offsets match the image.
typedef struct mnemo_state_s
{
	int				arena_size;
	byte*			arena_base;
	mnemo_header_t	head;				/* Block-list sentinel. */
	mnemo_header_t*	rover_cache;
	mnemo_header_t*	rover_hunk;
	mnemo_header_t*	rover_malloc;
	mnemo_header_t*	last_temp;
	int				reserved0;
	int				reserved1;
	int				alloc_seq;			/* Next block sequence number. */
	mnemo_purge_callback_t purge_callback;
	int				cache_bytes;		/* Live cache bytes. */
	struct cache_system_s* cache_mru;	/* Newest cache entry. */
	struct cache_system_s* cache_lru;	/* Oldest cache entry. */
	mnemo_pez_pool_t* pez_buckets[8];
	int				cache_epoch_frame;
	int				cache_epoch_bytes;
	int				alloc_attempts;
	int				last_chance;
} mnemo_state_t;

static mnemo_state_t g_mnemo;

static qboolean mnemo_arena_precommitted;
static int mnemo_temp_danger;
static byte* mnemo_zone_sloppy_ptr;
static int mnemo_zone_sloppy_left;
static int hunk_alloc_class;

void* g_edict_reserve;

extern int hunk_high_used;

static __forceinline int Mnemo_PezBucketIndex( int alignedSize );
static mnemo_pez_pool_t* Mnemo_PezCreatePool( int elemSize, int elemCount );
static __forceinline void* Mnemo_PezPopBucket( int bucketIdx );
static void Mnemo_InitArena( void *buf, int size );
static __forceinline int Mnemo_SelectAllocMode( unsigned int flags, int allocClass );
static __forceinline mnemo_header_t* Mnemo_PickFreeBlock( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* Mnemo_FindFreeBlock( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* Mnemo_FindCacheBlockByTag( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* Mnemo_FindZoneBlock( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* Mnemo_FindLockedBlock( int payload_size, unsigned int flags, int allocClass );
static mnemo_header_t* _AllocBlock( mnemo_header_t* block, int payload_size, unsigned int flags, int allocClass, const char* tag, int allocMode );
static void Mnemo_UpdateBlockLinks( mnemo_header_t* hdr );
static __forceinline void Mnemo_FreeArenaBlock( mnemo_header_t* hdr );
static mnemo_header_t* _FreeBlock_Coalesce( mnemo_header_t* hdr );
static mnemo_header_t* MnemoAlloc_Internal( int payload_size, unsigned int flags, int allocClass );
static __forceinline void Mnemo_FreeInline( void* ptr );
qboolean Mnemo_IsInArena( void* p );
static void Mnemo_DecommitBlock( mnemo_header_t* hdr );
static const char* Mnemo_FlagsToString( short flags );
static void Mnemo_Summary_f( void );
void Mnemo_ReportToFile( void );
static void Mnemo_FreeBlocksByTag( int tag );
void _FreeBlock( void );
static int Cache_FreeLRU( int aggressive );

static const char* Mnemo_FlagsToString( short flags )
{
	static char buffer[MAX_QPATH];
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

void Mnemo_ReportToFile( void )
{
	FILE* f;
	mnemo_header_t* node;
	const char* path = "\\PC\\mnemo_report.txt";

	f = fopen(path, "at");
	if (!f)
		f = fopen(path, "wt");

	if (f)
		fprintf(f, "Mnemo report in map %s:\n", sv.name[0] ? sv.name : "<none>");

	for (node = g_mnemo.head.next; node != &g_mnemo.head; node = node->next)
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

static int mnemo_profile_frame;

/*
============
Mnemo_ProfileMeter

Draw the on-screen memory meters (enabled by the profilemeter cvar bitmask):
1 = arena totals, 2 = by allocator class, 4 = by tag, 8 = sound/SPU memory.
============
*/
void Mnemo_ProfileMeter( void )
{
	mnemo_header_t* hdr;
	int used, freeTotal, freeBig;
	int cacheB, pezB, mallocB, hunkB;
	int sloppyB, mapB, bfileB, texcacheB;
	int flags, size;
	char c;
	int pm;
	int k;
	int spu, freeSpu, afile, ce;
	char buf[MAX_QPATH];

	used = 0;
	freeTotal = 0;
	freeBig = 0;
	cacheB = 0;
	pezB = 0;
	mallocB = 0;
	hunkB = 0;
	sloppyB = 0;
	mapB = 0;
	bfileB = 0;
	texcacheB = 0;

	mnemo_profile_frame++;

	if (profilemeter.value <= 0.0f)
		return;

	DCV_ClearMeters(0);

	for (hdr = g_mnemo.head.next; hdr != &g_mnemo.head; hdr = hdr->next)
	{
		flags = hdr->flags;
		size = hdr->payload_size;

		if (flags == 0)
		{
			freeTotal += size;
			if (freeBig < size)
				freeBig = size;
		}
		else
		{
			used += size;

			if (flags & MNEMO_FLAG_CACHE)  cacheB += size;
			if (flags & MNEMO_FLAG_PEZ)    pezB += size;
			if (flags & MNEMO_FLAG_HUNK)   hunkB += size;
			if (flags & MNEMO_FLAG_MALLOC) mallocB += size;

			if (strncmp(hdr->tag, "sloppy", 6) == 0)
				sloppyB += hdr->payload_size;

			if (strncmp(hdr->tag, "map", 4) == 0)
				mapB += hdr->payload_size;

			c = hdr->tag[0];

			if (c == '|' || c == '}')
				bfileB += hdr->payload_size;

			if (c == '>')
				texcacheB += hdr->payload_size;
		}
	}

	pm = (int)profilemeter.value;

	if (pm & 1)
	{
		sprintf(buf, "Mnemo total %dK", g_mnemo.arena_size / 1024);
		DCV_MeterText(METER_ARGB_TOTAL, 0, g_mnemo.arena_size / METER_SCALE_TOTAL, buf);

		sprintf(buf, "Mnemo used %dK", used / 1024);
		DCV_MeterText(METER_ARGB_USED, 0, used / METER_SCALE_TOTAL, buf);

		sprintf(buf, "Mnemo big %dK", freeBig / 1024);
		DCV_MeterText(METER_ARGB_BIG, used / METER_SCALE_TOTAL, (used + freeBig) / METER_SCALE_TOTAL, buf);

		pm = (int)profilemeter.value;
	}

	if (pm & 2)
	{
		k = cacheB / 1024;
		sprintf(buf, "Cache %dK", k);
		DCV_MeterText(METER_ARGB_BIG, 0, k / METER_SCALE_CLASS, buf);

		k = pezB / 1024;
		sprintf(buf, "Pez %dK", k);
		DCV_MeterText(METER_ARGB_BIG, 0, k / METER_SCALE_CLASS, buf);

		k = mallocB / 1024;
		sprintf(buf, "Malloc %dK", k);
		DCV_MeterText(METER_ARGB_CLASS, 0, k / METER_SCALE_CLASS, buf);

		k = hunkB / 1024;
		sprintf(buf, "Hunk %dK", k);
		DCV_MeterText(METER_ARGB_CLASS, 0, k / METER_SCALE_CLASS, buf);

		pm = (int)profilemeter.value;
	}

	if (pm & 4)
	{
		k = sloppyB / 1024;
		sprintf(buf, "Sloppy %dK", k);
		DCV_MeterText(METER_ARGB_TAG, 0, k / METER_SCALE_TAG, buf);

		k = mapB / 1024;
		sprintf(buf, "Map %dK", k);
		DCV_MeterText(METER_ARGB_TAG, 0, k / METER_SCALE_TAG, buf);

		k = bfileB / 1024;
		sprintf(buf, "BFile %dK", k);
		DCV_MeterText(METER_ARGB_TAG, 0, k / METER_SCALE_TAG, buf);

		k = texcacheB / 1024;
		sprintf(buf, "Texcache %dK", k);
		DCV_MeterText(METER_ARGB_TAG, 0, k / METER_SCALE_TAG, buf);

		pm = (int)profilemeter.value;
	}

	if (pm & 8)
	{
		S_GetDSPInfo(&spu, &freeSpu, &afile, &ce);

		sprintf(buf, "Sound in CE memory %dK", ce / 1024);
		DCV_MeterText(METER_ARGB_SOUND, 0, ce / METER_SCALE_TOTAL, buf);

		sprintf(buf, "Sound in SPU memory %dK", spu / 1024);
		DCV_MeterText(METER_ARGB_SOUND, 0, spu / METER_SCALE_TOTAL, buf);

		sprintf(buf, "AFile in SPU memory %dK", afile / 1024);
		DCV_MeterText(METER_ARGB_SPU, 0, afile / METER_SCALE_TOTAL, buf);

		sprintf(buf, "Free SPU memory %dK", freeSpu / 1024);
		DCV_MeterText(METER_ARGB_SPU, 0, freeSpu / METER_SCALE_TOTAL, buf);
	}
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

	for (node = g_mnemo.head.next; node != &g_mnemo.head; node = node->next)
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

	Con_Printf("Arena size: %d\n", g_mnemo.arena_size);
	Con_Printf("Total blocks: %d Total used: %d bytes Total free: %d bytes\n",
		totalBlocks, totalUsed, totalFree);
	Con_Printf("Biggest used block: %d Biggest free block: %d\n",
		biggestUsed, biggestFree);
	Con_Printf("Last sequence number: %d\n", lastSeq);
}

void Mnemo_SetPurgeCallback( mnemo_purge_callback_t callback )
{
	g_mnemo.purge_callback = callback;
}

/*
============
Mnemo_IsInArena

True if the pointer lies inside the arena; a NULL pointer counts as inside.
============
*/
qboolean Mnemo_IsInArena( void* p )
{
	if (p != 0 && (byte*)p < g_mnemo.arena_base)
		return FALSE;

	if ((byte*)p > g_mnemo.arena_base + g_mnemo.arena_size)
		return FALSE;

	return TRUE;
}

/*
============
Mnemo_FindPezPool

Return the pez pool whose slab holds this payload, or NULL for an ordinary
arena block.
============
*/
static __forceinline int Mnemo_PtrInPool( void* payload, mnemo_pez_pool_t* pool )
{
	if ((byte*)payload < pool->slab)
		return 0;
	return (byte*)payload < pool->slab + pool->count * pool->size;
}

static __forceinline mnemo_pez_pool_t* Mnemo_FindPezPool( void* payload )
{
	int i;
	mnemo_pez_pool_t* pool;

	for (i = 0; i < 8; i++)
	{
		for (pool = g_mnemo.pez_buckets[i]; pool; pool = pool->next)
		{
			if (Mnemo_PtrInPool(payload, pool))
				return pool;
		}
	}
	return NULL;
}

/*
============
Mnemo_BlockSize

Usable size of an allocated block: the pool element size for a pez block,
otherwise the arena header's payload size.
============
*/
int Mnemo_BlockSize( void* payload )
{
	mnemo_pez_pool_t* pool;

	pool = Mnemo_FindPezPool(payload);
	if (pool == NULL)
		return ((mnemo_header_t*)payload - 1)->payload_size;
	return pool->size;
}

/*
============
Mnemo_BlockSetName

Rename an arena block's tag; pez blocks carry no tag.
============
*/
void Mnemo_BlockSetName( void* payload, const char* name )
{
	mnemo_header_t* hdr;

	if (Mnemo_FindPezPool(payload) == NULL)
	{
		hdr = (mnemo_header_t*)payload - 1;
		strncpy(hdr->tag, name, sizeof(hdr->tag) - 1);
		hdr->tag[sizeof(hdr->tag) - 1] = 0;
	}
}


/* Page a block's storage back in. VirtualAlloc rounds the region to page
   bounds and is a no-op on pages that are already committed. */
static __forceinline void Mnemo_CommitBlock( void* base, int size )
{
	if (VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) == 0)
		mnemo_arena_precommitted = FALSE;
	else
		mnemo_arena_precommitted = TRUE;
}

static void Mnemo_DecommitBlock( mnemo_header_t* hdr )
{
	byte* decommitStart;
	byte* decommitEnd;

	decommitStart = (byte*)(((DWORD)((byte*)(hdr + 1)) + (MNEMO_PAGE_SIZE - 1)) & ~(MNEMO_PAGE_SIZE - 1));
	decommitEnd = (byte*)(((DWORD)((byte*)(hdr + 1)) + hdr->payload_size) & ~(MNEMO_PAGE_SIZE - 1));

	if (decommitStart < decommitEnd)
		VirtualFree(decommitStart, (int)(decommitEnd - decommitStart), MEM_DECOMMIT);
}

static void Mnemo_InitArena( void *buf, int size )
{
	byte* alignedBase;
	byte* alignedEnd;
	int remain;
	int i;
	mnemo_header_t* first;

	g_mnemo.arena_base = NULL;
	g_mnemo.arena_size = 0;
	mnemo_arena_precommitted = FALSE;
	g_mnemo.last_temp = NULL;
	g_mnemo.cache_epoch_frame = (unsigned int)host_framecount;
	g_mnemo.cache_epoch_bytes = 0;
	g_mnemo.alloc_attempts = 0;
	g_mnemo.rover_cache = g_mnemo.rover_hunk = g_mnemo.rover_malloc = NULL;

	for (i = 0; i < 8; i++)
		g_mnemo.pez_buckets[i] = NULL;

	/* Initialize sentinel: circular self-reference, flags mark it as root+used. */
	g_mnemo.head.prev = &g_mnemo.head;
	g_mnemo.head.next = &g_mnemo.head;
	g_mnemo.head.payload_size = 0;
	g_mnemo.head.flags = MNEMO_FLAG_ROOT | MNEMO_FLAG_USED;
	g_mnemo.head.alloc_class = 0;
	Q_strncpy(g_mnemo.head.tag, "Root Node", sizeof(g_mnemo.head.tag));
	g_mnemo.head.tag[sizeof(g_mnemo.head.tag) - 1] = 0;

	alignedBase = (byte*)(((DWORD)buf + 0x1f) & ~31);
	alignedEnd   = (byte*)(((DWORD)((byte*)buf + size)) & ~31);

	if (alignedEnd <= alignedBase)
		return;

	g_mnemo.arena_base = alignedBase;
	g_mnemo.arena_size = (int)(alignedEnd - alignedBase);
	remain = g_mnemo.arena_size - (int)sizeof(mnemo_header_t);

	if (remain <= 0)
		return;

	first = (mnemo_header_t*)g_mnemo.arena_base;
	first->prev = &g_mnemo.head;
	first->next = &g_mnemo.head;
	first->payload_size = remain;
	first->flags = 0;
	first->alloc_class = 0;
	first->sequence = ++g_mnemo.alloc_seq;
	Q_strncpy(first->tag, "_arena_", sizeof(first->tag));
	first->tag[sizeof(first->tag) - 1] = 0;
	Mnemo_DecommitBlock(first);

	/* Link into the circular sentinel list. */
	g_mnemo.head.next = first;
	g_mnemo.head.prev = first;

	/* Rover pointers start at the first (and only) free block. */
	g_mnemo.rover_cache = g_mnemo.rover_hunk = g_mnemo.rover_malloc = first;

	Mnemo_PezCreatePool(0x20, 0x400);
	Mnemo_PezCreatePool(0x40, 0x100);
	Mnemo_PezCreatePool(0x80, 0x100);

	g_edict_reserve = MnemoAlloc(100, MNEMO_FLAG_HUNK, 0, "edict reserve");
}

static __forceinline int Mnemo_SelectAllocMode( unsigned int flags, int allocClass )
{
	if (flags & 0x100)
		return 2;

	if (flags & 0x200)
		return 1;

	if (g_mnemo.alloc_seq < 100)
		return 2;

	if (flags & MNEMO_FLAG_TEMP)
		return 1;

	if (flags & MNEMO_FLAG_HUNK)
		return (allocClass < 3) ? 2 : 1;

	if (flags & MNEMO_FLAG_CACHE)
		return 1;
	return 2;
}

// Best-fit search for a free block sitting next to a cache block. Walks the
// whole block list, keeping the smallest free block big enough that borders a
// cache block on either side, and returns early once one fits within slack.
static mnemo_header_t* Mnemo_FindFreeBlock( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* cur;
	mnemo_header_t* neighbor;
	mnemo_header_t* best;
	int bestSize;
	int size;

	best = NULL;
	bestSize = MNEMO_HUGE_SIZE;

	for (cur = g_mnemo.head.next; cur != &g_mnemo.head; cur = cur->next)
	{
		if (cur->flags == 0)
		{
			size = cur->payload_size;

			if (size >= payload_size)
			{
				neighbor = cur->next;

				if ((neighbor->flags & MNEMO_FLAG_CACHE) && size < bestSize &&
				    (bestSize = size, best = cur, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;

				if ((cur->prev->flags & MNEMO_FLAG_CACHE) && size < bestSize &&
				    (best = cur, bestSize = size, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;
			}
		}
	}

	return best;
}

// Best-fit search for a free block bordering a hunk block of the same class,
// so a hunk allocation grows contiguously with its neighbours.
static mnemo_header_t* Mnemo_FindCacheBlockByTag( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* cur;
	mnemo_header_t* neighbor;
	mnemo_header_t* best;
	int bestSize;
	int size;

	best = NULL;
	bestSize = MNEMO_HUGE_SIZE;

	for (cur = g_mnemo.head.next; cur != &g_mnemo.head; cur = cur->next)
	{
		if (cur->flags == 0)
		{
			size = cur->payload_size;

			if (size >= payload_size)
			{
				neighbor = cur->next;

				if ((neighbor->flags & MNEMO_FLAG_HUNK) && neighbor->alloc_class == (short)allocClass &&
				    size < bestSize && (bestSize = size, best = cur, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;

				if ((cur->prev->flags & MNEMO_FLAG_HUNK) && cur->prev->alloc_class == (short)allocClass &&
				    size < bestSize && (best = cur, bestSize = size, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;
			}
		}
	}

	return best;
}

// Best-fit search for a free block bordering a malloc block.
static mnemo_header_t* Mnemo_FindZoneBlock( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* cur;
	mnemo_header_t* neighbor;
	mnemo_header_t* best;
	int bestSize;
	int size;

	best = NULL;
	bestSize = MNEMO_HUGE_SIZE;

	for (cur = g_mnemo.head.next; cur != &g_mnemo.head; cur = cur->next)
	{
		if (cur->flags == 0)
		{
			size = cur->payload_size;

			if (size >= payload_size)
			{
				neighbor = cur->next;

				if ((neighbor->flags & MNEMO_FLAG_MALLOC) && size < bestSize &&
				    (bestSize = size, best = cur, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;

				if ((cur->prev->flags & MNEMO_FLAG_MALLOC) && size < bestSize &&
				    (best = cur, bestSize = size, size <= payload_size + MNEMO_FIT_SLACK))
					return cur;
			}
		}
	}

	return best;
}

// Fallback search: any free block big enough, ignoring neighbours. Returns the
// last such block found, or the first one that fits within slack.
static mnemo_header_t* Mnemo_FindLockedBlock( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* cur;
	mnemo_header_t* best;

	best = NULL;

	for (cur = g_mnemo.head.next; cur != &g_mnemo.head; cur = cur->next)
	{
		if (cur->flags & MNEMO_FLAG_USED)
			continue;

		if (cur->payload_size < payload_size)
			continue;

		if (cur->payload_size >= MNEMO_HUGE_SIZE)
			continue;

		best = cur;

		if (payload_size + MNEMO_FIT_SLACK >= MNEMO_HUGE_SIZE)
			return cur;
	}

	return best;
}

static __forceinline mnemo_header_t* Mnemo_PickFreeBlock( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* block;

	block = NULL;

	if (flags & MNEMO_FLAG_CACHE)
		block = Mnemo_FindFreeBlock(payload_size, flags, allocClass);
	else if (flags & MNEMO_FLAG_HUNK)
		block = Mnemo_FindCacheBlockByTag(payload_size, flags, allocClass);
	else if (flags & MNEMO_FLAG_MALLOC)
		block = Mnemo_FindZoneBlock(payload_size, flags, allocClass);

	if (!block)
		block = Mnemo_FindLockedBlock(payload_size, flags, allocClass);

	return block;
}


static mnemo_header_t* _AllocBlock( mnemo_header_t* block, int payload_size, unsigned int flags, int allocClass, const char* tag, int allocMode )
{
	mnemo_header_t* split;
	int avail;

	avail = block->payload_size;

	if (payload_size + MNEMO_SPLIT_MIN <= avail)
	{
		if (allocMode == 2)
		{
			/* Carve the new block from the END of the free block; the front
			   remains as the free remnant. */
			int off = avail - payload_size;
			split = (mnemo_header_t*)((byte*)block + off);

			Mnemo_CommitBlock(split, (int)sizeof(mnemo_header_t));

			split->payload_size = payload_size;
			block->payload_size = off - (int)sizeof(mnemo_header_t);
			split->prev = block;
			split->next = block->next;
			block->next->prev = split;
			block->next = split;
			block = split;
		}
		else
		{
			/* Split the free remnant off after the payload. */
			split = (mnemo_header_t*)((byte*)(block + 1) + payload_size);

			Mnemo_CommitBlock(split, (int)sizeof(mnemo_header_t));

			split->payload_size = (avail - payload_size) - (int)sizeof(mnemo_header_t);
			split->flags = 0;
			block->payload_size = payload_size;
			split->prev = block;
			split->next = block->next;
			block->next->prev = split;
			block->next = split;
			strncpy(split->tag, block->tag, MNEMO_TAG_LEN);
			split->tag[MNEMO_TAG_LEN] = 0;
			split->sequence = block->sequence;
		}

		avail = block->payload_size;
	}

	Mnemo_CommitBlock((byte*)(block + 1), avail);

	block->flags = (unsigned short)(flags | MNEMO_FLAG_USED);
	block->alloc_class = (short)allocClass;
	block->sequence = g_mnemo.alloc_seq++;
	strncpy(block->tag, tag, MNEMO_TAG_LEN);
	block->tag[MNEMO_TAG_LEN] = 0;

	return block;
}


static void Mnemo_UpdateBlockLinks( mnemo_header_t* hdr )
{
	if (hdr->flags & MNEMO_FLAG_USED)
		Sys_Error("Oops!");

	if (g_mnemo.last_temp == hdr)
		g_mnemo.last_temp = NULL;

	/* malloc rover: take this block unless it falls strictly between the
	   largest and smallest known free blocks. */
	if (g_mnemo.rover_malloc == NULL ||
	    (g_mnemo.rover_cache != NULL && g_mnemo.rover_cache->payload_size >= hdr->payload_size) ||
	    (g_mnemo.rover_hunk != NULL && g_mnemo.rover_hunk->payload_size <= hdr->payload_size))
		g_mnemo.rover_malloc = hdr;

	/* cache rover tracks the largest free block, hunk rover the smallest. */
	if (g_mnemo.rover_cache == NULL || g_mnemo.rover_cache->payload_size < hdr->payload_size)
		g_mnemo.rover_cache = hdr;

	if (g_mnemo.rover_hunk == NULL || g_mnemo.rover_hunk->payload_size > hdr->payload_size)
		g_mnemo.rover_hunk = hdr;
}


static mnemo_header_t* _FreeBlock_Coalesce( mnemo_header_t* hdr )
{
	mnemo_header_t* next;
	mnemo_header_t* prev;

	next = hdr->next;
	prev = hdr->prev;

	/* Absorb next block if free. */
	if (next->flags == 0)
	{
		hdr->payload_size += next->payload_size + (int)sizeof(mnemo_header_t);
		if (g_mnemo.rover_cache   == next) g_mnemo.rover_cache   = NULL;
		if (g_mnemo.rover_hunk    == next) g_mnemo.rover_hunk    = NULL;
		if (g_mnemo.last_temp     == next) g_mnemo.last_temp     = NULL;
		if (g_mnemo.rover_malloc  == next) g_mnemo.rover_malloc  = NULL;
		next->prev->next = next->next;
		next->next->prev = next->prev;
	}

	/* Absorb into prev block if free. */
	if (prev->flags == 0)
	{
		prev->payload_size += hdr->payload_size + (int)sizeof(mnemo_header_t);
		if (g_mnemo.rover_cache   == hdr) g_mnemo.rover_cache   = NULL;
		if (g_mnemo.rover_hunk    == hdr) g_mnemo.rover_hunk    = NULL;
		if (g_mnemo.last_temp     == hdr) g_mnemo.last_temp     = NULL;
		if (g_mnemo.rover_malloc  == hdr) g_mnemo.rover_malloc  = NULL;
		hdr->prev->next = hdr->next;
		hdr->next->prev = hdr->prev;
		hdr = prev;
	}

	return hdr;
}

static __forceinline void Mnemo_FreeArenaBlock( mnemo_header_t* hdr )
{
	mnemo_header_t* merged;
	byte* decommitStart;
	byte* decommitEnd;
	char tag[MAX_QPATH];
	short flags;

	flags = hdr->flags;

	if (!(flags & MNEMO_FLAG_USED))
	{
		Sys_Error("_FreeBlock: block doesn't appear to be in use!");
		flags = hdr->flags;
	}

	if (flags & MNEMO_FLAG_ROOT)
		Sys_Error("_FreeBlock: attempt to free root block!");

	hdr->flags = 0;
	sprintf(tag, "(%s)", hdr->tag);
	strncpy(hdr->tag, tag, MNEMO_TAG_LEN);
	hdr->tag[MNEMO_TAG_LEN] = 0;
	hdr->sequence = g_mnemo.alloc_seq++;

	merged = _FreeBlock_Coalesce(hdr);

	decommitStart = (byte*)(((DWORD)((byte*)(merged + 1)) + (MNEMO_PAGE_SIZE - 1)) & ~(MNEMO_PAGE_SIZE - 1));
	decommitEnd = (byte*)(((DWORD)((byte*)(merged + 1)) + merged->payload_size) & ~(MNEMO_PAGE_SIZE - 1));
	if (decommitStart < decommitEnd)
		VirtualFree(decommitStart, (int)(decommitEnd - decommitStart), MEM_DECOMMIT);

	Mnemo_UpdateBlockLinks(merged);
}

static __forceinline int Mnemo_PezBucketIndex( int alignedSize )
{
	int bucketSize = 32;
	int bucketIdx = 0;

	if (32 < alignedSize)
	{
		do
		{
			bucketIdx++;
			bucketSize <<= 1;
		} while (bucketSize < alignedSize);
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
	int bucketIdx;
	int i;

	alignedSize = MNEMO_ALIGN(elemSize);

	sprintf(tag, "%d-Pez", alignedSize);

	pool = (mnemo_pez_pool_t*)MnemoAlloc(elemCount * alignedSize + elemCount * sizeof(void*) + MNEMO_PEZ_OVERHEAD,
	                                     MNEMO_FLAG_PEZ, 0, tag);

	if (pool == NULL)
		return NULL;

	pool->stack = (void**)((byte*)pool + sizeof(mnemo_pez_pool_t));
	pool->slab  = (byte*)pool + MNEMO_ALIGN(elemCount * sizeof(void*) + MNEMO_PEZ_SLAB_PAD);

	for (i = 0; i < elemCount; i++)
		pool->stack[i] = pool->slab + alignedSize * i;

	pool->top   = (unsigned int)elemCount;
	pool->size  = alignedSize;
	pool->count = (unsigned int)elemCount;

	bucketIdx = Mnemo_PezBucketIndex((int)alignedSize);

	pool->next = g_mnemo.pez_buckets[bucketIdx];
	g_mnemo.pez_buckets[bucketIdx] = pool;
	return pool;
}


int Mnemo_LastChanceActive( void )
{
	return g_mnemo.last_chance;
}

/* Pop a payload off the free stack of any pool in this bucket, creating a new
   pool if none has a spare slot. */
static __forceinline void* Mnemo_PezPopBucket( int bucketIdx )
{
	mnemo_pez_pool_t* pool;
	void* payload;
	int elemSize;
	int i;

	for (pool = g_mnemo.pez_buckets[bucketIdx]; pool; pool = pool->next)
	{
		if (pool->top == 0)
			payload = NULL;
		else
		{
			pool->top--;
			payload = pool->stack[pool->top];
			pool->stack[pool->top] = NULL;
		}

		if (payload)
			return payload;
	}

	elemSize = 32;
	for (i = bucketIdx; i != 0; i--)
		elemSize <<= 1;

	pool = Mnemo_PezCreatePool(elemSize, MNEMO_PAGE_SIZE / elemSize);

	if (pool == NULL)
		return NULL;

	if (pool->top == 0)
		return NULL;

	pool->top--;
	payload = pool->stack[pool->top];
	pool->stack[pool->top] = NULL;
	return payload;
}

static __forceinline void Mnemo_FreeInline( void* ptr )
{
	mnemo_pez_pool_t* pool;
	qboolean inArena;

	if ((byte*)ptr < g_mnemo.arena_base ||
	    g_mnemo.arena_base + g_mnemo.arena_size <= (byte*)ptr)
		inArena = FALSE;
	else
		inArena = TRUE;

	if (inArena)
	{
		pool = Mnemo_FindPezPool(ptr);

		if (pool == NULL)
		{
			Mnemo_FreeArenaBlock((mnemo_header_t*)ptr - 1);
		}
		else
		{
			if (!ptr)
				Sys_Error("Argh!");

			if (pool->top == pool->count)
				Sys_Error("Pez stack underflowed!\n");

			pool->stack[pool->top] = ptr;
			pool->top++;
		}
	}
	else
	{
		LocalFree(ptr);
	}
}

void* MnemoAlloc( int size, unsigned int flags, int allocClass, const char* tag )
{
	int payload_size;
	int allocMode;
	void* pez;
	mnemo_header_t* block;

	if (size > MNEMO_MAX_ALLOC)
		Sys_Error("Absurd MnemoAlloc(%d, %d, %d, %d)", size, flags, allocClass, (int)tag);

	mnemo_temp_danger = 0;
	g_mnemo.alloc_attempts++;

	payload_size = MNEMO_ALIGN(size);

	if ((flags & MNEMO_FLAG_TEMP) && g_mnemo.last_temp)
		Mnemo_FreeInline((mnemo_header_t*)g_mnemo.last_temp + 1);

	if (!(MNEMO_PEZ_MAX < payload_size || (flags & MNEMO_FLAG_CACHE) || (flags & MNEMO_FLAG_TEMP) || allocClass > 1))
	{
		int bucketIdx = Mnemo_PezBucketIndex(payload_size);
		pez = Mnemo_PezPopBucket(bucketIdx);

		if (pez != NULL &&
		    ((byte*)pez < g_mnemo.arena_base || g_mnemo.arena_base + g_mnemo.arena_size < (byte*)pez))
		{
			DebugBreak();
			bucketIdx = Mnemo_PezBucketIndex(payload_size);
			pez = Mnemo_PezPopBucket(bucketIdx);
		}

		if (pez)
			return pez;
	}

	allocMode = Mnemo_SelectAllocMode(flags, allocClass);
	block = MnemoAlloc_Internal(payload_size, flags, allocClass);

	g_mnemo.last_chance = 0;

	if (!block)
	{
		if ((flags & MNEMO_FLAG_NO_RECLAIM) == 0)
		{
			Mnemo_FlagsToString((unsigned short)flags);

			if (developer.value > 0.0f)
				Mnemo_ReportToFile();
		}
		return NULL;
	}

	if (block->flags & MNEMO_FLAG_USED)
		Sys_Error("Ooops!");

	block = _AllocBlock(block, payload_size, flags, allocClass, tag, allocMode);

	if (flags & MNEMO_FLAG_TEMP)
	{
		if (!(block->flags & MNEMO_FLAG_USED))
			Sys_Error("Oops!");

		if (g_mnemo.last_temp)
			Sys_Error("Are we supposed to have multiple temp blocks?");

		g_mnemo.last_temp = block;
	}

	if (g_mnemo.rover_cache  == block) g_mnemo.rover_cache  = NULL;
	if (g_mnemo.rover_hunk   == block) g_mnemo.rover_hunk   = NULL;
	if (g_mnemo.rover_malloc == block) g_mnemo.rover_malloc = NULL;

	if ((flags & MNEMO_FLAG_MALLOC) == 0)
		memset((void*)(block + 1), 0, payload_size);

	return (void*)(block + 1);
}

void* MnemoAllocDbg( int size, const char* srcFile, int srcLine )
{
	static char tag[MAX_QPATH];
	const char* base;

	base = strrchr(srcFile, '\\');
	if (!base)
		base = strrchr(srcFile, '/');
	if (base)
		srcFile = base + 1;

	sprintf(tag, "%d, %s", srcLine, srcFile);

	return MnemoAlloc(size, MNEMO_FLAG_MALLOC, 0, tag);
}

/*
===================
calloc

The C heap is tagged with its call site and backed by the arena. Every calloc
call site expands (via the dreamcast_crt.h macro) to pass __FILE__/__LINE__.
(free lives with the other CRT shims in dreamcast_crt.cpp.)
===================
*/
#undef calloc
void* calloc( unsigned int num, unsigned int size, const char* file, int line )
{
	static char tag[MAX_QPATH];
	const char* base;
	void* p;

	base = strrchr(file, '\\');
	if (!base)
		base = strrchr(file, '/');
	if (base)
		file = base + 1;

	sprintf(tag, "%d-%s", line, file);

	p = MnemoAlloc(size * num, MNEMO_FLAG_MALLOC, 0, tag);
	if (p)
		memset(p, 0, size * num);

	return p;
}

/*
=================
MnemoRealloc

Grow a scratch allocation, retaining its tag, flags and allocation class.
The old contents are discarded when the block grows.
=================
*/
void* MnemoRealloc( void* oldPtr, int sizeBytes )
{
	mnemo_header_t*	hdr;
	unsigned int		flags;
	int				allocClass;
	char				tag[sizeof(((mnemo_header_t*)0)->tag)];

	if (Mnemo_BlockSize(oldPtr) >= sizeBytes)
		return oldPtr;

	hdr = (mnemo_header_t*)oldPtr - 1;
	if (Mnemo_FindPezPool(oldPtr))
		strcpy(tag, "pez");
	strcpy(tag, hdr->tag);

	if (Mnemo_FindPezPool(oldPtr))
		flags = MNEMO_FLAG_PEZ;
	else
		flags = (short)hdr->flags;

	if (Mnemo_FindPezPool(oldPtr))
		allocClass = 0;
	else
		allocClass = (short)hdr->alloc_class;

	MnemoFree(oldPtr);
	return MnemoAlloc(sizeBytes, flags, allocClass, tag);
}

void MnemoFreeDbg( void* ptr )
{
	MnemoFree(ptr);
}

void MnemoFree( void* ptr )
{
	Mnemo_FreeInline(ptr);
}

void _FreeBlock( void )
{
	if (g_mnemo.last_temp)
		Mnemo_FreeInline((void*)(g_mnemo.last_temp + 1));
}

void MnemoShrink( void* ptr, int newsize )
{
	mnemo_pez_pool_t* pool;
	mnemo_header_t* hdr;
	mnemo_header_t* split;
	unsigned int alignedSize;
	char tag[MAX_QPATH];

	/* Pez blocks are fixed-size and never shrink. */
	pool = Mnemo_FindPezPool(ptr);

	if (pool == NULL)
	{
		alignedSize = MNEMO_ALIGN(newsize);
		hdr = (mnemo_header_t*)ptr - 1;

		/* Only split when the freed tail is big enough for its own block. */
		if ((int)(alignedSize + MNEMO_SPLIT_MIN) <= hdr->payload_size)
		{
			split = (mnemo_header_t*)((byte*)ptr + alignedSize);
			split->payload_size = (hdr->payload_size - alignedSize) - (int)sizeof(mnemo_header_t);
			split->flags = 0;
			hdr->payload_size = alignedSize;
			split->prev = hdr;
			split->next = hdr->next;
			hdr->next->prev = split;
			hdr->next = split;
			sprintf(tag, "(%s)", hdr->tag);
			strncpy(split->tag, tag, MNEMO_TAG_LEN);
			split->tag[MNEMO_TAG_LEN] = 0;
			split->sequence = hdr->sequence;
		}
	}
}

static void Mnemo_FreeBlocksByTag( int tag )
{
	mnemo_header_t* cur;

	/* Free every allocated block of this class, restarting the scan after each
	   free since freeing relinks the list. */
	while (1)
	{
		cur = g_mnemo.head.next;

		if (cur == &g_mnemo.head)
			return;

		while ((cur->flags & MNEMO_FLAG_USED) == 0 || cur->alloc_class != (short)tag)
		{
			cur = cur->next;

			if (cur == &g_mnemo.head)
				return;
		}

		Mnemo_FreeInline((void*)(cur + 1));
	}
}

//============================================================================

int		hunk_high_used;

qboolean	hunk_tempactive;
int		hunk_tempmark;

void R_FreeTextures( void );


/*
===================
Mnemo_SloppyAlloc

Small allocations bump off a shared "sloppy" pool that is never individually
freed, replenished from the arena a page at a time. Backs both small hunk
allocations and small zones.
===================
*/
static __forceinline void* Mnemo_SloppyAlloc( int size )
{
	int aligned;
	void* result;

	aligned = (size + 3) & ~3;

	if (mnemo_zone_sloppy_left < aligned || mnemo_zone_sloppy_ptr == NULL)
	{
		mnemo_zone_sloppy_left = MNEMO_SLOPPY_CHUNK;
		if (aligned > MNEMO_SLOPPY_CHUNK)
			mnemo_zone_sloppy_left = aligned;

		mnemo_zone_sloppy_ptr = MnemoAlloc(mnemo_zone_sloppy_left, MNEMO_FLAG_HUNK, hunk_alloc_class, "sloppy");
	}

	result = mnemo_zone_sloppy_ptr;
	mnemo_zone_sloppy_left -= aligned;
	mnemo_zone_sloppy_ptr += aligned;
	return result;
}

/*
===================
Hunk_AllocName
===================
*/
void* Hunk_AllocName(int size, char* name)
{
	if (size < MNEMO_SLOPPY_MAX)
		return Mnemo_SloppyAlloc(size);

	return MnemoAlloc(size, MNEMO_FLAG_HUNK, hunk_alloc_class, name);
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

void Hunk_Check( void )
{
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
		Mnemo_FreeBlocksByTag(cls);

	hunk_alloc_class = mark;
	mnemo_zone_sloppy_ptr = NULL;
	mnemo_zone_sloppy_left = 0;
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

#define CACHE_LOCKED  1  // block is locked against eviction

static __forceinline void Cache_MoveToMRU( cache_system_t* cs )
{
	cs->lru_next = NULL;
	cs->lru_prev = g_mnemo.cache_mru;        /* old head is now one step older */

	if (g_mnemo.cache_mru)
		g_mnemo.cache_mru->lru_next = cs;    /* old head points toward cs (newer) */

	g_mnemo.cache_mru = cs;

	if (!g_mnemo.cache_lru)
		g_mnemo.cache_lru = cs;              /* first entry: init tail */
}

static __forceinline void Cache_UnlinkLRU( cache_system_t* cs )
{
	if (g_mnemo.cache_mru == cs)
		g_mnemo.cache_mru = cs->lru_prev;    /* new head = older neighbor */

	if (g_mnemo.cache_lru == cs)
		g_mnemo.cache_lru = cs->lru_next;    /* new tail = newer neighbor */

	if (cs->lru_next)
		cs->lru_next->lru_prev = cs->lru_prev;

	if (cs->lru_prev)
		cs->lru_prev->lru_next = cs->lru_next;
}

/*
============
Cache_MakeLRU

Initialise a freshly allocated cache block: name (the tail of the resource
path), timestamp/frame stamps, and clear the flags.
============
*/
static void Cache_MakeLRU( cache_system_t* cs, const char* name )
{
	int skip;

	skip = strlen(name) - CACHE_NAME_LEN;

	if (skip < 0)
		skip = 0;

	strncpy(cs->name, name + skip, CACHE_NAME_LEN);
	cs->name[CACHE_NAME_LEN] = 0;

	if (skip > 7)
		cs->name[0] = name[7];

	cs->timestamp = gHostSpawnCount;
	cs->frame = host_framecount;
	cs->flags = 0;
}

/*
============
Cache_Lock / Cache_Unlock

Lock a cache entry against eviction (sets cs->flags bit 0).
Returns 0 if block has been moved (must call Cache_Check first).
============
*/
int Cache_Lock( cache_user_t* c )
{
	cache_system_t* cs;
	unsigned int data;
	unsigned int payload;

	data = (unsigned int)c->data;
	payload = 0;
	if ((data & 1) == 0)
		payload = data;
	if (payload == 0)
		return 0;
	if (data & 1)
		data = 0;
	cs = (cache_system_t*)data - 1;
	cs->flags |= CACHE_LOCKED;
	return 1;
}

int Cache_Unlock( cache_user_t* c )
{
	cache_system_t* cs;
	unsigned int data;
	unsigned int payload;

	data = (unsigned int)c->data;
	payload = 0;
	if ((data & 1) == 0)
		payload = data;
	if (payload == 0)
		return 0;
	if (data & 1)
		data = 0;
	cs = (cache_system_t*)data - 1;
	cs->flags &= ~CACHE_LOCKED;
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
int Cache_FreeAll( void )
{
	cache_system_t* cs;
	cache_system_t* next;

	for (cs = g_mnemo.cache_lru; cs != NULL; cs = next)
	{
		next = cs->lru_next;

		if (((unsigned int)*cs->user & 1u) == 0)  /* skip moved blocks */
			Cache_Free((cache_user_t*)cs->user, 0);
	}

	return 0;
}

static int MnemoCacheMove( cache_system_t* cs );
static __forceinline int Cache_MoveToAFile( cache_system_t* cs );
static __forceinline int MnemoCacheRelocate( cache_system_t* cs );

/*
============
Cache_FlushToDisk

Write back every cache block that still has somewhere to go, so the block can
be dropped now and reloaded later instead of holding on to arena space.
============
*/
int Cache_FlushToDisk( void )
{
	cache_system_t* cs;
	cache_system_t* next;
	cache_system_t* last;
	int count;

	count = 0;
	last = g_mnemo.cache_mru;

	for (cs = g_mnemo.cache_lru; cs != NULL; cs = next)
	{
		/* The move frees this block, so remember the link first. */
		next = cs->lru_next;

		if (((unsigned int)*cs->user & 1u) == 0)
		{
			/* Park the payload in audio-block memory when there is somewhere
			   to put it, otherwise just shuffle the block up so the space it
			   leaves behind joins the hole next to it. */
			if (AFile_HasRoomFor(cs->size))
				count += Cache_MoveToAFile(cs);
			else
				count += MnemoCacheRelocate(cs);
		}

		if (cs == last || count > CACHE_FLUSH_MAX)
			break;
	}

	return count;
}

int Cache_FreeAllLRU( void )
{
	int count;
	int freed;

	count = 0;
	freed = Cache_FreeLRU(1);

	while (freed != 0)
	{
		count++;
		freed = Cache_FreeLRU(1);
	}

	return count;
}

/*
============
Cache_FreeStale

Free LRU-tail blocks left over from a previous map (timestamp mismatch).
============
*/
int Cache_FreeStale( void )
{
	while (g_mnemo.cache_lru != NULL &&
	       g_mnemo.cache_lru->timestamp != (unsigned int)gHostSpawnCount)
		Cache_Free((cache_user_t*)g_mnemo.cache_lru->user, 0);

	return 0;
}

/*
============
Cache_FlushUnlocked

Re-touch the GPU textures of every resident, unmoved cache block.
============
*/
void Cache_FlushUnlocked( void )
{
	cache_system_t* cs;
	unsigned int data;

	for (cs = g_mnemo.cache_mru; cs != NULL; cs = cs->lru_prev)
	{
		/* A juggled block (low bit set) lives in audio memory, not the arena,
		   so its textures aren't resident to re-touch. */
		data = (unsigned int)*cs->user;

		if (((data & 1) ? 0 : data) != 0)
			Mod_TouchStudioTextures((void*)((data & 1) ? 0 : data));
	}
}

/*
============
Cache_FreeLRU

Pass 1: LRU tail -> MRU head (lru_next).
  Find unlocked block where timestamp != gHostSpawnCount OR frame != host_framecount.
Pass 2: MRU head -> LRU tail (lru_prev).
  Find any unlocked block regardless of stamp 
Returns cs->size of freed block, or 0 if nothing purgeable.
============
*/
static int Cache_FreeLRU( int aggressive )
{
	cache_system_t* cs;
	int bytes;

	/* Pass 1: from oldest toward newest; skip blocks that are current
	 * (same spawn-count AND same host_framecount). */
	for (cs = g_mnemo.cache_lru; cs != NULL; cs = cs->lru_next)
	{
		if (cs->flags & 1u)
			continue;  /* locked */

		if (cs->timestamp == (unsigned int)gHostSpawnCount &&
		    cs->frame == (unsigned int)host_framecount)
			continue;  /* current map, current frame — keep */

		if (!aggressive && cs->size <= CACHE_FREE_MIN)
			continue;

		bytes = cs->size;
		Cache_Free((cache_user_t*)cs->user, 0);
		return bytes;
	}

	/* Pass 2: from newest toward oldest; any unlocked block */
	for (cs = g_mnemo.cache_mru; cs != NULL; cs = cs->lru_prev)
	{
		if (cs->flags & 1u)
			continue;  /* locked */

		if (!aggressive && cs->size <= CACHE_FREE_MIN)
			continue;

		bytes = cs->size;
		Cache_Free((cache_user_t*)cs->user, 0);
		return bytes;
	}

	return 0;
}

/*
 * MnemoCacheMove
 *
 * Juggle a cache block out of the arena into AFile audio-block memory: create
 * an AFile from the payload, replace the block with a tiny placeholder whose
 * user->data is flagged "moved", and free the original. Cache_Check brings it
 * back on demand. Returns 1 if the block was moved.
 */
static int MnemoCacheMove( cache_system_t* cs )
{
	if (cs->flags & CACHE_LOCKED)
		return 0;

	if (((unsigned int)*cs->user & 1u) != 0)
		return 0;

	if (!AFile_HasRoomFor(cs->size))
		return 0;

	return Cache_MoveToAFile(cs);
}

static __forceinline int Cache_MoveToAFile( cache_system_t* cs )
{
	cache_system_t* newcs;

	if (AFile_LoadOrCreate((char*)cs, (byte*)cs + sizeof(cache_system_t),
	                       cs->size - (int)sizeof(cache_system_t), 1) == NULL)
		return 0;

	newcs = (cache_system_t*)MnemoAlloc(sizeof(cache_system_t), MNEMO_FLAG_MALLOC, 0, (char*)cs);
	memcpy(newcs, cs, sizeof(cache_system_t));
	newcs->size = sizeof(cache_system_t);
	g_mnemo.cache_bytes += sizeof(cache_system_t);
	newcs->lru_next = NULL;
	newcs->lru_prev = NULL;
	g_mnemo.cache_epoch_bytes += newcs->size;

	Cache_MakeLRU(newcs, (char*)cs);
	Cache_Free((cache_user_t*)cs->user, 1);

	*newcs->user = (void*)((byte*)newcs + sizeof(cache_system_t));
	*(unsigned int*)newcs->user |= 1u;

	Cache_UnlinkLRU(newcs);
	Cache_MoveToMRU(newcs);

	newcs->timestamp = gHostSpawnCount;
	newcs->frame = host_framecount;

	return 1;
}

/*
 * MnemoCacheRelocate
 *
 * Move a cache block to a fresh spot of the same size and free the old one, so
 * the space it was sitting on can merge with whatever is next to it. The block
 * keeps its payload, so nothing has to be read back in later. Returns 1 if the
 * block was moved.
 */
static __forceinline int MnemoCacheRelocate( cache_system_t* cs )
{
	cache_system_t* newcs;

	newcs = (cache_system_t*)MnemoAlloc(cs->size, MNEMO_FLAG_CACHE | MNEMO_FLAG_NO_RECLAIM,
	                                    0, (char*)cs);
	if (newcs == NULL)
		return 0;

	memcpy(newcs, cs, cs->size);
	g_mnemo.cache_bytes += newcs->size;
	newcs->lru_next = NULL;
	newcs->lru_prev = NULL;
	g_mnemo.cache_epoch_bytes += newcs->size;

	Cache_Free((cache_user_t*)cs->user, 1);

	*newcs->user = (void*)((byte*)newcs + sizeof(cache_system_t));

	Cache_UnlinkLRU(newcs);
	Cache_MoveToMRU(newcs);

	newcs->timestamp = gHostSpawnCount;
	newcs->frame = host_framecount;

	return 1;
}

/* Juggle out an unlocked cache block that neighbours a free hole. */
static __forceinline int Mnemo_TryCacheMoveBlock( mnemo_header_t* neighbor )
{
	static int (*volatile moveblock)(cache_system_t*) = MnemoCacheMove;
	cache_system_t* cs;

	if ((neighbor->flags & MNEMO_FLAG_CACHE) == 0)
	{
		Sys_Error("MnemoCacheMove called on non-cache block");
		return 0;
	}

	cs = (cache_system_t*)(neighbor + 1);

	if (cs->flags & CACHE_LOCKED)
		return 0;

	return moveblock(cs);
}

/*
 * MnemoCacheMoveScan
 *
 * Try to juggle out the cache blocks bordering the largest free hole
 * (rover_cache), repeating while progress is made.
 */
static int MnemoCacheMoveScan( void )
{
	mnemo_header_t* rover;
	mnemo_header_t* neighbor;
	int result;
	int moved;

	result = 0;

	do
	{
		rover = g_mnemo.rover_cache;
		moved = 0;

		if (rover == NULL)
			return result;

		neighbor = rover->next;
		if (neighbor->flags & MNEMO_FLAG_CACHE)
		{
			if (Mnemo_TryCacheMoveBlock(neighbor))
			{
				moved = 1;
				result = 1;
			}
		}

		neighbor = rover->prev;
		if (neighbor->flags & MNEMO_FLAG_CACHE)
		{
			if (Mnemo_TryCacheMoveBlock(neighbor))
			{
				moved = 1;
				result = 1;
			}
		}
	} while (moved);

	return result;
}

/*
 * MnemoCacheMoveList
 *
 * Walk the whole arena; for every free block, juggle out its cache neighbours,
 * restarting the walk after each move. Consolidates free space.
 */
static int MnemoCacheMoveList( void )
{
	mnemo_header_t* cur;
	mnemo_header_t* neighbor;
	int result;

	result = 0;

restart:
	cur = g_mnemo.head.next;

	while (1)
	{
		if (cur->flags == 0)
		{
			neighbor = cur->next;
			if (neighbor->flags & MNEMO_FLAG_CACHE)
			{
				if (Mnemo_TryCacheMoveBlock(neighbor))
				{
					result = 1;
					goto restart;
				}
			}

			neighbor = cur->prev;
			if (neighbor->flags & MNEMO_FLAG_CACHE)
			{
				if (Mnemo_TryCacheMoveBlock(neighbor))
				{
					result = 1;
					goto restart;
				}
			}
		}

		if (cur == &g_mnemo.head)
			return result;

		cur = cur->next;
	}
}

/*
 * MnemoAlloc_Internal
 *
 * Find a free block large enough for the request, escalating through the
 * reclaim stages until one is available: texture eviction, cache end/list
 * compaction, LRU cache freeing, a full shrink, and finally sacrificing a
 * dangling temp block. Returns the free block header (uncarved) or NULL.
 */
static mnemo_header_t* MnemoAlloc_Internal( int payload_size, unsigned int flags, int allocClass )
{
	mnemo_header_t* block;
	int freed;

	block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
	if (block)
		return block;

	if (flags & MNEMO_FLAG_NO_RECLAIM)
		return NULL;

	while (DC_ReclaimTextureSlot() != 0)
		;
	block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
	if (block)
		return block;

	if (MnemoCacheMoveScan())
	{
		block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
		if (block)
			return block;
	}

	if (MnemoCacheMoveList())
	{
		block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
		if (block)
			return block;
	}

	do
	{
		freed = Cache_FreeLRU(0);
		block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
		if (block)
			return block;
	} while (freed != 0);

	g_mnemo.last_chance = 1;
	Bshrink_all();

	block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);

	if (!block && g_mnemo.last_temp)
	{
		mnemo_temp_danger = 1;
		Mnemo_FreeInline((mnemo_header_t*)g_mnemo.last_temp + 1);
		block = Mnemo_PickFreeBlock(payload_size, flags, allocClass);
	}

	return block;
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

	stop = g_mnemo.cache_mru;

	cs = g_mnemo.cache_lru;
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
			g_mnemo.cache_bytes -= cs->size;
			MnemoFree(cs);

			/* Insert new block at MRU head. */
			Cache_MoveToMRU(newcs);
			g_mnemo.cache_bytes += newcs->size;

			moved_count++;
		}

next_cs:
		if (cs == stop) break;
		cs = next;
	}
}

/* A cached sprite/model resource is tagged with one of these in its header. */
#define CACHE_VQ_HDRSIZE  244
#define CACHE_VQ_MAGIC1   0xc0edbabe
#define CACHE_VQ_MAGIC2   0xc0edbeef

/*
==============
Cache_Free

Free a cache entry. Unless keep is set, first release the GPU textures the
resource holds: model caches release directly, sprite/audio caches read the
resource back out of AFile block memory to walk its texture table. Then clear
user->data, unlink from the LRU list, and free the block.
==============
*/
void Cache_Free( cache_user_t* c, int keep )
{
	cache_system_t* cs;
	byte* payload;
	int isAudio;
	afile_t* af;
	byte* juggle;
	studiohdr_t* phdr;
	mstudiotexture_t* ptexture;
	int count;
	int base;
	int align;
	int total;
	int i;

	isAudio = (int)((unsigned int)c->data & 1);
	payload = (byte*)((unsigned int)c->data & ~1u);
	cs = (cache_system_t*)(payload - sizeof(cache_system_t));
	c->data = payload;

	if (payload == NULL)
		Sys_Error("MnemoCacheFree: not allocated");

	if (cs->flags & CACHE_LOCKED)
		Sys_Error("Tried to free locked cache block!");

	if (keep == 0)
	{
		if (isAudio == 0)
		{
			Mod_FreeStudioTextures(payload);
		}
		else
		{
			/* The model was juggled out to AFile blocks; read the header back
			   to find its texture table, and release those GPU textures. */
			af = AFile_FindByName((char*)cs);

			if (af != NULL)
			{
				AFile_GetSize(af);
				juggle = (byte*)MnemoAlloc(CACHE_VQ_HDRSIZE, MNEMO_FLAG_MALLOC, 0, "juggling");

				if (juggle != NULL)
				{
					AFile_ReadBlocks(af, juggle, CACHE_VQ_HDRSIZE);
					phdr = (studiohdr_t*)juggle;

					if (phdr->version == CACHE_VQ_MAGIC1 ||
					    phdr->version == CACHE_VQ_MAGIC2)
					{
						count = phdr->numtextures;
						base = phdr->textureindex;
						align = base & 3;
						base = base - align;
						total = align + count * (int)sizeof(mstudiotexture_t);

						Mnemo_FreeInline(juggle);

						juggle = (byte*)MnemoAlloc(total, MNEMO_FLAG_MALLOC, 0, "juggling");

						if (juggle != NULL)
						{
							AFile_ReadBlocksOffset(af, juggle, base, total);

							if (align != 0 && count > 0)
							{
								ptexture = (mstudiotexture_t*)(juggle + align);

								for (i = 0; i < count; i++)
									DC_ReleaseTexture(ptexture[i].index);
							}
						}
					}

					if (juggle != NULL)
						Mnemo_FreeInline(juggle);
				}
			}
		}
	}

	if (isAudio && (af = AFile_FindByName((char*)cs)) != NULL)
		AFile_Free(af);

	g_mnemo.cache_bytes -= cs->size;
	*cs->user = NULL;

	if (g_mnemo.cache_mru == cs) g_mnemo.cache_mru = cs->lru_prev;
	if (g_mnemo.cache_lru == cs) g_mnemo.cache_lru = cs->lru_next;
	if (cs->lru_next) cs->lru_next->lru_prev = cs->lru_prev;
	if (cs->lru_prev) cs->lru_prev->lru_next = cs->lru_next;

	Mnemo_FreeInline(cs);
}

int Cache_TotalUsed( void )
{
	return g_mnemo.cache_bytes;
}

void Cache_Report( void )
{
}

/*
==============
Cache_Check

- If user->data LSB=1 (moved block): treat as invalid, clear to NULL.
- If user->data valid: bring to MRU head, update frame stamp, return payload.
==============
*/
unsigned int Mnemo_CacheCheck( cache_user_t* c )
{
	unsigned int data;
	cache_system_t* cs;
	cache_system_t* newcs;
	afile_t* af;
	int size;
	int needed;

	data = (unsigned int)c->data;

	/* Block was juggled out to AFile memory to make room; bring it back. */
	if (data & 1u)
	{
		c->data = (void*)(data & ~1u);
		cs = (cache_system_t*)((data & ~1u) - sizeof(cache_system_t));
		af = AFile_FindByName(cs->name);

		if (af == NULL)
		{
			c->data = NULL;
		}
		else
		{
			size = AFile_GetSize(af);
			needed = size + sizeof(cache_system_t);

			while (1)
			{
				if (needed == sizeof(cache_system_t))
					newcs = (cache_system_t*)MnemoAlloc(sizeof(cache_system_t), MNEMO_FLAG_MALLOC, 0, cs->name);
				else
					newcs = (cache_system_t*)MnemoAlloc(needed, MNEMO_FLAG_CACHE, 0, cs->name);

				if (newcs != NULL)
					break;

				if (Cache_FreeLRU(0) == 0)
				{
					Mnemo_Summary_f();
					Sys_ErrorColor(RGB565_RED, "Out of cache memory.\n");
					return 0;
				}
			}

			memcpy(newcs, cs, sizeof(cache_system_t));
			AFile_ReadBlocks(af, (byte*)newcs + sizeof(cache_system_t), size);
			AFile_Free(af);

			newcs->size = needed;
			g_mnemo.cache_bytes += needed;
			newcs->lru_next = NULL;
			newcs->lru_prev = NULL;
			g_mnemo.cache_epoch_bytes += newcs->size;

			Cache_MakeLRU(newcs, cs->name);
			Cache_Free(c, 1);
			c->data = (void*)((byte*)newcs + sizeof(cache_system_t));
		}

		data = (unsigned int)c->data;
	}

	if (data == 0)
		return 0;

	/* Move the entry to the MRU end of the LRU list and refresh its stamps. */
	cs = (cache_system_t*)(data - sizeof(cache_system_t));

	Cache_UnlinkLRU(cs);
	Cache_MoveToMRU(cs);

	cs->timestamp = gHostSpawnCount;
	cs->frame = host_framecount;

	return data;
}

void* Cache_Check( cache_user_t* c )
{
	return (void*)Mnemo_CacheCheck(c);
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
	int total;

	if (host_initialized != 1 && developer.value > 20.0f && c->data != NULL)
		Sys_Error("Cache_Alloc: already allocated");

	if (size < 1)
		Sys_Error("Cache_Alloc: size %i", size);

	total = size + (int)sizeof(cache_system_t);

	while (1)
	{
		if (total == (int)sizeof(cache_system_t))
			cs = (cache_system_t*)MnemoAlloc(total, MNEMO_FLAG_MALLOC, 0, name);
		else
			cs = (cache_system_t*)MnemoAlloc(total, MNEMO_FLAG_CACHE, 0, name);

		if (cs == NULL && Cache_FreeLRU(0) == 0)
			break;

		if (cs != NULL)
		{
			g_mnemo.cache_bytes += total;
			cs->size = total;
			cs->user = (void**)c;
			c->data = (byte*)cs + sizeof(cache_system_t);
			cs->lru_next = NULL;
			cs->lru_prev = NULL;
			g_mnemo.cache_epoch_bytes += total;

			if (g_mnemo.cache_epoch_frame != (unsigned int)host_framecount)
			{
				g_mnemo.cache_epoch_frame = (unsigned int)host_framecount;
				g_mnemo.cache_epoch_bytes = 0;
			}

			Cache_MakeLRU(cs, name);
			return (void*)Mnemo_CacheCheck(c);
		}
	}

	Mnemo_Summary_f();
	Sys_ErrorColor(RGB565_RED, "Out of cache memory!");
	return NULL;
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

	Mnemo_InitArena(buf, size);

	p = COM_CheckParm("-zone");
	if (p)
	{
		if (p < com_argc - 1)
			zonesize = Q_atoi(com_argv[p + 1]) * 1024;
		else
			Sys_Error("Memory_Init: you must specify a size in KB after -zone");
	}

	if (zonesize < MNEMO_SLOPPY_MAX)
		zonebuf = Mnemo_SloppyAlloc(zonesize);
	else
		zonebuf = MnemoAlloc(zonesize, MNEMO_FLAG_HUNK, hunk_alloc_class, "zone");

	mainzone = (memzone_t*)zonebuf;
	Z_ClearZone(mainzone, zonesize);

	Cmd_AddCommand("report", Mnemo_ReportToFile);
	Cmd_AddCommand("sizes", Mnemo_Sizes_f);
	Cmd_AddCommand("summary", Mnemo_Summary_f);

	Cvar_RegisterVariable(&mnemo_cache);
}

typedef struct dc_precache_map_s
{
	char	*name;
	char	**manifest;
} dc_precache_map_t;

#include "dc_precache_data.inc"

/*
========================
DC_PrecacheMap

Warms the models, animation groups, and decals used by a map while the server
is spawning. Shared manifest tails keep the map lists compact.
========================
*/
void DC_PrecacheMap( char* mapName )
{
	dc_precache_map_t	*map;
	char			**manifest;
	char			*entry;
	char			*last;
	char			lastDigit;
	texture_t		*texture;
	int			modelIndex;
	int			index;

	g_mnemo.cache_epoch_bytes = 0;

	for (map = dc_precache_maps ; map->name ; map++)
	{
		if (!strcmp(map->name, mapName))
			break;
	}

	if (!map->name)
		return;

	manifest = map->manifest;
	modelIndex = -1;

	while ((entry = *manifest) != NULL)
	{
		if (entry[0] == '+')
		{
			manifest++;
			R_StudioCacheAnim(sv.models[modelIndex], (int)*manifest);
			manifest++;
		}
		else if (entry[0] == ':')
		{
			manifest++;
			entry = *manifest;
			last = entry + strlen(entry) - 1;
			lastDigit = *last;

			do
			{
				index = Draw_CacheIndex(decal_wad, entry);
				texture = (texture_t*)Draw_CacheGet(decal_wad, index);
				DC_TouchTexture(texture->gl_texturenum);
				(*last)--;
			}
			while (isdigit(*last) && *last != '0');

			*last = lastDigit;
			manifest++;
		}
		else if (entry[0] == '-')
		{
			manifest = (char**)*(manifest + 1);
		}
		else
		{
			modelIndex = PF_precache_model_I(entry);
			manifest++;
		}
	}
}

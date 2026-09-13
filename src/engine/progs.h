#ifndef PROGS_H
#define PROGS_H

#include "progdefs.h"

#ifndef EIFACE_H
// Forward declare this type to avoid problems
typedef struct saverestore_s SAVERESTOREDATA;
#endif

// Entity state is used for the baseline and for delta compression of a packet of 
//  entities that is sent to a client.
typedef struct
{
	byte	controller[4];
	byte	blending[2];
	byte	padding[2];

	short	entityType;  // Normal or Custom to know how to parse the entity.
	short	skin;
	short	solid;
	short	renderfx;
	short	rendermode;
	short	renderamt;
	short	number;      // Index into cl_entities array for this entity.
	short	effects;
	short	colormap;

	int		flags;       // The delta compression bit header.
	int		modelindex;
	int		sequence;
	int		movetype;
	int		body;
	int		aiment;

	float	frame;
	float	scale;
	float	animtime;
	float	framerate;

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	velocity;
	vec3_t	mins;    // Send bbox down to client for use during prediction.
	vec3_t	maxs;

	color24	rendercolor;
} entity_state_t;

#define	MAX_ENT_LEAFS	24
typedef struct edict_s
{
	qboolean	free;
	short		num_leafs;
	short		leaf_capacity;
	short		*leafnums;
	int			serialnumber;
	link_t		area;				// linked to a division node or leaf

	entity_state_t	baseline;
	
	float		freetime;			// sv.time when the object was freed

	void*		pvPrivateData;		// Alloced and freed by engine, used by DLLs

	entvars_t	v;					// C exported fields from progs
// other fields from progs come immediately after
} edict_t;

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

extern	char* pr_strings;

// gGlobalVariables is defined in C (sv_main.c); keep C linkage so the C++ game
// DLL resolves the same symbol when it points gpGlobals at the engine globals.
extern	globalvars_t	gGlobalVariables;

//============================================================================

edict_t* ED_Alloc( void );
void ED_Free( edict_t* ed );

char* ED_NewString( const char* string );
// returns a copy of the string allocated from the server's string heap

void ED_Print( edict_t* ed );
void ED_Write( SAVERESTOREDATA* save, edict_t* ed );
char* ED_ParseEdict( char* data, edict_t* ent );


//void ED_WriteGlobals( SAVERESTOREDATA* save );
//void ED_ParseGlobals( char* data );

void ED_LoadFromFile( char* data );

//define EDICT_NUM(n) ((edict_t*)(sv.edicts + (n) * pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte*)(e) - sv.edicts) / pr_edict_size)

edict_t* EDICT_NUM( int n );
int NUM_FOR_EDICT( const edict_t* e );

#define	EDICT_TO_PROG(e) ((byte*)e - (byte*)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t*)((byte*)sv.edicts + e))

//============================================================================

#if 0
#define	G_FLOAT(o) (pr_globals[o])
#define	G_INT(o) (*(int *)&pr_globals[o])
#define	G_EDICT(o) ((edict_t *)((byte *)sv.edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (pr_strings + *(string_t *)&pr_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
#endif
#define	E_STRING(e,o) (pr_strings + *(string_t *)&((char *)&e->v)[o])

// The game code reads the engine's globals through this pointer; the engine
// points it at gGlobalVariables once the game rules are up.
extern globalvars_t* gpGlobals;

extern	int		type_size[8];

void ED_PrintEdicts( void );
void ED_PrintNum( int ent );

#ifdef __cplusplus
}
#endif

#endif // PROGS_H

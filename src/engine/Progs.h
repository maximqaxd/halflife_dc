#ifndef PROGS_H
#define PROGS_H

#include "progdefs.h"

#ifndef HLDC_OFFSETOF
#define HLDC_OFFSETOF(type, member) ((unsigned int)&(((type *)0)->member))
#endif

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
	byte	reserved[2];

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

typedef char entity_state_t_must_match_retail_size[
	(sizeof(entity_state_t) == 0x84) ? 1 : -1];
typedef char entity_state_t_flags_must_be_at_1c[
	(HLDC_OFFSETOF(entity_state_t, flags) == 0x1C) ? 1 : -1];
typedef char entity_state_t_modelindex_must_be_at_20[
	(HLDC_OFFSETOF(entity_state_t, modelindex) == 0x20) ? 1 : -1];
typedef char entity_state_t_aiment_must_be_at_30[
	(HLDC_OFFSETOF(entity_state_t, aiment) == 0x30) ? 1 : -1];
typedef char entity_state_t_origin_must_be_at_44[
	(HLDC_OFFSETOF(entity_state_t, origin) == 0x44) ? 1 : -1];
typedef char entity_state_t_rendercolor_must_be_at_80[
	(HLDC_OFFSETOF(entity_state_t, rendercolor) == 0x80) ? 1 : -1];

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

typedef char edict_t_num_leafs_must_be_at_02[
	(HLDC_OFFSETOF(edict_t, num_leafs) == 0x02) ? 1 : -1];
typedef char edict_t_leaf_capacity_must_be_at_04[
	(HLDC_OFFSETOF(edict_t, leaf_capacity) == 0x04) ? 1 : -1];
typedef char edict_t_leafnums_must_be_at_08[
	(HLDC_OFFSETOF(edict_t, leafnums) == 0x08) ? 1 : -1];
typedef char edict_t_serialnumber_must_be_at_0c[
	(HLDC_OFFSETOF(edict_t, serialnumber) == 0x0C) ? 1 : -1];
typedef char edict_t_area_must_be_at_10[
	(HLDC_OFFSETOF(edict_t, area) == 0x10) ? 1 : -1];
typedef char edict_t_baseline_must_be_at_18[
	(HLDC_OFFSETOF(edict_t, baseline) == 0x18) ? 1 : -1];
typedef char edict_t_private_data_must_be_at_a0[
	(HLDC_OFFSETOF(edict_t, pvPrivateData) == 0xA0) ? 1 : -1];
typedef char edict_t_entvars_must_be_at_a4[
	(HLDC_OFFSETOF(edict_t, v) == 0xA4) ? 1 : -1];
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

extern	char* pr_strings;

// gGlobalVariables is defined in C (sv_main.c); keep C linkage so the C++ game
// DLL resolves the same symbol when it points gpGlobals at the engine globals.
#ifdef __cplusplus
extern "C" {
#endif
extern	globalvars_t	gGlobalVariables;
#ifdef __cplusplus
}
#endif

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
#ifdef __cplusplus
extern "C" {
#endif
extern globalvars_t* gpGlobals;
#ifdef __cplusplus
}
#endif

extern	int		type_size[8];

void ED_PrintEdicts( void );
void ED_PrintNum( int ent );


#endif // PROGS_H

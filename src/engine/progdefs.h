#ifndef PROGDEFS_H
#define PROGDEFS_H


typedef struct
{
	float		time;
	float		frametime;
	float		force_retouch;
	string_t	mapname;
	string_t	startspot;
	float		deathmatch;
	float		coop;
	float		teamplay;
	float		serverflags;
	float		found_secrets;
	vec3_t		v_forward;
	vec3_t		v_up;
	vec3_t		v_right;
	float		trace_allsolid;
	float		trace_startsolid;
	float		trace_fraction;
	vec3_t		trace_endpos;
	vec3_t		trace_plane_normal;
	float		trace_plane_dist;
	edict_t*	trace_ent;
	float		trace_inopen;
	float		trace_inwater;
	int			trace_hitgroup;
	int			trace_flags;
	int			msg_entity;
	int			cdAudioTrack;
	int			maxClients;
	int			maxEntities;
	const char* pStringBase;

	void* pSaveData;
	vec3_t		vecLandmarkOffset;
} globalvars_t;


typedef struct entvars_s
{
	string_t	classname;
	string_t	globalname;

	vec3_t		origin;
	vec3_t		oldorigin;
	vec3_t		velocity;
	vec3_t		basevelocity;
	vec3_t      clbasevelocity;  // Base velocity that was passed in to server physics so 
	//  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	vec3_t		movedir;

	vec3_t		angles;			// Model angles
	vec3_t		avelocity;		// angle velocity (degrees per second)
	vec3_t		punchangle;		// auto-decaying view angle adjustment
	vec3_t		v_angle;		// Viewing angle (player only)
	int			fixangle;		// 0:nothing, 1:force view angles, 2:add avelocity
	float		idealpitch;
	float		pitch_speed;
	float		ideal_yaw;
	float		yaw_speed;

	int			modelindex;
	string_t	model;

	int			viewmodel;		// player's viewmodel
	int			weaponmodel;	// what other players see

	vec3_t		absmin;		// BB max translated to world coord
	vec3_t		absmax;		// BB max translated to world coord
	vec3_t		mins;		// local BB min
	vec3_t		maxs;		// local BB max
	vec3_t		size;		// maxs - mins

	float		ltime;
	float		nextthink;

	int			movetype;
	int			solid;

	int			skin;
	int			body;			// sub-model selection for studiomodels
	int 		effects;

	float		gravity;		// % of "normal" gravity
	float		friction;		// inverse elasticity of MOVETYPE_BOUNCE

	int			light_level;

	int			sequence;		// animation sequence
	int			gaitsequence;	// movement animation sequence for player (0 for none)
	float		frame;			// % playback position in animation sequences (0..255)
	float		animtime;		// world time when frame was set
	float		framerate;		// animation playback rate (-8x to 8x)
	byte		controller[4];	// bone controller setting (0..255)
	byte		blending[2];	// blending amount between sub-sequences (0..255)

	float		scale;			// sprite rendering scale (0..255)
	int			rendermode;
	float		renderamt;
	vec3_t		rendercolor;
	int			renderfx;

	float		health;
	float		frags;
	int			weapons;  // bit mask for available weapons
	float		takedamage;

	int			deadflag;
	vec3_t		view_ofs;	// eye position

	int			button;
	int			impulse;

	edict_t* chain;			// Entity pointer when linked into a linked list
	edict_t* dmg_inflictor;
	edict_t* enemy;
	edict_t* aiment;		// entity pointer when MOVETYPE_FOLLOW
	edict_t* owner;
	edict_t* groundentity;

	int			spawnflags;
	int			flags;

	int			colormap;		// lowbyte topcolor, highbyte bottomcolor
	int			team;

	float		max_health;
	float		teleport_time;
	float		armortype;
	float		armorvalue;
	int			waterlevel;
	int			watertype;

	string_t	target;
	string_t	targetname;
	string_t	netname;
	string_t	message;

	float		dmg_take;
	float		dmg_save;
	float		dmg;
	float		dmgtime;

	string_t	noise;
	string_t	noise1;
	string_t	noise2;
	string_t	noise3;

	float		speed;
	float		air_finished;
	float		pain_finished;
	float		radsuit_finished;

	edict_t* pContainingEntity;
} entvars_t;

#define PROGDEFS_OFFSETOF(type, member) ((unsigned int)&(((type *)0)->member))
typedef char entvars_t_must_match_retail_size[
	(sizeof(entvars_t) == 0x1EC) ? 1 : -1];
typedef char entvars_t_origin_must_be_at_08[
	(PROGDEFS_OFFSETOF(entvars_t, origin) == 0x08) ? 1 : -1];
typedef char entvars_t_angles_must_be_at_50[
	(PROGDEFS_OFFSETOF(entvars_t, angles) == 0x50) ? 1 : -1];
typedef char entvars_t_modelindex_must_be_at_94[
	(PROGDEFS_OFFSETOF(entvars_t, modelindex) == 0x94) ? 1 : -1];
typedef char entvars_t_absmin_must_be_at_a4[
	(PROGDEFS_OFFSETOF(entvars_t, absmin) == 0xA4) ? 1 : -1];
typedef char entvars_t_movetype_must_be_at_e8[
	(PROGDEFS_OFFSETOF(entvars_t, movetype) == 0xE8) ? 1 : -1];
typedef char entvars_t_sequence_must_be_at_108[
	(PROGDEFS_OFFSETOF(entvars_t, sequence) == 0x108) ? 1 : -1];
typedef char entvars_t_gaitsequence_must_be_at_10c[
	(PROGDEFS_OFFSETOF(entvars_t, gaitsequence) == 0x10C) ? 1 : -1];
typedef char entvars_t_frame_must_be_at_110[
	(PROGDEFS_OFFSETOF(entvars_t, frame) == 0x110) ? 1 : -1];
typedef char entvars_t_scale_must_be_at_124[
	(PROGDEFS_OFFSETOF(entvars_t, scale) == 0x124) ? 1 : -1];
typedef char entvars_t_aiment_must_be_at_174[
	(PROGDEFS_OFFSETOF(entvars_t, aiment) == 0x174) ? 1 : -1];
typedef char entvars_t_containing_entity_must_be_at_1e8[
	(PROGDEFS_OFFSETOF(entvars_t, pContainingEntity) == 0x1E8) ? 1 : -1];
#undef PROGDEFS_OFFSETOF


#endif // PROGDEFS_H

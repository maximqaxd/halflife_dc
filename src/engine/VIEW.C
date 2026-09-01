// view.c -- player eye positioning

#include "quakedef.h"
#include <floatmathlib.h>
#include <shintr.h>
#include "pmove.h"
#include "pr_cmds.h"
#include "shake.h"

#undef fabs
#pragma intrinsic(fabsf)
#pragma intrinsic(sqrtf)

extern float _utos( unsigned int value );

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t	lcd_x = { "lcd_x", "0" };
cvar_t	lcd_yaw = { "lcd_yaw", "0" };

cvar_t	scr_ofsx = { "scr_ofsx", "0" };
cvar_t	scr_ofsy = { "scr_ofsy", "0" };
cvar_t	scr_ofsz = { "scr_ofsz", "0" };

cvar_t	cl_rollspeed = { "cl_rollspeed", "200" };
cvar_t	cl_rollangle = { "cl_rollangle", "2.0" };

cvar_t	cl_bobcycle = { "cl_bobcycle", "0.8" };
cvar_t	cl_bob = { "cl_bob", "0.01" };
cvar_t	cl_bobup = { "cl_bobup", "0.5" };
cvar_t	cl_waterdist = { "cl_waterdist", "4" };

cvar_t	v_kicktime = { "v_kicktime", "0.5" };
cvar_t	v_kickroll = { "v_kickroll", "0.6" };
cvar_t	v_kickpitch = { "v_kickpitch", "0.6" };
cvar_t	v_iyaw_cycle = { "v_iyaw_cycle", "2", 0, 2.0f };
cvar_t	v_iroll_cycle = { "v_iroll_cycle", "0.5", 0, 0.5f };
cvar_t	v_ipitch_cycle = { "v_ipitch_cycle", "1", 0, 1.0f };
cvar_t	v_iyaw_level = { "v_iyaw_level", "0.3", 0, 0.3f };
cvar_t	v_iroll_level = { "v_iroll_level", "0.1", 0, 0.1f };
cvar_t	v_ipitch_level = { "v_ipitch_level", "0.3", 0, 0.3f };
float	v_idlescale;

cvar_t	v_dark = { "v_dark", "0" };
cvar_t	crosshair = { "crosshair", "0", TRUE };

byte		texgammatable[256];	// palette is sent through this to convert to screen gamma
int			lightgammatable[1024];
int			lineargammatable[1024];
int			screengammatable[1024];

#ifdef	GLQUAKE
byte		ramps[3][256];
float		v_blend[4];		// rgba 0.0 - 1.0
#endif	// GLQUAKE

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
float V_CalcRoll( float* angles, float* velocity, float rollangle, float rollspeed )
{
	int		sign;
	float	side;
	float	value;

	AngleVectors(angles, forward, right, up);
	side = DotProduct(velocity, right);
	sign = side < 0.0f ? -1 : 1;
	side = fabsf(side);

	value = rollangle;

	if (side < rollspeed)
		side = side * value / rollspeed;
	else
		side = value;

	return side * sign;
}

/*
===============
V_CalcBob

===============
*/
float V_CalcBob( void )
{
	static	double	bobtime;
	static float	bob;
	float	cycle;

	if (cl.spectator)
		return 0.0f;

	if (onground == -1 ||
		cl.time == cl.oldtime)
	{
		// just use old value
		return bob;
	}

	bobtime += host_frametime;
	cycle = bobtime - (int)(bobtime / cl_bobcycle.value) * cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = (float)M_PI * cycle / cl_bobup.value;
	else
		cycle = (float)M_PI + (float)M_PI * (cycle - cl_bobup.value) / (1.0f - cl_bobup.value);

// bob is proportional to simulated velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrtf(cl.simvel[0] * cl.simvel[0] + cl.simvel[1] * cl.simvel[1]) * cl_bob.value;
	bob = bob * 0.3f + bob * 0.7f * sin(cycle);
	if (bob > 4.0f)
		bob = 4.0f;
	else if (bob < -7.0f)
		bob = -7.0f;
	return bob;
}


//=============================================================================


cvar_t	v_centermove = { "v_centermove", "0.15" };
cvar_t	v_centerspeed = { "v_centerspeed", "500" };



void V_StartPitchDrift( void )
{
	if (cl.laststop == cl.time)
	{
		return;		// something else is keeping it from drifting
	}

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = FALSE;
		cl.driftmove = 0;
	}
}

void V_StopPitchDrift( void )
{
	cl.laststop = cl.time;
	cl.nodrift = TRUE;
	cl.pitchvel = 0.0f;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch( void )
{
	float		delta, move;

	if (noclip_anglehack || !cl.onground || cl.spectator)
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if (fabsf(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;

		if (cl.driftmove > v_centermove.value)
		{
			V_StartPitchDrift();
		}
		return;
	}

	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed.value;

//Con_Printf("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}	
}


/*
==============================================================================

						PALETTE FLASHES

==============================================================================
*/


cshift_t	cshift_empty = { { 130, 80, 50 }, 0 };
cshift_t	cshift_water = { { 130, 80, 50 }, 128 };
cshift_t	cshift_slime = { { 0, 25, 5 }, 150 };
cshift_t	cshift_lava = { { 255, 80, 0 }, 150 };

cvar_t		v_gamma = { "gamma", "2.5", TRUE };		// monitor gamma
cvar_t		v_brightness = { "brightness", "0.0", TRUE };	// low level light adjustment
cvar_t		v_lightgamma = { "lightgamma", "2.5" };
cvar_t		v_texgamma = { "texgamma", "2.0" };		// source gamma of textures
cvar_t		v_lambert = { "lambert", "1.5" };
cvar_t		v_direct = { "direct", "0.9" };

/*
=================
V_CheckGamma

FIXME:  Define this as a change function to the cvar's below rather than polling it
 every frame.  Note, still need to make sure it gets called very first time through frame loop.
=================
*/
qboolean V_CheckGamma( void )
{
	static float oldgammavalue, oldlightgamma, oldtexgamma, oldbrightness;
#if !defined ( GLQUAKE )
	static float ambientr, ambientg, ambientb;
#endif

	if ((v_gamma.value == oldgammavalue) &&
		(v_lightgamma.value == oldlightgamma) &&
		(v_texgamma.value == oldtexgamma) &&
		(v_brightness.value == oldbrightness)
#if !defined ( GLQUAKE )
		&& (r_ambient_r.value == ambientr) &&
		(r_ambient_g.value == ambientg) &&
		(r_ambient_b.value == ambientb)
#endif
		)
	{
		return FALSE;
	}

	oldgammavalue = v_gamma.value;
	oldlightgamma = v_lightgamma.value;
	oldtexgamma = v_texgamma.value;
	oldbrightness = v_brightness.value;

#if !defined ( GLQUAKE )
	ambientr = r_ambient_r.value;
	ambientg = r_ambient_g.value;
	ambientb = r_ambient_b.value;
#endif

	D_FlushCaches();
	vid.recalc_refdef = 1;				// force a surface cache flush

	return TRUE;
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift( void )
{
	static float cshift1;
	static float cshift2;
	static float cshift3;
	static float cshift4;

	cshift1 = cshift1 - host_frametime < 0.0f ? 0.0f : cshift1 - host_frametime;
	cshift2 = cshift2 - host_frametime < 0.0f ? 0.0f : cshift2 - host_frametime;
	cshift3 = cshift3 - host_frametime < 0.0f ? 0.0f : cshift3 - host_frametime;
	cshift4 = cshift4 - host_frametime < 0.0f ? 0.0f : cshift4 - host_frametime;
}

/*
=============
V_CalcBlend
=============
*/
#ifdef	GLQUAKE
void V_CalcBlend( void )
{
	float	r, g, b, a;

	r = 0.0f;
	g = 0.0f;
	b = 0.0f;
	a = 0.0f;

	v_blend[0] = r / 255.0f;
	v_blend[1] = g / 255.0f;
	v_blend[2] = b / 255.0f;
	v_blend[3] = a;
	if (v_blend[3] > 1.0f)
		v_blend[3] = 1.0f;
	if (v_blend[3] < 0.0f)
		v_blend[3] = 0.0f;
}
#endif

/*
=============
V_UpdatePalette
=============
*/
#ifdef	GLQUAKE
void V_UpdatePalette( void )
{
	int		i;
	qboolean	newFlag;
	float	r, g, b, a;
	int		ir, ig, ib;
	qboolean force;
	void (* volatile calcBlend)( void ) = V_CalcBlend;

	V_CalcPowerupCshift();

	newFlag = FALSE;

	force = V_CheckGamma();
	if (!newFlag && !force)
		return;

	calcBlend();

//Con_Printf("b: %4.2f %4.2f %4.2f %4.6f\n", v_blend[0],	v_blend[1],	v_blend[2],	v_blend[3]);

	a = v_blend[3];
	r = 255.0f * v_blend[0] * a;
	g = 255.0f * v_blend[1] * a;
	b = 255.0f * v_blend[2] * a;

	a = 1.0f - a;
	for (i = 0; i < 256; i++)
	{
		ir = r + i * a;
		ig = g + i * a;
		ib = b + i * a;
		if (ir > 255)
			ir = 255;
		if (ig > 255)
			ig = 255;
		if (ib > 255)
			ib = 255;

		ramps[0][i] = ir;
		ramps[1][i] = ig;
		ramps[2][i] = ib;
	}
}
#else	// !GLQUAKE
void V_UpdatePalette( void )
{
	V_CheckGamma();
}
#endif	// !GLQUAKE


/*
==============================================================================

						VIEW RENDERING

==============================================================================
*/

float angledelta( float a )
{
	a = anglemod(a);
	if (a > 180.0f)
		a -= 360.0f;
	return a;
}


/*
==================
V_CalcGunAngle
==================
*/
void V_CalcGunAngle( void )
{	
	cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + cl.crosshairangle[YAW];
	cl.viewent.angles[PITCH] = -r_refdef.viewangles[PITCH] + cl.crosshairangle[PITCH] * 0.25f;
	cl.viewent.angles[ROLL] -= v_idlescale * sin(cl.time * v_iroll_cycle.value) * v_iroll_level.value;

	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.	
	cl.viewent.angles[PITCH] -= v_idlescale * sin(cl.time * v_ipitch_cycle.value) * (v_ipitch_level.value * 0.5f);
	cl.viewent.angles[YAW] -= v_idlescale * sin(cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets( void )
{
// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (r_refdef.vieworg[0] < cl.simorg[0] - 14)
		r_refdef.vieworg[0] = cl.simorg[0] - 14;
	else if (r_refdef.vieworg[0] > cl.simorg[0] + 14)
		r_refdef.vieworg[0] = cl.simorg[0] + 14;
	if (r_refdef.vieworg[1] < cl.simorg[1] - 14)
		r_refdef.vieworg[1] = cl.simorg[1] - 14;
	else if (r_refdef.vieworg[1] > cl.simorg[1] + 14)
		r_refdef.vieworg[1] = cl.simorg[1] + 14;
	if (r_refdef.vieworg[2] < cl.simorg[2] - 22)
		r_refdef.vieworg[2] = cl.simorg[2] - 22;
	else if (r_refdef.vieworg[2] > cl.simorg[2] + 30)
		r_refdef.vieworg[2] = cl.simorg[2] + 30;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle( void )
{
	r_refdef.viewangles[ROLL] += v_idlescale * sin(cl.time * v_iroll_cycle.value) * v_iroll_level.value;
	r_refdef.viewangles[PITCH] += v_idlescale * sin(cl.time * v_ipitch_cycle.value) * v_ipitch_level.value;
	r_refdef.viewangles[YAW] += v_idlescale * sin(cl.time * v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
#pragma inline_depth(0)
void V_CalcViewRoll( void )
{
	float		side;

	side = V_CalcRoll(cl_entities[cl.viewentity].angles, cl.simvel,
		cl_rollangle.value, cl_rollspeed.value);

	r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0.0f)
	{
		r_refdef.viewangles[ROLL] += v_dmg_time / v_kicktime.value * v_dmg_roll;
		r_refdef.viewangles[PITCH] += v_dmg_time / v_kicktime.value * v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.stats[STAT_HEALTH] <= 0 && cl.viewheight != 0.0f)
	{
		r_refdef.viewangles[ROLL] = 80.0f;	// dead view angle
		return;
	}
}
#pragma inline_depth(255)


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef( void )
{
	cl_entity_t* view;
	float		old;

// view is the weapon model
	view = &cl.viewent;

	VectorCopy(cl.simorg, r_refdef.vieworg);
	VectorCopy(cl.simangles, r_refdef.viewangles);
	view->model = NULL;

// allways idle in intermission
	old = v_idlescale;
	v_idlescale = 1.0f;
	V_AddIdle();
	v_idlescale = old;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef( void )
{
	int				i;
	vec3_t			angles;
	vec3_t			forward, right, up;
	vec3_t			camAngles;
	float			bob, waterOffset;
	static float oldz = 0;

	V_DriftPitch();

	bob = V_CalcBob();

// refresh position from simulated origin
	VectorCopy(cl.simorg, r_refdef.vieworg);
	r_refdef.vieworg[2] += bob + cl.viewheight;

	VectorCopy(cl.viewangles, r_refdef.viewangles);

	V_CalcShake();
	V_ApplyShake(r_refdef.vieworg, r_refdef.viewangles, 1.0f);

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis

	r_refdef.vieworg[0] += 1.0f / 32.0f;
	r_refdef.vieworg[1] += 1.0f / 32.0f;
	r_refdef.vieworg[2] += 1.0f / 32.0f;

	// Check for problems around water, move the viewer artificially if necessary 
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0.0f;
	if (cl.waterlevel >= 2)
	{
		int		i, contents, waterDist, waterEntity;
		vec3_t	point;
		waterDist = cl_waterdist.value;

#if defined( GLQUAKE )
		waterEntity = PM_WaterEntity(cl.simorg);
		if (waterEntity >= 0 && waterEntity < cl.max_edicts && cl_entities[waterEntity].model)
		{
			waterDist += (cl_entities[waterEntity].scale * 16.0f);	// Add in wave height
		}
#else
		waterEntity = 0;	// Don't need this in software
#endif

		VectorCopy(r_refdef.vieworg, point);

		// eyes are above water, make sure we're above the waves
		if (cl.waterlevel == 2)
		{
			point[2] -= waterDist;
			for (i = 0; i < waterDist; i++)
			{
				contents = PM_PointContents(point);
				if (contents > CONTENTS_WATER)
					break;
				point[2] += 1.0f;
			}
			waterOffset = (point[2] + waterDist) - r_refdef.vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for (i = 0; i < waterDist; i++)
			{
				contents = PM_PointContents(point);
				if (contents <= CONTENTS_WATER)
					break;
				point[2] -= 1.0f;
			}
			waterOffset = (point[2] - waterDist) - r_refdef.vieworg[2];
		}
	}

	r_refdef.vieworg[2] += waterOffset;

	V_CalcViewRoll();
	V_AddIdle();

	// offsets
	VectorCopy(cl.viewangles, angles);

	AngleVectors(angles, forward, right, up);

	for (i = 0; i < 3; i++)
		r_refdef.vieworg[i] += scr_ofsx.value * forward[i]
		+ scr_ofsy.value * right[i]
		+ scr_ofsz.value * up[i];

	if (cam_thirdperson)
	{
		vec3_t camForward, camRight, camUp;

		camAngles[0] = cam_ofs[0];
		camAngles[1] = cam_ofs[1];
		camAngles[ROLL] = 0;

		AngleVectors(camAngles, camForward, camRight, camUp);

		for (i = 0; i < 3; i++)
		{
			r_refdef.vieworg[i] += -cam_ofs[2] * camForward[i];
		}
	}

	// Give gun our viewangles
	VectorCopy(cl.viewangles, cl.viewent.angles);

	// set up gun position
	V_CalcGunAngle();

	// Use predicted origin as view origin.
	VectorCopy(cl.simorg, cl.viewent.origin);
	cl.viewent.origin[2] += (waterOffset + cl.viewheight);

	// Let the viewmodel shake at about 10% of the amplitude
	V_ApplyShake(cl.viewent.origin, cl.viewent.angles, 0.9f);

	for (i = 0; i < 3; i++)
	{
		cl.viewent.origin[i] += bob * 0.4f * forward[i];
	}
	cl.viewent.origin[2] += bob;

	// throw in a little tilt.
	cl.viewent.angles[YAW] -= bob * 0.5f;
	cl.viewent.angles[ROLL] -= bob;
	cl.viewent.angles[PITCH] -= bob * 0.3f;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	cl.viewent.origin[2] -= 1.0f;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if (scr_viewsize.value == 110.0f)
	{
		cl.viewent.origin[2] += 1.0f;
	}
	else if (scr_viewsize.value == 100.0f)
	{
		cl.viewent.origin[2] += 2.0f;
	}
	else if (scr_viewsize.value == 90.0f)
	{
		cl.viewent.origin[2] += 1.0f;
	}
	else if (scr_viewsize.value == 80.0f)
	{
		cl.viewent.origin[2] += 0.5f;
	}
	
	cl.viewent.model = cl.model_precache[cl.stats[STAT_WEAPON]];
	cl.viewent.frame = 0.0f;
	cl.viewent.index = cl.playernum + 1;

// set up the refresh position
	VectorAdd(r_refdef.viewangles, cl.punchangle, r_refdef.viewangles);

// smooth out stair step ups
	if (cl.onground && (cl.simorg[2] - oldz) > 0)
	{
		float steptime;

		steptime = cl.time - cl.oldtime;
		if (steptime < 0)
//FIXME		I_Error ("steptime < 0");
			steptime = 0.0f;

		oldz += steptime * 80.0f;
		if (oldz > cl.simorg[2])
			oldz = cl.simorg[2];
		if (cl.simorg[2] - oldz > 12.0f)
			oldz = cl.simorg[2] - 12.0f;
		r_refdef.vieworg[2] += oldz - cl.simorg[2];
		cl.viewent.origin[2] += oldz - cl.simorg[2];
	}
	else
		oldz = cl.simorg[2];

	if (cam_thirdperson)
	{
		VectorCopy(camAngles, r_refdef.viewangles);
	}

	if (chase_active.value)
		Chase_Update();

	// override all previous settings if the viewent isn't the client
	if (cl.viewentity > cl.maxclients)
	{
		VectorCopy(cl_entities[cl.viewentity].origin, r_refdef.vieworg);
		VectorCopy(cl_entities[cl.viewentity].angles, r_refdef.viewangles);
	}
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
extern vrect_t	scr_vrect;

void V_RenderView( void )
{
	if (con_forcedup)
		return;

	if (cls.state != ca_active)
		return;

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
	{
		Cvar_Set("scr_ofsx", "0");
		Cvar_Set("scr_ofsy", "0");
		Cvar_Set("scr_ofsz", "0");
	}

	if (cl.intermission)
	{
		V_CalcIntermissionRefdef();
	}
	else if (!cl.paused /* && (sv.maxclients > 1 || key_dest == key_game) */)
	{
		V_CalcRefdef();
	}

	R_PushDlights();
	R_RenderView();
}

// Screen shake variables
typedef struct
{
	float time;
	float duration;
	float amplitude;
	float frequency;
	float nextShake;
	vec3_t offset;
	float angle;
	vec3_t appliedOffset;
	float appliedAngle;
} screenshake_t;

screenshake_t gVShake;

/*
=================
V_CalcShake

Apply noise to the eye position.
UNDONE: Feedback a bit of this into the view model position.  It shakes too much
=================
*/
void V_CalcShake( void )
{
	float	frametime;
	int		i;
	float	fraction, freq;

	if ((cl.time > gVShake.time) ||
		gVShake.duration <= 0.0f ||
		gVShake.amplitude <= 0.0f ||
		gVShake.frequency <= 0.0f)
	{
		if (gVShake.time != 0.0f)
		{
			gVShake.time = 0.0f;
			VectorCopy(vec3_origin, gVShake.appliedOffset);
			gVShake.appliedAngle = 0.0f;
		}
		return;
	}

	frametime = cl.time - cl.oldtime;

	if (cl.time > gVShake.nextShake)
	{
		// Higher frequency means we recalc the extents more often and perturb the display again
		gVShake.nextShake = cl.time + (gVShake.duration / gVShake.frequency);

		// Compute random shake extents (the shake will settle down from this)
		for (i = 0; i < 3; i++)
		{
			gVShake.offset[i] = RandomFloat(-gVShake.amplitude, gVShake.amplitude);
		}

		gVShake.angle = RandomFloat(-gVShake.amplitude * 0.25f, gVShake.amplitude * 0.25f);
	}

	// Ramp down amplitude over duration (fraction goes from 1 to 0 linearly with slope 1/duration)
	fraction = (cl.time - gVShake.time) / gVShake.duration;

	// Ramp up frequency over duration
	if (fraction)
	{
		freq = (gVShake.frequency / fraction) * gVShake.frequency;
	}
	else
	{
		freq = 0.0f;
	}

	// square fraction to approach zero more quickly
	fraction *= fraction;

	// Sine wave that slowly settles to zero
	fraction = fraction * sin(cl.time * freq);

	// Add to view origin
	for (i = 0; i < 3; i++)
	{
		gVShake.appliedOffset[i] = gVShake.offset[i] * fraction;
	}

	// Add to roll
	gVShake.appliedAngle = gVShake.angle * fraction;

	// Drop amplitude a bit, less for higher frequency shakes
	gVShake.amplitude -= gVShake.amplitude * (frametime / (gVShake.duration * gVShake.frequency));
}

/*
=================
V_ApplyShake

Apply the current screen shake to this origin/angles.  Factor is the amount to apply
This is so you can blend in part of the shake
=================
*/
void V_ApplyShake( float* origin, float* angles, float factor )
{
	if (origin)
		VectorMA(origin, factor, gVShake.appliedOffset, origin);

	if (angles)
		angles[ROLL] += gVShake.appliedAngle * factor;
}

/*
=================
V_ScreenShake

Message hook to parse ScreenShake messages
=================
*/
int V_ScreenShake( const char* pszName, int iSize, void* pbuf )
{
	ScreenShake* pShake = (ScreenShake*)pbuf;
	float amplitude;

	gVShake.duration = pShake->duration * (1.0f / 4096.0f);
	gVShake.time = gVShake.duration + cl.time;

	amplitude = pShake->amplitude * (1.0f / 4096.0f);

	// Don't overwrite larger existing shake unless we are told to
	if (gVShake.amplitude < amplitude)
		gVShake.amplitude = amplitude;

	gVShake.frequency = pShake->frequency * (1.0f / 256.0f);
	gVShake.nextShake = 0; // apply immediately

	return 1;
}

/*
=============
V_FadeAlpha

Compute the overall color & alpha of the fades
=============
*/
static void (*s_pfnFadeDoneCallback)( int parm1 );
static int s_nCallbackParameter;

int V_FadeAlpha( void )
{
	int alpha;

	if (cl.sf.fadeFlags & FFADE_STAYOUT)
		cl.sf.fadeEnd = cl.time + 0.1f;

	if (cl.sf.fadeReset < cl.time && cl.sf.fadeEnd < cl.time)
	{
		if (s_pfnFadeDoneCallback)
		{
			s_pfnFadeDoneCallback(s_nCallbackParameter);
			s_pfnFadeDoneCallback = NULL;
			s_nCallbackParameter = 0;
		}
		return 0;
	}

	if (cl.sf.fadeFlags & FFADE_OUT)
		alpha = cl.sf.fadealpha + (cl.sf.fadeEnd - cl.time) * cl.sf.fadeSpeed;
	else
		alpha = (cl.sf.fadeEnd - cl.time) * cl.sf.fadeSpeed;

	// clamp it
	if (alpha > cl.sf.fadealpha)
		alpha = cl.sf.fadealpha;
	else if (alpha < 0)
		alpha = 0;

	return alpha;
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init( void )
{
	Cmd_AddCommand("centerview", V_StartPitchDrift);

	Cvar_RegisterVariable(&lcd_x);
	Cvar_RegisterVariable(&lcd_yaw);

	Cvar_RegisterVariable(&v_centermove);
	Cvar_RegisterVariable(&v_centerspeed);
	Cvar_RegisterVariable(&v_dark);
	Cvar_RegisterVariable(&crosshair);
	Cvar_RegisterVariable(&scr_ofsx);
	Cvar_RegisterVariable(&scr_ofsy);
	Cvar_RegisterVariable(&scr_ofsz);
	Cvar_RegisterVariable(&cl_rollspeed);
	Cvar_RegisterVariable(&cl_rollangle);

	Cvar_RegisterVariable(&cl_bob);
	Cvar_RegisterVariable(&cl_bobcycle);
	Cvar_RegisterVariable(&cl_bobup);
	Cvar_RegisterVariable(&cl_waterdist);

	Cvar_RegisterVariable(&v_kicktime);
	Cvar_RegisterVariable(&v_kickroll);
	Cvar_RegisterVariable(&v_kickpitch);

	Cvar_RegisterVariable(&v_gamma);
	Cvar_RegisterVariable(&v_lightgamma);
	Cvar_RegisterVariable(&v_texgamma);
	Cvar_RegisterVariable(&v_brightness);
	Cvar_RegisterVariable(&v_lambert);
	Cvar_RegisterVariable(&v_direct);

	s_pfnFadeDoneCallback = NULL;
	s_nCallbackParameter = 0;
}

/*
=================
V_InitLevel

Initialize sceen fade/shake data
=================
*/
void V_InitLevel( void )
{
	memset(&gVShake, 0, sizeof(gVShake));

	if (v_dark.value)
	{
		cl.sf.fadeEnd = 5.0f;
		cl.sf.fadeReset = 5.0f;
		cl.sf.fadeb = 0;
		cl.sf.fadeg = 0;
		cl.sf.fader = 0;
		cl.sf.fadealpha = 255;
		cl.sf.fadeFlags = 0;
		if (cl.sf.fadeEnd)
			cl.sf.fadeSpeed = _utos(cl.sf.fadealpha) / cl.sf.fadeEnd;
		cl.sf.fadeReset += cl.time;
		cl.sf.fadeEnd += cl.sf.fadeReset;
		v_dark.value = 0.0f;
	}
	else
	{
		cl.sf.fadeSpeed = 0.0f;
		cl.sf.fadeEnd = 0.0f;
		cl.sf.fadeReset = 0.0f;
		cl.sf.fadealpha = 0;
		cl.sf.fadeb = 0;
		cl.sf.fadeg = 0;
		cl.sf.fader = 0;
		cl.sf.fadeFlags = 0;
	}
}

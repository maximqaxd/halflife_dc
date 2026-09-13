/* ZOID
 *
 * Player camera tracking in Spectator mode
 *
 * This takes over player controls for spectator automatic camera.
 * Player moves as a spectator, but the camera tracks and enemy player
 */

#include "quakedef.h"
#include "winquake.h"
#include <shintr.h>
#include "pmove.h"

#pragma intrinsic(sqrtf)

extern float atan2s( float y, float x );

#define	PM_SPECTATORMAXSPEED	500
#define	PM_STOPSPEED	100
#define	PM_MAXSPEED		320
#define BUTTON_JUMP		2
#define BUTTON_ATTACK	1
#define BUTTON_ATTACK2	2048
#define MAX_ANGLE_TURN	10

static vec3_t desired_position; // where the camera wants to be
static qboolean locked = FALSE;
static int oldbuttons;

// track high fragger
cvar_t cl_hightrack = { "cl_hightrack", "0" };

qboolean cam_forceview;
vec3_t cam_viewangles;
float cam_lastviewtime;

int spec_track = 0; // player# of who we are tracking
int autocam = CAM_NONE;

qboolean Cam_IsTracking( int cam );

static void vectoangles( vec_t* vec, vec_t* ang )
{
	float	forward;
	float	yaw, pitch;

	if (vec[1] == 0.0f && vec[0] == 0.0f)
	{
		yaw = 0.0f;
		if (vec[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
		yaw = (int)(atan2s(vec[1], vec[0]) * 180.0f / (float)M_PI);
		if (yaw < 0.0f)
			yaw += 360.0f;

		forward = sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = (int)(atan2s(vec[2], forward) * 180.0f / (float)M_PI);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0.0f;
}

static float vlen( vec_t* v )
{
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void Cam_Unlock( void )
{
	if (Cam_IsTracking(autocam))
	{
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, "ptrack");
		locked = FALSE;
	}
}

void Cam_Lock( int playernum )
{
	char st[40];

	sprintf(st, "ptrack %i", playernum);
	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, st);
	spec_track = playernum;
	cam_forceview = TRUE;
	locked = FALSE;
}

pmtrace_t Cam_DoTrace( vec_t* vec1, vec_t* vec2 )
{
#if 0
	memset(&pmove, 0, sizeof(pmove));

	pmove.numphysent = 1;
	VectorCopy(vec3_origin, pmove.physents[0].origin);
	pmove.physents[0].model = cl.worldmodel;
#endif

	VectorCopy(vec1, pmove.origin);
	return PM_PlayerMove(pmove.origin, vec2, 0);
}

// Returns distance or 9999 if invalid for some reason
static float Cam_TryFlyby( player_state_t* self, player_state_t* player, vec_t* vec, qboolean checkvis )
{
	vec3_t v;
	pmtrace_t trace;
	float len, scale;

	if (autocam == CAM_FIRSTPERSON)
		scale = 32.0f;
	else
		scale = 800.0f;

	vectoangles(vec, v);
//	v[0] = -v[0];
	VectorCopy(v, pmove.angles);
	VectorNormalize(vec);
	VectorMA(player->origin, scale, vec, v);
	// v is endpos
	// fake a player move
	trace = Cam_DoTrace(player->origin, v);
	if (/*trace.inopen ||*/ trace.inwater)
		return 9999;
	VectorCopy(trace.endpos, vec);
	VectorSubtract(trace.endpos, player->origin, v);
	len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (len < 8.0f || len > scale)
		return 9999.0f;
	if (checkvis)
	{
		VectorSubtract(trace.endpos, self->origin, v);
		len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

		trace = Cam_DoTrace(self->origin, vec);
		if (trace.fraction != 1 || trace.inwater)
			return 9999.0f;
	}
	return len;
}

// Is player visible?
static qboolean Cam_IsVisible( player_state_t* player, vec_t* vec )
{
	pmtrace_t trace;
	vec3_t v;
	float d;

	trace = Cam_DoTrace(player->origin, vec);
	if (trace.fraction != 1 || /*trace.inopen ||*/ trace.inwater)
		return FALSE;
	// check distance, don't let the player get too far away or too close
	VectorSubtract(player->origin, vec, v);
	d = vlen(v);
	if (d < 8.0f)
		return FALSE;
	return TRUE;
}

static qboolean InitFlyby( player_state_t* self, player_state_t* player, int checkvis )
{
	float f, max;
	vec3_t vec, vec2;
	vec3_t forward, right, up;

	VectorCopy(player->viewangles, vec);
	vec[0] = 0;
	AngleVectors(vec, forward, right, up);
//	for (i = 0; i < 3; i++)
//		forward[i] *= 3;

	max = 1000;

	if (autocam != CAM_FIRSTPERSON && autocam != CAM_TOPDOWN)
	{
		VectorAdd(forward, up, vec2);
		VectorAdd(vec2, right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorAdd(forward, up, vec2);
		VectorSubtract(vec2, right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorAdd(forward, right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorSubtract(forward, right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorAdd(forward, up, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorSubtract(forward, up, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorAdd(up, right, vec2);
		VectorSubtract(vec2, forward, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorSubtract(up, right, vec2);
		VectorSubtract(vec2, forward, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		// invert
		VectorSubtract(vec3_origin, forward, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorCopy(forward, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		// invert
		VectorSubtract(vec3_origin, right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
		VectorCopy(right, vec2);
		if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max)
		{
			max = f;
			VectorCopy(vec2, vec);
		}
	}

	// ack, can't find him
	if (max >= 1000)
	{
		Cam_Unlock();
		return FALSE;
	}
	locked = TRUE;
	VectorCopy(vec, desired_position);
	return TRUE;
}

static void Cam_CheckHighTarget( void )
{
	int i, j;
	player_info_t* s;

	j = -1;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		s = &cl.players[i];
		if (s->name[0] && !s->spectator)
		{
			j = i;
		}
	}
	if (j >= 0)
	{
		if (!locked)
			Cam_Lock(j);
	}
	else
		Cam_Unlock();
}

void Cam_GetPredictedTopDownOrigin( vec_t* v )
{
	vec3_t vec, origin, target;

	VectorCopy(vec3_origin, origin);
	VectorCopy(vec3_origin, target);
	target[2] += 800.0f;
	VectorSubtract(origin, target, vec);
	vectoangles(vec, v);
	v[0] = -v[0];
}

void Cam_GetTopDownOrigin( vec_t* source, vec_t* dest )
{
	vec3_t v;
	pmtrace_t trace;
	float len;

	VectorCopy(source, v);
	v[2] += 800;
	// v is endpos
	// fake a player move
	trace = Cam_DoTrace(source, v);
	if (trace.fraction < 1.0f)
		trace.endpos[2] -= 1.0f;

	VectorSubtract(trace.endpos, source, v);
	VectorScale(v, 0.5f, v);
	VectorAdd(source, v, dest);
	len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (len < 32.0f)
	{
		VectorCopy(source, dest);
		dest[2] += 32.0f;
	}
}

void Cam_GetPredictedFirstPersonOrigin( vec_t* v )
{
	int		i;
	float	f, ft, lerp;
	float	angleDelta, originDelta;
	vec3_t	curAngles, oldAngles, newAngles;
	frame_t* frame, * prevframe;
	player_state_t* state, * prevstate;

	frame = &cl.frames[cls.netchan.incoming_sequence & cl_update_mask];
	prevframe = &cl.frames[(cls.netchan.incoming_sequence - 1) & cl_update_mask];

	state = &frame->playerstate[spec_track];
	prevstate = &prevframe->playerstate[spec_track];

	ft = state->received_time - prevstate->received_time;
	if (ft == 0 || prevframe->receivedtime == -1 || frame->receivedtime == -1)
	{
		// can't interpolate
		VectorCopy(state->viewangles, v);
		VectorCopy(state->viewangles, v);
		return;
	}

	// calculate interpolation
	f = realtime - state->received_time;
	if (f < 0)
		f = 0;
	lerp = f / ft;
	if (lerp > 1)
		lerp = 1;

	VectorCopy(state->viewangles, curAngles);
	VectorCopy(prevstate->viewangles, oldAngles);

	// interpolate view angles and origin
	for (i = 0; i < 3; i++)
	{
		if (curAngles[i] < 0)
			curAngles[i] += 360;
		if (oldAngles[i] < 0)
			oldAngles[i] += 360;

		angleDelta = curAngles[i] - oldAngles[i];
		while (angleDelta > 180)
			angleDelta -= 360;
		while (angleDelta <= -180)
			angleDelta += 360;
		newAngles[i] = curAngles[i] + angleDelta * lerp;

		originDelta = state->origin[i] - prevstate->origin[i];
		cl.simorg[i] = state->origin[i] + originDelta * lerp;
	}

	VectorCopy(newAngles, v);
}

void Cam_TrackFirstPerson( usercmd_t* cmd )
{
	player_state_t* player, * self;
	frame_t* frame;

	if (!Cam_IsTracking(autocam) || cls.state != ca_active)
		return;

	if (locked && (!cl.players[spec_track].name[0] || cl.players[spec_track].spectator))
	{
		locked = FALSE;
		if (cl_hightrack.value)
			Cam_CheckHighTarget();
		else
			Cam_Unlock();		
		return;
	}

	frame = &cl.frames[cls.netchan.incoming_sequence & cl_update_mask];
	player = frame->playerstate + spec_track;
	self = frame->playerstate + cl.playernum;

	VectorCopy(player->origin, desired_position);

	// move there locally immediately
	VectorCopy(desired_position, self->origin);

	MSG_WriteByte(&cls.netchan.message, clc_tmove);
	MSG_WriteCoord(&cls.netchan.message, desired_position[0]);
	MSG_WriteCoord(&cls.netchan.message, desired_position[1]);
	MSG_WriteCoord(&cls.netchan.message, desired_position[2]);
}

void Cam_TrackTopDown( usercmd_t* cmd )
{
	player_state_t* player, * self;
	frame_t* frame;
	vec3_t v;

	if (!Cam_IsTracking(autocam) || cls.state != ca_active)
		return;

	if (locked && (!cl.players[spec_track].name[0] || cl.players[spec_track].spectator))
	{
		locked = FALSE;
		if (cl_hightrack.value)
			Cam_CheckHighTarget();
		else
			Cam_Unlock();		
		return;
	}

	frame = &cl.frames[cls.netchan.incoming_sequence & cl_update_mask];
	player = frame->playerstate + spec_track;
	self = frame->playerstate + cl.playernum;

	VectorCopy(player->origin, v);
	Cam_GetTopDownOrigin(v, desired_position);

	// move there locally immediately
	VectorCopy(desired_position, self->origin);

	MSG_WriteByte(&cls.netchan.message, clc_tmove);
	MSG_WriteCoord(&cls.netchan.message, desired_position[0]);
	MSG_WriteCoord(&cls.netchan.message, desired_position[1]);
	MSG_WriteCoord(&cls.netchan.message, desired_position[2]);
}

// ZOID
//
// Take over the user controls and track a player.
// We find a nice position to watch the player and move there
void Cam_Track( usercmd_t* cmd )
{
	player_state_t* player, * self;
	frame_t* frame;
	vec3_t vec;
	float len;

	if (!cl.spectator)
		return;

	if (autocam == CAM_FIRSTPERSON)
	{
		Cam_TrackFirstPerson(cmd);
		return;
	}

	if (autocam == CAM_TOPDOWN)
	{
		Cam_TrackTopDown(cmd);
		return;
	}

	if (cl_hightrack.value && !locked)
		Cam_CheckHighTarget();

	if (!Cam_IsTracking(autocam) || cls.state != ca_active)
		return;

	if (locked && (!cl.players[spec_track].name[0] || cl.players[spec_track].spectator))
	{
		locked = FALSE;
		if (cl_hightrack.value)
			Cam_CheckHighTarget();
		else
			Cam_Unlock();		
		return;
	}

	frame = &cl.frames[cls.netchan.incoming_sequence & cl_update_mask];
	player = frame->playerstate + spec_track;
	self = frame->playerstate + cl.playernum;

	if (!locked || !Cam_IsVisible(player, desired_position))
	{
		if (!locked || realtime - cam_lastviewtime > 0.1f)
		{
			if (!InitFlyby(self, player, TRUE))
				InitFlyby(self, player, FALSE);
			cam_lastviewtime = realtime;
		}
	}
	else
		cam_lastviewtime = realtime;

	// couldn't track for some reason
	if (!locked || !Cam_IsTracking(autocam))
		return;

	// Ok, move to our desired position and set our angles to view
	// the player
	VectorSubtract(desired_position, self->origin, vec);
	len = vlen(vec);
	cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
	if (len > 16)
	{ // close enough?
		MSG_WriteByte(&cls.netchan.message, clc_tmove);
		MSG_WriteCoord(&cls.netchan.message, desired_position[0]);
		MSG_WriteCoord(&cls.netchan.message, desired_position[1]);
		MSG_WriteCoord(&cls.netchan.message, desired_position[2]);
	}

	// move there locally immediately
	VectorCopy(desired_position, self->origin);

	VectorSubtract(player->origin, desired_position, vec);
	vectoangles(vec, cl.viewangles);
	cl.viewangles[0] = -cl.viewangles[0];
	VectorCopy(cl.viewangles, cl.simangles);
}

qboolean Cam_IsTracking( int cam )
{
	switch (cam)
	{
	case CAM_NONE:
		return FALSE;
	case CAM_TRACK:
		return TRUE;
	case CAM_FIRSTPERSON:
		return TRUE;
	case CAM_TOPDOWN:
		return TRUE;
	}

	return FALSE;
}

char* Cam_GetModeDescription( int cam )
{
	switch (cam)
	{
	case CAM_NONE:
		return "No camera";
	case CAM_TRACK:
		return "Tracking";
	case CAM_FIRSTPERSON:
		return "1st Person";
	case CAM_TOPDOWN:
		return "Top Down";
	}

	return "Unknown";
}

void Cam_FinishMove( usercmd_t* cmd )
{
	int i;
	player_info_t* s;
	int end;

	if (cls.state != ca_active)
		return;

	if (!cl.spectator) // only in spectator mode
		return;

	if (cmd->buttons & BUTTON_ATTACK)
	{
		if (!(oldbuttons & BUTTON_ATTACK))
		{
			oldbuttons |= BUTTON_ATTACK;
			autocam = (autocam + 1) % CAM_NUMMODES;

			Con_Printf("Specator mode set to %s\n", Cam_GetModeDescription(autocam));
			if (!Cam_IsTracking(autocam))
			{
				Cam_Unlock();
				VectorCopy(cl.viewangles, cmd->angles);
				return;
			}
		}
		else
			return;
	}
	else
	{
		oldbuttons &= ~BUTTON_ATTACK;
		if (!Cam_IsTracking(autocam))
			return;
	}

	if (cmd->buttons & BUTTON_ATTACK2)
	{
		if (!(oldbuttons & BUTTON_ATTACK2))
		{
			oldbuttons |= BUTTON_ATTACK2;
			autocam = (autocam - 1) % CAM_NUMMODES;

			Con_Printf("Specator mode set to %s\n", Cam_GetModeDescription(autocam));
			if (!Cam_IsTracking(autocam))
			{
				Cam_Unlock();
				VectorCopy(cl.viewangles, cmd->angles);
				return;
			}
		}
		else
			return;
	}
	else
	{
		oldbuttons &= ~BUTTON_ATTACK2;
		if (!Cam_IsTracking(autocam))
			return;
	}

	if (Cam_IsTracking(autocam) && cl_hightrack.value)
	{
		Cam_CheckHighTarget();
		return;
	}

	if (locked)
	{
		if ((cmd->buttons & BUTTON_JUMP) && (oldbuttons & BUTTON_JUMP))
			return;		// don't pogo stick

		if (!(cmd->buttons & BUTTON_JUMP))
		{
			oldbuttons &= ~BUTTON_JUMP;
			return;
		}
		oldbuttons |= BUTTON_JUMP;	// don't jump again until released
	}

//	Con_Printf("Selecting track target...\n");

	if (locked && Cam_IsTracking(autocam))
		end = (spec_track + 1) % MAX_CLIENTS;
	else
		end = spec_track;
	i = end;
	do
	{
		s = &cl.players[i];
		if (s->name[0] && !s->spectator)
		{
			Cam_Lock(i);
			return;
		}
		i = (i + 1) % MAX_CLIENTS;
	} while (i != end);
	// stay on same guy?
	i = spec_track;
	s = &cl.players[i];
	if (s->name[0] && !s->spectator)
	{
		Cam_Lock(i);
		return;
	}
	Con_Printf("No target found ...\n");
	autocam = locked = FALSE;
}

void Cam_Reset( void )
{
	autocam = CAM_NONE;
	spec_track = 0;
}

void CL_InitCam( void )
{
	Cvar_RegisterVariable(&cl_hightrack);
}

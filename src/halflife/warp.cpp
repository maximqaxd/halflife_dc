/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// warp.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"

#define SF_WARPBALL_FIRE_ONCE		0x0001
#define SF_WARPBALL_APPLY_DAMAGE	0x0002

class CWarpBall : public CBaseEntity
{
	friend void SR_Register_warp( void ); //SR_FRIEND


public:
	virtual int		Save( CSave& save );
	virtual int		Restore( CRestore& restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData *pkvd );

	void Spawn( void );
	void Precache( void );

	int Classify( void );

	void EXPORT BallThink( void );
	void EXPORT WarpBallUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	static CWarpBall *CreateWarpBall( Vector vecOrigin );

	CLightning* m_pBeams;
	CSprite* m_pSprite;
	int		m_iBeams;

	float	m_flLastTime;
	float	m_flMaxFrame;
	float	m_flBeamRadius;
	string_t m_iszWarpTarget;
	float	m_flWarpStart;
	float	m_flDamageDelay;
	float	m_flTargetDelay;

	BOOL	m_fPlaying;
	BOOL	m_fDamageApplied;
	BOOL	m_fBeamsCleared;
};

LINK_ENTITY_TO_CLASS( env_warpball, CWarpBall );

TYPEDESCRIPTION CWarpBall::m_SaveData[] =
{
	DEFINE_FIELD( CWarpBall, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_flLastTime, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flBeamRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_iszWarpTarget, FIELD_STRING ),
	DEFINE_FIELD( CWarpBall, m_flWarpStart, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flDamageDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flTargetDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_fPlaying, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fDamageApplied, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fBeamsCleared, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_pBeams, FIELD_CLASSPTR ),
	DEFINE_FIELD( CWarpBall, m_pSprite, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CWarpBall, CBaseEntity );

CWarpBall* CreateWarpBall( Vector vecOrigin )
{
	CWarpBall* warpBall = GetClassPtr( (CWarpBall*)NULL );
	UTIL_SetOrigin( warpBall->pev, vecOrigin );
	warpBall->pev->classname = MAKE_STRING( "env_warpball" );
	warpBall->Spawn();
	return warpBall;
}

void CWarpBall::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_flBeamRadius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "warp_target"))
	{
		m_iszWarpTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage_delay"))
	{
		m_flDamageDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CWarpBall::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10;

	m_pSprite = CSprite::SpriteCreate("sprites/Fexplo1.spr", pev->origin, TRUE);
	m_pSprite->TurnOff();

	SetUse( WarpBallUse );
}

void CWarpBall::BallThink( void )
{
	pev->frame = ((gpGlobals->time - m_flLastTime) * pev->framerate) + pev->frame;

	if (pev->frame > m_flMaxFrame)
	{
		SET_MODEL(edict(), "");

		SetThink( NULL );

		if (pev->spawnflags & SF_WARPBALL_FIRE_ONCE)
			UTIL_Remove(this);

		if (m_pSprite)
			m_pSprite->TurnOff();

		m_fPlaying = FALSE;
	}
	else
	{
		if ((pev->spawnflags & SF_WARPBALL_APPLY_DAMAGE) && !m_fDamageApplied && (gpGlobals->time - m_flWarpStart) >= m_flDamageDelay)
		{
			::RadiusDamage(pev->origin, pev, pev, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = TRUE;
		}

		if (m_pBeams)
		{
			if (pev->frame >= (m_flMaxFrame - 4.0f))
			{
				m_pBeams->SetThink( NULL );
				m_pBeams->pev->nextthink = gpGlobals->time;
			}
		}

		pev->nextthink = gpGlobals->time + 0.1f;
		m_flLastTime = gpGlobals->time;
	}
}

void CWarpBall::Precache( void )
{
	PRECACHE_MODEL("sprites/Fexplo1.spr");
	PRECACHE_MODEL("sprites/XFlare1.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/alien_teleport.wav");
}


void CWarpBall::WarpBallUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if (!m_fPlaying)
	{
		if (!FStringNull(m_iszWarpTarget))
		{
			edict_t* pentTarget = FIND_ENTITY_BY_STRING(NULL, "targetname", STRING(m_iszWarpTarget));
			if (pentTarget)
				UTIL_SetOrigin(pev, pentTarget->v.origin);
		}

		SET_MODEL(pev->pContainingEntity, "sprites/XFlare1.spr");

		m_flMaxFrame = MODEL_FRAMES(pev->modelindex) - 1;

		pev->rendercolor.x = 77;
		pev->rendercolor.y = 210;
		pev->rendercolor.z = 130;
		pev->scale = 1.2f;
		pev->frame = 0;

		if (m_pSprite)
		{
			m_pSprite->pev->rendermode = kRenderGlow;
			m_pSprite->pev->rendercolor.x = 77;
			m_pSprite->pev->rendercolor.y = 210;
			m_pSprite->pev->rendercolor.z = 130;
			m_pSprite->pev->renderamt = 255;
			m_pSprite->pev->renderfx = kRenderFxNoDissipation;
			m_pSprite->pev->scale = 1;
			m_pSprite->pev->framerate = 10;
			m_pSprite->TurnOn();
		}

		if (!m_pBeams)
		{
			m_pBeams = CLightning::LightningCreate("sprites/lgtning.spr", 18);

			m_pBeams->m_iszSpriteName = MAKE_STRING("sprites/lgtning.spr");

			m_pBeams->pev->origin = pev->origin;
			UTIL_SetOrigin(m_pBeams->pev, pev->origin);
			m_pBeams->m_restrike = -0.5f;
			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5f;
			m_pBeams->pev->rendercolor.x = 0;
			m_pBeams->pev->rendercolor.y = 255;
			m_pBeams->pev->rendercolor.z = 0;
			SetBits(m_pBeams->pev->spawnflags, SF_BEAM_SPARKEND);
			SetBits(m_pBeams->pev->spawnflags, SF_BEAM_TOGGLE);
			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;
			m_pBeams->BeamUpdateVars();
		}

		if (m_pBeams)
		{
			m_pBeams->pev->solid = 0;
			m_pBeams->Precache();
			m_pBeams->SetThink( CLightning :: StrikeThink );
			m_pBeams->pev->nextthink = gpGlobals->time + 0.1f;
		}

		SetThink( BallThink );
		pev->nextthink = gpGlobals->time + 0.1f;

		m_flLastTime = gpGlobals->time;
		m_fBeamsCleared = FALSE;
		m_fPlaying = TRUE;

		if (m_flDamageDelay == 0)
		{
			::RadiusDamage(pev->origin, pev, pev, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = TRUE;
		}
		else
		{
			m_fDamageApplied = FALSE;
		}

		SUB_UseTargets(this, USE_TOGGLE, 0);
		UTIL_ScreenShake(pev->origin, 4, 100, 2, 1000);
		m_flWarpStart = gpGlobals->time;
		EMIT_SOUND(edict(), CHAN_WEAPON, "debris/alien_teleport.wav", VOL_NORM, ATTN_NORM);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CWarpBall::Classify( void )
{
	return CLASS_NONE;
}

// BEGIN GENERATED SAVE-RESTORE EXPORTS
void SR_Register_warp( void )
{
	SR_REGISTER( "IX", CWarpBall, BallThink );
	SR_REGISTER( "IY", CWarpBall, WarpBallUse );
}
// END GENERATED SAVE-RESTORE EXPORTS

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
//
// crouchstate.cpp
//
// implementation of CHudCrouchState class
//

#include "hud.h"
#include "util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

extern "C" int scr_safe_x;
extern "C" int scr_safe_y;

DECLARE_MESSAGE(m_CrouchState, CrouchState)

int CHudCrouchState::Init(void)
{
	HOOK_MESSAGE(CrouchState);

	m_iState = STANCE_WALK;

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
}

void CHudCrouchState::Reset(void)
{
	m_iState = STANCE_WALK;
}

int CHudCrouchState::VidInit(void)
{
	int HUD_crouch_on = gHUD.GetSpriteIndex( "crouch_on" );
	int HUD_walk = gHUD.GetSpriteIndex( "walk" );
	int HUD_run = gHUD.GetSpriteIndex( "run" );

	m_hCrouch = gHUD.GetSprite( HUD_crouch_on );
	m_hWalk = gHUD.GetSprite( HUD_walk );
	m_hRun = gHUD.GetSprite( HUD_run );

	m_prcCrouch = &gHUD.GetSpriteRect( HUD_crouch_on );
	m_prcWalk = &gHUD.GetSpriteRect( HUD_walk );
	m_prcRun = &gHUD.GetSpriteRect( HUD_run );

	m_iWidth = m_prcWalk->right - m_prcWalk->left;

	return 1;
}

int CHudCrouchState::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int r, g, b, x, y;

	r = hud_color_r;
	g = hud_color_g;
	b = hud_color_b;
	ScaleColors(r, g, b, 225);

	// tuck the icon into the bottom right corner, inside the safe area
	y = ((m_prcCrouch->bottom - m_prcWalk->top) / 2) + scr_safe_y + 32;
	x = ScreenWidth - m_iWidth - (m_iWidth / 2) - scr_safe_x;

	switch ( m_iState )
	{
	case STANCE_RUN:
		SPR_Set( m_hRun, r, g, b );
		SPR_DrawAdditive( 0, x, y, m_prcRun );
		break;

	case STANCE_CROUCH:
		SPR_Set( m_hCrouch, r, g, b );
		SPR_DrawAdditive( 0, x, y, m_prcCrouch );
		break;

	default:
		SPR_Set( m_hWalk, r, g, b );
		SPR_DrawAdditive( 0, x, y, m_prcWalk );
		break;
	}

	return 1;
}

int CHudCrouchState::MsgFunc_CrouchState( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iState = READ_BYTE();

	return 1;
}

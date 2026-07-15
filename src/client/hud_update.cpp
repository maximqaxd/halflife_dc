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
//  hud_update.cpp
//

#include <math.h>
#include "hud.h"
#include "util.h"
#include <stdlib.h>



int CHud::UpdateClientData(client_data_t *cdata, float time)
{
	memcpy(m_vecOrigin, cdata->origin, sizeof(vec3_t));
	memcpy(m_vecAngles, cdata->viewangles, sizeof(vec3_t));
	m_iKeyBits = cdata->iKeyBits;
	m_iWeaponBits = cdata->iWeaponBits;
	gHUD.Think();

	cdata->iKeyBits = m_iKeyBits;
	cdata->fov = m_iFOV;
	cdata->view_idlescale = m_iConcussionEffect;

	if ( m_flMouseSensitivity )
		cdata->mouse_sensitivity = m_flMouseSensitivity;

	// return 1 if in anything in the client_data struct has been changed, 0 otherwise
	return 1;
}



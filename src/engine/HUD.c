// HUD.c - heads up display code

#include "quakedef.h"
#include "screen.h"
#include "cl_tent.h"
#include "pr_cmds.h"

int	gHealth = 100;
float gfFade;
int	gAmmo;
float gfAmmoFade;

int giHudLevel = 1;
int giSelectMode = 1;
int giSelAmmo1 = -1;
int giSelAmmo2 = -1;
int giClipAmmo;
int giTotalAmmo;
int giSecondAmmo;
int giAmmoDigits;
int giHealthWidth;
int giAmmoWidth;
int giGeigerRange = -1;
float gflGeigerDelay = -1.0f;

extern cvar_t crosshair;
extern vrect_t scr_vrect;

void DrawCrosshair( int x, int y );

void HudSizeUp( void )
{
	if (scr_viewsize.value < 120)
		Cvar_SetValue("viewsize", scr_viewsize.value + 10);
}

void HudSizeDown( void )
{
	if (scr_viewsize.value > 30)
		Cvar_SetValue("viewsize", scr_viewsize.value - 10);
}

/*
===============
Sbar_Geiger
===============
*/
void Sbar_Geiger( void )
{
	int pct;
	float flvol;
	sfx_t* rgsfx[3];
	int i;

	if (cl.time < gflGeigerDelay)
		return;

	gflGeigerDelay = cl.time + 0.1f;

	if (giGeigerRange < 1000 && giGeigerRange > 0)
	{
		// piecewise linear is better than continuous formula for this
		if (giGeigerRange > 800)
		{
			pct = 0;			//Con_Printf("range > 800\n");
		}
		else if (giGeigerRange > 600)
		{
			pct = 2;
			flvol = 0.4f;		//Con_Printf("range > 600\n");
			rgsfx[0] = cl_sfx_geiger1;
			rgsfx[1] = cl_sfx_geiger1;
			i = 2;
		}
		else if (giGeigerRange > 500)
		{
			pct = 4;
			flvol = 0.5f;		//Con_Printf("range > 500\n");
			rgsfx[0] = cl_sfx_geiger1;
			rgsfx[1] = cl_sfx_geiger2;
			i = 2;
		}
		else if (giGeigerRange > 400)
		{
			pct = 8;
			flvol = 0.6f;		//Con_Printf("range > 400\n");
			rgsfx[0] = cl_sfx_geiger1;
			rgsfx[1] = cl_sfx_geiger2;
			rgsfx[2] = cl_sfx_geiger3;
			i = 3;
		}
		else if (giGeigerRange > 300)
		{
			pct = 8;
			flvol = 0.7f;		//Con_Printf("range > 300\n");
			rgsfx[0] = cl_sfx_geiger2;
			rgsfx[1] = cl_sfx_geiger3;
			rgsfx[2] = cl_sfx_geiger4;
			i = 3;
		}
		else if (giGeigerRange > 200)
		{
			pct = 28;
			flvol = 0.78f;		//Con_Printf("range > 200\n");
			rgsfx[0] = cl_sfx_geiger2;
			rgsfx[1] = cl_sfx_geiger3;
			rgsfx[2] = cl_sfx_geiger4;
			i = 3;
		}
		else if (giGeigerRange > 150)
		{
			pct = 40;
			flvol = 0.80f;		//Con_Printf("range > 150\n");
			rgsfx[0] = cl_sfx_geiger3;
			rgsfx[1] = cl_sfx_geiger4;
			rgsfx[2] = cl_sfx_geiger5;
			i = 3;
		}
		else if (giGeigerRange > 100)
		{
			pct = 60;
			flvol = 0.85f;		//Con_Printf("range > 100\n");
			rgsfx[0] = cl_sfx_geiger3;
			rgsfx[1] = cl_sfx_geiger4;
			rgsfx[2] = cl_sfx_geiger5;
			i = 3;
		}
		else if (giGeigerRange > 75)
		{
			pct = 80;
			flvol = 0.9f;		//Con_Printf("range > 75\n");
			//gflGeigerDelay = cl.time + GEIGERDELAY * 0.75;
			rgsfx[0] = cl_sfx_geiger4;
			rgsfx[1] = cl_sfx_geiger5;
			rgsfx[2] = cl_sfx_geiger6;
			i = 3;
		}
		else if (giGeigerRange > 50)
		{
			pct = 90;
			flvol = 0.95f;		//Con_Printf("range > 50\n");
			rgsfx[0] = cl_sfx_geiger5;
			rgsfx[1] = cl_sfx_geiger6;
			i = 2;
		}
		else
		{
			pct = 95;
			flvol = 1.0f;		//Con_Printf("range < 50\n");
			rgsfx[0] = cl_sfx_geiger5;
			rgsfx[1] = cl_sfx_geiger6;
			i = 2;
		}

		flvol = flvol * RandomFloat(0.0f, 0.5f) + 0.25f;

		if (RandomLong(0, 127) < pct || RandomLong(0, 127) < pct)
		{
			S_StartDynamicSound(-1, CHAN_AUTO, rgsfx[RandomLong(0, i - 1)], r_origin, flvol, 1.0f, 0, PITCH_NORM);
		}
	}
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw( void )
{
	float x, y;
	vec3_t angles;
	vec3_t forward;
	vec3_t point, screen;

	if (cls.state != ca_active)
		return;

	if (giHudLevel == 0)
		return;

	if (scr_con_current == vid.height)
		return; // console is full screen

	scr_copyeverything = TRUE;

	if (crosshair.value && !scr_drawloading)
	{
		x = scr_vrect.x + (scr_vrect.width / 2);
		y = scr_vrect.y + (scr_vrect.height / 2);

		VectorAdd(r_refdef.viewangles, cl.crosshairangle, angles);
		AngleVectors(angles, forward, NULL, NULL);
		VectorAdd(r_origin, forward, point);
		ScreenTransform(point, screen);
		DrawCrosshair(x + (0.5f * screen[0] * scr_vrect.width + 0.5f), y + (0.5f * screen[1] * scr_vrect.height + 0.5f));
	}

	ClientDLL_HudRedraw(0);
}

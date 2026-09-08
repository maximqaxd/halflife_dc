// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "dreamcast_crt.h"
#include "dc_accum.h"
/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full


*/


// In other C files.
qboolean V_CheckGamma( void );
void V_UpdatePalette( void );
void UI_Draw( void );
void UI_Update( void );
void Host_CheckController( void );
void Host_DrawMessage( void );
void R_DrawAdaptive( void );
void SCR_DrawConnectMsg( void );

extern int	gfDrawMenu;			// draw the menu instead of the 3D view
extern float g_flHudDepth;		// current HUD depth sublayer
extern cvar_t cl_adaptive;

int			glx, gly, glwidth, glheight;

// only the refresh window will be updated unless these variables are flagged
int			scr_copytop;
int			scr_copyeverything;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

cvar_t		scr_viewsize = { "viewsize", "120", FCVAR_ARCHIVE };
float		scr_fov_value = 90;	// 10 - 170
cvar_t		scr_conspeed = { "scr_conspeed", "600" };
cvar_t		scr_centertime = { "scr_centertime", "2" };
cvar_t		scr_showram = { "showram", "0" };
cvar_t		scr_showpause = { "showpause", "1" };
cvar_t		scr_printspeed = { "scr_printspeed", "8" };
cvar_t		scr_netusage = { "netusage", "0" };
cvar_t		scr_graphheight = { "graphheight", "64.0f" };
cvar_t		scr_graphmedian = { "graphmedian", "128.0f" };
cvar_t		scr_graphhigh = { "graphhigh", "512.0f" };
cvar_t		scr_graphmean = { "graphmean", "1" };
cvar_t		scr_downloading = { "scr_downloading", "-1.0" };
cvar_t		scr_transparentui = { "scr_transparentui", "1" };
cvar_t		scr_connectmsg = { "scr_connectmsg", "0" };
cvar_t		scr_connectmsg1 = { "scr_connectmsg1", "0" };
cvar_t		scr_connectmsg2 = { "scr_connectmsg2", "0" };
cvar_t		forcefog = { "forcefog", "0" };
float		downloadpercent = -1.0;

qboolean	scr_initialized;		// ready to draw

//qpic_t* scr_ram;
//qpic_t* scr_net;

int			scr_fullupdate;

int			clearconsole;
int			clearnotify;

int			sb_lines;

viddef_t	vid;				// global video state

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;

qboolean	scr_skipupdate;

qboolean	scr_drawloading;
float		scr_disabled_time;

void SCR_ScreenShot_f( void );

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;   // for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
=================
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
=================
*/
void SCR_CenterPrint( char* str )
{
	strncpy(scr_centerstring, str, sizeof(scr_centerstring) - 1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}


void SCR_DrawCenterString( void )
{
	char*		start;
	int			l;
	int			j;
	int			x, y;
	int			remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = glheight * 0.35f;
	else
		y = 48;

	do
	{
	// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (glwidth - l * 8) / 2;
		for (j = 0; j < l; j++, x += 8)
		{
			Draw_Character(x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 12;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString( void )
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;

	SCR_DrawCenterString();
}

//=============================================================================

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef( void )
{
	float		size;
	float		full;
	int			h;

	scr_fullupdate = 0;             // force a background redraw
	vid.recalc_refdef = 0;

	scr_viewsize.value = 120.0f;

// bound viewsize
	if (scr_viewsize.value < 30.0f)
		Cvar_Set("viewsize", "30");
	if (scr_viewsize.value > 120.0f)
		Cvar_Set("viewsize", "120");

// bound field of view
	if (scr_fov_value < 10.0f)
		scr_fov_value = 10.0f;
	if (scr_fov_value > 170.0f)
		scr_fov_value = 170.0f;

	size = scr_viewsize.value;

// intermission is always full screen
	if (cl.intermission)
		full = 120.0f;
	else
		full = size;

	if (full >= 120.0f)
		sb_lines = 0;			// no status bar at all
	else if (full >= 110.0f)
		sb_lines = 24;			// no inventory
	else
		sb_lines = 24 + 16 + 8;

	if (size > 100.0f)
		size = 100.0f;

	if (cl.intermission)
	{
		sb_lines = 0;
		size = 100.0f;
	}
	size /= 100.0f;

	h = glheight - sb_lines;

	r_refdef.vrect.width = glwidth * size;
	if (r_refdef.vrect.width < 96)
	{
		size = 96.0f / r_refdef.vrect.width;
		r_refdef.vrect.width = 96;		// min for icons
	}

	r_refdef.vrect.height = glheight * size;
	if (r_refdef.vrect.height > h)
		r_refdef.vrect.height = h;

	r_refdef.vrect.x = (glwidth - r_refdef.vrect.width) / 2;
	r_refdef.vrect.y = (h - r_refdef.vrect.height) / 2;

	scr_vrect = r_refdef.vrect;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f( void )
{
	HudSizeUp();
	vid.recalc_refdef = 1;
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f( void )
{
	HudSizeDown();
	vid.recalc_refdef = 1;
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void )
{
	Cvar_RegisterVariable(&scr_viewsize);
	Cvar_RegisterVariable(&scr_showpause);
	Cvar_RegisterVariable(&scr_conspeed);
	Cvar_RegisterVariable(&scr_centertime);
	Cvar_RegisterVariable(&scr_printspeed);
	Cvar_RegisterVariable(&scr_showram);
	Cvar_RegisterVariable(&scr_netusage);
	Cvar_RegisterVariable(&scr_graphheight);
	Cvar_RegisterVariable(&scr_graphmedian);
	Cvar_RegisterVariable(&scr_graphhigh);
	Cvar_RegisterVariable(&scr_graphmean);

	NET_InitColors();

	Cvar_RegisterVariable(&scr_transparentui);
	Cvar_RegisterVariable(&scr_connectmsg);
	Cvar_RegisterVariable(&scr_connectmsg1);
	Cvar_RegisterVariable(&scr_connectmsg2);
	Cvar_RegisterVariable(&forcefog);

//
// register our commands
//
	Cmd_AddCommand("screenshot", SCR_ScreenShot_f);
	Cmd_AddCommand("sizeup", SCR_SizeUp_f);
	Cmd_AddCommand("sizedown", SCR_SizeDown_f);

	scr_initialized = TRUE;
}

/*
==============
Draw_CenterPic
==============
*/
void Draw_CenterPic( qpic_t* pPic )
{
	int			x, y;

	x = glwidth / 2;
	y = glheight / 2;

	Draw_Pic(x - pPic->width / 2, y - pPic->height / 2, pPic);
}

/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam( void )
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading( void )
{
	if (!scr_drawloading)
		return;

	DCV_SetHudDepth(g_flHudDepth + 3.0f);
	Draw_BeginDisc();
	DCV_SetHudDepth(g_flHudDepth - 3.0f);
}

//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole( void )
{
	Con_CheckResize();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = vid.height / 2;	// half screen
	else
		scr_conlines = 0;				// none visible

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed.value * host_frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed.value * host_frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
	{
	}
	else if (clearnotify++ < vid.numpages)
	{
	}
	else
		con_notifylines = 0;
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole( void )
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole((int)scr_con_current, TRUE);
		clearconsole = 0;
	}
	else if (key_dest == key_game || key_dest == key_message)
		Con_DrawNotify();		// only draw notify in game
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char pixel_size, attributes;
} TargaHeader;


/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f( void )
{

}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque( void )
{
// redraw with no console and the loading plaque
	Con_ClearNotify();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = TRUE;
	scr_fullupdate = 0;
	SCR_UpdateScreen();
	SCR_UpdateScreen();
	SCR_UpdateScreen();

	scr_disabled_for_loading = TRUE;
	scr_disabled_time = realtime;
	scr_fullupdate = 0;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque( void )
{
	scr_disabled_for_loading = FALSE;
	scr_fullupdate = 0;
	scr_drawloading = FALSE;
	Con_ClearNotify();
}

//=============================================================================

char*		scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString( void )
{
	char*		start;
	int			l;
	int			j;
	int			x, y;

	start = scr_notifystring;

	y = glheight * 0.35f;

	do
	{
	// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (glwidth - l * 8) / 2;
		for (j = 0; j < l; j++, x += 8)
			Draw_Character(x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage( char* text )
{
	if (cls.state == ca_dedicated)
		return TRUE;

	scr_notifystring = text;

// draw a fresh screen
	scr_fullupdate = 0;
	scr_drawdialog = TRUE;
	SCR_UpdateScreen();
	scr_drawdialog = FALSE;

	S_ClearBuffer(TRUE);		// so dma doesn't loop current sound

	do
	{
		key_count = -1;		// wait for a key down and up
		Sys_SendKeyEvents();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	scr_fullupdate = 0;
	SCR_UpdateScreen();

	return key_lastpress == 'y';
}


//=============================================================================

/*
===================
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole( void )
{
	int			i;

	scr_centertime_off = 0;

	for (i = 0; i < 20 && scr_conlines != scr_con_current; i++)
		SCR_UpdateScreen();
}

/*
===============
SCR_TileClear
================
*/
void SCR_TileClear( void )
{
	if (r_refdef.vrect.x > 0)
	{
		Draw_TileClear(0, 0, r_refdef.vrect.x, 152);
		Draw_TileClear(r_refdef.vrect.x + r_refdef.vrect.width, 0, 320 - r_refdef.vrect.x + r_refdef.vrect.width, 152);
	}

	if (r_refdef.vrect.height < 152)
	{
		Draw_TileClear(r_refdef.vrect.x, 0, r_refdef.vrect.width, r_refdef.vrect.y);
		Draw_TileClear(r_refdef.vrect.x,
			r_refdef.vrect.y + r_refdef.vrect.height,
			r_refdef.vrect.width,
			152 - (r_refdef.vrect.y + r_refdef.vrect.height));
	}
}

/*
==================
SCR_UpdateScreen

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen( void )
{
	static qboolean recursionGuard = FALSE;

	V_CheckGamma();

	if (gfBackground || scr_skipupdate)
		return;

	if (recursionGuard)
	{
		recursionGuard = FALSE;
		return;
	}

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time <= 60)
			return;

		scr_disabled_for_loading = FALSE;
		Con_Printf("load failed.\n");
	}

	if (cls.state == ca_dedicated)
		return;				// stdout only

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	// Rebuild the palette and gamma ramps if any of the gamma cvars changed.
	V_UpdatePalette();

	if (gfDrawMenu)
	{
		// the menu covers the whole screen: no world refresh needed
		GL_BeginRendering(&glx, &gly, &glwidth, &glheight);
		GLBeginHud();
		UI_Draw();
		UI_Update();
		GLFinishHud();
		GL_EndRendering();
		return;
	}

	Host_CheckController();

	GL_BeginRendering(&glx, &gly, &glwidth, &glheight);

	//
	// determine size of refresh window
	//
	if (vid.recalc_refdef)
		SCR_CalcRefdef();

	if (key_dest != key_ui || scr_transparentui.value)
	{
	//
	// do 3D refresh drawing, and then update the screen
	//
		SCR_SetUpToDrawConsole();

		// Draw world, etc.
		V_RenderView();

		GLBeginHud();
		//
		// draw any areas not covered by the refresh
		//
		SCR_TileClear();

		if (scr_drawdialog)
		{
			Sbar_Draw();
			Draw_FadeScreen();
			SCR_DrawNotifyString();
			scr_copyeverything = TRUE;
		}
		else if (scr_drawloading)
		{
			SCR_DrawLoading();
			Sbar_Draw();
		}
		else if (cl.intermission == 1 && key_dest == key_game)
		{
			ClientDLL_HudRedraw(1);
		}
		else if (cl.intermission == 2 && key_dest == key_game)
		{
			SCR_CheckDrawCenterString();
		}
		else
		{
			GL_Bind(r_notexture_mip->gl_texturenum, 0);

			if ((float)vid.height > scr_con_current)
				Sbar_Draw();

			SCR_CheckDrawCenterString();

			if (scr_con_current == 0)
			{
				if (key_dest == key_game || key_dest == key_message)
					Con_DrawNotify();	// only draw notify in game
			}
			else
			{
				scr_copyeverything = 1;
				Con_DrawConsole((int)scr_con_current, TRUE);
				scr_fullupdate = 0;
			}
		}

		if (scr_netusage.value)
		{
			if ((float)vid.height > scr_con_current)
				SCR_NetUsage();
		}

		if (r_netgraph.value)
			R_NetGraph();

		if (cl_adaptive.value)
			R_DrawAdaptive();

		CL_ShowSizes();

		SCR_DrawDownloadInfo();
		SCR_DrawConnectMsg();
		SCR_DrawDownloadProgress();

		Host_DrawMessage();

		GLFinishHud();
	}

	GL_EndRendering();
}

/*
================
D_FillRect
================
*/
void D_FillRect( vrect_t* r, byte* color )
{

}

/*
=================
SCR_DrawConnectMsg

Draws up to three lines of server-provided connect text, centered above the
bottom of the refresh window. A leading "0" hides a line.
=================
*/
void SCR_DrawConnectMsg( void )
{
	int			w, h, x, y;
	vrect_t		rcFill;
	byte		color[3];

	if (scr_connectmsg.string[0] == '0')
		return;

	if (scr_vrect.width <= 250)
		w = scr_vrect.width - 2;
	else
		w = 250;

	if (scr_vrect.height <= 10)
		h = scr_vrect.height - 2;
	else
		h = 10;

	x = scr_vrect.x + (scr_vrect.width - w) / 2 + 1;
	y = (scr_vrect.height - h * 5) - h / 2;

	rcFill.x = x - 5;
	rcFill.y = y - h / 2;
	rcFill.width = w + 10;
	rcFill.height = h * 4;

	color[0] = color[1] = color[2] = 0;
	D_FillRect(&rcFill, color);

	Draw_String(x, y, scr_connectmsg.string);

	if (scr_connectmsg1.string[0] != '0')
	{
		y += h;
		Draw_String(x, y, scr_connectmsg1.string);
	}
	if (scr_connectmsg2.string[0] != '0')
	{
		y += h;
		Draw_String(x, y, scr_connectmsg2.string);
	}
}

/*
=================
SCR_DrawDownloadText

=================
*/
void SCR_DrawDownloadText( void )
{
	char		szStatusText[128];
	int			i;
	int			w, h, x, y;
	int			recieved;
	int			bytes;
	float		speed;
	float		time;
	float		remaining;
	vrect_t		rcFill;
	byte		color[3];
	downloadtime_t* dt1;
	downloadtime_t* dt2;

	if (cls.state == ca_active)
		return;

	bytes = 0;
	speed = 0.0;
	time = 0.0;

	recieved = cls.nTotalToTransfer - cls.nRemainingToTransfer;

	for (i = 0; i < MAX_DL_STATS; i++)
	{
		dt1 = &cls.rgDownloads[(cls.downloadnumber - i - 2) & (MAX_DL_STATS - 1)];
		dt2 = &cls.rgDownloads[(cls.downloadnumber - i - 1) & (MAX_DL_STATS - 1)];

		if (dt1->bUsed && dt2->bUsed && dt2->fTime >= dt1->fTime)
		{
			bytes += dt1->nBytesRemaining - dt2->nBytesRemaining;
			time += dt2->fTime - dt1->fTime;
		}
	}

	if (time != 0)
		speed = bytes / time;

	if (speed != 0)
	{
		int			sec, minute, hour;

		if (scr_vrect.width <= 250)
			w = scr_vrect.width - 2;
		else
			w = 250;

		if (scr_vrect.height <= 10)
			h = scr_vrect.height - 2;
		else
			h = 10;

		x = scr_vrect.x + (scr_vrect.width - w) / 2 + 1;
		y = scr_vrect.y + scr_vrect.height - 12;
		if (cls.state != ca_active)
			y = scr_vrect.y + scr_vrect.height - 92;
		y = max(y, 22);

		rcFill.x = x;
		rcFill.y = y;
		rcFill.width = w;
		rcFill.height = h;

		color[0] = color[1] = color[2] = 0;
		D_FillRect(&rcFill, color);

		remaining = cls.nRemainingToTransfer / speed;

		sec = (int)remaining;
		minute = sec / 60;
		if (minute == 0)
			hour = 0;
		else
		{
			sec -= minute * 60;
			hour = minute / 60;
			if (hour)
				minute -= hour * 60;
		}

		if (hour)
			sprintf(szStatusText, "%iK received, %i:%02i:%02i remaining...\n", recieved / 1024, hour, minute, sec);
		else
			sprintf(szStatusText, "%iK received, %02i:%02i remaining...\n", recieved / 1024, minute, sec);
		Draw_String(x, y, szStatusText);
	}
}

/*
=================
SCR_DrawDownloadInfo

=================
*/
void SCR_DrawDownloadInfo( void )
{
	int			percent;
	int			w, h, x, y;
	vrect_t		rcFill;
	byte		color[3];

	if (scr_downloading.value < 0)
		return;

	if (!cls.download)
	{
		scr_downloading.value = -1.0;
		return;
	}

	percent = (int)scr_downloading.value;
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	if (cls.state == ca_active)
	{
	// small bar tucked into the corner of the refresh window
		if (scr_vrect.width < 101)
			w = scr_vrect.width - 10;
		else
			w = 100;

		if (scr_vrect.height < 7)
			h = scr_vrect.height - 2;
		else
			h = 6;

		x = scr_vrect.x + 3;
		y = scr_vrect.y + scr_vrect.height - 3 - h;
	}
	else
	{
	// big bar centered over the loading screen
		if (scr_vrect.width <= 250)
			w = scr_vrect.width - 2;
		else
			w = 250;

		if (scr_vrect.height <= 10)
			h = scr_vrect.height - 2;
		else
			h = 10;

		x = scr_vrect.x + (scr_vrect.width - w) / 2 + 1;
		y = scr_vrect.y + scr_vrect.height - 102;
	}

	if (y < 3)
		y = 2;

	// Background
	rcFill.x = x;
	rcFill.y = y;
	rcFill.width = w;
	rcFill.height = h;

	color[0] = 63;
	color[1] = 63;
	color[2] = 63;
	D_FillRect(&rcFill, color);

	// Progress bar
	rcFill.x += 2;
	rcFill.y += 2;
	rcFill.width = percent / 100.0f * (w - 4) + 0.5f;
	rcFill.height -= 4;

	color[0] = 127;
	color[1] = 63;
	color[2] = 0;
	D_FillRect(&rcFill, color);

	// Remaining space
	rcFill.x += rcFill.width;
	rcFill.width = (w - 4) - (percent / 100.0f * (w - 4) + 0.5f);

	color[0] = 0;
	color[1] = 0;
	color[2] = 0;
	D_FillRect(&rcFill, color);

	// Draw text info
	SCR_DrawDownloadText();
}

/*
=================
SCR_DrawDownloadProgress

=================
*/
void SCR_DrawDownloadProgress( void )
{
	int			percent;
	int			offset;
	int			w, h, x, y;
	float		scale;
	float		progress;
	vrect_t		rcFill;
	byte		color[3];

	if (scr_disabled_for_loading || downloadpercent < 0)
		return;

	if (cls.state != ca_connected && cls.state != ca_uninitialized)
	{
		downloadpercent = -1.0;
		return;
	}

	percent = (int)downloadpercent;
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	scale = scr_vrect.height / 240.0f;
	if (scale < 1.0f)
		scale = 1.0f;

	w = scr_vrect.width >> 1;
	h = scale * 8;

	offset = h / 4;
	if (offset < 2)
		offset = 2;

	x = scr_vrect.x + w / 2;
	y = scr_vrect.height * 0.6f;

	// Background
	rcFill.x = x;
	rcFill.y = y;
	rcFill.width = w;
	rcFill.height = h;

	color[0] = color[1] = color[2] = 0;
	D_FillRect(&rcFill, color);

	// Progress bar
	progress = percent / 100.0f;
	rcFill.x += offset;
	rcFill.y += offset;
	rcFill.height -= 2 * offset;
	rcFill.width = progress * (w - 2 * offset) + 0.5f;

	color[0] = 255;
	color[1] = 180;
	color[2] = 56;
	D_FillRect(&rcFill, color);
}

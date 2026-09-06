// console.c

#ifdef WIN32
#include <io.h>
#endif

#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#include "quakedef.h"
#include "winquake.h"

qboolean	con_loading = FALSE;

int 		con_linewidth;

float		con_cursorspeed = 4;

#define		CON_TEXTSIZE	16384

qboolean 	con_forcedup;		// because no entities to refresh

int			con_totallines;		// total lines in console scrollback
int			con_backscroll;		// lines up from bottom to display
int			con_current;		// where next message will be printed
int			con_x;				// offset in current line for next print
int			con_rows = 0;
char* con_text = NULL;

cvar_t		con_notifytime = { "con_notifytime", "4" };		//seconds

#define	NUM_CON_TIMES 4
int con_num_times = NUM_CON_TIMES;

float* con_times;	// realtime time the line was generated
					// for transparent notify lines
int* con_times_lines;	// scrollback line each notify slot points at

int			con_vislines;

qboolean	con_debuglog;

#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;


qboolean	con_initialized;

int			con_notifylines;		// scan lines to clear for notify lines

// On-screen text origin. The Dreamcast build insets all console/HUD/notify text
// by a fixed TV-overscan-safe margin; DCV_GammaRefresh_f recomputes these when
// the video mode changes. 8,24 are the defaults.
int			scr_safe_x = 8;
int			scr_safe_y = 24;

extern int	Font_CharHeight( qfont_t* font );				// render/text_draw.cpp
extern int	Font_StringWidth( qfont_t* font, char* text );	// render/text_draw.cpp
extern void	DCV_SetPackedColor( unsigned long diffuse );	// render/dc_accum.c

/*
================
Con_SetTimes_f
================
*/
void Con_SetTimes_f( void )
{
	int newtimes;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("contimes <n>\nShow <n> overlay lines [4-64].\n%i current overlay lines.\n", con_num_times);
		return;
	}

	newtimes = atoi(Cmd_Argv(1));
	if (newtimes < NUM_CON_TIMES)
		newtimes = NUM_CON_TIMES;
	if (newtimes > 64)
		newtimes = 64;

	if (con_times)
		free(con_times);
	if (con_times_lines)
		free(con_times_lines);

	con_times = malloc(sizeof(float) * newtimes);
	con_times_lines = malloc(sizeof(int) * newtimes);
	if (!con_times || !con_times_lines)
		Sys_Error("Couldn't allocate space for %i console overlays.", newtimes);

	con_num_times = newtimes;
	Con_Printf("%i lines will overlay.\n", newtimes);
}

/*
================
Con_HideConsole_f

================
*/
void Con_HideConsole_f( void )
{
	if (key_dest == key_console)
	{
		// hide everything
		Con_ToggleConsole_f();
	}
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void )
{
	if (key_dest == key_console)
	{
		if (cls.state == ca_connected ||
			cls.state == ca_uninitialized ||
			cls.state == ca_active)
		{
			con_backscroll = 0;
			key_dest = key_game;
			key_lines[edit_line][1] = 0;	// clear any typing
			key_linepos = 1;
		}
		else
		{
			giActive = DLL_PAUSED;
		}
	}
	else if (!(giSubState & 4) && !con_loading)
	{
		con_backscroll = 0;
		key_dest = key_console;
	}

	SCR_EndLoadingPlaque();
	memset(con_times, 0, sizeof(float));
	memset(con_times_lines, 0, sizeof(int));
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void )
{
	if (con_text)
		Q_memset(con_text, ' ', CON_TEXTSIZE);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void )
{
	int		i;

	for (i = 0; i < con_num_times; i++)
		con_times[i] = 0;
}


/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f( void )
{
	con_backscroll = 0;
	key_dest = key_message;

	if (Cmd_Argc() == 2)
	{
		strcpy(message_type, Cmd_Argv(1));
	}
	else
	{
		strcpy(message_type, "say");
	}
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f( void )
{
	con_backscroll = 0;
	key_dest = key_message;
	strcpy(message_type, "say_team");
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void )
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = ((640 - scr_safe_x * 2) >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)
	{
		width = 38;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		Q_memset(con_text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;

		if (con_linewidth < numchars)
			numchars = con_linewidth;

		Q_memcpy(tbuf, con_text, CON_TEXTSIZE);
		Q_memset(con_text, ' ', CON_TEXTSIZE);

		for (i = 0; i < numlines; i++)
		{
			for (j = 0; j < numchars; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linewidth + j] =
					tbuf[((con_current - i + oldtotallines) %
						oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}


/*
================
Con_Init
================
*/
void Con_Init( void )
{
#define MAXGAMEDIRLEN	1000
	char	temp[MAXGAMEDIRLEN + 1];
	char* t2 = "/qconsole.log";

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		if (strlen(com_gamedir) < (MAXGAMEDIRLEN - strlen(t2)))
		{
			sprintf(temp, "%s%s", com_gamedir, t2);
		}
	}

	con_text = Hunk_AllocName(CON_TEXTSIZE, "context");
	Q_memset(con_text, ' ', CON_TEXTSIZE);
	con_linewidth = -1;

	con_times = malloc(sizeof(float) * con_num_times);
	con_times_lines = malloc(sizeof(int) * con_num_times);
	if (!con_times || !con_times_lines)
		Sys_Error("Couldn't allocate space for %i console overlays.", con_num_times);

	Con_CheckResize();

//
// register our commands
//
	Cvar_RegisterVariable(&con_notifytime);

	Cmd_AddCommand("contimes", Con_SetTimes_f);
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("hideconsole", Con_HideConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	con_initialized = TRUE;
}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown( void )
{
	if (con_times)
		free(con_times);
	if (con_times_lines)
		free(con_times_lines);

	con_times = NULL;
	con_times_lines = NULL;

	con_initialized = FALSE;
}

/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed( void )
{
	con_x = 0;
	con_current++;
	Q_memset(&con_text[con_linewidth * (con_current % con_totallines)]
		, ' ', con_linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print( char* txt )
{
	int		y;
	int		c, l;
	static int	cr;
	qboolean	record;

	// only stamp notify times when the developer wants them or the line is
	// flagged talk text, so ordinary spew doesn't flash the overlay
	record = developer.value > 0;
	con_backscroll = 0;

	if (txt[0] == 1)
	{
		record = TRUE;
		txt++;
	}
	else if (txt[0] == 2)
	{
		txt++;
	}


	while ((c = *txt))
	{
		// count word length
		for (l = 0; l < con_linewidth; l++)
			if (txt[l] <= ' ')
				break;

		// word wrap
		if (l != con_linewidth && (con_x + l > con_linewidth))
			con_x = 0;

		txt++;

		if (cr)
		{
			con_current--;
			cr = FALSE;
		}


		if (!con_x)
		{
			Con_Linefeed();
			// mark time for transparent overlay
			if (record)
			{
				if (con_current >= 0)
					con_times[con_current % con_num_times] = realtime;
				con_times_lines[con_current % con_num_times] = con_current % con_totallines;
			}
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			break;

		case '\r':
			con_x = 0;
			cr = TRUE;
			break;

		default:	// display character and advance
			y = con_current % con_totallines;
			con_text[y * con_linewidth + con_x] = c;
			con_x++;
			if (con_x >= con_linewidth)
				con_x = 0;
			break;
		}

	}
}


/*
================
Con_DebugLog
================
*/
void Con_DebugLog( char* file, char* fmt, ... )
{
#ifndef  _WIN32_WCE
	va_list argptr;
	static char data[1024];
	int fd;

	va_start(argptr, fmt);
	vsprintf(data, fmt, argptr);
	va_end(argptr);

	fd = _open(file, _O_WRONLY | _O_CREAT | _O_APPEND, 438);
	_write(fd, data, strlen(data));
	_close(fd);
#endif
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
#define	MAXPRINTMSG	4096
extern char		outputbuf[8000];
#include "console.h"
qboolean 	g_fIsDebugPrint = FALSE;
// FIXME: make a buffer size safe vsprintf?
void Con_Printf( char* fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	static float	lastupdate;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	// Add to redirected message
	if (sv_redirected != RD_NONE)
	{
		if (strlen(outputbuf) + strlen(msg) > sizeof(outputbuf))
			Host_FlushRedirect();
		strcat(outputbuf, msg);
		return;
	}

	// log all messages to file
	if (con_debuglog)
		Con_DebugLog(va("%s/qconsole.log", com_gamedir), "%s", msg);

	if (con_initialized && cls.state != ca_dedicated)
	{
		if (!con_loading && !(giSubState & 4) || g_fIsDebugPrint)
		{
		// write it to the scrollable buffer
			Con_Print(msg);
		}

	// update the screen no more than a few times a second while the
	// console is displayed
		if (cls.signon != SIGNONS && !scr_disabled_for_loading)
		{
			float time = Sys_FloatTime();

			if (time - lastupdate > 0.3f)
			{
				lastupdate = time;

			// protect against infinite loop if something in SCR_UpdateScreen
			// calls Con_Printf
				if (!inupdate)
				{
					inupdate = TRUE;
					SCR_UpdateScreen();
					inupdate = FALSE;
				}
			}
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf( char* fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!developer.value)
		return;			// don't confuse non-developers with techie stuff...

	if (scr_con_current && cls.state == ca_active)
		return;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	g_fIsDebugPrint = TRUE;
	Con_Printf("%s", msg);
	g_fIsDebugPrint = FALSE;
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf( char* fmt, ... )
{
	va_list		argptr;
	char		msg[1024];
	int			temp;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = TRUE;
	g_fIsDebugPrint = TRUE;
	Con_Printf("%s", msg);
	g_fIsDebugPrint = FALSE;
	scr_disabled_for_loading = temp;
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput( void )
{
	int		x, y;
	int		i;
	char* text;

	if (key_dest != key_console && !con_forcedup)
		return;		// don't draw anything

	text = key_lines[edit_line];

// add the cursor frame
	text[key_linepos] = 10 + ((int)(realtime * con_cursorspeed) & 1);

// fill out remainder with spaces
	for (i = key_linepos + 1; i < con_linewidth; i++)
		text[i] = ' ';

//	prestep if horizontally scrolling
	if (key_linepos >= con_linewidth)
		text += 1 + key_linepos - con_linewidth;

// bottom-anchor the input line inside the overscan-safe area
	y = con_vislines;
	if (y > 480 - scr_safe_y)
		y = 480 - scr_safe_y;
	y -= Font_CharHeight(draw_chars) + 4;
	if (y < scr_safe_y + 4)
		y = scr_safe_y + 4;

	DCV_SetPackedColor(0xFFFF9000);

// draw it
	x = scr_safe_x + Font_StringWidth(draw_chars, "]");
	for (i = 0; i < con_linewidth; i++)
		x += Draw_Character(x, y, text[i]);

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void )
{
	int		x, v;
	char* text;
	int		i, charpos;
	float	time;
	extern char chat_buffer[];
	extern int chat_bufferlen;

	v = scr_safe_y + 4;

	for (i = con_current - con_num_times + 1; i <= con_current; i++)
	{
		if (i < 0)
			continue;
		time = con_times[i % con_num_times];
		if (time == 0)
			continue;
		time = realtime - time;
		if (time > con_notifytime.value)
			continue; // expired
		text = con_text + con_times_lines[i % con_num_times] * con_linewidth;

		clearnotify = 0;
		scr_copytop = 1;

		DCV_SetPackedColor(0xFFFF9000);

		x = scr_safe_x + 8;
		for (charpos = 0; charpos < con_linewidth; charpos++)
		{
			x += Draw_Character(x, v, text[charpos]);
		}

		v += Font_CharHeight(draw_chars);
	}

	if (key_dest == key_message)
	{
		clearnotify = 0;
		scr_copytop = 1;

		charpos = 0;

		if (64 < chat_bufferlen)
			charpos = chat_bufferlen - 64;

		x = scr_safe_x + 8;
		x = Draw_String(Draw_String(x, v, "say"), v, ": ");
		DCV_SetPackedColor(0xFFFF9000);
		while (chat_buffer[charpos])
		{
			x += Draw_Character(x, v, chat_buffer[charpos]);
			charpos++;
		}
		Draw_Character((charpos + 5) << 3, v, 10 + ((int)(realtime * con_cursorspeed) & 1));
		v += Font_CharHeight(draw_chars);
	}

	if (v > con_notifylines)
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole( int lines, qboolean drawinput )
{
	int				i, charpos, x, y;
	int				rows;
	char* text;
	int				j;

	if (lines <= 0)
		return;

// draw the background
	Draw_ConsoleBackground(lines);

	if ((giSubState & 4) || con_loading)
		return;

// draw the text
	con_vislines = lines;

	rows = (lines - (scr_safe_y + 4)) / Font_CharHeight(draw_chars) - 1;		// rows of text to draw
	con_rows = rows;

	// bottom-anchor the last row inside the overscan-safe area
	y = con_vislines;
	if (y > 480 - scr_safe_y)
		y = 480 - scr_safe_y;
	y -= Font_CharHeight(draw_chars) + 4;
	if (y < scr_safe_y + 4)
		y = scr_safe_y + 4;
	y -= Font_CharHeight(draw_chars) * rows;

	for (i = con_current - rows + 1; i <= con_current; i++, y += Font_CharHeight(draw_chars))
	{
		if (y <= scr_safe_y + 4)
			continue;						// above the safe area - clipped

		j = i - con_backscroll;
		if (j < 0)
			j = 0;
		text = con_text + (j % con_totallines) * con_linewidth;

		DCV_SetPackedColor(0xFFFF9000);

		x = scr_safe_x + 8;
		for (charpos = 0; charpos < con_linewidth; charpos++)
		{
			x += Draw_Character(x, y, text[charpos]);
		}
	}

// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput();
}



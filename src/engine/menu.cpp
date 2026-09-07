// menu.c -- the front-end menu pages
//
// A page is built on demand, drawn over the frame the renderer just finished
// and torn down again once it has run itself out. The engine only ever sees
// the open page through gpActiveMenu; everything else about it -- its items,
// its artwork and the passcodes it listens for -- stays in here.

#include "quakedef.h"
#include "ui.h"
#include "menu.h"
#include "dc_draw.h"
#include "text_draw.h"
#include "hud_handlers.h"
#include "../util/vmu.h"
#include <platutil.h>

#define JOY_AXIS_ACTIONS	5

extern "C" {
void GL_BindStage( int texnum, int stage );
void GL_DisableMultitexture( void );
void DCV_SetTextureClamp( void );
void DCV_SetColor( int r, int g, int b, int a );
void DCV_FlushIfLarge( void );
int  DCV_GetVertCount( void );
int  DCV_AddVertex( float x, float y, float z, float tu, float tv );
void DCV_AddPolyIndices( int base, int count );
qboolean IN_KeyboardActive( void );
qboolean IN_JoystickActive( void );
}

extern cvar_t joyshift1;


CMenu*	gpActiveMenu;		// page currently open, NULL if none
int		gfDrawMenu;			// draw the menu instead of the 3D view this frame

// Button sequences the pages listen for while they are up: one swaps the
// credits over to the other text, the other opens the hidden page.
int		gfCreditsCode;
int		gfSecretCode;
int		g_iCreditsCodePos;
int		g_iSecretCodePos;
int		g_iControlPreset;
int		g_iInvertPad;

#define MAX_MENU_SAVE_FILES	100
#define MAX_MENU_SAVE_SPACE	128
#define MAX_MENU_LOAD_COMMAND	128
#define MAX_MENU_CONTROL_KEYS	256
#define MAX_MENU_SAVE_LABEL	32
#define MAX_MENU_SAVE_COMMAND	32
#define MENU_LOAD_SLOT		0xe7
#define MENU_SAVE_SLOT		0xe8
#define MENU_OPTION_VALUE_X	404
#define MENU_SENSITIVITY_BAR_LEFT	260
#define MENU_SENSITIVITY_BAR_WIDTH	330
#define MENU_SENSITIVITY_SEGMENT_WIDTH	30
#define MENU_SENSITIVITY_MIDDLE_SEGMENTS	9
#define MENU_SENSITIVITY_THUMB_LEFT	284
#define MENU_SENSITIVITY_THUMB_STEP	15
#define MENU_SENSITIVITY_THUMB_WIDTH	12

typedef struct menusave_s
{
	char	*pszName;
	char	*pszDescription;
	int		character;
} menusave_t;

static menusave_t	g_MenuSaves[MAX_MENU_SAVE_FILES];
static int			g_nMenuSaves;
static char			g_szMenuSaveSpace[MAX_MENU_SAVE_SPACE];
static char			g_szMenuLoadCommand[MAX_MENU_LOAD_COMMAND];

static void M_InitSaveList( qboolean bSaving )
{
	int		i;

	g_nMenuSaves = 0;
	for (i = 0; i < MAX_MENU_SAVE_FILES; i++)
	{
		g_MenuSaves[i].pszDescription = NULL;
		g_MenuSaves[i].pszName = NULL;
		g_MenuSaves[i].character = 1;
	}

	if (bSaving)
	{
		g_MenuSaves[g_nMenuSaves].pszName = "________.___";
		g_MenuSaves[g_nMenuSaves].pszDescription = "%newsavedgame";
		g_MenuSaves[g_nMenuSaves].character = 1;
		g_nMenuSaves++;
	}
}

static int M_AddSaveFile( char* pszName, char* pszDescription, int character, void* pUserData )
{
	CMenuSaveSlotItem	*pSaveItem;

	pSaveItem = (CMenuSaveSlotItem *)pUserData;
	if (pSaveItem->m_noSpace)
	{
		if (pSaveItem->m_mode == MENU_SAVE_SLOT)
		{
			if (character == 16)
			{
				if (character == 2)
					goto add_file;
				if (character != 4)
					return 1;
			}
		}
	}

add_file:
	g_MenuSaves[g_nMenuSaves].pszName = pszName;
	g_MenuSaves[g_nMenuSaves].pszDescription = pszDescription;
	g_MenuSaves[g_nMenuSaves].character = character;
	g_nMenuSaves++;
	return 1;
}

static int M_BuildSaveFilename( qboolean bSaving, qboolean bNoSpace )
{
	int		slot;
	int		i;
	int		result;
	char	*text;

	slot = 0;
	result = 0;
	if (!bSaving)
	{
		if (!g_nMenuSaves)
		{
			g_MenuSaves[0].pszName = "________.___";
			g_MenuSaves[0].pszDescription = "%nofiles";
			g_MenuSaves[0].character = 1;
			result = 1;
		}
	}
	else if (!bNoSpace)
	{
		for (;;)
		{
			int	nextSlot;

			nextSlot = slot + 1;
			sprintf(g_MenuSaves[0].pszName, "HALFLIFE.%03d", slot);
			for (i = 1; i < g_nMenuSaves; i++)
			{
				if (!strcmp(g_MenuSaves[0].pszName, g_MenuSaves[i].pszName))
				{
					slot = nextSlot;
					break;
				}
			}

			if (i == g_nMenuSaves)
				break;
		}

		result = 1;
	}
	else
	{
		g_MenuSaves[0].pszName = "________.___";
		text = "%lowspace";
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(text, g_pLangTags[i].tag))
			{
				text = g_pLangTags[i].string;
				break;
			}
		}

		{
			char	*at;
			char	*output;

			at = strchr(text, '@');
			if (!at)
			{
				strcpy(g_szMenuSaveSpace, text);
			}
			else
			{
				output = g_szMenuSaveSpace;
				while (*text != '@')
				{
					*output++ = *text++;
				}

				sprintf(output, "%d",
					(Host_SaveGameSize() + 511) >> 9);
				strcat(output, text + 1);
			}
		}

		g_MenuSaves[0].pszDescription = g_szMenuSaveSpace;
		g_MenuSaves[0].character = 1;
		result = 1;
	}

	return result;
}

static float	g_flAccessCodeStartTime;
static float	g_flAccessCodeTime;
static byte	g_bAccessCodeResult;
static char	*g_pszAccessCodeResult;

static char	*g_pszAxisActions[JOY_AXIS_ACTIONS] =
{
	"%axis_none",
	"%axis_move",
	"%axis_look",
	"%axis_strafe",
	"%axis_turn"
};

static char	*g_pszShiftKeys[] =
{
	"JOY1", "JOY2", "JOY3", "JOY4", "AUX1",
	"AUX2", "AUX3", "AUX4", "AUX5", "AUX7"
};

#define NUM_SHIFT_KEYS	(sizeof(g_pszShiftKeys) / sizeof(g_pszShiftKeys[0]))
#define LAST_JOYSHIFT1	170

typedef struct controlaction_s
{
	char*	pszCommand;
	char*	pszLabel;
	char*	pszDescription;
} controlaction_t;

typedef struct controlkey_s
{
	short	control;
	short	key;
} controlkey_t;

controlaction_t g_ControlActions[] =
{
	{ "+forward", "%forwardshort", "%forwardlong" },
	{ "+back", "%backshort", "%backlong" },
	{ "+moveleft", "%leftshort", "%leftlong" },
	{ "+moveright", "%rightshort", "%rightlong" },
	{ "+moveup", "%moveupshort", "%moveuplong" },
	{ "+movedown", "%movedownshort", "%movedownlong" },
	{ "+lookup", "%lookupshort", "%lookuplong" },
	{ "+lookdown", "%lookdownshort", "%lookdownlong" },
	{ "+left", "%turnleftshort", "%turnleftlong" },
	{ "+right", "%turnrightshort", "%turnrightlong" },
	{ "+attack", "%attackshort", "%attacklong" },
	{ "+attack2", "%attack2short", "%attack2long" },
	{ "+use", "%useshort", "%uselong" },
	{ "impulse 100", "%flashlightshort", "%flashlightlong" },
	{ "+jump", "%jumpshort", "%jumplong" },
	{ "+duck", "%duckshort", "%ducklong" },
	{ "tduck", "%ducktoggleshort", "%ducktogglelong" },
	{ "+reload", "%reloadshort", "%reloadlong" },
	{ "invnext", "%invnextshort", "%invnextlong" },
	{ "invprev", "%invprevshort", "%invprevlong" },
	{ "lastinv", "%lastinvshort", "%lastinvlong" },
	{ "joyshift1", "%shift", "%shift" },
	{ "+strafe", "%strafeshiftshort", "%strafeshiftlong" },
	{ "+speed", "%speedshiftshort", "%speedshiftlong" },
	{ "tspeed", "%speedtoggleshort", "%speedtogglelong" },
	{ "force_centerview", "%centerviewshort", "%centerviewlong" },
	{ "+jlook", "%jlookshort", "%jlooklong" },
	{ "slot1", "%slot1short", "%slot1long" },
	{ "slot2", "%slot2short", "%slot2long" },
	{ "slot3", "%slot3short", "%slot3long" },
	{ "slot4", "%slot4short", "%slot4long" },
	{ "slot5", "%slot5short", "%slot5long" },
	{ NULL, NULL, NULL }
};

controlkey_t g_ControlKeys[MAX_MENU_CONTROL_KEYS];

static char *g_ControlKeyNames[][2] =
{
	{ "JOY4", "%pad_y" },
	{ "JOY1", "%pad_a" },
	{ "JOY3", "%pad_x" },
	{ "JOY2", "%pad_b" },
	{ "AUX5", "%trigger_right" },
	{ "AUX6", "%trigger_left" },
	{ "AUX4", "%dir_up" },
	{ "AUX3", "%dir_down" },
	{ "AUX2", "%dir_right" },
	{ "AUX1", "%dir_left" },
	{ "AUX7", "%start" },
	{ "S1AUX7", "%shiftstart" },
	{ "S1AUX6", "%shiftrigger_l" },
	{ "S1AUX5", "%shifttrigger_r" },
	{ "S1AUX4", "%shiftdir_up" },
	{ "S1AUX3", "%shiftdir_down" },
	{ "S1AUX2", "%shiftdir_right" },
	{ "S1AUX1", "%shiftdir_left" },
	{ "S1JOY4", "%shiftpad_y" },
	{ "S1JOY1", "%shiftpad_a" },
	{ "S1JOY3", "%shiftpad_x" },
	{ "S1JOY2", "%shiftpad_b" },
	{ "MOUSE1", "%mouse1" },
	{ "MOUSE2", "%mouse2" },
	{ "MOUSE3", "%mouse3" },
	{ "MWHEELDOWN", "%wheelup" },
	{ "MWHEELUP", "%wheeldown" },
	{ NULL, NULL }
};

static char *M_ControlKeyName( char *pszKey )
{
	int i;

	for (i = 0; g_ControlKeyNames[i][0]; i++)
	{
		if (!strcmp(g_ControlKeyNames[i][0], pszKey))
			return Text_FindString(g_ControlKeyNames[i][1]);
	}
	return pszKey;
}

/*
==================
Text_LookupAlias

Say what a button does. The key/command pairs come out of the preset script;
find the one this button is named in, look its command up in the action list
and hand back the wording for it. A button the script never mentions answers
with its own name.
==================
*/
char *Text_LookupAlias( char *pszKey, char **ppAliases, int *pnAliases, qboolean bLong )
{
	controlaction_t	*pAction;
	char			*psz;
	int				i;

	for (i = 0; i < *pnAliases; i++)
	{
		if (!strcmp(pszKey, ppAliases[i * 2]))
			break;
	}

	if (i == *pnAliases)
		return pszKey;

	for (pAction = g_ControlActions; pAction->pszCommand; pAction++)
	{
		if (!strcmp(ppAliases[i * 2 + 1], pAction->pszCommand))
			break;
	}

	if (!pAction->pszCommand)
		return pszKey;

	psz = bLong ? pAction->pszDescription : pAction->pszLabel;

	for (i = 0; i < g_nLangTags; i++)
	{
		if (!strcmp(psz, g_pLangTags[i].tag))
			return g_pLangTags[i].string;
	}

	return psz;
}

static int M_BuildControlList( qboolean bKeyboard, qboolean bJoystick )
{
	controlaction_t	*pAction;
	int				key;
	int				count;
	int				control;
	int				found;
	qboolean			add;

	count = 0;
	control = 0;
	if (!keybindings)
		return count;

	for (pAction = g_ControlActions; pAction->pszCommand; pAction++, control++)
	{
		if (!strcmp(pAction->pszCommand, "joyshift1"))
			continue;

		found = 0;
	for (key = 0; key < MAX_MENU_CONTROL_KEYS; key++)
		{
			if (!keybindings[key] || strcmp(pAction->pszCommand, keybindings[key]))
				continue;

			add = 0;
			if (bKeyboard)
			{
				if ((key < 160 || 192 < key)
					&& (key < K_MOUSE1 || K_MOUSE3 < key)
					&& (key < K_JOY1 || K_AUX32 < key)
					&& key != K_MWHEELUP && key != K_MWHEELDOWN)
					add = 1;
			}

			if (!add && bJoystick)
			{
				if ((K_MOUSE1 <= key && key <= K_MOUSE3)
					|| key == K_MWHEELUP || key == K_MWHEELDOWN)
					add = 1;
			}

			if (!add)
			{
				if ((160 <= key && key <= 192)
					|| (K_JOY1 <= key && key <= K_AUX32))
					add = 1;
			}

			if (add)
			{
			g_ControlKeys[count].control = control;
			g_ControlKeys[count].key = key;
			count++;
			found++;
			}
		}

		if (!found)
		{
			g_ControlKeys[count].control = control;
			g_ControlKeys[count].key = -1;
			count++;
		}
	}

	return count;
}

/*
==================
The pages and the items they are built from. A page names the items it wants;
each item carries the text it shows and the console command it runs.
==================
*/
menuitemdef_t g_MenuItems[] =
{
	{ 0x74, "%quit_trig", "menu confirm", "%quit_trig_des" },
	{ 0x75, "%quit_trig", "menu confirm2", "%quit_trig_des" },
	{ 0x66, "%newgame", "menu newgame", "%newgame_des" },
	{ 0x6d, "%loadgame", "menu load", "%loadgame_des" },
	{ 0x6e, "%loadgame", "menu load2", "%loadgame_des" },
	{ 0x70, "%savegame", "menu save", "%savegame_des" },
	{ 0x6c, "%options", "menu config", "%options_des" },
	{ 0xa0, "%credits", "menu credits", "%credits_des" },
	{ 0x71, "", "menu debug", "Jump to level and other debug features" },
	{ 0x6f, "%reload", "reload", "%reload_des" },
	{ 0x67, "%newhl", "menu newhl", "%newhl_des" },
	{ 0x68, "%newbs", "menu newblue", "%newbs_des" },
	{ 0x69, "%newhc", "startgame valve\nskill 1\nmap t0a0", "%newhc_des" },
	{ 0x76, "%easy", "startgame valve\nskill 1\nmap c0a0", "%easy_des" },
	{ 0x77, "%normal", "startgame valve\nskill 2\nmap c0a0", "%normal_des" },
	{ 0x78, "%hard", "startgame valve\nskill 3\nmap c0a0", "%hard_des" },
	{ 0x7a, "%easy", "startgame barney\nskill 1\nmap ba_tram1", "%easy_des" },
	{ 0x7b, "%normal", "startgame barney\nskill 2\nmap ba_tram1", "%normal_des" },
	{ 0x7c, "%hard", "startgame barney\nskill 3\nmap ba_tram1", "%hard_des" },
	{ 0x7d, "%easy", "skill 1\nloadskill\n", "%easy_des" },
	{ 0x7e, "%normal", "skill 2\nloadskill\n", "%normal_des" },
	{ 0x7f, "%hard", "skill 3\nloadskill\n", "%hard_des" },
	{ 0xa4, "%controls", "menu controls", "%controls_des" },
	{ 0xa1, "%audio", "menu audio", "%audio_des" },
	{ 0xa2, "%access", "menu access", "%access_des" },
	{ 0xa3, "%select", "menu activate", "%select_des" },
	{ 0xa9, "%preset", "menu preset", "%preset_des" },
	{ 0xaa, "%custom", "menu custom", "%custom_des" },
	{ 0xab, "%advanced", "menu advanced", "%advanced_des" },
	{ 0xa6, "", "", "" },
	{ 0xa7, "", "", "" },
	{ 0xa5, "%analogsense", "", "" },
	{ 0xa8, "%return", "menu config", "%return_des" },
	{ 0xb1, "%activate", "exec passcode_c1a0.cfg", "%level_c1a0" },
	{ 0xb2, "%activate", "exec passcode_c1a1.cfg", "%level_c1a1" },
	{ 0xb3, "%activate", "exec passcode_c1a2.cfg", "%level_c1a2" },
	{ 0xb4, "%activate", "exec passcode_c1a3.cfg", "%level_c1a3" },
	{ 0xb5, "%activate", "exec passcode_c1a4.cfg", "%level_c1a4" },
	{ 0xb6, "%activate", "exec passcode_c2a1.cfg", "%level_c2a1" },
	{ 0xb7, "%activate", "exec passcode_c2a2.cfg", "%level_c2a2" },
	{ 0xb8, "%activate", "exec passcode_c2a3.cfg", "%level_c2a3" },
	{ 0xb9, "%activate", "exec passcode_c2a4.cfg", "%level_c2a4" },
	{ 0xba, "%activate", "exec passcode_c2a4d.cfg", "%level_c2a4d" },
	{ 0xbb, "%activate", "exec passcode_c2a5.cfg", "%level_c2a5" },
	{ 0xbc, "%activate", "exec passcode_c3a1.cfg", "%level_c3a1" },
	{ 0xbd, "%activate", "exec passcode_c3a2.cfg", "%level_c3a2" },
	{ 0xbe, "%activate", "exec passcode_c4a1.cfg", "%level_c4a1" },
	{ 0xbf, "%activate", "exec passcode_c4a1a.cfg", "%level_c4a1a" },
	{ 0xc0, "%activate", "exec passcode_c4a2.cfg", "%level_c4a2" },
	{ 0xc1, "%activate", "exec passcode_c4a3.cfg", "%level_c4a3" },
	{ 0xc2, "%activate", "exec passcode_c5a1.cfg", "%level_c5a1" },
	{ 0xc3, "%activate", "exec passcode_ba_security1.cfg", "%level_ba_s1" },
	{ 0xc4, "%activate", "exec passcode_ba_canal1.cfg", "%level_ba_c1" },
	{ 0xc5, "%activate", "exec passcode_ba_yard1.cfg", "%level_ba_y1" },
	{ 0xc6, "%activate", "exec passcode_ba_xen1.cfg", "%level_ba_x1" },
	{ 0xc7, "%activate", "exec passcode_ba_power1.cfg", "%level_ba_p1" },
	{ 0xc8, "%activate", "exec passcode_ba_teleport2.cfg", "%level_ba_t2" },
	{ 0xac, "%yes", "disconnect\nmenu main", "%areyousure" },
	{ 0xad, "%no", "menu gamemenu", "%areyousure" },
	{ 0xae, "%no", "menu continuemenu", "%areyousure" },
	{ 0xaf, "New Game - Half-Life", "menu halflife", "Start a new game as Gordon Freeman" },
	{ 0x6a, "New Game - Blue Shift", "menu guard", "Start a new game as Barney Calhoun" },
	{ 0x6b, "Hazard Course", "map t0a0", "Learn to play Half-Life" },
	{ 0x72, "Cancel", "", "Return to game" },
	{ 0x79, "Return to main menu", "menu main", "Return to main menu" },
	{ 0x84, "Return to main menu", "menu main", "Nothing here yet" },
	{ 0x87, "Execute DEBUG.CFG", "exec debug.cfg", "Execute custom debug script" },
	{ 0x88, "Developer Mode/Texture Logs", "developer 2\nmenu debug", "Print info/create texture logs" },
	{ 0x89, "Export Dicts/Savegames", "exportdicts 1\nexportsaves 1\nmenu debug", "Export save games and HL1 dicts" },
	{ 0x8a, "Allow Cheats", "sv_cheats 1\nmenu debug", "Allows cheats like god, noclip, impulse 101" },
	{ 0x8b, "Map C0A0 (HL)", "map c0a0", "Black Mesa Inbound (intro)" },
	{ 0x8c, "Map C1A0 (HL)", "map c1a0", "Anomalous Materials (skip intro)" },
	{ 0x8d, "Map C1A1 (HL)", "map c1a1", "Jump to map C1A1" },
	{ 0x8e, "Map C1A2 (HL)", "map c1a2", "Office Complex" },
	{ 0x8f, "Map C2A1 (HL)", "map c2a1", "Power Up" },
	{ 0x90, "Map C2A2 (HL)", "map c2a2", "On A Rail" },
	{ 0x91, "Map C3A1 (HL)", "map c3a1", "Forget About Freeman!" },
	{ 0x92, "Map C3A2 (HL)", "map c3a2", "Jump to map C3A2" },
	{ 0x93, "Map C4A1 (HL)", "map c4a1", "Jump to map C4A1" },
	{ 0x94, "Map C4A2 (HL)", "map c4a2", "Jump to map C4A2" },
	{ 0x9b, "Console", "", "Activate Console" },
	{ 0x95, "Map BA_TRAM1 (BS)", "map ba_tram1", "Blue Shift (intro)" },
	{ 0x96, "Map BA_SECURITY2 (BS)", "map ba_security2", "Blue Shift (skip intro)" },
	{ 0x97, "Other Blue Shift Maps", "menu debugbarney", "Jump to other Blue Shift maps" },
	{ 0x98, "Other Half-Life Maps", "menu debugvalve", "Jump to other Half-Life maps" },
	{ 0x99, "\x7f\x80\x86\x87\x89\x8e\x90\x91\x96\x98\x9e\x9f\xb7", "", "Lookie!" },
	{ 0x9c, "RussMark", "developer 1\nmap c1a0", "Benchmark level loading time" },
	{ 0x9d, "Snoop", "developer 1\nsv_cheats 1\nbind i \"impulse 106\"\nmap c0a0\n", "Poke around c0a0" },
	{ 0x9a, "Cancel", "menu main", "Return to main menu" },
	{ 0x9e, "C\x98" "deZ", "c0dez\nmenu debug", "Activate all Access Codes" },
	{ 0x9f, "Dump VMU", "dumpvmu\nmenu debug", "Dump VMU data to dev system" },
	{ 0x73, "Resume", "", "Return to game or console" },
	{ 0x85, "Activate Half-Life", "startgame valve\nmenu debug", "Set game to Half-Life (valve)" },
	{ 0x86, "Activate Blue Shift", "startgame barney\nmenu debug", "Set game to Blue Shift (barney)" },
};

menupage_t g_MenuPages[] =
{
	{ "splash", "menu splash2", 1,
		{ 0xf2,  0xfc,  0xfe, } },
	{ "splash2", "", 1,
		{ 0xf3,  0xfd,  0xfe, } },
	{ "main", "", 1,
		{ 0x02,  0xf1,  0x66,  0x6d,  0x6c,  0xa0,  0x71,  0xfe, } },
	{ "gamemenu", "", 1,
		{ 0x02,  0xf1,  0x70,  0x6d,  0x6c,  0x74,  0xa7,  0xfe, } },
	{ "continuemenu", "", 1,
		{ 0x02,  0xf1,  0x6f,  0x6e,  0x75,  0xfe, } },
	{ "newgame", "menu main", 1,
		{ 0x02,  0xf1,  0xe2,  0xe3,  0x69,  0x67,  0x68,  0xa7,  0xfe, } },
	{ "load", "menu main", 1,
		{ 0x02,  0xf1,  0xe7,  0xe7,  0xe7,  0xe7,  0xa7,  0xfe, } },
	{ "load2", "menu continuemenu", 1,
		{ 0x02,  0xf1,  0xe7,  0xe7,  0xe7,  0xe7,  0xa7,  0xfe, } },
	{ "save", "menu gamemenu", 1,
		{ 0x02,  0xf1,  0xf7,  0xe8,  0xe8,  0xe8,  0xe8,  0xa7,  0xfe, } },
	{ "config", "menu main", 1,
		{ 0x02,  0xf1,  0xa4,  0xa1,  0xa2,  0xa3,  0xa7,  0xfe, } },
	{ "credits", "menu main", 1,
		{ 0x02,  0xf1,  0xf5,  0xa7,  0xfe, } },
	{ "newhl", "menu main", 1,
		{ 0x02,  0xf1,  0xe2,  0xe3,  0x76,  0x77,  0x78,  0xa7,  0xfe, } },
	{ "newblue", "menu main", 1,
		{ 0x02,  0xf1,  0xe2,  0xe3,  0x7a,  0x7b,  0x7c,  0xa7,  0xfe, } },
	{ "accessload", "menu activate", 1,
		{ 0x02,  0xf1,  0x7d,  0x7e,  0x7f,  0xa7,  0xfe, } },
	{ "controls", "menu config", 1,
		{ 0x02,  0xf1,  0xa9,  0xab,  0xaa,  0xa7,  0xfe, } },
	{ "audio", "menu config", 1,
		{ 0x02,  0xf1,  0xeb,  0xed,  0xec,  0xcd,  0xa7,  0xfe, } },
	{ "access", "menu config", 1,
		{ 0x02,  0xf1,  0xce,  0xcf,  0xd0,  0xa7,  0xfe, } },
	{ "activate", "menu config", 1,
		{ 0x02,  0xf1,  0xa7,  0xd5,  0xd6,  0xd7,  0xd8,  0xd9,  0xda,  0xdb,  0xdc,  0xdd,  0xdf,  0xe0,  0xb1,  0xb2,  0xb3,  0xb4,  0xb5,  0xb6,  0xb7,  0xb8,  0xb9,  0xba,  0xbb,  0xbc,  0xbd,  0xbe,  0xc0,  0xbf,  0xc1,  0xc2,  0xc3,  0xc4,  0xc5,  0xc6,  0xc7,  0xc8,  0xa8,  0xfe, } },
	{ "preset", "menu controls", 1,
		{ 0x02,  0xf1,  0xe4,  0xf8,  0xf9,  0xfa,  0xa6,  0xa7,  0xfe, } },
	{ "custom", "menu controls", 1,
		{ 0x02,  0xf1,  0xa6,  0xfb,  0xe4,  0xa7,  0xfe, } },
	{ "advanced", "menu controls", 1,
		{ 0x02,  0xf1,  0xca,  0xcb,  0xcc,  0xa5,  0xee,  0xef,  0xa7,  0xfe, } },
	{ "confirm", "menu gamemenu", 1,
		{ 0x02,  0xf1,  0xac,  0xad,  0xa7,  0xfe, } },
	{ "confirm2", "menu continuemenu", 1,
		{ 0x02,  0xf1,  0xac,  0xae,  0xa7,  0xfe, } },
	{ "debug", "menu main", 0,
		{ 0x02,  0xf1,  0x88,  0x89,  0x8a,  0x85,  0x86,  0x9b,  0x99,  0x9e,  0x9f,  0x9a,  0xfe, } },
	{ "debugbarney", "menu debug", 0,
		{ 0x02,  0xf1,  0x84,  0xfe, } },
	{ "debugvalve", "menu debug", 0,
		{ 0x02,  0xf1,  0x84,  0xfe, } },
	{ "oldmain", "", 1,
		{ 0x02,  0xf1,  0xaf,  0x6a,  0x6b,  0x6c,  0x6d,  0x71,  0x73,  0x74,  0xfe, } },
	{ "halflife", "menu oldmain", 1,
		{ 0x02,  0xf1,  0x76,  0x77,  0x78,  0x79,  0xfe, } },
	{ "guard", "menu oldmain", 1,
		{ 0x02,  0xf1,  0x7a,  0x7b,  0x7c,  0x79,  0xfe, } },
};

/*
==================
The passcodes. Each one is three controller buttons pressed in order on any
page; the cheats unlock a gameplay toggle, the level codes warp to a chapter.
The name is a language tag, looked up when the code is shown on the codes page.
==================
*/
menucode_t g_MenuCodes[] =
{
	{ 17, 11, 15, "%code_fly", FALSE, 0 },
	{ 18,  8, 16, "%code_loot3", FALSE, 0 },
	{ 10, 19, 19, "%code_medic", FALSE, 0 },
	{ 14, 14,  0, "%code_ghost", FALSE, 0 },
	{ 10, 11, 15, "%code_ammo", FALSE, 0 },
	{ 38, 15, 16, "%code_notarget", FALSE, 0 },
	{ 22, 19, 10, "%code_god", FALSE, 0 },
	{  0, 15, 37, "%code_slomo", FALSE, 0 },
	{ 16, 15, 12, "%code_loot1", FALSE, 0 },
	{ 16,  0, 17, "%code_lowgrav", FALSE, 0 },
	{ 10, 15, 14, "%code_loot2", FALSE, 0 },
	{ 16, 12, 43, "%level_c1a0", FALSE, 0 },
	{ 16, 21, 25, "%level_c1a1", FALSE, 0 },
	{ 14,  1, 13, "%level_c1a2", FALSE, 0 },
	{ 38, 22,  6, "%level_c1a3", FALSE, 0 },
	{ 34,  1, 39, "%level_c1a4", FALSE, 0 },
	{  5, 17,  4, "%level_c2a1", FALSE, 0 },
	{ 41,  1, 33, "%level_c2a2", FALSE, 0 },
	{ 13,  9, 16, "%level_c2a3", FALSE, 0 },
	{ 30, 20,  9, "%level_c2a4", FALSE, 0 },
	{ 32,  6, 27, "%level_c2a4d", FALSE, 0 },
	{  7,  1,  8, "%level_c2a5", FALSE, 0 },
	{ 38,  7, 40, "%level_c3a1", FALSE, 0 },
	{ 36,  2, 43, "%level_c3a2", FALSE, 0 },
	{ 13,  1, 17, "%level_c4a1", FALSE, 0 },
	{ 23, 16, 44, "%level_c4a1a", FALSE, 0 },
	{ 45,  1, 15, "%level_c4a2", FALSE, 0 },
	{ 23,  1,  9, "%level_c4a3", FALSE, 0 },
	{  2,  4, 28, "%level_c5a1", FALSE, 0 },
	{  3, 12, 43, "%level_ba_s1", FALSE, 0 },
	{ 31, 17, 35, "%level_ba_c1", FALSE, 0 },
	{  7,  1, 41, "%level_ba_y1", FALSE, 0 },
	{  3, 23, 44, "%level_ba_x1", FALSE, 0 },
	{ 26, 17, 27, "%level_ba_p1", FALSE, 0 },
	{ 25, 17,  4, "%level_ba_t2", FALSE, 0 },
	{ -1,  0,  0, NULL, FALSE, 0 },
};

/*
==================
The settings pages. Each option carries the values it steps through, which
one it starts on, and the help line it puts up while the stick is on it.
==================
*/
menuoption_t g_MenuOptions[] =
{
	{ 0xce,
		{
			"ACTIONS", "ALIENS", "ANSWERS", "BARNEY", "BEAUTIFUL", "BIG", "BLACK MESA", "COMBAT",
			"DEATH", "DIE", "DREAMCAST", "EVERYONE", "EXPLOSIVES", "FEAR", "FILES", "FIREPOWER",
			"GORDON", "GRAVITY", "HARD", "HEADCRABS", "KNOWLEDGE", "NEW MEXICO", "OTIS", "PANIC",
			"PIZZA", "PHYSICS", "POWER", "PROGRESS", "QUESTIONS", "QUIET", "RECYCLE", "RED",
			"REGRESSION", "ROCKETS", "SAFE", "SCARY", "SCIENTISTS", "SILENCE", "SOLDIERS", "SOUND",
			"TACOS", "TRAINS", "WALTER", "WORK", "XEN", "XENOPHOBIA"
		},
		46, 3, "%access_tog_des", NULL },
	{ 0xcf,
		{
			"ABHORS", "AND", "AT", "ATTACKS", "BEGET", "BREED", "BRINGS", "EAT", "EATS", "FINDS",
			"FEARS", "GIVES", "GOES TO", "HATES", "IF", "IGNORE", "IN", "IS", "IS NOT", "LOVES",
			"OR", "TEACHES", "VISIT", "VISITS"
		},
		24, 9, "%access_tog_des", NULL },
	{ 0xd0,
		{
			"ACTIONS", "ALIENS", "ANSWERS", "BARNEY", "BEAUTIFUL", "BIG", "BLACK MESA", "COMBAT",
			"DEATH", "DIE", "DREAMCAST", "EVERYONE", "EXPLOSIVES", "FEAR", "FILES", "FIREPOWER",
			"GORDON", "GRAVITY", "HARD", "HEADCRABS", "KNOWLEDGE", "NEW MEXICO", "OTIS", "PANIC",
			"PIZZA", "PHYSICS", "POWER", "PROGRESS", "QUESTIONS", "QUIET", "RECYCLE", "RED",
			"REGRESSION", "ROCKETS", "SAFE", "SCARY", "SCIENTISTS", "SILENCE", "SOLDIERS", "SOUND",
			"TACOS", "TRAINS", "WALTER", "WORK", "XEN", "XENOPHOBIA"
		},
		46, 44, "%access_tog_des", NULL },
	{ 0xd4,
		{
			"%off", "%on"
		},
		2, 0, "%code_console", NULL },
	{ 0xd5,
		{
			"%off", "%on"
		},
		2, 0, "%code_fly", Host_BindCheatFly },
	{ 0xd6,
		{
			"%off", "%on"
		},
		2, 0, "%code_loot3", Host_BindCheatWeapons },
	{ 0xd7,
		{
			"%off", "%on"
		},
		2, 0, "%code_medic", Host_BindCheatHealth },
	{ 0xd8,
		{
			"%off", "%on"
		},
		2, 0, "%code_ghost", Host_BindCheatNoclip },
	{ 0xd9,
		{
			"%off", "%on"
		},
		2, 0, "%code_ammo", Host_BindCheatAmmo },
	{ 0xda,
		{
			"%off", "%on"
		},
		2, 0, "%code_notarget", Host_BindCheatNotarget },
	{ 0xdb,
		{
			"%off", "%on"
		},
		2, 0, "%code_god", Host_BindCheatGod },
	{ 0xdc,
		{
			"%off", "%on"
		},
		2, 0, "%code_slomo", Host_BindCheatSlomo },
	{ 0xdd,
		{
			"%off", "%on"
		},
		2, 0, "%code_loot1", Host_BindCheatWeapons },
	{ 0xde,
		{
			"%off", "%on"
		},
		2, 0, "%code_lowgrav", Host_BindCheatGravity },
	{ 0xdf,
		{
			"%off", "%on"
		},
		2, 0, "%code_loot2", Host_BindCheatEverything },
	{ 0xe0,
		{
			"%off", "%on"
		},
		2, 0, "%code_xenonearth", Host_BindCheatAllies },
	{ 0xca,
		{
			"%no", "%yes"
		},
		2, 0, "%crosshair", NULL },
	{ 0xcb,
		{
			"%no", "%yes"
		},
		2, 1, "%invertpad", NULL },
	{ 0xcc,
		{
			"%no", "%yes"
		},
		2, 0, "%autoaim", NULL },
	{ 0xcd,
		{
			"%off", "%on"
		},
		2, 1, "%stereo", NULL },
	{ 0xe7,
		{
			"", ""
		},
		2, 0, "", NULL },
	{ 0xe8,
		{
			"", ""
		},
		2, 0, "", NULL },
};

menuslider_t g_MenuSliders[] =
{
	{ 0xeb, "%adjust_sfx", 10, 6, "%adjust_sfx_des" },
	{ 0xec, "%adjust_msc", 10, 5, "%adjust_msc_des" },
	{ 0xed, "%adjust_hev", 10, 5, "%adjust_hev_des" },
	{ 0xee, "%xsense", 19, 10, NULL },
	{ 0xef, "%ysense", 19, 6, NULL },
};

/*
==================
The credit roll. The tag character at the head of each line picks the style it
is drawn in; the second roll is what the button code on the credits page swaps
the first one for.
==================
*/
char* g_pszCredits[] =
{
	"$%gearboxcredits",
	"",
	"@%productiondirection",
	"&Randy Pitchford",
	"",
	"@%artdirection",
	"&Brian Martel",
	"",
	"@%artmodelsanimation",
	"&Stephen Bahl",
	"&Brian Martel",
	"&Landon Montgomery",
	"&Matthew VanDolen",
	"",
	"@%blueshiftdesign",
	"&Rob Heironimus",
	"&David Mertz",
	"",
	"@%blueshiftleveldesign",
	"&Matt Armstrong",
	"&Rob Heironimus",
	"&David Mertz",
	"&Randy Pitchford",
	"&Mike Wardwell",
	"",
	"@%programming",
	"&Sean Cavanaugh",
	"&Patrick Deupree",
	"&Steven Jones",
	"",
	"@%soundeffects",
	"&Rob Heironimus",
	"&Stephen Bahl",
	"",
	"@%writing",
	"&Rob Heironimus",
	"&David Mertz",
	"&Randy Pitchford",
	"",
	"@%manual",
	"&Kristy Junio",
	"&Eli Luna",
	"&Brian Martel",
	"&Randy Pitchford",
	"",
	"@%blueshiftvoices",
	"&Jon St. John",
	"&Kathy Levin",
	"&Harry S. Robins",
	"&Mike Shapiro",
	"",
	"@%administration",
	"&Stephen Bahl",
	"&Landon Montgomery",
	"",
	"@%specialthanks",
	"&Brian Hess",
	"&Steve Jones",
	"&Joe Kennebec",
	"&Stephen Palmer",
	"&Sean Reardon",
	"&Rob Selitto",
	"&Valve Software",
	"",
	"$%captivationcredits",
	"",
	"@%projectlead",
	"&Russell Bornschlegel",
	"",
	"@%programming",
	"&Russell Bornschlegel",
	"&Bob Hardy",
	"&Robert Morgan",
	"&David Sanner",
	"&Dan Windrem",
	"",
	"@%artmodelsanimation",
	"&Betty Cunningham",
	"&Brian Frederick",
	"&Arlin Robins",
	"&Dean Ruggles",
	"",
	"$%valvecredits",
	"",
	"@%marketingprojectmanager",
	"&Doug Lombardi",
	"",
	"$%additionalthanks",
	"",
	"&Gregory Lanz,",
	"&Sega of America",
	"",
	"$%halflifepublishedbysierra",
	"$%originalhalflifeforpc",
	"$%createdbyvalve",
	"",
	"$%sierrastudioscredits",
	"",
	"@%srvicepresident",
	"&J. Mark Hood",
	"",
	"@%producer",
	"&Jeff Pobst",
	"",
	"@%assistantproducer",
	"&Bernadette Pryor",
	"",
	"@%vpmarketing",
	"&Jim Veevaert",
	"",
	"@%directormarketing",
	"&Koren Buckner",
	"",
	"@%webeditor",
	"&Guy Welch",
	"",
	"@%srprmanager",
	"&Genevieve Ostergard",
	"",
	"@%creativeservices",
	"&Mike Rodgers",
	"&Orlena Yeung",
	"",
	"@%qamanager",
	"&Gary Stevens",
	"",
	"@%qasupervisor",
	"&Ken Eaton",
	"",
	"@%qaleadtester",
	"&Marc Nagel",
	"",
	"@%qatesters",
	"&Niko Simonson",
	"&Danny Harrison",
	"&Jon Pulling",
	"&Stephen Musch",
	"",
	"@%packagingartwork",
	"&Blue Spark Studios",
	"",
	"$%pchalflifecredits",
	"",
	"@%valvesoftwareis",
	"",
	"&Ted Backman",
	"&T.K. Backman",
	"&Kelly Bailey",
	"&Yahn Bernier",
	"&Ken Birdwell",
	"&Steve Bond",
	"&Dario Casali",
	"&John Cook",
	"&Greg Coomer",
	"&Wes Cumberland",
	"&John Guthrie",
	"&Mona Lisa Guthrie",
	"&Mike Harrington",
	"&Monica Harrington",
	"&Brett Johnson",
	"&Chuck Jones",
	"&Marc Laidlaw",
	"&Karen Laur",
	"&Randy Lundeen",
	"&Yatzse Mark",
	"&Lisa Mennet",
	"&Gabe Newell",
	"&Dave Riller",
	"&Aaron Stackpole",
	"&Jay Stelly",
	"&Harry Teasley",
	"&Stephen Theodore",
	"&Bill Van Buren",
	"&Robin Walker",
	"&Douglas R. Wood",
	"",
	"$%morepchalflifecredits",
	"",
	"@Sierra Studios",
	"",
	"&Scott Lynch - Senior Vice President",
	"&Jim Veevaert - Director of Marketing",
	"&Doug Lombardi - Product Manager",
	"&Genevieve Ostergard - PR Manager",
	"&Justin Kriby - Creative Services",
	"&Gary Stevens - Product Testing Manager",
	"&Cade Myers - Lead Tester",
	"&Erik Johnson - Assistant Lead Tester",
	"&Andrew Coward - Tester",
	"&Dave Lee - Tester",
	"&Julie Bazuzi- Tester",
	"&Kate Powell - Tester",
	"&Ken Eaton - Tester",
	"&Matt Eslick - Tester",
	"&Miene Lee - Tester",
	"&Phil Kuhlmey - Tester",
	"",
	"@%voices",
	"&Kathy Levin",
	"&Harry S. Robins",
	"&Mike Shapiro",
	"",
	"!%withthanksto",
	"!Chris Bokitch",
	"!Heather Mitchell",
	"!Dan Saimo",
	"!Ray Ueno",
	"!Ian Caughley",
	"!Eric Twelker",
	"!Christina Kelly",
	"!Nathan Dwyer",
	"!Joe Bryant",
	"!Stephen Hecht",
	"!Stephen Dennis",
	"!Steve Fleugel",
	"!Les Betterly",
	"!Russell Ginns",
	"!Ben Morris",
	"!Duncan",
	"!Karl Deckard",
	"!Louise Donaldson",
	"!Dhabih Eng",
	"!Robert Stanlee",
	"!Eddie Ranchigoda",
	"!Koren Buckner",
	"!Michael Abrash",
	"!%everyoneatidsoftware",
	"!and Joe Kennebec and",
	"!%allotherbetatesters",
	"",
	"",
	"",
	"",
	"",
	"",
	"$%inmemoryofbetty",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	NULL
};

char* g_pszCredits2[] =
{
	"$SHOUT-OUTS",
	"",
	"$THANKS FOR THE PATIENCE AND SUPPORT",
	"&Erin, Rachel, Meriko and",
	"&all our friends and families",
	"",
	"$THANKS FOR THE LAFFS",
	"&Gabe & Tycho - It's like a tiny god!",
	"&Scott Kurtz - Mmmmm, Segalicious!",
	"&Christina Z - Leet is as leet does...",
	"&Michelle Z - Is that a *snark*? -screech-",
	"&Tatsuya Ishida - Listen, you're very vulnerable right now.",
	"&Jonathan Rosenberg - Dammit, Scooter ran away again.",
	"",
	"$THANKS FOR THE NEWS",
	"&Redwood and the Stomped Crew",
	"&Blue & Co.",
	"&Slashdot",
	"&Memepool",
	"&Games Xtreme for first billing",
	"",
	"$THANKS FOR THE REST OF IT",
	"&Chris \"Bishop Squarepeg Roundhole\" Pressey",
	"&and all the Esolang Folks",
	"&El Granada Ace Hardware for all the coffee",
	"",
	"$THANKS FOR NOTHING",
	"&Fatbabies - \"Who dropped the ball over at Sierra?\"",
	"&You know that hole you put pie into? SHUT IT",
	NULL
};

// Where the access page has got to in each of the three word lists
int		g_iAccessNoun1;
int		g_iAccessNoun2;
int		g_iAccessVerb;

void CMenuItemBase::Destroy( void )
{
}

void CMenuItemBase::Draw( float flFade, qboolean bSelected )
{
}

void CMenuItemBase::Select( void )
{
}

void CMenuItemBase::Cancel( void )
{
}

void CMenuItemBase::Up( void )
{
}

void CMenuItemBase::Down( void )
{
}

void CMenuItemBase::Left( void )
{
}

void CMenuItemBase::Right( void )
{
}

void CMenuItemBase::SetPos( float x, float y )
{
}

int CMenuItemBase::IsActive( void )
{
	return 0;
}

/*
==================
CMenuTextItem::CMenuTextItem
==================
*/
CMenuTextItem::CMenuTextItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y, int align )
	: CMenuItem(pMenu, pDef, x, y)
{
	m_align = align;
}

void CMenuTextItem::Destroy( void )
{
}

void CMenuTextItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
	int		width;
	int		advance;
	int		ch;
	float	x1, x2;
	float	y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		psz = m_pszLabel;
		if (psz && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX * (float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		if (m_align == 0)
			DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		else
			DCV_SetColor(95, 95, 255, (int)(flFade * 120.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture = M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_labelX - 15);
		x2 = (float)(m_labelX + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_TexState_Blend();
		DCV_SetHudDepth(4.0f);
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (m_pszLabel)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszLabel, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (bSelected && m_pszDescription)
	{
		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect, (byte *)m_pszDescription,
			(int)(flFade * 192.0f));
	}

	g_nTextCharGap = 0;
}

/*
==================
CMenuTextItem::Select

Picking the item hands its command to the page and starts the page fading
away; the command runs once the fade is done. Items that are not fading run
their command straight away.
==================
*/
void CMenuTextItem::Select( void )
{
	CMenu*	pMenu;
	char*	pszCommand;

	pMenu = m_pMenu;
	pszCommand = m_pszCommand;

	if (!m_bEnabled)
	{
		gfDrawMenu = 0;

		// keep the sound blocked while the command is only going to bring up
		// another page
		if (strstr(pszCommand, "menu"))
			pMenu->m_state.iSoundBlocked = 1;
		else
			pMenu->m_state.iSoundBlocked = 0;

		Cbuf_AddText(pszCommand);
		Cbuf_AddText("\n");
	}
	else
	{
		strcpy(pMenu->m_szCommand, pszCommand);
		M_FadeOut(pMenu);
	}
}

/*
==================
CMenuTextItem::Cancel

Backing out runs whatever the page says it goes back to.
==================
*/
void CMenuTextItem::Cancel( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;

	gfDrawMenu = 0;

	if (strstr(pMenu->m_state.pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

/*
==================
CMenuTextItem::Up

Walk the selection back to the item before this one that the stick can
actually land on, wrapping round the page and giving up where it started.
==================
*/
void CMenuTextItem::Up( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

/*
==================
CMenuTextItem::Down
==================
*/
void CMenuTextItem::Down( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

int CMenuTextItem::IsActive( void )
{
	return 1;
}

void CMenuStaticItem::Destroy( void )
{
}

int CMenuStaticItem::IsActive( void )
{
	return 0;
}


/*
==================
The rest of the items a page can be built out of. Each one knows how to draw
itself and what the stick does to it; the page only ever sees the base.
==================
*/
CMenuTitleItem::CMenuTitleItem( CMenu* pMenu )
{
	m_pMenu = pMenu;

	m_widthPeriod[0] = 1.3f;
	m_widthPeriod[1] = 2.3f;
	m_xPeriod[0] = 2.1f;
	m_xPeriod[1] = 1.5f;
	m_alphaPeriod[0] = 2.1f;
	m_alphaPeriod[1] = 1.1f;

	m_widthPulse[0] = 0.0f;
	m_widthPulse[1] = 0.0f;
	m_width[0] = 700.0f;
	m_width[1] = 512.0f;
	m_xPulse[0] = 30.0f;
	m_xPulse[1] = 15.0f;
	m_x[0] = -30.0f;
	m_x[1] = 64.0f;
	m_y[0] = 14.0f;
	m_y[1] = 16.0f;
	m_height[0] = 100.0f;
	m_height[1] = 64.0f;
	m_alphaPulse[0] = 0.0f;
	m_alphaPulse[1] = 0.0f;
	m_alpha[0] = 0.55f;
	m_alpha[1] = 1.0f;

	m_texture[0] = M_LoadMenuTexture(pMenu, "gfx/menu_title.pvr");
	m_texture[1] = M_LoadMenuTexture(pMenu, "gfx/menu_title.pvr");
}

void CMenuTitleItem::Draw( float flFade, qboolean bSelected )
{
	float	width, x, y, alpha, time;
	float	vbase, v0, v1;
	int		i;

	DCV_SetHudDepth(4.0f);
	DCV_TexState_Additive();
	GL_DisableMultitexture();
	DCV_SetTextureClamp();

	for (i = 0; i < 2; i++)
	{
		GL_BindStage(m_texture[i], 0);

		time = m_pMenu->m_state.flTime;
		width = sins(time / m_widthPeriod[i]) * m_widthPulse[i] + m_width[i];
		x = sins(time / m_xPeriod[i]) * m_xPulse[i] + m_x[i];
		alpha = flFade * (sins(time / m_alphaPeriod[i]) * m_alphaPulse[i] + m_alpha[i]) * 255.0f;

		if (alpha < 0.0f)
			alpha = 0.0f;
		if (alpha > 255.0f)
			alpha = 255.0f;

		DCV_SetColor((int)alpha, (int)alpha, (int)alpha, 255);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		// the artwork holds the title twice, the soft copy that spreads out
		// behind the letters sitting under the sharp one
		vbase = (i == 0) ? 0.5f : 0.0f;
		v0 = vbase + 0.005f;
		v1 = vbase + 0.495f;

		y = m_y[i] + (float)scr_safe_y;
		DCV_AddVertex(x, y, dc_depthhud.value, 0.0f, v0);
		DCV_AddVertex(x + width, y, dc_depthhud.value, 1.0f, v0);
		DCV_AddVertex(x, y + m_height[i], dc_depthhud.value, 0.0f, v1);
		DCV_AddVertex(x + width, y + m_height[i], dc_depthhud.value, 1.0f, v1);
	}
}

int CMenuTitleItem::IsActive( void )
{
	return 0;
}

void CMenuHintItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
	int		ch;
	int		advance;
	int		width;

	DCV_TexState_Blend();

	if (!m_pszLabel)
		return;

	g_flTextScaleX = 0.7f;
	g_flTextScaleY = 0.93331f;

	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX = 0.581f;
		g_flTextScaleY = 0.774647f;
	}

	Text_DrawStringShadow("", 0, 0, (int)(flFade * 190.0f), 0);

	switch (g_iControlPreset)
	{
	case 0:
		m_pszLabel = "%preset_a";
		break;

	case 1:
		m_pszLabel = "%preset_b";
		break;

	case 2:
		m_pszLabel = "%preset_c";
		break;

	case 3:
		m_pszLabel = "%customconfig";
		break;
	}

	psz = m_pszLabel;
	width = 0;

	if (psz)
	{
		while (*psz)
		{
			ch = (byte)*psz++;
			if (ch >= 192)
				ch -= 64;

			advance = (int)(g_flTextScaleX * (float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
			if (advance < 2)
				advance = 2;
			if (advance > 40)
				advance = 40;
			width += advance;
		}
	}

	g_flTextScaleX = m_flLabelScale * 0.7f;
	g_flTextScaleY = m_flLabelAspect * 0.7f;

	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX *= 0.83f;
		g_flTextScaleY *= 0.83f;
	}

	Text_DrawStringShadow(m_pszLabel, m_labelX - (int)((float)width * 0.5f), m_labelY,
		(int)(flFade * 190.0f), 0);
}

void CMenuReturnItem::Draw( float flFade, qboolean bSelected )
{
	DCV_TexState_Blend();

	if (!m_pszLabel)
		return;

	g_flTextScaleX = m_flLabelScale * 0.6f;
	g_flTextScaleY = m_flLabelAspect * 0.6f;

	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX *= 0.83f;
		g_flTextScaleY *= 0.83f;
	}

	Text_DrawStringShadow("%bback", m_labelX, m_labelY, (int)(flFade * 190.0f), 0);
}

void CMenuCodeTextItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
	int		ch;
	int		advance;
	int		width;
	int		count;
	float	x1, x2, y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		psz = m_pszLabel;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX * (float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture = M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_labelX + 350 - 15);
		x2 = (float)(m_labelX + 350 + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));

		count = 0;
		for (ch = 0; ch < MAX_MENU_ITEMS; ch++)
		{
			if (m_pMenu->m_pItems[ch])
				count++;
		}

		if (count - 3 > 7)
		{
			if (m_pMenu->m_state.iTopItem > 0)
				M_DrawMenuElementQuad(m_pMenu, 3, 278.0f, 118.0f, 362.0f, 146.0f);

			if (m_pMenu->m_state.iTopItem + 7 < count - 3)
				M_DrawMenuElementQuad(m_pMenu, 4, 278.0f, 432.0f, 362.0f, 460.0f);
		}
	}

	if (m_pszLabel)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszLabel, m_labelX + 350, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (m_pszDescription)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszDescription, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

	g_nTextCharGap = 0;
}

void CMenuCodeTextItem::Up( void )
{
	CMenu*	pMenu;
	int		count;
	int		start;
	int		i;

	pMenu = m_pMenu;
	count = 0;

	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			count++;
	}

	if (pMenu->m_state.iSelectedRow > 0)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iSelectedRow - 1 + count - 3) % (count - 3)) < pMenu->m_state.iTopItem)
		{
			pMenu->m_state.iTopItem--;
		}

		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected--;
			if (pMenu->m_state.iSelected < 0)
				pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);
	}
}

void CMenuCodeTextItem::Down( void )
{
	CMenu*	pMenu;
	int		count;
	int		visible;
	int		start;
	int		i;

	pMenu = m_pMenu;
	count = 0;

	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			count++;
	}

	visible = 0;
	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			visible++;
	}

	if (pMenu->m_state.iSelectedRow + 1 < visible - 3)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem + 6) % (count - 3)) == pMenu->m_state.iSelectedRow)
		{
			pMenu->m_state.iTopItem++;
		}

		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected++;
			if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
				pMenu->m_state.iSelected = 0;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);
	}
}

void CMenuCodeTextItem::SetPos( float x, float y )
{
	m_labelX = (int)x;
	m_labelY = (int)y;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;
}

void CMenuIconItem::Draw( float flFade, qboolean bSelected )
{
	int		base;
	int		texture;
	float	x0, y0, x1, y1;
	int		selected;

	DCV_SetHudDepth(2.0f);
	DCV_SetColor(255, 255, 255, (int)(flFade * 128.0f));
	DCV_TexState_Blend();

	x0 = (float)m_x;
	y0 = (float)m_y;

	switch (m_id)
	{
	case 0xe2:
		selected = m_pMenu->m_state.iSelected;
		if (!strcmp(m_pMenu->m_szName, "newblue"))
			selected = 6;
		else if (!strcmp(m_pMenu->m_szName, "newhl"))
			selected = 4;

		if (selected == 4 || selected == 5)
		{
			if (m_alpha < 214)
				m_alpha += 3;
		}
		else if (m_alpha > 90)
			m_alpha -= 3;

		DCV_SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		x1 = x0 + 256.0f;
		y1 = y0 + 256.0f;
		texture = M_LoadMenuTexture(m_pMenu, "gfx/menu_gordon.pvr");
		m_pMenu->m_state.iGordonTexture = texture;
		GL_BindStage(texture, 0);
		break;

	case 0xe3:
		selected = m_pMenu->m_state.iSelected;
		if (!strcmp(m_pMenu->m_szName, "newblue"))
			selected = 6;
		else if (!strcmp(m_pMenu->m_szName, "newhl"))
			selected = 4;

		if (selected == 6)
		{
			if (m_alpha < 214)
				m_alpha += 3;
		}
		else if (m_alpha > 90)
			m_alpha -= 3;

		DCV_SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		x1 = x0 + 256.0f;
		y1 = y0 + 256.0f;
		texture = M_LoadMenuTexture(m_pMenu, "gfx/menu_barney.pvr");
		m_pMenu->m_state.iBarneyTexture = texture;
		GL_BindStage(texture, 0);
		break;

	case 0xe4:
		DCV_SetColor(255, 255, 255, (int)(flFade * 255.0f));
		M_DrawControllerIcon(m_pMenu, x0, y0, x0 + 64.0f, y0 + 64.0f);
		DCV_SetHudDepth(3.0f);
		DCV_SetColor(255, 255, 255, (int)(flFade * 220.0f));
		M_DrawMenuElementBox(m_pMenu, x0 + 31.0f, y0 + 34.0f, x0 + 33.0f, y0 + 60.0f);
		return;

	case 0xe5:
		DCV_SetColor(255, 255, 255, (int)(flFade * 255.0f));
		x1 = x0 + 32.0f;
		y1 = y0 + 64.0f;
		texture = M_LoadMenuTexture(m_pMenu, "gfx/menu_vmu.pvr");
		m_pMenu->m_state.iVMUTexture = texture;
		GL_BindStage(texture, 0);
		break;

	default:
		return;
	}

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, 0.0f, 0.0f);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1.0f, 0.0f);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0.0f, 1.0f);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1.0f, 1.0f);
}

int CMenuIconItem::IsActive( void )
{
	return 0;
}

/*
==================
CMenuPicItem::CMenuPicItem

In game the console background is already resident, so the page borrows it
instead of pulling another copy off the disc.
==================
*/
__forceinline CMenuPicItem::CMenuPicItem( CMenu* pMenu, char* pszName )
{
	m_pMenu = pMenu;

	if (cls.state == ca_active)
		m_iTexture = *(int *)conback->data;
	else
		m_iTexture = M_LoadMenuTexture(m_pMenu, pszName);
}

/*
==================
CMenuPicItem::Draw

The page's backdrop, one screen-filling quad that fades with the page.
==================
*/
void CMenuPicItem::Draw( float flFade, qboolean bSelected )
{
	int		base;

	DCV_SetHudDepth(1.0f);
	DCV_TexState_Opaque();
	GL_DisableMultitexture();
	DCV_SetTextureClamp();

	DCV_SetColor((int)(flFade * 255.0f), (int)(flFade * 255.0f),
		(int)(flFade * 255.0f), 255);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	GL_BindStage(m_iTexture, 0);

	DCV_AddVertex(0, 0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(639.0f, 0, dc_depthhud.value, 1.0f, 0);
	DCV_AddVertex(0, 479.0f, dc_depthhud.value, 0, 1.0f);
	DCV_AddVertex(639.0f, 479.0f, dc_depthhud.value, 1.0f, 1.0f);
}

__forceinline CMenuCreditsItem::CMenuCreditsItem( CMenu* pMenu )
{
	m_pMenu = pMenu;
	m_flStartTime = pMenu->m_state.flTime;
	m_bRestart = 1;
}

/*
==================
M_DrawMenuText

Draw one line of the credit roll centred on the screen. The tag character at
the head of the line picks the style it is drawn in, and every line fades out
as it leaves the middle band of the screen.
==================
*/
void M_DrawMenuText( CMenuItemBase* pItem, char* psz, float y, float flFade )
{
	char	tag;
	float	alpha;
	float	fade;
	float	scale;
	float	x;
	int		i;

	tag = *psz;
	if (tag < 'A')
		psz++;

	alpha = flFade * (1.2f - (float)fabs(y - 290.0f) / 100.0f);
	if (alpha < 0)
		return;

	fade = alpha;
	if (fade > 1.0f)
		fade = 1.0f;

	switch (tag)
	{
	case '$':
		// a heading
		DCV_SetHudDepth(4.0f);
		DCV_TexState_Additive();

		scale = 1.0f;
		if (sv_language.value != 0.0f)
			scale = 0.83f;

		g_flTextScaleX = scale;
		g_flTextScaleY = scale;

		DCV_SetColor((int)(fade * 255.0f), (int)(fade * 144.0f), (int)(fade * 0.0f), 255);

		if (psz != 0 && *psz == '%' && g_nLangTags > 0)
		{
			for (i = 0; i < g_nLangTags; i++)
			{
				if (!strcmp(psz, g_pLangTags[i].tag))
				{
					psz = g_pLangTags[i].string;
					break;
				}
			}
		}

		x = 320.0f - (float)Font_StringWidth((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
		while (*psz)
			x += Font_DrawChar(x, y, (dcfont_t *)draw_chars, *psz++);

		return;

	case '@':
		// a sub-heading, in the same colours as a heading
		DCV_SetHudDepth(4.0f);
		DCV_TexState_Additive();

		scale = 1.0f;
		if (sv_language.value != 0.0f)
			scale = 0.83f;

		g_flTextScaleX = scale;
		g_flTextScaleY = scale;

		DCV_SetColor((int)(fade * 255.0f), (int)(fade * 144.0f), (int)(fade * 0.0f), 255);

		if (psz != 0 && *psz == '%' && g_nLangTags > 0)
		{
			for (i = 0; i < g_nLangTags; i++)
			{
				if (!strcmp(psz, g_pLangTags[i].tag))
				{
					psz = g_pLangTags[i].string;
					break;
				}
			}
		}

		x = 320.0f - (float)Font_StringWidth((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
		while (*psz)
			x += Font_DrawChar(x, y, (dcfont_t *)draw_chars, *psz++);

		return;

	case '!':
	case '&':
	default:
		break;
	}

	// a plain line
	DCV_SetHudDepth(4.0f);
	DCV_TexState_Additive();

	g_flTextScaleX = 0.66f;
	g_flTextScaleY = 1.0f;

	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX = 0.5478f;
		g_flTextScaleY = 0.83f;
	}

	DCV_SetColor((int)(fade * 255.0f), (int)(fade * 255.0f), (int)(fade * 255.0f), 255);

	if (psz != 0 && *psz == '%' && g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(psz, g_pLangTags[i].tag))
			{
				psz = g_pLangTags[i].string;
				break;
			}
		}
	}

	x = 320.0f - (float)Font_StringWidth((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
	while (*psz)
		x += Font_DrawChar(x, y, (dcfont_t *)draw_chars, *psz++);
}

/*
==================
CMenuCreditsItem::Draw

Walk the roll up the screen from where it stood when the page came up. Once
the last line has climbed past the top the page runs whatever it goes back to.
==================
*/
void CMenuCreditsItem::Draw( float flFade, qboolean bSelected )
{
	char**	ppszLine;
	float	y;

	if (m_bRestart)
	{
		m_flStartTime = m_pMenu->m_state.flTime;
		m_bRestart = 0;
	}

	y = (m_pMenu->m_state.flTime - m_flStartTime) * -42.0f + 400.0f;

	ppszLine = g_pszCredits;
	if (gfCreditsCode)
		ppszLine = g_pszCredits2;

	for ( ; *ppszLine; ppszLine++)
	{
		if (**ppszLine && y > 0 && y < 480.0f)
			M_DrawMenuText(this, *ppszLine, y, flFade);

		y += 35.0f;
	}

	if (y < 80.0f)
	{
		gfCreditsCode = 0;
		gfDrawMenu = 0;

		if (strstr(m_pMenu->m_state.pszCommand, "menu"))
			m_pMenu->m_state.iSoundBlocked = 1;
		else
			m_pMenu->m_state.iSoundBlocked = 0;

		Cbuf_AddText(m_pMenu->m_state.pszCommand);
		Cbuf_AddText("\n");
	}
}

void CMenuCreditsItem::Cancel( void )
{
	CMenu*	pMenu;
	char*	pszCommand;

	pMenu = m_pMenu;
	pszCommand = pMenu->m_state.pszCommand;

	gfCreditsCode = 0;
	gfDrawMenu = 0;

	if (strstr(pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(pszCommand);
	Cbuf_AddText("\n");
}

int CMenuCreditsItem::IsActive( void )
{
	return 1;
}

/*
==================
CMenuAttractItem::CMenuAttractItem

The splash screen gives the player five seconds before the attract mode
takes the machine back.
==================
*/
__forceinline CMenuAttractItem::CMenuAttractItem( CMenu* pMenu )
{
	m_pMenu = pMenu;
	m_flTimeout = pMenu->m_state.flTime + 5.0f;
	m_bTaken = 0;
}

/*
==================
CMenuAttractItem::Draw

Nothing is drawn: this is the splash screen's timer. Once it runs out the
page hands itself over to the attract mode, and pressing anything does the
same thing early.
==================
*/
void CMenuAttractItem::Draw( float flFade, qboolean bSelected )
{
	if (m_flTimeout < m_pMenu->m_state.flTime && !m_bTaken)
	{
		m_bTaken = 1;

		strcpy(m_pMenu->m_szCommand, "menu splash2");
		M_FadeOut(m_pMenu);
	}
}

void CMenuAttractItem::Select( void )
{
	strcpy(m_pMenu->m_szCommand, "menu splash2");
	M_FadeOut(m_pMenu);
}

void CMenuAttractItem::Cancel( void )
{
	strcpy(m_pMenu->m_szCommand, "menu splash2");
	M_FadeOut(m_pMenu);
}

void CMenuAttractItem::Up( void )
{
}

void CMenuAttractItem::Down( void )
{
}

void CMenuAttractItem::Left( void )
{
}

void CMenuAttractItem::Right( void )
{
}

int CMenuAttractItem::IsActive( void )
{
	return 1;
}

/*
==================
CMenuSaveHeaderItem::CMenuSaveHeaderItem

Putting the save page up writes the slot the player is about to overwrite,
so the list has something to show for it.
==================
*/
__forceinline CMenuSaveHeaderItem::CMenuSaveHeaderItem( CMenu* pMenu )
{
	m_pMenu = pMenu;
	Cbuf_AddText("save fake\nsav fake\n");
}

void CMenuSaveHeaderItem::Draw( float flFade, qboolean bSelected )
{
}

void CMenuSaveHeaderItem::Select( void )
{
}

void CMenuSaveHeaderItem::Cancel( void )
{
}

void CMenuSaveHeaderItem::Up( void )
{
}

void CMenuSaveHeaderItem::Down( void )
{
}

void CMenuSaveHeaderItem::Left( void )
{
}

void CMenuSaveHeaderItem::Right( void )
{
}

int CMenuSaveHeaderItem::IsActive( void )
{
	return 0;
}

__forceinline CMenuAnyKeyItem::CMenuAnyKeyItem( CMenu* pMenu )
{
	m_pMenu = pMenu;
}

void CMenuAnyKeyItem::Draw( float flFade, qboolean bSelected )
{
	byte	*psz;
	byte	*p;
	float	flScaleX;
	float	flScaleY;
	float	flBrightness;
	int		width;
	int		advance;
	int		ch;
	int		i;

	DCV_SetHudDepth(3.0f);
	DCV_TexState_Blend();

	psz = (byte *)"%copyright";
	if (g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp((char *)psz, g_pLangTags[i].tag))
			{
				psz = (byte *)g_pLangTags[i].string;
				break;
			}
		}
	}

	Font_FitScale(0.7f, 0.93331f, (float)(620 - scr_safe_x * 2), psz, &flScaleX, &flScaleY);

	width = 0;
	if (psz)
	{
		p = psz;
		while (*p)
		{
			ch = *p++;
			if (ch >= 192)
				ch -= 64;

			advance = (int)((float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth * g_flTextScaleX + 1.4f);
			if (advance < 2)
				advance = 2;
			if (advance > 40)
				advance = 40;
			width += advance;
		}
	}

	Text_DrawString(flScaleX, flScaleY, (char *)psz,
		(int)(320.0f - (float)width / 2.0f), 436 - scr_safe_y,
		(int)(flFade * 255.0f), (int)(flFade * 255.0f), (int)(flFade * 255.0f));

	g_flTextScaleX = 1.0f;
	g_flTextScaleY = 1.3333f;
	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX = 0.83f;
		g_flTextScaleY = 1.10664f;
	}

	flBrightness = (coss(m_pMenu->m_state.flTime * 4.23f) + 1.0f) * 127.0f;
	psz = (byte *)"%pressstart";
	if (g_nLangTags > 0)
	{
		for (i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp((char *)psz, g_pLangTags[i].tag))
			{
				psz = (byte *)g_pLangTags[i].string;
				break;
			}
		}
	}

	width = 0;
	if (psz)
	{
		p = psz;
		while (*p)
		{
			ch = *p++;
			if (ch >= 192)
				ch -= 64;

			advance = (int)((float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth * g_flTextScaleX + 1.4f);
			if (advance < 2)
				advance = 2;
			if (advance > 40)
				advance = 40;
			width += advance;
		}
	}

	Text_DrawString(1.0f, 1.3333f, (char *)psz,
		(int)(320.0f - (float)width / 2.0f), 386 - scr_safe_y,
		(int)(flBrightness * flFade), (int)(flBrightness * flFade), (int)(flBrightness * flFade));
}

/*
==================
CMenuAnyKeyItem::Select

Any button on the splash screen goes to the main menu.
==================
*/
void CMenuAnyKeyItem::Select( void )
{
	strcpy(m_pMenu->m_szCommand, "menu main");
	M_FadeOut(m_pMenu);
}

void CMenuAnyKeyItem::Cancel( void )
{
}

void CMenuAnyKeyItem::Up( void )
{
}

void CMenuAnyKeyItem::Down( void )
{
}

void CMenuAnyKeyItem::Left( void )
{
}

void CMenuAnyKeyItem::Right( void )
{
}

int CMenuAnyKeyItem::IsActive( void )
{
	return 1;
}

/*
==================
CMenuOptionItem::CMenuOptionItem

Lay the option's values out once, so drawing only has to pick the one the
option is currently on.
==================
*/
CMenuOptionItem::CMenuOptionItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue )
{
	int		i;

	m_pMenu = pMenu;

	m_piIndex = &pOption->iValue;
	m_nValues = pOption->nValues;

	m_pValues = new menuvalue_t[m_nValues];

	for (i = 0; i < m_nValues; i++)
	{
		m_pValues[i].pszText = pOption->pszValues[i];
		m_pValues[i].flScale = 1.0f;
		m_pValues[i].flAspect = 1.3333f;
		m_pValues[i].x = x;
		m_pValues[i].y = y;
	}

	m_pszDescription = pOption->pszDescription;
	m_flDescScale = 1.0f;
	m_flDescAspect = 1.3333f;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;

	m_piValue = piValue;
	if (piValue)
		*piValue = *m_piIndex;
}

void CMenuOptionItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	float		flAlpha;

	DCV_TexState_Blend();

	pValue = &m_pValues[*m_piIndex];
	flAlpha = bSelected ? 255.0f : 128.0f;

	if (sv_language.value)
	{
		g_flTextScaleX = pValue->flScale * 0.83f;
		g_flTextScaleY = pValue->flAspect * 0.83f;
	}
	else
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;
	}

	Text_DrawStringShadow(pValue->pszText, pValue->x, pValue->y,
		(int)(flFade * flAlpha), 0);

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect,
			(byte *)m_pszDescription, (int)(flFade * 192.0f));
}

void CMenuOptionItem::Select( void )
{
	(*m_piIndex)++;
	*m_piIndex %= m_nValues;

	if (m_piValue)
		*m_piValue = *m_piIndex;
}

void CMenuOptionItem::Cancel( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	gfDrawMenu = 0;

	if (strstr(pMenu->m_state.pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuOptionItem::Up( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuOptionItem::Down( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuOptionItem::Left( void )
{
	*m_piIndex += m_nValues - 1;
	*m_piIndex %= m_nValues;

	if (m_piValue)
		*m_piValue = *m_piIndex;
}

void CMenuOptionItem::Right( void )
{
	(*m_piIndex)++;
	*m_piIndex %= m_nValues;

	if (m_piValue)
		*m_piValue = *m_piIndex;
}

int CMenuOptionItem::IsActive( void )
{
	return 1;
}

__forceinline CMenuStereoItem::CMenuStereoItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue )
	: CMenuOptionItem(pMenu, pOption, x, y, piValue)
{
	*m_piIndex = (int)Cvar_VariableValue("stereo");
}

void CMenuStereoItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	char		*psz;
	int			ch;
	int			advance;
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();
	pValue = &m_pValues[*m_piIndex];

	if (bSelected)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		psz = m_pszDescription;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(pValue->x - 15);
		x2 = (float)(pValue->x + width + 15);
		y1 = (float)(pValue->y - 14);
		y2 = (float)(pValue->y + 40);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	DCV_SetHudDepth(2.0f);
	DCV_TexState_Blend();
	DCV_SetColor(255, 144, 0, 200);

	m_pMenu->m_state.iElementTexture =
		M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_pMenu->m_state.iElementTexture, 0);
	DCV_FlushIfLarge();
	DCV_AddPolyIndices(DCV_GetVertCount(), 4);

	x1 = (float)(pValue->x + 290);
	x2 = x1 + 30.0f;
	y1 = (float)pValue->y;
	y2 = y1 + 30.0f;
	DCV_AddVertex(x1, y1, dc_depthhud.value, 0.552f, 0.568f);
	DCV_AddVertex(x2, y1, dc_depthhud.value, 0.943f, 0.568f);
	DCV_AddVertex(x1, y2, dc_depthhud.value, 0.552f, 0.946f);
	DCV_AddVertex(x2, y2, dc_depthhud.value, 0.943f, 0.946f);

	if (*m_piIndex)
	{
		DCV_TexState_Additive();
		DCV_SetColor(255, 144, 0, 255);
		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(pValue->x + 295);
		x2 = x1 + 20.0f;
		y1 = (float)(pValue->y + 5);
		y2 = y1 + 20.0f;
		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);
		DCV_TexState_Blend();
	}

	if (m_pszDescription)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;
		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}
		Text_DrawStringShadow(m_pszDescription, pValue->x, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (pValue->pszText)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;
		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}
		Text_DrawStringShadow(pValue->pszText, pValue->x + 340, pValue->y,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

	if (bSelected)
		Text_DrawCenteredStatus(pValue->flScale, pValue->flAspect,
			(byte *)"%stereo_des", (int)(flFade * 192.0f));
}

void CMenuStereoItem::Select( void )
{
	CMenuStereoItem::Right();
}

void CMenuStereoItem::Left( void )
{
	CMenuOptionItem::Left();
	Cvar_SetValue("stereo", (float)*m_piIndex);
	SetFirmwareValues(DWORD_DONT_CHANGE, BYTE_DONT_CHANGE, BYTE_DONT_CHANGE,
		stereo.value > 0.0f ? SOUND_MODE_STEREO : SOUND_MODE_MONO, BYTE_DONT_CHANGE);
}

void CMenuStereoItem::Right( void )
{
	CMenuOptionItem::Right();
	Cvar_SetValue("stereo", (float)*m_piIndex);
	SetFirmwareValues(DWORD_DONT_CHANGE, BYTE_DONT_CHANGE, BYTE_DONT_CHANGE,
		stereo.value > 0.0f ? SOUND_MODE_STEREO : SOUND_MODE_MONO, BYTE_DONT_CHANGE);
}

void CMenuCheatItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	char		*psz;
	int			ch;
	int			advance;
	int			width;
	int			count;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	pValue = &m_pValues[*m_piIndex];
	if (bSelected)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		psz = pValue->pszText;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(pValue->x + 350 - 15);
		x2 = (float)(pValue->x + 350 + width + 15);
		y1 = (float)(pValue->y - 10);
		y2 = (float)(pValue->y + 36);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));

		count = 0;
		for (ch = 0; ch < MAX_MENU_ITEMS; ch++)
		{
			if (m_pMenu->m_pItems[ch])
				count++;
		}

		if (count - 3 > 7)
		{
			if (m_pMenu->m_state.iTopItem > 0)
				M_DrawMenuElementQuad(m_pMenu, 3, 278.0f, 118.0f, 362.0f, 146.0f);

			count = 0;
			for (ch = 0; ch < MAX_MENU_ITEMS; ch++)
			{
				if (m_pMenu->m_pItems[ch])
					count++;
			}

			if (m_pMenu->m_state.iTopItem + 7 < count - 3)
				M_DrawMenuElementQuad(m_pMenu, 4, 278.0f, 432.0f, 362.0f, 460.0f);
		}
	}

	if (pValue->pszText)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(pValue->pszText, pValue->x + 350, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (m_pszDescription)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszDescription, pValue->x, pValue->y,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

	g_nTextCharGap = 0;
}

void CMenuCheatItem::Select( void )
{
	CMenuCheatItem::Right();
}

void CMenuCheatItem::Up( void )
{
	CMenu*	pMenu;
	int		count;
	int		start;
	int		i;

	pMenu = m_pMenu;
	count = 0;

	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			count++;
	}

	if (pMenu->m_state.iTopItem > 0)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem - 1 + count - 3) % (count - 3)) < pMenu->m_state.iTopItem)
		{
			pMenu->m_state.iTopItem--;
		}

		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected--;
			if (pMenu->m_state.iSelected < 0)
				pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);
	}
}

void CMenuCheatItem::Down( void )
{
	CMenu*	pMenu;
	int		count;
	int		visible;
	int		start;
	int		i;

	pMenu = m_pMenu;
	count = 0;

	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			count++;
	}

	visible = 0;
	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (pMenu->m_pItems[i])
			visible++;
	}

	if (pMenu->m_state.iSelectedRow + 1 < visible - 3)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem + 6) % (count - 3)) == pMenu->m_state.iSelectedRow)
		{
			pMenu->m_state.iTopItem++;
		}

		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected++;
			if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
				pMenu->m_state.iSelected = 0;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);
	}
}

void CMenuCheatItem::Left( void )
{
	CMenuOptionItem::Left();

	if (m_pfnBind)
		m_pfnBind(m_piIndex);
}

void CMenuCheatItem::Right( void )
{
	CMenuOptionItem::Right();

	if (m_pfnBind)
		m_pfnBind(m_piIndex);
}

/*
==================
CMenuToggleItem::CMenuToggleItem

The yes/no settings start out wherever the cvar they control left them.
==================
*/
CMenuToggleItem::CMenuToggleItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int id )
	: CMenuOptionItem(pMenu, pOption, x, y, NULL)
{
	m_id = id;

	if (m_id == 0xcb)
		*m_piIndex = (Cvar_VariableValue("joypitchsensitivity") < 0);

	if (m_id == 0xca)
		*m_piIndex = (int)Cvar_VariableValue("crosshair");

	if (m_id == 0xcc)
	{
		if (Cvar_VariableValue("sv_aim") > 0.5f)
			*m_piIndex = 1;
		else
			*m_piIndex = 0;
	}
}

void CMenuToggleItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	char		*psz;
	int			ch;
	int			advance;
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	pValue = &m_pValues[*m_piIndex];
	if (bSelected)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		psz = pValue->pszText;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(MENU_OPTION_VALUE_X - 15);
		x2 = (float)(MENU_OPTION_VALUE_X + width + 15);
		y1 = (float)(pValue->y - 14);
		y2 = (float)(pValue->y + 40);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (pValue->pszText)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(pValue->pszText, MENU_OPTION_VALUE_X, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (m_pszDescription)
	{
		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszDescription, pValue->x, pValue->y,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}
}

void CMenuToggleItem::Select( void )
{
	float	flPitch;
	char	sign;
	char	command[MAX_MENU_COMMAND_TEXT];

	(*m_piIndex)++;
	*m_piIndex %= m_nValues;

	if (m_id == 0xcb)
	{
		sign = *m_piIndex ? '-' : ' ';
		flPitch = Cvar_VariableValue("joypitchsensitivity");
		if (flPitch < 0.0f)
			flPitch = -flPitch;
		sprintf(command, "joypitchsensitivity %c%f\njoyadvancedupdate", sign, flPitch);
		g_iInvertPad = strstr(command, "-") != NULL;
		Cbuf_AddText(command);
		Cbuf_AddText("\n");
	}
	else if (m_id == 0xca)
		Cvar_SetValue("crosshair", (float)*m_piIndex);
	else if (m_id == 0xcc)
		Cvar_SetValue("sv_aim", (float)*m_piIndex * 0.9f);
}

void CMenuToggleItem::Cancel( void )
{
	CMenu	*pMenu;
	char	command[MAX_MENU_COMMAND_TEXT];

	sprintf(command, "joyadvancedupdate");
	pMenu = m_pMenu;
	pMenu->m_state.iSoundBlocked = strstr(command, "menu") != NULL;
	Cbuf_AddText(command);
	Cbuf_AddText("\n");
	Host_WriteConfiguration();

	gfDrawMenu = 0;
	pMenu->m_state.iSoundBlocked = strstr(pMenu->m_state.pszCommand, "menu") != NULL;
	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuToggleItem::Left( void )
{
	float	flPitch;
	char	sign;
	char	command[MAX_MENU_COMMAND_TEXT];

	CMenuOptionItem::Left();

	if (m_id == 0xcb)
	{
		sign = *m_piIndex ? '-' : ' ';
		flPitch = Cvar_VariableValue("joypitchsensitivity");
		if (flPitch < 0.0f)
			flPitch = -flPitch;
		sprintf(command, "joypitchsensitivity %c%f\njoyadvancedupdate", sign, flPitch);
		Cbuf_AddText(command);
		Cbuf_AddText("\n");
	}
	else if (m_id == 0xca)
		Cvar_SetValue("crosshair", (float)*m_piIndex);
	else if (m_id == 0xcc)
		Cvar_SetValue("sv_aim", (float)*m_piIndex * 0.9f);
}

void CMenuToggleItem::Right( void )
{
	float	flPitch;
	char	sign;
	char	command[MAX_MENU_COMMAND_TEXT];

	CMenuOptionItem::Right();

	if (m_id == 0xcb)
	{
		sign = *m_piIndex ? '-' : ' ';
		flPitch = Cvar_VariableValue("joypitchsensitivity");
		if (flPitch < 0.0f)
			flPitch = -flPitch;
		sprintf(command, "joypitchsensitivity %c%f\njoyadvancedupdate", sign, flPitch);
		Cbuf_AddText(command);
		Cbuf_AddText("\n");
	}
	else if (m_id == 0xca)
		Cvar_SetValue("crosshair", (float)*m_piIndex);
	else if (m_id == 0xcc)
		Cvar_SetValue("sv_aim", (float)*m_piIndex * 0.9f);
}

/*
==================
CMenuWordItem::CMenuWordItem

One of the three words the access code is spelled out of.
==================
*/
CMenuWordItem::CMenuWordItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue )
{
	int		i;

	m_pMenu = pMenu;

	g_flAccessCodeStartTime = 0.0f;
	g_flAccessCodeTime = 0.0f;
	g_bAccessCodeResult = 0;
	g_pszAccessCodeResult = NULL;

	m_nValues = pOption->nValues;
	m_pValues = new menuvalue_t[m_nValues];

	for (i = 0; i < m_nValues; i++)
	{
		m_pValues[i].pszText = pOption->pszValues[i];
		m_pValues[i].flScale = 1.0f;
		m_pValues[i].flAspect = 1.3333f;
		m_pValues[i].x = x;
		m_pValues[i].y = y;
	}

	m_pszDescription = pOption->pszDescription;
	m_flDescScale = 1.0f;
	m_flDescAspect = 1.3333f;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;

	m_piIndex = &pOption->iValue;
	m_piValue = piValue;
	if (piValue)
		*piValue = *m_piIndex;
}

void CMenuWordItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	char		*psz;
	int			index;
	int			offset;
	int			ch;
	int			advance;
	int			width;
	int			brightness;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();
	DCV_SetHudDepth(2.0f);
	DCV_SetColor(255, 144, 0, (int)(flFade * 128.0f));

	if (g_bAccessCodeResult)
	{
		g_flAccessCodeTime = 1.9f -
			(m_pMenu->m_state.flTime - g_flAccessCodeStartTime);
		if (g_flAccessCodeTime <= 0.0f)
		{
			g_flAccessCodeTime = 0.0f;
			g_bAccessCodeResult = 0;
		}
	}

	if (bSelected)
	{
		pValue = &m_pValues[*m_piIndex];
		M_DrawMenuElementQuad(m_pMenu, 3,
			(float)(pValue->x - 35), 118.0f,
			(float)(pValue->x + 35), 148.0f);
		M_DrawMenuElementQuad(m_pMenu, 4,
			(float)(pValue->x - 35), 388.0f,
			(float)(pValue->x + 35), 418.0f);
	}

	pValue = &m_pValues[*m_piIndex];
	DCV_SetColor(255, 144, 0, (int)(flFade * (bSelected ? 120.0f : 80.0f)));
	if (m_piValue == &g_iAccessVerb)
		M_DrawMenuElementBox(m_pMenu, 304.0f, 152.0f, 432.0f, 384.0f);
	else if (m_piValue == &g_iAccessNoun1 || m_piValue == &g_iAccessNoun2)
		M_DrawMenuElementBox(m_pMenu,
			(float)(pValue->x - 87), 152.0f,
			(float)(pValue->x + 87), 384.0f);

	for (offset = -2; offset <= 2; offset++)
	{
		index = (*m_piIndex + offset + m_nValues) % m_nValues;
		pValue = &m_pValues[index];

		g_flTextScaleX = pValue->flScale;
		g_flTextScaleY = pValue->flAspect;
		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		width = 0;
		psz = pValue->pszText;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		if (bSelected)
			brightness = (offset == 0) ? (int)(flFade * 255.0f) : (int)(flFade * 180.0f);
		else
			brightness = (offset == 0) ? (int)(flFade * 205.0f) : (int)(flFade * 128.0f);

		if (bSelected && offset == 0)
		{
			DCV_SetColor(255, 144, 0, (int)(flFade * 80.0f));
			DCV_SetHudDepth(2.0f);
			DCV_TexState_Additive();

			m_pMenu->m_state.iElementTexture =
				M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
			GL_BindStage(m_pMenu->m_state.iElementTexture, 0);
			DCV_FlushIfLarge();
			DCV_AddPolyIndices(DCV_GetVertCount(), 4);

			x1 = pValue->x - (width / 2) * 1.2f;
			x2 = pValue->x + (width / 2) * 1.2f;
			y1 = (float)(pValue->y - 10);
			y2 = (float)(pValue->y + 36);
			DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
			DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
			DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
			DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

			DCV_SetHudDepth(4.0f);
			DCV_TexState_Blend();
			DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
		}

		Text_DrawStringShadow(pValue->pszText, pValue->x - width / 2,
			pValue->y + offset * 40, brightness, 0);
	}

	if (m_piValue == &g_iAccessNoun1)
	{
		if (g_bAccessCodeResult && g_pszAccessCodeResult)
		{
			brightness = (int)(g_flAccessCodeTime * 192.0f);
			if (brightness > 192)
				brightness = 192;
			Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect,
				(byte *)g_pszAccessCodeResult, brightness);
		}
		else if (m_pszDescription)
			Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect,
				(byte *)m_pszDescription, (int)(flFade * 192.0f));
	}
}

void CMenuWordItem::Select( void )
{
	menucode_t	*code;

	for (code = g_MenuCodes; code->button1 != -1; code++)
	{
		if (code->button1 == g_iAccessNoun1
			&& code->button2 == g_iAccessVerb
			&& code->button3 == g_iAccessNoun2)
			break;
	}

	g_bAccessCodeResult = 1;
	g_flAccessCodeStartTime = m_pMenu->m_state.flTime;
	g_pszAccessCodeResult = code->pszName;

	if (code->button1 != -1)
	{
		code->enabled = TRUE;
		Cbuf_AddText("sv_cheats 1\n");
		allow_cheats = TRUE;
	}
}

void CMenuWordItem::Cancel( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	gfDrawMenu = 0;

	if (strstr(pMenu->m_state.pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuWordItem::Up( void )
{
	*m_piIndex += m_nValues - 1;
	*m_piIndex %= m_nValues;

	if (m_piValue)
		*m_piValue = *m_piIndex;
}

void CMenuWordItem::Down( void )
{
	(*m_piIndex)++;
	*m_piIndex %= m_nValues;

	if (m_piValue)
		*m_piValue = *m_piIndex;
}

void CMenuWordItem::Left( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuWordItem::Right( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

int CMenuWordItem::IsActive( void )
{
	return 1;
}

/*
==================
CMenuSaveSlotItem::CMenuSaveSlotItem
==================
*/
CMenuSaveSlotItem::CMenuSaveSlotItem( CMenu* pMenu, menuoption_t* pOption, int x, int iSlot, int id )
	: CMenuOptionItem(pMenu, pOption, x, 320, NULL)
{
	m_slot = iSlot;
	m_loaded = 0;
	m_visibleRow = 0;
	m_scrollTop = 0;
	m_selectedFile = 0;
	m_fileCount = 0;
	m_scanPending = 0;
	m_savePending = 0;
	m_frame = 0;
	m_noSpace = 0;
	m_saved = 0;
	m_mode = id;
	m_ready = 0;
}

void CMenuSaveSlotItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	int			alpha;
	int			dimAlpha;
	int			i;
	int			y;
	int			iconY;
	int			device;
	int			deviceAlpha[2];
	qboolean		anyDevice;
	char			label[MAX_MENU_SAVE_LABEL];
	char			*text;
	char			*status;
	float			textScaleX;
	float			textScaleY;

	DCV_TexState_Blend();

	if (m_scanPending)
	{
		m_frame++;
		if (m_frame == 6)
		{
			m_scanPending = 0;
			m_frame = 0;
		}
		if (m_frame == 4 && (m_mode == MENU_SAVE_SLOT || m_mode == MENU_LOAD_SLOT))
		{
			m_noSpace = VMU_GetFreeBlocks() < Host_SaveGameSize();
			M_InitSaveList(m_mode == MENU_SAVE_SLOT);
			VMU_SelectDeviceIfPresent(m_slot * 2 + *m_piIndex);
			VMU_EnumFiles((vmuenumproc_t)M_AddSaveFile, this);
			M_BuildSaveFilename(m_mode == MENU_SAVE_SLOT, m_noSpace);

			m_fileCount = 0;
			for (i = 0; i < MAX_MENU_SAVE_FILES; i++)
			{
				if (g_MenuSaves[i].pszDescription)
					m_fileCount++;
			}
		}
	}

	if (m_savePending && !m_ready)
	{
		m_frame++;
		if (m_frame == 3)
		{
			char command[MAX_MENU_SAVE_COMMAND];
			GDROM_SetDoorBehavior();
			sprintf(command, "save %s\n", g_MenuSaves[m_selectedFile].pszName);
			Cbuf_AddText(command);
		}

		if (m_frame == 6)
		{
			m_savePending = 0;
			m_saved = 1;
			m_loaded = 0;
		}
	}

	anyDevice = 0;
	for (i = 0; i < 8; i++)
	{
		if (VMU_IsDevicePresent(i))
		{
			anyDevice = 1;
			break;
		}
	}

	alpha = (int)(flFade * (bSelected ? 255.0f : 128.0f));
	if (!anyDevice)
	{
		if (bSelected)
		{
			char message[128];
			char *output = message;
			for (i = 0; i < 3; i++)
			{
				sprintf(label, m_mode == MENU_LOAD_SLOT ? "%%novmuload%d" : "%%novmusave%d", i + 1);
				text = Text_FindString(label);
				if (m_mode != MENU_LOAD_SLOT)
				{
					if (strchr(text, '@'))
					{
						while (*text != '@')
							*output++ = *text++;
						sprintf(output, "%d", (Host_SaveGameSize() + 511) / 512);
						strcat(output, text + 1);
					}
					else
						strcpy(message, text);
					text = message;
				}
				y = 320 - Font_StringWidth((dcfont_t *)draw_chars, (byte *)text) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(text, y, 176 + i * 40, alpha, 0);
			}
		}
		return;
	}

	pValue = &m_pValues[*m_piIndex];
	alpha = bSelected ? (int)(flFade * 255.0f) : (int)(flFade * 128.0f);
	dimAlpha = (int)(flFade * 128.0f);

	deviceAlpha[0] = deviceAlpha[1] = alpha;
	if (bSelected)
	{
		deviceAlpha[*m_piIndex] = (int)(alpha * flFade);
		deviceAlpha[1 - *m_piIndex] = (int)(alpha * flFade * 0.5f);
	}

	for (device = 0; device < 2; device++)
	{
		DCV_SetHudDepth(2.0f);
		DCV_SetColor(255, 144, 0,
			deviceAlpha[device]);
		M_DrawMenuElementBox(m_pMenu, (float)(pValue->x - 20),
			200.0f + (float)(device * 64), (float)(pValue->x + 20),
			260.0f + (float)(device * 64));

		if (!VMU_IsDevicePresent(m_slot * 2 + device))
			continue;

		DCV_SetHudDepth(4.0f);
		DCV_SetColor(255, 255, 255, deviceAlpha[device]);
		m_pMenu->m_state.iVMUTexture = M_LoadMenuTexture(m_pMenu, "gfx/menu_vmu.pvr");
		GL_BindStage(m_pMenu->m_state.iVMUTexture, 0);
		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);
		DCV_AddVertex((float)(pValue->x - 12), 207.0f + (float)(device * 64), dc_depthhud.value, 0.0f, 0.0f);
		DCV_AddVertex((float)(pValue->x + 12), 207.0f + (float)(device * 64), dc_depthhud.value, 1.0f, 0.0f);
		DCV_AddVertex((float)(pValue->x - 12), 255.0f + (float)(device * 64), dc_depthhud.value, 0.0f, 1.0f);
		DCV_AddVertex((float)(pValue->x + 12), 255.0f + (float)(device * 64), dc_depthhud.value, 1.0f, 1.0f);
	}

	DCV_SetHudDepth(4.0f);
	DCV_SetColor(255, 255, 255, (int)(alpha * flFade));
	M_DrawControllerIcon(m_pMenu, (float)(pValue->x - 32), 130.0f,
		(float)(pValue->x + 32), 194.0f);

	if (bSelected)
	{
		if (VMU_IsDevicePresent(m_slot * 2 + *m_piIndex))
		{
			text = Text_FindString("%port");
			sprintf(label, "%s %c", text, m_slot + 'A');
			textScaleX = sv_language.value ? 0.66399997f : 0.8f;
			textScaleY = sv_language.value ? 0.88531119f : 1.06664f;
			g_flTextScaleX = textScaleX;
			g_flTextScaleY = textScaleY;
			Text_DrawStringShadow(label, 54,
				cls.state == ca_active ? 160 : 176, alpha, 0);

			text = Text_FindString("%slot");
			sprintf(label, "%s %c", text, *m_piIndex + '1');
			Text_DrawStringShadow(label, cls.state == ca_active ? 54 : 62,
				cls.state == ca_active ? 200 : 216, alpha, 0);
		}

		DCV_SetHudDepth(2.0f);
		if (m_visibleRow == 0 && m_loaded && !m_scanPending)
			DCV_SetColor(255, 144, 0, (int)(flFade * 255.0f));
		else
			DCV_SetColor(255, 144, 0, dimAlpha);
		M_DrawMenuElementBox(m_pMenu, 150.0f, 344.0f, 580.0f, 374.0f);

		if (m_visibleRow == 1 && m_loaded && !m_scanPending)
			DCV_SetColor(255, 144, 0, (int)(flFade * 255.0f));
		else
			DCV_SetColor(255, 144, 0, dimAlpha);
		M_DrawMenuElementBox(m_pMenu, 150.0f, 378.0f, 580.0f, 408.0f);

		if (m_scrollTop < 1)
			DCV_SetColor(255, 144, 0, (int)(flFade * 64.0f));
		else
			DCV_SetColor(255, 144, 0, dimAlpha);
		M_DrawMenuElementQuad(m_pMenu, 3, 590.0f, 344.0f, 620.0f, 372.0f);

		if (m_scrollTop + 2 < m_fileCount)
			DCV_SetColor(255, 144, 0, dimAlpha);
		else
			DCV_SetColor(255, 144, 0, (int)(flFade * 64.0f));
		M_DrawMenuElementQuad(m_pMenu, 4, 590.0f, 380.0f, 620.0f, 408.0f);

		if (VMU_IsDevicePresent(m_slot * 2 + *m_piIndex))
		{
			for (i = 0; i < m_fileCount; i++)
			{
				if (!g_MenuSaves[i].pszDescription || i - m_scrollTop < 0 || i - m_scrollTop > 1)
					continue;

				y = i - m_scrollTop;
				if (y == 0)
				{
					y = 348;
					iconY = 347;
				}
				else
				{
					y = 383;
					iconY = 381;
				}

				DCV_SetHudDepth(3.0f);
				DCV_TexState_Blend();
				DCV_SetColor(255, 255, 255, (int)(flFade * 255.0f));

				if (g_MenuSaves[i].character == 2)
					M_DrawGordonIcon(m_pMenu, 151.0f, (float)iconY, 175.0f, (float)(iconY + 24));
				else if (g_MenuSaves[i].character == 4)
					M_DrawBarneyIcon(m_pMenu, 151.0f, (float)iconY, 175.0f, (float)(iconY + 24));

				Font_FitScale(m_flDescScale * 0.65f, m_flDescAspect * 0.65f, 390.0f,
					(byte *)g_MenuSaves[i].pszDescription, &textScaleX, &textScaleY);
				if (sv_language.value)
				{
					textScaleX *= 0.83f;
					textScaleY *= 0.83f;
				}
				g_flTextScaleX = textScaleX;
				g_flTextScaleY = textScaleY;
				Text_DrawStringShadow(g_MenuSaves[i].pszDescription,
					176, y, alpha, 0);
			}

		}

	}

	if (bSelected && m_pszDescription)
	{
		status = "%overwrite";
		if (!m_ready)
		{
			if (m_savePending)
			{
				VMU_SetDeviceIconState(m_slot * 2 + *m_piIndex, 2, 0);
				status = "%saving";
			}
			else if (m_scanPending)
			{
				status = "%scanning";
			}
			else if (m_mode == MENU_LOAD_SLOT)
			{
				status = m_loaded ? "%gameloadselect" : "%vmuloadselect";
			}
			else if (!m_loaded)
			{
				status = "%vmusaveselect";
				if (m_saved)
				{
					status = VMU_MarkSlotSaved();
					if (!VMU_GetCurrentDevice())
					{
						gfDrawMenu = 0;
						m_pMenu->m_state.iSoundBlocked = 0;
						Cbuf_AddText("");
						Cbuf_AddText("\n");
					}
				}
			}
			else
			{
				status = m_noSpace ? "%gamedeleteselect" : "%gamesaveselect";
			}
		}

		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect, (byte *)status, (int)(flFade * 192.0f));
	}
}

void CMenuSaveSlotItem::Select( void )
{
	if (m_ready)
	{
		m_ready = 0;
		m_fileCount = 0;
	}
	else if (m_mode == MENU_SAVE_SLOT)
	{
		if (VMU_SelectDeviceIfPresent(m_slot * 2 + *m_piIndex))
		{
			if (!m_loaded && !m_scanPending)
			{
				m_scrollTop = 0;
				m_selectedFile = 0;
				m_visibleRow = 0;
				m_scanPending = 1;
				m_frame = 0;
				m_loaded = 1;
			}
			else if (m_selectedFile < m_fileCount)
			{
				if (!m_noSpace)
				{
					if (m_selectedFile)
						m_ready = 1;
					m_savePending = 1;
					m_frame = 0;
				}
				else if (m_selectedFile > 0)
				{
					VMU_DeleteFile(g_MenuSaves[m_selectedFile].pszName);
					m_scrollTop = 0;
					m_selectedFile = 0;
					m_visibleRow = 0;
					m_scanPending = 1;
					m_frame = 0;
					m_loaded = 1;
				}
			}
		}
	}
	else if (m_mode == MENU_LOAD_SLOT && VMU_SelectDeviceIfPresent(m_slot * 2 + *m_piIndex))
	{
		if (!m_loaded && !m_scanPending)
		{
			m_loaded = 1;
			m_visibleRow = 0;
			m_scrollTop = 0;
			m_selectedFile = 0;
			m_scanPending = 1;
			m_frame = 0;
		}
		else if (m_selectedFile < m_fileCount)
		{
			if (g_MenuSaves[m_selectedFile].character == VMU_FILE_HALFLIFE)
			{
				sprintf(g_szMenuLoadCommand, "startgame valve\nload %s\n",
					g_MenuSaves[m_selectedFile].pszName);
				strcpy(m_pMenu->m_szCommand, g_szMenuLoadCommand);
				m_pMenu->m_state.flAnimating = 1.0f;
				m_pMenu->m_state.flFade = 1.0f;
				m_pMenu->m_state.flFadeFrom = 1.0f;
				m_pMenu->m_state.flFadeTo = 0.0f;
				m_pMenu->m_state.flAnimStart = Sys_FloatTime();
			}
			if (g_MenuSaves[m_selectedFile].character == VMU_FILE_BARNEY)
			{
				sprintf(g_szMenuLoadCommand, "startgame barney\nload %s\n",
					g_MenuSaves[m_selectedFile].pszName);
				strcpy(m_pMenu->m_szCommand, g_szMenuLoadCommand);
				m_pMenu->m_state.flAnimating = 1.0f;
				m_pMenu->m_state.flFade = 1.0f;
				m_pMenu->m_state.flFadeFrom = 1.0f;
				m_pMenu->m_state.flFadeTo = 0.0f;
				m_pMenu->m_state.flAnimStart = Sys_FloatTime();
			}
		}
	}
}

void CMenuSaveSlotItem::Cancel( void )
{
	CMenu	*pMenu;

	if (m_ready)
	{
		m_ready = 0;
		m_scrollTop = 0;
		m_selectedFile = 0;
		m_visibleRow = 0;
		m_frame = 0;
		m_loaded = 1;
		m_scanPending = 1;
		m_savePending = 0;
	}
	else if (m_loaded)
	{
		m_loaded = 0;
		m_fileCount = 0;
	}
	else
	{
		pMenu = m_pMenu;
		gfDrawMenu = 0;

		if (strstr(pMenu->m_state.pszCommand, "menu"))
			pMenu->m_state.iSoundBlocked = 1;
		else
			pMenu->m_state.iSoundBlocked = 0;

		Cbuf_AddText(pMenu->m_state.pszCommand);
		Cbuf_AddText("\n");
	}
}

void CMenuSaveSlotItem::Up( void )
{
	if (m_ready)
		return;

	if (!m_loaded)
	{
		*m_piIndex += m_nValues - 1;
		*m_piIndex %= m_nValues;

		if (m_piValue)
			*m_piValue = *m_piIndex;

		return;
	}

	if (m_selectedFile > 0)
	{
		if (m_selectedFile == m_scrollTop)
			m_scrollTop--;

		m_selectedFile--;
		m_visibleRow = m_selectedFile - m_scrollTop;
	}
}

void CMenuSaveSlotItem::Down( void )
{
	if (m_ready)
		return;

	if (!m_loaded)
	{
		(*m_piIndex)++;
		*m_piIndex %= m_nValues;

		if (m_piValue)
			*m_piValue = *m_piIndex;

		return;
	}

	if (m_selectedFile + 1 < m_fileCount)
	{
		if ((m_scrollTop + 1) % m_fileCount == m_selectedFile)
			m_scrollTop++;

		m_selectedFile++;
		m_visibleRow = m_selectedFile - m_scrollTop;
	}
}

void CMenuSaveSlotItem::Left( void )
{
	CMenu	*pMenu;
	int		start;

	if (m_ready || m_loaded)
		return;

	*m_piIndex = 0;
	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuSaveSlotItem::Right( void )
{
	CMenu	*pMenu;
	int		start;

	if (m_ready || m_loaded)
		return;

	*m_piIndex = 0;
	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

CMenuVolumeSlider::CMenuVolumeSlider( CMenu* pMenu, menuslider_t* pSlider, int x, int y, int id, int align )
{
	m_pMenu = pMenu;
	m_pszLabel = pSlider->pszLabel;
	m_x = x;
	m_y = y;
	m_flLabelScale = 1.0f;
	m_flLabelAspect = 1.3333f;
	m_pszDescription = pSlider->pszDescription;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;
	m_flDescScale = 1.0f;
	m_flDescAspect = 1.3333f;
	m_nSteps = pSlider->nSteps;
	m_piToggle = (int *)align;
	m_piValue = &pSlider->iValue;
	m_id = id;

	if (id == 0xeb)
		*m_piValue = (int)(Cvar_VariableValue("volume") * (float)(m_nSteps - 1) + 0.5f);
	else if (id == 0xec)
		*m_piValue = (int)(Cvar_VariableValue("bgmvolume") * (float)(m_nSteps - 1) + 0.5f);
	else if (id == 0xed)
		*m_piValue = (int)(Cvar_VariableValue("suitvolume") * (float)(m_nSteps - 1) + 0.5f);
}

void CMenuVolumeSlider::Draw( float flFade, qboolean bSelected )
{
	int		step;
	float		barY;
	float		barBottom;
	float		x;
	char		*state;
	char		*psz;
	int			ch;
	int			advance;
	int			width;
	int			labelAlpha;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	g_flTextScaleX = m_flLabelScale;
	g_flTextScaleY = m_flLabelAspect;

	if (sv_language.value)
	{
		g_flTextScaleX *= 0.83f;
		g_flTextScaleY *= 0.83f;
	}

	labelAlpha = (int)(flFade * (bSelected ? 255.0f : 128.0f));
	if (bSelected)
	{
		psz = m_pszLabel;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);
		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_x - 15);
		x2 = (float)(m_x + width + 15);
		y1 = (float)(m_y - 14);
		y2 = (float)(m_y + 40);
		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(3.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	Text_DrawStringShadow(m_pszLabel, m_x, m_y,
		labelAlpha, 0);

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect,
			(byte *)m_pszDescription, (int)(flFade * 192.0f));

	DCV_SetHudDepth(2.0f);
	DCV_SetColor(255, 144, 0, 128);
	DCV_TexState_Blend();

	barY = (float)(m_y + 40);
	barBottom = (float)(m_y + 60);
	x = (float)m_x;
	M_DrawMenuElementQuad2(m_pMenu, 1, x, barY, x + 30.0f, barBottom);

	for (step = 0; step < 9; step++)
	{
		x = (float)(m_x + 30 + step * 40);
		M_DrawMenuElementQuad2(m_pMenu, 0, x, barY, x + 40.0f, barBottom);
	}

	x = (float)(m_x + 390);
	M_DrawMenuElementQuad2(m_pMenu, 2, x, barY, x + 30.0f, barBottom);

	DCV_SetColor(255, 144, 0, 192);
	DCV_SetHudDepth(3.0f);
	x = (float)(m_x + 22 + *m_piValue * 40);
	M_DrawMenuElementBox(m_pMenu, x, barY - 5.0f, x + 16.0f, barBottom + 5.0f);

	if (m_piToggle)
	{
		state = *m_piToggle ? "%on" : "%off";
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(state, m_x + 340, m_y,
			labelAlpha, 0);

		DCV_SetHudDepth(3.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, 200);
		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);
		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_x + 290);
		x2 = x1 + 30.0f;
		y1 = (float)m_y;
		y2 = y1 + 30.0f;
		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.552f, 0.568f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.943f, 0.568f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.552f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.943f, 0.946f);

		if (*m_piToggle)
		{
			DCV_SetHudDepth(2.0f);
			DCV_TexState_Additive();
			DCV_SetColor(255, 144, 0, 255);
			DCV_FlushIfLarge();
			DCV_AddPolyIndices(DCV_GetVertCount(), 4);

			x1 = (float)(m_x + 295);
			x2 = x1 + 20.0f;
			y1 = (float)(m_y + 5);
			y2 = y1 + 20.0f;
			DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
			DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
			DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
			DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);
			DCV_TexState_Blend();
		}
	}
}

void CMenuVolumeSlider::Select( void )
{
	float	value;
	float	soundVolume;

	if (m_piToggle)
		*m_piToggle = 1 - *m_piToggle;
	else
		return;

	if (m_id == 0xeb)
	{
		value = (float)*m_piValue * (float)*m_piToggle / (float)(m_nSteps - 1);
		Cvar_SetValue("volume", value);
		Cvar_VariableValue("volume");
	}
	else if (m_id == 0xec)
	{
		value = (float)*m_piValue * (float)*m_piToggle / (float)(m_nSteps - 1);
		Cvar_SetValue("bgmvolume", value);
		Cvar_VariableValue("bgmvolume");

		soundVolume = Cvar_VariableValue("volume");
		if (soundVolume == 0.0f)
			soundVolume = 1.0f;
		else
			soundVolume = Cvar_VariableValue("bgmvolume") / soundVolume;
		PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
	}
	else if (m_id == 0xed)
	{
		value = (float)*m_piValue * (float)*m_piToggle / (float)(m_nSteps - 1);
		Cvar_SetValue("suitvolume", value);
		Cvar_VariableValue("suitvolume");

		soundVolume = Cvar_VariableValue("volume");
		if (soundVolume == 0.0f)
			soundVolume = 1.0f;
		else
			soundVolume = Cvar_VariableValue("suitvolume") / soundVolume;
		PlaySound("fvox/online.wav", soundVolume);
	}
}

void CMenuVolumeSlider::Cancel( void )
{
	CMenu*	pMenu;
	char	command[80];

	pMenu = m_pMenu;
	if (m_id == 0xeb || m_id == 0xec || m_id == 0xed)
	{
		sprintf(command, "joyadvancedupdate");
		if (strstr(command, "menu"))
			pMenu->m_state.iSoundBlocked = 1;
		else
			pMenu->m_state.iSoundBlocked = 0;
		Cbuf_AddText(command);
		Cbuf_AddText("\n");
		Host_WriteConfiguration();
	}

	gfDrawMenu = 0;
	if (strstr(pMenu->m_state.pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;
	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuVolumeSlider::Up( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuVolumeSlider::Down( void )
{
	CMenu*	pMenu;
	int		start;

	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuVolumeSlider::Left( void )
{
	float	value;
	float	soundVolume;

	(*m_piValue)--;
	if (*m_piValue < 0)
		*m_piValue = 0;

	if (!m_piToggle)
	{
		if (m_id == 0xeb)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("volume", value);
			Cvar_VariableValue("volume");
		}
		else if (m_id == 0xec)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("bgmvolume", value);
			Cvar_VariableValue("bgmvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("bgmvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("suitvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/online.wav", soundVolume);
		}
	}
	else
	{
		if (m_id == 0xeb)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("volume", value);
			Cvar_VariableValue("volume");
		}
		else if (m_id == 0xec)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("bgmvolume", value);
			Cvar_VariableValue("bgmvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("bgmvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("suitvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/online.wav", soundVolume);
		}
	}
}

void CMenuVolumeSlider::Right( void )
{
	float	value;
	float	soundVolume;

	(*m_piValue)++;
	if (*m_piValue >= m_nSteps)
		*m_piValue = m_nSteps - 1;

	if (!m_piToggle)
	{
		if (m_id == 0xeb)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("volume", value);
			Cvar_VariableValue("volume");
		}
		else if (m_id == 0xec)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("bgmvolume", value);
			Cvar_VariableValue("bgmvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("bgmvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("suitvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/online.wav", soundVolume);
		}
	}
	else
	{
		if (m_id == 0xeb)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("volume", value);
			Cvar_VariableValue("volume");
		}
		else if (m_id == 0xec)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("bgmvolume", value);
			Cvar_VariableValue("bgmvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("bgmvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			if (Cvar_VariableValue("volume") == 0.0f)
				soundVolume = 1.0f;
			else
				soundVolume = Cvar_VariableValue("suitvolume") /
					Cvar_VariableValue("volume");
			PlaySound("fvox/online.wav", soundVolume);
		}
	}
}

int CMenuVolumeSlider::IsActive( void )
{
	return 1;
}

CMenuSensitivitySlider::CMenuSensitivitySlider( CMenu* pMenu, menuslider_t* pSlider, int x, int y, int id )
	: CMenuVolumeSlider(pMenu, pSlider, x, y, id, 0)
{
	float	value;

	m_sensitivityId = id;

	if (id == 0xee)
	{
		value = Cvar_VariableValue("joyyawsensitivity");
		if (value < 0.0f)
			value = -value;
		*m_piValue = (int)(((value - 0.5f) / 1.8f) * (float)(m_nSteps - 1) + 0.5f);
	}
	else if (id == 0xef)
	{
		value = Cvar_VariableValue("joypitchsensitivity");
		g_iInvertPad = value < 0.0f;
		if (value < 0.0f)
			value = -value;
		*m_piValue = (int)(((value - 0.2f) / 0.9f) * (float)(m_nSteps - 1) + 0.5f);
	}
}

void CMenuSensitivitySlider::Draw( float flFade, qboolean bSelected )
{
	int		step;
	float		x;
	float		barY;
	float		barBottom;
	char		*psz;
	int			ch;
	int			advance;
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	g_flTextScaleX = m_flLabelScale;
	g_flTextScaleY = m_flLabelAspect;

	if (sv_language.value != 0.0f)
	{
		g_flTextScaleX *= 0.83f;
		g_flTextScaleY *= 0.83f;
	}

	if (bSelected)
	{
		psz = m_pszLabel;
		if (psz && *psz == '%' && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX *
					(float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture =
			M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);
		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_x - 15);
		x2 = (float)(m_x + width + 15);
		y1 = (float)(m_y - 14);
		y2 = (float)(m_y + 40);
		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	Text_DrawStringShadow(m_pszLabel, m_x, m_y,
		(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect,
			(byte *)m_pszDescription, (int)(flFade * 192.0f));

	DCV_SetHudDepth(2.0f);
	DCV_SetColor(255, 144, 0, 128);
	DCV_TexState_Blend();

	barY = (float)m_y;
	barBottom = barY + 28.0f;
	x = (float)MENU_SENSITIVITY_BAR_LEFT;
	M_DrawMenuElementQuad2(m_pMenu, 1, x, barY,
		x + (float)MENU_SENSITIVITY_SEGMENT_WIDTH, barBottom);

	for (step = 1; step <= MENU_SENSITIVITY_MIDDLE_SEGMENTS; step++)
	{
		x = (float)(MENU_SENSITIVITY_BAR_LEFT +
			step * MENU_SENSITIVITY_SEGMENT_WIDTH);
		M_DrawMenuElementQuad2(m_pMenu, 0, x, barY,
			x + (float)MENU_SENSITIVITY_SEGMENT_WIDTH, barBottom);
	}

	x = (float)(MENU_SENSITIVITY_BAR_LEFT + MENU_SENSITIVITY_BAR_WIDTH -
		MENU_SENSITIVITY_SEGMENT_WIDTH);
	M_DrawMenuElementQuad2(m_pMenu, 2, x, barY,
		x + (float)MENU_SENSITIVITY_SEGMENT_WIDTH, barBottom);

	DCV_SetColor(255, 144, 0, 192);
	DCV_SetHudDepth(3.0f);
	x = (float)(MENU_SENSITIVITY_THUMB_LEFT +
		*m_piValue * MENU_SENSITIVITY_THUMB_STEP);
	M_DrawMenuElementBox(m_pMenu, x, barY - 4.0f,
		x + (float)MENU_SENSITIVITY_THUMB_WIDTH, barY + 32.0f);
}

void CMenuSensitivitySlider::Left( void )
{
	char	command[MAX_QPATH];
	float	value;

	(*m_piValue)--;
	if (*m_piValue < 0)
		*m_piValue = 0;

	if (m_sensitivityId == 0xee)
	{
		value = (float)*m_piValue * 0.1f + 0.5f;
		sprintf(command, "joyyawsensitivity -%f\njoyadvancedupdate", value);
	}
	else
	{
		value = (float)*m_piValue * 0.05f + 0.2f;
		sprintf(command, "joypitchsensitivity %c%f\njoyadvancedupdate",
			g_iInvertPad ? '-' : ' ', value);
	}

	if (strstr(command, "menu"))
		m_pMenu->m_state.iSoundBlocked = 1;
	else
		m_pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(command);
	Host_WriteConfiguration();
}

void CMenuSensitivitySlider::Right( void )
{
	char	command[MAX_QPATH];
	float	value;

	(*m_piValue)++;
	if (*m_piValue >= m_nSteps)
		*m_piValue = m_nSteps - 1;

	if (m_sensitivityId == 0xee)
	{
		value = (float)*m_piValue * 0.1f + 0.5f;
		sprintf(command, "joyyawsensitivity -%f\njoyadvancedupdate", value);
	}
	else
	{
		value = (float)*m_piValue * 0.05f + 0.2f;
		sprintf(command, "joypitchsensitivity %c%f\njoyadvancedupdate",
			g_iInvertPad ? '-' : ' ', value);
	}

	if (strstr(command, "menu"))
		m_pMenu->m_state.iSoundBlocked = 1;
	else
		m_pMenu->m_state.iSoundBlocked = 0;

	Cbuf_AddText(command);
	Host_WriteConfiguration();
}

CMenuPresetItem::CMenuPresetItem( CMenu* pMenu, int preset, int x, int y )
	: CMenuTextItem()
{
	char	alias[13];

	m_pMenu = pMenu;
	m_align = 0;

	sprintf(m_szLabel, "%%preset_%c", preset);
	m_pszLabel = m_szLabel;
	m_labelX = x - 15;
	m_labelY = y;
	m_flLabelScale = 0.7f;
	m_flLabelAspect = 0.93331f;

	sprintf(m_szDescription, "%%activate_cont_%c", preset);
	m_pszDescription = m_szDescription;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;
	m_flDescScale = 1.0f;
	m_flDescAspect = 1.3333f;

	sprintf(m_szCommand, "exec preset_%c.cfg", preset);
	m_pszCommand = m_szCommand;
	m_bEnabled = 1;
	m_preset = (byte)preset;
	m_iAlias = 0;

	// the script the page runs also says what each button ends up doing
	strncpy(alias, m_szCommand + 5, 12);
	alias[12] = 0;
	Text_LoadAliases(&m_ppAliases, &m_nAliases, alias);
}

// The buttons the preset page points out, down the left of the picture and
// then down the right. A row with a caption instead of a button name is a
// fixed line the preset cannot change.
typedef struct presetcallout_s
{
	char*	pszJoyKey;
	char*	pszKey;
	char*	pszCaption;
	short	y;
	byte	right;
	byte	centered;
} presetcallout_t;

static presetcallout_t g_PresetCallouts[] =
{
	{ "S1AUX6", "AUX6", NULL,          120, 0, 0 },
	{ NULL,     NULL,   "%look",       168, 0, 0 },
	{ "S1AUX4", "AUX4", NULL,          216, 0, 0 },
	{ "S1AUX1", "AUX1", NULL,          248, 0, 0 },
	{ "S1AUX3", "AUX3", NULL,          280, 0, 0 },
	{ "S1AUX2", "AUX2", NULL,          312, 0, 0 },
	{ NULL,     NULL,   "%pauseshort", 382, 0, 1 },
	{ "S1AUX5", "AUX5", NULL,          120, 1, 0 },
	{ "S1JOY4", "JOY4", NULL,          168, 1, 0 },
	{ "S1JOY2", "JOY2", NULL,          216, 1, 0 },
	{ "S1JOY1", "JOY1", NULL,          264, 1, 0 },
	{ "S1JOY3", "JOY3", NULL,          312, 1, 0 },
};

/*
==================
CMenuPresetItem::DrawCallout

One button on the picture: the plate it sits on, then what the button does
under this preset. In shifted mode the shift button pulses,
and a button the preset leaves alone says so.
==================
*/
void CMenuPresetItem::DrawCallout( struct presetcallout_s* pCallout, float flFade )
{
	char	*psz;
	float	brightness;
	float	base;
	float	width;
	int		x;

	// longer translations get more room, and the plate grows with them
	width = (sv_language.value != 0) ? 130.0f : 100.0f;
	x = pCallout->right
		? ((sv_language.value != 0) ? 488 : 528)
		: ((sv_language.value != 0) ? 300 : 260);

	base = pCallout->centered ? 396.0f - width * 0.5f
		: (pCallout->right ? (float)x : (float)x - width);


	brightness = 254.0f;

	if (pCallout->pszCaption)
	{
		psz = pCallout->pszCaption;
	}
	else
	{
		psz = Text_LookupAlias(m_iAlias ? pCallout->pszJoyKey : pCallout->pszKey,
			m_ppAliases, &m_nAliases, 0);

		// mark the shift button while showing the shifted bindings
		if (m_iAlias && !strcmp(psz, Text_FindString("%shift")))
			brightness = (coss(m_pMenu->m_state.flTime * 5.23f) + 1.0f) * 80.0f + 94.0f;

		// nothing bound to it under this preset
		if (m_iAlias && !strcmp(psz, pCallout->pszJoyKey))
			psz = Text_FindString("%bind_none");
	}

	if (pCallout->centered)
	{
		Text_DrawStringCentered(0.7f, 0.93331f, psz, 396, pCallout->y + 5,
			(int)(flFade * brightness), m_iAlias, (int)width);
	}
	else if (pCallout->right)
	{
		Text_DrawStringLeft(0.7f, 0.93331f, psz, x, pCallout->y + 5,
			(int)(flFade * brightness), m_iAlias, (int)width);
	}
	else
	{
		Text_DrawStringRight(0.7f, 0.93331f, psz, x, pCallout->y + 5,
			(int)(flFade * brightness), m_iAlias, (int)width);
	}

	DCV_SetHudDepth(2.5f);
	DCV_TexState_Blend();

	if (m_iAlias == 0)
		DCV_SetColor(50, 30, 0, (int)(flFade * 250.0f));
	else
		DCV_SetColor(20, 20, 50, (int)(flFade * 250.0f));

	M_DrawMenuElementBox(m_pMenu, base - 5.0f, (float)pCallout->y,
		base + width + 8.0f, (float)(pCallout->y + 28));

}

/*
==================
CMenuPresetItem::Draw

The line itself, and when the stick is resting on it the controller picture
with a line running out to every button it names.
==================
*/
void CMenuPresetItem::Draw( float flFade, qboolean bSelected )
{
	int		base;
	char	*psz;
	int		width;
	int		advance;
	int		ch;
	float	x1, x2;
	float	y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		psz = m_pszLabel;
		if (psz && g_nLangTags > 0)
		{
			for (ch = 0; ch < g_nLangTags; ch++)
			{
				if (!strcmp(psz, g_pLangTags[ch].tag))
				{
					psz = g_pLangTags[ch].string;
					break;
				}
			}
		}

		width = 0;
		if (psz)
		{
			while (*psz)
			{
				ch = (byte)*psz++;
				if (ch >= 192)
					ch -= 64;

				advance = (int)(g_flTextScaleX * (float)((dcfont_t *)draw_chars)->fontinfo[ch].charwidth + 1.4f);
				if (advance < 2)
					advance = 2;
				if (advance > 40)
					advance = 40;
				width += advance;
			}
		}

		if (m_align == 0)
			DCV_SetColor(255, 144, 0, (int)(flFade * 100.0f));
		else
			DCV_SetColor(95, 95, 255, (int)(flFade * 120.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		m_pMenu->m_state.iElementTexture = M_LoadMenuTexture(m_pMenu, "gfx/menu_elements_alpha.pvr");
		GL_BindStage(m_pMenu->m_state.iElementTexture, 0);

		DCV_FlushIfLarge();
		DCV_AddPolyIndices(DCV_GetVertCount(), 4);

		x1 = (float)(m_labelX - 15);
		x2 = (float)(m_labelX + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);

		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.059f, 0.559f);
		DCV_AddVertex(x2, y1, dc_depthhud.value, 0.446f, 0.559f);
		DCV_AddVertex(x1, y2, dc_depthhud.value, 0.059f, 0.946f);
		DCV_AddVertex(x2, y2, dc_depthhud.value, 0.446f, 0.946f);

		DCV_TexState_Blend();
		DCV_SetHudDepth(4.0f);
		DCV_SetColor(255, 144, 0, (int)(flFade * 120.0f));

		DCV_SetHudDepth(2.3f);
		DCV_TexState_Blend();
		DCV_SetColor(255, 255, 255, (int)(flFade * 255.0f));

		M_DrawControllerIcon(m_pMenu, 266.0f, 120.0f, 522.0f, 376.0f);

		// the lines that run from each button out to its caption
		DCV_SetHudDepth(2.3f);
		DCV_SetColor(255, 255, 255, (int)(flFade * 220.0f));

		m_pMenu->m_state.iLinesTexture = M_LoadMenuTexture(m_pMenu, "gfx/menu_controllerlines.pvr");
		GL_BindStage(m_pMenu->m_state.iLinesTexture, 0);

		DCV_FlushIfLarge();
		base = DCV_GetVertCount();
		DCV_AddPolyIndices(base, 4);

		DCV_AddVertex(266.0f, 120.0f, dc_depthhud.value, 0.0f, 0.0f);
		DCV_AddVertex(522.0f, 120.0f, dc_depthhud.value, 1.0f, 0.0f);
		DCV_AddVertex(266.0f, 376.0f, dc_depthhud.value, 0.0f, 1.0f);
		DCV_AddVertex(522.0f, 376.0f, dc_depthhud.value, 1.0f, 1.0f);

		DrawCallout(&g_PresetCallouts[0], flFade);
		DrawCallout(&g_PresetCallouts[1], flFade);
		DrawCallout(&g_PresetCallouts[2], flFade);
		DrawCallout(&g_PresetCallouts[3], flFade);
		DrawCallout(&g_PresetCallouts[4], flFade);
		DrawCallout(&g_PresetCallouts[5], flFade);
		DrawCallout(&g_PresetCallouts[6], flFade);
		DrawCallout(&g_PresetCallouts[7], flFade);
		DrawCallout(&g_PresetCallouts[8], flFade);
		DrawCallout(&g_PresetCallouts[9], flFade);
		DrawCallout(&g_PresetCallouts[10], flFade);
		DrawCallout(&g_PresetCallouts[11], flFade);
	}

	if (m_pszLabel)
	{
		g_flTextScaleX = m_flLabelScale;
		g_flTextScaleY = m_flLabelAspect;

		if (sv_language.value != 0.0f)
		{
			g_flTextScaleX *= 0.83f;
			g_flTextScaleY *= 0.83f;
		}

		Text_DrawStringShadow(m_pszLabel, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (bSelected && m_pszDescription)
	{
		Text_DrawCenteredStatus(m_flDescScale, m_flDescAspect, (byte *)m_pszDescription,
			(int)(flFade * 192.0f));
	}

	g_nTextCharGap = 0;
}

void CMenuPresetItem::Select( void )
{
	Cbuf_AddText(m_pszCommand);

	switch (m_preset)
	{
	case 'A':
		g_iControlPreset = 0;
		break;

	case 'B':
		g_iControlPreset = 1;
		break;

	case 'C':
		g_iControlPreset = 2;
		break;
	}
}

void CMenuPresetItem::Cancel( void )
{
	CMenu	*pMenu;
	char	command[MAX_QPATH];

	sprintf(command, "joyadvancedupdate");
	pMenu = m_pMenu;
	if (strstr(command, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;
	Cbuf_AddText(command);
	Cbuf_AddText("\n");
	Host_WriteConfiguration();

	gfDrawMenu = 0;
	if (strstr(pMenu->m_state.pszCommand, "menu"))
		pMenu->m_state.iSoundBlocked = 1;
	else
		pMenu->m_state.iSoundBlocked = 0;
	Cbuf_AddText(pMenu->m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuPresetItem::Up( void )
{
	CMenu*	pMenu;
	int		start;

	m_iAlias = 0;
	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected--;
		if (pMenu->m_state.iSelected < 0)
			pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

void CMenuPresetItem::Down( void )
{
	CMenu*	pMenu;
	int		start;

	m_iAlias = 0;
	pMenu = m_pMenu;
	start = pMenu->m_state.iSelected;

	do
	{
		pMenu->m_state.iSelected++;
		if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
			pMenu->m_state.iSelected = 0;
	} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
		|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
		&& pMenu->m_state.iSelected != start);
}

int CMenuPresetItem::IsActive( void )
{
	return 1;
}

void CMenuPresetItem::Left( void )
{
	m_iAlias = 1 - m_iAlias;
}

void CMenuPresetItem::Right( void )
{
	m_iAlias = 1 - m_iAlias;
}

CMenuBindItem::CMenuBindItem( CMenu* pMenu )
{
	m_pMenu = pMenu;
	m_nEntries = M_BuildControlList(IN_KeyboardActive(), IN_JoystickActive());
	m_leftValue = (int)Cvar_VariableValue("joyadvaxisy");
	m_rightValue = (int)Cvar_VariableValue("joyadvaxisx");
	m_mode = 1;
	m_selection = 0;
	m_capturing = 0;
	m_reserved18 = 0;
	m_reserved28 = 0;
	m_reserved24 = 0;
}

void CMenuBindItem::Draw( float flFade, qboolean bSelected )
{
	int			keyboard;
	int			joystick;
	int			oldKeyboard;
	int			oldJoystick;
	int			alpha;
	int			row;
	int			slot;
	int			y;
	int			control;
	int			key;
	int			capturedKey;
	char		*pszKey;
	char		*pszCommand;
	char		bindCommand[MAX_QPATH];

	oldKeyboard = m_reserved24;
	oldJoystick = m_reserved28;
	keyboard = IN_KeyboardActive();
	joystick = IN_JoystickActive();
	m_reserved24 = keyboard;
	m_reserved28 = joystick;

	if (oldKeyboard != m_reserved24 || oldJoystick != m_reserved28)
	{
		m_mode = 1;
		m_selection = 0;
		m_reserved18 = 1;
	}

	if (m_reserved18)
	{
		m_nEntries = M_BuildControlList(IN_KeyboardActive(), IN_JoystickActive());
		m_reserved18 = 0;
	}

	if (m_mode > m_nEntries + 6)
		m_mode = m_nEntries + 6;

	DCV_SetHudDepth(2.0f);
	DCV_TexState_Blend();

	alpha = (int)(flFade * 255.0f);
	DCV_SetColor(255, 144, 0, alpha);
	Text_DrawStringLeft(0.7f, 0.93331f, "%custom_action", 170, 134,
		alpha, 0, 190);
	Text_DrawStringLeft(0.7f, 0.93331f, "%keybutton", 370, 134,
		alpha, 0, 230);

	DCV_SetColor(255, 144, 0, (int)(flFade * 128.0f));
	y = 164;
	for (slot = 0; slot < 8; slot++, y += 30)
	{
		row = m_selection + slot;
		if (row == m_mode && m_capturing)
		{
			DCV_SetColor(255, 144, 0, (int)(flFade * 240.0f));
			M_DrawMenuElementBox(m_pMenu, 348.0f, (float)(y - 4),
				604.0f, (float)(y + 24));
		}

		if (row == m_mode)
			DCV_SetColor(255, 144, 0, (int)(flFade * 198.0f));

		DCV_SetHudDepth(2.0f);
		if (row == 0 || row == 3 || row == 5)
			DCV_SetColor(255, 144, 0, (int)(flFade * 70.0f));
		M_DrawMenuElementBox(m_pMenu, 160.0f, (float)(y - 4),
			610.0f, (float)(y + 24));
		DCV_SetHudDepth(3.0f);

		if (row == 0)
		{
			Text_DrawStringCentered(0.7f, 0.93331f, "%axis_options", 385, y,
				alpha, 0, 400);
		}
		else if (row == 1)
		{
			Text_DrawStringLeft(0.7f, 0.93331f, "%up_down_axis", 170, y,
				alpha, 0, 190);
			Text_DrawStringLeft(0.7f, 0.93331f, g_pszAxisActions[m_leftValue], 370, y,
				alpha, 0, 230);
		}
		else if (row == 2)
		{
			Text_DrawStringLeft(0.7f, 0.93331f, "%left_right_axis", 170, y,
				alpha, 0, 190);
			Text_DrawStringLeft(0.7f, 0.93331f, g_pszAxisActions[m_rightValue], 370, y,
				alpha, 0, 230);
		}
		else if (row == 3)
		{
			Text_DrawStringCentered(0.7f, 0.93331f, "%shift_things", 385, y,
				alpha, 0, 400);
		}
		else if (row == 4)
		{
			Text_DrawStringLeft(0.7f, 0.93331f, "%shift", 170, y,
				alpha, 0, 190);

			pszKey = M_ControlKeyName(joyshift1.string);

			Text_DrawStringLeft(0.7f, 0.93331f, pszKey, 370, y,
				alpha, 0, 230);
		}
		else if (row == 5)
		{
			Text_DrawStringCentered(0.7f, 0.93331f, "%key_bindings", 385, y,
				alpha, 0, 400);
		}
		else if (row - 6 < m_nEntries)
		{
			control = g_ControlKeys[row - 6].control;
			key = g_ControlKeys[row - 6].key;

			if (row == 6 || g_ControlKeys[row - 7].control != control)
			{
				Text_DrawStringLeft(0.7f, 0.93331f,
					g_ControlActions[control].pszDescription, 170, y, alpha, 0, 190);
			}

			if (key < 0)
				pszKey = "---";
			else
			{
				pszKey = M_ControlKeyName(Key_KeynumToString(key));
			}

			Text_DrawStringLeft(0.7f, 0.93331f, pszKey, 370, y,
				alpha, 0, 230);
		}

		DCV_SetColor(255, 144, 0, (int)(flFade * 128.0f));
	}

	if (m_selection > 0)
		M_DrawMenuElementQuad(m_pMenu, 3, 520.0f, 138.0f, 590.0f, 158.0f);

	if (m_selection < m_nEntries - 2)
		M_DrawMenuElementQuad(m_pMenu, 4, 520.0f, 401.0f, 590.0f, 421.0f);

	capturedKey = Key_GetCapturedKey();
	if (capturedKey)
	{
		Key_SetCaptureMode(0);
		m_capturing = 0;
		m_reserved18 = 1;

		control = g_ControlKeys[m_mode - 6].control;
		pszCommand = g_ControlActions[control].pszCommand;
		if (pszCommand && strcmp(pszCommand, "(null)")
			&& capturedKey != K_AUX7 && capturedKey != LAST_JOYSHIFT1)
		{
			sprintf(bindCommand, "bind \"%s\" \"%s\"\n",
				Key_KeynumToString(capturedKey), pszCommand);
			Cbuf_AddText(bindCommand);
			key_dest = key_menu;
		}
	}

	if (m_mode < 5)
	{
		Text_DrawCenteredStatus(1.0f, 1.3333f,
			(byte *)(m_capturing ? "%lr_choose_function" : "%a_change_function"), 190);
	}
	else
	{
		Text_DrawCenteredStatus(1.0f, 1.3333f,
			(byte *)(m_capturing ? "%but_set_function" : "%a_change_function"), 190);
	}
}

void CMenuBindItem::Select( void )
{
	if (m_mode < 3)
		m_capturing = 1;
	else if (m_mode < 5)
		m_capturing = 1;
	else
	{
		Key_SetCaptureMode(1);
		m_capturing = 1;
	}
}

void CMenuBindItem::Cancel( void )
{
	CMenu	*pMenu;
	char	command[80];

	if (!m_capturing)
	{
		sprintf(command, "joyadvancedupdate");
		pMenu = m_pMenu;
		pMenu->m_state.iSoundBlocked = strstr(command, "menu") != NULL;
		Cbuf_AddText(command);
		Cbuf_AddText("\n");
		Host_WriteConfiguration();

		gfDrawMenu = 0;
		pMenu->m_state.iSoundBlocked = strstr(pMenu->m_state.pszCommand, "menu") != NULL;
		Cbuf_AddText(pMenu->m_state.pszCommand);
		Cbuf_AddText("\n");
		return;
	}

	if (m_mode == 4)
	{
		sprintf(command, "bind \"%s\" \"\"\n", "AUX6");
		Cbuf_AddText(command);
	}

	m_reserved18 = 1;
	m_capturing = 0;
}

void CMenuBindItem::Up( void )
{
	if (!m_capturing && m_mode > 0)
	{
		if (m_selection == m_mode)
			m_selection--;

		m_mode--;
		if (m_mode == 3 || m_mode == 5)
			m_mode--;

		if (m_selection == 3)
			m_selection--;
		if (m_selection == 5)
			m_selection--;

		if (!m_mode)
			m_mode++;
	}
}

void CMenuBindItem::Down( void )
{
	if (m_capturing)
		return;

	if (m_mode + 1 >= m_nEntries + 6)
		return;

	if ((m_selection + 7) % (m_nEntries + 6) == m_mode)
		m_selection++;

	m_mode++;
	if (m_mode == 3)
		m_mode++;
	if (m_mode == 5)
		m_mode++;
}

void CMenuBindItem::Left( void )
{
	int	key;
	int	choice;

	if (!m_capturing)
		return;

	if (m_mode == 1)
	{
		m_leftValue = (m_leftValue + JOY_AXIS_ACTIONS - 1) % JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisy", (float)m_leftValue);
	}
	else if (m_mode == 2)
	{
		m_rightValue = (m_rightValue + JOY_AXIS_ACTIONS - 1) % JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisx", (float)m_rightValue);
	}
	else if (m_mode == 4)
	{
		for (choice = 0; choice < NUM_SHIFT_KEYS; choice++)
		{
			if (!Q_stricmp(Cvar_VariableString("joyshift1"), g_pszShiftKeys[choice]))
				break;
		}

		key = choice - 1;
		if (key < 0)
			key += NUM_SHIFT_KEYS;

		Cvar_Set("joyshift1", g_pszShiftKeys[key]);
	}
}

void CMenuBindItem::Right( void )
{
	int	key;
	int	choice;

	if (!m_capturing)
		return;

	if (m_mode == 1)
	{
		m_leftValue = (m_leftValue + 1) % JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisy", (float)m_leftValue);
	}
	else if (m_mode == 2)
	{
		m_rightValue = (m_rightValue + 1) % JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisx", (float)m_rightValue);
	}
	else if (m_mode == 4)
	{
		for (choice = 0; choice < NUM_SHIFT_KEYS; choice++)
		{
			if (!Q_stricmp(Cvar_VariableString("joyshift1"), g_pszShiftKeys[choice]))
				break;
		}

		key = choice + 1;
		if (key >= NUM_SHIFT_KEYS)
			key -= NUM_SHIFT_KEYS;

		Cvar_Set("joyshift1", g_pszShiftKeys[key]);
	}
}

int CMenuBindItem::IsActive( void )
{
	return 1;
}

/*
==================
M_ClearTextures

Forget every artwork slot without touching what is in the texture cache.
==================
*/
void M_ClearTextures( CMenu* pMenu )
{
	int		i;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		pMenu->m_state.pTextureNames[i] = NULL;
		pMenu->m_state.iTextures[i] = 0;
	}
}

/*
==================
M_FreeTextures

Hand the page's artwork back to the texture cache.
==================
*/
void M_FreeTextures( CMenu* pMenu )
{
	int		i;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (pMenu->m_state.pTextureNames[i])
			DC_ForceFreeTextureByName(pMenu->m_state.pTextureNames[i]);

		pMenu->m_state.iTextures[i] = 0;
	}
}

/*
==================
M_LoadMenuTexture

Look the artwork up in the page's own slots, and pull it off the disc into a
free one if it is not there yet.
==================
*/
int M_LoadMenuTexture( CMenu* pMenu, char* pszName )
{
	pvrheader_t*	pHeader;
	int				texture;
	int				slot;
	int				length;
	int				i;

	texture = 0;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (pMenu->m_state.pTextureNames[i]
			&& !strcmp(pMenu->m_state.pTextureNames[i], pszName))
		{
			texture = pMenu->m_state.iTextures[i];
			break;
		}
	}

	if (texture)
		return texture;

	slot = -1;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (!pMenu->m_state.pTextureNames[i])
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
		return 0;

	pMenu->m_state.iTextures[slot] = 0;

	pHeader = (pvrheader_t *)COM_LoadTempFile(pszName, &length);
	if (pHeader)
	{
		pMenu->m_state.iTextures[slot] = DC_LoadTexture(pszName, GLT_WORLD,
			pHeader->width, pHeader->height, pHeader, FALSE, TEX_TYPE_GBIX, NULL);
		pMenu->m_state.pTextureNames[slot] = pszName;
	}

	// A failed lookup did not create a temporary block.
	if (pHeader)
		COM_FreeTempFile();

	return pMenu->m_state.iTextures[slot];
}

/*
==================
M_DrawMenuElementBox

The controller page uses a small nine-slice frame cut from the menu element
sheet. Thin boxes take the whole tile; larger boxes keep the corners fixed
and stretch the middle strips.
==================
*/
void M_DrawMenuElementBox( CMenu* pMenu, float x0, float y0, float x1, float y1 )
{
	float	left, right, top, bottom;
	int		base;

	pMenu->m_state.iElementTexture = M_LoadMenuTexture(pMenu, "gfx/menu_elements_alpha.pvr");
	GL_BindStage(pMenu->m_state.iElementTexture, 0);

	if (x1 - x0 <= 15.0f || y1 - y0 <= 24.0f)
	{
		DCV_FlushIfLarge();
		base = DCV_GetVertCount();
		DCV_AddPolyIndices(base, 4);

		DCV_AddVertex(x0, y0, dc_depthhud.value, 0.752f, 0.182f);
		DCV_AddVertex(x1, y0, dc_depthhud.value, 0.946f, 0.182f);
		DCV_AddVertex(x0, y1, dc_depthhud.value, 0.752f, 0.570f);
		DCV_AddVertex(x1, y1, dc_depthhud.value, 0.946f, 0.570f);
		return;
	}

	left = x0 + 5.0f;
	right = x1 - 5.0f;
	top = y0 + 8.0f;
	bottom = y1 - 8.0f;

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 8);
	DCV_AddVertex(x0, top, dc_depthhud.value, 0.752f, 0.311f);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0.752f, 0.182f);
	DCV_AddVertex(left, top, dc_depthhud.value, 0.817f, 0.311f);
	DCV_AddVertex(left, y0, dc_depthhud.value, 0.817f, 0.182f);
	DCV_AddVertex(right, top, dc_depthhud.value, 0.881f, 0.311f);
	DCV_AddVertex(right, y0, dc_depthhud.value, 0.881f, 0.182f);
	DCV_AddVertex(x1, top, dc_depthhud.value, 0.946f, 0.311f);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 0.946f, 0.182f);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 8);
	DCV_AddVertex(x0, bottom, dc_depthhud.value, 0.752f, 0.441f);
	DCV_AddVertex(x0, top, dc_depthhud.value, 0.752f, 0.311f);
	DCV_AddVertex(left, bottom, dc_depthhud.value, 0.817f, 0.441f);
	DCV_AddVertex(left, top, dc_depthhud.value, 0.817f, 0.311f);
	DCV_AddVertex(right, bottom, dc_depthhud.value, 0.881f, 0.441f);
	DCV_AddVertex(right, top, dc_depthhud.value, 0.881f, 0.311f);
	DCV_AddVertex(x1, bottom, dc_depthhud.value, 0.946f, 0.441f);
	DCV_AddVertex(x1, top, dc_depthhud.value, 0.946f, 0.311f);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 8);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0.752f, 0.570f);
	DCV_AddVertex(x0, bottom, dc_depthhud.value, 0.752f, 0.441f);
	DCV_AddVertex(left, y1, dc_depthhud.value, 0.817f, 0.570f);
	DCV_AddVertex(left, bottom, dc_depthhud.value, 0.817f, 0.441f);
	DCV_AddVertex(right, y1, dc_depthhud.value, 0.881f, 0.570f);
	DCV_AddVertex(right, bottom, dc_depthhud.value, 0.881f, 0.441f);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 0.946f, 0.570f);
	DCV_AddVertex(x1, bottom, dc_depthhud.value, 0.946f, 0.441f);
}

/*
==================
M_DrawMenuElementQuad

Put one of the four turns of the element strip into the given rectangle.
==================
*/
void M_DrawMenuElementQuad( CMenu* pMenu, int orientation, float x0, float y0, float x1, float y1 )
{
	float	u00, v00, u10, v10, u01, v01, u11, v11;
	int		base;

	switch (orientation)
	{
	case 1:
		u00 = 1.0f; v00 = 0.012f;
		u10 = 1.0f; v10 = 0.181f;
		u01 = 0.7f; v01 = 0.012f;
		u11 = 0.7f; v11 = 0.181f;
		break;

	case 2:
		u00 = 0.7f; v00 = 0.181f;
		u10 = 0.7f; v10 = 0.012f;
		u01 = 1.0f; v01 = 0.181f;
		u11 = 1.0f; v11 = 0.012f;
		break;

	case 3:
		u00 = 0.7f; v00 = 0.012f;
		u10 = 1.0f; v10 = 0.012f;
		u01 = 0.7f; v01 = 0.181f;
		u11 = 1.0f; v11 = 0.181f;
		break;

	case 4:
		u00 = 0.7f; v00 = 0.181f;
		u10 = 1.0f; v10 = 0.181f;
		u01 = 0.7f; v01 = 0.012f;
		u11 = 1.0f; v11 = 0.012f;
		break;
	}

	pMenu->m_state.iElementTexture = M_LoadMenuTexture(pMenu, "gfx/menu_elements_alpha.pvr");
	GL_BindStage(pMenu->m_state.iElementTexture, 0);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, u00, v00);
	DCV_AddVertex(x1, y0, dc_depthhud.value, u10, v10);
	DCV_AddVertex(x0, y1, dc_depthhud.value, u01, v01);
	DCV_AddVertex(x1, y1, dc_depthhud.value, u11, v11);
}

/*
==================
M_DrawMenuElementQuad2

The six pieces the item frames are built out of: the four corners and the two
end caps, all cut from the same strip.
==================
*/
void M_DrawMenuElementQuad2( CMenu* pMenu, int corner, float x0, float y0, float x1, float y1 )
{
	float	u00, v00, u10, v10, u01, v01, u11, v11;
	int		base;

	switch (corner)
	{
	case 0:
		u00 = 0.346f; v00 = 0;
		u10 = 0.662f; v10 = 0;
		u01 = 0.346f; v01 = 0.5f;
		u11 = 0.662f; v11 = 0.5f;
		break;

	case 5:
		u00 = 0.346f; v00 = 0.5f;
		u10 = 0.346f; v10 = 0;
		u01 = 0.662f; v01 = 0.5f;
		u11 = 0.662f; v11 = 0;
		break;

	case 3:
		u00 = 0;      v00 = 0.5f;
		u10 = 0;      v10 = 0;
		u01 = 0.346f; v01 = 0.5f;
		u11 = 0.346f; v11 = 0;
		break;

	case 4:
		u00 = 0.346f; v00 = 0.5f;
		u10 = 0.346f; v10 = 0;
		u01 = 0;      v01 = 0.5f;
		u11 = 0;      v11 = 0;
		break;

	case 1:
		u00 = 0;      v00 = 0;
		u10 = 0.346f; v10 = 0;
		u01 = 0;      v01 = 0.5f;
		u11 = 0.346f; v11 = 0.5f;
		break;

	case 2:
		u00 = 0.346f; v00 = 0;
		u10 = 0;      v10 = 0;
		u01 = 0.346f; v01 = 0.5f;
		u11 = 0;      v11 = 0.5f;
		break;
	}

	pMenu->m_state.iElementTexture = M_LoadMenuTexture(pMenu, "gfx/menu_elements_alpha.pvr");
	GL_BindStage(pMenu->m_state.iElementTexture, 0);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, u00, v00);
	DCV_AddVertex(x1, y0, dc_depthhud.value, u10, v10);
	DCV_AddVertex(x0, y1, dc_depthhud.value, u01, v01);
	DCV_AddVertex(x1, y1, dc_depthhud.value, u11, v11);
}

/*
==================
M_DrawControllerIcon

Put the controller picture in the given rectangle. The in-game page has room
for the bigger one.
==================
*/
void M_DrawControllerIcon( CMenu* pMenu, float x0, float y0, float x1, float y1 )
{
	int		base;

	if (cls.state == ca_active)
		pMenu->m_state.iControllerTexture = M_LoadMenuTexture(pMenu, "gfx/menu_controller_128.pvr");
	else
		pMenu->m_state.iControllerTexture = M_LoadMenuTexture(pMenu, "gfx/menu_controller.pvr");

	GL_BindStage(pMenu->m_state.iControllerTexture, 0);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

/*
==================
M_DrawGordonIcon

Freeman's portrait, dropping back to the full size picture when the small one
is not on the disc.
==================
*/
void M_DrawGordonIcon( CMenu* pMenu, float x0, float y0, float x1, float y1 )
{
	int		base;

	pMenu->m_state.iGordonTexture = M_LoadMenuTexture(pMenu, "gfx/menu_gordon_32.pvr");
	if (!pMenu->m_state.iGordonTexture)
		pMenu->m_state.iGordonTexture = M_LoadMenuTexture(pMenu, "gfx/menu_gordon.pvr");

	GL_BindStage(pMenu->m_state.iGordonTexture, 0);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

/*
==================
M_DrawBarneyIcon
==================
*/
void M_DrawBarneyIcon( CMenu* pMenu, float x0, float y0, float x1, float y1 )
{
	int		base;

	pMenu->m_state.iBarneyTexture = M_LoadMenuTexture(pMenu, "gfx/menu_barney_32.pvr");
	if (!pMenu->m_state.iBarneyTexture)
		pMenu->m_state.iBarneyTexture = M_LoadMenuTexture(pMenu, "gfx/menu_barney.pvr");

	GL_BindStage(pMenu->m_state.iBarneyTexture, 0);

	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);

	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

/*
==================
M_FadeIn

Start the page fading up from nothing.
==================
*/
void M_FadeIn( CMenu* pMenu )
{
	pMenu->m_state.flAnimating = 1.0f;
	pMenu->m_state.flFade = 0;
	pMenu->m_state.flFadeFrom = 0;
	pMenu->m_state.flFadeTo = 1.0f;
	pMenu->m_state.flAnimStart = Sys_FloatTime();
}

/*
==================
M_FadeOut

Start the page fading away.
==================
*/
void M_FadeOut( CMenu* pMenu )
{
	pMenu->m_state.flAnimating = 1.0f;
	pMenu->m_state.flFade = 1.0f;
	pMenu->m_state.flFadeFrom = 1.0f;
	pMenu->m_state.flFadeTo = 0;
	pMenu->m_state.flAnimStart = Sys_FloatTime();
}

/*
==================
CMenu::CMenu

Build the named page and start it fading in.
==================
*/
CMenu::CMenu( char* pszMenu )
{
	if (!strcmp(pszMenu, "main") || !strcmp(pszMenu, "gamemenu"))
		S_BlockSound();

	m_state.flAnimTime = 0.1f;
	m_state.iSoundBlocked = 0;
	m_state.iTopItem = 0;
	m_state.flScaleX = 1.0f;
	m_state.flScaleY = 1.0f;
	m_state.iVisible = 1;
	m_state.iEnabled = 1;
	m_state.flTime = m_state.flLastTime = Sys_FloatTime();

	strcpy(m_szCommand, "");

	M_ClearTextures(this);

	M_BuildMenu(this, pszMenu);

	M_FadeIn(this);

	gfDrawMenu = 1;
	UI_Activate();
}

/*
==================
CMenu::~CMenu

Give the artwork back, hand the keyboard over and let the sound run again.
==================
*/
CMenu::~CMenu( void )
{
	M_FreeTextures(this);

	UI_Deactivate();

	if (!m_state.iSoundBlocked)
		S_UnblockSound();
}

/*
==================
M_AnimateLerp

Slide the page's fade value from where it started towards where it is going,
and stop once the time is up.
==================
*/
void M_AnimateLerp( CMenu* pMenu )
{
	float	frac;

	if (pMenu->m_state.flTime - pMenu->m_state.flAnimStart > pMenu->m_state.flAnimTime)
	{
		pMenu->m_state.flFade = pMenu->m_state.flFadeTo;
		pMenu->m_state.flAnimating = 0;
		return;
	}

	frac = (pMenu->m_state.flTime - pMenu->m_state.flAnimStart) / pMenu->m_state.flAnimTime;
	pMenu->m_state.flFade = pMenu->m_state.flFadeFrom
		+ frac * (pMenu->m_state.flFadeTo - pMenu->m_state.flFadeFrom);
}

/*
==================
M_CheckCheatCode

Feed one button into both secret sequences. Getting to the end of either
one unlocks what it guards.
==================
*/
void M_CheckCheatCode( CMenu* pMenu, char button )
{
	char	szCredits[10] = "llbbldrab";
	char	szSecret[10] = "rrbbldrab";

	if (button == szCredits[g_iCreditsCodePos])
	{
		g_iCreditsCodePos++;
		if (!szCredits[g_iCreditsCodePos])
			gfCreditsCode = 1;
	}
	else
	{
		g_iCreditsCodePos = 0;
	}

	if (button == szSecret[g_iSecretCodePos])
	{
		g_iSecretCodePos++;
		if (!szSecret[g_iSecretCodePos])
			gfSecretCode = 1;
	}
	else
	{
		g_iSecretCodePos = 0;
	}
}

/*
==================
M_EnableAllItems

Unlock every passcode at once.
==================
*/
void M_EnableAllItems( void )
{
	menucode_t*	code;

	for (code = g_MenuCodes; code->button1 != -1; code++)
		code->enabled = TRUE;
}

/*
==================
UI_Draw

Draw the open page. Called from SCR_UpdateScreen while gfDrawMenu is set.
==================
*/
void UI_Draw( void )
{
	if (gpActiveMenu)
		UI_MenuDraw(gpActiveMenu);
}

/*
==================
UI_Update

Run the page's input, and tear it down once it has closed itself.
==================
*/
void UI_Update( void )
{
	if (!gpActiveMenu)
		return;

	UI_MenuInput(gpActiveMenu);

	if (gfDrawMenu)
		return;

	delete gpActiveMenu;
	gpActiveMenu = NULL;
}

/*
==================
UI_OpenMenu

Open a page by name. The cache is handed back first so the page has room for
its own artwork.
==================
*/
void UI_OpenMenu( char* pszMenu )
{
	CMenu*	pMenu;

	if (cls.state == ca_active || cls.state == ca_disconnected)
	{
		Cache_FlushToDisk();
		Cache_FreeAll();

		pMenu = new CMenu(pszMenu);
		gpActiveMenu = pMenu;
	}
}

/*
==================
M_BuildMenu

Fill the page with the items its script asks for. Most of them stack down the
page a line at a time; the passcode lists and the save slots keep counts of
their own so they can carry on where the last one left off.
==================
*/
void M_BuildMenu( CMenu* pMenu, char* pszMenu )
{
	menupage_t*		page;
	menuitemdef_t*	pDef;
	menuoption_t*	pOption;
	menuslider_t*	pSlider;
	menucode_t*		code;
	CMenuCheatItem*	pCheat;
	CMenuItemBase**	ppItem;
	int*			pId;
	int				id;
	int				spacing;
	int				left;
	int				top;
	int				first;
	int				wordY;
	int				codeY;
	int				nCodes;
	int				slotX;
	int				nSlots;
	int				start;
	int				i;

	nSlots = 0;

	strcpy(pMenu->m_szName, pszMenu);

	ppItem = pMenu->m_pItems;
	for (i = 0; i < MAX_MENU_ITEMS; i++)
		ppItem[i] = NULL;

	page = NULL;

	for (i = 0; i < sizeof(g_MenuPages) / sizeof(g_MenuPages[0]); i++)
	{
		if (!strcmp(pszMenu, g_MenuPages[i].pszName))
		{
			page = &g_MenuPages[i];
			break;
		}
	}

	if (!page)
	{
		Sys_Error("Can't set up menu %s!\n", pszMenu);
	}
	else
	{
		if (page->fullscreen)
		{
			// a page that owns the screen; in game it sits a little higher so
			// the world still shows underneath, and the first few lines step
			// in from the left
			spacing = 40;
			left = 54;

			if (cls.state == ca_active)
			{
				first = 0;
				top = 160;
			}
			else
			{
				first = 8;
				top = 176;
			}
		}
		else
		{
			spacing = 26;
			left = 80;
			top = 90;
			first = 0;
		}

		wordY = 210;
		nCodes = 0;

		pMenu->m_state.pszCommand = page->pszCommand;

		if (page->items[0] != MI_END)
		{
			slotX = nSlots * 104 + 220;

			for (pId = page->items; *pId != MI_END; pId++, ppItem++)
			{
				id = *pId;

				// the debug page stays out of sight until the button code
				// that unlocks it has been entered
				if (id != 0x71 || gfSecretCode)
				{
					if (id < 0x65)
					{
						*ppItem = new CMenuTitleItem(pMenu);
					}
					else if (id < 0xb0)
					{
						pDef = NULL;
						for (i = 0; i < sizeof(g_MenuItems) / sizeof(g_MenuItems[0]); i++)
						{
							if (g_MenuItems[i].id == id)
							{
								pDef = &g_MenuItems[i];
								break;
							}
						}

						// the way back only belongs on a page that has not
						// already filled itself with passcodes
						if (pDef && (id != 0xa8 || !nCodes))
						{
							if (id == 0xa5)
								*ppItem = new CMenuStaticItem(pMenu, pDef, left, top, 0);
							else if (id == 0xa7)
								*ppItem = new CMenuReturnItem(pMenu, pDef, 20, 420, 0);
							else if (id == 0xa6)
								*ppItem = new CMenuHintItem(pMenu, pDef, 100, 387, 0);
							else if (id == 0x68 || id == 0x7c || id == 0x7a || id == 0x7b)
								*ppItem = new CMenuTextItem(pMenu, pDef, left, top, 1);
							else
								*ppItem = new CMenuTextItem(pMenu, pDef, left, top, 0);

							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
						}
					}
					else if (id < 0xc9)
					{
						pDef = NULL;
						for (i = 0; i < sizeof(g_MenuItems) / sizeof(g_MenuItems[0]); i++)
						{
							if (g_MenuItems[i].id == id)
							{
								pDef = &g_MenuItems[i];
								break;
							}
						}

						// one line per chapter the player has unlocked
						if (pDef && g_MenuCodes[0].button1 != -1)
						{
							codeY = nCodes * 40 + 176;

							for (code = g_MenuCodes; code->button1 != -1; code++)
							{
								if (code->enabled
									&& !strcmp(pDef->pszDescription, code->pszName))
								{
									*ppItem = new CMenuCodeTextItem(pMenu, pDef, 100, codeY, 0);
									codeY += 40;
									nCodes++;
								}
							}
						}
					}
					else if (id < 0xd3)
					{
						pOption = NULL;
						for (i = 0; i < sizeof(g_MenuOptions) / sizeof(g_MenuOptions[0]); i++)
						{
							if (g_MenuOptions[i].id == id)
							{
								pOption = &g_MenuOptions[i];
								break;
							}
						}

						if (pOption)
						{
							switch (id)
							{
							case 0xd0:
								*ppItem = new CMenuWordItem(pMenu, pOption, wordY, 256, &g_iAccessNoun2);
								wordY += 158;
								break;

							case 0xcd:
								*ppItem = new CMenuStereoItem(pMenu, pOption, 169, 376, NULL);
								break;

							case 0xce:
								*ppItem = new CMenuWordItem(pMenu, pOption, wordY, 256, &g_iAccessNoun1);
								wordY += 158;
								break;

							case 0xcf:
								*ppItem = new CMenuWordItem(pMenu, pOption, wordY, 256, &g_iAccessVerb);
								wordY += 158;
								break;

							default:
								*ppItem = new CMenuToggleItem(pMenu, pOption, left, top, id);
								top += spacing;
								if (first)
								{
									left += first;
									first += 8;
								}
								break;
							}
						}
					}
					else if (id < 0xe1)
					{
						pOption = NULL;
						for (i = 0; i < sizeof(g_MenuOptions) / sizeof(g_MenuOptions[0]); i++)
						{
							if (g_MenuOptions[i].id == id)
							{
								pOption = &g_MenuOptions[i];
								break;
							}
						}

						// one line per cheat the player has unlocked
						if (pOption && g_MenuCodes[0].button1 != -1)
						{
							codeY = nCodes * 40 + 176;

							for (code = g_MenuCodes; code->button1 != -1; code++)
							{
								if (code->enabled
									&& !strcmp(pOption->pszDescription, code->pszName))
								{
									pCheat = new CMenuCheatItem(pMenu, pOption, 100, codeY, &code->result);
									pCheat->m_pfnBind = pOption->pfnBind;

									*ppItem = pCheat;
									codeY += 40;
									nCodes++;
								}
							}
						}
					}
					else if (id < 0xe6)
					{
						switch (id)
						{
						case 0xe5:
							*ppItem = new CMenuIconItem(pMenu, 300, 300, 0xe5, 128);
							break;

						case 0xe2:
							*ppItem = new CMenuIconItem(pMenu, 200, 100, 0xe2, 152);
							break;

						case 0xe3:
							*ppItem = new CMenuIconItem(pMenu, 390, 180, 0xe3, 128);
							break;

						case 0xe4:
							*ppItem = new CMenuIconItem(pMenu, 70, 317, 0xe4, 128);
							break;
						}
					}
					else if (id < 0xe9)
					{
						pOption = NULL;
						for (i = 0; i < sizeof(g_MenuOptions) / sizeof(g_MenuOptions[0]); i++)
						{
							if (g_MenuOptions[i].id == id)
							{
								pOption = &g_MenuOptions[i];
								break;
							}
						}

						*ppItem = new CMenuSaveSlotItem(pMenu, pOption, slotX, nSlots, id);
						nSlots++;
						slotX += 104;
					}
					else if (id < 0xf0)
					{
						pSlider = NULL;
						for (i = 0; i < sizeof(g_MenuSliders) / sizeof(g_MenuSliders[0]); i++)
						{
							if (g_MenuSliders[i].id == id)
							{
								pSlider = &g_MenuSliders[i];
								break;
							}
						}

						if (pSlider)
						{
							if (id == 0xeb)
							{
								*ppItem = new CMenuVolumeSlider(pMenu, pSlider, 169, 136, 0xeb, 0);
							}
							else if (id == 0xec)
							{
								*ppItem = new CMenuVolumeSlider(pMenu, pSlider, 169, 296, 0xec, 0);
							}
							else if (id == 0xed)
							{
								*ppItem = new CMenuVolumeSlider(pMenu, pSlider, 169, 216, 0xed, 0);
							}
							else if (id == 0xee)
							{
								*ppItem = new CMenuSensitivitySlider(pMenu, pSlider, left, top, 0xee);
								top += spacing;
								if (first)
								{
									left += first;
									first += 8;
								}
							}
							else if (id == 0xef)
							{
								*ppItem = new CMenuSensitivitySlider(pMenu, pSlider, left, top, 0xef);
								top += spacing;
								if (first)
								{
									left += first;
									first += 8;
								}
							}
						}
					}
					else if (id < 0xf4)
					{
						if (id == 0xf1)
						{
							*ppItem = new CMenuPicItem(pMenu, "gfx/menu_backdrop.pvr");
						}
						else if (id == 0xf2)
						{
							*ppItem = new CMenuPicItem(pMenu, "gfx/startup.pvr");

							// the title screens take their time fading
							pMenu->m_state.flAnimTime = 1.0f;
						}
						else
						{
							*ppItem = new CMenuPicItem(pMenu, "gfx/startup2.pvr");
							pMenu->m_state.flAnimTime = 1.0f;
						}
					}
					else if (id < 0xf6)
					{
						*ppItem = new CMenuCreditsItem(pMenu);
					}
					else
					{
						switch (id)
						{
						case 0xf7:
							*ppItem = new CMenuSaveHeaderItem(pMenu);
							break;

						case 0xf8:
							*ppItem = new CMenuPresetItem(pMenu, 'A', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xf9:
							*ppItem = new CMenuPresetItem(pMenu, 'B', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xfa:
							*ppItem = new CMenuPresetItem(pMenu, 'C', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xfb:
							*ppItem = new CMenuBindItem(pMenu);
							break;

						case 0xfc:
							*ppItem = new CMenuAttractItem(pMenu);
							break;

						case 0xfd:
							*ppItem = new CMenuAnyKeyItem(pMenu);
							break;
						}
					}
				}
			}
		}

		// a page comes up with nothing held down
		for (i = 0; i < MAX_MENU_BUTTONS; i++)
			joymenubuttons[i] = 0;

		pMenu->m_state.iSelected = 0;

		// settle the stick on the first line it can actually land on
		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected--;
			if (pMenu->m_state.iSelected < 0)
				pMenu->m_state.iSelected = MAX_MENU_ITEMS - 1;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);

		start = pMenu->m_state.iSelected;
		do
		{
			pMenu->m_state.iSelected++;
			if (pMenu->m_state.iSelected > MAX_MENU_ITEMS - 1)
				pMenu->m_state.iSelected = 0;
		} while ((!pMenu->m_pItems[pMenu->m_state.iSelected]
			|| !pMenu->m_pItems[pMenu->m_state.iSelected]->IsActive())
			&& pMenu->m_state.iSelected != start);
	}
}

/*
==================
UI_MenuDraw
==================
*/
void UI_MenuDraw( CMenu* pMenu )
{
	CMenuItemBase*	pItem;
	int			i;
	int			row;
	int			skip;

	if (!strcmp(pMenu->m_szName, "activate"))
	{
		// the codes scroll behind three fixed items
		if (pMenu->m_pItems[0])
			pMenu->m_pItems[0]->Draw(pMenu->m_state.flFade, pMenu->m_state.iSelected == 0);

		if (pMenu->m_pItems[1])
			pMenu->m_pItems[1]->Draw(pMenu->m_state.flFade, pMenu->m_state.iSelected == 1);

		if (pMenu->m_pItems[2])
			pMenu->m_pItems[2]->Draw(pMenu->m_state.flFade, pMenu->m_state.iSelected == 2);

		skip = pMenu->m_state.iTopItem;
		row = 0;

		for (i = 3; i < MAX_MENU_ITEMS; i++)
		{
			pItem = pMenu->m_pItems[i];
			if (!pItem)
				continue;

			if (skip > 0)
			{
				skip--;
			}
			else
			{
				if (i == pMenu->m_state.iSelected)
					pMenu->m_state.iSelectedRow = pMenu->m_state.iTopItem + row;

				pItem->SetPos(100.0f, ((float)row - 0.5f) * 40.0f + 176.0f);
				pItem->Draw(pMenu->m_state.flFade, i == pMenu->m_state.iSelected);

				row++;
			}

			if (row >= 7)
				break;
		}
	}
	else
	{
		for (i = 0; i < MAX_MENU_ITEMS; i++)
		{
			pItem = pMenu->m_pItems[i];
			if (pItem)
				pItem->Draw(pMenu->m_state.flFade, i == pMenu->m_state.iSelected);
		}
	}
}

/*
==================
UI_MenuInput
==================
*/
void UI_MenuInput( CMenu* pMenu )
{
	CMenuItemBase*	pItem;

	pMenu->m_state.flTime = Sys_FloatTime();

	if (pMenu->m_state.flAnimating == 1.0f)
	{
		M_AnimateLerp(pMenu);
		return;
	}

	if (pMenu->m_state.flFade == 0)
	{
		// the page has faded away; run whatever it left behind
		gfDrawMenu = 0;

		if (!strlen(pMenu->m_szCommand))
			return;

		// keep the sound blocked when the command is only going to bring up
		// another page
		if (strstr(pMenu->m_szCommand, "menu"))
			pMenu->m_state.iSoundBlocked = 1;
		else
			pMenu->m_state.iSoundBlocked = 0;

		Cbuf_AddText(pMenu->m_szCommand);
		Cbuf_AddText("\n");
		return;
	}

	// start backs out of the in-game page and confirms everywhere else
	if (joymenubuttons[3])
	{
		if (!strcmp(pMenu->m_szName, "gamemenu"))
			joymenubuttons[1] = 1;
		else
			joymenubuttons[0] = 1;
	}

	if (joymenubuttons[7] || joymenubuttons[15])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'u');
		pMenu->m_pItems[pMenu->m_state.iSelected]->Up();
	}

	if (joymenubuttons[6] || joymenubuttons[14])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'd');
		pMenu->m_pItems[pMenu->m_state.iSelected]->Down();
	}

	if (joymenubuttons[5] || joymenubuttons[13])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'r');
		pMenu->m_pItems[pMenu->m_state.iSelected]->Right();
	}

	if (joymenubuttons[4] || joymenubuttons[12])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'l');
		pMenu->m_pItems[pMenu->m_state.iSelected]->Left();
	}

	if (joymenubuttons[0])
	{
		PlaySound("common/wpn_select.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'a');

		pItem = pMenu->m_pItems[pMenu->m_state.iSelected];
		if (!pItem->IsActive())
		{
			gfDrawMenu = 0;

			if (strstr(pMenu->m_state.pszCommand, "menu"))
				pMenu->m_state.iSoundBlocked = 1;
			else
				pMenu->m_state.iSoundBlocked = 0;

			Cbuf_AddText(pMenu->m_state.pszCommand);
			Cbuf_AddText("\n");
		}
		else
		{
			pItem->Select();
		}
	}

	if (joymenubuttons[1])
	{
		PlaySound("common/wpn_denyselect.wav", 1.0f);
		M_CheckCheatCode(pMenu, 'b');

		pItem = pMenu->m_pItems[pMenu->m_state.iSelected];
		if (!pItem->IsActive())
		{
			gfDrawMenu = 0;

			if (strstr(pMenu->m_state.pszCommand, "menu"))
				pMenu->m_state.iSoundBlocked = 1;
			else
				pMenu->m_state.iSoundBlocked = 0;

			Cbuf_AddText(pMenu->m_state.pszCommand);
			Cbuf_AddText("\n");
		}
		else if (strcmp(pMenu->m_szName, "main") && pMenu->m_state.pszCommand)
		{
			pItem->Cancel();
		}
	}

	if (joymenubuttons[8])
		M_CheckCheatCode(pMenu, 'x');

	if (joymenubuttons[9])
		M_CheckCheatCode(pMenu, 'y');
}

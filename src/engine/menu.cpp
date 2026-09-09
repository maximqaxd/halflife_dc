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
#if HLDC_MP
#include "decal.h"
#include "cl_servercache.h"
#endif

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

void CMenuSaveSlotItem::InitSaveList( qboolean bSaving )
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

int CMenuSaveSlotItem::CountSaveFiles( void )
{
	int count = 0;
	int i;

	for (i = 0; i < MAX_MENU_SAVE_FILES; i++)
	{
		if (g_MenuSaves[i].pszDescription)
			count++;
	}
	return count;
}

int CMenuSaveSlotItem::AddSaveFile( char* pszName, char* pszDescription, int character, void* pUserData )
{
	CMenuSaveSlotItem	*pSaveItem;

	pSaveItem = (CMenuSaveSlotItem *)pUserData;
	if ((pSaveItem->m_noSpace && pSaveItem->m_mode == MENU_SAVE_SLOT && character != 16)
		|| character == 2 || character == 4)
	{
		g_MenuSaves[g_nMenuSaves].pszName = pszName;
		g_MenuSaves[g_nMenuSaves].pszDescription = pszDescription;
		g_MenuSaves[g_nMenuSaves].character = character;
		g_nMenuSaves++;
	}
	return 1;
}

int CMenuSaveSlotItem::BuildSaveFilename( qboolean bSaving, qboolean bNoSpace )
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
	"AUX2", "AUX3", "AUX4", "AUX5", "AUX6"
};

int g_nShiftKeys = sizeof(g_pszShiftKeys) / sizeof(g_pszShiftKeys[0]);
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
#if HLDC_MP
	{ "impulse 201", "SPRAY DECAL", "Spray your decal" },
	{ "+showscores", "SCOREBOARD", "Hold to show scores" },
	{ "messagemode", "CHAT", "Send a message to all players" },
	{ "messagemode2", "TEAM CHAT", "Send a message to your team" },
#endif
	{ NULL, NULL, NULL }
};

controlkey_t g_ControlKeys[MAX_MENU_CONTROL_KEYS];
#if HLDC_MP
static const char* g_pszBindFocus;
#endif

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

__forceinline char * CMenuBindItem::TranslateKeyName( char *pszKey )
{
	int i;

	for (i = 0; g_ControlKeyNames[i][0]; i++)
	{
		if (!strcmp(g_ControlKeyNames[i][0], pszKey))
		{
			char* psz = g_ControlKeyNames[i][1];
			int j;
			for (j = 0; j < g_nLangTags; j++)
			{
				if (!strcmp(psz, g_pLangTags[j].tag))
					return g_pLangTags[j].string;
			}
			return psz;
		}
	}
	return pszKey;
}


char * CMenuBindItem::GetKeyName( char *pszKey )
{
	return CMenuBindItem::TranslateKeyName(pszKey);
}

/*
==================
CMenuPresetItem::LookupAlias

Say what a button does. The key/command pairs come out of the preset script;
find the one this button is named in, look its command up in the action list
and hand back the wording for it. A button the script never mentions answers
with its own name.
==================
*/
char *CMenuPresetItem::LookupAlias( char *pszKey, char **ppAliases, int *pnAliases, qboolean bLong )
{
	controlaction_t	*pAction;
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
		if (!*pnAliases)
			return pszKey;
		if (!strcmp(ppAliases[i * 2 + 1], pAction->pszCommand))
			break;
	}

	if (!pAction->pszCommand)
		return pszKey;

	if (bLong)
		return Text_LocalizeString(pAction->pszDescription);
	return Text_LocalizeString(pAction->pszLabel);
}

int CMenuBindItem::BuildControlList( qboolean bKeyboard, qboolean bJoystick )
{
	controlaction_t	*pAction;
	int				key;
	int				count;
	short			control;
	int				found;
	qboolean			add;

	count = 0;
	control = 0;

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
#if HLDC_MP
	{ 0x100, "MULTIPLAYER", "menu multiplayer", "Join a game or set up your player" },
	{ 0x101, "JOIN GAME", "menu mpbrowser", "Find a server or enter its address" },
	{ 0x102, "PLAYER SETUP", "menu mpsetup", "Change your name and spray selection" },
#endif
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
#if HLDC_MP
	{ "multiplayer", "menu main", 1,
		{ 0x02, 0xf1, 0x101, 0x102, 0xa7, MI_END } },
	{ "mpbrowser", "menu multiplayer", 1,
		{ 0x02, 0xf1, 0x110, 0xa7, MI_END } },
	{ "mpsetup", "menu multiplayer", 1,
		{ 0x02, 0xf1, 0x111, 0xa7, MI_END } },
#endif
	{ "splash", "menu splash2", 1,
		{ 0xf2,  0xfc,  0xfe, } },
	{ "splash2", NULL, 1,
		{ 0xf3,  0xfd,  0xfe, } },
	{ "main", NULL, 1,
		{ 0x02,  0xf1,  0x66,  0x6d,  0x6c,
#if HLDC_MP
		  0x100,
#endif
		  0xa0,  0x71,  0xfe, } },
	{ "gamemenu", "", 1,
		{ 0x02,  0xf1,  0x70,  0x6d,  0x6c,  0x74,  0xa7,  0xfe, } },
	{ "continuemenu", NULL, 1,
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
	{ -1,  0,  0, "%code_invalid", FALSE, 0 },
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

CMenuItemBase::~CMenuItemBase( void )
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
	: CMenuItemBase(pMenu)
{
	m_align = align;

	m_pszLabel = pDef->pszLabel;
	m_flLabelScale = 1.0f;
	m_flLabelAspect = 1.3333f;
	m_labelX = x;
	m_labelY = y;

	m_pszDescription = pDef->pszDescription;
	m_flDescScale = 1.0f;
	m_flDescAspect = 1.3333f;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;

	m_pszCommand = pDef->pszCommand;
	m_bEnabled = 1;
}

CMenuTextItem::~CMenuTextItem( void )
{
}

__forceinline void CMenu::SetColor( int r, int g, int b, int a )
{
	m_state.rgba[0] = r;
	m_state.rgba[1] = g;
	m_state.rgba[2] = b;
	m_state.rgba[3] = a;
	DCV_SetColor(r, g, b, a);
}


__forceinline void CMenu::DrawElementTile( float x0, float y0, float x1, float y1 )
{
	int base;
	m_state.iElementTexture = LoadMenuTexture( "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_state.iElementTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0.059f, 0.559f);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 0.446f, 0.559f);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0.059f, 0.946f);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 0.446f, 0.946f);
}

__forceinline void CMenu::DrawElementTile2( float x0, float y0, float x1, float y1 )
{
	int base;
	m_state.iElementTexture = LoadMenuTexture( "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_state.iElementTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0.552f, 0.568f);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 0.943f, 0.568f);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0.552f, 0.946f);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 0.943f, 0.946f);
}

void CMenuTextItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
	int		width;
	float	x1, x2;
	float	y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow("", m_labelX, m_labelY, 0, m_align);

		psz = m_pszLabel;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		if (m_align == 0)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		else
			m_pMenu->SetColor(95, 95, 255, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(m_labelX - 15);
		x2 = (float)(m_labelX + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_TexState_Blend();
		DCV_SetHudDepth(4.0f);
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (m_pszLabel)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszLabel, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (bSelected && m_pszDescription)
	{
		Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);
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
void CMenu::ExecuteCommand( char* pszCommand, int fade, int close )
{
	if (fade && close)
	{
		strcpy(m_szCommand, pszCommand);
		FadeOut();
	}
	else
	{
		if (close)
			gfDrawMenu = 0;
		if (strstr(pszCommand, "menu"))
			m_state.iSoundBlocked = 1;
		else
			m_state.iSoundBlocked = 0;
		Cbuf_AddText(pszCommand);
		Cbuf_AddText("\n");
	}
}

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
		pMenu->FadeOut();
	}
}

/*
==================
CMenu::Cancel

Backing out runs whatever the page says it goes back to.
==================
*/
void CMenu::Cancel( void )
{


	gfDrawMenu = 0;

	if (strstr(m_state.pszCommand, "menu"))
		m_state.iSoundBlocked = 1;
	else
		m_state.iSoundBlocked = 0;

	Cbuf_AddText(m_state.pszCommand);
	Cbuf_AddText("\n");
}

void CMenuTextItem::Cancel( void )
{
	m_pMenu->Cancel();
}

__forceinline int CMenu::CountItems( void )
{
	int count = 0;
	for (int i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (m_pItems[i])
			count++;
	}
	return count;
}

/*
==================
CMenu::MoveSelection

Find the next selectable item in the given direction, wrapping at the
ends of the page and stopping when we reach the starting item.
==================
*/
__forceinline void CMenu::MoveSelection( int direction )
{
	int start = m_state.iSelected;
	do
	{
		if (direction < 0)
		{
			m_state.iSelected--;
			if (m_state.iSelected < 0)
				m_state.iSelected = MAX_MENU_ITEMS - 1;
		}
		else
		{
			m_state.iSelected++;
			if (m_state.iSelected > MAX_MENU_ITEMS - 1)
				m_state.iSelected = 0;
		}
	} while ((!m_pItems[m_state.iSelected]
		|| !m_pItems[m_state.iSelected]->IsActive())
		&& m_state.iSelected != start);
}

void CMenu::SelectPrevious( void )
{
	MoveSelection(-1);
}

void CMenuTextItem::Up( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

/*
==================
CMenu::SelectNext
==================
*/
void CMenu::SelectNext( void )
{
	MoveSelection(1);
}

void CMenuTextItem::Down( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
}

int CMenuTextItem::IsActive( void )
{
	return 1;
}

CMenuStaticItem::~CMenuStaticItem( void )
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
	: CMenuItemBase(pMenu)
{

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

	m_texture[0] = pMenu->LoadMenuTexture("gfx/menu_title.pvr");
	m_texture[1] = pMenu->LoadMenuTexture("gfx/menu_title.pvr");
}

void CMenuTitleItem::Draw( float flFade, qboolean bSelected )
{
	float	width, x, alpha;
	float	vbase, v0, v1;
	int		i;

	DCV_SetHudDepth(4.0f);
	DCV_TexState_Additive();
	GL_DisableMultitexture();
	DCV_SetTextureClamp();

	for (i = 0; i < 2; i++)
	{
		GL_BindStage(m_texture[i], 0);

		width = sins(m_pMenu->m_state.flTime / m_widthPeriod[i]) * m_widthPulse[i] + m_width[i];
		x = sins(m_pMenu->m_state.flTime / m_xPeriod[i]) * m_xPulse[i] + m_x[i];
		alpha = flFade * (sins(m_pMenu->m_state.flTime / m_alphaPeriod[i]) * m_alphaPulse[i] + m_alpha[i]) * 255.0f;

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

		DCV_AddVertex(x, (float)scr_safe_y + m_y[i], dc_depthhud.value, 0.0f, v0);
		DCV_AddVertex(x + width, (float)scr_safe_y + m_y[i], dc_depthhud.value, 1.0f, v0);
		DCV_AddVertex(x, (float)scr_safe_y + m_y[i] + m_height[i], dc_depthhud.value, 0.0f, v1);
		DCV_AddVertex(x + width, (float)scr_safe_y + m_y[i] + m_height[i], dc_depthhud.value, 1.0f, v1);
	}
}

int CMenuTitleItem::IsActive( void )
{
	return 0;
}

void CMenuHintItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
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
	width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

	Font_ApplyScale(m_flLabelScale * 0.7f, m_flLabelAspect * 0.7f);

	Text_DrawStringShadow(m_pszLabel, m_labelX - (int)((float)width * 0.5f), m_labelY,
		(int)(flFade * 190.0f), 0);
}

void CMenuReturnItem::Draw( float flFade, qboolean bSelected )
{
	int brightness;
	float scaleX, scaleY;

	DCV_TexState_Blend();
	brightness = (int)(flFade * 190.0f);
	if (m_pszLabel)
	{
		scaleX = m_flLabelScale * 0.6f;
		scaleY = m_flLabelAspect * 0.6f;
		Font_ApplyScale(scaleX, scaleY);
		Text_DrawStringShadow("%bback", m_labelX, m_labelY, brightness, 0);
	}
}

void CMenuCodeTextItem::Draw( float flFade, qboolean bSelected )
{
	char	*psz;
	int		width;
	int		count;
	float	x1, x2, y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		psz = m_pszLabel;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(m_labelX + 350 - 15);
		x2 = (float)(m_labelX + 350 + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));

		count = m_pMenu->CountItems();

		if (count - 3 > 7)
		{
			if (m_pMenu->m_state.iTopItem > 0)
				m_pMenu->DrawMenuElementQuad(3, 278.0f, 118.0f, 362.0f, 146.0f);

			count = m_pMenu->CountItems();

			if (m_pMenu->m_state.iTopItem + 7 < count - 3)
				m_pMenu->DrawMenuElementQuad(4, 278.0f, 432.0f, 362.0f, 460.0f);
		}
	}

	if (m_pszLabel)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszLabel, m_labelX + 350, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (m_pszDescription)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszDescription, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

	g_nTextCharGap = 0;
}

void CMenuCodeTextItem::Up( void )
{
	CMenu*	pMenu;
	int		count;

	pMenu = m_pMenu;
	count = pMenu->CountItems();

	if (pMenu->m_state.iSelectedRow > 0)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iSelectedRow - 1 + count - 3) % (count - 3)) < pMenu->m_state.iTopItem)
		{
			pMenu->m_state.iTopItem--;
		}

		pMenu->MoveSelection(-1);
	}
}

void CMenuCodeTextItem::Down( void )
{
	CMenu*	pMenu;
	int		count;
	int		visible;

	pMenu = m_pMenu;
	count = pMenu->CountItems();

	visible = pMenu->CountItems();

	if (pMenu->m_state.iSelectedRow + 1 < visible - 3)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem + 6) % (count - 3)) == pMenu->m_state.iSelectedRow)
		{
			pMenu->m_state.iTopItem++;
		}

		pMenu->MoveSelection(1);
	}
}

void CMenuCodeTextItem::SetPos( float x, float y )
{
	m_labelX = (int)x;
	m_labelY = (int)y;
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;
}

void CMenu::DrawControllerIconSmall( float x0, float y0, float x1, float y1 )
{
	int base;

	m_state.iControllerSmallTexture = LoadMenuTexture( "gfx/menu_controller_128.pvr");
	if (!m_state.iControllerSmallTexture)
		m_state.iControllerSmallTexture = LoadMenuTexture( "gfx/menu_controller.pvr");
	GL_BindStage(m_state.iControllerSmallTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}


__forceinline void CMenu::RenderControllerLines( float x0, float y0, float x1, float y1 )
{
	int base;

	m_state.iLinesTexture = LoadMenuTexture( "gfx/menu_controllerlines.pvr");
	GL_BindStage(m_state.iLinesTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

void CMenu::DrawControllerLines( float x0, float y0, float x1, float y1 )
{
	RenderControllerLines(x0, y0, x1, y1);
}


__forceinline void CMenu::RenderVMUIcon( float x0, float y0, float x1, float y1 )
{
	int base;

	m_state.iVMUTexture = LoadMenuTexture( "gfx/menu_vmu.pvr");
	GL_BindStage(m_state.iVMUTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

void CMenu::DrawVMUIcon( float x0, float y0, float x1, float y1 )
{
	RenderVMUIcon(x0, y0, x1, y1);
}


__forceinline void CMenu::RenderGordonIconLarge( float x0, float y0, float x1, float y1 )
{
	int base;

	m_state.iGordonLargeTexture = LoadMenuTexture( "gfx/menu_gordon.pvr");
	GL_BindStage(m_state.iGordonLargeTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

void CMenu::DrawGordonIconLarge( float x0, float y0, float x1, float y1 )
{
	RenderGordonIconLarge(x0, y0, x1, y1);
}


__forceinline void CMenu::RenderBarneyIconLarge( float x0, float y0, float x1, float y1 )
{
	int base;

	m_state.iBarneyLargeTexture = LoadMenuTexture( "gfx/menu_barney.pvr");
	GL_BindStage(m_state.iBarneyLargeTexture, 0);
	DCV_FlushIfLarge();
	base = DCV_GetVertCount();
	DCV_AddPolyIndices(base, 4);
	DCV_AddVertex(x0, y0, dc_depthhud.value, 0, 0);
	DCV_AddVertex(x1, y0, dc_depthhud.value, 1, 0);
	DCV_AddVertex(x0, y1, dc_depthhud.value, 0, 1);
	DCV_AddVertex(x1, y1, dc_depthhud.value, 1, 1);
}

void CMenu::DrawBarneyIconLarge( float x0, float y0, float x1, float y1 )
{
	RenderBarneyIconLarge(x0, y0, x1, y1);
}

__forceinline int CMenu::SelectedCharacter( void )
{
	if (!strcmp(m_szName, "newblue"))
		return 6;
	if (!strcmp(m_szName, "newhl"))
		return 4;
	return m_state.iSelected;
}

void CMenuIconItem::Draw( float flFade, qboolean bSelected )
{
	DCV_SetHudDepth(2.0f);
	m_pMenu->SetColor(255, 255, 255, (int)(flFade * 128.0f));
	DCV_TexState_Blend();

	switch (m_id)
	{
	case 0xe2:
		if (m_pMenu->SelectedCharacter() == 4 || m_pMenu->SelectedCharacter() == 5)
		{
			if (m_alpha < 214)
				m_alpha += 3;
			m_pMenu->SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		}
		else
		{
			if (m_alpha > 90)
				m_alpha -= 3;
			m_pMenu->SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		}
		m_pMenu->RenderGordonIconLarge((float)m_x, (float)m_y,
			(float)(m_x + 256), (float)(m_y + 256));
		break;

	case 0xe3:
		if (m_pMenu->SelectedCharacter() == 6)
		{
			if (m_alpha < 214)
				m_alpha += 3;
			m_pMenu->SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		}
		else
		{
			if (m_alpha > 90)
				m_alpha -= 3;
			m_pMenu->SetColor(255, 255, 255, (int)((float)m_alpha * flFade));
		}
		m_pMenu->RenderBarneyIconLarge((float)m_x, (float)m_y,
			(float)(m_x + 256), (float)(m_y + 256));
		break;

	case 0xe4:
		DCV_SetHudDepth(2.0f);
		m_pMenu->SetColor(255, 255, 255, (int)(flFade * 255.0f));
		m_pMenu->DrawControllerIcon((float)m_x, (float)m_y,
			(float)(m_x + 64), (float)(m_y + 64));
		DCV_SetHudDepth(3.0f);
		m_pMenu->SetColor(255, 255, 255, (int)(flFade * 220.0f));
		m_pMenu->DrawMenuElementBox((float)(m_x + 31), (float)(m_y + 34),
			(float)(m_x + 33), (float)(m_y + 60));
		break;

	case 0xe5:
		m_pMenu->SetColor(255, 255, 255, (int)(flFade * 255.0f));
		m_pMenu->RenderVMUIcon((float)m_x, (float)m_y,
			(float)(m_x + 32), (float)(m_y + 64));
		break;
	}
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
CMenuPicItem::CMenuPicItem( CMenu* pMenu, char* pszName )
	: CMenuItemBase(pMenu)
{

	if (cls.state == ca_active)
		m_iTexture = *(int *)conback->data;
	else
		m_iTexture = m_pMenu->LoadMenuTexture(pszName);
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

CMenuCreditsItem::CMenuCreditsItem( CMenu* pMenu )
	: CMenuItemBase(pMenu)
{
	m_bRestart = 1;
	m_flStartTime = m_pMenu->m_state.flTime;
}

/*
==================
CMenuCreditsItem::DrawLine

Draw one line of the credit roll centred on the screen. The tag character at
the head of the line picks the style it is drawn in, and every line fades out
as it leaves the middle band of the screen.
==================
*/
void CMenuCreditsItem::DrawLine( char* psz, float y, float flFade )
{
	char	tag;
	float	alpha;
	float	fade;
	float	scale;
	float	x;

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
			psz = Text_LocalizeString(psz);
		}

		x = 320.0f - (float)Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
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
			psz = Text_LocalizeString(psz);
		}

		x = 320.0f - (float)Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
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
		psz = Text_LocalizeString(psz);
	}

	x = 320.0f - (float)Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz) / 2.0f;
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
			DrawLine(*ppszLine, y, flFade);

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
CMenuAttractItem::CMenuAttractItem( CMenu* pMenu, float flDuration )
	: CMenuItemBase(pMenu)
{
	m_flTimeout = m_pMenu->m_state.flTime + flDuration;
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
		m_pMenu->FadeOut();
	}
}

void CMenuAttractItem::Select( void )
{
	strcpy(m_pMenu->m_szCommand, "menu splash2");
	m_pMenu->FadeOut();
}

void CMenuAttractItem::Cancel( void )
{
	strcpy(m_pMenu->m_szCommand, "menu splash2");
	m_pMenu->FadeOut();
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
CMenuSaveHeaderItem::CMenuSaveHeaderItem( CMenu* pMenu )
	: CMenuItemBase(pMenu)
{
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

CMenuAnyKeyItem::CMenuAnyKeyItem( CMenu* pMenu )
	: CMenuItemBase(pMenu)
{
}

void CMenuAnyKeyItem::Draw( float flFade, qboolean bSelected )
{
	byte	*psz;
	float	flScaleX;
	float	flScaleY;
	float	flBrightness;
	int		width;

	DCV_SetHudDepth(3.0f);
	DCV_TexState_Blend();

	psz = (byte *)"%copyright";
	if (g_nLangTags > 0)
	{
		psz = (byte *)Text_LocalizeString((char *)psz);
	}

	Font_FitScale(0.7f, 0.93331f, (float)(620 - scr_safe_x * 2), psz, &flScaleX, &flScaleY);

	width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

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
		psz = (byte *)Text_LocalizeString((char *)psz);
	}

	width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

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
	m_pMenu->FadeOut();
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
	: CMenuItemBase(pMenu)
{
	int		i;


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

	if (pValue->pszText)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);

		Text_DrawStringShadow(pValue->pszText, pValue->x, pValue->y,
			(int)(flFade * flAlpha), 0);
	}

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);
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
	m_pMenu->Cancel();
}

void CMenuOptionItem::Up( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

void CMenuOptionItem::Down( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
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

CMenuStereoItem::CMenuStereoItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue )
	: CMenuOptionItem(pMenu, pOption, x, y, piValue)
{
	*m_piIndex = (int)Cvar_VariableValue("stereo");
}

void CMenuStereoItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	char		*psz;
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();
	pValue = &m_pValues[*m_piIndex];

	if (bSelected)
	{


		psz = m_pszDescription;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(pValue->x - 15);
		x2 = (float)(pValue->x + width + 15);
		y1 = (float)(pValue->y - 14);
		y2 = (float)(pValue->y + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	DCV_SetHudDepth(2.0f);
	DCV_TexState_Blend();
	m_pMenu->SetColor(255, 144, 0, 200);

	x1 = (float)(pValue->x + 290);
	x2 = x1 + 30.0f;
	y1 = (float)pValue->y;
	y2 = y1 + 30.0f;
	m_pMenu->DrawElementTile2(x1, y1, x2, y2);

	if (*m_piIndex)
	{
		DCV_TexState_Additive();
		m_pMenu->SetColor(255, 144, 0, 255);
		x1 = (float)(pValue->x + 295);
		x2 = x1 + 20.0f;
		y1 = (float)(pValue->y + 5);
		y2 = y1 + 20.0f;
		m_pMenu->DrawElementTile(x1, y1, x2, y2);
		DCV_TexState_Blend();
	}

	if (m_pszDescription)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);
		Text_DrawStringShadow(m_pszDescription, pValue->x, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (pValue->pszText)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);
		Text_DrawStringShadow(pValue->pszText, pValue->x + 340, pValue->y,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

	if (bSelected)
		Text_DrawCenteredStatus((byte *)"%stereo_des", m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), pValue->flScale, pValue->flAspect);
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
	int			width;
	int			count;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	pValue = &m_pValues[*m_piIndex];
	if (bSelected)
	{


		psz = pValue->pszText;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(pValue->x + 350 - 15);
		x2 = (float)(pValue->x + 350 + width + 15);
		y1 = (float)(pValue->y - 10);
		y2 = (float)(pValue->y + 36);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));

		count = m_pMenu->CountItems();

		if (count - 3 > 7)
		{
			if (m_pMenu->m_state.iTopItem > 0)
				m_pMenu->DrawMenuElementQuad(3, 278.0f, 118.0f, 362.0f, 146.0f);

			count = m_pMenu->CountItems();

			if (m_pMenu->m_state.iTopItem + 7 < count - 3)
				m_pMenu->DrawMenuElementQuad(4, 278.0f, 432.0f, 362.0f, 460.0f);
		}
	}

	if (pValue->pszText)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);

		Text_DrawStringShadow(pValue->pszText, pValue->x + 350, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (m_pszDescription)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);

		Text_DrawStringShadow(m_pszDescription, pValue->x, pValue->y,
			(int)(flFade * (bSelected ? 192.0f : 128.0f)), 0);
	}

}

void CMenuCheatItem::Select( void )
{
	CMenuCheatItem::Right();
}

void CMenuCheatItem::Up( void )
{
	CMenu*	pMenu;
	int		count;

	pMenu = m_pMenu;
	count = pMenu->CountItems();

	if (pMenu->m_state.iTopItem > 0)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem - 1 + count - 3) % (count - 3)) < pMenu->m_state.iTopItem)
		{
			pMenu->m_state.iTopItem--;
		}

		pMenu->MoveSelection(-1);
	}
}

void CMenuCheatItem::Down( void )
{
	CMenu*	pMenu;
	int		count;
	int		visible;

	pMenu = m_pMenu;
	count = pMenu->CountItems();

	visible = pMenu->CountItems();

	if (pMenu->m_state.iSelectedRow + 1 < visible - 3)
	{
		if (count - 3 > 7
			&& ((pMenu->m_state.iTopItem + 6) % (count - 3)) == pMenu->m_state.iSelectedRow)
		{
			pMenu->m_state.iTopItem++;
		}

		pMenu->MoveSelection(1);
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
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	pValue = &m_pValues[*m_piIndex];
	if (bSelected)
	{


		psz = pValue->pszText;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(MENU_OPTION_VALUE_X - 15);
		x2 = (float)(MENU_OPTION_VALUE_X + width + 15);
		y1 = (float)(pValue->y - 14);
		y2 = (float)(pValue->y + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (pValue->pszText)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);

		Text_DrawStringShadow(pValue->pszText, MENU_OPTION_VALUE_X, pValue->y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (m_pszDescription)
	{
		Font_ApplyScale(pValue->flScale, pValue->flAspect);

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
	: CMenuItemBase(pMenu)
{
	int		i;


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

void CMenuWordItem::UpdateResult( void )
{
	float elapsed = m_pMenu->m_state.flTime - g_flAccessCodeStartTime;

	if (elapsed <= 1.9f)
	{
		g_flAccessCodeTime = 1.9f - elapsed;
		return;
	}
	g_flAccessCodeTime = 0.0f;
	g_bAccessCodeResult = 0;
}

void CMenuWordItem::Draw( float flFade, qboolean bSelected )
{
	menuvalue_t	*pValue;
	int			index;
	int			offset;
	int			width;
	int			brightness;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();
	DCV_SetHudDepth(2.0f);
	m_pMenu->SetColor(255, 144, 0, (int)(flFade * 128.0f));

	if (g_bAccessCodeResult)
		UpdateResult();

	if (bSelected)
	{
		pValue = &m_pValues[*m_piIndex];
		m_pMenu->DrawMenuElementQuad(3,
			(float)(pValue->x - 35), 118.0f,
			(float)(pValue->x + 35), 148.0f);
		m_pMenu->DrawMenuElementQuad(4,
			(float)(pValue->x - 35), 388.0f,
			(float)(pValue->x + 35), 418.0f);
	}

	pValue = &m_pValues[*m_piIndex];
	m_pMenu->SetColor(255, 144, 0, (int)(flFade * (bSelected ? 120.0f : 80.0f)));
	if (m_piValue == &g_iAccessNoun1)
	{
		pValue = &m_pValues[*m_piIndex];
		m_pMenu->DrawMenuElementBox((float)(pValue->x - 87), 152.0f,
			(float)(pValue->x + 87), 384.0f);
	}
	else if (m_piValue == &g_iAccessVerb)
		m_pMenu->DrawMenuElementBox(304.0f, 152.0f, 432.0f, 384.0f);
	else if (m_piValue == &g_iAccessNoun2)
	{
		pValue = &m_pValues[*m_piIndex];
		m_pMenu->DrawMenuElementBox((float)(pValue->x - 87), 152.0f,
			(float)(pValue->x + 87), 384.0f);
	}

	pValue = &m_pValues[*m_piIndex];
	Font_ApplyScale(pValue->flScale, pValue->flAspect);

	if (pValue->pszText)
	{
		for (offset = -2; offset <= 2; offset++)
		{
			index = (*m_piIndex + offset + m_nValues) % m_nValues;
			pValue = &m_pValues[index];


			width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)pValue->pszText);

			if (bSelected)
				brightness = (offset == 0) ? (int)(flFade * 255.0f) : (int)(flFade * 180.0f);
			else
				brightness = (offset == 0) ? (int)(flFade * 205.0f) : (int)(flFade * 128.0f);

			if (bSelected && offset == 0)
			{
				m_pMenu->SetColor(255, 144, 0, (int)(flFade * 80.0f));
				DCV_SetHudDepth(2.0f);
				DCV_TexState_Additive();

				x1 = pValue->x - (width / 2) * 1.2f;
				x2 = pValue->x + (width / 2) * 1.2f;
				y1 = (float)(pValue->y - 10);
				y2 = (float)(pValue->y + 36);
				m_pMenu->DrawElementTile(x1, y1, x2, y2);

				DCV_SetHudDepth(4.0f);
				DCV_TexState_Blend();
				m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
				pValue = &m_pValues[(*m_piIndex + m_nValues) % m_nValues];
			}

			Font_ApplyScale(pValue->flScale, pValue->flAspect);
			Text_DrawStringShadow(pValue->pszText, pValue->x - width / 2,
				pValue->y + offset * 40, brightness, 0);
		}
	}

	if (m_piValue == &g_iAccessNoun1)
	{
		if (g_bAccessCodeResult && g_pszAccessCodeResult)
		{
			brightness = (int)(g_flAccessCodeTime * 192.0f);
			if (brightness > 192)
				brightness = 192;
			Text_DrawCenteredStatus((byte *)g_pszAccessCodeResult, m_pMenu->m_state.flTime,
			brightness, m_flDescScale, m_flDescAspect);
		}
		else if (m_pszDescription)
			Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);
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
	m_pMenu->Cancel();
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

	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

void CMenuWordItem::Right( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
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
			CMenuSaveSlotItem::InitSaveList(m_mode == MENU_SAVE_SLOT);
			VMU_SelectDeviceIfPresent(m_slot * 2 + *m_piIndex);
			VMU_EnumFiles((vmuenumproc_t)CMenuSaveSlotItem::AddSaveFile, this);
			CMenuSaveSlotItem::BuildSaveFilename(m_mode == MENU_SAVE_SLOT, m_noSpace);

			m_fileCount = CountSaveFiles();
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
			if (m_mode == MENU_LOAD_SLOT)
			{
				text = Text_LocalizeString("%novmuload1");
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)text) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(text, y, 176, alpha, 0);
				text = Text_LocalizeString("%novmuload2");
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)text) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(text, y, 216, alpha, 0);
				text = Text_LocalizeString("%novmuload3");
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)text) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(text, y, 256, alpha, 0);
			}
			else
			{
				char message[128];
				char *output = message;
				text = Text_LocalizeString("%novmusave1");
				if (strchr(text, '@'))
				{
					while (*text != '@')
						*output++ = *text++;
					sprintf(output, "%d", (Host_SaveGameSize() + 511) / 512);
					strcat(output, text + 1);
				}
				else
					strcpy(message, text);
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)message) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(message, y, 176, alpha, 0);
				text = Text_LocalizeString("%novmusave2");
				if (strchr(text, '@'))
				{
					while (*text != '@')
						*output++ = *text++;
					sprintf(output, "%d", (Host_SaveGameSize() + 511) / 512);
					strcat(output, text + 1);
				}
				else
					strcpy(message, text);
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)message) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(message, y, 216, alpha, 0);
				text = Text_LocalizeString("%novmusave3");
				if (strchr(text, '@'))
				{
					while (*text != '@')
						*output++ = *text++;
					sprintf(output, "%d", (Host_SaveGameSize() + 511) / 512);
					strcat(output, text + 1);
				}
				else
					strcpy(message, text);
				y = 320 - Font_MeasureString((dcfont_t *)draw_chars, (byte *)message) / 2;
				g_flTextScaleX = sv_language.value ? 0.83f : 1.0f;
				g_flTextScaleY = sv_language.value ? 1.106639f : 1.3333f;
				Text_DrawStringShadow(message, y, 256, alpha, 0);
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

	DCV_SetHudDepth(2.0f);
	m_pMenu->SetColor(255, 144, 0, deviceAlpha[0]);
	m_pMenu->DrawMenuElementBox((float)(pValue->x - 20), 200.0f,
		(float)(pValue->x + 20), 260.0f);
	m_pMenu->SetColor(255, 144, 0, deviceAlpha[1]);
	m_pMenu->DrawMenuElementBox((float)(pValue->x - 20), 264.0f,
		(float)(pValue->x + 20), 324.0f);
	m_pMenu->SetColor(255, 255, 255, (int)(alpha * flFade));
	DCV_SetHudDepth(4.0f);
	m_pMenu->DrawControllerIcon((float)(pValue->x - 32), 130.0f,
		(float)(pValue->x + 32), 194.0f);
	m_pMenu->SetColor(255, 255, 255, deviceAlpha[0]);
	if (VMU_IsDevicePresent(m_slot * 2 + 0))
	{
		m_pMenu->RenderVMUIcon((float)(pValue->x - 12), 207.0f,
			(float)(pValue->x + 12), 255.0f);
	}
	m_pMenu->SetColor(255, 255, 255, deviceAlpha[1]);
	if (VMU_IsDevicePresent(m_slot * 2 + 1))
	{
		m_pMenu->RenderVMUIcon((float)(pValue->x - 12), 271.0f,
			(float)(pValue->x + 12), 319.0f);
	}
	m_pMenu->SetColor(255, 144, 0, dimAlpha);

	if (bSelected)
	{
		if (VMU_IsDevicePresent(m_slot * 2 + *m_piIndex))
		{
			text = Text_LocalizeString("%port");
			sprintf(label, "%s %c", text, m_slot + 'A');
			textScaleX = sv_language.value ? 0.66399997f : 0.8f;
			textScaleY = sv_language.value ? 0.88531119f : 1.06664f;
			g_flTextScaleX = textScaleX;
			g_flTextScaleY = textScaleY;
			Text_DrawStringShadow(label, 54,
				cls.state == ca_active ? 160 : 176, alpha, 0);

			text = Text_LocalizeString("%slot");
			sprintf(label, "%s %c", text, *m_piIndex + '1');
			Text_DrawStringShadow(label, cls.state == ca_active ? 54 : 62,
				cls.state == ca_active ? 200 : 216, alpha, 0);
		}

		DCV_SetHudDepth(2.0f);
		if (m_visibleRow == 0 && m_loaded && !m_scanPending)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 255.0f));
		else
			m_pMenu->SetColor(255, 144, 0, dimAlpha);
		m_pMenu->DrawMenuElementBox(150.0f, 344.0f, 580.0f, 374.0f);

		if (m_visibleRow == 1 && m_loaded && !m_scanPending)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 255.0f));
		else
			m_pMenu->SetColor(255, 144, 0, dimAlpha);
		m_pMenu->DrawMenuElementBox(150.0f, 378.0f, 580.0f, 408.0f);

		if (m_scrollTop < 1)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 64.0f));
		else
			m_pMenu->SetColor(255, 144, 0, dimAlpha);
		m_pMenu->DrawMenuElementQuad(3, 590.0f, 344.0f, 620.0f, 372.0f);

		if (m_scrollTop + 2 < m_fileCount)
			m_pMenu->SetColor(255, 144, 0, dimAlpha);
		else
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 64.0f));
		m_pMenu->DrawMenuElementQuad(4, 590.0f, 380.0f, 620.0f, 408.0f);

		if (VMU_IsDevicePresent(m_slot * 2 + *m_piIndex))
		{
			for (i = 0; i < m_fileCount; i++)
			{
				if (!g_MenuSaves[i].pszDescription)
					continue;
				if (i - m_scrollTop == 0)
				{
					DCV_SetHudDepth(3.0f);
					DCV_TexState_Blend();
					m_pMenu->SetColor(255, 255, 255, (int)(flFade * 255.0f));

					if (g_MenuSaves[i].character == 2)
						m_pMenu->DrawGordonIcon(151.0f, 347.0f, 175.0f, 371.0f);
					else if (g_MenuSaves[i].character == 4)
						m_pMenu->DrawBarneyIcon(151.0f, 347.0f, 175.0f, 371.0f);

					Font_FitScale(m_flDescScale * 0.65f, m_flDescAspect * 0.65f, 390.0f,
						(byte *)g_MenuSaves[i].pszDescription, &textScaleX, &textScaleY);
					Font_ApplyScale(textScaleX, textScaleY);
					Text_DrawStringShadow(g_MenuSaves[i].pszDescription,
						176, 348, alpha, 0);
				}
				if (i - m_scrollTop == 1)
				{
					DCV_SetHudDepth(3.0f);
					DCV_TexState_Blend();
					m_pMenu->SetColor(255, 255, 255, (int)(flFade * 255.0f));

					if (g_MenuSaves[i].character == 2)
						m_pMenu->DrawGordonIcon(151.0f, 381.0f, 175.0f, 405.0f);
					else if (g_MenuSaves[i].character == 4)
						m_pMenu->DrawBarneyIcon(151.0f, 381.0f, 175.0f, 405.0f);

					Font_FitScale(m_flDescScale * 0.65f, m_flDescAspect * 0.65f, 390.0f,
						(byte *)g_MenuSaves[i].pszDescription, &textScaleX, &textScaleY);
					Font_ApplyScale(textScaleX, textScaleY);
					Text_DrawStringShadow(g_MenuSaves[i].pszDescription,
						176, 383, alpha, 0);
				}
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
			else if (m_mode != MENU_SAVE_SLOT)
			{
				status = "confused";
			}
			else if (!m_loaded)
			{
				status = "%vmusaveselect";
				if (m_saved)
				{
					status = VMU_MarkSlotSaved();
					if (!VMU_GetSaveResult())
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

		Text_DrawCenteredStatus((byte *)status, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);
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
		(*m_piIndex)--;
		*m_piIndex += m_nValues;
		*m_piIndex %= m_nValues;

		if (m_piValue)
			*m_piValue = *m_piIndex;

	}
	else if (m_loaded)
	{
		if (m_selectedFile > 0)
		{
			if (m_selectedFile == m_scrollTop)
				m_scrollTop--;

			m_selectedFile--;
		}
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

	}
	else if (m_loaded)
	{
		if (m_selectedFile + 1 < m_fileCount)
		{
			if ((m_scrollTop + 1) % m_fileCount == m_selectedFile)
				m_scrollTop++;

			m_selectedFile++;
		}
		m_visibleRow = m_selectedFile - m_scrollTop;
	}
}

void CMenuSaveSlotItem::Left( void )
{
	CMenu	*pMenu;

	if (m_ready || m_loaded)
		return;

	*m_piIndex = 0;
	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

void CMenuSaveSlotItem::Right( void )
{
	CMenu	*pMenu;

	if (m_ready || m_loaded)
		return;

	*m_piIndex = 0;
	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
}

CMenuVolumeSlider::CMenuVolumeSlider( CMenu* pMenu, menuslider_t* pSlider, int x, int y, int id, int align )
	: CMenuItemBase(pMenu)
{
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
	float		barY;
	float		barBottom;
	float		x;
	char		*state;
	char		*psz;
	int			width;
	int			labelAlpha;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	labelAlpha = (int)(flFade * (bSelected ? 255.0f : 128.0f));
	if (bSelected)
	{
		psz = m_pszLabel;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(m_x - 15);
		x2 = (float)(m_x + width + 15);
		y1 = (float)(m_y - 14);
		y2 = (float)(m_y + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(3.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (m_pszLabel)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszLabel, m_x, m_y,
			labelAlpha, 0);
	}

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);

	DCV_SetHudDepth(2.0f);
	m_pMenu->SetColor(255, 144, 0, 128);
	DCV_TexState_Blend();

	barY = (float)(m_y + 40);
	barBottom = (float)(m_y + 60);
	m_pMenu->DrawMenuElementQuad2(1, (float)(m_x + 0), (float)(m_y + 40),
		(float)(m_x + 30), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 30), (float)(m_y + 40),
		(float)(m_x + 70), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 70), (float)(m_y + 40),
		(float)(m_x + 110), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 110), (float)(m_y + 40),
		(float)(m_x + 150), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 150), (float)(m_y + 40),
		(float)(m_x + 190), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 190), (float)(m_y + 40),
		(float)(m_x + 230), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 230), (float)(m_y + 40),
		(float)(m_x + 270), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 270), (float)(m_y + 40),
		(float)(m_x + 310), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 310), (float)(m_y + 40),
		(float)(m_x + 350), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(0, (float)(m_x + 350), (float)(m_y + 40),
		(float)(m_x + 390), (float)(m_y + 60));
	m_pMenu->DrawMenuElementQuad2(2, (float)(m_x + 390), (float)(m_y + 40),
		(float)(m_x + 420), (float)(m_y + 60));

	m_pMenu->SetColor(255, 144, 0, 192);
	DCV_SetHudDepth(3.0f);
	x = (float)(m_x + 22 + *m_piValue * 40);
	m_pMenu->DrawMenuElementBox(x, barY - 5.0f, x + 16.0f, barBottom + 5.0f);

	if (m_piToggle)
	{
		state = *m_piToggle ? "%on" : "%off";
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(state, m_x + 340, m_y,
			labelAlpha, 0);

		DCV_SetHudDepth(3.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, 200);
		x1 = (float)(m_x + 290);
		x2 = x1 + 30.0f;
		y1 = (float)m_y;
		y2 = y1 + 30.0f;
		m_pMenu->DrawElementTile2(x1, y1, x2, y2);

		if (*m_piToggle)
		{
			DCV_SetHudDepth(2.0f);
			DCV_TexState_Additive();
			m_pMenu->SetColor(255, 144, 0, 255);
			x1 = (float)(m_x + 295);
			x2 = x1 + 20.0f;
			y1 = (float)(m_y + 5);
			y2 = y1 + 20.0f;
			m_pMenu->DrawElementTile(x1, y1, x2, y2);
			DCV_TexState_Blend();
		}
	}
}

void CMenuVolumeSlider::PreviewMusicVolume( void )
{
	float soundVolume;

	if (Cvar_VariableValue("volume") == 0.0f)
		soundVolume = 1.0f;
	else
		soundVolume = Cvar_VariableValue("bgmvolume") / Cvar_VariableValue("volume");
	PlaySound("fvox/HEV_MEDKIT.wav", soundVolume);
}

void CMenuVolumeSlider::PreviewSuitVolume( void )
{
	float soundVolume;

	if (Cvar_VariableValue("volume") == 0.0f)
		soundVolume = 1.0f;
	else
		soundVolume = Cvar_VariableValue("suitvolume") / Cvar_VariableValue("volume");
	PlaySound("fvox/online.wav", soundVolume);
}

void CMenuVolumeSlider::Select( void )
{
	float	value;

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

		CMenuVolumeSlider::PreviewMusicVolume();
	}
	else if (m_id == 0xed)
	{
		value = (float)*m_piValue * (float)*m_piToggle / (float)(m_nSteps - 1);
		Cvar_SetValue("suitvolume", value);
		Cvar_VariableValue("suitvolume");

		CMenuVolumeSlider::PreviewSuitVolume();
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

	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

void CMenuVolumeSlider::Down( void )
{
	CMenu*	pMenu;

	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
}

void CMenuVolumeSlider::Left( void )
{
	float	value;

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
			CMenuVolumeSlider::PreviewMusicVolume();
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			CMenuVolumeSlider::PreviewSuitVolume();
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
			CMenuVolumeSlider::PreviewMusicVolume();
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			CMenuVolumeSlider::PreviewSuitVolume();
		}
	}
}

void CMenuVolumeSlider::Right( void )
{
	float	value;

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
			CMenuVolumeSlider::PreviewMusicVolume();
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piValue / (float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			CMenuVolumeSlider::PreviewSuitVolume();
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
			CMenuVolumeSlider::PreviewMusicVolume();
		}
		else if (m_id == 0xed)
		{
			value = (float)*m_piToggle * (float)*m_piValue /
				(float)(m_nSteps - 1);
			Cvar_SetValue("suitvolume", value);
			Cvar_VariableValue("suitvolume");
			CMenuVolumeSlider::PreviewSuitVolume();
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
	float		x;
	float		barY;
	char		*psz;
	int			width;
	float		x1, x2, y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{
		psz = m_pszLabel;
		if (g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(m_x - 15);
		x2 = (float)(m_x + width + 15);
		y1 = (float)(m_y - 14);
		y2 = (float)(m_y + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_SetHudDepth(4.0f);
		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));
	}

	if (m_pszLabel)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszLabel, m_x, m_y,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), 0);
	}

	if (bSelected && m_pszDescription)
		Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);

	DCV_SetHudDepth(2.0f);
	m_pMenu->SetColor(255, 144, 0, 128);
	DCV_TexState_Blend();

	barY = (float)m_y;
	m_pMenu->DrawMenuElementQuad2(1, 260.0f, (float)m_y,
		290.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 290.0f, (float)m_y,
		320.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 320.0f, (float)m_y,
		350.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 350.0f, (float)m_y,
		380.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 380.0f, (float)m_y,
		410.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 410.0f, (float)m_y,
		440.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 440.0f, (float)m_y,
		470.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 470.0f, (float)m_y,
		500.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 500.0f, (float)m_y,
		530.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(0, 530.0f, (float)m_y,
		560.0f, (float)(m_y + 28));
	m_pMenu->DrawMenuElementQuad2(2, 560.0f, (float)m_y,
		590.0f, (float)(m_y + 28));

	m_pMenu->SetColor(255, 144, 0, 192);
	DCV_SetHudDepth(3.0f);
	x = (float)(MENU_SENSITIVITY_THUMB_LEFT +
		*m_piValue * MENU_SENSITIVITY_THUMB_STEP);
	m_pMenu->DrawMenuElementBox(x, barY - 4.0f,
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


int CMenuPresetItem::GetCalloutWidth( void )
{
	if (sv_language.value != 0.0f)
		return 130;
	return 100;
}

int CMenuPresetItem::GetLeftX( void )
{
	if (sv_language.value != 0.0f)
		return 300;
	return 260;
}

int CMenuPresetItem::GetRightX( void )
{
	if (sv_language.value != 0.0f)
		return 488;
	return 528;
}

__forceinline void CMenuPresetItem::DrawCalloutBackground( int x, int y, float flFade )
{
	int width;
	int* color;

	DCV_SetHudDepth(2.5f);
	DCV_TexState_Blend();
	color = m_pMenu->m_state.rgba;
	if (m_iAlias == 0)
	{
		color[0] = 50;
		color[1] = 30;
		color[2] = 0;
		color[3] = (int)(flFade * 250.0f);
		DCV_SetColor(color[0], color[1], color[2], color[3]);
	}
	else
	{
		color[0] = 20;
		color[1] = 20;
		color[2] = 50;
		color[3] = (int)(flFade * 250.0f);
		DCV_SetColor(color[0], color[1], color[2], color[3]);
	}
	width = GetCalloutWidth();
	m_pMenu->DrawMenuElementBox((float)(x - 5), (float)(y - 5),
		(float)(x + width + 8), (float)(y + 23));
}

void CMenuPresetItem::DrawCalloutBox( int x, int y, float flFade )
{
	DrawCalloutBackground(x, y, flFade);
}


__forceinline void CMenuPresetItem::DrawLeftBinding( char *key, char *shiftedKey, int y, float flFade )
{
	char *binding;
	float brightness = 254.0f;

	if (m_iAlias == 0)
		binding = LookupAlias(key, m_ppAliases, &m_nAliases, 0);
	else
	{
		char *shift = "%shift";
		binding = LookupAlias(shiftedKey, m_ppAliases, &m_nAliases, 0);
		for (int i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(shift, g_pLangTags[i].tag))
			{
				shift = g_pLangTags[i].string;
				break;
			}
		}
		if (!strcmp(binding, shift))
			brightness = (coss(m_pMenu->m_state.flTime * 5.23f) + 1.0f) * 80.0f + 94.0f;
		if (!strcmp(binding, shiftedKey))
		{
			binding = "%bind_none";
			for (int i = 0; i < g_nLangTags; i++)
			{
				if (!strcmp(binding, g_pLangTags[i].tag))
				{
					binding = g_pLangTags[i].string;
					break;
				}
			}
		}
	}

	int calloutWidth = GetCalloutWidth();
	int x = GetLeftX();
	Text_DrawStringRight(0.7f, 0.93331f, binding, x, y,
		(int)(flFade * brightness), m_iAlias, calloutWidth);
	int boxX = GetLeftX() - GetCalloutWidth();
	DrawCalloutBackground(boxX, y, flFade);
}

__forceinline void CMenuPresetItem::DrawRightBinding( char *key, char *shiftedKey, int y, float flFade )
{
	char *binding;
	float brightness = 254.0f;

	if (m_iAlias == 0)
		binding = LookupAlias(key, m_ppAliases, &m_nAliases, 0);
	else
	{
		char *shift = "%shift";
		binding = LookupAlias(shiftedKey, m_ppAliases, &m_nAliases, 0);
		for (int i = 0; i < g_nLangTags; i++)
		{
			if (!strcmp(shift, g_pLangTags[i].tag))
			{
				shift = g_pLangTags[i].string;
				break;
			}
		}
		if (!strcmp(binding, shift))
			brightness = (coss(m_pMenu->m_state.flTime * 5.23f) + 1.0f) * 80.0f + 94.0f;
		if (!strcmp(binding, shiftedKey))
		{
			binding = "%bind_none";
		}
	}

	int calloutWidth = GetCalloutWidth();
	int x = GetRightX();
	Text_DrawStringLeft(0.7f, 0.93331f, binding, x, y,
		(int)(flFade * brightness), m_iAlias, calloutWidth);
	int boxX = GetRightX();
	DrawCalloutBackground(boxX, y, flFade);
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
	char	*psz;
	int		width;
	float	x1, x2;
	float	y1, y2;

	DCV_TexState_Blend();

	if (bSelected)
	{

		psz = m_pszLabel;
		if (psz && g_nLangTags > 0)
		{
			psz = Text_LocalizeString(psz);
		}

		width = Font_MeasureString((dcfont_t *)draw_chars, (byte *)psz);

		if (m_align == 0)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 100.0f));
		else
			m_pMenu->SetColor(95, 95, 255, (int)(flFade * 100.0f));
		DCV_SetHudDepth(2.0f);
		DCV_TexState_Additive();

		x1 = (float)(m_labelX - 15);
		x2 = (float)(m_labelX + width + 15);
		y1 = (float)(m_labelY - 14);
		y2 = (float)(m_labelY + 40);
		m_pMenu->DrawElementTile(x1, y1, x2, y2);

		DCV_TexState_Blend();
		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 120.0f));

		m_pMenu->SetColor(255, 255, 255, (int)(flFade * 255.0f));

		m_pMenu->DrawControllerIcon(266.0f, 120.0f, 522.0f, 376.0f);

		// the lines that run from each button out to its caption
		DCV_SetHudDepth(2.3f);
		m_pMenu->SetColor(255, 255, 255, (int)(flFade * 220.0f));

		m_pMenu->RenderControllerLines(266.0f, 120.0f, 522.0f, 376.0f);

		Font_ApplyScale(0.7f, 0.93331f);
		Text_DrawStringShadow("", 0, 0, 0, 0);

		DrawLeftBinding("AUX6", "S1AUX6", 125, flFade);

		{
			float brightness = 254.0f;
			int x, boxX, calloutWidth;
			psz = "%look";
			calloutWidth = GetCalloutWidth();
			x = GetLeftX();
			Text_DrawStringRight(0.7f, 0.93331f, psz, x, 173,
				(int)(flFade * brightness), m_iAlias, calloutWidth);

			boxX = (GetLeftX()) - (GetCalloutWidth());
			DrawCalloutBackground(boxX, 173, flFade);
		}

		DrawLeftBinding("AUX4", "S1AUX4", 221, flFade);

		DrawLeftBinding("AUX1", "S1AUX1", 253, flFade);

		DrawLeftBinding("AUX3", "S1AUX3", 285, flFade);

		DrawLeftBinding("AUX2", "S1AUX2", 317, flFade);

		{
			float brightness = 254.0f;
			int x, boxX, calloutWidth;
			psz = "%pauseshort";
			calloutWidth = GetCalloutWidth();
			x = 396;
			Text_DrawStringCentered(0.7f, 0.93331f, psz, x, 387,
				(int)(flFade * brightness), m_iAlias, calloutWidth);

			boxX = 396 - (GetCalloutWidth()) / 2;
			DrawCalloutBackground(boxX, 387, flFade);
		}

		DrawRightBinding("AUX5", "S1AUX5", 125, flFade);

		DrawRightBinding("JOY4", "S1JOY4", 173, flFade);

		DrawRightBinding("JOY2", "S1JOY2", 221, flFade);

		DrawRightBinding("JOY1", "S1JOY1", 269, flFade);

		DrawRightBinding("JOY3", "S1JOY3", 317, flFade);
	}

	if (m_pszLabel)
	{
		Font_ApplyScale(m_flLabelScale, m_flLabelAspect);

		Text_DrawStringShadow(m_pszLabel, m_labelX, m_labelY,
			(int)(flFade * (bSelected ? 255.0f : 128.0f)), m_align);
	}

	if (bSelected && m_pszDescription)
	{
		Text_DrawCenteredStatus((byte *)m_pszDescription, m_pMenu->m_state.flTime,
			(int)(flFade * 192.0f), m_flDescScale, m_flDescAspect);
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

	m_iAlias = 0;
	pMenu = m_pMenu;
	pMenu->MoveSelection(-1);
}

void CMenuPresetItem::Down( void )
{
	CMenu*	pMenu;

	m_iAlias = 0;
	pMenu = m_pMenu;
	pMenu->MoveSelection(1);
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
	: CMenuItemBase(pMenu)
{
	m_nEntries = CMenuBindItem::BuildControlList(IN_KeyboardActive(), IN_JoystickActive());
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
		m_nEntries = CMenuBindItem::BuildControlList(IN_KeyboardActive(), IN_JoystickActive());
		m_reserved18 = 0;
	}

#if HLDC_MP
	if (g_pszBindFocus)
	{
		for (control = 0; control < m_nEntries; control++)
		{
			if (!strcmp(g_ControlActions[g_ControlKeys[control].control].pszCommand, g_pszBindFocus))
			{
				m_mode = control + 6;
				m_selection = min(m_mode, max(0, m_nEntries - 2));
				break;
			}
		}
		g_pszBindFocus = NULL;
	}
#endif
	if (m_mode > m_nEntries + 6)
		m_mode = m_nEntries + 6;

	DCV_SetHudDepth(2.0f);
	DCV_TexState_Blend();

	alpha = (int)(flFade * 255.0f);
	Text_DrawStringLeft(0.7f, 0.93331f, "%custom_action", 170, 134,
		alpha, 0, 190);
	Text_DrawStringLeft(0.7f, 0.93331f, "%keybutton", 370, 134,
		alpha, 0, 230);

	m_pMenu->SetColor(255, 144, 0, (int)(flFade * 128.0f));
	y = 164;
	for (slot = 0; slot < 8; slot++, y += 30)
	{
		row = m_selection + slot;
		if (row == m_mode && m_capturing)
		{
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 240.0f));
			m_pMenu->DrawMenuElementBox(348.0f, (float)(y - 4),
				604.0f, (float)(y + 24));
		}

		if (row == m_mode)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 198.0f));

		DCV_SetHudDepth(2.0f);
		if (row == 0 || row == 3 || row == 5)
			m_pMenu->SetColor(255, 144, 0, (int)(flFade * 70.0f));
		m_pMenu->DrawMenuElementBox(160.0f, (float)(y - 4),
			610.0f, (float)(y + 24));
		DCV_SetHudDepth(3.0f);

		if (row >= 6)
		{
			if (row - 6 < m_nEntries)
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
					pszKey = CMenuBindItem::TranslateKeyName(Key_KeynumToString(key));
				}

				Text_DrawStringLeft(0.7f, 0.93331f, pszKey, 370, y,
					alpha, 0, 230);
			}
		}
		else
		{
			switch (row)
			{
			case 0:
				Text_DrawStringCentered(0.7f, 0.93331f, "%axis_options", 385, y,
					alpha, 0, 400);
				break;
			case 1:
				Text_DrawStringLeft(0.7f, 0.93331f, "%up_down_axis", 170, y,
					alpha, 0, 190);
				Text_DrawStringLeft(0.7f, 0.93331f, g_pszAxisActions[m_leftValue], 370, y,
					alpha, 0, 230);
				break;
			case 2:
				Text_DrawStringLeft(0.7f, 0.93331f, "%left_right_axis", 170, y,
					alpha, 0, 190);
				Text_DrawStringLeft(0.7f, 0.93331f, g_pszAxisActions[m_rightValue], 370, y,
					alpha, 0, 230);
				break;
			case 3:
				Text_DrawStringCentered(0.7f, 0.93331f, "%shift_things", 385, y,
					alpha, 0, 400);
				break;
			case 4:
				Text_DrawStringLeft(0.7f, 0.93331f, "%shift", 170, y,
					alpha, 0, 190);

				pszKey = CMenuBindItem::TranslateKeyName(joyshift1.string);

				Text_DrawStringLeft(0.7f, 0.93331f, pszKey, 370, y,
					alpha, 0, 230);
				break;
			case 5:
				Text_DrawStringCentered(0.7f, 0.93331f, "%key_bindings", 385, y,
					alpha, 0, 400);
				break;

			}		}

		m_pMenu->SetColor(255, 144, 0, (int)(flFade * 128.0f));
	}

	if (m_selection > 0)
		m_pMenu->DrawMenuElementQuad(3, 520.0f, 138.0f, 590.0f, 158.0f);

	if (m_selection < m_nEntries - 2)
		m_pMenu->DrawMenuElementQuad(4, 520.0f, 401.0f, 590.0f, 421.0f);

	capturedKey = Key_GetCapturedKey();
	if (capturedKey)
	{
		m_capturing = 0;
		m_reserved18 = 1;
		Key_SetCaptureMode(0);

		control = g_ControlKeys[m_mode - 6].control;
		pszCommand = g_ControlActions[control].pszCommand;
		if (pszCommand && strcmp(pszCommand, "(null)")
			&& capturedKey != K_AUX7 && capturedKey != LAST_JOYSHIFT1)
		{
			sprintf(bindCommand, "bind \"%s\" \"%s\"\n",
				Key_KeynumToString(capturedKey), pszCommand);
			Cbuf_AddText(bindCommand);
#if HLDC_MP
			key_dest = key_ui;
#else
			key_dest = key_menu;
#endif
		}
	}

	if (m_mode < 5)
	{
		if (m_capturing)
			Text_DrawCenteredStatus((byte *)"%lr_choose_function", m_pMenu->m_state.flTime,
				190, 1.0f, 1.3333f);
		else
			Text_DrawCenteredStatus((byte *)"%a_change_function", m_pMenu->m_state.flTime,
				190, 1.0f, 1.3333f);
	}
	else if (m_capturing)
		Text_DrawCenteredStatus((byte *)"%but_set_function", m_pMenu->m_state.flTime,
			190, 1.0f, 1.3333f);
	else
		Text_DrawCenteredStatus((byte *)"%a_change_function", m_pMenu->m_state.flTime,
			190, 1.0f, 1.3333f);
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
	char command[64];
	char binding[256];

	if (!m_capturing)
	{
		sprintf(command, "joyadvancedupdate");
		m_pMenu->ExecuteCommand(command, 0, 0);
		Host_WriteConfiguration();
		m_pMenu->Cancel();
	}
	else
	{
		if (m_mode == 4)
		{
			sprintf(binding, "bind \"%s\" \"\"\n", joyshift1.string);
			Cbuf_AddText(binding);
			m_reserved18 = 1;
		}
		m_capturing = 0;
	}
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

int CMenuBindItem::FindShiftKey( char* pszName )
{
	int i;

	for (i = 0; i < g_nShiftKeys; i++)
	{
		if (!Q_strcasecmp(pszName, g_pszShiftKeys[i]))
			return i;
	}
	return 0;
}

char* CMenuBindItem::GetShiftKey( int key )
{
	while (key < 0)
		key += g_nShiftKeys;
	return g_pszShiftKeys[key % g_nShiftKeys];
}

void CMenuBindItem::Left( void )
{
	if (!m_capturing)
		return;

	if (m_mode == 1)
	{
		m_leftValue += JOY_AXIS_ACTIONS - 1;
		m_leftValue %= JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisy", (float)m_leftValue);
	}
	if (m_mode == 2)
	{
		m_rightValue += JOY_AXIS_ACTIONS - 1;
		m_rightValue %= JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisx", (float)m_rightValue);
	}
	if (m_mode == 4)
		Cvar_Set("joyshift1", CMenuBindItem::GetShiftKey(CMenuBindItem::FindShiftKey(joyshift1.string) - 1));
}

void CMenuBindItem::Right( void )
{
	if (!m_capturing)
		return;

	if (m_mode == 1)
	{
		m_leftValue += 1;
		m_leftValue %= JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisy", (float)m_leftValue);
	}
	if (m_mode == 2)
	{
		m_rightValue += 1;
		m_rightValue %= JOY_AXIS_ACTIONS;
		Cvar_SetValue("joyadvaxisx", (float)m_rightValue);
	}
	if (m_mode == 4)
		Cvar_Set("joyshift1", CMenuBindItem::GetShiftKey(CMenuBindItem::FindShiftKey(joyshift1.string) + 1));
}

int CMenuBindItem::IsActive( void )
{
	return 1;
}

/*
==================
CMenu::ClearTextures

Forget every artwork slot without touching what is in the texture cache.
==================
*/
void CMenu::ClearTextures( void )
{
	int		i;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		m_state.pTextureNames[i] = NULL;
		m_state.iTextures[i] = 0;
	}
}

/*
==================
CMenu::FreeTextures

Hand the page's artwork back to the texture cache.
==================
*/
void CMenu::FreeTextures( void )
{
	int		i;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (m_state.pTextureNames[i])
			DC_ForceFreeTextureByName(m_state.pTextureNames[i]);

		m_state.iTextures[i] = 0;
	}
}

/*
==================
CMenu::LoadMenuTexture

Look the artwork up in the page's own slots, and pull it off the disc into a
free one if it is not there yet.
==================
*/
int CMenu::FindFreeTextureSlot( void )
{
	int i;
	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (!m_state.pTextureNames[i])
			return i;
	}
	return -1;
}

int CMenu::FindTexture( char* pszName )
{
	int i;
	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (m_state.pTextureNames[i]
			&& !strcmp(m_state.pTextureNames[i], pszName))
			return m_state.iTextures[i];
	}
	return 0;
}

int CMenu::LoadMenuTexture( char* pszName )
{
	pvrheader_t*	pHeader;
	int				texture;
	int				slot;
	int				length;
	int				i;

	texture = 0;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (m_state.pTextureNames[i]
			&& !strcmp(m_state.pTextureNames[i], pszName))
		{
			texture = m_state.iTextures[i];
			break;
		}
	}

	if (texture)
		return texture;

	slot = -1;

	for (i = 0; i < MAX_MENU_TEXTURES; i++)
	{
		if (!m_state.pTextureNames[i])
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
		return 0;

	m_state.iTextures[slot] = 0;

	pHeader = (pvrheader_t *)COM_LoadTempFile(pszName, &length);
	if (pHeader)
	{
		m_state.iTextures[slot] = DC_LoadTexture(pszName, GLT_WORLD,
			pHeader->width, pHeader->height, pHeader, FALSE, TEX_TYPE_GBIX, NULL);
		m_state.pTextureNames[slot] = pszName;
	}

	// A failed lookup did not create a temporary block.
	if (pHeader)
		COM_FreeTempFile();

	return m_state.iTextures[slot];
}

/*
==================
CMenu::DrawMenuElementBox

The controller page uses a small nine-slice frame cut from the menu element
sheet. Thin boxes take the whole tile; larger boxes keep the corners fixed
and stretch the middle strips.
==================
*/
void CMenu::DrawMenuElementTile( float x0, float y0, float x1, float y1 )
{
	DrawElementTile(x0, y0, x1, y1);
}


void CMenu::DrawMenuElementTile2( float x0, float y0, float x1, float y1 )
{
	DrawElementTile2(x0, y0, x1, y1);
}

void CMenu::DrawMenuElementBox( float x0, float y0, float x1, float y1 )
{
	float	left, right, top, bottom;
	int		base;

	m_state.iElementTexture = LoadMenuTexture( "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_state.iElementTexture, 0);

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
CMenu::DrawMenuElementQuad

Put one of the four turns of the element strip into the given rectangle.
==================
*/
void CMenu::DrawMenuElementQuad( int orientation, float x0, float y0, float x1, float y1 )
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

	m_state.iElementTexture = LoadMenuTexture( "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_state.iElementTexture, 0);

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
CMenu::DrawMenuElementQuad2

The six pieces the item frames are built out of: the four corners and the two
end caps, all cut from the same strip.
==================
*/
void CMenu::DrawMenuElementQuad2( int corner, float x0, float y0, float x1, float y1 )
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

	m_state.iElementTexture = LoadMenuTexture( "gfx/menu_elements_alpha.pvr");
	GL_BindStage(m_state.iElementTexture, 0);

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
CMenu::DrawControllerIcon

Put the controller picture in the given rectangle. The in-game page has room
for the bigger one.
==================
*/
void CMenu::DrawControllerIcon( float x0, float y0, float x1, float y1 )
{
	int		base;

	if (cls.state == ca_active)
		m_state.iControllerTexture = LoadMenuTexture( "gfx/menu_controller_128.pvr");
	else
		m_state.iControllerTexture = LoadMenuTexture( "gfx/menu_controller.pvr");

	GL_BindStage(m_state.iControllerTexture, 0);

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
CMenu::DrawGordonIcon

Freeman's portrait, dropping back to the full size picture when the small one
is not on the disc.
==================
*/
void CMenu::DrawGordonIcon( float x0, float y0, float x1, float y1 )
{
	int		base;

	m_state.iGordonTexture = LoadMenuTexture( "gfx/menu_gordon_32.pvr");
	if (!m_state.iGordonTexture)
		m_state.iGordonTexture = LoadMenuTexture( "gfx/menu_gordon.pvr");

	GL_BindStage(m_state.iGordonTexture, 0);

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
CMenu::DrawBarneyIcon
==================
*/
void CMenu::DrawBarneyIcon( float x0, float y0, float x1, float y1 )
{
	int		base;

	m_state.iBarneyTexture = LoadMenuTexture( "gfx/menu_barney_32.pvr");
	if (!m_state.iBarneyTexture)
		m_state.iBarneyTexture = LoadMenuTexture( "gfx/menu_barney.pvr");

	GL_BindStage(m_state.iBarneyTexture, 0);

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
CMenu::FadeIn

Start the page fading up from nothing.
==================
*/
void CMenu::FadeIn( void )
{
	m_state.flAnimating = 1.0f;
	m_state.flFade = 0;
	m_state.flFadeFrom = 0;
	m_state.flFadeTo = 1.0f;
	m_state.flAnimStart = Sys_FloatTime();
}

/*
==================
CMenu::FadeOut

Start the page fading away.
==================
*/
void CMenu::FadeOut( void )
{
	m_state.flAnimating = 1.0f;
	m_state.flFade = 1.0f;
	m_state.flFadeFrom = 1.0f;
	m_state.flFadeTo = 0;
	m_state.flAnimStart = Sys_FloatTime();
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

	ClearTextures();

	Build( pszMenu);

	FadeIn();

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
#if HLDC_MP
	if (!strncmp(m_szName, "mp", 2) || !strcmp(m_szName, "multiplayer"))
	{
		for (int i = 0; i < MAX_MENU_ITEMS; i++)
			delete m_pItems[i];
	}
#endif
	FreeTextures();

	UI_Deactivate();

	if (!m_state.iSoundBlocked)
		S_UnblockSound();
}

/*
==================
CMenu::AnimateLerp

Slide the page's fade value from where it started towards where it is going,
and stop once the time is up.
==================
*/
void CMenu::AnimateLerp( void )
{
	float	frac;

	if (m_state.flTime - m_state.flAnimStart > m_state.flAnimTime)
	{
		m_state.flFade = m_state.flFadeTo;
		m_state.flAnimating = 0;
		return;
	}

	frac = (m_state.flTime - m_state.flAnimStart) / m_state.flAnimTime;
	m_state.flFade = m_state.flFadeFrom
		+ frac * (m_state.flFadeTo - m_state.flFadeFrom);
}

/*
==================
CMenu::CheckCheatCode

Feed one button into both secret sequences. Getting to the end of either
one unlocks what it guards.
==================
*/
void CMenu::CheckCheatCode( char button )
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
		gpActiveMenu->Draw();
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

	gpActiveMenu->Input();

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
#if HLDC_MP
		if (!strcmp(pszMenu, "mpsetup"))
			Host_WriteConfiguration();
		if (!strcmp(pszMenu, "multiplayer"))
		{
			if (!keybindings['t'] || !keybindings['t'][0])
				Key_SetBinding('t', "impulse 201");
			if (!keybindings[K_TAB] || !keybindings[K_TAB][0])
				Key_SetBinding(K_TAB, "+showscores");
			if (!keybindings['y'] || !keybindings['y'][0])
				Key_SetBinding('y', "messagemode");
		}
#endif
		Cache_FlushToDisk();
		Cache_FreeAll();

		pMenu = new CMenu(pszMenu);
		gpActiveMenu = pMenu;
	}
}

/*
==================
CMenu::Build

Fill the page with the items its script asks for. Most of them stack down the
page a line at a time; the passcode lists and the save slots keep counts of
their own so they can carry on where the last one left off.
==================
*/
menuitemdef_t* CMenu::FindItemDef( int id )
{
	unsigned int i;
	for (i = 0; i < sizeof(g_MenuItems) / sizeof(g_MenuItems[0]); i++)
	{
		if (g_MenuItems[i].id == id)
			return &g_MenuItems[i];
	}
	return NULL;
}

menuoption_t* CMenu::FindOptionDef( int id )
{
	unsigned int i;
	for (i = 0; i < sizeof(g_MenuOptions) / sizeof(g_MenuOptions[0]); i++)
	{
		if (g_MenuOptions[i].id == id)
			return &g_MenuOptions[i];
	}
	return NULL;
}

menuslider_t* CMenu::FindSliderDef( int id )
{
	unsigned int i;
	for (i = 0; i < sizeof(g_MenuSliders) / sizeof(g_MenuSliders[0]); i++)
	{
		if (g_MenuSliders[i].id == id)
			return &g_MenuSliders[i];
	}
	return NULL;
}

menupage_t* CMenu::FindPage( char* pszName )
{
	unsigned int i;
	for (i = 0; i < sizeof(g_MenuPages) / sizeof(g_MenuPages[0]); i++)
	{
		if (!strcmp(pszName, g_MenuPages[i].pszName))
			return &g_MenuPages[i];
	}
	return NULL;
}

#if HLDC_MP
#define MP_BROWSER_ROWS 4
#define MP_MENU_ADDRESS 0x110
#define MP_MENU_PLAYER 0x111
#define MP_MENU_CHAT 0x112

static cvar_t mp_server = { "mp_server", "", FCVAR_ARCHIVE };
static cvar_t mp_spray = { "mp_spray", "", FCVAR_ARCHIVE };
extern char message_type[32];

class CMenuMultiplayerItem : public CMenuItemBase
{
public:
	CMenuMultiplayerItem( CMenu* menu, int page );
	virtual ~CMenuMultiplayerItem( void );
	virtual void Draw( float fade, qboolean selected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int IsActive( void ) { return 1; }
	qboolean Key( int key );
	qboolean IsEditing( void ) { return m_editing; }
	void Box( int x, int y, int width, int height, int alpha );

private:
	void Text( const char* text, int x, int y, int width, int alpha );
	void DrawKeyboard( float fade );
	void KeyboardSelect( void );
	void AddModel( const char* name );
	static void CollectModel( void* context, const char* path );
	void Model( int direction );
	void LoadPreview( void );
	int m_modelCount;
	int m_model;
	char (*m_models)[32];
	char m_previewName[MAX_QPATH];
	int m_preview;
	qboolean m_osk;
	qboolean m_uppercase;
	int m_oskKey;
	void Row( const char* label, const char* value, int row, int y, float fade );
	void Move( int direction );
	void Edit( const char* value, int limit );
	void Accept( void );
	void Character( int direction );
	void Spray( int direction );
	void Join( const char* address );
	int m_page;
	int m_row;
	int m_firstServer;
	int m_cursor;
	int m_limit;
	qboolean m_editing;
	char m_text[128];
	char m_status[96];
};

static CMenuMultiplayerItem* g_pMultiplayerItem;
static CMenuMultiplayerItem* g_pChatKeyboard;

void UI_MultiplayerInit( void )
{
	Cvar_RegisterVariable(&mp_server);
	Cvar_RegisterVariable(&mp_spray);
}

qboolean UI_MultiplayerKeyEvent( int key )
{
	if (!gpActiveMenu || !gfDrawMenu || gpActiveMenu->m_state.flAnimating)
		return FALSE;
	return g_pMultiplayerItem ? g_pMultiplayerItem->Key(key) : FALSE;
}

CMenuMultiplayerItem::CMenuMultiplayerItem( CMenu* menu, int page )
	: CMenuItemBase(menu)
{
	m_page = page;
	m_row = 0;
	m_firstServer = 0;
	m_cursor = 0;
	m_limit = sizeof(m_text) - 1;
	m_editing = FALSE;
	m_osk = FALSE;
	m_uppercase = TRUE;
	m_oskKey = 0;
	m_modelCount = 1;
	m_model = 0;
	m_models = (char (*)[32])malloc(32);
	if (!m_models)
		Sys_Error("Out of memory listing player models");
	m_models[0][0] = 0;
	m_preview = 0;
	m_previewName[0] = 0;
	m_text[0] = 0;
	m_status[0] = 0;
	if (menu)
		g_pMultiplayerItem = this;
	if (page == MP_MENU_PLAYER)
	{
		COM_EnumeratePlayerFiles(CollectModel, this);
		if (!decal_wad && COM_FileSize("decals.wad") > 0)
			Decal_Init();
		LoadPreview();
	}
	else if (page == MP_MENU_CHAT)
		Edit("", 120);
}

CMenuMultiplayerItem::~CMenuMultiplayerItem( void )
{
	free(m_models);
	if (g_pMultiplayerItem == this)
		g_pMultiplayerItem = NULL;
	if (m_preview)
		DC_ForceFreeTextureByName(m_previewName);
}


void CMenuMultiplayerItem::CollectModel( void* context, const char* path )
{
	char name[32];
	const char* start;
	const char* end;
	const char* extension;
	int length;

	if (Q_strncasecmp(path, "models/player/", 14))
		return;
	extension = strrchr(path, '.');
	if (!extension || (Q_strcasecmp(extension, ".mdl") && Q_strcasecmp(extension, ".bmp")))
		return;
	start = path + 14;
	end = strchr(start, '/');
	if (!end)
		end = strrchr(start, '.');
	if (!end)
		return;
	length = end - start;
	if (length < 1 || length >= sizeof(name))
		return;
	memcpy(name, start, length);
	name[length] = 0;
	((CMenuMultiplayerItem*)context)->AddModel(name);
}

void CMenuMultiplayerItem::AddModel( const char* name )
{
	char path[MAX_QPATH];
	int i;
	char (*models)[32];

	if (!name[0] || strlen(name) >= sizeof(m_models[0]))
		return;
	for (i = 0; name[i]; i++)
		if (!(name[i] >= 'a' && name[i] <= 'z') && !(name[i] >= 'A' && name[i] <= 'Z') &&
			!(name[i] >= '0' && name[i] <= '9') && name[i] != '_' && name[i] != '-')
			return;
	for (i = 1; i < m_modelCount; i++)
		if (!Q_strcasecmp(name, m_models[i]))
			return;
	if (strlen(name) * 2 + 19 >= sizeof(path))
		return;
	sprintf(path, "models/player/%s/%s.mdl", name, name);
	if (COM_FileSize(path) < 0)
		return;
	models = (char (*)[32])realloc(m_models, (m_modelCount + 1) * sizeof(m_models[0]));
	if (!models)
		return;
	m_models = models;
	strcpy(m_models[m_modelCount], name);
	if (!Q_strcasecmp(name, Cvar_VariableString("model")))
		m_model = m_modelCount;
	m_modelCount++;
}

void CMenuMultiplayerItem::Model( int direction )
{
	m_model = (m_model + direction + m_modelCount) % m_modelCount;
	Cvar_Set("model", m_models[m_model]);
	LoadPreview();
}

void CMenuMultiplayerItem::LoadPreview( void )
{
	byte* data;
	byte* source;
	unsigned short* pixels;
	int length, offset, width, height, compression, stride, x, y, row;
	unsigned short bits, planes;
	char* name;

	if (m_preview)
		DC_ForceFreeTextureByName(m_previewName);
	m_preview = 0;
	name = m_models[m_model][0] ? m_models[m_model] : "gordon";
	sprintf(m_previewName, "models/player/%s/%s.bmp", name, name);
	data = COM_LoadTempFile(m_previewName, &length);
	if (!data)
		return;
	if (length < 54 || data[0] != 'B' || data[1] != 'M')
	{
		COM_FreeTempFile();
		return;
	}
	memcpy(&offset, data + 10, 4);
	memcpy(&width, data + 18, 4);
	memcpy(&height, data + 22, 4);
	memcpy(&planes, data + 26, 2);
	memcpy(&bits, data + 28, 2);
	memcpy(&compression, data + 30, 4);
	if (planes != 1 || bits != 24 || compression || width < 1 || width > 256 ||
		height == 0 || height < -256 || height > 256 || (width & (width - 1)) ||
		(abs(height) & (abs(height) - 1)) || offset < 54)
	{
		COM_FreeTempFile();
		return;
	}
	stride = (width * 3 + 3) & ~3;
	if (offset > length || stride * abs(height) > length - offset)
	{
		COM_FreeTempFile();
		return;
	}
	pixels = (unsigned short*)malloc(width * abs(height) * 2);
	if (pixels)
	{
		for (y = 0; y < abs(height); y++)
		{
			row = height > 0 ? height - 1 - y : y;
			source = data + offset + row * stride;
			for (x = 0; x < width; x++, source += 3)
				pixels[y * width + x] = ((source[2] >> 3) << 11) |
					((source[1] >> 2) << 5) | (source[0] >> 3);
		}
		m_preview = DC_LoadTexture(m_previewName, GLT_SYSTEM, width, abs(height),
			pixels, FALSE, TEX_TYPE_RGB565_RAW, NULL);
		free(pixels);
	}
	COM_FreeTempFile();
}

void CMenuMultiplayerItem::DrawKeyboard( float fade )
{
	static char* actions[] = { "SPACE", "DEL", "<", ">", "CASE", "DONE", "BACK", "@", "-", "_" };
	const char* letters = "1234567890QWERTYUIOPASDFGHJKL_ZXCVBNM.-:";
	char key[2];
	int i, x, y;

	Text(m_page == MP_MENU_CHAT ? "CHAT" : "ENTER TEXT", 56, 140, 530, (int)(fade * 255));
	Row(m_page == MP_MENU_PLAYER ? "PLAYER NAME" : m_page == MP_MENU_CHAT ? "MESSAGE" :
		m_row == 0 ? "SERVER" : "PASSWORD", "", m_row, 184, fade);
	for (i = 0; i < 50; i++)
	{
		x = 50 + (i % 10) * 54;
		y = 232 + (i / 10) * 34;
		DCV_TexState_Blend();
		Box(x, y, 50, 30, (int)(fade * (i == m_oskKey ? 160 : 40)));
		if (i < 40)
		{
			key[0] = m_uppercase ? letters[i] : tolower(letters[i]);
			key[1] = 0;
			Text(key, x + 19, y + 4, 35, (int)(fade * 255));
		}
		else
			Text(actions[i - 40], x + 3, y + 4, 46, (int)(fade * 255));
	}
	Text("A: TYPE   X: DELETE   B: CANCEL   Select DONE to accept", 56, 414, 530, (int)(fade * 200));
}

void CMenuMultiplayerItem::KeyboardSelect( void )
{
	const char* letters = "1234567890QWERTYUIOPASDFGHJKL_ZXCVBNM.-:";

	if (m_oskKey < 40)
	{
		Key(m_uppercase ? letters[m_oskKey] : tolower(letters[m_oskKey]));
		return;
	}
	switch (m_oskKey)
	{
	case 40: Key(' '); break;
	case 41: Key(K_BACKSPACE); break;
	case 42: if (m_cursor) m_cursor--; break;
	case 43: if (m_cursor < strlen(m_text)) m_cursor++; break;
	case 44: m_uppercase = !m_uppercase; break;
	case 45: Accept(); break;
	case 46: Cancel(); break;
	case 47: Key('@'); break;
	case 48: Key('-'); break;
	case 49: Key('_'); break;
	}
}

qboolean UI_OpenChatKeyboard( void )
{
	if (cls.state != ca_active || IN_KeyboardActive() || gfDrawMenu)
		return FALSE;
	delete g_pChatKeyboard;
	g_pChatKeyboard = new CMenuMultiplayerItem(NULL, MP_MENU_CHAT);
	memset(joymenubuttons, 0, MAX_MENU_BUTTONS * sizeof(joymenubuttons[0]));
	key_dest = key_message;
	return TRUE;
}

qboolean UI_ChatKeyboardActive( void )
{
	if (g_pChatKeyboard && (key_dest != key_message || cls.state != ca_active ||
		gfDrawMenu || !g_pChatKeyboard->IsEditing() || IN_KeyboardActive()))
	{
		delete g_pChatKeyboard;
		g_pChatKeyboard = NULL;
	}
	return g_pChatKeyboard != NULL;
}

qboolean UI_ChatKeyboardKeyEvent( int key )
{
	if (!UI_ChatKeyboardActive())
		return FALSE;
	if (key < K_JOY1)
		g_pChatKeyboard->Key(key);
	return TRUE;
}

void UI_DrawChatKeyboard( void )
{
	if (!UI_ChatKeyboardActive())
		return;
	if (joymenubuttons[7] || joymenubuttons[15]) g_pChatKeyboard->Up();
	if (joymenubuttons[6] || joymenubuttons[14]) g_pChatKeyboard->Down();
	if (joymenubuttons[5] || joymenubuttons[13]) g_pChatKeyboard->Right();
	if (joymenubuttons[4] || joymenubuttons[12]) g_pChatKeyboard->Left();
	if (joymenubuttons[8]) g_pChatKeyboard->Key(K_BACKSPACE);
	if (joymenubuttons[1]) g_pChatKeyboard->Cancel();
	else if (joymenubuttons[0]) g_pChatKeyboard->Select();
	memset(joymenubuttons, 0, MAX_MENU_BUTTONS * sizeof(joymenubuttons[0]));
	if (UI_ChatKeyboardActive())
	{
		Draw_FillRGBA(40, 132, 560, 310, 0, 0, 0, 160);
		g_pChatKeyboard->Draw(1.0f, TRUE);
	}
}

void CMenuMultiplayerItem::Box( int x, int y, int width, int height, int alpha )
{
	if (m_pMenu)
	{
		m_pMenu->SetColor(255, 144, 0, alpha);
		m_pMenu->DrawMenuElementBox((float)x, (float)y, (float)(x + width), (float)(y + height));
	}
	else
		Draw_FillRGBA(x, y, width, height, 255, 144, 0, alpha);
}

void CMenuMultiplayerItem::Text( const char* text, int x, int y, int width, int alpha )
{
	char line[96];
	int i;

	for (i = 0; text[i] && i < sizeof(line) - 1; i++)
		line[i] = ((byte)text[i] >= 32 && (byte)text[i] < 127) ? text[i] : ' ';
	line[i] = 0;
	Font_ApplyScale(0.65f, 0.86667f);
	while (i && Font_MeasureString((dcfont_t*)draw_chars, (byte*)line) > width)
		line[--i] = 0;
	DCV_TexState_Blend();
	Text_DrawStringShadow(line, x, y, alpha, 0);
}

void CMenuMultiplayerItem::Row( const char* label, const char* value, int row, int y, float fade )
{
	char text[130];
	int i, first;

	DCV_TexState_Blend();
	Box(48, y - 3, m_page == MP_MENU_PLAYER && row > 0 ? 362 : 544,
		25, (int)(fade * (row == m_row ? 100 : 35)));
	Text(label, 56, y, 180, (int)(fade * 255));
	if (m_editing && row == m_row)
	{
		strcpy(text, m_text);
		if (m_page == MP_MENU_ADDRESS && m_row == 1)
			for (i = 0; text[i]; i++)
				text[i] = '*';
		i = strlen(text);
		if (m_cursor == i)
		{
			text[i] = '_';
			text[i + 1] = 0;
		}
		else if (((int)(Sys_FloatTime() * 3) & 1) == 0)
			text[m_cursor] = '_';
		first = m_cursor > 24 ? m_cursor - 24 : 0;
		Text(text + first, 240, y, 340, (int)(fade * 255));
	}
	else if (value)
		Text(value, 240, y, m_page == MP_MENU_PLAYER && row > 0 ? 160 : 340,
			(int)(fade * (row == m_row ? 255 : 160)));
}

void CMenuMultiplayerItem::Draw( float fade, qboolean selected )
{
	const server_cache_t* server;
	char text[96];
	int i, index, count;
	texture_t* texture;
	int base;

	DCV_TexState_Blend();
	if (m_editing && m_osk)
	{
		DrawKeyboard(fade);
		return;
	}
	if (m_page == MP_MENU_ADDRESS)
	{
		Text("JOIN GAME", 56, 140, 530, (int)(fade * 255));
		Row("SERVER ADDRESS", mp_server.string, 0, 174, fade);
		Row("PASSWORD", Cvar_VariableString("password")[0] ? "********" : "NONE", 1, 206, fade);
		Row("REFRESH LAN", "Also query the address above", 2, 238, fade);
		Text("SERVER", 56, 267, 245, (int)(fade * 180));
		Text("MAP", 320, 267, 110, (int)(fade * 180));
		Text("PLAYERS", 435, 267, 75, (int)(fade * 180));
		Text("PING", 533, 267, 55, (int)(fade * 180));
		count = CL_ServerListCount();
		for (i = 0; i < MP_BROWSER_ROWS; i++)
		{
			index = m_firstServer + i;
			server = CL_ServerListEntry(index);
			DCV_TexState_Blend();
			m_pMenu->SetColor(255, 144, 0, (int)(fade * (m_row == i + 3 ? 100 : 30)));
			m_pMenu->DrawMenuElementBox(48, (float)(287 + i * 22), 592, (float)(308 + i * 22));
			if (!server)
			{
				if (!i && !count)
					Text("No servers found. Refresh or enter an address.", 56, 290, 520, (int)(fade * 180));
				continue;
			}
			Text(server->name, 56, 290 + i * 22, 250, (int)(fade * 255));
			Text(server->map, 320, 290 + i * 22, 110, (int)(fade * 200));
			sprintf(text, "%d/%d%s", server->inuse, server->maxplayers, server->password ? " *" : "");
			Text(text, 435, 290 + i * 22, 85, (int)(fade * 200));
			sprintf(text, "%d", CL_ServerListPing(index));
			Text(text, 533, 290 + i * 22, 55, (int)(fade * 200));
		}
		Row("CONNECT", "Join the address above", 7, 386, fade);
	}
	else if (m_page == MP_MENU_PLAYER)
	{
		Text("PLAYER SETUP", 56, 140, 530, (int)(fade * 255));
		Row("PLAYER NAME", Cvar_VariableString("name"), 0, 184, fade);
		Row("PLAYER MODEL", m_models[m_model][0] ? m_models[m_model] : "GORDON", 1, 224, fade);
		Row("SPRAY DECAL", mp_spray.string[0] ? mp_spray.string : "NONE", 2, 264, fade);
		Row("CONTROLS", "Spray / scores / chat", 3, 304, fade);
		Row("SAVE SETTINGS", "Memory card", 4, 344, fade);
		Text("Spray changes apply on your next connection.", 56, 386, 350, (int)(fade * 180));
		if (m_preview)
		{
			GL_BindStage(m_preview, 0);
			m_pMenu->SetColor(255, 255, 255, (int)(fade * 255));
			DCV_TexState_Blend();
			DCV_FlushIfLarge();
			base = DCV_GetVertCount();
			DCV_AddPolyIndices(base, 4);
			DCV_AddVertex(462, 220, dc_depthhud.value, 0, 0);
			DCV_AddVertex(574, 220, dc_depthhud.value, 1, 0);
			DCV_AddVertex(462, 332, dc_depthhud.value, 0, 1);
			DCV_AddVertex(574, 332, dc_depthhud.value, 1, 1);
		}
		else
			Text("NO PREVIEW", 448, 260, 140, (int)(fade * 150));
		if (decal_wad && mp_spray.string[0])
		{
			for (i = 0; i < decal_wad->lumpCount; i++)
			{
				if (!strcmp(decal_wad->lumps[i].name, mp_spray.string))
				{
					texture = Draw_DecalTexture(Draw_CacheIndex(decal_wad, decal_wad->lumps[i].name));
					if (texture)
					{
						GL_BindStage(texture->gl_texturenum, 0);
						DCV_TexState_Blend();
						m_pMenu->SetColor(255, 255, 255, (int)(fade * 255));
						DCV_FlushIfLarge();
						base = DCV_GetVertCount();
						DCV_AddPolyIndices(base, 4);
						DCV_AddVertex(486, 344, dc_depthhud.value, 0, 0);
						DCV_AddVertex(550, 344, dc_depthhud.value, 1, 0);
						DCV_AddVertex(486, 408, dc_depthhud.value, 0, 1);
						DCV_AddVertex(550, 408, dc_depthhud.value, 1, 1);
					}
					break;
				}
			}
		}
	}
	else
	{
		Text("CHAT", 56, 140, 530, (int)(fade * 255));
		Row("MESSAGE", "", 0, 184, fade);
	}
	if (m_editing)
		Text("UP/DOWN: LETTER   LEFT/RIGHT: CURSOR   X: DELETE   A: OK   B: CANCEL", 56, 448, 530, (int)(fade * 200));
	else
		Text(m_status[0] ? m_status : "A / ENTER: SELECT     B / ESC: BACK", 56, 448, 530, (int)(fade * 200));
	g_nTextCharGap = 0;
}

void CMenuMultiplayerItem::Edit( const char* value, int limit )
{
	m_limit = min(limit, (int)sizeof(m_text) - 1);
	strncpy(m_text, value, m_limit);
	m_text[m_limit] = 0;
	m_cursor = strlen(m_text);
	m_editing = TRUE;
	m_osk = !IN_KeyboardActive();
	m_oskKey = 0;
	m_status[0] = 0;
}

void CMenuMultiplayerItem::Accept( void )
{
	if (m_page == MP_MENU_PLAYER)
	{
		if (!m_text[0])
			return;
		Cvar_Set("name", m_text);
	}
	else if (m_page == MP_MENU_CHAT)
	{
		char command[144];
		sprintf(command, "%s \"%s\"", !strcmp(message_type, "say_team") ? "say_team" : "say", m_text);
		if (m_text[0])
		{
			Cbuf_AddText(command);
			Cbuf_AddText("\n");
		}
		key_dest = key_game;
	}
	else
		Cvar_Set(m_row == 0 ? "mp_server" : "password", m_text);
	m_editing = FALSE;
	memset(m_text, 0, sizeof(m_text));
}

void CMenuMultiplayerItem::Character( int direction )
{
	const char* alphabet = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-:@";
	const char* at;
	int length = strlen(m_text);
	int count = strlen(alphabet);
	int index;

	if (m_cursor >= m_limit)
		return;
	at = strchr(alphabet, m_text[m_cursor]);
	index = at && *at ? (int)(at - alphabet) : 0;
	index = (index + direction + count) % count;
	if (m_page == MP_MENU_ADDRESS && m_row == 0 && !index)
		index = direction > 0 ? 1 : count - 1;
	m_text[m_cursor] = alphabet[index];
	if (m_cursor == length)
		m_text[length + 1] = 0;
}

void CMenuMultiplayerItem::Spray( int direction )
{
	int i, count;

	count = decal_wad ? decal_wad->lumpCount : 0;
	if (!count)
	{
		strcpy(m_status, "No local decals loaded. Load a map to preview sprays.");
		return;
	}
	for (i = 0; i < count; i++)
		if (!strcmp(decal_wad->lumps[i].name, mp_spray.string))
			break;
	i = (i + direction + count + 1) % (count + 1);
	if (i == count)
	{
		Cvar_Set("mp_spray", "");
		return;
	}
	if (CL_CanUploadSpray(decal_wad->lumps[i].name))
		Cvar_Set("mp_spray", decal_wad->lumps[i].name);
	else
		strcpy(m_status, "This decal cannot be used as a spray.");
}

void CMenuMultiplayerItem::Join( const char* address )
{
	char command[MAX_QPATH + 16];
	int i, length;

	length = strlen(address);
	if (!length || length >= MAX_QPATH)
	{
		strcpy(m_status, "Enter a server address first.");
		return;
	}
	for (i = 0; i < length; i++)
	{
		if ((byte)address[i] <= 32 || address[i] == '"' || address[i] == ';' || address[i] == '\\')
		{
			strcpy(m_status, "Invalid server address.");
			return;
		}
	}
	sprintf(command, "connect \"%s\"", address);
	Host_WriteConfiguration();
	m_pMenu->ExecuteCommand(command, 1, 1);
}

void CMenuMultiplayerItem::Select( void )
{
	const server_cache_t* server;
	char address[MAX_QPATH];

	if (m_editing)
	{
		if (m_osk)
			KeyboardSelect();
		else
			Accept();
		return;
	}
	m_status[0] = 0;
	if (m_page == MP_MENU_ADDRESS)
	{
		if (m_row == 0)
			Edit(mp_server.string, MAX_QPATH - 1);
		else if (m_row == 1)
			Edit(Cvar_VariableString("password"), MAX_QPATH - 1);
		else if (m_row == 2)
		{
			m_firstServer = 0;
			strcpy(m_status, CL_RefreshServerList(mp_server.string) ?
				"Searching LAN and the selected server..." : "Could not resolve the server address.");
		}
		else if (m_row == 7)
			Join(mp_server.string);
		else
		{
			server = CL_ServerListEntry(m_firstServer + m_row - 3);
			if (server)
			{
				sprintf(address, "%u.%u.%u.%u:%u", server->adr.ip[0], server->adr.ip[1],
					server->adr.ip[2], server->adr.ip[3], (unsigned short)BigShort(server->adr.port));
				Cvar_Set("mp_server", address);
				Join(mp_server.string);
			}
		}
	}
	else if (m_page == MP_MENU_PLAYER)
	{
		if (m_row == 0)
			Edit(Cvar_VariableString("name"), MAX_SCOREBOARDNAME - 1);
		else if (m_row == 1)
			Model(1);
		else if (m_row == 2)
			Spray(1);
		else if (m_row == 3)
		{
			g_pszBindFocus = "impulse 201";
			m_pMenu->ExecuteCommand("menu custom", 1, 1);
		}
		else
			Host_WriteConfiguration();
	}

}

void CMenuMultiplayerItem::Cancel( void )
{
	if (m_editing)
	{
		m_editing = FALSE;
		memset(m_text, 0, sizeof(m_text));
		if (m_page == MP_MENU_CHAT)
			key_dest = key_game;
	}
	else
	{
		if (m_page == MP_MENU_PLAYER)
			Host_WriteConfiguration();
		m_pMenu->Cancel();
	}
}

void CMenuMultiplayerItem::Move( int direction )
{
	int count = m_page == MP_MENU_ADDRESS ? 8 : m_page == MP_MENU_PLAYER ? 5 : 1;
	if (m_editing)
	{
		if (m_osk)
			m_oskKey = (m_oskKey + direction * 10 + 50) % 50;
		else
			Character(direction);
		return;
	}
	if (m_page == MP_MENU_ADDRESS)
	{
		if (direction > 0 && m_row == 6 && m_firstServer + MP_BROWSER_ROWS < CL_ServerListCount())
		{
			m_firstServer++;
			return;
		}
		if (direction < 0 && m_row == 3 && m_firstServer)
		{
			m_firstServer--;
			return;
		}
	}
	m_row = (m_row + direction + count) % count;
	while (m_page == MP_MENU_ADDRESS && m_row >= 3 && m_row <= 6 &&
		m_firstServer + m_row - 3 >= CL_ServerListCount())
		m_row = (m_row + direction + count) % count;
	m_status[0] = 0;
}

void CMenuMultiplayerItem::Up( void )
{
	Move(-1);
}

void CMenuMultiplayerItem::Down( void )
{
	Move(1);
}

void CMenuMultiplayerItem::Left( void )
{
	if (m_editing)
	{
		if (m_osk)
			m_oskKey = (m_oskKey + 49) % 50;
		else if (m_cursor)
			m_cursor--;
	}
	else if (m_page == MP_MENU_PLAYER && m_row == 1)
		Model(-1);
	else if (m_page == MP_MENU_PLAYER && m_row == 2)
		Spray(-1);
}

void CMenuMultiplayerItem::Right( void )
{
	if (m_editing)
	{
		if (m_osk)
			m_oskKey = (m_oskKey + 1) % 50;
		else if (m_cursor < strlen(m_text))
			m_cursor++;
	}
	else if (m_page == MP_MENU_PLAYER && m_row == 1)
		Model(1);
	else if (m_page == MP_MENU_PLAYER && m_row == 2)
		Spray(1);
}

qboolean CMenuMultiplayerItem::Key( int key )
{
	int length;

	if (!m_editing)
		return FALSE;
	length = strlen(m_text);
	if (key == K_ENTER) Accept();
	else if (key == K_ESCAPE) Cancel();
	else if (key == K_LEFTARROW) Left();
	else if (key == K_RIGHTARROW) Right();
	else if (key == K_UPARROW) Up();
	else if (key == K_DOWNARROW) Down();
	else if (key == K_BACKSPACE && m_cursor)
	{
		memmove(m_text + m_cursor - 1, m_text + m_cursor, length - m_cursor + 1);
		m_cursor--;
	}
	else if (key == K_DEL && m_cursor < length)
		memmove(m_text + m_cursor, m_text + m_cursor + 1, length - m_cursor);
	else if (key >= 32 && key < 127 && key != '"' && key != ';' && key != '\\')
	{
		if (!(m_page == MP_MENU_ADDRESS && m_row == 0 && key == ' ') && length < m_limit)
		{
			memmove(m_text + m_cursor + 1, m_text + m_cursor, length - m_cursor + 1);
			m_text[m_cursor++] = key;
		}
	}
	return TRUE;
}
#endif

void CMenu::Build( char* pszMenu )
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

	strcpy(m_szName, pszMenu);

	ppItem = m_pItems;
	for (i = 0; i < MAX_MENU_ITEMS; i++)
		ppItem[i] = NULL;

	page = FindPage(pszMenu);

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

		m_state.pszCommand = page->pszCommand;

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
#if HLDC_MP
					if (id >= 0x110 && id <= 0x112)
					{
						*ppItem = new CMenuMultiplayerItem(this, id);
					}
					else if (id >= 0x100 && id <= 0x102)
					{
						*ppItem = new CMenuTextItem(this, FindItemDef(id), left, top, 0);
						top += spacing;
						if (first)
						{
							left += first;
							first += 8;
						}
					}
					else
#endif
					if (id < 0x65)
					{
						*ppItem = new CMenuTitleItem(this);
					}
					else if (id < 0xb0)
					{
						pDef = FindItemDef(id);

						// the way back only belongs on a page that has not
						// already filled itself with passcodes
						if (pDef && (id != 0xa8 || !nCodes))
						{
							if (id == 0xa5)
								*ppItem = new CMenuStaticItem(this, pDef, left, top);
							else if (id == 0xa7)
								*ppItem = new CMenuReturnItem(this, pDef, 20, 420);
							else if (id == 0xa6)
								*ppItem = new CMenuHintItem(this, pDef, 100, 387);
							else if (id == 0x68 || id == 0x7c || id == 0x7a || id == 0x7b)
								*ppItem = new CMenuTextItem(this, pDef, left, top, 1);
							else
								*ppItem = new CMenuTextItem(this, pDef, left, top, 0);

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
						pDef = FindItemDef(id);

						// one line per chapter the player has unlocked
						if (pDef && g_MenuCodes[0].button1 != -1)
						{
							codeY = nCodes * 40 + 176;

							for (code = g_MenuCodes; code->button1 != -1; code++)
							{
								if (code->enabled
									&& !strcmp(pDef->pszDescription, code->pszName))
								{
									*ppItem = new CMenuCodeTextItem(this, pDef, 100, codeY);
									codeY += 40;
									nCodes++;
								}
							}
						}
					}
					else if (id < 0xd3)
					{
						pOption = FindOptionDef(id);

						if (pOption)
						{
							switch (id)
							{
							case 0xd0:
								*ppItem = new CMenuWordItem(this, pOption, wordY, 256, &g_iAccessNoun2);
								wordY += 158;
								break;

							case 0xcd:
								*ppItem = new CMenuStereoItem(this, pOption, 169, 376, NULL);
								break;

							case 0xce:
								*ppItem = new CMenuWordItem(this, pOption, wordY, 256, &g_iAccessNoun1);
								wordY += 158;
								break;

							case 0xcf:
								*ppItem = new CMenuWordItem(this, pOption, wordY, 256, &g_iAccessVerb);
								wordY += 158;
								break;

							default:
								*ppItem = new CMenuToggleItem(this, pOption, left, top, id);
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
						pOption = FindOptionDef(id);

						// one line per cheat the player has unlocked
						if (pOption && g_MenuCodes[0].button1 != -1)
						{
							codeY = nCodes * 40 + 176;

							for (code = g_MenuCodes; code->button1 != -1; code++)
							{
								if (code->enabled
									&& !strcmp(pOption->pszDescription, code->pszName))
								{
									pCheat = new CMenuCheatItem(this, pOption, 100, codeY, &code->result);

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
							*ppItem = new CMenuIconItem(this, 0xe5, 300, 300);
							break;

						case 0xe2:
							*ppItem = new CMenuIconItem(this, 0xe2, 200, 100);
							break;

						case 0xe3:
							*ppItem = new CMenuIconItem(this, 0xe3, 390, 180);
							break;

						case 0xe4:
							*ppItem = new CMenuIconItem(this, 0xe4, 70, 317);
							break;
						}
					}
					else if (id < 0xe9)
					{
						pOption = FindOptionDef(id);

						*ppItem = new CMenuSaveSlotItem(this, pOption, slotX, nSlots, id);
						nSlots++;
						slotX += 104;
					}
					else if (id < 0xf0)
					{
						pSlider = FindSliderDef(id);

						if (pSlider)
						{
							if (id == 0xeb)
							{
								*ppItem = new CMenuVolumeSlider(this, pSlider, 169, 136, 0xeb, 0);
							}
							else if (id == 0xec)
							{
								*ppItem = new CMenuVolumeSlider(this, pSlider, 169, 296, 0xec, 0);
							}
							else if (id == 0xed)
							{
								*ppItem = new CMenuVolumeSlider(this, pSlider, 169, 216, 0xed, 0);
							}
							else if (id == 0xee)
							{
								*ppItem = new CMenuSensitivitySlider(this, pSlider, left, top, 0xee);
								top += spacing;
								if (first)
								{
									left += first;
									first += 8;
								}
							}
							else if (id == 0xef)
							{
								*ppItem = new CMenuSensitivitySlider(this, pSlider, left, top, 0xef);
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
							*ppItem = new CMenuPicItem(this, "gfx/menu_backdrop.pvr");
						}
						else if (id == 0xf2)
						{
							*ppItem = new CMenuPicItem(this, "gfx/startup.pvr");

							// the title screens take their time fading
							m_state.flAnimTime = 1.0f;
						}
						else
						{
							*ppItem = new CMenuPicItem(this, "gfx/startup2.pvr");
							m_state.flAnimTime = 1.0f;
						}
					}
					else if (id < 0xf6)
					{
						*ppItem = new CMenuCreditsItem(this);
					}
					else
					{
						switch (id)
						{
						case 0xf7:
							*ppItem = new CMenuSaveHeaderItem(this);
							break;

						case 0xf8:
							*ppItem = new CMenuPresetItem(this, 'A', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xf9:
							*ppItem = new CMenuPresetItem(this, 'B', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xfa:
							*ppItem = new CMenuPresetItem(this, 'C', left, top);
							top += spacing;
							if (first)
							{
								left += first;
								first += 8;
							}
							break;

						case 0xfb:
							*ppItem = new CMenuBindItem(this);
							break;

						case 0xfc:
							*ppItem = new CMenuAttractItem(this, 5.0f);
							break;

						case 0xfd:
							*ppItem = new CMenuAnyKeyItem(this);
							break;
						}
					}
				}
			}
		}

		// a page comes up with nothing held down
		for (i = 0; i < MAX_MENU_BUTTONS; i++)
			joymenubuttons[i] = 0;

		m_state.iSelected = 0;

		// settle the stick on the first line it can actually land on
		start = m_state.iSelected;
		do
		{
			m_state.iSelected--;
			if (m_state.iSelected < 0)
				m_state.iSelected = MAX_MENU_ITEMS - 1;
		} while ((!m_pItems[m_state.iSelected]
			|| !m_pItems[m_state.iSelected]->IsActive())
			&& m_state.iSelected != start);

		start = m_state.iSelected;
		do
		{
			m_state.iSelected++;
			if (m_state.iSelected > MAX_MENU_ITEMS - 1)
				m_state.iSelected = 0;
		} while ((!m_pItems[m_state.iSelected]
			|| !m_pItems[m_state.iSelected]->IsActive())
			&& m_state.iSelected != start);
	}
}

/*
==================
CMenu::Draw
==================
*/
void CMenu::Draw( void )
{
	CMenuItemBase*	pItem;
	int			i;
	int			row;
	int			skip;

	if (!strcmp(m_szName, "activate"))
	{
		// the codes scroll behind three fixed items
		if (m_pItems[0])
			m_pItems[0]->Draw(m_state.flFade, m_state.iSelected == 0);

		if (m_pItems[1])
			m_pItems[1]->Draw(m_state.flFade, m_state.iSelected == 1);

		if (m_pItems[2])
			m_pItems[2]->Draw(m_state.flFade, m_state.iSelected == 2);

		skip = m_state.iTopItem;
		row = 0;

		for (i = 3; i < MAX_MENU_ITEMS; i++)
		{
			pItem = m_pItems[i];
			if (!pItem)
				continue;

			if (skip > 0)
			{
				skip--;
			}
			else
			{
				if (i == m_state.iSelected)
					m_state.iSelectedRow = m_state.iTopItem + row;

				pItem->SetPos(100.0f, ((float)row - 0.5f) * 40.0f + 176.0f);
				pItem->Draw(m_state.flFade, i == m_state.iSelected);

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
			pItem = m_pItems[i];
			if (pItem)
				pItem->Draw(m_state.flFade, i == m_state.iSelected);
		}
	}
}

/*
==================
CMenu::Input
==================
*/
void CMenu::Input( void )
{
	CMenuItemBase*	pItem;

	m_state.flTime = Sys_FloatTime();

	if (m_state.flAnimating == 1.0f)
	{
		AnimateLerp();
		return;
	}

	if (m_state.flFade == 0)
	{
		// the page has faded away; run whatever it left behind
		gfDrawMenu = 0;

		if (!strlen(m_szCommand))
			return;

		// keep the sound blocked when the command is only going to bring up
		// another page
		if (strstr(m_szCommand, "menu"))
			m_state.iSoundBlocked = 1;
		else
			m_state.iSoundBlocked = 0;

		Cbuf_AddText(m_szCommand);
		Cbuf_AddText("\n");
		return;
	}

#if HLDC_MP
	if (joymenubuttons[8])
		UI_MultiplayerKeyEvent(K_BACKSPACE);
#endif

	// start backs out of the in-game page and confirms everywhere else
	if (joymenubuttons[3])
	{
		if (!strcmp(m_szName, "gamemenu"))
			joymenubuttons[1] = 1;
		else
			joymenubuttons[0] = 1;
	}

	if (joymenubuttons[7] || joymenubuttons[15])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		CheckCheatCode( 'u');
		m_pItems[m_state.iSelected]->Up();
	}

	if (joymenubuttons[6] || joymenubuttons[14])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		CheckCheatCode( 'd');
		m_pItems[m_state.iSelected]->Down();
	}

	if (joymenubuttons[5] || joymenubuttons[13])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		CheckCheatCode( 'r');
		m_pItems[m_state.iSelected]->Right();
	}

	if (joymenubuttons[4] || joymenubuttons[12])
	{
		PlaySound("common/wpn_moveselect.wav", 1.0f);
		CheckCheatCode( 'l');
		m_pItems[m_state.iSelected]->Left();
	}

	if (joymenubuttons[0])
	{
		PlaySound("common/wpn_select.wav", 1.0f);
		CheckCheatCode( 'a');

		pItem = m_pItems[m_state.iSelected];
		if (!pItem->IsActive())
		{
			gfDrawMenu = 0;

			if (strstr(m_state.pszCommand, "menu"))
				m_state.iSoundBlocked = 1;
			else
				m_state.iSoundBlocked = 0;

			Cbuf_AddText(m_state.pszCommand);
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
		CheckCheatCode( 'b');

		pItem = m_pItems[m_state.iSelected];
		if (!pItem->IsActive())
		{
			gfDrawMenu = 0;

			if (strstr(m_state.pszCommand, "menu"))
				m_state.iSoundBlocked = 1;
			else
				m_state.iSoundBlocked = 0;

			Cbuf_AddText(m_state.pszCommand);
			Cbuf_AddText("\n");
		}
		else if (strcmp(m_szName, "main") && m_state.pszCommand)
		{
			pItem->Cancel();
		}
	}

	if (joymenubuttons[8])
		CheckCheatCode( 'x');

	if (joymenubuttons[9])
		CheckCheatCode( 'y');
}

CMenuTitleItem::~CMenuTitleItem( void )
{
}

CMenuHintItem::~CMenuHintItem( void )
{
}

CMenuReturnItem::~CMenuReturnItem( void )
{
}

CMenuPicItem::~CMenuPicItem( void )
{
}

CMenuCreditsItem::~CMenuCreditsItem( void )
{
}

CMenuAttractItem::~CMenuAttractItem( void )
{
}

CMenuSaveHeaderItem::~CMenuSaveHeaderItem( void )
{
}

CMenuAnyKeyItem::~CMenuAnyKeyItem( void )
{
}

CMenuOptionItem::~CMenuOptionItem( void )
{
	delete m_pValues;
}

CMenuWordItem::~CMenuWordItem( void )
{
	delete m_pValues;
}

CMenuVolumeSlider::~CMenuVolumeSlider( void )
{
}

CMenuSensitivitySlider::~CMenuSensitivitySlider( void )
{
}

CMenuPresetItem::~CMenuPresetItem( void )
{
	int i;

	for (i = 0; i < m_nAliases; i++)
	{
		delete m_ppAliases[i * 2 + 1];
		delete m_ppAliases[i * 2];
	}
	delete m_ppAliases;
}

void CMenuCheatItem::SetPos( float x, float y )
{
	int i;

	for (i = 0; i < m_nValues; i++)
	{
		m_pValues[i].x = (int)x;
		m_pValues[i].y = (int)y;
	}
	m_descX = scr_safe_x + 200;
	m_descY = 416 - scr_safe_y;
}

int CMenuPicItem::IsActive( void )
{
	return 0;
}

void CMenuSensitivitySlider::Cancel( void )
{
	char command[MAX_MENU_COMMAND_TEXT];

	sprintf(command, "joyadvancedupdate");
	m_pMenu->ExecuteCommand(command, 0, 0);
	Host_WriteConfiguration();
	m_pMenu->Cancel();
}

CMenuCheatItem::CMenuCheatItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue )
	: CMenuOptionItem(pMenu, pOption, x, y, piValue)
{
	m_pfnBind = pOption->pfnBind;
}

CMenuStaticItem::CMenuStaticItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y )
	: CMenuTextItem(pMenu, pDef, x, y, 0)
{
}

CMenuHintItem::CMenuHintItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y )
	: CMenuStaticItem(pMenu, pDef, x, y)
{
}

CMenuReturnItem::CMenuReturnItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y )
	: CMenuStaticItem(pMenu, pDef, x, y)
{
}

CMenuCodeTextItem::CMenuCodeTextItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y )
	: CMenuTextItem(pMenu, pDef, x, y, 0)
{
}

CMenuIconItem::CMenuIconItem( CMenu* pMenu, int id, int x, int y )
	: CMenuItemBase(pMenu)
{
	m_x = x;
	m_y = y;
	m_id = id;
	m_alpha = 128;
}

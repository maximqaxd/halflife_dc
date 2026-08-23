// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "quakedef.h"
#include "vmu.h"
#include "winquake.h"
#include <dinput.h>
#ifdef _WIN32_WCE
#include <maplusag.h>
#endif 
extern int iMouseInUse;
extern HINSTANCE g_hInstance;

BOOL CALLBACK IN_EnumDevicesCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef );

struct maplemouse_t;
qboolean IN_ReadMouseState( maplemouse_t *pMouse );
void IN_DebugPrintf( LPCTSTR fmt, ... );
qboolean IN_LogResult( LPCTSTR what );
void IN_UpdateMapleDevices( void );
void IN_ReadMouse( void );
void GDROM_DoorReset( void );
void M_DecodeStateFlags( void );
struct maplejoystick_t;
qboolean IN_ReadJoystick( maplejoystick_t *pJoy );

void Host_UpdateScreenSaver( int bCheckOnly );

// Win32 virtual-key (message wParam) -> engine key-number translation table.
unsigned char scantokey[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 127, 9, 0, 0, 0, 13, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 0, 0,
	32, 150, 149, 152, 151, 130, 128, 131, 129, 0, 0, 0, 0, 147, 148, 0,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 0, 0, 0, 0, 0,
	0, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 0, 0, 0, 0, 0,
	147, 152, 129, 149, 130, 0, 131, 151, 128, 150, 0, 0, 0, 0, 148, 0,
	135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	134, 134, 133, 133, 132, 132, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 59, 61, 44, 45, 46, 47,
	96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 91, 92, 93, 39, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int MapKey( int key )
{
	return scantokey[key];
}

// mouse variables

int			mouse_buttons;
int			mouse_oldbuttonstate;
POINT		current_pos;
int			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static qboolean	restore_spi;
static int		originalmouseparms[3], newmouseparms[3] = { 0, 0, 1 };
int		mouseactive;
qboolean		mouseinitialized;
static qboolean	mouseparmsvalid, mouseactivatetoggle;
static int	mouseshowtoggle = 1;

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		2				// X, Y
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1

// Buttons and hat switches the pad can report, addressed by usage
#define MAX_JOY_BUTTONS		22
#define MAX_JOY_POVS		11

// A usage slot that no object claimed
#define JOY_USAGE_NONE		255

// Holding a shift button moves the pad onto one of two alternate key ranges,
// and holding both moves it onto a third
#define K_JOYSHIFT1			160
#define K_JOYSHIFT2			171
#define K_JOYSHIFT12		182

enum _ControlList
{
	AxisNada = 0,
	AxisForward,
	AxisLook,
	AxisSide,
	AxisTurn
};


DWORD	dwAxisMap[JOY_MAX_AXES];
DWORD	dwControlMap[JOY_MAX_AXES];
PDWORD	pdwRawValue[JOY_MAX_AXES];

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t	in_joystick = { "joystick", "1", FCVAR_ARCHIVE };
cvar_t	joy_name = { "joyname", "joystick" };
cvar_t	joy_advanced = { "joyadvanced", "1", FCVAR_ARCHIVE };
cvar_t	joy_advaxisx = { "joyadvaxisx", "4", FCVAR_ARCHIVE };
cvar_t	joy_advaxisy = { "joyadvaxisy", "1", FCVAR_ARCHIVE };
cvar_t	joy_advaxisz = { "joyadvaxisz", "0", FCVAR_ARCHIVE };
cvar_t	joy_advaxisr = { "joyadvaxisr", "0", FCVAR_ARCHIVE };
cvar_t	joy_advaxisu = { "joyadvaxisu", "0", FCVAR_ARCHIVE };
cvar_t	joy_advaxisv = { "joyadvaxisv", "0", FCVAR_ARCHIVE };
cvar_t	joy_forwardthreshold = { "joyforwardthreshold", "0.02", FCVAR_ARCHIVE };
cvar_t	joy_sidethreshold = { "joysidethreshold", "0.02", FCVAR_ARCHIVE };
cvar_t	joy_pitchthreshold = { "joypitchthreshold", "0.02", FCVAR_ARCHIVE };
cvar_t	joy_yawthreshold = { "joyyawthreshold", "0.02", FCVAR_ARCHIVE };
cvar_t	joy_forwardsensitivity = { "joyforwardsensitivity", "-2.0", FCVAR_ARCHIVE };
cvar_t	joy_sidesensitivity = { "joysidesensitivity", "1.0", FCVAR_ARCHIVE };
cvar_t	joy_pitchsensitivity = { "joypitchsensitivity", "-0.5", FCVAR_ARCHIVE };
cvar_t	joy_yawsensitivity = { "joyyawsensitivity", "-1.5", FCVAR_ARCHIVE };
cvar_t	joy_wwhack1 = { "joywwhack1", "0.0" };
cvar_t	joy_wwhack2 = { "joywwhack2", "0.0" };

// The analog stick is smoothed rather than read raw: softstick turns the
// filtering on, softaccel and softdamp set how fast it winds up and decays,
// and softstop is the deflection below which it snaps back to centre.
cvar_t	softstick = { "softstick", "1", FCVAR_ARCHIVE };
cvar_t	softaccel = { "softaccel", "0.5", FCVAR_ARCHIVE };
cvar_t	softdamp = { "softdamp", "0.5", FCVAR_ARCHIVE };
cvar_t	softstop = { "softstop", "0.4", FCVAR_ARCHIVE };

// Buttons that shift the pad into its alternate binding set
cvar_t	joyshift1 = { "joyshift1", "AUX6", FCVAR_ARCHIVE };
cvar_t	joyshift2 = { "joyshift2", "", FCVAR_ARCHIVE };

// Frames the controller-missing warning stays up after the pad is unplugged
#define CONTROLLER_GRACE_FRAMES		3

// Every unit hanging off the four Maple sockets is described the same way:
// the GUID the driver knows it by, the socket it turned up in, what kind of
// device it is, and the DirectInput device created to talk to it.
typedef struct
{
	GUID					guid;
	int						port;
	int						type;
	LPDIRECTINPUTDEVICE2	pDevice;
} mapledevice_t;

// One analogue axis of the pad, paired up with the object the driver reports
// for it while the device is being enumerated.
typedef struct
{
	byte	usage;
	int		axis;
} maplejoyaxis_t;

// The pad. Buttons and axes are addressed by usage rather than by position,
// so a stick and a hat land in the right place whatever order they enumerate.
struct maplejoystick_t
{
	byte			buttonUsage[MAX_JOY_BUTTONS];
	maplejoyaxis_t	axes[4];
	byte			oldButtons[MAX_JOY_BUTTONS];
	int				buttonChanged[MAX_JOY_BUTTONS];
	int				axisValue[4];
	byte			povUsage[MAX_JOY_POVS];
	int				pov[MAX_JOY_POVS][2];
	DIPROPRANGE		range[2];
	int				numAxes;
	int				numButtons;
	DIDEVCAPS		caps;
	mapledevice_t	device;

	maplejoystick_t( GUID guid, int type );
};

// The keyboard. The driver hands over a full scan-code table each read.
struct maplekeyboard_t
{
	mapledevice_t	device;
	DIDEVCAPS		caps;
	byte			state[256];

	maplekeyboard_t( GUID guid, int type );
};

// The pointing device. The driver reports movement, and the cursor position is
// carried here between frames so it can be pulled back to the centre.
struct maplemouse_t
{
	int				numAxes;
	int				numButtons;
	int				x;
	int				y;
	int				z;
	int				lastx;
	int				lasty;
	int				acquired;
	byte			oldButtons[4];
	int				buttonChanged[4];
	DIMOUSESTATE	state;
	DIDEVCAPS		caps;
	mapledevice_t	device;

	maplemouse_t( GUID guid, int type );
};

// What is plugged into each of the four Maple sockets. A port that has been
// seen this scan has its stale flag cleared; whatever is left marked stale
// when the scan finishes has been unplugged.
typedef struct
{
	int		type;
	int		stale;
	int		present;
	void*	pDevice;
} mapleport_t;

#define MAPLE_MAX_PORTS		4

#define MAPLE_KEYBOARD		1
#define MAPLE_CONTROLLER	2
#define MAPLE_MOUSE			3

mapleport_t				g_MapleDevices[MAPLE_MAX_PORTS];

// Signalled by the driver when something is plugged in or pulled out
HANDLE					hNewDevice;
HANDLE					hDeviceRemoved;

LPDIRECTINPUT			g_pDI;
HRESULT					g_hResult;

// What a call is expected to come back with, and whether the successful ones
// are worth a line of their own
HRESULT					g_hrExpected;
int						g_bLogFailuresOnly;

// The attached pad, mouse and keyboard; NULL while the socket is empty
void*					pKeyboardDevice;
maplejoystick_t*		pJoystickDevice;
maplemouse_t*			pMouseDevice;

int			gnControllerGrace;

// Where the pointer sat last frame, so small twitches can be ignored
int			mouse_lastx, mouse_lasty;

// Where the sticks read when they are not being pushed
int			joy_centerx, joy_centery;

// Where the pointing device is sitting this frame
POINT		mouse_pos;

// Set from the client's client_data_t every frame
float		gMouseSensitivity;
float		gJoySensitivity = 1.0f;

// Which way the pad is pushing this frame, so the walk cycle can lean the
// right way; zeroed every move and set to -1 or 1 as the sticks are read
int			joy_forwarddir, joy_sidedir;

// Smoothed stick deflection carried between frames, one per axis
float		joy_softvalue[JOY_MAX_AXES];

// The pad button that each of the eleven shiftable positions reports as
int			joykeys[10] =
{
	K_JOY1, K_JOY2, K_JOY3, K_JOY4,
	K_AUX1, K_AUX2, K_AUX3, K_AUX4, K_AUX5, K_AUX6
};

// Which button each shiftable position maps to, and the key each button sends
const int	joyshiftmap[MAX_JOY_POVS] = { 0, 1, 8, 9, 4, 5, 6, 7, 16, 17, 3 };
const int	joybuttonkeys[MAX_JOY_BUTTONS] =
{
	K_JOY1,  K_JOY2,  K_AUX8,  K_AUX7,  K_AUX1,  K_AUX2,  K_AUX3,  K_AUX4,
	K_JOY3,  K_JOY4,  K_AUX9,  K_AUX10, K_AUX11, K_AUX12, K_AUX13, K_AUX14,
	K_AUX5,  K_AUX6,  K_AUX15, K_AUX14, K_AUX16, K_AUX17
};

// What the menu sees as held this frame. The analogue stick drives the four
// direction slots as well, so a stick and a d-pad navigate the same way.
int			joymenubuttons[MAX_JOY_BUTTONS];

// Stick deflection last frame, so a push only counts once until it recentres
int			joy_lastx, joy_lasty;

// Which positions the two shift buttons are bound to
int			joyshift1keys[MAX_JOY_POVS], joyshift1count;
int			joyshift2keys[MAX_JOY_POVS], joyshift2count;

qboolean	joy_advancedinit, joy_haspov;
DWORD		joy_oldbuttonstate, joy_oldpovstate;
DWORD		joy_numbuttons;

/* WinCE/Dreamcast: DirectInput controller support (MAPLE). */
static LPDIRECTINPUT			s_di;
static LPDIRECTINPUTDEVICE2		s_di_joy;
static LPDIRECTINPUTDEVICE2		s_di_kbd;
static HANDLE					s_di_newdev_event;
static DIJOYSTATE				s_di_state;
static DWORD					s_joy_raw[JOY_MAX_AXES];
static DWORD					s_joy_buttons;
static DWORD					s_joy_pov; /* 0..35999 or 0xFFFF (center) */
static GUID						s_joy_guid;
static int						s_joy_guid_valid;

/* DirectInput keyboard state (256 DIK scan codes). */
static BYTE						s_kbd_state[256];
static BYTE						s_kbd_oldstate[256];

static int IN_DI_KeyFromDIK(int dik)
{
	switch (dik)
	{
	/* Letters (DIK are scan codes; not contiguous). */
	case DIK_A: return 'a';
	case DIK_B: return 'b';
	case DIK_C: return 'c';
	case DIK_D: return 'd';
	case DIK_E: return 'e';
	case DIK_F: return 'f';
	case DIK_G: return 'g';
	case DIK_H: return 'h';
	case DIK_I: return 'i';
	case DIK_J: return 'j';
	case DIK_K: return 'k';
	case DIK_L: return 'l';
	case DIK_M: return 'm';
	case DIK_N: return 'n';
	case DIK_O: return 'o';
	case DIK_P: return 'p';
	case DIK_Q: return 'q';
	case DIK_R: return 'r';
	case DIK_S: return 's';
	case DIK_T: return 't';
	case DIK_U: return 'u';
	case DIK_V: return 'v';
	case DIK_W: return 'w';
	case DIK_X: return 'x';
	case DIK_Y: return 'y';
	case DIK_Z: return 'z';

	/* Number row. */
	case DIK_0: return '0';
	case DIK_1: return '1';
	case DIK_2: return '2';
	case DIK_3: return '3';
	case DIK_4: return '4';
	case DIK_5: return '5';
	case DIK_6: return '6';
	case DIK_7: return '7';
	case DIK_8: return '8';
	case DIK_9: return '9';

	case DIK_SPACE: return K_SPACE;
	case DIK_TAB: return K_TAB;
	case DIK_RETURN: return K_ENTER;
	case DIK_ESCAPE: return K_ESCAPE;
	case DIK_BACK:
		return K_BACKSPACE;

	case DIK_UP: return K_UPARROW;
	case DIK_DOWN: return K_DOWNARROW;
	case DIK_LEFT: return K_LEFTARROW;
	case DIK_RIGHT: return K_RIGHTARROW;

	case DIK_HOME: return K_HOME;
	case DIK_END: return K_END;
	case DIK_PRIOR: return K_PGUP;
	case DIK_NEXT: return K_PGDN;
	case DIK_INSERT: return K_INS;
	case DIK_DELETE: return K_DEL;

	case DIK_LSHIFT:
	case DIK_RSHIFT: return K_SHIFT;
	case DIK_LCONTROL:
	case DIK_RCONTROL: return K_CTRL;
	case DIK_LMENU:
	case DIK_RMENU:
		return K_ALT;

	case DIK_F1: return K_F1;
	case DIK_F2: return K_F2;
	case DIK_F3: return K_F3;
	case DIK_F4: return K_F4;
	case DIK_F5: return K_F5;
	case DIK_F6: return K_F6;
	case DIK_F7: return K_F7;
	case DIK_F8: return K_F8;
	case DIK_F9: return K_F9;
	case DIK_F10: return K_F10;
	case DIK_F11: return K_F11;
	case DIK_F12: return K_F12;

	/* Common punctuation (no shift handling here; binds typically use unshifted). */
	case DIK_MINUS: return '-';
	case DIK_EQUALS: return '=';
	case DIK_LBRACKET: return '[';
	case DIK_RBRACKET: return ']';
	case DIK_SEMICOLON: return ';';
	case DIK_APOSTROPHE: return '\'';
	case DIK_GRAVE: return '`';
	case DIK_BACKSLASH: return '\\';
	case DIK_COMMA: return ',';
	case DIK_PERIOD: return '.';
	case DIK_SLASH: return '/';
	default:
		return 0;
	}
}

/* Map Dreamcast HID usages -> DIJOYSTATE offsets. */
static int						s_di_ofs_x = -1;
static int						s_di_ofs_y = -1;
static int						s_di_ofs_rx = -1;
static int						s_di_ofs_ry = -1;
static int						s_di_ofs_slider0 = -1;
static int						s_di_ofs_pov0 = -1;
#ifndef _WIN32_WCE
#define DI_BUTTON_USAGE_COUNT 32
#else
#define DI_BUTTON_USAGE_COUNT (USAGE_LAST_BUTTON - USAGE_FIRST_BUTTON + 1)
#endif
static int						s_di_ofs_button_usage[DI_BUTTON_USAGE_COUNT];

static qboolean IN_DI_EnsureDirectInput(void)
{
	HRESULT hr;

	if (s_di)
		return TRUE;

	hr = DirectInputCreate(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &s_di, NULL);
	if (FAILED(hr) || !s_di)
	{
		Con_DPrintf("\nDirectInputCreate failed (%lx)\n\n", (long)hr);
		s_di = NULL;
		return FALSE;
	}

	return TRUE;
}

static BOOL CALLBACK IN_DI_EnumObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	(void)pvRef;
	if (!lpddoi)
		return DIENUM_CONTINUE;

#if defined(_WIN32_WCE)
	/* Axes (Dreamcast HID usage page). */
	if (lpddoi->wUsagePage == USAGE_PAGE_AXES)
	{
		if (lpddoi->wUsage == USAGE_X_AXIS) s_di_ofs_x = (int)lpddoi->dwOfs;
		else if (lpddoi->wUsage == USAGE_Y_AXIS) s_di_ofs_y = (int)lpddoi->dwOfs;
		else if (lpddoi->wUsage == USAGE_RX_AXIS) s_di_ofs_rx = (int)lpddoi->dwOfs;
		else if (lpddoi->wUsage == USAGE_RY_AXIS) s_di_ofs_ry = (int)lpddoi->dwOfs;
		else if (lpddoi->wUsage == USAGE_SLIDER && s_di_ofs_slider0 < 0) s_di_ofs_slider0 = (int)lpddoi->dwOfs;
		return DIENUM_CONTINUE;
	}

	/* Dreamcast buttons are reported via the made-up 0xFFxx usage range. */
	if (lpddoi->wUsage >= USAGE_FIRST_BUTTON && lpddoi->wUsage <= USAGE_LAST_BUTTON)
	{
		int idx = (int)(lpddoi->wUsage - USAGE_FIRST_BUTTON);
		if (idx >= 0 && idx < DI_BUTTON_USAGE_COUNT)
			s_di_ofs_button_usage[idx] = (int)lpddoi->dwOfs;
		return DIENUM_CONTINUE;
	}
#endif

	/* POV hat: on some devices it is a POV object rather than UA/DA/LA/RA buttons. */
	if ((lpddoi->dwType & DIDFT_POV) && s_di_ofs_pov0 < 0)
	{
		s_di_ofs_pov0 = (int)lpddoi->dwOfs;
	}

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK IN_DI_EnumKeyboardProc(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	GUID *outGuid;

	outGuid = (GUID*)pvRef;
	if (!lpddi || !outGuid)
		return DIENUM_CONTINUE;

	/* First keyboard-like device is good enough. */
	*outGuid = lpddi->guidInstance;
	return DIENUM_STOP;
}

static BOOL CALLBACK IN_DI_EnumDevicesProc(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	(void)pvRef;
	/* Take the first joystick/gamepad-like device. */
	if (!s_joy_guid_valid)
	{
		s_joy_guid = lpddi->guidInstance;
		s_joy_guid_valid = 1;
		return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}

static int IN_DI_IsZeroGuid(const GUID *g)
{
	static const GUID s_zero = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
	if (!g)
		return 1;
	return (memcmp(g, &s_zero, sizeof(GUID)) == 0) ? 1 : 0;
}

static void IN_StartupKeyboard(void)
{
	HRESULT hr;
	LPDIRECTINPUTDEVICE pDev1;
	GUID kbdGuid;
	int haveGuid;

	if (!IN_DI_EnsureDirectInput())
		return;
	if (s_di_kbd)
		return;

	pDev1 = NULL;
	hr = s_di->CreateDevice(GUID_SysKeyboard, &pDev1, NULL);
	if (FAILED(hr) || !pDev1)
	{
		/* Some WinCE builds don’t expose GUID_SysKeyboard; fall back to enumeration. */
		haveGuid = 0;
		memset(&kbdGuid, 0, sizeof(kbdGuid));
		s_di->EnumDevices(DIDEVTYPE_KEYBOARD, IN_DI_EnumKeyboardProc, &kbdGuid, 0);

		/* A zero GUID likely means we didn’t find anything. */
		if (!IN_DI_IsZeroGuid(&kbdGuid))
			haveGuid = 1;

		if (!haveGuid)
			return;

		pDev1 = NULL;
		hr = s_di->CreateDevice(kbdGuid, &pDev1, NULL);
		if (FAILED(hr) || !pDev1)
			return;
	}
	if (FAILED(hr) || !pDev1)
		return;

	hr = pDev1->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&s_di_kbd);
	pDev1->Release();
	if (FAILED(hr) || !s_di_kbd)
	{
		s_di_kbd = NULL;
		return;
	}

	hr = s_di_kbd->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
	{
		s_di_kbd->Release();
		s_di_kbd = NULL;
		return;
	}

	memset(s_kbd_state, 0, sizeof(s_kbd_state));
	memset(s_kbd_oldstate, 0, sizeof(s_kbd_oldstate));

	/* No cooperative level on WinCE sample; just acquire. */
	s_di_kbd->Acquire();
}

static void IN_ReadKeyboard(void)
{
	HRESULT hr;
	int i;

	if (!s_di_kbd)
		return;

	hr = s_di_kbd->Poll();
	if (FAILED(hr))
	{
		s_di_kbd->Acquire();
		hr = s_di_kbd->Poll();
	}
	if (FAILED(hr))
		return;

	memset(s_kbd_state, 0, sizeof(s_kbd_state));
	hr = s_di_kbd->GetDeviceState(sizeof(s_kbd_state), (LPVOID)s_kbd_state);
	if (FAILED(hr))
		return;

	for (i = 0; i < 256; i++)
	{
		int down_now = (s_kbd_state[i] & 0x80) ? 1 : 0;
		int down_old = (s_kbd_oldstate[i] & 0x80) ? 1 : 0;
		int key;

		if (down_now == down_old)
			continue;

		key = IN_DI_KeyFromDIK(i);
		if (!key)
			continue;

		Key_Event(key, down_now ? TRUE : FALSE);
	}

	memcpy(s_kbd_oldstate, s_kbd_state, sizeof(s_kbd_oldstate));
}

// forward-referenced functions
void IN_StartupJoystick( void );
void Joy_AdvancedUpdate_f( void );
void IN_JoyMove( usercmd_t* cmd );

/*
===========
IN_ControllerPresent

Report whether a controller is plugged in; the status text warns when it
isn't. TODO: query the actual pad state.
===========
*/
int IN_ControllerPresent( void )
{
	if (!pJoystickDevice)
	{
		// Give the player a moment to plug one back in before complaining
		gnControllerGrace = CONTROLLER_GRACE_FRAMES;
		return FALSE;
	}

	if (gnControllerGrace > 0)
		gnControllerGrace--;
	else
		gnControllerGrace = 0;

	return TRUE;
}


/*
===========
IN_DebugPrintf

Format a message about the state of the attached controllers. The text is
only wanted when tracking down a hotplug problem, so nothing is emitted.
===========
*/
void IN_DebugPrintf( LPCTSTR fmt, ... )
{
	TCHAR	buf[256];
	va_list	args;

	va_start(args, fmt);
	wvsprintf(buf, fmt, args);
	//OutputDebugString
		(buf);
}


/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f( void )
{
	if (!iMouseInUse)
	{
		cl.viewangles[PITCH] = 0;
	}
}


/*
===========
IN_UpdateClipCursor
===========
*/
void IN_UpdateClipCursor( void )
{
}


/*
===========
IN_ShowMouse
===========
*/
void IN_ShowMouse( void )
{
}


/*
===========
IN_HideMouse
===========
*/
void IN_HideMouse( void )
{
}


/*
===========
IN_ActivateMouse
===========
*/
void IN_ActivateMouse( void )
{
	if (mouseinitialized)
	{
		if (mouseparmsvalid)
			restore_spi = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0);

		mouseactive = TRUE;
	}
}


/*
===========
IN_SetQuakeMouseState
===========
*/
void IN_SetQuakeMouseState( void )
{
}


/*
===========
IN_DeactivateMouse
===========
*/
void IN_DeactivateMouse( void )
{
	mouseactivatetoggle = FALSE;

	if (mouseinitialized)
	{
		if (restore_spi)
			SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

		mouseactive = FALSE;
	}
}


/*
===========
IN_StartupMouse
===========
*/
qboolean IN_StartupMouse( maplemouse_t *pMouse )
{
	LPDIRECTINPUTDEVICE		did1;
	LPDIRECTINPUTDEVICE2	did2;
	DIPROPDWORD				prop;

	g_hResult = g_pDI->CreateDevice(pMouse->device.guid, &did1, NULL);
	if (IN_LogResult(TEXT("Create Device")))
		return FALSE;

	g_hResult = did1->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&did2);
	if (IN_LogResult(TEXT("Query Interface for DirectInputDevice2")))
	{
		did1->Release();
		return FALSE;
	}
	did1->Release();

	prop.diph.dwSize = sizeof(DIPROPDWORD);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = 0;
	prop.dwData = 0;
	g_hResult = did2->GetProperty(DIPROP_PORTNUMBER, &prop.diph);
	if (IN_LogResult(TEXT("Get Port Number")))
	{
		did2->Release();
		return FALSE;
	}

	pMouse->device.port = prop.dwData;
	g_MapleDevices[pMouse->device.port].stale = 0;
	if (g_MapleDevices[pMouse->device.port].present)
	{
		did2->Release();
		return FALSE;
	}

	pMouse->acquired = 1;
	pMouse->caps.dwSize = sizeof(DIDEVCAPS);
	g_hResult = did2->GetCapabilities(&pMouse->caps);
	if (IN_LogResult(TEXT("Get Device Capabilities")))
	{
		did2->Release();
		return FALSE;
	}

	pMouse->numButtons = pMouse->caps.dwButtons;
	pMouse->numAxes = pMouse->caps.dwAxes;

	g_hResult = did2->SetDataFormat(&c_dfDIMouse);
	if (IN_LogResult(TEXT("Set Data Format (Mouse)")))
	{
		did2->Release();
		return FALSE;
	}

	g_hResult = did2->Acquire();
	if (IN_LogResult(TEXT("Acquire port")))
	{
		did2->Release();
		return FALSE;
	}

	pMouse->device.pDevice = did2;
	IN_ReadMouseState(pMouse);

	// start the cursor in the middle of the screen
	pMouse->x = 320;
	pMouse->y = 240;
	pMouse->z = 0;
	pMouse->lastx = pMouse->state.lX;
	pMouse->lasty = pMouse->state.lY;

	return TRUE;
}


/*
===========
IN_ReadMouseState
===========
*/
qboolean IN_ReadMouseState( maplemouse_t *pMouse )
{
	int		i;

	// the mouse is handed back to us whenever the game loses focus
	if (pMouse->device.pDevice->GetDeviceState(sizeof(pMouse->state), &pMouse->state) == DIERR_INPUTLOST)
	{
		pMouse->device.pDevice->Acquire();
		pMouse->device.pDevice->GetDeviceState(sizeof(pMouse->state), &pMouse->state);
	}

	for (i = 0; i < pMouse->numButtons; i++)
	{
		pMouse->buttonChanged[i] = (pMouse->oldButtons[i] != pMouse->state.rgbButtons[i]);
		pMouse->oldButtons[i] = pMouse->state.rgbButtons[i];
	}

	return TRUE;
}


/*
===========
IN_ShutdownMouse
===========
*/
void IN_ShutdownMouse( maplemouse_t *pMouse )
{
	if (pMouse->device.pDevice)
	{
		pMouse->device.pDevice->Unacquire();
		pMouse->device.pDevice->Release();
	}
}


/*
===========
IN_Init
===========
*/
/*
===========
IN_InitDeviceState

Set up a freshly allocated pad. Every usage slot starts unclaimed so that
whatever the object enumeration finds lands in the right place.
===========
*/
maplejoystick_t::maplejoystick_t( GUID guid, int type )
{
	int		i;

	device.guid = guid;
	device.port = JOY_USAGE_NONE;
	device.pDevice = NULL;
	numAxes = 0;
	numButtons = 0;

	for (i = 0; i < MAX_JOY_POVS; i++)
	{
		povUsage[i] = JOY_USAGE_NONE;
		pov[i][0] = 0;
		pov[i][1] = 0;
	}

	device.type = type & 0xff;

	memset(buttonUsage, JOY_USAGE_NONE, sizeof(buttonUsage));
	memset(axes, JOY_USAGE_NONE, sizeof(axes));
	memset(oldButtons, 0, sizeof(oldButtons));
	memset(buttonChanged, 0, sizeof(buttonChanged));
	memset(axisValue, 0, sizeof(axisValue));
}


/*
===========
IN_EnumAxesCallback

Called once for every object the pad reports. Axes are numbered in the order
they turn up; buttons are filed by the usage the driver gives them so that a
pad with a different button order still lands on the right keys.
===========
*/
BOOL IN_EnumAxesCallback( maplejoystick_t *pJoy, LPCDIDEVICEOBJECTINSTANCE lpddoi )
{
	int		usage;

	if (lpddoi->dwType & DIDFT_AXIS)
	{
		if (memcmp(&lpddoi->guidType, &GUID_XAxis, sizeof(GUID)) == 0)
			pJoy->axes[pJoy->numAxes].axis = 0;
		else
			pJoy->axes[pJoy->numAxes].axis = 1;

		pJoy->axes[pJoy->numAxes].usage = pJoy->numAxes;
		pJoy->numAxes++;
	}
	else if (lpddoi->dwType & DIDFT_BUTTON)
	{
		usage = lpddoi->wUsage - USAGE_FIRST_BUTTON;
		pJoy->buttonUsage[usage] = pJoy->numButtons;

		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 0)
			pJoy->povUsage[0] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 1)
			pJoy->povUsage[1] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 8)
			pJoy->povUsage[2] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 9)
			pJoy->povUsage[3] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 4)
			pJoy->povUsage[4] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 5)
			pJoy->povUsage[5] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 6)
			pJoy->povUsage[6] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 7)
			pJoy->povUsage[7] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 16)
			pJoy->povUsage[8] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 17)
			pJoy->povUsage[9] = pJoy->numButtons;
		if (lpddoi->wUsage - USAGE_FIRST_BUTTON == 3)
			pJoy->povUsage[10] = pJoy->numButtons;

		pJoy->numButtons++;
	}
	else
	{
		IN_DebugPrintf(TEXT("Port %d EnumObjects -- UnknownObject 0x%X\n"), pJoy->device.port);
	}

	return DIENUM_CONTINUE;
}


static BOOL CALLBACK IN_EnumAxesProc( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef )
{
	return IN_EnumAxesCallback((maplejoystick_t*)pvRef, lpddoi);
}


/*
===========
IN_ActivateJoystick

Bring up the pad on whichever port it turned up in, matching each object the
driver reports to the axis that uses it.
===========
*/
qboolean IN_ActivateJoystick( maplejoystick_t *pJoy )
{
	LPDIRECTINPUTDEVICE		did1;
	LPDIRECTINPUTDEVICE2	did2;
	DIPROPDWORD				prop;
	int						i;

	g_hResult = g_pDI->CreateDevice(pJoy->device.guid, &did1, NULL);
	if (IN_LogResult(TEXT("Create Device")))
		return FALSE;

	g_hResult = did1->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&did2);
	if (IN_LogResult(TEXT("Query Interface for DirectInputDevice2")))
	{
		did1->Release();
		return FALSE;
	}
	did1->Release();

	prop.diph.dwSize = sizeof(DIPROPDWORD);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = 0;
	g_hResult = did2->GetProperty(DIPROP_PORTNUMBER, &prop.diph);
	if (IN_LogResult(TEXT("Get Port Number")))
	{
		did2->Release();
		return FALSE;
	}

	pJoy->device.port = prop.dwData;
	g_MapleDevices[pJoy->device.port].stale = 0;
	if (g_MapleDevices[pJoy->device.port].present)
	{
		did2->Release();
		return FALSE;
	}

	pJoy->device.pDevice = did2;
	pJoy->caps.dwSize = sizeof(DIDEVCAPS);
	g_hResult = pJoy->device.pDevice->GetCapabilities(&pJoy->caps);
	if (IN_LogResult(TEXT("Get Device Capabilities")))
	{
		pJoy->device.pDevice->Release();
		return FALSE;
	}

	g_hResult = pJoy->device.pDevice->EnumObjects(IN_EnumAxesProc, pJoy, 0);
	if (IN_LogResult(TEXT("Enumerate Objects")))
	{
		pJoy->device.pDevice->Release();
		return FALSE;
	}

	g_hResult = pJoy->device.pDevice->SetDataFormat(&c_dfDIJoystick);
	if (IN_LogResult(TEXT("Set Data Format (Joystick)")))
	{
		pJoy->device.pDevice->Release();
		return FALSE;
	}

	for (i = 0; i < 4; i++)
	{
		if (pJoy->axes[i].usage != JOY_USAGE_NONE)
		{
			prop.diph.dwSize = sizeof(DIPROPDWORD);
			prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			prop.diph.dwHow = DIPH_BYOFFSET;
			prop.dwData = 1000;

			if (pJoy->axes[i].axis == 0)
			{
				pJoy->range[0].diph.dwSize = sizeof(DIPROPRANGE);
				pJoy->range[0].diph.dwHeaderSize = sizeof(DIPROPHEADER);
				pJoy->range[0].diph.dwHow = DIPH_BYOFFSET;
				pJoy->range[0].diph.dwObj = 0;
				g_hResult = pJoy->device.pDevice->GetProperty(DIPROP_RANGE, &pJoy->range[0].diph);
				if (IN_LogResult(TEXT("Get Digital X Range")))
				{
					pJoy->device.pDevice->Release();
					return FALSE;
				}

				prop.diph.dwObj = 0;
				g_hResult = pJoy->device.pDevice->SetProperty(DIPROP_DEADZONE, &prop.diph);
				if (IN_LogResult(TEXT("Set X DeadZone")))
				{
					pJoy->device.pDevice->Release();
					return FALSE;
				}
			}
			else if (pJoy->axes[i].axis == 1)
			{
				pJoy->range[1].diph.dwSize = sizeof(DIPROPRANGE);
				pJoy->range[1].diph.dwHeaderSize = sizeof(DIPROPHEADER);
				pJoy->range[1].diph.dwHow = DIPH_BYOFFSET;
				pJoy->range[1].diph.dwObj = 4;
				g_hResult = pJoy->device.pDevice->GetProperty(DIPROP_RANGE, &pJoy->range[1].diph);
				if (IN_LogResult(TEXT("Get Digital Y Range")))
				{
					pJoy->device.pDevice->Release();
					return FALSE;
				}

				prop.diph.dwObj = 4;
				g_hResult = pJoy->device.pDevice->SetProperty(DIPROP_DEADZONE, &prop.diph);
				if (IN_LogResult(TEXT("Set Y DeadZone")))
				{
					pJoy->device.pDevice->Release();
					return FALSE;
				}
			}
		}
	}

	g_hResult = pJoy->device.pDevice->Acquire();
	if (IN_LogResult(TEXT("Acquire port")))
	{
		pJoy->device.pDevice->Release();
		return FALSE;
	}

	return TRUE;
}


/*
===========
IN_ReleaseJoystick
===========
*/
void IN_ReleaseJoystick( maplejoystick_t *pJoy )
{
	if (pJoy->device.pDevice)
	{
		pJoy->device.pDevice->Unacquire();
		pJoy->device.pDevice->Release();
	}
}


/*
===========
IN_InitMapleDeviceRecord
===========
*/
maplekeyboard_t::maplekeyboard_t( GUID guid, int type )
{
	device.guid = guid;
	device.port = JOY_USAGE_NONE;
	device.type = type;
	device.pDevice = NULL;

	memset(state, 0, sizeof(state));
}


/*
===========
IN_CreateMapleDevice

Bring up the keyboard on whichever port it turned up in.
===========
*/
qboolean IN_CreateMapleDevice( maplekeyboard_t *pKbd )
{
	LPDIRECTINPUTDEVICE		did1;
	LPDIRECTINPUTDEVICE2	did2;
	DIPROPDWORD				prop;

	g_hResult = g_pDI->CreateDevice(pKbd->device.guid, &did1, NULL);
	if (IN_LogResult(TEXT("Create Device")))
		return FALSE;

	g_hResult = did1->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&did2);
	if (IN_LogResult(TEXT("Query Interface for DirectInputDevice2")))
	{
		did1->Release();
		return FALSE;
	}
	did1->Release();

	prop.diph.dwSize = sizeof(DIPROPDWORD);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = 0;
	g_hResult = did2->GetProperty(DIPROP_PORTNUMBER, &prop.diph);
	if (IN_LogResult(TEXT("Get Port Number")))
	{
		did2->Release();
		return FALSE;
	}

	pKbd->device.port = prop.dwData;
	g_MapleDevices[pKbd->device.port].stale = 0;
	if (g_MapleDevices[pKbd->device.port].present)
	{
		did2->Release();
		return FALSE;
	}

	pKbd->device.pDevice = did2;
	pKbd->caps.dwSize = sizeof(DIDEVCAPS);
	g_hResult = did2->GetCapabilities(&pKbd->caps);
	if (IN_LogResult(TEXT("Get Device Capabilities")))
	{
		pKbd->device.pDevice->Release();
		return FALSE;
	}

	g_hResult = pKbd->device.pDevice->SetDataFormat(&c_dfDIKeyboard);
	if (IN_LogResult(TEXT("Set Data Format (Keyboard)")))
	{
		pKbd->device.pDevice->Release();
		return FALSE;
	}

	g_hResult = pKbd->device.pDevice->Acquire();
	if (IN_LogResult(TEXT("Acquire port")))
	{
		pKbd->device.pDevice->Release();
		return FALSE;
	}

	return TRUE;
}


/*
===========
IN_ReleaseMapleDevice
===========
*/
void IN_ReleaseMapleDevice( maplekeyboard_t *pKbd )
{
	if (pKbd->device.pDevice)
	{
		pKbd->device.pDevice->Unacquire();
		pKbd->device.pDevice->Release();
	}
}


/*
===========
IN_InitMouseDevice
===========
*/
maplemouse_t::maplemouse_t( GUID guid, int type )
{
	device.guid = guid;
	device.port = JOY_USAGE_NONE;
	device.type = type;
	device.pDevice = NULL;
	numButtons = 0;
	numAxes = 0;

	memset(&state, 0, sizeof(state));
	memset(oldButtons, 0, sizeof(oldButtons));
	memset(buttonChanged, 0, sizeof(buttonChanged));
}


/*
===========
IN_EnumDevicesCallback

Called once for every device the driver reports. Whatever comes up is built,
brought online and filed against the port it is plugged into.
===========
*/
BOOL CALLBACK IN_EnumDevicesCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef )
{
	maplemouse_t		*pMouse;
	maplekeyboard_t		*pKbd;
	maplejoystick_t		*pJoy;
	mapleport_t			*pPort;

	switch (GET_DIDEVICE_TYPE(lpddi->dwDevType))
	{
	case DIDEVTYPE_MOUSE:
		pMouse = new maplemouse_t(lpddi->guidInstance, lpddi->dwDevType);

		if (!IN_StartupMouse(pMouse))
		{
			if (pMouse)
			{
				IN_ShutdownMouse(pMouse);
				delete pMouse;
			}
		}
		else
		{
			pPort = &g_MapleDevices[pMouse->device.port];
			pPort->stale = 0;
			pPort->present = (int)pMouse->device.pDevice;
			pPort->pDevice = pMouse;
			pPort->type = MAPLE_MOUSE;
		}
		break;

	case DIDEVTYPE_KEYBOARD:
		pKbd = new maplekeyboard_t(lpddi->guidInstance, lpddi->dwDevType);

		if (!IN_CreateMapleDevice(pKbd))
		{
			if (pKbd)
			{
				IN_ReleaseMapleDevice(pKbd);
				delete pKbd;
			}
		}
		else
		{
			pPort = &g_MapleDevices[pKbd->device.port];
			pPort->present = (int)pKbd->device.pDevice;
			pPort->pDevice = pKbd;
			pPort->type = MAPLE_KEYBOARD;
		}
		break;

	case DIDEVTYPE_JOYSTICK:
		pJoy = new maplejoystick_t(lpddi->guidInstance, lpddi->dwDevType);

		if (!IN_ActivateJoystick(pJoy))
		{
			if (pJoy)
			{
				IN_ReleaseJoystick(pJoy);
				delete pJoy;
			}
		}
		else
		{
			pPort = &g_MapleDevices[pJoy->device.port];
			pPort->stale = 0;
			pPort->present = (int)pJoy->device.pDevice;
			pPort->pDevice = pJoy;
			pPort->type = MAPLE_CONTROLLER;
		}
		break;

	default:
		IN_DebugPrintf(TEXT("Enum Devices: Unknown Device type\n"));
		break;
	}

	return DIENUM_CONTINUE;
}


/*
===========
IN_LogResult

Note the outcome of the last DirectInput call. The text is only wanted when
tracking down a device problem, so nothing is emitted.
===========
*/
qboolean IN_LogResult( LPCTSTR what )
{
	TCHAR	buf[256];

	if (g_hResult == g_hrExpected)
	{
		if (!g_bLogFailuresOnly)
			wsprintf(buf, TEXT("%s succeeded.\n"), what);
	}
	else
	{
		wsprintf(buf, TEXT("****%s failed (Error # = 0x%08x).\n"), what, g_hResult);
	}

	return g_hResult != g_hrExpected;
}


/*
===========
IN_UpdateMapleDevices

Re-scan the four sockets. Anything that has gone is released, and the first
pad, mouse and keyboard found become the ones the game reads.
===========
*/
void IN_UpdateMapleDevices( void )
{
	void	*pDevice;
	int		i;

	// assume everything already known has gone until the scan finds it again
	for (i = 0; i < MAPLE_MAX_PORTS; i++)
	{
		if (g_MapleDevices[i].present)
			g_MapleDevices[i].stale = 1;
	}

	g_pDI->EnumDevices(0, IN_EnumDevicesCallback, NULL, 0);
	g_pDI->EnumDevices(2, IN_EnumDevicesCallback, NULL, 0);

	// let go of whatever did not turn up this time
	for (i = 0; i < MAPLE_MAX_PORTS; i++)
	{
		if (g_MapleDevices[i].present && g_MapleDevices[i].stale == 1)
		{
			switch (g_MapleDevices[i].type)
			{
			case MAPLE_KEYBOARD:
				if (g_MapleDevices[i].pDevice == pKeyboardDevice)
					pKeyboardDevice = NULL;
				pDevice = g_MapleDevices[i].pDevice;
				if (pDevice)
				{
					IN_ReleaseMapleDevice((maplekeyboard_t*)pDevice);
					delete pDevice;
				}
				IN_DebugPrintf(TEXT("Keyboard removed from port %d\n"), i);
				break;
			case MAPLE_CONTROLLER:
				if (g_MapleDevices[i].pDevice == pJoystickDevice)
					pJoystickDevice = NULL;
				pDevice = g_MapleDevices[i].pDevice;
				if (pDevice)
				{
					IN_ReleaseJoystick((maplejoystick_t*)pDevice);
					delete pDevice;
				}
				IN_DebugPrintf(TEXT("Controller removed from port %d\n"), i);
				break;
			case MAPLE_MOUSE:
				if (g_MapleDevices[i].pDevice == pMouseDevice)
					pMouseDevice = NULL;
				pDevice = g_MapleDevices[i].pDevice;
				if (pDevice)
				{
					IN_ShutdownMouse((maplemouse_t*)pDevice);
					delete pDevice;
				}
				IN_DebugPrintf(TEXT("Mouse removed from port %d\n"), i);
				break;
			}

			memset(&g_MapleDevices[i], 0, sizeof(g_MapleDevices[i]));
		}
	}

	if (!pMouseDevice)
	{
		for (i = 0; i < MAPLE_MAX_PORTS; i++)
		{
			if (g_MapleDevices[i].type == MAPLE_MOUSE)
			{
				pMouseDevice = (maplemouse_t*)g_MapleDevices[i].pDevice;
				IN_DebugPrintf(TEXT("Mouse installed on port %d\n"), i);
				break;
			}
		}
	}

	if (!pKeyboardDevice)
	{
		for (i = 0; i < MAPLE_MAX_PORTS; i++)
		{
			if (g_MapleDevices[i].type == MAPLE_KEYBOARD)
			{
				pKeyboardDevice = g_MapleDevices[i].pDevice;
				IN_DebugPrintf(TEXT("Keyboard installed on port %d\n"), i);
				break;
			}
		}
	}

	for (i = 0; i < MAPLE_MAX_PORTS; i++)
	{
		if (g_MapleDevices[i].type == MAPLE_CONTROLLER)
		{
			pJoystickDevice = (maplejoystick_t*)g_MapleDevices[i].pDevice;
			IN_DebugPrintf(TEXT("Controller installed on port %d\n"), i);
			IN_StartupJoystick();
			break;
		}
	}

	VMU_ResetDeviceTable();
}


/*
===========
IN_CheckMapleHotplug
===========
*/
void IN_CheckMapleHotplug( void )
{
	HANDLE	handles[2];
	int		result;

	handles[0] = hNewDevice;
	handles[1] = hDeviceRemoved;

	result = WaitForMultipleObjects(2, handles, FALSE, 0);
	if (result != WAIT_FAILED
		&& (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + 1 || result != WAIT_TIMEOUT))
		IN_UpdateMapleDevices();
}


/*
===========
IN_StartupDevices

Bring up whatever is plugged into the Maple ports.
===========
*/
qboolean IN_StartupDevices( void )
{
	pJoystickDevice = NULL;
	pMouseDevice = NULL;
	pKeyboardDevice = NULL;

	g_hResult = DirectInputCreate(g_hInstance, DIRECTINPUT_VERSION, &g_pDI, NULL);
	if (IN_LogResult(TEXT("DirectInputCreate")) || !g_pDI)
	{
		if (hNewDevice)
		{
			CloseHandle(hNewDevice);
			hNewDevice = NULL;
		}
		if (hDeviceRemoved)
		{
			CloseHandle(hDeviceRemoved);
			hDeviceRemoved = NULL;
		}
		return FALSE;
	}

	memset(g_MapleDevices, 0, sizeof(g_MapleDevices));

	// pick up whatever is already plugged in
	IN_UpdateMapleDevices();

	hNewDevice = CreateEvent(NULL, FALSE, FALSE, TEXT("MAPLE_NEW_DEVICE"));
	if (!hNewDevice)
		return FALSE;

	hDeviceRemoved = CreateEvent(NULL, FALSE, FALSE, TEXT("MAPLE_DEVICE_REMOVED"));

	return hDeviceRemoved != NULL;
}

void IN_Init( void )
{
	// joystick variables
	Cvar_RegisterVariable(&in_joystick);
	Cvar_RegisterVariable(&joy_name);
	Cvar_RegisterVariable(&joy_advanced);
	Cvar_RegisterVariable(&joy_advaxisx);
	Cvar_RegisterVariable(&joy_advaxisy);
	Cvar_RegisterVariable(&joy_advaxisz);
	Cvar_RegisterVariable(&joy_advaxisr);
	Cvar_RegisterVariable(&joy_advaxisu);
	Cvar_RegisterVariable(&joy_advaxisv);
	Cvar_RegisterVariable(&joy_forwardthreshold);
	Cvar_RegisterVariable(&joy_sidethreshold);
	Cvar_RegisterVariable(&joy_pitchthreshold);
	Cvar_RegisterVariable(&joy_yawthreshold);
	Cvar_RegisterVariable(&joy_forwardsensitivity);
	Cvar_RegisterVariable(&joy_sidesensitivity);
	Cvar_RegisterVariable(&joy_pitchsensitivity);
	Cvar_RegisterVariable(&joy_yawsensitivity);
	Cvar_RegisterVariable(&joy_wwhack1);
	Cvar_RegisterVariable(&joy_wwhack2);

	// analog stick smoothing
	Cvar_RegisterVariable(&softstick);
	Cvar_RegisterVariable(&softaccel);
	Cvar_RegisterVariable(&softdamp);
	Cvar_RegisterVariable(&softstop);

	Cmd_AddCommand("force_centerview", Force_CenterView_f);
	Cmd_AddCommand("joyadvancedupdate", Joy_AdvancedUpdate_f);

	Cvar_RegisterVariable(&joyshift1);
	Cvar_RegisterVariable(&joyshift2);

	// The sticks read centred until the pad reports otherwise
	joy_centerx = 127;
	joy_centery = 127;

	IN_StartupDevices();
	VMU_InitDeviceTable();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void )
{
	IN_DeactivateMouse();
	IN_ShowMouse();

	if (s_di_kbd)
	{
		s_di_kbd->Unacquire();
		s_di_kbd->Release();
		s_di_kbd = NULL;
	}
    if (s_di_joy)
    {
        s_di_joy->Unacquire();
        s_di_joy->Release();
        s_di_joy = NULL;
    }
    if (s_di)
    {
        s_di->Release();
        s_di = NULL;
    }
	if (s_di_newdev_event)
	{
		CloseHandle(s_di_newdev_event);
		s_di_newdev_event = NULL;
	}
}


/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent( int mstate )
{
	int	i;

	if (mouseactive)
	{
	// perform button actions
		for (i = 0; i < mouse_buttons; i++)
		{
			if ((mstate & (1 << i)) &&
				!(mouse_oldbuttonstate & (1 << i)))
			{
				Key_Event(K_MOUSE1 + i, TRUE);
			}

			if (!(mstate & (1 << i)) &&
				(mouse_oldbuttonstate & (1 << i)))
			{
				Key_Event(K_MOUSE1 + i, FALSE);
			}
		}

		mouse_oldbuttonstate = mstate;
	}
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove( usercmd_t* cmd )
{
	int		mx, my;

	if (pMouseDevice)
	{
		mouse_pos.x = pMouseDevice->x;
		mouse_pos.y = pMouseDevice->y;
	}

	mx = mouse_pos.x - window_center_x + mx_accum;
	my = mouse_pos.y - window_center_y + my_accum;
	mx_accum = 0;
	my_accum = 0;

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x = mx * (gMouseSensitivity * 4.0f);
	mouse_y = my * (gMouseSensitivity * 4.0f);

// add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	V_StopPitchDrift();

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > cl_pitchdown.value)
			cl.viewangles[PITCH] = cl_pitchdown.value;
		if (cl.viewangles[PITCH] < -cl_pitchup.value)
			cl.viewangles[PITCH] = -cl_pitchup.value;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}

// if the mouse has moved, force it back to the centre so there is room to move
	if (mx || my)
	{
		if (pMouseDevice)
		{
			pMouseDevice->x = window_center_x;
			pMouseDevice->y = window_center_y;
		}

		Host_UpdateScreenSaver(FALSE);
	}
}


/*
===========
IN_Move
===========
*/
void IN_Move( usercmd_t* cmd )
{
	if (mouseactive)
		IN_MouseMove(cmd);

	IN_JoyMove(cmd);
}


/*
===========
IN_Accumulate
===========
*/
void IN_Accumulate( void )
{
	IN_ReadMouse();
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates( void )
{
}


/*
===============
IN_StartupJoystick
===============
*/
void IN_StartupJoystick( void )
{
	// abort startup if user requests no joystick
	if (COM_CheckParm("-nojoy"))
		return;

	// nothing plugged in yet; this runs again when one turns up
	if (!pJoystickDevice)
		return;

	// save the joystick's number of buttons and whether it has a hat switch
	joy_numbuttons = pJoystickDevice->caps.dwButtons;
	joy_haspov = pJoystickDevice->caps.dwPOVs;

	// old button state defaults to no buttons pressed
	joy_oldbuttonstate = 0;

	// mark advanced initialization as not completed
	// this is needed as cvars are not available during initialization
	joy_advancedinit = FALSE;
}


/*
===========
RawValuePointer
===========
*/
PDWORD RawValuePointer( int axis )
{
    if (axis < 0) axis = 0;
    if (axis >= JOY_MAX_AXES) axis = JOY_MAX_AXES - 1;
    return &s_joy_raw[axis];
}


/*
===========
Joy_AdvancedUpdate_f
===========
*/
void Joy_AdvancedUpdate_f( void )
{
	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
	}

	if (joy_advanced.value == 0.0f)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (strcmp(joy_name.string, "joystick") != 0)
		{
			// notify user of advanced controller
			IN_DebugPrintf(TEXT("%s configured\n"), joy_name.string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD)joy_advaxisx.value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisy.value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
	}
}


/*
===========
IN_ReadMouse

Fold this frame's mouse movement into the cursor, and turn the wheel and
buttons into key events.
===========
*/
void IN_ReadMouse( void )
{
	int		i;
	int		delta;
	int		wheel;

	if (!pMouseDevice)
		return;

	IN_ReadMouseState(pMouseDevice);

	pMouseDevice->x += pMouseDevice->state.lX;
	pMouseDevice->y += pMouseDevice->state.lY;

	// only a real move counts against the screen saver
	delta = mouse_lastx - pMouseDevice->x;
	if (delta < 0)
		delta = -delta;
	if (delta < 5)
	{
		delta = mouse_lasty - pMouseDevice->y;
		if (delta < 0)
			delta = -delta;
		if (delta < 5)
			goto settled;
	}
	Host_UpdateScreenSaver(FALSE);

settled:
	mouse_lastx = pMouseDevice->x;
	mouse_lasty = pMouseDevice->y;

	// the wheel reports a position rather than clicks, so turn the change into
	// a press of whichever direction it moved
	wheel = pMouseDevice->state.lZ;
	if (wheel != pMouseDevice->z)
	{
		if (wheel == 0)
		{
			if (pMouseDevice->z < 1)
				Key_Event(K_MWHEELDOWN, FALSE);
			else
				Key_Event(K_MWHEELUP, FALSE);
		}
		else if (wheel < 1)
		{
			if (pMouseDevice->z > 0)
				Key_Event(K_MWHEELUP, FALSE);
			Key_Event(K_MWHEELDOWN, TRUE);
		}
		else
		{
			if (pMouseDevice->z < 0)
				Key_Event(K_MWHEELDOWN, FALSE);
			Key_Event(K_MWHEELUP, TRUE);
		}
		pMouseDevice->z = wheel;
	}

	for (i = 0; i < 4; i++)
	{
		if (pMouseDevice->buttonChanged[i])
		{
			if (pMouseDevice->oldButtons[i])
				Key_Event(K_MOUSE1 + i, TRUE);
			else
				Key_Event(K_MOUSE1 + i, FALSE);
		}
	}
}


/*
===========
IN_KeyboardActive
===========
*/
qboolean IN_KeyboardActive( void )
{
	return pKeyboardDevice != NULL;
}


/*
===========
IN_JoystickActive
===========
*/
qboolean IN_JoystickActive( void )
{
	return pJoystickDevice != NULL;
}


/*
===========
IN_Commands
===========
*/
void IN_Commands( void )
{
	int			i, j, key;
	int			modifier;
	int			value;
	int			held;
	int			bShift1, bShift2;
	int			bShifted;
	int			changed[MAX_JOY_POVS];
	int			pressed[MAX_JOY_POVS];

	IN_CheckMapleHotplug();
	VMU_UpdateDeviceIcons();
	IN_ReadMouse();

	if (!pJoystickDevice)
		return;

	IN_ReadJoystick(pJoystickDevice);

	for (i = 0; i < MAX_JOY_POVS; i++)
	{
		changed[i] = pJoystickDevice->buttonChanged[pJoystickDevice->povUsage[i]];
		pressed[i] = pJoystickDevice->oldButtons[pJoystickDevice->povUsage[i]];
	}

	// find which pad positions the two shift buttons are bound to
	key = Key_StringToKeynum(joyshift1.string);
	joyshift1count = 0;
	if (key != -1)
	{
		for (j = 0; j < MAX_JOY_POVS; j++)
		{
			if (key == joykeys[j])
			{
				joyshift1keys[joyshift1count++] = j;
				break;
			}
		}
	}

	key = Key_StringToKeynum(joyshift2.string);
	joyshift2count = 0;
	if (key != -1)
	{
		for (j = 0; j < MAX_JOY_POVS; j++)
		{
			if (key == joykeys[j])
			{
				joyshift2keys[joyshift2count++] = j;
				break;
			}
		}
	}

	// a shift is in effect only while every button bound to it is held, and the
	// buttons themselves stop reporting as ordinary presses
	held = 0;
	bShift1 = FALSE;
	for (i = 0; i < joyshift1count; i++)
	{
		if (pressed[joyshift1keys[i]])
			held++;
		changed[joyshift1keys[i]] = 0;
	}
	if (held > 0 && held == joyshift1count)
		bShift1 = TRUE;

	held = 0;
	bShift2 = FALSE;
	for (i = 0; i < joyshift2count; i++)
	{
		if (pressed[joyshift2keys[i]])
			held++;
		changed[joyshift2keys[i]] = 0;
	}
	if (held > 0 && held == joyshift2count)
		bShift2 = TRUE;

	modifier = 0;
	if (bShift1 && bShift2)
		modifier = K_JOYSHIFT12;
	else if (bShift1)
		modifier = K_JOYSHIFT1;
	else if (bShift2)
		modifier = K_JOYSHIFT2;

	bShifted = (modifier != 0);
	if (bShifted)
		Host_UpdateScreenSaver(FALSE);

	// a button pressed while shifted keeps sending the shifted key until it is
	// let go, even if the shift button is released first
	for (i = 0; i < MAX_JOY_POVS; i++)
	{
		if (changed[i])
		{
			if (pJoystickDevice->pov[i][0] == 0)
			{
				if (!pressed[i])
				{
					Key_Event(joybuttonkeys[joyshiftmap[i]], FALSE);
				}
				else if (bShifted)
				{
					pJoystickDevice->pov[i][0] = 1;
					pJoystickDevice->pov[i][1] = i + modifier;
					Key_Event(i + modifier, TRUE);
				}
				else
				{
					Key_Event(joybuttonkeys[joyshiftmap[i]], TRUE);
				}
				changed[i] = 0;
			}
			else
			{
				pJoystickDevice->pov[i][0] = 0;
				changed[i] = 0;
				Key_Event(pJoystickDevice->pov[i][1], FALSE);
			}
		}
	}

	// the buttons that are not shiftable report straight through
	for (i = 0; i < MAX_JOY_BUTTONS; i++)
	{
		if (pJoystickDevice->buttonUsage[i] != JOY_USAGE_NONE
			&& i != 0 && i != 1 && i != 8 && i != 9
			&& i != 4 && i != 5 && i != 6 && i != 7
			&& i != 16 && i != 17 && i != 3
			&& pJoystickDevice->buttonChanged[pJoystickDevice->buttonUsage[i]])
		{
			if (pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[i]])
				Key_Event(joybuttonkeys[i], TRUE);
			else
				Key_Event(joybuttonkeys[i], FALSE);
		}
	}

	// tell the menu which buttons are down, unless something else owns the pad
	for (i = 0; i < MAX_JOY_BUTTONS; i++)
	{
		joymenubuttons[i] = 0;
		if (key_dest != key_message)
		{
			if (pJoystickDevice->buttonUsage[i] != JOY_USAGE_NONE
				&& pJoystickDevice->buttonChanged[pJoystickDevice->buttonUsage[i]]
				&& pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[i]])
			{
				joymenubuttons[i] = 1;
			}
		}
	}

	// holding the whole face of the pad down drops back to the menu, or opens
	// the drive door when there is nothing to drop back to
	if (pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[0]]
		&& pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[1]]
		&& pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[8]]
		&& pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[9]]
		&& pJoystickDevice->oldButtons[pJoystickDevice->buttonUsage[3]])
	{
		if (cls.state == ca_active && sv.state != ss_active)
			Cbuf_AddText("disconnect\nmenu splash");
		else
			GDROM_DoorReset();
	}

	M_DecodeStateFlags();

	if (!pJoystickDevice->numAxes)
		return;

	if (gnControllerGrace)
		return;

	// the stick has to come back to the middle before it counts as pushed again
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		if (pJoystickDevice->axes[i].usage == JOY_USAGE_NONE)
			continue;

		value = pJoystickDevice->axisValue[pJoystickDevice->axes[i].usage] - 127;

		if (pJoystickDevice->axes[i].axis == 0)
		{
			if (value < -7 || value > 7)
				Host_UpdateScreenSaver(FALSE);

			if (value > 120 && joy_lastx < 7)
			{
				joymenubuttons[13] = 1;
				joy_lastx = value;
			}
			else if (value < -120 && joy_lastx > -7)
			{
				joymenubuttons[12] = 1;
				joy_lastx = value;
			}
			else if (value > -7 && value < 7)
			{
				joy_lastx = value;
			}
		}
		else if (pJoystickDevice->axes[i].axis == 1)
		{
			if (value < -7 || value > 7)
				Host_UpdateScreenSaver(FALSE);

			if (value > 120 && joy_lasty < 7)
			{
				joymenubuttons[14] = 1;
				joy_lasty = value;
			}
			else if (value < -120 && joy_lasty > -7)
			{
				joymenubuttons[15] = 1;
				joy_lasty = value;
			}
			else if (value > -7 && value < 7)
			{
				joy_lasty = value;
			}
		}
	}
}


/*
===============
IN_ReadJoystick
===============
*/
qboolean IN_ReadJoystick( maplejoystick_t *pJoy )
{
	DIJOYSTATE	js;
	int			i;
	int			usage;
	int			value;

	// the pad is handed back to us whenever the game loses focus
	if (pJoy->device.pDevice->GetDeviceState(sizeof(js), &js) == DIERR_INPUTLOST)
	{
		pJoy->device.pDevice->Acquire();
		pJoy->device.pDevice->GetDeviceState(sizeof(js), &js);
	}

	for (i = 0; i < MAX_JOY_BUTTONS; i++)
	{
		usage = pJoy->buttonUsage[i];
		if (usage != JOY_USAGE_NONE)
		{
			if (i == 16 || i == 17)
			{
				// the triggers report how far they are held; only the top bit
				// of that is a press
				pJoy->buttonChanged[usage] = (pJoy->oldButtons[usage] != (js.rgbButtons[usage] & 0x80));
				pJoy->oldButtons[usage] = js.rgbButtons[usage] & 0x80;
			}
			else
			{
				pJoy->buttonChanged[usage] = (pJoy->oldButtons[usage] != js.rgbButtons[usage]);
				pJoy->oldButtons[usage] = js.rgbButtons[usage];
			}
		}
	}

	for (i = 0; i < 4; i++)
	{
		usage = pJoy->axes[i].usage;
		if (usage != JOY_USAGE_NONE)
		{
			if (pJoy->axes[i].axis == 0)
				value = js.lX;
			else if (pJoy->axes[i].axis == 1)
				value = js.lY;

			pJoy->axisValue[usage] = value;
		}
	}

	return TRUE;
}


/*
===========
IN_JoyMove
===========
*/
float IN_ApplySoftDamp( float current, float target )
{
	int			bMoving;

	// A stick that is being held winds up towards where it is pushed. One that
	// has been let go decays back towards the centre, and snaps to it once the
	// remaining deflection is too small to be worth reporting.
	bMoving = (target >= 0.01f || target <= -0.01f);
	if (bMoving)
	{
		current = current + (target - current) * softaccel.value;
	}
	else
	{
		current = current * softdamp.value;
		if (current < softstop.value && current > -softstop.value)
			current = 0.0f;
	}

	return current;
}


/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove( usercmd_t *cmd )
{
	float	speed, aspeed, aspeedkey;
	float	fAxisValue, fTemp;
	maplejoyaxis_t	*pAxis;
	int		i;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (!joy_advancedinit)
	{
		Joy_AdvancedUpdate_f();
		joy_advancedinit = TRUE;
	}

	// verify the pad is plugged in and that we still hold it
	if (!pJoystickDevice || !pJoystickDevice->numAxes)
		return;

	if (in_speed.state & 1)
	{
		aspeedkey = cl_anglespeedkey.value;
		speed = cl_movespeedkey.value;
		aspeed = host_frametime * aspeedkey;
	}
	else
	{
		speed = 1.0f;
		aspeedkey = 1.0f;
		aspeed = host_frametime;
	}

	joy_forwarddir = 0;
	joy_sidedir = 0;

	// don't act on the sticks while the pad is still being counted as missing
	if (gnControllerGrace)
		return;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		pAxis = &pJoystickDevice->axes[i];
		if (pAxis->axis == JOY_USAGE_NONE)
			continue;

		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = ((float)pJoystickDevice->axisValue[pAxis->usage] - 127.0f) / 127.0f;

		if (softstick.value)
		{
			fAxisValue = IN_ApplySoftDamp(joy_softvalue[i], fAxisValue);
			joy_softvalue[i] = fAxisValue;
		}
		else
		{
			// raw stick: just ease off the first part of the throw
			fTemp = fabs(fAxisValue);
			if (fTemp < 0.4f)
				fAxisValue = 0.2f * fTemp * (fAxisValue / fTemp);
		}

		switch (dwAxisMap[i])
		{
		case AxisTurn:
			if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold.value)
				{
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
					if (fAxisValue <= 0.0f)
						joy_sidedir = -1;
					else
						joy_sidedir = 1;
				}
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold.value)
				{
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * gJoySensitivity * aspeed * cl_yawspeed.value;
					else
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * gJoySensitivity * aspeedkey * 180.0f;
				}
			}
			break;
		case AxisForward:
			if (!(in_mlook.state & 1))
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold.value)
				{
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is 0)
					if (m_pitch.value < 0.0f)
						cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity.value) * gJoySensitivity * aspeed * cl_pitchspeed.value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * gJoySensitivity * aspeed * cl_pitchspeed.value;
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (!lookspring.value)
						V_StopPitchDrift();
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold.value)
				{
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.value) * speed * cl_forwardspeed.value;
					if ((fAxisValue * joy_forwardsensitivity.value) > 0.0f)
						joy_forwarddir = 1;
					else
						joy_forwarddir = -1;
				}
			}
			break;
		case AxisLook:
			if (in_mlook.state & 1)
			{
				if (fabs(fAxisValue) > joy_pitchthreshold.value)
				{
					// pitch movement detected and pitch movement desired by user
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * gJoySensitivity * aspeed * cl_pitchspeed.value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * gJoySensitivity * aspeedkey * 180.0f;
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (!lookspring.value)
						V_StopPitchDrift();
				}
			}
			break;
		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold.value)
			{
				cmd->sidemove += (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
				if (fAxisValue > 0.0f)
					joy_sidedir = 1;
				else
					joy_sidedir = -1;
			}
			break;
		default:
			break;
		}
	}

	// bounds check pitch
	if (cl.viewangles[PITCH] > cl_pitchdown.value)
		cl.viewangles[PITCH] = cl_pitchdown.value;
	if (cl.viewangles[PITCH] < -cl_pitchup.value)
		cl.viewangles[PITCH] = -cl_pitchup.value;
}
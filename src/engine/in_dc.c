// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "quakedef.h"
#include "winquake.h"
#include <dinput.h>
#ifdef _WIN32_WCE
#include <maplusag.h>
#endif 
extern int iMouseInUse;

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
cvar_t	m_filter = { "m_filter", "0" };

int			mouse_buttons;
int			mouse_oldbuttonstate;
POINT		current_pos;
int			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static qboolean	restore_spi;
static int		originalmouseparms[3], newmouseparms[3] = { 0, 0, 1 };
qboolean	mouseactive;
qboolean		mouseinitialized;
static qboolean	mouseparmsvalid, mouseactivatetoggle;
static int	mouseshowtoggle = 1;

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

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
cvar_t	in_joystick = { "joystick", "1" };
cvar_t	joy_name = { "joyname", "joystick" };
cvar_t	joy_advanced = { "joyadvanced", "0" };
cvar_t	joy_advaxisx = { "joyadvaxisx", "0" };
cvar_t	joy_advaxisy = { "joyadvaxisy", "0" };
cvar_t	joy_advaxisz = { "joyadvaxisz", "0" };
cvar_t	joy_advaxisr = { "joyadvaxisr", "0" };
cvar_t	joy_advaxisu = { "joyadvaxisu", "0" };
cvar_t	joy_advaxisv = { "joyadvaxisv", "0" };
cvar_t	joy_forwardthreshold = { "joyforwardthreshold", "0.15" };
cvar_t	joy_sidethreshold = { "joysidethreshold", "0.15" };
cvar_t	joy_pitchthreshold = { "joypitchthreshold", "0.15" };
cvar_t	joy_yawthreshold = { "joyyawthreshold", "0.15" };
cvar_t	joy_forwardsensitivity = { "joyforwardsensitivity", "-1.0" };
cvar_t	joy_sidesensitivity = { "joysidesensitivity", "-1.0" };
cvar_t	joy_pitchsensitivity = { "joypitchsensitivity", "1.0" };
cvar_t	joy_yawsensitivity = { "joyyawsensitivity", "-1.0" };
cvar_t	joy_wwhack1 = { "joywwhack1", "0.0" };
cvar_t	joy_wwhack2 = { "joywwhack2", "0.0" };
cvar_t	in_didebug = { "in_didebug", "0" };

int			joy_avail, joy_advancedinit, joy_haspov;
DWORD		joy_oldbuttonstate, joy_oldpovstate;

int			joy_id;
DWORD		joy_flags;
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
	hr = s_di->lpVtbl->CreateDevice(s_di, &GUID_SysKeyboard, &pDev1, NULL);
	if (FAILED(hr) || !pDev1)
	{
		/* Some WinCE builds don’t expose GUID_SysKeyboard; fall back to enumeration. */
		haveGuid = 0;
		memset(&kbdGuid, 0, sizeof(kbdGuid));
		s_di->lpVtbl->EnumDevices(s_di, DIDEVTYPE_KEYBOARD, IN_DI_EnumKeyboardProc, &kbdGuid, 0);

		/* A zero GUID likely means we didn’t find anything. */
		if (!IN_DI_IsZeroGuid(&kbdGuid))
			haveGuid = 1;

		if (!haveGuid)
			return;

		pDev1 = NULL;
		hr = s_di->lpVtbl->CreateDevice(s_di, &kbdGuid, &pDev1, NULL);
		if (FAILED(hr) || !pDev1)
			return;
	}
	if (FAILED(hr) || !pDev1)
		return;

	hr = pDev1->lpVtbl->QueryInterface(pDev1, &IID_IDirectInputDevice2, (LPVOID*)&s_di_kbd);
	pDev1->lpVtbl->Release(pDev1);
	if (FAILED(hr) || !s_di_kbd)
	{
		s_di_kbd = NULL;
		return;
	}

	hr = s_di_kbd->lpVtbl->SetDataFormat(s_di_kbd, &c_dfDIKeyboard);
	if (FAILED(hr))
	{
		s_di_kbd->lpVtbl->Release(s_di_kbd);
		s_di_kbd = NULL;
		return;
	}

	memset(s_kbd_state, 0, sizeof(s_kbd_state));
	memset(s_kbd_oldstate, 0, sizeof(s_kbd_oldstate));

	/* No cooperative level on WinCE sample; just acquire. */
	s_di_kbd->lpVtbl->Acquire(s_di_kbd);
}

static void IN_ReadKeyboard(void)
{
	HRESULT hr;
	int i;

	if (!s_di_kbd)
		return;

	hr = s_di_kbd->lpVtbl->Poll(s_di_kbd);
	if (FAILED(hr))
	{
		s_di_kbd->lpVtbl->Acquire(s_di_kbd);
		hr = s_di_kbd->lpVtbl->Poll(s_di_kbd);
	}
	if (FAILED(hr))
		return;

	memset(s_kbd_state, 0, sizeof(s_kbd_state));
	hr = s_di_kbd->lpVtbl->GetDeviceState(s_di_kbd, sizeof(s_kbd_state), (LPVOID)s_kbd_state);
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
	return 1;
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
void IN_StartupMouse( void )
{
	if (COM_CheckParm("-nomouse"))
		return;
	
	mouseinitialized = TRUE;

	mouseparmsvalid = SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0);

	if (mouseparmsvalid)
	{
		if (COM_CheckParm("-noforcemspd"))
			newmouseparms[2] = originalmouseparms[2];

		if (COM_CheckParm("-noforcemaccel"))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
		}

		if (COM_CheckParm("-noforcemparms"))
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
			newmouseparms[2] = originalmouseparms[2];
		}
	}

	mouse_buttons = 3;
}


/*
===========
IN_Init
===========
*/
void IN_Init( void )
{
	// mouse variables
	Cvar_RegisterVariable(&m_filter);

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
	Cvar_RegisterVariable(&in_didebug);

	Cmd_AddCommand("force_centerview", Force_CenterView_f);
	Cmd_AddCommand("joyadvancedupdate", Joy_AdvancedUpdate_f);

	IN_StartupMouse();
	IN_StartupJoystick();
	IN_StartupKeyboard();
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
		s_di_kbd->lpVtbl->Unacquire(s_di_kbd);
		s_di_kbd->lpVtbl->Release(s_di_kbd);
		s_di_kbd = NULL;
	}
    if (s_di_joy)
    {
        s_di_joy->lpVtbl->Unacquire(s_di_joy);
        s_di_joy->lpVtbl->Release(s_di_joy);
        s_di_joy = NULL;
    }
    if (s_di)
    {
        s_di->lpVtbl->Release(s_di);
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
	int					mx, my;

	if (iMouseInUse)
		return;

	GetCursorPos(&current_pos);
	mx = current_pos.x + mx_accum - window_center_x;
	my = current_pos.y + my_accum - window_center_y;
	mx_accum = 0;
	my_accum = 0;

//	if (mx || my)
//		Con_DPrintf("mx=%d, my=%d\n", mx, my);

	if (m_filter.value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

// add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	if (in_mlook.state & 1)
		V_StopPitchDrift();

	if ((in_mlook.state & 1) && !(in_strafe.state & 1))
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

// if the mouse has moved, force it to the center, so there's room to move
	if (mx || my)
	{
		SetCursorPos(window_center_x, window_center_y);
	}
}


/*
===========
IN_Move
===========
*/
void IN_Move( usercmd_t* cmd )
{
	if (!iMouseInUse && mouseactive)
	{
		IN_MouseMove(cmd);
	}

	IN_JoyMove(cmd);
}


/*
===========
IN_Accumulate
===========
*/
void IN_Accumulate( void )
{
	//only accumulate mouse if we are not moving the camera with the mouse
	if (!iMouseInUse)
	{
		if (mouseactive)
		{
			GetCursorPos(&current_pos);

			mx_accum += current_pos.x - window_center_x;
			my_accum += current_pos.y - window_center_y;

		// force the mouse to the center, so there's room to move
			SetCursorPos(window_center_x, window_center_y);
		}
	}
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates( void )
{
	if (mouseactive)
	{
		mx_accum = 0;
		my_accum = 0;
		mouse_oldbuttonstate = 0;
	}
}


/*
===============
IN_StartupJoystick
===============
*/
void IN_StartupJoystick( void )
{
	HRESULT hr;
	LPDIRECTINPUTDEVICE did1;
	int i;

	// assume no joystick
	joy_avail = FALSE;

	// abort startup if user requests no joystick
	if (COM_CheckParm("-nojoy"))
		return;

	/* Create the event that is triggered when a device is added. */
	if (!s_di_newdev_event)
		s_di_newdev_event = CreateEvent(NULL, FALSE, FALSE, TEXT("MAPLE_NEW_DEVICE"));

	/* Ensure DirectInput object exists (used by keyboard too). */
	if (!IN_DI_EnsureDirectInput())
		return;

	/* Enumerate devices to find a controller. */
	s_joy_guid_valid = 0;
    s_di->lpVtbl->EnumDevices(s_di, 0, IN_DI_EnumDevicesProc, NULL, 0);
	if (!s_joy_guid_valid)
	{
		Con_DPrintf("\njoystick not found -- no DirectInput devices\n\n");
		return;
	}

	/* Create device and get IDirectInputDevice2. */
	did1 = NULL;
    hr = s_di->lpVtbl->CreateDevice(s_di, &s_joy_guid, &did1, NULL);
	if (FAILED(hr) || !did1)
		return;
    hr = did1->lpVtbl->QueryInterface(did1, &IID_IDirectInputDevice2, (LPVOID*)&s_di_joy);
    did1->lpVtbl->Release(did1);
	if (FAILED(hr) || !s_di_joy)
		return;

	/* Use joystick format and acquire. */
    hr = s_di_joy->lpVtbl->SetDataFormat(s_di_joy, &c_dfDIJoystick);
	if (FAILED(hr))
	{
        s_di_joy->lpVtbl->Release(s_di_joy);
		s_di_joy = NULL;
		return;
	}
    /* Build usage->offset maps so we read the right fields on Dreamcast. */
	s_di_ofs_x = s_di_ofs_y = s_di_ofs_rx = s_di_ofs_ry = s_di_ofs_slider0 = s_di_ofs_pov0 = -1;
	for (i = 0; i < DI_BUTTON_USAGE_COUNT; i++)
		s_di_ofs_button_usage[i] = -1;
	s_di_joy->lpVtbl->EnumObjects(s_di_joy, IN_DI_EnumObjectsProc, NULL, 0);

    s_di_joy->lpVtbl->Acquire(s_di_joy);

	memset(&s_di_state, 0, sizeof(s_di_state));
	memset(s_joy_raw, 0, sizeof(s_joy_raw));
	s_joy_buttons = 0;
	s_joy_pov = 0xFFFF;

	joy_numbuttons = 32;
	joy_haspov = 1;
	joy_oldbuttonstate = 0;
	joy_oldpovstate = 0;
	joy_avail = TRUE;
	joy_advancedinit = 0;
	Con_Printf("\nDirectInput joystick found\n\n");
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
		pdwRawValue[i] = &s_joy_raw[i];
	}

	if (joy_advanced.value == 0.0)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		/* Dreamcast default: left stick Y controls look pitch (mlook-style). */
		dwAxisMap[JOY_AXIS_Y] = AxisLook;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (Q_strcmp(joy_name.string, "joystick") != 0)
		{
			// notify user of advanced controller
			Con_Printf("\n%s configured\n\n", joy_name.string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD)joy_advaxisx.value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisy.value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisz.value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisr.value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisu.value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD)joy_advaxisv.value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

    /* DirectInput path: we always poll full DIJOYSTATE; no JOY_* flags. */
    joy_flags = 0;
}


/*
===========
IN_Commands
===========
*/
void IN_Commands( void )
{
	int		i, key_index;
	DWORD	buttonstate, povstate;

	/* Keyboard input is independent of controller availability. */
	IN_ReadKeyboard();

	if (!joy_avail)
		return;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = s_joy_buttons;
	for (i = 0; i < (int)joy_numbuttons; i++)
	{
		if ((buttonstate & (1 << i)) && !(joy_oldbuttonstate & (1 << i)))
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event(key_index + i, TRUE);
		}

		if (!(buttonstate & (1 << i)) && (joy_oldbuttonstate & (1 << i)))
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event(key_index + i, FALSE);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if (s_joy_pov != 0xFFFF)
		{
			if (s_joy_pov == 0)
				povstate |= 0x01;
			if (s_joy_pov == 9000)
				povstate |= 0x02;
			if (s_joy_pov == 18000)
				povstate |= 0x04;
			if (s_joy_pov == 27000)
				povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i = 0; i < 4; i++)
		{
			if ((povstate & (1 << i)) && !(joy_oldpovstate & (1 << i)))
			{
				Key_Event(K_AUX29 + i, TRUE);
			}

			if (!(povstate & (1 << i)) && (joy_oldpovstate & (1 << i)))
			{
				Key_Event(K_AUX29 + i, FALSE);
			}
		}
		joy_oldpovstate = povstate;
	}
}


/*
===============
IN_ReadJoystick
===============
*/
qboolean IN_ReadJoystick( void )
{
	HRESULT hr;
	LONG lx, ly;
	const BYTE *base;
	int ofs;
	int usage_idx;
	int up, down, left, right;
	static int s_last_dump_frame = -1;

	if (!s_di_joy)
		return FALSE;

    hr = s_di_joy->lpVtbl->Poll(s_di_joy);
	if (FAILED(hr))
	{
        s_di_joy->lpVtbl->Acquire(s_di_joy);
        hr = s_di_joy->lpVtbl->Poll(s_di_joy);
	}
	if (FAILED(hr))
		return FALSE;

	memset(&s_di_state, 0, sizeof(s_di_state));
    hr = s_di_joy->lpVtbl->GetDeviceState(s_di_joy, sizeof(s_di_state), &s_di_state);
	if (FAILED(hr))
		return FALSE;

	base = (const BYTE *)&s_di_state;

	/* Axes: prefer usage-mapped offsets; fall back to standard DIJOFS_* fields. */
	ofs = (s_di_ofs_x >= 0) ? s_di_ofs_x : (int)DIJOFS_X;
	lx = *(const LONG *)(base + ofs);
	ofs = (s_di_ofs_y >= 0) ? s_di_ofs_y : (int)DIJOFS_Y;
	ly = *(const LONG *)(base + ofs);

	/* Some devices report the main stick as Rx/Ry or Slider. */
	if ((lx == 0 && ly == 0) && (s_di_ofs_rx >= 0 && s_di_ofs_ry >= 0))
	{
		lx = *(const LONG *)(base + s_di_ofs_rx);
		ly = *(const LONG *)(base + s_di_ofs_ry);
	}
	if ((lx == 0 && ly == 0) && (s_di_ofs_slider0 >= 0))
	{
		lx = *(const LONG *)(base + s_di_ofs_slider0);
		ly = 0;
	}

	/*
	 * Dreamcast MAPLE devices often report analog axes as 8-bit unsigned (0..255)
	 * with a center around 128. Normalize that into a signed 16-bit-ish range
	 * so the existing Quake joystick math works (-32768..32767 after subtracting 32768).
	 */
	if ((lx >= 0 && lx <= 255) && (ly >= 0 && ly <= 255))
	{
		lx = (lx - 128) * 256;
		/* Dreamcast Y axis is typically inverted (up = smaller). */
		ly = (128 - ly) * 256;
	}

	s_joy_raw[JOY_AXIS_X] = (DWORD)(lx + 32768L);
	s_joy_raw[JOY_AXIS_Y] = (DWORD)(ly + 32768L);

	/* Remaining axes keep best-effort defaults. */
	s_joy_raw[JOY_AXIS_Z] = (DWORD)(s_di_state.lZ + 32768L);
	s_joy_raw[JOY_AXIS_R] = (DWORD)(s_di_state.lRz + 32768L);
	s_joy_raw[JOY_AXIS_U] = (DWORD)(s_di_state.rglSlider[0] + 32768L);
	s_joy_raw[JOY_AXIS_V] = (DWORD)(s_di_state.rglSlider[1] + 32768L);

#if defined(_WIN32_WCE)
	/* Buttons: build a stable logical layout using USAGE_* offsets (Dreamcast MAPLE). */
	s_joy_buttons = 0;
	usage_idx = (int)(USAGE_A_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT &&
		s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 0);
	usage_idx = (int)(USAGE_B_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT &&
		s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 1);
	usage_idx = (int)(USAGE_X_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT &&
		s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 2);
	usage_idx = (int)(USAGE_Y_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT &&
		s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 3);

	usage_idx = (int)(USAGE_START_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 4);
	usage_idx = (int)(USAGE_LTRIG_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 5);
	usage_idx = (int)(USAGE_RTRIG_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80))
		s_joy_buttons |= (1u << 6);

	s_joy_pov = 0xFFFF;
	if (s_di_ofs_pov0 >= 0)
		s_joy_pov = *(const DWORD *)(base + s_di_ofs_pov0);

	up = down = left = right = 0;
	usage_idx = (int)(USAGE_UA_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80)) up = 1;
	usage_idx = (int)(USAGE_DA_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80)) down = 1;
	usage_idx = (int)(USAGE_LA_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80)) left = 1;
	usage_idx = (int)(USAGE_RA_BUTTON - USAGE_FIRST_BUTTON);
	if (usage_idx >= 0 && usage_idx < DI_BUTTON_USAGE_COUNT && s_di_ofs_button_usage[usage_idx] >= 0 && (base[s_di_ofs_button_usage[usage_idx]] & 0x80)) right = 1;

	if (s_joy_pov == 0xFFFF)
	{
		if (up && !down && !left && !right) s_joy_pov = 0;
		else if (right && !left && !up && !down) s_joy_pov = 9000;
		else if (down && !up && !left && !right) s_joy_pov = 18000;
		else if (left && !right && !up && !down) s_joy_pov = 27000;
	}
#else
	/* Win32: use standard DIJOYSTATE layout. */
	s_joy_buttons = 0;
	if (s_di_state.rgbButtons[0] & 0x80) s_joy_buttons |= (1u << 0);
	if (s_di_state.rgbButtons[1] & 0x80) s_joy_buttons |= (1u << 1);
	if (s_di_state.rgbButtons[2] & 0x80) s_joy_buttons |= (1u << 2);
	if (s_di_state.rgbButtons[3] & 0x80) s_joy_buttons |= (1u << 3);
	if (joy_numbuttons > 4 && (s_di_state.rgbButtons[4] & 0x80)) s_joy_buttons |= (1u << 4);
	if (joy_numbuttons > 5 && (s_di_state.rgbButtons[5] & 0x80)) s_joy_buttons |= (1u << 5);
	if (joy_numbuttons > 6 && (s_di_state.rgbButtons[6] & 0x80)) s_joy_buttons |= (1u << 6);
	s_joy_pov = s_di_state.rgdwPOV[0];
#endif

	/* Debug dump (throttled) to verify mappings on hardware. */
	if (in_didebug.value && cls.state == ca_active)
	{
		if (s_last_dump_frame != r_framecount)
		{
			s_last_dump_frame = r_framecount;
			Con_Printf("[di] ofs(x=%d y=%d rx=%d ry=%d sl0=%d pov=%d) raw(x=%lu y=%lu) btn=%08lx pov=%lu\n",
				s_di_ofs_x, s_di_ofs_y, s_di_ofs_rx, s_di_ofs_ry, s_di_ofs_slider0, s_di_ofs_pov0,
				(unsigned long)s_joy_raw[JOY_AXIS_X], (unsigned long)s_joy_raw[JOY_AXIS_Y],
				(unsigned long)s_joy_buttons, (unsigned long)s_joy_pov);
		}
	}

	return TRUE;
}


/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove( usercmd_t *cmd )
{
	float	speed, aspeed;
	float	fAxisValue, fTemp;
	int		i;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (joy_advancedinit != 1)
	{
		Joy_AdvancedUpdate_f();
		joy_advancedinit = 1;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick.value)
	{
		return;
	}

	// collect the joystick data, if possible
	if (IN_ReadJoystick() != TRUE)
	{
		return;
	}

	if (in_speed.state & 1)
		speed = cl_movespeedkey.value;
	else
		speed = 1;
	aspeed = speed * host_frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float)*pdwRawValue[i];
		// move centerpoint to zero
		fAxisValue -= 32768.0;

		if (joy_wwhack2.value != 0.0)
		{
			if (dwAxisMap[i] == AxisTurn)
			{
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1 
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i])
		{
		case AxisForward:
			if ((joy_advanced.value == 0.0) && (in_mlook.state & 1))
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold.value)
				{
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is 0)
					if (m_pitch.value < 0.0)
					{
						cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					else
					{
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (lookspring.value == 0.0)
						V_StopPitchDrift();
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold.value)
				{
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.value) * speed * cl_forwardspeed.value;
				}
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold.value)
			{
				cmd->sidemove += (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
			}
			break;

		case AxisTurn:
			if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold.value)
				{
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
				}
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold.value)
				{
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
					{
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * aspeed * cl_yawspeed.value;
					}
					else
					{
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * speed * 180.0;
					}

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
					{
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					else
					{
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * speed * 180.0;
					}
					V_StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if (lookspring.value == 0.0)
						V_StopPitchDrift();
				}
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
// in_dc.h -- shared declarations for the Dreamcast Maple input files
// (in_dc.c, in_joy.c, in_kbd.c, in_mouse.c)
#ifndef IN_DC_H
#define IN_DC_H

// Buttons and hat switches the pad can report, addressed by usage
#define MAX_JOY_BUTTONS		22
#define MAX_JOY_POVS		11

// A usage slot that no object claimed
#define JOY_USAGE_NONE		255

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

extern mapleport_t			g_MapleDevices[MAPLE_MAX_PORTS];

extern LPDIRECTINPUT		g_pDI;
extern HRESULT				g_hResult;

// The attached pad, mouse and keyboard; NULL while the socket is empty
extern void*				pKeyboardDevice;
extern maplejoystick_t*		pJoystickDevice;
extern maplemouse_t*		pMouseDevice;

// Win32 virtual-key (message wParam) -> engine key-number translation table,
// shared with MapKey.
extern unsigned char		scantokey[256];

#ifdef __cplusplus
extern "C" {
#endif

qboolean	IN_LogResult( LPCTSTR what );
void		IN_DebugPrintf( LPCTSTR fmt, ... );

int			MapKey( int key );
qboolean	IN_CreateMapleDevice( maplekeyboard_t *pKbd );
void		IN_ReleaseMapleDevice( maplekeyboard_t *pKbd );

qboolean	IN_ActivateJoystick( maplejoystick_t *pJoy );
void		IN_ReleaseJoystick( maplejoystick_t *pJoy );
qboolean	IN_ReadJoystick( maplejoystick_t *pJoy );
void		IN_StartupJoystick( void );

qboolean	IN_StartupMouse( maplemouse_t *pMouse );
qboolean	IN_ReadMouseState( maplemouse_t *pMouse );
void		IN_ShutdownMouse( maplemouse_t *pMouse );

void		IN_UpdateMapleDevices( void );
void		IN_ReadMouse( void );

#ifdef __cplusplus
}
#endif

#endif // IN_DC_H

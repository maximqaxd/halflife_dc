// in_joy.c -- Dreamcast Maple controller support
#include "quakedef.h"
#include "vmu.h"
#include "winquake.h"
#include <dinput.h>
#ifdef _WIN32_WCE
#include <maplusag.h>
#endif
#include "in_dc.h"

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

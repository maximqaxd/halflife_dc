// in_mouse.c -- Dreamcast Maple mouse support
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

	prop.dwData = 0;
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

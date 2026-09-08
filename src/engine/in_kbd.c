// in_kbd.c -- Dreamcast Maple keyboard support
#include "quakedef.h"
#include "vmu.h"
#include "winquake.h"
#include <dinput.h>
#ifdef _WIN32_WCE
#include <maplusag.h>
#endif
#include "in_dc.h"

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

qboolean IN_ReadKeyboardState( maplekeyboard_t* pKbd )
{
	byte state[256];
	HRESULT result;

	memset(state, 0, sizeof(state));
	result = pKbd->device.pDevice->GetDeviceState(sizeof(state), state);
	if (DIERR_INPUTLOST == result)
	{
		pKbd->device.pDevice->Acquire();
		pKbd->device.pDevice->GetDeviceState(sizeof(state), state);
	}
	return TRUE;
}

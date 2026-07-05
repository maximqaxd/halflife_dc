// vid_dc.c -- Dreamcast video: window, DirectDraw and Direct3D setup

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dc_accum.h"

#include <windows.h>
#include <tchar.h>

extern HINSTANCE g_hInstance;
extern HINSTANCE g_hPrevInstance;

extern void     VID_UpdateWindowVars( RECT *pRect, int cx, int cy );
extern qboolean DC_InitTextureList( void );
extern cvar_t   r_testlight;
extern cvar_t   mipbias;

static HWND                 g_hwndAppDC;
static LPDIRECTDRAW         g_pDD           = NULL;
static LPDIRECTDRAW4        g_pDD4          = NULL;
static LPDIRECTDRAWSURFACE4 g_pddsPrimary   = NULL;
static LPDIRECTDRAWSURFACE4 g_pddsBack      = NULL;
static LPDIRECT3D3          g_pD3D          = NULL;
LPDIRECT3DDEVICE3           g_pD3DDevice    = NULL;
static LPDIRECT3DVIEWPORT3  g_pViewport     = NULL;
static LPDIRECT3DMATERIAL3  g_pBackgroundMaterial = NULL;
static LPDIRECT3DLIGHT      g_pLights[4];
static DDPIXELFORMAT        g_dc_backbuffer_fmt;

static D3DDEVICEDESC        g_d3dHWDeviceDesc;
static D3DDEVICEDESC        g_d3dHELDeviceDesc;
static D3DDEVICEDESC        g_d3dDeviceDesc;
static D3DVIEWPORT2         g_viewportDesc;
static D3DMATERIAL          g_backgroundMaterialData;
static D3DLIGHT2            g_lightData[4];
static D3DMATRIX            g_identityMatrix;
static D3DMATRIX            g_matNegY;
static D3DMATRIX            g_matNegX;
static D3DMATRIX            g_matAxis3;
static D3DMATRIX            g_matAxis4;
static DDPIXELFORMAT        g_pfRGB565;
static DDPIXELFORMAT        g_pfPalette8;
static DDPIXELFORMAT        g_pfARGB1555;
static DDPIXELFORMAT        g_pfARGB4444;
static DDPIXELFORMAT        g_pfScreenRGB565;

void* Sys_GetDirectDraw4( void )     { return (void*)g_pDD4; }
void* Sys_GetBackBuffer4( void )     { return (void*)g_pddsBack; }
void* Sys_GetPrimarySurface4( void ) { return (void*)g_pddsPrimary; }
void* Sys_GetD3D3( void )            { return (void*)g_pD3D; }
void* Sys_GetD3DDevice3( void )      { return (void*)g_pD3DDevice; }
void* Sys_GetD3DViewport( void )     { return (void*)g_pViewport; }

// Fog render states.
void DCV_SetupFog( void )
{
}

/*
================
DCV_Flip

Present the back buffer. The full version flushes the deferred triangle batch and
draws the profiling meters before the flip; this one just page-flips.
================
*/
void DCV_Flip( void )
{
	if (g_pddsPrimary)
		g_pddsPrimary->lpVtbl->Flip(g_pddsPrimary, NULL, DDFLIP_WAIT);
}

/*
================
DCV_InitDirectDraw

Create DirectDraw, take exclusive full-screen 640x480x16 and build the primary
surface with a single attached back buffer for page flipping.
================
*/
int DCV_InitDirectDraw( void )
{
	RECT           windowRect;
	DDSURFACEDESC2 ddsd;

	DirectDrawCreate(NULL, &g_pDD, NULL);
	g_pDD->lpVtbl->QueryInterface(g_pDD, &IID_IDirectDraw4, (LPVOID*)&g_pDD4);
	g_pDD4->lpVtbl->SetCooperativeLevel(g_pDD4, g_hwndAppDC, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	g_pDD4->lpVtbl->SetDisplayMode(g_pDD4, 640, 480, 16, 0, 0);

	windowRect.left   = 0;
	windowRect.top    = 0;
	windowRect.right  = 640;
	windowRect.bottom = 480;
	VID_UpdateWindowVars(&windowRect, 320, 240);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize  = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
	ddsd.dwBackBufferCount = 1;
	g_pDD4->lpVtbl->CreateSurface(g_pDD4, &ddsd, &g_pddsPrimary, NULL);

	ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
	g_pddsPrimary->lpVtbl->GetAttachedSurface(g_pddsPrimary, &ddsd.ddsCaps, &g_pddsBack);
	return 1;
}

/*
================
DCV_InitDirect3D

Create the Direct3D device on the back buffer, set up the viewport, a default
white material, four lights and identity transforms, then load the default
render and texture-stage states and begin the scene.
================
*/
qboolean DCV_InitDirect3D( void )
{
	LPD3DDEVICEDESC   lpChosenDesc;
	D3DMATERIALHANDLE hMaterial;
	int               i;

	// create the device and read its caps, preferring hardware
	g_pDD4->lpVtbl->QueryInterface(g_pDD4, &IID_IDirect3D3, (LPVOID*)&g_pD3D);
	g_pD3D->lpVtbl->CreateDevice(g_pD3D, &IID_IDirect3DHALDevice, g_pddsBack, &g_pD3DDevice, NULL);

	g_d3dHELDeviceDesc.dwSize = sizeof(D3DDEVICEDESC);
	g_d3dHWDeviceDesc.dwSize  = sizeof(D3DDEVICEDESC);
	g_pD3DDevice->lpVtbl->GetCaps(g_pD3DDevice, &g_d3dHWDeviceDesc, &g_d3dHELDeviceDesc);

	lpChosenDesc = &g_d3dHWDeviceDesc;
	if (g_d3dHWDeviceDesc.dwFlags == 0)
		lpChosenDesc = &g_d3dHELDeviceDesc;
	g_d3dDeviceDesc = *lpChosenDesc;

	// viewport covering the whole back buffer
	g_pD3D->lpVtbl->CreateViewport(g_pD3D, &g_pViewport, NULL);
	g_pD3DDevice->lpVtbl->AddViewport(g_pD3DDevice, g_pViewport);

	g_viewportDesc.dwSize       = sizeof(D3DVIEWPORT2);
	g_viewportDesc.dwX          = 0;
	g_viewportDesc.dwY          = 0;
	g_viewportDesc.dwWidth      = 640;
	g_viewportDesc.dwHeight     = 480;
	g_viewportDesc.dvClipX      = -1.0f;
	g_viewportDesc.dvClipY      = 1.0f;
	g_viewportDesc.dvClipWidth  = 2.0f;
	g_viewportDesc.dvClipHeight = 2.0f;
	g_viewportDesc.dvMinZ       = -0.1f;
	g_viewportDesc.dvMaxZ       = 1.0f;
	g_pViewport->lpVtbl->SetViewport2(g_pViewport, &g_viewportDesc);
	g_pD3DDevice->lpVtbl->SetCurrentViewport(g_pD3DDevice, g_pViewport);

	// default white material
	g_pD3D->lpVtbl->CreateMaterial(g_pD3D, &g_pBackgroundMaterial, NULL);

	memset(&g_backgroundMaterialData, 0, sizeof(D3DMATERIAL));
	g_backgroundMaterialData.dwSize     = sizeof(D3DMATERIAL);
	g_backgroundMaterialData.diffuse.r  = 1.0f;
	g_backgroundMaterialData.diffuse.g  = 1.0f;
	g_backgroundMaterialData.diffuse.b  = 1.0f;
	g_backgroundMaterialData.diffuse.a  = 1.0f;
	g_backgroundMaterialData.ambient.r  = 1.0f;
	g_backgroundMaterialData.ambient.g  = 1.0f;
	g_backgroundMaterialData.ambient.b  = 1.0f;
	g_backgroundMaterialData.ambient.a  = 1.0f;
	g_backgroundMaterialData.specular.r = 1.0f;
	g_backgroundMaterialData.specular.g = 1.0f;
	g_backgroundMaterialData.specular.b = 1.0f;
	g_backgroundMaterialData.power      = 10.0f;
	g_backgroundMaterialData.hTexture   = 0;
	g_pBackgroundMaterial->lpVtbl->SetMaterial(g_pBackgroundMaterial, &g_backgroundMaterialData);
	g_pBackgroundMaterial->lpVtbl->GetHandle(g_pBackgroundMaterial, g_pD3DDevice, &hMaterial);
	g_pD3DDevice->lpVtbl->SetLightState(g_pD3DDevice, D3DLIGHTSTATE_MATERIAL, hMaterial);

	// one directional light and three point lights
	for (i = 0; i < 4; i++)
	{
		g_pD3D->lpVtbl->CreateLight(g_pD3D, &g_pLights[i], NULL);
		memset(&g_lightData[i], 0, sizeof(D3DLIGHT2));
		if (i == 0)
		{
			g_lightData[i].dwSize       = sizeof(D3DLIGHT2);
			g_lightData[i].dltType      = D3DLIGHT_DIRECTIONAL;
			g_lightData[i].dcvColor.r   = 1.0f;
			g_lightData[i].dcvColor.g   = 1.0f;
			g_lightData[i].dcvColor.b   = 1.0f;
			g_lightData[i].dcvColor.a   = 1.0f;
			g_lightData[i].dvDirection.x = -1.0f;
			g_lightData[i].dvDirection.y = -1.0f;
			g_lightData[i].dvDirection.z = -1.0f;
			g_lightData[i].dwFlags      = 3;
		}
		else
		{
			g_lightData[i].dwSize        = sizeof(D3DLIGHT2);
			g_lightData[i].dltType       = D3DLIGHT_POINT;
			g_lightData[i].dcvColor.r    = 1.0f;
			g_lightData[i].dcvColor.g    = 1.0f;
			g_lightData[i].dcvColor.b    = 1.0f;
			g_lightData[i].dcvColor.a    = 1.0f;
			g_lightData[i].dvPosition.x  = 0.0f;
			g_lightData[i].dvPosition.y  = 0.0f;
			g_lightData[i].dvPosition.z  = 0.0f;
			g_lightData[i].dvRange       = 1.0f;
			g_lightData[i].dvAttenuation0 = 1.0f;
			g_lightData[i].dvAttenuation1 = 0.0f;
			g_lightData[i].dvAttenuation2 = 0.0f;
			g_lightData[i].dwFlags       = 2;
		}
		g_pLights[i]->lpVtbl->SetLight(g_pLights[i], (LPD3DLIGHT)&g_lightData[i]);
		g_pViewport->lpVtbl->AddLight(g_pViewport, g_pLights[i]);
	}

	// identity world, view and projection
	g_identityMatrix._11 = 1.0f;
	g_identityMatrix._22 = 1.0f;
	g_identityMatrix._33 = 1.0f;
	g_identityMatrix._44 = 1.0f;
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, D3DTRANSFORMSTATE_WORLD,      &g_identityMatrix);
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, D3DTRANSFORMSTATE_VIEW,       &g_identityMatrix);
	g_pD3DDevice->lpVtbl->SetTransform(g_pD3DDevice, D3DTRANSFORMSTATE_PROJECTION, &g_identityMatrix);

	// axis matrices
	g_matNegY._11 = 1.0f;  g_matNegY._22 = -1.0f; g_matNegY._33 = 1.0f;  g_matNegY._44 = 1.0f;
	g_matNegX._11 = -1.0f; g_matNegX._22 = 1.0f;  g_matNegX._33 = 1.0f;  g_matNegX._44 = 1.0f;
	g_matAxis3._11 = 1.0f; g_matAxis3._23 = -1.0f; g_matAxis3._32 = 1.0f; g_matAxis3._44 = 1.0f;
	g_matAxis4._12 = 1.0f; g_matAxis4._21 = -1.0f; g_matAxis4._33 = 1.0f; g_matAxis4._44 = 1.0f;

	// pixel format table; RGB565 is the template the rest are built from
	memset(&g_pfRGB565, 0, sizeof(DDPIXELFORMAT));
	g_pfRGB565.dwSize            = sizeof(DDPIXELFORMAT);
	g_pfRGB565.dwFlags           = DDPF_RGB;
	g_pfRGB565.dwRGBBitCount     = 16;
	g_pfRGB565.dwRBitMask        = 0xf800;
	g_pfRGB565.dwGBitMask        = 0x07e0;
	g_pfRGB565.dwBBitMask        = 0x001f;
	g_pfRGB565.dwRGBAlphaBitMask = 0;

	g_pfPalette8 = g_pfRGB565;
	g_pfPalette8.dwFlags       = 0x60;                   /* RGB | PALETTEINDEXED8 */
	g_pfPalette8.dwRGBBitCount = 8;

	g_pfScreenRGB565 = g_pfRGB565;
	g_pfARGB1555     = g_pfRGB565;
	g_pfARGB4444     = g_pfRGB565;

	g_pfScreenRGB565.dwFlags           = 0xc0;
	g_pfScreenRGB565.dwRGBBitCount     = 16;
	g_pfScreenRGB565.dwRBitMask        = 0xf800;
	g_pfScreenRGB565.dwGBitMask        = 0x07e0;
	g_pfScreenRGB565.dwBBitMask        = 0x001f;
	g_pfScreenRGB565.dwRGBAlphaBitMask = 0;

	g_pfARGB1555.dwFlags           = 0x41;               /* RGB | ALPHAPIXELS */
	g_pfARGB1555.dwRGBBitCount     = 16;
	g_pfARGB1555.dwRBitMask        = 0x7c00;
	g_pfARGB1555.dwGBitMask        = 0x03e0;
	g_pfARGB1555.dwBBitMask        = 0x001f;
	g_pfARGB1555.dwRGBAlphaBitMask = 0x8000;

	g_pfARGB4444.dwFlags           = 0x41;
	g_pfARGB4444.dwRGBBitCount     = 16;
	g_pfARGB4444.dwRBitMask        = 0x0f00;
	g_pfARGB4444.dwGBitMask        = 0x00f0;
	g_pfARGB4444.dwBBitMask        = 0x000f;
	g_pfARGB4444.dwRGBAlphaBitMask = 0xf000;

	// default render and texture-stage states
	DCV_SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_SPECULARENABLE,     FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_DITHERENABLE,       TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ZENABLE,            TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,       TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ZFUNC,             D3DCMP_LESSEQUAL);

	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

	DCV_SetupFog();

	DCV_SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	DCV_SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);

	if (r_testlight.value == 0)
	{
		DCV_SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
		DCV_SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		DCV_SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
		DCV_SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
		DCV_SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		DCV_SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
		DCV_SetRenderState(D3DRENDERSTATE_MIPMAPLODBIAS, (DWORD)mipbias.value);
	}
	else
	{
		DCV_SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
		DCV_SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT);
		DCV_SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
		DCV_SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_POINT);
		DCV_SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_POINT);
		DCV_SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
	}

	g_pD3DDevice->lpVtbl->BeginScene(g_pD3DDevice);
	return TRUE;
}

/*
================
DCV_CreateWindow

Register the "Halflife" window class, create the 640x480 window, push PVR tuning
into HKLM\DisplaySettings, bring up DirectDraw (640x480x16 flip chain) and the
Direct3D device.
================
*/
qboolean DCV_CreateWindow( void )
{
	WNDCLASS cls;
	HKEY     hKey;
	DWORD    dwValue;

	// PowerVR tuning values
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("DisplaySettings"), 0, 0, &hKey);
	dwValue = 0x1000;
	RegSetValueEx(hKey, TEXT("YUV420StagingBufferSize"), 0, REG_DWORD, (LPBYTE)&dwValue, 4);
	dwValue = 0x2ce20;
	RegSetValueEx(hKey, TEXT("CommandPolygonBufferSize"), 0, REG_DWORD, (LPBYTE)&dwValue, 4);
	dwValue = 0xdbba0;
	RegSetValueEx(hKey, TEXT("CommandVertexBufferSize"), 0, REG_DWORD, (LPBYTE)&dwValue, 4);
	dwValue = 0;
	RegSetValueEx(hKey, TEXT("SmallestPolygon"), 0, REG_DWORD, (LPBYTE)&dwValue, 4);
	RegCloseKey(hKey);

	/* Register the window class once (first instance only). */
	if (!g_hPrevInstance)
	{
		memset(&cls, 0, sizeof(cls));
		cls.style         = 0;
		cls.lpfnWndProc   = (WNDPROC)DefWindowProc;
		cls.cbClsExtra    = 0;
		cls.cbWndExtra    = 0;
		cls.hInstance     = g_hInstance;
		cls.hIcon         = NULL;
		cls.hCursor       = NULL;
		cls.hbrBackground = NULL;
		cls.lpszMenuName  = NULL;
		cls.lpszClassName = TEXT("Halflife");
		if (!RegisterClass(&cls))
			return FALSE;
	}

	g_hwndAppDC = CreateWindowEx(0,
	                             TEXT("Halflife"),
	                             TEXT("Halflife"),
	                             WS_VISIBLE,
	                             0, 0,
	                             640, 480,
	                             NULL, NULL,
	                             g_hInstance, NULL);

	/* Bring up DirectDraw, then Direct3D, then the texture subsystem. */
	if (!DCV_InitDirectDraw())
		return FALSE;
	if (!DCV_InitDirect3D())
		return FALSE;
	return DC_InitTextureList();
}

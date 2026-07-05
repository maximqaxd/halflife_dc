// dc_d3d.c -- Dreamcast video: fog and light state, gamma, DirectDraw/Direct3D setup,
// loading progress and the end-of-frame flip

#include "quakedef.h"
#include "winquake.h"
#include "sys.h"
#include "dc_accum.h"

#include <windows.h>
#include <tchar.h>
#include <shintr.h>

extern HINSTANCE g_hInstance;
extern HINSTANCE g_hPrevInstance;

extern void     VID_UpdateWindowVars( RECT *pRect, int cx, int cy );
extern qboolean DC_InitTextureList( void );
extern cvar_t   r_testlight;
extern cvar_t   mipbias;

extern float    Sys_FloatTime( void );
extern int      GetVideoOutputFormat( void );

// dc_draw.c: point the HUD transform at one of the 2D depth sublayers
extern void     DCV_SetHudDepth( float layer );

// dc_debug.c: framebuffer overlay text, image registry and profiling meters
extern void     DCV_DrawMeters( void );
extern int      DCV_FB_LoadImage( byte *rgb, int cache );

typedef struct fbmeter_s
{
	short type;
	short value;
	int   frame;
	DWORD color;
	char  pad[64];
} fbmeter_t;

extern fbmeter_t g_FBMeters[32];

#define MAX_METERS      32
#define METER_FLIP      400

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

// fog parameters set by the game through DCV_SetFog, applied by DCV_SetupFog
static int      g_bFogChanged;
static int      g_bFogEnabled;
static int      g_nFogR;
static int      g_nFogG;
static int      g_nFogB;
static int      g_nFogAmount;

// hardware gamma ramp, rebuilt as a Hermite curve from the dc_light_* cvars
unsigned short  g_GammaTable[1024];
byte            g_GammaTable256[256];

// overscan margins for the TV; VGA output needs none
int             g_nOverscanX;
int             g_nOverscanY;

// test images registered for the gamma calibration screen
static short    g_hGammaRamp256;
static short    g_hGammaPalette;
static short    g_hGammaPalette2;

// screen saver: fade the frame out after five minutes without input
int             g_bScreenSaverActive;
static float    g_flScreenSaverTime;

// loading progress bar
static int      g_nProgress;

DWORD           g_dwFlipTick;

void* Sys_GetDirectDraw4( void )     { return (void*)g_pDD4; }
void* Sys_GetBackBuffer4( void )     { return (void*)g_pddsBack; }
void* Sys_GetPrimarySurface4( void ) { return (void*)g_pddsPrimary; }
void* Sys_GetD3D3( void )            { return (void*)g_pD3D; }
void* Sys_GetD3DDevice3( void )      { return (void*)g_pD3DDevice; }
void* Sys_GetD3DViewport( void )     { return (void*)g_pViewport; }

/*
================
DCV_SetupFog

Apply the fog parameters last passed to DCV_SetFog to the device. Fog color
components are kept out of the black crush and the range shrinks as the fog
amount rises.
================
*/
void DCV_SetupFog( void )
{
	int   r, g, b, amount;
	float end;

	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, g_bFogEnabled);

	if (g_bFogChanged)
	{
		if (g_bFogEnabled)
		{
			amount = max(0, min(g_nFogAmount, 40));
			r = max(20, min(g_nFogR, 255));
			g = max(20, min(g_nFogG, 255));
			b = max(20, min(g_nFogB, 255));

			DCV_SetRenderState(D3DRENDERSTATE_FOGCOLOR, (r << 16) | (g << 8) | b);
			DCV_SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);

			end = fogrange.value - fogscale.value * (float)amount;

			DCV_SetRenderState(D3DRENDERSTATE_FOGTABLESTART, 0);
			DCV_SetRenderState(D3DRENDERSTATE_FOGTABLEEND, *(DWORD *)&end);

			g_pD3DDevice->lpVtbl->SetLightState(g_pD3DDevice, D3DLIGHTSTATE_FOGMODE, D3DFOG_LINEAR);
			g_pD3DDevice->lpVtbl->SetLightState(g_pD3DDevice, D3DLIGHTSTATE_FOGSTART, 0);
			g_pD3DDevice->lpVtbl->SetLightState(g_pD3DDevice, D3DLIGHTSTATE_FOGEND, *(DWORD *)&end);
			g_pD3DDevice->lpVtbl->SetLightState(g_pD3DDevice, D3DLIGHTSTATE_FOGDENSITY, 1);
		}
		g_bFogChanged = FALSE;
	}
}

void DCV_SetFog( int enable, int r, int g, int b, int amount )
{
	g_bFogEnabled = enable;
	g_nFogR = r;
	g_nFogG = g;
	g_nFogB = b;
	g_nFogAmount = amount;
	g_bFogChanged = TRUE;
}

/*
================
DCV_SetWorldLight

Drive the directional world light and the global material from the current
light direction and color. The material alpha follows the current batch color
so faded polygons dim with the screen.
================
*/
void DCV_SetWorldLight( float *dir, float *color, float *ambient )
{
	g_lightData[0].dcvColor.r = color[0];
	g_lightData[0].dcvColor.g = color[1];
	g_lightData[0].dcvColor.b = color[2];
	g_lightData[0].dvDirection.x = dir[0];
	g_lightData[0].dvDirection.y = dir[1];
	g_lightData[0].dvDirection.z = dir[2];
	g_pLights[0]->lpVtbl->SetLight(g_pLights[0], (LPD3DLIGHT)&g_lightData[0]);

	g_backgroundMaterialData.diffuse.r = 0.6f;
	g_backgroundMaterialData.diffuse.g = 0.6f;
	g_backgroundMaterialData.diffuse.b = 0.6f;
	g_backgroundMaterialData.diffuse.a = (float)(g_dwAccumCurrentDiffuse >> 24) * (1.0f / 255.0f);
	g_backgroundMaterialData.emissive.r = ambient[0] + color[0] * 0.4f;
	g_backgroundMaterialData.emissive.g = ambient[1] + color[1] * 0.4f;
	g_backgroundMaterialData.emissive.b = ambient[2] + color[2] * 0.4f;
	g_pBackgroundMaterial->lpVtbl->SetMaterial(g_pBackgroundMaterial, &g_backgroundMaterialData);
}

void DCV_SetDlight( int index, float *origin, float *color, float radius )
{
	g_lightData[index].dcvColor.r = color[0];
	g_lightData[index].dcvColor.g = color[1];
	g_lightData[index].dcvColor.b = color[2];
	g_lightData[index].dvPosition.x = origin[0];
	g_lightData[index].dvPosition.y = origin[1];
	g_lightData[index].dvPosition.z = origin[2];
	g_lightData[index].dvRange = radius;
	g_lightData[index].dwFlags = D3DLIGHT_ACTIVE | D3DLIGHT_NO_SPECULAR;
	g_pLights[index]->lpVtbl->SetLight(g_pLights[index], (LPD3DLIGHT)&g_lightData[index]);
}

void DCV_DisableDlight( int index )
{
	g_lightData[index].dwFlags = D3DLIGHT_NO_SPECULAR;
	g_pLights[index]->lpVtbl->SetLight(g_pLights[index], (LPD3DLIGHT)&g_lightData[index]);
}

/*
================
DCV_BuildGammaTable

Rebuild the gamma ramp as a Hermite curve through dc_light_min/dc_light_max
with tangents dc_light_alpha/dc_light_beta.
================
*/
void DCV_BuildGammaTable( float lo, float hi )
{
	float alpha, beta;
	float v;
	int   i;

	lo = dc_light_min.value;
	hi = dc_light_max.value;
	alpha = dc_light_alpha.value;
	beta = dc_light_beta.value;

	for (i = 0; i < 1024; i++)
	{
		v = (float)i / 1023.0f;
		v = v * v * v * ((lo + lo) - (hi + hi) + alpha + beta)
		  + v * v * ((lo * -3.0f + hi * 3.0f) - (alpha + alpha) - beta)
		  + v * alpha + lo;
		if (v < 0.0f)
			v = 0.0f;
		if (v > 1.0f)
			v = 1.0f;
		g_GammaTable[i] = (unsigned short)(int)(v * 1023.0f);
		g_GammaTable256[i / 4] = (byte)(int)(v * 255.0f);
	}
}

__inline void DCV_RefreshGamma( void )
{
	DCV_BuildGammaTable(0.58f, 0.9f);
	g_nOverscanX = 8;
	g_nOverscanY = 24;
}

/*
================
DCV_GammaRefresh_f

Pick the gamma curve for the current video output and register the test
images for the calibration screen: a 256-step gray ramp and the classic
216-color cube padded with 40 grays.
================
*/
void DCV_GammaRefresh_f( void )
{
	byte rgb[768];
	byte *p;
	int  i, j, k;

	switch (GetVideoOutputFormat())
	{
	case 2:
	case 3:
		DCV_RefreshGamma();
		break;
	case 0x12:
	case 0x13:
	case 0x22:
	case 0x23:
	case 0x32:
	case 0x33:
		DCV_RefreshGamma();
		break;
	case 0x40:
		DCV_BuildGammaTable(0.8f, 0.9f);
		g_nOverscanX = 0;
		g_nOverscanY = 0;
		break;
	default:
		DCV_BuildGammaTable(1.0f, 1.0f);
		g_nOverscanX = 8;
		g_nOverscanY = 24;
		break;
	}

	p = rgb;
	for (i = 0; i < 256; i++)
	{
		p[0] = (byte)i;
		p[1] = (byte)i;
		p[2] = (byte)i;
		p += 3;
	}
	g_hGammaRamp256 = (short)DCV_FB_LoadImage(rgb, 0);

	for (i = 0; i < 6; i++)
	{
		for (j = 0; j < 6; j++)
		{
			for (k = 0; k < 6; k++)
			{
				rgb[(i * 36 + j * 6 + k) * 3]     = (byte)(i * 255 / 5);
				rgb[(i * 36 + j * 6 + k) * 3 + 1] = (byte)(j * 255 / 5);
				rgb[(i * 36 + j * 6 + k) * 3 + 2] = (byte)(k * 255 / 5);
			}
		}
	}
	p = rgb + 6 * 6 * 6 * 3;
	for (i = 0; i < 40; i++)
	{
		p[0] = (byte)(i * 255 / 39);
		p[1] = (byte)(i * 255 / 39);
		p[2] = (byte)(i * 255 / 39);
		p += 3;
	}
	g_hGammaPalette = (short)DCV_FB_LoadImage(rgb, 0);
	g_hGammaPalette2 = (short)DCV_FB_LoadImage(rgb, 0);
}

void Sys_ShutdownDisplay( void )
{
	if (g_pD3DDevice)
		g_pD3DDevice->lpVtbl->Release(g_pD3DDevice);
	if (g_pddsBack)
		g_pddsBack->lpVtbl->Release(g_pddsBack);
	if (g_pddsPrimary)
		g_pddsPrimary->lpVtbl->Release(g_pddsPrimary);
	if (g_pD3D)
		g_pD3D->lpVtbl->Release(g_pD3D);
	if (g_pDD4)
		g_pDD4->lpVtbl->Release(g_pDD4);
}

qboolean DCV_DDError( HRESULT hr, TCHAR *op )
{
	TCHAR text[256];

	if (hr != DD_OK)
		wsprintf(text, TEXT("****%s failed (Error # = 0x%08x).\r\n"), op, hr);
	return hr != DD_OK;
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

	windowRect.top    = 0;
	windowRect.left   = 0;
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
DCV_FB_BackgroundRect

Paint a 128x32 rectangle straight into the primary surface behind the
framebuffer overlay text.
================
*/
void DCV_FB_BackgroundRect( WORD color )
{
	DDSURFACEDESC2 ddsd;
	WORD *p;
	int   i, x;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	g_pddsPrimary->lpVtbl->Lock(g_pddsPrimary, NULL, &ddsd, DDLOCK_WAIT, NULL);

	for (i = 0; i < 32; i++)
	{
		p = (WORD *)ddsd.lpSurface + (ddsd.lPitch * (g_nOverscanY + i) + 640 - 128) / 2;
		for (x = 0; x < 128; x++)
			*p++ = color;
	}

	g_pddsPrimary->lpVtbl->Unlock(g_pddsPrimary, NULL);
}

void DCV_WaitVBlank( void )
{
	g_pDD4->lpVtbl->WaitForVerticalBlank(g_pDD4, DDWAITVB_BLOCKBEGIN, NULL);
}

/*
================
Host_UpdateScreenSaver

Reset the idle timer on user input, or check it and arm the screen saver
after five minutes without any.
================
*/
void Host_UpdateScreenSaver( int bCheckOnly )
{
	float time;

	time = Sys_FloatTime();
	if (!bCheckOnly)
	{
		g_flScreenSaverTime = Sys_FloatTime();
		g_bScreenSaverActive = FALSE;
	}
	else if (time - g_flScreenSaverTime > 300.0f)
	{
		g_bScreenSaverActive = TRUE;
	}
}

/*
================
DCV_ScreenFade

Draw a full-screen colored quad over the frame at one of the HUD depth
sublayers; texturing off, ordinary alpha blend, no fog.
================
*/
__forceinline void DCV_ScreenFade( int r, int g, int b, int a, int layer )
{
	DCV_SetHudDepth((float)layer);

	DCV_SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	DCV_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	DCV_SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE,  FALSE);
	DCV_SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	DCV_SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

	DCV_SetColor(r, g, b, a);
	DCV_FlushIfLarge();
	DCV_AddPolyIndices(g_nAccumVertCount, 4);
	DCV_AddVertex(0.0f,   0.0f,   dc_depthhud.value, 0.0f, 0.0f);
	DCV_AddVertex(640.0f, 0.0f,   dc_depthhud.value, 1.0f, 0.0f);
	DCV_AddVertex(0.0f,   480.0f, dc_depthhud.value, 0.0f, 1.0f);
	DCV_AddVertex(640.0f, 480.0f, dc_depthhud.value, 1.0f, 1.0f);
}

__forceinline void DCV_DrawScreenSaver( void )
{
	if (Sys_FloatTime() - g_flScreenSaverTime > 300.0f)
		g_bScreenSaverActive = TRUE;
	if (g_bScreenSaverActive)
		DCV_ScreenFade(0, 0, 0, 192, 5);
}

/*
================
DCV_ProgressColor

Orange gradient across the progress bar, brightest in the middle rows.
================
*/
unsigned short DCV_ProgressColor( int row, int height )
{
	float f;

	f = 1.5f - fabsf((float)(row - height / 2)) / (float)(height / 2);
	if (f > 1.0f)
		f = 1.0f;
	if (f < 0.0f)
		f = 0.0f;
	return (unsigned short)((((int)(f * 255.0f) >> 3) << 11) |
	                        (((int)(f * 144.0f) >> 2) << 5) |
	                        ((int)(f * 0.0f) >> 3));
}

/*
================
DCV_DrawProgress

Draw the loading bar straight into the primary surface so it shows without
a flip; percent of a 186 pixel bar.
================
*/
void DCV_DrawProgress( int percent, int y, int height )
{
	DDSURFACEDESC2 ddsd;
	WORD *p;
	WORD  color;
	int   i, x, width;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	g_pddsPrimary->lpVtbl->Lock(g_pddsPrimary, NULL, &ddsd, DDLOCK_WAIT, NULL);

	if (height > 0)
	{
		width = 186 * percent / 100;
		for (i = 0; i < height; i++)
		{
			color = DCV_ProgressColor(i, height);
			p = (WORD *)ddsd.lpSurface + (ddsd.lPitch * (y + i) + 640 - 186) / 2;
			for (x = 0; x < width; x++)
				*p++ = color;
		}
	}

	g_pddsPrimary->lpVtbl->Unlock(g_pddsPrimary, NULL);
}

void DCV_SetProgress( int percent )
{
	float flProgress;

	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	flProgress = progress.value;
	g_nProgress = percent;

	if (flProgress == 0.0f)
	{
		if (g_nProgress)
			DCV_DrawProgress(g_nProgress, 257, 30);
	}
	else
	{
		DCV_DrawProgress((int)progress.value, 257, 30);
	}
}

/*
================
DCV_Flip

End of frame: screen saver fade, flip-time meter, overlay meters, final batch
flush and the page flip, then the progress bar on top.
================
*/
void DCV_Flip( void )
{
	fbmeter_t *pMeter;
	float      dt;
	int        i, val;

	DCV_DrawScreenSaver();

	if (profilescale.value > 0)
	{
		if (profilescale.value >= 1.0f)
		{
			dt = (float)(GetTickCount() - g_dwFlipTick) * 0.001f;
			if (dt < 0.0f || dt > 0.2f)
				val = 0;
			else
				val = (int)(profilescale.value * dt + 20.0f);

			if (val > 0)
			{
				pMeter = NULL;
				for (i = 0; i < MAX_METERS; i++)
				{
					if (g_FBMeters[i].type == 0)
					{
						pMeter = &g_FBMeters[i];
						break;
					}
				}
				if (pMeter)
				{
					pMeter->type = METER_FLIP;
					pMeter->color = 0xffffffff;
					pMeter->value = (short)val;
				}
			}
		}
	}

	DCV_DrawMeters();
	g_dwFlipTick = GetTickCount();

	DCV_Flush();
	g_pddsPrimary->lpVtbl->Flip(g_pddsPrimary, NULL, DDFLIP_WAIT);

	if (profilescale.value >= 1.0f)
	{
		dt = (float)(GetTickCount() - g_dwFlipTick) * 0.001f;
		if (dt < 0.0f || dt > 0.2f)
			val = 0;
		else
			val = (int)(profilescale.value * dt + 20.0f);

		if (val > 0)
		{
			pMeter = NULL;
			for (i = 0; i < MAX_METERS; i++)
			{
				if (g_FBMeters[i].type == 0)
				{
					pMeter = &g_FBMeters[i];
					break;
				}
			}
			if (pMeter)
			{
				pMeter->type = METER_FLIP;
				pMeter->color = 0xffffffff;
				pMeter->value = (short)val;
			}
		}
	}

	if (progress.value == 0.0f)
	{
		if (g_nProgress)
			DCV_DrawProgress(g_nProgress, 257, 30);
	}
	else
	{
		DCV_DrawProgress((int)progress.value, 257, 30);
	}

	g_dwAccumCurrentDiffuse = 0xffffffff;
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
			g_lightData[i].dwFlags      = D3DLIGHT_ACTIVE | D3DLIGHT_NO_SPECULAR;
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
			g_lightData[i].dwFlags       = D3DLIGHT_NO_SPECULAR;
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

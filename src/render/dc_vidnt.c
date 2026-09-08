// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"
#include "dc_accum.h"
#include "qgl.h"

BOOL		gfMiniDriver = FALSE;

const char*	gl_vendor;
const char*	gl_renderer;
const char*	gl_version;
const char*	gl_extensions;

static HANDLE	hMovieFile = INVALID_HANDLE_VALUE;

cvar_t		gl_ztrick = { "gl_ztrick", "1" };
cvar_t		gl_d3dflip = { "gl_d3dflip", "0" };
cvar_t		gl_allowsoftware = { "gl_allowsoftware", "0" };

viddef_t	vid;				// global video state

float		gldepthmin, gldepthmax;

PROC qglArrayElementEXT;
PROC qglColorPointerEXT;
PROC qglTexCoordPointerEXT;
PROC qglVertexPointerEXT;

qboolean	gl_mtexable = FALSE;

//====================================

cvar_t		vid_d3d = { "vid_d3d", "0" };
cvar_t		vid_mode = { "vid_mode", "0" };
// Note that 0 is MODE_WINDOWED
cvar_t		_vid_default_mode = { "_vid_default_mode", "0", FCVAR_ARCHIVE };
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t		_vid_default_mode_win = { "_vid_default_mode_win", "3", FCVAR_ARCHIVE };
cvar_t		vid_wait = { "vid_wait", "0" };
cvar_t		vid_nopageflip = { "vid_nopageflip", "0", FCVAR_ARCHIVE };
cvar_t		vid_wait_override = { "_vid_wait_override", "0", FCVAR_ARCHIVE };
cvar_t		vid_config_x = { "vid_config_x", "800", FCVAR_ARCHIVE };
cvar_t		vid_config_y = { "vid_config_y", "600", FCVAR_ARCHIVE };
cvar_t		vid_stretch_by_2 = { "vid_stretch_by_2", "1", FCVAR_ARCHIVE };
cvar_t		_windowed_mouse = { "_windowed_mouse", "0", FCVAR_ARCHIVE };

int			window_center_x, window_center_y;
RECT		window_rect;

extern int GlideReadPixels( int x, int y, int width, int height, word* pixels );



int			texture_mode = GL_LINEAR;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
//int		texture_mode = GL_LINEAR; TODO!
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int			texture_extension_number = 1;

/*
===============
CheckTextureExtensions
===============
*/
void CheckTextureExtensions( void )
{
}

/*
===============
CheckArrayExtensions
===============
*/
void CheckArrayExtensions( void )
{
}

/*
===============
CheckMultiTextureExtensions
===============
*/
void CheckMultiTextureExtensions( void )
{
}

/*
===============
GL_Config
===============
*/
void GL_Config( void )
{
}

/*
===============
GL_Init
===============
*/
void GL_Init( void )
{
#if 0
	gl_vendor = (const char*)qglGetString(GL_VENDOR);
	Con_DPrintf("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = (const char*)qglGetString(GL_RENDERER);
	Con_DPrintf("GL_RENDERER: %s\n", gl_renderer);

	gl_version = (const char*)qglGetString(GL_VERSION);
	Con_DPrintf("GL_VERSION: %s\n", gl_version);
	gl_extensions = (const char*)qglGetString(GL_EXTENSIONS);
	Con_DPrintf("GL_EXTENSIONS: %s\n", gl_extensions);

	qglClearColor(1, 0, 0, 0);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);

	qglAlphaFunc(GL_NOTEQUAL, 0.0);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel(GL_FLAT);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering( int* x, int* y, int* width, int* height )
{
	*x = *y = 0;
	*width = window_rect.right - window_rect.left;
	*height = window_rect.bottom - window_rect.top;
	vid.width = vid.conwidth = *width;
	vid.height = vid.conheight = *height;

	DCV_SetViewport(*x, *y, *width, *height);
}


void GL_EndRendering( void )
{
	DCV_Flush();
	DCV_Flip();
}

void VID_Update( struct vrect_s* rects )
{
}

/*
=================
VID_DescribeMode_f
=================
*/
static char	vid_describe_msg[256];

char* VID_GetExtModeDescription( int mode );

void VID_DescribeMode_f( void )
{
	int			modenum;

	modenum = Q_atoi(Cmd_Argv(1));

	Con_Printf("%s\n", VID_GetExtModeDescription(modenum));
}

//==========================================================================

BOOL bSetupPixelFormat( HDC hDC )
{
	return TRUE;
}

DLL_EXPORT int GL_SetMode( HWND mainwindow, HDC* pmaindc, HGLRC* pbaseRC, int fD3D, char* pszDriver )
{
	return 0;
}

DLL_EXPORT void GL_Shutdown( HWND hwnd, HDC hdc, HGLRC hglrc )
{
}


/*
================
VID_Init
================
*/
int VID_Init( word* palette )
{
	Cvar_RegisterVariable(&vid_mode);
	Cvar_RegisterVariable(&vid_wait);
	Cvar_RegisterVariable(&vid_nopageflip);
	Cvar_RegisterVariable(&vid_wait_override);
	Cvar_RegisterVariable(&_vid_default_mode);
	Cvar_RegisterVariable(&_vid_default_mode_win);
	Cvar_RegisterVariable(&vid_config_x);
	Cvar_RegisterVariable(&vid_config_y);
	Cvar_RegisterVariable(&vid_stretch_by_2);
	Cvar_RegisterVariable(&_windowed_mouse);
	Cvar_RegisterVariable(&gl_ztrick);
	Cvar_RegisterVariable(&vid_d3d);

	if (gfMiniDriver)
		Cvar_Set("vid_d3d", "1");

	Cvar_RegisterVariable(&gl_d3dflip);

	Cmd_AddCommand("vid_describemode", VID_DescribeMode_f);

	return TRUE;
}

/*
===================
VID_TakeSnapshot

Write vid.buffer out as a windows bitmap file
*/
void VID_TakeSnapshot( const char* pFilename )
{

}


void VID_TakeSnapshotRect( const char* pFilename, int x, int y, int w, int h )
{

}

void VID_WriteBuffer( const char* pFilename )
{

}

DLL_EXPORT void VID_UpdateWindowVars( void* prc, int x, int y )
{
	window_rect = *(RECT*)prc;
	window_center_x = x;
	window_center_y = y;
}

DLL_EXPORT void VID_UpdateVID( viddef_t* pvid )
{
	vid = *pvid;
}

DLL_EXPORT int VID_AllocBuffers( void )
{
	return TRUE;
}

DLL_EXPORT void VID_GetVID( viddef_t* pvid )
{
	if (pvid)
		*pvid = vid;
}

DLL_EXPORT void VID_FlipScreen( void )
{
	DCV_Flip();
}

void VID_Shutdown( void )
{
}

/*
================
VID_GetExtModeDescription

Mode driver name shown by the "vid_describemode" console command.
================
*/
char* VID_GetExtModeDescription( int mode )
{
	sprintf(vid_describe_msg, "FIXME: %s, %d", __FILE__, __LINE__);
	return vid_describe_msg;
}
/*
================
GetVideoOutputFormat

Dreamcast AV-cable / video-mode query used to pick the gamma curve. The real
call is a DC system service not present in the WinCE SDK; return 2 (the normal
composite/RGB path) until it is reconstructed.
================
*/
int GetVideoOutputFormat( void )
{
	return 2;
}

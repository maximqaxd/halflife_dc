// dc_rmisc.c

#include "quakedef.h"

cvar_t		r_cachestudio = { "r_cachestudio", "1" };
cvar_t		r_norefresh = { "r_norefresh", "0" };
cvar_t		r_drawentities = { "r_drawentities", "1" };
cvar_t		r_drawviewmodel = { "r_drawviewmodel", "1" };
cvar_t		r_speeds = { "r_speeds", "0" };
cvar_t		r_fullbright = { "r_fullbright", "0" };
cvar_t		r_decals = { "r_decals", "4096" };
cvar_t		mp_decals = { "mp_decals", "300" };
cvar_t		r_lightmap = { "r_lightmap", "0" };
cvar_t		r_shadows = { "r_shadows", "0" };
cvar_t		r_mirroralpha = { "r_mirroralpha", "1" };
cvar_t		r_wateralpha = { "r_wateralpha", "1" };
cvar_t		r_dynamic = { "r_dynamic", "1" };
cvar_t		r_novis = { "r_novis", "0" };
cvar_t		r_mmx = { "r_mmx", "0" };
cvar_t		r_traceglow = { "r_traceglow", "0" };
cvar_t		r_testlight = { "r_testlight", "0" };
cvar_t		r_drawadaptive = { "r_drawadaptive", "0" };
cvar_t		r_glowshellfreq = { "r_glowshellfreq", "2.2" };
cvar_t		d_spriteskip = { "d_spriteskip", "0" };
cvar_t		r_wadtextures = { "r_wadtextures", "0" };

cvar_t		gl_monolights = { "gl_monolights", "0" };

// Texture-sorted world rendering; always on for this build.
int			gl_texsort = 1;

cvar_t		gl_cull = { "gl_cull", "1" };
cvar_t		gl_smoothmodels = { "gl_smoothmodels", "1" };
cvar_t		gl_flashblend = { "gl_flashblend", "0" };
cvar_t		gl_keeptjunctions = { "gl_keeptjunctions", "1" };
cvar_t		gl_wateramp = { "gl_wateramp", "0.3" };
cvar_t		gl_spriteblend = { "gl_spriteblend", "1" };
cvar_t		gl_lightholes = { "gl_lightholes", "1" };
cvar_t		gl_zmax = { "gl_zmax", "4096" };
cvar_t		gl_alphamin = { "gl_alphamin", "0.25" };
cvar_t		gl_overdraw = { "gl_overdraw", "0" };
cvar_t		gl_watersides = { "gl_watersides", "0" };
cvar_t		gl_envmapsize = { "gl_envmapsize", "256" };

cvar_t		mipbias = { "mipbias", "0.0" };
cvar_t		fogrange = { "fogrange", "500.0" };
cvar_t		fogscale = { "fogscale", "0.03" };
cvar_t		progress = { "progress", "0.0" };
cvar_t		profilescale = { "profilescale", "0" };
cvar_t		profilemeter = { "profilemeter", "0" };

cvar_t		dc_light_min = { "dc_light_min", "0.04", 0, 0.04f };
cvar_t		dc_light_max = { "dc_light_max", "1", 0, 1.0f };
cvar_t		dc_light_alpha = { "dc_light_alpha", "2.0", 0, 2.0f };
cvar_t		dc_light_beta = { "dc_light_beta", "0", 0, 0.0f };

cvar_t		dc_depthhud = { "dc_depthhud", "-0.1" };
cvar_t		dc_depthminhud = { "dc_depthminhud", "-0.4" };
cvar_t		dc_depthmaxhud = { "dc_depthmaxhud", "0.4" };
cvar_t		dc_depthmin = { "dc_depthmin", "0.1" };
cvar_t		dc_depthmax = { "dc_depthmax", "1.0" };
cvar_t		dc_msw = { "dc_msw", "1.000" };
cvar_t		dc_msv = { "dc_msv", "0.300" };
cvar_t		dc_msd = { "dc_msd", "0.975" };
cvar_t		dc_msh = { "dc_msh", "0.500" };
cvar_t		dc_msh2 = { "dc_msh2", "0.475" };

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
void R_Envmap_f( void )
{

}

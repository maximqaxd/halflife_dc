//
// CDAudio stubs
//
#include "quakedef.h"
#include "winquake.h"
#include "pr_cmds.h"

#define CVOXFILESENTENCEMAX		1536		// max number of sentences in game. NOTE: this must match

char* rgpszrawsentence[CVOXFILESENTENCEMAX];

#ifdef __cplusplus
extern "C" {
#endif


void CDAudio_Play( int track, int looping )
{
	(void)track;
	(void)looping;
}

void CDAudio_Resume( void )
{

}

void CDAudio_Pause( void )
{
}

void CDAudio_Update( void )
{
}

void S_ClearBuffer( void )
{

}


void S_StartDynamicSound( int entnum, int entchannel, sfx_t* sfx, vec_t* origin, float fvol, float attenuation, int flags, int pitch )
{

}

sfx_t* S_PrecacheSound( char* name )
{
	return NULL;
}

void S_StartStaticSound( int entnum, int entchannel, sfx_t* sfxin, vec_t* origin, float fvol, float attenuation, int flags, int pitch )
{
}

// Stop all sounds for entity on a channel.
void S_StopSound( int entnum, int entchannel )
{

}


void S_StopAllSounds( qboolean clear )
{

}

void S_BeginPrecaching( void )
{
	
}

void S_EndPrecaching( void )
{
	
}

void S_LocalSound( char* sound )
{

}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( vec_t* origin, vec_t* forward, vec_t* right, vec_t* up )
{

}


void S_ExtraUpdate( void )
{

}

void S_Shutdown( void )
{

}

void S_GetDSPInfo( int* spu, int* freeSpu, int* afile, int* ce )
{
	*spu = 0;
	*freeSpu = 0;
	*afile = 0;
	*ce = 0;
}

int SNDDMA_BufferDrained( int size )
{
	return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif
// sound.h -- client sound i/o functions
#ifndef SOUND_H
#define SOUND_H
#ifdef _WIN32
#pragma once
#endif

// sentence groups
#define CBSENTENCENAME_MAX		16
#define CVOXFILESENTENCEMAX		1600		// max number of sentences in game. NOTE: this must match
											// CVOXFILESENTENCEMAX in dlls\util.h!!!

#define CHAR_STREAM			'*'		// as one of 1st 2 chars in name, indicates streaming wav data
#define CHAR_USERVOX		'?'		// as one of 1st 2 chars in name, indicates user realtime voice data
#define CHAR_SENTENCE		'!'		// as one of 1st 2 chars in name, indicates sentence wav
#define CHAR_DRYMIX			'#'		// as one of 1st 2 chars in name, indicates wav bypasses dsp fx

// Sounds the cache can hold at once, and objects the audio manager can track.
#define MAX_SFX				512
#define MAX_AUDIO			64

// sfx_t::flags
#define SFX_LOOPING			0x0008	// the wav carried a smpl chunk
#define SFX_KEEP			0x0010	// server precache, survives a level change

// The buffer a cached sound lives in. Declared opaquely so the engine's C files
// can carry sfx_t around without pulling in DirectSound.
struct IDirectSoundBuffer;

typedef struct sfx_s
{
	struct IDirectSoundBuffer*	buffer;
	unsigned short				flags;
	short						servercount;
	char						name[48];
} sfx_t;

#ifdef __cplusplus
extern "C" {
#endif

// device
void S_Init( void );
void S_Shutdown( void );
void S_ShutdownDevice( void );

// per-frame
void S_Update( vec_t* origin, vec_t* forward, vec_t* right, vec_t* up );
void S_ExtraUpdate( void );

// playing
void S_StartDynamicSound( int entnum, int entchannel, sfx_t* sfx, vec_t* origin, float fvol, float attenuation, int flags, int pitch );
void S_StartStaticSound( int entnum, int entchannel, sfx_t* sfx, vec_t* origin, float fvol, float attenuation, int flags, int pitch );
void S_StopSound( int entnum, int entchannel );
void S_StopAllSounds( qboolean clear );
void S_CloseAllSounds( void );
DLL_EXPORT void S_ClearBuffer( qboolean clear );
void S_BlockSound( void );
void S_UnblockSound( void );

// the sound cache
sfx_t* S_FindName( char* name );
sfx_t* S_FindNameNoCreate( char* name );
void S_TouchSound( char* name );
int S_AllocSfx( char* name );
sfx_t* S_GetSfxByIndex( int index );
int SV_LookupSoundIndex( char* name );
qboolean S_IsPlayableSound( char* name );
void S_BeginPrecaching( void );
void S_EndPrecaching( void );

// reports
void S_SoundInfo( void );
void S_PrintSoundInfo( void );
int S_SoundList( int start, int count );

// query SPU/audio memory usage for the memory profiling meters
void S_GetDSPInfo( int* ram, int* spuram, int* freeram, int* afile );

// sentences -- the parsed sentence list, shared with the message parser
extern char* rgpszrawsentence[CVOXFILESENTENCEMAX];

void SENTENCEG_Init( void );
char* S_FindNameByIndex( char* name, int* index );

// GD-ROM music
void CDAudio_Init( void );
void CDAudio_Shutdown( void );
void CDAudio_Pause( qboolean enable );
void CDAudio_PlayTrack( int track, qboolean looping );

char* S_DSoundError( long hr );

extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

extern cvar_t snd_show;
extern cvar_t bgmvolume;
extern cvar_t volume;
extern cvar_t suitvolume;
extern cvar_t nosound;
extern cvar_t stereo;
extern cvar_t stereo_sep;
extern cvar_t soundextra;
extern cvar_t mouthdelay;
extern cvar_t mouthrate;
extern cvar_t mouthscale;

#ifdef __cplusplus
}
#endif

#endif // SOUND_H

// audio_mgr.h -- the audio manager and the sound cache

#ifndef AUDIO_MGR_H
#define AUDIO_MGR_H

#include "audio.h"

// Largest wav we will read off the disc in one go, and the largest data chunk
// we will hand to a hardware buffer.
#define MAX_SOUNDFILE		98304
#define MAX_SOUNDDATA		65536

extern sfx_t	*known_sfx[MAX_SFX];
extern CAudio	*g_audio[MAX_AUDIO];
extern int	s_servercount;


sfx_t	*S_PrecacheSound (CAudioMgr *mgr, char *name, qboolean fromserver);
sfx_t	*S_CacheSoundRecord (CAudioMgr *mgr, char *path, byte *data, int size);
void	S_LoadSound (CAudioMgr *mgr, sfx_t *sfx);
void	S_LoadDSoundBuffer (CAudioMgr *mgr, sfx_t *sfx, byte *wav);



void	S_BeginLevelLoad (void);
void	S_FreeUnusedSounds (void);

void	S_InitSoundCache (void);
void	S_GetSoundRam (int *ram, int *spuram, int *freeram, int *afile);

int	S_GetSoundtime (CAudioMgr *mgr);
CAudio	*S_PickChannel (int entnum, int entchannel);
CAudio	*S_FindChannelForName (char *name);


#endif // AUDIO_MGR_H

// audio_cd.h -- music tracks played off the disc

#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include "audio_stream.h"

// Tracks the remap table can describe.
#define MAX_CDTRACKS		100

class CAudioCD : public CAudioStream
{
public:
	CAudioCD ();
	virtual ~CAudioCD ();

	virtual void	Stop (void);
	virtual void	Close (void);
	virtual void	Update (float frametime, qboolean quiet);
	virtual void	UpdateVolume (void);
	virtual void	Restart (void);

	int		m_track;
	float		m_lastvolume;
};

extern CAudioCD	*cdaudio;

void	CDAudio_Init (void);
void	CDAudio_Pause (qboolean enable);
void	CDAudio_Play (CAudioCD *cd, int track, qboolean looping);
void	CDAudio_PlayTrack (int track, qboolean looping);

void	Cmd_cd_f (void);

#endif // AUDIO_CD_H

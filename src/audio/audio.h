// audio.h -- the audio object hierarchy
//
// Everything that can make a noise is a CAudio: a single DirectSound buffer
// plus the state needed to place it in the world. CAudioStatic owns a buffer
// that holds the whole sound, CAudioStream refills its buffer as it plays, and
// CAudioCD is a stream fed from the music tracks on the disc.
//
// The audio manager keeps every live object in one list and walks it once a
// frame, updating volume and panning and reaping objects that have finished.

#ifndef AUDIO_H
#define AUDIO_H

#include <dsound.h>

// Single-precision math. The FPU runs fixed in single-precision mode, so the
// float entry points are the only ones worth calling.

// Sample rate everything is mixed at.
#define AFILE_RATE		44100
#define MAX_AUDIO_NAME	48

// The driver hands a static buffer a block of sample memory and records it
// here. Filling and reading blocks goes straight to that memory rather than
// locking the buffer for every transfer.
#define DSBUF_MEMOFS		0x50

typedef struct dsbufmem_s
{
	struct dsbufmem_s	*owner;
	int			reserved;
	void			*data;
} dsbufmem_t;

class CAudio
{
public:
	CAudio ();
	virtual ~CAudio ();

	virtual void	Stop (void);
	virtual void	Close (void);

	// Supplied by the derived class -- what to do when sound is taken away
	// and handed back, and what to do once a frame.
	virtual void	Suspend (void) = 0;
	virtual void	Restore (void) = 0;
	virtual void	Update (float frametime, qboolean quiet) = 0;

	virtual void	UpdateVolume (void);

	void		SetSpatial (float attenuation, float volume, vec3_t origin,
				int entnum, int entchannel, int pitch);
	void		SetVolume (float volume);
	void		SetPitch (int pitch);
	qboolean	Play (void);

	LPDIRECTSOUNDBUFFER	m_pBuffer;

	DSBUFFERDESC		m_desc;			// Description the buffer was made from.

	byte			m_loop;
	byte			m_stream;		// Refilled while it plays.
	byte			m_spatial;		// Placed in the world.

	vec3_t			m_origin;
	float			m_attenuation;
	float			m_volume;
	int			m_entnum;
	int			m_entchannel;
	int			m_pitch;		// 100 plays at the recorded rate.
	int			m_rate;			// Recorded sample rate.

	byte			m_stopping;		// Fading out, reaped when silent.
	byte			m_autofree;		// Manager deletes it once done.
	byte			m_speech;		// Drives the mouth flap.

	int			m_dsvolume;		// Hundredths of a dB, 0 is full.
	int			m_dspan;

	char			m_name[MAX_AUDIO_NAME];
};

// The audio manager. One instance, created by S_Init.
class CAudioMgr
{
public:
	CAudioMgr ();
	~CAudioMgr ();

	void		Add (CAudio *audio);
	void		Remove (CAudio *audio);
	void		Update (qboolean quiet);
	void		StopAll (qboolean close);
	void		Block (qboolean block);

	int		m_lasttime;
	int		m_mastervolume;
	byte		m_precaching;
};

extern CAudioMgr	*g_pAudioMgr;

extern LPDIRECTSOUND		lpDS;
extern LPDIRECTSOUNDBUFFER	lpDSPBuf;

#endif // AUDIO_H

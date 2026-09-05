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
#include <floatmathlib.h>

// Sample rate everything is mixed at.
#define AFILE_RATE		44100

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

	LPDIRECTSOUNDBUFFER	m_pBuffer;		// 0x04

	DSBUFFERDESC		m_desc;			// 0x08  description the buffer was made from

	byte			m_loop;			// 0x1c
	byte			m_stream;		// 0x1d  refilled while it plays
	byte			m_spatial;		// 0x1e  placed in the world

	vec3_t			m_origin;		// 0x20
	float			m_attenuation;		// 0x2c
	float			m_volume;		// 0x30
	int			m_entnum;		// 0x34
	int			m_entchannel;		// 0x38
	int			m_pitch;		// 0x3c  100 plays at the recorded rate
	int			m_rate;			// 0x40  recorded sample rate

	byte			m_stopping;		// 0x44  fading out, reaped when silent
	byte			m_autofree;		// 0x45  manager deletes it once done
	byte			m_speech;		// 0x46  drives the mouth flap

	int			m_dsvolume;		// 0x48  hundredths of a dB, 0 is full
	int			m_dspan;		// 0x4c

	char			m_name[48];		// 0x50
};

typedef char CAudio_must_match_retail_size[
	(sizeof(CAudio) == 0x80) ? 1 : -1];

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

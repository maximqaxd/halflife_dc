// audio_static.h -- sounds that live entirely in a hardware buffer

#ifndef AUDIO_STATIC_H
#define AUDIO_STATIC_H

#include "audio.h"
#include "audio_mgr.h"

// A sound quieter than this is not worth a hardware voice.
#define AUDIO_MINVOLUME		0.005f

// How long a sound sits idle before anything looks at it again.
#define AUDIO_IDLETIME		3600

// m_state
#define STATIC_PLAYING		-210		// running, watching for the end
#define STATIC_RETRY		-190		// try to get a voice again
#define STATIC_START		-180		// start it this frame
#define STATIC_IDLE		-170		// nothing to do
#define STATIC_OPENED		0		// just handed a buffer
#define STATIC_WAIT		1		// waiting for the world to run
#define STATIC_LOOPING		2		// looping, still running
#define STATIC_DONE		3		// finished, let it go

class CAudioStatic : public CAudio
{
public:
	CAudioStatic ();
	virtual ~CAudioStatic ();

	virtual void	Stop (void);
	virtual void	Suspend (void);
	virtual void	Restore (void);
	virtual void	Update (float frametime, qboolean quiet);

	qboolean	Open (sfx_t *sfx);
	qboolean	UpdateStop (void);

	int		m_state;
	float		m_timeleft;
	float		m_elapsed;
	qboolean	m_suspended;
};

void	SND_InitStatic (void);
void	SND_ShutdownStatic (void);

#endif // AUDIO_STATIC_H

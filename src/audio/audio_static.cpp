// audio_static.cpp -- sounds that live entirely in a hardware buffer
//
// A static sound duplicates one of the cached sfx buffers, so several copies of
// the same sound can play at once without another copy of the samples. There
// are only so many hardware voices, so an object does not hold one the whole
// time it exists: it runs through a small state machine that only asks for a
// voice while the sound is loud enough to be heard, and gives it back as soon
// as the sound is finished or has faded out.

#include "quakedef.h"
#include "audio.h"
#include "audio_mgr.h"
#include "audio_static.h"

/*
==================
SND_InitStatic
==================
*/
void SND_InitStatic (void)
{
}

/*
==================
SND_ShutdownStatic
==================
*/
void SND_ShutdownStatic (void)
{
}

/*
==================
CAudioStatic::CAudioStatic
==================
*/
CAudioStatic::CAudioStatic () : CAudio ()
{
	m_state = STATIC_OPENED;
	m_timeleft = 0;
	m_elapsed = 0;
	m_suspended = false;
}

/*
==================
CAudioStatic::~CAudioStatic
==================
*/
CAudioStatic::~CAudioStatic ()
{
}

/*
==================
CAudioStatic::Open

Take a copy of a cached sound. Duplicated buffers share the sample memory, so
this costs nothing but a voice.
==================
*/
qboolean CAudioStatic::Open (sfx_t *sfx)
{
	WAVEFORMATEX	wfx;
	HRESULT		hr;

	if (!lpDS)
		return false;

	if (!sfx || !sfx->buffer)
		return false;

	if (m_pBuffer)
	{
		m_pBuffer->Release ();
		m_pBuffer = NULL;
	}

	hr = lpDS->DuplicateSoundBuffer (sfx->buffer, &m_pBuffer);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	if (!m_pBuffer)
		return false;

	hr = m_pBuffer->GetFormat (&wfx, sizeof(WAVEFORMATEX), NULL);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	m_rate = wfx.nSamplesPerSec;

	if (sfx->flags & SFX_LOOPING)
		m_loop = true;

	strcpy (m_name, sfx->name);

	m_state = STATIC_WAIT;
	m_timeleft = 0;
	m_elapsed = 0;

	return true;
}

/*
==================
CAudioStatic::Stop

Nothing else refers to a static sound, so stopping it is also the end of it.
==================
*/
void CAudioStatic::Stop (void)
{
	m_autofree = true;

	CAudio::Stop ();
}

/*
==================
CAudioStatic::Suspend

Sound is being taken away. A one-shot has no way to pick up where it left off,
so it just goes; a looping sound can be started again later.
==================
*/
void CAudioStatic::Suspend (void)
{
	HRESULT	hr;

	if (!m_pBuffer || !m_loop)
	{
		Stop ();
		return;
	}

	hr = m_pBuffer->Stop ();
	if (hr != DS_OK)
		Stop ();

	m_suspended = true;
}

/*
==================
CAudioStatic::Restore
==================
*/
void CAudioStatic::Restore (void)
{
	if (!m_suspended)
		return;

	m_suspended = false;

	m_state = STATIC_WAIT;
	m_timeleft = 0;
	m_elapsed = 0;
}

/*
==================
CAudioStatic::UpdateStop

Returns false once the sound has run out. A looping sound that has faded below
the audible threshold gives its voice back and waits to be started again.
==================
*/
qboolean CAudioStatic::UpdateStop (void)
{
	DWORD	status;
	HRESULT	hr;

	if (!m_pBuffer)
		return false;

	status = 0;
	hr = m_pBuffer->GetStatus (&status);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		return false;
	}

	if (!m_loop)
		return (status & DSBSTATUS_PLAYING) != 0;

	if ((int)(log10(AUDIO_MINVOLUME) * 10 * 100) <= m_dsvolume)
	{
		if (!(status & DSBSTATUS_PLAYING))
			return Play ();
	}
	else
	{
		hr = m_pBuffer->Stop ();
		if (hr != DS_OK)
		{
			S_DSoundError (hr);
			Stop ();
			return false;
		}

		m_state = STATIC_WAIT;
		m_timeleft = 0;
		m_elapsed = 0;
	}

	return true;
}

/*
==================
CAudioStatic::Update
==================
*/
void CAudioStatic::Update (float frametime, qboolean quiet)
{
	if (m_suspended)
		return;

	m_timeleft -= frametime;
	m_elapsed += frametime;

	if (m_timeleft >= 0)
		return;

	switch (m_state)
	{
	case STATIC_OPENED:
		m_state = STATIC_IDLE;
		m_timeleft = 0;
		m_elapsed = 0;
		m_timeleft = AUDIO_IDLETIME;
		return;

	case STATIC_IDLE:
		m_timeleft = AUDIO_IDLETIME;
		return;

	case STATIC_WAIT:
		if (quiet)
			return;
		m_state = STATIC_START;
		m_timeleft = 0;
		m_elapsed = 0;
		return;

	case STATIC_DONE:
		Stop ();
		return;

	case STATIC_START:
		m_state = STATIC_RETRY;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STATIC_RETRY:
		// too quiet to be worth a voice
		if (m_dsvolume < (int)(log10(AUDIO_MINVOLUME) * 10 * 100))
		{
			if (m_loop)
				return;

			m_state = STATIC_DONE;
			m_timeleft = 0;
			m_elapsed = 0;
			return;
		}

		if (!Play ())
			return;

		m_state = STATIC_PLAYING;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STATIC_PLAYING:
	case STATIC_LOOPING:
		if (!UpdateStop ())
		{
			m_state = m_loop ? STATIC_LOOPING : STATIC_DONE;
			m_timeleft = 0;
			m_elapsed = 0;
		}
		return;

	default:
		return;
	}
}

// audio.cpp -- CAudio, one playing DirectSound buffer

#include "quakedef.h"
#include "vector.h"
#include "audio.h"
#include "audio_mgr.h"

/*
==================
CAudio::CAudio

A fresh object starts silent and centred, at the recorded pitch, and joins the
manager's list straight away so it gets updated even before it is playing.
==================
*/
CAudio::CAudio ()
{
	m_pBuffer = NULL;

	memset (&m_desc, 0, sizeof(m_desc));

	m_loop = false;
	m_stream = false;
	m_spatial = false;

	VectorClear (m_origin);
	m_attenuation = 0;
	m_volume = 0;
	m_entnum = 0;
	m_entchannel = 0;
	m_pitch = 100;
	m_rate = 0;

	m_stopping = false;
	m_autofree = false;
	m_speech = false;

	m_dsvolume = DSBVOLUME_MIN;
	m_dspan = 0;

	m_name[0] = 0;

	g_pAudioMgr->Add (this);
}

/*
==================
CAudio::~CAudio
==================
*/
CAudio::~CAudio ()
{
	g_pAudioMgr->Remove (this);

	if (m_pBuffer)
	{
		m_pBuffer->Release ();
		m_pBuffer = NULL;
	}
}

/*
==================
CAudio::Stop
==================
*/
void CAudio::Stop (void)
{
	HRESULT	hr;

	if (m_pBuffer)
	{
		hr = m_pBuffer->Stop ();
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	m_stopping = true;
	m_spatial = false;
}

/*
==================
CAudio::Close
==================
*/
void CAudio::Close (void)
{
	Stop ();
}

/*
==================
CAudio::SetSpatial

Place the sound in the world. Until this is called the object is mixed flat.
==================
*/
void CAudio::SetSpatial (float attenuation, float volume, vec3_t origin,
	int entnum, int entchannel, int pitch)
{
	m_spatial = true;

	VectorCopy (origin, m_origin);

	m_attenuation = attenuation;
	m_volume = volume;
	m_entnum = entnum;
	m_entchannel = entchannel;
	m_pitch = pitch;
}

/*
==================
CAudio::SetVolume
==================
*/
void CAudio::SetVolume (float volume)
{
	m_volume = volume;
}

/*
==================
CAudio::UpdateVolume

Work out the attenuated volume and the stereo position for this frame and push
them at the buffer. Both are read back first: the driver charges for a set even
when nothing changes.
==================
*/
void CAudio::UpdateVolume (void)
{
	cl_entity_t	*ent;
	Vector	dir;
	float	gain, dot;
	int	current;
	HRESULT	hr;

	m_dsvolume = DSBVOLUME_MIN;
	m_dspan = 0;

	if (!m_spatial)
	{
		m_dsvolume = (int)(log(volume.value) * 10.0f * 100.0f);
	}
	else if (m_entnum == cl.viewentity)
	{
		// coming from the player -- no attenuation, no panning
		if (m_volume <= 0)
			m_dsvolume = DSBVOLUME_MIN;
		else
			m_dsvolume = (int)(log(m_volume * volume.value) * 10.0f * 100.0f);
	}
	else
	{
		// follow the entity the sound was started on
		if (m_entnum > 0 && m_entnum < sv.num_edicts)
		{
			ent = &cl_entities[m_entnum];
			if (ent && ent->model)
			{
				VectorCopy (ent->origin, m_origin);

				// brush models sit at a corner of their box, so play
				// from the middle of it instead
				if (ent->model->type == mod_brush)
				{
					m_origin[0] += (ent->model->mins[0] + ent->model->maxs[0]) * 0.5f;
					m_origin[1] += (ent->model->mins[1] + ent->model->maxs[1]) * 0.5f;
					m_origin[2] += (ent->model->mins[2] + ent->model->maxs[2]) * 0.5f;
				}
			}
		}

		VectorSubtract (m_origin, listener_origin, dir);

		dot = DotProduct (listener_right, dir.Normalize());

		gain = volume.value * ((1.0f - m_attenuation * (float)sqrt(DotProduct(dir, dir))) * m_volume);
		if (m_speech)
			gain *= 1.1f;

		if (gain <= 0)
			m_dsvolume = DSBVOLUME_MIN;
		else
			m_dsvolume = (int)(log(gain) * 10.0f * 100.0f);

		// the pan scale only changes when the cvar does
		static float	last_sep = 0;
		static float	sep_scale = last_sep * 100.0f;

		if (last_sep != stereo_sep.value)
		{
			last_sep = stereo_sep.value;
			sep_scale = stereo_sep.value * 100.0f;
		}

		m_dspan = (int)(dot * sep_scale);
	}

	if (m_dsvolume < DSBVOLUME_MIN)
		m_dsvolume = DSBVOLUME_MIN;
	if (m_dsvolume > DSBVOLUME_MAX)
		m_dsvolume = DSBVOLUME_MAX;

	if (m_dspan < DSBPAN_LEFT)
		m_dspan = DSBPAN_LEFT;
	if (m_dspan > DSBPAN_RIGHT)
		m_dspan = DSBPAN_RIGHT;

	if (!m_pBuffer)
		return;

	current = m_dsvolume;
	hr = m_pBuffer->GetVolume ((LPLONG)&current);
	if (hr != DS_OK)
		S_DSoundError (hr);

	if (current != m_dsvolume)
	{
		hr = m_pBuffer->SetVolume (m_dsvolume);
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	current = m_dspan;
	hr = m_pBuffer->GetPan ((LPLONG)&current);
	if (hr != DS_OK)
		S_DSoundError (hr);

	if (current != m_dspan)
	{
		hr = m_pBuffer->SetPan (m_dspan);
		if (hr != DS_OK)
			S_DSoundError (hr);
	}
}

/*
==================
CAudio::SetPitch

Pitch is a percentage of the rate the sound was recorded at.
==================
*/
void CAudio::SetPitch (int pitch)
{
	DWORD	freq, current;
	HRESULT	hr;

	m_pitch = pitch;

	if (!m_pBuffer)
		return;

	freq = (DWORD)(((float)m_pitch / 100.0f) * (float)m_rate);

	current = freq;
	hr = m_pBuffer->GetFrequency (&current);
	if (hr != DS_OK)
		S_DSoundError (hr);

	if (current != freq)
	{
		hr = m_pBuffer->SetFrequency (freq);
		if (hr != DS_OK)
			S_DSoundError (hr);
	}
}

/*
==================
CAudio::Play

Start the buffer. If the hardware voices are all spoken for, give the mixer a
few frames to retire something before giving up on the sound.
==================
*/
qboolean CAudio::Play (void)
{
	int	tries;
	HRESULT	hr;

	if (!m_pBuffer)
		return false;

	UpdateVolume ();
	SetPitch (m_pitch);

	hr = m_pBuffer->Play (0, 0, m_stream ? DSBPLAY_LOOPING : 0);

	if (hr == DSERR_ALLOCATED && !m_loop && S_GetSoundtime (g_pAudioMgr))
	{
		for (tries = 0 ; tries < 4 ; tries++)
		{
			hr = m_pBuffer->Play (0, 0, 0);
			if (hr == DS_OK)
				return true;
		}
	}

	if (hr == DS_OK)
		return true;

	S_DSoundError (hr);
	Stop ();

	return false;
}

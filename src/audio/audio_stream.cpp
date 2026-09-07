// audio_stream.cpp -- long sounds played straight off the disc
//
// A stream owns a ring buffer that the driver plays continuously. While one
// half is playing the other is refilled from the file, so a sound of any length
// costs a fixed amount of audio RAM. Reads are started asynchronously and
// picked up on a later frame, which keeps the disc off the critical path.
//
// Speech carries a MOTH chunk of mouth openings next to the samples; as the
// stream plays, the value under the play cursor is written into the talking
// entity so the model's jaw follows the sound.

#include "quakedef.h"
#include "audio.h"
#include "audio_mgr.h"
#include "audio_static.h"
#include "audio_stream.h"

void CAudioStream::CheckOpen (void)
{
	if (!m_ready && COM_OpenFileAsync (m_path, m_file, &m_async) == 0)
		m_ready = true;
}

qboolean CAudioStream::PlayFromOffset (void)
{
	HRESULT hr = m_pBuffer->SetCurrentPosition (m_dataofs);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		return false;
	}
	return CAudio::Play ();
}

void CAudioStream::FreeMouth (void)
{
	if (m_mouth)
	{
		MnemoFree (m_mouth);
		m_mouth = NULL;
		m_mouthsize = 0;
		if (m_entnum > 0 && (m_entchannel == CHAN_VOICE || m_entchannel == CHAN_STREAM))
			cl_entities[m_entnum].mouth.mouthopen = 0;
	}
}

void CAudioStream::FreeBuffer (void)
{
	if (m_pBuffer)
	{
		m_pBuffer->Release ();
		m_pBuffer = NULL;
	}
}

cvar_t	mouthdelay	= {"mouthdelay", "0.066"};
cvar_t	mouthrate	= {"mouthrate", "1000"};
cvar_t	mouthscale	= {"mouthscale", "1.5"};

static byte		mouth_registered = true;


/*
==================
SND_InitMouth
==================
*/
void SND_InitMouth (void)
{
	if (!mouth_registered)
		return;

	mouth_registered = false;

	Cmd_AddCommand ("speak", Cmd_speak_f);

	Cvar_RegisterVariable (&mouthdelay);
	Cvar_RegisterVariable (&mouthscale);
	Cvar_RegisterVariable (&mouthrate);
}

/*
==================
SND_ShutdownMouth
==================
*/
void SND_ShutdownMouth (void)
{
}

/*
==================
CAudioStream::CAudioStream
==================
*/
CAudioStream::CAudioStream (int buffersize) : CAudio ()
{
	m_state = STREAM_OPEN;
	m_timeleft = 0;
	m_elapsed = 0;

	m_buffersize = buffersize;
	m_halfbuffer = buffersize / 2;

	memset (&m_async, 0, sizeof(m_async));
	memset (m_path, 0, sizeof(m_path));

	m_filesize = 0;
	m_readpos = 0;
	m_writepos = 0;
	m_dataofs = 0;
	m_filedata = NULL;

	m_playpos = 0;
	m_suspended = 0;
	m_pending = 0;
	m_fill = 0;

	m_ready = true;
	m_trystatic = true;

	m_mouth = NULL;
	m_mouthsize = 0;
}

/*
==================
CAudioStream::~CAudioStream
==================
*/
CAudioStream::~CAudioStream ()
{
	if (m_filedata)
	{
		MnemoFree (m_filedata);
		m_filedata = NULL;
	}

	if (m_mouth)
	{
		MnemoFree (m_mouth);
		m_mouth = NULL;
		m_mouthsize = 0;

		// stop the model talking
		if (m_entnum > 0 && (m_entchannel == CHAN_VOICE || m_entchannel == CHAN_STREAM))
			cl_entities[m_entnum].mouth.mouthopen = 0;
	}
}

/*
==================
CAudioStream::Stop
==================
*/
void CAudioStream::Stop (void)
{
	if (m_stopping)
		return;

	// a stream that has not started yet can drop its buffer straight away;
	// one that is running has to wait for the driver to let go
	if (!m_stream)
	{
		if (m_pBuffer)
		{
			m_pBuffer->Release ();
			m_pBuffer = NULL;
		}
	}
	else
	{
		m_autofree = true;
	}

	if (m_mouth)
	{
		MnemoFree (m_mouth);
		m_mouth = NULL;
		m_mouthsize = 0;

		if (m_entnum > 0 && (m_entchannel == CHAN_VOICE || m_entchannel == CHAN_STREAM))
			cl_entities[m_entnum].mouth.mouthopen = 0;
	}

	CAudio::Stop ();
}

/*
==================
CAudioStream::Suspend
==================
*/
void CAudioStream::Suspend (void)
{
	HRESULT	hr;

	if (!m_pBuffer)
		return;

	hr = m_pBuffer->Stop ();
	if (hr != DS_OK)
		Stop ();

	m_suspended = true;
}

/*
==================
CAudioStream::Restore
==================
*/
void CAudioStream::Restore (void)
{
	if (m_suspended)
	{
		m_suspended = 0;
		if (!CAudio::Play ())
			Stop ();
	}
}

/*
==================
CAudioStream::Restart
==================
*/
void CAudioStream::Restart (void)
{
}

/*
==================
VOX_LoadFile

Turn a sound name into a path under sound\ and start opening it. Nothing is
read here -- the state machine picks the file up once it is open.
==================
*/
qboolean VOX_LoadFile (CAudioStream *stream, char *name)
{
	if (!name)
		return false;

	if (!lpDS)
		return false;

	strcpy (stream->m_name, name);

	strcpy (stream->m_path, "sound\\");
	strcat (stream->m_path, name);

	if (!strrchr (stream->m_path, '.'))
		strcat (stream->m_path, ".wav");

	// mark the record closed so the first update opens it for async reading
	stream->m_file[2] = -1;

	if (COM_FindFile (stream->m_path, stream->m_file, NULL) == -1)
	{
#ifdef SND_DEBUG
		Con_Printf ("VOX: %s not found\n", stream->m_path);
#endif
		stream->m_path[0] = 0;
		stream->Stop ();
		return false;
	}

	stream->m_state = STREAM_ALLOC;
	stream->m_timeleft = 0;
	stream->m_elapsed = 0;

	return true;
}

/*
==================
VOX_LoadSound

Same, for a sentence. The leading '!' is the marker, the rest names an entry in
sentences.txt whose text is the real file name.
==================
*/
qboolean VOX_LoadSound (CAudioStream *stream, char *name)
{
	char		*sentence;
	char		*text;
	qboolean	loaded;

	if (!name)
		return false;

	if (!lpDS)
		return false;

	sentence = name;
	if (sentence[0] == '!')
		sentence++;

	text = S_FindNameByIndex (sentence, NULL);
	if (!text)
	{
#ifdef SND_DEBUG
		Con_Printf ("VOX: no sentence named %s\n", sentence);
#endif
		stream->Stop ();
		return false;
	}
#ifdef SND_DEBUG
	Con_Printf ("VOX: %s -> %s\n", sentence, text);
#endif

	// a sentence always streams, however short it turns out to be
	stream->m_trystatic = false;

	loaded = VOX_LoadFile (stream, text);

	// keep the sentence name, not the file it resolved to
	strcpy (stream->m_name, name);

	return loaded;
}

/*
==================
CAudioStream::AllocBuffer

Take the staging buffer the file is read into, and kick off the first read.
==================
*/
qboolean CAudioStream::AllocBuffer (void)
{
	if (m_filedata)
	{
		MnemoFree (m_filedata);
		m_filedata = NULL;
	}

	m_filedata = (byte *)MnemoAlloc (m_buffersize, MNEMO_FLAG_MALLOC, 0, "streaming sound buffer");
	if (!m_filedata)
	{
#ifdef SND_DEBUG
		Con_Printf ("stream: no room for %d bytes\n", m_buffersize);
#endif
		return false;
	}

	// until the header says otherwise, one buffer's worth is all we may read
	m_readpos = 0;
	m_filesize = m_buffersize;

	ReadAhead (0);

	return true;
}

/*
==================
CAudioStream::ReadAhead

Start the next block coming in behind the play cursor.
==================
*/
void CAudioStream::ReadAhead (int offset)
{
	int	read;

	if (!m_filedata)
		return;

	if (m_readpos >= m_filesize)
		return;

	m_ready = false;

	COM_LoadFileLimitAsync (m_path, m_readpos, m_buffersize, &read,
		m_file, m_filedata + offset, &m_async);

	m_readpos += read;
}

/*
==================
CAudioStream::ParseWavHeader

Walk the chunks at the head of the file. A sound small enough to fit in the
staging buffer is not worth streaming, so it is handed to the sfx cache and
replayed as an ordinary static sound instead.
==================
*/
qboolean CAudioStream::ParseWavHeader (void)
{
	byte		*chunk;
	int		id;
	unsigned	len;
	sfx_t		*sfx;
	CAudioStatic	*st;

	if (((int *)m_filedata)[0] != MAKEFOURCC('R','I','F','F') ||
		((int *)m_filedata)[2] != MAKEFOURCC('W','A','V','E'))
	{
#ifdef SND_DEBUG
		Con_Printf ("stream: %s is not a wav\n", m_path);
#endif
		return false;
	}

	// a short enough sound is worth caching whole and replaying as a static
	if (m_trystatic && ((int *)m_filedata)[1] + 12 < m_buffersize)
	{
		sfx = S_CacheSoundRecord (g_pAudioMgr, m_path, m_filedata, m_filesize);
		if (sfx)
		{
			st = new CAudioStatic;

			if (m_spatial)
			{
				m_spatial = false;
				st->SetSpatial (m_attenuation, m_volume, m_origin,
					m_entnum, m_entchannel, m_pitch);
			}

			st->Open (sfx);
			return false;
		}
	}

	chunk = m_filedata + 12;

	while (chunk - m_filedata >= 0 && chunk - m_filedata <= m_buffersize)
	{
		// chunks are only word aligned, so the header comes out a byte at a time
		memcpy (&id, chunk, 4);
		chunk += 4;
		memcpy (&len, chunk, 4);
		chunk += 4;

		if (id == MAKEFOURCC('f','m','t',' '))
		{
			m_desc.lpwfxFormat = (WAVEFORMATEX *)chunk;
			m_rate = (int)chunk[4] | ((int)chunk[5] << 8) |
				((int)chunk[6] << 16) | ((int)chunk[7] << 24);
		}
		else if (id == MAKEFOURCC('M','O','T','H'))
		{
			m_mouth = (byte *)MnemoAlloc (len, MNEMO_FLAG_MALLOC, 0, "mouth data");
			if (m_mouth)
			{
				memcpy (m_mouth, chunk, len);
				m_mouthsize = len;
				m_speech = true;
			}
		}
		else if (id == MAKEFOURCC('d','a','t','a'))
		{
			m_dataofs = chunk - m_filedata;
			m_filesize = m_dataofs + len;
#ifdef SND_DEBUG
			Con_Printf ("stream: %s data at %d, %d bytes, %d hz\n",
				m_path, m_dataofs, len, m_rate);
#endif
			return true;
		}

		chunk += len;
		if (len & 1)
			chunk++;
	}

#ifdef SND_DEBUG
	Con_Printf ("stream: %s has no data chunk\n", m_path);
#endif
	return false;
}

/*
==================
CAudioStream::StartPlayback

Build the ring buffer and prime it. A sound that turns out to fit entirely in
one buffer is played as a one-shot rather than looped.
==================
*/
qboolean CAudioStream::StartPlayback (void)
{
	HRESULT		hr;

	m_desc.dwSize = sizeof(DSBUFFERDESC);
	// a stream is refilled from main memory, so keep it out of audio RAM
	m_desc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN |
		DSBCAPS_CTRLVOLUME;

	if (m_filesize < m_buffersize)
		m_desc.dwBufferBytes = m_filesize - m_dataofs;
	else
		m_desc.dwBufferBytes = m_buffersize;

	if (m_pBuffer)
	{
		m_pBuffer->Release ();
		m_pBuffer = NULL;
	}

	hr = lpDS->CreateSoundBuffer (&m_desc, &m_pBuffer, NULL);
	if (hr != DS_OK)
	{
#ifdef SND_DEBUG
		Con_Printf ("stream: %s buffer of %d bytes refused\n",
			m_path, m_desc.dwBufferBytes);
#endif
		S_DSoundError (hr);
		return false;
	}

	if (!m_pBuffer)
		return false;

	if (m_filesize < m_buffersize)
	{
		// the whole sound is already in memory
		LockCopyTail (m_dataofs, m_filesize - m_dataofs);

		m_dataofs = 0;
		m_stream = false;

		MnemoFree (m_filedata);
		m_filedata = NULL;
	}
	else
	{
		LockAndCopy (0, m_dataofs, 0);
		LockAndCopy (1, 0, 0);

		ReadAhead (0);

		m_playpos = 0;
		m_pending = 2;
		m_stream = true;
		m_writepos = m_buffersize;
	}

	return true;
}

/*
==================
CAudioStream::LockAndCopy

Copy one half of the staging buffer into the ring buffer and blank the source
behind us, so a read that does not arrive in time plays silence rather than the
last block over again. leadin and fadeout blank the ends when a sound starts
part way in or runs out.
==================
*/
qboolean CAudioStream::LockAndCopy (int half, int leadin, int fadeout)
{
	void	*ptr1, *ptr2;
	DWORD	bytes1, bytes2;
	int	offset;
	HRESULT	hr;

	offset = half ? m_halfbuffer : 0;

	ptr1 = ptr2 = NULL;
	bytes1 = bytes2 = 0;

	hr = m_pBuffer->Lock (offset, m_halfbuffer, &ptr1, &bytes1, &ptr2, &bytes2, 0);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	if (ptr1 && bytes1)
	{
		memcpy (ptr1, m_filedata + offset, bytes1);
		memset (m_filedata + offset, 0, bytes1);

		if (leadin > 0)
			memset (ptr1, 0, leadin);

		if (fadeout > 0)
			memset ((byte *)ptr1 + bytes1 - fadeout, 0, fadeout);
	}

	hr = m_pBuffer->Unlock (ptr1, bytes1, ptr2, bytes2);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	return true;
}

/*
==================
CAudioStream::LockCopyTail

Fill a buffer that holds the whole sound in one go.
==================
*/
qboolean CAudioStream::LockCopyTail (int offset, int bytes)
{
	void	*ptr1, *ptr2;
	DWORD	bytes1, bytes2;
	HRESULT	hr;

	ptr1 = ptr2 = NULL;
	bytes1 = bytes2 = 0;

	hr = m_pBuffer->Lock (0, bytes, &ptr1, &bytes1, &ptr2, &bytes2, 0);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	if (ptr1 && bytes1)
		memcpy (ptr1, m_filedata + offset, bytes1);

	hr = m_pBuffer->Unlock (ptr1, bytes1, ptr2, bytes2);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return false;
	}

	return true;
}

/*
==================
CAudioStream::Service

Watch the play cursor and refill whichever half it has just left. Once the file
runs out the tail is blanked so the sound ends cleanly instead of looping back
into stale samples.
==================
*/
void CAudioStream::Service (void)
{
	DWORD	play;
	DWORD	status;
	int	played, left;
	HRESULT	hr;

	if (!m_stream)
	{
		// one-shot: nothing to feed, just wait for it to finish
		status = 0;
		hr = m_pBuffer->GetStatus (&status);
		if (hr != DS_OK)
		{
			S_DSoundError (hr);
			Stop ();
			return;
		}

		if (!(status & DSBSTATUS_PLAYING))
		{
			if (m_loop)
				Restart ();

			Stop ();
		}

		return;
	}

	hr = m_pBuffer->GetCurrentPosition (&play, NULL);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		Stop ();
		return;
	}

	if (!m_pending)
	{
		// running out: let it play to the point the data stopped
		played = play - m_playpos;
		if (played < 0)
			played += m_buffersize;

		if (!m_fill)
		{
			if (played > m_buffersize / 8)
				m_fill = 1;
		}
		else if (played < m_buffersize / 8)
		{
			LockAndCopy (0, m_halfbuffer, 0);
			LockAndCopy (1, m_halfbuffer, 0);

			if (m_loop)
				Restart ();

			Stop ();
		}

		return;
	}

	if (m_pending == 2 && play >= (DWORD)m_halfbuffer)
	{
		// the play cursor has left the first half
		m_writepos += m_halfbuffer;

		left = m_writepos - m_filesize;
		if (left < 0)
			left = 0;
		if (left >= m_halfbuffer)
			left = m_halfbuffer - 1;

		LockAndCopy (0, 0, left);

		if (left)
		{
			m_playpos = m_halfbuffer - left;
			m_fill = 0;
			m_pending = 0;
		}
		else
		{
			m_pending = 1;
		}

		return;
	}

	if (m_pending == 1 && play < (DWORD)m_halfbuffer)
	{
		// and now the second half
		m_writepos += m_halfbuffer;

		left = m_writepos - m_filesize;
		if (left < 0)
			left = 0;
		if (left >= m_halfbuffer)
			left = m_halfbuffer - 1;

		LockAndCopy (1, 0, 0);

		ReadAhead (0);

		if (!left)
		{
			m_pending = 2;
		}
		else
		{
			m_playpos = m_buffersize - left;
			m_fill = 0;
			m_pending = 0;
		}
	}
}

/*
==================
CAudioStream::Update
==================
*/
void CAudioStream::Update (float frametime, qboolean quiet)
{
	if (m_stopping)
	{
		if (!m_ready && COM_OpenFileAsync (m_path, m_file, &m_async) == 0)
			m_ready = true;

		if (m_ready)
			m_autofree = true;

		return;
	}

	m_timeleft -= frametime;
	m_elapsed += frametime;

	if (m_timeleft >= 0)
		return;

	switch (m_state)
	{
	case STREAM_OPEN:
		// nothing has been handed to us yet, so go quiet instead of
		// retrying the whole chain every frame
		m_state = STREAM_IDLE;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STREAM_IDLE:
		m_timeleft = STREAM_IDLE_TIME;
		return;

	case STREAM_ALLOC:
		if (!AllocBuffer ())
		{
			m_state = STREAM_DONE;
			m_timeleft = 0;
			m_elapsed = 0;
			return;
		}

		m_state = STREAM_OPENING;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STREAM_OPENING:
		if (!m_ready && COM_OpenFileAsync (m_path, m_file, &m_async) == 0)
			m_ready = true;

		if (!m_ready)
			return;

		m_state = STREAM_HEADER;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STREAM_HEADER:
		if (!ParseWavHeader ())
		{
			m_state = STREAM_DONE;
			m_timeleft = 0;
			m_elapsed = 0;
			return;
		}

		if (!StartPlayback ())
		{
			m_state = STREAM_DONE;
			m_timeleft = 0;
			m_elapsed = 0;
			return;
		}

		m_state = STREAM_START;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STREAM_START:
		if (m_pBuffer->SetCurrentPosition (m_dataofs) != DS_OK || !Play ())
			return;

		m_state = STREAM_PLAYING;
		m_timeleft = 0;
		m_elapsed = 0;
		// fall through

	case STREAM_PLAYING:
		if (!m_ready && COM_OpenFileAsync (m_path, m_file, &m_async) == 0)
			m_ready = true;

		Service ();
		SND_MoveMouth (this, frametime);
		return;

	case STREAM_DONE:
		Stop ();
		return;

	default:
		return;
	}
}

/*
==================
SND_MoveMouth

Read the mouth opening for wherever the sound has got to and write it into the
talking entity. mouthdelay pulls the jaw ahead of the sound so it does not look
like it is lagging, mouthrate says how many samples a mouth byte covers, and
mouthscale trades range for exaggeration.
==================
*/
void SND_MoveMouth (CAudioStream *stream, float frametime)
{
	DWORD	play;
	float	pos, frac;
	int	index, value;
	HRESULT	hr;

	if (stream->m_stopping)
		return;

	if (!stream->m_mouth)
		return;

	if (stream->m_entnum <= 0)
		return;

	if (stream->m_entchannel != CHAN_VOICE && stream->m_entchannel != CHAN_STREAM)
		return;

	hr = stream->m_pBuffer->GetCurrentPosition (&play, NULL);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		stream->Stop ();
		return;
	}

	// wind the cursor forward to the block being played out of
	if (stream->m_writepos)
	{
		while (play < (DWORD)stream->m_writepos)
			play += stream->m_buffersize;

		play -= stream->m_buffersize;
	}

	pos = (float)(play * 2) / mouthrate.value + mouthdelay.value;

	index = (int)pos;
	frac = pos - index;

	if (index < 0 || index > stream->m_mouthsize)
	{
		frac = 0;
	}
	else if (index == stream->m_mouthsize)
	{
		frac = stream->m_mouth[index] * (1.0f - frac);
	}
	else
	{
		frac = stream->m_mouth[index] * (1.0f - frac)
			+ stream->m_mouth[index + 1] * frac;
	}

	value = (int)(mouthscale.value * frac);

	if (value < 1)
		value = 0;
	if (value > MOUTH_MAX)
		value = MOUTH_MAX;

	cl_entities[stream->m_entnum].mouth.mouthopen = (byte)value;
}

/*
==================
Cmd_speak_f
==================
*/
void Cmd_speak_f (void)
{
	CAudioStream	*stream;

	if (Cmd_Argc () < 2)
		return;

	stream = new CAudioStream (STREAM_BUFFERSIZE);
	if (!stream)
		return;

	VOX_LoadSound (stream, Cmd_Argv (1));
}

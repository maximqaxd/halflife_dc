// audio_mgr.cpp -- the audio manager and the sound cache
//
// Every live CAudio registers itself here. Once a frame the manager walks the
// list, lets each object update its volume and refill itself, and deletes the
// ones that have finished. It also owns the sfx cache: the DirectSound buffers
// that hold precached sounds, stamped with the server count so that a level
// change can throw away everything the new map does not want.

#include "quakedef.h"
#include "audio.h"
#include "afile.h"
#include <platutil.h>

#include "audio_mgr.h"
#include "audio_static.h"
#include "audio_stream.h"
#include "audio_cd.h"

CAudioMgr	*g_pAudioMgr;
CAudio		*g_audio[MAX_AUDIO];
sfx_t		*known_sfx[MAX_SFX];
int		s_servercount;

static qboolean	s_registered = true;
static float	s_laststereo;

cvar_t	volume		= {"volume", "0.7", FCVAR_ARCHIVE};
cvar_t	suitvolume	= {"suitvolume", "0.75", FCVAR_ARCHIVE};
cvar_t	nosound		= {"nosound", "0"};
cvar_t	snd_show	= {"snd_show", "0"};
cvar_t	stereo_sep	= {"stereo_sep", "20"};
cvar_t	stereo		= {"stereo", "0", FCVAR_ARCHIVE};
cvar_t	soundextra	= {"soundextra", "0"};

/*
==================
CAudioMgr::CAudioMgr
==================
*/
CAudioMgr::CAudioMgr ()
{
	int	i;

	m_lasttime = GetTickCount ();
	m_mastervolume = -1;
	m_precaching = false;

	g_pAudioMgr = this;

	if (s_registered)
	{
		s_registered = false;

		Cvar_RegisterVariable (&volume);
		Cvar_RegisterVariable (&suitvolume);
		Cvar_RegisterVariable (&nosound);
		Cvar_RegisterVariable (&snd_show);
		Cvar_RegisterVariable (&stereo_sep);
		Cvar_RegisterVariable (&stereo);
		Cvar_RegisterVariable (&soundextra);

		for (i = 0 ; i < MAX_AUDIO ; i++)
			g_audio[i] = NULL;

		for (i = 0 ; i < MAX_SFX ; i++)
			known_sfx[i] = NULL;

		AFile_Init ();

		// the console setting decides whether we come up in stereo
		if (FirmwareGetSoundMode ())
			Cvar_SetValue ("stereo", 0);
		else
			Cvar_SetValue ("stereo", 1);
	}

	SND_InitMouth ();
	CDAudio_Init ();
	SND_InitStatic ();
}

/*
==================
CAudioMgr::~CAudioMgr
==================
*/
CAudioMgr::~CAudioMgr ()
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (g_audio[i])
			g_audio[i]->Stop ();
	}

	Update (false);

	SND_ShutdownStatic ();
	CDAudio_Shutdown ();
	SND_ShutdownMouth ();

	g_pAudioMgr = NULL;
}

/*
==================
S_InitSoundCache
==================
*/
void S_InitSoundCache (void)
{
	new CAudioMgr;
}

/*
==================
S_Shutdown
==================
*/
void S_Shutdown (void)
{
	if (g_pAudioMgr)
		delete g_pAudioMgr;
}

/*
==================
CAudioMgr::Add
==================
*/
void CAudioMgr::Add (CAudio *audio)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
		{
			g_audio[i] = audio;
			return;
		}
	}

	// no free slot in the audio manager
}

/*
==================
CAudioMgr::Remove
==================
*/
void CAudioMgr::Remove (CAudio *audio)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (g_audio[i] == audio)
		{
			g_audio[i] = NULL;
			return;
		}
	}

	// nothing to remove from the audio manager
}

/*
==================
CAudioMgr::Update

quiet is set while the game is not running the world -- objects keep their
buffers filled but nothing is repositioned and the device is left alone.
==================
*/
void CAudioMgr::Update (qboolean quiet)
{
	int		i, config;
	DWORD		now;
	float		frametime;
	CAudio		*audio;
	HRESULT		hr;
	LPKSPROPERTYSET	pKs;

	// the audio objects run off the wall clock, not the game clock: they have
	// to keep filling their buffers while the world is loading and frozen
	now = GetTickCount ();
	frametime = (float)(now - m_lasttime) * 0.001f;
	m_lasttime = now;

	if (m_precaching)
	{
		m_precaching = false;
		S_FreeUnusedSounds ();
	}

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		audio = g_audio[i];
		if (!audio)
			continue;

		if (!quiet)
			audio->UpdateVolume ();

		g_audio[i]->Update (frametime, quiet);

		audio = g_audio[i];
		if (audio && audio->m_autofree)
			delete audio;
	}

	if (quiet)
		return;

	// master volume is a property on the device, not on a buffer
	config = nosound.value ? 0 : 12;
	if (m_mastervolume != config)
	{
		m_mastervolume = config;

		hr = lpDS->QueryInterface (IID_IKsPropertySet, (LPVOID *)&pKs);
		if (hr != DS_OK)
			S_DSoundError (hr);

		hr = pKs->Set (DSPROPSETID_MasterControl, 0, NULL, 0,
				&config, sizeof(config));
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	// the primary buffer has to keep running or the mix goes away
	if (lpDSPBuf)
	{
		hr = lpDSPBuf->Play (0, 0, 0);
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	if (s_laststereo != stereo.value)
	{
		s_laststereo = stereo.value;

		hr = lpDS->SetSpeakerConfig (stereo.value > 0.5f ? DSSPEAKER_STEREO : DSSPEAKER_MONO);
		if (hr != DS_OK)
			S_DSoundError (hr);
	}

	if (snd_show.value)
		S_SoundInfo ();
}

/*
==================
CAudioMgr::StopAll
==================
*/
void CAudioMgr::StopAll (qboolean close)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
			continue;

		if (close)
			g_audio[i]->Close ();
		else
			g_audio[i]->Stop ();
	}

	Update (false);
}

/*
==================
CAudioMgr::Block

Hand the device over, or take it back.
==================
*/
void CAudioMgr::Block (qboolean block)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
			continue;

		if (block)
			g_audio[i]->Suspend ();
		else
			g_audio[i]->Restore ();
	}
}

/*
==================
S_GetSoundtime

The quietest of the streams that are still playing, so a sound that cannot get
a hardware voice knows whether it is worth waiting for one.
==================
*/
int S_GetSoundtime (CAudioMgr *mgr)
{
	int	i, quietest;
	DWORD	status;
	CAudio	*audio;
	HRESULT	hr;

	quietest = 0;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		audio = g_audio[i];
		if (!audio || !audio->m_loop || !audio->m_pBuffer)
			continue;

		status = 0;
		hr = audio->m_pBuffer->GetStatus (&status);
		if (hr != DS_OK)
			S_DSoundError (hr);

		if ((status & DSBSTATUS_PLAYING) && audio->m_dsvolume < quietest)
			quietest = audio->m_dsvolume;
	}

	return 0;
}

/*
==================
S_PickChannel
==================
*/
CAudio *S_PickChannel (int entnum, int entchannel)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
			continue;
		if (!g_audio[i]->m_spatial)
			continue;
		if (g_audio[i]->m_entnum != entnum)
			continue;
		if (g_audio[i]->m_entchannel != entchannel)
			continue;

		return g_audio[i];
	}

	return NULL;
}

/*
==================
S_FindChannelForName
==================
*/
CAudio *S_FindChannelForName (char *name)
{
	int	i;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
			continue;
		if (Q_stricmp (name, g_audio[i]->m_name))
			continue;

		return g_audio[i];
	}

	return NULL;
}

/*
==================
S_FindNameNoCreate
==================
*/
sfx_t *S_FindNameNoCreate (char *name)
{
	int	i;

	for (i = 0 ; i < MAX_SFX ; i++)
	{
		if (!known_sfx[i])
			continue;
		if (!Q_stricmp (name, known_sfx[i]->name))
			return known_sfx[i];
	}

	return NULL;
}

/*
==================
S_AllocSfx

Returns the index of the sound, allocating the record if this is the first the
cache has heard of it. A name stored with a leading '*' also matches the plain
name, so the server can flag a sound without changing how it is looked up.
==================
*/
int S_AllocSfx (char *name)
{
	int	i, free;
	sfx_t	*sfx;

	free = -1;

	for (i = 0 ; i < MAX_SFX ; i++)
	{
		if (!known_sfx[i])
		{
			if (free == -1)
				free = i;
			continue;
		}

		if (!Q_stricmp (name, known_sfx[i]->name))
			return i;

		if (known_sfx[i]->name[0] == '*' && !Q_stricmp (name, known_sfx[i]->name + 1))
			return i;
	}

	if (free == -1)
		return free;			// out of sfx_t slots

	sfx = (sfx_t *)MnemoAlloc (sizeof(sfx_t), MNEMO_FLAG_MALLOC, 0, "sfx_t record");
	known_sfx[free] = sfx;

	memset (sfx, 0, sizeof(sfx_t));
	strcpy (sfx->name, name);

	sfx->buffer = NULL;
	sfx->flags = 0;
	sfx->servercount = (short)s_servercount;

	return free;
}

/*
==================
S_PrecacheSound
==================
*/
sfx_t *S_PrecacheSound (CAudioMgr *mgr, char *name, qboolean fromserver)
{
	int	i;
	sfx_t	*sfx;

	mgr->m_precaching = true;

	sfx = S_FindNameNoCreate (name);

	if (!sfx)
	{
		for (i = 0 ; i < MAX_SFX ; i++)
		{
			if (known_sfx[i])
				continue;

			known_sfx[i] = (sfx_t *)MnemoAlloc (sizeof(sfx_t), MNEMO_FLAG_MALLOC, 0, "sfx_t record");
			sfx = known_sfx[i];
			if (!sfx)
				return NULL;

			memset (sfx, 0, sizeof(sfx_t));
			strcpy (sfx->name, name);
			sfx->servercount = (short)s_servercount;

			if (fromserver)
				sfx->flags |= SFX_KEEP;

			if (S_IsPlayableSound (name))
				S_LoadSound (mgr, sfx);

			return sfx;
		}

		return NULL;			// out of sfx_t slots
	}

	sfx->servercount = (short)s_servercount;

	if (fromserver)
		sfx->flags |= SFX_KEEP;

	if (!sfx->buffer && S_IsPlayableSound (name))
		S_LoadSound (mgr, sfx);

	return sfx;
}

/*
==================
S_CacheSoundRecord

Cache a sound whose data we already have in memory. The path still carries the
"sound\" prefix the streaming code built, so step over it.
==================
*/
sfx_t *S_CacheSoundRecord (CAudioMgr *mgr, char *path, byte *data, int size)
{
	char *name;
	int		i;
	sfx_t		*sfx;

	name = path + 6;

	if (!S_IsPlayableSound (name))
		return NULL;

	sfx = S_FindNameNoCreate (name);

	if (!sfx)
	{
		for (i = 0 ; i < MAX_SFX ; i++)
		{
			if (known_sfx[i])
				continue;

			known_sfx[i] = (sfx_t *)MnemoAlloc (sizeof(sfx_t), MNEMO_FLAG_MALLOC, 0, "sfx_t record");
			sfx = known_sfx[i];
			if (!sfx)
				return NULL;

			memset (sfx, 0, sizeof(sfx_t));
			strcpy (sfx->name, name);
			sfx->servercount = (short)s_servercount;

			S_LoadDSoundBuffer (sfx, data, size);
			return sfx;
		}

		return NULL;
	}

	sfx->servercount = (short)s_servercount;

	if (!sfx->buffer)
		S_LoadDSoundBuffer (sfx, data, size);

	return sfx;
}

/*
==================
S_IsPlayableSound

Sentences and the streamed tracks are not ordinary wavs and never get a cached
buffer of their own.
==================
*/
qboolean S_IsPlayableSound (char *name)
{
	if (name[0] == '!')
		return false;

	if (!strncmp (name, "tride", 5))
		return false;

	return true;
}

/*
==================
S_LoadSound
==================
*/
void S_LoadSound (CAudioMgr *mgr, sfx_t *sfx)
{
	char		path[52];
	byte		*data;
	int		size;
	int		hFile[3];

	strcpy (path, "sound\\");
	if (sfx->name[0] == '*')
		strcat (path, sfx->name + 1);
	else
		strcat (path, sfx->name);

	hFile[0] = 0;
	hFile[1] = 0;
	hFile[2] = -1;

	data = (byte *)COM_LoadFileLimit (path, 0, MAX_SOUNDFILE, &size, hFile);
	if (data)
	{
		if (size > 0 && size <= MAX_SOUNDFILE)
			S_LoadDSoundBuffer (sfx, data, size);

		CL_PollProgressBar ();
	}

	COM_FreeFile ();
}

/*
==================
S_BeginLevelLoad

Open a new precache generation. Sounds the incoming server does not ask for
this level are left behind by their server count and reclaimed later.
==================
*/
void S_BeginLevelLoad (void)
{
	s_servercount++;
}

/*
==================
S_FreeUnusedSounds

Anything the current server did not ask for goes away.
==================
*/
void S_FreeUnusedSounds (void)
{
	int	i;

	if (!lpDS)
		return;

	for (i = 0 ; i < MAX_SFX ; i++)
	{
		if (!known_sfx[i])
			continue;
		if ((unsigned short)known_sfx[i]->servercount == (unsigned)s_servercount)
			continue;
		if (known_sfx[i]->flags & SFX_KEEP)
			continue;

		if (known_sfx[i]->buffer)
		{
			known_sfx[i]->buffer->Release ();
			known_sfx[i]->buffer = NULL;
		}

		MnemoFree (known_sfx[i]);
		known_sfx[i] = NULL;
	}
}

/*
==================
S_LoadDSoundBuffer

Walk the wav chunks, then build a hardware buffer holding the samples. A smpl
chunk means the sound loops.
==================
*/
void S_LoadDSoundBuffer (sfx_t *sfx, byte *wav, int size)
{
	int		*fmt;
	byte		*chunk;
	int		id;
	unsigned	len;
	int		riffend, dataofs;
	unsigned	datasize;
	qboolean	looping;
	DSBUFFERDESC	desc;
	void		*ptr1, *ptr2;
	DWORD		bytes1, bytes2;
	int		*in, *out;
	unsigned	words, left;
	HRESULT		hr;

	if (!lpDS)
		return;
	if (((int *)wav)[0] != MAKEFOURCC('R','I','F','F'))
		return;
	if (((int *)wav)[2] != MAKEFOURCC('W','A','V','E'))
		return;

	riffend = ((int *)wav)[1] + 12;
	chunk = wav + 12;

	fmt = NULL;
	dataofs = 0;
	datasize = 0;
	looping = false;

	while (chunk - wav < riffend)
	{
		// chunks are only word aligned, so the header comes out a byte at a time
		memcpy (&id, chunk, 4);
		chunk += 4;
		memcpy (&len, chunk, 4);
		chunk += 4;

		if (id == MAKEFOURCC('f','m','t',' '))
		{
			fmt = (int *)chunk;
		}
		else if (id == MAKEFOURCC('d','a','t','a'))
		{
			dataofs = chunk - wav;
			datasize = len;
			if (datasize > (unsigned)MAX_SOUNDDATA)
				return;
		}
		else if (id == MAKEFOURCC('s','m','p','l'))
		{
			len = 4;
			looping = true;
		}

		if (len > (unsigned)riffend)
			len = riffend;

		chunk += len;
		if (len & 1)
			chunk++;
	}

	memset (&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = datasize;
	desc.lpwfxFormat = (WAVEFORMATEX *)fmt;

	if (sfx->buffer)
	{
		sfx->buffer->Release ();
		sfx->buffer = NULL;
	}

	hr = lpDS->CreateSoundBuffer (&desc, &sfx->buffer, NULL);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		return;
	}

	ptr1 = ptr2 = NULL;
	bytes1 = bytes2 = 0;

	hr = sfx->buffer->Lock (0, 0, &ptr1, &bytes1, &ptr2, &bytes2, DSBLOCK_ENTIREBUFFER);
	if (hr != DS_OK)
		S_DSoundError (hr);

	if (ptr1 && bytes1)
	{
		// audio RAM only takes whole words, and the buffer the driver
		// gave us may be longer than the data chunk
		in = (int *)(wav + dataofs);
		out = (int *)ptr1;

		words = datasize >> 2;
		left = bytes1 >> 2;

		while (words && left)
		{
			*out++ = *in++;
			words--;
			left--;
		}

		while (left--)
			*out++ = 0;

		if (bytes1 & 3)
			*out = 0;
	}

	hr = sfx->buffer->Unlock (ptr1, bytes1, ptr2, bytes2);
	if (hr != DS_OK)
	{
		S_DSoundError (hr);
		return;
	}

	if (looping)
		sfx->flags |= SFX_LOOPING;
}

/*
==================
SV_LookupSoundIndex
==================
*/
int SV_LookupSoundIndex (char *name)
{
	if (!g_pAudioMgr)
		return -1;

	return S_AllocSfx (name);
}

/*
==================
S_GetSfxByIndex
==================
*/
sfx_t *S_GetSfxByIndex (int index)
{
	if (!g_pAudioMgr)
		return NULL;

	if (index < 0 || index >= MAX_SFX)
		return NULL;

	return known_sfx[index];
}

/*
==================
S_SoundInfo
==================
*/
void S_SoundInfo (void)
{
	int	i, total, playing;
	DWORD	status;
	HRESULT	hr;

	total = 0;
	playing = 0;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		if (!g_audio[i])
			continue;

		total++;

		if (!g_audio[i]->m_pBuffer)
			continue;

		status = 0;
		hr = g_audio[i]->m_pBuffer->GetStatus (&status);
		if (hr != DS_OK)
			S_DSoundError (hr);

		if (status & DSBSTATUS_PLAYING)
			playing++;
	}

	Con_Printf ("%d sounds, %d playing\n", total, playing);
}

/*
==================
S_PrintSoundInfo
==================
*/
void S_PrintSoundInfo (void)
{
	int	i;
	int	sfxcount, cached, ram, spuram;
	CAudio	*audio;
	DWORD	status;
	DSBCAPS	caps;
	DSCAPS	dscaps;
	HRESULT	hr;

	for (i = 0 ; i < MAX_AUDIO ; i++)
	{
		audio = g_audio[i];
		if (!audio)
			continue;

		Con_Printf ("Audio %d: %s", i, audio->m_name);
		Con_Printf (", %d db", audio->m_dsvolume / 100);

		if (audio->m_pitch != 100)
			Con_Printf (", %d pitch", audio->m_pitch);

		if (audio->m_spatial)
			Con_Printf (", %d/%d ent/chan", audio->m_entnum, audio->m_entchannel);

		if (audio->m_loop)
			Con_Printf (", Loop");

		if (audio->m_stream)
			Con_Printf (", Stream");

		if (audio->m_stopping)
			Con_Printf (", Stopping");

		if (audio->m_pBuffer)
		{
			status = 0;
			hr = audio->m_pBuffer->GetStatus (&status);
			if (hr != DS_OK)
				S_DSoundError (hr);

			if (!(status & DSBSTATUS_PLAYING))
				Con_Printf (", NOT Playing");
		}

		Con_Printf ("\n");
	}

	sfxcount = 0;
	cached = 0;
	ram = 0;
	spuram = 0;

	for (i = 0 ; i < MAX_SFX ; i++)
	{
		if (!known_sfx[i])
			continue;

		sfxcount++;

		if (!known_sfx[i]->buffer)
			continue;

		cached++;

		hr = known_sfx[i]->buffer->GetCaps (&caps);
		if (hr != DS_OK)
			S_DSoundError (hr);

		if (caps.dwFlags & DSBCAPS_LOCSOFTWARE)
			ram += caps.dwBufferBytes;
		else
			spuram += caps.dwBufferBytes;
	}

	hr = lpDS->GetCaps (&dscaps);
	if (hr != DS_OK)
		S_DSoundError (hr);

	Con_Printf ("%d SFX_T, %d cached\n", sfxcount, cached);
	Con_Printf ("%d RAM, %d SPU RAM, %d Free, %d A-File\n",
		ram, spuram, dscaps.dwFreeHwMemBytes, AFile_TotalCachedBytes ());
}

/*
==================
S_GetSoundRam
==================
*/
void S_GetSoundRam (int *ram, int *spuram, int *freeram, int *afile)
{
	int	i, sw, hw;
	DSBCAPS	caps;
	DSCAPS	dscaps;
	HRESULT	hr;

	sw = 0;
	hw = 0;

	for (i = 0 ; i < MAX_SFX ; i++)
	{
		if (!known_sfx[i] || !known_sfx[i]->buffer)
			continue;

		hr = known_sfx[i]->buffer->GetCaps (&caps);
		if (hr != DS_OK)
			S_DSoundError (hr);

		if (caps.dwFlags & DSBCAPS_LOCSOFTWARE)
			hw += caps.dwBufferBytes;
		else
			sw += caps.dwBufferBytes;
	}

	hr = lpDS->GetCaps (&dscaps);
	if (hr != DS_OK)
		S_DSoundError (hr);

	*ram = sw;
	*spuram = hw;
	*freeram = dscaps.dwFreeHwMemBytes;
	*afile = AFile_TotalCachedBytes ();
}

/*
==================
S_SoundList

Print count sounds starting at start, wrapping round the table. Returns where
to pick up next time.
==================
*/
int S_SoundList (int start, int count)
{
	DSBCAPS	caps;
	HRESULT	hr;

	while (count)
	{
		if (known_sfx[start])
		{
			if (!known_sfx[start]->buffer)
			{
				Con_Printf ("S #%d: %s - not loaded\n", start, known_sfx[start]->name);
			}
			else
			{
				hr = known_sfx[start]->buffer->GetCaps (&caps);
				if (hr != DS_OK)
					S_DSoundError (hr);

				if (caps.dwFlags & DSBCAPS_LOCSOFTWARE)
					Con_Printf ("S #%d: %s - SW:%d\n", start,
						known_sfx[start]->name, caps.dwBufferBytes);
				else
					Con_Printf ("S #%d: %s - HW:%d\n", start,
						known_sfx[start]->name, caps.dwBufferBytes);
			}

			count--;
		}

		start++;
		if (start >= MAX_SFX)
			start = 0;
	}

	return start;
}

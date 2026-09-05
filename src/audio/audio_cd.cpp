// audio_cd.cpp -- music tracks played off the disc
//
// The music is just another stream: a track is opened as !MUSn and refilled by
// the streaming code like any other long sound. Only one is ever playing, so
// the object is a singleton that the play commands create on demand.

#include "quakedef.h"
#include "audio.h"
#include "audio_stream.h"
#include "audio_cd.h"

cvar_t		bgmvolume = {"bgmvolume", "1", FCVAR_ARCHIVE};

CAudioCD	*cdaudio;

static byte		remap[MAX_CDTRACKS];
static byte		cdenabled = true;
static byte		cdfirsttime = true;

/*
==================
CAudioCD::CAudioCD
==================
*/
CAudioCD::CAudioCD () : CAudioStream (STREAM_BUFFERSIZE)
{
	m_track = 0;
	m_lastvolume = -1.0f;

	// first one created owns the music
	if (!cdaudio)
		cdaudio = this;
}

/*
==================
CAudioCD::~CAudioCD
==================
*/
CAudioCD::~CAudioCD ()
{
	if (cdaudio == this)
		cdaudio = NULL;
}

void CDAudio_Shutdown (void)
{
}

/*
==================
CDAudio_Init

Tracks start out mapped straight through. The command and the volume cvar are
only registered the first time around, since a "cd reset" comes back here.
==================
*/
void CDAudio_Init (void)
{
	int	i;

	for (i = 0 ; i < MAX_CDTRACKS ; i++)
		remap[i] = i;

	if (cdfirsttime)
	{
		Cmd_AddCommand ("cd", Cmd_cd_f);
		Cvar_RegisterVariable (&bgmvolume);
	}

	cdfirsttime = false;
}

/*
==================
CDAudio_Pause
==================
*/
void CDAudio_Pause (qboolean enable)
{
	cdenabled = enable;

	if (!enable && cdaudio)
		cdaudio->Stop ();
}

/*
==================
CAudioCD::Stop
==================
*/
void CAudioCD::Stop (void)
{
	if (cdaudio == this)
		cdaudio = NULL;

	CAudioStream::Stop ();
}

/*
==================
CAudioCD::Close
==================
*/
void CAudioCD::Close (void)
{
}

/*
==================
CDAudio_Play

Play a track, remapping it first. Tracks 0 and 1 are the data track and the
sound effects, so anything below 2 is silence.
==================
*/
void CDAudio_Play (CAudioCD *cd, int track, qboolean looping)
{
	char	name[16];

	for (;;)
	{
		track = remap[track];
		if (track < 2)
			return;

		if (!cd->m_track || cd->m_track == track)
			break;

		cd->Stop ();
		new CAudioCD;
		cd = cdaudio;
	}

	cd->m_track = track;
	cd->m_loop = looping;

	sprintf (name, "!MUS%d", cd->m_track - 1);
	VOX_LoadSound (cd, name);
}

/*
==================
CAudioCD::Update
==================
*/
void CAudioCD::Update (float frametime, qboolean quiet)
{
	if (!cdenabled)
		Stop ();
	else
		CAudioStream::Update (frametime, quiet);
}

/*
==================
CAudioCD::UpdateVolume

Music rides on bgmvolume alone -- it is never attenuated or panned.
==================
*/
void CAudioCD::UpdateVolume (void)
{
	int	vol;
	HRESULT	hr;

	if (!m_pBuffer)
		return;

	if (m_lastvolume == bgmvolume.value)
		return;

	vol = DSBVOLUME_MIN;
	if (bgmvolume.value > 0)
		vol = (int)(log10(bgmvolume.value) * 10 * 100);

	if (vol > DSBVOLUME_MAX)
		vol = DSBVOLUME_MAX;
	if (vol < DSBVOLUME_MIN)
		vol = DSBVOLUME_MIN;

	hr = m_pBuffer->SetVolume (vol);
	if (hr != DS_OK)
		Stop ();

	hr = m_pBuffer->SetPan (0);
	if (hr != DS_OK)
		Stop ();

	m_lastvolume = bgmvolume.value;
	m_dsvolume = vol;
	m_dspan = 0;
}

/*
==================
CAudioCD::Restart

Sound has come back to us -- pick the track up where it left off.
==================
*/
void CAudioCD::Restart (void)
{
	int	track;

	Stop ();

	track = (byte)m_track;

	if (!cdaudio)
		new CAudioCD;

	CDAudio_Play (cdaudio, track, true);
}

/*
==================
CDAudio_PlayTrack
==================
*/
void CDAudio_PlayTrack (int track, qboolean looping)
{
	if (!cdaudio)
		new CAudioCD;

	CDAudio_Play (cdaudio, track, looping);
}

/*
==================
Cmd_cd_f
==================
*/
void Cmd_cd_f (void)
{
	char	*command;
	int	i;

	if (Cmd_Argc () < 2)
		return;

	command = Cmd_Argv (1);

	if (!Q_stricmp (command, "play"))
	{
		if (!cdaudio)
			new CAudioCD;
		CDAudio_Play (cdaudio, atoi(Cmd_Argv(2)), false);
		return;
	}

	if (!Q_stricmp (command, "loop"))
	{
		if (!cdaudio)
			new CAudioCD;
		CDAudio_Play (cdaudio, atoi(Cmd_Argv(2)), true);
		return;
	}

	if (!Q_stricmp (command, "stop"))
	{
		if (cdaudio)
			cdaudio->Stop ();
		return;
	}

	if (!Q_stricmp (command, "pause"))
	{
		if (cdaudio)
			cdaudio->Stop ();
		return;
	}

	if (!Q_stricmp (command, "resume"))
	{
		if (cdaudio)
			cdaudio->Restore ();
		return;
	}

	if (!Q_stricmp (command, "on"))
	{
		cdenabled = true;
		return;
	}

	if (!Q_stricmp (command, "off"))
	{
		cdenabled = false;
		if (cdaudio)
			cdaudio->Stop ();
		return;
	}

	if (!Q_stricmp (command, "reset"))
	{
		if (cdaudio)
			cdaudio->Stop ();

		cdenabled = true;

		for (i = 0 ; i < MAX_CDTRACKS ; i++)
			remap[i] = i;

		if (cdfirsttime)
		{
			Cmd_AddCommand ("cd", Cmd_cd_f);
			Cvar_RegisterVariable (&bgmvolume);
		}

		cdfirsttime = false;
		return;
	}
}

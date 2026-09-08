// snd_null.cpp -- the DirectSound device and the engine's sound entry points
//
// Everything the rest of the engine calls to make a noise lands here. This is
// also where the device itself is created and torn down: the DirectSound
// object, the primary buffer that keeps the mixer running, and the listener
// position the spatialisation reads.

#include "quakedef.h"
#include "audio.h"
#include "afile.h"
#include "audio_mgr.h"
#include "audio_static.h"
#include "audio_stream.h"
#include "audio_cd.h"

// The engine-facing sound layer is built unoptimized and never inlined. It is
// pure dispatch into the audio objects -- there is nothing here worth a cycle,
// and keeping every entry point a real call makes it steppable when a sound
// goes wrong out on the hardware.
#pragma optimize( "", off )
#pragma inline_depth( 0 )

cvar_t room_type = { "room_type", "0" };
cvar_t waterroom_type = { "waterroom_type", "14" };
cvar_t room_off = { "room_off", "0" };

void SX_Init (void)
{
	Cvar_RegisterVariable (&room_type);
	Cvar_RegisterVariable (&waterroom_type);
	Cvar_RegisterVariable (&room_off);
}

char *VOX_GetDirectory (char *szpath, char *psz)
{
	char c;
	int cb = 0;
	char *pszscan = psz + Q_strlen (psz) - 1;

	c = *pszscan;
	while (pszscan > psz && c != '/')
	{
		c = *(--pszscan);
		cb++;
	}

	if (c != '/')
	{
		Q_strcpy (szpath, "vox/");
		return psz;
	}

	cb = Q_strlen (psz) - cb;
	Q_memcpy (szpath, psz, cb);
	szpath[cb] = 0;
	return pszscan + 1;
}

extern "C" HWND	g_hWnd;
extern int	fSentencesInit;

// Sounds started from the console are not attached to anything in the world,
// so they play against an entity number nothing else can claim.
#define SND_CONSOLE_ENT		10000

LPDIRECTSOUND		lpDS;
LPDIRECTSOUNDBUFFER	lpDSPBuf;

vec3_t	listener_origin;
vec3_t	listener_forward;
vec3_t	listener_right;
vec3_t	listener_up;

static qboolean	snd_registered = true;
static byte		snd_fromserver = true;
static float	snd_refdist = 1000.0f;
static int	soundlist_cursor;

static char	*sentence_data;
char	*rgpszrawsentence[CVOXFILESENTENCEMAX];
static int	sentence_count;
static char	sentence_path[MAX_QPATH];

/*
==================
SENTENCEG_Init

Read the sentence list. Each line is a name followed by the text of the
sentence; a line starting with '/' is a comment. The buffer is kept and the
names point into it, with the separating space turned into a terminator.
==================
*/
void SENTENCEG_Init (void)
{
	char	*p, *end;
	int	count, size;
	FILE	*file;

	size = 0;

	if (sentence_data)
	{
		COM_FreeFile (sentence_data);
		sentence_data = NULL;
	}

	Q_memset (rgpszrawsentence, 0, sizeof(rgpszrawsentence));

	// a map pack can drop in its own list
	if (COM_FOpenFile ("sound/new_sentences.txt", &file) == -1)
		strcpy (sentence_path, "sound/sentences.txt");
	else
	{
		Sys_CloseHandle (file);
		strcpy (sentence_path, "sound/new_sentences.txt");
	}

	p = (char *)COM_LoadFile (sentence_path, 5, &size);
	sentence_data = p;
	if (!p)
		return;

	end = p + size;
	count = 0;

	while (count < CVOXFILESENTENCEMAX && p < end)
	{
		while (p < end && (*p == '\n' || *p == '\r' || *p == '\t' || *p == ' '))
			p++;

		if (*p != '/')
		{
			rgpszrawsentence[count++] = p;

			while (p < end && *p != ' ')
				p++;

			if (p < end)
				*p++ = 0;
		}

		if (p < end && *p != '\n')
		{
			while (*p != '\r')
			{
				p++;
				if (p >= end || *p == '\n')
					break;
			}
		}

		if (p < end)
			*p++ = 0;
	}

	sentence_count = count;
}

/*
==================
S_FindNameByIndex

Look a sentence up by name. Returns the text that follows it, and the index it
was found at.
==================
*/
char *S_FindNameByIndex (char *name, int *index)
{
	int	i;

	for (i = 0 ; i < sentence_count ; i++)
	{
		if (Q_strcasecmp (name, rgpszrawsentence[i]))
			continue;

		if (index)
			*index = i;

		return rgpszrawsentence[i] + Q_strlen (rgpszrawsentence[i]) + 1;
	}

	return NULL;
}

/*
==================
S_Init

Come up on the device, then hand everything else to the audio manager. Called
again on a restart, so only the commands that have never been registered are
added.
==================
*/

void	Cmd_play_f (void);
void	Cmd_playvol_f (void);
void	Cmd_stopsound_f (void);
void	Cmd_soundlist_f (void);
void	Cmd_soundinfo_f (void);
void	Cmd_bunny_f (void);

void S_Init (void)
{
	DSBUFFERDESC	desc;
	HRESULT		hr;

	fSentencesInit = FALSE;

	if (!g_pAudioMgr)
	{
		hr = DirectSoundCreate (NULL, &lpDS, NULL);
		if (hr != DS_OK)
			S_DSoundError (hr);

		hr = lpDS->SetCooperativeLevel (g_hWnd, DSSCL_EXCLUSIVE);
		if (hr != DS_OK)
			S_DSoundError (hr);

		// the primary buffer never carries samples of its own -- it is
		// what keeps the hardware mixer turning over
		memset (&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
		desc.dwBufferBytes = 0;
		desc.lpwfxFormat = NULL;

		hr = lpDS->CreateSoundBuffer (&desc, &lpDSPBuf, NULL);
		if (hr != DS_OK)
			S_DSoundError (hr);

		hr = lpDSPBuf->Play (0, 0, DSBPLAY_LOOPING);
		if (hr != DS_OK)
			S_DSoundError (hr);

		S_InitSoundCache ();
	}

	S_StopAllSounds (true);
	S_ClearBuffer (true);
	SENTENCEG_Init ();

	if (snd_registered)
	{
		snd_registered = false;

		Cmd_AddCommand ("play", Cmd_play_f);
		Cmd_AddCommand ("playvol", Cmd_playvol_f);
		Cmd_AddCommand ("stopsound", Cmd_stopsound_f);
		Cmd_AddCommand ("soundlist", Cmd_soundlist_f);
		Cmd_AddCommand ("soundinfo", Cmd_soundinfo_f);
		Cmd_AddCommand ("afilelist", Cmd_afilelist_f);
		Cmd_AddCommand ("bunny", Cmd_bunny_f);
	}
}

/*
==================
S_ShutdownDevice
==================
*/
void S_ShutdownDevice (void)
{
	HRESULT	hr;

	S_Shutdown ();

	if (lpDSPBuf)
	{
		hr = lpDSPBuf->Release ();
		if (hr != DS_OK)
			S_DSoundError (hr);

		lpDSPBuf = NULL;
	}

	if (lpDS)
	{
		hr = lpDS->Release ();
		if (hr != DS_OK)
			S_DSoundError (hr);

		lpDS = NULL;
	}
}

/*
==================
S_EndPrecaching

Bracket the server's resource list with this. Those sounds are tracked by their
server count, so they must not be marked permanent.
==================
*/
void S_EndPrecaching (void)
{
	snd_fromserver = false;
}

/*
==================
S_BeginPrecaching

Anything looked up outside the resource list is the engine's own -- the temp
entity sounds, the console commands -- and is kept for the life of the game.
==================
*/
void S_BeginPrecaching (void)
{
	snd_fromserver = true;
}

/*
==================
S_FindName
==================
*/
sfx_t *S_FindName (char *name)
{
	if (!g_pAudioMgr)
		return NULL;

	return S_PrecacheSound (g_pAudioMgr, name, snd_fromserver);
}

/*
==================
S_Update

Move the listener, then let every object catch up.
==================
*/
void S_Update (vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	VectorCopy (origin, listener_origin);
	VectorCopy (forward, listener_forward);
	VectorCopy (right, listener_right);
	VectorCopy (up, listener_up);

	if (g_pAudioMgr)
		g_pAudioMgr->Update (false);
}

/*
==================
S_ExtraUpdate

Called from the middle of a long frame to keep the streams fed. The world has
not moved, so nothing is repositioned.
==================
*/
void S_ExtraUpdate (void)
{
	if (g_pAudioMgr)
		g_pAudioMgr->Update (true);
}

/*
==================
S_StopSound
==================
*/
void S_StopSound (int entnum, int entchannel)
{
	CAudio	*ch;

	while ((ch = S_PickChannel (entnum, entchannel)) != NULL)
		ch->Stop ();
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds (qboolean clear)
{
	if (g_pAudioMgr)
		g_pAudioMgr->StopAll (false);
}

/*
==================
S_CloseAllSounds

Let go of every hardware buffer without destroying the objects, so the device
can be handed to something else.
==================
*/
void S_CloseAllSounds (void)
{
	if (g_pAudioMgr)
		g_pAudioMgr->StopAll (true);
}

/*
==================
S_TouchSound

The server precaches by name; nothing has to be pulled in here because the
sound is loaded the first time it is actually played.
==================
*/
void S_TouchSound (char *name)
{
}

/*
==================
S_ClearBuffer
==================
*/
void S_ClearBuffer (qboolean clear)
{
	if (g_pAudioMgr)
		S_BeginLevelLoad ();
}

/*
==================
S_BlockSound
==================
*/
void S_BlockSound (void)
{
	if (g_pAudioMgr)
		g_pAudioMgr->Block (true);
}

/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound (void)
{
	if (g_pAudioMgr)
		g_pAudioMgr->Block (false);
}

/*
==================
S_StartStaticSound
==================
*/
void S_StartStaticSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin,
	float volume, float attenuation, int flags, int pitch)
{
	S_StartDynamicSound (entnum, entchannel, sfx, origin, volume, attenuation, flags, pitch);
}

/*
==================
S_StartDynamicSound

The one way into the sound system. A sound that is already cached becomes a
static object; anything else is streamed. Sentences always stream.
==================
*/
void S_StartDynamicSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin,
	float volume, float attenuation, int flags, int pitch)
{
	CAudio		*ch;
	CAudioStatic	*st;
	CAudioStream	*stream;
	sfx_t		*cached;

	if (!sfx || !g_pAudioMgr)
		return;

	// an update to something already playing rather than a new sound
	if ((flags & SND_STOP) || (flags & SND_CHANGE_VOL) || (flags & SND_CHANGE_PITCH))
	{
		ch = S_PickChannel (entnum, entchannel);
		if (ch)
		{
			if (flags & SND_CHANGE_VOL)
				ch->SetVolume (volume);

			if (flags & SND_CHANGE_PITCH)
				ch->SetPitch (pitch);

			if (flags & SND_STOP)
				ch->Stop ();
		}

		return;
	}

	// a '*' sound is a stream that must not be started twice
	if (sfx->name[0] == '*')
	{
		if (S_FindChannelForName (sfx->name))
			return;

		entchannel = CHAN_STREAM;
	}

	// speech and streams are mutually exclusive on an entity: a voice line
	// never displaces a stream, and a stream shuts any voice line up
	if (entchannel == CHAN_VOICE && S_PickChannel (entnum, CHAN_STREAM))
		return;

	if (entchannel == CHAN_STREAM)
	{
		ch = S_PickChannel (entnum, CHAN_VOICE);
		if (ch)
			ch->Stop ();
	}

	// the player's own sounds are allowed to overlap
	if (entnum != cl.viewentity)
	{
		ch = S_PickChannel (entnum, entchannel);
		if (ch)
			ch->Stop ();
	}

	if (!Q_stricmp (sfx->name, "common/null.wav"))
		return;

	if (sfx->buffer)
	{
		st = new CAudioStatic;
		st->SetSpatial (attenuation / snd_refdist, volume, origin, entnum, entchannel, pitch);
		st->Open (sfx);
		return;
	}

	if (sfx->name[0] == '!')
	{
		stream = new CAudioStream (STREAM_BUFFERSIZE);
		stream->SetSpatial (attenuation / snd_refdist, volume, origin, entnum, entchannel, pitch);
		VOX_LoadSound (stream, sfx->name);
		return;
	}

	cached = S_FindNameNoCreate (sfx->name);
	if (cached && cached->buffer)
	{
		st = new CAudioStatic;
		st->SetSpatial (attenuation / snd_refdist, volume, origin, entnum, entchannel, pitch);
		st->Open (cached);
		return;
	}

	stream = new CAudioStream (STREAM_BUFFERSIZE);
	stream->SetSpatial (attenuation / snd_refdist, volume, origin, entnum, entchannel, pitch);
	VOX_LoadFile (stream, sfx->name);
}

/*
==================
Cmd_play_f
==================
*/
void Cmd_play_f (void)
{
	char	name[MAX_CONSOLE_SOUND_NAME];
	int	i;
	sfx_t	*sfx;

	for (i = 1 ; i < Cmd_Argc () ; i++)
	{
		Q_strcpy (name, Cmd_Argv (i));

		if (name[0] != '!' && !Q_strrchr (Cmd_Argv (i), '.'))
			Q_strcat (name, ".wav");

		sfx = S_FindName (name);

		S_StartDynamicSound (SND_CONSOLE_ENT, CHAN_AUTO, sfx, listener_origin,
			1.0f, 1.0f, 0, PITCH_NORM);
	}
}

/*
==================
Cmd_playvol_f
==================
*/
void Cmd_playvol_f (void)
{
	char	name[MAX_CONSOLE_SOUND_NAME];
	int	i;
	float	vol;
	sfx_t	*sfx;

	for (i = 1 ; i < Cmd_Argc () ; i += 2)
	{
		Q_strcpy (name, Cmd_Argv (i));

		if (name[0] != '!' && !Q_strrchr (Cmd_Argv (i), '.'))
			Q_strcat (name, ".wav");

		sfx = S_FindName (name);
		vol = Q_atof (Cmd_Argv (i + 1));

		S_StartDynamicSound (SND_CONSOLE_ENT, CHAN_AUTO, sfx, listener_origin,
			vol, 1.0f, 0, PITCH_NORM);
	}
}

/*
==================
Cmd_stopsound_f
==================
*/
void Cmd_stopsound_f (void)
{
	S_StopAllSounds (true);
}

/*
==================
Cmd_soundinfo_f
==================
*/
void Cmd_soundinfo_f (void)
{
	if (g_pAudioMgr)
		S_PrintSoundInfo ();
}

/*
==================
Cmd_soundlist_f

Pages through the cache, four at a time unless told otherwise.
==================
*/
void Cmd_soundlist_f (void)
{
	int	count;

	count = 4;
	if (Cmd_Argc () == 2)
		count = atoi (Cmd_Argv (1));

	if (g_pAudioMgr)
		soundlist_cursor = S_SoundList (soundlist_cursor, count);
}

/*
==================
S_GetDSPInfo
==================
*/
void S_GetDSPInfo (int *ram, int *spuram, int *freeram, int *afile)
{
	*ram = 0;
	*spuram = 0;
	*freeram = 0;
	*afile = 0;

	if (g_pAudioMgr)
		S_GetSoundRam (ram, spuram, freeram, afile);
}

/*
==================
Cmd_bunny_f
In memory of Betty Cunningham (1962 - 2000)
==================
*/
void Cmd_bunny_f (void)
{
	Con_Printf ("I miss ya, Betty. -Robert\n");
}

/*
==================
S_DSoundError
==================
*/
char *S_DSoundError (HRESULT hr)
{
	char	*name;

	switch (hr)
	{
	case DS_OK:				name = "DS_OK"; break;
	case DSERR_ALLOCATED:			name = "DSERR_ALLOCATED"; break;
	case DSERR_CONTROLUNAVAIL:		name = "DSERR_CONTROLUNAVAIL"; break;
	case DSERR_INVALIDPARAM:		name = "DSERR_INVALIDPARAM"; break;
	case DSERR_INVALIDCALL:			name = "DSERR_INVALIDCALL"; break;
	case DSERR_GENERIC:			name = "DSERR_GENERIC"; break;
	case DSERR_PRIOLEVELNEEDED:		name = "DSERR_PRIOLEVELNEEDED"; break;
	case DSERR_OUTOFMEMORY:			name = "DSERR_OUTOFMEMORY"; break;
	case DSERR_BADFORMAT:			name = "DSERR_BADFORMAT"; break;
	case DSERR_NODRIVER:			name = "DSERR_NODRIVER"; break;
	case DSERR_ALREADYINITIALIZED:		name = "DSERR_ALREADYINITIALIZED"; break;
	case DSERR_NOAGGREGATION:		name = "DSERR_NOAGGREGATION"; break;
	case DSERR_BUFFERLOST:			name = "DSERR_BUFFERLOST"; break;
	case DSERR_OTHERAPPHASPRIO:		name = "DSERR_OTHERAPPHASPRIO"; break;
	case DSERR_UNINITIALIZED:		name = "DSERR_UNINITIALIZED"; break;
	case DSERR_NOINTERFACE:			name = "DSERR_NOINTERFACE"; break;
	case DSERR_NOT32BYTEALIGNED:		name = "DSERR_NOT32BYTEALIGNED"; break;
	case DSERR_UNSUPPORTED:			name = "DSERR_UNSUPPORTED"; break;
	default:				name = "<UNKNOWN>"; break;
	}

	Con_DPrintf ("DirectSound Error (%x): %s\n", hr, name);

	return name;
}

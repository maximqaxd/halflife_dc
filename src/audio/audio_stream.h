// audio_stream.h -- long sounds played straight off the disc

#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "audio.h"
#include "audio_mgr.h"

// Ring buffer a stream plays out of. Half of it is refilled at a time while
// the other half plays.
#define STREAM_BUFFERSIZE	32768
#define MAX_AUDIO_STREAM_PATH	48

// m_state
#define STREAM_OPEN		0		// fresh object, nothing to play yet
#define STREAM_ALLOC		1		// need a play buffer
#define STREAM_DONE		2		// finished, let it go
#define STREAM_IDLE		-262	// nothing to play; sit still rather than spin
#define STREAM_OPENING		-274	// waiting for the read to land
#define STREAM_HEADER		-282	// read the wav header
#define STREAM_START		-289	// hand the buffer to the driver
#define STREAM_PLAYING		-295	// running

// How long an idle stream waits before looking at itself again.
#define STREAM_IDLE_TIME	3600.0f

class CAudioStream : public CAudio
{
public:
	CAudioStream (int buffersize);
	virtual ~CAudioStream ();

	virtual void	Stop (void);
	virtual void	Suspend (void);
	virtual void	Restore (void);
	virtual void	Update (float frametime, qboolean quiet);
	virtual void	Restart (void);

	qboolean	AllocBuffer (void);
	qboolean	ParseWavHeader (void);
	qboolean	StartPlayback (void);
	void		Service (void);
	qboolean	LockAndCopy (int half, int leadin, int fadeout);
	qboolean	LockCopyTail (int offset, int bytes);
	void		ReadAhead (int offset);
	void		CheckOpen (void);
	qboolean	PlayFromOffset (void);
	void		FreeMouth (void);
	void		FreeBuffer (void);

	int		m_state;
	float		m_timeleft;
	float		m_elapsed;

	int		m_buffersize;
	int		m_halfbuffer;

	int		m_file[3];		// Open file record.
	OVERLAPPED	m_async;		// Read in flight.

	char		m_path[MAX_AUDIO_STREAM_PATH];

	int		m_filesize;		// Data chunk offset plus its length.
	int		m_readpos;		// How far into the file we have read.
	int		m_writepos;
	int		m_dataofs;		// Data chunk offset inside m_filedata.
	byte		*m_filedata;

	byte		m_suspended;		// Resume playback when sound is restored.
	int		m_playpos;
	byte		m_fill;
	int		m_pending;

	byte		m_ready;		// The read we started has landed.
	byte		m_trystatic;		// Small enough to cache whole.

	byte		*m_mouth;		// MOTH chunk, drives the lip sync.
	int		m_mouthsize;
};

// The wav carries a MOTH chunk of per-sample mouth openings alongside the
// audio, so talking models move their jaw in step with the sound.
#define MOUTH_MAX		255

qboolean	VOX_LoadSound (CAudioStream *stream, char *name);
qboolean	VOX_LoadFile (CAudioStream *stream, char *name);

void		SND_InitMouth (void);
void		SND_ShutdownMouth (void);
void		SND_MoveMouth (CAudioStream *stream, float frametime);

void		SENTENCEG_Init (void);
char		*S_FindNameByIndex (char *name, int *index);

void		Cmd_speak_f (void);

extern cvar_t	mouthdelay;
extern cvar_t	mouthrate;
extern cvar_t	mouthscale;

#endif // AUDIO_STREAM_H

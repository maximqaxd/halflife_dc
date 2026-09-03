// audio_stream.h -- long sounds played straight off the disc

#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "audio.h"
#include "audio_mgr.h"

// Ring buffer a stream plays out of. Half of it is refilled at a time while
// the other half plays.
#define STREAM_BUFFERSIZE	32768

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
	qboolean	LockCopyTail (int offset);
	void		ReadAhead (int offset);

	int		m_state;		// 0x80
	float		m_timeleft;		// 0x84
	float		m_elapsed;		// 0x88

	int		m_buffersize;		// 0x8c
	int		m_halfbuffer;		// 0x90

	int		m_file[3];		// 0x94  open file record
	OVERLAPPED	m_async;		// 0xa0  read in flight

	char		m_path[48];		// 0xb4

	int		m_filesize;		// 0xe4  data chunk offset + its length
	int		m_readpos;		// 0xe8  how far into the file we have read
	int		m_writepos;		// 0xec
	int		m_dataofs;		// 0xf0  data chunk offset inside m_filedata
	byte		*m_filedata;		// 0xf4

	byte		m_half;			// 0xf8  half of the ring being filled
	int		m_playpos;		// 0xfc
	byte		m_fill;			// 0x100
	int		m_pending;		// 0x104

	byte		m_ready;		// 0x108  the read we started has landed
	byte		m_trystatic;		// 0x109  small enough to cache whole

	byte		*m_mouth;		// 0x10c  MOTH chunk, drives the lip sync
	int		m_mouthsize;		// 0x110
};

typedef char CAudioStream_must_match_retail_size[
	(sizeof(CAudioStream) == 0x114) ? 1 : -1];

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

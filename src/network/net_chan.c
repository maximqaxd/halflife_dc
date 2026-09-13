#include "quakedef.h"

#ifdef _WIN32
#include "winquake.h"
#endif

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16  qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are allways placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.


*/

int		net_drop;
float	net_rate;
cvar_t	showpackets = { "showpackets", "0" };
cvar_t	showdrop = { "showdrop", "0" };
cvar_t	r_netgraph = { "r_netgraph", "0" };
cvar_t	net_chokeloopback = { "netchokeloop", "0" };

/*
===============
Net_Rate_f

Calculates the amount of bytes per second max that the server can send you data
===============
*/
void Net_Rate_f( void )
{
	float	fNewRate;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("Usage:  rate <num>\nTransmission rate (bytes/sec)\nCurrent:  %.5f\n", net_rate);
		return;
	}

	fNewRate = (float)atof(Cmd_Argv(1));
	if (fNewRate == 0.0f)
	{
		fNewRate = DEFAULT_RATE;
	}

	if (fNewRate < MIN_RATE || fNewRate > MAX_RATE)
	{
		Con_Printf("Rate:  Maximum %f, Minimum %f\n", MAX_RATE, MIN_RATE);
		return;
	}

	net_rate = fNewRate;
	cls.netchan.rate = fNewRate;
}

/*
===============
Netchan_Init

===============
*/
void Netchan_Init( void )
{
	Cvar_RegisterVariable(&showpackets);
	Cvar_RegisterVariable(&showdrop);
	Cvar_RegisterVariable(&r_netgraph);
	Cvar_RegisterVariable(&net_chokeloopback);

	Cmd_AddCommand("uprate", Net_Rate_f);

	net_rate = DEFAULT_RATE;
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand( netsrc_t sock, netadr_t adr, int length, byte* data )
{
	sizebuf_t	send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];

// write the packet header
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 0;

	MSG_WriteLong(&send, -1);	// -1 sequence means out of band
	SZ_Write(&send, data, length);

// send the datagram
	NET_SendPacket(sock, send.cursize, send.data, adr);
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint( netsrc_t sock, netadr_t adr, char* format, ... )
{
	va_list		argptr;
	char		string[8192];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	Netchan_OutOfBand(sock, adr, strlen(string), (byte*)string);
}


/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup( netsrc_t socketnumber, netchan_t* chan, netadr_t adr )
{
	memset(chan, 0, sizeof(*chan));

	chan->sock = socketnumber;
	chan->remote_address = adr;
	chan->last_received = realtime;
	chan->connect_time = realtime;

	chan->message.data = chan->message_buf;
	chan->message.allowoverflow = TRUE;
	chan->message.maxsize = MAX_MSGLEN;

	chan->rate = net_rate;
}


/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
#define	MAX_BACKUP	200
qboolean Netchan_CanPacket( netchan_t* chan )
{
	// Never choke loopback packets.
	if (!net_chokeloopback.value && chan->remote_address.type == NA_LOOPBACK)
		return TRUE;

	if (sv.active && sv_lan.value)
	{
		// LAN games don't need to honor the client's configured rate
		if (chan->cleartime < realtime + MAX_BACKUP / (float)DEFAULT_RATE)
			return TRUE;
	}
	else
	{
		if (chan->cleartime < realtime + MAX_BACKUP * (1.0f / chan->rate))
			return TRUE;
	}

	return FALSE;
}


/*
===============
Netchan_CanReliable

Returns true if the bandwidth choke isn't
================
*/
qboolean Netchan_CanReliable( netchan_t* chan )
{
	if (chan->reliable_length)
		return FALSE;			// waiting for ack

	return Netchan_CanPacket(chan);
}

/*
===============
Netchan_Transmit

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit( netchan_t* chan, int length, byte* data )
{
	sizebuf_t	send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];
	qboolean	send_reliable;
	unsigned	w1, w2;
	int			i;
	float		fRate;

// check for message overflow
	if (chan->message.overflowed)
	{
		chan->fatal_error = TRUE;
		Con_Printf("%s:Outgoing message overflow\n",
			NET_AdrToString(chan->remote_address));
		return;
	}

// if the remote side dropped the last reliable message, resend it
	send_reliable = FALSE;

	if (chan->incoming_acknowledged > chan->last_reliable_sequence
		&& chan->incoming_reliable_acknowledged != chan->reliable_sequence)
		send_reliable = TRUE;

// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliable_length && chan->message.cursize)
	{
		memcpy(chan->reliable_buf, chan->message_buf, chan->message.cursize);
		chan->reliable_length = chan->message.cursize;
		chan->message.cursize = 0;
		chan->reliable_sequence ^= 1;
		send_reliable = TRUE;
	}

// write the packet header
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 0;

	w1 = chan->outgoing_sequence | (send_reliable << 31);
	w2 = chan->incoming_sequence | (chan->incoming_reliable_sequence << 31);

	chan->outgoing_sequence++;

	MSG_WriteLong(&send, w1);
	MSG_WriteLong(&send, w2);

// copy the reliable message to the packet first
	if (send_reliable)
	{
		SZ_Write(&send, chan->reliable_buf, chan->reliable_length);
		chan->last_reliable_sequence = chan->outgoing_sequence;
	}

	// Is there room for the unreliable payload?
	if (send.maxsize - send.cursize >= length)
	{
		SZ_Write(&send, data, length);
	}

// send the datagram
	i = chan->outgoing_sequence & (MAX_LATENT - 1);
	chan->outgoing_size[i] = send.cursize;
	chan->outgoing_time[i] = realtime;

	NET_SendPacket(chan->sock, send.cursize, send.data, chan->remote_address);

	if (sv.active && sv_lan.value)
		// LAN games don't need to honor the client's configured rate
		fRate = 1.0f / (float)DEFAULT_RATE;
	else
		fRate = 1.0f / chan->rate;

	if (chan->cleartime < realtime)
		chan->cleartime = realtime + send.cursize * fRate;
	else
		chan->cleartime += send.cursize * fRate;

	if (showpackets.value)
		Con_Printf("--> s=%i(%i) a=%i(%i) %i ",
			chan->outgoing_sequence,
			send_reliable,
			chan->incoming_sequence,
			chan->incoming_reliable_sequence,
			send.cursize);

	if (showpackets.value)
		Con_Printf("\n");
}

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process( netchan_t* chan )
{
	unsigned		sequence, sequence_ack;
	unsigned		reliable_ack, reliable_message;
	float			elapsed;

	if (!NET_CompareAdr(net_from, chan->remote_address))
		return FALSE;

// get sequence numbers	
	MSG_BeginReading();
	sequence = MSG_ReadLong();
	sequence_ack = MSG_ReadLong();

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	sequence &= ~(1 << 31);
	sequence_ack &= ~(1 << 31);

	if (showpackets.value)
		Con_Printf("<-- s=%i(%i) a=%i(%i) %i\n",
			sequence,
			reliable_message,
			sequence_ack,
			reliable_ack,
			net_message.cursize);

	if (showpackets.value)
		Con_Printf("\n");

//
// discard stale or duplicated packets
//
	if (sequence <= (unsigned)chan->incoming_sequence)
	{
		if (showdrop.value)
			Con_Printf("%s:Out of order packet %i at %i\n",
				NET_AdrToString(chan->remote_address),
				sequence,
				chan->incoming_sequence);
		return FALSE;
	}

//
// dropped packets don't keep the message from being used
//
	net_drop = sequence - (chan->incoming_sequence + 1);
	if (net_drop > 0)
	{
		chan->drop_count += 1;

		if (showdrop.value)
			Con_Printf("%s:Dropped %i packets at %i\n",
				NET_AdrToString(chan->remote_address),
				net_drop,
				sequence);
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == (unsigned)chan->reliable_sequence)
		chan->reliable_length = 0;	// it has been received

//
// if this message contains a reliable message, bump incoming_reliable_sequence 
//
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
		chan->incoming_reliable_sequence ^= 1;

//
// the message can now be read from the current message pointer
// update statistics counters
//
	elapsed = realtime - chan->last_received;

	if (elapsed < 0.001f)
		elapsed = 0.001f;
	if (elapsed > 0.5f)
		elapsed = 0.5f;

	Netchan_UpdateStats(chan, chan->outgoing_sequence - sequence_ack, 1.0f / elapsed);

	chan->good_count += 1;

	chan->last_received = realtime;

	return TRUE;
}

/*
=================
Netchan_UpdateStats

Keeps a rolling window of the last MAX_FLOWS packets and averages them into
frame_latency / frame_rate, rather than an exponential average, so a single
bad sample can't dominate the reading.
=================
*/
void Netchan_UpdateStats( netchan_t* chan, int count, float rate )
{
	flowstat_t* pflow;

	pflow = &chan->flow[chan->good_count & (MAX_FLOWS - 1)];
	if (pflow)
	{
		pflow->size = count;
		pflow->time = rate;

		if (chan->good_count < MAX_FLOWS)
		{
			chan->frame_latency = 0;
			chan->frame_rate = 0;
		}
		else
		{
			int		i;
			float	sumCount = 0;
			float	sumRate = 0;

			for (i = 0; i < MAX_FLOWS; i++)
			{
				sumRate += chan->flow[i].time;
				sumCount += (float)chan->flow[i].size;
			}

			chan->frame_latency = sumCount / MAX_FLOWS;
			chan->frame_rate = sumRate / MAX_FLOWS;
		}
	}
}

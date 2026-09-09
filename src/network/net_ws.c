// net_ws.c
// Windows IP Support layer.

#include "quakedef.h"
#include "pr_cmds.h"

#if defined( _WIN32 )

#include <winsock.h>
#ifndef  _WIN32_WCE
#include <WSipx.h> // For ipx socket
#endif
typedef int socklen_t;

#endif

cvar_t host_name = { "hostname", "Half-Life" };

static cvar_t ipname = { "ip", "localhost" };
cvar_t iphostport = { "ip_hostport", "0" };
cvar_t hostport = { "hostport", "0" };
static cvar_t defport = { "port", PORT_SERVER };
static cvar_t ip_clientport = { "ip_clientport", "0" };
static cvar_t clientport = { "clientport", PORT_CLIENT };

#ifdef _WIN32
static cvar_t ipx_hostport = { "ipx_hostport", "0" };
static cvar_t ipx_clientport = { "ipx_clientport", "0" };
#endif

cvar_t fakelag = { "fakelag", "0.0f" };  // Lag all incoming network data (including loopback) by xxx ms.
cvar_t fakeloss = { "fakeloss", "0.0f" }; // Act like we dropped the packet this % of the time.

qboolean noip = TRUE;    // Disable IP Support

netadr_t net_local_adr;

#ifdef _WIN32
qboolean noipx = FALSE;    // Disable IPX Support

netadr_t net_local_ipx_adr;
#endif

int			net_maxpacket = 0;
int			net_maxingame = 0;
int			net_activeconnections = 0;

netadr_t	net_from;

static byte net_message_buffer[MAX_MSGLEN];
sizebuf_t	net_message;

#ifdef _WIN32
WSADATA		winsockdata;
#endif

#if HLDC_MP
static SOCKET ip_sockets[2] = { INVALID_SOCKET, INVALID_SOCKET };
static qboolean winsock_initialized;

#define NET_SPLIT_HEADER 9
#define NET_SPLIT_PAYLOAD (1400 - NET_SPLIT_HEADER)
#define NET_SPLIT_PARTS ((MAX_MSGLEN + NET_SPLIT_PAYLOAD - 1) / NET_SPLIT_PAYLOAD)

typedef struct
{
	qboolean active;
	netadr_t from;
	unsigned int sequence;
	unsigned int received;
	int count;
	int total;
	DWORD started;
	byte data[MAX_MSGLEN];
} splitpacket_t;

static splitpacket_t net_split[2];

static qboolean NET_GetLong( netsrc_t sock, netadr_t from, byte *data, int size, int *outSize )
{
	splitpacket_t *split = &net_split[sock];
	unsigned int sequence;
	int part, count, payload, offset;
	DWORD now;

	if (size <= NET_SPLIT_HEADER || size > 1400)
		return FALSE;
	memcpy(&sequence, data + 4, sizeof(sequence));
	part = data[8] >> 4;
	count = data[8] & 15;
	payload = size - NET_SPLIT_HEADER;
	offset = part * NET_SPLIT_PAYLOAD;
	if (!count || count > NET_SPLIT_PARTS || part >= count ||
		offset + payload > MAX_MSGLEN ||
		(part != count - 1 && payload != NET_SPLIT_PAYLOAD))
	{
		return FALSE;
	}
	now = GetTickCount();
	if (split->active && (DWORD)(now - split->started) >= 5000)
		split->active = FALSE;
	if (split->active && !NET_CompareAdr(split->from, from))
		return FALSE;
	if (!split->active || split->sequence != sequence)
	{
		split->active = TRUE;
		split->from = from;
		split->sequence = sequence;
		split->received = 0;
		split->count = count;
		split->total = 0;
		split->started = now;
	}
	if (count != split->count || (split->received & (1U << part)))
		return FALSE;
	memcpy(split->data + offset, data + NET_SPLIT_HEADER, payload);
	split->received |= 1U << part;
	if (part == count - 1)
		split->total = offset + payload;

	if (split->received != (1U << count) - 1)
		return FALSE;
	memcpy(data, split->data, split->total);
	*outSize = split->total;
	split->active = FALSE;

	return TRUE;
}

static qboolean NET_InitWinsock( void )
{
	int error;

	if (COM_CheckParm("-noip"))
		return FALSE;
	if (!winsock_initialized)
	{
		error = WSAStartup(MAKEWORD(1, 1), &winsockdata);

		if (!error)
		{
			winsock_initialized = TRUE;
		}
		else
			Con_Printf("Winsock initialization failed: %d\n", error);
	}
	noip = !winsock_initialized;
	return winsock_initialized;
}
#else
int			ip_sockets[2] = { 0, 0 };
#endif
#ifdef _WIN32
int			ipx_sockets[2] = { 0, 0 };
#endif

#define	MAX_LOOPBACK 4

typedef struct
{
	byte	data[MAX_MSGLEN + PACKET_HEADER + 1];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get;
	int			send;
} loopback_t;

static loopback_t	loopbacks[2];

typedef struct packetlag_s
{
	char* pData; // Raw stream data is stored.
	int   nSize;
	netadr_t net_from;
	float receivedTime;
	struct packetlag_s* pNext;
	struct packetlag_s* pPrev;
} packetlag_t;

static packetlag_t g_pLagData[2];  // List of lag structures, if fakelag is set.

int losscount[2] = { 0, 0 };

//=============================================================================

qboolean NET_CompareAdr( netadr_t a, netadr_t b )
{
#if HLDC_MP
	return NET_CompareBaseAdr(a, b) &&
		(a.type == NA_LOOPBACK || a.port == b.port);
#else
	return TRUE;
#endif
}

qboolean NET_CompareClassBAdr( netadr_t a, netadr_t b )
{
#if HLDC_MP
	if (a.type != b.type)
		return FALSE;
	if (a.type == NA_LOOPBACK)
		return TRUE;
	return a.type == NA_IP && a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1];
#else
	return TRUE;
#endif
}

qboolean NET_IsReservedAdr( netadr_t adr )
{
#if HLDC_MP
	if (adr.type == NA_LOOPBACK)
		return TRUE;
	if (adr.type != NA_IP)
		return FALSE;
	return adr.ip[0] == 10 || adr.ip[0] == 127 ||
		(adr.ip[0] == 172 && adr.ip[1] >= 16 && adr.ip[1] <= 31) ||
		(adr.ip[0] == 192 && adr.ip[1] == 168);
#else
	return TRUE;
#endif
}

qboolean NET_CompareBaseAdr( netadr_t a, netadr_t b )
{
#if HLDC_MP
	if (a.type != b.type)
		return FALSE;
	if (a.type == NA_LOOPBACK)
		return TRUE;
	return (a.type == NA_IP || a.type == NA_BROADCAST) &&
		!memcmp(a.ip, b.ip, sizeof(a.ip));
#else
	return TRUE;
#endif
}

char* NET_AdrToString( netadr_t a )
{
	static char s[64];

	memset(s, 0, sizeof(s));

#if HLDC_MP
	if (a.type == NA_IP || a.type == NA_BROADCAST)
	{
		sprintf(s, "%u.%u.%u.%u:%u", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
		return s;
	}
#endif
	sprintf(s, "loopback");

	return s;
}

char *NET_BaseAdrToString( netadr_t address )
{
	static char text[64];
	memset(text, 0, sizeof(text));
#if HLDC_MP
	if (address.type == NA_IP || address.type == NA_BROADCAST)
	{
		sprintf(text, "%u.%u.%u.%u", address.ip[0], address.ip[1], address.ip[2], address.ip[3]);
		return text;
	}
#endif
	sprintf(text, "loopback");
	return text;
}

void SCR_DrawGraphRect( vrect_t *rect, byte *color )
{
	Draw_FillRGBA(rect->x, rect->y, rect->width, rect->height,
		color[0], color[1], color[2], 128);
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean NET_StringToAdr( char* s, netadr_t* a )
{
#if HLDC_MP
	char name[256], *port, *p;
	struct hostent *host;
	unsigned long ip;
	unsigned int number;

	memset(a, 0, sizeof(*a));
	if (!s || !s[0] || strlen(s) >= sizeof(name))
		return FALSE;
	if (!strcmp(s, "localhost"))
	{
		a->type = NA_LOOPBACK;
		return TRUE;
	}
	strcpy(name, s);
	port = strchr(name, ':');
	if (port)
	{
		*port++ = 0;
		if (!*port)
			return FALSE;
		number = 0;
		for (p = port; *p; p++)
		{
			if (*p < '0' || *p > '9')
				return FALSE;
			number = number * 10 + *p - '0';
			if (number > 65535)
				return FALSE;
		}
		if (!number)
			return FALSE;
		a->port = htons((unsigned short)number);
	}
	if (!name[0])
		return FALSE;
	ip = inet_addr(name);
	if (ip == INADDR_NONE && strcmp(name, "255.255.255.255"))
	{
		if (!winsock_initialized)
			return FALSE;
		host = gethostbyname(name);
		if (!host || host->h_addrtype != AF_INET || host->h_length != 4)
			return FALSE;
		memcpy(&ip, host->h_addr_list[0], 4);
	}
	memcpy(a->ip, &ip, 4);
	a->type = ip == INADDR_BROADCAST ? NA_BROADCAST : NA_IP;
	return TRUE;
#else
	if (!strcmp(s, "localhost"))
	{
		memset(a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
	}

	return TRUE;
#endif
}

qboolean NET_IsLocalAddress( netadr_t adr )
{
	return adr.type == NA_LOOPBACK;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

qboolean NET_GetLoopPacket( netsrc_t sock, netadr_t* in_from, sizebuf_t* msg )
{
	int		i;
	loopback_t* loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return FALSE;

	i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	memcpy(msg->data, &loop->msgs[i].data[0], loop->msgs[i].datalen);
	msg->cursize = loop->msgs[i].datalen;

	memset(in_from, 0, sizeof(*in_from));
	in_from->type = NA_LOOPBACK;

	return TRUE;
}

void NET_SendLoopPacket( netsrc_t sock, int length, void* data )
{
	int		i;
	loopback_t* loop;

	loop = &loopbacks[sock ^ 1];

	i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

/*
==================
NET_RemoveFromPacketList(packetlag_t *pPacket)

Unlinks it from the current list.
==================
*/
void NET_RemoveFromPacketList( packetlag_t* pPacket )
{
	pPacket->pPrev->pNext = pPacket->pNext;
	pPacket->pNext->pPrev = pPacket->pPrev;

	pPacket->pNext = pPacket->pPrev = NULL;
}

/*
==================
NET_ClearLaggedList(packetlag_t *pList)

==================
*/
void NET_ClearLaggedList( packetlag_t* pList )
{
	packetlag_t* p, * n;

	for (p = pList->pNext; p && p != pList;)
	{
		n = p->pNext;

		NET_RemoveFromPacketList(p);

		if (p->pData)
			free(p->pData);
		p->pData = NULL;

		free(p);
		p = n;
	}

	pList->pNext = pList->pPrev = pList;
}

/*
===================
NET_AddToLagged

===================
*/
void NET_AddToLagged( netsrc_t sock, packetlag_t* pList, packetlag_t* pPacket, netadr_t* net_from, sizebuf_t messagedata )
{
	if (pPacket->pPrev || pPacket->pNext)
	{
		Con_Printf("Packet already linked\n");
		return;
	}

	pPacket->pPrev = pList->pPrev;
	pList->pPrev->pNext = pPacket;
	pList->pPrev = pPacket;
	pPacket->pNext = pList;

	pPacket->pData = (char*)MnemoAllocDbg(messagedata.cursize, __FILE__, __LINE__);
	memcpy(pPacket->pData, messagedata.data, messagedata.cursize);

	pPacket->receivedTime = realtime;   // Our time stamp.
	pPacket->net_from = *net_from;
	pPacket->nSize = messagedata.cursize;
}


qboolean NET_LagPacket( qboolean newdata, netsrc_t sock, netadr_t* from, sizebuf_t* data )
{
	packetlag_t* pNewPacketLag;
	packetlag_t* pPacket;

	if (fakelag.value <= 0.0f)
	{
		// Never leave any old msgs around
		return newdata;
	}

	if (newdata)
	{
		if (fakeloss.value)
		{
			losscount[sock]++;

			if (fakeloss.value > 0.0f)
			{
				// Act like we didn't hear anything if we are going to lose the packet.
				// Depends on random # generator.
				if (RandomLong(0, 100) <= (int)fakeloss.value)
					return FALSE;
			}
			else
			{
				// Deterministic loss: drop every Nth packet, N derived from the
				// (negative) fakeloss value.
				int nMod = (int)fabs(fakeloss.value);
				if (nMod < 2)
					nMod = 2;

				if (losscount[sock] % nMod == 0)
					return FALSE;
			}
		}

		pNewPacketLag = (packetlag_t*)MnemoAllocDbg(sizeof(packetlag_t), __FILE__, __LINE__);
		memset(pNewPacketLag, 0, sizeof(packetlag_t));

		NET_AddToLagged(sock, &g_pLagData[sock], pNewPacketLag, from, *data);
	}

	// Now check the correct list and feed any message that is old enought.
	pPacket = g_pLagData[sock].pNext;

	// Find an old enough packet.  If none are old enough, return false.
	while (pPacket != &g_pLagData[sock])
	{
		if (pPacket->receivedTime <= (realtime - (fakelag.value / 1000.0f)))
		{
			NET_RemoveFromPacketList(pPacket);

			if (pPacket->pData)
				memcpy(net_message.data, pPacket->pData, pPacket->nSize);
			net_message.cursize = pPacket->nSize;
			net_from = pPacket->net_from;

			if (pPacket->pData)
				free(pPacket->pData);
			free(pPacket);
			return TRUE;
		}

		pPacket = pPacket->pNext;
	}

	return FALSE;
}

//=============================================================================

#if HLDC_MP
static void NET_AdrToSockaddr( netadr_t a, struct sockaddr_in* address )
{
	memset(address, 0, sizeof(*address));
	address->sin_family = AF_INET;
	address->sin_port = a.port;
	if (a.type == NA_BROADCAST)
		address->sin_addr.s_addr = INADDR_BROADCAST;
	else
		memcpy(&address->sin_addr, a.ip, 4);
}

static SOCKET NET_IPSocket( char* net_interface, int port )
{
	SOCKET newsocket;
	struct sockaddr_in address;
	netadr_t adr;
	u_long nonblocking = 1;
	int broadcast = 1;

	if (port != PORT_ANY && (port < 0 || port > 65535))
		return INVALID_SOCKET;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = port == PORT_ANY ? 0 : htons((unsigned short)port);
	if (net_interface[0] && Q_stricmp(net_interface, "localhost"))
	{
		if (!NET_StringToAdr(net_interface, &adr) || adr.type != NA_IP)
			return INVALID_SOCKET;
		memcpy(&address.sin_addr, adr.ip, 4);
	}
	newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newsocket == INVALID_SOCKET)
		return INVALID_SOCKET;

	if (ioctlsocket(newsocket, FIONBIO, &nonblocking) == SOCKET_ERROR ||
		setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR ||
		bind(newsocket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Con_Printf("UDP socket error %i\n", WSAGetLastError());
		closesocket(newsocket);
		return INVALID_SOCKET;
	}
	return newsocket;
}

static void NET_OpenIP( void )
{
	int port;
	struct sockaddr_in address;
	int length = sizeof(address);

	if (ip_sockets[NS_CLIENT] != INVALID_SOCKET)
		return;
	port = (int)ip_clientport.value;
	if (!port)
		port = (int)clientport.value;
	if (!port)
		port = PORT_ANY;
	ip_sockets[NS_CLIENT] = NET_IPSocket(ipname.string, port);
	if (ip_sockets[NS_CLIENT] == INVALID_SOCKET && port != PORT_ANY)
		ip_sockets[NS_CLIENT] = NET_IPSocket(ipname.string, PORT_ANY);
	if (ip_sockets[NS_CLIENT] == INVALID_SOCKET)
	{
		Con_Printf("Unable to open UDP port\n");
		return;
	}
	memset(&net_local_adr, 0, sizeof(net_local_adr));
	if (!getsockname(ip_sockets[NS_CLIENT], (struct sockaddr*)&address, &length))
	{
		net_local_adr.type = NA_IP;
		net_local_adr.port = address.sin_port;
		memcpy(net_local_adr.ip, &address.sin_addr, 4);
	}
}
#endif

qboolean NET_GetPacket( netsrc_t sock )
{
#if HLDC_MP
	struct sockaddr_in address;
	int length, received, error;

	if (sock != NS_CLIENT && sock != NS_SERVER)
		return FALSE;
	if (sock == NS_CLIENT)
		NET_DialFrame();
#endif
	// If we got a message from the loopback system, see if it should be lagged.
	if (NET_GetLoopPacket(sock, &net_from, &net_message))
	{
		return NET_LagPacket(TRUE, sock, &net_from, &net_message);
	}

#if HLDC_MP
	if (!noip && ip_sockets[sock] != INVALID_SOCKET)
	{
		length = sizeof(address);
		received = recvfrom(ip_sockets[sock], (char*)net_message.data,
			net_message.maxsize, 0, (struct sockaddr*)&address, &length);
		if (received != SOCKET_ERROR)
		{
			if (received > 0 && length == sizeof(address) && address.sin_family == AF_INET)
			{
				memset(&net_from, 0, sizeof(net_from));
				net_from.type = NA_IP;
				net_from.port = address.sin_port;
				memcpy(net_from.ip, &address.sin_addr, 4);
				if (received >= 4 && !memcmp(net_message.data, "\xfe\xff\xff\xff", 4))
				{
					if (!NET_GetLong(sock, net_from, net_message.data, received, &received))
						return NET_LagPacket(FALSE, sock, &net_from, &net_message);
				}
				net_message.cursize = received;

				return NET_LagPacket(TRUE, sock, &net_from, &net_message);
			}
		}
		else
		{
			error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK && error != WSAECONNRESET && error != WSAEMSGSIZE)
				Con_DPrintf("UDP receive error %i\n", error);
		}
	}
	return NET_LagPacket(FALSE, sock, &net_from, &net_message);
#else
	return FALSE;
#endif
}

//=============================================================================

void NET_SendPacket( netsrc_t sock, int length, void* data, netadr_t to )
{
#if HLDC_MP
	struct sockaddr_in address;
	int error;

	if ((sock != NS_CLIENT && sock != NS_SERVER) || length < 0 || length > MAX_MSGLEN)
		return;
	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket(sock, length, data);
		return;
	}
	if (noip || ip_sockets[sock] == INVALID_SOCKET ||
		(to.type != NA_IP && to.type != NA_BROADCAST))
	{
		return;
	}

	NET_AdrToSockaddr(to, &address);
	if (sendto(ip_sockets[sock], (char*)data, length, 0,
		(struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
			Con_DPrintf("UDP send error %i\n", error);
	}
#else
	NET_SendLoopPacket(sock, length, data);
#endif
}

//=============================================================================

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config( qboolean multiplayer )
{
#if HLDC_MP
	int i;

	if (!multiplayer)
	{
		memset(net_split, 0, sizeof(net_split));
		for (i = 0; i < 2; i++)
		{
			if (ip_sockets[i] != INVALID_SOCKET)
				closesocket(ip_sockets[i]);
			ip_sockets[i] = INVALID_SOCKET;
		}
		memset(&net_local_adr, 0, sizeof(net_local_adr));
	}
	else if (NET_InitWinsock())
		NET_OpenIP();
#else
	static	qboolean	old_config;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;
#endif
}

void NET_ShowMaxPacketSizes_f( void )
{
	Con_Printf("Max Packet     :  %i\n", net_maxpacket);
	Con_Printf("Max Game Packet:  %i\n", net_maxingame);
}

void Net_BadConnection_f( void )
{
	if (cls.state != ca_active)
		return;

	Cbuf_AddText(va("fakelag 200\nfakeloss 10\nnetchokeloop 1\nr_netgraph 1\n"));
	Con_Printf("Simulating a poor connection\n");
	Cbuf_AddText("ping\n");
}

/*
====================
NET_Init

====================
*/
void NET_Init( void )
{
	int		i;

	Cmd_AddCommand("netbad", Net_BadConnection_f);
	Cmd_AddCommand("netmax", NET_ShowMaxPacketSizes_f);

	Cvar_RegisterVariable(&ipname);
	Cvar_RegisterVariable(&iphostport);
	Cvar_RegisterVariable(&hostport);
	Cvar_RegisterVariable(&defport);
	Cvar_RegisterVariable(&ip_clientport);
	Cvar_RegisterVariable(&clientport);
	Cvar_RegisterVariable(&ipx_hostport);
	Cvar_RegisterVariable(&ipx_clientport);
	Cvar_RegisterVariable(&fakelag);
	Cvar_RegisterVariable(&fakeloss);
	Cvar_RegisterVariable(&host_name);

	noipx = TRUE;
	noip = TRUE;
#if HLDC_MP
	NET_InitWinsock();
	NET_DialInit();
#endif

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.data = net_message_buffer;

	for (i = 0; i < 2; i++)
	{
		g_pLagData[i].pNext = g_pLagData[i].pPrev = &g_pLagData[i];  // List of lag structures, if fakelag is set.
	}
}

/*
====================
NET_Shutdown

====================
*/
void NET_Shutdown( void )
{
#if HLDC_MP
	NET_DialHangup();
#endif
	NET_ClearLaggedList(&g_pLagData[0]);
	NET_ClearLaggedList(&g_pLagData[1]);

	NET_Config(FALSE);
#if HLDC_MP
	if (winsock_initialized)
		WSACleanup();
	winsock_initialized = FALSE;
	noip = TRUE;
#endif
}

#define MAX_GRAPH_WIDTH	256

typedef struct
{
	int   count;
	float high;
	float rolling;
} netgraph_sample_t;

netgraph_sample_t	net_bytes_colors[MAX_GRAPH_WIDTH];
int		net_bytes_current_color_index = 0;
netgraph_sample_t	net_datagram_colors[MAX_GRAPH_WIDTH];
int		net_datagram_current_color_index = 0;

/*
==============
NET_InitColors
==============
*/
void NET_InitColors( void )
{
	int i;

	for (i = 0; i < MAX_GRAPH_WIDTH; i++)
	{
		net_bytes_colors[i].high = 0;
		net_bytes_colors[i].rolling = 0;
		net_bytes_colors[i].count = 0;
		net_datagram_colors[i].high = 0;
		net_datagram_colors[i].rolling = 0;
		net_datagram_colors[i].count = 0;
	}
}

/*
==============
SCR_ClampHigh
==============
*/
float SCR_ClampHigh( float value )
{
	float result = sqrtf(value) / sqrtf(scr_graphhigh.value);

	if (result > 1.0f)
		result = 1.0f;
	if (result < 0.0f)
		result = 0.0f;

	return result;
}

/*
==============
SCR_UpdateUsage
==============
*/
void SCR_UpdateNetUsage( int nBytes, int nListeners, qboolean bIsDatagram )
{
	int		i;
	int		index;
	int		nClients;
	float	rolling;
	float	normalizedBytes; // bytes per a listener/a player
	float	high;

	if (!scr_netusage.value)
		return;

	normalizedBytes = nBytes;

	if (scr_graphmean.value)
	{
		if (nListeners > 0)
			normalizedBytes /= nListeners;
		else
			normalizedBytes = 0;
	}

	// spread the value between all the players
	SV_CountPlayers(&nClients, NULL);
	if (nClients > 0)
		normalizedBytes /= nClients;

	high = SCR_ClampHigh(normalizedBytes);

	if (bIsDatagram)
	{
		if (nBytes == 0)
			return;

		index = net_datagram_current_color_index & (MAX_GRAPH_WIDTH - 1);
		net_datagram_colors[index].high = high;
		net_datagram_colors[index].count = normalizedBytes;

		rolling = 0.0f;
		for (i = 0; i < 32; i++)
		{
			rolling += net_datagram_colors[(net_datagram_current_color_index - i - 1) & (MAX_GRAPH_WIDTH - 1)].count;
		}
		net_datagram_colors[index].rolling = SCR_ClampHigh(rolling / 32.0f);

		net_datagram_current_color_index++;
	}
	else
	{
		if (nBytes == 0)
			return;

		// Store regular transfer stats
		index = net_bytes_current_color_index & (MAX_GRAPH_WIDTH - 1);
		net_bytes_colors[index].high = high;
		net_bytes_colors[index].count = normalizedBytes;

		net_bytes_current_color_index++;
	}
}

/*
==============
SCR_ClampHeight
==============
*/
int SCR_ClampHeight( float value )
{
	int height = (int)(scr_graphheight.value * value);
	if (height < 0)
		height = 0;
	if ((int)scr_graphheight.value < height)
		height = (int)scr_graphheight.value;
	return height;
}

/*
==============
SCR_SetTintFromFactor
==============
*/
void SCR_SetTintFromFactor( float factor, qboolean bTintCyan, byte* color )
{
	if (bTintCyan)
	{
		color[0] = 0;
		color[1] = factor * 255.0f;
		color[2] = factor * 255.0f;
	}
	else
	{
		color[0] = factor * 255.0f;
		color[1] = 0;
		color[2] = 0;
	}
}

/*
==============
SCR_DrawOutlineRect
==============
*/
void SCR_DrawOutlineRect( vrect_t* r, byte* color )
{
	vrect_t rcFill;

	rcFill.width = r->width;
	rcFill.height = 1;
	rcFill.x = r->x;
	rcFill.y = r->y;
	D_FillRect(&rcFill, color); // top

	rcFill.y = r->y + r->height;
	D_FillRect(&rcFill, color); // bottom

	rcFill.width = 1;
	rcFill.height = r->height;
	rcFill.x = r->x;
	rcFill.y = r->y;
	D_FillRect(&rcFill, color); // left

	rcFill.x = r->x + r->width;
	D_FillRect(&rcFill, color); // right
}

/*
==============
SCR_NetUsage
==============
*/
void SCR_NetUsage( void )
{
	float   shade;
	int     i, j;
	int     x, y;
	int     height, lastHeight;
	int		index;
	int     width;
	byte    color[3];
	vrect_t rcFill;

	if (!scr_netusage.value)
		return;

	if (!net_bytes_current_color_index && !net_datagram_current_color_index)
		return;

	if (scr_vrect.width > MAX_GRAPH_WIDTH + 2)
		width = MAX_GRAPH_WIDTH;
	else
		width = scr_vrect.width - 2;

	// center horizontally, 1 px up from bottom
	x = scr_vrect.x + (scr_vrect.width - width) / 2 + 1;
	y = scr_vrect.y + scr_vrect.height - 2;

	rcFill.y = y - SCR_ClampHeight(SCR_ClampHigh(scr_graphmedian.value));
	rcFill.width = 1;
	rcFill.height = 1;

	color[0] = 63;
	color[1] = 63;
	color[2] = 63;

	rcFill.x = x - (net_datagram_current_color_index & 3) + 4;
	while (rcFill.x < x + width)
	{
		D_FillRect(&rcFill, color);
		rcFill.x += 4;
	}

	color[0] = 63;
	color[1] = 63;
	color[2] = 63;

	rcFill.x = x - 1;
	rcFill.y = y - scr_graphheight.value - 1.0f;
	rcFill.width = width + 1;
	rcFill.height = scr_graphheight.value + 2.0f;
	SCR_DrawOutlineRect(&rcFill, color);

	// byte usage
	i = net_bytes_current_color_index;
	if (i >= MAX_GRAPH_WIDTH)
		i = MAX_GRAPH_WIDTH;
	j = net_bytes_current_color_index - i;

	index = (net_bytes_current_color_index - i - 1) & (MAX_GRAPH_WIDTH - 1);
	SCR_ClampHeight(net_bytes_colors[index].high);

	for (i = 0; i < width; i++)
	{
		index = (j - i - 1) & (MAX_GRAPH_WIDTH - 1);
		shade = net_bytes_colors[index].high;
		if (shade == -1.0f)
			continue;

		if (shade < 0.8f)
		{
			color[0] = 128 + (int)(shade * 127.0f);
			color[1] = 128 + (int)(shade * 127.0f);
			color[2] = 0;
		}
		else
		{
			color[0] = 255;
			color[1] = 255;
			color[2] = 255;
		}

		Draw_FillRGBA(x + width - i - 1, y - SCR_ClampHeight(shade), 1, 1, color[0], color[1], color[2], 128);
	}

	// datagram usage
	i = net_datagram_current_color_index;
	if (i >= MAX_GRAPH_WIDTH)
		i = MAX_GRAPH_WIDTH;
	j = net_datagram_current_color_index - i;

	lastHeight = SCR_ClampHeight(net_datagram_colors[(j - 1) & (MAX_GRAPH_WIDTH - 1)].high);

	for (i = 0; i < width; i++)
	{
		index = (j - i - 1) & (MAX_GRAPH_WIDTH - 1);

		height = SCR_ClampHeight(net_datagram_colors[index].high);
		SCR_SetTintFromFactor(net_datagram_colors[index].high, FALSE, color);
		Draw_FillRGBA(x + width - i - 1, y - height, 1, 1, color[0], color[1], color[2], 128);

		height = SCR_ClampHeight(net_datagram_colors[index].rolling);
		SCR_SetTintFromFactor(net_datagram_colors[index].rolling, TRUE, color);
		Draw_FillRGBA(x + width - i - 1, y - (height > lastHeight ? height : lastHeight), 1, abs(height - lastHeight) + 1, color[0], color[1], color[2], 128);

		lastHeight = height;
	}
}

int packet_latency[MAX_GRAPH_WIDTH];

// Per-column latency-distribution buckets: 5 values, each divided by 5, read
// from a pair of unconfirmed frame_t-adjacent fields (raw offset +300..+320
// bytes past the frame_t base). The exact source fields haven't been traced
// yet -- see CLAUDE.md.
typedef struct
{
	unsigned short	a, b, c, d, e;
} netgraph_percentile_t;

netgraph_percentile_t	netgraph_percentiles[MAX_GRAPH_WIDTH];

// CL_ReallocateDynamicData selects the frame history size for the server.
int cl_update_backup = UPDATE_BACKUP;
int cl_update_mask = UPDATE_MASK;

/*
==============
R_NetGraph
==============
*/
void R_NetGraph( void )
{
	vrect_t			vrect;
	int				i;
	int				x, y;
	int				width, height, lastheight;
	int				sequence, start;
	int				index;
	int				diff;
	byte			color[3];
	frame_t*		frame;
	unsigned short*	pStats;

	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	width = vrect.width;
	if (width > MAX_GRAPH_WIDTH)
		width = MAX_GRAPH_WIDTH;

	sequence = cls.netchan.outgoing_sequence;
	start = sequence - cl_update_backup + 1;

	// Fill in frame data
	if (start <= sequence)
	{
		for (i = start; i <= sequence; i++)
		{
			frame = &cl.frames[i & cl_update_mask];
			index = i & (MAX_GRAPH_WIDTH - 1);

			if (frame->receivedtime == -1.0f)
			{
				packet_latency[index] = 9999;	// dropped
			}
			else if (frame->receivedtime == -2.0f)
			{
				packet_latency[index] = 10000;	// choked
			}
			else if (frame->receivedtime == -3.0f)
			{
				packet_latency[index] = 9997;	// ???
			}
			else if (frame->invalid)
			{
				packet_latency[index] = 9998;	// invalid delta
			}
			else
			{
				packet_latency[index] = (int)((frame->receivedtime - frame->senttime) * 20.0f);
			}

			pStats = (short*)((byte*)frame + 300);
			netgraph_percentiles[index].a = pStats[6] / 5;
			netgraph_percentiles[index].b = pStats[7] / 5;
			netgraph_percentiles[index].c = pStats[8] / 5;
			netgraph_percentiles[index].d = pStats[9] / 5;
			netgraph_percentiles[index].e = pStats[10] / 5;
		}
	}

	diff = vrect.width - width;
	if (diff < 0)
		diff += 1;
	x = vrect.x + diff / 2 + 1;
	y = vrect.y + vrect.height - 1;

	if (width > 0)
	{
		lastheight = 0;
		x = x + width - 1;

		for (i = 0; i < width; i++)
		{
			index = (sequence - i) & (MAX_GRAPH_WIDTH - 1);
			height = packet_latency[index];

			switch (height)
			{
			case 10000:	// choked
				color[0] = 255;
				color[1] = 255;
				color[2] = 0;
				break;
			case 9999:	// dropped
				color[0] = 255;
				color[1] = 0;
				color[2] = 0;
				break;
			case 9998:	// invalid delta
				color[0] = 0;
				color[1] = 0;
				color[2] = 255;
				break;
			case 9997:	// ???
				color[0] = 240;
				color[1] = 127;
				color[2] = 7;
				height = lastheight;
				break;
			default:
				color[0] = 63;
				color[1] = 255;
				color[2] = 63;
				lastheight = height;
				break;
			}

			if (height > scr_graphheight.value)
				height = (int)scr_graphheight.value;

			Draw_FillRGBA(x, y - height, 1, height, color[0], color[1], color[2], 128);

			if (r_netgraph.value >= 2.0f)
			{
				int topy = (int)(((float)y - scr_graphheight.value) - 1.0f);

				Draw_FillRGBA(x, topy, 1, 1, 255, 255, 255, 128);

				if (i == 0)
				{
					int ticky;

					for (ticky = topy; ticky > 0; ticky -= 10)
						Draw_FillRGBA(x + 1, ticky, 4, height, 64, 192, 128, 128);
				}

				if (packet_latency[index] <= 9995)
				{
					int remaining = packet_latency[index] - 1 - netgraph_percentiles[index].a;

					if (remaining > 1)
					{
						Draw_FillRGBA(x, remaining, 1, height, 255, 255, 0, 128);

						remaining -= netgraph_percentiles[index].b;
						if (remaining > 1)
						{
							Draw_FillRGBA(x, remaining, 1, height, 255, 0, 255, 128);

							remaining -= netgraph_percentiles[index].c;
							if (remaining > 1)
							{
								Draw_FillRGBA(x, remaining, 1, height, 0, 0, 255, 128);

								if (r_netgraph.value >= 3.0f)
								{
									remaining -= netgraph_percentiles[index].d;
									if (remaining > 1)
									{
										int lasty;

										Draw_FillRGBA(x, remaining, 1, height, 0, 255, 0, 128);

										lasty = (int)((((float)y - scr_graphheight.value) - 1.0f) - (float)netgraph_percentiles[index].e);
										if (lasty > 1)
											Draw_FillRGBA(x, lasty, 1, 2, 200, 200, 200, 128);
									}
								}
							}
						}
					}
				}
			}

			x--;
		}
	}
}

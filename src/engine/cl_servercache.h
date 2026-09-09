// cl_servercache.h
#if !defined( CL_SERVERCACHE_H )
#define CL_SERVERCACHE_H
#ifdef _WIN32
#pragma once
#endif

// Server cache to replace slist command.
#define MAX_LOCAL_SERVERS 16

typedef struct
{
	char		name[80];
	char		desc[256];
	char		gamedir[256];
	char		map[16];
	int			inuse;
	int			maxplayers;
	char		type;
	char		os;
	char		password;
	char		pad;
	short		mod;
	char		info_url[256];
	char		download_url[256];
	int			version;
	int			size;
	char		info[32];
	short		secure;
	short		dll;
	netadr_t	adr;
} server_cache_t;

#if HLDC_MP
#ifdef __cplusplus
extern "C" {
#endif
int CL_ServerListCount( void );
const server_cache_t* CL_ServerListEntry( int index );
int CL_ServerListPing( int index );
qboolean CL_RefreshServerList( char* address );
#ifdef __cplusplus
}
#endif
#endif

#endif

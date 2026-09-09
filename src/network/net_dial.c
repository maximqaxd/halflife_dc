// net_dial.c -- Dreamcast dial-up connections

#include "quakedef.h"

#if HLDC_MP
#include <windows.h>
#include <ras.h>

#pragma comment(lib, "mras.lib")

extern HWND g_hWnd;

static HRASCONN dial_connection;
static qboolean dial_closing;
static qboolean dial_connected;
static DWORD dial_polltime;
static DWORD dial_starttime;
static char dial_server[MAX_QPATH];
static cvar_t net_dial_profile = { "net_dial_profile", "ISP0", FCVAR_ARCHIVE };

void NET_DialHangup( void )
{
	DWORD error;
	dial_server[0] = 0;

	if (!dial_connection || dial_closing)
		return;

	error = RasHangUp(dial_connection);

	if (error == ERROR_INVALID_HANDLE)
	{
		dial_connection = NULL;
		dial_closing = FALSE;
		dial_connected = FALSE;
		NET_Config(FALSE);
		return;
	}
	if (error)
	{
		Con_Printf("Hangup error %lu\n", error);
		return;
	}
	dial_closing = TRUE;
	dial_connected = FALSE;
	NET_Config(FALSE);
}

void NET_DialFrame( void )
{
	RASCONNSTATUS status;
	DWORD error, now;
	char command[MAX_QPATH + 16];

	if (!dial_connection)
		return;
	now = GetTickCount();
	if (!dial_connected && !dial_closing && (DWORD)(now - dial_starttime) > 180000)
	{
		Con_Printf("Dial timed out\n");
		NET_DialHangup();
	}
	if ((DWORD)(now - dial_polltime) < 250)
		return;
	dial_polltime = now;
	memset(&status, 0, sizeof(status));
	status.dwSize = sizeof(status);
	error = RasGetConnectStatus(dial_connection, &status);
	if (dial_closing)
	{
		// RAS keeps the handle valid until the modem has released the line.
		if (error == ERROR_INVALID_HANDLE)
		{
			dial_connection = NULL;
			dial_closing = FALSE;
			Con_Printf("Modem disconnected\n");
		}
		return;
	}
	if (error || status.dwError || status.rasconnstate == RASCS_Disconnected)
	{
		Con_Printf("Dial disconnected (%lu)\n", error ? error : status.dwError);
		if (error == ERROR_INVALID_HANDLE)
		{
			dial_connection = NULL;
			dial_connected = FALSE;
			NET_Config(FALSE);
		}
		else
			NET_DialHangup();
		return;
	}
	if (status.rasconnstate == RASCS_Connected && !dial_connected)
	{
		dial_connected = TRUE;
		NET_Config(FALSE);
		NET_Config(TRUE);
		Con_Printf("PPP connected\nUse connect <server:port>\n");
		if (dial_server[0])
		{
			Con_Printf("Connecting to %s\n", dial_server);
			sprintf(command, "connect \"%s\"\n", dial_server);
			dial_server[0] = 0;
			Cbuf_AddText(command);
		}
	}
	else if (status.rasconnstate & RASCS_PAUSED)
	{
		Con_Printf("Dial needs new credentials\n");
		NET_DialHangup();
	}
}

qboolean NET_DialMessage( unsigned int message, unsigned long state, long error )
{
	if (message != WM_RASDIALEVENT)
		return FALSE;

	if (dial_connection && !dial_closing && error)
	{
		Con_Printf("Dial state %lu error %ld\n", state, error);
		NET_DialHangup();
	}
	return TRUE;
}

static void NET_DialList_f( void )
{
	RASENTRYNAME *entries;
	DWORD size = 0, count = 0, error, i;
	char name[RAS_MaxEntryName * 3 + 1];

	if (cmd_source != src_command)
		return;
	RasEnumEntries(NULL, NULL, NULL, &size, &count);
	if (!size)
	{
		Con_Printf("No dial-up profiles\nConfigure a CE RAS entry first\n");
		return;
	}
	entries = (RASENTRYNAME *)LocalAlloc(LPTR, size);
	if (!entries)
		return;
	entries[0].dwSize = sizeof(*entries);
	error = RasEnumEntries(NULL, NULL, entries, &size, &count);
	if (error)
		Con_Printf("RAS entries error %lu\n", error);
	else
	{
		for (i = 0; i < count; i++)
		{
			if (WideCharToMultiByte(CP_ACP, 0, entries[i].szEntryName, -1,
				name, sizeof(name), NULL, NULL))
				Con_Printf("%s\n", name);
		}
	}
	LocalFree(entries);
}

static qboolean NET_DialStart( char *profile )
{
	RASDIALPARAMS params;
	RASCONN existing;
	DWORD error, size, count;
	BOOL password;

	if (dial_connection)
	{
		Con_Printf("Modem busy; use net_hangup\n");
		return FALSE;
	}
	if (COM_CheckParm("-noip"))
	{
		Con_Printf("IP networking is disabled\n");
		return FALSE;
	}
	memset(&existing, 0, sizeof(existing));
	existing.dwSize = sizeof(existing);
	size = sizeof(existing);
	count = 0;
	error = RasEnumConnections(&existing, &size, &count);

	if (count || size > sizeof(existing))
	{
		Con_Printf("RAS connection already active\nUse net_dial_status\n");
		return FALSE;
	}
	if (error)
	{
		Con_Printf("RAS unavailable (%lu)\n", error);
		return FALSE;
	}
	memset(&params, 0, sizeof(params));
	params.dwSize = sizeof(params);
	if (!MultiByteToWideChar(CP_ACP, 0, profile, -1,
		params.szEntryName, RAS_MaxEntryName + 1))
	{
		Con_Printf("Invalid dial-up profile\n");
		return FALSE;
	}
	password = FALSE;
	error = RasGetEntryDialParams(NULL, &params, &password);

	if (!error && g_hWnd)
	{
		dial_connected = FALSE;
		dial_starttime = GetTickCount();

		error = RasDial(NULL, NULL, &params, 0xFFFFFFFF, g_hWnd, &dial_connection);
	}
	else if (!error)
		error = ERROR_INVALID_WINDOW_HANDLE;
	memset(&params, 0, sizeof(params));
	if (error)
	{
		Con_Printf("Dial error %lu\n", error);
		NET_DialHangup();
		return FALSE;
	}
	return TRUE;
}

static void NET_Dial_f( void )
{
	if (cmd_source != src_command)
		return;
	if (Cmd_Argc() != 2)
	{
		Con_Printf("net_dial <profile>\n");
		return;
	}
	NET_DialStart(Cmd_Argv(1));
}

qboolean NET_DialBeforeConnect( char *server )
{
	RASCONN connection;
	RASCONNSTATUS status;
	RASENTRYNAME entry;
	DWORD size, count, error;
	char profile[RAS_MaxEntryName * 3 + 1];

	if (!Q_stricmp(server, "local") || !Q_stricmp(server, "localhost"))
		return TRUE;
	if (strlen(server) >= sizeof(dial_server) || strchr(server, '"') ||
		strchr(server, ';') || strchr(server, '\r') || strchr(server, '\n'))
	{
		Con_Printf("Invalid server address\n");
		return FALSE;
	}
	if (dial_connected && !dial_closing)
		return TRUE;
	if (dial_connection)
	{
		if (!dial_closing)
		{
			strcpy(dial_server, server);
			Con_Printf("Waiting for PPP: %s\n", server);
		}
		else
			Con_Printf("Wait for modem hangup\n");
		return FALSE;
	}
	memset(&connection, 0, sizeof(connection));
	connection.dwSize = sizeof(connection);
	size = sizeof(connection);
	count = 0;
	error = RasEnumConnections(&connection, &size, &count);
	if (!error && count)
	{
		memset(&status, 0, sizeof(status));
		status.dwSize = sizeof(status);
		if (!RasGetConnectStatus(connection.hrasconn, &status) &&
			!status.dwError && status.rasconnstate == RASCS_Connected)
		{
			NET_Config(TRUE);
			return TRUE;
		}
		Con_Printf("Existing RAS is not connected\n");
		return FALSE;
	}
	if (error)
	{
		Con_Printf("RAS connection query failed: %lu\n", error);
		return FALSE;
	}
	if (net_dial_profile.string[0])
	{
		if (strlen(net_dial_profile.string) >= sizeof(profile))
			return FALSE;
		strcpy(profile, net_dial_profile.string);
	}
	else
	{
		memset(&entry, 0, sizeof(entry));
		entry.dwSize = sizeof(entry);
		size = sizeof(entry);
		count = 0;
		error = RasEnumEntries(NULL, NULL, &entry, &size, &count);
		if (error || count != 1)
		{
			Con_Printf("Select net_dial_profile first\nUse net_dial_list\n");
			return FALSE;
		}
		if (!WideCharToMultiByte(CP_ACP, 0, entry.szEntryName, -1,
			profile, sizeof(profile), NULL, NULL))
			return FALSE;
	}
	strcpy(dial_server, server);
	Con_Printf("Dialing %s for %s\n", profile, server);
	if (!NET_DialStart(profile))
		dial_server[0] = 0;
	return FALSE;
}

static void NET_Hangup_f( void )
{
	if (cmd_source == src_command)
		NET_DialHangup();
}

static void NET_DialStatus_f( void )
{
	RASCONN connection;
	RASCONNSTATUS status;
	DWORD size, count, error;

	if (cmd_source != src_command)
		return;
	if (!dial_connection)
	{
		memset(&connection, 0, sizeof(connection));
		connection.dwSize = sizeof(connection);
		size = sizeof(connection);
		count = 0;
		error = RasEnumConnections(&connection, &size, &count);
		if (!error && count)
		{
			memset(&status, 0, sizeof(status));
			status.dwSize = sizeof(status);
			error = RasGetConnectStatus(connection.hrasconn, &status);
			if (!error)
				Con_Printf("External RAS state %d\n", status.rasconnstate);
		}
		if (error)
			Con_Printf("RAS status error %lu\n", error);
	}
	Con_Printf("Modem: %s\n", dial_closing ? "disconnecting" :
		dial_connected ? "connected" : dial_connection ? "dialing" : "idle");
}

void NET_DialInit( void )
{
	Cvar_RegisterVariable(&net_dial_profile);

	Cmd_AddCommand("net_dial_list", NET_DialList_f);
	Cmd_AddCommand("net_dial", NET_Dial_f);
	Cmd_AddCommand("net_hangup", NET_Hangup_f);
	Cmd_AddCommand("net_dial_status", NET_DialStatus_f);
}
#endif

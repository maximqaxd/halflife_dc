// cvar.c -- dynamic variable tracking

#include "quakedef.h"
#include "winquake.h"
#include "info.h"

cvar_t* cvar_vars;
char* cvar_null_string = "";

/*
============
Cvar_FindVar
============
*/
cvar_t* Cvar_FindVar( char* var_name )
{
	cvar_t* var;

	for (var = cvar_vars; var; var = var->next)
		if (!Q_strcmp(var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue( char* var_name )
{
	cvar_t* var;

	var = Cvar_FindVar(var_name);
	if (!var)
		return 0;
	return Q_atof(var->string);
}

/*
============
Cvar_VariableInt
============
*/
int Cvar_VariableInt( char* var_name )
{
	cvar_t* var;

	var = Cvar_FindVar(var_name);
	if (!var)
		return 0;
	return Q_atoi(var->string);
}


/*
============
Cvar_VariableString
============
*/
char* Cvar_VariableString( char* var_name )
{
	cvar_t* var;

	var = Cvar_FindVar(var_name);
	if (!var)
		return cvar_null_string;
	return var->string;	
}


/*
============
Cvar_CompleteVariable
============
*/
char* Cvar_CompleteVariable( char* partial )
{
	cvar_t* cvar;
	char* name = NULL;
	int			len;

	len = Q_strlen(partial);

	if (!len)
		return NULL;

// check functions
	for (cvar = cvar_vars; cvar; cvar = cvar->next)
	{
		if (!Q_strncmp(partial, cvar->name, len))
		{
			if (Q_strlen(cvar->name) == len)
				return cvar->name;
			name = cvar->name;
		}
	}

	return name;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set( char* var_name, char* value )
{
	cvar_t*		var;
	qboolean	changed;
	char		szNew[1024];

	var = Cvar_FindVar(var_name);
	if (!var)
		return;

	if (var->flags & FCVAR_PRINTABLEONLY)
	{
		char*	pszValue;
		char*	pszDest;

		pszValue = value;
		pszDest = szNew;
		szNew[0] = 0;

		while (*pszValue)
		{
			if (*pszValue >= 32 && *pszValue <= 127)
				*pszDest++ = *pszValue;
			pszValue++;
		}
		*pszDest = 0;

		if (strlen(szNew) == 0)
			strcpy(szNew, "empty");

		value = szNew;
	}

	changed = Q_strcmp(var->string, value);

	if (var->flags & FCVAR_USERINFO)
	{
		if (cls.state == ca_dedicated)
		{
			Info_SetValueForKey(Info_Serverinfo(), var_name, value, MAX_INFO_STRING);
			SV_BroadcastCommand("fullserverinfo \"%s\"\n", Info_Serverinfo());
		}

		if (cls.state != ca_dedicated)
		{
			Info_SetValueForKey(cls.userinfo, var_name, value, sizeof(cls.userinfo));
			if (changed && cls.state > ca_connecting)
			{
				MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
				SZ_Print(&cls.netchan.message, va("setinfo \"%s\" \"%s\"\n", var_name, value));
			}
		}
	}

	if ((var->flags & FCVAR_SERVER) && changed)
	{
		Log_Printf("\"%s\" = \"%s\"\n", var_name, value);
		SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var_name, value);
	}

	Z_Free(var->string);	// free the old value string

	var->string = Z_Malloc(Q_strlen(value) + 1);
	Q_strcpy(var->string, value);
	var->value = Q_atof(var->string);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue( char* var_name, float value )
{
	char	val[32];

	sprintf(val, "%f", value);
	Cvar_Set(var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable( cvar_t* variable )
{
	char* oldstr;

// first check to see if it has allready been defined
	if (!Cvar_FindVar(variable->name))
	{
	// check for overlap with a command
		if (Cmd_Exists(variable->name))
		{
			Con_Printf("Cvar_RegisterVariable: %s is a command\n", variable->name);
		}
		else
		{
		// copy the value off, because future sets will Z_Free it
			oldstr = variable->string;
			variable->string = Z_Malloc(Q_strlen(oldstr) + 1);
			Q_strcpy(variable->string, oldstr);
			variable->value = Q_atof(variable->string);

		// link the variable in
			variable->next = cvar_vars;
			cvar_vars = variable;
		}
	}
}

/*
============
Cvar_RemoveHudCvars

Drops every cvar registered by the client DLL, freeing them and rebuilding
the list from the ones that remain.
============
*/
void Cvar_RemoveHudCvars( void )
{
	cvar_t*	var;
	cvar_t*	newlist;
	cvar_t*	next;

	newlist = NULL;
	for (var = cvar_vars; var; var = next)
	{
		next = var->next;
		if (var->flags & FCVAR_CLIENTDLL)
		{
			Z_Free(var->string);
			Z_Free(var);
		}
		else
		{
			var->next = newlist;
			newlist = var;
		}
	}
	cvar_vars = newlist;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command( void )
{
	cvar_t* v;

// check variables
	v = Cvar_FindVar(Cmd_Argv(0));
	if (!v)
		return FALSE;

// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf("\"%s\" is \"%s\"\n", v->name, v->string);
		return TRUE;
	}

// don't let clients change single-player-only cvars on a multiplayer server
	if (!(v->flags & FCVAR_SPONLY)
		|| cls.state == ca_dedicated
		|| cls.state == ca_disconnected
		|| cl.maxclients <= 1)
	{
		Cvar_Set(v->name, Cmd_Argv(1));
		return TRUE;
	}
	else
	{
		Con_Printf("Can't set %s in multiplayer\n", v->name);
		return TRUE;
	}
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables( void* f )
{
	cvar_t* var;

	for (var = cvar_vars; var; var = var->next)
		if (var->flags & FCVAR_ARCHIVE)
			Sys_FPrintf(f, "%s \"%s\"\n", var->name, var->string);
}


/*
============
Cmd_CvarListPrintCvar

Print some info to the console
============
*/
void Cmd_CvarListPrintCvar( cvar_t* var, FILE* f )
{
	char szOutstr[256];   // Ouput string
	if (var->value == (int)var->value)   // Clean up integers
		sprintf(szOutstr, "%-15s : %8i", var->name, (int)var->value);
	else
		sprintf(szOutstr, "%-15s : %8.3f", var->name, var->value);

	// Tack on archive setting
	if (var->flags & FCVAR_ARCHIVE)
	{
		strcat(szOutstr, ", a");
	}

	// And server setting
	if (var->flags & FCVAR_SERVER)
	{
		strcat(szOutstr, ", sv");
	}

	// And userinfo setting
	if (var->flags & FCVAR_USERINFO)
	{
		strcat(szOutstr, ", u");
	}

	// End the line
	strcat(szOutstr, "\n");

	Con_Printf("%s", szOutstr);

	if (f)
	{
		Sys_FPrintf(f, "%s", szOutstr);
	}
}

/*
============
Cmd_CvarList_f

List all cvars
============
*/
void Cmd_CvarList_f( void )
{
	cvar_t* var;			// Temporary Pointer to cvars
	int iCvars = 0;			// Number retrieved...
	int iArgs;				// Argument count
	char* partial = NULL;	// Partial cvar to search for...
	// E.eg
	int ipLen = 0;			// Length of the partial cvar

	char szTemp[256];
	FILE* f = NULL;         // FilePointer for logging
	qboolean bArchive = FALSE;	// Only list archive cvars
	qboolean bServer = FALSE;	// Only list server cvars
	qboolean bLogging = FALSE;

	iArgs = Cmd_Argc();		// Get count

	if (iArgs >= 2)			// Check for "CvarList ?" or "CvarList xxx"
	{
		if (!_stricmp(Cmd_Argv(1), "?"))
		{
			Con_Printf(
				"CvarList           : List all cvars\n"
				"CvarList [Partial] : List cvars starting with 'Partial'\n"
				"CvarList log logfile [Partial] : Logs cvars to file c:\\logfile.\n"
				"NOTE:  No relative paths allowed!");
			return;
		}

		if (!_stricmp(Cmd_Argv(1), "log"))
		{
			sprintf(szTemp, "c:\\%s", Cmd_Argv(2));
			f = fopen(szTemp, "wt");
			if (f)
				bLogging = TRUE;
			else
			{
				Con_Printf("Couldn't open [%s] for writing!\n", Cmd_Argv(2));
				fclose(f);
			}

			if (iArgs == 4)
			{
				partial = Cmd_Argv(3);
				ipLen = strlen(partial);
			}
		}
		else if (!_stricmp(Cmd_Argv(1), "-a"))
		{
			bArchive = TRUE;
		}
		else if (!_stricmp(Cmd_Argv(1), "-s"))
		{
			bServer = TRUE;
		}
		else
		{
			partial = Cmd_Argv(1);
			ipLen = strlen(partial);
		}
	}

	// Banner
	Con_Printf("CVar List\n--------------\n");

	// Loop through cvars...
	for (var = cvar_vars; var; var = var->next)
	{
		if ((!bArchive || (var->flags & FCVAR_ARCHIVE))
			&& (!bServer || (var->flags & FCVAR_SERVER)))
		{
			if (partial == NULL)		// List all cvars
			{
				Cmd_CvarListPrintCvar(var, f);
				iCvars++;
			}
			else if (!_strnicmp(var->name, partial, ipLen))	// Partial match
			{
				Cmd_CvarListPrintCvar(var, f);
				iCvars++;
			}
		}
	}

	// Show total and syntax help...
	if ((iArgs == 2) && partial && partial[0])
	{
		Con_Printf("--------------\n%3i CVars for [%s]\nCvarList ? for syntax\n", iCvars, partial);
	}
	else
	{
		Con_Printf("--------------\n%3i Total CVars\nCvarList ? for syntax\n", iCvars);
	}

	if (bLogging)
	{
		fclose(f);
	}
}

/*
============
Cvar_CountServerVariables

============
*/
int Cvar_CountServerVariables( void )
{
	int i = 0;
	cvar_t* var;

	for (var = cvar_vars; var; var = var->next)
	{
		if (var->flags & FCVAR_SERVER)
		{
			i++;
		}
	}

	return i;
}

void Cvar_CmdInit( void )
{
	Cmd_AddCommand("cvarlist", Cmd_CvarList_f);
}

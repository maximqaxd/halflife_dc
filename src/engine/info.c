// info.c -- info string (key/value) manipulation

#include "quakedef.h"
#include "info.h"

char serverinfo[MAX_INFO_STRING];

/*
===============
Info_ValueForKey

Searches the string for the given key and returns the associated value,
or an empty string.
===============
*/
char* Info_ValueForKey( char* s, char* key )
{
	char			pkey[MAX_INFO_STRING];
	static char		value[4][128];	// use two buffers so compares work without stomping on each other
	static int		valueindex;
	char*			o;

	valueindex = (valueindex + 1) & 3;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}

/*
===============
Info_RemoveKey
===============
*/
void Info_RemoveKey( char* s, char* key )
{
	char*	start;
	char	pkey[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char*	o;

	if (strstr(key, "\\"))
	{
		Con_Printf("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}
}

/*
===============
Info_RemovePrefixedKeys
===============
*/
void Info_RemovePrefixedKeys( char* start, char prefix )
{
	char*	s;
	char	pkey[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char*	o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] == prefix)
		{
			Info_RemoveKey(start, pkey);
			s = start;
		}

		if (!*s)
			return;
	}
}

/*
===============
Info_FindLargestKey

Returns the largest key in the info string that is not important
(one whose removal is allowed to make room).
===============
*/
char* Info_FindLargestKey( char* s, int maxsize )
{
	char			key[MAX_INFO_STRING];
	char			value[MAX_INFO_STRING];
	char*			o;
	int				l;
	static char		largest_key[MAX_INFO_STRING];
	int				largest_size;

	largest_key[0] = 0;
	largest_size = 0;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		l = o - key;
		*o = 0;

		if (!*s)
			return largest_key;
		s++;

		o = value;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		l += o - value;
		if (largest_size < l)
		{
			if (key[0] != '*'
				&& strcmp(key, "name")
				&& strcmp(key, "model")
				&& strcmp(key, "rate")
				&& strcmp(key, "topcolor")
				&& strcmp(key, "bottomcolor"))
			{
				strcpy(largest_key, key);
				largest_size = l;
			}
		}
	}

	return largest_key;
}

/*
===============
Info_SetValueForStarKey
===============
*/
void Info_SetValueForStarKey( char* s, char* key, char* value, int maxsize )
{
	char	newv[MAX_INFO_STRING], *v;
	int		c;

	if (strstr(key, "\\") || strstr(value, "\\"))
	{
		Con_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr(key, "\"") || strstr(value, "\""))
	{
		Con_Printf("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) >= 64 || strlen(value) >= 64)
	{
		Con_Printf("Keys and values must be < 64 characters.\n");
		return;
	}

	Info_RemoveKey(s, key);
	if (!value || !strlen(value))
		return;

	sprintf(newv, "\\%s\\%s", key, value);

	if (strlen(newv) + strlen(s) >= maxsize)
	{
		// no room, so remove the largest data until it fits
		if (*key != '*'
			&& strcmp(key, "name")
			&& strcmp(key, "model")
			&& strcmp(key, "topcolor")
			&& strcmp(key, "bottomcolor"))
		{
			Con_Printf("Info string length exceeded\n");
			return;
		}

		do
		{
			v = Info_FindLargestKey(s, maxsize);
			Info_RemoveKey(s, v);
			if (strlen(s) + strlen(newv) < maxsize)
				break;
		} while (*v);

		if (!*v)
		{
			Con_Printf("Info string length exceeded\n");
			return;
		}
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;
		if (!Q_stricmp(key, "name"))
		{
			// no non-printable
			if (c > 13)
				*s++ = c;
		}
		else
		{
			c &= 127;
			if (c > 31 && c < 128)
			{
				if (!Q_stricmp(key, "team"))
					c = tolower(c);
				*s++ = c;
			}
		}
	}
	*s = 0;
}

/*
===============
Info_SetValueForKey
===============
*/
void Info_SetValueForKey( char* s, char* key, char* value, int maxsize )
{
	if (key[0] == '*')
	{
		Con_Printf("Can't set * keys\n");
		return;
	}

	Info_SetValueForStarKey(s, key, value, maxsize);
}

/*
===============
Info_Print
===============
*/
void Info_Print( char* s )
{
	char	key[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char*	o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset(o, ' ', 20 - l);
			key[20] = 0;
		}
		else
			*o = 0;
		Con_Printf("%s", key);

		if (!*s)
		{
			Con_Printf("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Con_Printf("%s\n", value);
	}
}

/*
===================
Info_Serverinfo
===================
*/
char* Info_Serverinfo( void )
{
	return serverinfo;
}

/*
===============
Info_WriteVars

Write the archived info-string cvars out to a config file
===============
*/
void Info_WriteVars( void* f )
{
}

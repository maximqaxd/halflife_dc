// info.h -- info string (key/value) manipulation

#ifndef INFO_H
#define INFO_H

#define MAX_INFO_STRING 196
#define MAX_SERVERINFO_STRING 512
#define MAX_LOCALINFO 32768

extern char serverinfo[MAX_INFO_STRING];

char* Info_ValueForKey( char* s, char* key );
void  Info_RemoveKey( char* s, char* key );
void  Info_RemovePrefixedKeys( char* start, char prefix );
char* Info_FindLargestKey( char* s, int maxsize );
void  Info_SetValueForStarKey( char* s, char* key, char* value, int maxsize );
void  Info_SetValueForKey( char* s, char* key, char* value, int maxsize );
void  Info_Print( char* s );
char* Info_Serverinfo( void );
void  Info_WriteVars( void* f );

#endif

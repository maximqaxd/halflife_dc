#ifndef TMESSAGE_H
#define TMESSAGE_H
#pragma once

#define DEMO_MESSAGE "__DEMOMESSAGE__"
#define NETWORK_MESSAGE1 "__NETMESSAGE__1"
#define NETWORK_MESSAGE2 "__NETMESSAGE__2"
#define NETWORK_MESSAGE3 "__NETMESSAGE__3"
#define NETWORK_MESSAGE4 "__NETMESSAGE__4"
#define MAX_NETMESSAGE 4

extern client_textmessage_t* gMessageTable;
extern int					gMessageTableCount;
extern client_textmessage_t gNetworkTextMessage[MAX_NETMESSAGE];
extern char gNetworkTextMessageBuffer[MAX_NETMESSAGE][512];
extern const char* gNetworkMessageNames[MAX_NETMESSAGE];

char* memfgets( byte* pMemFile, int fileSize, int* pFilePos, char* pBuffer, int bufferSize );

// text message system
void					TextMessageInit( void );
client_textmessage_t*	TextMessageGet( const char* pName );
void CL_ParseTextMessage( void );

int TextMessageDrawCharacter( int x, int y, int number, int r, int g, int b );

void TrimSpace( const char* source, char* dest );

#endif		//TMESSAGE_H

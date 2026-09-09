// comndef.h  -- general definitions

#ifndef COMMON_H
#define COMMON_H
#pragma once

typedef short qboolean;

//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte* data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

#ifdef __cplusplus
extern "C" {
#endif

void SZ_Alloc( sizebuf_t* buf, int startsize );
void SZ_Clear( sizebuf_t* buf );
void* SZ_GetSpace( sizebuf_t* buf, int length );
void SZ_Write( sizebuf_t* buf, void* data, int length );
void SZ_Print( sizebuf_t* buf, char* data );

//============================================================================


void ClearLink( link_t* l );
void RemoveLink( link_t* l );
void InsertLinkBefore( link_t* l, link_t* before );
void InsertLinkAfter( link_t* l, link_t* after );

// (type*)STRUCT_FROM_LINK(link_t* link, type, member)
// ent = STRUCT_FROM_LINK(link, cl_entity_t, order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l, t, m) ((t*)((byte*)l - (int)&(((t*)0)->m)))

//============================================================================

#define COM_COPY_CHUNK_SIZE 1024   // For copying operations

#ifndef NULL
#define NULL ((void*)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	qboolean		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int		(*BigLong) (int l);
extern	int		(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

void MSG_WriteChar( sizebuf_t* sb, int c );
void MSG_WriteByte( sizebuf_t* sb, int c );
void MSG_WriteShort( sizebuf_t* sb, int c );
void MSG_WriteWord( sizebuf_t* sb, int c );
void MSG_WriteLong( sizebuf_t* sb, int c );
void MSG_WriteFloat( sizebuf_t* sb, float f );
void MSG_WriteString( sizebuf_t* sb, const char* s );
void MSG_WriteBuf( sizebuf_t* sb, int iSize, void* buf );
void MSG_WriteCoord( sizebuf_t* sb, float f );
void MSG_WriteAngle( sizebuf_t* sb, float f );
void MSG_WriteHiresAngle( sizebuf_t* sb, float f );
void MSG_WriteDeltaUsercmd( sizebuf_t* buf, struct usercmd_s* from, struct usercmd_s* cmd );
void MSG_WriteUsercmdGold( sizebuf_t* buf, struct usercmd_s* cmd, struct usercmd_s* from );
void MSG_WriteUsercmd36( sizebuf_t* buf, struct usercmd_s* cmd, struct usercmd_s* from );
void MSG_WriteBitUsercmd( struct usercmd_s* cmd, struct usercmd_s* from );
void MSG_WriteUsercmdByProtocol( sizebuf_t* buf, struct usercmd_s* cmd, struct usercmd_s* from );

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading( void );
int MSG_ReadChar( void );
int MSG_ReadByte( void );
int MSG_ReadShort( void );
int MSG_ReadWord( void );
int MSG_ReadLong( void );
float MSG_ReadFloat( void );
int MSG_ReadBuf( int iSize, void* pbuf );
char* MSG_ReadString( void );
char* MSG_ReadStringUntil( int terminator );
char* MSG_ReadStringLine( void );
float MSG_ReadCoord( void );
float MSG_ReadAngle( void );
float MSG_ReadHiresAngle( void );
void MSG_ReadDeltaUsercmd( struct usercmd_s* move, struct usercmd_s* from );
void MSG_ReadUsercmdGold( struct usercmd_s* move, struct usercmd_s* from );
void MSG_ReadUsercmd36( struct usercmd_s* move, struct usercmd_s* from );
void MSG_ReadBitUsercmd( struct usercmd_s* move, struct usercmd_s* from );
void MSG_ReadUsercmd( struct usercmd_s* move, struct usercmd_s* from );

void MSG_StartBitReading( sizebuf_t* buf );
void MSG_EndBitReading( sizebuf_t* buf );
qboolean MSG_ReadOneBit( void );
unsigned char MSG_ReadBitField8( unsigned int numbits );
unsigned short MSG_ReadBitField16( unsigned int numbits );
unsigned int MSG_ReadBitField32( unsigned int numbits );
unsigned int MSG_PeekBits( unsigned int numbits );
unsigned int MSG_PeekByteBits( unsigned int numbits );
float MSG_ReadScaledBitValue( unsigned int numbits );
int MSG_ReadSignMagnitude8( int numbits );
short MSG_ReadSignMagnitude16( int numbits );
unsigned int MSG_ReadSignMagnitude32( int numbits );

void MSG_StartBitWriting( sizebuf_t* buf );
void MSG_EndBitWriting( sizebuf_t* buf );
void MSG_WriteOneBit( int value );
void MSG_WriteBitByte( byte* data, int numbits );
void MSG_WriteBitShort( unsigned short* data, int numbits );
void MSG_WriteBitLong( unsigned int* data, int numbits );
void MSG_WriteSBitByte( char* data, int numbits );
void MSG_WriteSBitShort( short* data, int numbits );
void MSG_WriteSBitLong( int* data, int numbits );
void MSG_WriteBitAngle( float angle, int numbits );

//============================================================================

void Q_memset( void* dest, int fill, int count );
void Q_memcpy( void* dest, void* src, int count );
int Q_memcmp( void* m1, void* m2, int count );
void Q_strcpy( char* dest, char* src );
void Q_strncpy( char* dest, char* src, int count );
int Q_strlen( const char* str );
char* Q_strrchr( char* s, char c );
void Q_strcat( char* dest, char* src );
int Q_strcmp( char* s1, char* s2 );
int Q_strncmp( char* s1, char* s2, int count );
int Q_strcasecmp( const char* s1, const char* s2 );
int Q_strncasecmp( const char* s1, const char* s2, int n );
int	Q_atoi( char* str );
float Q_atof( char* str );
int Q_FileNameCmp( char* file1, char* file2 );
int Q_FileNameSuffixCmp( char* suffix, char* filename );
char* COM_BinPrintf( byte* buf, int nLen );

extern qboolean gfExtendedError;
extern char gszDisconnectReason[256];
void COM_ExplainDisconnection( qboolean bPrint, char* format, ... );

//============================================================================

extern	char		com_token[1024];
extern qboolean com_ignorecolons;

char* COM_Parse( char* data );

void COM_HexConvert( char* pszInput, int nInputLength, byte* pOutput );


extern	int		com_argc;
extern	char** com_argv;

int COM_CheckParm( char* parm );
void COM_Init( char* basedir );
void COM_InitArgv( int argc, char** argv );

char* COM_SkipPath( char* pathname );
void COM_StripExtension( char* in, char* out );
char* COM_FileExtension( char* in );
void COM_FileBase( char* in, char* out );
void COM_DefaultExtension( char* path, char* extension );
int COM_FindFile( char *filename, int *handle, FILE **file );
int COM_FileSize( char* filename );
char* COM_StringToLower( char* string );
void COM_FixSlashes( char* pname );
int COM_EntsForPlayerSlots( int nPlayers );

// does a varargs printf into a temp buffer
char* va( char* format, ... );
// prints a vector into a temp buffer.
char* vstr( float* v );


//============================================================================

extern int com_filesize;

extern	char	com_gamedir[MAX_OSPATH];
extern	char	com_gamedirname[64];

// A directory listing entry, as returned by the file system enumerator.
typedef struct FileList_s
{
	int					fileLen;
	int*				handles;
	char				pathID[MAX_OSPATH];
	struct FileList_s*	next;
} FileList_t;

int COM_BuildFileList( char* filename, FileList_t** ppList );
void COM_CloseUnusedFiles( FileList_t* list );
void COM_DestroyMultipleFileList( FileList_t** ppList );

void COM_CreatePath( char* path );
void COM_FreeSearchPath( void );
void COM_SetFileLog( int level );
void COM_LogFileOpen( char* source, char* filename, int offset, int size );
int COM_OpenFile( char* filename, int* hndl );
void COM_LoadFileChunk( char* path, byte* dest, int offset, int length );
int COM_OpenFileByName( char* gamedir, char* filename, int* hndl );
int COM_FOpenFile( char* filename, FILE** file );
void COM_CloseFile( int filepos, int filelen, int handle );

void COM_FreeFile( void* buffer );
void COM_FreeTempFile( void );
byte* COM_LoadFile( char* path, int usehunk, int* pLength );
byte* COM_LoadFileLimit( char* path, int pos, int cbmax, int* pcbread, int* phFile );
byte* COM_LoadFileLimitAsync( char* path, int pos, int cbmax, int* pcbread, int* phFile, byte* dest, struct _OVERLAPPED* pov );
int COM_OpenFileAsync( char* path, int* phFile, struct _OVERLAPPED* pov );
byte* COM_LoadStackFile( char* path, void* buffer, int bufsize );
byte* COM_LoadTempFile( char* path, int* pLength );
byte* COM_LoadHunkFile( char* path );
byte* COM_LoadCacheFile( char* path, struct cache_user_s* cu );
byte* COM_LoadFileForMe( char* path, int* pLength );

void COM_FileSeek( int filepos, int filelen, int handle, int pos );

int Sys_CompareFileTime( int* ft1, int* ft2 );
int COM_CompareFileTime( char* filename1, char* filename2, int* iCompare );

struct resource_s;
qboolean COM_CreateCustomization( struct customization_s* pListHead, struct resource_s* pResource, int playernumber, int flags, struct customization_s** pCustomization, int* nLumps );
void COM_ClearCustomizationList( struct customization_s* pHead, qboolean bCleanDecals );

int COM_FileTell( int filepos, int filelen, int handle, ... );
struct qpic_s* COM_LoadBMP( char* filename, int textureMode, int firstRow, int endRow );
byte* LoadBMP16( void* fin, qboolean is15bit );
void LoadBMP8( int* phFile, byte** pPalette, int* nPalette, byte** pImage );
void COM_Log( char* pszFile, char* fmt, ... );
void COM_ListMaps( char* pszSubString );
#if HLDC_MP
void COM_EnumeratePlayerFiles( void (*callback)(void*, const char*), void* context );
#endif

unsigned char COM_Nibble( char c );
void COM_CheckAuthenticationType( void );
void COM_AddGameDirectory( int flags, char* basedir, char* gamedir );
void COM_ChangeGameDir( char* pszDir );

void COM_GetGameDir( char* szGameDir );



int build_number( void );

extern qboolean		standard_quake, rogue, hipnotic;

void COM_WriteFile( char* filename, void* data, int length );
void COM_CopyFile( char* netpath, char* cachepath );
void COM_CopyFileChunk( void* dest, void* source, int size );
byte* COM_LoadFileLimitIntoBuffer( char* path, int pos, int cbmax, int* pcbread, int* phFile, byte* dest );

#ifdef __cplusplus
}
#endif

#endif // COMMON_H

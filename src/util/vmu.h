// vmu.h -- Visual Memory Unit save-game storage.

#ifndef VMU_H
#define VMU_H

#ifdef __cplusplus
extern "C" {
#endif

// What kind of file the browser found on a card. The menu filters on these so
// a load screen only offers saves the current game can actually read.
#define VMU_FILE_HALFLIFE	2
#define VMU_FILE_BARNEY		4
#define VMU_FILE_OTHER		8
#define VMU_FILE_CONFIG		16

// Result of the last save, and the message the front end shows for it.
#define VMU_SAVE_OK				0
#define VMU_SAVE_WRITEFAILED	1
#define VMU_SAVE_CREATEFAILED	2
#define VMU_SAVE_NOROOM			3
#define VMU_SAVE_QUIET			4

// Bytes the current save needs on the card, and the two strings the card's file
// browser shows for it.
extern int	gSaveGameSize;
extern char	vmuSaveComment[64];
extern char	vmuSaveTitle[16];

// Called once per file the card holds, newest first.
typedef int (*vmuenumproc_t)( char *name, char *description, int type, void *userData );

// GD-ROM door
extern int	g_gdDoorOpened;
extern int	g_gdDoorPending;

void	GDROM_SetDoorBehavior( void );

// Devices on the Maple bus
char*	ES_ErrorTypeToString( int errorCode );
void	VMU_InitDeviceTable( void );
void	VMU_ResetDeviceTable( void );
void	VMU_SetDeviceIconState( int slot, short iconState, short blinkCount );
void	VMU_UpdateDeviceIcons( void );
int		VMU_SelectDeviceIfPresent( int slot );
int		VMU_GetCurrentDevice( void );
int		VMU_IsDevicePresent( int slot );
int		VMU_GetFreeBlocks( void );

// Files on the card
unsigned int	VMU_MakeShortName( char *path, char *shortName );
unsigned int	VMU_GetFileDescription( struct IEsDevice *device, char *fileName, char *descOut );
unsigned int	VMU_DeleteFile( char *fileName );
int				VMU_EnumFiles( vmuenumproc_t callback, void *userData );
int				VMU_CreateFile( char *fileName, unsigned int blockCount );

// Saving and loading
int		VMU_SaveGameHL4( char *saveName );
int		VMU_SaveGameHL1( char *saveName );
void	VMU_RemoveSave( char *saveName );
void	VMU_FormatSlotName( char *saveName );
char*	VMU_MarkSlotSaved( void );
void	VMU_SetSaveResult( int result );
int		VMU_GetSaveResult( void );

int		Host_SaveGameSize( void );
int		Host_SaveGameSizeHL1( char *saveName );
char*	Host_FindRecentSave( void );

int		FileExists( char *path );

void	Cmd_dumpvmu_f( void );

#ifdef __cplusplus
}
#endif

#endif // VMU_H

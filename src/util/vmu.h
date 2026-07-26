// vmu.h -- Visual Memory Unit save-game storage.

#ifndef VMU_H
#define VMU_H

#ifdef __cplusplus
extern "C" {
#endif

// Bytes the current save occupies on the card, and the two strings the card's
// file browser shows for it.
extern int	gSaveGameSize;
extern char	vmuSaveComment[64];
extern char	vmuSaveTitle[16];

// Device table
void	GDROM_SetDoorBehavior( void );
char*	ES_ErrorTypeToString( int errorCode );
void	VMU_InitDeviceTable( void );
void	VMU_ResetDeviceTable( void );
void	VMU_SetDeviceIconState( int slot, short iconState, short blinkCount );
void	VMU_UpdateDeviceIcons( void );
int		VMU_SelectDeviceIfPresent( int slot );
int		VMU_IsDevicePresent( int slot );
void	VMU_SetCurrentDevice( int device );
int		VMU_GetCurrentDevice( void );
void*	VMU_OpenDevice( void );
int		VMU_GetFreeBlocks( void );

// Files on the card
unsigned int	VMU_MakeShortName( char* path, char* shortName );
unsigned int	VMU_GetFileDescription( void* device, char* fileName, char* descOut );
unsigned int	VMU_DeleteFile( char* fileName );
int				VMU_WriteBlockRetry( void* file, unsigned int offset, unsigned int length, void* data );
int				VMU_EnumFiles( void* callback, void* userData );
unsigned char	VMU_CreateFile( char* fileName, unsigned int blockCount );
int				VMU_ClearDescription( int desc );
void			VMU_FreeDescription( int desc );

// Saving and loading
unsigned int	VMU_SaveGameHL2( void* gameState, char* fileName, int dataLen, void* extraData );
unsigned int	VMU_SaveGameHL3( void* gameState, char* fileName, unsigned int dataLen, void* extraData );
int				VMU_SaveGameHL4( char* saveName );
int				VMU_SaveGameHL1( char* saveName );
int				VMU_LoadGameHL4( char* saveName, void* device );
int				VMU_LoadGameHL1( char* saveName, void* device );
void			VMU_LoadGameHL4_Thunk( char* saveName );
int				VMU_ReadFileToHandle( char* slotName, void* device, int length );
int				VMU_ReadFileToHandle_Impl( void* device, int length );
void			VMU_FormatSlotName( char* saveName );
char*			VMU_MarkSlotSaved( void );

int		Host_SaveGameSize( void );
int		Host_SaveGameSizeHL1( char* saveName );
char*	Host_FindRecentSave( void );

int		FileExists( char* path );

void	Cmd_dumpvmu_f( void );

#ifdef __cplusplus
}
#endif

#endif // VMU_H

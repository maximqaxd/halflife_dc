// vmu.c -- Visual Memory Unit save-game storage. Talks to the memory cards over the
// Maple bus: enumerating devices, reading and writing save blocks, and managing the
// GD-ROM door so a save can span discs.

#include "quakedef.h"

// Open \Device\CDROM0 and lock the GD-ROM door closed.
void GDROM_SetDoorBehavior( void )
{
}

// Return a human-readable string for an ELF/VMU device error code.
char *ES_ErrorTypeToString( int errorCode )
{
	return NULL;
}

// Scan the Maple bus and build the table of attached memory-card devices.
void VMU_InitDeviceTable( void )
{
}

// Clear the per-slot device table and re-enumerate memory cards on the Maple bus.
void VMU_ResetDeviceTable( void )
{
}

// Set the LCD icon state and blink counter for the card in the given slot.
void VMU_SetDeviceIconState( int slot, short iconState, short blinkCount )
{
}

// Refresh the LCD save-in-progress icons on all attached memory cards.
void VMU_UpdateDeviceIcons( void )
{
}

// Derive a 12-character VMU file name from a full path, space-padded.
unsigned int VMU_MakeShortName( char *path, char *shortName )
{
	return 0;
}

// Look up a file on the device and fill in its description and block count.
unsigned int VMU_GetFileDescription( void *device, char *fileName, char *descOut )
{
	return 0;
}

// Create a Maple device object for the first available memory card.
void *VMU_OpenDevice( void )
{
	return NULL;
}

// Return the number of free blocks on the memory card.
int VMU_GetFreeBlocks( void )
{
	return 0;
}

// Delete a file from the memory card.
unsigned int VMU_DeleteFile( char *fileName )
{
	return 0;
}

// Write one block to a file, retrying a few times on error.
int VMU_WriteBlockRetry( void *file, unsigned int offset, unsigned int length, void *data )
{
	return 0;
}

// Write a saved game to the memory card (header + payload + footer).
unsigned int VMU_SaveGameHL2( void *gameState, char *fileName, int dataLen, void *extraData )
{
	return 0;
}

// Write a saved game to the memory card (header + payload variant).
unsigned int VMU_SaveGameHL3( void *gameState, char *fileName, unsigned int dataLen, void *extraData )
{
	return 0;
}

// Enumerate saved files on the card, sort them, and invoke a callback for each.
int VMU_EnumFiles( void *callback, void *userData )
{
	return 0;
}

// Create a new Half-Life save file on the memory card with icon and header.
unsigned char VMU_CreateFile( char *fileName, unsigned int blockCount )
{
	return 0;
}

// Select the VMU device in the given slot if a card is present there.
int VMU_SelectDeviceIfPresent( int slot )
{
	return 0;
}

// Test whether a VMU card is inserted in the given slot.
int VMU_IsDevicePresent( int slot )
{
	return 0;
}

// Build the short VMU file name for the given slot into the shared buffer.
void VMU_FormatSlotName( int slot )
{
}

// Compute the storage size needed for the HL4/HL1 save-game files.
int Host_SaveGameSize( void )
{
	return 0;
}

// Set the current active VMU device index.
void VMU_SetCurrentDevice( int device )
{
}

// Mark the current slot as having a saved game and return its device handle.
void *VMU_MarkSlotSaved( void )
{
	return NULL;
}

// Return the current active VMU device index.
int VMU_GetCurrentDevice( void )
{
	return 0;
}

// Dump a Half-Life (HL4-format) save game to the VMU.
int VMU_SaveGameHL4( char *saveName )
{
	return 0;
}

// Dump a Half-Life (HL1-format) save game to the VMU.
int VMU_SaveGameHL1( char *saveName )
{
	return 0;
}

// Compute the total size of the HL1 save-game data.
int Host_SaveGameSizeHL1( void )
{
	return 0;
}

// Copy a VMU file's contents into an open engine file handle.
int VMU_ReadFileToHandle_Impl( void *device, int length )
{
	return 0;
}

// Load a Half-Life (HL4-format) save game from the VMU.
int VMU_LoadGameHL4( char *saveName, void *device )
{
	return 0;
}

// Test whether the named save game exists on the VMU.
int FileExists( char *path )
{
	return 0;
}

// Thunk to the HL4 load path.
void VMU_LoadGameHL4_Thunk( void )
{
}

// Read a VMU file into a PC-side file, translating the CRC/header.
int VMU_ReadFileToHandle( char *slotName, void *device, int length )
{
	return 0;
}

// Load a Half-Life (HL1-format) save game from the VMU.
int VMU_LoadGameHL1( char *saveName, void *device )
{
	return 0;
}

// Console command: enumerate and dump save games on the VMU.
void Cmd_dumpvmu_f( void )
{
}

// Reset the description pointer on a VMU file entry.
int VMU_ClearDescription( int desc )
{
	return 0;
}

// Free a VMU file's description string.
void VMU_FreeDescription( int desc )
{
}

// vmu.cpp -- Visual Memory Unit save-game storage. Talks to the memory cards over the
// Maple bus: enumerating devices, reading and writing save blocks, driving the little
// LCD on the front of the card, and holding the GD-ROM door shut while a save runs.

#include "quakedef.h"
#include <mapledev.h>
#include <ceddcdrm.h>
#include <segagdrm.h>
#include <lcd.h>
#include <initguid.h>
#include "esfile.h"
#include "kzap.h"
#include "vmu.h"

// The card structures are far too big to copy inline.
#pragma function( memset, memcpy )


#define VMU_MAX_DEVICES		8		// two slots on each of the four ports
#define VMU_MAX_FILES		100		// files the browser will list off one card

// LCD icon the card is showing, or has been asked to show.
#define VMU_ICON_IDLE		1
#define VMU_ICON_BUSY		2
#define VMU_ICON_SAVED		3
#define VMU_ICON_FAILED		4

#define VMU_LCD_BYTES		192		// 48x32 monochrome

// Signatures the first four bytes of a file carry, so a card written by one game
// can be told apart from one written by another.
#define VMU_SIG_HALFLIFE	"HLS3"
#define VMU_SIG_BARNEY		"BSS3"
#define VMU_SIG_CONFIG		"HLC2"

// A save is stored as its four byte signature, its uncompressed length, the
// compressed payload, and the browser header the card itself reads.
#define VMU_PAYLOAD_OFFSET	8

#define VMU_SAVE_OVERHEAD	136		// signature, length and browser header
#define VMU_SAVE_SLACK		1024	// room the compressor may need
#define VMU_BYTES_PER_SAVE	9216	// budgeted for each save already on disc
#define VMU_MAX_SAVE_BYTES	97280	// never ask for more of the card than this

// Chatter about the card is compiled out of release builds, but the status
// lookup still runs so a failure is always decoded.
#define VMU_Printf	(void)

// One memory card. The card answers as two Maple devices - the flash storage and
// the LCD - so both instances are kept, along with what the LCD is showing.
typedef struct
{
	int					present;
	MAPLEDEVICEINSTANCE	storage;
	MAPLEDEVICEINSTANCE	lcd;
	short				hasLcd;
	short				blinkCount;
	short				iconShown;
	short				iconWanted;
} vmudevice_t;

// One file as the browser lists it. The stamp is the card's own creation time,
// most significant field first, so files sort newest to oldest field by field.
class CVmuFile
{
public:
					CVmuFile( void );
					~CVmuFile( void );

	void			SetDescription( char *text )
					{
						if (description)
						{
							MnemoFree( description );
							description = NULL;
						}

						description = (char *)MnemoAlloc( strlen( text ) + 1, MNEMO_FLAG_MALLOC,
														 0, "VMU description" );
						strcpy( description, text );
					}

	char			name[16];
	char			*description;
	esstamp_t		stamp;
	int				index;
	int				type;
};

// Door state shared with the low-level GD-ROM driver.
int					g_gdDoorOpened;
int					g_gdDoorPending;

static int			vmuCurrentDevice;

#include "vmu_icons.inc"

static int			vmuFileCount;

// Bytes the save needs on the card, and how much of the card the front end was
// told to reserve for it.
int					gSaveGameSize;
static int			vmuReservedBytes;

static char			*vmuSaveResultText[] =
{
	"%gamesavedok",
	"%gamesavefailederror",
	"%gamesavefailederror",
	"%gamesavefailed",
	NULL
};

static int			vmuSaveResult;

static ES_FILEINFO	vmuBrowseInfo;
static CVmuFile		vmuFiles[VMU_MAX_FILES];
static unsigned int	vmuFileHandle;

// Save the player last picked, as "@<slot>@<name>".
static char			vmuRecentSave[32];

// The descriptor written on the tail of every save the game creates. The card's
// browser shows the comment; the front end reads the title back for its own list.
char				vmuSaveComment[64];
char				vmuSaveTitle[16];
static char			vmuSaveTail[48];

static ES_FILEINFO	vmuFindInfo;
static vmudevice_t	vmuDevices[VMU_MAX_DEVICES];

static IEsDevice *VMU_OpenDevice( void );

/*
==================
GDROM_SetDoorBehavior

Lock the disc tray shut. Ejecting mid-save would take the level files away
underneath the save code, so the door stays down until the save finishes.
==================
*/
void GDROM_SetDoorBehavior( void )
{
	HANDLE	h;
	DWORD	behavior;
	DWORD	returned;
	DWORD	err;

	h = CreateFile( TEXT("\\Device\\CDROM0"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
	if (h != INVALID_HANDLE_VALUE)
	{
		// Reboot the console on door open, so a disc swap cannot be missed.
		behavior = 1;
		if (!DeviceIoControl( h, IOCTL_SEGACD_SET_DOOR_BEHAVIOR, &behavior, sizeof(behavior),
							  NULL, 0, &returned, NULL ))
		{
			err = GetLastError();
			if (err != ERROR_NO_MEDIA_IN_DRIVE)
				Sys_Error( "Error setting GD-ROM door behavior (0x%08x).\n", err );
		}

		CloseHandle( h );
	}

	g_gdDoorPending = 1;
}

/*
==================
GDROM_ConfigureDoorBehavior

Let the tray open again. If the door was already open the console has to go back
to the firmware for the disc swap to mean anything.
==================
*/
void GDROM_ConfigureDoorBehavior( void )
{
	HANDLE	h;
	DWORD	behavior;
	DWORD	returned;
	DWORD	err;

	h = CreateFile( TEXT("\\Device\\CDROM0"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
	if (h != INVALID_HANDLE_VALUE)
	{
		// Notify the app instead of rebooting. A missing disc is fine here;
		// the door flow below deals with it.
		behavior = 0;
		if (!DeviceIoControl( h, IOCTL_SEGACD_SET_DOOR_BEHAVIOR, &behavior, sizeof(behavior),
							  NULL, 0, &returned, NULL ))
		{
			err = GetLastError();
			if (err != ERROR_NO_MEDIA_IN_DRIVE)
				Sys_Error( "Error setting GD-ROM door behavior (0x%08x).\n", err );
		}

		CloseHandle( h );
	}

	g_gdDoorPending = 0;

	if (g_gdDoorOpened)
		GDROM_DoorReset();
}

/*
==================
ES_ErrorTypeToString

Plain text for a status the flash driver handed back.
==================
*/
char *ES_ErrorTypeToString( int errorCode )
{
	switch (errorCode)
	{
	case ES_ACCESSDENIED:		return "access denied";
	case ES_OUTOFMEMORY:		return "out of memory";
	case ES_INVALIDPARAMETER:	return "invalid parameter";
	case ES_PERMANENTBADBLOCK:	return "permanent bad block";
	case ES_BADFILECRC:			return "bad file CRC";
	case ES_DEVICECORRUPTED:	return "device corrupted";
	case ES_FILECORRUPTED:		return "file corrupted";
	case ES_BADFILENAME:		return "bad filename";
	case ES_UNKNOWNDEVICEERROR:	return "unknown device error";
	case ES_UNFORMATTEDDEVICE:	return "unformatted device";
	case ES_DEVICENOTFOUND:		return "device not found";
	case ES_FILEEXISTS:			return "file exists";
	case ES_FILENOTFOUND:		return "file not found";
	case ES_NOTENOUGHSPACE:		return "not enough space";
	case ES_UNKNOWNERROR:		return "unknown error";
	case ES_UNKNOWNDEVICETYPE:	return "unknown device type";
	case ES_UNKNOWNFILETYPE:	return "unknown file type";
	case ES_ENDOFFILE:			return "end of file";
	case ES_SUCCESS:			return "success";
	}

	return "Too damn many things";
}

/*
==================
CVmuFile::CVmuFile
==================
*/
CVmuFile::CVmuFile( void )
{
	description = NULL;
}

/*
==================
CVmuFile::~CVmuFile
==================
*/
CVmuFile::~CVmuFile( void )
{
	if (description)
		MnemoFree( description );
}

/*
==================
VMU_SetFileDescription
==================
*/
static __inline void VMU_SetFileDescription( int index, char *text )
{
	vmuFiles[index].SetDescription( text );
}

/*
==================
VMU_StorageCallback

A card's flash device showed up. Ports and device numbers are one based, so
port 0 device 1 is slot 0, port 0 device 2 is slot 1, and so on.
==================
*/
static BOOL PASCAL VMU_StorageCallback( LPCMAPLEDEVICEINSTANCE mdi, LPVOID context )
{
	int	slot;

	slot = mdi->dwPort * 2 + mdi->dwDevNum - 1;
	if (slot < 0 || slot >= VMU_MAX_DEVICES)
		Sys_Error( "Surprising VMU index: port: %d, devnum: %d, index: %d\n", slot );

	vmuDevices[slot].present = 1;
	memcpy( &vmuDevices[slot].storage, mdi, sizeof(MAPLEDEVICEINSTANCE) );

	return TRUE;
}

/*
==================
VMU_LcdCallback

The LCD half of a card. Showing the idle icon right away is how the player can
tell the game noticed the card go in.
==================
*/
static BOOL PASCAL VMU_LcdCallback( LPCMAPLEDEVICEINSTANCE mdi, LPVOID context )
{
	int	slot;

	slot = mdi->dwPort * 2 + mdi->dwDevNum - 1;
	if (slot < 0 || slot >= VMU_MAX_DEVICES)
		Sys_Error( "Surprising VMU LCD index: port: %d, devnum: %d, index: %d\n", slot );

	vmuDevices[slot].hasLcd = 1;
	vmuDevices[slot].blinkCount = 0;
	vmuDevices[slot].iconWanted = VMU_ICON_IDLE;
	memcpy( &vmuDevices[slot].lcd, mdi, sizeof(MAPLEDEVICEINSTANCE) );

	return TRUE;
}

/*
==================
VMU_ResetDeviceTable

Forget every card and walk the bus again.
==================
*/
void VMU_ResetDeviceTable( void )
{
	int	i;

	for (i = 0; i < VMU_MAX_DEVICES; i++)
	{
		vmuDevices[i].present = 0;
		vmuDevices[i].hasLcd = 0;
		vmuDevices[i].blinkCount = 0;
		vmuDevices[i].iconShown = 0;
		vmuDevices[i].iconWanted = 0;
	}

	MapleEnumerateDevices( MDT_STORAGE, VMU_StorageCallback, NULL, 0 );
	MapleEnumerateDevices( MDT_LCD, VMU_LcdCallback, NULL, 0 );
}

/*
==================
VMU_SelectDeviceIfPresent

Point the save code at a particular card, if there is one in that slot.
==================
*/
int VMU_SelectDeviceIfPresent( int slot )
{
	if (!vmuDevices[slot].present)
		return 0;

	vmuCurrentDevice = slot;
	return 1;
}

int VMU_GetCurrentDevice( void )
{
	return vmuCurrentDevice;
}

/*
==================
VMU_SelectFirstDevice

Point at the lowest numbered card that answered, or -1 if there are none.
==================
*/
__inline int VMU_SelectFirstDevice( void )
{
	int i;
	int selected;

	vmuCurrentDevice = -1;
	for (i = 0; i < VMU_MAX_DEVICES; i++)
	{
		selected = 0;
		if (vmuDevices[i].present)
		{
			vmuCurrentDevice = i;
			selected = 1;
		}
		if (selected)
			return i;
	}
	return vmuCurrentDevice;
}

/*
==================
VMU_MountDevice
==================
*/
static __inline qboolean VMU_MountDevice( IEsDevice *device )
{
	int	hr;

	if (!device)
		return false;

	hr = device->lpVtbl->Mount( device );
	if (hr != ES_SUCCESS)
	{
		VMU_Printf( "VMU error: %s\n", ES_ErrorTypeToString( hr ) );
		return false;
	}

	return true;
}

/*
==================
VMU_DeviceFreeBytes

Room left on a card that is already open.
==================
*/
__inline int VMU_DeviceFreeBytes( IEsDevice *device )
{
	int	hr;

	if (!device)
		return 0;

	if (!VMU_MountDevice( device ))
		return 0;

	{
		ES_MEDIAINFO	info = { 0 };

		info.dwSize = sizeof(info);
		info.dwMaxFiles = 255;

		hr = device->lpVtbl->GetMediaInfo( device, &info );
		if (hr != ES_SUCCESS)
		{
			VMU_Printf( "VMU error: %s\n", ES_ErrorTypeToString( hr ) );
			return 0;
		}

		return info.dwFreeBlocks * info.dwBlockSize;
	}
}

__inline int VMU_CheckResult( int result )
{
	if (result != ES_SUCCESS)
		ES_ErrorTypeToString( result );
	return result;
}

__inline void VMU_ReleaseDevice( IEsDevice *device )
{
	if (device)
		device->lpVtbl->Release( device );
}

__inline bool VMU_DeviceHasSpace( IEsDevice *device, char *fileName, int required )
{
	if (!device)
		return false;
	int free = VMU_DeviceFreeBytes( device );
	return required <= free + (int)VMU_GetFileDescription( device, fileName, NULL );
}

/*
==================
VMU_InitDeviceTable

Scan the bus and take a look at whatever card came back first, so the amount of
free space is known before the player ever opens the memory card screen.
==================
*/
void VMU_InitDeviceTable( void )
{
	IEsDevice	*device;
	int			free;

	VMU_ResetDeviceTable();
	VMU_SelectFirstDevice();

	device = VMU_OpenDevice();
	if (!device)
		return;

	free = VMU_DeviceFreeBytes( device );
	free += VMU_GetFileDescription( device, "bogus", NULL );

	VMU_ReleaseDevice( device );

	vmuRecentSave[0] = 0;
}

/*
==================
VMU_SetDeviceIconState

Ask a card's LCD for a picture. A non-zero blink count makes it revert to the
idle icon that many frames later.
==================
*/
void VMU_SetDeviceIconState( int slot, short iconState, short blinkCount )
{
	vmuDevices[slot].iconWanted = iconState;
	vmuDevices[slot].blinkCount = blinkCount;
}

/*
==================
VMU_UpdateDeviceIcons

Push a new picture to any card whose LCD is out of date. Called once a frame,
so the busy icon appears while a save is still running.
==================
*/
void VMU_UpdateDeviceIcons( void )
{
	int				i;
	short			wanted;
	const byte		*bits;
	IUnknown		*unknown;
	PLCD			lcd;
	BYTE			*buffer;
	DWORD			bufId;

	for (i = 0; i < VMU_MAX_DEVICES; i++)
	{
		if (!vmuDevices[i].present || !vmuDevices[i].hasLcd)
			continue;

		if (vmuDevices[i].blinkCount && --vmuDevices[i].blinkCount == 0)
			vmuDevices[i].iconWanted = VMU_ICON_IDLE;

		wanted = vmuDevices[i].iconWanted;
		if (vmuDevices[i].iconShown == wanted)
			continue;

		vmuDevices[i].iconShown = wanted;

		switch (wanted)
		{
		case VMU_ICON_IDLE:		bits = vmuLcdIdle;		break;
		case VMU_ICON_BUSY:		bits = vmuLcdBusy;		break;
		case VMU_ICON_SAVED:	bits = vmuLcdSaved;		break;
		case VMU_ICON_FAILED:	bits = vmuLcdFailed;	break;
		default:				bits = NULL;			break;
		}

		if (MapleCreateDevice( &vmuDevices[i].lcd.guidDevice, &unknown ) != MD_OK)
		{
			VMU_Printf( "Can't create VMU device for LCD\n" );
			return;
		}

		lcd = NULL;
		unknown->QueryInterface( IID_ILcd, (void **)&lcd );
		unknown->Release();

		if (lcd && ILcd_IsStandardLcd( lcd )
			&& ILcd_GetLcdBuffer( lcd, &buffer, &bufId, VMU_LCD_BYTES ) == ES_SUCCESS)
		{
			memcpy( buffer, bits, VMU_LCD_BYTES );
			ILcd_SendLcdBuffer( lcd, bufId, 0, 0, 0, NULL );
		}

		if (lcd)
			ILcd_Release( lcd );
	}
}

/*
==================
VMU_MakeShortName

Cards only hold twelve character names, and the browser expects the unused
positions to be blanks rather than nulls.
==================
*/
unsigned int VMU_MakeShortName( char *path, char *shortName )
{
	char	*p;
	char	*out;
	int		i;

	p = strrchr( path, '/' );
	if (!p)
		p = strrchr( path, '\\' );
	if (p)
		path = p + 1;

	out = shortName;
	for (i = 0; i < ES_MAX_FILENAME; i++)
	{
		if (*path)
			*out = *path++;
		else
			*out = ' ';
		out++;
	}

	shortName[ES_MAX_FILENAME - 1] = 0;

	return 1;
}

/*
==================
VMU_FindFileCallback

Stop on the entry whose name matches the one already sitting in the context,
keeping the whole directory record and the handle needed to open it.
==================
*/
static int VMU_FindFileCallback( IEsDevice *device, unsigned int hFile,
								 ES_FILEINFO *info, ES_FILEINFO *match )
{
	if (strcmp( info->szName, match->szName ) != 0)
		return TRUE;

	memcpy( match, info, sizeof(ES_FILEINFO) );
	vmuFileHandle = hFile;

	return FALSE;
}

/*
==================
VMU_FindFile

Look a file up by name. The record comes back in a shared buffer, so it is only
good until the next lookup.
==================
*/
static __inline ES_FILEINFO *VMU_FindFile( IEsDevice *device, char *fileName )
{
	if (!device)
		return NULL;

	memset( &vmuFindInfo, 0, sizeof(vmuFindInfo) );
	strcpy( vmuFindInfo.szName, fileName );

	device->lpVtbl->EnumFiles( device, (ESENUMFILEPROC)VMU_FindFileCallback, &vmuFindInfo );

	if (!vmuFindInfo.dwValid)
		return NULL;

	return &vmuFindInfo;
}

/*
==================
VMU_GetFileDescription

Size of the named file, and optionally the text the card's own browser shows
for it. The description lives in the header on the tail of the file.
==================
*/
unsigned int VMU_GetFileDescription( IEsDevice *device, char *fileName, char *descOut )
{
	ES_FILEINFO	*info;
	IEsFile		*file;
	char		blocks[32];
	int			hr;

	info = VMU_FindFile( device, fileName );

	if (!device)
		return 0;

	if (!info)
		return 0;

	if (descOut)
	{
		strcpy( descOut, "No description available" );

		hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
		if (hr != ES_SUCCESS)
		{
			VMU_Printf( "Can't get VMU device description.\n", ES_ErrorTypeToString( hr ) );
		}
		else
		{
			file->lpVtbl->Read( file, info->dwSize - ES_HEADER_SIZE, ES_HEADER_SIZE, descOut );
			file->lpVtbl->Close( file );
			file->lpVtbl->Release( file );
		}

		sprintf( blocks, " [%d blocks]", info->dwBlocks );
		strcat( descOut, blocks );
	}

	return info->dwSize;
}

/*
==================
VMU_OpenDevice

Flash interface for the selected card, picking the first card that answered if
nothing has been selected yet.
==================
*/
static IEsDevice *VMU_OpenDevice( void )
{
	IUnknown	*unknown;
	IEsDevice	*device;

	if (vmuCurrentDevice == -1 && VMU_SelectFirstDevice() == -1)
		return NULL;

	if (MapleCreateDevice( &vmuDevices[vmuCurrentDevice].storage.guidDevice, &unknown ) != MD_OK)
	{
		VMU_Printf( "Can't create VMU device\n" );
		return NULL;
	}

	device = NULL;
	unknown->QueryInterface( IID_IEsDevice, (void **)&device );
	unknown->Release();

	return device;
}

bool VMU_ReadDescription( char *fileName, char *description )
{
	IEsDevice *device = VMU_OpenDevice();
	if (!device)
		return false;
	int size = VMU_GetFileDescription( device, fileName, description );
	VMU_ReleaseDevice( device );
	return size != 0;
}

__inline bool VMU_HasSpace( char *fileName, int required )
{
	IEsDevice *device = VMU_OpenDevice();
	if (!device)
		return false;
	bool room = VMU_DeviceHasSpace( device, fileName, required );
	VMU_ReleaseDevice( device );
	return room;
}

/*
==================
VMU_GetFreeBlocks

Bytes still going spare on the selected card.
==================
*/
int VMU_GetFreeBlocks( void )
{
	IEsDevice	*device;
	int			free;

	device = VMU_OpenDevice();
	if (!device)
		return 0;

	free = VMU_DeviceFreeBytes( device );

	VMU_ReleaseDevice( device );

	return free;
}

/*
==================
VMU_DeleteFile

Throw a file off the card.
==================
*/
unsigned int VMU_DeleteFile( char *fileName )
{
	IEsDevice		*device;
	IEsFile			*file;
	ES_FILEINFO		*info;
	char			shortName[20];
	int				hr;
	unsigned int	result;

	result = 0;

	VMU_MakeShortName( fileName, shortName );

	device = VMU_OpenDevice();
	if (device)
	{
		info = VMU_FindFile( device, shortName );

		if (info)
		{
			hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
			if (hr != ES_SUCCESS)
			{
				VMU_Printf( "File %s was found but couldn't be deleted.\n",
							ES_ErrorTypeToString( hr ) );
			}
			else
			{
				file->lpVtbl->Delete( file );
				file->lpVtbl->Release( file );
				result = 1;
			}
		}

		VMU_ReleaseDevice( device );
	}

	return result;
}

/*
==================
VMU_WriteBlockRetry

Flash writes fail often enough that one bad block is worth retrying before the
save is called off.
==================
*/
static int VMU_WriteBlockRetry( IEsFile *file, unsigned int offset, unsigned int length, void *data )
{
	int	hr;
	int	tries;

	tries = 4;
	do
	{
		hr = file->lpVtbl->Write( file, offset, length, data );
		if (hr == ES_SUCCESS)
			return ES_SUCCESS;

		VMU_Printf( "VMU write failed: %s\n", ES_ErrorTypeToString( hr ) );
	} while (tries--);

	VMU_Printf( "VMU write failed: %s\n", ES_ErrorTypeToString( hr ) );

	return hr;
}

/*
==================
VMU_WriteSaveGame

Signature, uncompressed length, payload, and the browser header the card reads
off the tail.
==================
*/
static unsigned int VMU_WriteSaveGame( char *sourcePath, char *fileName, int dataLen, int rawLen )
{
	IEsDevice		*device;
	IEsFile			*file;
	ES_FILEINFO		*info;
	char			signature[5];
	void			*data;
	int				hr;
	unsigned int	result;

	strcpy( signature, VMU_SIG_HALFLIFE );
	result = 0;

	if (!strcmp( com_gamedirname, "barney" ))
		strcpy( signature, VMU_SIG_BARNEY );

	device = VMU_OpenDevice();
	if (!device)
		return 0;

	info = VMU_FindFile( device, fileName );

	if (info)
	{
		hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
		if (hr != ES_SUCCESS)
		{
			VMU_Printf( "Can't get VMU device interface\n", ES_ErrorTypeToString( hr ) );
		}
		else
		{
			data = Bfileptr_path( sourcePath );
			if (data)
			{
				if (VMU_WriteBlockRetry( file, 0, 4, signature ) == ES_SUCCESS
					&& VMU_WriteBlockRetry( file, 4, 4, &rawLen ) == ES_SUCCESS
					&& VMU_WriteBlockRetry( file, VMU_PAYLOAD_OFFSET, dataLen, data ) == ES_SUCCESS
					&& VMU_WriteBlockRetry( file, dataLen + VMU_PAYLOAD_OFFSET, ES_HEADER_SIZE,
											vmuSaveComment ) == ES_SUCCESS)
				{
					result = 1;
				}

				file->lpVtbl->Close( file );
				file->lpVtbl->Release( file );
			}
			else
			{
				VMU_Printf( "Can't get raw buffer from Bfile %s\n", sourcePath );
			}
		}
	}

	VMU_ReleaseDevice( device );

	return result;
}

/*
==================
VMU_WriteConfig

Same shape as a save game, but the settings file carries no browser header.
==================
*/
static unsigned int VMU_WriteConfig( char *sourcePath, char *fileName, unsigned int dataLen )
{
	IEsDevice		*device;
	IEsFile			*file;
	ES_FILEINFO		*info;
	char			signature[5];
	void			*data;
	int				hr;
	unsigned int	result;

	strcpy( signature, VMU_SIG_CONFIG );
	result = 0;

	device = VMU_OpenDevice();
	if (!device)
		return 0;

	info = VMU_FindFile( device, fileName );

	if (info)
	{
		hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
		if (hr != ES_SUCCESS)
		{
			VMU_Printf( "Can't get VMU device interface\n", ES_ErrorTypeToString( hr ) );
		}
		else
		{
			data = Bfileptr_path( sourcePath );
			if (data)
			{
				if (VMU_WriteBlockRetry( file, 0, 4, signature ) == ES_SUCCESS
					&& VMU_WriteBlockRetry( file, 4, 4, &dataLen ) == ES_SUCCESS
					&& VMU_WriteBlockRetry( file, VMU_PAYLOAD_OFFSET, dataLen, data ) == ES_SUCCESS)
				{
					result = 1;
				}

				file->lpVtbl->Close( file );
				file->lpVtbl->Release( file );
			}
			else
			{
				VMU_Printf( "Can't get raw buffer from Bfile %s\n", sourcePath );
			}
		}
	}

	VMU_ReleaseDevice( device );

	return result;
}

/*
==================
VMU_SortFileCompare

Newest first: the card stamps every file with century, year, month, day, hour
and minute, and files written in the same minute keep the order they came back
off the card in.
==================
*/
static int VMU_SortFileCompare( const void *a, const void *b )
{
	CVmuFile	*fa;
	CVmuFile	*fb;
	int			diff;

	fa = (CVmuFile *)a;
	fb = (CVmuFile *)b;

	diff = fb->stamp.b[0] - fa->stamp.b[0];
	if (diff)
		return diff;

	diff = fb->stamp.b[1] - fa->stamp.b[1];
	if (diff)
		return diff;

	diff = fb->stamp.b[2] - fa->stamp.b[2];
	if (diff)
		return diff;

	diff = fb->stamp.b[3] - fa->stamp.b[3];
	if (diff)
		return diff;

	diff = fb->stamp.b[4] - fa->stamp.b[4];
	if (diff)
		return diff;

	diff = fb->stamp.b[5] - fa->stamp.b[5];
	if (diff)
		return diff;

	return fb->index - fa->index;
}

__inline void VMU_SortFiles( void )
{
	qsort( vmuFiles, vmuFileCount, sizeof(CVmuFile), VMU_SortFileCompare );
}

/*
==================
VMU_ListFileCallback

Add one file to the browse list, working out from its signature whether this
game can read it.
==================
*/
static int VMU_ListFileCallback( IEsDevice *device, unsigned int hFile,
								 ES_FILEINFO *info, void *context )
{
	IEsFile	*file;
	char	signature[5];
	char	blocks[32];
	char	description[200];
	int		hr;

	if (vmuFileCount >= VMU_MAX_FILES)
		return FALSE;

	strcpy( vmuFiles[vmuFileCount].name, info->szName );
	vmuFiles[vmuFileCount].SetDescription( "No description available" );
	vmuFiles[vmuFileCount].stamp = info->stamp;
	vmuFiles[vmuFileCount].index = vmuFileCount;

	hr = device->lpVtbl->OpenFile( device, &file, hFile );
	if (hr != ES_SUCCESS)
	{
		VMU_Printf( "VMU error: %s\n", ES_ErrorTypeToString( hr ) );
	}
	else
	{
		file->lpVtbl->Read( file, 0, 4, signature );
		signature[4] = 0;

		if (!strcmp( signature, VMU_SIG_HALFLIFE ))
		{
			file->lpVtbl->Read( file, info->dwSize - ES_HEADER_SIZE, ES_HEADER_SIZE, description );
			vmuFiles[vmuFileCount].type = VMU_FILE_HALFLIFE;
		}
		else if (!strcmp( signature, VMU_SIG_BARNEY ))
		{
			file->lpVtbl->Read( file, info->dwSize - ES_HEADER_SIZE, ES_HEADER_SIZE, description );
			vmuFiles[vmuFileCount].type = VMU_FILE_BARNEY;
		}
		else if (!strcmp( signature, VMU_SIG_CONFIG ))
		{
			sprintf( description, "(Config File)", info->szName );
			vmuFiles[vmuFileCount].type = VMU_FILE_CONFIG;
		}
		else
		{
			sprintf( description, "(Non-Halflife File: %s)", info->szName );
			vmuFiles[vmuFileCount].type = VMU_FILE_OTHER;
		}

		file->lpVtbl->Close( file );
		file->lpVtbl->Release( file );
	}

	sprintf( blocks, " [%d blocks]", info->dwBlocks );
	strcat( description, blocks );

	vmuFiles[vmuFileCount].SetDescription( description );
	vmuFileCount++;

	return TRUE;
}

/*
==================
VMU_ClearFileList
==================
*/
static __inline void VMU_ClearFileList( void )
{
	int	i;

	vmuFileCount = sizeof(CVmuFile);

	for (i = 0; i < VMU_MAX_FILES; i++)
	{
		strcpy( vmuFiles[i].name, "---" );
		vmuFiles[i].SetDescription( "---" );
		vmuFiles[i].type = VMU_FILE_OTHER;
	}

	vmuFileCount = 0;
}

/*
==================
VMU_EnumFiles

Walk everything on the selected card, newest first, handing each file to the
caller until it says stop.
==================
*/
int VMU_EnumFiles( vmuenumproc_t callback, void *userData )
{
	IEsDevice	*device;
	int			i;
	int			hr;

	device = VMU_OpenDevice();
	if (!device)
		return 0;

	VMU_ClearFileList();

	memset( &vmuBrowseInfo, 0, sizeof(vmuBrowseInfo) );
	strcpy( vmuBrowseInfo.szName, "!!" );

	hr = device->lpVtbl->EnumFiles( device, VMU_ListFileCallback, &vmuBrowseInfo );
	if (hr != ES_SUCCESS)
	{
		VMU_Printf( "EnumFlashFiles failed\n", ES_ErrorTypeToString( hr ) );
		return 0;
	}

	VMU_ReleaseDevice( device );

	VMU_SortFiles();

	for (i = 0; i < vmuFileCount; i++)
	{
		if (!callback( vmuFiles[i].name, vmuFiles[i].description, vmuFiles[i].type, userData ))
			break;
	}

	return vmuFileCount;
}

/*
==================
VMU_CreateFile

Lay a fresh file out on the card, big enough for the save and carrying the
artwork and text the card's own browser shows for it. Any old copy of the file
goes first, so the blocks it held come back into the free pool.
==================
*/
int VMU_CreateFile( char *fileName, unsigned int blockCount )
{
	IEsDevice	*device;
	IEsFile		*file;
	ES_FILEINFO	*info;
	int			hr;

	device = VMU_OpenDevice();
	if (!device)
		return 0;

	info = VMU_FindFile( device, fileName );

	if (info)
	{
		hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
		if (hr != ES_SUCCESS)
		{
			VMU_Printf( "File %s was found but couldn't be deleted.\n",
						ES_ErrorTypeToString( hr ) );
		}
		else
		{
			file->lpVtbl->Delete( file );
			file->lpVtbl->Release( file );
		}
	}

	{
		ES_FILEHEADER	header = { 0 };

		header.dwSize = sizeof(header);
		header.dwHeaderSize = 5116;
		header.dwBlocks = blockCount;
		strcpy( header.szName, fileName );
		strcpy( header.szAppId, "HALF_LIFE_GAME" );
		strcpy( header.szDescription, "Half Life" );
		strcpy( header.szBootDescription, "Half Life" );
		header.bIconFlags = 0x33;
		header.bPad = 0;
		header.icon = vmuFileIcon;

		hr = device->lpVtbl->CreateFile( device, &file, &header );
		if (hr != ES_SUCCESS)
			VMU_Printf( "Can't create file on VMU!\n", ES_ErrorTypeToString( hr ) );
	}

	if (hr == ES_SUCCESS)
	{
		file->lpVtbl->Close( file );
		file->lpVtbl->Release( file );
	}

	VMU_ReleaseDevice( device );

	return hr == ES_SUCCESS;
}

/*
==================
VMU_IsDevicePresent
==================
*/
int VMU_IsDevicePresent( int slot )
{
	return vmuDevices[slot].present != 0;
}

/*
==================
VMU_FormatSlotName

Remember what was saved and which card it went on, so the loader can come back
to the same place.
==================
*/
void VMU_FormatSlotName( char *saveName )
{
	sprintf( vmuRecentSave, "@%d@%s", vmuCurrentDevice, saveName );
}

/*
==================
Host_FindRecentSave

Name of the save the player last picked, prefixed with the card it came off.
Selects that card again if it is still in, so the level files load from the
same place the header did.
==================
*/
char *Host_FindRecentSave( void )
{
	int	slot;

	if (!vmuRecentSave[0])
		return NULL;

	slot = vmuRecentSave[1] - '0';

	if (vmuDevices[slot].present)
		vmuCurrentDevice = slot;

	return vmuRecentSave + 3;
}

/*
==================
Host_SaveGameSize

How much of a card the next save wants. With nothing measured yet, guess from
how many saves are already sitting on the disc and cap it so a save screen can
never demand a whole card.
==================
*/
int Host_SaveGameSize( void )
{
	char	path[256];
	int		hl4;
	int		hl1;
	int		size;

	if (gSaveGameSize)
		return gSaveGameSize;

	sprintf( path, "%s*.HL4", Host_SaveGameDirectory() );
	COM_FixSlashes( path );
	hl4 = DirectoryCount( path );

	sprintf( path, "%s*.HL1", Host_SaveGameDirectory() );
	COM_FixSlashes( path );
	hl1 = DirectoryCount( path );

	size = VMU_BYTES_PER_SAVE * (hl4 + hl1) + VMU_BYTES_PER_SAVE;
	if (size > VMU_MAX_SAVE_BYTES)
		size = VMU_MAX_SAVE_BYTES;

	vmuReservedBytes = size;

	return size;
}

/*
==================
VMU_SetSaveResult
==================
*/
void VMU_SetSaveResult( int result )
{
	vmuSaveResult = result;
}

/*
==================
VMU_GetSaveResult
==================
*/
int VMU_GetSaveResult( void )
{
	return vmuSaveResult;
}

/*
==================
VMU_MarkSlotSaved

Flash the card that was written and hand back the message the front end should
put on screen.
==================
*/
char *VMU_MarkSlotSaved( void )
{
	if (vmuSaveResult == VMU_SAVE_OK)
	{
		vmuDevices[vmuCurrentDevice].iconWanted = VMU_ICON_SAVED;
		vmuDevices[vmuCurrentDevice].blinkCount = 100;
	}
	else
	{
		vmuDevices[vmuCurrentDevice].iconWanted = VMU_ICON_FAILED;
		vmuDevices[vmuCurrentDevice].blinkCount = 200;
	}

	return vmuSaveResultText[vmuSaveResult];
}

/*
==================
VMU_SaveGameHL4

Copy the settings file out to the card. What the file already takes up counts
as free, since it is about to be rewritten.
==================
*/
int VMU_SaveGameHL4( char *saveName )
{
	char		shortName[20];
	int			dataLen;
	int			needed;
	qboolean	room;

	VMU_MakeShortName( saveName, shortName );

	dataLen = Bfilesize_path( saveName );
	needed = dataLen + VMU_PAYLOAD_OFFSET;

	room = VMU_HasSpace( shortName, needed );

	if (!room)
	{
		vmuSaveResult = VMU_SAVE_NOROOM;
		gSaveGameSize = needed;
		return 0;
	}

	if (!VMU_CreateFile( shortName, needed ))
	{
		vmuSaveResult = VMU_SAVE_CREATEFAILED;
		return 0;
	}

	if (!VMU_WriteConfig( saveName, shortName, dataLen ))
	{
		vmuSaveResult = VMU_SAVE_WRITEFAILED;
		return 0;
	}

	Bremove_path( saveName );
	vmuSaveResult = VMU_SAVE_OK;

	return 1;
}

/*
==================
VMU_SaveGameHL1

Copy a saved game out to the card and drop the working copy on disc.
==================
*/
int VMU_SaveGameHL1( char *saveName )
{
	char		shortName[20];
	int			dataLen;
	int			rawLen;
	int			needed;
	qboolean	room;

	VMU_MakeShortName( saveName, shortName );

	dataLen = Bfilesize_path( saveName );
	needed = dataLen + VMU_SAVE_OVERHEAD;
	rawLen = Bfilesize_path( saveName );

	room = VMU_HasSpace( shortName, needed );

	if (!room)
	{
		vmuSaveResult = VMU_SAVE_NOROOM;
		gSaveGameSize = needed;
		return 0;
	}

	if (!VMU_CreateFile( shortName, needed ))
	{
		vmuSaveResult = VMU_SAVE_CREATEFAILED;
		return 0;
	}

	if (!VMU_WriteSaveGame( saveName, shortName, dataLen, rawLen ))
	{
		vmuSaveResult = VMU_SAVE_WRITEFAILED;
		return 0;
	}

	Bremove_path( saveName );
	vmuSaveResult = VMU_SAVE_OK;

	return 1;
}

/*
==================
Host_SaveGameSizeHL1

Work out what the save would cost on a card without writing anything, so the
front end can warn about a full card before the player commits.
==================
*/
int Host_SaveGameSizeHL1( char *saveName )
{
	gSaveGameSize = Bfilesize_path( saveName ) + VMU_SAVE_OVERHEAD + VMU_SAVE_SLACK;
	return 0;
}

/*
==================
VMU_ReadConfigFile

Pull a settings file off the card back into the working file system.
==================
*/
static int VMU_ReadConfigFile( char *saveName, IEsFile *file, int length )
{
	bfile_t	*out;
	char	signature[5];
	int		rawLen;
	int		offset;
	int		chunk;
	int		hr;
	char	buffer[512];

	out = (bfile_t *)Bopen( saveName, "wb" );
	if (!out)
		return 0;

	file->lpVtbl->Read( file, 0, 4, signature );
	signature[4] = 0;
	file->lpVtbl->Read( file, 4, 4, &rawLen );

	offset = VMU_PAYLOAD_OFFSET;
	chunk = sizeof(buffer) - VMU_PAYLOAD_OFFSET;
	length -= VMU_PAYLOAD_OFFSET;

	while (length > 0)
	{
		if (length < chunk)
			chunk = length;

		hr = file->lpVtbl->Read( file, offset, chunk, buffer );
		if (hr != ES_SUCCESS)
			VMU_Printf( "VMU error: %s\n", ES_ErrorTypeToString( hr ) );

		Bwrite( buffer, chunk, 1, out );

		length -= chunk;
		offset += chunk;
		chunk = sizeof(buffer);
	}

	Bclose( out );
	out->size = rawLen;

	return 1;
}

/*
==================
VMU_LoadGameHL4

Read the settings file back off the card.
==================
*/
static int VMU_LoadGameHL4( char *saveName )
{
	IEsDevice	*device;
	IEsFile		*file;
	ES_FILEINFO	*info;
	char		shortName[20];
	int			hr;
	int			result;

	result = 0;

	VMU_MakeShortName( saveName, shortName );

	device = VMU_OpenDevice();
	if (device)
	{
		info = VMU_FindFile( device, shortName );

		if (info)
		{
			hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
			if (hr != ES_SUCCESS)
			{
				VMU_Printf( "Unspecified trouble reading from VMU\n",
							ES_ErrorTypeToString( hr ) );
			}
			else
			{
				result = VMU_ReadConfigFile( saveName, file, info->dwSize );
				file->lpVtbl->Close( file );
				file->lpVtbl->Release( file );
			}
		}

		VMU_ReleaseDevice( device );
	}

	return result;
}

/*
==================
FileExists

True when the named file can be read. A save the player picked off a memory
card is not on disc yet, so this pulls it across before answering.
==================
*/
int FileExists( char *path )
{
	return VMU_LoadGameHL4( path );
}

/*
==================
VMU_RemoveSave
==================
*/
void VMU_RemoveSave( char *saveName )
{
	Bremove_path( saveName );
}

/*
==================
VMU_ReadFileToHandle

Copy a file off the card out to the development machine. Saves this game wrote
end with a browser header, which is not part of the save and is left behind.
==================
*/
static int VMU_ReadFileToHandle( char *slotName, IEsFile *file, int length )
{
	void	*out;
	char	signature[5];
	char	*read;
	char	*write;
	int		offset;
	int		chunk;
	int		hr;
	char	path[256];
	char	buffer[512];

	sprintf( path, "\\PC\\VMU_%s", slotName );

	out = Sys_OpenHandle( path, "wb" );
	if (!out)
		return 0;

	file->lpVtbl->Read( file, 0, 4, signature );
	signature[4] = 0;
	file->lpVtbl->Read( file, 4, 4, buffer );

	offset = VMU_PAYLOAD_OFFSET;
	chunk = sizeof(buffer) - VMU_PAYLOAD_OFFSET;
	length -= VMU_PAYLOAD_OFFSET;

	if (!strcmp( signature, VMU_SIG_HALFLIFE ) || !strcmp( signature, VMU_SIG_BARNEY ))
		length -= ES_HEADER_SIZE;

	if (length > 0)
	{
		read = buffer;
		write = buffer;

		do
		{
			if (length < chunk)
				chunk = length;

			hr = file->lpVtbl->Read( file, offset, chunk, read );
			if (hr != ES_SUCCESS)
				VMU_Printf( "CRC error reading from VMU\n", ES_ErrorTypeToString( hr ) );

			DC_fwrite( write, chunk, 1, out );

			length -= chunk;
			offset += chunk;
			chunk = sizeof(buffer);
		} while (length > 0);
	}

	Sys_CloseHandle( out );

	return 1;
}

/*
==================
VMU_DumpFile

Pull one file off the card and drop it on the development machine.
==================
*/
static int VMU_DumpFile( char *saveName )
{
	IEsDevice	*device;
	IEsFile		*file;
	ES_FILEINFO	*info;
	char		shortName[20];
	int			hr;
	int			result;

	result = 0;

	VMU_MakeShortName( saveName, shortName );

	device = VMU_OpenDevice();
	if (device)
	{
		info = VMU_FindFile( device, shortName );

		if (info)
		{
			hr = device->lpVtbl->OpenFile( device, &file, vmuFileHandle );
			if (hr != ES_SUCCESS)
			{
				VMU_Printf( "Can't copy file to VMU!\n", ES_ErrorTypeToString( hr ) );
			}
			else
			{
				result = VMU_ReadFileToHandle( saveName, file, info->dwSize );
				file->lpVtbl->Close( file );
				file->lpVtbl->Release( file );
			}
		}

		VMU_ReleaseDevice( device );
	}

	return result;
}

/*
==================
VMU_DumpEnumCallback
==================
*/
static int VMU_DumpEnumCallback( char *name, char *description, int type, void *userData )
{
	return TRUE;
}

/*
==================
Cmd_dumpvmu_f

Copy every file on the card over to the development machine.
==================
*/
void Cmd_dumpvmu_f( void )
{
	int	count;
	int	i;

	count = VMU_EnumFiles( VMU_DumpEnumCallback, NULL );

	for (i = 0; i < count; i++)
		VMU_DumpFile( vmuFiles[i].name );
}

// esfile.h -- flash storage service exposed by the Maple bus driver.
//
// A memory card answers as two Maple devices: a storage device, which is a
// small flash file system, and an LCD (see lcd.h). Both are reached by handing
// the device's GUID to MapleCreateDevice and asking the returned IUnknown for
// the interface below.
//
// Files live in whole blocks and always carry a 128 byte header describing them
// for the card's own browser: a short name, the application that wrote them,
// two description strings and the icon to draw beside them.

#ifndef ESFILE_H
#define ESFILE_H

#ifdef __cplusplus
extern "C" {
#endif

// Result codes. Anything but ES_SUCCESS comes back as a string from
// ES_ErrorTypeToString.
#define ES_SUCCESS					0
#define ES_ACCESSDENIED				5
#define ES_OUTOFMEMORY				14
#define ES_PERMANENTBADBLOCK		16
#define ES_BADFILECRC				24
#define ES_DEVICECORRUPTED			32
#define ES_FILECORRUPTED			48
#define ES_BADFILENAME				64
#define ES_UNKNOWNDEVICEERROR		80
#define ES_UNFORMATTEDDEVICE		96
#define ES_DEVICENOTFOUND			112
#define ES_FILEEXISTS				128
#define ES_FILENOTFOUND				144
#define ES_NOTENOUGHSPACE			160
#define ES_UNKNOWNERROR				176
#define ES_UNKNOWNDEVICETYPE		192
#define ES_UNKNOWNFILETYPE			208
#define ES_ENDOFFILE				224
#define ES_INVALIDPARAMETER			87

#define ES_MAX_FILENAME			13		// 12 characters plus the terminator
#define ES_HEADER_SIZE			128		// browser header at the tail of a file
#define ES_ICON_SIZE			1570	// palette plus the animation frames

// The moment the card stamped a file: century, year, month, day, hour, minute,
// second and weekday, each packed one to a byte.
typedef struct
{
	unsigned char	b[8];
} esstamp_t;

// The icon a file shows in the card's browser. Copied into the header whole, so
// it is a type rather than a plain array.
typedef struct
{
	unsigned short	data[ES_ICON_SIZE / 2];
} vmuicon_t;

// What the card reports about itself. Only the free block count and the block
// size are interesting; the rest is name and geometry the game never reads.
typedef struct
{
	unsigned int	dwSize;
	unsigned int	dwMaxFiles;
	unsigned int	dwReserved1[3];
	unsigned int	dwFreeBlocks;
	unsigned int	dwBlockSize;
	unsigned char	bReserved2[692];
} ES_MEDIAINFO;

// One directory entry. The tail carries the file's browser header and icon,
// which is why an entry is so much bigger than the fields anybody reads.
typedef struct
{
	unsigned int	dwValid;
	unsigned int	dwReserved1;
	unsigned int	dwBlocks;
	unsigned int	dwSize;
	unsigned int	dwReserved2;
	char			szName[ES_MAX_FILENAME];
	unsigned char	bHeader[9705];
	esstamp_t		stamp;
	unsigned short	wPad;
} ES_FILEINFO;

// Passed to CreateFile to lay out a new file: how many blocks it needs and
// everything the card's browser will show for it.
typedef struct
{
	unsigned int	dwSize;
	unsigned int	dwHeaderSize;
	unsigned int	dwReserved1[2];
	unsigned int	dwBlocks;
	char			szName[ES_MAX_FILENAME];
	char			szAppId[17];
	char			szDescription[33];
	char			szBootDescription[17];
	unsigned char	bIconFlags;
	unsigned char	bPad;
	vmuicon_t		icon;
	unsigned char	bReserved2[8076];
} ES_FILEHEADER;

// {59C842A1-C2B2-11D1-BB4D-00C04FC324BC}
DEFINE_GUID( IID_IEsDevice,
0x59c842a1, 0xc2b2, 0x11d1, 0xbb, 0x4d, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xbc );

typedef struct IEsDevice IEsDevice;
typedef struct IEsFile IEsFile;

// Called once per file on the card. Returning FALSE stops the walk.
typedef int (*ESENUMFILEPROC)( IEsDevice *pDevice, unsigned int hFile,
							   ES_FILEINFO *pInfo, void *pvContext );

typedef struct IEsFileVtbl
{
	int		(*QueryInterface)( IEsFile *This, const GUID *riid, void **ppv );
	int		(*AddRef)( IEsFile *This );
	int		(*Release)( IEsFile *This );
	int		(*Reserved0)( IEsFile *This );
	int		(*Read)( IEsFile *This, unsigned int offset, unsigned int length, void *buffer );
	int		(*Write)( IEsFile *This, unsigned int offset, unsigned int length, void *buffer );
	int		(*Delete)( IEsFile *This );
	int		(*Close)( IEsFile *This );
} IEsFileVtbl;

struct IEsFile
{
	IEsFileVtbl	*lpVtbl;
};

typedef struct IEsDeviceVtbl
{
	int		(*QueryInterface)( IEsDevice *This, const GUID *riid, void **ppv );
	int		(*AddRef)( IEsDevice *This );
	int		(*Release)( IEsDevice *This );
	int		(*Mount)( IEsDevice *This );
	int		(*Reserved0)( IEsDevice *This );
	int		(*GetMediaInfo)( IEsDevice *This, ES_MEDIAINFO *pInfo );
	int		(*EnumFiles)( IEsDevice *This, ESENUMFILEPROC pfn, void *pvContext );
	int		(*Reserved1)( IEsDevice *This );
	int		(*CreateFile)( IEsDevice *This, IEsFile **ppFile, ES_FILEHEADER *pHeader );
	int		(*OpenFile)( IEsDevice *This, IEsFile **ppFile, unsigned int hFile );
} IEsDeviceVtbl;

struct IEsDevice
{
	IEsDeviceVtbl	*lpVtbl;
};

#ifdef __cplusplus
}
#endif

#endif // ESFILE_H

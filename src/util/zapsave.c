// zapsave.c -- Zap save-game serializer for the Dreamcast VMU. Delta-compresses the
// engine's save buffer (string table, entity, classname, keyvalue, light and meta
// sections) against a base image so a saved game fits a memory card, and reads it
// back on load.

#include "quakedef.h"
#include "kzap.h"

// One savegame as it comes off the memory card. The .HL4 on the card is only
// the difference from the dictionary save, so it is read in one pass: the
// header words, the token table every section indexes its field names out of,
// and a pointer to each decoded section.
// Section records carry no padding and land wherever the previous one ended,
// so every header field has to be copied out rather than read in place.
#define ZAP_UNZIP_TEMP_FILE	"\\CD-ROM\\valve\\SAVE\\UnzipTmp.sdj"

#define ZAP_SAVEENT_SIZE	68		// engine-side entity record, classname at +4
#define ZAP_END_SECTION		200		// length byte no real record can carry

#define ZAP_RECORD_SIZE	8		// short size, short slot, int key count
#define ZAP_KEY_SIZE		4		// short size, short slot

// The size and token slot every key and record header starts with.
typedef struct
{
	short	size;
	short	slot;
} zapkey_t;

// One entity slot in the save file entity table.
typedef struct
{
	int		id;
	int		location;
	int		size;
	int		flags;
	int		classname;
} zaptable_t;

// The level header the save/restore code needs handed back.
typedef struct
{
	int		saveVersion;
	int		version;
	int		skillLevel;
	int		entityCount;
	int		connectionCount;
	int		lightStyleCount;
	float	time;
	char		mapName[32];
	char		skyName[32];
	int		skyColor_r;
	int		skyColor_g;
	int		skyColor_b;
	float	skyVec_x;
	float	skyVec_y;
	float	skyVec_z;
} zapheader_t;

// One entry in the adjacency list: where this level connects, and by which landmark.
typedef struct
{
	char		mapName[32];
	char		landmarkName[32];
	int		pentLandmark;
	float	vecLandmarkOrigin[3];
} zapmeta_t;

// A saved light style.
typedef struct
{
	int		index;
	char		style[64];
} zapstyle_t;

// One key of one entity: the token that names it, its size, and where the value
// sits. Values pulled in from the save file are allocated and freed once written.
typedef struct
{
	short	slot;
	short	size;
	char*	pData;
	char	allocated;
	char	pad[3];
} zapkeyref_t;

// One entity in the parsed list.
typedef struct
{
	short	slot;
	short	unused;
	char*	classname;
	char		keyCount;
	char		pad[3];
	zapkeyref_t*	pKeys;
} zapentity_t;

typedef struct zapsave_s
{
	int		tag;
	int		version;
	short	keyvalueSlot;
	short	pad0;
	int		tableCount;
	int		keyvalueSize;
	zaptable_t*	pTable;
	char*	pTokenData;
	char**	pTokens;
	int		tokenCount;
	int		tokenSize;
	char*	pBaseData;
	int		size;
	short	globalSlot;
	short	pad1;
	zapheader_t	header;
	short	metaSlot;
	short	pad2;
	zapmeta_t*	pMeta;
	short	stateSlot;
	short	pad3;
	zapstyle_t*	pState;
	int		entityCount;
	zapkeyref_t*	pKeys;
	zapentity_t*	pEntities;
	int		entitySize;
} zapsave_t;

int  UnzipSaveFile( char *pszName, zapsave_t *save );
void Zap_BuildTokenTable( char *pTokenData, char **pTokens, int count );
char *Zap_ParseKeyvalueSection( char *in, zaptable_t *pTable, short *pSlot, int count, char **pTokens );
char *Zap_ParseGlobalFields( char *in, short *pSlot, zapheader_t *pHeader, char **pTokens );
char *Zap_ParseSaveMetaSection( char *in, short *pSlot, zapmeta_t *pMeta, int count, char **pTokens );
char *Zap_ParseGlobalStateTable( char *in, short *pSlot, zapstyle_t *pState, int count, char **pTokens );
char *Zap_ParseEntityList( char *in, zapentity_t *pEntities, int count, zapkeyref_t **ppKeys, int totalKeys, char **pTokens );
char *Zap_SkipEntityBlock( char *in, int *pKeyCount );
void Bensure_capacity( bfile_t *h, int needed );
int  Zap_EncodeStringTable( void *handle, int count, char **pStrings, int baseCount,
	char **pBaseStrings );
int  Zap_EncodeLightSection( void *handle, int version, int count, int *pLights,
	int baseCount, int *pBaseLights );
int  Zap_EncodeEntitySection( void *handle, zapheader_t *pHeader, zapheader_t *pBaseHeader );
int  Zap_EncodeClassnameSection( void *handle, int count, void *pEntities, int baseCount,
	void *pBaseEntities );
int  Zap_EncodeKeyvalueSection( void *handle, int count, zapentity_t *pEntities,
	int baseCount, zapentity_t *pBaseEntities );
void ZipSaveGame_WriteHeader( zapsave_t save, zapsave_t base, char *pszName );
void UnzipSaveGame_ReadHeader( char *pszName, zapsave_t save, char *pszOutName );
int  Zap_DecodeStringTable( void *in, void *out, int count, char **pStrings, int padTo );
void Zap_DecodeEntitySection( void *in, void *out, int count, int baseCount, zaptable_t *pTable,
	short slot );
void Zap_DecodeGlobalSection( void *in, void *out, short slot, zapheader_t *pHeader );
void Zap_EncodeSaveMetaSection( void *in, void *out, zapsave_t save );
void Zap_DecodeSaveMetaSection( void *in, void *out, zapsave_t save );
void Zap_DecodeKeyvalueSection( void *in, void *out, zapsave_t *save );
void Zap_FreeParsedSave( zapsave_t save );
void Zap_MapKeyFieldSlot( short slot, char *name );
void Zap_FixSlashes( char *psz );
int  Zap_FieldIsEmpty( char *pData, int size );
int  Zap_FileExists( char *pszName );

// A field only travels when it holds something. Scan the whole run rather than
// stopping at the first byte, so a value with an embedded zero still counts.
__inline int Zap_FieldIsEmpty( char *pData, int size )
{
	int		i;
	int		empty;

	empty = 1;
	for (i = 0; i < size; i++)
	{
		if (pData[i])
			empty = 0;
	}

	return empty;
}

// Card paths are built with forward slashes; the file table wants backslashes.
__inline void Zap_FixSlashes( char *psz )
{
	char	c;

	c = *psz;
	while (c)
	{
		if (c == '/')
			*psz = '\\';

		psz++;
		c = *psz;
	}
}

// The token table arrives as one run of packed strings. Walk it once and hand
// out a pointer to each so the sections can index them by slot.
__inline void Zap_BuildTokenTable( char *pTokenData, char **pTokens, int count )
{
	int		i;

	for (i = 0; i < count; i++)
	{
		*pTokens = pTokenData;
		while (*pTokenData++)
			;

		pTokens++;
	}
}

// Is this file already sitting in the file table?
__inline int Zap_FileExists( char *pszName )
{
	void*	pFile;
	int		found;

	pFile = Sys_OpenHandle(pszName, "rb");
	if (pFile)
	{
		Sys_CloseHandle(pFile);
		found = 1;
	}
	else
	{
		found = 0;
	}

	return found;
}


// Delta-compress the level save against its dictionary into a .HL4 file.
void ZipSaveGame( char *pszDir, char *pszName )
{
	zapsave_t	save;
	zapsave_t	base;
	char		szDicts[256];
	char		szName[256];
	char		szZap[256];

	sprintf(szName, "%s%s.HL1", pszDir, pszName);
	sprintf(szDicts, "%s%s/%s.HL1", pszDir, "dicts", pszName);
	sprintf(szZap, "%s%s.HL4", pszDir, pszName);
	Zap_FixSlashes(szName);
	Zap_FixSlashes(szDicts);
	Zap_FixSlashes(szZap);

	// The dictionary carries the base image every section is delta-encoded
	// against, so there is nothing to write without it
	if (UnzipSaveFile(szDicts, &base) == 1)
	{
		if (UnzipSaveFile(szName, &save) == 1)
		{
			ZipSaveGame_WriteHeader(save, base, szZap);
			Zap_FreeParsedSave(save);
		}

		Zap_FreeParsedSave(base);
	}

	// The dictionary has served its purpose either way; a copy that would not
	// parse must not be left for the next level to trip over.
	Bremove_path(szDicts);
}

// Expand the compressed level save on the card back out to a plain .HL1 the
// save/restore code can read, using the dictionary save as the baseline.
void UnzipSaveGame( char *pszDir, const char *pszName )
{
	zapsave_t	save;
	char		szDicts[256];
	char		szZap[256];
	char		szName[256];

	sprintf(szName, "%s%s.HL1", pszDir, pszName);
	sprintf(szDicts, "%s%s/%s.HL1", pszDir, "dicts", pszName);
	sprintf(szZap, "%s%s.HL4", pszDir, pszName);
	Zap_FixSlashes(szName);
	Zap_FixSlashes(szDicts);
	Zap_FixSlashes(szZap);

	if (Zap_FileExists(szZap))
	{
		if (UnzipSaveFile(szDicts, &save) == 1)
		{
			UnzipSaveGame_ReadHeader(szZap, save, szName);
			Zap_FreeParsedSave(save);
		}

		Bremove_path(szDicts);
	}
}

// Running totals for the zlib heap, reported by the memory stats display.
static int	nBytesAlloced;
static int	nAllocs;
static int	nFrees;

// malloc wrapper that tracks the allocation and errors out when out of memory.
void *mallocx( int size )
{
	void*	p;

	p = MnemoAlloc(size, MNEMO_FLAG_MALLOC, 0, "gearbox");
	if (!p && size)
		Sys_Error("Out of memory in mallocx\n");

	if (p)
	{
		nAllocs++;
		nBytesAlloced += size;
	}

	return p;
}

// free wrapper that tracks the deallocation.
void freex( void *ptr )
{
	nFrees++;
	MnemoFree(ptr);
}

// Write the savegame header plus every delta-encoded section to the open file.
void ZipSaveGame_WriteHeader( zapsave_t save, zapsave_t base, char *pszName )
{
	bfile_t*	handle;

	// The two images have to describe the same save format to delta against
	if (save.tag != base.tag)
		return;

	if (save.version != base.version)
		return;

	handle = (bfile_t *)Bopen(pszName, "wb");
	Bensure_capacity(handle, 25600);

	Bwrite(&save.size, 4, 1, handle);
	Bwrite(&save.tableCount, 4, 1, handle);
	Bwrite(&save.tokenCount, 4, 1, handle);
	Bwrite(&save.tokenSize, 4, 1, handle);

	Zap_EncodeStringTable(handle, save.tokenCount, save.pTokens,
		base.tokenCount, base.pTokens);
	Zap_EncodeLightSection(handle, save.keyvalueSize, save.tableCount,
		(int *)save.pTable, base.tableCount, (int *)base.pTable);
	Zap_EncodeEntitySection(handle, &save.header, &base.header);
	Zap_EncodeClassnameSection(handle, save.header.lightStyleCount, save.pState,
		base.header.lightStyleCount, base.pState);
	Zap_EncodeKeyvalueSection(handle, save.entityCount, save.pEntities,
		base.entityCount, base.pEntities);

	Bclose(handle);
	Zip_CompressFile(pszName, 9);
}

// Token-table slot each save field landed in. The save file stores field names
// once in a string table and refers to them by slot afterwards, so every field
// the decoder cares about gets looked up on the way past and remembered here.
char*	pszZapEmpty = "";

short	keyId;
short	keyLocation;
short	keySize;
short	keyFlags;
short	keyClassname;
short	keySkillLevel;
short	keyEntityCount;
short	keyConnectionCount;
short	keyLightStyleCount;
short	keyTime;
short	keyMapName;
short	keySkyName;
short	keySkyColor_r;
short	keySkyColor_g;
short	keySkyColor_b;
short	keySkyVec_x;
short	keySkyVec_y;
short	keySkyVec_z;
short	keyAdjacentMapName;
short	keyLandmarkName;
short	keyPentLandmark;
short	keyVecLandmarkOrigin;
short	keyIndex;
short	keyStyle;

// Emit the string table, storing only strings that differ from the base image.
int Zap_EncodeStringTable( void *handle, int count, char **pStrings, int baseCount,
	char **pBaseStrings )
{
	char*	pSame;
	short	i;
	short	len;
	short	changed;
	int		written;

	written = 0;
	changed = 0;

	// Mark every string that survived from the base image; only the rest travel
	pSame = (char *)mallocx(count);
	memset(pSame, 0, count);

	for (i = 0; i < count; i++)
	{
		if (!strcmp(pBaseStrings[i], pStrings[i]))
			pSame[i] = 1;
		else
			changed++;
	}

	Bwrite(&changed, 2, 1, handle);

	written += 2;
	for (i = 0; i < count; i++)
	{
		if (!pSame[i])
		{
			if (i < count)
				len = strlen(pStrings[i]);
			else
				len = 0;

			Bwrite(&i, 2, 1, handle);
			Bwrite(&len, 2, 1, handle);
			if (len > 0)
				Bwrite(pStrings[i], 1, len, handle);

			written += len + 4;
		}
	}

	freex(pSame);

	return written;
}

// Emit the light section as a per-light change bitmask plus changed fields.
int Zap_EncodeLightSection( void *handle, int version, int count, int *pLights,
	int baseCount, int *pBaseLights )
{
	unsigned char	bits;
	short			len;
	int*			pLight;
	int*			pBase;
	int				i;
	int				written;

	Bwrite(&version, 4, 1, handle);

	written = 4;
	pBase = pBaseLights;
	pLight = pLights;

	for (i = 0; i < count; i++)
	{
		// A style past the end of the base image has nothing to delta against
		if (i < baseCount)
		{
			bits = (unsigned char)(pLight[0] != pBase[0]);

			if (pLight[1] != pBase[1])
				bits |= 2;
			if (pLight[2] != pBase[2])
				bits |= 4;
			if (pLight[3] != pBase[3])
				bits |= 8;
			if (strcmp((char *)pLight[4], (char *)pBase[4]))
				bits |= 16;
		}
		else
		{
			bits = 0xFF;
		}

		Bwrite(&bits, 1, 1, handle);
		written++;

		if (bits & 1)
		{
			Bwrite(&pLight[0], 4, 1, handle);
			written += 4;
		}
		if (bits & 2)
		{
			Bwrite(&pLight[1], 4, 1, handle);
			written += 4;
		}
		if (bits & 4)
		{
			Bwrite(&pLight[2], 4, 1, handle);
			written += 4;
		}
		if (bits & 8)
		{
			Bwrite(&pLight[3], 4, 1, handle);
			written += 4;
		}
		if (bits & 16)
		{
			len = strlen((char *)pLight[4]);
			Bwrite(&len, 2, 1, handle);
			Bwrite((char *)pLight[4], 1, len, handle);
			written += 3;
		}

		pBase += 5;
		pLight += 5;
	}

	return written;
}

// Emit the level header as a change bitmask plus the fields that differ from
// the base image. Everything past the save version and format version travels;
// the two version words are already known to agree.
int Zap_EncodeEntitySection( void *handle, zapheader_t *pHeader, zapheader_t *pBaseHeader )
{
	unsigned short	bits;
	short			mapLen;
	short			skyLen;
	int				written;

	written = 0;
	bits = 0;

	if (pHeader->skillLevel != pBaseHeader->skillLevel)
		bits |= 1;
	if (pHeader->entityCount != pBaseHeader->entityCount)
		bits |= 2;
	if (pHeader->connectionCount != pBaseHeader->connectionCount)
		bits |= 4;
	if (pHeader->lightStyleCount != pBaseHeader->lightStyleCount)
		bits |= 8;
	if (pHeader->time != pBaseHeader->time)
		bits |= 16;
	if (strcmp(pHeader->mapName, pBaseHeader->mapName))
		bits |= 32;
	if (strcmp(pHeader->skyName, pBaseHeader->skyName))
		bits |= 64;
	if (pHeader->skyColor_r != pBaseHeader->skyColor_r)
		bits |= 128;
	if (pHeader->skyColor_g != pBaseHeader->skyColor_g)
		bits |= 256;
	if (pHeader->skyColor_b != pBaseHeader->skyColor_b)
		bits |= 512;
	if (pHeader->skyVec_x != pBaseHeader->skyVec_x)
		bits |= 1024;
	if (pHeader->skyVec_y != pBaseHeader->skyVec_y)
		bits |= 2048;
	if (pHeader->skyVec_z != pBaseHeader->skyVec_z)
		bits |= 4096;

	Bwrite(&bits, 2, 1, handle);
	written += 2;

	if (bits & 1)
	{
		Bwrite(&pHeader->skillLevel, 4, 1, handle);
		written += 4;
	}
	if (bits & 2)
	{
		Bwrite(&pHeader->entityCount, 4, 1, handle);
		written += 4;
	}
	if (bits & 4)
	{
		Bwrite(&pHeader->connectionCount, 4, 1, handle);
		written += 4;
	}
	if (bits & 8)
	{
		Bwrite(&pHeader->lightStyleCount, 4, 1, handle);
		written += 4;
	}
	if (bits & 16)
	{
		Bwrite(&pHeader->time, 4, 1, handle);
		written += 4;
	}
	if (bits & 32)
	{
		mapLen = strlen(pHeader->mapName) + 1;
		Bwrite(&mapLen, 2, 1, handle);
		Bwrite(pHeader->mapName, 1, mapLen, handle);
		written += mapLen + 2;
	}
	if (bits & 64)
	{
		skyLen = strlen(pHeader->skyName) + 1;
		Bwrite(&skyLen, 2, 1, handle);
		Bwrite(pHeader->skyName, 1, skyLen, handle);
		written += skyLen + 2;
	}
	if (bits & 128)
	{
		Bwrite(&pHeader->skyColor_r, 4, 1, handle);
		written += 4;
	}
	if (bits & 256)
	{
		Bwrite(&pHeader->skyColor_g, 4, 1, handle);
		written += 4;
	}
	if (bits & 512)
	{
		Bwrite(&pHeader->skyColor_b, 4, 1, handle);
		written += 4;
	}
	if (bits & 1024)
	{
		Bwrite(&pHeader->skyVec_x, 4, 1, handle);
		written += 4;
	}
	if (bits & 2048)
	{
		Bwrite(&pHeader->skyVec_y, 4, 1, handle);
		written += 4;
	}
	if (bits & 4096)
	{
		Bwrite(&pHeader->skyVec_z, 4, 1, handle);
		written += 4;
	}

	return written;
}

// Emit the classname section, storing names that differ from the base image.
int Zap_EncodeClassnameSection( void *handle, int count, void *pEntities, int baseCount,
	void *pBaseEntities )
{
	unsigned char	len;
	unsigned char	end;
	short			i;
	int				written;

	written = 0;

	for (i = 0; i < count; i++)
	{
		if (strcmp((char *)pEntities + i * ZAP_SAVEENT_SIZE + 4,
			(char *)pBaseEntities + i * ZAP_SAVEENT_SIZE + 4))
		{
			len = (char)strlen((char *)pEntities + i * ZAP_SAVEENT_SIZE + 4) + 1;

			Bwrite(&len, 1, 1, handle);
			Bwrite(&i, 2, 1, handle);
			Bwrite((char *)pEntities + i * ZAP_SAVEENT_SIZE + 4, 1, len, handle);

			written += len + 3;
		}
	}

	end = ZAP_END_SECTION;
	Bwrite(&end, 1, 1, handle);

	return written + 1;
}

// Emit the keyvalue section, delta-encoding each entity's fields against the base.
int Zap_EncodeKeyvalueSection( void *handle, int count, zapentity_t *pEntities,
	int baseCount, zapentity_t *pBaseEntities )
{
	zapentity_t*	pEnt;
	char*			pMatched;
	unsigned char	matchedSize;
	unsigned char	opNew;
	unsigned char	opChange;
	unsigned char	opResize;
	unsigned char	opDelete;
	unsigned char	opAddKey;
	unsigned char	opAddAll;
	unsigned char	opEnd;
	unsigned char	found;
	unsigned char	j;
	unsigned char	k;
	short			best;
	short			candidate;
	short			start;
	short			i;
	unsigned int	cost;
	unsigned int	bestCost;
	int				written;
	int				overrun;

	written = 0;
	pMatched = 0;
	matchedSize = 0;

	for (i = 0; i < count; i++)
	{
		best = -1;
		bestCost = 0xFFFFFFFF;

		// One scratch byte per key of this entity, marking the ones the
		// candidate already carries
		if (!pMatched)
		{
			pEnt = &pEntities[i];
			pMatched = (char *)mallocx(pEnt->keyCount + 10);
			matchedSize = pEnt->keyCount + 10;
		}
		else
		{
			pEnt = &pEntities[i];
			if (matchedSize < pEnt->keyCount)
			{
				freex(pMatched);
				pMatched = (char *)mallocx(pEnt->keyCount + 10);
				matchedSize = pEnt->keyCount + 10;
			}
		}

		// Walk the base list from the matching index round to where we started,
		// keeping whichever entity is cheapest to describe the difference from
		start = i;
		if (i >= baseCount)
			start = 0;

		candidate = start;
		do
		{
			cost = 0;
			if (pEnt->slot == pBaseEntities[candidate].slot)
			{
				if (!pEntities[i].classname)
					found = 1;
				else if (!pBaseEntities[candidate].classname)
					found = 0;
				else
					found = (strcmp(pEntities[i].classname,
						pBaseEntities[candidate].classname) == 0);

				if (found)
				{
					for (j = 0; j < pEnt->keyCount; j++)
						pMatched[j] = 0;

					overrun = 0;
					for (k = 0; k < pBaseEntities[candidate].keyCount; k++)
					{
						found = 0;
						for (j = 0; j < pEnt->keyCount; j++)
						{
							if (pEntities[i].pKeys[j].slot
								== pBaseEntities[candidate].pKeys[k].slot)
							{
								pMatched[j] = 1;
								found = 1;

								if (pEntities[i].pKeys[j].size
									== pBaseEntities[candidate].pKeys[k].size)
								{
									if (*pEntities[i].pKeys[j].pData
										== *pBaseEntities[candidate].pKeys[k].pData)
									{
										if (memcmp(pEntities[i].pKeys[j].pData,
											pBaseEntities[candidate].pKeys[k].pData,
											pBaseEntities[candidate].pKeys[k].size))
											cost += pEnt->pKeys[j].size + 5;
									}
									else
									{
										cost += pEntities[i].pKeys[j].size + 5;
									}
								}
#if HLDC_FIXES
								else
								{
									// A key whose length changed is written out
									// again in full, so it has to be paid for
									// here as well. Costing it at nothing lets
									// the search settle on a base entity that is
									// more expensive to describe than one it
									// passed over.
									cost += pEntities[i].pKeys[j].size + 5;
								}
#endif
								break;
							}
						}

						if (!found)
							cost += 3;

						if (cost > bestCost)
						{
							overrun = 1;
							break;
						}
					}

					if (!overrun)
					{
						for (j = 0; j < pEntities[i].keyCount; j++)
						{
							if (!pMatched[j])
								cost += pEnt->pKeys[j].size + 5;
						}

						if (cost < bestCost)
						{
							best = candidate;
							bestCost = cost;
						}
					}
				}
			}

			candidate = (candidate + 1) % baseCount;
		} while (bestCost > 3 && candidate != start);

		opNew = 4;
		Bwrite(&opNew, 1, 1, handle);

		if (best == -1)
		{
			// Nothing in the base image is close enough; spell the entity out
			Bwrite(&best, 2, 1, handle);
			Bwrite(&pEnt->slot, 2, 1, handle);
			written += 5;

			for (j = 0; j < pEnt->keyCount; j++)
			{
				opAddAll = 3;
				Bwrite(&opAddAll, 1, 1, handle);
				Bwrite(&pEntities[i].pKeys[j].slot, 2, 1, handle);
				Bwrite(&pEntities[i].pKeys[j].size, 2, 1, handle);
				Bwrite(pEntities[i].pKeys[j].pData, 1,
					pEntities[i].pKeys[j].size, handle);
				written += pEntities[i].pKeys[j].size + 5;
			}
		}
		else
		{
			Bwrite(&best, 2, 1, handle);
			written += 3;

			for (j = 0; j < pEntities[i].keyCount; j++)
				pMatched[j] = 0;

			// Keys the base entity has: changed ones travel, missing ones are dropped
			for (k = 0; k < pBaseEntities[best].keyCount; k++)
			{
				found = 0;
				for (j = 0; j < pEnt->keyCount; j++)
				{
					if (pEntities[i].pKeys[j].slot == pBaseEntities[best].pKeys[k].slot)
					{
						pMatched[j] = 1;
						found = 1;

						if (pEntities[i].pKeys[j].size
							== pBaseEntities[best].pKeys[k].size)
						{
							if (memcmp(pEntities[i].pKeys[j].pData,
								pBaseEntities[best].pKeys[k].pData,
								pBaseEntities[best].pKeys[k].size))
							{
								opChange = 1;
								Bwrite(&opChange, 1, 1, handle);
								Bwrite(&pEnt->pKeys[j].slot, 2, 1, handle);
								Bwrite(&pEnt->pKeys[j].size, 2, 1, handle);
								Bwrite(pEnt->pKeys[j].pData,
									pEnt->pKeys[j].size, 1, handle);
								written += pEnt->pKeys[j].size + 5;
							}
						}
						else
						{
							opResize = 1;
							Bwrite(&opResize, 1, 1, handle);
							Bwrite(&pEnt->pKeys[j].slot, 2, 1, handle);
							Bwrite(&pEnt->pKeys[j].size, 2, 1, handle);
							Bwrite(pEnt->pKeys[j].pData,
								pEnt->pKeys[j].size, 1, handle);
							written += pEnt->pKeys[j].size + 5;
						}
						break;
					}
				}

				if (!found)
				{
					opDelete = 2;
					Bwrite(&opDelete, 1, 1, handle);
					Bwrite(&pBaseEntities[best].pKeys[k].slot, 2, 1, handle);
					written += 3;
				}
			}

			// Whatever the base entity never had
			for (j = 0; j < pEntities[i].keyCount; j++)
			{
				if (!pMatched[j])
				{
					opAddKey = 3;
					Bwrite(&opAddKey, 1, 1, handle);
					Bwrite(&pEnt->pKeys[j].slot, 2, 1, handle);
					Bwrite(&pEnt->pKeys[j].size, 2, 1, handle);
					Bwrite(pEnt->pKeys[j].pData, 1, pEnt->pKeys[j].size, handle);
					written += pEnt->pKeys[j].size + 5;
				}
			}
		}
	}

	opEnd = 5;
	Bwrite(&opEnd, 1, 1, handle);

	if (pMatched)
		freex(pMatched);

	return written + 1;
}

// Read and re-emit a compressed savegame header from the VMU temp file.
void UnzipSaveGame_ReadHeader( char *pszName, zapsave_t save, char *pszOutName )
{
	void*	pIn;
	void*	pOut;
	int		size;
	int		tableCount;
	int		tokenCount;
	int		tokenSize;

	Zip_DecompressFile(pszName, ZAP_UNZIP_TEMP_FILE);

	pIn = Sys_OpenHandle(ZAP_UNZIP_TEMP_FILE, "rb");
	if (!pIn)
		return;

	pOut = Bopen(pszOutName, "wb");

	DC_fread(&size, sizeof(int), 1, pIn);
	DC_fread(&tableCount, sizeof(int), 1, pIn);
	DC_fread(&tokenCount, sizeof(int), 1, pIn);
	DC_fread(&tokenSize, sizeof(int), 1, pIn);

	// The header goes back out unchanged ahead of the expanded sections
	Bwrite(&save.tag, sizeof(int), 1, pOut);
	Bwrite(&save.version, sizeof(int), 1, pOut);
	Bwrite(&size, sizeof(int), 1, pOut);
	Bwrite(&tableCount, sizeof(int), 1, pOut);
	Bwrite(&tokenCount, sizeof(int), 1, pOut);
	Bwrite(&tokenSize, sizeof(int), 1, pOut);

	// Nothing has been seen in the string table yet
	keyId = -1;
	keyLocation = -1;
	keySize = -1;
	keyFlags = -1;
	keyClassname = -1;
	keySkillLevel = -1;
	keyEntityCount = -1;
	keyConnectionCount = -1;
	keyLightStyleCount = -1;
	keyTime = -1;
	keyMapName = -1;
	keySkyName = -1;
	keySkyColor_r = -1;
	keySkyColor_g = -1;
	keySkyColor_b = -1;
	keySkyVec_x = -1;
	keySkyVec_y = -1;
	keySkyVec_z = -1;
	keyAdjacentMapName = -1;
	keyLandmarkName = -1;
	keyPentLandmark = -1;
	keyVecLandmarkOrigin = -1;
	keyIndex = -1;
	keyStyle = -1;

	Zap_DecodeStringTable(pIn, pOut, save.tokenCount, save.pTokens, tokenSize);
	Zap_DecodeEntitySection(pIn, pOut, tableCount, save.tableCount, save.pTable,
		save.keyvalueSlot);
	Zap_DecodeGlobalSection(pIn, pOut, save.globalSlot, &save.header);
	Zap_EncodeSaveMetaSection(pIn, pOut, save);
	Zap_DecodeSaveMetaSection(pIn, pOut, save);
	Zap_DecodeKeyvalueSection(pIn, pOut, &save);

	Sys_CloseHandle(pIn);
	Bclose(pOut);
	Bremove_path(ZAP_UNZIP_TEMP_FILE);

	Zip_CompressFile(pszOutName, 5);
}

// Decode the savegame string table into the output stream.
int Zap_DecodeStringTable( void *in, void *out, int count, char **pStrings, int padTo )
{
	char		token[256];
	char		pad;
	short		changed;
	short		slot;
	short		len;
	int			length;
	int			written;
	int			next;
	int			i;

	written = 0;
	next = 0;

	DC_fread(&changed, 2, 1, in);

	for (i = 0; i < changed; i++)
	{
		DC_fread(&slot, 2, 1, in);
		DC_fread(&len, 2, 1, in);

		// Everything up to this slot came through unchanged, so it comes back off
		// the base image
		while (next < slot)
		{
			length = strlen(pStrings[next]);
			Bwrite(pStrings[next], 1, length + 1, out);
			written += length + 1;
			Zap_MapKeyFieldSlot((short)next, pStrings[next]);
			next++;
		}

		if (len < 1)
		{
			Bwrite(pszZapEmpty, 1, 1, out);
			written++;
		}
		else
		{
			DC_fread(token, 1, len, in);
			token[len] = 0;
			Bwrite(token, 1, len + 1, out);
			written += len + 1;
			Zap_MapKeyFieldSlot((short)next, token);
		}

		next++;
	}

	// Whatever is left never changed
	while (next < count)
	{
		length = strlen(pStrings[next]);
		Bwrite(pStrings[next], 1, length + 1, out);
		written += length + 1;
		Zap_MapKeyFieldSlot((short)next, pStrings[next]);
		next++;
	}

	// The section is a fixed size, so pad it back out
	while (written < padTo)
	{
		pad = 0;
		Bwrite(&pad, 1, 1, out);
		written++;
	}

	return next;
}

// Decode the delta-compressed entity section, expanding fields against a baseline.
void Zap_DecodeEntitySection( void *in, void *out, int count, int baseCount, zaptable_t *pTable,
	short slot )
{
	zaptable_t		entry;
	char			name[256];
	unsigned char	bits;
	unsigned char	pad;
	short			len;
	short			size;
	short			idSize;
	short			locationSize;
	short			sizeSize;
	short			flagsSize;
	short			classnameSize;
	int				sectionSize;
	int				keyCount;
	int				written;
	int				i;

	DC_fread(&sectionSize, 4, 1, in);

	written = 0;
	for (i = 0; i < count; i++)
	{
		// A zero mask means this record survived the delta untouched
		DC_fread(&bits, 1, 1, in);
		if (!bits)
		{
			entry.id = pTable[i].id;
			entry.location = pTable[i].location;
			entry.size = pTable[i].size;
			entry.flags = pTable[i].flags;
			entry.classname = pTable[i].classname;
		}
		else
		{
			if (bits & 1)
				DC_fread(&entry.id, 4, 1, in);
			else
				entry.id = pTable[i].id;

			if (bits & 2)
				DC_fread(&entry.location, 4, 1, in);
			else
				entry.location = pTable[i].location;

			if (bits & 4)
				DC_fread(&entry.size, 4, 1, in);
			else
				entry.size = pTable[i].size;

			if (bits & 8)
				DC_fread(&entry.flags, 4, 1, in);
			else
				entry.flags = pTable[i].flags;

			if (bits & 16)
			{
				DC_fread(&len, 2, 1, in);
				DC_fread(name, 1, len, in);
				name[len] = 0;
				entry.classname = (int)name;
			}
			else
			{
				entry.classname = pTable[i].classname;
			}
		}

		// Empty fields are left out of the record entirely
		keyCount = 0;
		if (entry.id)
			keyCount++;
		if (entry.location)
			keyCount++;
		if (entry.size)
			keyCount++;
		if (entry.flags)
			keyCount++;
		if (*(char *)entry.classname)
			keyCount++;

		size = 4;
		Bwrite(&size, 2, 1, out);
		Bwrite(&slot, 2, 1, out);
		Bwrite(&keyCount, 4, 1, out);
		written += ZAP_RECORD_SIZE;

		if (entry.id)
		{
			idSize = 4;
			Bwrite(&idSize, 2, 1, out);
			Bwrite(&keyId, 2, 1, out);
			Bwrite(&entry.id, idSize, 1, out);
			written += idSize + ZAP_KEY_SIZE;
		}
		if (entry.location)
		{
			locationSize = 4;
			Bwrite(&locationSize, 2, 1, out);
			Bwrite(&keyLocation, 2, 1, out);
			Bwrite(&entry.location, locationSize, 1, out);
			written += locationSize + ZAP_KEY_SIZE;
		}
		if (entry.size)
		{
			sizeSize = 4;
			Bwrite(&sizeSize, 2, 1, out);
			Bwrite(&keySize, 2, 1, out);
			Bwrite(&entry.size, sizeSize, 1, out);
			written += sizeSize + ZAP_KEY_SIZE;
		}
		if (entry.flags)
		{
			flagsSize = 4;
			Bwrite(&flagsSize, 2, 1, out);
			Bwrite(&keyFlags, 2, 1, out);
			Bwrite(&entry.flags, flagsSize, 1, out);
			written += flagsSize + ZAP_KEY_SIZE;
		}
		if (*(char *)entry.classname)
		{
			classnameSize = strlen((char *)entry.classname) + 1;
			Bwrite(&classnameSize, 2, 1, out);
			Bwrite(&keyClassname, 2, 1, out);
			Bwrite((char *)entry.classname, classnameSize, 1, out);
			written += classnameSize + ZAP_KEY_SIZE;
		}
	}

	// The section has to come out the length the file says it is
	while (written < sectionSize)
	{
		pad = 0;
		Bwrite(&pad, 1, 1, out);
		written++;
	}
}

// Decode the global (level) save block, expanding delta fields against a baseline.
void Zap_DecodeGlobalSection( void *in, void *out, short slot, zapheader_t *pHeader )
{
	zapheader_t		header;
	unsigned short	bits;
	short			mapLen;
	short			skyLen;
	short			size;
	short			skillSize;
	short			entitySize;
	short			connectionSize;
	short			lightStyleSize;
	short			timeSize;
	short			mapSize;
	short			skySize;
	short			colorRSize;
	short			colorGSize;
	short			colorBSize;
	short			vecXSize;
	short			vecYSize;
	short			vecZSize;
	int				keyCount;

	keyCount = 0;

	// Fields the mask does not claim come back off the base image unchanged
	DC_fread(&bits, 2, 1, in);

	if (bits & 1)
		DC_fread(&header.skillLevel, 4, 1, in);
	else
		header.skillLevel = pHeader->skillLevel;
	if (header.skillLevel)
		keyCount++;

	if (bits & 2)
		DC_fread(&header.entityCount, 4, 1, in);
	else
		header.entityCount = pHeader->entityCount;
	if (header.entityCount)
		keyCount++;

	if (bits & 4)
		DC_fread(&header.connectionCount, 4, 1, in);
	else
		header.connectionCount = pHeader->connectionCount;
	if (header.connectionCount)
		keyCount++;

	if (bits & 8)
		DC_fread(&header.lightStyleCount, 4, 1, in);
	else
		header.lightStyleCount = pHeader->lightStyleCount;
	if (header.lightStyleCount)
		keyCount++;

	if (bits & 16)
		DC_fread(&header.time, 4, 1, in);
	else
		header.time = pHeader->time;
	if (!Zap_FieldIsEmpty((char *)&header.time, 4))
		keyCount++;

	memset(header.mapName, 0, sizeof(header.mapName));
	if (bits & 32)
	{
		DC_fread(&mapLen, 2, 1, in);
		DC_fread(header.mapName, mapLen, 1, in);
	}
	else
	{
		strcpy(header.mapName, pHeader->mapName);
	}
	if (!Zap_FieldIsEmpty(header.mapName, 32))
		keyCount++;

	memset(header.skyName, 0, sizeof(header.skyName));
	if (bits & 64)
	{
		DC_fread(&skyLen, 2, 1, in);
		DC_fread(header.skyName, skyLen, 1, in);
	}
	else
	{
		strcpy(header.skyName, pHeader->skyName);
	}
	if (!Zap_FieldIsEmpty(header.skyName, 32))
		keyCount++;

	if (bits & 128)
		DC_fread(&header.skyColor_r, 4, 1, in);
	else
		header.skyColor_r = pHeader->skyColor_r;
	if (header.skyColor_r)
		keyCount++;

	if (bits & 256)
		DC_fread(&header.skyColor_g, 4, 1, in);
	else
		header.skyColor_g = pHeader->skyColor_g;
	if (header.skyColor_g)
		keyCount++;

	if (bits & 512)
		DC_fread(&header.skyColor_b, 4, 1, in);
	else
		header.skyColor_b = pHeader->skyColor_b;
	if (header.skyColor_b)
		keyCount++;

	if (bits & 1024)
		DC_fread(&header.skyVec_x, 4, 1, in);
	else
		header.skyVec_x = pHeader->skyVec_x;
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_x, 4))
		keyCount++;

	if (bits & 2048)
		DC_fread(&header.skyVec_y, 4, 1, in);
	else
		header.skyVec_y = pHeader->skyVec_y;
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_y, 4))
		keyCount++;

	if (bits & 4096)
		DC_fread(&header.skyVec_z, 4, 1, in);
	else
		header.skyVec_z = pHeader->skyVec_z;
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_z, 4))
		keyCount++;

	size = 4;
	Bwrite(&size, 2, 1, out);
	Bwrite(&slot, 2, 1, out);
	Bwrite(&keyCount, 4, 1, out);

	if (header.skillLevel)
	{
		skillSize = 4;
		Bwrite(&skillSize, 2, 1, out);
		Bwrite(&keySkillLevel, 2, 1, out);
		Bwrite(&header.skillLevel, skillSize, 1, out);
	}
	if (header.entityCount)
	{
		entitySize = 4;
		Bwrite(&entitySize, 2, 1, out);
		Bwrite(&keyEntityCount, 2, 1, out);
		Bwrite(&header.entityCount, entitySize, 1, out);
	}
	if (header.connectionCount)
	{
		connectionSize = 4;
		Bwrite(&connectionSize, 2, 1, out);
		Bwrite(&keyConnectionCount, 2, 1, out);
		Bwrite(&header.connectionCount, connectionSize, 1, out);
	}
	if (header.lightStyleCount)
	{
		lightStyleSize = 4;
		Bwrite(&lightStyleSize, 2, 1, out);
		Bwrite(&keyLightStyleCount, 2, 1, out);
		Bwrite(&header.lightStyleCount, lightStyleSize, 1, out);
	}
	if (!Zap_FieldIsEmpty((char *)&header.time, 4))
	{
		timeSize = 4;
		Bwrite(&timeSize, 2, 1, out);
		Bwrite(&keyTime, 2, 1, out);
		Bwrite(&header.time, timeSize, 1, out);
	}
	if (!Zap_FieldIsEmpty(header.mapName, 32))
	{
		mapSize = 32;
		Bwrite(&mapSize, 2, 1, out);
		Bwrite(&keyMapName, 2, 1, out);
		Bwrite(header.mapName, mapSize, 1, out);
	}
	if (!Zap_FieldIsEmpty(header.skyName, 32))
	{
		skySize = 32;
		Bwrite(&skySize, 2, 1, out);
		Bwrite(&keySkyName, 2, 1, out);
		Bwrite(header.skyName, skySize, 1, out);
	}
	if (header.skyColor_r)
	{
		colorRSize = 4;
		Bwrite(&colorRSize, 2, 1, out);
		Bwrite(&keySkyColor_r, 2, 1, out);
		Bwrite(&header.skyColor_r, colorRSize, 1, out);
	}
	if (header.skyColor_g)
	{
		colorGSize = 4;
		Bwrite(&colorGSize, 2, 1, out);
		Bwrite(&keySkyColor_g, 2, 1, out);
		Bwrite(&header.skyColor_g, colorGSize, 1, out);
	}
	if (header.skyColor_b)
	{
		colorBSize = 4;
		Bwrite(&colorBSize, 2, 1, out);
		Bwrite(&keySkyColor_b, 2, 1, out);
		Bwrite(&header.skyColor_b, colorBSize, 1, out);
	}
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_x, 4))
	{
		vecXSize = 4;
		Bwrite(&vecXSize, 2, 1, out);
		Bwrite(&keySkyVec_x, 2, 1, out);
		Bwrite(&header.skyVec_x, vecXSize, 1, out);
	}
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_y, 4))
	{
		vecYSize = 4;
		Bwrite(&vecYSize, 2, 1, out);
		Bwrite(&keySkyVec_y, 2, 1, out);
		Bwrite(&header.skyVec_y, vecYSize, 1, out);
	}
	if (!Zap_FieldIsEmpty((char *)&header.skyVec_z, 4))
	{
		vecZSize = 4;
		Bwrite(&vecZSize, 2, 1, out);
		Bwrite(&keySkyVec_z, 2, 1, out);
		Bwrite(&header.skyVec_z, vecZSize, 1, out);
	}
}

// Emit the save-meta (adjacency/landmark) section to the output stream.
void Zap_EncodeSaveMetaSection( void *in, void *out, zapsave_t save )
{
	short		size;
	short		mapSize;
	short		landmarkSize;
	short		pentSize;
	short		vecSize;
	int			keyCount;
	int			i;

	for (i = 0; i < save.header.connectionCount; i++)
	{
		// Only the fields this landmark actually carries go in the record
		keyCount = 0;
		if (!Zap_FieldIsEmpty(save.pMeta[i].mapName, 32))
			keyCount++;
		if (!Zap_FieldIsEmpty(save.pMeta[i].landmarkName, 32))
			keyCount++;
		if (save.pMeta[i].pentLandmark)
			keyCount++;
		if (!Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[0], 4)
			|| !Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[1], 4)
			|| !Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[2], 4))
			keyCount++;

		size = 4;
		Bwrite(&size, 2, 1, out);
		Bwrite(&save.metaSlot, 2, 1, out);
		Bwrite(&keyCount, 4, 1, out);

		if (!Zap_FieldIsEmpty(save.pMeta[i].mapName, 32))
		{
			mapSize = 32;
			Bwrite(&mapSize, 2, 1, out);
			Bwrite(&keyAdjacentMapName, 2, 1, out);
			Bwrite(save.pMeta[i].mapName, 1, mapSize, out);
		}
		if (!Zap_FieldIsEmpty(save.pMeta[i].landmarkName, 32))
		{
			landmarkSize = 32;
			Bwrite(&landmarkSize, 2, 1, out);
			Bwrite(&keyLandmarkName, 2, 1, out);
			Bwrite(save.pMeta[i].landmarkName, 1, landmarkSize, out);
		}
		if (save.pMeta[i].pentLandmark)
		{
			pentSize = 4;
			Bwrite(&pentSize, 2, 1, out);
			Bwrite(&keyPentLandmark, 2, 1, out);
			Bwrite(&save.pMeta[i].pentLandmark, 1, pentSize, out);
		}
		if (!Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[0], 4)
			|| !Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[1], 4)
			|| !Zap_FieldIsEmpty((char *)&save.pMeta[i].vecLandmarkOrigin[2], 4))
		{
			vecSize = 12;
			Bwrite(&vecSize, 2, 1, out);
			Bwrite(&keyVecLandmarkOrigin, 2, 1, out);
			Bwrite(save.pMeta[i].vecLandmarkOrigin, vecSize, 1, out);
		}
	}
}

// Decode the save-meta (adjacency/landmark) section into the output stream.
void Zap_DecodeSaveMetaSection( void *in, void *out, zapsave_t save )
{
	unsigned char	len;
	short			index;
	short			size;
	short			indexSize;
	short			styleSize;
	int				keyCount;
	int				i;

	// Pull back the names that differed from the base image
	DC_fread(&len, 1, 1, in);
	while (len != ZAP_END_SECTION)
	{
		DC_fread(&index, 2, 1, in);
		DC_fread((char *)save.pState + index * ZAP_SAVEENT_SIZE + 4, len, 1, in);
		DC_fread(&len, 1, 1, in);
	}

	for (i = 0; i < save.header.lightStyleCount; i++)
	{
		size = 4;
		keyCount = (save.pState[i].index != 0);
		if (!Zap_FieldIsEmpty(save.pState[i].style, 64))
			keyCount++;

		Bwrite(&size, 2, 1, out);
		Bwrite(&save.stateSlot, 2, 1, out);
		Bwrite(&keyCount, 4, 1, out);

		if (save.pState[i].index)
		{
			indexSize = 4;
			Bwrite(&indexSize, 2, 1, out);
			Bwrite(&keyIndex, 2, 1, out);
			Bwrite(&save.pState[i].index, indexSize, 1, out);
		}
		if (!Zap_FieldIsEmpty(save.pState[i].style, 64))
		{
			styleSize = 64;
			Bwrite(&styleSize, 2, 1, out);
			Bwrite(&keyStyle, 2, 1, out);
			Bwrite(save.pState[i].style, styleSize, 1, out);
		}
	}
}

// Decode the keyvalue section, tracking active keys per token.
void Zap_DecodeKeyvalueSection( void *in, void *out, zapsave_t *save )
{
	zapkeyref_t		keys[255];
	zapkeyref_t		edit;
	zapkeyref_t		key;
	unsigned char	op;
	unsigned char	keyCount;
	unsigned char	i;
	short			entitySlot;
	short			delSlot;
	short			slot;
	short			size;
	int				count;

	keyCount = 0;

	DC_fread(&op, 1, 1, in);
	while (op == 4)
	{
		// Slot -1 means a brand new entity; otherwise start from the base one
		DC_fread(&entitySlot, 2, 1, in);
		if (entitySlot == -1)
		{
			DC_fread(&slot, 2, 1, in);
			keyCount = 0;
		}
		else
		{
			slot = save->pEntities[entitySlot].slot;
			keyCount = save->pEntities[entitySlot].keyCount;

			for (i = 0; i < save->pEntities[entitySlot].keyCount; i++)
			{
				keys[i].slot = save->pEntities[entitySlot].pKeys[i].slot;
				keys[i].size = save->pEntities[entitySlot].pKeys[i].size;
				keys[i].pData = save->pEntities[entitySlot].pKeys[i].pData;
				keys[i].allocated = 0;
			}
		}

		DC_fread(&op, 1, 1, in);
		while (op < 4)
		{
			if (op == 1)
			{
				// Replace the value this entity had in the base image
				DC_fread(&edit.slot, 2, 1, in);
				DC_fread(&edit.size, 2, 1, in);
				edit.pData = (char *)mallocx(edit.size);
				DC_fread(edit.pData, edit.size, 1, in);

				for (i = 0; i < keyCount; i++)
				{
					if (keys[i].slot == edit.slot)
					{
						keys[i].size = edit.size;
						keys[i].pData = edit.pData;
						keys[i].allocated = 1;
						break;
					}
				}
			}
			else if (op == 2)
			{
				// Drop a key the base image had and this save does not
				DC_fread(&delSlot, 2, 1, in);

				for (i = 0; i < keyCount; i++)
				{
					if (keys[i].slot == delSlot)
					{
						for ( ; i < keyCount - 1; i++)
							keys[i] = keys[i + 1];

						keyCount--;
						break;
					}
				}
			}
			else if (op == 3)
			{
				// A key the base image never had
				DC_fread(&key.slot, 2, 1, in);
				DC_fread(&key.size, 2, 1, in);
				key.pData = (char *)mallocx(key.size);
				DC_fread(key.pData, key.size, 1, in);
				key.allocated = 1;

				keys[keyCount] = key;
				keyCount++;
			}

			DC_fread(&op, 1, 1, in);
		}

		size = 4;
		Bwrite(&size, 2, 1, out);
		Bwrite(&slot, 2, 1, out);
		count = keyCount;
		Bwrite(&count, 4, 1, out);

		for (i = 0; i < keyCount; i++)
		{
			Bwrite(&keys[i].size, 2, 1, out);
			Bwrite(&keys[i].slot, 2, 1, out);
			Bwrite(keys[i].pData, keys[i].size, 1, out);

			if (keys[i].allocated == 1)
				freex(keys[i].pData);
		}
	}
}

// Bind a decoded field name to its fixed keyvalue slot index.
void Zap_MapKeyFieldSlot( short slot, char *name )
{
	if (!name[0])
		return;

	if (keyId == -1 && !strcmp("id", name))
		keyId = slot;

	if (keyLocation == -1 && !strcmp("location", name))
		keyLocation = slot;

	if (keySize == -1 && !strcmp("size", name))
		keySize = slot;

	if (keyFlags == -1 && !strcmp("flags", name))
		keyFlags = slot;

	if (keyClassname == -1 && !strcmp("classname", name))
		keyClassname = slot;

	if (keySkillLevel == -1 && !strcmp("skillLevel", name))
		keySkillLevel = slot;

	if (keyEntityCount == -1 && !strcmp("entityCount", name))
		keyEntityCount = slot;

	if (keyConnectionCount == -1 && !strcmp("connectionCount", name))
		keyConnectionCount = slot;

	if (keyLightStyleCount == -1 && !strcmp("lightStyleCount", name))
		keyLightStyleCount = slot;

	if (keyTime == -1 && !strcmp("time", name))
		keyTime = slot;

	if (keyMapName == -1 && !strcmp("mapName", name))
		keyMapName = slot;

	if (keySkyName == -1 && !strcmp("skyName", name))
		keySkyName = slot;

	if (keySkyColor_r == -1 && !strcmp("skyColor_r", name))
		keySkyColor_r = slot;

	if (keySkyColor_g == -1 && !strcmp("skyColor_g", name))
		keySkyColor_g = slot;

	if (keySkyColor_b == -1 && !strcmp("skyColor_b", name))
		keySkyColor_b = slot;

	if (keySkyVec_x == -1 && !strcmp("skyVec_x", name))
		keySkyVec_x = slot;

	if (keySkyVec_y == -1 && !strcmp("skyVec_y", name))
		keySkyVec_y = slot;

	if (keySkyVec_z == -1 && !strcmp("skyVec_z", name))
		keySkyVec_z = slot;

	if (keyAdjacentMapName == -1 && !strcmp("mapName", name))
		keyAdjacentMapName = slot;

	if (keyLandmarkName == -1 && !strcmp("landmarkName", name))
		keyLandmarkName = slot;

	if (keyPentLandmark == -1 && !strcmp("pentLandmark", name))
		keyPentLandmark = slot;

	if (keyVecLandmarkOrigin == -1 && !strcmp("vecLandmarkOrigin", name))
		keyVecLandmarkOrigin = slot;

	if (keyIndex == -1 && !strcmp("index", name))
		keyIndex = slot;

	if (keyStyle == -1 && !strcmp("style", name))
		keyStyle = slot;
}

// Read a full compressed savegame file into an in-memory parse structure.
int UnzipSaveFile( char *pszName, zapsave_t *save )
{
	void*	pFile;
	char*	p;
	char*	pEntities;
	int		pad, keys, count;
	int		result;

	pFile = Sys_OpenHandle(pszName, "rb");
	if (!pFile)
		return 0;

	DC_fread(&save->tag, sizeof(int), 1, pFile);
	if (save->tag == SAVEFILE_HEADER)
	{
		DC_fread(&save->version, sizeof(int), 1, pFile);
		DC_fread(&save->size, sizeof(int), 1, pFile);
		DC_fread(&save->tableCount, sizeof(int), 1, pFile);
		DC_fread(&save->tokenCount, sizeof(int), 1, pFile);
		DC_fread(&save->tokenSize, sizeof(int), 1, pFile);

		save->pTokenData = (char *)mallocx(save->tokenSize);
		save->pTokens = (char **)mallocx((save->tokenCount + 8) * sizeof(char *));
		DC_fread(save->pTokenData, save->tokenSize, 1, pFile);
		Zap_BuildTokenTable(save->pTokenData, save->pTokens, save->tokenCount);

		save->pBaseData = (char *)mallocx(save->size + 3);
		DC_fread(save->pBaseData, save->size, 1, pFile);

		save->pTable = (zaptable_t *)mallocx(save->tableCount * sizeof(zaptable_t));
		p = Zap_ParseKeyvalueSection(save->pBaseData, save->pTable, &save->keyvalueSlot,
			save->tableCount, save->pTokens);
		save->keyvalueSize = p - save->pBaseData;

		// Sections start on a long boundary, so pull the padding across too
		pad = ((save->keyvalueSize + 3) & ~3) - save->keyvalueSize;
		if (pad > 0)
		{
			DC_fread(save->pBaseData + save->size, 1, pad, pFile);
			save->keyvalueSize += pad;
			p += pad;
		}

		p = Zap_ParseGlobalFields(p, &save->globalSlot, &save->header, save->pTokens);

		save->pMeta = (zapmeta_t *)mallocx(save->header.connectionCount * sizeof(zapmeta_t));
		p = Zap_ParseSaveMetaSection(p, &save->metaSlot, save->pMeta, save->header.connectionCount,
			save->pTokens);

		save->pState = (zapstyle_t *)mallocx(save->header.lightStyleCount * sizeof(zapstyle_t));
		pEntities = Zap_ParseGlobalStateTable(p, &save->stateSlot, save->pState,
			save->header.lightStyleCount, save->pTokens);

		// Entities run to the end of the block, so count them before allocating
		keys = 0;
		save->entityCount = 0;
		save->pKeys = 0;
		p = pEntities;
		while (p - save->pBaseData < save->size)
		{
			p = Zap_SkipEntityBlock(p, &count);
			save->entityCount++;
			keys += count;
		}

		save->pEntities = (zapentity_t *)mallocx(save->entityCount * sizeof(zapentity_t));
		p = Zap_ParseEntityList(pEntities, save->pEntities, save->entityCount, &save->pKeys,
			keys, save->pTokens);
		save->entitySize = p - pEntities;

		result = 1;
	}
	else
	{
		result = 0;
	}

	Sys_CloseHandle(pFile);

	return result;
}

// Free every buffer owned by a parsed savegame structure.
void Zap_FreeParsedSave( zapsave_t save )
{
	freex(save.pTokenData);
	freex(save.pTokens);
	freex(save.pBaseData);
	freex(save.pTable);
	freex(save.pMeta);
	freex(save.pState);

	if (save.pKeys)
		freex(save.pKeys);

	if (save.pEntities)
		freex(save.pEntities);
}

// Parse the packed keyvalue section from RAM into the field arrays.
char *Zap_ParseKeyvalueSection( char *in, zaptable_t *pTable, short *pSlot, int count, char **pTokens )
{
	short	size;
	short	slot;
	int		keys;
	int		i, j;

	for (i = 0; i < count; i++)
	{
		memcpy(&size, in, sizeof(size));
		memcpy(pSlot, in + 2, sizeof(short));
		memcpy(&keys, in + 4, sizeof(keys));
		in += ZAP_RECORD_SIZE;

		pTable->id = 0;
		pTable->location = 0;
		pTable->size = 0;
		pTable->flags = 0;
		pTable->classname = (int)pszZapEmpty;

		for (j = 0; j < keys; j++)
		{
			memcpy(&size, in, sizeof(size));
			memcpy(&slot, in + 2, sizeof(slot));
			in += ZAP_KEY_SIZE;

			switch (*pTokens[slot])
			{
			case 'c':
				pTable->classname = (int)in;
				break;
			case 'f':
				memcpy(&pTable->flags, in, size);
				break;
			case 'i':
				memcpy(&pTable->id, in, size);
				break;
			case 'l':
				memcpy(&pTable->location, in, size);
				break;
			case 's':
				memcpy(&pTable->size, in, size);
				break;
			}

			in += size;
		}

		pTable++;
	}

	return in;
}

// Parse the packed global-fields block from RAM into the header struct.
char *Zap_ParseGlobalFields( char *in, short *pSlot, zapheader_t *pHeader, char **pTokens )
{
	char*	pName;
	short	size;
	short	slot;
	int		keys;
	int		i;

	memcpy(&size, in, sizeof(size));
	memcpy(pSlot, in + 2, sizeof(short));
	memcpy(&keys, in + 4, sizeof(keys));
	in += ZAP_RECORD_SIZE;

	pHeader->saveVersion = 0;
	pHeader->version = 0;
	pHeader->skillLevel = 0;
	pHeader->entityCount = 0;
	pHeader->connectionCount = 0;
	pHeader->lightStyleCount = 0;
	pHeader->time = 0;
	memset(pHeader->mapName, 0, sizeof(pHeader->mapName));
	memset(pHeader->skyName, 0, sizeof(pHeader->skyName));
	pHeader->skyColor_r = 0;
	pHeader->skyColor_g = 0;
	pHeader->skyColor_b = 0;
	pHeader->skyVec_x = 0;
	pHeader->skyVec_y = 0;
	pHeader->skyVec_z = 0;

	for (i = 0; i < keys; i++)
	{
		memcpy(&size, in, sizeof(size));
		memcpy(&slot, in + 2, sizeof(slot));
		in += ZAP_KEY_SIZE;

		pName = pTokens[slot];

		// The header field names all differ by their third character, bar the sky
		// block, which needs another letter or two to separate
		switch (pName[2])
		{
		case 'g':
			memcpy(&pHeader->lightStyleCount, in, size);
			break;
		case 'i':
			memcpy(&pHeader->skillLevel, in, size);
			break;
		case 'm':
			memcpy(&pHeader->time, in, size);
			break;
		case 'n':
			memcpy(&pHeader->connectionCount, in, size);
			break;
		case 'p':
			memcpy(pHeader->mapName, in, size);
			break;
		case 'r':
			memcpy(&pHeader->version, in, size);
			break;
		case 't':
			memcpy(&pHeader->entityCount, in, size);
			break;
		case 'v':
			memcpy(&pHeader->saveVersion, in, size);
			break;
		case 'y':
			if (pName[3] == 'C')
			{
				if (pName[9] == 'b')
					memcpy(&pHeader->skyColor_b, in, size);
				else if (pName[9] == 'g')
					memcpy(&pHeader->skyColor_g, in, size);
				else if (pName[9] == 'r')
					memcpy(&pHeader->skyColor_r, in, size);
			}
			else if (pName[3] == 'N')
			{
				memcpy(pHeader->skyName, in, size);
			}
			else if (pName[3] == 'V')
			{
				if (pName[7] == 'x')
					memcpy(&pHeader->skyVec_x, in, size);
				else if (pName[7] == 'y')
					memcpy(&pHeader->skyVec_y, in, size);
				else if (pName[7] == 'z')
					memcpy(&pHeader->skyVec_z, in, size);
			}
			break;
		}

		in += size;
	}

	return in;
}

// Parse the packed save-meta (adjacency) records from RAM.
char *Zap_ParseSaveMetaSection( char *in, short *pSlot, zapmeta_t *pMeta, int count, char **pTokens )
{
	short	size;
	short	slot;
	int		keys;
	int		i, j;

	for (i = 0; i < count; i++)
	{
		memcpy(&size, in, sizeof(size));
		memcpy(pSlot, in + 2, sizeof(short));
		memcpy(&keys, in + 4, sizeof(keys));
		in += ZAP_RECORD_SIZE;

		memset(pMeta->mapName, 0, sizeof(pMeta->mapName));
		memset(pMeta->landmarkName, 0, sizeof(pMeta->landmarkName));
		pMeta->pentLandmark = 0;
		memset(pMeta->vecLandmarkOrigin, 0, sizeof(pMeta->vecLandmarkOrigin));

		for (j = 0; j < keys; j++)
		{
			memcpy(&size, in, sizeof(size));
			memcpy(&slot, in + 2, sizeof(slot));
			in += ZAP_KEY_SIZE;

			switch (*pTokens[slot])
			{
			case 'l':
				memcpy(pMeta->landmarkName, in, size);
				break;
			case 'm':
				memcpy(pMeta->mapName, in, size);
				break;
			case 'p':
				memcpy(&pMeta->pentLandmark, in, size);
				break;
			case 'v':
				memcpy(pMeta->vecLandmarkOrigin, in, size);
				break;
			}

			in += size;
		}

		pMeta++;
	}

	return in;
}

// Parse the packed global-state table from RAM.
char *Zap_ParseGlobalStateTable( char *in, short *pSlot, zapstyle_t *pState, int count, char **pTokens )
{
	short	size;
	short	slot;
	int		keys;
	int		i, j;

	for (i = 0; i < count; i++)
	{
		memcpy(&size, in, sizeof(size));
		memcpy(pSlot, in + 2, sizeof(short));
		memcpy(&keys, in + 4, sizeof(keys));
		in += ZAP_RECORD_SIZE;

		pState->index = 0;
		memset(pState->style, 0, sizeof(pState->style));

		for (j = 0; j < keys; j++)
		{
			memcpy(&size, in, sizeof(size));
			memcpy(&slot, in + 2, sizeof(slot));
			in += ZAP_KEY_SIZE;

			if (*pTokens[slot] == 'i')
				memcpy(&pState->index, in, size);
			else if (*pTokens[slot] == 's')
				memcpy(pState->style, in, size);

			in += size;
		}

		pState++;
	}

	return in;
}

// Parse the entity list from RAM, filling per-entity keyvalue arrays.
char *Zap_ParseEntityList( char *in, zapentity_t *pEntities, int count, zapkeyref_t **ppKeys, int totalKeys, char **pTokens )
{
	zapkeyref_t*	pPool;
	short	size;
	short	slot;
	int		keys;
	int		used;
	int		i, j;

	pPool = (zapkeyref_t *)mallocx(totalKeys * sizeof(zapkeyref_t));
	*ppKeys = pPool;
	used = 0;

	for (i = 0; i < count; i++)
	{
		memcpy(&size, in, sizeof(size));
		memcpy(&slot, in + 2, sizeof(slot));
		memcpy(&keys, in + 4, sizeof(keys));
		in += ZAP_RECORD_SIZE;

		pEntities->slot = slot;
		pEntities->classname = 0;
		pEntities->keyCount = (char)keys;
		pEntities->pKeys = pPool + used;
		used += keys;

		for (j = 0; j < keys; j++)
		{
			memcpy(&size, in, sizeof(size));
			memcpy(&slot, in + 2, sizeof(slot));

			pEntities->pKeys[j].size = size;
			pEntities->pKeys[j].slot = slot;
			pEntities->pKeys[j].pData = in + ZAP_KEY_SIZE;
			in += ZAP_KEY_SIZE + size;

			if (!strcmp(pTokens[slot], "classname"))
				pEntities->classname = pEntities->pKeys[j].pData;
		}

		pEntities++;
	}

	return in;
}

// Advance past one packed entity block, returning its key count.
char *Zap_SkipEntityBlock( char *in, int *pKeyCount )
{
	zapkey_t	key;
	int		i;

	memcpy(&key, in, sizeof(key));
	memcpy(pKeyCount, in + 4, sizeof(int));
	in += ZAP_RECORD_SIZE;

	for (i = 0; i < *pKeyCount; i++)
	{
		memcpy(&key, in, sizeof(key));
		in += ZAP_KEY_SIZE + key.size;
	}

	return in;
}

// gl_model.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "decal.h"
#include "textures.h"
#include "gl_water.h"

model_t* loadmodel;
char loadname[32];	// for hunk tags
char* wadpath;

void Mod_LoadSpriteModel( model_t* mod, void* buffer );
void Mod_LoadBrushModel( model_t* mod, void* buffer );
void Mod_LoadStudioModel( model_t* mod, void* buffer );
model_t* Mod_LoadModel( model_t* mod, qboolean crash, qboolean bDefer );

model_t* mod_known[MAX_MODELS];
int		mod_numknown;

int gSpriteTextureFormat = SPR_NORMAL;

extern qboolean gSpriteMipMap;

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void* Mod_Extradata( model_t* mod )
{
	void* r;

	r = Cache_Check(&mod->cache);
	if (r)
		return r;

	Mod_LoadModel(mod, TRUE, FALSE);

	if (!mod->cache.data)
		Sys_Error("Mod_Extradata: caching failed");
	return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t* Mod_PointInLeaf( vec_t* p, model_t* model )
{
	mnode_t* node;
	float		d;
	mplane_t* plane;

	if (!model || !model->nodes)
		Sys_Error("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll( void )
{
	int		i;
	model_t* mod;

	for (i = 0; i < mod_numknown; i++)
	{
		mod = mod_known[i];
		if (mod->type != mod_alias && mod->needload != NL_CLIENT)
			mod->needload = NL_NEEDS_LOADED;
	}

	Mod_InitNormalTable();
	DC_FreeStaleTextureSlots();
}

/*
==================
Mod_FindName

==================
*/
model_t* Mod_FindName( char* name )
{
	int		i;
	model_t* mod;

	if (!name[0])
		Sys_Error("Mod_FindName: NULL name");

	for (i = 0; i < mod_numknown; i++)
	{
		if (!strcmp(mod_known[i]->name, name))
			return mod_known[i];
	}

	if (mod_numknown == MAX_MODELS)
		Sys_Error("mod_numknown == MAX_MODELS");

	mod = (model_t*)MnemoAlloc(sizeof(model_t), 0x20, 0, "mod_known");
	memset(mod, 0, sizeof(model_t));
	strcpy(mod->name, name);
	mod->needload = NL_NEEDS_LOADED;
	mod_known[mod_numknown] = mod;
	mod_numknown++;
	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel( char* name )
{
	model_t* mod;

	mod = Mod_FindName(name);

	if (mod->needload == NL_PRESENT || mod->needload == (NL_NEEDS_LOADED | NL_UNREFERENCED))
	{
		if (mod->type == mod_alias || mod->type == mod_studio)
			Cache_Check(&mod->cache);
	}
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t* Mod_LoadModel( model_t* mod, qboolean crash, qboolean bDefer )
{
	unsigned* buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

	if (mod->type == mod_alias || mod->type == mod_studio)
	{
		if (Cache_Check(&mod->cache))
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT || mod->needload == (NL_NEEDS_LOADED | NL_UNREFERENCED))
			return mod;		// not cached at all
	}

//
// studio models are streamed off the disc on demand, so a precache only needs
// to register the name; the file itself is read the first time the model is
// drawn (through Mod_Extradata).
//
	if (bDefer && !strstr(mod->name, ".bsp") && strstr(mod->name, ".mdl"))
	{
		mod->type = mod_studio;
		mod->needload = NL_PRESENT;
		return mod;
	}

//
// load the file
//
	buf = (unsigned*)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error("Mod_NumForName: %s not found", mod->name);
		return NULL;
	}

	if (developer.value > 1)
		Con_DPrintf("loading %s\n", mod->name);

//
// allocate a new model
//
	COM_FileBase(mod->name, loadname);

	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	mod->needload = NL_PRESENT;

	switch (LittleLong(*(unsigned*)buf))
	{
	case IDPOLYHEADER:
		Sys_Error("Alias models are so 1995!\n");
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;

	case IDSTUDIOHEADER:
		Mod_LoadStudioModel(mod, buf);
		break;

	default:
		Mod_LoadBrushModel(mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t* Mod_ForName( char* name, qboolean crash )
{
	model_t* mod;

	mod = Mod_FindName(name);

	return Mod_LoadModel(mod, crash, FALSE);
}

/*
==================
Mod_ForNameDefer

As Mod_ForName, but a model that can wait is left unloaded until something
actually draws it.
==================
*/
model_t* Mod_ForNameDefer( char* name, qboolean crash )
{
	model_t* mod;

	mod = Mod_FindName(name);

	return Mod_LoadModel(mod, crash, TRUE);
}

void Mod_MarkClient( model_t* pModel )
{
	pModel->needload = (NL_NEEDS_LOADED | NL_UNREFERENCED);
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte* mod_base;

#define MIPSCALE			(64 + 16 + 4 + 1)
#define PIXELS_SIZE			(MIPSCALE * (512 * 512) / 64)
#define PALETTE_SIZE		(256 * 3) + 2
#define TEXTUREDATA_SIZE	(PIXELS_SIZE + PALETTE_SIZE + MIP_EXTRASIZE + sizeof(miptex_t))
#define TEMP_TEXBUF_INIT	0x5846

/*
===============
Mod_LoadTextures
===============
*/
void Mod_LoadTextures( lump_t* l )
{
	int				i, j, pixels, palette, num, max, altmax;
	miptex_t* mt;
	miptex_t* source_mt;
	texture_t* tx, * tx2;
	texture_t* anims[10];
	texture_t* altanims[10];
	dmiptexlump_t* m;
	char			perMapWadPath[1024];
	byte*			tempTexData = NULL;
	int				tempTexCapacity = 0;
	unsigned char* pPal;
	qboolean		wads_parsed = FALSE;
	qboolean		hasPerMapWads = FALSE;
	double			starttime;
	byte* rawtex;

	starttime = Sys_FloatTime();

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}

	m = (dmiptexlump_t*)(mod_base + l->fileofs);
	hasPerMapWads = TEX_BuildPerMapWadPath(loadmodel->name, perMapWadPath, sizeof(perMapWadPath));

	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = (texture_t**)Hunk_AllocName(m->nummiptex * sizeof(*loadmodel->textures), loadname);

	for (i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t*)((byte*)m + m->dataofs[i]);
		source_mt = mt;

		if (r_wadtextures.value || !LittleLong(mt->offsets[0]))
		{
			qboolean isGbix = FALSE;
			int srcW, srcH;
			int neededSize;

			if (!wads_parsed)
			{
				if (hasPerMapWads)
					TEX_InitFromWad(perMapWadPath);
				else if (wadpath && wadpath[0])
					TEX_InitFromWad(wadpath);
				else
					Con_DPrintf("Mod_LoadTextures: no WAD path available for %s\n", loadmodel->name);
				TEX_AddAnimatingTextures();
				wads_parsed = TRUE;
			}

			if (!tempTexData)
			{
				tempTexData = (byte*)MnemoAlloc(TEMP_TEXBUF_INIT, 0x20, 0, "temp texture buffer");
				tempTexCapacity = TEMP_TEXBUF_INIT;
			}

			srcW = LittleLong(mt->width);
			srcH = LittleLong(mt->height);
			if (srcW > 0 && srcH > 0)
			{
				neededSize = ((srcW * srcH) + ((srcW >> 1) * (srcH >> 1)) +
					((srcW >> 2) * (srcH >> 2)) + ((srcW >> 3) * (srcH >> 3))) * 4 + 0x346;

				if (neededSize > tempTexCapacity)
				{
					byte* newBuf = (byte*)MnemoAlloc(neededSize, MNEMO_FLAG_MALLOC, 0, "temp texture buffer");
					memcpy(newBuf, tempTexData, tempTexCapacity);
					MnemoFree(tempTexData);
					tempTexData = newBuf;
					tempTexCapacity = neededSize;
				}
			}

			if (!TEX_LoadLump(mt->name, tempTexData))
				continue;

			isGbix = (*(unsigned int *)tempTexData == GBIXHEADER) ? TRUE : FALSE;

			if (isGbix)
			{
				mt = source_mt;
			}
			else
			{
				mt = (miptex_t*)tempTexData;
				source_mt = mt;
			}

			mt->width = LittleLong(mt->width);
			mt->height = LittleLong(mt->height);
			for (j = 0; j < MIPLEVELS; j++)
				mt->offsets[j] = LittleLong(mt->offsets[j]);

			if ((mt->width & 15) || (mt->height & 15))
				Sys_Error("Texture %s is not 16 aligned", mt->name);

			if (isGbix)
			{
				byte* fallbackRaw = NULL;
				byte* fallbackPal = NULL;
				int fallbackPixels;
				pvrt_t *pvrt;

				tx = (texture_t*)Hunk_AllocName(sizeof(texture_t), loadname);
				memset(tx, 0, sizeof(texture_t));
				loadmodel->textures[i] = tx;

				memcpy(tx->name, mt->name, sizeof(tx->name));
				if (strchr(tx->name, '~'))
					tx->name[2] = ' ';

				tx->width = mt->width;
				tx->height = mt->height;

				if (!Q_strncmp(mt->name, "sky", 3))
					R_InitSky();
				else
					tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, tempTexData, TRUE, TEX_TYPE_GBIX, NULL);

				continue;
			}
		}

		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		if ((mt->width & 15) || (mt->height & 15))
			Sys_Error("Texture %s is not 16 aligned", mt->name);

		// total amount of pixels and palette entires
		pixels = mt->width * mt->height / 64 * MIPSCALE;
		palette = *(word*)((byte*)mt + pixels + sizeof(miptex_t)) * 3;

		tx = (texture_t*)Hunk_AllocName(sizeof(texture_t) + palette, loadname);

		loadmodel->textures[i] = tx;

		// copy data
		memcpy(tx->name, mt->name, sizeof(tx->name));
		if (strchr(tx->name, '~'))
			tx->name[2] = ' ';
		tx->width = mt->width;
		tx->height = mt->height;
		for (j = 0; j < MIPLEVELS; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);

		// palette is at the end of current texture field
		pPal = (byte*)mt + pixels + sizeof(miptex_t) + sizeof(word);
		tx->pPal = (byte*)(tx + 1);

		// store palette data
		memcpy(tx + 1, pPal, palette);

		rawtex = (byte*)(mt + 1);

		if (!Q_strncmp(mt->name, "sky", 3))
			R_InitSky();
		else
		{
			if (mt->name[0] == '{')
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_ALPHA, pPal);
			}
			else
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_NONE, pPal);
			}

		}
	}

	if (wads_parsed)
	{
		TEX_CleanupWadInfo();
	}
	if (tempTexData)
		MnemoFree(tempTexData);

//
// sequence the animations
//
	for (i = 0; i < m->nummiptex; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || (tx->name[0] != '+' && tx->name[0] != '-'))
			continue;
		if (tx->anim_next)
			continue; // allready sequenced

	// find the number of frames in the animation
		memset(anims, 0, sizeof(anims));
		memset(altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error("Bad animating texture %s", tx->name);

		for (j = i + 1; j < m->nummiptex; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || (tx2->name[0] != '+' && tx2->name[0] != '-'))
				continue;
			if (strcmp(tx2->name + 2, tx->name + 2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num + 1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num + 1 > altmax)
					altmax = num + 1;
			}
			else
				Sys_Error("Bad animating texture %s", tx->name);
		}

#define	ANIM_CYCLE	1
	// link them all together
		for (j = 0; j < max; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error("Missing frame %i of %s", j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j = 0; j < altmax; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error("Missing frame %i of %s", j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}

	Con_DPrintf("Texture load: %6.1fms\n", (Sys_FloatTime() - starttime) * 1000.0);
}

/*
===============
Mod_LoadLighting
===============
*/
static qboolean Mod_TryLoadLt2Lighting( void )
{
	int h[3];
	int lt2Len;
	int lt2Mode;
	int lightBytes;
	int surfCount;
	int headerBytes;
	char lt2Path[MAX_QPATH];
	byte* lt2Data;
	byte* cursor;

	if (!loadmodel || !loadmodel->name[0])
		return FALSE;

	Q_strncpy(lt2Path, loadmodel->name, sizeof(lt2Path));
	lt2Path[sizeof(lt2Path) - 1] = '\0';

	if (Q_strncmp(lt2Path, "maps/", 5))
		return FALSE;

	{
		int nameLen = (int)strlen(lt2Path);
		if (nameLen < 4 || Q_strcasecmp(lt2Path + nameLen - 4, ".bsp"))
			return FALSE;
		strcpy(lt2Path + nameLen - 4, ".lt2");
	}

	h[2] = -1;
	lt2Len = COM_OpenFile(lt2Path, h);
	if (h[2] == -1 || lt2Len <= 0)
		return FALSE;
	COM_CloseFile(h[0], h[1], h[2]);

	lt2Data = (byte*)COM_LoadFile(lt2Path, 5, NULL);
	if (!lt2Data)
		return FALSE;

	cursor = lt2Data;
	headerBytes = 8;
	lt2Mode = 0;
	if (!Q_strncmp((char*)cursor, "LT2", 3))
	{
		lt2Mode = cursor[3];
		cursor += 4;
		headerBytes += 4;
	}

	lightBytes = LittleLong(*(int*)cursor);
	cursor += 4;
	surfCount = LittleLong(*(int*)cursor);
	cursor += 4;

	if (lightBytes <= 0 || surfCount < 0 || (headerBytes + lightBytes + (surfCount << 2)) > lt2Len)
	{
		COM_FreeFile(lt2Data);
		return FALSE;
	}

	loadmodel->lightdata = (byte*)MnemoAlloc(lightBytes, 0x20, 0, "LightSurfs");
	if (!loadmodel->lightdata)
	{
		COM_FreeFile(lt2Data);
		return FALSE;
	}
	memcpy(loadmodel->lightdata, cursor, lightBytes);

	/* Allocate lightsurfs table */
	loadmodel->lightsurfs = (int*)MnemoAlloc(surfCount * 4, 0x20, 0, "LightSurfs");
	if (!loadmodel->lightsurfs)
	{
		COM_FreeFile(lt2Data);
		return FALSE;
	}
	{
		int* src = (int*)(cursor + ((lightBytes + 3) & ~3));
		int i;
		for (i = 0; i < surfCount; i++)
			loadmodel->lightsurfs[i] = LittleLong(src[i]);
	}

	loadmodel->lightmap_mode = (lt2Mode == 'a') ? 3 : 2;
	loadmodel->lightBytes = lightBytes;
	COM_FreeFile(lt2Data);

	if (lt2Mode == 'a')
		Con_DPrintf("LT2 LERP lights (alpha): %s bytes=%d surfs=%d\n", lt2Path, lightBytes, surfCount);
	else
		Con_DPrintf("LT2 LERP lights: %s bytes=%d surfs=%d\n", lt2Path, lightBytes, surfCount);

	return TRUE;
}

void Mod_LoadLighting( lump_t* l )
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		loadmodel->lightmap_mode = 0;
		loadmodel->lightBytes = 0;
		loadmodel->lightsurfs = NULL;
		loadmodel->lightdata = NULL;
		return;
	}

	if (Mod_TryLoadLt2Lighting())
		return;

	loadmodel->lightmap_mode = 0;
	loadmodel->lightBytes = 0;
	loadmodel->lightsurfs = NULL;
	loadmodel->lightdata = NULL;
	loadmodel->lightdata = (color24*)MnemoAlloc(l->filelen, MNEMO_FLAG_MALLOC, 0, loadname);
	if (!loadmodel->lightdata)
		Sys_Error("Mod_LoadLighting: failed to allocate %d bytes for %s", l->filelen, loadmodel->name);
	memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
===============
Mod_LoadVisibility
===============
*/
void Mod_LoadVisibility( lump_t* l )
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = (byte*)Hunk_AllocName(l->filelen, loadname);
	memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities( lump_t* l )
{
	char* pszInputStream;

	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = (char*)Hunk_AllocName(l->filelen, loadname);
	memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);

	if (loadmodel->entities)
	{
		pszInputStream = COM_Parse(loadmodel->entities);
		while (*pszInputStream && com_token[0] != '}')
		{
			if (!strcmp(com_token, "wad"))
			{
				COM_Parse(pszInputStream);

				if (wadpath)
					free(wadpath);
				wadpath = _strdup(com_token);
				break;
			}
			pszInputStream = COM_Parse(pszInputStream);
		}
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes( lump_t* l )
{
	dvertex_t* in;
	mvertex_t* out;
	int			i, count;

	in = (dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels( lump_t* l )
{
	dmodel_t* in;
	dmodel_t* out;
	int			i, j, count;

	in = (dmodel_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dmodel_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (j = 0; j < MAX_MAP_HULLS; j++)
			out->headnode[j] = LittleLong(in->headnode[j]);
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges( lump_t* l )
{
	dedge_t* in;
	medge_t* out;
	int 	i, count;

	in = (dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*)Hunk_AllocName((count + 1) * sizeof(*out), loadname);

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo( lump_t* l )
{
	texinfo_t* in;
	mtexinfo_t* out;
	int 	i, j, count;
	int		miptex;

	in = (texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
		miptex = LittleLong(in->miptex);
		out->flags = LittleLong(in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
===============
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
===============
*/
void CalcSurfaceExtents( msurface_t* s )
{
	float	mins[2], maxs[2], val;
	int		i, j, e;
	mvertex_t* v;
	mtexinfo_t* tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++)
		{
			val = v->position[0] * (double)tex->vecs[j][0] +
				v->position[1] * (double)tex->vecs[j][1] +
				v->position[2] * (double)tex->vecs[j][2] +
				(double)tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 512)
			Sys_Error("Bad surface extents %d/%d", s->extents[0], s->extents[1]);
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces( lump_t* l )
{
	dface_t* in;
	msurface_t* out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t*)Hunk_AllocName(count * sizeof(*out), loadname);
	
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;
		out->pdecals = NULL; // the surface has no decals by default

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort(in->texinfo);
		
		CalcSurfaceExtents(out);

	// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		if ((loadmodel->lightmap_mode == 2 || loadmodel->lightmap_mode == 3) && loadmodel->lightsurfs)
			i = loadmodel->lightsurfs[surfnum];
		else
			i = LittleLong(in->lightofs);

		if (i == -1)
			out->samples = NULL;
		else
			out->samples = (color24*)((byte*)loadmodel->lightdata + i);
		
	// set the drawing flags flag
		
		if (!Q_strncmp(out->texinfo->texture->name, "sky", 3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}

		if (!Q_strncmp(out->texinfo->texture->name, "scroll", 6))	// scroll
		{
			out->flags |= SURF_DRAWTILED;
			continue;
		}

		if (out->texinfo->texture->name[0] == '!' ||
			!_strnicmp(out->texinfo->texture->name, "laser", 5) ||
			!_strnicmp(out->texinfo->texture->name, "water", 5))	// turbulent
		{
			out->flags |= SURF_DRAWTURB;
			GL_SubdivideSurface(out);	// cut up polygon for warps
			continue;
		}
		
		if (out->texinfo->flags & TEX_SPECIAL)
		{
			out->flags |= SURF_DRAWTILED;
			continue;
		}
	}
}

#pragma optimize("", off)
/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent( mnode_t* node, mnode_t* parent )
{
	*(volatile mnode_t **)&node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}
#pragma optimize("", on)
/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes( lump_t* l )
{
	int			i, j, count, p;
	dnode_t* in;
	mnode_t* out;

	in = (dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs( lump_t* l )
{
	dleaf_t* in;
	mleaf_t* out;
	int			i, j, count, p;

	in = (dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			in->firstmarksurface;
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j = 0; j < 4; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes( lump_t* l )
{
	dclipnode_t* in, * out;
	int			i, count;
	hull_t* hull;

	in = (dclipnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -36;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 36;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -32;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[3];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -18;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 18;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0( void )
{
	mnode_t* in, * child;
	dclipnode_t* out;
	int			i, j, count;
	hull_t* hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j = 0; j < 2; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces( lump_t* l )
{
	int		i, j, count;
	short* in;
	msurface_t** out;

	in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges( lump_t* l )
{
	int		i, count;
	int* in, * out;

	in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (int*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}

/*
=================
Mod_LoadPlanes
=================
*/
/*
==============================================================================

PLANE NORMAL TABLE

A level's planes share very few distinct normals -- every axial plane in the
map reuses one of six -- so the loader hashes each normal into one table and
keeps only the index on the plane itself.

==============================================================================
*/

#define NORMAL_TABLE_SIZE	4096
#define NORMAL_TABLE_MASK	(NORMAL_TABLE_SIZE - 1)
#define NORMAL_TABLE_EMPTY	1000.0f		// no real normal has a component this big

planenormal_t*	g_planeNormalTable;
int				normal_count;
int				normal_collisions;

/*
===============
Mod_AddNormalToTable

Returns the index this normal lives at, adding it if it is not there yet.
===============
*/
int Mod_AddNormalToTable( vec_t* normal, unsigned int hash )
{
	planenormal_t*	entry;
	byte*			pb;
	unsigned short	index;
	int				i;

	pb = (byte*)normal;
	for (i = 0; i < 12; i++)
	{
		hash ^= *pb++;
		if (hash & 1)
			hash = (hash >> 1) | 0x0800;
		else
			hash = hash >> 1;
	}

	index = hash & NORMAL_TABLE_MASK;
	entry = g_planeNormalTable + index;
	i = NORMAL_TABLE_MASK;

	while (1)
	{
		if (entry->normal[0] > 2.0f)
		{
			entry->normal[0] = normal[0];
			entry->normal[1] = normal[1];
			entry->normal[2] = normal[2];
			entry->unused = 0.0f;
			normal_count++;
			return index;
		}

		if (entry->normal[0] == normal[0] &&
			entry->normal[1] == normal[1] &&
			entry->normal[2] == normal[2])
		{
			return index;
		}

		index++;
		normal_collisions++;
		entry++;

		if (index == NORMAL_TABLE_SIZE)
		{
			index = 0;
			entry = g_planeNormalTable;
		}

		if (i-- == 0)
			return Sys_Error("Normal table full!");
	}
}

/*
===============
Mod_InitNormalTable

Empties the table and seeds it with the six axial normals.
===============
*/
void Mod_InitNormalTable( void )
{
	vec3_t	normal;
	int		i;

	if (!g_planeNormalTable)
		g_planeNormalTable = (planenormal_t*)MnemoAlloc(NORMAL_TABLE_SIZE * sizeof(planenormal_t), 0x20, 0, "norm_table");

	for (i = 0; i < NORMAL_TABLE_SIZE * 4; i++)
		((float*)g_planeNormalTable)[i] = NORMAL_TABLE_EMPTY;

	normal_count = 0;
	normal_collisions = 0;

	for (i = 0; i < 6; i++)
	{
		normal[0] = 0.0f;
		normal[1] = 0.0f;
		normal[2] = 0.0f;
		normal[i >> 1] = (i & 1) ? -1.0f : 1.0f;

		Mod_AddNormalToTable(normal, (i & 1) << (i >> 1));
	}
}

void Mod_LoadPlanes( lump_t* l )
{
	int			i, j;
	mplane_t* out;
	dplane_t* in;
	int			count;
	int			bits;

	in = (dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mplane_t*)Hunk_AllocName(count * 2 * sizeof(*out), loadname);

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( vec_t* mins, vec_t* maxs )
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength(corner);
}

qboolean Mod_LoadTexturesFile( char* path );

/*
==============================================================================

PIECEWISE WORLD LOADING

A whole BSP does not fit in memory at once, so the world is brought in a lump
at a time: each chunk is read straight off the disc into its own buffer, handed
to the ordinary lump loader, and released again before the next one starts.

==============================================================================
*/

/*
=================
Mod_LoadEntitiesChunk
=================
*/
qboolean Mod_LoadEntitiesChunk( char* path, dheader_t* header )
{
	lump_t*	l;
	byte*	buf;
	char*	pszInputStream;

	l = &header->lumps[LUMP_ENTITIES];

	buf = (byte*)Hunk_AllocName(l->filelen + 0x21, loadname);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, l->fileofs, (l->filelen + 31) & ~31);
	mod_base = buf - l->fileofs;

	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return FALSE;
	}

	loadmodel->entities = (char*)buf;

	pszInputStream = COM_Parse(loadmodel->entities);
	if (*pszInputStream && com_token[0] != '}')
	{
		while (strcmp(com_token, "wad"))
		{
			pszInputStream = COM_Parse(pszInputStream);
			if (!*pszInputStream || com_token[0] == '}')
				return TRUE;
		}

		COM_Parse(pszInputStream);

		if (wadpath)
			free(wadpath);
		wadpath = _strdup(com_token);
	}

	return TRUE;
}

/*
=================
Mod_LoadVisibilityChunk
=================
*/
qboolean Mod_LoadVisibilityChunk( char* path, dheader_t* header )
{
	lump_t*	l;
	byte*	buf;

	l = &header->lumps[LUMP_VISIBILITY];

	buf = (byte*)Hunk_AllocName(l->filelen + 0x21, loadname);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, l->fileofs, (l->filelen + 31) & ~31);
	mod_base = buf - l->fileofs;

	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return FALSE;
	}

	loadmodel->visdata = buf;
	return TRUE;
}

/*
=================
Mod_LoadLightingChunk
=================
*/
qboolean Mod_LoadLightingChunk( char* path, dheader_t* header )
{
	lump_t*	l;
	byte*	buf;

	if (Mod_TryLoadLt2Lighting())
		return TRUE;

	l = &header->lumps[LUMP_LIGHTING];

	buf = (byte*)Hunk_AllocName(l->filelen + 0x21, loadname);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, l->fileofs, (l->filelen + 31) & ~31);
	mod_base = buf - l->fileofs;

	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return FALSE;
	}

	loadmodel->lightBytes = l->filelen;
	loadmodel->lightdata = (color24*)buf;
	loadmodel->lightmap_mode = 0;

	return TRUE;
}

/*
=================
Mod_LoadWorldChunk

Reads one lump off the disc and runs the loader that owns it.
=================
*/
qboolean Mod_LoadWorldChunk( char* path, int lumpnum, dheader_t* header )
{
	lump_t*	l;
	byte*	buf;

	l = &header->lumps[lumpnum];

	buf = (byte*)Hunk_AllocName(l->filelen + 0x21, loadname);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, l->fileofs, (l->filelen + 31) & ~31);
	mod_base = buf - l->fileofs;

	switch (lumpnum)
	{
	case LUMP_ENTITIES:
		Mod_LoadEntities(l);
		break;

	case LUMP_PLANES:
		Mod_LoadPlanes(l);
		break;

	case LUMP_TEXTURES:
		if (!Mod_LoadTexturesFile(path))
		{
			Sys_SetTaskName("Looked for compact texture info, loading from BSP");
			Mod_LoadTextures(l);
		}
		break;

	case LUMP_VERTEXES:
		Mod_LoadVertexes(l);
		break;

	case LUMP_VISIBILITY:
		if (!l->filelen)
		{
			loadmodel->visdata = NULL;
		}
		else
		{
			loadmodel->visdata = (byte*)Hunk_AllocName(l->filelen, loadname);
			memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
		}
		break;

	case LUMP_NODES:
		Mod_LoadNodes(l);
		break;

	case LUMP_TEXINFO:
		Mod_LoadTexinfo(l);
		break;

	case LUMP_FACES:
		Mod_LoadFaces(l);
		break;

	case LUMP_LIGHTING:
		if (!Mod_TryLoadLt2Lighting())
		{
			if (!l->filelen)
			{
				loadmodel->lightdata = NULL;
			}
			else
			{
				loadmodel->lightBytes = l->filelen;
				loadmodel->lightdata = (color24*)Hunk_AllocName(l->filelen, loadname);
				memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
				loadmodel->lightmap_mode = 0;
			}
		}
		break;

	case LUMP_CLIPNODES:
		Mod_LoadClipnodes(l);
		break;

	case LUMP_LEAFS:
		Mod_LoadLeafs(l);
		break;

	case LUMP_MARKSURFACES:
		Mod_LoadMarksurfaces(l);
		break;

	case LUMP_EDGES:
		Mod_LoadEdges(l);
		break;

	case LUMP_SURFEDGES:
		Mod_LoadSurfedges(l);
		break;

	case LUMP_MODELS:
		Mod_LoadSubmodels(l);
		break;
	}

	COM_FreeFile(buf);
	return TRUE;
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel( model_t* mod, void* buffer )
{
	int			i, j;
	dheader_t* header;
	dmodel_t* bm;

	loadmodel->type = mod_brush;

	header = (dheader_t*)buffer;

	i = LittleLong(header->version);
	if (i != Q1BSP_VERSION && i != BSPVERSION)
		Sys_Error("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

// swap all the lumps
	mod_base = (byte*)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

// load into heap

	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
	Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0();

	mod->numframes = 2;		// regular and alternate animation
	mod->flags = 0;

//
// set up the submodels (FIXME: this is confusing)
//
	for (i = 0; i < mod->numsubmodels; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j = 1; j < MAX_MAP_HULLS; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy(bm->maxs, mod->maxs);
		VectorCopy(bm->mins, mod->mins);

		mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{	// duplicate the basic information
			char	name[10];

			sprintf(name, "*%i", i + 1);
			loadmodel = Mod_FindName(name);
			*loadmodel = *mod;
			strcpy(loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

aliashdr_t* pheader;

//=========================================================

/*
===============
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
===============
*/

typedef struct
{
	short		x, y;
} floodfill_t;


// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

//=========================================================================

//=============================================================================

byte* pspritepal;

/*
===============
Mod_LoadSpriteFrame
===============
*/
void* Mod_LoadSpriteFrame( void* pin, mspriteframe_t** ppframe, int framenum )
{
	dspriteframe_t* pinframe;
	mspriteframe_t* pspriteframe;
	int					width, height, size, origin[2], textureType;
	int                 rawWidth, rawHeight;
	int                 rawOrigin[2];
	byte* pdata, * ppal;
	char				name[MAX_QPATH];
	byte				bPal[768];
	
	memcpy(bPal, pspritepal, sizeof(bPal));

	pinframe = (dspriteframe_t*)pin;

	memcpy(&rawWidth,  &pinframe->width,  sizeof(rawWidth));
	memcpy(&rawHeight, &pinframe->height, sizeof(rawHeight));
	width = LittleLong(rawWidth);
	height = LittleLong(rawHeight);
	size = width * height;

	pspriteframe = (mspriteframe_t*)Hunk_AllocName(sizeof(mspriteframe_t), loadname);
	Q_memset(pspriteframe, 0, sizeof(mspriteframe_t));

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;

	memcpy(&rawOrigin[0], &pinframe->origin[0], sizeof(rawOrigin[0]));
	memcpy(&rawOrigin[1], &pinframe->origin[1], sizeof(rawOrigin[1]));
	origin[0] = LittleLong(rawOrigin[0]);
	origin[1] = LittleLong(rawOrigin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = origin[0] + width;

	sprintf(name, "%s_%i", loadmodel->name, framenum);
	pdata = (byte*)(pinframe + 1);
	ppal = bPal;

	// Get the sprite texture type
	switch (gSpriteTextureFormat)
	{
	case SPR_NORMAL:
	case SPR_ADDITIVE:
		textureType = TEX_TYPE_NONE;
		break;
	case SPR_INDEXALPHA:
		textureType = TEX_TYPE_ALPHA_GRADIENT;
		break;
	case SPR_ALPHTEST:
		textureType = TEX_TYPE_ALPHA;
		break;
	default:
		textureType = TEX_TYPE_ALPHA_GRADIENT;
		break;
	}

	if (gSpriteMipMap)
		pspriteframe->gl_texturenum = GL_LoadTexture(name, GLT_SPRITE, width, height, pdata, FALSE, textureType, ppal);
	else
		pspriteframe->gl_texturenum = GL_LoadTexture(name, GLT_HUDSPRITE, width, height, pdata, FALSE, textureType, ppal);

	return (void*)((byte*)pinframe + sizeof(dspriteframe_t) + size);
}

/*
===============
Mod_LoadSpriteGroup
===============
*/
void* Mod_LoadSpriteGroup( void* pin, mspriteframe_t** ppframe, int framenum )
{
	dspritegroup_t* pingroup;
	mspritegroup_t* pspritegroup;
	int					i, numframes;
	dspriteinterval_t* pin_intervals;
	float* poutintervals;
	void* ptemp;

	pingroup = (dspritegroup_t*)pin;

	numframes = LittleLong(pingroup->numframes);

	pspritegroup = (mspritegroup_t*)Hunk_AllocName(sizeof(mspritegroup_t) +
		(numframes - 1) * sizeof(pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t*)pspritegroup;

	pin_intervals = (dspriteinterval_t*)(pingroup + 1);

	poutintervals = (float*)Hunk_AllocName(numframes * sizeof(float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error("Mod_LoadSpriteGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void*)pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i], framenum * 100 + i);
	}

	return ptemp;
}


/*
===============
Mod_LoadSpriteModel
===============
*/
void Mod_LoadSpriteModel( model_t* mod, void* buffer )
{
	int					i;
	int					version;
	dsprite_t* pin;
	msprite_t* psprite;
	int					numframes;
	int					size;
	int					palsize;
	dspriteframetype_t* pframetype;

	pin = (dsprite_t*)buffer;

	version = LittleLong(pin->version);
	if (version != SPRITE_VERSION)
		Sys_Error("%s has wrong version number (%i should be %i)",
			mod->name, version, SPRITE_VERSION);

	numframes = LittleLong(pin->numframes);
	// Skip header and read palette size in 16bit mode
	palsize = (*(word*)((byte*)buffer + sizeof(dsprite_t))) * 3 + 2;
	size = sizeof(msprite_t) + (numframes - 1) * sizeof(psprite->frames);
	psprite = (msprite_t*)Hunk_AllocName(size + palsize, loadname);
	mod->cache.data = psprite;

	psprite->type = LittleLong(pin->type);

	psprite->texFormat = LittleLong(pin->texFormat);
	gSpriteTextureFormat = psprite->texFormat;

	psprite->maxwidth = LittleLong(pin->width);
	psprite->maxheight = LittleLong(pin->height);
	psprite->beamlength = LittleFloat(pin->beamlength);

	mod->synctype = (synctype_t)LittleLong(pin->synctype);

	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
	mod->mins[2] = -psprite->maxheight / 2;
	mod->maxs[2] = psprite->maxheight / 2;

	psprite->paloffset = numframes * sizeof(psprite->frames) + 30;
	pspritepal = (byte*)(psprite->frames + numframes);

	memcpy(psprite->frames + numframes, (byte*)(pin + 1) + 2, palsize);

//
// load the frames
//
	if (numframes < 1)
		Sys_Error("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

	mod->numframes = numframes;
	mod->flags = 0;

	pframetype = (dspriteframetype_t*)((byte*)(pin + 1) + palsize);

	for (i = 0; i < numframes; i++)
	{
		spriteframetype_t	frametype;
		int                 rawtype;


		memcpy(&rawtype, &pframetype->type, sizeof(rawtype));
		frametype = (spriteframetype_t)LittleLong(rawtype);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteFrame(pframetype + 1,
					&psprite->frames[i].frameptr, i);
		}
		else
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteGroup(pframetype + 1,
					&psprite->frames[i].frameptr, i);
		}
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print( void )
{
	int		i;
	model_t* mod;

	Con_Printf("Cached models:\n");
	for (i = 0; i < mod_numknown; i++)
	{
		mod = mod_known[i];
		if (mod)
			Con_Printf("%8p : %s\n", mod->cache.data, mod->name);
	}
}
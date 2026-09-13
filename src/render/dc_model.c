// gl_model.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "decal.h"
#include "textures.h"
#include "gl_water.h"
#include "dc_draw.h"
#include "qgl.h"
#include "kzap.h"
#include "crc.h"

#pragma intrinsic(fabsf)

model_t*	loadmodel;
char		loadname[32];	// for hunk tags
char*		wadpath;

void Mod_LoadSpriteModel( model_t* mod, void* buffer );
void Mod_LoadBrushModel( model_t* mod, void* buffer );
void Mod_LoadStudioModel( model_t* mod, void* buffer );
void Mod_LoadStudioNeoModel( model_t* mod, void* buffer );
model_t* Mod_LoadModel( model_t* mod, qboolean crash, qboolean bDefer );
model_t* Mod_LoadModelWorldPiecewise( model_t* mod, qboolean crash );

model_t*	mod_known[MAX_MODELS];
int			mod_numknown;

#define MAX_SPRITE_TEXTURES	8

int			gSpriteTextureFormat = SPR_NORMAL;

// Frames that resolve to the same bucket share one upload; this holds the last
// texture handed out so the next frame can point at it instead of loading again.
int			gLastSpriteTexture;

//
// Each lump of the world lives in its own chunk on the disc, named after the map
// with the lump appended.
//
char		chunk_textures[MAX_QPATH];
char		chunk_palettes[MAX_QPATH];
char		chunk_lighting[MAX_QPATH];
char		chunk_visdata[MAX_QPATH];
char		chunk_entities[MAX_QPATH];
char		chunk_vertexes[MAX_QPATH];
char		chunk_submodels[MAX_QPATH];
char		chunk_edges[MAX_QPATH];
char		chunk_texinfo[MAX_QPATH];
char		chunk_faces[MAX_QPATH];
char		chunk_nodes[MAX_QPATH];
char		chunk_leafs[MAX_QPATH];
char		chunk_clipnodes[MAX_QPATH];
char		chunk_hull[MAX_QPATH];
char		chunk_marksurfs[MAX_QPATH];
char		chunk_planes[MAX_QPATH];
char		chunk_surfedges[MAX_QPATH];

/*
=================
Mod_BuildChunkNames
=================
*/
static __forceinline void Mod_BuildChunkNames( char* name )
{
	COM_FileBase(name, loadname);

	strcpy(chunk_textures, loadname);
	strcpy(chunk_palettes, loadname);
	strcpy(chunk_lighting, loadname);
	strcpy(chunk_visdata, loadname);
	strcpy(chunk_entities, loadname);
	strcpy(chunk_vertexes, loadname);
	strcpy(chunk_submodels, loadname);
	strcpy(chunk_edges, loadname);
	strcpy(chunk_texinfo, loadname);
	strcpy(chunk_faces, loadname);
	strcpy(chunk_nodes, loadname);
	strcpy(chunk_leafs, loadname);
	strcpy(chunk_clipnodes, loadname);
	strcpy(chunk_hull, loadname);
	strcpy(chunk_marksurfs, loadname);
	strcpy(chunk_planes, loadname);
	strcpy(chunk_surfedges, loadname);

	strcat(chunk_textures, ":textures");
	strcat(chunk_palettes, ":palettes");
	strcat(chunk_lighting, ":lighting");
	strcat(chunk_visdata, ":visdata");
	strcat(chunk_entities, ":entities");
	strcat(chunk_vertexes, ":vertexes");
	strcat(chunk_submodels, ":submodels");
	strcat(chunk_edges, ":edges");
	strcat(chunk_texinfo, ":texinfo");
	strcat(chunk_faces, ":faces");
	strcat(chunk_nodes, ":nodes");
	strcat(chunk_leafs, ":leafs");
	strcat(chunk_clipnodes, ":clipnodes");
	strcat(chunk_hull, ":hull");
	strcat(chunk_marksurfs, ":marksurfs");
	strcat(chunk_planes, ":planes");
	strcat(chunk_surfedges, ":surfedges");
}


extern qboolean gSpriteMipMap;

//
// A cache slot whose data pointer has the low bit set is only a placeholder,
// so it does not count as loaded model data.
//
#define MOD_CACHEDATA(mod)	(((unsigned)(mod)->cache.data & 1) ? NULL : (mod)->cache.data)

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void* Mod_Extradata( model_t* mod )
{
	void*		r;

	r = Cache_Check(&mod->cache);
	if (r)
		return r;

	Mod_LoadModel(mod, TRUE, FALSE);

	if (!MOD_CACHEDATA(mod))
		Sys_Error("Mod_Extradata: caching failed");

	return MOD_CACHEDATA(mod);
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t* Mod_PointInLeaf( vec_t* p, model_t* model )
{
	mnode_t*	node;
	float		d;
	mclipplane_t* plane;

	if (!model || !model->nodes)
		Sys_Error("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		plane = node->plane;
		d = DotProduct(p, g_planeNormalTable[plane->normalindex].normal) - plane->dist;
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
	int			i;

	for (i = 0; i < mod_numknown; i++)
	{
		if (mod_known[i]->type != mod_alias && mod_known[i]->needload != NL_CLIENT)
			mod_known[i]->needload = NL_NEEDS_LOADED;
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
	int			i;
	model_t** pmod;

	if (!name[0])
		Sys_Error("Mod_FindName: NULL name");

	pmod = mod_known;
	for (i = 0; i < mod_numknown; i++, pmod++)
	{
		if (!Q_stricmp((*pmod)->name, name))
			break;
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MODELS)
			Sys_Error("mod_numknown == MAX_MODELS");

		mod_known[i] = (model_t*)MnemoAlloc(sizeof(model_t), 0x20, 0, "mod_known");
		memset(mod_known[i], 0, sizeof(model_t));
		strcpy(mod_known[i]->name, name);
		mod_known[i]->needload = NL_NEEDS_LOADED;
		mod_numknown++;
	}

	return mod_known[i];
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel( char* name )
{
	model_t*	mod;

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
	unsigned*	buf;
	char		taskname[128];
	byte		stackbuf[1024];		// avoid dirtying the cache heap

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
		if (mod->needload == NL_PRESENT || mod->needload == NL_CLIENT)
			return mod;		// not cached at all
	}

//
// the world is far too big to hold in memory in one piece, so it comes in a
// lump at a time instead of through the usual whole-file loader.
//
	if (strstr(mod->name, ".bsp"))
	{
		if (!Mod_LoadModelWorldPiecewise(mod, crash))
			return NULL;

		return mod;
	}
//
// studio models are streamed off the disc on demand, so a precache only needs
// to register the name; the file itself is read the first time the model is
// drawn (through Mod_Extradata).
//
	if (strstr(mod->name, ".mdl") && bDefer)
	{
		mod->type = mod_studio;
		mod->flags = 0;
		mod->cache.data = NULL;
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
			Sys_ErrorColor(RGB565_GREEN, "Mod_NumForName: %s not found", mod->name);

		return NULL;
	}

//
// allocate a new model
//
	Mod_BuildChunkNames(mod->name);

	loadmodel = mod;

//
// fill it in
//
	mod->needload = NL_PRESENT;

// call the apropriate loader
	switch (LittleLong(*(unsigned*)buf))
	{
	case IDPOLYHEADER:
		Sys_Error("Alias models are so 1995!\n");
		break;

	case IDSPRITEHEADER:
		CL_UpdateProgressBar();
		Mod_LoadSpriteModel(mod, buf);
		sprintf(taskname, "Loaded sprite model: %16s", mod->name);
		CL_SetProgressName(taskname);
		break;

	case IDSTUDIOHEADER:
		CL_UpdateProgressBar();
		Mod_LoadStudioModel(mod, buf);
		sprintf(taskname, "Loaded studio model: %16s", mod->name);
		CL_SetProgressName(taskname);
		break;

	case IDSTUDIONEOHEADER:
		CL_UpdateProgressBar();
		Mod_LoadStudioNeoModel(mod, buf);
		sprintf(taskname, "Loaded neo model: %16s", mod->name);
		CL_SetProgressName(taskname);
		break;

	default:
		CL_UpdateProgressBar();
		Mod_LoadBrushModel(mod, buf);
		sprintf(taskname, "Loaded brush model: %16s", mod->name);
		CL_SetProgressName(taskname);
		break;
	}

	_FreeBlock();

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
	model_t*	mod;

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
	model_t*	mod;

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

byte*		mod_base;

#define MIPSCALE			(64 + 16 + 4 + 1)
#define PIXELS_SIZE			(MIPSCALE * (512 * 512) / 64)
#define PALETTE_SIZE		(256 * 3) + 2
#define TEXTUREDATA_SIZE	(PIXELS_SIZE + PALETTE_SIZE + MIP_EXTRASIZE + sizeof(miptex_t))
#define TEMP_TEXBUF_INIT	0x5846
#define VQ_CODEBOOK_SIZE	2048		// 256 entries of one 2x2 texel block
#define TEXBUF_SLACK		0x346		// palette, mip counts and the PVR chunk headers

// Texture loads are logged when developer is turned up past 1.
static char	texlogline[1024];
static void* texlogfile;

/*
===============
Mod_LoadTextures
===============
*/
void Mod_LoadTextures( lump_t* l )
{
	int			i, j, pixels, num, max, altmax;
	int			r, g, b;
	int			srcwidth, srcheight;
	miptex_t*	mt;
	texture_t*	tx, * tx2;
	texture_t*	anims[10];
	texture_t*	altanims[10];
	dmiptexlump_t* m;
	texture_t*	texheaders;
	byte*		tempTexData;
	byte*		rawtex;
	byte*		pPal;
	byte*		vq;
	byte*		vqindex;
	int			i2;
	char*		pColon;
	char		perMapWadPath[MAX_OSPATH];
	qboolean	wads_parsed;
	int			hasPerMapWads;
	qboolean	isGbix;

	wads_parsed = false;
	Sys_FloatTime();
	isGbix = false;
	hasPerMapWads = false;

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}

	tempTexData = (byte*)MnemoAlloc(TEMP_TEXBUF_INIT, MNEMO_FLAG_MALLOC, 0, "temp texture buffer");

	m = (dmiptexlump_t*)(mod_base + l->fileofs);
	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = (texture_t**)Hunk_AllocName(m->nummiptex * sizeof(*loadmodel->textures), chunk_textures);

	// every texture header for the map comes out of one block
	texheaders = (texture_t*)Hunk_AllocName(m->nummiptex * sizeof(texture_t), "textureheaders");

	if (TEX_BuildPerMapWadPath(loadname, perMapWadPath))
		hasPerMapWads = true;

	for (i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t*)((byte*)m + m->dataofs[i]);

		if (developer.value > 1)
		{
			if (!i)
			{
				sprintf(texlogline, "\\PC\\%s.log", chunk_textures);
				pColon = strchr(texlogline, ':');
				if (pColon)
					*pColon = '_';

				texlogfile = Sys_OpenHandle(texlogline, "w");
				if (texlogfile)
				{
					sprintf(texlogline, "start\r\n");
					DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
				}
			}

			if (texlogfile)
			{
				sprintf(texlogline, "[%s]\r\n", mt->name);
				DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
			}
		}

		srcwidth = mt->width;
		srcheight = mt->height;

		if (hasPerMapWads || r_wadtextures.value || !LittleLong(mt->offsets[0]))
		{
			if (!wads_parsed)
			{
				if (hasPerMapWads)
					TEX_InitFromWad(perMapWadPath);
				else
					TEX_InitFromWad(wadpath);

				TEX_AddAnimatingTextures();
				wads_parsed = true;
			}

			// room for the whole mip chain plus the palette and header
			pixels = LittleLong(mt->width) * LittleLong(mt->height);
			pixels += (LittleLong(mt->width) >> 1) * (LittleLong(mt->height) >> 1);
			pixels += (LittleLong(mt->width) >> 2) * (LittleLong(mt->height) >> 2);
			pixels += (LittleLong(mt->width) >> 3) * (LittleLong(mt->height) >> 3);

			tempTexData = (byte*)MnemoRealloc(tempTexData, pixels * 4 + TEXBUF_SLACK);

			if (!TEX_LoadLump(mt->name, tempTexData))
			{
				m->dataofs[i] = -1;
				continue;
			}

			isGbix = true;
			if (*(unsigned int*)tempTexData != GBIXHEADER)
			{
				isGbix = false;
				srcwidth = mt->width;
				srcheight = mt->height;
				mt = (miptex_t*)tempTexData;
			}
		}

		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		if ((mt->width & 15) || (mt->height & 15))
			Sys_Error("Texture %s is not 16 aligned", mt->name);

		rawtex = (byte*)mt + sizeof(miptex_t);

		// total amount of pixels across every mip level
		pixels = mt->width * mt->height / 64 * MIPSCALE;

		tx = &texheaders[i];
		loadmodel->textures[i] = tx;

		memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;

		// palette sits behind the mip chain, after its entry count
		pPal = rawtex + pixels + sizeof(word);

		if (isGbix)
		{
			// no palette to pull the water fade out of, so average the first
			// codebook entry the texture actually uses
			vq = tempTexData + 32;
			vqindex = vq + VQ_CODEBOOK_SIZE;
			i2 = vqindex[1] * 8;

			r = ((*(word*)(vq + i2) & 0xF800) + (*(word*)(vq + i2 + 4) & 0xF800)
				+ (*(word*)(vq + i2 + 6) & 0xF800) + (*(word*)(vq + i2 + 2) & 0xF800)) >> 10;
			tx->fade_r = r;
			g = ((*(word*)(vq + i2) & 0x07E0) + (*(word*)(vq + i2 + 4) & 0x07E0)
				+ (*(word*)(vq + i2 + 6) & 0x07E0) + (*(word*)(vq + i2 + 2) & 0x07E0)) >> 5;
			tx->fade_g = g;
			b = ((*(word*)(vq + i2) & 0x001F) + (*(word*)(vq + i2 + 4) & 0x001F)
				+ (*(word*)(vq + i2 + 6) & 0x001F) + (*(word*)(vq + i2 + 2) & 0x001F)) * 2;
			tx->fade_b = b;
			tx->fade_fog = 100 - max(r, max(g, b)) / 3;
		}
		else
		{
			tx->fade_r = pPal[9];
			tx->fade_g = pPal[10];
			tx->fade_b = pPal[11];
			tx->fade_fog = pPal[12];
		}

		if (!Q_strncmp(mt->name, "sky", 3))
			R_ForceLoadSkys();
		else
		{
			texture_mode = GL_LINEAR_MIPMAP_NEAREST;

			if (mt->name[0] == '{')
			{
				if (isGbix)
					Sys_Error("I'm not sure I'm ready to cope with PVR/VQ alpha textures yet.\n");

				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_ALPHA, pPal);
			}
			else if (isGbix)
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, tempTexData, TRUE, TEX_TYPE_GBIX, NULL);
			}
			else
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_NONE, pPal);
			}

			texture_mode = GL_LINEAR;
		}

		// the wad copy owned the sizes we just byte swapped, so put the
		// lump's own back
		if (!isGbix)
		{
			mt->width = LittleLong(srcwidth);
			mt->height = LittleLong(srcheight);
			tx->width = LittleLong(srcwidth);
			tx->height = LittleLong(srcheight);
		}
	}

	if (wads_parsed)
		TEX_CleanupWadInfo();

	if (hasPerMapWads)
		Bremove_path(perMapWadPath);

	CL_SetProgressName("Sequencing texture animations");

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
			anims[max] = tx;
			max++;
			altmax = 0;
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

	if (developer.value > 1 && texlogfile)
	{
		sprintf(texlogline, "end\r\n");
		DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
		Sys_CloseHandle(texlogfile);
		texlogfile = NULL;
	}

	MnemoFree(tempTexData);
}

/*
=================
Mod_TryLoadLt2Lighting

Looks for the map's precomputed lighting alongside it. The payload is slid down
over its own header so the block can be shrunk to exactly what it holds.
=================
*/
static qboolean Mod_TryLoadLt2Lighting( char* name )
{
	char		path[64];
	int*		buf;
	int*		in;
	int*		out;
	int			lightBytes;
	int			surfCount;
	char		subformat;

	subformat = 0;

	sprintf(path, "maps/%s.lt2", name);

	buf = (int*)COM_LoadHunkFile(path);
	if (!buf)
		return FALSE;

	in = buf;
	if (((char*)buf)[0] == 'L' && ((char*)buf)[1] == 'T' && ((char*)buf)[2] == '2')
	{
		subformat = ((char*)buf)[3];
		in = buf + 1;
	}

	lightBytes = *in++;
	loadmodel->lightBytes = lightBytes;
	surfCount = *in++;

	out = buf;
	while (lightBytes > 0)
	{
		*out++ = *in++;
		lightBytes -= 4;
	}

	loadmodel->lightsurfs = (int*)MnemoAlloc(surfCount * 4, 0x20, 0, "LightSurfs");
	memcpy(loadmodel->lightsurfs, in, surfCount * 4);

	MnemoShrink(buf, loadmodel->lightBytes);
	Mnemo_BlockSetName(buf, chunk_lighting);

	loadmodel->lightdata = (color24*)buf;

	if (!subformat)
		loadmodel->lightmap_mode = 2;
	else if (subformat == 'a')
		loadmodel->lightmap_mode = 3;

	return TRUE;
}

void Mod_LoadLighting( lump_t* l )
{
	if (Mod_TryLoadLt2Lighting(loadname))
		return;

	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}

	loadmodel->lightBytes = l->filelen;
	loadmodel->lightdata = (color24*)Hunk_AllocName(l->filelen, chunk_lighting);
	memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
	loadmodel->lightmap_mode = 0;
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
	loadmodel->visdata = (byte*)Hunk_AllocName(l->filelen, chunk_visdata);
	memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities( lump_t* l )
{
	char*		pszInputStream;

	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = (char*)Hunk_AllocName(l->filelen, chunk_entities);
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
	dvertex_t*	in;
	mvertex_t*	out;
	int			i, count;

	in = (dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Hunk_AllocName(count * sizeof(*out), chunk_vertexes);

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
	dmodel_t*	in;
	dmodel_t*	out;
	int			i, j, count;

	in = (dmodel_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dmodel_t*)Hunk_AllocName(count * sizeof(*out), chunk_submodels);

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
	dedge_t*	in;
	medge_t*	out;
	int			i, count;

	in = (dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t*)Hunk_AllocName((count + 1) * sizeof(*out), chunk_edges);

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
	texinfo_t*	in;
	mtexinfo_t*	out;
	int			i, j, count;
	int			miptex;

	in = (texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t*)Hunk_AllocName(count * sizeof(*out), chunk_texinfo);

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
	float		mins[2], maxs[2], val;
	int			i, j, e;
	mvertex_t*	v;
	mtexinfo_t*	tex;
	int			bmins[2], bmaxs[2];

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
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
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
	dface_t*	in;
	msurface_t*	out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t*)Hunk_AllocName(count * sizeof(*out), chunk_faces);
	
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	R_DecalInit();

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

	// the per-face lighting offsets have all been consumed
	if (loadmodel->lightsurfs)
	{
		MnemoFree(loadmodel->lightsurfs);
		loadmodel->lightsurfs = NULL;
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
	dnode_t*	in;
	mnode_t*	out;

	in = (dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Hunk_AllocName(count * sizeof(*out), chunk_nodes);

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
		out->visframe = r_visframecount - 1;

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
	dleaf_t*	in;
	mleaf_t*	out;
	int			i, j, count, p;

	in = (dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Hunk_AllocName(count * sizeof(*out), chunk_leafs);

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
		out->visframe = r_visframecount - 1;

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
	hull_t*		hull;

	in = (dclipnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), chunk_clipnodes);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
		hull->boxplanes = loadmodel->planes;
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
		hull->boxplanes = loadmodel->planes;
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
		hull->boxplanes = loadmodel->planes;
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
	mnode_t*	in, * child;
	dclipnode_t* out;
	int			i, j, count;
	hull_t*		hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), chunk_hull);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->boxplanes = loadmodel->planes;

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
	int			i, j, count;
	short*		in;
	msurface_t** out;

	in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Hunk_AllocName(count * sizeof(*out), chunk_marksurfs);

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
	int			i, count;
	int*		in, * out;

	in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (int*)Hunk_AllocName(count * sizeof(*out), chunk_surfedges);

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

planenormal_t* g_planeNormalTable;
int			normal_count;
int			normal_collisions;

/*
===============
Mod_AddNormalToTable

Returns the index this normal lives at, adding it if it is not there yet.
===============
*/
unsigned short Mod_HashNormal( vec_t *normal, unsigned int hash )
{
	byte *data = (byte *)normal;
	int			i;

	for (i = 0; i < 12; i++)
	{
		hash ^= *data++;
		if (hash & 1)
			hash = (hash >> 1) | 0x0800;
		else
			hash >>= 1;
	}
	return hash & NORMAL_TABLE_MASK;
}

unsigned short Mod_AddNormalToTable( vec_t* normal, unsigned int hash )
{
	planenormal_t* entry;
	unsigned short index;
	int			i;

	index = Mod_HashNormal(normal, hash);
	entry = g_planeNormalTable + index;
	i = NORMAL_TABLE_MASK;

	while (1)
	{
		if (entry->normal[0] > 2.0f)
		{
			entry->normal[0] = normal[0];
			entry->normal[1] = normal[1];
			entry->normal[2] = normal[2];
			entry->padding = 0.0f;
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
		{
			Sys_Error("Normal table full!");
			return index;
		}
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
	vec3_t		normal;
	int			i;

	if (!g_planeNormalTable)
		g_planeNormalTable = (planenormal_t*)MnemoAlloc(NORMAL_TABLE_SIZE * sizeof(planenormal_t), 0x20, 0, "norm table");

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
	mclipplane_t* out;
	dplane_t*	in;
	int			count;
	int			bits;
	vec3_t		normal;

	in = (dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mclipplane_t*)Hunk_AllocName((count + 1) * sizeof(*out), chunk_planes);

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			normal[j] = LittleFloat(in->normal[j]);
			if (normal[j] < 0)
				bits |= 1 << j;
		}

		out->normalindex = Mod_AddNormalToTable(normal, bits);
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
	int			i;
	vec3_t		corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabsf(mins[i]) > fabsf(maxs[i]) ? fabsf(mins[i]) : fabsf(maxs[i]);
	}

	return VectorLength(corner);
}

static qboolean Mod_LoadExternalTextureTable( char* mapPath );

/*
=================
Mod_LoadExternalTextureTable

Loads the Dreamcast sidecar texture table (<map>.tex).  It is an override for
the BSP texture lump, not a generic image-file loader.
=================
*/
static qboolean Mod_LoadExternalTextureTable( char* mapPath )
{
	int			i, j, pixels, num, max, altmax;
	int			r, g, b;
	int			srcwidth, srcheight;
	miptex_t*	mt;
	texture_t*	tx, * tx2;
	texture_t*	anims[10];
	texture_t*	altanims[10];
	dmiptexlump_t* m;
	texture_t*	texheaders;
	byte*		tempTexData;
	byte*		rawtex;
	byte*		pPal;
	byte*		vq;
	byte*		vqindex;
	int			i2;
	char*		pColon;
	char		perMapWadPath[MAX_OSPATH];
	char		texturePath[MAX_OSPATH];
	char*		extension;
	int			length;
	qboolean	wads_parsed;
	int			hasPerMapWads;
	qboolean	isGbix;

	wads_parsed = false;
	Sys_FloatTime();
	isGbix = false;
	hasPerMapWads = false;

	strcpy(texturePath, mapPath);
	COM_StringToLower(texturePath);
	extension = strstr(texturePath, ".bsp");
	if (extension)
		*extension = 0;
	strcat(texturePath, ".tex");

	m = (dmiptexlump_t*)COM_LoadFile(texturePath, 2, &length);
	if (!m)
		return FALSE;

	tempTexData = (byte*)MnemoAlloc(TEMP_TEXBUF_INIT, MNEMO_FLAG_MALLOC, 0, "temp texture buffer");

	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = (texture_t**)Hunk_AllocName(m->nummiptex * sizeof(*loadmodel->textures), chunk_textures);

	// every texture header for the map comes out of one block
	texheaders = (texture_t*)Hunk_AllocName(m->nummiptex * sizeof(texture_t), "textureheaders");

	if (TEX_BuildPerMapWadPath(loadname, perMapWadPath))
		hasPerMapWads = true;

	for (i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t*)((byte*)m + m->dataofs[i]);

		if (developer.value > 1)
		{
			if (!i)
			{
				sprintf(texlogline, "\\PC\\%s.log", chunk_textures);
				pColon = strchr(texlogline, ':');
				if (pColon)
					*pColon = '_';

				texlogfile = Sys_OpenHandle(texlogline, "w");
				if (texlogfile)
				{
					sprintf(texlogline, "start\r\n");
					DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
				}
			}

			if (texlogfile)
			{
				sprintf(texlogline, "[%s]\r\n", mt->name);
				DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
			}
		}

		srcwidth = mt->width;
		srcheight = mt->height;

		if (LittleLong(mt->offsets[0]))
			Sys_Error("Look, the whole point of the compact texture data is that it DOESN'T include the textures themselves....\n");

		if (hasPerMapWads || r_wadtextures.value || !LittleLong(mt->offsets[0]))
		{
			if (!wads_parsed)
			{
				if (hasPerMapWads)
					TEX_InitFromWad(perMapWadPath);
				else
					TEX_InitFromWad(wadpath);

				TEX_AddAnimatingTextures();
				wads_parsed = true;
			}

			// room for the whole mip chain plus the palette and header
			pixels = LittleLong(mt->width) * LittleLong(mt->height);
			pixels += (LittleLong(mt->width) >> 1) * (LittleLong(mt->height) >> 1);
			pixels += (LittleLong(mt->width) >> 2) * (LittleLong(mt->height) >> 2);
			pixels += (LittleLong(mt->width) >> 3) * (LittleLong(mt->height) >> 3);

			tempTexData = (byte*)MnemoRealloc(tempTexData, pixels * 4 + TEXBUF_SLACK);

			if (!TEX_LoadLump(mt->name, tempTexData))
			{
				m->dataofs[i] = -1;
				continue;
			}

			isGbix = true;
			if (*(unsigned int*)tempTexData != GBIXHEADER)
			{
				isGbix = false;
				srcwidth = mt->width;
				srcheight = mt->height;
				mt = (miptex_t*)tempTexData;
			}
		}

		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		if ((mt->width & 15) || (mt->height & 15))
			Sys_Error("Texture %s is not 16 aligned", mt->name);

		rawtex = (byte*)mt + sizeof(miptex_t);

		// total amount of pixels across every mip level
		pixels = mt->width * mt->height / 64 * MIPSCALE;

		tx = &texheaders[i];
		loadmodel->textures[i] = tx;

		memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;

		// palette sits behind the mip chain, after its entry count
		pPal = rawtex + pixels + sizeof(word);

		if (isGbix)
		{
			// no palette to pull the water fade out of, so average the first
			// codebook entry the texture actually uses
			vq = tempTexData + 32;
			vqindex = vq + VQ_CODEBOOK_SIZE;
			i2 = vqindex[1] * 8;

			r = ((*(word*)(vq + i2) & 0xF800) + (*(word*)(vq + i2 + 4) & 0xF800)
				+ (*(word*)(vq + i2 + 6) & 0xF800) + (*(word*)(vq + i2 + 2) & 0xF800)) >> 10;
			tx->fade_r = r;
			g = ((*(word*)(vq + i2) & 0x07E0) + (*(word*)(vq + i2 + 4) & 0x07E0)
				+ (*(word*)(vq + i2 + 6) & 0x07E0) + (*(word*)(vq + i2 + 2) & 0x07E0)) >> 5;
			tx->fade_g = g;
			b = ((*(word*)(vq + i2) & 0x001F) + (*(word*)(vq + i2 + 4) & 0x001F)
				+ (*(word*)(vq + i2 + 6) & 0x001F) + (*(word*)(vq + i2 + 2) & 0x001F)) * 2;
			tx->fade_b = b;
			tx->fade_fog = 100 - max(r, max(g, b)) / 3;
		}
		else
		{
			tx->fade_r = pPal[9];
			tx->fade_g = pPal[10];
			tx->fade_b = pPal[11];
			tx->fade_fog = pPal[12];
		}

		if (!Q_strncmp(mt->name, "sky", 3))
			R_ForceLoadSkys();
		else
		{
			texture_mode = GL_LINEAR_MIPMAP_NEAREST;

			if (mt->name[0] == '{')
			{
				if (isGbix)
					Sys_Error("I'm not sure I'm ready to cope with PVR/VQ alpha textures yet.\n");

				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_ALPHA, pPal);
			}
			else if (isGbix)
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, tempTexData, TRUE, TEX_TYPE_GBIX, NULL);
			}
			else
			{
				tx->gl_texturenum = GL_LoadTexture(mt->name, GLT_WORLD, tx->width, tx->height, rawtex, TRUE, TEX_TYPE_NONE, pPal);
			}

			texture_mode = GL_LINEAR;
		}

		// the wad copy owned the sizes we just byte swapped, so put the
		// lump's own back
		if (!isGbix)
		{
			mt->width = LittleLong(srcwidth);
			mt->height = LittleLong(srcheight);
			tx->width = LittleLong(srcwidth);
			tx->height = LittleLong(srcheight);
		}
	}

	if (wads_parsed)
		TEX_CleanupWadInfo();

	if (hasPerMapWads)
		Bremove_path(perMapWadPath);

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
			anims[max] = tx;
			max++;
			altmax = 0;
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

	if (developer.value > 1 && texlogfile)
	{
		sprintf(texlogline, "end\r\n");
		DC_fwrite(texlogline, strlen(texlogline), 1, texlogfile);
		Sys_CloseHandle(texlogfile);
		texlogfile = NULL;
	}

	MnemoFree(tempTexData);
	COM_FreeTempFile();
	return TRUE;
}

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
	byte*		buf;
	char*		pszInputStream;

	buf = (byte*)Hunk_AllocName(header->lumps[LUMP_ENTITIES].filelen + 0x21, chunk_entities);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, header->lumps[LUMP_ENTITIES].fileofs,
		(header->lumps[LUMP_ENTITIES].filelen + 31) & ~31);
	mod_base = buf - header->lumps[LUMP_ENTITIES].fileofs;

	if (!header->lumps[LUMP_ENTITIES].filelen)
	{
		loadmodel->entities = NULL;
		return FALSE;
	}

	loadmodel->entities = (char*)buf;

	pszInputStream = COM_Parse(loadmodel->entities);
	if (*pszInputStream && com_token[0] != '}')
	{
		while (1)
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
			if (!*pszInputStream || com_token[0] == '}')
				break;
		}
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
	byte*		buf;

	buf = (byte*)Hunk_AllocName(header->lumps[LUMP_VISIBILITY].filelen + 0x21, chunk_visdata);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, header->lumps[LUMP_VISIBILITY].fileofs, (header->lumps[LUMP_VISIBILITY].filelen + 31) & ~31);
	mod_base = buf - header->lumps[LUMP_VISIBILITY].fileofs;

	if (!header->lumps[LUMP_VISIBILITY].filelen)
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
	byte*		buf;

	if (Mod_TryLoadLt2Lighting(loadname))
		return TRUE;

	buf = (byte*)Hunk_AllocName(header->lumps[LUMP_LIGHTING].filelen + 0x21, chunk_lighting);
	if (!buf)
		return FALSE;

	COM_LoadFileChunk(path, buf, header->lumps[LUMP_LIGHTING].fileofs, (header->lumps[LUMP_LIGHTING].filelen + 31) & ~31);
	mod_base = buf - header->lumps[LUMP_LIGHTING].fileofs;

	if (!header->lumps[LUMP_LIGHTING].filelen)
	{
		loadmodel->lightdata = NULL;
		return FALSE;
	}

	loadmodel->lightBytes = header->lumps[LUMP_LIGHTING].filelen;
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
	lump_t*		l;
	byte*		buf;

	l = &header->lumps[lumpnum];

	buf = (byte*)Hunk_TempAlloc(l->filelen + 0x21);
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
		if (!Mod_LoadExternalTextureTable(path))
		{
			CL_SetProgressName("Looked for compact texture info");
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
			loadmodel->visdata = (byte*)Hunk_AllocName(l->filelen, chunk_visdata);
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
		if (!Mod_TryLoadLt2Lighting(loadname))
		{
			if (!l->filelen)
			{
				loadmodel->lightdata = NULL;
			}
			else
			{
				loadmodel->lightBytes = l->filelen;
				loadmodel->lightdata = (color24*)Hunk_AllocName(l->filelen, chunk_lighting);
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

	_FreeBlock();
	return TRUE;
}

/*
=================
Mod_LoadModelWorldPiecewise

Loads the world one lump at a time, reporting progress as it goes, and finishes
by splitting the submodels off into their own entries.
=================
*/
model_t* Mod_LoadModelWorldPiecewise( model_t* mod, qboolean crash )
{
	dheader_t	header;
	dmodel_t*	bm;
	model_t*	world;
	int			i, j;
	int			version;

	Mod_BuildChunkNames(mod->name);

	world = mod;
	loadmodel = mod;

	mod->type = mod_brush;
	mod->needload = NL_PRESENT;

	COM_LoadFileChunk(mod->name, (byte*)&header, 0, sizeof(header));

	version = LittleLong(header.version);
	if (version != Q1BSP_VERSION && version != BSPVERSION)
	{
		Sys_Error("Mod_LoadModelWorldPiecewise: %s has wrong version number (%i should be %i)",
			mod->name, version, BSPVERSION);
	}

	CL_SetProgressName("World header loaded");

	// swap all the lumps
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)&header)[i] = LittleLong(((int*)&header)[i]);

	if (!Mod_LoadEntitiesChunk(mod->name, &header))
		return NULL;
	CL_SetProgressName("Loaded world entities");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_TEXTURES, &header))
		return NULL;
	CL_SetProgressName("Loaded world textures");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_VERTEXES, &header))
		return NULL;
	CL_SetProgressName("Loaded world vertices");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_EDGES, &header))
		return NULL;
	CL_SetProgressName("Loaded world edges");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_SURFEDGES, &header))
		return NULL;
	CL_SetProgressName("Loaded world surfedges");

	if (!Mod_LoadLightingChunk(mod->name, &header))
		return NULL;
	CL_SetProgressName("Loaded world lighting");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_PLANES, &header))
		return NULL;
	CL_SetProgressName("Loaded world planes");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_TEXINFO, &header))
		return NULL;
	CL_SetProgressName("Loaded world texinfo");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_FACES, &header))
		return NULL;
	CL_SetProgressName("Loaded world faces");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_MARKSURFACES, &header))
		return NULL;
	CL_SetProgressName("Loaded world marksurfs");

	if (!Mod_LoadVisibilityChunk(mod->name, &header))
		return NULL;
	CL_SetProgressName("Loaded world visibility");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_LEAFS, &header))
		return NULL;
	CL_SetProgressName("Loaded world leaves");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_NODES, &header))
		return NULL;
	CL_SetProgressName("Loaded world nodes");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_CLIPNODES, &header))
		return NULL;
	CL_SetProgressName("Loaded world clipnodes");

	if (!Mod_LoadWorldChunk(mod->name, LUMP_MODELS, &header))
		return NULL;
	CL_SetProgressName("Loaded world models");

	Mod_MakeHull0();

	mod->numframes = 2;		// regular and alternate animation
	mod->flags = 0;

	CL_SetProgressName("Loaded entire world");

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
			char		name[10];

			sprintf(name, "*%i", i + 1);
			loadmodel = Mod_FindName(name);
			*loadmodel = *mod;
			strcpy(loadmodel->name, name);
			mod = loadmodel;
		}
	}

	CL_SetProgressName("Set up world submodels");

	return world;
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel( model_t* mod, void* buffer )
{
	int			i, j;
	dheader_t*	header;
	dmodel_t*	bm;

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
			char		name[10];

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

aliashdr_t*	pheader;

//=========================================================

byte*		pspritepal;

/*
===============
Mod_LoadSpriteFrame
===============
*/
void Mod_SpriteTextureName( char *name, const char *modelname, int frame )
{
	sprintf(name, "%s_%i", modelname, frame);
}

void* Mod_LoadSpriteFrame( void* pin, mspriteframe_t** ppframe, int framenum, int bReuseTexture )
{
	dspriteframe_t* pinframe;
	mspriteframe_t* pspriteframe;
	int			width, height, size, origin[2], textureType;
	byte*		pdata, * ppal;
	char		name[256];
	byte		bPal[768];
	
	memcpy(bPal, pspritepal, sizeof(bPal));

	pinframe = (dspriteframe_t*)pin;

	width = LittleLong(LoadUnalignedLong(&pinframe->width));
	height = LittleLong(LoadUnalignedLong(&pinframe->height));
	size = width * height;

	pspriteframe = (mspriteframe_t*)Hunk_AllocName(sizeof(mspriteframe_t), loadname);
	Q_memset(pspriteframe, 0, sizeof(mspriteframe_t));

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;

	origin[0] = LittleLong(LoadUnalignedLong(&pinframe->origin[0]));
	origin[1] = LittleLong(LoadUnalignedLong(&pinframe->origin[1]));

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = origin[0] + width;

	Mod_SpriteTextureName(name, loadmodel->name, framenum);
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
	}

	if (bReuseTexture)
	{
		pspriteframe->gl_texturenum = gLastSpriteTexture;
	}
	else
	{
		if (gSpriteMipMap)
			pspriteframe->gl_texturenum = GL_LoadTexture(name, GLT_SPRITE, width, height, pdata, FALSE, textureType, ppal);
		else
			pspriteframe->gl_texturenum = GL_LoadTexture(name, GLT_HUDSPRITE, width, height, pdata, FALSE, textureType, ppal);
	}

	gLastSpriteTexture = pspriteframe->gl_texturenum;

	return (void*)((byte*)pinframe + sizeof(dspriteframe_t) + size);
}

/*
===============
Mod_LoadSpriteGroup
===============
*/
void* Mod_LoadSpriteGroup( void* pin, mspriteframe_t** ppframe, int framenum, int bReuseTexture )
{
	dspritegroup_t* pingroup;
	mspritegroup_t* pspritegroup;
	int			i, numframes;
	dspriteinterval_t* pin_intervals;
	float*		poutintervals;
	void*		ptemp;

	pingroup = (dspritegroup_t*)pin;

	numframes = LittleLong(LoadUnalignedLong(&pingroup->numframes));

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
		if (*poutintervals <= 0.0f)
			Sys_Error("Mod_LoadSpriteGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void*)pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i], framenum * 100 + i, bReuseTexture);
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
	int			i;
	int			version;
	dsprite_t*	pin;
	msprite_t*	psprite;
	int			numframes;
	int			maxtextures;
	int			lastbucket;
	int			size;
	int			palsize;
	dspriteframetype_t* pframetype;

	pin = (dsprite_t*)buffer;

	version = LittleLong(LoadUnalignedLong(&pin->version));
	if (version != SPRITE_VERSION)
		Sys_Error("%s has wrong version number (%i should be %i)",
			mod->name, version, SPRITE_VERSION);

	numframes = LittleLong(LoadUnalignedLong(&pin->numframes));

	// A sprite gets at most this many distinct uploads, however many frames it has
	if (numframes > MAX_SPRITE_TEXTURES)
		maxtextures = MAX_SPRITE_TEXTURES;
	else
		maxtextures = numframes;
	// Skip header and read palette size in 16bit mode
	palsize = (*(short*)((byte*)buffer + sizeof(dsprite_t))) * 3 + 2;
	size = sizeof(msprite_t) + (numframes - 1) * sizeof(psprite->frames);
	psprite = (msprite_t*)Hunk_AllocName(size + palsize, loadname);
	mod->cache.data = psprite;

	psprite->type = LittleLong(LoadUnalignedLong(&pin->type));

	psprite->texFormat = LittleLong(LoadUnalignedLong(&pin->texFormat));
	gSpriteTextureFormat = psprite->texFormat;

	psprite->maxwidth = LittleLong(LoadUnalignedLong(&pin->width));
	psprite->maxheight = LittleLong(LoadUnalignedLong(&pin->height));
	psprite->beamlength = LittleFloat(pin->beamlength);

	mod->synctype = (synctype_t)LittleLong(pin->synctype);

	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
	mod->mins[2] = -psprite->maxheight / 2;
	mod->maxs[2] = psprite->maxheight / 2;

	psprite->paloffset = size + 2;
	pspritepal = (byte*)psprite + size;

	memcpy(pspritepal, (byte*)buffer + sizeof(dsprite_t) + 2, palsize);

//
// load the frames
//
	if (numframes < 1)
		Sys_Error("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

	mod->numframes = numframes;
	mod->flags = 0;

	pframetype = (dspriteframetype_t*)((byte*)buffer + sizeof(dsprite_t) + palsize);
	lastbucket = -1;

	for (i = 0; i < numframes; i++)
	{
		spriteframetype_t frametype;
		int			bucket;
		int			bReuseTexture;

		bucket = i;
		if (numframes > 1)
			bucket = ((maxtextures - 1) * i) / (numframes - 1);

		// frames that land in the same bucket share the one upload
		bReuseTexture = (lastbucket == bucket);
		lastbucket = bucket;

		frametype = (spriteframetype_t)LittleLong(LoadUnalignedLong(&pframetype->type));
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteFrame(pframetype + 1,
					&psprite->frames[i].frameptr, i, bReuseTexture);
		}
		else
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteGroup(pframetype + 1,
					&psprite->frames[i].frameptr, i, bReuseTexture);
		}
	}

	mod->type = mod_sprite;
}

/*
=================
Mod_UnloadSpriteTextures
=================
*/
void Mod_UnloadSpriteTextures( model_t* mod )
{
	msprite_t*	sprite;
	char		name[256];
	int			i;

	if (mod->type != mod_sprite)
		return;

	mod->needload = NL_NEEDS_LOADED;
	sprite = (msprite_t*)MOD_CACHEDATA(mod);
	if (!sprite)
		return;

	for (i = 0; i < sprite->numframes; i++)
	{
		sprintf(name, "%s_%i", mod->name, i);
		DC_ForceFreeTextureByName(name);
	}
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print( void )
{
	int			i;
	model_t*	mod;

	Con_Printf("Cached models:\n");
	for (i = 0; i < mod_numknown; i++)
	{
		mod = mod_known[i];
		Con_Printf("%8p : %s\n", mod->cache.data, mod->name);
	}
}

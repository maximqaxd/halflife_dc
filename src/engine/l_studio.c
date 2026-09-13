// l_studio.c - studio model loading

#include "quakedef.h"
#include "studio.h"

int giTextureSize;

extern char loadname[32];
extern void CL_PollProgressBar( void );
extern void DC_TouchTexture( int texnum );

//=============================================================================

/*
=================
Mod_LoadStudioModel
=================
*/
void Mod_LoadStudioModel( model_t* mod, void* buffer )
{
	int					i;
	byte* pin;
	studiohdr_t* phdr;
	mstudiotexture_t* ptexture;
	int					total;
	int					version;
	byte* pData;
	byte* pPal;

	pin = (byte*)buffer;
	phdr = (studiohdr_t*)pin;

	version = LittleLong(phdr->version);
	if (version != STUDIO_VERSION)
	{
		memset(phdr, 0, sizeof(*phdr));
		strcpy(phdr->name, "bogus");
		phdr->length = sizeof(*phdr);
		phdr->texturedataindex = sizeof(*phdr);
	}
	phdr->version = 0xC0EDBEEF;

	if (phdr->numbones > MAXSTUDIOBONES)
		Sys_Error("Too damn many bones in %s!\n", mod->name);

	mod->type = mod_studio;
	mod->flags = phdr->flags;

	// the raw texture data doesn't need to stay resident once it has been
	// uploaded, so only the header and texture directory get cached
	total = phdr->length;
	if (phdr->textureindex != 0)
		total -= (phdr->length - phdr->texturedataindex);

	Cache_Alloc(&mod->cache, total, mod->name);
	if (!mod->cache.data)
		return;

	if (phdr->textureindex != 0 && phdr->numtextures > 0)
	{
		ptexture = (mstudiotexture_t*)(pin + phdr->textureindex);

		for (i = 0; i < phdr->numtextures; i++, ptexture++)
		{
			char name[260];
			int texofs;
			int texBytes;

			CL_PollProgressBar();

			strcpy(name, mod->name);
			strcat(name, ptexture->name);

			if (ptexture->index & 3)
		Sys_Error("Model %s has misaligned textures - run it through a newer studiomdl.exe!", mod->name);

			texofs = ptexture->index;
			pData = pin + texofs;
			if (*(int*)pData == 0x58494247) /* "GBIX" - PVR textures */
			{
				// PVR textures carry their own dimensions in the texture data
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, *(short*)(pData + 0x1C), *(short*)(pData + 0x1E),
					pData, FALSE, TEX_TYPE_GBIX, NULL);
			}
			else
			{
				texBytes = ptexture->width * ptexture->height;

				pPal = pin + texofs + texBytes;
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, ptexture->width, ptexture->height,
					pData, FALSE, TEX_TYPE_NONE, pPal);
			}

			DC_TouchTexture(ptexture->index);
		}
	}

	memcpy(mod->cache.data, pin, total);
}

int Mod_IsStudioNeoModel( const studiohdr_t* phdr )
{
	if (phdr->version == 0xC0EDBABE)
		return TRUE;
	if (phdr->version == 0xC0EDBEEF)
		return FALSE;
	return FALSE;
}


void Mod_FreeStudioTextures( void* buffer )
{
	studiohdr_t* phdr = (studiohdr_t*)buffer;
	mstudiotexture_t* texture;
	int			i;

	if (phdr->version == 0xC0EDBEEF || phdr->version == 0xC0EDBABE)
	{
		if (phdr->textureindex != 0)
		{
			texture = (mstudiotexture_t*)((byte*)phdr + phdr->textureindex);
			for (i = 0; i < phdr->numtextures; i++, texture++)
				DC_ReleaseTexture(texture->index);
		}
	}
}

void Mod_TouchStudioTextures( void* buffer )
{
	studiohdr_t* phdr = (studiohdr_t*)buffer;
	mstudiotexture_t* texture;
	int			i;

	if (phdr->version == 0xC0EDBEEF || phdr->version == 0xC0EDBABE)
	{
		if (Mod_IsStudioNeoModel(phdr))
			Mod_TouchStudioNeoTextures(buffer);
		else if (phdr->textureindex != 0)
		{
			texture = (mstudiotexture_t*)((byte*)phdr + phdr->textureindex);
			for (i = 0; i < phdr->numtextures; i++, texture++)
				DC_TouchTexture(texture->index);
		}
	}
}

/*
=================
Mod_LoadStudioNeoModel

The Dreamcast's Neo studio path uses the same on-disk studio header and PVR
upload rules as the normal studio loader.  It remains a separate entry point
because model dispatch selects it by the Neo model marker.
=================
*/
void Mod_LoadStudioNeoModel( model_t* mod, void* buffer )
{
	int					i;
	byte*				pin;
	studiohdr_t*		phdr;
	mstudiotexture_t*	ptexture;
	int					total;
	int					version;
	byte*				pData;
	byte*				pPal;

	pin = (byte*)buffer;
	phdr = (studiohdr_t*)pin;

	version = LittleLong(phdr->version);
	if (version != STUDIO_VERSION)
	{
		memset(phdr, 0, sizeof(*phdr));
		strcpy(phdr->name, "bogus");
		phdr->length = sizeof(*phdr);
		phdr->texturedataindex = sizeof(*phdr);
	}
	phdr->version = 0xC0EDBABE;

	if (phdr->numbones > MAXSTUDIOBONES)
		Sys_Error("Too damn many bones in %s!\n", mod->name);

	mod->type = mod_studio;
	mod->flags = phdr->flags;

	total = phdr->length;
	if (phdr->textureindex != 0)
		total -= phdr->length - phdr->texturedataindex;

	Cache_Alloc(&mod->cache, total, mod->name);
	if (!mod->cache.data)
		return;

	if (phdr->textureindex != 0 && phdr->numtextures > 0)
	{
		ptexture = (mstudiotexture_t*)(pin + phdr->textureindex);

		for (i = 0; i < phdr->numtextures; i++, ptexture++)
		{
			char name[260];
			int texofs;
			int texBytes;

			CL_PollProgressBar();
			strcpy(name, mod->name);
			strcat(name, ptexture->name);

			if (ptexture->index & 3)
		Sys_Error("Model %s has misaligned textures - run it through a newer studiomdl.exe!", mod->name);

			texofs = ptexture->index;
			pData = pin + texofs;
			if (*(int*)pData == 0x58494247)
			{
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, *(short*)(pData + 0x1C), *(short*)(pData + 0x1E),
					pData, FALSE, TEX_TYPE_GBIX, NULL);
			}
			else
			{
				texBytes = ptexture->width * ptexture->height;
				pPal = pin + texofs + texBytes;
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, ptexture->width, ptexture->height,
					pData, FALSE, TEX_TYPE_NONE, pPal);
			}

			DC_TouchTexture(ptexture->index);
		}
	}

	memcpy(mod->cache.data, pin, total);
}

/*
=================
Mod_TouchStudioNeoTextures
=================
*/
void Mod_TouchStudioNeoTextures( void* buffer )
{
	studiohdr_t* phdr = (studiohdr_t*)buffer;
	mstudiotexture_t* texture;
	int i;

	if (phdr->version == 0xC0EDBABE)
	{
		if (phdr->textureindex != 0)
		{
			texture = (mstudiotexture_t*)((byte*)phdr + phdr->textureindex);
			for (i = 0; i < phdr->numtextures; i++, texture++)
				DC_TouchTexture(texture->index);
		}
	}
}

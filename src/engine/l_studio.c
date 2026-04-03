// l_studio.c - studio model loading

#include "quakedef.h"
#include "studio.h"

int giTextureSize;

extern char loadname[32];

//=============================================================================

/*
=================
Mod_LoadStudioModel
=================
*/
void Mod_LoadStudioModel( model_t* mod, void* buffer )
{
	int					i;
	byte* pin, * pout;
	studiohdr_t* phdr;
	mstudiotexture_t* ptexture;
	int					total;
	int					version;
	byte* pData;
	byte* pPal;
	int					start;
	qboolean			merged_external = FALSE;

	pin = (byte*)buffer;

	phdr = (studiohdr_t*)pin;
	start = Hunk_LowMark();

	version = LittleLong(phdr->version);
	if (version != STUDIO_VERSION)
	{
		memset(phdr, 0, sizeof(*phdr));
		strcpy(phdr->name, "bogus");
		phdr->length = sizeof(*phdr);
		phdr->texturedataindex = sizeof(*phdr);
	}

	if (phdr->numbones > MAXSTUDIOBONES)
		Sys_Error("Too damn many bones in %s!\n", mod->name);

	mod->type = mod_studio;
	mod->flags = phdr->flags;

	pout = (byte*)Hunk_AllocName(phdr->length, loadname);
	memcpy(pout, pin, phdr->length);
	phdr = (studiohdr_t*)pout;

	/* T.mdl support. that's should be an external fun, not inlined? TODO */
	if (phdr->numtextures == 0)
	{
		char texname[MAX_QPATH];
		int namelen = (int)strlen(mod->name);

		texname[0] = '\0';
		if (namelen >= 4 && !Q_strcasecmp(&mod->name[namelen - 4], ".mdl"))
		{
			memcpy(texname, mod->name, namelen + 1);
			texname[namelen - 4] = '\0';
			strcat(texname, "T.mdl");
		}

		if (texname[0])
		{
			byte* buffer2 = COM_LoadFile(texname, 5, NULL);
			if (buffer2)
			{
				studiohdr_t* thdr = (studiohdr_t*)buffer2;
				if (thdr->version == STUDIO_VERSION && thdr->numtextures > 0 && thdr->textureindex > 0)
				{
					size_t size1 = (size_t)thdr->numtextures * sizeof(mstudiotexture_t);
					size_t size2 = (size_t)thdr->numskinfamilies * (size_t)thdr->numskinref * sizeof(short);
					byte* newpout = (byte*)Hunk_AllocName(phdr->length + (int)size1 + (int)size2, loadname);
					byte* in = (byte*)thdr + thdr->textureindex;
					byte* out;
					mstudiotexture_t* ptex_out;
					mstudiotexture_t* ptex_in;

					memcpy(newpout, pout, phdr->length);
					pout = newpout;
					phdr = (studiohdr_t*)pout;

					out = (byte*)phdr + phdr->length;
					memcpy(out, in, size1 + size2);
					phdr->textureindex = phdr->length;
					phdr->skinindex = phdr->textureindex + (int)size1;
					phdr->numtextures = thdr->numtextures;
					phdr->numskinfamilies = thdr->numskinfamilies;
					phdr->numskinref = thdr->numskinref;
					phdr->length += (int)size1 + (int)size2;

					ptex_out = (mstudiotexture_t*)((byte*)phdr + phdr->textureindex);
					ptex_in = (mstudiotexture_t*)((byte*)thdr + thdr->textureindex);
					for (i = 0; i < thdr->numtextures; i++)
					{
						char name[260];
						byte* src = (byte*)thdr + ptex_in[i].index;

						strcpy(name, mod->name);
						strcat(name, ptex_in[i].name);

						if (!Q_strncmp((char*)src, "GBIX", 4))
							ptex_out[i].index = GL_LoadTexture(name, GLT_STUDIO, ptex_in[i].width, ptex_in[i].height, src, FALSE, TEX_TYPE_GBIX, NULL);
						else
						{
							byte* pal = (byte*)thdr + ptex_in[i].index + (ptex_in[i].width * ptex_in[i].height);
							ptex_out[i].index = GL_LoadTexture(name, GLT_STUDIO, ptex_in[i].width, ptex_in[i].height, src, FALSE, TEX_TYPE_NONE, pal);
						}
					}

					merged_external = TRUE;
				}
				COM_FreeFile(buffer2);
			}
		}
	}

	if (phdr->textureindex != 0 && phdr->numtextures > 0 && !merged_external)
	{
		ptexture = (mstudiotexture_t*)(pout + phdr->textureindex);

		for (i = 0; i < phdr->numtextures; i++, ptexture++)
		{
			char name[260];
			int texofs;
			int texBytes;
			char sig[5];

			strcpy(name, mod->name);
			strcat(name, ptexture->name);

			if (ptexture->index & 3)
				Sys_Error("Model %s has misaligned textures - run it through a newer studiomdl.exe!\n", mod->name);

			texofs = ptexture->index;
			pData = pout + texofs;
			sig[0] = ((char*)pData)[0];
			sig[1] = ((char*)pData)[1];
			sig[2] = ((char*)pData)[2];
			sig[3] = ((char*)pData)[3];
			sig[4] = '\0';
			if (*(int*)pData == 0x58494247) /* "GBIX" - PVR textures */
			{
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, ptexture->width, ptexture->height, pData, FALSE, TEX_TYPE_GBIX, NULL);
			}
			else
			{
				texBytes = ptexture->width * ptexture->height;

				pPal = pout + texofs + texBytes;
				ptexture->index = GL_LoadTexture(name, GLT_STUDIO, ptexture->width, ptexture->height,
					pData, FALSE, TEX_TYPE_NONE, pPal);
			}
		}
	}

	total = ((studiohdr_t*)pout)->length;

	Cache_Alloc(&mod->cache, total, mod->name);
	if (!mod->cache.data)
	{
		Hunk_FreeToLowMark(start);
		return;
	}
	memcpy(mod->cache.data, pout, total);
	Hunk_FreeToLowMark(start);
}
#include "quakedef.h"
#include "server.h"
#include "decal.h"
#include "hashpak.h"

/*
==================
SV_ParseUpload

Handles incoming file upload data from client
==================
*/
void SV_ParseUpload( void )
{
}

/*
==================
SV_PrintResource

==================
*/
void SV_PrintResource( int index, resource_t* pResource )
{
	static char type[12];
	static char fatal[8];

	if (pResource->ucFlags & RES_FATALIFMISSING)
		sprintf(fatal, "Y");
	else
		sprintf(fatal, "N");

	switch (pResource->type)
	{
	case t_sound:
		sprintf(type, "sound");
		break;
	case t_skin:
		sprintf(type, "skin");
		break;
	case t_model:
		sprintf(type, "model");
		break;
	case t_decal:
		sprintf(type, "decal");
		break;
	case t_generic:
		sprintf(type, "generic");
		break;
	default:
		sprintf(type, "unknown");
		break;
	}

	Con_Printf("%3i %s:%15s %i %s\n", index, type, pResource->szFileName, pResource->nIndex, fatal);
}

/*
==================
SV_PrintResourceLists_f

==================
*/
void SV_PrintResourceLists_f( void )
{
	int		i;
	resource_t* pResource;

	i = 1;

	Con_Printf("- Needed -------------------------------------------\n");
	Con_Printf("#   Name                  Size Type Index Fatal\n");

	for (pResource = host_client->resourcesneeded.pNext; pResource != &host_client->resourcesneeded; pResource = pResource->pNext)
	{
		SV_PrintResource(i, pResource);
		i++;
	}

	Con_Printf("- On hand ------------------------------------------\n");
	Con_Printf("#   Name                  Size Type Index Fatal\n");

	for (pResource = host_client->resourcesonhand.pNext; pResource != &host_client->resourcesonhand; pResource = pResource->pNext)
	{
		SV_PrintResource(i, pResource);
		i++;
	}

	Con_Printf("--------------------------------------------\n\n");
}

/*
==================
SV_ClearResourceList

==================
*/
void SV_ClearResourceList( resource_t* pList )
{
	resource_t* p, * n = NULL;

	for (p = pList->pNext; p && p != pList; p = n)
	{
		n = p->pNext;

		SV_RemoveFromResourceList(p);
		free(p);
	}

	pList->pPrev = pList;
	pList->pNext = pList;
}

/*
==================
SV_ClearResourceLists

==================
*/
void SV_ClearResourceLists( client_t* cl )
{
	if (!cl)
		Sys_Error("SV_ClearResourceLists with NULL client!");

	SV_ClearResourceList(&cl->resourcesneeded);
	SV_ClearResourceList(&cl->resourcesonhand);
}

/*
==================
SV_PrintCusomizations_f

==================
*/
void SV_PrintCusomizations_f( void )
{
	int		i;
	int		nIndex;
	client_t* cl;
	customization_t* pCust;

	if (cmd_source == src_command && !sv.active)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (!NET_IsLocalAddress(net_from))
		Host_BeginRedirect(RD_CLIENT, &net_from);

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
	{
		if (!cl->active && !cl->spawned)
			continue;
		if (!cl->customdata.pNext)
			continue;

		nIndex = 1;
		Con_DPrintf("SV Customizations:\nPlayer %i:%s\n", nIndex, cl->name);

		for (pCust = cl->customdata.pNext; pCust; pCust = pCust->pNext)
		{
			if (pCust->bInUse)
			{
				SV_PrintResource(nIndex, &pCust->resource);
				nIndex++;
			}
		}

		Con_DPrintf("-----------------\n\n");
	}

	if (!NET_IsLocalAddress(net_from))
		Host_EndRedirect();
}

/*
==================
SV_AddToResourceList

==================
*/
void SV_AddToResourceList( resource_t* pResource, resource_t* pList )
{
	if (!pResource->pPrev && !pResource->pNext)
	{
		pResource->pPrev = pList->pPrev;
		pList->pPrev->pNext = pResource;
		pList->pPrev = pResource;
		pResource->pNext = pList;
	}
	else
	{
		Con_Printf("Resource already linked\n");
	}
}

/*
==================
SV_RemoveFromResourceList

==================
*/
void SV_RemoveFromResourceList( resource_t* pResource )
{
	pResource->pPrev->pNext = pResource->pNext;
	pResource->pNext->pPrev = pResource->pPrev;
	pResource->pPrev = NULL;
	pResource->pNext = NULL;
}

/*
===============
SV_CountResources

Sums a nominal per-resource size and tallies it by category.
===============
*/
int SV_CountResources( resource_t* pList, int* nWorldSize, int* nModelsSize, int* nDecalsSize, int* nSoundsSize, int* nSkinsSize, int* nGenericSize )
{
	resource_t* p;
	int nSize;

	nSize = 0;

	*nModelsSize = 0;
	*nWorldSize = 0;
	*nDecalsSize = 0;
	*nSoundsSize = 0;
	*nSkinsSize = 0;
	*nGenericSize = 0;

	for (p = pList->pNext; p != pList; p = p->pNext)
	{
		nSize += 1000;

		switch (p->type)
		{
		case t_sound:
			*nSoundsSize += 1000;
			break;
		case t_skin:
			*nSkinsSize += 1000;
			break;
		case t_model:
			if (p->nIndex == 1) // worldmodel always take 1 slot
			{
				*nWorldSize = 1000;
			}
			else
			{
				*nModelsSize += 1000;
			}
			break;
		case t_decal:
			*nDecalsSize += 1000;
			break;
		case t_generic:
			*nGenericSize += 1000;
			break;
		}
	}

	return nSize;
}

/*
==================
SV_MoveToOnHandList

==================
*/
void SV_MoveToOnHandList( resource_t* pResource )
{
	if (!pResource)
		return;

	SV_RemoveFromResourceList(pResource);
	SV_AddToResourceList(pResource, &host_client->resourcesonhand);
}

/*
================
SV_RequestResourceList_f

Request a resource from client
================
*/
void SV_RequestResourceList_f( void )
{
	int		servercount;
	int		index;

	if (Cmd_Argc() != 3)
	{
		Con_Printf("Invalid resource request\n");
		return;
	}

	servercount = atoi(Cmd_Argv(1));
	if (servercount != svs.spawncount)
	{
		Con_Printf("Resource request with mismatched servercount\n");
		return;
	}

	index = atoi(Cmd_Argv(2));
	if (index < 0)
	{
		Con_Printf("Resource request with bogus starting index %i\n", index);
		return;
	}

	MSG_WriteByte(&host_client->netchan.message, svc_resourcerequest);
	MSG_WriteLong(&host_client->netchan.message, svs.spawncount);
	MSG_WriteLong(&host_client->netchan.message, index);
}

/*
==================
SV_ParseResourceList
==================
*/
void SV_ParseResourceList( void )
{
	int		i, total;
	int		totalsize;
	int		worldSize, modelsSize, decalsSize, soundsSize, skinsSize, genericSize;
	resource_t* resource;

	totalsize = MSG_ReadShort();
	i = MSG_ReadShort();
	total = MSG_ReadShort();

	for (; i < total; i++)
	{
		resource = (resource_t*)MnemoAllocDbg(sizeof(resource_t), __FILE__, __LINE__);
		memset(resource, 0, sizeof(resource_t));

		resource->type = MSG_ReadByte();
		strcpy(resource->szFileName, MSG_ReadString());
		resource->nIndex = MSG_ReadShort();
		MSG_ReadLong();
		resource->ucFlags = MSG_ReadByte();
		resource->pNext = resource->pPrev = NULL;
		resource->ucFlags &= ~RES_WASMISSING;

		if (i == 0)
		{
			SV_ClearResourceList(&host_client->resourcesneeded);
			SV_ClearResourceList(&host_client->resourcesonhand);
		}

		// Add new entry in the linked list
		SV_AddToResourceList(resource, &host_client->resourcesneeded);
	}

	if (total < totalsize)
	{
		host_client->uploading = FALSE;
	}
	else
	{
		host_client->nTotalSize = SV_CountResources(&host_client->resourcesneeded, &worldSize, &modelsSize, &decalsSize, &soundsSize, &skinsSize, &genericSize);
		host_client->nTotalToTransfer = 0;

		host_client->uploading = TRUE;

		host_client->uploaddoneregistering = FALSE;
		host_client->uploadinprogress = FALSE;

		host_client->fLastStatusUpdate = realtime;
		host_client->fLastUploadTime = realtime;

		host_client->nRemainingToTransfer = host_client->nTotalToTransfer;

		memset(host_client->rgUploads, 0, sizeof(host_client->rgUploads));
		host_client->nCurUpload = 0;
	}
}

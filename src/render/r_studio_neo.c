// r_studio_neo.c: rendering support for Neo studio models

#include <shintr.h>

#include "quakedef.h"
#include "CL_TENT.H"
#include "r_studio.h"
#include "dc_accum.h"

extern studiohdr_t* pstudiohdr;
extern mstudiobodyparts_t* pbodypart;
extern mstudiomodel_t* psubmodel;
extern float lighttransform[MAXSTUDIOBONES][3][4];
extern float r_avertexnormals[162][3];
extern int	cached_numbones;
extern char	cached_bonename[MAXSTUDIOBONES * 32];
extern float bonetransform[MAXSTUDIOBONES][4][4];
extern float rotationmatrix[3][4];
extern float cached_bonetransform[MAXSTUDIOBONES][4][4];
extern float cached_lighttransform[MAXSTUDIOBONES][3][4];
extern vec3_t vpn;
extern vec3_t r_origin;
extern int	r_studio_clip_required;
extern vec3_t g_ChromeOrigin;
extern int	g_ForcedFaceFlags;
extern model_t* r_studio_model;
extern player_info_t* r_playerinfo;
extern float r_gaitmovement;
extern float chrome[MAXSTUDIOVERTS][2];
extern int	chromeage[MAXSTUDIOBONES];
extern vec3_t r_chromeup[MAXSTUDIOBONES];
extern vec3_t r_chromeright[MAXSTUDIOBONES];
extern vec3_t vright;
extern int	r_smodels_total;
extern int	r_topcolor;
extern int	r_bottomcolor;
extern byte	g_studioTranslatedPalette[STUDIO_PALETTE_RGB_BYTES];
extern int	r_amodels_drawn;
extern auxvert_t* pauxverts;
extern vec3_t* pvlightvalues;

void StudioComputeBBox( vec3_t mins, vec3_t maxs, const vec3_t angles );

static byte* g_pStudioBitStream;
static int	g_StudioBitsRemaining;
static byte	g_StudioAnimBuffer[8192];

static const byte g_StudioHighBitMask[9] =
{
	0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF
};

static const byte g_StudioLowBitMask[9] =
{
	0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
};

typedef struct
{
	int			bone;
	vec3_t		org;
	vec3_t		vectors[3];
} mstudioattachment_neo_t;

typedef struct studio_skin_cache_neo_s
{
	int			playerIndex;
	int			topColor;
	int			bottomColor;
	model_t*	model;
	char		textureName[STUDIO_SKIN_CACHE_NAME_LENGTH];
	byte		skinState[STUDIO_SKIN_STATE_BYTES];
	int			textureIndex;
	int			textureState;
	int			width;
	int			height;
	cache_user_t pixels;
	int			glTexture;
} studio_skin_cache_neo_t;

typedef struct studio_player_model_neo_ref_s
{
	char		name[MAX_OSPATH];
	char		modelName[MAX_OSPATH];
	model_t*	model;
} studio_player_model_neo_ref_t;

extern studio_player_model_neo_ref_t g_studioPlayerModels[MAX_CLIENTS];
extern int	r_playerindex;

studio_skin_cache_neo_t* R_StudioGetPlayerSkinCache( int playerIndex );
void R_StudioRemapPaletteRange( byte* palette, int color, int first, int last );

sfx_t* R_StudioFindEventSound( const char* name )
{
	int			i;
	sfx_t*		sound;

	for (i = 0; i < MAX_SOUNDS; i++)
	{
		sound = cl.sound_precache[i];
		if (sound && !strcmp(name, (char*)sound + 8))
			return sound;
	}

	return NULL;
}

mstudioanim_t* R_GetAnim_Neo( model_t* model, mstudioseqdesc_t* sequence )
{
	mstudioseqgroup_t* sequenceGroup;
	cache_user_t* sequenceCache;
	unsigned int data;

	sequenceGroup = (mstudioseqgroup_t*)((byte*)pstudiohdr + pstudiohdr->seqgroupindex) + sequence->seqgroup;
	if (!sequence->seqgroup)
		return (mstudioanim_t*)((byte*)pstudiohdr + sequenceGroup->data + sequence->animindex);

	sequenceCache = (cache_user_t*)model->submodels;
	if (!sequenceCache)
	{
		sequenceCache = (cache_user_t*)calloc(16, sizeof(cache_user_t));
		model->submodels = (dmodel_t*)sequenceCache;
	}

	if (!Cache_Check(&sequenceCache[sequence->seqgroup]))
	{
		Cache_Lock(&model->cache);
		COM_LoadCacheFile(sequenceGroup->name, &sequenceCache[sequence->seqgroup]);
		Cache_Unlock(&model->cache);
	}

	data = (unsigned int)sequenceCache[sequence->seqgroup].data;
	if (data & 1)
		data = 0;
	return (mstudioanim_t*)(data + sequence->animindex);
}

void R_StudioCacheAnim_Neo( model_t* model, int sequenceGroupIndex )
{
	studiohdr_t* header;
	mstudioseqgroup_t* sequenceGroup;
	cache_user_t* sequenceCache;

	header = (studiohdr_t*)Mod_Extradata(model);
	sequenceGroup = (mstudioseqgroup_t*)((byte*)header + header->seqgroupindex) + sequenceGroupIndex;
	if (sequenceGroupIndex)
	{
		sequenceCache = (cache_user_t*)model->submodels;
		if (!sequenceCache)
		{
			sequenceCache = (cache_user_t*)calloc(16, sizeof(cache_user_t));
			model->submodels = (dmodel_t*)sequenceCache;
		}

		sequenceCache += sequenceGroupIndex;
		if (!Cache_Check(sequenceCache))
		{
			Cache_Lock(&model->cache);
			COM_LoadCacheFile(sequenceGroup->name, sequenceCache);
			Cache_Unlock(&model->cache);
		}
	}
}

int R_StudioBodyVariations_Neo( model_t* model )
{
	studiohdr_t* pstudiohdr;
	mstudiobodyparts_t* pbodypart;
	int			i, count;

	if (model->type != mod_studio)
		return 0;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(model);
	if (!pstudiohdr)
		return 0;

	count = 1;
	pbodypart = (mstudiobodyparts_t*)((byte*)pstudiohdr + pstudiohdr->bodypartindex);
	for (i = 0; i < pstudiohdr->numbodyparts; i++, pbodypart++)
		count *= pbodypart->nummodels;

	return count;
}

void R_StudioSetupModel_Neo( int bodypart )
{
	int			index;

	if (bodypart > pstudiohdr->numbodyparts)
		bodypart = 0;

	pbodypart = (mstudiobodyparts_t*)((byte*)pstudiohdr + pstudiohdr->bodypartindex) + bodypart;
	index = currententity->body / pbodypart->base;
	index %= pbodypart->nummodels;
	psubmodel = (mstudiomodel_t*)((byte*)pstudiohdr + pbodypart->modelindex) + index;
}

float R_StudioEstimateFrame_Neo( mstudioseqdesc_t* sequence )
{
	float		frame;
	float		frameDelta;

	frameDelta = (cl.time - currententity->animtime) *
		ShortToFloat(currententity->framerate) * sequence->fps;
	if (sequence->numframes <= 1)
		frame = 0.0f;
	else
		frame = currententity->frame * (sequence->numframes - 1) * (1.0f / 256.0f);

	frame += frameDelta;
	if (sequence->flags & STUDIO_LOOPING)
	{
		if (sequence->numframes > 1)
			frame -= (int)(frame / (sequence->numframes - 1)) * (sequence->numframes - 1);
		if (frame < 0.0f)
			frame += sequence->numframes - 1;
	}
	else
	{
		if (frame >= sequence->numframes - 1.001f)
			frame = sequence->numframes - 1.001f;
		if (frame < 0.0f)
			frame = 0.0f;
	}

	return frame;
}

void R_StudioPlayerBlend_Neo( mstudioseqdesc_t* sequence, int* blend, float* pitch )
{
	*blend = (int)(*pitch * 3.0f);
	if (*blend < sequence->blendstart[0])
	{
		*pitch -= sequence->blendstart[0] / 3.0f;
		*blend = 0;
	}
	else if (*blend > sequence->blendend[0])
	{
		*pitch -= sequence->blendend[0] / 3.0f;
		*blend = 255;
	}
	else if (sequence->blendend[0] - sequence->blendstart[0] < 0.1f)
	{
		*blend = 127;
	}
	else
	{
		*blend = (int)(255.0f * ((float)*blend - sequence->blendstart[0]) /
			(sequence->blendend[0] - sequence->blendstart[0]));
		*pitch = 0.0f;
	}
}

void R_StudioSaveBones_Neo( void )
{
	int			i;
	mstudiobone_t* bones;

	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	cached_numbones = pstudiohdr->numbones;
	if (cached_numbones > MAXSTUDIOBONES)
		Sys_Error("Too damn many bones!\n");

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		strcpy(&cached_bonename[i * 32], bones[i].name);
		memcpy(cached_bonetransform[i], bonetransform[i], 0x40);
		memcpy(cached_lighttransform[i], lighttransform[i], 0x40);
	}
}

void R_StudioCalcAttachments_Neo( void )
{
	int			i;
	mstudioattachment_neo_t* attachment;

	if (pstudiohdr->numattachments > 4)
		Sys_Error("Too many attachments on %s\n", currententity->model->name);

	attachment = (mstudioattachment_neo_t*)((byte*)pstudiohdr + pstudiohdr->attachmentindex);
	for (i = 0; i < pstudiohdr->numattachments; i++)
	{
		VectorTransform(attachment[i].org, lighttransform[attachment[i].bone],
			currententity->attachment[i]);
	}
}

qboolean R_StudioCheckBBox_Neo( void )
{
	mplane_t	plane;
	vec3_t		mins;
	vec3_t		maxs;
	int			side;
	mstudioseqdesc_t* sequence;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;
	VectorCopy(sequence->bbmin, mins);
	VectorCopy(sequence->bbmax, maxs);
	StudioComputeBBox(mins, maxs, currententity->angles);
	VectorAdd(mins, currententity->origin, mins);
	VectorAdd(maxs, currententity->origin, maxs);

	if (currententity->model &&
		!memcmp(currententity->model->name, "models/barnacle.mdl", 20))
		mins[2] -= 1024.0f;

	if (R_CullBox(mins, maxs))
		return FALSE;

	VectorCopy(vpn, plane.normal);
	plane.dist = DotProduct(plane.normal, r_origin);
	plane.type = PLANE_ANYZ;
	plane.signbits = SignbitsForPlane(&plane);

	side = BoxOnPlaneSide(mins, maxs, &plane);
	if (side == 3)
	{
		DCV_SetClipRequired();
		r_studio_clip_required = TRUE;
	}
	else
	{
		DCV_SetNoClip();
		r_studio_clip_required = FALSE;
	}

	return side != 2;
}


void R_StudioRenderModel_Neo( void )
{
	VectorCopy(r_origin, g_ChromeOrigin);
	g_ForcedFaceFlags = 0;
	R_StudioRenderFinal_Neo();
}

void R_StudioTransformVerts_Neo( vec3_t* output, const char* normalIndices, int count )
{
	while (count--)
	{
		(*output)[0] = r_avertexnormals[*normalIndices][0];
		(*output)[1] = r_avertexnormals[*normalIndices][1];
		(*output)[2] = r_avertexnormals[*normalIndices][2];
		output++;
		normalIndices++;
	}
}

void R_StudioTransformVertsMatrix_Neo( vec3_t* output, const char* bones,
	const char*	normalIndices, int count )
{
	int			lastBone;

	lastBone = -1;
	while (count--)
	{
		int			bone;

		bone = *bones;
		if (bone != lastBone)
		{
			float*		matrix;

			lastBone = bone;
			matrix = &lighttransform[bone][0][0];
			_LoadMatrix(matrix);
		}

		_XDXform3dV(r_avertexnormals[*normalIndices], *output);

		output++;
		bones++;
		normalIndices++;
	}
}

int StudioReadBits_Neo( int bitCount );
int StudioReadSignedBits_Neo( int bitCount );

void R_StudioReadAnimQuaternion_Neo( int startFrame, int endFrame, int span,
	const byte* data )
{
	int			frameCount;
	int			frameSpan;
	int			bufferOffset;
	int			bits;

	frameCount = 0;
	frameSpan = span;
	bufferOffset = 0;
	while (frameCount < frameSpan)
	{
		byte*		run;
		byte		total;

		run = g_StudioAnimBuffer + bufferOffset;
		run[0] = *data++;
		total = *data++;
		g_StudioAnimBuffer[bufferOffset + 1] = total;
		bufferOffset += run[0] * 2 + 2;
		frameCount += total;
	}

	g_pStudioBitStream = (byte*)data;
	g_StudioBitsRemaining = 8;
	frameCount = 0;
	bufferOffset = 0;
	while (frameCount < endFrame)
	{
		mstudioanimvalue_t* run;

		run = (mstudioanimvalue_t*)(g_StudioAnimBuffer + bufferOffset);
		bits = StudioReadBits_Neo(4);
		frameCount += g_StudioAnimBuffer[bufferOffset + 1];
		if (frameCount >= startFrame)
		{
			int			i;

			run[1].value = (short)(StudioReadBits_Neo(12) << 4);
			for (i = 1; i < g_StudioAnimBuffer[bufferOffset]; i++)
				run[i + 1].value = run[i].value +
					(StudioReadSignedBits_Neo(bits) << 4);
		}
		else
		{
			int			bitCount;
			int			byteCount;
			int			remainder;

			bitCount = (g_StudioAnimBuffer[bufferOffset] - 1) * bits + 12;
			byteCount = bitCount / 8;
			g_pStudioBitStream += byteCount;
			remainder = bitCount - byteCount * 8;
			if (remainder)
				StudioReadBits_Neo(remainder);
		}
		bufferOffset += g_StudioAnimBuffer[bufferOffset] * 2 + 2;
	}
}

void R_StudioReadAnimPosition_Neo( int startFrame, int endFrame, int span,
	const byte* data )
{
	int			frameCount;
	int			frameSpan;
	int			bufferOffset;
	int			bits;

	frameCount = 0;
	frameSpan = span;
	bufferOffset = 0;
	while (frameCount < frameSpan)
	{
		byte*		run;
		byte		total;

		run = g_StudioAnimBuffer + bufferOffset;
		run[0] = *data++;
		total = *data++;
		g_StudioAnimBuffer[bufferOffset + 1] = total;
		bufferOffset += run[0] * 2 + 2;
		frameCount += total;
	}

	g_pStudioBitStream = (byte*)data;
	g_StudioBitsRemaining = 8;
	frameCount = 0;
	bufferOffset = 0;
	while (frameCount < endFrame)
	{
		mstudioanimvalue_t* run;

		run = (mstudioanimvalue_t*)(g_StudioAnimBuffer + bufferOffset);
		bits = StudioReadBits_Neo(4);
		frameCount += g_StudioAnimBuffer[bufferOffset + 1];
		if (frameCount >= startFrame)
		{
			int			i;
			short		firstValue;

			firstValue = (short)StudioReadBits_Neo(16);
			run[1].value = firstValue;
			for (i = 1; i < g_StudioAnimBuffer[bufferOffset]; i++)
				run[i + 1].value = run[i].value +
					StudioReadSignedBits_Neo(bits);
		}
		else
		{
			int			bitCount;
			int			byteCount;
			int			remainder;

			bitCount = (g_StudioAnimBuffer[bufferOffset] - 1) * bits + 16;
			byteCount = bitCount / 8;
			g_pStudioBitStream += byteCount;
			remainder = bitCount - byteCount * 8;
			if (remainder)
				StudioReadBits_Neo(remainder);
		}
		bufferOffset += g_StudioAnimBuffer[bufferOffset] * 2 + 2;
	}
}

void R_StudioCalcBoneQuaternion_Neo( int frame, float s, mstudiobone_t* bone,
	mstudioanim_t* anim, float* adjustment, float* quaternion, int numFrames )
{
	int			j;
	int			k;
	int			endFrame;
	vec4_t		q1;
	vec4_t		q2;
	vec3_t		angle1;
	vec3_t		angle2;
	mstudioanimvalue_t* animValue;

	endFrame = frame + 2;
	if (endFrame >= numFrames)
		endFrame = numFrames;

	for (j = 0; j < 3; j++)
	{
		if (!anim->offset[j + 3])
		{
			angle2[j] = angle1[j] = bone->value[j + 3];
		}
		else
		{
			R_StudioReadAnimQuaternion_Neo(frame, endFrame, numFrames,
				(byte*)anim + anim->offset[j + 3]);
			animValue = (mstudioanimvalue_t*)g_StudioAnimBuffer;
			k = frame;
			if (animValue->num.total < animValue->num.valid)
				k = 0;
			while (animValue->num.total <= k)
			{
				k -= animValue->num.total;
				animValue += animValue->num.valid + 1;
				if (animValue->num.total < animValue->num.valid)
					k = 0;
			}

			if (animValue->num.valid > k)
			{
				angle1[j] = animValue[k + 1].value;
				if (animValue->num.valid > k + 1)
					angle2[j] = animValue[k + 2].value;
				else if (animValue->num.total > k + 1)
					angle2[j] = angle1[j];
				else
					angle2[j] = animValue[animValue->num.valid + 2].value;
			}
			else
			{
				angle1[j] = animValue[animValue->num.valid].value;
				if (animValue->num.total > k + 1)
					angle2[j] = angle1[j];
				else
					angle2[j] = animValue[animValue->num.valid + 2].value;
			}

			angle1[j] = bone->value[j + 3] + angle1[j] * bone->scale[j + 3];
			angle2[j] = bone->value[j + 3] + angle2[j] * bone->scale[j + 3];
		}

		if (bone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += adjustment[bone->bonecontroller[j + 3]];
			angle2[j] += adjustment[bone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, quaternion);
	}
	else
	{
		AngleQuaternion(angle1, quaternion);
	}
}

void R_StudioCalcBonePosition_Neo( int frame, float s, mstudiobone_t* bone,
	mstudioanim_t* anim, float* adjustment, float* position, int numFrames )
{
	int			j;
	int			k;
	int			endFrame;
	mstudioanimvalue_t* animValue;

	endFrame = frame + 2;
	if (endFrame >= numFrames)
		endFrame = numFrames;

	for (j = 0; j < 3; j++)
	{
		position[j] = bone->value[j];
		if (anim->offset[j])
		{
			R_StudioReadAnimPosition_Neo(frame, endFrame, numFrames,
				(byte*)anim + anim->offset[j]);
			animValue = (mstudioanimvalue_t*)g_StudioAnimBuffer;
			k = frame;
			if (animValue->num.total < animValue->num.valid)
				k = 0;
			while (animValue->num.total <= k)
			{
				k -= animValue->num.total;
				animValue += animValue->num.valid + 1;
				if (animValue->num.total < animValue->num.valid)
					k = 0;
			}

			if (animValue->num.valid > k)
			{
				if (animValue->num.valid > k + 1)
				{
					position[j] += (animValue[k + 1].value * (1.0f - s) +
						s * animValue[k + 2].value) * bone->scale[j];
				}
				else
				{
					position[j] += animValue[k + 1].value * bone->scale[j];
				}
			}
			else if (animValue->num.total > k + 1)
			{
				position[j] += animValue[animValue->num.valid].value * bone->scale[j];
			}
			else
			{
				position[j] += (animValue[animValue->num.valid].value * (1.0f - s) +
					s * animValue[animValue->num.valid + 2].value) * bone->scale[j];
			}
		}

		if (bone->bonecontroller[j] != -1)
			position[j] += adjustment[bone->bonecontroller[j]];
	}
}

void R_StudioCalcRotations_Neo( vec3_t* position, vec4_t* quaternion,
	mstudioseqdesc_t* sequence, mstudioanim_t* animation, float f )
{
	int			i;
	int			frame;
	mstudiobone_t* bone;
	float		s;
	float		adjustment[MAXSTUDIOCONTROLLERS];
	float		dadt;

	if (f > sequence->numframes - 1)
		f = 0.0f;

	frame = (int)f;
	dadt = CL_StudioEstimateInterpolant();
	s = f - frame;
	bone = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);

	R_StudioCalcBoneAdj(dadt, adjustment, currententity->controller,
		currententity->prevcontroller, currententity->mouth.mouthopen);

	for (i = 0; i < pstudiohdr->numbones; i++, bone++, animation++)
	{
		R_StudioCalcBoneQuaternion_Neo(frame, s, bone, animation,
			adjustment, quaternion[i], sequence->numframes);
		R_StudioCalcBonePosition_Neo(frame, s, bone, animation,
			adjustment, position[i], sequence->numframes);
	}

	if (sequence->motiontype & STUDIO_X)
		position[sequence->motionbone][0] = 0.0f;
	if (sequence->motiontype & STUDIO_Y)
		position[sequence->motionbone][1] = 0.0f;
	if (sequence->motiontype & STUDIO_Z)
		position[sequence->motionbone][2] = 0.0f;

	s = 0.0f * ((1.0f - (f - (int)f)) / sequence->numframes) *
		ShortToFloat(currententity->framerate);
	if (sequence->motiontype & STUDIO_LX)
		position[sequence->motionbone][0] += s * sequence->linearmovement[0];
	if (sequence->motiontype & STUDIO_LY)
		position[sequence->motionbone][1] += s * sequence->linearmovement[1];
	if (sequence->motiontype & STUDIO_LZ)
		position[sequence->motionbone][2] += s * sequence->linearmovement[2];
}

void R_StudioSetupBones_Neo( void )
{
	int			i;
	float		frame;
	mstudiobone_t* bones;
	mstudioseqdesc_t* sequence;
	mstudioanim_t* animation;
	static float position[MAXSTUDIOBONES][3];
	static vec4_t quaternion[MAXSTUDIOBONES];
	float		boneMatrix[3][4];
	static float position2[MAXSTUDIOBONES][3];
	static vec4_t quaternion2[MAXSTUDIOBONES];
	static float position3[MAXSTUDIOBONES][3];
	static vec4_t quaternion3[MAXSTUDIOBONES];
	static float position4[MAXSTUDIOBONES][3];
	static vec4_t quaternion4[MAXSTUDIOBONES];

	if (currententity->sequence >= pstudiohdr->numseq)
		currententity->sequence = 0;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;
	frame = R_StudioEstimateFrame_Neo(sequence);
	animation = R_GetAnim_Neo(r_studio_model, sequence);
	R_StudioCalcRotations_Neo(position, quaternion, sequence, animation, frame);

	if (sequence->numblends > 1)
	{
		float		s;
		float		dadt;

		animation += pstudiohdr->numbones;
		R_StudioCalcRotations_Neo(position2, quaternion2, sequence, animation, frame);
		dadt = CL_StudioEstimateInterpolant();
		s = (currententity->blending[0] * dadt +
			currententity->prevblending[0] * (1.0f - dadt)) * (1.0f / 255.0f);
		R_StudioSlerpBones(quaternion, position, quaternion2, position2, s);

		if (sequence->numblends == 4)
		{
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations_Neo(position3, quaternion3, sequence, animation, frame);
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations_Neo(position4, quaternion4, sequence, animation, frame);

			s = (currententity->blending[0] * dadt +
				currententity->prevblending[0] * (1.0f - dadt)) * (1.0f / 255.0f);
			R_StudioSlerpBones(quaternion3, position3, quaternion4, position4, s);
			s = (currententity->blending[1] * dadt +
				currententity->prevblending[1] * (1.0f - dadt)) * (1.0f / 255.0f);
			R_StudioSlerpBones(quaternion, position, quaternion3, position3, s);
		}
	}

	if (currententity->sequencetime &&
		currententity->sequencetime + 0.2f > cl.time &&
		currententity->prevsequence < pstudiohdr->numseq)
	{
		static float previousPosition[MAXSTUDIOBONES][3];
		static vec4_t previousQuaternion[MAXSTUDIOBONES];
		float		s;

		sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
			currententity->prevsequence;
		animation = R_GetAnim_Neo(r_studio_model, sequence);
		R_StudioCalcRotations_Neo(previousPosition, previousQuaternion, sequence,
			animation, currententity->prevframe);

		if (sequence->numblends > 1)
		{
			animation += pstudiohdr->numbones;
			R_StudioCalcRotations_Neo(position2, quaternion2, sequence, animation,
				frame);
			s = currententity->prevseqblending[0] * (1.0f / 255.0f);
			R_StudioSlerpBones(previousQuaternion, previousPosition,
				quaternion2, position2, s);

			if (sequence->numblends == 4)
			{
				animation += pstudiohdr->numbones;
				R_StudioCalcRotations_Neo(position3, quaternion3, sequence,
					animation, frame);
				animation += pstudiohdr->numbones;
				R_StudioCalcRotations_Neo(position4, quaternion4, sequence,
					animation, frame);
				s = currententity->prevseqblending[0] * (1.0f / 255.0f);
				R_StudioSlerpBones(quaternion3, position3, quaternion4, position4, s);
				s = currententity->prevseqblending[1] * (1.0f / 255.0f);
				R_StudioSlerpBones(previousQuaternion, previousPosition,
					quaternion3, position3, s);
			}
		}

		s = 1.0f - (cl.time - currententity->sequencetime) * 5.0f;
		R_StudioSlerpBones(quaternion, position, previousQuaternion,
			previousPosition, s);
	}
	else
	{
		currententity->prevframe = frame;
	}

	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	if (r_playerinfo && r_playerinfo->gaitsequence != 0)
	{
		sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
			r_playerinfo->gaitsequence;
		animation = R_GetAnim_Neo(r_studio_model, sequence);
		R_StudioCalcRotations_Neo(position2, quaternion2, sequence, animation,
			r_playerinfo->gaitframe);

		for (i = 0; i < pstudiohdr->numbones; i++)
		{
			if (!strcmp(bones[i].name, "Bip01 Spine"))
				break;
			memcpy(position[i], position2[i], sizeof(position[i]));
			memcpy(quaternion[i], quaternion2[i], sizeof(quaternion[i]));
		}
	}

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		QuaternionMatrix(quaternion[i], boneMatrix);
		boneMatrix[0][3] = position[i][0];
		boneMatrix[1][3] = position[i][1];
		boneMatrix[2][3] = position[i][2];

		if (bones[i].parent == -1)
		{
			R_ConcatTransforms(rotationmatrix, boneMatrix, bonetransform[i]);
			R_ConcatTransforms(rotationmatrix, boneMatrix, lighttransform[i]);
			CL_FxTransform(currententity, bonetransform[i][0]);
		}
		else
		{
			R_ConcatTransforms(bonetransform[bones[i].parent], boneMatrix,
				bonetransform[i]);
			R_ConcatTransforms(lighttransform[bones[i].parent], boneMatrix,
				lighttransform[i]);
		}
	}
}

void R_StudioMergeBones_Neo( model_t* model )
{
	int			i;
	int			j;
	float		frame;
	mstudiobone_t* bones;
	mstudioseqdesc_t* sequence;
	mstudioanim_t* animation;
	static float position[MAXSTUDIOBONES][3];
	float		boneMatrix[3][4];
	static vec4_t quaternion[MAXSTUDIOBONES];

	if (currententity->sequence >= pstudiohdr->numseq)
		currententity->sequence = 0;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;
	frame = R_StudioEstimateFrame_Neo(sequence);
	animation = R_GetAnim_Neo(model, sequence);
	R_StudioCalcRotations_Neo(position, quaternion, sequence, animation, frame);
	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);

	for (i = 0; i < pstudiohdr->numbones; i++)
	{
		for (j = 0; j < cached_numbones; j++)
		{
			if (_stricmp(bones[i].name, &cached_bonename[j * 32]) == 0)
			{
				memcpy(bonetransform[i], cached_bonetransform[j], 0x40);
				memcpy(lighttransform[i], cached_lighttransform[j], 0x40);
				break;
			}
		}

		if (j >= cached_numbones)
		{
			QuaternionMatrix(quaternion[i], boneMatrix);
			boneMatrix[0][3] = position[i][0];
			boneMatrix[1][3] = position[i][1];
			boneMatrix[2][3] = position[i][2];

			if (bones[i].parent == -1)
			{
				R_ConcatTransforms(rotationmatrix, boneMatrix, bonetransform[i]);
				R_ConcatTransforms(rotationmatrix, boneMatrix, lighttransform[i]);
				CL_FxTransform(currententity, bonetransform[i][0]);
			}
			else
			{
				R_ConcatTransforms(bonetransform[bones[i].parent], boneMatrix,
					bonetransform[i]);
				R_ConcatTransforms(lighttransform[bones[i].parent], boneMatrix,
					lighttransform[i]);
			}
		}
	}
}

void R_StudioSetupChrome_Neo( int count, int normalIndex,
	const char*	normalBones, const byte* normalIndices )
{
	normalIndices += normalIndex;
	while (count-- != 0)
	{
		int			bone;
		float		n;

		bone = normalBones[normalIndex];
		if (chromeage[bone] != r_smodels_total)
		{
			vec3_t		chromeUp;
			vec3_t		chromeRight;
			vec3_t		direction;

			VectorScale(g_ChromeOrigin, -1.0f, direction);
			direction[0] += lighttransform[bone][0][3];
			direction[1] += lighttransform[bone][1][3];
			direction[2] += lighttransform[bone][2][3];
			VectorNormalize(direction);
			CrossProduct(direction, vright, chromeUp);
			VectorNormalize(chromeUp);
			CrossProduct(chromeUp, direction, chromeRight);
			VectorNormalize(chromeRight);
			VectorIRotate(chromeUp, lighttransform[bone], r_chromeup[bone]);
			VectorIRotate(chromeRight, lighttransform[bone], r_chromeright[bone]);
			chromeage[bone] = r_smodels_total;
		}

		n = DotProduct(r_avertexnormals[*normalIndices],
			r_chromeright[bone]);
		chrome[normalIndex][0] = (n + 1.0f) * 32.0f;
		n = DotProduct(r_avertexnormals[*normalIndices++],
			r_chromeup[bone]);
		chrome[normalIndex][1] = (n + 1.0f) * 32.0f;
		normalIndex++;
	}
}

void R_StudioLoadPlayerSkin_Neo( model_t* model, int textureIndex,
	studio_skin_cache_neo_t* cache )
{
	studiohdr_t* header;
	mstudiotexture_t* texture;
	byte*		fileData;
	byte*		pixels;
	unsigned int pixelData;
	int			dataSize;

	if (Cache_Check(&cache->pixels))
	{
		if (cache->model == model)
			return;
		Cache_Free(&cache->pixels, 0);
	}

	cache->textureIndex = textureIndex;
	fileData = COM_LoadFileForMe(model->name, NULL);
	header = (studiohdr_t*)fileData;
	texture = (mstudiotexture_t*)(fileData + header->textureindex) +
		cache->textureIndex;

	cache->width = texture->width;
	cache->height = texture->height;

	dataSize = cache->width * cache->height + STUDIO_PALETTE_RGB_BYTES;
	Cache_Alloc(&cache->pixels, dataSize, cache->textureName);

	pixelData = (unsigned int)cache->pixels.data;
	if (pixelData & 1U)
		pixelData = 0;

	pixels = (byte*)pixelData;
	memcpy(pixels, fileData + texture->index, dataSize);

	COM_FreeFile(fileData);
}

void R_StudioSetupPlayerSkin_Neo( studiohdr_t* textureHeader, int textureIndex )
{
	mstudiotexture_t* texture;
	studio_skin_cache_neo_t* cache;
	byte*		pixels;
	int			playerIndex;
	char		textureName[STUDIO_SKIN_CACHE_NAME_LENGTH];

	if (g_ForcedFaceFlags & STUDIO_NF_CHROME)
		return;

	texture = (mstudiotexture_t*)((byte*)textureHeader +
		textureHeader->textureindex) + textureIndex;
	playerIndex = currententity->index;
	if (playerIndex > 0 && !Q_stricmp(texture->name, "DM_Base.bmp"))
	{
		cache = R_StudioGetPlayerSkinCache(playerIndex);
		if (cache->model != r_studio_model || cache->topColor != r_topcolor ||
			cache->bottomColor != r_bottomcolor)
		{
			R_StudioLoadPlayerSkin_Neo(r_studio_model, textureIndex, cache);

			sprintf(textureName, "%s%d", texture->name, playerIndex);

			pixels = (byte*)cache->pixels.data;
			if ((unsigned int)pixels & 1U)
				pixels = NULL;
			memcpy(g_studioTranslatedPalette,
				pixels + texture->width * texture->height, STUDIO_PALETTE_RGB_BYTES);

			cache->model = r_studio_model;
			cache->topColor = r_topcolor;
			cache->bottomColor = r_bottomcolor;
			R_StudioRemapPaletteRange(g_studioTranslatedPalette, r_topcolor,
				STUDIO_TOP_COLOR_START, STUDIO_TOP_COLOR_END);
			R_StudioRemapPaletteRange(g_studioTranslatedPalette, cache->bottomColor,
				STUDIO_BOTTOM_COLOR_START, STUDIO_BOTTOM_COLOR_END);

			GL_UnloadTexture(textureName);
			pixels = (byte*)cache->pixels.data;
			if ((unsigned int)pixels & 1U)
				pixels = NULL;

			cache->glTexture = GL_LoadTexture(textureName, GLT_STUDIO,
				cache->width, cache->height, pixels, FALSE, TEX_TYPE_NONE,
				g_studioTranslatedPalette);
		}

		if (cache->glTexture != 0)
		{
			GL_Bind(cache->glTexture, 0);
			return;
		}
	}

	GL_Bind(texture->index, 0);
}

void R_StudioProcessGait_Neo( player_state_t* player )
{
	mstudioseqdesc_t* sequence;
	float		dt;
	float		yaw;
	int			blend;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;

	R_StudioPlayerBlend_Neo(sequence, &blend, &currententity->angles[PITCH]);
	currententity->prevangles[PITCH] = currententity->angles[PITCH];
	currententity->blending[0] = (byte)blend;
	currententity->prevblending[0] = currententity->blending[0];
	currententity->prevseqblending[0] = currententity->blending[0];

	dt = cl.time - cl.oldtime;
	if (dt < 0.0f)
		dt = 0.0f;
	else if (dt > 1.0f)
		dt = 1.0f;

	R_StudioEstimateGait(player);
	yaw = currententity->angles[YAW] - r_playerinfo->gaityaw;
	yaw -= (int)(yaw / 360.0f) * 360;
	if (yaw < -180.0f)
		yaw += 360.0f;
	if (yaw > 180.0f)
		yaw -= 360.0f;

	if (yaw > 120.0f)
	{
		r_playerinfo->gaityaw -= 180.0f;
		r_gaitmovement = -r_gaitmovement;
		yaw -= 180.0f;
	}
	else if (yaw < -120.0f)
	{
		r_playerinfo->gaityaw += 180.0f;
		r_gaitmovement = -r_gaitmovement;
		yaw += 180.0f;
	}

	currententity->controller[0] =
		(byte)(((yaw * 0.25f) + 30.0f) * (255.0f / 60.0f));
	currententity->controller[1] =
		(byte)(((yaw * 0.25f) + 30.0f) * (255.0f / 60.0f));
	currententity->controller[2] =
		(byte)(((yaw * 0.25f) + 30.0f) * (255.0f / 60.0f));
	currententity->controller[3] =
		(byte)(((yaw * 0.25f) + 30.0f) * (255.0f / 60.0f));
	currententity->prevcontroller[0] = currententity->controller[0];
	currententity->prevcontroller[1] = currententity->controller[1];
	currententity->prevcontroller[2] = currententity->controller[2];
	currententity->prevcontroller[3] = currententity->controller[3];

	currententity->angles[YAW] = r_playerinfo->gaityaw;
	if (currententity->angles[YAW] < 0.0f)
		currententity->angles[YAW] += 360.0f;
	currententity->prevangles[YAW] = currententity->angles[YAW];

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		player->gaitsequence;
	if (sequence->linearmovement[0] > 0.0f)
	{
		r_playerinfo->gaitframe +=
			(r_gaitmovement / sequence->linearmovement[0]) * sequence->numframes;
	}
	else
	{
		r_playerinfo->gaitframe += sequence->fps * dt;
	}
	r_playerinfo->gaitframe -=
		(int)(r_playerinfo->gaitframe / sequence->numframes) * sequence->numframes;
	if (r_playerinfo->gaitframe < 0.0f)
		r_playerinfo->gaitframe += sequence->numframes;
}

int R_StudioDrawPlayer_Neo( int flags, player_state_t* player )
{
	alight_t	lighting;
	vec3_t		direction;
	vec3_t		savedAngles;

	r_playerindex = player->number;
	if (r_playerindex < 0 || r_playerindex >= cl.maxclients)
		return 0;

	if ((developer.value == 0.0f && SV_Active()) ||
		cl.players[r_playerindex].model[0] == 0)
	{
		g_studioPlayerModels[r_playerindex].name[0] = 0;

		if (g_studioPlayerModels[r_playerindex].model != currententity->model)
		{
			g_studioPlayerModels[r_playerindex].model = currententity->model;
			R_StudioResetPlayerModel();
		}
	}
	else if (strcmp(g_studioPlayerModels[r_playerindex].name,
		cl.players[r_playerindex].model) != 0)
	{
		strcpy(g_studioPlayerModels[r_playerindex].name,
			cl.players[r_playerindex].model);

		strcpy(g_studioPlayerModels[r_playerindex].modelName, "models/player/");
		strcat(g_studioPlayerModels[r_playerindex].modelName,
			cl.players[r_playerindex].model);
		strcat(g_studioPlayerModels[r_playerindex].modelName, "/");
		strcat(g_studioPlayerModels[r_playerindex].modelName,
			cl.players[r_playerindex].model);
		strcat(g_studioPlayerModels[r_playerindex].modelName, ".mdl");

		g_studioPlayerModels[r_playerindex].model =
			Mod_ForName(g_studioPlayerModels[r_playerindex].modelName, FALSE);
		if (!g_studioPlayerModels[r_playerindex].model)
			g_studioPlayerModels[r_playerindex].model = currententity->model;

		R_StudioResetPlayerModel();
	}

	r_studio_model = g_studioPlayerModels[r_playerindex].model;
	if (!r_studio_model)
		return 0;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(r_studio_model);

	if (player->gaitsequence != 0)
	{
		r_playerinfo = &cl.players[r_playerindex];
		VectorCopy(currententity->angles, savedAngles);
		R_StudioProcessGait_Neo(player);
		r_playerinfo->gaitsequence = player->gaitsequence;
		r_playerinfo = NULL;
		R_StudioSetUpTransform(0);
		VectorCopy(savedAngles, currententity->angles);
	}
	else
	{
		currententity->controller[0] = 127;
		currententity->controller[1] = 127;
		currententity->controller[2] = 127;
		currententity->controller[3] = 127;
		currententity->prevcontroller[0] = currententity->controller[0];
		currententity->prevcontroller[1] = currententity->controller[1];
		currententity->prevcontroller[2] = currententity->controller[2];
		currententity->prevcontroller[3] = currententity->controller[3];
		r_playerinfo = &cl.players[r_playerindex];
		r_playerinfo->gaitsequence = 0;
		R_StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		if (!R_StudioCheckBBox_Neo())
		{
			DCV_SetClipRequired();
			return 0;
		}
		r_amodels_drawn++;
		r_smodels_total++;
		if (pstudiohdr->numbodyparts == 0)
		{
			DCV_SetClipRequired();
			return 1;
		}
	}

	r_playerinfo = &cl.players[r_playerindex];
	R_StudioSetupBones_Neo();
	R_StudioSaveBones_Neo();
	player->renderframe = r_framecount;

	r_playerinfo = NULL;

	if (flags & STUDIO_EVENTS)
	{
		R_StudioCalcAttachments_Neo();
		R_StudioClientEvents_Neo();
		if (currententity->index > 0)
		{
			memcpy(cl_entities[currententity->index].attachment,
				currententity->attachment, sizeof(vec3_t) * 4);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		if (cl_himodels.value &&
			g_studioPlayerModels[r_playerindex].model != currententity->model)
			currententity->body = 255;
		if (!((developer.value == 0.0f) && SV_Active()) &&
			g_studioPlayerModels[r_playerindex].name[0] != 0 &&
			g_studioPlayerModels[r_playerindex].model == currententity->model)
			currententity->body = 1;

		lighting.plightvec = direction;
		R_StudioDynamicLight(currententity, &lighting);
		R_StudioEntityLight(&lighting);
		R_StudioSetupLighting(&lighting);

		r_playerinfo = &cl.players[r_playerindex];
		r_topcolor = r_playerinfo->color;
		if (r_topcolor < 0)
			r_topcolor = 0;
		if (r_topcolor > 360)
			r_topcolor = 360;
		r_bottomcolor = r_playerinfo->bottomcolor;
		if (r_bottomcolor < 0)
			r_bottomcolor = 0;
		if (r_bottomcolor > 360)
			r_bottomcolor = 360;

		R_StudioRenderModel_Neo();
		r_playerinfo = NULL;

		if (player->weaponmodel)
		{
			cl_entity_t	savedEntity;
			model_t*	weaponModel;

			savedEntity = *currententity;
			weaponModel = cl.model_precache[player->weaponmodel];

			pstudiohdr = (studiohdr_t*)Mod_Extradata(weaponModel);

			R_StudioMergeBones_Neo(weaponModel);
			R_StudioSetupLighting(&lighting);

			R_StudioRenderModel_Neo();
			R_StudioCalcAttachments_Neo();
			*currententity = savedEntity;
		}
	}

	DCV_SetClipRequired();
	return 1;
}

#pragma inline_depth(0)
void R_StudioClientEvents_Neo( void )
{
	int			i;
	mstudioevent_t* event;
	mstudioseqdesc_t* sequence;
	float		frameStart;
	float		frameEnd;
	static float currentTime;
	static float lastTime;

	sequence = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) +
		currententity->sequence;

	if (cl.time == cl.oldtime)
		return;

	if (currentTime != cl.time)
	{
		lastTime = currentTime;
		currentTime = cl.time;
	}

	if (currententity->effects & EF_MUZZLEFLASH)
	{
		dlight_t*	light;

		light = CL_AllocElight(0);
		VectorCopy(currententity->attachment[0], light->origin);
		light->radius = 16.0f;
		light->decay = light->radius / 0.05f;
		light->die = cl.time + 0.05f;
		light->color.r = 255;
		light->color.g = 192;
		light->color.b = 64;
		currententity->effects &= ~EF_MUZZLEFLASH;
	}

	if (!sequence->numevents)
		return;

	event = (mstudioevent_t*)((byte*)pstudiohdr + sequence->eventindex);
	frameEnd = R_StudioEstimateFrame_Neo(sequence);
	frameStart = frameEnd - (currentTime - lastTime) *
		ShortToFloat(currententity->framerate) * sequence->fps;

	if (currententity->sequencetime == currententity->animtime &&
		!(sequence->flags & STUDIO_LOOPING))
		frameStart = -0.01f;

	for (i = 0; i < sequence->numevents; i++, event++)
	{
		if (event->event < 5000)
			continue;
		if (event->frame <= frameStart || event->frame > frameEnd)
			continue;

		switch (event->event)
		{
		case 5001:
			R_MuzzleFlash(currententity->attachment[0], atoi(event->options));
			break;
		case 5011:
			R_MuzzleFlash(currententity->attachment[1], atoi(event->options));
			break;
		case 5021:
			R_MuzzleFlash(currententity->attachment[2], atoi(event->options));
			break;
		case 5031:
			R_MuzzleFlash(currententity->attachment[3], atoi(event->options));
			break;
		case 5002:
			R_SparkEffect(currententity->attachment[0], atoi(event->options), -100, 100);
			break;
		case 5004:
		{
			sfx_t*		sound;

			sound = R_StudioFindEventSound(event->options);
			if (sound)
			{
				S_StartDynamicSound(cl.viewentity, CHAN_AUTO, sound,
					currententity->attachment[0], 1.0f, 1.0f, 0, PITCH_NORM);
			}
			break;
		}
		}
	}
}
#pragma inline_depth(255)

int R_StudioDrawModel_Neo( int flags, int checkBBox )
{
	alight_t	lighting;
	vec3_t		direction;

	r_studio_model = currententity->model;
	pstudiohdr = (studiohdr_t*)Mod_Extradata(r_studio_model);

	R_StudioSetUpTransform(0);

	if (flags & STUDIO_RENDER)
	{
		if (!checkBBox)
			r_studio_clip_required = TRUE;
		else if (!R_StudioCheckBBox_Neo())
		{
			DCV_SetClipRequired();
			return 0;
		}

		r_amodels_drawn++;
		r_smodels_total++;
		if (pstudiohdr->numbodyparts == 0)
		{
			DCV_SetClipRequired();
			return 1;
		}
	}

	if (currententity->movetype == MOVETYPE_FOLLOW)
		R_StudioMergeBones_Neo(r_studio_model);
	else
		R_StudioSetupBones_Neo();

	R_StudioSaveBones_Neo();

	if (flags & STUDIO_EVENTS)
	{
		R_StudioCalcAttachments_Neo();
		R_StudioClientEvents_Neo();

		if (currententity->index > 0)
			memcpy(cl_entities[currententity->index].attachment,
				currententity->attachment, sizeof(vec3_t) * 4);
	}

	if (flags & STUDIO_RENDER)
	{
		lighting.plightvec = direction;
		R_StudioDynamicLight(currententity, &lighting);

		R_StudioEntityLight(&lighting);

		R_StudioSetupLighting(&lighting);

		r_topcolor = currententity->colormap & 0xFF;
		r_bottomcolor = (currententity->colormap >> 8) & 0xFF;

		R_StudioRenderModel_Neo();
	}

	DCV_SetClipRequired();
	return 1;
}

#pragma inline_depth(0)
#pragma auto_inline(off)
void R_StudioRenderFinal_Neo( void )
{
	int			i;
	int			rendermode;
	qboolean	translucent;

	GL_DisableMultitexture();
	DCV_PushMatrix(D3DTRANSFORMSTATE_WORLD);

	if (gl_smoothmodels.value)
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

	rendermode = currententity->rendermode;
	if (g_ForcedFaceFlags)
		rendermode = kRenderTransAdd;

	if (r_drawentities.value != 2.0f && r_drawentities.value != 3.0f)
	{
		translucent = FALSE;
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);

		if (rendermode != kRenderNormal)
		{
			DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);
			translucent = TRUE;

			if (rendermode == kRenderTransColor)
			{
				DCV_TexState_Blend();
			}
			else if (rendermode == kRenderTransAdd)
			{
				DCV_SetColor((int)(r_blend * 255.0f),
					(int)(r_blend * 255.0f), (int)(r_blend * 255.0f), 255);
				DCV_TexState_Additive();
			}
			else
			{
				DCV_TexState_Blend();
				DCV_SetColor(255, 255, 255, (int)(r_blend * 255.0f));
			}
		}
		else
		{
			DCV_TexState_Opaque();
		}

		for (i = 0; i < pstudiohdr->numbodyparts; i++)
		{
			R_StudioSetupModel_Neo(i);
			if (r_studio_clip_required || translucent)
				R_StudioDrawPoints_Neo();
			else
				R_StudioDrawPointsSimple_Neo();
		}
	}

	if (r_drawentities.value == 4.0f)
	{
		DCV_SetTexStateFromRenderMode(kRenderTransAdd);
		DCV_SetTexStateFromRenderMode(kRenderNormal);
	}

	DCV_FlushApplyRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
	DCV_PopMatrix(D3DTRANSFORMSTATE_WORLD);
}
#pragma auto_inline(on)

void R_StudioDrawPoints_Neo( void )
{
	studiohdr_t* textureHeader;
	mstudiotexture_t* textures;
	mstudiomesh_t* meshes;
	short*		skinref;
	byte*		vertBones;
	byte*		normBones;
	vec3_t*		studioVerts;
	byte*		studioNorms;
	int			normalIndex;
	int			i;

	vertBones = (byte*)pstudiohdr + psubmodel->vertinfoindex;
	normBones = (byte*)pstudiohdr + psubmodel->norminfoindex;
	meshes = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex);
	studioVerts = (vec3_t*)((byte*)pstudiohdr + psubmodel->vertindex);
	studioNorms = (byte*)pstudiohdr + psubmodel->normindex;

	textureHeader = R_StudioGetTextureHeader(r_studio_model);
	textures = (mstudiotexture_t*)((byte*)textureHeader + textureHeader->textureindex);
	skinref = (short*)((byte*)textureHeader + textureHeader->skinindex);
	if (currententity->skin != 0 && currententity->skin < textureHeader->numskinfamilies)
		skinref += currententity->skin * textureHeader->numskinref;

	DCV_Flush();

	normalIndex = 0;
	for (i = 0; i < psubmodel->nummesh; i++)
	{
		int			textureIndex;
		int			flags;

		textureIndex = skinref[meshes[i].skinref];
		flags = textures[textureIndex].flags | g_ForcedFaceFlags;
		if (r_fullbright.value >= 2.0f)
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
		if (flags & STUDIO_NF_CHROME)
			R_StudioSetupChrome_Neo(meshes[i].numnorms, normalIndex,
				(const char*)normBones, studioNorms);
		normalIndex += meshes[i].numnorms;
	}

	R_StudioTransformVertsMatrix_Neo(pvlightvalues, (const char*)normBones,
		(const char*)studioNorms, normalIndex);
	StudioTransformVerts(pauxverts, vertBones, studioVerts, psubmodel->numverts);

	for (i = 0; i < psubmodel->nummesh; i++)
	{
		mstudiomesh_t* mesh;
		short*		commands;
		int			textureIndex;
		int			flags;
		int			count;

		mesh = &meshes[i];
		commands = (short*)((byte*)pstudiohdr + mesh->triindex);
		textureIndex = skinref[mesh->skinref];
		g_flStudioTexScaleS = 1.0f / (float)textures[textureIndex].width;
		g_flStudioTexScaleT = 1.0f / (float)textures[textureIndex].height;
		flags = textures[textureIndex].flags | g_ForcedFaceFlags;

		if (r_fullbright.value < 2.0f)
		{
			R_StudioSetupPlayerSkin_Neo(textureHeader, textureIndex);
		}
		else
		{
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
			R_TriangleSpriteTexture(cl_sprite_white, 0);
		}

		c_alias_polys += mesh->numtris;
		count = commands[1];
		commands += 2;
		if (flags & STUDIO_NF_CHROME)
			DCV_AddStudioMeshChrome(count, commands, (const byte*)pauxverts,
				(const byte*)pvlightvalues);
		else
			DCV_AddStudioMesh(count, commands, (const byte*)pauxverts,
				(const byte*)pvlightvalues);
		DCV_BuildStudioIndexList(commands + count * 4);
		DCV_SubmitBatchCopy();
	}
}

void R_StudioDrawPointsSimple_Neo( void )
{
	studiohdr_t* textureHeader;
	mstudiotexture_t* textures;
	mstudiomesh_t* meshes;
	short*		skinref;
	byte*		vertBones;
	byte*		normBones;
	vec3_t*		studioVerts;
	byte*		studioNorms;
	int			normalIndex;
	int			i;

	vertBones = (byte*)pstudiohdr + psubmodel->vertinfoindex;
	normBones = (byte*)pstudiohdr + psubmodel->norminfoindex;
	meshes = (mstudiomesh_t*)((byte*)pstudiohdr + psubmodel->meshindex);
	studioVerts = (vec3_t*)((byte*)pstudiohdr + psubmodel->vertindex);
	studioNorms = (byte*)pstudiohdr + psubmodel->normindex;

	textureHeader = R_StudioGetTextureHeader(r_studio_model);
	textures = (mstudiotexture_t*)((byte*)textureHeader + textureHeader->textureindex);
	skinref = (short*)((byte*)textureHeader + textureHeader->skinindex);
	if (currententity->skin != 0 && currententity->skin < textureHeader->numskinfamilies)
		skinref += currententity->skin * textureHeader->numskinref;

	DCV_Flush();

	normalIndex = 0;
	for (i = 0; i < psubmodel->nummesh; i++)
	{
		int			textureIndex;
		int			flags;

		textureIndex = skinref[meshes[i].skinref];
		flags = textures[textureIndex].flags | g_ForcedFaceFlags;
		if (r_fullbright.value >= 2.0f)
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
		if (flags & STUDIO_NF_CHROME)
			R_StudioSetupChrome_Neo(meshes[i].numnorms, normalIndex,
				(const char*)normBones, studioNorms);
		normalIndex += meshes[i].numnorms;
	}

	R_StudioTransformVerts_Neo(pvlightvalues, (const char*)studioNorms, normalIndex);
	DCV_SetupStudioLighting(bonetransform, pstudiohdr->numbones);

	for (i = 0; i < psubmodel->nummesh; i++)
	{
		mstudiomesh_t* mesh;
		short*		commands;
		int			textureIndex;
		int			flags;
		int			count;

		mesh = &meshes[i];
		commands = (short*)((byte*)pstudiohdr + mesh->triindex);
		textureIndex = skinref[mesh->skinref];
		g_flStudioTexScaleS = 1.0f / (float)textures[textureIndex].width;
		g_flStudioTexScaleT = 1.0f / (float)textures[textureIndex].height;
		flags = textures[textureIndex].flags | g_ForcedFaceFlags;

		if (r_fullbright.value < 2.0f)
		{
			R_StudioSetupPlayerSkin_Neo(textureHeader, textureIndex);
		}
		else
		{
			flags &= ~(STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME);
			R_TriangleSpriteTexture(cl_sprite_white, 0);
		}

		c_alias_polys += mesh->numtris;
		count = commands[1];
		commands += 2;
		if (flags & STUDIO_NF_CHROME)
			DCV_AddStudioMeshChromeTagged(count, commands, (const byte*)studioVerts,
				(const byte*)pvlightvalues, vertBones);
		else
			DCV_AddStudioMeshTagged(count, commands, (const byte*)studioVerts,
				(const byte*)pvlightvalues, vertBones);
		DCV_AssembleStudioIndexListRestart(commands + count * 4);
		DCV_SubmitBatchGuarded();
	}
}

#pragma inline_depth(255)

int StudioReadBits_Neo( int bitCount )
{
	const byte*	highBitMask;
	int			outputShift;
	int			result;
	int			bits;
	unsigned short	value;

	highBitMask = g_StudioHighBitMask;
	outputShift = 0;
	result = 0;
	while (bitCount >= g_StudioBitsRemaining)
	{
		byte*		input;

		input = g_pStudioBitStream;
		bits = g_StudioBitsRemaining;
		value = *input++ & highBitMask[bits];
		g_pStudioBitStream = input;
		value >>= 8 - bits;
		result |= (short)(value << outputShift);
		outputShift += bits;
		bitCount -= bits;
		g_StudioBitsRemaining = 8;
	}

	if (bitCount)
	{
		byte*		input;

		input = g_pStudioBitStream;
		bits = g_StudioBitsRemaining;
		value = *input & g_StudioHighBitMask[bits];
		value >>= 8 - bits;
		value &= g_StudioLowBitMask[bitCount];
		result |= (short)(value << outputShift);
		g_StudioBitsRemaining = bits - bitCount;
		if (!g_StudioBitsRemaining)
		{
			g_StudioBitsRemaining = 8;
			g_pStudioBitStream = input + 1;
		}
	}

	return result;
}

int StudioReadSignedBits_Neo( int bitCount )
{
	const byte*	highBitMask;
	int			outputShift;
	int			result;
	int			bits;
	short		value;

	highBitMask = g_StudioHighBitMask;
	outputShift = 0;
	result = 0;
	while (bitCount >= g_StudioBitsRemaining)
	{
		byte*		input;

		input = g_pStudioBitStream;
		bits = g_StudioBitsRemaining;
		value = *input++ & highBitMask[bits];
		g_pStudioBitStream = input;
		value >>= 8 - bits;
		result |= (short)(value << outputShift);
		outputShift += bits;
		bitCount -= bits;
		g_StudioBitsRemaining = 8;
	}

	if (bitCount)
	{
		byte*		input;

		input = g_pStudioBitStream;
		bits = g_StudioBitsRemaining;
		value = *input & g_StudioHighBitMask[bits];
		value >>= 8 - bits;
		value &= g_StudioLowBitMask[bitCount];
		result |= (short)(value << outputShift);
		outputShift += bitCount;
		g_StudioBitsRemaining = bits - bitCount;
		if (!g_StudioBitsRemaining)
		{
			g_StudioBitsRemaining = 8;
			g_pStudioBitStream = input + 1;
		}
	}

	if (result)
	{
		value = (short)(1 << (--outputShift));
		result |= (short)(-2 * (value & (short)result));
	}

	return result;
}

void SV_StudioSetupBones_Neo( model_t* model, float frame, int sequence,
	const vec_t* angles, const vec_t* origin, const unsigned char* controller,
	const unsigned char* blending, int boneIndex )
{
	int			i;
	int			j;
	float		f;
	float		s;
	float		adjustment[MAXSTUDIOCONTROLLERS];
	mstudiobone_t* bones;
	mstudioseqdesc_t* sequenceDesc;
	mstudioanim_t* animation;
	static float position[MAXSTUDIOBONES][3];
	float		boneMatrix[3][4];
	static vec4_t quaternion[MAXSTUDIOBONES];
	int			chain[MAXSTUDIOBONES];
	int			chainLength;

	chainLength = 0;
	if (sequence < 0 || sequence >= pstudiohdr->numseq)
		sequence = 0;

	bones = (mstudiobone_t*)((byte*)pstudiohdr + pstudiohdr->boneindex);
	sequenceDesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + sequence;
	animation = R_GetAnim_Neo(model, sequenceDesc);

	if (boneIndex < -1 || boneIndex >= pstudiohdr->numbones)
		boneIndex = 0;

	if (boneIndex == -1)
	{
		chainLength = pstudiohdr->numbones;
		for (i = 0; i < chainLength; i++)
			chain[chainLength - i - 1] = i;
	}
	else
	{
		for (i = boneIndex; i != -1; i = bones[i].parent)
			chain[chainLength++] = i;
	}

	if (sequenceDesc->numframes > 1)
		f = (float)(sequenceDesc->numframes - 1) * frame * (1.0f / 256.0f);
	else
		f = 0.0f;

	s = f - (int)f;
	R_StudioCalcBoneAdj(0.0f, adjustment, controller, controller, 0);

	for (i = chainLength - 1; i >= 0; i--)
	{
		j = chain[i];
		R_StudioCalcBoneQuaternion_Neo((int)f, s, &bones[j], &animation[j],
			adjustment, quaternion[j], sequenceDesc->numframes);
		R_StudioCalcBonePosition_Neo((int)f, s, &bones[j], &animation[j],
			adjustment, position[j], sequenceDesc->numframes);
	}

	if (sequenceDesc->numblends > 1)
	{
		static vec3_t position2[MAXSTUDIOBONES];
		static vec4_t quaternion2[MAXSTUDIOBONES];
		float		blend;

		sequenceDesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + sequence;
		animation = R_GetAnim_Neo(model, sequenceDesc) + pstudiohdr->numbones;
		for (i = chainLength - 1; i >= 0; i--)
		{
			j = chain[i];
			R_StudioCalcBoneQuaternion_Neo((int)f, s, &bones[j], &animation[j],
				adjustment, quaternion2[j], sequenceDesc->numframes);
			R_StudioCalcBonePosition_Neo((int)f, s, &bones[j], &animation[j],
				adjustment, position2[j], sequenceDesc->numframes);
		}

		blend = blending[0] * (1.0f / 255.0f);
		R_StudioSlerpBones(quaternion, position, quaternion2, position2, blend);
	}

	AngleMatrix(angles, rotationmatrix);
	rotationmatrix[0][3] = origin[0];
	rotationmatrix[1][3] = origin[1];
	rotationmatrix[2][3] = origin[2];

	for (i = chainLength - 1; i >= 0; i--)
	{
		j = chain[i];
		QuaternionMatrix(quaternion[j], boneMatrix);
		boneMatrix[0][3] = position[j][0];
		boneMatrix[1][3] = position[j][1];
		boneMatrix[2][3] = position[j][2];

		if (bones[j].parent == -1)
			R_ConcatTransforms(rotationmatrix, boneMatrix, bonetransform[j]);
		else
			R_ConcatTransforms(bonetransform[bones[j].parent], boneMatrix,
				bonetransform[j]);
	}
}

void R_StudioGetAttachment_Neo( const edict_t* edict, int attachmentIndex,
	float*		attachmentOrigin, float* attachmentAngles )
{
	mstudioattachment_neo_t* attachment;
	vec3_t		angles;

	pstudiohdr = (studiohdr_t*)Mod_Extradata(sv.models[edict->v.modelindex]);
	VectorCopy(edict->v.angles, angles);
	angles[PITCH] = -edict->v.angles[PITCH];

	attachment = (mstudioattachment_neo_t*)((byte*)pstudiohdr +
		pstudiohdr->attachmentindex) + attachmentIndex;
	SV_StudioSetupBones_Neo(sv.models[edict->v.modelindex], edict->v.frame,
		edict->v.sequence, angles, edict->v.origin, edict->v.controller,
		edict->v.blending, attachment->bone);

	if (attachmentOrigin)
		VectorTransform(attachment->org, bonetransform[attachment->bone],
			attachmentOrigin);
}

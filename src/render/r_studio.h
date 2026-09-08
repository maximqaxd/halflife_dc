// r_studio.h
#ifndef R_STUDIO_H
#define R_STUDIO_H
#ifdef _WIN32
#pragma once
#endif

// Additional studio flags for client-side models
#define STUDIO_DYNAMIC_LIGHT		0x100	// force to use ambient shading
#define STUDIO_TRACE_HITBOX			0x0200	// always use hitbox trace instead of bbox
#define STUDIO_FORCE_SKYLIGHT		0x400	// always grab lightvalues from the sky settings (even if sky is invisible)

#define STUDIO_SKIN_CACHE_COUNT		64
#define STUDIO_SKIN_PLAYER_MAX_INDEX	64
#define STUDIO_SKIN_PLAYER_SLOTS	(STUDIO_SKIN_PLAYER_MAX_INDEX + 1)
#define STUDIO_SKIN_CACHE_NAME_LENGTH	224
#define STUDIO_SKIN_STATE_BYTES		36
#define STUDIO_PALETTE_COLOR_COUNT	256
#define STUDIO_PALETTE_RGB_BYTES		(STUDIO_PALETTE_COLOR_COUNT * 3)
#define STUDIO_TOP_COLOR_START		160
#define STUDIO_TOP_COLOR_END		191
#define STUDIO_BOTTOM_COLOR_START	192
#define STUDIO_BOTTOM_COLOR_END		223

extern int r_dointerp;

void	R_StudioGetBonePosition( const edict_t* pEdict, int iBone, float* rgflOrigin, float* rgflAngles );
void	R_StudioGetAttachment( const edict_t* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles );

hull_t* R_StudioHull( model_t* pModel, float frame, int sequence, const vec_t* angles, const vec_t* origin, const vec_t* size, const byte* pcontroller, const byte* pblending, int* pNumHulls );
hull_t* SV_HullForStudioModel( const edict_t* pEdict, const vec_t* mins, const vec_t* maxs, vec_t* offset, int* pNumHulls );
int		SV_HitgroupForStudioHull( int index );

int		R_StudioBodyVariations( struct model_s* model );
int		R_StudioBodyVariations_Neo( struct model_s* model );
struct mstudioanim_s;
mstudioanim_t* R_GetAnim_Neo( struct model_s* model, mstudioseqdesc_t* sequence );
void	R_StudioCacheAnim( struct model_s* model, int sequenceGroup );
void	R_StudioCacheAnim_Neo( struct model_s* model, int sequenceGroup );
void	R_StudioSetupModel_Neo( int bodypart );
float	R_StudioEstimateFrame_Neo( mstudioseqdesc_t* sequence );
void	R_StudioPlayerBlend_Neo( mstudioseqdesc_t* sequence, int* blend, float* pitch );
void	R_StudioProcessGait_Neo( player_state_t* player );
void	R_StudioSaveBones_Neo( void );
void	R_StudioCalcAttachments_Neo( void );
qboolean R_StudioCheckBBox_Neo( void );
void	R_StudioRenderModel_Neo( void );
void	R_StudioRenderFinal_Neo( void );
void	R_StudioDrawPoints_Neo( void );
void	R_StudioDrawPointsSimple_Neo( void );
void	R_StudioClientEvents_Neo( void );
sfx_t* R_StudioFindEventSound( const char* name );
void	R_StudioReadAnimQuaternion_Neo( int startFrame, int endFrame, int span, const byte* data );
void	R_StudioReadAnimPosition_Neo( int startFrame, int endFrame, int span, const byte* data );
void	R_StudioCalcBoneQuaternion_Neo( int frame, float s, mstudiobone_t* bone,
	mstudioanim_t* anim, float* adjustment, float* quaternion, int numFrames );
void	R_StudioCalcBonePosition_Neo( int frame, float s, mstudiobone_t* bone,
	mstudioanim_t* anim, float* adjustment, float* position, int numFrames );
void	R_StudioCalcRotations_Neo( vec3_t* position, vec4_t* quaternion,
	mstudioseqdesc_t* sequence, mstudioanim_t* animation, float frame );
void	R_StudioSetupBones_Neo( void );
void	R_StudioMergeBones_Neo( model_t* model );
void	R_StudioSetupChrome_Neo( int count, int normalIndex,
	const char* normalBones, const byte* normalIndices );
void	R_StudioSetupPlayerSkin_Neo( studiohdr_t* textureHeader, int textureIndex );
int		R_StudioDrawModel_Neo( int flags, int checkBBox );
int		R_StudioDrawPlayer_Neo( int flags, player_state_t* player );
void	SV_StudioSetupBones_Neo( model_t* model, float frame, int sequence,
	const vec_t* angles, const vec_t* origin, const unsigned char* controller,
	const unsigned char* blending, int bone );
void	R_StudioGetAttachment_Neo( const edict_t* edict, int attachment,
	float* origin, float* angles );
void	R_StudioTransformVerts_Neo( vec3_t* output, const char* normalIndices, int count );
void	R_StudioTransformVertsMatrix_Neo( vec3_t* output, const char* bones, const char* normalIndices, int count );
studiohdr_t* R_StudioGetTextureHeader( model_t* model );
int		R_TriangleSpriteTexture( model_t* sprite, int frame );
void	StudioTransformVerts( auxvert_t* output, const char* bones,
	vec3_t* vertices, int count );

void	AngleQuaternion( vec_t* angles, vec_t* quaternion );
void	QuaternionSlerp( vec_t* p, vec_t* q, float t, vec_t* qt );
void	R_StudioCalcBoneAdj( float dadt, float* adjustment,
	const unsigned char* controller1, const unsigned char* controller2,
	unsigned char mouthOpen );
float	CL_StudioEstimateInterpolant( void );
void	R_StudioSlerpBones( vec4_t* quaternion1, vec3_t* position1,
	vec4_t* quaternion2, vec3_t* position2, float s );
void	QuaternionMatrix( vec_t* quaternion, float (*matrix)[4] );
void	VectorIRotate( vec_t* input, float (*matrix)[4], vec_t* output );
void	R_StudioSetUpTransform( int trivialAccept );
void	R_StudioSetupLighting( alight_t* lighting );
void	R_StudioEstimateGait( player_state_t* player );
void	R_StudioResetPlayerModel( void );

int		R_StudioDrawModel( int flags, int checkBBox );
int		R_StudioDrawPlayer( int flags, player_state_t* pplayer );
void	R_StudioDynamicLight( cl_entity_t* ent, struct alight_s* plight );
void	R_StudioEntityLight( struct alight_s* plight );
void	R_StudioClientEvents( void );
void	R_StudioRenderFinal( void );
void	R_StudioDrawPoints( void );

int SignbitsForPlane( mplane_t* plane );

void R_StudioRenderModel( void );

#endif // R_STUDIO_H

#include "quakedef.h"
#include "r_triangle.h"
#include "dc_accum.h"

float gGlR, gGlG, gGlB, gGlW;

void tri_DC_Color4f( float x, float y, float z, float w )
{
	DCV_SetColorFloat(x * w, y * w, z * w, 1.0f);
	gGlR = x;
	gGlG = y;
	gGlB = z;
	gGlW = w;
}

void tri_DC_Color4ub( byte r, byte g, byte b, byte a )
{
	gGlR = r * (1.0f / 255.0f);
	gGlG = g * (1.0f / 255.0f);
	gGlB = b * (1.0f / 255.0f);
	gGlW = a * (1.0f / 255.0f);
	DCV_SetColor(r, g, b, a);
}

void tri_DC_Brightness( float x )
{
	DCV_SetColorFloat(gGlR * x * gGlW, gGlG * x * gGlW,
		gGlB * x * gGlW, 1.0f);
}

// Set the rendering mode
void tri_DC_RenderMode( int mode )
{
	switch (mode)
	{
	case kRenderNormal:
		DCV_TexState_Opaque();
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		DCV_TexState_Blend();
		break;
	case kRenderTransAdd:
		DCV_TexState_Additive();
		break;
	default:
		break;
	}
}

void tri_DC_CullFace( TRICULLSTYLE style )
{
	if (style == TRI_FRONT)
	{
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_CCW);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_CCW);
	}
	else
	{
		DCV_FlushApplyRenderState(D3DRENDERSTATE_SWCULLMODE, D3DCULL_NONE);
		DCV_FlushApplyRenderState(D3DRENDERSTATE_HWCULLMODE, D3DCULL_NONE);
	}
}

int R_TriangleSpriteTexture( model_t* pSpriteModel, int frame )
{
	mspriteframe_t* pSpriteFrame;
	unsigned int data;

	data = (unsigned int)pSpriteModel->cache.data;
	if (data & 1)
		data = 0;
	pSpriteFrame = R_GetSpriteFrame((msprite_t*)data, frame);
	if (!pSpriteFrame)
		return FALSE;

	GL_Bind(pSpriteFrame->gl_texturenum, 0);
	return TRUE;
}

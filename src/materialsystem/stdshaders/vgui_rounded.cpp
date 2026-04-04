//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple VGUI shader with optional rounded corner alpha masking
//
//=============================================================================

#include "BaseVSShader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( VGUIRounded, "Help for VGUI Rounded", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "unused" )
		SHADER_PARAM( ALPHAMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Alpha mask texture for rounded corners" )
		SHADER_PARAM( ALPHAMASKTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "Frame number for $alphamasktexture" )
		SHADER_PARAM( ALPHAMASKFULLOPA, SHADER_PARAM_TYPE_BOOL, "0", "Use full opacity for unmasked areas" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		if (params[ALPHAMASKTEXTURE]->IsDefined())
		{
			LoadTexture( ALPHAMASKTEXTURE );
		}
	}

	SHADER_FALLBACK
	{
		// No fallback
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			
			bool bHasAlphaMask = IsTextureSet( ALPHAMASKTEXTURE, params );
			if ( bHasAlphaMask )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			}

			int fmt = VERTEX_POSITION | VERTEX_COLOR | VERTEX_TEXCOORD_SIZE( 0, 2 );
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Use simple vertex shader
			pShaderShadow->SetVertexShader( "vgui_rounded_vs20", 0 );
			
			// Set pixel shader (dynamic combos set later)
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				pShaderShadow->SetPixelShader( "vgui_rounded_ps20b", 0 );
			}
			else
			{
				pShaderShadow->SetPixelShader( "vgui_rounded_ps20", 0 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, ALPHAMASKTEXTUREFRAME );
			
			bool bHasAlphaMask = IsTextureSet( ALPHAMASKTEXTURE, params );
			bool bFullOpacity = params[ALPHAMASKFULLOPA]->GetIntValue() != 0;
			
			// Always bind something to the alpha mask sampler
			if ( bHasAlphaMask )
			{
				BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
			}
			else
			{
				// Bind white texture if no alpha mask specified (no masking)
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_WHITE );
			}

			// Pass shader parameters
			float shaderParams[4] = { 
				bHasAlphaMask ? 1.0f : 0.0f,  // x = has alpha mask
				bFullOpacity ? 1.0f : 0.0f,   // y = use full opacity for unmasked areas
				0.0f, 0.0f 
			};
			pShaderAPI->SetPixelShaderConstant( 0, shaderParams, 1 );

			// Matrices are automatically set up by the engine
			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER

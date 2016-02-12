//=============================================================================//
// Purpose:	Distraction Half-Life 2 grayscale shader
//
// Author: Skillet
//=============================================================================//
#include "BaseVSShader.h"
#include "convar.h"
#include "dhl_grayscale_ps20.inc"
#include "dhl_passthrough_vs20.inc"

BEGIN_VS_SHADER( dhl_grayscale, "Grayscale shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( GRAYSCALE_ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SetInitialShadowState();
			// Enable the texture for base texture and lightmap.
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( dhl_passthrough_vs20 );
			SET_STATIC_VERTEX_SHADER( dhl_passthrough_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( dhl_grayscale_ps20 );
			SET_STATIC_PIXEL_SHADER( dhl_grayscale_ps20 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			DECLARE_DYNAMIC_VERTEX_SHADER( dhl_passthrough_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( dhl_passthrough_vs20 );

			float flAlpha = params[GRAYSCALE_ALPHA]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 0, &flAlpha );

			DECLARE_DYNAMIC_PIXEL_SHADER( dhl_grayscale_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( dhl_grayscale_ps20 );
		}
		Draw();
	}
END_SHADER

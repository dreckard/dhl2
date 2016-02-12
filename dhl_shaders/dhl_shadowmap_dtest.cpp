//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Just about as simple as a shader gets. Specify a vertex
//          and pixel shader, bind textures, and that's it.
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "dhl_shadowmap_dtest_ps20.inc"
#include "dhl_shadowmap_dtest_vs20.inc"


/*

NOTE: this shader needs to be called from the client DLL. You could insert this code at the end of CViewRender::RenderView.

if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 80 )
{
		// Get whatever material references your postprocess shader.
		IMaterial *pMaterial = materials->FindMaterial( "dhl_shaders/dhl_grayscale", TEXTURE_GROUP_CLIENT_EFFECTS, true );
		if ( pMaterial )
		{
				// This copies the contents of the framebuffer (drawn during RenderView) into a texture that your shader can use.
				UpdateScreenEffectTexture( 0 );

				materials->MatrixMode( MATERIAL_PROJECTION );
				materials->PushMatrix();
				materials->LoadIdentity();	

				materials->MatrixMode( MATERIAL_VIEW );
				materials->PushMatrix();
				materials->LoadIdentity();	

				materials->DrawScreenSpaceQuad( pMaterial );

				materials->MatrixMode( MATERIAL_PROJECTION );
				materials->PopMatrix();
				materials->MatrixMode( MATERIAL_VIEW );
				materials->PopMatrix();
		}
}

*/


BEGIN_VS_SHADER( dhl_shadowmap_dtest, "Shadow mapping depth test shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( LIGHTNUM, SHADER_PARAM_TYPE_INTEGER, "0", "Light number" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	bool NeedsFullFrameBufferTexture( IMaterialVar **params ) const
	{
		return true;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableDepthWrites( false );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "dhl_shadowmap_dtest_vs20", 0 );
			pShaderShadow->SetPixelShader( "dhl_shadowmap_dtest_ps20" );

			// Optional, do some blending..
			//pShaderShadow->EnableBlending( true );
			//pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			VMatrix matProj, matView;
			matView.Identity(); matProj.Identity(); //Initialize to ident
			pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProj.m[0] );
			MatrixTranspose( matProj, matProj );
			pShaderAPI->GetMatrix( MATERIAL_VIEW, matView.m[0] );
			MatrixTranspose( matView, matView );

			VMatrix matInvView, matInvProj, matConcat;
			matConcat.Identity();
			//Go backwards to world: Light Projection->Light View->World
			//Then when we want to apply the depth test's results we'll go World->Camera View->Camera Projection
			if ( matView.InverseGeneral( matInvView ) && matProj.InverseGeneral( matInvProj ) )
			{
				matConcat = matInvView * matInvProj;
			}
			//We should always set this.  If either inversion failed it will be identity
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &matConcat[0][0], 3 );

			float flLightNum[4];
			for ( int i = 0; i < 4; i++ )
				flLightNum[i] = params[LIGHTNUM]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 0, flLightNum );
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
			pShaderAPI->SetVertexShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER

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
#include "dhl_motionblur_ps20.inc"
#include "dhl_motionblur_vs20.inc"
#include <time.h>

//Params adjusted by client.dll
//float flShiftInterval;
//float flUpdateInterval;
//float flMin;
//float flMax;
//
//float flShiftTime;
//float flUpdateTime;
//float flChangePerUpdate;
//float amt;
//bool bGoingUp;

BEGIN_VS_SHADER( DHL_MotionBlur, "Motion Blur shader" )

	//These are parsed from the .vmt file for textures on which the shader is applied
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FILTERSIZE, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( SHARPENFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )

		SHADER_PARAM( PASS, SHADER_PARAM_TYPE_FLOAT, "1.0", "Which pass are we doing? (Standard or blurred)" )
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

	//Returns current time in seconds to hundred thousandths
	float GetClockTime( void )
	{
		return float(clock()) / CLK_TCK;
	}

	//Receives changes from client DLL
	void UpdateParams( IMaterialVar** params )
	{
		/*flShiftInterval = params[SHIFTINTERVAL]->GetFloatValue();
		flUpdateInterval = params[UPDATEINTERVAL]->GetFloatValue();
		flMin = params[MIN]->GetFloatValue();
		flMax = params[MAX]->GetFloatValue();*/
	}

	//This is called once when the shader is first loaded
	SHADER_INIT
	{
		//UpdateParams(params);

		//flShiftTime = GetClockTime() + flShiftInterval;
		//flUpdateTime = GetClockTime() + flUpdateInterval;
		////We need to increase/decrease by this amount every update for everything to work out
		//flChangePerUpdate = ( flMax - flMin ) / ( flShiftInterval / flUpdateInterval );
		//bGoingUp = true;
		//amt = flMin;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Enable the texture for base texture and lightmap.
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "dhl_motionblur_vs20", 0 );
			pShaderShadow->SetPixelShader( "dhl_motionblur_ps20" );

			// Optional, do some blending..
			//pShaderShadow->EnableBlending( true );
			//pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			//UpdateParams(params);
			//flChangePerUpdate = ( flMax - flMin ) / ( flShiftInterval / flUpdateInterval );

			//float flCurtime = GetClockTime();
			//if ( flShiftTime <= flCurtime )
			//{
			//	//Toggle the var
			//	if ( bGoingUp )
			//		bGoingUp = false;
			//	else
			//		bGoingUp = true;
			//	//Reset the timer
			//	flShiftTime = flCurtime + flShiftInterval;
			//}

			//if ( flUpdateTime <= flCurtime )
			//{
			//	if ( bGoingUp )
			//		amt += flChangePerUpdate;
			//	else
			//		amt -= flChangePerUpdate;
			//	flUpdateTime = flCurtime + flUpdateInterval;
			//}

			////Make sure these don't get out of bounds
			//if ( flShiftTime > flCurtime + flShiftInterval )
			//	flShiftTime = flCurtime + flShiftInterval;
			//if ( flUpdateTime > flCurtime + flUpdateInterval )
			//	flUpdateTime = flCurtime + flUpdateInterval;

			//// Tell it how wide the blur is.
			//int width, height;
			//pShaderAPI->GetBackBufferDimensions( width, height );

			//float v[4] = {0,0,0,0};
			//v[0] = amt;
			////Hook it up so that it can be read by the shader
			//pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v, true );

			//float factor[4];
			//factor[0] = params[SHARPENFACTOR]->GetFloatValue();
			//factor[1] = factor[2] = factor[3] = factor[0];
			//pShaderAPI->SetPixelShaderConstant( 0, factor );

			//if ( params[BASETEXTURE]->IsDefined() )
			//{
			//	static ITexture* SavedFB = params[BASETEXTURE]->GetTextureValue();
			//	static float flUpdateTime = -1.0f;
			//	if ( flUpdateTime < GetClockTime() )
			//	{
			//		SavedFB = params[BASETEXTURE]->GetTextureValue();
			//		flUpdateTime = GetClockTime() + 1.0f;
			//	}
			//	BindTexture( SHADER_TEXTURE_STAGE1, SavedFB, FRAME );
			//}
			float flPassNum[4];
			flPassNum[0] = params[PASS]->GetFloatValue();
			flPassNum[1] = flPassNum[2] = flPassNum[3] = flPassNum[0];
			pShaderAPI->SetPixelShaderConstant( 0, flPassNum );
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0, 0 ); //Current frame
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE1, 1 ); //Old frame

			pShaderAPI->SetVertexShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER

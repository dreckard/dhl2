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
#include "dhl_blur_ps20.inc"
#include "dhl_blur_vs20.inc"
#include <time.h>

//Params adjusted by client.dll
float flShiftInterval;
float flUpdateInterval;
float flMin;
float flMax;
float flGameCurtime;
float flTimescale;

//float flShiftTime; //Obselete
float flUpdateTime;
float flChangePerUpdate;
float amt;
bool bGoingUp;

BEGIN_VS_SHADER( DHL_Blur, "Blur shader" )

	//These are parsed from the .vmt file for textures on which the shader is applied
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SHIFTINTERVAL, SHADER_PARAM_TYPE_FLOAT, "5.0", "Time taken to go from min blur to max blur" )
		SHADER_PARAM( UPDATEINTERVAL, SHADER_PARAM_TYPE_FLOAT, "0.025", "Number of seconds between blur value updates (lower values will make things smoother)" )
		SHADER_PARAM( MIN, SHADER_PARAM_TYPE_FLOAT, "0.01", "Min blur" )
		SHADER_PARAM( MAX, SHADER_PARAM_TYPE_FLOAT, "0.05", "Max blur" )
		SHADER_PARAM( GAMECURTIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "Curtime from HL2" )
		SHADER_PARAM( TIMESCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "Timescale" )
		SHADER_PARAM( FRACTION, SHADER_PARAM_TYPE_FLOAT, "1.0", "Local player's health divided by the health at which effects begin" )
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
		flShiftInterval = params[SHIFTINTERVAL]->GetFloatValue();
		flUpdateInterval = params[UPDATEINTERVAL]->GetFloatValue();
		flMin = params[MIN]->GetFloatValue();
		flMax = params[MAX]->GetFloatValue();
		flGameCurtime = params[GAMECURTIME]->GetFloatValue();
		flTimescale = params[TIMESCALE]->GetFloatValue();
	}

	//This is called once when the shader is first loaded
	SHADER_INIT
	{
		UpdateParams(params);

		//flShiftTime = GetClockTime() + flShiftInterval;
		flUpdateTime = flGameCurtime + flUpdateInterval;
		bGoingUp = true;
		amt = flMin;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Enable the texture for base texture and lightmap.
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableDepthWrites( false );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "dhl_blur_vs20", 0 );
			pShaderShadow->SetPixelShader( "dhl_blur_ps20" );

			// Optional, do some blending..
			//pShaderShadow->EnableBlending( true );
			//pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );

			DefaultFog();
		}
		DYNAMIC_STATE
		{
			UpdateParams(params);

			//We need to increase/decrease by this amount every update for everything to work out
			flChangePerUpdate = flTimescale * (( flMax - flMin ) / ( flShiftInterval / flUpdateInterval ));

			//float flCurtime = GetClockTime();
			//if ( flShiftTime <= flCurtime )
			if ( amt > flMax || amt < flMin )
			{
				//Toggle the var
				bGoingUp = !bGoingUp;

				if ( amt > flMax )
					amt = flMax;
				else if ( amt < flMin )
					amt = flMin;
				//Reset the timer
				//flShiftTime = flCurtime + flShiftInterval;
			}

			if ( flUpdateTime <= flGameCurtime )
			{
				if ( bGoingUp )
					amt += flChangePerUpdate;
				else
					amt -= flChangePerUpdate;
				flUpdateTime = flGameCurtime + flUpdateInterval;
			}

			//Make sure these don't get out of bounds
			/*if ( flShiftTime > flCurtime + flShiftInterval )
				flShiftTime = flCurtime + flShiftInterval;
			if ( flUpdateTime > flCurtime + flUpdateInterval )
				flUpdateTime = flCurtime + flUpdateInterval;*/

			// Tell it how wide the blur is.
			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			float v[4] = {0,0,0,0};
			v[0] = amt;
			//Hook it up so that it can be read by the shader
			//pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v, true );
			//This has been moved to the pixel shader
			pShaderAPI->SetPixelShaderConstant( 1, v );

			/*float factor[4];
			factor[0] = params[SHARPENFACTOR]->GetFloatValue();
			factor[1] = factor[2] = factor[3] = factor[0];
			pShaderAPI->SetPixelShaderConstant( 0, factor );*/

			const float BaseRedScalar = 1.0f;

			float redscalar[4];
			redscalar[0] = BaseRedScalar / params[FRACTION]->GetFloatValue();
			redscalar[1] = redscalar[2] = redscalar[3] = redscalar[0];

			pShaderAPI->SetPixelShaderConstant( 0, redscalar );

			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
			pShaderAPI->SetVertexShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER

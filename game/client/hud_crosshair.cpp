//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_crosshair.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/controls.h"
#include "vgui/ISurface.h"
#include "IVRenderView.h"

//DHL - Skillet
#include "input.h" //For CAM_IsThirdPerson()

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar crosshair( "crosshair", "1", FCVAR_ARCHIVE );
ConVar cl_observercrosshair( "cl_observercrosshair", "1", FCVAR_ARCHIVE );

//DHL - Skillet - The Great Stub Reef.
//The engine gets upset when the options panel is opened if it can't find these
ConVar cl_crosshairusealpha( "cl_crosshairusealpha", "0", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshairalpha( "cl_crosshairalpha", "255", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshair_red ( "cl_crosshair_red", "", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshair_green ( "cl_crosshair_green", "", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshair_blue ( "cl_crosshair_blue", "", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshair_scale ( "cl_crosshair_scale", "", FCVAR_NONE, "Stub" ); //Stub
ConVar cl_crosshair_file ( "cl_crosshair_file", "", FCVAR_NONE, "Stub" ); //Stub
//The rest aren't related to the crosshair but there's no better place to put them
ConVar cl_himodels ( "cl_himodels", "", FCVAR_NONE, "Stub" ); //Stub
ConVar topcolor ( "topcolor", "", FCVAR_NONE, "Stub" ); //Stub
ConVar bottomcolor ( "bottomcolor", "", FCVAR_NONE, "Stub" ); //Stub
//Happens on Options->Video->Advanced
ConVar fov_desired ( "fov_desired", "", FCVAR_NONE, "Stub" ); //Stub
//On game startup
ConVar hud_saytext ( "hud_saytext", "", FCVAR_NONE, "Stub" ); //Stub
ConVar spec_drawstatus ( "spec_drawstatus", "", FCVAR_NONE, "Stub" ); //Stub

//DHL - Skillet - Actually used
ConVar cl_crosshaircolor ( "cl_crosshaircolor", "0", FCVAR_ARCHIVE );
ConVar cl_crosshairscale ( "cl_crosshairscale", "768", FCVAR_ARCHIVE );

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

DECLARE_HUDELEMENT( CHudCrosshair );

CHudCrosshair::CHudCrosshair( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudCrosshair" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = 0;

	m_clrCrosshair = Color( 0, 0, 0, 0 );

	m_vecCrossHairOffsetAngle.Init();

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );
}

void CHudCrosshair::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_pDefaultCrosshair = gHUD.GetIcon("crosshair_default");
	SetPaintBackgroundEnabled( false );

	//DHL - Skillet
	m_pCrosshair = m_pDefaultCrosshair;

    SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudCrosshair::ShouldDraw( void )
{
	bool bNeedsDraw;

	if ( m_bHideCrosshair )
		return false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	//DHL - Skillet - Don't draw when dead
	if ( !pPlayer->IsAlive() )
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon && !pWeapon->ShouldDrawCrosshair() )
		return false;

	/* disabled to avoid assuming it's an HL2 player.
	// suppress crosshair in zoom.
	if ( pPlayer->m_HL2Local.m_bZooming )
		return false;
	*/

	// draw a crosshair only if alive or spectating in eye
	if ( IsX360() )
	{
		bNeedsDraw = m_pCrosshair && 
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			( !pPlayer->IsSuitEquipped() || g_pGameRules->IsMultiplayer() ) &&
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			( pPlayer->IsAlive() ||	( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) || ( cl_observercrosshair.GetBool() && pPlayer->GetObserverMode() == OBS_MODE_ROAMING ) );
	}
	else
	{
		bNeedsDraw = m_pCrosshair && 
			crosshair.GetInt() &&
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			!pPlayer->IsInVGuiInputMode() &&
			( pPlayer->IsAlive() ||	( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) || ( cl_observercrosshair.GetBool() && pPlayer->GetObserverMode() == OBS_MODE_ROAMING ) );
	}

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

#define CROSS_SCALE_DEN 4 //DHL
void CHudCrosshair::Paint( void )
{
	if ( !m_pCrosshair )
		return;

	if ( !IsCurrentViewAccessAllowed() )
		return;

	//DHL - Skillet
	ConVarRef cvarColor( "cl_crosshaircolor" );
	switch ( cvarColor.GetInt() )
	{
	case 0: //Green
		m_clrCrosshair = Color( 0, 255, 0, 255 );
		break;
		
	case 1: //Red
		m_clrCrosshair = Color( 255, 0, 0, 255 );
		break;

	case 2: //Blue
		m_clrCrosshair = Color( 0, 0, 255, 255 );
		break;

	case 3: //Yellow
		m_clrCrosshair = Color( 255, 255, 0, 255 );
		break;

	case 4: //Light Blue
		m_clrCrosshair = Color( 0, 255, 255, 255 );
		break;

	default:
		m_clrCrosshair = Color( 255, 255, 255, 255 );
		break;
	}

	//Crosshair textures are 128x128 at the moment which is 4x the original 32x32 HL2 ones
	float flScale = 1.5f/CROSS_SCALE_DEN;
	ConVarRef cvarScale( "cl_crosshairscale" );
	switch ( cvarScale.GetInt() )
	{
	//case 0: //"Auto-size"
	//	break;

	case 1200: //Small
		flScale = 2.0f/CROSS_SCALE_DEN;
		break;

	case 768: //Medium
		flScale = 3.0f/CROSS_SCALE_DEN;
		break;

	case 600: //Large
		flScale = 4.0f/CROSS_SCALE_DEN;
		break;
		
	default:
		break;

	}

	m_curViewAngles = CurrentViewAngles();
	m_curViewOrigin = CurrentViewOrigin();

	float x, y;
	x = ScreenWidth()/2;
	y = ScreenHeight()/2;

	// MattB - m_vecCrossHairOffsetAngle is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the 
	// screen
	if ( m_vecCrossHairOffsetAngle != vec3_angle )
	{
		QAngle angles;
		Vector forward;
		Vector point, screen;

		// this code is wrong
		angles = m_curViewAngles + m_vecCrossHairOffsetAngle;
		AngleVectors( angles, &forward );
		VectorAdd( m_curViewOrigin, forward, point );
		ScreenTransform( point, screen );

		x += 0.5f * screen[0] * ScreenWidth() + 0.5f;
		y += 0.5f * screen[1] * ScreenHeight() + 0.5f;
	}

	/*m_pCrosshair->DrawSelf( 
			x - 0.5f * m_pCrosshair->Width(), 
			y - 0.5f * m_pCrosshair->Height(),
			m_clrCrosshair );*/

	//DHL - Skillet - Third person needs to place the crosshair dynamically
	//int finalXPos = x - 0.5f * (m_pCrosshair->Width() * flScale);
	//int finalYPos = y - 0.5f * (m_pCrosshair->Height() * flScale);
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		if ( ::input->CAM_IsThirdPerson() )
		{
			trace_t tr;
			Vector vecDir = vec3_origin;
			AngleVectors( pPlayer->EyeAngles(), &vecDir );
			UTIL_TraceLine( pPlayer->Weapon_ShootPosition(), pPlayer->Weapon_ShootPosition() + ( vecDir * MAX_TRACE_LENGTH ), 
				MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
			if ( tr.DidHit() && !tr.allsolid )
			{
				Vector vecScreenPos = vec3_origin;
				ScreenTransform( tr.endpos, vecScreenPos );
				x += vecScreenPos.x * ScreenWidth() * 0.5f;
				y -= vecScreenPos.y * ScreenHeight() * 0.5f;
			}
		}
	}

	//DHL - Skillet
	m_pCrosshair->DrawSelf( 
			x - 0.5f * (m_pCrosshair->Width() * flScale), 
			y - 0.5f * (m_pCrosshair->Height() * flScale),
			m_pCrosshair->Width() * flScale,
			m_pCrosshair->Height() * flScale,
			m_clrCrosshair );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshairAngle( const QAngle& angle )
{
	VectorCopy( angle, m_vecCrossHairOffsetAngle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshair( CHudTexture *texture, Color& clr )
{
	m_pCrosshair = texture;
	m_clrCrosshair = clr;
}

//-----------------------------------------------------------------------------
// Purpose: Resets the crosshair back to the default
//-----------------------------------------------------------------------------
void CHudCrosshair::ResetCrosshair()
{
	//DHL - Skillet - C_BaseCombatWeapon::DrawCrosshair() calls this every frame for weapons without a crosshair in script
	if ( m_pCrosshair != m_pDefaultCrosshair )
		SetCrosshair( m_pDefaultCrosshair, Color(255, 255, 255, 255) );
}

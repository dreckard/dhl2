//=============================================================================//
// Purpose:	Distraction Half-Life 2 round time HUD
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "hud_basetimer.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "dhl/dhl_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CHudDHLRoundTime : public CHudElement, public CHudBaseTimer
{
	DECLARE_CLASS_SIMPLE( CHudDHLRoundTime, CHudBaseTimer );

public:
	CHudDHLRoundTime( const char *pElementName ) : CHudElement( pElementName ), CHudBaseTimer( NULL, "DHLRoundTime" )
	{
		SetPaintBackgroundEnabled( false );
		m_clrDefaultFg = Color( 0, 0, 0, 255 );
		m_clrLowTime = Color( 255, 0, 0, 255 ); //Red
	}
public:
	void OnThink( void );
	bool ShouldDraw( void );
	void VidInit( void );
	void Paint( void );
	void MsgFunc_RoundEnd( bf_read &msg );
	void Init( void );

protected:
	void ApplySchemeSettings(IScheme *pScheme);

private:
	void DrawMsg( wchar_t* wszStr, int xPos, int yPos );
	Color m_clrDefaultFg;
	Color m_clrLowTime;
	HFont m_nMsgFont;
	Color m_clrMsg;
	wchar_t wszHudText[50];
	float flHudTextTime;
};
DECLARE_HUDELEMENT( CHudDHLRoundTime );
DECLARE_HUD_MESSAGE( CHudDHLRoundTime, RoundEnd );

void CHudDHLRoundTime::Init( void )
{
	HOOK_HUD_MESSAGE( CHudDHLRoundTime, RoundEnd );
	CHudElement::Init();
}

void CHudDHLRoundTime::VidInit( void )
{
	m_nMsgFont = surface()->CreateFont();
	surface()->SetFontGlyphSet( m_nMsgFont, "Impact", YRES(18), 0, 0, 0, surface()->FONTFLAG_ANTIALIAS | surface()->FONTFLAG_ADDITIVE | surface()->FONTFLAG_CUSTOM );
	m_clrMsg = gHUD.m_clrNormal;
	m_clrMsg[3] = 204; //80% alpha
	flHudTextTime = -1.0f;
}

void CHudDHLRoundTime::OnThink( void )
{
	//Update time
	if ( DHLRules() )
	{
		SetMinutes( DHLRules()->GetNetRoundTimeLeft() / 60 );
		SetSeconds( DHLRules()->GetNetRoundTimeLeft() % 60 );

		//Make the timer red if we're under 10 seconds remaining
		if ( DHLRules()->GetNetRoundTimeLeft() < 10.0f && GetFgColor() != m_clrLowTime )
			SetFgColor( m_clrLowTime );
		if ( DHLRules()->GetNetRoundTimeLeft() > 10.0f && GetFgColor() != m_clrDefaultFg )
			SetFgColor( m_clrDefaultFg );
	}
	BaseClass::OnThink();
}

void CHudDHLRoundTime::Paint( void )
{
	if ( DHLRules()->IsGameWaiting() )
	{
		wchar_t wszLocalized[75];
		static ConVarRef firstRoundStartDelay( "dhl_firstroundstartdelay" );
		if ( DHLRules()->m_iTimeToRoundStart >= 0 && UTIL_PlayerByIndex(2) ) //Do we have at least 2 players (1-based)?
		{
			wchar_t wszTime[5];
			swprintf( wszTime, 5, L"%i", DHLRules()->m_iTimeToRoundStart );
			//_snwprintf( health, sizeof( health ), L"%i", iHealth );
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#DHL_GAME_STARTDELAY" ), 1, wszTime );
		}
		else
			swprintf( wszLocalized, 75, g_pVGuiLocalize->Find( "#DHL_GAME_WAITING" ) );

		int iXPos, iYPos, iTxtW, iTxtH;
		GetPos( iXPos, iYPos );
		surface()->GetTextSize( m_nMsgFont, wszLocalized, iTxtW, iTxtH );
		DrawMsg( wszLocalized, (ScreenWidth() / 2) - (iTxtW / 2), iYPos + YRES(35) );
	}

	if ( flHudTextTime > gpGlobals->curtime )
	{
		int iXPos, iYPos, iTxtW, iTxtH;
		GetPos( iXPos, iYPos );
		surface()->GetTextSize( m_nMsgFont, wszHudText, iTxtW, iTxtH );
		DrawMsg( wszHudText, (ScreenWidth() / 2) - (iTxtW / 2), iYPos + YRES(50) );
	}

	BaseClass::Paint();
}

void CHudDHLRoundTime::DrawMsg( wchar_t* wszStr, int xPos, int yPos )
{
	//Draw right on the root viewport instead of on this HUD element's panel
	surface()->PushMakeCurrent( g_pClientMode->GetViewport()->GetVPanel(), false );
	surface()->DrawSetTextFont(m_nMsgFont);
	surface()->DrawSetTextColor(m_clrMsg);
	surface()->DrawSetTextPos( xPos, yPos );
	surface()->DrawUnicodeString( wszStr );
	surface()->PopMakeCurrent( g_pClientMode->GetViewport()->GetVPanel() );
}

void CHudDHLRoundTime::MsgFunc_RoundEnd( bf_read &msg )
{
	bool bSuccess = false;
	byte val = msg.ReadByte();
	if ( val == 255 ) //Arbitrary "round draw" value
	{
		wcsncpy( wszHudText, g_pVGuiLocalize->Find( "#DHL_ROUND_DRAW" ), 50 );
		bSuccess = true;
	}
	else
	{
		if ( DHLRules()->IsTeamplay() )
		{
			//Val indicates winning team #
			C_Team* pTeam = GetGlobalTeam( val );
			if ( pTeam )
			{
				wchar_t wszTeamName[32];
				g_pVGuiLocalize->ConvertANSIToUnicode( pTeam->Get_Name(), wszTeamName, sizeof(wszTeamName) );
				g_pVGuiLocalize->ConstructString( wszHudText, sizeof( wszHudText ), g_pVGuiLocalize->Find( "#DHL_ROUNDPLAY_WINNER" ), 1, wszTeamName );
				bSuccess = true;
			}
		}
		else
		{
			//Val indicates winning player's index
			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( val );
			if ( pPlayer )
			{
				wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
				g_pVGuiLocalize->ConstructString( wszHudText, sizeof( wszHudText ), g_pVGuiLocalize->Find( "#DHL_LMS_WINNER" ), 1, wszPlayerName );
				bSuccess = true;
			}
		}
	}

	static ConVarRef restartDelay( "dhl_roundrestartdelay" );
	if ( bSuccess )
		flHudTextTime = gpGlobals->curtime + restartDelay.GetFloat();
}

bool CHudDHLRoundTime::ShouldDraw( void )
{
	return ( DHLRules() && DHLRules()->IsRoundplay() );
}

void CHudDHLRoundTime::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_clrDefaultFg = pScheme->GetColor( "FgColor", Color( 0, 0, 0 ) );
}
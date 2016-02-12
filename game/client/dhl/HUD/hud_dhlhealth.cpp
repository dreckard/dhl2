//=============================================================================//
// Purpose:	Distraction Half-Life 2 health, armor, and ammo HUD
//
// Author: Payback
// Edited by Skillet
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "dhl/dhl_player_inc.h"
#include "dhl/dhl_shareddefs.h"

using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include "VguiMatSurface/IMatSystemSurface.h"
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DHLHEALTH_INFO_EVENT_DURATION	1.0f
/*
==================================================
CHUDDHLHealth
==================================================
*/

using namespace vgui;

class CHudDHLHealth : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDHLHealth, vgui::Panel );
public:
	CHudDHLHealth( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void OnThink();
	virtual void Paint();
	
	virtual void ApplySchemeSettings( IScheme *scheme );
private:
	
	void	DrawWarning( int x, int y, CHudTexture *icon, float &time );
	void	UpdateEventTime( void );
	bool	EventTimeElapsed( void );

	//DHL - Skillet
	int		m_iLastArmor;
	int		m_iLastHealth;

	bool	m_bFadedOut;
	
	bool	m_bDimmed;			// Whether or not we are dimmed down
	float	m_flLastEventTime;	// Last active event (controls dimmed state)

	//CHudTexture	*m_icon_rb;		// right bracket, full
	//CHudTexture	*m_icon_lb;		// left bracket, full
	//CHudTexture	*m_icon_rbe;	// right bracket, empty
	//CHudTexture	*m_icon_lbe;	// left bracket, empty
	CHudTexture *m_icon_background;
	CHudTexture *m_icon_health;
	CHudTexture *m_icon_armor;
	CHudTexture *m_icon_arminact; //DHL - Skillet - Texture for "inactive" armor
	CHudTexture *m_icon_blank; //Blank transparent texture

	vgui::HFont m_nAmmoFont;
	vgui::HFont m_nSmallAmmoFont;
	Color m_nTextColor;
	int m_iAmmoFontHeight;
	int m_iBaseTextCoords[2];
};

DECLARE_HUDELEMENT( CHudDHLHealth );

// The "HudDHLHealth" in this line links us to the HudLayout.res
CHudDHLHealth::CHudDHLHealth( const char *pElementName ) :	CHudElement( pElementName ), BaseClass( NULL, "HudDHLHealth" ) 
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudDHLHealth::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}


void CHudDHLHealth::Init( void )
{
	m_bFadedOut			= false;
	m_bDimmed			= false;
	m_flLastEventTime   = 0.0f;

	m_iLastArmor = -1.0f;
	m_iLastHealth = -1.0f;
}


void CHudDHLHealth::VidInit( void )
{
	Init();

	//m_icon_lb =		gHUD.GetIcon( "hud_armor_full" );
	//m_icon_lbe =	gHUD.GetIcon( "hud_armor_empty" );
	m_icon_background = gHUD.GetIcon( "hud_health_background" );
	m_icon_health = gHUD.GetIcon( "hud_health" );
	m_icon_armor = gHUD.GetIcon( "hud_armor" );
	m_icon_arminact = gHUD.GetIcon( "hud_armor_inactive" );
	m_icon_blank = gHUD.GetIcon( "blank" );

	if ( engine->GetScreenAspectRatio() != 4.0f/3.0f )
	{
		SetSize( XRES(128), YRES(128) );
		SetPos( ScreenWidth() - GetWide(), ScreenHeight() - GetTall() );
	}

	//m_icon_rb =		gHUD.GetIcon( "hud_health_full" );
	//m_icon_rbe =	gHUD.GetIcon( "hud_health_empty" );

	//DHL - Skillet - Custom fonts - It sucks that these values are hard coded, but I don't know of any existing way to suck them from script
	m_nAmmoFont = surface()->CreateFont();
	surface()->SetFontGlyphSet( m_nAmmoFont, "Impact", YRES(18), 0, 0, 0, surface()->FONTFLAG_ANTIALIAS | surface()->FONTFLAG_ADDITIVE | surface()->FONTFLAG_CUSTOM );
	m_nSmallAmmoFont = surface()->CreateFont();
	surface()->SetFontGlyphSet( m_nSmallAmmoFont, "Impact", YRES(12), 0, 0, 0, surface()->FONTFLAG_ANTIALIAS | surface()->FONTFLAG_ADDITIVE | surface()->FONTFLAG_CUSTOM );

	m_nTextColor = gHUD.m_clrNormal;
	//m_nTextColor[3] = 255; //Opaque
	m_iAmmoFontHeight = surface()->GetFontTall( m_nAmmoFont );
	m_iBaseTextCoords[0] = XRES(15); m_iBaseTextCoords[1] = YRES(64) - m_iAmmoFontHeight;

	SetPaintEnabled(true);
}

bool CHudDHLHealth::ShouldDraw( void )
{
	//if ( !m_icon_arminact || !m_icon_rb || !m_icon_rbe || !m_icon_lb || !m_icon_lbe )
	if ( !(m_icon_background && m_icon_health && m_icon_armor && m_icon_arminact) )
		return false;

	if ( !C_BasePlayer::GetLocalPlayer() )
		return false;

	return ( CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage() );
}

void CHudDHLHealth::OnThink()
{
	BaseClass::OnThink();
}

void CHudDHLHealth::Paint()
{
	C_DHL_Player *pPlayer = ToDHLPlayer(C_BasePlayer::GetLocalPlayer());
	if ( !pPlayer )
		return;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		//Ammo Counter
		// get the ammo in our clip
		int ammo1 = pWeapon->Clip1();
		int ammo2;
		if (ammo1 < 0)
		{
			// we don't use clip ammo, just use the total ammo count
			ammo1 = pPlayer->GetAmmoCount(pWeapon->GetPrimaryAmmoType());
			ammo2 = 0;
		}
		else
		{
			// we use clip ammo, so the second ammo is the total ammo
			ammo2 = pPlayer->GetAmmoCount(pWeapon->GetPrimaryAmmoType());
		}

		surface()->DrawSetTextFont(m_nAmmoFont);
		surface()->DrawSetTextColor(m_nTextColor);
		surface()->DrawSetTextPos(m_iBaseTextCoords[0], m_iBaseTextCoords[1]);

		wchar_t unicode[25];
		swprintf(unicode, 20, L"%i", ammo1);

		//Scale it
		//Note: Doesn't work.
		/*int idx = 0;
		while ( unicode[idx] != 0 )
		{
			float flScaleX = ScreenWidth() / 640.0f;
			float flScaleY = ScreenHeight() / 480.0f;
			vgui::CharRenderInfo info;
			if ( vgui::surface()->DrawGetUnicodeCharRenderInfo( unicode[idx], info ) )
			{
				info.verts[0].m_Position.x = Lerp( flScaleX, info.verts[0].m_Position.x, info.verts[1].m_Position.x );
				info.verts[0].m_TexCoord.x = Lerp( flScaleX, info.verts[0].m_TexCoord.x, info.verts[1].m_TexCoord.x );

				info.verts[0].m_Position.y = Lerp( flScaleY, info.verts[0].m_Position.y, info.verts[1].m_Position.y );
				info.verts[0].m_TexCoord.y = Lerp( flScaleY, info.verts[0].m_TexCoord.y, info.verts[1].m_TexCoord.y );
			}
			surface()->DrawRenderCharFromInfo( info );
			surface()->DrawSetTextPos( m_iBaseTextCoords[0] + surface()->GetCharacterWidth( m_nAmmoFont, unicode[idx] ), m_iBaseTextCoords[1] );
			idx++;
		}*/

		surface()->DrawUnicodeString( unicode );

		//Determine offsets
		int iOffsetX = XRES(2); //Push out by 2 pixels always so numbers aren't touching
		for ( int i = 0; i < 6; i++ )
		{
			if ( unicode[i] == 0 )
				break;
			iOffsetX += surface()->GetCharacterWidth( m_nAmmoFont, unicode[i] );
		}

		int iOffsetY = m_iAmmoFontHeight / 2;

		surface()->DrawSetTextFont(m_nSmallAmmoFont);
		swprintf(unicode, 20, L"%i", ammo2);
		surface()->DrawSetTextPos(m_iBaseTextCoords[0] + iOffsetX, m_iBaseTextCoords[1] + iOffsetY);
		surface()->DrawUnicodeString( unicode );

		//DHL TEST: Style points
		swprintf(unicode, 20, L"SP: %i/%i", pPlayer->m_iStylePoints, DHL_MAXSTYLEPOINTS );
		//surface()->DrawSetTextFont(m_nAmmoFont);
		static ConVarRef slowVar( "dhl_slowmotion" );
		if ( !slowVar.GetBool() )
			surface()->DrawSetTextColor( 255, 0, 0, 255 );
		surface()->DrawSetTextPos( XRES(15), YRES(82) );
		surface()->DrawUnicodeString( unicode );
	}

	int	health	= pPlayer->GetHealth();
	int armor = pPlayer->GetDHLArmor();
	
	Color clrNormal = gHUD.m_clrNormal;

	m_icon_background->DrawSelf( 0, 0, XRES(128), YRES(128), clrNormal );

	// Update our armor
	float armorPerc = (float) armor / 100.0f;
	armorPerc = clamp( armorPerc, 0.0f, 1.0f );
	Color armorColor = gHUD.m_clrNormal;

	//Note that the X/Y coords in these drawing functions are relative to the panel, not the entire screen
	if ( armor <= 0 )
		m_icon_arminact->DrawSelf( 0, YRES(75.0f/2.0f), XRES(m_icon_arminact->Width()/2), YRES(m_icon_arminact->Height()/2), armorColor );
	else
		gHUD.DrawScaledIconProgressBar( 0, YRES(75.0f/2.0f), XRES(m_icon_armor->Width()/2), YRES(m_icon_armor->Height()/2), m_icon_armor, m_icon_blank, ( 1.0 - armorPerc ), armorColor, CHud::HUDPB_VERTICAL );

	float healthPerc = (float) health / 100.0f;
	Color healthColor = gHUD.m_clrNormal;

	gHUD.DrawScaledIconProgressBar( 0, YRES(75.0f/2.0f), XRES(m_icon_health->Width()/2), YRES(m_icon_health->Height()/2), m_icon_health, m_icon_blank, ( 1.0 - healthPerc ), healthColor, CHud::HUDPB_VERTICAL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDHLHealth::UpdateEventTime( void )
{
	m_flLastEventTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudDHLHealth::EventTimeElapsed( void )
{
	if (( gpGlobals->curtime - m_flLastEventTime ) > DHLHEALTH_INFO_EVENT_DURATION )
		return true;

	return false;
}
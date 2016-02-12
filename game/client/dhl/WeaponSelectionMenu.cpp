//=============================================================================
// Distraction Half-Life 2
// Main weapon selection menu
// Author: Skillet
//=============================================================================

#include "cbase.h"
#include "WeaponSelectionMenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSelectionMenu::CWeaponSelectionMenu(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTION, 18 )
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionMenu::~CWeaponSelectionMenu()
{
}

void CWeaponSelectionMenu::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;

	SetTitle("Choose a weapon category", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionMenu.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	/*m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("Melee"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("Pistols"));
	m_pButtons[2] = dynamic_cast<Button *>(FindChildByName("Automatics"));
	m_pButtons[3] = dynamic_cast<Button *>(FindChildByName("Shotguns"));
	m_pButtons[4] = dynamic_cast<Button *>(FindChildByName("Rifles"));
	m_pButtons[5] = dynamic_cast<Button *>(FindChildByName("Items"));*/
	//Melee
	m_pButtons[0] = static_cast<Button *>(FindChildByName("CombatKnife"));
	m_iWeaponIndices[0] = WEAPON_COMBATKNIFE;
	m_pButtons[1] = static_cast<Button *>(FindChildByName("Katana"));
	m_iWeaponIndices[1] = WEAPON_KATANA;
	m_pButtons[2] = static_cast<Button *>(FindChildByName("BaseballBat"));
	m_iWeaponIndices[2] = WEAPON_BASEBALLBAT;
	//Handguns
	m_pButtons[3] = static_cast<Button *>(FindChildByName("Beretta"));
	m_iWeaponIndices[3] = WEAPON_BERETTA;
	m_pButtons[4] = static_cast<Button *>(FindChildByName("Beretta_Akimbo"));
	m_iWeaponIndices[4] = WEAPON_BERETTA_AKIMBO;
	m_pButtons[5] = static_cast<Button *>(FindChildByName("Deagle"));
	m_iWeaponIndices[5] = WEAPON_DEAGLE;
	m_pButtons[6] = static_cast<Button *>(FindChildByName("SAA"));
	m_iWeaponIndices[6] = WEAPON_SAA;
	//Automatics
	m_pButtons[7] = static_cast<Button *>(FindChildByName("AK47"));
	m_iWeaponIndices[7] = WEAPON_AK47;
	m_pButtons[8] = static_cast<Button *>(FindChildByName("Mac11"));
	m_iWeaponIndices[8] = WEAPON_MAC11;
	m_pButtons[9] = static_cast<Button *>(FindChildByName("Thompson"));
	m_iWeaponIndices[9] = WEAPON_THOMPSON;
	//Shotguns
	m_pButtons[10] = static_cast<Button *>(FindChildByName("Remington"));
	m_iWeaponIndices[10] = WEAPON_REMINGTON;
	m_pButtons[11] = static_cast<Button *>(FindChildByName("SawedOff"));
	m_iWeaponIndices[11] = WEAPON_SAWEDOFF;
	//Rifles
	m_pButtons[12] = static_cast<Button *>(FindChildByName("Winchester"));
	m_iWeaponIndices[12] = WEAPON_WINCHESTER;
	m_pButtons[13] = static_cast<Button *>(FindChildByName("MosinNagant"));
	m_iWeaponIndices[13] = WEAPON_MOSINNAGANT;
	//Items
	m_pButtons[14] = static_cast<Button *>(FindChildByName("Flashlight"));
	m_iWeaponIndices[14] = ITEM_FLASHLIGHT;
	m_pButtons[15] = static_cast<Button *>(FindChildByName("Nightvision"));
	m_iWeaponIndices[15] = ITEM_NIGHTVISION;
	m_pButtons[16] = static_cast<Button *>(FindChildByName("Grenade"));
	m_iWeaponIndices[16] = WEAPON_GRENADE;
	m_pButtons[17] = static_cast<Button *>(FindChildByName("Kevlar"));
	m_iWeaponIndices[17] = ITEM_KEVLAR;

	BaseClass::SetupPanel();
}

void CWeaponSelectionMenu::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
	if ( IsVisible() )
	{
		//Can't allow players to select both akimbo and single berettas
		if ( m_pButtons[3]->IsDepressed() )
			m_pButtons[4]->SetEnabled( false );
		else if ( iCurInvValue + C_DHL_Player::GetWeaponInvVal( m_iWeaponIndices[4]) < DHL_INV_VAL_SELMAX )
			m_pButtons[4]->SetEnabled( true );

		if ( m_pButtons[4]->IsDepressed() )
			m_pButtons[3]->SetEnabled( false );
		else if ( iCurInvValue + C_DHL_Player::GetWeaponInvVal( m_iWeaponIndices[3]) < DHL_INV_VAL_SELMAX )
			m_pButtons[3]->SetEnabled( true );
	}
}

void CWeaponSelectionMenu::TogglePanel()
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return;
	if ( gViewPortInterface )
	{
		IViewPortPanel* WeaponPanel = gViewPortInterface->FindPanelByName( PANEL_WEAPONSELECTION );
        if (!WeaponPanel)
		{
			DevMsg( "Couldn't find WeaponSelection menu!!\n" );
			return;
		}
		bool bShow = !(WeaponPanel->IsVisible());

		gViewPortInterface->ShowPanel( PANEL_WEAPONSELECTION , bShow );
	}
}
static ConCommand changeweapons( "changeweapons", CWeaponSelectionMenu::TogglePanel );
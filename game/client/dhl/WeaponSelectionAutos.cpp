//=============================================================================
// Distraction Half-Life 2
// Automatics weapon selection menu
// Author: Skillet
//=============================================================================

#include "cbase.h"
#include "WeaponSelectionAutos.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSelectionAutos::CWeaponSelectionAutos(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTIONAUTOS, 3 ) //3 button panel
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionAutos::~CWeaponSelectionAutos()
{
}

void CWeaponSelectionAutos::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose a weapon", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionAutos.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("AK47"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("Mac11"));
	m_pButtons[2] = dynamic_cast<Button *>(FindChildByName("Thompson"));

	m_iWeaponIndices[0] = WEAPON_AK47;
	m_iWeaponIndices[1] = WEAPON_MAC11;
	m_iWeaponIndices[2] = WEAPON_THOMPSON;

	m_szButtonCommands[0] = "select ak47";
	m_szButtonCommands[1] = "select mac11";
	m_szButtonCommands[2] = "select thompson";

	//Setup image panel
	m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";
	m_szImagePaths[2] = "weaponselection/main/cat_automatics";

	BaseClass::SetupPanel();
}

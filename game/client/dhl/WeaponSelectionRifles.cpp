//=============================================================================
// Distraction Half-Life 2
// Automatics weapon selection menu
// Author: Skillet
//=============================================================================

#include "cbase.h"
#include "WeaponSelectionRifles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSelectionRifles::CWeaponSelectionRifles(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTIONRIFLES, 2 ) //2 button panel
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionRifles::~CWeaponSelectionRifles()
{
}

void CWeaponSelectionRifles::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose a weapon", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionRifles.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("MosinNagant"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("Winchester"));

	m_iWeaponIndices[0] = WEAPON_MOSINNAGANT;
	m_iWeaponIndices[1] = WEAPON_WINCHESTER;

	m_szButtonCommands[0] = "select mosinnagant";
	m_szButtonCommands[1] = "select winchester";

	//Setup image panel
	m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";

	BaseClass::SetupPanel();
}

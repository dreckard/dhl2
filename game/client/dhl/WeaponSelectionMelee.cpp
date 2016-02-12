//=============================================================================
// Distraction Half-Life 2
// Melee weapon selection menu
// Author: Skillet
//=============================================================================
#include "cbase.h"
#include "WeaponSelectionMelee.h"

using namespace vgui;
//-----------------------------------------------------------------------------
// Pistol selection panel
//-----------------------------------------------------------------------------
CWeaponSelectionMelee::CWeaponSelectionMelee(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTIONMELEE, 3 ) //3 button panel
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionMelee::~CWeaponSelectionMelee()
{
}

void CWeaponSelectionMelee::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose a weapon", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionMelee.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("BaseballBat"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("CombatKnife"));
	m_pButtons[2] = dynamic_cast<Button *>(FindChildByName("Katana"));

	m_iWeaponIndices[0] = WEAPON_BASEBALLBAT;
	m_iWeaponIndices[1] = WEAPON_COMBATKNIFE;
	m_iWeaponIndices[2] = WEAPON_KATANA;

	m_szButtonCommands[0] = "select baseballbat";
	m_szButtonCommands[1] = "select combatknife";
	m_szButtonCommands[2] = "select katana";

	//Setup image panel
	m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";
	m_szImagePaths[2] = "weaponselection/main/cat_automatics";

	BaseClass::SetupPanel();
}

//=============================================================================
// Distraction Half-Life 2
// Automatics weapon selection menu
// Author: Skillet
//=============================================================================

#include "cbase.h"
#include "WeaponSelectionShotguns.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSelectionShotguns::CWeaponSelectionShotguns(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTIONSHOTGUNS, 2 ) //2 button panel
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionShotguns::~CWeaponSelectionShotguns()
{
}

void CWeaponSelectionShotguns::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose a weapon", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionShotguns.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("Remington"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("SawedOff"));

	m_iWeaponIndices[0] = WEAPON_REMINGTON;
	m_iWeaponIndices[1] = WEAPON_SAWEDOFF;

	m_szButtonCommands[0] = "select remington";
	m_szButtonCommands[1] = "select sawedoff";

	//Setup image panel
	m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";

	BaseClass::SetupPanel();
}

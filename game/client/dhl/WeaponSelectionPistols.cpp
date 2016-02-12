//=============================================================================
// Distraction Half-Life 2
// Automatics weapon selection menu
// Author: Skillet
//=============================================================================

#include "cbase.h"
#include "WeaponSelectionPistols.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSelectionPistols::CWeaponSelectionPistols(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_WEAPONSELECTIONPISTOLS, 3 ) //3 button panel
{
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CWeaponSelectionPistols::~CWeaponSelectionPistols()
{
}

void CWeaponSelectionPistols::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose a weapon", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/WeaponSelectionPistols.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = dynamic_cast<Button *>(FindChildByName("Beretta"));
	m_pButtons[1] = dynamic_cast<Button *>(FindChildByName("Deagle"));
	m_pButtons[2] = dynamic_cast<Button *>(FindChildByName("SAA"));
	//m_pButtons[3] = dynamic_cast<Button *>(FindChildByName("Deringer"));

	m_iWeaponIndices[0] = WEAPON_BERETTA;
	m_iWeaponIndices[1] = WEAPON_DEAGLE;
	m_iWeaponIndices[2] = WEAPON_SAA;
	//m_iWeaponIndices[3] = WEAPON_DERINGER;

	m_szButtonCommands[0] = "select beretta";
	m_szButtonCommands[1] = "select deagle";
	m_szButtonCommands[2] = "select saa";
	//m_szButtonCommands[3] = "select deringer";

	//Setup image panel
	m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";
	m_szImagePaths[2] = "weaponselection/main/cat_automatics";
	//m_szImagePaths[3] = "weaponselection/main/cat_shotguns";

	BaseClass::SetupPanel();
}

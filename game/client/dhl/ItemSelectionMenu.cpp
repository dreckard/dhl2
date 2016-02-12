//=============================================================================
// Distraction Half-Life 2
// Team selection Menu
// Author: Skillet
//=============================================================================
#include "cbase.h"
#include "ItemSelectionMenu.h"
#include "dhl/dhl_gamerules.h"

using namespace vgui;

const int NUM_ITEMS = 3;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CItemSelectionMenu::CItemSelectionMenu(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_ITEMSELECTION, NUM_ITEMS ) //3 button panel
{
	m_bIsItem = new bool[NUM_ITEMS];
	for ( int i = 0; i < NUM_ITEMS; i++ )
		m_bIsItem[i] = 0;

	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CItemSelectionMenu::~CItemSelectionMenu()
{
	delete [] m_bIsItem;
	m_bIsItem = NULL;
}

void CItemSelectionMenu::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_WEAPONSELECTION;
	SetTitle("Choose items", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/ItemSelection.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();

	//Setup buttons
	m_pButtons[0] = static_cast<Button *>(FindChildByName("Grenade"));
	m_pButtons[1] = static_cast<Button *>(FindChildByName("Kevlar"));
	m_pButtons[2] = static_cast<Button *>(FindChildByName("Nightvision"));

	m_szButtonCommands[0] = "select grenade";
	m_szButtonCommands[1] = "select kevlar";
	m_szButtonCommands[2] = "select nightvision";

	m_iWeaponIndices[0] = WEAPON_GRENADE;
	m_iWeaponIndices[1] = ITEM_KEVLAR;
	m_iWeaponIndices[2] = ITEM_NIGHTVISION;

	m_bIsItem[0] = false;
	m_bIsItem[1] = true;
	m_bIsItem[2] = true;

	//Setup image panel
	m_pImagePanel = static_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";

	BaseClass::SetupPanel();
}

void CItemSelectionMenu::ApplyChanges( void )
{
	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( m_bWeaponButtonDown[i] != m_bSavedWeaponButtonStatus[i] )
		{
			if ( m_bIsItem[i] )
				ToggleItem( m_iWeaponIndices[i] );
			else
				ToggleWeapon( m_iWeaponIndices[i] );
		}
	}
}

//=============================================================================
// Distraction Half-Life 2
// Selection Menu
// Author: Skillet
//=============================================================================
#include "cbase.h"
#include "BaseSelectionPanel.h"
#include "iclientmode.h"
#include "KeyValues.h"
#include <vgui/IInput.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar* dhl_weaponrestrictions = NULL;

CBaseSelectionPanel::CBaseSelectionPanel(IViewPort *pViewPort, const char* name, int iNumButtons) : Frame(NULL, name)
{
	m_pViewPort = pViewPort;

	//Initialize everything
	m_iNumButtons = iNumButtons;
	m_bWeaponButtonDown = new bool[iNumButtons];
	m_bSavedWeaponButtonStatus = new bool[iNumButtons];
	m_iWeaponIndices = new int[iNumButtons];
	m_szButtonCommands = new const char*[iNumButtons];
	m_szImagePaths = new const char*[iNumButtons];
	m_pButtons = new vgui::Button*[iNumButtons];

	InitializeVars();
	gameeventmanager->AddListener( this, "game_newmap", false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseSelectionPanel::~CBaseSelectionPanel()
{
	//Clean up the arrays
	delete [] m_bWeaponButtonDown;
	m_bWeaponButtonDown = NULL;
	delete [] m_bSavedWeaponButtonStatus;
	m_bSavedWeaponButtonStatus = NULL;
	delete [] m_iWeaponIndices;
	m_iWeaponIndices = NULL;
	delete [] m_szButtonCommands;
	m_szButtonCommands = NULL;
	delete [] m_szImagePaths;
	m_szImagePaths = NULL;
	delete [] m_pButtons;
	m_pButtons = NULL;
}

//Initialize vars - this gets called by the constructor
void CBaseSelectionPanel::InitializeVars( void )
{
	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		m_bWeaponButtonDown[i] = false;
		m_bSavedWeaponButtonStatus[i] = false;
		m_iWeaponIndices[i] = 0;
		m_szButtonCommands[i] = NULL;
		m_szImagePaths[i] = NULL;
		m_pButtons[i] = NULL;
	}
	m_szDefaultImage = NULL;
	m_pImagePanel = NULL;
	iCurInvValue = 0;
	pDHLPlayer = NULL;
	m_bCheckScoreboard = false;
}

void CBaseSelectionPanel::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	//Reset everything on map change
	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		ShowPanel( false );
		for ( int i = 0; i < m_iNumButtons; i++ )
		{
			m_bWeaponButtonDown[i] = false;
			m_bSavedWeaponButtonStatus[i] = false;
		}
		iCurInvValue = 0;
		pDHLPlayer = NULL;
	}
}

//This is called just before SetupPanel()
void CBaseSelectionPanel::PreSetupPanel( void )
{
	SetScheme( "ClientScheme" );
}

//Do stuff that should be applied to all weapon selection panels here
void CBaseSelectionPanel::SetupPanel( void )
{
	//SetSize( XRES(480), YRES(360) );
	//Center assuming upper left corner is 0
	SetPos( (ScreenWidth() - GetWide()) / 2, (ScreenHeight() - GetTall()) / 2 );
}

//Be aware that this function gets called sometimes (on showpanel?) even if NeedsUpdate returns false
void CBaseSelectionPanel::Update( void )
{
}
bool CBaseSelectionPanel::NeedsUpdate( void )
{
	//Awful hack - this function gets called even if the panel isn't visible, update itself doesn't
	if ( m_bCheckScoreboard && !input()->IsKeyDown(KEY_TAB) && gViewPortInterface )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
		m_bCheckScoreboard = false;
	}

	return false;
}

void CBaseSelectionPanel::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );
	SetVisible( bShow );
	SetMouseInputEnabled( bShow );

	pDHLPlayer = ToDHLPlayer(C_BasePlayer::GetLocalPlayer());
	if ( !pDHLPlayer )
		return;

	if ( !dhl_weaponrestrictions )
		dhl_weaponrestrictions = cvar->FindVar( "dhl_weaponrestrictions" );

	if ( bShow )
	{
		//Activate();
		//SetMouseInputEnabled( true );
		SaveStatus();

		if ( m_iPanelType == PANELTYPE_WEAPONSELECTION )
			iCurInvValue = pDHLPlayer->m_iSelectedInventoryValue;

		SyncDepressedStates();
	}
	else
	{
		//SetVisible( false );
		//SetMouseInputEnabled( false );
		if ( m_iPanelType == PANELTYPE_WEAPONSELECTION )
			pDHLPlayer->m_iSelectedInventoryValue = iCurInvValue;
	}
}

void CBaseSelectionPanel::TogglePanel( void )
{
	if ( BaseClass::IsVisible() )
		ShowPanel( false );
	else
		ShowPanel( true );
}

void CBaseSelectionPanel::OnCommand( const char *command )
{

	if (!Q_strcmp(command, "vguiok") )
	{
		OnOkButton();
		return;
	}
	if (!Q_strcmp(command, "vguiclose") )
	{
		OnCloseButton();
		return;
	}

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( !m_pButtons[i] )
			continue;
		if ( !Q_strcmp(command, m_pButtons[i]->GetCommand()->GetString("command") ) )
		{
			if ( !m_pButtons[i]->IsEnabled() )
				break;

			if ( !m_bWeaponButtonDown[i] )
			{
				m_bWeaponButtonDown[i] = true;
				iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[i] );
			}
			else 
			{
				m_bWeaponButtonDown[i] = false;
				iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[i] );
			}
			SyncDepressedStates();

			if ( m_iPanelType != PANELTYPE_WEAPONSELECTION )
			{
				switch(i)
				{
				case 0:
					OnButton1();
					break;
				case 1:
					OnButton2();
					break;
				case 2:
					OnButton3();
					break;
				case 3:
					OnButton4();
					break;
				case 4:
					OnButton5();
					break;
				case 5:
					OnButton6();
					break;
				default:
					break;
				}
			}
		}
	}
	vgui::Frame::OnCommand(command);
}

void CBaseSelectionPanel::OnCloseButton( void )
{
	RevertChanges();
	ShowPanel( false );
}

void CBaseSelectionPanel::OnOkButton( void )
{
	//Hide our panel
	TogglePanel();
	//Don't change their weapon selection until the 'Ok' button is clicked
	ApplyChanges();
}

void CBaseSelectionPanel::OnButton1( void )
{
	if ( m_iNumButtons < 1 )
		return;
	if ( !m_pButtons[0]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[0] )
	{
		m_bWeaponButtonDown[0] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[0] );
	}
	else 
	{
		m_bWeaponButtonDown[0] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[0] );
	}
	SyncDepressedStates();
}

void CBaseSelectionPanel::OnButton2( void )
{
	if ( m_iNumButtons < 2 )
		return;
	if ( !m_pButtons[1]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[1] )
	{
		m_bWeaponButtonDown[1] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[1] );
	}
	else
	{
		m_bWeaponButtonDown[1] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[1] );
	}
	SyncDepressedStates();
}

void CBaseSelectionPanel::OnButton3( void )
{
	if ( m_iNumButtons < 3 )
		return;
	if ( !m_pButtons[2]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[2] )
	{
		m_bWeaponButtonDown[2] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[2] );
	}
	else
	{
		m_bWeaponButtonDown[2] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[2] );
	}
	SyncDepressedStates();
}

void CBaseSelectionPanel::OnButton4( void )
{
	if ( m_iNumButtons < 4 )
		return;
	if ( !m_pButtons[3]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[3] )
	{
		m_bWeaponButtonDown[3] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[3] );
	}
	else
	{
		m_bWeaponButtonDown[3] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[3] );
	}
	SyncDepressedStates();
}

void CBaseSelectionPanel::OnButton5( void )
{
	if ( m_iNumButtons < 5 )
		return;
	if ( !m_pButtons[4]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[4] )
	{
		m_bWeaponButtonDown[4] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[4] );
	}
	else
	{
		m_bWeaponButtonDown[4] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[4] );
	}
	SyncDepressedStates();
}

void CBaseSelectionPanel::OnButton6( void )
{
	if ( m_iNumButtons < 6 )
		return;
	if ( !m_pButtons[5]->IsEnabled() )
		return;

	if ( !m_bWeaponButtonDown[5] )
	{
		m_bWeaponButtonDown[5] = true;
		iCurInvValue += pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[5] );
	}
	else
	{
		m_bWeaponButtonDown[5] = false;
		iCurInvValue -= pDHLPlayer->GetWeaponInvVal( m_iWeaponIndices[5] );
	}
	SyncDepressedStates();
}

//Make changes to the weapon selection
void CBaseSelectionPanel::ApplyChanges( void )
{
	if ( m_iPanelType != PANELTYPE_WEAPONSELECTION )
		return;

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( m_bWeaponButtonDown[i] != m_bSavedWeaponButtonStatus[i] )
		{
			if ( m_iWeaponIndices[i] >= FIRST_ITEM && m_iWeaponIndices[i] <= LAST_ITEM )
				ToggleItem( m_iWeaponIndices[i] );
			else
				ToggleWeapon( m_iWeaponIndices[i] );
		}
	}
}

void CBaseSelectionPanel::ToggleWeapon( int iWeapon )
{
	char sWeaponCommand[40];
	Q_snprintf( sWeaponCommand, sizeof(sWeaponCommand), "selectweapon %i \n", iWeapon );
	engine->ClientCmd( sWeaponCommand );
}

void CBaseSelectionPanel::ToggleItem( int iItem )
{
	char szTemp[64];
	Q_snprintf( szTemp, sizeof(szTemp), "selectitem %i\n", iItem );
	engine->ClientCmd( szTemp );
}

//Saves the status of all our vars.
//Called everytime the panel is opened
void CBaseSelectionPanel::SaveStatus( void )
{
	if ( m_iPanelType != PANELTYPE_WEAPONSELECTION )
		return;

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		m_bSavedWeaponButtonStatus[i] = m_bWeaponButtonDown[i];
	}
}

//Syncronize the variables and depression states on the buttons
void CBaseSelectionPanel::SyncDepressedStates( void )
{
	if ( m_iPanelType != PANELTYPE_WEAPONSELECTION )
		return;

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( m_pButtons[i] )
		{
			if ( m_pButtons[i]->IsDepressed() != m_bWeaponButtonDown[i] )
			{
				m_pButtons[i]->ForceDepressed( m_bWeaponButtonDown[i] );
				m_pButtons[i]->RecalculateDepressedState();
			}
			if ( m_bWeaponButtonDown[i] )
			{
				m_pButtons[i]->SetEnabled( true );
				continue;
			}
			if ( dhl_weaponrestrictions->GetBool() )
			{
				if ( iCurInvValue + pDHLPlayer->GetWeaponInvVal(m_iWeaponIndices[i]) > DHL_INV_VAL_SELMAX )
					m_pButtons[i]->SetEnabled( false );
				else
					m_pButtons[i]->SetEnabled( true );
			}
			else
				m_pButtons[i]->SetEnabled( true );
		}
	}
}

//Handle the user pressing the cancel button
//Revert everything to the way it was when we opened the panel
void CBaseSelectionPanel::RevertChanges( void )
{
	if ( m_iPanelType != PANELTYPE_WEAPONSELECTION )
		return;

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( m_bWeaponButtonDown[i] != m_bSavedWeaponButtonStatus[i] )
			m_bWeaponButtonDown[i] = m_bSavedWeaponButtonStatus[i];
	}
	//Revert the inventory value
	iCurInvValue = pDHLPlayer->m_iSelectedInventoryValue;
	SyncDepressedStates();
}


void CBaseSelectionPanel::OnKeyCodePressed( KeyCode code )
{
	switch ( code )
	{
	case KEY_ESCAPE:
		OnCloseButton();
		return;
		break;
	case KEY_ENTER:
		OnOkButton();
		return;
		break;
	case KEY_TAB: //Doesn't look at particular binding for +showscores
		if ( gViewPortInterface )
		{
			gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
			m_bCheckScoreboard = true;
		}
		return;
		break;
	}
	BaseClass::OnKeyCodePressed( code );
}

//Update the image panel for which button the cursor is over
void CBaseSelectionPanel::UpdateImages( void )
{
	if ( m_iPanelType == PANELTYPE_NOIMAGES )
		return;

	if ( !m_pImagePanel )
		return;

	const char *ImagePath = m_szDefaultImage;

	for ( int i = 0; i < m_iNumButtons; i++ )
	{
		if ( m_pButtons[i] )
		{
			if ( m_pButtons[i]->IsCursorOver() )
			{
				ImagePath = m_szImagePaths[i];
				break;
			}
		}
	}
	m_pImagePanel->setTexture( ImagePath );
}

//int CBaseSelectionPanel::MapNameToIndex( const char* name )
//{
//	if ( !Q_stricmp( name, "combatknife" ) )
//		return WEAPON_COMBATKNIFE;
//	if ( !Q_stricmp( name, "baseballbat" ) )
//		return WEAPON_BASEBALLBAT;
//	if ( !Q_stricmp( name, "katana" ) )
//		return WEAPON_KATANA;
//	if ( !Q_stricmp( name, "beretta" ) )
//		return WEAPON_BERETTA;
//	if ( !Q_stricmp( name, "deagle" ) )
//		return WEAPON_DEAGLE;
//	if ( !Q_stricmp( name, "saa" ) )
//		return WEAPON_SAA;
//	if ( !Q_stricmp( name, "deringer" ) )
//		return WEAPON_DERINGER;
//	if ( !Q_stricmp( name, "ak47" ) )
//		return WEAPON_AK47;
//	if ( !Q_stricmp( name, "mac11" ) )
//		return WEAPON_MAC11;
//	if ( !Q_stricmp( name, "thompson" ) )
//		return WEAPON_THOMPSON;
//	if ( !Q_stricmp( name, "remington" ) )
//		return WEAPON_REMINGTON;
//	if ( !Q_stricmp( name, "sawedoff" ) )
//		return WEAPON_SAWEDOFF;
//	if ( !Q_stricmp( name, "mosinnagant" ) )
//		return WEAPON_MOSINNAGANT;
//	if ( !Q_stricmp( name, "winchester" ) )
//		return WEAPON_WINCHESTER;
//	if ( !Q_stricmp( name, "grenade" ) )
//		return WEAPON_GRENADE;
//}
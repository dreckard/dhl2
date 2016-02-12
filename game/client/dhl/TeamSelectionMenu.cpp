//=============================================================================
// Distraction Half-Life 2
// Team selection Menu
// Author: Skillet
//=============================================================================
#include "cbase.h"
#include "TeamSelectionMenu.h"
#include "dhl/dhl_gamerules.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTeamSelectionMenu::CTeamSelectionMenu(IViewPort *pViewPort) : CBaseSelectionPanel( pViewPort, PANEL_TEAMSELECTION, 4 ) //4 button panel
{
	m_iLastGamemode = -1;
	m_iLastTeam = -1;
	PreSetupPanel();
	SetupPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTeamSelectionMenu::~CTeamSelectionMenu()
{
}

void CTeamSelectionMenu::SetupPanel( void )
{
	m_iPanelType = PANELTYPE_DEFAULTSELECTION;
	SetTitle("Choose a team", true);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	//Careful with the LoadControlSettings - If it's placed after the movable/sizable flaggings, the size of
	//Your panel from the resource script will not be properly applied
	LoadControlSettings("Resource/UI/TeamSelection.res");

	SetMoveable( false );
	SetSizeable( false );

	InvalidateLayout();
	m_bMapLoad = false;

	//Setup buttons
	m_pButtons[0] = static_cast<Button *>(FindChildByName("Button1"));
	m_pButtons[1] = static_cast<Button *>(FindChildByName("Button2"));
	m_pButtons[2] = static_cast<Button *>(FindChildByName("Button3"));
	m_pButtons[3] = static_cast<Button *>(FindChildByName("Button4"));

	m_szButtonCommands[0] = "select 1";
	m_szButtonCommands[1] = "select 2";
	m_szButtonCommands[2] = "select 3";
	m_szButtonCommands[3] = "select 4";

	//Setup image panel
	//m_pImagePanel = dynamic_cast<CBitmapImagePanel *>(FindChildByName("Image"));

	//Setup images
	/*m_szDefaultImage = "weaponselection/main/cat_none";
	m_szImagePaths[0] = "weaponselection/main/cat_melee";
	m_szImagePaths[1] = "weaponselection/main/cat_handguns";
	m_szImagePaths[2] = "weaponselection/main/cat_automatics";*/

	BaseClass::SetupPanel();
}

void CTeamSelectionMenu::SetData(KeyValues *data)
{
	m_bMapLoad = data->GetInt( "mapLoad", 0 );
}

void CTeamSelectionMenu::TogglePanel()
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return;
	if ( gViewPortInterface )
	{
		IViewPortPanel* TeamPanel = gViewPortInterface->FindPanelByName( PANEL_TEAMSELECTION );
        if (!TeamPanel)
		{
			DevMsg( "Couldn't find team selection menu!!\n" );
			return;
		}
		bool bShow = !(TeamPanel->IsVisible());

		gViewPortInterface->ShowPanel( PANEL_TEAMSELECTION, bShow );
	}
}
static ConCommand changeteam( "changeteam", CTeamSelectionMenu::TogglePanel );

//Update layout every time the panel is opened
void CTeamSelectionMenu::ShowPanel( bool bShow )
{
	if ( !DHLRules() )
		return;

	if ( bShow )
		UpdateLayout();

	BaseClass::ShowPanel( bShow );
}

void CTeamSelectionMenu::ChangeTeam( int iTeam )
{
	char szTemp[64];
	Q_snprintf( szTemp, sizeof(szTemp), "setteam %i \n", iTeam );
	engine->ClientCmd( szTemp );
}

void CTeamSelectionMenu::OnButton1( void )
{
	if ( DHLRules()->IsTeamplay() )
		ChangeTeam( TEAM_CMD_AUTOASSIGN );
	else
		ChangeTeam( TEAM_CMD_DEFAULT );

	//Close the panel after they select a team
	ShowPanel( false );

	//Show the weapon selection page automatically if we just loaded a map
	if ( m_bMapLoad )
	{
		gViewPortInterface->ShowPanel( PANEL_WEAPONSELECTION, true );
		m_bMapLoad = false;
	}
}

void CTeamSelectionMenu::OnButton2( void )
{
	if ( DHLRules()->IsTeamplay() )
		ChangeTeam( TEAM_CMD_MOB );
	else
		ChangeTeam( TEAM_CMD_SPEC );
	//Close the panel after they select a team
	ShowPanel( false );

	//Show the weapon selection page automatically if we just loaded a map
	if ( m_bMapLoad )
	{
		gViewPortInterface->ShowPanel( PANEL_WEAPONSELECTION, true );
		m_bMapLoad = false;
	}
}

void CTeamSelectionMenu::OnButton3( void )
{
	if ( DHLRules()->IsTeamplay() )
		ChangeTeam( TEAM_CMD_PROS );
	else
		return;
	//Close the panel after they select a team
	ShowPanel( false );

	//Show the weapon selection page automatically if we just loaded a map
	if ( m_bMapLoad )
	{
		gViewPortInterface->ShowPanel( PANEL_WEAPONSELECTION, true );
		m_bMapLoad = false;
	}
}

void CTeamSelectionMenu::OnButton4( void )
{
	if ( DHLRules()->IsTeamplay() )
		ChangeTeam( TEAM_CMD_SPEC );
	else
		return;
	//Close the panel after they select a team
	ShowPanel( false );
}

//Called when the panel is opened 
//Updates our buttons and such
void CTeamSelectionMenu::UpdateLayout( void )
{
	if ( m_iLastGamemode != DHLRules()->GetGameMode() )
	{
		if ( !DHLRules()->IsTeamplay() )
		{
			m_pJoinButton = m_pButtons[0];
			m_pSpecButton = m_pButtons[1];
			m_pMobstersButton = NULL;
			m_pProsButton = NULL;
			m_pAutoAssignButton = NULL;

			m_pJoinButton->SetText( "#DHL_TEAMSEL_JOINGAME" );
			m_pSpecButton->SetText( "#DHL_TEAMSEL_SPECTATE" );
			m_pButtons[2]->SetVisible( false );
			m_pButtons[3]->SetVisible( false );
		}
		else
		{
			m_pAutoAssignButton = m_pButtons[0];
			m_pMobstersButton = m_pButtons[1];
			m_pProsButton = m_pButtons[2];
			m_pSpecButton = m_pButtons[3];
			m_pJoinButton = NULL;

			m_pAutoAssignButton->SetText( "#DHL_TEAMSEL_AUTOASSIGN" );
			m_pMobstersButton->SetText( "#DHL_TEAMSEL_MOB" );
			m_pProsButton->SetText( "#DHL_TEAMSEL_PROS" );
			m_pSpecButton->SetText( "#DHL_TEAMSEL_SPECTATE" );

			m_pButtons[2]->SetVisible( true );
			m_pButtons[3]->SetVisible( true );
		}
		m_iLastGamemode = DHLRules()->GetGameMode();
	}

	//Team based button stuffs
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		int iTeam = pPlayer->GetTeamNumber();
		if ( m_iLastTeam != iTeam )
		{
			for ( int i = 0; i < m_iNumButtons; i++ )
				m_pButtons[i]->SetEnabled( true );
			switch ( iTeam )
			{
				case TEAM_MOBSTERS:
					if ( m_pMobstersButton )
						m_pMobstersButton->SetEnabled( false );
					if ( m_pAutoAssignButton )
						m_pAutoAssignButton->SetEnabled( false );
					break;
				case TEAM_PROS:
					if ( m_pProsButton )
						m_pProsButton->SetEnabled( false );
					if ( m_pAutoAssignButton )
						m_pAutoAssignButton->SetEnabled( false );
					break;
				case TEAM_UNASSIGNED:
					if ( m_pJoinButton )
						m_pJoinButton->SetEnabled( false );
					break;
				case TEAM_SPECTATOR:
					if ( m_pSpecButton )
						m_pSpecButton->SetEnabled( false );
					break;
			}
		}
	}
}

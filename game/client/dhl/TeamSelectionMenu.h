//=============================================================================
// Distraction Half-Life 2
// Team selection Menu
// Author: Skillet
//=============================================================================
#ifndef TeamSelectionMenu_H
#define TeamSelectionMenu_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CTeamSelectionMenu : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTeamSelectionMenu, CBaseSelectionPanel );

public:
	CTeamSelectionMenu(IViewPort *pViewPort);
	virtual ~CTeamSelectionMenu();

	virtual const char *GetName( void ) { return PANEL_TEAMSELECTION; }
	virtual void SetData(KeyValues *data);
	static void TogglePanel( void );
	virtual void ShowPanel( bool bShow );
	void UpdateLayout( void );

protected:
	virtual void SetupPanel( void );
	virtual void OnButton1( void );
	virtual void OnButton2( void );
	virtual void OnButton3( void );
	virtual void OnButton4( void );

	bool m_bMapLoad;

private:
	void ChangeTeam( int iTeam );

	int m_iLastGamemode;
	int m_iLastTeam;

	//This sucks a bit, but it's better than all the conditionals that would be need otherwise
	//Pointers that always point to buttons with certain effects
	vgui::Button* m_pAutoAssignButton;
	vgui::Button* m_pMobstersButton;
	vgui::Button* m_pProsButton;
	vgui::Button* m_pSpecButton;
	vgui::Button* m_pJoinButton;
};


#endif // TeamSelectionMenu_H

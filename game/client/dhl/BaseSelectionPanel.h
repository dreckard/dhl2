//=============================================================================
// Distraction Half-Life 2
// Base class for selection menus (weapons, teams, etc.)
// Allows up to six buttons per panel, can be increased quite easily
// Author: Skillet
//=============================================================================
#ifndef BaseSelectionPanel_H
#define BaseSelectionPanel_H
#ifdef _WIN32
#pragma once
#endif

#include "dhl/dhl_shareddefs.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/BitmapImagePanel.h>
#include "igameevents.h"

#include <game/client/iviewport.h>
#include <vgui/KeyCode.h>
#include "dhl/dhl_player_inc.h"

const int PANELTYPE_DEFAULTSELECTION = 0; //Basic buttons that trigger callback functions
const int PANELTYPE_NOIMAGES = 1;
const int PANELTYPE_WEAPONSELECTION = 2; //Toggle buttons

class CBaseSelectionPanel : public vgui::Frame, public IViewPortPanel, public IGameEventListener2
{
private:
	DECLARE_CLASS_SIMPLE( CBaseSelectionPanel, vgui::Frame );

public:
	CBaseSelectionPanel(IViewPort *pViewPort, const char* name, int iNumButtons);
	virtual ~CBaseSelectionPanel();

	virtual const char *GetName( void ) { return "UnnamedWeaponSelectionPanel"; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual void FireGameEvent( IGameEvent *event);

	//DHL - Skillet 
	//int MapNameToIndex( const char* name );

protected:
	virtual void PreSetupPanel( void );
	virtual void SetupPanel( void );
	void TogglePanel( void );
	virtual void ApplyChanges( void );
	void RevertChanges( void );
	void SaveStatus( void );
	void SyncDepressedStates( void );
	virtual void OnCloseButton( void );
	virtual void OnOkButton( void );
	virtual void OnButton1( void );
	virtual void OnButton2( void );
	virtual void OnButton3( void );
	virtual void OnButton4( void );
	virtual void OnButton5( void );
	virtual void OnButton6( void );
	void ToggleWeapon( int iWeapon );
	void ToggleItem( int iItem );
	void UpdateImages( void );

	//Dynamic Arrays
	bool* m_bWeaponButtonDown;
	bool* m_bSavedWeaponButtonStatus;

	vgui::Button** m_pButtons; //Dynamic Array
	vgui::CBitmapImagePanel* m_pImagePanel;

	//Dynamic Arrays - Double pointers are a bit ugly and confusing, but they save memory and allow more flexibility
	const char** m_szButtonCommands;
	const char** m_szImagePaths;

	const char* m_szDefaultImage;

	int* m_iWeaponIndices; //Dynamic Array
	int m_iPanelType;
	int iCurInvValue;

	int m_iNumButtons;
	
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command);

	IViewPort	*m_pViewPort;

private:
	void InitializeVars( void );
	C_DHL_Player* pDHLPlayer;
	bool m_bCheckScoreboard;

};

#endif //BaseSelectionPanel_H
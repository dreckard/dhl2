//=============================================================================
// Distraction Half-Life 2
// Main weapon selection menu
// Author: Skillet
//=============================================================================
#ifndef WeaponSelectionMenu_H
#define WeaponSelectionMenu_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CWeaponSelectionMenu : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionMenu, CBaseSelectionPanel );

public:
	CWeaponSelectionMenu(IViewPort *pViewPort);
	virtual ~CWeaponSelectionMenu();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTION; }
	static void TogglePanel( void );

protected:
	virtual void SetupPanel( void );
	void SyncDepressedStates( void );
	virtual void OnCommand( const char *command );
};


#endif // WeaponSelectionMenu_H

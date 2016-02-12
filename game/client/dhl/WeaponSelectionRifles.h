//=============================================================================
// Distraction Half-Life 2
// Rifles weapon selection menu
// Author: Skillet
//=============================================================================
#ifndef WeaponSelectionRifles_H
#define WeaponSelectionRifles_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CWeaponSelectionRifles : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionRifles, CBaseSelectionPanel );

public:
	CWeaponSelectionRifles(IViewPort *pViewPort);
	virtual ~CWeaponSelectionRifles();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTIONRIFLES; }

protected:
	virtual void SetupPanel( void );
};

#endif // WeaponSelectionMenu_H
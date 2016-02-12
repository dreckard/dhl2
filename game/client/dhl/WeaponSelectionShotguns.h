//=============================================================================
// Distraction Half-Life 2
// Shotguns weapon selection menu
// Author: Skillet
//=============================================================================
#ifndef WeaponSelectionShotguns_H
#define WeaponSelectionShotguns_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CWeaponSelectionShotguns : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionShotguns, CBaseSelectionPanel );

public:
	CWeaponSelectionShotguns(IViewPort *pViewPort);
	virtual ~CWeaponSelectionShotguns();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTIONSHOTGUNS; }

protected:
	virtual void SetupPanel( void );
};



#endif // WeaponSelectionMenu_H
//=============================================================================
// Distraction Half-Life 2
// Pistols weapon selection menu
// Author: Skillet
//=============================================================================
#ifndef WeaponSelectionPistols_H
#define WeaponSelectionPistols_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CWeaponSelectionPistols : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionPistols, CBaseSelectionPanel );

public:
	CWeaponSelectionPistols(IViewPort *pViewPort);
	virtual ~CWeaponSelectionPistols();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTIONPISTOLS; }

protected:
	virtual void SetupPanel( void );
};

#endif // WeaponSelectionMenu_H
//=============================================================================
// Distraction Half-Life 2
// Automatics weapon selection menu
// Author: Skillet
//=============================================================================
#ifndef WeaponSelectionAutos_H
#define WeaponSelectionAutos_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"


class CWeaponSelectionAutos : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionAutos, CBaseSelectionPanel );

public:
	CWeaponSelectionAutos(IViewPort *pViewPort);
	virtual ~CWeaponSelectionAutos();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTIONAUTOS; }

protected:
	virtual void SetupPanel( void );
};


#endif // WeaponSelectionMenu_H
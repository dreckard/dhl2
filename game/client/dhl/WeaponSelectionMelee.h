//=============================================================================
// Distraction Half-Life 2
//=============================================================================

#ifndef WeaponSelectionMelee_H
#define WeaponSelectionMelee_H
#ifdef _WIN32
#pragma once
#endif
#include "BaseSelectionPanel.h"

class CWeaponSelectionMelee : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CWeaponSelectionMelee, CBaseSelectionPanel );

public:
	CWeaponSelectionMelee(IViewPort *pViewPort);
	virtual ~CWeaponSelectionMelee();

	virtual const char *GetName( void ) { return PANEL_WEAPONSELECTIONMELEE; }

protected:
	virtual void SetupPanel( void );

};


#endif // WeaponSelectionMenu_H
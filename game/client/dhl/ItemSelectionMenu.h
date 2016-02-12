//=============================================================================
// Distraction Half-Life 2
// Item selection Menu
// Author: Skillet
//=============================================================================
#ifndef ITEMSELECTIONMENU_H
#define ITEMSELECTIONMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseSelectionPanel.h"

class CItemSelectionMenu : public CBaseSelectionPanel
{
private:
	DECLARE_CLASS_SIMPLE( CItemSelectionMenu, CBaseSelectionPanel );

public:
	CItemSelectionMenu(IViewPort *pViewPort);
	virtual ~CItemSelectionMenu();

	virtual const char *GetName( void ) { return PANEL_ITEMSELECTION; }

protected:
	virtual void SetupPanel( void );
	virtual void ApplyChanges( void );

private:
	bool* m_bIsItem; //Dynamic Array
};


#endif //ITEMSELECTIONMENU_H

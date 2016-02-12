#ifndef PLAYERWEAPONSELECTION_H
#define PLAYERWEAPONSELECTION_H

#include "dhl_mapfilter.h"
#include "utlvector.h"

class CPlayerWeaponSelection
{
public:
	//Constructor
	CPlayerWeaponSelection();

	//Destructor
	~CPlayerWeaponSelection();

	//Add a weapon to the player's selection
	void AddWeapon( int iWeapon );

	//Remove a weapon from the player's selection
	void RemoveWeapon( int iWeapon );

	//Add/remove it
	void ToggleWeapon( int iWeapon );

	//Have we selected this weapon?
	bool FindWeapon( const char *sWeapon );

	//Clear their weapon selection
	void Clear( void );

	//Get the total number of selected weapons
	int GetNumWeapons( void );

	//Get a member of the list by index number
	const char* GetWeaponByIndex( int iIndex );

	const char* ParseCommand( int iWeapon );
private:
	CUtlVector< const char* > *WeaponList;
};
#endif
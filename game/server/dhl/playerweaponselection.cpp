//-----------------------------------------------------------------------------
// Distraction Half-Life 2:
// Class containing each player's list of selected weapons
//-----------------------------------------------------------------------------
#include "cbase.h"
#include "playerweaponselection.h"
#include "dhl/dhl_shareddefs.h"

CPlayerWeaponSelection::CPlayerWeaponSelection()
{
	WeaponList = new CUtlVector< const char *>;
}

CPlayerWeaponSelection::~CPlayerWeaponSelection()
{
	delete WeaponList;
}

void CPlayerWeaponSelection::AddWeapon( int iWeapon )
{
	const char* sWeapon = ParseCommand( iWeapon );
	WeaponList->AddToTail( sWeapon );
}

void CPlayerWeaponSelection::RemoveWeapon( int iWeapon )
{
	const char* sWeapon = ParseCommand( iWeapon );
	WeaponList->FindAndRemove( sWeapon );
}

void CPlayerWeaponSelection::ToggleWeapon( int iWeapon )
{
	const char* sWeapon = ParseCommand( iWeapon );
	//Not found, add it
	if ( WeaponList->Find( sWeapon ) == -1 )
		WeaponList->AddToTail( sWeapon );
	//Found, remove it
	else
		WeaponList->FindAndRemove( sWeapon );
}

bool CPlayerWeaponSelection::FindWeapon( const char *sWeapon )
{
	return ( WeaponList->Find( sWeapon ) != -1 );
}

void CPlayerWeaponSelection::Clear( void )
{
	WeaponList->RemoveAll();
}

int CPlayerWeaponSelection::GetNumWeapons( void )
{
	return WeaponList->Count();
}

const char* CPlayerWeaponSelection::GetWeaponByIndex( int iIndex )
{
	return WeaponList->Element( iIndex );
}

//We send these over the wire as one or two character number strings instead of full length names
const char* CPlayerWeaponSelection::ParseCommand( int iWeapon )
{
	const char* sWeapon = NULL;
	switch( iWeapon )
	{
	case WEAPON_BASEBALLBAT:
		sWeapon = "weapon_baseballbat";
		break;
	case WEAPON_COMBATKNIFE:
		sWeapon = "weapon_combatknife";
		break;
	case WEAPON_KATANA:
		sWeapon = "weapon_katana";
		break;
	case WEAPON_BERETTA:
		sWeapon = "weapon_beretta";
		break;
	case WEAPON_BERETTA_AKIMBO:
		sWeapon = "weapon_beretta_akimbo";
		break;
	case WEAPON_DEAGLE:
		sWeapon = "weapon_deagle";
		break;
	case WEAPON_SAA:
		sWeapon = "weapon_saa";
		break;
	case WEAPON_DERINGER:
		sWeapon = "weapon_deringer";
		break;
	case WEAPON_AK47:
		sWeapon = "weapon_ak47";
		break;
	case WEAPON_MAC11:
		sWeapon = "weapon_mac11";
		break;
	case WEAPON_THOMPSON:
		sWeapon = "weapon_thompson";
		break;
	case WEAPON_REMINGTON:
		sWeapon = "weapon_remington";
		break;
	case WEAPON_SAWEDOFF:
		sWeapon = "weapon_sawedoff";
		break;
	case WEAPON_MOSINNAGANT:
		sWeapon = "weapon_mosinnagant";
		break;
	case WEAPON_WINCHESTER:
		sWeapon = "weapon_winchester";
		break;
	case WEAPON_GRENADE:
		sWeapon = "weapon_frag";
		break;
	}
	return sWeapon;
}
//=============================================================================//
// Purpose:	Distraction Half-Life 2 melee weapon baseclass
//
// Author: Skillet
//=============================================================================//
#ifndef DHL_BASEMELEEWEAPON_H
#define DHL_BASEMELEEWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbasebasebludgeon.h"

#if defined( CLIENT_DLL )
#define CDHLBaseMeleeWeapon C_DHLBaseMeleeWeapon
#endif

class CDHLBaseMeleeWeapon : public CBaseHL2MPBludgeonWeapon
{
	DECLARE_CLASS( CDHLBaseMeleeWeapon, CBaseHL2MPBludgeonWeapon );
public:
	CDHLBaseMeleeWeapon();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual void	PrimaryAttack( void );
	//No secondary attack by default
	virtual void	SecondaryAttack( void ) { return; }

	//Grab refire rate, range, and damage from scripts
	virtual	float	GetFireRate( void )								{ return	CDHLBaseWeapon::GetRateOfFire(); }
	virtual float	GetRange( void )								{ return	CDHLBaseWeapon::GetMeleeRange(); }
	virtual	float	GetDamageForActivity( Activity hitActivity )	{ return	CDHLBaseWeapon::GetMeleeDamage(); }
	virtual int		GetDamageType( void )							{ return	DMG_SLASH; } //Damage types don't need to be scripted

	CDHLBaseMeleeWeapon( const CDHLBaseMeleeWeapon & );
};

#endif
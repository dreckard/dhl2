//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Kung Fu, basic attack which is always with the player.
//
// $NoKeywords: $
//=============================================================================//

#include "weapon_hl2mpbasebasebludgeon.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponKungFu C_WeaponKungFu
#endif

//-----------------------------------------------------------------------------
// CWeaponKungFu
//-----------------------------------------------------------------------------

class CWeaponKungFu : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponKungFu, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponKungFu();

	float		GetRange( void )			{	return	KUNGFU_RANGE;	}

	float		GetDamageForActivity( Activity hitActivity );
	float		GetFireRate( void ) { return CDHLBaseWeapon::GetRateOfFire(); }
	int			GetDamageType( void ) { return DMG_CLUB; }

	void		Drop( const Vector &vecVelocity );


	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif

	CWeaponKungFu( const CWeaponKungFu & );

private:
		
};
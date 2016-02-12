//=============================================================================//
// Purpose:	Distraction Half-Life 2 AK-47
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponAK47 C_WeaponAK47
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponAK47 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponAK47, CHL2MPMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponAK47();
	void PrimaryAttack( void )
	{
		BaseClass::PrimaryAttack();
		#ifndef CLIENT_DLL
			SignalShellEject();
		#endif
	}
	void	SecondaryAttack( void ) { m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f; }
	bool	Reload( void );
	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }
	int GetShellType( void ) { return 1; } //-1 = none, 0 = pistol, 1 = rifle, 2 = shotgun

	DECLARE_ACTTABLE();
	
private:
	CWeaponAK47( const CWeaponAK47 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAK47, DT_WeaponAK47 )

BEGIN_NETWORK_TABLE( CWeaponAK47, DT_WeaponAK47 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAK47 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_ak47, CWeaponAK47 );
PRECACHE_WEAPON_REGISTER(weapon_ak47);

acttable_t	CWeaponAK47::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SMG1,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SMG1,				false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_SMG1,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SMG1,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_SMG1,					false },
};
IMPLEMENT_ACTTABLE(CWeaponAK47);

CWeaponAK47::CWeaponAK47( )
{
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	m_iFireInputType = FIREINPUTTYPE_CANHOLD;
}

bool CWeaponAK47::Reload( void )
{
	bool fRet;
	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
		WeaponSound( RELOAD );
	return fRet;
}

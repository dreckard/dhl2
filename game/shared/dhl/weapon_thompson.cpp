//=============================================================================//
// Purpose:	Distraction Half-Life 2 Thompson
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponThompson C_WeaponThompson
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponThompson : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponThompson, CHL2MPMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponThompson();
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

	DECLARE_ACTTABLE();
	
private:
	CWeaponThompson( const CWeaponThompson & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponThompson, DT_WeaponThompson )

BEGIN_NETWORK_TABLE( CWeaponThompson, DT_WeaponThompson )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponThompson )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_thompson, CWeaponThompson );
PRECACHE_WEAPON_REGISTER(weapon_thompson);

acttable_t	CWeaponThompson::m_acttable[] = 
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
IMPLEMENT_ACTTABLE(CWeaponThompson);

CWeaponThompson::CWeaponThompson( )
{
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	m_iFireInputType = FIREINPUTTYPE_CANHOLD;
}

bool CWeaponThompson::Reload( void )
{
	bool fRet;
	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
		WeaponSound( RELOAD );
	return fRet;
}

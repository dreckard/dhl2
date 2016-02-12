//=============================================================================//
// Purpose:	Distraction Half-Life 2 Mac-11
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponMac11 C_WeaponMac11
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponMac11 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponMac11, CHL2MPMachineGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponMac11();
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
	CWeaponMac11( const CWeaponMac11 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMac11, DT_WeaponMac11 )

BEGIN_NETWORK_TABLE( CWeaponMac11, DT_WeaponMac11 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMac11 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mac11, CWeaponMac11 );
PRECACHE_WEAPON_REGISTER(weapon_mac11);

acttable_t	CWeaponMac11::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,					false },
};
IMPLEMENT_ACTTABLE(CWeaponMac11);

CWeaponMac11::CWeaponMac11( )
{
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	m_iFireInputType = FIREINPUTTYPE_CANHOLD;
}

bool CWeaponMac11::Reload( void )
{
	bool fRet;
	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
		WeaponSound( RELOAD );
	return fRet;
}

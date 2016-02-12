//=============================================================================//
// Purpose:	Distraction Half-Life 2 Desert Eagle
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponDeagle C_WeaponDeagle
#endif

//-----------------------------------------------------------------------------
// CWeaponDeagle
//-----------------------------------------------------------------------------

class CWeaponDeagle : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponDeagle, CBaseHL2MPCombatWeapon );

	CWeaponDeagle(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	DryFire( void );

	void PrimaryAttack( void )
	{
		BaseClass::PrimaryAttack();
		#ifndef CLIENT_DLL
			SignalShellEject();
		#endif
	}
	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	virtual bool Reload( void );
	
	DECLARE_ACTTABLE();

private:
	CWeaponDeagle( const CWeaponDeagle & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDeagle, DT_WeaponDeagle )

BEGIN_NETWORK_TABLE( CWeaponDeagle, DT_WeaponDeagle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDeagle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );

acttable_t CWeaponDeagle::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponDeagle );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeagle::CWeaponDeagle( void )
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = DHL_INV_VAL_HANDGUN;
}

void CWeaponDeagle::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponDeagle::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
	}
}

bool CWeaponDeagle::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
	}
	return fRet;
}
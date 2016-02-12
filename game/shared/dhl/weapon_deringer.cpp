//=============================================================================//
// Purpose:	Distraction Half-Life 2 Deringer
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponDeringer C_WeaponDeringer
#endif

//-----------------------------------------------------------------------------
// CWeaponDeringer
//-----------------------------------------------------------------------------

class CWeaponDeringer : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponDeringer, CBaseHL2MPCombatWeapon );

	CWeaponDeringer(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	DryFire( void );

	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	virtual bool Reload( void );
	
	DECLARE_ACTTABLE();

	CNetworkVar( int, m_iDHLv12 ); //This is a dummy var to force client/server version match

private:
	CWeaponDeringer( const CWeaponDeringer & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDeringer, DT_WeaponDeringer )

BEGIN_NETWORK_TABLE( CWeaponDeringer, DT_WeaponDeringer )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iDHLv12 ) ),
#else
	SendPropInt( SENDINFO( m_iDHLv12 ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDeringer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_deringer, CWeaponDeringer );
PRECACHE_WEAPON_REGISTER( weapon_deringer );

acttable_t CWeaponDeringer::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponDeringer );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeringer::CWeaponDeringer( void )
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = 0;
}

void CWeaponDeringer::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponDeringer::ItemPostFrame( void )
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

bool CWeaponDeringer::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
	}
	return fRet;
}
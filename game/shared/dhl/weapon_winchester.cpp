//=============================================================================//
// Purpose:	Distraction Half-Life 2 Winchester
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponWinchester C_WeaponWinchester
#endif

//This sucks. Time between bullets being loaded, should probably be SequenceDuration() but we don't actually have a load animation nor do I particularly want one
//#define RELOAD_INTERVAL 0.5f

//-----------------------------------------------------------------------------
// CWeaponWinchester
//-----------------------------------------------------------------------------
class CWeaponWinchester : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponWinchester, CBaseHL2MPCombatWeapon );

	CWeaponWinchester(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	Cock( void );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	DryFire( void );

	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	bool Reload( void );
	void FinishReload( void );

	DECLARE_ACTTABLE();

private:
	CWeaponWinchester( const CWeaponWinchester & );
	CNetworkVar( bool, m_bCocked );
	bool m_bFinishingReload;
	bool m_bChamberEmpty;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponWinchester, DT_WeaponWinchester )

BEGIN_NETWORK_TABLE( CWeaponWinchester, DT_WeaponWinchester )
#ifndef CLIENT_DLL
	SendPropBool(SENDINFO(m_bCocked) ),
#else
	RecvPropBool(RECVINFO(m_bCocked) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponWinchester )
	DEFINE_PRED_FIELD( m_bCocked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_bFinishingReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bChamberEmpty, FIELD_BOOLEAN ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_winchester, CWeaponWinchester );
PRECACHE_WEAPON_REGISTER( weapon_winchester );

acttable_t CWeaponWinchester::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponWinchester );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponWinchester::CWeaponWinchester( void )
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	//m_bReloadsSingly = true;
	m_bCocked = true;
	m_bFinishingReload = false;
	m_bChamberEmpty = false;
}

void CWeaponWinchester::Cock( void )
{
	WeaponSound( SPECIAL1 ); //Cocking sound.
	if ( m_bChamberEmpty )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		m_bChamberEmpty = false;
	}
	else
		SendWeaponAnim( ACT_VM_DHL_COCK );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_bCocked = true;

	#ifndef CLIENT_DLL
		SignalShellEject();
	#endif
}

void CWeaponWinchester::DryFire( void )
{
	if ( !m_bCocked ) 
	{
		Cock();
		return;
	}

	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponWinchester::PrimaryAttack( void )
{
	if ( !m_bCocked )
	{
		Cock();
		return;
	}
	BaseClass::PrimaryAttack();
	m_bCocked = false;
}

void CWeaponWinchester::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;
	if ( m_bInReload ) //Handle reload
	{
		//Abort if they're pressing attack, finish normally if they run out of ammo or fill the magazine
		if ( pOwner->m_nButtons & IN_ATTACK || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		{
			if ( !m_bFinishingReload )
			{
				FinishReload();
				return;
			}
		}
		if ( !m_bFinishingReload && m_flNextPrimaryAttack <= gpGlobals->curtime )
		{
			if ( m_iClip1 >= GetMaxClip1() )
			{
				FinishReload();
				return;
			}

#ifdef CLIENT_DLL
			if ( input->CAM_IsThirdPerson() )
				WeaponSound( RELOAD );
#else
			WeaponSound( RELOAD ); //Should be audible for other players...local player does it through vm anim event
#endif
			SendWeaponAnim( ACT_VM_RELOAD );
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
			m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		}

		if ( m_bFinishingReload )
		{
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( !m_bCocked )
					Cock();
				m_bFinishingReload = false;
				m_bInReload = false;
			}
		}
		return;
	}
	BaseClass::ItemPostFrame();

	if ( pOwner->m_nButtons & IN_ATTACK2 )
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
		DryFire();
}

bool CWeaponWinchester::Reload( void )
{
	//bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	if ( m_iClip1 == GetMaxClip1() )
		return false;

	if ( m_iClip1 == 0 ) //Have to operate the action to chamber a round
	{
		m_bCocked = false;
		m_bChamberEmpty = true;
	}

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;
	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	m_bInReload = true;
	SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

void CWeaponWinchester::FinishReload( void )
{
	//BaseClass::FinishReload();
	SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_bFinishingReload = true;
	//Cock();
}
//=============================================================================//
// Purpose:	Distraction Half-Life 2 Mosin Nagant
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "dhl/dhl_player_inc.h"

#ifdef CLIENT_DLL
#include "input.h"
#define CWeaponMosinNagant C_WeaponMosinNagant
#endif 

//-----------------------------------------------------------------------------
// CWeaponMosinNagant
//-----------------------------------------------------------------------------
class CWeaponMosinNagant : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponMosinNagant, CBaseHL2MPCombatWeapon );

	CWeaponMosinNagant();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ToggleZoom( void );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	ModAccuracy( Vector& accuracy );
	void	Cock( void );
	void	DryFire( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	void	Drop( const Vector &vecVelocity );

	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	virtual bool Reload( void );
	void FinishReload( void );
	int GetShellType( void ) { return 1; } //-1 = none, 0 = pistol, 1 = rifle, 2 = shotgun
		
	DECLARE_ACTTABLE();

private:
	CWeaponMosinNagant( const CWeaponMosinNagant & );
	bool m_bWantsThirdPerson;
	CNetworkVar( bool, m_bInZoom );
	CNetworkVar( bool, m_bCocked );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMosinNagant, DT_WeaponMosinNagant )

BEGIN_NETWORK_TABLE( CWeaponMosinNagant, DT_WeaponMosinNagant )
	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_bCocked ) ),
		RecvPropBool( RECVINFO( m_bInZoom ) ),
	#else
		SendPropBool( SENDINFO( m_bCocked ) ),
		SendPropBool( SENDINFO( m_bInZoom ) ),
	#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMosinNagant )
	DEFINE_PRED_FIELD( m_bCocked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_mosinnagant, CWeaponMosinNagant );
PRECACHE_WEAPON_REGISTER( weapon_mosinnagant );

acttable_t CWeaponMosinNagant::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponMosinNagant );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMosinNagant::CWeaponMosinNagant()
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	m_bInZoom = false;
	m_bCocked = true;
	m_bWantsThirdPerson = false;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle Zooming (scope)
//-----------------------------------------------------------------------------
void CWeaponMosinNagant::ToggleZoom( void )
{
	CDHL_Player *pPlayer = ToDHLPlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

#ifdef CLIENT_DLL
	if ( !m_bInZoom )
	{
		if ( ::input->CAM_IsThirdPerson() )
		{
			::input->CAM_ToFirstPerson();
			m_bWantsThirdPerson = true;
		}
		else
			m_bWantsThirdPerson = false;
		m_bInZoom = true;
	}
	else
	{
		if ( m_bWantsThirdPerson )
			::input->CAM_ToThirdPerson();
		m_bInZoom = false;
	}
#endif

#ifndef CLIENT_DLL
	if ( m_bInZoom )
	{
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
			pPlayer->m_bScoped = false;
			pPlayer->ShowCrosshair( true );
		}
	}
	else
	{
		if ( pPlayer->SetFOV( this, 20, 0.1f ) )
		{
			m_bInZoom = true;
			pPlayer->m_bScoped = true;
			pPlayer->ShowCrosshair( false );
		}
	}
#endif
}

void CWeaponMosinNagant::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

void CWeaponMosinNagant::PrimaryAttack( void )
{
	if ( m_bCocked )
	{
		BaseClass::PrimaryAttack();
		m_bCocked = false;
	}
	else if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
		Cock();
}

void CWeaponMosinNagant::SecondaryAttack( void )
{
	ToggleZoom();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}

void CWeaponMosinNagant::ModAccuracy( Vector& accuracy )
{
	if ( !m_bInZoom ) //Accuracy much worse if not scoped
		accuracy *= 10;
	BaseClass::ModAccuracy( accuracy );
}

void CWeaponMosinNagant::Cock( void )
{
	WeaponSound( SPECIAL1 );
	SendWeaponAnim( ACT_SHOTGUN_PUMP );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_bCocked = true;
#ifndef CLIENT_DLL
	SignalShellEject();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponMosinNagant::ItemPostFrame( void )
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

#ifdef CLIENT_DLL
	if ( m_bInZoom ) //Can't let them switch to third person
	{
		if ( ::input->CAM_IsThirdPerson() )
		{
			::input->CAM_ToFirstPerson();
			m_bWantsThirdPerson = !m_bWantsThirdPerson;
		}
	}
#endif
}

bool CWeaponMosinNagant::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
		if ( m_bInZoom )
		{
			ToggleZoom(); //DHL: Leave zoom at reload
		}
	}
	return fRet;
}

void CWeaponMosinNagant::FinishReload( void )
{
	m_bCocked = true;
	BaseClass::FinishReload();
}

bool CWeaponMosinNagant::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bInZoom )
	{
		ToggleZoom();
	}
	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponMosinNagant::Drop( const Vector &vecVelocity )
{
	if ( m_bInZoom )
		ToggleZoom();
	m_bWantsThirdPerson = false;
	BaseClass::Drop( vecVelocity );
}
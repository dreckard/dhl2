//=============================================================================//
// Purpose:	Distraction Half-Life 2 Single Action Army
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSAA C_WeaponSAA
#endif 

//-----------------------------------------------------------------------------
// CWeaponSAA
//-----------------------------------------------------------------------------

class CWeaponSAA : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponSAA, CBaseHL2MPCombatWeapon );

	CWeaponSAA(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	Cock( void ); //DHL
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void ); //DHL
	void	DryFire( void );

	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	bool Reload( void );
	void FinishReload( void );
	
	DECLARE_ACTTABLE();

private:
	CWeaponSAA( const CWeaponSAA & );
	CNetworkVar( bool, m_bCocked );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSAA, DT_WeaponSAA )

BEGIN_NETWORK_TABLE( CWeaponSAA, DT_WeaponSAA )
#ifndef CLIENT_DLL
	SendPropBool(SENDINFO(m_bCocked) ),
#else
	RecvPropBool(RECVINFO(m_bCocked) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSAA )
	DEFINE_PRED_FIELD( m_bCocked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_SAA, CWeaponSAA );
PRECACHE_WEAPON_REGISTER( weapon_SAA );

acttable_t CWeaponSAA::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponSAA );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSAA::CWeaponSAA( void )
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = DHL_INV_VAL_HANDGUN;
	m_bCocked = true;
	m_bReloadsSingly = true;
}

void CWeaponSAA::Cock( void ) //DHL
{
	if ( m_bCocked )
		return;
	WeaponSound( SPECIAL1 ); //Cocking sound.
	//SendWeaponAnim( ACT_SAA_COCK ); //DHL
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.25f; //Hack, should eventually be SequenceDuration()
	m_bCocked = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSAA::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSAA::PrimaryAttack( void )
{
	if ( m_bCocked == false )
		return;
	BaseClass::PrimaryAttack();
	m_bCocked = false;
}

void CWeaponSAA::SecondaryAttack( void )
{
	Cock();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
}

void CWeaponSAA::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK2 )
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
		DryFire();
}

bool CWeaponSAA::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
		WeaponSound( RELOAD );
	return fRet;
}

void CWeaponSAA::FinishReload( void )
{
	BaseClass::FinishReload();
	Cock();
}
//=============================================================================//
// Purpose:	Distraction Half-Life 2 Sawed Off Double Barrel Shotgun
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSawedOff C_WeaponSawedOff
#endif 

//-----------------------------------------------------------------------------
// CWeaponSawedOff
//-----------------------------------------------------------------------------

class CWeaponSawedOff : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponSawedOff, CBaseHL2MPCombatWeapon );

	CWeaponSawedOff(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DryFire( void );
	bool	Reload( void );
	Activity	GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; }

	unsigned char GetProjectileType( void ) { return DHL_PROJECTILE_TYPE_PELLET; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponSawedOff( const CWeaponSawedOff & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSawedOff, DT_WeaponSawedOff )

BEGIN_NETWORK_TABLE( CWeaponSawedOff, DT_WeaponSawedOff )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSawedOff )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sawedoff, CWeaponSawedOff );
PRECACHE_WEAPON_REGISTER( weapon_sawedoff );

acttable_t CWeaponSawedOff::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponSawedOff );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSawedOff::CWeaponSawedOff( void )
{
	m_bFiresUnderwater	= false;
	m_iInventoryValue = DHL_INV_VAL_UNIQUE;
	m_iFireInputType = FIREINPUTTYPE_CANHOLD;
}

void CWeaponSawedOff::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

void CWeaponSawedOff::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	WeaponSound(SINGLE);

	pOwner->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	//Never unless they release their fire key
	m_flNextPrimaryAttack = gpGlobals->curtime + FLT_MAX;

	m_iClip1 -= 1;

	// player "shoot" animation
	pOwner->SetAnimation( PLAYER_ATTACK1 );

	Vector	vecSrc		= pOwner->Weapon_ShootPosition( );
	Vector	vecAiming	= pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	FireBulletsInfo_t info( 7, vecSrc, vecAiming, CDHLBaseWeapon::GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pOwner;

	pOwner->FireBullets( info );
	
	AddDHLViewKick( BaseClass::GetRecoilX(), BaseClass::GetRecoilY() );
}

//Fire both barrels
void CWeaponSawedOff::SecondaryAttack( void )
{
	if ( m_iClip1 < 2 )
	{
		if ( m_iClip1 == 1 )
			PrimaryAttack();
		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	WeaponSound(WPN_DOUBLE);
	pOwner->DoMuzzleFlash();
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	m_iClip1 -= 2;
	pOwner->SetAnimation( PLAYER_ATTACK1 );

	Vector	vecSrc		= pOwner->Weapon_ShootPosition( );
	Vector	vecAiming	= pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	FireBulletsInfo_t info( 14, vecSrc, vecAiming, CDHLBaseWeapon::GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pOwner;

	pOwner->FireBullets( info );

	AddDHLViewKick( BaseClass::GetRecoilX(), BaseClass::GetRecoilY() );
}

void CWeaponSawedOff::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Player must release the fire key to fire again
	if ( (( pOwner->m_nButtons & IN_ATTACK ) == false ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime;
	} 
}

bool CWeaponSawedOff::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
	}
	return fRet;
}
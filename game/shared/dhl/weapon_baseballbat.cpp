//=============================================================================//
// Purpose:	Distraction Half-Life 2 Baseball Bat
//
// Author: Skillet
//=============================================================================//

#include "cbase.h"
#include "dhl/dhl_basemeleeweapon.h"
#include "vstdlib/random.h"
#include "npcevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponBaseballBat C_WeaponBaseballBat
#endif

//-----------------------------------------------------------------------------
// CWeaponBaseballBat
//-----------------------------------------------------------------------------

class CWeaponBaseballBat : public CDHLBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CWeaponBaseballBat, CDHLBaseMeleeWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CWeaponBaseballBat();

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	virtual void PrimaryAttack( void );

	CWeaponBaseballBat( const CWeaponBaseballBat & );
};

//-----------------------------------------------------------------------------
// CWeaponBaseballBat
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponBaseballBat, DT_WeaponBaseballBat )

BEGIN_NETWORK_TABLE( CWeaponBaseballBat, DT_WeaponBaseballBat )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponBaseballBat )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_baseballbat, CWeaponBaseballBat );
PRECACHE_WEAPON_REGISTER( weapon_baseballbat );

acttable_t	CWeaponBaseballBat::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_MELEE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_MELEE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_MELEE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponBaseballBat);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponBaseballBat::CWeaponBaseballBat( void )
{
	m_iInventoryValue = DHL_INV_VAL_ITEM;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponBaseballBat::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponBaseballBat::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif //!CLIENT_DLL

void CWeaponBaseballBat::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();

	//HACK - our model crashes the decompiler and all the original files are lost so we can't fix the messed up activities
	SendWeaponAnim( ACT_VM_HITCENTER );

	CHL2MP_Player *pOwner = ToHL2MPPlayer( GetOwner() );
	if ( !pOwner )
		return;

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

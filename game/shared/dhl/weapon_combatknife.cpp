//=============================================================================//
// Purpose:	Distraction Half-Life 2 Combat Knife
//
// Author: Skillet
//=============================================================================//

#include "cbase.h"
#include "dhl/dhl_basemeleeweapon.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#ifndef CLIENT_DLL
	#include "dhl/dhl_projectile.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponCombatKnife C_WeaponCombatKnife
#endif

//-----------------------------------------------------------------------------
// CWeaponCombatKnife
//-----------------------------------------------------------------------------

class CWeaponCombatKnife : public CDHLBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CWeaponCombatKnife, CDHLBaseMeleeWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CWeaponCombatKnife();
	unsigned char GetProjectileType( void ) { return DHL_PROJECTILE_TYPE_COMBATKNIFE; }
	bool DropsAmmoIndividually( void ) { return true; }
	bool AllowBumpAmmoPickup( void ) { return false; }
	void SecondaryAttack( void );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	CWeaponCombatKnife( const CWeaponCombatKnife & );
};

//-----------------------------------------------------------------------------
// CWeaponCombatKnife
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCombatKnife, DT_WeaponCombatKnife )

BEGIN_NETWORK_TABLE( CWeaponCombatKnife, DT_WeaponCombatKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCombatKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_combatknife, CWeaponCombatKnife );
PRECACHE_WEAPON_REGISTER( weapon_combatknife );

acttable_t	CWeaponCombatKnife::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponCombatKnife);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCombatKnife::CWeaponCombatKnife( void )
{
	m_iInventoryValue = DHL_INV_VAL_ITEM;
}

//Secondary attack - throw it
ConVar dhl_flamingknives( "dhl_flamingknives", "0", FCVAR_CHEAT | FCVAR_NOTIFY );
void CWeaponCombatKnife::SecondaryAttack( void )
{
	CBasePlayer* pOwner = ToBasePlayer( GetOwner() );
	Assert( pOwner );

	//Can't throw last knife
	if ( pOwner->GetAmmoCount(GetPrimaryAmmoType()) <= 0 )
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
		return;
	}

	#ifndef CLIENT_DLL
		CDHLProjectile* pKnife = (CDHLProjectile*)(CreateEntityByName( "dhl_projectile" ));
		Assert( pKnife );
		pKnife->SetOwnerEntity( pOwner );
		pKnife->Spawn();

		//This is just an easy way of getting an eye vector, there isn't really any autoaim in MP
		Vector vecDir = pOwner->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
		//HACK - move it forward so it doesn't appear to spawn inside the player's face
		Vector vecSrc = pOwner->Weapon_ShootPosition() + ( vecDir * 6.0f );

		pKnife->Fire( vecSrc, vecDir * 1232.0f /*70mph*/, GetHL2MPWpnData().m_iPlayerDamage, this, pOwner, -1 );
		
		if ( dhl_flamingknives.GetBool() )
			pKnife->Ignite( 30.0f, false );
		
	#endif

		//Make sure this is done after the call to Fire()
		int iAmmo = pOwner->GetAmmoCount(GetPrimaryAmmoType());
		pOwner->RemoveAmmo( 1, GetPrimaryAmmoType() );
		if ( iAmmo <= 0 )
		{
			AddEffects( EF_NODRAW );
			#ifndef CLIENT_DLL
				pOwner->Weapon_Drop( this, NULL, NULL );
				Remove();
			#endif
		}
		else
		{
			SendWeaponAnim( ACT_VM_THROW ); //Need to be able to predict this

			pOwner->SetAnimation( PLAYER_ATTACK1 ); //Use the primary attack anim for now
			ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

			m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
		}

		/*IPhysicsObject* pPhysObj = VPhysicsGetObject();
		if ( pPhysObj )
		{
			pPhysObj->Sleep();
			pPhysObj->EnableMotion( false );
			pPhysObj->EnableCollisions( false );
		}
		m_bAllowPickup = false;*/
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponCombatKnife::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
void CWeaponCombatKnife::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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

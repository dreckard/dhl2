//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "grenade_frag.h"
	#include "hierarchy.h" //DHL
#endif

//DHL
#include "takedamageinfo.h"
#include "dhl/dhl_gamerules.h"
#include "dhl/dhl_shareddefs.h"

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//DHL NOTE! No longer used, time is now randomized in the attack functions
#define GRENADE_TIMER	3.0f //Seconds

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	10.0f // inches

#define GRENADE_DAMAGE_RADIUS 250.0f

#ifdef CLIENT_DLL
#define CWeaponFrag C_WeaponFrag
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag: public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponFrag, CBaseHL2MPCombatWeapon );
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponFrag();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	bool	Reload( void );

	virtual void Drop( const Vector &vecVelocity );

	//DHL - Skillet
	float flExplodeTime;
	float flPrimeTime;
	float flGrenadeTimer;

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	void	ThrowGrenade( CBasePlayer *pPlayer, bool Invis = false ); //DHL added Invis arg for explosions 'in hand'
	bool	IsPrimed( bool ) { return ( m_AttackPaused != 0 );	}
	
private:

	void	RollGrenade( CBasePlayer *pPlayer );
	void	LobGrenade( CBasePlayer *pPlayer );
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool,	m_bRedraw );	//Draw the weapon again after throwing a grenade
	
	CNetworkVar( int,	m_AttackPaused );
	CNetworkVar( bool,	m_fDrawbackFinished );

	CWeaponFrag( const CWeaponFrag & );
	DECLARE_ACTTABLE();
};
acttable_t	CWeaponFrag::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_GRENADE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_GRENADE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFrag, DT_WeaponFrag )

BEGIN_NETWORK_TABLE( CWeaponFrag, DT_WeaponFrag )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropBool( RECVINFO( m_fDrawbackFinished ) ),
	RecvPropInt( RECVINFO( m_AttackPaused ) ),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropBool( SENDINFO( m_fDrawbackFinished ) ),
	SendPropInt( SENDINFO( m_AttackPaused ) ),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponFrag )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_frag, CWeaponFrag );
PRECACHE_WEAPON_REGISTER(weapon_frag);

CWeaponFrag::CWeaponFrag( void ) :
	CBaseHL2MPCombatWeapon()
{
	m_bRedraw = false;
	//DHL - Skillet
	flExplodeTime = 0.0f;
	flPrimeTime = 0.0f;
	flGrenadeTimer = 0.0f;
	m_iInventoryValue = DHL_INV_VAL_ITEM;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "npc_grenade_frag" );
#endif

	PrecacheScriptSound( "WeaponFrag.Throw" );
	PrecacheScriptSound( "WeaponFrag.Roll" );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewGrenade = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_fDrawbackFinished = true;
			break;

		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW2:
			RollGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW3:
			LobGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewGrenade )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack( void )
{
	//DHL - Skillet - Disabled secondary attack
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
	return;

	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;
	
	flPrimeTime = gpGlobals->curtime; //DHL: Grab the time we dropped the trigger
	flExplodeTime = gpGlobals->curtime + GRENADE_TIMER; //DHL: We're going boom when this hits curtime

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	//DHL - Scale for slow motion
	float flTimescale = DHLRules()->GetTimescale();

	flGrenadeTimer = random->RandomFloat( 3.0f, 3.5f ) / flTimescale; //Randomize the fuse burn time
	flPrimeTime = gpGlobals->curtime / flTimescale; //DHL: Grab the time we dropped the trigger
	flExplodeTime = gpGlobals->curtime + flGrenadeTimer; //DHL: We're going boom when this hits curtime
	//Warning( "Randomized fuse time is " 'flGrenadeTimer' "! \n");

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	
	//DHL - Skillet - Once we throw our last grenade remove weapon_grenade from our inventory
#ifndef CLIENT_DLL
	if ( !HasPrimaryAmmo() /*pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0*/ )
	{
		if ( GetLastWeapon() )
			pOwner->SwitchToNextBestWeapon( this );
		Drop( vec3_origin );
		SUB_Remove();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame( void )
{
	if( m_fDrawbackFinished )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

		if (pOwner)
		{
			switch( m_AttackPaused )
			{
			case GRENADE_PAUSED_PRIMARY:
				if( !(pOwner->m_nButtons & IN_ATTACK) )
				{
					SendWeaponAnim( ACT_VM_THROW );
					ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

					//Tony; fire the sequence
					m_fDrawbackFinished = false;
				}
				//We are still holding attack...
				else
				{
					if ( flExplodeTime <= gpGlobals->curtime )
					{
						//DHL: Skip the animations and go boom immediately if we've been holding it too long
						ThrowGrenade( pOwner, true ); //Make it invisible as to maintain the illusion of going off in the player's hand
						DecrementAmmo( pOwner );
	
						//Make absolutely certain that if the grenade goes off in a player's hand, they die.
						#ifndef CLIENT_DLL
						CTakeDamageInfo GrenadeInfo( this, pOwner, 100.0f, DMG_BURN, 0 );
						pOwner->TakeDamage( GrenadeInfo );
						#endif
	
					}
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK2) )
				{
					//See if we're ducking
					if ( pOwner->m_nButtons & IN_DUCK )
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_HAULBACK );
					}
					//Tony; the grenade really should have a secondary anim. but it doesn't on the player.
					ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

					m_fDrawbackFinished = false;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade )
{
	CWeaponFrag *pWeaponFrag = dynamic_cast<CWeaponFrag*>( pGrenade );

	if ( pWeaponFrag )
	{
		pWeaponFrag->ThrowGrenade( pPlayer );
		pWeaponFrag->DecrementAmmo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade( CBasePlayer *pPlayer, bool Invis /*= false*/ )
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	//DHL - We're just gonna set the grenade off at vecSrc if it goes off "in their hand", so don't worry about velocity
	if ( !Invis )
		vecThrow += vForward * 1200;
	//DHL: Added conditional to last arg, to allow priming...
	//If we haven't hit the time we should explode yet, use the remaining time as the clock for the grenade
	//Otherwise (we're due to explode) throw (will be forced in the PostFrame function) and go off near immediately
	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, ( (flExplodeTime > gpGlobals->curtime) ? (flExplodeTime - gpGlobals->curtime) : 0.01f ), false );

	if ( pGrenade )
	{
		if ( pPlayer && pPlayer->m_lifeState != LIFE_ALIVE )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
			}
		}
		
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
		if ( Invis ) //DHL - Don't always want to be able to see the grenade
		{
			pGrenade->AddEffects( EF_NODRAW );
			pGrenade->SetMoveType( MOVETYPE_NONE );
			
			CUtlVector<CBaseEntity *> childrenList;
			GetAllChildren( pGrenade, childrenList );
			if ( childrenList.Count() ) // If there's any children in the list...
			{
				for ( int i = childrenList.Count()-1; i >= 0; --i )
				{
					UTIL_Remove( childrenList[i] ); //Remove them all
				}
			}
			pGrenade->SetAbsVelocity( vec3_origin );
			pGrenade->SetAbsOrigin( vecSrc ); //Put the grenade right back in their hand incase it's moved
			pGrenade->Detonate();
		}
	}
#endif

	m_bRedraw = true;

	WeaponSound( SINGLE );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::LobGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 350 + Vector( 0, 0, 50 );
	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0), pPlayer, GRENADE_TIMER, false );

	if ( pGrenade )
	{
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
	}
#endif

	WeaponSound( WPN_DOUBLE );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::RollGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, GRENADE_TIMER, false );

	if ( pGrenade )
	{
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
	}

#endif

	WeaponSound( SPECIAL1 );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

//DHL - Skillet - Fix ammo exploits
void CWeaponFrag::Drop( const Vector &vecVelocity )
{
	if ( !HasPrimaryAmmo() )
	{
		//BaseClass::Drop( vecVelocity );
		SUB_Remove();
		return;
	}

	if ( !GetOwner() )
		return;

	DecrementAmmo( GetOwner() );
	Reload(); //Do draw animation and stuff
	
#ifndef CLIENT_DLL
	CBasePlayer *owner = ToBasePlayer(GetOwner());

	Vector vThrowPos = owner->Weapon_ShootPosition() - Vector(0,0,12);
	//Create a grenade
	CBaseCombatWeapon* pGrenade;
	pGrenade = (CBaseCombatWeapon *)CBaseEntity::Create( "weapon_frag", vThrowPos, vec3_angle, NULL );
	if ( !pGrenade )
		return;

	pGrenade->SetRemoveable( true );

	//If it was dropped then there's no need to respawn it.
	pGrenade->AddSpawnFlags( SF_NORESPAWN );

	pGrenade->StopAnimation();
	pGrenade->StopFollowingEntity( );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY );
	// clear follow stuff, setup for collision
	pGrenade->SetGravity(1.0);
	pGrenade->m_iState = WEAPON_NOT_CARRIED;
	pGrenade->RemoveEffects( EF_NODRAW );
	pGrenade->FallInit();
	pGrenade->SetGroundEntity( NULL );
	pGrenade->SetTouch(NULL);

	pGrenade->SetOwnerEntity( NULL );
	pGrenade->SetOwner( NULL );

	//Toss it in the direction of the player's view
	Vector vecNewVelocity;
	Vector vecDir;
	owner->EyeVectors( &vecDir );
	vecNewVelocity = ( vecDir * 500.0f );

	IPhysicsObject *pObj = pGrenade->VPhysicsGetObject();
	if ( pObj != NULL )
	{
		AngularImpulse angImp( 200, 200, 200 );
		pObj->AddVelocity( &vecNewVelocity, &angImp );
	}
#endif
}



//=============================================================================//
// Purpose:	Distraction Half-Life 2 projectile entity
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl_gamerules.h"
#include "takedamageinfo.h"
#ifdef CLIENT_DLL
	#include "prediction.h"
	#include "dhl/c_dhl_projectile.h"
	#include "clientsideeffects.h"
	#include "fx_impact.h"
#else
	#include "dhl/dhl_projectile.h"
#endif

#include "dhl_baseweapon.h"
#include "dhl_shareddefs.h"
#include "dhl_player_inc.h"
#include "dhl/dhl_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
extern CBaseEntity* DHL_FX_AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType, CUtlVector<CBaseEntity*>* pSkipList );
#define CDHLProjectile C_DHLProjectile
#endif

//const unsigned char BULLET_TYPE_GENERIC = 0;
//const unsigned char BULLET_TYPE_PELLET = 1;

const unsigned char MSG_NOTIFY_REMOVAL = 0;
const char* PelletModel = "models/shotgun_pellet/shotgun_pellet.mdl";
const char* BulletModel = "models/bullet/bullet.mdl";
const char* TrailModel = "models/bullet/bullet_trail.mdl";
const char* CombatKnifeModel = "models/weapons/combatknife/w_combatknife.mdl";

void CDHLProjectile::Precache( void )
{
	PrecacheModel( BulletModel );
	PrecacheModel( PelletModel );
	PrecacheModel( TrailModel );
	PrecacheModel( CombatKnifeModel );
}

void CDHLProjectile::Spawn( void )
{
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
	//SetModel( szModel );

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE ); //Not solid..we're going to do our own movement and simulation with tracelines each frame :)
}

#ifdef CLIENT_DLL
void CDHLProjectile::UpdateModel( void )
{
	const char* szModel = BulletModel;
	switch ( m_iType )
	{
		case DHL_PROJECTILE_TYPE_BULLET:
			szModel = BulletModel;
			break;
		case DHL_PROJECTILE_TYPE_PELLET:
			szModel = PelletModel;
			break;
		case DHL_PROJECTILE_TYPE_COMBATKNIFE:
			szModel = CombatKnifeModel;
			break;
		default:
			return;
			break;
	}
	SetModel( szModel );
}
#endif

//Handle all necessary movement stuffs
void CDHLProjectile::MoveProjectileToPosition( Vector& vecPos )
{
	#ifdef CLIENT_DLL
		SetLocalOrigin( vecPos );

		//This is VERY important - C_BaseEntity::PostDataUpdate calls MoveToLastReceivedPosition which forces the origin and angles to their "network" values
		//Result is that if we don't set them here there will be painful hitching in real world situations (with lag).
		SetNetworkOrigin( vecPos );
		if ( m_pTrail )
			m_pTrail->SetAbsOrigin( vecPos );
		//m_pTrail->m_vecPosition = vecPos;
	#else
		m_vecCurPosition = vecPos;
	#endif
}

ConVar dhl_bulletspeed( "dhl_bulletspeed", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_NOTIFY, "Speed scalar for slow motion bullets \n" );
ConVar dhl_knifespeed( "dhl_knifespeed", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_NOTIFY, "Speed scalar for throwing knives \n" );
void CDHLProjectile::PhysicsSimulate( void )
{
	//-------------------------------------------------------------------------------
	//Our own movement/physics simulation!
	//-------------------------------------------------------------------------------
	#ifdef CLIENT_DLL
		if ( m_bCollided )
			return;

		if ( !m_pShooter && m_hShooter )
			m_pShooter = m_hShooter.Get();
	#else
		if ( m_flRemoveAt > 0.0f )
		{
			if ( m_flRemoveAt < gpGlobals->curtime )
			{
				m_flRemoveAt = 0.0f;
				SUB_Remove();
			}
			return;
		}
		if ( IsMarkedForDeletion() )
			return;
	#endif

	float flFrametime = gpGlobals->frametime;
	//Scale for slow motion
	if ( DHLRules() )
	{
		if ( (m_iType == DHL_PROJECTILE_TYPE_BULLET || m_iType == DHL_PROJECTILE_TYPE_PELLET) )
			flFrametime *= (dhl_bulletspeed.GetFloat() * DHLRules()->GetTimescale());
		else if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
			flFrametime *= (dhl_knifespeed.GetFloat() * DHLRules()->GetTimescale());
		else
			flFrametime *= DHLRules()->GetTimescale();
	}

	Vector vecDir = vec3_origin;
#ifndef CLIENT_DLL
	Vector vecStartPos = m_vecCurPosition; //This is where we are
	Vector vecEndPos = m_vecCurPosition; //This is where we're going
	Vector vecVelocity = m_vecCurVelocity; //Velocity
#else
	Vector vecStartPos = GetLocalOrigin(); //This is where we are
	Vector vecEndPos = GetLocalOrigin(); //This is where we're going
	Vector vecVelocity = GetLocalVelocity(); //Velocity
#endif
	//Find out where we should move to
	if ( vecVelocity != vec3_origin )
	{
		static ConVarRef gravVar( "sv_gravity" );
		//Gravity
		float newZVelocity = vecVelocity.z - ( flFrametime * gravVar.GetFloat() * GetGravity() );
		vecVelocity.z = ( (vecVelocity.z + newZVelocity) / 2 );

		vecDir = vecVelocity;
		VectorNormalize( vecDir );

		//Gravity needs to be cumulative
		#ifndef CLIENT_DLL
			m_vecCurVelocity = vecVelocity;
		#else
			SetLocalVelocity( vecVelocity );
		#endif
		vecVelocity *= flFrametime;
		vecEndPos = vecStartPos + vecVelocity;
		if ( vecEndPos.IsValid() )
		{
			CTraceFilterSkipTwoEntities movetrfilter( this, m_pShooter, COLLISION_GROUP_NONE );
			trace_t movetr;
			UTIL_TraceLine( vecStartPos, vecEndPos, MASK_SHOT, &movetrfilter, &movetr );

			#ifndef CLIENT_DLL
				//Trace to triggers so we can hit surf glass and such
				CTakeDamageInfo	triggerInfo( this, GetOwnerEntity(), m_iDamage, DMG_BULLET );
				if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
				{
					//CalculateMeleeDamageForce( &triggerInfo, vecDir, movetr.endpos, 0.7f );
					Vector vecForce = vecDir;
					VectorNormalize( vecForce );
					//vecForce *= 10.0f;
					triggerInfo.SetDamageForce( vecForce );
				}
				else
					CalculateBulletDamageForce( &triggerInfo, m_iAmmoType, vecDir, movetr.endpos, 1.0f );
				triggerInfo.SetDamagePosition( movetr.endpos );
				TraceAttackToTriggers( triggerInfo, movetr.startpos, movetr.endpos, vecDir );
			#else
				//Hit ragdolls on the client
				CBaseEntity* pEnt = DHL_FX_AffectRagdolls( movetr.endpos, movetr.startpos, DMG_BULLET, &m_RagdollHitList );

				//Keep track of ones we've hit
				if ( pEnt )
					m_RagdollHitList.AddToTail( pEnt );
			#endif

			if ( movetr.DidHit() )
				if ( OnTouch( movetr, false, &movetrfilter ) )
					return;
			
			MoveProjectileToPosition( vecEndPos );
			m_flDistanceTravelled += vecEndPos.DistTo( vecStartPos );

			#ifndef CLIENT_DLL
				//On rare occasions the projectile likes to fly right through the world and keep going forever, causing a memory leak
				if ( m_flDistanceTravelled > MAX_TRACE_LENGTH )
				{
					SUB_Remove();
					//SetThink( &CDHLProjectile::SUB_Remove );
					//SetNextThink( gpGlobals->curtime + 0.1 );
				}
			#endif

		}

		//Simulate Angles
		//QAngle angles;
		#ifdef CLIENT_DLL
			QAngle angles = GetLocalAngles();
			//VectorAngles( vecDir, angles );
			//angles.z = GetLocalAngles().z; //Vector conversion loses z
			QAngle angVel = GetLocalAngularVelocity();
			angles += angVel * flFrametime;
			SetLocalAngles( angles );
			SetNetworkAngles( angles );
		#endif
	}
}

//Called from PhysicsSimulate() or ReceiveMessage()
bool CDHLProjectile::OnTouch( trace_t &touchtr, bool bDecalOnly /*= false*/, ITraceFilter* pTraceFilter /*= NULL*/ )
{
	//Direction
	Vector vecDir = touchtr.endpos - touchtr.startpos;
	if ( vecDir == vec3_origin ) //Sometimes endpos==startpos so we need to get dir from velocity instead
	{
		#ifdef CLIENT_DLL
			vecDir = GetLocalVelocity();
		#else
			vecDir = m_vecCurVelocity;
		#endif
		VectorNormalize( vecDir );
	}

	CBaseEntity* ent = touchtr.m_pEnt;
	if ( !ent )
		return false;

	if ( touchtr.DidHit() )
	{
		//Never collide with self, shooter, or other projectiles
		if ( ent == this || dynamic_cast<CDHLProjectile*>(ent)
			|| ent == (CBaseEntity*)m_pShooter )
			//|| ( (m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE) && (ent == m_pFiringWeapon) ) ) //Combat knife - don't collide with weapon ent
			return false;

		//Hack: Sometimes hits are registered prematurely (usually to the torso area) with no hitbox.  Pretend nothing happened unless one is found.
		if ( ent->IsPlayer() && touchtr.hitgroup == 0 )
			return false;

		//Check friendly fire
		if ( CheckFriendlyFire( ent ) )
		{
			if ( !bDecalOnly )
			{
				ClearMultiDamage();

				//Do damage
				CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_BULLET  );
				if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
				{
					//CalculateMeleeDamageForce( &dmgInfo, vecDir, touchtr.endpos, 0.01f );
					Vector vecForce = vecDir;
					VectorNormalize( vecForce );
					//vecForce *= 10.0f; //Ripped from C_ClientRagdoll::ImpactTrace
					dmgInfo.SetDamageForce( vecForce );

					#ifndef CLIENT_DLL
						if ( IsOnFire() )
						{
							CBaseAnimating* pBAnim = dynamic_cast<CBaseAnimating*>(ent);
							if ( pBAnim )
								pBAnim->Ignite( 10.0f, false );
						}
					#endif
				}
				else
					CalculateBulletDamageForce( &dmgInfo, m_iAmmoType, vecDir, touchtr.endpos, 1.0f );
				dmgInfo.SetDamagePosition( touchtr.endpos );
				ent->DispatchTraceAttack( dmgInfo, vecDir, &touchtr );
				ApplyMultiDamage();
			}

			#ifdef CLIENT_DLL
				if ( ent->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
					return false;

				//Decals and such
				if ( !( touchtr.surface.flags & SURF_SKY ) && !touchtr.allsolid )
				{
					IPredictionSystem::SuppressEvents( false );
					if ( (m_iType == DHL_PROJECTILE_TYPE_BULLET || m_iType == DHL_PROJECTILE_TYPE_PELLET) )
					{
						UTIL_ImpactTrace( &touchtr, DMG_BULLET );
					}
					if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
						PlayImpactSound( touchtr.m_pEnt, touchtr, touchtr.endpos, touchtr.surface.surfaceProps );
					IPredictionSystem::SuppressEvents( !prediction->IsFirstTimePredicted() );
				}
			#endif
		}

		if ( pTraceFilter && m_iType != DHL_PROJECTILE_TYPE_COMBATKNIFE )
		{
			PenetrationData_t nPenetrationData = DHLShared::TestPenetration( touchtr, m_pShooter, pTraceFilter, 
				m_iTimesPenetrated, m_flDistanceTravelled, m_iAmmoType );
			if ( nPenetrationData.m_bShouldPenetrate )
			{
				m_flDistanceTravelled += GetLocalOrigin().DistTo( nPenetrationData.m_vecNewBulletPos );
				MoveProjectileToPosition( nPenetrationData.m_vecNewBulletPos );
				m_iTimesPenetrated++;
				return true; //Keep going - but don't do anything else in this frame of PhysicsSimulate()
			}
		}
	
		//We're done unless what we hit was breakable glass
		if ( ent->GetCollisionGroup() != COLLISION_GROUP_BREAKABLE_GLASS )
		{
			#ifdef CLIENT_DLL
				m_bCollided = true;
				AddEffects( EF_NODRAW );
				if ( m_pTrail ) //NULL pointer here sometimes somehow...
					m_pTrail->AddEffects( EF_NODRAW );
			#else
				EntityMessageBegin( this );
					WRITE_BYTE( MSG_NOTIFY_REMOVAL );
				MessageEnd();
				
				if ( touchtr.DidHitWorld() && m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE && !( touchtr.surface.flags & SURF_SKY ) )
				{
					CBaseCombatWeapon* pKnifeEnt = assert_cast<CBaseCombatWeapon*>(CreateEntityByName( "weapon_combatknife" ));
					if ( pKnifeEnt )
					{
						pKnifeEnt->AddSpawnFlags( SF_NORESPAWN ); //Needed for weapon spawn & VPhysics setup to work correctly
						pKnifeEnt->SetAbsOrigin( touchtr.endpos );
						QAngle angles = vec3_angle;
						Vector vecKnifeDir = touchtr.startpos - touchtr.endpos;
						VectorAngles( vecKnifeDir, angles );
						angles[PITCH] -= 15.0f; //Correct for the .mdl being offset a bit
						pKnifeEnt->SetLocalAngles( angles );
						DispatchSpawn( pKnifeEnt );

						//Spawns vphys object and sets it up, essentially a copy of CWeaponHL2MPBase::FallInit()
						pKnifeEnt->VPhysicsDestroyObject();
						//Using SOLID_VPHYSICS instead of SOLID_BBOX (as ordinary weapons do) helps resolve some of the client side collision oddities
						Assert( pKnifeEnt->VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_STANDABLE | FSOLID_TRIGGER, true ) );
						pKnifeEnt->SetPickupTouch(); //Sets up automagic removal after time
						IPhysicsObject* pKnifePhys = pKnifeEnt->VPhysicsGetObject();
						if ( pKnifePhys )
						{
							//Knives are solid to bullets...the only way to make them non-solid to bullets is to do SetSolid( SOLID_NONE ) or AddSolidFlags( FSOLID_NOT_SOLID )
							//which breaks the +use pickup even with FSOLID_TRIGGER set.  Let's just call it a feature :)
							pKnifePhys->EnableMotion( false );
							pKnifePhys->EnableCollisions( false );
						}

						if ( IsOnFire() )
							pKnifeEnt->Ignite( 10.0f, false );
					}
				}

				//SetThink( &CDHLProjectile::SUB_Remove );
				//SetNextThink( gpGlobals->curtime + 0.1 );
				//SUB_Remove();
				//SetMoveType( MOVETYPE_NONE );
				m_flRemoveAt = gpGlobals->curtime + 0.1f; //Give the notification message a head start so that the client will have time to react
			#endif
		}
	}
	return true;
}

//Note - this only gets called on the server for now despite being shared
void CDHLProjectile::Fire( Vector vecOrigin, Vector vecVelocity, int iDamage, CBaseCombatWeapon *pSrcWeapon, CBasePlayer* pShooter, int iAmmoType )
{
	SetOwnerEntity( pShooter );
	m_iDamage = m_iOriginalDamage = iDamage;
	m_pFiringWeapon = pSrcWeapon;
	m_pShooter = pShooter;
	m_hShooter = pShooter;
	m_iAmmoType = iAmmoType;

	m_vecProjectileVelocity = vecVelocity;
	m_vecProjectileOrigin = vecOrigin;

	CDHLBaseWeapon* pDHLWep = static_cast<CDHLBaseWeapon*>(pShooter->GetActiveWeapon());

	m_iType = 0;
	if ( pDHLWep )
		m_iType = pDHLWep->GetProjectileType();

#ifndef CLIENT_DLL
	m_vecCurPosition = vecOrigin;
	m_vecCurVelocity = vecVelocity;

	SetupGravity(); //Client does this in OnDataChanged() creation
#endif

	SetMoveType( MOVETYPE_CUSTOM );
}

//Check friendly fire rules to see if a player should be hit
//Returns true if damage should be dealt, false if it should not
bool CDHLProjectile::CheckFriendlyFire( CBaseEntity* pEnt )
{
	if ( DHLRules()->IsTeamplay() )
	{
		if ( pEnt->IsPlayer() )
		{
			CDHL_Player* pPlayer = ToDHLPlayer( pEnt );
			if ( pPlayer && m_pShooter )
			{
				if ( pPlayer->GetTeamNumber() == m_pShooter->GetTeamNumber() )
				{
					static ConVarRef ffVar("mp_friendlyfire");
					if ( ffVar.GetBool() )
						return false;
				}
			}
		}
	}
	return true;
}

void CDHLProjectile::SetupGravity( void )
{
	float flGravity = 1.0f;
	if ( (m_iType == DHL_PROJECTILE_TYPE_BULLET || m_iType == DHL_PROJECTILE_TYPE_PELLET) )
		flGravity = 0.1f;
	else if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
		flGravity = 1.35f;
	SetGravity( flGravity );
}

#ifdef CLIENT_DLL
void CDHLProjectile::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}
	int iType = msg.ReadByte();

	//Server is letting us know that we're about to be deleted
	if ( iType == MSG_NOTIFY_REMOVAL )
	{
		if ( !m_bCollided )
		{
			Vector vecDir = vec3_origin;
			if ( GetAbsVelocity() != vec3_origin )
				vecDir = GetAbsVelocity();
			else
				vecDir = m_vecProjectileVelocity;
			VectorNormalize( vecDir );

			Vector vecStartPos = GetMoveType() == MOVETYPE_CUSTOM ? GetLocalOrigin() : m_vecProjectileOrigin;

			//Try to plant a decal
			trace_t decaltr;
			UTIL_TraceLine( vecStartPos, vecStartPos + (vecDir * 120.0), MASK_SHOT, this, //Pretty long distance, but seems necessary in practice
				COLLISION_GROUP_NONE, &decaltr );
			//DebugDrawLine( decaltr.startpos, decaltr.endpos, 255, 0, 0, false, 3.0f );
			if ( decaltr.DidHit() )
			{
				OnTouch( decaltr, true );
			}

			m_bCollided = true;
		}

	}
}
#endif
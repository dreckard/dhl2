//=============================================================================//
// Purpose:	Distraction Half-Life 2 melee weapon baseclass
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl/dhl_basemeleeweapon.h"
#include "engine/ivdebugoverlay.h"

#ifndef CLIENT_DLL
#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( DHLBaseMeleeWeapon, DT_BaseDHLBaseMeleeWeapon )

BEGIN_NETWORK_TABLE( CDHLBaseMeleeWeapon, DT_BaseDHLBaseMeleeWeapon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CDHLBaseMeleeWeapon )
END_PREDICTION_DATA()

CDHLBaseMeleeWeapon::CDHLBaseMeleeWeapon()
{
}

#ifdef CLIENT_DLL
extern bool FX_AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType );
#endif

ConVar dhl_visualizemeleerange( "dhl_visualizemeleerange", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar dhl_visualizemeleerange_time( "dhl_visualizemeleerange_time", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar dhl_meleehullscalew( "dhl_meleehullscalew", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar dhl_meleehullscaleh( "dhl_meleehullscaleh", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar dhl_meleerangescale( "dhl_meleerangescale", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
void CDHLBaseMeleeWeapon::PrimaryAttack( void )
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	//DHL - Skillet - Modified version of CBaseHL2MPBludgeonWeapon::Swing()
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange() * dhl_meleerangescale.GetFloat();
	trace_t traceHit;
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

	#ifndef CLIENT_DLL
		CTakeDamageInfo triggerInfo( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), GetDamageType() );
		TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );
	#endif

	//Do some hull nonsense
	if ( traceHit.fraction == 1.0 )
	{
		float flHullDimw = GetMeleeHullW() * dhl_meleehullscalew.GetFloat();
		float flHullDimh = GetMeleeHullH() * dhl_meleehullscaleh.GetFloat();

		float bludgeonHullRadius = 1.732f * (1/((GetRange()*dhl_meleerangescale.GetFloat())/75.0f)) * flHullDimw;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		Vector vecBludgeonMins(-flHullDimw,-flHullDimw,-flHullDimh);
		//Vector vecBludgeonMins( 0.0f, 0.0f, 0.0f );
		Vector vecBludgeonMaxs(flHullDimw,flHullDimw,flHullDimh);

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull( swingStart, swingEnd, vecBludgeonMins, vecBludgeonMaxs, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &traceHit );
#ifdef CLIENT_DLL
		if ( dhl_visualizemeleerange.GetBool() )
			debugoverlay->AddSweptBoxOverlay( swingStart, swingEnd, vecBludgeonMins, vecBludgeonMaxs, EyeAngles(), 255, 0, 0, 150, dhl_visualizemeleerange_time.GetFloat() );
#endif
		if ( traceHit.fraction < 1.0 && traceHit.m_pEnt )
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize( vecToTarget );

			float dot = vecToTarget.Dot( forward );

			//Skillet - The world's origin is useless for this kind of thing
			if ( traceHit.m_pEnt->entindex() == 0 )
				dot = 1.0f;

			// YWB:  Make sure they are sort of facing the guy at least...
			if ( dot < 0.70721f )
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				nHitActivity = ChooseIntersectionPointAndActivity( traceHit, vecBludgeonMins, vecBludgeonMaxs, pOwner );
			}
		}
	}

	WeaponSound( SINGLE );

	if ( !traceHit.DidHit() )
	{
		nHitActivity = ACT_VM_MISSCENTER;
		ImpactWater( swingStart, swingEnd );
	}
	else
	{
		//CBaseHL2MPBludgeonWeapon::Hit()
		CBaseEntity	*pHitEntity = traceHit.m_pEnt;
		CBasePlayer *pPlayer = pOwner;

		//Apply damage to a hit target
		if ( pHitEntity != NULL )
		{
			Vector hitDirection;
			pPlayer->EyeVectors( &hitDirection, NULL, NULL );
			VectorNormalize( hitDirection );

			#ifndef CLIENT_DLL
				CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), GetDamageType() );

				if( pPlayer && pHitEntity->IsNPC() )
				{
					// If bonking an NPC, adjust damage.
					info.AdjustPlayerDamageInflictedForSkillLevel();
				}

				//Tracehull won't find hitboxes, everything is HITGROUP_GENERIC
				if ( traceHit.hitgroup == 0 && pHitEntity->IsPlayer() )
				{
					CBasePlayer* pHitPlayer = ToBasePlayer(pHitEntity);
					float flZDiff = (traceHit.endpos.z - pHitPlayer->GetAbsOrigin().z);
					if ( flZDiff > 0 && !(pHitPlayer->GetFlags() & FL_PRONE) )
					{
						//Keep really low leg swings from dealing the full GENERIC 1.0x damage
						ConVarRef sk_player_leg( "sk_player_leg" );
						if ( (pHitPlayer->GetFlags() & FL_DUCKING) && flZDiff < 16.0f )
							info.ScaleDamage( sk_player_leg.GetFloat() );
						else if ( flZDiff < 34.0f )
							info.ScaleDamage( sk_player_leg.GetFloat() );
					}
				}

				CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

				pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
				ApplyMultiDamage();

				// Now hit all triggers along the ray that... 
				TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
			#endif
			//DHL - Skillet
			if ( pHitEntity->entindex() == 0 )
				WeaponSound( MELEE_HIT_WORLD );
			else
				WeaponSound( MELEE_HIT );
		}

		// Apply an impact effect
		ImpactEffect( traceHit );
	}

	#ifdef CLIENT_DLL
		//Affect ragdolls
		FX_AffectRagdolls( traceHit.endpos, traceHit.startpos, GetDamageType() );
	#endif

	SendWeaponAnim( nHitActivity );

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();


#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}
//=============================================================================//
// Purpose:	Distraction Half-Life 2 player entity
//
// Author: Skillet
//=============================================================================//

#include "cbase.h"
#include "dhl_shared.h"
#include "shot_manipulator.h"
#include "dhl_gamerules.h"
#include "dhl/dhl_shareddefs.h"
#include "dhl/dhl_baseweapon.h"

#ifdef CLIENT_DLL
#define CDHLProjectile C_DHLProjectile
#include "dhl/c_dhl_projectile.h"
#include "imovehelper.h"
#else
#include "dhl/dhl_projectile.h"
#endif

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "in_buttons.h"

#include "dhl/dhl_player_inc.h"
#ifndef CLIENT_DLL
	#include "func_breakablesurf.h"
	#include "ilagcompensationmanager.h"
#endif

#include "takedamageinfo.h"
#include "weapon_hl2mpbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar dhl_sprint( "dhl_sprint", "0", FCVAR_REPLICATED, "Enable (1) or disable (0) sprinting" );

void CDHL_Player::PreThink( void )
{
	//Hack to let player fly through glass
	/*if ( vecSavedVelocity != vec3_origin )
	{
		SetAbsVelocity( vecSavedVelocity );
		vecSavedVelocity = vec3_origin;
	}*/

#ifndef CLIENT_DLL
	if ( m_bProne )
		Prone();
#endif
	BaseClass::PreThink();
}

ConVar dhl_bleeding( "dhl_bleeding", "1", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_NOTIFY );
void CDHL_Player::PostThink( void )
{
	if ( m_bIsBleeding )
	{
		if ( m_nButtons & IN_BANDAGE || m_bBandaging )
		{
			Bandage();
		}
	}
#ifndef CLIENT_DLL
	if ( m_bIsBleeding && m_flBleedTimer <= gpGlobals->curtime && dhl_bleeding.GetBool() )
	{
		Bleed();
	}
#else
	//Bleeding effects
	//Bleeding info is only valid for the local player
	if ( IsLocalPlayer() )
	{
		//We are bleeding, figure out blur
		if ( m_bIsBleeding && IsAlive() && dhl_bleeding.GetBool() )
		{
			const float flEffectsHealth = 30.0f; //Bleeding effects start when player is bleeding and below this threshold
			const float flMaxMax = 0.010f;
			const float flMinInterval = 1.0f;

			float flHealth = float(GetHealth());
			blurInfo.flFraction = min(1.0, flHealth / flEffectsHealth);
			if ( flHealth < flEffectsHealth )
			{
				//Decide how much to blur based on health
				//This should decrease as health increases
				float flIntensityFactor = flHealth / flEffectsHealth;
				blurInfo.flInterval = max(flMinInterval, flIntensityFactor * 6.0f);

				//These should increase as health decreases
				flIntensityFactor = flEffectsHealth / flHealth;
				blurInfo.flMax = min(flMaxMax, flIntensityFactor * 0.01f);
				//blurInfo.flMin = -blurInfo.flMax;

				blurInfo.bBlur = true;
			}
			else if ( blurInfo.bBlur )
				blurInfo.bBlur = false;
		}
		else if ( blurInfo.bBlur )
		{
			blurInfo.bBlur = false;
		}

		//HACK: Check on nightvision
		if ( GetNightvisionEnabled() && !(m_iItemFlags & DHL_IFLAG_NIGHTVISION) )
			EnableNightvision( false );
	}
	UpdateGradVAngle(); //Client only now
#endif

	BaseClass::PostThink();

#ifndef CLIENT_DLL
	QAngle angles = EyeAngles();
	angles[ROLL] = GetLocalAngles()[ROLL];

	SetLocalAngles( angles );
#endif
}

bool CDHL_Player::ShouldTouch( CBaseEntity *pOther, trace_t &trace )
{
	if ( GetMoveType() != MOVETYPE_NOCLIP && GetMoveType() != MOVETYPE_OBSERVER && GetMoveType() != MOVETYPE_NONE && GetSolid() != SOLID_NONE )
	{
		if ( GetAbsVelocity().Length() > 275.0f )
		{
			//Apparently only func_breakable_surf goes into this collision group, regular breakable glass is COLLISION_GROUP_NONE
			if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			{
				#ifndef CLIENT_DLL
					CBreakableSurface *pSurfGlass = static_cast<CBreakableSurface*>( pOther );
					if ( pSurfGlass )
					{
						if ( !pSurfGlass->IsBroken() )
						{
							pSurfGlass->Die( this, trace.endpos - trace.startpos );
						}
					}
				#endif
				return false;
			}

			//For some damn reason the movetype on breakables is MOVETYPE_PUSH
			//Also, m_takedamage isn't valid on the client for breakables so this is the best I can do for identifying them
			//Other ents with MOVETYPE_PUSH: doors, func_brush. Filter them out.
			//Hopefully this narrows down to just func_breakables.  Prediction will wonk if a breakable has more than 20 health, too bad.
			else if ( pOther->GetCollisionGroup() == COLLISION_GROUP_NONE && pOther->GetMoveType() == MOVETYPE_PUSH && !(pOther->GetFlags() & FL_WORLDBRUSH) )
			{
				if ( pOther->GetSolid() == SOLID_BSP && !(pOther->GetSolidFlags() & FSOLID_NOT_SOLID) )
				{
					#ifndef CLIENT_DLL
						CTakeDamageInfo info( this, this, GetAbsVelocity(), trace.endpos, 20.0f, DMG_GENERIC );
						pOther->DispatchTraceAttack( info, trace.endpos - trace.startpos, &trace );
						ApplyMultiDamage();
					#endif
					return false;
				}
			}
		}
	}
	return BaseClass::ShouldTouch( pOther, trace );
}

bool CDHL_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*=0*/ )
{
	//Mixed Akimbo
	/*if ( BaseClass::Weapon_Switch( pWeapon, viewmodelindex ) )
	{
		if ( GetViewModel( 1 ) )
		{
			CDHLBaseWeapon* pDHLWep = dynamic_cast<CDHLBaseWeapon*>(pWeapon);
			if ( pDHLWep && pDHLWep->IsAkimbo() && pDHLWep->m_pOtherAkimbo )
			{
				GetViewModel( 1 )->RemoveEffects( EF_NODRAW );
				//Weapon_Switch( pDHLWep->m_pOtherAkimbo, 1 );
			}
			else
				GetViewModel( 1 )->AddEffects( EF_NODRAW );
		}
		return true;
	}
	return false;*/
	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}

//Bandage the player
const float DHL_BANDAGE_TIME = 4.0f;
void CDHL_Player::Bandage( void )
{
	if ( !m_bIsBleeding )
		return;

	//Must be on the ground
	if ( GetGroundEntity() && !m_bBandaging )
	{
		//Can't fire weapon while bandaging
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->m_flNextPrimaryAttack = GetActiveWeapon()->m_flNextSecondaryAttack = gpGlobals->curtime + DHL_BANDAGE_TIME / DHLRules()->GetTimescale();

			//Deal with the view model
			GetActiveWeapon()->SendWeaponAnim( ACT_VM_HOLSTER );
			GetActiveWeapon()->SetWeaponIdleTime( FLT_MAX ); //Never
			//flVisTime = gpGlobals->curtime + GetActiveWeapon->SequenceDuration();
		}

		m_flBandageEndTime = gpGlobals->curtime + DHL_BANDAGE_TIME / DHLRules()->GetTimescale();
		m_bBandaging = true;
		SetMaxSpeed( DHL_NORM_SPEED / 2 );
		m_bSpeedLocked = true;
	}
	else if ( m_bBandaging )
	{
		if ( m_flBandageEndTime <= gpGlobals->curtime )
		{
			CBaseCombatWeapon *pWeapon = GetActiveWeapon();
			if ( pWeapon )
			{
				//pWeapon->SetWeaponVisible( true );
				pWeapon->SendWeaponAnim( ACT_VM_DRAW );
				pWeapon->WeaponSound( DRAW );
				pWeapon->SetWeaponIdleTime( gpGlobals->curtime + pWeapon->SequenceDuration() );
			}

			m_bSpeedLocked = false;
			SetMaxSpeed( DHL_NORM_SPEED );
			m_bBandaging = false;
			m_flBandageEndTime = 0.0f;
			#ifndef CLIENT_DLL
				StopBleeding();
			#endif
		}
	}
}

void CDHL_Player::SetMaxSpeed( float flMaxSpeed )
{
	if ( !m_bSpeedLocked )
		BaseClass::SetMaxSpeed( flMaxSpeed );
}

void CDHL_Player::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;
	//For some reason this isn't set automatically...
	modinfo.m_pAttacker = this;

	CWeaponHL2MPBase *pWeapon = static_cast<CWeaponHL2MPBase*>( GetActiveWeapon() );
	if ( pWeapon )
	{
		//DHL: Added damage scalar
		modinfo.m_iPlayerDamage = modinfo.m_iDamage = ( pWeapon->GetHL2MPWpnData().m_iPlayerDamage * modinfo.m_flDamageScalar );
	}
	//Physics bullets in slow motion
	if ( DHLRules()->GetTimescale() < 1.0f )
	{
		FireBullet( modinfo );
		return;
	}
//#ifndef CLIENT_DLL
//	//If we're doing a traceline bullet, inflict damage to surf glass first
//	bool bGlassCleared = false;
//	while ( !bGlassCleared )
//	{
//		bGlassCleared = true;
//		trace_t glassTrace;
//		UTIL_TraceLine( modinfo.m_vecSrc, modinfo.m_vecSrc + ( modinfo.m_vecDirShooting * modinfo.m_flDistance ), 
//			MASK_SOLID, NULL, COLLISION_GROUP_NONE, &glassTrace );
//		CTakeDamageInfo dmgInfo( this, this, vec3_origin, glassTrace.endpos, modinfo.m_iDamage, DMG_BULLET );
//		if ( glassTrace.DidHitNonWorldEntity() )
//		{
//			CBreakableSurface *pSurfGlass = dynamic_cast<CBreakableSurface*>( glassTrace.m_pEnt );
//			if ( pSurfGlass )
//				//Apply damage
//				this->TraceAttack( dmgInfo, modinfo.m_vecDirShooting, &glassTrace );
//			bGlassCleared = false;
//		}
//	}
//#endif
	#ifndef CLIENT_DLL
		//Only lagcomp on the original shot or we end up nesting the lagcomp calls and things get ugly
		bool bShouldLagComp = (info.m_iTimesPenetrated==0);
		if ( bShouldLagComp )
			lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );
	#endif

	BaseClass::FireBullets( modinfo );

	#ifndef CLIENT_DLL
		if ( bShouldLagComp )
			lagcompensation->FinishLagCompensation( this );
	#endif
}

//Fire a physics bullet
void CDHL_Player::FireBullet( const FireBulletsInfo_t info )
{
#ifndef CLIENT_DLL
	CShotManipulator Manipulator( info.m_vecDirShooting ); //Only need one of these
	//Ass on a pancake, we have to run all of this repeatedly for multiple m_iShots
	for ( int i = 1; i <= info.m_iShots; i++ )
	{
		CDHLProjectile* pBullet = (CDHLProjectile*)(CreateEntityByName( "dhl_projectile" ));
		if ( !pBullet )
			return;

		pBullet->SetOwnerEntity( this );
		pBullet->Spawn();
		Vector vecDir;
		Vector vecSrc;

		//Same function the trace line code uses to determine actual direction vectors
		vecDir = Manipulator.ApplySpread( info.m_vecSpread );

		//HACK - move the bullet forward so it doesn't appear to spawn inside the player's face
		//Note: the new bullet spawn should still be inside the player's bbox, so hopefully there won't be any
		//hit detection problems at point blank
		vecSrc = info.m_vecSrc + ( vecDir * 6.0f );

		Vector vecBulletVelocity = ( vecDir * 16800.0f ); //16800 in/s :|

		pBullet->Fire( vecSrc, vecBulletVelocity, info.m_iDamage, GetActiveWeapon(), this, info.m_iAmmoType );
	}
#endif
}

void CDHL_Player::TimescaleChanged( float flNewTimescale, float flOldTimescale )
{
#ifdef CLIENT_DLL
	if ( flNewTimescale != flOldTimescale ) //Stupid conditional but who knows...
		m_flLastTimescaleChange = gpGlobals->curtime;
#endif

	float flFraction = flNewTimescale / flOldTimescale;
	if ( m_flBandageEndTime > gpGlobals->curtime )
		m_flBandageEndTime = gpGlobals->curtime + ( (m_flBandageEndTime - gpGlobals->curtime) / flFraction );
#ifndef CLIENT_DLL
	if ( m_flBleedTimer > gpGlobals->curtime )
		m_flBleedTimer = gpGlobals->curtime + ( (m_flBleedTimer - gpGlobals->curtime) / flFraction );
#endif
}

//Update gradual view angle change - once per frame
#ifdef CLIENT_DLL
void CDHL_Player::UpdateGradVAngle( void )
{
	//Bad news for cheating prevention - there's really nothing that can be done to try and enforce recoil on a client.
	//Randomization helps against possible scripts, and hopefully VAC will keep the memory hookers away somewhat.
	//This code will run on the server as well, and will network fine, but with internet level pings it makes things really choppy.
	QAngle angGradVAngle = m_angGradVAngle; //This looks stupid, but its needed for server side operators to work properly (CNetworkVar)
	if ( angGradVAngle != vec3_angle && m_flGradVAngleTime >= 0.0f )
	{
		m_flGradVAngleTimeElapsed += gpGlobals->frametime;

		angGradVAngle = angGradVAngle * (m_flGradVAngleTimeElapsed / m_flGradVAngleTime);

		QAngle angDesired = EyeAngles() + angGradVAngle;
		engine->SetViewAngles( angDesired );

		if ( m_flGradVAngleTimeElapsed >= m_flGradVAngleTime )
		{
			m_angGradVAngle = vec3_angle;
			m_flGradVAngleTime = 0.0f;
			m_flGradVAngleTimeElapsed = 0.0f;
		}
	}
	else
		m_angGradVAngle = vec3_angle;
}

void CDHL_Player::GradVAngleChange( QAngle angAngles, float flTime )
{
	m_angGradVAngle += angAngles;
	m_flGradVAngleTime = flTime;
	m_flGradVAngleTimeElapsed = 0.0f;
}
#endif //CLIENT_DLL

int CDHL_Player::GetWeaponInvVal( int iWeapon )
{
	if ( iWeapon >= WEAPON_FIRST_MELEE && iWeapon <= WEAPON_LAST_MELEE ) //Melee
		return DHL_INV_VAL_ITEM;
	if ( iWeapon >= WEAPON_FIRST_HANDGUN && iWeapon <= WEAPON_LAST_HANDGUN ) //Handguns
	{
		//HACK: Because I'm too lazy to change all the indices in 20 places
		if ( iWeapon == WEAPON_BERETTA_AKIMBO )
			return DHL_INV_VAL_UNIQUE;
		return DHL_INV_VAL_HANDGUN;
	}
	if ( iWeapon >= WEAPON_FIRST_UNIQUE && iWeapon <= WEAPON_LAST_UNIQUE ) //"Uniques"
		return DHL_INV_VAL_UNIQUE;
	if ( iWeapon == WEAPON_GRENADE ) //Grenade is all by its lonesome with the weapons
		return DHL_INV_VAL_ITEM;
	
	return DHL_INV_VAL_ITEM;
}

//Total "value" of items we're carrying
int CDHL_Player::GetTotalInvVal( void )
{
	int iRet = 0;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CDHLBaseWeapon* pWep = static_cast<CDHLBaseWeapon*>(GetWeapon(i));
		if ( pWep )
			iRet += pWep->m_iInventoryValue;
	}
	return iRet;
}

bool CDHL_Player::CanSprint( void )
{
	if ( m_bProne || !dhl_sprint.GetBool() )
		return false;
	return BaseClass::CanSprint();
}
//=============================================================================//
// Purpose:	Distraction Half-Life 2 player entity
//
// Author: Skillet/Payback
//=============================================================================//

#include "cbase.h"
#include "dhl_player.h"
#include "dhl/dhl_shared.h"
#include "hl2mp_player.h"
#include "dhl/dhl_gamerules.h"
#include "in_buttons.h"
#include "func_breakablesurf.h"
#include "weapon_hl2mpbase.h"
#include "dhl/dhl_shareddefs.h"
#include "dhl/dhl_baseweapon.h"
#include "dhl/dhl_projectile.h"
#include "ammodef.h"
#include "te_effect_dispatch.h"
#include "ai_basenpc.h" //For VecCheckToss()
#include "effect_dispatch_data.h"
#include "datacache/imdlcache.h"
#include "team.h"
#include "EntityFlame.h"
#ifndef _WIN32
#include "minmax.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float flHealthToArmorRatio = 1.25f; //You lose 1.25x as much armor as you would health
const float flArmorProtection = 0.8f; //Protects from 80% of damage

ConVar dhl_weaponrestrictions( "dhl_weaponrestrictions", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Enable (1) or disable (0) weapon restrictions" );

CON_COMMAND( selectweapon, "Adds an item to the player's item selection \n Usage: <command> (numeric item identifier) (0 Add/1 Remove) \n Toggles the given item if no arguments are given" )
{
	CDHL_Player* pPlayer = ToDHLPlayer( UTIL_GetCommandClient() );
    if ( !pPlayer )
        return;

	//Return if no parameters given
    if (args.ArgC() == 1)
        return;

	//Parse the number from the given parameters
	int iWeapon = atoi(args.Arg( 1 ));

	//Command + two params
	if ( args.ArgC() == 3 )
	{
		int iRemove = atoi(args.Arg( 2 ));
		if ( iRemove > 0 )
			pPlayer->GetWeaponSelection()->RemoveWeapon( iWeapon );
		else
		{
			if ( dhl_weaponrestrictions.GetBool() && pPlayer->m_iSelectedInventoryValue + pPlayer->GetWeaponInvVal( iWeapon ) > DHL_INV_VAL_SELMAX )
				return;
			pPlayer->GetWeaponSelection()->AddWeapon( iWeapon );
		}
	}
	//Toggle if no value to set is given
	else if ( args.ArgC() == 2 )
	{
		//If adding this weapon would put the player over their maximum, they may only remove it
		if ( dhl_weaponrestrictions.GetBool() && pPlayer->m_iSelectedInventoryValue + pPlayer->GetWeaponInvVal( iWeapon ) > DHL_INV_VAL_SELMAX )
		{
			if ( !pPlayer->GetWeaponSelection()->FindWeapon( pPlayer->GetWeaponSelection()->ParseCommand(iWeapon) ) )
				return;
		}
		//If only one param is given toggle the weapon
		//Add it to our player's weapon selection list
		pPlayer->GetWeaponSelection()->ToggleWeapon( iWeapon );
	}
}
//ConCommand selectweapon( "selectweapon", CC_SelectWeapon, "Adds a weapon to the player's weapon selection \n Usage: <command> (numeric weapon identifier) (0 Add/1 Remove) \n Toggles the given weapon if no arguments are given", 0 );

CON_COMMAND( selectitem, "Adds an item to the player's item selection \n Usage: <command> (numeric item identifier) (0 Add/1 Remove) \n Toggles the given item if no arguments are given" )
{
	CDHL_Player* pPlayer = ToDHLPlayer( UTIL_GetCommandClient() );
    if ( !pPlayer )
        return;

	//Return if no parameters given
    if (args.ArgC() == 1)
        return;

	//Parse the number from the given parameters
	int iItem = atoi(args.Arg( 1 ));

	int iShiftCount = iItem - FIRST_ITEM;

	//Command + two params
	if ( args.ArgC() == 3 )
	{
		int iRemove = atoi(args.Arg( 2 ));
		if ( iRemove > 0 )
			pPlayer->RemoveItems((1<<iShiftCount));
		else
		{
			if ( dhl_weaponrestrictions.GetBool() && pPlayer->m_iSelectedInventoryValue + DHL_INV_VAL_ITEM > DHL_INV_VAL_SELMAX )
				return;

			pPlayer->AddItems((1<<iShiftCount));
		}
	}
	else if ( args.ArgC() == 2 )
	{
		//Can't add more than we're able to handle
		if ( dhl_weaponrestrictions.GetBool() && pPlayer->m_iSelectedInventoryValue + DHL_INV_VAL_ITEM > DHL_INV_VAL_SELMAX )
		{
			if ( !(pPlayer->m_iItemFlags & (1<<iShiftCount)) )
				return;
		}

		//Toggle
		pPlayer->ToggleItem((1<<iShiftCount));
	}
}
//ConCommand selectitem( "selectitem", CC_SelectItem, "Adds an item to the player's item selection \n Usage: <command> (numeric item identifier) (0 Add/1 Remove) \n Toggles the given item if no arguments are given", 0 );

LINK_ENTITY_TO_CLASS( player, CDHL_Player );

//Data that only gets sent to the client who owns this particular player instance
BEGIN_SEND_TABLE_NOBASE( CDHL_Player, DT_DHLLocalPlayerExclusive )
	SendPropInt( SENDINFO( m_iDHLArmor ), 7, SPROP_UNSIGNED ), //128

	SendPropBool(SENDINFO(m_bIsBleeding) ),
	SendPropBool(SENDINFO(m_bProneStandReady) ),
	SendPropInt(SENDINFO(m_iSelectedInventoryValue), 5, SPROP_UNSIGNED ), //32
	SendPropBool(SENDINFO(m_bScoped) ),
	SendPropInt(SENDINFO(m_iStylePoints), 5, SPROP_UNSIGNED ), //32
	SendPropInt(SENDINFO(m_iItemFlags), DHL_IFLAG_COUNT, SPROP_UNSIGNED ),
	SendPropBool(SENDINFO(m_bAutoReload) ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CDHL_Player, DT_DHL_Player)
	SendPropDataTable( "dhllocaldata", 0, &REFERENCE_SEND_TABLE(DT_DHLLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	SendPropInt( SENDINFO( m_iStuntState ), 4, SPROP_UNSIGNED ), //16
	SendPropInt( SENDINFO( m_iStuntDir ), 3, SPROP_UNSIGNED ), //8
	SendPropBool(SENDINFO(m_bProne) ),
END_SEND_TABLE()

BEGIN_DATADESC( CDHL_Player )
	DEFINE_FIELD( m_iItemFlags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bProne, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bProneStandReady, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsBleeding, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iSelectedInventoryValue, FIELD_INTEGER ),
	DEFINE_FIELD( m_bScoped, FIELD_BOOLEAN ),
END_DATADESC()

ConVar* pSlowVar;
CDHL_Player::CDHL_Player()
{
	vecSavedVelocity = vec3_origin;
	WeaponSelection.Clear();
	m_bAllowBumpPickup = false;
	m_bHasRespawned = false;

	//Bleeding
	m_bIsBleeding = false;
	m_flBleedTimer = 0.0f;
	m_bBandaging = false;
	m_flBandageEndTime = 0.0f;

	m_iDHLArmor = 0;
	m_bSpeedLocked = false;
	m_iQueuedItemFlags = 0;
	m_iItemFlags = 0;
	m_iStuntState = STUNT_NONE;
	m_iLMSWins = 0;

	m_iSelectedInventoryValue = 0;
	m_bScoped = false;
	m_iStylePoints = 0;
	m_bAutoReload = true;
	flLastTeamChange = 0.0f;

	m_flProneStandTime = 0.0f;
	m_iStuntDir = 0;
	m_flLastDiveYaw = 0.0f;

	m_iLastDamagedHitgroup = HITGROUP_GENERIC;
	m_bGibbing = false;
	m_flLastRespawn = 0.0f;
}

CDHL_Player::~CDHL_Player()
{
}

void CDHL_Player::Precache( void )
{
	BaseClass::Precache();

	//DHL - Skillet - Precache this just in case, as it's the fallback model if things fail
	//Other models are parsed and precached in CDHLGamerules::Precache() because this function gets called on every respawn
	//but the precaching only needs to be done once per map and a bunch of slow string operations are required to do ours
	PrecacheModel( "models/combine_soldier.mdl" );
}

void CDHL_Player::Spawn( void )
{
	if ( DHLShared::IsBackgroundMap() )
	{
		CBasePlayer::Spawn();
		g_pGameRules->GetPlayerSpawnSpot( this );
		return;
	}

	//Put them in spec if they're not allowed to spawn
	if ( !DHLRules()->AllowRespawn() )
	{
		StartObserverMode( OBS_MODE_ROAMING );
		return;
	}

	//Incase these got changed while dead for some reason, reset them again
	StopBleeding();

	m_bBandaging = false;
	m_bSpeedLocked = false;
	RemoveFlag( FL_PRONE );
	m_bProne = false;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &VEC_HULL_MIN, &VEC_HULL_MAX );
	m_iStuntState = STUNT_NONE;

	//!!!!!!!!!!!!!!! CALL BASECLASS SPAWN BEFORE GIVING ITEMS !!!!!!!!!!!!!!!
	//AddEffects( EF_NOINTERP ); //Disable interpolation to minimize player teleportation problem on respawn
	BaseClass::Spawn();

	//For akimbos //Mixed Akimbo
	//CreateViewModel(1);
	//GetViewModel(1)->AddEffects( EF_NODRAW );

	SetMaxSpeed( DHL_NORM_SPEED );
	m_flLastRespawn = gpGlobals->curtime;

	if ( DHLRules()->IsRoundRunning() )
		m_bHasRespawned = true;

	//Give them whatever they've selected
	if ( !IsObserver() )
	{
		StopObserverMode();
		State_Transition( STATE_ACTIVE );

		GiveSelectedItems();
		m_iItemFlags = m_iQueuedItemFlags;

		//DHL - Skillet - Armor
		if ( HasDHLArmor() )
			m_iDHLArmor = 100;
		else
			m_iDHLArmor = 0;
	}
	//HACK - Play weapon draw sound
	CDisablePredictionFiltering* disableFilter = new CDisablePredictionFiltering;
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	Assert( pWeapon );
	pWeapon->WeaponSound( DRAW );
	delete disableFilter;

	pSlowVar = cvar->FindVar( "dhl_slowmotion" );
	//Bots spam warnings on this
	if ( !IsBot() )
		m_bAutoReload = Q_atoi(engine->GetClientConVarValue( engine->IndexOfEdict(edict()), "cl_autoreload" )) > 0;
}

//This extern seems quite dangerous (could contain data for another player or just be invalid), 
//but valve does it in player.cpp as well as prediction and it seems to work here too
//extern CMoveData *g_pMoveData;
void CDHL_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	Activity actDesired = ACT_INVALID;
	//A bit hacky, this used to look for PLAYER_DIVE in playerAnim, but the baseclass think functions screw it up
	if ( GetStuntState() == STUNT_DIVE )
		actDesired = ACT_DHL_DIVE_GENERIC;
	if ( GetStuntState() == STUNT_PRONE )
	{
		//Check the sidemove to prevent the animation from drifting during the slide after dive landing
		if ( m_iStuntDir == STUNTDIR_FORWARDS && abs(m_flSideMove) >= DHL_WALK_SPEED * DHL_PRONESPEED_SCALAR )
			actDesired = ACT_DHL_PRONEMOVE_GENERIC;
		else
			actDesired = ACT_DHL_PRONE_GENERIC;
	}

	if ( actDesired != ACT_INVALID )
	{
		if ( GetActivity() == actDesired )
			return;
		SetActivity( actDesired );
		int animDesired = SelectWeightedSequence( GetActivity() );
		if ( GetSequence() == animDesired )
			return;
		ResetSequence( animDesired );
		SetCycle( 0.0f );
		return;
	}

	BaseClass::SetAnimation( playerAnim );
}

bool CDHL_Player::ClientCommand( const CCommand &args )
{
	//Drop weapon
	if ( !Q_stricmp( args[0], "DropPrimary" ) )
	{
		Weapon_Drop( GetActiveWeapon(), 0, 0);
		return true;
	}
	return BaseClass::ClientCommand( args );
}

extern int gEvilImpulse101;
bool CDHL_Player::BumpWeapon( CBaseCombatWeapon* pWeapon )
{
	if ( !(m_bAllowBumpPickup || gEvilImpulse101) )
	{
		CDHLBaseWeapon* pDHLWep = dynamic_cast<CDHLBaseWeapon*>(pWeapon);
		if ( !pDHLWep || !pDHLWep->AllowBumpAmmoPickup() )
			return false;
		//Grab ammo if we have this weapon already
		//HACK: considers only berettas for akimbo
		if ( ( Weapon_OwnsThisType( pDHLWep->GetClassname() ) && !pDHLWep->CanAkimbo() ) )
		{
			if ( GetAmmoCount( pDHLWep->GetPrimaryAmmoType() ) < GetAmmoDef()->MaxCarry( pDHLWep->GetPrimaryAmmoType() ) )
			{
				if ( Weapon_EquipAmmoOnly( pDHLWep ) )
					//Holy convoluted conditional
					//If the weapon does not have primary ammo but uses it (not melee) and it is either out of secondary ammo or does not use any, remove it.
					if ( ( !pDHLWep->HasPrimaryAmmo() && pDHLWep->UsesClipsForAmmo1() ) && ( !pDHLWep->HasSecondaryAmmo() || !pDHLWep->UsesClipsForAmmo2() ) )
						pDHLWep->Remove();
			}
		}

		/*char szAkimboName[DHL_WEPNAME_MAXLEN];
		Q_strncpy( szAkimboName, pDHLWep->GetClassname(), DHL_WEPNAME_MAXLEN );
		Q_strncat( szAkimboName, "_akimbo", sizeof(szAkimboName), 7 );
		DevMsg( "szAkimboName: %s\n", szAkimboName );*/

		//if ( FClassnameIs( pDHLWep, "weapon_beretta" ) && Weapon_OwnsThisType( "weapon_beretta_akimbo" ) )
		if ( pDHLWep->CanAkimbo() && Weapon_OwnsThisType( pDHLWep->GetAkimboEntClassname() ) )
		{
			if ( GetAmmoCount( pDHLWep->GetPrimaryAmmoType() ) < GetAmmoDef()->MaxCarry( pDHLWep->GetPrimaryAmmoType() ) )
			{
				int iAmt = GiveAmmo( pDHLWep->Clip1(), pDHLWep->GetPrimaryAmmoType(), false );
				pDHLWep->m_iClip1 -= iAmt;
				if ( ( !pDHLWep->HasPrimaryAmmo() && pDHLWep->UsesClipsForAmmo1() ) && ( !pDHLWep->HasSecondaryAmmo() || !pDHLWep->UsesClipsForAmmo2() ) )
					pDHLWep->Remove();
			}
		}
		return false;
	}
	return BaseClass::BumpWeapon( pWeapon );
}

void CDHL_Player::ChangeTeam( int iTeam )
{
	if ( iTeam != TEAM_SPECTATOR && DHLRules()->IsGameWaiting() && DHLRules()->IsRoundplay() && UTIL_PlayerByIndex(2) )
	{
		if ( !DHLRules()->IsTeamplay() ) //LMS
		{
			CTeam *pUnassigned = g_Teams[TEAM_UNASSIGNED];
			if ( pUnassigned && pUnassigned->GetNumPlayers() == 1 && iTeam == TEAM_UNASSIGNED )
				DHLRules()->SetFirstRoundStartTime( gpGlobals->curtime + 20.0f );
		}
		else //Team Roundplay
		{
			if ( iTeam == TEAM_MOBSTERS || iTeam == TEAM_PROS )
			{
				int iOtherTeam = iTeam == TEAM_MOBSTERS ? TEAM_PROS : TEAM_MOBSTERS;
				CTeam *pTargetTeam = g_Teams[iTeam];
				CTeam *pOtherTeam = g_Teams[iOtherTeam];
				if ( pTargetTeam && pOtherTeam )
				{
					if ( pTargetTeam->GetNumPlayers() == 0 && pOtherTeam->GetNumPlayers() > 0 )
						DHLRules()->SetFirstRoundStartTime( gpGlobals->curtime + 20.0f );
				}
			}
		}
	}

	BaseClass::ChangeTeam( iTeam );
}

CBaseEntity* CDHL_Player::EntSelectSpawnPoint( void )
{
	ConVarRef rrVarRef( "dhl_allowroundrespawns" );
	ConVarRef tsVarRef( "dhl_dynamicteamspawns" );
	//Stick to the hl2dm logic except for in team roundplay
	if ( !DHLRules()->IsTeamplay() || !DHLRules()->IsRoundplay() || !DHLRules()->IsRoundRunning() || rrVarRef.GetBool() || !tsVarRef.GetBool() ) 
		return BaseClass::EntSelectSpawnPoint();

	//This should never happen if CDHLRules::GetPlayerSpawnSpot works properly
	if ( (GetTeamNumber() == TEAM_PROS && DHLRules()->m_pProSpawn != NULL) || (GetTeamNumber() == TEAM_MOBSTERS && DHLRules()->m_pMobSpawn != NULL) )
		return NULL;

	CBaseEntity *pSpot = NULL;

	// Randomize the start spot
	for ( int i = random->RandomInt(1,12); i > 0; i-- )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
	}

	//The iterator hits NULL when it reaches the end so just grab the first one if that happens
	if ( !pSpot )
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
	if ( !pSpot ) //If still nothing then there evidently are no spawn entities
		return NULL;

	for ( int i = 0; i < 6; i++ )
	{
		if ( !pSpot )
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( !pSpot )
			return NULL;

		CBaseEntity *pOtherTeamSpot = (GetTeamNumber() == TEAM_MOBSTERS) ? DHLRules()->m_pProSpawn : DHLRules()->m_pMobSpawn;
		//Try to find primary spawns more than 10 ft. apart
		if ( pOtherTeamSpot && pOtherTeamSpot->GetAbsOrigin().DistTo( pSpot->GetAbsOrigin() ) < 120.0f )
		{
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
			continue;
		}
		if ( PosContainsPlayer( pSpot->GetAbsOrigin() ) )
		{
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
			continue;
		}
		break;
	}

	return pSpot;
}

CBaseEntity* CDHL_Player::GiveNamedItem( const char *szName, int iSubType )
{
	m_bAllowBumpPickup = true;
	CBaseEntity* pRet = BaseClass::GiveNamedItem( szName, iSubType );
	CBaseCombatWeapon* pWeapon = dynamic_cast<CBaseCombatWeapon*>( pRet );
	if ( pWeapon )
		BumpWeapon( pWeapon );
	m_bAllowBumpPickup = false;
	return pRet;
}

//DHL Bleeding
//-----------------------------------------------------------------------------------
// Decides upon taking damage whether or not the player should start bleeding
//-----------------------------------------------------------------------------------
bool CDHL_Player::ShouldBleed( const CTakeDamageInfo &inputInfo )
{
	//Not if we're already bleeding, or the damage event was fired by bleeding in progress
	if ( inputInfo.GetDamageType() == DMG_BLEED || m_bIsBleeding )
		return false;

	//Always if our health is/will be less than or equal to 10 
	if ( GetHealth() - inputInfo.GetDamage() <= 10 )
		return true;

	if ( inputInfo.GetDamage() > 15.0f && inputInfo.GetDamageType() & DMG_SLASH )
		return true;

	if ( inputInfo.GetDamage() > 32.0f && inputInfo.GetDamageType() & DMG_BULLET )
		return true;

	if ( inputInfo.GetDamage() > 50.0f && inputInfo.GetDamageType() & DMG_BLAST )
		return true;

	if ( inputInfo.GetDamage() > 65.0f && inputInfo.GetDamageType() & (DMG_FALL | DMG_CRUSH) )
		return true;

	return false;
}

const float DHL_BLEED_INTERVAL = 2.0f; //Seconds
const float DHL_BLEED_DAMAGE = 1.0f; //Damage points
//-----------------------------------------------------------------------------------
// Makes this player start bleeding
//-----------------------------------------------------------------------------------
void CDHL_Player::StartBleeding( const CTakeDamageInfo &inputInfo )
{
	m_BleedInputInfo = inputInfo;
	m_flBleedTimer = gpGlobals->curtime + DHL_BLEED_INTERVAL / DHLRules()->GetTimescale();

	//We've got a bleeder!
	m_bIsBleeding = true;
}

//-----------------------------------------------------------------------------------
// Called every BLEED_INTERVAL, handles dealing of bleeding damage
//-----------------------------------------------------------------------------------
void CDHL_Player::Bleed( void )
{
	CTakeDamageInfo BleedInfo;
	BleedInfo = m_BleedInputInfo;
	BleedInfo.SetDamageForce( vec3_origin );
	BleedInfo.SetDamage( DHL_BLEED_DAMAGE );
	BleedInfo.SetDamageType( DMG_BLEED );
	TakeDamage( BleedInfo );

	//Spawn some blood on the ground
	if ( GetGroundEntity() )
	{
		trace_t tr;
		//Grab a straight down vector
		Vector vecDown( 0, 0, -1 );
		UTIL_TraceLine( WorldSpaceCenter(), WorldSpaceCenter() + (vecDown * 100.0f), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		//DecalTrace( &tr, "Blood" );
		//DebugDrawLine( tr.startpos, tr.endpos, 0, 0, 255, false, 10.0f );
		if ( tr.DidHit() )
			UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED, true );
	}

	//Reset our timer (when this hits zero, this bleed function will be called again)
	m_flBleedTimer = gpGlobals->curtime + DHL_BLEED_INTERVAL / DHLRules()->GetTimescale();
}

//-----------------------------------------------------------------------------------
// Makes this player stop bleeding immediately
//-----------------------------------------------------------------------------------
void CDHL_Player::StopBleeding( void )
{
	m_bIsBleeding = false;
	m_flBleedTimer = 0.0f;
}

CPlayerWeaponSelection* CDHL_Player::GetWeaponSelection( void )
{
	return &WeaponSelection;
}

void CDHL_Player::Prone( void )
{
}

void CDHL_Player::UnProne( void )
{
}

ConVar dhl_spawnprotection_time( "dhl_spawnprotection_time", "2.0", FCVAR_CHEAT );
void CDHL_Player::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	//Track this for gibbing
	m_iLastDamagedHitgroup = ptr->hitgroup;
	CTakeDamageInfo info = inputInfo;
	
	//Spawn protection
	if ( (gpGlobals->curtime - m_flLastRespawn) < dhl_spawnprotection_time.GetFloat() )
	{
		info.ScaleDamage( 0.0f );
	}

	//Do armor stuff
	int iDmg = info.GetDamage();
	if ( m_takedamage && iDmg > 0 )
	{
		if ( ptr->hitgroup > HITGROUP_GENERIC )
		{
			CDHLBaseWeapon* pWeapon = NULL;
			if ( inputInfo.GetInflictor()->IsPlayer() ) //Must be hitscan
				pWeapon = dynamic_cast<CDHLBaseWeapon*>(ToBasePlayer( inputInfo.GetInflictor() )->GetActiveWeapon());
			else if ( inputInfo.GetInflictor() ) //Must be slow mo bullet/knife
			{
				CDHLProjectile* pProjectile = dynamic_cast<CDHLProjectile*>(inputInfo.GetInflictor());
				if ( pProjectile )
					pWeapon = dynamic_cast<CDHLBaseWeapon*>(pProjectile->m_pFiringWeapon);
			}

			if ( pWeapon )
			{
				switch ( ptr->hitgroup )
				{
					case HITGROUP_HEAD:
						info.ScaleDamage( pWeapon->GetHeadDamageScalar() );
						break;
					case HITGROUP_CHEST:
					case HITGROUP_STOMACH:
						info.ScaleDamage( pWeapon->GetBodyDamageScalar() );
						break;
					case HITGROUP_LEFTARM:
					case HITGROUP_RIGHTARM:
						info.ScaleDamage( pWeapon->GetArmDamageScalar() );
						break;
					case HITGROUP_LEFTLEG:
					case HITGROUP_RIGHTLEG:
						info.ScaleDamage( pWeapon->GetLegDamageScalar() );
						break;
					default:
						break;
				}
			}
		}

		if ( HasDHLArmor() && GetDHLArmor() > 0 )
		{
			if ( ptr->hitgroup == HITGROUP_CHEST || ptr->hitgroup == HITGROUP_STOMACH )
			{
				if ( !(info.GetDamageType() & (DMG_FALL | DMG_DROWN | DMG_POISON | DMG_RADIATION | DMG_BLEED)) ) //No armor protection from these types
				{
					float flArmorRequired = float(iDmg) * flHealthToArmorRatio;
					float flReductionPerc = float(GetDHLArmor()) / flArmorRequired;
					flReductionPerc = min( 1.0f, flReductionPerc ); //Capped at 1.0 (100%)
					info.ScaleDamage( flReductionPerc * flArmorProtection );

					SetDHLArmor( max(0, int(GetDHLArmor() - flArmorRequired)) );
				}
			}
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

int CDHL_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo newInfo = inputInfo;
	int iDmg = inputInfo.GetDamage(); //BaseClass::OnTakeDamage( inputInfo );
	if ( iDmg > 0 )
	{
		//Do armor stuff
		//if ( HasDHLArmor() && GetDHLArmor() > 0 )
		//{
		//	if ( !(inputInfo.GetDamageType() & (DMG_FALL | DMG_DROWN | DMG_POISON | DMG_RADIATION | DMG_BLEED)) ) //No armor protection from these types
		//	{
		//		float flArmorRequired = float(iDmg) * flHealthToArmorRatio;
		//		float flReductionPerc = float(GetDHLArmor()) / flArmorRequired;
		//		flReductionPerc = min( 1.0f, flReductionPerc ); //Capped at 1.0 (100%)
		//		newInfo.ScaleDamage( flReductionPerc * flArmorProtection );

		//		SetDHLArmor( max(0, int(GetDHLArmor() - flArmorRequired)) );
		//	}
		//}

		if ( ShouldBleed( newInfo ) )
		{
			StartBleeding( newInfo );
		}
	}
	return BaseClass::OnTakeDamage( newInfo );
}

ConVar dhl_always_decapitate( "dhl_always_decapitate", "0", FCVAR_CHEAT, "Test decapitation gibbing" );
void CDHL_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//For decapitation
	Vector vecEyePos = EyePosition();

	//Remove unwanted flags & state identifiers
	StopBleeding();
	RemoveFlag( FL_PRONE );
	m_bProne = false;
	m_iStuntState = STUNT_NONE;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &VEC_HULL_MIN, &VEC_HULL_MAX );

	bool bIgniteRagdoll = false;
	if ( IsOnFire() )
	{
		bIgniteRagdoll = true;
		CEntityFlame* pFlame = dynamic_cast<CEntityFlame*>(GetEffectEntity());
		if ( pFlame )
		{
			pFlame->SUB_Remove();
		}
	}

	//No screen fades after death
	color32 nothing = {0,0,0,255};
	UTIL_ScreenFade( this, nothing, 0, 0, FFADE_IN | FFADE_PURGE );

	//Distribute style points to our killer
	CDHL_Player* pAttacker = ToDHLPlayer(info.GetAttacker());
	if ( pAttacker && pAttacker != this )
	{
		//No style pts for killing teammates!
		if ( !DHLRules()->IsTeamplay() || pAttacker->GetTeamNumber() != GetTeamNumber() )
		{
			if ( !DHLRules()->IsGameWaiting() )
			{
				if ( pSlowVar && pSlowVar->GetBool() )
				{
					//TODO: Script these values
					if ( pAttacker->m_iStylePoints < DHL_MAXSTYLEPOINTS )
					{
						if ( pAttacker->GetStuntState() == STUNT_DIVE )
							pAttacker->m_iStylePoints += 3;
						else
							pAttacker->m_iStylePoints += 2;
						pAttacker->m_iStylePoints = min( pAttacker->m_iStylePoints, DHL_MAXSTYLEPOINTS );
					}
					if ( m_iStylePoints < DHL_MAXSTYLEPOINTS )
						m_iStylePoints += 1;
				}
			}
		}
	}

	BaseClass::Event_Killed( info );

	CBaseAnimating *pRagdoll = static_cast<CBaseAnimating*>( m_hRagdoll.Get() );
	if ( pRagdoll && bIgniteRagdoll )
	{
		if ( pRagdoll )
			pRagdoll->Ignite( 10.0f, false );
	}

	AddEffects( EF_NODRAW );

	//Gibbing
//#if 0
	if ( ( info.GetDamageType() & DMG_SLASH && m_iLastDamagedHitgroup == HITGROUP_HEAD ) || dhl_always_decapitate.GetBool() ) //Only doing decapitation for now
	{
		CEffectData data;
		data.m_nEntIndex = entindex(); //This is translated to client handle in the networking functions
		data.m_vOrigin = vecEyePos + Vector( 0, 0, 6.0f );
		//data.m_vAngles = GetLocalAngles();
		data.m_nDamageType = DMG_SLASH;
		//CreateRagGib( "models/player/dhl/mobsters/yakuza_bodygib.mdl", GetAbsOrigin(), GetAbsAngles(), GetAbsVelocity(), 10.0f );
		DispatchEffect( "DHLPlayerGib", data );
		pRagdoll->AddEffects( EF_NODRAW );
	}
//#endif
}

void CDHL_Player::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();
	
	if ( IsBot() && m_pPhysicsController )
		//Bot VPhysics fix
		//http://developer.valvesoftware.com/wiki/SDK_Known_Issues_List#Bot_physics.2Fhitbox_not_accurate
		UpdateVPhysicsPosition(m_vNewVPhysicsPosition, m_vNewVPhysicsVelocity, gpGlobals->frametime);
}

//extern ConVar forcerespawn; //In player.cpp
void CDHL_Player::PlayerDeathThink( void )
{
	BaseClass::PlayerDeathThink();
	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		ConVarRef forcerespawn( "mp_forcerespawn" );

		//Must press +attack or +attack2 to spawn
		if ( !((m_nButtons & IN_ATTACK) || (m_nButtons & IN_ATTACK2)) && !forcerespawn.GetBool() )
			return;

		//Packed all this stuff into a function
		AttemptRespawn();
	}
}

//extern ConVar flashlight;
void CDHL_Player::FlashlightTurnOn( void )
{
	ConVarRef flashlight( "mp_flashlight" );
	if ( (HasDHLFlashlight() || flashlight.GetBool()) && IsAlive() )
		AddEffects( EF_DIMLIGHT );
}

CON_COMMAND( dhl_doslowmo, "Start slow motion" )
{
	ConVarRef SlowVarRef( "dhl_slowmotion" );
	if ( !SlowVarRef.GetBool() )
		return;

	CDHL_Player* pDHLPlayer = ToDHLPlayer(UTIL_GetCommandClient());
	if ( pDHLPlayer )
	{
		if ( pDHLPlayer->m_iStylePoints >= DHL_SLOWMOSTYLEPOINTS )
		{
			if ( DHLRules()->InitSlowMotion( pDHLPlayer, 3.0f ) )
				pDHLPlayer->m_iStylePoints -= DHL_SLOWMOSTYLEPOINTS;
		}
	}
}

CON_COMMAND_F( dhl_givesp, "Give style points", FCVAR_CHEAT )
{
	CDHL_Player* pDHLPlayer = ToDHLPlayer(UTIL_GetCommandClient());
	if ( pDHLPlayer && args.ArgC() > 1 )
	{
		pDHLPlayer->m_iStylePoints += atoi(args.Arg(1));

		if ( pDHLPlayer->m_iStylePoints > DHL_MAXSTYLEPOINTS )
			pDHLPlayer->m_iStylePoints = DHL_MAXSTYLEPOINTS;
		if ( pDHLPlayer->m_iStylePoints < 0 )
			pDHLPlayer->m_iStylePoints = 0;
	}
}

void CDHL_Player::ImpulseCommands( void )
{
	BaseClass::ImpulseCommands();

	/*int iImpulse = (int)m_nImpulse;
	switch (iImpulse)
	{
	default:
		BaseClass::ImpulseCommands();
		break;
	}*/
}

void CDHL_Player::EquipSuit( bool bPlayEffects )
{
	if ( !DHLShared::IsBackgroundMap() )
		BaseClass::EquipSuit( bPlayEffects );
}

void CDHL_Player::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	//The target entity isn't technically the viewentity, even with OBS_MODE_IN_EYE, so PVS needs to be added manually here
	if ( !pViewEntity && IsObserver() && ( GetObserverMode() == OBS_MODE_IN_EYE || GetObserverMode() == OBS_MODE_CHASE ) && GetObserverTarget() )
	{
		engine->AddOriginToPVS( GetObserverTarget()->EyePosition() );
		return;
	}
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );
}

extern void respawn( CBaseEntity *pEdict, bool fCopyCorpse ); //In player.cpp
void CDHL_Player::AttemptRespawn( void )
{
	if ( !DHLRules()->AllowRespawn() )
		return;

	//if ( m_lifeState != LIFE_RESPAWNABLE )
		//return;

	if ( !g_pGameRules->IsMultiplayer() )
		return;

	//If we're on the spec team we're not going to spawn on click.
	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return;

	//Wait for the player to press a key to go to observer mode from ragdoll cam
	if ( (gpGlobals->curtime > (m_flDeathTime + DEATH_ANIMATION_TIME)) && !IsObserver() )
	{
		if ( DHLRules()->IsRoundplay() ) //Only if rounds are enabled.
		{
			State_Transition( STATE_OBSERVER_MODE );
			SetObserverMode( OBS_MODE_ROAMING );
			SetObserverTarget( NULL );
		}
	}

	m_nButtons = 0;
	m_iRespawnFrames = 0;

	//Now handled in Spawn()
	//Leave observer
	/*if ( GetObserverMode() )
	{
		StopObserverMode();
		State_Transition( STATE_ACTIVE );
	}*/

	respawn( this, !IsObserver() );// don't copy a corpse if we're in deathcam.

	SetNextThink( TICK_NEVER_THINK );
}

bool CDHL_Player::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	CDHLBaseWeapon* pDHLWep = dynamic_cast<CDHLBaseWeapon*>(pWeapon);
	if ( !pDHLWep )
		return false;

	if ( !pDHLWep->CanUse() )
		return false;

	if ( dhl_weaponrestrictions.GetBool() && pDHLWep->m_iInventoryValue + GetTotalInvVal() > DHL_INV_VAL_ABSMAX )
		return false;
	
	//NOTE: Picking up ammo is handled by bumpweapon
	if ( !pDHLWep->CanAkimbo() && Weapon_OwnsThisType( pDHLWep->GetClassname() ) )
		return false;

	//Mixed Akimbo
	/*bool bPossibleAkimbo = false;
	CDHLBaseWeapon* pOwnedWeapon = dynamic_cast<CDHLBaseWeapon*>(Weapon_OwnsThisType( pDHLWep->GetClassname() ));
	if ( pOwnedWeapon )
	{
		//Make sure both instances ok the akimboing
		if ( !pDHLWep->CanAkimbo() || !pOwnedWeapon->CanAkimbo() )
			return false;
		else
			bPossibleAkimbo = true;
	}

	if ( bPossibleAkimbo )
		return true;*/

	return BaseClass::Weapon_CanUse( pWeapon );
}

void CDHL_Player::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	//Disable akimbo for now 
	//Mixed Akimbo
	/*CDHLBaseWeapon* pDHLWep = dynamic_cast<CDHLBaseWeapon*>(pWeapon);
	Assert( pDHLWep );

	CDHLBaseWeapon* pOwnedWeapon = dynamic_cast<CDHLBaseWeapon*>(Weapon_OwnsThisType( pDHLWep->GetClassname() ));
	if ( pOwnedWeapon )
	{
		//Going akimbo
		pOwnedWeapon->StartAkimbo( pDHLWep, AKIMBOPOS_RIGHT );
		pDHLWep->StartAkimbo( pOwnedWeapon, AKIMBOPOS_LEFT );
		GetViewModel( 1 )->RemoveEffects( EF_NODRAW );
		BaseClass::Weapon_Equip( pWeapon );
		//Weapon_Switch( pOwnedWeapon, 1 );
		return;
	}*/

	Assert( pWeapon );
	CDHLBaseWeapon* pDHLWep = static_cast<CDHLBaseWeapon*>(pWeapon);

	//New simple and shitty akimbo code
	//if ( FClassnameIs( pWeapon, "weapon_beretta" ) )
	if ( pDHLWep->CanAkimbo() )
	{
		//Don't pick up more if we already have akimbos
		//CBaseCombatWeapon* pAkimbos = Weapon_OwnsThisType( "weapon_beretta_akimbo" );
		CBaseCombatWeapon* pAkimbos = Weapon_OwnsThisType( pDHLWep->GetAkimboEntClassname() );
		if ( pAkimbos && !pAkimbos->IsMarkedForDeletion() ) //I probably deserve to die for this
			return;

		CDHLBaseWeapon* pOwnedWeapon = static_cast<CDHLBaseWeapon*>(Weapon_OwnsThisType( pDHLWep->GetClassname() ));
		if ( pOwnedWeapon )
		{
			//pOwnedWeapon->AddEffects( EF_NODRAW );
			//Weapon_Drop( pOwnedWeapon, NULL, NULL );
			pOwnedWeapon->Remove();

			//CDHLBaseWeapon* pNewWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( "weapon_beretta_akimbo" ));
			CDHLBaseWeapon* pNewWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( pDHLWep->GetAkimboEntClassname() ));
			pNewWep->m_iClip1 = pDHLWep->Clip1() + pOwnedWeapon->Clip1();
			Weapon_Switch( pNewWep );
			pDHLWep->Remove(); //Get rid of the one on the ground
			return;
		}
	}

	BaseClass::Weapon_Equip( pWeapon );
}

void CDHL_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	CDHLBaseWeapon* pDHLWep = dynamic_cast<CDHLBaseWeapon*>(pWeapon);
	//This used to be an assert, but the bots can run this code without weapons and cause crashes.
	if ( !pDHLWep )
		return;
	//Assert( pDHLWep );

	if ( pWeapon->GetWeaponFlags() & ITEM_FLAG_NODROP )
		return;

	//if ( FClassnameIs( pWeapon, "weapon_beretta_akimbo" ) )
	if ( pDHLWep->IsAkimboEnt() )
	{
		pWeapon->Remove();

		//Need two new single berettas, one to keep and one to drop
		//CDHLBaseWeapon* pDropWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( "weapon_beretta" ));
		CDHLBaseWeapon* pDropWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( pDHLWep->GetSingleEntClassname() ));
		BaseClass::Weapon_Drop( pDropWep, pvecTarget, pVelocity );
		//CDHLBaseWeapon* pNewWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( "weapon_beretta" ));
		CDHLBaseWeapon* pNewWep = static_cast<CDHLBaseWeapon*>(GiveNamedItem( pDHLWep->GetSingleEntClassname() ));

		//Figure out how much ammo should be in each magazine...
		pDropWep->m_iClip1 = (pWeapon->Clip1() / 2);
		if ( pWeapon->Clip1() % 2 == 0 ) //Assume the one we're keeping has more ammo
			pNewWep->m_iClip1 = (pWeapon->Clip1() / 2);
		else
			pNewWep->m_iClip1 = (pWeapon->Clip1() / 2) + 1;

		if ( m_lifeState == LIFE_DYING ) //HACK: drop both if dead
		{
			BaseClass::Weapon_Drop( pNewWep );
			return;
		}

		Weapon_Switch( pNewWep );
		
		//pWeapon = static_cast<CDHLBaseWeapon*>(CreateEntityByName( "weapon_beretta" ));
		return;
	}

	if ( pDHLWep->DropsAmmoIndividually() )
	{
		//Check for extra ammo
		int iAmmo = GetAmmoCount(pDHLWep->GetPrimaryAmmoType());
		if ( iAmmo > 0 )
		{
			RemoveAmmo( 1, pDHLWep->GetPrimaryAmmoType() );
			CBaseCombatWeapon* pNewWeapon = assert_cast<CBaseCombatWeapon*>(CreateEntityByName( pDHLWep->GetClassname() ));
			pNewWeapon->AddSpawnFlags( SF_NORESPAWN );
			Vector vThrowPos = Weapon_ShootPosition() - Vector(0,0,12);
			pNewWeapon->SetAbsOrigin( vThrowPos );

			//Throw it - the velocity bits are copied from basecombatcharacter.cpp
			Vector vecThrow = vec3_origin;
			if (pvecTarget)
			{
				// I've been told to throw it somewhere specific.
				vecThrow = VecCheckToss( this, pDHLWep->GetAbsOrigin(), *pvecTarget, 0.2, 1.0, false );
			}
			else
			{
				if ( pVelocity )
				{
					vecThrow = *pVelocity;
					float flLen = vecThrow.Length();
					if (flLen > 400)
					{
						VectorNormalize(vecThrow);
						vecThrow *= 400;
					}
				}
				else
				{
					// Nowhere in particular; just drop it.
					float throwForce = 400.0f;
					vecThrow = BodyDirection3D() * throwForce;
				}
			}
			DispatchSpawn( pNewWeapon );
			pNewWeapon->FallInit();

			IPhysicsObject *pObj = pNewWeapon->VPhysicsGetObject();
			if ( pObj )
			{
				AngularImpulse	angImp( 200, 200, 200 );
				pObj->AddVelocity( &vecThrow, &angImp );
			}
			else
				pNewWeapon->SetAbsVelocity( vecThrow );
			return; //Done
		}
	}

	/*if ( pDHLWep->IsAkimbo() ) //Mixed Akimbo
	{
		pDHLWep->EndAkimbo(); //This handles shutting down akimbo for both this weapon and it's paired weapon
		GetViewModel( 1 )->AddEffects( EF_NODRAW );
	}*/

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if ( !GetActiveWeapon() )
	{
		CBaseCombatWeapon *pLastWeapon = GetWeapon(0);
		if ( pLastWeapon )
			Weapon_Switch( pLastWeapon );
	}
}

void CDHL_Player::GiveSelectedItems( void )
{
	int iMaxInvVal = -1;
	CDHLBaseWeapon* pSwitchTo = NULL;
	for ( int i = 0; i < GetWeaponSelection()->GetNumWeapons(); i++ )
	{
		const char* szWep = GetWeaponSelection()->GetWeaponByIndex( i );
		if ( szWep )
		{
			//I'm using static_cast here because it shouldn't be possible to select a weapon from the menu not derived from CDHLBaseWeapon
			CDHLBaseWeapon* pWeapon = static_cast<CDHLBaseWeapon*>(Weapon_OwnsThisType( szWep ));
			if ( pWeapon )
			{
				//This happens with the combat knife specfically because it is given by default, selecting in menu it should just give more ammo
				CBasePlayer::GiveAmmo( pWeapon->GetWpnData().iDefaultAmmoPrimary, pWeapon->GetPrimaryAmmoType() );
				CBasePlayer::GiveAmmo( pWeapon->GetWpnData().iDefaultAmmoSecondary, pWeapon->GetSecondaryAmmoType() );
				continue;
			}

			pWeapon = static_cast<CDHLBaseWeapon*>(GiveNamedItem( szWep ));
			if ( pWeapon )
			{
				//if ( bWepSwitch && i == 0 )
					//Weapon_Switch( pWeapon );
				if ( pWeapon->m_iInventoryValue > iMaxInvVal ) //Keep track of the weapon with the highest selection index
				{
					iMaxInvVal = pWeapon->m_iInventoryValue;
					pSwitchTo = pWeapon;
				}

				CBasePlayer::GiveAmmo( pWeapon->GetWpnData().iDefaultAmmoPrimary, pWeapon->GetPrimaryAmmoType() );
				CBasePlayer::GiveAmmo( pWeapon->GetWpnData().iDefaultAmmoSecondary, pWeapon->GetSecondaryAmmoType() );
			}
		}
	}
	if ( pSwitchTo )
		Weapon_Switch( pSwitchTo );
}

const char* szDefaultModel = "models/humans/group03/male_01.mdl";
void CDHL_Player::SetPlayerModel( void )
{
	MDLCACHE_CRITICAL_SECTION(); //To fix assert
	BaseClass::SetPlayerModel();
	//const char* szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	//bool bFound = filesystem->FileExists( szModelName, "MOD" );
	//if ( bFound )
	//	//We found the model
	//	SetModel( szModelName );
	//else
	//{
	//	//We didn't find the model
	//	Warning( "Model \"%s\" not found.", szModelName );
	//	SetModel( szDefaultModel );
	//}
}

//void CDHL_Player::Touch( CBaseEntity *pOther )
//{
//	if ( pOther == GetGroundEntity() )
//		return;
//
//	//DHL - Break glass if hit hard enough
//	//Not if we're nonsolid
//	if ( GetMoveType() != MOVETYPE_NOCLIP && GetMoveType() != MOVETYPE_OBSERVER && GetMoveType() != MOVETYPE_NONE && GetSolid() != SOLID_NONE )
//	{
//		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
//		{
//			if ( abs(GetAbsVelocity().x) > 225.0f || abs(GetAbsVelocity().y) > 225.0f || abs(GetAbsVelocity().x) > 225.0f )
//			{
//				//Save our pre-touch velocity
//				vecSavedVelocity = GetAbsVelocity();
//
//				Vector vecDir = GetAbsVelocity();
//				VectorNormalize( vecDir );
//				trace_t tr;
//				UTIL_TraceLine( GetAbsOrigin(), pOther->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
//				if ( strcmp( pOther->GetClassname(), "func_breakable" ) == 0 || strcmp( pOther->GetClassname(), "func_breakable_surf" ) == 0 )
//				{
//					if ( strcmp( pOther->GetClassname(), "func_breakable_surf" ) == 0 )
//					{
//						//We do NOT want to do the damage or velocity restoration hacks if we're touching a pane of surf
//						//glass that is already broken
//						CBreakableSurface *pSurfGlass = dynamic_cast<CBreakableSurface*>( pOther );
//						if ( pSurfGlass && pSurfGlass->IsBroken() )
//							return;
//					}
//					if ( GetGroundEntity() == NULL )
//					{
//						CTakeDamageInfo glassinfo = CTakeDamageInfo( this, this, 5.0f, DMG_CRUSH );
//						glassinfo.SetDamagePosition( tr.endpos );
//						glassinfo.SetDamageForce( GetAbsVelocity() );
//						pOther->DispatchTraceAttack( glassinfo, vecDir, &tr );
//						return; //No physics dealings here
//					}
//				}
//			}
//		}
//	}
//	BaseClass::Touch( pOther );
//}

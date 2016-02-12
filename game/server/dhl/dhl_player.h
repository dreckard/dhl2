//=============================================================================//
// Purpose:	Distraction Half-Life 2 player entity
//
// Author: Skillet
//=============================================================================//
#ifndef DHL_PLAYER_H
#define DHL_PLAYER_H

#include "hl2mp_player.h"
#include "dhl/dhl_shareddefs.h"
#include "dhl/dhl_gamerules.h"

class CDHL_Player : public CHL2MP_Player
{
public:
	DECLARE_CLASS( CDHL_Player, CHL2MP_Player );

	CDHL_Player();
	~CDHL_Player( void );

	static CDHL_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CDHL_Player::s_PlayerEdict = ed;
		CDHL_Player* pPlayer = (CDHL_Player*)CreateEntityByName( className );
		return pPlayer;
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	/*virtual*/ void Precache( void );
	/*virtual*/ void Spawn( void );

	/*virtual*/ void PreThink( void ); //Shared
	/*virtual*/ void PostThink( void ); //Shared

	/*virtual*/ void SetAnimation( PLAYER_ANIM playerAnim );

	/*virtual*/ void FireBullets( const FireBulletsInfo_t &info );

	/*virtual*/ bool ClientCommand( const CCommand &args );
	/*virtual*/ bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	/*virtual*/ void ChangeTeam( int iTeam );
	/*virtual*/ CBaseEntity* EntSelectSpawnPoint( void );
	bool Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon) { if (m_bBandaging) return false; return BaseClass::Weapon_CanSwitchTo(pWeapon); }
	void Weapon_Equip( CBaseCombatWeapon *pWeapon );
	void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 ); //Shared

	void Prone( void ); //Called when the player goes prone (immediately on stunt)
	void UnProne( void ); //Called when the player leaves prone

	void TimescaleChanged( float flNewTimescale, float flOldTimescale );

	CPlayerWeaponSelection*	GetWeaponSelection( void );
	
	void AddItems( int iFlags ) { m_iQueuedItemFlags |= iFlags; }
	void RemoveItems( int iFlags ) { m_iQueuedItemFlags &= ~iFlags; }
	void ToggleItem( int iFlag ) { m_iQueuedItemFlags ^= iFlag; }
	void ClearItems( void ) { m_iItemFlags = 0; m_iQueuedItemFlags = 0; }
	void SetItems( int iFlags ) { m_iItemFlags = iFlags; m_iQueuedItemFlags = iFlags; }

	bool ShouldBleed( const CTakeDamageInfo &inputInfo );
	void StartBleeding( const CTakeDamageInfo &inputInfo );
	void Bleed( void ); //Applies bleeding damage at intervals
	void Bandage( void ); //Player applies bandages to wounds (takes time) //Shared
	void StopBleeding( void ); //Stop bleeding instantly
	void FireBullet( const FireBulletsInfo_t info ); //Fire a physics bullet

	/*virtual*/ CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 );

	/*virtual*/ int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	/*virtual*/ void TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr );

	/*virtual*/ void Event_Killed( const CTakeDamageInfo &info );

	///*virtual*/ void Touch( CBaseEntity *pOther ); //Shared
	/*virtual*/ bool ShouldTouch( CBaseEntity *pOther, trace_t &trace ); //Shared
	/*virtual*/ void PhysicsSimulate( void );
	/*virtual*/ void SetMaxSpeed( float flMaxSpeed ); //Shared

	/*virtual*/ void PlayerDeathThink( void );
	/*virtual*/ void FlashlightTurnOn( void );
	/*virtual*/ void ImpulseCommands( void );
	/*virtual*/ void EquipSuit( bool bPlayEffects = true );
	/*virtual*/	void SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize );
	void AttemptRespawn( void );

#pragma warning(disable: 4800) // Disable retarded int to bool performance warnings
	bool HasDHLFlashlight( void ) { return m_iItemFlags & DHL_IFLAG_FLASHLIGHT; }
	bool HasDHLArmor( void ) { return m_iItemFlags & DHL_IFLAG_KEVLAR; }
#pragma warning(default: 4800) // Enable retarded int to bool performance warnings
	int GetDHLArmor( void ) { return m_iDHLArmor; }
	void SetDHLArmor( int iNewArmor ) { m_iDHLArmor = iNewArmor; }

	int GetWeaponInvVal( int iWeapon );
	int GetTotalInvVal( void );
	void GiveSelectedItems( void );

	void SetPlayerModel( void ); //DHL - Skillet - Virtual

	//Used to check suitability of a position for spawning
	bool TestPlayerBBox( const Vector& pos )
	{
		trace_t pm;
		Ray_t ray;
		ray.Init( pos, pos, VEC_HULL_MIN, VEC_HULL_MAX );
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &pm );

		if ( !pm.DidHit() )
		{
			CBaseEntity* ent = NULL;
			edict_t* player = edict();
			for ( CEntitySphereQuery sphere( pos, 16 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
			{
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					return false;
			}
			return true;
		}

		return false;
	}
	bool PosContainsPlayer( const Vector& vPos )
	{
		edict_t* player = edict();
		CBaseEntity* ent = NULL;
		for ( CEntitySphereQuery sphere( vPos, 64 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			if ( ent->IsPlayer() && !(ent->edict() == player) )
			{
				if ( static_cast<CDHL_Player*>(ent)->m_bHasRespawned )
					return true;
			}
		}
		return false;
	}

	//Inherited crap
	void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float flvol, bool force )
	{ 
		if ( m_bProne )
			return;
		else 
			BaseClass::PlayStepSound( vecOrigin, psurface, flvol, force ); 
	}
	bool CanSprint( void );

	virtual const Vector GetPlayerMins( void ) const
	{
		if ( m_bProne )
			return VEC_PRONE_HULL_MIN;

		return BaseClass::GetPlayerMins();
	}
	virtual const Vector GetPlayerMaxs( void ) const
	{
		if ( m_bProne )
			return VEC_PRONE_HULL_MAX;

		return BaseClass::GetPlayerMaxs();
	}

	void SetStuntState( int iStuntState ) { m_iStuntState = iStuntState; }
	int GetStuntState( void ) { return m_iStuntState; }

	bool m_bAllowBumpPickup;
	bool m_bHasRespawned; //Applies only to team roundplay

	int m_iQueuedItemFlags; //To be applied on next respawn
	CNetworkVar( int, m_iItemFlags );
	CNetworkVar( int, m_iDHLArmor );
	CNetworkVar( bool, m_bProne );
	CNetworkVar( bool, m_bIsBleeding );
	CNetworkVar( bool, m_bProneStandReady );
	CNetworkVar( int, m_iStuntState );
	CNetworkVar( int, m_iSelectedInventoryValue );
	CNetworkVar( bool, m_bScoped );
	CNetworkVar( int, m_iStylePoints );
	CNetworkVar( int, m_iStuntDir );
	CNetworkVar( bool, m_bAutoReload );

	float flLastTeamChange; //Tracks time since player last changed teams

	//Per-player gamemovement crap
	float m_flProneStandTime;
	float m_flLastDiveYaw;

	int m_iLMSWins; //Networked for scoreboard through CPlayerResource

private:
	Vector vecSavedVelocity;
	CPlayerWeaponSelection WeaponSelection;

	//Bleeding
	CTakeDamageInfo m_BleedInputInfo;
	float m_flBleedTimer;
	bool m_bBandaging;
	bool m_bSpeedLocked;
	float m_flBandageEndTime;
	int m_iLastDamagedHitgroup;
	bool m_bGibbing;
	float m_flLastRespawn;
};

inline CDHL_Player *ToDHLPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return static_cast<CDHL_Player*>( pEntity ); //Should be safe to static cast any CBasePlayer (IsPlayer()==true)
}

/*inline CDHL_Player *ToDHLPlayer( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	return static_cast<CDHL_Player*>( pPlayer );
}*/

#endif
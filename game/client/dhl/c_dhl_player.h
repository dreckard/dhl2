//=============================================================================//
// Purpose:	Distraction Half-Life 2 player entity
//
// Author: Skillet
//=============================================================================//
#ifndef C_DHL_PLAYER_H
#define C_DHL_PLAYER_H
#pragma once

#include "c_hl2mp_player.h"
#include "c_dhl_projectile.h"
#include "dhl/dhl_gamerules.h"

//Constants
const int PCHANGE_TOTHIRDPERSON = 0;
const int PCHANGE_TOFIRSTPERSON = 1;

class C_DHL_Player : public C_HL2MP_Player
{
public:
	DECLARE_CLASS( C_DHL_Player, C_HL2MP_Player );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_DHL_Player();
	~C_DHL_Player( void );

	/*virtual*/ void Precache( void );
	/*virtual*/ void PreThink( void ); //Shared
	/*virtual*/ void PostThink( void ); //Shared
	/*virtual*/ void SetMaxSpeed( float flMaxSpeed ); //Shared
	/*virtual*/ void PostDataUpdate( DataUpdateType_t updateType );
	/*virtual*/ bool ShouldDraw( void );
	/*virtual*/ int DrawModel( int flags );

	/*virtual*/ void FireBullets( const FireBulletsInfo_t &info );

	//virtual void Touch( CBaseEntity *pOther ); //Shared
	/*virtual*/ bool ShouldTouch( CBaseEntity *pOther, trace_t &trace ); //Shared

	void Bandage( void ); //Shared
	bool Weapon_CanSwitchTo(C_BaseCombatWeapon *pWeapon) { if (m_bBandaging) return false; return BaseClass::Weapon_CanSwitchTo(pWeapon); }
	bool Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 ); //Shared

	void FireBullet( const FireBulletsInfo_t info ); //Fire a physics bullet - Shared

	void TimescaleChanged( float flNewTimescale, float flOldTimescale );
	void SetAnimation( PLAYER_ANIM playerAnim );

	Activity GetActivity() const	{ return m_Activity; }
	inline void SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	Activity TranslateTeamActivity( Activity ActToTranslate );
	Activity Weapon_TranslateActivity( Activity baseAct, bool *pRequired = NULL );

	void EnableNightvision( bool bEnable ) { m_bNightvision = bEnable; }
	bool GetNightvisionEnabled( void ) { return m_bNightvision; }
	int GetDHLArmor( void ) { return m_iDHLArmor; }

	void SetStuntState( int iStuntState ) { m_iStuntState = iStuntState; }
	int GetStuntState( void ) { return m_iStuntState; }

	void UpdateGradVAngle( void );
	void GradVAngleChange( QAngle angAngles, float flTime );

	static int GetWeaponInvVal( int iWeapon );
	int GetTotalInvVal( void );

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

	int m_iDHLArmor;
	int m_iItemFlags;
	int m_iStuntState;
	bool m_bIsBleeding;
	bool m_bProne;
	bool m_bProneStandReady;
	QAngle m_angGradVAngle;
	float m_flGradVAngleTime;
	int m_iSelectedInventoryValue;
	bool m_bScoped;
	int m_iStylePoints;
	bool m_bAutoReload;
	float m_flLastTimescaleChange;

	//Per-player gamemovement crap
	float m_flProneStandTime;
	int m_iStuntDir;
	float m_flLastDiveYaw;

	void PerspectiveChanged( int iType );
	void ClientRespawn( void );
	void ClientDeath( void );

	//Gibs
	bool GetHeadGib( char* pDst, int iLen );
	bool GetBodyGib( char* pDst, int iLen );
	C_HL2MPRagdoll* GetHL2MPRagdoll( void );
	bool HideRagdoll( void );
	bool SetRagdollModel( const char* szMdlName );

	static void CC_ToggleNightvision( void );

	//DHL - Skillet - Blurring
	struct BlurInfo_t
	{
		bool bBlur;
		float flMax;
		float flMin;
		float flInterval;
		float flFraction; //Current health / effects health

		//Constructor
		BlurInfo_t()
		{
			bBlur = false;
			flMax = 0.005f;
			flMin = 0.001f;
			flInterval = 5.0f;
			flFraction = 1.0f;
		}
	} blurInfo;

private:
	Vector vecSavedVelocity;
	bool m_bBandaging;
	bool m_bSpeedLocked;
	bool m_bNightvision;
	float m_flBandageEndTime;
	bool m_bWantsThirdPerson;
	bool m_bWantsNV;
	float m_flGradVAngleTimeElapsed;

	Activity m_Activity;

	/*CUtlVector< C_DHLBullet* > m_ActiveBulletsList;
	friend class C_DHLBullet;*/
};

inline C_DHL_Player *ToDHLPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return static_cast<C_DHL_Player*>( pEntity ); //Should be safe to static cast any C_BasePlayer (IsPlayer()==true)
}

/*inline C_DHL_Player *ToDHLPlayer( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	return static_cast<C_DHL_Player*>( pPlayer );
}*/

#endif //C_DHL_PLAYER_H

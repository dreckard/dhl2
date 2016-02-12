//=============================================================================//
// Purpose:	Distraction Half-Life 2 Gamerules entity
//
// Author: Skillet
//=============================================================================//
#ifndef DHL_GAMERULES_H
#define DHL_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif
#include "hl2mp_gamerules.h"

#ifdef GAME_DLL
	#include "dhl/dhl_maprules.h"
	#include "dhl/dhl_mapfilter.h"
	class CDHL_Player; //Forward declaration to avoid circular dependency with dhl_player.h
#else
	#include "c_hl2mp_player.h"
#endif

//DHL Round end conditions
const int ROUNDCONDIT_NONE = 0;
const int ROUNDCONDIT_DRAW = 1;
const int ROUNDCONDIT_TEAMWIN_MOB = 2;
const int ROUNDCONDIT_TEAMWIN_PRO = 3;
const int ROUNDCONDIT_LMSWIN = 4;

//DHL - Skillet - Event Messages
const byte DHL_MSG_ROUNDRESTART = 0;
const byte DHL_MSG_TIMESCALECHANGE = 1;

//Respawn "flags"
const int DHL_RESPAWN_DEFAULT = 0;
const int DHL_RESPAWN_FORCEALLOW = 1;
const int DHL_RESPAWN_FORCEDISABLE = 2;

//Game modes
const int DHL_GAMEMODE_DEATHMATCH = 0;
const int DHL_GAMEMODE_TEAMDEATHMATCH = 1;
const int DHL_GAMEMODE_LASTMANSTANDING = 2;
const int DHL_GAMEMODE_TEAMROUNDPLAY = 3;

const int TEAM_CMD_SPEC = 0; //Spectator
const int TEAM_CMD_DEFAULT = 1; //Team for players in deathmatch
const int TEAM_CMD_MOB = 2;
const int TEAM_CMD_PROS = 3;
const int TEAM_CMD_AUTOASSIGN = 4;

#define VEC_PRONE_HULL_MIN	DHLRules()->GetDHLViewVectors()->m_vProneHullMin
#define VEC_PRONE_HULL_MAX	DHLRules()->GetDHLViewVectors()->m_vProneHullMax
#define VEC_PRONE_VIEW		DHLRules()->GetDHLViewVectors()->m_vProneView

#define VEC_PRONE_TR_MIN	DHLRules()->GetDHLViewVectors()->m_vProneTRMin
#define VEC_PRONE_TR_MAX	DHLRules()->GetDHLViewVectors()->m_vProneTRMax
#define VEC_STUNT_TR_MIN	DHLRules()->GetDHLViewVectors()->m_vStuntTRMin
#define VEC_STUNT_TR_MAX	DHLRules()->GetDHLViewVectors()->m_vStuntTRMax

#ifdef CLIENT_DLL
	#define CDHLRules C_DHLRules
	#define CDHLGameRulesProxy C_DHLGameRulesProxy
#endif

//Used for networking
class CDHLGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CDHLGameRulesProxy, CHL2MPGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	void RemoveClientEntities( void );
	virtual void ReceiveMessage( int classID, bf_read &msg );
#endif
};

class DHLViewVectors : public HL2MPViewVectors
{
public:
	DHLViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,

		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax,
		Vector vProneHullMin,
		Vector vProneHullMax,
		Vector vProneView,
		Vector vProneTRMin,
		Vector vProneTRMax,
		Vector vStuntTRMin,
		Vector vStuntTRMax,
		Vector vDiveHullMin,
		Vector vDiveHullMax) :
			HL2MPViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,

				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight,
				vCrouchTraceMin,
				vCrouchTraceMax)
	{
		m_vProneHullMin = vProneHullMin;
		m_vProneHullMax = vProneHullMax;
		m_vProneView = vProneView;
		m_vProneTRMin = vProneTRMin;
		m_vProneTRMax = vProneTRMax;
		m_vStuntTRMin = vStuntTRMin;
		m_vStuntTRMax = vStuntTRMax;
		m_vDiveHullMin = vDiveHullMin;
		m_vDiveHullMax = vDiveHullMax;
	}

	Vector m_vProneHullMin;
	Vector m_vProneHullMax;
	Vector m_vProneView;
	Vector m_vProneTRMin;
	Vector m_vProneTRMax;
	Vector m_vStuntTRMin;
	Vector m_vStuntTRMax;
	Vector m_vDiveHullMin;
	Vector m_vDiveHullMax;
};

class CDHLRules : public CHL2MPRules
{
public:
	DECLARE_CLASS( CDHLRules, CHL2MPRules );
	#ifdef CLIENT_DLL
		DECLARE_CLIENTCLASS_NOBASE();
	#else
		DECLARE_SERVERCLASS_NOBASE();
	#endif
	CDHLRules();
	~CDHLRules();
	virtual void CreateStandardEntities( void );

	const char** FindModels( const char* searchString );
	virtual void Precache( void );

	//DHL: IMPORTANT!! ENCRYPTION KEY!
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"bobross4"; }
	virtual const char *GetGameDescription( void ); //Shown in server browser
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return FALSE; } //No death screenfade
	virtual int	  Damage_GetTimeBased( void );
	virtual int   Damage_GetShowOnHud( void );

	virtual void Think( void );

	const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;
	const DHLViewVectors* GetDHLViewVectors() const;

	#ifndef CLIENT_DLL
		void SetTimescale( float flNewTimescale );
		bool InitSlowMotion( CDHL_Player* pInitiator, float flLength );
		void SlowMotionThink( void );
		void RemoveClientEntities( void );
		void SignalTimescaleChange( float flNewTimescale );

		void InitRoundSystem( void );
		void RoundThink( void );
		void RoundThink_Teamplay( void );
		void CheckPlayers_RoundTeamplay( void );
		void RoundThink_LMS( void );
		void CheckPlayers_LMS( void );
		void UpdateMaprules( void );
		void DecrementTimers( void );
		void CheckRoundClock( void );
		void StartRound( float flLength );
		void EndRound( int iDelay, bool bNotify );
		void DrawRoundHUD( void );

		virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );

		virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
		virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
		virtual CBaseEntity* GetPlayerSpawnSpot( CBasePlayer* pPlayer );
		bool SpawnNearby( CBaseEntity* pSpawnPoint, CDHL_Player* pPlayer );
		Vector FindNearbySpawnablePoint( CBaseEntity* pSpawnEnt, const Vector& vPos, CDHL_Player* pPlayer );
		bool TestSpawnablePoint( const Vector& vOrigPos, const Vector& vTestPos, CDHL_Player* pPlayer );
		void SetupSpawn( CBasePlayer* pPlayer, const Vector& vPos, const QAngle& angDir );

		CMapEntityFilter m_filter;
		int m_iDeadPlayers;
		int m_iTotalPlayers;

		CBaseEntity	*m_pMobSpawn;
		CBaseEntity	*m_pProSpawn;
	#else
		void TimescaleChangedClient( float flNewTimescale, float flOldTimescale );
		CUtlVector<C_HL2MPRagdoll*> m_hRagdollList; //List of player ragdolls, so the last one can be removed if necessary
	#endif

	void RestartRound( void );
	int GetGameMode( void ) { return m_iGameMode; }
	bool IsRoundplay( void ) { return (m_iGameMode == DHL_GAMEMODE_LASTMANSTANDING || m_iGameMode == DHL_GAMEMODE_TEAMROUNDPLAY); }
	virtual bool IsTeamplay( void ) { return (m_iGameMode == DHL_GAMEMODE_TEAMDEATHMATCH || m_iGameMode == DHL_GAMEMODE_TEAMROUNDPLAY); }
	bool IsGameWaiting( void ) { return m_bGameWaiting; }
	bool IsRoundRunning( void ) { return (m_bRoundRunning && IsRoundplay()); }

#ifndef CLIENT_DLL
	float GetRoundTimeLeft( void ) { return m_flRoundTimeLeft; }
#endif
	int GetNetRoundTimeLeft( void ) { return m_iNetRoundTimeLeft; } //Integer for faster networking

	bool AllowRespawn( void );
	bool GetGrayscale( void );
	void SetGrayscale( bool bNewGrayscale );
	float GetTimescale( bool bRaw = false );

	void SetFirstRoundStartTime( float flTime ) { m_flFirstRoundStartTime = flTime; }
	float GetFirstRoundStartTime( void ) { return m_flFirstRoundStartTime; }

	const char** MobModels;
	const char** ProModels;

	CNetworkVar( int, m_iTimeToRoundStart );

private:
	CNetworkVar( int, m_iGameMode );

	//This bGameWaiting networking dosen't actually do anything at the moment,
	//But it will be necessary if (when?) we need to check for gamewaiting from
	//The client side (When we install a HUD instead of UTIL_ClientPrintAll)

	CNetworkVar( bool, m_bGameWaiting ); //Is the game waiting for additional players?
	CNetworkVar( bool, m_bGrayscale ); //Grayscale shader status
	CNetworkVar( int, m_iNetRoundTimeLeft ); //Round time left, for HUD clocks
	CNetworkVar( float, m_flTimescalar ); //Time scalar, for slow motion

	float m_flRoundTimeLeft;

	int m_iRoundCondition; //Used for holding messages on screen
	bool m_bRoundRunning; //Is a round in progress right now?
	float m_flRoundEndTime; //When this round will end
	float m_flRoundStartTime; //When this round started
	float m_flRoundRestartTime; //When the round should restart
	float m_flDHL_Roundtime; //The value of dhl_roundtime
	float m_flNextRoundThink; //Next RoundThink() call
	float m_flFirstRoundStartTime;
	char cLMSLivePlayer[256]; //Name of a living player during a Last Man Standing game
	CBasePlayer *pLMSLivePlayer; //Pointer to a (or the if all but one are dead) living player in LMS

	float m_flSlowMotionEndTime;

	int m_iRespawnFlags;

#ifndef CLIENT_DLL
	CBaseEntity* EntDHLMapRules;
	CDHLMapRules* DHLMapRules;
	//hudtextparms_s m_timerParam; //Used for the HUD message round timer
#endif
};

inline CDHLRules* DHLRules()
{
	return static_cast<CDHLRules*>(g_pGameRules);
}

#endif //DHL_GAMERULES_H
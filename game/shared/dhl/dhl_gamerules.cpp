//=============================================================================//
// Purpose:	Distraction Half-Life 2 Gamerules entity
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl_gamerules.h"

#include "dhl/dhl_player_inc.h"
#include "dhl/dhl_shared.h"
#include "dhl/dhl_baseweapon.h"
#include "filesystem.h"
#include "ammodef.h"
#ifndef CLIENT_DLL
	#include "dhl/dhl_maprules.h"
	#include "hl2mp_bot_temp.h"
	#include "dhl/dhl_projectile.h"
	#include "team.h"
	#include "game.h"
#else
	#include "cliententitylist.h"
	#include "SoundEmitterSystem/isoundemittersystembase.h"
	#include "engine/IEngineSound.h"
	#include "c_te_legacytempents.h"
	#include "tempent.h"
#endif

//Ammodef defines - bad!
	#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
	#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))
	// exaggerate all of the forces, but use real numbers to keep them consistent
	#define BULLET_IMPULSE_EXAGGERATION			3.5
	// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
	#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

const float DHL_ROUNDTHINK_INTERVAL = 0.5f;

static DHLViewVectors g_DHLViewVectors(
	Vector( 0, 0, 66 ),       //m_vView
							  
	Vector(-16, -16, 0 ),	  //m_vHullMin
	Vector( 16,  16, 72 ),	  //m_vHullMax
							  
	Vector(-16, -16, 0 ),	  //m_vDuckHullMin
	Vector( 16,  16, 48 ),	  //m_vDuckHullMax
	Vector( 0, 0, 48 ),		  //m_vDuckView
							  
	Vector(-10, -10, -10 ),	  //m_vObsHullMin
	Vector( 10,  10, 10 ),	  //m_vObsHullMax
							  
	Vector( 0, 0, 14 ),		  //m_vDeadViewHeight

	Vector(-16, -16, 0 ),	  //m_vCrouchTraceMin
	Vector( 16,  16, 60 ),	  //m_vCrouchTraceMax

	Vector( -16, -16, 0 ),	  //m_vProneHullMin
	Vector( 16, 16, 18 ),	  //m_vProneHullMax
	Vector( 0, 0, 15 ),		  //m_vProneView

	//These are used for surrounding bounds ONLY - not collisions
	Vector( -38, -38, 0 ),    //m_vProneTRMin
	Vector( 38, 38, 20 ),	  //m_vProneTRMax
	Vector( -38, -38, 0 ),	  //m_vStuntTRMin
	Vector( 38, 38, 72 ),	  //m_vStuntTRMax

	Vector( -16, -16, 40 ),	  //m_vDiveHullMin
	Vector( 16, 16, 60 )	  //m_vDiveHullMax
);

#ifndef CLIENT_DLL
void TimescaleChanged_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVarRef TSRef( var );
	if ( DHLRules() )
		DHLRules()->SetTimescale( TSRef.GetFloat() );

}
ConVar dhl_timescale_shared( "dhl_timescale_shared", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT, "Slow motion testing on the server", FnChangeCallback_t(TimescaleChanged_Callback) );
#endif
ConVar dhl_slowmotion( "dhl_slowmotion", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "1 - Enable slowmotion (default) 0 - Disable slow motion" );
ConVar dhl_gamemode( "dhl_gamemode", "0", FCVAR_REPLICATED, "0 - Deathmatch, 1 - Team Deathmatch, 2 - Last Man Standing, 3 - Team Roundplay" );
ConVar dhl_roundtime( "dhl_roundtime", "180", FCVAR_REPLICATED, "Length of rounds, in seconds" );
ConVar dhl_allowroundrespawns( "dhl_allowroundrespawns", "0", FCVAR_REPLICATED, "Enable to allow players to respawn mid-round" );
ConVar dhl_dynamicteamspawns( "dhl_dynamicteamspawns", "1", FCVAR_REPLICATED, "Group spawning in team roundplay" );
ConVar dhl_roundrestartdelay( "dhl_roundrestartdelay", "5", FCVAR_REPLICATED, "Period in seconds to wait before restarting the round after one team is eliminated" );
ConVar dhl_firstroundstartdelay( "dhl_firstroundstartdelay", "30", FCVAR_REPLICATED, "Period in seconds to wait before starting the first round after a mapchange (for players to load and select weapons)" );

#ifndef CLIENT_DLL
CON_COMMAND( dhl_restartround, "Restarts the round immediately" )
{
	if ( !DHLRules()->IsRoundplay() )
		return;
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	//Restart the round
	DHLRules()->RestartRound();
}
//ConCommand dhl_restartround( "dhl_restartround", CC_RestartRound, "Restarts the round immediately" );

CON_COMMAND( setteam, "Sets the player's team" )
{
	if ( args.ArgC() == 1 )
		return;

	CDHL_Player* pPlayer = ToDHLPlayer(UTIL_GetCommandClient());
	if ( !pPlayer )
		return;

	//Don't let people spam team changes
	float flLastTeamChange = pPlayer->flLastTeamChange;
	if ( (gpGlobals->curtime - flLastTeamChange) < 5.0f && gpGlobals->curtime >= 5.0f )
	{
		char szReturnString[128];
		Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %i more seconds before trying to switch teams.\n", (5 - int(gpGlobals->curtime - flLastTeamChange) ));
		ClientPrint( pPlayer, HUD_PRINTTALK, szReturnString );
		return;
	}

	//const char* szTeam = engine->Cmd_Argv(1);
	int iTeam = atoi( args.Arg(1) );

	if ( iTeam == TEAM_CMD_SPEC )
	{
		engine->ClientCommand( pPlayer->edict(), "spectate" );
		pPlayer->flLastTeamChange = gpGlobals->curtime;
		return;
	}

	if ( DHLRules()->IsTeamplay() /*&& !DHLRules()->IsRoundplay()*/ )
	{
		if ( iTeam == TEAM_CMD_AUTOASSIGN )
		{
			CTeam *pMobsters = g_Teams[TEAM_MOBSTERS];
			CTeam *pProfessionals = g_Teams[TEAM_PROS];

			if ( pMobsters && pProfessionals )
			{
				pPlayer->flLastTeamChange = gpGlobals->curtime;
				int iNumMobsters = pMobsters->GetNumPlayers();
				int iNumPros = pProfessionals->GetNumPlayers();

				if ( iNumMobsters == iNumPros )
					pPlayer->ChangeTeam( random->RandomInt( TEAM_MOBSTERS, TEAM_PROS ) );
				else if ( iNumPros > iNumMobsters )
					pPlayer->ChangeTeam( TEAM_MOBSTERS );
				else
					pPlayer->ChangeTeam( TEAM_PROS );

			}
			return;
		}
		if ( iTeam == TEAM_CMD_MOB || iTeam == TEAM_CMD_PROS )
		{
			if ( iTeam == TEAM_CMD_MOB )
				pPlayer->ChangeTeam( TEAM_MOBSTERS );
			if ( iTeam == TEAM_CMD_PROS )
				pPlayer->ChangeTeam( TEAM_PROS );
			pPlayer->flLastTeamChange = gpGlobals->curtime;
		}
		return;
	}
	else
	{
		if ( iTeam == TEAM_CMD_DEFAULT )
		{
			pPlayer->ChangeTeam( TEAM_UNASSIGNED );
			pPlayer->flLastTeamChange = gpGlobals->curtime;
		}
	}
}
//ConCommand setteam( "setteam", CC_SetTeam, "Sets the player's team" );
#endif //!CLIENT_DLL

REGISTER_GAMERULES_CLASS( CDHLRules );

BEGIN_NETWORK_TABLE_NOBASE( CDHLRules, DT_DHLRules )
	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_iGameMode ) ),
		RecvPropBool( RECVINFO( m_bGameWaiting ) ),
		RecvPropBool( RECVINFO( m_bGrayscale ) ),
		RecvPropFloat( RECVINFO( m_iNetRoundTimeLeft ) ),
		RecvPropFloat( RECVINFO( m_flTimescalar ) ),
		RecvPropInt( RECVINFO( m_iTimeToRoundStart ) ),
	#else
		SendPropInt( SENDINFO( m_iGameMode ) ),
		SendPropBool( SENDINFO( m_bGameWaiting ) ),
		SendPropBool( SENDINFO( m_bGrayscale ) ),
		SendPropFloat( SENDINFO( m_iNetRoundTimeLeft ) ),
		SendPropFloat( SENDINFO( m_flTimescalar ) ),
		SendPropInt( SENDINFO( m_iTimeToRoundStart ) ),
	#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( dhl_gamerules, CDHLGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( DHLGameRulesProxy, DT_DHLGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_DHLRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CDHLRules *pRules = DHLRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CDHLGameRulesProxy, DT_DHLGameRulesProxy )
		RecvPropDataTable( "dhl_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_DHLRules ), RecvProxy_DHLRules )
	END_RECV_TABLE()
#else
	void* SendProxy_DHLRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CDHLRules *pRules = DHLRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CDHLGameRulesProxy, DT_DHLGameRulesProxy )
		SendPropDataTable( "dhl_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_DHLRules ), SendProxy_DHLRules )
	END_SEND_TABLE()
#endif

#ifdef CLIENT_DLL
void CDHLGameRulesProxy::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
		case DHL_MSG_ROUNDRESTART:
			RemoveClientEntities();

			//Some engine command that clears all decals (from the world too)
			//Only way to get them all that I can see
			engine->ClientCmd( "r_cleardecals" );
			break;

		case DHL_MSG_TIMESCALECHANGE:
			float flNewTimescale = msg.ReadFloat();
			float flOldTimescale = msg.ReadFloat();
			DHLRules()->TimescaleChangedClient( flNewTimescale, flOldTimescale );
			break;
	}
}

void CDHLGameRulesProxy::RemoveClientEntities( void )
{
	//Client - Removes client only entities
	C_BaseEntity *pEnt;
	C_BaseEntityIterator iterator;
	while ( (pEnt = iterator.Next()) != NULL )
	{
		if ( !pEnt->IsServerEntity() )
		{
			pEnt->Remove();
		}
	}
}

#endif


extern CBaseEntity	 *g_pLastCombineSpawn;
extern CBaseEntity	 *g_pLastRebelSpawn;

#ifndef CLIENT_DLL
ConVar dhl_override_lmslimit( "dhl_override_lmslimit", "0", FCVAR_GAMEDLL, "Please make sure the maps you run have enough spawns if you're going to use this" );
#endif

CDHLRules::CDHLRules()
{
	m_bGrayscale = false;
	m_flTimescalar = 1.0f;
	m_flRoundTimeLeft = 0.0f;
	m_iNetRoundTimeLeft = 0;
	m_iRespawnFlags = DHL_RESPAWN_DEFAULT;
	m_iGameMode = dhl_gamemode.GetInt();

#ifndef CLIENT_DLL
	if ( !dhl_override_lmslimit.GetBool() && m_iGameMode == DHL_GAMEMODE_LASTMANSTANDING && gpGlobals->maxClients > 8 )
	{
		Warning( "\nERROR: Last Man Standing currently supports a maximum of 8 players\n\n" );
		engine->ServerCommand( "disconnect\n" );
	}
#endif

	m_flSlowMotionEndTime = -1.0f;
	#ifndef CLIENT_DLL
		//Dependent HL2MP code looks at these
		teamplay.SetValue( IsTeamplay() );
		m_bTeamPlayEnabled = IsTeamplay();

		m_iDeadPlayers = 0;
		m_iTotalPlayers = 0;
		InitRoundSystem();
		UpdateMaprules();

		m_pMobSpawn = NULL;
		m_pProSpawn = NULL;

		//DHL - Skillet - Bot code from Battlegrounds 2
		//BG2 - Tjoppen - ClientPrintAll()- and bot initialization
		/*g_CurBotNumber = 1;
		for( int x = 0; x < MAX_PLAYERS; x++ )
		{
			gBots[x].m_pPlayer = NULL;
			gBots[x].m_bInuse = false;
		}*/
	#else
		m_hRagdollList.RemoveAll();
	#endif

	//Find models
	MobModels = FindModels( "models/player/dhl/mobsters/*.mdl" );
	ProModels = FindModels( "models/player/dhl/professionals/*.mdl" );

	//Do AmmoDef stuff
	CAmmoDef* def = GetAmmoDef();
		//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
		  //Pistols
		def->AddAmmoType("Beretta",			DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("Deagle",				DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			225,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("SAA",			DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("Deringer",		DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		  //Automatics
		def->AddAmmoType("Mac11",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			225,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("AK47",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("Thompson",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			225,		BULLET_IMPULSE(200, 1225),	0 );
		  //Rifles
		def->AddAmmoType("Winchester",				DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			225,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("MosinNagant",			DMG_BULLET,					TRACER_WHIZ_ONLY,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		  //Shotguns
		def->AddAmmoType("Remington",				DMG_BULLET,					TRACER_NONE,	0,			0,			225,		BULLET_IMPULSE(200, 1225),	0 );
		def->AddAmmoType("SawedOff",			DMG_BULLET,					TRACER_NONE,	0,			0,			150,		BULLET_IMPULSE(200, 1225),	0 );
		  //Misc
		def->AddAmmoType("CombatKnife",				DMG_SLASH,					TRACER_NONE,	0,			0,			2,		BULLET_IMPULSE(200, 1225),	0 );
		//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
}

CDHLRules::~CDHLRules()
{
	//Msg( "CDHLRules Destructor \n");
}

void CDHLRules::Precache( void )
{
	//DHL - Skillet - Precache all of the models in our directories
	//Moved here from CDHL_Player::Precache() so that it will be run only once per map
	DHLShared::PrecacheModels( "models/player/dhl/" );
	DHLShared::PrecacheModels( "models/player/dhl/mobsters/" );
	DHLShared::PrecacheModels( "models/player/dhl/professionals/" );
}

const char** CDHLRules::FindModels( const char* searchString )
{
	const char** dstList = NULL;
	FileFindHandle_t handle;
	const char* szModelName = filesystem->FindFirstEx( searchString, "MOD", &handle );
	int iNumModels = 0;
	CUtlVector<const char*> vecList;
	while ( szModelName )
	{
		//Q_strncpy( dstList[iNumModels], szModelName, 256 ); //.AddToTail( szModelName );
		if ( modelinfo->GetModelIndex( szModelName ) != -1 )
		{
			vecList.AddToTail( szModelName );
			iNumModels++;
		}
		szModelName = filesystem->FindNext( handle );
	}
	filesystem->FindClose( handle );
	dstList = new const char*[iNumModels];
	vecList.CopyArray( dstList, iNumModels );

	return dstList;
}

//Damage_NoPhysicsForce() includes these
int CDHLRules::Damage_GetTimeBased( void )
{
	return ( BaseClass::Damage_GetTimeBased() | DMG_BLEED );
}

//Necessary so we can early out of CHudDamageIndicator::MsgFunc_Damage(), damage types not included in this aren't networked
int CDHLRules::Damage_GetShowOnHud( void )
{
	return ( BaseClass::Damage_GetShowOnHud() | DMG_BLEED );
}

const CViewVectors* CDHLRules::GetViewVectors()const
{
	return &g_DHLViewVectors;
}

const HL2MPViewVectors* CDHLRules::GetHL2MPViewVectors()const
{
	return &g_DHLViewVectors;
}
const DHLViewVectors* CDHLRules::GetDHLViewVectors()const
{
	return &g_DHLViewVectors;
}

void CDHLRules::CreateStandardEntities( void )
{
	#ifndef CLIENT_DLL
		//Bypass CHL2MPRules
		CTeamplayRules::CreateStandardEntities();

		g_pLastCombineSpawn = NULL;
		g_pLastRebelSpawn = NULL;

		#ifdef _DEBUG
			CBaseEntity *pEnt = 
		#endif
		// Create the entity that will send our data to the client.
		CBaseEntity::Create( "dhl_gamerules", vec3_origin, vec3_angle );
		Assert( pEnt );
	#endif
}

void CDHLRules::Think( void )
{
	BaseClass::Think();
	#ifndef CLIENT_DLL
		SlowMotionThink();
		if ( IsRoundplay() )
		{
			DecrementTimers();
			m_iNetRoundTimeLeft = DHLShared::RoundFloat(m_flRoundTimeLeft);
			if ( m_flNextRoundThink > gpGlobals->curtime + DHL_ROUNDTHINK_INTERVAL )
				m_flNextRoundThink = gpGlobals->curtime + DHL_ROUNDTHINK_INTERVAL;
			if ( m_flNextRoundThink <= gpGlobals->curtime )
			{
				RoundThink();
				m_flNextRoundThink = gpGlobals->curtime + DHL_ROUNDTHINK_INTERVAL;
			}
		}
	#endif
}

bool CDHLRules::AllowRespawn( void )
{
#ifndef CLIENT_DLL
	if ( dhl_allowroundrespawns.GetBool() || !IsRoundplay() || IsGameWaiting() )
		return true;

	if ( m_iRespawnFlags == 1 )
		return true;
	else if ( m_iRespawnFlags == 2 )
		return false;
#endif
	return true;
}

bool CDHLRules::GetGrayscale( void )
{
	return m_bGrayscale;
}

void CDHLRules::SetGrayscale( bool bNewGrayscale )
{
	m_bGrayscale = bNewGrayscale;
}

float CDHLRules::GetTimescale( bool bRaw )
{
	if ( bRaw )
		return m_flTimescalar;
	else
	{
		if ( m_flTimescalar <= 0 )
			return 1.0;
	}
	return m_flTimescalar;
}

#ifndef CLIENT_DLL
void CDHLRules::SetTimescale( float flNewTimescale )
{
	//Changing this means we have to resend it to all clients, so make sure we really need to
	if ( m_flTimescalar && m_flTimescalar != flNewTimescale )
	{
		//Notify weapons
		CDHLBaseWeapon *pWeapon = NULL;
		while ( (pWeapon = gEntList.NextEntByClass( pWeapon )) != NULL )
		{
			pWeapon->TimescaleChanged( flNewTimescale, m_flTimescalar );
		}

		//Notify players
		CDHL_Player *pPlayer = NULL;
		while ( (pPlayer = gEntList.NextEntByClass( pPlayer )) != NULL )
		{
			pPlayer->TimescaleChanged( flNewTimescale, m_flTimescalar );
		}

		//Notify the client directly with an entity message
		SignalTimescaleChange( flNewTimescale );

		//Change the timescale (will be networked to client automatically)
		m_flTimescalar = flNewTimescale;
	}
}

bool CDHLRules::InitSlowMotion( CDHL_Player* pInitiator, float flLength )
{
	if ( !pInitiator || !pInitiator->IsAlive() || IsGameWaiting() ) //Only if alive & game not waiting
		return false;
	if ( m_flSlowMotionEndTime >= 0 ) //Can't do slowmo if another player has already started it
		return false;
	m_flSlowMotionEndTime = gpGlobals->curtime + flLength;
	SetTimescale( 0.5f );
	return true;
}

void CDHLRules::SlowMotionThink( void )
{
	if ( m_flSlowMotionEndTime >= 0 && m_flSlowMotionEndTime <= gpGlobals->curtime )
	{
		m_flSlowMotionEndTime = -1.0f;
		SetTimescale( 1.0f );
	}
}
#endif

//DHL - Skillet - Called when a message is received from the server signaling that a timescale change has occurred
#ifdef CLIENT_DLL
extern ISoundEmitterSystemBase *soundemitterbase;
void CDHLRules::TimescaleChangedClient( float flNewTimescale, float flOldTimescale )
{
	#ifdef USE_FMOD
		FMODChannelGroups* channelGroups = DHLShared::GetChannelGroups();
		DHLShared::ScaleGroupFrequency( channelGroups->pChanMusic, flNewTimescale );
		DHLShared::ScaleGroupFrequency( channelGroups->pChanOther, flNewTimescale );
		DHLShared::ScaleGroupFrequency( channelGroups->pChanWeapons, flNewTimescale );
	#else
		//Update pitch though Source sound system
		CUtlVector< SndInfo_t > sndList;
		enginesound->GetActiveSounds( sndList );
		for ( int i = 0; i < sndList.Count(); i++ )
		{
			SndInfo_t sndInfo = sndList.Element( i );
			char sndName[128];
			filesystem->String( sndInfo.m_filenameHandle, sndName, 128 );
			//We only get .wav names from SndInfo_t, so this won't work
			//soundlevel_t iSoundLevel = soundemitterbase->LookupSoundLevel( sndName );

			//Emitting the same sound with matching EmitSound_t info and SND_CHANGE_PITCH will change the pitch of the one in progress
			//As far as I can tell from testing the engine won't change attenutation or soundlevel, which means these don't matter
			soundlevel_t iSoundLevel = SNDLVL_NORM;
			CPASAttenuationFilter filter( *sndInfo.m_pOrigin, ATTN_NORM );

			//This is important
			EmitSound_t ep;
			ep.m_nChannel = sndInfo.m_nChannel;
			ep.m_pSoundName = sndName;
			ep.m_flVolume = sndInfo.m_flVolume;
			ep.m_SoundLevel = iSoundLevel;
			ep.m_nFlags = SND_CHANGE_PITCH;

			//Will cause problems for any sounds that aren't PITCH_NORM at timescale 1.0, but there are none now. Slowing is handled in EmitSound()
			ep.m_nPitch = PITCH_NORM;

			CBaseEntity::EmitSound( filter, sndInfo.m_nSoundSource, ep );
		}
	#endif

	//Notify weapons
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt = NULL;
	iterator.Restart();
	while ( (pEnt = iterator.Next()) != NULL )
	{
		C_DHLBaseWeapon* pWeapon = dynamic_cast<C_DHLBaseWeapon*>( pEnt );
		if ( pWeapon )
		{
			pWeapon->TimescaleChanged( flNewTimescale, flOldTimescale );
			continue;
		}
		
		C_DHL_Player* pPlayer = dynamic_cast<C_DHL_Player*>( pEnt );
		if ( pPlayer )
		{
			pPlayer->TimescaleChanged( flNewTimescale, flOldTimescale );
			continue;
		}
	}

	CBasePlayer* pLocalPlayer = CBasePlayer::GetLocalPlayer();
	Assert( pLocalPlayer );
	//THIS CONCEPT ACTUALLY WORKS!!! To eliminate viewmodel anim hitching on timescale change
	//See C_BaseViewModel::Interpolate(), the idea is to change m_flAnimTime so that the value of Timescale*(FinalPredictedTime() - m_flAnimTime) remains
	//constant all the time
	//Behold my attempt at a general mathematical solution, seems to work properly...
	//T(x-y)=C => y = x - C/T or something like that so newAnimTime = finalPredTime - ((finalPredtime-oldAnimTime) / (newTimeScale/oldTimescale))
	//float flOldAnimTime = pLocalPlayer->GetViewModel()->m_flAnimTime;
	if ( pLocalPlayer->GetViewModel() )
		pLocalPlayer->GetViewModel()->m_flAnimTime = pLocalPlayer->GetFinalPredictedTime() - ((pLocalPlayer->GetFinalPredictedTime() - pLocalPlayer->GetViewModel()->m_flAnimTime) / (flNewTimescale/flOldTimescale));

	//Msg( "fPredTime: %f\n", pLocalPlayer->GetFinalPredictedTime() );
	//Msg( "Animtime: %f -> %f\n", flOldAnimTime, pLocalPlayer->GetViewModel()->m_flAnimTime );
	//Msg( "Diff: %f -> %f\n", pLocalPlayer->GetFinalPredictedTime() - flOldAnimTime, pLocalPlayer->GetFinalPredictedTime() - pLocalPlayer->GetViewModel()->m_flAnimTime );

	//Update temp entity death time (shell brass, etc)
	/*CTempEnts* pTempEnts = dynamic_cast<CTempEnts*>(tempents);

	if ( pTempEnts )
	{
		FOR_EACH_LL( pTempEnts->GetTempEntsList(), i )
		{
			C_LocalTempEntity *current = pTempEnts->GetTempEntsList()[ i ];
			if ( current->die > gpGlobals->curtime )
				current->die = gpGlobals->curtime + ((current->die - gpGlobals->curtime) / (flNewTimescale / flOldTimescale));
		}
	}*/
}
#endif

//----------------------------------------------------------------
// Set up round system stuff - called on gamerules construction
//----------------------------------------------------------------
#ifndef CLIENT_DLL
void CDHLRules::InitRoundSystem( void )
{
	m_bRoundRunning = false;

	m_flRoundEndTime = -1.0f;
	m_flRoundStartTime = -1.0f;
	m_flDHL_Roundtime = dhl_roundtime.GetFloat();
	m_flNextRoundThink = gpGlobals->curtime + DHL_ROUNDTHINK_INTERVAL;
	m_flFirstRoundStartTime = gpGlobals->curtime + dhl_firstroundstartdelay.GetFloat();
	m_iTimeToRoundStart = dhl_firstroundstartdelay.GetInt();
	m_bGameWaiting = IsRoundplay() ? true : false;

	//Round restart filter:
	m_filter.AddKeep("ai_network");
	m_filter.AddKeep("ai_hint");
	m_filter.AddKeep("info_node");
	m_filter.AddKeep("info_node_hint");
	m_filter.AddKeep("func_brush");
	m_filter.AddKeep("func_wall");
	m_filter.AddKeep("func_buyzone");
	m_filter.AddKeep("func_illusionary");
	m_filter.AddKeep("infodecal");
	m_filter.AddKeep("info_projecteddecal");
	m_filter.AddKeep("info_target");
	m_filter.AddKeep("info_map_parameters");
	m_filter.AddKeep("keyframe_rope");
	m_filter.AddKeep("move_rope");
	m_filter.AddKeep("info_ladder");
	m_filter.AddKeep("point_viewcontrol");
	m_filter.AddKeep("viewmodel");
	m_filter.AddKeep("predicted_viewmodel");
	m_filter.AddKeep("sky_camera");
	m_filter.AddKeep("env_sun");
	m_filter.AddKeep("env_wind");
	m_filter.AddKeep("env_fog_controller");
	m_filter.AddKeep("env_soundscape");
	m_filter.AddKeep("env_soundscape_proxy");
	m_filter.AddKeep("env_soundscape_triggerable");
	m_filter.AddKeep("trigger_soundscape");
	m_filter.AddKeep("shadow_control");

	m_filter.AddKeep("worldspawn");				
	m_filter.AddKeep("soundent");
	m_filter.AddKeep("dhl_gamerules");
	m_filter.AddKeep("scene_manager");
	m_filter.AddKeep("team_manager");
	m_filter.AddKeep("event_queue_saveload_proxy");
	m_filter.AddKeep("player_manager");
	m_filter.AddKeep("player");
	m_filter.AddKeep("game_player_manager");
	m_filter.AddKeep("ambient_generic");
	m_filter.AddKeep("dhl_maprules");
	m_filter.AddKeep("info_player_start");
	m_filter.AddKeep("info_player_deathmatch");
	m_filter.AddKeep("info_player_combine");
	m_filter.AddKeep("info_player_rebel");
	
	//Visual round timer stuff
	//m_timerParam.x			= 0.470; //Centered? Was 0.7
	//m_timerParam.y			= 0.1; //Was 0.65
	//m_timerParam.effect		= 0;
	//m_timerParam.r1			= 255;
	//m_timerParam.g1			= 255;
	//m_timerParam.b1			= 255;
	//m_timerParam.a1			= 255;
	//m_timerParam.r2			= 255;
	//m_timerParam.g2			= 255;
	//m_timerParam.b2			= 255;
	//m_timerParam.a2			= 255;
	//m_timerParam.fadeinTime	= 0;
	//m_timerParam.fadeoutTime	= 0;
	//m_timerParam.holdTime		= 0;
	//m_timerParam.fxTime		= 0;
	//m_timerParam.channel		= 1;
}

//-------------------------------------------------------------------------------------
// Start the round
//-------------------------------------------------------------------------------------
void CDHLRules::StartRound( float flLength )
{
	if ( flLength < 0 )
		flLength = m_flDHL_Roundtime;

	m_flRoundStartTime = gpGlobals->curtime;
	m_flRoundEndTime = gpGlobals->curtime + flLength;
	m_flRoundTimeLeft = flLength;
	m_bRoundRunning = true;
}

//-------------------------------------------------------------------------------------
// End currently running round and restart in flDelay seconds
//-------------------------------------------------------------------------------------
void CDHLRules::EndRound( int iDelay, bool bNotify )
{
	//We can't end the round unless it's running :)
	if ( !m_bRoundRunning )
		return;

	if ( iDelay < 0 )
		iDelay = dhl_roundrestartdelay.GetInt();

	if ( iDelay > 0 && bNotify )
		UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs("Round Restarting in %i seconds...", iDelay ) );

	//Round is over, we have no end time
	m_flRoundEndTime = -1.0f;

	m_flRoundRestartTime = gpGlobals->curtime + iDelay;
	m_bRoundRunning = false;
}

//-------------------------------------------------------------------------------------
// Called every Think
//-------------------------------------------------------------------------------------
void CDHLRules::RoundThink( void )
{
	if ( !IsRoundplay() )
		return;

	CheckRoundClock();
	DrawRoundHUD();

	//if ( !IsRoundRunning() )
		//return;

	//Don't check anything for the game-start grace period
	if ( m_flFirstRoundStartTime > gpGlobals->curtime )
	{
		m_iTimeToRoundStart = -1;
		if ( IsTeamplay() )
		{
			CTeam *pMobsters = g_Teams[TEAM_MOBSTERS];
			CTeam *pPros = g_Teams[TEAM_PROS];
			if ( pMobsters->GetNumPlayers() > 0 && pPros->GetNumPlayers() > 0 )
				m_iTimeToRoundStart = int(m_flFirstRoundStartTime - gpGlobals->curtime);
		}
		else
		{
			CTeam *pUnassigned = g_Teams[TEAM_UNASSIGNED];
			if ( pUnassigned->GetNumPlayers() > 1 )
				m_iTimeToRoundStart = int(m_flFirstRoundStartTime - gpGlobals->curtime);
		}
		return;
	}
	m_iTimeToRoundStart = -1;
	//Round-based Teamplay
	if ( IsTeamplay() )
		RoundThink_Teamplay();

	//Last Man Standing
	else
		RoundThink_LMS();
}

//-------------------------------------------------------------------------------------
// Called every Think in round-based teamplay
//-------------------------------------------------------------------------------------
void CDHLRules::RoundThink_Teamplay( void )
{
	CTeam *pMobsters = g_Teams[TEAM_MOBSTERS]; //Mob
	CTeam *pProfessionals = g_Teams[TEAM_PROS]; //Pros
	if ( pMobsters->GetNumPlayers() == 0 || pProfessionals->GetNumPlayers() == 0 )
	{
		if ( !IsGameWaiting() )
		{
			m_bGameWaiting = true;
			//If we were playing and players just left, restart and go to wait mode
			if ( pMobsters->GetNumPlayers() > 1 || pProfessionals->GetNumPlayers() > 1 )
			{
				CTeam* pStackedTeam = NULL;
				CTeam* pEmptyTeam = NULL;
				for ( int i = TEAM_MOBSTERS; i <= TEAM_PROS; i++ )
				{
					if ( g_Teams[i]->GetNumPlayers() > 1 )
						pStackedTeam = g_Teams[i];
					else if ( g_Teams[i]->GetNumPlayers() == 0 )
						pEmptyTeam = g_Teams[i];
				}
				if ( pStackedTeam && pEmptyTeam )
				{
					int iDifference = (pStackedTeam->GetNumPlayers() + pEmptyTeam->GetNumPlayers()) % 2;
					while ( (pStackedTeam->GetNumPlayers() - pEmptyTeam->GetNumPlayers()) > iDifference )
					{
						pStackedTeam->GetPlayer( pStackedTeam->GetNumPlayers() - 1 )->ChangeTeam( pEmptyTeam->GetTeamNumber() );
					}
				}
			}
				
			RestartRound();
		}
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Not enough players for Team Roundplay, waiting..." );
	}
	
	//We were waiting but now we have enough players
	else
	{
		if ( IsGameWaiting() )
		{
			RestartRound();
			m_bGameWaiting = false;
		}
	}
}

void CDHLRules::CheckPlayers_RoundTeamplay( void )
{
	//Round based teamplay
	int iMobstersDead = 0;
	int iProsDead = 0;

	CTeam *pMobsters = g_Teams[TEAM_COMBINE]; //Mob
	CTeam *pProfessionals = g_Teams[TEAM_REBELS]; //Pros
	if ( pMobsters->GetNumPlayers() > 0 && pProfessionals->GetNumPlayers() > 0 )
	{
		if ( IsGameWaiting() == true )
		{
			//If we were waiting and enough players just joined, restart
			RestartRound();
			m_bGameWaiting = false;
		}

		//GetHealth <= 0 is kind of a hack, but IsDead dosen't seem to register properly all of the time
		//Check to see if either team has all players dead...
		for ( int c = 0; c < pMobsters->GetNumPlayers(); c++ )
		{
			CBasePlayer *pPlayer = pMobsters->GetPlayer( c );
			if ( ( pPlayer && pPlayer->GetHealth() <= 0 ) || pPlayer->IsObserver() )
			{
				iMobstersDead ++;
			}
		}
		for ( int r = 0; r < pProfessionals->GetNumPlayers(); r++ )
		{
			CBasePlayer *pPlayer = pProfessionals->GetPlayer( r );
			if ( ( pPlayer && pPlayer->GetHealth() <= 0 ) || pPlayer->IsObserver() )
			{
				iProsDead ++;
			}
		}
		if ( iMobstersDead >= pMobsters->GetNumPlayers() && iProsDead < pProfessionals->GetNumPlayers() )
		{
			//All the Mobsters are dead...
			EndRound(-1, false);
			m_iRoundCondition = ROUNDCONDIT_TEAMWIN_PRO;

			CBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "RoundEnd" );
				WRITE_BYTE( pProfessionals->GetTeamNumber() );
			MessageEnd();

			pProfessionals->IncrementRoundsWon(); //Give em a point for winning
		}
		else if ( iProsDead >= pProfessionals->GetNumPlayers() && iMobstersDead < pMobsters->GetNumPlayers() )
		{
			//All the Professionals are dead...
			EndRound(-1, false);
			m_iRoundCondition = ROUNDCONDIT_TEAMWIN_MOB;

			CBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "RoundEnd" );
				WRITE_BYTE( pMobsters->GetTeamNumber() );
			MessageEnd();

			pMobsters->IncrementRoundsWon(); //Point to the Mob for winning
		}
		else if ( iMobstersDead >= pMobsters->GetNumPlayers() && iProsDead >= pProfessionals->GetNumPlayers() )
		{
			//Everyone is dead.. :O!
			EndRound(-1, false);
			m_iRoundCondition = ROUNDCONDIT_DRAW;

			CBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "RoundEnd" );
				WRITE_BYTE( 255 ); //Hack - if we ever have 256 players this will break :P
			MessageEnd();
		}
	}

	m_iDeadPlayers = iMobstersDead + iProsDead;
	m_iTotalPlayers = pMobsters->GetNumPlayers() + pProfessionals->GetNumPlayers();
}

//-------------------------------------------------------------------------------------
// Called every Think in last man standing game mode
//-------------------------------------------------------------------------------------
void CDHLRules::RoundThink_LMS( void )
{
	m_iTotalPlayers = 0;
	CTeam *pUnassigned = g_Teams[TEAM_UNASSIGNED];
	for ( int i = 1; i <= pUnassigned->GetNumPlayers(); i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		//This iteration of the loop ain't happening if we didn't find a player
		if ( !pPlayer )
			continue;
	
		if ( pPlayer->IsConnected() ) //Don't count players that are still connecting
		{
			m_iTotalPlayers++;
		}
	}

	if ( m_iTotalPlayers < 2 )
	{
		if ( !IsGameWaiting() )
		{
			//If we were playing and players just left, restart and go to wait mode
			RestartRound();
			m_bGameWaiting = true;
		}
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Not enough players for Last Man Standing, waiting..." );
	}
	
	//Check to see if we were waiting but now have enough players
	else if ( m_iTotalPlayers >= 2 )
	{
		if ( IsGameWaiting() )
		{
			RestartRound();
			m_bGameWaiting = false;
		}
	}
}

void CDHLRules::CheckPlayers_LMS( void )
{
	//With teamplay off all players should be on TEAM_UNASSIGNED or TEAM_SPECTATOR
	m_iDeadPlayers = 0;
	m_iTotalPlayers = 0;
	//CTeam *pUnassigned = g_Teams[TEAM_UNASSIGNED];
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		//This iteration of the loop ain't happening if we didn't find a player
		if ( !pPlayer || pPlayer->GetTeamNumber() != TEAM_UNASSIGNED )
			continue;
	
		if ( pPlayer->IsConnected() ) //Don't count players that are still connecting
		{
			m_iTotalPlayers ++;
		}

		//DHL: Kind of a hack, but IsDead dosen't seem to register properly all of the time
		if ( pPlayer->GetHealth() <= 0 || pPlayer->IsObserver() )
		{
			m_iDeadPlayers ++; //Add one to the list for every dead player
		}
		else
		{
			//Grab a pointer to em
			pLMSLivePlayer = pPlayer;

			//Format and buffer a string with all that in it
			//Q_snprintf( cLMSLivePlayer,sizeof(cLMSLivePlayer), "%s wins!", pPlayer->GetPlayerName() );
		}
	}

	if ( m_iTotalPlayers >= 2 )
	{
		if ( IsGameWaiting() == true )
		{
			//If we were waiting and players just joined, restart
			RestartRound();
			m_bGameWaiting = false;
		}
		if ( m_iDeadPlayers == (m_iTotalPlayers - 1) && pLMSLivePlayer ) //If all but one are dead
		{
			//Print the name of the remaining player and prepare to restart
			EndRound(-1, false);
			m_iRoundCondition = ROUNDCONDIT_LMSWIN;
			//UTIL_ClientPrintAll( HUD_PRINTCENTER, cLMSLivePlayer );
			//Give dude what done won some points
			ToDHLPlayer(pLMSLivePlayer)->m_iLMSWins++;

			CBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "RoundEnd" );
				WRITE_BYTE( pLMSLivePlayer->RequiredEdictIndex() );
			MessageEnd();
		}
		else if ( m_iDeadPlayers == m_iTotalPlayers )
		{
			//Everyone is dead, no winner, prepare to restart
			EndRound(-1, false);
			m_iRoundCondition = ROUNDCONDIT_DRAW;

			CBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "RoundEnd" );
				WRITE_BYTE( 255 ); //Hack - if we ever have 256 players this will break :P
			MessageEnd();
		}
	}
}

//Called every think (frame)
//Deal with our timers
void CDHLRules::DecrementTimers( void )
{
	if ( !IsGameWaiting() )
	{
		if ( m_flRoundTimeLeft > 0.0f )
			m_flRoundTimeLeft -= (gpGlobals->frametime);
	}
}

//-------------------------------------------------------------------------------------
//Called every RoundThink, deals with the round clock
//-------------------------------------------------------------------------------------
void CDHLRules::CheckRoundClock()
{
	//Time remaining - only used for clocks as of yet
	if ( IsGameWaiting() )
	{
		m_flRoundTimeLeft = m_flDHL_Roundtime;
		return;
	}
	else
	{
		//Don't let it drop below 0
		m_flRoundTimeLeft = max( 0.0, m_flRoundTimeLeft );
	}

	if ( IsRoundRunning() && m_flRoundTimeLeft <= 0.0f )
	{
		//Out of time, it's a draw
		m_iRoundCondition = ROUNDCONDIT_DRAW;
		EndRound(-1, false);

		CBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "RoundEnd" );
			WRITE_BYTE( 255 ); //Hack - if we ever have 256 players this will break :P
		MessageEnd();

		return;
	}

	if ( !IsRoundRunning() && m_flRoundRestartTime <= gpGlobals->curtime )
	{
		RestartRound();
		return;
	}
}

//-------------------------------------------------------------------------------------
//Called every RoundThink, draws round clock at top of screen
//At some point we should move this to the client side and make it a HUD element
//Note that it runs only on the server side currently and is all networked
//-------------------------------------------------------------------------------------
void CDHLRules::DrawRoundHUD( void )
{
	switch ( m_iRoundCondition )
	{
	case ROUNDCONDIT_DRAW:
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Round Draw" );
		break;
	case ROUNDCONDIT_TEAMWIN_MOB:
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Mobsters Win!" );
		break;
	case ROUNDCONDIT_TEAMWIN_PRO:
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Professionals Win!" );
		break;
	case ROUNDCONDIT_LMSWIN:
		if ( cLMSLivePlayer )
			//UTIL_ClientPrintAll( HUD_PRINTCENTER, cLMSLivePlayer );
		break;
	}

	//Color it red if we're at 10 seconds or less
	//if ( GetRoundTimeLeft() < 11.0f )
	//{
	//	m_timerParam.g1 = 0;
	//	m_timerParam.b1 = 0;
	//}
	//else
	//{
	//	m_timerParam.g1 = 255;
	//	m_timerParam.b1 = 255;
	//}
	////Draw Min:Sec timer at the top of the screen
	////Finally a use for the modulo operator!
	//UTIL_HudMessageAll( m_timerParam, UTIL_VarArgs( ((int(GetRoundTimeLeft()) % 60 ) >= 10) ? "%i:%i" : "%i:0%i", (int( GetRoundTimeLeft() / 60 )), (int(GetRoundTimeLeft()) % 60 ) ) );
}

//-------------------------------------------------------------------------------------
// Called every Think, updates map specific rules, which are defined
// by the mapper in the dhl_maprules entity.
//-------------------------------------------------------------------------------------
void CDHLRules::UpdateMaprules( void )
{
	//Grab a CBaseEntity pointer to the DHL map rules entity
	EntDHLMapRules = gEntList.FindEntityByClassname( NULL, "dhl_maprules" );

	//Cast our CBaseEntity into CDHLMapRules
	DHLMapRules = static_cast<CDHLMapRules*>( EntDHLMapRules );

	if ( DHLMapRules )
	{
		if ( DHLMapRules->GetForceGrayscale() == true )
			m_bGrayscale = true;
		else
			m_bGrayscale = false;
	}
}
#endif //!CLIENT_DLL

//-----------------------------------------------------------------------
// DHL round restart function, partly from SourceWiki article
//-----------------------------------------------------------------------
void CDHLRules::RestartRound( void )
{
	//Don't do anything if rounds are disabled
	if ( !IsRoundplay() )
		return;

	if ( m_flFirstRoundStartTime > gpGlobals->curtime )
	{
		m_flFirstRoundStartTime = -1.0f;
		return;
	}

#ifndef CLIENT_DLL
	m_pMobSpawn = NULL;
	m_pProSpawn = NULL;

	RemoveClientEntities();

	//Remove all the decals (blood, bullet holes, player sprays and such) from models
	EntityMessageBegin( gEntList.FindEntityByClassname( NULL, "worldspawn" ) );
		WRITE_BYTE( BASEENTITY_MSG_REMOVE_DECALS );
	MessageEnd();

	m_iRoundCondition = ROUNDCONDIT_NONE;

	m_bRoundRunning = true;

	//Round is over, we have no end time
	m_flRoundEndTime = -1.0f;

	m_flRoundRestartTime = -1.0f;

	m_flDHL_Roundtime = dhl_roundtime.GetFloat(); //DHL: Every restart sync the var used in think operations with the CVAR

	//Force allow respawn, so we can spawn players back at the end of the round
	m_iRespawnFlags = DHL_RESPAWN_FORCEALLOW;

	CBaseEntity *pEnt;
	CBaseEntity *tmpEnt;
 
	// find the first entity in the entity list
	pEnt = gEntList.FirstEnt();
 
	// as long as we've got a valid pointer, keep looping through the list
	while (pEnt) {
		if (m_filter.ShouldCreateEntity (pEnt->GetClassname() ) )
		{
			// if we don't need to keep the entity, we remove it from the list
			tmpEnt = gEntList.NextEnt (pEnt);
			UTIL_Remove (pEnt);
			pEnt = tmpEnt;
		}	
		else
		{
			//DHL: Clean off the decals for entities that we're not respawning
			//pEnt->RemoveAllDecals(); //HORRIBLE idea
			// if we need to keep it, we move on to the next entity
			pEnt = gEntList.NextEnt (pEnt);
		}
	} 
 
        // force the entities we've set to be removed to actually be removed
    gEntList.CleanupDeleteList();
 
	// with any unrequired entities removed, we use MapEntity_ParseAllEntities to reparse the map entities
	// this in effect causes them to spawn back to their normal position.
	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &m_filter, true);
 
	//Skillet - ugh, gotta do this for my group spawning code to work properly
	if ( IsTeamplay() )
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CDHL_Player *plr = ToDHLPlayer(UTIL_PlayerByIndex( i ));
	 
			if ( plr )
				plr->m_bHasRespawned = false;
		}
	}

	// now we've got all our entities back in place and looking pretty, we need to respawn all the players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = UTIL_PlayerByIndex( i );
 
		if ( plr && (plr->GetTeamNumber() != TEAM_SPECTATOR) )
		{
			plr->RemoveAllItems(true);
			plr->Spawn();
		}
	}
	StartRound( -1 );
	//Force lock respawns, so players killed during a round stay dead.
	m_iRespawnFlags = DHL_RESPAWN_FORCEDISABLE;
#else
	m_hRagdollList.RemoveAll();
#endif
}

//Used on round restarts
#ifndef CLIENT_DLL
void CDHLRules::RemoveClientEntities( void )
{
	//Sends message to client
	EntityMessageBegin( gEntList.FindEntityByClassname( NULL, "dhl_gamerules" ) );
		WRITE_BYTE( DHL_MSG_ROUNDRESTART );
	MessageEnd();
}

void CDHLRules::SignalTimescaleChange( float flNewTimescale )
{
	//For the love of God, please don't set reliable = true, it causes CRAZY things to happen on the client side
	EntityMessageBegin( gEntList.FindEntityByClassname( NULL, "dhl_gamerules" ) );
		WRITE_BYTE( DHL_MSG_TIMESCALECHANGE );
		WRITE_FLOAT( flNewTimescale );
		WRITE_FLOAT( m_flTimescalar );
	MessageEnd();
}

void CDHLRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	if ( IsRoundplay() )
	{
		//No need to check if the game has already been won
		if ( m_iRoundCondition == ROUNDCONDIT_NONE )
		{
			if ( GetGameMode() == DHL_GAMEMODE_TEAMROUNDPLAY )
				CheckPlayers_RoundTeamplay();
			else if ( GetGameMode() == DHL_GAMEMODE_LASTMANSTANDING )
				CheckPlayers_LMS();
		}
	}
	BaseClass::PlayerKilled( pVictim, info );
}

bool CDHLRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	//Bypass CHL2MPRules
	return CTeamplayRules::CanHavePlayerItem( pPlayer, pItem );
}
bool CDHLRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	//Never switch weapons automatically when they're being given out on spawn
	CDHL_Player* pDHLPlayer = ToDHLPlayer( pPlayer );
	if ( pDHLPlayer && pDHLPlayer->m_bAllowBumpPickup )
		return false;

	return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
}

CBaseEntity* CDHLRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	//Stick to the hl2dm logic except for in team roundplay
	if ( !IsTeamplay() || !IsRoundplay() || !IsRoundRunning() || dhl_allowroundrespawns.GetBool() || !dhl_dynamicteamspawns.GetBool() )
		return BaseClass::GetPlayerSpawnSpot( pPlayer );

	if ( pPlayer->GetTeamNumber() == TEAM_PROS && m_pProSpawn != NULL )
	{
		CBaseEntity* pTempSpawn = m_pProSpawn;
		for ( int i = 0; i < 6; i++ )
		{
			if ( (m_pProSpawn == m_pMobSpawn) || (pTempSpawn == m_pMobSpawn) || !SpawnNearby( pTempSpawn, ToDHLPlayer(pPlayer) ) )
				pTempSpawn = gEntList.FindEntityByClassname( pTempSpawn, "info_player_deathmatch" );
			else
				return pTempSpawn;
		}

		//We have TF2 teammate collisions (non-solid w/pushaway) so just warn and spawn everyone on top of each other
		Warning( "DHL WARNING: Insufficient spawns for proper team roundplay dynamic spawning!\n" );
		SetupSpawn( pPlayer, m_pProSpawn->GetAbsOrigin(), m_pProSpawn->GetLocalAngles() );

		return NULL;
	}
	if ( pPlayer->GetTeamNumber() == TEAM_MOBSTERS && m_pMobSpawn != NULL )
	{
		CBaseEntity* pTempSpawn = m_pMobSpawn;
		for ( int i = 0; i < 6; i++ )
		{
			if ( (m_pMobSpawn == m_pProSpawn) || (pTempSpawn == m_pProSpawn) || !SpawnNearby( pTempSpawn, ToDHLPlayer(pPlayer) ) )
				pTempSpawn = gEntList.FindEntityByClassname( pTempSpawn, "info_player_deathmatch" );
			else
				return pTempSpawn;
		}

		//We have TF2 teammate collisions (non-solid w/pushaway) so just warn and spawn everyone on top of each other
		Warning( "DHL WARNING: Insufficient spawns for proper team roundplay dynamic spawning!\n" );
		SetupSpawn( pPlayer, m_pMobSpawn->GetAbsOrigin(), m_pMobSpawn->GetLocalAngles() );

		return NULL;
	}
	Assert( (pPlayer->GetTeamNumber() == TEAM_PROS) || (pPlayer->GetTeamNumber() == TEAM_MOBSTERS) );

	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
	if ( !pSpawnSpot )
	{
		Warning( "DHL WARNING: Team roundplay dynamic spawning failed! Setting dhl_dynamicteamspawns = 0\n" );
		dhl_dynamicteamspawns.SetValue( false );
		return BaseClass::GetPlayerSpawnSpot( pPlayer );
	}

	if ( pPlayer->GetTeamNumber() == TEAM_PROS )
		m_pProSpawn = pSpawnSpot;
	if ( pPlayer->GetTeamNumber() == TEAM_MOBSTERS )
		m_pMobSpawn = pSpawnSpot;

	SetupSpawn( pPlayer, pSpawnSpot->GetAbsOrigin(), pSpawnSpot->GetLocalAngles() );
	return pSpawnSpot;
}

#define TEST_DIST 58
bool CDHLRules::SpawnNearby( CBaseEntity* pSpawnPoint, CDHL_Player* pPlayer )
{
	if ( !pPlayer || !pSpawnPoint )
		return false;

	const Vector vTestPos = pSpawnPoint->GetAbsOrigin();
	QAngle angles = pSpawnPoint->GetLocalAngles();
	Vector vPos = FindNearbySpawnablePoint( pSpawnPoint, vTestPos, pPlayer );
	if ( vPos != vec3_invalid )
	{
		SetupSpawn( pPlayer, vPos, angles );
		return true;
	}
	vPos = FindNearbySpawnablePoint( pSpawnPoint, vTestPos + Vector(0,TEST_DIST,0), pPlayer );
	if ( vPos != vec3_invalid )
	{
		SetupSpawn( pPlayer, vPos, angles );
		return true;
	}
	vPos = FindNearbySpawnablePoint( pSpawnPoint, vTestPos + Vector(0,-TEST_DIST,0), pPlayer );
	if ( vPos != vec3_invalid )
	{
		SetupSpawn( pPlayer, vPos, angles );
		return true;
	}
	vPos = FindNearbySpawnablePoint( pSpawnPoint, vTestPos + Vector(TEST_DIST,0,0), pPlayer );
	if ( vPos != vec3_invalid )
	{
		SetupSpawn( pPlayer, vPos, angles );
		return true;
	}
	vPos = FindNearbySpawnablePoint( pSpawnPoint, vTestPos + Vector(-TEST_DIST,0,0), pPlayer );
	if ( vPos != vec3_invalid )
	{
		SetupSpawn( pPlayer, vPos, angles );
		return true;
	}

	return false;
}

void CDHLRules::SetupSpawn( CBasePlayer* pPlayer, const Vector& vPos, const QAngle& angDir )
{
	pPlayer->SetLocalOrigin( vPos + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( angDir );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( angDir );
}

//Tests 6 positions around the point in sequence
//Input point z should be equivalent to GetAbsOrigin() point of spawn entity
Vector CDHLRules::FindNearbySpawnablePoint( CBaseEntity* pSpawnEnt, const Vector& vPos, CDHL_Player* pPlayer )
{
	Vector vTestPos = vPos;
	Vector vSpawnPos = pSpawnEnt->GetAbsOrigin();
	
	vTestPos.y += TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.x += TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.y -= TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.y -= TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.x -= TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.x -= TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.y += TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;
	vTestPos.y += TEST_DIST;
	if ( TestSpawnablePoint( vSpawnPos, vTestPos, pPlayer ) )
		return vTestPos;

	return vec3_invalid;
}

bool CDHLRules::TestSpawnablePoint( const Vector& vOrigPos, const Vector& vTestPos, CDHL_Player* pPlayer )
{
	if ( pPlayer->TestPlayerBBox( vTestPos ) )
	{
		trace_t pm;
		Ray_t ray;
		ray.Init( vTestPos, vOrigPos, VEC_HULL_MIN, VEC_HULL_MAX );
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &pm );
		if ( pm.DidHit() )
			return false;

		//Make sure there's something for them to stand on
		//If the spawn point is aligned with the ground its origin should be 36 units above, so look an extra 24 units down
		trace_t tr;
		UTIL_TraceLine( vTestPos, vTestPos + (Vector( 0, 0, -1 ) * 60.0f), MASK_PLAYERSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr );
		//DebugDrawLine( tr.startpos, tr.endpos, 255, 0, 0, false, 3.0f );
		if ( !tr.DidHit() )
			return false;

		return true;
	}
	return false;
}
#endif //!CLIENT_DLL

const char *CDHLRules::GetGameDescription( void )
{ 
	//This is shown at in the game column of the server list
	if ( IsTeamplay() )
	{
		if ( IsRoundplay() == true )
		{
			return "DHL2 b1.1 Roundplay";
		}
		return "DHL2 b1.1 TDM";
	}

	else if ( IsRoundplay() == true )
	{
		return "DHL2 b1.1 LMS";
	}
	return "DHL2 b1.1 DM";
}
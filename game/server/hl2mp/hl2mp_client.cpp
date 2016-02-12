//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "team.h"
#include "viewport_panel_names.h"

//DHL
#include "cdll_int.h"
#include "in_buttons.h"
#include "dhl/dhl_shared.h"
#include "dhl/dhl_player.h"
#ifdef USE_FMOD
#include "ambient_generic.h"
#include "recipientfilter.h"
#endif
#include "dhl/dhl_gamerules.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say( edict_t *pEdict, bool teamonly );

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer, char *classname );
extern bool			g_fGameOver;

void FinishClientPutInServer( CDHL_Player *pPlayer ) //DHL - Skillet - DHL_Player
{
	pPlayer->InitialSpawn();
	//DHL - Don't automatically spawn them so they have a chance to choose weapons
	if ( /*!DHLRules()->IsRoundplay() &&*/ !DHLShared::IsBackgroundMap() )
	{
		//pPlayer->StartObserverMode( OBS_MODE_ROAMING );
		//Use the state system
		pPlayer->State_Transition( STATE_OBSERVER_MODE );
		pPlayer->SetObserverMode( OBS_MODE_ROAMING ); 
		pPlayer->m_lifeState = LIFE_RESPAWNABLE; //Required to get the death think function running
		pPlayer->CBasePlayer::ChangeTeam( TEAM_SPECTATOR, false, true ); //Just go spec
	}
	//Spawn them right away in a background map
	else
	{
		pPlayer->Spawn();
		return;
	}

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	//UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" ); //DHL - Skillet - handled on client by player_connect

	//DHL - Skillet - Skip
	/*if ( HL2MPRules()->IsTeamplay() == true )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "You are on team %s1\n", pPlayer->GetTeam()->GetName() );
	}*/

	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"motd" );		// use this stringtable entry

	pPlayer->ShowViewPortPanel( PANEL_INFO, true, data );

	data->deleteThis();
	
	#ifdef USE_FMOD
		//DHL - Skillet - Play ambient sounds
		CAmbientGeneric *pAmbient = NULL;
		while ( ( pAmbient = (CAmbientGeneric*) gEntList.FindEntityByClassname( pAmbient, "ambient_generic" ) ) != NULL )
		{
			if (pAmbient->m_bPlaying )
			{
				Vector vecOrigin = pAmbient->GetAbsOrigin();
				EmitSound_t ep;
				ep.m_pSoundName = STRING(pAmbient->m_iszSound);
				ep.m_pOrigin = &vecOrigin;
				ep.m_flVolume = 1.0f;
				ep.m_SoundLevel = pAmbient->m_iSoundLevel;
				ep.m_nSpeakerEntity = pAmbient->entindex();
				ep.m_bLooping = pAmbient->m_fLooping;

				CSingleUserRecipientFilter filter( pPlayer );
				filter.MakeReliable();
				pAmbient->EmitSound( filter, pAmbient->entindex(), ep );
			}
		}
	#endif
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn

	//DHL - Skillet
	//CHL2MP_Player *pPlayer = CHL2MP_Player::CreatePlayer( "player", pEdict );
	CDHL_Player *pPlayer = CDHL_Player::CreatePlayer( "player", pEdict );

	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	//DHL - Skillet
	//CHL2MP_Player *pPlayer = ToHL2MPPlayer( CBaseEntity::Instance( pEdict ) );
	CDHL_Player *pPlayer = ToDHLPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Distraction Half-Life 2"; //DHL; Console outputs "Game DLL Loaded for <this>" on engine start
}

//-----------------------------------------------------------------------------
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity* FindEntity( edict_t *pEdict, char *classname)
{
	// If no name was given set bits based on the picked
	if (FStrEq(classname,"")) 
	{
		return (FindPickerEntityClass( static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname ));
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	CBaseEntity::PrecacheModel("models/player.mdl");
	CBaseEntity::PrecacheModel( "models/gibs/agibs.mdl" );
	CBaseEntity::PrecacheModel ("models/weapons/v_hands.mdl");

	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowAmmo" );
	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowHealth" );

	CBaseEntity::PrecacheScriptSound( "FX_AntlionImpact.ShellImpact" );
	CBaseEntity::PrecacheScriptSound( "Missile.ShotDown" );
	CBaseEntity::PrecacheScriptSound( "Bullets.DefaultNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.GunshipNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.StriderNearmiss" );
	
	CBaseEntity::PrecacheScriptSound( "Geiger.BeepHigh" );
	CBaseEntity::PrecacheScriptSound( "Geiger.BeepLow" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( pEdict );

	if ( pPlayer )
	{
		if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
		{
			if ( DHLRules()->AllowRespawn() || DHLRules()->IsGameWaiting() ) //DHL: Check if we want to let the player respawn..
			{
				// respawn player
				pPlayer->Spawn();
			}
			else if ( DHLRules()->IsRoundplay() )
			{
				pPlayer->State_Transition( STATE_OBSERVER_MODE );
				pPlayer->SetObserverMode( OBS_MODE_ROAMING );
			}
		}
		else
		{
			pPlayer->SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}
}

void GameStartFrame( void )
{
	VPROF("GameStartFrame()");
	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = (teamplay.GetInt() != 0);

	//DHL - Skillet - Commented #ifdef
//#ifdef DEBUG
	extern void Bot_RunAll();
	Bot_RunAll();
//#endif
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	//DHL - Skillet
#ifdef DISTRACTION
	CreateGameRulesObject( "CDHLRules" );
#else
	// vanilla deathmatch
	CreateGameRulesObject( "CHL2MPRules" );
#endif
}


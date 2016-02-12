//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "datacache/imdlcache.h"
#include "utlvector.h"
#include "vprof.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"

#endif

//DHL - Skillet - FMOD
#include "dhl/dhl_shared.h"
#include "dhl/dhl_player_inc.h"
#include "hl2mp_gamerules.h"

#ifdef USE_FMOD
#ifdef CLIENT_DLL 
#include "dhl/fmod/fmod_errors.h"
#endif
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Pointer to a member method of IGameSystem
typedef void (IGameSystem::*GameSystemFunc_t)();

// Pointer to a member method of IGameSystem
typedef void (IGameSystemPerFrame::*PerFrameGameSystemFunc_t)();

// Used to invoke a method of all added Game systems in order
static void InvokeMethod( GameSystemFunc_t f, char const *timed = 0 );
// Used to invoke a method of all added Game systems in reverse order
static void InvokeMethodReverseOrder( GameSystemFunc_t f );

// Used to invoke a method of all added Game systems in order
static void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed = 0 );

static bool s_bSystemsInitted = false; 

// List of all installed Game systems
static CUtlVector<IGameSystem*> s_GameSystems( 0, 4 );
// List of all installed Game systems
static CUtlVector<IGameSystemPerFrame*> s_GameSystemsPerFrame( 0, 4 );

// The map name
static char* s_pMapName = 0;

static CBasePlayer *s_pRunCommandPlayer = NULL;
static CUserCmd *s_pRunCommandUserCmd = NULL;

//-----------------------------------------------------------------------------
// Auto-registration of game systems
//-----------------------------------------------------------------------------
static	CAutoGameSystem *s_pSystemList = NULL;

CAutoGameSystem::CAutoGameSystem( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pSystemList;
		s_pSystemList = this;
	}
}

static	CAutoGameSystemPerFrame *s_pPerFrameSystemList = NULL;

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
CAutoGameSystemPerFrame::CAutoGameSystemPerFrame( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pPerFrameSystemList;
		s_pPerFrameSystemList = this;
	}
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystem::~IGameSystem()
{
	Remove( this );
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystemPerFrame::~IGameSystemPerFrame()
{
	Remove( this );
}


//-----------------------------------------------------------------------------
// Adds a system to the list of systems to run
//-----------------------------------------------------------------------------
void IGameSystem::Add( IGameSystem* pSys )
{
	s_GameSystems.AddToTail( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.AddToTail( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}


//-----------------------------------------------------------------------------
// Removes a system from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::Remove( IGameSystem* pSys )
{
	s_GameSystems.FindAndRemove( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.FindAndRemove( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}

//-----------------------------------------------------------------------------
// Removes *all* systems from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::RemoveAll(  )
{
	s_GameSystems.RemoveAll();
	s_GameSystemsPerFrame.RemoveAll();
}


//-----------------------------------------------------------------------------
// Client systems can use this to get at the map name
//-----------------------------------------------------------------------------
char const*	IGameSystem::MapName()
{
	return s_pMapName;
}

#ifndef CLIENT_DLL
CBasePlayer *IGameSystem::RunCommandPlayer()
{
	return s_pRunCommandPlayer;
}

CUserCmd *IGameSystem::RunCommandUserCmd()
{
	return s_pRunCommandUserCmd;
}
#endif

//-----------------------------------------------------------------------------
// Invokes methods on all installed game systems
//-----------------------------------------------------------------------------
bool IGameSystem::InitAllSystems()
{
	int i;

	{
		// first add any auto systems to the end
		CAutoGameSystem *pSystem = s_pSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}
			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}

	{
		CAutoGameSystemPerFrame *pSystem = s_pPerFrameSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}

			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}
	// Now remember that we are initted so new CAutoGameSystems will add themselves automatically.
	s_bSystemsInitted = true;

	for ( i = 0; i < s_GameSystems.Count(); ++i )
	{
		MDLCACHE_CRITICAL_SECTION();

		IGameSystem *sys = s_GameSystems[i];

#if defined( _X360 )
		char sz[128];
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Start", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		bool valid = sys->Init();

#if defined( _X360 )
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Finish", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		if ( !valid )
			return false;
	}

	return true;
}

void IGameSystem::PostInitAllSystems( void )
{
	InvokeMethod( &IGameSystem::PostInit, "PostInit" );
}

void IGameSystem::ShutdownAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::Shutdown );
}

void IGameSystem::LevelInitPreEntityAllSystems( char const* pMapName )
{
	// Store off the map name
	if ( s_pMapName )
	{
		delete[] s_pMapName;
	}

	int len = Q_strlen(pMapName) + 1;
	s_pMapName = new char [ len ];
	Q_strncpy( s_pMapName, pMapName, len );

	InvokeMethod( &IGameSystem::LevelInitPreEntity, "LevelInitPreEntity" );
}

void IGameSystem::LevelInitPostEntityAllSystems( void )
{
	InvokeMethod( &IGameSystem::LevelInitPostEntity, "LevelInitPostEntity" );
}

void IGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreEntity );
}

void IGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPostEntity );

	if ( s_pMapName )
	{
		delete[] s_pMapName;
		s_pMapName = 0;
	}
}

void IGameSystem::OnSaveAllSystems()
{
	InvokeMethod( &IGameSystem::OnSave );
}

void IGameSystem::OnRestoreAllSystems()
{
	InvokeMethod( &IGameSystem::OnRestore );
}

void IGameSystem::SafeRemoveIfDesiredAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::SafeRemoveIfDesired );
}

#ifdef CLIENT_DLL

void IGameSystem::PreRenderAllSystems()
{
	VPROF("IGameSystem::PreRenderAllSystems");
	InvokePerFrameMethod( &IGameSystemPerFrame::PreRender );
}


#ifdef USE_FMOD
	ConVar dhl_fmod_visualizelistenpos( "dhl_fmod_visualizelistenpos", "0", FCVAR_CHEAT, "Show the local 3D listener position for FMOD." );
#endif
void IGameSystem::UpdateAllSystems( float frametime )
{
	#ifdef USE_FMOD
		//DHL - Skillet - FMOD: Update every frame
		if ( DHLShared::GetFMODSystem() )
		{
			DHLShared::GetFMODSystem()->update();

			//Unfortunately, the change callback functions for the volume CVARs aren't called
			//when they're changed through the VGUI.  Therefore this awful think-based hack is required
			//to update the volume for FMOD stuffs.  Damn.  Hopefully it isn't too expensive.

			//Check for snd_musicvolume changes
			FMOD::ChannelGroup *pBGMusic = DHLShared::GetChannelGroups()->pChanBGMusic;
			int iNumChannels;
			pBGMusic->getNumChannels( &iNumChannels );
			const float flMusicVolInterval = 0.5f, flVolInterval = 1.0f; //Seconds
			//This CVAR only concerns menu music, therefore we only need to update it while at the menu
			if ( iNumChannels > 0 && !engine->IsInGame() )
			{
				static float flNextMusicVolUpdate = gpGlobals->realtime + flMusicVolInterval;
				if ( gpGlobals->realtime >= flNextMusicVolUpdate )
				{
					flNextMusicVolUpdate = gpGlobals->realtime + flMusicVolInterval;
					static ConVar* musicVolume = cvar->FindVar( "snd_musicvolume" );
					static float flLastMusicVolume = musicVolume->GetFloat();
					float flVolume = 0.0f;
					pBGMusic->getVolume( &flVolume );
					if ( flLastMusicVolume != musicVolume->GetFloat() || flLastMusicVolume != flVolume )
					{
						pBGMusic->overrideVolume( musicVolume->GetFloat() );
						flLastMusicVolume = musicVolume->GetFloat();
					}
				}
			}

			//Update volume CVAR
			FMOD::ChannelGroup *pGameMaster = DHLShared::GetChannelGroups()->pChanGameMaster;
			static float flNextVolUpdate = gpGlobals->realtime + flVolInterval;
			if ( gpGlobals->realtime >= flNextVolUpdate )
			{
				flNextVolUpdate = gpGlobals->realtime + flVolInterval;
				static ConVar* Volume = cvar->FindVar( "volume" );
				static float flLastVolume = Volume->GetFloat();
				float flVolume = 0.0f;
				pGameMaster->getVolume( &flVolume );
				if ( flLastVolume != Volume->GetFloat() || flLastVolume != flVolume )
				{
					pGameMaster->setVolume( Volume->GetFloat() );
					flLastVolume = Volume->GetFloat();
				}
			}

			C_DHL_Player* pPlayer = ToDHLPlayer(CBasePlayer::GetLocalPlayer());
			if ( pPlayer )
			{
				Vector vecForward;
				Vector vecUp;
				pPlayer->GetVectors( &vecForward, NULL, &vecUp );

				//Use the actual view position, not origin
				FMOD_VECTOR fmodPos;
				DHLShared::VectorToFMOD( pPlayer->m_vecClientViewPos, &fmodPos );
				/*if ( pPlayer->IsAlive() || pPlayer->GetObserverMode == OBS_MODE_ROAMING )
					DHLShared::VectorToFMOD( pPlayer->GetAbsOrigin(), &fmodPos );
				else
					DHLShared::VectorToFMOD( pPlayer->m_vecClientViewPos, &fmodPos );*/

				FMOD_VECTOR fmodVelocity;
				if ( pPlayer->IsAlive() || pPlayer->GetObserverMode() == OBS_MODE_ROAMING )
					DHLShared::VectorToFMOD( pPlayer->GetAbsVelocity(), &fmodVelocity );
				else
					//Velocity will be invalid while dead and not roaming
					DHLShared::VectorToFMOD( vec3_origin, &fmodVelocity );
					

				FMOD_VECTOR fmodForward;
				DHLShared::VectorToFMOD( vecForward, &fmodForward );

				FMOD_VECTOR fmodUp;
				DHLShared::VectorToFMOD( vecUp, &fmodUp );

				FMOD_RESULT result;
				result = DHLShared::GetFMODSystem()->set3DListenerAttributes( 0, &fmodPos, &fmodVelocity, &fmodForward, &fmodUp );
				DHLShared::HandleFMODError( result );

				if ( dhl_fmod_visualizelistenpos.GetBool() )
				{
					Vector visorigin = vec3_origin;
					DHLShared::FMODToVector( fmodPos, &visorigin );
					char Command[128];
					Q_snprintf( Command, sizeof( Command ), "drawcross %f %f %f \n", visorigin.x, visorigin.y, visorigin.z );
					engine->ClientCmd( Command );
				}
			}
		}
	#endif


	SafeRemoveIfDesiredAllSystems();

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c; ++i )
	{
		IGameSystemPerFrame *sys = s_GameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		sys->Update( frametime );
	}
}

void IGameSystem::PostRenderAllSystems()
{
	InvokePerFrameMethod( &IGameSystemPerFrame::PostRender );
}

#else

void IGameSystem::FrameUpdatePreEntityThinkAllSystems()
{
	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePreEntityThink );
}

void IGameSystem::FrameUpdatePostEntityThinkAllSystems()
{
	SafeRemoveIfDesiredAllSystems();

	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePostEntityThink );
}

void IGameSystem::PreClientUpdateAllSystems() 
{
	InvokePerFrameMethod( &IGameSystemPerFrame::PreClientUpdate );
}

#endif


//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethod( GameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_GameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystemPerFrame *sys  = s_GameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in reverse order
//-----------------------------------------------------------------------------
void InvokeMethodReverseOrder( GameSystemFunc_t f )
{
	int i;
	int c = s_GameSystems.Count();
	for ( i = c; --i >= 0; )
	{
		IGameSystem *sys = s_GameSystems[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}



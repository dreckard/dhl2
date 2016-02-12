//Distraction Half-Life 2
//Miscellaneous useful stuff

// redd: fixed the stuff for you :)
#include "cbase.h"
#include "dhl_gamerules.h"
#include "dhl/dhl_shared.h"
#include "filesystem.h"
#include "stdstring.h"
#include "ammodef.h"

#ifdef CLIENT_DLL
#ifdef USE_FMOD
	#include "dhl/fmod/fmod.hpp"
	#include "dhl/fmod/fmod_errors.h"
	#include "soundchars.h"
	#pragma warning(disable: 4005) // Disable macro redefinition warnings
	#include <windows.h> //Included for SetCurrentDirectory function
	#pragma warning(default: 4005) // Restore macro redefinition warnings
#endif
#endif

#ifndef CLIENT_DLL
	#include "func_breakablesurf.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Global Variables
//const char** DHLShared::szMobModelList = NULL;
//const char** DHLShared::szProModelList = NULL;
#ifdef USE_FMOD
	char FMODExclusionFileData[2048]; //Make it nice and big
	#ifdef CLIENT_DLL
		CUtlVector<FMODSound_t*> playingChannelsList;
		static ConVar dhl_fmod_debug( "dhl_fmod_debug", "0", FCVAR_CHEAT, "Output FMOD sound system debug information to the console." );
		FMOD::System* FMODSystem = NULL;
		FMODChannelGroups channelGroups;
	#endif
#endif

	//---------------------------------------------------------------------------------
	// Shared functions
	//---------------------------------------------------------------------------------
	void DHLShared::Init( void ) //Only called on game startup
	{
		#ifdef CLIENT_DLL
			#ifdef USE_FMOD
				InitFMOD();
				ParseFMODExclusionScript( "scripts\\dhl_fmod_exclusions.txt" );
			#endif //USE_FMOD
		#endif
	}

	void DHLShared::LevelInitPostEntity( void ) //Called every changelevel
	{
	}

	void DHLShared::Shutdown( void )
	{
		#ifdef USE_FMOD
			#ifdef CLIENT_DLL
				//DHL - Skillet -Shutdown FMOD
				DHLShared::GetFMODSystem()->release();
				DHLShared::SetFMODSystem( NULL );
			#endif
		#endif //USE_FMOD
	}

	//Real (unaffected by slow motion) curtime
	//NOTE: Obselete 8/24/06
	float DHLShared::RealCurtime( void )
	{
		if ( DHLRules() && DHLRules()->GetTimescale() > 0 )
			return gpGlobals->curtime / DHLRules()->GetTimescale();
		else
			return gpGlobals->curtime;
	}

	//Scales gpGlobals->frametime by the given value or the current timescale (slow motion)
	void DHLShared::ScaleFrametime( float flScalar )
	{
		if ( !DHLRules() )
			return;
		if ( !flScalar || flScalar < 0 )
			flScalar = DHLRules()->GetTimescale() > 0 ? DHLRules()->GetTimescale() : 1.0f;
		gpGlobals->frametime *= flScalar;
	}

	void DHLShared::PrecacheModels( std::string dir )
	{
		FileFindHandle_t handle;
		std::string searchString = dir;
		searchString.append( "*.mdl" );
		const char* szModelName = filesystem->FindFirstEx( searchString.c_str(), "MOD", &handle );
		while ( szModelName )
		{
			//We need a path relative to our root mod folder
			std::string strMdl = dir;
			strMdl.append(szModelName);
			CBaseEntity::PrecacheModel( strMdl.c_str() );
			Msg( "DHL: Precaching model \"%s\"\n", strMdl.c_str() );
			szModelName = filesystem->FindNext( handle );
		}
		filesystem->FindClose( handle );
	}

	//Scales gpGlobals->curtime by the given value or the current timescale (slow motion)
	void DHLShared::ScaleCurtime( float flScalar )
	{
		if ( !DHLRules() )
			return;
		if ( !flScalar || flScalar < 0 )
			flScalar = DHLRules()->GetTimescale() > 0 ? DHLRules()->GetTimescale() : 1.0f;
		gpGlobals->curtime *= flScalar;
	}

	//Reads a command value from a .cfg file, outputting it to Val
	void DHLShared::GetConfigValue( const char *filename, const char *commandName, char* Val, unsigned int ArraySize )
	{
		if ( !filesystem->FileExists( filename ) )
			return;

		FileHandle_t configf = filesystem->Open( filename, "rt" );
		int fileSize = filesystem->Size(configf);
		char *buffer = (char*)malloc(fileSize + 1);
		filesystem->Read(buffer, fileSize, configf); // read into local buffer
		buffer[fileSize] = 0; // null terminate file as EOF
		filesystem->Close( configf );
		std::string sBuffer = buffer;

		unsigned int iPos = sBuffer.find( commandName, 0 );

		if ( iPos != std::string::npos )
		{
			iPos += Q_strlen( commandName );

			if ( iPos != std::string::npos )
			{
				//Find the starting quotation
				unsigned int iValStartPos = sBuffer.find_first_of( "\"", iPos );
				if ( iValStartPos != std::string::npos )
				{
					//Find the ending quotation
					unsigned int iValEndPos = sBuffer.find_first_of( "\"", iValStartPos + 1 );
					if ( iValEndPos != std::string::npos )
					{
						iValEndPos -= 1; //Subtract 1 to get the last character of the value
						unsigned int iValSize = iValEndPos - iValStartPos;

						//Buffer overflow protection
						if ( iValSize <= ArraySize )
						{
							for ( unsigned int i = 0; i < iValSize; i++ )
							{
								Val[i] = sBuffer.at( iValStartPos + i );
							}
						}
					}
				}
			}
		}
		free(buffer);
	}

	//Load a file into memory though the Valve filesystem (mounted GCF data is accessable this way)
	void DHLShared::LoadFileIntoMemory( const char *name, void **buff, unsigned int *length )
	{
		if ( !filesystem->FileExists( name ) )
			return;

		//FILE *fp = fopen(name, "rb");
		FileHandle_t file = filesystem->Open( name, "rb" );
	    
		//fseek(fp, 0, SEEK_END);
		filesystem->Seek( file, 0, FILESYSTEM_SEEK_TAIL );

		//*length = ftell(fp);
		*length = filesystem->Tell( file );

		//fseek(fp, 0, SEEK_SET);
		filesystem->Seek( file, 0, FILESYSTEM_SEEK_HEAD );
	    
		//*buff = malloc(*length);
		*buff = malloc(*length);

		//fread(*buff, *length, 1, fp);
		filesystem->Read( *buff, *length, file );
	    
		//fclose(fp);
		filesystem->Close( file );
	}

	//extern ConVar dhl_debugpenetration;
	//extern ConVar dhl_penetrationscalar;
	//extern ConVar dhl_materialspecificpenetration;
	//extern ConVar dhl_distancescaledpenetration;
	PenetrationData_t DHLShared::TestPenetration( const trace_t &tr, CBasePlayer *pPlayer, ITraceFilter* pTraceFilter, int iTimesPenetrated, float flDistance, 
		int iAmmoType, float flFudgePos /*=0.0f*/, bool bDoExitDecal /*=true*/)
	{
		PenetrationData_t nDontPenetrate( false );
		if ( !pPlayer )
			return nDontPenetrate;
		CBaseCombatWeapon* pWeapon = pPlayer->GetActiveWeapon();
		if ( !pWeapon )
			return nDontPenetrate;
		//Don't want to penetrate thin objects forever
		if ( iTimesPenetrated >= 5 )
			return nDontPenetrate;
		//If our original trace started inside something, we've got a problem.
		if ( tr.fraction == 0.0f )
			return nDontPenetrate;
		//We don't want to penetrate the skybox, weapons or displacements (which appear to cause crashes)
		if ( tr.surface.flags & SURF_SKY || tr.dispFlags & DISPSURF_FLAG_SURFACE )
			return nDontPenetrate;
		//Note that weapons use SOLID_BBOX and seem to almost always fail the retrace test, so removing this can do little good
		if ( tr.m_pEnt->GetCollisionGroup() == COLLISION_GROUP_WEAPON )
			return nDontPenetrate;
		//Seems to crash sometimes with players :(
		if ( tr.m_pEnt->IsPlayer() )
			return nDontPenetrate;

		ConVarRef dhl_debugpenetration( "dhl_debugpenetration" );
		ConVarRef dhl_penetrationscalar( "dhl_penetrationscalar" );
		ConVarRef dhl_materialspecificpenetration( "dhl_materialspecificpenetration" );
		ConVarRef dhl_distancescaledpenetration( "dhl_distancescaledpenetration" );

		bool bPenetrationDebugMode = dhl_debugpenetration.GetBool();
		Vector vecDir = tr.endpos - tr.startpos;
		VectorNormalize( vecDir );

#ifndef CLIENT_DLL
		if ( tr.m_pEnt->ClassMatches( "func_breakable_surf" ) )
		{
			CBreakableSurface *pSurfGlass = static_cast<CBreakableSurface*>( tr.m_pEnt );
			if ( pSurfGlass )
			{
				pSurfGlass->Die( pPlayer, vecDir );

				Vector vecEndpos = tr.endpos;
				PenetrationData_t nRet( true, &vecEndpos );
				return nRet;
			}
		}
#endif

		float flGlobalPenetrationScalar = 1.0f;
		if ( dhl_penetrationscalar.GetFloat() > 0.0f )
		{
			flGlobalPenetrationScalar = dhl_penetrationscalar.GetFloat();
		}
		else
		{
			Warning( "Penetration scalar must be a positive, non-zero number \n");
			dhl_penetrationscalar.SetValue( 1 );
		} 
		if ( flDistance > 3500.0f ) //Nothing should be penetrated beyond 3500 units
			return nDontPenetrate;

		CAmmoDef *pAmmoDef = GetAmmoDef();
		float flPenetrationDist = 9.0f;
		//Set the base penetration distance based on the weapon that fired the round
		if ( iAmmoType == pAmmoDef->Index("Beretta") )
			flPenetrationDist = 8.5f;
		else if ( iAmmoType == pAmmoDef->Index("Deagle") )
			flPenetrationDist = 10.5f;
		else if ( iAmmoType == pAmmoDef->Index("SAA") )
			flPenetrationDist = 11.0f;
		else if ( iAmmoType == pAmmoDef->Index("Deringer") )
			flPenetrationDist = 4.0f;
		else if ( iAmmoType == pAmmoDef->Index("Mac11") )
			flPenetrationDist = 9.5f;
		else if ( iAmmoType == pAmmoDef->Index("AK47") )
			flPenetrationDist = 10.0f;
		else if ( iAmmoType == pAmmoDef->Index("Winchester") )
			flPenetrationDist = 13.5f;
		else if ( iAmmoType == pAmmoDef->Index("Thompson") )
			flPenetrationDist = 10.5f;
		else if ( iAmmoType == pAmmoDef->Index("MosinNagant") )
			flPenetrationDist = 15.5f;
		else if ( iAmmoType == pAmmoDef->Index("Remington") )
				flPenetrationDist = 6.5f;
		else if ( iAmmoType == pAmmoDef->Index("SawedOff") )
				flPenetrationDist = 6.5f;

		if ( dhl_materialspecificpenetration.GetBool() )
		{
			surfacedata_t *surfacedata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			//Scale the base distances for different material types
			switch ( surfacedata->game.material )
			{
				case 'M': //Metal
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Metal \n" );
					flPenetrationDist *= 0.6f;
					break;
				case 'C': //Concrete
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Concrete \n" );
					flPenetrationDist *= 0.8f;
					break;
				case 'T': //Tile
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Tile \n" );
					flPenetrationDist *= 0.75f;
					break;
				case 'W': //Wood
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Wood \n" );
					flPenetrationDist *= 1.2f;
					break;
				case 'F': //Flesh
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Flesh \n" );
					flPenetrationDist *= 2.0f;
					break;
				case 'L': //Plastic
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Plastic \n" );
					flPenetrationDist *= 1.5f;
					break;
				case 'N': //Sand
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Sand \n" );
					flPenetrationDist *= 1.1f;
					break;
				case 'D': //Dirt
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Dirt \n" );
					flPenetrationDist *= 1.1f;
					break;
				case 'O': //Foliage
					if ( bPenetrationDebugMode )
						Msg("Scaling Penetration For Material: Foliage \n" );
					flPenetrationDist *= 1.0f;
					break;
				case 'I': //Clip
					if ( bPenetrationDebugMode )
						Msg("Hit a clip, not penetrating \n " );
					return nDontPenetrate;
					break;
				default: //None of these
					break;
			}
		}
		if ( dhl_distancescaledpenetration.GetBool() )
		{
			//We want a much steeper ramp for shotguns
			if ( iAmmoType == pAmmoDef->Index("Remington") || iAmmoType == pAmmoDef->Index("SawedOff") )
			{
				if ( ( (flDistance / 60.0f) * 1.0f ) > flPenetrationDist )
				{
					if ( bPenetrationDebugMode )
					{
						Msg( "Distance %f is too far, not penetrating \n", flDistance );
					}
					return nDontPenetrate;
				}
				flPenetrationDist -= (flDistance / 60.0f) * 1.0f;
				if ( bPenetrationDebugMode )
					Msg( "Reducing penetration distance by %f units for %f units travelled \n", ( (flDistance / 60.0f) * 1.0f ), flDistance );
			}
			else
			{
				if ( ( (flDistance / 200.0f) * 0.5f ) > flPenetrationDist )
				{
					if ( bPenetrationDebugMode )
						Msg( "Distance %f is too far, not penetrating \n", flDistance );
					return nDontPenetrate;
				}
				flPenetrationDist -= (flDistance / 200.0f) * 0.5f; //Lose 0.5 units of penetration distance for every 200 units travelled
				if ( bPenetrationDebugMode )
					Msg( "Reducing penetration distance by %f units for %f units travelled \n", ( (flDistance / 200.0f) * 0.5f ), flDistance );
			}
		}
		flPenetrationDist *= flGlobalPenetrationScalar;

		trace_t	penetrateTrace;
		Vector vecPenetrateDist;
		vecPenetrateDist = ( tr.endpos + ( vecDir * flPenetrationDist ) );

		//Trace forward of where we hit back to the end of the original trace
		UTIL_TraceLine( vecPenetrateDist, tr.endpos, MASK_SHOT, pTraceFilter, &penetrateTrace );
		if ( !penetrateTrace.DidHit() )
			return nDontPenetrate;
		if ( penetrateTrace.allsolid && !tr.m_pEnt->IsPlayer() )
			//Never got out of what we hit and didn't hit a player/surf glass, don't do anything.
			return nDontPenetrate;

#ifdef CLIENT_DLL
		//There's going to be nowhere to put an exit decal on surf glass, we will have destroyed the section we hit
		if ( bDoExitDecal )
			//Exit decal
			pPlayer->DoImpactEffect( penetrateTrace, GetAmmoDef()->DamageType(iAmmoType) );
#endif

		if ( bPenetrationDebugMode )
			DebugDrawLine( penetrateTrace.startpos, penetrateTrace.endpos, 255, 0, 0, false, 3.0f );

		trace_t	reTrace;

		// Re-trace as if the bullet had passed right through
		UTIL_TraceLine( penetrateTrace.endpos, tr.endpos, MASK_SHOT, pTraceFilter, &reTrace );
		
		if ( penetrateTrace.surface.flags & SURF_SKY )
			return nDontPenetrate;

		// See if we found the surface again
		if ( ( reTrace.startsolid || reTrace.fraction == 1.0f || tr.m_pEnt->IsPlayer() ) )
			return nDontPenetrate;

		Vector vecFinalPos = penetrateTrace.endpos + ( penetrateTrace.plane.normal * flFudgePos );

		PenetrationData_t nRet( true, &vecFinalPos );
		return nRet;
	}

	#ifdef USE_FMOD
	//Filename is relative to mod directory
	void DHLShared::ParseFMODExclusionScript( const char *filename )
	{
		char updatedFilename[256];
		char gameDir[256];

		//Client and server game dir functions are different for some reason
		//They should have the same output, however
		#ifdef CLIENT_DLL
			Q_strncpy( gameDir, engine->GetGameDirectory(), 256 );
		#else
			engine->GetGameDir( gameDir, 256 );
		#endif

		Q_snprintf( updatedFilename, 256, "%s\\%s", gameDir, filename );
		Q_FixSlashes( updatedFilename );

		if ( !filesystem->FileExists( updatedFilename ) )
			return;

		FileHandle_t FMODExclusionFile = filesystem->Open( filename, "r" );
		if ( FMODExclusionFile )
		{
			filesystem->Seek( FMODExclusionFile, 0, FILESYSTEM_SEEK_TAIL );
			long length = filesystem->Tell( FMODExclusionFile );
			filesystem->Seek( FMODExclusionFile, 0, FILESYSTEM_SEEK_HEAD );
			char *Buffer = new char [length+1];
			filesystem->Read( Buffer, length, FMODExclusionFile );
			filesystem->Close( FMODExclusionFile );
			/*string strInput = Buffer;
			unsigned int strpos = 0;*/
			Q_strncpy( FMODExclusionFileData, Buffer, 2047 ); //No overflows please
			delete[] Buffer;
		}
	}

	//Search our FMOD exclusion script for this file name
	bool DHLShared::IsSoundExcluded( const char* soundName )
	{
		std::string str = FMODExclusionFileData;
		std::string::size_type lastFoundPos = 0;

		//Check if the filename exists in the script
		lastFoundPos = str.find( soundName, lastFoundPos );
		while ( lastFoundPos != std::string::npos )
		{
			std::string::size_type lineStart = str.rfind( "\n", lastFoundPos );
			if ( lineStart != std::string::npos )
			{
				bool bComment = false;
				bool bValid = true;
				for ( unsigned int i = lineStart; i < (lastFoundPos - lineStart); i++ )
				{
					//Determine whether this text is commented or not
					if ( str.at( i ) == '/' )
					{
						if ( !bComment )
							bComment = true;
						if ( bComment )
						{
							bValid = false;
							break;
						}
					}
					else
						//The '/' chars have to be consecutive to signal a comment
						if ( bComment )
							bComment = false;
				}
				if ( bValid )
					return true;
				else
					lastFoundPos = str.find( soundName, lastFoundPos );
			}
		}
		return false;
	}

	//---------------------------------------------------------------------------------
	// Client only functions
	//---------------------------------------------------------------------------------
	#ifdef CLIENT_DLL
		static void CC_DumpChans( void )
		{
			CUtlVector<FMODSound_t*>* pList = DHLShared::GetPlayingChannelsList();
			Msg( "Playing channels list (Size: %i): \n", sizeof(*pList) );
			if ( pList )
			{
				for ( int i = 0; i < pList->Count(); i++ )
				{
					FMODSound_t* pSound = pList->Element( i );
					Msg( "%i - %s \n", i, pSound->name );
				}
			}
		}
		static ConCommand dhl_fmod_dumpplayingchannels( "dhl_fmod_dumpplayingchannels", CC_DumpChans, "Dump playing channel information." );

		void DHLShared::InitFMOD( void )
		{
			FMOD::System *fmod_system;
			FMOD::Channel *bgmusic_chan;

			//DHL - Skillet - Allows our DLL to be loaded from the directory of our choice
			char fmodlib[256];
			Q_snprintf( fmodlib, 256, "%s/bin", engine->GetGameDirectory() );
			Q_FixSlashes( fmodlib );
			char sOldDirectory[256];
			GetCurrentDirectory( sizeof(sOldDirectory), sOldDirectory );

			//NOTE: Delay loading must be set in the client project properties for fmodex.dll, that way it will be loaded
			//On the first call into the DLL (System_Create()) rather than on application start, giving us a chance
			//To dictate the directory we want to load from through SetCurrentDirectory

			//Set the process directory to our mod/bin path, that's where the FMOD DLL will be
			SetCurrentDirectory( fmodlib );

			//DHL - Skillet - Load up FMOD!
			FMOD_RESULT result;
			result = FMOD::System_Create(&fmod_system);

			HandleFMODError( result );

			result = fmod_system->init( 100, FMOD_INIT_NORMAL, NULL );
			HandleFMODError( result );

			//Distance factor = 39.4 (~ num of inches in a meter), FMOD uses meters by default, HL2 uses inches
			fmod_system->set3DSettings( 0.25, FMOD_UNIT_FACTOR, 1.0 );
			//We won't make it here if we have any errors
			SetFMODSystem( fmod_system );

			GetPlayingChannelsList()->RemoveAll();

			FMOD::ChannelGroup* pWeapons;
			FMOD::ChannelGroup* pMusic;
			FMOD::ChannelGroup* pOther;
			FMOD::ChannelGroup* pNoSlow;
			FMOD::ChannelGroup* pGameMaster;
			FMOD::ChannelGroup* pBGMusic;
			fmod_system->createChannelGroup( "group_weapons", &pWeapons );
			fmod_system->createChannelGroup( "group_music", &pMusic );
			fmod_system->createChannelGroup( "group_other", &pOther );
			fmod_system->createChannelGroup( "group_noslow", &pNoSlow );
			fmod_system->createChannelGroup( "group_gamemaster", &pGameMaster );
			fmod_system->createChannelGroup( "group_bgmusic", &pBGMusic );

			//This channel is parent to all in game sounds, so we can shut them down on map finish
			pGameMaster->addGroup( pWeapons );
			pGameMaster->addGroup( pMusic );
			pGameMaster->addGroup( pOther );
			pGameMaster->addGroup( pNoSlow );

			FMOD::ChannelGroup *masterChan;
			GetFMODSystem()->getMasterChannelGroup( &masterChan );
			if ( masterChan )
				masterChan->addGroup( pGameMaster );

			FMODChannelGroups newChannelGroups;
			newChannelGroups.pChanWeapons = pWeapons;
			newChannelGroups.pChanNoSlow = pNoSlow;
			newChannelGroups.pChanMusic = pMusic;
			newChannelGroups.pChanOther = pOther;
			newChannelGroups.pChanGameMaster = pGameMaster;
			newChannelGroups.pChanBGMusic = pBGMusic;

			SetChannelGroups( newChannelGroups );

			//Background music
			FMOD::Sound *sound;
			char soundPath[256];
			Q_snprintf( soundPath, 256, "%s/sound/music/ave_maria.mp3", engine->GetGameDirectory() );
			fmod_system->createStream( soundPath, FMOD_DEFAULT, 0, &sound );
			HandleFMODError( result );
			sound->setMode( FMOD_LOOP_NORMAL ); //Looping
			fmod_system->playSound( FMOD_CHANNEL_FREE, sound, true, &bgmusic_chan );
			bgmusic_chan->setChannelGroup( pBGMusic );
			bgmusic_chan->setCallback( FMOD_CHANNEL_CALLBACKTYPE_END, endCallback, 0 );
			bgmusic_chan->setPaused( false );

			FMODSound_t* soundStruct = new FMODSound_t;
			Q_strcpy( soundStruct->name, soundPath );
			soundStruct->pChannel = bgmusic_chan;
			GetPlayingChannelsList()->AddToTail( soundStruct );

			//Reset the directory after we've loaded the FMOD DLL
			SetCurrentDirectory( sOldDirectory );
		}

		//Gets the FMOD system pointer
		FMOD::System* DHLShared::GetFMODSystem( void )
		{
			return FMODSystem;
		}

		//Sets the FMOD system pointer (done on client DLL init)
		void DHLShared::SetFMODSystem( FMOD::System* System )
		{
			FMODSystem = System;
		}

		//Convert FMOD_VECTOR to Vector
		void DHLShared::FMODToVector( FMOD_VECTOR fmodInput, Vector* vecOutput )
		{
			vecOutput->x = fmodInput.x;
			vecOutput->y = fmodInput.z;
			vecOutput->z = fmodInput.y;
		}

		//Convert Vector to FMOD_VECTOR
		void DHLShared::VectorToFMOD( Vector vecInput, FMOD_VECTOR *fmodOutput )
		{
			fmodOutput->x = vecInput.x;
			fmodOutput->y = vecInput.z;
			fmodOutput->z = vecInput.y;
		}

		//Handles a (possible) error from FMOD, outputs the error message to the console if the
		//dhl_fmod_debug ConVar is set.
		//Input: FMOD_RESULT
		//Output: Returns true if there was an error
		bool DHLShared::HandleFMODError( FMOD_RESULT result )
		{
			if ( result == FMOD_OK )
				return false;
			else
			{
				//FMOD errors only displayed if CVAR is enabled
				if ( dhl_fmod_debug.GetBool() )
					Msg("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
				return true;
			}
		}

		//Sets the channel groups
		void DHLShared::SetChannelGroups( FMODChannelGroups newChannelGroups )
		{
			channelGroups = newChannelGroups;
		}

		//Returns the set of DHL FMOD channel groups in the form of an FMODChannelGroups structure
		FMODChannelGroups* DHLShared::GetChannelGroups( void )
		{
			return &channelGroups;
		}

		//Set the paused state of all channels within a channel group
		void DHLShared::ChannelGroupSetPaused( FMOD::ChannelGroup *channelGroup, bool bPaused )
		{
			if ( !channelGroup )
				return;

			int iNumChannels = 0;
			FMOD_RESULT result = channelGroup->getNumChannels( &iNumChannels );
			HandleFMODError( result );

			for ( int i = 0; i <= iNumChannels; i++ )
			{
				FMOD::Channel *channel = NULL;
				channelGroup->getChannel( i, &channel );
				if ( channel )
				{
					channel->setPaused( bPaused );
				}
			}
		}

		//Scales the frequency of all sounds within a channel group
		void DHLShared::ScaleGroupFrequency( FMOD::ChannelGroup *channelGroup, float scale )
		{
			if ( !channelGroup )
				return;

			int iNumChannels = 0;
			FMOD_RESULT result = channelGroup->getNumChannels( &iNumChannels );
			HandleFMODError( result );

			for ( int i = 0; i <= iNumChannels; i++ )
			{
				FMOD::Channel *channel = NULL;
				FMOD::Sound *sound = NULL;
				channelGroup->getChannel( i, &channel );
				if ( channel )
				{
					channel->getCurrentSound( &sound );
					if ( sound )
					{
						float oldFreq = 1.0;
						result = sound->getDefaults( &oldFreq, NULL, NULL, NULL );
						HandleFMODError( result );
						result = channel->setFrequency( oldFreq * scale );
						HandleFMODError( result );
					}
				}
			}
		}

		//Determines the scaled frequency of a sound (needed because default frequencies vary)
		float DHLShared::GetScaledFrequency( FMOD::Channel *channel, float scale )
		{
			if ( !channel || !scale )
				return 0.0f;

			FMOD::Sound *sound;
			channel->getCurrentSound( &sound );
			if ( sound )
			{
				float oldFreq = 1.0;
				sound->getDefaults( &oldFreq, NULL, NULL, NULL );
				return (oldFreq * scale);
			}
			return 0.0f;
		}

		//Designed for each channel to have only one sound playing at a time
		FMODSound_t* DHLShared::LookupFMODSound( const char *pName )
		{
			//Make sure both paths use the same type of slashes
			char pFixedName[256];
			Q_strcpy( pFixedName, pName );
			Q_FixSlashes( pFixedName );

			for ( int idx = 0; idx < playingChannelsList.Count(); idx++ )
			{
				FMOD::Sound *pSound = NULL;
				playingChannelsList.Element( idx )->pChannel->getCurrentSound( &pSound );
				if ( pSound )
				{
					char pSndName[256];
					Q_strcpy( pSndName, playingChannelsList.Element( idx )->name );
					Q_FixSlashes( pSndName );

					//If the names match...
					if ( Q_strcmp( pSndName, pFixedName ) == 0 )
						//Return a pointer to the structure
						return playingChannelsList.Element( idx );
				}
			}
			return NULL;
		}

		CUtlVector<FMODSound_t*>* DHLShared::GetPlayingChannelsList( void )
		{
			return &playingChannelsList;
		}

		FMOD_RESULT F_CALLBACK DHLShared::endCallback(FMOD_CHANNEL *channel, FMOD_CHANNEL_CALLBACKTYPE type, int command, unsigned int commanddata1, unsigned int commanddata2)
		{
			FMOD::Channel *cppchannel = (FMOD::Channel *)channel;
			if ( cppchannel )
			{
				SoundEnded( cppchannel );
			}
			return FMOD_OK;
		}

		void DHLShared::SoundEnded( FMOD::Channel* pChannel )
		{
			if ( pChannel )
			{
				for ( int i = 0; i < playingChannelsList.Count(); i++ )
				{
					FMODSound_t* pElem = playingChannelsList.Element( i );
					if ( pElem && pElem->pChannel )
					{
						if ( pElem->pChannel == pChannel )
						{
							playingChannelsList.Remove( i );
							delete pElem;
						}
					}
				}
			}
		}

		void DHLShared::FormatSoundName( char* soundName, char* relativeSoundName )
		{
			std::string str = PSkipSoundChars(soundName);

			char modDir[128];
			Q_strncpy( modDir, engine->GetGameDirectory(), 128 );
			strcat( modDir, "/sound/" );

			Q_FixSlashes( modDir );

			if ( relativeSoundName )
				Q_strncpy( relativeSoundName, str.c_str(), 128 );

			//Only if this isn't a full path already
			if ( str.find( modDir, 0 ) == std::string::npos )
				str.insert( 0, modDir );

			Q_strncpy( soundName, str.c_str(), 256 );

			Q_FixSlashes( soundName );
		}
		#endif //CLIENT_DLL
	#endif //USE_FMOD

	//---------------------------------------------------------------------------------
	// Server only functions
	//---------------------------------------------------------------------------------
	#ifndef CLIENT_DLL
		bool bIsBackgroundMap = false;
		bool DHLShared::IsBackgroundMap( void )
		{
			return bIsBackgroundMap;
		}

		void DHLShared::SetBackgroundMap( bool bState )
		{
			bIsBackgroundMap = bState;
		}
	#endif

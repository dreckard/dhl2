//Distraction Half-Life 2
//Miscellaneous useful stuff

#ifndef DHL_SHARED_H
#define DHL_SHARED_H

#include "dhl/dhl_shareddefs.h"
#include "stdstring.h"

// redd: these should be included in the files that adds this header file anyway
//#include "cbase.h"
//#include "convar.h"
#ifdef CLIENT_DLL
	#ifdef USE_FMOD
		#include "dhl/fmod/fmod.hpp"
		#define FMOD_UNIT_FACTOR 39.4f
	#else
		class CSoundControllerImp;
		#include "soundenvelope.h"
	#endif
#endif

#if defined( CLIENT_DLL ) && defined( USE_FMOD )
	//Structure containing pointers to DHL FMOD channel groups, to allow type specific
	//sound changes
	struct FMODChannelGroups
	{
		FMOD::ChannelGroup* pChanGameMaster;
		FMOD::ChannelGroup* pChanWeapons;
		FMOD::ChannelGroup* pChanMusic;
		FMOD::ChannelGroup* pChanNoSlow;
		FMOD::ChannelGroup* pChanOther;
		FMOD::ChannelGroup* pChanBGMusic;

		FMODChannelGroups()
		{
			pChanGameMaster = NULL;
			pChanWeapons = NULL;
			pChanMusic = NULL;
			pChanNoSlow = NULL;
			pChanOther = NULL;
			pChanBGMusic = NULL;
		}
	};

	//Used to build an index of playing sounds.  Needed because sounds playing from memory will not return a correct
	//Value from the FMOD::Sound::getName function
	//Designed for each channel to contain only one sound
	struct FMODSound_t
	{
		char name[256];
		FMOD::Channel* pChannel;
		int iCreatorIndex;

		FMODSound_t()
		{
			pChannel = NULL;
			iCreatorIndex = -1;
		}
	};
#endif

#if defined( CLIENT_DLL ) && !defined( USE_FMOD )
	struct SourceSound_t
	{
		int m_iGuid;
		const char *m_pScriptName;
		EmitSound_t m_EmitSnd;

		SourceSound_t()
		{
			m_iGuid = -1;
			m_pScriptName = NULL;
		}
		SourceSound_t( int iGuid, const char* szScriptName, const EmitSound_t& ep )
		{
			m_iGuid = iGuid;
			m_pScriptName = szScriptName;
			m_EmitSnd = ep;
		}
		SourceSound_t( const SourceSound_t& other )
		{
			m_iGuid = other.m_iGuid;
			m_pScriptName = other.m_pScriptName;
			m_EmitSnd = other.m_EmitSnd;
		}
	};
#endif

	struct PenetrationData_t
	{
		bool m_bShouldPenetrate;
		Vector m_vecNewBulletPos;

		PenetrationData_t()
		{
			m_bShouldPenetrate = false;
			m_vecNewBulletPos = vec3_origin;
		}
		PenetrationData_t( bool bShouldPenetrate, Vector* vecNewPos = NULL )
		{
			m_bShouldPenetrate = bShouldPenetrate;

			if ( vecNewPos )
				m_vecNewBulletPos = *vecNewPos;
		}
	};

class DHLShared
{
public:
	#ifdef CLIENT_DLL //Client
		#ifdef USE_FMOD
			static void InitFMOD( void );

			static FMOD::System* GetFMODSystem( void );
			static void SetFMODSystem( FMOD::System* System );
			static void FMODToVector( FMOD_VECTOR fmodInput, Vector* vecOutput );
			static void VectorToFMOD( Vector vecInput, FMOD_VECTOR *fmodOutput );
			static bool HandleFMODError( FMOD_RESULT result );

			static FMODSound_t* LookupFMODSound( const char *pName );

			static void SetChannelGroups( FMODChannelGroups newChannelGroups );
			static FMODChannelGroups* GetChannelGroups( void );

			static void ChannelGroupSetPaused( FMOD::ChannelGroup *channelGroup, bool bPaused );
			static void ScaleGroupFrequency( FMOD::ChannelGroup *channelGroup, float scale );
			static float GetScaledFrequency( FMOD::Channel *channel, float scale );

			static CUtlVector<FMODSound_t*>* GetPlayingChannelsList( void );

			static void FormatSoundName( char* soundName, char* relativeSoundName = NULL );

			static FMOD_RESULT F_CALLBACK endCallback(FMOD_CHANNEL *channel, FMOD_CHANNEL_CALLBACKTYPE type, int command, unsigned int commanddata1, unsigned int commanddata2);
			static void SoundEnded( FMOD::Channel* pChannel );
		#else
			//static CUtlVector<CSoundPatch*> m_SndList;
		#endif //USE_FMOD
	#else //Server
		static bool IsBackgroundMap( void );
		static void SetBackgroundMap( bool bState );
	#endif

	//http://www.codingforums.com/showthread.php?t=10827
	static inline int RoundFloat(float x)
	{
	   return int(x > 0.0 ? x + 0.5 : x - 0.5);
	}

	static PenetrationData_t TestPenetration( const trace_t &tr, CBasePlayer *pPlayer, ITraceFilter* pTraceFilter, int iTimesPenetrated, float flDistance, int iAmmoType, float flFudgePos = 0.0f, bool bDoExitDecal = true );

	//Real (unaffected by slow motion) curtime
	static float RealCurtime( void );

	static void ScaleFrametime( float flScalar = -1.0f );

	static void ScaleCurtime( float flScalar = -1.0f );

	static void GetConfigValue( const char *filename, const char *commandName, char* Array, unsigned int ArraySize );

	static void LoadFileIntoMemory( const char *name, void **buff, unsigned int *length );
	static void PrecacheModels( std::string dir ); //Precaches all .mdl files in a directory (no recurse)

	//Called from DLL and CLDLL functions
	static void Init( void );
	static void LevelInitPostEntity( void );
	static void Shutdown( void );

#ifdef USE_FMOD
	static void ParseFMODExclusionScript( const char* filename );
	static bool IsSoundExcluded( const char* soundName );
#endif
};

static const DHLShared* pDHLShared;
#endif //DHL_SHARED_H

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_SHARED_H
#define HL2MP_PLAYER_SHARED_H
#pragma once

#define HL2MP_PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)
#include "studio.h"

//DHL - Skillet - Circular dependency between these files, this should fix the inclusion order only working one way
#ifdef CLIENT_DLL
	//#include "c_hl2mp_player.h"
	class C_HL2MP_Player;
#else
	//#include "hl2mp_player.h"
	class CHL2MP_Player;
#endif


enum
{
	PLAYER_SOUNDS_CITIZEN = 0,
	PLAYER_SOUNDS_COMBINESOLDIER,
	PLAYER_SOUNDS_METROPOLICE,
	PLAYER_SOUNDS_MAX,
};

enum HL2MPPlayerState
{
	// Happily running around in the game.
	STATE_ACTIVE=0,
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.
	NUM_PLAYER_STATES
};


#if defined( CLIENT_DLL )
#define CHL2MP_Player C_HL2MP_Player
#endif

// Player avoidance
#define PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)

#endif //HL2MP_PLAYER_SHARED_h

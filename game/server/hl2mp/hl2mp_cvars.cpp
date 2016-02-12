//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2mp_cvars.h"


//DHL - Skillet - marked cvars CHEAT | HIDDEN to avoid confusion

// Ready restart
ConVar mp_readyrestart(
							"mp_readyrestart", 
							"0", 
							FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_HIDDEN,
							"If non-zero, game will restart once each player gives the ready signal" );

// Ready signal
ConVar mp_ready_signal(
							"mp_ready_signal",
							"ready",
							FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_HIDDEN,
							"Text that each player must speak for the match to begin" );
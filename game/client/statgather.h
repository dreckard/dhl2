//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: module for gathering performance stats for upload so that we can
//  monitor performance regressions and improvements
//
//=====================================================================================//


#ifndef STATGATHER_H
#define STATGATHER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"


// get the string record for sending to the server. Contains perf data and hardware/software
// info. Returns NULL if there isn't a good record to send (i.e. not enough data yet).
// A successful Get() resets the stats
char const *GetPerfStatsString( void ) ;	

// call once per frame to update internal stats. Should only call in "normal" game frames
// unless you also want to gather data on frame rates during menu displays, etc.
void UpdatePerfStats( void );

// upload performance to steam, if we have any. This is a blocking call.
void UploadPerfData( void );
#endif // STATGATHER_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "tier0/icommandline.h"

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameClients implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	// TF2V: support up to MAX_PLAYERS (128) slots -- 127 human + 1 SourceTV.
	// The old SDK comment warned against going above 32 without knowing what
	// you are doing; we handle the perf cost with adaptive entity culling
	// (CTFPlayer::ShouldTransmit) and extended respawn-wave scaling as 64-bit only.
	minplayers = 2;  // Force multiplayer.
#ifdef PLATFORM_64BITS
	maxplayers = MAX_PLAYERS;
#else
	if ( CommandLine()->HasParm("-unrestricted_maxplayers") )
	{
		static bool s_bWarned = false;
		if ( !s_bWarned )
		{
			Warning( "The use of -unrestricted_maxplayers is NOT supported and definitely NOT recommended and may be unstable.\n" );
			s_bWarned = true;
		}
		maxplayers = MAX_PLAYERS;
	}
	else
		maxplayers = 33;
#endif
	defaultMaxPlayers = 24;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}


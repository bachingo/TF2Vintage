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
	// (CTFPlayer::ShouldTransmit) and extended respawn-wave scaling.
	// Both 32-bit and 64-bit builds get the full slot count.
	minplayers = 2;            // Force multiplayer.
	maxplayers = MAX_PLAYERS;  // 128 (127 human + SourceTV)
	defaultMaxPlayers = 24;    // Sensible default for small servers.
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

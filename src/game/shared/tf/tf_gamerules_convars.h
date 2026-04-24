//=============================================================================
// tf_gamerules_convars.h
//
// Extern declarations for TF2V gameplay convars defined in
// tf_gamerules_convars.cpp.
//
// PUBLIC-FACING CONVARS (visible to server operators):
//   tf2v_era              — active era as a day-since-epoch integer
//                           (Sep 17 2007 = Day 1).  FCVAR_NOTIFY | FCVAR_ARCHIVE.
//   tf2v_use_era_mapcycle — 1 = auto-set mapcyclefile from the era table.
//                           FCVAR_NOTIFY | FCVAR_ARCHIVE.
//
// ALL OTHER CONVARS ARE HIDDEN (FCVAR_HIDDEN).
// They are set exclusively by ApplyEra() and must never be set by hand.
// Gameplay code running during a live round must read them through
// TFGameRules()->EraState() to respect the era lock, not directly.
//
// The extern declarations for hidden convars are still listed here so that
// ApplyEra(), LockEraState(), and snapshot code can reference them cleanly
// without scattering extern declarations across multiple files.
//=============================================================================
#ifndef TF_GAMERULES_CONVARS_H
#define TF_GAMERULES_CONVARS_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

// =========================================================================
// PUBLIC: ERA MANAGEMENT (two convars — the only ones operators should use)
// =========================================================================

// Active era expressed as days since September 17 2007 (beta launch = Day 1).
// Changing this drives all hidden sub-convars via ApplyEra().
extern ConVar tf2v_era;

// When 1, the mapcyclefile convar is automatically set to the era-accurate
// map list.  Replaces tf2v_enforcement level 3.
extern ConVar tf2v_use_era_mapcycle;

// =========================================================================
// HIDDEN: CERTIFICATION / COMPLIANCE TAGS (set by compliance checker only)
// =========================================================================
extern ConVar tf2v_certified;
extern ConVar tf2v_certified_partial;
extern ConVar tf2v_certified_casual;
extern ConVar tf2v_certified_competitive;
extern ConVar tf2v_quiet_server;
extern ConVar tf2v_quickplay_casual;
extern ConVar tf2v_quickplay_competitive;

#endif // TF_GAMERULES_CONVARS_H

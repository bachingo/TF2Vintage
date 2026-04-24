//=============================================================================
// tf_gamerules_convars.cpp
//
// Definitions for all TF2V gameplay convars.
//
// ERA SYSTEM — DAY-BASED EPOCH
// ============================================================
// The active era is expressed as the number of days since the TF2 beta
// opened on September 17 2007.  Day 1 = that date.
//
// Only two convars are visible to server operators:
//
//   tf2v_era              The current era day integer.
//                         Changing it calls ApplyEra() which cascades to
//                         all hidden sub-convars.
//
//   tf2v_use_era_mapcycle When 1, the mapcyclefile convar is automatically
//                         set to the era-accurate map list at each round
//                         boundary.  Replaces tf2v_enforcement level 3.
//
// EVERYTHING ELSE IS HIDDEN.
// Hidden convars are set only by ApplyEra() and must never be set by hand.
// Gameplay code reads them through TFGameRules()->EraState(), never directly.
//
// ORGANISATION:
//   Section 1  — Public: era management (2 convars)
//   Section 2  — Hidden: certification / compliance tags
//   Section 3  — Hidden: permanent server options (not era-gated)
//   Section 4  — Hidden: era sub-convars (managed by ApplyEra)
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamerules_convars.h"
#include "tf_gamerules_era_internal.h"


// =========================================================================
// SECTION 1: PUBLIC ERA MANAGEMENT
// These are the only two convars operators should ever set.
// =========================================================================

// tf2v_era — active era as days since Sep 17 2007 (Day 1).
// Defaults to TF2V_ERA_MAX (current day maximum).
ConVar tf2v_era(
    "tf2v_era",
    "6659",
    FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED,
    "Active era: days since the TF2 beta launch (September 17 2007 = Day 1).",
    true, 0, true, 6659
#ifdef GAME_DLL
    , TF2VEraChanged
#endif
);

// tf2v_use_era_mapcycle — whether the mapcyclefile is auto-set from the era.
// Replaces tf2v_enforcement level 3.  Level 1 and 2 are now implicit (balance
// and weapon gating are always active when tf2v_era is set).
ConVar tf2v_use_era_mapcycle(
    "tf2v_use_era_mapcycle",
    "0",
    FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED,
    "1 = automatically set mapcyclefile to the era-accurate map list at each "
    "round boundary.  0 = server operator controls mapcyclefile manually.",
    true, 0, true, 1
#ifdef GAME_DLL
    , TF2VMapcycleToggleChanged
#endif
);


// =========================================================================
// SECTION 2: HIDDEN — CERTIFICATION / COMPLIANCE TAGS
// Set exclusively by TF2VUpdateQuickPlayCompliance(). Never set manually.
// =========================================================================

ConVar tf2v_certified( "tf2v_certified", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if era-managed + mapcycle-managed + QuickPlay compliant." );

ConVar tf2v_certified_partial( "tf2v_certified_partial", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if era-managed (no mapcycle gate) + QuickPlay compliant." );

ConVar tf2v_certified_casual( "tf2v_certified_casual", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if certified + casual mode (crits on)." );

ConVar tf2v_certified_competitive( "tf2v_certified_competitive", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if certified + competitive mode." );

ConVar tf2v_quiet_server( "tf2v_quiet_server", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if in quiet server mode." );
	
ConVar tf2v_quickplay_casual( "tf2v_quickplay_casual", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if QuickPlay casual compliant (not certified)." );

ConVar tf2v_quickplay_competitive( "tf2v_quickplay_competitive", "0",
    FCVAR_GAMEDLL | FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Read-only. 1 if QuickPlay competitive compliant (not certified)." );


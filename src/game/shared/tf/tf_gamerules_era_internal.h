//=============================================================================
// tf_gamerules_era_internal.h
//
// Internal header shared between tf_gamerules.cpp, tf_gamerules_convars.cpp,
// tf_gamerules_era_internal.cpp, and tf_gamerules_applyera.cpp.
//
// NOT intended for inclusion outside those four TUs.
// Do NOT include this in tf_gamerules.h or any public/client header.
//
// Provides:
//   - Era integer boundary #defines (TF2V_ERA_MAX, TF2V_ERA_MAX_STR, etc.)
//   - Forward declarations for the five static convar callbacks, so that
//     tf_gamerules_convars.cpp can reference them as ConVar callback args
//     before their definitions appear in tf_gamerules_era_internal.cpp.
//   - The three static inline enforcement-level helpers (TF2V_EraManaged,
//     TF2V_WeaponGated, TF2V_MapcycleManaged), so they are available
//     regardless of which TU includes this header and regardless of link order.
//=============================================================================
#ifndef TF_GAMERULES_ERA_INTERNAL_H
#define TF_GAMERULES_ERA_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif

// -------------------------------------------------------------------------
// Era integer bounds and stringify helpers.
// -------------------------------------------------------------------------
#define TF2V_ERA_MIN            0       // PS3 internal build (~Aug 2007)
#define TF2V_ERA_MAX            200     // TF2 SDK Release (Feb 18 2025)
#define TF2V_ERA_POST_BALANCE   180     // Mar 2018 — balance freeze point
#define TF2V_ERA_VSCRIPT        190     // Dec 2022 — VScript; VSH/ZI basis
#define TF2V_ERA_ASYM_MIN       190     // Earliest era with official ASYM modes
#define TF2V_ERA_MVM_MIN        121     // Mann vs. Machine (Aug 15 2012)
#define _TF2V_STRINGIFY(x)      #x
#define TF2V_STRINGIFY(x)       _TF2V_STRINGIFY(x)
#define TF2V_ERA_MAX_STR        TF2V_STRINGIFY(TF2V_ERA_MAX)    // "200"

#ifdef GAME_DLL

#include "convar.h"

// -------------------------------------------------------------------------
// Convar callback forward declarations.
//
// The actual function bodies are defined in tf_gamerules_era_internal.cpp.
// tf_gamerules_convars.cpp needs to name these functions as ConVar callback
// arguments before the definitions are visible, so they are forward-declared
// here as static functions (translation-unit local linkage).
//
// Both tf_gamerules_convars.cpp and tf_gamerules_era_internal.cpp include
// this header, so they share the same static function identity within their
// respective TUs. The callbacks are never called across TU boundaries —
// the ConVar system holds function pointers, so this is safe.
// -------------------------------------------------------------------------
static void TF2VEraChanged         ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VEnforcementChanged ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VMapcycleModeChanged( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VServerTypeChanged  ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VAnySubConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue );

// -------------------------------------------------------------------------
// Enforcement level inline accessors.
//
// Defined here as static inline so that every TU that includes this header
// gets its own copy, independent of link order. All three read
// tf2v_enforcement, which is defined in tf_gamerules_convars.cpp and
// extern-declared here.
// -------------------------------------------------------------------------
extern ConVar tf2v_enforcement;

static inline bool TF2V_EraManaged()      { return tf2v_enforcement.GetInt() >= 1; }
static inline bool TF2V_WeaponGated()     { return tf2v_enforcement.GetInt() >= 2; }
static inline bool TF2V_MapcycleManaged() { return tf2v_enforcement.GetInt() >= 3; }

#endif // GAME_DLL

#endif // TF_GAMERULES_ERA_INTERNAL_H

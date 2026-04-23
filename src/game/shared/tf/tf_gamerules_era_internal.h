//=============================================================================
// tf_gamerules_era_internal.h
//
// Internal header shared between tf_gamerules.cpp, tf_gamerules_convars.cpp,
// tf_gamerules_era_internal.cpp, and tf_gamerules_applyera.cpp.
//
// NOT intended for inclusion outside those four TUs.
// Do NOT include this in tf_gamerules.h or any public/client header.
//
// ERA SYSTEM — DAY-BASED EPOCH
// ============================================================
// The era integer is now a count of days since the TF2 beta opened,
// September 17 2007 = Day 1.
//
//   day = (YYYY/MM/DD - 2007/09/17) + 1
//
// Examples:
//   2007/09/17  ->  1     (beta launch, Day 1)
//   2007/10/10  ->  24    (PC / Xbox 360 retail launch)
//   2010/09/30  ->  1110  (Mann-Conomy Update)
//   2017/10/20  ->  3687  (Jungle Inferno)
//   2025/07/24  ->  6521  (Summer 2025, present-day maximum)
//
// The two public-facing convars are:
//   tf2v_era            — day integer (visible, FCVAR_NOTIFY|FCVAR_ARCHIVE)
//   tf2v_use_era_mapcycle — 0/1, auto-override mapcyclefile from era table
//                           (visible, FCVAR_NOTIFY|FCVAR_ARCHIVE)
//
// All other convars (sub-balance flags, certification tags, etc.) are hidden
// (FCVAR_HIDDEN) and should never be set by server operators directly.
//
// Provides:
//   - Day-epoch #defines and the TF2V_DATE_TO_DAY compile-time helper.
//   - TF2V_ERA_MIN / TF2V_ERA_MAX in day units.
//   - Forward declarations for the five static convar callbacks.
//   - The three static inline enforcement-level helpers.
//   - TF2VDayFromDate() — runtime YYYY/MM/DD -> day integer conversion.
//=============================================================================
#ifndef TF_GAMERULES_ERA_INTERNAL_H
#define TF_GAMERULES_ERA_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif

// -------------------------------------------------------------------------
// Epoch anchor — September 17, 2007 = Day 1.
// -------------------------------------------------------------------------
#define TF2V_EPOCH_YEAR    2007
#define TF2V_EPOCH_MONTH   9
#define TF2V_EPOCH_DAY     17

// -------------------------------------------------------------------------
// Era integer bounds (in days-since-epoch).
//
//   TF2V_ERA_MIN   = 1     (Sep 17 2007 — beta launch)
//   TF2V_ERA_MAX   = 6659  (Oct  9 2025 — Scream Fortress XVII)
//
// Key named milestones (for use in ApplyEra threshold comparisons):
//
//   TF2V_ERA_DAY_LAUNCH       =   24  (Oct 10 2007 — PC/Xbox retail)
//   TF2V_ERA_DAY_GOLDRUSH     =  226  (Apr 29 2008)
//   TF2V_ERA_DAY_PYRO         =  277  (Jun 19 2008)
//   TF2V_ERA_DAY_HEAVY        =  338  (Aug 19 2008)
//   TF2V_ERA_DAY_SCOUT        =  527  (Feb 24 2009)
//   TF2V_ERA_DAY_SNIPSPY      =  613  (May 21 2009)
//   TF2V_ERA_DAY_CLASSLESS    =  697  (Aug 13 2009)
//   TF2V_ERA_DAY_WAR          =  823  (Dec 17 2009)
//   TF2V_ERA_DAY_ENGINEER     = 1026  (Jul  8 2010)
//   TF2V_ERA_DAY_MANNCONOMY   = 1110  (Sep 30 2010)
//   TF2V_ERA_DAY_AUSSIE2010   = 1188  (Dec 17 2010)
//   TF2V_ERA_DAY_UBER_F2P     = 1376  (Jun 23 2011)
//   TF2V_ERA_DAY_AUSSIE2011   = 1551  (Dec 15 2011)
//   TF2V_ERA_DAY_PYROMANIA    = 1746  (Jun 27 2012)
//   TF2V_ERA_DAY_MVM          = 1795  (Aug 15 2012 — MvM min era)
//   TF2V_ERA_DAY_LOVEANDWAR   = 2467  (Jun 18 2014)
//   TF2V_ERA_DAY_SMISSMAS2014 = 2654  (Dec 22 2014)
//   TF2V_ERA_DAY_GUNMETTLE    = 2846  (Jul  2 2015)
//   TF2V_ERA_DAY_TOUGHBREAK   = 3014  (Dec 17 2015)
//   TF2V_ERA_DAY_MYM          = 3217  (Jul  7 2016)
//   TF2V_ERA_DAY_JI           = 3687  (Oct 20 2017)
//   TF2V_ERA_DAY_FREEZE       = 3846  (Mar 28 2018 — post-balance freeze)
//   TF2V_ERA_DAY_VSCRIPT      = 5559  (Dec  5 2022 — VScript; ASYM floor)
//   TF2V_ERA_DAY_100          = 5791  (Jul 25 2023)
//   TF2V_ERA_DAY_SDK          = 6365  (Feb 18 2025)
// -------------------------------------------------------------------------
#define TF2V_ERA_DAY_BETA        	1
#define TF2V_ERA_DAY_LAUNCH         24
#define TF2V_ERA_DAY_GOLDRUSH       226
#define TF2V_ERA_DAY_PYRO           277
#define TF2V_ERA_DAY_HEAVY          338
#define TF2V_ERA_DAY_SCOUT          527
#define TF2V_ERA_DAY_SNIPSPY        613
#define TF2V_ERA_DAY_CLASSLESS      697
#define TF2V_ERA_DAY_WAR            823
#define TF2V_ERA_DAY_ENGINEER       1026
#define TF2V_ERA_DAY_MANNCONOMY     1110
#define TF2V_ERA_DAY_AUSSIE2010     1188
#define TF2V_ERA_DAY_UBER_F2P       1376
#define TF2V_ERA_DAY_AUSSIE2011     1551
#define TF2V_ERA_DAY_PYROMANIA      1746
#define TF2V_ERA_DAY_MVM            1795
#define TF2V_ERA_DAY_LOVEANDWAR     2467
#define TF2V_ERA_DAY_SMISSMAS2014   2654
#define TF2V_ERA_DAY_GUNMETTLE      2846
#define TF2V_ERA_DAY_TOUGHBREAK     3014
#define TF2V_ERA_DAY_MYM            3217
#define TF2V_ERA_DAY_JI             3687
#define TF2V_ERA_DAY_FREEZE         3846
#define TF2V_ERA_DAY_VSCRIPT        5559
#define TF2V_ERA_DAY_100            5791
#define TF2V_ERA_DAY_SDK            6365
#define TF2V_ERA_DAY_LATEST         6659

#define TF2V_ERA_MIN                TF2V_ERA_DAY_BETA
#define TF2V_ERA_MAX                TF2V_ERA_DAY_LATEST

// Stringify helpers
#define _TF2V_STRINGIFY(x)          #x
#define TF2V_STRINGIFY(x)           _TF2V_STRINGIFY(x)
#define TF2V_ERA_MIN_STR            TF2V_STRINGIFY(TF2V_ERA_MIN)    // "1"
#define TF2V_ERA_MAX_STR            TF2V_STRINGIFY(TF2V_ERA_MAX)    // "6365"

// -------------------------------------------------------------------------
// Server-type era floors (day integers).
// -------------------------------------------------------------------------
#define TF2V_ERA_MVM_MIN            TF2V_ERA_DAY_MVM       // 1795 — MvM
#define TF2V_ERA_ASYM_MIN           TF2V_ERA_DAY_VSCRIPT   // 5559 — VSH/ZI

#ifdef GAME_DLL

#include "convar.h"

// -------------------------------------------------------------------------
// Convar callback forward declarations.
// Bodies live in tf_gamerules_era_internal.cpp.
// -------------------------------------------------------------------------
void TF2VEraChanged         ( IConVar *pConVar, const char *pOldString, float flOldValue );
void TF2VMapcycleToggleChanged( IConVar *pConVar, const char *pOldString, float flOldValue );
void TF2VServerTypeChanged  ( IConVar *pConVar, const char *pOldString, float flOldValue );
void TF2VAnySubConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue );

// -------------------------------------------------------------------------
// Enforcement helpers — now derived from the two visible convars:
//   tf2v_era               — day integer > 0 means era management is active
//   tf2v_use_era_mapcycle  — 1 means the mapcycle is also era-managed
//
// "Era managed" is always true when tf2v_era > 0 (the server is running
// any era at all — there is no "enforcement 0" manual bypass in the new
// model; sub-convars are always hidden and always driven by ApplyEra).
// -------------------------------------------------------------------------
extern ConVar tf2v_era;
extern ConVar tf2v_use_era_mapcycle;

// Era management is unconditional in the new model — tf2v_era always drives
// the sub-convars. This helper exists for call-site readability only.
inline bool TF2V_EraManaged()      { return true; }

// Weapon gating is always on in the new model (hidden tf2v_allowed_weapon_era
// is driven automatically by ApplyEra, not settable by operators).
inline bool TF2V_WeaponGated()     { return true; }

// Mapcycle is managed only when the visible toggle is on.
inline bool TF2V_MapcycleManaged() { return tf2v_use_era_mapcycle.GetBool(); }

// -------------------------------------------------------------------------
// TF2VDayFromDate — runtime conversion: "YYYY/MM/DD" -> day integer.
//
// Day 1 = September 17, 2007.  Returns 0 on parse error.
// This is the authoritative runtime implementation; the compile-time
// equivalent for known milestone dates is the #define table above.
// -------------------------------------------------------------------------
int TF2VDayFromDate( const char *pszDate );

#endif // GAME_DLL

#endif // TF_GAMERULES_ERA_INTERNAL_H

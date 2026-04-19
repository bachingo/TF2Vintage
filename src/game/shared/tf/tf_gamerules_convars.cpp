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
    TF2V_ERA_MAX_STR,
    FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED,
    "Active era: days since the TF2 beta launch (September 17 2007 = Day 1). "
    "Day 24 = retail launch, Day 1110 = Mann-Conomy, Day 3687 = Jungle Inferno, "
    "Day " TF2V_ERA_MAX_STR " = current maximum (Scream Fortress XVII). "
    "Setting this automatically drives all balance flags and the weapon gate. "
    "Use the era table (tf2v_list_eras) for named update milestones.",
    true, (float)TF2V_ERA_MIN,
    true, (float)TF2V_ERA_MAX
#ifdef GAME_DLL
    , TF2VEraChanged
#endif
);

// tf2v_use_era_mapcycle — whether the mapcyclefile is auto-set from the era.
// Replaces tf2v_enforcement level 3.  Level 1 and 2 are now implicit (balance
// and weapon gating are always active when tf2v_era is set).
ConVar tf2v_use_era_mapcycle(
    "tf2v_use_era_mapcycle",
    "1",
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

ConVar tf2v_quickplay_profile( "tf2v_quickplay_profile", "1",
    FCVAR_GAMEDLL | FCVAR_HIDDEN,
    "QuickPlay/Certified opt-in. 0=off, 1=casual, 2=competitive, 3=either." );

ConVar tf2v_server_type( "tf2v_server_type", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Server type for mapcycle. 0=PVP, 1=PVE (MvM), 2=ASYM (VSH/ZI, era 5559+).",
    true, 0, true, 2
#ifdef GAME_DLL
    , TF2VServerTypeChanged
#endif
);


// =========================================================================
// SECTION 3: HIDDEN — PERMANENT SERVER OPTIONS
// Not era-gated.  Set once from server.cfg.  Never touched by ApplyEra().
// =========================================================================

ConVar tf2v_ctf_capcrits( "tf2v_ctf_capcrits", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Enable critical hits on flag capture." );

ConVar tf2v_critchance( "tf2v_critchance", "2.0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Percent chance for regular critical hits." );

ConVar tf2v_critchance_rapid( "tf2v_critchance_rapid", "2.0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Percent chance for rapid fire critical hits." );

ConVar tf2v_critchance_melee( "tf2v_critchance_melee", "2.0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Percent chance of melee critical hits." );

ConVar tf2v_crit_duration_rapid( "tf2v_crit_duration_rapid", "2.0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Duration in seconds of rapid-fire crit windows.",
    true, 0.5f, true, 5.0f );

ConVar tf2v_allcrit( "tf2v_allcrit", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=all hits are critical hits (fun/novelty option, not era-gated).",
    true, 0, true, 1 );

ConVar tf2v_randomizer( "tf2v_randomizer", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=randomize player loadouts each respawn (fun/novelty option).",
    true, 0, true, 1 );


// =========================================================================
// SECTION 4: HIDDEN — ERA SUB-CONVARS
//
// These are managed exclusively by ApplyEra() and snapshotted by
// LockEraState().  Gameplay code must NEVER read these directly during a
// round — use TFGameRules()->EraState().fieldName instead.
//
// TF2VAnySubConvarChanged is the callback on all of these; any attempt to
// change them mid-round is automatically reverted.
//
// Defaults reflect TF2V_ERA_MAX (Day 6598).
// ApplyEra() overwrites them at level init.
//
// NOTE: FCVAR_HIDDEN is intentional on every convar in this section.
// Do not remove it.
// =========================================================================

// ---- Weapon gate (hidden; always mirrors tf2v_era via ApplyEra) ----
ConVar tf2v_allowed_weapon_era( "tf2v_allowed_weapon_era",
    TF2V_ERA_MAX_STR,
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Hidden. Weapon gate ceiling — set automatically from tf2v_era.",
    true, (float)TF2V_ERA_MIN,
    true, (float)TF2V_ERA_MAX );

// ---- Damage system ----

ConVar tf2v_crit_model( "tf2v_crit_model", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=5%% base/1600 ramp (days 1-504), 1=2%% base/800 ramp (day 505+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_damage_spread_mode( "tf2v_damage_spread_mode", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=+-25%% on (days 1-504), 1=+-10%% on by default (days 505-2845), "
    "2=+-10%% off by default (day 2846+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

// ---- Fall sounds ----

ConVar tf2v_fall_sounds( "tf2v_fall_sounds", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=TFC-origin thump (days 1-276), 1=retail thump (days 277-3686), "
    "2=JI crunch + voice pain (day 3687+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

// ---- Demoman: Grenade Launcher ----

ConVar tf2v_console_grenadelauncher_damage( "tf2v_console_grenadelauncher_damage", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=PS3 GL 112 dmg (day 1), 0=PC 100 dmg (day 2+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_console_grenadelauncher_magazine( "tf2v_console_grenadelauncher_magazine", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=PS3 6-clip (day 1), 0=4-clip (day 2+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_grenades_explode_contact( "tf2v_grenades_explode_contact", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=grenades detonate on contact (days 1-11), 0=day 12+.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_grenade_player_collision( "tf2v_grenade_player_collision", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=grenades collide with players (day 23+), 0=pass through.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_grenade_radius( "tf2v_use_new_grenade_radius", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=159Hu radius (days 1-1109), 1=146Hu (day 1110+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_demo_explosion_variance( "tf2v_use_new_demo_explosion_variance", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=original +-10%% variance (days 1-504), 1=close-range nerf (days 505-3845), "
    "2=+-2%% (day 3846+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

// ---- Demoman: Stickies ----

ConVar tf2v_use_stickybomb_damage_rampup( "tf2v_use_stickybomb_damage_rampup", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=no rampup (days 1-504), 1=rampup active (day 505+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_stickybomb_radius_rampup( "tf2v_use_stickybomb_radius_rampup", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=no radius rampup (days 1-2466), 1=rampup (day 2467+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_sticky_bullet_break( "tf2v_sticky_bullet_break", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=bullets pass through stickies (days 1-337), 1=bullets break (day 338+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_targe_own_explosion( "tf2v_targe_own_explosion", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Targe blocks own explosions (days 613-1109), 0=day 1110+.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_demo_charge_debuff_remove( "tf2v_demo_charge_debuff_remove", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Charge removes debuffs on activation (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_caber( "tf2v_use_new_caber", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Caber only explodes on first hit (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_honorbound( "tf2v_use_new_honorbound", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Eyelander/Claidheamh Mor honourbound on equip (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Ammo pools ----

ConVar tf2v_ammo_era( "tf2v_ammo_era", "4",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=PS3 day 1; 1=PC day 2; 2=day 131; 3=day 207; 4=day 505+.",
    true, 0, true, 4,
    TF2VAnySubConvarChanged );

// ---- Soldier ----

ConVar tf2v_soldier_self_damage_reduction( "tf2v_soldier_self_damage_reduction", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=40%% self-damage reduction (days 1-276), 0=removed (day 277+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_gunboats_nerf( "tf2v_gunboats_nerf", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Gunboats 60%% reduction (day 1026+), 0=75%%.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_rocket_jumper_health_penalty( "tf2v_rocket_jumper_health_penalty", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Rocket Jumper -25 max HP (day 1137+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_blackbox( "tf2v_use_new_blackbox", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Black Box +15HP per hit, -1 clip (day 2467+), 0=original.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_buff_charges( "tf2v_use_new_buff_charges", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Banner charges while dealing damage (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_sentry_resist_bonus( "tf2v_sentry_resist_bonus", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Battalion's Backup provides Sentry resistance bonus (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_equalizer_damage( "tf2v_use_new_equalizer_damage", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Equalizer damage scales with HP (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_split_equalizer( "tf2v_use_new_split_equalizer", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Equalizer split into Equalizer+Escape Plan (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_speed_buff_duration( "tf2v_new_speed_buff_duration", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Buff Banner speed bonus duration extended (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_beggars( "tf2v_use_new_beggars", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Beggar's Bazooka overload reworked (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Pyro ----

ConVar tf2v_airblast( "tf2v_airblast", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=no airblast (days 1-276), 1=airblast only (days 277-3686), "
    "2=full airblast+deflect (day 3687+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_airblast_players( "tf2v_airblast_players", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=airblast can push players (day 277+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_airblast_minicrits( "tf2v_airblast_minicrits", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=airblasted projectiles deal minicrits (days 277-3686), 0=removed (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_airblast_sticky_push( "tf2v_airblast_sticky_push", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=2x push on grounded stickies (day 1110+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_afterburn_contact_time( "tf2v_afterburn_contact_time", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=afterburn duration based on flame contact time (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_afterburn_heal_debuff( "tf2v_afterburn_heal_debuff", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=afterburn disrupts Medi Gun healing (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_backburner_damage_bonus( "tf2v_backburner_damage_bonus", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Backburner +20%% damage (days 956-1304), 0=removed (day 1305+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_backburner_airblast( "tf2v_backburner_airblast", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Backburner has airblast (day 1306+), 0=no airblast.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_flame_mode( "tf2v_flame_mode", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=PS3 flame (day 1), 1=PC flame (days 2-3686), 2=JI flame (day 3687+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_minicrits_on_deflect( "tf2v_minicrits_on_deflect", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=deflected projectiles deal minicrits (day 277+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_extinguish_heal( "tf2v_use_extinguish_heal", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=extinguishing allies heals 20HP (day 2467+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_extinguish_cooldown( "tf2v_use_extinguish_cooldown", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=extinguish heal has cooldown per target (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_flare( "tf2v_use_new_flare", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Flare Gun minicrits burning targets (day 1376+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_flare_radius( "tf2v_use_new_flare_radius", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Flare Gun radius increased from 110 to 128Hu (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_phlog_fill( "tf2v_use_new_phlog_fill", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Phlogistinator fills on damage dealt (day 3687+), 0=time-based.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_phlog_taunt( "tf2v_use_new_phlog_taunt", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Phlogistinator MMMPH taunt activation (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_axtinguisher( "tf2v_use_new_axtinguisher", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Axtinguisher crits burning, no random crits (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_disable_updraft( "tf2v_disable_updraft", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=B.A.S.E. Jumper updraft removed (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_prevent_voice_spam( "tf2v_prevent_voice_spam", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=JI voice spam prevention enabled (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Heavy ----

ConVar tf2v_use_new_minigun_rampup( "tf2v_use_new_minigun_rampup", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=original (days 1-337), 1=damage rampup (days 338-2466), "
    "2=rampup+spread (day 2467+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_sandvich_behavior( "tf2v_sandvich_behavior", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=unlimited (days 1-337), 1=cooldown (days 338-1025), "
    "2=throw-to-heal (day 1026+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_natascha_fixed( "tf2v_natascha_fixed", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Natascha corrected slow/damage (day 500+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Engineer ----

ConVar tf2v_building_upgrades( "tf2v_building_upgrades", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=level 1 only (days 1-451), 1=level 2 (days 452-1025), "
    "2=level 3 (day 1026+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_building_hauling( "tf2v_building_hauling", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=buildings can be picked up and carried (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_hauling_speed( "tf2v_use_new_hauling_speed", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=hauling speed penalty reduced (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_wrench_mechanics( "tf2v_use_new_wrench_mechanics", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Wrench hits repair and upgrade (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sapper_damage( "tf2v_use_new_sapper_damage", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sapper damages buildings over time (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sapper_disable( "tf2v_use_new_sapper_disable", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sapper disables building immediately (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sentry_minigun_resist( "tf2v_use_new_sentry_minigun_resist", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sentry has minigun damage resistance (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_sentry_wrangle_location( "tf2v_new_sentry_wrangle_location", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Wrangled Sentry uses new placement logic (day 1026+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_sentry_damage_falloff( "tf2v_new_sentry_damage_falloff", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sentry damage falls off at range (day 2467+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_teleporter_cost( "tf2v_use_new_teleporter_cost", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Teleporter metal cost reduced (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_short_circuit( "tf2v_use_new_short_circuit", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Short Circuit reworked to projectile-destroy (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_minibuildings( "tf2v_use_new_minibuildings", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Mini-Sentries use new stat scaling (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_jag( "tf2v_use_new_jag", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Jag attack speed bonus reworked (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Medic ----

ConVar tf2v_use_new_medic_regen( "tf2v_use_new_medic_regen", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Medic passive regen reworked (day 2467+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_medigun_heal_rate( "tf2v_medigun_heal_rate", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Medi Gun heal rate increased at era 2467+ values.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_setup_uber_rate( "tf2v_setup_uber_rate", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=original (days 1-94), 1=setup charge gain (days 95-2465), "
    "2=charge build on damage (day 2466+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_uber_juggle_penalty( "tf2v_uber_juggle_penalty", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Uber gain paused while target is invulnerable (day 1376+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_uber_taunt( "tf2v_use_new_uber_taunt", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Medic taunt heals nearby allies (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_medic_speed_match( "tf2v_use_medic_speed_match", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Medic matches heal target speed when outpacing them (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_health_regen_attrib( "tf2v_use_new_health_regen_attrib", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=health regen attribute uses new formula (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_uber_range_falloff( "tf2v_uber_range_falloff", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Uber charge drains faster at range (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_quick_fix_weapon_restriction( "tf2v_quick_fix_weapon_restriction", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=no primary during Quick-Fix Uber (days 1376-2466), 0=allowed (day 2467+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Scout ----

ConVar tf2v_sandman_stun_type( "tf2v_sandman_stun_type", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=full stun (days 527-2845), 1=slow stun (days 2846-3686), "
    "2=mini stun + HP drain (day 3687+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bonk_length( "tf2v_use_new_bonk_length", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Bonk! invuln shortened (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sodapopper_hype( "tf2v_use_new_sodapopper_hype", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Soda Popper hype meter reworked (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sodapopper_fill( "tf2v_use_new_sodapopper_fill", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Soda Popper hype fills on damage (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_manual_sodapopper( "tf2v_use_manual_sodapopper", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Soda Popper hype requires manual activation (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_shortstop_shove( "tf2v_use_shortstop_shove", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Shortstop has shove mechanic (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_shortstop_slowdown( "tf2v_use_shortstop_slowdown", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Shortstop victim slowdown active (days 1746-2845 and day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_guillotine( "tf2v_use_new_guillotine", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Flying Guillotine reworked to charge mechanic (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_ball_regen( "tf2v_use_new_ball_regen", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sandman/Wrap Assassin ball regen on catch (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_fan_damage_bonus( "tf2v_fan_damage_bonus", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=FaN +10%% damage (day 631+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_dead_ringer_flag_carry( "tf2v_dead_ringer_flag_carry", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Dead Ringer can be activated while carrying the flag (day 631+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Spy ----

ConVar tf2v_spy_cloak_reload( "tf2v_spy_cloak_reload", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=cloak recharges from ammo pickups (days 1-22), 0=passive (day 23+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_spy_cloak_ammo_recharge( "tf2v_spy_cloak_ammo_recharge", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=cloak recharges passively and from ammo crates (day 452+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_cloak( "tf2v_use_new_cloak", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Cloak grants 20%% damage resistance (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_feign_death_activate( "tf2v_new_feign_death_activate", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=90%% resist (days 613-2845), 1=50%% (day 2846), 2=75%% (day 3217+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_feign_death_stealth( "tf2v_new_feign_death_stealth", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Dead Ringer cloak resist scales with cloak meter (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_yer( "tf2v_use_new_yer", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Your Eternal Reward gives full cloak on disguise (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_big_earner( "tf2v_use_new_big_earner", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Big Earner grants speed boost on backstab (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_fast_redisguise( "tf2v_use_fast_redisguise", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=instant redisguise after backstab (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_allow_disguiseweapons( "tf2v_allow_disguiseweapons", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Spy can change disguise weapon display (day 527+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_disguise_spy_teleport( "tf2v_disguise_spy_teleport", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Spy can use enemy teleporters while disguised (day 697+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_disguise_speed_match( "tf2v_disguise_speed_match", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Spy moves at disguised class speed (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_spy_movespeeds( "tf2v_use_new_spy_movespeeds", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Alias for tf2v_disguise_speed_match. 1=Spy matches class speed (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_spy_base_speed( "tf2v_spy_base_speed", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=300HU/s (days 1-1375), 1=320HU/s (day 1376+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_ambassador( "tf2v_use_new_ambassador", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=full crit any range (days 613-822), 1=range falloff (days 823-3845), "
    "2=minicrits only (day 3846+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_diamondback( "tf2v_use_new_diamondback", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Diamondback stores crits on backstab (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_pomson( "tf2v_use_new_pomson", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Pomson 6000 reduced drain with distance falloff (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Sniper ----

ConVar tf2v_sniper_zoom_mode( "tf2v_sniper_zoom_mode", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=no restrictions (days 1-130). 1=re-zoom lock (day 131). "
    "2=re-zoom lock + 200ms crit delay (day 151+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_allow_sniper_crosshairs( "tf2v_allow_sniper_crosshairs", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Sniper can toggle crosshair visibility (day 697+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_cleaners( "tf2v_use_new_cleaners", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Cleaner's Carbine fills CRIKEY meter on kill (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bison_damage( "tf2v_use_new_bison_damage", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=2011 Bison (days 1376-3216), 1=2016 values (day 3217), 2=2017 values (day 3687+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bison_speed( "tf2v_use_new_bison_speed", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Bison projectile 30%% slower (day 3217+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

// ---- Cross-class ----

ConVar tf2v_use_new_autofire( "tf2v_use_new_autofire", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=hold-and-release fire for certain weapons (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_weapon_swap_speed( "tf2v_use_new_weapon_swap_speed", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=0.50s weapon holster time (day 1110+), 0=0.67s.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_fast_weapon_switch( "tf2v_fast_weapon_switch", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=global weapon draw time reduced to 0.50s (day 3014+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_reload_cancel_available( "tf2v_reload_cancel_available", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=clip reload can be cancelled by firing (day 697+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_faster_reload( "tf2v_use_faster_reload", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=auto-reload enabled by default (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_new_chocolate_behavior( "tf2v_new_chocolate_behavior", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Dalokohs Bar raises max HP by 50 (day 2846+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_atomizer( "tf2v_use_new_atomizer", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=Atomizer third jump requires active weapon (day 3687+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_use_new_backstabs( "tf2v_use_new_backstabs", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=no raise check (day 1), 1=facing check (day 39+), 2=no stab delay (day 527+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

// ---- Movement ----

ConVar tf2v_radius_damage_teammates( "tf2v_radius_damage_teammates", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=teammates absorb radius damage (days 1-696), 0=pass through (day 697+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_clamp_speed_absolute( "tf2v_clamp_speed_absolute", "520",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "Absolute speed cap HU/s. 450=days 1-1745, 520=day 1746+.",
    true, 300, true, 600,
    TF2VAnySubConvarChanged );

ConVar tf2v_class_death_animations( "tf2v_class_death_animations", "2",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=none (days 1-612), 1=Heavy+Sniper only (days 613-696), 2=all (day 697+).",
    true, 0, true, 2,
    TF2VAnySubConvarChanged );

ConVar tf2v_minicrit_self_inflicted( "tf2v_minicrit_self_inflicted", "0",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=self-inflicted minicrits possible (days 1-696), 0=removed (day 697+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_clamp_airducks( "tf2v_clamp_airducks", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "1=air ducking formalised, can duck twice in air (day 537+).",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

ConVar tf2v_pistol_fixed_firerate( "tf2v_pistol_fixed_firerate", "1",
    FCVAR_REPLICATED | FCVAR_HIDDEN,
    "0=days 1-696: Pistol semi-automatic. 1=day 697+: Pistol fixed auto-fire rate.",
    true, 0, true, 1,
    TF2VAnySubConvarChanged );

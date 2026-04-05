//=============================================================================
// tf_gamerules_convars.h
//
// Extern declarations for all TF2V gameplay convars defined in
// tf_gamerules_convars.cpp.
//
// Any file that needs to read a tf2v_* convar should include this header
// instead of writing its own extern ConVar declaration.
//
// Usage:
//   #include "tf_gamerules_convars.h"
//   if ( tf2v_airblast.GetInt() >= 1 ) { ... }
//
// NOTE: Gameplay code running during a live round should NEVER read these
//       convars directly. Use TFGameRules()->EraState().fieldName instead.
//       Direct reads bypass the era lock and allow mid-round exploits.
//       The extern declarations here are for ApplyEra(), LockEraState(),
//       snapshot functions, and server-side setup code only.
//=============================================================================
#ifndef TF_GAMERULES_CONVARS_H
#define TF_GAMERULES_CONVARS_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"

// =========================================================================
// ERA MANAGEMENT
// =========================================================================
extern ConVar tf2v_era;
extern ConVar tf2v_enforcement;
extern ConVar tf2v_allowed_weapon_era;
extern ConVar tf2v_quickplay_profile;
extern ConVar tf2v_server_type;

// =========================================================================
// CERTIFICATION / COMPLIANCE TAGS (read-only, set by compliance checker)
// =========================================================================
extern ConVar tf2v_certified;
extern ConVar tf2v_certified_partial;
extern ConVar tf2v_certified_casual;
extern ConVar tf2v_certified_competitive;
extern ConVar tf2v_certified_ps3;
extern ConVar tf2v_certified_xbox;
extern ConVar tf2v_quickplay_casual;
extern ConVar tf2v_quickplay_competitive;

// =========================================================================
// PERMANENT SERVER OPTIONS
// Not era-gated. Set once from server.cfg.
// =========================================================================
extern ConVar tf2v_ctf_capcrits;
extern ConVar tf2v_critchance;
extern ConVar tf2v_critchance_rapid;
extern ConVar tf2v_critchance_melee;
extern ConVar tf2v_crit_duration_rapid;
extern ConVar tf2v_individual_classlimit;
extern ConVar tf2v_allcrit;
extern ConVar tf2v_randomizer;

// =========================================================================
// ERA SUB-CONVARS
// Managed by ApplyEra(). Read via EraState() during a round.
// =========================================================================

// ---- Damage system ----
extern ConVar tf2v_crit_model;
extern ConVar tf2v_damage_spread_mode;

// ---- Fall sounds ----
extern ConVar tf2v_fall_sounds;

// ---- Demoman: Grenade Launcher ----
extern ConVar tf2v_console_grenadelauncher_damage;
extern ConVar tf2v_console_grenadelauncher_magazine;
extern ConVar tf2v_grenades_explode_contact;
extern ConVar tf2v_grenade_player_collision;
extern ConVar tf2v_use_new_grenade_radius;
extern ConVar tf2v_use_new_demo_explosion_variance;

// ---- Demoman: Stickies ----
extern ConVar tf2v_use_stickybomb_damage_rampup;
extern ConVar tf2v_use_stickybomb_radius_rampup;
extern ConVar tf2v_sticky_bullet_break;
extern ConVar tf2v_targe_own_explosion;
extern ConVar tf2v_demo_charge_debuff_remove;
extern ConVar tf2v_use_new_caber;
extern ConVar tf2v_use_new_honorbound;

// ---- Ammo pools ----
extern ConVar tf2v_ammo_era;

// ---- Soldier ----
extern ConVar tf2v_soldier_self_damage_reduction;
extern ConVar tf2v_gunboats_nerf;
extern ConVar tf2v_rocket_jumper_health_penalty;
extern ConVar tf2v_use_new_blackbox;
extern ConVar tf2v_use_new_buff_charges;
extern ConVar tf2v_sentry_resist_bonus;
extern ConVar tf2v_use_new_equalizer_damage;
extern ConVar tf2v_use_new_split_equalizer;
extern ConVar tf2v_new_speed_buff_duration;
extern ConVar tf2v_use_new_beggars;

// ---- Pyro ----
extern ConVar tf2v_airblast;
extern ConVar tf2v_airblast_players;
extern ConVar tf2v_airblast_minicrits;
extern ConVar tf2v_airblast_sticky_push;
extern ConVar tf2v_afterburn_contact_time;
extern ConVar tf2v_afterburn_heal_debuff;
extern ConVar tf2v_backburner_damage_bonus;
extern ConVar tf2v_backburner_airblast;
extern ConVar tf2v_flame_mode;
extern ConVar tf2v_minicrits_on_deflect;
extern ConVar tf2v_use_extinguish_heal;
extern ConVar tf2v_use_extinguish_cooldown;
extern ConVar tf2v_use_new_flare;
extern ConVar tf2v_use_new_flare_radius;
extern ConVar tf2v_use_new_phlog_fill;
extern ConVar tf2v_use_new_phlog_taunt;
extern ConVar tf2v_use_new_axtinguisher;
extern ConVar tf2v_disable_updraft;
extern ConVar tf2v_prevent_voice_spam;

// ---- Heavy ----
extern ConVar tf2v_use_new_minigun_rampup;
extern ConVar tf2v_sandvich_behavior;
extern ConVar tf2v_natascha_fixed;

// ---- Engineer ----
extern ConVar tf2v_building_upgrades;
extern ConVar tf2v_building_hauling;
extern ConVar tf2v_use_new_hauling_speed;
extern ConVar tf2v_use_new_wrench_mechanics;
extern ConVar tf2v_use_new_sapper_damage;
extern ConVar tf2v_use_new_sapper_disable;
extern ConVar tf2v_use_new_sentry_minigun_resist;
extern ConVar tf2v_new_sentry_wrangle_location;
extern ConVar tf2v_new_sentry_damage_falloff;
extern ConVar tf2v_use_new_teleporter_cost;
extern ConVar tf2v_use_new_short_circuit;
extern ConVar tf2v_use_new_minibuildings;
extern ConVar tf2v_use_new_jag;

// ---- Medic ----
extern ConVar tf2v_use_new_medic_regen;
extern ConVar tf2v_medigun_heal_rate;
extern ConVar tf2v_setup_uber_rate;
extern ConVar tf2v_uber_juggle_penalty;
extern ConVar tf2v_use_new_uber_taunt;
extern ConVar tf2v_use_medic_speed_match;
extern ConVar tf2v_use_new_health_regen_attrib;
extern ConVar tf2v_uber_range_falloff;
extern ConVar tf2v_quick_fix_weapon_restriction;

// ---- Scout ----
extern ConVar tf2v_sandman_stun_type;
extern ConVar tf2v_use_new_bonk_length;
extern ConVar tf2v_use_new_sodapopper_hype;
extern ConVar tf2v_use_new_sodapopper_fill;
extern ConVar tf2v_use_manual_sodapopper;
extern ConVar tf2v_use_shortstop_shove;
extern ConVar tf2v_use_shortstop_slowdown;
extern ConVar tf2v_use_new_guillotine;
extern ConVar tf2v_use_new_ball_regen;
extern ConVar tf2v_fan_damage_bonus;
extern ConVar tf2v_dead_ringer_flag_carry;

// ---- Spy ----
extern ConVar tf2v_spy_cloak_reload;
extern ConVar tf2v_spy_cloak_ammo_recharge;
extern ConVar tf2v_use_new_cloak;
extern ConVar tf2v_new_feign_death_activate;
extern ConVar tf2v_new_feign_death_stealth;
extern ConVar tf2v_use_new_yer;
extern ConVar tf2v_use_new_big_earner;
extern ConVar tf2v_use_fast_redisguise;
extern ConVar tf2v_allow_disguiseweapons;
extern ConVar tf2v_disguise_spy_teleport;
extern ConVar tf2v_disguise_speed_match;
extern ConVar tf2v_use_new_spy_movespeeds;
extern ConVar tf2v_spy_base_speed;
extern ConVar tf2v_use_new_ambassador;
extern ConVar tf2v_use_new_diamondback;
extern ConVar tf2v_use_new_pomson;

// ---- Sniper ----
extern ConVar tf2v_sniper_zoom_mode;
extern ConVar tf2v_allow_sniper_crosshairs;
extern ConVar tf2v_use_new_cleaners;
extern ConVar tf2v_use_new_bison_damage;
extern ConVar tf2v_use_new_bison_speed;

// ---- Cross-class ----
extern ConVar tf2v_use_new_autofire;
extern ConVar tf2v_use_new_weapon_swap_speed;
extern ConVar tf2v_fast_weapon_switch;
extern ConVar tf2v_reload_cancel_available;
extern ConVar tf2v_use_faster_reload;
extern ConVar tf2v_new_chocolate_behavior;
extern ConVar tf2v_use_new_atomizer;
extern ConVar tf2v_use_new_backstabs;

// ---- Movement ----
extern ConVar tf2v_radius_damage_teammates;
extern ConVar tf2v_clamp_speed_absolute;
extern ConVar tf2v_class_death_animations;
extern ConVar tf2v_minicrit_self_inflicted;
extern ConVar tf2v_clamp_airducks;
extern ConVar tf2v_pistol_fixed_firerate;

#endif // TF_GAMERULES_CONVARS_H

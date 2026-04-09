//=============================================================================
// tf_gamerules_convars.cpp
//
// Definitions for all TF2V gameplay convars.
//
// Split from tf_gamerules.cpp so that:
//   - The main gamerules file stays focused on CTFGameRules implementation.
//   - Adding or tweaking a convar doesn't require touching the 20k+ line
//     gamerules file.
//   - tf_gamerules_applyera.cpp and any other TU can get extern declarations
//     cleanly via tf_gamerules_convars.h.
//
// ORGANISATION:
//   Section 1  — Era management (tf2v_era, tf2v_enforcement, etc.)
//   Section 2  — Certification / compliance tags (read-only)
//   Section 3  — Permanent server options (not era-gated)
//   Section 4  — Era sub-convars (managed by ApplyEra, read via EraState)
//
// See tf_gamerules_applyera.cpp for the full era cascade documentation.
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"               // TF2V_ERA_MAX_STR and era #defines
#include "tf_gamerules_convars.h"        // our own extern declarations
#include "tf_gamerules_era_internal.h"  // callback fwd decls + inline helpers


// =========================================================================
// SECTION 1: ERA MANAGEMENT
// =========================================================================

ConVar tf2v_era( "tf2v_era", TF2V_ERA_MAX, FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Active era integer. Round numbers = major balance eras (10=GoldRush, "
	"20=Pyro, 30=Heavy, 40=Feb09, 50=Scout, 60=SnipSpy, 70=Classless, "
	"80=WAR, 90=Engi, 100=Mann, 110=Uber/F2P, 120=Pyromania, 130=L&W, "
	"140=GunMettle, 150=ToughBreak, 160=MYM, 170=JI, 180=Mar2018). "z
	"Intermediate integers = content drops between major eras. "
	"0=PS3internal 4=PClaunch/Xbox."
#ifdef GAME_DLL
	, TF2VEraChanged
#endif
);

ConVar tf2v_enforcement( "tf2v_enforcement", "3",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Era enforcement level. "
	"0=manual (no gating). "
	"1=era managed balance only. "
	"2=partial: balance + weapon gate (partial certification eligible). "
	"3=strict: balance + weapon gate + era mapcycle (full certification eligible).",
	true, 0, true, 3
#ifdef GAME_DLL
	, TF2VEnforcementChanged
#endif
);

ConVar tf2v_allowed_weapon_era( "tf2v_allowed_weapon_era", TF2V_ERA_MAX,
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Weapon gate ceiling. Only relevant when tf2v_enforcement is 0 (full manual). "
	"At enforcement 1+ the gate is driven automatically from tf2v_era. "
	"Items with min_era above this value are replaced with stock." );

ConVar tf2v_quickplay_profile( "tf2v_quickplay_profile", "1",
	FCVAR_NOTIFY | FCVAR_GAMEDLL,
	"QuickPlay/Certified opt-in. 0=off, 1=casual, 2=competitive, 3=either." );

ConVar tf2v_server_type( "tf2v_server_type", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Server type for mapcycle and certification. "
	"0=PVP (standard), 1=PVE (MvM, tf_mvm_defenders_team_size controls human cap), "
	"2=ASYM (VSH/ZI, era 200 only).",
	true, 0, true, 2
#ifdef GAME_DLL
	, TF2VServerTypeChanged
#endif
);


// =========================================================================
// SECTION 2: CERTIFICATION / COMPLIANCE TAGS
// Set by TF2VUpdateQuickPlayCompliance(). Never set manually.
// =========================================================================

ConVar tf2v_certified( "tf2v_certified", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if enforcement 3 + valid era + QuickPlay compliant. Full certification." );

ConVar tf2v_certified_partial( "tf2v_certified_partial", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if enforcement 2 + valid era + QuickPlay compliant. Partial certification." );

ConVar tf2v_certified_casual( "tf2v_certified_casual", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if certified + casual mode (crits on)." );

ConVar tf2v_certified_competitive( "tf2v_certified_competitive", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if certified + competitive mode." );

ConVar tf2v_certified_ps3( "tf2v_certified_ps3", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if certified + casual + era 0 + 16 players (PS3 build)." );

ConVar tf2v_certified_xbox( "tf2v_certified_xbox", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if certified + era 4 + 16 players (Xbox 360 build)." );

ConVar tf2v_quickplay_casual( "tf2v_quickplay_casual", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if QuickPlay casual compliant (not certified)." );

ConVar tf2v_quickplay_competitive( "tf2v_quickplay_competitive", "0",
	FCVAR_NOTIFY | FCVAR_GAMEDLL | FCVAR_REPLICATED,
	"Read-only. 1 if QuickPlay competitive compliant (not certified)." );


// =========================================================================
// SECTION 3: PERMANENT SERVER OPTIONS
// Not era-gated. Set once from server.cfg. Never touched by ApplyEra().
// =========================================================================

ConVar tf2v_ctf_capcrits( "tf2v_ctf_capcrits", "1",
	FCVAR_REPLICATED,
	"Enable critical hits on flag capture." );

ConVar tf2v_critchance( "tf2v_critchance", "2.0",
	FCVAR_REPLICATED,
	"Percent chance for regular critical hits." );

ConVar tf2v_critchance_rapid( "tf2v_critchance_rapid", "2.0",
	FCVAR_REPLICATED,
	"Percent chance for rapid fire critical hits." );

ConVar tf2v_critchance_melee( "tf2v_critchance_melee", "2.0",
	FCVAR_REPLICATED,
	"Percent chance of melee critical hits." );

ConVar tf2v_crit_duration_rapid( "tf2v_crit_duration_rapid", "2.0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Duration in seconds of rapid-fire crit windows.",
	true, 0.5f, true, 5.0f );

ConVar tf2v_allcrit( "tf2v_allcrit", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=all hits are critical hits (fun/novelty option, not era-gated).",
	true, 0, true, 1 );

ConVar tf2v_randomizer( "tf2v_randomizer", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=randomize player loadouts each respawn (fun/novelty option).",
	true, 0, true, 1 );


// =========================================================================
// SECTION 4: ERA SUB-CONVARS
//
// These are managed by ApplyEra() and snapshotted by LockEraState().
// Gameplay code must NEVER read these directly during a round — use
// TFGameRules()->EraState().fieldName instead. Direct reads bypass the
// era lock and allow mid-round exploits.
//
// All sub-convars use TF2VAnySubConvarChanged as their callback so that
// any attempt to change them mid-round is automatically reverted.
//
// Defaults reflect the current TF2V_ERA_MAX state (era 200).
// ApplyEra() will overwrite them at level init if tf2v_enforcement >= 1.
// =========================================================================

// ---- Damage system ----

ConVar tf2v_crit_model( "tf2v_crit_model", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Crit probability model. 0=5%% base/1600 ramp (eras 1-39), 1=2%% base/800 ramp (era 40+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_damage_spread_mode( "tf2v_damage_spread_mode", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=+-25%% on (eras 1-39), 1=+-10%% on by default (eras 40-139), "
	"2=+-10%% off by default (eras 140+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

// ---- Fall sounds ----

ConVar tf2v_fall_sounds( "tf2v_fall_sounds", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=TFC-origin thump (eras 1-19), 1=retail thump (eras 20-169), "
	"2=JI crunch + voice pain (eras 170+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

// ---- Demoman: Grenade Launcher ----

ConVar tf2v_console_grenadelauncher_damage( "tf2v_console_grenadelauncher_damage", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=PS3 Grenade Launcher 112 damage (era 0 only), 0=PC 100 damage (era 1+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_console_grenadelauncher_magazine( "tf2v_console_grenadelauncher_magazine", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=PS3 Grenade Launcher 6-clip magazine (era 0 only), 0=PC 4-clip (era 1+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_grenades_explode_contact( "tf2v_grenades_explode_contact", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=grenades detonate on contact (eras 0-1), 0=era 2+.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_grenade_player_collision( "tf2v_grenade_player_collision", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=grenades pass through players after first bounce (eras 0-2), 1=collide (era 3+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_grenade_radius( "tf2v_use_new_grenade_radius", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=146Hu grenade blast radius (era 100+), 0=159Hu (eras 1-99).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_demo_explosion_variance( "tf2v_use_new_demo_explosion_variance", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Demo explosion variance. 0=+-10%% (eras 1-39), 1=close-range nerf (eras 40-179), "
	"2=+-2%% (eras 180+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

// ---- Demoman: Stickies ----

ConVar tf2v_use_stickybomb_damage_rampup( "tf2v_use_stickybomb_damage_rampup", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=stickybombs deal reduced damage before 2s arming (era 40+), 0=no rampup.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_stickybomb_radius_rampup( "tf2v_use_stickybomb_radius_rampup", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=stickybomb blast radius also ramps up over 2s (era 130+), 0=no radius rampup.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_sticky_bullet_break( "tf2v_sticky_bullet_break", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=bullets destroy stickybombs (era 31+), 0=bullets pass through.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_targe_own_explosion( "tf2v_targe_own_explosion", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Chargin Targe protects against own explosive damage (eras 60-99), 0=removed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_demo_charge_debuff_remove( "tf2v_demo_charge_debuff_remove", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=shield charge removes debuffs (era 150+), 0=no removal.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_caber( "tf2v_use_new_caber", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Ullapool Caber has reduced explosion after first hit (era 150+), 0=original.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_honorbound( "tf2v_use_new_honorbound", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Eyelander can be holstered with HP penalty (era 150+), 0=cannot holster.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Ammo pools ----

ConVar tf2v_ammo_era( "tf2v_ammo_era", "4",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=PS3 1=PCbeta 2=Launch 3=Feb28_2008 4=Feb02_2009. "
	"Controls GL clip/reserve, Sticky reserve, RL reserve.",
	true, 0, true, 4,
	TF2VAnySubConvarChanged );

// ---- Soldier ----

ConVar tf2v_soldier_self_damage_reduction( "tf2v_soldier_self_damage_reduction", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Soldier takes 40%% less self-damage from own rockets (eras 0-19), "
	"0=removed at Pyro Update (era 20+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_gunboats_nerf( "tf2v_gunboats_nerf", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=Gunboats 75%% self-damage reduction (eras 80-89), "
	"1=60%% reduction (era 90+, Engineer Update).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_rocket_jumper_health_penalty( "tf2v_rocket_jumper_health_penalty", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Rocket Jumper applies -25 max HP penalty while equipped (era 91+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_blackbox( "tf2v_use_new_blackbox", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Black Box heals +20HP per attack (era 110+), 0=+15HP per hit.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_buff_charges( "tf2v_use_new_buff_charges", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Banner charges build on damage taken (era 120+), 0=damage dealt.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_sentry_resist_bonus( "tf2v_sentry_resist_bonus", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Battalion's Backup grants +15%% sentry damage resist (era 120+), 0=no bonus.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_equalizer_damage( "tf2v_use_new_equalizer_damage", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Equalizer reduced damage (era 120+), 0=original damage+speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_split_equalizer( "tf2v_use_new_split_equalizer", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Equalizer and Escape Plan are separate weapons (era 120+), 0=shared reskin.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_speed_buff_duration( "tf2v_new_speed_buff_duration", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Disciplinary Action speed buff lasts 2s (era 130+), 0=3s.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_beggars( "tf2v_use_new_beggars", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Beggar's Bazooka deducts ammo per rocket (era 150+), 0=no deduction.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Pyro ----

ConVar tf2v_airblast( "tf2v_airblast", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=no airblast (eras 1-19), 1=pre-JI airblast (eras 20-169), "
	"2=post-JI momentum airblast (eras 170+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_airblast_players( "tf2v_airblast_players", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=airblast can push players (era 20+), 0=projectiles only.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_airblast_minicrits( "tf2v_airblast_minicrits", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=targets launched by airblast take minicrits (eras 20-169), "
	"0=removed at Jungle Inferno (era 170+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_airblast_sticky_push( "tf2v_airblast_sticky_push", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=airblast pushes grounded stickies approximately 2x further (era 100+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_afterburn_contact_time( "tf2v_afterburn_contact_time", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=afterburn duration based on flame contact time, min 3s to max 10s (era 170+), "
	"0=fixed afterburn duration.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_afterburn_heal_debuff( "tf2v_afterburn_heal_debuff", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=afterburn disrupts Medic healing and resist shields by 20%% (era 170+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_backburner_damage_bonus( "tf2v_backburner_damage_bonus", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Backburner has +20%% damage bonus (era 82+), 0=no bonus.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_backburner_airblast( "tf2v_backburner_airblast", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Backburner can airblast (era 104+), 0=no-airblast attribute active.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_flame_mode( "tf2v_flame_mode", "6",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=era 0 broken. 1=era 2 fixed. 2=era 20 Pyro. 3=era 21. "
	"4=era 90. 5=era 120. 6=era 170 JI.",
	true, 0, true, 6,
	TF2VAnySubConvarChanged );

ConVar tf2v_minicrits_on_deflect( "tf2v_minicrits_on_deflect", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=deflected projectiles minicrit (era 90+), 0=no minicrit.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_extinguish_heal( "tf2v_use_extinguish_heal", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=extinguishing a burning player heals 20HP (era 100+), 0=no heal.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_extinguish_cooldown( "tf2v_use_extinguish_cooldown", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=extinguishing reduces jar cooldown by 20%% (era 100+), 0=no reduction.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_flare( "tf2v_use_new_flare", "4",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Flare Gun mode. 0=eras 20-89, 1=eras 90-99, 2=eras 100-109, "
	"3=eras 110-129, 4=eras 130+.",
	true, 0, true, 4,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_flare_radius( "tf2v_use_new_flare_radius", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=110Hu Flare Gun blast radius (era 110+), 0=92Hu.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_phlog_fill( "tf2v_use_new_phlog_fill", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Phlogistinator fill threshold. 0=225 damage (eras 120-169), "
	"1=300 damage (eras 170+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_phlog_taunt( "tf2v_use_new_phlog_taunt", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Phlogistinator taunt. 0=full heal, 1=heal+Uber, 2=Uber only (era 170+), "
	"3=Uber+immunity.",
	true, 0, true, 3,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_axtinguisher( "tf2v_use_new_axtinguisher", "3",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Axtinguisher model. 0=original (eras 20-129), 1=L&W, 2=Tough Break, 3=JI (era 170+).",
	true, 0, true, 3,
	TF2VAnySubConvarChanged );

ConVar tf2v_disable_updraft( "tf2v_disable_updraft", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=B.A.S.E.Jumper updraft removed (era 150+), 0=bug active (eras 130-149).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_prevent_voice_spam( "tf2v_prevent_voice_spam", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=limit voice command spam rate (era 170+), 0=no limit.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Heavy ----

ConVar tf2v_use_new_minigun_rampup( "tf2v_use_new_minigun_rampup", "3",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Minigun mode. 0=no spinup (eras 1-29), 1=spinup (eras 30-129), "
	"3=full rampup both axes (era 130+).",
	true, 0, true, 3,
	TF2VAnySubConvarChanged );

ConVar tf2v_sandvich_behavior( "tf2v_sandvich_behavior", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=120HP no throw (era 30), 1=300HP+throw (eras 50-119 and 100+), "
	"2=300HP no self-throw (era 120+), 3=cooldown (eras 90-99).",
	true, 0, true, 3,
	TF2VAnySubConvarChanged );

ConVar tf2v_natascha_fixed( "tf2v_natascha_fixed", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=Natascha shipped with inverted slow (eras 30-31), "
	"1=corrected slow and proper damage (era 32+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Engineer ----

ConVar tf2v_building_upgrades( "tf2v_building_upgrades", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=Sentry only upgrades (eras 1-30), 1=all buildings to L3 (eras 31-89), "
	"2=all buildings + hauling (era 90+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_building_hauling( "tf2v_building_hauling", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Engineer can haul buildings (era 90+), 0=no hauling.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_hauling_speed( "tf2v_use_new_hauling_speed", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=10%% hauling speed penalty (era 140+), 0=25%% penalty (eras 90-139).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_wrench_mechanics( "tf2v_use_new_wrench_mechanics", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=F2P wrench repair values (era 100+), 0=original.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sapper_damage( "tf2v_use_new_sapper_damage", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=33%% sap damage resist (era 100+), 0=66%%.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sapper_disable( "tf2v_use_new_sapper_disable", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=sapper briefly disables building on apply (era 100+), 0=no disable.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sentry_minigun_resist( "tf2v_use_new_sentry_minigun_resist", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=L2/L3 sentry 15%%/20%% minigun resist (era 100+), 0=20%%/33%%.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_sentry_wrangle_location( "tf2v_new_sentry_wrangle_location", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=wrangled sentry uses sentry position for damage falloff (era 100+), "
	"0=Engineer position.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_sentry_damage_falloff( "tf2v_new_sentry_damage_falloff", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=sentry damage falloff extends to max range (era 140+), 0=original range.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_teleporter_cost( "tf2v_use_new_teleporter_cost", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=reduced teleporter metal cost (era 160+), 0=original cost.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_short_circuit( "tf2v_use_new_short_circuit", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Short Circuit fires energy ball (era 150+), 0=energy blast.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_minibuildings( "tf2v_use_new_minibuildings", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=mini-buildings can be repaired by wrench (era 160+), 0=not repairable.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_jag( "tf2v_use_new_jag", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Jag has reduced sapper damage bonus (era 150+), 0=faster swing speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Medic ----

ConVar tf2v_use_new_medic_regen( "tf2v_use_new_medic_regen", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=3-6HP/s passive regen (era 10+), 0=1-3HP/s.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_medigun_heal_rate( "tf2v_medigun_heal_rate", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=original lower rates (eras 1-9), 1=24-72HP/s scaling (era 10+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_setup_uber_rate( "tf2v_setup_uber_rate", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=faster Uber charge rate during setup phase (era 8+), 0=no bonus.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_uber_juggle_penalty( "tf2v_uber_juggle_penalty", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Uber drains faster per extra target held (era 9+), 0=no extra drain.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_uber_taunt( "tf2v_use_new_uber_taunt", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Kritz drains 25%%/75%% on stab/retract (era 100+), 0=50%% on retract.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_medic_speed_match( "tf2v_use_medic_speed_match", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Medic matches heal target movement speed (era 140+), 0=own speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_health_regen_attrib( "tf2v_use_new_health_regen_attrib", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=damage-time based health regen attribute (era 180+), 0=flat rate.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_uber_range_falloff( "tf2v_uber_range_falloff", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Uber and cloak drain decrease over distance from target "
	"(512Hu start, zero at 1536Hu) (era 140+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_quick_fix_weapon_restriction( "tf2v_quick_fix_weapon_restriction", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Quick-Fix cannot use primary weapons while Uber is deployed "
	"(eras 110-129), 0=restriction removed (era 130+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Scout ----

ConVar tf2v_sandman_stun_type( "tf2v_sandman_stun_type", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=full stun+Uber drain (era 50), 1=no Uber stun (eras 70-169), "
	"2=slowdown only (era 170+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bonk_length( "tf2v_use_new_bonk_length", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=8s Bonk duration (era 180+), 0=6s.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sodapopper_hype( "tf2v_use_new_sodapopper_hype", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Soda Popper hype gives 5 dashes (era 130+), 0=minicrits.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_sodapopper_fill( "tf2v_use_new_sodapopper_fill", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Soda Popper hype fills by damage dealt (era 130+), 0=by distance run.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_manual_sodapopper( "tf2v_use_manual_sodapopper", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Soda Popper requires manual alt-fire activation (era 150+), 0=auto-activates.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_shortstop_shove( "tf2v_use_shortstop_shove", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Shortstop has alt-fire shove (era 140+), 0=no shove.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_shortstop_slowdown( "tf2v_use_shortstop_slowdown", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Shortstop slows hit targets (eras 120-139 only), 0=no slowdown.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_guillotine( "tf2v_use_new_guillotine", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Flying Guillotine reduces cooldown on bleed hit (era 130+), "
	"0=minicrit on bleed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_ball_regen( "tf2v_use_new_ball_regen", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Wrap Assassin ball regens 33%% faster (era 100+), 0=standard regen.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_fan_damage_bonus( "tf2v_fan_damage_bonus", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Force-A-Nature has +10%% damage bonus (era 61+), 0=no bonus.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_dead_ringer_flag_carry( "tf2v_dead_ringer_flag_carry", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Dead Ringer can be activated while carrying the Intelligence (era 61+), "
	"0=cannot activate with flag.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Spy ----

ConVar tf2v_spy_cloak_reload( "tf2v_spy_cloak_reload", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Spy can reload revolver while cloaked (eras 0-2), 0=cannot (era 3+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_spy_cloak_ammo_recharge( "tf2v_spy_cloak_ammo_recharge", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=cloak recharges from ammo pickups (era 31+), 0=dispenser/spawn only.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_cloak( "tf2v_use_new_cloak", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Cloak grants 20%% damage resist (era 140+), 0=no resist.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_feign_death_activate( "tf2v_new_feign_death_activate", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Dead Ringer activation resist. 0=90%% (eras 60-149), 1=50%% (era 150), "
	"2=75%% (era 160+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_feign_death_stealth( "tf2v_new_feign_death_stealth", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Dead Ringer cloak resist scales with cloak meter (era 160+), 0=flat resist.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_yer( "tf2v_use_new_yer", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Your Eternal Reward gives full cloak on disguise (era 150+), 0=standard.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_big_earner( "tf2v_use_new_big_earner", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Big Earner grants speed boost on backstab (era 150+), 0=no boost.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_fast_redisguise( "tf2v_use_fast_redisguise", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=instant redisguise after backstab (era 150+), 0=standard speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_allow_disguiseweapons( "tf2v_allow_disguiseweapons", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Spy can change disguise weapon display (era 50+), 0=fixed weapon shown.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_disguise_spy_teleport( "tf2v_disguise_spy_teleport", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Spy can use enemy teleporters while disguised (era 70+), 0=cannot.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_disguise_speed_match( "tf2v_disguise_speed_match", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Spy moves at disguised class speed (era 160+), 0=own speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// Compatibility alias for tf2v_disguise_speed_match.
ConVar tf2v_use_new_spy_movespeeds( "tf2v_use_new_spy_movespeeds", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Alias for tf2v_disguise_speed_match. 1=Spy matches class speed (era 160+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_spy_base_speed( "tf2v_spy_base_speed", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=300HU/s (eras 1-109), 1=320HU/s (era 110+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_ambassador( "tf2v_use_new_ambassador", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=full crit any range (eras 60-89), 1=range falloff (eras 90-179), "
	"2=minicrits only (era 180+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_diamondback( "tf2v_use_new_diamondback", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Diamondback stores crits on backstab (era 150+), 0=no storage.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_pomson( "tf2v_use_new_pomson", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Pomson 6000 has reduced drain with distance falloff (era 150+), 0=original.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Sniper ----

ConVar tf2v_sniper_zoom_mode( "tf2v_sniper_zoom_mode", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=no restrictions (eras 0-6). 1=re-zoom lock (era 7, Jan 15 2008). "
	"2=re-zoom lock + 200ms zoom-to-crit delay (era 7+, Feb 14 2008).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_allow_sniper_crosshairs( "tf2v_allow_sniper_crosshairs", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Sniper can toggle crosshair visibility (era 70+), 0=option unavailable.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_cleaners( "tf2v_use_new_cleaners", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Cleaner's Carbine fills CRIKEY meter on kill (era 150+), 0=minicrit on kill.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bison_damage( "tf2v_use_new_bison_damage", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=2011 Bison (eras 104-159), 1=2016 damage (era 160), 2=2017 values (era 170+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_bison_speed( "tf2v_use_new_bison_speed", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Bison projectile is 30%% slower (era 160+), 0=original speed.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

// ---- Cross-class ----

ConVar tf2v_use_new_autofire( "tf2v_use_new_autofire", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=certain weapons require hold-and-release to fire (era 150+), 0=immediate.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_weapon_swap_speed( "tf2v_use_new_weapon_swap_speed", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=0.50s weapon holster time (era 100+), 0=0.67s.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_fast_weapon_switch( "tf2v_fast_weapon_switch", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=global weapon draw time reduced 0.67s to 0.50s (era 150+). "
	"Distinct from tf2v_use_new_weapon_swap_speed (holster time, era 100).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_reload_cancel_available( "tf2v_reload_cancel_available", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=clip reload can be cancelled by firing (era 70+), 0=cannot cancel.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_faster_reload( "tf2v_use_faster_reload", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=auto-reload enabled by default (era 140+), 0=off by default.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_new_chocolate_behavior( "tf2v_new_chocolate_behavior", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Dalokohs Bar raises max HP by 50 (era 140+), 0=standard 200HP cap.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_atomizer( "tf2v_use_new_atomizer", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=Atomizer third jump requires active weapon (era 170+), 0=always available.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_use_new_backstabs( "tf2v_use_new_backstabs", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=no raise check (era 0), 1=facing check (era 7+), 2=no stab delay (era 50+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

// ---- Movement ----

ConVar tf2v_radius_damage_teammates( "tf2v_radius_damage_teammates", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=teammates absorb radius damage and break rocket jumps (eras 1-69), "
	"0=pass through (era 70+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_clamp_speed_absolute( "tf2v_clamp_speed_absolute", "520",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Absolute speed cap in HU/s. 450=eras 1-119, 520=era 120+.",
	true, 300, true, 600,
	TF2VAnySubConvarChanged );

ConVar tf2v_class_death_animations( "tf2v_class_death_animations", "2",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=none (eras 1-59), 1=Heavy+Sniper only (eras 60-69), 2=all classes (era 70+).",
	true, 0, true, 2,
	TF2VAnySubConvarChanged );

ConVar tf2v_minicrit_self_inflicted( "tf2v_minicrit_self_inflicted", "0",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=self-inflicted minicrits possible (eras 1-69), 0=removed (era 70+).",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_clamp_airducks( "tf2v_clamp_airducks", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"1=air ducking formalised, can duck twice in air (era 51+), "
	"0=mid-air ducking not available as intended feature.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

ConVar tf2v_pistol_fixed_firerate( "tf2v_pistol_fixed_firerate", "1",
	FCVAR_NOTIFY | FCVAR_REPLICATED,
	"0=eras 0-69: Pistol semi-automatic, requires new input edge per shot. "
	"1=era 70+: Pistol fires at fixed automatic rate regardless of input.",
	true, 0, true, 1,
	TF2VAnySubConvarChanged );

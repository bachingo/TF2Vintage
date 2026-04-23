//=============================================================================
// CTFGameRules::ApplyEra( int nEra )
//
// Single source of truth for all era-gated convar values.
// Parameter is INTEGER matching the weapon_min_era table.
//
// Called by:
//   - TF2VEraChanged() callback when tf2v_era is set
//   - TF2VEraManagedChanged() when tf2v_era_managed flips to 1
//   - RoundRespawn() when m_bEraDirty is true
//
// INTEGER ERA MAPPING (major balance eras = round tens):
//   0   = ps3_internal       (~Aug 2007)
//   1   = beta_sep17          (Sep 17 2007) PC beta opens; GL clip 4 hotfix
//         absorbed (Sep 20 patch, same continuous beta window — no stable
//         intermediate state worth running a server on)
//   2   = beta_sep28          (Sep 28 2007) contact-explode removed, flame fix
//   3   = beta_oct9           (Oct 9 2007)  grenade collision + spy cloak reload
//   4   = launch_oct2007      (Oct 10 2007) PC launch / Xbox 360
//   5   = oct25_2007          (Oct 25 2007) Backstab facing check fix
//   6   = dec20_2007          (Dec 20 2007) Medi Gun setup charge
//   7   = jan15_2008          (Jan 15 2008) Sniper re-zoom delay enforced;
//         Feb 14 2008 200ms zoom-to-crit delay (same era slot, 4-week window)
//   8   = feb28_2008          (Feb 28 2008) reserves nerfed
//   9   = apr02_2008          (Apr 2  2008) Uber multi-target drain faster
//   10  = gold_rush           (Apr 29 2008) Gold Rush — Medic unlocks
//   11  = may21_2008          (May 21 2008) Uber weapon-switch exploit fixed;
//         rapid-fire crit sync fixed (no new convars — engine-level)
//   -- 12-19 free --
//   20  = pyro_update         (Jun 19 2008) Pyro unlocks + airblast;
//         Soldier self-damage reduction removed
//   21  = jul01_2008          (Jul 1  2008) Flamethrower falloff restored
//   -- 22-29 free --
//   30  = heavy_update        (Aug 19 2008) Heavy unlocks
//   31  = dec11_2008          (Dec 11 2008) Disp/Tele upgrades + cloak ammo
//   32  = jan28_2009          (Jan 28 2009) Natascha damage/slow fixed
//   -- 33-39 free --
//   40  = feb02_2009          (Feb 2  2009) Crit rework + spread nerf
//   -- 41-49 free --
//   50  = scout_update        (Feb 24 2009) Scout unlocks; initial disguise
//         weapon display added
//   51  = mar06_2009          (Mar 6  2009) Air ducking formalised
//   -- 52-59 free --
//   60  = sniper_spy          (May 21 2009) Sniper/Spy unlocks
//   61  = jun08_2009          (Jun 8  2009) FaN +10%, Flare minicrit on
//         burning, Dead Ringer flag carry
//   -- 62-69 free --
//   70  = classless           (Aug 13 2009) KOTH + reload cancel + cap crits;
//         pistol fixed fire rate
//   -- 71-79 free (confirmed clean window) --
//   80  = war_update          (Dec 17 2009) WAR! unlocks
//   81  = community_1         (Mar 18 2010) Pain Train, Homewrecker, Dalokohs
//   82  = apr29_2010          (Apr 29 2010) Backburner +20% damage bonus
//         (119th Update); also Crit-a-Cola weapon addition
//   83  = community_2         (May 20 2010) Skullcutter, Tribalman's Shiv
//   90  = engineer_update     (Jul 8 2010)  Engineer unlocks + hauling;
//         Gunboats 75%->60%; Sandvich cooldown added
//   91  = scream_2010         (Oct 27 2010) Frying Pan; Rocket Jumper
//         health penalty added
//   -- 92-99 free --
//   100 = mannconomy          (Sep 30 2010) Mannconomy unlocks; Sandvich
//         cooldown removed; Shortstop slowdown removed (1st removal);
//         Targe own-explosion removed; airblast sticky push 2x
//   101 = polycount           (Oct 6 2010)  Polycount Pack
//   102 = manniversary_2010   (Oct 13 2010) Fan O'War etc.
//   103 = aus_xmas_2010       (Dec 17 2010) Australian Christmas;
//         cp_degrootkeep + Medieval Mode added to map pool.
//         Minigun spin-up increased; Natascha distance slowdown reduced.
//         Shortstop slowdown removed (first removal, re-added era 120).
//         NOTE: era 100 mapcycle file covers 100-109; era 103 is the
//         map pool split — a separate mapcycle_era103.txt should be
//         created to distinguish pre/post degrootkeep rotation.
//   104 = community_3         (Feb-Apr 2011) RIFT promo + Community 3
//   105 = apr14_2011          (Apr 14 2011) Backburner gains airblast
//   -- 105-109 free --
//   110 = uber_f2p            (Jun 23 2011) F2P + Uber Update; Spy 320 HU/s;
//         Flare Gun radius 110Hu; Quick-Fix weapon restriction added
//   111 = manniversary_2011   (Oct 13 2011) Scottish Handshake etc.
//   112 = halloween_2011      (Oct 27 2011) Unarmed Combat etc.
//   113 = aus_xmas_2011       (Dec 15 2011) Australian Christmas + Festives
//   115 = the_original        (Mar 2012)    The Original
//   116 = grordborts_2        (Apr 2012)    Widowmaker, Machina, Diamondback
//   117 = oct2012_pack        (Oct 26 2012) Loose Cannon, Rescue Ranger,
//         Vaccinator
//   120 = pyromania           (Jun 27 2012) Pyromania unlocks; Shortstop
//         slowdown re-added
//   121 = aug2012_pack        (Aug 15 2012) MVM launch + Flying Guillotine etc.
//   -- 122-129 free (confirmed clean) --
//   130 = love_and_war        (Jun 18 2014) Love & War; minigun/sticky
//         damage rampup; sentry range falloff; Quick-Fix restriction removed;
//         Two Cities Nov 2013 MVM (same era slot)
//   133 = smissmas_2014       (Dec 22 2014) Mannpower beta gamemode
//   -- 131-139 free (confirmed clean) --
//   140 = gun_mettle          (Jul 2 2015)  Gun Mettle balance pass;
//         Uber range falloff; weapon switch 0.5s; Shortstop slowdown removed
//   -- 141-149 free (confirmed clean) --
//   150 = tough_break         (Dec 17 2015) Tough Break balance pass
//   -- 151-159 free (confirmed clean) --
//   160 = meet_your_match     (Jul 7 2016)  Meet Your Match
//   -- 161-169 free (confirmed clean) --
//   170 = jungle_inferno      (Oct 20 2017) JI unlocks + balance;
//         airblast no-minicrit; contact-time afterburn; afterburn heal debuff
//   -- 171-179 free (confirmed clean) --
//   180 = march_2018          (Mar 28 2018) Terminal balance state
//   181-189 = Scream Fortress/Smissmas 2018-2022 (map/content only,
//             no balance changes — holiday and rotational events)
//   190 = vscript             (Dec 1 2022)  VScript update; basis for
//         VSH and ZI official gamemodes; ASYM minimum era
//   191 = summer_2023_vsh     (Jul 12 2023) Summer 2023 — VSH official
//   192 = sf_xv_zi            (Oct 9 2023)  Scream Fortress XV — ZI official
//   193 = sf_xvi_tow          (Oct 2024)    Scream Fortress XVI — Tug of War
//   194 = smissmas_2024       (Dec 2024)    Smissmas 2024
//   200 = sdk_release         (Feb 18 2025) TF2 SDK Release — TF2V_ERA_MAX
//   201 = sf_xvii_htf         (Oct 2025)    Scream Fortress XVII — Hold the Flag
//
// WEAPON ERA GATING:
//   tf2v_allowed_weapon_era mirrors nEra automatically.
//   Replaces tf2v_allowed_year_weapons and tf2v_force_year_weapons.
//   Server can override tf2v_allowed_weapon_era after ApplyEra() runs
//   when tf2v_era_managed is 0 (manual mode).
//
// REMOVED FROM ERA MANAGEMENT (permanent server options):
//   tf2v_allow_reskins      — era-independent server toggle
//   tf2v_allow_demoknights  — era-independent server toggle
//   tf2v_allowed_year_weapons / tf2v_force_year_weapons — replaced by
//                             tf2v_allowed_weapon_era
//
// Convar status legend:
//   [EXISTS]    = in premerge, needs porting to merge
//   [MERGE]     = already in merge
//   [NEW]       = not in either branch, needs implementing
//   [RENAME]    = exists under old name, rename on port
//   [TRUNCATED] = multiple old convars collapsed into one
//=============================================================================
#include "cbase.h"

#ifdef GAME_DLL
#include "tf_gamerules.h"
#include "tf_gamerules_convars.h"
#include "tf_gamerules_era_internal.h"
#include "tf_player.h"

extern ConVar tf2v_quickplay_profile;


void CTFGameRules::ApplyEra( int nEra )
{
    m_bApplyingEra = true;
/*
    // =========================================================================
    // SECTION 1: PERMANENT SERVER OPTIONS
    // Not era-gated. Set once from server.cfg. Never touched by ApplyEra.
    // =========================================================================
    //
    // -- Win / round config --
    //   mp_timelimit, mp_winlimit, mp_windifference, mp_windifference_min
    //   mp_maxrounds, tf_flag_caps_per_round
    //
    // -- Team config --
    //   mp_autoteambalance, mp_teams_unbalance_limit
    //   tf_tournament_classlimit_*, tf2v_individual_classlimit
    //
    // -- Access / visibility --
    //   sv_alltalk, mp_forcecamera, mp_friendlyfire
    //   tf2v_allow_thirdperson, tf2v_enforce_whitelist
    //   hide_server, sv_password
    //
    // -- Physics --
    //   sv_gravity (default 800), sv_airaccelerate (default 10)
    //
    // -- Holidays --
    //   tf_birthday, tf_halloween, tf_christmas, tf_forced_holiday
    //
    // -- Damage tuning (not era-gated) --
    //   tf_damage_lineardist, tf_damage_range
    //   tf2v_bonus_distance_range
    //   tf_fall_damage_disablespread   (TF2V-only, no era equivalent)
    //   tf_use_fixed_weaponspreads     (competitive server setting)
    //
    // -- Crit tuning (constants set in Section 2, not era-gated) --
    //   tf2v_critchance_melee          (constant 15%)
    //   tf2v_crit_duration_rapid       (constant 2s)
    //   tf2v_player_misses, tf2v_misschance
    //
    // -- Item filters (server choice, not era-gated) --
    //   tf2v_allow_cosmetics, tf2v_misc_slot_count
    //   tf2v_allow_reskins             (era-independent)
    //   tf2v_allow_demoknights         (era-independent)
    //   tf2v_allow_cut_weapons
    //   tf2v_allow_multiclass_weapons
    //   tf2v_allow_mod_weapons
    //   tf2v_allowed_year_cosmetics, tf2v_force_year_cosmetics
    //   tf2v_legacy_items
    //
    // -- Audio options (TF2V additions, no confirmed era dates) --
    //   tf2v_generic_voice_death
    //   tf2v_generic_voice_medic
    //
    // -- Fun / novelty options (TF2V inventions, no era) --
    //   tf2v_randomizer, tf2v_random_classes, tf2v_random_weapons
    //   tf2v_unrestrict_random_weapons, tf2v_force_melee
    //   tf2v_allcrit
    //   tf2v_homing_rockets, tf2v_homing_deflected_rockets
    //   tf2v_autojump, tf2v_duckjump, tf2v_bunnyjump_max_speed_factor
    //   tf2v_groundspeed_cap, tf2v_clamp_speed, tf2v_clamp_speed_difference
    //   tf2v_assault_ctf_rules, tf2v_attrib_mult
    //   tf2v_explosive_dispensers
    //   tf_enable_grenades             (TFC hangover, server-opt)
    //   tf2v_remove_loser_disguise, tf2v_overflow_ammo
    //   tf2v_use_new_ammo_drops, tf2v_teleport_bread
    //   tf2v_use_dispenser_touch       (no official era equivalent)
    //   tf2v_use_spawn_glows
    //   tf2v_blastjump_only_airborne
    //   tf2v_allow_objective_glow_ctf, tf2v_allow_objective_glow_pl
    //   tf2v_force_flame_visual        (client preference)
    //   tf2v_disguise_break_touch      (TFC hangover, server-opt)
    //   tf2v_allow_spy_sprint          (TF2V invention)
    //   tf2v_use_spy_moveattrib        (TF2V invention)
    //   tf_preround_push_from_damage_enable (launch bug, server-managed)
    //   tf2v_backburner_health_buff    (brief era 20 only, server-opt)
    //
    // -- QuickPlay / platform (separate systems) --
    //   tf2v_quickplay_profile
    //   tf2v_platform



    // =========================================================================
    // SECTION 3: BASELINE — ERA 1 (PS3 INTERNAL ~Aug 2007)
    // Every convar set to its earliest known state.
    // Cascade in Section 4 only sets what changed at each era.
    // =========================================================================

    // ---- WEAPON ERA GATE ----
    // [NEW] Automatically gates item loading to weapons with min_era <= this value.
    // Replaces tf2v_allowed_year_weapons + tf2v_force_year_weapons.
    tf2v_allowed_weapon_era.SetValue( nEra );        // [NEW]

    // ---- DAMAGE SYSTEM ----
    // [NEW] 0=±25% (eras 1-39), 1=±10% on by default (eras 40-139),
    //       2=±10% off by default (eras 140+)
    tf2v_damage_spread_mode.SetValue( 0 );           // [NEW]

    // ---- CRITICAL HIT SYSTEM ----
    // [NEW] bool: 0=5% base/1600 ramp (eras 1-39), 1=2% base/800 ramp (eras 40+)
    tf2v_crit_model.SetValue( 0 );                   // [NEW]

    // [MERGE] CTF cap crits. 0=eras 1-69, 1=eras 70+
    tf2v_ctf_capcrits.SetValue( 0 );                 // [MERGE] corrected default

    // [MERGE] Arena first blood. 0=eras 1-69 (convar unavailable), 1=eras 70+
    tf_arena_first_blood.SetValue( 0 );              // [MERGE]

    // ---- FALL SOUNDS ----
    // [EXISTS->RENAME] tf2v_use_new_fallsounds -> tf2v_fall_sounds
    // 0=TFC-origin thump (eras 1-19, exact retail swap date unknown)
    // 2=retail crunch + voice pain (eras 170+, JI confirmed)
    // Mode 1 (retail thump, no voice) has no confirmed patch date —
    // set at era 20 as best available approximation.
    tf2v_fall_sounds.SetValue( 0 );                  // [EXISTS]

    // ---- DEMOMAN: GRENADE LAUNCHER ----
    // [EXISTS] 1=PS3 112dmg (era 1), 0=PC 100dmg (eras 2+)
    tf2v_console_grenadelauncher_damage.SetValue( 1 );   // [EXISTS]
    // [EXISTS] 1=PS3 6-clip (era 1), 0=4-clip (eras 2+)
    tf2v_console_grenadelauncher_magazine.SetValue( 1 ); // [EXISTS]

    // [NEW in code] 1=contact-explode on first bounce (eras 1-4), 0=eras 5+
    tf2v_grenades_explode_contact.SetValue( 1 );     // [NEW in code]

    // [NEW] 0=pass through players/buildings (eras 1-5), 1=collide (eras 6+)
    tf2v_grenade_player_collision.SetValue( 0 );     // [NEW]

    // [EXISTS in cfg] 0=159Hu radius (eras 1-99), 1=146Hu (eras 100+)
    tf2v_use_new_grenade_radius.SetValue( 0 );       // [EXISTS]

    // [EXISTS] 0=original ±10% variance (eras 1-39),
    //          1=close-range nerf (eras 40-179), 2=±2% (eras 180+)
    tf2v_use_new_demo_explosion_variance.SetValue( 0 );  // [EXISTS]

    // [EXISTS] 0=no rampup nerf (eras 1-39), 1=nerf active (eras 40+)
    tf2v_use_stickybomb_damage_rampup.SetValue( 0 );     // [EXISTS]

    // [EXISTS] 0=no radius rampup (eras 1-129), 1=rampup (eras 130+)
    tf2v_use_stickybomb_radius_rampup.SetValue( 0 );     // [EXISTS]

    // [NEW] 0=bullets pass through stickies (eras 1-30), 1=bullets break (eras 31+)
    tf2v_sticky_bullet_break.SetValue( 0 );          // [NEW]

    // ---- AMMO POOLS ----
    // [EXISTS->EXPAND] tf2v_era_ammocounts -> tf2v_ammo_era (int 0-4)
    // 0=PS3 era 1  (GL clip 6, GL res 40, Sticky res 30, RL res 36)
    // 1=PC  era 2  (GL clip 4, GL res 40, Sticky res 30, RL res 36)
    // 2=    era 7  (GL clip 4, GL res 30, Sticky res 40, RL res 36)
    // 3=    era 9  (GL clip 4, GL res 16, Sticky res 24, RL res 16)
    // 4=    era 40 (GL clip 4, GL res 16, Sticky res 24, RL res 20)
    tf2v_ammo_era.SetValue( 0 );                     // [EXISTS->EXPAND]

    // ---- SOLDIER ----
    // [NEW] Gunboats self-damage reduction nerfed.
    // 0=eras 80-89: 75% reduction (launch)
    // 1=era 90+:    60% reduction (Engineer Update)
    tf2v_gunboats_reduction.SetValue( 75 );          // [NEW]

    // [NEW] Rocket Jumper health penalty.
    // 0=eras 80-90: no health penalty
    // 1=era 91+:    -25 max HP while equipped
    tf2v_rocket_jumper_health_penalty.SetValue( 0 ); // [NEW]

    // [NEW] Soldier self-damage reduction from own rockets.
    // 1=eras 0-19: Soldier takes 40% less damage from own rockets
    // 0=era 20+:   removed at Pyro Update (Jun 19 2008)
    tf2v_soldier_self_damage_reduction.SetValue( 1 ); // [NEW]

    // [EXISTS] 0=+15HP/hit (eras 1-109), 1=+20HP/attack (eras 110+)
    tf2v_use_new_blackbox.SetValue( 0 );             // [EXISTS]

    // [NEW] Airblast minicrits on launched targets.
    // 1=eras 20-169: airblast-launched targets take minicrits
    // 0=era 170+:    removed at Jungle Inferno
    tf2v_airblast_minicrits.SetValue( 1 );           // [NEW]

    // [NEW] Afterburn uses contact-time model.
    // 0=eras 20-169: fixed afterburn duration
    // 1=era 170+:    min 3s to max 10s based on flame contact
    tf2v_afterburn_contact_time.SetValue( 0 );       // [NEW]

    // [NEW] Afterburn disrupts Medic healing/resist shields.
    // 0=eras 20-169: no heal disruption
    // 1=era 170+:    20% disruption on afterburn
    tf2v_afterburn_heal_debuff.SetValue( 0 );        // [NEW]

    // [NEW] Airblast pushes grounded stickies 2x further.
    // 0=eras 20-99:  normal push distance
    // 1=era 100+:    doubled push distance
    tf2v_airblast_sticky_push.SetValue( 0 );         // [NEW]

    // ---- PYRO: FLAMETHROWER ----
    // [EXISTS] 0=no airblast (eras 1-19), 1=pre-JI (eras 20-169), 2=post-JI (eras 170+)
    tf2v_airblast.SetValue( 0 );                     // [EXISTS]
    tf2v_airblast_players.SetValue( 0 );             // [EXISTS]

    // [NEW->TRUNCATED] Collapses tf2v_new_flame_damage + related.
    // 0=era 1 (broken hit detection)
    // 1=eras 2-19 (fixed, buffed point-blank, Sep 28)
    // 2=era 20 (Pyro Update values)
    // 3=eras 21+ (close-range falloff restored, Jul 2008)
    // 4=eras 90+ (all deflections minicrit, Engineer Update)
    // 5=eras 120+ (+10% base damage, Pyromania)
    // 6=eras 170+ (JI calculations)
    tf2v_flame_mode.SetValue( 0 );                   // [NEW]

    // [EXISTS] 0=eras 1-89, 1=eras 90+ (deflected projectiles minicrit)
    tf2v_minicrits_on_deflect.SetValue( 0 );         // [EXISTS]

    // [EXISTS] 0=eras 1-99 (no heal), 1=eras 100+ (20HP on extinguish)
    tf2v_use_extinguish_heal.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=eras 1-99 (no reduction), 1=eras 100+ (-20% jar cooldown)
    tf2v_use_extinguish_cooldown.SetValue( 0 );      // [EXISTS]

    // [EXISTS] Flare Gun mode (weapon unavailable before era 20, value irrelevant).
    // 0=eras 20-89, 1=eras 90-99, 2=eras 100-109, 3=eras 110-129, 4=eras 130+
    tf2v_use_new_flare.SetValue( 0 );                // [EXISTS]

    // [EXISTS] 0=92Hu (eras 20-109), 1=110Hu (eras 110+)
    tf2v_use_new_flare_radius.SetValue( 0 );         // [EXISTS]

    // [EXISTS] 0=225dmg fill (eras 120-169), 1=300dmg fill (eras 170+)
    tf2v_use_new_phlog_fill.SetValue( 0 );           // [EXISTS]

    // [EXISTS] 0=full heal (120-139), 1=full heal+Uber (140-149),
    //          3=Uber+immunity (150-169), 2=Uber only (170+)
    tf2v_use_new_phlog_taunt.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=original (20-129), 1=L&W (130-149),
    //          2=TB (150-169), 3=JI (170+)
    tf2v_use_new_axtinguisher.SetValue( 0 );         // [EXISTS]

    // ---- HEAVY: MINIGUN ----
    // [NEW] Natascha slow/damage fix.
    // 0=eras 30-31: inverted (75% speed minor slow, reduced damage)
    // 1=era 32+:    corrected (25% speed major slow, proper damage)
    tf2v_natascha_fixed.SetValue( 0 );               // [NEW]

    // [NEW] Quick-Fix secondary weapon restriction.
    // 1=eras 110-129: cannot use primary weapons while Uber deployed
    // 0=era 130+:     restriction removed (Two Cities Nov 2013)
    tf2v_quick_fix_weapon_restriction.SetValue( 0 ); // [NEW] (pre-era110: weapon DNE)

    // Minigun damage rampup handled by tf2v_use_new_minigun_rampup mode 3 (EXISTS)

    // [EXISTS->TRUNCATED] Collapses minigun_spinup + aim_speed + rampup.
    // 0=eras 1-29 (no spinup, 80HU/s aim)
    // 1=eras 30-129 (spinup, 110HU/s aim)
    // 3=eras 130+ (full rampup both axes)
    tf2v_use_new_minigun_rampup.SetValue( 0 );       // [EXISTS->TRUNCATED]

    // [EXISTS] 0=120HP no throw (era 30), 1=300HP+throw (eras 50-119),
    //          2=300HP no self-throw (eras 120+)
    tf2v_sandvich_behavior.SetValue( 0 );            // [EXISTS]

    // ---- ENGINEER: BUILDINGS ----
    // [EXISTS] 0=Sentry only (eras 1-30), 1=all L3 (eras 31-89), 2=all+haul (eras 90+)
    tf2v_building_upgrades.SetValue( 0 );            // [EXISTS]

    // [EXISTS] Superseded by tf2v_building_upgrades mode 2. Kept for compatibility.
    tf2v_building_hauling.SetValue( 0 );             // [EXISTS]

    // [EXISTS] 0=25% penalty (eras 90-139), 1=10% penalty (eras 140+)
    tf2v_use_new_hauling_speed.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=original (eras 1-99), 1=F2P values (eras 100+)
    tf2v_use_new_wrench_mechanics.SetValue( 0 );     // [EXISTS]

    // [EXISTS] 0=66% resist while sapped (eras 1-99), 1=33% (eras 100+)
    tf2v_use_new_sapper_damage.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=no disable (eras 1-99), 1=brief disable (eras 100+)
    tf2v_use_new_sapper_disable.SetValue( 0 );       // [EXISTS]

    // [EXISTS] 0=20%/33% L2/L3 (eras 1-99), 1=15%/20% (eras 100+)
    tf2v_use_new_sentry_minigun_resist.SetValue( 0 );    // [EXISTS]

    // [EXISTS] 0=falloff from Engineer (eras 1-99), 1=from Sentry (eras 100+)
    tf2v_new_sentry_wrangle_location.SetValue( 0 );  // [EXISTS]

    // [EXISTS] 0=original range (eras 1-139), 1=extended to max range (eras 140+)
    tf2v_new_sentry_damage_falloff.SetValue( 0 );    // [EXISTS]

    // [EXISTS] 0=original cost (eras 1-159), 1=cheaper (eras 160+)
    tf2v_use_new_teleporter_cost.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=energy blast (eras 110-149), 1=energy ball (eras 150+)
    tf2v_use_new_short_circuit.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=no repair (eras 1-159), 1=repairable (eras 160+)
    tf2v_use_new_minibuildings.SetValue( 0 );        // [EXISTS]

    // ---- MEDIC ----
    // [EXISTS] 0=1-3HP/s (eras 1-9), 1=3-6HP/s (eras 10+)
    tf2v_use_new_medic_regen.SetValue( 0 );          // [EXISTS]

    // [NEW] 0=original lower rates (eras 1-9), 1=24-72HP/s (eras 10+)
    tf2v_medigun_heal_rate.SetValue( 0 );            // [NEW]

    // [EXISTS] 0=no setup bonus (eras 1-7), 1=faster during setup (eras 8+)
    tf2v_setup_uber_rate.SetValue( 0 );              // [EXISTS]

    // [EXISTS] 0=no extra drain (eras 1-109), 1=50% drain per extra target (eras 110+)
    // [EXISTS] Uber multi-target drain penalty.
    // 0=eras 0-7:  no extra drain for multiple targets
    // 1=era 8+:    drain faster per extra target (Apr 2 2008)
    tf2v_uber_juggle_penalty.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=50% on retract (eras 1-99), 1=25%+75% on stab/retract (eras 100+)
    tf2v_use_new_uber_taunt.SetValue( 0 );           // [EXISTS]

    // [EXISTS] 0=own speed (eras 1-139), 1=match target speed (eras 140+)
    tf2v_use_medic_speed_match.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=flat rate (eras 1-179), 1=damage-time based (eras 180+)
    tf2v_use_new_health_regen_attrib.SetValue( 0 );  // [EXISTS]

    // ---- SCOUT ----
    // [EXISTS] 0=full stun+Uber (era 50), 1=no Uber stun (eras 70-169),
    //          2=slowdown only (eras 170+)
    tf2v_sandman_stun_type.SetValue( 0 );            // [EXISTS]

    // [EXISTS] 0=6s (eras 50-179), 1=8s (eras 180+)
    tf2v_use_new_bonk_length.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=minicrits (eras 110-129), 1=5 dashes (eras 130+)
    tf2v_use_new_sodapopper_hype.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=fill by distance (eras 110-129), 1=fill by damage (eras 130+)
    tf2v_use_new_sodapopper_fill.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=auto-activate (eras 110-149), 1=manual alt-fire (eras 150+)
    tf2v_use_manual_sodapopper.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=no shove (eras 100-139), 1=alt-fire shove (eras 140+)
    tf2v_use_shortstop_shove.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=no slowdown (eras 1-119 and 140+), 1=slowdown (eras 120-139 only)
    tf2v_use_shortstop_slowdown.SetValue( 0 );       // [EXISTS]

    // [EXISTS] 0=crit/minicrit on bleed (eras 121-129), 1=cooldown reduction (eras 130+)
    tf2v_use_new_guillotine.SetValue( 0 );           // [EXISTS]

    // [EXISTS] 0=standard regen (eras 50-99), 1=33% faster (eras 100+)
    tf2v_use_new_ball_regen.SetValue( 0 );           // [EXISTS]

    // ---- SOLDIER ----
    // [EXISTS] 0=builds on damage dealt (eras 80-119), 1=damage taken (eras 120+)
    tf2v_use_new_buff_charges.SetValue( 0 );         // [EXISTS]

    // [EXISTS] 0=no sentry resist (eras 80-119), 1=+15% resist (eras 120+)
    tf2v_sentry_resist_bonus.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=original damage+speed (eras 80-119), 1=reduced damage (eras 120+)
    tf2v_use_new_equalizer_damage.SetValue( 0 );     // [EXISTS]

    // [EXISTS] 0=Escape Plan=Equalizer reskin (eras 80-119), 1=split (eras 120+)
    tf2v_use_new_split_equalizer.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=3s buff (eras 80-129), 1=2s buff (eras 130+)
    tf2v_new_speed_buff_duration.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=no ammo deduction (eras 120-149), 1=ammo deducted (eras 150+)
    tf2v_use_new_beggars.SetValue( 0 );              // [EXISTS]

    // [NEW] Chargin' Targe protects against own explosive damage.
    // 1=eras 60-99:  Targe blocks own explosion damage
    // 0=era 100+:    removed
    tf2v_targe_own_explosion.SetValue( 1 );          // [NEW]

    // ---- DEMOMAN ----
    // [EXISTS] 0=no debuff clear (eras 80-149), 1=charge clears debuffs (eras 150+)
    tf2v_demo_charge_debuff_remove.SetValue( 0 );    // [EXISTS]

    // [EXISTS] 0=original explosion (eras 101-149), 1=reduced after 1st hit (eras 150+)
    tf2v_use_new_caber.SetValue( 0 );                // [EXISTS]

    // [EXISTS] 0=cannot holster (eras 80-149), 1=can holster with HP penalty (eras 150+)
    tf2v_use_new_honorbound.SetValue( 0 );           // [EXISTS]

    // ---- SPY ----
    // [NEW] 1=can reload while cloaked (eras 1-5), 0=cannot (eras 6+)
    tf2v_spy_cloak_reload.SetValue( 1 );             // [NEW]

    // [NEW] 0=dispenser/spawn only (eras 1-30), 1=ammo pickups too (eras 31+)
    tf2v_spy_cloak_ammo_recharge.SetValue( 0 );      // [NEW]

    // [EXISTS] 0=no resist (eras 1-139), 1=20% resist cloaked (eras 140+)
    tf2v_use_new_cloak.SetValue( 0 );                // [EXISTS]

    // [EXISTS] 0=90% on activate (eras 60-149), 1=50% (era 150), 2=75% (eras 160+)
    tf2v_new_feign_death_activate.SetValue( 0 );     // [EXISTS]

    // [EXISTS] 0=flat resist (eras 60-159), 1=scales with cloak amount (eras 160+)
    tf2v_new_feign_death_stealth.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=standard YER (eras 60-149), 1=full cloak on disguise (eras 150+)
    tf2v_use_new_yer.SetValue( 0 );                  // [EXISTS]

    // [EXISTS] 0=no speed boost (eras 110-149), 1=speed boost on backstab (eras 150+)
    tf2v_use_new_big_earner.SetValue( 0 );           // [EXISTS]

    // [EXISTS] 0=standard redisguise (eras 1-149), 1=instant redisguise (eras 150+)
    tf2v_use_fast_redisguise.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=cannot change weapon shown (eras 1-99), 1=can swap (eras 100+)
    // [EXISTS] Disguise weapon display control.
    // 0=eras 0-49:  fixed weapon shown, no player control
    // 1=era 50+:    defaults to primary; last-disguise updates display
    // NOTE: era 100 may add further PDA cycling — verify.
    tf2v_allow_disguiseweapons.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=cannot use enemy tele (eras 1-69), 1=can use (eras 70+)
    tf2v_disguise_spy_teleport.SetValue( 0 );        // [EXISTS]

    // [EXISTS] 0=own speed (eras 1-159), 1=match disguised class speed (eras 160+)
    tf2v_disguise_speed_match.SetValue( 0 );         // [EXISTS]
    tf2v_use_new_spy_movespeeds.SetValue( 0 );       // [EXISTS] compat alias

    // [NEW] 0=300HU/s (eras 1-109), 1=320HU/s (eras 110+)
    tf2v_spy_base_speed.SetValue( 0 );               // [NEW]

    // [EXISTS] 0=full crit any range (eras 60-89), 1=falloff (eras 90-179),
    //          2=minicrits only (eras 180+)
    tf2v_use_new_ambassador.SetValue( 0 );           // [EXISTS]

    // [EXISTS] 0=no crit storage (eras 116-149), 1=stores crits (eras 150+)
    tf2v_use_new_diamondback.SetValue( 0 );          // [EXISTS]

    // [EXISTS] 0=original drain (eras 113-149), 1=lower with falloff (eras 150+)
    tf2v_use_new_pomson.SetValue( 0 );               // [EXISTS]

    // ---- SNIPER ----
    // [EXISTS] 0=option unavailable (eras 1-69), 1=available (eras 70+)
    tf2v_allow_sniper_crosshairs.SetValue( 0 );      // [EXISTS]

    // [EXISTS] 0=minicrit on kill (eras 121-149), 1=CRIKEY meter (eras 150+)
    tf2v_use_new_cleaners.SetValue( 0 );             // [EXISTS]

    // [EXISTS] 0=2011 Bison (eras 104-159), 1=2016 (era 160), 2=2017 (eras 170+)
    tf2v_use_new_bison_damage.SetValue( 0 );         // [EXISTS]

    // [EXISTS] 0=original speed (eras 104-159), 1=30% slower (eras 160+)
    tf2v_use_new_bison_speed.SetValue( 0 );          // [EXISTS]

    // ---- CROSS-CLASS ----
    // [EXISTS] 0=fire immediately (eras 1-149), 1=hold and release (eras 150+)
    tf2v_use_new_autofire.SetValue( 0 );             // [EXISTS]

    // [EXISTS->RENAME] Holster/unholster time.
    // 0=0.67s (eras 1-99), 1=0.50s (eras 100+)
    // Distinct from reload cancel and auto-reload default.
    tf2v_use_new_weapon_swap_speed.SetValue( 0 );    // [EXISTS]

    // [NEW] Clip-based reload can be cancelled by firing.
    // 0=eras 1-69 (cannot cancel reload), 1=eras 70+ (can cancel)
    tf2v_reload_cancel_available.SetValue( 0 );      // [NEW]

    // [NEW] Pistol fire mode.
    // 0=eras 0-69: semi-automatic, requires discrete input per shot.
    //              Hold fires slowly; rapid clicking fires faster than intended.
    //              Source engine semi-auto behaviour inherited from HL2 USP Match.
    // 1=era 70+:   fixed automatic rate — hold or tap gives same speed.
    //              Both the exploit closure and hold-to-fire arrived in same patch.
    tf2v_pistol_fixed_firerate.SetValue( 0 );        // [NEW]

    // [NEW] Air ducking formalised.
    // 0=eras 0-50: mid-air ducking not available as intended feature
    // 1=era 51+:   can duck twice in air; double jumps reset count
    tf2v_clamp_airducks.SetValue( 0 );               // [NEW]

    // [EXISTS] Auto-reload on by default server-side.
    // 0=eras 70-139 (available but off by default), 1=eras 140+ (on by default)
    // Before era 70: no auto-reload at all. reload_cancel_available handles that.
    tf2v_use_faster_reload.SetValue( 0 );            // [EXISTS]

    // [EXISTS] 0=standard Dalokohs (eras 81-139), 1=max health buff (eras 140+)
    tf2v_new_chocolate_behavior.SetValue( 0 );       // [EXISTS]

    // [EXISTS] 0=always available (eras 110-169), 1=must deploy (eras 170+)
    tf2v_use_new_atomizer.SetValue( 0 );             // [EXISTS]

    // [EXISTS] 0=no raise check (era 1), 1=facing check (eras 7+),
    //          2=stab delay removed (eras 50+)
    tf2v_use_new_backstabs.SetValue( 0 );            // [EXISTS]

    // [EXISTS] 0=faster swing (eras 101-149), 1=less sapper damage (eras 150+)
    tf2v_use_new_jag.SetValue( 0 );                  // [EXISTS]

    // ---- MOVEMENT ----
    // [NEW] 1=teammates absorb radius damage (eras 1-69), 0=pass through (eras 70+)
    tf2v_radius_damage_teammates.SetValue( 1 );      // [NEW]

    // [EXISTS] 450=eras 1-119, 520=eras 120+
    tf2v_clamp_speed_absolute.SetValue( 450 );       // [EXISTS]

    // [NEW] 0=no custom anims (eras 1-59), 1=Heavy+Sniper (eras 60-69),
    //       2=all classes (eras 70+)
    tf2v_class_death_animations.SetValue( 0 );       // [NEW]

    // [NEW] 1=self minicrits possible (eras 1-69), 0=removed (eras 70+)
    tf2v_minicrit_self_inflicted.SetValue( 1 );      // [NEW]

    // [EXISTS] B.A.S.E. Jumper updraft while burning.
    // 0=updraft bug active (eras 130-149), 1=removed (eras 150+)
    tf2v_disable_updraft.SetValue( 0 );              // [EXISTS]

    // [EXISTS] 0=no spam prevention (eras 1-169), 1=JI spam prevention (eras 170+)
    tf2v_prevent_voice_spam.SetValue( 0 );           // [EXISTS]


    // =========================================================================
    // SECTION 4: ERA TRANSITION CASCADE
    // Only changes from the previous era listed at each block.
    // =========================================================================

    if ( nEra >= 1 )    // PC BETA OPENS Sep 17 2007
    {
        tf2v_console_grenadelauncher_damage.SetValue( 0 );    // 100 dmg
        tf2v_console_grenadelauncher_magazine.SetValue( 0 );  // 4 clip
        tf2v_ammo_era.SetValue( 1 );
    }

    // Era 3: GL clip hotfix — same as era 2 in practice.
    // Era 4: mp_friendlyfire removed — no gameplay convar changes.

    if ( nEra >= 2 )    // Sep 28 2007
    {
        tf2v_grenades_explode_contact.SetValue( 0 );
        tf2v_flame_mode.SetValue( 1 );              // fixed hit detection
    }

    if ( nEra >= 3 )    // Oct 9 2007
    {
        tf2v_grenade_player_collision.SetValue( 1 );
        tf2v_spy_cloak_reload.SetValue( 0 );
    }

    if ( nEra >= 4 )    // LAUNCH Oct 10 2007
    {
        tf2v_ammo_era.SetValue( 2 );                // GL res 30, Sticky res 40
    }

    if ( nEra >= 5 )    // Oct 25 2007 — backstab facing check
    {
        // "Fixed a Spy backstab exploit where you could stab a player who was
        // not facing away from you." — IsBehindAndFacingTarget added,
        // requiring flDotOwner > 0.5 (Spy must face target within ~60 deg).
        tf2v_use_new_backstabs.SetValue( 1 );
    }

    if ( nEra >= 6 )    // Dec 20 2007
    {
        tf2v_setup_uber_rate.SetValue( 1 );
    }

    if ( nEra >= 7 )    // Jan 15 + Feb 14 2008 — Sniper zoom fixes
    {
        // Jan 15: fixed being able to zoom too quickly after firing a
        // zoomed shot (re-zoom delay enforced server-side).
        // Feb 14: added 200ms delay before a zoomed shot can crit.
        // Jan 14 auto-rezoom option is client-side only — no server convar.
        // Jan 15 2008: re-zoom lock (0.5s post-fire). Mode 1.
        // Feb 14 2008: +200ms zoom-to-crit delay. Mode 2.
        // Both fixes within 30 days; era 7 covers the combined window.
        tf2v_sniper_zoom_mode.SetValue( 2 );        // re-zoom lock + crit delay
    }

    if ( nEra >= 8 )    // Feb 28 2008 — reserves nerf
    {
        tf2v_ammo_era.SetValue( 3 );                // GL res 16, Sticky res 24, RL res 16
    }

    if ( nEra >= 9 )    // Apr 2 2008 — Uber multi-target drain
    {
        // Uber meter now drains faster for each additional target
        // still holding Uber beyond the primary target.
        tf2v_uber_juggle_penalty.SetValue( 1 );
    }

    // Era 11 (May 21 2008): Uber weapon-switch exploit fixed;
    // rapid-fire crit sync fixed. No new convars.

    if ( nEra >= 10 )   // GOLD RUSH Apr 29 2008
    {
        tf2v_use_new_medic_regen.SetValue( 1 );
        tf2v_medigun_heal_rate.SetValue( 1 );
    }

    if ( nEra >= 20 )   // PYRO UPDATE Jun 19 2008
    {
        tf2v_soldier_self_damage_reduction.SetValue( 0 ); // reduction removed
        tf2v_airblast.SetValue( 1 );
        tf2v_airblast_players.SetValue( 1 );
        tf2v_flame_mode.SetValue( 2 );
        tf2v_fall_sounds.SetValue( 1 );             // retail thump (best available date)
    }

    if ( nEra >= 21 )   // Jul 1 2008
    {
        tf2v_flame_mode.SetValue( 3 );              // close-range falloff restored
    }

    if ( nEra >= 30 )   // HEAVY UPDATE Aug 19 2008
    {
        tf2v_use_new_minigun_rampup.SetValue( 1 );  // spinup + 110HU/s aim
        tf2v_sandvich_behavior.SetValue( 0 );       // 120HP no throw (Sandvich exists)
    }

    if ( nEra >= 31 )   // Dec 11 2008
    {
        tf2v_building_upgrades.SetValue( 1 );       // all buildings to L3
        tf2v_spy_cloak_ammo_recharge.SetValue( 1 );
        tf2v_sticky_bullet_break.SetValue( 1 );
    }

    if ( nEra >= 32 )   // Jan 28 2009 — Natascha fixed
    {
        // Natascha shipped inverted: 75% speed (minor) slow instead of
        // 25% (major), plus lower damage. Jan 28 corrected both.
        tf2v_natascha_fixed.SetValue( 1 );
    }

    if ( nEra >= 40 )   // Feb 2 2009
    {
        tf2v_crit_model.SetValue( 1 );              // 2% base, 800 ramp
        tf2v_damage_spread_mode.SetValue( 1 );      // ±10%
        tf2v_ammo_era.SetValue( 4 );                // RL res 20
        tf2v_use_stickybomb_damage_rampup.SetValue( 1 );
        tf2v_use_new_demo_explosion_variance.SetValue( 1 );
    }

    if ( nEra >= 50 )   // SCOUT UPDATE Feb 24 2009
    {
        tf2v_sandman_stun_type.SetValue( 0 );       // early mode (Uber-stun)
        tf2v_use_new_backstabs.SetValue( 2 );       // stab delay removed
        tf2v_sandvich_behavior.SetValue( 1 );       // 300HP + throw
        // Initial disguise weapon display control added (Scout Update).
        tf2v_allow_disguiseweapons.SetValue( 1 );
    }

    if ( nEra >= 51 )   // Mar 6 2009 — air ducking formalised
    {
        // Can now duck twice in the air. Scout double jumps reset
        // in-air duck count. tf_clamp_airducks convar added.
        tf2v_clamp_airducks.SetValue( 1 );
    }

    if ( nEra >= 60 )   // SNIPER VS SPY May 21 2009
    {
        tf2v_class_death_animations.SetValue( 1 );  // Heavy + Sniper only
    }

    if ( nEra >= 61 )   // Jun 8 2009 — FaN buff, Flare minicrit, DR flag
    {
        tf2v_fan_damage_bonus.SetValue( 1 );        // FaN +10% damage
        tf2v_use_new_flare.SetValue( 1 );           // minicrit on burning
        tf2v_dead_ringer_flag_carry.SetValue( 1 );  // DR activates with flag
    }

    if ( nEra >= 70 )   // CLASSLESS UPDATE Aug 13 2009
    {
        tf2v_pistol_fixed_firerate.SetValue( 1 );   // fixed fire rate
        tf2v_class_death_animations.SetValue( 2 );  // all classes
        tf2v_minicrit_self_inflicted.SetValue( 0 );
        tf2v_radius_damage_teammates.SetValue( 0 );
        tf2v_reload_cancel_available.SetValue( 1 );
        tf2v_ctf_capcrits.SetValue( 1 );
        tf_arena_first_blood.SetValue( 1 );
        tf2v_allow_sniper_crosshairs.SetValue( 1 );
        tf2v_disguise_spy_teleport.SetValue( 1 );
        tf2v_sandman_stun_type.SetValue( 1 );       // no Uber stun, -25HP
    }

    // Era 80-83: no balance convar changes.

    if ( nEra >= 82 )   // Apr 29 2010 — Backburner damage bonus
    {
        // 119th Update: Backburner gets +20% damage bonus.
        tf2v_backburner_damage_bonus.SetValue( 1 );
    }

    if ( nEra >= 90 )   // ENGINEER UPDATE Jul 8 2010
    {
        // Gunboats self-damage reduction changed 75% -> 60%.
        tf2v_gunboats_nerf.SetValue( 1 );           // 60% reduction
        // Sandvich now uses cooldown timer (removed at era 100).
        tf2v_sandvich_behavior.SetValue( 3 );       // [NEW MODE] cooldown
        tf2v_building_upgrades.SetValue( 2 );       // all buildings + hauling
        tf2v_building_hauling.SetValue( 1 );
        tf2v_minicrits_on_deflect.SetValue( 1 );
        tf2v_flame_mode.SetValue( 4 );              // all deflections minicrit
    }

    if ( nEra >= 91 )   // SCREAM FORTRESS Oct 27 2010
    {
        // Rocket Jumper health penalty added (was 0, now -25 max HP).
        tf2v_rocket_jumper_health_penalty.SetValue( 1 ); // [NEW]
        // Sticky Jumper added — weapon gate handles it.
        // Sandvich charge meter bug fixed — absorbed into era 90 behaviour.
    }

    if ( nEra >= 100 )  // MANNCONOMY Sep 30 2010 + Australian Christmas Dec 17 2010
    {
        // Sandvich cooldown (added era 90) removed at Mannconomy;
        // reverts to eat-anywhere-anytime behaviour.
        tf2v_sandvich_behavior.SetValue( 1 );         // back to 300HP + throw
        // Shortstop slowdown removed (re-added at era 120, final removal era 140).
        tf2v_use_shortstop_slowdown.SetValue( 0 );    // [NEW]
        // Chargin' Targe no longer protects against own explosive damage.
        tf2v_targe_own_explosion.SetValue( 0 );       // [NEW]
        // Airblast pushes grounded stickies ~2x further.
        tf2v_airblast_sticky_push.SetValue( 1 );      // [NEW]
        // Minigun spin-up time increased; Natascha slowdown-on-hit
        // reduced over distance. Further Minigun/Natascha adjustments
        // beyond era 32 fix — tracked via tf2v_minigun_mode. [TODO: verify mode]
        tf2v_use_new_weapon_swap_speed.SetValue( 1 ); // 0.50s holster
        // disguiseweapons already set at era 50; era 100 may extend
        // to full PDA cycling — needs verification. TODO.
        tf2v_use_extinguish_heal.SetValue( 1 );
        tf2v_use_extinguish_cooldown.SetValue( 1 );
        tf2v_use_new_ambassador.SetValue( 1 );        // falloff era
        tf2v_use_new_ball_regen.SetValue( 1 );
        tf2v_use_new_grenade_radius.SetValue( 1 );    // 146Hu
        tf2v_use_new_wrench_mechanics.SetValue( 1 );
        tf2v_use_new_sapper_damage.SetValue( 1 );
        tf2v_use_new_sapper_disable.SetValue( 1 );
        tf2v_use_new_sentry_minigun_resist.SetValue( 1 );
        tf2v_new_sentry_wrangle_location.SetValue( 1 );
        tf2v_use_new_uber_taunt.SetValue( 1 );
    }

    // Eras 101-102: no balance convars (Polycount, Manniversary).

    if ( nEra >= 103 )  // AUSTRALIAN CHRISTMAS Dec 17 2010
    {
        // cp_degrootkeep and Medieval Mode added.
        // tf_medieval enabled by this era — map pool split point.
        // Minigun spin-up time increased beyond era 32 fix.
        // Natascha slowdown-on-hit reduced over distance.
        // Shortstop slowdown removed — tracked in tf2v_use_shortstop_slowdown.
        // [TODO] tf2v_minigun_mode needs a new value for the Dec 17 spin-up
        // increase, distinct from the era 30 launch value and era 32 fix.
        tf2v_use_shortstop_slowdown.SetValue( 0 );
    }

    // Eras 104: Community 3 — no balance convars.

    if ( nEra >= 105 )  // Apr 14 2011 — Backburner gains airblast
    {
        // No-airblast attribute removed from Backburner. The Backburner
        // launched without airblast (era 30-103); this patch enabled it.
        tf2v_backburner_airblast.SetValue( 1 );     // [NEW]
    }

    if ( nEra >= 110 )  // UBER UPDATE + F2P Jun 23 2011
    {
        tf2v_spy_base_speed.SetValue( 1 );          // 320 HU/s
        // Quick-Fix added with secondary weapon restriction.
        tf2v_quick_fix_weapon_restriction.SetValue( 1 );  // active at launch
        // uber_juggle_penalty already set at era 9 (Apr 2 2008).
        // Era 110 may have increased the penalty further — verify. TODO.
        // tf2v_uber_juggle_penalty.SetValue( 1 );  // already set at era 8
        tf2v_use_new_blackbox.SetValue( 1 );        // +20 on attack
        // Flare Gun: radius increased to 110Hu at Uber Update.
        // minicrit-on-burning was set at era 61; mode 2 here adds the
        // blast radius expansion on top of that.
        tf2v_use_new_flare.SetValue( 2 );           // minicrit + radius
        tf2v_use_new_flare_radius.SetValue( 1 );    // 110Hu blast radius
    }

    // Eras 111-117: no balance convars.

    if ( nEra >= 120 )  // PYROMANIA Jun 27 2012
    {
        tf2v_clamp_speed_absolute.SetValue( 520 );
        tf2v_flame_mode.SetValue( 5 );              // +10% base damage
        tf2v_use_new_buff_charges.SetValue( 1 );    // damage taken
        tf2v_sentry_resist_bonus.SetValue( 1 );
        tf2v_use_new_equalizer_damage.SetValue( 1 );
        tf2v_use_new_split_equalizer.SetValue( 1 );
        tf2v_sandvich_behavior.SetValue( 2 );       // no self-throw
        tf2v_sandman_stun_type.SetValue( 1 );       // still mode 1
        tf2v_use_new_flare.SetValue( 3 );           // crits burning mid/long
        tf2v_use_shortstop_slowdown.SetValue( 1 );  // re-added (was removed era 100)
    }

    // Era 121: no balance convars.

    if ( nEra >= 130 )  // LOVE & WAR Jun 18 2014
    {
        // Minigun damage/accuracy ramp-up added (full damage after 1 second).
        // Minigun damage rampup: tf2v_use_new_minigun_rampup.SetValue(3) below
        // already handles this via nMinigunMode=3. No separate convar needed.
        // Sticky bomb damage ramp-up added (full damage after 2 seconds).
        tf2v_use_stickybomb_damage_rampup.SetValue( 1 ); // was tf2v_sticky_damage_rampup
        // Sentry uses Engineer position-based damage falloff fixed.
        tf2v_new_sentry_damage_falloff.SetValue( 1 ); // was tf2v_sentry_range_falloff
        // Quick-Fix secondary weapon restriction removed (was added era 110).
        tf2v_quick_fix_weapon_restriction.SetValue( 0 ); // [NEW]
        tf2v_use_new_minigun_rampup.SetValue( 3 );  // full rampup
        tf2v_use_stickybomb_radius_rampup.SetValue( 1 );
        tf2v_use_new_sodapopper_hype.SetValue( 1 ); // 5 dashes
        tf2v_use_new_sodapopper_fill.SetValue( 1 ); // fill by damage
        tf2v_use_new_axtinguisher.SetValue( 1 );    // L&W variant
        tf2v_new_speed_buff_duration.SetValue( 1 ); // 2s
        tf2v_use_new_flare.SetValue( 4 );           // crits all burning
        tf2v_use_new_guillotine.SetValue( 1 );
        tf2v_new_chocolate_behavior.SetValue( 1 );
        tf2v_disable_updraft.SetValue( 0 );         // B.A.S.E. Jumper added, bug active
    }

    if ( nEra >= 140 )  // GUN METTLE Jul 2 2015
    {
        // Uber/cloak drain distance falloff: starts at 512Hu, zero at 1536Hu.
        tf2v_uber_range_falloff.SetValue( 1 );      // [NEW]
        tf2v_damage_spread_mode.SetValue( 2 );      // off by default
        tf2v_use_faster_reload.SetValue( 1 );       // auto-reload default on
        tf2v_use_new_hauling_speed.SetValue( 1 );   // 10% penalty
        tf2v_use_medic_speed_match.SetValue( 1 );
        tf2v_use_shortstop_shove.SetValue( 1 );
        tf2v_use_shortstop_slowdown.SetValue( 0 );  // slowdown reverted
        tf2v_use_new_teleporter_cost.SetValue( 1 );
        tf2v_use_new_autofire.SetValue( 1 );
        tf2v_new_sentry_damage_falloff.SetValue( 1 );
        tf2v_use_new_cloak.SetValue( 1 );
        tf2v_use_new_diamondback.SetValue( 1 );
        tf2v_use_fast_redisguise.SetValue( 1 );
        tf2v_use_new_phlog_taunt.SetValue( 1 );     // full heal + Uber
    }

    if ( nEra >= 150 )  // TOUGH BREAK Dec 17 2015
    {
        // Global weapon switch time reduced from 0.67s to 0.50s.
        tf2v_fast_weapon_switch.SetValue( 1 );
        tf2v_demo_charge_debuff_remove.SetValue( 1 );
        tf2v_use_new_honorbound.SetValue( 1 );
        tf2v_use_new_caber.SetValue( 1 );
        tf2v_new_feign_death_activate.SetValue( 1 ); // 50% resist
        tf2v_use_new_yer.SetValue( 1 );
        tf2v_use_new_big_earner.SetValue( 1 );
        tf2v_use_new_cleaners.SetValue( 1 );
        tf2v_use_new_phlog_taunt.SetValue( 3 );      // Uber + immunity
        tf2v_use_new_axtinguisher.SetValue( 2 );     // TB variant
        tf2v_use_manual_sodapopper.SetValue( 1 );
        tf2v_use_new_beggars.SetValue( 1 );
        tf2v_use_new_jag.SetValue( 1 );
        tf2v_use_new_short_circuit.SetValue( 1 );
        tf2v_disable_updraft.SetValue( 1 );          // bug fixed
    }

    if ( nEra >= 160 )  // MEET YOUR MATCH Jul 7 2016
    {
        tf2v_disguise_speed_match.SetValue( 1 );
        tf2v_use_new_spy_movespeeds.SetValue( 1 );  // compat alias
        tf2v_new_feign_death_activate.SetValue( 2 ); // 75% resist
        tf2v_new_feign_death_stealth.SetValue( 1 );
        tf2v_use_new_minibuildings.SetValue( 1 );
        tf2v_use_new_bison_damage.SetValue( 1 );
        tf2v_use_new_bison_speed.SetValue( 1 );
    }

    if ( nEra >= 170 )  // JUNGLE INFERNO Oct 20 2017
    {
        // Airblast no longer generates minicrits on launched targets.
        tf2v_airblast_minicrits.SetValue( 0 );      // [NEW]
        // Afterburn duration now based on flame contact time
        // (min 3s, max 10s) rather than fixed duration.
        tf2v_afterburn_contact_time.SetValue( 1 );  // [NEW]
        // Afterburn disrupts Medic healing/resist shields by 20%.
        tf2v_afterburn_heal_debuff.SetValue( 1 );   // [NEW]
        tf2v_airblast.SetValue( 2 );                // post-JI momentum airblast
        tf2v_fall_sounds.SetValue( 2 );             // crunch + voice pain
        tf2v_use_new_axtinguisher.SetValue( 3 );    // JI variant
        tf2v_sandman_stun_type.SetValue( 2 );       // slowdown only
        tf2v_use_new_phlog_fill.SetValue( 1 );      // 300 damage fill
        tf2v_use_new_phlog_taunt.SetValue( 2 );     // Uber only
        tf2v_use_new_atomizer.SetValue( 1 );        // must deploy
        tf2v_flame_mode.SetValue( 6 );              // JI calculations
        tf2v_prevent_voice_spam.SetValue( 1 );
        tf2v_use_new_bison_damage.SetValue( 2 );
    }

    if ( nEra >= 180 )  // MARCH 2018 — balance freeze
    {
        tf2v_use_new_ambassador.SetValue( 2 );                  // minicrits only
        tf2v_use_new_health_regen_attrib.SetValue( 1 );
        tf2v_use_new_demo_explosion_variance.SetValue( 2 );     // ±2% variance
        tf2v_use_new_bonk_length.SetValue( 1 );                 // 8s
        // Sydney Sleeper no-splash and Pocket Pistol HP are weapon
        // script attribute changes, not separate convars.
    }

    // Eras 181-189: Scream Fortress/Smissmas 2018-2022.
    // These are map and cosmetic content drops only — no balance convar changes.
    // Each adds to the map pool but does not alter any gameplay mechanic.
    // Mapcycle files distinguish the map pool at each point.

    if ( nEra >= TF2V_ERA_VSCRIPT )  // VSCRIPT Dec 1 2022 — ASYM era floor
    {
        // VScript implementation added. This is the basis on which VSH and ZI
        // were built as official gamemodes. No balance convar changes at this
        // patch — the significance is gamemode availability, not balance.
        // ASYM server type becomes valid from this era onward.
    }

    // Era 191: Summer 2023 — VSH official (Jul 12 2023). ASYM maps available.
    // Era 192: Scream Fortress XV — ZI official (Oct 9 2023).
    // Era 193: Scream Fortress XVI — Tug of War (Oct 2024).
    // Era 194: Smissmas 2024 (Dec 2024).
    // All content-only, no balance convar changes.

    if ( nEra >= TF2V_ERA_MAX )  // SDK RELEASE Feb 18 2025 — TF2V_ERA_MAX
    {
        // TF2 SDK released. Full modern map pool including Hold the Flag
        // (Scream Fortress XVII, Oct 2025) and all 2025 content.
        // No balance convar changes — the SDK release is an infrastructure
        // and distribution event, not a balance patch.
    }


    // =========================================================================
    // SECTION 5: FINALISE
    // =========================================================================
*/
    m_bApplyingEra = false;
    m_bEraDirty    = false;

    if ( tf2v_quickplay_profile.GetInt() > 0 )
        TF2VUpdateQuickPlayCompliance();

    // Refresh live player loadouts so weapon era gating takes effect immediately.
    // TF2VRefreshEraLoadout() is a no-op for dead players (they get correct
    // weapons on their next natural spawn). For alive players it sets
    // m_bForceItemRemovalOnRespawn and calls ForceRegenerateAndRespawn(),
    // which strips all live weapons via ValidateWeapons then re-issues them
    // through the TF2VStripAnachronisticModifiers path in ManageRegularWeapons.
    //
    // ApplyEra() is never called during an active round (TF2VEraChanged defers
    // mid-round changes to the next round boundary), so this never interrupts
    // a live game.
    if ( TF2V_WeaponGated() )
    {
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
            if ( pPlayer )
                pPlayer->TF2VRefreshEraLoadout();
        }
    }

    DevMsg( "[TF2V] ApplyEra( %d ) complete.\n", nEra );
}

#endif
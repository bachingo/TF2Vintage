//=============================================================================
//
// TF2V ERA SYSTEM IMPLEMENTATION
//
// DAY-BASED EPOCH — September 17 2007 = Day 1.
//
// Changes from the old integer-era system:
//   - tf2v_enforcement removed; replaced by tf2v_use_era_mapcycle (1 visible convar).
//   - Era management (balance + weapon gate) is ALWAYS active. There is no
//     "enforcement 0" manual mode. All sub-convars are hidden.
//   - TF2VEnforcementChanged removed; TF2VMapcycleToggleChanged is its replacement.
//   - All era boundary comparisons in this file use TF2V_ERA_DAY_* day constants
//     instead of the old small integers.
//   - TakeManualEraSnapshot/RestoreManualEraSnapshot removed (no manual mode).
//   - LockEraState simplified: nAllowedWeaponEra always mirrors tf2v_era.
//
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_gamerules_convars.h"
#include "tf_gamerules_era_internal.h"


#ifdef GAME_DLL
extern ConVar tf_arena_first_blood;
extern ConVar hide_server;

//-----------------------------------------------------------------------------
// TF2V_GetEraMapcycleFile — returns era-accurate mapcycle filename.
//
// All boundary values are now day integers (days since Sep 17 2007).
// Variant selection by tf2v_server_type (hidden convar):
//   0 = PVP (default)
//   1 = PVE (MvM, day 1795+)
//   2 = ASYM (VSH/ZI, day 5559+)
//-----------------------------------------------------------------------------
static const char *TF2V_GetEraMapcycleFile( int nDay )
{
    int nType = tf2v_server_type.GetInt();

    // PVE — MVM-only rotation, day 1795+
    if ( nType == 1 )
    {
        if ( nDay < TF2V_ERA_MVM_MIN )
        {
            DevWarning( "[TF2V] PVE mapcycle requested for day %d "
                        "but MvM doesn't exist until day %d. "
                        "Using PVP mapcycle instead.\n", nDay, TF2V_ERA_MVM_MIN );
            // fall through to PVP
        }
        else
        {
            if ( nDay <= 1867 ) return "maps/mapcycle_day1795_pve.txt";  // MvM launch
            if ( nDay <= 2467 ) return "maps/mapcycle_day1867_pve.txt";  // Oct 2012 pack
            if ( nDay <= 2654 ) return "maps/mapcycle_day2467_pve.txt";  // Two Cities
            if ( nDay <= 2846 ) return "maps/mapcycle_day2654_pve.txt";
            if ( nDay <= 3014 ) return "maps/mapcycle_day2846_pve.txt";
            if ( nDay <= 3687 ) return "maps/mapcycle_day3014_pve.txt";
            return "maps/mapcycle_day3014_pve.txt";  // MvM pool fixed after Tough Break
        }
    }

    // ASYM — VSH/ZI rotation, day 5559+
    if ( nType == 2 )
    {
        if ( nDay < TF2V_ERA_ASYM_MIN )
        {
            DevWarning( "[TF2V] ASYM mapcycle requested for day %d "
                        "but VSH/ZI require VScript (day %d+). "
                        "Using PVP mapcycle instead.\n", nDay, TF2V_ERA_ASYM_MIN );
            // fall through to PVP
        }
        else
        {
            if ( nDay <= 5778 ) return "maps/mapcycle_day5559_asym.txt"; // VScript
            if ( nDay <= 5867 ) return "maps/mapcycle_day5778_asym.txt"; // VSH official
            if ( nDay <= 5926 ) return "maps/mapcycle_day5867_asym.txt"; // ZI official
            if ( nDay <= 6234 ) return "maps/mapcycle_day5926_asym.txt";
            static const char *s_pszTermASYM = "maps/mapcycle_day" TF2V_ERA_MAX_STR "_asym.txt";
            return s_pszTermASYM;
        }
    }

    // PVP — standard rotation, cumulative pool grows at each content update.
    // Boundaries are day integers; the corresponding real dates are shown
    // in comments for reference.
    if ( nDay <=   24 ) return "maps/mapcycle_day1_pvp.txt";     // Sep 17 2007 beta
    if ( nDay <=   39 ) return "maps/mapcycle_day24_pvp.txt";    // Oct 10 2007 launch (6 maps)
    if ( nDay <=  131 ) return "maps/mapcycle_day39_pvp.txt";    // Oct 25 2007
    if ( nDay <=  151 ) return "maps/mapcycle_day131_pvp.txt";   // Jan 25 2008 +ctf_well (7)
    if ( nDay <=  226 ) return "maps/mapcycle_day151_pvp.txt";   // Feb 14 2008 +cp_badlands (8)
    if ( nDay <=  277 ) return "maps/mapcycle_day226_pvp.txt";   // Apr 29 2008 Gold Rush (9)
    if ( nDay <=  338 ) return "maps/mapcycle_day277_pvp.txt";   // Jun 19 2008 Pyro (11)
    if ( nDay <=  452 ) return "maps/mapcycle_day338_pvp.txt";   // Aug 19 2008 Heavy (17)
    if ( nDay <=  527 ) return "maps/mapcycle_day452_pvp.txt";   // Dec 11 2008 +cp_steel (18)
    if ( nDay <=  537 ) return "maps/mapcycle_day527_pvp.txt";   // Feb 24 2009 Scout (19)
    if ( nDay <=  613 ) return "maps/mapcycle_day537_pvp.txt";   // Mar  6 2009
    if ( nDay <=  697 ) return "maps/mapcycle_day613_pvp.txt";   // May 21 2009 Sniper/Spy (20)
    if ( nDay <=  730 ) return "maps/mapcycle_day697_pvp.txt";   // Aug 13 2009 Classless/KOTH (32)
    if ( nDay <=  823 ) return "maps/mapcycle_day730_pvp.txt";   // Sep 15 2009 +egypt/junction (34)
    if ( nDay <=  956 ) return "maps/mapcycle_day823_pvp.txt";   // Dec 17 2009 WAR! (35)
    if ( nDay <= 1026 ) return "maps/mapcycle_day956_pvp.txt";   // Apr 28 2010 +freight/upward (37)
    if ( nDay <= 1110 ) return "maps/mapcycle_day1026_pvp.txt";  // Jul  8 2010 Engineer (39)
    if ( nDay <= 1137 ) return "maps/mapcycle_day1110_pvp.txt";  // Sep 30 2010 Mann-Conomy (42)
    if ( nDay <= 1188 ) return "maps/mapcycle_day1137_pvp.txt";  // Oct 27 2010 +Scream Fortress
    if ( nDay <= 1376 ) return "maps/mapcycle_day1188_pvp.txt";  // Dec 17 2010 Aus Christmas (44)
    if ( nDay <= 1445 ) return "maps/mapcycle_day1376_pvp.txt";  // Jun 23 2011 F2P/Uber (51)
    if ( nDay <= 1551 ) return "maps/mapcycle_day1445_pvp.txt";  // Aug 31 2011 +viaduct_event (52)
    if ( nDay <= 1746 ) return "maps/mapcycle_day1551_pvp.txt";  // Dec 15 2011 +foundry (53)
    if ( nDay <= 1795 ) return "maps/mapcycle_day1746_pvp.txt";  // Jun 27 2012 Pyromania (59)
    if ( nDay <= 1867 ) return "maps/mapcycle_day1795_pvp.txt";  // Aug 15 2012 MvM (60)
    if ( nDay <= 2124 ) return "maps/mapcycle_day1867_pvp.txt";  // Oct 26 2012 +snakewater/helltower
    if ( nDay <= 2467 ) return "maps/mapcycle_day2124_pvp.txt";  // Jul 2013 +process/standin
    if ( nDay <= 2654 ) return "maps/mapcycle_day2467_pvp.txt";  // Jun 2014 Love & War (65)
    if ( nDay <= 2846 ) return "maps/mapcycle_day2654_pvp.txt";  // Dec 2014 Mannpower (same pool)
    if ( nDay <= 3014 ) return "maps/mapcycle_day2846_pvp.txt";  // Jul 2015 Gun Mettle (69)
    if ( nDay <= 3217 ) return "maps/mapcycle_day3014_pvp.txt";  // Dec 2015 Tough Break (73)
    if ( nDay <= 3687 ) return "maps/mapcycle_day3217_pvp.txt";  // Jul 2016 MYM (88)
    if ( nDay <= 3846 ) return "maps/mapcycle_day3687_pvp.txt";  // Oct 2017 Jungle Inferno (97)

    // Post-balance-freeze: map pool grows each holiday, balance unchanged.
    if ( nDay <= 4051 ) return "maps/mapcycle_day3846_pvp.txt";  // Mar 2018 freeze
    if ( nDay <= 4407 ) return "maps/mapcycle_day4051_pvp.txt";  // SF X  Oct 2018  (+5)
    if ( nDay <= 4764 ) return "maps/mapcycle_day4407_pvp.txt";  // SF XI Oct 2019  (+2)
    if ( nDay <= 4827 ) return "maps/mapcycle_day4764_pvp.txt";  // SF XII Oct 2020 (+4)
    if ( nDay <= 5133 ) return "maps/mapcycle_day4827_pvp.txt";  // Smissmas 2020 Dec (+4)
    if ( nDay <= 5191 ) return "maps/mapcycle_day5133_pvp.txt";  // SF XIII Oct 2021 (+6)
    if ( nDay <= 5498 ) return "maps/mapcycle_day5191_pvp.txt";  // Smissmas 2021 Dec (+6)
    if ( nDay <= 5559 ) return "maps/mapcycle_day5498_pvp.txt";  // SF XIV Oct 2022 (+5)
    if ( nDay <= 5778 ) return "maps/mapcycle_day5559_pvp.txt";  // VScript+Smissmas 2022 (+5)
    if ( nDay <= 5867 ) return "maps/mapcycle_day5778_pvp.txt";  // SF XV Oct 2023  (+12)
    if ( nDay <= 5926 ) return "maps/mapcycle_day5867_pvp.txt";  // Smissmas 2023 Dec (+8)
    if ( nDay <= 6234 ) return "maps/mapcycle_day5926_pvp.txt";  // Summer+SF XVI 2024 (+7)
    if ( nDay <= 6296 ) return "maps/mapcycle_day6234_pvp.txt";  // Smissmas 2024 Dec (+5)
    if ( nDay <= 6365 ) return "maps/mapcycle_day6296_pvp.txt";  // TF2 SDK Feb 2025
    if ( nDay <= 6521 ) return "maps/mapcycle_day6365_pvp.txt";  // Summer 2025 (+9)
    if ( nDay <= 6598 ) return "maps/mapcycle_day6521_pvp.txt";  // SF XVII Oct 2025
    static const char *s_pszTermPVP = "maps/mapcycle_day" TF2V_ERA_MAX_STR "_pvp.txt";
    return s_pszTermPVP;
}


//-----------------------------------------------------------------------------
// TF2VGetGamemodeMinEra — returns earliest day a gamemode existed.
//-----------------------------------------------------------------------------
static int TF2VGetGamemodeMinEra( const char *pszGamemodeConvar )
{
    struct GamemodeEra_t
    {
        const char *pszConvar;
        int nMinDay;
    };

    static const GamemodeEra_t s_GamemodeEras[] =
    {
        // PVP — launch window
        { "tf_gamemode_ctf",        TF2V_ERA_MIN             },  // Oct 10 2007 launch
        { "tf_gamemode_cp",         TF2V_ERA_MIN             },
        { "tf_gamemode_tc",         TF2V_ERA_MIN             },  // tc_hydro

        // PVP — content updates
        { "tf_gamemode_payload",    TF2V_ERA_DAY_GOLDRUSH    },  // day 226  Apr 29 2008
        { "tf_gamemode_arena",      TF2V_ERA_DAY_HEAVY       },  // day 338  Aug 19 2008
        { "tf_gamemode_koth",       TF2V_ERA_DAY_CLASSLESS   },  // day 697  Aug 13 2009
        { "tf_gamemode_plr",        TF2V_ERA_DAY_WAR         },  // day 823  Dec 17 2009
        { "tf_gamemode_medieval",   TF2V_ERA_DAY_AUSSIE2010  },  // day 1188 Dec 17 2010
        { "tf_gamemode_sd",         TF2V_ERA_DAY_PYROMANIA   },  // day 1746 Jun 27 2012
        { "tf_gamemode_rd",         TF2V_ERA_DAY_LOVEANDWAR  },  // day 2467 Jun 18 2014
        { "tf_gamemode_pd",         TF2V_ERA_DAY_LOVEANDWAR  },
        { "tf_gamemode_mannpower",  TF2V_ERA_DAY_SMISSMAS2014},  // day 2654 Dec 22 2014
        { "tf_gamemode_passtime",   TF2V_ERA_DAY_MYM         },  // day 3217 Jul  7 2016

        // PVE
        { "tf_gamemode_mvm",        TF2V_ERA_MVM_MIN         },  // day 1795 Aug 15 2012

        // ASYM (VScript-based)
        { "tf_gamemode_vsh",        TF2V_ERA_ASYM_MIN        },  // day 5559 VScript
        { "tf_gamemode_dr",         TF2V_ERA_ASYM_MIN        },
    };

    for ( int i = 0; i < ARRAYSIZE( s_GamemodeEras ); i++ )
    {
        if ( V_strcmp( pszGamemodeConvar, s_GamemodeEras[i].pszConvar ) == 0 )
            return s_GamemodeEras[i].nMinDay;
    }
    return TF2V_ERA_MAX + 1;  // unknown gamemode — never valid
}


//-----------------------------------------------------------------------------
// TF2VCheckMapGamemode — validates current map's gamemode against active era.
//-----------------------------------------------------------------------------
static bool TF2VCheckMapGamemode( const char **pFail = NULL )
{
    if ( !TFGameRules() ) return true;

    int nDay = TFGameRules()->EraState().nCurrentEra;

    static const char *s_GamemodeConvars[] =
    {
        "tf_gamemode_ctf", "tf_gamemode_cp", "tf_gamemode_tc",
        "tf_gamemode_payload", "tf_gamemode_arena", "tf_gamemode_koth",
        "tf_gamemode_plr", "tf_gamemode_medieval", "tf_gamemode_sd",
        "tf_gamemode_rd", "tf_gamemode_pd", "tf_gamemode_mannpower",
        "tf_gamemode_passtime", "tf_gamemode_mvm", "tf_gamemode_vsh",
        "tf_gamemode_dr",
    };

    for ( int i = 0; i < ARRAYSIZE( s_GamemodeConvars ); i++ )
    {
        ConVarRef cv( s_GamemodeConvars[i] );
        if ( !cv.IsValid() || !cv.GetBool() ) continue;

        int nMinDay = TF2VGetGamemodeMinEra( s_GamemodeConvars[i] );
        if ( nDay < nMinDay )
        {
            if ( pFail )
                *pFail = "map gamemode not introduced until a later era";
            DevWarning( "[TF2V] Gamemode %s requires day >= %d (current day %d)\n",
                        s_GamemodeConvars[i], nMinDay, nDay );
            return false;
        }
    }
    return true;
}


//-----------------------------------------------------------------------------
// TF2VApplyMapcycle — sets mapcyclefile from the era table.
// Called when tf2v_use_era_mapcycle is 1.
//-----------------------------------------------------------------------------
static void TF2VApplyMapcycle()
{
    if ( !engine ) return;
    if ( !TF2V_MapcycleManaged() ) return;

    const char *pszFile = TF2V_GetEraMapcycleFile( TFGameRules()->EraState().nCurrentEra );
    if ( !pszFile )
    {
        Msg( "[TF2V] No era-accurate mapcycle for day %d.\n",
             TFGameRules()->EraState().nCurrentEra );
        return;
    }

    ConVarRef mapcyclefile( "mapcyclefile" );
    if ( mapcyclefile.IsValid() &&
         V_strcmp( mapcyclefile.GetString(), pszFile ) != 0 )
    {
        mapcyclefile.SetValue( pszFile );
        Msg( "[TF2V] Mapcycle -> %s\n", pszFile );
        if ( TFGameRules() )
            TFGameRules()->ForceMapCycleNeedsUpdate();
    }
}

//-----------------------------------------------------------------------------
// TF2VMapcycleToggleChanged — fires when tf2v_use_era_mapcycle is changed.
// Replaces TF2VMapcycleModeChanged + part of TF2VEnforcementChanged.
//-----------------------------------------------------------------------------
void TF2VMapcycleToggleChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
    if ( !TFGameRules() ) return;
    ConVarRef var( pConVar );
    bool bNew = var.GetBool();
    bool bOld = (bool)(int)flOldValue;
    if ( bNew == bOld ) return;

    if ( bNew )
    {
        Msg( "[TF2V] Era mapcycle ENABLED — applying day %d mapcycle.\n",
             tf2v_era.GetInt() );
        TF2VApplyMapcycle();
    }
    else
    {
        Msg( "[TF2V] Era mapcycle DISABLED — server controls mapcyclefile manually.\n" );
    }

    if ( tf2v_quickplay_profile.GetInt() > 0 )
        TFGameRules()->TF2VUpdateQuickPlayCompliance();
}


//-----------------------------------------------------------------------------
// Server type floor helpers.
//-----------------------------------------------------------------------------
static int TF2VGetServerTypeEraFloor( int nServerType )
{
    switch ( nServerType )
    {
    case 1:  return TF2V_ERA_MVM_MIN;    // PVE — day 1795
    case 2:  return TF2V_ERA_ASYM_MIN;   // ASYM — day 5559
    default: return TF2V_ERA_MIN;         // PVP — no floor
    }
}

void TF2VServerTypeChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
    if ( !TFGameRules() ) return;

    ConVarRef var( pConVar );
    int nNew = var.GetInt();
    int nOld = (int)flOldValue;
    if ( nNew == nOld ) return;

    static const char *s_pszTypeNames[] = { "PVP", "PVE", "ASYM" };
    Msg( "[TF2V] Server type: %s -> %s\n",
         s_pszTypeNames[ clamp( nOld, 0, 2 ) ],
         s_pszTypeNames[ clamp( nNew, 0, 2 ) ] );

    // Clamp era upward to server-type floor if needed.
    int nFloor = TF2VGetServerTypeEraFloor( nNew );
    int nEra   = tf2v_era.GetInt();
    if ( nEra < nFloor )
    {
        Msg( "[TF2V] Server type %s requires day >= %d. "
             "Raising era from %d to %d.\n",
             s_pszTypeNames[ clamp( nNew, 0, 2 ) ], nFloor, nEra, nFloor );
        tf2v_era.SetValue( nFloor );
    }

    if ( TF2V_MapcycleManaged() )
    {
        if ( CTFGameRules::TF2V_IsRoundActive() )
        {
            // Revert and let admin change during intermission.
            TFGameRules()->m_bApplyingEra = true;
            var.SetValue( nOld );
            TFGameRules()->m_bApplyingEra = false;
            Msg( "[TF2V] tf2v_server_type change deferred — "
                 "make changes during intermission.\n" );
            return;
        }
        TF2VApplyMapcycle();
    }

    if ( tf2v_quickplay_profile.GetInt() > 0 )
        TFGameRules()->TF2VUpdateQuickPlayCompliance();
}


//-----------------------------------------------------------------------------
// TF2V_IsRoundActive
//-----------------------------------------------------------------------------
bool CTFGameRules::TF2V_IsRoundActive()
{
    if ( !TFGameRules() ) return false;
    int nState = TFGameRules()->State_Get();
    return ( nState == GR_STATE_PREROUND    ||
             nState == GR_STATE_RND_RUNNING ||
             nState == GR_STATE_STALEMATE );
}


//-----------------------------------------------------------------------------
// TF2VEraChanged — defers if round is live, applies immediately if safe.
// Era management is unconditional in the new model; no enforcement check.
//-----------------------------------------------------------------------------
void TF2VEraChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
    if ( !TFGameRules() ) return;
    if ( TFGameRules()->m_bApplyingEra ) return;

    ConVarRef var( pConVar );
    int nNew = var.GetInt();
    int nOld = (int)flOldValue;
    if ( nNew == nOld ) return;

    // Hard clamp to [TF2V_ERA_MIN, TF2V_ERA_MAX]
    int nClamped = clamp( nNew, TF2V_ERA_MIN, TF2V_ERA_MAX );
    if ( nClamped != nNew )
    {
        Warning( "[TF2V] Day %d out of range [%d, %d] — clamping to %d.\n",
                 nNew, TF2V_ERA_MIN, TF2V_ERA_MAX, nClamped );
        TFGameRules()->m_bApplyingEra = true;
        var.SetValue( nClamped );
        TFGameRules()->m_bApplyingEra = false;
        nNew = nClamped;
    }

    // Server type floor
    int nFloor = TF2VGetServerTypeEraFloor( tf2v_server_type.GetInt() );
    if ( nNew < nFloor )
    {
        static const char *s_pszTypeNames[] = { "PVP", "PVE", "ASYM" };
        Warning( "[TF2V] Day %d is below the floor for %s servers (day %d). "
                 "Clamping to %d.\n",
                 nNew,
                 s_pszTypeNames[ clamp( tf2v_server_type.GetInt(), 0, 2 ) ],
                 nFloor, nFloor );
        TFGameRules()->m_bApplyingEra = true;
        var.SetValue( nFloor );
        TFGameRules()->m_bApplyingEra = false;
        nNew = nFloor;
    }

    if ( CTFGameRules::TF2V_IsRoundActive() )
    {
        TFGameRules()->m_nPendingEra    = nNew;
        TFGameRules()->m_bHasPendingEra = true;

        TFGameRules()->m_bApplyingEra = true;
        var.SetValue( nOld );
        TFGameRules()->m_bApplyingEra = false;

        Msg( "[TF2V] Era change to day %d queued — takes effect at next round boundary.\n", nNew );
    }
    else
    {
        TFGameRules()->ApplyEra( nNew );
    }
}


//-----------------------------------------------------------------------------
// TF2VAnySubConvarChanged — reverts mid-round changes to hidden sub-convars.
// Operators should change tf2v_era instead. In the new model there is no
// manual mode, so all sub-convars always revert mid-round.
//-----------------------------------------------------------------------------
void TF2VAnySubConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
    if ( !TFGameRules() ) return;
    if ( TFGameRules()->m_bApplyingEra ) return;

    ConVarRef var( pConVar );

    if ( CTFGameRules::TF2V_IsRoundActive() )
    {
        TFGameRules()->m_bApplyingEra = true;
        var.SetValue( pOldString );
        TFGameRules()->m_bApplyingEra = false;

        TFGameRules()->m_bEraDirty = true;
        Msg( "[TF2V] %s is a hidden era sub-convar — "
             "change tf2v_era instead; takes effect next round.\n",
             var.GetName() );
    }
    // Outside a round: ApplyEra() owns the value; direct writes are silently
    // overwritten at the next round start. No action needed here.
}


//-----------------------------------------------------------------------------
// LockEraState — snapshots convar values into m_EraState at round start.
// nAllowedWeaponEra always mirrors tf2v_era (weapon gate is always active).
//-----------------------------------------------------------------------------
void CTFGameRules::LockEraState()
{
    TF2VEraState_t &s = m_EraState;

    // Weapon gate always mirrors the active era day.
    s.nAllowedWeaponEra = tf2v_era.GetInt();
    s.nCurrentEra       = tf2v_era.GetInt();

    // All other fields from hidden convars set by ApplyEra().
    s.nDamageSpreadMode         = tf2v_damage_spread_mode.GetInt();
    s.bCritModel                = tf2v_crit_model.GetBool();
    s.bCtfCapCrits              = tf2v_ctf_capcrits.GetBool();
    s.bArenaFirstBlood          = tf_arena_first_blood.GetBool();
    s.nFallSounds               = tf2v_fall_sounds.GetInt();
    s.bConsoleDamage            = tf2v_console_grenadelauncher_damage.GetBool();
    s.bConsoleMagazine          = tf2v_console_grenadelauncher_magazine.GetBool();
    s.bGrenadeContactExplode    = tf2v_grenades_explode_contact.GetBool();
    s.bGrenadePlayerCollision   = tf2v_grenade_player_collision.GetBool();
    s.bNewGrenadeRadius         = tf2v_use_new_grenade_radius.GetBool();
    s.nDemoExplosionVariance    = tf2v_use_new_demo_explosion_variance.GetInt();
    s.bStickyDamageRampup       = tf2v_use_stickybomb_damage_rampup.GetBool();
    s.bStickyRadiusRampup       = tf2v_use_stickybomb_radius_rampup.GetBool();
    s.bStickyBulletBreak        = tf2v_sticky_bullet_break.GetBool();
    s.nAmmoEra                  = tf2v_ammo_era.GetInt();
    s.bNewBlackBox              = tf2v_use_new_blackbox.GetBool();
    s.nAirblast                 = tf2v_airblast.GetInt();
    s.bAirblastPlayers          = tf2v_airblast_players.GetBool();
    s.nFlameMode                = tf2v_flame_mode.GetInt();
    s.bMinicritsOnDeflect       = tf2v_minicrits_on_deflect.GetBool();
    s.bExtinguishHeal           = tf2v_use_extinguish_heal.GetBool();
    s.bExtinguishCooldown       = tf2v_use_extinguish_cooldown.GetBool();
    s.nFlareMode                = tf2v_use_new_flare.GetInt();
    s.bNewFlareRadius           = tf2v_use_new_flare_radius.GetBool();
    s.nPhlogFill                = tf2v_use_new_phlog_fill.GetInt();
    s.nPhlogTaunt               = tf2v_use_new_phlog_taunt.GetInt();
    s.nAxtinguisher             = tf2v_use_new_axtinguisher.GetInt();
    s.nMinigunMode              = tf2v_use_new_minigun_rampup.GetInt();
    s.nSandvichBehavior         = tf2v_sandvich_behavior.GetInt();
    s.nBuildingUpgrades         = tf2v_building_upgrades.GetInt();
    s.bBuildingHauling          = tf2v_building_hauling.GetBool();
    s.bNewHaulingSpeed          = tf2v_use_new_hauling_speed.GetBool();
    s.bNewWrenchMechanics       = tf2v_use_new_wrench_mechanics.GetBool();
    s.bNewSapperDamage          = tf2v_use_new_sapper_damage.GetBool();
    s.bNewSapperDisable         = tf2v_use_new_sapper_disable.GetBool();
    s.bNewSentryMinigunResist   = tf2v_use_new_sentry_minigun_resist.GetBool();
    s.bNewSentryWrangleLocation = tf2v_new_sentry_wrangle_location.GetBool();
    s.bNewSentryDamageFalloff   = tf2v_new_sentry_damage_falloff.GetBool();
    s.bNewTeleporterCost        = tf2v_use_new_teleporter_cost.GetBool();
    s.bNewShortCircuit          = tf2v_use_new_short_circuit.GetBool();
    s.bNewMinibuildings         = tf2v_use_new_minibuildings.GetBool();
    s.bNewMedicRegen            = tf2v_use_new_medic_regen.GetBool();
    s.bMedigunHealRate          = tf2v_medigun_heal_rate.GetBool();
    s.nSetupUberRate            = tf2v_setup_uber_rate.GetInt();
    s.bUberJugglePenalty        = tf2v_uber_juggle_penalty.GetBool();
    s.bNewUberTaunt             = tf2v_use_new_uber_taunt.GetBool();
    s.bMedicSpeedMatch          = tf2v_use_medic_speed_match.GetBool();
    s.bNewHealthRegenAttrib     = tf2v_use_new_health_regen_attrib.GetBool();
    s.nSandmanStunType          = tf2v_sandman_stun_type.GetInt();
    s.bNewBonkLength            = tf2v_use_new_bonk_length.GetBool();
    s.bNewSodapopperHype        = tf2v_use_new_sodapopper_hype.GetBool();
    s.bNewSodapopperFill        = tf2v_use_new_sodapopper_fill.GetBool();
    s.bManualSodapopper         = tf2v_use_manual_sodapopper.GetBool();
    s.bShortstopShove           = tf2v_use_shortstop_shove.GetBool();
    s.bShortstopSlowdown        = tf2v_use_shortstop_slowdown.GetBool();
    s.bNewGuillotine            = tf2v_use_new_guillotine.GetBool();
    s.bNewBallRegen             = tf2v_use_new_ball_regen.GetBool();
    s.bNewBuffCharges           = tf2v_use_new_buff_charges.GetBool();
    s.bSentryResistBonus        = tf2v_sentry_resist_bonus.GetBool();
    s.bNewEqualizerDamage       = tf2v_use_new_equalizer_damage.GetBool();
    s.bNewSplitEqualizer        = tf2v_use_new_split_equalizer.GetBool();
    s.bNewSpeedBuffDuration     = tf2v_new_speed_buff_duration.GetBool();
    s.bNewBeggars               = tf2v_use_new_beggars.GetBool();
    s.bDemoChargeDebuffRemove   = tf2v_demo_charge_debuff_remove.GetBool();
    s.bNewCaber                 = tf2v_use_new_caber.GetBool();
    s.bNewHonorbound            = tf2v_use_new_honorbound.GetBool();
    s.bSpyCloakReload           = tf2v_spy_cloak_reload.GetBool();
    s.bSpyCloakAmmoRecharge     = tf2v_spy_cloak_ammo_recharge.GetBool();
    s.bNewCloak                 = tf2v_use_new_cloak.GetBool();
    s.nFeignDeathActivate       = tf2v_new_feign_death_activate.GetInt();
    s.bFeignDeathStealth        = tf2v_new_feign_death_stealth.GetBool();
    s.bNewYer                   = tf2v_use_new_yer.GetBool();
    s.bNewBigEarner             = tf2v_use_new_big_earner.GetBool();
    s.bFastRedisguise           = tf2v_use_fast_redisguise.GetBool();
    s.bAllowDisguiseWeapons     = tf2v_allow_disguiseweapons.GetBool();
    s.bDisguiseSpyTeleport      = tf2v_disguise_spy_teleport.GetBool();
    s.bDisguiseSpeedMatch       = tf2v_disguise_speed_match.GetBool();
    s.bNewSpyMovespeeds         = tf2v_use_new_spy_movespeeds.GetBool();
    s.bSpyBaseSpeed             = tf2v_spy_base_speed.GetBool();
    s.nAmbassadorMode           = tf2v_use_new_ambassador.GetInt();
    s.bNewDiamondback           = tf2v_use_new_diamondback.GetBool();
    s.bNewPomson                = tf2v_use_new_pomson.GetBool();
    s.bAllowSniperCrosshairs    = tf2v_allow_sniper_crosshairs.GetBool();
    s.bNewCleaners              = tf2v_use_new_cleaners.GetBool();
    s.nBisonDamage              = tf2v_use_new_bison_damage.GetInt();
    s.bNewBisonSpeed            = tf2v_use_new_bison_speed.GetBool();
    s.bNewAutofire              = tf2v_use_new_autofire.GetBool();
    s.bNewWeaponSwapSpeed       = tf2v_use_new_weapon_swap_speed.GetBool();
    s.bFastWeaponSwitch         = tf2v_fast_weapon_switch.GetBool();
    s.bReloadCancelAvailable    = tf2v_reload_cancel_available.GetBool();
    s.bFasterReloadDefault      = tf2v_use_faster_reload.GetBool();
    s.bNewChocolate             = tf2v_new_chocolate_behavior.GetBool();
    s.bNewAtomizer              = tf2v_use_new_atomizer.GetBool();
    s.nBackstabMode             = tf2v_use_new_backstabs.GetInt();
    s.bNewJag                   = tf2v_use_new_jag.GetBool();
    s.bRadiusDamageTeammates    = tf2v_radius_damage_teammates.GetBool();
    s.nSpeedCap                 = tf2v_clamp_speed_absolute.GetInt();
    s.nClassDeathAnimations     = tf2v_class_death_animations.GetInt();
    s.bMinicritselfInflicted    = tf2v_minicrit_self_inflicted.GetBool();
    s.bDisableUpdraft           = tf2v_disable_updraft.GetBool();
    s.bPreventVoiceSpam         = tf2v_prevent_voice_spam.GetBool();
    s.bClampAirducks            = tf2v_clamp_airducks.GetBool();
    s.bPistolFixedFirerate      = tf2v_pistol_fixed_firerate.GetBool();
    s.nSniperZoomMode           = tf2v_sniper_zoom_mode.GetInt();
    s.bSoldierSelfDamageReduction = tf2v_soldier_self_damage_reduction.GetBool();
    s.bGunboatsNerf             = tf2v_gunboats_nerf.GetBool();
    s.bRocketJumperHealthPenalty = tf2v_rocket_jumper_health_penalty.GetBool();
    s.bBackburnerDamageBonus    = tf2v_backburner_damage_bonus.GetBool();
    s.bBackburnerAirblast       = tf2v_backburner_airblast.GetBool();
    s.bAirblastMinicrits        = tf2v_airblast_minicrits.GetBool();
    s.bAfterburnContactTime     = tf2v_afterburn_contact_time.GetBool();
    s.bAfterburnHealDebuff      = tf2v_afterburn_heal_debuff.GetBool();
    s.bAirblastStickyPush       = tf2v_airblast_sticky_push.GetBool();
    s.bTargeOwnExplosion        = tf2v_targe_own_explosion.GetBool();
    s.bFaNDamageBonus           = tf2v_fan_damage_bonus.GetBool();
    s.bDeadRingerFlagCarry      = tf2v_dead_ringer_flag_carry.GetBool();
    s.bQuickFixWeaponRestriction = tf2v_quick_fix_weapon_restriction.GetBool();
    s.bNataschaFixed            = tf2v_natascha_fixed.GetBool();
    s.bUberRangeFalloff         = tf2v_uber_range_falloff.GetBool();

    m_bEraStateLocked = true;

    DevMsg( "[TF2V] LockEraState: day=%d mapcycle_managed=%d\n",
            s.nCurrentEra, tf2v_use_era_mapcycle.GetBool() ? 1 : 0 );
}


//-----------------------------------------------------------------------------
// TF2VUpdateQuickPlayCompliance — certification tier tags.
// "Partial" = era-managed without mapcycle management (tf2v_use_era_mapcycle=0).
// "Full"    = era-managed with mapcycle management (tf2v_use_era_mapcycle=1).
//-----------------------------------------------------------------------------

static bool TF2VCheckQuickPlayBase( CUtlString *pFail )
{
    ConVarRef sv_cheats( "sv_cheats" );
    ConVarRef sv_lan( "sv_lan" );
    ConVarRef mp_friendlyfire( "mp_friendlyfire" );
    ConVarRef mp_highlander( "mp_highlander" );
    ConVarRef sv_password( "sv_password" );

    if ( sv_cheats.IsValid()       && sv_cheats.GetBool() )
    { if(pFail)*pFail="sv_cheats 1";        return false; }
    if ( sv_lan.IsValid()          && sv_lan.GetBool() )
    { if(pFail)*pFail="sv_lan 1";           return false; }
    if ( mp_friendlyfire.IsValid() && mp_friendlyfire.GetBool() )
    { if(pFail)*pFail="mp_friendlyfire 1";  return false; }
    if ( mp_highlander.IsValid()   && mp_highlander.GetBool() )
    { if(pFail)*pFail="mp_highlander 1";    return false; }
    if ( sv_password.IsValid()     && sv_password.GetString()[0] != '\0' )
    { if(pFail)*pFail="server has password";return false; }
    if ( hide_server.GetBool() )
    { if(pFail)*pFail="hide_server 1";      return false; }
    if ( tf2v_allcrit.GetBool() )
    { if(pFail)*pFail="tf2v_allcrit 1";     return false; }
    if ( tf2v_randomizer.GetBool() )
    { if(pFail)*pFail="randomizer on";      return false; }
    if ( gpGlobals->maxClients > 32 )
    { if(pFail)*pFail="maxplayers > 32";    return false; }
    return true;
}

static bool TF2VCheckCasual( CUtlString *pFail )
{
    if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;
    ConVarRef tf_weapon_criticals( "tf_weapon_criticals" );
    if ( tf_weapon_criticals.IsValid() && !tf_weapon_criticals.GetBool() )
    { if(pFail)*pFail="crits off (use competitive)"; return false; }
    return true;
}

static bool TF2VCheckCompetitive( CUtlString *pFail )
{
    if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;
    ConVarRef tf_weapon_criticals( "tf_weapon_criticals" );
    ConVarRef tf_damage_disablespread( "tf_damage_disablespread" );
    ConVarRef tf_use_fixed_weaponspreads( "tf_use_fixed_weaponspreads" );
    ConVarRef sv_pure( "sv_pure" );
    ConVarRef mp_decals( "mp_decals" );

    if ( tf_weapon_criticals.IsValid() && tf_weapon_criticals.GetBool() )
    { if(pFail)*pFail="crits on";            return false; }
    if ( !( ( tf_damage_disablespread.IsValid() && tf_damage_disablespread.GetBool() ) ||
            ( tf_use_fixed_weaponspreads.IsValid() && tf_use_fixed_weaponspreads.GetBool() ) ) )
    { if(pFail)*pFail="spread not disabled"; return false; }
    if ( sv_pure.IsValid() && sv_pure.GetInt() < 1 )
    { if(pFail)*pFail="sv_pure < 1";         return false; }
    if ( mp_decals.IsValid() && mp_decals.GetInt() > 0 )
    { if(pFail)*pFail="sprays enabled";      return false; }
    if ( gpGlobals->maxClients != 12 )
    { if(pFail)*pFail="maxplayers not 12";   return false; }
    return true;
}

// "Certified base" — era state locked, valid day range.
// This now covers what was enforcement >= 2 (balance + weapon gate always on).
static bool TF2VCheckCertifiedBase( CUtlString *pFail )
{
    if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;

    int nDay = TFGameRules()->EraState().nCurrentEra;
    if ( nDay < TF2V_ERA_MIN || nDay > TF2V_ERA_MAX )
    { if(pFail)*pFail="tf2v_era out of range"; return false; }

    if ( !TFGameRules()->IsEraStateLocked() )
    { if(pFail)*pFail="era state not locked"; return false; }

    // Weapon era must mirror tf2v_era (always true in new model).
    if ( TFGameRules()->EraState().nAllowedWeaponEra != nDay )
    { if(pFail)*pFail="weapon era != tf2v_era"; return false; }

    if ( tf2v_server_type.GetInt() == 1 )
    {
        if ( nDay < TF2V_ERA_MVM_MIN )
        { if(pFail)*pFail="PVE requires day >= 1795 (MvM)"; return false; }
        ConVarRef defenders( "tf_mvm_defenders_team_size" );
        if ( defenders.IsValid() && defenders.GetInt() > 6 )
        { if(pFail)*pFail="PVE certified requires tf_mvm_defenders_team_size <= 6"; return false; }
    }

    if ( tf2v_server_type.GetInt() == 2 && nDay < TF2V_ERA_ASYM_MIN )
    { if(pFail)*pFail="ASYM requires day >= 5559 (VScript/VSH/ZI)"; return false; }

    return true;
}

// "Full certified" — certified base + mapcycle managed.
static bool TF2VCheckCertified( CUtlString *pFail )
{
    if ( !TF2VCheckCertifiedBase( pFail ) ) return false;

    if ( !tf2v_use_era_mapcycle.GetBool() )
    { if(pFail)*pFail="tf2v_use_era_mapcycle is 0 (no mapcycle gate)"; return false; }

    const char *pszExpected = TF2V_GetEraMapcycleFile(
        TFGameRules()->EraState().nCurrentEra );
    ConVarRef mapcyclefile( "mapcyclefile" );
    if ( pszExpected && mapcyclefile.IsValid() &&
         V_strcmp( mapcyclefile.GetString(), pszExpected ) != 0 )
    { if(pFail)*pFail="mapcyclefile does not match era"; return false; }

    return true;
}

static bool TF2VCheckQuietServer( CUtlString *pFail )
{
	// Placeholder. We need to implement disabling voice, text, and sprays on clientside. 
	// Clients can do this themselves, but the point of quiet servers is to force it closed.
	// This is to avoid griefing or a high stress experience.
    *pFail="Implementation for quiet servers not added yet."; 
	return false;
}

void CTFGameRules::TF2VUpdateQuickPlayCompliance()
{
    auto ClearAll = [&]()
    {
        tf2v_certified.SetValue( 0 );
        tf2v_certified_partial.SetValue( 0 );
        tf2v_certified_casual.SetValue( 0 );
        tf2v_certified_competitive.SetValue( 0 );
        tf2v_quiet_server.SetValue( 0 );
        tf2v_quickplay_casual.SetValue( 0 );
        tf2v_quickplay_competitive.SetValue( 0 );
    };

    if ( tf2v_quickplay_profile.GetInt() == 0 )
    { ClearAll(); return; }

    CUtlString fail;
    int nProfile = tf2v_quickplay_profile.GetInt();

    bool bCasual        = TF2VCheckCasual( &fail );
    bool bComp          = TF2VCheckCompetitive( &fail );

    bool bCertBase      = TF2VCheckCertifiedBase( &fail );
    bool bCertFull      = TF2VCheckCertified( &fail );

    bool bQuietServer   = TF2VCheckQuietServer( nullptr );

    bool bFullCasual    = bCertFull && bCasual;
    bool bFullComp      = bCertFull && bComp;
    bool bFullCert      = bFullCasual || bFullComp;

    bool bPartialCasual = !bFullCasual && bCertBase && bCasual;
    bool bPartialComp   = !bFullComp   && bCertBase && bComp;
    bool bPartialCert   = bPartialCasual || bPartialComp;

    bool bQPCasual      = !bFullCasual && !bPartialCasual && bCasual &&
                          ( nProfile == 1 || nProfile == 3 );
    bool bQPComp        = !bFullComp   && !bPartialComp   && bComp  &&
                          ( nProfile == 2 || nProfile == 3 );

    auto Log = [&]( const char *tag, bool bOld, bool bNew )
    {
        if ( bOld == bNew ) return;
        if ( bNew ) Msg( "[TF2V] Tag '%s' ACTIVE.\n", tag );
        else        Msg( "[TF2V] Tag '%s' lost: %s\n", tag, fail.Get() );
    };

    Log( "certified",             tf2v_certified.GetBool(),             bFullCert    );
    Log( "certified_partial",     tf2v_certified_partial.GetBool(),     bPartialCert );
    Log( "certified_casual",      tf2v_certified_casual.GetBool(),      bFullCasual || bPartialCasual );
    Log( "certified_competitive", tf2v_certified_competitive.GetBool(), bFullComp   || bPartialComp   );
    Log( "quiet_server",          tf2v_quiet_server.GetBool(),          bQuietServer         );
    Log( "quickplay_casual",      tf2v_quickplay_casual.GetBool(),      bQPCasual    );
    Log( "quickplay_competitive", tf2v_quickplay_competitive.GetBool(), bQPComp      );

    tf2v_certified.SetValue( bFullCert ? 1 : 0 );
    tf2v_certified_partial.SetValue( bPartialCert ? 1 : 0 );
    tf2v_certified_casual.SetValue( ( bFullCasual || bPartialCasual ) ? 1 : 0 );
    tf2v_certified_competitive.SetValue( ( bFullComp || bPartialComp ) ? 1 : 0 );
    tf2v_quiet_server.SetValue( bQuietServer ? 1 : 0 );
    tf2v_quickplay_casual.SetValue( bQPCasual ? 1 : 0 );
    tf2v_quickplay_competitive.SetValue( bQPComp ? 1 : 0 );
}

#endif // GAME_DLL

//=============================================================================
// END TF2V ERA SYSTEM IMPLEMENTATION
//=============================================================================

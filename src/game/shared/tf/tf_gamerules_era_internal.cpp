//=============================================================================
//
// TF2V ERA SYSTEM IMPLEMENTATION
//
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"            // CTFGameRules class + era member declarations
#include "tf_gamerules_convars.h"    // extern ConVar tf2v_* declarations
#include "tf_gamerules_era_internal.h"  // callback fwd decls + inline helpers


#ifdef GAME_DLL
extern ConVar tf_arena_first_blood;
extern ConVar hide_server;

//-----------------------------------------------------------------------------
// TF2V_GetEraMapcycleFile — returns era-accurate mapcycle filename.
//
// Selects variant based on tf2v_server_type:
//   0=PVP (default) — era files are all-PVP for eras 1-110 (no suffix),
//                     _pvp suffix for era 120+ which strip MVM/VSH/ZI.
//   1=PVE           — _pve suffix, MVM maps only, available era 120+.
//                     Falls back to PVP for eras before MVM existed.
//   2=ASYM          — _asym suffix, VSH/ZI maps only, era 180 only.
//                     Falls back to PVP for eras before VSH/ZI existed.
//
// Map/mode introduction dates:
//   Era 120: MVM launch (Aug 15 2012), sd_doomsday, koth_king
//   Era 130: mvm_mannhattan/rottenburg (Nov 2013), rd_asteroid, pd_watergate
//   Era 160: PASS Time (Jul 7 2016)
//   Era 180: VSH, ZI, all post-2018 community additions (terminal era)
//-----------------------------------------------------------------------------
static const char *TF2V_GetEraMapcycleFile( int nEra )
{
	int nType = tf2v_server_type.GetInt();

	// PVE — MVM-only rotation, era 120+
	if ( nType == 1 )
	{
		if ( nEra < TF2V_ERA_MVM_MIN )
		{
			DevWarning( "[TF2V] PVE mapcycle requested for era %d "
			            "but MVM doesn't exist until era 121. "
			            "Using PVP mapcycle instead.\n", nEra );
			// fall through to PVP below
		}
		else
		{
			if ( nEra <= 121) return "maps/mapcycle_era120_pve.txt";
			if ( nEra <= 130) return "maps/mapcycle_era130_pve.txt";
			if ( nEra <= 140) return "maps/mapcycle_era140_pve.txt";
			if ( nEra <= 150) return "maps/mapcycle_era150_pve.txt";
			if ( nEra <= 160) return "maps/mapcycle_era160_pve.txt";
			if ( nEra <= 170) return "maps/mapcycle_era170_pve.txt";
			return "maps/mapcycle_era170_pve.txt";  // MvM pool fixed after era121
		}
	}

	// ASYM — VSH/ZI rotation, era 180 only
	if ( nType == 2 )
	{
		if ( nEra < TF2V_ERA_ASYM_MIN )
		{
			DevWarning( "[TF2V] ASYM mapcycle requested for era %d "
			            "but VSH/ZI don't exist until era 190 (VScript). "
			            "Using PVP mapcycle instead.\n", nEra );
			// fall through to PVP below
		}
		else
		{
			if ( nEra <= 191 ) return "maps/mapcycle_era191_asym.txt";
			if ( nEra <= 192 ) return "maps/mapcycle_era192_asym.txt";
			if ( nEra <= 193 ) return "maps/mapcycle_era193_asym.txt";
			if ( nEra <= 194 ) return "maps/mapcycle_era194_asym.txt";
			// era 200+ includes HTF and Smissmas 2025 maps
			static const char *s_pszTermASYM = "maps/mapcycle_era" TF2V_ERA_MAX_STR "_asym.txt";
			return s_pszTermASYM;
		}
	}

	// PVP — standard rotation, cumulative pool grows at each content update.
	// All files use _pvp suffix for consistency.
	// Eras 1-170: launch through Jungle Inferno, precise per-update files.
	// Eras 180+:  post-balance-freeze, map-only additions.
	if ( nEra <=   1 ) return "maps/mapcycle_era1_pvp.txt";   // Oct 10 2007 Launch (6)
	if ( nEra <=   7 ) return "maps/mapcycle_era7_pvp.txt";   // Jan 25 2008 +ctf_well (7)
	if ( nEra <=   8 ) return "maps/mapcycle_era8_pvp.txt";   // Feb 14 2008 +cp_badlands (8)
	if ( nEra <=  10 ) return "maps/mapcycle_era10_pvp.txt";  // Apr 29 2008 Gold Rush (9)
	if ( nEra <=  20 ) return "maps/mapcycle_era20_pvp.txt";  // Jun 19 2008 Pyro Update (11)
	if ( nEra <=  30 ) return "maps/mapcycle_era30_pvp.txt";  // Aug 19 2008 Heavy Update (17)
	if ( nEra <=  31 ) return "maps/mapcycle_era31_pvp.txt";  // Dec 11 2008 +cp_steel (18)
	if ( nEra <=  50 ) return "maps/mapcycle_era50_pvp.txt";  // Feb 24 2009 Scout Update (19)
	if ( nEra <=  60 ) return "maps/mapcycle_era60_pvp.txt";  // May 21 2009 Sniper/Spy (20)
	if ( nEra <=  70 ) return "maps/mapcycle_era70_pvp.txt";  // Aug 13 2009 Classless/KOTH (32)
	if ( nEra <=  71 ) return "maps/mapcycle_era71_pvp.txt";  // Sep 15 2009 egypt/junction (34)
	if ( nEra <=  80 ) return "maps/mapcycle_era80_pvp.txt";  // Dec 17 2009 WAR! (35)
	if ( nEra <=  82 ) return "maps/mapcycle_era82_pvp.txt";  // Apr 28 2010 freight/upward (37)
	if ( nEra <=  90 ) return "maps/mapcycle_era90_pvp.txt";  // Jul  8 2010 Engineer Update (39)
	if ( nEra <=  91 ) return "maps/mapcycle_era91_pvp.txt";  // Oct 27 2010 Scream Fortress (42)
	if ( nEra <= 103 ) return "maps/mapcycle_era103_pvp.txt"; // Dec 17 2010 Aus Christmas (44)
	if ( nEra <= 104 ) return "maps/mapcycle_era104_pvp.txt"; // Jan 19 2011 +cp_5gorge (45)
	if ( nEra <= 105 ) return "maps/mapcycle_era105_pvp.txt"; // Apr 14 2011 lakeside/badlands (49)
	if ( nEra <= 110 ) return "maps/mapcycle_era110_pvp.txt"; // Jun 23 2011 F2P/Uber (51)
	if ( nEra <= 111 ) return "maps/mapcycle_era111_pvp.txt"; // Aug 31 2011 +viaduct_event (52)
	if ( nEra <= 112 ) return "maps/mapcycle_era112_pvp.txt"; // Dec 15 2011 Atribute+foundry (53)
	if ( nEra <= 115 ) return "maps/mapcycle_era115_pvp.txt"; // Jul 10 2013 process/standin (59)
	if ( nEra <= 117 ) return "maps/mapcycle_era117_pvp.txt"; // Nov 2013 snakewater/helltower (60)
	if ( nEra <= 130 ) return "maps/mapcycle_era130_pvp.txt"; // Jun 2014 Love & War (65)
	if ( nEra <= 133 ) return "maps/mapcycle_era133_pvp.txt"; // Dec 2014 Mannpower (65, same)
	if ( nEra <= 140 ) return "maps/mapcycle_era140_pvp.txt"; // Jul 2015 Gun Mettle (69)
	if ( nEra <= 150 ) return "maps/mapcycle_era150_pvp.txt"; // Dec 2015 Tough Break (73)
	if ( nEra <= 160 ) return "maps/mapcycle_era160_pvp.txt"; // Jul 2016 MYM (88)
	if ( nEra <= 170 ) return "maps/mapcycle_era170_pvp.txt"; // Oct 2017 Jungle Inferno (97)

	// Post-balance-freeze (era 180+): balance unchanged, map pool grows each
	// holiday update. Each file is cumulative — it includes all prior maps.
	if ( nEra <= 180 ) return "maps/mapcycle_era180_pvp.txt";  // Mar 2018 freeze (= era170)
	if ( nEra <= 181 ) return "maps/mapcycle_era181_pvp.txt";  // SF X  Oct 19 2018 (+5)
	if ( nEra <= 184 ) return "maps/mapcycle_era182_pvp.txt";  // SF XI Oct 10 2019 (+2); 183-184 no maps
	if ( nEra <= 185 ) return "maps/mapcycle_era185_pvp.txt";  // SF XII  Oct 1 2020  (+4)
	if ( nEra <= 186 ) return "maps/mapcycle_era186_pvp.txt";  // Smissmas 2020 Dec 3 (+4)
	if ( nEra <= 187 ) return "maps/mapcycle_era187_pvp.txt";  // SF XIII Oct 5 2021  (+6)
	if ( nEra <= 188 ) return "maps/mapcycle_era188_pvp.txt";  // Smissmas 2021 Dec 2 (+6)
	if ( nEra <= 189 ) return "maps/mapcycle_era189_pvp.txt";  // SF XIV  Oct 5 2022  (+5)
	if ( nEra <= 190 ) return "maps/mapcycle_era190_pvp.txt";  // VScript+Smissmas 2022 (+5)
	if ( nEra <= 191 ) return "maps/mapcycle_era191_pvp.txt";  // SF XV   Oct 9 2023  (+12)
	if ( nEra <= 192 ) return "maps/mapcycle_era192_pvp.txt";  // Smissmas 2023 Dec 7 (+8)
	if ( nEra <= 193 ) return "maps/mapcycle_era193_pvp.txt";  // Summer+SF XVI 2024 (+7)
	if ( nEra <= 199 ) return "maps/mapcycle_era194_pvp.txt";  // Smissmas 2024 Dec 11 (+5)
	if ( nEra <= 200 ) return "maps/mapcycle_era200_pvp.txt";  // Summer 2025 Jul 24 (+9)
	if ( nEra <= 201 ) return "maps/mapcycle_era201_pvp.txt";  // SF XVII Oct 9 2025
	static const char *s_pszTermPVP = "maps/mapcycle_era" TF2V_ERA_MAX_STR "_pvp.txt";
	return s_pszTermPVP;
}


//-----------------------------------------------------------------------------
// TF2VGetGamemodeMinEra — returns the earliest era a gamemode officially existed.
// Maps the tf_gamemode_* flag convars to their introduction era.
// Returns 0 for gamemodes that shipped at launch (CTF, CP, TC).
// Returns TF2V_ERA_MAX+1 for unknown/unsupported gamemodes.
//
// Used by TF2VCheckMapGamemode to enforce period-accurate map rotation on
// certified strict servers (tf2v_enforcement == 3).
//-----------------------------------------------------------------------------
static int TF2VGetGamemodeMinEra( const char *pszGamemodeConvar )
{
	struct GamemodeEra_t
	{
		const char *pszConvar;
		int nMinEra;
	};

	static const GamemodeEra_t s_GamemodeEras[] =
	{

		// PVP — launch window

		{ "tf_gamemode_ctf",        TF2V_ERA_MIN  },  // Oct 10 2007 launch

		{ "tf_gamemode_cp",         TF2V_ERA_MIN  },  // Oct 10 2007 launch

		{ "tf_gamemode_tc",         TF2V_ERA_MIN  },  // Oct 10 2007 launch (tc_hydro)

		// PVP — content updates

		{ "tf_gamemode_payload",    10   },  // Apr 29 2008 Gold Rush

		{ "tf_gamemode_arena",      30   },  // Aug 19 2008 Heavy Update

		{ "tf_gamemode_koth",       70   },  // Aug 13 2009 Classless Update

		{ "tf_gamemode_plr",        80   },  // Dec 17 2009 WAR! Update

		{ "tf_gamemode_medieval",   103  },  // Dec 17 2010 Australian Christmas

		{ "tf_gamemode_sd",         120  },  // Jun 27 2012 Pyromania

		{ "tf_gamemode_rd",         130  },  // Jun 18 2014 Love & War

		{ "tf_gamemode_pd",         130  },  // Jun 18 2014 Love & War

		{ "tf_gamemode_mannpower",  133  },  // Dec 22 2014 Smissmas 2014 (beta)

		{ "tf_gamemode_passtime",   160  },  // Jul 7 2016 Meet Your Match

		// PVE

		{ "tf_gamemode_mvm",        TF2V_ERA_MVM_MIN  },  // Aug 15 2012 MvM launch

		// ASYM (VScript-based)

		{ "tf_gamemode_vsh",        TF2V_ERA_ASYM_MIN },  // Jul 12 2023 Summer 2023

		{ "tf_gamemode_dr",         TF2V_ERA_ASYM_MIN },  // ZI uses same era floor

	};

	for ( int i = 0; i < ARRAYSIZE( s_GamemodeEras ); i++ )

	{
		if ( V_strcmp( pszGamemodeConvar, s_GamemodeEras[i].pszConvar ) == 0 )

			return s_GamemodeEras[i].nMinEra;
	}

	return TF2V_ERA_MAX + 1;  // unknown gamemode — never valid
}

//-----------------------------------------------------------------------------
// TF2VCheckMapGamemode — returns false (with reason) if the current map's
// gamemode is not yet introduced at the active era.
// Only enforced when tf2v_enforcement == 3 (strict certified).
// Allows the server to serve as an era-floor guarantee even if the operator
// manually sets a mapcycle with pre-era maps.
//-----------------------------------------------------------------------------
#ifdef GAME_DLL

static bool TF2VCheckMapGamemode( const char **pFail = NULL )
{

	if ( !TFGameRules() ) return true;

	int nEra = tf2v_era.GetInt();


	// Build list of active gamemode convars

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


		int nMinEra = TF2VGetGamemodeMinEra( s_GamemodeConvars[i] );

		if ( nEra < nMinEra )
		{

			if ( pFail )

				*pFail = "map gamemode not introduced until a later era";

			DevWarning( "[TF2V] Gamemode %s requires era >= %d (current era %d)\n",

			            s_GamemodeConvars[i], nMinEra, nEra );

			return false;
		}
	}
	return true;
}
#endif

//-----------------------------------------------------------------------------
// TF2VApplyMapcycle — sets mapcyclefile based on current mode and era.
// Called on mode change, era change (when mode 1), and at round start.
//-----------------------------------------------------------------------------
static void TF2VApplyMapcycle()
{
	if ( !engine ) return;
	if ( !TF2V_MapcycleManaged() ) return;  // enforcement < 3 — server controls mapcyclefile

	const char *pszFile = TF2V_GetEraMapcycleFile( tf2v_era.GetInt() );
	if ( !pszFile )
	{
		Msg( "[TF2V] No era-accurate mapcycle for era %d.\n", tf2v_era.GetInt() );
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

void TF2VMapcycleModeChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	TF2VApplyMapcycle();
}

//-----------------------------------------------------------------------------
// TF2VServerTypeChanged — switches to the matching mapcycle variant.
// Applies immediately when not mid-round; defers to round boundary if live.
// Revalidates compliance since PVE/ASYM have different player count rules.
//-----------------------------------------------------------------------------
// Returns the minimum valid era for a given server type.
// A server cannot run below this era without losing its server type designation.
//   PVP  (0): no floor — any era from TF2V_ERA_MIN is valid
//   PVE  (1): TF2V_ERA_MVM_MIN — MvM doesn't exist before era 121
//   ASYM (2): TF2V_ERA_ASYM_MIN — VSH/ZI require VScript (era 190)
static int TF2VGetServerTypeEraFloor( int nServerType )
{
	switch ( nServerType )
	{
	case 1:  return TF2V_ERA_MVM_MIN;   // PVE
	case 2:  return TF2V_ERA_ASYM_MIN;  // ASYM
	default: return TF2V_ERA_MIN;        // PVP — no floor
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

	// Era floor enforcement: if the new server type requires a higher minimum
	// era than the current setting, clamp up to that floor immediately.
	// This prevents a PVE server from sitting at era 50 (no MvM) or an ASYM
	// server from sitting at era 150 (no VScript/VSH/ZI).
	// We clamp upward only — switching from ASYM back to PVP does not force
	// the era down, because a PVP server at era 190 is perfectly valid.
	if ( TF2V_EraManaged() )
	{
		int nFloor = TF2VGetServerTypeEraFloor( nNew );
		int nEra   = tf2v_era.GetInt();
		if ( nEra < nFloor )
		{
			Msg( "[TF2V] Server type %s requires era >= %d. "
			     "Raising era from %d to %d.\n",
			     s_pszTypeNames[ clamp( nNew, 0, 2 ) ], nFloor, nEra, nFloor );
			tf2v_era.SetValue( nFloor );
		}
	}

	// Mapcycle update: only applies when enforcement is managing the mapcycle
	if ( TF2V_MapcycleManaged() )
	{
		if ( CTFGameRules::TF2V_IsRoundActive() )
		{
			// Can't safely hot-swap the mapcycle mid-round.
			// Revert and let admin change it during intermission.
			TFGameRules()->m_bApplyingEra = true;
			var.SetValue( nOld );
			TFGameRules()->m_bApplyingEra = false;
			Msg( "[TF2V] tf2v_server_type change deferred — "
			     "make changes during intermission.\n" );
			return;
		}

		TF2VApplyMapcycle();
	}

	// Revalidate compliance — server type floors and cert requirements differ
	if ( tf2v_quickplay_profile.GetInt() > 0 )
		TFGameRules()->TF2VUpdateQuickPlayCompliance();
}

//-----------------------------------------------------------------------------
// Returns true during active play — era changes must be deferred.
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
//-----------------------------------------------------------------------------
void TF2VEraChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	if ( !TFGameRules() || !TF2V_EraManaged() ) return;
	if ( TFGameRules()->m_bApplyingEra ) return;

	ConVarRef var( pConVar );
	int nNew = var.GetInt();
	int nOld = (int)flOldValue;
	if ( nNew == nOld ) return;

	// Hard clamp to [TF2V_ERA_MIN, TF2V_ERA_MAX]
	int nClamped = clamp( nNew, TF2V_ERA_MIN, TF2V_ERA_MAX );
	if ( nClamped != nNew )
	{
		Warning( "[TF2V] Era %d out of range [%d, %d] — clamping to %d.\n",
		         nNew, TF2V_ERA_MIN, TF2V_ERA_MAX, nClamped );
		TFGameRules()->m_bApplyingEra = true;
		var.SetValue( nClamped );
		TFGameRules()->m_bApplyingEra = false;
		nNew = nClamped;
	}

	// Server type floor: PVE requires era >= TF2V_ERA_MVM_MIN,
	// ASYM requires era >= TF2V_ERA_ASYM_MIN.
	// A managed server cannot be set below its server type's era floor.
	int nFloor = TF2VGetServerTypeEraFloor( tf2v_server_type.GetInt() );
	if ( nNew < nFloor )
	{
		static const char *s_pszTypeNames[] = { "PVP", "PVE", "ASYM" };
		Warning( "[TF2V] Era %d is below the floor for %s servers (era %d). "
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

		Msg( "[TF2V] Era change to %d queued — takes effect at next round boundary.\n", nNew );
	}
	else
	{
		TFGameRules()->ApplyEra( nNew );
	}
}

//-----------------------------------------------------------------------------
// TF2VEnforcementChanged — handles switching enforcement levels.
//-----------------------------------------------------------------------------
void TF2VEnforcementChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	if ( !TFGameRules() ) return;
	int nNew = tf2v_enforcement.GetInt();
	int nOld = (int)flOldValue;

	if ( nNew >= 1 && nOld < 1 )
	{
		Msg( "[TF2V] Enforcement %d: era management enabled. Applying era %d.\n",
		     nNew, tf2v_era.GetInt() );
		TFGameRules()->ApplyEra( tf2v_era.GetInt() );
	}
	else if ( nNew < 1 && nOld >= 1 )
	{
		Msg( "[TF2V] Enforcement 0: full manual control.\n" );
		TFGameRules()->m_bManualSnapshotStale = true;
	}

	if ( nNew >= 3 && nOld < 3 )
		TF2VApplyMapcycle();
	else if ( nNew < 3 && nOld >= 3 )
		Msg( "[TF2V] Enforcement %d: mapcycle no longer managed by TF2V.\n", nNew );

	if ( tf2v_quickplay_profile.GetInt() > 0 )
		TFGameRules()->TF2VUpdateQuickPlayCompliance();
}

//-----------------------------------------------------------------------------
// TF2VAnySubConvarChanged — reverts mid-round sub-convar changes.
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

		if ( TF2V_EraManaged() )
		{
			TFGameRules()->m_bEraDirty = true;
			Msg( "[TF2V] %s reverted — change tf2v_era instead, takes effect next round.\n",
			     var.GetName() );
		}
		else
		{
			Msg( "[TF2V] %s reverted — make changes during intermission.\n",
			     var.GetName() );
		}
	}
	else
	{
		if ( !TF2V_EraManaged() )
			TFGameRules()->m_bManualSnapshotStale = true;
	}
}

//-----------------------------------------------------------------------------
// LockEraState — snapshots current convar values into m_EraState.
// Called at round start after any pending era changes are resolved.
// All gameplay code reads from EraState(), never from convars directly.
//-----------------------------------------------------------------------------
void CTFGameRules::LockEraState()
{
	TF2VEraState_t &s = m_EraState;

	if ( TF2V_EraManaged() )
	{
		// Enforcement 1+: ApplyEra() ran, convars are authoritative.
		// Weapon era: locked to tf2v_era when enforcement >= 2,
		// or uses tf2v_allowed_weapon_era when enforcement is 1 (balance only).
		s.nAllowedWeaponEra = TF2V_WeaponGated()
			? tf2v_era.GetInt()
			: tf2v_allowed_weapon_era.GetInt();
	}
	else
	{
		// Enforcement 0: snapshot system restored server config values.
		// Weapon gate from tf2v_allowed_weapon_era (server config authority).
		s.nAllowedWeaponEra = tf2v_allowed_weapon_era.GetInt();
	}

	// All other fields read from convars regardless of mode —
	// in managed mode ApplyEra() set them, in manual mode the
	// snapshot system restored them. Either way they're correct.
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

	DevMsg( "[TF2V] LockEraState: enforcement=%d era=%d weapon_era=%d\n",
		tf2v_enforcement.GetInt(), tf2v_era.GetInt(), s.nAllowedWeaponEra );
}

//-----------------------------------------------------------------------------
// Manual mode snapshot — takes/restores convar values for server config auth.
//-----------------------------------------------------------------------------
void CTFGameRules::TakeManualEraSnapshot()
{
	m_ManualEraSnapshot.RemoveAll();
	m_bManualSnapshotTaken = false;

	static const char *s_pszManaged[] =
	{
		"tf2v_allowed_weapon_era","tf2v_damage_spread_mode","tf2v_crit_model",
		"tf2v_ctf_capcrits","tf_arena_first_blood","tf2v_fall_sounds",
		"tf2v_console_grenadelauncher_damage","tf2v_console_grenadelauncher_magazine",
		"tf2v_grenades_explode_contact","tf2v_grenade_player_collision",
		"tf2v_use_new_grenade_radius","tf2v_use_new_demo_explosion_variance",
		"tf2v_use_stickybomb_damage_rampup","tf2v_use_stickybomb_radius_rampup",
		"tf2v_sticky_bullet_break","tf2v_ammo_era","tf2v_use_new_blackbox",
		"tf2v_airblast","tf2v_airblast_players","tf2v_flame_mode",
		"tf2v_minicrits_on_deflect","tf2v_use_extinguish_heal",
		"tf2v_use_extinguish_cooldown","tf2v_use_new_flare","tf2v_use_new_flare_radius",
		"tf2v_use_new_phlog_fill","tf2v_use_new_phlog_taunt","tf2v_use_new_axtinguisher",
		"tf2v_use_new_minigun_rampup","tf2v_sandvich_behavior",
		"tf2v_building_upgrades","tf2v_building_hauling","tf2v_use_new_hauling_speed",
		"tf2v_use_new_wrench_mechanics","tf2v_use_new_sapper_damage",
		"tf2v_use_new_sapper_disable","tf2v_use_new_sentry_minigun_resist",
		"tf2v_new_sentry_wrangle_location","tf2v_new_sentry_damage_falloff",
		"tf2v_use_new_teleporter_cost","tf2v_use_new_short_circuit",
		"tf2v_use_new_minibuildings","tf2v_use_new_medic_regen","tf2v_medigun_heal_rate",
		"tf2v_setup_uber_rate","tf2v_uber_juggle_penalty","tf2v_use_new_uber_taunt",
		"tf2v_use_medic_speed_match","tf2v_use_new_health_regen_attrib",
		"tf2v_sandman_stun_type","tf2v_use_new_bonk_length",
		"tf2v_use_new_sodapopper_hype","tf2v_use_new_sodapopper_fill",
		"tf2v_use_manual_sodapopper","tf2v_use_shortstop_shove",
		"tf2v_use_shortstop_slowdown","tf2v_use_new_guillotine",
		"tf2v_use_new_ball_regen","tf2v_use_new_buff_charges",
		"tf2v_sentry_resist_bonus","tf2v_use_new_equalizer_damage",
		"tf2v_use_new_split_equalizer","tf2v_new_speed_buff_duration",
		"tf2v_use_new_beggars","tf2v_demo_charge_debuff_remove",
		"tf2v_use_new_caber","tf2v_use_new_honorbound",
		"tf2v_spy_cloak_reload","tf2v_spy_cloak_ammo_recharge","tf2v_use_new_cloak",
		"tf2v_new_feign_death_activate","tf2v_new_feign_death_stealth",
		"tf2v_use_new_yer","tf2v_use_new_big_earner","tf2v_use_fast_redisguise",
		"tf2v_allow_disguiseweapons","tf2v_disguise_spy_teleport",
		"tf2v_disguise_speed_match","tf2v_use_new_spy_movespeeds",
		"tf2v_spy_base_speed","tf2v_use_new_ambassador","tf2v_use_new_diamondback",
		"tf2v_use_new_pomson","tf2v_allow_sniper_crosshairs","tf2v_use_new_cleaners",
		"tf2v_use_new_bison_damage","tf2v_use_new_bison_speed","tf2v_use_new_autofire",
		"tf2v_use_new_weapon_swap_speed","tf2v_reload_cancel_available",
		"tf2v_use_faster_reload","tf2v_new_chocolate_behavior","tf2v_use_new_atomizer",
		"tf2v_use_new_backstabs","tf2v_use_new_jag","tf2v_radius_damage_teammates",
		"tf2v_clamp_speed_absolute","tf2v_class_death_animations",
		"tf2v_minicrit_self_inflicted","tf2v_disable_updraft","tf2v_prevent_voice_spam",
		"tf2v_critchance_melee","tf2v_crit_duration_rapid",
	};

	for ( int i = 0; i < ARRAYSIZE( s_pszManaged ); i++ )
	{
		ConVarRef cv( s_pszManaged[i] );
		if ( !cv.IsValid() ) continue;
		TF2VManualSnapshot_t snap;
		snap.pszName = s_pszManaged[i];
		V_strncpy( snap.szValue, cv.GetString(), sizeof(snap.szValue) );
		m_ManualEraSnapshot.AddToTail( snap );
	}

	m_bManualSnapshotTaken = true;
	DevMsg( "[TF2V] Manual snapshot taken (%d convars).\n",
		m_ManualEraSnapshot.Count() );
}

void CTFGameRules::RestoreManualEraSnapshot()
{
	if ( !m_bManualSnapshotTaken )
	{
		TakeManualEraSnapshot();
		return;
	}

	int nDrifted = 0;
	m_bApplyingEra = true;

	for ( int i = 0; i < m_ManualEraSnapshot.Count(); i++ )
	{
		const TF2VManualSnapshot_t &snap = m_ManualEraSnapshot[i];
		ConVarRef cv( snap.pszName );
		if ( !cv.IsValid() ) continue;
		if ( V_strcmp( cv.GetString(), snap.szValue ) != 0 )
		{
			cv.SetValue( snap.szValue );
			nDrifted++;
		}
	}

	m_bApplyingEra = false;

	if ( nDrifted > 0 )
		Msg( "[TF2V] Restored %d mid-round drifted convar(s) to round-start values.\n",
			nDrifted );
}

//-----------------------------------------------------------------------------
// TF2VUpdateQuickPlayCompliance — computes all tier/mode tags.
// Called at round boundary and every 30 seconds from Think().
//-----------------------------------------------------------------------------

// Helper: base QuickPlay requirements
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

// Helper: casual mode
static bool TF2VCheckCasual( CUtlString *pFail )
{
	if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;
	ConVarRef tf_weapon_criticals( "tf_weapon_criticals" );
	if ( tf_weapon_criticals.IsValid() && !tf_weapon_criticals.GetBool() )
	{ if(pFail)*pFail="crits off (use competitive)"; return false; }
	return true;
}

// Helper: competitive mode
static bool TF2VCheckCompetitive( CUtlString *pFail )
{
	if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;
	ConVarRef tf_weapon_criticals( "tf_weapon_criticals" );
	ConVarRef tf_damage_disablespread( "tf_damage_disablespread" );
	ConVarRef tf_use_fixed_weaponspreads( "tf_use_fixed_weaponspreads" );
	ConVarRef sv_pure( "sv_pure" );
	ConVarRef mp_decals( "mp_decals" );

	if ( tf_weapon_criticals.IsValid() && tf_weapon_criticals.GetBool() )
	{ if(pFail)*pFail="crits on";           return false; }
	if ( !( ( tf_damage_disablespread.IsValid() && tf_damage_disablespread.GetBool() ) ||
	        ( tf_use_fixed_weaponspreads.IsValid() && tf_use_fixed_weaponspreads.GetBool() ) ) )
	{ if(pFail)*pFail="spread not disabled"; return false; }
	if ( sv_pure.IsValid() && sv_pure.GetInt() < 1 )
	{ if(pFail)*pFail="sv_pure < 1";        return false; }
	if ( mp_decals.IsValid() && mp_decals.GetInt() > 0 )
	{ if(pFail)*pFail="sprays enabled";     return false; }
	if ( gpGlobals->maxClients != 12 )
	{ if(pFail)*pFail="maxplayers not 12";  return false; }
	return true;
}

// Helper: certified base — enforcement >= 2, valid era, era state locked,
//         weapon era matches tf2v_era
static bool TF2VCheckCertifiedBase( CUtlString *pFail )
{
	if ( !TF2VCheckQuickPlayBase( pFail ) ) return false;

	// Requires at least partial enforcement (era managed + weapon gated)
	if ( tf2v_enforcement.GetInt() < 2 )
	{ if(pFail)*pFail="tf2v_enforcement < 2"; return false; }

	int nEra = tf2v_era.GetInt();
	if ( nEra < TF2V_ERA_MIN || nEra > TF2V_ERA_MAX )
	{ if(pFail)*pFail="tf2v_era out of range"; return false; }

	if ( !TFGameRules()->IsEraStateLocked() )
	{ if(pFail)*pFail="era state not locked"; return false; }

	// Weapon era must be locked to tf2v_era (enforcement >= 2 guarantees this)
	if ( TFGameRules()->EraState().nAllowedWeaponEra != nEra )
	{ if(pFail)*pFail="weapon era != tf2v_era"; return false; }

	// PVE servers must be era 120+ (MVM doesn't exist before that).
	// Human team size is controlled by tf_mvm_defenders_team_size, NOT maxplayers —
	// MvM runs with bots filling the invader team so gpGlobals->maxClients is
	// intentionally higher than the human player cap.
	// Standard MvM = 6 defenders. This is what certification verifies.
	if ( tf2v_server_type.GetInt() == 1 )
	{
		if ( nEra < TF2V_ERA_MVM_MIN )
		{ if(pFail)*pFail="PVE requires era >= 121 (MVM)"; return false; }

		ConVarRef defenders( "tf_mvm_defenders_team_size" );
		if ( defenders.IsValid() && defenders.GetInt() > 6 )
		{ if(pFail)*pFail="PVE certified requires tf_mvm_defenders_team_size <= 6"; return false; }
	}

	// ASYM servers must be era 180 (VSH/ZI don't exist before that)
	if ( tf2v_server_type.GetInt() == 2 && nEra < TF2V_ERA_ASYM_MIN )
	{ if(pFail)*pFail="ASYM requires era 190+ (VScript/VSH/ZI)"; return false; }

	return true;
}

// Helper: full certified — enforcement == 3, mapcycle matches era + gamemode valid
static bool TF2VCheckCertified( CUtlString *pFail )
{
	if ( !TF2VCheckCertifiedBase( pFail ) ) return false;

	if ( tf2v_enforcement.GetInt() < 3 )
	{ if(pFail)*pFail="tf2v_enforcement < 3 (no mapcycle gate)"; return false; }

	const char *pszExpected = TF2V_GetEraMapcycleFile( tf2v_era.GetInt() );
	ConVarRef mapcyclefile( "mapcyclefile" );
	if ( pszExpected && mapcyclefile.IsValid() &&
	     V_strcmp( mapcyclefile.GetString(), pszExpected ) != 0 )
	{ if(pFail)*pFail="mapcyclefile does not match era"; return false; }

	return true;
}

// Helper: PS3 — full certified + casual + era 0 + 16 players
static bool TF2VCheckPS3( CUtlString *pFail )
{
	if ( !TF2VCheckCertified( pFail ) ) return false;
	if ( !TF2VCheckCasual( pFail ) )    return false;
	if ( tf2v_era.GetInt() != 0 )
	{ if(pFail)*pFail="PS3 requires era 0"; return false; }
	if ( gpGlobals->maxClients != 16 )
	{ if(pFail)*pFail="PS3 requires 16 players"; return false; }
	return true;
}

// Helper: Xbox 360 — full certified + era 7 + 16 players (mode checked separately)
static bool TF2VCheckXbox( CUtlString *pFail )
{
	if ( !TF2VCheckCertified( pFail ) ) return false;
	if ( tf2v_era.GetInt() != 4 )
	{ if(pFail)*pFail="Xbox requires era 4"; return false; }
	if ( gpGlobals->maxClients != 16 )
	{ if(pFail)*pFail="Xbox requires 16 players"; return false; }
	return true;
}

void CTFGameRules::TF2VUpdateQuickPlayCompliance()
{
	auto ClearAll = [&]()
	{
		tf2v_certified.SetValue( 0 );
		tf2v_certified_partial.SetValue( 0 );
		tf2v_certified_casual.SetValue( 0 );
		tf2v_certified_competitive.SetValue( 0 );
		tf2v_certified_ps3.SetValue( 0 );
		tf2v_certified_xbox.SetValue( 0 );
		tf2v_quickplay_casual.SetValue( 0 );
		tf2v_quickplay_competitive.SetValue( 0 );
	};

	if ( tf2v_quickplay_profile.GetInt() == 0 )
	{ ClearAll(); return; }

	CUtlString fail;
	int nProfile = tf2v_quickplay_profile.GetInt();

	// Mode checks
	bool bCasual        = TF2VCheckCasual( &fail );
	bool bComp          = TF2VCheckCompetitive( &fail );

	// Certification tiers
	bool bCertBase      = TF2VCheckCertifiedBase( &fail );  // enforcement >= 2
	bool bCertFull      = TF2VCheckCertified( &fail );      // enforcement == 3

	bool bPS3           = TF2VCheckPS3( nullptr );
	bool bXboxBase      = TF2VCheckXbox( nullptr );

	// Full certification (enforcement 3)
	bool bFullCasual    = bCertFull && bCasual;
	bool bFullComp      = bCertFull && bComp;
	bool bFullCert      = bFullCasual || bFullComp;

	// Partial certification (enforcement 2 — balance + weapons, any maps)
	// Only fires if full cert doesn't — partial is the lower tier
	bool bPartialCasual = !bFullCasual && bCertBase && bCasual;
	bool bPartialComp   = !bFullComp   && bCertBase && bComp;
	bool bPartialCert   = bPartialCasual || bPartialComp;

	// Xbox can be casual or competitive at either cert tier
	bool bXboxCasual    = bXboxBase && bCasual;
	bool bXboxComp      = bXboxBase && bComp;
	bool bXbox          = bXboxCasual || bXboxComp;

	// QuickPlay (non-certified — passes base requirements only)
	bool bQPCasual      = !bFullCasual  && !bPartialCasual && bCasual &&
	                      ( nProfile == 1 || nProfile == 3 );
	bool bQPComp        = !bFullComp    && !bPartialComp   && bComp  &&
	                      ( nProfile == 2 || nProfile == 3 );

	// Log changes
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
	Log( "ps3",                   tf2v_certified_ps3.GetBool(),         bPS3         );
	Log( "xbox",                  tf2v_certified_xbox.GetBool(),        bXbox        );
	Log( "quickplay_casual",      tf2v_quickplay_casual.GetBool(),      bQPCasual    );
	Log( "quickplay_competitive", tf2v_quickplay_competitive.GetBool(), bQPComp      );

	tf2v_certified.SetValue( bFullCert ? 1 : 0 );
	tf2v_certified_partial.SetValue( bPartialCert ? 1 : 0 );
	// certified_casual/competitive fire for both full and partial
	tf2v_certified_casual.SetValue( ( bFullCasual || bPartialCasual ) ? 1 : 0 );
	tf2v_certified_competitive.SetValue( ( bFullComp || bPartialComp ) ? 1 : 0 );
	tf2v_certified_ps3.SetValue( bPS3 ? 1 : 0 );
	tf2v_certified_xbox.SetValue( bXbox ? 1 : 0 );
	tf2v_quickplay_casual.SetValue( bQPCasual ? 1 : 0 );
	tf2v_quickplay_competitive.SetValue( bQPComp ? 1 : 0 );
}

#endif // GAME_DLL

//=============================================================================
// END TF2V ERA SYSTEM IMPLEMENTATION
//=============================================================================

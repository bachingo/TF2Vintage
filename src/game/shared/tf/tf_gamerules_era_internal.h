//=============================================================================
// tf_gamerules_era_internal.h
//
// Internal forward declarations shared between tf_gamerules.cpp and
// tf_gamerules_convars.cpp. NOT intended for inclusion outside those two TUs.
//
// Including this header gives you:
//   - Forward declarations for the four static convar callbacks so that
//     tf_gamerules_convars.cpp can pass them as callback arguments.
//   - The three static inline enforcement-level helpers so that any code
//     inside tf_gamerules.cpp can call them regardless of definition order.
//
// Do NOT include this in tf_gamerules.h or any public header.
//=============================================================================
#ifndef TF_GAMERULES_ERA_INTERNAL_H
#define TF_GAMERULES_ERA_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif


// -------------------------------------------------------------------------
// Era integer bounds.
// TF2V_ERA_MIN: PS3 internal build (era 0).
// TF2V_ERA_MAX: terminal balance state. Update this when new eras are added
//               (e.g. yearly holiday content drops beyond the March 2018
//               balance freeze). All validation, clamping, and cascade
//               code should reference these rather than hardcoding values.
// -------------------------------------------------------------------------
#define TF2V_ERA_MIN        0
#define TF2V_ERA_MAX        200  // TF2 SDK Release (Feb 18 2025)

// Era boundary constants for significant content thresholds.
// Used by gamemode validation to enforce period-accurate restrictions.
#define TF2V_ERA_POST_BALANCE   180  // Mar 2018 — balance freeze; map/content still added
#define TF2V_ERA_VSCRIPT        190  // Dec 1 2022 — VScript; VSH/ZI basis
#define TF2V_ERA_ASYM_MIN       190  // Earliest era with official ASYM gamemodes
#define TF2V_ERA_MVM_MIN        121  // Mann vs. Machine (Aug 15 2012)

// Gamemode minimum era table.
// TF2VGetGamemodeMinEra( int nGamemodeFlag ) returns the era at which
// the given gamemode first existed. A certified strict server must
// never run a gamemode before its introduction era.
// Gamemode flags match tf_gamemode_* convar semantics.
//
// Era | Gamemode          | Date
//   0   CTF, CP, TC         Oct 10 2007 (launch)
//  10   Payload              Apr 29 2008 (Gold Rush)
//  30   Arena                Aug 19 2008 (Heavy Update)
//  70   KOTH                 Aug 13 2009 (Classless Update)
//  80   Payload Race         Dec 17 2009 (WAR! Update)
// 103   Medieval Mode        Dec 17 2010 (Australian Christmas)
// 120   Special Delivery     Jun 27 2012 (Pyromania)
// 121   MvM (PVE)            Aug 15 2012 (Mann vs. Machine)
// 130   Robot Destruction    Jun 18 2014 (Love & War)
// 130   Player Destruction   Jun 18 2014 (Love & War)
// 133   Mannpower            Dec 22 2014 (Smissmas 2014, beta)
// 160   PASS Time            Jul 7 2016  (Meet Your Match)
// 190   VSH (ASYM)           Jul 12 2023 (Summer 2023)
// 190   ZI  (ASYM)           Oct 9 2023  (Scream Fortress XV)
// 200   Tug of War           Oct 2024    (Scream Fortress XVI)
// 200   Hold the Flag        Oct 2025    (Scream Fortress XVII)

// Helper: stringify an integer constant for use as a ConVar default string
// or in compile-time string concatenation.
// TF2V_ERA_MAX_STR expands to the string literal form of TF2V_ERA_MAX.
#define _TF2V_STRINGIFY(x)  #x
#define TF2V_STRINGIFY(x)   _TF2V_STRINGIFY(x)
#define TF2V_ERA_MAX_STR    TF2V_STRINGIFY(TF2V_ERA_MAX)  // e.g. "200"

#ifdef GAME_DLL

// ---------------------------------------------------------------------------
// Convar callback forward declarations.
// Definitions live in tf_gamerules.cpp in the era system implementation block.
// tf_gamerules_convars.cpp references these as callback arguments.
// ---------------------------------------------------------------------------
static void TF2VEraChanged         ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VEnforcementChanged ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VMapcycleModeChanged( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VServerTypeChanged  ( IConVar *pConVar, const char *pOldString, float flOldValue );
static void TF2VAnySubConvarChanged( IConVar *pConVar, const char *pOldString, float flOldValue );

// ---------------------------------------------------------------------------
// Enforcement level accessors.
// Defined here as inline so both tf_gamerules.cpp and tf_gamerules_convars.cpp
// see them regardless of which TU's link order comes first.
// All three read tf2v_enforcement, which is defined in tf_gamerules_convars.cpp
// and extern'd in tf_gamerules_convars.h.
// ---------------------------------------------------------------------------
extern ConVar tf2v_enforcement;

static inline bool TF2V_EraManaged()      { return tf2v_enforcement.GetInt() >= 1; }
static inline bool TF2V_WeaponGated()     { return tf2v_enforcement.GetInt() >= 2; }
static inline bool TF2V_MapcycleManaged() { return tf2v_enforcement.GetInt() >= 3; }


#ifdef GAME_DLL

	// =========================================================================
	// TF2V ERA SYSTEM
	// =========================================================================

public:

	// -------------------------------------------------------------------------
	// Era state cache — frozen at round start, read by all gameplay code.
	// Never access sub-convars directly during play; use EraState() instead.
	// -------------------------------------------------------------------------
	struct TF2VEraState_t
	{
		// Damage
		int		nDamageSpreadMode;
		bool	bCritModel;
		bool	bCtfCapCrits;
		bool	bArenaFirstBlood;
		int		nFallSounds;

		// Demoman / GL
		bool	bConsoleDamage;
		bool	bConsoleMagazine;
		bool	bGrenadeContactExplode;
		bool	bGrenadePlayerCollision;
		bool	bNewGrenadeRadius;
		int		nDemoExplosionVariance;
		bool	bStickyDamageRampup;
		bool	bStickyRadiusRampup;
		bool	bStickyBulletBreak;
		int		nAmmoEra;

		// Soldier
		bool	bNewBlackBox;

		// Pyro
		int		nAirblast;
		bool	bAirblastPlayers;
		int		nFlameMode;
		bool	bMinicritsOnDeflect;
		bool	bExtinguishHeal;
		bool	bExtinguishCooldown;
		int		nFlareMode;
		bool	bNewFlareRadius;
		int		nPhlogFill;
		int		nPhlogTaunt;
		int		nAxtinguisher;

		// Heavy
		int		nMinigunMode;
		int		nSandvichBehavior;

		// Engineer
		int		nBuildingUpgrades;
		bool	bBuildingHauling;
		bool	bNewHaulingSpeed;
		bool	bNewWrenchMechanics;
		bool	bNewSapperDamage;
		bool	bNewSapperDisable;
		bool	bNewSentryMinigunResist;
		bool	bNewSentryWrangleLocation;
		bool	bNewSentryDamageFalloff;
		bool	bNewTeleporterCost;
		bool	bNewShortCircuit;
		bool	bNewMinibuildings;

		// Medic
		bool	bNewMedicRegen;
		bool	bMedigunHealRate;
		int		nSetupUberRate;
		bool	bUberJugglePenalty;
		bool	bNewUberTaunt;
		bool	bMedicSpeedMatch;
		bool	bNewHealthRegenAttrib;

		// Scout
		int		nSandmanStunType;
		bool	bNewBonkLength;
		bool	bNewSodapopperHype;
		bool	bNewSodapopperFill;
		bool	bManualSodapopper;
		bool	bShortstopShove;
		bool	bShortstopSlowdown;
		bool	bNewGuillotine;
		bool	bNewBallRegen;

		// Soldier (banner/equalizer)
		bool	bNewBuffCharges;
		bool	bSentryResistBonus;
		bool	bNewEqualizerDamage;
		bool	bNewSplitEqualizer;
		bool	bNewSpeedBuffDuration;
		bool	bNewBeggars;

		// Demoman (sword/caber)
		bool	bDemoChargeDebuffRemove;
		bool	bNewCaber;
		bool	bNewHonorbound;

		// Spy
		bool	bSpyCloakReload;
		bool	bSpyCloakAmmoRecharge;
		bool	bNewCloak;
		int		nFeignDeathActivate;
		bool	bFeignDeathStealth;
		bool	bNewYer;
		bool	bNewBigEarner;
		bool	bFastRedisguise;
		bool	bAllowDisguiseWeapons;
		bool	bDisguiseSpyTeleport;
		bool	bDisguiseSpeedMatch;
		bool	bNewSpyMovespeeds;
		bool	bSpyBaseSpeed;
		int		nAmbassadorMode;
		bool	bNewDiamondback;
		bool	bNewPomson;

		// Sniper
		bool	bAllowSniperCrosshairs;
		bool	bNewCleaners;
		int		nBisonDamage;
		bool	bNewBisonSpeed;

		// Cross-class
		bool	bNewAutofire;
		bool	bNewWeaponSwapSpeed;
		bool	bFastWeaponSwitch;			// era 140: draw time 0.67->0.50s
		bool	bReloadCancelAvailable;
		bool	bFasterReloadDefault;
		bool	bNewChocolate;
		bool	bNewAtomizer;
		int		nBackstabMode;
		bool	bNewJag;

		// Movement
		bool	bRadiusDamageTeammates;
		int		nSpeedCap;
		int		nClassDeathAnimations;
		bool	bMinicritselfInflicted;
		bool	bDisableUpdraft;
		bool	bPreventVoiceSpam;
		bool	bClampAirducks;				// era 51: air ducking formalised
		bool	bPistolFixedFirerate;		// era 70: fixed fire rate
		int		nSniperZoomMode;			// 0=none, 1=rezoom lock, 2=+crit delay

		// Soldier
		bool	bSoldierSelfDamageReduction; // era 0-19: 40% self-damage reduction
		bool	bGunboatsNerf;				// era 90: 75%->60% reduction
		bool	bRocketJumperHealthPenalty;	// era 91: -25 max HP

		// Pyro (additional)
		bool	bBackburnerDamageBonus;		// era 82: +20% damage
		bool	bBackburnerAirblast;		// era 104: airblast enabled
		bool	bAirblastMinicrits;			// era 20-169: airblast launches minicrit
		bool	bAfterburnContactTime;		// era 170: contact-time afterburn model
		bool	bAfterburnHealDebuff;		// era 170: afterburn disrupts healing
		bool	bAirblastStickyPush;		// era 100: 2x grounded sticky push

		// Demoman (additional)
		bool	bTargeOwnExplosion;			// era 60-99: Targe blocks own explosions

		// Scout (additional)
		bool	bFaNDamageBonus;			// era 61: FaN +10% damage
		bool	bDeadRingerFlagCarry;		// era 61: DR activates with flag

		// Spy (additional)
		bool	bQuickFixWeaponRestriction;	// era 110-129: no primary during Uber

		// Heavy (additional)
		bool	bNataschaFixed;				// era 32: corrected slow/damage

		// Medic (additional)
		bool	bUberRangeFalloff;			// era 140: drain falloff over distance

		// Weapon gate
		int		nAllowedWeaponEra;
	};

	inline const TF2VEraState_t &EraState() const	{ return m_EraState; }
	inline bool IsEraStateLocked() const			{ return m_bEraStateLocked; }

	// Primary era management entry points
	void	ApplyEra( int nEra );
	void	LockEraState();
	void	TF2VUpdateQuickPlayCompliance();

	// Manual mode snapshot system
	void	TakeManualEraSnapshot();
	void	RestoreManualEraSnapshot();

	// Era change helpers
	static bool TF2V_IsRoundActive();

private:

	// Era state cache
	TF2VEraState_t	m_EraState;
	bool			m_bEraStateLocked;
	bool			m_bApplyingEra;
	bool			m_bEraDirty;

	// Managed mode pending change
	int				m_nPendingEra;
	bool			m_bHasPendingEra;

	// Manual mode snapshot
	struct TF2VManualSnapshot_t
	{
		const char *pszName;
		char		szValue[64];
	};
	CUtlVector<TF2VManualSnapshot_t>	m_ManualEraSnapshot;
	bool								m_bManualSnapshotTaken;
	bool								m_bManualSnapshotStale;

	// QuickPlay think timer
	float	m_flNextQuickPlayCheck;

	// =========================================================================
	// END TF2V ERA SYSTEM
	// =========================================================================


#endif // GAME_DLL

#endif // TF_GAMERULES_ERA_INTERNAL_H

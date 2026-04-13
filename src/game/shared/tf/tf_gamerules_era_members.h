//=============================================================================
// tf_gamerules_era_members.h
//
// TF2V era system class members for CTFGameRules.
//
// This file is #included directly inside the CTFGameRules class body in
// tf_gamerules.h, inside a #ifdef GAME_DLL block.  It is NOT a standalone
// header and must never be included anywhere else.
//
// ERA SYSTEM — DAY-BASED EPOCH
// ============================================================
// The era integer is "days since September 17 2007" (Day 1 = beta launch).
// Two public convars exist:
//   tf2v_era              — visible, drives everything via ApplyEra()
//   tf2v_use_era_mapcycle — visible, 1 = auto-set mapcyclefile from era table
//
// All sub-balance convars are hidden (FCVAR_HIDDEN) and managed exclusively
// by ApplyEra() / LockEraState().  There is no "manual mode" or enforcement
// level selector; era management is always active.
//
// Declares:
//   - TF2VEraState_t (frozen snapshot struct read by all gameplay code)
//   - Public accessor and entry-point method declarations
//   - Private era state data members
//
// Implementations live in tf_gamerules_era_internal.cpp and
// tf_gamerules_applyera.cpp.
//=============================================================================

// No include guard — this is a class body fragment.

public:

    // -------------------------------------------------------------------------
    // Era state cache — frozen at round start, read by all gameplay code.
    // Use TFGameRules()->EraState().fieldName; never read sub-convars directly.
    // -------------------------------------------------------------------------
    struct TF2VEraState_t
    {
        // Current era (day integer) and weapon gate ceiling (always equal).
        int     nCurrentEra;
        int     nAllowedWeaponEra;

        // Damage
        int     nDamageSpreadMode;
        bool    bCritModel;
        bool    bCtfCapCrits;
        bool    bArenaFirstBlood;
        int     nFallSounds;

        // Demoman / GL
        bool    bConsoleDamage;
        bool    bConsoleMagazine;
        bool    bGrenadeContactExplode;
        bool    bGrenadePlayerCollision;
        bool    bNewGrenadeRadius;
        int     nDemoExplosionVariance;
        bool    bStickyDamageRampup;
        bool    bStickyRadiusRampup;
        bool    bStickyBulletBreak;
        int     nAmmoEra;

        // Soldier
        bool    bNewBlackBox;

        // Pyro
        int     nAirblast;
        bool    bAirblastPlayers;
        int     nFlameMode;
        bool    bMinicritsOnDeflect;
        bool    bExtinguishHeal;
        bool    bExtinguishCooldown;
        int     nFlareMode;
        bool    bNewFlareRadius;
        int     nPhlogFill;
        int     nPhlogTaunt;
        int     nAxtinguisher;

        // Heavy
        int     nMinigunMode;
        int     nSandvichBehavior;

        // Engineer
        int     nBuildingUpgrades;
        bool    bBuildingHauling;
        bool    bNewHaulingSpeed;
        bool    bNewWrenchMechanics;
        bool    bNewSapperDamage;
        bool    bNewSapperDisable;
        bool    bNewSentryMinigunResist;
        bool    bNewSentryWrangleLocation;
        bool    bNewSentryDamageFalloff;
        bool    bNewTeleporterCost;
        bool    bNewShortCircuit;
        bool    bNewMinibuildings;

        // Medic
        bool    bNewMedicRegen;
        bool    bMedigunHealRate;
        int     nSetupUberRate;
        bool    bUberJugglePenalty;
        bool    bNewUberTaunt;
        bool    bMedicSpeedMatch;
        bool    bNewHealthRegenAttrib;

        // Scout
        int     nSandmanStunType;
        bool    bNewBonkLength;
        bool    bNewSodapopperHype;
        bool    bNewSodapopperFill;
        bool    bManualSodapopper;
        bool    bShortstopShove;
        bool    bShortstopSlowdown;
        bool    bNewGuillotine;
        bool    bNewBallRegen;

        // Soldier (banner/equalizer)
        bool    bNewBuffCharges;
        bool    bSentryResistBonus;
        bool    bNewEqualizerDamage;
        bool    bNewSplitEqualizer;
        bool    bNewSpeedBuffDuration;
        bool    bNewBeggars;

        // Demoman (sword/caber)
        bool    bDemoChargeDebuffRemove;
        bool    bNewCaber;
        bool    bNewHonorbound;

        // Spy
        bool    bSpyCloakReload;
        bool    bSpyCloakAmmoRecharge;
        bool    bNewCloak;
        int     nFeignDeathActivate;
        bool    bFeignDeathStealth;
        bool    bNewYer;
        bool    bNewBigEarner;
        bool    bFastRedisguise;
        bool    bAllowDisguiseWeapons;
        bool    bDisguiseSpyTeleport;
        bool    bDisguiseSpeedMatch;
        bool    bNewSpyMovespeeds;
        bool    bSpyBaseSpeed;
        int     nAmbassadorMode;
        bool    bNewDiamondback;
        bool    bNewPomson;

        // Sniper
        int     nSniperZoomMode;
        bool    bAllowSniperCrosshairs;
        bool    bNewCleaners;
        int     nBisonDamage;
        bool    bNewBisonSpeed;

        // Cross-class
        bool    bNewAutofire;
        bool    bNewWeaponSwapSpeed;
        bool    bFastWeaponSwitch;
        bool    bReloadCancelAvailable;
        bool    bFasterReloadDefault;
        bool    bNewChocolate;
        bool    bNewAtomizer;
        int     nBackstabMode;
        bool    bNewJag;

        // Movement
        bool    bRadiusDamageTeammates;
        int     nSpeedCap;
        int     nClassDeathAnimations;
        bool    bMinicritselfInflicted;
        bool    bClampAirducks;
        bool    bPistolFixedFirerate;

        // Soldier (self-damage / jumpers)
        bool    bSoldierSelfDamageReduction;
        bool    bGunboatsNerf;
        bool    bRocketJumperHealthPenalty;

        // Pyro (additional)
        bool    bBackburnerDamageBonus;
        bool    bBackburnerAirblast;
        bool    bAirblastMinicrits;
        bool    bAfterburnContactTime;
        bool    bAfterburnHealDebuff;
        bool    bAirblastStickyPush;
        bool    bDisableUpdraft;
        bool    bPreventVoiceSpam;

        // Demoman (additional)
        bool    bTargeOwnExplosion;

        // Scout (additional)
        bool    bFaNDamageBonus;
        bool    bDeadRingerFlagCarry;

        // Medic (additional)
        bool    bQuickFixWeaponRestriction;
        bool    bUberRangeFalloff;

        // Heavy (additional)
        bool    bNataschaFixed;
    };

    // Read-only accessor for gameplay code.
    inline const TF2VEraState_t &EraState() const
    {
        AssertMsg( m_bEraStateLocked,
            "EraState() read before LockEraState() — era values may be stale." );
        return m_EraState;
    }
    inline bool IsEraStateLocked() const { return m_bEraStateLocked; }

    // Primary era management entry points.
    // Called by era callbacks and RoundRespawn().
    void    ApplyEra( int nDay );
    void    LockEraState();
    void    TF2VUpdateQuickPlayCompliance();

    // Returns true when a round is actively in progress.
    static bool TF2V_IsRoundActive();

    // Friend declarations for static callbacks in tf_gamerules_era_internal.cpp
    // that need access to private era members.
    friend void TF2VEraChanged          ( IConVar*, const char*, float );
    friend void TF2VMapcycleToggleChanged( IConVar*, const char*, float );
    friend void TF2VServerTypeChanged   ( IConVar*, const char*, float );
    friend void TF2VAnySubConvarChanged ( IConVar*, const char*, float );

private:

    // Frozen era snapshot — populated by LockEraState() at round start.
    TF2VEraState_t  m_EraState;
    bool            m_bEraStateLocked;

    // Guards against callback re-entrancy while ApplyEra() is writing convars.
    bool            m_bApplyingEra;

    // Set when a sub-convar changes outside a safe window.
    bool            m_bEraDirty;

    // Pending era change deferred from a live round.
    int             m_nPendingEra;
    bool            m_bHasPendingEra;

    // Throttle timer for QuickPlay compliance re-evaluation.
    float           m_flNextQuickPlayCheck;

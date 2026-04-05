//=============================================================================
// tf_gamerules_era_members.h
//
// TF2V era system class members for CTFGameRules.
//
// This file is #included directly inside the CTFGameRules class body in
// tf_gamerules.h, inside a #ifdef GAME_DLL block. It is NOT a standalone
// header and must never be included anywhere else.
//
// It declares:
//   - TF2VEraState_t (the frozen snapshot struct read by all gameplay code)
//   - Public accessor and entry-point method declarations
//   - Private era state data members
//
// Implementations live in tf_gamerules_era_internal.cpp and
// tf_gamerules_applyera.cpp.
//=============================================================================

// This file has no include guard intentionally — it is a class body fragment,
// not a standalone translation unit. The surrounding #ifdef GAME_DLL in
// tf_gamerules.h protects it.

public:

	// -------------------------------------------------------------------------
	// Era state cache — frozen at round start, read by all gameplay code.
	// Never access sub-convars directly during a live round.
	// Use TFGameRules()->EraState().fieldName instead.
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
		int		nSniperZoomMode;		// 0=none, 1=rezoom lock, 2=+crit delay
		bool	bAllowSniperCrosshairs;
		bool	bNewCleaners;
		int		nBisonDamage;
		bool	bNewBisonSpeed;

		// Cross-class
		bool	bNewAutofire;
		bool	bNewWeaponSwapSpeed;
		bool	bFastWeaponSwitch;		// era 150: draw time 0.67->0.50s
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
		bool	bClampAirducks;			// era 51: air ducking formalised
		bool	bPistolFixedFirerate;	// era 70: fixed fire rate

		// Soldier (self-damage / jumpers)
		bool	bSoldierSelfDamageReduction;	// era 0-19: 40% self-damage reduction
		bool	bGunboatsNerf;					// era 90: 75%->60% reduction
		bool	bRocketJumperHealthPenalty;		// era 91: -25 max HP

		// Pyro (additional)
		bool	bBackburnerDamageBonus;		// era 82: +20% damage
		bool	bBackburnerAirblast;		// era 104: airblast enabled
		bool	bAirblastMinicrits;			// era 20-169: airblast launches minicrit
		bool	bAfterburnContactTime;		// era 170: contact-time afterburn model
		bool	bAfterburnHealDebuff;		// era 170: afterburn disrupts healing
		bool	bAirblastStickyPush;		// era 100: 2x grounded sticky push
		bool	bDisableUpdraft;			// era 150: B.A.S.E.Jumper updraft removed
		bool	bPreventVoiceSpam;			// era 170: JI voice spam prevention

		// Demoman (additional)
		bool	bTargeOwnExplosion;			// era 60-99: Targe blocks own explosions

		// Scout (additional)
		bool	bFaNDamageBonus;			// era 61: FaN +10% damage
		bool	bDeadRingerFlagCarry;		// era 61: DR activates with flag

		// Medic (additional)
		bool	bQuickFixWeaponRestriction;	// era 110-129: no primary during Uber
		bool	bUberRangeFalloff;			// era 140: drain falloff over distance

		// Heavy (additional)
		bool	bNataschaFixed;				// era 32: corrected slow/damage

		// Weapon gate
		int		nAllowedWeaponEra;
	};

	// Read-only accessor for gameplay code. Asserts locked in debug builds.
	inline const TF2VEraState_t &EraState() const
	{
		AssertMsg( m_bEraStateLocked,
			"EraState() read before LockEraState() — era values may be stale." );
		return m_EraState;
	}
	inline bool IsEraStateLocked() const { return m_bEraStateLocked; }

	// Primary era management entry points.
	// Called by era callbacks and RoundRespawn(). Never call directly.
	void	ApplyEra( int nEra );
	void	LockEraState();
	void	TF2VUpdateQuickPlayCompliance();

	// Manual mode snapshot — preserves server.cfg values across rounds
	// when tf2v_enforcement is 0.
	void	TakeManualEraSnapshot();
	void	RestoreManualEraSnapshot();

	// Returns true when a round is actively in progress (not intermission,
	// not waiting for players). Used to gate mid-round convar changes.
	static bool TF2V_IsRoundActive();

	// Friend declarations — these static callbacks in tf_gamerules_era_internal.cpp
	// need access to private era members to set m_bApplyingEra, m_bEraDirty, etc.
	friend void TF2VEraChanged         ( IConVar*, const char*, float );
	friend void TF2VEnforcementChanged ( IConVar*, const char*, float );
	friend void TF2VMapcycleModeChanged( IConVar*, const char*, float );
	friend void TF2VServerTypeChanged  ( IConVar*, const char*, float );
	friend void TF2VAnySubConvarChanged( IConVar*, const char*, float );

private:

	// Frozen era snapshot — populated by LockEraState() at round start.
	TF2VEraState_t	m_EraState;
	bool			m_bEraStateLocked;

	// Guards against callback re-entrancy while ApplyEra() is writing convars.
	bool			m_bApplyingEra;

	// Set when a sub-convar changes outside a safe window — triggers
	// a re-apply at the next round boundary.
	bool			m_bEraDirty;

	// Pending era change deferred from a live round.
	// Applied at the next RoundRespawn() call.
	int				m_nPendingEra;
	bool			m_bHasPendingEra;

	// Manual mode snapshot storage. Each entry is one convar name + value.
	struct TF2VManualSnapshot_t
	{
		const char *pszName;
		char		szValue[64];
	};
	CUtlVector<TF2VManualSnapshot_t>	m_ManualEraSnapshot;
	bool								m_bManualSnapshotTaken;
	bool								m_bManualSnapshotStale;

	// Throttle timer for QuickPlay compliance re-evaluation.
	float	m_flNextQuickPlayCheck;

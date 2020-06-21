//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#ifdef GAME_DLL
#include "SpriteTrail.h"
#else
#include "c_te_legacytempents.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
class C_TFPlayer;
// Server specific.
#else
class CTFPlayer;
#endif

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif


//=============================================================================

#define PERMANENT_CONDITION		-1

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
	DECLARE_EMBEDDED_NETWORKVAR()
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();
#endif

public:
	float flDamage;
	float flTime;
	bool bKill;
};

enum
{
	STUN_PHASE_NONE,
	STUN_PHASE_LOOP,
	STUN_PHASE_END,
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

	// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

	// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );
	void 	ResetMeters( void );
	
	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	// Condition (TF_COND_*).
	int		GetCond() const						{ return m_nPlayerCond; }
	void	SetCond( int nCond )				{ m_nPlayerCond = nCond; }
	void	AddCond( int nCond, float flDuration = PERMANENT_CONDITION );
	void	RemoveCond( int nCond );
	bool	InCond( int nCond );
	void	RemoveAllCond( CTFPlayer *pPlayer );
	void	OnConditionAdded( int nCond );
	void	OnConditionRemoved( int nCond );
	void	ConditionThink( void );
	float	GetConditionDuration( int nCond );

	bool	IsCritBoosted( void );
	bool	IsMiniCritBoosted( void );
	bool	IsSpeedBoosted( void );
	bool	IsInvulnerable( void );
	bool	IsStealthed( void );

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	int		GetMaxBuffedHealth(void);

	// Max Health
	int		GetMaxHealth( void );
	void	SetMaxHealth( int iMaxHealth )		{ m_iMaxHealth = iMaxHealth; }

	// Sanguisuge
	void	ChangeSanguisugeHealth(int value)		{ m_iLeechHealth += value; }
	void	SetSanguisugeHealth( int iLeechHealth )		{ m_iLeechHealth = iLeechHealth; }
	int		GetSanguisugeHealth( void )					{ return m_iLeechHealth; }
	void	SetNextSanguisugeDecay()		{ m_iLeechDecayTime = gpGlobals->curtime + 0.5; }

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	SyncConditions( int nCond, int nOldCond, int nUnused, int iOffset );

	void	ClientDemoBuffThink( void );
	void	ClientShieldChargeThink( void );
#endif

	void	Disguise( int nTeam, int nClass, CTFPlayer *pTarget = nullptr, bool b1 = true );
	void	CompleteDisguise( void );
	void	RemoveDisguise( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void )				{ return m_nDisguiseTeam; }
	int		GetDisguiseClass( void ) 			{ return m_nDisguiseClass; }
	int		GetDesiredDisguiseClass( void )		{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void )		{ return m_nDesiredDisguiseTeam; }
	float	GetDisguiseChargeLevel( void )      { return m_flDisguiseChargeLevel; }
	int		GetMaskClass( void )				{ return m_nMaskClass; }
	EHANDLE GetDisguiseTarget( void )
	{
#ifdef CLIENT_DLL
		if ( m_iDisguiseTargetIndex == TF_DISGUISE_TARGET_INDEX_NONE )
			return NULL;
		return cl_entitylist->GetNetworkableHandle( m_iDisguiseTargetIndex );
#else
		return m_hDisguiseTarget.Get();
#endif
	}
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );
	int		AddDisguiseHealth( int iHealthToAdd, bool bOverheal = false, float flOverhealAmount = 1.0f );
	int		GetDisguiseMaxHealth( void );
	int		GetDisguiseMaxBuffedHealth( void );

	CEconItemView *GetDisguiseItem( void )				{ return &m_DisguiseItem; }
	void	RecalcDisguiseWeapon( int iSlot = 0 );
	void	CalculateDisguiseWearables(void);
	
	// Wearable bodygroups.
	int		GetWearableBodygroups(void)							{return m_iWearableBodygroups; }
	void	SetWearableBodygroups(int input)					{ m_iWearableBodygroups = input; }
	int		m_iWearableBodygroups;
	
	int		GetFullDisguiseWearables(void)				{ return GetDisguiseBodygroups() + GetWeaponDisguiseBodygroups(); }
	
	// Wearable disguises.
	int		GetDisguiseBodygroups(void)						{return m_iDisguiseBodygroups;}
	void	SetDisguiseBodygroups(int input)				{m_iDisguiseBodygroups = input;}
	int		m_iDisguiseBodygroups;
	
	// Weapon disguise bodygroups.
	int		GetWeaponDisguiseBodygroups(void)						{ return m_iWeaponBodygroup; }
	void	SetWeaponDisguiseBodygroups(int input)					{ m_iWeaponBodygroup = input; }
	int		m_iWeaponBodygroup;

#ifdef CLIENT_DLL
	void	OnDisguiseChanged( void );
	int		GetDisguiseWeaponModelIndex( void ) { return m_iDisguiseWeaponModelIndex; }
	void	RecalcDisguiseWeaponLegacy( void );
	CTFWeaponInfo *GetDisguiseWeaponInfo( void );

	void	UpdateCritBoostEffect( bool bForceHide = false );
#endif

#ifdef GAME_DLL
	void	Heal( CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal = false );
	void	StopHealing( CTFPlayer *pPlayer );
	void	RecalculateChargeEffects( bool bInstantRemove = false );
	EHANDLE GetHealerByIndex( int index );
	int		FindHealerIndex( CTFPlayer *pPlayer );
	EHANDLE	GetFirstHealer();
	void	HealthKitPickupEffects( int iAmount );
	bool	HealerIsDispenser( int index ) const;

	// Jarate Player
	EHANDLE	m_hUrineAttacker;

	// Milk Player
	EHANDLE	m_hMilkAttacker;

	// Gas Player
	EHANDLE m_hGasAttacker;
#endif
	int		GetNumHealers( void )				{ return m_nNumHealers; }

	void	Burn( CBaseCombatCharacter *pAttacker, float flFlameDuration = -1.0f );
	void	Burn( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon = NULL, float flFlameDuration = -1.0f );
	void	StunPlayer( float flDuration, float flSpeed, float flResistance, int nStunFlags, CTFPlayer *pStunner );
	void	MakeBleed( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon, float flBleedDuration, int iDamage );

	bool	IsControlStunned( void );

#ifdef GAME_DLL
	void	AddPhaseEffects( void );
	CUtlVector< CSpriteTrail * > m_pPhaseTrails;
#else
	CNewParticleEffect *m_pWarp;
	CNewParticleEffect *m_pStun;
	CNewParticleEffect *m_pSpeedTrails;
	CNewParticleEffect *m_pBuffAura;
	CNewParticleEffect *m_pMarkedIcon;
	
	CNewParticleEffect *m_pResistanceIcon;
	C_BaseAnimating	   *m_pResistanceShield;
	int				m_nCurrentResistanceIcon;
	int				m_nResistanceIconTeam;
	int				m_nResistanceShieldTeam;
#endif

	void	UpdatePhaseEffects( void );
	void	UpdateSpeedBoostEffects( void );

	void	RecalculatePlayerBodygroups( void );

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;

	// Utility.
	bool	IsAlly( CBaseEntity *pEntity );

	bool	IsLoser( void );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float flInvisFadeTime );
	float	GetPercentInvisible( void );
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void )	{ return m_flLastStealthExposeTime; }
	void	SetHasMotionCloak( bool bSet )		{ m_bHasMotionCloak = bSet; }
	void	SetCloakDrainRate( float flRate )	{ m_flCloakDrainRate = flRate; }
	void	SetCloakRegenRate( float flRate )	{ m_flCloakRegenRate = flRate; }

	void	SetSpySprint(bool bSet)		{ m_bSpySprint = bSet; }
	float	GetSpySprint(void)			{ return m_bSpySprint; }

	int		GetDesiredPlayerClassIndex( void );

	int		GetDesiredWeaponIndex( void )		{ return m_iDesiredWeaponID; }
	void	SetDesiredWeaponIndex( int iWeaponID ) { m_iDesiredWeaponID = iWeaponID; }
	int		GetRespawnParticleID( void )		{ return m_iRespawnParticleID; }
	void	SetRespawnParticleID( int iParticleID ) { m_iRespawnParticleID = iParticleID; }

	bool	AddToSpyCloakMeter( float amt, bool bForce = false, bool bIgnoreAttribs = false );
	void	RemoveFromSpyCloakMeter( float amt, bool bDrainSound = false );
	float	GetSpyCloakMeter() const			{ return m_flCloakMeter; }
	void	SetSpyCloakMeter( float val )		{ m_flCloakMeter = val; }

	void	SetFeignReady( bool bSet )			{ m_bFeignDeathReady = bSet; }
	bool	IsFeignDeathReady( void )			{ return m_bFeignDeathReady; }

	bool	IsFeigningDeath( void ) const		{ return m_bFeigningDeath; }

	bool	IsJumping( void )					{ return m_bJumping; }
	void	SetJumping( bool bJumping );
	bool	CanGoombaStomp( void );
	bool	HasParachute( void );
	bool	CanParachute(void);	
	void	DeployParachute(void);
	bool	IsParachuting(void);
	void    ResetParachute(void);

	bool	CanAirDash( void );
	int		GetAirDashCount( void )				{ return m_nAirDashCount; }
	void    IncrementAirDashCount( void )		{ m_nAirDashCount += 1; }
	void    ResetAirDashCount( void )			{ m_nAirDashCount = 0; }
	float	GetLastDashTime(void)				{ return m_flLastDashTime; }
	void	SetLastDashTime( float flLastDash );

	int		GetAirDucks( void )					{ return m_nAirDucked; }
	void	IncrementAirDucks( void );
	void	ResetAirDucks( void );

	void	DebugPrintConditions( void );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

#ifndef CLIENT_DLL
	int     GetDominationCount( void );
#endif

	bool	IsCarryingObject( void )			{ return m_bCarryingObject; }

#ifdef GAME_DLL
	void			SetCarriedObject( CBaseObject *pObj );
	CBaseObject*	GetCarriedObject( void );
#endif

	int		GetKillstreak( int weaponSlot )						{ return m_nStreaks.Get( weaponSlot ); }
	void	SetKillstreak( int weaponSlot, int iStreak )	{ m_nStreaks.Set( weaponSlot, iStreak ); }
	void	IncKillstreak( int weaponSlot );

	int		GetStunPhase( void )				{ return m_iStunPhase; }
	void	SetStunPhase( int iPhase )			{ m_iStunPhase = iPhase; }
	float	GetStunExpireTime( void )			{ return m_flStunExpireTime; }
	void	SetStunExpireTime( float flTime )	{ m_flStunExpireTime = flTime; }
	int		GetStunFlags( void )				{ return m_nStunFlags; }

	int		GetTeleporterEffectColor( void )	{ return m_nTeamTeleporterUsed; }
	void	SetTeleporterEffectColor( int iTeam ) { m_nTeamTeleporterUsed = iTeam; }
#ifdef CLIENT_DLL
	bool	ShouldShowRecentlyTeleported( void );
#endif

	int		GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom );

#ifdef GAME_DLL
	void	AddAttributeToPlayer( char const *szName, float flValue );
	void	RemoveAttributeFromPlayer( char const *szName );
#endif

	// Banners
	void	UpdateRageBuffsAndRage( void );
	void	SetRageMeter( float flRagePercent, int iBuffType );
	void	ActivateRageBuff( CBaseEntity *pEntity, int iBuffType );
	void	PulseRageBuff( /*CTFPlayerShared::ERageBuffSlot*/ );
	void	SetRageActive( bool bSet )          { m_bRageActive = bSet; }
	bool	IsRageActive( void )				{ return m_bRageActive; }
	float	GetRageProgress( void )				{ return m_flEffectBarProgress; }
	void	ResetRageSystem( void );

	// Scatterguns
	bool	HasRecoiled( void ) const			{ return m_bRecoiled; }
	void	SetHasRecoiled( bool value )		{ m_bRecoiled = value; }
	float	GetHypeMeter( void ) const			{ return m_flHypeMeter; }
	void	AddHypeMeter( float value );
	void	RemoveHypeMeter( float value );
	void	SetHypeMeterAbsolute( float value )	{ m_flHypeMeter = value; }
	
	int		GetKnockbackWeaponID( void ) const  { return m_iWeaponKnockbackID; }
	void	SetKnockbackWeaponID( int userid )  { m_iWeaponKnockbackID = userid; }
	CBasePlayer *GetKnockbackWeaponOwner( void );

	// Knights
	void	IncrementDecapitationCount( void )       { m_iDecapitations += 1; }
	int		GetDecapitationCount( void ) const       { return m_iDecapitations; }
	void	SetDecapitationCount( int count )        { m_iDecapitations = count; }
	bool	HasDemoShieldEquipped( void ) const;
	void	SetDemoShieldEquipped( bool bEquipped )  { m_bShieldEquipped = bEquipped; }
	int		GetNextMeleeCrit( void ) const           { return m_iNextMeleeCrit; }
	void	SetNextMeleeCrit( int iType )            { m_iNextMeleeCrit = iType; }
	float	GetShieldChargeMeter( void ) const       { return m_flChargeMeter; }
	void	SetShieldChargeMeter( float flVal )      { m_flChargeMeter = flVal; }
	void	SetShieldChargeDrainRate( float flRate ) { m_flChargeDrainRate = flRate; }
	void	SetShieldChargeRegenRate( float flRate ) { m_flChargeRegenRate = flRate; }
	void	CalcChargeCrit( bool bForceFull );
	
	// Sniper rifle headshots (ie: Bazaar Bargin)
	int		GetHeadshotCount( void ) const       { return m_iHeadshots; }
	void	SetHeadshotCount( int count )        { m_iHeadshots = count; }
	void	IncrementHeadshotCount( void )       { m_iHeadshots += 1; }
	
	// Killstreak for attribute items (ie: Air Strike)
	int		GetStrikeCount( void ) const       { return m_iStrike; }
	void	SetStrikeCount( int count )        { m_iStrike = count; }
	void	IncrementStrikeCount( void )       { m_iStrike += 1; }

	// Sapper/Backstab content (ie: Diamondback)
	int		GetSapperKillCount(void) const       { return m_iSapperKill; }
	void	SetSapperKillCount(int count)        { m_iSapperKill = count; }
	void	IncrementSapperKillCount(void)       { m_iSapperKill += 1; } // Not affected by TF_WEAPON_MAX_REVENGE
	void	StoreSapperKillCount(void)			 { m_iSapperKill = Min( (m_iSapperKill + 1), TF_WEAPON_MAX_REVENGE ); } // Affected by TF_WEAPON_MAX_REVENGE
	void	DeductSapperKillCount(void)			 { m_iSapperKill = Max( (m_iSapperKill - 1), 0 ); } // Affected by TF_WEAPON_MAX_REVENGE
	
	// Revenge Crit Counter (ie: Frontier Justice)
	int		GetRevengeCritCount( void ) const        { return m_iRevengeCrits; }
	void	SetRevengeCritCount(int count)      	 { m_iRevengeCrits = count; }
	void	IncrementRevengeCrit( void )     		 { m_iRevengeCrits += 1; }	
	void	StoreRevengeCrit(void)					 { m_iRevengeCrits = Min( (m_iRevengeCrits + 1), TF_WEAPON_MAX_REVENGE ); } // Affected by TF_WEAPON_MAX_REVENGE
	void	DeductRevengeCrit( void )     	 		 { m_iRevengeCrits = Max( (m_iRevengeCrits - 1), 0 ); }
	bool	HasRevengeCrits( void )      			 { return m_iRevengeCrits > 0; }
	
	// Airblast Crit Counter (ie: Manmelter)
	int		GetAirblastCritCount( void ) const        { return m_iAirblastCrits; }
	void	SetAirblastCritCount(int count)      	 { m_iAirblastCrits = count; }
	void	IncrementAirblastCrit( void )     		 { m_iAirblastCrits += 1; }	
	void	StoreAirblastCrit(void)					 { m_iAirblastCrits = Min( (m_iAirblastCrits + 1), TF_WEAPON_MAX_REVENGE ); } // Affected by TF_WEAPON_MAX_REVENGE
	void	DeductAirblastCrit( void )     	 		 { m_iAirblastCrits = Max( (m_iAirblastCrits - 1), 0 ); }
	bool	HasAirblastCrits( void )      			 { return m_iAirblastCrits > 0; }
	
	
	// Killstreak counter, for HUD.
	int		GetKillstreakCount( void ) const       { return m_iKillstreak; }
	void	SetKillstreakCount( int count )        { m_iKillstreak = count; }
	void	IncrementKillstreakCount( void )       { m_iKillstreak += 1; }
	
	void	SetFocusLevel(float amount)        { m_flFocusLevel = amount; }
	
#ifdef GAME_DLL
	void	UpdateCloakMeter( void );
	void 	UpdateSanguisugeHealth( void );
	void	UpdateChargeMeter( void );
	void	UpdateEnergyDrinkMeter( void );
	
	// Focus.
	void	UpdateFocusLevel( void );
	void	AddFocusLevel(bool bKillOrAssist);
#endif
	bool	HasFocusCharge(void)	{return m_flFocusLevel > 0;}
	float	GetFocusLevel(void)		{return m_flFocusLevel;}
	
	void	EndCharge( void );
	
#ifdef GAME_DLL
	// Mmmph.
	void	UpdateFireRage( void );
	void	AddFireRage(float input) { m_flFireRage = Min( (m_flFireRage + input), 100.0f ); }
	
	// CRIKEY.
	void	UpdateCrikeyMeter( void );
	void	AddCrikeyMeter(float input) { m_flCrikeyMeter = Min( (m_flCrikeyMeter + input), 100.0f ); }
#endif
	void	SetFireRageMeter( float value ) {m_flFireRage = value;}
	bool	HasFullFireRage(void)	{return m_flFireRage >= 100.0f;}
	float	GetFireRage(void)		{return m_flFireRage;}

	void	SetCrikeyMeter( float value ) {m_flCrikeyMeter = value;}
	bool	HasFullCrikeyMeter(void)	{return m_flCrikeyMeter >= 100.0f;}
	float	GetCrikeyMeter(void)		{return m_flCrikeyMeter;}
private:

	void OnAddStealthed( void );
	void OnAddFeignDeath( void );
	void OnAddInvulnerable( void );
	void OnAddMegaheal( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddTaunting( void );
	void OnAddStunned( void );
	void OnAddSlowed( void );
	void OnAddShieldCharge( void );
	void OnAddRegenerate( void );
	void OnAddCritboosted( void );
	void OnAddHalloweenGiant( void );
	void OnAddHalloweenTiny( void );
	void OnAddUrine( void );
	void OnAddMadMilk(void);
	void OnAddGas(void);
	void OnAddPhase( void );
	void OnAddSpeedBoost( void );
	void OnAddBuff( void );
	void OnAddInPurgatory( void );
	void OnAddMarkedForDeath( void );
	void UpdateResistanceIcon(void);
	void UpdateResistanceShield(void);
	void OnAddHalloweenThriller( void );
	void OnAddHalloweenBombHead( void );

	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveMegaheal( void );
	void OnRemoveTeleported( void );
	void OnRemoveTaunting( void );
	void OnRemoveStunned( void );
	void OnRemoveSlowed( void );
	void OnRemoveShieldCharge( void );
	void OnRemoveRegenerate( void );
	void OnRemoveCritboosted( void );
	void OnRemoveHalloweenGiant( void );
	void OnRemoveHalloweenTiny( void );
	void OnRemoveUrine( void );
	void OnRemoveMadMilk( void );
	void OnRemoveGas( void );
	void OnRemovePhase( void );
	void OnRemoveSpeedBoost( void );
	void OnRemoveBuff( void );
	void OnRemoveInPurgatory( void );
	void OnRemoveMarkedForDeath( void );
	void OnRemoveHalloweenThriller( void );
	void OnRemoveHalloweenBombHead( void );

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	medigun_charge_types  GetChargeEffectBeingProvided( CTFPlayer *pPlayer );
	void  SetChargeEffect( medigun_charge_types chargeType, bool bShouldCharge, bool bInstantRemove, const MedigunEffects_t &chargeEffect, float flRemoveTime, CTFPlayer *pProvider );
	void  TestAndExpireChargeEffect( medigun_charge_types chargeType );
	
	// Resistances.
	int		GetPassiveChargeEffect( CTFPlayer *pPlayer );
	void	SetPassiveResist(int nResistanceType, bool bShouldResist, CTFPlayer *pProvider);
	
#endif

private:

	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	// Ugh...
	CNetworkVar( int, m_nPlayerCondEx ); // 33-64
	CNetworkVar( int, m_nPlayerCondEx2 ); // 65-96
	CNetworkVar( int, m_nPlayerCondEx3 ); // 97-128
	CNetworkVar( int, m_nPlayerCondEx4 ); // 129-160
	CNetworkArray( float, m_flCondExpireTimeLeft, TF_COND_LAST ); // Time until each condition expires

	//TFTODO: What if the player we're disguised as leaves the server?
	//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	CNetworkVar( int, m_nMaskClass );             // Fake disguise class.F
	EHANDLE m_hDisguiseTarget;					// Playing the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseTargetIndex );
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_iDisguiseMaxHealth );
	CNetworkVar( float, m_flDisguiseChargeLevel );
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );
	CEconItemView m_DisguiseItem;
	EHANDLE m_hForcedDisguise;

	CNetworkVar( int, m_iMaxHealth );
	CNetworkVar(int, m_iLeechHealth);
	float m_iLeechDecayTime;

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player seperate from teammates

	float m_flInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;

	CNetworkVar( int, m_nNumHealers );

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

	bool m_bRageActive;

	bool m_bRecoiled;			// Recoil in midair from scattergun
	
	bool m_bSpySprint;			// Allow spies to override disguise speed.

#ifdef GAME_DLL
	// Healer handling
	struct healers_t
	{
		EHANDLE	pPlayer;
		float	flAmount;
		bool	bDispenserHeal;
	};
	CUtlVector< healers_t >	m_aHealers;
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing

	float		m_flInvulnerableOffTime;
	float		m_flChargeOffTime[TF_CHARGE_COUNT];
	bool		m_bChargeSounds[TF_CHARGE_COUNT];
#endif

	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CHandle<CTFWeaponBase>	m_hBurnWeapon;
	CNetworkVar( int, m_nNumFlames );
	float					m_flFlameBurnTime;
	float					m_flFlameStack;
	float					m_flFlameLife;
	float					m_flFlameRemoveTime;
	float					m_flTauntRemoveTime;
	

#ifdef GAME_DLL
	struct bleed_struct_t
	{
		CHandle<CTFPlayer> m_hAttacker;
		CHandle<CTFWeaponBase> m_hWeapon;
		float m_flBleedTime;
		float m_flEndTime;
		int m_iDamage;
	};
	CUtlVector< bleed_struct_t > m_aBleeds;
#endif

	// Other
	float					m_flStunTime;
	float					m_flPhaseTime;

	// Banner
	CNetworkVar( float, m_flEffectBarProgress );
	float					m_flNextRageCheckTime;
	float					m_flRageTimeRemaining;
	int						m_iActiveBuffType;



	float m_flDisguiseCompleteTime;

	CNetworkVar( int, m_iDesiredPlayerClass );
	CNetworkVar( int, m_iDesiredWeaponID );
	CNetworkVar( int, m_iRespawnParticleID );

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]
	float m_flCloakDrainRate;
	float m_flCloakRegenRate;

	bool m_bHasMotionCloak;

	CNetworkVar( bool, m_bFeignDeathReady );
	bool m_bFeigningDeath;

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( int, m_nAirDashCount );
	CNetworkVar( float, m_flLastDashTime );
	CNetworkVar( int, m_nAirDucked );

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( int, m_nStreaks, 3 );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS+1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS+1 );	// array of state per other player whether other players are dominating this player

	CNetworkHandle( CTFPlayer, m_hStunner );
	CNetworkVar( float, m_flStunExpireTime );
	int m_iStunPhase;

	CNetworkVar( int, m_nStunFlags );
	CNetworkVar( float, m_flStunMovementSpeed );
	CNetworkVar( float, m_flStunResistance );

	// Stored for environmental kill credit
	CNetworkVar( int, m_iWeaponKnockbackID );

	CNetworkVar( int, m_iDecapitations );
	CNetworkVar( bool, m_bShieldEquipped );
	CNetworkVar( int, m_iNextMeleeCrit );
	
	CNetworkVar( int, m_iHeadshots );
	CNetworkVar( int, m_iStrike );
	CNetworkVar( int, m_iKillstreak );
	CNetworkVar( int, m_iSapperKill );
	CNetworkVar( int, m_iRevengeCrits );
	CNetworkVar( int, m_iAirblastCrits );
#ifdef GAME_DLL
public:
	CNetworkVar( float, m_flEnergyDrinkMeter );
	CNetworkVar( float, m_flFocusLevel );
	CNetworkVar( float, m_flChargeMeter );
	CNetworkVar( float, m_flHypeMeter );
	CNetworkVar( float, m_flFireRage );
	CNetworkVar( float, m_flCrikeyMeter );
private:
#else
	float m_flEnergyDrinkMeter;
	float m_flFocusLevel;
	float m_flChargeMeter;
	float m_flHypeMeter;
	float m_flFireRage;
	float m_flCrikeyMeter;
#endif
	float m_flEnergyDrinkDrainRate;
	float m_flEnergyDrinkRegenRate;
	float m_flChargeDrainRate;
	float m_flChargeRegenRate;
#ifdef CLIENT_DLL
public:
	int m_iDecapitationsParity;
	float m_flShieldChargeEndTime;
	bool m_bShieldChargeStopped;

private:
#endif

	CNetworkHandle( CBaseObject, m_hCarriedObject );
	CNetworkVar( bool, m_bCarryingObject );

	CNetworkVar( int, m_nTeamTeleporterUsed );

	// Arena spectators
	CNetworkVar( bool, m_bArenaSpectator );

	// Gunslinger
	CNetworkVar( bool, m_bGunslinger );

#ifdef GAME_DLL

	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;
#else
	int m_iDisguiseWeaponModelIndex;
	int m_iOldDisguiseWeaponModelIndex;
	CTFWeaponInfo *m_pDisguiseWeaponInfo;

	WEAPON_FILE_INFO_HANDLE	m_hDisguiseWeaponInfo;

	CNewParticleEffect *m_pCritEffect;
	EHANDLE m_hCritEffectHost;
	CSoundPatch *m_pCritSound;

	int	m_nOldDisguiseClass;
	int m_nOldDisguiseTeam;

	int m_iOldDisguiseWeaponID;

	int	m_nOldConditions;
	int m_nOldConditionsEx;
	int m_nOldConditionsEx2;
	int m_nOldConditionsEx3;
	int m_nOldConditionsEx4;

	bool m_bWasCritBoosted;
#endif
};

#define TF_DEATH_DOMINATION				0x0001	// killer is dominating victim
#define TF_DEATH_ASSISTER_DOMINATION	0x0002	// assister is dominating victim
#define TF_DEATH_REVENGE				0x0004	// killer got revenge on victim
#define TF_DEATH_ASSISTER_REVENGE		0x0008	// assister got revenge on victim

extern const char *g_pszBDayGibs[22];

#endif // TF_PLAYER_SHARED_H

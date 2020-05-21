//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//	CTFWeaponBase
//	|
//	|--> CTFWeaponBaseMelee
//	|		|
//	|		|--> CTFWeaponCrowbar
//	|		|--> CTFWeaponKnife
//	|		|--> CTFWeaponMedikit
//	|		|--> CTFWeaponWrench
//	|
//	|--> CTFWeaponBaseGrenade
//	|		|
//	|		|--> CTFWeapon
//	|		|--> CTFWeapon
//	|
//	|--> CTFWeaponBaseGun
//
//=============================================================================
#ifndef TF_WEAPONBASE_H
#define TF_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "tf_weapon_parse.h"
#include "npcevent.h"
#include "econ_item_system.h"

// Client specific.
#if defined( CLIENT_DLL )
	#define CTFWeaponBase C_TFWeaponBase
	#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
	#define CTFViewModel C_TFViewModel
	#include "tf_fx_muzzleflash.h"
	#include "c_tf_viewmodeladdon.h"
#endif

#define MAX_TRACER_NAME		128

CTFWeaponInfo *GetTFWeaponInfo(int iWeapon);
CTFWeaponInfo *GetTFWeaponInfoForItem( int iItemID, int iClass );

class CTFPlayer;
class CBaseObject;
class CTFWeaponBaseGrenadeProj;

class CTraceFilterIgnoreFriendlyCombatItems : public CTraceFilterSimple
{
	DECLARE_CLASS_GAMEROOT( CTraceFilterIgnoreFriendlyCombatItems, CTraceFilterSimple );
public:
	CTraceFilterIgnoreFriendlyCombatItems( IHandleEntity const *ignore, int collissionGroup, int teamNumber )
		: CTraceFilterSimple( ignore, collissionGroup )
	{
		m_iTeamNumber = teamNumber;
		m_bSkipBaseTrace = false;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity == nullptr )
			return false;

		if ( !pEntity->IsCombatItem() )
			return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );

		if ( pEntity->GetTeamNumber() == m_iTeamNumber )
			return false;

		if( !m_bSkipBaseTrace )
			return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );

		return true;
	}

	void AlwaysHitItems( void ) { m_bSkipBaseTrace = true; }

private:
	int m_iTeamNumber;
	bool m_bSkipBaseTrace;
};

// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// TFTODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity );

// Reloading singly.
enum
{
	TF_RELOAD_START = 0,
	TF_RELOADING,
	TF_RELOADING_CONTINUE,
	TF_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

typedef struct
{
	Activity actBaseAct;
	Activity actTargetAct;
	int		iWeaponRole;
} viewmodel_acttable_t;

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

// Interface for weapons that have a charge time
class ITFChargeUpWeapon 
{
public:
	virtual float GetChargeBeginTime( void ) = 0;
	virtual float GetChargeMaxTime( void ) = 0;
};

//=============================================================================
//
// Base TF Weapon Class
//
class CTFWeaponBase : public CBaseCombatWeapon
{
	DECLARE_CLASS( CTFWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined ( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	// Setup.
	CTFWeaponBase();

	virtual void Spawn();
	virtual void Precache();
	virtual bool IsPredicted() const			{ return true; }
	virtual void FallInit( void );
	virtual void OnPickedUp( CBaseCombatCharacter *pNewOwner );

	// Weapon Data.
	CTFWeaponInfo const	&GetTFWpnData() const;
	virtual int GetWeaponID( void ) const;
	bool IsWeapon( int iWeapon ) const;
	virtual int	GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }

	// View model.
	virtual int TranslateViewmodelHandActivity( int iActivity );
	virtual void SetViewModel();
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *DetermineViewModelType(const char *vModel) const;

	// World model.
	virtual const char *GetWorldModel( void ) const;

	virtual bool HideWhenStunned( void ) const { return true; }

#ifdef CLIENT_DLL
	virtual void UpdateViewModel( void );

	C_ViewmodelAttachmentModel *GetViewmodelAddon( void );

	// AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN
	C_BaseAnimating *GetAppropriateWorldOrViewModel( void );

	string_t GetViewModelOffset( void );

	virtual const char*	ModifyEventParticles( const char* token ) { return token; }

	// Stunball
	virtual const char *GetStunballViewmodel( void ) { return NULL_STRING; }
#endif

	virtual void Drop( const Vector &vecVelocity );
	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool Deploy( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool IsViewModelFlipped( void );

	virtual void DepleteAmmo( void ) {} // accessor for consumables
	void IncrementAmmo( void );

	virtual void ReapplyProvision( void );
	virtual void OnActiveStateChanged( int iOldState );
	virtual void UpdateOnRemove( void );

	// Attacks.
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	void CalcIsAttackCritical( void );
	void CalcIsAttackMiniCritical( void );
	virtual bool CalcIsAttackCriticalHelper();
	bool IsCurrentAttackACrit() { return m_bCurrentAttackIsCrit; }
	bool IsCurrentAttackAMiniCrit() { return m_bCurrentAttackIsMiniCrit; }

	// Ammo.
	virtual int	GetMaxClip1( void ) const;
	virtual int	GetDefaultClip1( void ) const;

	// Reloads.
	virtual bool Reload( void );
	virtual void AbortReload( void );
	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	void SendReloadEvents();
	virtual bool CanAutoReload( void ) { return true; }
	virtual bool ReloadOrSwitchWeapons( void );

	virtual bool CanDrop( void ) { return false; }

	// Accessor for bodygroup switching
	virtual void SwitchBodyGroups( void ) {}

	virtual void UpdatePlayerBodygroups( void );

	// Sound.
	bool PlayEmptySound();
	virtual const char *GetShootSound( int iIndex ) const;

	// Activities.
	virtual void ItemBusyFrame( void );
	virtual void ItemPostFrame( void );
	virtual void ItemHolsterFrame( void );

	virtual void SetWeaponVisible( bool visible );

	virtual int GetActivityWeaponRole( void );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t s_acttablePrimary[];
	static acttable_t s_acttableSecondary[];
	static acttable_t s_acttableMelee[];
	static acttable_t s_acttableBuilding[];
	static acttable_t s_acttablePDA[];
	static acttable_t s_acttableItem1[];
	static acttable_t s_acttableItem2[];
	static acttable_t s_acttableMeleeAllClass[];
	static acttable_t s_acttableSecondary2[];
	static acttable_t s_acttablePrimary2[];
	static acttable_t s_acttableItem3[];
	static acttable_t s_acttableItem4[];
	static acttable_t s_acttableLoserState[];
	static acttable_t s_acttableBuildingDeployed[];
	static viewmodel_acttable_t s_viewmodelacttable[];

#ifdef GAME_DLL
	virtual void	AddAssociatedObject( CBaseObject *pObject ) { }
	virtual void	RemoveAssociatedObject( CBaseObject *pObject ) { }
#endif

	// Utility.
	CBasePlayer *GetPlayerOwner() const;
	CTFPlayer *GetTFPlayerOwner() const;

#ifdef CLIENT_DLL
	bool UsingViewModel( void );
	C_BaseEntity *GetWeaponForEffect();

	bool IsFirstPersonView( void ) const;
#endif

	bool CanAttack( void );

	// Raising & Lowering for grenade throws
	bool			WeaponShouldBeLowered( void );
	virtual bool	Ready( void );
	virtual bool	Lower( void );

	virtual void	WeaponIdle( void );

	virtual void	WeaponReset( void );
	virtual void	WeaponRegenerate() {}

	// Muzzleflashes
	virtual const char *GetMuzzleFlashEffectName_3rd( void ) { return NULL; }
	virtual const char *GetMuzzleFlashEffectName_1st( void ) { return NULL; }
	virtual const char *GetMuzzleFlashModel( void );
	virtual float		GetMuzzleFlashModelLifetime( void );
	virtual float		GetMuzzleFlashModelScale( void );
	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual const char	*GetTracerType( void );

	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false ) { return true; }

	float				GetLastFireTime( void ) { return m_flLastFireTime; }

	virtual bool		HasChargeBar( void ) { return false; }
	void				StartEffectBarRegen( void );
	void				EffectBarRegenFinished( void );
	void				CheckEffectBarRegen( void );
	virtual float		GetEffectBarProgress( void );
	virtual void		SetEffectBarProgress( float flEffectBarRegenTime ) { m_flEffectBarRegenTime = flEffectBarRegenTime; }
	virtual const char *GetEffectLabelText( void ) { return ""; }
	void				ReduceEffectBarRegenTime( float flTime ) { m_flEffectBarRegenTime -= flTime; }
	virtual bool		EffectMeterShouldFlash( void ) { return false; }

	void				OnControlStunned( void );

	// StunBall
	virtual bool		PickedUpBall( CTFPlayer *pPlayer ) { return false; }

	const char*			GetExtraWearableModel( void ) const;

	virtual float		GetSpeedMod( void ) const { return 1.0f; }

	bool				IsHonorBound( void ) const;

	bool 				IsPenetrating(void);
	
	virtual bool		CanDecapitate( void );

	// Energy Weapons
	bool IsEnergyWeapon(void);
	float GetEnergyPercentage(void);
	Vector GetEnergyWeaponColor( bool bUseAlternateColorPalette );
	
// Server specific.
#if !defined( CLIENT_DLL )

	// Spawning.
	virtual void CheckRespawn();
	virtual CBaseEntity* Respawn();
	void Materialize();
	void AttemptToMaterialize();

	// Death.
	void Die( void );
	void SetDieThink( bool bDie );

	// Ammo.
	virtual const Vector& GetBulletSpread();

	// On hit effects.
	virtual void ApplyOnHitAttributes( CBaseEntity *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info );
	virtual void ApplyPostOnHitAttributes( CTakeDamageInfo const &info, CTFPlayer *pVictim );

	bool IsSilentKiller( void ) const;
	
	virtual bool OwnerCanTaunt( void ) const { return true; }

	virtual bool GetProjectileModelOverride( CAttribute_String *pOut );
	
// Client specific.
#else

	virtual void	ProcessMuzzleFlashEvent( void );
	virtual int		InternalDrawModel( int flags );
	virtual bool	ShouldDraw( void );

	virtual bool	ShouldPredict();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	GetWeaponCrosshairScale( float &flScale );
	virtual void	Redraw( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	virtual ShadowType_t	ShadowCastType( void );
	virtual int		GetSkin();
	BobState_t		*GetBobState();

	bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel[2];


	void SetMuzzleAttachment( int iAttachment ) { m_iMuzzleAttachment = iAttachment; }
#endif

protected:
#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
#endif // CLIENT_DLL

	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime );
	bool ReloadSingly( void );
	void ReloadSinglyPostFrame( void );
	void Overload(void);

	virtual float InternalGetEffectBarRechargeTime( void ) { return 0.0f; }

protected:

	int				m_iWeaponMode;
	CNetworkVar(	int,	m_iReloadMode );
	CTFWeaponInfo	*m_pWeaponInfo;
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;
	bool			m_bCurrentAttackIsMiniCrit;

	CNetworkVar(	bool,	m_bLowered );

	int				m_iAltFireHint;

	int				m_iReloadStartClipAmount;

	float			m_flCritTime;
	float			m_flLastCritCheckTime;
	int				m_iLastCritCheckFrame;
	int				m_iCurrentSeed;

	CNetworkVar(	float,	m_flLastFireTime );

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar(	bool, m_bResetParity );

	int				m_iRefundedAmmo;

#ifdef CLIENT_DLL
	bool m_bOldResetParity;

	int m_iMuzzleAttachment;
#endif

	CNetworkVar( bool,	m_bReloadedThroughAnimEvent );
	CNetworkVar( float, m_flEffectBarRegenTime );

private:
	CTFWeaponBase( const CTFWeaponBase & );

	CUtlVector< int > m_iHiddenBodygroups;
};

#define WEAPON_RANDOM_RANGE 10000

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )	\
															\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );
#endif // TF_WEAPONBASE_H

//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TF_PLAYER_H
#define TF_PLAYER_H
#pragma once

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "tf_playeranimstate.h"
#include "tf_shareddefs.h"
#include "tf_player_shared.h"
#include "tf_playerclass.h"
#include "entity_tfstart.h"
#include "tf_inventory.h"
#include "tf_weapon_medigun.h"
#include "ihasattributes.h"
#include "nav_mesh/tf_nav_area.h"
#include "Path/NextBotPathFollow.h"
#include "NextBotUtil.h"

struct foundPlayer
{
	foundPlayer *next;
	CTFPlayer *player;
};

class CTFPlayer;
class CTFBot;
class CTFTeam;
class CTFGoal;
class CTFGoalItem;
class CTFItem;
class CTFWeaponBuilder;
class CBaseObject;
class CTFWeaponBase;
class CIntroViewpoint;
class CTriggerAreaCapture;

//=============================================================================
//
// Player State Information
//
class CPlayerStateInfo
{
public:

	int				m_nPlayerState;
	const char *m_pStateName;

	// Enter/Leave state.
	void ( CTFPlayer:: *pfnEnterState )( );
	void ( CTFPlayer:: *pfnLeaveState )( );

	// Think (called every frame).
	void ( CTFPlayer:: *pfnThink )( );
};

struct DamagerHistory_t
{
	DamagerHistory_t()
	{
		Reset();
	}
	void Reset()
	{
		hDamager = NULL;
		flTimeDamage = 0;
	}
	EHANDLE hDamager;
	float	flTimeDamage;
};
#define MAX_DAMAGER_HISTORY 2

struct AppliedContext_t
{
	float flContextExpireTime;
	string_t pszContext;
};

class CTFPlayerPathCost : public IPathCost
{
public:
	CTFPlayerPathCost( CTFPlayer *player )
		: m_pPlayer( player )
	{
		m_flStepHeight = 18.0f;
		m_flMaxJumpHeight = 72.0f;
		m_flDeathDropHeight = 200.0f;
	}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const;

private:
	CTFPlayer *m_pPlayer;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};

//=============================================================================
//
// TF Player
//
class CTFPlayer : public CBaseMultiplayerPlayer, public IHasAttributes
{
public:
	DECLARE_CLASS( CTFPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFPlayer();
	~CTFPlayer();

	// Creation/Destruction.
	static CTFPlayer	*CreatePlayer( const char *className, edict_t *ed );
	static CTFPlayer	*Instance( int iEnt );

	virtual void		Spawn();
	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void		ForceRespawn();
	virtual CBaseEntity *EntSelectSpawnPoint( void );
	virtual void		InitialSpawn();
	virtual void		Precache();
	virtual bool		IsReadyToPlay( void );
	virtual bool		IsReadyToSpawn( void );
	virtual bool		ShouldGainInstantSpawn( void );
	virtual void		ResetScores( void );
	virtual void		PlayerUse( void );
	virtual int			ModCalculateObjectCost(int iObjectType, bool bMini = false /*, int iNumberOfObjects, int iTeam, bool bLast = false*/);

	virtual void		ApplyAbsVelocityImpulse( Vector const &impulse );
	void				ApplyAirBlastImpulse( Vector const &imuplse );

	void				CreateViewModel( int iViewModel = 0 );
	CBaseViewModel		*GetOffHandViewModel();
	void				SendOffHandViewModelActivity( Activity activity );

	virtual void		CheatImpulseCommands( int iImpulse );

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );

	virtual void		LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles );

	virtual CTFNavArea *GetLastKnownArea( void ) const override;

	// Combats
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int			TakeHealth( float flHealth, int bitsDamageType );
	virtual void		OnMyWeaponFired( CBaseCombatWeapon *weapon ) override;
	virtual	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual bool		Event_Gibbed( const CTakeDamageInfo &info );
	virtual bool		BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	void				StopRagdollDeathAnim( void );
	virtual void		PlayerDeathThink( void );

	virtual bool 		IsBehindTarget( CBaseEntity *pVictim );
	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void				ApplyPushFromDamage( const CTakeDamageInfo &info, Vector &vecDir );
	void				SetBlastJumpState( int iJumpType, bool bPlaySound );
	void				ClearBlastJumpState( void );
	int					GetBlastJumpFlags( void ) { return m_nBlastJumpFlags; }
	void				AddDamagerToHistory( EHANDLE hDamager );
	void				ClearDamagerHistory();
	DamagerHistory_t	&GetDamagerHistory( int i ) { return m_DamagerHistory[i]; }
	virtual void		DamageEffect( float flDamage, int fDamageType );
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual int			GetMaxHealth( void ) const;
	virtual int			GetMaxHealthForBuffing( void ) const;

	void				SetHealthBuffTime( float flTime ) { m_flHealthBuffTime = flTime; }

	CTFWeaponBase		*GetActiveTFWeapon( void ) const;
	bool				IsActiveTFWeapon( int iWeaponID );

	CEconItemView		*GetLoadoutItem( int iClass, int iSlot );
	void				HandleCommand_WeaponPreset( int iSlotNum, int iPresetNum );
	void				HandleCommand_WeaponPreset( int iClass, int iSlotNum, int iPresetNum );

	CBaseEntity			*GiveNamedItem( const char *pszName, int iSubType = 0, CEconItemView* pItem = NULL, int iClassNum = -1 );

	void				SaveMe( void );

	void				FireBullet( const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType = TF_DMG_CUSTOM_NONE );
	void				ImpactWaterTrace( trace_t &trace, const Vector &vecStart );
	void				NoteWeaponFired();

	bool				HasItem( void );					// Currently can have only one item at a time.
	void				SetItem( CTFItem *pItem );
	CTFItem				*GetItem( void );

	void				Regenerate( void );
	float				GetNextRegenTime( void ) { return m_flNextRegenerateTime; }
	void				SetNextRegenTime( float flTime ) { m_flNextRegenerateTime = flTime; }

	float				GetNextChangeClassTime( void ) { return m_flNextChangeClassTime; }
	void				SetNextChangeClassTime( float flTime ) { m_flNextChangeClassTime = flTime; }

	virtual	void		RemoveAllItems( bool removeSuit );
	virtual void		RemoveAllWeapons( void );

	bool				DropCurrentWeapon( void );
	void				DropFlag( void );
	void				TFWeaponRemove( int iWeaponID );
	bool				TFWeaponDrop( CTFWeaponBase *pWeapon, bool bThrowForward );

	// Class.
	CTFPlayerClass		*GetPlayerClass( void ) { return &m_PlayerClass; }
	CTFPlayerClass const*GetPlayerClass( void ) const { return &m_PlayerClass; }
	int					GetDesiredPlayerClassIndex( void ) { return m_Shared.m_iDesiredPlayerClass; }
	void				SetDesiredPlayerClassIndex( int iClass ) { m_Shared.m_iDesiredPlayerClass = iClass; }

	// Team.
	void				ForceChangeTeam( int iTeamNum );
	virtual void		ChangeTeam( int iTeamNum, bool bAutoTeam = false, bool bSilent = false );

	// mp_fadetoblack
	void				HandleFadeToBlack( void );
	
	// Water.
	float m_flWetTime;
	virtual float PlayerWetTime( void ) { return m_flWetTime; }
	virtual void  SetWetTime( void ) { m_flWetTime = gpGlobals->curtime; }
	virtual void  ResetWetTime( void ) { m_flWetTime = 0; }
	virtual bool PlayerIsDrippingWet( void ) { return ( ( m_flWetTime > 0 ) && ( gpGlobals->curtime - PlayerWetTime() ) <= 5 ); }
	virtual bool PlayerIsSoaked( void );

	// Flashlight controls for SFM - JasonM
	virtual int			FlashlightIsOn( void );
	virtual void		FlashlightTurnOn( void );
	virtual void		FlashlightTurnOff( void );

	// Think.
	virtual void		PreThink();
	virtual void		PostThink();

	virtual void		ItemPostFrame();
	virtual void		Weapon_FrameUpdate( void );
	virtual void		Weapon_HandleAnimEvent( animevent_t *pEvent );
	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );

	virtual void		OnEmitFootstepSound( CSoundParameters const &sound, Vector const &pos, float volume );

	// Utility.
	void				UpdateModel( void );
	void				UpdateSkin( int iTeam );

	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource ammosource );
	int					GetMaxAmmo( int iAmmoIndex, int iClassNumber = -1 ) const;

	bool				CanAttack( void );

	virtual void		OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );

	// This passes the event to the client's and server's CPlayerAnimState.
	void				DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0 );

	virtual bool		ClientCommand( const CCommand &args );
	void				ClientHearVox( const char *pSentence );
	void				DisplayLocalItemStatus( CTFGoal *pGoal );

	bool				CanPickupBuilding( CBaseObject *pObject );
	bool				TryToPickupBuilding( void );

	int					BuildObservableEntityList( void );
	virtual int			GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget( bool bReverse );
	virtual bool		IsValidObserverTarget( CBaseEntity *target ); // true, if player is allowed to see this target
	virtual bool		SetObserverTarget( CBaseEntity *target );
	virtual bool		ModeWantsSpectatorGUI( int iMode ) { return ( iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM ); }
	void				FindInitialObserverTarget( void );
	CBaseEntity			*FindNearestObservableTarget( Vector vecOrigin, float flMaxDist );
	virtual void		ValidateCurrentObserverTarget( void );

	void				CheckUncoveringSpies( CTFPlayer *pTouchedPlayer );
	void				Touch( CBaseEntity *pOther );

	void				TeamFortress_SetSpeed();
	EHANDLE				TeamFortress_GetDisguiseTarget( int nTeam, int nClass );

	void				TeamFortress_ClientDisconnected();
	void				RemoveAllOwnedEntitiesFromWorld( bool bSilent = true );
	void				RemoveOwnedProjectiles( void );

	CTFTeamSpawn		*GetSpawnPoint( void ) { return m_pSpawnPoint; }

	void				SetAnimation( PLAYER_ANIM playerAnim );

	bool				IsPlayerClass( int iClass ) const;

	void				PlayFlinch( const CTakeDamageInfo &info );

	float				PlayCritReceivedSound( void );
	void				PainSound( const CTakeDamageInfo &info );
	void				DeathSound( const CTakeDamageInfo &info );

	// TF doesn't want the explosion ringing sound
	virtual void		OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	void				OnBurnOther( CTFPlayer *pTFPlayerVictim );

	// Buildables
	void				SetWeaponBuilder( CTFWeaponBuilder *pBuilder );
	CTFWeaponBuilder	*GetWeaponBuilder( void );

	int					GetBuildResources( void );
	void				RemoveBuildResources( int iAmount );
	void				AddBuildResources( int iAmount );

	bool				IsBuilding( void );
	int					CanBuild( int iObjectType, int iObjectMode );

	CBaseObject			*GetObject( int index );
	int					GetObjectCount( void );
	int					GetNumObjects( int iObjectType, int iObjectMode );
	void				RemoveAllObjects( bool bSilent );
	void				StopPlacement( void );
	int					StartedBuildingObject( int iObjectType );
	void				StoppedBuilding( int iObjectType );
	void				FinishedObject( CBaseObject *pObject );
	void				AddObject( CBaseObject *pObject );
	void				OwnedObjectDestroyed( CBaseObject *pObject );
	void				RemoveObject( CBaseObject *pObject );
	bool				PlayerOwnsObject( CBaseObject *pObject );
	void				DetonateOwnedObjectsOfType( int iType, int iMode );
	void				StartBuildingObjectOfType( int iType, int iMode );
	CBaseObject			*GetObjectOfType( int iType, int iMode );

	CTFTeam				*GetTFTeam( void );
	CTFTeam				*GetOpposingTFTeam( void );

	void				TeleportEffect( void );
	void				RemoveTeleportEffect( void );
	virtual bool		IsAllowedToPickUpFlag( void );
	bool				HasTheFlag( void );

	// Death & Ragdolls.
	virtual void		CreateRagdollEntity( void );
	void				CreateRagdollEntity( bool bGibbed, bool bBurning, bool bElectrocute, bool bOnGround, bool bCloak, bool bGoldStatue, bool bIceStatue, bool bDisintigrate, int iDamageCustom, bool bCreatePhysics );
	void				CreateFeignDeathRagdoll( CTakeDamageInfo const &info, bool bGibbed, bool bBurning, bool bFriendlyDisguise );
	void				DestroyRagdoll( void );
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	virtual bool		ShouldGib( const CTakeDamageInfo &info );

	// Dropping Ammo
	void				DropAmmoPack( bool bLunchbox = false, bool bFeigning = false );
	void				DropFakeWeapon( CTFWeaponBase *pWeapon );
	void				DropHealthPack( void );

	bool				CanDisguise( void );
	bool				CanGoInvisible( bool bFeigning = false );
	void				RemoveInvisibility( void );

	void				SpyDeadRingerDeath( CTakeDamageInfo const &info );

	void				RemoveDisguise( void );
	void				PrintTargetWeaponInfo( void );

	bool				DoClassSpecialSkill( void );
	void				EndClassSpecialSkill( void );

	bool				CheckBlockBackstab( CTFPlayer *pAttacker );

	float				GetLastDamageTime( void ) { return m_flLastDamageTime; }

	void				SetClassMenuOpen( bool bIsOpen );
	bool				IsClassMenuOpen( void );

	float				GetCritMult( void ) { return m_Shared.GetCritMult(); }
	void				RecordDamageEvent( const CTakeDamageInfo &info, bool bKill ) { m_Shared.RecordDamageEvent( info, bKill ); }

	bool				GetHudClassAutoKill( void ) { return m_bHudClassAutoKill; }
	void				SetHudClassAutoKill( bool bAutoKill ) { m_bHudClassAutoKill = bAutoKill; }

	bool				GetMedigunAutoHeal( void ) { return m_bMedigunAutoHeal; }
	void				SetMedigunAutoHeal( bool bMedigunAutoHeal ) { m_bMedigunAutoHeal = bMedigunAutoHeal; }

	bool				ShouldAutoRezoom( void ) { return m_bAutoRezoom; }
	void				SetAutoRezoom( bool bAutoRezoom ) { m_bAutoRezoom = bAutoRezoom; }

	bool				ShouldAutoReload( void ) { return m_bAutoReload; }
	void				SetAutoReload( bool bAutoReload ) { m_bAutoReload = bAutoReload; }

	bool				ShouldFlipViewModel( void ) { return m_bFlipViewModel; }
	void				SetFlipViewModel( bool bFlip ) { m_bFlipViewModel = bFlip; }

	virtual void		ModifyOrAppendCriteria( AI_CriteriaSet &criteriaSet );

	virtual bool		CanHearAndReadChatFrom( CBasePlayer *pPlayer );

	Vector 				GetClassEyeHeight( void );

	void				UpdateExpression( void );
	void				ClearExpression( void );

	void				AddPhaseEffects( void );

	virtual IResponseSystem *GetResponseSystem();
	virtual bool		SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool		CanSpeakVoiceCommand( void );
	virtual bool		ShouldShowVoiceSubtitleToEnemy( void );
	virtual void		NoteSpokeVoiceCommand( const char *pszScenePlayed );
	void				SpeakWeaponFire( int iCustomConcept = MP_CONCEPT_NONE );
	void				ClearWeaponFireScene( void );

	virtual int			DrawDebugTextOverlays( void );

	float	m_flNextVoiceCommandTime;
	float	m_flNextSpeakWeaponFire;

	virtual int			CalculateTeamBalanceScore( void );

	bool				ShouldAnnouceAchievement( void );

	virtual void		PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual bool		IsDeflectable( void ) { return true; }

	virtual CAttributeManager *GetAttributeManager() { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer() { return NULL; }
	virtual CBaseEntity *GetAttributeOwner() { return NULL; }
	virtual CAttributeList *GetAttributeList() { return &m_AttributeList; }
	virtual void		ReapplyProvision( void ) { /*Do nothing*/ };

	// Entity inputs
	void				InputIgnitePlayer( inputdata_t &inputdata );
	void				InputExtinguishPlayer( inputdata_t &inputdata );
	void				InputSpeakResponseConcept( inputdata_t &inputdata );
	void				InputSetForcedTauntCam( inputdata_t &inputdata );

public:

	CNetworkVector( m_vecPlayerColor );

	CTFPlayerShared m_Shared;

	bool	m_bPuppet;

	int	    item_list;			// Used to keep track of which goalitems are 
								// affecting the player at any time.
								// GoalItems use it to keep track of their own 
								// mask to apply to a player's item_list

	float	invincible_finished;
	float	invisible_finished;
	float	super_damage_finished;
	float	radsuit_finished;

	int		m_flNextTimeCheck;		// Next time the player can execute a "timeleft" command

	// TEAMFORTRESS VARIABLES
	int		no_sentry_message;
	int		no_dispenser_message;

	CNetworkVar( bool, m_bSaveMeParity );

	// teleporter variables
	int		no_entry_teleporter_message;
	int		no_exit_teleporter_message;

	float	m_flNextNameChangeTime;

	bool	m_bBlastLaunched;

	bool	m_bIsPlayerADev;
	bool	m_bIsPlayerAVIP;
	int		m_iPlayerVIPRanking;
	

	int					StateGet( void ) const;

	void				SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void				HolsterOffHandWeapon( void );

	float				GetSpawnTime() { return m_flSpawnTime; }

	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity );

	bool				ItemsMatch( CEconItemView *pItem1, CEconItemView *pItem2, CTFWeaponBase *pWeapon = NULL );
	void				ValidateWeapons( bool bRegenerate );
	void				ValidateWearables( void );
	void				ValidateWeaponSlots( void );
	void				ValidateWearableSlots( void );
	void				ManageRegularWeapons( TFPlayerClassData_t *pData );
	void				ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData );
	void				ManageRandomWeapons( TFPlayerClassData_t *pData );
	void				ManageBuilderWeapons( TFPlayerClassData_t *pData );
	void				ManageGrenades( TFPlayerClassData_t *pData );
	void				ManagePlayerCosmetics( TFPlayerClassData_t *pData );
	void				ManagePlayerEventCosmetic( TFPlayerClassData_t *pData );
	void				ManageVIPMedal( TFPlayerClassData_t *pData );

	void				PostInventoryApplication( void );

	float				GetDesiredHeadScale( void ) const;
	float				GetHeadScaleSpeed( void );
	
	virtual bool		IsWhiteListed ( const char *pszClassname );

	CTriggerAreaCapture *GetControlPointStandingOn( void );

	bool				IsCapturingPoint( void );

	const Vector		&EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon );
	const Vector		&EstimateProjectileImpactPosition( float pitch, float yaw, float speed );
	const Vector		&EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge );

	// Taunts.
	void				Taunt( taunts_t eTaunt = TAUNT_NORMAL, int iConcept = MP_CONCEPT_PLAYER_TAUNT );
	bool				IsAllowedToTaunt( void );
	bool				IsTaunting( void ) { return m_Shared.InCond( TF_COND_TAUNTING ); }
	void				DoTauntAction( void );
	void				DoTauntActionThink( void );
	float				m_flTauntEmitTime;
	int					m_iSpecialTauntType;
	void				DoTauntAttack( void );
	void				ClearTauntAttack( void );
	QAngle				m_angTauntCamera;

	virtual float		PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	void				ResetTauntHandle( void ) { m_hTauntScene = NULL; }
	void				SetDeathFlags( int iDeathFlags ) { m_iDeathFlags = iDeathFlags; }
	int					GetDeathFlags() { return m_iDeathFlags; }
	void				SetMaxSentryKills( int iMaxSentryKills ) { m_iMaxSentryKills = iMaxSentryKills; }
	int					GetMaxSentryKills() { return m_iMaxSentryKills; }

	CNetworkVar( int, m_iSpawnCounter );

	void				CheckForIdle( void );
	void				PickWelcomeObserverPoint();

	void				StopRandomExpressions( void ) { m_flNextRandomExpressionTime = -1; }
	void				StartRandomExpressions( void ) { m_flNextRandomExpressionTime = gpGlobals->curtime; }

	virtual bool		WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	float				MedicGetChargeLevel( void );
	CBaseEntity			*MedicGetHealTarget( void );

	CWeaponMedigun		*GetMedigun( void );
	CTFWeaponBase		*Weapon_OwnsThisID( int iWeaponID );
	CTFWeaponBase		*Weapon_GetWeaponByType( int iType );
	CEconEntity			*GetEntityForLoadoutSlot( int iSlot );
	CEconWearable		*GetWearableForLoadoutSlot( int iSlot );

	float	m_flSpawnProtectTime;

	bool				CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles );

	// Stun
	void				PlayStunSound( CTFPlayer *pStunner, int nStunFlags/*, int nCurrentStunFlags*/ );

	// Arena
	float	m_flArenaQueueTime;
	bool	m_bInArenaQueue;

	// Not really sure why this is needed in Arena
	bool	m_bIgnoreLastAction;

	bool				IsArenaSpectator( void ) { return m_Shared.m_bArenaSpectator; }

	// Gunslinger
	bool				HasGunslinger( void ) { return m_Shared.m_bGunslinger; }

	CountdownTimer m_purgatoryDuration;

	IntervalTimer m_lastCalledMedic;

	void				SetBombHeadTimestamp( void );
	float				GetTimeSinceWasBombHead( void ) const;
	void				BombHeadExplode( bool bSuicide );
	IntervalTimer		m_lastWasBombHead;
	bool				m_bDiedWithBombHead;
	
	// Eureka Effect.
	
	void StartEurekaTeleport( void ) {m_bEurekaTeleport = true;}
	void SetEurekaTeleportTime( void ) { m_flEurekaTeleportTime = gpGlobals->curtime + 2.0f; }
	bool m_bEurekaTeleport;
	float m_flEurekaTeleportTime;

private:

	int					GetAutoTeam( void );
	float	m_flStunTime;

	// Creation/Destruction.
	virtual void		InitClass( void );
	void				GiveDefaultItems();
	bool				SelectSpawnSpot( const char *pEntClassName, CBaseEntity *&pSpot );
	void				PrecachePlayerModels( void );
	void				RemoveNemesisRelationships();

	// Think.
	void				TFPlayerThink();
	void				MedicRegenThink( void );
	public:
	void				AOEHeal( CTFPlayer *pPatient, CTFPlayer *pHealer );
	private:
	void				UpdateTimers( void );

public:
	void				RemoveMeleeCrit( void );

private:
	// Taunt.
	EHANDLE	m_hTauntScene;
	bool	m_bInitTaunt;

	// Client commands.
	void				HandleCommand_JoinTeam( const char *pTeamName );
	void				HandleCommand_JoinClass( const char *pClassName );
	void				HandleCommand_JoinTeam_NoMenus( const char *pTeamName );
	void				HandleCommand_JoinTeam_NoKill( const char *pTeamName );

	// Bots.
	friend void Bot_Think( CTFPlayer *pBot );
	friend static void tf_bot_add( const CCommand &args );
	friend class CTFBot; friend class CTFBotManager;

	// Physics.
	void				PhysObjectSleep();
	void				PhysObjectWake();

	// Ammo pack.
	void				AmmoPackCleanUp( void );

	// State.
	CPlayerStateInfo	*StateLookupInfo( int nState );
	void				StateEnter( int nState );
	void				StateLeave( void );
	void				StateTransition( int nState );
	void				StateEnterWELCOME( void );
	void				StateThinkWELCOME( void );
	void				StateEnterPICKINGTEAM( void );
	void				StateEnterACTIVE( void );
	void				StateEnterOBSERVER( void );
	void				StateThinkOBSERVER( void );
	void				StateEnterDYING( void );
	void				StateThinkDYING( void );

	virtual bool		SetObserverMode( int mode );
	virtual void		AttemptToExitFreezeCam( void );

	virtual bool		ClickPDA(void);
	bool				PlayGesture( const char *pGestureName );
	bool				PlaySpecificSequence( const char *pSequenceName );
	bool				PlayDeathAnimation( const CTakeDamageInfo &info, CTakeDamageInfo &info_modified );

	bool				GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes );

	void				AddContext( AppliedContext_t context );
	
	float				GetRecursiveDamageTime() { return m_flRecursiveDamage; }
	float				m_flRecursiveDamage;

private:
	// Map introductions
	int					m_iIntroStep;
	CHandle<CIntroViewpoint> m_hIntroView;
	float				m_flIntroShowHintAt;
	float				m_flIntroShowEventAt;
	bool				m_bHintShown;
	bool				m_bAbortFreezeCam;
	bool				m_bSeenRoundInfo;
	bool				m_bRegenerating;

	// Items.
	CNetworkHandle( CTFItem, m_hItem );

	// Combat.
	CNetworkHandle( CTFWeaponBase, m_hOffHandWeapon );

	float					m_flHealthBuffTime;

	float					m_flNextRegenerateTime;
	float					m_flNextChangeClassTime;
	float					m_flNextHealthRegen;

	// Ragdolls.
	Vector					m_vecTotalBulletForce;

	// State.
	CPlayerStateInfo		*m_pStateInfo;

	// Spawn Point
	CTFTeamSpawn			*m_pSpawnPoint;

	// Networked.
	CNetworkQAngle( m_angEyeAngles );					// Copied from EyeAngles() so we can send it to the client.

public:
	QAngle				m_angPrevEyeAngles;

protected:
	CTFPlayerClass		m_PlayerClass;
	int					m_WeaponPreset[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT];

private:
	CTFPlayerAnimState *m_PlayerAnimState;
	int					m_iLastWeaponFireUsercmd;				// Firing a weapon.  Last usercmd we shot a bullet on.
	int					m_iLastSkin;
	public:
	float				m_flLastDamageTime;
	private:
	float				m_flNextPainSoundTime;
	int					m_LastDamageType;
	int					m_iDeathFlags;				// TF_DEATH_* flags with additional death info
	int					m_iMaxSentryKills;			// most kills by a single sentry

	bool				m_bPlayedFreezeCamSound;

	CHandle< CTFWeaponBuilder > m_hWeaponBuilder;

	CUtlVector<EHANDLE>	m_aObjects;			// List of player objects

	bool m_bIsClassMenuOpen;

	Vector m_vecLastDeathPosition;

	float				m_flSpawnTime;

	float				m_flLastAction;
	bool				m_bIsIdle;

	CUtlVector<EHANDLE>	m_hObservableEntities;
	DamagerHistory_t m_DamagerHistory[MAX_DAMAGER_HISTORY];	// history of who has damaged this player
	CUtlVector<float>	m_aBurnOtherTimes;					// vector of times this player has burned others

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	float				m_flNextRandomExpressionTime;
	EHANDLE				m_hWeaponFireSceneEnt;

	bool				m_bSpeakingConceptAsDisguisedSpy;

	bool				m_bHudClassAutoKill;
	bool 				m_bMedigunAutoHeal;
	bool				m_bAutoRezoom;	// does the player want to re-zoom after each shot for sniper rifles
	bool				m_bAutoReload;
	bool				m_bFlipViewModel;

	CNetworkVar( float, m_flHeadScale );

	float				m_flTauntAttackTime;
	int					m_iTauntAttack;

	// Gunslinger taunt
	short				m_nTauntDamageCount;

	float				m_flNextCarryTalkTime;

	int					m_nBlastJumpFlags;
	bool				m_bJumpEffect;

	CNetworkVar( int, m_nForceTauntCam );
	CNetworkVar( bool, m_bTyping );

	friend class CAttributeContainerPlayer;
	CAttributeContainerPlayer m_AttributeManager;

	COutputEvent		m_OnDeath;

	CUtlVector<AppliedContext_t> m_hActiveContexts;

public:
	int				GetPlayerVIPRanking( void );
};

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert an entity into a tf player.
//   Input: pEntity - the entity to convert into a player
//-----------------------------------------------------------------------------
inline CTFPlayer *ToTFPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<CTFPlayer *>( pEntity ) != 0 );
	return static_cast<CTFPlayer *>( pEntity );
}

inline int CTFPlayer::StateGet( void ) const
{
	return m_Shared.m_nPlayerState;
}
#endif //TF_PLAYER

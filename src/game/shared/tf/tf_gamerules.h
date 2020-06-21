//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_GAMERULES_H
#define TF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplayroundbased_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tf_gamestats_shared.h"
#include "tier1/UtlStringMap.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_autorp.h"
#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL

#define CTFGameRules C_TFGameRules
#define CTFGameRulesProxy C_TFGameRulesProxy

#else

extern BOOL no_cease_fire_text;
extern BOOL cease_fire;

class CHealthKit;
class CTeamControlPoint;
class CTeamTrainWatcher;

#endif

extern ConVar tf_avoidteammates;
extern ConVar tf_avoidteammates_pushaway;

extern ConVar fraglimit;

extern Vector g_TFClassViewVectors[];

#ifdef GAME_DLL
class CTFRadiusDamageInfo
{
public:
	CTFRadiusDamageInfo();

	bool ApplyToEntity( CBaseEntity *pEntity );

public:
	const CTakeDamageInfo *info;
	Vector m_vecSrc;
	float m_flRadius;
	float m_flSelfDamageRadius;
	int m_iClassIgnore;
	CBaseEntity *m_pEntityIgnore;
};
#endif

class CTFGameRulesProxy : public CTeamplayRoundBasedRulesProxy
{
public:
	DECLARE_CLASS( CTFGameRulesProxy, CTeamplayRoundBasedRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
	void	InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetGreenTeamRespawnWaveTime(inputdata_t &inputdata);
	void	InputSetYellowTeamRespawnWaveTime(inputdata_t &inputdata);
	void	InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddGreenTeamRespawnWaveTime(inputdata_t &inputdata);
	void	InputAddYellowTeamRespawnWaveTime(inputdata_t &inputdata);
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetGreenTeamGoalString(inputdata_t &inputdata);
	void	InputSetYellowTeamGoalString(inputdata_t &inputdata);
	void	InputSetRedTeamRole( inputdata_t &inputdata );
	void	InputSetBlueTeamRole( inputdata_t &inputdata );
	void	InputSetGreenTeamRole( inputdata_t &inputdata );
	void	InputSetYellowTeamRole( inputdata_t &inputdata );
	void	InputAddRedTeamScore( inputdata_t &inputdata );
	void	InputAddBlueTeamScore( inputdata_t &inputdata );
	void	InputAddGreenTeamScore( inputdata_t &inputdata );
	void	InputAddYellowTeamScore( inputdata_t &inputdata );

	void	InputSetRedKothClockActive( inputdata_t &inputdata );
	void	InputSetBlueKothClockActive( inputdata_t &inputdata );
	void	InputSetGreenKothClockActive( inputdata_t &inputdata );
	void	InputSetYellowKothClockActive( inputdata_t &inputdata );

	void	InputSetCTFCaptureBonusTime( inputdata_t &inputdata );

	void	InputPlayVO( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );
	void	InputPlayVOGreen( inputdata_t &inputdata );
	void	InputPlayVOYellow( inputdata_t &inputdata );

	virtual void Activate();

	int		m_iHud_Type;
	bool	m_bFourTeamMode;
	bool	m_bCTF_Overtime;

#endif
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
	int	iKills;
	int iDeaths;
};

#define MAX_TEAMGOAL_STRING		256

class CTFGameRules : public CTeamplayRoundBasedRules
{
public:
	DECLARE_CLASS( CTFGameRules, CTeamplayRoundBasedRules );

	CTFGameRules();

	// Halloween Scenarios
	enum
	{
		HALLOWEEN_SCENARIO_MANOR = 1,
		HALLOWEEN_SCENARIO_VIADUCT,
		HALLOWEEN_SCENARIO_LAKESIDE,
		HALLOWEEN_SCENARIO_HIGHTOWER,
		HALLOWEEN_SCENARIO_DOOMSDAY
	};

	// Damage Queries.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	virtual int		Damage_GetTimeBased( void );
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetShouldNotBleed( void );

	int				GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );
	virtual bool	TeamMayCapturePoint( int iTeam, int iPointIndex );
	virtual bool	PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	virtual bool	PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
#ifdef GAME_DLL
	void			CollectCapturePoints( CBasePlayer *player, CUtlVector<CTeamControlPoint *> *controlPointVector );
	void			CollectDefendPoints( CBasePlayer *player, CUtlVector<CTeamControlPoint *> *controlPointVector );
	CTeamTrainWatcher *GetPayloadToPush( int iTeam );
	CTeamTrainWatcher *GetPayloadToBlock( int iTeam );
#endif
	static int		CalcPlayerScore( RoundStats_t *pRoundStats );

	bool			IsBirthday( void );
	bool			IsHalloween( void );
	bool			IsFullMoon( void );
	bool			IsChristmas( void );
	bool			IsValentinesDay( void );
	bool			IsAprilFools( void );
	bool			IsEOTL( void );
	bool			IsBreadUpdate( void );
	bool			IsRememberingSoldier( void );
	virtual bool	IsHolidayActive( /*EHoliday*/ int eHoliday );

	bool 			IsNormalClass(CBaseEntity *pPlayer);
	bool 			IsBossClass(CBaseEntity *pPlayer);

	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"E2NcUkG2"; }

	virtual bool	AllowThirdPersonCamera( void );
	
	virtual bool	AllowGlowOutlinesFlags( void );
	virtual bool	AllowGlowOutlinesCarts( void );

	virtual float	GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers = true );

	virtual bool	ShouldBalanceTeams( void );

	CTeamRoundTimer* GetBlueKothRoundTimer( void ) { if (IsInKothMode()) return m_hBlueKothTimer.Get(); else return NULL; }
	CTeamRoundTimer* GetRedKothRoundTimer( void ) { if (IsInKothMode()) return m_hRedKothTimer.Get(); else return NULL; }
	CTeamRoundTimer* GetGreenKothRoundTimer( void ) { return m_hGreenKothTimer.Get(); }
	CTeamRoundTimer* GetYellowKothRoundTimer( void ) { return m_hYellowKothTimer.Get(); }

	CBaseEntity*	GetIT( void ) const { return m_itHandle.Get(); }
	void			SetIT( CBaseEntity *pPlayer );

	int				GetActiveHalloweenEffect( void ) const          { return m_nHalloweenEffect; }
	void			SetActiveHalloweenEffect( int iType )           { m_nHalloweenEffect = iType; }
	float			GetTimeHalloweenEffectStarted( void ) const     { return m_flHalloweenEffectStartTime; }
	void			SetTimeHalloweenEffectStarted( float flTime )   { m_flHalloweenEffectStartTime = flTime; }
	float			GetHalloweenEffectDuration( void ) const        { return m_flHalloweenEffectDuration; }
	void			SetHalloweenEffectDuration( float flDuration )  { m_flHalloweenEffectDuration = flDuration; }

	CUtlStringMap<string_t> m_SavedConvars;
	bool			HaveSavedConvar( ConVarRef const& cvar );
	void			SaveConvar( ConVarRef const& cvar );
	void			RevertSingleConvar( ConVarRef &cvar );
	void			RevertSavedConvars();

	virtual void	RegisterScriptFunctions( void ) override;

#ifdef GAME_DLL
public:
	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool	ShouldCreateEntity( const char *pszClassName );
	virtual void	CleanUpMap( void );

	virtual void	FrameUpdatePostEntityThink();

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart( void );

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning( void );

	// Called before a new round is started (so the previous round can end)
	virtual void	PreviousRoundEnd( void );

	// Send the team scores down to the client
	virtual void	SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	virtual void	SendWinPanelInfo( void );

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound( void );

	// Called when a round has entered stalemate mode (timer has run out)
	virtual void	SetupOnStalemateStart( void );
	virtual void	SetupOnStalemateEnd( void );

	void			RecalculateControlPointState( void );

	virtual void	HandleSwitchTeams( void );
	virtual void	HandleScrambleTeams( void );
	bool			CanChangeClassInStalemate( void );

	virtual void	SetRoundOverlayDetails( void );
	virtual void	ShowRoundInfoPanel( CTFPlayer *pPlayer = NULL ); // NULL pPlayer means show the panel to everyone

	virtual bool	TimerMayExpire( void );

	void			HandleCTFCaptureBonus( int iTeam );

	virtual void	Arena_CleanupPlayerQueue( void );
	virtual void	Arena_ClientDisconnect( const char *pszPlayerName );
	virtual void	Arena_NotifyTeamSizeChange( void );
	virtual int		Arena_PlayersNeededForMatch( void );
	virtual void	Arena_PrepareNewPlayerQueue( bool bScramble );
	virtual void	Arena_ResetLosersScore( bool bStreakReached );
	virtual void	Arena_RunTeamLogic( void );
	virtual void	Arena_SendPlayerNotifications( void );

	float			GetStalemateStartTime( void ) { return m_flStalemateStartTime; }

	virtual void	AddPlayerToQueue( CTFPlayer *pPlayer );
	virtual void	AddPlayerToQueueHead( CTFPlayer *pPlayer );
	virtual void	RemovePlayerFromQueue( CTFPlayer *pPlayer );

	virtual void	Activate();

	virtual void	OnNavMeshLoad( void );

	virtual void	LevelShutdown( void );

	virtual void	SetHudType( int iHudType ) { m_nHudType = iHudType; };

	virtual bool	AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	virtual int		GetClassLimit( int iDesiredClassIndex );
	virtual bool	CanPlayerChooseClass( CBasePlayer *pPlayer, int iDesiredClassIndex );
	bool			CanBotChooseClass( CBasePlayer *pBot, int iDesiredClassIndex );
	bool			CanBotChangeClass( CBasePlayer *pBot );

	void			SetTeamGoalString( int iTeam, const char *pszGoal );

	// Speaking, vcds, voice commands.
	virtual void	InitCustomResponseRulesDicts();
	virtual void	ShutdownCustomResponseRulesDicts();

	virtual bool	HasPassedMinRespawnTime( CBasePlayer *pPlayer );

	bool			ShouldScorePerRound( void );

	virtual int		PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	void			SetBlueKothRoundTimer( CTeamRoundTimer *pTimer ) { m_hBlueKothTimer.Set( pTimer ); }
	void			SetRedKothRoundTimer( CTeamRoundTimer *pTimer ) { m_hRedKothTimer.Set( pTimer ); }
	void			SetGreenKothRoundTimer( CTeamRoundTimer *pTimer ) { m_hGreenKothTimer.Set( pTimer ); }
	void			SetYellowKothRoundTimer( CTeamRoundTimer *pTimer ) { m_hYellowKothTimer.Set( pTimer ); }
	float			GetRoundStartTime( void ) { return m_flRoundStartTime; }

	virtual bool	ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );

	void			StartCompetitiveMatch( void );
	void			StopCompetitiveMatch(/*CMsgGC_Match_Result_Status*/int eMatchResult=0 );
	void			EndCompetitiveMatch( void );

	void			RegisterBoss( CBaseCombatCharacter *pNPC )  { if( m_hBosses.Find( pNPC ) == m_hBosses.InvalidIndex() ) m_hBosses.AddToHead( pNPC ); }
	void			RemoveBoss( CBaseCombatCharacter *pNPC )    { EHANDLE hNPC( pNPC ); m_hBosses.FindAndRemove( hNPC ); }
	CBaseCombatCharacter *GetActiveBoss( void ) const           { if ( m_hBosses.IsEmpty() ) return nullptr; return m_hBosses[0]; }

	void			StartBossTimer( float time )				{ m_bossSpawnTimer.Start( time ); }

protected:
	virtual void	InitTeams( void );

	virtual void	RoundRespawn( void );

	virtual void	InternalHandleTeamWin( int iWinningTeam );

	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );

	virtual void	FillOutTeamplayRoundWinEvent( IGameEvent *event );

	virtual bool	CanChangelevelBecauseOfTimeLimit( void );
	virtual bool	CanGoToStalemate( void );

	virtual int		CountActivePlayers( void );
#endif // GAME_DLL

public:
	// Return the value of this player towards capturing a point
	virtual int		GetCaptureValueForPlayer( CBasePlayer *pPlayer );

	// Collision and Damage rules.
	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );

	int				GetTimeLeft( void );

	// Get the view vectors for this mod.
	virtual const	CViewVectors *GetViewVectors() const;

	virtual void	FireGameEvent( IGameEvent *event );

	virtual const char *GetGameTypeName( void ) { return g_aGameTypeNames[m_nGameType]; }
	virtual int		GetGameType( void ) { return m_nGameType; }

	virtual bool	FlagsMayBeCapped( void );

	void			RunPlayerConditionThink( void );

	const char*		GetTeamGoalString( int iTeam );

	int				GetAssignedHumanTeam( void ) const;

	virtual int		GetHudType( void ){ return m_nHudType; };

	virtual bool	IsMultiplayer( void ){ return true; };

	virtual bool	IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer ) { return true; };

	virtual void	LevelShutdownPostEntity( void );

	float			GetGravityMultiplier( void ) const { return m_flGravityScale; }
	void			SetGravityMultiplier( float value ) { m_flGravityScale = value; }

	virtual bool	IsFourTeamGame( void ){ return m_bFourTeamMode; };
	bool			IsMannVsMachineMode( void ) { return false; };
	virtual bool	IsInArenaMode( void ) { return m_nGameType == TF_GAMETYPE_ARENA; }
	virtual bool    IsInEscortMode( void ) { return m_nGameType == TF_GAMETYPE_ESCORT; }
	virtual bool	IsInMedievalMode( void ) { return m_nGameType == TF_GAMETYPE_MEDIEVAL; }
	virtual bool	IsInKothMode( void ) { return m_bPlayingKoth; }
	virtual bool	IsInVSHMode( void ) { return m_bPlayingVSH; }
	virtual bool	IsInDRMode( void ) { return m_bPlayingDR; }
	virtual bool    IsHalloweenScenario( int iEventType ) { return m_halloweenScenario == iEventType; };
	virtual bool	IsPVEModeActive( void ) { return false; };
	virtual bool	IsCompetitiveMode( void ) { return m_bCompetitiveMode; };
	virtual bool	IsInHybridCTF_CPMode( void ) { return m_bPlayingHybrid_CTF_CP; };
	virtual bool	IsInSpecialDeliveryMode( void ) { return m_bPlayingSpecialDeliveryMode; };
	bool			UsePlayerReadyStatusMode( void );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes data tables able to access our private vars.

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	HandleOvertimeBegin();
	virtual void	GetTeamGlowColor( int nTeam, float &r, float &g, float &b );

	virtual bool	AllowMapVisionFilterShaders( void );
	virtual const char *TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName );
	void			SetUpVisionFilterKeyValues( void );

	bool			ShouldShowTeamGoal( void );

	const char*		GetVideoFileForMap( bool bWithExtension = true );

	// AutoRP
	virtual void	ModifySentChat( char *pBuf, int iBufSize );

private:
	KeyValues *m_pVisionFilterTranslations;
	KeyValues *m_pVisionFilterWhitelist;
#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes data tables able to access our private vars.

	virtual ~CTFGameRules();

	virtual bool	ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void	Think();

	bool			CheckWinLimit();
	bool			CheckFragLimit();
	bool			CheckCapsPerRound();

	virtual bool	FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	// Spawing rules.
	CBaseEntity*	GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool			IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers );

	virtual float	FlItemRespawnTime( CItem *pItem );
	virtual Vector	VecItemRespawnSpot( CItem *pItem );
	virtual QAngle	VecItemRespawnAngles( CItem *pItem );

	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void			ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual void	GetTaggedConVarList( KeyValues *pCvarTagList );
	void			ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName );

	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem );

	bool			IsInPreMatch() const;
	float			GetPreMatchEndTime() const;	// Returns the time at which the prematch will be over.
	void			GoToIntermission( void );

	virtual int		GetAutoAimMode() { return AUTOAIM_NONE; }

	bool			CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	virtual const char *GetGameDescription( void );

	// Sets up g_pPlayerResource.
	virtual void	CreateStandardEntities();
	void			CreateSoldierStatue( void );

	virtual void	PlayerKilled( CBasePlayer *pVictim, CTakeDamageInfo const &info );
	virtual void	DeathNotice( CBasePlayer *pVictim, CTakeDamageInfo const &info, char const *szName );
	virtual void	DeathNotice( CBasePlayer *pVictim, CTakeDamageInfo const &info );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	void			CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	const char*		GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int &iWeaponID );
	CBasePlayer*	GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor );
	CTFPlayer*		GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed );

	virtual void	ClientDisconnected( edict_t *pClient );

	void			RadiusDamage( CTFRadiusDamageInfo &radiusInfo );
	bool			RadiusJarEffect( CTFRadiusDamageInfo &radiusInfo, int iCond );
	virtual void	RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	
	virtual float	FlPlayerFallDamage( CBasePlayer *pPlayer );

	virtual bool	FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	virtual bool	UseSuicidePenalty() { return false; }

	int				GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void			SendHudNotification( IRecipientFilter &filter, HudNotification_t iType );
	void			SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );

	virtual void	PlayerSpawn( CBasePlayer *pPlayer );

	const CUtlVector<EHANDLE> &GetAmmoEnts( void ) const { Assert( m_hAmmoEntities.Count() ); return m_hAmmoEntities; }
	const CUtlVector<EHANDLE> &GetHealthEnts( void ) const { Assert( m_hHealthEntities.Count() ); return m_hHealthEntities; }

	void			PushAllPlayersAway( Vector const &vecPos, float flRange, float flForce, int iTeamNum, CUtlVector<CTFPlayer *> *outVector );

	CUtlVector< CHandle<CBaseCombatCharacter> > m_hBosses;

private:

	int				DefaultFOV( void ) { return 75; }

	void			BeginHaunting( int nDesiredCount, float flMinLifetime, float flMaxLifetime );
	void			SpawnHalloweenBoss( void );
	void			SpawnZombieMob( void );
	CountdownTimer	m_bossSpawnTimer;
	CountdownTimer	m_mobSpawnTimer;
	int				m_nZombiesToSpawn;
	Vector			m_vecMobSpawnLocation;

#endif

private:

#ifdef GAME_DLL

	Vector2D m_vecPlayerPositions[MAX_PLAYERS];

	CUtlVector< CHandle<CHealthKit> > m_hDisabledHealthKits;

	char m_szMostRecentCappers[MAX_PLAYERS+1];	// list of players who made most recent capture.  Stored as string so it can be passed in events.
	int	m_iNumCaps[TF_TEAM_COUNT];				// # of captures ever by each team during a round

	int SetCurrentRoundStateBitString();
	void SetMiniRoundBitMask( int iMask );
	int m_iPrevRoundState;	// bit string representing the state of the points at the start of the previous miniround
	int m_iCurrentRoundState;
	int m_iCurrentMiniRoundMask;
	float m_flTimerMayExpireAt;

	bool m_bFirstBlood;
	int	m_iArenaTeamCount;
	float m_flArenaNotificationSend;

	float m_flStalemateStartTime;

	CUtlVector<CTFPlayer *> m_hArenaQueue;

	CHandle<CTeamTrainWatcher> m_hRedAttackTrain;
	CHandle<CTeamTrainWatcher> m_hBlueAttackTrain;
	CHandle<CTeamTrainWatcher> m_hRedDefendTrain;
	CHandle<CTeamTrainWatcher> m_hBlueDefendTrain;

	CUtlVector<EHANDLE> m_hAmmoEntities;
	CUtlVector<EHANDLE> m_hHealthEntities;

	CHandle<CBaseEntity> m_hSoldierStatue;
#endif

private:
	CNetworkVar( int, m_nGameType ); // Type of game this map is (CTF, CP)
	CNetworkVar( int, m_nMapHolidayType );
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringGreen, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringYellow, MAX_TEAMGOAL_STRING );
	CNetworkVar( float, m_flCapturePointEnableTime );
	CNetworkVar( int, m_nHudType );
	CNetworkVar( bool, m_bPlayingKoth );
	CNetworkVar( bool, m_bPlayingVSH );
	CNetworkVar( bool, m_bPlayingDR );
	CNetworkVar( bool, m_bPlayingMedieval );
	CNetworkVar( bool, m_bPlayingSpecialDeliveryMode );
	CNetworkVar( bool, m_bPlayingRobotDestructionMode );
	CNetworkVar( bool, m_bPlayingMannVsMachine );
	CNetworkVar( bool, m_bPlayingHybrid_CTF_CP );
	CNetworkVar( bool, m_bCompetitiveMode );
	CNetworkVar( bool, m_bPowerupMode );
	CNetworkVar( float, m_flGravityScale );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hBlueKothTimer );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hRedKothTimer );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hGreenKothTimer );
	CNetworkVar( CHandle<CTeamRoundTimer>, m_hYellowKothTimer );
	CNetworkVar( EHANDLE, m_itHandle );
	CNetworkVar( int, m_nHalloweenEffect );
	CNetworkVar( float, m_flHalloweenEffectStartTime );
	CNetworkVar( float, m_flHalloweenEffectDuration );
	CNetworkVar( int, m_halloweenScenario );

public:

	bool m_bControlSpawnsPerTeam[MAX_TEAMS][MAX_CONTROL_POINTS];
	int	 m_iPreviousRoundWinners;

	int	m_iBirthdayMode;
	int	m_iHalloweenMode;
	int	m_iFullMoonMode;
	int	m_iChristmasMode;
	int	m_iValentinesDayMode;
	int	m_iAprilFoolsMode;
	int	m_iBreadUpdateMode;
	int	m_iEOTLMode;
	int m_iSoldierMemorialMode;

	CNetworkVar( bool, m_bFourTeamMode );
	
#ifdef GAME_DLL
	float m_flCTFBonusTime;
#endif

};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CTFGameRules* TFGameRules()
{
	return static_cast<CTFGameRules*>( g_pGameRules );
}

#ifdef GAME_DLL
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );

// Sorting methods
int ScramblePlayersSort( CTFPlayer* const *p1, CTFPlayer* const *p2 );
int SortPlayerSpectatorQueue( CTFPlayer* const *p1, CTFPlayer* const *p2 );
#endif

#ifdef CLIENT_DLL
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
#endif

#endif // TF_GAMERULES_H

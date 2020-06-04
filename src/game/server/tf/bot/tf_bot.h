//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_H
#define TF_BOT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBot/Player/NextBotPlayer.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_path_follower.h"

class CTeamControlPoint;
class CCaptureFlag;
class CCaptureZone;
class CTFBotSquad;

class CClosestTFPlayer
{
public:
	CClosestTFPlayer( Vector start )
		: m_vecOrigin( start )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
		m_iTeam = TEAM_ANY;
	}

	CClosestTFPlayer( Vector start, int teamNum )
		: m_vecOrigin( start ), m_iTeam( teamNum )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
	}

	inline bool operator()( CBasePlayer *player )
	{
		if ( ( player->GetTeamNumber() == TF_TEAM_RED || player->GetTeamNumber() == TF_TEAM_BLUE )
			 && ( m_iTeam == TEAM_ANY || player->GetTeamNumber() == m_iTeam ) )
		{
			float flDistance = ( m_vecOrigin - player->GetAbsOrigin() ).LengthSqr();
			if ( flDistance < m_flMinDist )
			{
				m_flMinDist = flDistance;
				m_pPlayer = (CTFPlayer *)player;
			}
		}

		return true;
	}

	Vector m_vecOrigin;
	float m_flMinDist;
	CTFPlayer *m_pPlayer;
	int m_iTeam;
};


class CTFBot : public NextBotPlayer<CTFPlayer>, public CGameEventListener
{
	DECLARE_CLASS( CTFBot, NextBotPlayer<CTFPlayer> )
public:

	static CBasePlayer *AllocatePlayerEntity( edict_t *edict, const char *playerName );

	CTFBot( CTFPlayer *player=nullptr );
	virtual ~CTFBot();

	DECLARE_INTENTION_INTERFACE( CTFBot )
	ILocomotion *m_locomotor;
	IBody *m_body;
	IVision *m_vision;

	struct DelayedNoticeInfo
	{
		CHandle<CBaseEntity> m_hEnt;
		float m_flWhen;
	};
	void			DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay );
	void			UpdateDelayedThreatNotices( void );

	struct SuspectedSpyInfo
	{
		CHandle<CTFPlayer> m_hSpy;
		CUtlVector<int> m_times;

		void Suspect()
		{
			this->m_times.AddToHead( (int)floor( gpGlobals->curtime ) );
		}

		bool IsCurrentlySuspected()
		{
			if ( this->m_times.IsEmpty() )
			{
				return false;
			}

			extern ConVar tf_bot_suspect_spy_forget_cooldown;
			return ( (float)this->m_times.Head() > ( gpGlobals->curtime - tf_bot_suspect_spy_forget_cooldown.GetFloat() ) );
		}

		bool TestForRealizing()
		{
			extern ConVar tf_bot_suspect_spy_touch_interval;
			int nCurTime = (int)floor( gpGlobals->curtime );
			int nMinTime = nCurTime - tf_bot_suspect_spy_touch_interval.GetInt();

			for ( int i=m_times.Count()-1; i >= 0; --i )
			{
				if ( m_times[i] <= nMinTime )
					m_times.Remove( i );
			}

			m_times.AddToHead( nCurTime );

			CUtlVector<bool> checks;

			checks.SetCount( tf_bot_suspect_spy_touch_interval.GetInt() );
			for ( int i=0; i < checks.Count(); ++i )
				checks[ i ] = false;

			for ( int i=0; i<m_times.Count(); ++i )
			{
				int idx = nCurTime - m_times[i];
				if ( checks.IsValidIndex( idx ) )
					checks[ idx ] = true;
			}

			for ( int i=0; i<checks.Count(); ++i )
			{
				if ( !checks[ i ] )
					return false;
			}

			return true;
		}
	};
	SuspectedSpyInfo *IsSuspectedSpy( CTFPlayer *spy );
	void			SuspectSpy( CTFPlayer *spy );
	void			StopSuspectingSpy( CTFPlayer *spy );
	bool			IsKnownSpy( CTFPlayer *spy ) const;
	void			RealizeSpy( CTFPlayer *spy );
	void			ForgetSpy( CTFPlayer *spy );

	virtual void	Spawn( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual void	UpdateOnRemove( void ) override;
	virtual void	FireGameEvent( IGameEvent *event );
	virtual int		GetBotType() const { return 1337; }
	virtual int		DrawDebugTextOverlays( void );
	virtual void	PhysicsSimulate( void );
	virtual void	Touch( CBaseEntity *other );

	virtual bool	IsDormantWhenDead( void ) const { return false; }
	virtual bool	IsDebugFilterMatch( const char *name ) const;

	virtual void	PressFireButton( float duration = -1.0f );
	virtual void	PressAltFireButton( float duration = -1.0f );
	virtual void	PressSpecialFireButton( float duration = -1.0f );

	virtual void	AvoidPlayers( CUserCmd *pCmd );

	virtual CBaseCombatCharacter *GetEntity( void ) const;

	virtual bool	IsAllowedToPickUpFlag( void );

	void			DisguiseAsEnemy( void );

	bool			IsCombatWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsQuietWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsHitScanWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsExplosiveProjectileWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsContinuousFireWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsBarrageAndReloadWeapon( CTFWeaponBase *weapon = nullptr ) const;

	float			GetMaxAttackRange( void ) const;
	float			GetDesiredAttackRange( void ) const;

	float			GetDesiredPathLookAheadRange( void ) const;

	bool			ShouldFireCompressionBlast( void );

	CTFNavArea*		FindVantagePoint( float flMaxDist );

	virtual ILocomotion *GetLocomotionInterface( void ) const override { return m_locomotor; }
	virtual IBody *GetBodyInterface( void ) const override { return m_body; }
	virtual IVision *GetVisionInterface( void ) const override { return m_vision; }

	bool			IsLineOfFireClear( CBaseEntity *to );
	bool			IsLineOfFireClear( const Vector& to );
	bool			IsLineOfFireClear( const Vector& from, CBaseEntity *to );
	bool			IsLineOfFireClear( const Vector& from, const Vector& to );
	bool			IsAnyEnemySentryAbleToAttackMe( void ) const;
	bool			IsThreatAimingTowardsMe( CBaseEntity *threat, float dotTolerance = 0.8 ) const;
	bool			IsThreatFiringAtMe( CBaseEntity *threat ) const;
	bool			IsEntityBetweenTargetAndSelf( CBaseEntity *blocker, CBaseEntity *target ) const;

	bool			IsAmmoLow( void ) const;
	bool			IsAmmoFull( void ) const;

	bool			AreAllPointsUncontestedSoFar( void ) const;
	bool			IsNearPoint( CTeamControlPoint *point ) const;
	void			ClearMyControlPoint( void ) { m_hMyControlPoint = nullptr; }
	CTeamControlPoint *GetMyControlPoint( void );
	bool			IsAnyPointBeingCaptured( void ) const;
	bool			IsPointBeingContested( CTeamControlPoint *point ) const;
	float			GetTimeLeftToCapture( void );
	CTeamControlPoint *SelectPointToCapture( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectPointToDefend( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectClosestPointByTravelDistance( const CUtlVector<CTeamControlPoint *> &candidates ) const;

	CCaptureZone*	GetFlagCaptureZone( void );
	CCaptureFlag*	GetFlagToFetch( void );

	float			TransientlyConsistentRandomValue( float duration, int seed ) const;

	Action<CTFBot> *OpportunisticallyUseWeaponAbilities( void );

	CBaseObject*	GetNearestKnownSappableTarget( void ) const;

	void			UpdateLookingAroundForEnemies( void );
	void			UpdateLookingForIncomingEnemies( bool );

	bool			EquipBestWeaponForThreat( const CKnownEntity *threat );
	bool			EquipLongRangeWeapon( void );

	void			PushRequiredWeapon( CTFWeaponBase *weapon );
	bool			EquipRequiredWeapon( void );
	void			PopRequiredWeapon( void );

	CTFBotSquad*	GetSquad( void ) const { return m_pSquad; }
	bool			IsSquadmate( CTFPlayer *player ) const;
	void			JoinSquad( CTFBotSquad *squad );
	void			LeaveSquad();

	struct SniperSpotInfo
	{
		CTFNavArea *m_pHomeArea;
		Vector m_vecHome;
		CTFNavArea *m_pForwardArea;
		Vector m_vecForward;
		float m_flRange;
		float m_flIncursionDiff;
	};
	void			AccumulateSniperSpots( void );
	void			SetupSniperSpotAccumulation( void );
	void			ClearSniperSpots( void );

	void			SelectReachableObjects( CUtlVector<EHANDLE> const& append, CUtlVector<EHANDLE> *outVector, INextBotFilter const& func, CNavArea *pStartArea, float flMaxRange );
	CTFPlayer *		SelectRandomReachableEnemy( void );

	bool			CanChangeClass( void );
	const char*		GetNextSpawnClassname( void );

	float			GetUberDeployDelayDuration( void ) const;
	float			GetUberHealthThreshold( void ) const;

	friend class CTFBotSquad;
	CTFBotSquad *m_pSquad;
	float m_flFormationError;
	bool m_bIsInFormation;

	CTFNavArea *m_HomeArea;
	CUtlVector<SniperSpotInfo> m_sniperSpots;

	CHandle<CBaseObject> m_hTargetSentry;
	Vector m_vecLastHurtBySentry;

	enum DifficultyType
	{
		EASY   = 0,
		NORMAL = 1,
		HARD   = 2,
		EXPERT = 3,
		MAX
	}
	m_iSkill;

	enum class AttributeType : int
	{
		NONE                    = 0,

		REMOVEONDEATH           = (1 << 0),
		AGGRESSIVE              = (1 << 1),
		DONTLOOKAROUND			= (1 << 2),
		SUPPRESSFIRE            = (1 << 3),
		DISABLEDODGE            = (1 << 4),
		BECOMESPECTATORONDEATH  = (1 << 5),
		// 6?
		RETAINBUILDINGS         = (1 << 7),
		SPAWNWITHFULLCHARGE     = (1 << 8),
		ALWAYSCRIT              = (1 << 9),
		IGNOREENEMIES           = (1 << 10),
		HOLDFIREUNTILFULLRELOAD = (1 << 11),
		// 12?
		ALWAYSFIREWEAPON        = (1 << 13),
		TELEPORTTOHINT          = (1 << 14),
		MINIBOSS                = (1 << 15),
		USEBOSSHEALTHBAR        = (1 << 16),
		IGNOREFLAG              = (1 << 17),
		AUTOJUMP                = (1 << 18),
		AIRCHARGEONLY           = (1 << 19),
		VACCINATORBULLETS       = (1 << 20),
		VACCINATORBLAST         = (1 << 21),
		VACCINATORFIRE          = (1 << 22),
		BULLETIMMUNE            = (1 << 23),
		BLASTIMMUNE             = (1 << 24),
		FIREIMMUNE              = (1 << 25),
	}
	m_nBotAttrs;

	bool m_bLookingAroundForEnemies;

	CountdownTimer m_cpChangedTimer;
	

private:
	void ManageRandomWeapons( void );

	bool m_bWantsToChangeClass;

	CountdownTimer m_lookForEnemiesTimer;

	CTFPlayer *m_controlling;

	CHandle<CTeamControlPoint> m_hMyControlPoint;
	CountdownTimer m_myCPValidDuration;

	CHandle<CCaptureZone> m_hMyCaptureZone;

	CountdownTimer m_useWeaponAbilityTimer;

	CUtlVector< CHandle<CTFWeaponBase> > m_requiredEquipStack;

	CUtlVector<DelayedNoticeInfo> m_delayedThreatNotices;

	CUtlVectorAutoPurge<SuspectedSpyInfo *> m_suspectedSpies;
	CUtlVector< CHandle<CTFPlayer> > m_knownSpies;

	CUtlVector<CTFNavArea *> m_sniperStandAreas;
	CUtlVector<CTFNavArea *> m_sniperLookAreas;
	CBaseEntity *m_sniperGoalEnt;
	Vector m_sniperGoal;
	CountdownTimer m_sniperSpotTimer;
};

class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost( CTFBot *actor, RouteType routeType = DEFAULT_ROUTE );
	virtual ~CTFBotPathCost() { }

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const override;

private:
	CTFBot *m_Actor;
	RouteType m_iRouteType;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};

DEFINE_ENUM_BITWISE_OPERATORS( CTFBot::AttributeType )
inline bool operator!(CTFBot::AttributeType const &rhs)
{
	return (int const &)rhs == 0;
}
inline bool operator!=( CTFBot::AttributeType const &rhs, int const &lhs )
{
	return (int const &)rhs != lhs;
}

inline CTFBot *ToTFBot( CBaseEntity *ent )
{
	CTFPlayer *player = ToTFPlayer( ent );
	if ( player == nullptr )
		return NULL;

	if ( !player->IsBotOfType( 1337 ) )
		return NULL;

	Assert( dynamic_cast<CTFBot *>( ent ) );
	return static_cast<CTFBot *>( ent );
}

class CTFBotItemSchema : public CAutoGameSystem
{
	DECLARE_CLASS_GAMEROOT( CTFBotItemSchema, CAutoGameSystem );
public:
	CTFBotItemSchema(char const *name)
		: CAutoGameSystem( name )
	{
		m_pSchema = NULL;
	}

	virtual void PostInit();
	virtual void Shutdown();

	virtual void LevelInitPreEntity()		{ PostInit(); }
	virtual void LevelShutdownPostEntity()  { Shutdown(); }

	float		 GetItemChance( char const *pszItemDefIndex, char const *pszChanceName, char const *pszClassName = NULL );
	float		 GetItemSetChance( char const *pszItemSetName );

	KeyValues *operator->() { return m_pSchema; }
private:
	KeyValues *m_pSchema;
};

extern CTFBotItemSchema s_BotSchema;
inline CTFBotItemSchema &TFBotItemSchema( void )
{
	return s_BotSchema;
}

#endif

//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mathlib/mathlib.h"
#include "team_control_point_master.h"
#include "team_train_watcher.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_buff_item.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "tf_bot.h"
#include "tf_bot_components.h"
#include "tf_bot_squad.h"
#include "tf_bot_manager.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "behavior/tf_bot_behavior.h"
#include "behavior/tf_bot_use_item.h"
#include "NextBotUtil.h"

void DifficultyChanged( IConVar *var, const char *pOldValue, float flOldValue );
void PrefixNameChanged( IConVar *var, const char *pOldValue, float flOldValue );

ConVar tf_bot_difficulty( "tf_bot_difficulty", "1", FCVAR_NONE, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert.", &DifficultyChanged );
ConVar tf_bot_force_class( "tf_bot_force_class", "", FCVAR_NONE, "If set to a class name, all TFBots will respawn as that class" );
ConVar tf_bot_keep_class_after_death( "tf_bot_keep_class_after_death", "0" );
ConVar tf_bot_prefix_name_with_difficulty( "tf_bot_prefix_name_with_difficulty", "0", FCVAR_NONE, "Append the skill level of the bot to the bot's name", &PrefixNameChanged );
ConVar tf_bot_path_lookahead_range( "tf_bot_path_lookahead_range", "300", FCVAR_NONE, "", true, 0.0f, true, 1500.0f );
ConVar tf_bot_near_point_travel_distance( "tf_bot_near_point_travel_distance", "750", FCVAR_CHEAT );
ConVar tf_bot_pyro_shove_away_range( "tf_bot_pyro_shove_away_range", "250", FCVAR_CHEAT, "If a Pyro bot's target is closer than this, compression blast them away" );
ConVar tf_bot_pyro_always_reflect( "tf_bot_pyro_always_reflect", "0", FCVAR_CHEAT, "Pyro bots will always reflect projectiles fired at them. For tesing/debugging purposes.", true, 0.0f, true, 1.0f );
ConVar tf_bot_pyro_deflect_tolerance( "tf_bot_pyro_deflect_tolerance", "0.5", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_min_range( "tf_bot_sniper_spot_min_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_max_count( "tf_bot_sniper_spot_max_count", "10", FCVAR_CHEAT, "Stop searching for sniper spots when each side has found this many" );
ConVar tf_bot_sniper_spot_search_count( "tf_bot_sniper_spot_search_count", "10", FCVAR_CHEAT, "Search this many times per behavior update frame" );
ConVar tf_bot_sniper_spot_point_tolerance( "tf_bot_sniper_spot_point_tolerance", "750", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_epsilon( "tf_bot_sniper_spot_epsilon", "100", FCVAR_CHEAT );
ConVar tf_bot_sniper_goal_entity_move_tolerance( "tf_bot_sniper_goal_entity_move_tolerance", "500", FCVAR_CHEAT );
ConVar tf_bot_suspect_spy_touch_interval( "tf_bot_suspect_spy_touch_interval", "5", FCVAR_CHEAT, "How many seconds back to look for touches against suspicious spies", true, 0.0f, false, 0.0f );
ConVar tf_bot_suspect_spy_forget_cooldown( "tf_bot_suspect_spy_forced_cooldown", "5", FCVAR_CHEAT, "How long to consider a suspicious spy as suspicious", true, 0.0f, false, 0.0f );

ConVar tf_bot_use_items( "tf_bot_use_items", "1" );
ConVar tf_bot_debug_items( "tf_bot_debug_items", "0", FCVAR_CHEAT );
ConVar tf_bot_random_loadouts( "tf_bot_random_loadouts", "0", FCVAR_NOTIFY, "Randomly outfit class specific items to bots?" );
ConVar tf_bot_reroll_loadout_chance( "tf_bot_reroll_loadout_chance", "33", FCVAR_NONE, "The chance to reroll a loadout selection if tf_bot_keep_items_after_death = 1" );
ConVar tf_bot_keep_items_after_death( "tf_bot_keep_items_after_death", "1", FCVAR_NONE, "Keep our item sets we were given when respawning?" );


extern ConVar tf2v_force_melee;


LINK_ENTITY_TO_CLASS( tf_bot, CTFBot )

CBasePlayer *CTFBot::AllocatePlayerEntity( edict_t *edict, const char *playerName )
{
	CTFPlayer::s_PlayerEdict = edict;
	return (CTFBot *)CreateEntityByName( "tf_bot" );
}


class SelectClosestPotentiallyVisible
{
public:
	SelectClosestPotentiallyVisible( const Vector &origin )
		: m_vecOrigin( origin )
	{
		m_pSelected = NULL;
		m_flMinDist = FLT_MAX;
	}

	bool operator()( CNavArea *area )
	{
		Vector vecClosest;
		area->GetClosestPointOnArea( m_vecOrigin, &vecClosest );
		float flDistance = ( vecClosest - m_vecOrigin ).LengthSqr();

		if ( flDistance < m_flMinDist )
		{
			m_flMinDist = flDistance;
			m_pSelected = area;
		}

		return true;
	}

	Vector m_vecOrigin;
	CNavArea *m_pSelected;
	float m_flMinDist;
};


class CollectReachableObjects : public ISearchSurroundingAreasFunctor
{
public:
	CollectReachableObjects( CTFBot *actor, CUtlVector<EHANDLE> *selectedHealths, CUtlVector<EHANDLE> *outVector, float flMaxLength )
	{
		m_pBot = actor;
		m_flMaxRange = flMaxLength;
		m_pHealths = selectedHealths;
		m_pVector = outVector;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
	{
		for ( int i=0; i<m_pHealths->Count(); ++i )
		{
			CBaseEntity *pEntity = ( *m_pHealths )[i];
			if ( !pEntity || !area->Contains( pEntity->WorldSpaceCenter() ) )
				continue;

			for ( int j=0; j<m_pVector->Count(); ++j )
			{
				CBaseEntity *pSelected = ( *m_pVector )[j];
				if ( ENTINDEX( pEntity ) == ENTINDEX( pSelected ) )
					return true;
			}

			EHANDLE hndl( pEntity );
			m_pVector->AddToTail( hndl );
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( m_pBot->GetTeamNumber() ) || travelDistanceSoFar > m_flMaxRange )
			return false;

		return currentArea->IsContiguous( adjArea );
	}

private:
	CTFBot *m_pBot;
	CUtlVector<EHANDLE> *m_pHealths;
	CUtlVector<EHANDLE> *m_pVector;
	float m_flMaxRange;
};


class CountClassMembers
{
public:
	CountClassMembers( CTFBot *bot, int teamNum )
		: m_pBot( bot ), m_iTeam( teamNum )
	{
		Q_memset( &m_aClassCounts, 0, sizeof( m_aClassCounts ) );
	}

	bool operator()( CBasePlayer *player )
	{
		if ( player->GetTeamNumber() == m_iTeam )
		{
			++m_iTotal;
			CTFPlayer *pTFPlayer = static_cast<CTFPlayer *>( player );
			if ( !m_pBot->IsSelf( player ) )
				++m_aClassCounts[ pTFPlayer->GetPlayerClass()->GetClassIndex() ];
		}

		return true;
	}

	CTFBot *m_pBot;
	int m_iTeam;
	int m_aClassCounts[TF_CLASS_COUNT_ALL];
	int m_iTotal;
};


IMPLEMENT_INTENTION_INTERFACE( CTFBot, CTFBotMainAction )


CTFBot::CTFBot( CTFPlayer *player )
{
	m_controlling = player;

	m_body = new CTFBotBody( this );
	m_vision = new CTFBotVision( this );
	m_locomotor = new CTFBotLocomotion( this );
	m_intention = new CTFBotIntention( this );

	ListenForGameEvent( "teamplay_point_startcapture" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
}

CTFBot::~CTFBot()
{
	if ( m_body )
		delete m_body;
	if ( m_vision )
		delete m_vision;
	if ( m_locomotor )
		delete m_locomotor;
	if ( m_intention )
		delete m_intention;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::Spawn( void )
{
	// Perform before spawn so we override getting new weapons
	ManageRandomWeapons();

	BaseClass::Spawn();

	m_iSkill = (DifficultyType)tf_bot_difficulty.GetInt();
	m_nBotAttrs = AttributeType::NONE;

	m_useWeaponAbilityTimer.Start( 5.0f );
	m_bLookingAroundForEnemies = true;
	m_suspectedSpies.PurgeAndDeleteElements();
	m_cpChangedTimer.Invalidate();
	m_requiredEquipStack.RemoveAll();
	m_hMyControlPoint = NULL;
	m_hMyCaptureZone = NULL;

	GetVisionInterface()->ForgetAllKnownEntities();

	ClearSniperSpots();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	LeaveSquad();

	if ( !tf_bot_keep_class_after_death.GetBool() )
	{
		if ( TFGameRules()->CanBotChangeClass( this ) )
			m_bWantsToChangeClass = true;
	}

	CTFNavArea *pArea = GetLastKnownArea();
	if ( pArea )
	{
		// remove us from old visible set
		NavAreaCollector visibleSet;
		pArea->ForAllPotentiallyVisibleAreas( visibleSet );

		for( CNavArea *pVisible : visibleSet.m_area )
			static_cast<CTFNavArea *>( pVisible )->RemovePotentiallyVisibleActor( this );
	}

	if ( info.GetInflictor() && info.GetInflictor()->GetTeamNumber() != GetTeamNumber() )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( info.GetInflictor() );
		if ( pSentry )
		{
			m_hTargetSentry = pSentry;
			m_vecLastHurtBySentry = GetAbsOrigin();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateOnRemove( void )
{
	LeaveSquad();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Notify my components
//-----------------------------------------------------------------------------
void CTFBot::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "teamplay_point_startcapture" ) )
	{
		int iCPIndex = event->GetInt( "cp" );
		OnTerritoryContested( iCPIndex );
	}
	else if ( FStrEq( event->GetName(), "teamplay_point_captured" ) )
	{
		ClearMyControlPoint();

		int iCPIndex = event->GetInt( "cp" );
		int iTeam = event->GetInt( "team" );
		if ( iTeam == GetTeamNumber() )
		{
			OnTerritoryCaptured( iCPIndex );
		}
		else
		{
			OnTerritoryLost( iCPIndex );
			m_cpChangedTimer.Start( RandomFloat( 10.0f, 20.0f ) );
		}
	}
	else if ( FStrEq( event->GetName(), "teamplay_flag_event" ) )
	{
		if ( event->GetInt( "eventtype" ) == TF_FLAGEVENT_PICKUP )
		{
			int iPlayer = event->GetInt( "player" );
			if ( iPlayer == GetUserID() )
				OnPickUp( nullptr, nullptr );
		}
	}
	else if ( FStrEq( event->GetName(), "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );
		if ( event->GetBool( "full_round" ) )
		{
			if ( iWinningTeam == GetTeamNumber() )
				OnWin();
			else
				OnLose();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBot::DrawDebugTextOverlays( void )
{
	int text_offset = CTFPlayer::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		EntityText( text_offset, CFmtStr( "FOV: %.2f (%i)", GetVisionInterface()->GetFieldOfView(), GetFOV() ), 0 );
		text_offset++;
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Perform some updates on physics step
//-----------------------------------------------------------------------------
void CTFBot::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();

	if ( m_HomeArea == nullptr )
		m_HomeArea = GetLastKnownArea();

	TeamFortress_SetSpeed();

	if ( m_pSquad && ( m_pSquad->GetMemberCount() <= 1 || !m_pSquad->GetLeader() ) )
		LeaveSquad();

	if ( !IsAlive() && m_bWantsToChangeClass )
	{
		const char *pszClassname = GetNextSpawnClassname();
		HandleCommand_JoinClass( pszClassname );

		m_bWantsToChangeClass = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Alert us and others we bumped a spy
//-----------------------------------------------------------------------------
void CTFBot::Touch( CBaseEntity *other )
{
	BaseClass::Touch( other );

	CTFPlayer *pOther = ToTFPlayer( other );
	if ( !pOther )
		return;

	if ( IsEnemy( pOther ) )
	{
		if ( pOther->m_Shared.IsStealthed() || pOther->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			RealizeSpy( pOther );
		}

		// hack nearby bots into reacting to bumping someone
		TheNextBots().OnWeaponFired( pOther, pOther->GetActiveTFWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAllowedToPickUpFlag( void )
{
	if ( BaseClass::IsAllowedToPickUpFlag() )
	{
		if ( !m_pSquad || this == m_pSquad->GetLeader() )
		{
			//return DWORD( this + 2468 ) == 0;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Disguise as a dead enemy for maximum espionage
//-----------------------------------------------------------------------------
void CTFBot::DisguiseAsEnemy( void )
{
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( this ), false );

	int iClass = TF_CLASS_UNDEFINED;
	for ( int i=0; i < enemies.Count(); ++i )
	{
		if ( !enemies[i]->IsAlive() )
			iClass = enemies[i]->GetPlayerClass()->GetClassIndex();
	}

	if ( iClass == TF_CLASS_UNDEFINED )
		iClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );

	m_Shared.Disguise( GetEnemyTeam( this ), iClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsCombatWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return true;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_PASSTIME_GUN:
		case TF_WEAPON_PARACHUTE:
		case TF_WEAPON_ROCKETPACK:
			return false;

		default:
			return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsQuietWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_FISTS:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_PDA_SPY_BUILD:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsHitScanWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	if ( !IsCombatWeapon( weapon ) )
	{
		return false;
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SMG:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_REVOLVER:
		case TF_WEAPON_SENTRY_BULLET:
		case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
		case TF_WEAPON_PEP_BRAWLER_BLASTER:
		case TF_WEAPON_SODA_POPPER:
		case TF_WEAPON_CHARGED_SMG:
		case TF_WEAPON_HANDGUN_SCOUT_SEC:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsExplosiveProjectileWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_SENTRY_ROCKET:
		case TF_WEAPON_JAR:
		case TF_WEAPON_JAR_MILK:
		case TF_WEAPON_DIRECTHIT:
		case TF_WEAPON_JAR_GAS:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsContinuousFireWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	if ( !IsCombatWeapon( weapon ) )
	{
		return false;
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SMG:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_FLAMETHROWER:
		case TF_WEAPON_LASER_POINTER:
		case TF_WEAPON_CHARGED_SMG:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsBarrageAndReloadWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
			return true;

		default:
			return false;
	}
}

//TODO: why does this only care about the current weapon?
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAmmoLow( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return false;

	if ( weapon->GetWeaponID() != TF_WEAPON_WRENCH )
	{
		if ( !weapon->IsMeleeWeapon() )
		{
			// int ammoType = weapon->GetPrimaryAmmoType();
			int current = GetAmmoCount( 1 );
			return current / GetMaxAmmo( 1 ) < 0.2f;
		}

		return false;
	}

	return GetAmmoCount( TF_AMMO_METAL ) < 50;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAmmoFull( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return false;

	int primaryCount = GetAmmoCount( TF_AMMO_PRIMARY );
	bool primaryFull = primaryCount >= GetMaxAmmo( TF_AMMO_PRIMARY );

	int secondaryCount = GetAmmoCount( TF_AMMO_SECONDARY );
	bool secondaryFull = secondaryCount >= GetMaxAmmo( TF_AMMO_SECONDARY );

	if ( !IsPlayerClass( TF_CLASS_ENGINEER ) )
		return primaryFull && secondaryFull;

	return GetAmmoCount( TF_AMMO_METAL ) >= 200;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::AreAllPointsUncontestedSoFar( void ) const
{
	if ( g_hControlPointMasters.IsEmpty() )
		return true;

	if ( !g_hControlPointMasters[0].IsValid() )
		return true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	for ( int i=0; i<pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pPoint && pPoint->HasBeenContested() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsNearPoint( CTeamControlPoint *point ) const
{
	if ( !point )
		return false;

	CTFNavArea *myArea = GetLastKnownArea();
	if ( !myArea )
		return false;
	
	int iPointIdx = point->GetPointIndex();
	if ( iPointIdx < MAX_CONTROL_POINTS )
	{
		CTFNavArea *cpArea = TFNavMesh()->GetMainControlPointArea( iPointIdx );
		if ( !cpArea )
			return false;

		return abs( myArea->GetIncursionDistance( GetTeamNumber() ) - cpArea->GetIncursionDistance( GetTeamNumber() ) ) < tf_bot_near_point_travel_distance.GetFloat();
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return a CP that we desire to defend or capture
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::GetMyControlPoint( void )
{
	if ( !m_hMyControlPoint || m_myCPValidDuration.IsElapsed() )
	{
		m_myCPValidDuration.Start( RandomFloat( 1.0f, 2.0f ) );

		CUtlVector<CTeamControlPoint *> defensePoints;
		CUtlVector<CTeamControlPoint *> attackPoints;
		TFGameRules()->CollectDefendPoints( this, &defensePoints );
		TFGameRules()->CollectCapturePoints( this, &attackPoints );

		if ( ( IsPlayerClass( TF_CLASS_SNIPER ) || IsPlayerClass( TF_CLASS_ENGINEER )/* || BYTE( this + 10061 ) & ( 1 << 4 ) */) && !defensePoints.IsEmpty() )
		{
			CTeamControlPoint *pPoint = SelectPointToDefend( defensePoints );
			if ( pPoint )
			{
				m_hMyControlPoint = pPoint;
				return pPoint;
			}
		}
		else
		{
			CTeamControlPoint *pPoint = SelectPointToCapture( attackPoints );
			if ( pPoint )
			{
				m_hMyControlPoint = pPoint;
				return pPoint;
			}
			else
			{
				m_myCPValidDuration.Invalidate();

				pPoint = SelectPointToDefend( defensePoints );
				if ( pPoint )
				{
					m_hMyControlPoint = pPoint;
					return pPoint;
				}
			}
		}

		m_myCPValidDuration.Invalidate();

		return nullptr;
	}

	return m_hMyControlPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAnyPointBeingCaptured( void ) const
{
	if ( g_hControlPointMasters.IsEmpty() )
		return false;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( pMaster )
	{
		for ( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
			if ( IsPointBeingContested( pPoint ) )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsPointBeingContested( CTeamControlPoint *point ) const
{
	if ( point )
	{
		if ( ( point->LastContestedAt() + 5.0f ) > gpGlobals->curtime )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetTimeLeftToCapture( void )
{
	int iTeam = GetTeamNumber();

	if ( TFGameRules()->IsInKothMode() )
	{
		if ( iTeam != TF_TEAM_RED )
		{
			if ( iTeam != TF_TEAM_BLUE )
				return 0.0f;

			CTeamRoundTimer *pBlueTimer = TFGameRules()->GetBlueKothRoundTimer();
			if ( pBlueTimer )
				return pBlueTimer->GetTimeRemaining();
		}
		else
		{
			CTeamRoundTimer *pRedTimer = TFGameRules()->GetRedKothRoundTimer();
			if ( pRedTimer )
				return pRedTimer->GetTimeRemaining();
		}
	}
	else
	{
		CTeamRoundTimer *pTimer = TFGameRules()->GetActiveRoundTimer();
		if ( pTimer )
			return pTimer->GetTimeRemaining();
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToCapture( CUtlVector<CTeamControlPoint *> const &candidates )
{
	if ( candidates.IsEmpty() )
		return nullptr;

	if ( candidates.Count() == 1 )
		return candidates[0];

	if ( IsCapturingPoint() )
	{
		CTriggerAreaCapture *pCapArea = GetControlPointStandingOn();
		if ( pCapArea )
			return pCapArea->GetControlPoint();
	}

	CTeamControlPoint *pClose = SelectClosestPointByTravelDistance( candidates );
	if ( pClose && IsPointBeingContested( pClose ) )
		return pClose;

	float flMaxDanger = FLT_MIN;
	bool bInCombat = false;
	CTeamControlPoint *pDangerous = nullptr;

	for ( int i=0; i<candidates.Count(); ++i )
	{
		CTeamControlPoint *pPoint = candidates[i];
		if ( IsPointBeingContested( pPoint ) )
			return pPoint;

		CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
		if ( pCPArea == nullptr )
			continue;

		float flDanger = pCPArea->GetCombatIntensity();
		bInCombat = flDanger > 0.1f ? true : false;

		if ( flMaxDanger < flDanger )
		{
			flMaxDanger = flDanger;
			pDangerous = pPoint;
		}
	}

	if ( bInCombat )
		return pDangerous;

	// Probaly some Min/Max going on here
	int iSelection = candidates.Count() - 1;
	if ( iSelection >= 0 )
	{
		int iRandSel = candidates.Count() * TransientlyConsistentRandomValue( 60.0f, 0 );
		if ( iRandSel < 0 )
			return candidates[0];

		if ( iRandSel <= iSelection )
			iSelection = iRandSel;
	}

	return candidates[iSelection];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToDefend( CUtlVector<CTeamControlPoint *> const &candidates )
{
	if ( candidates.IsEmpty() )
		return nullptr;

	if ( ( m_nBotAttrs & CTFBot::AttributeType::DISABLEDODGE ) != 0 )
		return SelectClosestPointByTravelDistance( candidates );

	return candidates.Random();
}

//-----------------------------------------------------------------------------
// Purpose: Return the closest control point to us
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectClosestPointByTravelDistance( CUtlVector<CTeamControlPoint *> const &candidates ) const
{
	CTeamControlPoint *pClosest = nullptr;
	float flMinDist = FLT_MAX;
	CTFPlayerPathCost cost( (CTFPlayer *)this );

	if ( GetLastKnownArea() )
	{
		for ( int i=0; i<candidates.Count(); ++i )
		{
			CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( candidates[i]->GetPointIndex() );
			float flDist = NavAreaTravelDistance( GetLastKnownArea(), pCPArea, cost );

			if ( flDist >= 0.0f && flMinDist > flDist )
			{
				flMinDist = flDist;
				pClosest = candidates[i];
			}
		}
	}

	return pClosest;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureZone *CTFBot::GetFlagCaptureZone( void )
{
	if ( !m_hMyCaptureZone && TFGameRules()->GetGameType() == TF_GAMETYPE_CTF )
	{
		for ( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
		{
			CCaptureZone *pZone = static_cast<CCaptureZone *>( ICaptureZoneAutoList::AutoList()[i] );
			if ( pZone && pZone->GetTeamNumber() == GetTeamNumber() )
				m_hMyCaptureZone = pZone;
		}
	}

	return m_hMyCaptureZone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureFlag *CTFBot::GetFlagToFetch( void )
{
	CUtlVector<CCaptureFlag *> flags;
	int nNumStolen = 0;
	for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
	{
		CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
		if ( !pFlag || pFlag->IsDisabled() )
			continue;

		if ( HasTheFlag(/* 0, 0 */) && pFlag->GetOwnerEntity() == this )
			return pFlag;

		if ( pFlag->GetGameType() > TF_FLAGTYPE_CTF && pFlag->GetGameType() <= TF_FLAGTYPE_INVADE )
		{
			if ( pFlag->GetTeamNumber() != GetEnemyTeam( this ) )
				flags.AddToTail( pFlag );

			nNumStolen += pFlag->IsStolen();
		}
		else if ( pFlag->GetGameType() == TF_FLAGTYPE_CTF )
		{
			if ( pFlag->GetTeamNumber() == GetEnemyTeam( this ) )
				flags.AddToTail( pFlag );

			nNumStolen += pFlag->IsStolen();
		}
	}

	float flMinDist = FLT_MAX;
	float flMinStolenDist = FLT_MAX;
	CCaptureFlag *pClosest = NULL;
	CCaptureFlag *pClosestStolen = NULL;

	for ( CCaptureFlag *pFlag : flags )
	{
		float flDistance = ( pFlag->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( flDistance > flMinDist )
		{
			flMinDist = flDistance;
			pClosest = pFlag;
		}

		if ( flags.Count() > nNumStolen )
		{
			if ( pFlag->IsStolen() || flMinStolenDist <= flDistance )
				continue;

			flMinStolenDist = flDistance;
			pClosestStolen = pFlag;
		}
	}

	if ( pClosestStolen )
		return pClosestStolen;

	return pClosest;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( CBaseEntity *to )
{
	return IsLineOfFireClear( EyePosition(), to );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &to )
{
	return IsLineOfFireClear( EyePosition(), to );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &from, CBaseEntity *to )
{
	NextBotTraceFilterIgnoreActors filter( nullptr, COLLISION_GROUP_NONE );

	trace_t trace;
	UTIL_TraceLine( from, to->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit() || trace.m_pEnt == to;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &from, const Vector &to )
{
	NextBotTraceFilterIgnoreActors filter( nullptr, COLLISION_GROUP_NONE );

	trace_t trace;
	UTIL_TraceLine( from, to, MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAnyEnemySentryAbleToAttackMe( void ) const
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( obj == nullptr )
			continue;

		if ( obj->ObjectType() == OBJ_SENTRYGUN && !obj->IsPlacing() &&
			!obj->IsBuilding() && !obj->IsBeingCarried() && !obj->HasSapper() )
		{
			if ( ( GetAbsOrigin() - obj->GetAbsOrigin() ).LengthSqr() < Square( SENTRYGUN_BASE_RANGE ) &&
				IsThreatAimingTowardsMe( obj, 0.95f ) && IsLineOfSightClear( obj, CBaseCombatCharacter::IGNORE_ACTORS ) )
			{
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsThreatAimingTowardsMe( CBaseEntity *threat, float dotTolerance ) const
{
	if ( threat == nullptr )
		return false;

	Vector vecToActor = GetAbsOrigin() - threat->GetAbsOrigin();
	vecToActor.NormalizeInPlace();

	CTFPlayer *player = ToTFPlayer( threat );
	if ( player )
	{
		Vector fwd;
		player->EyeVectors( &fwd );

		return vecToActor.Dot( fwd ) > dotTolerance;
	}

	CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry )
	{
		Vector fwd;
		AngleVectors( sentry->GetTurretAngles(), &fwd );

		return vecToActor.Dot( fwd ) > dotTolerance;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsThreatFiringAtMe( CBaseEntity *threat ) const
{
	if ( !IsThreatAimingTowardsMe( threat ) )
		return false;

	// looking at me, but has it shot at me yet
	if ( threat->IsPlayer() )
		return ( (CBasePlayer *)threat )->IsFiringWeapon();

	CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry )
	{
		// if it hasn't fired recently then it's clearly not shooting at me
		return sentry->GetTimeSinceLastFired() < 1.0f;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsEntityBetweenTargetAndSelf( CBaseEntity *blocker, CBaseEntity *target ) const
{
	Vector vecToTarget = ( target->GetAbsOrigin() - GetAbsOrigin() );
	Vector vecToEntity = ( blocker->GetAbsOrigin() - GetAbsOrigin() );
	if ( vecToEntity.NormalizeInPlace() < vecToTarget.NormalizeInPlace() )
	{
		if ( vecToTarget.Dot( vecToEntity ) > 0.7071f )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::TransientlyConsistentRandomValue( float duration, int seed ) const
{
	CTFNavArea *area = GetLastKnownArea();
	if ( area == nullptr )
	{
		return 0.0f;
	}

	int time_seed = (int)( gpGlobals->curtime / duration ) + 1;
	seed += ( area->GetID() * time_seed * entindex() );

	return fabs( FastCos( (float)seed ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetMaxAttackRange() const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
	{
		return 0.0f;
	}

	if ( weapon->IsMeleeWeapon() )
	{
		return 100.0f;
	}

	if ( weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		return 250.0f;
	}

	if ( IsExplosiveProjectileWeapon( weapon ) )
	{
		return 3000.0f;
	}

	return FLT_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetDesiredAttackRange( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return 0.0f;

	if ( weapon->IsWeapon( TF_WEAPON_KNIFE ) )
		return 70.0f;

	if ( !weapon->IsMeleeWeapon() && !weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		if ( !WeaponID_IsSniperRifle( weapon->GetWeaponID() ) )
		{
			if ( !weapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) )
				return 500.0f; // this will make pretty much every weapon use this as the desired range, not sure if intended/correct

			return 1250.0f; // rocket launchers apperantly have a larger desired range than hitscan
		}

		return FLT_MAX;
	}

	return 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetDesiredPathLookAheadRange( void ) const
{
	return GetModelScale() * tf_bot_path_lookahead_range.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsDebugFilterMatch( const char *name ) const
{
	if ( !Q_stricmp( name, GetPlayerClass()->GetName() ) )
		return true;

	return INextBot::IsDebugFilterMatch( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressAltFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressAltFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressSpecialFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressSpecialFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::ShouldFireCompressionBlast( void )
{
	if ( !tf_bot_pyro_always_reflect.GetBool() )
	{
		if ( TFGameRules()->IsInTraining() || m_iSkill == CTFBot::EASY )
			return false;

		if ( m_iSkill == CTFBot::NORMAL && TransientlyConsistentRandomValue( 1.0, 0 ) < 0.5f )
			return false;

		if ( m_iSkill == CTFBot::HARD && TransientlyConsistentRandomValue( 1.0, 0 ) < 0.1f )
			return false;
	}

	const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() && threat->GetEntity()->IsPlayer() )
	{
		CTFPlayer *pTarget = ToTFPlayer( threat->GetEntity() );
		if ( IsRangeLessThan( pTarget, tf_bot_pyro_shove_away_range.GetFloat() ) )
		{
			if ( pTarget->m_Shared.IsInvulnerable() )
				return true;

			if ( pTarget->GetGroundEntity() )
			{
				if ( pTarget->IsCapturingPoint() && TransientlyConsistentRandomValue( 3.0, 0 ) < 0.5f )
					return true;
			}

			return TransientlyConsistentRandomValue( 0.5, 0 ) < 0.5f;
		}
	}

	CBaseEntity *pList[128];
	Vector vecOrig = EyePosition();
	QAngle angDir = EyeAngles();

	Vector vecFwd;
	AngleVectors( angDir, &vecFwd );
	vecOrig += vecFwd * 128.0f;

	Vector vecMins, vecMaxs;
	vecMins = vecOrig - Vector( 128.0f, 128.0f, 64.0f );
	vecMaxs = vecOrig + Vector( 128.0f, 128.0f, 64.0f );

	int count = UTIL_EntitiesInBox( pList, 128, vecMins, vecMaxs, FL_CLIENT|FL_GRENADE );
	for ( int i=0; i<count; ++i )
	{
		CBaseEntity *pEnt = pList[i];
		if ( pEnt != this && !pEnt->IsPlayer() && pEnt->IsDeflectable() )
		{
			if ( FClassnameIs( pEnt, "tf_projectile_rocket" ) || FClassnameIs( pEnt, "tf_projectile_energy_ball" ) )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( pEnt->WorldSpaceCenter() ) )
					return true;
			}

			Vector vecVel = pEnt->GetAbsVelocity();
			vecVel.NormalizeInPlace();

			if ( vecVel.Dot( vecFwd.Normalized() ) <= abs( tf_bot_pyro_deflect_tolerance.GetFloat() ) )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( pEnt->WorldSpaceCenter() ) )
					return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFNavArea *CTFBot::FindVantagePoint( float flMaxDist )
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Action<CTFBot> *CTFBot::OpportunisticallyUseWeaponAbilities( void )
{
	if ( !m_useWeaponAbilityTimer.IsElapsed() )
		return nullptr;

	m_useWeaponAbilityTimer.Start( RandomFloat( 0.1f, 0.4f ) );

	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBase *weapon = dynamic_cast<CTFWeaponBase *>( GetWeapon( i ) );
		if ( weapon == nullptr )
			continue;

		if ( weapon->GetWeaponID() == TF_WEAPON_BUFF_ITEM )
		{
			CTFBuffItem *buff = static_cast<CTFBuffItem *>( weapon );
			if ( buff->IsFull() )
				return new CTFBotUseItem( weapon );

			continue;
		}

		if ( weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX || weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX_DRINK )
		{
			if ( !weapon->HasAmmo() || ( IsPlayerClass( TF_CLASS_SCOUT ) && weapon->GetEffectBarProgress() < 1.0f ) )
				continue;

			return new CTFBotUseItem( weapon );
		}

		if ( weapon->GetWeaponID() == TF_WEAPON_BAT_WOOD && GetAmmoCount( weapon->GetSecondaryAmmoType() ) > 0 )
		{
			const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat( false );
			if ( threat == nullptr || !threat->IsVisibleRecently() )
				continue;

			this->PressAltFireButton();
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject *CTFBot::GetNearestKnownSappableTarget( void ) const
{
	CUtlVector<CKnownEntity> knowns;
	GetVisionInterface()->CollectKnownEntities( &knowns );

	float flMinDist = Square( 500.0f );
	CBaseObject *ret = nullptr;
	for ( int i=0; i<knowns.Count(); ++i )
	{
		CBaseObject *obj = dynamic_cast<CBaseObject *>( knowns[i].GetEntity() );
		if ( obj && !obj->HasSapper() && this->IsEnemy( knowns[i].GetEntity() ) )
		{
			float flDist = this->GetRangeSquaredTo( obj );
			if ( flDist < flMinDist )
			{
				ret = obj;
				flMinDist = flDist;
			}
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay )
{
	const float when = gpGlobals->curtime + delay;

	FOR_EACH_VEC( m_delayedThreatNotices, i )
	{
		DelayedNoticeInfo *info = &m_delayedThreatNotices[i];

		if ( ent == info->m_hEnt )
		{
			if ( when < info->m_flWhen )
			{
				info->m_flWhen = when;
			}

			return;
		}
	}

	m_delayedThreatNotices.AddToTail( {ent, delay} );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateDelayedThreatNotices()
{
	FOR_EACH_VEC_BACK( m_delayedThreatNotices, i )
	{
		DelayedNoticeInfo *info = &m_delayedThreatNotices[i];

		if ( gpGlobals->curtime >= info->m_flWhen )
		{
			CBaseEntity *ent = info->m_hEnt;
			if ( ent )
			{
				CTFPlayer *player = ToTFPlayer( ent );
				if ( player && player->IsPlayerClass( TF_CLASS_SPY ) )
				{
					RealizeSpy( player );
				}

				GetVisionInterface()->AddKnownEntity( ent );
			}

			m_delayedThreatNotices.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBot::SuspectedSpyInfo *CTFBot::IsSuspectedSpy( CTFPlayer *spy )
{
	FOR_EACH_VEC( m_suspectedSpies, i )
	{
		SuspectedSpyInfo *info = m_suspectedSpies[i];
		if ( info->m_hSpy == spy )
		{
			return info;
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SuspectSpy( CTFPlayer *spy )
{
	SuspectedSpyInfo *info = IsSuspectedSpy( spy );
	if ( info == nullptr )
	{
		info = new SuspectedSpyInfo;
		info->m_hSpy = spy;
		m_suspectedSpies.AddToHead( info );
	}

	info->Suspect();
	if ( info->TestForRealizing() )
	{
		RealizeSpy( spy );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::StopSuspectingSpy( CTFPlayer *spy )
{
	FOR_EACH_VEC( m_suspectedSpies, i )
	{
		SuspectedSpyInfo *info = m_suspectedSpies[i];
		if ( info->m_hSpy == spy )
		{
			delete info;
			m_suspectedSpies.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsKnownSpy( CTFPlayer *spy ) const
{
	return m_knownSpies.HasElement( spy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::RealizeSpy( CTFPlayer *spy )
{
	if ( IsKnownSpy( spy ) )
		return;

	m_knownSpies.AddToHead( spy );

	SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_CLOAKEDSPY );

	SuspectedSpyInfo *info = IsSuspectedSpy( spy );
	if ( info && info->IsCurrentlySuspected() )
	{
		CUtlVector<CTFPlayer *> teammates;
		CollectPlayers( &teammates, GetTeamNumber(), true );

		FOR_EACH_VEC( teammates, i )
		{
			CTFBot *teammate = ToTFBot( teammates[i] );
			if ( teammate && !teammate->IsKnownSpy( spy ) )
			{
				if ( EyePosition().DistToSqr( teammate->EyePosition() ) < Square( 512.0f ) )
				{
					teammate->SuspectSpy( spy );
					teammate->RealizeSpy( spy );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::ForgetSpy( CTFPlayer *spy )
{
	StopSuspectingSpy( spy );

	CHandle<CTFPlayer> hndl( spy );
	m_knownSpies.FindAndFastRemove( hndl );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateLookingAroundForEnemies( void )
{
	if ( !m_bLookingAroundForEnemies || m_Shared.IsControlStunned() || ( m_nBotAttrs & AttributeType::DONTLOOKAROUND ) == AttributeType::DONTLOOKAROUND )
		return;

	const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat();
	if ( !threat || !threat->GetEntity() )
	{
		UpdateLookingForIncomingEnemies( true );
		return;
	}

	if ( threat->IsVisibleInFOVNow() )
	{
		if ( IsPlayerClass( TF_CLASS_SPY ) && m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.IsStealthed() && m_iSkill > DifficultyType::NORMAL )
		{
			UpdateLookingForIncomingEnemies( false );
		}
		else
		{
			GetBodyInterface()->AimHeadTowards( threat->GetEntity(), IBody::CRITICAL, 1.0f, nullptr, "Aiming at a visible threat" );
		}
		return;
	}
	else if ( IsLineOfSightClear( threat->GetEntity(), CBaseCombatCharacter::IGNORE_ACTORS ) )
	{
		// ???
		Vector vecToThreat = threat->GetEntity()->GetAbsOrigin() - GetAbsOrigin();
		float sin, trash;
		FastSinCos( BitsToFloat( 0x3F060A92 ), &sin, &trash );
		float flAdjustment = vecToThreat.NormalizeInPlace() * sin;

		Vector vecToTurnTo = threat->GetEntity()->WorldSpaceCenter() + Vector( RandomFloat( -flAdjustment, flAdjustment ), RandomFloat( -flAdjustment, flAdjustment ), 0 );

		GetBodyInterface()->AimHeadTowards( vecToTurnTo, IBody::IMPORTANT, 1.0f, nullptr, "Turning around to find threat out of our FOV" );
		return;
	}

	if ( IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		UpdateLookingForIncomingEnemies( true );
		return;
	}

	CTFNavArea *pArea = GetLastKnownArea();
	if ( pArea )
	{
		SelectClosestPotentiallyVisible functor( threat->GetLastKnownPosition() );
		pArea->ForAllPotentiallyVisibleAreas( functor );

		if ( functor.m_pSelected )
		{
			for ( int i = 0; i < 10; ++i )
			{
				const Vector vSpot = functor.m_pSelected->GetRandomPoint() + Vector( 0, 0, HumanHeight * 0.75f );
				if ( GetVisionInterface()->IsLineOfSightClear( vSpot ) )
				{
					GetBodyInterface()->AimHeadTowards( vSpot, IBody::IMPORTANT, 1.0f, nullptr, "Looking toward potentially visible area near known but hidden threat" );
					return;
				}
			}

			DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0xFF, 0, 0xFF ), "%3.2f: %s can't find clear line to look at potentially visible near known but hidden entity %s(#%d)\n",
				gpGlobals->curtime, GetPlayerName(), threat->GetEntity()->GetClassname(), ENTINDEX( threat->GetEntity() ) );

			return;
		}
	}

	DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0xFF, 0, 0xFF ), "%3.2f: %s no potentially visible area to look toward known but hidden entity %s(#%d)\n",
		gpGlobals->curtime, GetPlayerName(), threat->GetEntity()->GetClassname(), ENTINDEX( threat->GetEntity() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateLookingForIncomingEnemies( bool enemy )
{
	if ( !m_lookForEnemiesTimer.IsElapsed() )
		return;

	m_lookForEnemiesTimer.Start( RandomFloat( 0.3f, 1.0f ) );

	CTFNavArea *area = GetLastKnownArea();
	if ( area == nullptr )
		return;

	int iTeam = enemy ? GetTeamNumber() : GetEnemyTeam( this );
	// really shouldn't happen
	if ( iTeam < 0 || iTeam > 3 )
		iTeam = 0;

	float fRange = 150.0f;
	if ( m_Shared.InCond( TF_COND_AIMING ) )
		fRange = 750.0f;

	const CUtlVector<CTFNavArea *> &areas = area->GetInvasionAreasForTeam( iTeam );
	if ( !areas.IsEmpty() )
	{
		for ( int i = 0; i < 20; ++i )
		{
			const Vector vSpot = areas.Random()->GetRandomPoint();
			if ( this->IsRangeGreaterThan( vSpot, fRange ) )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( vSpot ) )
				{
					GetBodyInterface()->AimHeadTowards( vSpot, IBody::INTERESTING, 1.0f, nullptr, "Looking toward enemy invasion areas" );
					return;
				}
			}
		}
	}

	DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0, 0, 0xFF ), "%3.2f: %s no invasion areas to look toward to predict oncoming enemies\n",
		gpGlobals->curtime, GetPlayerName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::EquipBestWeaponForThreat( const CKnownEntity *threat )
{
	if ( !threat )
		return false;

	if ( !EquipRequiredWeapon() )
	{
		CTFWeaponBase *primary = dynamic_cast<CTFWeaponBase *>( Weapon_GetSlot( TF_LOADOUT_SLOT_PRIMARY ) );
		CTFWeaponBase *secondary = dynamic_cast<CTFWeaponBase *>( Weapon_GetSlot( TF_LOADOUT_SLOT_SECONDARY ) );
		CTFWeaponBase *melee = dynamic_cast<CTFWeaponBase *>( Weapon_GetSlot( TF_LOADOUT_SLOT_MELEE ) );

		if ( !IsCombatWeapon( primary ) )
			primary = nullptr;
		if ( !IsCombatWeapon( secondary ) )
			secondary = nullptr;
		if ( !IsCombatWeapon( melee ) )
			melee = nullptr;

		CTFWeaponBase *pWeapon = primary;
		if ( !primary )
		{
			pWeapon = secondary;
			if ( !secondary )
				pWeapon = melee;
		}

		if ( m_iSkill != EASY )
		{
			if ( threat->WasEverVisible() && threat->GetTimeSinceLastSeen() <= 5.0f )
			{
				if ( GetAmmoCount( TF_AMMO_PRIMARY ) <= 0 )
					primary = nullptr;
				if ( GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
					secondary = nullptr;

				switch ( m_PlayerClass.GetClassIndex() )
				{
					case TF_CLASS_SNIPER:
						if ( secondary && IsRangeLessThan( threat->GetLastKnownPosition(), 750.0f ) )
						{
							pWeapon = secondary;
						}
						break;
					case TF_CLASS_SOLDIER:
						if ( pWeapon && pWeapon->Clip1() <= 0 )
						{
							if ( secondary && secondary->Clip1() != 0 && IsRangeLessThan( threat->GetLastKnownPosition(), 500.0f ) )
								pWeapon = secondary;
						}
						break;
					case TF_CLASS_PYRO:
						if ( secondary && IsRangeGreaterThan( threat->GetLastKnownPosition(), 750.0f ) )
						{
							pWeapon = secondary;
						}
						if ( threat->GetEntity() )
						{
							CTFPlayer *pPlayer = ToTFPlayer( threat->GetEntity() );
							if ( pPlayer )
							{
								if ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
									pWeapon = primary;
							}
						}
						break;
					case TF_CLASS_SCOUT:
						if ( pWeapon && pWeapon->Clip1() <= 0 )
						{
							pWeapon = secondary;
						}
						break;
				}
			}
		}

		if ( pWeapon )
			return Weapon_Switch( pWeapon );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Swap to a weapon our class uses for range
//-----------------------------------------------------------------------------
bool CTFBot::EquipLongRangeWeapon( void )
{	// This is so terrible
	if ( IsPlayerClass( TF_CLASS_SOLDIER ) || IsPlayerClass( TF_CLASS_DEMOMAN ) || IsPlayerClass( TF_CLASS_SNIPER ) || IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		CBaseCombatWeapon *pWeapon = Weapon_GetSlot( 0 );
		if ( pWeapon )
		{
			if ( GetAmmoCount( TF_AMMO_PRIMARY ) > 0 )
			{
				Weapon_Switch( pWeapon );
				return true;
			}
		}
	}

	CBaseCombatWeapon *pWeapon = Weapon_GetSlot( 1 );
	if ( pWeapon )
	{
		if ( GetAmmoCount( TF_AMMO_SECONDARY ) > 0 )
		{
			Weapon_Switch( pWeapon );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PushRequiredWeapon( CTFWeaponBase *weapon )
{
	CHandle<CTFWeaponBase> hndl;
	if ( weapon ) hndl.Set( weapon );

	m_requiredEquipStack.AddToTail( hndl );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PopRequiredWeapon( void )
{
	m_requiredEquipStack.RemoveMultipleFromTail( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::EquipRequiredWeapon( void )
{
	if ( m_requiredEquipStack.Count() <= 0 )
		return false;

	CHandle<CTFWeaponBase> &hndl = m_requiredEquipStack.Tail();
	CTFWeaponBase *weapon = hndl.Get();

	return Weapon_Switch( weapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsSquadmate( CTFPlayer *player ) const
{
	if ( m_pSquad == nullptr )
		return false;

	CTFBot *bot = ToTFBot( player );
	if ( bot )
		return m_pSquad == bot->m_pSquad;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::JoinSquad( CTFBotSquad *squad )
{
	if ( squad )
	{
		squad->Join( this );
		m_pSquad = squad;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::LeaveSquad( void )
{
	if ( m_pSquad )
	{
		m_pSquad->Leave( this );
		m_pSquad = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::AccumulateSniperSpots( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	SetupSniperSpotAccumulation();

	if ( m_sniperStandAreas.IsEmpty() || m_sniperLookAreas.IsEmpty() )
	{
		if ( m_sniperSpotTimer.IsElapsed() )
			ClearSniperSpots();

		return;
	}

	for ( int i=0; i<tf_bot_sniper_spot_search_count.GetInt(); ++i )
	{
		SniperSpotInfo newInfo{};
		newInfo.m_pHomeArea = m_sniperStandAreas.Random();
		newInfo.m_vecHome = newInfo.m_pHomeArea->GetRandomPoint();
		newInfo.m_pForwardArea = m_sniperLookAreas.Random();
		newInfo.m_vecForward = newInfo.m_pForwardArea->GetRandomPoint();

		newInfo.m_flRange = ( newInfo.m_vecHome - newInfo.m_vecForward ).Length();

		if ( newInfo.m_flRange < tf_bot_sniper_spot_min_range.GetFloat() )
			continue;

		if ( !IsLineOfFireClear( newInfo.m_vecHome + Vector( 0, 0, 60.0f ), newInfo.m_vecForward + Vector( 0, 0, 60.0f ) ) )
			continue;

		float flIncursion1 = newInfo.m_pHomeArea->GetIncursionDistance( GetEnemyTeam( this ) );
		float flIncursion2 = newInfo.m_pForwardArea->GetIncursionDistance( GetEnemyTeam( this ) );

		newInfo.m_flIncursionDiff = flIncursion1 - flIncursion2;

		if ( m_sniperSpots.Count() < tf_bot_sniper_spot_max_count.GetInt() )
			m_sniperSpots.AddToTail( newInfo );

		for ( int j=0; j<m_sniperSpots.Count(); ++j )
		{
			SniperSpotInfo *info = &m_sniperSpots[j];

			if ( flIncursion1 - flIncursion2 <= info->m_flIncursionDiff )
				continue;

			*info = newInfo;
		}
	}

	if ( IsDebugging( NEXTBOT_BEHAVIOR ) )
	{
		for ( int i=0; i<m_sniperSpots.Count(); ++i )
		{
			NDebugOverlay::Cross3D( m_sniperSpots[i].m_vecHome, 5.0f, 255, 0, 255, true, 0.1f );
			NDebugOverlay::Line( m_sniperSpots[i].m_vecHome, m_sniperSpots[i].m_vecForward, 0, 200, 0, true, 0.1f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SetupSniperSpotAccumulation( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CBaseEntity *pObjective = nullptr;
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *pWatcher = TFGameRules()->GetPayloadToPush( GetTeamNumber() );
		if ( !pWatcher )
		{
			pWatcher = TFGameRules()->GetPayloadToBlock( GetTeamNumber() );
			if ( !pWatcher )
			{
				ClearSniperSpots();
				return;
			}
		}

		pObjective = pWatcher->GetTrainEntity();
	}
	else
	{
		if ( TFGameRules()->GetGameType() != TF_GAMETYPE_CP )
		{
			ClearSniperSpots();
			return;
		}

		pObjective = GetMyControlPoint();
	}

	if ( pObjective == nullptr )
	{
		ClearSniperSpots();
		return;
	}

	if ( pObjective == m_sniperGoalEnt && Square( tf_bot_sniper_goal_entity_move_tolerance.GetFloat() ) > ( pObjective->WorldSpaceCenter() - m_sniperGoal ).LengthSqr() )
		return;

	ClearSniperSpots();

	const int iMyTeam = GetTeamNumber();
	const int iEnemyTeam = GetEnemyTeam( this );
	bool bCheckForward = false;
	CTFNavArea *pObjectiveArea = nullptr;

	m_sniperStandAreas.RemoveAll();
	m_sniperLookAreas.RemoveAll();

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		pObjectiveArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( pObjective->WorldSpaceCenter(), true, 500.0f ) );
		bCheckForward = iEnemyTeam != pObjective->GetTeamNumber();
	}
	else
	{
		if ( GetMyControlPoint()->GetPointIndex() >= MAX_CONTROL_POINTS )
			return;

		pObjectiveArea = TFNavMesh()->GetMainControlPointArea( GetMyControlPoint()->GetPointIndex() );
		bCheckForward = GetMyControlPoint()->GetOwner() == iMyTeam;
	}

	if ( !pObjectiveArea )
		return;

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		float flMyIncursion = area->GetIncursionDistance( iMyTeam );
		if ( flMyIncursion < 0.0f )
			continue;

		float flEnemyIncursion = area->GetIncursionDistance( iEnemyTeam );
		if ( flEnemyIncursion < 0.0f )
			continue;

		if ( flEnemyIncursion <= pObjectiveArea->GetIncursionDistance( iEnemyTeam ) )
			m_sniperLookAreas.AddToTail( area );

		if ( bCheckForward )
		{
			if ( pObjectiveArea->GetIncursionDistance( iMyTeam ) + tf_bot_sniper_spot_point_tolerance.GetFloat() >= flMyIncursion )
				m_sniperStandAreas.AddToTail( area );
		}
		else
		{
			if ( pObjectiveArea->GetIncursionDistance( iMyTeam ) - tf_bot_sniper_spot_point_tolerance.GetFloat() >= flMyIncursion )
				m_sniperStandAreas.AddToTail( area );
		}
	}

	m_sniperGoalEnt = pObjective;
	m_sniperGoal = pObjective->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::ClearSniperSpots( void )
{
	m_sniperSpots.Purge();
	m_sniperStandAreas.RemoveAll();
	m_sniperLookAreas.RemoveAll();
	m_sniperGoalEnt = nullptr;

	m_sniperSpotTimer.Start( RandomFloat( 5.0f, 10.0f ) );
}

//-----------------------------------------------------------------------------
// Purpose: Seperate ourselves with minor push forces from teammates
//-----------------------------------------------------------------------------
void CTFBot::AvoidPlayers( CUserCmd *pCmd )
{
	if ( !tf_avoidteammates.GetBool() || !tf_avoidteammates_pushaway.GetBool() )
		return;

	Vector vecFwd, vecRight;
	this->EyeVectors( &vecFwd, &vecRight );

	Vector vecAvoidCenter = vec3_origin;
	const float flRadius = 50.0;

	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers( &teammates, GetTeamNumber(), true );
	for ( int i=0; i<teammates.Count(); i++ )
	{
		if ( IsSelf( teammates[i] ) || HasTheFlag() )
			continue;

		Vector vecToTeamMate = GetAbsOrigin() - teammates[i]->GetAbsOrigin();
		if ( Square( flRadius ) > vecToTeamMate.LengthSqr() )
		{
			vecAvoidCenter += vecToTeamMate.Normalized() * ( 1.0f - ( 1.0f / flRadius ) );
		}
	}

	if ( !vecAvoidCenter.IsZero() )
	{
		vecAvoidCenter.NormalizeInPlace();

		m_Shared.SetSeparation( true );
		m_Shared.SetSeparationVelocity( vecAvoidCenter * flRadius );
		pCmd->forwardmove += vecAvoidCenter.Dot( vecFwd ) * flRadius;
		pCmd->sidemove += vecAvoidCenter.Dot( vecRight ) * flRadius;
	}
	else
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If we were assigned to take over a real player, return them
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CTFBot::GetEntity( void ) const
{
	return ToBasePlayer( m_controlling ) ? m_controlling : (CTFPlayer *)this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SelectReachableObjects( CUtlVector<EHANDLE> const &knownHealth, CUtlVector<EHANDLE> *outVector, INextBotFilter const &func, CNavArea *pStartArea, float flMaxRange )
{
	if ( !pStartArea || !outVector )
		return;

	CUtlVector<EHANDLE> selectedHealths;
	for ( int i=0; i<knownHealth.Count(); ++i )
	{
		CBaseEntity *pEntity = knownHealth[i];
		if ( !pEntity || !func.IsSelected( pEntity ) )
			continue;

		EHANDLE hndl( pEntity );
		selectedHealths.AddToTail( hndl );
	}

	outVector->RemoveAll();

	CollectReachableObjects collector( this, &selectedHealths, outVector, flMaxRange );
	SearchSurroundingAreas( pStartArea, collector, flMaxRange );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFBot::SelectRandomReachableEnemy( void )
{
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( this ), true );

	CUtlVector<CTFPlayer *> validEnemies;
	for ( int i=0; i<enemies.Count(); ++i )
	{
		CTFPlayer *pEnemy = enemies[i];
		if ( PointInRespawnRoom( pEnemy, pEnemy->WorldSpaceCenter() ) )
			continue;

		validEnemies.AddToTail( pEnemy );
	}

	if ( !validEnemies.IsEmpty() )
		return validEnemies.Random();

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Can we change class? If nested or have uber then no
//-----------------------------------------------------------------------------
bool CTFBot::CanChangeClass( void )
{
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( !GetObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE ) && !GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT ) )
			return true;

		return false;
	}

	if ( !IsPlayerClass( TF_CLASS_MEDIC ) )
		return true;

	CWeaponMedigun *medigun = GetMedigun();
	if ( !medigun )
		return true;

	return medigun->GetChargeLevel() <= 0.25f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFBot::GetNextSpawnClassname( void )
{
	typedef struct
	{
		int m_iClass;
		int m_nMinTeamSize;
		int m_nRatioTeamSize;
		int m_nMinimum;
		int m_nMaximum[ DifficultyType::MAX ];
	} ClassSelection_t;

	static ClassSelection_t defenseRoster[] = 
	{
		{ TF_CLASS_ENGINEER, 0, 4, 1, { 1, 2, 3, 3 } },
		{ TF_CLASS_SOLDIER, 0, 0, 0, { -1, -1, -1, -1 } },
		{ TF_CLASS_DEMOMAN, 0, 0, 0, { 2, 3, 3, 3 } },
		{ TF_CLASS_PYRO, 3, 0, 0, { -1, -1, -1, -1 } },
		{ TF_CLASS_HEAVYWEAPONS, 3, 0, 0, { 1, 1, 2, 2 } },
		{ TF_CLASS_MEDIC, 4, 4, 1, { 1, 1, 2, 2 } },
		{ TF_CLASS_SNIPER, 5, 0, 0, { 0, 1, 1, 1 } },
		{ TF_CLASS_SPY, 5, 0, 0, { 0, 1, 2, 2 } },
		{ TF_CLASS_UNDEFINED, 0, -1 },
	};

	static ClassSelection_t offenseRoster[] = 
	{
		{ TF_CLASS_SCOUT, 0, 0, 1, { 3, 3, 3, 3 } },
		{ TF_CLASS_SOLDIER, 0, 0, 0, { -1, -1, -1, -1 } },
		{ TF_CLASS_DEMOMAN, 0, 0, 0, { 2, 3, 3, 3 } },
		{ TF_CLASS_PYRO, 3, 0, 0, { -1, -1, -1, -1 } },
		{ TF_CLASS_HEAVYWEAPONS, 3, 0, 0, { 1, 1, 2, 2 } },
		{ TF_CLASS_MEDIC, 4, 4, 1, { 1, 1, 2, 2 } },
		{ TF_CLASS_SNIPER, 5, 0, 0, { 0, 1, 1, 1 } },
		{ TF_CLASS_SPY, 5, 0, 0, { 0, 1, 2, 2 } },
		{ TF_CLASS_ENGINEER, 5, 0, 0, { 1, 1, 1, 1 } },
		{ TF_CLASS_UNDEFINED, 0, -1 },
	};

	static ClassSelection_t compRoster[] =
	{
		{ TF_CLASS_SCOUT, 0, 0, 0, { 0, 0, 2, 2 } },
		{ TF_CLASS_SOLDIER, 0, 0, 0, { 0, 0, -1, -1 } },
		{ TF_CLASS_DEMOMAN, 0, 0, 0, { 0, 0, 2, 2 } },
		{ TF_CLASS_PYRO, 0, -1 },
		{ TF_CLASS_HEAVYWEAPONS, 3, 0, 0, { 0, 0, 2, 2 } },
		{ TF_CLASS_MEDIC, 1, 0, 1, { 0, 0, 1, 1 } },
		{ TF_CLASS_SNIPER, 0, -1 },
		{ TF_CLASS_SPY, 0, -1 },
		{ TF_CLASS_ENGINEER, 0, -1 },
		{ TF_CLASS_UNDEFINED, 0, -1 },
	};

	static auto ClassBits =[] ( int i ) {
		return ( 1 << ( i - 1 ) );
	};

	const char *szClassName = tf_bot_force_class.GetString();
	if ( !FStrEq( szClassName, "" ) )
	{
		const int iClassIdx = GetClassIndexFromString( szClassName );
		if ( iClassIdx != TF_CLASS_UNDEFINED )
			return GetPlayerClassData( iClassIdx )->m_szClassName;
	}

	if ( !CanChangeClass() )
		return m_PlayerClass.GetName();

	CountClassMembers func( this, GetTeamNumber() );
	ForEachPlayer( func );

	const ClassSelection_t *pRoster = offenseRoster;
	if ( !TFGameRules()->IsInKothMode() )
	{
		if ( TFGameRules()->GetGameType() != TF_GAMETYPE_CP )
		{
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT && GetTeamNumber() == TF_TEAM_RED )
				pRoster = defenseRoster;
		}
		else
		{
			CUtlVector<CTeamControlPoint *> attackPoints;
			TFGameRules()->CollectCapturePoints( this, &attackPoints );

			CUtlVector<CTeamControlPoint *> defensePoints;
			TFGameRules()->CollectDefendPoints( this, &defensePoints );

			if ( attackPoints.IsEmpty() && !defensePoints.IsEmpty() )
				pRoster = defenseRoster;
		}
	}
	else
	{
		CTeamControlPoint *pPoint = GetMyControlPoint();
		if ( pPoint )
		{
			if ( GetTeamNumber() == ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) )
				pRoster = defenseRoster;
		}
	}

	int iDesiredClass = TF_CLASS_UNDEFINED;
	int iAllowedClasses = 0;

	for ( int i=0; pRoster[i].m_iClass != TF_CLASS_UNDEFINED; ++i )
	{
		ClassSelection_t const *pInfo = &pRoster[i];
		if ( !TFGameRules()->CanBotChooseClass( this, pInfo->m_iClass ) )
			continue;

		if ( func.m_iTotal < pInfo->m_nMinTeamSize )
			continue;

		if ( func.m_aClassCounts[ pInfo->m_iClass ] < pInfo->m_nMinimum )
		{
			iDesiredClass = pInfo->m_iClass;
			break;
		}

		const int nMaximum = pInfo->m_nMaximum[ m_iSkill ];
		if ( nMaximum > -1 && func.m_aClassCounts[ pInfo->m_iClass ] >= nMaximum )
			continue;

		if ( pInfo->m_nRatioTeamSize > 0 )
		{
			const int nNumPerSize = func.m_iTotal / pInfo->m_nRatioTeamSize;
			const int nActualCount = func.m_aClassCounts[ pInfo->m_iClass ] - pInfo->m_nMinTeamSize;
			if ( nActualCount < nNumPerSize )
			{
				iDesiredClass = pInfo->m_iClass;
				break;
			}
		}

		iAllowedClasses |= ClassBits( pInfo->m_iClass );
	}

	if ( iAllowedClasses == 0 )
	{
		Warning( "TFBot unable to get data for desired class, defaulting to 'auto'\n" );
		return "auto";
	}

	if ( iDesiredClass == TF_CLASS_UNDEFINED )
	{
		for( int i=0; i < TF_CLASS_COUNT_ALL; ++i )
		{
			int iRandom = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
			if ( iAllowedClasses & ClassBits( iRandom ) )
			{
				iDesiredClass = iRandom;
				break;
			}
		}
	}

	if ( m_hTargetSentry )
	{
		if ( iAllowedClasses & ClassBits( TF_CLASS_DEMOMAN ) )
			iDesiredClass = TF_CLASS_DEMOMAN;
		else if ( iAllowedClasses & ClassBits( TF_CLASS_SPY ) )
			iDesiredClass = TF_CLASS_SPY;
		else if ( iAllowedClasses & ClassBits( TF_CLASS_SOLDIER ) )
			iDesiredClass = TF_CLASS_SOLDIER;
	}

	if ( iDesiredClass > TF_CLASS_UNDEFINED )
		return GetPlayerClassData( iDesiredClass )->m_szClassName;

	Warning( "TFBot unable to get data for desired class, defaulting to 'auto'\n" );
	return "auto";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetUberDeployDelayDuration( void ) const
{
	float flDelay = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, bot_medic_uber_deploy_delay_duration );
	return flDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetUberHealthThreshold( void ) const
{
	float flThreshold = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flThreshold, bot_medic_uber_health_threshold );
	return flThreshold > 0.0f ? flThreshold : 50.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Equip a random loadout for our class
//-----------------------------------------------------------------------------
void CTFBot::ManageRandomWeapons( void )
{
	// If we're in randomizer then we're already receiving random loadouts
	extern ConVar tf2v_randomizer;
	if ( tf2v_randomizer.GetBool() )
		return;

	// Are we already being handed random weapons?
	extern ConVar tf2v_random_weapons;
	if ( tf2v_random_weapons.GetBool() )
		return;

	// Are we even allowed random weapons?
	if ( !tf_bot_random_loadouts.GetBool() )
		return;

	if ( tf_bot_keep_items_after_death.GetBool() )
	{
		int nChance = tf_bot_reroll_loadout_chance.GetInt();
		if ( nChance <= RandomInt( 0, 100 ) )
			return;
	}

	Assert( GetTFInventory() );

	KeyValues *pItemSets = TFBotItemSchema()->FindKey( "itemsets" );
	FOR_EACH_TRUE_SUBKEY( pItemSets, pItemSet )
	{

	}

	for ( int i = 0, iSlot = 0; i < TF_PLAYER_WEAPON_COUNT; iSlot = ++i )
	{
		// Only allow for melee items, if we enable it or are in a special gamemode.
		if ( ( TFGameRules()->IsInDRMode() || tf2v_force_melee.GetBool() ) && ( i != TF_LOADOUT_SLOT_MELEE ) )
			continue;

		// Spy's equip slots do not correct match the weapon slot so we need to accommodate for that
		int iClass = m_PlayerClass.GetClassIndex();
		if ( iClass == TF_CLASS_SPY )
		{
			switch ( i )
			{
				case TF_LOADOUT_SLOT_PRIMARY:
					iSlot = TF_LOADOUT_SLOT_SECONDARY;
					break;
				case TF_LOADOUT_SLOT_SECONDARY:
					iSlot = TF_LOADOUT_SLOT_BUILDING;
					break;
			}
		}

		int iPreset = RandomInt( 0, GetTFInventory()->GetNumPresets( iClass, iSlot ) - 1 );
		CEconItemView *pItem = GetTFInventory()->GetItem( iClass, iSlot, iPreset );
		if ( pItem )
		{
			char szItemDefIndex[16];
			itoa( pItem->GetItemDefIndex(), szItemDefIndex, sizeof szItemDefIndex );

			float flChance = TFBotItemSchema().GetItemChance( szItemDefIndex, "drop_chance" );
			if ( ( flChance * 0.1f ) <= RandomFloat() )
				return;
		}

		// Just store in weapon preset, GiveDefaultItems will handle the rest
		m_WeaponPreset[ iClass ][ iSlot ] = iPreset;
	}
}



CTFBotPathCost::CTFBotPathCost( CTFBot *actor, RouteType routeType )
	: m_Actor( actor ), m_iRouteType( routeType )
{
	const ILocomotion *loco = m_Actor->GetLocomotionInterface();
	m_flStepHeight = loco->GetStepHeight();
	m_flMaxJumpHeight = loco->GetMaxJumpHeight();
	m_flDeathDropHeight = loco->GetDeathDropHeight();
}

float CTFBotPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( fromArea == nullptr )
	{
		// first area in path; zero cost
		return 0.0f;
	}

	if ( !m_Actor->GetLocomotionInterface()->IsAreaTraversable( area ) )
	{
		// dead end
		return -1.0f;
	}

	float fDist;
	if ( ladder != nullptr )
		fDist = ladder->m_length;
	else if ( length != 0.0f )
		fDist = length;
	else
		fDist = ( area->GetCenter() - fromArea->GetCenter() ).Length();

	const float dz = fromArea->ComputeAdjacentConnectionHeightChange( area );
	if ( dz >= m_flStepHeight )
	{
		// too high!
		if ( dz >= m_flMaxJumpHeight )
			return -1.0f;

		// jumping is slow
		fDist *= 2;
	}
	else
	{
		// yikes, this drop will hurt too much!
		if ( dz < -m_flDeathDropHeight )
			return -1.0f;
	}

	// consistently random pathing with huge cost modifier
	float fMultiplier = 1.0f;
	if ( m_iRouteType == DEFAULT_ROUTE )
	{
		const float rand = m_Actor->TransientlyConsistentRandomValue( 10.0f, 0 );
		fMultiplier += ( rand + 1.0f ) * 50.0f;
	}

	const int iOtherTeam = GetEnemyTeam( m_Actor );

	for ( int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );

		if ( obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == iOtherTeam )
		{
			obj->UpdateLastKnownArea();
			if ( area == obj->GetLastKnownArea() )
			{
				if ( m_iRouteType == SAFEST_ROUTE )
					fDist *= 5.0f;
				else if ( m_Actor->IsPlayerClass( TF_CLASS_SPY ) ) // spies always consider sentryguns to avoid
					fDist *= 10.0f;
			}
		}
	}

	// we need to be sneaky, try to take routes where no players are
	if ( m_Actor->IsPlayerClass( TF_CLASS_SPY ) )
		fDist += ( fDist * 10.0f * area->GetPlayerCount( m_Actor->GetTeamNumber() ) );

	float fCost = fDist * fMultiplier;

	if ( area->HasAttributes( NAV_MESH_FUNC_COST ) )
		fCost *= area->ComputeFuncNavCost( m_Actor );

	return fromArea->GetCostSoFar() + fCost;
}


void DifficultyChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( tf_bot_difficulty.GetInt() >= CTFBot::EASY && tf_bot_difficulty.GetInt() <= CTFBot::EXPERT )
	{
		CUtlVector<INextBot *> bots;
		TheNextBots().CollectAllBots( &bots );
		for ( int i=0; i<bots.Count(); ++i )
		{
			CTFBot *pBot = dynamic_cast<CTFBot *>( bots[i]->GetEntity() );
			if ( pBot == nullptr )
				continue;

			pBot->m_iSkill = (CTFBot::DifficultyType)tf_bot_difficulty.GetInt();
		}
	}
	else
		Warning( "tf_bot_difficulty value out of range [0,4]: %d", tf_bot_difficulty.GetInt() );
}

void PrefixNameChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	CUtlVector<INextBot *> bots;
	TheNextBots().CollectAllBots( &bots );
	for ( int i=0; i<bots.Count(); ++i )
	{
		CTFBot *pBot = dynamic_cast<CTFBot *>( bots[i]->GetEntity() );
		if ( pBot == nullptr )
			continue;

		if ( tf_bot_prefix_name_with_difficulty.GetBool() )
		{
			const char *szSkillName = DifficultyToName( pBot->m_iSkill );
			const char *szCurrentName = pBot->GetPlayerName();

			engine->SetFakeClientConVarValue( pBot->edict(), "name", CFmtStr( "%s%s", szSkillName, szCurrentName ) );
		}
		else
		{
			const char *szSkillName = DifficultyToName( pBot->m_iSkill );
			const char *szCurrentName = pBot->GetPlayerName();

			engine->SetFakeClientConVarValue( pBot->edict(), "name", &szCurrentName[Q_strlen( szSkillName )] );
		}
	}
}


CON_COMMAND_F( tf_bot_add, "Add a bot.", FCVAR_GAMEDLL )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		int count = Clamp( Q_atoi( args.Arg( 1 ) ), 1, gpGlobals->maxClients );
		for ( int i = 0; i < count; ++i )
		{
			char szBotName[64];
			if ( args.ArgC() > 4 )
				Q_snprintf( szBotName, sizeof szBotName, args.Arg( 4 ) );
			else
				V_strcpy_safe( szBotName, TheTFBots().GetRandomBotName() );

			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>( szBotName );
			if ( bot == nullptr )
				return;

			char szTeam[10];
			if ( args.ArgC() > 2 )
			{
				if ( IsTeamName( args.Arg( 2 ) ) )
					Q_snprintf( szTeam, sizeof szTeam, args.Arg( 2 ) );
				else
				{
					Warning( "Invalid argument '%s'\n", args.Arg( 2 ) );
					Q_snprintf( szTeam, sizeof szTeam, "auto" );
				}
			}
			else
				Q_snprintf( szTeam, sizeof szTeam, "auto" );

			bot->HandleCommand_JoinTeam( szTeam );

			char szClassName[16];
			if ( args.ArgC() > 3 )
			{
				if ( IsPlayerClassName( args.Arg( 3 ) ) )
					Q_snprintf( szClassName, sizeof szClassName, args.Arg( 3 ) );
				else
				{
					Warning( "Invalid argument '%s'\n", args.Arg( 3 ) );
					Q_snprintf( szClassName, sizeof szClassName, "random" );
				}
			}
			else
				Q_snprintf( szClassName, sizeof szClassName, "random" );

			bot->HandleCommand_JoinClass( szClassName );
		}

		TheTFBots().OnForceAddedBots( count );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only removes INextBots that are CTFBot derivatives with the CTFBotManager
//-----------------------------------------------------------------------------
class TFBotDestroyer
{
public:
	TFBotDestroyer( int team=TEAM_ANY ) : m_team( team ) { }

	bool operator()( CBaseCombatCharacter *bot )
	{
		if ( m_team == TEAM_ANY || bot->GetTeamNumber() == m_team )
		{
			CTFBot *pBot = ToTFBot( bot->GetBaseEntity() );
			if ( pBot == nullptr )
				return true;

			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
			TheTFBots().OnForceKickedBots( 1 );
		}

		return true;
	}

private:
	int m_team;
};

CON_COMMAND_F( tf_bot_kick, "Remove a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		const char *arg = args.Arg( 1 );
		if ( !Q_strncmp( arg, "all", 3 ) )
		{
			TFBotDestroyer func;
			TheNextBots().ForEachCombatCharacter( func );
		}
		else
		{
			CBasePlayer *pBot = UTIL_PlayerByName( arg );
			if ( pBot && pBot->IsFakeClient() )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
				TheTFBots().OnForceKickedBots( 1 );
			}
			else if ( IsTeamName( arg ) )
			{
				TFBotDestroyer func;
				if ( !Q_stricmp( arg, "red" ) )
					func = TFBotDestroyer( TF_TEAM_RED );
				else if ( !Q_stricmp( arg, "blue" ) )
					func = TFBotDestroyer( TF_TEAM_BLUE );

				TheNextBots().ForEachCombatCharacter( func );
			}
			else
			{
				Msg( "No bot or team with that name\n" );
			}
		}
	}
}

CTFBotItemSchema s_BotSchema( "TFBotItemSchema" );

// Based on https://forums.alliedmods.net/showthread.php?p=1539933
void CTFBotItemSchema::PostInit()
{
	if ( !m_pSchema )
	{
		static char const *const szChances[] = {
			"drop_chance", "vintage_chance", "genuine_chance",
			"strange_chance", "itemset_echance", "itemset_chance"
		};

		char const *pszConfigName = "cfg\\tfbot.schema.txt";
		if( TFGameRules() )
		{
			if ( TFGameRules()->IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
				pszConfigName = "cfg\\tfbot.schema.halloween.txt";
			if ( TFGameRules()->IsInMedievalMode() ) // intentionally overriding here
				pszConfigName = "cfg\\tfbot.schema.medieval.txt";
		}

		KeyValuesAD kvConfig( "items_config" );
		if ( !kvConfig->LoadFromFile( g_pFullFileSystem, pszConfigName, "GAME", true ) )
			return;

		m_pSchema = new KeyValues( "BotWeaponSchema" );

		if ( KeyValues *pSettings = kvConfig->FindKey( "settings" ) )
		{
			KeyValues *pNewKey = m_pSchema->CreateNewKey();
			pNewKey->SetName( "settings" );
			pSettings->CopySubkeys( pNewKey );
		}

		if ( KeyValues *pItemSets = kvConfig->FindKey( "itemsets" ) )
		{
			KeyValues *pNewKey = m_pSchema->CreateNewKey();
			pNewKey->SetName( "itemsets" );
			pItemSets->CopySubkeys( pNewKey );
		}

		if ( KeyValues *pItems = kvConfig->FindKey( "items" ) )
		{
			FOR_EACH_TRUE_SUBKEY( pItems, pSubKey )
			{
				char const *pszName = pSubKey->GetName();
				
				CUtlVector<char *> indexTokens, rangeTokens;
				V_SplitString( pszName, ",", indexTokens );

				for( int i=0; i < indexTokens.Count(); ++i )
				{
					if ( V_stristr( indexTokens[i], ".." ) )
					{
						V_SplitString( indexTokens[i], "..", rangeTokens );

						int iMin = atoi( rangeTokens[0] ), iMax = atoi( rangeTokens[1] );
						if( iMin <= 0 && !FStrEq( "0", rangeTokens[0] ) || iMax <= 0 && !FStrEq( "0", rangeTokens[1] ) || iMin > iMax )
						{
							Warning("Error while parsing config file: invalid range of indexes '%s'", indexTokens[i]);
							continue;
						}

						for ( int j=iMin; j <= iMax; ++j )
						{
							char szIndex[32];
							itoa( j, szIndex, sizeof szIndex );

							KeyValues *pItem = NULL;
							if ( ( pItem = m_pSchema->FindKey( szIndex ) ) != NULL )
								Warning( "Duplicate entry found in %s: '%d'", pszConfigName, j );
							else
								pItem = m_pSchema->FindKey( szIndex, true );

							if( pItem )
							{
								KeyValues *pSettings = kvConfig->FindKey( "settings" );
								for ( int k=0; k < ARRAYSIZE( szChances ); ++k )
								{
									KeyValues *pChances = pSubKey->FindKey( szChances[k] );
									if ( pChances && pChances->GetFloat( NULL, -1.f ) < 0 )
									{
										float flChance = pChances->GetFloat( "any", pSettings->GetFloat( szChances[k] ) );
										pItem->SetFloat( szChances[k], flChance );

										for ( int l=0; l < ARRAYSIZE( g_aRawPlayerClassNamesShort ); ++l )
										{
											flChance = pSubKey->GetFloat( g_aRawPlayerClassNamesShort[l], -1.f );
											if ( flChance >= 0 )
												pItem->SetFloat( CFmtStr( "%s_%s", szChances[k], g_aRawPlayerClassNamesShort[l] ), flChance );
										}
									}
									else
									{
										// assign a default value from our global settings
										float flChance = pSubKey->GetFloat( szChances[k], pSettings->GetFloat( szChances[k] ) );
										pItem->SetFloat( szChances[k], flChance );
									}
								}
							}
						}
					}
					else
					{
						int nItemIndex = atoi( indexTokens[i] );
						if ( nItemIndex <= 0 )
						{
							Warning( "Error while parsing %s: invalid item index '%d'", pszConfigName, nItemIndex );
							continue;
						}

						KeyValues *pItem = NULL;
						if ( ( pItem = m_pSchema->FindKey( indexTokens[i] ) ) != NULL )
							Warning( "Duplicate entry found in %s: '%d'", pszConfigName, nItemIndex );
						else
							pItem = m_pSchema->FindKey( indexTokens[i], true );

						if( pItem )
						{
							KeyValues *pSettings = kvConfig->FindKey( "settings" );
							for ( int j=0; j < ARRAYSIZE( szChances ); ++j )
							{
								KeyValues *pChances = pSubKey->FindKey( szChances[j] );
								if ( pChances && pChances->GetFloat( NULL, -1.f ) < 0 )
								{
									float flChance = pChances->GetFloat( "any", pSettings->GetFloat( szChances[j] ) );
									pItem->SetFloat( szChances[j], flChance );

									for ( int k=0; k < ARRAYSIZE( g_aRawPlayerClassNamesShort ); ++k )
									{
										flChance = pSubKey->GetFloat( g_aRawPlayerClassNamesShort[k], -1.f );
										if ( flChance >= 0 )
											pItem->SetFloat( CFmtStr( "%s_%s", szChances[j], g_aRawPlayerClassNamesShort[k] ), flChance );
									}
								}
								else
								{
									// assign a default value from our global settings
									float flChance = pSubKey->GetFloat( szChances[j], pSettings->GetFloat( szChances[j] ) );
									pItem->SetFloat( szChances[j], flChance );
								}
							}
						}
					}
				}
			}
		}
	}

	if ( tf_bot_debug_items.GetBool() )
	{
		m_pSchema->SaveToFile( g_pFullFileSystem, "cfg\\tfbot.schema.dump.txt", NULL, false, true );
	}
}

void CTFBotItemSchema::Shutdown()
{
	if ( m_pSchema )
	{
		m_pSchema->deleteThis();
		m_pSchema = NULL;
	}
}

float CTFBotItemSchema::GetItemChance( char const *pszItemDefIndex, char const *pszChanceName, char const *pszClassName )
{
	KeyValues *pSettings = m_pSchema->FindKey( "settings" );
	if ( KeyValues *pItem = m_pSchema->FindKey( pszItemDefIndex ) )
	{
		float flChance = pItem->GetFloat( pszChanceName, pSettings->GetFloat( pszChanceName ) );
		if ( pszClassName && !FStrEq( pszClassName, "" ) )
			return pItem->GetFloat( CFmtStr( "%s_%s", pszChanceName, pszClassName ), flChance );

		return flChance;
	}

	return pSettings->GetFloat( pszChanceName );
}

float CTFBotItemSchema::GetItemSetChance( char const *pszItemSetName )
{
	KeyValues *pSettings = m_pSchema->FindKey( "settings" );
	if ( KeyValues *pItemSet = m_pSchema->FindKey( pszItemSetName ) )
		return pItemSet->GetFloat( "chance", pSettings->GetFloat( "itemset_chance" ) );

	return pSettings->GetFloat( "itemset_chance" );
}

//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "eyeball_boss_behavior.h"
#include "eyeball_boss_emerge.h"
#include "eyeball_boss_dead.h"
#include "eyeball_boss_stun.h"

CEyeBallBossBehavior::CEyeBallBossBehavior()
{
}

const char *CEyeBallBossBehavior::GetName() const
{
	return "Behavior";
}


Action<CEyeBallBoss>* CEyeBallBossBehavior::InitialContainedAction( CEyeBallBoss *me )
{
	return new CEyeBallBossEmerge;
}

ActionResult<CEyeBallBoss> CEyeBallBossBehavior::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossBehavior::Update( CEyeBallBoss *me, float dt )
{
	return Action<CEyeBallBoss>::Continue();
}


EventDesiredResult<CEyeBallBoss> CEyeBallBossBehavior::OnInjured( CEyeBallBoss *me, const CTakeDamageInfo& info )
{
	CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
	if( !pPlayer )
		return Action<CEyeBallBoss>::TryContinue();

	if ( !pPlayer->m_purgatoryDuration.HasStarted() || pPlayer->m_purgatoryDuration.IsElapsed() || !m_stunDelay.IsElapsed() )
	{
		if ( info.GetDamageType() & DMG_VEHICLE || me->m_flDPSCounter > 300.0f )
			me->BecomeEnraged( 5.0f );

		return Action<CEyeBallBoss>::TryContinue();
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_stunned" );
	if ( event )
	{
		event->SetInt( "level", me->GetLevel() );
		event->SetInt( "player_entindex", pPlayer->entindex() );

		gameeventmanager->FireEvent( event );
	}

	me->LogPlayerInteraction( "eyeball_stunned", pPlayer );
	m_stunDelay.Start( 10.0f );

	return Action<CEyeBallBoss>::TrySuspendFor( new CEyeBallBossStunned, RESULT_IMPORTANT, "Hurt by Purgatory Buff!" );
}

EventDesiredResult<CEyeBallBoss> CEyeBallBossBehavior::OnKilled( CEyeBallBoss *me, const CTakeDamageInfo& info )
{
	for ( int i=0; i<me->m_lastAttackers.Count(); ++i )
	{
		CTFPlayer *pPlayer = me->m_lastAttackers[i].m_hPlayer;
		if ( !pPlayer || ( gpGlobals->curtime - me->m_lastAttackers[i].m_flTimeDamaged ) > 5.0f )
			continue;

		if ( me->GetTeamNumber() == TF_TEAM_NPC )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_killer" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				event->SetInt( "player_entindex", pPlayer->entindex() );

				gameeventmanager->FireEvent( event );
			}
		}

		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_VIADUCT ) && !me->m_bSpawnedWithCheats )
		{
			pPlayer->AwardAchievement( 1910 );
		}

		me->LogPlayerInteraction( "eyeball_killer", pPlayer );
	}

	UTIL_LogPrintf( "HALLOWEEN: eyeball_death (max_dps %3.2f) (max_health %d) (player_count %d) (level %d)\n",
					me->m_flDPSMax,
					me->GetMaxHealth(),
					GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers(),
					me->GetLevel() );

	return Action<CEyeBallBoss>::TryChangeTo( new CEyeBallBossDead, RESULT_CRITICAL, "I died!" );
}

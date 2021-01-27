#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_squad.h"
#include "tf_bot_manager.h"
#include "tf_bot_scenario_monitor.h"
#include "tf_bot_seek_and_destroy.h"
#include "tf_bot_roam.h"
#include "map_entities/tf_hint.h"
#include "medic/tf_bot_medic_heal.h"
#include "spy/tf_bot_spy_infiltrate.h"
#include "sniper/tf_bot_sniper_lurk.h"
#include "engineer/tf_bot_engineer_build.h"
#include "scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "scenario/capture_point/tf_bot_capture_point.h"
#include "scenario/capture_point/tf_bot_defend_point.h"
#include "scenario/payload/tf_bot_payload_push.h"
#include "scenario/payload/tf_bot_payload_guard.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"


ConVar tf_bot_fetch_lost_flag_time( "tf_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT, "How long busy TFBots will ignore the dropped flag before they give up what they are doing and go after it" );
ConVar tf_bot_flag_kill_on_touch( "tf_bot_flag_kill_on_touch", "0", FCVAR_CHEAT, "If nonzero, any bot that picks up the flag dies. For testing." );


const char *CTFBotScenarioMonitor::GetName( void ) const
{
	return "ScenarioMonitor";
}


ActionResult<CTFBot> CTFBotScenarioMonitor::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_fetchFlagDelay.Start( 20.0f );
	m_fetchFlagDuration.Invalidate();

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotScenarioMonitor::Update( CTFBot *me, float dt )
{
	if ( me->HasTheFlag(/* 0, 0 */ ) )
	{
		if ( tf_bot_flag_kill_on_touch.GetBool() )
		{
			me->CommitSuicide( false, true );
			return BaseClass::Done( "Flag kill" );
		}

		return BaseClass::SuspendFor( new CTFBotDeliverFlag, "I've picked up the flag! Running it in..." );
	}

	if ( m_fetchFlagDelay.IsElapsed() && me->IsAllowedToPickUpFlag() )
	{
		CCaptureFlag *pFlag = me->GetFlagToFetch();
		if ( pFlag == nullptr )
			return BaseClass::Continue();

		CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
		if ( pCarrier )
		{
			m_fetchFlagDuration.Invalidate();
		}
		else
		{
			if ( m_fetchFlagDuration.HasStarted() )
			{
				if ( m_fetchFlagDuration.IsElapsed() )
				{
					m_fetchFlagDuration.Invalidate();

					if ( me->MedicGetHealTarget() == nullptr )
						return BaseClass::SuspendFor( new CTFBotFetchFlag( true ), "Fetching lost flag..." );
				}
			}
			else
			{
				m_fetchFlagDuration.Start( tf_bot_fetch_lost_flag_time.GetFloat() );
			}
		}
	}

	return BaseClass::Continue();
}


Action<CTFBot> *CTFBotScenarioMonitor::InitialContainedAction( CTFBot *actor )
{
	return DesiredScenarioAndClassAction( actor );
}


Action<CTFBot> *CTFBotScenarioMonitor::DesiredScenarioAndClassAction( CTFBot *actor )
{
	if ( TheNavAreas.IsEmpty() )
		return nullptr;

	if ( actor->IsPlayerClass( TF_CLASS_SPY ) )
		return new CTFBotSpyInfiltrate;

	if ( !TheTFBots().IsMeleeOnly() )
	{
		if ( actor->IsPlayerClass( TF_CLASS_SNIPER ) )
			return new CTFBotSniperLurk;

		if ( actor->IsPlayerClass( TF_CLASS_MEDIC ) )
			return new CTFBotMedicHeal;

		if ( actor->IsPlayerClass( TF_CLASS_ENGINEER ) )
			return new CTFBotEngineerBuild;
	}

	if ( actor->GetFlagToFetch() )
		return new CTFBotFetchFlag( false );

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ARENA )
		return new CTFBotRoam;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		// TODO: PLR logic
		if ( actor->GetTeamNumber() == TF_TEAM_BLUE )
			return new CTFBotPayloadPush;
		else if ( actor->GetTeamNumber() == TF_TEAM_RED )
			return new CTFBotPayloadGuard;
	}

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		CUtlVector<CTeamControlPoint *> capture_points;
		TFGameRules()->CollectCapturePoints( actor, &capture_points );

		if ( !capture_points.IsEmpty() )
			return new CTFBotCapturePoint;

		CUtlVector<CTeamControlPoint *> defend_points;
		TFGameRules()->CollectDefendPoints( actor, &defend_points );

		if ( !defend_points.IsEmpty() )
			return new CTFBotDefendPoint;

		DevMsg( "%3.2f: %s: Gametype is CP, but I can't find a point to capture or defend!\n",  gpGlobals->curtime, actor->GetDebugIdentifier() );
		return new CTFBotCapturePoint;
	}

	return new CTFBotSeekAndDestroy;
}

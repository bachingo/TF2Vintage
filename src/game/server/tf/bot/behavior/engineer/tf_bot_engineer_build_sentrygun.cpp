#include "cbase.h"
#include "tf_bot.h"
#include "tf_obj.h"
#include "tf_weapon_builder.h"
#include "tf_bot_engineer_build_sentrygun.h"
#include "../tf_bot_get_ammo.h"


CTFBotEngineerBuildSentryGun::CTFBotEngineerBuildSentryGun( CTFBotHintSentrygun *hint )
{
	m_pHint = hint;
}

CTFBotEngineerBuildSentryGun::~CTFBotEngineerBuildSentryGun()
{
}


const char *CTFBotEngineerBuildSentryGun::GetName() const
{
	return "EngineerBuildSentryGun";
}


ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_iTries = 5;

	m_fetchAmmoTimer.Invalidate();
	// ct 0034 Invalidate()

	// 4858 = 1
	// 485c = true

	if ( m_pHint )
		m_vecTarget = m_pHint->GetAbsOrigin();
	else
		m_vecTarget = me->GetAbsOrigin();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::Update( CTFBot *me, float dt )
{
	if ( me->GetTimeSinceLastInjury() < 1.0f )
		return Action<CTFBot>::Done( "Ouch! I'm under attack" );

	if ( me->GetObjectOfType( OBJ_SENTRYGUN, 0 ) )
		return Action<CTFBot>::Done( "Sentry built" );

	if ( me->CanBuild( OBJ_SENTRYGUN, OBJECT_MODE_NONE ) == CB_NEED_RESOURCES )
	{
		if ( m_fetchAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			m_fetchAmmoTimer.Start( 1.0f );
			return Action<CTFBot>::SuspendFor( new CTFBotGetAmmo, "Need more metal to build my Sentry" );
		}
	}

	if ( me->IsRangeGreaterThan( m_vecTarget, 25.0f ) )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, m_vecTarget, cost );

			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		if ( !m_PathFollower.IsValid() )
			return Action<CTFBot>::Done( "Path failed" );

		m_PathFollower.Update( me );
		return Action<CTFBot>::Continue();
	}

	if ( m_iTries <= 0 )
		return Action<CTFBot>::Done( "Couldn't find a place to build" );

	if ( m_pHint )
	{
		CBaseObject *pSentry = (CBaseObject *)CreateEntityByName( "obj_sentrygun" );
		if ( pSentry )
		{
			m_pHint->m_nSentriesHere++;

			pSentry->SetAbsOrigin( m_pHint->GetAbsOrigin() );
			pSentry->SetAbsAngles( m_pHint->GetAbsAngles() );

			pSentry->Spawn();

			pSentry->SetBuilder( me );
			pSentry->StartBuilding( me );
		}

		return Action<CTFBot>::Continue();
	}

	CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( me->GetActiveTFWeapon() );
	if ( pBuilder && pBuilder->GetType() == OBJ_SENTRYGUN && pBuilder->m_hObjectBeingBuilt )
	{
		if ( me->GetBodyInterface()->IsHeadSteady() )
		{
			me->PressFireButton();
		}
		else
		{
			if ( m_shimmyTimer.IsElapsed() )
			{
				m_shimmyTimer.Start( RandomFloat( 0.1f, 0.25f ) );
				m_iShimmyDirection = RandomInt( 0, 3 );

				--m_iTries;
			}

			if( m_iShimmyDirection == 0 )
				me->PressForwardButton();
			else if ( m_iShimmyDirection == 1 )
				me->PressBackwardButton();
			else if ( m_iShimmyDirection == 2 )
				me->PressLeftButton();
			else if ( m_iShimmyDirection == 3 )
				me->PressRightButton();
		}
	}
	else
	{
		me->StartBuildingObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.Invalidate();
	m_recomputePathTimer.Invalidate();

	return Action<CTFBot>::Continue();
}

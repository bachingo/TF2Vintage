#include "cbase.h"
#include "tf_bot.h"
#include "tf_obj.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "nav_mesh.h"
#include "tf_bot_engineer_build_dispenser.h"
#include "../tf_bot_get_ammo.h"


class PressFireButtonIfValidBuildPositionReply : public INextBotReply
{
public:
	PressFireButtonIfValidBuildPositionReply()
		: m_pBuilder( nullptr )
	{
	}

	virtual void OnSuccess( INextBot *bot )
	{
		if ( m_pBuilder && m_pBuilder->IsValidPlacement() )
		{
			INextBotPlayerInput *input = dynamic_cast<INextBotPlayerInput *>( bot->GetEntity() );
			if ( input )
				input->PressFireButton();
		}
	}

	void SetBuilder( CTFWeaponBuilder *builder )
	{
		m_pBuilder = builder;
	}

private:
	CTFWeaponBuilder *m_pBuilder;
};


CTFBotEngineerBuildDispenser::CTFBotEngineerBuildDispenser()
{
}

CTFBotEngineerBuildDispenser::~CTFBotEngineerBuildDispenser()
{
}


const char *CTFBotEngineerBuildDispenser::GetName() const
{
	return "EngineerBuildDispenser";
}


ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_iTries = 3;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildDispenser::Update( CTFBot *me, float dt )
{
	if ( me->GetTimeSinceLastInjury() < 1.0f )
		return Action<CTFBot>::Done( "Ouch! I'm under attack" );

	CBaseObject *sentry = me->GetObjectOfType( OBJ_SENTRYGUN, 0 );
	if ( sentry == nullptr )
		return Action<CTFBot>::Done( "No Sentry" );

	if ( sentry->GetTimeSinceLastInjury() < 1.0f || sentry->GetMaxHealth() > sentry->GetHealth() )
		return Action<CTFBot>::Done( "Need to repair my Sentry" );

	if ( me->GetObjectOfType( OBJ_DISPENSER, 0 ) != nullptr )
		return Action<CTFBot>::Done( "Dispenser built" );

	if ( m_iTries <= 0 )
		return Action<CTFBot>::Done( "Can't find a place to build a Dispenser" );

	if ( me->CanBuild( OBJ_DISPENSER, OBJECT_MODE_NONE ) == CB_NEED_RESOURCES )
	{
		if ( m_fetchAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			m_fetchAmmoTimer.Start( 1.0f );
			return Action<CTFBot>::SuspendFor( new CTFBotGetAmmo, "Need more metal to build" );
		}
	}

	Vector vecDir = sentry->BodyDirection2D();
	Vector vecBehind = sentry->GetAbsOrigin() - vecDir * 75.0f;
	TheNavMesh->GetSimpleGroundHeight( vecBehind, &vecBehind.z );

	if ( ( me->GetAbsOrigin() - vecBehind ).LengthSqr() < Square( 100.0f ) )
		me->PressCrouchButton();

	if ( ( me->GetAbsOrigin() - vecBehind ).LengthSqr() > Square( 25.0f ) )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_PathFollower.Compute( me, vecBehind, cost );

			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		m_PathFollower.Update( me );
		return Action<CTFBot>::Continue();
	}

	CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( me->GetActiveTFWeapon() );
	if ( pBuilder && pBuilder->m_hObjectBeingBuilt )
	{
		if ( m_retryTimer.IsElapsed() )
		{
			m_retryTimer.Start( 1.0f );

			Vector vecToSentry = sentry->GetAbsOrigin() - me->GetAbsOrigin();
			float flLength = vecToSentry.NormalizeInPlace();

			float flSin, flCos;
			FastSinCos( RandomFloat( -M_PI_F/2, M_PI_F/2 ), &flSin, &flCos );

			float x = ( ( vecToSentry.x*flLength ) * flCos ) - ( ( vecToSentry.y*flLength ) * flSin );
			float y = ( ( vecToSentry.x*flLength ) * flSin ) + ( ( vecToSentry.y*flLength ) * flCos );

			static PressFireButtonIfValidBuildPositionReply buildReply;
			buildReply.SetBuilder( pBuilder );

			Vector vecDir( x * 100.0f, y * 100.0f, 0.0f );
			me->GetBodyInterface()->AimHeadTowards( me->EyePosition() - vecDir, IBody::CRITICAL, 0.5f, &buildReply, "Trying to place my dispenser" );

			--m_iTries;
		}
	}
	else
	{
		me->StartBuildingObjectOfType( OBJ_DISPENSER, OBJECT_MODE_NONE );
	}

	return Action<CTFBot>::Continue();
}

void CTFBotEngineerBuildDispenser::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->GetBodyInterface()->ClearPendingAimReply();
}

ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.Invalidate();
	m_recomputePathTimer.Invalidate();

	me->GetBodyInterface()->ClearPendingAimReply();

	return Action<CTFBot>::Continue();
}

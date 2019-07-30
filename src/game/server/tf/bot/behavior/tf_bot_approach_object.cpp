#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_approach_object.h"


CTFBotApproachObject::CTFBotApproachObject( CBaseEntity *object, float dist )
{
	m_hObject = object;
	m_flDist  = dist;
}

CTFBotApproachObject::~CTFBotApproachObject()
{
}


const char *CTFBotApproachObject::GetName() const
{
	return "ApproachObject";
}


ActionResult<CTFBot> CTFBotApproachObject::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotApproachObject::Update( CTFBot *me, float dt )
{
	if ( m_hObject == nullptr )
		return Action<CTFBot>::Done( "Object is NULL" );

	if ( m_hObject->IsEffectActive( EF_NODRAW ) )
		return Action<CTFBot>::Done( "Object is NODRAW" );

	if ( me->GetLocomotionInterface()->GetGround() == m_hObject )
		return Action<CTFBot>::Done( "I'm standing on the object" );

	if ( ( me->GetAbsOrigin() - m_hObject->GetAbsOrigin() ).LengthSqr() < Square( m_flDist ) )
		return Action<CTFBot>::Done( "Reached object" );

	if ( m_recomputePathTimer.IsElapsed() )
	{
		m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, m_hObject->GetAbsOrigin(), cost );
	}

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}

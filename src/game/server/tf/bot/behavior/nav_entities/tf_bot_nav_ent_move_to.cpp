#include "cbase.h"
#include "NavMeshEntities/func_nav_prerequisite.h"
#include "../../tf_bot.h"
#include "tf_bot_nav_ent_move_to.h"


CTFBotNavEntMoveTo::CTFBotNavEntMoveTo( const CFuncNavPrerequisite *prereq )
{
	m_hPrereq = prereq;
	m_GoalArea = nullptr;
}

CTFBotNavEntMoveTo::~CTFBotNavEntMoveTo()
{
}


const char *CTFBotNavEntMoveTo::GetName() const
{
	return "NavEntMoveTo";
}


ActionResult<CTFBot> CTFBotNavEntMoveTo::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if (m_hPrereq == nullptr)
		return Action<CTFBot>::Done( "Prerequisite has been removed before we started" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_waitDuration.Invalidate();

	CBaseEntity *target = m_hPrereq->GetTaskEntity();
	if (target == nullptr)
		return Action<CTFBot>::Done( "Prerequisite target entity is NULL" );

	Extent ext;
	ext.Init( target );

	m_vecGoalPos.x = ext.lo.x + RandomFloat( 0.0f, ext.SizeX() );
	m_vecGoalPos.y = ext.lo.y + RandomFloat( 0.0f, ext.SizeY() );
	m_vecGoalPos.z = ext.SizeZ() - ext.lo.z;

	TheNavMesh->GetSimpleGroundHeight( m_vecGoalPos, &m_vecGoalPos.z );
	m_GoalArea = TheNavMesh->GetNavArea( m_vecGoalPos );

	if (m_GoalArea == nullptr)
		return Action<CTFBot>::Done( "There's no nav area for the goal position" );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotNavEntMoveTo::Update( CTFBot *me, float dt )
{
	if (m_hPrereq == nullptr)
		return Action<CTFBot>::Done( "Prerequisite has been removed" );

	if (!m_hPrereq->IsEnabled())
		return Action<CTFBot>::Done( "Prerequisite has been disabled" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if (threat != nullptr && threat->IsVisibleRecently())
		me->EquipBestWeaponForThreat( threat );

	if (!m_waitDuration.HasStarted())
	{
		if (me->GetLastKnownArea() == m_GoalArea)
		{
			m_waitDuration.Start( m_hPrereq->GetTaskValue() );
		}
		else
		{
			if (m_recomputePath.IsElapsed())
			{
				m_recomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

				CTFBotPathCost func( me, FASTEST_ROUTE );
				m_PathFollower.Compute( me, m_vecGoalPos, func, 0.0f, true );
			}

			m_PathFollower.Update( me );
		}

		return Action<CTFBot>::Continue();
	}

	if (m_waitDuration.IsElapsed())
		return Action<CTFBot>::Done( "Wait duration elapsed" );

	return Action<CTFBot>::Continue();
}

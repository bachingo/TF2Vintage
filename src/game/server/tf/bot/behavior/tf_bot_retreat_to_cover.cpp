#include "cbase.h"
#include "../tf_bot.h"
#include "tf_bot_retreat_to_cover.h"
#include "nav_mesh/tf_nav_area.h"


ConVar tf_bot_retreat_to_cover_range( "tf_bot_retreat_to_cover_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_debug_retreat_to_cover( "tf_bot_debug_retreat_to_cover", "0", FCVAR_CHEAT );
ConVar tf_bot_wait_in_cover_min_time( "tf_bot_wait_in_cover_min_time", "1", FCVAR_CHEAT );
ConVar tf_bot_wait_in_cover_max_time( "tf_bot_wait_in_cover_max_time", "2", FCVAR_CHEAT );


CTFBotRetreatToCover::CTFBotRetreatToCover( Action<CTFBot> *done_action )
{
	m_flDuration = -1.0f;
	m_DoneAction = done_action;
}

CTFBotRetreatToCover::CTFBotRetreatToCover( float duration )
{
	m_flDuration = duration;
	m_DoneAction = nullptr;
}

CTFBotRetreatToCover::~CTFBotRetreatToCover()
{
}


const char *CTFBotRetreatToCover::GetName() const
{
	return "RetreatToCover";
}


ActionResult<CTFBot> CTFBotRetreatToCover::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_CoverArea = FindCoverArea( me );
	if ( m_CoverArea == nullptr )
		return Action<CTFBot>::Done( "No cover available!" );

	if ( m_flDuration < 0.0f )
	{
		m_flDuration = RandomFloat( tf_bot_wait_in_cover_min_time.GetFloat(),
									tf_bot_wait_in_cover_max_time.GetFloat() );
	}

	m_actionDuration.Start( m_flDuration );

	if ( me->IsPlayerClass( TF_CLASS_SPY ) && !me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotRetreatToCover::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );

	if ( me->m_Shared.InCond( TF_COND_INVULNERABLE ) )
		return Action<CTFBot>::Done( "I'm invulnerable - no need to reatreat!" );

	/* does this even query the right thing? */
	if ( !this->ShouldRetreat( me ) )
		return Action<CTFBot>::Done( "No longer need to retreat" );

	me->EquipBestWeaponForThreat( threat );

	CTFWeaponBase *primary = static_cast<CTFWeaponBase *>( me->Weapon_GetSlot( 0 ) );

	bool reloading = false;
	if ( primary && me->GetAmmoCount( TF_AMMO_PRIMARY ) > 0 && me->IsBarrageAndReloadWeapon( primary ) && primary->Clip1() < primary->GetMaxClip1() )
	{
		me->PressReloadButton();
		reloading = true;
	}

	if ( me->GetLastKnownArea() == m_CoverArea && threat )
	{
		m_CoverArea = FindCoverArea( me );
		if ( m_CoverArea == nullptr )
			return Action<CTFBot>::Done( "My cover is exposed, and there is no other cover available!" );
	}

	if ( threat )
	{
		m_actionDuration.Reset();

		if ( m_recomputeTimer.IsElapsed() )
		{
			m_recomputeTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			CTFBotPathCost func( me, RETREAT_ROUTE );
			m_PathFollower.Compute( me, m_CoverArea->GetCenter(), func );
		}

		m_PathFollower.Update( me );

		return Action<CTFBot>::Continue();
	}

	if ( me->IsPlayerClass( TF_CLASS_SPY ) && me->m_Shared.InCond( TF_COND_DISGUISED ) )
		return Action<CTFBot>::Continue();

	if ( me->m_Shared.IsStealthed() )
		me->PressAltFireButton();

	if ( m_DoneAction != nullptr )
		return Action<CTFBot>::ChangeTo( m_DoneAction, "Doing given action now that I'm in cover" );

	for ( int i=0; i<me->m_Shared.GetNumHealers(); ++i )
	{
		CTFPlayer *pMedic = ToTFPlayer( me->m_Shared.GetHealerByIndex( i ) );
		if ( pMedic && pMedic->MedicGetChargeLevel() > 0.9f )
			return Action<CTFBot>::Continue();
	}

	if ( !reloading && m_actionDuration.IsElapsed() )
		return Action<CTFBot>::Done( "Been in cover long enough" );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotRetreatToCover::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

class CSearchForCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCover( CTFBot *actor )
		: m_pActor( actor )
	{
		m_areas.RemoveAll();
		m_iMaxVisible = 9999;
	}

	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) OVERRIDE;

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) OVERRIDE
	{
		if ( travelDistanceSoFar > tf_bot_retreat_to_cover_range.GetFloat() )
		{
			return false;
		}

		return ( m_pActor->GetLocomotionInterface()->GetStepHeight() > currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) );
	}

	virtual void PostSearch() OVERRIDE
	{
		if ( tf_bot_debug_retreat_to_cover.GetBool() )
		{
			FOR_EACH_VEC( m_areas, i )
			{
				TheNavMesh->AddToSelectedSet( (CNavArea *)( m_areas )[i] );
			}
		}
	}

	CTFBot *m_pActor;
	CUtlVector<CTFNavArea *> m_areas;
	int m_iMaxVisible;
};
CTFNavArea *CTFBotRetreatToCover::FindCoverArea( CTFBot *actor )
{
	VPROF_BUDGET( "CTFBotRetreatToCover::FindCoverArea", "NextBot" );

	CSearchForCover functor( actor );

	if ( tf_bot_debug_retreat_to_cover.GetBool() )
		TheNavMesh->ClearSelectedSet();

	SearchSurroundingAreas( actor->GetLastKnownArea(), functor );

	if ( !functor.m_areas.IsEmpty() )
		return functor.m_areas.Random();

	return nullptr;
}


class CTestAreaAgainstThreats : public IVision::IForEachKnownEntity
{
public:
	CTestAreaAgainstThreats( CTFBot *actor, CNavArea *area )
		: m_pActor( actor ), m_pArea( area )
	{
		m_nVisible = 0;
	}

	virtual bool Inspect( const CKnownEntity& known ) OVERRIDE
	{
		VPROF_BUDGET( __FUNCTION__, "NextBot" );

		if ( m_pActor->IsEnemy( known.GetEntity() ) )
		{
			const CNavArea *lastknown = known.GetLastKnownArea();
			if ( lastknown && m_pArea->IsPotentiallyVisible( lastknown ) )
				++m_nVisible;
		}

		return true;
	}

	CTFBot *m_pActor;
	CNavArea *m_pArea;
	int m_nVisible;
};
bool CSearchForCover::operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
{
	VPROF_BUDGET( "CSearchForCover::operator()", "NextBot" );

	CTestAreaAgainstThreats functor( m_pActor, area );
	m_pActor->GetVisionInterface()->ForEachKnownEntity( functor );

	if ( functor.m_nVisible <= m_iMaxVisible )
	{
		if ( functor.m_nVisible < m_iMaxVisible )
			m_areas.RemoveAll(); // ??

		m_areas.AddToTail( (CTFNavArea *)area );
	}

	return true;
}

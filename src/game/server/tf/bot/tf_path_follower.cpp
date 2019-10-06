//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_path_follower.h"


CTFPathFollower::CTFPathFollower()
{
	m_Goal = nullptr;
	m_flMinLookAheadDistance = -1.0f;
}

CTFPathFollower::~CTFPathFollower()
{
}

void CTFPathFollower::Invalidate( void )
{
	Path::Invalidate();

	m_Goal = nullptr;
}

void CTFPathFollower::OnPathChanged( INextBot *bot, Path::ResultType result )
{
	m_Goal = FirstSegment();
	MoveCursorToStart();
}

void CTFPathFollower::Update( INextBot *bot )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ILocomotion *loco = bot->GetLocomotionInterface();

	bot->SetCurrentPath( this );

	if ( !IsValid() || m_Goal == nullptr )
		return;

	if ( loco->IsOnGround() )
	{
		if ( ( loco->GetFeet() - GetEndPosition() ).LengthSqr() <= Square( 25.0f ) )
		{
			if ( bot->IsDebugging( NEXTBOT_PATH ) )
				DevMsg( "CTFPathFollower: OnMoveToSuccess\n" );

			if ( GetAge() <= 0.0f )
				return;

			Invalidate();

			return;
		}
	}

	MoveCursorToClosestPosition( loco->GetFeet(), SEEK_AHEAD );

	const float distAlong = GetCursorPosition();
	const Path::Data &data = GetCursorData();

	if ( !data.segmentPrior )
	{
		loco->GetBot()->OnMoveToFailure( this, FAIL_STUCK );
		Invalidate();

		return;
	}

	const Path::Segment *nextSegment = NextSegment( data.segmentPrior );
	m_Goal = nextSegment;

	if ( !nextSegment )
		m_Goal = data.segmentPrior;

	Vector vecGoal = m_Goal->pos;
	Vector vecNewGoal = m_Goal->pos;

	if ( m_flMinLookAheadDistance > 0.0f )
	{
		for ( float dt = m_flMinLookAheadDistance; dt > 0.0f; dt -= 50.0f )
		{
			MoveCursor( distAlong, PATH_ABSOLUTE_DISTANCE );
			MoveCursor( dt, PATH_RELATIVE_DISTANCE );

			vecNewGoal = GetCursorData().pos;
			if ( loco->IsPotentiallyTraversable( loco->GetFeet(), vecNewGoal, ILocomotion::EVENTUALLY ) )
				break;
		}

		vecGoal = vecNewGoal;
	}

	loco->FaceTowards( vecGoal );
	loco->Approach( vecGoal );

	if ( bot->IsDebugging( NEXTBOT_PATH ) )
	{
		Path::Draw();

		NDebugOverlay::Cross3D( vecGoal, 5.0f, 150, 150, 255, true, 0.1 );
		NDebugOverlay::Line( bot->GetEntity()->WorldSpaceCenter(), vecGoal, 255, 255, 0, true, 0.1 );
	}
}

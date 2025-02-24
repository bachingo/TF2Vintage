//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mathlib/mathlib.h"
#include "tf_path_follower.h"



CTFPathFollower::CTFPathFollower()
{
	Q_memset( &m_Detour, 0, sizeof( m_Detour ) );
	m_flMinLookAheadDistance = -1.0f;
}

CTFPathFollower::~CTFPathFollower()
{
}

void CTFPathFollower::Invalidate( void )
{
	BaseClass::Invalidate();

	Q_memset( &m_Detour, 0, sizeof( m_Detour ) );
}

void CTFPathFollower::OnPathChanged( INextBot *bot, Path::ResultType result )
{
	BaseClass::OnPathChanged( bot, result );
	Q_memset( &m_Detour, 0, sizeof( m_Detour ) );
	MoveCursorToStart();
}

const Path::Segment *CTFPathFollower::NextSegment( const Path::Segment *currentSegment ) const
{
	return nullptr;
}

const Path::Segment *CTFPathFollower::PriorSegment( const Path::Segment *currentSegment ) const
{
	return nullptr;
}

void CTFPathFollower::Update( INextBot *bot )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ILocomotion *loco = bot->GetLocomotionInterface();

	bot->SetCurrentPath( this );

	if ( !IsValid() )
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

	Segment const *nextSegment = NextSegment( data.segmentPrior );
	if ( !nextSegment )
		nextSegment = data.segmentPrior;

	Vector vecGoal = nextSegment->pos;
	Vector vecNewGoal = nextSegment->pos;

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

Path::Segment const *CTFPathFollower::GetClosestSegment( Vector const &vecStart )
{
	float flMinDist = FLT_MAX;
	Segment const *pSegment = NULL;

	// First check out detour route
	// We don't check first/lastSegment due to checking whole route after this
	for ( int i = 0; i < MAX_DETOUR_LENGTH; ++i )
	{
		float flDistance = ( m_Detour.detour[i].pos - vecStart ).LengthSqr();
		if ( flDistance < flMinDist )
		{
			flMinDist = flDistance;
			pSegment = &m_Detour.detour[i];
		}
	}

	// Then our original route
	Segment const *pStart = FirstSegment();
	if ( pStart )
	{
		Segment const *pNext = Path::NextSegment( pStart ); // Skipping our override
		while ( pNext )
		{
			float flDistance = ( pNext->pos - vecStart ).LengthSqr();
			if ( flDistance < flMinDist )
			{
				flMinDist = flDistance;
				pSegment = pNext;
			}

			pNext = Path::NextSegment( pNext );
		}
	}

	return pSegment;
}

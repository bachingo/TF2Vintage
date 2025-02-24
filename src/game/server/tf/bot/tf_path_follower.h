//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_PATH_FOLLOWER_H
#define TF_PATH_FOLLOWER_H
#ifdef _WIN32
#pragma once
#endif

#include "Path/NextBotPathFollow.h"

class CTFPathFollower : public PathFollower
{
	DECLARE_CLASS_GAMEROOT( CTFPathFollower, PathFollower );
public:
	CTFPathFollower();
	virtual ~CTFPathFollower();

	virtual void Invalidate( void ) OVERRIDE;
	virtual void Update( INextBot *bot ) OVERRIDE;

	virtual void OnPathChanged( INextBot *bot, Path::ResultType result ) OVERRIDE;

	virtual const Segment *NextSegment( const Segment *currentSegment ) const OVERRIDE;		// return next segment of path, given current one
	virtual const Segment *PriorSegment( const Segment *currentSegment ) const OVERRIDE;	// return previous segment of path, given current one

	virtual void SetMinLookAheadDistance( float value ) OVERRIDE;

private:
	Segment const *GetClosestSegment( Vector const &vecStart );

	enum { MAX_DETOUR_LENGTH = 16 };
	struct Detour
	{
		Segment detour[MAX_DETOUR_LENGTH];
		Segment const *firstSegment;
		Segment const *lastSegment;
	};
	Detour m_Detour;
	EHANDLE m_hAvoidEntity;
	int m_nGoalSegment;

	float m_flMinLookAheadDistance;
};


inline void CTFPathFollower::SetMinLookAheadDistance( float value )
{
	// We need this value but so does our base
	BaseClass::SetMinLookAheadDistance( value );
	m_flMinLookAheadDistance = value;
}

#endif

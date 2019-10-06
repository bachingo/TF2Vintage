//========= Copyright © Valve LLC, All rights reserved. =======================
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
public:
	CTFPathFollower();
	virtual ~CTFPathFollower();

	virtual void Invalidate( void ) override;

	virtual void OnPathChanged( INextBot *bot, Path::ResultType result ) override;

	virtual void Update( INextBot *bot ) override;

	virtual const Path::Segment *GetCurrentGoal( void ) const override;

	virtual void SetMinLookAheadDistance( float value ) override;

private:
	const Path::Segment *m_Goal;
	float m_flMinLookAheadDistance;
};

inline const Path::Segment *CTFPathFollower::GetCurrentGoal( void ) const
{
	return m_Goal;
}

inline void CTFPathFollower::SetMinLookAheadDistance( float value )
{
	m_flMinLookAheadDistance = value;
}

#endif
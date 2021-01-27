#ifndef TF_BOT_RETREAT_TO_COVER_H
#define TF_BOT_RETREAT_TO_COVER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "NextBotVisionInterface.h"
#include "NextBotUtil.h"
#include "nav_pathfind.h"

class CTFNavArea;

class CTFBotRetreatToCover : public Action<CTFBot>
{
public:
	CTFBotRetreatToCover( Action<CTFBot> *done_action );
	CTFBotRetreatToCover( float duration = -1.0f );
	virtual ~CTFBotRetreatToCover();

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;

private:
	CTFNavArea *FindCoverArea( CTFBot *actor );

	float m_flDuration;
	Action<CTFBot> *m_DoneAction;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputeTimer;
	CTFNavArea *m_CoverArea;
	CountdownTimer m_actionDuration;
};

#endif
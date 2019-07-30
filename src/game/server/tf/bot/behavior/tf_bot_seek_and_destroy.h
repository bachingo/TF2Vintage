#ifndef TF_BOT_SEEK_AND_DESTROY_H
#define TF_BOT_SEEK_AND_DESTROY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFNavArea;

class CTFBotSeekAndDestroy : public Action<CTFBot>
{
public:
	CTFBotSeekAndDestroy( float duration = -1.0f );
	virtual ~CTFBotSeekAndDestroy();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *interruptingAction ) override;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested( CTFBot *me, int i1 ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured( CTFBot *me, int i1 ) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost( CTFBot *me, int i1 ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;

private:
	CTFNavArea *ChooseGoalArea( CTFBot *actor );
	void RecomputeSeekPath( CTFBot *actor );

	CTFNavArea *m_GoalArea;
	bool m_bPointLocked;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputeTimer;
	CountdownTimer m_actionDuration;
};

#endif
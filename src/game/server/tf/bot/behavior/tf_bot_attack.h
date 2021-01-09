#ifndef TF_BOT_ATTACK_H
#define TF_BOT_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "Path/NextBotChasePath.h"
#include "NextBotUtil.h"

class CTFBotAttack : public Action<CTFBot>
{
public:
	CTFBotAttack();
	virtual ~CTFBotAttack();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;

private:
	PathFollower m_PathFollower;
	ChasePath m_ChasePath;
	CountdownTimer m_recomputeTimer;
};

#endif
//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_PAYLOAD_BLOCK_H
#define TF_BOT_PAYLOAD_BLOCK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotPayloadPush : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotPayloadPush, Action<CTFBot> );
public:
	virtual ~CTFBotPayloadPush() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;

private:
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	// 4814 float
};

#endif

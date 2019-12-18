//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ATTACK_H
#define TF_BOT_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "NextBotUtil.h"

class CTFBotDestroyEnemySentry : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotDestroyEnemySentry, Action<CTFBot> )
public:
	virtual ~CTFBotDestroyEnemySentry() {}

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

	static bool IsPossible( CTFBot *actor );

private:
	void ComputeCornerAttackSpot( CTFBot *actor );
	void ComputeSafeAttackSpot( CTFBot *actor );

	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	bool m_bWalkToSpot;
	Vector m_vecAttackSpot;
	bool m_bFoundAttackSpot;
	bool m_bAtSafeSpot;
	bool m_bUbered;
	CHandle<CObjectSentrygun> m_hSentry;
};



class CTFBotUberAttackEnemySentry : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotUberAttackEnemySentry, Action<CTFBot> )
public:
	CTFBotUberAttackEnemySentry( CObjectSentrygun *sentry );
	virtual ~CTFBotUberAttackEnemySentry();

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

private:
	bool m_bSavedIgnoreEnemies;
	PathFollower m_PathFollower;
	CountdownTimer m_recomputePathTimer;
	CHandle<CObjectSentrygun> m_hSentry;
};


bool FindGrenadeAim( CTFBot *actor, CBaseEntity *target, float *pYaw, float *pPitch );
bool FindStickybombAim( CTFBot *actor, CBaseEntity *target, float *pYaw, float *pPitch, float *f3 );

#endif

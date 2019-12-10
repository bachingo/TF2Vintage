#ifndef TF_BOT_STICKYBOMB_SENTRY_H
#define TF_BOT_STICKYBOMB_SENTRY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotStickybombSentrygun : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotStickybombSentrygun, Action<CTFBot> );
public:
	CTFBotStickybombSentrygun( CObjectSentrygun *pSentry );
	CTFBotStickybombSentrygun( CObjectSentrygun *pSentry, float flPitch, float flYaw, float flChargePerc );
	virtual ~CTFBotStickybombSentrygun() {};

	virtual const char *GetName() const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) override;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *newAction ) override;

	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;

private:
	bool IsAimOnTarget( CTFBot *actor, float pitch, float yaw, float charge );

	float m_flDesiredPitch;
	float m_flDesiredYaw;
	float m_flDesiredCharge;
	bool m_bOpportunistic;
	bool m_bReload;
	CHandle<CObjectSentrygun> m_hSentry;
	bool m_bChargeShot;
	CountdownTimer m_aimDuration;
	bool m_bAimOnTarget;
	Vector m_vecAimTarget;
	Vector m_vecHome;
	float m_flChargeLevel;
};

#endif

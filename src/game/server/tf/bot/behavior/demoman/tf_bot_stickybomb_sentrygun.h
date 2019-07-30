#ifndef TF_BOT_STICKYBOMB_SENTRY_H
#define TF_BOT_STICKYBOMB_SENTRY_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotStickybombSentrygun : public Action<CTFBot>
{
public:
	CTFBotStickybombSentrygun( CObjectSentrygun *sentry );
	CTFBotStickybombSentrygun( CObjectSentrygun *sentry, const Vector& vec );
	virtual ~CTFBotStickybombSentrygun();

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
	bool IsAimOnTarget( CTFBot *actor, float pitch, float yaw, float speed );

	// 34 Vector/QAngle
	bool m_bOpportunistic;               // +0x40
	bool m_bReload;                      // +0x41
	CHandle<CObjectSentrygun> m_hSentry; // +0x44
	// 48 bool
	CountdownTimer m_aimDuration;       // +0x4c
	// 58 bool
	Vector m_vecAimTarget;               // +0x5c
	// 68 Vector/QAngle
	float m_flChargeLevel;               // +0x74
	// 78 dword
};

#endif

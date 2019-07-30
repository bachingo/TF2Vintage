#ifndef TF_BOT_SPY_ATTACK_H
#define TF_BOT_SPY_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "Path/NextBotChasePath.h"

class CTFBotSpyAttack : public Action<CTFBot>
{
public:
	CTFBotSpyAttack( CTFPlayer *victim=nullptr );
	virtual ~CTFBotSpyAttack();

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) override;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *ent, CGameTrace *trace ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *em ) override;
	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *blocker ) const override;

	virtual const CKnownEntity *SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const override;

private:
	CHandle<CTFPlayer> m_hVictim;
	ChasePath m_ChasePath;
	bool m_bInDanger;
	// 483c CountdownTimer - Chuckle timer in MvM
	CountdownTimer m_stealthTimer;
};

#endif
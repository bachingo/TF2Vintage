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

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *ent, CGameTrace *trace ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *em ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *blocker ) const OVERRIDE;

	virtual const CKnownEntity *SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const OVERRIDE;

private:
	CHandle<CTFPlayer> m_hVictim;
	ChasePath m_ChasePath;
	bool m_bInDanger;
	// 483c CountdownTimer - Chuckle timer in MvM
	CountdownTimer m_stealthTimer;
};

#endif
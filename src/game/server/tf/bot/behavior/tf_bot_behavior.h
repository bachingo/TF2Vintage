//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_BEHAVIOR_H
#define TF_BOT_BEHAVIOR_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotMainAction : public Action<CTFBot>
{
	DECLARE_CLASS( CTFBotMainAction, Action<CTFBot> )
public:
	virtual ~CTFBotMainAction() {}

	virtual const char *GetName( void ) const override;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) override;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) override;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *actor ) override;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *trace ) override;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) override;
	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) override;
	virtual EventDesiredResult<CTFBot> OnKilled( CTFBot *me, const CTakeDamageInfo& info ) override;
	virtual EventDesiredResult<CTFBot> OnOtherKilled( CTFBot *me, CBaseCombatCharacter *who, const CTakeDamageInfo& info ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const override;
	virtual Vector SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *them ) const override;
	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector& pos ) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const override;

private:
	void Dodge( CTFBot *actor );
	void FireWeaponAtEnemy( CTFBot *actor );
	const CKnownEntity *GetHealerOfThreat( const CKnownEntity *threat ) const;
	bool IsImmediateThreat( const CBaseCombatCharacter *who, const CKnownEntity *threat ) const;
	const CKnownEntity *SelectCloserThreat( CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const;
	const CKnownEntity *SelectMoreDangerousThreatInternal( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const;

	// TODO 34
	// TODO 38
	// TODO 3c
	// TODO 40
	// TODO 44
	CountdownTimer m_sniperAimErrorTimer;
	float m_flSniperAimError1;
	float m_flSniperAimError2;
	float m_flYawDelta;
	float m_flPreviousYaw;
	IntervalTimer m_sniperSteadyInterval;
	int m_iDesiredDisguise;
	bool m_bReloadingBarrage;
	// TODO 68 CHandle<CBaseEntity>, set in OnContact to the entity touching us
	// TODO 6c float, set in OnContact to the time when we touched a solid non-player entity
	IntervalTimer m_undergroundTimer;
};

#endif

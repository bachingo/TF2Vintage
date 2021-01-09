//========= Copyright � Valve LLC, All rights reserved. =======================
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

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual Action<CTFBot> *InitialContainedAction( CTFBot *actor ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *trace ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnInjured( CTFBot *me, const CTakeDamageInfo& info ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnKilled( CTFBot *me, const CTakeDamageInfo& info ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnOtherKilled( CTFBot *me, CBaseCombatCharacter *who, const CTakeDamageInfo& info ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;
	virtual Vector SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *them ) const OVERRIDE;
	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector& pos ) const OVERRIDE;
	virtual const CKnownEntity *SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const OVERRIDE;

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
	mutable CountdownTimer m_sniperAimErrorTimer;
	mutable float m_flSniperAimErrorRadius;
	mutable float m_flSniperAimErrorAngle;
	float m_flYawDelta;
	float m_flPreviousYaw;
	IntervalTimer m_sniperSteadyInterval;
	int m_iDesiredDisguise;
	bool m_bReloadingBarrage;
	CHandle<CBaseEntity> m_hLastTouch;
	float m_flLastTouchTime;
	IntervalTimer m_undergroundTimer;
};

#endif

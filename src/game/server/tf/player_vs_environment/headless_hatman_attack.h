//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef HEADLESS_HATMAN_ATTACK_H
#define HEADLESS_HATMAN_ATTACK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "Path/NextBotPathFollow.h"
#include "headless_hatman.h"

class CTFPlayer;


class CHeadlessHatmanAttack : public Action<CHeadlessHatman>
{
public:
	CHeadlessHatmanAttack();
	virtual ~CHeadlessHatmanAttack() { };

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CHeadlessHatman> OnStart( CHeadlessHatman *me, Action<CHeadlessHatman> *priorAction ) OVERRIDE;
	virtual ActionResult<CHeadlessHatman> Update( CHeadlessHatman *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CHeadlessHatman> OnStuck( CHeadlessHatman *me ) OVERRIDE;
	virtual EventDesiredResult<CHeadlessHatman> OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result = NULL ) OVERRIDE;

private:
	void AttackTarget( CHeadlessHatman *actor, CBaseCombatCharacter *victim, float dist );
	void SelectVictim( CHeadlessHatman *actor );
	void ValidateChaseVictim( CHeadlessHatman *actor );
	bool IsPotentiallyChaseable( CHeadlessHatman *actor, CTFPlayer *victim );
	bool IsSwingingAxe( void );
	void UpdateAxeSwing( CHeadlessHatman *actor );
	void RecomputeHomePosition( void );

	PathFollower m_PathFollower;
	Vector m_vecHome;
	CountdownTimer m_recomputeHomeTimer;
	CountdownTimer m_attackTimer;
	CountdownTimer m_attackDuration;
	CountdownTimer m_evilCackleTimer;
	CountdownTimer m_terrifyTimer;
	CountdownTimer m_notifyVictimTimer;
	CountdownTimer m_rumbleTimer;
	CHandle<CBaseCombatCharacter> m_hAimTarget;
	CHandle<CBaseCombatCharacter> m_hOldTarget;
	CountdownTimer m_chaseDuration;
	CHandle<CBaseCombatCharacter> m_hTarget;
	CountdownTimer m_forcedTargetDuration;
};

#endif
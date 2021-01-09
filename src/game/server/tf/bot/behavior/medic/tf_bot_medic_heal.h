#ifndef TF_BOT_MEDIC_HEAL_H
#define TF_BOT_MEDIC_HEAL_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"
#include "NextBotVisionInterface.h"
#include "Path/NextBotChasePath.h"

class CTFBotMedicHeal : public Action<CTFBot>
{
public:
	CTFBotMedicHeal();
	virtual ~CTFBotMedicHeal();

	virtual const char *GetName( void ) const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnActorEmoted( CTFBot *me, CBaseCombatCharacter *who, int concept ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	void ComputeFollowPosition( CTFBot *actor );
	bool IsGoodUberTarget( CTFPlayer *player ) const;
	bool IsReadyToDeployUber( CWeaponMedigun *medigun ) const;
	bool CanDeployUber( CTFBot *actor, CWeaponMedigun *medigun ) const;
	bool IsStable( CTFPlayer *player ) const;
	bool IsVisibleToEnemy( CTFBot *actor, const Vector& v1 ) const;
	CTFPlayer *SelectPatient( CTFBot *actor, CTFPlayer *old_patient );

	ChasePath m_ChasePath;
	CountdownTimer m_coverTimer;
	CountdownTimer m_changePatientTimer;
	CHandle<CTFPlayer> m_hPatient;
	Vector m_vecPatientPosition;
	CountdownTimer m_isPatientRunningTimer;
	// 0x4868 dword	--seems unused
	CountdownTimer m_delayUberTimer;
	PathFollower m_PathFollower;
	Vector m_vecFollowPosition;
};

#endif
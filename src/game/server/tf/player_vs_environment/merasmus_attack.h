//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_ATTACK_H
#define MERASMUS_ATTACK_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"
#include "Path/NextBotPathFollow.h"

class CMerasmusAttack : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusAttack, Action<CMerasmus> )
public:

	virtual ~CMerasmusAttack() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CMerasmus> OnStuck( CMerasmus *me ) OVERRIDE;
	virtual EventDesiredResult<CMerasmus> OnContact( CMerasmus *me, CBaseEntity *other, CGameTrace *result = NULL ) OVERRIDE;

private:
	void RecomputeHomePosition( void );
	bool IsPotentiallyChaseable( CMerasmus *me, CTFPlayer *pVictim );
	void SelectVictim( CMerasmus *me );

	PathFollower m_PathFollower;
	Vector m_vecHome;
	CountdownTimer m_recomputeHomeTimer;
	CountdownTimer m_staffAttackTimer;
	CountdownTimer m_grenadeTimer;
	CountdownTimer m_zapTimer;
	CountdownTimer m_bombHeadTimer;
	CountdownTimer unk11;
	CHandle<CTFPlayer> m_hTarget;
	CountdownTimer m_chooseVictimTimer;
};

#endif
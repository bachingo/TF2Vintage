//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_AOEATTACK_H
#define MERASMUS_AOEATTACK_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CTFNavArea;

class CMerasmusAOEAttack : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusAOEAttack, Action<CMerasmus> )
public:

	virtual ~CMerasmusAOEAttack() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;
	virtual void OnEnd( CMerasmus *me, Action<CMerasmus> *newAction ) OVERRIDE;

private:
	void QueueBombRingsForLaunch( CMerasmus *me );
	void QueueBombSpokesForLaunch( CMerasmus *me );

	enum
	{
		AOE_BEGIN,
		AOE_FIRING,
		AOE_END
	} m_nAttackState;

	CountdownTimer m_attackDelay;
	CountdownTimer m_attackTimer;
	CUtlVector<CTFNavArea *> m_attackAreas;
	CountdownTimer m_recomputeWanderTimer;
	CTFNavArea *m_pTargetArea;

	typedef struct
	{
		Vector velocity;
		CMerasmus *pOwner;
	} MerasmusGrenadeCreateSpec_t;
	CUtlVector<MerasmusGrenadeCreateSpec_t> m_queuedBombs;
};

#endif
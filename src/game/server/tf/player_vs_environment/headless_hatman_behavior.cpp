//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player.h"
#include "headless_hatman_behavior.h"
#include "headless_hatman_emerge.h"
#include "headless_hatman_dying.h"


CHeadlessHatmanBehavior::CHeadlessHatmanBehavior()
{
}


const char *CHeadlessHatmanBehavior::GetName( void ) const
{
	return "Behavior";
}


Action<CHeadlessHatman> *CHeadlessHatmanBehavior::InitialContainedAction( CHeadlessHatman *me )
{
	return new CHeadlessHatmanEmerge;
}


ActionResult<CHeadlessHatman> CHeadlessHatmanBehavior::Update( CHeadlessHatman *me, float dt )
{
	if (!me->IsAlive())
	{
		if (!me->DidSpawnWithCheats())
		{
			for (int i=0; i<me->m_lastAttackers.Count(); ++i)
			{
				if (!me->m_lastAttackers[i].m_hPlayer || ( gpGlobals->curtime - me->m_lastAttackers[i].m_flTimeDamaged ) > 5.0f)
					continue;

				CReliableBroadcastRecipientFilter filter;
				UTIL_SayText2Filter( filter, me->m_lastAttackers[i].m_hPlayer, false, "#TF_Halloween_Boss_Killers", me->m_lastAttackers[i].m_hPlayer->GetPlayerName() );
			}
		}

		TFGameRules()->SetIT( nullptr );

		return Action<CHeadlessHatman>::ChangeTo( new CHeadlessHatmanDying, "I am dead!" );
	}

	return Action<CHeadlessHatman>::Continue();
}

QueryResultType CHeadlessHatmanBehavior::IsPositionAllowed( const INextBot *me, const Vector& pos ) const
{
	return ANSWER_YES;
}

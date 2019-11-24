//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "eyeball_boss_notice.h"
#include "eyeball_boss_approach.h"

CEyeBallBossNotice::CEyeBallBossNotice()
{
}


const char *CEyeBallBossNotice::GetName( void ) const
{
	return "Notice";
}


ActionResult<CEyeBallBoss> CEyeBallBossNotice::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	m_chaseDelay.Start( 0.25f );

	me->EmitSound( "Halloween.EyeballBossBecomeAlert" );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossNotice::Update( CEyeBallBoss *me, float dt )
{
	CBaseCombatCharacter *pVictim = me->GetVictim();
	if ( pVictim == nullptr )
		return Action<CEyeBallBoss>::Done("Victim gone...");

	me->GetBodyInterface()->AimHeadTowards( pVictim );

	if ( m_chaseDelay.IsElapsed() )
		return Action<CEyeBallBoss>::ChangeTo( new CEyeBallBossApproachTarget, "Chasing victim" );

	return Action<CEyeBallBoss>::Continue();
}

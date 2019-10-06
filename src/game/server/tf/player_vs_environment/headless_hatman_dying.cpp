//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "headless_hatman_dying.h"


CHeadlessHatmanDying::CHeadlessHatmanDying()
{
}

const char *CHeadlessHatmanDying::GetName( void ) const
{
	return "Dying";
}

ActionResult<CHeadlessHatman> CHeadlessHatmanDying::OnStart( CHeadlessHatman *me, Action<CHeadlessHatman> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_DIESIMPLE );
	me->EmitSound( "Halloween.HeadlessBossDying" );

	return Action<CHeadlessHatman>::Continue();
}

ActionResult<CHeadlessHatman> CHeadlessHatmanDying::Update( CHeadlessHatman *me, float dt )
{
	if (me->IsSequenceFinished())
	{
		me->Break();

		extern void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity );
		DispatchParticleEffect( "halloween_boss_death", me->GetAbsOrigin(), me->GetAbsAngles(), NULL );

		UTIL_Remove( me );

		IGameEvent *event = gameeventmanager->CreateEvent( "pumpkin_lord_killed" );
		if (event)
			gameeventmanager->FireEvent( event );

		return Action<CHeadlessHatman>::Done();
	}

	return Action<CHeadlessHatman>::Continue();
}

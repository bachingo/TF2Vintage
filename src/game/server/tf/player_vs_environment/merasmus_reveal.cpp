//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "merasmus_reveal.h"
#include "merasmus_attack.h"


char const *CMerasmusReveal::GetName( void ) const
{
	return "Reveal";
}

ActionResult<CMerasmus> CMerasmusReveal::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	me->OnRevealed( false );

	me->GetBodyInterface()->StartActivity( ACT_SHIELD_UP );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusReveal::Update( CMerasmus *me, float dt )
{
	if ( me->IsSequenceFinished() )
		return ChangeTo( new CMerasmusAttack, "Here I come!" );

	return Continue();
}


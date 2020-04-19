//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "merasmus_taunt.h"


char const *CMerasmusTaunt::GetName( void ) const
{
	return "Taunt";
}


ActionResult<CMerasmus> CMerasmusTaunt::OnStart( CMerasmus *me, Action<CMerasmus> *priorAction )
{
	static char const *const s_szGestures[] ={
		"gesture_melee_cheer",
		"gesture_melee_go",
		"taunt01",
		"taunt06",
		"taunt_laugh"
	};
	const int nSequence = RandomInt( 0, ARRAYSIZE( s_szGestures ) - 1 );

	int nGesture = me->LookupSequence( s_szGestures[ nSequence ] );
	me->AddGestureSequence( nGesture );

	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 1 );

	m_tauntDuration.Start( 3.0 );

	return Continue();
}

ActionResult<CMerasmus> CMerasmusTaunt::Update( CMerasmus *me, float dt )
{
	if ( m_tauntDuration.IsElapsed() )
		return Done();

	return Continue();
}

void CMerasmusTaunt::OnEnd( CMerasmus *me, Action<CMerasmus> *newAction )
{
	int nStaff = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( nStaff, 0 );
}

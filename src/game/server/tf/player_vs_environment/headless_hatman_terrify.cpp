//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "headless_hatman_terrify.h"

extern ConVar tf_halloween_bot_terrify_radius;


CHeadlessHatmanTerrify::CHeadlessHatmanTerrify()
{
}


const char *CHeadlessHatmanTerrify::GetName( void ) const
{
	return "Terrify";
}


ActionResult<CHeadlessHatman> CHeadlessHatmanTerrify::OnStart( CHeadlessHatman *me, Action<CHeadlessHatman>* priorAction )
{
	me->AddGesture( ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1 );

	m_booDelay.Start( 0.25f );
	m_stunDelay.Start( 0.75f );
	m_actionDuration.Start( 1.25f );

	return Action<CHeadlessHatman>::Continue();
}

ActionResult<CHeadlessHatman> CHeadlessHatmanTerrify::Update( CHeadlessHatman *me, float dt )
{
	if (m_actionDuration.IsElapsed())
	{
		return Action<CHeadlessHatman>::Done();
	}
	else
	{
		if (m_booDelay.HasStarted() && m_booDelay.IsElapsed())
		{
			m_booDelay.Invalidate();
			me->EmitSound( "Halloween.HeadlessBossBoo" );
		}

		if (m_stunDelay.IsElapsed())
		{
			CUtlVector<CTFPlayer *> victims;
			CollectPlayers( &victims, TF_TEAM_RED, true );
			CollectPlayers( &victims, TF_TEAM_BLUE, true, true );

			for (int i=0; i<victims.Count(); ++i)
			{
				CTFPlayer *player = victims[i];
				if (me->IsRangeLessThan( player, tf_halloween_bot_terrify_radius.GetFloat() ))
				{
					if (IsWearingPumpkinHeadOrSaxtonMask( player ) || !me->IsAbleToSee( player, CBaseCombatCharacter::USE_FOV ))
						continue;

					player->m_Shared.StunPlayer( 2.0f, 0.0f, 0.0f, TF_STUNFLAGS_GHOSTSCARE, NULL );
				}
			}
		}
	}

	return Action<CHeadlessHatman>::Continue();
}


bool CHeadlessHatmanTerrify::IsWearingPumpkinHeadOrSaxtonMask( CTFPlayer *player )
{
	// loops through a players equipment handles and checks their item ID == 277 or 278
	return false;
}

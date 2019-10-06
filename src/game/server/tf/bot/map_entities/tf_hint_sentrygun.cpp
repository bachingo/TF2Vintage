//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_hint_sentrygun.h"


BEGIN_DATADESC( CTFBotHintSentrygun )
	DEFINE_KEYFIELD( m_isSticky, FIELD_BOOLEAN, "sticky" ),

	DEFINE_OUTPUT( m_OnSentryGunDestroyed, "OnSentryGunDestroyed" ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( bot_hint_sentrygun, CTFBotHintSentrygun );


CTFBotHintSentrygun::CTFBotHintSentrygun()
{
}

CTFBotHintSentrygun::~CTFBotHintSentrygun()
{
}


CBaseTFBotHintEntity::HintType CTFBotHintSentrygun::GetHintType() const
{
	return CBaseTFBotHintEntity::SENTRY_GUN;
}


bool CTFBotHintSentrygun::IsAvailableForSelection( CTFPlayer *player ) const
{
	if ( m_hOwner && m_hOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
		return false;
	else if ( m_isDisabled || m_nSentriesHere != 0 )
		return false;
	else
		return this->InSameTeam( player );
}


void CTFBotHintSentrygun::OnSentryGunDestroyed( CBaseEntity *ent )
{
	m_OnSentryGunDestroyed.FireOutput( ent, ent );
}

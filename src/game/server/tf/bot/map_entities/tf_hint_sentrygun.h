//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HINT_SENTRYGUN_H
#define TF_HINT_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_hint_entity.h"

class CTFBotHintSentrygun : public CBaseTFBotHintEntity
{
	DECLARE_CLASS( CTFBotHintSentrygun, CBaseTFBotHintEntity );
public:
	CTFBotHintSentrygun();
	virtual ~CTFBotHintSentrygun();

	DECLARE_DATADESC();

	virtual HintType GetHintType() const override;

	bool IsAvailableForSelection( CTFPlayer *player ) const;

	void OnSentryGunDestroyed( CBaseEntity *ent );

	bool m_isSticky;
	int m_nSentriesHere;
	COutputEvent m_OnSentryGunDestroyed;
	CHandle<CTFPlayer> m_hOwner;
};

#endif

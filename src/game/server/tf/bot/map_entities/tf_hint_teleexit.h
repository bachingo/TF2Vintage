//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HINT_TELEEXIT_H
#define TF_HINT_TELEEXIT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_hint_entity.h"

class CTFBotHintTeleporterExit : public CBaseTFBotHintEntity
{
	DECLARE_CLASS( CTFBotHintTeleporterExit, CBaseTFBotHintEntity );
public:
	CTFBotHintTeleporterExit();
	virtual ~CTFBotHintTeleporterExit();

	DECLARE_DATADESC();
	
	virtual HintType GetHintType() const override;
};

#endif

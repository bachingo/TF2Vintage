//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_hint_teleexit.h"


BEGIN_DATADESC( CTFBotHintTeleporterExit )
END_DATADESC();

LINK_ENTITY_TO_CLASS( bot_hint_teleporter_exit, CTFBotHintTeleporterExit );


CTFBotHintTeleporterExit::CTFBotHintTeleporterExit()
{
}

CTFBotHintTeleporterExit::~CTFBotHintTeleporterExit()
{
}


CBaseTFBotHintEntity::HintType CTFBotHintTeleporterExit::GetHintType() const
{
	return CBaseTFBotHintEntity::TELEPORTER_EXIT;
}

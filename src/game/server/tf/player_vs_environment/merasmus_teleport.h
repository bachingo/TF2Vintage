//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef MERASMUS_TELEPORT_H
#define MERASMUS_TELEPORT_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "merasmus.h"

class CMerasmusTeleport : public Action<CMerasmus>
{
	DECLARE_CLASS( CMerasmusTeleport, Action<CMerasmus> )
public:

	CMerasmusTeleport( bool, bool );
	virtual ~CMerasmusTeleport() {}

	virtual char const *GetName( void ) const OVERRIDE;

	virtual ActionResult<CMerasmus> OnStart( CMerasmus *me, Action<CMerasmus> *priorAction ) OVERRIDE;
	virtual ActionResult<CMerasmus> Update( CMerasmus *me, float dt ) OVERRIDE;

private:
	Vector GetTeleportPosition( CMerasmus *actor );

	enum {
		TELEPORT_OUT,
		TELEPORT_IN,
		TELEPORT_DONE
	} m_nTeleportState;
	bool m_bGoToHome;
	bool m_bPerformAOE;
};

#endif
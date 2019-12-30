//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

class C_MerasmusDancer : public CBaseAnimating
{
public:
	DECLARE_CLASS( C_MerasmusDancer, CBaseAnimating );
	DECLARE_CLIENTCLASS();

	virtual ~C_MerasmusDancer() {}
};

IMPLEMENT_CLIENTCLASS_DT( C_MerasmusDancer, DT_MerasmusDancer, CMerasmusDancer )
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( merasmus_dancer, C_MerasmusDancer );

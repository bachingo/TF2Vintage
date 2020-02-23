//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef C_MERASMUS_BOMB_EFFECT_H
#define C_MERASMUS_BOMB_EFFECT_H
#ifdef _WIN32
#pragma once
#endif

#include "c_playerattachedmodel.h"


class C_MerasmusBombEffect : public C_PlayerRelativeModel
{
	DECLARE_CLASS( C_MerasmusBombEffect, C_PlayerRelativeModel );
public:

	virtual ~C_MerasmusBombEffect() {}

	static C_MerasmusBombEffect *Create( char const *szClassName, C_TFPlayer *pOwner, Vector vecOrigin, QAngle angAngles, float flDamage, float flLifeTime, int iFlags );

	bool	Initialize( char const *szClassName, C_TFPlayer *pOwner, Vector vecOrigin, QAngle angAngles, float flDamage, float flLifeTime, int iFlags );
	void	ClientThink( void );

private:
	CNewParticleEffect *m_pBombTrail;
};

#endif

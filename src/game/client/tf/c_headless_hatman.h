#ifndef C_HEADLESS_HATMAN_H
#define C_HEADLESS_HATMAN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_halloween_boss.h"


class C_HeadlessHatman : public C_HalloweenBaseBoss
{
	DECLARE_CLASS( C_HeadlessHatman, C_HalloweenBaseBoss )
public:
	C_HeadlessHatman();
	virtual ~C_HeadlessHatman() { };
	
	DECLARE_CLIENTCLASS();
	
	virtual void Spawn( void );
	virtual void ClientThink( void );
	virtual void FireEvent(const Vector& origin, const QAngle& angle, int event, const char* options);

public:
	virtual int GetBossType( void ) const { return HEADLESS_HATMAN; }

private:
	CNewParticleEffect *m_pGlow;
	CNewParticleEffect *m_pLeftEye;
	CNewParticleEffect *m_pRightEye;

	C_HeadlessHatman( const C_HeadlessHatman& );
};

#endif
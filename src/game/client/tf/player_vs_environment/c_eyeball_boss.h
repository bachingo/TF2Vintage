#ifndef C_EYEBALL_BOSS_H
#define C_EYEBALL_BOSS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_halloween_boss.h"


class C_EyeBallBoss : public C_HalloweenBaseBoss
{
	DECLARE_CLASS( C_EyeBallBoss, C_HalloweenBaseBoss )
public:
	C_EyeBallBoss();
	virtual ~C_EyeBallBoss() { };

	enum
	{
		ATTITUDE_CALM,
		ATTITUDE_GRUMPY,
		ATTITUDE_ANGRY,
		ATTITUDE_HATEBLUE,
		ATTITUDE_HATERED
	};
	
	DECLARE_CLIENTCLASS();
	
	virtual void Spawn( void );
	virtual void ClientThink( void );
	virtual void FireEvent(const Vector& origin, const QAngle& angle, int event, const char* options);
	virtual int InternalDrawModel( int flags );
	virtual const QAngle& GetRenderAngles( void ) OVERRIDE;
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

public:
	virtual int GetBossType( void ) const { return EYEBALL_BOSS; }

	CNetworkVector( m_lookAtSpot );
	int m_attitude;

private:
	int m_attitudeParity;

	int m_iLookLeftRight;
	int m_iLookUpDown;

	QAngle m_angRender;
	
	CSmartPtr<CNewParticleEffect> m_pGlow;
	CSmartPtr<CNewParticleEffect> m_pAura;

	CMaterialReference m_TeamMaterial;

	C_EyeBallBoss( const C_EyeBallBoss& );
};

#endif
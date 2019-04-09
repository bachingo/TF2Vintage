#ifndef ENTITY_BOSSRESOURCE_H
#define ENTITY_BOSSRESOURCE_H
#ifdef _WIN32
#pragma once
#endif


#include "fmtstr.h"

#ifdef CLIENT_DLL
#define CTeleportVortex C_TeleportVortex
#endif

class CTeleportVortex : public CBaseAnimating
{
	DECLARE_CLASS( CTeleportVortex, CBaseAnimating );
public:

	CTeleportVortex();
	virtual ~CTeleportVortex();

	DECLARE_NETWORKCLASS()

	virtual void Precache( void );
	virtual void Spawn( void );

#ifdef GAME_DLL
	virtual int UpdateTransmitState( void );

	void StartTouch( CBaseEntity *pOther );
	void Touch( CBaseEntity *pOther );

	void SetupVortex( bool bGotoLoot, bool b2 );
	void VortexThink( void );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink( void );

	virtual void BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion *q, const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );
#endif

private:
	CNetworkVar( int, m_iState );

	CountdownTimer m_lifeTimeDuration;

#ifdef GAME_DLL
	DECLARE_DATADESC()

	CFmtStr m_pszTeleTarget;
	bool m_bUseTeamSpawns;
#else
	CNewParticleEffect *m_pGlowEffect;
	int m_iStateParity;
	float m_flFadeFraction;
#endif
};

#endif
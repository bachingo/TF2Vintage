//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef ENTITY_BOSSRESOURCE_H
#define ENTITY_BOSSRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CMonsterResource C_MonsterResource
#endif

class CMonsterResource : public CBaseEntity
{
	DECLARE_CLASS( CMonsterResource, CBaseEntity );
public:

	CMonsterResource();
	virtual ~CMonsterResource();

	DECLARE_NETWORKCLASS()

#ifdef GAME_DLL
	virtual void Spawn( void );
	virtual int ObjectCaps( void );
	virtual int UpdateTransmitState( void );

	void Update( void );

	void SetBossHealthPercentage( float percent );
	void SetBossStunPercentage( float percent );

	void IncrementSkillShotComboMeter( void );
	void StartSkillShotComboMeter( float duration );

	void HideBossHealthMeter( void );
	void HideBossStunMeter( void );
	void HideSkillShotComboMeter( void );
#else
	float GetBossHealthPercentage( void ) const;
	float GetBossStunPercentage( void ) const;
#endif

private:
	CNetworkVar( byte, m_iBossHealthPercentageByte );
	CNetworkVar( byte, m_iBossStunPercentageByte );
	CNetworkVar( int, m_iSkillShotCompleteCount );
	CNetworkVar( float, m_fSkillShotComboEndTime );
	CNetworkVar( int, m_iBossState );

#ifdef GAME_DLL
	DECLARE_DATADESC()
#endif
};

extern CMonsterResource *g_pMonsterResource;

#endif
//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_bossresource.h"
#ifdef CLIENT_DLL
#include "tf_hud_bosshealthmeter.h"
#endif

CMonsterResource *g_pMonsterResource = nullptr;


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RecvProxy_UpdateBossHud( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*(int *)pOut = pData->m_Value.m_Int;

	CHudBossHealthMeter *pMeter = (CHudBossHealthMeter *)gHUD.FindElement( "CHudBossHealthMeter" );
	if (pMeter)
		pMeter->Update();
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( MonsterResource, DT_MonsterResource )
BEGIN_NETWORK_TABLE( CMonsterResource, DT_MonsterResource )
#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iBossHealthPercentageByte ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBossStunPercentageByte ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSkillShotCompleteCount ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_fSkillShotComboEndTime ) ),
	SendPropInt( SENDINFO( m_iBossState ) ),
#else
	RecvPropInt( RECVINFO( m_iBossHealthPercentageByte ), 0, RecvProxy_UpdateBossHud ),
	RecvPropInt( RECVINFO( m_iBossStunPercentageByte ) ),
	RecvPropInt( RECVINFO( m_iSkillShotCompleteCount ) ),
	RecvPropTime( RECVINFO( m_fSkillShotComboEndTime ) ),
	RecvPropInt( RECVINFO( m_iBossState ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CMonsterResource )

	DEFINE_FIELD( m_iBossHealthPercentageByte, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBossStunPercentageByte, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSkillShotCompleteCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_fSkillShotComboEndTime, FIELD_TIME ),

	DEFINE_THINKFUNC( Update )

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( monster_resource, CMonsterResource );


CMonsterResource::CMonsterResource()
{
	g_pMonsterResource = this;
}

CMonsterResource::~CMonsterResource()
{
	Assert( g_pMonsterResource == this );
	g_pMonsterResource = NULL;
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::Spawn( void )
{
	SetThink( &CMonsterResource::Update );
	SetNextThink( gpGlobals->curtime );

	m_iBossHealthPercentageByte = 0;
	m_iBossStunPercentageByte = 0;
	m_iSkillShotCompleteCount = 0;
	m_fSkillShotComboEndTime = 0;
	m_iBossState = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMonsterResource::ObjectCaps( void )
{
	return ( BaseClass::ObjectCaps() | FCAP_DONT_SAVE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMonsterResource::UpdateTransmitState( void )
{
	return BaseClass::SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::Update( void )
{
	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::SetBossHealthPercentage( float percent )
{
	m_iBossHealthPercentageByte = RoundFloatToByte( 255.0f * percent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::SetBossStunPercentage( float percent )
{
	m_iBossStunPercentageByte = RoundFloatToByte( 255.0f * percent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::IncrementSkillShotComboMeter( void )
{
	m_iSkillShotCompleteCount += 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::StartSkillShotComboMeter( float duration )
{
	m_fSkillShotComboEndTime = duration + gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::HideBossHealthMeter( void )
{
	m_iBossHealthPercentageByte = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::HideBossStunMeter( void )
{
	m_iBossStunPercentageByte = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMonsterResource::HideSkillShotComboMeter( void )
{
	m_iSkillShotCompleteCount = 0;
}

#else // GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CMonsterResource::GetBossHealthPercentage( void ) const
{
	return m_iBossHealthPercentageByte / 255.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CMonsterResource::GetBossStunPercentage( void ) const
{
	return m_iBossStunPercentageByte / 255.0f;
}

#endif

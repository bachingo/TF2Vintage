//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//
#include "cbase.h"
#include "tf_halloween_boss.h"


class C_Merasmus : public C_HalloweenBaseBoss
{
	DECLARE_CLASS( C_Merasmus, C_HalloweenBaseBoss );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_Merasmus();
	virtual ~C_Merasmus();

	virtual Vector	GetObserverCamOrigin( void ) OVERRIDE	{ return EyePosition(); }

	virtual void	Spawn( void );
	virtual int		GetSkin( void );

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual int		GetBossType( void ) const				{ return MERASMUS; }

private:
	bool m_bRevealedParity;
	bool m_bDoingAOEAttackParity;
	bool m_bStunnedParity;
	
	CNetworkVar( bool, m_bRevealed );
	CNetworkVar( bool, m_bDoingAOEAttack );
	CNetworkVar( bool, m_bStunned );
	
	HPARTICLEFFECT m_pBodyAura;
	HPARTICLEFFECT m_pBookParticle;
	HPARTICLEFFECT m_pStunnedParticle;
};


IMPLEMENT_CLIENTCLASS_DT( C_Merasmus, DT_Merasmus, CMerasmus )
	RecvPropBool( RECVINFO( m_bRevealed ) ),
	RecvPropBool( RECVINFO( m_bDoingAOEAttack ) ),
	RecvPropBool( RECVINFO( m_bStunned ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_Merasmus )
END_PREDICTION_DATA()


C_Merasmus::C_Merasmus()
{
	m_pBodyAura = NULL;
	m_pBookParticle = NULL;
	m_pStunnedParticle = NULL;
}

C_Merasmus::~C_Merasmus()
{
	if ( m_pBodyAura )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pBodyAura );
		m_pBodyAura = NULL;
	}

	if ( m_pBookParticle )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pBookParticle );
		m_pBookParticle = NULL;
	}

	if ( m_pStunnedParticle )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pStunnedParticle );
		m_pStunnedParticle = NULL;
	}
}

void C_Merasmus::Spawn( void )
{
	BaseClass::Spawn();

	SetViewOffset( Vector{0, 0, 100.f} );
}

int C_Merasmus::GetSkin( void )
{
	if ( m_bDoingAOEAttack )
		return 0;

	if ( m_bStunned )
		return 0;

	return 1;
}

void C_Merasmus::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bRevealedParity = m_bRevealed;
	m_bDoingAOEAttackParity = m_bDoingAOEAttack;
	m_bStunnedParity = m_bStunned;
}

void C_Merasmus::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bRevealed != m_bRevealedParity )
	{
		if ( m_bRevealed )
		{
			if( !m_pBodyAura )
			{
				m_pBodyAura = ParticleProp()->Create( "merasmus_ambient_body", PATTACH_ABSORIGIN_FOLLOW );
			}
		}
		else if ( m_pBodyAura )
		{
			ParticleProp()->StopEmission( m_pBodyAura );
			m_pBodyAura = nullptr;
		}
	}

	if ( m_bStunned != m_bStunnedParity )
	{
		if ( m_bStunned )
		{
			if ( !m_pStunnedParticle )
			{
				int iHeadBone = LookupAttachment( "head" );
				m_pStunnedParticle = ParticleProp()->Create( "merasmus_dazed", PATTACH_POINT_FOLLOW, iHeadBone );
			}
		}
		else if ( m_pStunnedParticle )
		{
			ParticleProp()->StopEmission( m_pStunnedParticle );
			m_pStunnedParticle = nullptr;
		}
	}

	if ( m_bDoingAOEAttack != m_bDoingAOEAttackParity )
	{
		if ( m_bDoingAOEAttack )
		{
			if ( !m_pBookParticle )
			{
				int iHandBone = LookupAttachment( "effect_hand_R" );
				m_pBookParticle = ParticleProp()->Create( "merasmus_book_attack", PATTACH_POINT_FOLLOW, iHandBone );
			}
		}
		else if ( m_pBookParticle )
		{
			ParticleProp()->StopEmission( m_pBookParticle );
			m_pBookParticle = nullptr;
		}
	}
}

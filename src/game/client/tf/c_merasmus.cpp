//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
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
	virtual ~C_Merasmus() {}

	virtual Vector	GetObserverCamOrigin( void ) override	{ return EyePosition(); }

	virtual void	Spawn( void );
	virtual int		GetSkin( void );

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual int		GetBossType( void ) const				{ return MERASMUS; }

private:
	bool m_bRevealedParity;
	bool m_bRevealed;
	bool m_bDoingAOEAttack;
	bool m_bDoingAOEAttackParity;
	bool m_bStunned;
	bool m_bStunnedParity;

	CSmartPtr<CNewParticleEffect> m_pBodyAura;
	CSmartPtr<CNewParticleEffect> m_pBookParticle;
	CSmartPtr<CNewParticleEffect> m_pStunnedParticle;
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
	UseClientSideAnimation();

	m_pBodyAura = NULL;
	m_pBookParticle = NULL;
	m_pStunnedParticle = NULL;
}

void C_Merasmus::Spawn( void )
{
	BaseClass::Spawn();

	SetViewOffset( Vector{0, 0, 100.f} );
}

int C_Merasmus::GetSkin( void )
{
	if ( m_bDoingAOEAttack )
		return 1;

	if ( m_bStunned )
		return 1;

	return 0;
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
			if( m_pBodyAura == nullptr )
			{
				m_pBodyAura = ParticleProp()->Create( "merasmus_ambient_body", PATTACH_ABSORIGIN_FOLLOW );
			}
		}
		else if ( m_pBodyAura.IsValid() )
		{
			ParticleProp()->StopEmission( m_pBodyAura.GetObject() );
			m_pBodyAura = nullptr;
		}
	}

	if ( m_bStunned != m_bStunnedParity )
	{
		if ( m_bStunned )
		{
			if ( m_pStunnedParticle == nullptr )
			{
				int iHeadBone = LookupAttachment( "head" );
				m_pStunnedParticle = ParticleProp()->Create( "merasmus_dazed", PATTACH_POINT_FOLLOW, iHeadBone );
			}
		}
		else if ( m_pStunnedParticle.IsValid() )
		{
			ParticleProp()->StopEmission( m_pStunnedParticle.GetObject() );
			m_pStunnedParticle = nullptr;
		}
	}

	if ( m_bDoingAOEAttack != m_bDoingAOEAttackParity )
	{
		if ( m_bDoingAOEAttack )
		{
			if ( m_pBookParticle == nullptr )
			{
				int iHandBone = LookupAttachment( "effect_hand_R" );
				m_pBookParticle = ParticleProp()->Create( "merasmus_book_attack", PATTACH_POINT_FOLLOW, iHandBone );
			}
		}
		else if ( m_pBookParticle.IsValid() )
		{
			ParticleProp()->StopEmission( m_pBookParticle.GetObject() );
			m_pBookParticle = nullptr;
		}
	}
}

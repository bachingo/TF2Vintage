#include "cbase.h"
#include "c_tf_player.h"
#include "c_merasmus_bomb_effect.h"


C_MerasmusBombEffect *C_MerasmusBombEffect::Create( char const *szModelName, C_TFPlayer *pOwner, Vector vecOffset, QAngle angOffset, float flMovementScale, float flLifeTime, int iFlags )
{
	C_MerasmusBombEffect *pEffect = new C_MerasmusBombEffect;
	if( pEffect == nullptr )
		return nullptr;

	if ( !pEffect->Initialize( szModelName, pOwner, vecOffset, angOffset, flMovementScale, flLifeTime, iFlags ) )
		return nullptr;

	return pEffect;
}

bool C_MerasmusBombEffect::Initialize( char const *szModelName, C_TFPlayer *pOwner, Vector vecOffset, QAngle angOffset, float flMovementScale, float flLifeTime, int iFlags )
{
	if ( !BaseClass::Initialize( szModelName, pOwner, vecOffset, angOffset, flMovementScale, flLifeTime, iFlags ) )
		return false;
	
	if ( m_pBombTrail )
	{
		m_pBombTrail->StopEmission();
		m_pBombTrail = nullptr;
	}

	m_pBombTrail = ParticleProp()->Create( "bombonomicon_spell_trail", PATTACH_ABSORIGIN_FOLLOW );
	if ( m_pBombTrail )
		ParticleProp()->AddControlPoint( m_pBombTrail, 1, pOwner, PATTACH_POINT_FOLLOW, "head" );

	return true;
}

void C_MerasmusBombEffect::ClientThink( void )
{
	if ( GetOwnerEntity() == nullptr )
	{
		if ( m_pBombTrail )
		{
			m_pBombTrail->StopEmission();
			m_pBombTrail = nullptr;
		}
	}

	if ( m_flExpiresAt != PAM_PERMANENT && m_flExpiresAt < gpGlobals->curtime )
	{
		if ( m_pBombTrail )
		{
			m_pBombTrail->StopEmission();
			m_pBombTrail = nullptr;
		}
	}

	BaseClass::ClientThink();
}

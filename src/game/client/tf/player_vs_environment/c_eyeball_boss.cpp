#include "cbase.h"
#include "c_eyeball_boss.h"


IMPLEMENT_CLIENTCLASS_DT( C_EyeBallBoss, DT_EyeBallBoss, CEyeBallBoss )
	RecvPropVector( RECVINFO( m_lookAtSpot ) ),
	RecvPropInt( RECVINFO( m_attitude ) ),
END_RECV_TABLE()


C_EyeBallBoss::C_EyeBallBoss()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_EyeBallBoss::Spawn( void )
{
	BaseClass::Spawn();

	m_iLookLeftRight = -1;
	m_iLookUpDown = -1;
	m_angRender = vec3_angle;
	m_attitudeParity = m_attitude = 0;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Update where we're looking
//-----------------------------------------------------------------------------
void C_EyeBallBoss::ClientThink( void )
{
	if ( m_iLookLeftRight < 0 )
		m_iLookLeftRight = LookupPoseParameter( "left_right" );
	if ( m_iLookUpDown < 0 )
		m_iLookUpDown = LookupPoseParameter( "up_down" );

	Vector vecTo = m_lookAtSpot - WorldSpaceCenter();
	vecTo.NormalizeInPlace();

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( m_angRender, &vecFwd, &vecRight, &vecUp );

	vecFwd += vecTo * 3.0f * gpGlobals->frametime;
	vecFwd.NormalizeInPlace();

	QAngle vecAng;
	VectorAngles( vecFwd, vecAng );

	SetAbsAngles( vecAng );
	m_angRender = vecAng;

	if ( m_iLookLeftRight >= 0 )
		SetPoseParameter( m_iLookLeftRight, vecFwd.Dot( vecRight ) * -50.0f );

	if ( m_iLookUpDown >= 0 )
		SetPoseParameter( m_iLookUpDown, vecFwd.Dot( vecUp ) * -50.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Stub out this call
//-----------------------------------------------------------------------------
void C_EyeBallBoss::FireEvent( const Vector& origin, const QAngle& angle, int event, const char *options )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int C_EyeBallBoss::InternalDrawModel( int flags )
{
	if ( GetTeamNumber() != TF_TEAM_RED && GetTeamNumber() != TF_TEAM_BLUE )
		return BaseClass::InternalDrawModel( flags );

	modelrender->ForcedMaterialOverride( m_TeamMaterial );
	int ret = BaseClass::InternalDrawModel( flags );
	modelrender->ForcedMaterialOverride( NULL );

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const QAngle& C_EyeBallBoss::GetRenderAngles( void )
{
	return m_angRender;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_EyeBallBoss::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_attitudeParity = m_attitude;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_EyeBallBoss::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				m_pAura = ParticleProp()->Create( "eyeboss_team_red", PATTACH_ABSORIGIN_FOLLOW );
				break;
			case TF_TEAM_BLUE:
				m_pAura = ParticleProp()->Create( "eyeboss_team_blue", PATTACH_ABSORIGIN_FOLLOW );
				break;
			default:
				m_pAura = ParticleProp()->Create( "eyeboss_aura_calm", PATTACH_ABSORIGIN_FOLLOW );
				m_pGlow = ParticleProp()->Create( "ghost_pumpkin", PATTACH_ABSORIGIN_FOLLOW );
				break;
		}

		m_TeamMaterial.Shutdown();
	}
	else
	{
		if ( GetTeamNumber() == TF_TEAM_NPC && m_attitude != m_attitudeParity )
		{
			if ( m_pAura.IsValid() )
			{
				ParticleProp()->StopEmission( m_pAura.GetObject() );
				m_pAura = NULL;
			}

			switch ( m_attitude )
			{
				case ATTITUDE_CALM:
					m_pAura = ParticleProp()->Create( "eyeboss_aura_calm", PATTACH_ABSORIGIN_FOLLOW );
					break;
				case ATTITUDE_GRUMPY:
					m_pAura = ParticleProp()->Create( "eyeboss_aura_grumpy", PATTACH_ABSORIGIN_FOLLOW );
					break;
				case ATTITUDE_ANGRY:
					m_pAura = ParticleProp()->Create( "eyeboss_aura_angry", PATTACH_ABSORIGIN_FOLLOW );
					break;
				default:
					break;
			}

			m_attitudeParity = m_attitude;
		}
	}
}


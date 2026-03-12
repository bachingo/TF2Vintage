#include "cbase.h"
#include "c_headless_hatman.h"


IMPLEMENT_CLIENTCLASS_DT( C_HeadlessHatman, DT_HeadlessHatman, CHeadlessHatman )
END_RECV_TABLE()


C_HeadlessHatman::C_HeadlessHatman()
{
}


void C_HeadlessHatman::Spawn( void )
{
	BaseClass::Spawn();

	if (!m_pGlow)
		m_pGlow = ParticleProp()->Create( "ghost_pumpkin", PATTACH_ABSORIGIN_FOLLOW );

	SetNextClientThink( gpGlobals->curtime + 1.0f );
}

void C_HeadlessHatman::ClientThink( void )
{
	if (!m_pLeftEye)
		m_pLeftEye = ParticleProp()->Create( "halloween_boss_eye_glow", PATTACH_POINT_FOLLOW, "lefteye" );
	if (!m_pRightEye)
		m_pRightEye = ParticleProp()->Create( "halloween_boss_eye_glow", PATTACH_POINT_FOLLOW, "righteye" );

	SetNextClientThink( CLIENT_THINK_NEVER );
}

void C_HeadlessHatman::FireEvent( const Vector& origin, const QAngle& angle, int event, const char *options )
{
	if (event == 7001) // footsteps
	{
		EmitSound( "Halloween.HeadlessBossFootfalls" );
		ParticleProp()->Create( "halloween_boss_foot_impact", PATTACH_ABSORIGIN );
	}

	BaseClass::FireEvent( origin, angle, event, options );
}

//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NextBot.h"
#include "merasmus_components.h"
#include "merasmus.h"


ConVar tf_merasmus_speed( "tf_merasmus_speed", "600", FCVAR_CHEAT );
// Seems unused
ConVar tf_merasmus_speed_recovery_rate( "tf_merasmus_speed_recovery_rate", "100", FCVAR_CHEAT, "Movement units/second" );

#define MERASMUS_AERIAL_SPEED		250.f
#define MERASMUS_AERIAL_TURN_SPEED	100.f

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusLocomotion::Update( void )
{
	CMerasmus *pActor = assert_cast<CMerasmus *>( GetBot()->GetEntity() );

	// Only perform locomotion if we're on the ground
	if ( !pActor->m_bDoingAOEAttack )
		BaseClass::Update();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusLocomotion::GetRunSpeed( void ) const
{
	return tf_merasmus_speed.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmusLocomotion::ShouldCollideWith( const CBaseEntity *other ) const
{
	if ( other == nullptr )
		return false;

	if ( other->IsPlayer() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::Update( void )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();

	MaintainAltitude();

	const float flLength = m_localVelocity.NormalizeInPlace();
	m_vecMotion = m_localVelocity;

	m_verticalSpeed = m_localVelocity.z * flLength;

	const float flUpdateRate = GetUpdateInterval();
	m_localVelocity.x += ( m_wishVelocity.x - m_localVelocity.x ) * flUpdateRate;
	m_localVelocity.y += ( m_wishVelocity.y - m_localVelocity.y ) * flUpdateRate;
	m_localVelocity.z += ( m_wishVelocity.z - m_localVelocity.z ) * flUpdateRate;

	pActor->SetAbsVelocity( m_localVelocity );

	Vector vecOrigin = pActor->GetAbsOrigin();
	pActor->SetAbsOrigin( vecOrigin + m_localVelocity * flUpdateRate );

	m_wishVelocity = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();
	Vector vecOrigin = pActor->GetAbsOrigin();

	float flHeight = vecOrigin.z;
	TheNavMesh->GetSimpleGroundHeight( vecOrigin, &flHeight );

	m_wishVelocity.z += Clamp( GetDesiredAltitude() - ( vecOrigin.z - flHeight ), -MERASMUS_AERIAL_SPEED, MERASMUS_AERIAL_SPEED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::Reset( void )
{
	m_verticalSpeed = 0;
	m_localVelocity = vec3_origin;
	m_wishVelocity = vec3_origin;
	m_desiredAltitude = 50.f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmusFlyingLocomotion::ShouldCollideWith( const CBaseEntity *other ) const
{
	if ( other == nullptr )
		return false;

	if ( other->IsPlayer() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetStepHeight( void ) const
{
	return 50.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetMaxJumpHeight( void ) const
{
	return 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetDeathDropHeight( void ) const
{
	return 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::Approach( const Vector& goalPos, float goalWeight )
{
	Vector vecGoal = goalPos - GetBot()->GetEntity()->GetAbsOrigin();
	Vector vecVelocity = vecGoal.Normalized() * MERASMUS_AERIAL_SPEED;

	m_wishVelocity.x += vecVelocity.x;
	m_wishVelocity.y += vecVelocity.y;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::FaceTowards( const Vector &target )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();

	QAngle angRotation = pActor->GetLocalAngles();
	const float flUpdateRate = GetUpdateInterval();

	// Pick the quickest direction to face our target
	Vector vecDirection = target - GetFeet();
	const float flYaw = UTIL_VecToYaw( vecDirection );
	const float flDifference = UTIL_AngleDiff( flYaw, angRotation[YAW] );
	
	const float flSpeed = MERASMUS_AERIAL_TURN_SPEED * flUpdateRate;
	if ( -flSpeed > flDifference )
		angRotation.y -= flSpeed;
	else if ( flSpeed < flDifference )
		angRotation.y += flSpeed;
	else
		angRotation.y += flDifference;

	pActor->SetLocalAngles( angRotation );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CMerasmusFlyingLocomotion::GetGroundNormal( void ) const
{
	static const Vector up( 0, 0, 1.0f );
	return up;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CMerasmusFlyingLocomotion::GetVelocity( void ) const
{
	return m_localVelocity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetDesiredSpeed( void ) const
{
	return tf_merasmus_speed.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CMerasmusFlyingLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusFlyingLocomotion::SetDesiredAltitude( float fHeight )
{
	m_desiredAltitude = fHeight;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMerasmusBody::Update( void )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();
	if ( m_iMoveX < 0 )
		m_iMoveX = pActor->LookupPoseParameter( "move_x" );
	if ( m_iMoveY < 0 )
		m_iMoveY = pActor->LookupPoseParameter( "move_y" );

	float flSpeed = GetBot()->GetLocomotionInterface()->GetGroundSpeed();
	if ( flSpeed >= 0.01f ) // only animate if moving enough
	{
		Vector vecFwd, vecRight;
		pActor->GetVectors( &vecFwd, &vecRight, nullptr );

		Vector vecDir = GetBot()->GetLocomotionInterface()->GetGroundMotionVector();

		if ( m_iMoveX >= 0 )
			pActor->SetPoseParameter( m_iMoveX, vecDir.Dot( vecFwd ) );
		if ( m_iMoveY >= 0 )
			pActor->SetPoseParameter( m_iMoveY, vecDir.Dot( vecRight ) );
	}
	else
	{
		if ( m_iMoveX >= 0 )
			pActor->SetPoseParameter( m_iMoveX, 0.0f );
		if ( m_iMoveY >= 0 )
			pActor->SetPoseParameter( m_iMoveY, 0.0f );
	}

	if ( pActor->m_flGroundSpeed )
		pActor->SetPlaybackRate( Clamp( flSpeed / pActor->m_flGroundSpeed, -4.0f, 12.0f ) );

	pActor->StudioFrameAdvance();
	pActor->DispatchAnimEvents( pActor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMerasmusBody::StartActivity( Activity act, unsigned int flags )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();

	int iSequence = pActor->SelectWeightedSequence( act );
	if ( iSequence )
	{
		m_Activity = act;

		pActor->SetSequence( iSequence );
		pActor->SetPlaybackRate( 1.0f );
		pActor->SetCycle( 0.0f );
		pActor->ResetSequenceInfo();

		return true;
	}

	return false;
}
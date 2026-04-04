//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR physical throw system - velocity tracking implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_vr_throw.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
CVRVelocityTracker::CVRVelocityTracker()
{
	Reset();
}

//-----------------------------------------------------------------------------
void CVRVelocityTracker::Reset()
{
	m_nSampleCount = 0;
	m_nNewestIndex = -1;
}

//-----------------------------------------------------------------------------
void CVRVelocityTracker::AddSample( const Vector &pos, const QAngle &ang, float flTime )
{
	m_nNewestIndex = ( m_nNewestIndex + 1 ) % MAX_SAMPLES;
	m_samples[m_nNewestIndex].pos = pos;
	m_samples[m_nNewestIndex].ang = ang;
	m_samples[m_nNewestIndex].flTime = flTime;

	if ( m_nSampleCount < MAX_SAMPLES )
		m_nSampleCount++;
}

//-----------------------------------------------------------------------------
// Compute averaged velocity over the sample window.
//
// Walks backward from the newest sample, accumulating per-frame velocity
// vectors until we exceed WINDOW_DURATION. Each frame's velocity is weighted
// equally so that a consistent throw arc produces a stable direction, while
// a short flick still reads correctly from the most recent frames.
//-----------------------------------------------------------------------------
Vector CVRVelocityTracker::GetAveragedVelocity() const
{
	if ( m_nSampleCount < 2 )
		return vec3_origin;

	Vector vecAccum = vec3_origin;
	int nVelocities = 0;

	int iCur = m_nNewestIndex;

	for ( int i = 0; i < m_nSampleCount - 1; i++ )
	{
		int iPrev = ( iCur - 1 + MAX_SAMPLES ) % MAX_SAMPLES;

		float dt = m_samples[iCur].flTime - m_samples[iPrev].flTime;
		if ( dt <= 0.0f )
		{
			iCur = iPrev;
			continue;
		}

		float age = m_samples[m_nNewestIndex].flTime - m_samples[iPrev].flTime;
		if ( age > WINDOW_DURATION )
			break;

		Vector vecFrameVel = ( m_samples[iCur].pos - m_samples[iPrev].pos ) / dt;
		vecAccum += vecFrameVel;
		nVelocities++;

		iCur = iPrev;
	}

	if ( nVelocities == 0 )
		return vec3_origin;

	return vecAccum / (float)nVelocities;
}

//-----------------------------------------------------------------------------
float CVRVelocityTracker::GetAveragedSpeed() const
{
	return GetAveragedVelocity().Length();
}

//-----------------------------------------------------------------------------
// Compute averaged angular velocity (deg/sec) over the window.
//
// Uses quaternion differences instead of Euler angle rates so the result
// is a proper rotation vector (axis * speed) in world-space XYZ.
// Euler angle rates would scramble the axes (pitch → Y, yaw → Z, roll → X)
// and degenerate near gimbal lock.
//-----------------------------------------------------------------------------
Vector CVRVelocityTracker::GetAveragedAngularVelocity() const
{
	if ( m_nSampleCount < 2 )
		return vec3_origin;

	Vector vecAccum = vec3_origin;
	int nVelocities = 0;

	int iCur = m_nNewestIndex;

	for ( int i = 0; i < m_nSampleCount - 1; i++ )
	{
		int iPrev = ( iCur - 1 + MAX_SAMPLES ) % MAX_SAMPLES;

		float dt = m_samples[iCur].flTime - m_samples[iPrev].flTime;
		if ( dt <= 0.0f )
		{
			iCur = iPrev;
			continue;
		}

		float age = m_samples[m_nNewestIndex].flTime - m_samples[iPrev].flTime;
		if ( age > WINDOW_DURATION )
			break;

		Quaternion qCur, qPrev, qInvPrev, qDelta;
		AngleQuaternion( m_samples[iCur].ang, qCur );
		AngleQuaternion( m_samples[iPrev].ang, qPrev );

		QuaternionInvert( qPrev, qInvPrev );
		QuaternionMult( qCur, qInvPrev, qDelta );
		QuaternionNormalize( qDelta );

		// Ensure shortest-path rotation
		if ( qDelta.w < 0.0f )
		{
			qDelta.x = -qDelta.x;
			qDelta.y = -qDelta.y;
			qDelta.z = -qDelta.z;
			qDelta.w = -qDelta.w;
		}

		Vector axis;
		float flAngle; // radians
		QuaternionAxisAngle( qDelta, axis, flAngle );

		if ( flAngle > 0.0001f )
		{
			vecAccum += axis * ( RAD2DEG( flAngle ) / dt );
		}

		nVelocities++;
		iCur = iPrev;
	}

	if ( nVelocities == 0 )
		return vec3_origin;

	return vecAccum / (float)nVelocities;
}

//-----------------------------------------------------------------------------
QAngle CVRVelocityTracker::GetNewestAngles() const
{
	if ( m_nSampleCount == 0 )
		return QAngle( 0, 0, 0 );

	return m_samples[m_nNewestIndex].ang;
}

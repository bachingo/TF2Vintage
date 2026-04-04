//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR physical throw system - velocity tracking and per-weapon tuning
//
//=============================================================================

#ifndef TFVR_VR_THROW_H
#define TFVR_VR_THROW_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

//=============================================================================
// Per-weapon throw tuning parameters
//=============================================================================
struct VRThrowParams
{
	float flBaseSpeed;          // Weapon's base projectile speed (units/sec)
	float flMinSpeedMult;       // Floor multiplier for very slow releases / drops
	float flMaxSpeedMult;       // Ceiling multiplier for hard throws
	float flReferenceHandSpeed; // Hand speed (units/sec) that maps to 1.0x multiplier
};

//=============================================================================
// Ring-buffer velocity tracker with Valve-style averaging
//
// Samples hand position and orientation each frame and computes smoothed
// linear and angular velocity over a short window (~80-110ms) to filter
// tracking jitter while preserving throw intent.
//=============================================================================
class CVRVelocityTracker
{
public:
	CVRVelocityTracker();

	void AddSample( const Vector &pos, const QAngle &ang, float flTime );
	Vector GetAveragedVelocity() const;
	float GetAveragedSpeed() const;
	Vector GetAveragedAngularVelocity() const;
	QAngle GetNewestAngles() const;
	void Reset();

private:
	struct Sample
	{
		Vector pos;
		QAngle ang;
		float flTime;
	};

	static const int MAX_SAMPLES = 10;
	static constexpr float WINDOW_DURATION = 0.09f; // ~8 frames at 90fps

	Sample m_samples[MAX_SAMPLES];
	int m_nSampleCount;
	int m_nNewestIndex;
};

#endif // TFVR_VR_THROW_H

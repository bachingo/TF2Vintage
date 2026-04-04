//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR Spectator Camera - Implements Half-Life: Alyx style camera
//          smoothing for streaming and trailer recording.
//
// Two modes:
// - Mode 1 (Spectator-Only): Smooths desktop mirror view without affecting VR
// - Mode 2 (Full Smoothing): Applies decay to actual rendered view
//
//=============================================================================

#ifndef VR_SPECTATOR_CAMERA_H
#define VR_SPECTATOR_CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Spectator camera modes
//-----------------------------------------------------------------------------
enum VRSpectatorMode_t
{
    VR_SPECTATOR_OFF = 0,           // No smoothing
    VR_SPECTATOR_MIRROR_ONLY = 1,   // Smooth mirror view only (for streaming)
    VR_SPECTATOR_FULL = 2,          // Smooth actual view (for trailers)
};

//-----------------------------------------------------------------------------
// Purpose: Manages VR spectator camera smoothing using exponential decay
//          Similar to Half-Life: Alyx implementation
//-----------------------------------------------------------------------------
class CVRSpectatorCamera
{
public:
    CVRSpectatorCamera();
    ~CVRSpectatorCamera();
    
    // Initialize/shutdown
    bool Initialize();
    void Shutdown();
    void Reset();
    
    // Update smoothing each frame
    void Update(float deltaTime);
    
    // Get current spectator mode
    VRSpectatorMode_t GetMode() const;
    
    // Apply smoothing to angles
    // Input: raw angles from VR headset
    // Output: smoothed angles for spectator view
    void ApplySmoothing(const QAngle& rawAngles, QAngle& smoothedAngles);
    
    // Get smoothed angles for mirror rendering
    // Returns the delta between raw and smoothed (for texture rotation)
    QAngle GetMirrorSmoothingDelta() const;
    
    // Get zoom factor for mirror view (to hide edges during rotation)
    float GetMirrorZoom() const;
    
    // Check if smoothing is active
    bool IsSmoothing() const { return GetMode() != VR_SPECTATOR_OFF; }
    bool IsMirrorOnlyMode() const { return GetMode() == VR_SPECTATOR_MIRROR_ONLY; }
    bool IsFullSmoothingMode() const { return GetMode() == VR_SPECTATOR_FULL; }
    
    // Debug
    void DrawDebug();
    
private:
    // Exponential decay smoothing for a single angle component
    // halfLife: time in seconds to reach halfway to target
    float SmoothAngleComponent(float current, float target, float halfLife, float deltaTime);
    
    // Apply exponential decay to full QAngle
    void SmoothAngles(const QAngle& target, QAngle& current, 
                      float rollHalfLife, float yawPitchHalfLife, float deltaTime);
    
private:
    bool m_bInitialized;
    
    // Current smoothed angles
    QAngle m_smoothedAngles;
    
    // Last raw angles (for delta calculation)
    QAngle m_lastRawAngles;
    
    // Timing
    float m_flLastUpdateTime;
    bool m_bFirstFrame;
};

// Global instance
extern CVRSpectatorCamera* g_pVRSpectatorCamera;

// ConVar accessors (defined in cpp)
VRSpectatorMode_t GetVRSpectatorMode();
float GetVRSpectatorRollHalfLife();
float GetVRSpectatorYawPitchHalfLife();
float GetVRSpectatorZoom();
bool IsVRSpectatorDebugEnabled();

#endif // VR_SPECTATOR_CAMERA_H

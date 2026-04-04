//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR Health Overlay - Renders health bar on hand-attached quad in VR
//
//=============================================================================//

#ifndef VR_HEALTH_OVERLAY_H
#define VR_HEALTH_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "openxr_hand_tracking.h"

class IMaterial;
class ITexture;
class C_TFPlayer;
class CTFHudPlayerStatus;

//-----------------------------------------------------------------------------
// Purpose: Manages rendering of health bar as a 3D quad attached to player's hand
// NEW APPROACH: References the main TF2 HUD panel directly (like objective overlay)
//-----------------------------------------------------------------------------
class CVRHealthOverlay
{
public:
    CVRHealthOverlay();
    ~CVRHealthOverlay();

    // Initialize the overlay system
    bool Initialize();
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Update each frame - checks for health changes and updates position
    void Update();
    
    // Render the health quad in 3D space
    void RenderHealthQuad();
    
    // Set which hand the overlay is attached to (0=left, 1=right)
    void SetHandAttachment(int hand);
    
    // Enable/disable the overlay
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Set position offset relative to hand
    void SetQuadOffset(const Vector& offset) { m_vQuadOffset = offset; }
    void SetQuadRotation(const QAngle& rotation) { m_angQuadRotation = rotation; }
    
    // Reset overlay state (called on map change to clear stale data)
    void ResetOverlayState();

private:
    // Calculate the quad transform matrix based on hand position
    bool CalculateQuadTransform(VMatrix& quadTransform);
    
    // Calculate transform using hand tracking instead of controller
    bool CalculateHandTrackingTransform(VMatrix& quadTransform);
    
    // Check if health display should be shown based on game state
    bool ShouldDisplayHealth();

private:
    bool        m_bInitialized;
    bool        m_bEnabled;
    int         m_nAttachedHand;        // 0=left, 1=right
    float       m_flLastUpdateTime;
    
    // Positioning
    Vector      m_vQuadOffset;          // Offset from hand position
    QAngle      m_angQuadRotation;      // Rotation relative to hand
    
    // Reference to the main health/player status panel (we don't modify it!)
    CTFHudPlayerStatus* m_pMainPlayerStatusPanel;
};

// Global instance
extern CVRHealthOverlay* g_pVRHealthOverlay;

#endif // VR_HEALTH_OVERLAY_H

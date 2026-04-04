#ifndef VR_OBJECTIVE_OVERLAY_H
#define VR_OBJECTIVE_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "openxr_hand_tracking.h"
#include "vgui_controls/Panel.h"
#include "Color.h"

// Forward declarations
class CTFHudObjectiveStatus;
class CVRObjectivePanel;
class IMaterial;
class ITexture;
class C_TFPlayer;

// We create a VR panel that mirrors content from the main objective HUD panel

//-----------------------------------------------------------------------------
// Purpose: Manages rendering of map objectives as a 3D quad attached to player's hand
// Positioned below the health/status overlay for easy viewing
//-----------------------------------------------------------------------------
class CVRObjectiveOverlay
{
public:
    CVRObjectiveOverlay();
    ~CVRObjectiveOverlay();

    // Initialize the overlay system
    bool Initialize();
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Update each frame - checks for objective changes and updates position
    void Update();
    
    // Render the objective quad in 3D space
    void RenderObjectiveQuad();
    
    // Set which hand the overlay is attached to (0=left, 1=right) - should be same as health overlay
    void SetHandAttachment(int hand);
    
    // Enable/disable the overlay
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Set position offset relative to hand (below health overlay)
    void SetQuadOffset(const Vector& offset) { m_vQuadOffset = offset; }
    void SetQuadRotation(const QAngle& rotation) { m_angQuadRotation = rotation; }
    
    // Reset overlay state (called on map change to clear stale data)
    void ResetOverlayState();

private:
    // Calculate the quad transform matrix based on hand position
    bool CalculateQuadTransform(VMatrix& quadTransform);
    
    // Calculate transform using hand tracking instead of controller
    bool CalculateHandTrackingTransform(VMatrix& quadTransform);
    
    // Check if objectives should be displayed based on game state
    bool ShouldDisplayObjectives();
    
    // Force the objective panel to update
    void ForceObjectiveUpdate();

private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // Hand attachment settings
    int m_nAttachedHand;            // Which hand to attach to (0=left, 1=right)
    
    // Position and rotation offsets relative to hand
    Vector m_vQuadOffset;           // Offset from hand position
    QAngle m_angQuadRotation;       // Additional rotation to apply
    
    // Last known state for change detection
    float m_flLastUpdateTime;
    
    // Reference to the main objective panel (for getting bounds info only - we don't modify it!)
    CTFHudObjectiveStatus* m_pMainObjectivePanel;
};

// Global instance
extern CVRObjectiveOverlay* g_pVRObjectiveOverlay;

#endif // VR_OBJECTIVE_OVERLAY_H

#ifndef VR_SPRING_HUD_H
#define VR_SPRING_HUD_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"

// Forward declarations
class CHudElement;

//-----------------------------------------------------------------------------
// Purpose: Manages a HUD element that lazily follows the player's view yaw.
//          Features:
//          - Deadzone: HUD stays still when view is within deadzone angle
//          - Smooth follow: Outside deadzone, HUD smoothly catches up
//          - Edge clamp: Clamps to max_lag angle when turning quickly
//          - Fixed pitch/roll: HUD always stays level
//-----------------------------------------------------------------------------
class CVRSpringHUDManager
{
public:
    CVRSpringHUDManager();
    ~CVRSpringHUDManager();
    
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void ResetState();
    
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }

private:
    bool CalculateSpringTransform(VMatrix& transform);
    float GetCurrentViewYaw() const;
    void UpdateSpringYaw(float deltaTime);
    
private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // The HUD panel we're rendering
    vgui::Panel* m_pKillFeedPanel;
    CHudElement* m_pKillFeedElement;
    
    // Spring arm state
    float m_flCurrentYaw;       // Current spring arm yaw (world space)
    float m_flTargetYaw;        // Target yaw (player's view yaw)
    
    // Configuration
    float m_flDistance;         // Distance from head
    float m_flFollowSpeed;      // How fast the HUD follows rotation (higher = faster)
    float m_flDeadzone;         // Angle deadzone where HUD stays locked to view
    float m_flMaxLagAngle;      // Max angle the HUD can lag behind before clamping
    float m_flPanelWidth;       // World width of the panel
    float m_flPanelHeight;      // World height of the panel
    
    // Panel offset within the view (normalized, -1 to 1)
    float m_flOffsetX;          // Horizontal offset (positive = right)
    float m_flOffsetY;          // Vertical offset (positive = up)
    
    // Pixel dimensions
    int m_nPanelPixelWidth;
    int m_nPanelPixelHeight;
};

// Global instance
extern CVRSpringHUDManager* g_pVRSpringHUDManager;

#endif // VR_SPRING_HUD_H


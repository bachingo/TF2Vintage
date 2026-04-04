#ifndef VR_DAMAGE_INDICATOR_H
#define VR_DAMAGE_INDICATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"

// Forward declarations
class CHudElement;

//-----------------------------------------------------------------------------
// Purpose: Manages the VR damage direction indicator that lazily follows the
//          player's view yaw. Similar to the spring HUD for kill feed but
//          with its own settings tuned for damage indication visibility.
//          Also renders medic caller panels on the same overlay.
//          Features:
//          - Deadzone: HUD stays still when view is within deadzone angle
//          - Smooth follow: Outside deadzone, HUD smoothly catches up
//          - Edge clamp: Clamps to max_lag angle when turning quickly
//          - Fixed pitch/roll: HUD always stays level
//-----------------------------------------------------------------------------
class CVRDamageIndicatorManager
{
public:
    CVRDamageIndicatorManager();
    ~CVRDamageIndicatorManager();
    
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void ResetState();
    
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Static method for VR suppression of vanilla 2D medic caller rendering
    static bool ShouldSuppressMedicCallerPanel();

private:
    bool CalculateSpringTransform(VMatrix& transform);
    float GetCurrentViewYaw() const;
    float GetCurrentViewPitch() const;
    void UpdateSpringAngles(float deltaTime);
    void RenderMedicCallers(const VMatrix& panelToWorld);
    
private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // The HUD panel we're rendering
    vgui::Panel* m_pDamageIndicatorPanel;
    CHudElement* m_pDamageIndicatorElement;
    
    // Spring arm state
    float m_flCurrentYaw;       // Current spring arm yaw (world space)
    float m_flTargetYaw;        // Target yaw (player's view yaw)
    float m_flCurrentPitch;     // Current spring arm pitch
    float m_flTargetPitch;      // Target pitch (player's view pitch)
    
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
extern CVRDamageIndicatorManager* g_pVRDamageIndicatorManager;

#endif // VR_DAMAGE_INDICATOR_H




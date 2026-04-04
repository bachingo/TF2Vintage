// Purpose: VR World Health Icon - Renders floating health icons in 3D world
//          space above players' heads, always facing the player's view.

#ifndef VR_WORLD_HEALTH_ICON_H
#define VR_WORLD_HEALTH_ICON_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include <vgui_controls/EditablePanel.h>

// Forward declarations
class CFloatingHealthIcon;
class C_BaseEntity;
class CMainTargetID;

//-----------------------------------------------------------------------------
// Purpose: Wrapper panel that renders a target panel at (0,0) regardless of
//          its actual screen position. Used to properly render panels in 3D.
//-----------------------------------------------------------------------------
class CVRHealthIconWrapper : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CVRHealthIconWrapper, vgui::EditablePanel);
public:
    CVRHealthIconWrapper(vgui::Panel* parent, const char* name);
    
    void SetTargetPanel(vgui::Panel* pPanel) { m_pTargetPanel = pPanel; }
    vgui::Panel* GetTargetPanel() const { return m_pTargetPanel; }
    
    virtual void Paint() override;
    
private:
    vgui::Panel* m_pTargetPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Manages rendering of floating health icons and target ID in VR world space.
//          Takes over rendering from the 2D CFloatingHealthIcon when in VR.
//          - Positions the health icon above the target entity's head
//          - Also renders the full target ID panel (name, health bar, etc.) above that
//          - Billboards to always face the player's view
//          - Uses DrawPanelIn3DSpace for proper VR stereo rendering
//-----------------------------------------------------------------------------
class CVRWorldHealthIconManager
{
public:
    CVRWorldHealthIconManager();
    ~CVRWorldHealthIconManager();
    
    bool Initialize();
    void Shutdown();
    void Update();
    void Render();
    void ResetState();
    
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Called by CFloatingHealthIcon to check if VR should suppress 2D rendering
    static bool ShouldSuppressVanillaRendering();
    
    // Flag to allow rendering during DrawPanelIn3DSpace (set by Render(), checked by Paint())
    static bool IsRendering3D() { return s_bIsRendering3D; }
    
private:
    bool CalculateBillboardTransform(C_BaseEntity* pEntity, VMatrix& transform);
    Vector GetEntityHeadPosition(C_BaseEntity* pEntity);
    
    // Render the target ID panel above an entity
    void RenderTargetID(C_BaseEntity* pEntity, CMainTargetID* pTargetID, float distanceScale, bool bIsBuilding,
                        int panelWidth, int panelHeight, float tagWorldWidth, float tagWorldHeight, float tagHeightOffset);
    
    bool m_bInitialized;
    bool m_bEnabled;
    
    // Static flag for 3D rendering
    static bool s_bIsRendering3D;
    
    // Wrapper panel for health icon 3D rendering
    CVRHealthIconWrapper* m_pWrapper;
    
    // Wrapper for target ID panel rendering
    CVRHealthIconWrapper* m_pTargetIDWrapper;
    
    // Configuration
    float m_flScale;            // World scale of the health icon
    float m_flHeightOffset;     // Additional height above entity head
    float m_flOffsetX;          // Horizontal offset in world units
    float m_flOffsetY;          // Forward/back offset in world units
    int m_nRenderScale;         // Resolution multiplier for sharper text
    
    // Target ID configuration
    float m_flTargetIDScale;        // World scale of the target ID panel
    float m_flTargetIDOffset;       // Height offset above the health icon (for players)
    float m_flTargetIDBuildingOffset; // Height offset for buildings
    
    // Track target changes to avoid first-frame flash
    int m_nLastTargetIndex;
    int m_nFramesSinceTargetChange;
};

// Global instance
extern CVRWorldHealthIconManager* g_pVRWorldHealthIconManager;

#endif // VR_WORLD_HEALTH_ICON_H

// Purpose: VR Spectator Extras - Renders player names and health bars in 3D
//          world space above teammates/entities when glowing or spectating.

#ifndef VR_SPECTATOR_EXTRAS_H
#define VR_SPECTATOR_EXTRAS_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "utlvector.h"
#include <vgui_controls/EditablePanel.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class C_BaseEntity;

//-----------------------------------------------------------------------------
// Purpose: Individual entity entry for spectator extras display
//-----------------------------------------------------------------------------
struct vr_spec_extra_t
{
    int m_nEntIndex;
    wchar_t m_wszName[256];
    float m_flHealth;
    Color m_clrGlowColor;
    bool m_bDrawName;
    int m_nOffset;
};

//-----------------------------------------------------------------------------
// Purpose: Panel to render a single name/health bar entry
//-----------------------------------------------------------------------------
class CVRSpectatorExtrasPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CVRSpectatorExtrasPanel, vgui::EditablePanel);
    
public:
    CVRSpectatorExtrasPanel(vgui::Panel* parent, const char* name);
    
    void SetData(const wchar_t* wszName, float flHealth, Color color);
    virtual void Paint() override;
    
private:
    wchar_t m_wszName[256];
    float m_flHealth;
    Color m_Color;
    
    vgui::HFont m_hFont;
};

//-----------------------------------------------------------------------------
// Purpose: Manages rendering of spectator extras (names/health) in VR world space
//-----------------------------------------------------------------------------
class CVRSpectatorExtrasManager
{
public:
    CVRSpectatorExtrasManager();
    ~CVRSpectatorExtrasManager();
    
    bool Initialize();
    void Shutdown();
    void ResetState();
    
    void Update(float deltaTime);
    void Render();
    
    // Static accessors
    static bool IsEnabled();
    static bool ShouldSuppressVanillaRendering();
    
private:
    // Build list of entities that should have name/health displayed
    void UpdateEntityList();
    
    // Calculate billboard transform facing the player
    bool CalculateBillboardTransform(const Vector& worldPos, VMatrix& transform);
    
private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // Entity list (mirrors CTFHudSpectatorExtras logic)
    CUtlVector<vr_spec_extra_t> m_vecEntitiesToDraw;
    
    // Rendering panel (reused for each entity)
    CVRSpectatorExtrasPanel* m_pRenderPanel;
    
    // Configuration
    float m_flScale;
    int m_nRenderScale;
};

// Global instance
extern CVRSpectatorExtrasManager* g_pVRSpectatorExtrasManager;

#endif // VR_SPECTATOR_EXTRAS_H

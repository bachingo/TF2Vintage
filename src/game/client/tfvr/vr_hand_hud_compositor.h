#ifndef VR_HAND_HUD_COMPOSITOR_H
#define VR_HAND_HUD_COMPOSITOR_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "openxr_hand_tracking.h"
#include <vgui_controls/EditablePanel.h>

// Forward declarations
class CTFHudPlayerStatus;
class CTFHudObjectiveStatus;
class CTFHudWeaponAmmo;
class CHudItemEffectMeter;
class CTFHudMatchStatus;
class C_TFPlayer;
class C_TFVRHand;

// Forward declare CHudElement
class CHudElement;

//-----------------------------------------------------------------------------
// HUD Element slot for the compositor
//-----------------------------------------------------------------------------
struct VRHudElementSlot_t
{
    vgui::Panel* pPanel;        // The HUD panel to render
    CHudElement* pHudElement;   // The CHudElement (for ShouldDraw checks), may be null
    const char* szName;         // Element name for lookup
    
    // Positioning
    int nOffsetX;               // X offset within compositor
    int nOffsetY;               // Y offset within compositor
    int nContentOffsetX;        // Content shift to compensate for internal panel layout
    int nContentOffsetY;        // Content vertical shift
    int nWidth;                 // Override width (0 = use panel's width)
    int nHeight;                // Override height (0 = use panel's height)
    float flScale;              // Scale factor (1.0 = native size)
    int nPriority;              // Priority for stacking (lower = higher on screen)
    
    // Flags
    bool bEnabled;              // Whether to render this element
    bool bCenterHorizontally;   // Auto-center horizontally in compositor
    bool bCenterVertically;     // Auto-center vertically in compositor
    bool bDynamic;              // Dynamic element (may come/go based on weapon/class)
    bool bLayerOnAmmo;          // Layer on top of ammo (like charge meters)
    
    // Check if this slot should be drawn (uses CHudElement::ShouldDraw if available)
    bool ShouldDraw() const;
    
    // Check if the panel pointer is still valid
    bool IsValid() const;
    
    VRHudElementSlot_t()
        : pPanel(nullptr)
        , pHudElement(nullptr)
        , szName("")
        , nOffsetX(0)
        , nOffsetY(0)
        , nContentOffsetX(0)
        , nContentOffsetY(0)
        , nWidth(0)
        , nHeight(0)
        , flScale(1.0f)
        , nPriority(0)
        , bEnabled(true)
        , bCenterHorizontally(false)
        , bCenterVertically(false)
        , bDynamic(false)
        , bLayerOnAmmo(false)
    {}
};

//-----------------------------------------------------------------------------
// Purpose: Base compositor panel that renders multiple HUD elements into a single
//          unified panel for VR hand display
//-----------------------------------------------------------------------------
class CVRHUDCompositor : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CVRHUDCompositor, vgui::EditablePanel);
    
public:
    CVRHUDCompositor(vgui::Panel* parent, const char* name);
    virtual ~CVRHUDCompositor();
    
    // Initialize - override in derived classes to set up HUD element references
    virtual bool Initialize();
    
    // Shutdown and cleanup
    virtual void Shutdown();
    
    // Update layout - call each frame
    virtual void UpdateLayout();
    
    // Check if compositor is ready to render
    bool IsReady() const { return m_bInitialized; }
    
    // Get compositor dimensions for 3D rendering
    void GetCompositorSize(int& width, int& height) const;
    
    // Set compositor size
    void SetCompositorSize(int width, int height);
    
protected:
    // Override Paint to draw child panels at our specified offsets
    virtual void Paint() override;
    
    // Override PaintBackground for optional debug visualization
    virtual void PaintBackground() override;
    
    // Paint a HUD panel at the specified offset within our compositor
    void PaintPanelAtOffset(vgui::Panel* pPanel, int offsetX, int offsetY, float scale = 1.0f);
    
    // Calculate centered X position for a panel
    int CalculateCenteredX(int panelWidth) const;
    
    // Calculate centered Y position for a panel
    int CalculateCenteredY(int panelHeight) const;
    
    // Add a slot to the compositor
    int AddSlot(vgui::Panel* pPanel, const char* szName);
    int AddSlot(vgui::Panel* pPanel, CHudElement* pHudElement, const char* szName, bool bDynamic);
    
    // Get slot by index
    VRHudElementSlot_t* GetSlot(int index);
    
    // Clear all slots
    void ClearSlots();
    
protected:
    bool m_bInitialized;
    
    // Compositor canvas size
    int m_nCompositorWidth;
    int m_nCompositorHeight;
    
    // HUD element slots
    static const int MAX_HUD_SLOTS = 16;
    VRHudElementSlot_t m_HudSlots[MAX_HUD_SLOTS];
    int m_nNumSlots;
    
    // Debug visualization
    bool m_bShowDebugBackground;
};

//-----------------------------------------------------------------------------
// Purpose: Status HUD Compositor (health + objective for left hand)
//-----------------------------------------------------------------------------
class CVRStatusHUDCompositor : public CVRHUDCompositor
{
    DECLARE_CLASS_SIMPLE(CVRStatusHUDCompositor, CVRHUDCompositor);
    
public:
    CVRStatusHUDCompositor(vgui::Panel* parent, const char* name);
    
    virtual bool Initialize() override;
    virtual void UpdateLayout() override;
    
private:
    void RefreshHUDReferences();
    
private:
    CTFHudPlayerStatus* m_pHealthPanel;
    CTFHudObjectiveStatus* m_pObjectivePanel;
    vgui::Panel* m_pMatchStatusPanel;
    CHudElement* m_pMatchStatusElement;
    
    int m_nHealthSlotIndex;
    int m_nObjectiveSlotIndex;
    int m_nMatchStatusSlotIndex;
};

//-----------------------------------------------------------------------------
// Purpose: Weapon HUD Compositor (ammo + meters for right hand)
//-----------------------------------------------------------------------------
class CVRWeaponHUDCompositor : public CVRHUDCompositor
{
    DECLARE_CLASS_SIMPLE(CVRWeaponHUDCompositor, CVRHUDCompositor);
    
public:
    CVRWeaponHUDCompositor(vgui::Panel* parent, const char* name);
    
    virtual bool Initialize() override;
    virtual void UpdateLayout() override;
    
    // Refresh dynamic elements (call when weapon/class changes)
    void RefreshDynamicElements();
    
private:
    void RefreshHUDReferences();
    void GatherItemEffectMeters();
    void UpdateMeterLayout();
    
private:
    // Core weapon HUD elements (panel + CHudElement pairs)
    CTFHudWeaponAmmo* m_pAmmoPanel;
    
    // Class-specific HUD elements (stored as both panel and CHudElement for ShouldDraw checks)
    vgui::Panel* m_pDemomanPipes;
    CHudElement* m_pDemomanPipesElement;
    vgui::Panel* m_pBowCharge;
    CHudElement* m_pBowChargeElement;
    vgui::Panel* m_pDemomanCharge;
    CHudElement* m_pDemomanChargeElement;
    vgui::Panel* m_pMedicCharge;
    CHudElement* m_pMedicChargeElement;
    vgui::Panel* m_pFlameRocketCharge;
    CHudElement* m_pFlameRocketChargeElement;
    vgui::Panel* m_pAccountPanel;  // Engineer metal
    CHudElement* m_pAccountPanelElement;
    
    // Slot indices
    int m_nAmmoSlotIndex;
    int m_nDemomanPipesSlotIndex;
    int m_nBowChargeSlotIndex;
    int m_nDemomanChargeSlotIndex;
    int m_nMedicChargeSlotIndex;
    int m_nFlameRocketChargeSlotIndex;
    int m_nAccountPanelSlotIndex;
    
    // Dynamic item effect meters start at this index
    int m_nFirstMeterSlotIndex;
    int m_nNumMeterSlots;
};

//-----------------------------------------------------------------------------
// Purpose: Base manager class for VR HUD - handles 3D positioning and rendering
//-----------------------------------------------------------------------------
class CVRHUDManager
{
public:
    CVRHUDManager();
    virtual ~CVRHUDManager();
    
    // Initialize the system
    virtual bool Initialize() = 0;
    
    // Shutdown and cleanup
    virtual void Shutdown();
    
    // Update each frame
    virtual void Update();
    
    // Render the HUD in 3D space
    virtual void Render() = 0;
    
    // Enable/disable the HUD
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Reset state (call on map change)
    virtual void ResetState();
    
protected:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // Positioning
    Vector m_vOffset;       // Offset from attachment point
    QAngle m_angRotation;   // Rotation adjustment
    float m_flScale;        // World scale
};

//-----------------------------------------------------------------------------
// Purpose: Status HUD Manager (left hand, palm attachment)
//-----------------------------------------------------------------------------
class CVRStatusHUDManager : public CVRHUDManager
{
public:
    CVRStatusHUDManager();
    virtual ~CVRStatusHUDManager();
    
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void Update() override;
    virtual void Render() override;
    virtual void ResetState() override;
    
    // Set which hand the HUD is attached to (0=left, 1=right)
    void SetHandAttachment(int hand);
    
    CVRStatusHUDCompositor* GetCompositor() { return m_pCompositor; }

private:
    bool CalculateHandTransform(VMatrix& transform);
    bool CalculateHandTrackingTransform(VMatrix& transform);
    bool CalculateControllerTransform(VMatrix& transform);
    void ApplyCenteringOffset(VMatrix& transform, float worldWidth, float worldHeight);
    
private:
    int m_nAttachedHand;    // 0=left, 1=right
    CVRStatusHUDCompositor* m_pCompositor;
};

//-----------------------------------------------------------------------------
// Purpose: Weapon HUD Manager (right hand, weapon_bone attachment)
//-----------------------------------------------------------------------------
class CVRWeaponHUDManager : public CVRHUDManager
{
public:
    CVRWeaponHUDManager();
    virtual ~CVRWeaponHUDManager();
    
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void Update() override;
    virtual void Render() override;
    virtual void ResetState() override;
    
    CVRWeaponHUDCompositor* GetCompositor() { return m_pCompositor; }

private:
    bool CalculateWeaponBoneTransform(VMatrix& transform);
    void ApplyCenteringOffset(VMatrix& transform, float worldWidth, float worldHeight);
    
private:
    CVRWeaponHUDCompositor* m_pCompositor;
    
    // Track last weapon to detect changes
    CHandle<C_BaseCombatWeapon> m_hLastWeapon;
    
    // True when weapon is held by left hand (e.g. medigun)
    bool m_bWeaponOnLeftHand;
};

// Global instances
extern CVRStatusHUDManager* g_pVRStatusHUDManager;
extern CVRWeaponHUDManager* g_pVRWeaponHUDManager;

#endif // VR_HAND_HUD_COMPOSITOR_H

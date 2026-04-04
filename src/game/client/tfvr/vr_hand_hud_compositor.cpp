#include "cbase.h"
#include "vr_hand_hud_compositor.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "c_tfvr_hand.h"
#include "openxr_manager.h"
#include "hudelement.h"
#include "hud.h"
#include "tf_hud_playerstatus.h"
#include "tf/tf_hud_objectivestatus.h"
#include "tf_hud_ammostatus.h"
#include "tf_hud_itemeffectmeter.h"
#include "tf_hud_match_status.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "view.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instances
CVRStatusHUDManager* g_pVRStatusHUDManager = nullptr;
CVRWeaponHUDManager* g_pVRWeaponHUDManager = nullptr;

static float GetVRScreenScaleFactor()
{
	int w, h;
	vgui::surface()->GetScreenSize(w, h);
	return (float)w / 1280.0f;
}

//=============================================================================
// ConVars - Status HUD (left hand: health/objective)
//=============================================================================

ConVar tfvr_status_hud_enabled("tfvr_status_hud_enabled", "1", FCVAR_ARCHIVE, 
    "Enable the VR hand HUD (health/objective on left hand)");
ConVar tfvr_status_hud_hand("tfvr_status_hud_hand", "0", FCVAR_ARCHIVE, 
    "Which hand to attach HUD to: 0=left, 1=right");
ConVar tfvr_status_hud_use_hand_tracking("tfvr_status_hud_use_hand_tracking", "1", FCVAR_ARCHIVE, 
    "Use hand tracking instead of controller pose");
ConVar tfvr_status_hud_width("tfvr_status_hud_width", "350", FCVAR_ARCHIVE, 
    "Width of the VR hand HUD canvas in pixels");
ConVar tfvr_status_hud_height("tfvr_status_hud_height", "500", FCVAR_ARCHIVE, 
    "Height of the VR hand HUD canvas in pixels");
ConVar tfvr_status_hud_scale("tfvr_status_hud_scale", "15", FCVAR_ARCHIVE, 
    "Scale of the hand HUD in world units");
ConVar tfvr_status_hud_center_on_palm("tfvr_status_hud_center_on_palm", "1", FCVAR_ARCHIVE, 
    "Auto-center the HUD on the palm bone (1=centered, 0=use offsets from palm)");
ConVar tfvr_status_hud_offset_x("tfvr_status_hud_offset_x", "0", FCVAR_ARCHIVE, 
    "X offset from palm center (right/left in palm space)");
ConVar tfvr_status_hud_offset_y("tfvr_status_hud_offset_y", "0", FCVAR_ARCHIVE, 
    "Y offset from palm center (up/down in palm space)");
ConVar tfvr_status_hud_offset_z("tfvr_status_hud_offset_z", "0", FCVAR_ARCHIVE, 
    "Z offset from palm center (forward/back in palm space)");
ConVar tfvr_status_hud_base_pitch("tfvr_status_hud_base_pitch", "-90", FCVAR_ARCHIVE, 
    "Base pitch rotation to orient panel as watch face (-90 = facing up from palm)");
ConVar tfvr_status_hud_base_yaw("tfvr_status_hud_base_yaw", "0", FCVAR_ARCHIVE, 
    "Base yaw rotation for panel orientation");
ConVar tfvr_status_hud_base_roll("tfvr_status_hud_base_roll", "0", FCVAR_ARCHIVE, 
    "Base roll rotation for panel orientation");
ConVar tfvr_status_hud_pitch("tfvr_status_hud_pitch", "0", FCVAR_ARCHIVE, 
    "Additional pitch rotation adjustment");
ConVar tfvr_status_hud_yaw("tfvr_status_hud_yaw", "0", FCVAR_ARCHIVE, 
    "Additional yaw rotation adjustment");
ConVar tfvr_status_hud_roll("tfvr_status_hud_roll", "0", FCVAR_ARCHIVE, 
    "Additional roll rotation adjustment");

// Health panel layout
ConVar tfvr_status_hud_health_enabled("tfvr_status_hud_health_enabled", "1", FCVAR_ARCHIVE, 
    "Show health panel in hand HUD");
ConVar tfvr_status_hud_health_x("tfvr_status_hud_health_x", "0", FCVAR_ARCHIVE, 
    "Health panel X offset (0=centered)");
ConVar tfvr_status_hud_health_y("tfvr_status_hud_health_y", "0", FCVAR_ARCHIVE, 
    "Health panel Y offset from top");
ConVar tfvr_status_hud_health_center("tfvr_status_hud_health_center", "1", FCVAR_ARCHIVE, 
    "Auto-center health panel horizontally");
ConVar tfvr_status_hud_health_center_v("tfvr_status_hud_health_center_v", "0", FCVAR_ARCHIVE, 
    "Auto-center health panel vertically");
ConVar tfvr_status_hud_health_content_offset_x("tfvr_status_hud_health_content_offset_x", "-50", FCVAR_ARCHIVE, 
    "Content X offset to visually center the health cross (default -50 compensates for internal layout)");
ConVar tfvr_status_hud_health_content_offset_y("tfvr_status_hud_health_content_offset_y", "0", FCVAR_ARCHIVE, 
    "Content Y offset for health panel");
ConVar tfvr_status_hud_health_scale("tfvr_status_hud_health_scale", "1.0", FCVAR_ARCHIVE, 
    "Scale factor for health panel (1.0 = native size, 0.5 = half size, 2.0 = double)");

// Objective panel layout
ConVar tfvr_status_hud_objective_enabled("tfvr_status_hud_objective_enabled", "1", FCVAR_ARCHIVE, 
    "Show objective panel in hand HUD");
ConVar tfvr_status_hud_objective_x("tfvr_status_hud_objective_x", "0", FCVAR_ARCHIVE, 
    "Objective panel X offset (0=centered)");
ConVar tfvr_status_hud_objective_y("tfvr_status_hud_objective_y", "140", FCVAR_ARCHIVE, 
    "Objective panel Y offset (below health)");
ConVar tfvr_status_hud_objective_center("tfvr_status_hud_objective_center", "1", FCVAR_ARCHIVE, 
    "Auto-center objective panel horizontally");
ConVar tfvr_status_hud_objective_center_v("tfvr_status_hud_objective_center_v", "0", FCVAR_ARCHIVE, 
    "Auto-center objective panel vertically");
ConVar tfvr_status_hud_objective_content_offset_x("tfvr_status_hud_objective_content_offset_x", "0", FCVAR_ARCHIVE, 
    "Content X offset for objective panel");
ConVar tfvr_status_hud_objective_content_offset_y("tfvr_status_hud_objective_content_offset_y", "0", FCVAR_ARCHIVE, 
    "Content Y offset for objective panel");
ConVar tfvr_status_hud_objective_scale("tfvr_status_hud_objective_scale", "1.0", FCVAR_ARCHIVE, 
    "Scale factor for objective panel (1.0 = native size, 0.5 = half size, 2.0 = double)");

// Match status panel layout (team compositions + timer)
ConVar tfvr_status_hud_matchstatus_enabled("tfvr_status_hud_matchstatus_enabled", "0", FCVAR_ARCHIVE, 
    "Show match status panel (team compositions + timer) in hand HUD (disabled by default, use VR popup HUD instead)");
ConVar tfvr_status_hud_matchstatus_x("tfvr_status_hud_matchstatus_x", "0", FCVAR_ARCHIVE, 
    "Match status panel X offset");
ConVar tfvr_status_hud_matchstatus_y("tfvr_status_hud_matchstatus_y", "-200", FCVAR_ARCHIVE, 
    "Match status panel Y offset (negative = above health)");
ConVar tfvr_status_hud_matchstatus_center("tfvr_status_hud_matchstatus_center", "1", FCVAR_ARCHIVE, 
    "Auto-center match status panel horizontally");
ConVar tfvr_status_hud_matchstatus_content_offset_x("tfvr_status_hud_matchstatus_content_offset_x", "0", FCVAR_ARCHIVE, 
    "Content X offset for match status panel");
ConVar tfvr_status_hud_matchstatus_content_offset_y("tfvr_status_hud_matchstatus_content_offset_y", "0", FCVAR_ARCHIVE, 
    "Content Y offset for match status panel");
ConVar tfvr_status_hud_matchstatus_scale("tfvr_status_hud_matchstatus_scale", "1.0", FCVAR_ARCHIVE, 
    "Scale factor for match status panel");

ConVar tfvr_status_hud_debug_bg("tfvr_status_hud_debug_bg", "0", FCVAR_ARCHIVE, 
    "Show debug background for compositor bounds");

//=============================================================================
// ConVars - Weapon HUD (right hand: ammo/meters)
//=============================================================================

ConVar tfvr_weapon_hud_enabled("tfvr_weapon_hud_enabled", "1", FCVAR_ARCHIVE, 
    "Enable the VR weapon HUD (ammo/meters on right hand)");
ConVar tfvr_weapon_hud_width("tfvr_weapon_hud_width", "300", FCVAR_ARCHIVE, 
    "Width of the weapon HUD canvas in pixels");
ConVar tfvr_weapon_hud_height("tfvr_weapon_hud_height", "250", FCVAR_ARCHIVE, 
    "Height of the weapon HUD canvas in pixels");
ConVar tfvr_weapon_hud_scale("tfvr_weapon_hud_scale", "8", FCVAR_ARCHIVE, 
    "Scale of the weapon HUD in world units");
ConVar tfvr_weapon_hud_center("tfvr_weapon_hud_center", "1", FCVAR_ARCHIVE, 
    "Auto-center the weapon HUD on the weapon bone");
ConVar tfvr_weapon_hud_offset_x("tfvr_weapon_hud_offset_x", "0", FCVAR_ARCHIVE, 
    "X offset from weapon bone");
ConVar tfvr_weapon_hud_offset_y("tfvr_weapon_hud_offset_y", "5", FCVAR_ARCHIVE, 
    "Y offset from weapon bone");
ConVar tfvr_weapon_hud_offset_z("tfvr_weapon_hud_offset_z", "0", FCVAR_ARCHIVE, 
    "Z offset from weapon bone");
ConVar tfvr_weapon_hud_pitch("tfvr_weapon_hud_pitch", "0", FCVAR_ARCHIVE, 
    "Pitch rotation adjustment");
ConVar tfvr_weapon_hud_yaw("tfvr_weapon_hud_yaw", "0", FCVAR_ARCHIVE, 
    "Yaw rotation adjustment");
ConVar tfvr_weapon_hud_roll("tfvr_weapon_hud_roll", "0", FCVAR_ARCHIVE, 
    "Roll rotation adjustment");

// Ammo panel layout
ConVar tfvr_weapon_hud_ammo_enabled("tfvr_weapon_hud_ammo_enabled", "1", FCVAR_ARCHIVE, 
    "Show ammo panel in weapon HUD");
ConVar tfvr_weapon_hud_ammo_x("tfvr_weapon_hud_ammo_x", "0", FCVAR_ARCHIVE, 
    "Ammo panel X offset");
ConVar tfvr_weapon_hud_ammo_y("tfvr_weapon_hud_ammo_y", "0", FCVAR_ARCHIVE, 
    "Ammo panel Y offset");
ConVar tfvr_weapon_hud_ammo_center("tfvr_weapon_hud_ammo_center", "1", FCVAR_ARCHIVE, 
    "Auto-center ammo panel horizontally");
ConVar tfvr_weapon_hud_ammo_content_offset_x("tfvr_weapon_hud_ammo_content_offset_x", "0", FCVAR_ARCHIVE, 
    "Content X offset for ammo panel");
ConVar tfvr_weapon_hud_ammo_content_offset_y("tfvr_weapon_hud_ammo_content_offset_y", "0", FCVAR_ARCHIVE, 
    "Content Y offset for ammo panel");
ConVar tfvr_weapon_hud_ammo_scale("tfvr_weapon_hud_ammo_scale", "1.0", FCVAR_ARCHIVE, 
    "Scale factor for ammo panel");

// Charge bar layout (bow charge, sticky charge - elements that layer on ammo)
ConVar tfvr_weapon_hud_charge_offset_x("tfvr_weapon_hud_charge_offset_x", "0", FCVAR_ARCHIVE, 
    "X offset for charge bars relative to ammo position");
ConVar tfvr_weapon_hud_charge_offset_y("tfvr_weapon_hud_charge_offset_y", "40", FCVAR_ARCHIVE, 
    "Y offset for charge bars relative to ammo position (positive = below ammo)");

// Metal/account panel layout
ConVar tfvr_weapon_hud_account_offset_x("tfvr_weapon_hud_account_offset_x", "0", FCVAR_ARCHIVE, 
    "X offset for metal count panel (added after meter layout, positive = right)");
ConVar tfvr_weapon_hud_account_offset_y("tfvr_weapon_hud_account_offset_y", "0", FCVAR_ARCHIVE, 
    "Y offset for metal count panel (added after meter layout, positive = down)");

// Meter layout (sticky count, effect meters, etc.)
ConVar tfvr_weapon_hud_meters_y("tfvr_weapon_hud_meters_y", "80", FCVAR_ARCHIVE, 
    "Y offset for meter panels (below ammo)");
ConVar tfvr_weapon_hud_meters_spacing("tfvr_weapon_hud_meters_spacing", "5", FCVAR_ARCHIVE, 
    "Horizontal spacing between meter panels");
ConVar tfvr_weapon_hud_meters_width_override("tfvr_weapon_hud_meters_width_override", "0", FCVAR_ARCHIVE, 
    "Override width for meter panels (0=use actual panel width, >0=use this width for spacing calculation)");
ConVar tfvr_weapon_hud_meters_content_offset_x("tfvr_weapon_hud_meters_content_offset_x", "0", FCVAR_ARCHIVE, 
    "X content offset for meter panels (negative to shift left)");

ConVar tfvr_weapon_hud_debug_bg("tfvr_weapon_hud_debug_bg", "0", FCVAR_ARCHIVE, 
    "Show debug background for weapon HUD");

//=============================================================================
// Helper Functions
//=============================================================================

//-----------------------------------------------------------------------------
// Check if a panel is facing away from the camera (backface culling)
//-----------------------------------------------------------------------------
static bool IsPanelBackfacing(const VMatrix& panelToWorld)
{
    // Panel normal is the Z-axis of the transform
    Vector panelNormal(panelToWorld[0][2], panelToWorld[1][2], panelToWorld[2][2]);
    Vector panelPos = panelToWorld.GetTranslation();
    Vector toCamera = MainViewOrigin() - panelPos;
    return DotProduct(panelNormal, toCamera) < 0;
}

//-----------------------------------------------------------------------------
// Check if an item effect meter pointer is still valid
//-----------------------------------------------------------------------------
static bool IsItemEffectMeterValid(CHudElement* pElement)
{
    if (!pElement)
        return false;
    
    // Check if this pointer is in the current auto list
    for (int i = 0; i < IHudItemEffectMeterAutoList::AutoList().Count(); i++)
    {
        CHudItemEffectMeter* pMeter = static_cast<CHudItemEffectMeter*>(IHudItemEffectMeterAutoList::AutoList()[i]);
        if (static_cast<CHudElement*>(pMeter) == pElement)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool VRHudElementSlot_t::ShouldDraw() const
{
    if (!szName || szName[0] == '\0')
        return false;
    
    CHudElement* pElement = nullptr;
    
    // Named elements - look up fresh to avoid dangling pointers
    if (V_strcmp(szName, "ItemEffectMeter") != 0)
    {
        pElement = gHUD.FindElement(szName);
    }
    else if (pHudElement && IsItemEffectMeterValid(pHudElement))
    {
        pElement = pHudElement;
    }
    
    return pElement ? pElement->ShouldDraw() : false;
}

//-----------------------------------------------------------------------------
bool VRHudElementSlot_t::IsValid() const
{
    if (!szName || szName[0] == '\0')
        return false;
    
    // Named elements - check if element exists
    if (V_strcmp(szName, "ItemEffectMeter") != 0)
    {
        return gHUD.FindElement(szName) != nullptr;
    }
    
    // Item effect meters - validate against auto list
    return pHudElement && IsItemEffectMeterValid(pHudElement);
}

//-----------------------------------------------------------------------------
// Get a validated panel pointer for a slot, looking up fresh to avoid dangling pointers
//-----------------------------------------------------------------------------
static vgui::Panel* GetSlotPanel(const VRHudElementSlot_t& slot)
{
    if (!slot.szName || slot.szName[0] == '\0')
        return nullptr;
    
    // Named elements (except generic "ItemEffectMeter") - look up by name
    if (V_strcmp(slot.szName, "ItemEffectMeter") != 0)
    {
        CHudElement* pElement = gHUD.FindElement(slot.szName);
        return pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    }
    
    // Item effect meters - validate pointer is still in the auto list
    if (slot.pHudElement && IsItemEffectMeterValid(slot.pHudElement))
    {
        return dynamic_cast<vgui::Panel*>(slot.pHudElement);
    }
    
    return nullptr;
}

//=============================================================================
// CVRHUDCompositor Base Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRHUDCompositor);

//-----------------------------------------------------------------------------
CVRHUDCompositor::CVRHUDCompositor(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_bInitialized = false;
    m_nCompositorWidth = 350;
    m_nCompositorHeight = 500;
    m_nNumSlots = 0;
    m_bShowDebugBackground = false;
    
    for (int i = 0; i < MAX_HUD_SLOTS; i++)
    {
        m_HudSlots[i] = VRHudElementSlot_t();
    }
    
    SetSize(m_nCompositorWidth, m_nCompositorHeight);
    SetPaintBackgroundEnabled(true);
    SetPaintBorderEnabled(false);
    SetVisible(true);
    SetEnabled(true);
    SetMouseInputEnabled(false);
    SetKeyBoardInputEnabled(false);
}

//-----------------------------------------------------------------------------
CVRHUDCompositor::~CVRHUDCompositor()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRHUDCompositor::Initialize()
{
    if (m_bInitialized)
        return true;
    
    m_bInitialized = true;
    return true;
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::Shutdown()
{
    ClearSlots();
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::UpdateLayout()
{
    // Base class does nothing - derived classes override
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::GetCompositorSize(int& width, int& height) const
{
    width = m_nCompositorWidth;
    height = m_nCompositorHeight;
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::SetCompositorSize(int width, int height)
{
    m_nCompositorWidth = width;
    m_nCompositorHeight = height;
    SetSize(width, height);
}

//-----------------------------------------------------------------------------
int CVRHUDCompositor::CalculateCenteredX(int panelWidth) const
{
    return (m_nCompositorWidth - panelWidth) / 2;
}

//-----------------------------------------------------------------------------
int CVRHUDCompositor::CalculateCenteredY(int panelHeight) const
{
    return (m_nCompositorHeight - panelHeight) / 2;
}

//-----------------------------------------------------------------------------
int CVRHUDCompositor::AddSlot(vgui::Panel* pPanel, const char* szName)
{
    return AddSlot(pPanel, nullptr, szName, false);
}

//-----------------------------------------------------------------------------
int CVRHUDCompositor::AddSlot(vgui::Panel* pPanel, CHudElement* pHudElement, const char* szName, bool bDynamic)
{
    if (m_nNumSlots >= MAX_HUD_SLOTS)
    {
        Warning("VR HUD Compositor: Too many slots, max is %d\n", MAX_HUD_SLOTS);
        return -1;
    }
    
    int index = m_nNumSlots++;
    VRHudElementSlot_t& slot = m_HudSlots[index];
    slot.pPanel = pPanel;
    slot.pHudElement = pHudElement;
    slot.szName = szName;
    slot.bEnabled = true;
    slot.bDynamic = bDynamic;
    
    return index;
}

//-----------------------------------------------------------------------------
VRHudElementSlot_t* CVRHUDCompositor::GetSlot(int index)
{
    if (index < 0 || index >= m_nNumSlots)
        return nullptr;
    return &m_HudSlots[index];
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::ClearSlots()
{
    for (int i = 0; i < MAX_HUD_SLOTS; i++)
    {
        m_HudSlots[i].pPanel = nullptr;
    }
    m_nNumSlots = 0;
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::PaintBackground()
{
    if (m_bShowDebugBackground)
    {
        vgui::surface()->DrawSetColor(Color(40, 40, 40, 180));
        vgui::surface()->DrawFilledRect(0, 0, m_nCompositorWidth, m_nCompositorHeight);
        
        vgui::surface()->DrawSetColor(Color(255, 200, 0, 255));
        vgui::surface()->DrawOutlinedRect(0, 0, m_nCompositorWidth, m_nCompositorHeight);
    }
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::Paint()
{
    VPROF("VRHUDCompositor_Paint");
    
    if (!m_bInitialized)
        return;
    
    UpdateLayout();
    
    g_pMatSystemSurface->DisableClipping(true);
    
    for (int i = 0; i < m_nNumSlots; i++)
    {
        VRHudElementSlot_t& slot = m_HudSlots[i];
        
        if (!slot.bEnabled)
            continue;
        
        // For dynamic elements, check ShouldDraw first
        if (slot.bDynamic && !slot.ShouldDraw())
            continue;
        
        // Get a valid panel pointer (looks up fresh to avoid dangling pointers)
        vgui::Panel* pPanel = GetSlotPanel(slot);
        if (!pPanel)
            continue;
        
        int panelWidth, panelHeight;
        pPanel->GetSize(panelWidth, panelHeight);
        
        // Skip if panel has no size
        if (panelWidth <= 0 || panelHeight <= 0)
            continue;
        
        float scale = clamp(slot.flScale, 0.1f, 4.0f);
        int scaledWidth = (int)(panelWidth * scale);
        int scaledHeight = (int)(panelHeight * scale);
        
        int offsetX = slot.nOffsetX;
        if (slot.bCenterHorizontally)
        {
            offsetX = CalculateCenteredX(scaledWidth) + slot.nOffsetX;
        }
        
        int offsetY = slot.nOffsetY;
        if (slot.bCenterVertically)
        {
            offsetY = CalculateCenteredY(scaledHeight) + slot.nOffsetY;
        }
        
        int finalX = offsetX + (int)(slot.nContentOffsetX * scale);
        int finalY = offsetY + (int)(slot.nContentOffsetY * scale);
        
        // Temporarily make the panel visible for painting
        bool bWasVisible = pPanel->IsVisible();
        pPanel->SetVisible(true);
        
        PaintPanelAtOffset(pPanel, finalX, finalY, scale);
        
        // Restore visibility state
        pPanel->SetVisible(bWasVisible);
    }
    
    g_pMatSystemSurface->DisableClipping(false);
}

//-----------------------------------------------------------------------------
void CVRHUDCompositor::PaintPanelAtOffset(vgui::Panel* pPanel, int targetX, int targetY, float scale)
{
    if (!pPanel)
        return;
    
    // Get the panel's current absolute screen position
    int panelScreenX = 0, panelScreenY = 0;
    pPanel->LocalToScreen(panelScreenX, panelScreenY);
    
    // Calculate offset to move panel from its screen position to our target position
    // When rendering via DrawPanelIn3DSpace, the render target starts at (0,0)
    // so targetX/targetY are the absolute position we want within the render target
    int offsetX = targetX - panelScreenX;
    int offsetY = targetY - panelScreenY;
    
    // Apply the offset and paint
    vgui::surface()->ForceScreenPosOffset(true, offsetX, offsetY);
    vgui::surface()->PaintTraverse(pPanel->GetVPanel());
    vgui::surface()->ForceScreenPosOffset(false, 0, 0);
}

//=============================================================================
// CVRStatusHUDCompositor Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRStatusHUDCompositor);

//-----------------------------------------------------------------------------
CVRStatusHUDCompositor::CVRStatusHUDCompositor(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_pHealthPanel = nullptr;
    m_pObjectivePanel = nullptr;
    m_pMatchStatusPanel = nullptr;
    m_pMatchStatusElement = nullptr;
    m_nHealthSlotIndex = -1;
    m_nObjectiveSlotIndex = -1;
    m_nMatchStatusSlotIndex = -1;
}

//-----------------------------------------------------------------------------
bool CVRStatusHUDCompositor::Initialize()
{
    if (m_bInitialized)
        return true;
    
    RefreshHUDReferences();
    
    // Match status panel at top (team compositions + timer, dynamic - respects ShouldDraw)
    if (m_pMatchStatusPanel)
    {
        m_nMatchStatusSlotIndex = AddSlot(m_pMatchStatusPanel, m_pMatchStatusElement, "CTFHudMatchStatus", true);
    }
    
    // Health panel
    if (m_pHealthPanel)
    {
        m_nHealthSlotIndex = AddSlot(m_pHealthPanel, m_pHealthPanel, "CTFHudPlayerStatus", false);
    }
    
    // Objective panel below health
    if (m_pObjectivePanel)
    {
        m_nObjectiveSlotIndex = AddSlot(m_pObjectivePanel, m_pObjectivePanel, "CTFHudObjectiveStatus", false);
    }
    
    UpdateLayout();
    
    m_bInitialized = true;
    DevMsg("VR Status HUD Compositor: Initialized with %d HUD elements\n", m_nNumSlots);
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRStatusHUDCompositor::RefreshHUDReferences()
{
    m_pHealthPanel = GET_HUDELEMENT(CTFHudPlayerStatus);
    m_pObjectivePanel = GET_HUDELEMENT(CTFHudObjectiveStatus);
    
    // Get match status HUD (team compositions + timer) by name lookup
    CHudElement* pElement = gHUD.FindElement("CTFHudMatchStatus");
    m_pMatchStatusElement = pElement;
    m_pMatchStatusPanel = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    if (m_nHealthSlotIndex >= 0)
        m_HudSlots[m_nHealthSlotIndex].pPanel = m_pHealthPanel;
    
    if (m_nObjectiveSlotIndex >= 0)
        m_HudSlots[m_nObjectiveSlotIndex].pPanel = m_pObjectivePanel;
    
    if (m_nMatchStatusSlotIndex >= 0)
    {
        m_HudSlots[m_nMatchStatusSlotIndex].pPanel = m_pMatchStatusPanel;
        m_HudSlots[m_nMatchStatusSlotIndex].pHudElement = m_pMatchStatusElement;
    }
}

//-----------------------------------------------------------------------------
void CVRStatusHUDCompositor::UpdateLayout()
{
    float sf = GetVRScreenScaleFactor();
    SetCompositorSize((int)(tfvr_status_hud_width.GetInt() * sf), (int)(tfvr_status_hud_height.GetInt() * sf));
    m_bShowDebugBackground = tfvr_status_hud_debug_bg.GetBool();
    
    // Match status panel at top (team compositions + timer)
    if (m_nMatchStatusSlotIndex >= 0)
    {
        VRHudElementSlot_t& slot = m_HudSlots[m_nMatchStatusSlotIndex];
        slot.bEnabled = tfvr_status_hud_matchstatus_enabled.GetBool();
        slot.nOffsetX = (int)(tfvr_status_hud_matchstatus_x.GetInt() * sf);
        slot.nOffsetY = (int)(tfvr_status_hud_matchstatus_y.GetInt() * sf);
        slot.bCenterHorizontally = tfvr_status_hud_matchstatus_center.GetBool();
        slot.bCenterVertically = false;
        slot.nContentOffsetX = (int)(tfvr_status_hud_matchstatus_content_offset_x.GetInt() * sf);
        slot.nContentOffsetY = (int)(tfvr_status_hud_matchstatus_content_offset_y.GetInt() * sf);
        slot.flScale = tfvr_status_hud_matchstatus_scale.GetFloat();
    }
    
    // Health panel
    if (m_nHealthSlotIndex >= 0)
    {
        VRHudElementSlot_t& slot = m_HudSlots[m_nHealthSlotIndex];
        slot.bEnabled = tfvr_status_hud_health_enabled.GetBool();
        slot.nOffsetX = (int)(tfvr_status_hud_health_x.GetInt() * sf);
        slot.nOffsetY = (int)(tfvr_status_hud_health_y.GetInt() * sf);
        slot.bCenterHorizontally = tfvr_status_hud_health_center.GetBool();
        slot.bCenterVertically = tfvr_status_hud_health_center_v.GetBool();
        slot.nContentOffsetX = (int)(tfvr_status_hud_health_content_offset_x.GetInt() * sf);
        slot.nContentOffsetY = (int)(tfvr_status_hud_health_content_offset_y.GetInt() * sf);
        slot.flScale = tfvr_status_hud_health_scale.GetFloat();
    }
    
    // Objective panel below health
    if (m_nObjectiveSlotIndex >= 0)
    {
        VRHudElementSlot_t& slot = m_HudSlots[m_nObjectiveSlotIndex];
        slot.bEnabled = tfvr_status_hud_objective_enabled.GetBool();
        slot.nOffsetX = (int)(tfvr_status_hud_objective_x.GetInt() * sf);
        slot.nOffsetY = (int)(tfvr_status_hud_objective_y.GetInt() * sf);
        slot.bCenterHorizontally = tfvr_status_hud_objective_center.GetBool();
        slot.bCenterVertically = tfvr_status_hud_objective_center_v.GetBool();
        slot.nContentOffsetX = (int)(tfvr_status_hud_objective_content_offset_x.GetInt() * sf);
        slot.nContentOffsetY = (int)(tfvr_status_hud_objective_content_offset_y.GetInt() * sf);
        slot.flScale = tfvr_status_hud_objective_scale.GetFloat();
    }
}

//=============================================================================
// CVRWeaponHUDCompositor Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRWeaponHUDCompositor);

//-----------------------------------------------------------------------------
CVRWeaponHUDCompositor::CVRWeaponHUDCompositor(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_pAmmoPanel = nullptr;
    m_pDemomanPipes = nullptr;
    m_pDemomanPipesElement = nullptr;
    m_pBowCharge = nullptr;
    m_pBowChargeElement = nullptr;
    m_pDemomanCharge = nullptr;
    m_pDemomanChargeElement = nullptr;
    m_pMedicCharge = nullptr;
    m_pMedicChargeElement = nullptr;
    m_pFlameRocketCharge = nullptr;
    m_pFlameRocketChargeElement = nullptr;
    m_pAccountPanel = nullptr;
    m_pAccountPanelElement = nullptr;
    
    m_nAmmoSlotIndex = -1;
    m_nDemomanPipesSlotIndex = -1;
    m_nBowChargeSlotIndex = -1;
    m_nDemomanChargeSlotIndex = -1;
    m_nMedicChargeSlotIndex = -1;
    m_nFlameRocketChargeSlotIndex = -1;
    m_nAccountPanelSlotIndex = -1;
    m_nFirstMeterSlotIndex = -1;
    m_nNumMeterSlots = 0;
}

//-----------------------------------------------------------------------------
bool CVRWeaponHUDCompositor::Initialize()
{
    if (m_bInitialized)
        return true;
    
    RefreshHUDReferences();
    
    // Add ammo panel first - dynamic so it respects ShouldDraw (hides on melee, etc.)
    if (m_pAmmoPanel)
    {
        m_nAmmoSlotIndex = AddSlot(m_pAmmoPanel, m_pAmmoPanel, "CTFHudWeaponAmmo", true);
    }
    
    // Add class-specific HUD elements (marked as dynamic so they respect ShouldDraw)
    if (m_pDemomanPipes)
    {
        m_nDemomanPipesSlotIndex = AddSlot(m_pDemomanPipes, m_pDemomanPipesElement, "CHudDemomanPipes", true);
    }
    
    if (m_pBowCharge)
    {
        m_nBowChargeSlotIndex = AddSlot(m_pBowCharge, m_pBowChargeElement, "CHudBowChargeMeter", true);
        // Bow charge meter layers on top of ammo
        m_HudSlots[m_nBowChargeSlotIndex].bLayerOnAmmo = true;
    }
    
    if (m_pDemomanCharge)
    {
        m_nDemomanChargeSlotIndex = AddSlot(m_pDemomanCharge, m_pDemomanChargeElement, "CHudDemomanChargeMeter", true);
        // Sticky charge meter layers on top of ammo
        m_HudSlots[m_nDemomanChargeSlotIndex].bLayerOnAmmo = true;
    }
    
    if (m_pMedicCharge)
    {
        m_nMedicChargeSlotIndex = AddSlot(m_pMedicCharge, m_pMedicChargeElement, "CHudMedicChargeMeter", true);
    }
    
    if (m_pFlameRocketCharge)
    {
        m_nFlameRocketChargeSlotIndex = AddSlot(m_pFlameRocketCharge, m_pFlameRocketChargeElement, "CHudFlameRocketChargeMeter", true);
    }
    
    if (m_pAccountPanel)
    {
        m_nAccountPanelSlotIndex = AddSlot(m_pAccountPanel, m_pAccountPanelElement, "CHudAccountPanel", true);
    }
    
    // Mark where dynamic meters start
    m_nFirstMeterSlotIndex = m_nNumSlots;
    
    // Gather item effect meters
    GatherItemEffectMeters();
    
    UpdateLayout();
    
    m_bInitialized = true;
    DevMsg("VR Weapon HUD Compositor: Initialized with %d HUD elements\n", m_nNumSlots);
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDCompositor::RefreshHUDReferences()
{
    m_pAmmoPanel = GET_HUDELEMENT(CTFHudWeaponAmmo);
    
    // Get class-specific HUD elements by name lookup
    // These are registered with DECLARE_HUDELEMENT so we can find them
    // Store both the Panel pointer and CHudElement pointer for ShouldDraw checks
    CHudElement* pElement = nullptr;
    
    pElement = gHUD.FindElement("CHudDemomanPipes");
    m_pDemomanPipesElement = pElement;
    m_pDemomanPipes = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    pElement = gHUD.FindElement("CHudBowChargeMeter");
    m_pBowChargeElement = pElement;
    m_pBowCharge = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    pElement = gHUD.FindElement("CHudDemomanChargeMeter");
    m_pDemomanChargeElement = pElement;
    m_pDemomanCharge = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    pElement = gHUD.FindElement("CHudMedicChargeMeter");
    m_pMedicChargeElement = pElement;
    m_pMedicCharge = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    pElement = gHUD.FindElement("CHudFlameRocketChargeMeter");
    m_pFlameRocketChargeElement = pElement;
    m_pFlameRocketCharge = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    pElement = gHUD.FindElement("CHudAccountPanel");
    m_pAccountPanelElement = pElement;
    m_pAccountPanel = pElement ? dynamic_cast<vgui::Panel*>(pElement) : nullptr;
    
    // Update existing slot references (both panel and CHudElement)
    if (m_nAmmoSlotIndex >= 0)
    {
        m_HudSlots[m_nAmmoSlotIndex].pPanel = m_pAmmoPanel;
        m_HudSlots[m_nAmmoSlotIndex].pHudElement = m_pAmmoPanel; // CTFHudWeaponAmmo is a CHudElement
    }
    if (m_nDemomanPipesSlotIndex >= 0)
    {
        m_HudSlots[m_nDemomanPipesSlotIndex].pPanel = m_pDemomanPipes;
        m_HudSlots[m_nDemomanPipesSlotIndex].pHudElement = m_pDemomanPipesElement;
    }
    if (m_nBowChargeSlotIndex >= 0)
    {
        m_HudSlots[m_nBowChargeSlotIndex].pPanel = m_pBowCharge;
        m_HudSlots[m_nBowChargeSlotIndex].pHudElement = m_pBowChargeElement;
    }
    if (m_nDemomanChargeSlotIndex >= 0)
    {
        m_HudSlots[m_nDemomanChargeSlotIndex].pPanel = m_pDemomanCharge;
        m_HudSlots[m_nDemomanChargeSlotIndex].pHudElement = m_pDemomanChargeElement;
    }
    if (m_nMedicChargeSlotIndex >= 0)
    {
        m_HudSlots[m_nMedicChargeSlotIndex].pPanel = m_pMedicCharge;
        m_HudSlots[m_nMedicChargeSlotIndex].pHudElement = m_pMedicChargeElement;
    }
    if (m_nFlameRocketChargeSlotIndex >= 0)
    {
        m_HudSlots[m_nFlameRocketChargeSlotIndex].pPanel = m_pFlameRocketCharge;
        m_HudSlots[m_nFlameRocketChargeSlotIndex].pHudElement = m_pFlameRocketChargeElement;
    }
    if (m_nAccountPanelSlotIndex >= 0)
    {
        m_HudSlots[m_nAccountPanelSlotIndex].pPanel = m_pAccountPanel;
        m_HudSlots[m_nAccountPanelSlotIndex].pHudElement = m_pAccountPanelElement;
    }
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDCompositor::GatherItemEffectMeters()
{
    // Clear existing dynamic meter slots
    m_nNumMeterSlots = 0;
    
    // Iterate through all item effect meters via the auto list
    for (int i = 0; i < IHudItemEffectMeterAutoList::AutoList().Count(); i++)
    {
        CHudItemEffectMeter* pMeter = static_cast<CHudItemEffectMeter*>(IHudItemEffectMeterAutoList::AutoList()[i]);
        if (!pMeter)
            continue;
        
        // Only add if we have room
        if (m_nNumSlots >= MAX_HUD_SLOTS)
            break;
        
        // Store both the panel pointer and the CHudElement pointer for ShouldDraw checks
        int slotIndex = AddSlot(pMeter, pMeter, "ItemEffectMeter", true);
        if (slotIndex >= 0)
        {
            m_nNumMeterSlots++;
        }
    }
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDCompositor::RefreshDynamicElements()
{
    // Remove dynamic slots
    if (m_nFirstMeterSlotIndex >= 0)
    {
        m_nNumSlots = m_nFirstMeterSlotIndex;
        for (int i = m_nFirstMeterSlotIndex; i < MAX_HUD_SLOTS; i++)
        {
            m_HudSlots[i].pPanel = nullptr;
        }
    }
    
    // Re-gather effect meters
    GatherItemEffectMeters();
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDCompositor::UpdateLayout()
{
    float sf = GetVRScreenScaleFactor();
    SetCompositorSize((int)(tfvr_weapon_hud_width.GetInt() * sf), (int)(tfvr_weapon_hud_height.GetInt() * sf));
    m_bShowDebugBackground = tfvr_weapon_hud_debug_bg.GetBool();
    
    // Ammo panel layout
    if (m_nAmmoSlotIndex >= 0)
    {
        VRHudElementSlot_t& slot = m_HudSlots[m_nAmmoSlotIndex];
        slot.bEnabled = tfvr_weapon_hud_ammo_enabled.GetBool();
        slot.nOffsetX = (int)(tfvr_weapon_hud_ammo_x.GetInt() * sf);
        slot.nOffsetY = (int)(tfvr_weapon_hud_ammo_y.GetInt() * sf);
        slot.bCenterHorizontally = tfvr_weapon_hud_ammo_center.GetBool();
        slot.bCenterVertically = false;
        slot.nContentOffsetX = (int)(tfvr_weapon_hud_ammo_content_offset_x.GetInt() * sf);
        slot.nContentOffsetY = (int)(tfvr_weapon_hud_ammo_content_offset_y.GetInt() * sf);
        slot.flScale = tfvr_weapon_hud_ammo_scale.GetFloat();
    }
    
    // Update meter layout (stacked horizontally below ammo)
    UpdateMeterLayout();
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDCompositor::UpdateMeterLayout()
{
    float sf = GetVRScreenScaleFactor();
    int metersY = (int)(tfvr_weapon_hud_meters_y.GetInt() * sf);
    int spacing = (int)(tfvr_weapon_hud_meters_spacing.GetInt() * sf);
    int rawWidthOverride = tfvr_weapon_hud_meters_width_override.GetInt();
    int widthOverride = rawWidthOverride > 0 ? (int)(rawWidthOverride * sf) : 0;
    int contentOffsetX = (int)(tfvr_weapon_hud_meters_content_offset_x.GetInt() * sf);
    
    // Get ammo position for layered elements
    int ammoX = 0, ammoY = 0;
    if (m_nAmmoSlotIndex >= 0)
    {
        VRHudElementSlot_t& ammoSlot = m_HudSlots[m_nAmmoSlotIndex];
        ammoX = ammoSlot.nOffsetX + ammoSlot.nContentOffsetX;
        ammoY = ammoSlot.nOffsetY + ammoSlot.nContentOffsetY;
        if (ammoSlot.bCenterHorizontally)
        {
            vgui::Panel* pAmmoPanel = GetSlotPanel(ammoSlot);
            if (pAmmoPanel)
            {
                int ammoW, ammoH;
                pAmmoPanel->GetSize(ammoW, ammoH);
                ammoX = (m_nCompositorWidth - ammoW) / 2 + ammoSlot.nOffsetX + ammoSlot.nContentOffsetX;
            }
        }
    }
    
    // Build list of visible meter slots and their panels
    struct MeterInfo {
        int slotIndex;
        vgui::Panel* pPanel;
        int width;
        int height;
    };
    CUtlVector<MeterInfo> visibleMeters;
    int maxHeight = 0;
    
    // First pass: gather visible meters
    for (int i = 0; i < m_nNumSlots; i++)
    {
        VRHudElementSlot_t& slot = m_HudSlots[i];
        
        // Skip non-meter slots (ammo is handled separately)
        if (i == m_nAmmoSlotIndex)
            continue;
        
        // Skip elements that layer on ammo (they don't go in horizontal stack)
        if (slot.bLayerOnAmmo)
        {
            slot.bCenterHorizontally = false;
            slot.bCenterVertically = false;
            slot.nOffsetX = ammoX + (int)(tfvr_weapon_hud_charge_offset_x.GetInt() * sf);
            slot.nOffsetY = ammoY + (int)(tfvr_weapon_hud_charge_offset_y.GetInt() * sf);
            continue;
        }
        
        // Check if this slot should draw (handles ShouldDraw for dynamic elements)
        if (slot.bDynamic && !slot.ShouldDraw())
            continue;
        
        // Look up the panel fresh
        vgui::Panel* pPanel = GetSlotPanel(slot);
        if (!pPanel)
            continue;
        
        int panelWidth, panelHeight;
        pPanel->GetSize(panelWidth, panelHeight);
        
        if (panelWidth <= 0 || panelHeight <= 0)
            continue;
        
        MeterInfo info;
        info.slotIndex = i;
        info.pPanel = pPanel;
        info.width = panelWidth;
        info.height = panelHeight;
        visibleMeters.AddToTail(info);
        
        // Track the tallest meter for bottom alignment
        if (panelHeight > maxHeight)
            maxHeight = panelHeight;
    }
    
    // Calculate total width
    int totalWidth = 0;
    for (int i = 0; i < visibleMeters.Count(); i++)
    {
        int effectiveWidth = (widthOverride > 0) ? widthOverride : visibleMeters[i].width;
        if (i > 0)
            totalWidth += spacing;
        totalWidth += effectiveWidth;
    }
    
    // Second pass: position meters (in reverse order for right-to-left display)
    // All meters aligned at same Y (metersY) - vanilla uses top alignment
    int currentX = (m_nCompositorWidth - totalWidth) / 2;
    
    for (int i = visibleMeters.Count() - 1; i >= 0; i--)
    {
        MeterInfo& info = visibleMeters[i];
        VRHudElementSlot_t& slot = m_HudSlots[info.slotIndex];
        
        slot.bCenterHorizontally = false;
        slot.bCenterVertically = false;
        slot.nContentOffsetX = contentOffsetX;
        
        slot.nOffsetX = currentX;
        slot.nOffsetY = metersY;  // Same Y for all meters
        
        int effectiveWidth = (widthOverride > 0) ? widthOverride : info.width;
        currentX += effectiveWidth + spacing;
    }
    
    // Apply per-element offsets (metal/account panel can be repositioned independently)
    if (m_nAccountPanelSlotIndex >= 0)
    {
        VRHudElementSlot_t& accountSlot = m_HudSlots[m_nAccountPanelSlotIndex];
        accountSlot.nOffsetX += (int)(tfvr_weapon_hud_account_offset_x.GetInt() * sf);
        accountSlot.nOffsetY += (int)(tfvr_weapon_hud_account_offset_y.GetInt() * sf);
    }
}

//=============================================================================
// CVRHUDManager Base Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRHUDManager::CVRHUDManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    m_vOffset.Init(0, 0, 0);
    m_angRotation.Init(0, 0, 0);
    m_flScale = 15.0f;
}

//-----------------------------------------------------------------------------
CVRHUDManager::~CVRHUDManager()
{
}

//-----------------------------------------------------------------------------
void CVRHUDManager::Shutdown()
{
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
void CVRHUDManager::Update()
{
    // Base class does nothing
}

//-----------------------------------------------------------------------------
void CVRHUDManager::ResetState()
{
    DevMsg("VR HUD Manager: Resetting state\n");
}

//=============================================================================
// CVRStatusHUDManager Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRStatusHUDManager::CVRStatusHUDManager()
{
    m_nAttachedHand = 0;
    m_pCompositor = nullptr;
}

//-----------------------------------------------------------------------------
CVRStatusHUDManager::~CVRStatusHUDManager()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRStatusHUDManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    vgui::Panel* pParent = g_pClientMode->GetViewport();
    if (!pParent)
    {
        Warning("VR Status HUD Manager: Could not get client mode viewport\n");
        return false;
    }
    
    m_pCompositor = new CVRStatusHUDCompositor(pParent, "VRStatusHUDCompositor");
    if (!m_pCompositor)
    {
        Warning("VR Status HUD Manager: Could not create compositor panel\n");
        return false;
    }
    
    m_pCompositor->Initialize();
    m_pCompositor->SetVisible(false);
    
    m_bInitialized = true;
    DevMsg("VR Status HUD Manager: Initialized successfully\n");
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::Shutdown()
{
    if (m_pCompositor)
    {
        m_pCompositor->Shutdown();
        m_pCompositor->MarkForDeletion();
        m_pCompositor = nullptr;
    }
    
    CVRHUDManager::Shutdown();
}

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::Update()
{
    if (!m_bInitialized || !m_pCompositor)
        return;
    
    m_bEnabled = tfvr_status_hud_enabled.GetBool();
    m_nAttachedHand = tfvr_status_hud_hand.GetInt();
    
    m_vOffset.Init(
        tfvr_status_hud_offset_x.GetFloat(),
        tfvr_status_hud_offset_y.GetFloat(),
        tfvr_status_hud_offset_z.GetFloat()
    );
    
    m_angRotation.Init(
        tfvr_status_hud_pitch.GetFloat(),
        tfvr_status_hud_yaw.GetFloat(),
        tfvr_status_hud_roll.GetFloat()
    );
    
    m_flScale = tfvr_status_hud_scale.GetFloat();
}

// Priority for hand HUDs (rendered close to player)
static const int PRIORITY_HAND_HUD = 200;

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::Render()
{
    VPROF("VRStatusHUDManager_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pCompositor)
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive())
        return;
    
    VMatrix panelToWorld;
    if (!CalculateHandTransform(panelToWorld))
        return;
    
    int compositorWidth, compositorHeight;
    m_pCompositor->GetCompositorSize(compositorWidth, compositorHeight);
    
    float aspectRatio = (float)compositorWidth / (float)compositorHeight;
    float worldHeight = m_flScale;
    float worldWidth = worldHeight * aspectRatio;
    
    if (tfvr_status_hud_center_on_palm.GetBool())
    {
        ApplyCenteringOffset(panelToWorld, worldWidth, worldHeight);
    }
    
    // Skip if panel is facing away from camera
    if (IsPanelBackfacing(panelToWorld))
        return;
    
    // Queue for distance-sorted rendering
    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        bool bWasVisible = m_pCompositor->IsVisible();
        g_pVRWorldUIQueue->QueuePanel(m_pCompositor, panelToWorld,
                                      compositorWidth, compositorHeight,
                                      worldWidth, worldHeight,
                                      PRIORITY_HAND_HUD, true, bWasVisible);
    }
    else
    {
        // Fallback: render immediately
        m_pCompositor->SetVisible(true);
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pCompositor->GetVPanel(),
            panelToWorld,
            compositorWidth,
            compositorHeight,
            worldWidth,
            worldHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        m_pCompositor->SetVisible(false);
    }
}

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::SetHandAttachment(int hand)
{
    m_nAttachedHand = clamp(hand, 0, 1);
}

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::ResetState()
{
    CVRHUDManager::ResetState();
    
    if (m_pCompositor)
    {
        m_pCompositor->Initialize();
    }
}

//-----------------------------------------------------------------------------
bool CVRStatusHUDManager::CalculateHandTransform(VMatrix& transform)
{
    if (tfvr_status_hud_use_hand_tracking.GetBool())
    {
        return CalculateHandTrackingTransform(transform);
    }
    else
    {
        return CalculateControllerTransform(transform);
    }
}

//-----------------------------------------------------------------------------
bool CVRStatusHUDManager::CalculateHandTrackingTransform(VMatrix& transform)
{
    if (!g_pOpenXRManager)
        return false;
    
    bool leftHand = (m_nAttachedHand == 0);
    Vector palmPosition;
    QAngle palmAngles;
    bool palmValid = false;
    
    // Try palm pose from action system (grip_surface or palm_ext)
    if (g_pOpenXRManager->IsPalmPoseSupported())
    {
        VMatrix palmMatrix;
        bool gotPalm = leftHand ?
            g_pOpenXRManager->GetLeftPalmPose(palmMatrix) :
            g_pOpenXRManager->GetRightPalmPose(palmMatrix);
        if (gotPalm)
        {
            palmPosition = palmMatrix.GetTranslation();
            MatrixAngles(palmMatrix.As3x4(), palmAngles);
            palmValid = true;
        }
    }
    
    // Fall back to hand tracking palm joint
    COpenXRHandTracker* handTracker = g_pOpenXRManager->GetHandTracker();
    if (!palmValid)
    {
        if (!handTracker)
            return false;
        bool handTracked = leftHand ? handTracker->IsLeftHandTracked() : handTracker->IsRightHandTracked();
        if (!handTracked)
            return false;
        if (!handTracker->GetHandJoint(leftHand, XR_HAND_JOINT_PALM_EXT, palmPosition, palmAngles))
            return false;
    }
    
    // Wrist from hand tracking (optional, for improved forward direction)
    Vector wristPosition;
    QAngle wristAngles;
    bool wristValid = handTracker && 
        handTracker->GetHandJoint(leftHand, XR_HAND_JOINT_WRIST_EXT, wristPosition, wristAngles);
    
    Vector handForward, handRight, handUp;
    if (wristValid)
    {
        handForward = (palmPosition - wristPosition).Normalized();
        AngleVectors(palmAngles, nullptr, &handRight, &handUp);
        if (!leftHand)
            handRight = -handRight;
    }
    else
    {
        AngleVectors(palmAngles, &handForward, &handRight, &handUp);
        if (!leftHand)
            handRight = -handRight;
    }
    
    Vector quadPosition = palmPosition;
    
    if (!tfvr_status_hud_center_on_palm.GetBool())
    {
        Vector backOfHandOffset = -handForward * 2.0f + handUp * 1.0f;
        quadPosition += backOfHandOffset;
    }
    
    Vector worldOffset = handRight * m_vOffset.x + handUp * m_vOffset.y + handForward * m_vOffset.z;
    quadPosition += worldOffset;
    
    transform.Identity();
    
    matrix3x4_t handMatrix;
    AngleMatrix(palmAngles, Vector(0, 0, 0), handMatrix);
    VMatrix handVMatrix;
    handVMatrix.CopyFrom3x4(handMatrix);
    transform = transform * handVMatrix;
    
    VMatrix baseRotMatrix;
    matrix3x4_t baseRot3x4;
    QAngle baseRotation(
        tfvr_status_hud_base_pitch.GetFloat(),
        tfvr_status_hud_base_yaw.GetFloat(),
        tfvr_status_hud_base_roll.GetFloat()
    );
    AngleMatrix(baseRotation, Vector(0, 0, 0), baseRot3x4);
    baseRotMatrix.CopyFrom3x4(baseRot3x4);
    transform = transform * baseRotMatrix;
    
    if (m_angRotation.x != 0 || m_angRotation.y != 0 || m_angRotation.z != 0)
    {
        matrix3x4_t rotMatrix;
        AngleMatrix(m_angRotation, Vector(0, 0, 0), rotMatrix);
        VMatrix rotVMatrix;
        rotVMatrix.CopyFrom3x4(rotMatrix);
        transform = transform * rotVMatrix;
    }
    
    transform.SetTranslation(quadPosition);
    
    return true;
}

//-----------------------------------------------------------------------------
bool CVRStatusHUDManager::CalculateControllerTransform(VMatrix& transform)
{
    if (!g_pOpenXRManager)
        return false;
    
    VMatrix handPose;
    bool handValid = false;
    
    if (m_nAttachedHand == 0)
    {
        if (g_pOpenXRManager->IsLeftControllerPoseValid())
            handValid = g_pOpenXRManager->GetLeftControllerGripPose(handPose);
    }
    else
    {
        if (g_pOpenXRManager->IsRightControllerPoseValid())
            handValid = g_pOpenXRManager->GetRightControllerGripPose(handPose);
    }
    
    if (!handValid)
        return false;
    
    Vector handPos = handPose.GetTranslation();
    Vector forward, right, up;
    handPose.GetBasisVectors(forward, right, up);
    
    Vector quadPos = handPos + 
                     right * m_vOffset.x + 
                     up * m_vOffset.y + 
                     forward * m_vOffset.z;
    
    transform = handPose;
    transform.SetTranslation(quadPos);
    
    VMatrix adjustMatrix;
    matrix3x4_t adjustMatrix3x4;
    QAngle adjustAngles(-90, -90, 90);
    adjustAngles += m_angRotation;
    AngleMatrix(adjustAngles, Vector(0, 0, 0), adjustMatrix3x4);
    adjustMatrix.CopyFrom3x4(adjustMatrix3x4);
    
    transform = transform * adjustMatrix;
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRStatusHUDManager::ApplyCenteringOffset(VMatrix& transform, float worldWidth, float worldHeight)
{
    Vector panelXAxis(transform[0][0], transform[1][0], transform[2][0]);
    Vector panelYAxis(transform[0][1], transform[1][1], transform[2][1]);
    
    Vector currentPos = transform.GetTranslation();
    Vector centeringOffset = -panelXAxis * (worldWidth * 0.5f) + panelYAxis * (worldHeight * 0.5f);
    transform.SetTranslation(currentPos + centeringOffset);
}

//=============================================================================
// CVRWeaponHUDManager Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRWeaponHUDManager::CVRWeaponHUDManager()
{
    m_pCompositor = nullptr;
    m_hLastWeapon = nullptr;
    m_bWeaponOnLeftHand = false;
}

//-----------------------------------------------------------------------------
CVRWeaponHUDManager::~CVRWeaponHUDManager()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRWeaponHUDManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    vgui::Panel* pParent = g_pClientMode->GetViewport();
    if (!pParent)
    {
        Warning("VR Weapon HUD Manager: Could not get client mode viewport\n");
        return false;
    }
    
    m_pCompositor = new CVRWeaponHUDCompositor(pParent, "VRWeaponHUDCompositor");
    if (!m_pCompositor)
    {
        Warning("VR Weapon HUD Manager: Could not create compositor panel\n");
        return false;
    }
    
    m_pCompositor->Initialize();
    m_pCompositor->SetVisible(false);
    
    m_bInitialized = true;
    DevMsg("VR Weapon HUD Manager: Initialized successfully\n");
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDManager::Shutdown()
{
    if (m_pCompositor)
    {
        m_pCompositor->Shutdown();
        m_pCompositor->MarkForDeletion();
        m_pCompositor = nullptr;
    }
    
    CVRHUDManager::Shutdown();
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDManager::Update()
{
    if (!m_bInitialized || !m_pCompositor)
        return;
    
    m_bEnabled = tfvr_weapon_hud_enabled.GetBool();
    
    m_vOffset.Init(
        tfvr_weapon_hud_offset_x.GetFloat(),
        tfvr_weapon_hud_offset_y.GetFloat(),
        tfvr_weapon_hud_offset_z.GetFloat()
    );
    
    m_angRotation.Init(
        tfvr_weapon_hud_pitch.GetFloat(),
        tfvr_weapon_hud_yaw.GetFloat(),
        tfvr_weapon_hud_roll.GetFloat()
    );
    
    m_flScale = tfvr_weapon_hud_scale.GetFloat();
    
    // Check for weapon change and refresh dynamic elements
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (pPlayer)
    {
        C_BaseCombatWeapon* pWeapon = pPlayer->GetActiveWeapon();
        if (pWeapon != m_hLastWeapon.Get())
        {
            m_hLastWeapon = pWeapon;
            m_pCompositor->RefreshDynamicElements();
            
            // Medigun is held on left hand; all other weapons on right
            CTFWeaponBase* pTFWeapon = pPlayer->GetActiveTFWeapon();
            m_bWeaponOnLeftHand = (pTFWeapon && pTFWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN);
        }
    }
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDManager::Render()
{
    VPROF("VRWeaponHUDManager_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pCompositor)
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive())
        return;
    
    // VR: Don't show weapon HUD if no weapon is equipped (e.g., during loser/stalemate)
    C_TFVRHand* pWeaponHand = m_bWeaponOnLeftHand ? GetLocalPlayerLeftHand() : GetLocalPlayerRightHand();
    if (!pWeaponHand || !pWeaponHand->GetHeldWeapon())
        return;
    
    VMatrix panelToWorld;
    if (!CalculateWeaponBoneTransform(panelToWorld))
        return;
    
    int compositorWidth, compositorHeight;
    m_pCompositor->GetCompositorSize(compositorWidth, compositorHeight);
    
    float aspectRatio = (float)compositorWidth / (float)compositorHeight;
    float worldHeight = m_flScale;
    float worldWidth = worldHeight * aspectRatio;
    
    if (tfvr_weapon_hud_center.GetBool())
    {
        ApplyCenteringOffset(panelToWorld, worldWidth, worldHeight);
    }
    
    // Skip if panel is facing away from camera
    if (IsPanelBackfacing(panelToWorld))
        return;
    
    // Queue for distance-sorted rendering
    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        bool bWasVisible = m_pCompositor->IsVisible();
        g_pVRWorldUIQueue->QueuePanel(m_pCompositor, panelToWorld,
                                      compositorWidth, compositorHeight,
                                      worldWidth, worldHeight,
                                      PRIORITY_HAND_HUD, true, bWasVisible);
    }
    else
    {
        // Fallback: render immediately
        m_pCompositor->SetVisible(true);
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pCompositor->GetVPanel(),
            panelToWorld,
            compositorWidth,
            compositorHeight,
            worldWidth,
            worldHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        m_pCompositor->SetVisible(false);
    }
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDManager::ResetState()
{
    CVRHUDManager::ResetState();
    
    if (m_pCompositor)
    {
        m_pCompositor->Initialize();
        m_pCompositor->RefreshDynamicElements();
    }
    
    m_hLastWeapon = nullptr;
    m_bWeaponOnLeftHand = false;
}

//-----------------------------------------------------------------------------
bool CVRWeaponHUDManager::CalculateWeaponBoneTransform(VMatrix& transform)
{
    // Get the hand that holds the weapon (left for medigun, right for everything else)
    C_TFVRHand* pHand = m_bWeaponOnLeftHand ? GetLocalPlayerLeftHand() : GetLocalPlayerRightHand();
    if (!pHand)
        return false;
    
    bool bGotTransform = false;
    
    // Try 1: Use cached weapon bone transform from the hand
    matrix3x4_t cachedBoneMatrix;
    if (pHand->GetCachedWeaponBoneTransform(cachedBoneMatrix))
    {
        transform.CopyFrom3x4(cachedBoneMatrix);
        bGotTransform = true;
    }
    
    // Try 2: Fallback to render weapon's weapon_bone
    if (!bGotTransform)
    {
        C_BaseAnimating* pRenderWeapon = pHand->GetRenderWeapon();
        if (pRenderWeapon)
        {
            // Left-hand medigun uses weapon_bone_L
            int weaponBone = -1;
            if (m_bWeaponOnLeftHand)
                weaponBone = pRenderWeapon->LookupBone("weapon_bone_L");
            if (weaponBone < 0)
                weaponBone = pRenderWeapon->LookupBone("weapon_bone");
            
            if (weaponBone >= 0)
            {
                matrix3x4_t boneMatrix;
                pRenderWeapon->GetBoneTransform(weaponBone, boneMatrix);
                transform.CopyFrom3x4(boneMatrix);
                bGotTransform = true;
            }
            else
            {
                Vector weaponPos = pRenderWeapon->GetAbsOrigin();
                QAngle weaponAngles = pRenderWeapon->GetAbsAngles();
                
                matrix3x4_t weaponMatrix;
                AngleMatrix(weaponAngles, weaponPos, weaponMatrix);
                transform.CopyFrom3x4(weaponMatrix);
                bGotTransform = true;
            }
        }
    }
    
    // Try 3: Fallback to hand position
    if (!bGotTransform)
    {
        Vector handPos = pHand->GetAbsOrigin();
        QAngle handAngles = pHand->GetAbsAngles();
        
        matrix3x4_t handMatrix;
        AngleMatrix(handAngles, handPos, handMatrix);
        transform.CopyFrom3x4(handMatrix);
        bGotTransform = true;
    }
    
    if (!bGotTransform)
        return false;
    
    // Apply user offsets in weapon space
    // Flip X offset for left-hand weapons so the HUD mirrors to the other side
    if (m_vOffset.x != 0 || m_vOffset.y != 0 || m_vOffset.z != 0)
    {
        Vector weaponForward, weaponRight, weaponUp;
        transform.GetBasisVectors(weaponForward, weaponRight, weaponUp);
        
        float offsetX = m_bWeaponOnLeftHand ? -m_vOffset.x : m_vOffset.x;
        
        Vector worldOffset = weaponRight * offsetX + 
                             weaponUp * m_vOffset.y + 
                             weaponForward * m_vOffset.z;
        
        Vector currentPos = transform.GetTranslation();
        transform.SetTranslation(currentPos + worldOffset);
    }
    
    // Apply rotation adjustments
    // Flip yaw and roll for left-hand weapons to mirror the HUD orientation
    if (m_angRotation.x != 0 || m_angRotation.y != 0 || m_angRotation.z != 0)
    {
        QAngle adjustedRotation = m_angRotation;
        if (m_bWeaponOnLeftHand)
        {
            adjustedRotation.y = -adjustedRotation.y;
            adjustedRotation.z = -adjustedRotation.z;
        }
        
        VMatrix rotationMatrix;
        matrix3x4_t rotMatrix;
        AngleMatrix(adjustedRotation, Vector(0, 0, 0), rotMatrix);
        rotationMatrix.CopyFrom3x4(rotMatrix);
        
        transform = transform * rotationMatrix;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRWeaponHUDManager::ApplyCenteringOffset(VMatrix& transform, float worldWidth, float worldHeight)
{
    Vector panelXAxis(transform[0][0], transform[1][0], transform[2][0]);
    Vector panelYAxis(transform[0][1], transform[1][1], transform[2][1]);
    
    Vector currentPos = transform.GetTranslation();
    Vector centeringOffset = -panelXAxis * (worldWidth * 0.5f) + panelYAxis * (worldHeight * 0.5f);
    transform.SetTranslation(currentPos + centeringOffset);
}

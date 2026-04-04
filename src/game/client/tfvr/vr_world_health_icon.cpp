// Purpose: VR World Health Icon - Renders floating health icons in 3D world
//          space above players' heads, always facing the player's view.
//
// NOTE: This manager renders immediately rather than using the global UI queue
//       because it reuses wrapper panels for multiple entities in a loop.
//       The panel data changes for each entity, so queuing doesn't work.

#include "cbase.h"
#include "vr_world_health_icon.h"
#include "c_tf_player.h"
#include "c_baseobject.h"
#include "tf_hud_target_id.h"
#include "hudelement.h"
#include "hud.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "tier0/vprof.h"
#include "sourcevr/isourcevirtualreality.h"
#include "vr_menu_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// CVRHealthIconWrapper Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRHealthIconWrapper);

CVRHealthIconWrapper::CVRHealthIconWrapper(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_pTargetPanel = nullptr;
    SetPaintBackgroundEnabled(false);
}

void CVRHealthIconWrapper::Paint()
{
    if (!m_pTargetPanel)
        return;
    
    // Get our size and the target panel size
    int wrapperWidth, wrapperHeight;
    GetSize(wrapperWidth, wrapperHeight);
    
    int targetWidth, targetHeight;
    m_pTargetPanel->GetSize(targetWidth, targetHeight);
    
    // Calculate offset to center the target panel content within the wrapper
    int offsetX = (wrapperWidth - targetWidth) / 2;
    int offsetY = (wrapperHeight - targetHeight) / 2;
    
    // Temporarily make the target panel visible
    bool bWasVisible = m_pTargetPanel->IsVisible();
    m_pTargetPanel->SetVisible(true);
    
    // Disable clipping
    g_pMatSystemSurface->DisableClipping(true);
    
    // Apply offset to center the content, then paint the target panel
    vgui::surface()->ForceScreenPosOffset(true, offsetX, offsetY);
    vgui::surface()->PaintTraverse(m_pTargetPanel->GetVPanel());
    vgui::surface()->ForceScreenPosOffset(false, 0, 0);
    
    // Restore clipping
    g_pMatSystemSurface->DisableClipping(false);
    
    // Restore visibility
    m_pTargetPanel->SetVisible(bWasVisible);
}

// Global instance
CVRWorldHealthIconManager* g_pVRWorldHealthIconManager = nullptr;

// Static member for 3D rendering flag
bool CVRWorldHealthIconManager::s_bIsRendering3D = false;

//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_world_health_enabled("tfvr_world_health_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR world-space health icons above players");
ConVar tfvr_world_health_scale("tfvr_world_health_scale", "0.15", FCVAR_ARCHIVE, 
    "Scale of the world health icon (world height = scale * 100 units)");
ConVar tfvr_world_health_height_offset("tfvr_world_health_height_offset", "15", FCVAR_ARCHIVE, 
    "Additional height offset above player head");
ConVar tfvr_world_health_offset_x("tfvr_world_health_offset_x", "0", FCVAR_ARCHIVE, 
    "Horizontal offset in world units (positive = right)");
ConVar tfvr_world_health_anchor("tfvr_world_health_anchor", "1.0", FCVAR_ARCHIVE, 
    "How much health follows tag edge: 1.0=full left edge, 0.5=halfway, 0.0=centered");
ConVar tfvr_world_health_offset_y("tfvr_world_health_offset_y", "0", FCVAR_ARCHIVE, 
    "Forward/back offset in world units (positive = toward player)");
ConVar tfvr_world_health_render_scale("tfvr_world_health_render_scale", "4", FCVAR_ARCHIVE,
    "Resolution multiplier for sharper text (1-8)");

// Target ID ConVars
ConVar tfvr_world_targetid_enabled("tfvr_world_targetid_enabled", "1", FCVAR_ARCHIVE,
    "Enable VR world-space target ID panel above players");
ConVar tfvr_world_targetid_scale("tfvr_world_targetid_scale", "0.12", FCVAR_ARCHIVE,
    "Scale of the target ID panel (world height = scale * 100 units)");
ConVar tfvr_world_targetid_offset("tfvr_world_targetid_offset", "10", FCVAR_ARCHIVE,
    "Height offset of target ID above the health icon (for players)");
ConVar tfvr_world_targetid_building_offset("tfvr_world_targetid_building_offset", "20", FCVAR_ARCHIVE,
    "Height offset of target ID above buildings");
ConVar tfvr_world_targetid_render_scale("tfvr_world_targetid_render_scale", "2", FCVAR_ARCHIVE,
    "Resolution multiplier for sharper text (1-8, higher = sharper but more expensive)");
ConVar tfvr_world_health_debug("tfvr_world_health_debug", "0", FCVAR_ARCHIVE,
    "Debug output for world health icon rendering");

//=============================================================================
// CVRWorldHealthIconManager Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRWorldHealthIconManager::CVRWorldHealthIconManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    m_pWrapper = nullptr;
    m_pTargetIDWrapper = nullptr;
    
    m_flScale = 0.15f;
    m_flHeightOffset = 15.0f;
    m_flOffsetX = 0.0f;
    m_flOffsetY = 0.0f;
    m_nRenderScale = 4;
    
    m_flTargetIDScale = 0.12f;
    m_flTargetIDOffset = 10.0f;
    m_flTargetIDBuildingOffset = 20.0f;
    m_nLastTargetIndex = 0;
    m_nFramesSinceTargetChange = 0;
}

//-----------------------------------------------------------------------------
CVRWorldHealthIconManager::~CVRWorldHealthIconManager()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRWorldHealthIconManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Create the wrapper panel - parent to the viewport
    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    m_pWrapper = new CVRHealthIconWrapper(pViewport, "VRHealthIconWrapper");
    m_pWrapper->SetBounds(0, 0, 512, 512);  // Large enough for high-res rendering
    m_pWrapper->SetVisible(false);  // Hidden normally, only visible during 3D render
    
    // Create wrapper for target ID panel
    m_pTargetIDWrapper = new CVRHealthIconWrapper(pViewport, "VRTargetIDWrapper");
    m_pTargetIDWrapper->SetBounds(0, 0, 512, 512);
    m_pTargetIDWrapper->SetVisible(false);
    
    m_bInitialized = true;
    DevMsg("VR World Health Icon Manager: Initialized\n");
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRWorldHealthIconManager::Shutdown()
{
    if (m_pWrapper)
    {
        m_pWrapper->MarkForDeletion();
        m_pWrapper = nullptr;
    }
    if (m_pTargetIDWrapper)
    {
        m_pTargetIDWrapper->MarkForDeletion();
        m_pTargetIDWrapper = nullptr;
    }
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
void CVRWorldHealthIconManager::ResetState()
{
    // Nothing to reset currently
}

//-----------------------------------------------------------------------------
void CVRWorldHealthIconManager::Update()
{
    if (!m_bInitialized)
        return;
    
    // Update configuration from ConVars
    m_bEnabled = tfvr_world_health_enabled.GetBool();
    m_flScale = tfvr_world_health_scale.GetFloat();
    m_flHeightOffset = tfvr_world_health_height_offset.GetFloat();
    m_flOffsetX = tfvr_world_health_offset_x.GetFloat();
    m_flOffsetY = tfvr_world_health_offset_y.GetFloat();
    m_nRenderScale = clamp(tfvr_world_health_render_scale.GetInt(), 1, 8);
    
    // Target ID configuration
    m_flTargetIDScale = tfvr_world_targetid_scale.GetFloat();
    m_flTargetIDOffset = tfvr_world_targetid_offset.GetFloat();
    m_flTargetIDBuildingOffset = tfvr_world_targetid_building_offset.GetFloat();
}

//-----------------------------------------------------------------------------
bool CVRWorldHealthIconManager::ShouldSuppressVanillaRendering()
{
    // Don't suppress if we're currently doing 3D rendering (let Paint() execute)
    if (s_bIsRendering3D)
        return false;
    
    // Suppress vanilla 2D rendering when VR world health is active
    return g_pVRWorldHealthIconManager && 
           g_pVRWorldHealthIconManager->m_bInitialized && 
           g_pVRWorldHealthIconManager->m_bEnabled &&
           UseVR();
}

//-----------------------------------------------------------------------------
Vector CVRWorldHealthIconManager::GetEntityHeadPosition(C_BaseEntity* pEntity)
{
    if (!pEntity)
        return vec3_origin;
    
    Vector vecTarget = pEntity->GetAbsOrigin();
    
    // Get the entity's bounding box height (no offsets - those are applied separately)
    C_BaseAnimating* pAnimating = pEntity->GetBaseAnimating();
    if (pAnimating)
    {
        vecTarget.z += VEC_HULL_MAX_SCALED(pAnimating).z;
    }
    
    // Add any entity-specific health bar offset
    vecTarget.z += pEntity->GetHealthBarHeightOffset();
    
    return vecTarget;
}

//-----------------------------------------------------------------------------
bool CVRWorldHealthIconManager::CalculateBillboardTransform(C_BaseEntity* pEntity, VMatrix& transform)
{
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || !pEntity)
        return false;
    
    // Get the world position above the entity's head
    Vector panelPos = GetEntityHeadPosition(pEntity);
    
    // Get the player's eye position for billboarding
    Vector vecEyePos = MainViewOrigin();
    
    // Calculate direction from panel to player (for billboarding)
    Vector forward = vecEyePos - panelPos;
    forward.z = 0; // Keep the icon upright (only rotate around Z axis)
    
    // Handle edge case where player is directly above/below
    if (forward.LengthSqr() < 0.001f)
    {
        forward = Vector(1, 0, 0);
    }
    VectorNormalize(forward);
    
    // Build orientation vectors
    Vector up(0, 0, 1);  // Keep upright
    Vector right;
    CrossProduct(up, forward, right);
    VectorNormalize(right);
    
    // Recalculate up to ensure orthogonality
    CrossProduct(forward, right, up);
    VectorNormalize(up);
    
    // Panel faces the player (negative forward direction)
    // Same pattern as vr_spring_hud.cpp
    Vector panelForward = -forward;  // Face the player
    Vector panelRight = right;
    Vector panelUp = up;
    
    // Note: Health-specific offsets are applied separately by the caller, not here
    
    // Build rotation matrix (column-major for VMatrix) - same as vr_spring_hud.cpp
    transform.Identity();
    transform[0][0] = panelRight.x;  transform[0][1] = panelUp.x;  transform[0][2] = panelForward.x;
    transform[1][0] = panelRight.y;  transform[1][1] = panelUp.y;  transform[1][2] = panelForward.y;
    transform[2][0] = panelRight.z;  transform[2][1] = panelUp.z;  transform[2][2] = panelForward.z;
    transform.SetTranslation(panelPos);
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRWorldHealthIconManager::Render()
{
    VPROF("VRWorldHealthIconManager_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pWrapper)
        return;
    
    if (!UseVR())
        return;
    
    if (g_pVRMenuManager && g_pVRMenuManager->IsMenuPanelOpen())
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver())
        return;
    
    // Check if player is actually aiming at a valid target
    int iIDTarget = pPlayer->GetIDTarget();
    if (iIDTarget <= 0)
    {
        m_nLastTargetIndex = 0;
        m_nFramesSinceTargetChange = 0;  // Reset frame counter when no target
        return;
    }
    
    // Get the target entity directly from target index (works for players AND buildings)
    C_BaseEntity* pEntity = cl_entitylist->GetEnt(iIDTarget);
    if (!pEntity || pEntity->IsDormant())
    {
        m_nLastTargetIndex = 0;
        m_nFramesSinceTargetChange = 0;
        return;
    }
    
    // Track frames since target changed - need frames for layout to settle
    if (iIDTarget != m_nLastTargetIndex)
    {
        m_nFramesSinceTargetChange = 0;
        m_nLastTargetIndex = iIDTarget;
    }
    else
    {
        m_nFramesSinceTargetChange++;
    }
    
    // Get the main target ID HUD element
    CMainTargetID* pTargetID = (CMainTargetID*)GET_HUDELEMENT(CMainTargetID);
    if (!pTargetID)
        return;
    
    // Set flag before ShouldDraw check so our query passes the VR suppression check
    s_bIsRendering3D = true;
    
    // Only render if the target ID would actually be shown (valid friendly target, not doors, etc.)
    if (!pTargetID->ShouldDraw())
    {
        s_bIsRendering3D = false;
        // Reset frame counter - panel hasn't been through a layout cycle yet
        m_nFramesSinceTargetChange = 0;
        return;
    }
    
    // Check if this is a building
    bool bIsBuilding = dynamic_cast<C_BaseObject*>(pEntity) != nullptr;
    
    // Calculate distance from player to target for consistent sizing
    Vector panelPos = GetEntityHeadPosition(pEntity);
    Vector eyePos = MainViewOrigin();
    float distance = (panelPos - eyePos).Length();
    
    // Scale world size based on distance to keep apparent size consistent
    const float referenceDistance = 100.0f;
    float distanceScale = distance / referenceDistance;
    
    // Force target ID layout ONCE here so we get consistent dimensions for both health icon and tag
    // Flag is already set from before ShouldDraw check, so PerformLayout positions at (0,0)
    pTargetID->InvalidateLayout(true);
    
    // Get the target ID dimensions
    int targetIDPanelWidth, targetIDPanelHeight;
    pTargetID->GetSize(targetIDPanelWidth, targetIDPanelHeight);
    if (targetIDPanelWidth <= 0 || targetIDPanelHeight <= 0)
    {
        targetIDPanelWidth = 150;
        targetIDPanelHeight = 50;
    }
    
    // Calculate tag world dimensions ONCE
    float tagAspectRatio = (float)targetIDPanelWidth / (float)targetIDPanelHeight;
    float tagWorldHeight = m_flTargetIDScale * 100.0f * distanceScale;
    float tagWorldWidth = tagWorldHeight * tagAspectRatio;
    
    // Calculate tag height offset
    float tagHeightOffset = bIsBuilding ? m_flTargetIDBuildingOffset : m_flTargetIDOffset;
    
    s_bIsRendering3D = false;  // Reset for now, will set again when actually rendering
    
    // Get the floating health icon (may not exist for buildings)
    CFloatingHealthIcon* pFloatingIcon = pTargetID->GetFloatingHealthIcon();
    
    // Render health icon FIRST (so tag draws on top)
    if (pFloatingIcon && pFloatingIcon->GetEntity() == pEntity)
    {
        int basePanelWidth, basePanelHeight;
        pFloatingIcon->GetSize(basePanelWidth, basePanelHeight);
        
        if (basePanelWidth <= 0 || basePanelHeight <= 0)
        {
            basePanelWidth = 48;
            basePanelHeight = 48;
        }
        
        int renderWidth = basePanelWidth * m_nRenderScale;
        int renderHeight = basePanelHeight * m_nRenderScale;
        
        VMatrix panelToWorld;
        if (CalculateBillboardTransform(pEntity, panelToWorld))
        {
            float aspectRatio = (float)basePanelWidth / (float)basePanelHeight;
            float worldHeight = m_flScale * 100.0f * distanceScale;
            float worldWidth = worldHeight * aspectRatio;
            
            // Extract orientation vectors
            Vector panelRight(panelToWorld[0][0], panelToWorld[1][0], panelToWorld[2][0]);
            Vector panelUp(panelToWorld[0][1], panelToWorld[1][1], panelToWorld[2][1]);
            
            // Get tag center position (same as RenderTargetID uses)
            Vector tagCenter = GetEntityHeadPosition(pEntity);
            tagCenter.z += tagHeightOffset;  // Fixed offset, not scaled by distance
            
            // Anchor health icon relative to the tag
            // anchorFactor: 1.0 = full left edge, 0.5 = halfway between center and edge, 0.0 = center
            float anchorFactor = clamp(tfvr_world_health_anchor.GetFloat(), 0.0f, 1.0f);
            Vector anchorPos = tagCenter - panelRight * (tagWorldWidth * 0.5f * anchorFactor);
            
            // Position health icon with its right edge at the anchor point
            Vector topLeftPos = anchorPos - panelRight * worldWidth + panelUp * (worldHeight * 0.5f);
            
            // Apply health-specific offsets
            topLeftPos += panelRight * m_flOffsetX * distanceScale;
            topLeftPos.z += m_flHeightOffset * distanceScale;
            
            if (tfvr_world_health_debug.GetBool())
            {
                DevMsg("Health: panelWxH=%dx%d aspect=%.2f tagWorldW=%.1f healthWorldW=%.1f dist=%.0f\n",
                       targetIDPanelWidth, targetIDPanelHeight, tagAspectRatio, tagWorldWidth, worldWidth, distance);
            }
            
            panelToWorld.SetTranslation(topLeftPos);
            
            m_pWrapper->SetSize(renderWidth, renderHeight);
            m_pWrapper->SetTargetPanel(pFloatingIcon);
            
            s_bIsRendering3D = true;
            m_pWrapper->SetVisible(true);
            
            g_pMatSystemSurface->DrawPanelIn3DSpace(
                m_pWrapper->GetVPanel(),
                panelToWorld,
                renderWidth,
                renderHeight,
                worldWidth,
                worldHeight
            );
            
            m_pWrapper->SetVisible(false);
            s_bIsRendering3D = false;
        }
    }
    
    // Render target ID SECOND (so it draws on top of health icon)
    // Skip target ID for first few frames after target change to let layout settle
    if (tfvr_world_health_debug.GetBool())
    {
        DevMsg("TargetID: enabled=%d frames=%d panelW=%d\n", 
               tfvr_world_targetid_enabled.GetBool(), m_nFramesSinceTargetChange, targetIDPanelWidth);
    }
    
    if (tfvr_world_targetid_enabled.GetBool() && m_nFramesSinceTargetChange >= 2)
    {
        RenderTargetID(pEntity, pTargetID, distanceScale, bIsBuilding,
                       targetIDPanelWidth, targetIDPanelHeight, tagWorldWidth, tagWorldHeight, tagHeightOffset);
    }
}

//-----------------------------------------------------------------------------
void CVRWorldHealthIconManager::RenderTargetID(C_BaseEntity* pEntity, CMainTargetID* pTargetID, float distanceScale, bool bIsBuilding,
                                                int panelWidth, int panelHeight, float tagWorldWidth, float tagWorldHeight, float tagHeightOffset)
{
    if (!pEntity || !pTargetID || !m_pTargetIDWrapper)
        return;
    
    // Set flag so PerformLayout positions at (0,0)
    s_bIsRendering3D = true;
    
    // Use pre-calculated panel dimensions (already had layout forced in Render())
    int basePanelWidth = panelWidth;
    int basePanelHeight = panelHeight;
    
    // Scale up for sharper text
    int targetIDRenderScale = clamp(tfvr_world_targetid_render_scale.GetInt(), 1, 8);
    int renderWidth = basePanelWidth * targetIDRenderScale;
    int renderHeight = basePanelHeight * targetIDRenderScale;
    
    // Calculate billboard transform
    VMatrix panelToWorld;
    if (!CalculateBillboardTransform(pEntity, panelToWorld))
    {
        s_bIsRendering3D = false;
        return;
    }
    
    // Use pre-calculated world dimensions for consistency with health icon
    float worldWidth = tagWorldWidth;
    float worldHeight = tagWorldHeight;
    
    // Extract orientation vectors
    Vector panelRight(panelToWorld[0][0], panelToWorld[1][0], panelToWorld[2][0]);
    Vector panelUp(panelToWorld[0][1], panelToWorld[1][1], panelToWorld[2][1]);
    
    // Get position above entity head using the same offset as health icon used
    Vector targetIDPos = GetEntityHeadPosition(pEntity);
    targetIDPos.z += tagHeightOffset;  // Fixed offset, not scaled by distance
    
    // Calculate top-left position - tag is simply centered
    Vector topLeftPos = targetIDPos - panelRight * (worldWidth * 0.5f) + panelUp * (worldHeight * 0.5f);
    panelToWorld.SetTranslation(topLeftPos);
    
    if (tfvr_world_health_debug.GetBool())
    {
        DevMsg("Tag render: mainPanelW=%d mainWorldW=%.1f\n",
               basePanelWidth, worldWidth);
    }
    
    // Set up wrapper
    m_pTargetIDWrapper->SetSize(renderWidth, renderHeight);
    m_pTargetIDWrapper->SetTargetPanel(pTargetID);
    m_pTargetIDWrapper->SetVisible(true);
    
    g_pMatSystemSurface->DrawPanelIn3DSpace(
        m_pTargetIDWrapper->GetVPanel(),
        panelToWorld,
        renderWidth,
        renderHeight,
        worldWidth,
        worldHeight
    );
    
    m_pTargetIDWrapper->SetVisible(false);
    s_bIsRendering3D = false;
}

#include "cbase.h"
#include "vr_damage_indicator.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "hudelement.h"
#include "hud.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "tier0/vprof.h"
#include "sourcevr/isourcevirtualreality.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Medic caller panel size constants (from tf_hud_mediccallers.cpp)
#define MEDICCALLER_WIDE        (XRES(56))
#define MEDICCALLER_TALL        (YRES(48))

// Global instance
CVRDamageIndicatorManager* g_pVRDamageIndicatorManager = nullptr;

//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_damage_indicator_enabled("tfvr_damage_indicator_enabled", "1", FCVAR_ARCHIVE, 
    "Enable the VR damage direction indicator overlay");
ConVar tfvr_damage_indicator_distance("tfvr_damage_indicator_distance", "80", FCVAR_ARCHIVE, 
    "Distance of damage indicator from head in units");
ConVar tfvr_damage_indicator_scale("tfvr_damage_indicator_scale", "0.25", FCVAR_ARCHIVE, 
    "Scale of the damage indicator panel");
ConVar tfvr_damage_indicator_offset_x("tfvr_damage_indicator_offset_x", "0.0", FCVAR_ARCHIVE, 
    "Horizontal offset (-1 to 1, positive = right)");
ConVar tfvr_damage_indicator_offset_y("tfvr_damage_indicator_offset_y", "0.0", FCVAR_ARCHIVE, 
    "Vertical offset (-1 to 1, positive = up)");

ConVar tfvr_damage_indicator_follow_speed("tfvr_damage_indicator_follow_speed", "6.0", FCVAR_ARCHIVE, 
    "How fast the damage indicator follows head rotation (higher = faster, 0 = instant)");
ConVar tfvr_damage_indicator_deadzone("tfvr_damage_indicator_deadzone", "10.0", FCVAR_ARCHIVE, 
    "Angle deadzone where the damage indicator stays locked to view (degrees)");
ConVar tfvr_damage_indicator_max_lag("tfvr_damage_indicator_max_lag", "30", FCVAR_ARCHIVE, 
    "Maximum angle the damage indicator can lag behind view before clamping");

ConVar tfvr_damage_indicator_width("tfvr_damage_indicator_width", "512", FCVAR_ARCHIVE, 
    "Pixel width of damage indicator panel");
ConVar tfvr_damage_indicator_height("tfvr_damage_indicator_height", "512", FCVAR_ARCHIVE, 
    "Pixel height of damage indicator panel");

ConVar tfvr_damage_indicator_debug("tfvr_damage_indicator_debug", "0", FCVAR_ARCHIVE, 
    "Show debug info for damage indicator positioning");

// Medic caller ConVars
ConVar tfvr_medic_caller_enabled("tfvr_medic_caller_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR rendering of medic caller panels on the damage indicator overlay");
ConVar tfvr_medic_caller_debug("tfvr_medic_caller_debug", "0", FCVAR_ARCHIVE, 
    "Show debug info for medic caller rendering");

//=============================================================================
// CVRDamageIndicatorManager Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRDamageIndicatorManager::CVRDamageIndicatorManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    
    m_pDamageIndicatorPanel = nullptr;
    m_pDamageIndicatorElement = nullptr;
    
    m_flCurrentYaw = 0.0f;
    m_flTargetYaw = 0.0f;
    m_flCurrentPitch = 0.0f;
    m_flTargetPitch = 0.0f;
    
    m_flDistance = 80.0f;
    m_flFollowSpeed = 6.0f;
    m_flDeadzone = 10.0f;
    m_flMaxLagAngle = 30.0f;
    m_flPanelWidth = 20.0f;
    m_flPanelHeight = 20.0f;
    
    m_flOffsetX = 0.0f;
    m_flOffsetY = 0.0f;
    
    m_nPanelPixelWidth = 512;
    m_nPanelPixelHeight = 512;
}

//-----------------------------------------------------------------------------
CVRDamageIndicatorManager::~CVRDamageIndicatorManager()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRDamageIndicatorManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Get the damage indicator HUD element
    // Note: The element name is the class name used in DECLARE_HUDELEMENT
    m_pDamageIndicatorElement = gHUD.FindElement("CHudDamageIndicator");
    if (m_pDamageIndicatorElement)
    {
        m_pDamageIndicatorPanel = dynamic_cast<vgui::Panel*>(m_pDamageIndicatorElement);
    }
    
    if (!m_pDamageIndicatorPanel)
    {
        // Don't spam - element will be acquired during Update()
        DevMsg("VR Damage Indicator: Damage indicator panel not yet available\n");
    }
    
    // Initialize angles to current view
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    m_flCurrentPitch = GetCurrentViewPitch();
    m_flTargetPitch = m_flCurrentPitch;
    
    m_bInitialized = true;
    DevMsg("VR Damage Indicator Manager: Initialized\n");
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::Shutdown()
{
    m_pDamageIndicatorPanel = nullptr;
    m_pDamageIndicatorElement = nullptr;
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::ResetState()
{
    // Reset to current view
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    m_flCurrentPitch = GetCurrentViewPitch();
    m_flTargetPitch = m_flCurrentPitch;
    
    // Re-acquire panel reference
    m_pDamageIndicatorElement = gHUD.FindElement("CHudDamageIndicator");
    if (m_pDamageIndicatorElement)
    {
        m_pDamageIndicatorPanel = dynamic_cast<vgui::Panel*>(m_pDamageIndicatorElement);
    }
}

//-----------------------------------------------------------------------------
float CVRDamageIndicatorManager::GetCurrentViewYaw() const
{
    // Get the forward direction from the actual view vectors (roll-independent)
    Vector forward = MainViewForward();
    
    // Project forward onto horizontal plane and get yaw
    float yaw = RAD2DEG(atan2f(forward.y, forward.x));
    return yaw;
}

//-----------------------------------------------------------------------------
float CVRDamageIndicatorManager::GetCurrentViewPitch() const
{
    // Get the forward direction from the actual view vectors (roll-independent)
    Vector forward = MainViewForward();
    
    // Get pitch from forward direction
    float horizontalDist = sqrtf(forward.x * forward.x + forward.y * forward.y);
    float pitch = RAD2DEG(atan2f(-forward.z, horizontalDist));
    return pitch;
}

//-----------------------------------------------------------------------------
// Updates the HUD angles with lazy follow behavior:
// 1. Deadzone - HUD stays still when view is close
// 2. Smooth follow - HUD catches up with exponential smoothing
// 3. Edge clamp - HUD clamps when view exceeds max lag
//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::UpdateSpringAngles(float deltaTime)
{
    m_flTargetYaw = GetCurrentViewYaw();
    m_flTargetPitch = GetCurrentViewPitch();
    
    // Framerate-independent exponential smoothing factor
    float t = (m_flFollowSpeed <= 0.0f) ? 1.0f : (1.0f - expf(-m_flFollowSpeed * deltaTime));
    
    // Update yaw
    float yawDiff = AngleDiff(m_flTargetYaw, m_flCurrentYaw);
    float absYawDiff = fabsf(yawDiff);
    
    if (absYawDiff > m_flDeadzone)
    {
        if (absYawDiff > m_flMaxLagAngle)
        {
            // Edge clamp
            if (yawDiff > 0)
                m_flCurrentYaw = AngleNormalize(m_flTargetYaw - m_flMaxLagAngle);
            else
                m_flCurrentYaw = AngleNormalize(m_flTargetYaw + m_flMaxLagAngle);
        }
        else
        {
            // Smooth follow
            float effectiveDiff = (yawDiff > 0) ? (yawDiff - m_flDeadzone) : (yawDiff + m_flDeadzone);
            m_flCurrentYaw = AngleNormalize(m_flCurrentYaw + effectiveDiff * t);
        }
    }
    
    // Update pitch with same logic
    float pitchDiff = AngleDiff(m_flTargetPitch, m_flCurrentPitch);
    float absPitchDiff = fabsf(pitchDiff);
    
    if (absPitchDiff > m_flDeadzone)
    {
        if (absPitchDiff > m_flMaxLagAngle)
        {
            // Edge clamp
            if (pitchDiff > 0)
                m_flCurrentPitch = AngleNormalize(m_flTargetPitch - m_flMaxLagAngle);
            else
                m_flCurrentPitch = AngleNormalize(m_flTargetPitch + m_flMaxLagAngle);
        }
        else
        {
            // Smooth follow
            float effectiveDiff = (pitchDiff > 0) ? (pitchDiff - m_flDeadzone) : (pitchDiff + m_flDeadzone);
            m_flCurrentPitch = AngleNormalize(m_flCurrentPitch + effectiveDiff * t);
        }
    }
}

//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;
    
    m_bEnabled = tfvr_damage_indicator_enabled.GetBool();
    
    // Update configuration from ConVars
    m_flDistance = tfvr_damage_indicator_distance.GetFloat();
    m_flFollowSpeed = tfvr_damage_indicator_follow_speed.GetFloat();
    m_flDeadzone = tfvr_damage_indicator_deadzone.GetFloat();
    m_flMaxLagAngle = tfvr_damage_indicator_max_lag.GetFloat();
    m_flOffsetX = tfvr_damage_indicator_offset_x.GetFloat();
    m_flOffsetY = tfvr_damage_indicator_offset_y.GetFloat();
    int sw, sh;
    vgui::surface()->GetScreenSize(sw, sh);
    float sf = (float)sw / 1280.0f;
    m_nPanelPixelWidth = (int)(tfvr_damage_indicator_width.GetInt() * sf);
    m_nPanelPixelHeight = (int)(tfvr_damage_indicator_height.GetInt() * sf);
    
    // Calculate world size from scale
    float scale = tfvr_damage_indicator_scale.GetFloat();
    float aspectRatio = (float)m_nPanelPixelWidth / (float)m_nPanelPixelHeight;
    m_flPanelHeight = m_flDistance * scale;
    m_flPanelWidth = m_flPanelHeight * aspectRatio;
    
    // Update spring physics (both yaw and pitch)
    UpdateSpringAngles(deltaTime);
    
    // Try to re-acquire panel if we don't have it
    if (!m_pDamageIndicatorPanel)
    {
        m_pDamageIndicatorElement = gHUD.FindElement("CHudDamageIndicator");
        if (m_pDamageIndicatorElement)
        {
            m_pDamageIndicatorPanel = dynamic_cast<vgui::Panel*>(m_pDamageIndicatorElement);
        }
    }
}

//-----------------------------------------------------------------------------
bool CVRDamageIndicatorManager::CalculateSpringTransform(VMatrix& transform)
{
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return false;
    
    // Get the player's eye position
    Vector eyePos = MainViewOrigin();
    
    // Build orientation: panelAngles already has roll=0 so AngleVectors
    // gives roll-independent vectors matching the convention DrawPanelIn3DSpace expects.
    QAngle panelAngles(m_flCurrentPitch, m_flCurrentYaw, 0);
    
    Vector forward, right, up;
    AngleVectors(panelAngles, &forward, &right, &up);
    
    // Calculate panel position
    // Start at distance in front, then apply offsets
    Vector panelPos = eyePos + forward * m_flDistance;
    
    // Apply horizontal and vertical offsets
    // Offset is relative to the panel's orientation
    float horizontalOffset = m_flOffsetX * m_flDistance * 0.5f;
    float verticalOffset = m_flOffsetY * m_flDistance * 0.3f;
    panelPos += right * horizontalOffset;
    panelPos += up * verticalOffset;
    
    // Build the transformation matrix
    // Panel should face the player (negative forward direction)
    // The panel's local Z axis should point toward the viewer
    Vector panelForward = -forward;  // Face the player
    Vector panelRight = right;
    Vector panelUp = up;
    
    // Build rotation matrix (column-major for VMatrix)
    transform.Identity();
    transform[0][0] = panelRight.x;  transform[0][1] = panelUp.x;  transform[0][2] = panelForward.x;
    transform[1][0] = panelRight.y;  transform[1][1] = panelUp.y;  transform[1][2] = panelForward.y;
    transform[2][0] = panelRight.z;  transform[2][1] = panelUp.z;  transform[2][2] = panelForward.z;
    transform.SetTranslation(panelPos);
    
    return true;
}

// Priority for damage indicator (head-relative, medium distance)
static const int PRIORITY_DAMAGE_INDICATOR = 160;

//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::Render()
{
    VPROF("VRDamageIndicatorManager_Render");
    
    if (!m_bInitialized || !m_bEnabled)
        return;
    
    if (!m_pDamageIndicatorPanel || !m_pDamageIndicatorElement)
        return;
    
    if (!m_pDamageIndicatorElement->ShouldDraw())
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver())
        return;
    
    VMatrix panelToWorld;
    if (!CalculateSpringTransform(panelToWorld))
        return;
    
    // Queue for distance-sorted rendering
    bool bWasVisible = m_pDamageIndicatorPanel->IsVisible();
    
    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        g_pVRWorldUIQueue->QueuePanel(m_pDamageIndicatorPanel, panelToWorld,
                                      m_nPanelPixelWidth, m_nPanelPixelHeight,
                                      m_flPanelWidth, m_flPanelHeight,
                                      PRIORITY_DAMAGE_INDICATOR, true, bWasVisible);
    }
    else
    {
        // Fallback: render immediately
        m_pDamageIndicatorPanel->SetVisible(true);
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pDamageIndicatorPanel->GetVPanel(),
            panelToWorld,
            m_nPanelPixelWidth,
            m_nPanelPixelHeight,
            m_flPanelWidth,
            m_flPanelHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        m_pDamageIndicatorPanel->SetVisible(bWasVisible);
    }
    
    if (tfvr_damage_indicator_debug.GetBool())
    {
        DevMsg("DamageIndicator: target=%.1f, current=%.1f, diff=%.1f\n",
            m_flTargetYaw, m_flCurrentYaw, 
            AngleDiff(m_flTargetYaw, m_flCurrentYaw));
    }
    
    // Also render medic caller panels on the same overlay
    if (tfvr_medic_caller_enabled.GetBool())
    {
        RenderMedicCallers(panelToWorld);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Render medic caller panels on the damage indicator overlay
//-----------------------------------------------------------------------------
void CVRDamageIndicatorManager::RenderMedicCallers(const VMatrix& panelToWorld)
{
    // Get the viewport which is the parent of all medic caller panels
    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    if (!pViewport)
        return;
    
    // Iterate through viewport children looking for MedicCallerPanel instances
    int nChildren = pViewport->GetChildCount();
    int nMedicCallersFound = 0;
    
    for (int i = 0; i < nChildren; i++)
    {
        vgui::Panel* pChild = pViewport->GetChild(i);
        if (!pChild || !pChild->IsVisible())
            continue;
        
        // Check if this is a MedicCallerPanel by name
        const char* pszName = pChild->GetName();
        if (!pszName || V_strcmp(pszName, "MedicCallerPanel") != 0)
            continue;
        
        nMedicCallersFound++;
        
        // Get the panel size
        int panelWidth, panelHeight;
        pChild->GetSize(panelWidth, panelHeight);
        
        if (panelWidth <= 0 || panelHeight <= 0)
            continue;
        
        // Calculate world size based on the same scale as damage indicator
        float scale = tfvr_damage_indicator_scale.GetFloat();
        float aspectRatio = (float)panelWidth / (float)panelHeight;
        float worldHeight = m_flDistance * scale * 0.5f;  // Slightly smaller than damage indicator
        float worldWidth = worldHeight * aspectRatio;
        
        // Queue for rendering on the same overlay as damage indicator
        if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
        {
            g_pVRWorldUIQueue->QueuePanel(pChild, panelToWorld,
                                          panelWidth, panelHeight,
                                          worldWidth, worldHeight,
                                          PRIORITY_DAMAGE_INDICATOR + 1, true, false);
        }
        else
        {
            // Fallback: render immediately
            pChild->SetVisible(true);
            g_pMatSystemSurface->DisableClipping(true);
            g_pMatSystemSurface->DrawPanelIn3DSpace(
                pChild->GetVPanel(),
                panelToWorld,
                panelWidth,
                panelHeight,
                worldWidth,
                worldHeight
            );
            g_pMatSystemSurface->DisableClipping(false);
            pChild->SetVisible(false);
        }
    }
    
    if (tfvr_medic_caller_debug.GetBool() && nMedicCallersFound > 0)
    {
        DevMsg("MedicCaller: Rendered %d caller panels\n", nMedicCallersFound);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Check if medic caller panels should be suppressed in 2D
//-----------------------------------------------------------------------------
bool CVRDamageIndicatorManager::ShouldSuppressMedicCallerPanel()
{
    if (!g_pVRDamageIndicatorManager)
        return false;
    
    if (!g_pVRDamageIndicatorManager->m_bInitialized)
        return false;
    
    if (!g_pVRDamageIndicatorManager->m_bEnabled)
        return false;
    
    if (!tfvr_medic_caller_enabled.GetBool())
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    return true;
}


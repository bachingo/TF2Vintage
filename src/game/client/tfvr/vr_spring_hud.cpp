#include "cbase.h"
#include "vr_spring_hud.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "hudelement.h"
#include "hud.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CVRSpringHUDManager* g_pVRSpringHUDManager = nullptr;

//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_killfeed_enabled("tfvr_killfeed_enabled", "1", FCVAR_ARCHIVE, 
    "Enable the VR kill feed overlay");
ConVar tfvr_killfeed_distance("tfvr_killfeed_distance", "100", FCVAR_ARCHIVE, 
    "Distance of kill feed from head in units");
ConVar tfvr_killfeed_scale("tfvr_killfeed_scale", "0.15", FCVAR_ARCHIVE, 
    "Scale of the kill feed panel");
ConVar tfvr_killfeed_offset_x("tfvr_killfeed_offset_x", "0.6", FCVAR_ARCHIVE, 
    "Horizontal offset (-1 to 1, positive = right)");
ConVar tfvr_killfeed_offset_y("tfvr_killfeed_offset_y", "0.4", FCVAR_ARCHIVE, 
    "Vertical offset (-1 to 1, positive = up)");

ConVar tfvr_killfeed_follow_speed("tfvr_killfeed_follow_speed", "8.0", FCVAR_ARCHIVE, 
    "How fast the kill feed follows head rotation (higher = faster, 0 = instant)");
ConVar tfvr_killfeed_deadzone("tfvr_killfeed_deadzone", "5.0", FCVAR_ARCHIVE, 
    "Angle deadzone where the kill feed stays locked to view (degrees)");
ConVar tfvr_killfeed_max_lag("tfvr_killfeed_max_lag", "45", FCVAR_ARCHIVE, 
    "Maximum angle the kill feed can lag behind view before clamping");

ConVar tfvr_killfeed_width("tfvr_killfeed_width", "400", FCVAR_ARCHIVE, 
    "Pixel width of kill feed panel");
ConVar tfvr_killfeed_height("tfvr_killfeed_height", "300", FCVAR_ARCHIVE, 
    "Pixel height of kill feed panel");

ConVar tfvr_killfeed_debug("tfvr_killfeed_debug", "0", FCVAR_ARCHIVE, 
    "Show debug info for kill feed positioning");

//=============================================================================
// CVRSpringHUDManager Implementation
//=============================================================================

//-----------------------------------------------------------------------------
CVRSpringHUDManager::CVRSpringHUDManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    
    m_pKillFeedPanel = nullptr;
    m_pKillFeedElement = nullptr;
    
    m_flCurrentYaw = 0.0f;
    m_flTargetYaw = 0.0f;
    
    m_flDistance = 100.0f;
    m_flFollowSpeed = 8.0f;
    m_flDeadzone = 5.0f;
    m_flMaxLagAngle = 45.0f;
    m_flPanelWidth = 15.0f;
    m_flPanelHeight = 12.0f;
    
    m_flOffsetX = 0.6f;
    m_flOffsetY = 0.4f;
    
    m_nPanelPixelWidth = 400;
    m_nPanelPixelHeight = 300;
}

//-----------------------------------------------------------------------------
CVRSpringHUDManager::~CVRSpringHUDManager()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
bool CVRSpringHUDManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Get the kill feed HUD element
    m_pKillFeedElement = gHUD.FindElement("CTFHudDeathNotice");
    if (m_pKillFeedElement)
    {
        m_pKillFeedPanel = dynamic_cast<vgui::Panel*>(m_pKillFeedElement);
    }
    
    if (!m_pKillFeedPanel)
    {
        Warning("VR Spring HUD: Could not find kill feed panel\n");
        // Don't fail - the panel might become available later
    }
    
    // Initialize yaw to current view
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    
    m_bInitialized = true;
    DevMsg("VR Spring HUD Manager: Initialized\n");
    
    return true;
}

//-----------------------------------------------------------------------------
void CVRSpringHUDManager::Shutdown()
{
    m_pKillFeedPanel = nullptr;
    m_pKillFeedElement = nullptr;
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
void CVRSpringHUDManager::ResetState()
{
    // Reset to current view
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    
    // Re-acquire panel reference
    m_pKillFeedElement = gHUD.FindElement("CTFHudDeathNotice");
    if (m_pKillFeedElement)
    {
        m_pKillFeedPanel = dynamic_cast<vgui::Panel*>(m_pKillFeedElement);
    }
}

//-----------------------------------------------------------------------------
float CVRSpringHUDManager::GetCurrentViewYaw() const
{
    // Get the current view angles
    QAngle viewAngles;
    engine->GetViewAngles(viewAngles);
    return viewAngles[YAW];
}

//-----------------------------------------------------------------------------
// Updates the HUD yaw with lazy follow behavior:
// 1. Deadzone - HUD stays still when view is close
// 2. Smooth follow - HUD catches up with exponential smoothing
// 3. Edge clamp - HUD clamps when view exceeds max lag
//-----------------------------------------------------------------------------
void CVRSpringHUDManager::UpdateSpringYaw(float deltaTime)
{
    m_flTargetYaw = GetCurrentViewYaw();
    
    float diff = AngleDiff(m_flTargetYaw, m_flCurrentYaw);
    float absDiff = fabsf(diff);
    
    // Deadzone: HUD stays still when view is within deadzone
    if (absDiff <= m_flDeadzone)
        return;
    
    // Edge clamp: snap to max lag distance when turning too fast
    if (absDiff > m_flMaxLagAngle)
    {
        if (diff > 0)
            m_flCurrentYaw = AngleNormalize(m_flTargetYaw - m_flMaxLagAngle);
        else
            m_flCurrentYaw = AngleNormalize(m_flTargetYaw + m_flMaxLagAngle);
        return;
    }
    
    // Smooth follow: interpolate toward target, respecting deadzone boundary
    float effectiveDiff = (diff > 0) ? (diff - m_flDeadzone) : (diff + m_flDeadzone);
    
    if (m_flFollowSpeed <= 0.0f)
    {
        m_flCurrentYaw = AngleNormalize(m_flCurrentYaw + effectiveDiff);
        return;
    }
    
    // Framerate-independent exponential smoothing
    float t = 1.0f - expf(-m_flFollowSpeed * deltaTime);
    m_flCurrentYaw = AngleNormalize(m_flCurrentYaw + effectiveDiff * t);
}

//-----------------------------------------------------------------------------
void CVRSpringHUDManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;
    
    m_bEnabled = tfvr_killfeed_enabled.GetBool();
    
    // Update configuration from ConVars
    m_flDistance = tfvr_killfeed_distance.GetFloat();
    m_flFollowSpeed = tfvr_killfeed_follow_speed.GetFloat();
    m_flDeadzone = tfvr_killfeed_deadzone.GetFloat();
    m_flMaxLagAngle = tfvr_killfeed_max_lag.GetFloat();
    m_flOffsetX = tfvr_killfeed_offset_x.GetFloat();
    m_flOffsetY = tfvr_killfeed_offset_y.GetFloat();
    int w, h;
    vgui::surface()->GetScreenSize(w, h);
    float sf = (float)w / 1280.0f;
    m_nPanelPixelWidth = (int)(tfvr_killfeed_width.GetInt() * sf);
    m_nPanelPixelHeight = (int)(tfvr_killfeed_height.GetInt() * sf);
    
    // Calculate world size from scale
    float scale = tfvr_killfeed_scale.GetFloat();
    float aspectRatio = (float)m_nPanelPixelWidth / (float)m_nPanelPixelHeight;
    m_flPanelHeight = m_flDistance * scale;
    m_flPanelWidth = m_flPanelHeight * aspectRatio;
    
    // Update spring physics
    UpdateSpringYaw(deltaTime);
    
    // Try to re-acquire panel if we don't have it
    if (!m_pKillFeedPanel)
    {
        m_pKillFeedElement = gHUD.FindElement("CTFHudDeathNotice");
        if (m_pKillFeedElement)
        {
            m_pKillFeedPanel = dynamic_cast<vgui::Panel*>(m_pKillFeedElement);
        }
    }
}

//-----------------------------------------------------------------------------
bool CVRSpringHUDManager::CalculateSpringTransform(VMatrix& transform)
{
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return false;
    
    // Get the player's eye position
    Vector eyePos = MainViewOrigin();
    
    // Build orientation: use spring yaw, but fixed pitch/roll
    QAngle panelAngles(0, m_flCurrentYaw, 0);
    
    // Get forward/right/up vectors for the spring arm
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

// Priority for spring HUD (kill feed - medium distance from player)
static const int PRIORITY_SPRING_HUD = 150;

//-----------------------------------------------------------------------------
void CVRSpringHUDManager::Render()
{
    VPROF("VRSpringHUDManager_Render");
    
    if (!m_bInitialized || !m_bEnabled)
        return;
    
    if (!m_pKillFeedPanel || !m_pKillFeedElement)
        return;
    
    // Check if the kill feed should draw
    if (!m_pKillFeedElement->ShouldDraw())
        return;
    
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver())
        return;
    
    // Don't render if game UI is open
    if (enginevgui && enginevgui->IsGameUIVisible())
        return;
    
    // Don't render if class menu is open
    ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
    if (pClassMenuOpen && pClassMenuOpen->GetBool())
        return;
    
    VMatrix panelToWorld;
    if (!CalculateSpringTransform(panelToWorld))
        return;
    
    // Queue for distance-sorted rendering
    bool bWasVisible = m_pKillFeedPanel->IsVisible();
    
    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        g_pVRWorldUIQueue->QueuePanel(m_pKillFeedPanel, panelToWorld,
                                      m_nPanelPixelWidth, m_nPanelPixelHeight,
                                      m_flPanelWidth, m_flPanelHeight,
                                      PRIORITY_SPRING_HUD, true, bWasVisible);
    }
    else
    {
        // Fallback: render immediately
        m_pKillFeedPanel->SetVisible(true);
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pKillFeedPanel->GetVPanel(),
            panelToWorld,
            m_nPanelPixelWidth,
            m_nPanelPixelHeight,
            m_flPanelWidth,
            m_flPanelHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        m_pKillFeedPanel->SetVisible(bWasVisible);
    }
    
    if (tfvr_killfeed_debug.GetBool())
    {
        DevMsg("KillFeed: target=%.1f, current=%.1f, diff=%.1f\n",
            m_flTargetYaw, m_flCurrentYaw, 
            AngleDiff(m_flTargetYaw, m_flCurrentYaw));
    }
}

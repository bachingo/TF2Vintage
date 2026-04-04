//=============================================================================
// TF2VR - VR Popup HUD Manager
// Renders win/loss panels and scoreboard in VR with spring-follow positioning
//=============================================================================

#include "cbase.h"
#include "vr_popup_hud.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "hudelement.h"
#include "hud.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "voice_status.h"
#include "vgui/IVGui.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tier0/vprof.h"
#include "client_virtualreality.h"
#include "openxr_manager.h"
#include "engine/ivdebugoverlay.h"
#include "sourcevr/isourcevirtualreality.h"
#include "tf_hud_target_id.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CVRPopupHUDManager* g_pVRPopupHUDManager = nullptr;


//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_popup_hud_enabled("tfvr_popup_hud_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR popup HUD for win/loss screens and scoreboard");
ConVar tfvr_popup_hud_distance("tfvr_popup_hud_distance", "120", FCVAR_ARCHIVE, 
    "Distance of popup HUD from head in units");
ConVar tfvr_popup_hud_scale("tfvr_popup_hud_scale", "80", FCVAR_ARCHIVE, 
    "Scale of the popup HUD panel in world units");
ConVar tfvr_popup_hud_vertical_offset("tfvr_popup_hud_vertical_offset", "0", FCVAR_ARCHIVE, 
    "Vertical offset of popup HUD (positive = up)");

ConVar tfvr_popup_hud_follow_speed("tfvr_popup_hud_follow_speed", "4.0", FCVAR_ARCHIVE, 
    "How fast the popup HUD follows head rotation (higher = faster)");
ConVar tfvr_popup_hud_deadzone("tfvr_popup_hud_deadzone", "15.0", FCVAR_ARCHIVE, 
    "Angle deadzone where popup HUD stays locked to view (degrees)");
ConVar tfvr_popup_hud_max_lag("tfvr_popup_hud_max_lag", "45", FCVAR_ARCHIVE, 
    "Maximum angle the popup HUD can lag behind view before clamping");

ConVar tfvr_popup_hud_debug("tfvr_popup_hud_debug", "0", FCVAR_ARCHIVE, 
    "Show debug info for popup HUD");

ConVar tfvr_popup_hud_offset_x("tfvr_popup_hud_offset_x", "0", FCVAR_ARCHIVE, 
    "Horizontal offset adjustment for popup HUD (positive = right)");
ConVar tfvr_popup_hud_offset_y("tfvr_popup_hud_offset_y", "0", FCVAR_ARCHIVE, 
    "Vertical offset adjustment for popup HUD (positive = up)");

// Per-panel offset adjustments (added to global offset)
ConVar tfvr_popup_hud_scoreboard_offset_x("tfvr_popup_hud_scoreboard_offset_x", "0", FCVAR_ARCHIVE, 
    "Additional horizontal offset for scoreboard (positive = right)");
ConVar tfvr_popup_hud_scoreboard_offset_y("tfvr_popup_hud_scoreboard_offset_y", "0", FCVAR_ARCHIVE, 
    "Additional vertical offset for scoreboard (positive = up)");
ConVar tfvr_popup_hud_winpanel_offset_x("tfvr_popup_hud_winpanel_offset_x", "0", FCVAR_ARCHIVE, 
    "Additional horizontal offset for win panel (positive = right)");
ConVar tfvr_popup_hud_winpanel_offset_y("tfvr_popup_hud_winpanel_offset_y", "0", FCVAR_ARCHIVE, 
    "Additional vertical offset for win panel (positive = up)");
ConVar tfvr_popup_hud_matchstatus_offset_x("tfvr_popup_hud_matchstatus_offset_x", "0", FCVAR_ARCHIVE, 
    "Additional horizontal offset for match status (positive = right)");
ConVar tfvr_popup_hud_matchstatus_offset_y("tfvr_popup_hud_matchstatus_offset_y", "-10", FCVAR_ARCHIVE, 
    "Additional vertical offset for match status (positive = up)");
ConVar tfvr_popup_hud_matchstatus_scale("tfvr_popup_hud_matchstatus_scale", "0.25", FCVAR_ARCHIVE, 
    "Scale multiplier for match status panel (smaller = shows more of the panel)");
ConVar tfvr_popup_hud_matchstatus_crop_bottom("tfvr_popup_hud_matchstatus_crop_bottom", "0.75", FCVAR_ARCHIVE, 
    "Fraction of the bottom to crop off match status (0.75 = show top 25%)");
ConVar tfvr_popup_hud_matchstatus_content_y("tfvr_popup_hud_matchstatus_content_y", "0", FCVAR_ARCHIVE, 
    "Y content offset for match status (positive = shift content down in capture area)");

// Bottom-center notification area ConVars
ConVar tfvr_popup_hud_notifications_enabled("tfvr_popup_hud_notifications_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR rendering of bottom-center notifications (notification panel, spectator target, building status)");
ConVar tfvr_popup_hud_notifications_offset_x("tfvr_popup_hud_notifications_offset_x", "0", FCVAR_ARCHIVE, 
    "Horizontal offset of notification area (positive = right)");
ConVar tfvr_popup_hud_notifications_offset_y("tfvr_popup_hud_notifications_offset_y", "-15", FCVAR_ARCHIVE, 
    "Vertical offset of notification area below timer (negative = down)");
ConVar tfvr_popup_hud_notifications_offset_z("tfvr_popup_hud_notifications_offset_z", "0", FCVAR_ARCHIVE, 
    "Depth offset of notification area (positive = closer)");
ConVar tfvr_popup_hud_notifications_scale("tfvr_popup_hud_notifications_scale", "0.5", FCVAR_ARCHIVE, 
    "Scale multiplier for notification panels (relative to main popup scale)");
ConVar tfvr_popup_hud_notifications_spacing("tfvr_popup_hud_notifications_spacing", "10", FCVAR_ARCHIVE, 
    "Vertical spacing between notification panel slots");
ConVar tfvr_popup_hud_notifications_debug("tfvr_popup_hud_notifications_debug", "0", FCVAR_ARCHIVE,
    "Debug output for notification panel rendering");

// Healer panel specific offsets
ConVar tfvr_popup_hud_healer_offset_x("tfvr_popup_hud_healer_offset_x", "0", FCVAR_ARCHIVE, 
    "Horizontal offset for healer notification panel (positive = right)");
ConVar tfvr_popup_hud_healer_offset_y("tfvr_popup_hud_healer_offset_y", "0", FCVAR_ARCHIVE, 
    "Vertical offset for healer notification panel (positive = up)");

// Building status specific offsets
ConVar tfvr_popup_hud_building_offset_x("tfvr_popup_hud_building_offset_x", "0", FCVAR_ARCHIVE, 
    "Horizontal offset for building status panel (positive = right)");
ConVar tfvr_popup_hud_building_offset_y("tfvr_popup_hud_building_offset_y", "0", FCVAR_ARCHIVE, 
    "Vertical offset for building status panel (positive = up)");

// Voice status UI ConVars
ConVar tfvr_popup_hud_voice_enabled("tfvr_popup_hud_voice_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR rendering of voice status panels (self-icon and other players speaking)");
ConVar tfvr_popup_hud_voice_distance("tfvr_popup_hud_voice_distance", "100", FCVAR_ARCHIVE,
    "Distance of voice UI from head (independent of popup panels)");
ConVar tfvr_popup_hud_voice_scale("tfvr_popup_hud_voice_scale", "0.4", FCVAR_ARCHIVE, 
    "Scale multiplier for voice status panels");

// Self-status (local player speaking icon) positioning
ConVar tfvr_popup_hud_voice_self_offset_x("tfvr_popup_hud_voice_self_offset_x", "35", FCVAR_ARCHIVE, 
    "Horizontal offset for self speaking icon (positive = right)");
ConVar tfvr_popup_hud_voice_self_offset_y("tfvr_popup_hud_voice_self_offset_y", "20", FCVAR_ARCHIVE, 
    "Vertical offset for self speaking icon (positive = up)");

// Other players speaking list positioning
ConVar tfvr_popup_hud_voice_others_offset_x("tfvr_popup_hud_voice_others_offset_x", "-35", FCVAR_ARCHIVE, 
    "Horizontal offset for other players speaking list (positive = right)");
ConVar tfvr_popup_hud_voice_others_offset_y("tfvr_popup_hud_voice_others_offset_y", "15", FCVAR_ARCHIVE, 
    "Vertical offset for other players speaking list (positive = up)");

ConVar tfvr_popup_hud_voice_debug("tfvr_popup_hud_voice_debug", "0", FCVAR_ARCHIVE, 
    "Debug output for voice status VR rendering");
ConVar tfvr_popup_hud_voice_test("tfvr_popup_hud_voice_test", "0", FCVAR_CHEAT, 
    "Force voice UI to render for testing (bypasses voice manager check)");

// Chat panel ConVars
ConVar tfvr_popup_hud_chat_enabled("tfvr_popup_hud_chat_enabled", "1", FCVAR_ARCHIVE, 
    "Enable VR rendering of the text chat window on the popup HUD");
ConVar tfvr_popup_hud_chat_offset_x("tfvr_popup_hud_chat_offset_x", "-25", FCVAR_ARCHIVE, 
    "Horizontal offset for chat panel (positive = right)");
ConVar tfvr_popup_hud_chat_offset_y("tfvr_popup_hud_chat_offset_y", "-20", FCVAR_ARCHIVE, 
    "Vertical offset for chat panel (positive = up)");
ConVar tfvr_popup_hud_chat_scale("tfvr_popup_hud_chat_scale", "0.5", FCVAR_ARCHIVE, 
    "Scale multiplier for chat panel (relative to main popup scale)");
ConVar tfvr_popup_hud_chat_debug("tfvr_popup_hud_chat_debug", "0", FCVAR_ARCHIVE, 
    "Debug output for chat panel VR rendering");

//=============================================================================
// CVRPanelWrapper Implementation
//=============================================================================

DECLARE_BUILD_FACTORY(CVRPanelWrapper);

CVRPanelWrapper::CVRPanelWrapper(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    m_pTargetPanel = nullptr;
    m_nContentOffsetX = 0;
    m_nContentOffsetY = 0;
    SetPaintBackgroundEnabled(false);
}

// Static flag definition
bool CVRPanelWrapper::s_bInsideWrapperPaint = false;

void CVRPanelWrapper::Paint()
{
    if (!m_pTargetPanel)
        return;
    
    // Set bypass flag so suppression functions know we're actively rendering
    s_bInsideWrapperPaint = true;
    
    // Get the target panel's screen position
    int panelScreenX = 0, panelScreenY = 0;
    m_pTargetPanel->LocalToScreen(panelScreenX, panelScreenY);
    
    // Calculate offset to move panel to content offset position in our render target
    int targetX = m_nContentOffsetX;
    int targetY = m_nContentOffsetY;
    int offsetX = targetX - panelScreenX;
    int offsetY = targetY - panelScreenY;
    
    // Temporarily make the target panel visible
    bool bWasVisible = m_pTargetPanel->IsVisible();
    m_pTargetPanel->SetVisible(true);
    
    // Disable clipping completely
    g_pMatSystemSurface->DisableClipping(true);
    
    // Also set a very large clip rect to override any internal clipping
    g_pMatSystemSurface->SetClippingRect(-4096, -4096, 4096, 4096);
    
    // Apply the offset and paint the target panel
    vgui::surface()->ForceScreenPosOffset(true, offsetX, offsetY);
    vgui::surface()->PaintTraverse(m_pTargetPanel->GetVPanel());
    vgui::surface()->ForceScreenPosOffset(false, 0, 0);
    
    // Restore clipping
    g_pMatSystemSurface->DisableClipping(false);
    
    // Restore visibility
    m_pTargetPanel->SetVisible(bWasVisible);
    
    // Clear bypass flag
    s_bInsideWrapperPaint = false;
}

//=============================================================================
// CVRPopupHUDManager Implementation
//=============================================================================

CVRPopupHUDManager::CVRPopupHUDManager()
{
    m_bInitialized = false;
    m_bEnabled = true;
    
    m_pScoreboardPanel = nullptr;
    m_pWinPanel = nullptr;
    m_pArenaWinPanel = nullptr;
    m_pMatchSummaryPanel = nullptr;
    m_pMatchStatusWrapper = nullptr;
    m_pHealerWrapper = nullptr;
    m_pActivePanel = nullptr;
    
    // Bottom-center notification panels
    m_pNotificationPanel = nullptr;
    m_pMainTargetID = nullptr;
    m_pSpectatorTargetID = nullptr;
    m_pSecondaryTargetID = nullptr;
    m_pSecondaryTargetIDElement = nullptr;
    m_pBuildingStatusEngineer = nullptr;
    m_pBuildingStatusSpy = nullptr;
    
    // Voice status panels
    m_pVoiceSelfStatus = nullptr;
    m_pVoiceStatus = nullptr;
    
    // Chat panel
    m_pChatPanel = nullptr;
    m_pChatElement = nullptr;
    
    m_flCurrentYaw = 0.0f;
    m_flTargetYaw = 0.0f;
    
    m_flDistance = 120.0f;
    m_flFollowSpeed = 4.0f;
    m_flDeadzone = 15.0f;
    m_flMaxLagAngle = 45.0f;
    m_flScale = 80.0f;
    m_flVerticalOffset = 0.0f;
    
    // Notification area configuration
    m_bNotificationsEnabled = true;
    m_flNotificationsOffsetY = -15.0f;
    m_flNotificationsScale = 0.5f;
    
    // Voice status UI configuration
    m_bVoiceStatusEnabled = true;
    m_flVoiceDistance = 100.0f;
    m_flVoiceScale = 0.4f;
    m_flVoiceSelfOffsetX = 35.0f;
    m_flVoiceSelfOffsetY = 20.0f;
    m_flVoiceOthersOffsetX = -35.0f;
    m_flVoiceOthersOffsetY = 15.0f;
    
    // Chat panel configuration
    m_bChatEnabled = true;
    m_flChatOffsetX = -25.0f;
    m_flChatOffsetY = -20.0f;
    m_flChatScale = 0.5f;
}

CVRPopupHUDManager::~CVRPopupHUDManager()
{
    Shutdown();
}

bool CVRPopupHUDManager::Initialize()
{
    if (m_bInitialized)
        return true;
    
    // Try to acquire panels (may not be available yet)
    AcquirePanels();
    
    // Create wrapper panel for match status (hidden, used only for rendering)
    if (!m_pMatchStatusWrapper)
    {
        // Parent to the viewport so it gets proper context
        vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
        m_pMatchStatusWrapper = new CVRPanelWrapper(pViewport, "VRMatchStatusWrapper");
        m_pMatchStatusWrapper->SetVisible(false);
        uint32_t specW, specH;
        g_pOpenXRManager->GetSpectatorScreenDims(specW, specH);
        m_pMatchStatusWrapper->SetSize(specW, specH);
    }
    
    // Create wrapper panel for healer notification (to center content properly)
    if (!m_pHealerWrapper)
    {
        vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
        m_pHealerWrapper = new CVRPanelWrapper(pViewport, "VRHealerWrapper");
        m_pHealerWrapper->SetVisible(false);
        m_pHealerWrapper->SetSize(512, 128);  // Reasonable size for healer panel
    }
    
    // Initialize yaw to current view
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    
    m_bInitialized = true;
    DevMsg("VR Popup HUD Manager: Initialized\n");
    
    return true;
}

void CVRPopupHUDManager::Shutdown()
{
    if (m_pMatchStatusWrapper)
    {
        m_pMatchStatusWrapper->MarkForDeletion();
        m_pMatchStatusWrapper = nullptr;
    }
    
    if (m_pHealerWrapper)
    {
        m_pHealerWrapper->MarkForDeletion();
        m_pHealerWrapper = nullptr;
    }
    
    m_pScoreboardPanel = nullptr;
    m_pWinPanel = nullptr;
    m_pArenaWinPanel = nullptr;
    m_pMatchSummaryPanel = nullptr;
    m_pActivePanel = nullptr;
    
    // Clear notification panel pointers
    m_pNotificationPanel = nullptr;
    m_pMainTargetID = nullptr;
    m_pSpectatorTargetID = nullptr;
    m_pSecondaryTargetID = nullptr;
    m_pSecondaryTargetIDElement = nullptr;
    m_pBuildingStatusEngineer = nullptr;
    m_pBuildingStatusSpy = nullptr;
    
    // Clear voice panel pointers
    m_pVoiceSelfStatus = nullptr;
    m_pVoiceStatus = nullptr;
    
    // Clear chat panel pointer
    m_pChatPanel = nullptr;
    m_pChatElement = nullptr;
    
    m_bInitialized = false;
}

void CVRPopupHUDManager::AcquirePanels()
{
    static bool bFirstAcquire = true;
    
    // Get the viewport panel first
    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    
    // Try to find scoreboard - it's a child of the viewport named "scores" or similar
    if (!m_pScoreboardPanel && pViewport)
    {
        // Try finding by the registered panel name first
        m_pScoreboardPanel = pViewport->FindChildByName("scores", true);
        
        // If not found, try alternate names
        if (!m_pScoreboardPanel)
        {
            m_pScoreboardPanel = pViewport->FindChildByName("scoreboard", true);
        }
        if (!m_pScoreboardPanel)
        {
            m_pScoreboardPanel = pViewport->FindChildByName("CTFClientScoreBoardDialog", true);
        }
        
        if (m_pScoreboardPanel)
        {
            DevMsg("VR Popup HUD: Found scoreboard panel '%s'\n", m_pScoreboardPanel->GetName());
        }
    }
    
    // Try to find win panel via HUD element (CTFWinPanel)
    if (!m_pWinPanel)
    {
        CHudElement* pElement = gHUD.FindElement("CTFWinPanel");
        if (pElement)
        {
            m_pWinPanel = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pWinPanel)
            {
                DevMsg("VR Popup HUD: Found win panel '%s'\n", m_pWinPanel->GetName());
            }
        }
    }
    
    // Arena win panel is a child of the viewport
    if (!m_pArenaWinPanel && pViewport)
    {
        m_pArenaWinPanel = pViewport->FindChildByName("ArenaWinPanel", true);
        if (m_pArenaWinPanel)
        {
            DevMsg("VR Popup HUD: Found arena win panel\n");
        }
    }
    
    // Match summary panel (CTFHudMatchStatus) - this is a HUD element
    if (!m_pMatchSummaryPanel)
    {
        CHudElement* pElement = gHUD.FindElement("CTFHudMatchStatus");
        if (pElement)
        {
            m_pMatchSummaryPanel = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pMatchSummaryPanel)
            {
                DevMsg("VR Popup HUD: Found match summary panel\n");
            }
        }
    }
    
    // Acquire bottom-center notification panels
    
    // CHudNotificationPanel - game notifications like "Intelligence captured"
    if (!m_pNotificationPanel)
    {
        CHudElement* pElement = gHUD.FindElement("CHudNotificationPanel");
        if (pElement)
        {
            m_pNotificationPanel = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pNotificationPanel)
            {
                DevMsg("VR Popup HUD: Found notification panel\n");
            }
        }
    }
    
    // CMainTargetID - target ID when pointing at players/buildings while alive
    if (!m_pMainTargetID)
    {
        CHudElement* pElement = gHUD.FindElement("CMainTargetID");
        if (pElement)
        {
            m_pMainTargetID = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pMainTargetID)
            {
                DevMsg("VR Popup HUD: Found main target ID panel\n");
            }
        }
    }
    
    // CSpectatorTargetID - spectator target with team-color backdrop
    if (!m_pSpectatorTargetID)
    {
        CHudElement* pElement = gHUD.FindElement("CSpectatorTargetID");
        if (pElement)
        {
            m_pSpectatorTargetID = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pSpectatorTargetID)
            {
                DevMsg("VR Popup HUD: Found spectator target ID panel\n");
            }
        }
    }
    
    // CSecondaryTargetID - healer notification ("Healer: [name]" with UberCharge)
    if (!m_pSecondaryTargetID)
    {
        CHudElement* pElement = gHUD.FindElement("CSecondaryTargetID");
        if (pElement)
        {
            m_pSecondaryTargetID = dynamic_cast<vgui::Panel*>(pElement);
            m_pSecondaryTargetIDElement = pElement;  // Store CHudElement for ShouldDraw() check
            if (m_pSecondaryTargetID)
            {
                DevMsg("VR Popup HUD: Found secondary target ID (healer) panel\n");
            }
        }
    }
    
    // CHudBuildingStatusContainer_Engineer - engineer building status
    if (!m_pBuildingStatusEngineer)
    {
        CHudElement* pElement = gHUD.FindElement("BuildingStatus_Engineer");
        if (pElement)
        {
            m_pBuildingStatusEngineer = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pBuildingStatusEngineer)
            {
                DevMsg("VR Popup HUD: Found engineer building status panel\n");
            }
        }
    }
    
    // CHudBuildingStatusContainer_Spy - spy sapper status
    if (!m_pBuildingStatusSpy)
    {
        CHudElement* pElement = gHUD.FindElement("BuildingStatus_Spy");
        if (pElement)
        {
            m_pBuildingStatusSpy = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pBuildingStatusSpy)
            {
                DevMsg("VR Popup HUD: Found spy building status panel\n");
            }
        }
    }
    
    // CHudVoiceSelfStatus - local player speaking icon
    if (!m_pVoiceSelfStatus)
    {
        CHudElement* pElement = gHUD.FindElement("CHudVoiceSelfStatus");
        if (pElement)
        {
            m_pVoiceSelfStatus = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pVoiceSelfStatus)
            {
                DevMsg("VR Popup HUD: Found voice self status panel\n");
            }
        }
    }
    
    // CHudVoiceStatus - other players speaking list
    if (!m_pVoiceStatus)
    {
        CHudElement* pElement = gHUD.FindElement("CHudVoiceStatus");
        if (pElement)
        {
            m_pVoiceStatus = dynamic_cast<vgui::Panel*>(pElement);
            if (m_pVoiceStatus)
            {
                DevMsg("VR Popup HUD: Found voice status panel\n");
            }
        }
    }
    
    // CHudChat - text chat window
    if (!m_pChatPanel)
    {
        CHudElement* pElement = gHUD.FindElement("CHudChat");
        if (pElement)
        {
            m_pChatPanel = dynamic_cast<vgui::Panel*>(pElement);
            m_pChatElement = pElement;
            if (m_pChatPanel)
            {
                DevMsg("VR Popup HUD: Found chat panel\n");
            }
        }
    }
    
    if (bFirstAcquire && (m_pScoreboardPanel || m_pWinPanel))
    {
        DevMsg("VR Popup HUD: Acquired panels - Scoreboard=%p, WinPanel=%p, ArenaWin=%p, MatchSummary=%p\n",
            m_pScoreboardPanel, m_pWinPanel, m_pArenaWinPanel, m_pMatchSummaryPanel);
        DevMsg("VR Popup HUD: Notification panels - Notification=%p, MainTarget=%p, SpectatorTarget=%p, SecondaryTarget=%p, BuildingEngy=%p, BuildingSpy=%p\n",
            m_pNotificationPanel, m_pMainTargetID, m_pSpectatorTargetID, m_pSecondaryTargetID, m_pBuildingStatusEngineer, m_pBuildingStatusSpy);
        DevMsg("VR Popup HUD: Voice panels - VoiceSelf=%p, VoiceStatus=%p\n",
            m_pVoiceSelfStatus, m_pVoiceStatus);
        DevMsg("VR Popup HUD: Chat panel - Chat=%p\n", m_pChatPanel);
        bFirstAcquire = false;
    }
    
    if (tfvr_popup_hud_debug.GetBool())
    {
        static float lastDebugTime = 0;
        if (gpGlobals->curtime - lastDebugTime > 5.0f)
        {
            DevMsg("VR Popup HUD Panels: Scoreboard=%p (vis=%d), WinPanel=%p (vis=%d)\n",
                m_pScoreboardPanel, m_pScoreboardPanel ? m_pScoreboardPanel->IsVisible() : 0,
                m_pWinPanel, m_pWinPanel ? m_pWinPanel->IsVisible() : 0);
            lastDebugTime = gpGlobals->curtime;
        }
    }
}

vgui::Panel* CVRPopupHUDManager::DetermineActivePanel()
{
    // Priority order: Scoreboard > Win panels > Match summary
    // This matches vanilla behavior where scoreboard overrides win panel
    
    // Check scoreboard first (highest priority - TAB overrides everything)
    if (m_pScoreboardPanel && m_pScoreboardPanel->IsVisible())
    {
        return m_pScoreboardPanel;
    }
    
    // Check win panel
    if (m_pWinPanel && m_pWinPanel->IsVisible())
    {
        return m_pWinPanel;
    }
    
    // Check arena win panel
    if (m_pArenaWinPanel && m_pArenaWinPanel->IsVisible())
    {
        return m_pArenaWinPanel;
    }
    
    // Check match summary
    if (m_pMatchSummaryPanel && m_pMatchSummaryPanel->IsVisible())
    {
        // Only use match summary if it's actually showing the summary (not just existing)
        // The match summary panel has specific visibility states
        return m_pMatchSummaryPanel;
    }
    
    return nullptr;
}

float CVRPopupHUDManager::GetCurrentViewYaw() const
{
    // Get the player's current view yaw from the VR headset
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix mideyePose = g_pOpenXRManager->GetMideyePose();
        QAngle angles;
        MatrixAngles(mideyePose.As3x4(), angles);
        
        // Add world rotation from VR system
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        QAngle worldAngles;
        MatrixAngles(worldFromMideye.As3x4(), worldAngles);
        
        return worldAngles[YAW];
    }
    
    // Fallback to engine view
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        return pPlayer->EyeAngles()[YAW];
    }
    
    return 0.0f;
}

void CVRPopupHUDManager::UpdateSpringYaw(float deltaTime)
{
    m_flTargetYaw = GetCurrentViewYaw();
    
    // Calculate angular difference (handle wrap-around)
    float diff = AngleDiff(m_flTargetYaw, m_flCurrentYaw);
    
    // Apply deadzone - if within deadzone, don't move
    if (fabsf(diff) <= m_flDeadzone)
    {
        // Stay put
        return;
    }
    
    // Outside deadzone - move toward target
    float adjustedDiff = diff;
    if (diff > 0)
        adjustedDiff = diff - m_flDeadzone;
    else
        adjustedDiff = diff + m_flDeadzone;
    
    // Smooth follow
    float moveAmount = adjustedDiff * m_flFollowSpeed * deltaTime;
    
    // Clamp to max lag angle
    float newYaw = m_flCurrentYaw + moveAmount;
    float newDiff = AngleDiff(m_flTargetYaw, newYaw);
    
    if (fabsf(newDiff) > m_flMaxLagAngle)
    {
        // Clamp to max lag
        if (newDiff > 0)
            newYaw = m_flTargetYaw - m_flMaxLagAngle;
        else
            newYaw = m_flTargetYaw + m_flMaxLagAngle;
    }
    
    m_flCurrentYaw = AngleNormalize(newYaw);
}

void CVRPopupHUDManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;
    
    // Update settings from ConVars
    m_bEnabled = tfvr_popup_hud_enabled.GetBool();
    m_flDistance = tfvr_popup_hud_distance.GetFloat();
    m_flScale = tfvr_popup_hud_scale.GetFloat();
    m_flVerticalOffset = tfvr_popup_hud_vertical_offset.GetFloat();
    m_flFollowSpeed = tfvr_popup_hud_follow_speed.GetFloat();
    m_flDeadzone = tfvr_popup_hud_deadzone.GetFloat();
    m_flMaxLagAngle = tfvr_popup_hud_max_lag.GetFloat();
    
    // Notification area settings
    m_bNotificationsEnabled = tfvr_popup_hud_notifications_enabled.GetBool();
    m_flNotificationsOffsetY = tfvr_popup_hud_notifications_offset_y.GetFloat();
    m_flNotificationsScale = tfvr_popup_hud_notifications_scale.GetFloat();
    
    // Voice status UI settings
    m_bVoiceStatusEnabled = tfvr_popup_hud_voice_enabled.GetBool();
    m_flVoiceDistance = tfvr_popup_hud_voice_distance.GetFloat();
    m_flVoiceScale = tfvr_popup_hud_voice_scale.GetFloat();
    m_flVoiceSelfOffsetX = tfvr_popup_hud_voice_self_offset_x.GetFloat();
    m_flVoiceSelfOffsetY = tfvr_popup_hud_voice_self_offset_y.GetFloat();
    m_flVoiceOthersOffsetX = tfvr_popup_hud_voice_others_offset_x.GetFloat();
    m_flVoiceOthersOffsetY = tfvr_popup_hud_voice_others_offset_y.GetFloat();
    
    // Chat panel settings
    m_bChatEnabled = tfvr_popup_hud_chat_enabled.GetBool();
    m_flChatOffsetX = tfvr_popup_hud_chat_offset_x.GetFloat();
    m_flChatOffsetY = tfvr_popup_hud_chat_offset_y.GetFloat();
    m_flChatScale = tfvr_popup_hud_chat_scale.GetFloat();
    
    // Try to acquire panels if we don't have them yet
    if (!m_pScoreboardPanel && !m_pWinPanel)
    {
        AcquirePanels();
    }
    
    // Update spring position
    UpdateSpringYaw(deltaTime);
    
    // Determine which panel to show
    m_pActivePanel = DetermineActivePanel();
    
    if (tfvr_popup_hud_debug.GetBool() && m_pActivePanel)
    {
        static float lastDebugTime = 0;
        if (gpGlobals->curtime - lastDebugTime > 1.0f)
        {
            DevMsg("VR Popup HUD: Active panel visible, yaw=%.1f\n", m_flCurrentYaw);
            lastDebugTime = gpGlobals->curtime;
        }
    }
}

bool CVRPopupHUDManager::CalculateSpringTransform(VMatrix& transform)
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer)
        return false;
    
    // Get head position
    Vector headPos;
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        headPos = worldFromMideye.GetTranslation();
    }
    else
    {
        headPos = pPlayer->EyePosition();
    }
    
    // Calculate panel position using spring yaw
    Vector forward, right, up;
    AngleVectors(QAngle(0, m_flCurrentYaw, 0), &forward, &right, &up);
    
    // Position panel in front of player at spring yaw
    Vector panelPos = headPos + forward * m_flDistance;
    panelPos.z += m_flVerticalOffset;
    
    // Panel faces back toward the player (opposite of forward)
    Vector panelForward = -forward;
    Vector panelRight = right;
    Vector panelUp = Vector(0, 0, 1); // Keep panel upright
    
    // Recalculate right to be perpendicular
    panelRight = CrossProduct(panelUp, panelForward);
    panelRight.NormalizeInPlace();
    
    // Build transform matrix
    transform.Identity();
    
    // The panel is drawn from top-left corner, so we need to offset to center it
    // We'll handle this in the render function
    
    transform[0][0] = panelRight.x;  transform[0][1] = panelUp.x;  transform[0][2] = panelForward.x;
    transform[1][0] = panelRight.y;  transform[1][1] = panelUp.y;  transform[1][2] = panelForward.y;
    transform[2][0] = panelRight.z;  transform[2][1] = panelUp.z;  transform[2][2] = panelForward.z;
    transform.SetTranslation(panelPos);
    
    return true;
}

void CVRPopupHUDManager::Render()
{
    VPROF("VRPopupHUDManager_Render");
    
    if (!m_bInitialized || !m_bEnabled)
        return;
    
    // Safety check
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return;
    
    // Don't render if main menu/game UI is open
    if (enginevgui && enginevgui->IsGameUIVisible())
        return;
    
    // Don't render if class menu is open
    ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
    if (pClassMenuOpen && pClassMenuOpen->GetBool())
        return;
    
    // Calculate spring transform
    VMatrix panelToWorld;
    if (!CalculateSpringTransform(panelToWorld))
        return;
    
    // Head position for local calculations (global queue handles distance sorting)
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        m_headPosForSort = worldFromMideye.GetTranslation();
    }
    else
    {
        m_headPosForSort = pPlayer->EyePosition();
    }
    
    // Determine if we have a main popup panel to render
    bool bHasActivePanel = m_pActivePanel && m_pActivePanel->IsVisible();
    
    // For main popups: Don't render if player is dead (unless it's the scoreboard)
    if (bHasActivePanel && !pPlayer->IsAlive() && m_pActivePanel != m_pScoreboardPanel)
        bHasActivePanel = false;
    
    // Skip main panel rendering if none active
    if (!bHasActivePanel)
    {
        // But still queue notifications if they're enabled and player is alive
        if (m_bNotificationsEnabled && pPlayer->IsAlive())
        {
            RenderNotifications(panelToWorld);
        }
        
        // And still queue voice status even without a main panel
        if (m_bVoiceStatusEnabled)
        {
            RenderVoiceStatus(panelToWorld);
        }
        
        // Always render chat panel (it manages its own fade/visibility internally)
        if (m_bChatEnabled)
        {
            RenderChat(panelToWorld);
        }
        
        // Global queue will flush all panels at end of VR UI rendering
        return;
    }
    
    // Get the panel's actual size and screen position
    int panelWidth, panelHeight;
    m_pActivePanel->GetSize(panelWidth, panelHeight);
    
    int origX, origY;
    m_pActivePanel->GetPos(origX, origY);
    
    // Get screen size to calculate relative position
    int screenWidth, screenHeight;
    vgui::surface()->GetScreenSize(screenWidth, screenHeight);
    
    // Debug: Log panel info
    if (tfvr_popup_hud_debug.GetBool())
    {
        static vgui::Panel* lastDebugPanel = nullptr;
        if (lastDebugPanel != m_pActivePanel)
        {
            // Calculate panel center vs screen center for debug
            float panelCenterX = origX + panelWidth * 0.5f;
            float panelCenterY = origY + panelHeight * 0.5f;
            float screenCenterX = screenWidth * 0.5f;
            float screenCenterY = screenHeight * 0.5f;
            
            DevMsg("VR Popup HUD: Rendering '%s' - Size: %dx%d, Pos: %d,%d, Screen: %dx%d\n",
                m_pActivePanel->GetName(), panelWidth, panelHeight, origX, origY, screenWidth, screenHeight);
            DevMsg("  Panel center: (%.0f, %.0f), Screen center: (%.0f, %.0f), Delta: (%.0f, %.0f)\n",
                panelCenterX, panelCenterY, screenCenterX, screenCenterY,
                panelCenterX - screenCenterX, panelCenterY - screenCenterY);
            lastDebugPanel = m_pActivePanel;
        }
    }
    
    // Sanity check dimensions
    if (panelWidth <= 0 || panelWidth > 4096) panelWidth = 1024;
    if (panelHeight <= 0 || panelHeight > 4096) panelHeight = 768;
    
    // Capture dimensions - use panel size
    int captureWidth = panelWidth;
    int captureHeight = panelHeight;
    
    // Calculate world size maintaining the aspect ratio
    float aspectRatio = (float)captureWidth / (float)captureHeight;
    float worldHeight = m_flScale;
    float worldWidth = worldHeight * aspectRatio;
    
    // For match status, apply special scaling to show just the top portion
    bool bMatchStatusSpecialHandling = (m_pActivePanel == m_pMatchSummaryPanel);
    float matchStatusScale = 1.0f;
    float cropBottom = 0.0f;
    
    if (bMatchStatusSpecialHandling)
    {
        matchStatusScale = tfvr_popup_hud_matchstatus_scale.GetFloat();
        cropBottom = tfvr_popup_hud_matchstatus_crop_bottom.GetFloat();
        
        // Apply scale to world dimensions
        worldWidth *= matchStatusScale;
        worldHeight *= matchStatusScale;
    }
    
    // Get basis vectors from transform
    Vector panelPos = panelToWorld.GetTranslation();
    Vector panelRight(panelToWorld[0][0], panelToWorld[1][0], panelToWorld[2][0]);
    Vector panelUp(panelToWorld[0][1], panelToWorld[1][1], panelToWorld[2][1]);
    
    // Calculate how far the panel center is from screen center
    // For match status, we relocate to (0,0), so the center is just half the capture size
    float panelCenterX, panelCenterY;
    if (bMatchStatusSpecialHandling)
    {
        // We'll relocate the panel to (0,0), so center is based on capture dimensions
        panelCenterX = captureWidth * 0.5f;
        panelCenterY = captureHeight * 0.5f;
    }
    else
    {
        panelCenterX = origX + captureWidth * 0.5f;
        panelCenterY = origY + captureHeight * 0.5f;
    }
    
    // Screen center
    float screenCenterX = screenWidth * 0.5f;
    float screenCenterY = screenHeight * 0.5f;
    
    // Offset from screen center (positive = panel is right/below center)
    float deltaX = panelCenterX - screenCenterX;
    float deltaY = panelCenterY - screenCenterY;
    
    // Convert to normalized (-0.5 to 0.5 range relative to screen)
    float normalizedDeltaX = deltaX / (float)screenWidth;
    float normalizedDeltaY = deltaY / (float)screenHeight;
    
    // Scale by world size (using screen aspect for proper mapping)
    float screenAspect = (float)screenWidth / (float)screenHeight;
    float worldScreenWidth = worldHeight * screenAspect;
    
    // World offset to shift the panel to center
    float worldOffsetX = normalizedDeltaX * worldScreenWidth;
    float worldOffsetY = normalizedDeltaY * worldHeight;
    
    // Apply user configurable offset (global + per-panel)
    float userOffsetX = tfvr_popup_hud_offset_x.GetFloat();
    float userOffsetY = tfvr_popup_hud_offset_y.GetFloat();
    
    // Add per-panel offsets
    if (m_pActivePanel == m_pScoreboardPanel)
    {
        userOffsetX += tfvr_popup_hud_scoreboard_offset_x.GetFloat();
        userOffsetY += tfvr_popup_hud_scoreboard_offset_y.GetFloat();
    }
    else if (m_pActivePanel == m_pWinPanel || m_pActivePanel == m_pArenaWinPanel)
    {
        userOffsetX += tfvr_popup_hud_winpanel_offset_x.GetFloat();
        userOffsetY += tfvr_popup_hud_winpanel_offset_y.GetFloat();
    }
    else if (m_pActivePanel == m_pMatchSummaryPanel)
    {
        userOffsetX += tfvr_popup_hud_matchstatus_offset_x.GetFloat();
        userOffsetY += tfvr_popup_hud_matchstatus_offset_y.GetFloat();
    }
    
    // Center the panel - DrawPanelIn3DSpace draws from top-left, so offset to center
    // Then shift by the calculated offset to compensate for panel's screen position
    Vector topLeft = panelPos 
        - panelRight * (worldWidth * 0.5f)   // Center horizontally (panel's own width)
        + panelUp * (worldHeight * 0.5f)     // Center vertically (panel's own height)
        - panelRight * worldOffsetX          // Shift to compensate for panel being off-center
        + panelUp * worldOffsetY             // Shift to compensate for panel being off-center
        + panelRight * userOffsetX           // User adjustment
        + panelUp * userOffsetY;             // User adjustment
    
    // For match status, shift up to show only the top portion (crop the bottom)
    if (bMatchStatusSpecialHandling && cropBottom > 0.0f)
    {
        // Shift the panel up by (cropBottom * worldHeight) to hide the bottom portion
        // This effectively shows only the top (1 - cropBottom) of the panel
        topLeft += panelUp * (cropBottom * worldHeight);
    }
    
    panelToWorld.SetTranslation(topLeft);
    
    // Queue the main panel for distance-sorted rendering
    // For match status, use the wrapper panel which applies ForceScreenPosOffset during paint
    if (bMatchStatusSpecialHandling && m_pMatchStatusWrapper)
    {
        // Set the target panel for the wrapper to paint
        m_pMatchStatusWrapper->SetTargetPanel(m_pActivePanel);
        m_pMatchStatusWrapper->SetSize(captureWidth, captureHeight);
        
        // Apply content offset (to nudge content into the visible capture area)
        int contentOffsetY = tfvr_popup_hud_matchstatus_content_y.GetInt();
        m_pMatchStatusWrapper->SetContentOffset(0, contentOffsetY);
        m_pMatchStatusWrapper->SetVisible(true);
        
        // Queue the wrapper panel
        QueuePanelForRender(m_pMatchStatusWrapper, panelToWorld, 
                           captureWidth, captureHeight, worldWidth, worldHeight,
                           m_headPosForSort, true, false);
    }
    else
    {
        // Queue normal panel for rendering
        QueuePanelForRender(m_pActivePanel, panelToWorld,
                           captureWidth, captureHeight, worldWidth, worldHeight,
                           m_headPosForSort);
    }
    
    // Queue bottom-center notification panels below the main popup
    if (m_bNotificationsEnabled && pPlayer->IsAlive())
    {
        RenderNotifications(panelToWorld);
    }
    
    // Queue voice status panels (self-icon and other players speaking)
    if (m_bVoiceStatusEnabled)
    {
        RenderVoiceStatus(panelToWorld);
    }
    
    // Queue chat panel (always rendered - manages its own fade)
    if (m_bChatEnabled)
    {
        RenderChat(panelToWorld);
    }
    
    // Global queue will flush all panels at end of VR UI rendering in viewrender.cpp
    
    if (tfvr_popup_hud_debug.GetBool())
    {
        // Draw debug visualization at center
        debugoverlay->AddBoxOverlay(panelPos, Vector(-5, -5, -5), Vector(5, 5, 5),
            QAngle(0, m_flCurrentYaw, 0), 0, 255, 255, 100, 0.0f);
    }
}

void CVRPopupHUDManager::RenderNotificationPanel(vgui::Panel* pPanel, const VMatrix& baseTransform, float verticalOffset, float horizontalOffset, bool bRestoreVisibility, bool bWasVisible)
{
    if (!pPanel || !pPanel->IsVisible())
        return;
    
    int panelWidth, panelHeight;
    pPanel->GetSize(panelWidth, panelHeight);
    
    if (panelWidth <= 0 || panelHeight <= 0)
        return;
    
    // Get the panel's screen position for debug
    int panelX, panelY;
    pPanel->GetPos(panelX, panelY);
    
    // Calculate world dimensions - use a fixed scale relative to notification settings
    float notificationScale = m_flScale * m_flNotificationsScale;
    
    // Scale panel to world size based on a reference height (e.g., 100 pixels = notificationScale units)
    float pixelsPerUnit = 100.0f;
    float worldWidth = (panelWidth / pixelsPerUnit) * notificationScale;
    float worldHeight = (panelHeight / pixelsPerUnit) * notificationScale;
    
    // Get basis vectors from the base transform
    Vector basePos = baseTransform.GetTranslation();
    Vector panelRight(baseTransform[0][0], baseTransform[1][0], baseTransform[2][0]);
    Vector panelUp(baseTransform[0][1], baseTransform[1][1], baseTransform[2][1]);
    Vector panelForward(baseTransform[0][2], baseTransform[1][2], baseTransform[2][2]);
    
    // Get manual offsets from ConVars
    float userOffsetX = tfvr_popup_hud_notifications_offset_x.GetFloat();
    float userOffsetZ = tfvr_popup_hud_notifications_offset_z.GetFloat();
    
    // Calculate top-left position for 3D rendering
    // Simply center the panel at basePos + offsets
    // DrawPanelIn3DSpace renders from top-left, so offset from center to top-left
    Vector topLeft = basePos 
        - panelRight * (worldWidth * 0.5f)       // Center horizontally (offset to left edge)
        + panelUp * (worldHeight * 0.5f)         // Center vertically (offset to top edge)
        + panelUp * verticalOffset               // Apply slot vertical offset
        + panelRight * (userOffsetX + horizontalOffset) // Manual horizontal adjustment + per-panel offset
        - panelForward * userOffsetZ;            // Manual depth adjustment
    
    VMatrix notificationTransform = baseTransform;
    notificationTransform.SetTranslation(topLeft);
    
    if (tfvr_popup_hud_notifications_debug.GetBool())
    {
        DevMsg("NotificationPanel: name=%s screenPos=(%d,%d) size=(%dx%d) worldSize=(%.2fx%.2f)\n",
            pPanel->GetName(), panelX, panelY, panelWidth, panelHeight, worldWidth, worldHeight);
    }
    
    // Queue the panel for distance-sorted rendering
    QueuePanelForRender(pPanel, notificationTransform, panelWidth, panelHeight,
                        worldWidth, worldHeight, m_headPosForSort, bRestoreVisibility, bWasVisible);
}

void CVRPopupHUDManager::RenderNotifications(const VMatrix& baseTransform)
{
    // Each panel type has a fixed offset from the base position
    // This prevents panels from jumping around when other panels appear/disappear
    float baseOffset = m_flNotificationsOffsetY;
    float slotSpacing = tfvr_popup_hud_notifications_spacing.GetFloat();
    
    // CMainTargetID is now rendered above the target entity in world space (see RenderWorldTargetID)
    // CSpectatorTargetID stays on the popup panel for spectating
    
    // Slot 0: Spectator target ID (when spectating)
    if (m_pSpectatorTargetID && m_pSpectatorTargetID->IsVisible())
    {
        RenderNotificationPanel(m_pSpectatorTargetID, baseTransform, baseOffset);
    }
    
    // Slot 0.5: Secondary target ID (healer notification - "Healer: [name]")
    // Shares slot with spectator target ID since they're mutually exclusive (alive vs spectating)
    // Uses wrapper to ensure content is centered (panel has screen-relative positioning internally)
    // NOTE: We check ShouldDraw() on the CHudElement instead of IsVisible() on the panel
    // because VGUI panel visibility is not automatically synced with CHudElement::ShouldDraw()
    // Also verify player is still alive - ShouldDraw may briefly return true after death
    C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    bool bPlayerAlive = pLocalPlayer && pLocalPlayer->IsAlive();
    if (m_pSecondaryTargetID && m_pSecondaryTargetIDElement && bPlayerAlive && 
        m_pSecondaryTargetIDElement->ShouldDraw() && m_pHealerWrapper)
    {
        float healerOffsetX = tfvr_popup_hud_healer_offset_x.GetFloat();
        float healerOffsetY = tfvr_popup_hud_healer_offset_y.GetFloat();
        
        // Get the actual panel size
        int panelWidth, panelHeight;
        m_pSecondaryTargetID->GetSize(panelWidth, panelHeight);
        
        // Configure wrapper to capture just this panel's content
        m_pHealerWrapper->SetTargetPanel(m_pSecondaryTargetID);
        m_pHealerWrapper->SetSize(panelWidth, panelHeight);
        m_pHealerWrapper->SetContentOffset(0, 0);  // Content at origin
        m_pHealerWrapper->SetVisible(true);
        
        // Render the wrapper (which relocates content to 0,0)
        RenderNotificationPanel(m_pHealerWrapper, baseTransform, baseOffset + healerOffsetY, healerOffsetX);
        
        m_pHealerWrapper->SetVisible(false);
    }
    
    // Slot 1: Notification panel (objective notifications like "Intelligence captured")
    // Paint() suppression handles 2D, so don't toggle visibility (breaks stereo rendering)
    float slot1Offset = baseOffset - slotSpacing;
    if (m_pNotificationPanel && m_pNotificationPanel->IsVisible())
    {
        RenderNotificationPanel(m_pNotificationPanel, baseTransform, slot1Offset, 0.0f, false);
    }
    
    // Slot 2: Building status (engineer or spy)
    // Paint() suppression handles 2D, so don't toggle visibility (breaks stereo rendering)
    float buildingOffsetX = tfvr_popup_hud_building_offset_x.GetFloat();
    float buildingOffsetY = tfvr_popup_hud_building_offset_y.GetFloat();
    float slot2Offset = baseOffset - (slotSpacing * 2.0f) + buildingOffsetY;
    if (m_pBuildingStatusEngineer && m_pBuildingStatusEngineer->IsVisible())
    {
        RenderNotificationPanel(m_pBuildingStatusEngineer, baseTransform, slot2Offset, buildingOffsetX, false);
    }
    else if (m_pBuildingStatusSpy && m_pBuildingStatusSpy->IsVisible())
    {
        RenderNotificationPanel(m_pBuildingStatusSpy, baseTransform, slot2Offset, buildingOffsetX, false);
    }
}

extern ConVar tfvr_popup_hud_voice_debug;
extern ConVar tfvr_popup_hud_voice_test;

void CVRPopupHUDManager::RenderVoiceStatus(const VMatrix& /*baseTransform*/)
{
    // Render voice status panels using the SPRING ARM positioning
    // Uses m_flCurrentYaw (spring arm yaw) for consistent positioning
    // Not affected by popup panel size/position - only by spring arm state
    // This includes:
    // - CHudVoiceSelfStatus: Icon when local player is speaking (separate offset)
    // - CHudVoiceStatus: List of other players who are speaking (separate offset)
    
    // Get voice manager to check speaking status directly
    // (We can't rely on panel visibility since we're suppressing the 2D HUD)
    CVoiceStatus* pVoiceMgr = GetClientVoiceMgr();
    bool bTestMode = tfvr_popup_hud_voice_test.GetBool();
    
    if (!pVoiceMgr && !bTestMode)
    {
        if (tfvr_popup_hud_voice_debug.GetBool())
        {
            static float lastDebugTime = 0.0f;
            if (gpGlobals->curtime - lastDebugTime > 1.0f)
            {
                DevMsg("VR Voice UI: No voice manager!\n");
                lastDebugTime = gpGlobals->curtime;
            }
        }
        return;
    }
    
    // Get player for fallback positioning
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return;
    
    // Get head position in world space (same method as main popup panels)
    // This properly tracks the HMD including when crouching
    Vector headPos;
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        headPos = worldFromMideye.GetTranslation();
    }
    else
    {
        headPos = pPlayer->EyePosition();
    }
    
    // Use the SPRING ARM yaw (m_flCurrentYaw) for consistent positioning
    // This is the same spring-follow system used by popup panels
    Vector forward, right, up;
    QAngle springAngles(0, m_flCurrentYaw, 0);
    AngleVectors(springAngles, &forward, &right, &up);
    
    // Calculate base position using head position at voice distance
    Vector basePos = headPos + forward * m_flVoiceDistance;
    
    // Build base rotation matrix (facing player, using spring arm orientation)
    VMatrix voiceBaseTransform;
    voiceBaseTransform.Identity();
    
    // Set rotation: -forward as Z (facing player), right as X, up as Y
    voiceBaseTransform[0][0] = right.x;    voiceBaseTransform[0][1] = up.x;    voiceBaseTransform[0][2] = -forward.x;
    voiceBaseTransform[1][0] = right.y;    voiceBaseTransform[1][1] = up.y;    voiceBaseTransform[1][2] = -forward.y;
    voiceBaseTransform[2][0] = right.z;    voiceBaseTransform[2][1] = up.z;    voiceBaseTransform[2][2] = -forward.z;
    voiceBaseTransform.SetTranslation(basePos);
    
    // Calculate voice panel scale (use m_flScale as base, which is the popup HUD scale)
    float voiceScale = m_flScale * m_flVoiceScale;
    float pixelsPerUnit = 100.0f;
    
    // Check if local player is speaking (use voice manager, not panel visibility)
    // In test mode, always pretend local player is speaking
    bool bLocalPlayerSpeaking = bTestMode || (pVoiceMgr && pVoiceMgr->IsLocalPlayerSpeaking());
    
    // Debug output
    if (tfvr_popup_hud_voice_debug.GetBool())
    {
        static float lastDebugTime = 0.0f;
        if (gpGlobals->curtime - lastDebugTime > 0.5f)
        {
            int selfW = 0, selfH = 0;
            if (m_pVoiceSelfStatus)
                m_pVoiceSelfStatus->GetSize(selfW, selfH);
            
            DevMsg("VR Voice UI: Speaking=%d, Panel=%p, Size=%dx%d, Scale=%.2f, Dist=%.1f, SpringYaw=%.1f\n",
                   bLocalPlayerSpeaking ? 1 : 0,
                   m_pVoiceSelfStatus,
                   selfW, selfH,
                   voiceScale,
                   m_flVoiceDistance,
                   m_flCurrentYaw);
            lastDebugTime = gpGlobals->curtime;
        }
    }
    
    // Voice self status (local player speaking icon)
    if (m_pVoiceSelfStatus && bLocalPlayerSpeaking)
    {
        int panelWidth, panelHeight;
        m_pVoiceSelfStatus->GetSize(panelWidth, panelHeight);
        
        if (panelWidth > 0 && panelHeight > 0)
        {
            float worldWidth = (panelWidth / pixelsPerUnit) * voiceScale;
            float worldHeight = (panelHeight / pixelsPerUnit) * voiceScale;
            
            // Position using SELF offsets
            Vector topLeft = basePos 
                - right * (worldWidth * 0.5f)           // Center horizontally
                + up * (worldHeight * 0.5f)             // Center vertically
                + right * m_flVoiceSelfOffsetX          // Apply self horizontal offset
                + up * m_flVoiceSelfOffsetY;            // Apply self vertical offset
            
            VMatrix voiceTransform = voiceBaseTransform;
            voiceTransform.SetTranslation(topLeft);
            
            // Queue for distance-sorted rendering (needs visibility restore)
            bool bWasVisible = m_pVoiceSelfStatus->IsVisible();
            QueuePanelForRender(m_pVoiceSelfStatus, voiceTransform, panelWidth, panelHeight,
                               worldWidth, worldHeight, m_headPosForSort, true, bWasVisible);
            
            if (tfvr_popup_hud_voice_debug.GetBool())
            {
                DevMsg("VR Voice UI: Queued self-status at (%.1f, %.1f, %.1f), offset (%.1f, %.1f)\n",
                       topLeft.x, topLeft.y, topLeft.z,
                       m_flVoiceSelfOffsetX, m_flVoiceSelfOffsetY);
            }
        }
        else if (tfvr_popup_hud_voice_debug.GetBool())
        {
            DevMsg("VR Voice UI: Self-status panel size is 0!\n");
        }
    }
    
    // Check if any other players are speaking
    // In test mode, don't show other players panel (just test self-status)
    bool bOtherPlayersSpeaking = false;
    if (pVoiceMgr)
    {
        for (int i = 1; i <= gpGlobals->maxClients; i++)
        {
            if (pVoiceMgr->IsPlayerSpeaking(i))
            {
                bOtherPlayersSpeaking = true;
                break;
            }
        }
    }
    
    // Voice status (other players speaking)
    if (m_pVoiceStatus && bOtherPlayersSpeaking)
    {
        int panelWidth, panelHeight;
        m_pVoiceStatus->GetSize(panelWidth, panelHeight);
        
        if (panelWidth > 0 && panelHeight > 0)
        {
            float worldWidth = (panelWidth / pixelsPerUnit) * voiceScale;
            float worldHeight = (panelHeight / pixelsPerUnit) * voiceScale;
            
            // Position using OTHERS offsets (completely independent of self-status)
            Vector topLeft = basePos 
                - right * (worldWidth * 0.5f)           // Center horizontally
                + up * (worldHeight * 0.5f)             // Center vertically
                + right * m_flVoiceOthersOffsetX        // Apply others horizontal offset
                + up * m_flVoiceOthersOffsetY;          // Apply others vertical offset
            
            VMatrix voiceTransform = voiceBaseTransform;
            voiceTransform.SetTranslation(topLeft);
            
            // Queue for distance-sorted rendering (needs visibility restore)
            bool bWasVisible = m_pVoiceStatus->IsVisible();
            QueuePanelForRender(m_pVoiceStatus, voiceTransform, panelWidth, panelHeight,
                               worldWidth, worldHeight, m_headPosForSort, true, bWasVisible);
        }
    }
}

void CVRPopupHUDManager::RenderChat(const VMatrix& /*baseTransform*/)
{
    if (!m_pChatPanel)
        return;
    
    int panelWidth, panelHeight;
    m_pChatPanel->GetSize(panelWidth, panelHeight);
    
    if (panelWidth <= 0 || panelHeight <= 0)
        return;
    
    // Get player for positioning
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return;
    
    // Get head position
    Vector headPos;
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        headPos = worldFromMideye.GetTranslation();
    }
    else
    {
        headPos = pPlayer->EyePosition();
    }
    
    // Use the spring arm yaw for consistent positioning
    Vector forward, right, up;
    QAngle springAngles(0, m_flCurrentYaw, 0);
    AngleVectors(springAngles, &forward, &right, &up);
    
    // Position at the same distance as other popup panels
    Vector basePos = headPos + forward * m_flDistance;
    
    // Calculate world dimensions
    float chatScale = m_flScale * m_flChatScale;
    float pixelsPerUnit = 100.0f;
    float worldWidth = (panelWidth / pixelsPerUnit) * chatScale;
    float worldHeight = (panelHeight / pixelsPerUnit) * chatScale;
    
    // Build rotation matrix (facing player)
    VMatrix chatTransform;
    chatTransform.Identity();
    chatTransform[0][0] = right.x;    chatTransform[0][1] = up.x;    chatTransform[0][2] = -forward.x;
    chatTransform[1][0] = right.y;    chatTransform[1][1] = up.y;    chatTransform[1][2] = -forward.y;
    chatTransform[2][0] = right.z;    chatTransform[2][1] = up.z;    chatTransform[2][2] = -forward.z;
    
    // Position with offsets (bottom-left by default)
    Vector topLeft = basePos
        - right * (worldWidth * 0.5f)
        + up * (worldHeight * 0.5f)
        + right * m_flChatOffsetX
        + up * m_flChatOffsetY;
    
    chatTransform.SetTranslation(topLeft);
    
    if (tfvr_popup_hud_chat_debug.GetBool())
    {
        static float lastDebugTime = 0.0f;
        if (gpGlobals->curtime - lastDebugTime > 1.0f)
        {
            DevMsg("VR Chat: Panel=%p, Size=%dx%d, WorldSize=(%.1f, %.1f), Offset=(%.1f, %.1f)\n",
                   m_pChatPanel, panelWidth, panelHeight, worldWidth, worldHeight,
                   m_flChatOffsetX, m_flChatOffsetY);
            lastDebugTime = gpGlobals->curtime;
        }
    }
    
    // Capture current visibility so we restore correctly after 3D capture.
    // When the user is typing, the panel is visible (suppression is bypassed)
    // and must stay visible for VGUI keyboard focus to work.
    bool bWasVisible = m_pChatPanel->IsVisible();
    QueuePanelForRender(m_pChatPanel, chatTransform, panelWidth, panelHeight,
                       worldWidth, worldHeight, m_headPosForSort, true, bWasVisible);
}

//=============================================================================
// Distance-sorted rendering (uses global VR World UI Queue)
//=============================================================================

// Priority levels for popup HUD elements (higher = rendered later/on top)
static const int PRIORITY_POPUP_MAIN = 100;       // Main popup panels (scoreboard, win/loss)
static const int PRIORITY_POPUP_NOTIFICATION = 90; // Notification panels
static const int PRIORITY_POPUP_VOICE = 80;       // Voice status panels

void CVRPopupHUDManager::QueuePanelForRender(vgui::Panel* pPanel, const VMatrix& transform,
                                              int pixelWidth, int pixelHeight,
                                              float worldWidth, float worldHeight,
                                              const Vector& /*headPos*/,
                                              bool bRestoreVisibility, bool bWasVisible)
{
    // Use the global VR World UI Queue for distance-sorted rendering
    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        g_pVRWorldUIQueue->QueuePanel(pPanel, transform, pixelWidth, pixelHeight,
                                      worldWidth, worldHeight, PRIORITY_POPUP_MAIN,
                                      bRestoreVisibility, bWasVisible);
    }
    else
    {
        // Fallback: render immediately if global queue not available
        if (!pPanel || pixelWidth <= 0 || pixelHeight <= 0)
            return;
        
        if (bRestoreVisibility)
            pPanel->SetVisible(true);
        
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            pPanel->GetVPanel(),
            transform,
            pixelWidth,
            pixelHeight,
            worldWidth,
            worldHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        
        if (bRestoreVisibility)
            pPanel->SetVisible(bWasVisible);
    }
}

void CVRPopupHUDManager::RenderQueuedPanels()
{
    // No longer needed - global queue handles rendering
    // Kept for API compatibility
}

void CVRPopupHUDManager::ClearRenderQueue()
{
    // No longer needed - global queue handles clearing
    // Kept for API compatibility
}

void CVRPopupHUDManager::ResetState()
{
    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;
    m_pActivePanel = nullptr;
}

//=============================================================================
// Static suppression methods for vanilla 2D rendering
//=============================================================================

bool CVRPopupHUDManager::ShouldSuppressNotificationPanel()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bNotificationsEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressSpectatorTargetID()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bNotificationsEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressSecondaryTargetID()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bNotificationsEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    // Don't suppress when VR wrapper panel is actively painting
    // This allows the panel to render when captured for 3D display
    if (CVRPanelWrapper::s_bInsideWrapperPaint)
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressBuildingStatus()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bNotificationsEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    // Don't suppress when the world UI queue is actively flushing (3D capture)
    // or when a VR wrapper panel is painting — the panel must paint normally
    // so DrawPanelIn3DSpace can capture its content.
    if (CVRWorldUIQueue::s_bInsideFlush || CVRPanelWrapper::s_bInsideWrapperPaint)
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressVoiceSelfStatus()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bVoiceStatusEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressVoiceStatus()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bVoiceStatusEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    return true;
}

bool CVRPopupHUDManager::ShouldSuppressChat()
{
    if (!g_pVRPopupHUDManager)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bInitialized)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bEnabled)
        return false;
    
    if (!g_pVRPopupHUDManager->m_bChatEnabled)
        return false;
    
    // Only suppress if VR is active
    if (!UseVR())
        return false;
    
    // Don't suppress when VR wrapper panel is actively painting
    if (CVRPanelWrapper::s_bInsideWrapperPaint)
        return false;
    
    // Don't suppress when the user is actively typing in chat.
    // The panel must remain visible for VGUI keyboard focus/input to work.
    // StartMessageMode() sets keyboard input enabled; StopMessageMode() clears it.
    if (g_pVRPopupHUDManager->m_pChatPanel && 
        g_pVRPopupHUDManager->m_pChatPanel->IsKeyBoardInputEnabled())
        return false;
    
    return true;
}

//=============================================================================
// Debug method to dump voice UI status
//=============================================================================
void CVRPopupHUDManager::DebugDumpVoiceUIState()
{
    Msg("=== VR Voice UI Debug ===\n");
    Msg("Initialized: %d\n", m_bInitialized ? 1 : 0);
    Msg("Enabled: %d\n", m_bEnabled ? 1 : 0);
    Msg("VoiceStatusEnabled: %d\n", m_bVoiceStatusEnabled ? 1 : 0);
    Msg("UseVR(): %d\n", UseVR() ? 1 : 0);
    
    Msg("\n--- Panels ---\n");
    Msg("VoiceSelfStatus panel: %p\n", m_pVoiceSelfStatus);
    Msg("VoiceStatus panel: %p\n", m_pVoiceStatus);
    
    if (m_pVoiceSelfStatus)
    {
        int w, h;
        m_pVoiceSelfStatus->GetSize(w, h);
        Msg("VoiceSelfStatus size: %dx%d\n", w, h);
        Msg("VoiceSelfStatus visible: %d\n", m_pVoiceSelfStatus->IsVisible() ? 1 : 0);
    }
    
    if (m_pVoiceStatus)
    {
        int w, h;
        m_pVoiceStatus->GetSize(w, h);
        Msg("VoiceStatus size: %dx%d\n", w, h);
        Msg("VoiceStatus visible: %d\n", m_pVoiceStatus->IsVisible() ? 1 : 0);
    }
    
    Msg("\n--- Voice Manager ---\n");
    CVoiceStatus* pVoiceMgr = GetClientVoiceMgr();
    if (pVoiceMgr)
    {
        Msg("Voice manager: %p\n", pVoiceMgr);
        Msg("Local player speaking: %d\n", pVoiceMgr->IsLocalPlayerSpeaking() ? 1 : 0);
        
        int speakingCount = 0;
        for (int i = 1; i <= gpGlobals->maxClients; i++)
        {
            if (pVoiceMgr->IsPlayerSpeaking(i))
            {
                Msg("  Player %d is speaking\n", i);
                speakingCount++;
            }
        }
        Msg("Total players speaking: %d\n", speakingCount);
    }
    else
    {
        Msg("Voice manager: NULL\n");
    }
    
    Msg("\n--- Config ---\n");
    Msg("Voice scale: %.2f (m_flScale=%.2f * m_flVoiceScale=%.2f)\n", 
        m_flScale * m_flVoiceScale,
        m_flScale,
        m_flVoiceScale);
    Msg("Voice distance: %.1f\n", m_flVoiceDistance);
    Msg("Voice self offset: (%.1f, %.1f)\n", 
        m_flVoiceSelfOffsetX,
        m_flVoiceSelfOffsetY);
    Msg("Voice others offset: (%.1f, %.1f)\n", 
        m_flVoiceOthersOffsetX,
        m_flVoiceOthersOffsetY);
    
    // Try to find the HUD elements directly
    Msg("\n--- HUD Element Search ---\n");
    CHudElement* pSelfElement = gHUD.FindElement("CHudVoiceSelfStatus");
    CHudElement* pStatusElement = gHUD.FindElement("CHudVoiceStatus");
    Msg("FindElement(CHudVoiceSelfStatus): %p\n", pSelfElement);
    Msg("FindElement(CHudVoiceStatus): %p\n", pStatusElement);
    
    if (pSelfElement)
    {
        vgui::Panel* pPanel = dynamic_cast<vgui::Panel*>(pSelfElement);
        Msg("  Cast to vgui::Panel: %p\n", pPanel);
        if (pPanel)
        {
            int w, h;
            pPanel->GetSize(w, h);
            Msg("  Panel size: %dx%d\n", w, h);
        }
    }
}

//=============================================================================
// Console command to dump voice UI status
//=============================================================================
CON_COMMAND(tfvr_debug_voice_ui, "Dump VR voice UI debug info")
{
    if (!g_pVRPopupHUDManager)
    {
        Msg("VR Popup HUD Manager not initialized\n");
        return;
    }
    
    g_pVRPopupHUDManager->DebugDumpVoiceUIState();
}

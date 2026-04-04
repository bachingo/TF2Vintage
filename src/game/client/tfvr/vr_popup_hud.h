#ifndef VR_POPUP_HUD_H
#define VR_POPUP_HUD_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/EditablePanel.h"
#include "utlvector.h"

// Forward declarations
class CHudElement;

//-----------------------------------------------------------------------------
// Purpose: Render item for distance-sorted 3D UI rendering
//-----------------------------------------------------------------------------
struct VRPanelRenderItem
{
    vgui::Panel* pPanel;        // The panel to render
    VMatrix transform;          // World transform for rendering
    int pixelWidth;             // Panel width in pixels
    int pixelHeight;            // Panel height in pixels
    float worldWidth;           // World width in units
    float worldHeight;          // World height in units
    float distanceFromHead;     // Distance from head (for sorting)
    bool bRestoreVisibility;    // Whether to restore visibility after render
    bool bWasVisible;           // Original visibility state
};

//-----------------------------------------------------------------------------
// Purpose: Simple wrapper panel that paints a target panel at (0,0) offset
//          Used to bypass clipping issues when rendering HUD panels in 3D
//-----------------------------------------------------------------------------
class CVRPanelWrapper : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CVRPanelWrapper, vgui::EditablePanel);
public:
    CVRPanelWrapper(vgui::Panel* parent, const char* name);
    
    void SetTargetPanel(vgui::Panel* pPanel) { m_pTargetPanel = pPanel; }
    vgui::Panel* GetTargetPanel() const { return m_pTargetPanel; }
    
    // Content offset to nudge where content renders (like the hand HUD does)
    void SetContentOffset(int x, int y) { m_nContentOffsetX = x; m_nContentOffsetY = y; }
    
    virtual void Paint() override;
    
    // Static flag to bypass suppression during VR wrapper paint
    // Set to true inside Paint() so suppression functions know we're actively rendering
    static bool s_bInsideWrapperPaint;
    
private:
    vgui::Panel* m_pTargetPanel;
    int m_nContentOffsetX;
    int m_nContentOffsetY;
};

//-----------------------------------------------------------------------------
// Purpose: Manages VR popup HUD elements like win/loss panels and scoreboard.
//          Uses spring-follow positioning for comfortable viewing.
//          Scoreboard overrides win/loss panel when TAB is held (vanilla behavior).
//          Also renders bottom-center notifications below the match status.
//-----------------------------------------------------------------------------
class CVRPopupHUDManager
{
public:
    CVRPopupHUDManager();
    ~CVRPopupHUDManager();
    
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void ResetState();
    
    void SetEnabled(bool enabled) { m_bEnabled = enabled; }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Check if any popup is currently being shown
    bool IsShowingPopup() const { return m_pActivePanel != nullptr; }
    
    // Debug output for voice UI state
    void DebugDumpVoiceUIState();
    
    // Static methods for VR suppression of vanilla 2D rendering
    static bool ShouldSuppressNotificationPanel();
    static bool ShouldSuppressSpectatorTargetID();
    static bool ShouldSuppressSecondaryTargetID();
    static bool ShouldSuppressBuildingStatus();
    static bool ShouldSuppressVoiceSelfStatus();
    static bool ShouldSuppressVoiceStatus();
    static bool ShouldSuppressChat();

private:
    // Find TF2's HUD panels
    void AcquirePanels();
    
    // Determine which panel should be shown (scoreboard has priority)
    vgui::Panel* DetermineActivePanel();
    
    // Spring-follow positioning
    bool CalculateSpringTransform(VMatrix& transform);
    float GetCurrentViewYaw() const;
    void UpdateSpringYaw(float deltaTime);
    
    // Render a single notification panel at a given vertical offset and optional horizontal offset
    void RenderNotificationPanel(vgui::Panel* pPanel, const VMatrix& baseTransform, float verticalOffset, float horizontalOffset = 0.0f, bool bRestoreVisibility = true, bool bWasVisible = false);
    
    // Render all notification panels (called from Render)
    void RenderNotifications(const VMatrix& baseTransform);
    
    // Render voice status panels (self-icon and other players speaking)
    void RenderVoiceStatus(const VMatrix& baseTransform);
    
    // Render chat panel (text chat window)
    void RenderChat(const VMatrix& baseTransform);
    
    // Distance-sorted rendering
    void QueuePanelForRender(vgui::Panel* pPanel, const VMatrix& transform, 
                             int pixelWidth, int pixelHeight,
                             float worldWidth, float worldHeight,
                             const Vector& headPos,
                             bool bRestoreVisibility = false, bool bWasVisible = false);
    void RenderQueuedPanels();
    void ClearRenderQueue();
    
    CUtlVector<VRPanelRenderItem> m_renderQueue;
    Vector m_headPosForSort;  // Cached head position for distance calculations
    
private:
    bool m_bInitialized;
    bool m_bEnabled;
    
    // TF2 HUD panels we capture
    vgui::Panel* m_pScoreboardPanel;      // Scoreboard (TAB)
    vgui::Panel* m_pWinPanel;             // Win/loss panel
    vgui::Panel* m_pArenaWinPanel;        // Arena mode win panel
    vgui::Panel* m_pMatchSummaryPanel;    // Match summary
    
    // Bottom-center notification panels
    vgui::Panel* m_pNotificationPanel;    // CHudNotificationPanel - objective notifications
    vgui::Panel* m_pMainTargetID;         // CMainTargetID - target ID when pointing at players/buildings
    vgui::Panel* m_pSpectatorTargetID;    // CSpectatorTargetID - spectator mode target ID
    vgui::Panel* m_pSecondaryTargetID;    // CSecondaryTargetID - healer notification
    CHudElement* m_pSecondaryTargetIDElement; // CHudElement pointer for ShouldDraw() check
    vgui::Panel* m_pBuildingStatusEngineer; // CHudBuildingStatusContainer_Engineer
    vgui::Panel* m_pBuildingStatusSpy;    // CHudBuildingStatusContainer_Spy
    
    // Voice status panels
    vgui::Panel* m_pVoiceSelfStatus;      // CHudVoiceSelfStatus - local player speaking icon
    vgui::Panel* m_pVoiceStatus;          // CHudVoiceStatus - other players speaking list
    
    // Chat panel
    vgui::Panel* m_pChatPanel;            // CHudChat - text chat window
    CHudElement* m_pChatElement;          // CHudElement pointer for ShouldDraw() check
    
    // Wrapper panel for rendering match status without clipping
    CVRPanelWrapper* m_pMatchStatusWrapper;
    
    // Wrapper panel for rendering healer notification centered
    CVRPanelWrapper* m_pHealerWrapper;
    
    // Currently active panel (what we're rendering)
    vgui::Panel* m_pActivePanel;
    
    // Spring arm state
    float m_flCurrentYaw;       // Current spring arm yaw (world space)
    float m_flTargetYaw;        // Target yaw (player's view yaw)
    
    // Configuration
    float m_flDistance;         // Distance from head
    float m_flFollowSpeed;      // How fast the HUD follows rotation
    float m_flDeadzone;         // Angle deadzone where HUD stays locked
    float m_flMaxLagAngle;      // Max angle HUD can lag behind
    float m_flScale;            // World scale of the panel
    
    // Vertical offset (positive = up)
    float m_flVerticalOffset;
    
    // Notification area configuration
    bool m_bNotificationsEnabled;
    float m_flNotificationsOffsetY;     // Vertical offset below timer
    float m_flNotificationsScale;       // Scale multiplier for notifications
    
    // Voice status UI configuration
    bool m_bVoiceStatusEnabled;
    float m_flVoiceDistance;            // Distance from head (independent transform)
    float m_flVoiceScale;               // Scale multiplier for voice panels
    
    // Self-status (local player speaking icon) offsets
    float m_flVoiceSelfOffsetX;         // Horizontal offset (positive = right)
    float m_flVoiceSelfOffsetY;         // Vertical offset (positive = up)
    
    // Other players speaking list offsets  
    float m_flVoiceOthersOffsetX;       // Horizontal offset (positive = right)
    float m_flVoiceOthersOffsetY;       // Vertical offset (positive = up)
    
    // Chat panel configuration
    bool m_bChatEnabled;
    float m_flChatOffsetX;              // Horizontal offset (positive = right)
    float m_flChatOffsetY;              // Vertical offset (positive = up)
    float m_flChatScale;                // Scale multiplier for chat panel
};

// Global instance
extern CVRPopupHUDManager* g_pVRPopupHUDManager;

#endif // VR_POPUP_HUD_H

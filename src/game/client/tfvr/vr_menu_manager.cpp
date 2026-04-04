#include "cbase.h"
#include "vr_menu_manager.h"
#include "c_tf_player.h"
#include "openxr_manager.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"
#include "vgui/IInputInternal.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "mathlib/mathlib.h"
#include "convar.h"
#include "iclientmode.h"
#include "hud.h"
#include "hudelement.h"
#include "menu.h"
#include "tf_viewport.h"
#include "tf_shareddefs.h"
#include "tf_playerclass_shared.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "iloadingdisc.h" 
#include "tfvr/hmdWrapper.h"
#include "econ/econ_ui.h"
#include "tf/vgui/class_loadout_panel.h"
#include "tf/vgui/character_info_panel.h"
#include "vr_hand_hud_compositor.h"
#include "vr_spring_hud.h"
#include "vr_damage_indicator.h"
#include "vr_weapon_select.h"
#include "vr_popup_hud.h"
#include "vr_world_health_icon.h"
#include "vr_damage_numbers.h"
#include "vr_spectator_extras.h"
#include "vr_spectator_camera.h"
#include "vr_collision_warning.h"
#include "vr_world_ui_queue.h"
#include "vr_controller_model.h"

// Game-specific externs
extern IViewPort* gViewPortInterface;
extern IClientMode* g_pClientMode;

// Global menu manager pointer
CVRMenuManager* g_pVRMenuManager = nullptr;

// ConVars for VR menu control
ConVar tfvr_primary_hand("tfvr_primary_hand", "1", FCVAR_ARCHIVE, "Primary hand for VR input: 0=left, 1=right");
ConVar tfvr_menu_distance("tfvr_menu_distance", "70", FCVAR_ARCHIVE, "Distance from player to VR menu plane");
extern ConVar tfvr_menu_scale;
ConVar tfvr_cursor_threshold("tfvr_cursor_threshold", "0.05", FCVAR_ARCHIVE, "Minimum VR controller movement required to override mouse (in world units)");
ConVar tfvr_cursor_head_threshold("tfvr_cursor_head_threshold", "0.1", FCVAR_ARCHIVE, "Minimum VR head movement required to override mouse (in world units)");
ConVar tfvr_cursor_debug("tfvr_cursor_debug", "0", FCVAR_ARCHIVE, "Show debug info for VR cursor threshold");
ConVar tfvr_cursor_mouse_priority("tfvr_cursor_mouse_priority", "1", FCVAR_ARCHIVE, "Enable mouse/VR priority system: 0=VR only, 1=Auto (more active input wins), 2=Mouse only");
ConVar tfvr_cursor_vr_sensitivity("tfvr_cursor_vr_sensitivity", "500", FCVAR_ARCHIVE, "Scaling factor to compare VR movement to mouse (higher = VR needs less movement to win)");
ConVar tfvr_cursor_smoothing("tfvr_cursor_smoothing", "0.8", FCVAR_ARCHIVE, "Smoothing for input activity tracking (0-1, higher = slower response to input changes)");
ConVar tfvr_cursor_vr_deadzone("tfvr_cursor_vr_deadzone", "2", FCVAR_ARCHIVE, "Dead zone radius for VR controller (world units). VR only takes priority when controller moves outside this radius from where mouse took over.");
ConVar tfvr_cursor_vr_reclaim_cooldown("tfvr_cursor_vr_reclaim_cooldown", "0.5", FCVAR_ARCHIVE, "Seconds after VR reclaims priority during which mouse cannot re-take (prevents flip-flop)");
ConVar tfvr_menu_debug("tfvr_menu_debug", "0", FCVAR_ARCHIVE, "Show debug info for VR menu rendering");
ConVar tfvr_playspace_anchoring("tfvr_playspace_anchoring", "1", FCVAR_ARCHIVE, "Anchor menu to playspace origin instead of player");
ConVar tfvr_class_menu_hold_threshold("tfvr_class_menu_hold_threshold", "0.25", FCVAR_ARCHIVE, "Hold time in seconds for team menu (quick release = class menu)");

// Helper function to get scaled menu dimensions
static void GetScaledMenuDimensions(float& menuWidth, float& menuHeight)
{
    float baseHeight = 80.0f; // Base height in world units
    float scale = tfvr_menu_scale.GetFloat();
    
    menuHeight = baseHeight * scale;
    menuWidth = menuHeight * (16.0f / 9.0f); // Maintain 16:9 aspect ratio
}

CVRMenuManager::CVRMenuManager()
    : m_bMenuButtonPressed(false)
    , m_bMenuButtonReleased(false)
    , m_nMenuHand(1) // Default to right hand
    , m_nOldCursorX(-1)
    , m_nOldCursorY(-1)
    , m_nLastMouseCursorX(-1)
    , m_nLastMouseCursorY(-1)
    , m_vecLastControllerPos(0, 0, 0)
    , m_vecControllerAnchor(0, 0, 0)
    , m_flRecentMouseMovement(0.0f)
    , m_flRecentVRMovement(0.0f)
    , m_bVRCursorUpdateInProgress(false)
    , m_bMouseHasPriority(false)
    , m_flVRReclaimCooldown(0.0f)
    , m_pVRManager(nullptr)
    , m_pLocalPlayer(nullptr)
    , m_savedPlayerViewOrigin(0, 0, 0)
    , m_fixedMenuPosition(0, 0, 0)
    , m_fixedMenuRotation(0, 0, 0)
    , m_bMenuPositionFixed(false)
    , m_bUsePlayspaceAnchoring(true)
    , m_pConVarPrimaryHand(nullptr)
    , m_szLastMapName("")
    , m_flLastClassMenuTime(0.0f)
    , m_flClassMenuButtonPressTime(0.0f)
    , m_bClassMenuHoldActionExecuted(false)
    , m_bVRFrameStarted(false)
    , m_nLastVRTrackingUpdateFrame(-1)
    , m_pVRStatusHUDManager(nullptr)
    , m_pVRWeaponHUDManager(nullptr)
    , m_pVRSpringHUDManager(nullptr)
    , m_pVRDamageIndicatorManager(nullptr)
    , m_pVRWeaponSelectManager(nullptr)
    , m_pVRPopupHUDManager(nullptr)
    , m_pVRWorldHealthIconManager(nullptr)
    , m_pVRDamageNumberManager(nullptr)
    , m_pVRSpectatorExtrasManager(nullptr)
    , m_pVRSpectatorCamera(nullptr)
    , m_pVRCollisionWarningManager(nullptr)
{
    m_menuPlayspaceAnchor.Identity();
}

CVRMenuManager::~CVRMenuManager()
{
    Shutdown();
}

void CVRMenuManager::Initialize()
{
    m_pConVarPrimaryHand = cvar->FindVar("tfvr_primary_hand");
    if (m_pConVarPrimaryHand)
    {
        m_nMenuHand = m_pConVarPrimaryHand->GetInt();
    }
    
    // Initialize VR World UI Queue first (used by all other managers for distance-sorted rendering)
    if (!g_pVRWorldUIQueue)
    {
        g_pVRWorldUIQueue = new CVRWorldUIQueue();
        if (g_pVRWorldUIQueue->Initialize())
        {
            DevMsg("VR Menu Manager: World UI Queue initialized\n");
        }
        else
        {
            delete g_pVRWorldUIQueue;
            g_pVRWorldUIQueue = nullptr;
            Warning("VR Menu Manager: Failed to initialize World UI Queue\n");
        }
    }
    
    // Initialize VR Status HUD Manager (left hand: health, objectives)
    if (!m_pVRStatusHUDManager)
    {
        m_pVRStatusHUDManager = new CVRStatusHUDManager();
        if (m_pVRStatusHUDManager->Initialize())
        {
            g_pVRStatusHUDManager = m_pVRStatusHUDManager;
            DevMsg("VR Menu Manager: Status HUD Manager initialized\n");
        }
        else
        {
            delete m_pVRStatusHUDManager;
            m_pVRStatusHUDManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Status HUD Manager\n");
        }
    }
    
    // Initialize VR Weapon HUD Manager (right hand: ammo, meters, charges)
    if (!m_pVRWeaponHUDManager)
    {
        m_pVRWeaponHUDManager = new CVRWeaponHUDManager();
        if (m_pVRWeaponHUDManager->Initialize())
        {
            g_pVRWeaponHUDManager = m_pVRWeaponHUDManager;
            DevMsg("VR Menu Manager: Weapon HUD Manager initialized\n");
        }
        else
        {
            delete m_pVRWeaponHUDManager;
            m_pVRWeaponHUDManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Weapon HUD Manager\n");
        }
    }
    
    // Initialize VR Spring HUD Manager (head-relative: kill feed)
    if (!m_pVRSpringHUDManager)
    {
        m_pVRSpringHUDManager = new CVRSpringHUDManager();
        if (m_pVRSpringHUDManager->Initialize())
        {
            g_pVRSpringHUDManager = m_pVRSpringHUDManager;
            DevMsg("VR Menu Manager: Spring HUD Manager initialized\n");
        }
        else
        {
            delete m_pVRSpringHUDManager;
            m_pVRSpringHUDManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Spring HUD Manager\n");
        }
    }
    
    // Initialize VR Damage Indicator Manager (head-relative: damage direction)
    if (!m_pVRDamageIndicatorManager)
    {
        m_pVRDamageIndicatorManager = new CVRDamageIndicatorManager();
        if (m_pVRDamageIndicatorManager->Initialize())
        {
            g_pVRDamageIndicatorManager = m_pVRDamageIndicatorManager;
            DevMsg("VR Menu Manager: Damage Indicator Manager initialized\n");
        }
        else
        {
            delete m_pVRDamageIndicatorManager;
            m_pVRDamageIndicatorManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Damage Indicator Manager\n");
        }
    }
    
    // Initialize VR Weapon Select Manager (radial weapon selection)
    if (!m_pVRWeaponSelectManager)
    {
        m_pVRWeaponSelectManager = new CVRWeaponSelectManager();
        if (m_pVRWeaponSelectManager->Initialize())
        {
            g_pVRWeaponSelectManager = m_pVRWeaponSelectManager;
            DevMsg("VR Menu Manager: Weapon Select Manager initialized\n");
        }
        else
        {
            delete m_pVRWeaponSelectManager;
            m_pVRWeaponSelectManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Weapon Select Manager\n");
        }
    }
    
    // Initialize VR Popup HUD Manager (head-relative: win/loss, scoreboard)
    if (!m_pVRPopupHUDManager)
    {
        m_pVRPopupHUDManager = new CVRPopupHUDManager();
        if (m_pVRPopupHUDManager->Initialize())
        {
            g_pVRPopupHUDManager = m_pVRPopupHUDManager;
            DevMsg("VR Menu Manager: Popup HUD Manager initialized\n");
        }
        else
        {
            delete m_pVRPopupHUDManager;
            m_pVRPopupHUDManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Popup HUD Manager\n");
        }
    }
    
    // Initialize VR World Health Icon Manager (world-space health icons above players)
    if (!m_pVRWorldHealthIconManager)
    {
        m_pVRWorldHealthIconManager = new CVRWorldHealthIconManager();
        if (m_pVRWorldHealthIconManager->Initialize())
        {
            g_pVRWorldHealthIconManager = m_pVRWorldHealthIconManager;
            DevMsg("VR Menu Manager: World Health Icon Manager initialized\n");
        }
        else
        {
            delete m_pVRWorldHealthIconManager;
            m_pVRWorldHealthIconManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize World Health Icon Manager\n");
        }
    }
    
    // Initialize VR Damage Number Manager (world-space floating damage numbers)
    if (!m_pVRDamageNumberManager)
    {
        m_pVRDamageNumberManager = new CVRDamageNumberManager();
        if (m_pVRDamageNumberManager->Initialize())
        {
            g_pVRDamageNumberManager = m_pVRDamageNumberManager;
            DevMsg("VR Menu Manager: Damage Number Manager initialized\n");
        }
        else
        {
            delete m_pVRDamageNumberManager;
            m_pVRDamageNumberManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Damage Number Manager\n");
        }
    }
    
    // Initialize VR Spectator Extras Manager (world-space player names and health bars)
    if (!m_pVRSpectatorExtrasManager)
    {
        m_pVRSpectatorExtrasManager = new CVRSpectatorExtrasManager();
        if (m_pVRSpectatorExtrasManager->Initialize())
        {
            g_pVRSpectatorExtrasManager = m_pVRSpectatorExtrasManager;
            DevMsg("VR Menu Manager: Spectator Extras Manager initialized\n");
        }
        else
        {
            delete m_pVRSpectatorExtrasManager;
            m_pVRSpectatorExtrasManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Spectator Extras Manager\n");
        }
    }
    
    // Initialize VR Spectator Camera (Half-Life: Alyx style camera smoothing)
    if (!m_pVRSpectatorCamera)
    {
        m_pVRSpectatorCamera = new CVRSpectatorCamera();
        if (m_pVRSpectatorCamera->Initialize())
        {
            g_pVRSpectatorCamera = m_pVRSpectatorCamera;
            DevMsg("VR Menu Manager: Spectator Camera initialized\n");
        }
        else
        {
            delete m_pVRSpectatorCamera;
            m_pVRSpectatorCamera = nullptr;
            Warning("VR Menu Manager: Failed to initialize Spectator Camera\n");
        }
    }
    
    // Initialize VR Collision Warning (floating warning text)
    if (!m_pVRCollisionWarningManager)
    {
        m_pVRCollisionWarningManager = new CVRCollisionWarningManager();
        if (m_pVRCollisionWarningManager->Initialize())
        {
            g_pVRCollisionWarningManager = m_pVRCollisionWarningManager;
            DevMsg("VR Menu Manager: Collision Warning Manager initialized\n");
        }
        else
        {
            delete m_pVRCollisionWarningManager;
            m_pVRCollisionWarningManager = nullptr;
            Warning("VR Menu Manager: Failed to initialize Collision Warning Manager\n");
        }
    }
    
    // Initialize VR Controller Model Manager (shows controllers during preamble/death)
    if (!g_pVRControllerModelManager && g_pOpenXRManager)
    {
        g_pVRControllerModelManager = new CVRControllerModelManager();
        if (g_pVRControllerModelManager->Initialize(g_pOpenXRManager))
        {
            DevMsg("VR Menu Manager: Controller Model Manager initialized\n");
        }
        else
        {
            delete g_pVRControllerModelManager;
            g_pVRControllerModelManager = nullptr;
            DevMsg("VR Menu Manager: Controller Model Manager not available (render model extension may not be supported)\n");
        }
    }
    
    // VR Menu Manager initialized
    
    // Ensure initial HUD positioning is set up for compositor
    // This handles the startup case when no player exists yet
    extern void NotifyCompositorPlayspaceUpdate();
    NotifyCompositorPlayspaceUpdate();
}

void CVRMenuManager::Shutdown()
{
    // Shutdown VR Status HUD Manager
    if (m_pVRStatusHUDManager)
    {
        m_pVRStatusHUDManager->Shutdown();
        delete m_pVRStatusHUDManager;
        m_pVRStatusHUDManager = nullptr;
        g_pVRStatusHUDManager = nullptr;
    }
    
    // Shutdown VR Weapon HUD Manager
    if (m_pVRWeaponHUDManager)
    {
        m_pVRWeaponHUDManager->Shutdown();
        delete m_pVRWeaponHUDManager;
        m_pVRWeaponHUDManager = nullptr;
        g_pVRWeaponHUDManager = nullptr;
    }
    
    // Shutdown VR Spring HUD Manager
    if (m_pVRSpringHUDManager)
    {
        m_pVRSpringHUDManager->Shutdown();
        delete m_pVRSpringHUDManager;
        m_pVRSpringHUDManager = nullptr;
        g_pVRSpringHUDManager = nullptr;
    }
    
    // Shutdown VR Damage Indicator Manager
    if (m_pVRDamageIndicatorManager)
    {
        m_pVRDamageIndicatorManager->Shutdown();
        delete m_pVRDamageIndicatorManager;
        m_pVRDamageIndicatorManager = nullptr;
        g_pVRDamageIndicatorManager = nullptr;
    }
    
    // Shutdown VR Weapon Select Manager
    if (m_pVRWeaponSelectManager)
    {
        m_pVRWeaponSelectManager->Shutdown();
        delete m_pVRWeaponSelectManager;
        m_pVRWeaponSelectManager = nullptr;
        g_pVRWeaponSelectManager = nullptr;
    }
    
    // Shutdown VR Popup HUD Manager
    if (m_pVRPopupHUDManager)
    {
        m_pVRPopupHUDManager->Shutdown();
        delete m_pVRPopupHUDManager;
        m_pVRPopupHUDManager = nullptr;
        g_pVRPopupHUDManager = nullptr;
    }
    
    // Shutdown VR World Health Icon Manager
    if (m_pVRWorldHealthIconManager)
    {
        m_pVRWorldHealthIconManager->Shutdown();
        delete m_pVRWorldHealthIconManager;
        m_pVRWorldHealthIconManager = nullptr;
        g_pVRWorldHealthIconManager = nullptr;
    }
    
    // Shutdown VR Damage Number Manager
    if (m_pVRDamageNumberManager)
    {
        m_pVRDamageNumberManager->Shutdown();
        delete m_pVRDamageNumberManager;
        m_pVRDamageNumberManager = nullptr;
        g_pVRDamageNumberManager = nullptr;
    }
    
    // Shutdown VR Spectator Extras Manager
    if (m_pVRSpectatorExtrasManager)
    {
        m_pVRSpectatorExtrasManager->Shutdown();
        delete m_pVRSpectatorExtrasManager;
        m_pVRSpectatorExtrasManager = nullptr;
        g_pVRSpectatorExtrasManager = nullptr;
    }
    
    // Shutdown VR Collision Warning
    if (m_pVRCollisionWarningManager)
    {
        m_pVRCollisionWarningManager->Shutdown();
        delete m_pVRCollisionWarningManager;
        m_pVRCollisionWarningManager = nullptr;
        g_pVRCollisionWarningManager = nullptr;
    }
    
    // Shutdown VR Spectator Camera
    if (m_pVRSpectatorCamera)
    {
        m_pVRSpectatorCamera->Shutdown();
        delete m_pVRSpectatorCamera;
        m_pVRSpectatorCamera = nullptr;
        g_pVRSpectatorCamera = nullptr;
    }
    
    // Shutdown VR Controller Model Manager
    if (g_pVRControllerModelManager)
    {
        g_pVRControllerModelManager->Shutdown();
        delete g_pVRControllerModelManager;
        g_pVRControllerModelManager = nullptr;
    }
    
    // Shutdown VR World UI Queue last (used by all other managers)
    if (g_pVRWorldUIQueue)
    {
        g_pVRWorldUIQueue->Shutdown();
        delete g_pVRWorldUIQueue;
        g_pVRWorldUIQueue = nullptr;
    }
    
    m_pVRManager = nullptr;
    m_pLocalPlayer = nullptr;
    // VR Menu Manager shutdown
}

void CVRMenuManager::Update()
{
    VPROF("CVRMenuManager::Update");
    
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
        return;

    m_pVRManager = g_pOpenXRManager;
    m_pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    
    // Update VR Controller Model Manager
    if (g_pVRControllerModelManager)
    {
        g_pVRControllerModelManager->Update(gpGlobals->frametime);
    }
    
    // Debug output during state checks
    static float s_flLastDebugTime = 0.0f;
    static SourceEngineState s_lastState = SOURCE_STATE_GAMEPLAY;
    SourceEngineState currentState = DetermineSourceState();
    
    if (gpGlobals->curtime > s_flLastDebugTime + 1.0f || currentState != s_lastState)  // Every second or state change
    {
        s_flLastDebugTime = gpGlobals->curtime;
        
        // Only call dxvkSetSourceState when state actually changes (not every frame!)
        if (currentState != s_lastState) {
            dxvkSetSourceState(currentState);
        }
        
        s_lastState = currentState;
    }
    
    // Handle VR rendering based on compositor state
    
    if (dxvkIsCompositorActive())
    {
        // Compositor is handling VR frames - we just submit content
        HandleCompositorMode(currentState);
    }
    else
    {
        // Traditional Source VR pipeline (gameplay)
        HandleTraditionalVRMode(currentState);
    }
    
    if (m_pLocalPlayer)
    {
        // Save the current view origin for menu input
        // Use GetVRViewPosition() which returns the death position if in VR death mode
        m_savedPlayerViewOrigin = m_pLocalPlayer->GetVRViewPosition();
    }
    
    // Update VR Status HUD Manager
    if (m_pVRStatusHUDManager)
    {
        m_pVRStatusHUDManager->Update();
    }
    
    // Update VR Weapon HUD Manager
    if (m_pVRWeaponHUDManager)
    {
        m_pVRWeaponHUDManager->Update();
    }
    
    // Update VR Spring HUD Manager (kill feed)
    if (m_pVRSpringHUDManager)
    {
        m_pVRSpringHUDManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Damage Indicator Manager (damage direction)
    if (m_pVRDamageIndicatorManager)
    {
        m_pVRDamageIndicatorManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Weapon Select Manager (radial weapon selection)
    if (m_pVRWeaponSelectManager)
    {
        m_pVRWeaponSelectManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Popup HUD Manager (win/loss panels, scoreboard)
    if (m_pVRPopupHUDManager)
    {
        m_pVRPopupHUDManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Collision Warning (floating warning text)
    if (m_pVRCollisionWarningManager)
    {
        m_pVRCollisionWarningManager->Update(gpGlobals->frametime);
    }
    
    // Update VR World Health Icon Manager
    if (m_pVRWorldHealthIconManager)
    {
        m_pVRWorldHealthIconManager->Update();
    }
    
    // Update VR Damage Number Manager (world-space damage numbers)
    if (m_pVRDamageNumberManager)
    {
        m_pVRDamageNumberManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Spectator Extras Manager (world-space player names/health)
    if (m_pVRSpectatorExtrasManager)
    {
        m_pVRSpectatorExtrasManager->Update(gpGlobals->frametime);
    }
    
    // Update VR Hands
    // TODO: This is here to ensure update is late enough to ensure correct hand 
    // tracking data is available for the hands. Should maybe be moved to a more appropriate place.
    extern void UpdateVRHands();
    UpdateVRHands();
    
    // NOTE: Menu input handling moved to OverridePlayerMotion for fresh positioning data
    // We still handle menu input here for non-positional aspects
    HandleMenuInput();
}

// Global loading state - set by VGui_PreRender() every frame with authoritative loading state
static bool g_bIsGloballyLoading = false;

void TF2VR_SetLoadingState(bool isLoading)
{
    static bool s_lastLoadingState = false;
    
    g_bIsGloballyLoading = isLoading;
    
    // Only update when state actually changes
    if (isLoading != s_lastLoadingState) {
        s_lastLoadingState = isLoading;
        
        // Notify DXVK when loading state changes
        if (g_pVRMenuManager) {
            SourceEngineState newState = g_pVRMenuManager->DetermineSourceState();
            dxvkSetSourceState(newState);
        }
    }
}

// TF2VR: Simple loading detection
void TF2VR_CheckEarlyLoadingState()
{
    static bool s_lastLoadingState = false;
    static int s_checkCount = 0;
    s_checkCount++;
    
    // Combine multiple detection methods for the most reliable loading state
    bool bEngineDrawingLoading = engine && engine->IsDrawingLoadingImage();

    // Primary detection: engine->IsDrawingLoadingImage() is the most authoritative
    bool bIsLoading = bEngineDrawingLoading;
    
    // Update global loading state if it changed
    if (bIsLoading != s_lastLoadingState) {
        TF2VR_SetLoadingState(bIsLoading);
        s_lastLoadingState = bIsLoading;
    }
}

SourceEngineState CVRMenuManager::DetermineSourceState()
{
    // Use the authoritative global loading state
    bool bInMenu = (!engine->IsInGame() && !engine->IsConnected() && !m_pLocalPlayer);
    
    if (g_bIsGloballyLoading)
    {
        return SOURCE_STATE_LOADING;
    }
    else if (bInMenu)
    {
        return SOURCE_STATE_MENU;
    }
    else
    {
        return SOURCE_STATE_GAMEPLAY;
    }
}

void CVRMenuManager::HandleCompositorMode(SourceEngineState state)
{
    // Compositor is handling VR frames - we just submit content when needed
    switch (state)
    {
        case SOURCE_STATE_MENU:
            SubmitMenuFrameToCompositor();
            break;
            
        case SOURCE_STATE_LOADING:
            SubmitLoadingFrameToCompositor();
            break;
            
        default:
            // Compositor shouldn't be active in other states
            break;
    }
}

void CVRMenuManager::HandleTraditionalVRMode(SourceEngineState state)
{
    // Traditional Source VR pipeline - handle BeginFrame/EndFrame ourselves
    switch (state)
    {
        case SOURCE_STATE_MENU:
            RenderMenuOnlyMode();
            break;
            
        case SOURCE_STATE_LOADING:
            RenderLoadingScreenMode();
            break;
            
        case SOURCE_STATE_GAMEPLAY:
            // Normal gameplay - VR handled by main pipeline
            break;
            
        default:
            break;
    }
}

void CVRMenuManager::SubmitMenuFrameToCompositor()
{
    // Render VGUI to our local texture
    RenderVGUIToTexture();
    
    // Submit to DXVK compositor (non-blocking)
    ITexture *pVGUITexture = materials->FindTexture("_rt_vgui", NULL, false);
    if (pVGUITexture)
    {
        // Get texture handle and submit to compositor
        // Note: This would need proper texture handle extraction
        dxvkSubmitMenuFrame(pVGUITexture, pVGUITexture->GetActualWidth(), pVGUITexture->GetActualHeight());
    }
}

void CVRMenuManager::SubmitLoadingFrameToCompositor()
{
    // Similar to menu but for loading screens
    RenderVGUIToTexture();  // This includes loading disc
    
    ITexture *pVGUITexture = materials->FindTexture("_rt_vgui", NULL, false);
    if (pVGUITexture)
    {
        dxvkSubmitMenuFrame(pVGUITexture, pVGUITexture->GetActualWidth(), pVGUITexture->GetActualHeight());
    }
}

void CVRMenuManager::HandleMenuInput()
{
    bool menuVisible = IsMenuVisible();
    
    // Check for class/team menu button (left A button)
    // Quick press = class menu, long hold = team menu
    static bool bLastClassMenuButtonState = false;
    bool bCurrentClassMenuButtonState = m_pVRManager && m_pVRManager->IsButtonPressed("left_class_menu");
    float currentTime = gpGlobals->curtime;
    float holdThreshold = tfvr_class_menu_hold_threshold.GetFloat();
    
    // Detect if time has gone backwards (map change, etc.) and reset cooldown
    if (currentTime < m_flLastClassMenuTime)
    {
        DevMsg("VR Menu: Time went backwards (%.1f -> %.1f), resetting class menu cooldown\n", 
               m_flLastClassMenuTime, currentTime);
        m_flLastClassMenuTime = 0.0f;
        m_flClassMenuButtonPressTime = 0.0f;
    }
    
    // On button press: record timestamp
    if (bCurrentClassMenuButtonState && !bLastClassMenuButtonState)
    {
        m_flClassMenuButtonPressTime = currentTime;
        m_bClassMenuHoldActionExecuted = false;
    }
    
    // While held: check for long hold threshold (team menu)
    if (bCurrentClassMenuButtonState && !m_bClassMenuHoldActionExecuted)
    {
        float holdTime = currentTime - m_flClassMenuButtonPressTime;
        if (holdTime >= holdThreshold)
        {
            // Long hold - open team menu
            if (currentTime - m_flLastClassMenuTime >= 0.5f)
            {
                if (engine && engine->IsInGame())
                {
                    // Check if team menu is already open
                    IViewPortPanel* pTeamPanel = gViewPortInterface ? gViewPortInterface->FindPanelByName(PANEL_TEAM) : nullptr;
                    bool bTeamMenuOpen = pTeamPanel && pTeamPanel->IsVisible();
                    
                    if (bTeamMenuOpen)
                    {
                        engine->ClientCmd("escape");
                        DevMsg("VR Menu: Team menu closed via left A button hold (escape command)\n");
                    }
                    else
                    {
                        engine->ClientCmd("changeteam");
                        DevMsg("VR Menu: Team menu opened via left A button hold (changeteam command)\n");
                    }
                    
                    m_flLastClassMenuTime = currentTime;
                }
            }
            m_bClassMenuHoldActionExecuted = true;
        }
    }
    
    // On button release: if hold action wasn't executed, it was a quick press (class menu)
    if (!bCurrentClassMenuButtonState && bLastClassMenuButtonState && !m_bClassMenuHoldActionExecuted)
    {
        // Quick press - open class menu
        if (currentTime - m_flLastClassMenuTime >= 0.5f)
        {
            if (engine && engine->IsInGame())
            {
                // Check if class menu is already open
                bool bClassMenuOpen = false;
                ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
                if (pClassMenuOpen && pClassMenuOpen->GetBool())
                {
                    bClassMenuOpen = true;
                }
                
                if (bClassMenuOpen)
                {
                    engine->ClientCmd("escape");
                    DevMsg("VR Menu: Class menu closed via left A button quick press (escape command)\n");
                }
                else
                {
                    engine->ClientCmd("changeclass");
                    DevMsg("VR Menu: Class menu opened via left A button quick press (changeclass command)\n");
                }
                
                m_flLastClassMenuTime = currentTime;
            }
        }
        else
        {
            DevMsg("VR Menu: Class/team menu command blocked - too soon since last execution (%.1f seconds)\n", 
                   currentTime - m_flLastClassMenuTime);
        }
    }
    
    bLastClassMenuButtonState = bCurrentClassMenuButtonState;
    
    // Check if player has moved significantly (e.g., changed maps)
    if (m_pLocalPlayer && m_bMenuPositionFixed)
    {
        Vector currentPos = m_pLocalPlayer->GetVRViewPosition();
        float distanceMoved = (currentPos - m_fixedMenuPosition).Length();
        
        // If player moved more than 1000 units, they probably changed maps
        if (distanceMoved > 1000.0f)
        {
            DevMsg("VR Menu: Player moved %.1f units, resetting menu position\n", distanceMoved);
            m_bMenuPositionFixed = false;
            // Clear the old HUD bounds
            g_ClientVirtualReality.ClearCustomHUDBounds();
        }
    }
    
    // Check for map changes
    if (engine && engine->IsInGame())
    {
        const char* currentMapName = engine->GetLevelName();
        if (currentMapName && strcmp(currentMapName, m_szLastMapName) != 0)
        {
            Msg(_T("VR Menu: Map changed from '%s' to '%s', resetting menu position and VR HUD overlays\n"), 
                   m_szLastMapName, currentMapName);
            m_bMenuPositionFixed = false;
            Q_strncpy(m_szLastMapName, currentMapName, sizeof(m_szLastMapName));
            // Clear the old HUD bounds
            g_ClientVirtualReality.ClearCustomHUDBounds();
            
            // IMPORTANT: Reset VR HUD managers to clear stuck data from previous map
            if (m_pVRStatusHUDManager)
            {
                m_pVRStatusHUDManager->ResetState();
            }
            if (m_pVRWeaponHUDManager)
            {
                m_pVRWeaponHUDManager->ResetState();
            }
            if (m_pVRSpringHUDManager)
            {
                m_pVRSpringHUDManager->ResetState();
            }
            // Reset spectator camera on map change to avoid lingering smoothing state
            if (m_pVRSpectatorCamera)
            {
                m_pVRSpectatorCamera->Reset();
            }
        }
    }
    
    // If menu just became visible, capture the fixed position
    if (menuVisible && !m_bMenuPositionFixed)
    {
        if (m_pLocalPlayer)
        {
                         // Wait for player to have a valid rotation (not zero angles)
             QAngle currentAngles = m_pLocalPlayer->EyeAngles();
             if (currentAngles.LengthSqr() < 0.1f)
             {
                 // Player rotation not ready yet, wait for next frame
                 return;
             }
            
                         // Check if this is a natural menu opening (not button-triggered)
             // Only run HMD stabilization check for natural openings like startup/MOTD
             static bool bFirstPositioning = true;
             static Vector lastValidPosition = Vector(0, 0, 0);
             static QAngle lastValidRotation = QAngle(0, 0, 0);
             static float flRotationStableTime = 0.0f;
             Vector currentPlayerPos = m_pLocalPlayer->GetVRViewPosition();
             
             // If this is the first time or player moved significantly from last valid position
             if (bFirstPositioning || (lastValidPosition != Vector(0, 0, 0) && 
                 (currentPlayerPos - lastValidPosition).Length() > 100.0f))
             {
                 bFirstPositioning = false;
                 lastValidPosition = currentPlayerPos;
                 lastValidRotation = currentAngles;
                 flRotationStableTime = gpGlobals->curtime;
                 
                 // Don't set menu position yet - wait for rotation to stabilize
                 return;
             }
             
                         // Playspace anchoring provides stability, so immediately set menu position
            // Use the player's current position and view angles for consistent placement
            m_fixedMenuPosition = currentPlayerPos;
            m_fixedMenuRotation = currentAngles;
            m_fixedMenuRotation.x = 0; // Keep level
            m_fixedMenuRotation.z = 0; // No roll
            
            m_bMenuPositionFixed = true;
            
            // Reset mouse priority tracking to prevent false detection from cursor position changes
            ResetMousePriorityTracking();
            
            // Check if we should use playspace anchoring
            m_bUsePlayspaceAnchoring = tfvr_playspace_anchoring.GetBool();
            
            if (m_bUsePlayspaceAnchoring && m_pVRManager)
            {
                // Calculate and store the menu's absolute position in playspace coordinates
                CalculatePlayspaceAnchor();
                
                // For playspace-anchored menus, custom bounds are set in UpdatePlayspaceAnchoredPosition()
                // So we need to call that first, then notify the compositor
                UpdatePlayspaceAnchoredPosition();
                
                // Now trigger compositor HUD update with the custom bounds that were just set
                extern void NotifyCompositorPlayspaceUpdate();
                NotifyCompositorPlayspaceUpdate();
            }
            
            // For ViewPort menus, we need to make the cursor visible
            if (vgui::surface())
            {
                vgui::surface()->SetCursorAlwaysVisible(true);
            }
            
            // Set custom HUD bounds ONCE when menu opens (only if not using playspace anchoring)
            if (!m_bUsePlayspaceAnchoring)
            {
                float menuDistance = tfvr_menu_distance.GetFloat();
                Vector forward, right, up;
                AngleVectors(m_fixedMenuRotation, &forward, &right, &up);
                
                // Place the menu directly in front of the player at the specified distance
                Vector menuPlaneCenter = m_fixedMenuPosition + forward * menuDistance;
                
                // Create a quad representing the menu plane
                // Use scaled size based on tfvr_menu_scale
                float menuHeight, menuWidth;
                GetScaledMenuDimensions(menuWidth, menuHeight);
                
                Vector ul = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (menuHeight * 0.5f);
                Vector ur = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (menuHeight * 0.5f);
                Vector ll = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
                Vector lr = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
                
                // Set the custom HUD bounds in the VR system
                Vector currentHeadWorldPos = m_pLocalPlayer->GetVRViewPosition();
                g_ClientVirtualReality.SetCustomHUDBounds(currentHeadWorldPos, ul, ur, ll, lr);
                
                // Trigger compositor HUD update for non-playspace-anchored menus too
                extern void NotifyCompositorPlayspaceUpdate();
                NotifyCompositorPlayspaceUpdate();
            }
         }
         else
         {
             // TF2VR: Main menu case - no local player available
             // Use a simple fixed position in front of the headset
             if (m_pVRManager)
             {
                 // Get HMD pose for menu positioning
                 Vector hmdOrigin;
                 QAngle hmdAngles;
                 m_pVRManager->GetHMDInChaperone(hmdOrigin, hmdAngles);
                 
                 // Use HMD position and orientation for menu
                 m_fixedMenuPosition = hmdOrigin;
                 m_fixedMenuRotation = hmdAngles;
                     
                     // Keep level (no pitch/roll)
                     m_fixedMenuRotation.x = 0;
                     m_fixedMenuRotation.z = 0;
                     
                     m_bMenuPositionFixed = true;
                     m_bUsePlayspaceAnchoring = tfvr_playspace_anchoring.GetBool();
                     
                     // Reset mouse priority tracking to prevent false detection from cursor position changes
                     ResetMousePriorityTracking();
                     
                     DevMsg("VR Menu: Fixed menu position for main menu (no player) at HMD pose\n");
                     
                     // Trigger compositor HUD update for main menu positioning
                     extern void NotifyCompositorPlayspaceUpdate();
                     NotifyCompositorPlayspaceUpdate();
                     
                     // Make cursor visible for main menu
                     if (vgui::surface())
                     {
                         vgui::surface()->SetCursorAlwaysVisible(true);
                     }
                 }
            }
        }
     
     // If menu just became hidden, reset the fixed position
    else if (!menuVisible && m_bMenuPositionFixed)
    {
        m_bMenuPositionFixed = false;
        
                 // Hide the cursor when menu closes
         if (vgui::surface())
         {
             vgui::surface()->SetCursorAlwaysVisible(false);
         }
         
         // Clear the custom HUD bounds to restore normal VR system behavior
         g_ClientVirtualReality.ClearCustomHUDBounds();
        
    }
    
    // Always handle menu button input and cursor position when VR manager is available
    if (m_pVRManager)
    {
        // Handle menu button input
        HandleMenuButtonInput();
        
        // Update cursor position
        UpdateCursorPosition();
    }

    // Additional menu input handling that requires player (for in-game menus)
    if (!menuVisible || m_savedPlayerViewOrigin == Vector(0, 0, 0))
    {
        return;
    }

    if (!m_pLocalPlayer || !m_pVRManager)
        return;

    // Additional player-dependent menu logic can go here if needed
}

bool CVRMenuManager::IsMenuPanelOpen()
{
    bool bGameUIVisible = enginevgui && enginevgui->IsGameUIVisible();
    if (bGameUIVisible)
        return true;

    if (gViewPortInterface)
    {
        IViewPortPanel* pPanel = nullptr;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_CLASS_RED);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_CLASS_BLUE);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_TEAM);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_INTRO);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_INFO);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_MAPINFO);
        if (pPanel && pPanel->IsVisible()) return true;
        pPanel = gViewPortInterface->FindPanelByName(PANEL_ARENA_TEAM);
        if (pPanel && pPanel->IsVisible()) return true;
    }

    if (g_pClientMode)
    {
        CHudMenu* pHudMenu = GET_HUDELEMENT(CHudMenu);
        if (pHudMenu && pHudMenu->IsMenuOpen())
            return true;
    }

    if (engine && engine->IsInGame())
    {
        ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
        if (pClassMenuOpen && pClassMenuOpen->GetBool())
            return true;
    }

    return false;
}

bool CVRMenuManager::IsMenuVisible()
{
    // Check multiple conditions to be more robust
    bool bGameUIVisible = enginevgui && enginevgui->IsGameUIVisible();
    bool bCursorVisible = vgui::surface() && vgui::surface()->IsCursorVisible();
    bool bNotInGame = engine && (!engine->IsInGame() || !engine->IsConnected());
    
             // Check for ViewPort panels (class select, team select, intro, MOTD, etc.)
         bool bViewPortVisible = false;
         if (gViewPortInterface)
         {
             // Check common TF2 ViewPort panels by finding them and checking visibility
             IViewPortPanel* pPanel = nullptr;
             
             // Check class selection panels
             pPanel = gViewPortInterface->FindPanelByName(PANEL_CLASS_RED);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             pPanel = gViewPortInterface->FindPanelByName(PANEL_CLASS_BLUE);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             // Check team selection panel
             pPanel = gViewPortInterface->FindPanelByName(PANEL_TEAM);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             // Check intro panel
             pPanel = gViewPortInterface->FindPanelByName(PANEL_INTRO);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             // Check info panel (MOTD, etc.)
             pPanel = gViewPortInterface->FindPanelByName(PANEL_INFO);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             // Check map info panel
             pPanel = gViewPortInterface->FindPanelByName(PANEL_MAPINFO);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
             
             // Check arena team panel
             pPanel = gViewPortInterface->FindPanelByName(PANEL_ARENA_TEAM);
             if (pPanel && pPanel->IsVisible()) 
             {
                 bViewPortVisible = true;
             }
         }
    
         // Check for HUD menus (voice commands, etc.)
     bool bHudMenuVisible = false;
     if (g_pClientMode)
     {
         CHudMenu* pHudMenu = GET_HUDELEMENT(CHudMenu);
         if (pHudMenu && pHudMenu->IsMenuOpen())
         {
             bHudMenuVisible = true;
         }
     }
    
         // Check for class menu state via console variables
     bool bClassMenuOpen = false;
     if (engine && engine->IsInGame())
     {
         ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
         if (pClassMenuOpen && pClassMenuOpen->GetBool())
         {
             bClassMenuOpen = true;
         }
     }
    
    // Check for TF2-specific menu states via ConVars
    bool bTF2MenuState = false;
    if (engine && engine->IsInGame())
    {
                 // Check for team UI setup state
         ConVar* pTeamUI = g_pCVar->FindVar("team_ui_setup");
         if (pTeamUI && pTeamUI->GetBool())
         {
             bTF2MenuState = true;
         }
         
         // Check player state for menu indicators
         C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
         if (pLocalPlayer)
         {
             // If dead and not spectating, treat as menu visible to block VR input
             // This prevents movement/tracking issues during death cam.
             // Note: The menu button is handled separately in vr_input.cpp to still
             // allow opening the escape menu while dead.
             if (pLocalPlayer->IsPlayerDead() && pLocalPlayer->GetTeamNumber() != TEAM_SPECTATOR)
             {
                 bTF2MenuState = true;
             }
             
             // Check for team assignment state
             if (pLocalPlayer->GetTeamNumber() == TEAM_UNASSIGNED)
             {
                 bTF2MenuState = true;
             }
             
             // Check for undefined class state (shows class selection)
             if (pLocalPlayer->GetPlayerClass() && 
                 pLocalPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED)
             {
                 bTF2MenuState = true;
             }
         }
    }
    
        // Menu is considered visible if any of these conditions are true
    return bGameUIVisible || bViewPortVisible || bHudMenuVisible || bClassMenuOpen || bTF2MenuState;
    
    
}

void CVRMenuManager::HandleMenuButtonInput()
{
    // Check for menu press on both hands using trigger threshold
    bool leftMenuPress = m_pVRManager->IsUIInteractionPressed("left_ui_interact");
    bool rightMenuPress = m_pVRManager->IsUIInteractionPressed("right_ui_interact");
    
    // Determine which hand is being used
    if (leftMenuPress && !m_bMenuButtonPressed)
    {
        m_nMenuHand = 0; // Left hand
        m_bMenuButtonPressed = true;
        
        // VR trigger press is an explicit signal to use VR - force VR priority
        m_bMouseHasPriority = false;
        m_flRecentMouseMovement = 0.0f;
        m_flVRReclaimCooldown = tfvr_cursor_vr_reclaim_cooldown.GetFloat();
        
        // Update compositor laser to match active hand
        dxvkSetLaserActiveHand(true);  // Left hand
        
        // For ViewPort menus, we need to simulate a mouse click at the current cursor position
        if (vgui::surface() && vgui::surface()->IsCursorVisible())
        {
            // Get current cursor position
            int cursorX, cursorY;
            vgui::surface()->SurfaceGetCursorPos(cursorX, cursorY);
            
            // Simulate mouse press at cursor position
            if (g_InputInternal)
            {
                g_InputInternal->InternalCursorMoved(cursorX, cursorY);
                g_InputInternal->SetMouseCodeState(MOUSE_LEFT, vgui::BUTTON_PRESSED);
                g_InputInternal->InternalMousePressed(MOUSE_LEFT);
            }
        }
        else
        {
            // Fallback to VGUI input system
            if (g_InputInternal)
            {
                g_InputInternal->SetMouseCodeState(MOUSE_LEFT, vgui::BUTTON_PRESSED);
                g_InputInternal->InternalMousePressed(MOUSE_LEFT);
            }
        }
    }
    else if (rightMenuPress && !m_bMenuButtonPressed)
    {
        m_nMenuHand = 1; // Right hand
        m_bMenuButtonPressed = true;
        
        // VR trigger press is an explicit signal to use VR - force VR priority
        m_bMouseHasPriority = false;
        m_flRecentMouseMovement = 0.0f;
        m_flVRReclaimCooldown = tfvr_cursor_vr_reclaim_cooldown.GetFloat();
        
        // Update compositor laser to match active hand
        dxvkSetLaserActiveHand(false);  // Right hand
        
        // For ViewPort menus, we need to simulate a mouse click at the current cursor position
        if (vgui::surface() && vgui::surface()->IsCursorVisible())
        {
            // Get current cursor position
            int cursorX, cursorY;
            vgui::surface()->SurfaceGetCursorPos(cursorX, cursorY);
            
            // Simulate mouse press at cursor position
            if (g_InputInternal)
            {
                g_InputInternal->InternalCursorMoved(cursorX, cursorY);
                g_InputInternal->SetMouseCodeState(MOUSE_LEFT, vgui::BUTTON_PRESSED);
                g_InputInternal->InternalMousePressed(MOUSE_LEFT);
            }
        }
        else
        {
            // Fallback to VGUI input system
            if (g_InputInternal)
            {
                g_InputInternal->SetMouseCodeState(MOUSE_LEFT, vgui::BUTTON_PRESSED);
                g_InputInternal->InternalMousePressed(MOUSE_LEFT);
            }
        }
    }
    
    // Handle button release
    if (!leftMenuPress && !rightMenuPress && m_bMenuButtonPressed)
    {
        m_bMenuButtonPressed = false;
        
        if (g_InputInternal)
        {
            g_InputInternal->SetMouseCodeState(MOUSE_LEFT, vgui::BUTTON_RELEASED);
            g_InputInternal->InternalMouseReleased(MOUSE_LEFT);
        }
    }
}

void CVRMenuManager::ResetMousePriorityTracking()
{
    // Reset input priority tracking state
    // Called when menus open to start fresh
    m_nLastMouseCursorX = -1;
    m_nLastMouseCursorY = -1;
    m_vecLastControllerPos.Init(0, 0, 0);
    m_vecControllerAnchor.Init(0, 0, 0);
    m_flRecentMouseMovement = 0.0f;
    m_flRecentVRMovement = 0.0f;
    m_bMouseHasPriority = false;
    m_flVRReclaimCooldown = 0.0f;
    
    if (tfvr_cursor_debug.GetBool())
    {
        DevMsg("VR Cursor: Reset input priority tracking (menu opened)\n");
    }
}

void CVRMenuManager::UpdateCursorPosition()
{
    // TF2VR: Allow cursor positioning without a local player (main menu case)
    if (!m_bMenuPositionFixed)
    {
        return;
    }
    
    // Update playspace anchored position each frame if enabled
    if (m_bUsePlayspaceAnchoring)
    {
        UpdatePlayspaceAnchoredPosition();
    }

    // Mouse priority mode check
    int mousePriorityMode = tfvr_cursor_mouse_priority.GetInt();
    
    // Mode 2: Mouse only - completely disable VR cursor positioning
    if (mousePriorityMode == 2)
    {
        return;
    }
    
    // Mode 1: Auto priority - compare mouse vs VR movement, more active input wins
    if (mousePriorityMode == 1)
    {
        float smoothing = clamp(tfvr_cursor_smoothing.GetFloat(), 0.0f, 0.99f);
        float vrSensitivity = tfvr_cursor_vr_sensitivity.GetFloat();
        
        // Measure mouse movement this frame
        float mouseMovementThisFrame = 0.0f;
        int currentCursorX = 0, currentCursorY = 0;
        if (vgui::surface())
        {
            vgui::surface()->SurfaceGetCursorPos(currentCursorX, currentCursorY);
        }
        
        if (!m_bVRCursorUpdateInProgress && m_nLastMouseCursorX >= 0 && m_nLastMouseCursorY >= 0)
        {
            int deltaX = abs(currentCursorX - m_nLastMouseCursorX);
            int deltaY = abs(currentCursorY - m_nLastMouseCursorY);
            if (deltaX > 1 || deltaY > 1)  // Filter tiny noise
            {
                mouseMovementThisFrame = sqrtf((float)(deltaX * deltaX + deltaY * deltaY));
            }
        }
        
        // Update mouse position tracking
        m_nLastMouseCursorX = currentCursorX;
        m_nLastMouseCursorY = currentCursorY;
        
        // Measure VR controller movement this frame
        float vrMovementThisFrame = 0.0f;
        Vector currentControllerPos(0, 0, 0);
        bool controllerValid = false;
        
        if (m_nMenuHand == 0 && g_pOpenXRManager && g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            VMatrix controllerMatrix;
            bool poseValid = dxvkIsCompositorActive() ? 
                g_pOpenXRManager->GetLeftControllerPoseRaw(controllerMatrix) :
                g_pOpenXRManager->GetLeftControllerPose(controllerMatrix);
            if (poseValid)
            {
                currentControllerPos = controllerMatrix.GetTranslation();
                controllerValid = true;
            }
        }
        else if (m_nMenuHand == 1 && g_pOpenXRManager && g_pOpenXRManager->IsRightControllerPoseValid())
        {
            VMatrix controllerMatrix;
            bool poseValid = dxvkIsCompositorActive() ? 
                g_pOpenXRManager->GetRightControllerPoseRaw(controllerMatrix) :
                g_pOpenXRManager->GetRightControllerPose(controllerMatrix);
            if (poseValid)
            {
                currentControllerPos = controllerMatrix.GetTranslation();
                controllerValid = true;
            }
        }
        
        if (controllerValid && m_vecLastControllerPos.LengthSqr() > 0.0f)
        {
            vrMovementThisFrame = (currentControllerPos - m_vecLastControllerPos).Length();
        }
        m_vecLastControllerPos = currentControllerPos;
        
        // Scale VR movement to be comparable to mouse pixels
        float scaledVRMovement = vrMovementThisFrame * vrSensitivity;
        
        // Update rolling averages with exponential smoothing
        m_flRecentMouseMovement = (m_flRecentMouseMovement * smoothing) + (mouseMovementThisFrame * (1.0f - smoothing));
        m_flRecentVRMovement = (m_flRecentVRMovement * smoothing) + (scaledVRMovement * (1.0f - smoothing));
        
        // Priority logic with dead zone for VR
        float deadZoneRadius = tfvr_cursor_vr_deadzone.GetFloat();
        
        // Tick down VR reclaim cooldown
        if (m_flVRReclaimCooldown > 0.0f)
        {
            m_flVRReclaimCooldown -= gpGlobals->frametime;
        }
        
        if (m_bMouseHasPriority)
        {
            // Mouse currently has priority
            // VR can only take back if controller moves OUTSIDE the dead zone from anchor
            if (controllerValid && m_vecControllerAnchor.LengthSqr() > 0.0f)
            {
                float distanceFromAnchor = (currentControllerPos - m_vecControllerAnchor).Length();
                
                if (distanceFromAnchor > deadZoneRadius)
                {
                    // Controller moved outside dead zone - VR takes priority
                    m_bMouseHasPriority = false;
                    m_flRecentMouseMovement = 0.0f;
                    m_flVRReclaimCooldown = tfvr_cursor_vr_reclaim_cooldown.GetFloat();
                    if (tfvr_cursor_debug.GetBool())
                    {
                        DevMsg("VR Cursor: VR takes priority (controller moved %.3f outside %.3f dead zone, cooldown %.1fs)\n",
                               distanceFromAnchor, deadZoneRadius, m_flVRReclaimCooldown);
                    }
                }
            }
        }
        else
        {
            // VR currently has priority
            // Mouse can take over if it's more active than VR, but not during reclaim cooldown
            if (m_flVRReclaimCooldown <= 0.0f &&
                m_flRecentMouseMovement > m_flRecentVRMovement && m_flRecentMouseMovement > 1.0f)
            {
                // Mouse is more active - takes priority
                m_bMouseHasPriority = true;
                
                // Save controller position as anchor for dead zone
                if (controllerValid)
                {
                    m_vecControllerAnchor = currentControllerPos;
                }
                
                if (tfvr_cursor_debug.GetBool())
                {
                    DevMsg("VR Cursor: Mouse takes priority (mouse=%.1f > VR=%.1f), anchor set\n",
                           m_flRecentMouseMovement, m_flRecentVRMovement);
                }
            }
        }
        
        // If mouse has priority, skip VR cursor update
        if (m_bMouseHasPriority)
        {
            return;
        }
    }

    // Get current VR controller position for threshold checking
    Vector currentControllerPos;
    bool controllerValid = false;
    
    if (m_nMenuHand == 0) // Left hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            VMatrix controllerMatrix;
            // Use raw poses for compositor mode, world poses for in-game
            bool poseValid = dxvkIsCompositorActive() ? 
                g_pOpenXRManager->GetLeftControllerPoseRaw(controllerMatrix) :
                g_pOpenXRManager->GetLeftControllerPose(controllerMatrix);
            
            if (poseValid)
            {
                currentControllerPos = controllerMatrix.GetTranslation();
                controllerValid = true;
            }
        }
    }
    else // Right hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsRightControllerPoseValid())
        {
            VMatrix controllerMatrix;
            // Use raw poses for compositor mode, world poses for in-game
            bool poseValid = dxvkIsCompositorActive() ? 
                g_pOpenXRManager->GetRightControllerPoseRaw(controllerMatrix) :
                g_pOpenXRManager->GetRightControllerPose(controllerMatrix);
            
            if (poseValid)
            {
                currentControllerPos = controllerMatrix.GetTranslation();
                controllerValid = true;
            }
        }
    }
    
    // Simple threshold check: only update cursor if VR movement is significant
    if (controllerValid)
    {
        static Vector lastControllerPos = currentControllerPos;
        float movementDistance = (currentControllerPos - lastControllerPos).Length();
        
        // If movement is too small, don't update cursor
        if (movementDistance < tfvr_cursor_threshold.GetFloat())
        {
            return;
        }
        
        lastControllerPos = currentControllerPos;
    }
    else
    {
        // No controller available - for main menu case, we still want to allow cursor updates
        // Skip movement threshold when no controller is available (main menu scenario)
        // Use OpenXR HMD pose directly instead of player entity
        if (g_pOpenXRManager)
        {
            Vector currentHeadPos;
            QAngle currentHeadAngles;
            g_pOpenXRManager->GetHMDInChaperone(currentHeadPos, currentHeadAngles);
            
            static Vector lastHeadPos = currentHeadPos;
            float headMovementDistance = (currentHeadPos - lastHeadPos).Length();
            
            // Use reduced threshold for compositor mode for more responsive cursor
            float currentHeadThreshold = dxvkIsCompositorActive() ? 
                (tfvr_cursor_head_threshold.GetFloat() * 0.1f) : tfvr_cursor_head_threshold.GetFloat();
            
            // If head movement is too small, don't update cursor (preserve mouse input)
            if (headMovementDistance < currentHeadThreshold)
            {
                return;
            }
            
            lastHeadPos = currentHeadPos;
        }
    }

    // Use the fixed menu position and rotation for cursor calculations
    // This keeps the menu in a stable position even when the player looks around
    Vector pointerPosition = m_fixedMenuPosition;
    QAngle pointerRotation = m_fixedMenuRotation;
    
    // Compute cursor position using the fixed menu position
    int px, py;
    ComputeCursorPosition(pointerPosition, pointerRotation, px, py);
    
    // Update cursor if position changed
    if ((px != m_nOldCursorX) || (py != m_nOldCursorY))
    {
        // Mark that we're doing a VR cursor update (so we don't mistake this for mouse movement)
        m_bVRCursorUpdateInProgress = true;
        
        // For ViewPort menus, we need to use surface cursor functions
        if (vgui::surface())
        {
            // Set the cursor position on the surface
            vgui::surface()->SurfaceSetCursorPos(px, py);
        }
        
        // Also update VGUI input system for compatibility
        if (g_InputInternal)
        {
            g_InputInternal->InternalCursorMoved(px, py);
        }
        
        if (vgui::input())
        {
            vgui::input()->SetCursorPos(px, py);
        }
        
        m_nOldCursorX = px;
        m_nOldCursorY = py;
        
        // Update tracked mouse position to the new VR-set position
        // so we can detect when the user moves it with the actual mouse
        m_nLastMouseCursorX = px;
        m_nLastMouseCursorY = py;
        
        m_bVRCursorUpdateInProgress = false;
    }
}

void CVRMenuManager::ComputeCursorPosition(const Vector& pointerPosition, const QAngle& pointerRotation, int& px, int& py)
{
    // Special path for compositor mode - use simpler playspace-relative calculations
    if (dxvkIsCompositorActive())
    {
        ComputeCursorPositionCompositor(px, py);
        return;
    }
    
    // Original world-space logic for in-game mode
    Vector controllerPos;
    QAngle controllerAngles;
    bool controllerValid = false;
    Vector rayDir;
    
    // Try to get the aim ray from the active hand using properly transformed pose
    if (m_nMenuHand == 0) // Left hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            controllerValid = g_pOpenXRManager->GetLeftControllerAimRay(controllerPos, rayDir);
            if (controllerValid)
            {
                VectorAngles(rayDir, controllerAngles);
            }
        }
    }
    else // Right hand
    {
        if (g_pOpenXRManager && g_pOpenXRManager->IsRightControllerPoseValid())
        {
            controllerValid = g_pOpenXRManager->GetRightControllerAimRay(controllerPos, rayDir);
            if (controllerValid)
            {
                VectorAngles(rayDir, controllerAngles);
            }
        }
    }
    
    Vector rayStart, rayEnd;
    
    if (controllerValid)
    {
        // Use controller position and direction for ray
        rayStart = controllerPos;
        VectorMA(controllerPos, 1000.0f, rayDir, rayEnd);
    }
    else
    {
        // Fallback to head-based cursor if controller not available
        if (m_pLocalPlayer)
        {
            // In-game: use player eye position and angles (world coordinates)
            QAngle currentViewAngles = m_pLocalPlayer->EyeAngles();
            Vector rayDir;
            AngleVectors(currentViewAngles, &rayDir);
            VectorNormalize(rayDir);
            
            Vector currentEyePos = m_pLocalPlayer->GetVRViewPosition();
            rayStart = currentEyePos;
            VectorMA(currentEyePos, 1000.0f, rayDir, rayEnd);
        }
        else if (dxvkIsCompositorActive() && m_pVRManager)
        {
            // Compositor/main menu: use OpenXR HMD pose (chaperone coordinates)
            Vector hmdOrigin;
            QAngle hmdAngles;
            m_pVRManager->GetHMDInChaperone(hmdOrigin, hmdAngles);
            
            Vector rayDir;
            AngleVectors(hmdAngles, &rayDir);
            VectorNormalize(rayDir);
            
            rayStart = hmdOrigin;
            VectorMA(hmdOrigin, 1000.0f, rayDir, rayEnd);
        }
        else
        {
            // No valid input source available
            px = py = -1;
            return;
        }
        
        
    }
    
    // For playspace anchoring, m_fixedMenuPosition is the actual menu world position
    // For cursor calculation, we need to use that directly (no distance offset)
    Vector menuPlaneCenter;
    Vector forward, right, up;
    AngleVectors(m_fixedMenuRotation, &forward, &right, &up);
    
    if (m_bUsePlayspaceAnchoring)
    {
        // m_fixedMenuPosition is already the menu world position
        menuPlaneCenter = m_fixedMenuPosition;
    }
    else
    {
        // Legacy mode: m_fixedMenuPosition is viewer position, add distance
        float menuDistance = tfvr_menu_distance.GetFloat();
        menuPlaneCenter = m_fixedMenuPosition + forward * menuDistance;
    }
    
    // Use the exact same size calculations as in HandleMenuInput
    float menuHeight, menuWidth;
    GetScaledMenuDimensions(menuWidth, menuHeight);
    
    // Calculate menu plane corners - EXACTLY the same as in HandleMenuInput
    Vector ul = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ur = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ll = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    Vector lr = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    
    
    
    // Compute intersection with the fixed menu plane
    float u, v;
    if (ComputeIntersectionBarycentricCoordinates(rayStart, rayEnd, ul, ur, ll, lr, u, v))
    {
        
        
        // Clamp UV coordinates to valid range
        u = max(0.0f, min(1.0f, u));
        v = max(0.0f, min(1.0f, v));
        
        // Convert (u,v) to screen coordinates
        int screenWidth, screenHeight;
        vgui::surface()->GetScreenSize(screenWidth, screenHeight);
        px = (int)(u * screenWidth + 0.5f);
        py = (int)(v * screenHeight + 0.5f);
        
        // Clamp screen coordinates to valid range
        px = max(0, min(screenWidth - 1, px));
        py = max(0, min(screenHeight - 1, py));
        
        
        return;
    }
    
    
    // If no intersection, use center of screen
    int screenWidth, screenHeight;
    vgui::surface()->GetScreenSize(screenWidth, screenHeight);
    px = screenWidth / 2;
    py = screenHeight / 2;
}

bool CVRMenuManager::ComputeIntersectionBarycentricCoordinates(const Vector& rayStart, const Vector& rayEnd, 
                                                              const Vector& ul, const Vector& ur, const Vector& ll, const Vector& lr,
                                                              float& u, float& v)
{
    // Compute ray direction
    Vector rayDir = rayEnd - rayStart;
    VectorNormalize(rayDir);
    
    // Compute the plane normal from the quad
    Vector edge1 = ur - ul;
    Vector edge2 = ll - ul;
    Vector planeNormal = CrossProduct(edge1, edge2);
    VectorNormalize(planeNormal);
    
    // Check if ray is parallel to plane
    float denominator = DotProduct(rayDir, planeNormal);
    if (fabs(denominator) < 0.0001f)
        return false;
    
    // Find intersection point with plane
    Vector planePoint = ul;
    float t = DotProduct(planePoint - rayStart, planeNormal) / denominator;
    
    if (t < 0)
        return false; // Behind ray start
    
    Vector intersectionPoint = rayStart + rayDir * t;
    
    // Project intersection point onto quad's local coordinate system
    Vector localX = ur - ul; // Right vector
    Vector localY = ll - ul; // Down vector
    
    VectorNormalize(localX);
    VectorNormalize(localY);
    
    Vector toIntersection = intersectionPoint - ul;
    
    // Calculate u and v coordinates
    float quadWidth = VectorLength(ur - ul);
    float quadHeight = VectorLength(ll - ul);
    
    u = DotProduct(toIntersection, localX) / quadWidth;
    v = DotProduct(toIntersection, localY) / quadHeight;
    
    return true; // Return true and let the caller check bounds
}

Vector CVRMenuManager::GetMenuPlaneIntersection(const Vector& controllerPos, const Vector& controllerForward)
{
    // Only return valid intersection if menu position is fixed
    if (!m_bMenuPositionFixed)
        return vec3_origin;
    
    // Use the EXACT same fixed menu plane logic as ComputeCursorPosition
    Vector menuPlaneCenter;
    Vector forward, right, up;
    AngleVectors(m_fixedMenuRotation, &forward, &right, &up);
    
    if (m_bUsePlayspaceAnchoring)
    {
        // m_fixedMenuPosition is already the menu world position
        menuPlaneCenter = m_fixedMenuPosition;
    }
    else
    {
        // Legacy mode: m_fixedMenuPosition is viewer position, add distance
        float menuDistance = tfvr_menu_distance.GetFloat();
        menuPlaneCenter = m_fixedMenuPosition + forward * menuDistance;
    }
    
    // Use the exact same size calculations as in HandleMenuInput
    float menuHeight, menuWidth;
    GetScaledMenuDimensions(menuWidth, menuHeight);
    
    // Calculate menu plane corners - EXACTLY the same as in ComputeCursorPosition
    Vector ul = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ur = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ll = menuPlaneCenter + right * (-menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    Vector lr = menuPlaneCenter + right * (menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    
    // Set up ray from controller
    Vector rayStart = controllerPos;
    Vector rayEnd = controllerPos + controllerForward * 1000.0f;
    
    // Use the same intersection calculation
    float u, v;
    if (ComputeIntersectionBarycentricCoordinates(rayStart, rayEnd, ul, ur, ll, lr, u, v))
    {
        // Compute the actual world intersection point
        Vector rayDir = rayEnd - rayStart;
        VectorNormalize(rayDir);
        
        // Compute the plane normal from the quad
        Vector edge1 = ur - ul;
        Vector edge2 = ll - ul;
        Vector planeNormal = CrossProduct(edge1, edge2);
        VectorNormalize(planeNormal);
        
        // Find intersection point with plane
        Vector planePoint = ul;
        float denominator = DotProduct(rayDir, planeNormal);
        if (fabs(denominator) >= 0.0001f)
        {
            float t = DotProduct(planePoint - rayStart, planeNormal) / denominator;
            if (t >= 0)
            {
                return rayStart + rayDir * t;
            }
        }
    }
    
    return vec3_origin; // No valid intersection
}

void CVRMenuManager::CalculatePlayspaceAnchor()
{
    if (!m_pVRManager || !m_pLocalPlayer)
        return;
    
    // Simple approach: Calculate current playspace origin and store menu position relative to it
    
    // STEP 1: Calculate current playspace origin in world coordinates
    Vector playspaceOriginWorldPos = CalculateCurrentPlayspaceOriginWorldPos();
    
    // STEP 2: Calculate menu position in playspace coordinates
    float menuDistance = tfvr_menu_distance.GetFloat();
    QAngle leveledAngles = m_fixedMenuRotation; // Already leveled
    
    Vector forward, right, up;
    AngleVectors(leveledAngles, &forward, &right, &up);
    Vector menuWorldPos = m_fixedMenuPosition + forward * menuDistance;
    
    // Store menu position relative to playspace origin
    m_menuPositionInPlayspace = menuWorldPos - playspaceOriginWorldPos;
    
    // Store full matrix with rotation
    m_menuPlayspaceAnchor.Identity();
    matrix3x4_t menuMatrix3x4;
    AngleMatrix(leveledAngles, m_menuPositionInPlayspace, menuMatrix3x4);
    m_menuPlayspaceAnchor.CopyFrom3x4(menuMatrix3x4);
}

Vector CVRMenuManager::CalculateCurrentPlayspaceOriginWorldPos()
{
    if (!m_pVRManager || !m_pLocalPlayer) {
        DevMsg("VR Menu: CalculateCurrentPlayspaceOriginWorldPos failed - null pointers (VRManager: %s, Player: %s)\n",
            m_pVRManager ? "valid" : "NULL", m_pLocalPlayer ? "valid" : "NULL");
        return Vector(0, 0, 0);
    }
    
    extern CClientVirtualReality g_ClientVirtualReality;
    VMatrix currentHeadWorldMatrix = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
    
    // GetMideyePose() returns head position relative to playspace origin
    VMatrix headRelativeToPlayspace = m_pVRManager->GetMideyePose();
    
    // Calculate playspace origin relative to head
    VMatrix headToPlayspaceTransform = headRelativeToPlayspace.InverseTR();
    
    // If WorldFromMidEye is valid, use it directly (should have fresh VR data)
    if (!currentHeadWorldMatrix.IsIdentity())
    {
        // Calculate playspace origin in world coordinates using fresh VR matrix
        VMatrix playspaceWorldMatrix = currentHeadWorldMatrix * headToPlayspaceTransform;
        Vector result = playspaceWorldMatrix.GetTranslation();
        
        return result;
    }
    
    // Fallback: If WorldFromMidEye is not available, fall back to player entity data
    DevMsg("VR Menu: WorldFromMidEye not available, using fallback player position (BAD - this causes lag!)\n");
    Vector currentHeadWorldPos = m_pLocalPlayer->GetVRViewPosition();
    QAngle currentHeadWorldAngles = m_pLocalPlayer->EyeAngles();
    
    VMatrix fallbackHeadWorldMatrix;
    fallbackHeadWorldMatrix.Identity();
    matrix3x4_t headMatrix3x4;
    AngleMatrix(currentHeadWorldAngles, currentHeadWorldPos, headMatrix3x4);
    fallbackHeadWorldMatrix.CopyFrom3x4(headMatrix3x4);
    
    VMatrix playspaceWorldMatrix = fallbackHeadWorldMatrix * headToPlayspaceTransform;
    Vector result = playspaceWorldMatrix.GetTranslation();

    return result;
}

Vector CVRMenuManager::GetPlayspaceOriginWorldPos()
{
    return CalculateCurrentPlayspaceOriginWorldPos();
}

void CVRMenuManager::UpdatePlayspaceAnchoredPosition()
{
    if (!m_bUsePlayspaceAnchoring || !m_pVRManager || !m_pLocalPlayer)
        return;
    
    // STEP 1: Calculate current playspace origin in world coordinates
    Vector currentPlayspaceOriginWorldPos = CalculateCurrentPlayspaceOriginWorldPos();
    
    static float lastDebugTime = 0.0f;
    if (gpGlobals && gpGlobals->realtime - lastDebugTime > 0.5f) // Debug every 0.5 seconds
    {
        Vector currentHeadWorldPos = m_pLocalPlayer->GetVRViewPosition();
        VMatrix headRelativeToPlayspace = m_pVRManager->GetMideyePose();
        Vector headPlayspacePos = headRelativeToPlayspace.GetTranslation();
    }
    
    // STEP 3: Transform the stored playspace menu position to current world coordinates  
    Vector newMenuWorldPos = currentPlayspaceOriginWorldPos + m_menuPositionInPlayspace;
    
    static Vector lastMenuWorldPos = newMenuWorldPos;
    lastMenuWorldPos = newMenuWorldPos;
    
    // Get menu rotation from stored anchor
    QAngle newMenuWorldAngles;
    MatrixToAngles(m_menuPlayspaceAnchor, newMenuWorldAngles);
    
    // Update the HUD bounds using the new position
    float menuHeight, menuWidth;
    GetScaledMenuDimensions(menuWidth, menuHeight);
    
    Vector forward, right, up;
    AngleVectors(newMenuWorldAngles, &forward, &right, &up);
    
    Vector ul = newMenuWorldPos + right * (-menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ur = newMenuWorldPos + right * (menuWidth * 0.5f) + up * (menuHeight * 0.5f);
    Vector ll = newMenuWorldPos + right * (-menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    Vector lr = newMenuWorldPos + right * (menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
    
    // Get current head position for viewer reference
    Vector currentHeadWorldPos = m_pLocalPlayer->GetVRViewPosition();
    
    // Update HUD bounds and cached position for cursor calculations
    g_ClientVirtualReality.SetCustomHUDBounds(currentHeadWorldPos, ul, ur, ll, lr);
    
    // Update cached values for cursor calculations
    // For playspace anchoring, we need to store the actual menu world position for cursor calculations
    m_fixedMenuPosition = newMenuWorldPos; // Use actual menu position, not head position
    m_fixedMenuRotation = newMenuWorldAngles;
}

//-----------------------------------------------------------------------------
// Purpose: Render 3D menu in world space when in menu-only mode
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderMenuOnlyMode()
{
    if (!m_pVRManager)
    {
        DevMsg("VR Menu: m_pVRManager is null!\n");
        return;
    }

    // SIMPLIFIED: Just render the menu - let normal VR frame cycle handle BeginFrame/EndFrame
    CopyVGUIDirectlyToVR();
    
    // Debug output (only once per second to avoid spam)
    static float lastDebugTime = 0;
    float currentTime = gpGlobals->realtime;
    if (currentTime - lastDebugTime > 1.0f)
    {
        DevMsg("VR Menu Manager: Rendering menu in VR (simplified approach)\n");
        lastDebugTime = currentTime;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Render VR during loading screens
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderLoadingScreenMode()
{
    if (!m_pVRManager)
    {
        DevMsg("VR Menu: m_pVRManager is null during loading!\n");
        return;
    }

    // Check if normal render cycle is happening by checking frame number
    static int s_lastFrameCount = -1;
    static bool s_normalCycleRunning = false;
    
    if (gpGlobals->framecount != s_lastFrameCount)
    {
        s_lastFrameCount = gpGlobals->framecount;
        // Reset normal cycle detection each frame
        s_normalCycleRunning = false;
    }
    
    // During loading screens, check if we need to manage VR frames
    // Only start our own frame if normal cycle hasn't started one
    bool bShouldManageFrame = !s_normalCycleRunning;
    
    if (bShouldManageFrame && !m_bVRFrameStarted)
    {
        if (m_pVRManager->BeginFrame())
        {
            m_bVRFrameStarted = true;
        }
        else
        {
            // BeginFrame failed, might mean normal cycle already started frame
            s_normalCycleRunning = true;
            bShouldManageFrame = false;
        }
    }

    // For loading screens, render the VGUI (which includes the loading disc)
    CopyVGUIDirectlyToVR();
    
    // Only end frame if we started it
    if (bShouldManageFrame && m_bVRFrameStarted)
    {
        m_pVRManager->EndFrame();
        m_bVRFrameStarted = false;
    }
    
    // Debug output (only once per second to avoid spam)
    static float lastDebugTime = 0;
    float currentTime = gpGlobals->realtime;
    if (currentTime - lastDebugTime > 1.0f)
    {
        DevMsg("VR Menu Manager: Rendering loading screen in VR (managing frames: %d)\n", bShouldManageFrame);
        lastDebugTime = currentTime;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Render VGUI menu panels to texture (based on DrawMainMenu)
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderVGUIToTexture()
{
    VPROF("VR_MenuManager_RenderVGUI");
    
    // Find the VGUI render texture
    ITexture *pTexture = materials->FindTexture("_rt_vgui", NULL, false);
    if (!pTexture)
        return;

    CMatRenderContextPtr pRenderContext(materials);
    int viewActualWidth = pTexture->GetActualWidth();
    int viewActualHeight = pTexture->GetActualHeight();

    int viewWidth, viewHeight;
    vgui::surface()->GetScreenSize(viewWidth, viewHeight);

    // Clear depth before we push the render target
    pRenderContext->ClearBuffers(false, true, true);

    // Set up render target for VGUI
    pRenderContext->PushRenderTargetAndViewport(pTexture, NULL, 0, 0, viewActualWidth, viewActualHeight);

    // When in compositor mode, always use opaque rendering
    bool bUseTranslucent = false;
    
    // Check if we're in main pause menu vs overlay menus
    bool bIsMainMenu = enginevgui && enginevgui->IsGameUIVisible();
    bool bIsEconUIVisible = false;
    
    // Check if any EconUI panels are visible (loadout, backpack, crafting, etc.)
    if ( EconUI() )
    {
        bIsEconUIVisible = EconUI()->IsUIPanelVisible( ECONUI_BACKPACK ) ||
                           EconUI()->IsUIPanelVisible( ECONUI_LOADOUT ) ||
                           EconUI()->IsUIPanelVisible( ECONUI_CRAFTING ) ||
                           EconUI()->IsUIPanelVisible( ECONUI_ARMORY ) ||
                           EconUI()->IsUIPanelVisible( ECONUI_TRADING );
    }
    
    // In compositor mode, always use opaque for better visibility
    // (Translucent rendering can be handled by the compositor itself if needed)
    bUseTranslucent = false;
    
    // Additional detection for other menu types
    bool bIsLoadoutScreen = false;
    bool bIsCursorVisible = vgui::surface() && vgui::surface()->IsCursorVisible();
    bool bIsConnectedToServer = engine && engine->IsConnected();
    
    // Check for class menu state via console variables (loadout selection)
    if (engine && engine->IsInGame())
    {
        ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
        if (pClassMenuOpen && pClassMenuOpen->GetBool())
        {
            bIsLoadoutScreen = true;
        }
    }
    
    // Check for class loadout panel specifically
    if (g_pClassLoadoutPanel && g_pClassLoadoutPanel->IsVisible())
    {
        bIsLoadoutScreen = true;
    }
    
    // Check for character info panel (class selection screen) 
    CCharacterInfoPanel* pCharInfoPanel = GetCharInfoPanel(false);
    if (pCharInfoPanel && pCharInfoPanel->IsVisible())
    {
        bIsLoadoutScreen = true;
    }
    
    // Only log material changes, not every frame
    static bool s_bLastUseTranslucent = true;
    if ( s_bLastUseTranslucent != bUseTranslucent )
    {
        s_bLastUseTranslucent = bUseTranslucent;
        DevMsg("VR VGUI: Switching to %s rendering\n", bUseTranslucent ? "TRANSLUCENT" : "OPAQUE");
    }

    // Configure alpha writing and clear based on menu type
    if ( bUseTranslucent )
    {
        // Translucent: Allow alpha writing and clear with transparent background
        pRenderContext->OverrideAlphaWriteEnable(true, true);
        pRenderContext->ClearColor4ub(0, 0, 0, 0);  // Transparent background
    }
    else
    {
        // Opaque: Disable alpha writing and clear with solid background
        pRenderContext->OverrideAlphaWriteEnable(true, false);
        pRenderContext->ClearColor4ub(0, 0, 0, 255);  // Solid black background
    }
    pRenderContext->ClearBuffers(true, false);

    // Set up VGUI panels for rendering
    vgui::VPANEL root = enginevgui->GetPanel(PANEL_CLIENTDLL);
    if (root != 0)
    {
        vgui::ipanel()->SetSize(root, viewWidth, viewHeight);
    }
    root = enginevgui->GetPanel(PANEL_CLIENTDLL_TOOLS);
    if (root != 0)
    {
        vgui::ipanel()->SetSize(root, viewWidth, viewHeight);
    }

    // Paint the main menu and cursor (this is the key part!)
    render->VGui_Paint((PaintMode_t)(PAINT_UIPANELS | PAINT_CURSOR));
    
    // TF2VR: Also paint in-game panels during loading to capture loading screen
    bool bIsLoading = engine && engine->IsDrawingLoadingImage();
    bool bIsConnected = engine && engine->IsConnected();
    bool bIsInGame = engine && engine->IsInGame();
    bool bIsLoadingAlternative = bIsConnected && !bIsInGame;

    // Restore render context
    pRenderContext->OverrideAlphaWriteEnable(false, true);
    pRenderContext->PopRenderTargetAndViewport();
    pRenderContext->Flush();
}

//-----------------------------------------------------------------------------
// Purpose: Copy VGUI texture directly to VR eye buffer (bypass 3D)
//-----------------------------------------------------------------------------
void CVRMenuManager::CopyVGUIDirectlyToVR()
{
    VPROF("VR_MenuManager_CopyToVR");
     // Step 1: Render the actual TF2 menu to the VGUI texture
    RenderVGUIToTexture();
    
    // Step 2: Get both textures
    ITexture *pVGUITexture = materials->FindTexture("_rt_vgui", NULL, false);
    ITexture *pVRTexture = m_pVRManager->GetRenderTarget();
    
    if (!pVGUITexture || !pVRTexture)
        return;
    
    CMatRenderContextPtr pRenderContext(materials);
    
    // Clear VR buffer first to prevent artifacts
    pRenderContext->PushRenderTargetAndViewport(pVRTexture, NULL, 
        0, 0, pVRTexture->GetActualWidth(), pVRTexture->GetActualHeight());
    
    // Clear with solid color first
    pRenderContext->ClearColor4ub(64, 0, 128, 255);  // Dark purple
    pRenderContext->ClearBuffers(true, true);
    
    // Use a simple copy material to copy VGUI texture to VR buffer
    IMaterial *pCopyMaterial = materials->FindMaterial("vgui/white", TEXTURE_GROUP_VGUI);
    if (pCopyMaterial && !pCopyMaterial->IsErrorMaterial())
    {
        // Bind the VGUI texture to the copy material
        IMaterialVar* pBaseTextureVar = pCopyMaterial->FindVar("$basetexture", NULL);
        if (pBaseTextureVar)
        {
            pBaseTextureVar->SetTextureValue(pVGUITexture);
        }
        
        // Draw a smaller rectangle in the center to test
        int vrWidth = pVRTexture->GetActualWidth();
        int vrHeight = pVRTexture->GetActualHeight();
        int quadWidth = vrWidth / 2;   // Half size
        int quadHeight = vrHeight / 2; // Half size
        int offsetX = vrWidth / 4;     // Center it
        int offsetY = vrHeight / 4;    // Center it
        
        pRenderContext->DrawScreenSpaceRectangle(
            pCopyMaterial,
            offsetX, offsetY,           // x, y (centered)
            quadWidth, quadHeight,      // width, height (half size)
            0, 0,                       // src x, y  
            pVGUITexture->GetActualWidth(), pVGUITexture->GetActualHeight(), // src width, height
            pVGUITexture->GetActualWidth(), pVGUITexture->GetActualHeight()  // texture size
        );
    }
    
    // Restore render target
    pRenderContext->PopRenderTargetAndViewport();
    pRenderContext->Flush();
}

//-----------------------------------------------------------------------------
// Purpose: Render a test pattern to the VGUI texture for debugging
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderTestPattern()
{
    // Find the VGUI render texture
    ITexture *pTexture = materials->FindTexture("_rt_vgui", NULL, false);
    if (!pTexture)
        return;

    CMatRenderContextPtr pRenderContext(materials);
    int viewActualWidth = pTexture->GetActualWidth();
    int viewActualHeight = pTexture->GetActualHeight();

    // Set up render target for test pattern
    pRenderContext->PushRenderTargetAndViewport(pTexture, NULL, 0, 0, viewActualWidth, viewActualHeight);
    pRenderContext->OverrideAlphaWriteEnable(true, true);

    // Clear with a bright color first
    pRenderContext->ClearColor4ub(255, 0, 255, 255);  // Magenta background
    pRenderContext->ClearBuffers(true, false);
    
    // Draw some simple test rectangles using screen space rectangles
    IMaterial *pWhiteMaterial = materials->FindMaterial("vgui/white", TEXTURE_GROUP_VGUI);
    if (pWhiteMaterial && !pWhiteMaterial->IsErrorMaterial())
    {
        // Draw a green rectangle in the center
        pRenderContext->Bind(pWhiteMaterial);
        pRenderContext->DrawScreenSpaceRectangle(
            pWhiteMaterial,
            viewActualWidth/4, viewActualHeight/4,     // x, y
            viewActualWidth/2, viewActualHeight/2,     // width, height
            0, 0, 1, 1,                                // texture coords
            viewActualWidth, viewActualHeight          // texture size
        );
    }

    // Restore render context
    pRenderContext->OverrideAlphaWriteEnable(false, true);
    pRenderContext->PopRenderTargetAndViewport();
    pRenderContext->Flush();
}

//-----------------------------------------------------------------------------
// Purpose: Render test pattern directly to VR render target (debug)
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderTestPatternDirectly()
{
    // Get the VR render target
    ITexture* pVRTexture = m_pVRManager->GetRenderTarget();
    if (!pVRTexture)
    {
        DevMsg("VR Menu: GetRenderTarget() returned null!\n");
        return;
    }

    DevMsg("VR Menu: Rendering test pattern to VR target %dx%d\n", 
           pVRTexture->GetActualWidth(), pVRTexture->GetActualHeight());

    CMatRenderContextPtr pRenderContext(materials);
    
    // Render directly to the VR target
    pRenderContext->PushRenderTargetAndViewport(pVRTexture, NULL, 
        0, 0, pVRTexture->GetActualWidth(), pVRTexture->GetActualHeight());
    
    // Clear with bright red to test
    pRenderContext->ClearColor4ub(255, 0, 0, 255);  // Bright red
    pRenderContext->ClearBuffers(true, true);
    
    // Draw some test rectangles directly in screen space
    IMaterial *pWhiteMaterial = materials->FindMaterial("vgui/white", TEXTURE_GROUP_VGUI);
    if (pWhiteMaterial && !pWhiteMaterial->IsErrorMaterial())
    {
        int width = pVRTexture->GetActualWidth();
        int height = pVRTexture->GetActualHeight();
        
        // Draw a blue rectangle in the center
        pRenderContext->DrawScreenSpaceRectangle(
            pWhiteMaterial,
            width/4, height/4,      // x, y
            width/2, height/2,      // width, height  
            0, 0, 1, 1,             // texture coords
            width, height           // texture size
        );
    }
    
    // Pop render target
    pRenderContext->PopRenderTargetAndViewport();
    pRenderContext->Flush();
}

//-----------------------------------------------------------------------------
// Purpose: Set up minimal 3D world context for rendering
//-----------------------------------------------------------------------------
void CVRMenuManager::SetupMinimal3DWorld()
{
    // Get the VR render target
    ITexture* pVRTexture = m_pVRManager->GetRenderTarget();
    if (!pVRTexture)
        return;

    // Create a basic view setup for 3D rendering
    CViewSetup viewSetup;
    memset(&viewSetup, 0, sizeof(viewSetup));
    
    // Set up basic 3D view parameters
    viewSetup.x = 0;
    viewSetup.y = 0;
    viewSetup.width = pVRTexture->GetActualWidth();
    viewSetup.height = pVRTexture->GetActualHeight();
    viewSetup.fov = 90.0f;
    viewSetup.origin = Vector(0, 0, 64);  // Player head position
    viewSetup.angles = QAngle(0, 0, 0);   // Looking forward
    viewSetup.zNear = 1.0f;
    viewSetup.zFar = 30000.0f;
    viewSetup.m_bOrtho = false;

    // Set up render target
    CMatRenderContextPtr pRenderContext(materials);
    pRenderContext->PushRenderTargetAndViewport(pVRTexture, NULL, 
        0, 0, viewSetup.width, viewSetup.height);

    // Clear with a dark background (not pure black to see the menu better)
    pRenderContext->ClearColor4ub(32, 32, 32, 255);
    pRenderContext->ClearBuffers(true, true);

    // Set up 3D view for rendering
    render->Push3DView(viewSetup, 0, pVRTexture, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Render the menu texture as a 3D quad in world space
//-----------------------------------------------------------------------------
void CVRMenuManager::RenderMenuQuadIn3D()
{
    // Render menu quad in 3D space
    
    // Find the VGUI texture that we rendered to
    ITexture *pVGUITexture = materials->FindTexture("_rt_vgui", NULL, false);
    if (!pVGUITexture)
    {
        DevMsg("VR Menu: _rt_vgui texture not found!\n");
        return;
    }
    
    // DevMsg("VR Menu: Rendering 3D quad with VGUI texture %dx%d\n", 
    //        pVGUITexture->GetActualWidth(), pVGUITexture->GetActualHeight());

    // Use static material caching to avoid repeated lookups and precaching
    static IMaterial *pCachedMaterial = nullptr;
    static bool bMaterialInitialized = false;
    
    // Determine material selection based on menu type (similar to RenderHUDQuad logic)
    bool bUseTranslucent = false;
    
    // Check if we're in main pause menu vs overlay menus
    bool bIsMainMenu = enginevgui && enginevgui->IsGameUIVisible();
    bool bIsEconUIVisible = false;
    bool bIsConnectedToServer = engine && engine->IsConnected();
    
    // Check if normal gameplay HUD is visible (health, ammo, etc.)
    bool bIsNormalHUDVisible = false;
    bool bIsDeadPlayerInGame = false;
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer && engine->IsInGame())
    {
        // Check if HUD elements are not hidden
        int iHideHud = pPlayer->m_Local.m_iHideHUD;
        extern ConVar hidehud;
        if (hidehud.GetInt())
        {
            iHideHud = hidehud.GetInt();
        }
        
        // HUD is visible if not all hidden and not in VGui input mode
        bool bHUDNotHidden = !(iHideHud & HIDEHUD_ALL) && !pPlayer->IsInVGuiInputMode() && !bIsMainMenu;
        
        if (pPlayer->IsAlive())
        {
            // Living player with normal HUD
            bIsNormalHUDVisible = bHUDNotHidden;
        }
        else 
        {
            // Dead player - check if they're spectating or in death cam (should still use translucent)
            bIsDeadPlayerInGame = bHUDNotHidden;
        }
    }
    
    	// Check if any EconUI panels are visible (loadout, backpack, crafting, etc.)
	if ( EconUI() )
	{
		bIsEconUIVisible = EconUI()->IsUIPanelVisible( ECONUI_BACKPACK ) ||
						   EconUI()->IsUIPanelVisible( ECONUI_LOADOUT ) ||
						   EconUI()->IsUIPanelVisible( ECONUI_CRAFTING ) ||
						   EconUI()->IsUIPanelVisible( ECONUI_ARMORY ) ||
						   EconUI()->IsUIPanelVisible( ECONUI_TRADING );
	}
	
	// Additional checks for loadout/armory screens that EconUI might miss
	bool bIsLoadoutOrArmoryScreen = false;
	if (engine && engine->IsConnected())
	{
		// Check for class loadout panel specifically
		if (g_pClassLoadoutPanel && g_pClassLoadoutPanel->IsVisible())
		{
			bIsLoadoutOrArmoryScreen = true;
		}
		
		// Check for character info panel (class selection screen) 
		CCharacterInfoPanel* pCharInfoPanel = GetCharInfoPanel(false);
		if (pCharInfoPanel && pCharInfoPanel->IsVisible())
		{
			bIsLoadoutOrArmoryScreen = true;
		}
	}
	
	// Material selection logic with proper priority (same as viewrender.cpp):
	// 1. True main menu (not connected) = opaque
	// 2. Overlay menus (class select, loadout, inventory, etc.) = opaque  
	// 3. In-game pause menu = translucent
	// 4. Normal gameplay HUD (health, ammo, etc.) = translucent
	// 5. Dead player in-game (spectating, death cam) = translucent
	// 6. Default = opaque
	if (!bIsConnectedToServer)
	{
		// True main menu (not connected) - use opaque
		bUseTranslucent = false;
	}
	else if (bIsEconUIVisible || bIsLoadoutOrArmoryScreen)
	{
		// Overlay menus (class select, loadout, inventory, etc.) - use opaque
		bUseTranslucent = false;
	}
	else if (bIsMainMenu)
	{
		// In-game pause menu - use translucent
		bUseTranslucent = true;
	}
	else if (bIsNormalHUDVisible || bIsDeadPlayerInGame)
	{
		// Normal gameplay HUD with health/ammo OR dead player in-game - use translucent
		bUseTranslucent = true;
	}
	else
	{
		// Default: opaque for unknown states
		bUseTranslucent = false;
	}
    
    // Debug output
    static bool s_bLastUseTranslucent = true;
    static int s_debugFrameCount = 0;
    s_debugFrameCount++;
    
    if ( s_debugFrameCount % 60 == 0 ) // Every second
    {
        DevMsg("VR Menu Manager: MainMenu=%d, EconUI=%d, UseTranslucent=%d\n", 
            bIsMainMenu ? 1 : 0, bIsEconUIVisible ? 1 : 0, bUseTranslucent ? 1 : 0);
    }
    
    if ( s_bLastUseTranslucent != bUseTranslucent )
    {
        s_bLastUseTranslucent = bUseTranslucent;
        DevMsg("VR Menu Manager: Switching to %s material\n", bUseTranslucent ? "TRANSLUCENT" : "OPAQUE");
    }

    // Select the appropriate material based on menu type
    if ( bUseTranslucent )
    {
        pCachedMaterial = materials->FindMaterial("vgui/inworldui", TEXTURE_GROUP_VGUI);
    }
    else
    {
        pCachedMaterial = materials->FindMaterial("vgui/inworldui_opaque", TEXTURE_GROUP_VGUI);
    }
    
    // If material loading fails, fall back to basic material
    if (!pCachedMaterial || pCachedMaterial->IsErrorMaterial())
    {
        pCachedMaterial = materials->FindMaterial("vgui/white", TEXTURE_GROUP_VGUI);
    }
    
    // Ensure the material is properly precached and referenced
    if (pCachedMaterial && !pCachedMaterial->IsErrorMaterial())
    {
        if (!pCachedMaterial->IsPrecached()) 
        {
            PrecacheMaterial(pCachedMaterial->GetName());
            pCachedMaterial->IncrementReferenceCount();
        }
    }
    
    if (!pCachedMaterial || pCachedMaterial->IsErrorMaterial())
        return;

    // Force render state for opaque materials to ensure no transparency
    if ( !bUseTranslucent )
    {
        // For opaque materials, force full opacity and disable blending
        float color[3] = { 1.0f, 1.0f, 1.0f };
        render->SetColorModulation( color );
        render->SetBlend( 1.0f );
    }

    // Position the menu quad MUCH closer to the player
    Vector vCenter = Vector(0, 50, 64);  // 50 units in front, at head height  
    float menuHeight, menuWidth;
    GetScaledMenuDimensions(menuWidth, menuHeight);
    // Scale down for closer 3D rendering
    menuWidth *= 0.75f;
    menuHeight *= 0.75f;
    
    // Create quad corners
    Vector vUL = vCenter + Vector(-menuWidth/2, 0, menuHeight/2);   // Upper Left
    Vector vUR = vCenter + Vector(menuWidth/2, 0, menuHeight/2);    // Upper Right
    Vector vLL = vCenter + Vector(-menuWidth/2, 0, -menuHeight/2);  // Lower Left
    Vector vLR = vCenter + Vector(menuWidth/2, 0, -menuHeight/2);   // Lower Right

    // Bind the VGUI texture to the material 
    CMatRenderContextPtr pRenderContext(materials);
    pRenderContext->Bind(pCachedMaterial, NULL);
    
    // Set the VGUI texture as the base texture for the material
    IMaterialVar* pBaseTextureVar = pCachedMaterial->FindVar("$basetexture", NULL);
    if (pBaseTextureVar)
    {
        pBaseTextureVar->SetTextureValue(pVGUITexture);
    }
    
    // Render the quad using dynamic mesh (same pattern as RenderHUDQuad)
    IMesh *pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, pCachedMaterial);

    CMeshBuilder meshBuilder;
    meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

    // Lower Right
    meshBuilder.Position3fv(vLR.Base());
    meshBuilder.TexCoord2f(0, 1, 1);
    meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

    // Lower Left
    meshBuilder.Position3fv(vLL.Base());
    meshBuilder.TexCoord2f(0, 0, 1);
    meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

    // Upper Right
    meshBuilder.Position3fv(vUR.Base());
    meshBuilder.TexCoord2f(0, 1, 0);
    meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

    // Upper Left
    meshBuilder.Position3fv(vUL.Base());
    meshBuilder.TexCoord2f(0, 0, 0);
    meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

    meshBuilder.End();
    pMesh->Draw();
    
    // NOTE: VR HUD rendering has been moved to viewrender.cpp
    // g_pVRStatusHUDManager->Render() and g_pVRWeaponHUDManager->Render()
    // are called from CViewRender::RenderView()

    // Clean up 3D view
    render->PopView(NULL);
    
    // Pop render target
    CMatRenderContextPtr pRenderContext2(materials);
    pRenderContext2->PopRenderTargetAndViewport();
    pRenderContext2->Flush();
}

//-----------------------------------------------------------------------------
// Compositor-specific cursor positioning using playspace-relative poses
//-----------------------------------------------------------------------------
void CVRMenuManager::ComputeCursorPositionCompositor(int& px, int& py)
{
    // Initialize to invalid position
    px = py = -1;
    
    if (!m_pVRManager)
    {
        DevMsg("VR Cursor (Compositor): No VR Manager available\n");
        return;
    }
    
    // Debug output (reduce frequency to avoid performance issues)
    static float lastDebugTime = 0;
    bool shouldDebug = tfvr_cursor_debug.GetBool() && (gpGlobals->curtime - lastDebugTime > 0.5f);
    if (shouldDebug) lastDebugTime = gpGlobals->curtime;
    
    // Safety check: ensure OpenXR manager is fully initialized
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): OpenXR manager not active - skipping frame\n");
        }
        return;
    }
    
    // Safety check: ensure VGUI surface is available
    if (!vgui::surface())
    {
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): VGUI surface not available - skipping frame\n");
        }
        return;
    }
    
    // For compositor mode, we work entirely in playspace/chaperone coordinates
    // This avoids the need for world-space conversions that require a player entity
    
    Vector rayStart, rayDir;
    bool validRay = false;
    const char* inputSource = "none";
    
    // Try to get controller pose first (preferred) - use RAW poses for compositor
    if (m_nMenuHand == 0) // Left hand
    {
        inputSource = "left_controller";
        if (g_pOpenXRManager && g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            VMatrix controllerMatrix;
            if (g_pOpenXRManager->GetLeftControllerPoseRaw(controllerMatrix))
            {
                rayStart = controllerMatrix.GetTranslation();
                rayDir = controllerMatrix.GetForward();
                validRay = true;
                
                if (shouldDebug)
                {
                    DevMsg("VR Cursor (Compositor): Left controller - pos=(%.1f,%.1f,%.1f) dir=(%.2f,%.2f,%.2f)\n",
                           rayStart.x, rayStart.y, rayStart.z, rayDir.x, rayDir.y, rayDir.z);
                }
            }
            else if (shouldDebug)
            {
                DevMsg("VR Cursor (Compositor): Left controller pose valid but GetLeftControllerPoseRaw failed\n");
            }
        }
        else if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): Left controller pose not valid\n");
        }
    }
    else // Right hand
    {
        inputSource = "right_controller";
        if (g_pOpenXRManager && g_pOpenXRManager->IsRightControllerPoseValid())
        {
            VMatrix controllerMatrix;
            if (g_pOpenXRManager->GetRightControllerPoseRaw(controllerMatrix))
            {
                rayStart = controllerMatrix.GetTranslation();
                rayDir = controllerMatrix.GetForward();
                validRay = true;
                
                if (shouldDebug)
                {
                    DevMsg("VR Cursor (Compositor): Right controller - pos=(%.1f,%.1f,%.1f) dir=(%.2f,%.2f,%.2f)\n",
                           rayStart.x, rayStart.y, rayStart.z, rayDir.x, rayDir.y, rayDir.z);
                }
            }
            else if (shouldDebug)
            {
                DevMsg("VR Cursor (Compositor): Right controller pose valid but GetRightControllerPoseRaw failed\n");
            }
        }
        else if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): Right controller pose not valid\n");
        }
    }
    
    // Fallback to HMD if no controller available
    if (!validRay)
    {
        inputSource = "hmd_fallback";
        Vector hmdOrigin;
        QAngle hmdAngles;
        m_pVRManager->GetHMDInChaperone(hmdOrigin, hmdAngles);
        
        rayStart = hmdOrigin;
        AngleVectors(hmdAngles, &rayDir);
        validRay = true;
        
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): HMD fallback - pos=(%.1f,%.1f,%.1f) angles=(%.1f,%.1f,%.1f) dir=(%.2f,%.2f,%.2f)\n",
                   hmdOrigin.x, hmdOrigin.y, hmdOrigin.z, hmdAngles.x, hmdAngles.y, hmdAngles.z, rayDir.x, rayDir.y, rayDir.z);
        }
    }
    
    if (!validRay)
    {
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): No valid ray source available\n");
        }
        return;
    }
    
    // For compositor mode, we need to work in playspace coordinates
    // Use the same logic as the regular cursor positioning but with raw poses
    
    Vector ul, ur, ll, lr;
    
    // For compositor mode, use the cached coordinates that were last sent to the compositor
    // This ensures perfect alignment between cursor collision and what's actually being rendered
    if (g_ClientVirtualReality.HasCachedCompositorCoords())
    {
        // Get the exact same coordinates that were sent to the compositor
        Vector cachedUL, cachedUR, cachedLL, cachedLR;
        g_ClientVirtualReality.GetCachedCompositorCoords(cachedUL, cachedUR, cachedLL, cachedLR);
        
        ul = cachedUL;
        ur = cachedUR;
        ll = cachedLL;
        lr = cachedLR;
        
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): Using cached compositor coordinates!\n");
            DevMsg("VR Cursor (Compositor): Cached coords - UL=(%.1f,%.1f,%.1f) UR=(%.1f,%.1f,%.1f)\n",
                   ul.x, ul.y, ul.z, ur.x, ur.y, ur.z);
        }
        }
    else
    {
        // Fallback: No fixed menu position, use HMD-relative positioning
        Vector currentHmdOrigin;
        QAngle currentHmdAngles;
        m_pVRManager->GetHMDInChaperone(currentHmdOrigin, currentHmdAngles);
        
        float menuDistance = tfvr_menu_distance.GetFloat();
        Vector forward, right, up;
        AngleVectors(currentHmdAngles, &forward, &right, &up);
        Vector menuCenter = currentHmdOrigin + forward * menuDistance;
        
        float menuHeight, menuWidth;
        GetScaledMenuDimensions(menuWidth, menuHeight);
        
        ul = menuCenter + right * (-menuWidth * 0.5f) + up * (menuHeight * 0.5f);
        ur = menuCenter + right * (menuWidth * 0.5f) + up * (menuHeight * 0.5f);
        ll = menuCenter + right * (-menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
        lr = menuCenter + right * (menuWidth * 0.5f) + up * (-menuHeight * 0.5f);
        
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): Using HMD-relative fallback - HMD=(%.1f,%.1f,%.1f) center=(%.1f,%.1f,%.1f)\n",
                   currentHmdOrigin.x, currentHmdOrigin.y, currentHmdOrigin.z, menuCenter.x, menuCenter.y, menuCenter.z);
        }
    }
    
    if (shouldDebug)
    {
        DevMsg("VR Cursor (Compositor): Menu corners - UL=(%.1f,%.1f,%.1f) UR=(%.1f,%.1f,%.1f) LL=(%.1f,%.1f,%.1f) LR=(%.1f,%.1f,%.1f)\n",
               ul.x, ul.y, ul.z, ur.x, ur.y, ur.z, ll.x, ll.y, ll.z, lr.x, lr.y, lr.z);
    }
    
    // Ray-plane intersection
    Vector rayEnd = rayStart + rayDir * 1000.0f;
    
    if (shouldDebug)
    {
        DevMsg("VR Cursor (Compositor): Ray - start=(%.1f,%.1f,%.1f) end=(%.1f,%.1f,%.1f)\n",
               rayStart.x, rayStart.y, rayStart.z, rayEnd.x, rayEnd.y, rayEnd.z);
    }
    
    float u, v;
    if (ComputeIntersectionBarycentricCoordinates(rayStart, rayEnd, ul, ur, ll, lr, u, v))
    {
        // Convert UV coordinates to screen pixels
        int screenWidth, screenHeight;
        vgui::surface()->GetScreenSize(screenWidth, screenHeight);
        
        // Store original UV values for debug
        float originalU = u, originalV = v;
        
        // Clamp to valid range
        u = clamp(u, 0.0f, 1.0f);
        v = clamp(v, 0.0f, 1.0f);
        
        px = (int)(u * screenWidth);
        py = (int)(v * screenHeight);
        
        // Enhanced debug output
        if (shouldDebug)
        {
            bool clamped = (originalU != u) || (originalV != v);
            DevMsg("VR Cursor (Compositor): SUCCESS! Source=%s UV=(%.3f,%.3f) %s Screen=(%d,%d) ScreenSize=(%dx%d)\n", 
                   inputSource, originalU, originalV, clamped ? "CLAMPED" : "", px, py, screenWidth, screenHeight);
        }
    }
    else
    {
        if (shouldDebug)
        {
            DevMsg("VR Cursor (Compositor): MISS! Source=%s - ray does not intersect menu plane\n", inputSource);
        }
    }
}
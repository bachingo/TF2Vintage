#ifndef VR_MENU_MANAGER_H
#define VR_MENU_MANAGER_H

#include "cbase.h"
#include "mathlib/vector.h"
#include "vgui/IInput.h"
#include "vgui/IInputInternal.h"
#include "client_virtualreality.h"
#include "hmdWrapper.h"

// Forward declarations
class C_TFPlayer;
class COpenXRManager;
class CVRStatusHUDManager;
class CVRWeaponHUDManager;
class CVRSpringHUDManager;
class CVRDamageIndicatorManager;
class CVRWeaponSelectManager;
class CVRPopupHUDManager;
class CVRWorldHealthIconManager;
class CVRDamageNumberManager;
class CVRSpectatorExtrasManager;
class CVRSpectatorCamera;
class CVRCollisionWarningManager;

// Global input pointer declarations
extern vgui::IInputInternal *g_InputInternal;

class CVRMenuManager
{
public:
    CVRMenuManager();
    ~CVRMenuManager();

    void Initialize();
    void Shutdown();
    void Update();

    // Main menu input handling function
    void HandleMenuInput();

    // Public method to check if menu is visible (for external access)
    // Includes death state for VR input blocking purposes.
    bool IsMenuVisible();

    // Check if a real menu panel is open (escape menu, class/team select, etc.)
    // Unlike IsMenuVisible(), does NOT count the death state itself as a menu.
    bool IsMenuPanelOpen();
    
    // Public method to get the world position where the controller ray intersects the menu plane
    Vector GetMenuPlaneIntersection(const Vector& controllerPos, const Vector& controllerForward);
    
    // Public method to get which hand is currently active for menu interaction
    int GetActiveMenuHand() const { return m_nMenuHand; }

    // Compositor integration methods
    void SubmitMenuFrameToCompositor();
    void SubmitLoadingFrameToCompositor();
    
    // VGUI rendering method (public so it can be called from render loop)
    void RenderVGUIToTexture();
    
    // State determination and mode handling
    SourceEngineState DetermineSourceState();
    
    // Public method to get playspace origin for external access (needed for HUD positioning)
    Vector GetPlayspaceOriginWorldPos();
    
    // Public method to update cursor position (called from OverridePlayerMotion for fresh positioning)
    void UpdateCursorPosition();

private:
    // Helper functions
    void HandleMenuButtonInput();
    void ComputeCursorPositionCompositor(int& px, int& py); // Playspace-relative cursor for compositor mode
    void ResetMousePriorityTracking();  // Reset mouse/VR control tracking state when menu opens
    void HandleCompositorMode(SourceEngineState state);
    void HandleTraditionalVRMode(SourceEngineState state);
    
    // VR rendering methods
    void RenderMenuOnlyMode();
    void RenderLoadingScreenMode();
    void CopyVGUIDirectlyToVR();
    void SetupMinimal3DWorld();
    void RenderMenuQuadIn3D();
    void RenderTestPattern();
    void RenderTestPatternDirectly();
    
    // Cursor positioning
    void ComputeCursorPosition(const Vector& pointerPosition, const QAngle& pointerRotation, int& px, int& py);
    bool ComputeIntersectionBarycentricCoordinates(const Vector& rayStart, const Vector& rayEnd, 
                                                   const Vector& ul, const Vector& ur, const Vector& ll, const Vector& lr,
                                                   float& u, float& v);
    
    // Playspace anchoring methods
    void CalculatePlayspaceAnchor();
    void UpdatePlayspaceAnchoredPosition();
    Vector CalculateCurrentPlayspaceOriginWorldPos();

    // VR input state
    bool m_bMenuButtonPressed;
    bool m_bMenuButtonReleased;
    int m_nMenuHand; // 0 = left, 1 = right
    
    // Cursor state
    int m_nOldCursorX;
    int m_nOldCursorY;
    
    // Input priority tracking - compare mouse vs VR movement to determine which has priority
    int m_nLastMouseCursorX;           // Last cursor position (to detect mouse movement)
    int m_nLastMouseCursorY;
    Vector m_vecLastControllerPos;     // Last controller position (to detect VR movement)
    Vector m_vecControllerAnchor;      // Controller position when mouse took priority (dead zone center)
    float m_flRecentMouseMovement;     // Rolling average of mouse movement (pixels/frame)
    float m_flRecentVRMovement;        // Rolling average of VR controller movement (world units/frame)
    bool m_bVRCursorUpdateInProgress;  // Flag to distinguish VR cursor updates from mouse
    bool m_bMouseHasPriority;          // True if mouse is currently more active than VR
    float m_flVRReclaimCooldown;       // Seconds remaining before mouse can re-take priority after VR reclaims
    
    // VR manager reference
    COpenXRManager* m_pVRManager;
    
    // Player reference
    C_TFPlayer* m_pLocalPlayer;
    
    // Saved view origin for menu input
    Vector m_savedPlayerViewOrigin;
    
    // Fixed menu position and orientation (captured when menu opens)
    Vector m_fixedMenuPosition;
    QAngle m_fixedMenuRotation;
    bool m_bMenuPositionFixed;
    
    // Playspace-anchored menu positioning
    VMatrix m_menuPlayspaceAnchor;  // Menu's absolute position/rotation in playspace coordinates
    bool m_bUsePlayspaceAnchoring;
    Vector m_menuPositionInPlayspace;  // Menu's fixed position relative to playspace origin
    
    // ConVars
    ConVar* m_pConVarPrimaryHand;
    
    // Map change detection
    char m_szLastMapName[64];
    float m_flLastClassMenuTime; // Time when changeclass was last executed
    
    // Class/Team menu hold detection
    float m_flClassMenuButtonPressTime;     // When button was first pressed
    bool m_bClassMenuHoldActionExecuted;    // Whether hold action (team menu) was already triggered
    
    // VR frame management
    bool m_bVRFrameStarted;
    
    // VR tracking update optimization
    int m_nLastVRTrackingUpdateFrame;
    
    // VR Status HUD (left hand: health, objectives)
    CVRStatusHUDManager* m_pVRStatusHUDManager;
    
    // VR Weapon HUD (right hand: ammo, meters, charges)
    CVRWeaponHUDManager* m_pVRWeaponHUDManager;
    
    // VR Spring HUD (head-relative: kill feed with spring-arm behavior)
    CVRSpringHUDManager* m_pVRSpringHUDManager;
    
    // VR Damage Indicator (head-relative: damage direction with spring-arm behavior)
    CVRDamageIndicatorManager* m_pVRDamageIndicatorManager;
    
    // VR Weapon Select (radial weapon selection menu)
    CVRWeaponSelectManager* m_pVRWeaponSelectManager;
    
    // VR Popup HUD (head-relative: win/loss panels, scoreboard)
    CVRPopupHUDManager* m_pVRPopupHUDManager;
    
    // VR World Health Icon (world-space health icons above players)
    CVRWorldHealthIconManager* m_pVRWorldHealthIconManager;
    
    // VR Damage Numbers (world-space floating damage/healing numbers)
    CVRDamageNumberManager* m_pVRDamageNumberManager;
    
    // VR Spectator Extras (world-space player names and health bars)
    CVRSpectatorExtrasManager* m_pVRSpectatorExtrasManager;
    
    // VR Spectator Camera (Half-Life: Alyx style camera smoothing for streaming/trailers)
    CVRSpectatorCamera* m_pVRSpectatorCamera;

    // VR Collision Warning (floating warning text when head position is desynced)
    CVRCollisionWarningManager* m_pVRCollisionWarningManager;
};

// Global instance
extern CVRMenuManager* g_pVRMenuManager;

#endif // VR_MENU_MANAGER_H

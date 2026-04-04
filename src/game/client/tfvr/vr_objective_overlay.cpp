#include "cbase.h"
#include "vr_objective_overlay.h"
#include "tf/tf_hud_objectivestatus.h"
#include "tf/tf_hud_flagstatus.h"
#include "tf/tf_hud_escort.h"
#include "tf/tf_hud_training.h"
#include "tf/tf_hud_robot_destruction_status.h"
#include "tf/tf_hud_passtime.h"
#include "hud_controlpointicons.h"

//-----------------------------------------------------------------------------
// NEW APPROACH: Sample directly from the _rt_vgui render target!
// This avoids ALL the reparenting/state modification issues by just reading 
// the already-rendered content from where it exists on the main HUD.
//-----------------------------------------------------------------------------

// Include VGUI headers first to ensure types are defined
#include "vgui/ISurface.h"
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"

// Now include other headers
#include "c_tf_player.h"
#include "openxr_manager.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "bitmap/imageformat.h"
#include "view_shared.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "hudelement.h"
#include "KeyValues.h"
#include "ienginevgui.h"
#include "hud.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "tf_shareddefs.h"

// Include TF2 headers last, after VGUI types are defined
#include "tf/tf_hud_objectivestatus.h"
#include "tf_gamerules.h"

// Simple approach: Just use the original panel but render it differently
// We'll go back to the hijacking approach but make it cleaner

// ConVars for configuration
ConVar tfvr_objective_overlay_enabled("tfvr_objective_overlay_enabled", "1", FCVAR_ARCHIVE, "Enable VR objective overlay on hand");
ConVar tfvr_objective_overlay_hand("tfvr_objective_overlay_hand", "0", FCVAR_ARCHIVE, "Hand to attach objective overlay to: 0=left, 1=right (should match health overlay)");
ConVar tfvr_objective_overlay_use_hand_tracking("tfvr_objective_overlay_use_hand_tracking", "1", FCVAR_ARCHIVE, "Use hand tracking instead of controller pose: 0=controller, 1=hand tracking");
ConVar tfvr_objective_overlay_offset_x("tfvr_objective_overlay_offset_x", "-3.2", FCVAR_ARCHIVE, "X offset from hand position");
ConVar tfvr_objective_overlay_offset_y("tfvr_objective_overlay_offset_y", "3", FCVAR_ARCHIVE, "Y offset from hand position (below health overlay)");
ConVar tfvr_objective_overlay_offset_z("tfvr_objective_overlay_offset_z", "-1.2", FCVAR_ARCHIVE, "Z offset from hand position (forward)");
ConVar tfvr_objective_overlay_scale("tfvr_objective_overlay_scale", "6", FCVAR_ARCHIVE, "Scale of objective overlay");
ConVar tfvr_objective_overlay_debug_bg("tfvr_objective_overlay_debug_bg", "0", FCVAR_ARCHIVE, "Show debug background to see quad boundaries");
ConVar tfvr_objective_overlay_center_content("tfvr_objective_overlay_center_content", "0", FCVAR_ARCHIVE, "Automatically center objective content in capture area");
ConVar tfvr_objective_overlay_force_flag_updates("tfvr_objective_overlay_force_flag_updates", "1", FCVAR_ARCHIVE, "Force flag panel updates for CTF arrow indicators");
ConVar tfvr_objective_overlay_no_main_hud("tfvr_objective_overlay_no_main_hud", "1", FCVAR_ARCHIVE, "Prevent VR objective overlay from appearing on main HUD");
ConVar tfvr_objective_overlay_restore_state("tfvr_objective_overlay_restore_state", "1", FCVAR_ARCHIVE, "Restore panel state after VR rendering to avoid interfering with main HUD");
ConVar tfvr_objective_overlay_restore_position("tfvr_objective_overlay_restore_position", "0", FCVAR_ARCHIVE, "Restore panel position (0=size only, 1=position too)");
ConVar tfvr_objective_overlay_content_center_x("tfvr_objective_overlay_content_center_x", "960", FCVAR_ARCHIVE, "X coordinate of content center in original HUD layout");
ConVar tfvr_objective_overlay_content_center_y("tfvr_objective_overlay_content_center_y", "540", FCVAR_ARCHIVE, "Y coordinate of content center in original HUD layout");
ConVar tfvr_objective_overlay_simple_transform("tfvr_objective_overlay_simple_transform", "0", FCVAR_ARCHIVE, "Use simple identity transform for debugging");
ConVar tfvr_objective_overlay_no_rotation("tfvr_objective_overlay_no_rotation", "0", FCVAR_ARCHIVE, "Skip final 180-degree rotation for debugging");
ConVar tfvr_objective_overlay_world_width("tfvr_objective_overlay_world_width", "-1", FCVAR_ARCHIVE, "Override world width (0=auto)");
ConVar tfvr_objective_overlay_panel_width("tfvr_objective_overlay_panel_width", "500", FCVAR_ARCHIVE, "Panel capture width in pixels");
ConVar tfvr_objective_overlay_panel_height("tfvr_objective_overlay_panel_height", "700", FCVAR_ARCHIVE, "Panel capture height in pixels");
ConVar tfvr_objective_overlay_panel_x("tfvr_objective_overlay_panel_x", "0", FCVAR_ARCHIVE, "Panel X position within capture area");
ConVar tfvr_objective_overlay_panel_y("tfvr_objective_overlay_panel_y", "0", FCVAR_ARCHIVE, "Panel Y position within capture area");
ConVar tfvr_objective_overlay_wrist_back("tfvr_objective_overlay_wrist_back", "1.5", FCVAR_ARCHIVE, "Distance behind wrist (toward forearm)");
ConVar tfvr_objective_overlay_wrist_up("tfvr_objective_overlay_wrist_up", "-2.0", FCVAR_ARCHIVE, "Distance below wrist bone (negative = below health overlay)");
ConVar tfvr_objective_overlay_wrist_side("tfvr_objective_overlay_wrist_side", "0.2", FCVAR_ARCHIVE, "Distance toward thumb side");
ConVar tfvr_objective_overlay_pitch("tfvr_objective_overlay_pitch", "-90", FCVAR_ARCHIVE, "Pitch rotation (up/down tilt) in degrees");
ConVar tfvr_objective_overlay_yaw("tfvr_objective_overlay_yaw", "-90", FCVAR_ARCHIVE, "Yaw rotation (left/right turn) in degrees");
ConVar tfvr_objective_overlay_roll("tfvr_objective_overlay_roll", "0", FCVAR_ARCHIVE, "Roll rotation (twist) in degrees");
ConVar tfvr_objective_overlay_rotation_x("tfvr_objective_overlay_rotation_x", "90", FCVAR_ARCHIVE, "Additional X rotation (pitch)");
ConVar tfvr_objective_overlay_rotation_y("tfvr_objective_overlay_rotation_y", "90", FCVAR_ARCHIVE, "Additional Y rotation (yaw)");
ConVar tfvr_objective_overlay_rotation_z("tfvr_objective_overlay_rotation_z", "0", FCVAR_ARCHIVE, "Additional Z rotation (roll)");

// Global instance
CVRObjectiveOverlay* g_pVRObjectiveOverlay = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVRObjectiveOverlay::CVRObjectiveOverlay()
{
    m_bInitialized = false;
    m_bEnabled = false;
    m_nAttachedHand = 0; // Default to left hand (same as health overlay typically)
    m_flLastUpdateTime = 0.0f;
    m_pMainObjectivePanel = nullptr;
    
    // Set default offsets (positioned below health overlay)
    m_vQuadOffset.Init(
        tfvr_objective_overlay_offset_x.GetFloat(),
        tfvr_objective_overlay_offset_y.GetFloat(), 
        tfvr_objective_overlay_offset_z.GetFloat()
    );
    m_angQuadRotation.Init(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVRObjectiveOverlay::~CVRObjectiveOverlay()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the overlay system
//-----------------------------------------------------------------------------
bool CVRObjectiveOverlay::Initialize()
{
    if (m_bInitialized)
        return true;

    // NEW APPROACH: No need to create any panels! We'll sample from _rt_vgui instead.
    
    // Get reference to main objective panel 
    m_pMainObjectivePanel = GET_HUDELEMENT(CTFHudObjectiveStatus);
    if (!m_pMainObjectivePanel)
    {
        Warning(_T("VR Objective Overlay: Could not find main CTFHudObjectiveStatus\n"));
        return false;
    }
    
    // DevMsg("CVRObjectiveOverlay: Initialized successfully with render target sampling\n");

    m_bEnabled = tfvr_objective_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_objective_overlay_hand.GetInt();

    m_bInitialized = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown and cleanup
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    // No panels to clean up - we just sampled from the render target!
    m_pMainObjectivePanel = nullptr;
    
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Update each frame - checks for objective changes
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::Update()
{
    if (!m_bInitialized)
        return;

    // Update settings from ConVars
    m_bEnabled = tfvr_objective_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_objective_overlay_hand.GetInt();

    // Update offsets from ConVars
    m_vQuadOffset.Init(
        tfvr_objective_overlay_offset_x.GetFloat(),
        tfvr_objective_overlay_offset_y.GetFloat(),
        tfvr_objective_overlay_offset_z.GetFloat()
    );
    
    // Update additional rotation from ConVars (like health overlay does)
    m_angQuadRotation.Init(
        tfvr_objective_overlay_rotation_x.GetFloat(),
        tfvr_objective_overlay_rotation_y.GetFloat(),
        tfvr_objective_overlay_rotation_z.GetFloat()
    );

    // The objective panel updates automatically via Think()
}

//-----------------------------------------------------------------------------
// Purpose: Render the objective panel in 3D space
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::RenderObjectiveQuad()
{
    VPROF("VR_ObjectiveOverlay_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pMainObjectivePanel)
        return;
        
    // Quick disable for testing
    if (!tfvr_objective_overlay_enabled.GetBool())
        return;
        
    // Safety check: Don't render if there's no valid player or we're in spectator mode
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive())
    {
        return;
    }
    
    // SIMPLE APPROACH: Just render the main panel directly with minimal changes
    // This avoids all the render target sampling complexity
    
    if (!m_pMainObjectivePanel->IsVisible())
    {
        return;
    }
    
    // Calculate panel-to-world transform based on hand position
    VMatrix panelToWorld;
    
    if (tfvr_objective_overlay_simple_transform.GetBool())
    {
        // Simple identity transform for debugging - just put it in front of player
        panelToWorld.Identity();
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            Vector playerPos = pPlayer->EyePosition();
            playerPos += Vector(100, -50, 0); // 100 units in front, 50 units down
            panelToWorld.SetTranslation(playerPos);
        }
    }
    else if (!CalculateQuadTransform(panelToWorld))
    {
        return;
    }
    
    // Get panel dimensions for world size calculation
    int panelWidth, panelHeight;
    m_pMainObjectivePanel->GetSize(panelWidth, panelHeight);
    
    // Use reasonable defaults if panel size is weird
    if (panelWidth <= 0 || panelWidth > 2000) panelWidth = 400;
    if (panelHeight <= 0 || panelHeight > 2000) panelHeight = 200;
    
    // Calculate world size based on scale ConVar
    float scale = tfvr_objective_overlay_scale.GetFloat();
    float aspectRatio = (float)panelWidth / (float)panelHeight;
    float worldWidth = scale * aspectRatio;
    float worldHeight = scale;
    
    // Allow override of world width
    if (tfvr_objective_overlay_world_width.GetFloat() > 0.0f)
    {
        worldWidth = tfvr_objective_overlay_world_width.GetFloat();
    }
    
    // Use DrawPanelIn3DSpace directly - simple and reliable!
    g_pMatSystemSurface->DrawPanelIn3DSpace(
        m_pMainObjectivePanel->GetVPanel(),  // The main objective panel
        panelToWorld,                        // Transform matrix (panel center to world)
        panelWidth,                          // Panel pixel width
        panelHeight,                         // Panel pixel height
        worldWidth,                          // World width (meters)
        worldHeight                          // World height (meters)
    );
}

//-----------------------------------------------------------------------------
// Purpose: Force the objective panel to update by triggering the same logic as Think
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::ForceObjectiveUpdate()
{
    // With render target sampling, we don't need to force updates -
    // the main HUD panel updates automatically and we just sample from it!
}

//-----------------------------------------------------------------------------
// Purpose: Set which hand the overlay is attached to
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::SetHandAttachment(int hand)
{
    m_nAttachedHand = clamp(hand, 0, 1);
}

//-----------------------------------------------------------------------------
// Purpose: Check if objectives should be displayed based on game state
//-----------------------------------------------------------------------------
bool CVRObjectiveOverlay::ShouldDisplayObjectives()
{
    // Don't show if TF2 game rules aren't available
    if (!TFGameRules())
        return false;
        
    // Don't show during match summary
    if (TFGameRules()->ShowMatchSummary())
        return false;
        
    // Don't show in certain game states
    if (TFGameRules()->State_Get() == GR_STATE_PREGAME ||
        TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS)
        return false;
        
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the quad transform matrix based on hand position
//-----------------------------------------------------------------------------
bool CVRObjectiveOverlay::CalculateQuadTransform(VMatrix& quadTransform)
{
    // Check if we should use hand tracking instead of controller
    if (tfvr_objective_overlay_use_hand_tracking.GetBool())
    {
        return CalculateHandTrackingTransform(quadTransform);
    }
    
    // Use controller grip pose (legacy mode)
    if (!g_pOpenXRManager)
    {
        return false;
    }
        
    VMatrix handPose;
    bool handValid = false;
    
    // Get the appropriate hand grip pose
    if (m_nAttachedHand == 0) // Left hand
    {
        if (g_pOpenXRManager->IsLeftControllerPoseValid())
        {
            handValid = g_pOpenXRManager->GetLeftControllerGripPose(handPose);
        }
    }
    else // Right hand
    {
        if (g_pOpenXRManager->IsRightControllerPoseValid())
        {
            handValid = g_pOpenXRManager->GetRightControllerGripPose(handPose);
        }
    }
    
    if (!handValid)
    {
        return false;
    }
    
    // Get hand position and orientation
    Vector handPos = handPose.GetTranslation();
    Vector forward, right, up;
    handPose.GetBasisVectors(forward, right, up);
    
    // Apply offset to position the quad relative to the hand (below health overlay)
    Vector quadPos = handPos + 
                     right * m_vQuadOffset.x + 
                     up * m_vQuadOffset.y + 
                     forward * m_vQuadOffset.z;
    
    // Use the exact controller pose matrix directly - no auto-rotation
    quadTransform = handPose;
    
    // Just update the position to the offset position we calculated
    quadTransform.SetTranslation(quadPos);
    
    // Apply any additional rotation from ConVars if needed
    if (m_angQuadRotation.x != 0 || m_angQuadRotation.y != 0 || m_angQuadRotation.z != 0)
    {
        VMatrix rotationMatrix;
        QAngle totalRotation = m_angQuadRotation;
        totalRotation.x += -90;  // Pitch to stand up from flat
        totalRotation.y += -90;  // Yaw to face backward instead of forward
        totalRotation.z += 90;   // Roll to align widget top with controller top (blue axis)
        
        matrix3x4_t rotMatrix;
        AngleMatrix(totalRotation, Vector(0,0,0), rotMatrix);
        rotationMatrix.CopyFrom3x4(rotMatrix);
        
        // Apply rotation on top of hand pose
        quadTransform = quadTransform * rotationMatrix;
    }
    else
    {
        // Apply rotations for proper orientation (match health overlay exactly)
        VMatrix adjustMatrix;
        matrix3x4_t adjustMatrix3x4;
        AngleMatrix(QAngle(-90, -90, 90), Vector(0,0,0), adjustMatrix3x4);  // Pitch -90 to stand up, Yaw -90 to face backward, Roll 90 to align top
        adjustMatrix.CopyFrom3x4(adjustMatrix3x4);
        
        quadTransform = quadTransform * adjustMatrix;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate transform using hand tracking instead of controller
//-----------------------------------------------------------------------------
bool CVRObjectiveOverlay::CalculateHandTrackingTransform(VMatrix& quadTransform)
{
    if (!g_pOpenXRManager)
    {
        return false;
    }

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
        palmValid = handTracker->GetHandJoint(leftHand, XR_HAND_JOINT_PALM_EXT, palmPosition, palmAngles);
        if (!palmValid)
            return false;
    }

    // Wrist from hand tracking (optional, for improved forward direction)
    Vector wristPosition;
    QAngle wristAngles;
    bool wristValid = handTracker && 
        handTracker->GetHandJoint(leftHand, XR_HAND_JOINT_WRIST_EXT, wristPosition, wristAngles);
    
    // Calculate position below the health overlay
    Vector quadPosition = palmPosition;
    
    // Use wrist-to-palm vector to determine hand orientation if available
    Vector handForward, handRight, handUp;
    if (wristValid)
    {
        // Vector from wrist to palm gives us the hand forward direction (towards fingers)
        handForward = (palmPosition - wristPosition).Normalized();
        
        // Use palm angles to get the proper hand coordinate frame
        AngleVectors(palmAngles, nullptr, &handRight, &handUp);
        
        // Ensure right-handed coordinate system
        if (!leftHand)
        {
            handRight = -handRight; // Right hand uses inverted right vector
        }
    }
    else
    {
        // Fallback to just palm orientation if wrist not available
        AngleVectors(palmAngles, &handForward, &handRight, &handUp);
        if (!leftHand)
        {
            handRight = -handRight; // Right hand uses inverted right vector
        }
    }
    
    // For objective overlay, position below health overlay at wrist
    if (wristValid)
    {
        // Use actual wrist position as base
        quadPosition = wristPosition;
        
        // Position below the health overlay
        Vector wristOffset = -handForward * tfvr_objective_overlay_wrist_back.GetFloat() +    // Behind wrist (toward forearm)
                            handUp * tfvr_objective_overlay_wrist_up.GetFloat() +              // Below wrist bone (negative value)
                            handRight * tfvr_objective_overlay_wrist_side.GetFloat();          // Toward thumb side
        quadPosition += wristOffset;
    }
    else
    {
        // Fallback to palm-based positioning if wrist tracking unavailable
        Vector belowHandOffset = -handForward * 2.0f + handUp * -3.0f; // Position below palm
        quadPosition += belowHandOffset;
    }
    
    // Apply ConVar offsets in hand coordinate space
    Vector userOffset(
        tfvr_objective_overlay_offset_x.GetFloat(),
        tfvr_objective_overlay_offset_y.GetFloat(), 
        tfvr_objective_overlay_offset_z.GetFloat()
    );
    
    // Transform user offset to hand coordinate space
    Vector worldOffset = handRight * userOffset.x + handUp * userOffset.y + handForward * userOffset.z;
    quadPosition += worldOffset;
    
    // Create the quad transform matrix
    quadTransform.Identity();
    quadTransform.SetTranslation(quadPosition);
    
    // Create a matrix from the palm angles
    matrix3x4_t handMatrix;
    AngleMatrix(palmAngles, Vector(0,0,0), handMatrix);
    
    VMatrix handVMatrix;
    handVMatrix.CopyFrom3x4(handMatrix);
    
    // Apply the hand orientation
    quadTransform = quadTransform * handVMatrix;
    
    // Apply rotation for proper orientation
    if (m_angQuadRotation.x != 0 || m_angQuadRotation.y != 0 || m_angQuadRotation.z != 0)
    {
        VMatrix rotationMatrix;
        QAngle totalRotation = m_angQuadRotation;
        // Add orientation from ConVars
        totalRotation.x += tfvr_objective_overlay_pitch.GetFloat();  // Pitch up/down
        totalRotation.y += tfvr_objective_overlay_yaw.GetFloat();    // Yaw left/right
        totalRotation.z += tfvr_objective_overlay_roll.GetFloat();   // Roll twist
        
        matrix3x4_t rotMatrix;
        AngleMatrix(totalRotation, Vector(0,0,0), rotMatrix);
        rotationMatrix.CopyFrom3x4(rotMatrix);
        quadTransform = quadTransform * rotationMatrix;
    }
    else if (!tfvr_objective_overlay_no_rotation.GetBool())
    {
        // Default: flip the widget 180° so it faces the correct direction (match health overlay)
        VMatrix flipMatrix;
        matrix3x4_t flipMatrix3x4;
        AngleMatrix(QAngle(0, 0, 180), Vector(0,0,0), flipMatrix3x4);  // 180° roll to flip
        flipMatrix.CopyFrom3x4(flipMatrix3x4);
        quadTransform = quadTransform * flipMatrix;
    }
     
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reset overlay state (called on map change to clear stale data)
//-----------------------------------------------------------------------------
void CVRObjectiveOverlay::ResetOverlayState()
{
    Msg(_T("VR Objective Overlay: Resetting overlay state due to map change\n"));
    
    // Reset update time to force refresh
    m_flLastUpdateTime = 0.0f;
    
    // Get a fresh reference to the main objective panel
    // (the main objective panel is managed by the HUD system and should reset automatically)
    m_pMainObjectivePanel = GET_HUDELEMENT(CTFHudObjectiveStatus);
    if (!m_pMainObjectivePanel)
    {
        Warning(_T("VR Objective Overlay: Could not find main CTFHudObjectiveStatus after map change\n"));
    }
}

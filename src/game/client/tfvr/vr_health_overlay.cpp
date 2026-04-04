//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR Health Overlay - Renders health panel in 3D space attached to hand
//
//=============================================================================//

#include "cbase.h"
#include "vr_health_overlay.h"
#include "c_tf_player.h"
#include "openxr_manager.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "bitmap/imageformat.h"
#include "view_shared.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "hudelement.h"
#include "tf_hud_playerstatus.h"
#include "vgui/ISurface.h"
#include "KeyValues.h"
#include "ienginevgui.h"
#include "hud.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui/IScheme.h>

// ConVars for configuration
ConVar tfvr_health_overlay_enabled("tfvr_health_overlay_enabled", "1", FCVAR_ARCHIVE, "Enable VR health overlay on hand");
ConVar tfvr_health_overlay_hand("tfvr_health_overlay_hand", "0", FCVAR_ARCHIVE, "Hand to attach health overlay to: 0=left, 1=right");
ConVar tfvr_health_overlay_use_hand_tracking("tfvr_health_overlay_use_hand_tracking", "1", FCVAR_ARCHIVE, "Use hand tracking instead of controller pose: 0=controller, 1=hand tracking");
ConVar tfvr_health_overlay_offset_x("tfvr_health_overlay_offset_x", "-18.5", FCVAR_ARCHIVE, "X offset from hand position");
ConVar tfvr_health_overlay_offset_y("tfvr_health_overlay_offset_y", "0", FCVAR_ARCHIVE, "Y offset from hand position (up)");
ConVar tfvr_health_overlay_offset_z("tfvr_health_overlay_offset_z", "0", FCVAR_ARCHIVE, "Z offset from hand position (forward)");
ConVar tfvr_health_overlay_scale("tfvr_health_overlay_scale", "20", FCVAR_ARCHIVE, "Scale of health overlay");
ConVar tfvr_health_overlay_panel_x("tfvr_health_overlay_panel_x", "0", FCVAR_ARCHIVE, "Panel X position within capture area");
ConVar tfvr_health_overlay_panel_y("tfvr_health_overlay_panel_y", "0", FCVAR_ARCHIVE, "Panel Y position within capture area");
ConVar tfvr_health_overlay_debug_bg("tfvr_health_overlay_debug_bg", "0", FCVAR_ARCHIVE, "Show debug background to see quad boundaries");
ConVar tfvr_health_overlay_simple_transform("tfvr_health_overlay_simple_transform", "0", FCVAR_ARCHIVE, "Use simple identity transform for debugging");
ConVar tfvr_health_overlay_no_rotation("tfvr_health_overlay_no_rotation", "1", FCVAR_ARCHIVE, "Skip final 180-degree rotation for debugging");
ConVar tfvr_health_overlay_world_width("tfvr_health_overlay_world_width", "0", FCVAR_ARCHIVE, "Override world width (0=auto)");
ConVar tfvr_health_overlay_panel_width("tfvr_health_overlay_panel_width", "220", FCVAR_ARCHIVE, "Panel capture width in pixels");
ConVar tfvr_health_overlay_panel_height("tfvr_health_overlay_panel_height", "800", FCVAR_ARCHIVE, "Panel capture height in pixels");

// Global instance
CVRHealthOverlay* g_pVRHealthOverlay = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVRHealthOverlay::CVRHealthOverlay()
{
    m_bInitialized = false;
    m_bEnabled = false;
    m_nAttachedHand = 0; // Default to left hand
    m_flLastUpdateTime = 0.0f;
    m_pMainPlayerStatusPanel = nullptr;
    
    // Set default offsets
    m_vQuadOffset.Init(
        tfvr_health_overlay_offset_x.GetFloat(),
        tfvr_health_overlay_offset_y.GetFloat(), 
        tfvr_health_overlay_offset_z.GetFloat()
    );
    m_angQuadRotation.Init(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVRHealthOverlay::~CVRHealthOverlay()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the overlay system
//-----------------------------------------------------------------------------
bool CVRHealthOverlay::Initialize()
{
    if (m_bInitialized)
        return true;

    // NEW APPROACH: No need to create any panels! Just get reference to main HUD panel
    
    // Get reference to main player status panel (includes health + class icon)
    m_pMainPlayerStatusPanel = GET_HUDELEMENT(CTFHudPlayerStatus);
    if (!m_pMainPlayerStatusPanel)
    {
        Warning(_T("VR Health Overlay: Could not find main CTFHudPlayerStatus\n"));
        return false;
    }
    
    m_bEnabled = tfvr_health_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_health_overlay_hand.GetInt();
    
    m_bInitialized = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown and cleanup
//-----------------------------------------------------------------------------
void CVRHealthOverlay::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    // No panels to clean up - we just reference the main HUD panel!
    m_pMainPlayerStatusPanel = nullptr;
    
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Update each frame - checks for health changes
//-----------------------------------------------------------------------------
void CVRHealthOverlay::Update()
{
    if (!m_bInitialized)
        return;
        
    // Update settings from ConVars
    m_bEnabled = tfvr_health_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_health_overlay_hand.GetInt();
    
    // Update offsets from ConVars
    m_vQuadOffset.Init(
        tfvr_health_overlay_offset_x.GetFloat(),
        tfvr_health_overlay_offset_y.GetFloat(), 
        tfvr_health_overlay_offset_z.GetFloat()
    );
    
    // The main health panel updates automatically via the HUD system
}

//-----------------------------------------------------------------------------
// Purpose: Render the health panel in 3D space
//-----------------------------------------------------------------------------
void CVRHealthOverlay::RenderHealthQuad()
{
    VPROF("VR_HealthOverlay_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pMainPlayerStatusPanel)
        return;
        
    // Quick disable for testing
    if (!tfvr_health_overlay_enabled.GetBool())
        return;
        
    // Safety check: Don't render if there's no valid player or we're in spectator mode
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive())
    {
        return;
    }
    
    // SIMPLE APPROACH: Just render the main panel directly with minimal changes
    // This avoids all the custom panel creation complexity
    
    if (!m_pMainPlayerStatusPanel->IsVisible())
    {
        return;
    }
    
    // Calculate panel-to-world transform based on hand position
    VMatrix panelToWorld;
    
    if (tfvr_health_overlay_simple_transform.GetBool())
    {
        // Simple identity transform for debugging - just put it in front of player
        panelToWorld.Identity();
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            Vector playerPos = pPlayer->EyePosition();
            playerPos += Vector(100, 0, 0); // 100 units in front
            panelToWorld.SetTranslation(playerPos);
        }
    }
    else if (!CalculateQuadTransform(panelToWorld))
    {
        return;
    }
    
    // Get panel dimensions for world size calculation
    int panelWidth, panelHeight;
    m_pMainPlayerStatusPanel->GetSize(panelWidth, panelHeight);
    
    // Use reasonable defaults if panel size is weird
    if (panelWidth <= 0 || panelWidth > 2000) panelWidth = 200;
    if (panelHeight <= 0 || panelHeight > 2000) panelHeight = 200;
    
    // Calculate world size based on scale ConVar
    float scale = tfvr_health_overlay_scale.GetFloat();
    float aspectRatio = (float)panelWidth / (float)panelHeight;
    float worldWidth = scale * aspectRatio;
    float worldHeight = scale;
    
    // Allow override of world width
    if (tfvr_health_overlay_world_width.GetFloat() > 0.0f)
    {
        worldWidth = tfvr_health_overlay_world_width.GetFloat();
    }
    
    // Use DrawPanelIn3DSpace directly - simple and reliable!
    g_pMatSystemSurface->DrawPanelIn3DSpace(
        m_pMainPlayerStatusPanel->GetVPanel(),  // The main player status panel
        panelToWorld,                           // Transform matrix (panel center to world)
        panelWidth,                             // Panel pixel width
        panelHeight,                            // Panel pixel height
        worldWidth,                             // World width (meters)
        worldHeight                             // World height (meters)
    );
}

//-----------------------------------------------------------------------------
// Purpose: Set which hand the overlay is attached to
//-----------------------------------------------------------------------------
void CVRHealthOverlay::SetHandAttachment(int hand)
{
    m_nAttachedHand = clamp(hand, 0, 1);
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the quad transform matrix based on hand position
//-----------------------------------------------------------------------------
bool CVRHealthOverlay::CalculateQuadTransform(VMatrix& quadTransform)
{
    // Check if we should use hand tracking instead of controller
    if (tfvr_health_overlay_use_hand_tracking.GetBool())
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
    
    // Get the appropriate hand grip pose (more natural for overlays)
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
    
    // Apply offset to position the quad relative to the hand
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
        // Apply rotations for proper orientation
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
bool CVRHealthOverlay::CalculateHandTrackingTransform(VMatrix& quadTransform)
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
    
    // Calculate position above the back of the hand
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
    
    // Position the overlay above the back of the hand
    // The "back" of the hand is opposite to the palm direction
    Vector backOfHandOffset = -handForward * 2.0f + handUp * 1.0f;  // 2 units back, 1 unit up
    quadPosition += backOfHandOffset;
    
    // Apply ConVar offsets in hand coordinate space
    Vector userOffset(
        tfvr_health_overlay_offset_x.GetFloat(),
        tfvr_health_overlay_offset_y.GetFloat(), 
        tfvr_health_overlay_offset_z.GetFloat()
    );
    
    // Transform user offset to hand coordinate space
    Vector worldOffset = handRight * userOffset.x + handUp * userOffset.y + handForward * userOffset.z;
    quadPosition += worldOffset;
    
    // Create the quad transform matrix
    quadTransform.Identity();
    quadTransform.SetTranslation(quadPosition);
    
    // Set orientation to face upward from the back of the hand
    // Keep it simple - use the palm orientation directly
    
    // Create a matrix from the palm angles (this was working before)
    matrix3x4_t handMatrix;
    AngleMatrix(palmAngles, Vector(0,0,0), handMatrix);
    
    VMatrix handVMatrix;
    handVMatrix.CopyFrom3x4(handMatrix);
    
    // Apply the hand orientation
    quadTransform = quadTransform * handVMatrix;
    
    // Apply additional rotation to make the widget face upward from the back of the hand
    if (m_angQuadRotation.x != 0 || m_angQuadRotation.y != 0 || m_angQuadRotation.z != 0)
    {
        VMatrix rotationMatrix;
        QAngle totalRotation = m_angQuadRotation;
        
        matrix3x4_t rotMatrix;
        AngleMatrix(totalRotation, Vector(0,0,0), rotMatrix);
        rotationMatrix.CopyFrom3x4(rotMatrix);
        quadTransform = quadTransform * rotationMatrix;
    }
    else if (!tfvr_health_overlay_no_rotation.GetBool())
    {
        // Default: flip the widget 180° so it faces the correct direction
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
void CVRHealthOverlay::ResetOverlayState()
{
    Msg(_T("VR Health Overlay: Resetting overlay state due to map change\n"));
    
    // Reset update time to force refresh
    m_flLastUpdateTime = 0.0f;
    
    // Get a fresh reference to the main player status panel
    // (the main player status panel is managed by the HUD system and should reset automatically)
    m_pMainPlayerStatusPanel = GET_HUDELEMENT(CTFHudPlayerStatus);
    if (!m_pMainPlayerStatusPanel)
    {
        Warning(_T("VR Health Overlay: Could not find main CTFHudPlayerStatus after map change\n"));
    }
}

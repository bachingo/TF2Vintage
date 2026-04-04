#include "cbase.h"
#include "vr_ammo_overlay.h"

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
#include "bitmap/imageformat.h"
#include "view_shared.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "hudelement.h"
#include "KeyValues.h"
#include "ienginevgui.h"
#include "hud.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tf_shareddefs.h"

// Include TF2 headers last, after VGUI types are defined
#include "tf/tf_hud_ammostatus.h"
#include "tf/tf_weaponbase.h"
#include "tfvr/c_tfvr_hand.h"


// ConVars for configuration
ConVar tfvr_ammo_overlay_enabled("tfvr_ammo_overlay_enabled", "1", FCVAR_ARCHIVE, "Enable VR ammo overlay on hand");
ConVar tfvr_ammo_overlay_hand("tfvr_ammo_overlay_hand", "1", FCVAR_ARCHIVE, "Hand to attach ammo overlay to: 0=left, 1=right (should be main shooting hand)");
ConVar tfvr_ammo_overlay_use_hand_tracking("tfvr_ammo_overlay_use_hand_tracking", "2", FCVAR_ARCHIVE, "Use hand tracking instead of controller pose: 0=controller, 1=hand tracking, 2=weapon bone");
ConVar tfvr_ammo_overlay_offset_x("tfvr_ammo_overlay_offset_x", "25.5", FCVAR_ARCHIVE, "X offset from hand position");
ConVar tfvr_ammo_overlay_offset_y("tfvr_ammo_overlay_offset_y", "0", FCVAR_ARCHIVE, "Y offset from hand position (up)");
ConVar tfvr_ammo_overlay_offset_z("tfvr_ammo_overlay_offset_z", "36", FCVAR_ARCHIVE, "Z offset from hand position (forward)");
ConVar tfvr_ammo_overlay_scale("tfvr_ammo_overlay_scale", "2", FCVAR_ARCHIVE, "Scale of ammo overlay");
ConVar tfvr_ammo_overlay_panel_x("tfvr_ammo_overlay_panel_x", "50", FCVAR_ARCHIVE, "Panel X position within capture area");
ConVar tfvr_ammo_overlay_panel_y("tfvr_ammo_overlay_panel_y", "50", FCVAR_ARCHIVE, "Panel Y position within capture area");
ConVar tfvr_ammo_overlay_debug_bg("tfvr_ammo_overlay_debug_bg", "0", FCVAR_ARCHIVE, "Show debug background to see quad boundaries");
ConVar tfvr_ammo_overlay_simple_transform("tfvr_ammo_overlay_simple_transform", "0", FCVAR_ARCHIVE, "Use simple identity transform for debugging");
ConVar tfvr_ammo_overlay_no_rotation("tfvr_ammo_overlay_no_rotation", "0", FCVAR_ARCHIVE, "Skip final 180-degree rotation for debugging");
ConVar tfvr_ammo_overlay_world_width("tfvr_ammo_overlay_world_width", "0", FCVAR_ARCHIVE, "Override world width (0=auto)");
ConVar tfvr_ammo_overlay_panel_width("tfvr_ammo_overlay_panel_width", "1280", FCVAR_ARCHIVE, "Panel capture width in pixels");
ConVar tfvr_ammo_overlay_panel_height("tfvr_ammo_overlay_panel_height", "720", FCVAR_ARCHIVE, "Panel capture height in pixels");
ConVar tfvr_ammo_overlay_wrist_back("tfvr_ammo_overlay_wrist_back", "1.5", FCVAR_ARCHIVE, "Distance behind wrist (toward forearm)");
ConVar tfvr_ammo_overlay_wrist_up("tfvr_ammo_overlay_wrist_up", "0.5", FCVAR_ARCHIVE, "Distance above wrist bone");
ConVar tfvr_ammo_overlay_wrist_side("tfvr_ammo_overlay_wrist_side", "0.3", FCVAR_ARCHIVE, "Distance toward thumb side");
ConVar tfvr_ammo_overlay_pitch("tfvr_ammo_overlay_pitch", "180", FCVAR_ARCHIVE, "Pitch rotation (up/down tilt) in degrees");
ConVar tfvr_ammo_overlay_yaw("tfvr_ammo_overlay_yaw", "0", FCVAR_ARCHIVE, "Yaw rotation (left/right turn) in degrees");
ConVar tfvr_ammo_overlay_roll("tfvr_ammo_overlay_roll", "-10", FCVAR_ARCHIVE, "Roll rotation (twist) in degrees");

// Global instance
CVRAmmoOverlay* g_pVRAmmoOverlay = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVRAmmoOverlay::CVRAmmoOverlay()
{
    m_bInitialized = false;
    m_bEnabled = false;
    m_nAttachedHand = 1; // Default to right hand (main shooting hand for most players)
    m_flLastUpdateTime = 0.0f;
    m_pMainAmmoPanel = nullptr;
    
    // Set default offsets (slightly different from health overlay)
    m_vQuadOffset.Init(
        tfvr_ammo_overlay_offset_x.GetFloat(),
        tfvr_ammo_overlay_offset_y.GetFloat(), 
        tfvr_ammo_overlay_offset_z.GetFloat()
    );
    m_angQuadRotation.Init(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVRAmmoOverlay::~CVRAmmoOverlay()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the overlay system
//-----------------------------------------------------------------------------
bool CVRAmmoOverlay::Initialize()
{
    if (m_bInitialized)
        return true;

    // NEW APPROACH: No need to create any panels! Just get reference to main HUD panel
    
    // Get reference to main ammo panel
    m_pMainAmmoPanel = GET_HUDELEMENT(CTFHudWeaponAmmo);
    if (!m_pMainAmmoPanel)
    {
        Warning(_T("VR Ammo Overlay: Could not find main CTFHudWeaponAmmo\n"));
        return false;
    }
    
    m_bEnabled = tfvr_ammo_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_ammo_overlay_hand.GetInt();

    m_bInitialized = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown and cleanup
//-----------------------------------------------------------------------------
void CVRAmmoOverlay::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    // No panels to clean up - we just reference the main HUD panel!
    m_pMainAmmoPanel = nullptr;
    
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Update each frame - checks for ammo changes
//-----------------------------------------------------------------------------
void CVRAmmoOverlay::Update()
{
    if (!m_bInitialized)
        return;

    // Update settings from ConVars
    m_bEnabled = tfvr_ammo_overlay_enabled.GetBool();
    m_nAttachedHand = tfvr_ammo_overlay_hand.GetInt();

    // Update offsets from ConVars
    m_vQuadOffset.Init(
        tfvr_ammo_overlay_offset_x.GetFloat(),
        tfvr_ammo_overlay_offset_y.GetFloat(),
        tfvr_ammo_overlay_offset_z.GetFloat()
    );

    // The main ammo panel updates automatically via the HUD system
}

//-----------------------------------------------------------------------------
// Purpose: Render the ammo panel in 3D space
//-----------------------------------------------------------------------------
void CVRAmmoOverlay::RenderAmmoQuad()
{
    VPROF("VR_AmmoOverlay_Render");
    
    if (!m_bInitialized || !m_bEnabled || !m_pMainAmmoPanel)
        return;
        
    // Quick disable for testing
    if (!tfvr_ammo_overlay_enabled.GetBool())
        return;
        
    // Safety check: Don't render if there's no valid player or we're in spectator mode
    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive())
    {
        return;
    }
    
    // VR: Don't show ammo overlay if no weapon is equipped (e.g., during loser/stalemate)
    C_TFVRHand* pRightHand = GetLocalPlayerRightHand();
    if (!pRightHand || !pRightHand->GetHeldWeapon())
    {
        return;
    }
    
    // SIMPLE APPROACH: Just render the main panel directly with minimal changes
    // This avoids all the custom panel creation complexity
    
    if (!m_pMainAmmoPanel->IsVisible())
    {
        return;
    }
        
    // Calculate panel-to-world transform based on hand position
    VMatrix panelToWorld;
    
    if (tfvr_ammo_overlay_simple_transform.GetBool())
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
    m_pMainAmmoPanel->GetSize(panelWidth, panelHeight);
    
    // Use reasonable defaults if panel size is weird
    if (panelWidth <= 0 || panelWidth > 2000) panelWidth = 400;
    if (panelHeight <= 0 || panelHeight > 2000) panelHeight = 200;
    
    // Calculate world size based on scale ConVar
    float scale = tfvr_ammo_overlay_scale.GetFloat();
    float aspectRatio = (float)panelWidth / (float)panelHeight;
    float worldWidth = scale * aspectRatio;
    float worldHeight = scale;
    
    // Allow override of world width
    if (tfvr_ammo_overlay_world_width.GetFloat() > 0.0f)
    {
        worldWidth = tfvr_ammo_overlay_world_width.GetFloat();
    }
    
    // Use DrawPanelIn3DSpace directly - simple and reliable!
    g_pMatSystemSurface->DrawPanelIn3DSpace(
        m_pMainAmmoPanel->GetVPanel(),  // The main ammo panel
        panelToWorld,                   // Transform matrix (panel center to world)
        panelWidth,                     // Panel pixel width
        panelHeight,                    // Panel pixel height  
        worldWidth,                     // World width (meters)
        worldHeight                     // World height (meters)
    );
}


//-----------------------------------------------------------------------------
// Purpose: Set which hand the overlay is attached to
//-----------------------------------------------------------------------------
void CVRAmmoOverlay::SetHandAttachment(int hand)
{
    m_nAttachedHand = clamp(hand, 0, 1);
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the quad transform matrix based on hand position
//-----------------------------------------------------------------------------
bool CVRAmmoOverlay::CalculateQuadTransform(VMatrix& quadTransform)
{
	int trackingMode = tfvr_ammo_overlay_use_hand_tracking.GetInt();
	
	// Mode 2: Attach to weapon's weapon_bone
	if (trackingMode == 2)
	{
		return CalculateWeaponBoneTransform(quadTransform);
	}
	// Mode 1: Use hand tracking
	else if (trackingMode == 1)
	{
		return CalculateHandTrackingTransform(quadTransform);
	}
	
	// Mode 0: Use controller grip pose (legacy mode)
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
	
	// Position like a tactical display behind the wrist
	// Instead of using generic offset, position specifically for "pistol grip" viewing
	Vector quadPos = handPos + 
					 right * m_vQuadOffset.x +       // Side offset (slightly toward thumb side)
					 up * m_vQuadOffset.y +          // Up/down (toward wrist)  
					 forward * m_vQuadOffset.z;      // Forward/back (behind wrist)
	
	// Use the exact controller pose matrix directly
	quadTransform = handPose;
	quadTransform.SetTranslation(quadPos);
	
	// Apply rotations for tactical display orientation (face forward/up for pistol grip viewing)
	if (m_angQuadRotation.x != 0 || m_angQuadRotation.y != 0 || m_angQuadRotation.z != 0)
	{
		VMatrix rotationMatrix;
		QAngle totalRotation = m_angQuadRotation;
		// Add tactical display orientation from ConVars
		totalRotation.x += tfvr_ammo_overlay_pitch.GetFloat();  // Pitch: Tilt up/down
		totalRotation.y += tfvr_ammo_overlay_yaw.GetFloat();    // Yaw: Turn left/right
		totalRotation.z += tfvr_ammo_overlay_roll.GetFloat();   // Roll: Twist
		
		matrix3x4_t rotMatrix;
		AngleMatrix(totalRotation, Vector(0,0,0), rotMatrix);
		rotationMatrix.CopyFrom3x4(rotMatrix);
		
		// Apply rotation on top of hand pose
		quadTransform = quadTransform * rotationMatrix;
	}
	else
	{
		// Default tactical display orientation using ConVars
		VMatrix adjustMatrix;
		matrix3x4_t adjustMatrix3x4;
		QAngle tacticalAngles(
			tfvr_ammo_overlay_pitch.GetFloat(),  // Pitch up/down
			tfvr_ammo_overlay_yaw.GetFloat(),    // Yaw left/right
			tfvr_ammo_overlay_roll.GetFloat()    // Roll twist
		);
		AngleMatrix(tacticalAngles, Vector(0,0,0), adjustMatrix3x4);
		adjustMatrix.CopyFrom3x4(adjustMatrix3x4);
		
		quadTransform = quadTransform * adjustMatrix;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Calculate transform using hand tracking instead of controller
//-----------------------------------------------------------------------------
bool CVRAmmoOverlay::CalculateHandTrackingTransform(VMatrix& quadTransform)
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
    
    // For ammo overlay, position at wrist like a tactical display
    if (wristValid)
    {
        // Use actual wrist position as base instead of palm
        quadPosition = wristPosition;
        
        // Position behind the wrist for "pistol grip" style viewing
        Vector wristOffset = -handForward * tfvr_ammo_overlay_wrist_back.GetFloat() +    // Behind wrist (toward forearm)
                            handUp * tfvr_ammo_overlay_wrist_up.GetFloat() +              // Above wrist bone
                            handRight * tfvr_ammo_overlay_wrist_side.GetFloat();          // Toward thumb side
        quadPosition += wristOffset;
    }
    else
    {
        // Fallback to palm-based positioning if wrist tracking unavailable
        Vector backOfHandOffset = -handForward * 2.0f + handUp * 1.0f;
        quadPosition += backOfHandOffset;
    }
    
    // Apply ConVar offsets in hand coordinate space
    Vector userOffset(
        tfvr_ammo_overlay_offset_x.GetFloat(),
        tfvr_ammo_overlay_offset_y.GetFloat(), 
        tfvr_ammo_overlay_offset_z.GetFloat()
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
    
    // Apply tactical display rotation for wrist-mounted viewing
    if (m_angQuadRotation.x != 0 || m_angQuadRotation.y != 0 || m_angQuadRotation.z != 0)
    {
        VMatrix rotationMatrix;
        QAngle totalRotation = m_angQuadRotation;
        // Add tactical display orientation from ConVars
        totalRotation.x += tfvr_ammo_overlay_pitch.GetFloat();  // Pitch up/down
        totalRotation.y += tfvr_ammo_overlay_yaw.GetFloat();    // Yaw left/right
        totalRotation.z += tfvr_ammo_overlay_roll.GetFloat();   // Roll twist
        
        matrix3x4_t rotMatrix;
        AngleMatrix(totalRotation, Vector(0,0,0), rotMatrix);
        rotationMatrix.CopyFrom3x4(rotMatrix);
        quadTransform = quadTransform * rotationMatrix;
    }
    else if (!tfvr_ammo_overlay_no_rotation.GetBool())
    {
        // Default tactical display orientation using ConVars
        VMatrix tacticalMatrix;
        matrix3x4_t tacticalMatrix3x4;
        QAngle tacticalAngles(
            tfvr_ammo_overlay_pitch.GetFloat(),  // Pitch up/down
            tfvr_ammo_overlay_yaw.GetFloat(),    // Yaw left/right
            tfvr_ammo_overlay_roll.GetFloat()    // Roll twist
        );
        AngleMatrix(tacticalAngles, Vector(0,0,0), tacticalMatrix3x4);
        tacticalMatrix.CopyFrom3x4(tacticalMatrix3x4);
        quadTransform = quadTransform * tacticalMatrix;
    }
	 
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate transform using weapon's weapon_bone
//-----------------------------------------------------------------------------
bool CVRAmmoOverlay::CalculateWeaponBoneTransform(VMatrix& quadTransform)
{
	// Get the VR hand
	C_TFVRHand *pHand = (m_nAttachedHand == 0) ? GetLocalPlayerLeftHand() : GetLocalPlayerRightHand();
	if (!pHand)
	{
		return false;
	}
	
	// Use cached weapon bone transform from the hand
	// This avoids bone cache timing issues that cause lag
	matrix3x4_t cachedBoneMatrix;
	if (pHand->GetCachedWeaponBoneTransform(cachedBoneMatrix))
	{
		// Use the cached transform - set during PositionWeaponFromBones
		quadTransform.CopyFrom3x4(cachedBoneMatrix);
	}
	else
	{
		// Fallback to render weapon if cache isn't valid
		C_BaseAnimating *pRenderWeapon = pHand->GetRenderWeapon();
		if (!pRenderWeapon)
		{
			return false;
		}
		
		// Look up the weapon_bone
		int weaponBone = pRenderWeapon->LookupBone("weapon_bone");
		if (weaponBone < 0)
		{
			// No weapon_bone, fallback to weapon origin
			Vector weaponPos = pRenderWeapon->GetAbsOrigin();
			QAngle weaponAngles = pRenderWeapon->GetAbsAngles();
			
			matrix3x4_t weaponMatrix;
			AngleMatrix(weaponAngles, weaponPos, weaponMatrix);
			quadTransform.CopyFrom3x4(weaponMatrix);
		}
		else
		{
			// Get the weapon_bone world transform (may cause lag if cache is stale)
			matrix3x4_t boneMatrix;
			pRenderWeapon->GetBoneTransform(weaponBone, boneMatrix);
			
			// Convert to VMatrix
			quadTransform.CopyFrom3x4(boneMatrix);
		}
	}
	
	// Apply user offsets in weapon space
	Vector userOffset(
		tfvr_ammo_overlay_offset_x.GetFloat(),
		tfvr_ammo_overlay_offset_y.GetFloat(), 
		tfvr_ammo_overlay_offset_z.GetFloat()
	);
	
	if (userOffset.x != 0 || userOffset.y != 0 || userOffset.z != 0)
	{
		// Get weapon basis vectors
		Vector weaponForward, weaponRight, weaponUp;
		quadTransform.GetBasisVectors(weaponForward, weaponRight, weaponUp);
		
		// Apply offset in weapon space
		Vector worldOffset = weaponRight * userOffset.x + 
							weaponUp * userOffset.y + 
							weaponForward * userOffset.z;
		
		Vector currentPos = quadTransform.GetTranslation();
		quadTransform.SetTranslation(currentPos + worldOffset);
	}
	
	// Apply rotation adjustments
	if (!tfvr_ammo_overlay_no_rotation.GetBool())
	{
		VMatrix rotationMatrix;
		matrix3x4_t rotMatrix;
		QAngle rotation(
			tfvr_ammo_overlay_pitch.GetFloat(),
			tfvr_ammo_overlay_yaw.GetFloat(),
			tfvr_ammo_overlay_roll.GetFloat()
		);
		
		AngleMatrix(rotation, Vector(0,0,0), rotMatrix);
		rotationMatrix.CopyFrom3x4(rotMatrix);
		
		quadTransform = quadTransform * rotationMatrix;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Reset overlay state (called on map change to clear stale data)
//-----------------------------------------------------------------------------
void CVRAmmoOverlay::ResetOverlayState()
{
    Msg(_T("VR Ammo Overlay: Resetting overlay state due to map change\n"));
    
    // Reset update time to force refresh
    m_flLastUpdateTime = 0.0f;
    
    // Get a fresh reference to the main ammo panel
    // (the main ammo panel is managed by the HUD system and should reset automatically)
    m_pMainAmmoPanel = GET_HUDELEMENT(CTFHudWeaponAmmo);
    if (!m_pMainAmmoPanel)
    {
        Warning(_T("VR Ammo Overlay: Could not find main CTFHudWeaponAmmo after map change\n"));
    }
}
#include "cbase.h"
#include "openxr_hand_tracking.h"
#include "openxr_manager.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "engine/ivdebugoverlay.h"
#include "mathlib/mathlib.h"
#include "c_baseplayer.h"

// External ConVar from openxr_manager.cpp
extern ConVar tfvr_use_floor_aligned_poses;

// Console variables for hand tracking debug
static ConVar tfvr_hand_tracking_debug("tfvr_hand_tracking_debug", "0", FCVAR_ARCHIVE, "Enable hand tracking debug visualization");
static ConVar tfvr_hand_tracking_cube_size("tfvr_hand_tracking_cube_size", "0.5", FCVAR_ARCHIVE, "Size of debug cubes for hand joints");

// Hand joint names for debugging
static const char* g_handJointNames[XR_HAND_JOINT_COUNT_EXT] = {
    "PALM", "WRIST", 
    "THUMB_METACARPAL", "THUMB_PROXIMAL", "THUMB_DISTAL", "THUMB_TIP",
    "INDEX_METACARPAL", "INDEX_PROXIMAL", "INDEX_INTERMEDIATE", "INDEX_DISTAL", "INDEX_TIP",
    "MIDDLE_METACARPAL", "MIDDLE_PROXIMAL", "MIDDLE_INTERMEDIATE", "MIDDLE_DISTAL", "MIDDLE_TIP",
    "RING_METACARPAL", "RING_PROXIMAL", "RING_INTERMEDIATE", "RING_DISTAL", "RING_TIP",
    "LITTLE_METACARPAL", "LITTLE_PROXIMAL", "LITTLE_INTERMEDIATE", "LITTLE_DISTAL", "LITTLE_TIP"
};

COpenXRHandTracker::COpenXRHandTracker(COpenXRManager* manager)
    : m_manager(manager)
    , m_instance(manager->GetInstance())
    , m_session(manager->GetSession())
    , m_referenceSpace(manager->GetReferenceSpace())
    , m_leftHandTracker(XR_NULL_HANDLE)
    , m_rightHandTracker(XR_NULL_HANDLE)
    , m_pfnCreateHandTrackerEXT(nullptr)
    , m_pfnDestroyHandTrackerEXT(nullptr)
    , m_pfnLocateHandJointsEXT(nullptr)
    , m_handTrackingSupported(false)
    , m_handTrackingInitialized(false)
{
    // Initialize hand data
    memset(&m_leftHandData, 0, sizeof(HandTrackingData));
    memset(&m_rightHandData, 0, sizeof(HandTrackingData));
}

COpenXRHandTracker::~COpenXRHandTracker()
{
    Shutdown();
}

bool COpenXRHandTracker::Initialize()
{
    DevMsg("OpenXR Hand Tracking: Starting initialization...\n");
    
    // Get function pointers for hand tracking extension
    XrResult result = xrGetInstanceProcAddr(m_instance, "xrCreateHandTrackerEXT", 
                                           reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnCreateHandTrackerEXT));
    if (result != XR_SUCCESS || m_pfnCreateHandTrackerEXT == nullptr)
    {
        DevMsg("OpenXR Hand Tracking: xrCreateHandTrackerEXT function not available: %d\n", result);
        return false;
    }
    
    result = xrGetInstanceProcAddr(m_instance, "xrDestroyHandTrackerEXT", 
                                  reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnDestroyHandTrackerEXT));
    if (result != XR_SUCCESS || m_pfnDestroyHandTrackerEXT == nullptr)
    {
        DevMsg("OpenXR Hand Tracking: xrDestroyHandTrackerEXT function not available: %d\n", result);
        return false;
    }
    
    result = xrGetInstanceProcAddr(m_instance, "xrLocateHandJointsEXT", 
                                  reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnLocateHandJointsEXT));
    if (result != XR_SUCCESS || m_pfnLocateHandJointsEXT == nullptr)
    {
        DevMsg("OpenXR Hand Tracking: xrLocateHandJointsEXT function not available: %d\n", result);
        return false;
    }
    
    DevMsg("OpenXR Hand Tracking: Function pointers acquired successfully\n");
    
    // Create hand trackers
    if (!CreateHandTrackers())
    {
        DevMsg("OpenXR Hand Tracking: Failed to create hand trackers\n");
        return false;
    }
    
    m_handTrackingSupported = true;
    m_handTrackingInitialized = true;
    
    DevMsg("OpenXR Hand Tracking: Initialized successfully!\n");
    return true;
}

bool COpenXRHandTracker::CreateHandTrackers()
{
    // Create left hand tracker
    XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
    createInfo.hand = XR_HAND_LEFT_EXT;
    createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
    
    XrResult result = m_pfnCreateHandTrackerEXT(m_session, &createInfo, &m_leftHandTracker);
    if (result != XR_SUCCESS)
    {
        DevMsg("OpenXR Hand Tracking: Failed to create left hand tracker: %d\n", result);
        return false;
    }
    
    // Create right hand tracker
    createInfo.hand = XR_HAND_RIGHT_EXT;
    result = m_pfnCreateHandTrackerEXT(m_session, &createInfo, &m_rightHandTracker);
    if (result != XR_SUCCESS)
    {
        DevMsg("OpenXR Hand Tracking: Failed to create right hand tracker: %d\n", result);
        m_pfnDestroyHandTrackerEXT(m_leftHandTracker);
        m_leftHandTracker = XR_NULL_HANDLE;
        return false;
    }
    
    DevMsg("OpenXR Hand Tracking: Created hand trackers successfully\n");
    return true;
}

void COpenXRHandTracker::Shutdown()
{
    if (m_leftHandTracker != XR_NULL_HANDLE && m_pfnDestroyHandTrackerEXT)
    {
        m_pfnDestroyHandTrackerEXT(m_leftHandTracker);
        m_leftHandTracker = XR_NULL_HANDLE;
    }
    
    if (m_rightHandTracker != XR_NULL_HANDLE && m_pfnDestroyHandTrackerEXT)
    {
        m_pfnDestroyHandTrackerEXT(m_rightHandTracker);
        m_rightHandTracker = XR_NULL_HANDLE;
    }
    
    m_handTrackingInitialized = false;
    DevMsg("OpenXR Hand Tracking: Shut down\n");
}

void COpenXRHandTracker::UpdateHandTracking()
{
    if (!m_handTrackingInitialized)
        return;
    
    // Update left hand
    UpdateHandData(XR_HAND_LEFT_EXT, m_leftHandTracker, m_leftHandData);
    
    // Update right hand
    UpdateHandData(XR_HAND_RIGHT_EXT, m_rightHandTracker, m_rightHandData);
}

void COpenXRHandTracker::UpdateHandData(XrHandEXT hand, XrHandTrackerEXT tracker, HandTrackingData& handData)
{
    if (tracker == XR_NULL_HANDLE)
    {
        handData.isHandTracked = false;
        return;
    }
    
    // Get current frame time
    XrTime currentTime = 0;
    extern void dxvkGetPredictedDisplayTime(XrTime& time);
    dxvkGetPredictedDisplayTime(currentTime);
    
    if (currentTime == 0)
    {
        handData.isHandTracked = false;
        return;
    }
    
    // Prepare hand joint locations query
    XrHandJointLocationEXT jointLocations[XR_HAND_JOINT_COUNT_EXT];
    // Note: XrHandJointLocationEXT doesn't have a type field, it's just data
    
    XrHandJointLocationsEXT locations{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
    locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
    locations.jointLocations = jointLocations;
    
    XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
    locateInfo.baseSpace = m_referenceSpace;
    locateInfo.time = currentTime;
    
    XrResult result = m_pfnLocateHandJointsEXT(tracker, &locateInfo, &locations);
    if (result != XR_SUCCESS)
    {
        handData.isHandTracked = false;
        return;
    }
    
    // Check if hand is actively tracked
    handData.isHandTracked = locations.isActive;
    
    if (!handData.isHandTracked)
        return;
    
    // Store raw joint data (conversion to world space happens on-demand in GetHandJoint)
    for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++)
    {
        const XrHandJointLocationEXT& jointLoc = jointLocations[i];
        HandJointData& jointData = handData.joints[i];
        
        // Check if this joint has valid position and orientation
        XrSpaceLocationFlags requiredFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        jointData.isValid = (jointLoc.locationFlags & requiredFlags) == requiredFlags;
        jointData.isTracked = (jointLoc.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0;
        
        // Store raw pose - DON'T convert to world space yet!
        // This ensures we use the current frame's smoothed data when GetHandJoint() is called
        jointData.rawPose = jointLoc.pose;
    }
}

void COpenXRHandTracker::ConvertXrPoseToSourceFormat(const XrPosef& xrPose, Vector& position, QAngle& angles) const
{
    // *** EXACTLY MATCH CONTROLLER LOGIC FROM GetRightControllerPose() ***
    
    // Get current views for head pose
    const XrView* views = m_manager->GetViews();
    if (!views)
    {
        position = Vector(0, 0, 0);
        angles = QAngle(0, 0, 0);
        return;
    }
    
    // Get the head pose from OpenXR (center eye) - SAME AS CONTROLLERS  
    XrPosef headPose = views[0].pose;
    
    // Create head transform matrix and get its inverse - SAME AS CONTROLLERS
    VMatrix headMatrix = tfvr_use_floor_aligned_poses.GetBool() ? 
        m_manager->ToSourceCoordinateSystemFloorAligned(headPose) : m_manager->ToSourceCoordinateSystem(headPose);
    VMatrix headInverse = headMatrix.InverseTR();
    
    // Convert hand joint pose to Source coordinate system using SAME method as HMD - SAME AS CONTROLLERS
    VMatrix jointMatrix = tfvr_use_floor_aligned_poses.GetBool() ? 
        m_manager->ToSourceCoordinateSystemFloorAligned(xrPose) : m_manager->ToSourceCoordinateSystem(xrPose);
    
    // Transform hand joint to head-relative space, then through player's world transform - SAME AS CONTROLLERS
    // Use original head-relative transformation method
    VMatrix headRelativeJoint = headInverse * jointMatrix;
    
    // NOTE: Controllers apply tfvr_aim_pose_y_correction here, but hand tracking doesn't need that
    
    // Get player's world transform - SAME AS CONTROLLERS
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        // VR FIX: Use the SAME smoothing logic as controllers!
        // Calculate hand joint position RELATIVE to head in playspace,
        // then apply that relative transform to the smoothed head in world space.
        extern CClientVirtualReality g_ClientVirtualReality;
        extern bool UseVR();
        
        if (UseVR())
        {
            // Get raw head pose in playspace (unsmoothed)
            VMatrix rawHeadPlayspace = m_manager->GetMideyePose();
            
            // Calculate hand joint relative to head (both in playspace coordinates)
            VMatrix jointRelativeToHead = rawHeadPlayspace.InverseTR() * jointMatrix;
            
            // Get smoothed head-in-world transform (includes stair/prediction smoothing)
            VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
            
            // Apply the relative transform to the smoothed head position
            VMatrix finalJointPose = smoothedHeadWorld * jointRelativeToHead;
            
            // Extract final position and angles
            position = finalJointPose.GetTranslation();
            MatrixAngles(finalJointPose.As3x4(), angles);
        }
        else
        {
            // Non-VR fallback: use raw player position
            VMatrix playerMatrix;
            playerMatrix.Identity();
            
            matrix3x4_t playerMatrix3x4;
            AngleMatrix(pPlayer->EyeAngles(), playerMatrix3x4);
            playerMatrix.CopyFrom3x4(playerMatrix3x4);
            playerMatrix.SetTranslation(pPlayer->EyePosition());
            
            // Transform hand joint through player's world transform
            VMatrix finalJointPose = playerMatrix * headRelativeJoint;
            
            // Extract final position and angles
            position = finalJointPose.GetTranslation();
            MatrixAngles(finalJointPose.As3x4(), angles);
        }
    }
    else
    {
        // Fallback if no player available
        position = jointMatrix.GetTranslation();
        MatrixAngles(jointMatrix.As3x4(), angles);
    }
}

bool COpenXRHandTracker::GetHandJoint(bool leftHand, XrHandJointEXT joint, Vector& position, QAngle& angles) const
{
    if (!m_handTrackingInitialized || joint >= XR_HAND_JOINT_COUNT_EXT)
        return false;
    
    const HandTrackingData& handData = leftHand ? m_leftHandData : m_rightHandData;
    
    if (!handData.isHandTracked || !handData.joints[joint].isValid)
        return false;
    
    // Convert from raw playspace pose to world coordinates NOW (using current frame's smoothing)
    ConvertXrPoseToSourceFormat(handData.joints[joint].rawPose, position, angles);
    return true;
}

void COpenXRHandTracker::RenderDebugCubes() const
{
    if (!tfvr_hand_tracking_debug.GetBool() || !m_handTrackingInitialized)
        return;
    
    float cubeSize = tfvr_hand_tracking_cube_size.GetFloat();
    
    // Render left hand joints
    if (m_leftHandData.isHandTracked)
    {
        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++)
        {
            const HandJointData& joint = m_leftHandData.joints[i];
            if (joint.isValid)
            {
                // Convert from raw pose to world coordinates NOW (for current frame's smoothing)
                Vector position;
                QAngle angles;
                ConvertXrPoseToSourceFormat(joint.rawPose, position, angles);
                
                Vector boxSize(cubeSize/2, cubeSize/2, cubeSize/2);
                
                // Use blue for left hand - SAME FORMAT AS CONTROLLERS
                debugoverlay->AddBoxOverlay(position, -boxSize, boxSize, angles, 0, 0, 255, 128, 0.016f);
            }
        }
    }
    
    // Render right hand joints
    if (m_rightHandData.isHandTracked)
    {
        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++)
        {
            const HandJointData& joint = m_rightHandData.joints[i];
            if (joint.isValid)
            {
                // Convert from raw pose to world coordinates NOW (for current frame's smoothing)
                Vector position;
                QAngle angles;
                ConvertXrPoseToSourceFormat(joint.rawPose, position, angles);
                
                Vector boxSize(cubeSize/2, cubeSize/2, cubeSize/2);
                
                // Use red for right hand - SAME FORMAT AS CONTROLLERS
                debugoverlay->AddBoxOverlay(position, -boxSize, boxSize, angles, 255, 0, 0, 128, 0.016f);
            }
        }
    }
}

// Console command to toggle hand tracking debug
static void ToggleHandTrackingDebug()
{
    bool currentValue = tfvr_hand_tracking_debug.GetBool();
    tfvr_hand_tracking_debug.SetValue(!currentValue);
    DevMsg("Hand tracking debug visualization: %s\n", !currentValue ? "ENABLED" : "DISABLED");
}

// Console command to debug hand tracking positions and scale
CON_COMMAND(vr_debug_hand_positions, "Debug hand tracking positions and scale")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    COpenXRHandTracker* handTracker = g_pOpenXRManager->GetHandTracker();
    if (!handTracker)
    {
        DevMsg("Hand tracking not available\n");
        return;
    }
    
    DevMsg("=== Hand Tracking vs Controller Comparison ===\n");
    DevMsg("Current world scale: %.1f\n", g_pOpenXRManager->GetWorldScale());
    DevMsg("Cube size: %.1f\n", tfvr_hand_tracking_cube_size.GetFloat());
    
    // Get controller positions for comparison
    VMatrix rightController, leftController;
    bool rightControllerValid = g_pOpenXRManager->GetRightControllerPose(rightController);
    bool leftControllerValid = g_pOpenXRManager->GetLeftControllerPose(leftController);
    
    if (rightControllerValid)
    {
        Vector controllerPos = rightController.GetTranslation();
        DevMsg("\\nRight Controller: (%.1f, %.1f, %.1f)\n", controllerPos.x, controllerPos.y, controllerPos.z);
    }
    else
    {
        DevMsg("\\nRight Controller: NOT TRACKED\n");
    }
    
    if (leftControllerValid)
    {
        Vector controllerPos = leftController.GetTranslation();
        DevMsg("Left Controller: (%.1f, %.1f, %.1f)\n", controllerPos.x, controllerPos.y, controllerPos.z);
    }
    else
    {
        DevMsg("Left Controller: NOT TRACKED\n");
    }
    
    const HandTrackingData& leftHand = handTracker->GetLeftHandData();
    const HandTrackingData& rightHand = handTracker->GetRightHandData();
    
    if (rightHand.isHandTracked)
    {
        DevMsg("\\nRight Hand (tracked):\n");
        Vector palmPos, wristPos;
        QAngle palmAngles, wristAngles;
        if (handTracker->GetHandJoint(false, XR_HAND_JOINT_PALM_EXT, palmPos, palmAngles) &&
            handTracker->GetHandJoint(false, XR_HAND_JOINT_WRIST_EXT, wristPos, wristAngles))
        {
            DevMsg("  Palm: (%.1f, %.1f, %.1f)\n", palmPos.x, palmPos.y, palmPos.z);
            DevMsg("  Wrist: (%.1f, %.1f, %.1f)\n", wristPos.x, wristPos.y, wristPos.z);
            DevMsg("  Distance palm->wrist: %.1f units\n", palmPos.DistTo(wristPos));
            
            // Compare with controller if available
            if (rightControllerValid)
            {
                Vector controllerPos = rightController.GetTranslation();
                DevMsg("  Distance from right controller: %.1f units\n", palmPos.DistTo(controllerPos));
            }
        }
    }
    else
    {
        DevMsg("\\nRight Hand: NOT TRACKED\n");
    }
    
    if (leftHand.isHandTracked)
    {
        DevMsg("\\nLeft Hand (tracked):\n");
        Vector palmPos, wristPos;
        QAngle palmAngles, wristAngles;
        if (handTracker->GetHandJoint(true, XR_HAND_JOINT_PALM_EXT, palmPos, palmAngles) &&
            handTracker->GetHandJoint(true, XR_HAND_JOINT_WRIST_EXT, wristPos, wristAngles))
        {
            DevMsg("  Palm: (%.1f, %.1f, %.1f)\n", palmPos.x, palmPos.y, palmPos.z);
            DevMsg("  Wrist: (%.1f, %.1f, %.1f)\n", wristPos.x, wristPos.y, wristPos.z);
            DevMsg("  Distance palm->wrist: %.1f units\n", palmPos.DistTo(wristPos));
            
            // Compare with controller if available
            if (leftControllerValid)
            {
                Vector controllerPos = leftController.GetTranslation();
                DevMsg("  Distance from left controller: %.1f units\n", palmPos.DistTo(controllerPos));
            }
            
            if (rightHand.isHandTracked)
            {
                Vector rightPalm;
                QAngle rightPalmAngles;
                if (handTracker->GetHandJoint(false, XR_HAND_JOINT_PALM_EXT, rightPalm, rightPalmAngles))
                {
                    DevMsg("  Distance between palms: %.1f units\n", palmPos.DistTo(rightPalm));
                }
            }
        }
    }
    else
    {
        DevMsg("\\nLeft Hand: NOT TRACKED\n");
    }
    
    DevMsg("\\nExpected values:\n");
    DevMsg("- Normal palm->wrist distance: ~2-4 units (at 48x scale)\n");
    DevMsg("- Normal hand span (palm to palm): ~10-20 units (at 48x scale)\n");
    DevMsg("- Hand should be near controller (within ~5-15 units)\n");
    DevMsg("\\nAdjustments:\n");
    DevMsg("- Increase cube size: vr_set_hand_cube_size 10\n");
    DevMsg("- Adjust world scale: tfvr_worldscale 24 (or 12)\n");
    DevMsg("- Enable controller debug: tfvr_controller_debug_draw 1\n");
}

static ConCommand tfvr_toggle_hand_debug("tfvr_toggle_hand_debug", ToggleHandTrackingDebug, "Toggle hand tracking debug visualization");

// Console command to quickly set cube size
CON_COMMAND(vr_set_hand_cube_size, "Set hand tracking debug cube size")
{
    if (args.ArgC() < 2)
    {
        DevMsg("Usage: vr_set_hand_cube_size <size>\n");
        DevMsg("Current cube size: %.1f\n", tfvr_hand_tracking_cube_size.GetFloat());
        DevMsg("Suggested sizes: 1.0 (small), 2.0 (normal), 5.0 (large), 10.0 (huge)\n");
        return;
    }
    
    float newSize = atof(args.Arg(1));
    if (newSize <= 0)
    {
        DevMsg("Cube size must be positive\n");
        return;
    }
    
    tfvr_hand_tracking_cube_size.SetValue(newSize);
    DevMsg("Hand tracking cube size set to: %.1f\n", newSize);
}

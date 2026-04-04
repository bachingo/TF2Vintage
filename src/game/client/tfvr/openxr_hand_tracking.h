#ifndef OPENXR_HAND_TRACKING_H
#define OPENXR_HAND_TRACKING_H

#include "../public/openxr/openxr.h"
#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"
#include <vector>

// Forward declarations
class COpenXRManager;

// Hand tracking joint data
struct HandJointData
{
    XrPosef rawPose;  // Store raw playspace pose instead of converted world position
    bool isValid;
    bool isTracked;
};

// Complete hand data for both hands
struct HandTrackingData
{
    HandJointData joints[XR_HAND_JOINT_COUNT_EXT];
    bool isHandTracked;
};

class COpenXRHandTracker
{
public:
    COpenXRHandTracker(COpenXRManager* manager);
    ~COpenXRHandTracker();

    bool Initialize();
    void Shutdown();
    void UpdateHandTracking();

    // Hand tracking data access
    const HandTrackingData& GetLeftHandData() const { return m_leftHandData; }
    const HandTrackingData& GetRightHandData() const { return m_rightHandData; }
    
    bool IsLeftHandTracked() const { return m_leftHandData.isHandTracked; }
    bool IsRightHandTracked() const { return m_rightHandData.isHandTracked; }
    
    // Get specific joint data
    bool GetHandJoint(bool leftHand, XrHandJointEXT joint, Vector& position, QAngle& angles) const;
    
    // Debug visualization
    void RenderDebugCubes() const;

private:
    bool CreateHandTrackers();
    void UpdateHandData(XrHandEXT hand, XrHandTrackerEXT tracker, HandTrackingData& handData);
    void ConvertXrPoseToSourceFormat(const XrPosef& xrPose, Vector& position, QAngle& angles) const;
    VMatrix ConvertXrPoseToPlayspaceMatrix(const XrPosef& xrPose) const;

    COpenXRManager* m_manager;
    XrInstance m_instance;
    XrSession m_session;
    XrSpace m_referenceSpace;
    
    // Hand tracker handles
    XrHandTrackerEXT m_leftHandTracker;
    XrHandTrackerEXT m_rightHandTracker;
    
    // Hand tracking data
    HandTrackingData m_leftHandData;
    HandTrackingData m_rightHandData;
    
    // OpenXR function pointers
    PFN_xrCreateHandTrackerEXT m_pfnCreateHandTrackerEXT;
    PFN_xrDestroyHandTrackerEXT m_pfnDestroyHandTrackerEXT;
    PFN_xrLocateHandJointsEXT m_pfnLocateHandJointsEXT;
    
    // Extension support flags
    bool m_handTrackingSupported;
    bool m_handTrackingInitialized;
};

#endif // OPENXR_HAND_TRACKING_H

#ifndef OPENXR_MANAGER_H
#define OPENXR_MANAGER_H

#include <vulkan/vulkan.h>

#include "../public/openxr/openxr.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include "../public/openxr/openxr_platform.h"

#include "materialsystem/itexture.h"
#include "vr_integration.h"
#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"
#include "vr_menu_manager.h"
// Forward declaration to avoid circular dependency
class CVRLaserPointer;

class COpenXRInputManager;
class COpenXRHandTracker;

class COpenXRManager : public CAutoGameSystemPerFrame
{
public:
    COpenXRManager();
    ~COpenXRManager();

    bool Initialize();
    void Shutdown();
    void Deactivate(); // Deactivate session without destroying it
    void Reactivate(); // Reactivate existing session
    bool IsActive() const { return m_vrActive; }
    bool IsSessionFocused() const { return m_sessionFocused; }  // False when SteamVR overlay is open

    bool BeginFrame();
    bool EndFrame();

    void UpdateOpenXRViewData();
    bool GetEyeViewLocations(VMatrix& leftEyePose, VMatrix& rightEyePose);

    void Update(float frametime) override;
    
    // Function to recenter the VR view to match the game's camera
    void RecenterView();

    VMatrix CreateVRProjectionMatrix(XrFovf fov, float zNear, float zFar);
    
    // Add method to create and get shared render target
    ITexture* GetSharedRenderTarget();

    IMaterial* GetRenderTargetMat();
    ITexture* GetRenderTarget(int index = -1);

    // Function to adjust viewmodel position for VR
    Vector AdjustViewModelPosition(const Vector& originalPos, const QAngle& headAngles);

    XrSession GetSession() const { return m_session; }
    XrInstance GetInstance() const { return m_instance; }
    XrSpace GetReferenceSpace() const { return m_referenceSpace; }
    XrSpace GetHeadSpace() const { return m_headSpace; }
    XrSpaceLocation GetHeadLocation() const { return m_headLocation; }

    void GetSpectatorScreenDims(uint32_t& width, uint32_t& height);
    void SetSpectatorScreenDims(uint32_t width, uint32_t height);
    bool ConsumeSpectatorDimsChanged();

    void GetEyeProjectionMatrix(VMatrix& pResult, ISourceVirtualReality::VREye eye, float zNear, float zFar);

    VMatrix GetEyeViewFromMidEyeView(ISourceVirtualReality::VREye eye);

    VMatrix GetMideyePose() const;
    Vector GetRawHMDPosition() const; // Raw unscaled position for calibration
    float GetWorldScale() const; // Get current dynamic world scale

    void GetViewportBounds(ISourceVirtualReality::VREye eye, int* pnX, int* pnY, int* pnWidth, int* pnHeight);

    Vector2D GetBufferSize();

    void GetHMDInChaperone(class Vector &origin, QAngle &angles) const;

    // Input handling
    void PollInput();
    bool IsButtonPressed(const char* actionName);
    bool WasButtonPressed(const char* actionName);
    bool WasButtonReleased(const char* actionName);
    float GetAnalogValue(const char* actionName);
    
    // UI interaction with trigger threshold
    bool IsUIInteractionPressed(const char* actionName, float threshold = 0.7f);
    bool WasUIInteractionPressed(const char* actionName, float threshold = 0.7f);
    bool WasUIInteractionReleased(const char* actionName, float threshold = 0.7f);

    // Controller pose tracking
    bool GetLeftControllerPose(VMatrix& pose);
    bool GetRightControllerPose(VMatrix& pose);
    bool GetLeftControllerGripPose(VMatrix& pose);
    bool GetRightControllerGripPose(VMatrix& pose);
    bool IsLeftControllerPoseValid();
    bool IsRightControllerPoseValid();
    
    // Raw controller poses (playspace-relative, no player transforms)
    bool GetLeftControllerPoseRaw(VMatrix& pose);
    bool GetRightControllerPoseRaw(VMatrix& pose);
    
    // Raw XR poses (OpenXR space, for custom transformation)
    bool GetLeftControllerPoseXR(XrPosef& pose);
    bool GetRightControllerPoseXR(XrPosef& pose);
    
    // Properly transformed aim pose (position and direction in world space)
    // Uses same transformation as laser pointer for consistent aiming
    bool GetLeftControllerAimRay(Vector& worldPos, Vector& worldDir);
    bool GetRightControllerAimRay(Vector& worldPos, Vector& worldDir);
    
    bool IsLeftControllerGripPoseValid();
    bool IsRightControllerGripPoseValid();
    
    // Fresh pose sampling (bypasses cache for lowest latency - use for sounds/effects)
    bool SampleFreshRightControllerPose(Vector& worldPos, QAngle& worldAngles);
    bool SampleFreshLeftControllerPose(Vector& worldPos, QAngle& worldAngles);

    // Frame state access
    const XrFrameState& GetFrameState() const { return m_frameState; }
    
    // Input manager access (for debugging)
    COpenXRInputManager* GetInputManager() const { return m_inputManager; }
    
    // Hand tracking access
    COpenXRHandTracker* GetHandTracker() const { return m_handTracker; }
    bool IsHandTrackingSupported() const { return m_handTrackingSupported; }
    
    // Palm pose (grip_surface from OpenXR 1.1/maintenance1, or palm_ext fallback)
    bool IsPalmPoseSupported() const { return m_palmPoseSupported; }
    bool IsGripSurfaceAvailable() const { return m_gripSurfaceAvailable; }
    bool GetLeftPalmPose(VMatrix& pose);
    bool GetRightPalmPose(VMatrix& pose);
    
    // Coordinate conversion functions
    VMatrix ToSourceCoordinateSystem(const XrPosef& pose) const;
    VMatrix ToSourceCoordinateSystemFloorAligned(const XrPosef& pose) const;
    
    // View access (for debugging)
    const XrView* GetViews() const { return m_views; }

private:
    // Helper for computing aim ray from XrPosef
    bool ComputeAimRayFromXRPose(const XrPosef& xrPose, Vector& worldPos, Vector& worldDir);
    
    // OpenXR Resources
    XrInstance m_instance = nullptr;
    XrSession m_session = nullptr;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSpace m_referenceSpace = nullptr;
    XrSpace m_headSpace = nullptr;
    std::vector<XrSwapchain> m_swapchains;
    bool m_vrActive = false;
    bool m_sessionInitialized = false; // Track if session was ever created
    
    bool m_sessionRunning = false;
    bool m_sessionFocused = true;  // True when session has input focus (false when overlay is open)
    
    // Frame-consistent pose tracking: only poll input once per frame to prevent
    // pose changes between game logic and rendering
    int m_lastInputPollFrame = -1;

    // Recentering data
    Quaternion m_recenterQuaternion;  // Stores the quaternion to adjust HMD orientation
    Vector m_recenterPosition;        // Stores position offset for recentering
    bool m_hasRecenterData = false;   // Flag to check if recenter data has been initialized

    // View configuration (added to store recommended resolutions)
    XrViewConfigurationView m_viewConfigs[2] = { {XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW} };

    // Helper functions
    bool CreateOpenXRInstance();
    bool GetSystem();
    bool CreateSession();
    bool CreateReferenceSpace();
    bool CreateHeadSpace();
    void ReleaseResources();

    bool CreateVulkanContext(XrGraphicsRequirementsVulkan2KHR* pRequirements);

    bool m_frameStarted = false;
    XrFrameState m_frameState;
    XrSpace m_viewSpace = XR_NULL_HANDLE;

    XrView* m_views;
    XrSpaceLocation m_headLocation;

    uint32_t m_viewCount;

    uint32_t m_spectatorScreenWidth = 1280;
	uint32_t m_spectatorScreenHeight = 720;
	bool m_bSpectatorDimsChanged = false;

    ITexture* m_pEyeRenderTargets[2] = { nullptr, nullptr };
    ITexture* m_pSharedRenderTarget = nullptr; // Shared render target for both eyes
    bool InitializeSharedRenderTarget(); // Initialize the shared render target

    // OpenXR Function bindings
    PFN_xrGetVulkanGraphicsRequirements2KHR m_pfnGetVulkanGraphicsRequirements2KHR;
    PFN_xrCreateVulkanInstanceKHR m_pfnCreateVulkanInstanceKHR;
    PFN_xrCreateVulkanDeviceKHR m_pfnCreateVulkanDeviceKHR;
    PFN_xrGetVulkanGraphicsDevice2KHR m_pfnGetVulkanGraphicsDevice2KHR;

    // Vulkan related
    VkInstance m_vkInstance;
    VkPhysicalDevice m_vkPhysicalDevice;
    VkDevice m_vkDevice;
    VkQueue m_vkQueue;
    uint32_t m_vkQueueFamilyIndex;

    uint32_t m_currentRenderBufferIndex;

    // Input system
    COpenXRInputManager* m_inputManager;
    
    // Hand tracking system
    COpenXRHandTracker* m_handTracker;
    bool m_handTrackingSupported;
    bool m_palmPoseSupported;
    bool m_gripSurfaceAvailable;
    bool m_maintenance1Enabled;
    
    // VR Menu Manager
    CVRMenuManager* m_menuManager;
    
    // VR Laser Pointer
    CVRLaserPointer* m_laserPointer;
};

static ConVar vr_quat_x_sign("vr_quat_x_sign", "1", FCVAR_ARCHIVE, "Sign for X quaternion component (0=negative, 1=positive)");
static ConVar vr_quat_y_sign("vr_quat_y_sign", "1", FCVAR_ARCHIVE, "Sign for Y quaternion component (0=negative, 1=positive)");
static ConVar vr_quat_z_sign("vr_quat_z_sign", "0", FCVAR_ARCHIVE, "Sign for Z quaternion component (0=negative, 1=positive)");
static ConVar vr_quat_w_sign("vr_quat_w_sign", "0", FCVAR_ARCHIVE, "Sign for W quaternion component (0=negative, 1=positive)");
static ConVar vr_position_scale("vr_position_scale", "50.0", FCVAR_ARCHIVE, "Scales head position movement in VR");
static ConVar vr_floor_offset("vr_floor_offset", "0", FCVAR_ARCHIVE, "Padded pre-scaled distance from floor");
static ConVar vr_follow_game_camera("vr_follow_game_camera", "1", FCVAR_ARCHIVE, "Follow game camera in VR");
static ConVar vr_eye_height_adjust("vr_eye_height_adjust", "0", FCVAR_ARCHIVE, "Adjust eye height in VR");

extern COpenXRManager* g_pOpenXRManager;

#endif // OPENXR_MANAGER_H
#include "cbase.h"
#include "openxr_manager.h"
#include "openxr_input.h"
#include "openxr_hand_tracking.h"
#include "hmdWrapper.h"
#include "vr_rendertargets.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vr_input.h"
#include "iinput.h"
#include "vr_laser_pointer.h"
#include "vr_menu_manager.h"
#include "engine/ivdebugoverlay.h"
#include "c_tf_player.h"
#include "tf_playerclass_shared.h"
#include "const.h"
#include "c_tf_playerclass.h"
#include "client_virtualreality.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

#include "mathlib/mathlib.h"

#include <vector>
#include <cstring>
// Forward declaration for global menu manager pointer
extern class CVRMenuManager* g_pVRMenuManager;

ConVar tfvr_worldscale("tfvr_worldscale", "48", FCVAR_ARCHIVE | FCVAR_REPLICATED, "This scales everything.");
#define METERS_TO_GAME_UNITS tfvr_worldscale.GetFloat()

ConVar tfvr_controller_debug_draw("tfvr_controller_debug_draw", "0", FCVAR_ARCHIVE, "Draw debug visualization for controller positions and orientations");
ConVar tfvr_debug_aim_poses("tfvr_debug_aim_poses", "0", FCVAR_ARCHIVE, "Show debug visualization for aim poses (red/green cubes)");
ConVar tfvr_debug_grip_poses("tfvr_debug_grip_poses", "0", FCVAR_ARCHIVE, "Show debug visualization for grip poses (blue/yellow cubes)");
ConVar tfvr_debug_pose_size("tfvr_debug_pose_size", "0.4", FCVAR_ARCHIVE, "Size of debug pose visualization cubes");
ConVar tfvr_use_floor_aligned_poses("tfvr_use_floor_aligned_poses", "1", FCVAR_ARCHIVE, "Use floor-aligned coordinate conversion for controller poses (1=new method, 0=old method)");
ConVar tfvr_debug_raw_poses("tfvr_debug_raw_poses", "0", FCVAR_ARCHIVE, "Show raw OpenXR poses before any transformation");
ConVar tfvr_pose_offset_x("tfvr_pose_offset_x", "0.0", FCVAR_ARCHIVE, "Manual X offset for controller poses (testing)");
ConVar tfvr_pose_offset_y("tfvr_pose_offset_y", "0.0", FCVAR_ARCHIVE, "Manual Y offset for controller poses (testing)");
ConVar tfvr_pose_offset_z("tfvr_pose_offset_z", "0.0", FCVAR_ARCHIVE, "Manual Z offset for controller poses (testing)");
ConVar tfvr_fix_head_relative_transform("tfvr_fix_head_relative_transform", "0", FCVAR_ARCHIVE, "Use improved head-relative transformation method");
ConVar tfvr_aim_pose_y_correction("tfvr_aim_pose_y_correction", "1.5", FCVAR_ARCHIVE, "Permanent Y-axis correction for aim poses");
ConVar tfvr_crosshair_offset_x("tfvr_crosshair_offset_x", "0.0", FCVAR_ARCHIVE, "Crosshair X-axis offset for fine-tuning aim");
ConVar tfvr_crosshair_offset_y("tfvr_crosshair_offset_y", "0.0", FCVAR_ARCHIVE, "Crosshair Y-axis offset for fine-tuning aim");
ConVar tfvr_crosshair_offset_z("tfvr_crosshair_offset_z", "0.0", FCVAR_ARCHIVE, "Crosshair Z-axis offset for fine-tuning aim");
ConVar tfvr_crosshair_follow_controller_roll("tfvr_crosshair_follow_controller_roll", "1", FCVAR_ARCHIVE, "Make crosshair rotate with controller roll (1=enabled, 0=disabled)");
ConVar tfvr_msaa("tfvr_msaa", "4", FCVAR_ARCHIVE, "Controls multi-sampling anti-aliasing levels in TFVR. Set to the number of samples to use.");
ConVar tfvr_dynamic_worldscale("tfvr_dynamic_worldscale", "1", FCVAR_ARCHIVE, "Enable dynamic world scaling based on merc height and crouch state");
ConVar tfvr_forcemaxlod("tfvr_forcemaxlod", "1", FCVAR_ARCHIVE);
ConVar tfvr_hud_forward("tfvr_hud_forward", "500", FCVAR_ARCHIVE, "Apparent distance of the HUD in inches");
ConVar tfvr_hud_scale("tfvr_hud_scale", "0.5", FCVAR_ARCHIVE);
ConVar tfvr_hud_axis_lock_to_world("tfvr_hud_axis_lock_to_world", "5", FCVAR_ARCHIVE, "Bitfield - locks HUD axes to the world - 1=pitch, 2=yaw, 4=roll");
ConVar tfvr_hud_height_adjust("tfvr_hud_height_adjust", "0", FCVAR_ARCHIVE);

// Height calibration and seated mode ConVars
ConVar tfvr_player_height("tfvr_player_height", "67", FCVAR_ARCHIVE, "Player's real height in inches for VR calibration");
ConVar tfvr_height_calibration("tfvr_height_calibration", "1", FCVAR_ARCHIVE, "Enable height calibration for world scaling");
ConVar tfvr_seated_mode("tfvr_seated_mode", "0", FCVAR_ARCHIVE, "Enable seated VR mode");
ConVar tfvr_seated_height_offset("tfvr_seated_height_offset", "24", FCVAR_ARCHIVE, "Height offset for seated mode in inches");
ConVar tfvr_calibration_debug("tfvr_calibration_debug", "0", FCVAR_ARCHIVE, "Show debug output for height calibration and seated mode");


ConVar tfvr_menu_scale("tfvr_menu_scale", "0.5", FCVAR_ARCHIVE);

static void MirrorResolutionChanged(IConVar *var, const char *pOldValue, float flOldValue);
ConVar tfvr_mirror_resolution("tfvr_mirror_resolution", "720", FCVAR_ARCHIVE,
	"Desktop mirror resolution preset. Values: 720, 1080, 1440, 2160.",
	MirrorResolutionChanged);

static void MirrorResolutionChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	int height = ((ConVar*)var)->GetInt();
	uint32_t w, h;
	switch (height)
	{
	case 2160: w = 3840; h = 2160; break;
	case 1440: w = 2560; h = 1440; break;
	case 1080: w = 1920; h = 1080; break;
	default:   w = 1280; h = 720;  break;
	}

	if (!g_pOpenXRManager)
		return;

	g_pOpenXRManager->SetSpectatorScreenDims(w, h);

	if (g_pOpenXRManager->IsActive() && g_pMatSystemSurface)
	{
		g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget(0, 0, w, h, NULL);

		char szCmd[256];
		Q_snprintf(szCmd, sizeof(szCmd), "mat_setvideomode %u %u 1\n", w, h);
		engine->ClientCmd_Unrestricted(szCmd);
	}
}

ConVar tfvr_r_show_both_eyes("tfvr_r_show_both_eyes", "0", FCVAR_ARCHIVE, "Show both eyes on the game window.");

// Common conversions
namespace
{
	// Calculate dynamic world scale based on merc height, crouch state, and height calibration
	float CalculateDynamicWorldScale()
	{
		// Get the base world scale ConVar
		static ConVar* tfvr_worldscale = cvar->FindVar("tfvr_worldscale");
		float baseWorldScale = tfvr_worldscale ? tfvr_worldscale->GetFloat() : 48.0f;
		
		// Check if dynamic scaling is enabled
		static ConVar* tfvr_dynamic_worldscale = cvar->FindVar("tfvr_dynamic_worldscale");
		if (!tfvr_dynamic_worldscale || !tfvr_dynamic_worldscale->GetBool())
			return baseWorldScale;
		
		// Get local player to check class and crouch state
		C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (!pLocalPlayer)
			return baseWorldScale;
		
		// Get the player's class eye height
		float classEyeHeight = 72.0f; // Default TF2 eye height
		
		// Get player class for height calculation and debug output
		const C_TFPlayerClass* pPlayerClass = pLocalPlayer->GetPlayerClass();
		
		// Only adjust world scale based on class height, not crouch state
		// Crouching will use the normal TF2 animation/view changes
		if (pPlayerClass)
		{
			// Get class-specific eye height
			int classIndex = pPlayerClass->GetClassIndex();
			switch (classIndex)
			{
				case TF_CLASS_SCOUT:
				case TF_CLASS_CIVILIAN:
					classEyeHeight = 65.0f;
					break;
				case TF_CLASS_SNIPER:
				case TF_CLASS_MEDIC:
				case TF_CLASS_HEAVYWEAPONS:
				case TF_CLASS_SPY:
					classEyeHeight = 75.0f;
					break;
				case TF_CLASS_SOLDIER:
				case TF_CLASS_DEMOMAN:
				case TF_CLASS_PYRO:
				case TF_CLASS_ENGINEER:
					classEyeHeight = 68.0f;
					break;
				default:
					classEyeHeight = 72.0f; // Default
					break;
			}
		}
		
		// Calculate base class scale: base scale * (class height / default height)
		float classScale = baseWorldScale * (classEyeHeight / 72.0f);
		
		// Apply height calibration if enabled
		static ConVar* tfvr_height_calibration = cvar->FindVar("tfvr_height_calibration");
		static ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
		static ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
		
		if (tfvr_height_calibration && tfvr_height_calibration->GetBool() && 
		    tfvr_player_height && tfvr_seated_mode)
		{
			// Don't apply height calibration in seated mode - it uses base class scale only
			if (!tfvr_seated_mode->GetBool())
			{
				// Standard height calibration for standing VR
				// Shorter players need BIGGER world scale, taller players need SMALLER world scale
				// This makes the world feel the right size relative to their height
				float playerHeight = tfvr_player_height->GetFloat();
				float averageHeight = 67.0f; // Average adult height in inches
				float heightScale = averageHeight / playerHeight; // INVERTED: shorter = bigger scale
				
				classScale *= heightScale;
			}
		}
		
		float dynamicScale = classScale;
		
		// Debug output with calibration info
		static float lastDebugTime = 0.0f;
		static ConVar* tfvr_calibration_debug = cvar->FindVar("tfvr_calibration_debug");
		if (gpGlobals && gpGlobals->realtime - lastDebugTime > 2.0f && 
		    tfvr_calibration_debug && tfvr_calibration_debug->GetBool()) // Only print every 2 seconds
		{
			bool heightCalibActive = tfvr_height_calibration && tfvr_height_calibration->GetBool();
			bool seatedMode = tfvr_seated_mode && tfvr_seated_mode->GetBool();
			float playerHeight = tfvr_player_height ? tfvr_player_height->GetFloat() : 67.0f;
			
			DevMsg("VR World Scale: Class=%s, EyeHeight=%.1f, Scale=%.1f, HeightCalib=%s, Seated=%s, PlayerHeight=%.1f\n", 
				pPlayerClass ? pPlayerClass->GetName() : "Unknown",
				classEyeHeight,
				dynamicScale,
				heightCalibActive ? "ON" : "OFF",
				seatedMode ? "ON" : "OFF",
				playerHeight);
			lastDebugTime = gpGlobals->realtime;
		}
		
		return dynamicScale;
	}

	VMatrix ConvertFromOpenXRQuatVector(const Quaternion &rot, const Vector &pos)
	{
		VMatrix result;
		result.Identity();

		// OpenXR to Source coordinate conversion:
		// OpenXR: +X=right, +Y=up, +Z=forward
		// Source: +X=forward, +Y=left, +Z=up
		// So: OpenXR.x -> -Source.y (right -> left)
		//     OpenXR.y -> Source.z  (up -> up)
		//     OpenXR.z -> Source.x  (forward -> forward)
		Quaternion convertedRot(-rot.z, -rot.x, rot.y, rot.w);
		Vector convertedPos(-pos.z, -pos.x, pos.y);
		convertedPos *= CalculateDynamicWorldScale();

        matrix3x4_t matrix;
		QuaternionMatrix(convertedRot, convertedPos, matrix);
        result.CopyFrom3x4(matrix);
		return result;
	}

}

void ShutdownOpenXRManager() 
{
    if (g_pOpenXRManager) 
    {
        g_pOpenXRManager->Shutdown();
        delete g_pOpenXRManager;
        g_pOpenXRManager = nullptr;
        DevMsg("OpenXR manager shut down.\n");
    }
}

void InitializeOpenXRManager() 
{
    if (!g_pOpenXRManager) 
    {
        DevMsg("Allocating OpenXR manager...\n");
        g_pOpenXRManager = new COpenXRManager();


        if (!g_pOpenXRManager->Initialize()) 
        {
            DevMsg("OpenXR manager initialization failed!\n");
            ShutdownOpenXRManager();
        }
        else 
        {
            DevMsg("OpenXR manager initialized successfully.\n");
        }
    }
    else 
    {
        DevMsg("OpenXR manager already initialized.\n");
    }
}

COpenXRManager::COpenXRManager() 
{
    m_vrActive = false;
    m_inputManager = nullptr;
    m_handTracker = nullptr;
    m_handTrackingSupported = false;
    m_palmPoseSupported = false;
    m_gripSurfaceAvailable = false;
    m_maintenance1Enabled = false;
    m_menuManager = nullptr;
    m_laserPointer = nullptr;
}

COpenXRManager::~COpenXRManager() 
{
    Shutdown();
}

bool COpenXRManager::Initialize() 
{
    if (m_vrActive) return true;

    // Initialize OpenXR
    if (!CreateOpenXRInstance()) return false;
    if (!GetSystem()) return false;

    XrViewConfigurationView viewConfigs[2] = { {XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW} };
    XrResult result = xrEnumerateViewConfigurationViews(m_instance, m_systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &m_viewCount, m_viewConfigs);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to enumerate view configurations: %d\n", result);
        return false;
    }

    if (!CreateSession()) return false;
    if (!CreateReferenceSpace()) return false;
    if (!CreateHeadSpace()) return false;

    // Initialize input system
    m_inputManager = new COpenXRInputManager(this);
    if (!m_inputManager->Initialize())
    {
        DevMsg("Failed to initialize input system\n");
        delete m_inputManager;
        m_inputManager = nullptr;
        return false;
    }

    // Initialize hand tracking system
    m_handTracker = new COpenXRHandTracker(this);
    if (m_handTracker->Initialize())
    {
        m_handTrackingSupported = true;
        DevMsg("Hand tracking initialized successfully\n");
    }
    else
    {
        DevMsg("Hand tracking initialization failed - continuing without hand tracking\n");
        delete m_handTracker;
        m_handTracker = nullptr;
        m_handTrackingSupported = false;
        // Don't return false - hand tracking is optional
    }

    if (!dxvkInitOpenXR(m_instance, m_systemId, m_session, m_referenceSpace, m_headSpace))
    {
        DevMsg("Failed to send OpenXR info to DXVK");
        return false;
    }

    // Initialize VR Menu Manager
    m_menuManager = new CVRMenuManager();
    m_menuManager->Initialize();

    // Initialize VR Laser Pointer
    m_laserPointer = new CVRLaserPointer();
    m_laserPointer->Initialize();
    // VR Laser Pointer initialized

    // Set the global pointers for external access
    g_pVRMenuManager = m_menuManager;
    g_pVRLaserPointer = m_laserPointer;

    m_vrActive = true;
    
    // Enable VR particle stabilization - particles ignore camera roll for comfort
    ConVarRef vrParticleStabilize( "vr_particle_stabilize_roll" );
    if ( vrParticleStabilize.IsValid() )
    {
        vrParticleStabilize.SetValue( 1 );
    }
    
    // Apply the mirror resolution from whatever config.cfg restored (or the
    // default).  When exec tfvr runs later and changes the value, the ConVar
    // callback will re-apply with the correct resolution.
    extern ConVar tfvr_mirror_resolution;
    MirrorResolutionChanged(&tfvr_mirror_resolution, "720", 720.0f);

    // OpenXR VR mode initialized successfully
    return true;
}

void COpenXRManager::Shutdown() 
{
    if (!m_vrActive && !m_sessionInitialized) return;

    // Clean up VR Menu Manager
    if (m_menuManager)
    {
        m_menuManager->Shutdown();
        delete m_menuManager;
        m_menuManager = nullptr;
    }

    // Clean up VR Laser Pointer
    if (m_laserPointer)
    {
        m_laserPointer->Shutdown();
        delete m_laserPointer;
        m_laserPointer = nullptr;
        g_pVRLaserPointer = nullptr;
    }

    // Clean up input manager
    if (m_inputManager)
    {
        m_inputManager->Shutdown();
        delete m_inputManager;
        m_inputManager = nullptr;
    }

    if (m_handTracker)
    {
        m_handTracker->Shutdown();
        delete m_handTracker;
        m_handTracker = nullptr;
    }

    ReleaseResources();

    m_vrActive = false;
    m_sessionInitialized = false;
    
    // Disable VR particle stabilization
    ConVarRef vrParticleStabilize( "vr_particle_stabilize_roll" );
    if ( vrParticleStabilize.IsValid() )
    {
        vrParticleStabilize.SetValue( 0 );
    }
    
    DevMsg("OpenXR resources cleaned up.\n");
}

void COpenXRManager::Deactivate()
{
    if (!m_vrActive) return;

    DevMsg("Deactivating VR session (keeping all components alive)...\n");

    // DON'T shutdown any components - just mark as inactive
    // All VR components (input, hand tracking, menus, session) stay alive
    // This avoids the "Failed to sync actions" error when reactivating

    // Only clean up the shared render target since it can be recreated quickly
    if (m_pSharedRenderTarget) 
    {
        m_pSharedRenderTarget->DecrementReferenceCount();
        m_pSharedRenderTarget = nullptr;
    }

    m_vrActive = false;
    
    // Disable VR particle stabilization while VR is inactive
    ConVarRef vrParticleStabilize( "vr_particle_stabilize_roll" );
    if ( vrParticleStabilize.IsValid() )
    {
        vrParticleStabilize.SetValue( 0 );
    }
    
    DevMsg("VR session deactivated (all components preserved for instant reactivation)\n");
}

void COpenXRManager::Reactivate()
{
    if (m_vrActive) return;

    if (!m_sessionInitialized || !m_session)
    {
        DevMsg("No existing session to reactivate, calling full Initialize()\n");
        Initialize();
        return;
    }

    DevMsg("Reactivating existing VR session...\n");

    // All components are still alive, we just need to recreate the shared render target
    // and mark as active again

    // Reinitialize shared render target (the only thing we cleaned up)
    if (!InitializeSharedRenderTarget())
    {
        DevMsg("Failed to reinitialize shared render target\n");
        return;
    }

    // All components should still exist - just verify they're still valid
    if (!m_inputManager || !m_menuManager || !m_laserPointer)
    {
        DevMsg("Warning: Some VR components were lost during deactivation, falling back to full Initialize()\n");
        Initialize();
        return;
    }

    // Set the global pointers for external access (in case they were cleared)
    g_pVRMenuManager = m_menuManager;
    g_pVRLaserPointer = m_laserPointer;

    m_vrActive = true;
    
    // Re-enable VR particle stabilization
    ConVarRef vrParticleStabilize( "vr_particle_stabilize_roll" );
    if ( vrParticleStabilize.IsValid() )
    {
        vrParticleStabilize.SetValue( 1 );
    }
    
    extern ConVar tfvr_mirror_resolution;
    MirrorResolutionChanged(&tfvr_mirror_resolution, "720", 720.0f);

    DevMsg("VR session reactivated instantly (all components preserved)\n");
}

bool COpenXRManager::CreateOpenXRInstance() 
{
    // First, enumerate available extensions to check for optional ones
    uint32_t extensionCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    
    std::vector<XrExtensionProperties> availableExtensions(extensionCount);
    for (auto& ext : availableExtensions) {
        ext.type = XR_TYPE_EXTENSION_PROPERTIES;
        ext.next = nullptr;
    }
    xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensions.data());
    
    // Log all available extensions for debugging
    DevMsg("OpenXR: Runtime reports %d available extensions:\n", extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        DevMsg("  [%d] %s (v%d)\n", i, availableExtensions[i].extensionName, availableExtensions[i].extensionVersion);
    }
    
    // Specifically check for render model extensions
    DevMsg("OpenXR: Looking for extension '%s'\n", XR_EXT_RENDER_MODEL_EXTENSION_NAME);
    DevMsg("OpenXR: Looking for extension '%s'\n", XR_EXT_INTERACTION_RENDER_MODEL_EXTENSION_NAME);
    
    // Helper to check if an extension is available
    auto isExtensionAvailable = [&](const char* name) -> bool {
        for (const auto& ext : availableExtensions) {
            if (strcmp(ext.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    };
    
    // Build list of extensions to enable
    std::vector<const char*> extensionsToEnable;
    
    // Required extension - Vulkan graphics binding
    bool hasVulkanEnable2 = isExtensionAvailable(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    if (!hasVulkanEnable2) {
        DevMsg("OpenXR: CRITICAL - %s not available! VR cannot initialize.\n", XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
        return false;
    }
    extensionsToEnable.push_back(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    
    // Optional: Hand tracking
    bool hasHandTracking = isExtensionAvailable(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    if (hasHandTracking) {
        extensionsToEnable.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
        DevMsg("OpenXR: Hand tracking extension available and enabled\n");
    } else {
        DevMsg("OpenXR: Hand tracking extension not available\n");
    }
    
    // Optional: KHR_maintenance1 (backports OpenXR 1.1 features like grip_surface to 1.0 apps)
    m_maintenance1Enabled = isExtensionAvailable(XR_KHR_MAINTENANCE1_EXTENSION_NAME);
    if (m_maintenance1Enabled) {
        extensionsToEnable.push_back(XR_KHR_MAINTENANCE1_EXTENSION_NAME);
        DevMsg("OpenXR: KHR_maintenance1 available - grip_surface pose enabled\n");
    }
    
    // Optional: Palm pose (fallback for runtimes without maintenance1/1.1)
    bool palmExtAvailable = isExtensionAvailable(XR_EXT_PALM_POSE_EXTENSION_NAME);
    if (palmExtAvailable) {
        extensionsToEnable.push_back(XR_EXT_PALM_POSE_EXTENSION_NAME);
        DevMsg("OpenXR: Palm pose extension available and enabled\n");
    }
    
    // Determine palm pose strategy: prefer grip_surface (1.1 core), fall back to palm_ext
    m_gripSurfaceAvailable = m_maintenance1Enabled;
    m_palmPoseSupported = m_gripSurfaceAvailable || palmExtAvailable;
    if (m_palmPoseSupported) {
        DevMsg("OpenXR: Palm pose will use %s path\n", 
               m_gripSurfaceAvailable ? "grip_surface (via maintenance1)" : "palm_ext");
    } else {
        DevMsg("OpenXR: No palm pose support - will fall back to hand tracking or grip pose\n");
    }
    
    // Optional: XR_EXT_uuid (required by render model extension)
    bool hasUuidExt = isExtensionAvailable(XR_EXT_UUID_EXTENSION_NAME);
    if (hasUuidExt) {
        extensionsToEnable.push_back(XR_EXT_UUID_EXTENSION_NAME);
        DevMsg("OpenXR: UUID extension available and enabled\n");
    }
    
    // Optional render model extensions (require XR_EXT_uuid)
    bool hasRenderModelExt = isExtensionAvailable(XR_EXT_RENDER_MODEL_EXTENSION_NAME);
    bool hasInteractionRenderModelExt = isExtensionAvailable(XR_EXT_INTERACTION_RENDER_MODEL_EXTENSION_NAME);
    
    if (hasUuidExt && hasRenderModelExt && hasInteractionRenderModelExt) {
        extensionsToEnable.push_back(XR_EXT_RENDER_MODEL_EXTENSION_NAME);
        extensionsToEnable.push_back(XR_EXT_INTERACTION_RENDER_MODEL_EXTENSION_NAME);
        DevMsg("OpenXR: Render model extensions available and enabled\n");
    } else {
        DevMsg("OpenXR: Render model extensions not available or missing dependencies (uuid=%d, renderModel=%d, interactionRenderModel=%d)\n", 
               hasUuidExt, hasRenderModelExt, hasInteractionRenderModelExt);
    }
    
    // Log all extensions we're requesting
    DevMsg("OpenXR: Requesting %d extensions:\n", (int)extensionsToEnable.size());
    for (const char* ext : extensionsToEnable) {
        DevMsg("  - %s\n", ext);
    }
    
    XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsToEnable.size());
    createInfo.enabledExtensionNames = extensionsToEnable.data();
    // Use OpenXR 1.0.0 API version - SteamVR doesn't support 1.1 yet
    // We can still use 1.1 extensions if the runtime advertises them
    createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    strcpy_s(createInfo.applicationInfo.applicationName, "TF2VR");
    strcpy_s(createInfo.applicationInfo.engineName, "Source 2013");
    
    XrResult result = xrCreateInstance(&createInfo, &m_instance);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create OpenXR instance: %d\n", result);
        return false;
    }
    
    // Load extension function pointers right after instance creation
    result = xrGetInstanceProcAddr
    (
        m_instance,
        "xrGetVulkanGraphicsRequirements2KHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsRequirements2KHR)
    );
    
    if (result != XR_SUCCESS || m_pfnGetVulkanGraphicsRequirements2KHR == nullptr) 
    {
        DevMsg("Failed to get xrGetVulkanGraphicsRequirements2KHR function: %d\n", result);
        return false;
    }
    
    // Other Vulkan-related function pointers
    result = xrGetInstanceProcAddr
    (
        m_instance,
        "xrCreateVulkanInstanceKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnCreateVulkanInstanceKHR)
    );
    
    if (result != XR_SUCCESS || m_pfnCreateVulkanInstanceKHR == nullptr) 
    {
        DevMsg("Failed to get xrCreateVulkanInstanceKHR function: %d\n", result);
        return false;
    }
    
    result = xrGetInstanceProcAddr
    (
        m_instance,
        "xrCreateVulkanDeviceKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnCreateVulkanDeviceKHR)
    );
    
    if (result != XR_SUCCESS || m_pfnCreateVulkanDeviceKHR == nullptr) 
    {
        DevMsg("Failed to get xrCreateVulkanDeviceKHR function: %d\n", result);
        return false;
    }

    result = xrGetInstanceProcAddr
    (
        m_instance,
        "xrGetVulkanGraphicsDevice2KHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsDevice2KHR)
    );

    if (result != XR_SUCCESS || m_pfnGetVulkanGraphicsDevice2KHR == nullptr) 
    {
        DevMsg("Failed to get xrGetVulkanGraphicsDevice2KHR function: %d\n", result);
        return false;
    }
    
    DevMsg("OpenXR instance created with Vulkan 2 support!\n");
    return true;
}

bool COpenXRManager::GetSystem() 
{
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrResult result = xrGetSystem(m_instance, &systemInfo, &m_systemId);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to get VR system: %d\n", result);
        return false;
    }
    DevMsg("VR system retrieved with ID: %llu\n", m_systemId);
    return true;
}

bool COpenXRManager::CreateSession() 
{
    if (!m_pfnGetVulkanGraphicsRequirements2KHR) 
    {
        DevMsg("Vulkan graphics requirements function not available\n");
        return false;
    }
    
    // Get OpenXR's Vulkan requirements BEFORE creating the Vulkan instance
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    XrResult result = m_pfnGetVulkanGraphicsRequirements2KHR(m_instance, m_systemId, &graphicsRequirements);
    if (result != XR_SUCCESS) 
    {
        DevMsg("xrGetVulkanGraphicsRequirements2KHR failed: %d\n", result);
        return false;
    }
    
    // Try to use DXVK's existing VkDevice for the OpenXR session binding.
    bool usedDxvkDevice = false;
    if (dxvkGetVulkanDeviceInfo(&m_vkInstance, &m_vkPhysicalDevice, &m_vkDevice, &m_vkQueue, &m_vkQueueFamilyIndex))
    {
        DevMsg("VR: Using DXVK's Vulkan device for OpenXR session (shared device mode)\n");
        usedDxvkDevice = true;
    }
    else
    {
        DevMsg("VR: DXVK Vulkan device not available, creating dedicated Vulkan context\n");
        if (!CreateVulkanContext(&graphicsRequirements)) 
        {
            DevMsg("Vulkan context creation failed!\n");
            return false;
        }
    }
   
    // Set up Vulkan graphics binding - use the Vulkan2KHR version 
    XrGraphicsBindingVulkan2KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };
    graphicsBinding.instance = m_vkInstance;
    graphicsBinding.physicalDevice = m_vkPhysicalDevice;
    graphicsBinding.device = m_vkDevice;
    graphicsBinding.queueFamilyIndex = m_vkQueueFamilyIndex;
    graphicsBinding.queueIndex = 0;

    // Create OpenXR session
    XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = m_systemId;
    sessionInfo.createFlags = 0;

    result = xrCreateSession(m_instance, &sessionInfo, &m_session);
    if (!XR_SUCCEEDED(result)) 
    {
        if (usedDxvkDevice)
        {
            DevMsg("VR: Session creation failed with DXVK device (error %d), falling back to dedicated Vulkan context\n", result);
            if (!CreateVulkanContext(&graphicsRequirements))
            {
                DevMsg("Vulkan context creation failed!\n");
                return false;
            }
            graphicsBinding.instance = m_vkInstance;
            graphicsBinding.physicalDevice = m_vkPhysicalDevice;
            graphicsBinding.device = m_vkDevice;
            graphicsBinding.queueFamilyIndex = m_vkQueueFamilyIndex;
            
            result = xrCreateSession(m_instance, &sessionInfo, &m_session);
            if (!XR_SUCCEEDED(result))
            {
                DevMsg("Failed to create OpenXR session (fallback): %d\n", result);
                return false;
            }
            DevMsg("VR: OpenXR session created with dedicated Vulkan context (fallback)\n");
        }
        else
        {
            DevMsg("Failed to create OpenXR session: %d\n", result);
            return false;
        }
    }
    else if (usedDxvkDevice)
    {
        DevMsg("VR: OpenXR session created successfully with DXVK's shared Vulkan device\n");
    }
    DevMsg("OpenXR session created!\n");

    // Per OpenXR spec, we must wait for XR_SESSION_STATE_READY before calling
    // xrBeginSession. Poll events until we see the READY state or time out.
    bool sessionReady = false;
    const int maxPollAttempts = 200;  // ~2 seconds at 10ms intervals
    for (int attempt = 0; attempt < maxPollAttempts && !sessionReady; attempt++)
    {
        XrEventDataBuffer eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
        while (xrPollEvent(m_instance, &eventBuffer) == XR_SUCCESS)
        {
            if (eventBuffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
            {
                auto* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventBuffer);
                DevMsg("VR: Session state changed to %d during init\n", stateEvent->state);
                if (stateEvent->state == XR_SESSION_STATE_READY)
                {
                    sessionReady = true;
                    break;
                }
            }
            eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
        }
        if (!sessionReady)
        {
            ThreadSleep(10);
        }
    }
    
    if (!sessionReady)
    {
        DevMsg("VR: WARNING - Did not receive XR_SESSION_STATE_READY after polling, attempting xrBeginSession anyway\n");
    }

    XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    result = xrBeginSession(m_session, &beginInfo);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to begin OpenXR session: %d\n", result);
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
        return false;
    }
    DevMsg("OpenXR session started successfully!\n");
	m_sessionRunning = true;
    m_sessionInitialized = true;
    return true;
}

bool COpenXRManager::CreateReferenceSpace() 
{
    // Try to create a STAGE reference space first (floor-aligned)
    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceInfo.poseInReferenceSpace = { {0, 0, 0, 1}, {0, 0, 0} };
    XrResult result = xrCreateReferenceSpace(m_session, &spaceInfo, &m_referenceSpace);
    
    if (!XR_SUCCEEDED(result))
    {
        // Fall back to LOCAL if STAGE is not supported
        DevMsg("STAGE reference space not supported, falling back to LOCAL: %d\n", result);
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        result = xrCreateReferenceSpace(m_session, &spaceInfo, &m_referenceSpace);
        
        if (!XR_SUCCEEDED(result))
        {
            DevMsg("Failed to create reference space: %d\n", result);
            return false;
        }
        DevMsg("LOCAL reference space created successfully\n");
    }
    else
    {
        DevMsg("STAGE reference space created successfully (floor-aligned)\n");
    }
    
    return true;
}

bool COpenXRManager::CreateHeadSpace()
{
    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceInfo.poseInReferenceSpace = { {0, 0, 0, 1}, {0, 0, 0} };
    XrResult result = xrCreateReferenceSpace(m_session, &spaceInfo, &m_headSpace);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create head space: %d\n", result);
        return false;
    }
    DevMsg("Head space created successfully!\n");
    return true;
}

void COpenXRManager::ReleaseResources() 
{
    // Clean up shared render target
    if (m_pSharedRenderTarget) 
    {
        m_pSharedRenderTarget->DecrementReferenceCount();
        m_pSharedRenderTarget = nullptr;
    }

    for (auto& swapchain : m_swapchains) 
    {
        xrDestroySwapchain(swapchain);
    }

    if (m_viewSpace) xrDestroySpace(m_viewSpace);
    if (m_referenceSpace) xrDestroySpace(m_referenceSpace);
    if (m_headSpace) xrDestroySpace(m_headSpace);
    if (m_session) xrDestroySession(m_session);
    if (m_instance) xrDestroyInstance(m_instance);
}

bool COpenXRManager::BeginFrame()
{
    VPROF_BUDGET("OpenXRManager::BeginFrame", VPROF_BUDGETGROUP_WORLD_RENDERING);
    return dxvkBeginFrame();
}

bool COpenXRManager::EndFrame()
{
    VPROF_BUDGET("OpenXRManager::EndFrame", VPROF_BUDGETGROUP_WORLD_RENDERING);
    return dxvkEndFrame();
}

bool COpenXRManager::InitializeSharedRenderTarget()
{
    // Clean up existing shared render target
    if (m_pSharedRenderTarget) 
    {
        m_pSharedRenderTarget->DecrementReferenceCount();
        m_pSharedRenderTarget = nullptr;
    }

    // Get width and height from the view configs
    uint32_t width = m_viewConfigs[0].recommendedImageRectWidth;
    uint32_t height = m_viewConfigs[0].recommendedImageRectHeight;

    // Create a shared render target with double the width for both eyes
    char rtName[64];
    V_sprintf_safe(rtName, "_rt_VRSharedEyes");

    materials->BeginRenderTargetAllocation();
    
    m_pSharedRenderTarget = materials->CreateNamedRenderTargetTextureEx
    (
        rtName,
        width * 2, height,  // Double width to fit both eyes
        RT_SIZE_NO_CHANGE,
        IMAGE_FORMAT_BGRA8888,
        MATERIAL_RT_DEPTH_SHARED,
        TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_SRGB
    );

    materials->EndRenderTargetAllocation();
   
    
    if (!m_pSharedRenderTarget)
    {
        Warning("Failed to create shared render target for VR eyes\n");
        return false;
    }
    
    return true;
}

ITexture* COpenXRManager::GetSharedRenderTarget()
{
    // If the shared render target doesn't exist yet, create it
    if (!m_pSharedRenderTarget) 
    {
        if (!InitializeSharedRenderTarget()) 
        {
            return nullptr;
        }
    }
    
    return m_pSharedRenderTarget;
}

IMaterial* renderTargetMaterials[VR_NUM_BUFFERS];
IMaterial* COpenXRManager::GetRenderTargetMat()
{
	if (!renderTargetMaterials[m_currentRenderBufferIndex])
	{
		char name[256];
		sprintf(name, "vr_target_material_%d", m_currentRenderBufferIndex);

		KeyValues *keys = new KeyValues("UnlitGeneric");
		keys->SetInt("$ignorez", 1);
		keys->SetInt("$nocull", 1);
		keys->SetString("$basetexture", backBufferNamePerIndex(m_currentRenderBufferIndex));

		renderTargetMaterials[m_currentRenderBufferIndex] = materials->CreateMaterial(name, keys);
		Assert(!IsErrorMaterial(renderTargetMaterials[m_currentRenderBufferIndex]));
	}

	return renderTargetMaterials[m_currentRenderBufferIndex];
}

ITexture* COpenXRManager::GetRenderTarget(int index)
{
	if (index < 0)
		index = m_currentRenderBufferIndex;
	index = index % VR_NUM_BUFFERS;
	return vrRenderTargets->GetVRRenderTarget(index);
}

void COpenXRManager::GetSpectatorScreenDims(uint32_t &width, uint32_t &height)
{
	width = m_spectatorScreenWidth;
	height = m_spectatorScreenHeight;
}

void COpenXRManager::SetSpectatorScreenDims(uint32_t width, uint32_t height)
{
	if (m_spectatorScreenWidth == width && m_spectatorScreenHeight == height)
		return;

	m_spectatorScreenWidth = width;
	m_spectatorScreenHeight = height;
	m_bSpectatorDimsChanged = true;
	Msg("VR: Mirror resolution set to %ux%u\n", width, height);
}

bool COpenXRManager::ConsumeSpectatorDimsChanged()
{
	if (!m_bSpectatorDimsChanged)
		return false;
	m_bSpectatorDimsChanged = false;
	return true;
}

void COpenXRManager::UpdateOpenXRViewData()
{
    VPROF("OpenXRManager::UpdateViewData");
    
    if (!m_vrActive || !m_session)
    {
        return;
    }

    // CRITICAL: Only poll input ONCE per frame to ensure pose consistency
    // between game logic (effects, sounds) and rendering (visuals).
    // If we poll multiple times, poses can change mid-frame causing effects
    // to lag behind the visual hand/weapon position.
    int currentFrame = gpGlobals ? gpGlobals->framecount : 0;
    bool bFirstPollThisFrame = (m_lastInputPollFrame != currentFrame);
    
    if (bFirstPollThisFrame)
    {
        if (m_inputManager)
        {
            m_inputManager->PollInput();
            // Note: Aim poses are now sampled directly by compositor via dxvkSetAimSpaces()
        }

        // Update hand tracking (debug rendering moved to view.cpp after smoothing is applied)
        if (m_handTracker)
        {
            m_handTracker->UpdateHandTracking();
        }
        
        m_lastInputPollFrame = currentFrame;
    }

    uint32_t viewCount;
    dxvkGetViews(m_views, m_headLocation, viewCount);
}

bool COpenXRManager::GetEyeViewLocations(VMatrix& leftEyePose, VMatrix& rightEyePose)
{
    VPROF("OpenXRManager::GetEyeViewLocations");
    
    if (!m_vrActive || !m_session)
    {
        return false;
    }

    leftEyePose = this->ToSourceCoordinateSystemFloorAligned(m_views[0].pose);
    rightEyePose = this->ToSourceCoordinateSystemFloorAligned(m_views[1].pose);
    return true;
}


VMatrix COpenXRManager::CreateVRProjectionMatrix(XrFovf fov, float zNear, float zFar)
{
    VMatrix mat;
    mat.Identity();

    // OpenXR FOV angles are already in radians
    float tanLeft = zNear * tanf(fov.angleLeft);
    float tanRight = zNear * tanf(fov.angleRight);
    float tanDown = zNear * tanf(fov.angleDown);
    float tanUp = zNear * tanf(fov.angleUp);

    float tanWidth = tanRight - tanLeft;
    float tanHeight = tanUp - tanDown;

    // Compute scale and offset terms with near plane factor
    float xScale = (2.0f * zNear) / tanWidth;      // X scale
    float yScale = (2.0f * zNear) / tanHeight;     // Y scale
    float xOffset = (tanLeft + tanRight) / tanWidth;  // X offset
    float yOffset = (tanDown + tanUp) / tanHeight;    // Y offset

    // Compute depth terms
    float zScale = -(zFar + zNear) / (zFar - zNear);
    float zOffset = (-2.0f * zFar * zNear) / (zFar - zNear);

    // Populate the matrix (assuming VMatrix is row-major: mat[row][col])
    mat[0][0] = xScale;    // X scale
    mat[1][1] = yScale;    // Y scale
    mat[0][2] = xOffset;   // X offset
    mat[1][2] = yOffset;   // Y offset
    mat[2][2] = zScale;    // Z scale
    mat[2][3] = zOffset;   // Z offset
    mat[3][2] = -1.0f;     // Perspective term
    mat[3][3] = 0.0f;      // Homogeneous coordinate

    /*
    // Log input values
    DevMsg("VR Projection Matrix Input:\n");
    DevMsg("  FOV (rad): L=%.3f, R=%.3f, D=%.3f, U=%.3f\n", fov.angleLeft, fov.angleRight, fov.angleDown, fov.angleUp);
    DevMsg("  FOV (deg): L=%.1f, R=%.1f, D=%.1f, U=%.1f\n", RAD2DEG(fov.angleLeft), RAD2DEG(fov.angleRight), RAD2DEG(fov.angleDown), RAD2DEG(fov.angleUp));
    DevMsg("  zNear=%.3f, zFar=%.3f\n", zNear, zFar);

    // Log computed values
    DevMsg("Computed values:\n");
    DevMsg("  tanLeft=%.3f, tanRight=%.3f, tanDown=%.3f, tanUp=%.3f\n", tanLeft, tanRight, tanDown, tanUp);
    DevMsg("  xScale=%.3f, yScale=%.3f\n", xScale, yScale);
    DevMsg("  xOffset=%.3f, yOffset=%.3f\n", xOffset, yOffset);
    DevMsg("  zScale=%.3f, zOffset=%.3f\n", zScale, zOffset);

    // Log resulting matrix
    DevMsg("Projection Matrix:\n");
    DevMsg("  [%.3f %.3f %.3f %.3f]\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
    DevMsg("  [%.3f %.3f %.3f %.3f]\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
    DevMsg("  [%.3f %.3f %.3f %.3f]\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
    DevMsg("  [%.3f %.3f %.3f %.3f]\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    */

    return mat;
}

void COpenXRManager::GetEyeProjectionMatrix(VMatrix& pResult, ISourceVirtualReality::VREye eye, float zNear, float zFar)
{
    XrFovf fov = m_views[eye].fov;
    // Use a smaller zNear value for better VR near plane
    pResult = CreateVRProjectionMatrix(fov, zNear, zFar);
}

bool COpenXRManager::CreateVulkanContext(XrGraphicsRequirementsVulkan2KHR* pRequirements) {
    if (!m_pfnCreateVulkanInstanceKHR || !m_pfnCreateVulkanDeviceKHR) 
    {
        DevMsg("Vulkan creation functions not available\n");
        return false;
    }
    
    // Let OpenXR create the Vulkan instance for us
    VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = pRequirements->minApiVersionSupported;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pApplicationName = "TF2VR";
    appInfo.pEngineName = "Source Engine";
    instanceCreateInfo.pApplicationInfo = &appInfo;
    
    // Let OpenXR create the Vulkan instance with proper extensions
    XrVulkanInstanceCreateInfoKHR createInfo{ XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
    createInfo.systemId = m_systemId;
    createInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    createInfo.vulkanCreateInfo = &instanceCreateInfo;
    createInfo.vulkanAllocator = nullptr;
    
    VkResult vkResult;
    XrResult result = m_pfnCreateVulkanInstanceKHR(m_instance, &createInfo, &m_vkInstance, &vkResult);
    if (result != XR_SUCCESS || vkResult != VK_SUCCESS) 
    {
        DevMsg("Failed to create Vulkan instance. XrResult: %d, VkResult: %d\n", result, vkResult);
        return false;
    }
    
    // Select the physical device that the OpenXR runtime wants us to use
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());
    
    // Let OpenXR select the physical device
    XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo{ XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
    deviceGetInfo.systemId = m_systemId;
    deviceGetInfo.vulkanInstance = m_vkInstance;
    
    result = m_pfnGetVulkanGraphicsDevice2KHR(m_instance, &deviceGetInfo, &m_vkPhysicalDevice);

    if (result != XR_SUCCESS) 
    {
        DevMsg("Failed to get Vulkan physical device from OpenXR: %d\n", result);
        return false;
    }
    
    // Find queue family that supports graphics
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());
    
    m_vkQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) 
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            m_vkQueueFamilyIndex = i;
            break;
        }
    }
    
    if (m_vkQueueFamilyIndex == UINT32_MAX) 
    {
        DevMsg("Failed to find graphics queue family!\n");
        return false;
    }
    
    // Let OpenXR create the Vulkan device for us
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = m_vkQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    
    XrVulkanDeviceCreateInfoKHR deviceCreateInfoXr{ XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
    deviceCreateInfoXr.systemId = m_systemId;
    deviceCreateInfoXr.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    deviceCreateInfoXr.vulkanPhysicalDevice = m_vkPhysicalDevice;
    deviceCreateInfoXr.vulkanCreateInfo = &deviceCreateInfo;
    deviceCreateInfoXr.vulkanAllocator = nullptr;
    
    result = m_pfnCreateVulkanDeviceKHR(m_instance, &deviceCreateInfoXr, &m_vkDevice, &vkResult);
    if (result != XR_SUCCESS || vkResult != VK_SUCCESS) 
    {
        DevMsg("Failed to create Vulkan device. XrResult: %d, VkResult: %d\n", result, vkResult);
        return false;
    }
    
    // Get device queue
    vkGetDeviceQueue(m_vkDevice, m_vkQueueFamilyIndex, 0, &m_vkQueue);
    
    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProps);
    DevMsg("Vulkan context created successfully with device: %s (API version %u.%u.%u)\n",
           deviceProps.deviceName,
           VK_VERSION_MAJOR(deviceProps.apiVersion),
           VK_VERSION_MINOR(deviceProps.apiVersion),
           VK_VERSION_PATCH(deviceProps.apiVersion));
    
    return true;
}

void COpenXRManager::RecenterView()
{
    if (!m_vrActive || !m_session)
    {
        return;
    }
    
    // Get player view angles
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    QAngle playerAngles = pPlayer ? pPlayer->EyeAngles() : QAngle(0, 0, 0);
    Vector playerPos = pPlayer ? pPlayer->EyePosition() : Vector(0, 0, 0);
    
    // Get current HMD orientation as a quaternion
    XrPosef eyePose = m_views[0].pose;  // Use center eye or first eye
    Quaternion currentHmdQuat
    (
        eyePose.orientation.z, 
        eyePose.orientation.x,
        eyePose.orientation.y,
        eyePose.orientation.w
    );
    
    // Convert player angles to quaternion - this is the target orientation
    Quaternion playerQuat;
    AngleQuaternion(playerAngles, playerQuat);
    
    // When recentering, we want to calculate the quaternion that would make the HMD's
    // current forward direction match the game camera's forward direction
    
    // Get the inverse of the current HMD orientation
    Quaternion inverseHmdQuat;
    QuaternionConjugate(currentHmdQuat, inverseHmdQuat);
    
    // Calculate the recenter quaternion
    QuaternionMult(playerQuat, inverseHmdQuat, m_recenterQuaternion);
    
    // Also store position offset - this should be the delta to make the current HMD position
    // appear centered relative to the game camera
    static ConVar* vr_position_scale = cvar->FindVar("vr_position_scale");
    float scale = vr_position_scale ? vr_position_scale->GetFloat() : 1.0f;
    
    // Use the same floor-aligned coordinate conversion as in GetMideyePose
    Vector currentPos
    (
        -eyePose.position.z * CalculateDynamicWorldScale(),
        -eyePose.position.x * CalculateDynamicWorldScale(),
        eyePose.position.y * CalculateDynamicWorldScale()
    );
    
    // The recenter position is designed to cancel out the current HMD position offset
    // Making it centered at the player's position when recentering
    m_recenterPosition = Vector(0, 0, 0) - currentPos;
    
    m_hasRecenterData = true;
    
    // VR view recentered
}

// Command to trigger recentering
CON_COMMAND(vr_recenter, "Recenter the VR headset orientation")
{
    if (g_pOpenXRManager)
    {
        g_pOpenXRManager->RecenterView();
    }
}

// Command to calibrate view height (simplified - just recenters)
CON_COMMAND(vr_calibrate_height, "Recenter VR view to match game camera")
{
    if (g_pOpenXRManager)
    {
        g_pOpenXRManager->RecenterView();
        // VR view recentered
    }
}

// Command to calibrate player height
CON_COMMAND(vr_calibrate_player_height, "Calibrate your real height for VR scaling")
{
    if (args.ArgC() < 2)
    {
        ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
        float currentHeight = tfvr_player_height ? tfvr_player_height->GetFloat() : 67.0f;
        DevMsg("Current player height: %.1f inches\n", currentHeight);
        DevMsg("Usage: vr_calibrate_player_height <height_in_inches>\n");
        DevMsg("Example: vr_calibrate_player_height 72 (for 6 feet tall)\n");
        DevMsg("Or use: vr_calibrate_player_height_auto\n");
        return;
    }
    
    float height = atof(args.Arg(1));
    if (height < 20.0f || height > 100.0f) // Reasonable range: 4' to 7'
    {
        DevMsg("Height must be between 20 and 100 inches\n");
        return;
    }
    
    ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
    if (tfvr_player_height)
    {
        tfvr_player_height->SetValue(height);
        DevMsg("Player height set to %.1f inches\n", height);
        
        // Enable height calibration if not already enabled
        ConVar* tfvr_height_calibration = cvar->FindVar("tfvr_height_calibration");
        if (tfvr_height_calibration && !tfvr_height_calibration->GetBool())
        {
            tfvr_height_calibration->SetValue(1);
            DevMsg("Height calibration enabled\n");
        }
    }
}

// Command to auto-calibrate player height from current HMD position
CON_COMMAND(vr_calibrate_player_height_auto, "Auto-calibrate your height from current HMD position (stand tall!)")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active - cannot calibrate height\n");
        return;
    }
    
    // Get raw unscaled HMD position in play space (meters)
    Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
    
    // Convert HMD height from meters to inches
    // rawHmdPos.y is height above play space floor in meters
    float hmdHeightMeters = rawHmdPos.y;
    float hmdHeightInches = hmdHeightMeters * 39.3701f; // meters to inches
    
    // Add estimate for head-to-top-of-head (about 4 inches)
    float estimatedPlayerHeight = hmdHeightInches + 4.0f;
    
    if (estimatedPlayerHeight < 24.0f || estimatedPlayerHeight > 100.0f)
    {
        DevMsg("Detected height %.1f inches seems out of range (48-84). Check your play space setup.\n", estimatedPlayerHeight);
        return;
    }
    
    ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
    if (tfvr_player_height)
    {
        tfvr_player_height->SetValue(estimatedPlayerHeight);
        DevMsg("Auto-calibrated player height to %.1f inches (%.1f' %.0f\")\n", 
               estimatedPlayerHeight, 
               floor(estimatedPlayerHeight / 12.0f), 
               fmod(estimatedPlayerHeight, 12.0f));
        
        // Enable height calibration
        ConVar* tfvr_height_calibration = cvar->FindVar("tfvr_height_calibration");
        ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
        if (tfvr_height_calibration)
        {
            tfvr_height_calibration->SetValue(1);

            // Disable seated mode if it's enabled
            tfvr_seated_mode->SetValue(0);

            DevMsg("Height calibration enabled\n");
        }
    }
}

// Command to toggle seated mode
CON_COMMAND(vr_seated_mode, "Toggle VR seated mode")
{
    ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
    if (!tfvr_seated_mode)
        return;
        
    bool currentValue = tfvr_seated_mode->GetBool();
    tfvr_seated_mode->SetValue(!currentValue);
    
    DevMsg("VR Seated Mode: %s\n", !currentValue ? "ENABLED" : "DISABLED");
    if (!currentValue)
    {
        ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
        float offset = tfvr_seated_height_offset ? tfvr_seated_height_offset->GetFloat() : 24.0f;
        DevMsg("  Using height offset: %.1f inches\n", offset);
    }
}

// Command to set seated mode height offset
CON_COMMAND(vr_seated_height_offset, "Set the height offset for seated mode")
{
    if (args.ArgC() < 2)
    {
        ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
        float currentOffset = tfvr_seated_height_offset ? tfvr_seated_height_offset->GetFloat() : 24.0f;
        DevMsg("Current seated height offset: %.1f inches\n", currentOffset);
        DevMsg("Usage: vr_seated_height_offset <offset_in_inches>\n");
        DevMsg("Example: vr_seated_height_offset 24 (for 2 feet offset)\n");
        DevMsg("Or use: vr_calibrate_seated_height_auto\n");
        return;
    }
    
    float offset = atof(args.Arg(1));
    if (offset < 1.0f || offset > 60.0f) // Reasonable range: 1' to 4'
    {
        DevMsg("Height offset must be between 1 and 60 inches\n");
        return;
    }
    
    ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
    if (tfvr_seated_height_offset)
    {
        tfvr_seated_height_offset->SetValue(offset);
        DevMsg("Seated height offset set to %.1f inches\n", offset);
    }
}

// Command to auto-calibrate seated height offset from current HMD position
CON_COMMAND(vr_calibrate_seated_height_auto, "Auto-calibrate seated height offset from current HMD position (sit normally!)")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active - cannot calibrate seated height\n");
        return;
    }
    
    // Get raw unscaled HMD position in play space (meters)
    Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
    
    // Convert HMD height from meters to inches
    float hmdHeightMeters = rawHmdPos.y;
    float hmdHeightInches = hmdHeightMeters * 39.3701f; // meters to inches
    
    // Calculate offset needed to reach a comfortable standing eye height
    // Assume comfortable standing eye height is about 60-65 inches
    // (this is rough average standing eye height for most people)
    float targetStandingEyeHeight = 64.0f; // inches
    float neededOffset = targetStandingEyeHeight - hmdHeightInches;
    
    ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
    if (tfvr_seated_height_offset)
    {
        tfvr_seated_height_offset->SetValue(neededOffset);
        DevMsg("Auto-calibrated seated height offset to %.1f inches\n", neededOffset);
        DevMsg("Current seated HMD height: %.1f inches\n", hmdHeightInches);
        DevMsg("Will offset to standing height: %.1f inches\n", hmdHeightInches + neededOffset);
        
        // Enable seated mode if not already enabled
        ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
        if (tfvr_seated_mode && !tfvr_seated_mode->GetBool())
        {
            tfvr_seated_mode->SetValue(1);
            DevMsg("Seated mode enabled\n");
        }
    }
}

// Command to toggle seated/standing mode and auto-calibrate (for pause menu button)
CON_COMMAND(vr_toggle_seated_mode, "Toggle between seated and standing VR mode with auto-calibration")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active - cannot toggle seated mode\n");
        return;
    }
    
    ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
    if (!tfvr_seated_mode)
        return;
        
    bool bCurrentlySeated = tfvr_seated_mode->GetBool();
    
    if (bCurrentlySeated)
    {
        // Switching to STANDING mode - disable seated mode and auto-calibrate standing height
        tfvr_seated_mode->SetValue(0);
        Msg("VR Mode: Switching to STANDING\n");
        
        // Auto-calibrate standing height
        Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
        float hmdHeightMeters = rawHmdPos.y;
        float hmdHeightInches = hmdHeightMeters * 39.3701f;
        
        // Add estimate for head-to-top-of-head (about 4 inches)
        float estimatedPlayerHeight = hmdHeightInches + 4.0f;
        
        if (estimatedPlayerHeight >= 24.0f && estimatedPlayerHeight <= 100.0f)
        {
            ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
            if (tfvr_player_height)
            {
                tfvr_player_height->SetValue(estimatedPlayerHeight);
                Msg("  Auto-calibrated standing height to %.1f inches\n", estimatedPlayerHeight);
            }
            
            // Enable height calibration
            ConVar* tfvr_height_calibration = cvar->FindVar("tfvr_height_calibration");
            if (tfvr_height_calibration)
            {
                tfvr_height_calibration->SetValue(1);
            }
        }
        else
        {
            Msg("  Height %.1f inches out of range - please stand tall and try again\n", estimatedPlayerHeight);
        }
    }
    else
    {
        // Switching to SEATED mode - enable seated mode and auto-calibrate seated height offset
        Msg("VR Mode: Switching to SEATED\n");
        
        // Get current HMD height and calculate offset needed
        Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
        float hmdHeightMeters = rawHmdPos.y;
        float hmdHeightInches = hmdHeightMeters * 39.3701f;
        
        // Calculate offset to reach comfortable standing eye height (about 64 inches)
        float targetStandingEyeHeight = 64.0f;
        float neededOffset = targetStandingEyeHeight - hmdHeightInches;
        
        ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
        if (tfvr_seated_height_offset)
        {
            tfvr_seated_height_offset->SetValue(neededOffset);
            Msg("  Auto-calibrated seated height offset to %.1f inches\n", neededOffset);
        }
        
        // Enable seated mode
        tfvr_seated_mode->SetValue(1);
    }
}

// Command to recalibrate current VR mode (for pause menu button)
CON_COMMAND(vr_recalibrate, "Recalibrate the current VR mode (seated or standing)")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active - cannot recalibrate\n");
        return;
    }
    
    ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
    bool bSeatedMode = tfvr_seated_mode && tfvr_seated_mode->GetBool();
    
    Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
    float hmdHeightMeters = rawHmdPos.y;
    float hmdHeightInches = hmdHeightMeters * 39.3701f;
    
    if (bSeatedMode)
    {
        // Recalibrate seated mode
        Msg("VR Recalibrate: Seated mode - sit normally\n");
        
        float targetStandingEyeHeight = 64.0f;
        float neededOffset = targetStandingEyeHeight - hmdHeightInches;
        
        ConVar* tfvr_seated_height_offset = cvar->FindVar("tfvr_seated_height_offset");
        if (tfvr_seated_height_offset)
        {
            tfvr_seated_height_offset->SetValue(neededOffset);
            Msg("  Seated height offset recalibrated to %.1f inches\n", neededOffset);
        }
    }
    else
    {
        // Recalibrate standing mode
        Msg("VR Recalibrate: Standing mode - stand tall\n");
        
        float estimatedPlayerHeight = hmdHeightInches + 4.0f;
        
        if (estimatedPlayerHeight >= 24.0f && estimatedPlayerHeight <= 100.0f)
        {
            ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
            if (tfvr_player_height)
            {
                tfvr_player_height->SetValue(estimatedPlayerHeight);
                Msg("  Standing height recalibrated to %.1f inches\n", estimatedPlayerHeight);
            }
        }
        else
        {
            Msg("  Height out of valid range (24-100 inches)\n");
        }
    }
}

// Command to test dynamic world scaling
// Command to monitor world scale changes affecting aim poses
CON_COMMAND(vr_debug_worldscale_aim, "Debug how world scale changes affect aim poses")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
    if (!pTFPlayer)
    {
        DevMsg("No TF player found\n");
        return;
    }
    
    DevMsg("=== World Scale vs Aim Pose Debug ===\n");
    
    // Get current world scaling info
    float baseScale = tfvr_worldscale.GetFloat();
    float dynamicScale = CalculateDynamicWorldScale();
    
    DevMsg("Base World Scale: %.2f\n", baseScale);
    DevMsg("Dynamic World Scale: %.2f\n", dynamicScale);
    DevMsg("Scale Ratio: %.3f (%.1f%%)\n", dynamicScale / baseScale, (dynamicScale / baseScale) * 100.0f);
    
    // Get player class info
    const C_TFPlayerClass* pPlayerClass = pTFPlayer->GetPlayerClass();
    if (pPlayerClass)
    {
        DevMsg("Player Class: %s\n", pPlayerClass->GetName());
        DevMsg("Class Index: %d\n", pPlayerClass->GetClassIndex());
    }
    
    // Get controller poses with current scaling
    VMatrix rightControllerPose;
    if (g_pOpenXRManager->GetRightControllerPose(rightControllerPose))
    {
        Vector controllerPos = rightControllerPose.GetTranslation();
        DevMsg("Right Controller Position (scaled): (%.2f, %.2f, %.2f)\n", 
               controllerPos.x, controllerPos.y, controllerPos.z);
    }
    
    // Get weapon shoot position (should match)
    Vector shootPos = pTFPlayer->Weapon_ShootPosition();
    DevMsg("Weapon Shoot Position: (%.2f, %.2f, %.2f)\n", shootPos.x, shootPos.y, shootPos.z);
    
    // Check if dynamic scaling is enabled
    ConVar* tfvr_dynamic_worldscale = cvar->FindVar("tfvr_dynamic_worldscale");
    DevMsg("Dynamic Scaling: %s\n", 
           tfvr_dynamic_worldscale && tfvr_dynamic_worldscale->GetBool() ? "ENABLED" : "DISABLED");
    
    if (!tfvr_dynamic_worldscale || !tfvr_dynamic_worldscale->GetBool())
    {
        DevMsg("NOTE: Dynamic world scaling is disabled. Enable with 'tfvr_dynamic_worldscale 1'\n");
    }
    
    DevMsg("\nTo test: Switch classes and run this command again to see scale changes.\n");
}

CON_COMMAND(vr_test_scaling, "Test dynamic world scaling based on merc height")
{
    DevMsg("VR Dynamic Scaling Test:\n");
    DevMsg("  tfvr_dynamic_worldscale: %s\n", 
           cvar->FindVar("tfvr_dynamic_worldscale")->GetBool() ? "Enabled" : "Disabled");
    DevMsg("  tfvr_worldscale: %.1f\n", 
           cvar->FindVar("tfvr_worldscale")->GetFloat());
    
    ConVar* tfvr_height_calibration = cvar->FindVar("tfvr_height_calibration");
    ConVar* tfvr_player_height = cvar->FindVar("tfvr_player_height");
    ConVar* tfvr_seated_mode = cvar->FindVar("tfvr_seated_mode");
    
    DevMsg("  Height Calibration: %s\n", 
           tfvr_height_calibration && tfvr_height_calibration->GetBool() ? "Enabled" : "Disabled");
    DevMsg("  Player Height: %.1f inches\n", 
           tfvr_player_height ? tfvr_player_height->GetFloat() : 67.0f);
    DevMsg("  Seated Mode: %s\n", 
           tfvr_seated_mode && tfvr_seated_mode->GetBool() ? "Enabled" : "Disabled");
    
    C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (pLocalPlayer)
    {
        DevMsg("  Player Class: %s\n", 
               pLocalPlayer->GetPlayerClass() ? pLocalPlayer->GetPlayerClass()->GetName() : "Unknown");
        DevMsg("  Is Crouching: %s\n", (pLocalPlayer->GetFlags() & FL_DUCKING) ? "Yes" : "No");
        DevMsg("  Current Eye Height: %.1f\n", pLocalPlayer->EyePosition().z - pLocalPlayer->GetAbsOrigin().z);
    }
    else
    {
        DevMsg("  No local player found\n");
    }
}

// Command to debug controller poses
CON_COMMAND(vr_debug_poses, "Show debug information about controller poses")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    DevMsg("=== VR Controller Pose Debug ===\n");
    DevMsg("Debug Settings:\n");
    DevMsg("  tfvr_controller_debug_draw: %s\n", tfvr_controller_debug_draw.GetBool() ? "ON" : "OFF");
    DevMsg("  tfvr_debug_aim_poses: %s\n", tfvr_debug_aim_poses.GetBool() ? "ON" : "OFF");
    DevMsg("  tfvr_debug_grip_poses: %s\n", tfvr_debug_grip_poses.GetBool() ? "ON" : "OFF");
    DevMsg("  tfvr_debug_pose_size: %.1f\n", tfvr_debug_pose_size.GetFloat());
    
    DevMsg("\nPose Validity:\n");
    DevMsg("  Left Aim Pose Valid: %s\n", g_pOpenXRManager->IsLeftControllerPoseValid() ? "YES" : "NO");
    DevMsg("  Right Aim Pose Valid: %s\n", g_pOpenXRManager->IsRightControllerPoseValid() ? "YES" : "NO");
    DevMsg("  Left Grip Pose Valid: %s\n", g_pOpenXRManager->IsLeftControllerGripPoseValid() ? "YES" : "NO");
    DevMsg("  Right Grip Pose Valid: %s\n", g_pOpenXRManager->IsRightControllerGripPoseValid() ? "YES" : "NO");
    
    DevMsg("\nController Pose Positions:\n");
    VMatrix leftAim, rightAim, leftGrip, rightGrip;
    if (g_pOpenXRManager->GetLeftControllerPose(leftAim))
    {
        Vector pos = leftAim.GetTranslation();
        DevMsg("  Left Aim: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
    }
    if (g_pOpenXRManager->GetRightControllerPose(rightAim))
    {
        Vector pos = rightAim.GetTranslation();
        DevMsg("  Right Aim: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
    }
    if (g_pOpenXRManager->GetLeftControllerGripPose(leftGrip))
    {
        Vector pos = leftGrip.GetTranslation();
        DevMsg("  Left Grip: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
    }
    if (g_pOpenXRManager->GetRightControllerGripPose(rightGrip))
    {
        Vector pos = rightGrip.GetTranslation();
        DevMsg("  Right Grip: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
    }
    
    DevMsg("\nColors in debug overlay:\n");
    DevMsg("  Left Aim Pose: RED cube\n");
    DevMsg("  Right Aim Pose: GREEN cube\n");
    DevMsg("  Left Grip Pose: BLUE cube\n");
    DevMsg("  Right Grip Pose: YELLOW cube\n");
}

// Command to toggle all pose debugging on/off quickly
CON_COMMAND(vr_toggle_pose_debug, "Toggle all controller pose debugging on/off")
{
    bool currentState = tfvr_controller_debug_draw.GetBool();
    tfvr_controller_debug_draw.SetValue(!currentState);
    DevMsg("Controller pose debugging: %s\n", !currentState ? "ENABLED" : "DISABLED");
}

// Command to debug coordinate transformations in detail
CON_COMMAND(vr_debug_coord_transform, "Debug coordinate transformation issues")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    DevMsg("=== VR Coordinate Transformation Debug ===\n");
    
    // Get raw OpenXR poses
    if (g_pOpenXRManager->GetInputManager())
    {
        XrPosef rawLeftAim, rawRightAim, rawLeftGrip, rawRightGrip, rawHead;
        
        bool leftAimValid = g_pOpenXRManager->GetInputManager()->GetControllerPose("left_hand_pose", rawLeftAim);
        bool rightAimValid = g_pOpenXRManager->GetInputManager()->GetControllerPose("right_hand_pose", rawRightAim);
        bool leftGripValid = g_pOpenXRManager->GetInputManager()->GetControllerPose("left_hand_grip_pose", rawLeftGrip);
        bool rightGripValid = g_pOpenXRManager->GetInputManager()->GetControllerPose("right_hand_grip_pose", rawRightGrip);
        
        DevMsg("\nRaw OpenXR Poses:\n");
        if (rightAimValid)
        {
            DevMsg("Right Aim - Pos: (%.3f, %.3f, %.3f) Rot: (%.3f, %.3f, %.3f, %.3f)\n",
                   rawRightAim.position.x, rawRightAim.position.y, rawRightAim.position.z,
                   rawRightAim.orientation.x, rawRightAim.orientation.y, rawRightAim.orientation.z, rawRightAim.orientation.w);
        }
        if (rightGripValid)
        {
            DevMsg("Right Grip - Pos: (%.3f, %.3f, %.3f) Rot: (%.3f, %.3f, %.3f, %.3f)\n",
                   rawRightGrip.position.x, rawRightGrip.position.y, rawRightGrip.position.z,
                   rawRightGrip.orientation.x, rawRightGrip.orientation.y, rawRightGrip.orientation.z, rawRightGrip.orientation.w);
        }
        
        // Test both coordinate conversion functions
        DevMsg("\nCoordinate Conversion Comparison:\n");
        if (rightAimValid)
        {
            VMatrix oldConv = g_pOpenXRManager->ToSourceCoordinateSystem(rawRightAim);
            VMatrix newConv = g_pOpenXRManager->ToSourceCoordinateSystemFloorAligned(rawRightAim);
            
            Vector oldPos = oldConv.GetTranslation();
            Vector newPos = newConv.GetTranslation();
            
            DevMsg("Right Aim - Old method: (%.1f, %.1f, %.1f)\n", oldPos.x, oldPos.y, oldPos.z);
            DevMsg("Right Aim - New method: (%.1f, %.1f, %.1f)\n", newPos.x, newPos.y, newPos.z);
            DevMsg("Right Aim - Difference: (%.1f, %.1f, %.1f)\n", newPos.x - oldPos.x, newPos.y - oldPos.y, newPos.z - oldPos.z);
        }
    }
}

// Command to test different coordinate transformation methods
CON_COMMAND(vr_test_pose_transforms, "Test different coordinate transformation methods")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    DevMsg("=== Testing Coordinate Transformation Methods ===\n");
    
    bool oldMethod = !tfvr_use_floor_aligned_poses.GetBool();
    DevMsg("Currently using: %s\n", oldMethod ? "OLD method (ToSourceCoordinateSystem)" : "NEW method (ToSourceCoordinateSystemFloorAligned)");
    
    DevMsg("\nSwitching to %s method for testing...\n", oldMethod ? "NEW" : "OLD");
    tfvr_use_floor_aligned_poses.SetValue(!oldMethod);
    
    DevMsg("Test complete. Current method is now: %s\n", 
           tfvr_use_floor_aligned_poses.GetBool() ? "NEW (floor-aligned)" : "OLD (original)");
    DevMsg("Try moving your controller and check if the aim pose offset changes.\n");
    DevMsg("Use 'vr_debug_poses' to see current pose positions.\n");
}

// Command to help isolate the pose offset issue
CON_COMMAND(vr_isolate_pose_offset, "Show step-by-step pose transformation to isolate offset source")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    DevMsg("=== Isolating Right Controller Pose Offset ===\n");
    
    // Get the raw right controller pose
    XrPosef rawRightPose;
    if (g_pOpenXRManager->GetInputManager() && 
        g_pOpenXRManager->GetInputManager()->GetControllerPose("right_hand_pose", rawRightPose))
    {
        DevMsg("Step 1 - Raw OpenXR right controller pose:\n");
        DevMsg("  Position: (%.3f, %.3f, %.3f)\n", rawRightPose.position.x, rawRightPose.position.y, rawRightPose.position.z);
        DevMsg("  Rotation: (%.3f, %.3f, %.3f, %.3f)\n", rawRightPose.orientation.x, rawRightPose.orientation.y, rawRightPose.orientation.z, rawRightPose.orientation.w);
        
        // Test coordinate conversion
        VMatrix oldConv = g_pOpenXRManager->ToSourceCoordinateSystem(rawRightPose);
        VMatrix newConv = g_pOpenXRManager->ToSourceCoordinateSystemFloorAligned(rawRightPose);
        
        Vector oldPos = oldConv.GetTranslation();
        Vector newPos = newConv.GetTranslation();
        
        DevMsg("\nStep 2 - After coordinate conversion:\n");
        DevMsg("  Old method: (%.1f, %.1f, %.1f)\n", oldPos.x, oldPos.y, oldPos.z);
        DevMsg("  New method: (%.1f, %.1f, %.1f)\n", newPos.x, newPos.y, newPos.z);
        DevMsg("  Difference: (%.1f, %.1f, %.1f)\n", newPos.x - oldPos.x, newPos.y - oldPos.y, newPos.z - oldPos.z);
        
        // Get head pose for detailed analysis
        const XrView* views = g_pOpenXRManager->GetViews();
        XrPosef headPose = views[0].pose;
        VMatrix headMatrix = tfvr_use_floor_aligned_poses.GetBool() ? 
            g_pOpenXRManager->ToSourceCoordinateSystemFloorAligned(headPose) : g_pOpenXRManager->ToSourceCoordinateSystem(headPose);
        VMatrix headInverse = headMatrix.InverseTR();
        
        Vector usedConvPos = tfvr_use_floor_aligned_poses.GetBool() ? newPos : oldPos;
        VMatrix controllerMatrix;
        controllerMatrix.Identity();
        controllerMatrix.SetTranslation(usedConvPos);
        
        VMatrix headRelativeController = headInverse * controllerMatrix;
        Vector headRelativePos = headRelativeController.GetTranslation();
        Vector headPos = headMatrix.GetTranslation();
        
        DevMsg("\nStep 3 - Head-relative transformation:\n");
        DevMsg("  Head position: (%.1f, %.1f, %.1f)\n", headPos.x, headPos.y, headPos.z);
        DevMsg("  Controller->Head-relative: (%.1f, %.1f, %.1f)\n", headRelativePos.x, headRelativePos.y, headRelativePos.z);
        
        // Get current final pose
        VMatrix finalPose;
        if (g_pOpenXRManager->GetRightControllerPose(finalPose))
        {
            Vector finalPos = finalPose.GetTranslation();
            DevMsg("\nStep 4 - Final world pose:\n");
            DevMsg("  Position: (%.1f, %.1f, %.1f)\n", finalPos.x, finalPos.y, finalPos.z);
            
            Vector diff = finalPos - usedConvPos;
            DevMsg("  Total transform: (%.1f, %.1f, %.1f)\n", diff.x, diff.y, diff.z);
        }
        
        DevMsg("\nStep 5 - Testing manual corrections:\n");
        DevMsg("  Try: tfvr_pose_offset_y -10 (move left)\n");
        DevMsg("  Try: tfvr_pose_offset_y 10 (move right)\n");
        DevMsg("  Try: tfvr_pose_offset_x -10 (move backward)\n");
        DevMsg("  Try: tfvr_pose_offset_x 10 (move forward)\n");
        DevMsg("  Reset: tfvr_pose_offset_x 0; tfvr_pose_offset_y 0; tfvr_pose_offset_z 0\n");
    }
}

// Command to quickly test offset corrections
CON_COMMAND(vr_test_offset, "Quick test different pose offsets")
{
    if (args.ArgC() < 3)
    {
        DevMsg("Usage: vr_test_offset <axis> <value>\n");
        DevMsg("  axis: x, y, or z\n");
        DevMsg("  value: offset amount (e.g., -10, 5, 0)\n");
        DevMsg("Example: vr_test_offset y -10\n");
        return;
    }
    
    const char* axis = args.Arg(1);
    float value = atof(args.Arg(2));
    
    // Reset all offsets first
    tfvr_pose_offset_x.SetValue(0.0f);
    tfvr_pose_offset_y.SetValue(0.0f);
    tfvr_pose_offset_z.SetValue(0.0f);
    
    // Apply the new offset
    if (strcmp(axis, "x") == 0)
    {
        tfvr_pose_offset_x.SetValue(value);
        DevMsg("Applied X offset: %.1f\n", value);
    }
    else if (strcmp(axis, "y") == 0)
    {
        tfvr_pose_offset_y.SetValue(value);
        DevMsg("Applied Y offset: %.1f\n", value);
    }
    else if (strcmp(axis, "z") == 0)
    {
        tfvr_pose_offset_z.SetValue(value);
        DevMsg("Applied Z offset: %.1f\n", value);
    }
    else
    {
        DevMsg("Invalid axis. Use x, y, or z\n");
    }
}

// Command to set the permanent aim pose correction
CON_COMMAND(vr_set_aim_correction, "Set permanent Y-axis correction for aim poses")
{
    if (args.ArgC() < 2)
    {
        float current = tfvr_aim_pose_y_correction.GetFloat();
        DevMsg("Current aim pose Y correction: %.2f\n", current);
        DevMsg("Usage: vr_set_aim_correction <value>\n");
        DevMsg("Example: vr_set_aim_correction 2.0\n");
        DevMsg("Note: This correction is applied in head-relative space before world transform\n");
        return;
    }
    
    float value = atof(args.Arg(1));
    tfvr_aim_pose_y_correction.SetValue(value);
    
    // Show scaling information
    float baseScale = tfvr_worldscale.GetFloat();
    float currentScale = CalculateDynamicWorldScale();
    float scaleFactor = currentScale / baseScale;
    float scaledCorrection = value * scaleFactor;
    
    DevMsg("Aim pose Y correction set to: %.2f\n", value);
    DevMsg("Base world scale: %.2f, Current scale: %.2f (%.1f%%)\n", 
           baseScale, currentScale, (scaleFactor * 100.0f));
    DevMsg("Actual applied correction: %.2f (scaled by %.3f)\n", scaledCorrection, scaleFactor);
    DevMsg("This correction will now scale automatically with world scale changes.\n");
}

// Command to set crosshair offset for fine-tuning
CON_COMMAND(vr_set_crosshair_offset, "Set crosshair offset for fine-tuning aim")
{
    if (args.ArgC() < 4)
    {
        float currentX = tfvr_crosshair_offset_x.GetFloat();
        float currentY = tfvr_crosshair_offset_y.GetFloat();
        float currentZ = tfvr_crosshair_offset_z.GetFloat();
        DevMsg("Current crosshair offset: (%.2f, %.2f, %.2f)\n", currentX, currentY, currentZ);
        DevMsg("Usage: vr_set_crosshair_offset <x> <y> <z>\n");
        DevMsg("Example: vr_set_crosshair_offset 0 -2 0  (move crosshair left)\n");
        DevMsg("Note: This adjusts where the crosshair appears relative to controller aim\n");
        return;
    }
    
    float x = atof(args.Arg(1));
    float y = atof(args.Arg(2));
    float z = atof(args.Arg(3));
    
    tfvr_crosshair_offset_x.SetValue(x);
    tfvr_crosshair_offset_y.SetValue(y);
    tfvr_crosshair_offset_z.SetValue(z);
    
    DevMsg("Crosshair offset set to: (%.2f, %.2f, %.2f)\n", x, y, z);
    DevMsg("Try small adjustments until crosshair aligns with where you're aiming.\n");
}

// Command to verify weapon shooting consistency
CON_COMMAND(vr_debug_weapon_consistency, "Debug weapon shooting vs crosshair consistency")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer)
    {
        DevMsg("No local player\n");
        return;
    }
    
    C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
    if (!pTFPlayer)
    {
        DevMsg("Not a TF player\n");
        return;
    }
    
    DevMsg("=== VR Weapon Shooting vs Crosshair Consistency Debug ===\n");
    DevMsg("Crosshair controller roll following: %s\n", tfvr_crosshair_follow_controller_roll.GetBool() ? "ENABLED" : "DISABLED");
    
    // Get controller pose directly
    VMatrix rightControllerPose;
    if (g_pOpenXRManager->GetRightControllerPose(rightControllerPose))
    {
        Vector controllerPos = rightControllerPose.GetTranslation();
        QAngle controllerAngles;
        MatrixAngles(rightControllerPose.As3x4(), controllerAngles);
        
        DevMsg("Right Controller Pose:\n");
        DevMsg("  Position: (%.2f, %.2f, %.2f)\n", controllerPos.x, controllerPos.y, controllerPos.z);
        DevMsg("  Angles: (%.2f, %.2f, %.2f) [Pitch, Yaw, Roll]\n", controllerAngles.x, controllerAngles.y, controllerAngles.z);
        if (tfvr_crosshair_follow_controller_roll.GetBool())
        {
            DevMsg("  Crosshair Roll: %.2f degrees\n", controllerAngles.z);
        }
    }
    
    // Get weapon shooting data
    Vector shootPos = pTFPlayer->Weapon_ShootPosition();
    QAngle shootAngles = pTFPlayer->Weapon_ShootAngles();
    
    DevMsg("\nWeapon Shooting Data:\n");
    DevMsg("  Shoot Position: (%.2f, %.2f, %.2f)\n", shootPos.x, shootPos.y, shootPos.z);
    DevMsg("  Shoot Angles: (%.2f, %.2f, %.2f)\n", shootAngles.x, shootAngles.y, shootAngles.z);
    
    // Get crosshair data
    Vector aimOrigin, aimDirection;
    bool hasAimOverride = g_ClientVirtualReality.OverrideWeaponHudAimVectors(&aimOrigin, &aimDirection);
    
    DevMsg("\nCrosshair Data:\n");
    if (hasAimOverride)
    {
        DevMsg("  Aim Origin: (%.2f, %.2f, %.2f)\n", aimOrigin.x, aimOrigin.y, aimOrigin.z);
        DevMsg("  Aim Direction: (%.3f, %.3f, %.3f)\n", aimDirection.x, aimDirection.y, aimDirection.z);
        
        // Calculate differences
        Vector posDiff = aimOrigin - shootPos;
        DevMsg("\nPosition Difference (Crosshair - Weapon):\n");
        DevMsg("  Delta: (%.2f, %.2f, %.2f)\n", posDiff.x, posDiff.y, posDiff.z);
        
        // Check if differences match crosshair offset settings
        extern ConVar tfvr_crosshair_offset_x, tfvr_crosshair_offset_y, tfvr_crosshair_offset_z;
        Vector expectedOffset(tfvr_crosshair_offset_x.GetFloat(), tfvr_crosshair_offset_y.GetFloat(), tfvr_crosshair_offset_z.GetFloat());
        DevMsg("  Expected Offset: (%.2f, %.2f, %.2f)\n", expectedOffset.x, expectedOffset.y, expectedOffset.z);
        
        Vector unexpectedDiff = posDiff - expectedOffset;
        DevMsg("  Unexpected Difference: (%.2f, %.2f, %.2f)\n", unexpectedDiff.x, unexpectedDiff.y, unexpectedDiff.z);
        
        if (unexpectedDiff.Length() > 0.1f)
        {
            DevMsg("WARNING: Crosshair and weapon shooting positions don't match!\n");
        }
        else
        {
            DevMsg("SUCCESS: Crosshair and weapon shooting are consistent.\n");
        }
    }
    else
    {
        DevMsg("  No crosshair aim override available\n");
    }
    
    // Check laser pointer consistency
    extern CVRLaserPointer* g_pVRLaserPointer;
    if (g_pVRLaserPointer)
    {
        Vector laserStart, laserEnd, laserHitPoint, laserHitNormal;
        C_BaseEntity* hitEntity;
        
        // Get laser start position (should match controller position)
        laserStart = g_pVRLaserPointer->GetLaserStart();
        laserEnd = g_pVRLaserPointer->GetLaserEnd();
        
        DevMsg("\nLaser Pointer Data:\n");
        DevMsg("  Laser Start: (%.2f, %.2f, %.2f)\n", laserStart.x, laserStart.y, laserStart.z);
        DevMsg("  Laser End: (%.2f, %.2f, %.2f)\n", laserEnd.x, laserEnd.y, laserEnd.z);
        
        if (hasAimOverride)
        {
            Vector laserPosDiff = laserStart - shootPos;
            DevMsg("  Laser vs Weapon Position Diff: (%.2f, %.2f, %.2f)\n", laserPosDiff.x, laserPosDiff.y, laserPosDiff.z);
            
            Vector laserCrosshairDiff = laserStart - aimOrigin;
            DevMsg("  Laser vs Crosshair Position Diff: (%.2f, %.2f, %.2f)\n", laserCrosshairDiff.x, laserCrosshairDiff.y, laserCrosshairDiff.z);
            
            if (laserPosDiff.Length() > 0.1f)
            {
                DevMsg("WARNING: Laser pointer and weapon shooting positions don't match!\n");
            }
        }
    }
    else
    {
        DevMsg("\nLaser Pointer: Not available\n");
    }
}

// Command to debug crosshair rotation pivot issue
CON_COMMAND(vr_debug_crosshair_pivot, "Debug crosshair rotation pivot issue")
{
    if (!g_pOpenXRManager || !g_pOpenXRManager->IsActive())
    {
        DevMsg("VR not active\n");
        return;
    }
    
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer)
    {
        DevMsg("No local player\n");
        return;
    }
    
    C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
    if (!pTFPlayer)
    {
        DevMsg("Not a TF player\n");
        return;
    }
    
    DevMsg("=== Crosshair Rotation Pivot Debug ===\n");
    
    // Get multiple samples of controller pose
    VMatrix controllerPose1, controllerPose2;
    bool pose1Valid = g_pOpenXRManager->GetRightControllerPose(controllerPose1);
    
    bool pose2Valid = g_pOpenXRManager->GetRightControllerPose(controllerPose2);
    
    if (pose1Valid && pose2Valid)
    {
        Vector pos1 = controllerPose1.GetTranslation();
        Vector pos2 = controllerPose2.GetTranslation();
        
        QAngle angles1, angles2;
        MatrixAngles(controllerPose1.As3x4(), angles1);
        MatrixAngles(controllerPose2.As3x4(), angles2);
        
        DevMsg("Sample 1: Pos(%.2f, %.2f, %.2f) Roll(%.2f)\n", pos1.x, pos1.y, pos1.z, angles1.z);
        DevMsg("Sample 2: Pos(%.2f, %.2f, %.2f) Roll(%.2f)\n", pos2.x, pos2.y, pos2.z, angles2.z);
        
        Vector posDiff = pos2 - pos1;
        float rollDiff = angles2.z - angles1.z;
        
        DevMsg("Position drift: (%.3f, %.3f, %.3f) Roll drift: %.3f\n", 
               posDiff.x, posDiff.y, posDiff.z, rollDiff);
    }
    
    // Get weapon shoot data
    Vector shootPos = pTFPlayer->Weapon_ShootPosition();
    QAngle shootAngles = pTFPlayer->Weapon_ShootAngles();
    
    DevMsg("Weapon shoot pos: (%.2f, %.2f, %.2f)\n", shootPos.x, shootPos.y, shootPos.z);
    DevMsg("Weapon shoot angles: (%.2f, %.2f, %.2f)\n", shootAngles.x, shootAngles.y, shootAngles.z);
    
    // Check laser pointer
    extern CVRLaserPointer* g_pVRLaserPointer;
    if (g_pVRLaserPointer)
    {
        Vector laserStart = g_pVRLaserPointer->GetLaserStart();
        DevMsg("Laser start pos: (%.2f, %.2f, %.2f)\n", laserStart.x, laserStart.y, laserStart.z);
        
        Vector shootLaserDiff = shootPos - laserStart;
        DevMsg("Weapon vs Laser diff: (%.2f, %.2f, %.2f)\n", shootLaserDiff.x, shootLaserDiff.y, shootLaserDiff.z);
    }
    
    // Get crosshair data
    Vector aimOrigin, aimDirection;
    bool hasAimOverride = g_ClientVirtualReality.OverrideWeaponHudAimVectors(&aimOrigin, &aimDirection);
    if (hasAimOverride)
    {
        DevMsg("Crosshair aim pos: (%.2f, %.2f, %.2f)\n", aimOrigin.x, aimOrigin.y, aimOrigin.z);
        
        Vector crosshairLaserDiff = aimOrigin - g_pVRLaserPointer->GetLaserStart();
        DevMsg("Crosshair vs Laser diff: (%.2f, %.2f, %.2f)\n", crosshairLaserDiff.x, crosshairLaserDiff.y, crosshairLaserDiff.z);
    }
}

// Command to toggle crosshair controller roll
CON_COMMAND(vr_toggle_crosshair_roll, "Toggle crosshair controller roll following")
{
    bool currentValue = tfvr_crosshair_follow_controller_roll.GetBool();
    tfvr_crosshair_follow_controller_roll.SetValue(!currentValue);
    
    DevMsg("Crosshair controller roll following: %s\n", !currentValue ? "ENABLED" : "DISABLED");
    if (!currentValue)
    {
        DevMsg("Crosshair will now rotate with controller roll.\n");
    }
    else
    {
        DevMsg("Crosshair will stay level (no roll rotation).\n");
    }
}

VMatrix COpenXRManager::GetEyeViewFromMidEyeView(ISourceVirtualReality::VREye eye)
{
    // Calculate full 3D IPD between eyes
    XrVector3f leftPos = m_views[0].pose.position;
    XrVector3f rightPos = m_views[1].pose.position;
    float dx = rightPos.x - leftPos.x;
    float dy = rightPos.y - leftPos.y;
    float dz = rightPos.z - leftPos.z;
    float ipdMeters = sqrt(dx * dx + dy * dy + dz * dz); // Full distance, e.g., 0.064m
    float ipdUnits = ipdMeters * CalculateDynamicWorldScale();   // e.g., 2.51968 units
    float halfIpd = ipdUnits / 2.0f;                     // e.g., 1.25984 units

    // Construct result matrix with offset only in Source Y
    VMatrix result;
    result.Identity();
    result.m[1][3] = (eye == 0) ? halfIpd : -halfIpd; // Left: +Y, Right: -Y

    return result;
}

VMatrix COpenXRManager::GetMideyePose() const
{
	if (!IsActive())
	    return VMatrix();

    return this->ToSourceCoordinateSystemFloorAligned(m_headLocation.pose);
}

Vector COpenXRManager::GetRawHMDPosition() const
{
	if (!IsActive())
	    return Vector(0, 0, 0);

    // Return the raw OpenXR head position without any world scaling applied
    // This is needed for calibration purposes
    return Vector(m_headLocation.pose.position.x, m_headLocation.pose.position.y, m_headLocation.pose.position.z);
}

float COpenXRManager::GetWorldScale() const
{
    return CalculateDynamicWorldScale();
}

void COpenXRManager::GetHMDInChaperone(class Vector& origin, QAngle& angles) const
{
    origin = GetMideyePose().GetTranslation();
    MatrixToAngles(GetMideyePose(), angles);
}

void COpenXRManager::GetViewportBounds(ISourceVirtualReality::VREye eye, int* pnX, int* pnY, int* pnWidth, int* pnHeight)
{
    if (pnX != NULL)
    {
		*pnX = m_viewConfigs[0].recommendedImageRectWidth * eye;
        *pnY = 0;
    }
    *pnWidth = m_viewConfigs[0].recommendedImageRectWidth;
    *pnHeight = m_viewConfigs[0].recommendedImageRectHeight;
}

Vector2D COpenXRManager::GetBufferSize()
{
    return Vector2D(m_viewConfigs[0].recommendedImageRectWidth, m_viewConfigs[0].recommendedImageRectHeight);
}

bool g_firstUpdateVR = true;
bool g_firstUpdateNonVR = false;
void COpenXRManager::Update(float frametime)
{
	VPROF("VRManager::Update", VPROF_BUDGETGROUP_WORLD_RENDERING);

	if (!g_pOpenXRManager->IsActive())
	{
		if (g_firstUpdateNonVR)
		{
			g_firstUpdateNonVR = false;
			g_firstUpdateVR = true;
		}
		return;
	}

	if (g_firstUpdateVR)
	{
		g_firstUpdateVR = false;
		g_firstUpdateNonVR = true;

		// Swap input system
		if (::input != nullptr)
		{
			g_OriginalNonVRInputPtr = ::input;
		}
		::input = (IInput*)&g_VRInput;
		::input->Init_All();

		for (m_currentRenderBufferIndex = 0; m_currentRenderBufferIndex < VR_NUM_BUFFERS; ++m_currentRenderBufferIndex)
		{
			// pre-create render target materials
			GetRenderTargetMat();
		}

		m_currentRenderBufferIndex = 0;
	}

	m_currentRenderBufferIndex = (++m_currentRenderBufferIndex) % VR_NUM_BUFFERS;

	// Update VR Menu Manager
	if (m_menuManager)
	{
		m_menuManager->Update();
	}
	
	// NOTE: VR Laser Pointer update moved to after CalcView for better timing
	// (see view.cpp after CalcView)

	// Poll OpenXR events (poll even when not running to receive READY state)
	XrEventDataBuffer eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
	while (m_instance != XR_NULL_HANDLE && xrPollEvent(m_instance, &eventBuffer) == XR_SUCCESS)
	{
		switch (eventBuffer.type)
		{
		// Handle session state changes
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
		{
			XrEventDataSessionStateChanged *sessionStateEvent = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventBuffer);

			DevMsg("VR: Session state changed to %d\n", sessionStateEvent->state);
			switch (sessionStateEvent->state)
			{
			case XR_SESSION_STATE_IDLE:
				break;

			case XR_SESSION_STATE_READY:
			{
				if (!m_sessionRunning)
				{
					XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
					beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					XrResult beginResult = xrBeginSession(m_session, &beginInfo);
					if (XR_SUCCEEDED(beginResult))
					{
						DevMsg("VR: Session (re)started from READY state\n");
						m_sessionRunning = true;
					}
					else
					{
						DevMsg("VR: Failed to begin session from READY state: %d\n", beginResult);
					}
				}
				break;
			}

			case XR_SESSION_STATE_SYNCHRONIZED:
				break;

			case XR_SESSION_STATE_FOCUSED:
				m_sessionFocused = true;
				break;
				
			case XR_SESSION_STATE_VISIBLE:
				m_sessionFocused = false;
				break;
			
			case XR_SESSION_STATE_STOPPING:
				Log("Shutdown requested by OpenXR runtime\n");
				xrEndSession(m_session);
				m_sessionRunning = false;
				m_sessionFocused = false;
				break;

			case XR_SESSION_STATE_LOSS_PENDING:
				Log("OpenXR dashboard likely activated\n");
				m_sessionFocused = false;
				if (engine && !enginevgui->IsGameUIVisible())
				{
					engine->ClientCmd_Unrestricted("gameui_toggle\n");
				}
				break;

			case XR_SESSION_STATE_EXITING:
				m_sessionRunning = false;
				m_sessionFocused = false;
				break;

			default:
				break;
			}
			break;
		}

		// Handle instance loss (e.g., runtime shutting down)
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
		{
			XrEventDataInstanceLossPending *lossEvent = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventBuffer);
			Log("OpenXR instance loss pending\n");
			m_sessionRunning = false;
			break;
		}

		default:
			break;
		}

		// Clear the event buffer for the next iteration
		eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER};
	}

	UpdateOpenXRViewData();

	vrRenderTargets->UpdateVRRenderTargets();

	if (ConsumeSpectatorDimsChanged())
	{
		uint32_t w, h;
		GetSpectatorScreenDims(w, h);
		g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget(0, 0, w, h, NULL);

		char szCmd[256];
		Q_snprintf(szCmd, sizeof(szCmd), "mat_setvideomode %u %u 1\n", w, h);
		engine->ClientCmd_Unrestricted(szCmd);
	}

	// unfortunately, r_lod is not archived, so to make it an option, we have to map from our own convar here
	static ConVarRef r_lod("r_lod");
	if (tfvr_forcemaxlod.GetBool() && r_lod.GetInt() != 0)
	{
		r_lod.SetValue(0);
	}
	else if (!tfvr_forcemaxlod.GetBool() && r_lod.GetInt() == 0)
	{
		r_lod.SetValue(-1);
	}

    if (WasButtonPressed("left_trigger")) 
    {
		DevMsg("Left Trigger pressed!\n");
    }

    if (WasButtonPressed("right_trigger"))
	{
		DevMsg("Right Trigger pressed!\n");
	}
}

char* backBufferNamePerIndex(int i)
{
	i = i % VR_NUM_BUFFERS;
	static char name[sizeof(VR_BACK_BUFFER_X) + 2] = {0}; // The most we could possibly need is 100, right?
	if (name[0] == 0)
	{
		sprintf(name, VR_BACK_BUFFER_X);
	}
	sprintf(&name[sizeof(VR_BACK_BUFFER_X) - 1], "%i", i);
	return name;
}

void COpenXRManager::PollInput()
{
    VPROF("OpenXRManager::PollInput");
    
    if (m_inputManager)
    {
        m_inputManager->PollInput();
    }

    // Update hand tracking
    if (m_handTracker)
    {
        m_handTracker->UpdateHandTracking();
    }
}

bool COpenXRManager::IsButtonPressed(const char* actionName)
{
    return m_inputManager ? m_inputManager->IsButtonPressed(actionName) : false;
}

bool COpenXRManager::WasButtonPressed(const char* actionName)
{
    return m_inputManager ? m_inputManager->WasButtonPressed(actionName) : false;
}

bool COpenXRManager::WasButtonReleased(const char* actionName)
{
    return m_inputManager ? m_inputManager->WasButtonReleased(actionName) : false;
}

float COpenXRManager::GetAnalogValue(const char* actionName)
{
    return m_inputManager ? m_inputManager->GetAnalogValue(actionName) : 0.0f;
}

bool COpenXRManager::IsUIInteractionPressed(const char* actionName, float threshold)
{
    return m_inputManager ? m_inputManager->IsUIInteractionPressed(actionName, threshold) : false;
}

bool COpenXRManager::WasUIInteractionPressed(const char* actionName, float threshold)
{
    return m_inputManager ? m_inputManager->WasUIInteractionPressed(actionName, threshold) : false;
}

bool COpenXRManager::WasUIInteractionReleased(const char* actionName, float threshold)
{
    return m_inputManager ? m_inputManager->WasUIInteractionReleased(actionName, threshold) : false;
}

bool COpenXRManager::GetLeftControllerPose(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("left_hand_pose", xrPose))
    {
        // Convert controller pose to Source coordinate system (in playspace)
        VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        // Apply position correction in playspace if enabled
        if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
        {
            // Get raw head pose for calculating correction offset
            VMatrix headInPlayspace = GetMideyePose();
            VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
            
            Vector headRelativePos = headRelativeController.GetTranslation();
            // Scale the correction with the dynamic world scale to maintain consistency
            float baseScale = tfvr_worldscale.GetFloat();
            float currentScale = CalculateDynamicWorldScale();
            float scaleFactor = currentScale / baseScale;
            float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
            headRelativePos.y += scaledCorrection;
            
            // Apply correction back to playspace controller pose
            headRelativeController.SetTranslation(headRelativePos);
            controllerInPlayspace = headInPlayspace * headRelativeController;
        }
        
        // Get player's world transform
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            // VR FIX: Transform controller from playspace directly to world using smoothed playspace-to-world transform
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                // VR FIX: Calculate controller position RELATIVE to head in playspace,
                // then apply that relative transform to the smoothed head in world space.
                // This ensures controllers stay at the correct position relative to the smoothed head.
                
                // Get raw head pose in playspace (unsmoothed)
                VMatrix rawHeadPlayspace = GetMideyePose();
                
                // Calculate controller relative to head (both in playspace coordinates)
                VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
                
                // Get smoothed head-in-world transform (includes stair/prediction smoothing)
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                
                // Apply the relative transform to the smoothed head position
                pose = smoothedHeadWorld * controllerRelativeToHead;
            }
            else
            {
                // Non-VR: just use controller in playspace directly
                pose = controllerInPlayspace;
            }
            
            // Enhanced debug visualization for both aim and grip poses
            if (debugoverlay && tfvr_controller_debug_draw.GetBool())
            {
                float cubeSize = tfvr_debug_pose_size.GetFloat();
                Vector boxSize(cubeSize, cubeSize, cubeSize);
                
                // Show aim pose (current pose) - RED
                if (tfvr_debug_aim_poses.GetBool())
                {
                    Vector aimWorldPos = pose.GetTranslation();
                    
                    // Draw aim pose cube (RED)
                    debugoverlay->AddBoxOverlay(aimWorldPos, -boxSize, boxSize, QAngle(0, 0, 0), 255, 0, 0, 255, 0.016f);
                    
                    // Draw aim pose orientation axes
                    Vector forward, right, up;
                    pose.GetBasisVectors(forward, right, up);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + forward * 20.0f, 255, 0, 0, 255, false, 0.016f);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + right * 20.0f, 0, 255, 0, 255, false, 0.016f);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + up * 20.0f, 0, 0, 255, 255, false, 0.016f);
                }
                
                // Show grip pose - BLUE
                if (tfvr_debug_grip_poses.GetBool())
                {
                    VMatrix gripPose;
                    if (GetLeftControllerGripPose(gripPose))
                    {
                        Vector gripWorldPos = gripPose.GetTranslation();
                        
                        // Draw grip pose cube (BLUE)
                        debugoverlay->AddBoxOverlay(gripWorldPos, -boxSize, boxSize, QAngle(0, 0, 0), 0, 0, 255, 255, 0.016f);
                        
                        // Draw grip pose orientation axes (dimmer colors)
                        Vector gripForward, gripRight, gripUp;
                        gripPose.GetBasisVectors(gripForward, gripRight, gripUp);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripForward * 15.0f, 150, 0, 0, 255, false, 0.016f);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripRight * 15.0f, 0, 150, 0, 255, false, 0.016f);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripUp * 15.0f, 0, 0, 150, 255, false, 0.016f);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool COpenXRManager::GetRightControllerPose(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("right_hand_pose", xrPose))
    {
        // Convert controller pose to Source coordinate system (in playspace)
        VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        // Apply position correction in playspace if enabled
        if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
        {
            // Get raw head pose for calculating correction offset
            VMatrix headInPlayspace = GetMideyePose();
            VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
            
            Vector headRelativePos = headRelativeController.GetTranslation();
            // Scale the correction with the dynamic world scale to maintain consistency
            float baseScale = tfvr_worldscale.GetFloat();
            float currentScale = CalculateDynamicWorldScale();
            float scaleFactor = currentScale / baseScale;
            float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
            headRelativePos.y += scaledCorrection;
            
            // Apply correction back to playspace controller pose
            headRelativeController.SetTranslation(headRelativePos);
            controllerInPlayspace = headInPlayspace * headRelativeController;
        }
        
        // Get player's world transform
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            // VR FIX: Transform controller from playspace directly to world using smoothed playspace-to-world transform
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                // VR FIX: Calculate controller position RELATIVE to head in playspace,
                // then apply that relative transform to the smoothed head in world space.
                // This ensures controllers stay at the correct position relative to the smoothed head.
                
                // Get raw head pose in playspace (unsmoothed)
                VMatrix rawHeadPlayspace = GetMideyePose();
                
                // Calculate controller relative to head (both in playspace coordinates)
                VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
                
                // Get smoothed head-in-world transform (includes stair/prediction smoothing)
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                
                // Apply the relative transform to the smoothed head position
                pose = smoothedHeadWorld * controllerRelativeToHead;
            }
            else
            {
                // Non-VR: just use controller in playspace directly
                pose = controllerInPlayspace;
            }
            
            // Apply manual testing offsets if set
            if (tfvr_pose_offset_x.GetFloat() != 0.0f || tfvr_pose_offset_y.GetFloat() != 0.0f || tfvr_pose_offset_z.GetFloat() != 0.0f)
            {
                Vector currentPos = pose.GetTranslation();
                currentPos.x += tfvr_pose_offset_x.GetFloat();
                currentPos.y += tfvr_pose_offset_y.GetFloat();
                currentPos.z += tfvr_pose_offset_z.GetFloat();
                pose.SetTranslation(currentPos);
            }
            
            // Debug raw pose data if enabled
            if (tfvr_debug_raw_poses.GetBool())
            {
                static float lastDebugTime = 0.0f;
                if (gpGlobals && gpGlobals->realtime - lastDebugTime > 0.5f) // Every half second
                {
                    Vector controllerPos = controllerInPlayspace.GetTranslation();
                    Vector finalPos = pose.GetTranslation();
                    
                    DevMsg("Right Controller Debug:\n");
                    DevMsg("  1. Raw OpenXR: pos(%.3f, %.3f, %.3f)\n", xrPose.position.x, xrPose.position.y, xrPose.position.z);
                    DevMsg("  2. Playspace (after coord conv): pos(%.1f, %.1f, %.1f)\n", controllerPos.x, controllerPos.y, controllerPos.z);
                    DevMsg("  3. Final world (with smoothing): pos(%.1f, %.1f, %.1f)\n", finalPos.x, finalPos.y, finalPos.z);
                    lastDebugTime = gpGlobals->realtime;
                }
            }
            
            // Enhanced debug visualization for both aim and grip poses
            if (debugoverlay && tfvr_controller_debug_draw.GetBool())
            {
                float cubeSize = tfvr_debug_pose_size.GetFloat();
                Vector boxSize(cubeSize, cubeSize, cubeSize);
                
                // Show aim pose (current pose) - GREEN
                if (tfvr_debug_aim_poses.GetBool())
                {
                    Vector aimWorldPos = pose.GetTranslation();
                    
                    // Draw aim pose cube (GREEN)
                    debugoverlay->AddBoxOverlay(aimWorldPos, -boxSize, boxSize, QAngle(0, 0, 0), 0, 255, 0, 255, 0.016f);
                    
                    // Draw aim pose orientation axes
                    Vector forward, right, up;
                    pose.GetBasisVectors(forward, right, up);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + forward * 20.0f, 255, 0, 0, 255, false, 0.016f);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + right * 20.0f, 0, 255, 0, 255, false, 0.016f);
                    debugoverlay->AddLineOverlayAlpha(aimWorldPos, aimWorldPos + up * 20.0f, 0, 0, 255, 255, false, 0.016f);
                }
                
                // Show grip pose - YELLOW
                if (tfvr_debug_grip_poses.GetBool())
                {
                    VMatrix gripPose;
                    if (GetRightControllerGripPose(gripPose))
                    {
                        Vector gripWorldPos = gripPose.GetTranslation();
                        
                        // Draw grip pose cube (YELLOW)
                        debugoverlay->AddBoxOverlay(gripWorldPos, -boxSize, boxSize, QAngle(0, 0, 0), 255, 255, 0, 255, 0.016f);
                        
                        // Draw grip pose orientation axes (dimmer colors)
                        Vector gripForward, gripRight, gripUp;
                        gripPose.GetBasisVectors(gripForward, gripRight, gripUp);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripForward * 15.0f, 150, 0, 0, 255, false, 0.016f);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripRight * 15.0f, 0, 150, 0, 255, false, 0.016f);
                        debugoverlay->AddLineOverlayAlpha(gripWorldPos, gripWorldPos + gripUp * 15.0f, 0, 0, 150, 255, false, 0.016f);
                    }
                }
            }
        }
        
        return true;
    }
    return false;
}

bool COpenXRManager::GetLeftControllerGripPose(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("left_hand_grip_pose", xrPose))
    {
        // Convert controller pose to Source coordinate system (in playspace)
        VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        // Apply position correction in playspace if enabled
        if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
        {
            // Get raw head pose for calculating correction offset
            VMatrix headInPlayspace = GetMideyePose();
            VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
            
            Vector headRelativePos = headRelativeController.GetTranslation();
            // Scale the correction with the dynamic world scale to maintain consistency
            float baseScale = tfvr_worldscale.GetFloat();
            float currentScale = CalculateDynamicWorldScale();
            float scaleFactor = currentScale / baseScale;
            float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
            headRelativePos.y += scaledCorrection;
            
            // Apply correction back to playspace controller pose
            headRelativeController.SetTranslation(headRelativePos);
            controllerInPlayspace = headInPlayspace * headRelativeController;
        }
        
        // Get player's world transform
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            // VR FIX: Transform controller from playspace directly to world using smoothed playspace-to-world transform
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                // VR FIX: Calculate controller position RELATIVE to head in playspace,
                // then apply that relative transform to the smoothed head in world space.
                // This ensures controllers stay at the correct position relative to the smoothed head.
                
                // Get raw head pose in playspace (unsmoothed)
                VMatrix rawHeadPlayspace = GetMideyePose();
                
                // Calculate controller relative to head (both in playspace coordinates)
                VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
                
                // Get smoothed head-in-world transform (includes stair/prediction smoothing)
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                
                // Apply the relative transform to the smoothed head position
                pose = smoothedHeadWorld * controllerRelativeToHead;
            }
            else
            {
                // Non-VR: just use controller in playspace directly
                pose = controllerInPlayspace;
            }
            
            return true;
        }
    }
    return false;
}

bool COpenXRManager::GetRightControllerGripPose(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("right_hand_grip_pose", xrPose))
    {
        // Convert controller pose to Source coordinate system (in playspace)
        VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        // Apply position correction in playspace if enabled
        if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
        {
            // Get raw head pose for calculating correction offset
            VMatrix headInPlayspace = GetMideyePose();
            VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
            
            Vector headRelativePos = headRelativeController.GetTranslation();
            // Scale the correction with the dynamic world scale to maintain consistency
            float baseScale = tfvr_worldscale.GetFloat();
            float currentScale = CalculateDynamicWorldScale();
            float scaleFactor = currentScale / baseScale;
            float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
            headRelativePos.y += scaledCorrection;
            
            // Apply correction back to playspace controller pose
            headRelativeController.SetTranslation(headRelativePos);
            controllerInPlayspace = headInPlayspace * headRelativeController;
        }
        
        // Get player's world transform
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            // VR FIX: Transform controller from playspace directly to world using smoothed playspace-to-world transform
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                // VR FIX: Calculate controller position RELATIVE to head in playspace,
                // then apply that relative transform to the smoothed head in world space.
                // This ensures controllers stay at the correct position relative to the smoothed head.
                
                // Get raw head pose in playspace (unsmoothed)
                VMatrix rawHeadPlayspace = GetMideyePose();
                
                // Calculate controller relative to head (both in playspace coordinates)
                VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
                
                // Get smoothed head-in-world transform (includes stair/prediction smoothing)
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                
                // Apply the relative transform to the smoothed head position
                pose = smoothedHeadWorld * controllerRelativeToHead;
            }
            else
            {
                // Non-VR: just use controller in playspace directly
                pose = controllerInPlayspace;
            }
            
            return true;
        }
    }
    return false;
}

bool COpenXRManager::IsLeftControllerPoseValid()
{
    return m_inputManager ? m_inputManager->IsControllerPoseValid("left_hand_pose") : false;
}

bool COpenXRManager::IsRightControllerPoseValid()
{
    return m_inputManager ? m_inputManager->IsControllerPoseValid("right_hand_pose") : false;
}

bool COpenXRManager::IsLeftControllerGripPoseValid()
{
    return m_inputManager ? m_inputManager->IsControllerPoseValid("left_hand_grip_pose") : false;
}

bool COpenXRManager::IsRightControllerGripPoseValid()
{
    return m_inputManager ? m_inputManager->IsControllerPoseValid("right_hand_grip_pose") : false;
}

//-----------------------------------------------------------------------------
// Palm pose from XR_EXT_palm_pose (standardized across runtimes, no hand
// tracking extension required). Uses the same head-relative smoothing
// pipeline as controller poses.
//-----------------------------------------------------------------------------
bool COpenXRManager::GetLeftPalmPose(VMatrix& pose)
{
    if (!m_inputManager || !m_palmPoseSupported) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("left_palm_pose", xrPose))
    {
        VMatrix palmInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                VMatrix rawHeadPlayspace = GetMideyePose();
                VMatrix palmRelativeToHead = rawHeadPlayspace.InverseTR() * palmInPlayspace;
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                pose = smoothedHeadWorld * palmRelativeToHead;
            }
            else
            {
                pose = palmInPlayspace;
            }
            return true;
        }
    }
    return false;
}

bool COpenXRManager::GetRightPalmPose(VMatrix& pose)
{
    if (!m_inputManager || !m_palmPoseSupported) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("right_palm_pose", xrPose))
    {
        VMatrix palmInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            extern CClientVirtualReality g_ClientVirtualReality;
            extern bool UseVR();
            
            if (UseVR())
            {
                VMatrix rawHeadPlayspace = GetMideyePose();
                VMatrix palmRelativeToHead = rawHeadPlayspace.InverseTR() * palmInPlayspace;
                VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
                pose = smoothedHeadWorld * palmRelativeToHead;
            }
            else
            {
                pose = palmInPlayspace;
            }
            return true;
        }
    }
    return false;
}

VMatrix COpenXRManager::ToSourceCoordinateSystem(const XrPosef& pose) const
{
    Vector oXrPos(pose.position.x, pose.position.y, pose.position.z);
    Quaternion oXrRot(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);

    return ConvertFromOpenXRQuatVector(oXrRot, oXrPos);
}

VMatrix COpenXRManager::ToSourceCoordinateSystemFloorAligned(const XrPosef& pose) const
{
    // OpenXR to Source coordinate conversion with automatic floor alignment:
    // OpenXR: +X=right, +Y=up, +Z=forward
    // Source: +X=forward, +Y=left, +Z=up
    // 
    // The key insight is that OpenXR's STAGE reference space should already
    // be aligned with the physical floor. We just need to convert coordinates
    // properly without manual offsets.
    
    Quaternion convertedRot(-pose.orientation.z, -pose.orientation.x, pose.orientation.y, pose.orientation.w);
    
    // Convert position with automatic floor alignment
    Vector convertedPos;
    convertedPos.x = -pose.position.z * CalculateDynamicWorldScale();  // OpenXR Z -> Source X (forward)
    convertedPos.y = -pose.position.x * CalculateDynamicWorldScale();  // OpenXR X -> Source Y (left)
    
    // Convert position with automatic floor alignment
    // The world scaling system should handle class height differences naturally
    float rawZ = pose.position.y * CalculateDynamicWorldScale();
    
    // Ensure Z is never negative - the floor should always be at Z=0 or higher
    convertedPos.z = max(0.0f, rawZ);  // OpenXR Y -> Source Z (up)
    
    matrix3x4_t matrix;
    QuaternionMatrix(convertedRot, convertedPos, matrix);
    
    VMatrix result;
    result.CopyFrom3x4(matrix);
    return result;
}

//-----------------------------------------------------------------------------
// Raw controller poses (playspace-relative, no player transforms)
//-----------------------------------------------------------------------------
bool COpenXRManager::GetLeftControllerPoseRaw(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("left_hand_pose", xrPose))
    {
        // Convert directly to Source coordinate system without any player transforms
        pose = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        return true;
    }
    return false;
}

bool COpenXRManager::GetRightControllerPoseRaw(VMatrix& pose)
{
    if (!m_inputManager) return false;
    
    XrPosef xrPose;
    if (m_inputManager->GetControllerPose("right_hand_pose", xrPose))
    {
        // Convert directly to Source coordinate system without any player transforms
        pose = tfvr_use_floor_aligned_poses.GetBool() ? 
            this->ToSourceCoordinateSystemFloorAligned(xrPose) : this->ToSourceCoordinateSystem(xrPose);
        return true;
    }
    return false;
}

bool COpenXRManager::GetLeftControllerPoseXR(XrPosef& pose)
{
    if (!m_inputManager) return false;
    return m_inputManager->GetControllerPose("left_hand_pose", pose);
}

bool COpenXRManager::GetRightControllerPoseXR(XrPosef& pose)
{
    if (!m_inputManager) return false;
    return m_inputManager->GetControllerPose("right_hand_pose", pose);
}

bool COpenXRManager::GetLeftControllerAimRay(Vector& worldPos, Vector& worldDir)
{
    XrPosef xrPose;
    if (!GetLeftControllerPoseXR(xrPose))
        return false;
    
    return ComputeAimRayFromXRPose(xrPose, worldPos, worldDir);
}

bool COpenXRManager::GetRightControllerAimRay(Vector& worldPos, Vector& worldDir)
{
    XrPosef xrPose;
    if (!GetRightControllerPoseXR(xrPose))
        return false;
    
    return ComputeAimRayFromXRPose(xrPose, worldPos, worldDir);
}

bool COpenXRManager::SampleFreshRightControllerPose(Vector& worldPos, QAngle& worldAngles)
{
    if (!m_inputManager)
        return false;
    
    // Sample FRESH pose directly from OpenXR (bypasses per-frame cache)
    XrPosef xrPose;
    if (!m_inputManager->SamplePoseNow("right_hand_pose", xrPose))
        return false;
    
    // Use the exact same transformation as GetRightControllerPose but with fresh pose data
    VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
        ToSourceCoordinateSystemFloorAligned(xrPose) : ToSourceCoordinateSystem(xrPose);
    
    // Apply position correction in playspace if enabled
    if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
    {
        VMatrix headInPlayspace = GetMideyePose();
        VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
        
        Vector headRelativePos = headRelativeController.GetTranslation();
        float baseScale = tfvr_worldscale.GetFloat();
        float currentScale = CalculateDynamicWorldScale();
        float scaleFactor = currentScale / baseScale;
        float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
        headRelativePos.y += scaledCorrection;
        
        headRelativeController.SetTranslation(headRelativePos);
        controllerInPlayspace = headInPlayspace * headRelativeController;
    }
    
    // Get player's world transform and apply VR smoothing
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        extern CClientVirtualReality g_ClientVirtualReality;
        extern bool UseVR();
        
        VMatrix pose;
        if (UseVR())
        {
            // Calculate controller relative to head, then apply smoothed head world transform
            VMatrix rawHeadPlayspace = GetMideyePose();
            VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
            VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
            pose = smoothedHeadWorld * controllerRelativeToHead;
        }
        else
        {
            pose = controllerInPlayspace;
        }
        
        // Apply manual testing offsets if set
        if (tfvr_pose_offset_x.GetFloat() != 0.0f || tfvr_pose_offset_y.GetFloat() != 0.0f || tfvr_pose_offset_z.GetFloat() != 0.0f)
        {
            Vector currentPos = pose.GetTranslation();
            currentPos.x += tfvr_pose_offset_x.GetFloat();
            currentPos.y += tfvr_pose_offset_y.GetFloat();
            currentPos.z += tfvr_pose_offset_z.GetFloat();
            pose.SetTranslation(currentPos);
        }
        
        worldPos = pose.GetTranslation();
        MatrixAngles(pose.As3x4(), worldAngles);
        return true;
    }
    
    // Fallback: just use playspace pose
    worldPos = controllerInPlayspace.GetTranslation();
    MatrixAngles(controllerInPlayspace.As3x4(), worldAngles);
    return true;
}

bool COpenXRManager::SampleFreshLeftControllerPose(Vector& worldPos, QAngle& worldAngles)
{
    if (!m_inputManager)
        return false;
    
    // Sample FRESH pose directly from OpenXR (bypasses per-frame cache)
    XrPosef xrPose;
    if (!m_inputManager->SamplePoseNow("left_hand_pose", xrPose))
        return false;
    
    // Use the exact same transformation as GetLeftControllerPose but with fresh pose data
    VMatrix controllerInPlayspace = tfvr_use_floor_aligned_poses.GetBool() ? 
        ToSourceCoordinateSystemFloorAligned(xrPose) : ToSourceCoordinateSystem(xrPose);
    
    // Apply position correction in playspace if enabled
    if (tfvr_aim_pose_y_correction.GetFloat() != 0.0f)
    {
        VMatrix headInPlayspace = GetMideyePose();
        VMatrix headRelativeController = headInPlayspace.InverseTR() * controllerInPlayspace;
        
        Vector headRelativePos = headRelativeController.GetTranslation();
        float baseScale = tfvr_worldscale.GetFloat();
        float currentScale = CalculateDynamicWorldScale();
        float scaleFactor = currentScale / baseScale;
        float scaledCorrection = tfvr_aim_pose_y_correction.GetFloat() * scaleFactor;
        headRelativePos.y += scaledCorrection;
        
        headRelativeController.SetTranslation(headRelativePos);
        controllerInPlayspace = headInPlayspace * headRelativeController;
    }
    
    // Get player's world transform and apply VR smoothing
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        extern CClientVirtualReality g_ClientVirtualReality;
        extern bool UseVR();
        
        VMatrix pose;
        if (UseVR())
        {
            // Calculate controller relative to head, then apply smoothed head world transform
            VMatrix rawHeadPlayspace = GetMideyePose();
            VMatrix controllerRelativeToHead = rawHeadPlayspace.InverseTR() * controllerInPlayspace;
            VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
            pose = smoothedHeadWorld * controllerRelativeToHead;
        }
        else
        {
            pose = controllerInPlayspace;
        }
        
        // Apply manual testing offsets if set
        if (tfvr_pose_offset_x.GetFloat() != 0.0f || tfvr_pose_offset_y.GetFloat() != 0.0f || tfvr_pose_offset_z.GetFloat() != 0.0f)
        {
            Vector currentPos = pose.GetTranslation();
            currentPos.x += tfvr_pose_offset_x.GetFloat();
            currentPos.y += tfvr_pose_offset_y.GetFloat();
            currentPos.z += tfvr_pose_offset_z.GetFloat();
            pose.SetTranslation(currentPos);
        }
        
        worldPos = pose.GetTranslation();
        MatrixAngles(pose.As3x4(), worldAngles);
        return true;
    }
    
    // Fallback: just use playspace pose
    worldPos = controllerInPlayspace.GetTranslation();
    MatrixAngles(controllerInPlayspace.As3x4(), worldAngles);
    return true;
}

bool COpenXRManager::ComputeAimRayFromXRPose(const XrPosef& xrPose, Vector& worldPos, Vector& worldDir)
{
    // This uses the same transformation approach as the laser pointer
    // to ensure consistent aiming between the laser and menu cursor
    
    float worldScale = CalculateDynamicWorldScale();
    
    // Build rotation matrix from quaternion (OpenXR space)
    float qx = xrPose.orientation.x;
    float qy = xrPose.orientation.y;
    float qz = xrPose.orientation.z;
    float qw = xrPose.orientation.w;
    
    float xx = qx * qx, yy = qy * qy, zz = qz * qz;
    float xz = qx * qz, yz = qy * qz;
    float wx = qw * qx, wy = qw * qy;
    
    // Get Z axis direction (backward in OpenXR space) - third column of rotation matrix
    // For quaternion (qx,qy,qz,qw), Z column is: (2(xz+wy), 2(yz-wx), 1-2(xx+yy))
    Vector zAxisXR;
    zAxisXR.x = 2.0f * (xz + wy);
    zAxisXR.y = 2.0f * (yz - wx);
    zAxisXR.z = 1.0f - 2.0f * (xx + yy);
    
    // Position in OpenXR space (meters)
    Vector posXR(xrPose.position.x, xrPose.position.y, xrPose.position.z);
    
    // Convert position from OpenXR to Source playspace and scale to game units
    Vector playspacePosSource;
    playspacePosSource.x = -posXR.z * worldScale;
    playspacePosSource.y = -posXR.x * worldScale;
    playspacePosSource.z = posXR.y * worldScale;
    
    // Transform position through head-relative to world
    extern CClientVirtualReality g_ClientVirtualReality;
    VMatrix headInPlayspace = GetMideyePose();
    VMatrix headInverse = headInPlayspace.InverseTR();
    VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();
    
    Vector posRelativeToHead = headInverse.VMul4x3(playspacePosSource);
    worldPos = smoothedHeadWorld.VMul4x3(posRelativeToHead);
    
    // To get the correct world direction, transform a point along the forward direction
    // the same way we transform the position, then compute the difference
    Vector testPointXR = posXR + zAxisXR * (-0.1f);  // 10cm forward in OpenXR (negative Z)
    
    // Convert test point to Source playspace
    Vector testPointSource;
    testPointSource.x = -testPointXR.z * worldScale;
    testPointSource.y = -testPointXR.x * worldScale;
    testPointSource.z = testPointXR.y * worldScale;
    
    // Transform test point through head-relative to world
    Vector testRelativeToHead = headInverse.VMul4x3(testPointSource);
    Vector testPointWorld = smoothedHeadWorld.VMul4x3(testRelativeToHead);
    
    // Direction is from controller position to test point
    worldDir = testPointWorld - worldPos;
    worldDir.NormalizeInPlace();
    
    return true;
}

COpenXRManager g_TFVR;
COpenXRManager* g_pOpenXRManager = &g_TFVR;
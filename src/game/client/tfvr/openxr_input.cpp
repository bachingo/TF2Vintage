#include "cbase.h"
#include "openxr_input.h"
#include "openxr_manager.h"
#include "hmdWrapper.h"

COpenXRInputManager::COpenXRInputManager(COpenXRManager* manager)
    : m_manager(manager)
    , m_instance(manager->GetInstance())
    , m_session(manager->GetSession())
    , m_actionSet(XR_NULL_HANDLE)
{
}

COpenXRInputManager::~COpenXRInputManager()
{
    Shutdown();
}

bool COpenXRInputManager::Initialize()
{
    if (!CreateActionSet()) return false;
    if (!CreateActions()) return false;
    if (!CreateInteractionProfiles()) return false;
    if (!AttachActionSet()) return false;
    
    // Pass aim spaces to compositor for direct sampling
    XrSpace leftAimSpace = GetActionSpace("left_hand_pose");
    XrSpace rightAimSpace = GetActionSpace("right_hand_pose");
    if (leftAimSpace != XR_NULL_HANDLE && rightAimSpace != XR_NULL_HANDLE) {
        dxvkSetAimSpaces(leftAimSpace, rightAimSpace);
        DevMsg("Passed aim spaces to compositor for direct sampling\n");
    }
    
    return true;
}

void COpenXRInputManager::Shutdown()
{
    for (auto& action : m_actions)
    {
        if (action.second.handle != XR_NULL_HANDLE)
        {
            xrDestroyAction(action.second.handle);
        }
    }
    m_actions.clear();

    // Clean up action spaces
    for (auto& actionSpace : m_actionSpaces)
    {
        if (actionSpace.second != XR_NULL_HANDLE)
        {
            xrDestroySpace(actionSpace.second);
        }
    }
    m_actionSpaces.clear();

    if (m_actionSet != XR_NULL_HANDLE)
    {
        xrDestroyActionSet(m_actionSet);
        m_actionSet = XR_NULL_HANDLE;
    }
}

bool COpenXRInputManager::CreateActionSet()
{
    XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
    strcpy_s(actionSetInfo.actionSetName, "gameplay");
    strcpy_s(actionSetInfo.localizedActionSetName, "Gameplay");
    
    XrResult result = xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create action set: %d\n", result);
        return false;
    }

    result = xrStringToPath(m_instance, "/user/hand/left", &m_leftHandPath);
    if (!XR_SUCCEEDED(result)) return false;

    result = xrStringToPath(m_instance, "/user/hand/right", &m_rightHandPath);
    if (!XR_SUCCEEDED(result)) return false;

    return true;
}

bool COpenXRInputManager::CreateActions()
{
    // Create movement actions
    XrInputAction moveX = CreateFloatAction("move_x", "Move X");
    if (moveX.handle == XR_NULL_HANDLE) return false;
    m_actions["move_x"] = moveX;

    XrInputAction moveY = CreateFloatAction("move_y", "Move Y");
    if (moveY.handle == XR_NULL_HANDLE) return false;
    m_actions["move_y"] = moveY;

    // Create turning actions for smooth/snap turning
    XrInputAction turnX = CreateFloatAction("turn_x", "Turn X");
    if (turnX.handle == XR_NULL_HANDLE) return false;
    m_actions["turn_x"] = turnX;

    // Create controller pose actions for tracking
    XrInputAction leftHandPose = CreatePoseAction("left_hand_pose", "Left Hand Pose");
    if (leftHandPose.handle == XR_NULL_HANDLE) return false;
    m_actions["left_hand_pose"] = leftHandPose;

    XrInputAction rightHandPose = CreatePoseAction("right_hand_pose", "Right Hand Pose");
    if (rightHandPose.handle == XR_NULL_HANDLE) return false;
    m_actions["right_hand_pose"] = rightHandPose;

    // Create grip pose actions for tracking
    XrInputAction leftHandGripPose = CreatePoseAction("left_hand_grip_pose", "Left Hand Grip Pose");
    if (leftHandGripPose.handle == XR_NULL_HANDLE) return false;
    m_actions["left_hand_grip_pose"] = leftHandGripPose;

    XrInputAction rightHandGripPose = CreatePoseAction("right_hand_grip_pose", "Right Hand Grip Pose");
    if (rightHandGripPose.handle == XR_NULL_HANDLE) return false;
    m_actions["right_hand_grip_pose"] = rightHandGripPose;

    // Create palm pose actions (XR_EXT_palm_pose - standardized hand placement)
    if (m_manager->IsPalmPoseSupported())
    {
        XrInputAction leftPalmPose = CreatePoseAction("left_palm_pose", "Left Palm Pose");
        if (leftPalmPose.handle == XR_NULL_HANDLE) return false;
        m_actions["left_palm_pose"] = leftPalmPose;

        XrInputAction rightPalmPose = CreatePoseAction("right_palm_pose", "Right Palm Pose");
        if (rightPalmPose.handle == XR_NULL_HANDLE) return false;
        m_actions["right_palm_pose"] = rightPalmPose;
    }

    // Create button actions
    XrInputAction primaryAttack = CreateFloatAction("primary_attack", "Primary Attack");
    if (primaryAttack.handle == XR_NULL_HANDLE) return false;
    m_actions["primary_attack"] = primaryAttack;

    XrInputAction secondaryAttack = CreateFloatAction("secondary_attack", "Secondary Attack");
    if (secondaryAttack.handle == XR_NULL_HANDLE) return false;
    m_actions["secondary_attack"] = secondaryAttack;

    XrInputAction use = CreateBooleanAction("use", "Use");
    if (use.handle == XR_NULL_HANDLE) return false;
    m_actions["use"] = use;

    XrInputAction duck = CreateBooleanAction("duck", "Duck");
    if (duck.handle == XR_NULL_HANDLE) return false;
    m_actions["duck"] = duck;

    XrInputAction jump = CreateBooleanAction("jump", "Jump");
    if (jump.handle == XR_NULL_HANDLE) return false;
    m_actions["jump"] = jump;

    XrInputAction menu = CreateBooleanAction("menu", "Menu");
    if (menu.handle == XR_NULL_HANDLE) return false;
    m_actions["menu"] = menu;

    // Add separate UI interaction actions for left and right hands (using float for trigger values)
    XrInputAction leftUIInteract = CreateFloatAction("left_ui_interact", "Left UI Interact");
    if (leftUIInteract.handle == XR_NULL_HANDLE) return false;
    m_actions["left_ui_interact"] = leftUIInteract;

    XrInputAction rightUIInteract = CreateFloatAction("right_ui_interact", "Right UI Interact");
    if (rightUIInteract.handle == XR_NULL_HANDLE) return false;
    m_actions["right_ui_interact"] = rightUIInteract;

    // Add class menu action for left A button
    XrInputAction leftClassMenu = CreateBooleanAction("left_class_menu", "Left Class Menu");
    if (leftClassMenu.handle == XR_NULL_HANDLE) return false;
    m_actions["left_class_menu"] = leftClassMenu;

    // Add weapon switching actions
    XrInputAction weaponSwitch = CreateFloatAction("right_weapon_switch", "Right Weapon Switch");
    if (weaponSwitch.handle == XR_NULL_HANDLE) return false;
    m_actions["weapon_switch"] = weaponSwitch;

    // Add left grip button action for two-handed weapon gripping
    XrInputAction leftGrip = CreateFloatAction("left_grip", "Left Grip");
    if (leftGrip.handle == XR_NULL_HANDLE) return false;
    m_actions["left_grip"] = leftGrip;

    // Right grip force — used to activate throw hold (requires deliberate squeeze)
    XrInputAction rightGrip = CreateFloatAction("right_grip", "Right Grip");
    if (rightGrip.handle == XR_NULL_HANDLE) return false;
    m_actions["right_grip"] = rightGrip;

    // Right grip value — used to detect throw release (instant binary on Index)
    XrInputAction rightGripValue = CreateFloatAction("right_grip_value", "Right Grip Value");
    if (rightGripValue.handle == XR_NULL_HANDLE) return false;
    m_actions["right_grip_value"] = rightGripValue;

    // Add voice action for voice chat activation
    // Used when gesture mode is disabled, or for the gesture trigger when enabled
    XrInputAction voice = CreateFloatAction("voice", "Voice Chat");
    if (voice.handle == XR_NULL_HANDLE) return false;
    m_actions["voice"] = voice;

    // Add weapon select hold action (for radial weapon selection menu)
    XrInputAction weaponSelectHold = CreateBooleanAction("weapon_select_hold", "Weapon Select Hold");
    if (weaponSelectHold.handle == XR_NULL_HANDLE) return false;
    m_actions["weapon_select_hold"] = weaponSelectHold;

    // Add scoreboard action (left thumbstick click for Index/Quest, plus trackpad force for Index)
    XrInputAction scoreboard = CreateBooleanAction("scoreboard", "Scoreboard");
    if (scoreboard.handle == XR_NULL_HANDLE) return false;
    m_actions["scoreboard"] = scoreboard;

    // Add trackpad force actions for Index controllers (trackpad has force, not click)
    XrInputAction leftTrackpadForce = CreateFloatAction("left_trackpad_force", "Left Trackpad Force");
    if (leftTrackpadForce.handle == XR_NULL_HANDLE) return false;
    m_actions["left_trackpad_force"] = leftTrackpadForce;

    XrInputAction rightTrackpadForce = CreateFloatAction("right_trackpad_force", "Right Trackpad Force");
    if (rightTrackpadForce.handle == XR_NULL_HANDLE) return false;
    m_actions["right_trackpad_force"] = rightTrackpadForce;

    // WMR trackpad click + Y position for split-trackpad jump/duck
    XrInputAction rightTrackpadClick = CreateBooleanAction("right_trackpad_click", "Right Trackpad Click");
    if (rightTrackpadClick.handle == XR_NULL_HANDLE) return false;
    m_actions["right_trackpad_click"] = rightTrackpadClick;

    XrInputAction rightTrackpadY = CreateFloatAction("right_trackpad_y", "Right Trackpad Y");
    if (rightTrackpadY.handle == XR_NULL_HANDLE) return false;
    m_actions["right_trackpad_y"] = rightTrackpadY;

    // Vive wand trackpad actions (touch + position for movement/turning, click for actions)
    XrInputAction leftTrackpadTouch = CreateBooleanAction("left_trackpad_touch", "Left Trackpad Touch");
    if (leftTrackpadTouch.handle == XR_NULL_HANDLE) return false;
    m_actions["left_trackpad_touch"] = leftTrackpadTouch;

    XrInputAction leftTrackpadX = CreateFloatAction("left_trackpad_x", "Left Trackpad X");
    if (leftTrackpadX.handle == XR_NULL_HANDLE) return false;
    m_actions["left_trackpad_x"] = leftTrackpadX;

    XrInputAction leftTrackpadY = CreateFloatAction("left_trackpad_y", "Left Trackpad Y");
    if (leftTrackpadY.handle == XR_NULL_HANDLE) return false;
    m_actions["left_trackpad_y"] = leftTrackpadY;

    XrInputAction rightTrackpadX = CreateFloatAction("right_trackpad_x", "Right Trackpad X");
    if (rightTrackpadX.handle == XR_NULL_HANDLE) return false;
    m_actions["right_trackpad_x"] = rightTrackpadX;

    XrInputAction rightTrackpadTouch = CreateBooleanAction("right_trackpad_touch", "Right Trackpad Touch");
    if (rightTrackpadTouch.handle == XR_NULL_HANDLE) return false;
    m_actions["right_trackpad_touch"] = rightTrackpadTouch;

    return true;
}

bool COpenXRInputManager::CreateInteractionProfiles()
{
    bool success = false;
    
    // Try to create Valve Index profile
    success |= CreateIndexControllerProfile();
    
    // Try to create Quest controller profile
    success |= CreateQuestControllerProfile();
    
    // Try to create HP Reverb G2 controller profile
    success |= CreateHPReverbControllerProfile();
    
    // Try to create HTC Vive wand controller profile
    success |= CreateViveControllerProfile();
    
    // Try to create standard WMR controller profile (Samsung Odyssey, Lenovo Explorer, etc.)
    success |= CreateWMRControllerProfile();
    
    // Try minimal Quest profile for debugging if full profile fails
    if (!success)
    {
        success |= CreateMinimalQuestControllerProfile();
    }
    
    // Fall back to generic profile if all specific profiles fail
    if (!success)
    {
        success |= CreateGenericControllerProfile();
    }
    
    return success;
}

bool COpenXRInputManager::CreateIndexControllerProfile()
{
    XrPath indexProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/valve/index_controller", &indexProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for Valve Index profile: %d\n", result);
        return false;
    }

    // Create suggested bindings for Index controller
    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Movement bindings (left controller)
    if (m_actions.find("move_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("move_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Turning bindings (right controller)
    if (m_actions.find("turn_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["turn_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Primary attack (right trigger)
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Secondary attack (left trigger)
    if (m_actions.find("secondary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["secondary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Voice chat (left trigger - same as secondary, used for voice gesture or direct voice)
    if (m_actions.find("voice") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["voice"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Use (right A button)
    if (m_actions.find("use") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/a/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["use"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Duck (left A button)
    if (m_actions.find("duck") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/a/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["duck"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Jump (right B button)
    if (m_actions.find("jump") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["jump"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Menu (left Y button)
    if (m_actions.find("menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/b/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left UI interaction binding (left trigger value for Index)
    if (m_actions.find("left_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left class menu binding (left A button)
    if (m_actions.find("left_class_menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/a/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_class_menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right UI interaction binding (right trigger value for Index)
    if (m_actions.find("right_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon switching bindings (right stick tilt forward/backward)
    if (m_actions.find("weapon_switch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_switch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Left grip button binding (for two-handed weapon gripping) - Index squeeze/value
    if (m_actions.find("left_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip force — activation (requires deliberate squeeze on Index)
    if (m_actions.find("right_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/force", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip value — release detection (instant binary on Index)
    if (m_actions.find("right_grip_value") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip_value"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Scoreboard binding (left thumbstick click for Index - also available via trackpad force)
    if (m_actions.find("scoreboard") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["scoreboard"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Trackpad force bindings for Index (trackpad has force sensor, not click button)
    // These are used to simulate button presses via force threshold
    if (m_actions.find("left_trackpad_force") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/force", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_trackpad_force"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_trackpad_force") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/force", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_force"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Pose action bindings for controller tracking
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Grip pose action bindings for controller tracking
    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }
    
    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    AddPalmPoseBindings(suggestedBindings);

    // Suggest bindings for Index controller
    return SuggestBindings(indexProfilePath, suggestedBindings, "Valve Index");
}

bool COpenXRInputManager::CreateQuestControllerProfile()
{
    XrPath questProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &questProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for Quest controller profile: %d\n", result);
        return false;
    }

    // Create suggested bindings for Quest controller
    // Using only well-supported paths according to OpenXR spec for Oculus Touch
    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Movement bindings (left controller thumbstick)
    if (m_actions.find("move_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("move_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Turning bindings (right controller thumbstick)
    if (m_actions.find("turn_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["turn_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Primary attack (right trigger)
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Secondary attack (left trigger)  
    if (m_actions.find("secondary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["secondary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Voice chat (left trigger - same as secondary, used for voice gesture or direct voice)
    if (m_actions.find("voice") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["voice"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Use action not bound for now - will be added later

    // Jump (right B button)
    if (m_actions.find("jump") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["jump"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Menu (left Y button)
    if (m_actions.find("menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/y/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Duck/Crouch (right A button)
    if (m_actions.find("duck") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/a/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["duck"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Left UI interaction binding (left trigger value - will use threshold)
    if (m_actions.find("left_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Right UI interaction binding (right trigger value - will use threshold)
    if (m_actions.find("right_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Left class menu binding (left X button)
    if (m_actions.find("left_class_menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/x/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_class_menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Weapon switching bindings (right stick tilt forward/backward)
    if (m_actions.find("weapon_switch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_switch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Left grip button binding (for two-handed weapon gripping) - Quest uses squeeze
    if (m_actions.find("left_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip — Quest has no force sensor so both map to squeeze/value
    if (m_actions.find("right_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_grip_value") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip_value"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon select hold binding (right thumbstick click for Quest)
    if (m_actions.find("weapon_select_hold") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_select_hold"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Scoreboard binding (left thumbstick click for Quest)
    if (m_actions.find("scoreboard") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["scoreboard"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Pose action bindings for controller tracking
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    // Grip pose action bindings for controller tracking
    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }
    
    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    AddPalmPoseBindings(suggestedBindings);

    // Suggest bindings for Quest controller
    return SuggestBindings(questProfilePath, suggestedBindings, "Quest Touch");
}

bool COpenXRInputManager::CreateMinimalQuestControllerProfile()
{
    XrPath questProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &questProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for minimal Quest controller profile: %d\n", result);
        return false;
    }

    // Create minimal bindings for Quest controller - only essential controls and poses
    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Just basic movement and one trigger for testing
    if (m_actions.find("move_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("move_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // One trigger for primary attack
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Pose action bindings for controller tracking (most important for debugging)
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);

        }
    }

    AddPalmPoseBindings(suggestedBindings);

    // Suggest minimal bindings for Quest controller
    return SuggestBindings(questProfilePath, suggestedBindings, "Quest Touch (Minimal)");
}

bool COpenXRInputManager::CreateWMRControllerProfile()
{
    XrPath wmrProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/microsoft/motion_controller", &wmrProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for WMR controller profile: %d\n", result);
        return false;
    }

    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Movement bindings (left thumbstick)
    if (m_actions.find("move_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("move_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Turning (right thumbstick)
    if (m_actions.find("turn_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["turn_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Primary attack (right trigger)
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Secondary attack (left trigger)
    if (m_actions.find("secondary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["secondary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Voice chat (left trigger)
    if (m_actions.find("voice") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["voice"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right trackpad click + Y for split-trackpad: top half = jump, bottom half = duck
    // Actual jump/duck routing is handled in PollInput() based on trackpad Y position
    if (m_actions.find("right_trackpad_click") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_click"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_trackpad_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Menu (left menu button)
    if (m_actions.find("menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left UI interaction (left trigger)
    if (m_actions.find("left_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right UI interaction (right trigger)
    if (m_actions.find("right_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left class menu (left trackpad click)
    if (m_actions.find("left_class_menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_class_menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon switching (right thumbstick y)
    if (m_actions.find("weapon_switch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_switch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left grip (squeeze/click - auto-converts boolean to float 0/1)
    if (m_actions.find("left_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip (squeeze/click)
    if (m_actions.find("right_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_grip_value") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip_value"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon select hold (right thumbstick click)
    if (m_actions.find("weapon_select_hold") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_select_hold"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Scoreboard (left thumbstick click)
    if (m_actions.find("scoreboard") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["scoreboard"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Pose bindings
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    AddPalmPoseBindings(suggestedBindings);

    return SuggestBindings(wmrProfilePath, suggestedBindings, "WMR Motion Controller");
}

bool COpenXRInputManager::CreateViveControllerProfile()
{
    XrPath viveProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/htc/vive_controller", &viveProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for Vive controller profile: %d\n", result);
        return false;
    }

    std::vector<XrActionSuggestedBinding> suggestedBindings;

    // === LEFT CONTROLLER ===

    // Secondary attack (left trigger)
    if (m_actions.find("secondary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["secondary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Voice chat (left trigger)
    if (m_actions.find("voice") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["voice"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left UI interaction (left trigger)
    if (m_actions.find("left_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left trackpad position + touch (for movement via PollInput synthesis)
    if (m_actions.find("left_trackpad_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_trackpad_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("left_trackpad_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_trackpad_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("left_trackpad_touch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/touch", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_trackpad_touch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Scoreboard / medic call (left trackpad click)
    if (m_actions.find("scoreboard") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trackpad/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["scoreboard"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left grip (squeeze - boolean maps to float as 0/1)
    if (m_actions.find("left_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Menu / escape (left menu button)
    if (m_actions.find("menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // === RIGHT CONTROLLER ===

    // Primary attack (right trigger)
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right UI interaction (right trigger)
    if (m_actions.find("right_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right trackpad position + touch (turning via PollInput, click zones for jump/duck/weapon select)
    if (m_actions.find("right_trackpad_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_trackpad_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_trackpad_click") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_click"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_trackpad_touch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trackpad/touch", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_trackpad_touch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip (squeeze - boolean maps to float as 0/1)
    if (m_actions.find("right_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_grip_value") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip_value"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Class/team menu (right menu button)
    if (m_actions.find("left_class_menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/menu/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_class_menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // === POSE BINDINGS ===

    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    AddPalmPoseBindings(suggestedBindings);

    return SuggestBindings(viveProfilePath, suggestedBindings, "HTC Vive Controller");
}

bool COpenXRInputManager::CreateHPReverbControllerProfile()
{
    XrPath hpProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/hp/mixed_reality_controller", &hpProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for HP Reverb controller profile: %d\n", result);
        return false;
    }

    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Movement bindings (left thumbstick)
    if (m_actions.find("move_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("move_y") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["move_y"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Turning (right thumbstick)
    if (m_actions.find("turn_x") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["turn_x"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Primary attack (right trigger)
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Secondary attack (left trigger)
    if (m_actions.find("secondary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["secondary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Voice chat (left trigger)
    if (m_actions.find("voice") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["voice"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Jump (right B button)
    if (m_actions.find("jump") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["jump"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Duck (right A button)
    if (m_actions.find("duck") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/a/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["duck"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Menu (left Y button)
    if (m_actions.find("menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/y/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left UI interaction (left trigger)
    if (m_actions.find("left_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right UI interaction (right trigger)
    if (m_actions.find("right_ui_interact") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_ui_interact"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left class menu (left X button)
    if (m_actions.find("left_class_menu") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/x/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_class_menu"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon switching (right thumbstick y)
    if (m_actions.find("weapon_switch") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_switch"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Left grip (squeeze/value - G2 has analog squeeze)
    if (m_actions.find("left_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Right grip (squeeze/value)
    if (m_actions.find("right_grip") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("right_grip_value") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_grip_value"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Weapon select hold (right thumbstick click)
    if (m_actions.find("weapon_select_hold") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["weapon_select_hold"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Scoreboard (left thumbstick click)
    if (m_actions.find("scoreboard") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/thumbstick/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["scoreboard"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Pose bindings
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    AddPalmPoseBindings(suggestedBindings);

    return SuggestBindings(hpProfilePath, suggestedBindings, "HP Reverb G2");
}

bool COpenXRInputManager::CreateGenericControllerProfile()
{
    XrPath genericProfilePath;
    
    XrResult result = xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", &genericProfilePath);
    if (!XR_SUCCEEDED(result)) 
    {
        DevMsg("Failed to create path for generic profile: %d\n", result);
        return false;
    }

    // Create basic bindings for generic controller (simplified set)
    std::vector<XrActionSuggestedBinding> suggestedBindings;
    
    // Basic movement and actions for simple controller
    if (m_actions.find("primary_attack") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/select/click", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["primary_attack"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    // Pose bindings
    if (m_actions.find("left_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/aim/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    // Grip pose bindings for generic controller
    if (m_actions.find("left_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["left_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }
    
    if (m_actions.find("right_hand_grip_pose") != m_actions.end())
    {
        XrPath bindingPath;
        if (XR_SUCCEEDED(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &bindingPath)))
        {
            XrActionSuggestedBinding binding;
            binding.action = m_actions["right_hand_grip_pose"].handle;
            binding.binding = bindingPath;
            suggestedBindings.push_back(binding);
        }
    }

    AddPalmPoseBindings(suggestedBindings);

    // Suggest bindings for generic controller
    return SuggestBindings(genericProfilePath, suggestedBindings, "Generic");
}

bool COpenXRInputManager::SuggestBindings(XrPath profilePath, const std::vector<XrActionSuggestedBinding>& bindings, const char* profileName)
{
    if (bindings.empty())
    {
        DevMsg("No bindings to suggest for %s profile\n", profileName);
        return false;
    }





    XrInteractionProfileSuggestedBinding suggestedBindingsInfo{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    suggestedBindingsInfo.interactionProfile = profilePath;
    suggestedBindingsInfo.suggestedBindings = bindings.data();
    suggestedBindingsInfo.countSuggestedBindings = bindings.size();

    XrResult result = xrSuggestInteractionProfileBindings(m_instance, &suggestedBindingsInfo);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to suggest bindings for %s profile: %d (XR_ERROR_PATH_UNSUPPORTED = -22)\n", profileName, result);
        
        // Try fallback approach for unsupported paths
        if (result == -22) // XR_ERROR_PATH_UNSUPPORTED
        {
            DevMsg("Some binding paths unsupported for %s profile, trying fallback approach\n", profileName);
            
            // Try to suggest bindings one by one to identify valid ones
            std::vector<XrActionSuggestedBinding> validBindings;
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                XrInteractionProfileSuggestedBinding singleBindingInfo{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
                singleBindingInfo.interactionProfile = profilePath;
                singleBindingInfo.suggestedBindings = &bindings[i];
                singleBindingInfo.countSuggestedBindings = 1;
                
                XrResult singleResult = xrSuggestInteractionProfileBindings(m_instance, &singleBindingInfo);
                if (XR_SUCCEEDED(singleResult))
                {
                    validBindings.push_back(bindings[i]);
                }
            }
            
            // Try to suggest just the valid bindings
            if (!validBindings.empty())
            {
                XrInteractionProfileSuggestedBinding validBindingsInfo{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
                validBindingsInfo.interactionProfile = profilePath;
                validBindingsInfo.suggestedBindings = validBindings.data();
                validBindingsInfo.countSuggestedBindings = validBindings.size();
                
                XrResult validResult = xrSuggestInteractionProfileBindings(m_instance, &validBindingsInfo);
                if (XR_SUCCEEDED(validResult))
                {
                    DevMsg("Successfully suggested %zu supported bindings for %s profile\n", validBindings.size(), profileName);
                    return true;
                }
            }
        }
        return false;
    }
    
    DevMsg("Successfully suggested %d bindings for %s profile\n", bindings.size(), profileName);
    return true;
}

bool COpenXRInputManager::AttachActionSet()
{
    XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    
    XrResult result = xrAttachSessionActionSets(m_session, &attachInfo);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to attach action set: %d\n", result);
        return false;
    }
    return true;
}

XrInputAction COpenXRInputManager::CreateBooleanAction(const char* name, const char* localizedName)
{
    XrInputAction action;
    action.name = name;           // Now properly copies the string
    action.localizedName = localizedName;  // Now properly copies the string
    action.type = XR_ACTION_TYPE_BOOLEAN_INPUT;
    action.subactionPaths = { m_leftHandPath, m_rightHandPath };

    XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy_s(actionInfo.actionName, name);
    strcpy_s(actionInfo.localizedActionName, localizedName);
    actionInfo.actionType = action.type;
    actionInfo.countSubactionPaths = action.subactionPaths.size();
    actionInfo.subactionPaths = action.subactionPaths.data();

    XrResult result = xrCreateAction(m_actionSet, &actionInfo, &action.handle);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create boolean action %s: %d\n", name, result);
        action.handle = XR_NULL_HANDLE;
    }
    return action;
}

XrInputAction COpenXRInputManager::CreateFloatAction(const char* name, const char* localizedName)
{
    XrInputAction action;
    action.name = name;           // Now properly copies the string
    action.localizedName = localizedName;  // Now properly copies the string
    action.type = XR_ACTION_TYPE_FLOAT_INPUT;
    
    // Set subaction paths based on the action name
    if (strstr(name, "left") != nullptr)
    {
        action.subactionPaths = { m_leftHandPath };
    }
    else if (strstr(name, "right") != nullptr)
    {
        action.subactionPaths = { m_rightHandPath };
    }
    else
    {
        // Default to both hands for general actions
        action.subactionPaths = { m_leftHandPath, m_rightHandPath };
    }

    XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy_s(actionInfo.actionName, name);
    strcpy_s(actionInfo.localizedActionName, localizedName);
    actionInfo.actionType = action.type;
    actionInfo.countSubactionPaths = action.subactionPaths.size();
    actionInfo.subactionPaths = action.subactionPaths.data();

    XrResult result = xrCreateAction(m_actionSet, &actionInfo, &action.handle);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create float action %s: %d\n", name, result);
        action.handle = XR_NULL_HANDLE;
    }
    return action;
}

XrInputAction COpenXRInputManager::CreatePoseAction(const char* name, const char* localizedName)
{
    XrInputAction action;
    action.name = name;
    action.localizedName = localizedName;
    action.type = XR_ACTION_TYPE_POSE_INPUT;
    action.handle = XR_NULL_HANDLE;

    XrActionCreateInfo createInfo{ XR_TYPE_ACTION_CREATE_INFO };
    strcpy_s(createInfo.actionName, name);
    strcpy_s(createInfo.localizedActionName, localizedName);
    createInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    
    // Pose actions need subaction paths for left/right hands
    if (strstr(name, "left") != nullptr)
    {
        createInfo.countSubactionPaths = 1;
        createInfo.subactionPaths = &m_leftHandPath;
    }
    else if (strstr(name, "right") != nullptr)
    {
        createInfo.countSubactionPaths = 1;
        createInfo.subactionPaths = &m_rightHandPath;
    }
    else
    {
        createInfo.countSubactionPaths = 0;
        createInfo.subactionPaths = nullptr;
    }

    XrResult result = xrCreateAction(m_actionSet, &createInfo, &action.handle);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create pose action %s: %d\n", name, result);
        action.handle = XR_NULL_HANDLE;
        return action;
    }

    // Create action space for this pose action
    XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    actionSpaceInfo.action = action.handle;
    
    // Set the appropriate subaction path for the action space
    if (strstr(name, "left") != nullptr)
    {
        actionSpaceInfo.subactionPath = m_leftHandPath;
    }
    else if (strstr(name, "right") != nullptr)
    {
        actionSpaceInfo.subactionPath = m_rightHandPath;
    }
    else
    {
        actionSpaceInfo.subactionPath = XR_NULL_PATH;
    }
    
    actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
    actionSpaceInfo.poseInActionSpace.orientation.x = 0.0f;
    actionSpaceInfo.poseInActionSpace.orientation.y = 0.0f;
    actionSpaceInfo.poseInActionSpace.orientation.z = 0.0f;
    actionSpaceInfo.poseInActionSpace.position.x = 0.0f;
    actionSpaceInfo.poseInActionSpace.position.y = 0.0f;
    actionSpaceInfo.poseInActionSpace.position.z = 0.0f;

    XrSpace actionSpace;
    result = xrCreateActionSpace(m_session, &actionSpaceInfo, &actionSpace);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to create action space for %s: %d\n", name, result);
        xrDestroyAction(action.handle);
        action.handle = XR_NULL_HANDLE;
        return action;
    }

    // Store the action space
    m_actionSpaces[name] = actionSpace;

    return action;
}

void COpenXRInputManager::AddPalmPoseBindings(std::vector<XrActionSuggestedBinding>& bindings)
{
    if (m_actions.find("left_palm_pose") == m_actions.end())
        return;
    
    const char* leftPath = m_manager->IsGripSurfaceAvailable() ?
        "/user/hand/left/input/grip_surface/pose" : "/user/hand/left/input/palm_ext/pose";
    const char* rightPath = m_manager->IsGripSurfaceAvailable() ?
        "/user/hand/right/input/grip_surface/pose" : "/user/hand/right/input/palm_ext/pose";
    
    XrPath leftBindingPath, rightBindingPath;
    if (XR_SUCCEEDED(xrStringToPath(m_instance, leftPath, &leftBindingPath)))
    {
        XrActionSuggestedBinding binding;
        binding.action = m_actions["left_palm_pose"].handle;
        binding.binding = leftBindingPath;
        bindings.push_back(binding);
    }
    if (XR_SUCCEEDED(xrStringToPath(m_instance, rightPath, &rightBindingPath)))
    {
        XrActionSuggestedBinding binding;
        binding.action = m_actions["right_palm_pose"].handle;
        binding.binding = rightBindingPath;
        bindings.push_back(binding);
    }
}

void COpenXRInputManager::PollInput()
{
    static int frameCount = 0;
    frameCount++;
    m_previousButtonStates = m_currentButtonStates;
    m_previousAnalogStates = m_currentAnalogStates;

    XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
    XrActiveActionSet activeActionSet{ m_actionSet };
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeActionSet;
    
    XrResult result = xrSyncActions(m_session, &syncInfo);
    if (!XR_SUCCEEDED(result))
    {
        DevMsg("Failed to sync actions: %d\n", result);
        return;
    }

    for (const auto& action : m_actions)
    {
        if (action.second.type == XR_ACTION_TYPE_BOOLEAN_INPUT)
        {
            // Poll for both hands
            //for (size_t i = 0; i < action.second.subactionPaths.size(); ++i)
            //{
                XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = action.second.handle;

                XrActionStateBoolean state{ XR_TYPE_ACTION_STATE_BOOLEAN };
                result = xrGetActionStateBoolean(m_session, &getInfo, &state);
                if (XR_SUCCEEDED(result))
                {
                    std::string key = action.first;
                    bool previousState = m_currentButtonStates[key];
                    m_currentButtonStates[key] = state.currentState;
                    
                    // Store button states (no logging to reduce noise)
                }
            //}
        }
        else if (action.second.type == XR_ACTION_TYPE_FLOAT_INPUT)
        {
            // Poll for both hands
            //for (size_t i = 0; i < action.second.subactionPaths.size(); ++i)
            //{
                XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = action.second.handle;

                XrActionStateFloat state{ XR_TYPE_ACTION_STATE_FLOAT };
                result = xrGetActionStateFloat(m_session, &getInfo, &state);
                if (XR_SUCCEEDED(result))
                {
                    std::string key = action.first;
                    float previousValue = m_currentAnalogStates[key];
                    m_currentAnalogStates[key] = state.currentState;
                    
                    // Store analog values (no logging to reduce noise)
                }
            //}
        }
        else if (action.second.type == XR_ACTION_TYPE_POSE_INPUT)
        {
            // Poll pose state for both hands
            XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = action.second.handle;

            XrActionStatePose state{ XR_TYPE_ACTION_STATE_POSE };
            result = xrGetActionStatePose(m_session, &getInfo, &state);
            if (XR_SUCCEEDED(result))
            {
                std::string key = action.first;
                m_currentPoseValidStates[key] = state.isActive;
                
                // If pose is active, get the current pose
                if (state.isActive)
                {
                    // Get pose relative to reference space using the action's space
                    XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION };
                    XrSpaceLocationFlags requiredFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
                    
                    // Get pose relative to reference space
                    auto actionSpaceIt = m_actionSpaces.find(key);
                    if (actionSpaceIt != m_actionSpaces.end())
                    {
                        // Get the current frame time from the DXVK layer instead of the manager's frame state
                        XrTime currentTime = 0;
                        dxvkGetPredictedDisplayTime(currentTime);
                        
                        if (currentTime == 0)
                        {
                            continue;
                        }
                        
                        result = xrLocateSpace(actionSpaceIt->second, m_manager->GetReferenceSpace(), 
                                             currentTime, &spaceLocation);
                        if (XR_SUCCEEDED(result) && (spaceLocation.locationFlags & requiredFlags) == requiredFlags)
                        {
                            m_currentPoseStates[key] = spaceLocation.pose;
                        }
                        // Pose location failed - silent handling
                    }
                }
            }
            // Failed to get pose state - silent handling
        }
    }

    // Convert trackpad force to boolean states for Index controllers
    // These set the scoreboard and weapon_select_hold button states when trackpad force exceeds threshold
    // Only applies when trackpad force actions are active (Index controller connected)
    const float TRACKPAD_FORCE_THRESHOLD = 0.25f;
    
    // Check if left trackpad force action is active (has valid binding on current controller)
    // OR with existing scoreboard state (from thumbstick click) so either input works
    auto leftForceActionIt = m_actions.find("left_trackpad_force");
    if (leftForceActionIt != m_actions.end())
    {
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = leftForceActionIt->second.handle;
        XrActionStateFloat state{ XR_TYPE_ACTION_STATE_FLOAT };
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &state)) && state.isActive)
        {
            // Trackpad force action is active - OR with existing scoreboard state
            bool trackpadPressed = (state.currentState > TRACKPAD_FORCE_THRESHOLD);
            m_currentButtonStates["scoreboard"] = m_currentButtonStates["scoreboard"] || trackpadPressed;
        }
    }
    
    // Check if right trackpad force action is active (has valid binding on current controller)
    auto rightForceActionIt = m_actions.find("right_trackpad_force");
    if (rightForceActionIt != m_actions.end())
    {
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = rightForceActionIt->second.handle;
        XrActionStateFloat state{ XR_TYPE_ACTION_STATE_FLOAT };
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &state)) && state.isActive)
        {
            // Trackpad force action is active - use it for weapon_select_hold
            m_currentButtonStates["weapon_select_hold"] = (state.currentState > TRACKPAD_FORCE_THRESHOLD);
        }
    }

    // Vive wand trackpad handling
    // left_trackpad_touch is only bound in the Vive profile, so isActive serves as controller detection
    bool isViveController = false;
    auto leftPadTouchIt = m_actions.find("left_trackpad_touch");
    if (leftPadTouchIt != m_actions.end())
    {
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = leftPadTouchIt->second.handle;
        XrActionStateBoolean touchState{ XR_TYPE_ACTION_STATE_BOOLEAN };
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &touchState)) && touchState.isActive)
        {
            isViveController = true;

            // Left trackpad: touch → movement (always, even during click for scoreboard)
            if (touchState.currentState)
            {
                m_currentAnalogStates["move_x"] = m_currentAnalogStates["left_trackpad_x"];
                m_currentAnalogStates["move_y"] = m_currentAnalogStates["left_trackpad_y"];
            }

            // Right trackpad touch: X axis → turning, Y axis → jump (top) / duck (bottom)
            // Right trackpad click: weapon select (position-independent)
            bool rightTouched = m_currentButtonStates["right_trackpad_touch"];
            bool rightClicked = m_currentButtonStates["right_trackpad_click"];

            if (rightTouched && !rightClicked)
            {
                float padX = m_currentAnalogStates["right_trackpad_x"];
                float padY = m_currentAnalogStates["right_trackpad_y"];

                if (padY > 0.5f)
                    m_currentButtonStates["jump"] = true;
                else if (padY < -0.5f)
                    m_currentButtonStates["duck"] = true;
                else
                    m_currentAnalogStates["turn_x"] = padX;
            }

            if (rightClicked)
            {
                m_currentButtonStates["weapon_select_hold"] = true;
            }
        }
    }

    // WMR split-trackpad: top half = jump, bottom half = duck
    // Only active when right_trackpad_click has a valid binding (WMR controllers, not Vive)
    if (!isViveController)
    {
        auto trackpadClickIt = m_actions.find("right_trackpad_click");
        auto trackpadYIt = m_actions.find("right_trackpad_y");
        if (trackpadClickIt != m_actions.end() && trackpadYIt != m_actions.end())
        {
            XrActionStateGetInfo clickGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
            clickGetInfo.action = trackpadClickIt->second.handle;
            XrActionStateBoolean clickState{ XR_TYPE_ACTION_STATE_BOOLEAN };

            XrActionStateGetInfo yGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
            yGetInfo.action = trackpadYIt->second.handle;
            XrActionStateFloat yState{ XR_TYPE_ACTION_STATE_FLOAT };

            if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &clickGetInfo, &clickState)) && clickState.isActive &&
                XR_SUCCEEDED(xrGetActionStateFloat(m_session, &yGetInfo, &yState)) && yState.isActive)
            {
                if (clickState.currentState)
                {
                    if (yState.currentState > 0.0f)
                        m_currentButtonStates["jump"] = true;
                    else
                        m_currentButtonStates["duck"] = true;
                }
            }
        }
    }
}

bool COpenXRInputManager::IsButtonPressed(const char* actionName)
{
    auto it = m_currentButtonStates.find(actionName);
    return it != m_currentButtonStates.end() && it->second;
}

bool COpenXRInputManager::WasButtonPressed(const char* actionName)
{
    auto prevIt = m_previousButtonStates.find(actionName);
    auto currIt = m_currentButtonStates.find(actionName);
    return prevIt != m_previousButtonStates.end() && 
           currIt != m_currentButtonStates.end() && 
           !prevIt->second && currIt->second;
}

bool COpenXRInputManager::WasButtonReleased(const char* actionName)
{
    auto prevIt = m_previousButtonStates.find(actionName);
    auto currIt = m_currentButtonStates.find(actionName);
    return prevIt != m_previousButtonStates.end() && 
           currIt != m_currentButtonStates.end() && 
           prevIt->second && !currIt->second;
}

float COpenXRInputManager::GetAnalogValue(const char* actionName)
{
    auto it = m_currentAnalogStates.find(actionName);
    return it != m_currentAnalogStates.end() ? it->second : 0.0f;
}

bool COpenXRInputManager::GetControllerPose(const char* actionName, XrPosef& pose)
{
    auto it = m_currentPoseStates.find(actionName);
    if (it != m_currentPoseStates.end())
    {
        pose = it->second;
        return true;
    }
    return false;
}

bool COpenXRInputManager::IsControllerPoseValid(const char* actionName)
{
    auto it = m_currentPoseValidStates.find(actionName);
    return it != m_currentPoseValidStates.end() && it->second;
}

XrSpace COpenXRInputManager::GetActionSpace(const char* actionName)
{
    auto it = m_actionSpaces.find(actionName);
    if (it != m_actionSpaces.end()) {
        return it->second;
    }
    return XR_NULL_HANDLE;
}

bool COpenXRInputManager::SamplePoseNow(const char* actionName, XrPosef& pose)
{
    // Get the action space for this pose action
    XrSpace actionSpace = GetActionSpace(actionName);
    if (actionSpace == XR_NULL_HANDLE)
        return false;
    
    XrSpace refSpace = m_manager->GetReferenceSpace();
    if (refSpace == XR_NULL_HANDLE)
        return false;
    
    // Get the current predicted display time directly from DXVK
    XrTime currentTime = 0;
    dxvkGetPredictedDisplayTime(currentTime);
    
    if (currentTime == 0)
        return false;
    
    // Sample the pose directly from OpenXR
    XrSpaceLocation spaceLocation = {XR_TYPE_SPACE_LOCATION};
    XrResult result = xrLocateSpace(actionSpace, refSpace, currentTime, &spaceLocation);
    
    XrSpaceLocationFlags requiredFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if (XR_SUCCEEDED(result) && (spaceLocation.locationFlags & requiredFlags) == requiredFlags)
    {
        pose = spaceLocation.pose;
        return true;
    }
    
    return false;
}

bool COpenXRInputManager::IsUIInteractionPressed(const char* actionName, float threshold)
{
    auto it = m_currentAnalogStates.find(actionName);
    return it != m_currentAnalogStates.end() && it->second >= threshold;
}

bool COpenXRInputManager::WasUIInteractionPressed(const char* actionName, float threshold)
{
    auto prevIt = m_previousAnalogStates.find(actionName);
    auto currIt = m_currentAnalogStates.find(actionName);
    return prevIt != m_previousAnalogStates.end() && 
           currIt != m_currentAnalogStates.end() && 
           prevIt->second < threshold && currIt->second >= threshold;
}

bool COpenXRInputManager::WasUIInteractionReleased(const char* actionName, float threshold)
{
    auto prevIt = m_previousAnalogStates.find(actionName);
    auto currIt = m_currentAnalogStates.find(actionName);
    return prevIt != m_previousAnalogStates.end() && 
           currIt != m_currentAnalogStates.end() && 
           prevIt->second >= threshold && currIt->second < threshold;
}

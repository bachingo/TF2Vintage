#ifndef OPENXR_INPUT_H
#define OPENXR_INPUT_H

#include "../public/openxr/openxr.h"
#include <string>
#include <map>
#include <vector>

class COpenXRManager;

struct XrInputAction {
    std::string name;
    std::string localizedName;
    XrActionType type;
    XrAction handle;
    std::vector<XrPath> subactionPaths;
};

class COpenXRInputManager {
public:
    COpenXRInputManager(COpenXRManager* manager);
    ~COpenXRInputManager();

    bool Initialize();
    void Shutdown();
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
    bool GetControllerPose(const char* actionName, XrPosef& pose);
    bool IsControllerPoseValid(const char* actionName);
    
    // Get action space handle for direct sampling
    XrSpace GetActionSpace(const char* actionName);
    
    // Sample a fresh pose directly from OpenXR (bypasses cache for lowest latency)
    bool SamplePoseNow(const char* actionName, XrPosef& pose);

private:
    bool CreateActionSet();
    bool CreateActions();
    bool CreateInteractionProfiles();
    bool AttachActionSet();

    // Individual interaction profile creation methods
    bool CreateIndexControllerProfile();
    bool CreateQuestControllerProfile();
    bool CreateMinimalQuestControllerProfile();
    bool CreateWMRControllerProfile();
    bool CreateViveControllerProfile();
    bool CreateHPReverbControllerProfile();
    bool CreateGenericControllerProfile();
    bool SuggestBindings(XrPath profilePath, const std::vector<XrActionSuggestedBinding>& bindings, const char* profileName);

    XrInputAction CreateBooleanAction(const char* name, const char* localizedName);
    XrInputAction CreateFloatAction(const char* name, const char* localizedName);
    XrInputAction CreatePoseAction(const char* name, const char* localizedName);
    void AddPalmPoseBindings(std::vector<XrActionSuggestedBinding>& bindings);

    COpenXRManager* m_manager;
    XrInstance m_instance;
    XrSession m_session;
    XrActionSet m_actionSet;

    // Input state
    std::map<std::string, XrInputAction> m_actions;
    std::map<std::string, bool> m_previousButtonStates;
    std::map<std::string, bool> m_currentButtonStates;
    std::map<std::string, float> m_previousAnalogStates;
    std::map<std::string, float> m_currentAnalogStates;
    std::map<std::string, XrPosef> m_currentPoseStates;
    std::map<std::string, bool> m_currentPoseValidStates;
    std::map<std::string, XrSpace> m_actionSpaces;

    // Common paths
    XrPath m_leftHandPath;
    XrPath m_rightHandPath;
};

#endif // OPENXR_INPUT_H 
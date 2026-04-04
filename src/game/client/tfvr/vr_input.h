#ifndef VR_INPUT_H
#define VR_INPUT_H

#include "cbase.h"
#include "input.h"
#include "usercmd.h"
#include "openxr_manager.h"
#include "iinput.h"
#include "tfvr/tfvr_vr_throw.h"

// Forward declarations
class CInput;
class IInput;
class C_TFWeaponBase;

// Global input pointer declaration
extern IInput* input;

class CVRInput : public CInput
{
public:
    CVRInput();
    virtual ~CVRInput();

    // Override input processing functions
    void CreateMove(int sequence_number, float input_sample_frametime, bool active) override;
    void ExtraMouseSample(float frametime, bool active) override;

    static void CopyVRPosesToUserCmd(CUserCmd* cmd);

protected:
    // Helper functions for VR input processing
    void ProcessVRControllerInput(CUserCmd* cmd);
    void ProcessVRViewAngles(CUserCmd* cmd);
    void ProcessVRMovement(CUserCmd* cmd, float frametime);
    void ProcessVRControllerTracking(CUserCmd* cmd);

    // Physical throw gesture handling
    void ProcessThrowGesture(CUserCmd* cmd, bool bTriggerHeld, bool bSuppressTrigger);
    static bool IsThrowableWeapon( C_TFWeaponBase *pWeapon );

    // Mouth proximity activation for lunchbox items and soldier horns
    bool IsWeaponNearMouth( C_TFWeaponBase *pWeapon );
    static bool IsMouthActivatedWeapon( C_TFWeaponBase *pWeapon );

private:
    CVRVelocityTracker m_throwTracker;
    bool m_bThrowHolding;
    int m_nLastThrowableWeaponID;
    bool m_bInCreateMove;
};

// Global instances - declared after class definition
extern CVRInput g_VRInput;
extern IInput* g_OriginalNonVRInputPtr;

#endif // VR_INPUT_H 
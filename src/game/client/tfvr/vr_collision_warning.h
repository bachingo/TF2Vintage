#ifndef VR_COLLISION_WARNING_H
#define VR_COLLISION_WARNING_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vmatrix.h"

namespace vgui { class Panel; }

//-----------------------------------------------------------------------------
// Purpose: Floating "Please move back or recalibrate" text that appears in VR
//          when the server detects sustained head collision / position desync.
//          Uses the same spring-follow positioning as the popup HUD.
//-----------------------------------------------------------------------------
class CVRCollisionWarningManager
{
public:
    CVRCollisionWarningManager();
    ~CVRCollisionWarningManager();

    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render();

private:
    bool CalculateTransform(VMatrix& transform);
    float GetCurrentViewYaw() const;
    void UpdateSpringYaw(float deltaTime);

    bool m_bInitialized;

    vgui::Panel* m_pLabel;

    // Spring arm state
    float m_flCurrentYaw;
    float m_flTargetYaw;

    // Configuration
    float m_flDistance;
    float m_flFollowSpeed;
    float m_flDeadzone;
    float m_flMaxLagAngle;
    float m_flVerticalOffset;
    float m_flScale;
};

extern CVRCollisionWarningManager* g_pVRCollisionWarningManager;

#endif // VR_COLLISION_WARNING_H

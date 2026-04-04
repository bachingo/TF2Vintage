//=============================================================================
// TF2VR - VR Collision Warning Overlay
// Floating "Please move back or recalibrate" text with spring-follow positioning
//=============================================================================

#include "cbase.h"
#include "vr_collision_warning.h"
#include "vr_world_ui_queue.h"
#include "c_tf_player.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tier0/vprof.h"
#include "client_virtualreality.h"
#include "openxr_manager.h"
#include "sourcevr/isourcevirtualreality.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CVRCollisionWarningManager* g_pVRCollisionWarningManager = nullptr;

//=============================================================================
// ConVars
//=============================================================================

ConVar tfvr_collision_warning_distance("tfvr_collision_warning_distance", "80", FCVAR_ARCHIVE,
    "Distance of collision warning from head in units");
ConVar tfvr_collision_warning_scale("tfvr_collision_warning_scale", "25", FCVAR_ARCHIVE,
    "World-space width of the collision warning panel");
ConVar tfvr_collision_warning_vertical("tfvr_collision_warning_vertical", "-5", FCVAR_ARCHIVE,
    "Vertical offset of collision warning (positive = up)");
ConVar tfvr_collision_warning_follow_speed("tfvr_collision_warning_follow_speed", "4.0", FCVAR_ARCHIVE,
    "How fast the warning follows head rotation");
ConVar tfvr_collision_warning_deadzone("tfvr_collision_warning_deadzone", "10.0", FCVAR_ARCHIVE,
    "Angle deadzone where warning stays locked (degrees)");
ConVar tfvr_collision_warning_max_lag("tfvr_collision_warning_max_lag", "30.0", FCVAR_ARCHIVE,
    "Maximum angle warning can lag behind view");

//=============================================================================
// Simple panel that draws centered warning text
//=============================================================================
class CVRWarningLabel : public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CVRWarningLabel, vgui::Panel);
public:
    CVRWarningLabel(vgui::Panel* parent, const char* name)
        : BaseClass(parent, name)
    {
        SetPaintBackgroundEnabled(false);
        m_hFont = vgui::INVALID_FONT;
    }

    virtual void Paint() override
    {
        if (m_hFont == vgui::INVALID_FONT)
        {
            m_hFont = vgui::surface()->CreateFont();
            vgui::surface()->SetFontGlyphSet(m_hFont, "TF2 Build", 28, 600,
                0, 0, vgui::ISurface::FONTFLAG_ANTIALIAS);
        }

        const wchar_t* text = L"Please move back or recalibrate";

        int textW, textH;
        vgui::surface()->GetTextSize(m_hFont, text, textW, textH);

        int panelW, panelH;
        GetSize(panelW, panelH);

        int x = (panelW - textW) / 2;
        int y = (panelH - textH) / 2;

        vgui::surface()->DrawSetTextFont(m_hFont);
        vgui::surface()->DrawSetTextColor(Color(255, 200, 80, 255));
        vgui::surface()->DrawSetTextPos(x, y);
        vgui::surface()->DrawPrintText(text, wcslen(text));
    }

private:
    vgui::HFont m_hFont;
};

//=============================================================================
// CVRCollisionWarningManager
//=============================================================================

CVRCollisionWarningManager::CVRCollisionWarningManager()
{
    m_bInitialized = false;
    m_pLabel = nullptr;
    m_flCurrentYaw = 0.0f;
    m_flTargetYaw = 0.0f;
    m_flDistance = 80.0f;
    m_flFollowSpeed = 4.0f;
    m_flDeadzone = 10.0f;
    m_flMaxLagAngle = 30.0f;
    m_flVerticalOffset = -5.0f;
    m_flScale = 25.0f;
}

CVRCollisionWarningManager::~CVRCollisionWarningManager()
{
    Shutdown();
}

bool CVRCollisionWarningManager::Initialize()
{
    if (m_bInitialized)
        return true;

    vgui::Panel* pViewport = g_pClientMode ? g_pClientMode->GetViewport() : nullptr;
    m_pLabel = new CVRWarningLabel(pViewport, "VRCollisionWarning");
    m_pLabel->SetSize(512, 64);
    m_pLabel->SetVisible(false);

    m_flCurrentYaw = GetCurrentViewYaw();
    m_flTargetYaw = m_flCurrentYaw;

    m_bInitialized = true;
    DevMsg("VR Collision Warning Manager: Initialized\n");
    return true;
}

void CVRCollisionWarningManager::Shutdown()
{
    if (m_pLabel)
    {
        m_pLabel->MarkForDeletion();
        m_pLabel = nullptr;
    }
    m_bInitialized = false;
}

float CVRCollisionWarningManager::GetCurrentViewYaw() const
{
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        QAngle angles;
        MatrixAngles(worldFromMideye.As3x4(), angles);
        return angles[YAW];
    }

    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
        return pPlayer->EyeAngles()[YAW];

    return 0.0f;
}

void CVRCollisionWarningManager::UpdateSpringYaw(float deltaTime)
{
    m_flTargetYaw = GetCurrentViewYaw();

    float diff = AngleDiff(m_flTargetYaw, m_flCurrentYaw);
    float absDiff = fabsf(diff);

    if (absDiff <= m_flDeadzone)
        return;

    if (absDiff > m_flMaxLagAngle)
    {
        if (diff > 0)
            m_flCurrentYaw = AngleNormalize(m_flTargetYaw - m_flMaxLagAngle);
        else
            m_flCurrentYaw = AngleNormalize(m_flTargetYaw + m_flMaxLagAngle);
        return;
    }

    float effectiveDiff = (diff > 0) ? (diff - m_flDeadzone) : (diff + m_flDeadzone);

    float t = 1.0f - expf(-m_flFollowSpeed * deltaTime);
    m_flCurrentYaw = AngleNormalize(m_flCurrentYaw + effectiveDiff * t);
}

void CVRCollisionWarningManager::Update(float deltaTime)
{
    if (!m_bInitialized)
        return;

    m_flDistance = tfvr_collision_warning_distance.GetFloat();
    m_flScale = tfvr_collision_warning_scale.GetFloat();
    m_flVerticalOffset = tfvr_collision_warning_vertical.GetFloat();
    m_flFollowSpeed = tfvr_collision_warning_follow_speed.GetFloat();
    m_flDeadzone = tfvr_collision_warning_deadzone.GetFloat();
    m_flMaxLagAngle = tfvr_collision_warning_max_lag.GetFloat();

    UpdateSpringYaw(deltaTime);
}

bool CVRCollisionWarningManager::CalculateTransform(VMatrix& transform)
{
    Vector headPos;
    if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        VMatrix worldFromMideye = g_ClientVirtualReality.GetWorldFromMidEye();
        headPos = worldFromMideye.GetTranslation();
    }
    else
    {
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (!pPlayer)
            return false;
        headPos = pPlayer->EyePosition();
    }

    Vector forward, right, up;
    AngleVectors(QAngle(0, m_flCurrentYaw, 0), &forward, &right, &up);

    Vector panelPos = headPos + forward * m_flDistance;
    panelPos.z += m_flVerticalOffset;

    transform.Identity();
    transform[0][0] = right.x;    transform[0][1] = up.x;    transform[0][2] = -forward.x;
    transform[1][0] = right.y;    transform[1][1] = up.y;    transform[1][2] = -forward.y;
    transform[2][0] = right.z;    transform[2][1] = up.z;    transform[2][2] = -forward.z;

    int pixelW, pixelH;
    m_pLabel->GetSize(pixelW, pixelH);
    float aspectRatio = (float)pixelW / (float)pixelH;
    float worldWidth = m_flScale;
    float worldHeight = worldWidth / aspectRatio;

    Vector topLeft = panelPos
        - right * (worldWidth * 0.5f)
        + up * (worldHeight * 0.5f);

    transform.SetTranslation(topLeft);
    return true;
}

static const int PRIORITY_COLLISION_WARNING = 200;

void CVRCollisionWarningManager::Render()
{
    VPROF("VRCollisionWarning_Render");

    if (!m_bInitialized || !m_pLabel)
        return;

    C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if (!pPlayer)
        return;

    if (!pPlayer->HasHeadCollisionWarning())
        return;

    VMatrix panelToWorld;
    if (!CalculateTransform(panelToWorld))
        return;

    int pixelW, pixelH;
    m_pLabel->GetSize(pixelW, pixelH);
    float aspectRatio = (float)pixelW / (float)pixelH;
    float worldWidth = m_flScale;
    float worldHeight = worldWidth / aspectRatio;

    if (g_pVRWorldUIQueue && g_pVRWorldUIQueue->IsInitialized())
    {
        g_pVRWorldUIQueue->QueuePanel(m_pLabel, panelToWorld,
                                      pixelW, pixelH,
                                      worldWidth, worldHeight,
                                      PRIORITY_COLLISION_WARNING, true, false);
    }
    else
    {
        m_pLabel->SetVisible(true);
        g_pMatSystemSurface->DisableClipping(true);
        g_pMatSystemSurface->DrawPanelIn3DSpace(
            m_pLabel->GetVPanel(),
            panelToWorld,
            pixelW, pixelH,
            worldWidth, worldHeight
        );
        g_pMatSystemSurface->DisableClipping(false);
        m_pLabel->SetVisible(false);
    }
}

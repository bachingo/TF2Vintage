//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VR Spectator Camera - Implements Half-Life: Alyx style camera
//          smoothing for streaming and trailer recording.
//
//=============================================================================

#include "cbase.h"
#include "vr_spectator_camera.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// ConVars - Following Half-Life: Alyx naming conventions
//-----------------------------------------------------------------------------
ConVar tfvr_spectator_mode("tfvr_spectator_mode", "0", FCVAR_ARCHIVE,
    "Spectator camera smoothing mode: 0=off, 1=mirror-only (streaming), 2=full smoothing (trailers)");

ConVar tfvr_spectator_roll_halflife("tfvr_spectator_roll_halflife", "0.5", FCVAR_ARCHIVE,
    "Roll smoothing half-life in seconds. Higher values = more smoothing. (Alyx default: 0.5)");

ConVar tfvr_spectator_yawpitch_halflife("tfvr_spectator_yawpitch_halflife", "0.09", FCVAR_ARCHIVE,
    "Yaw/pitch smoothing half-life in seconds. Higher values = more smoothing. (Alyx default: 0.09)");

ConVar tfvr_spectator_zoom("tfvr_spectator_zoom", "1.1", FCVAR_ARCHIVE,
    "Zoom factor for spectator mirror view. 1.0 = fit horizontally. Higher values provide margin for roll rotation.");

ConVar tfvr_spectator_debug("tfvr_spectator_debug", "0", FCVAR_NONE,
    "Show debug visualization for spectator camera smoothing");

ConVar tfvr_spectator_roll_invert("tfvr_spectator_roll_invert", "0", FCVAR_ARCHIVE,
    "Invert roll compensation direction (try toggling if roll goes wrong way)");

//-----------------------------------------------------------------------------
// ConVar accessors
//-----------------------------------------------------------------------------
VRSpectatorMode_t GetVRSpectatorMode()
{
    int mode = tfvr_spectator_mode.GetInt();
    if (mode < 0 || mode > 2)
        return VR_SPECTATOR_OFF;
    return static_cast<VRSpectatorMode_t>(mode);
}

float GetVRSpectatorRollHalfLife()
{
    return MAX(0.001f, tfvr_spectator_roll_halflife.GetFloat());
}

float GetVRSpectatorYawPitchHalfLife()
{
    return MAX(0.001f, tfvr_spectator_yawpitch_halflife.GetFloat());
}

float GetVRSpectatorZoom()
{
    return clamp(tfvr_spectator_zoom.GetFloat(), 1.0f, 2.0f);
}

bool IsVRSpectatorDebugEnabled()
{
    return tfvr_spectator_debug.GetBool();
}

//-----------------------------------------------------------------------------
// Global instance
//-----------------------------------------------------------------------------
CVRSpectatorCamera* g_pVRSpectatorCamera = nullptr;

//-----------------------------------------------------------------------------
// External debug overlay
//-----------------------------------------------------------------------------
extern IVDebugOverlay* debugoverlay;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVRSpectatorCamera::CVRSpectatorCamera()
    : m_bInitialized(false)
    , m_smoothedAngles(0, 0, 0)
    , m_lastRawAngles(0, 0, 0)
    , m_flLastUpdateTime(0.0f)
    , m_bFirstFrame(true)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVRSpectatorCamera::~CVRSpectatorCamera()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the spectator camera system
//-----------------------------------------------------------------------------
bool CVRSpectatorCamera::Initialize()
{
    if (m_bInitialized)
        return true;
    
    Reset();
    m_bInitialized = true;
    
    DevMsg("VR Spectator Camera: Initialized\n");
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown the spectator camera system
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    m_bInitialized = false;
    DevMsg("VR Spectator Camera: Shutdown\n");
}

//-----------------------------------------------------------------------------
// Purpose: Reset smoothing state (e.g., on level change)
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::Reset()
{
    m_smoothedAngles.Init();
    m_lastRawAngles.Init();
    m_flLastUpdateTime = 0.0f;
    m_bFirstFrame = true;
}

//-----------------------------------------------------------------------------
// Purpose: Get current spectator mode
//-----------------------------------------------------------------------------
VRSpectatorMode_t CVRSpectatorCamera::GetMode() const
{
    return GetVRSpectatorMode();
}

//-----------------------------------------------------------------------------
// Purpose: Update smoothing each frame
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::Update(float deltaTime)
{
    VPROF_BUDGET("CVRSpectatorCamera::Update", VPROF_BUDGETGROUP_OTHER_VGUI);
    
    // Nothing to update if disabled
    if (GetMode() == VR_SPECTATOR_OFF)
    {
        m_bFirstFrame = true;
        return;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Exponential decay smoothing for a single angle component
//          halfLife: time in seconds to reach halfway to target
//          Uses formula: decay = exp(-deltaTime * ln(2) / halfLife)
//-----------------------------------------------------------------------------
float CVRSpectatorCamera::SmoothAngleComponent(float current, float target, 
                                                float halfLife, float deltaTime)
{
    if (halfLife <= 0.0f)
        return target;
    
    // Normalize the angle difference to handle wrapping
    float diff = AngleNormalize(target - current);
    
    // Exponential decay: decay factor = e^(-dt * ln(2) / halfLife)
    // ln(2) ≈ 0.693147
    float decay = expf(-deltaTime * 0.693147f / halfLife);
    
    // New value = target + (current - target) * decay
    // Which is equivalent to: current + (1 - decay) * diff
    float result = current + diff * (1.0f - decay);
    
    return AngleNormalize(result);
}

//-----------------------------------------------------------------------------
// Purpose: Apply exponential decay to full QAngle with different rates
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::SmoothAngles(const QAngle& target, QAngle& current,
                                       float rollHalfLife, float yawPitchHalfLife, 
                                       float deltaTime)
{
    // Pitch uses yaw/pitch half-life
    current.x = SmoothAngleComponent(current.x, target.x, yawPitchHalfLife, deltaTime);
    
    // Yaw uses yaw/pitch half-life
    current.y = SmoothAngleComponent(current.y, target.y, yawPitchHalfLife, deltaTime);
    
    // Roll uses roll half-life (typically higher for more smoothing)
    current.z = SmoothAngleComponent(current.z, target.z, rollHalfLife, deltaTime);
}

//-----------------------------------------------------------------------------
// Purpose: Apply smoothing to angles
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::ApplySmoothing(const QAngle& rawAngles, QAngle& smoothedAngles)
{
    VRSpectatorMode_t mode = GetMode();
    
    if (mode == VR_SPECTATOR_OFF)
    {
        smoothedAngles = rawAngles;
        m_bFirstFrame = true;
        return;
    }
    
    float currentTime = gpGlobals->realtime;
    
    // On first frame, snap to current angles
    if (m_bFirstFrame || m_flLastUpdateTime <= 0.0f)
    {
        m_smoothedAngles = rawAngles;
        m_lastRawAngles = rawAngles;
        m_flLastUpdateTime = currentTime;
        m_bFirstFrame = false;
        smoothedAngles = rawAngles;
        return;
    }
    
    // Calculate delta time
    float deltaTime = currentTime - m_flLastUpdateTime;
    deltaTime = clamp(deltaTime, 0.001f, 0.1f); // Clamp to reasonable range
    
    // Get half-life values from ConVars
    float rollHalfLife = GetVRSpectatorRollHalfLife();
    float yawPitchHalfLife = GetVRSpectatorYawPitchHalfLife();
    
    // Apply smoothing
    SmoothAngles(rawAngles, m_smoothedAngles, rollHalfLife, yawPitchHalfLife, deltaTime);
    
    // Store for next frame
    m_lastRawAngles = rawAngles;
    m_flLastUpdateTime = currentTime;
    
    smoothedAngles = m_smoothedAngles;
}

//-----------------------------------------------------------------------------
// Purpose: Get the delta between raw and smoothed angles (for texture rotation)
// This returns how much to rotate the mirror image to show smoothed angles
//-----------------------------------------------------------------------------
extern ConVar tfvr_spectator_roll_invert;

QAngle CVRSpectatorCamera::GetMirrorSmoothingDelta() const
{
    if (GetMode() == VR_SPECTATOR_OFF)
        return QAngle(0, 0, 0);
    
    // Delta = smoothed - raw
    // Positive delta means we need to rotate in that direction to show the smoothed view
    QAngle delta;
    delta.x = AngleNormalize(m_smoothedAngles.x - m_lastRawAngles.x);
    delta.y = AngleNormalize(m_smoothedAngles.y - m_lastRawAngles.y);
    delta.z = AngleNormalize(m_smoothedAngles.z - m_lastRawAngles.z);
    
    // Allow inverting roll direction if needed
    if (tfvr_spectator_roll_invert.GetBool())
    {
        delta.z = -delta.z;
    }
    
    return delta;
}

//-----------------------------------------------------------------------------
// Purpose: Get zoom factor for mirror view
//-----------------------------------------------------------------------------
float CVRSpectatorCamera::GetMirrorZoom() const
{
    if (GetMode() == VR_SPECTATOR_OFF)
        return 1.0f;
    
    return GetVRSpectatorZoom();
}

//-----------------------------------------------------------------------------
// Purpose: Draw debug visualization
//-----------------------------------------------------------------------------
void CVRSpectatorCamera::DrawDebug()
{
    if (!IsVRSpectatorDebugEnabled() || !debugoverlay)
        return;
    
    if (GetMode() == VR_SPECTATOR_OFF)
        return;
    
    // Display current smoothing state
    const char* modeStr = (GetMode() == VR_SPECTATOR_MIRROR_ONLY) ? "Mirror-Only" : "Full";
    
    debugoverlay->AddScreenTextOverlay(0.01f, 0.1f, 0.0f, 255, 255, 0, 255,
        CFmtStr("Spectator Mode: %s", modeStr));
    
    debugoverlay->AddScreenTextOverlay(0.01f, 0.12f, 0.0f, 255, 255, 255, 255,
        CFmtStr("Raw:      P:%.1f Y:%.1f R:%.1f", 
                m_lastRawAngles.x, m_lastRawAngles.y, m_lastRawAngles.z));
    
    debugoverlay->AddScreenTextOverlay(0.01f, 0.14f, 0.0f, 0, 255, 0, 255,
        CFmtStr("Smoothed: P:%.1f Y:%.1f R:%.1f", 
                m_smoothedAngles.x, m_smoothedAngles.y, m_smoothedAngles.z));
    
    QAngle delta = GetMirrorSmoothingDelta();
    debugoverlay->AddScreenTextOverlay(0.01f, 0.16f, 0.0f, 255, 128, 0, 255,
        CFmtStr("Delta:    P:%.1f Y:%.1f R:%.1f (roll invert: %s)", 
                delta.x, delta.y, delta.z,
                tfvr_spectator_roll_invert.GetBool() ? "ON" : "OFF"));
    
    debugoverlay->AddScreenTextOverlay(0.01f, 0.18f, 0.0f, 128, 128, 255, 255,
        CFmtStr("Zoom: %.2f | Roll HL: %.2fs | YP HL: %.2fs",
                GetMirrorZoom(), GetVRSpectatorRollHalfLife(), GetVRSpectatorYawPitchHalfLife()));
    
    // Show if roll compensation is active
    bool rollActive = fabsf(delta.z) > 0.5f;
    debugoverlay->AddScreenTextOverlay(0.01f, 0.20f, 0.0f, rollActive ? 0 : 128, rollActive ? 255 : 128, rollActive ? 0 : 128, 255,
        CFmtStr("Roll Compensation: %s", rollActive ? "ACTIVE" : "inactive"));
}

//-----------------------------------------------------------------------------
// Console commands for quick mode switching
//-----------------------------------------------------------------------------
CON_COMMAND(tfvr_spectator_off, "Disable spectator camera smoothing")
{
    tfvr_spectator_mode.SetValue(0);
    Msg("VR Spectator Camera: Disabled\n");
}

CON_COMMAND(tfvr_spectator_streaming, "Enable streaming mode (mirror-only smoothing)")
{
    tfvr_spectator_mode.SetValue(1);
    Msg("VR Spectator Camera: Streaming mode (mirror-only smoothing)\n");
}

CON_COMMAND(tfvr_spectator_trailer, "Enable trailer mode (full camera smoothing)")
{
    tfvr_spectator_mode.SetValue(2);
    Msg("VR Spectator Camera: Trailer mode (full camera smoothing - WARNING: adds input lag!)\n");
}

//-----------------------------------------------------------------------------
// Preset commands for common smoothing levels (like Alyx low/medium/high)
//-----------------------------------------------------------------------------
CON_COMMAND(tfvr_spectator_smooth_low, "Set low smoothing (responsive, minimal lag)")
{
    tfvr_spectator_roll_halflife.SetValue(0.2f);
    tfvr_spectator_yawpitch_halflife.SetValue(0.05f);
    tfvr_spectator_zoom.SetValue(1.05f);
    Msg("VR Spectator Camera: Low smoothing preset\n");
}

CON_COMMAND(tfvr_spectator_smooth_medium, "Set medium smoothing (balanced)")
{
    tfvr_spectator_roll_halflife.SetValue(0.5f);
    tfvr_spectator_yawpitch_halflife.SetValue(0.09f);
    tfvr_spectator_zoom.SetValue(1.1f);
    Msg("VR Spectator Camera: Medium smoothing preset (Alyx defaults)\n");
}

CON_COMMAND(tfvr_spectator_smooth_high, "Set high smoothing (cinematic, more lag)")
{
    tfvr_spectator_roll_halflife.SetValue(1.0f);
    tfvr_spectator_yawpitch_halflife.SetValue(0.2f);
    tfvr_spectator_zoom.SetValue(1.15f);
    Msg("VR Spectator Camera: High smoothing preset\n");
}

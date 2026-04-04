#include "cbase.h"
#include "vr_input.h"
#include "in_buttons.h"
#include "prediction.h"
#include "mathlib/mathlib.h"
#include "iinput.h"
#include "iclientmode.h"
#include "input.h"
#include "convar.h"
#include "vr_menu_manager.h"
#include "engine/ivdebugoverlay.h"
#include "tf/c_tf_player.h"
#include "tf/tf_weaponbase.h"
#include "tf/tf_shareddefs.h"
#include "c_tfvr_hand.h"
#include <game/client/iviewport.h>
#include "viewport_panel_names.h"
#include "ienginevgui.h"
#include "client_virtualreality.h"

// Engine interface for executing client commands
extern IVEngineClient *engine;

// ConVars for input sensitivity and deadzone
ConVar tfvr_move_sensitivity("tfvr_move_sensitivity", "1.0", FCVAR_ARCHIVE, "Sensitivity multiplier for VR movement");
ConVar tfvr_thumbstick_deadzone("tfvr_thumbstick_deadzone", "0.1", FCVAR_ARCHIVE, "Deadzone for thumbstick movement");
ConVar tfvr_use_hmd_angles("tfvr_use_hmd_angles", "0", FCVAR_ARCHIVE, "Use HMD angles for view");

// Controller tracking ConVars
ConVar tfvr_enable_controller_tracking("tfvr_enable_controller_tracking", "1", FCVAR_ARCHIVE, "Enable VR controller position and orientation tracking");
ConVar tfvr_controller_tracking_debug("tfvr_controller_tracking_debug", "0", FCVAR_ARCHIVE, "Show debug output for controller tracking");

// VR Turning ConVars
ConVar tfvr_turning_mode( "tfvr_turning_mode", "1", FCVAR_ARCHIVE, "VR turning mode: 0=disabled, 1=smooth, 2=snap" );
ConVar tfvr_smooth_turn_rate( "tfvr_smooth_turn_rate", "120", FCVAR_ARCHIVE, "Smooth turning rate in degrees per second" );
ConVar tfvr_snap_turn_angle( "tfvr_snap_turn_angle", "45", FCVAR_ARCHIVE, "Snap turning angle in degrees" );
ConVar tfvr_turn_deadzone( "tfvr_turn_deadzone", "0.3", FCVAR_ARCHIVE, "Deadzone for turning input (0.0-1.0)" );
ConVar tfvr_snap_turn_delay( "tfvr_snap_turn_delay", "0.25", FCVAR_ARCHIVE, "Delay between snap turns in seconds" );

// Weapon switching ConVars
ConVar tfvr_weapon_switch_stick_enabled( "tfvr_weapon_switch_stick_enabled", "0", FCVAR_ARCHIVE, "Enable right stick up/down weapon switching (0=disabled, 1=enabled). Disabled by default since radial weapon select menu is available." );
ConVar tfvr_weapon_switch_tilt_threshold( "tfvr_weapon_switch_tilt_threshold", "0.7", FCVAR_ARCHIVE, "Tilt threshold for weapon switching (0.0-1.0)" );
ConVar tfvr_weapon_switch_debug( "tfvr_weapon_switch_debug", "0", FCVAR_ARCHIVE, "Show debug output for weapon switching actions" );

// Voice chat gesture ConVars (walkie-talkie style activation)
ConVar tfvr_voice_gesture_enabled( "tfvr_voice_gesture_enabled", "1", FCVAR_ARCHIVE, "Enable walkie-talkie style voice chat (hold offhand near ear and press trigger)" );
ConVar tfvr_voice_ear_radius( "tfvr_voice_ear_radius", "15.0", FCVAR_ARCHIVE, "Radius around left ear for voice chat activation (in game units)" );
ConVar tfvr_voice_ear_offset( "tfvr_voice_ear_offset", "8.0", FCVAR_ARCHIVE, "Lateral offset from head center to left ear (in game units)" );
ConVar tfvr_voice_gesture_debug( "tfvr_voice_gesture_debug", "0", FCVAR_ARCHIVE, "Show debug output for voice chat gesture detection" );

// Physical throw ConVars
ConVar tfvr_physical_throw( "tfvr_physical_throw", "1", FCVAR_ARCHIVE, "Enable physical throwing for throwable weapons (0=classic aim-based throw, 1=gesture-based throw)" );
ConVar tfvr_physical_ball( "tfvr_physical_ball", "1", FCVAR_ARCHIVE, "Enable physical ball launch for Sandman/Wrap Assassin (0=trigger launches ball, 1=offhand aim + swing)" );
ConVar tfvr_physical_throw_debug( "tfvr_physical_throw_debug", "0", FCVAR_ARCHIVE, "Show debug output for physical throw gesture detection" );
ConVar tfvr_throw_grip_threshold( "tfvr_throw_grip_threshold", "0.4", FCVAR_ARCHIVE, "Grip force threshold to start throw hold (0.0-1.0, release uses grip value)" );

// Mouth proximity activation for lunchbox/buff items
ConVar tfvr_mouth_activate_enabled( "tfvr_mouth_activate_enabled", "1", FCVAR_ARCHIVE, "Require holding lunchbox/horn items near mouth to activate" );
ConVar tfvr_mouth_radius( "tfvr_mouth_radius", "14.0", FCVAR_ARCHIVE, "Radius around mouth for item activation (game units)" );
ConVar tfvr_mouth_forward_offset( "tfvr_mouth_forward_offset", "6.0", FCVAR_ARCHIVE, "Forward offset from head center to mouth (game units)" );

// Physical crouching ConVars
ConVar tfvr_physical_crouch( "tfvr_physical_crouch", "1", FCVAR_ARCHIVE, "Enable physical crouching by lowering your head in VR" );
ConVar tfvr_physical_crouch_threshold( "tfvr_physical_crouch_threshold", "0.75", FCVAR_ARCHIVE, "Fraction of standing height below which physical crouch triggers (0.0-1.0)" );
ConVar tfvr_physical_crouch_hysteresis( "tfvr_physical_crouch_hysteresis", "0.04", FCVAR_ARCHIVE, "Height ratio band above threshold to exit physical crouch (prevents flickering)" );
ConVar tfvr_physical_crouch_debug( "tfvr_physical_crouch_debug", "0", FCVAR_ARCHIVE, "Show debug output for physical crouch detection" );
ConVar tfvr_mouth_down_offset( "tfvr_mouth_down_offset", "4.0", FCVAR_ARCHIVE, "Downward offset from head center to mouth (game units)" );
ConVar tfvr_mouth_activate_debug( "tfvr_mouth_activate_debug", "0", FCVAR_ARCHIVE, "Show debug output for mouth proximity activation" );
ConVar tfvr_mouth_debug_draw( "tfvr_mouth_debug_draw", "0", FCVAR_ARCHIVE, "Draw wireframe sphere at the mouth detection zone" );

// Voice gesture state - used to suppress offhand attack when voice is active
static bool s_bVoiceGestureActive = false;

// VR ball aim state - offhand aiming for Sandman/Wrap Assassin ball launch
bool g_bVRBallAimActive = false;
Vector g_vecVRBallAimOrigin;
QAngle g_angVRBallAimAngles;

// Left thumbstick click or trackpad press: quick press = medic call, long hold = scoreboard
ConVar tfvr_medic_hold_threshold( "tfvr_medic_hold_threshold", "0.3", FCVAR_ARCHIVE, "Hold time in seconds before scoreboard shows (shorter = medic call)" );

// Primary hand ConVar (defined in vr_menu_manager.cpp)
extern ConVar tfvr_primary_hand;

// Movement speed ConVars
extern ConVar cl_forwardspeed;
extern ConVar cl_sidespeed;

// Define global instances
CVRInput g_VRInput;
IInput* g_OriginalNonVRInputPtr = nullptr;

// Global variable for snap turning (shared with in_main.cpp)
float s_flLastSnapTurnTime = 0.0f;

CVRInput::CVRInput()
{
    m_bThrowHolding = false;
    m_nLastThrowableWeaponID = -1;
    m_bInCreateMove = false;
}

CVRInput::~CVRInput()
{
}

void CVRInput::CopyVRPosesToUserCmd(CUserCmd *cmd)
{
    g_pOpenXRManager->GetHMDInChaperone(cmd->playerToHmdOrigin, cmd->playerToHmdAngles);
}

void CVRInput::CreateMove(int sequence_number, float input_sample_frametime, bool active)
{
    // Get the command for this sequence
    CUserCmd* cmd = &m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
    CVerifiedUserCmd* pVerified = &m_pVerifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
    
    // Let base input system handle everything first
    CInput::CreateMove(sequence_number, input_sample_frametime, active);
    
    // Only process VR input if active
    if (active && g_pOpenXRManager && g_pOpenXRManager->IsActive())
    {
        // Flag so throw gesture only fires during CreateMove, not ExtraMouseSample
        m_bInCreateMove = true;

        // Process VR-specific input
        ProcessVRControllerInput(cmd);
        
        // Process VR controller tracking
        ProcessVRControllerTracking(cmd);
        
        // Process VR view angles to use controller angles for aiming
        ProcessVRViewAngles(cmd);
        
        // Process VR movement
        ProcessVRMovement(cmd, input_sample_frametime);

        m_bInCreateMove = false;
        
        // VR turning is now handled in the main input system
    }

    // CopyVRPosesToUserCmd(cmd);

    // Let the game mode process the command
    //if (g_pClientMode)
    //{
    //    g_pClientMode->CreateMove(input_sample_frametime, cmd);
    //}



    // Store the command for verification
    pVerified->m_cmd = *cmd;
    pVerified->m_crc = cmd->GetChecksum();
}

bool g_bExtraMouseSample = false;

void CVRInput::ExtraMouseSample(float frametime, bool active)
{
    CUserCmd dummy;
    CUserCmd *cmd = &dummy;

    CInput::ExtraMouseSample(frametime, active);

   cmd->Reset();

   QAngle viewangles;
   engine->GetViewAngles(viewangles);
   QAngle originalViewangles = viewangles;

   if (active)
   {
       // Process VR-specific input
       ProcessVRControllerInput(cmd);
        
       // Process VR controller tracking
       ProcessVRControllerTracking(cmd);
        
       // Only process VR view angles if explicitly enabled
       if (tfvr_use_hmd_angles.GetBool())
       {
           // ProcessVRViewAngles(cmd);
       }
       
       // Process VR movement
       ProcessVRMovement(cmd, frametime);
       
       // VR turning is now handled in the main input system
   }

   // Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
   engine->GetViewAngles(viewangles);

   cmd->buttons = GetButtonBits(0);

   VectorCopy(m_angPreviousViewAngles, cmd->viewangles);

   //CopyVRPosesToUserCmd(cmd);

   g_bExtraMouseSample = true;

   // Let the move manager override anything it wants to.
   if (g_pClientMode->CreateMove(frametime, cmd))
   {
       // Get current view angles after the client mode tweaks with it
       //engine->SetViewAngles( cmd->viewangles );
       prediction->SetLocalViewAngles(cmd->viewangles);
   }

   g_bExtraMouseSample = false;
}

void CVRInput::ProcessVRControllerInput(CUserCmd* cmd)
{
    // Store original button state
    int oldButtons = cmd->buttons;
    
    // Get current menu button state first
    bool bMenu = g_pOpenXRManager->IsButtonPressed("menu");
    
    // Track button press events
    static bool bLastMenuButtonState = false;
    bool bMenuButtonPressed = bMenu && !bLastMenuButtonState;
    
    // Get left thumbstick/trackpad state (medic call on quick press, scoreboard on hold)
    static bool bLastScoreboardButtonState = false;
    static float flButtonPressTime = 0.0f;
    static bool bScoreboardShowing = false;
    
    bool bScoreboard = g_pOpenXRManager->IsButtonPressed("scoreboard");
    bool bScoreboardButtonPressed = bScoreboard && !bLastScoreboardButtonState;
    bool bScoreboardButtonReleased = !bScoreboard && bLastScoreboardButtonState;
    
    float flHoldThreshold = tfvr_medic_hold_threshold.GetFloat();
    
    // Handle left thumbstick/trackpad: quick press = medic, long hold = scoreboard
    if (bScoreboardButtonPressed)
    {
        // Record when button was pressed
        flButtonPressTime = gpGlobals->curtime;
        bScoreboardShowing = false;
    }
    else if (bScoreboard && !bScoreboardShowing)
    {
        // Button is being held - check if we've passed the threshold to show scoreboard
        float flHoldTime = gpGlobals->curtime - flButtonPressTime;
        if (flHoldTime >= flHoldThreshold)
        {
            // Show scoreboard after hold threshold
            if (gViewPortInterface)
            {
                gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, true);
            }
            bScoreboardShowing = true;
        }
    }
    else if (bScoreboardButtonReleased)
    {
        float flHoldTime = gpGlobals->curtime - flButtonPressTime;
        
        if (bScoreboardShowing)
        {
            // Hide scoreboard when button is released after long hold
            if (gViewPortInterface)
            {
                gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, false);
            }
        }
        else if (flHoldTime < flHoldThreshold)
        {
            // Quick press - call for medic
            engine->ClientCmd_Unrestricted("voicemenu 0 0");
        }
        
        bScoreboardShowing = false;
    }
    bLastScoreboardButtonState = bScoreboard;
    
    // Check if menu is visible
    bool bMenuVisible = g_pVRMenuManager && g_pVRMenuManager->IsMenuVisible();
    
    // Track menu state changes
    static bool lastMenuState = false;
    if (bMenuVisible != lastMenuState)
    {
        lastMenuState = bMenuVisible;
    }
    
    // Handle menu button press to toggle menu state
    // Note: We check if the Game UI is ACTUALLY visible, not just IsMenuVisible().
    // IsMenuVisible() returns true when dead to block VR input, but the game UI
    // might not actually be open. This allows opening the escape menu while dead.
    if (bMenuButtonPressed)
    {
        bool bGameUIActuallyVisible = enginevgui && enginevgui->IsGameUIVisible();
        if (bGameUIActuallyVisible)
        {
            engine->ClientCmd_Unrestricted("gameui_hide\n");
        }
        else
        {
            engine->ClientCmd_Unrestricted("gameui_activate\n");
        }
    }
    
    // =========================================================================
    // Voice chat activation
    // MUST be processed BEFORE the menu-visible early return so voice works
    // while dead, and BEFORE attack inputs so we can suppress the offhand trigger.
    // Uses the "voice" action which is bound to the left trigger by default.
    // 
    // When gesture mode is ENABLED (tfvr_voice_gesture_enabled 1):
    //   - Press trigger while controller is near left ear to activate voice
    //   - Voice stays active until trigger is released (even if moving out of range)
    //   - Moving into range while already holding trigger does NOT activate
    //
    // When gesture mode is DISABLED (tfvr_voice_gesture_enabled 0):
    //   - Simply pressing the voice action activates voice directly
    //   - No proximity check required
    //
    // In both modes, when voice is active, secondary fire is suppressed.
    // =========================================================================
    if (g_pOpenXRManager)
    {
        // Persistent state for voice action edge detection
        static bool bLastVoiceActionPressed = false;
        
        // Get the voice action value (bound to left trigger by default)
        float voiceActionValue = g_pOpenXRManager->GetAnalogValue("voice");
        bool bVoiceActionPressed = (voiceActionValue > 0.5f);
        bool bVoiceActionJustPressed = bVoiceActionPressed && !bLastVoiceActionPressed;
        bool bVoiceActionJustReleased = !bVoiceActionPressed && bLastVoiceActionPressed;
        
        if (tfvr_voice_gesture_enabled.GetBool())
        {
            // GESTURE MODE: Require proximity to left ear
            
            // Determine which hand is the offhand (opposite of primary hand)
            // tfvr_primary_hand: 0 = left primary (right is offhand), 1 = right primary (left is offhand)
            bool bLeftIsOffhand = (tfvr_primary_hand.GetInt() == 1);
            
            // Get HMD pose in playspace/chaperone coordinates
            Vector hmdOrigin;
            QAngle hmdAngles;
            g_pOpenXRManager->GetHMDInChaperone(hmdOrigin, hmdAngles);
            
            // Calculate left ear position (offset to the left of the head center)
            Vector hmdForward, hmdRight, hmdUp;
            AngleVectors(hmdAngles, &hmdForward, &hmdRight, &hmdUp);
            
            float earOffset = tfvr_voice_ear_offset.GetFloat();
            Vector leftEarPos = hmdOrigin - hmdRight * earOffset; // Negative right = left
            
            // Get offhand controller position in RAW playspace coordinates (same as HMD)
            Vector offhandPos;
            bool bOffhandValid = false;
            VMatrix offhandPose;
            
            if (bLeftIsOffhand)
            {
                if (g_pOpenXRManager->GetLeftControllerPoseRaw(offhandPose))
                {
                    offhandPos = offhandPose.GetTranslation();
                    bOffhandValid = true;
                }
            }
            else
            {
                if (g_pOpenXRManager->GetRightControllerPoseRaw(offhandPose))
                {
                    offhandPos = offhandPose.GetTranslation();
                    bOffhandValid = true;
                }
            }
            
            // Check proximity to left ear
            float earRadius = tfvr_voice_ear_radius.GetFloat();
            bool bNearEar = false;
            float distanceToEar = 0.0f;
            
            if (bOffhandValid)
            {
                distanceToEar = (offhandPos - leftEarPos).Length();
                bNearEar = (distanceToEar <= earRadius);
            }
            
            // Gesture activation logic:
            // - Activate ONLY when voice action is pressed (rising edge) while inside ear radius
            // - Stay active until voice action is released, regardless of position
            // - Moving into range while already holding doesn't activate
            
            if (bVoiceActionJustPressed && bNearEar && !s_bVoiceGestureActive)
            {
                s_bVoiceGestureActive = true;
                engine->ClientCmd_Unrestricted("+voicerecord\n");
                if (tfvr_voice_gesture_debug.GetBool())
                {
                    DevMsg("VR Voice: Activated (gesture mode, distance: %.1f)\n", distanceToEar);
                }
            }
            else if (bVoiceActionJustReleased && s_bVoiceGestureActive)
            {
                s_bVoiceGestureActive = false;
                engine->ClientCmd_Unrestricted("-voicerecord\n");
                if (tfvr_voice_gesture_debug.GetBool())
                {
                    DevMsg("VR Voice: Deactivated (gesture mode)\n");
                }
            }
            
            // Debug output
            if (tfvr_voice_gesture_debug.GetBool())
            {
                static float lastVoiceDebugTime = 0.0f;
                float currentTime = gpGlobals->curtime;
                
                if (currentTime - lastVoiceDebugTime > 0.5f)
                {
                    DevMsg("VR Voice: Mode=Gesture, Offhand=%s, Valid=%d, Distance=%.1f (radius=%.1f), Action=%.2f, NearEar=%d, Active=%d\n",
                           bLeftIsOffhand ? "Left" : "Right",
                           bOffhandValid ? 1 : 0,
                           distanceToEar,
                           earRadius,
                           voiceActionValue,
                           bNearEar ? 1 : 0,
                           s_bVoiceGestureActive ? 1 : 0);
                    lastVoiceDebugTime = currentTime;
                }
            }
        }
        else
        {
            // DIRECT MODE: Voice action directly activates voice (no proximity check)
            
            if (bVoiceActionJustPressed && !s_bVoiceGestureActive)
            {
                s_bVoiceGestureActive = true;
                engine->ClientCmd_Unrestricted("+voicerecord\n");
                if (tfvr_voice_gesture_debug.GetBool())
                {
                    DevMsg("VR Voice: Activated (direct mode)\n");
                }
            }
            else if (bVoiceActionJustReleased && s_bVoiceGestureActive)
            {
                s_bVoiceGestureActive = false;
                engine->ClientCmd_Unrestricted("-voicerecord\n");
                if (tfvr_voice_gesture_debug.GetBool())
                {
                    DevMsg("VR Voice: Deactivated (direct mode)\n");
                }
            }
            
            // Debug output
            if (tfvr_voice_gesture_debug.GetBool())
            {
                static float lastVoiceDebugTime = 0.0f;
                float currentTime = gpGlobals->curtime;
                
                if (currentTime - lastVoiceDebugTime > 0.5f)
                {
                    DevMsg("VR Voice: Mode=Direct, Action=%.2f, Active=%d\n",
                           voiceActionValue,
                           s_bVoiceGestureActive ? 1 : 0);
                    lastVoiceDebugTime = currentTime;
                }
            }
        }
        
        // Update action state for next frame
        bLastVoiceActionPressed = bVoiceActionPressed;
    }
    
    // If menu is visible, block all gameplay actions except menu controls
    // (voice chat above is intentionally allowed through so it works while dead)
    if (bMenuVisible)
    {
        bLastMenuButtonState = bMenu;
        return;
    }
    
    // Normal gameplay input processing (only when menu is not visible)
    
    // Determine which hand is offhand for voice gesture suppression
    // tfvr_primary_hand: 0 = left primary (right is offhand), 1 = right primary (left is offhand)
    bool bLeftIsOffhand = (tfvr_primary_hand.GetInt() == 1);
    
    // Primary attack - suppress if voice gesture is active AND right hand is offhand
    bool bPrimaryAttack = g_pOpenXRManager->GetAnalogValue("primary_attack") > 0.5f;
    bool bSuppressPrimary = s_bVoiceGestureActive && !bLeftIsOffhand; // Right is offhand, uses primary_attack
    if (bPrimaryAttack && !bSuppressPrimary)
        cmd->buttons |= IN_ATTACK;

    // Secondary attack - suppress if voice gesture is active AND left hand is offhand
    bool bSecondaryAttack = g_pOpenXRManager->GetAnalogValue("secondary_attack") > 0.5f;
    bool bSuppressSecondary = s_bVoiceGestureActive && bLeftIsOffhand; // Left is offhand, uses secondary_attack

    // VR ball aim: intercept secondary attack for ball-launching bats (Sandman, Wrap Assassin).
    // Holding the offhand trigger enters aiming mode instead of immediately firing the ball.
    // Note: cmd->leftControllerOrigin is not yet populated at this point (tracking runs later),
    // so we fetch the left controller pose directly from OpenXR for the crosshair globals.
    bool bBallAimConsumed = false;
    if (tfvr_physical_ball.GetBool() && bSecondaryAttack && !bSuppressSecondary)
    {
        C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
        if (pLocalPlayer)
        {
            CTFWeaponBase *pActiveWeapon = pLocalPlayer->GetActiveTFWeapon();
            if (pActiveWeapon)
            {
                int iWpnID = pActiveWeapon->GetWeaponID();
                if (iWpnID == TF_WEAPON_BAT_WOOD || iWpnID == TF_WEAPON_BAT_GIFTWRAP)
                {
                    // Always consume IN_ATTACK2 for ball bats so the normal
                    // SecondaryAttack path never fires.  Without this, holding
                    // the trigger while ammo is empty sends IN_ATTACK2 every
                    // frame and the ball auto-fires the instant ammo recharges.
                    bBallAimConsumed = true;

                    int iBallAmmo = pLocalPlayer->GetAmmoCount(TF_AMMO_GRENADES1);
                    if (iBallAmmo > 0)
                    {
                        cmd->vrBallAimActive = true;
                        g_bVRBallAimActive = true;

                        VMatrix leftPose;
                        if (g_pOpenXRManager->GetLeftControllerPose(leftPose))
                        {
                            g_vecVRBallAimOrigin = leftPose.GetTranslation();
                            MatrixAngles(leftPose.As3x4(), g_angVRBallAimAngles);
                        }
                    }
                }
            }
        }
    }

    if (!bBallAimConsumed)
    {
        g_bVRBallAimActive = false;
        if (bSecondaryAttack && !bSuppressSecondary)
            cmd->buttons |= IN_ATTACK2;
    }

    // Mouth proximity gate for lunchbox items and soldier horns:
    // Suppress primary attack unless the weapon is held near the player's mouth.
    if ( tfvr_mouth_activate_enabled.GetBool() && m_bInCreateMove )
    {
        C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
        if ( pPlayer )
        {
            CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
            if ( IsMouthActivatedWeapon( pWeapon ) )
            {
                bool bNearMouth = IsWeaponNearMouth( pWeapon );

                if ( !bNearMouth )
                {
                    cmd->buttons &= ~IN_ATTACK;
                }
            }
        }
    }

    // Physical throw gesture — intercepts IN_ATTACK for throwable weapons.
    // Only process during CreateMove, not ExtraMouseSample (which uses a
    // disposable dummy command and would consume the release event).
    if ( tfvr_physical_throw.GetBool() && m_bInCreateMove )
    {
        ProcessThrowGesture( cmd, bPrimaryAttack, bSuppressPrimary );
    }

    // Use
    bool bUse = g_pOpenXRManager->IsButtonPressed("use");
    if (bUse)
        cmd->buttons |= IN_USE;

    // Duck (button + physical crouch)
    bool bDuck = g_pOpenXRManager->IsButtonPressed("duck");
    if (bDuck)
        cmd->buttons |= IN_DUCK;

    // Physical crouch detection: compare HMD height to calibrated standing height
    // Disabled in seated mode — HMD height is unreliable when sitting.
    static ConVar *pSeatedMode = cvar->FindVar( "tfvr_seated_mode" );
    bool bSeated = pSeatedMode && pSeatedMode->GetBool();
    static bool s_bPhysicallyCrouching = false;
    if ( tfvr_physical_crouch.GetBool() && g_pOpenXRManager->IsActive() && !bSeated )
    {

        Vector rawHmdPos = g_pOpenXRManager->GetRawHMDPosition();
        float currentHmdHeight = rawHmdPos.y; // meters above floor (OpenXR Y-up)

        static ConVar *pPlayerHeight = cvar->FindVar( "tfvr_player_height" );
        float playerHeightInches = pPlayerHeight ? pPlayerHeight->GetFloat() : 67.0f;

        // Standing HMD height ≈ player height minus head-top-to-eye offset (~4 inches)
        float standingHmdMeters = ( playerHeightInches - 4.0f ) / 39.3701f;

        if ( standingHmdMeters > 0.1f )
        {
            float heightRatio = currentHmdHeight / standingHmdMeters;

            float threshold = tfvr_physical_crouch_threshold.GetFloat();
            float hysteresis = tfvr_physical_crouch_hysteresis.GetFloat();

            if ( s_bPhysicallyCrouching )
            {
                if ( heightRatio > threshold + hysteresis )
                    s_bPhysicallyCrouching = false;
            }
            else
            {
                if ( heightRatio < threshold )
                    s_bPhysicallyCrouching = true;
            }

            if ( tfvr_physical_crouch_debug.GetBool() )
            {
                static float s_flLastDebugTime = 0.0f;
                if ( gpGlobals->curtime - s_flLastDebugTime > 0.5f )
                {
                    DevMsg( "PhysCrouch: ratio=%.3f thresh=%.3f hyst=%.3f crouching=%d hmd=%.3fm standing=%.3fm\n",
                            heightRatio, threshold, hysteresis, s_bPhysicallyCrouching ? 1 : 0,
                            currentHmdHeight, standingHmdMeters );
                    s_flLastDebugTime = gpGlobals->curtime;
                }
            }
        }

        if ( s_bPhysicallyCrouching )
        {
            cmd->buttons |= IN_DUCK;
            cmd->vrPhysicalCrouch = true;
        }

        C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
        if ( pPlayer )
        {
            pPlayer->m_bPhysicalCrouch = s_bPhysicallyCrouching;
            if ( s_bPhysicallyCrouching )
                pPlayer->m_bDuckWasPhysical = true;
        }
    }
    else if ( s_bPhysicallyCrouching )
    {
        s_bPhysicallyCrouching = false;
        C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
        if ( pPlayer )
            pPlayer->m_bPhysicalCrouch = false;
    }

    // Button duck while standing clears the physical-duck flag so the
    // artificial offset applies normally for button-initiated crouches.
    if ( bDuck && !s_bPhysicallyCrouching )
    {
        C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
        if ( pPlayer )
            pPlayer->m_bDuckWasPhysical = false;
    }

    // Jump
    bool bJump = g_pOpenXRManager->IsButtonPressed("jump");
    if (bJump)
        cmd->buttons |= IN_JUMP;

    // Weapon switching
    float weaponSwitchValue = g_pOpenXRManager->GetAnalogValue("weapon_switch");
    
    // Validate that we got valid values
    if (weaponSwitchValue == 0.0f)
    {
        // This could indicate the actions aren't properly bound or working
        static float lastWarningTime = 0.0f;
        float currentTime = gpGlobals->curtime;
        
        if (currentTime - lastWarningTime > 5.0f) // Only warn every 5 seconds
        {
            if (tfvr_weapon_switch_debug.GetBool())
            {
                DevMsg("VR: Warning - Weapon switching action returning 0.0 values. Check OpenXR bindings.\n");
            }
            lastWarningTime = currentTime;
        }
    }
    
    // Track weapon switching button press events to avoid continuous execution
    static bool bLastNextWeaponState = false;
    static bool bLastPrevWeaponState = false;
    
    // Threshold for detecting tilt (0.7 = 70% tilt)
    const float TILT_THRESHOLD = tfvr_weapon_switch_tilt_threshold.GetFloat();
    
    // Detect forward tilt (positive Y) for next weapon
    bool bNextWeaponActive = weaponSwitchValue > TILT_THRESHOLD;
    // Detect backward tilt (negative Y) for previous weapon
    bool bPrevWeaponActive = weaponSwitchValue < -TILT_THRESHOLD;
    
    bool bNextWeaponPressed = bNextWeaponActive && !bLastNextWeaponState;
    bool bPrevWeaponPressed = bPrevWeaponActive && !bLastPrevWeaponState;
    
    // Check if stick weapon switching is enabled (disabled by default, use radial menu instead)
    if (tfvr_weapon_switch_stick_enabled.GetBool())
    {
        // Check if weapon switching is allowed before executing commands
        C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
        bool bCanSwitchWeapons = pLocalPlayer && pLocalPlayer->IsAllowedToSwitchWeapons();
        
        if (bNextWeaponPressed && bCanSwitchWeapons)
        {
            engine->ExecuteClientCmd("invnext");
            if (tfvr_weapon_switch_debug.GetBool())
            {
                DevMsg("VR: Next weapon triggered (tilt: %.2f, threshold: %.2f)\n", weaponSwitchValue, TILT_THRESHOLD);
            }
        }
        else if (bNextWeaponPressed && !bCanSwitchWeapons)
        {
            if (tfvr_weapon_switch_debug.GetBool())
            {
                DevMsg("VR: Next weapon blocked - weapon switching not allowed\n");
            }
        }
        
        if (bPrevWeaponPressed && bCanSwitchWeapons)
        {
            engine->ExecuteClientCmd("invprev");
            if (tfvr_weapon_switch_debug.GetBool())
            {
                DevMsg("VR: Previous weapon triggered (tilt: %.2f, threshold: %.2f)\n", weaponSwitchValue, TILT_THRESHOLD);
            }
        }
        else if (bPrevWeaponPressed && !bCanSwitchWeapons)
        {
            if (tfvr_weapon_switch_debug.GetBool())
            {
                DevMsg("VR: Previous weapon blocked - weapon switching not allowed\n");
            }
        }
    }
    
    // Update weapon switching button state tracking
    bLastNextWeaponState = bNextWeaponActive;
    bLastPrevWeaponState = bPrevWeaponActive;
    
    // Debug output for weapon switching values
    if (tfvr_weapon_switch_debug.GetBool())
    {
        static float lastDebugTime = 0.0f;
        float currentTime = gpGlobals->curtime;
        
        // Only output debug info every 0.5 seconds to avoid spam
        if (currentTime - lastDebugTime > 0.5f)
        {
            const char* direction = "neutral";
            if (weaponSwitchValue > TILT_THRESHOLD)
                direction = "forward (next)";
            else if (weaponSwitchValue < -TILT_THRESHOLD)
                direction = "backward (prev)";
            
            DevMsg("VR: Weapon switch - Y-axis: %.2f, Direction: %s, Threshold: %.2f\n", 
                   weaponSwitchValue, direction, TILT_THRESHOLD);
            lastDebugTime = currentTime;
        }
    }

    // Update button state tracking
    bLastMenuButtonState = bMenu;
}

void CVRInput::ProcessVRViewAngles(CUserCmd* cmd)
{
    // Check if controller tracking is enabled
    if (!tfvr_enable_controller_tracking.GetBool())
        return;

    // Check if menu is visible - if so, disable view angle changes
    bool bMenuVisible = g_pVRMenuManager && g_pVRMenuManager->IsMenuVisible();
    if (bMenuVisible)
    {
        // Don't process view angle changes when menu is open
        return;
    }

    // Get right controller pose for aiming (typically the shooting hand)
    VMatrix rightControllerPose;
    if (g_pOpenXRManager->GetRightControllerPose(rightControllerPose))
    {
        // Extract angles from the controller pose matrix
        QAngle controllerAngles;
        MatrixAngles(rightControllerPose.As3x4(), controllerAngles);
        
        // We DON'T change cmd->viewangles here because:
        // - cmd->viewangles affects locomotion (should use headset)
        // - Weapon_ShootAngles() handles shooting direction (uses controller)
        // This keeps locomotion and shooting completely separate

        // Debug output for controller angles
        if (tfvr_controller_tracking_debug.GetBool())
        {
            DevMsg("VR Aim: Using controller angles - pitch=%.1f yaw=%.1f roll=%.1f\n", 
                   controllerAngles.x, controllerAngles.y, controllerAngles.z);
        }
    }
}

void CVRInput::ProcessVRMovement(CUserCmd* cmd, float frametime)
{
    // Check if menu is visible - if so, disable movement
    bool bMenuVisible = g_pVRMenuManager && g_pVRMenuManager->IsMenuVisible();
    if (bMenuVisible)
    {
        // Don't process any movement when menu is open
        return;
    }

    // Block VR thumbstick movement in states that restrict mobility.
    // Without this, the VR thumbstick bypasses the movement zeroing done in
    // C_TFPlayer::CreateMove because ProcessVRMovement runs afterward.
    C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
    if ( pLocalPlayer )
    {
        bool bTaunting = pLocalPlayer->m_Shared.InCond( TF_COND_TAUNTING );
        bool bThriller = pLocalPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER );
        bool bFreezeInput = pLocalPlayer->m_Shared.InCond( TF_COND_FREEZE_INPUT );

        if ( bFreezeInput )
            return;

        if ( ( bTaunting || bThriller ) && !pLocalPlayer->CanMoveDuringTaunt() )
            return;

        if ( pLocalPlayer->m_Shared.IsControlStunned() || pLocalPlayer->m_Shared.IsLoserStateStunned() )
            return;
    }
    
    // Get movement values
    float moveX = g_pOpenXRManager->GetAnalogValue("move_x");
    float moveY = g_pOpenXRManager->GetAnalogValue("move_y");

    // DevMsg("VR Movement: Raw values - x=%.2f y=%.2f\n", moveX, moveY);

    // Apply deadzone
    float deadzone = tfvr_thumbstick_deadzone.GetFloat();
    if (fabs(moveX) < deadzone) moveX = 0;
    if (fabs(moveY) < deadzone) moveY = 0;

    //DevMsg("VR Movement: After deadzone - x=%.2f y=%.2f\n", moveX, moveY);

    // Apply sensitivity
    float sensitivity = tfvr_move_sensitivity.GetFloat();
    moveX *= sensitivity;
    moveY *= sensitivity;

    //DevMsg("VR Movement: After sensitivity - x=%.2f y=%.2f\n", moveX, moveY);

    // Get current movement values before modification
    float oldForward = cmd->forwardmove;
    float oldSide = cmd->sidemove;

    // Map to movement values - use cl_forwardspeed and cl_sidespeed for proper scaling
    float forwardSpeed = cl_forwardspeed.GetFloat();
    float sideSpeed = cl_sidespeed.GetFloat();
    
    // DevMsg("VR Movement: Speed values - forward=%.2f side=%.2f\n", forwardSpeed, sideSpeed);

    // Add VR movement to existing values
    cmd->forwardmove += moveY * forwardSpeed;
    cmd->sidemove += moveX * sideSpeed;

    // Clamp movement values to prevent excessive speed
    float maxSpeed = forwardSpeed;
    cmd->forwardmove = clamp(cmd->forwardmove, -maxSpeed, maxSpeed);
    cmd->sidemove = clamp(cmd->sidemove, -maxSpeed, maxSpeed);

    // Debug output for movement values
    //DevMsg("VR Movement: Final values - forward=%.2f->%.2f side=%.2f->%.2f\n", 
    //       oldForward, cmd->forwardmove, oldSide, cmd->sidemove);
}

void CVRInput::ProcessVRControllerTracking(CUserCmd* cmd)
{
    // Check if controller tracking is enabled
    if (!tfvr_enable_controller_tracking.GetBool())
        return;
    
    // Check if menu is visible - if so, disable tracking
    bool bMenuVisible = g_pVRMenuManager && g_pVRMenuManager->IsMenuVisible();
    if (bMenuVisible)
    {
        // Don't process controller tracking when menu is open
        return;
    }
    
    // Get controller poses
    VMatrix leftControllerPose, rightControllerPose;
    bool leftValid = g_pOpenXRManager->GetLeftControllerPose(leftControllerPose);
    bool rightValid = g_pOpenXRManager->GetRightControllerPose(rightControllerPose);
    
    
    
    // Store controller poses in the command for use by other systems
    if (leftValid)
    {
        // Extract position and orientation from the pose matrix
        Vector leftPos = leftControllerPose.GetTranslation();
        QAngle leftAngles;
        MatrixAngles(leftControllerPose.As3x4(), leftAngles);

        // Store hand bone position for third-person arm IK networking.
        // Must call SetupBones with a real buffer because C_TFVRHand::SetupBones
        // skips the anchor delta when pBoneToWorldOut is NULL (GetBonePosition path).
        C_TFVRHand* pLeftHand = GetLocalPlayerLeftHand();
        if (pLeftHand)
        {
            int iHandBone = pLeftHand->GetHandBoneIndex();
            matrix3x4_t handBones[MAXSTUDIOBONES];
            if (iHandBone >= 0 && iHandBone < MAXSTUDIOBONES &&
                pLeftHand->SetupBones(handBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime))
            {
                MatrixAngles(handBones[iHandBone], cmd->vrIKHandAngL, cmd->vrIKHandPosL);
            }
            else
            {
                cmd->vrIKHandPosL = leftPos;
                cmd->vrIKHandAngL = leftAngles;
            }
        }
        else
        {
            cmd->vrIKHandPosL = leftPos;
            cmd->vrIKHandAngL = leftAngles;
        }

        // Check if left hand is holding a weapon (e.g., medigun) - if so, send muzzle position/angles
        if (pLeftHand && pLeftHand->GetHeldWeapon())
        {
            Vector muzzlePos;
            QAngle muzzleAngles;
            if (pLeftHand->GetWeaponMuzzlePositionAndAngles(muzzlePos, muzzleAngles))
            {
                // Send weapon muzzle position AND angles for server-side hit detection
                cmd->leftControllerOrigin = muzzlePos;
                cmd->leftControllerAngles = muzzleAngles;
                
                if (tfvr_controller_tracking_debug.GetBool())
                {
                    DevMsg("Left Hand: Weapon muzzle pos(%.2f, %.2f, %.2f), angles(%.1f, %.1f, %.1f)\n", 
                           muzzlePos.x, muzzlePos.y, muzzlePos.z,
                           muzzleAngles.x, muzzleAngles.y, muzzleAngles.z);
                }
            }
            else
            {
                // Fallback to controller position if muzzle lookup fails
                cmd->leftControllerOrigin = leftPos;
                cmd->leftControllerAngles = leftAngles;
            }
        }
        else
        {
            // No weapon held, use controller position/angles
            cmd->leftControllerOrigin = leftPos;
            cmd->leftControllerAngles = leftAngles;
            
            // Debug output
            if (tfvr_controller_tracking_debug.GetBool())
            {
                DevMsg("Left Controller: pos(%.2f, %.2f, %.2f) angles(%.1f, %.1f, %.1f)\n", 
                       leftPos.x, leftPos.y, leftPos.z, leftAngles.x, leftAngles.y, leftAngles.z);
            }
        }
    }
    
    if (rightValid)
    {
        // Extract position and orientation from the pose matrix
        Vector rightPos = rightControllerPose.GetTranslation();
        QAngle rightAngles;
        MatrixAngles(rightControllerPose.As3x4(), rightAngles);

        // Store hand bone position for third-person arm IK networking.
        // Same SetupBones buffer approach as the left hand (see comment above).
        C_TFVRHand* pRightHand = GetLocalPlayerRightHand();
        if (pRightHand)
        {
            int iHandBone = pRightHand->GetHandBoneIndex();
            matrix3x4_t handBones[MAXSTUDIOBONES];
            if (iHandBone >= 0 && iHandBone < MAXSTUDIOBONES &&
                pRightHand->SetupBones(handBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime))
            {
                MatrixAngles(handBones[iHandBone], cmd->vrIKHandAngR, cmd->vrIKHandPosR);
            }
            else
            {
                cmd->vrIKHandPosR = rightPos;
                cmd->vrIKHandAngR = rightAngles;
            }
        }
        else
        {
            cmd->vrIKHandPosR = rightPos;
            cmd->vrIKHandAngR = rightAngles;
        }

        // Check if right hand is holding a weapon - if so, send muzzle position/angles
        if (pRightHand && pRightHand->GetHeldWeapon())
        {
            // Weapons that use raw controller pose instead of muzzle/weapon_bone aiming
            C_TFWeaponBase *pHeldWeapon = pRightHand->GetHeldWeapon();
            bool bIsFists = (pHeldWeapon && pHeldWeapon->GetWeaponID() == TF_WEAPON_FISTS);
            bool bIsGunslinger = (pHeldWeapon && V_stristr(pHeldWeapon->GetClassname(), "robot_arm"));
            bool bUseRawController = bIsFists || bIsGunslinger;
            if (pHeldWeapon)
            {
                const char *cls = pHeldWeapon->GetClassname();
                if (cls && (V_stristr(cls, "sapper") || V_stristr(cls, "builder")))
                    bUseRawController = true;
            }

            // If we have a cached idle weapon_bone offset, always use it.
            // This keeps the hitbox stable regardless of visual animations
            // (backstab raise, fire anims, etc.).
            Vector idleWpnPos;
            QAngle idleWpnAng;

            Vector muzzlePos;
            QAngle muzzleAngles;
            if (pRightHand->GetIdleWeaponBoneTransform( idleWpnPos, idleWpnAng ))
            {
                cmd->rightControllerOrigin = idleWpnPos;
                cmd->rightControllerAngles = idleWpnAng;
            }
            else if (!bUseRawController && pRightHand->GetWeaponMuzzlePositionAndAngles(muzzlePos, muzzleAngles))
            {
                cmd->rightControllerOrigin = muzzlePos;
                cmd->rightControllerAngles = muzzleAngles;
                
                if (tfvr_controller_tracking_debug.GetBool())
                {
                    DevMsg("Right Hand: Weapon muzzle pos(%.2f, %.2f, %.2f), angles(%.1f, %.1f, %.1f)\n", 
                           muzzlePos.x, muzzlePos.y, muzzlePos.z,
                           muzzleAngles.x, muzzleAngles.y, muzzleAngles.z);
                }
            }
            else
            {
                cmd->rightControllerOrigin = rightPos;
                cmd->rightControllerAngles = rightAngles;
            }

            // Compute melee grip speed in VR playspace for any melee weapon,
            // including those without weapon models (fists).
            // Only during real CreateMove (command_number > 0) to avoid
            // ExtraMouseSample clobbering the statics.
            int wtype = pHeldWeapon->GetTFWpnData().m_iWeaponType;
            if ( cmd->command_number != 0 &&
                 ( wtype == TF_WPN_TYPE_MELEE || wtype == TF_WPN_TYPE_MELEE_ALLCLASS || bIsGunslinger ) )
            {
                float flRightSpeed = 0.0f;
                float flLeftSpeed  = 0.0f;

                VMatrix matRawPose;
                if ( g_pOpenXRManager->GetRightControllerPoseRaw( matRawPose ) )
                {
                    static Vector s_vecPrevTrackingPos = vec3_origin;
                    static float  s_flPrevTrackingTime = 0.0f;

                    Vector vecTrackingPos = matRawPose.GetTranslation();
                    float  flNow = Plat_FloatTime();
                    float  flDt  = flNow - s_flPrevTrackingTime;

                    if ( s_flPrevTrackingTime > 0.0f && flDt > 0.001f && flDt < 0.25f )
                    {
                        flRightSpeed = ( vecTrackingPos - s_vecPrevTrackingPos ).Length() / flDt;
                    }

                    s_vecPrevTrackingPos = vecTrackingPos;
                    s_flPrevTrackingTime = flNow;
                }

                if ( bIsFists )
                {
                    VMatrix matLeftPose;
                    if ( g_pOpenXRManager->GetLeftControllerPoseRaw( matLeftPose ) )
                    {
                        static Vector s_vecPrevLeftTrackingPos = vec3_origin;
                        static float  s_flPrevLeftTrackingTime = 0.0f;

                        Vector vecLeftPos = matLeftPose.GetTranslation();
                        float  flNow = Plat_FloatTime();
                        float  flDt  = flNow - s_flPrevLeftTrackingTime;

                        if ( s_flPrevLeftTrackingTime > 0.0f && flDt > 0.001f && flDt < 0.25f )
                        {
                            flLeftSpeed = ( vecLeftPos - s_vecPrevLeftTrackingPos ).Length() / flDt;
                        }

                        s_vecPrevLeftTrackingPos = vecLeftPos;
                        s_flPrevLeftTrackingTime = flNow;
                    }
                }

                cmd->vrMeleeGripSpeed = flRightSpeed;
                cmd->vrMeleeGripSpeedLeft = flLeftSpeed;
            }

            return; // Early return, we've set controller values
        }
        
        // Fallback: No weapon held, use controller position/angles
        cmd->rightControllerOrigin = rightPos;
        cmd->rightControllerAngles = rightAngles;
        
        if (tfvr_controller_tracking_debug.GetBool())
        {
            DevMsg("Right Controller: pos(%.2f, %.2f, %.1f) angles(%.1f, %.1f, %.1f)\n", 
                   rightPos.x, rightPos.y, rightPos.z, rightAngles.x, rightAngles.y, rightAngles.z);
        }
    }
    
    // Laser pointer functionality is now handled by CVRLaserPointer class
}

//-----------------------------------------------------------------------------
bool CVRInput::IsThrowableWeapon( C_TFWeaponBase *pWeapon )
{
    if ( !pWeapon )
        return false;

    int id = pWeapon->GetWeaponID();
    return ( id == TF_WEAPON_JAR ||
             id == TF_WEAPON_JAR_MILK ||
             id == TF_WEAPON_CLEAVER ||
             id == TF_WEAPON_JAR_GAS ||
             id == TF_WEAPON_THROWABLE );
}

//-----------------------------------------------------------------------------
// Handles grip/trigger hold-and-release for throwable weapons.
// Returns true if the primary attack should be suppressed this frame
// (because we're in physical throw mode and managing IN_ATTACK ourselves).
//-----------------------------------------------------------------------------
void CVRInput::ProcessThrowGesture( CUserCmd *cmd, bool bTriggerHeld, bool bSuppressTrigger )
{
    C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
    if ( !pPlayer )
        return;

    CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

    // If the current weapon isn't throwable, reset state and bail
    if ( !IsThrowableWeapon( pWeapon ) )
    {
        if ( m_bThrowHolding )
        {
            m_bThrowHolding = false;
            m_throwTracker.Reset();
            if ( tfvr_physical_throw_debug.GetBool() )
                DevMsg( "VR Throw: Reset (weapon changed)\n" );
        }
        m_nLastThrowableWeaponID = -1;
        return;
    }

    // If we switched to a different throwable, reset tracking
    int nWeaponID = pWeapon->GetWeaponID();
    if ( nWeaponID != m_nLastThrowableWeaponID )
    {
        m_bThrowHolding = false;
        m_throwTracker.Reset();
        m_nLastThrowableWeaponID = nWeaponID;
    }

    // Grip activation uses squeeze/force (requires deliberate squeeze on Index).
    // Release detection uses squeeze/value (instant binary on Index).
    // This gives a firm activation threshold without losing throw velocity on release.
    float flGripForce = g_pOpenXRManager->GetAnalogValue( "right_grip" );
    float flGripValue = g_pOpenXRManager->GetAnalogValue( "right_grip_value" );
    bool bGripActivate = ( flGripForce > tfvr_throw_grip_threshold.GetFloat() );
    bool bGripStillHeld = ( flGripValue > 0.5f );

    // Use force to start the hold, value to sustain it
    bool bGripHeld = m_bThrowHolding ? bGripStillHeld : bGripActivate;

    bool bEitherHeld = bGripHeld || ( bTriggerHeld && !bSuppressTrigger );

    // Get weapon-hand controller pose.  Position is stored relative to the
    // player entity so artificial locomotion doesn't inflate the velocity.
    VMatrix controllerPose;
    bool bPoseValid = g_pOpenXRManager->GetRightControllerPose( controllerPose );
    Vector vecHandPos = vec3_origin;
    QAngle angHand( 0, 0, 0 );
    if ( bPoseValid )
    {
        vecHandPos = controllerPose.GetTranslation() - pPlayer->GetAbsOrigin();
        MatrixAngles( controllerPose.As3x4(), angHand );
    }

    if ( !m_bThrowHolding )
    {
        // Not holding yet — check if player just grabbed
        if ( bEitherHeld && bPoseValid )
        {
            m_bThrowHolding = true;
            m_throwTracker.Reset();
            m_throwTracker.AddSample( vecHandPos, angHand, gpGlobals->curtime );

            if ( tfvr_physical_throw_debug.GetBool() )
                DevMsg( "VR Throw: Holding started (force=%.2f value=%.2f trigger=%d)\n", flGripForce, flGripValue, bTriggerHeld ? 1 : 0 );
        }
    }
    else
    {
        // Currently holding
        if ( bPoseValid )
            m_throwTracker.AddSample( vecHandPos, angHand, gpGlobals->curtime );

        if ( !bEitherHeld )
        {
            // Released! Compute throw velocity and fire.
            // The tracker gives us gesture velocity relative to the player.
            // Player movement velocity is added server-side after the speed
            // multiplier so it isn't amplified by the throw params.
            Vector vecThrowVel = m_throwTracker.GetAveragedVelocity();

            cmd->vrThrowVelocity = vecThrowVel;

            // Origin as player-relative offset — the server reconstructs
            // world-space from its own player position, avoiding latency drift.
            cmd->vrThrowOrigin = bPoseValid ? vecHandPos : vec3_origin;

            // Hand orientation at release and angular velocity (deg/sec, world XYZ)
            cmd->vrThrowAngles = m_throwTracker.GetNewestAngles();
            cmd->vrThrowAngVel = m_throwTracker.GetAveragedAngularVelocity();

            cmd->buttons |= IN_ATTACK;

            m_bThrowHolding = false;
            m_throwTracker.Reset();

            if ( tfvr_physical_throw_debug.GetBool() )
            {
                DevMsg( "VR Throw: Released! velocity=(%.1f, %.1f, %.1f) speed=%.1f angVel=(%.1f, %.1f, %.1f)\n",
                        vecThrowVel.x, vecThrowVel.y, vecThrowVel.z, vecThrowVel.Length(),
                        cmd->vrThrowAngVel.x, cmd->vrThrowAngVel.y, cmd->vrThrowAngVel.z );
            }
        }
    }

    // While in holding state, strip IN_ATTACK so the weapon doesn't fire prematurely
    if ( m_bThrowHolding )
    {
        cmd->buttons &= ~IN_ATTACK;
    }
}

//-----------------------------------------------------------------------------
bool CVRInput::IsMouthActivatedWeapon( C_TFWeaponBase *pWeapon )
{
    if ( !pWeapon )
        return false;

    int id = pWeapon->GetWeaponID();
    return ( id == TF_WEAPON_LUNCHBOX || id == TF_WEAPON_BUFF_ITEM );
}

//-----------------------------------------------------------------------------
// Returns true when the weapon-holding controller is within mouth proximity.
// Uses raw playspace coordinates (same space as voice gesture ear check).
//-----------------------------------------------------------------------------
bool CVRInput::IsWeaponNearMouth( C_TFWeaponBase *pWeapon )
{
    if ( !g_pOpenXRManager || !pWeapon )
        return false;

    // Determine which hand holds the weapon
    C_TFVRHand *pLeftHand = GetLocalPlayerLeftHand();
    C_TFVRHand *pRightHand = GetLocalPlayerRightHand();

    bool bWeaponInLeft = ( pLeftHand && pLeftHand->GetHeldWeapon() == pWeapon );
    bool bWeaponInRight = ( pRightHand && pRightHand->GetHeldWeapon() == pWeapon );

    if ( !bWeaponInLeft && !bWeaponInRight )
        return false;

    // Get weapon-hand controller position in raw playspace
    VMatrix controllerPose;
    bool bPoseValid = false;
    if ( bWeaponInLeft )
        bPoseValid = g_pOpenXRManager->GetLeftControllerPoseRaw( controllerPose );
    else
        bPoseValid = g_pOpenXRManager->GetRightControllerPoseRaw( controllerPose );

    if ( !bPoseValid )
        return false;

    Vector controllerPos = controllerPose.GetTranslation();

    // Get HMD pose in playspace
    Vector hmdOrigin;
    QAngle hmdAngles;
    g_pOpenXRManager->GetHMDInChaperone( hmdOrigin, hmdAngles );

    Vector hmdForward, hmdRight, hmdUp;
    AngleVectors( hmdAngles, &hmdForward, &hmdRight, &hmdUp );

    // Mouth is slightly forward and below head center
    float fwdOffset = tfvr_mouth_forward_offset.GetFloat();
    float downOffset = tfvr_mouth_down_offset.GetFloat();
    Vector mouthPos = hmdOrigin + hmdForward * fwdOffset - hmdUp * downOffset;

    float distance = ( controllerPos - mouthPos ).Length();
    float radius = tfvr_mouth_radius.GetFloat();

    if ( tfvr_mouth_activate_debug.GetBool() )
    {
        static float flLastDebug = 0.0f;
        if ( gpGlobals->curtime - flLastDebug > 0.3f )
        {
            DevMsg( "VR Mouth: dist=%.1f radius=%.1f hand=%s weapon=%d\n",
                    distance, radius,
                    bWeaponInLeft ? "Left" : "Right",
                    pWeapon->GetWeaponID() );
            flLastDebug = gpGlobals->curtime;
        }
    }

    // Debug draw: wireframe sphere at the mouth zone in world space
    if ( tfvr_mouth_debug_draw.GetBool() && debugoverlay )
    {
        // Convert mouth position from playspace to world space
        VMatrix rawHeadPlayspace = g_pOpenXRManager->GetMideyePose();
        VMatrix smoothedHeadWorld = g_ClientVirtualReality.GetWorldFromMidEyeRaw();

        // Build mouth position as a point in playspace, then transform
        // mouthPos is already in playspace; express it relative to head,
        // then apply the smoothed world transform.
        VMatrix mouthInPlayspace;
        mouthInPlayspace.Identity();
        mouthInPlayspace.SetTranslation( mouthPos );

        VMatrix mouthRelHead = rawHeadPlayspace.InverseTR() * mouthInPlayspace;
        VMatrix mouthWorld = smoothedHeadWorld * mouthRelHead;
        Vector worldMouthPos = mouthWorld.GetTranslation();

        bool bInside = ( distance <= radius );
        int r = bInside ? 0 : 255;
        int g = bInside ? 255 : 100;
        int b = 0;

        const int kSegments = 24;
        for ( int ring = 0; ring < 3; ring++ )
        {
            for ( int i = 0; i < kSegments; i++ )
            {
                float a0 = ( 2.0f * M_PI * i ) / kSegments;
                float a1 = ( 2.0f * M_PI * ( i + 1 ) ) / kSegments;

                Vector p0, p1;
                if ( ring == 0 )      // XY
                {
                    p0 = worldMouthPos + Vector( cos(a0), sin(a0), 0 ) * radius;
                    p1 = worldMouthPos + Vector( cos(a1), sin(a1), 0 ) * radius;
                }
                else if ( ring == 1 ) // XZ
                {
                    p0 = worldMouthPos + Vector( cos(a0), 0, sin(a0) ) * radius;
                    p1 = worldMouthPos + Vector( cos(a1), 0, sin(a1) ) * radius;
                }
                else                  // YZ
                {
                    p0 = worldMouthPos + Vector( 0, cos(a0), sin(a0) ) * radius;
                    p1 = worldMouthPos + Vector( 0, cos(a1), sin(a1) ) * radius;
                }

                debugoverlay->AddLineOverlayAlpha( p0, p1, r, g, b, 200, false, 0.016f );
            }
        }
    }

    return ( distance <= radius );
}

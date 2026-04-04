//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

#include "client_virtualreality.h"

#include "materialsystem/itexture.h"
#include "materialsystem/materialsystem_config.h"
#include "view_shared.h"
#include "view_scene.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/Controls.h"
#include "sourcevr/isourcevirtualreality.h"
#include "ienginevgui.h"
#include "tfvr/hmdWrapper.h"
#include "cdll_client_int.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "tier0/vprof_telemetry.h"
#include <time.h>
#include "steam/steam_api.h"
#include <tfvr/openxr_manager.h>
#include "tf/c_tf_player.h"
#include "engine/ivdebugoverlay.h"
#include "econ/econ_ui.h"
#include "tf/vgui/class_loadout_panel.h"
#include "tf/vgui/character_info_panel.h"
#include "tfvr/vr_menu_manager.h"

const char *COM_GetModDirectory(); // return the mod dir (rather than the complete -game param, which can be a path)

// External debug overlay interface for drawing debug visualizations
extern IVDebugOverlay *debugoverlay;

CClientVirtualReality g_ClientVirtualReality;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientVirtualReality, IClientVirtualReality, 
	CLIENTVIRTUALREALITY_INTERFACE_VERSION, g_ClientVirtualReality );


// --------------------------------------------------------------------
// A huge pile of VR convars
// --------------------------------------------------------------------
ConVar vr_activate_default( "vr_activate_default",		"0", FCVAR_ARCHIVE, "If this is true the game will switch to VR mode once startup is complete." );

// Debug visualization ConVars
ConVar tfvr_debug_playspace_origin( "tfvr_debug_playspace_origin", "0", FCVAR_NONE, "Draw a debug cube at the calculated playspace origin in world coordinates" );

ConVar vr_moveaim_mode      ( "vr_moveaim_mode",      "3", FCVAR_ARCHIVE, "0=move+shoot from face. 1=move with torso. 2,3,4=shoot with face+mouse cursor. 5+ are probably not that useful." );
ConVar vr_moveaim_mode_zoom ( "vr_moveaim_mode_zoom", "3", FCVAR_ARCHIVE, "0=move+shoot from face. 1=move with torso. 2,3,4=shoot with face+mouse cursor. 5+ are probably not that useful." );

ConVar vr_moveaim_reticle_yaw_limit        ( "vr_moveaim_reticle_yaw_limit",        "10", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse drags the torso" );
ConVar vr_moveaim_reticle_pitch_limit      ( "vr_moveaim_reticle_pitch_limit",      "30", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse clamps" );
// Note these are scaled by the zoom factor.
ConVar vr_moveaim_reticle_yaw_limit_zoom   ( "vr_moveaim_reticle_yaw_limit_zoom",   "0", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse drags the torso" );
ConVar vr_moveaim_reticle_pitch_limit_zoom ( "vr_moveaim_reticle_pitch_limit_zoom", "-1", FCVAR_ARCHIVE, "Beyond this number of degrees, the mouse clamps" );

// This are somewhat obsolete.
ConVar vr_aim_yaw_offset( "vr_aim_yaw_offset", "90", 0, "This value is added to Yaw when returning the vehicle aim angles to Source." );

ConVar vr_stereo_swap_eyes ( "vr_stereo_swap_eyes", "0", 0, "1=swap eyes." );

// Useful for debugging wacky-projection problems, separate from multi-rendering problems.
ConVar vr_stereo_mono_set_eye ( "vr_stereo_mono_set_eye", "0", 0, "0=off, Set all eyes to 1=left, 2=right, 3=middle eye" );

// Useful for examining anims, etc.
ConVar vr_debug_remote_cam( "vr_debug_remote_cam", "0" );
ConVar vr_debug_remote_cam_pos_x( "vr_debug_remote_cam_pos_x", "150.0" );
ConVar vr_debug_remote_cam_pos_y( "vr_debug_remote_cam_pos_y", "0.0" );
ConVar vr_debug_remote_cam_pos_z( "vr_debug_remote_cam_pos_z", "0.0" );
ConVar vr_debug_remote_cam_target_x( "vr_debug_remote_cam_target_x", "0.0" );
ConVar vr_debug_remote_cam_target_y( "vr_debug_remote_cam_target_y", "0.0" );
ConVar vr_debug_remote_cam_target_z( "vr_debug_remote_cam_target_z", "-50.0" );

ConVar vr_translation_limit( "vr_translation_limit", "10.0", 0, "How far the in-game head will translate before being clamped." );

// HUD config values
ConVar vr_render_hud_in_world( "vr_render_hud_in_world", "1" );
ConVar vr_hud_max_fov( "vr_hud_max_fov", "60", FCVAR_ARCHIVE, "Max FOV of the HUD" );
ConVar vr_hud_forward( "vr_hud_forward", "500", FCVAR_ARCHIVE, "Apparent distance of the HUD in inches" );
ConVar vr_hud_display_ratio( "vr_hud_display_ratio", "0.95", FCVAR_ARCHIVE );
ConVar vr_hud_never_overlay( "vr_hud_never_overlay", "0" );

ConVar vr_hud_axis_lock_to_world( "vr_hud_axis_lock_to_world", "0", FCVAR_ARCHIVE, "Bitfield - locks HUD axes to the world - 0=pitch, 1=yaw, 2=roll" );

// Default distance clips through rocketlauncher, heavy's body, etc.
ConVar vr_projection_znear_multiplier( "vr_projection_znear_multiplier", "0.3", 0, "Allows moving the ZNear plane to deal with body clipping" );

// Should the viewmodel (weapon) translate with the HMD, or remain fixed to the in-world body (but still rotate with the head)? Purely a graphics effect - no effect on actual bullet aiming.
// Has no effect in aim modes where aiming is not controlled by the head.
ConVar vr_viewmodel_translate_with_head ( "vr_viewmodel_translate_with_head", "0", 0, "1=translate the viewmodel with the head motion." );

ConVar vr_zoom_multiplier ( "vr_zoom_multiplier", "2.0", FCVAR_ARCHIVE, "When zoomed, how big is the scope on your HUD?" );
ConVar vr_zoom_scope_scale ( "vr_zoom_scope_scale", "6.0", 0, "Something to do with the default scope HUD overlay size." );		// Horrible hack - should work out the math properly, but we need to ship.


ConVar vr_viewmodel_offset_forward( "vr_viewmodel_offset_forward", "-8", 0 );
ConVar vr_viewmodel_offset_forward_large( "vr_viewmodel_offset_forward_large", "-15", 0 );

ConVar vr_force_windowed ( "vr_force_windowed", "0", FCVAR_ARCHIVE );

ConVar vr_first_person_uses_world_model ( "vr_first_person_uses_world_model", "0", 0, "Causes the third person model to be drawn instead of the view model" );

extern ConVar tfvr_menu_distance;
extern ConVar tfvr_menu_scale;

extern ConVar tfvr_hud_onwrist;
extern ConVar tfvr_hud_forward;
extern ConVar tfvr_hud_scale;
extern ConVar tfvr_hud_axis_lock_to_world;
extern ConVar tfvr_hud_height_adjust;

// --------------------------------------------------------------------
// Purpose: Cycle through the aim & move modes.
// --------------------------------------------------------------------
void CC_VR_Cycle_Aim_Move_Mode ( const CCommand& args )
{
	int hmmCurrentMode = vr_moveaim_mode.GetInt();
	if ( g_ClientVirtualReality.CurrentlyZoomed() )
	{
		hmmCurrentMode = vr_moveaim_mode_zoom.GetInt();
	}

	hmmCurrentMode++;
	if ( hmmCurrentMode >= HMM_LAST )
	{
		hmmCurrentMode = 0;
	}

	if ( g_ClientVirtualReality.CurrentlyZoomed() )
	{
		vr_moveaim_mode_zoom.SetValue ( hmmCurrentMode );
		Warning ( "Headtrack mode (zoomed) %d\n", hmmCurrentMode );
	}
	else
	{
		vr_moveaim_mode.SetValue ( hmmCurrentMode );
		Warning ( "Headtrack mode %d\n", hmmCurrentMode );
	}
}
static ConCommand vr_cycle_aim_move_mode("vr_cycle_aim_move_mode", CC_VR_Cycle_Aim_Move_Mode, "Cycle through the aim & move modes." );


// --------------------------------------------------------------------
// Purpose:  Switch to/from VR mode.
// --------------------------------------------------------------------
CON_COMMAND( vr_activate, "Switch to VR mode" )
{
	g_ClientVirtualReality.Activate();
}
CON_COMMAND( vr_deactivate, "Switch from VR mode to normal mode" )
{
	g_ClientVirtualReality.Deactivate();
}
CON_COMMAND( vr_toggle, "Toggles VR mode" )
{
	if( g_pOpenXRManager )
	{
		if( g_pOpenXRManager->IsActive() )
			g_ClientVirtualReality.Deactivate();
		else
			g_ClientVirtualReality.Activate();
	}
	else
	{
		Msg( "VR Mode is not enabled.\n" );
	}
}


// --------------------------------------------------------------------
// Purpose: Returns true if the matrix is orthonormal
// --------------------------------------------------------------------
bool IsOrthonormal ( VMatrix Mat, float fTolerance )
{
	float LenFwd = Mat.GetForward().Length();
	float LenUp = Mat.GetUp().Length();
	float LenLeft = Mat.GetLeft().Length();
	float DotFwdUp = Mat.GetForward().Dot ( Mat.GetUp() );
	float DotUpLeft = Mat.GetUp().Dot ( Mat.GetLeft() );
	float DotLeftFwd = Mat.GetLeft().Dot ( Mat.GetForward() );
	if ( fabsf ( LenFwd - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenUp - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( LenLeft - 1.0f ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotFwdUp ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotUpLeft ) > fTolerance )
	{
		return false;
	}
	if ( fabsf ( DotLeftFwd ) > fTolerance )
	{
		return false;
	}
	return true;
}

static bool IsMenuUp()
{
	return (enginevgui && enginevgui->IsGameUIVisible()) || vgui::surface()->IsCursorVisible();
}

// --------------------------------------------------------------------
// Purpose: Computes the FOV from the projection matrix
// --------------------------------------------------------------------
void CalcFovFromProjection ( float *pFov, const VMatrix &proj )
{
	// The projection matrices should be of the form:
	// p0  0   z1 p1 
	// 0   p2  z2 p3
	// 0   0   z3 1
	// (p0 = X fov, p1 = X offset, p2 = Y fov, p3 = Y offset )
	// TODO: cope with more complex projection matrices?
	float xscale  = proj.m[0][0];
	Assert ( proj.m[0][1] == 0.0f );
	float xoffset = proj.m[0][2];
	Assert ( proj.m[0][3] == 0.0f );
	Assert ( proj.m[1][0] == 0.0f );
	float yscale  = proj.m[1][1];
	float yoffset = proj.m[1][2];
	Assert ( proj.m[1][3] == 0.0f );
	// Row 2 determines Z-buffer values - don't care about those for now.
	Assert ( proj.m[3][0] == 0.0f );
	Assert ( proj.m[3][1] == 0.0f );
	Assert ( proj.m[3][2] == -1.0f );
	Assert ( proj.m[3][3] == 0.0f );

	/*
	// The math here:
	// A view-space vector (x,y,z,1) is transformed by the projection matrix
	// / xscale   0     xoffset  0 \
	// |    0   yscale  yoffset  0 |
	// |    ?     ?        ?     ? |
	// \    0     0       -1     0 /
	//
	// Then the result is normalized (i.e. divide by w) and the result clipped to the [-1,+1] unit cube.
	// (ignore Z for now, and the clipping is slightly different).
	// So, we want to know what vectors produce a clip value of -1 and +1 in each direction, e.g. in the X direction:
	//    +-1 = ( xscale*x + xoffset*z ) / (-1*z)
	//        = xscale*(x/z) + xoffset            (I flipped the signs of both sides)
	// => (+-1 - xoffset)/xscale = x/z
	// ...and x/z is tan(theta), and theta is the half-FOV.
	*/

	float fov_px = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - xoffset ) / xscale ) ) );
	float fov_nx = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - xoffset ) / xscale ) ) );
	float fov_py = 2.0f * RAD2DEG ( atanf ( fabsf ( (  1.0f - yoffset ) / yscale ) ) );
	float fov_ny = 2.0f * RAD2DEG ( atanf ( fabsf ( ( -1.0f - yoffset ) / yscale ) ) );

	*pFov = Max ( Max ( fov_px, fov_nx ), Max ( fov_py, fov_ny ) );
	// FIXME: hey you know, I could do the Max() series before I call all those expensive atanf()s...
}


// --------------------------------------------------------------------
// construction/destruction
// --------------------------------------------------------------------
CClientVirtualReality::CClientVirtualReality()
{
	m_PlayerTorsoOrigin.Init();
	m_PlayerTorsoAngle.Init();
	m_WorldFromWeapon.Identity();
	m_WorldFromMidEye.Identity();
	m_WorldFromMidEyeRaw.Identity();
	
	m_bOverrideTorsoAngle = false;
	m_OverrideTorsoOffset.Init();

	// Also reset our model of the player's torso orientation
	m_PlayerTorsoAngle.Init ( 0.0f, 0.0f, 0.0f );

	m_WorldZoomScale = 1.0f;
	m_hmmMovementActual = HMM_SHOOTFACE_MOVEFACE;
	m_iAlignTorsoAndViewToWeaponCountdown = 0;
	
	// Initialize custom HUD bounds
	m_bCustomHUDBoundsSet = false;
	m_CustomHUDViewer.Init();
	m_CustomHUDUL.Init();
	m_CustomHUDUR.Init();
	m_CustomHUDLL.Init();
	m_CustomHUDLR.Init();

	m_rtLastMotionSample = 0;
	m_bMotionUpdated = false;

#if defined( USE_SDL )
    m_nNonVRSDLDisplayIndex = 0;
#endif
}

CClientVirtualReality::~CClientVirtualReality()
{
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
bool			CClientVirtualReality::Connect( CreateInterfaceFn factory )
{
	if ( !factory )
		return false;

	if ( !BaseClass::Connect( factory ) )
		return false;

	return true;
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void			CClientVirtualReality::Disconnect()
{
	BaseClass::Disconnect();
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void *			CClientVirtualReality::QueryInterface( const char *pInterfaceName )
{
	CreateInterfaceFn factory = Sys_GetFactoryThis();	// This silly construction is necessary
	return factory( pInterfaceName, NULL );				// to prevent the LTCG compiler from crashing.
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
InitReturnVal_t	CClientVirtualReality::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	return INIT_OK;
}


// --------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------
void			CClientVirtualReality::Shutdown()
{
	BaseClass::Shutdown();
}


// --------------------------------------------------------------------
// Purpose: Draws the main menu in Stereo
// --------------------------------------------------------------------
void CClientVirtualReality::DrawMainMenu()
{
	// have to draw the UI in stereo via the render texture or it won't fuse properly

	// Draw it into the render target first
	ITexture *pTexture = materials->FindTexture( "_rt_vgui", NULL, false );
	Assert( pTexture );
	if( !pTexture) 
		return;

	CMatRenderContextPtr pRenderContext( materials );
	int viewActualWidth = pTexture->GetActualWidth();
	int viewActualHeight = pTexture->GetActualHeight();

	int viewWidth, viewHeight;
	vgui::surface()->GetScreenSize( viewWidth, viewHeight );

	// clear depth in the backbuffer before we push the render target
	pRenderContext->ClearBuffers( false, true, true );

	// constrain where VGUI can render to the view
	pRenderContext->PushRenderTargetAndViewport( pTexture, NULL, 0, 0, viewActualWidth, viewActualHeight );
	pRenderContext->OverrideAlphaWriteEnable( true, true );

	// clear the render target 
	pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
	pRenderContext->ClearBuffers( true, false );

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "VGui_DrawHud", __FUNCTION__ );

	// Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
	vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
	}
	// Same for client .dll tools
	root = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, viewWidth, viewHeight );
	}

	// paint the main menu and cursor
	render->VGui_Paint( (PaintMode_t) ( PAINT_UIPANELS | PAINT_CURSOR ) );

	pRenderContext->OverrideAlphaWriteEnable( false, true );
	pRenderContext->PopRenderTargetAndViewport();
	pRenderContext->Flush();

	int leftX, leftY, leftW, leftH, rightX, rightY, rightW, rightH;
	g_pOpenXRManager->GetViewportBounds( ISourceVirtualReality::VREye_Left, &leftX, &leftY, &leftW, &leftH );
	g_pOpenXRManager->GetViewportBounds( ISourceVirtualReality::VREye_Right, &rightX, &rightY, &rightW, &rightH );


	// render the main view
	CViewSetup viewEye[STEREO_EYE_MAX];
	viewEye[ STEREO_EYE_MONO ].zNear = 0.1;
	viewEye[ STEREO_EYE_MONO ].zFar = 10000.f;
	viewEye[ STEREO_EYE_MONO ].angles.Init();
	viewEye[ STEREO_EYE_MONO ].origin.Zero();
	viewEye[ STEREO_EYE_MONO ].x = viewEye[ STEREO_EYE_MONO ].m_nUnscaledX =  leftX;
	viewEye[ STEREO_EYE_MONO ].y = viewEye[ STEREO_EYE_MONO ].m_nUnscaledY = leftY;
	viewEye[ STEREO_EYE_MONO ].width = viewEye[ STEREO_EYE_MONO ].m_nUnscaledWidth = leftW;
	viewEye[ STEREO_EYE_MONO ].height = viewEye[ STEREO_EYE_MONO ].m_nUnscaledHeight = leftH;

	viewEye[STEREO_EYE_LEFT] = viewEye[STEREO_EYE_RIGHT] = viewEye[ STEREO_EYE_MONO ] ;
	viewEye[STEREO_EYE_LEFT].m_eStereoEye = STEREO_EYE_LEFT;
	viewEye[STEREO_EYE_RIGHT].x = rightX;
	viewEye[STEREO_EYE_RIGHT].y = rightY;
	viewEye[STEREO_EYE_RIGHT].m_eStereoEye = STEREO_EYE_RIGHT;

	// let sourcevr.dll tell us where to put the cameras
	ProcessCurrentTrackingState( 0 );
	Vector vViewModelOrigin;
	QAngle qViewModelAngles;
	OverrideView( &viewEye[ STEREO_EYE_MONO ] , &vViewModelOrigin, &qViewModelAngles, HMM_NOOVERRIDE );
	g_ClientVirtualReality.OverrideStereoView( &viewEye[ STEREO_EYE_MONO ] , &viewEye[STEREO_EYE_LEFT], &viewEye[STEREO_EYE_RIGHT] );

	// render both eyes
	for( int nView = STEREO_EYE_LEFT; nView <= STEREO_EYE_RIGHT; nView++ )
	{
		CMatRenderContextPtr pRenderContextMat( materials );
		PIXEvent pixEvent( pRenderContextMat, nView == STEREO_EYE_LEFT ? "left eye" : "right eye" );

		ITexture *pColor = g_pOpenXRManager->GetRenderTarget();
		ITexture *pDepth = g_pOpenXRManager->GetRenderTarget();
		render->Push3DView( viewEye[nView], VIEW_CLEAR_DEPTH|VIEW_CLEAR_COLOR, pColor, NULL, pDepth );
		RenderHUDQuad( false );
		render->PopView( NULL );

		PostProcessFrame( (StereoEye_t)nView );

		OverlayHUDQuadWithUndistort( viewEye[nView], true, true, false );
	}
}


// --------------------------------------------------------------------
// Purpose:
//		Offset the incoming view appropriately.
//		Set up the "middle eye" from that.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideView ( CViewSetup *pViewMiddle, Vector *pViewModelOrigin, QAngle *pViewModelAngles, HeadtrackMovementMode_t hmmMovementOverride )
{
	return true;
}


// --------------------------------------------------------------------
// Purpose:
//		In some aim/move modes, the HUD aim reticle lags because it's
//		using slightly stale data. This will feed it the newest data. 
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideWeaponHudAimVectors ( Vector *pAimOrigin, Vector *pAimDirection )
{
	if( !UseVR() )
	{
		return false;
	}

	Assert ( pAimOrigin != NULL );
	Assert ( pAimDirection != NULL );

	// VR ball aim: use offhand (left) controller for crosshair when aiming a ball
	extern bool g_bVRBallAimActive;
	extern Vector g_vecVRBallAimOrigin;
	extern QAngle g_angVRBallAimAngles;

	if (g_bVRBallAimActive && g_vecVRBallAimOrigin != vec3_origin)
	{
		*pAimOrigin = g_vecVRBallAimOrigin;

		Vector forward;
		AngleVectors( g_angVRBallAimAngles, &forward );
		*pAimDirection = forward;

		extern ConVar tfvr_crosshair_follow_controller_roll;
		if (tfvr_crosshair_follow_controller_roll.GetBool())
		{
			m_flCrosshairRollAngle = g_angVRBallAimAngles.z - 90.0f;
			m_bCrosshairRollValid = true;
		}
		else
		{
			m_bCrosshairRollValid = false;
		}

		return true;
	}

	// Use the player's weapon shooting position and angles for crosshair (controller-based in VR)
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		// Cast to TF player to access VR-specific functions
		C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
		if ( pTFPlayer )
		{
			// Use weapon shoot position for origin (controller position in VR)
			*pAimOrigin = pTFPlayer->Weapon_ShootPosition();
			
			// Use weapon shoot angles for direction (controller angles in VR)
			QAngle weaponAngles = pTFPlayer->Weapon_ShootAngles();
			Vector forward;
			AngleVectors( weaponAngles, &forward );
			*pAimDirection = forward;
			
			// Store the weapon roll angle for crosshair rotation
			extern ConVar tfvr_crosshair_follow_controller_roll;
			if (tfvr_crosshair_follow_controller_roll.GetBool())
			{
				// Use weapon roll instead of controller roll
				// Add 90 degree offset to account for weapon model orientation
				m_flCrosshairRollAngle = weaponAngles.z - 90.0f;
				m_bCrosshairRollValid = true;
			}
			else
			{
				m_bCrosshairRollValid = false;
			}
			
			// Apply crosshair offset if any ConVars are set
			extern ConVar tfvr_crosshair_offset_x, tfvr_crosshair_offset_y, tfvr_crosshair_offset_z;
			if (tfvr_crosshair_offset_x.GetFloat() != 0.0f || tfvr_crosshair_offset_y.GetFloat() != 0.0f || tfvr_crosshair_offset_z.GetFloat() != 0.0f)
			{
				Vector offset(tfvr_crosshair_offset_x.GetFloat(), tfvr_crosshair_offset_y.GetFloat(), tfvr_crosshair_offset_z.GetFloat());
				*pAimOrigin += offset;
			}
			
			return true;
		}
	}

	return false;
}


// --------------------------------------------------------------------
// Purpose:
//		Set up the left and right eyes from the middle eye if stereo is on.
//		Advise calling soonish after OverrideView().
// --------------------------------------------------------------------
bool CClientVirtualReality::OverrideStereoView( CViewSetup *pViewMiddle, CViewSetup *pViewLeft, CViewSetup *pViewRight  )
{
	// Everything in here is in Source coordinate space.
	if( !UseVR() )
	{
		// CRITICAL: When VR is off, ensure projection matrix overrides are disabled
		// This restores normal FOV calculation instead of using VR projection matrices
		DevMsg("OverrideStereoView: VR is OFF - resetting projection overrides. Middle was: %s\n", 
			pViewMiddle->m_bViewToProjectionOverride ? "OVERRIDDEN" : "NORMAL");
		pViewMiddle->m_bViewToProjectionOverride = false;
		pViewLeft->m_bViewToProjectionOverride = false; 
		pViewRight->m_bViewToProjectionOverride = false;
		return false;
	}

	const VMatrix viewAsMatrix = SetupMatrixOrgAngles(pViewMiddle->origin, pViewMiddle->angles);

	VMatrix leftEyeView = viewAsMatrix * g_pOpenXRManager->GetEyeViewFromMidEyeView(ISourceVirtualReality::VREye_Left);
	VMatrix rightEyeView = viewAsMatrix * g_pOpenXRManager->GetEyeViewFromMidEyeView(ISourceVirtualReality::VREye_Right);

	MatrixToAngles(leftEyeView, pViewLeft->angles);
	pViewLeft->origin = leftEyeView.GetTranslation();

	MatrixToAngles(rightEyeView, pViewRight->angles);
	pViewRight->origin = rightEyeView.GetTranslation();

	// Find the projection matrices.
	pViewLeft->m_bViewToProjectionOverride = true;
	pViewRight->m_bViewToProjectionOverride = true;
	g_pOpenXRManager->GetEyeProjectionMatrix(pViewLeft->m_ViewToProjection, ISourceVirtualReality::VREye_Left, pViewMiddle->zNear, pViewMiddle->zFar);
	g_pOpenXRManager->GetEyeProjectionMatrix(pViewRight->m_ViewToProjection, ISourceVirtualReality::VREye_Right, pViewMiddle->zNear, pViewMiddle->zFar);

	// And bodge together some sort of average for our cyclops friends.
	pViewMiddle->m_bViewToProjectionOverride = true;
	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			pViewMiddle->m_ViewToProjection.m[i][j] = (pViewLeft->m_ViewToProjection.m[i][j] + pViewRight->m_ViewToProjection.m[i][j] ) * 0.5f;
		}
	}

	switch ( vr_stereo_mono_set_eye.GetInt() )
	{
	case 0:
		// ... nothing.
		break;
	case 1:
		// Override all eyes with left
		*pViewMiddle = *pViewLeft;
		*pViewRight = *pViewLeft;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	case 2:
		// Override all eyes with right
		*pViewMiddle = *pViewRight;
		*pViewLeft = *pViewRight;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		break;
	case 3:
		// Override all eyes with middle
		*pViewRight = *pViewMiddle;
		*pViewLeft = *pViewMiddle;
		pViewLeft->m_eStereoEye = STEREO_EYE_LEFT;
		pViewRight->m_eStereoEye = STEREO_EYE_RIGHT;
		break;
	}

	// To make culling work correctly, calculate the widest FOV of each projection matrix.
	CalcFovFromProjection ( &(pViewLeft  ->fov), pViewLeft  ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewRight ->fov), pViewRight ->m_ViewToProjection );
	CalcFovFromProjection ( &(pViewMiddle->fov), pViewMiddle->m_ViewToProjection );

	// Figure out the current HUD FOV.
	m_fHudHorizontalFov = pViewLeft->fov * (IsMenuUp() ? tfvr_menu_scale : tfvr_hud_scale).GetFloat();

	// remember the view angles so we can limit the weapon to something near those
	m_PlayerViewAngle = pViewMiddle->angles;
	m_PlayerViewOrigin = pViewMiddle->origin + Vector(0, 0, tfvr_hud_height_adjust.GetFloat());

	// Figure out the HUD vectors and frustum.
	// The aspect ratio of the HMD may be something bizarre (e.g. Rift is 640x800), and the pixels may not be square, so don't use that!
	// Use 16:9 aspect ratio to match the _rt_vgui texture (all presets are 16:9)
	static const float fAspectRatio = 16.f/9.f;
	float fHFOV = m_fHudHorizontalFov;
	float fVFOV = m_fHudHorizontalFov / fAspectRatio;

	const float fHudForward = (IsMenuUp() ? tfvr_menu_distance.GetFloat() : tfvr_hud_forward.GetFloat());
	m_fHudHalfWidth = tan( DEG2RAD( fHFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;
	m_fHudHalfHeight = tan( DEG2RAD( fVFOV * 0.5f ) ) * fHudForward * m_WorldZoomScale;

	QAngle HudAngles;
	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVETORSO:
		// Put the HUD in front of the player's torso.
		// This helps keep you oriented about where "forwards" is, which is otherwise surprisingly tricky!
		// TODO: try preserving roll and/or pitch from the view?
		HudAngles = m_PlayerTorsoAngle;
		break;
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
		// Put the HUD in front of wherever the player is looking.
		HudAngles = m_PlayerViewAngle;
		break;
	default: Assert ( false ); break;
	}

	// This is a bitfield. A set bit means lock to the world, a clear bit means don't.
	int iVrHudAxisLockToWorld = tfvr_hud_axis_lock_to_world.GetInt();
	
	// When locking roll to world, we need to compute orientation using world-up
	// Simply zeroing the roll Euler angle doesn't work because the angles are coupled
	bool bLockRoll = ( iVrHudAxisLockToWorld & (1<<ROLL) ) != 0;
	bool bLockPitch = ( iVrHudAxisLockToWorld & (1<<PITCH) ) != 0;
	
	if ( bLockRoll )
	{
		// Compute HUD forward from angles (ignoring roll)
		Vector hudForward;
		AngleVectors( HudAngles, &hudForward, nullptr, nullptr );
		
		// If locking pitch, project forward onto horizontal plane
		if ( bLockPitch )
		{
			hudForward.z = 0.0f;
			float len = hudForward.NormalizeInPlace();
			if ( len < 0.001f )
			{
				// Looking straight up/down, use a default forward
				hudForward = Vector(1, 0, 0);
			}
		}
		
		// Use world-up to compute a level orientation
		Vector worldUp(0, 0, 1);
		Vector hudRight = CrossProduct(worldUp, hudForward);
		float rightLen = hudRight.NormalizeInPlace();
		
		Vector hudUp;
		if ( rightLen < 0.001f )
		{
			// HUD forward is straight up/down
			hudRight = Vector(1, 0, 0);
			hudUp = CrossProduct(hudForward, hudRight);
			hudUp.NormalizeInPlace();
		}
		else
		{
			hudUp = CrossProduct(hudForward, hudRight);
			hudUp.NormalizeInPlace();
		}
		
		// Build the matrix directly instead of using SetupMatrixOrgAngles
		m_WorldFromHud.Identity();
		m_WorldFromHud[0][0] = hudForward.x;  m_WorldFromHud[0][1] = -hudRight.x;  m_WorldFromHud[0][2] = hudUp.x;
		m_WorldFromHud[1][0] = hudForward.y;  m_WorldFromHud[1][1] = -hudRight.y;  m_WorldFromHud[1][2] = hudUp.y;
		m_WorldFromHud[2][0] = hudForward.z;  m_WorldFromHud[2][1] = -hudRight.z;  m_WorldFromHud[2][2] = hudUp.z;
		m_WorldFromHud.SetTranslation( m_PlayerViewOrigin );
	}
	else
	{
		// Original behavior - just zero out angles as requested
		if ( bLockPitch )
		{
			HudAngles[PITCH] = 0.0f;
		}
		if ( ( iVrHudAxisLockToWorld & (1<<YAW) ) != 0 )
		{
			// Locking the yaw to the world is not particularly helpful, so what it actually means is lock it to the weapon.
			QAngle aimAngles;
			MatrixAngles( m_WorldFromWeapon.As3x4(), aimAngles );
			HudAngles[YAW] = aimAngles[YAW];
		}
		m_WorldFromHud.SetupMatrixOrgAngles( m_PlayerViewOrigin, HudAngles );
	}

	// Remember in source X forwards, Y left, Z up.
	// We need to transform to a more conventional X right, Y up, Z backwards before doing the projection.
	VMatrix WorldFromHudView;
	WorldFromHudView./*X vector*/SetForward ( -m_WorldFromHud.GetLeft() );
	WorldFromHudView./*Y vector*/SetLeft    ( m_WorldFromHud.GetUp() );
	WorldFromHudView./*Z vector*/SetUp      ( -m_WorldFromHud.GetForward() );
	WorldFromHudView.SetTranslation         ( m_PlayerViewOrigin );

	VMatrix HudProjection;
	HudProjection.Identity();
	HudProjection.m[0][0] = fHudForward / m_fHudHalfWidth;
	HudProjection.m[1][1] = fHudForward / m_fHudHalfHeight;
	// Z vector is not used/valid, but w is for projection.
	HudProjection.m[3][2] = -1.0f;

	// This will transform a world point into a homogeneous vector that
	//  when projected (i.e. divide by w) maps to HUD space [-1,1]
	m_HudProjectionFromWorld = HudProjection * WorldFromHudView.InverseTR();

	return true;
}


// --------------------------------------------------------------------
// Purpose: Updates player orientation, position and motion according
//			to HMD status.
// --------------------------------------------------------------------
bool CClientVirtualReality::OverridePlayerMotion( float flInputSampleFrametime, const QAngle &oldAngles, const QAngle &curAngles, const Vector &curMotion, QAngle *pNewAngles, Vector *pNewMotion )
{
	Assert ( pNewAngles != NULL );
	Assert ( pNewMotion != NULL );
	*pNewAngles = curAngles;
	*pNewMotion = curMotion;

	if ( !UseVR() )
	{
		return false;
	}


	m_bMotionUpdated = true;

	// originalAngles tells us what the weapon angles were before whatever mouse, joystick, etc thing changed them - called "old"
	// curAngles holds the new weapon angles after mouse, joystick, etc. applied.
	// We need to compute what weapon angles WE want and return them in *pNewAngles - called "new"


	VMatrix worldFromTorso;

	CBasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    // Whatever position is already here (set up by OverrideView) needs to be preserved.
    Vector vWeaponOrigin = m_WorldFromWeapon.GetTranslation();

	/*
    if (!m_bRunYet && pPlayer)
    {
        g_pOpenXRManager->rotationOffset = 0.f;
        QAngle hmdrot;
        MatrixToAngles(g_pOpenXRManager->GetMideyePose(), hmdrot);
        g_pOpenXRManager->rotationOffset = -hmdrot.y + pPlayer->GetAbsAngles().y;
        DevMsg("player: %f, head: %f, torso: %f\n", pPlayer->GetAbsAngles().y, hmdrot.y, m_PlayerTorsoAngle.y);
        savedPlayerViewOrigin = Vector(0.f, 0.f, 0.f);
        m_bRunYet = true;
    }

    if (!isnan<float>(curAngles.y)) 
    {
        g_pOpenXRManager->rotationOffset += curAngles.y - oldAngles.y;
    }

    g_pOpenXRManager->rotationOffset += m_PlayerTorsoAngle.y;
	*/

    if ( pPlayer )
    {
        // VR FIX: Calculate smoothed position directly here since OverridePlayerMotion
        // runs BEFORE CalcView (during input processing, not view rendering)
        // We need to apply the same smoothing that CalcView will apply
        
        // Get base eye position
        Vector eyeOrigin = pPlayer->EyePosition();
        QAngle eyeAngles = pPlayer->EyeAngles();
        
        // Apply prediction error smoothing (same as CalcView does)
        C_BasePlayer *pBasePlayer = dynamic_cast<C_BasePlayer*>(pPlayer);
        if (pBasePlayer)
        {
            Vector smoothingOffset;
            pBasePlayer->GetPredictionErrorSmoothingVector(smoothingOffset);
            eyeOrigin += smoothingOffset;
        }
        
        m_PlayerTorsoOrigin = eyeOrigin;
        m_PlayerTorsoAngle = eyeAngles;
        m_PlayerTorsoAngle[PITCH] = 0.0f;  // Don't tilt the body up/down
        m_PlayerTorsoAngle[ROLL] = 0.0f;   // Don't roll the body
		
		// Apply the VR torso transform to the local player model
		OverrideTorsoTransform( m_PlayerTorsoOrigin, m_PlayerTorsoAngle );
    }
    else
    {
        m_PlayerTorsoAngle = QAngle(0, 0, 0);
		m_PlayerTorsoOrigin = Vector(0, 0, 0);
    }
    
    // Update worldFromTorso with smoothed data
    worldFromTorso.SetupMatrixOrgAngles(m_PlayerTorsoOrigin, m_PlayerTorsoAngle);

    // NOTE: Don't update m_WorldFromMidEye here - it will be updated later by
    // UpdateWorldFromMidEyeMatrices() with the FINAL smoothed data from CalcView
    // which includes both stair smoothing AND prediction error smoothing
    m_TorsoFromMideye.Identity();
    // m_WorldFromMidEye = worldFromTorso;  // REMOVED - causes desync with smoothing

    // Weapon view = mideye view, so apply that to the torso to find the world view direction.
    // Set up m_WorldFromWeapon with correct VR position and orientation
    if ( pPlayer )
    {
        // Check if we should use controller roll for crosshair rotation
        extern ConVar tfvr_crosshair_follow_controller_roll;
        if (g_pOpenXRManager && g_pOpenXRManager->IsActive() && tfvr_crosshair_follow_controller_roll.GetBool())
        {
            // Get the controller pose for weapon orientation
            VMatrix rightControllerPose;
            if (g_pOpenXRManager->GetRightControllerPose(rightControllerPose))
            {
                // Extract controller angles including roll
                QAngle controllerAngles;
                MatrixAngles(rightControllerPose.As3x4(), controllerAngles);
                
                // Use controller angles for weapon matrix (this affects crosshair rotation)
                m_WorldFromWeapon.SetupMatrixOrgAngles( m_PlayerTorsoOrigin, controllerAngles );
            }
            else
            {
                // Fallback to eye angles if controller not available
                m_WorldFromWeapon.SetupMatrixOrgAngles( m_PlayerTorsoOrigin, pPlayer->EyeAngles() );
            }
        }
        else
        {
            // Original behavior: use eye angles (no controller roll)
            m_WorldFromWeapon.SetupMatrixOrgAngles( m_PlayerTorsoOrigin, pPlayer->EyeAngles() );
        }
    }
    else
    {
        m_WorldFromWeapon.SetupMatrixOrgAngles( Vector(0,0,0), QAngle(0,0,0) );
    }

    //Override rotation, disabled as it's done later per the weapon
    //g_pVRManager->OverrideWeaponMatrix(m_WorldFromWeapon); 

    //MatrixAngles(m_WorldFromWeapon.As3x4(), *pNewAngles);
	// *pNewAngles = pPlayer->EyeAngles();
	// *pNewAngles = QAngle(0, curAngles.y, 0);

    // Figure out player motion. It says weapon, but it's the HMD
    VMatrix mideyeFromWorld = m_WorldFromMidEye.InverseTR();
    VMatrix newMidEyeFromWeapon = mideyeFromWorld * m_WorldFromWeapon;
    newMidEyeFromWeapon.SetTranslation(Vector(0.0f, 0.0f, 0.0f));;
    //*pNewMotion = newMidEyeFromWeapon * curMotion;

	m_WorldFromMidEyeNoDebugCam = m_WorldFromMidEye;

	// Whatever position is already here (set up by OverrideView) needs to be preserved.
	/*
	Vector vWeaponOrigin = m_WorldFromWeapon.GetTranslation();

	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTFACE_MOVETORSO:
		{
			// Figure out what changes were made to the WEAPON by mouse/joystick/etc
			VMatrix worldFromOldWeapon, worldFromCurWeapon;
			worldFromOldWeapon.SetupMatrixAngles( oldAngles );
			worldFromCurWeapon.SetupMatrixAngles( curAngles );

			// We ignore mouse pitch, the mouse can't do rolls, so it's just yaw changes.
			if( !m_bOverrideTorsoAngle )
			{
				m_PlayerTorsoAngle[YAW] += curAngles[YAW] - oldAngles[YAW];
				m_PlayerTorsoAngle[ROLL] = 0.0f;
				m_PlayerTorsoAngle[PITCH] = 0.0f;
			}

			worldFromTorso.SetupMatrixAngles( m_PlayerTorsoAngle );

			// Weapon view = mideye view, so apply that to the torso to find the world view direction.
			m_WorldFromWeapon = worldFromTorso * m_TorsoFromMideye;

			// ...and we return this new weapon direction as the player's orientation.
			MatrixAngles( m_WorldFromWeapon.As3x4(), *pNewAngles );

			// Restore the translation.
			m_WorldFromWeapon.SetTranslation ( vWeaponOrigin );
		}
		break;
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
		{
			// The mouse just controls the weapon directly.
			*pNewAngles = curAngles;
			*pNewMotion = curMotion;

			// Move the torso by the yaw angles - torso should not have roll or pitch or you'll make folks ill.
			if( !m_bOverrideTorsoAngle && m_hmmMovementActual != HMM_SHOOTMOVELOOKMOUSEFACE )
			{
				m_PlayerTorsoAngle[YAW] = curAngles[YAW];
				m_PlayerTorsoAngle[ROLL] = 0.0f;
				m_PlayerTorsoAngle[PITCH] = 0.0f;
			}

			// Let every other system know.
			m_WorldFromWeapon.SetupMatrixOrgAngles( vWeaponOrigin, *pNewAngles );
			worldFromTorso.SetupMatrixAngles( m_PlayerTorsoAngle );
		}
		break;
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
		{
			// The mouse controls the weapon directly.
			*pNewAngles = curAngles;
			*pNewMotion = curMotion;

			float fReticleYawLimit = vr_moveaim_reticle_yaw_limit.GetFloat();
			float fReticlePitchLimit = vr_moveaim_reticle_pitch_limit.GetFloat();

			if ( CurrentlyZoomed() )
			{
				fReticleYawLimit = vr_moveaim_reticle_yaw_limit_zoom.GetFloat() * m_WorldZoomScale;
				fReticlePitchLimit = vr_moveaim_reticle_pitch_limit_zoom.GetFloat() * m_WorldZoomScale;
				if ( fReticleYawLimit > 180.0f )
				{
					fReticleYawLimit = 180.0f;
				}
				if ( fReticlePitchLimit > 180.0f )
				{
					fReticlePitchLimit = 180.0f;
				}
			}

			if ( fReticlePitchLimit >= 0.0f )
			{
				// Clamp pitch to within the limits.
				(*pNewAngles)[PITCH] = Clamp ( curAngles[PITCH], m_PlayerViewAngle[PITCH] - fReticlePitchLimit, m_PlayerViewAngle[PITCH] + fReticlePitchLimit );
			}

			// For yaw the concept here is the torso stays within a set number of degrees of the weapon in yaw.
			// However, with drifty tracking systems (e.g. IMUs) the concept of "torso" is hazy.
			// Really it's just a mechanism to turn the view without moving the head - its absolute
			// orientation is not that useful.
			// So... if the mouse is to the right greater than the chosen angle from the view, and then
			// moves more right, it will drag the torso (and thus the view) right, so it stays on the edge of the view.
			// But if it moves left towards the view, it does no dragging.
			// Note that if the mouse does not move, but the view moves, it will NOT drag at all.
			// This allows people to mouse-aim within their view, but also to flick-turn with the mouse,
			// and to flick-glance with the head.
			if ( fReticleYawLimit >= 0.0f )
			{
				float fViewToWeaponYaw = AngleDiff ( curAngles[YAW], m_PlayerViewAngle[YAW] );
				float fWeaponYawMovement = AngleDiff ( curAngles[YAW], oldAngles[YAW] );
				if ( fViewToWeaponYaw > fReticleYawLimit )
				{
					if ( fWeaponYawMovement > 0.0f )
					{
						m_PlayerTorsoAngle[YAW] += fWeaponYawMovement;
					}
				}
				else if ( fViewToWeaponYaw < -fReticleYawLimit )
				{
					if ( fWeaponYawMovement < 0.0f )
					{
						m_PlayerTorsoAngle[YAW] += fWeaponYawMovement;
					}
				}
			}

			// Let every other system know.
			m_WorldFromWeapon.SetupMatrixOrgAngles( vWeaponOrigin, *pNewAngles );
			worldFromTorso.SetupMatrixAngles( m_PlayerTorsoAngle );
		}
		break;
	case HMM_SHOOTMOUSE_MOVEFACE:
		{
			(*pNewAngles)[PITCH] = clamp( (*pNewAngles)[PITCH], m_PlayerViewAngle[PITCH]-15.f, m_PlayerViewAngle[PITCH]+15.f );

			float fDiff = AngleDiff( (*pNewAngles)[YAW], m_PlayerViewAngle[YAW] );

			if( fDiff > 15.f )
			{
				(*pNewAngles)[YAW] = AngleNormalize( m_PlayerViewAngle[YAW] + 15.f );
				if( !m_bOverrideTorsoAngle )
					m_PlayerTorsoAngle[ YAW ] += fDiff - 15.f;
			}
			else if( fDiff < -15.f )
			{
				(*pNewAngles)[YAW] = AngleNormalize( m_PlayerViewAngle[YAW] - 15.f );
				if( !m_bOverrideTorsoAngle )
					m_PlayerTorsoAngle[ YAW ] += fDiff + 15.f;
			}
			else
			{
				m_PlayerTorsoAngle[ YAW ] += AngleDiff( curAngles[YAW], oldAngles[YAW] ) /2.f;
			}

			m_WorldFromWeapon.SetupMatrixOrgAngles( vWeaponOrigin, *pNewAngles );
			worldFromTorso.SetupMatrixAngles( m_PlayerTorsoAngle );
		}
		break;
	default: Assert ( false ); break;
	}

	// Figure out player motion.
	switch ( m_hmmMovementActual )
	{
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE:
		{
			// The motion passed in is meant to be relative to the face, so jimmy it to be relative to the new weapon aim.
			VMatrix mideyeFromWorld = m_WorldFromMidEye.InverseTR();
			VMatrix newMidEyeFromWeapon = mideyeFromWorld * m_WorldFromWeapon;
			newMidEyeFromWeapon.SetTranslation ( Vector ( 0.0f, 0.0f, 0.0f ) );
			*pNewMotion = newMidEyeFromWeapon * curMotion;
		}
		break;
	case HMM_SHOOTFACE_MOVETORSO:
		{
			// The motion passed in is meant to be relative to the torso, so jimmy it to be relative to the new weapon aim.
			VMatrix torsoFromWorld = worldFromTorso.InverseTR();
			VMatrix newTorsoFromWeapon = torsoFromWorld * m_WorldFromWeapon;
			newTorsoFromWeapon.SetTranslation ( Vector ( 0.0f, 0.0f, 0.0f ) );
			*pNewMotion = newTorsoFromWeapon * curMotion;
		}
		break;
	case HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE:
	case HMM_SHOOTMOVELOOKMOUSEFACE:
	case HMM_SHOOTFACE_MOVEFACE:
	case HMM_SHOOTMOUSE_MOVEFACE:
	case HMM_SHOOTMOVEMOUSE_LOOKFACE:
	case HMM_SHOOTMOVELOOKMOUSE:
		// Motion is meant to be relative to the weapon, so we're fine.
		*pNewMotion = curMotion;
		break;
	default: Assert ( false ); break;
	}

	// If the game told us to, recenter the torso yaw to match the weapon
	if ( m_iAlignTorsoAndViewToWeaponCountdown > 0 )
	{
		m_iAlignTorsoAndViewToWeaponCountdown--;

		// figure out the angles from the torso to the head
		QAngle torsoFromHeadAngles;
		MatrixAngles( m_TorsoFromMideye.As3x4(), torsoFromHeadAngles );

		QAngle weaponAngles;
		MatrixAngles( m_WorldFromWeapon.As3x4(), weaponAngles );
		m_PlayerTorsoAngle[ YAW ] = weaponAngles[ YAW ] - torsoFromHeadAngles[ YAW ] ;
		NormalizeAngles( m_PlayerTorsoAngle );
	}
	*/

	// remember the motion for stat tracking
	m_PlayerLastMovement = *pNewMotion;

	return true;
}

// --------------------------------------------------------------------
// Purpose: Returns true if the world is zoomed
// --------------------------------------------------------------------
bool CClientVirtualReality::CurrentlyZoomed()
{
	return ( m_WorldZoomScale != 1.0f );
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to keep the torso angle of the player
//			fixed at this point until the game tells us something 
//			different.
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideTorsoTransform( const Vector & position, const QAngle & angles )
{
	if( m_iAlignTorsoAndViewToWeaponCountdown > 0 )
	{
		m_iAlignTorsoAndViewToWeaponCountdown--;

		// figure out the angles from the torso to the head
		QAngle torsoFromHeadAngles;
		MatrixAngles( m_TorsoFromMideye.As3x4(), torsoFromHeadAngles );

		// this is how far off the torso we actually set will need to be to keep the current "forward"
		// vector while the torso angle is being overridden.
		m_OverrideTorsoOffset[ YAW ] = -torsoFromHeadAngles[ YAW ];
	}

	m_bOverrideTorsoAngle = true;
	m_OverrideTorsoAngle = angles + m_OverrideTorsoOffset;

	// overriding pitch and roll isn't allowed to avoid making people sick
	m_OverrideTorsoAngle[ PITCH ] = 0;
	m_OverrideTorsoAngle[ ROLL ] = 0;

	NormalizeAngles( m_OverrideTorsoAngle );
	
	m_PlayerTorsoAngle = m_OverrideTorsoAngle;
}


// --------------------------------------------------------------------
// Purpose: Tells the headtracker to resume using its own notion of 
//			where the torso is pointed.
// --------------------------------------------------------------------
void CClientVirtualReality::CancelTorsoTransformOverride()
{
	m_bOverrideTorsoAngle = false;
}


bool CClientVirtualReality::CanOverlayHudQuad()
{
	return false;
}


// --------------------------------------------------------------------
// Purpose: Returns the bounds in world space where the game should 
//			position the HUD.
// --------------------------------------------------------------------
void CClientVirtualReality::GetHUDBounds( Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR )
{
	// If custom HUD bounds are set, use those instead of the dynamic head-based bounds
	if ( m_bCustomHUDBoundsSet )
	{
		*pViewer = m_CustomHUDViewer;
		*pUL = m_CustomHUDUL;
		*pUR = m_CustomHUDUR;
		*pLL = m_CustomHUDLL;
		*pLR = m_CustomHUDLR;
		return;
	}

	Vector vHalfWidth = m_WorldFromHud.GetLeft() * -m_fHudHalfWidth;
	Vector vHalfHeight = m_WorldFromHud.GetUp() * m_fHudHalfHeight;
	Vector vHUDOrigin = m_PlayerViewOrigin + m_WorldFromHud.GetForward() * vr_hud_forward.GetFloat();

	*pViewer = m_PlayerViewOrigin;
	*pUL = vHUDOrigin - vHalfWidth + vHalfHeight;
	*pUR = vHUDOrigin + vHalfWidth + vHalfHeight;
	*pLL = vHUDOrigin - vHalfWidth - vHalfHeight;
	*pLR = vHUDOrigin + vHalfWidth - vHalfHeight;
	
	// Only capture HUD position during gameplay (when compositor is NOT active)
	// This preserves the last known gameplay position for seamless transitions
	extern bool dxvkIsCompositorActive();
	if ( !dxvkIsCompositorActive() )
	{
		// Notify compositor about dynamic HUD position (for gameplay)
		NotifyCompositorHUDPosition( *pViewer, *pUL, *pUR, *pLL, *pLR, false );
	}
}

// --------------------------------------------------------------------
void CClientVirtualReality::SetCustomHUDBounds( const Vector& viewer, const Vector& ul, const Vector& ur, const Vector& ll, const Vector& lr )
{
	m_bCustomHUDBoundsSet = true;
	m_CustomHUDViewer = viewer;
	m_CustomHUDUL = ul;
	m_CustomHUDUR = ur;
	m_CustomHUDLL = ll;
	m_CustomHUDLR = lr;
}

// --------------------------------------------------------------------
bool CClientVirtualReality::GetCustomHUDBounds( Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR )
{
	if ( !m_bCustomHUDBoundsSet )
		return false;
		
	if ( pViewer )
		*pViewer = m_CustomHUDViewer;
	if ( pUL )
		*pUL = m_CustomHUDUL;
	if ( pUR )
		*pUR = m_CustomHUDUR;
	if ( pLL )
		*pLL = m_CustomHUDLL;
	if ( pLR )
		*pLR = m_CustomHUDLR;
		
	return true;
}

// --------------------------------------------------------------------
void CClientVirtualReality::ClearCustomHUDBounds()
{
	m_bCustomHUDBoundsSet = false;
}

// --------------------------------------------------------------------
// Purpose: Update VR matrices with fresh player data
// Note: Spectator camera smoothing (Mode 2) is applied in view.cpp before this is called
// --------------------------------------------------------------------
void CClientVirtualReality::UpdateWorldFromMidEyeMatrices( const Vector &origin, const QAngle &angles )
{
	// m_WorldFromMidEye: Full head orientation including pitch/roll (for menus)
	m_WorldFromMidEye.SetupMatrixOrgAngles(origin, angles);
	
	// m_WorldFromMidEyeRaw: Same as m_WorldFromMidEye when no smoothing is active
	m_WorldFromMidEyeRaw.SetupMatrixOrgAngles(origin, angles);
	
	// m_WorldFromMidEyeNoDebugCam: Torso angles without pitch/roll tilt (for player body/meathook)
	QAngle torsoAngles = angles;
	torsoAngles[PITCH] = 0.0f;  // Don't tilt the body up/down
	torsoAngles[ROLL] = 0.0f;   // Don't roll the body
	m_WorldFromMidEyeNoDebugCam.SetupMatrixOrgAngles(origin, torsoAngles);
}

// --------------------------------------------------------------------
// Purpose: Update VR matrices with separate smoothed (for view) and raw (for controllers) angles
// Used by spectator camera Mode 2 to keep hands stable while smoothing the view
// --------------------------------------------------------------------
void CClientVirtualReality::UpdateWorldFromMidEyeMatricesWithRaw( const Vector &origin, const QAngle &smoothedAngles, const QAngle &rawAngles )
{
	// m_WorldFromMidEye: Smoothed head orientation (for view rendering and menus)
	m_WorldFromMidEye.SetupMatrixOrgAngles(origin, smoothedAngles);
	
	// m_WorldFromMidEyeRaw: Raw (unsmoothed) head orientation (for controller positioning)
	m_WorldFromMidEyeRaw.SetupMatrixOrgAngles(origin, rawAngles);
	
	// m_WorldFromMidEyeNoDebugCam: Torso angles without pitch/roll tilt (for player body/meathook)
	// Use smoothed angles for consistency with the view
	QAngle torsoAngles = smoothedAngles;
	torsoAngles[PITCH] = 0.0f;
	torsoAngles[ROLL] = 0.0f;
	m_WorldFromMidEyeNoDebugCam.SetupMatrixOrgAngles(origin, torsoAngles);
}

// --------------------------------------------------------------------
// Purpose: Notify the VR compositor about the current HUD quad position
// Full coordinate conversion preserving exact calculated position
// --------------------------------------------------------------------
void CClientVirtualReality::NotifyCompositorHUDPosition( const Vector& viewer, const Vector& ul, const Vector& ur, const Vector& ll, const Vector& lr, bool isCustomBounds )
{
	// Only send updates when VR is active and we have valid data
	if ( !UseVR() )
		return;
	
	// Get the current frame number for tracking
	static int s_frameNumber = 0;
	s_frameNumber++;
	
	// DEBUG: Initialize debug counter
	static int s_debugCallCount = 0;
	s_debugCallCount++;
	
	// COORDINATE CONVERSION: Convert from Source world coordinates to playspace-anchored coordinates
	// This follows the same approach as VR menu manager's playspace anchoring
	
	// Calculate the center of the HUD quad
	Vector hudCenter = (ul + ur + ll + lr) * 0.25f;
	
	// Get the actual playspace origin from the VR system (same as VR menu manager)
	// BUT force it to use canonical orientation instead of current player world yaw
	Vector playspaceOriginWorldPos = Vector(0, 0, 0);
	if (g_pVRMenuManager) {
		// TODO: We need a version of GetPlayspaceOriginWorldPos that doesn't include player world rotation
		// For now, use the current implementation but we'll correct for it below
		playspaceOriginWorldPos = g_pVRMenuManager->GetPlayspaceOriginWorldPos();
	}
	
	// FALLBACK: If playspace origin calculation fails, use current head position as approximation
	if (playspaceOriginWorldPos == Vector(0, 0, 0)) {
		if (C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer()) {
			playspaceOriginWorldPos = pPlayer->EyePosition();
			Msg("VR Client: Using head position fallback for playspace origin\n");
		}
	}
	
	// DEBUG: Check if the issue is player height offset
	Vector currentHeadWorldPos = Vector(0,0,0);
	QAngle currentHeadWorldAngles = QAngle(0,0,0);
	if (C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer()) {
		currentHeadWorldPos = pPlayer->EyePosition();
		currentHeadWorldAngles = pPlayer->EyeAngles();
	}

	// PROPER APPROACH: Derive a complete worldToPlayspace transformation matrix
	// This ensures we encode coordinates correctly in one clean operation
	
	// Get scale factor first
	extern COpenXRManager* g_pOpenXRManager;
	extern ConVar tfvr_worldscale;
	float dynamicWorldScale = (g_pOpenXRManager && g_pOpenXRManager->IsActive()) ? 
							   g_pOpenXRManager->GetWorldScale() : 48.0f;
	
	// Convert Source units to meters
	float scaleToMeters = 1.0f / dynamicWorldScale;
	
	// Step 1: Get the complete playspace transformation matrix from VR manager
	// This gives us the complete worldToPlayspace matrix including orientation
	VMatrix playspaceWorldMatrix;
	playspaceWorldMatrix.Identity();
	
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (g_pVRMenuManager && g_pOpenXRManager && pPlayer) {
		// Use the exact same calculation as CVRMenuManager::CalculateCurrentPlayspaceOriginWorldPos()
		VMatrix headRelativeToPlayspace = g_pOpenXRManager->GetMideyePose();
		
		// Get current head world matrix (reuse existing variables)
		// currentHeadWorldPos and currentHeadWorldAngles already defined above
		
		VMatrix currentHeadWorldMatrix;
		currentHeadWorldMatrix.Identity();
		matrix3x4_t headMatrix3x4;
		
		// For custom bounds (menus), use GetVRViewPosition() to match how menu bounds
		// are calculated. During death, EyePosition() returns dead view height (ground
		// level), but menus are positioned using GetVRViewPosition() which returns the
		// VR death position (standing height). Using mismatched heights causes the
		// compositor menu to appear too high when quitting while dead.
		Vector playerHeadPos;
		if (isCustomBounds)
		{
			C_TFPlayer* pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if (pTFPlayer)
			{
				playerHeadPos = pTFPlayer->GetVRViewPosition();
			}
			else
			{
				playerHeadPos = pPlayer->EyePosition();
			}
		}
		else
		{
			playerHeadPos = pPlayer->EyePosition();
		}
		QAngle playerHeadAngles = pPlayer->EyeAngles();
		AngleMatrix(playerHeadAngles, playerHeadPos, headMatrix3x4);
		currentHeadWorldMatrix.CopyFrom3x4(headMatrix3x4);
		
		// Calculate playspace origin relative to head
		VMatrix headToPlayspaceTransform = headRelativeToPlayspace.InverseTR();
		
		// Get the complete playspace transformation (position + orientation)
		playspaceWorldMatrix = currentHeadWorldMatrix * headToPlayspaceTransform;
	} else {
		// Fallback: Create matrix from just the position
		matrix3x4_t playspaceMatrix3x4;
		AngleMatrix(QAngle(0, 0, 0), playspaceOriginWorldPos, playspaceMatrix3x4);
		playspaceWorldMatrix.CopyFrom3x4(playspaceMatrix3x4);
	}
	
	// Step 2: Get the worldToPlayspace transformation matrix (inverse of playspaceWorldMatrix)
	VMatrix worldToPlayspace = playspaceWorldMatrix.InverseTR();

	// Step 3: Transform HUD center to playspace coordinates (in Source units first)
	Vector hudPositionInPlayspace = worldToPlayspace * hudCenter;
	
	// Step 4: Apply scaling to convert Source units to meters AFTER the transformation
	hudPositionInPlayspace *= scaleToMeters;
	
	// Scale factor already calculated above in worldToPlayspace matrix
	
	// Apply the worldToPlayspace matrix to all four corners, then scale
	Vector hudULInPlayspace = worldToPlayspace * ul;
	Vector hudURInPlayspace = worldToPlayspace * ur;
	Vector hudLLInPlayspace = worldToPlayspace * ll;
	Vector hudLRInPlayspace = worldToPlayspace * lr;
	
	// CACHE: Scale coordinates back to base world scale for menu consistency
	// If we saved coords at worldscale 52 (Heavy), scale them back to base worldscale 48
	extern ConVar tfvr_worldscale;
	float baseWorldScale = tfvr_worldscale.GetFloat(); // Base/standard world scale (usually 48.0)
	float scaleRatio = baseWorldScale / dynamicWorldScale; // e.g., 48/52 = 0.923
	
	Vector cachedUL = hudULInPlayspace * scaleRatio;
	Vector cachedUR = hudURInPlayspace * scaleRatio;
	Vector cachedLL = hudLLInPlayspace * scaleRatio;
	Vector cachedLR = hudLRInPlayspace * scaleRatio;
	
	SetCachedCompositorCoords(cachedUL, cachedUR, cachedLL, cachedLR);
	
	// Apply scaling to convert Source units to meters AFTER the transformation
	hudULInPlayspace *= scaleToMeters;
	hudURInPlayspace *= scaleToMeters;
	hudLLInPlayspace *= scaleToMeters;
	hudLRInPlayspace *= scaleToMeters;
	
	// Coordinates are already scaled to meters by the worldToPlayspace matrix
	Vector playspaceUL = hudULInPlayspace;
	Vector playspaceUR = hudURInPlayspace;
	Vector playspaceLL = hudLLInPlayspace;
	Vector playspaceLR = hudLRInPlayspace;
	Vector playspaceHudCenter = hudPositionInPlayspace;
	
	// Convert from Source coordinate system to OpenGL/Vulkan coordinate system  
	// Source: +X=forward, +Y=left, +Z=up
	// OpenGL/Vulkan: +X=right, +Y=up, +Z=back (negative Z = forward)
	// TEST: Try multiple coordinate system mappings to find the right one
	auto ConvertToVR = [&](const Vector& src) -> Vector {
		Vector vr;
		
		// Source(X=fwd,Y=left,Z=up) -> VR(X=right,Y=up,Z=back)
		vr.x = -src.y;  // Source Y (left) -> -X (right)
		vr.y = -src.z;  // Source Z (up) -> Y (up), flipped for correct orientation
		vr.z = -src.x;  // Source X (forward) -> -Z (back)

		return vr;
	};
	
	// Convert all corners to VR coordinate system
	Vector vrUL = ConvertToVR(playspaceUL);
	Vector vrUR = ConvertToVR(playspaceUR);
	Vector vrLL = ConvertToVR(playspaceLL);
	Vector vrLR = ConvertToVR(playspaceLR);
	Vector vrHudCenter = ConvertToVR(playspaceHudCenter);

	if (vrUL.y < vrLL.y) {
		Vector tempUL = vrUL;
		Vector tempUR = vrUR;
		vrUL = vrLL;
		vrUR = vrLR;
		vrLL = tempUL;
		vrLR = tempUR;
	}
	
	// Calculate the distance for reference
	float hudDistance = vrHudCenter.Length();
	
	// No artificial distance scaling - let the coordinate conversion be natural
	// Focus on getting the Source->VR unit conversion correct
	
	// Fix aspect ratio to proper 16:9 while preserving orientation
	Vector vrHudCenterUpdated = (vrUL + vrUR + vrLL + vrLR) * 0.25f;
	Vector originalWidthVec = vrUR - vrUL;
	Vector originalHeightVec = vrLL - vrUL;
	
	float measuredWidth = originalWidthVec.Length();
	float measuredHeight = originalHeightVec.Length();
	float actualAspectRatio = (measuredHeight > 0.01f) ? (measuredWidth / measuredHeight) : 0.0f;
	
	// Use natural HUD dimensions (aspect ratio correction disabled by default)
	float correctedHeight = measuredHeight;
	
	// Log essential info for first few calls
	if ( s_debugCallCount <= 3 ) {
		DevMsg("VR Client: Positioning HUD #%d - Custom: %s, Distance: %.2fm\n", 
			s_debugCallCount, isCustomBounds ? "true" : "false", hudDistance);
	}
	
	// Send the converted coordinates to the compositor
	extern void TF2VR_UpdateHUDPosition(
		float viewer_x, float viewer_y, float viewer_z,
		float ul_x, float ul_y, float ul_z,
		float ur_x, float ur_y, float ur_z,
		float ll_x, float ll_y, float ll_z,
		float lr_x, float lr_y, float lr_z,
		bool is_custom_bounds, int frame_number, float world_scale);
	
	TF2VR_UpdateHUDPosition(
		0.0f, 0.0f, 0.0f,  // Viewer is now at origin (playspace center)
		vrUL.x, vrUL.y, vrUL.z,  // Upper-left (converted)
		vrUR.x, vrUR.y, vrUR.z,  // Upper-right (converted)
		vrLL.x, vrLL.y, vrLL.z,  // Lower-left (converted)
		vrLR.x, vrLR.y, vrLR.z,  // Lower-right (converted)
		isCustomBounds, s_frameNumber, 1.0f  // world_scale = 1.0 since we already converted
	);
}

// --------------------------------------------------------------------
// Purpose: Generate fallback HUD bounds for startup when no player exists
// Uses raw head pose (already in playspace coordinates) to calculate menu position
// --------------------------------------------------------------------
void GetFallbackStartupHUDBounds( Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR )
{
	// Get the raw head pose (already in playspace coordinates)
	extern COpenXRManager* g_pOpenXRManager;
	if ( !g_pOpenXRManager )
	{
		// Fallback if no VR manager available
		*pViewer = Vector( 0.0f, 0.0f, 64.0f );
		*pUL = *pUR = *pLL = *pLR = *pViewer;
		return;
	}
	
	// Get head pose from VR manager (already in playspace/Source coordinates)
	VMatrix headPose = g_pOpenXRManager->GetMideyePose();
	Vector headPos = headPose.GetTranslation();
	QAngle headAngles;
	MatrixToAngles( headPose, headAngles );
	
	// Remove pitch rotation to keep HUD level (no tilting up/down)
	headAngles.x = 0.0f;  // Zero out pitch
	headAngles.z = 0.0f;  // Zero out roll
	
	// Use head position as viewer position
	*pViewer = headPos;
	
	// Get menu distance from ConVar (same as used by VR menu manager)
	extern ConVar tfvr_menu_distance;
	extern ConVar tfvr_menu_scale;
	float hudDistance = tfvr_menu_distance.GetFloat();
	
	// HUD dimensions: same as VR menu manager uses (with scaling)
	float baseHeight = 80.0f;
	float scale = tfvr_menu_scale.GetFloat();
	float hudHeight = baseHeight * scale;
	float hudWidth = hudHeight * (16.0f / 9.0f); // 16:9 aspect ratio
	
	// Calculate forward direction from leveled head orientation (no pitch)
	Vector forward, right, up;
	AngleVectors( headAngles, &forward, &right, &up );
	
	// Position menu at the specified distance in front of head
	Vector hudCenter = headPos + forward * hudDistance;
	
	// Calculate corner positions using the head orientation
	*pUL = hudCenter + right * (-hudWidth * 0.5f) + up * (hudHeight * 0.5f);
	*pUR = hudCenter + right * (hudWidth * 0.5f) + up * (hudHeight * 0.5f);
	*pLL = hudCenter + right * (-hudWidth * 0.5f) + up * (-hudHeight * 0.5f);
	*pLR = hudCenter + right * (hudWidth * 0.5f) + up * (-hudHeight * 0.5f);
	
	DevMsg( "VR Client: Generated startup fallback HUD bounds using raw head pose\n" );
}

// --------------------------------------------------------------------
// Purpose: Update compositor HUD position when playspace anchor changes
// This is called from VR menu manager during playspace updates
// --------------------------------------------------------------------
void NotifyCompositorPlayspaceUpdate()
{
	// Only send updates when VR is active
	if ( !UseVR() )  // Fixed: should be NOT UseVR()
		return;
	
	// Check if we have custom menu bounds set - use those instead of HUD bounds
	Vector viewer, ul, ur, ll, lr;
	bool hasCustomBounds = false;
	
	if ( g_ClientVirtualReality.GetCustomHUDBounds( &viewer, &ul, &ur, &ll, &lr ) )
	{
		// Use the custom menu bounds (close, comfortable for VR)
		hasCustomBounds = true;
		// DevMsg( "VR Client: Using custom menu bounds for compositor update\n" );
	}
	else
	{
		// Check if we have a valid player to get HUD bounds from
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && pPlayer->IsAlive() )
		{
			// Player exists - use normal HUD bounds
			g_ClientVirtualReality.GetHUDBounds( &viewer, &ul, &ur, &ll, &lr );
			// DevMsg( "VR Client: Using player-based HUD bounds for compositor update\n" );
		}
		else
		{
			// No player available (startup) - use fallback bounds as if player at origin
			GetFallbackStartupHUDBounds( &viewer, &ul, &ur, &ll, &lr );
			hasCustomBounds = true;  // Treat as custom bounds since it's not player-based
			// DevMsg( "VR Client: Using startup fallback HUD bounds (no player available)\n" );
		}
	}
	
	// Send the position to the compositor
	g_ClientVirtualReality.NotifyCompositorHUDPosition(viewer, ul, ur, ll, lr, hasCustomBounds);
}

// --------------------------------------------------------------------
// Purpose: Ensure compositor gets HUD position when it starts
// This is called from DXVK side when compositor initializes
// --------------------------------------------------------------------
extern "C" void TF2VR_RefreshCompositorHUDPosition()
{
	// This is called from the compositor when it starts to get current HUD position
	NotifyCompositorPlayspaceUpdate();
}

// --------------------------------------------------------------------
// Purpose: Cache the final coordinates sent to compositor for cursor collision
// --------------------------------------------------------------------
void CClientVirtualReality::SetCachedCompositorCoords( const Vector& ul, const Vector& ur, const Vector& ll, const Vector& lr )
{
	m_bHasCachedCompositorCoords = true;
	m_CachedCompositorUL = ul;
	m_CachedCompositorUR = ur;
	m_CachedCompositorLL = ll;
	m_CachedCompositorLR = lr;
}

// --------------------------------------------------------------------
void CClientVirtualReality::GetCachedCompositorCoords( Vector& ul, Vector& ur, Vector& ll, Vector& lr ) const
{
	ul = m_CachedCompositorUL;
	ur = m_CachedCompositorUR;
	ll = m_CachedCompositorLL;
	lr = m_CachedCompositorLR;
}


// --------------------------------------------------------------------
// Purpose: Renders the HUD in the world.
// --------------------------------------------------------------------
void CClientVirtualReality::RenderHUDQuad( bool bBlackout )
{
	VPROF("VR_ClientVR_RenderHUDQuad");
	
	// If we can overlay the HUD directly onto the target later, we'll do that instead (higher image quality).
	if ( CanOverlayHudQuad() )
	{
		static int s_overlayCount = 0;
		s_overlayCount++;
		if ( s_overlayCount % 120 == 0 )
		{
			DevMsg("RenderHUDQuad: Using overlay path instead of quad rendering\n");
		}
		return;
	}

	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds ( &vHead, &vUL, &vUR, &vLL, &vLR );

	CMatRenderContextPtr pRenderContext( materials );

	{
		IMaterial *mymat = NULL;
		
		// Determine material selection based on HUD type and menu state
		bool bUseTranslucent = false;
		
		// Declare variables outside scope so they can be used in debug output
		bool bIsMainMenu = enginevgui && enginevgui->IsGameUIVisible();
		bool bIsEconUIVisible = false;
		bool bIsConnectedToServer = engine && engine->IsConnected();
		bool bIsCursorVisible = vgui::surface() && vgui::surface()->IsCursorVisible();
		bool bIsLoadoutOrArmoryScreen = false;
		
		// Check if normal gameplay HUD is visible (health, ammo, etc.)
		bool bIsNormalHUDVisible = false;
		bool bIsDeadPlayerInGame = false;
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer && engine->IsInGame())
		{
			// Check if HUD elements are not hidden
			int iHideHud = pPlayer->m_Local.m_iHideHUD;
			extern ConVar hidehud;
			if (hidehud.GetInt())
			{
				iHideHud = hidehud.GetInt();
			}
			
			// HUD is visible if not all hidden and not in VGui input mode
			bool bHUDNotHidden = !(iHideHud & HIDEHUD_ALL) && !pPlayer->IsInVGuiInputMode() && !bIsMainMenu;
			
			if (pPlayer->IsAlive())
			{
				// Living player with normal HUD
				bIsNormalHUDVisible = bHUDNotHidden;
			}
			else 
			{
				// Dead player - check if they're spectating or in death cam (should still use translucent)
				bIsDeadPlayerInGame = bHUDNotHidden;
			}
		}

		if ( !m_bCustomHUDBoundsSet )
		{
			// HUD is attached to face - use translucent for normal HUD or dead player
			bUseTranslucent = bIsNormalHUDVisible || bIsDeadPlayerInGame;
		}
		else
		{
			// HUD is positioned in world space (menus)
			// Check if any EconUI panels are visible (loadout, backpack, crafting, etc.)
			if ( EconUI() )
			{
				bIsEconUIVisible = EconUI()->IsUIPanelVisible( ECONUI_BACKPACK ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_LOADOUT ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_CRAFTING ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_ARMORY ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_TRADING );
			}
		
		// Additional checks for loadout/armory screens that EconUI might miss
		if (bIsConnectedToServer)
		{
			// Check for class menu state via console variables
			ConVar* pClassMenuOpen = g_pCVar->FindVar("_cl_classmenuopen");
			if (pClassMenuOpen && pClassMenuOpen->GetBool())
			{
				bIsLoadoutOrArmoryScreen = true;
			}
			
			// Check for class loadout panel specifically
			if (g_pClassLoadoutPanel && g_pClassLoadoutPanel->IsVisible())
			{
				bIsLoadoutOrArmoryScreen = true;
			}
			
			// Check for character info panel (class selection screen) 
			CCharacterInfoPanel* pCharInfoPanel = GetCharInfoPanel(false);
			if (pCharInfoPanel && pCharInfoPanel->IsVisible())
			{
				bIsLoadoutOrArmoryScreen = true;
			}
		}
		
		// Material selection logic with proper priority (same as viewrender.cpp and vr_menu_manager.cpp):
		// 1. True main menu (not connected) = opaque
		// 2. Overlay menus (class select, loadout, inventory, etc.) = opaque  
		// 3. In-game pause menu = translucent
		// 4. Normal gameplay HUD (health, ammo, etc.) = translucent
		// 5. Dead player in-game (spectating, death cam) = translucent
		// 6. Default = opaque
		if (!bIsConnectedToServer)
		{
			// True main menu (not connected) - use opaque
			bUseTranslucent = false;
		}
		else if (bIsEconUIVisible || bIsLoadoutOrArmoryScreen)
		{
			// Overlay menus (class select, loadout, inventory, etc.) - use opaque
			bUseTranslucent = false;
		}
		else if (bIsMainMenu)
		{
			// In-game pause menu - use translucent
			bUseTranslucent = true;
		}
		else if (bIsNormalHUDVisible || bIsDeadPlayerInGame)
		{
			// Normal gameplay HUD with health/ammo OR dead player in-game - use translucent
			bUseTranslucent = true;
		}
		else
		{
			// Default: opaque for unknown states
			bUseTranslucent = false;
		}
		}
		
		// Select the appropriate material
		if ( bUseTranslucent )
		{
			mymat = materials->FindMaterial( "vgui/inworldui", TEXTURE_GROUP_VGUI );
		}
		else
		{
			mymat = materials->FindMaterial( "vgui/inworldui_opaque", TEXTURE_GROUP_VGUI );
		}
		
		// Debug output to verify material selection - always print for now
		static int s_debugFrameCount = 0;
		s_debugFrameCount++;
		
		// Print debug info every 60 frames (about once per second at 60fps)
		if ( s_debugFrameCount % 60 == 0 )
		{
			bool bIsMainMenu = enginevgui && enginevgui->IsGameUIVisible();
			bool bIsEconUIVisible = false;
			
			// Check if any EconUI panels are visible (same logic as above)
			if ( EconUI() )
			{
				bIsEconUIVisible = EconUI()->IsUIPanelVisible( ECONUI_BACKPACK ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_LOADOUT ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_CRAFTING ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_ARMORY ) ||
								   EconUI()->IsUIPanelVisible( ECONUI_TRADING );
					}
		
		// Log material changes for debugging
		static bool s_bLastUseTranslucent = true;
		if ( s_bLastUseTranslucent != bUseTranslucent )
		{
			s_bLastUseTranslucent = bUseTranslucent;
			DevMsg("VR HUD: Switching to %s rendering\n", bUseTranslucent ? "TRANSLUCENT" : "OPAQUE");
		}
	}
		Assert( mymat && !mymat->IsErrorMaterial() );

		if (!mymat->IsPrecached()) {
			PrecacheMaterial(mymat->GetName());
			mymat->IncrementReferenceCount();
		}
		
		// Force render state for opaque materials to ensure no transparency
		if ( !bUseTranslucent )
		{
			// For opaque materials, force full opacity and disable blending
			float color[3] = { 1.0f, 1.0f, 1.0f };
			render->SetColorModulation( color );
			render->SetBlend( 1.0f );
		}

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, mymat );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		meshBuilder.Position3fv (vLR.Base() );
		meshBuilder.TexCoord2f( 0, 1, 1 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vLL.Base());
		meshBuilder.TexCoord2f( 0, 0, 1 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vUR.Base());
		meshBuilder.TexCoord2f( 0, 1, 0 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.Position3fv (vUL.Base());
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 1>();

		meshBuilder.End();
		pMesh->Draw();
	}

	if( false && bBlackout )
	{
		Vector vbUL, vbUR, vbLL, vbLR;
		// "Reflect" the HUD bounds through the viewer to find the ones behind the head.
		vbUL = 2 * vHead - vLR;
		vbUR = 2 * vHead - vLL;
		vbLL = 2 * vHead - vUR;
		vbLR = 2 * vHead - vUL;

		IMaterial *mymat = materials->FindMaterial( "vgui/black", TEXTURE_GROUP_VGUI );
		mymat->IncrementReferenceCount();
		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, mymat );

		// Tube around the outside.
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 8 );

		meshBuilder.Position3fv (vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base() );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();

		// Cap behind the viewer.
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, 2 );

		meshBuilder.Position3fv (vbUR.Base() );
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbUL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLR.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.Position3fv (vbLL.Base());
		meshBuilder.AdvanceVertexF<VTX_HAVEPOS, 0>();

		meshBuilder.End();
		pMesh->Draw();
	}
}


// --------------------------------------------------------------------
// Purpose: Gets the amount of zoom to apply
// --------------------------------------------------------------------
float CClientVirtualReality::GetZoomedModeMagnification()
{
	return m_WorldZoomScale * vr_zoom_scope_scale.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Does some client-side tracking work and then tells headtrack
//			to do its own work.
// --------------------------------------------------------------------
bool CClientVirtualReality::ProcessCurrentTrackingState( float fGameFOV )
{
	m_WorldZoomScale = 1.0f;
	if ( fGameFOV != 0.0f )
	{
		// To compensate for the lack of pixels on most HUDs, let's grow this a bit.
		// Remember that MORE zoom equals LESS fov!
		fGameFOV *= ( 1.0f / vr_zoom_multiplier.GetFloat() );
		fGameFOV = Min ( fGameFOV, 170.0f );

		// The game has overridden the FOV, e.g. because of a sniper scope. So we need to match this view with whatever actual FOV the HUD has.
		float wantedGameTanfov = tanf ( DEG2RAD ( fGameFOV * 0.5f ) );
		// OK, so now in stereo mode, we're going to also draw an overlay, but that overlay usually covers more of the screen (because in a good HMD usually our actual FOV is much wider)
		float overlayActualPhysicalTanfov = tanf ( DEG2RAD ( m_fHudHorizontalFov * 0.5f ) );
		// Therefore... (remembering that a zoom > 1.0 means you zoom *out*)
		m_WorldZoomScale = wantedGameTanfov / overlayActualPhysicalTanfov;
	}
	return false;
	// return g_pSourceVR->SampleTrackingState( fGameFOV, 0.f /* seconds to predict */ );
}


// --------------------------------------------------------------------
// Purpose: Returns the projection matrix to use for the HUD
// --------------------------------------------------------------------
const VMatrix &CClientVirtualReality::GetHudProjectionFromWorld()
{
	// This matrix will transform a world-space position into a homogenous HUD-space vector.
	// So if you divide x+y by w, you will get the position on the HUD in [-1,1] space.
	return m_HudProjectionFromWorld;
}


// --------------------------------------------------------------------
// Purpose: Returns the aim vector relative to the torso
// --------------------------------------------------------------------
void CClientVirtualReality::GetTorsoRelativeAim( Vector *pPosition, QAngle *pAngles )
{
	MatrixAngles( m_TorsoFromMideye.As3x4(), *pAngles, *pPosition );
	pAngles->y += vr_aim_yaw_offset.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Returns distance of the HUD in front of the eyes.
// --------------------------------------------------------------------
float CClientVirtualReality::GetHUDDistance()
{
	return tfvr_hud_forward.GetFloat();
}


// --------------------------------------------------------------------
// Purpose: Returns true if the HUD should be rendered into a render 
//			target and then into the world on a quad.
// --------------------------------------------------------------------
bool CClientVirtualReality::ShouldRenderHUDInWorld()
{
	return UseVR();
}


// --------------------------------------------------------------------
// Purpose: Lets headtrack tweak the view model origin and angles to match 
//			aim angles and handle strange viewmode FOV stuff
// --------------------------------------------------------------------
void CClientVirtualReality::OverrideViewModelTransform( Vector & vmorigin, QAngle & vmangles, bool bUseLargeOverride ) 
{
	Vector vForward, vRight, vUp;
	AngleVectors( vmangles, &vForward, &vRight, &vUp );

	float fForward = bUseLargeOverride ? vr_viewmodel_offset_forward_large.GetFloat() : vr_viewmodel_offset_forward.GetFloat();

	vmorigin += vForward * fForward;
	MatrixAngles( m_WorldFromWeapon.As3x4(), vmangles );
}


// --------------------------------------------------------------------
// Purpose: Tells the head tracker to reset the torso position in case
//			we're on a drifty tracker.
// --------------------------------------------------------------------
void CClientVirtualReality::AlignTorsoAndViewToWeapon()
{
	return;
}


// --------------------------------------------------------------------
// Purpose: Lets VR do stuff at the very end of the rendering process
// --------------------------------------------------------------------
void CClientVirtualReality::PostProcessFrame( StereoEye_t eEye )
{
	if( !UseVR() )
		return;

	// Only draw debug overlays for the left eye to avoid duplicates
	if (eEye == STEREO_EYE_LEFT) {
		DrawPlayspaceDebugVisualization();
	}

	// g_pSourceVR->DoDistortionProcessing( eEye == STEREO_EYE_LEFT ? ISourceVirtualReality::VREye_Left : ISourceVirtualReality::VREye_Right );
}


// --------------------------------------------------------------------
// Pastes the HUD directly onto the backbuffer / render target.
// (higher quality than the RenderHUDQuad() path but can't always be used)
// --------------------------------------------------------------------
void CClientVirtualReality::OverlayHUDQuadWithUndistort( const CViewSetup &eyeView, bool bDoUndistort, bool bBlackout, bool bTranslucent )
{
	if ( ! UseVR() )
		return;

	// If we can't overlay the HUD, it will be handled on another path (rendered into the scene with RenderHUDQuad()).
	if ( ! CanOverlayHudQuad() )
		return;

	// Get the position of the HUD quad in world space as used by RenderHUDQuad().  Then convert to a rectangle in normalized
	// device coordinates.

	Vector vHead, vUL, vUR, vLL, vLR;
	GetHUDBounds ( &vHead, &vUL, &vUR, &vLL, &vLR );

	VMatrix worldToView, viewToProjection, worldToProjection, worldToPixels;
	render->GetMatricesForView( eyeView, &worldToView, &viewToProjection, &worldToProjection, &worldToPixels );

	Vector pUL, pUR, pLL, pLR;

	worldToProjection.V3Mul( vUL, pUL );
	worldToProjection.V3Mul( vUR, pUR );
	worldToProjection.V3Mul( vLL, pLL );
	worldToProjection.V3Mul( vLR, pLR );

	float ndcHudBounds[4];
	ndcHudBounds[0] = Min ( Min( pUL.x, pUR.x ), Min( pLL.x, pLR.x ) );
	ndcHudBounds[1] = Min ( Min( pUL.y, pUR.y ), Min( pLL.y, pLR.y ) );
	ndcHudBounds[2] = Max ( Max( pUL.x, pUR.x ), Max( pLL.x, pLR.x ) );
	ndcHudBounds[3] = Max ( Max( pUL.y, pUR.y ), Max( pLL.y, pLR.y ) );

	// ISourceVirtualReality::VREye sourceVrEye = ( eyeView.m_eStereoEye == STEREO_EYE_LEFT ) ? ISourceVirtualReality::VREye_Left : ISourceVirtualReality::VREye_Right;
	// g_pSourceVR->CompositeHud ( sourceVrEye, ndcHudBounds, bDoUndistort, bBlackout, bTranslucent );
}


// --------------------------------------------------------------------
// Purpose: Switches to VR mode
// --------------------------------------------------------------------
void CClientVirtualReality::Activate()
{
	// Try to reactivate existing session first, fallback to full initialization
	if( g_pOpenXRManager->IsActive() )
		return; // Already active
		
	// Try reactivation first (faster if session exists)
	g_pOpenXRManager->Reactivate();
	
	// If reactivation failed and we're still not active, do full initialization
	if( !g_pOpenXRManager->IsActive() && !g_pOpenXRManager->Initialize() )
		return;

	// General all-game stuff
	engine->ExecuteClientCmd("mat_reset_rendertargets\n");

	// Game specific VR config
	engine->ExecuteClientCmd("exec tfvr\n");

    vgui::surface()->SetSoftwareCursor( true );

#if defined(POSIX)
	ConVarRef m_rawinput( "m_rawinput" );
    m_bNonVRRawInput = m_rawinput.GetBool();
    m_rawinput.SetValue( 1 );

	ConVarRef mat_vsync( "mat_vsync" );
	mat_vsync.SetValue( 0 );
#endif

	vgui::ivgui()->SetVRMode(true);
	uint32_t width, height;
	g_pOpenXRManager->GetSpectatorScreenDims(width, height);

	g_pMatSystemSurface->SetFullscreenViewportAndRenderTarget( 0, 0, width, height, NULL );
}


void CClientVirtualReality::Deactivate()
{
	// can't deactivate when we aren't active
	if( !UseVR() )
		return;

	g_pOpenXRManager->Deactivate();
	
	// CRITICAL: Notify server that VR mode is deactivated 
	// This disables server-side VR features like head collision detection
	KeyValues *kvMode = new KeyValues( "VRModeInactive" );
	engine->ServerCmdKeyValues( kvMode );
	DevMsg("VR Deactivate: Sent VRModeInactive to server - disabling head collision detection\n");

    static ConVarRef cl_software_cursor( "cl_software_cursor" );
    vgui::surface()->SetSoftwareCursor( cl_software_cursor.GetBool() );

#if defined( USE_SDL )
    static ConVarRef sdl_displayindex( "sdl_displayindex" );
    sdl_displayindex.SetValue( m_nNonVRSDLDisplayIndex );
#endif

#if defined(POSIX)
    ConVarRef m_rawinput( "m_rawinput" );
    m_rawinput.SetValue( m_bNonVRRawInput );
#endif

	/*
    // Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
	vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, m_nNonVRWidth, m_nNonVRHeight );
	}
	// Same for client .dll tools
	root = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
	if ( root != 0 )
	{
		vgui::ipanel()->SetSize( root, m_nNonVRWidth, m_nNonVRHeight );
	}

	int viewWidth, viewHeight;
	vgui::surface()->GetScreenSize( viewWidth, viewHeight );
	*/

	engine->ExecuteClientCmd( "mat_reset_rendertargets\n" );

	// set mode
	/*
	char szCmd[ 256 ];
	Q_snprintf( szCmd, sizeof( szCmd ), "mat_setvideomode %i %i %i\n", m_nNonVRWidth, m_nNonVRHeight, m_bNonVRWindowed ? 1 : 0 );
	engine->ClientCmd_Unrestricted( szCmd );
	*/
}


// Called when startup is complete
void CClientVirtualReality::StartupComplete()
{
	if ( vr_activate_default.GetBool() || ShouldForceVRActive() )
		Activate();
}

// --------------------------------------------------------------------
// Purpose: Draw debug visualizations for playspace origin and HUD positions
// Called every frame during PostProcessFrame
// --------------------------------------------------------------------
void CClientVirtualReality::DrawPlayspaceDebugVisualization()
{
	if (!debugoverlay || !tfvr_debug_playspace_origin.GetBool())
		return;
		
	// Get current player for head position
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	// Get playspace origin from VR menu manager (same calculation as HUD positioning)
	Vector playspaceOriginWorldPos = Vector(0, 0, 0);
	if (g_pVRMenuManager) {
		playspaceOriginWorldPos = g_pVRMenuManager->GetPlayspaceOriginWorldPos();
	}
	
	// Fallback to head position if playspace calculation fails
	if (playspaceOriginWorldPos == Vector(0, 0, 0)) {
		playspaceOriginWorldPos = pPlayer->EyePosition();
	}
	
	// Draw playspace origin - MAGENTA cube with coordinate axes
	float cubeSize = 5.0f; // 50 Source units ≈ 1 meter
	Vector boxSize(cubeSize, cubeSize, cubeSize);
	debugoverlay->AddBoxOverlay(playspaceOriginWorldPos, -boxSize, boxSize, QAngle(0, 0, 0), 255, 0, 255, 100, 0.1f);
	
	// Draw Source coordinate system axes (no duration = persistent until next frame)
	Vector forward = Vector(100, 0, 0);   // X-axis (forward in Source) = RED
	Vector right = Vector(0, 100, 0);     // Y-axis (left in Source) = GREEN  
	Vector up = Vector(0, 0, 100);        // Z-axis (up in Source) = BLUE
	debugoverlay->AddLineOverlayAlpha(playspaceOriginWorldPos, playspaceOriginWorldPos + forward, 255, 0, 0, 255, false, 0.0f);
	debugoverlay->AddLineOverlayAlpha(playspaceOriginWorldPos, playspaceOriginWorldPos + right, 0, 255, 0, 255, false, 0.0f);
	debugoverlay->AddLineOverlayAlpha(playspaceOriginWorldPos, playspaceOriginWorldPos + up, 0, 0, 255, 255, false, 0.0f);
	
	// Draw current head position - CYAN cube for comparison
	Vector headPos = pPlayer->EyePosition();
	float headCubeSize = 25.0f;
	Vector headBoxSize(headCubeSize, headCubeSize, headCubeSize);
	debugoverlay->AddBoxOverlay(headPos, -headBoxSize, headBoxSize, QAngle(0, 0, 0), 0, 255, 255, 100, 0.0f);
	
	// If we have custom HUD bounds set, draw the HUD center too
	if (m_bCustomHUDBoundsSet) {
		Vector hudCenter = (m_CustomHUDUL + m_CustomHUDUR + m_CustomHUDLL + m_CustomHUDLR) * 0.25f;
		float hudCubeSize = 20.0f;
		Vector hudBoxSize(hudCubeSize, hudCubeSize, hudCubeSize);
		// YELLOW cube for HUD center
		debugoverlay->AddBoxOverlay(hudCenter, -hudBoxSize, hudBoxSize, QAngle(0, 0, 0), 255, 255, 0, 100, 0.0f);
	}
}


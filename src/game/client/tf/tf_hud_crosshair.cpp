//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "clientmode.h"
#include "c_tf_player.h"
#include "tf_hud_crosshair.h"
#include "hud_crosshair.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tf_logic_halloween_2014.h"
#include "tf_gamerules.h"
#include "mathlib/mathlib.h"
#include "tfvr/openxr_manager.h"
#include "client_virtualreality.h"

ConVar cl_crosshair_red( "cl_crosshair_red", "200", FCVAR_ARCHIVE );
ConVar cl_crosshair_green( "cl_crosshair_green", "200", FCVAR_ARCHIVE );
ConVar cl_crosshair_blue( "cl_crosshair_blue", "200", FCVAR_ARCHIVE );

ConVar cl_crosshair_file( "cl_crosshair_file", "default", FCVAR_ARCHIVE );

ConVar cl_crosshair_scale( "cl_crosshair_scale", "32.0", FCVAR_ARCHIVE );

using namespace vgui;

// Everything else is expecting to find "CHudCrosshair"
DECLARE_NAMED_HUDELEMENT( CHudTFCrosshair, CHudCrosshair );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::CHudTFCrosshair( const char *pName ) :
	CHudCrosshair ( pName )
{
	m_szPreviousCrosshair[0] = '\0';
	m_iCrosshairTextureID = -1;
	m_flTimeToHideUntil = -1.f;

	ListenForGameEvent( "restart_timer_time" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::~CHudTFCrosshair( void )
{
	if ( vgui::surface() && m_iCrosshairTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iCrosshairTextureID );
		m_iCrosshairTextureID = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTFCrosshair::ShouldDraw( void )
{
	// turn off for the minigames
	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	// turn off if the local player is a ghost
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		if ( pPlayer->IsTaunting() )
			return false;

		// VR: hide crosshair on throwables when physical throw is active
		extern ConVar tfvr_physical_throw;
		if ( tfvr_physical_throw.GetBool() )
		{
			CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
			if ( pWeapon )
			{
				int wid = pWeapon->GetWeaponID();
				if ( wid == TF_WEAPON_JAR || wid == TF_WEAPON_JAR_MILK ||
					 wid == TF_WEAPON_CLEAVER || wid == TF_WEAPON_JAR_GAS ||
					 wid == TF_WEAPON_THROWABLE )
				{
					return false;
				}
			}
		}

		// VR: hide crosshair on melee weapons (physical melee is always active)
		// Exception: ball-launching bats show the crosshair during ball aim mode
		if ( pPlayer->IsInVRMode() )
		{
			CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
			if ( pWeapon )
			{
				int wtype = pWeapon->GetTFWpnData().m_iWeaponType;
				bool bIsRobotArm = V_stristr( pWeapon->GetClassname(), "robot_arm" ) != NULL;
				if ( wtype == TF_WPN_TYPE_MELEE || wtype == TF_WPN_TYPE_MELEE_ALLCLASS || bIsRobotArm )
				{
					int wid = pWeapon->GetWeaponID();

					extern bool g_bVRBallAimActive;
					if ( g_bVRBallAimActive &&
						 ( wid == TF_WEAPON_BAT_WOOD || wid == TF_WEAPON_BAT_GIFTWRAP ) )
					{
						// Allow crosshair - ball aim is active on offhand
					}
					else if ( wid != TF_WEAPON_BUFF_ITEM && wid != TF_WEAPON_ROCKETPACK )
					{
						return false;
					}
				}
			}
		}
	}

	if ( m_flTimeToHideUntil > gpGlobals->curtime )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::LevelShutdown( void )
{
	m_szPreviousCrosshair[0] = '\0';

	if ( m_pCrosshairMaterial )
	{
		delete m_pCrosshairMaterial;
		m_pCrosshairMaterial = NULL;
	}
	
	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Init()
{
	if ( m_iCrosshairTextureID == -1 )
	{
		m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
	}

	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "restart_timer_time", event->GetName() ) )
	{
		if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		{
			int nTime = event->GetInt( "time" );
			if ( ( nTime <= 10 ) && ( nTime > 0 ) )
			{
				m_flTimeToHideUntil = gpGlobals->curtime + nTime;
				return;
			}
		}
	}

	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Paint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( !pPlayer )
		return;

	const char *crosshairfile = cl_crosshair_file.GetString();
	if ( ( crosshairfile == NULL ) || ( Q_stricmp( m_szPreviousCrosshair, crosshairfile ) != 0 ) )
	{
		char buf[256];
		Q_snprintf( buf, sizeof(buf), "vgui/crosshairs/%s", crosshairfile );

		if ( m_iCrosshairTextureID != -1 )
		{
			vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, buf, true, false );
		}

		if ( m_pCrosshairMaterial )
		{
			delete m_pCrosshairMaterial;
		}

		m_pCrosshairMaterial = vgui::surface()->DrawGetTextureMatInfoFactory( m_iCrosshairTextureID );

		if ( !m_pCrosshairMaterial )
			return;

		// save the name to compare with the cvar in the future
		Q_strncpy( m_szPreviousCrosshair, crosshairfile, sizeof(m_szPreviousCrosshair) );
	}

	if ( m_szPreviousCrosshair[0] == '\0' )
	{
		// Handle VR controller roll for default crosshair (when cl_crosshair_file is empty)
		if ( UseVR() && g_pOpenXRManager && g_pOpenXRManager->IsActive() )
		{
			extern ConVar tfvr_crosshair_follow_controller_roll;
			if ( tfvr_crosshair_follow_controller_roll.GetBool() )
			{
				// Set up VR crosshair roll angle before calling base class
				VMatrix rightControllerPose;
				if (g_pOpenXRManager->GetRightControllerPose(rightControllerPose))
				{
					QAngle controllerAngles;
					MatrixAngles(rightControllerPose.As3x4(), controllerAngles);
					g_ClientVirtualReality.m_flCrosshairRollAngle = controllerAngles.z;
					g_ClientVirtualReality.m_bCrosshairRollValid = true;
				}
				else
				{
					g_ClientVirtualReality.m_bCrosshairRollValid = false;
				}
			}
		}
		
		return BaseClass::Paint();
	}


	// This is somewhat cut'n'paste from CHudCrosshair::Paint(). Would be nice to unify them some more.
	float x, y;
	bool bBehindCamera;
	GetDrawPosition ( &x, &y, &bBehindCamera );

	if( bBehindCamera )
		return;

	float flWeaponScale = 1.f;
	int iTextureW = 32;
	int iTextureH = 32;
	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetWeaponCrosshairScale( flWeaponScale );
	}

	float flPlayerScale = 1.0f;
#ifdef TF_CLIENT_DLL
	Color clr( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), 255 );
	flPlayerScale = cl_crosshair_scale.GetFloat() / 32.0f;  // the player can change the scale in the options/multiplayer tab
#else
	Color clr = m_clrCrosshair;
#endif
	float flWidth = flWeaponScale * flPlayerScale * (float)iTextureW;
	float flHeight = flWeaponScale * flPlayerScale * (float)iTextureH;
	int iWidth = (int)( flWidth + 0.5f );
	int iHeight = (int)( flHeight + 0.5f );
	int iX = (int)( x + 0.5f );
	int iY = (int)( y + 0.5f );

	vgui::ISurface *pSurf = vgui::surface();
	pSurf->DrawSetColor( clr );
	pSurf->DrawSetTexture( m_iCrosshairTextureID );
	
	// Check if we should rotate crosshair with controller roll in VR
	extern ConVar tfvr_crosshair_follow_controller_roll;
	bool bRotateCrosshair = false;
	float flRotationAngle = 0.0f;
	
	if (g_pOpenXRManager && g_pOpenXRManager->IsActive())
	{
		if (tfvr_crosshair_follow_controller_roll.GetBool())
		{
			// Use the stored roll angle from OverrideWeaponHudAimVectors for consistency
			if (g_ClientVirtualReality.m_bCrosshairRollValid)
			{
				flRotationAngle = g_ClientVirtualReality.m_flCrosshairRollAngle;
				bRotateCrosshair = true;
			}
		}
		
		// Compensate for head roll to keep crosshair level when head tilts
		// Since we zero head roll in m_headInPlayerA, we need to get the raw HMD roll
		QAngle rawHmdAngles;
		MatrixAngles(g_pOpenXRManager->GetMideyePose().As3x4(), rawHmdAngles);
		float headRollCompensation = -rawHmdAngles.z; // Negative to counteract the roll
		
		if (bRotateCrosshair)
		{
			// Add head roll compensation to controller roll
			flRotationAngle += headRollCompensation;
		}
		else
		{
			// Use only head roll compensation
			flRotationAngle = headRollCompensation;
			bRotateCrosshair = (fabs(flRotationAngle) > 0.1f);
		}
	}
	
	if (bRotateCrosshair && fabs(flRotationAngle) > 0.1f) // Only rotate if significant angle
	{
		// Draw rotated crosshair using polygon method with proper texture coordinates
		Vertex_t vertices[4];
		
		// Convert rotation angle to radians
		float flRadians = DEG2RAD(flRotationAngle);
		float cosAngle = cos(flRadians);
		float sinAngle = sin(flRadians);
		
		// Calculate rotated corner positions relative to center
		float halfWidth = (float)iWidth;
		float halfHeight = (float)iHeight;
		
		// For custom crosshairs, we typically want to use the full texture (0,0 to 1,1)
		// since custom crosshairs are usually individual files, not texture atlases
		// Top-left vertex (rotated)
		vertices[0].m_Position.x = iX + (-halfWidth * cosAngle - -halfHeight * sinAngle);
		vertices[0].m_Position.y = iY + (-halfWidth * sinAngle + -halfHeight * cosAngle);
		vertices[0].m_TexCoord.x = 0.0f;
		vertices[0].m_TexCoord.y = 0.0f;
		
		// Top-right vertex (rotated)
		vertices[1].m_Position.x = iX + (halfWidth * cosAngle - -halfHeight * sinAngle);
		vertices[1].m_Position.y = iY + (halfWidth * sinAngle + -halfHeight * cosAngle);
		vertices[1].m_TexCoord.x = 1.0f;
		vertices[1].m_TexCoord.y = 0.0f;
		
		// Bottom-right vertex (rotated)
		vertices[2].m_Position.x = iX + (halfWidth * cosAngle - halfHeight * sinAngle);
		vertices[2].m_Position.y = iY + (halfWidth * sinAngle + halfHeight * cosAngle);
		vertices[2].m_TexCoord.x = 1.0f;
		vertices[2].m_TexCoord.y = 1.0f;
		
		// Bottom-left vertex (rotated)
		vertices[3].m_Position.x = iX + (-halfWidth * cosAngle - halfHeight * sinAngle);
		vertices[3].m_Position.y = iY + (-halfWidth * sinAngle + halfHeight * cosAngle);
		vertices[3].m_TexCoord.x = 0.0f;
		vertices[3].m_TexCoord.y = 1.0f;
		
		// Draw as textured polygon
		pSurf->DrawTexturedPolygon( 4, vertices );
	}
	else
	{
		// Draw normal non-rotated crosshair
		pSurf->DrawTexturedRect( iX-iWidth, iY-iHeight, iX+iWidth, iY+iHeight );
	}
	
	pSurf->DrawSetTexture(0);
}

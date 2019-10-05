//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_sniperrifle_classic.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"

// For TFGameRules() and Player resources
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#endif

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0f
#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0f
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50.0f
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150.0f
#define TF_WEAPON_SNIPERRIFLE_RELOAD_TIME		1.5f
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define SNIPER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"
#define SNIPER_DOT_SPRITE_CLEAR		"effects/sniperdot_clear.vmt"

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle_Classic, DT_TFSniperRifle_Classic )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle_Classic, DT_SniperRifleClassicLocalData )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flChargedDamage), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargedDamage) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle_Classic, DT_TFSniperRifle_Classic )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleClassicLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleClassicLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle_Classic )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flUnzoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flRezoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle_classic, CTFSniperRifle_Classic );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle_classic );

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle_Classic::CTFSniperRifle_Classic()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle_Classic::~CTFSniperRifle_Classic()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Precache()
{
	BaseClass::Precache();
	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ResetTimers( void )
{
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Classic::Reload( void )
{
	// We currently don't reload.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Classic::CanHolster( void ) const
{
 	CTFPlayer *pPlayer = GetTFPlayerOwner();
 	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() ) && ( m_flNextPrimaryAttack > gpGlobals->curtime ) )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Classic::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}

	m_flChargedDamage = 0.0f;
	ResetTimers();

	return BaseClass::Holster( pSwitchingTo );
}

void CTFSniperRifle_Classic::WeaponReset( void )
{
	BaseClass::WeaponReset();

	ZoomOut();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) || pPlayer->m_Shared.InCond( TF_COND_STUNNED ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
	}

	if ( m_flUnzoomTime > 0 && gpGlobals->curtime > m_flUnzoomTime )
	{
		if ( m_bRezoomAfterShot )
		{
			ZoomOutIn();
			m_bRezoomAfterShot = false;
		}
		else
		{
			ZoomOut();
		}

		m_flUnzoomTime = -1;
	}

	if ( m_flRezoomTime > 0 )
	{
		if ( gpGlobals->curtime > m_flRezoomTime )
		{
            ZoomIn();
			m_flRezoomTime = -1;
		}
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		// If we're in the process of rezooming, just cancel it
		if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
		{
			// Prevent them from rezooming in less time than they would have
			m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
			m_flRezoomTime = -1;
		}
		else
		{
			Zoom();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ItemPostFrame( void )
{
	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;
	
	CheckReload();

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot )
	{
		UpdateSniperDot();
	}
#endif

	// Start charging when we're zoomed in, and allowed to fire
	if ( pPlayer->m_Shared.IsJumping() )
	{
		// Unzoom if we're jumping
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		m_flChargedDamage = 0.0f;
		m_bRezoomAfterShot = false;
	}
	else if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !m_bRezoomAfterShot )
		{
			float flChargeRate = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;
			CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_sniper_charge_per_sec );
			m_flChargedDamage = Min( m_flChargedDamage + gpGlobals->frametime * flChargeRate, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );
		}
		else
		{
			m_flChargedDamage = Max( 0.0f, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC );
		}
	}

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}
	
	//  Reload pressed / Clip Empty
	if ( ( pPlayer->m_nButtons & IN_RELOAD ) && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Classic::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsJumping() )
	{
		if ( pPlayer->GetFOV() >= 75 )
			return;
	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = Max( m_flNextPrimaryAttack.Get(), gpGlobals->curtime + 0.1f );
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ZoomOutIn( void )
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->ShouldAutoRezoom() )
	{
		m_flRezoomTime = gpGlobals->curtime + 0.9;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ZoomIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	BaseClass::ZoomIn();

	pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Create the sniper dot.
	CreateSniperDot();
	pPlayer->ClearExpression();
#endif
}

bool CTFSniperRifle_Classic::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ZoomOut( void )
{
	BaseClass::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	pPlayer->ClearExpression();
#endif

	// if we are thinking about zooming, cancel it
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
	m_flChargedDamage = 0.0f;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Fire( CTFPlayer *pPlayer )
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	if ( IsZoomed() )
	{
		// If we have more bullets, zoom out, play the bolt animation and zoom back in
		if( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
		{
			SetRezoom( true, 0.5f );	// zoom out in 0.5 seconds, then rezoom
		}
		else	
		{
			//just zoom out
			SetRezoom( false, 0.5f );	// just zoom out in 0.5 seconds
		}
	}
	else
	{
		// Prevent primary fire preventing zooms
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	}

	m_flChargedDamage = 0.0f;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::SetRezoom( bool bRezoom, float flDelay )
{
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle_Classic::GetProjectileDamage( void )
{
	// Uncharged? Min damage.
	return Max( m_flChargedDamage.Get(), TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifle_Classic::GetDamageType( void ) const
{
	// Only do hit location damage if we're zoomed
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return BaseClass::GetDamageType();

	return ( BaseClass::GetDamageType() & ~DMG_USE_HITLOCATIONS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::CreateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Check to see if we have already been created?
	if ( m_hSniperDot )
		return;

	// Get the owning player (make sure we have one).
	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	// Create the sniper dot, but do not make it visible yet.
	m_hSniperDot = CSniperDot::Create( GetAbsOrigin(), pPlayer, true );
	m_hSniperDot->ChangeTeam( pPlayer->GetTeamNumber() );

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::DestroySniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Destroy the sniper dot.
	if ( m_hSniperDot )
	{
		UTIL_Remove( m_hSniperDot );
		m_hSniperDot = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::UpdateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Get the start and endpoints.
	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, ( MASK_SHOT & ~CONTENTS_WINDOW ), GetOwner(), COLLISION_GROUP_NONE, &trace );

	// Update the sniper dot.
	if ( m_hSniperDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
		}

		m_hSniperDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Classic::CanFireCriticalShot( bool bIsHeadshot )
{
	if ( !BaseClass::CanFireCriticalShot( bIsHeadshot ) )
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// are we being damage boosted?
	if ( !pPlayer || pPlayer->m_Shared.IsCritBoosted() )
		return true;

	int nSniperNoHeadshot = 0;
	CALL_ATTRIB_HOOK_INT( nSniperNoHeadshot, set_weapon_mode );
	if ( nSniperNoHeadshot == 1 )
		return false;

	int nSniperFullChargeHeadShotOnly = 0;
	CALL_ATTRIB_HOOK_INT( nSniperFullChargeHeadShotOnly, sniper_no_headshot_without_full_charge );
	if ( nSniperFullChargeHeadShotOnly == 1 && m_flChargedDamage < TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX )
		return false;

	int nSniperNoScopeCrits = 0;
	CALL_ATTRIB_HOOK_INT( nSniperNoScopeCrits, sniper_crit_no_scope );
	if ( nSniperNoScopeCrits == 0 )
	{
		// no crits if they're not zoomed
		if ( pPlayer->GetFOV() >= pPlayer->GetDefaultFOV() )
		{
			return false;
		}

		// no crits for 0.2 seconds after starting to zoom
		if ( ( gpGlobals->curtime - pPlayer->GetFOVTime() ) < TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME )
		{
			return false;
		}
	}

	return true;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle_Classic::GetHUDDamagePerc( void )
{
	return ( m_flChargedDamage / TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );
}

//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleClassicCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleClassicCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( GetSpectatorTarget() ) );
	}

	if ( pPlayer )
	{
		CTFSniperRifle_Classic *pWeapon = assert_cast<CTFSniperRifle_Classic *>( pPlayer->GetActiveTFWeapon() );
		if ( pWeapon )
		{
			float flChargeValue = ( ( 1.0 - pWeapon->GetHUDDamagePerc() ) * 0.8 ) + 0.6;

			VMatrix mat, temp;

			Vector2D center( 0.5, 0.5 );
			MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );

			// scale
			{
				Vector2D scale( 1.0f, 0.25f );
				MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
				MatrixMultiply( temp, mat, mat );
			}

			MatrixBuildTranslation( temp, center.x, center.y, 0.0f );
			MatrixMultiply( temp, mat, mat );

			// translation
			{
				Vector2D translation( 0.0f, flChargeValue );
				MatrixBuildTranslation( temp, translation.x, translation.y, 0.0f );
				MatrixMultiply( temp, mat, mat );
			}

			m_pResult->SetMatrixValue( mat );
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CProxySniperRifleClassicCharge, IMaterialProxy, "SniperRifleClassicCharge" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif
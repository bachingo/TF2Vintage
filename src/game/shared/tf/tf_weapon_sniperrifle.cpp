//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_sniperrifle.h"
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

#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0f
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50.0f
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150.0f
#define TF_WEAPON_SNIPERRIFLE_RELOAD_TIME		1.5f
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define SNIPER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"
#define SNIPER_DOT_SPRITE_GREEN		"effects/sniperdot_green.vmt"
#define SNIPER_DOT_SPRITE_YELLOW	"effects/sniperdot_yellow.vmt"
#define SNIPER_DOT_SPRITE_CLEAR		"effects/sniperdot_clear.vmt"

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle, DT_TFSniperRifle )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle, DT_SniperRifleLocalData )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flChargedDamage ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO( m_flChargedDamage ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle, DT_TFSniperRifle )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flUnzoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flRezoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle );


CREATE_SIMPLE_WEAPON_TABLE( TFSniperRifle_Real, tf_weapon_sniperrifle_real )
CREATE_SIMPLE_WEAPON_TABLE( TFSniperRifle_Decap, tf_weapon_sniperrifle_decap )
CREATE_SIMPLE_WEAPON_TABLE( TFSniperRifle_Classic, tf_weapon_sniperrifle_classic )

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::~CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Precache()
{
	BaseClass::Precache();
	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ResetTimers( void )
{
	m_flUnzoomTime = -1;
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Reload( void )
{
	//Enable reloading, but only if our clipsize is defined.
	if ( Clip1() >= 0 )
	{
		if ( BaseClass::Reload() == true )
		{
			if ( IsZoomed() )
			ZoomOut();
			return true;
		}
		else
		return false;
	}
	else
	// We currently don't reload.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanHolster( void ) const
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
bool CTFSniperRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
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

void CTFSniperRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	ZoomOut();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::HandleZooms( void )
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
void CTFSniperRifle::ItemPostFrame( void )
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
		if ( ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !m_bRezoomAfterShot ) || ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && pPlayer->m_Shared.InCond(TF_COND_SNIPERCHARGE_RAGE_BUFF) ) ) 
		{
			float flChargeRate = GetChargingRate();
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
		if ( HasFocus() )
			ActivateFocus();
		else
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
bool CTFSniperRifle::Lower( void )
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
void CTFSniperRifle::Zoom( void )
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
void CTFSniperRifle::ZoomOutIn( void )
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
void CTFSniperRifle::ZoomIn( void )
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

	int nWeaponModeDot = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponModeDot, sniper_no_dot);
	
#ifdef GAME_DLL
	if (nWeaponModeDot == 0)
	{
		// Create the sniper dot.
		CreateSniperDot();
	}
	pPlayer->ClearExpression();
#endif
}

bool CTFSniperRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}

void CTFSniperRifle::DoFireEffects(void)
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return;

	// Muzzle flash on weapon.
	bool bMuzzleFlash = true;

	if (IsZoomed())
	{
		bMuzzleFlash = false;
	}

	if (bMuzzleFlash)
	{
		pPlayer->DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomOut( void )
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
void CTFSniperRifle::Fire( CTFPlayer *pPlayer )
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}
	
	int nRequiresZoom = 0;
	CALL_ATTRIB_HOOK_INT(nRequiresZoom, sniper_only_fire_zoomed);
	if ( nRequiresZoom && !IsZoomed() )
	{
		DenySniperShot();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	int nWeaponModeScope = 0;
	CALL_ATTRIB_HOOK_INT( nWeaponModeScope, sniper_no_zoomout );
	
	if (nWeaponModeScope == 0 || pPlayer->m_Shared.InCond(TF_COND_SNIPERCHARGE_RAGE_BUFF))
	{
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
void CTFSniperRifle::DenySniperShot( void )
{
		// Play this effect only when we can play the empty clip sound to prevent endless SPAM
		if ( m_flNextEmptySoundTime < gpGlobals->curtime )
		{
#ifdef CLIENT_DLL
			// Make a fizzing noise, and draw sparks.
			WeaponSound( SPECIAL2 );
			C_BaseEntity *pModel = GetWeaponForEffect();
			if ( pModel )
			{
				pModel->ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, "muzzle" );
			}
#endif	
			// Delay the next empty clip sound.
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			return;
		}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::SetRezoom( bool bRezoom, float flDelay )
{
	m_flUnzoomTime = gpGlobals->curtime + flDelay;

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProjectileDamage( void )
{
	// Uncharged? Min damage.
	float flDamage = Max( m_flChargedDamage.Get(), TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN );
	// Fully charged? Add extra damage attribute.
	if ( m_flChargedDamage == TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX )
		CALL_ATTRIB_HOOK_FLOAT (flDamage, sniper_full_charge_damage_bonus);
	
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifle::GetDamageType( void ) const
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
void CTFSniperRifle::CreateSniperDot( void )
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
void CTFSniperRifle::DestroySniperDot( void )
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
void CTFSniperRifle::UpdateSniperDot( void )
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
bool CTFSniperRifle::CanFireCriticalShot( bool bIsHeadshot )
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

float CTFSniperRifle::GetJarateTime( void )
{
	float flJarateDuration = 0;
	CALL_ATTRIB_HOOK_FLOAT( flJarateDuration, jarate_duration );
	if ( flJarateDuration > 0.0 )
		return RemapValClamped( m_flChargedDamage, TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX, 2.0, flJarateDuration );

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Used for calculating penetrating shots.
//-----------------------------------------------------------------------------
bool CTFSniperRifle::IsPenetrating(void)
{
	// If we penetrate on full charges, check our charge level.
	if ( m_flChargedDamage == TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX )
	{
		int nFullChargePenetrate = 0;
		CALL_ATTRIB_HOOK_INT(nFullChargePenetrate, sniper_penetrate_players_when_charged);
		
		if ( nFullChargePenetrate != 0 )
			return true;
		
	}
	
	return BaseClass::IsPenetrating();
}

//-----------------------------------------------------------------------------
// Purpose: Used for calculating Focus.
//-----------------------------------------------------------------------------
bool CTFSniperRifle::HasFocus(void)
{
	int nHasFocus = 0;
	CALL_ATTRIB_HOOK_INT(nHasFocus, sniper_rage_DISPLAY_ONLY);
		
	if ( nHasFocus != 0 )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays the Charge bar.
//-----------------------------------------------------------------------------
bool CTFSniperRifle::HasChargeBar(void)
{
	if ( HasFocus() )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Activates focus on our sniper rifle.
//-----------------------------------------------------------------------------
void CTFSniperRifle::ActivateFocus(void)
{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if (!pPlayer)
			return;
		
		if (!pPlayer->m_Shared.InCond(TF_COND_SNIPERCHARGE_RAGE_BUFF) && pPlayer->m_Shared.HasFocusCharge())
			pPlayer->m_Shared.AddCond(TF_COND_SNIPERCHARGE_RAGE_BUFF);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Decap::Deploy( void )
{
	if ( CTFSniperRifle::Deploy() )
	{
#ifdef GAME_DLL
		SetupGameEventListeners();
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle_Decap::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if (CTFSniperRifle::Holster(pSwitchingTo))
	{
#ifdef GAME_DLL
		StopListeningForAllEvents();
#endif
		return true;
	}

	return false;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Decap::SetupGameEventListeners( void )
{
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Decap::FireGameEvent( IGameEvent *event )
{
	if (FStrEq( event->GetName(), "player_death" ))
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( pOwner && engine->GetPlayerUserId( pOwner->edict() ) == event->GetInt( "attacker" ) && 
			 ( event->GetInt( "weaponid" ) == TF_WEAPON_SNIPERRIFLE_DECAP && event->GetInt( "customkill" ) == TF_DMG_CUSTOM_HEADSHOT ) )
		{
			OnHeadshot();
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Decap::OnHeadshot( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return;

	pOwner->m_Shared.IncrementHeadshotCount();

}

//-----------------------------------------------------------------------------
// Purpose: Tracks the current charging rate, from 50% at 0 Headshots to 200% at 6 Headshots.
//-----------------------------------------------------------------------------
float CTFSniperRifle_Decap::GetChargingRate( void )
{
	CTFPlayer *pOwner = ToTFPlayer(GetOwner());
	if (pOwner == nullptr)
		return TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;

	return 	( TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC * ( 0.5 + Min((pOwner->m_Shared.GetHeadshotCount()), 6 ) * 0.25 ) );
}


// Weapon Classic Start


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle_Classic::CTFSniperRifle_Classic()
{
	m_bIsChargingAttack = false;
#ifdef CLIENT_DLL
	m_pLaserSight = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle_Classic::~CTFSniperRifle_Classic()
{
	m_bIsChargingAttack = false;
#ifdef CLIENT_DLL
	if (m_pLaserSight)
		ToggleLaser();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:	Precaches the laser sights.
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "tfc_sniper_charge_red" );
	PrecacheParticleSystem( "tfc_sniper_charge_blue" );
}

//-----------------------------------------------------------------------------
// Purpose: Handles our charging.
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
	}

	// We're holding down the attack button, charge up our shot.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
#ifdef GAME_DLL
		if (!m_hSniperDot)
			CreateSniperDot();
#endif
#ifdef CLIENT_DLL
		if (!m_pLaserSight)
			ToggleLaser();
#endif
		float flChargeRate = GetChargingRate();
		CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_sniper_charge_per_sec );
		m_flChargedDamage = Min( m_flChargedDamage + gpGlobals->frametime * flChargeRate, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );
		if (!m_bIsChargingAttack)
			m_bIsChargingAttack = true;
		// We're now charging our shot, slow us down.
		if ( !pPlayer->m_Shared.InCond(TF_COND_AIMING) )
		{
			pPlayer->m_Shared.AddCond( TF_COND_AIMING );
			pPlayer->TeamFortress_SetSpeed();
		}
	}
	else
	{
		// We charged up our shot and released the primary, fire.
		if (m_bIsChargingAttack)
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
bool CTFSniperRifle_Classic::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#endif
#ifdef CLIENT_DLL
	// Destroy the laser sight.
	if (m_pLaserSight)
		ToggleLaser();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}
	
	// Remove our charging.
	m_bIsChargingAttack = false;	
	if ( pPlayer->m_Shared.InCond(TF_COND_AIMING) )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

	m_flChargedDamage = 0.0f;
	ResetTimers();


	return BaseClass::Holster( pSwitchingTo );
}

bool CTFSniperRifle_Classic::CanFire( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return true;
	
	// Can't shoot while jumping/airborne.
	if (!(pPlayer->GetFlags() & FL_ONGROUND))
		return false;
	
	return true;
	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::Fire(CTFPlayer *pPlayer)
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}
	
	int nRequiresZoom = 0;
	CALL_ATTRIB_HOOK_INT(nRequiresZoom, sniper_only_fire_zoomed);
	if ( nRequiresZoom && !IsZoomed() )
	{
		DenySniperShot();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	// If we are airborne then skip over this.
	if ( CanFire() )
		PrimaryAttack();
	
	// Reset our attack status.
	m_flChargedDamage = 0.0f;
	m_bIsChargingAttack = false;	
	if ( pPlayer->m_Shared.InCond(TF_COND_AIMING) )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

#ifdef GAME_DLL
	// Destroy the sniper dot.
	// This is implicit; since we just fired we're not charging the rifle.
	DestroySniperDot();
#endif
#ifdef CLIENT_DLL
	// Destroy the laser sight as well.
	if (m_pLaserSight)
		ToggleLaser();
#endif
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

	CTFWeaponBaseGun::ZoomIn();
	
#ifdef GAME_DLL
	pPlayer->ClearExpression();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ZoomOut( void )
{
	CTFWeaponBaseGun::ZoomOut();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Used for creating or destroying the laser sight.
//-----------------------------------------------------------------------------
void CTFSniperRifle_Classic::ToggleLaser( void )
{
	C_BaseEntity *pModel = GetWeaponForEffect();
		if ( !pModel )
		return;
			
	if ( m_bIsChargingAttack )
	{
		if ( !m_pLaserSight )
		{
			switch ( GetTeamNumber() )
			{
				case TF_TEAM_RED:
					m_pLaserSight = pModel->ParticleProp()->Create( "tfc_sniper_charge_red", PATTACH_POINT_FOLLOW, "laser" );
					break;
				case TF_TEAM_BLUE:
					m_pLaserSight = pModel->ParticleProp()->Create( "tfc_sniper_charge_blue", PATTACH_POINT_FOLLOW, "laser" );
					break;
				case TF_TEAM_GREEN:
					m_pLaserSight = pModel->ParticleProp()->Create( "tfc_sniper_charge_green", PATTACH_POINT_FOLLOW, "laser" );
					break;
				case TF_TEAM_YELLOW:
					m_pLaserSight = pModel->ParticleProp()->Create( "tfc_sniper_charge_yellow", PATTACH_POINT_FOLLOW, "laser" );
					break;
				default:
					m_pLaserSight = pModel->ParticleProp()->Create( "tfc_sniper_charge_red", PATTACH_POINT_FOLLOW, "laser" );
					break;
			}
		}
	}
	else
	{
		if ( m_pLaserSight )
		{
			pModel->ParticleProp()->StopEmissionAndDestroyImmediately( m_pLaserSight );
			m_pLaserSight = NULL;
		}
	}
}
#endif

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetHUDDamagePerc( void )
{
	return (m_flChargedDamage / TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX);
}

//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( pPlayer )
	{
		CTFSniperRifle *pWeapon = static_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
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

EXPOSE_INTERFACE( CProxySniperRifleCharge, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( SniperDot, DT_SniperDot )

BEGIN_NETWORK_TABLE( CSniperDot, DT_SniperDot )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeStartTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_sniperdot, CSniperDot );

BEGIN_DATADESC( CSniperDot )
DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSniperDot::CSniperDot( void )
{
	m_vecSurfaceNormal.Init();
	m_hTargetEnt = NULL;

#ifdef CLIENT_DLL
	m_hSpriteMaterial = NULL;
#endif

	ResetChargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CSniperDot::~CSniperDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CSniperDot
//-----------------------------------------------------------------------------
CSniperDot *CSniperDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
// Client specific.
#ifdef CLIENT_DLL

	return NULL;

// Server specific.
#else

	// Create the sniper dot entity.
	CSniperDot *pDot = static_cast<CSniperDot*>( CBaseEntity::Create( "env_sniperdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	//Create the graphic
	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	// Set owner.
	pDot->SetOwnerEntity( pOwner );

	// Force updates even though we don't have a model.
	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	return pDot;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal )
{
	SetAbsOrigin( vecOrigin );
	m_vecSurfaceNormal = vecNormal;
	m_hTargetEnt = pTarget;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// TFTODO: Make the sniper dot get brighter the more damage it will do.
//-----------------------------------------------------------------------------
int CSniperDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return -1;

	// Get the sprite rendering position.
	Vector vecEndPos;

	float flSize = 6.0;

	if ( !pPlayer->IsDormant() )
	{
		Vector vecAttachment, vecDir;
		QAngle angles;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			// Clamp the forward distance for the sniper's firstperson
			flDist = 384;

			flSize = 2.0;
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors( angles, &vecDir );
		}

		trace_t tr;
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

		// Backup off the hit plane, towards the source
		vecEndPos = tr.endpos + vecDir * -4;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	
	float flSniperChargeRatio;
	CTFSniperRifle *pWeapon = static_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
		if ( pWeapon )
			flSniperChargeRatio = (TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX / pWeapon->GetChargingRate());
		else
			flSniperChargeRatio = (TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX / TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC);
		
	float flStrength = RemapValClamped(flLifeTime, 0.0, flSniperChargeRatio, 0.1, 1.0);
	
	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSniperDot::ShouldDraw( void )			
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;
#if 0
	// Don't draw the sniper dot when in thirdperson.
	if ( ::input->CAM_IsThirdPerson() )
		return false;
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		switch (GetTeamNumber())
		{
		case TF_TEAM_RED:
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_RED, TEXTURE_GROUP_CLIENT_EFFECTS);
			break;
		case TF_TEAM_BLUE:
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_BLUE, TEXTURE_GROUP_CLIENT_EFFECTS);
			break;
		case TF_TEAM_GREEN:
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_GREEN, TEXTURE_GROUP_CLIENT_EFFECTS);
			break;
		case TF_TEAM_YELLOW:
			m_hSpriteMaterial.Init(SNIPER_DOT_SPRITE_YELLOW, TEXTURE_GROUP_CLIENT_EFFECTS);
			break;
		}
	}
}

#endif

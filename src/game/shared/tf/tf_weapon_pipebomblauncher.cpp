//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_fx_shared.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "prediction.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif
#include "tf_shareddefs.h"

#define HIGHLIGHT_CONTEXT	"BOMB_HIGHLIGHT_THINK"

#define TF_PIPEBOMB_MODE_REMOTE		0
#define TF_PIPEBOMB_MODE_VICINITY	1

//=============================================================================
//
// Weapon Pipebomb Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPipebombLauncher, DT_WeaponPipebombLauncher )

BEGIN_NETWORK_TABLE_NOBASE( CTFPipebombLauncher, DT_PipebombLauncherLocalData )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPipebombCount ) ),
#else
	SendPropInt( SENDINFO( m_iPipebombCount ), 5, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()


BEGIN_NETWORK_TABLE( CTFPipebombLauncher, DT_WeaponPipebombLauncher )
#ifdef CLIENT_DLL
	RecvPropDataTable( "PipebombLauncherLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PipebombLauncherLocalData ) ),
#else
	SendPropDataTable( "PipebombLauncherLocalData", 0, &REFERENCE_SEND_TABLE( DT_PipebombLauncherLocalData ), SendProxy_SendLocalWeaponDataTable ),
#endif	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFPipebombLauncher )
	DEFINE_FIELD( m_flChargeBeginTime, FIELD_FLOAT )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_pipebomblauncher, CTFPipebombLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_pipebomblauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFPipebombLauncher )
END_DATADESC()
#endif


CREATE_SIMPLE_WEAPON_TABLE( TFPipebombLauncher_Legacy, tf_weapon_pipebomblauncher_legacy )
CREATE_SIMPLE_WEAPON_TABLE( TFPipebombLauncher_TF2Beta, tf_weapon_pipebomblauncher_tf2beta )
CREATE_SIMPLE_WEAPON_TABLE( TFPipebombLauncher_TFC, tf_weapon_pipebomblauncher_tfc )

//=============================================================================
//
// Weapon Pipebomb Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::CTFPipebombLauncher()
{
	m_bReloadsSingly = true;
	m_flLastDenySoundTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::~CTFPipebombLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_PIPEBOMBLAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::Precache()
{
	PrecacheScriptSound("Weapon_StickyBombLauncher.ChargeUp");
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0;
	StopSound( "Weapon_StickyBombLauncher.ChargeUp" );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifndef CLIENT_DLL
	DetonateRemotePipebombs( true );
#endif

	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;

		SendWeaponAnim( ACT_VM_PULLBACK );
		
		EmitSound( "Weapon_StickyBombLauncher.ChargeUp" );
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;

		if ( flTotalChargeTime >= GetChargeMaxTime() )
		{
			LaunchGrenade();
		}
	}

#ifdef CLIENT_DLL
	if ( GetDetonateMode() == TF_PIPEBOMB_MODE_VICINITY && GetIndexForThinkContext( HIGHLIGHT_CONTEXT ) == NO_THINK_CONTEXT )
		SetContextThink( &CTFPipebombLauncher::BombHighlightThink, gpGlobals->curtime + 0.1, HIGHLIGHT_CONTEXT );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::WeaponIdle( void )
{
	if ( m_flChargeBeginTime > 0 && m_iClip1 > 0 )
	{
		LaunchGrenade();
	}
	else
	{
		BaseClass::WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	StopSound( "Weapon_StickyBombLauncher.ChargeUp" );

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	CTFGrenadePipebombProjectile *pProjectile = static_cast<CTFGrenadePipebombProjectile *>( FireProjectile( pPlayer ) );
	if ( pProjectile )
	{
		// Save the charge time to scale the detonation timer.
		pProjectile->SetChargeTime( gpGlobals->curtime - m_flChargeBeginTime );

		if ( GetDetonateMode() == TF_PIPEBOMB_MODE_VICINITY )
		{
			pProjectile->m_bDefensiveBomb = true;
			pProjectile->SetModel( "models/weapons/w_models/w_stickybomb_d.mdl" );
		}
	}
#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

	m_flLastDenySoundTime = gpGlobals->curtime;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}

	m_flChargeBeginTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPipebombLauncher::GetProjectileSpeed( void )
{
	float flForwardSpeed = RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
											0.0f,
											GetChargeMaxTime(),
											TF_PIPEBOMB_MIN_CHARGE_VEL,
											TF_PIPEBOMB_MAX_CHARGE_VEL );

	CALL_ATTRIB_HOOK_FLOAT( flForwardSpeed, mult_projectile_range );
	return flForwardSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::AddPipeBomb( CTFGrenadePipebombProjectile *pBomb )
{
	PipebombHandle hHandle;
	hHandle = pBomb;
	m_Pipebombs.AddToTail( hHandle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::ModifyPipebombsInView( int iMode )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return false;

#ifdef GAME_DLL
	int nStickiesKillStickies = 0;
	CALL_ATTRIB_HOOK_INT( nStickiesKillStickies, stickies_detonate_stickies );
#endif

	bool bFailedToDetonate = true;
	for ( int i = 0; i < m_Pipebombs.Count(); i++ )
	{
		CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[ i ];
		if ( pTemp )
		{
			if ( pTemp->IsEffectActive( EF_NODRAW ) )
				continue;

			if ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) < pTemp->GetLiveTime() )
				continue;

			Vector vecToPlayer = pTemp->WorldSpaceCenter() - pPlayer->EyePosition();
			vecToPlayer.NormalizeInPlace();

			Vector vecFwd;
			AngleVectors( pPlayer->EyeAngles(), &vecFwd );
			vecFwd.NormalizeInPlace();

			float flDistance = ( pPlayer->GetAbsOrigin() - pTemp->GetAbsOrigin() ).Length();

			if ( vecToPlayer.Dot( vecFwd ) > 0.975f || flDistance < 100.0f )
			{
			#ifdef GAME_DLL
				if ( iMode == TF_PIPEBOMB_DETONATE_CHECK )
				{
					if ( nStickiesKillStickies == 1 )
						pTemp->DetonateStickies();

					pTemp->Detonate();
				}
			#endif

			#ifdef CLIENT_DLL
				if ( iMode == TF_PIPEBOMB_GLOW_CHECK && pTemp->m_bDefensiveBomb )
					pTemp->m_bGlowing = true;
			#endif

				bFailedToDetonate = false;
			}
			else
			{
			#ifdef CLIENT_DLL
				if ( iMode == TF_PIPEBOMB_GLOW_CHECK && pTemp->m_bDefensiveBomb )
					pTemp->m_bGlowing = false;
			#endif
			}
		}
	}

	return bFailedToDetonate;
}

//-----------------------------------------------------------------------------
// Purpose: Add pipebombs to our list as they're fired
//-----------------------------------------------------------------------------
CBaseEntity *CTFPipebombLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
	if ( pProjectile )
	{
	#ifdef GAME_DLL
		int nMaxPipebombs = TF_WEAPON_PIPEBOMB_COUNT;
		CALL_ATTRIB_HOOK_INT( nMaxPipebombs, add_max_pipebombs );

		// If we've gone over the max pipebomb count, detonate the oldest
		if ( m_Pipebombs.Count() >= nMaxPipebombs )
		{
			CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[0];
			if ( pTemp )
			{
				pTemp->SetTimer( gpGlobals->curtime ); // explode NOW
			}

			m_Pipebombs.Remove( 0 );
		}

		PipebombHandle hHandle;
		hHandle = (CTFGrenadePipebombProjectile *)pProjectile;
		m_Pipebombs.AddToTail( hHandle );

		m_iPipebombCount = m_Pipebombs.Count();
	#endif
	}

	return pProjectile;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::BombHighlightThink( void )
{
	if ( GetOwner() == nullptr )
		return;

	ModifyPipebombsInView( TF_PIPEBOMB_GLOW_CHECK );

	SetNextThink( gpGlobals->curtime + 0.1, HIGHLIGHT_CONTEXT );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	// Allow player to fire and detonate at the same time.
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		if ( m_flChargeBeginTime > 0 && m_iClip1 > 0 )
		{
			LaunchGrenade();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs if secondary fire is down.
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemBusyFrame( void )
{
#ifdef GAME_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// We need to do this to catch the case of player trying to detonate
		// pipebombs while in the middle of reloading.
		SecondaryAttack();
	}
#endif

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Detonate active pipebombs
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

	if ( m_iPipebombCount )
	{
		// Get a valid player.
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return;

		//If one or more pipebombs failed to detonate then play a sound.
		if ( DetonateRemotePipebombs( false ) )
		{
			if ( m_flLastDenySoundTime <= gpGlobals->curtime )
			{
				// Deny!
				m_flLastDenySoundTime = gpGlobals->curtime + 1;
				WeaponSound( SPECIAL2 );
				return;
			}
		}
		else
		{
			// Play a detonate sound.
			WeaponSound( SPECIAL3 );
		}
	}
}

//=============================================================================
//
// Server specific functions.
//
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::UpdateOnRemove( void )
{
	// If we just died, we want to fizzle our pipebombs.
	// If the player switched classes, our pipebombs have already been removed.
	DetonateRemotePipebombs( true );

	BaseClass::UpdateOnRemove();
}


#endif


//-----------------------------------------------------------------------------
// Purpose: If a pipebomb has been removed, remove it from our list
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::DeathNotice( CBaseEntity *pVictim )
{
	Assert( dynamic_cast<CTFGrenadePipebombProjectile *>( pVictim ) );

	PipebombHandle hHandle;
	hHandle = (CTFGrenadePipebombProjectile *)pVictim;
	m_Pipebombs.FindAndRemove( hHandle );

	m_iPipebombCount = m_Pipebombs.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Remove *with* explosions
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::DetonateRemotePipebombs( bool bFizzle )
{
	if ( GetDetonateMode() != TF_PIPEBOMB_MODE_VICINITY || bFizzle )
	{
		bool bFailedToDetonate = false;

		for ( int i = 0; i < m_Pipebombs.Count(); i++ )
		{
			CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[ i ];
			if ( pTemp )
			{
				//This guy will die soon enough.
				if ( pTemp->IsEffectActive( EF_NODRAW ) )
					continue;

			#ifdef GAME_DLL
				if ( bFizzle )
				{
					pTemp->Fizzle();
				}
			#endif

				if ( !bFizzle )
				{
					if ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) < pTemp->GetLiveTime() )
					{
						bFailedToDetonate = true;
						continue;
					}
				}

			#ifdef GAME_DLL
				pTemp->Detonate();
			#endif
			}
		}

		return bFailedToDetonate;
	}

	return ModifyPipebombsInView( TF_PIPEBOMB_DETONATE_CHECK );
}


float CTFPipebombLauncher::GetChargeMaxTime( void )
{
	float flMaxChargeTime = TF_PIPEBOMB_MAX_CHARGE_TIME;
	CALL_ATTRIB_HOOK_INT( flMaxChargeTime, stickybomb_charge_rate );
	return flMaxChargeTime;
}


int CTFPipebombLauncher::GetDetonateMode( void ) const
{
	int nDetonateMode = 0;
	CALL_ATTRIB_HOOK_INT( nDetonateMode, set_detonate_mode );
	return nDetonateMode;
}


bool CTFPipebombLauncher::Reload( void )
{
	if ( m_flChargeBeginTime > 0 )
		return false;

	return BaseClass::Reload();
}

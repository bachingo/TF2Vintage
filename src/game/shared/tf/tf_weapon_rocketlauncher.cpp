//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher, DT_WeaponRocketLauncher )

BEGIN_NETWORK_TABLE( CTFRocketLauncher, DT_WeaponRocketLauncher )
#ifndef CLIENT_DLL
	//SendPropBool( SENDINFO(m_bLockedOn) ),
#else
	//RecvPropInt( RECVINFO(m_bLockedOn) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher, CTFRocketLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher )
END_DATADESC()
#endif


CREATE_SIMPLE_WEAPON_TABLE( TFRocketLauncher_Legacy, tf_weapon_rocketlauncher_legacy )
CREATE_SIMPLE_WEAPON_TABLE( TFRocketLauncher_Airstrike, tf_weapon_rocketlauncher_airstrike )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::CTFRocketLauncher()
{
	m_bReloadsSingly = true;
	m_nReloadPitchStep = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::~CTFRocketLauncher()
{
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "rocketbackblast" );

	PrecacheScriptSound( "Weapon_Airstrike.AltFire" );
	PrecacheScriptSound( "Weapon_Airstrike.Fail" );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::Misfire( void )
{
	CalcIsAttackCritical();

#ifdef GAME_DLL
	if ( CanOverload() )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( !pPlayer )
			return;

		CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket *>( BaseClass::FireProjectile( pPlayer ) );
		if ( pRocket )
		{
			trace_t tr;
			UTIL_TraceLine( pRocket->GetAbsOrigin(), pPlayer->EyePosition(), MASK_SOLID, pRocket, COLLISION_GROUP_NONE, &tr );
			pRocket->Explode( &tr, pPlayer );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFRocketLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	m_flShowReloadHintAt = gpGlobals->curtime + 30;
	m_nReloadPitchStep = Max( 0, m_nReloadPitchStep - 1 );

#ifdef GAME_DLL
	m_bOverloading = false;
#endif

	return BaseClass::FireProjectile( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();

#ifdef GAME_DLL
	if ( m_flShowReloadHintAt && m_flShowReloadHintAt < gpGlobals->curtime )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			pOwner->HintMessage( HINT_SOLDIER_RPG_RELOAD );
		}
		m_flShowReloadHintAt = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketLauncher::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	m_flShowReloadHintAt = 0;
	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketLauncher::CheckReloadMisfire( void )
{
#ifdef GAME_DLL
	if ( !CanOverload() )
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer == NULL )
		return false;

	if ( m_bOverloading )
	{
		if ( Clip1() > 0 )
		{
			Misfire();
			return true;
		}
		
		m_bOverloading = false;
	}
	else if ( Clip1() >= GetMaxClip1() || ( Clip1() > 0 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) == 0 ) )
	{
		Misfire();
		m_bOverloading = true;
		return true;
	}
#endif // GAME_DLL
	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	BaseClass::CreateMuzzleFlashEffects( pAttachEnt, nIndex );

	// Don't do backblast effects in first person
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner->IsLocalPlayer() )
		return;

	ParticleProp()->Create( "rocketbackblast", PATTACH_POINT_FOLLOW, "backblast" );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::ModifyEmitSoundParams( EmitSound_t &params )
{
	if ( !AutoFiresFullClip() )
		return;

	if ( V_strcmp( params.m_pSoundName, "Weapon_RPG.Reload" ) && V_strcmp( params.m_pSoundName, "Weapon_DumpsterRocket.Reload" ) )
		return;

	float flMaxClip = GetMaxClip1();
	float flAmmoPercentage = m_nReloadPitchStep / flMaxClip;

	if ( V_strcmp( params.m_pSoundName, "Weapon_RPG.Reload" ) == 0 )
	{
		params.m_pSoundName = "Weapon_DumpsterRocket.Reload_FP";
	}
	else
	{
		params.m_pSoundName = "Weapon_DumpsterRocket.Reload";
	}

	params.m_nPitch *= RemapVal( flAmmoPercentage, 0.0f, ( flMaxClip - 1.0f ) / flMaxClip, 0.79f, 1.19f );
	params.m_nFlags |= SND_CHANGE_PITCH;

	m_nReloadPitchStep = Min( GetMaxClip1() - 1, m_nReloadPitchStep + 1 );

	IncrementAmmo();

	m_bReloadedThroughAnimEvent = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketLauncher_Airstrike::Deploy( void )
{
	if ( CTFRocketLauncher::Deploy() )
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
bool CTFRocketLauncher_Airstrike::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if (CTFRocketLauncher::Holster( pSwitchingTo ))
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
void CTFRocketLauncher_Airstrike::SetupGameEventListeners( void )
{
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher_Airstrike::FireGameEvent( IGameEvent *event )
{
	if (FStrEq( event->GetName(), "player_death" ))
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( pOwner && engine->GetPlayerUserId( pOwner->edict() ) == event->GetInt( "attacker" ) && 
			 ( event->GetInt( "weaponid" ) == TF_WEAPON_ROCKETLAUNCHER_AIRSTRIKE ) )
		{
			OnKill();
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRocketLauncher_Airstrike::OnKill( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return;

	pOwner->m_Shared.IncrementStrikeCount();
	
}

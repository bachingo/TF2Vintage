//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_fx.h"
#endif

ConVar tf_double_donk_window( "tf_double_donk_window", "0.5", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "How long after an impact from a cannonball that an explosion will count as a double-donk." );

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_WeaponGrenadeLauncher )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_WeaponGrenadeLauncher )
#ifndef CLIENT_DLL
	SendPropTime( SENDINFO( m_flChargeBeginTime ) )
#else
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) )
#endif
END_NETWORK_TABLE()



#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CTFGrenadeLauncher)
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE )
END_PREDICTION_DATA()
#else
BEGIN_PREDICTION_DATA(CTFGrenadeLauncher)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );

//=============================================================================

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFGrenadeLauncher )
END_DATADESC()
#endif

#define TF_GRENADE_LAUNCER_MIN_VEL 1200
#define TF_GRENADES_SWITCHGROUP 2 
#define TF_GRENADE_BARREL_SPIN 0.25 // barrel increments by one quarter for each pill


CREATE_SIMPLE_WEAPON_TABLE( TFGrenadeLauncher_Legacy, tf_weapon_grenadelauncher_legacy )
CREATE_SIMPLE_WEAPON_TABLE( TFCannon, tf_weapon_cannon )

extern ConVar tf2v_console_grenadelauncher_magazine;

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;

#ifdef CLIENT_DLL
	m_pCannonFuse = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
#ifdef CLIENT_DLL
	if (m_pCannonFuse)
		ToggleCannonFuse();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();

	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound("Weapon_LooseCannon.Charge");
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0;
	StopSound("Weapon_LooseCannon.Charge");

#ifdef CLIENT_DLL
	if (m_pCannonFuse)
		ToggleCannonFuse();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponReset(void)
{
	BaseClass::WeaponReset();

	m_flChargeBeginTime = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetMaxClip1( void ) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetMaxClip1() but with the base set to TF_GRENADE_LAUNCHER_XBOX_CLIP.
	// We need to do it this way in order to consider attributes.
	if ( tf2v_console_grenadelauncher_magazine.GetBool() )
	{
		
		int iMaxClip = TF_GRENADE_LAUNCHER_XBOX_CLIP;
		if ( iMaxClip < 0 )
			return iMaxClip;

		CALL_ATTRIB_HOOK_FLOAT( iMaxClip, mult_clipsize );
		if ( iMaxClip < 0 )
			return iMaxClip;

		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner == NULL )
			return iMaxClip;

		int nClipSizePerKill = 0;
		CALL_ATTRIB_HOOK_INT( nClipSizePerKill, clipsize_increase_on_kill );

		iMaxClip += Min( nClipSizePerKill, pOwner->m_Shared.GetStrikeCount() );

		return iMaxClip;

	}

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetDefaultClip1( void ) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetDefaultClip1() is just checking GetMaxClip1(), nothing fancy to do here.
	if ( tf2v_console_grenadelauncher_magazine.GetBool() )
		return GetMaxClip1();
	 
	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::PrimaryAttack( void )
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

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if ( IsMortar() )
	{
		if ( m_flChargeBeginTime == 0.0f )
		{
			// save that we had the attack button down
			m_flChargeBeginTime = gpGlobals->curtime;
			// Turn on the cannon fuse.
#ifdef CLIENT_DLL
			if (!m_pCannonFuse)
				ToggleCannonFuse();

			EmitSound( "Weapon_LooseCannon.Charge" );
#endif

			SendWeaponAnim( ACT_VM_PULLBACK );
		}
	}
	else
	{
		LaunchGrenade();

		SwitchBodyGroups();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::FireFullClipAtOnce( void )
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Misfire( void )
{
	LaunchGrenade();
	SwitchBodyGroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponIdle( void )
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_flChargeBeginTime > 0.0f )
	{
		if ( gpGlobals->curtime < m_flChargeBeginTime + GetMortarTimeLength() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
			if ( !pPlayer )
				return;

			// If we're not holding down the attack button, launch our grenade
			if ( m_iClip1 > 0  && !(pPlayer->m_nButtons & IN_ATTACK) )
			{
				LaunchGrenade();
			}
		}
		else
		{
			Misfire();
		}
	}
}

CBaseEntity *CTFGrenadeLauncher::FireProjectileInternal( CTFPlayer *pPlayer )
{
	CTFWeaponBaseGrenadeProj *pGrenade = (CTFWeaponBaseGrenadeProj *)FireProjectile( pPlayer );
	if ( pGrenade )
	{
#ifdef GAME_DLL
		// Mortar weapons have a custom detonator time.
		if ( IsMortar() )
		{
			float flDetonateTimeLength = gpGlobals->curtime - m_flChargeBeginTime;
			pGrenade->SetDetonateTimerLength( flDetonateTimeLength );
			if ( flDetonateTimeLength <= 0.0f )
			{
				trace_t tr;
				UTIL_TraceLine( pGrenade->GetAbsOrigin(), pPlayer->EyePosition(), MASK_SOLID, pGrenade, COLLISION_GROUP_NONE, &tr );
				pGrenade->Explode( &tr, GetDamageType() );
			}
		}

		/*if ( GetDetonateMode() == TF_GL_MODE_FIZZLE )
			pGrenade->m_bFizzle = true;*/

		float flDetonationPenalty = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDetonationPenalty, grenade_detonation_damage_penalty );
		if ( flDetonationPenalty != 1.0f )
		{
			// Setting the initial damage of a grenade lower will set its fused time damage lower
			// on contact detonations reset the damage to max
			pGrenade->SetDamage( pGrenade->GetDamage() * flDetonationPenalty );
		}
#endif
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	if( !AutoFiresFullClipAllAtOnce() )
	{
		FireProjectileInternal( pPlayer );
	}
	else
	{
		int nNumShots = m_iClip1;
		int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
		QAngle punchAngle = pPlayer->GetPunchAngle();
		for ( int i=0; i<nNumShots; ++i, ++iSeed )
		{
			RandomSeed( iSeed );
			FireProjectileInternal( pPlayer );
			if ( i == 0 )
			{
				// store the punch from the first shot
				punchAngle = pPlayer->GetPunchAngle();
			}
		}
		pPlayer->SetPunchAngle( punchAngle );
	}

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}

	m_flChargeBeginTime = 0;

#ifndef CLIENT_DLL
	if ( IsMortar() )
	{
		Vector vPosition;
		QAngle qAngles;
		if ( GetAttachment( "muzzle", vPosition, qAngles ) )
		{
			CPVSFilter filter( vPosition );
			TE_TFParticleEffect( filter, 0.f, "loose_cannon_bang", PATTACH_POINT, this, "muzzle" );
		}
	}
#else
	if (m_pCannonFuse)
		ToggleCannonFuse();

	StopSound( "Weapon_LooseCannon.Charge" );
#endif
}

float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	float flVelocity = TF_GRENADE_LAUNCER_MIN_VEL;

	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	return flVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SecondaryAttack( void )
{
	BaseClass::SecondaryAttack();
}

bool CTFGrenadeLauncher::Reload( void )
{
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: Change model state to reflect available pills in launcher
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SwitchBodyGroups( void )
{
	if ( GetNumBodyGroups() < TF_GRENADES_SWITCHGROUP )
		return; 

    int iState = 4;

    iState = m_iClip1;

    SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
	SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );

    CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );

    if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
    {
		CBaseViewModel *vm = pTFPlayer->GetViewModel( m_nViewModelIndex );
        if ( vm )
        {
            vm->SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
			vm->SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );
        }
    }
}

int CTFGrenadeLauncher::GetDetonateMode( void ) const
{
	int nDetonateMode = 0;
	CALL_ATTRIB_HOOK_INT( nDetonateMode, set_detonate_mode );
	return nDetonateMode;
}


bool CTFGrenadeLauncher::IsMortar(void) const
{
	int nMortarMode = 0;
	CALL_ATTRIB_HOOK_INT(nMortarMode, grenade_launcher_mortar_mode);
	return ( nMortarMode != 0 );
}

float CTFGrenadeLauncher::GetMortarTimeLength(void)
{
	float flMortarMode = 0;
	CALL_ATTRIB_HOOK_FLOAT( flMortarMode, grenade_launcher_mortar_mode );
	return flMortarMode;
}


#ifdef CLIENT_DLL
void CTFGrenadeLauncher::ToggleCannonFuse()
{
	if (!m_pCannonFuse)
	{
		m_pCannonFuse = ParticleProp()->Create("loose_cannon_sparks", PATTACH_POINT_FOLLOW, "cannon_fuse");
	}
	else if (m_pCannonFuse)
	{
		ParticleProp()->StopEmission(m_pCannonFuse);
		m_pCannonFuse = NULL;
	}
}
#endif
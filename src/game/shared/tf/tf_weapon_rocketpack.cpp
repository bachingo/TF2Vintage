//====== Copyright © 1996-2020, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_rocketpack.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_team.h"
#endif

//=============================================================================
//
// Weapon Rocket Pack tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketPack, DT_TFWeaponRocketPack )

BEGIN_NETWORK_TABLE( CTFRocketPack, DT_TFWeaponRocketPack )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketPack )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketpack, CTFRocketPack );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketpack );

//=============================================================================
//
// Weapon Rocket Pack functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFRocketPack::CTFRocketPack()
{
	m_flInitLaunchTime = 0.0f;
	m_flLaunchTime = 0.0f;
	m_flToggleEndTime = 0.0f;
	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFRocketPack::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_RocketPack.BoostersExtend" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersRetract" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersCharge" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersLoop" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersFire" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersNotReady" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersReady" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersShutdown" );
	PrecacheScriptSound( "Weapon_RocketPack.Land" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRocketPack::Deploy( void )
{
	EmitSound( "Weapon_RocketPack.BoostersExtend" );
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::CanDeploy( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::CanFire( void ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( !pOwner->CanAttack() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::CanHolster( void ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_ROCKETPACK ) )
		return false;

	if ( m_flInitLaunchTime > gpGlobals->curtime )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::PrimaryAttack( void )
{
	if ( !CanFire() )
		return;

	InitiateLaunch();
}

//-----------------------------------------------------------------------------
// Purpose: same as primary
//-----------------------------------------------------------------------------
void CTFRocketPack::SecondaryAttack( void )
{
	PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRocketPack::InitiateLaunch( void )
{
	if ( m_flInitLaunchTime > gpGlobals->curtime )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	EmitSound( "Weapon_RocketPack.BoostersCharge" );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	Vector vecLaunch = pOwner->GetAbsVelocity();
	vecLaunch.z += 268.3281572999747f;
	pOwner->SetAbsVelocity( vecLaunch );

	m_flInitLaunchTime = gpGlobals->curtime + 0.7f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::PreLaunch( void )
{
	if ( m_flLaunchTime > gpGlobals->curtime )
		return;

	Launch();
	m_flLaunchTime = gpGlobals->curtime + 0.7f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::Launch( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	RocketLaunchPlayer( pOwner, vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::RocketLaunchPlayer( CTFPlayer *pPlayer, const Vector& vecLaunch )
{
	pPlayer->SetAbsVelocity( vecLaunch );
	pPlayer->m_Shared.AddCond( TF_COND_ROCKETPACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	EmitSound( "Weapon_RocketPack.BoostersFire" );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFRocketPack::WeaponReset( void )
{
	BaseClass::WeaponReset();
}

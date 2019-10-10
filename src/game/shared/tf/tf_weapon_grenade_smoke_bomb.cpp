//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Gas Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_smoke_bomb.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "particle_parse.h"
#include "te_effect_dispatch.h"
#endif

//=============================================================================
//
// TF Smoke Bomb tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeSmokeBomb, DT_TFGrenadeSmokeBomb )

BEGIN_NETWORK_TABLE( CTFGrenadeSmokeBomb, DT_TFGrenadeSmokeBomb )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeSmokeBomb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_smoke_bomb, CTFGrenadeSmokeBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_smoke_bomb );

ConVar tf_smoke_bomb_time("tf_smoke_bomb_time", "5.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY);

//=============================================================================
//
// TF Smoke Bomb functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeSmokeBomb )
END_DATADESC()

extern ConVar tf_smoke_bomb_time;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeSmokeBomb::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
													 AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{							
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer )
	{
		CDisablePredictionFiltering disabler;

		// Explosion effect on client
		CEffectData explosionData;
		explosionData.m_vOrigin = pPlayer->GetAbsOrigin();
		explosionData.m_vAngles = pPlayer->GetAbsAngles();
		explosionData.m_fFlags = GetWeaponID();
//		DispatchEffect( "TF_Explosion", explosionData );

		// give them the smoke bomb condition
		// ( invis for X seconds, able to move full speed )
		// ( attacking removes the condition )

		if ( pTFPlayer->CanGoInvisible() )
		{
			pTFPlayer->m_Shared.AddCond( TF_COND_SMOKE_BOMB, tf_smoke_bomb_time.GetFloat() );
		}
	}
	return CTFGrenadeSmokeBombProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

//-----------------------------------------------------------------------------
// Purpose: Don't explode automatically
//-----------------------------------------------------------------------------
bool CTFGrenadeSmokeBomb::ShouldDetonate( void )
{
	return false;
}

#endif // GAME_DLL


//=============================================================================
//
// TF Smoke Bomb Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/Weapons/w_models/w_grenade_frag.mdl"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_smoke_bomb_projectile, CTFGrenadeSmokeBombProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_smoke_bomb_projectile );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeSmokeBombProjectile* CTFGrenadeSmokeBombProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeSmokeBombProjectile *pGrenade = static_cast<CTFGrenadeSmokeBombProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_smoke_bomb_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBombProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBombProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBombProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Nail.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBombProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();

#if 0
	// Tell the bots an HE grenade has exploded
	CTFPlayer *pPlayer = ToTFPlayer( GetThrower() );
	if ( pPlayer )
	{
		KeyValues *pEvent = new KeyValues( "tf_weapon_grenade_detonate" );
		pEvent->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEventServerOnly( pEvent );
	}
#endif
}

#endif


//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2VR Scout Bat Implementation
//
//=============================================================================

#include "cbase.h"
#include "tfvr_bat.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF2VR Bat tables
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFVRBat, DT_TFVRBat )

BEGIN_NETWORK_TABLE( CTFVRBat, DT_TFVRBat )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVRBat )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_vr, CTFVRBat );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_vr );

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFVRBat )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFVRBat::CTFVRBat()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFVRBat::~CTFVRBat()
{
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CTFVRBat::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Perform a swing with the bat
//-----------------------------------------------------------------------------
void CTFVRBat::Swing( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;
	
	// Call base class to handle timing
	BaseClass::Swing( pPlayer );
	
	// Do trace to find what we hit
	trace_t trace;
	if ( DoSwingTrace( trace ) )
	{
#if !defined( CLIENT_DLL )
		// Hit something! Apply damage
		CBaseEntity *pEntity = trace.m_pEnt;
		if ( pEntity )
		{
			// Calculate damage with velocity multiplier
			int iDamageType = DMG_CLUB;
			int iCustomDamage = TF_DMG_CUSTOM_NONE;
			float flDamage = GetMeleeDamage( pEntity, &iDamageType, &iCustomDamage );
			
			// Apply velocity multiplier
			float flVelocityMult = CalculateSwingDamageMultiplier();
			flDamage *= flVelocityMult;
			
			// Create damage info
			CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDamageType, iCustomDamage );
			CalculateMeleeDamageForce( &info, trace.endpos - trace.startpos, trace.endpos, 1.0f );
			
			// Apply damage
			pEntity->DispatchTraceAttack( info, trace.endpos - trace.startpos, &trace );
			ApplyMultiDamage();
			
			// Call hit callback
			OnEntityHit( pEntity, &info );
			
			// Play impact sound
			WeaponSound( MELEE_HIT );
		}
#endif
	}
	else
	{
		// Missed - play miss sound
		WeaponSound( MELEE_MISS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do swing trace from bat tip
//-----------------------------------------------------------------------------
bool CTFVRBat::DoSwingTrace( trace_t &trace )
{
	// Get bat tip position (use muzzle position)
	Vector tipPos = GetMuzzleAbsOrigin();
	
	// Get player
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return false;
	
	// Trace from player's hand position to bat tip
	Vector startPos = GetAbsOrigin();
	
	// Extend the trace a bit beyond the tip
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector endPos = tipPos + forward * BAT_RANGE;
	
	// Perform trace
	CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
	UTIL_TraceLine( startPos, endPos, MASK_SOLID, &filter, &trace );
	
	// Also do a hull trace for better hit detection
	if ( trace.fraction >= 1.0f )
	{
		Vector mins( -8, -8, -8 );
		Vector maxs( 8, 8, 8 );
		UTIL_TraceHull( startPos, endPos, mins, maxs, MASK_SOLID, &filter, &trace );
	}
	
	return ( trace.fraction < 1.0f && trace.m_pEnt );
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Get melee damage (base TF2 bat damage is 35)
//-----------------------------------------------------------------------------
float CTFVRBat::GetMeleeDamage( CBaseEntity *pTarget, int *piDamageType, int *piCustomDamage )
{
	// Base bat damage
	float flDamage = 35.0f;
	
	// Apply TF2 damage modifiers (crits, mini-crits, etc.)
	if ( IsCurrentAttackACrit() )
	{
		flDamage *= 3.0f; // Crit multiplier
	}
	
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Called when we hit an entity
//-----------------------------------------------------------------------------
void CTFVRBat::OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;
	
	// Apply on-hit effects
	ApplyOnHitAttributes( pEntity, pPlayer, *info );
	
	// Track stats
	if ( pEntity->IsPlayer() )
	{
		CTF_GameStats.Event_PlayerDamage( pPlayer, *info, info->GetDamage() );
	}
}
#endif

//=============================================================================

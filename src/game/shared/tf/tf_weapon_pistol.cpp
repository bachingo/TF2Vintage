//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Weapon Pistol tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE_NOBASE( CTFPistol, DT_PistolLocalData )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFPistol, DT_WeaponPistol )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "PistolLocalData", 0, &REFERENCE_SEND_TABLE( DT_PistolLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "PistolLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PistolLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol, CTFPistol );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFPistol )
END_DATADESC()
#endif

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Scout, DT_WeaponPistol_Scout )

BEGIN_NETWORK_TABLE( CTFPistol_Scout, DT_WeaponPistol_Scout )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Scout )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_scout, CTFPistol_Scout );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_scout );

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFHandgun_Scout_Primary, DT_WeaponHandgun_Scout_Primary )

BEGIN_NETWORK_TABLE( CTFHandgun_Scout_Primary, DT_WeaponHandgun_Scout_Primary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFHandgun_Scout_Primary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_handgun_scout_primary, CTFHandgun_Scout_Primary );
PRECACHE_WEAPON_REGISTER( tf_weapon_handgun_scout_primary );

	ConVar tf2v_use_shortstop_shove( "tf2v_use_shortstop_shove", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows Shortstop to Alt-Fire and shove enemies away." );


//=============================================================================
//
// Weapon Pistol functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPistol::PrimaryAttack( void )
{
#if 0
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		//pOwner->ViewPunchReset();
	}
#endif

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHandgun_Scout_Primary::Precache( void )
{
	PrecacheScriptSound( "Weapon_Hands.Push" );
	PrecacheScriptSound( "Weapon_Hands.PushImpact" );
	
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHandgun_Scout_Primary::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;
	
	// Don't allow us to shove if shoves are disabled.
	if (!tf2v_use_shortstop_shove.GetBool())
		return;

	// Do our animations and sounds.
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	SendWeaponAnim( ACT_SECONDARY_VM_ALTATTACK );
	EmitSound( "Weapon_Hands.Push" );

	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;	// Shoves have a longer waiting time between attacks.
	
	// Set up the wait for the shove swing calculation.
	m_flPushDelay = gpGlobals->curtime + 0.2f;	// Melee delay before performing swing calculations

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHandgun_Scout_Primary::Shove( void )
{
#if !defined( CLIENT_DLL )
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;


	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );


	// see if we can hit an object with a higher range

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pOwner->EyeAngles(), &vecForward );
	Vector vecSwingStart = pOwner->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;


	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnoreTeammates filterFriendlies( this, COLLISION_GROUP_NONE, GetTeamNumber() );
	UTIL_TraceLine(vecSwingStart, vecSwingEnd, MASK_SOLID, &filterFriendlies, &trace);
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull(vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &filterFriendlies, &trace);
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsPlayer() &&
		 trace.m_pEnt->IsAlive() &&
		 trace.m_pEnt->GetTeamNumber() != pOwner->GetTeamNumber() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( trace.m_pEnt );
		if (pTFPlayer)
		{
			// Play the push sound.
			EmitSound( "Weapon_Hands.PushImpact" );
				
			// We do a pathetic amount of damage.
			CTakeDamageInfo info( pOwner, pOwner, this, 1, DMG_PREVENT_PHYSICS_FORCE );
			pTFPlayer->TakeDamage( info );

			// Set up the blast back.
						
			Vector vecPushDir;
			QAngle angPushDir = pOwner->EyeAngles();

			// assume that shooter is looking at least 45 degrees up.
			angPushDir[PITCH] = Min( -45.f, angPushDir[PITCH] );
			AngleVectors( angPushDir, &vecPushDir );
						
			// Don't push players if they're too far off to the side. Ignore Z.
			Vector vecVictimDir = pTFPlayer->WorldSpaceCenter() - pOwner->WorldSpaceCenter();

			Vector vecVictimDir2D( vecVictimDir.x, vecVictimDir.y, 0.0f );
			VectorNormalize( vecVictimDir2D );

			Vector vecDir2D( vecPushDir.x, vecPushDir.y, 0.0f );
			VectorNormalize( vecDir2D );

			float flDot = DotProduct( vecDir2D, vecVictimDir2D );
			if ( flDot >= 0.8 )
			{
				// Push enemy players.
				pTFPlayer->SetGroundEntity(NULL);
											
				pTFPlayer->SetAbsVelocity(vecPushDir * 500);
				pTFPlayer->m_Shared.AddCond(TF_COND_NO_MOVE, 0.5f);
			}
		}
	}


	lagcompensation->FinishLagCompensation( pOwner );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFHandgun_Scout_Primary::ItemPostFrame()
{
	// Check for smack.
	if (tf2v_use_shortstop_shove.GetBool())
	{
		if ( m_flPushDelay > 0 && gpGlobals->curtime > m_flPushDelay )
		{
			Shove();
			m_flPushDelay = 0;
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFHandgun_Scout_Primary::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Don't holster if we're waiting to shove.
	if ( m_flPushDelay > 0 && gpGlobals->curtime <= m_flPushDelay )
		return false;

	return BaseClass::Holster( pSwitchingTo );
}

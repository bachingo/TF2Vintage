//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Defibrillator
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_defib.h"
#include "decals.h"
#include "tf_viewmodel.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "in_buttons.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Defib 
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFDefib, DT_TFWeaponDefib )

BEGIN_NETWORK_TABLE( CTFDefib, DT_TFWeaponDefib )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFDefib )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_defib, CTFDefib );
PRECACHE_WEAPON_REGISTER( tf_weapon_defib );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFDefib::CTFDefib()
{
	
}

//-----------------------------------------------------------------------------
// Purpose: Modified version of the melee swing.
//			Similar to Engineer's wrench, but instead of buildings, we look for corpses.
//-----------------------------------------------------------------------------
void CTFDefib::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

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

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}
	
	// See if this is a special swing.
	bool bSpecialSwing = false;
#ifdef GAME_DLL
	if (trace.fraction < 1.0f && trace.m_pEnt)
		bSpecialSwing = OnRagdollHit(pOwner, trace.endpos);
#endif
		
	// if it's not a special swing, smack as usual for player hits
	if (!bSpecialSwing)
		BaseClass::Smack();
}

#ifdef GAME_DLL

#define  TF_DEFIB_REVIVE_RADIUS 32
//-----------------------------------------------------------------------------
// Purpose: Revives the dead player.
// Output:  Whether anything interesting happens or not.
//-----------------------------------------------------------------------------
bool CTFDefib::OnRagdollHit(CTFPlayer* pOwner, Vector vecHitPos )
{
	// See if there are any corpses near the end of our swing.
	// We do this as a sphere, for any error such as desync.
	CBaseEntity* pEntity = NULL;
	CTFPlayer* pVictim = NULL;
	for (CEntitySphereQuery sphere(GetAbsOrigin(), TF_DEFIB_REVIVE_RADIUS); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if (!pEntity)
			continue;

		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint(GetAbsOrigin(), &vecHitPoint);
		Vector vecDir = vecHitPoint - GetAbsOrigin();
		if (vecDir.LengthSqr() < (TF_DEFIB_REVIVE_RADIUS * TF_DEFIB_REVIVE_RADIUS))
		{
			CTFPlayer* pCandidate = dynamic_cast<CTFPlayer*>(pEntity);
			if (pCandidate)
			{
				// Check if they're alive and in the same team as the reviver.
				if (pCandidate->IsAlive() || !pCandidate->InSameTeam(pOwner))
					continue;

				// we only define one entity here intentionally.
				pVictim = pCandidate;
				break;
			}
		}
	}
	
	// Can't return an invalid entity.
	if (!pVictim)
		return false;
	
	// Already alive, skip.
	if ( pVictim->IsAlive() || !pVictim->InSameTeam(pOwner) )
		return false;
	
	// Likely someone we can revive, try to revive them.
	bool bRevived = RevivePlayer( pVictim, pOwner );
	if (bRevived)
	{
		// If we're here, it means we revived them.
		// We need to do some extra effects with the corpse.
		
		// We're going to move them to where the ragdoll is, and play a sound.

		pVictim->Teleport( &vecHitPos, &GetAbsAngles(), &vec3_origin);
		pVictim->EmitSound( "MVM.PlayerRevived" );
		
		// Since it's not a corpse anymore, remove.
		//UTIL_Remove(pRagdoll);

		return true;
	}
	else
	{
		// We ran the function but we didn't revive for some reason.
		// Play a sound that we tried to revive and failed.
		EmitSound( "Halloween.spell_lightning_impact" );
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Revives the dead player.
// Output: 	Is if revival was successful.
//-----------------------------------------------------------------------------
bool CTFDefib::RevivePlayer( CTFPlayer *pRevival, CTFPlayer *pOwner )
{
	// We can only revive players who are both actually dead and on our team.
	if ( !pRevival->IsAlive() && pRevival->InSameTeam( pOwner ) )
	{
		pRevival->ForceRespawn();
		
		// Did the player respawn?
		if ( pRevival->IsAlive() )
		{
			pRevival->SpeakConceptIfAllowed( MP_CONCEPT_RESURRECTED );
			
			// This worked, so we should award a bonus point to the person who healed.
			IGameEvent *event_bonus = gameeventmanager->CreateEvent( "player_bonuspoints" );
			if ( event_bonus )
			{
				event_bonus->SetInt( "player_entindex", pRevival->entindex() );
				event_bonus->SetInt( "source_entindex", pOwner->entindex() );
				event_bonus->SetInt( "points", 1 );

				gameeventmanager->FireEvent( event_bonus );
			}
			
			CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, pRevival, 1 );
			
			return true;
		}
		
	}
	return false;
}
#endif
//====== Copyright ? 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_syringe.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Weapon Syringe tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFSyringe, tf_weapon_syringe )

//=============================================================================
//
// Weapon Syringe functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFSyringe::CTFSyringe()
{
}



bool CTFSyringe::HitPlayer( CBaseEntity *pTarget )
{
	// We check to make sure they can be healed first.

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	if ( !pTFPlayer )
		return false;

	bool bStealthed = pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED );
	bool bDisguised = pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED );

	// We can heal teammates and enemies that are disguised as teammates
	if ( !bStealthed &&
		( pTFPlayer->InSameTeam( pOwner ) ||
		 ( bDisguised && pTFPlayer->m_Shared.GetDisguiseTeam() == pOwner->GetTeamNumber() ) ) )
	{
		return true;
	}

	return false;
}

bool CTFSyringe::CanBeHealed( CBaseEntity *pTarget ) const
{
#if defined( GAME_DLL )
	int nWeaponBlocksHealing = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTarget, nWeaponBlocksHealing, weapon_blocks_healing );
	if ( nWeaponBlocksHealing == 1 )
		return false;
#endif
	return true;
}

void CTFSyringe::Smack( void )
{
	trace_t trace;
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

#if !defined( CLIENT_DLL )
	// Do extra lag compensation here so we don't struggle to hit friendlies
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	// See if we hit anything.
	bool bHitSomething = DoSwingTrace( trace );
#if !defined( CLIENT_DLL )
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
	// We hit, setup the smack.
	if ( bHitSomething )
	{
		// We make sure the person is a friendly, or disguised as one.
		if ( HitPlayer( trace.m_pEnt ) )
		{
			// We run a second check to make sure they aren't running any no-heal items.
			if ( CanBeHealed( trace.m_pEnt ) )
			{
			#if defined( GAME_DLL )
				CTFPlayer *pPatient = ToTFPlayer( trace.m_pEnt );
				int iDamageCustom = GetCustomDamageType(), iDamageType = DMG_GENERIC;
				int iHealthRestored = pPatient->TakeHealth( GetMeleeDamage( trace.m_pEnt, iDamageType, iDamageCustom ), iDamageType );
				CTF_GameStats.Event_PlayerHealedOther( pPlayer, iHealthRestored );

				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					event->SetInt( "patient", pPatient->GetUserID() );
					event->SetInt( "healer", pPlayer->GetUserID() );
					event->SetInt( "amount", iHealthRestored );

					gameeventmanager->FireEvent( event );
				}
			#endif

				OnEntityHit( trace.m_pEnt );
			#if defined( CLIENT_DLL )
				UTIL_ImpactTrace( &trace, DMG_CLUB );
			#endif
				m_bConnected = true;
			}
			else
			{
				// We just end the script without bothering to do a melee attack, since we'd be meleeing our friend otherwise.
				return;
			}
		}
		else
		{
			// If we can't heal, then do the regular melee instead.
			BaseClass::Smack();
		}
	}
}
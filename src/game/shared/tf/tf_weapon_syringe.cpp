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



bool CTFSyringe::OnPlayerHit( CBaseObject *pTarget )
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
}

bool CTFSyringe::CanBeHealed( CBaseObject *pTarget )
{
	int nWeaponBlocksHealing = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, nWeaponBlocksHealing, weapon_blocks_healing );
	if ( nWeaponBlocksHealing == 1 )
		return false;

	return true;
}

void CTFSyringe::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against players.

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnoreObjects traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt)
	{
		// We make sure the person is a friendly, or disguised as one.
		bool OnPlayerHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ));
		if ( OnPlayerHit ) 
		{
			// We run a second check to make sure they aren't running any no-heal items.
			bool CanBeHealed( dynamic_cast< CBaseObject *>( trace.m_pEnt ));
			if ( CanBeHealed )
			{
				int iHealthRestored = pOwner->TakeHealth( m_flDamage, DMG_GENERIC );

				if ( iHealthRestored )
				{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );

					if ( event )
					{
					event->SetInt( "amount", iHealthRestored );
					event->SetInt( "entindex", pOwner->entindex() );
					gameeventmanager->FireEvent( event );
					}
				}
			}
			else
			return; // We just end the script without bothering to do a melee attack, since we'd be meleeing our friend otherwise.
		}
		else
		// If we can't heal, then do the regular melee instead.
		BaseClass::Smack();
	}
	else	 
	{
		// if it's a building, melee as usual.
		BaseClass::Smack();
	}
}
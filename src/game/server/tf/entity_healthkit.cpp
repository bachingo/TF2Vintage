//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_healthkit.h"
#include "tf_gamestats.h"
#include "tf_weapon_lunchbox.h"

extern ConVar tf2v_sandvich_behavior;

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_MODEL			"models/items/healthkit.mdl"
#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKit.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_HEALTHKIT_PICKUP_SOUND );
	PrecacheScriptSound( "OverhealPillRattle.Touch" );
	PrecacheScriptSound( "OverhealPillNoRattle.Touch" );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		Assert( pTFPlayer );

		int iHealthToAdd = ceil( pPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()] );
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, iHealthToAdd, mult_health_frompacks );
		int iHealthRestored = 0;

		// Don't heal the player who dropped this healthkit, recharge his lunchbox instead
		// This only applies to the 2nd lunchbox behavior type.
		if ( ( pTFPlayer != GetOwnerEntity() ) || tf2v_sandvich_behavior.GetInt() != 2 )
		{
			iHealthRestored = pTFPlayer->TakeHealth( iHealthToAdd, DMG_GENERIC );
			//iHealthRestored = pPlayer->TakeHealth( iHealthToAdd, DMG_GENERIC );

			if ( iHealthRestored )
				bSuccess = true;

			// Restore disguise health.
			if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				int iFakeHealthToAdd = ceil( pTFPlayer->m_Shared.GetDisguiseClass() * PackRatios[ GetPowerupSize() ] );
				CTFPlayer *pDisguiseTarget = ToTFPlayer(pTFPlayer->m_Shared.GetDisguiseTarget());
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pDisguiseTarget, iFakeHealthToAdd, mult_health_frompacks );

				if ( pTFPlayer->m_Shared.AddDisguiseHealth( iFakeHealthToAdd ) )
					bSuccess = true;
			}

			// Remove any negative conditions whether player got healed or not.
			if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) || pTFPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
			{
				bSuccess = true;
			}
		}

		if ( bSuccess )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( GetClassname() );
			MessageEnd();

			const char *pszSound = TF_HEALTHKIT_PICKUP_SOUND;

			EmitSound( user, entindex(), pszSound );

			pTFPlayer->m_Shared.HealthKitPickupEffects( iHealthRestored );

			CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
			if ( pTFOwner && pTFOwner->InSameTeam( pTFPlayer ) )
			{
				// Bonus points.
				IGameEvent *event_bonus = gameeventmanager->CreateEvent( "player_bonuspoints" );
				if ( event_bonus )
				{
					event_bonus->SetInt( "player_entindex", pTFPlayer->entindex() );
					event_bonus->SetInt( "source_entindex", pTFOwner->entindex() );
					event_bonus->SetInt( "points", 1 );

					gameeventmanager->FireEvent( event_bonus );
				}

				CTF_GameStats.Event_PlayerAwardBonusPoints( pTFOwner, pPlayer, 1 );
			}

			if ( iHealthRestored  )
			{
				IGameEvent *event_healonhit = gameeventmanager->CreateEvent( "player_healonhit" );
				if ( event_healonhit )
				{
					event_healonhit->SetInt( "amount", iHealthRestored );
					event_healonhit->SetInt( "entindex", pPlayer->entindex() );

					gameeventmanager->FireEvent( event_healonhit );
				}

				// Show healing to the one who dropped the healthkit.
				CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
				if ( pOwner )
				{
					IGameEvent *event_healed = gameeventmanager->CreateEvent( "player_healed" );
					if ( event_healed )
					{
						event_healed->SetInt( "patient", pPlayer->GetUserID() );
						event_healed->SetInt( "healer", pOwner->GetUserID() );
						event_healed->SetInt( "amount", iHealthRestored );

						gameeventmanager->FireEvent( event_healed );
					}
				}
			}
		}
		else
		{
			// Recharge lunchbox if player's at full health.
			CTFWeaponBase *pLunch = pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX );
			if ( pLunch && pLunch->GetEffectBarProgress() < 1.0f )
			{
				CDisablePredictionFiltering disabler;

				pLunch->EffectBarRegenFinished();
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Update healthkit model for holiday events
//-----------------------------------------------------------------------------
const char *CHealthKit::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/medkit_large_bday.mdl";
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			return "models/props_halloween/halloween_medkit_large.mdl";
		}
	}

	return "models/items/medkit_large.mdl"; // default
}

//-----------------------------------------------------------------------------
// Purpose: Update healthkit model for holiday events
//-----------------------------------------------------------------------------
const char *CHealthKitSmall::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/medkit_small_bday.mdl";
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			return "models/props_halloween/halloween_medkit_small.mdl";
		}
	}

	return "models/items/medkit_small.mdl"; // default
}

//-----------------------------------------------------------------------------
// Purpose: Update healthkit model for holiday events
//-----------------------------------------------------------------------------
const char *CHealthKitMedium::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/medkit_medium_bday.mdl";
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			return "models/props_halloween/halloween_medkit_medium.mdl";
		}
		else if ( TFGameRules()->IsInMedievalMode() )
		{
			return "models/props_medieval/medieval_meat.mdl";
		}
	}

	return "models/items/medkit_medium.mdl"; // default
}
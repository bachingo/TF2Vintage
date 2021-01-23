//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_ammopack.h"

//=============================================================================
//
// CTF AmmoPack defines.
//

LINK_ENTITY_TO_CLASS( item_ammopack_full, CAmmoPack );
LINK_ENTITY_TO_CLASS( item_ammopack_small, CAmmoPackSmall );
LINK_ENTITY_TO_CLASS( item_ammopack_medium, CAmmoPackMedium );

//=============================================================================
//
// CTF AmmoPack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CAmmoPack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		int iMaxPrimary = pTFPlayer->GetMaxAmmo( TF_AMMO_PRIMARY );
		if ( pPlayer->GiveAmmo( ceil(iMaxPrimary * PackRatios[GetPowerupSize()]), TF_AMMO_PRIMARY, true ) )
		{
			bSuccess = true;
		}

		int iMaxSecondary = pTFPlayer->GetMaxAmmo( TF_AMMO_SECONDARY );
		if ( pPlayer->GiveAmmo( ceil(iMaxSecondary * PackRatios[GetPowerupSize()]), TF_AMMO_SECONDARY, true ) )
		{
			bSuccess = true;
		}

		int iMaxMetal = pTFPlayer->GetMaxAmmo( TF_AMMO_METAL );
		if ( pPlayer->GiveAmmo( ceil(iMaxMetal * PackRatios[GetPowerupSize()]), TF_AMMO_METAL, true ) )
		{
			bSuccess = true;
		}
		
		// Add grenades if we are missing them.
		int iMaxGrenade1 = pTFPlayer->GetMaxAmmo( TF_AMMO_GRENADES1 );
		if ( pPlayer->GiveAmmo( ceil(iMaxGrenade1 * PackRatios[GetPowerupSize()]), TF_AMMO_GRENADES1, true ) )
		{
			bSuccess = true;
		}
		
		int iMaxGrenade2 = pTFPlayer->GetMaxAmmo( TF_AMMO_GRENADES2 );
		if ( pPlayer->GiveAmmo( ceil(iMaxGrenade2 * PackRatios[GetPowerupSize()]), TF_AMMO_GRENADES2, true ) )
		{
			bSuccess = true;
		}
		

		if (pTFPlayer->m_Shared.AddToSpyCloakMeter( ceil( 100.0f * PackRatios[GetPowerupSize()] ) ))
		{
			bSuccess = true;
		}
		
		int nAmmoGivesCharge = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( pTFPlayer, nAmmoGivesCharge, ammo_gives_charge );
		if ( nAmmoGivesCharge != 0 )
		{
			if ( pTFPlayer->m_Shared.m_flChargeMeter < 100.0f )
			{
				pTFPlayer->m_Shared.m_flChargeMeter = min( ( pTFPlayer->m_Shared.m_flChargeMeter + ( ( PackRatios[GetPowerupSize()] ) * 100 ) ), 100.0f ) ;
				bSuccess = true;
			}
		}

		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TF_AMMOPACK_PICKUP_SOUND );

			//CTF_GameStats.Event_PlayerAmmokitPickup( pTFPlayer );

			IGameEvent *event = gameeventmanager->CreateEvent( "item_pickup" );
			if( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetString( "item", GetAmmoPackName() );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	return bSuccess; 
}

//-----------------------------------------------------------------------------
// Purpose: Update ammo pack model for holiday events
//-----------------------------------------------------------------------------
const char *CAmmoPack::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/ammopack_large_bday.mdl";
		}
	}

	return "models/items/ammopack_large.mdl"; // default
}

//-----------------------------------------------------------------------------
// Purpose: Update ammo pack model for holiday events
//-----------------------------------------------------------------------------
const char *CAmmoPackMedium::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/ammopack_medium_bday.mdl";
		}
	}

	return "models/items/ammopack_medium.mdl"; // default
}

//-----------------------------------------------------------------------------
// Purpose: Update ammo pack model for holiday events
//-----------------------------------------------------------------------------
const char *CAmmoPackSmall::GetDefaultPowerupModel( void )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_TF2Birthday ) )
		{
			return "models/items/ammopack_small_bday.mdl";
		}
	}

	return "models/items/ammopack_small.mdl"; // default
}
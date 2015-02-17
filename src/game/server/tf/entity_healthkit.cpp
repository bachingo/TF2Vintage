//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
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
#include "tf_weapon_backpack.h"

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
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{		
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GetActiveWeapon();
		if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_BACKPACK)
		{
			CTFBackpack* pBackpack = (CTFBackpack*)pWeapon;
			if (pBackpack->CanPickup())
			{				
				int Size = GetPowerupSize();
				pBackpack->AddNewEntityByType(TF_BACKPACK_HEALTHKIT, Size);
				CSingleUserRecipientFilter filter(pPlayer);
				EmitSound(filter, entindex(), TF_BACKPACK_TAKE_SOUND);
				bSuccess = true;
			}
			
		}
		else if ( pPlayer->TakeHealth( ceil(pPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()]), DMG_GENERIC ) )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();

			UserMessageBegin( user, "ItemPickup" );
				WRITE_STRING( GetClassname() );
			MessageEnd();

			EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );

			bSuccess = true;

			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

			Assert( pTFPlayer );

			// Healthkits also contain a fire blanket.
			if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );		
			}

			// Remove tranq condition
			if ( pTFPlayer->m_Shared.InCond( TF_COND_SLOWED ) )
			{
				pTFPlayer->m_Shared.RemoveCond( TF_COND_SLOWED );
			}

		}
	}

	if (bSuccess && GetRespawnDelay() == -1)
	{
		UTIL_Remove(this);
	}
	return bSuccess;
}

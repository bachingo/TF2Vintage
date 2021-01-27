//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ammo_pack.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "tf_powerup.h"
#include "entity_ammopack.h"
#include "tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TF_MODEL_PUMPKIN_LOOT "models/props_halloween/pumpkin_loot.mdl"
#define TF_PUMPKIN_LOOT_DROP "Halloween.PumpkinDrop"
#define TF_PUMPKIN_LOOT_PICKUP "Halloween.PumpkinPickup"
#define TF_MODEL_GIFT "models/items/tf_gift.mdl"
#define TF_GIFT_DROP "Christmas.GiftDrop"
#define TF_GIFT_PICKUP "Christmas.GiftPickup"

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFAmmoPack, DT_AmmoPack )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFAmmoPack )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_ammo_pack, CTFAmmoPack );

PRECACHE_REGISTER( tf_ammo_pack );

void CTFAmmoPack::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFAmmoPack::FlyThink );

	SetTouch( &CTFAmmoPack::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// no ammo to start
	memset( m_iAmmo, 0, sizeof(m_iAmmo) );

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFAmmoPack::Precache( void )
{
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			PrecacheModel( TF_MODEL_PUMPKIN_LOOT );
			PrecacheScriptSound( TF_PUMPKIN_LOOT_DROP );
			PrecacheScriptSound( TF_PUMPKIN_LOOT_PICKUP );
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			PrecacheModel( TF_MODEL_GIFT );
			PrecacheScriptSound( TF_GIFT_DROP );
			PrecacheScriptSound( TF_GIFT_PICKUP );
		}
	}
}

CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, bool bUseCustomAmmoCount )
{
	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack*>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		pAmmoPack->SetModelName( AllocPooledString( pszModelName ) );
		DispatchSpawn( pAmmoPack );
	}

	pAmmoPack->m_bUseCustomAmmoCount = bUseCustomAmmoCount;
	return pAmmoPack;
}

void CTFAmmoPack::SetInitialVelocity( Vector &vecVelocity )
{ 
	if ( iHoliday )
	{
		// Special rules for holiday loot packs
		VPhysicsDestroyObject();
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetAbsVelocity( vecVelocity + Vector( 0.0f, 0.0f, 200.0f ) );
		SetAbsAngles( vec3_angle );
		UseClientSideAnimation();
		ResetSequence( LookupSequence( "idle" ) );
	}
	m_vecInitialVelocity = vecVelocity;
}

int CTFAmmoPack::GiveAmmo( int iCount, int iAmmoType )
{
	if (iAmmoType == -1 || iAmmoType >= TF_AMMO_COUNT )
	{
		Msg("ERROR: Attempting to give unknown ammo type (%d)\n", iAmmoType);
		return 0;
	}

	m_iAmmo[iAmmoType] += iCount;

	return iCount;
}

void CTFAmmoPack::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}

void CTFAmmoPack::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;

	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	Assert( pPlayer );

	// tf_ammo_pack (dropped weapons) originally packed killed player's ammo.
	// This was changed to make them act as medium ammo packs.
	// PistonMiner: Someone screwed the system up making it impossible 
	//				to use custom ammo values using GiveAmmo, I changed 
	//				this to only use this code if no custom ammo is specified.
#if 0
	// Old ammo giving code.
	int iAmmoTaken = 0;

	int i;
	for ( i=0;i<TF_AMMO_COUNT;i++ )
	{
		iAmmoTaken += pPlayer->GiveAmmo( m_iAmmo[i], i );
	}

	if ( iAmmoTaken > 0 )
	{
		UTIL_Remove( this );
	}
#else
	// Copy-paste from CAmmoPack code.

	bool bSuccess = false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return;

	if ( !m_bIsLunchbox )
	{
		if ( !m_bUseCustomAmmoCount )
		{
			int iMaxPrimary = pTFPlayer->GetMaxAmmo( TF_AMMO_PRIMARY );
			if ( pPlayer->GiveAmmo( ceil(iMaxPrimary * PackRatios[POWERUP_MEDIUM]), TF_AMMO_PRIMARY ) )
			{
				bSuccess = true;
			}

			int iMaxSecondary = pTFPlayer->GetMaxAmmo( TF_AMMO_SECONDARY );
			if ( pPlayer->GiveAmmo( ceil(iMaxSecondary * PackRatios[POWERUP_MEDIUM]), TF_AMMO_SECONDARY ) )
			{
				bSuccess = true;
			}

			//int iMaxMetal = pTFPlayer->GetPlayerClass()->GetData()->m_aAmmoMax[TF_AMMO_METAL];
			// Unlike other ammo, give fixed amount of metal that was given to us at spawn.
			if ( pPlayer->GiveAmmo( m_iAmmo[TF_AMMO_METAL], TF_AMMO_METAL ) )
			{
				bSuccess = true;
			}

			// Add grenades if we are missing them.
			int iMaxGrenade1 = pTFPlayer->GetMaxAmmo( TF_AMMO_GRENADES1 );
			if (pPlayer->GiveAmmo(ceil(iMaxGrenade1 * PackRatios[POWERUP_MEDIUM]), TF_AMMO_GRENADES1, true))
			{
				bSuccess = true;
			}
			
			int iMaxGrenade2 = pTFPlayer->GetMaxAmmo( TF_AMMO_GRENADES2 );
			if (pPlayer->GiveAmmo(ceil(iMaxGrenade2 * PackRatios[POWERUP_MEDIUM]), TF_AMMO_GRENADES2, true))
			{
				bSuccess = true;
			}
		
			// Unlike medium ammo packs, restore only 25% cloak.
			if (pTFPlayer->m_Shared.AddToSpyCloakMeter( 25.0f ))
			{
				bSuccess = true;
			}

			int nAmmoGiveCharge = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nAmmoGiveCharge, ammo_gives_charge );
			if ( nAmmoGiveCharge )
			{
				float flCharge = pTFPlayer->m_Shared.GetShieldChargeMeter();
				if ( flCharge < 100.0f )
				{
					pTFPlayer->m_Shared.SetShieldChargeMeter( Min( flCharge + 25.0f, 100.0f ) );
					bSuccess = true;
				}
			}

			switch ( iHoliday )
			{
				case kHoliday_Halloween: // Give player crits for three seconds
					if ( pTFPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED_PUMPKIN ) || pTFPlayer->m_Shared.GetConditionDuration( TF_COND_CRITBOOSTED_PUMPKIN ) < 3.0f )
					{
						pTFPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, 3.0f );
					}
					EmitSound( TF_PUMPKIN_LOOT_PICKUP );
					bSuccess = true;
					break;
				case kHoliday_Christmas: // Gifts don't do anything (for now at least)
					EmitSound( TF_GIFT_PICKUP );
					bSuccess = true;
					break;
			}
		}
		else
		{
			for ( int i = 0; i < TF_AMMO_COUNT; ++i )
			{
				pPlayer->GiveAmmo( m_iAmmo[i], i );
			}
			bSuccess = true;
		}
	}
	else
	{
		int iHealthRestored = 0;
		// If the player is a scout give them 75hp, otherwise give them 50hp
		if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
		{
			pPlayer->TakeHealth( 75.0f, DMG_GENERIC );
			iHealthRestored = 75;
		}
		else
		{
			pPlayer->TakeHealth( 50.0f, DMG_GENERIC );
			iHealthRestored = 50;
		}

		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();

		UserMessageBegin( user, "ItemPickup" );
		WRITE_STRING( GetClassname() );
		MessageEnd();

		const char *pszSound = "HealthKit.Touch";

		EmitSound( user, entindex(), pszSound );

		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
			
		if ( event )
		{
			event->SetInt( "amount", iHealthRestored );
			event->SetInt( "entindex", pPlayer->entindex() );
				
			gameeventmanager->FireEvent( event );
		}
		bSuccess = true;
	}

	// did we give them anything?
	if ( bSuccess )
	{
		//CTF_GameStats.Event_PlayerAmmokitPickup( pPlayer );

		IGameEvent * event = gameeventmanager->CreateEvent( "item_pickup" );
		if( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "item", "tf_ammo_pack" );
			gameeventmanager->FireEvent( event );
		}

		UTIL_Remove( this );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFAmmoPack::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFAmmoPack::MakeHolidayPack( void )
{ 
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			iHoliday = kHoliday_Halloween;
			SetModel( TF_MODEL_PUMPKIN_LOOT );
			SetContextThink( &CTFAmmoPack::DropSoundThink, gpGlobals->curtime + 0.1f, "DROP_SOUND_THINK" );
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			iHoliday = kHoliday_Christmas;
			SetModel( TF_MODEL_GIFT );
			SetContextThink( &CTFAmmoPack::DropSoundThink, gpGlobals->curtime + 0.1f, "DROP_SOUND_THINK" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFAmmoPack::DropSoundThink( void )
{ 
	const char *iszSound = "";

	switch ( iHoliday )
	{
		case kHoliday_Halloween:
			iszSound = TF_PUMPKIN_LOOT_DROP;
			break;
		case kHoliday_Christmas:
			iszSound = TF_GIFT_DROP;
			break;
	}

	if ( iszSound[0] )
	{
		EmitSound( iszSound );
	}
}
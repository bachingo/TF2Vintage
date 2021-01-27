//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_upgrades.h"
#include "tf_upgrades_shared.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "tf_weapon_wrench.h"
#include "tf_obj.h"
#include "tf_powerup_bottle.h"

CHandle<CUpgrades> g_hUpgradeEntity;

BEGIN_DATADESC( CUpgrades )
	DEFINE_KEYFIELD( m_nStartDisabled, FIELD_INTEGER, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_ENTITYFUNC( UpgradeTouch )
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_upgradestation, CUpgrades );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
attrib_def_index_t ApplyUpgrade_Default( CMannVsMachineUpgrades const &upgrade, CTFPlayer *pPlayer, CEconItemView *pItem, int nCost, bool bDowngrade )
{
	if ( pItem == nullptr )
		return INVALID_ATTRIBUTE_DEF_INDEX;

	// Attribute must exist
	CEconAttributeDefinition const *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttribute );
	if ( pAttribute == nullptr )
		return INVALID_ATTRIBUTE_DEF_INDEX;

	// Determine source
	CAttributeList *pAttribList = NULL;
	if ( upgrade.nUIGroup == UIGROUP_UPGRADE_PLAYER )
		pAttribList = pPlayer->GetAttributeList();
	else if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ITEM )
		pAttribList = pItem->GetAttributeList();

	Assert( pAttribList );
	if ( pAttribList == nullptr )
		return INVALID_ATTRIBUTE_DEF_INDEX; // Should never happen

	float flBaseValue = ( pAttribute->description_format == ATTRIB_FORMAT_PERCENTAGE || pAttribute->description_format == ATTRIB_FORMAT_INVERTED_PERCENTAGE ) ? 1.0f : 0.0f;
	// Get the starting value from our item's static data, if applicable
	if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ITEM )
		FindAttribute<uint32>( pItem->GetStaticData(), pAttribute, &flBaseValue );

	float flIncrement = upgrade.flIncrement;
	const float flCap = upgrade.flCap;

	float flCurrentValue = 0.0f;
	if ( FindAttribute<uint32>( pAttribList, pAttribute, &flCurrentValue ) )
	{
		// Ensure we can actually increase the value
		if ( !bDowngrade && ( AlmostEqual( flCurrentValue, flCap ) ||
			 ( flIncrement > 0 && flCurrentValue >= flCap ) ||
			 ( flIncrement < 0 && flCurrentValue <= flCap ) ) )
		{
			return INVALID_ATTRIBUTE_DEF_INDEX;
		}

		float flNewValue = flBaseValue;
		if ( bDowngrade )
		{
			const float flStartValue = flBaseValue;
			flIncrement = fabsf( flIncrement );

			//Ugh... not even sure why this is done
			if ( AlmostEqual( flCurrentValue, flCap ) && !AlmostEqual( flStartValue, flBaseValue ) )
			{
				float flShift = flStartValue;
				if ( flIncrement > 0 )
				{
					do {
						flShift += flIncrement;
					} while ( flShift < flCap && !AlmostEqual( flShift, flCap ) );
				}
				else
				{
					do  {
						flShift += flIncrement;
					} while ( flShift > flCap && !AlmostEqual( flShift, flCap ) );
				}

				const float flDelta = fabsf( flCap - flShift );
				if ( !AlmostEqual( flIncrement, flDelta ) )
				{
					flIncrement -= flDelta;
				}
			}

			flNewValue = Approach( flStartValue, flCurrentValue, flIncrement );
			if ( AlmostEqual( flNewValue, flStartValue ) )
			{
				// We downgraded back to base value, remove
				pAttribList->RemoveAttribute( pAttribute );

				return pAttribute->index;
			}
		}
		else
		{
			flNewValue = Approach( flCap, flCurrentValue, fabsf( flIncrement ) );
		}

		pAttribList->SetRuntimeAttributeValue( pAttribute, flNewValue );
		pAttribList->SetRuntimeAttributeRefundableCurrency( pAttribute, nCost );

		return pAttribute->index;
	}

	if ( bDowngrade )
	{
		// Can't downgrade from here
		return INVALID_ATTRIBUTE_DEF_INDEX;
	}

	// Didn't exist, add it dynamically
	pAttribList->SetRuntimeAttributeValue( pAttribute, flBaseValue + flIncrement );
	pAttribList->SetRuntimeAttributeRefundableCurrency( pAttribute, nCost );

	return pAttribute->index;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::Spawn( void )
{
	m_bEnabled = true;

	BaseClass::Spawn();
	g_hUpgradeEntity = this;

	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	InitTrigger();
	SetTouch( &CUpgrades::UpgradeTouch );

	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::FireGameEvent( IGameEvent *event )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::UpgradeTouch( CBaseEntity *pOther )
{
	if ( m_bEnabled && PassesTriggerFilters( pOther ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
		if ( pTFPlayer )
			pTFPlayer->m_Shared.SetInUpgradeZone( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::EndTouch( CBaseEntity *pOther )
{
	if ( IsTouching( pOther ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
		if( pTFPlayer )
			pTFPlayer->m_Shared.SetInUpgradeZone( false );
	}

	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;

	FOR_EACH_VEC( m_hTouchingEntities, i )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hTouchingEntities[i].Get() );
		if ( pTFPlayer )
			pTFPlayer->m_Shared.SetInUpgradeZone( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputReset( inputdata_t &inputdata )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::ApplyUpgradeAttributeBlock( UpgradeAttribBlock_t *pUpgradeBlock, int nUpgradeCount, CTFPlayer *pPlayer, bool bDowngrade )
{
	if ( pPlayer == nullptr )
		return;

	for ( int i=0; i < nUpgradeCount; ++i )
	{
		if ( !pUpgradeBlock[i].szName || !pUpgradeBlock[i].szName[0] )
			continue;

		CAttributeList *pAttribList = NULL; CEconEntity *pItem = NULL;
		if ( pUpgradeBlock[i].iItemSlot == TF_LOADOUT_SLOT_COUNT )
		{
			pAttribList = pPlayer->GetAttributeList();
		}
		else
		{
			pItem = pPlayer->GetEntityForLoadoutSlot( pUpgradeBlock[i].iItemSlot );
			if ( pItem == nullptr )
				continue;

			pAttribList = pItem->GetAttributeList();
		}

		CEconAttributeDefinition *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( pUpgradeBlock[i].szName );
		if ( pAttribute == nullptr )
			continue;

		if ( bDowngrade )
		{
			if ( pItem )
				RestoreItemAttributeToBaseValue( pAttribute, pItem->GetItem() );
			else
				RestorePlayerAttributeToBaseValue( pAttribute, pPlayer );
		}
		else
		{
			pAttribList->SetRuntimeAttributeValue( pAttribute, pUpgradeBlock[i].flValue );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
attrib_def_index_t CUpgrades::ApplyUpgradeToItem( CTFPlayer *pPlayer, CEconItemView *pItem, int nUpgrade, int nCost, bool bDowngrade, bool bIsFresh )
{
	if( pPlayer == nullptr )
		return INVALID_ATTRIBUTE_DEF_INDEX;

	const CMannVsMachineUpgrades& upgrade = g_MannVsMachineUpgrades.GetUpgradeVector()[ nUpgrade ];

	const CEconAttributeDefinition *pAttribute = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttribute );
	if ( pAttribute == nullptr )
		return INVALID_ATTRIBUTE_DEF_INDEX;

	bool bIsBottle = upgrade.nUIGroup == UIGROUP_UPGRADE_POWERUP;
	ReportUpgrade( pPlayer, 
				   pItem ? pItem->GetItemDefIndex() : 0, 
				   pAttribute->index, upgrade.nQuality, nCost, 
				   bDowngrade, bIsFresh, bIsBottle );

	if ( bIsBottle )
	{

		CEconEntity *pActionItem = pPlayer->GetEntityForLoadoutSlot( TF_LOADOUT_SLOT_ACTION );
		CTFPowerupBottle *pPowerupBottle = dynamic_cast<CTFPowerupBottle *>( pActionItem );
		if ( !pPowerupBottle )
			return INVALID_ATTRIBUTE_DEF_INDEX;

		CAttributeList *pAttribList = pItem->GetAttributeList();

		Assert( pAttribList );
		if ( pAttribList == nullptr )
			return INVALID_ATTRIBUTE_DEF_INDEX; // Should never happen

		if ( FindAttribute( pAttribList, pAttribute ) )
		{
			const int nSign = bDowngrade ? -1 : 1;
			const int nNewCharges =  pPowerupBottle->GetNumCharges() + nSign;

			if ( nNewCharges < 0 || nNewCharges > pPowerupBottle->GetMaxNumCharges() )
				return INVALID_ATTRIBUTE_DEF_INDEX;

			pPowerupBottle->SetNumCharges( nNewCharges );
			pAttribList->SetRuntimeAttributeRefundableCurrency( pAttribute,
																pAttribList->GetRuntimeAttributeRefundableCurrency( pAttribute ) + ( nCost * nSign ) );

			if ( nNewCharges == 0 )
			{
				pPowerupBottle->RemoveEffect();
				pAttribList->RemoveAllAttributes();

				/*if ( g_pPopulationManager )
					g_pPopulationManager->ForgetOtherBottleUpgrades( pPlayer, pItem, nUpgrade );*/
			}

			return pAttribute->index;
		}

		if ( bDowngrade )
		{
			// Can't downgrade from here
			return INVALID_ATTRIBUTE_DEF_INDEX;
		}

		// Remove old powerup
		pPowerupBottle->RemoveEffect();
		pAttribList->RemoveAllAttributes();

		/*if ( g_pPopulationManager )
			g_pPopulationManager->ForgetOtherBottleUpgrades( pPlayer, pItem, nUpgrade );*/

		pPowerupBottle->SetNumCharges( 1 );

		pAttribList->SetRuntimeAttributeValue( pAttribute, upgrade.flIncrement );
		pAttribList->SetRuntimeAttributeRefundableCurrency( pAttribute, nCost );

		return pAttribute->index;
	}

	return ApplyUpgrade_Default( upgrade, pPlayer, pItem, nCost, bDowngrade );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CUpgrades::GetUpgradeAttributeName( int iUpgrade ) const
{
	if ( iUpgrade < 0 || iUpgrade >= g_MannVsMachineUpgrades.GetUpgradeCount() )
		return nullptr;

	return g_MannVsMachineUpgrades.GetUpgradeVector()[ iUpgrade ].szAttribute;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::NotifyItemOnUpgrade( CTFPlayer *pPlayer, attrib_def_index_t nAttrDefIndex, bool bDowngrade )
{
	static CSchemaAttributeHandle pAttrDef_BuffDuration( "mod buff duration" );
	if ( pPlayer == nullptr )
		return;

	switch ( nAttrDefIndex )
	{
		case 286:
		{
			CTFWrench *pWrench = dynamic_cast<CTFWrench *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
			if ( pWrench )
			{
				pWrench->ApplyBuildingHealthUpgrade();
			}
			break;
		}
		case 320:
		{
			CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );
			if ( pBuilder )
			{
				pBuilder->SetRoboSapper( true );
			}
			break;
		}
		case 351:
			if ( bDowngrade )
			{
				// if we're refunding engy_disposable_sentries we need to destroy the disposable sentry if we have one
				if ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					for ( int i = pPlayer->GetObjectCount(); --i >= 0; )
					{
						CBaseObject *pObj = pPlayer->GetObject( i );
						if ( pObj && ( pObj->GetType() == OBJ_SENTRYGUN ) && ( pObj->IsDisposableBuilding() ) )
							pObj->DetonateObject();
					}
				}
			}
			break;
		case 375:
			if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				if ( !pAttrDef_BuffDuration )
					return;

				pPlayer->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_BuffDuration, 0.5f );
			}
			break;
		case 499:
			if ( pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				if ( !pAttrDef_BuffDuration )
					return;

				pPlayer->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_BuffDuration, 1.2f );
			}
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::ReportUpgrade( CTFPlayer *pPlayer, int iItemDef, int iAttributeDef, int nQuality, int nCost, bool bDowngrade, bool bIsFresh, bool bIsBottle )
{
	if ( pPlayer == nullptr )
		return;

	if ( TFGameRules() && TFGameRules()->IsPVEModeActive() )
	{
		// Some stat reporting stuff, might be implemented later
	}

	if ( bDowngrade )
		return;

	pPlayer->EmitSound( "MVM.PlayerUpgraded" );

	IGameEvent *event = gameeventmanager->CreateEvent( "player_upgraded" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::RestoreItemAttributeToBaseValue( CEconAttributeDefinition *pAttribute, CEconItemView *pItem )
{
	CAttributeList *pAttribList = pItem->GetAttributeList();

	float flCurrentValue = 0.0f;
	if ( FindAttribute<uint32>( pAttribList, pAttribute, &flCurrentValue ) )
	{
		float flBaseValue = ( pAttribute->description_format == ATTRIB_FORMAT_PERCENTAGE || pAttribute->description_format == ATTRIB_FORMAT_INVERTED_PERCENTAGE ) ? 1.0f : 0.0f;
		FindAttribute<uint32>( pAttribList, pAttribute, &flBaseValue ); // This doesn't seem right
		
		if ( AlmostEqual( flCurrentValue, flBaseValue ) )
		{
			pAttribList->RemoveAttribute( pAttribute );
			return;
		}

		pAttribList->SetRuntimeAttributeValue( pAttribute, flBaseValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::RestorePlayerAttributeToBaseValue( CEconAttributeDefinition *pAttribute, CTFPlayer *pPlayer )
{
	CAttributeList *pAttribList = pPlayer->GetAttributeList();

	float flCurrentValue = 0.0f;
	if ( FindAttribute<uint32>( pAttribList, pAttribute, &flCurrentValue ) )
	{
		float flBaseValue = ( pAttribute->description_format == ATTRIB_FORMAT_PERCENTAGE || pAttribute->description_format == ATTRIB_FORMAT_INVERTED_PERCENTAGE ) ? 1.0f : 0.0f;
		FindAttribute<uint32>( pAttribList, pAttribute, &flBaseValue ); // This doesn't seem right

		if ( AlmostEqual( flCurrentValue, flBaseValue ) )
		{
			pAttribList->RemoveAttribute( pAttribute );
			return;
		}

		pAttribList->SetRuntimeAttributeValue( pAttribute, flBaseValue );
	}
}

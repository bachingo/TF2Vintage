//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_inventory.h"
#include "econ_entity_creation.h"
#include "tf_item_system.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "item_pickup_panel.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "character_info_panel.h"
#include "ienginevgui.h"
#include "c_tf_gamestats.h"
#include "econ_notifications.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "econ/econ_item_preset.h"
#include "tf_shared_content_manager.h"
#include "c_playerresource.h"
#include "quest_log_panel.h"
#include "backpack_panel.h"
#include "materialsystem/itexture.h"

#include "tf_gc_client.h"
#include "gcsdk/gcsdk_auto.h"

#else
#include "tf_player.h"
#endif
#include "gc_clientsystem.h"
#include "econ_game_account_client.h"
#include "gcsdk/gcclientsdk.h"
#include "econ_gcmessages.h"
#include "tf_gamerules.h"
#include "tf_gcmessages.h"
#include "econ_item.h"
#include "game_item_schema.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

#define LOCAL_LOADOUT_FILE		"cfg/local_loadout.txt"
#define OFFLINE_ITEM_CACHE_FILE	"cfg/local_inventory.txt"

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
CEconNotification_HasNewItems::CEconNotification_HasNewItems() : CEconNotification()
{
	m_bHasTriggered = false;
	m_flExpireTime = -1.0f;		// Does not initially expire

	m_bShowInGame = false;

	// Check to see if any items are not drops, if so show in game
	// do not show in game if the new items are only drops
	CPlayerInventory *pLocalInv = InventoryManager()->GetLocalInventory();
	if ( pLocalInv )
	{
		// Go through the root inventory and find any items that are in the "found" position
		int iCount = pLocalInv->GetItemCount();
		for ( int i = 0; i < iCount; i++ )
		{
			CEconItemView *pTmp = pLocalInv->GetItem(i);
			if ( !pTmp )
				continue;

			if ( pTmp->GetStaticData()->IsHidden() )
				continue;

			uint32 iPosition = pTmp->GetInventoryPosition();
			if ( IsUnacknowledged(iPosition) == false )
				continue;
			if ( InventoryManager()->GetBackpackPositionFromBackend(iPosition) != 0 )
				continue;

			// Now make sure we haven't got a clientside saved ack for this item.
			if ( InventoryManager()->HasBeenAckedByClient( pTmp ) )
				continue;

			// If item is not a drop we want to show the notification otherwise they'll get the notification on death
			int iFoundMethod = GetUnacknowledgedReason( iPosition );
			if ( iFoundMethod > UNACK_ITEM_DROPPED )
			{
				m_bShowInGame = true;
				break;
			}
		}
	}
}
	

CEconNotification_HasNewItems::~CEconNotification_HasNewItems()
{
	if ( !m_bHasTriggered )
	{
		m_bHasTriggered = true;
		TFInventoryManager()->ShowItemsPickedUp( true, true, true );
	}
}

//-----------------------------------------------------------------------------
CEconNotification_HasNewItemsOnKill::CEconNotification_HasNewItemsOnKill( int iVictimID )
{
	m_bHasTriggered = false;
	SetLifetime( 3.0f );
	SetText( "#TF_EnemyDroppedItem" );

	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
	g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iVictimID ), wszPlayerName, sizeof( wszPlayerName ) );
	AddStringToken( "victim", wszPlayerName );

	m_bShowInGame = true;
}

/*static*/ bool CEconNotification_HasNewItemsOnKill::HasUnacknowledgedItems () 
{
	// Check to see if any items are not drops, if so show in game
	// do not show in game if the new items are only drops
	CPlayerInventory *pLocalInv = InventoryManager()->GetLocalInventory();
	if ( pLocalInv )
	{
		// Go through the root inventory and find any items that are in the "found" position
		int iCount = pLocalInv->GetItemCount();
		for ( int i = 0; i < iCount; i++ )
		{
			CEconItemView *pTmp = pLocalInv->GetItem( i );
			if ( !pTmp )
				continue;

			if ( pTmp->GetStaticData()->IsHidden() )
				continue;

			uint32 iPosition = pTmp->GetInventoryPosition();
			if ( IsUnacknowledged( iPosition ) == false )
				continue;
			if ( InventoryManager()->GetBackpackPositionFromBackend( iPosition ) != 0 )
				continue;

			// Now make sure we haven't got a clientside saved ack for this item.
			if ( InventoryManager()->HasBeenAckedByClient( pTmp ) )
				continue;

			// If item is not a drop we want to show the notification otherwise they'll get the notification on death
			int iFoundMethod = GetUnacknowledgedReason( iPosition );
			if ( iFoundMethod > UNACK_ITEM_DROPPED )
			{
				return true;
			}
		}
	}
	return false;
}


#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool AreSlotsConsideredIdentical( EEquipType_t eEquipType, int iBaseSlot, int iTestSlot )
{
	if ( eEquipType == EQUIP_TYPE_CLASS )
	{
		if ( iBaseSlot == LOADOUT_POSITION_MISC )
		{
			return IsMiscSlot( iTestSlot );
		}

		if ( iBaseSlot == LOADOUT_POSITION_BUILDING )
		{
			return IsBuildingSlot( iTestSlot );
		}

		if ( iBaseSlot == LOADOUT_POSITION_TAUNT )
		{
			return IsTauntSlot( iTestSlot );
		}
	}
	else if ( eEquipType == EQUIP_TYPE_ACCOUNT )
	{
		if ( iBaseSlot == ACCOUNT_LOADOUT_POSITION_ACCOUNT1 )
		{
			return IsQuestSlot( iTestSlot );
		}
	}

	return iBaseSlot == iTestSlot;
}

//-----------------------------------------------------------------------------
CTFInventoryManager g_TFInventoryManager;
CInventoryManager *InventoryManager( void )
{
	return &g_TFInventoryManager;
}
CTFInventoryManager *TFInventoryManager( void )
{
	return &g_TFInventoryManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFInventoryManager::CTFInventoryManager( void )

{
}

CTFInventoryManager::~CTFInventoryManager( void )
{
	m_pBaseLoadoutItems.PurgeAndDeleteElements();
	m_ModItems.PurgeAndDeleteElements();
	m_ModItemsBacking.PurgeAndDeleteElements();
	m_LoanerItems.PurgeAndDeleteElements();
	m_LoanerItemsBacking.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFInventoryManager::PostInit( void )
{
	BaseClass::PostInit();
	GenerateBaseItems();
	// NOTE: LoadModItems and LoadLoanerItems are intentionally NOT called here.
	// PostInit runs before the item schema is guaranteed to be fully loaded, so
	// GetItemDefinition() returns null for every defindex and every synthetic item
	// gets silently skipped. They are loaded in PostInitGC instead, where the
	// schema is ready.
}

//-----------------------------------------------------------------------------
// Purpose: Called after CInventoryManager::PostInitGC() runs UpdateLocalInventory()
//          → RequestInventory() → AddSOCacheListener(). At this point the local
//          inventory is registered as a listener. If we have a disk cache from a
//          previous live session, inject it now so the loadout panel is immediately
//          usable without waiting for the webapi round-trip.
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CTFInventoryManager::PostInitGC()
{
	// Base calls UpdateLocalInventory() → RequestInventory() → AddSOCacheListener().
	// The inventory is now registered and ready to receive items.
	BaseClass::PostInitGC();

	// Don't call LoadModItems/LoadLoanerItems here.
	// GetItemDefinition() may not be ready this early even in PostInitGC.
	// WebapiInventoryThink will retry via TryLoadSyntheticItems() once
	// the schema is confirmed ready (non-null definition lookup).

	CTFPlayerInventory *pLocalInv = GetLocalTFInventory();
	if ( pLocalInv && !pLocalInv->RetrievedInventoryFromSteam() )
	{
		pLocalInv->LoadOfflineItemCache();
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Generate & store the base item details for each class & loadout slot
//-----------------------------------------------------------------------------
void CTFInventoryManager::GenerateBaseItems( void )
{
	// Purge our lists and make new
	m_pBaseLoadoutItems.PurgeAndDeleteElements();
	
	// Load a base top level invalid item
	{
		m_pDefaultItem = new CEconItemView;
		m_pDefaultItem->Invalidate();
	}
	//
	const CEconItemSchema::BaseItemDefinitionMap_t& mapItems = GetItemSchema()->GetBaseItemDefinitionMap();
	int iStart = 0;
	for ( int it = iStart; it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
	{
		CEconItemView *pItem = new CEconItemView;
		pItem->Init( mapItems[it]->GetDefinitionIndex(), AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, false );
		m_pBaseLoadoutItems.AddToTail( pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper — allocate a CEconItem and matching CEconItemView for a flat
//          synthetic item. Returns the view (owned by the caller) or nullptr on
//          failure. The backing CEconItem* is stored in vecOwned.
//-----------------------------------------------------------------------------
static CEconItemView *CreateSyntheticItem(
    item_definition_index_t nDef,
    int nQuality, int nLevel,
    eEconItemOrigin eOrigin,
    itemid_t ulID,
    CUtlVector<CEconItem *> &vecOwned )
{
    if ( !GetItemSchema()->GetItemDefinition( nDef ) )
    {
        DevWarning( "[SyntheticItems] def_index %d not in schema, skipping\n", (int)nDef );
        return nullptr;
    }

    CEconItemView *pView = new CEconItemView;
    pView->Init( nDef, nQuality, nLevel, 0 /*no account ID*/ );
    if ( !pView->IsValid() )
    {
        delete pView;
        DevWarning( "[SyntheticItems] Init failed for def_index %d\n", (int)nDef );
        return nullptr;
    }
    pView->SetItemID( ulID );

    // Allocate the backing CEconItem so GetSOCData() works without a live SOCache.
    CEconItem *pEconItem = new CEconItem();
    pEconItem->SetItemID( ulID );
    pEconItem->SetDefinitionIndex( nDef );
    pEconItem->SetQuality( nQuality );
    pEconItem->SetItemLevel( static_cast<uint32>( nLevel ) );
    pEconItem->SetOrigin( eOrigin );
    pEconItem->SetAccountID( 0 );
    vecOwned.AddToTail( pEconItem );

    pView->SetNonSOEconItem( pEconItem );
    return pView;
}

//-----------------------------------------------------------------------------
// Purpose: Deferred schema-ready check and load of synthetic items.
//          Called repeatedly from WebapiInventoryThink until schema is ready.
//          Uses a known defindex (Scattergun = 13) as a canary to test whether
//          GetItemDefinition() is working yet. Once it returns non-null, the
//          schema's defindex->definition map is populated and we can safely
//          call LoadModItems/LoadLoanerItems which depend on it.
//-----------------------------------------------------------------------------
void CTFInventoryManager::TryLoadSyntheticItems()
{
	if ( m_bLoadedSyntheticItems )
		return;

	// Use a known def from the base game (e.g. Scattergun = 13) as a canary.
	// If GetItemDefinition returns non-null, the lookup map is ready.
	if ( !GetItemSchema()->GetItemDefinition( 13 ) )
		return; // schema not ready yet, try again next frame

	LoadModItems();
	LoadLoanerItems();
	m_bLoadedSyntheticItems = true;
}

//-----------------------------------------------------------------------------
// Purpose: Loads scripts/items/mod_items.txt and populates m_ModItems.
//          These are custom items with def_indices only in TF2V's items_game.txt.
//          Format:
//            "mod_items" { "item" { "defindex" "40000"  "quality" "6"  "level" "1" } }
//-----------------------------------------------------------------------------
void CTFInventoryManager::LoadModItems()
{
    // Always purge both together — views hold raw pointers into backing.
    m_ModItems.PurgeAndDeleteElements();
    m_ModItemsBacking.PurgeAndDeleteElements();

    KeyValues *pKV = new KeyValues( "mod_items" );
    if ( !pKV->LoadFromFile( reinterpret_cast<IBaseFileSystem *>( g_pFullFileSystem ),
                             "scripts/items/mod_items.txt", "GAME" ) )
    {
        // File absent or malformed — not an error, mod items are optional.
        pKV->deleteThis();
        return;
    }

    // Each synthetic mod item gets a deterministic ID: base + defindex.
    // IDs are stable across sessions so local_loadout.txt remains valid.

    int nLoaded = 0;
    for ( KeyValues *pItem = pKV->GetFirstTrueSubKey();
          pItem; pItem = pItem->GetNextTrueSubKey() )
    {
        item_definition_index_t nDef =
            static_cast<item_definition_index_t>( pItem->GetInt( "defindex", -1 ) );
        if ( nDef == (item_definition_index_t)-1 )
            continue;

        int nQuality = pItem->GetInt( "quality", AE_UNIQUE );
        int nLevel   = pItem->GetInt( "level",   1 );
        itemid_t ulID = k_ModItemIDBase + static_cast<itemid_t>( nDef );

        CEconItemView *pView = CreateSyntheticItem( nDef, nQuality, nLevel,
            kEconItemOrigin_Invalid, ulID, m_ModItemsBacking );
        if ( pView )
        {
            m_ModItems.AddToTail( pView );
            ++nLoaded;
        }
    }

    pKV->deleteThis();
    DevMsg( "[ModItems] Loaded %d mod items\n", nLoaded );
}

//-----------------------------------------------------------------------------
// Purpose: Loads scripts/items/loaner_items.txt and populates m_LoanerItems.
//          These are real TF2 def_indices issued at normal quality, level 1,
//          and origin kEconItemOrigin_QuestLoanerItem (value 25) so players and
//          tools can clearly identify them as mod-provided stand-ins.
//          Format:
//            "loaner_items" { "item" { "defindex" "133" } }  // Gunboats = 133
//-----------------------------------------------------------------------------
void CTFInventoryManager::LoadLoanerItems()
{
    // Always purge both together — views hold raw pointers into backing.
    m_LoanerItems.PurgeAndDeleteElements();
    m_LoanerItemsBacking.PurgeAndDeleteElements();

    KeyValues *pKV = new KeyValues( "loaner_items" );
    if ( !pKV->LoadFromFile( reinterpret_cast<IBaseFileSystem *>( g_pFullFileSystem ),
                             "scripts/items/loaner_items.txt", "GAME" ) )
    {
        // File absent or malformed — not an error, loaner items are optional.
        pKV->deleteThis();
        return;
    }


    int nLoaded = 0;
    for ( KeyValues *pItem = pKV->GetFirstTrueSubKey();
          pItem; pItem = pItem->GetNextTrueSubKey() )
    {
        item_definition_index_t nDef =
            static_cast<item_definition_index_t>( pItem->GetInt( "defindex", -1 ) );
        if ( nDef == (item_definition_index_t)-1 )
            continue;

        // Loaner items are always: normal quality, level 1, origin QuestLoanerItem.
        // Quality and level are not configurable — the whole point is uniformity.
        itemid_t ulID = k_LoanerItemIDBase + static_cast<itemid_t>( nDef );

        CEconItemView *pView = CreateSyntheticItem( nDef, AE_NORMAL, 1,
            kEconItemOrigin_QuestLoanerItem, ulID, m_LoanerItemsBacking );
        if ( pView )
        {
            m_LoanerItems.AddToTail( pView );
            ++nLoaded;
        }
    }

    pKV->deleteThis();
    DevMsg( "[LoanerItems] Loaded %d loaner items\n", nLoaded );
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFInventoryManager::EquipItemInLoadout( int iClass, int iSlot, itemid_t iItemID )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return false;

	// If they pass in a INVALID_ITEM_ID item id, we're just clearing the loadout slot
	if ( iItemID == INVALID_ITEM_ID )
		return m_LocalInventory.ClearLoadoutSlot( iClass, iSlot );

	CEconItemView *pItem = m_LocalInventory.GetInventoryItemByItemID( iItemID );
	if ( !pItem )
		return false;

	// We check for validity on the GC when we equip items, but we can't really trust anyone
	// and so we check here as well.
	if ( !AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot(iClass), iSlot ) )
	{
		return false;
	}

	if ( !pItem->GetStaticData()->CanBeUsedByClass( iClass ) )
	{
		return false;
	}

	// Equip the new item
	UpdateInventoryEquippedState( &m_LocalInventory, iItemID, iClass, iSlot );

	// TODO: Prediction
	// Item has been moved, so update our loadout.
	//m_LoadoutItems[iClass][iSlot] = iItemID;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Fills out pList with all inventory items that could fit into the specified loadout slot for a given class
//-----------------------------------------------------------------------------
int	CTFInventoryManager::GetAllUsableItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView*> *pList )
{
	bool bIsAccountIndex = iClass == GEconItemSchema().GetAccountIndex();
	if ( bIsAccountIndex )
	{
		Assert( IsQuestSlot( iSlot ) );
	}
	else
	{
		Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT );
		Assert( iSlot >= -1 && iSlot < CLASS_LOADOUT_POSITION_COUNT );
	}

	int iCount = m_LocalInventory.GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItemView *pItem = m_LocalInventory.GetItem(i);
		CTFItemDefinition *pItemData = pItem->GetStaticData();

		if ( bIsAccountIndex != ( pItemData->GetEquipType() == EEquipType_t::EQUIP_TYPE_ACCOUNT ) )
			continue;

		if ( !bIsAccountIndex && !pItemData->CanBeUsedByClass(iClass) )
			continue;

		// Passing in iSlot of -1 finds all items usable by the class
		if ( iSlot >= 0 && pItem->GetStaticData()->GetLoadoutSlot( iClass ) != iSlot )
			continue;

		// Ignore unpack'd items
		if ( IsUnacknowledged( pItem->GetInventoryPosition() ) )
			continue;

		pList->AddToTail( m_LocalInventory.GetItem(i) );
	}

	return pList->Count();
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CTFInventoryManager::GetItemInLoadoutForClass( int iClass, int iSlot, CSteamID *pID )
{
#ifdef CLIENT_DLL
	CSteamID localSteamID;
	if ( !pID )
	{
		// If they didn't specify a steamID, use the local player
		if ( !steamapicontext || !steamapicontext->SteamUser() )
			return NULL;

		localSteamID = steamapicontext->SteamUser()->GetSteamID();
		pID = &localSteamID;
	}
#endif

	CTFPlayerInventory *pInv = GetInventoryForPlayer( *pID );
	if ( !pInv )
		return GetBaseItemForClass( iClass, iSlot );

	return pInv->GetItemInLoadout( iClass, iSlot );
}

CEconItemView *CTFInventoryManager::GetItemInLoadoutForAccount( int iSlot, CSteamID *pID )
{
#ifdef CLIENT_DLL
	if ( !pID )
	{
		// If they didn't specify a steamID, use the local player
		if ( !steamapicontext || !steamapicontext->SteamUser() )
			return NULL;

		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
		pID = &localSteamID;
	}
#endif

	CTFPlayerInventory *pInv = GetInventoryForPlayer( *pID );
	if ( !pInv )
		return NULL;

	return pInv->GetItemInLoadout( GEconItemSchema().GetAccountIndex(), iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory *CTFInventoryManager::GetInventoryForPlayer( const CSteamID &playerID )
{
	for ( int i = 0; i < m_pInventories.Count(); i++ )
	{
		if ( m_pInventories[i].pInventory->GetOwner() != playerID )
			continue;

		return assert_cast<CTFPlayerInventory*>( m_pInventories[i].pInventory );
	}

	return NULL;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFInventoryManager::GetNumItemPickedUpItems( void )
{
	int iResult = 0;
	int iCount = m_LocalInventory.GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( IsUnacknowledged( m_LocalInventory.GetItem(i)->GetInventoryPosition() ) && !m_LocalInventory.GetItem(i)->GetStaticData()->IsHidden() )
		{
			++iResult;
		}
	}
	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFInventoryManager::ShowItemsPickedUp( bool bForce, bool bReturnToGame, bool bNoPanel )
{
	// don't show new items in training, unless forced to do so
	// i.e. purchased something or traded...
	if ( bForce == false && TFGameRules() && ( TFGameRules()->IsInTraining() || TFGameRules()->IsCompetitiveMode() ) )
	{
		return false;
	}

	// Don't bring it up if we're already browsing something in the gameUI
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	if ( !bForce && vgui::ipanel()->IsVisible( gameuiPanel ) )
		return false;

	CUtlVector<CEconItemView*> aItemsFound;

	// Go through the root inventory and find any items that are in the "found" position
	int iCount = m_LocalInventory.GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItemView *pTmp = m_LocalInventory.GetItem(i);
		if ( !pTmp )
			continue;

		if ( pTmp->GetStaticData()->IsHidden() )
			continue;

		uint32 iPosition = pTmp->GetInventoryPosition();
		if ( IsUnacknowledged(iPosition) == false )
			continue;
		if ( GetBackpackPositionFromBackend(iPosition) != 0 )
			continue;

		// Now make sure we haven't got a clientside saved ack for this item.
		// This makes sure we don't show multiple pickups for items that we've found,
		// but haven't been able to move out of unack'd position due to the GC being unavailable.
		if ( HasBeenAckedByClient( pTmp ) )
			continue;

		aItemsFound.AddToTail( pTmp );
	}

	if ( !aItemsFound.Count() )
		return CheckForRoomAndForceDiscard();

	// We're not forcing the player to make room yet. Just show the pickup panel.
	NotificationQueue_Remove( &CEconNotification_HasNewItems::IsNotificationType );
	CItemPickupPanel *pItemPanel = bNoPanel ? NULL : EconUI()->OpenItemPickupPanel();

	if ( pItemPanel )
	{
		pItemPanel->SetReturnToGame( bReturnToGame );
	}

	// Only acknowledge items if there is no panel
	// Panel will make calls to acknowledge items itself 
	for ( int i = 0; i < aItemsFound.Count(); i++ )
	{
		if ( pItemPanel )
		{
			pItemPanel->AddItem( aItemsFound[i] );
		}
		else 
		{
			AcknowledgeItem( aItemsFound[i] );
		}
	}

	if ( pItemPanel )
	{
		pItemPanel->MoveToFront();
	}
	else
	{
		SaveAckFile();
	}
	
	aItemsFound.Purge();
	return true;
}

//-----------------------------------------------------------------------------
void CTFInventoryManager::AcknowledgeItem( CEconItemView *pItem, bool bMoveToBackpack /* = true */ )
{
	SetAckedByClient( pItem );

	int iMethod = GetUnacknowledgedReason( pItem->GetInventoryPosition() ) - 1;
	if ( iMethod >= ARRAYSIZE( g_pszItemPickupMethodStringsUnloc ) || iMethod < 0 )
		iMethod = 0;
	EconUI()->Gamestats_ItemTransaction( IE_ITEM_RECEIVED, pItem, g_pszItemPickupMethodStringsUnloc[iMethod] );

	if ( iMethod+1 == UNACK_ITEM_PREVIEW_ITEM_PURCHASED )
	{
		// If we found a purchased preview item, we want to refresh the store view to remove the discount indicator.
		CStorePage* pStorePage = dynamic_cast<CStorePage*>( EconUI()->GetStorePanel()->GetActivePage() );
		if ( pStorePage )
		{
			pStorePage->UpdateModelPanels();
		}
	}

	// Move it to the first empty backpack position.
	if ( bMoveToBackpack )
	{
		SetItemBackpackPosition( pItem, 0, false, true );
	}
}

void CTFInventoryManager::Update( float frametime )
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 );
	m_LocalInventory.UpdateWeaponSkinRequest();

	BaseClass::Update( frametime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFInventoryManager::ShowItemsCrafted( CUtlVector<itemid_t> *vecCraftedIndices )
{
	CUtlVector<CEconItemView*> aItemsFound;
	FOR_EACH_VEC( *vecCraftedIndices, i )
	{
		CEconItemView *pItem = m_LocalInventory.GetInventoryItemByItemID( vecCraftedIndices->Element(i) );
		if ( pItem )
		{
			// Now make sure we haven't got a clientside saved ack for this item.
			// This makes sure we don't show multiple pickups for items that we've found,
			// but haven't been able to move out of unack'd position due to the GC being unavailable.
			if ( !HasBeenAckedByClient( pItem ) )
			{
				aItemsFound.AddToTail( pItem );
			}
		}
	}

	if ( !aItemsFound.Count() )
		return;

	NotificationQueue_Remove( &CEconNotification_HasNewItems::IsNotificationType );
	CItemPickupPanel *pItemPanel = EconUI()->OpenItemPickupPanel();
	for ( int i = 0; i < aItemsFound.Count(); i++ )
	{
		pItemPanel->AddItem( aItemsFound[i] );

		SetAckedByClient( aItemsFound[i] );

#ifdef CLIENT_DLL
		EconUI()->Gamestats_ItemTransaction( IE_ITEM_RECEIVED, aItemsFound[i], "crafted" );
#endif

		// Then move it to the first empty backpack position
		SetItemBackpackPosition( aItemsFound[i], 0, false, true );
	}
	SaveAckFile();
	pItemPanel->MoveToFront();

	aItemsFound.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFInventoryManager::CheckForRoomAndForceDiscard( void )
{
	// Go through the inventory and attempt to move any items outside the backpack into valid positions.
	// Remember the first item that we failed to move, so we can force a discard later.
	CEconItemView *pItem = NULL;
	const int iMaxItems = m_LocalInventory.GetMaxItemCount();
	int iCount = m_LocalInventory.GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItemView *pTmp = m_LocalInventory.GetItem(i);
		if ( !pTmp )
			continue;

		if ( pTmp->GetStaticData()->IsHidden() )
			continue;

		uint32 iPosition = pTmp->GetInventoryPosition();
		if ( IsUnacknowledged(iPosition) || GetBackpackPositionFromBackend(iPosition) > iMaxItems )
		{
			if ( !SetItemBackpackPosition( pTmp, 0, false, false ) )
			{
				pItem = pTmp;
				break;
			}
		}
	}

	// If we're not over the limit, we're done.
	if ( ( iCount - m_iPredictedDiscards ) <= iMaxItems )
		return false;

	if ( !pItem )
		return false;

	// We're forcing the player to make room for items he's found. Bring up that panel with the first item over the limit.
	CItemDiscardPanel *pDiscardPanel = EconUI()->OpenItemDiscardPanel();
	pDiscardPanel->SetItem( pItem );
	return true;
}
#endif

bool CTFInventoryManager::SlotContainsBaseItems( EEquipType_t eType, int iSlot )
{
	Assert( eType != EEquipType_t::EQUIP_TYPE_INVALID );

	if ( eType == EEquipType_t::EQUIP_TYPE_ACCOUNT )
	{
		return !IsQuestSlot( iSlot );
	}

	// Passtime gun
	if ( (iSlot == LOADOUT_POSITION_UTILITY) && TFGameRules() && TFGameRules()->IsPasstimeMode() )
	{
		return true;
	}

	// Halloween spellbook
	if ( iSlot == LOADOUT_POSITION_ACTION )
	{
		if ( TFGameRules() && TFGameRules()->IsUsingSpells() )
			return true;

		if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
			return true;
	}
	// Normal game
	return iSlot < LOADOUT_POSITION_HEAD;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the item data for the base item in the loadout slot for a given class
//-----------------------------------------------------------------------------
CEconItemView *CTFInventoryManager::GetBaseItemForClass( int iClass, int iSlot )
{
	// There is no base account item
	if ( iClass == GEconItemSchema().GetAccountIndex() )
		return m_pDefaultItem;

	if ( !HushAsserts() )
	{
		AssertMsg( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT, "Invalid TF_CLASS_: %d", iClass );
	}
	Assert( iSlot >= 0 && iSlot < CLASS_LOADOUT_POSITION_COUNT );

	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_CLASS_COUNT || iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return m_pDefaultItem;

	// Halloween spellbook
	if ( iSlot == LOADOUT_POSITION_ACTION )
	{
		CUtlVector< item_definition_index_t > stockActionItemDefIndices;

		static CSchemaItemDefHandle pItemDef_SpellBook( "TF_WEAPON_SPELLBOOK" );
		if ( TFGameRules() && TFGameRules()->IsUsingSpells() && pItemDef_SpellBook )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_SpellBook->GetDefinitionIndex() );
		}

		static CSchemaItemDefHandle pItemDef_GrapplingHook( "TF_WEAPON_GRAPPLINGHOOK" );
		if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() && pItemDef_GrapplingHook )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_GrapplingHook->GetDefinitionIndex() );
		}

		static CSchemaItemDefHandle pItemDef_MvMCanteen( "Default Power Up Canteen (MvM)" );
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pItemDef_MvMCanteen )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_MvMCanteen->GetDefinitionIndex() );
		}
		
		// Traverse List
		for ( CEconItemView *pActionItem : m_pBaseLoadoutItems )
		{
			for ( item_definition_index_t defIndex : stockActionItemDefIndices )
			{
				if ( pActionItem->GetItemDefIndex() == defIndex )
					return pActionItem;
			}
		}
	}

	if ( iSlot >= LOADOUT_POSITION_HEAD )
		return m_pDefaultItem;

	// Traverse List
	FOR_EACH_VEC( m_pBaseLoadoutItems, iItem )
	{
		if ( m_pBaseLoadoutItems[iItem]->GetItemDefinition()->GetLoadoutSlot( iClass ) == iSlot )
			return m_pBaseLoadoutItems[iItem];
	}

	return m_pDefaultItem;
}

//-----------------------------------------------------------------------------
// Purpose: Fills out the vector with the sets that are currently active on the specified player & class
//-----------------------------------------------------------------------------
void CTFInventoryManager::GetActiveSets( CUtlVector<const CEconItemSetDefinition *> *pItemSets, CSteamID steamIDForPlayer, int iClass )
{
	pItemSets->Purge();

	CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
	if ( !pSchema )
		return;

	// Loop through all the items we have equipped, and build a list of only set items
	// Accumulate a list of our equipped set items.
	CUtlVector<CEconItemView*> equippedSetItems;
	for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
	{
		CEconItemView *pItem = NULL;

#ifdef GAME_DLL
		// On the server we need to look at what the player actually has equipped
		// because they might be on a tournament server that's using an item_whitelist
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( steamIDForPlayer ) );
		if ( pPlayer && pPlayer->Inventory() )
		{
			pItem = pPlayer->GetEquippedItemForLoadoutSlot( i );
		}
#else
		pItem = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i, &steamIDForPlayer );
#endif		

		if ( !pItem )
			continue;

		CEconItemDefinition* pData = pItem->GetStaticData();
		if ( !pData )
			continue;

		// Ignore items that don't have set bonuses.
		if ( !pData->GetItemSetDefinition() )
			continue;

		// Make sure this item isn't failing a Holiday restriction before giving out the set bonus!
		if ( pData->GetHolidayRestriction() )
		{
			int iHolidayRestriction = UTIL_GetHolidayForString( pData->GetHolidayRestriction() );
			if ( iHolidayRestriction != kHoliday_None && (!TFGameRules() || !TFGameRules()->IsHolidayActive( iHolidayRestriction )) )
				continue;
		}

		equippedSetItems.AddToTail( pItem );
	}

	// Find out which sets to apply.
	CUtlVector<const char*> testedSets;
	for ( int inv = 0; inv < equippedSetItems.Count(); inv++ )
	{
		CEconItemView *pItem = equippedSetItems[inv];
		if ( !pItem )
			continue;

		const CEconItemSetDefinition *pItemSet = pItem->GetStaticData()->GetItemSetDefinition();
		if ( !pItemSet )
			continue;

		if ( testedSets.HasElement( pItemSet->m_strName ) )
			continue; // Don't try to apply set bonuses we have already tested.

		testedSets.AddToTail( pItemSet->m_strName );

		// Count how much of this set we have equipped.
		int iSetItemsEquipped = 0;
		for ( int i=0; i<pItemSet->m_iItemDefs.Count(); i++ )
		{
			unsigned int iIndex = pItemSet->m_iItemDefs[i];

			for ( int j=0; j<equippedSetItems.Count(); j++ )
			{
				const CEconItemView *pTestItem = equippedSetItems[j];
				const item_definition_index_t unEquippedItemSetDefIndex = pTestItem->GetItemDefinition()->GetSetItemRemap();

				if ( iIndex == unEquippedItemSetDefIndex )
				{
					iSetItemsEquipped++;
					break;
				}
			}
		}

		// Note: this logic will break if we ever have sets that have misc-slot items where we can have multiple items
		// of the same type with different remaps equipped.
		if ( iSetItemsEquipped == pItemSet->m_iItemDefs.Count() )
		{
			// The entire set is equipped. 
			pItemSets->AddToTail( pItemSet );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: We're generating a base item. We need to add the game-specific keys to the criteria so that it'll find the right base item.
//-----------------------------------------------------------------------------
void CTFInventoryManager::AddBaseItemCriteria( baseitemcriteria_t *pCriteria, CItemSelectionCriteria *pSelectionCriteria )
{
	pSelectionCriteria->BAddCondition( "used_by_classes", k_EOperator_Subkey_Contains, ItemSystem()->GetItemSchema()->GetClassUsabilityStrings()[pCriteria->iClass], true );
	pSelectionCriteria->BAddCondition( "item_slot", k_EOperator_String_EQ, ItemSystem()->GetItemSchema()->GetLoadoutStrings( EEquipType_t::EQUIP_TYPE_CLASS )[pCriteria->iSlot], true );
}


//=======================================================================================================================
// TF PLAYER INVENTORY
//=======================================================================================================================
// Inventory Less function.
// Used to sort the inventory items into their positions.
class CTFInventoryListLess
{
public:
	bool Less( const CEconItemView &src1, const CEconItemView &src2, void *pCtx )
	{
		if ( src1.GetInventoryPosition() > src2.GetInventoryPosition() )
			return true;

		return false;
	}
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory::CTFPlayerInventory()
{
	m_aInventoryItems.SetLessContext( this );
#ifdef CLIENT_DLL
	for ( int i = 0; i < TF_TEAM_COUNT; ++i ) 
		m_CachedBaseTextureLowRes[ i ].SetLessFunc( DefLessFunc( itemid_t ) );

	memset(m_ActivePreset, LOADOUT_SLOT_USE_BASE_ITEM, sizeof(m_ActivePreset));
	memset(m_PresetItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof(m_PresetItems));
#endif

	memset( m_LoadoutItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof( m_LoadoutItems ) );
	memset( m_AccountLoadoutItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof( m_AccountLoadoutItems ) );
	ClearClassLoadoutChangeTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory::~CTFPlayerInventory()
{
#ifdef CLIENT_DLL
	for ( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		FOR_EACH_MAP_FAST( m_CachedBaseTextureLowRes[ iTeam ], i )
			m_CachedBaseTextureLowRes[ iTeam ][ i ]->Release();
		m_CachedBaseTextureLowRes[ iTeam ].RemoveAll();
	}
	m_vecOfflineItems.PurgeAndDeleteElements();
	m_vecModItems.PurgeAndDeleteElements();
	m_vecLoanerItems.PurgeAndDeleteElements();
#endif
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::CheckSaxtonMaskAchievement( const CEconItem *pEconItem )
{
	if ( pEconItem )
	{
		if ( pEconItem->GetDefinitionIndex() == 277 && pEconItem->GetOrigin() == kEconItemOrigin_Crafted )	// Saxton mask is item index 277
		{
			g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HALLOWEEN_CRAFT_SAXTON_MASK );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateCachedServerLoadoutItems()
{
	V_memcpy( m_CachedServerLoadoutItems, m_LoadoutItems, sizeof( itemid_t ) * ARRAYSIZE( m_CachedServerLoadoutItems ) * ARRAYSIZE( m_CachedServerLoadoutItems[0] ) );
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateRealTFLoadoutItems()
{
	V_memcpy( m_RealTFLoadoutItems, m_LoadoutItems, sizeof( itemid_t ) * ARRAYSIZE( m_RealTFLoadoutItems ) * ARRAYSIZE( m_RealTFLoadoutItems[0] ) );
}

void CTFPlayerInventory::LoadLocalLoadout()
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return;

	if (GetOwner() != steamapicontext->SteamUser()->GetSteamID())
		return;

	if (!g_pFullFileSystem) {
		return;
	}

	KeyValues *pLoadoutKV = new KeyValues("local_loadout");
	if (!pLoadoutKV->LoadFromFile(g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD"))
	{
		SaveLocalLoadout( true, true );

		if ( !pLoadoutKV->LoadFromFile( g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD" ) )
		{
			Warning( "Unable to parse local_loadout.txt into keyvalues.\n" );
			return;
		}
	}

	KeyValues *pActivePresetKV = pLoadoutKV->FindKey("active_preset");
	if (pActivePresetKV) 
	{
		for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
		{
			const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];
			int activePreset = pActivePresetKV->GetInt(pszClassName);
			m_ActivePreset[iClass] = activePreset;
		}
	}

	int numPresets = static_cast<int>(GetItemSchema()->GetNumAllowedItemPresets());
	for (int iPreset = 0; iPreset < numPresets; ++iPreset)
	{
		char szPreset[256];
		V_snprintf(szPreset, sizeof(szPreset), "%i", iPreset);
		KeyValues* pPresetKV = pLoadoutKV->FindKey(szPreset);
		if (!pPresetKV)
			continue;

		FOR_EACH_TRUE_SUBKEY(pPresetKV, pClassKey)
		{
			const char *pszClassName = pClassKey->GetName();
			const int iClass = GetClassIndexFromString(pszClassName, TF_CLASS_COUNT_ALL);

			FOR_EACH_SUBKEY(pClassKey, pLoadoutEntry)
			{
				const int iSlot = V_atoi(pLoadoutEntry->GetName());
				const itemid_t uItemId = pLoadoutEntry->GetUint64();

				m_PresetItems[iPreset][iClass][iSlot] = uItemId;

				if (iPreset == m_ActivePreset[iClass]) {
					m_LoadoutItems[iClass][iSlot] = uItemId;

					CEconItemView *pItem = GetInventoryItemByItemID(uItemId);
					if (pItem) {
						pItem->GetSOCData()->Equip(iClass, iSlot);
					}
				}
			}
		}
	}

	pLoadoutKV->deleteThis();

	GTFGCClientSystem()->LocalInventoryChanged();
}

//-----------------------------------------------------------------------------
// Purpose: If we are in mod mode, we track loadout changes locally.
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SaveLocalLoadout( bool bReset, bool bDefaultToGC )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return;

	if (GetOwner() != steamapicontext->SteamUser()->GetSteamID())
		return;

	if (!g_pFullFileSystem) {
		return;
	}

	KeyValues *pLoadoutKV = new KeyValues("local_loadout");

	KeyValues *pActivePresetKV = new KeyValues("active_preset");
	for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
	{
		const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];
		pActivePresetKV->SetInt(pszClassName, m_ActivePreset[iClass]);
	}
	pLoadoutKV->AddSubKey(pActivePresetKV);

	int numPresets = static_cast<int>(GetItemSchema()->GetNumAllowedItemPresets());
	for (int iPreset = 0; iPreset < numPresets; ++iPreset)
	{
		char szPreset[256];
		V_snprintf(szPreset, sizeof(szPreset), "%i", iPreset);
		KeyValues *pPresetKV = new KeyValues(szPreset);
		pLoadoutKV->AddSubKey(pPresetKV);

		for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
		{
			const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];

			KeyValues *pClassKV = new KeyValues(pszClassName);
			pPresetKV->AddSubKey(pClassKV);

			for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
			{
				char szSlot[256];
				V_snprintf(szSlot, sizeof(szSlot), "%i", iSlot);

				itemid_t uItemId = m_PresetItems[iPreset][iClass][iSlot];
				//itemid_t uItemId = m_LoadoutItems[iClass][iSlot];
				if (bReset) {
					uItemId = ( bDefaultToGC && iPreset == 0 ) ? m_RealTFLoadoutItems[iClass][iSlot] : 0;
				}

				pClassKV->SetUint64(szSlot, uItemId);
			}
		}
	}

	pLoadoutKV->SaveToFile(g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD");

	pLoadoutKV->deleteThis();
}


#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: If we are in mod mode, we track loadout changes locally.
//-----------------------------------------------------------------------------
void CTFPlayerInventory::EquipLocal(uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot)
{
	// These interactions normally result from a round-trip with the GC.
	// We will never get those messages, so we do everything locally.

	// Unequip whatever was previously in the slot.
	{
		itemid_t ulPreviousItem = m_LoadoutItems[unClass][unSlot];
		CEconItemView *pPreviousItem = GetInventoryItemByItemID(ulPreviousItem);
		if (pPreviousItem) {
			pPreviousItem->GetSOCData()->UnequipFromClass(unClass);
		}
	}

	// Equip the new item and add it to our loadout.
	CEconItemView *pItem = GetInventoryItemByItemID(ulItemID);
	if ( pItem )
	{
		pItem->GetSOCData()->Equip(unClass, unSlot);
	}

	m_LoadoutItems[unClass][unSlot] = ulItemID;

#ifdef CLIENT_DLL
	int activePreset = m_ActivePreset[unClass];
	m_PresetItems[activePreset][unClass][unSlot] = ulItemID;

	GTFGCClientSystem()->LocalInventoryChanged();
#endif
}

void CTFPlayerInventory::UnequipLocal(uint64 ulItemID)
{
	for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
	{
		for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
		{
			if (m_LoadoutItems[iClass][iSlot] == ulItemID) {
				m_LoadoutItems[iClass][iSlot] = 0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::SOUpdated( steamIDOwner, pObject, eEvent );

#ifdef CLIENT_DLL
	if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;

	// Clear out any predicted backpack slots when items move into them
	CEconItem *pEconItem = (CEconItem *)pObject;
	if ( eEvent == eSOCacheEvent_Incremental )
	{
		EconUI()->GetBackpackPanel()->MarkItemIDDirty( pEconItem->GetItemID() );
	}
	int iBackpackPos = TFInventoryManager()->GetBackpackPositionFromBackend( pEconItem->GetInventoryToken() );
	TFInventoryManager()->PredictedBackpackPosFilled( iBackpackPos );
#endif // CLIENT_DLL
}

void CTFPlayerInventory::OnHasNewItems()
{
	BaseClass::OnHasNewItems();
#ifdef CLIENT_DLL
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
		return;

	NotificationQueue_Remove( &CEconNotification_HasNewItems::IsNotificationType );
	CEconNotification_HasNewItems *pNotification = new CEconNotification_HasNewItems();
	pNotification->SetText( "TF_HasNewItems" );
	pNotification->SetLifetime( 7.0f );
	NotificationQueue_Add( pNotification );	
#endif
}

void CTFPlayerInventory::OnHasNewQuest()
{
#ifdef CLIENT_DLL

#endif
}

#ifdef _DEBUG
#ifdef CLIENT_DLL
CON_COMMAND( cl_newitem_test, "Tests the new item ui notification." )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	CEconNotification_HasNewItems *pNotification = new CEconNotification_HasNewItems();
	pNotification->SetText( "TF_HasNewItems" );
	pNotification->SetLifetime( 7.0f );
	NotificationQueue_Add( pNotification );
}
#endif
#endif // _DEBUG

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ValidateInventoryPositions( void )
{
	BaseClass::ValidateInventoryPositions();

#ifdef CLIENT_DLL
	bool bHasNewItems = false;
	const int iMaxItems = GetMaxItemCount();
	// First, check for duplicate positions
	int iCount = m_aInventoryItems.Count();
	for ( int i = iCount-1; i >= 0; i-- )
	{
		CEconItemView *pEconItemView = &m_aInventoryItems[i];

		CheckSaxtonMaskAchievement( pEconItemView->GetSOCData() );

		uint32 iPosition = pEconItemView->GetInventoryPosition();

		// Waiting to be acknowledged?
		if ( IsUnacknowledged(iPosition) )
		{
			if ( !pEconItemView->GetStaticData()->IsHidden() )
			{
				bHasNewItems = true;
			}
			continue;
		}

		bool bInvalidSlot = false;
		if ( !IsNewPositionFormat(iPosition) )
		{
			ConvertOldFormatInventoryToNew();
			break;
		}

		// Inside the backpack?
		if ( i < (iCount-1) )
		{
			// We're not in an invalid slot yet. But if we're in the same position as another item, we should be moved too.
			int iPos1 = TFInventoryManager()->GetBackpackPositionFromBackend(iPosition);
			int iPos2 = TFInventoryManager()->GetBackpackPositionFromBackend(m_aInventoryItems[i+1].GetInventoryPosition());
			if ( iPos1 == iPos2 )
			{
				Warning("WARNING: Found item in a duplicate backpack position. Moving to the backpack end.\n" );
				bInvalidSlot = true;
			}
		}

		// Make sure it's not outside the backpack extents
		if ( !bInvalidSlot )
		{
			bInvalidSlot = (TFInventoryManager()->GetBackpackPositionFromBackend(iPosition) > iMaxItems);
		}

		if ( bInvalidSlot  )
		{
			// The item is NOT hidden and is in an invalid slot. Move it back to the backpack.
			if ( pEconItemView->GetItemDefinition() && !pEconItemView->GetItemDefinition()->IsHidden() && !TFInventoryManager()->SetItemBackpackPosition( pEconItemView, 0, true ) )
			{
				// We failed to move it to the backpack, because the player has no room.
				// Force them to "refind" the item, which will make them throw something out.
				TFInventoryManager()->UpdateInventoryPosition( this, pEconItemView->GetItemID(), GetUnacknowledgedPositionFor(UNACK_ITEM_DROPPED) );
			}
		}

		// Make sure it isn't equipped by any invalid classes.
		for ( int j = TF_FIRST_NORMAL_CLASS; j <= TF_LAST_NORMAL_CLASS; j++ )
		{
			if ( !pEconItemView->GetStaticData()->CanBeUsedByClass( j ) &&
				 pEconItemView->IsEquippedForClass( j ) )
			{
				// Unequip this item from this class.
				InventoryManager()->UpdateInventoryEquippedState( this, INVALID_ITEM_ID, j, pEconItemView->GetEquippedPositionForClass( j ) );
			}
		}
	}

	if ( bHasNewItems )
	{
		OnHasNewItems();
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ConvertOldFormatInventoryToNew( void )
{
	uint32 iBackpackPos = 1;

	// Loop through all items in the inventory. Move them all to the backpack, and in order.
	int iCount = m_aInventoryItems.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		uint32 iPosition = m_aInventoryItems[i].GetInventoryPosition();

		// Waiting to be acknowledged?
		if ( IsUnacknowledged(iPosition) )
			continue;

		TFInventoryManager()->SetItemBackpackPosition( &m_aInventoryItems[i], iBackpackPos, true );
		iBackpackPos++;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Creates a script item and associates it with this econ item
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::SOCreated( steamIDOwner, pObject, eEvent );

	if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;

	CEconItem *pEconItem = (CEconItem *)pObject;
 //	CEconItem *pEconItem = assert_cast<CEconItem*>( pObject );

#ifdef CLIENT_DLL
	if ( InventoryManager()->GetLocalInventory() == this && GetOwner() == steamIDOwner )
	{
		if ( pObject->GetTypeID() == CEconItem::k_nTypeID )
		{
			CheckSaxtonMaskAchievement( (CEconItem*)pObject );
		}
	}

//	CSteamID ownerSteamID( pEconItem->GetAccountID(), GetUniverse(), k_EAccountTypeIndividual );
//	if ( ownerSteamID == ClientSteamContext().GetLocalPlayerSteamID() )
//	{
//		CheckSaxtonMaskAchievement( pEconItem );
//	}

	static CSchemaItemDefHandle pItemDef_HardyLaurel( "The Hardy Laurel" );
	if ( pEconItem->GetItemDefinition() == pItemDef_HardyLaurel )
	{
		if ( TFSharedContentManager() )
		{
			TFSharedContentManager()->OfferSharedVision( TF_VISION_FILTER_ROME, pEconItem->GetAccountID() );
		}
	}
 #else
	// Summer 2015 Operation Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_Summer2015Operation( "Activated Summer 2015 Operation Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_Summer2015Operation )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_SUMMER2015 );
		}
	}

	// Invasion Community Update Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_InvasionPass( "Activated Invasion Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_InvasionPass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_INVASION );
		}
	}

	// Halloween Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_HalloweenPass( "Activated Halloween Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_HalloweenPass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_HALLOWEEN );
		}
	}

	// Winter2016 Pass so players can display the stamp
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_Winter2016Pass( "Activated Operation Tough Break Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_Winter2016Pass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_WINTER2016 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::DumpInventoryToConsole( bool bRoot )
{
	if ( bRoot )
	{
		Msg("========================================\n");
#ifdef CLIENT_DLL
		Msg("(CLIENT) Inventory:\n");
#else
		Msg("(SERVER) Inventory for account (%d):\n", m_OwnerID.GetAccountID() );
#endif
		Msg("  Version: %llu:\n", m_pSOCache ? m_pSOCache->GetVersion() : -1 );
	}

	int iCount = m_aInventoryItems.Count();
	Msg("   Num items: %d\n", iCount );
	for ( int i = 0; i < iCount; i++ )
	{
		Msg("      %s (ID %llu) at backpack slot %d\n", m_aInventoryItems[i].GetStaticData()->GetDefinitionName(), m_aInventoryItems[i].GetItemID(), TFInventoryManager()->GetBackpackPositionFromBackend( m_aInventoryItems[i].GetInventoryPosition() ) );

		int iEquipped = 0;
		for ( equipped_class_t eq = TF_FIRST_NORMAL_CLASS; eq < TF_LAST_NORMAL_CLASS; eq++ )
		{
			if ( m_aInventoryItems[i].IsEquippedForClass( eq ) )
			{
				if ( iEquipped == 0 )
				{
					iEquipped++;
					Msg("         -> EQUIPPED: ");
				}
				Msg("%s ", g_aPlayerClassNames_NonLocalized[eq] );
			}
		}

		if ( iEquipped )
		{
			Msg("\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ClearClassLoadoutChangeTracking( void )
{
	memset(m_bLoadoutChanged,0, sizeof(m_bLoadoutChanged));
#ifdef CLIENT_DLL
	UpdateCachedServerLoadoutItems();
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Find the low res 
//-----------------------------------------------------------------------------
ITexture *CTFPlayerInventory::GetWeaponSkinBaseLowRes( itemid_t nItemId, int iTeam ) const
{
	Assert( iTeam >= 0 && iTeam < TF_TEAM_COUNT );
	int index = m_CachedBaseTextureLowRes[ iTeam ].Find( nItemId );
	if ( index == m_CachedBaseTextureLowRes[ iTeam ].InvalidIndex() )
		return NULL;

	return m_CachedBaseTextureLowRes[ iTeam ][ index ];
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Find the first item in the local user's inventory with the given def index
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerInventory::GetFirstItemOfItemDef( item_definition_index_t nDefIndex, CPlayerInventory* pInventory )
{
#ifdef CLIENT_DLL
	if ( pInventory == NULL )
	{
		pInventory = InventoryManager()->GetLocalInventory();
	}
#endif

	if ( !pInventory )
		return NULL;

	for ( int i = 0; i < pInventory->GetItemCount(); i++ )
	{
		CEconItemView *pItem = pInventory->GetItem(i);
		if ( pItem->GetItemDefIndex() == nDefIndex )
		{
			return pItem;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the item in the specified loadout slot for a given class
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerInventory::GetItemInLoadout( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return NULL;

	if ( iClass == GEconItemSchema().GetAccountIndex() )
	{
		return GetInventoryItemByItemID( m_AccountLoadoutItems[ iSlot ] );
	}
	else
	{
		if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS  )
			return NULL;

		// If we don't have an item in the loadout at that slot, we return the base item
		if ( m_LoadoutItems[iClass][iSlot] != LOADOUT_SLOT_USE_BASE_ITEM )
		{
			CEconItemView *pItem = GetInventoryItemByItemID( m_LoadoutItems[iClass][iSlot] );

			// To protect against users lying to the backend about the position of their items,
			// we need to validate their position on the server when we retrieve them.
			if ( pItem && AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot( iClass ), iSlot ) )
				return pItem;
		}
	}

	return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
}

#ifdef CLIENT_DLL
CEconItemView *CTFPlayerInventory::GetCacheServerItemInLoadout( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return NULL;
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
		return NULL;

	// If we don't have an item in the loadout at that slot, we return the base item
	if ( m_CachedServerLoadoutItems[iClass][iSlot] != LOADOUT_SLOT_USE_BASE_ITEM )
	{
		CEconItemView *pItem = GetInventoryItemByItemID( m_CachedServerLoadoutItems[iClass][iSlot] );

		// To protect against users lying to the backend about the position of their items,
		// we need to validate their position on the server when we retrieve them.
		if ( pItem && AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot( iClass ), iSlot ) )
			return pItem;
	}

	return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Builds a base64-encoded CMsgSOCacheSubscribed blob from the offline
//          item cache. The server passes this directly to AddLocalSOCache(),
//          skipping the webapi validation round-trip entirely.
//          Only valid when IsOfflineCacheActive() is true.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Merges mod and loaner items from CTFInventoryManager into this
//          player's inventory. Called at the end of both LoadOfflineItemCache()
//          and SOCacheSubscribed() after the base inventory is populated.
//          Safe to call multiple times — skips items already present by ID.
//          Mod items get a unique ID per defindex (k_ModItemIDBase + defindex).
//          Loaner items get a unique ID per defindex (k_LoanerItemIDBase + defindex).
//          Both types use SetNonSOEconItem so GetSOCData() works without a SOCache.
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CTFPlayerInventory::InjectModAndLoanerItems()
{
    // ── Step 1: Remove stale synthetic views from m_aInventoryItems FIRST ──
    // Must happen before PurgeAndDeleteElements — freeing CEconItem* objects first
    // leaves dangling m_pNonSOEconItem pointers in the views, causing
    // GetInventoryItemByItemID to find stale views and skip re-injection.
    for ( int i = m_aInventoryItems.Count() - 1; i >= 0; --i )
    {
        if ( m_aInventoryItems[i].GetItemID() >= k_LoanerItemIDBase )
            m_aInventoryItems.Remove( i );
    }
    DirtyItemHandles();

    // ── Step 2: Free old backing CEconItem objects ──
    m_vecModItems.PurgeAndDeleteElements();
    m_vecLoanerItems.PurgeAndDeleteElements();

    // ── Step 3: Find the highest occupied backpack slot ──
    // Synthetic items need new-format acknowledged tokens so ValidateInventoryPositions
    // doesn't treat them as unacknowledged (pickup queue) or old-format (GC write).
    // From econ_item_constants.h:
    //   kBackendPosition_NewFormat = 1 << 31 = 0x80000000  (acknowledged, new format)
    //   kBackendPosition_Unacked   = 1 << 30 = 0x40000000  (NOT acknowledged)
    // A valid acknowledged new-format token = slot_number | 0x80000000.
    // SetItemBackpackPosition requires a live SOCache (GC round-trip), so we assign
    // tokens directly on the CEconItem before AddEconItem instead.
    int nNextSlot = 1;
    for ( int i = 0; i < GetItemCount(); ++i )
    {
        CEconItemView *pV = GetItem( i );
        if ( !pV ) continue;
        uint32 tok = pV->GetInventoryPosition();
        if ( !IsUnacknowledged( tok ) )
        {
            int nSlot = TFInventoryManager()->GetBackpackPositionFromBackend( tok );
            if ( nSlot >= nNextSlot )
                nNextSlot = nSlot + 1;
        }
    }

    CTFInventoryManager *pMgr = TFInventoryManager();

    // ── Mod items ──
    for ( int i = 0; i < pMgr->GetModItemCount(); ++i )
    {
        CEconItemView *pSrc = pMgr->GetModItem( i );
        if ( !pSrc ) continue;

        const itemid_t ulID = pSrc->GetItemID();

        CEconItem *pEconItem = new CEconItem();
        pEconItem->SetItemID( ulID );
        pEconItem->SetDefinitionIndex( pSrc->GetItemDefIndex() );
        pEconItem->SetQuality( pSrc->GetItemQuality() );
        pEconItem->SetItemLevel( 1 );
        pEconItem->SetOrigin( kEconItemOrigin_Invalid );
        pEconItem->SetAccountID( 0 );
        // Assign a new-format acknowledged token so this item lands in the normal
        // backpack rather than the unacknowledged pickup queue.
        pEconItem->SetInventoryToken( (uint32)nNextSlot | 0x80000000u );
        ++nNextSlot;
        m_vecModItems.AddToTail( pEconItem );

        if ( AddEconItem( pEconItem, false, false, false ) )
        {
            CEconItemView *pView = GetInventoryItemByItemID( ulID );
            if ( pView )
            {
                pView->SetNonSOEconItem( pEconItem );
                // ItemHasBeenUpdated fired with null GetSOCData() (no m_pSOCache,
                // m_pNonSOEconItem not yet set), so equipped state was not applied.
                // Mod items start unequipped — no extra action needed here.
                // They'll be picked up by LoadLocalLoadout if previously assigned.
            }
        }
    }

    // ── Loaner items ──
    // Only inject loaner items the player does NOT already own a real copy of.
    for ( int i = 0; i < pMgr->GetLoanerItemCount(); ++i )
    {
        CEconItemView *pSrc = pMgr->GetLoanerItem( i );
        if ( !pSrc ) continue;

        const item_definition_index_t nDef      = pSrc->GetItemDefIndex();
        const itemid_t               ulLoanerID = pSrc->GetItemID();

        bool bPlayerOwnsReal = false;
        for ( int j = 0; j < GetItemCount(); ++j )
        {
            CEconItemView *pOwned = GetItem( j );
            if ( pOwned && pOwned->GetItemDefIndex() == nDef
                        && pOwned->GetItemID() < k_LoanerItemIDBase )
            {
                bPlayerOwnsReal = true;
                break;
            }
        }
        if ( bPlayerOwnsReal )
            continue;

        CEconItem *pEconItem = new CEconItem();
        pEconItem->SetItemID( ulLoanerID );
        pEconItem->SetDefinitionIndex( nDef );
        pEconItem->SetQuality( AE_NORMAL );
        pEconItem->SetItemLevel( 1 );
        pEconItem->SetOrigin( kEconItemOrigin_QuestLoanerItem );
        pEconItem->SetAccountID( 0 );
        pEconItem->SetInventoryToken( (uint32)nNextSlot | 0x80000000u );
        ++nNextSlot;
        m_vecLoanerItems.AddToTail( pEconItem );

        if ( AddEconItem( pEconItem, false, false, false ) )
        {
            CEconItemView *pView = GetInventoryItemByItemID( ulLoanerID );
            if ( pView )
                pView->SetNonSOEconItem( pEconItem );
        }
    }
}
#endif // CLIENT_DLL


#ifdef CLIENT_DLL
// ---------------------------------------------------------------------------
// Hand-rolled protobuf helpers for BuildOfflineSOCacheBlob.
// CMsgSOCacheSubscribed and CMsgSOCacheSubscribed_SubscribedType are not
// available as generated classes in this SDK (the .pb.h does not exist).
// We hand-encode the minimal wire format that AddLocalSOCache expects.
//
// CMsgSOCacheSubscribed wire layout:
//   field 1  (owner,   uint64,  wire 0=varint)
//   field 7  (version, uint64,  wire 0=varint)
//   field 2  (objects, message, wire 2=length-delimited)  -- repeated
//     CMsgSOCacheSubscribed_SubscribedType:
//       field 1  (type_id,     int32, wire 0=varint)
//       field 2  (object_data, bytes, wire 2=length-delimited) -- repeated
// ---------------------------------------------------------------------------
namespace
{
	// Append a protobuf varint to a CUtlBuffer.
	static void PB_WriteVarint( CUtlBuffer &buf, uint64 v )
	{
		do
		{
			uint8 b = static_cast<uint8>( v & 0x7F );
			v >>= 7;
			if ( v ) b |= 0x80;
			buf.PutUnsignedChar( b );
		} while ( v );
	}

	// Append a tag byte (field_number << 3 | wire_type).
	static void PB_WriteTag( CUtlBuffer &buf, int fieldNumber, int wireType )
	{
		PB_WriteVarint( buf, static_cast<uint64>( ( fieldNumber << 3 ) | wireType ) );
	}

	// Append a length-delimited field (bytes / embedded message).
	static void PB_WriteBytes( CUtlBuffer &buf, int fieldNumber, const void *pData, int nLen )
	{
		PB_WriteTag( buf, fieldNumber, 2 );
		PB_WriteVarint( buf, static_cast<uint64>( nLen ) );
		buf.Put( pData, nLen );
	}
} // anonymous namespace

bool CTFPlayerInventory::BuildOfflineSOCacheBlob( CUtlMemory<char> &bufOut )
{
	if ( m_vecOfflineItems.IsEmpty() )
		return false;

	// ------------------------------------------------------------------
	// Build the inner CMsgSOCacheSubscribed_SubscribedType message.
	// field 1: type_id  (varint)
	// field 2: object_data (repeated bytes, one entry per item)
	// ------------------------------------------------------------------
	CUtlBuffer bufSubType( 0, 0, 0 );
	bufSubType.SetBigEndian( false );

	// field 1: type_id = CEconItem::k_nTypeID
	PB_WriteTag( bufSubType, 1, 0 );
	PB_WriteVarint( bufSubType, static_cast<uint64>( CEconItem::k_nTypeID ) );

	FOR_EACH_VEC( m_vecOfflineItems, i )
	{
		CEconItem *pEconItem = m_vecOfflineItems[i];
		if ( !pEconItem )
			continue;

		// Skip synthetic items — mod and loaner IDs are above k_ModItemIDBase.
		// The server injects these independently; including them here wastes
		// blob space and causes the reliable stream buffer to overflow with
		// large inventories (700+ real items + 200+ loaners > 4000 bytes).
		if ( pEconItem->GetItemID() >= k_ModItemIDBase )
			continue;

		CSOEconItem msgItem;
		pEconItem->SerializeToProtoBufItem( msgItem );

		std::string sBytes;
		if ( !msgItem.SerializeToString( &sBytes ) )
			continue;

		// field 2: object_data (bytes)
		PB_WriteBytes( bufSubType, 2, sBytes.data(), static_cast<int>( sBytes.size() ) );
	}

	if ( bufSubType.TellPut() == 0 )
		return false;

	// ------------------------------------------------------------------
	// Build the outer CMsgSOCacheSubscribed message.
	// field 1: owner   (uint64 varint)
	// field 7: version (uint64 varint)
	// field 2: objects (embedded message = bufSubType)
	// ------------------------------------------------------------------
	CUtlBuffer bufMsg( 0, 0, 0 );
	bufMsg.SetBigEndian( false );

	// field 1: owner
	PB_WriteTag( bufMsg, 1, 0 );
	PB_WriteVarint( bufMsg, m_OwnerID.ConvertToUint64() );

	// field 7: version = 1
	PB_WriteTag( bufMsg, 7, 0 );
	PB_WriteVarint( bufMsg, 1ULL );

	// field 2: objects (the SubscribedType sub-message)
	PB_WriteBytes( bufMsg, 2, bufSubType.Base(), bufSubType.TellPut() );

	// Base64-encode the result — the server calls Base64Decode then AddLocalSOCache
	return Base64EncodeIntoUTLMemory(
		reinterpret_cast<const uint8 *>( bufMsg.Base() ),
		static_cast<uint32>( bufMsg.TellPut() ),
		bufOut );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static CEconGameAccountClient *GetSOCacheGameAccountClient( CGCClientSharedObjectCache *pSOCache )
{
	if ( !pSOCache )
		return NULL;

	return pSOCache->GetSingleton<CEconGameAccountClient>();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerInventory::GetPreviewItemDef( void ) const
{
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( !pGameAccountClient  )
		return 0;

	return pGameAccountClient->Obj().preview_item_def();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::CanPurchaseItems( int iItemCount ) const
{
	// If we're not a free trial account, we fall back to our default logic of "do
	// we have enough empty slots?".
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( !pGameAccountClient || !pGameAccountClient->Obj().trial_account() )
		return BaseClass::CanPurchaseItems( iItemCount );

	// We're a free trial account, so when we purchase these items, our inventory
	// will actually expand. We check to make sure that we have room for these
	// items against what will be our new maximum backpack size, not our current
	// backpack limit. Include the synthetic item expansion in the post-purchase max.
	const int nSyntheticItems = TFInventoryManager()->GetModItemCount()
	                          + TFInventoryManager()->GetLoanerItemCount();
	const int kPageSize = 50;
	const int nExtraSlots = nSyntheticItems > 0
	    ? ( ( nSyntheticItems + kPageSize - 1 ) / kPageSize ) * kPageSize
	    : 0;

	int iNewItemCount			   = GetItemCount() + iItemCount,
		iAfterPurchaseMaxItemCount = DEFAULT_NUM_BACKPACK_SLOTS
								   + (pGameAccountClient ? pGameAccountClient->Obj().additional_backpack_slots() : 0)
								   + nExtraSlots;

	return iNewItemCount <= iAfterPurchaseMaxItemCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerInventory::GetMaxItemCount( void ) const
{
	// Start with the account-based limit.
	int iMaxItems = DEFAULT_NUM_BACKPACK_SLOTS;
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( pGameAccountClient )
	{
		if ( pGameAccountClient->Obj().trial_account() )
			iMaxItems = DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT;
		iMaxItems += pGameAccountClient->Obj().additional_backpack_slots();
	}
#ifdef CLIENT_DLL
	else if ( m_nCachedAdditionalBackpackSlots > 0 )
	{
		// Offline: m_nCachedAdditionalBackpackSlots stores the FULL real-item slot
		// count (base + GC expansion), saved by SaveOfflineItemCache. Use it directly
		// rather than adding it to DEFAULT_NUM_BACKPACK_SLOTS.
		iMaxItems = m_nCachedAdditionalBackpackSlots;
	}
#endif

	// Add extra pages for synthetic (mod + loaner) items, rounded to page size.
	const int nSyntheticItems = TFInventoryManager()->GetModItemCount()
	                          + TFInventoryManager()->GetLoanerItemCount();
	if ( nSyntheticItems > 0 )
	{
		const int kPageSize = 50;
		iMaxItems += ( ( nSyntheticItems + kPageSize - 1 ) / kPageSize ) * kPageSize;
	}

	return MIN( iMaxItems, MAX_NUM_BACKPACK_SLOTS );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Removes any item in a loadout slot. If the slot has a base item,
//			the player essentially returns to using that item.
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::ClearLoadoutSlot( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return false;

	if ( iClass == GEconItemSchema().GetAccountIndex() )
	{
		if ( m_AccountLoadoutItems[iSlot] == LOADOUT_SLOT_USE_BASE_ITEM )
			return false;
	}
	else
	{
		if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
			return false;

		if ( m_LoadoutItems[iClass][iSlot] == LOADOUT_SLOT_USE_BASE_ITEM )
			return false;
	}
	
	CEconItemView *pItemInSlot = GetItemInLoadout( iClass, iSlot );
	if ( !pItemInSlot )
		return false;

	InventoryManager()->UpdateInventoryEquippedState( this, INVALID_ITEM_ID, iClass, iSlot );

	// TODO: Prediction
	// It's been moved to the backpack, so clear out loadout entry
	//m_LoadoutItems[iClass][iSlot] = LOADOUT_SLOT_USE_BASE_ITEM;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Update weapon skin request list
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateWeaponSkinRequest()
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 );

	FOR_EACH_VEC_BACK( m_vecWeaponSkinRequestList, i )
	{
		SkinRequest_t &req = m_vecWeaponSkinRequestList[i];
		CEconItemView *pItem = GetInventoryItemByItemID( req.m_nID );
		Assert( pItem );
		if ( !pItem ) 
		{
			m_vecWeaponSkinRequestList.Remove( i );
			continue;
		}

		// Have we loaded this one yet? If not, do it here.
		if ( mdlcache->IsErrorModel( req.m_hModel ) )
		{
			const char *pszModelName = pItem->GetPlayerDisplayModel( 0, 0 );
			if ( pszModelName )
			{
				MDLHandle_t hItemMDL = mdlcache->FindMDL( pszModelName );
				if ( mdlcache->IsErrorModel( hItemMDL ) )
				{
					AssertMsg( 0, "failed to find %s from mdlcache", pszModelName );
					m_vecWeaponSkinRequestList.Remove( i );
					continue;
				}

				req.m_hModel = hItemMDL;
			}
			else
			{
				AssertMsg( 0, "failed to precache weapon skin because there's no model" );
				m_vecWeaponSkinRequestList.Remove( i );
			}
		}
		
		// Draw!
		{
			CMDL mdl;
			mdl.SetMDL( req.m_hModel );
			mdl.m_pProxyData = static_cast<IClientRenderable*>( pItem );

			pItem->SetWeaponSkinUseLowRes( true );
			int nRestoreTeam = pItem->GetTeamNumber();
			pItem->SetTeamNumber( req.m_nTeam );

			matrix3x4_t matIdentity;
			SetIdentityMatrix( matIdentity );
			IMaterial *pOverrideMaterial = pItem->GetMaterialOverride( pItem->GetTeamNumber() );

			if ( pOverrideMaterial )
				modelrender->ForcedMaterialOverride( pOverrideMaterial );

			mdl.Draw( matIdentity );

			if ( pOverrideMaterial )
				modelrender->ForcedMaterialOverride( NULL );

			pItem->SetTeamNumber( nRestoreTeam );
			pItem->SetWeaponSkinUseLowRes( false );
		}

		// Don't remove until it's complete. 
		if ( pItem->GetWeaponSkinBase() )
		{
			Assert( pItem->GetWeaponSkinBaseCompositor() == NULL );
			ITexture* pTex = pItem->GetWeaponSkinBase();
			if ( pTex )
			{
				pTex->AddRef(); // We need to hold a ref to the texture.
				m_CachedBaseTextureLowRes[ req.m_nTeam ].Insert( req.m_nID, pTex );
				pItem->SetWeaponSkinBase( NULL ); // Clean up.
			}

			// We do RED, then BLUE. Then drop it out of the list. 
			if ( req.m_nTeam == TF_TEAM_BLUE )
			{
				Assert( !mdlcache->IsErrorModel( req.m_hModel ) );
				mdlcache->Release( req.m_hModel );
				m_vecWeaponSkinRequestList.Remove( i );
			}
			else
			{
				Assert( req.m_nTeam == TF_TEAM_RED );
				req.m_nTeam = TF_TEAM_BLUE;
			}
		}

		// Only do one of these per frame to avoid hitching.
		break;
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ItemHasBeenUpdated( CEconItemView *pItem, bool bUpdateAckFile, bool bWriteAckFile )
{
	Assert( pItem );

	BaseClass::ItemHasBeenUpdated( pItem, bUpdateAckFile, bWriteAckFile );
	const CEconItem *pEconItem = pItem->GetSOCData();

#ifdef CLIENT_DLL
	bool bLocalInv = InventoryManager()->GetLocalInventory() == this;
#endif // CLIENT_DLL

	for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		equipped_slot_t unSlot = pEconItem ? pEconItem->GetEquippedPositionForClass( iClass ) : INVALID_EQUIPPED_SLOT;

		Assert( GetInventoryItemByItemID( pItem->GetItemID() ) );

		bool bThisClassLoadoutChanged = UpdateEquipStateForClass( pItem->GetItemID(), unSlot, m_LoadoutItems[iClass], ARRAYSIZE( m_LoadoutItems[iClass] ) );

		if ( bThisClassLoadoutChanged )
		{
			m_bLoadoutChanged[iClass] = true;

#ifdef CLIENT_DLL
			// if we can inspect this item, it has unique skin.
			// draw it once per team to tell the system to generate unique skin
			if ( bLocalInv && GetPaintKitDefIndex( pItem ) )
			{
				SkinRequest_t req = { TF_TEAM_RED, pItem->GetItemID(), MDLHANDLE_INVALID };
				m_vecWeaponSkinRequestList.AddToTail( req );
			}
#endif // CLIENT_DLL
		}
	}

	// Update account items as well
	equipped_slot_t unSlot = pEconItem ? pEconItem->GetEquippedPositionForClass( GEconItemSchema().GetAccountIndex() ) : INVALID_EQUIPPED_SLOT;
	UpdateEquipStateForClass( pItem->GetItemID(), unSlot, m_AccountLoadoutItems, ARRAYSIZE( m_AccountLoadoutItems ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::UpdateEquipStateForClass( const itemid_t& itemID, equipped_slot_t nSlot, itemid_t *pLoadout, int nCount )
{
	bool bThisClassLoadoutChanged = false;

	// For each slot this item may have been equipped in, set to the base item, unless that slot
	// is the current slot. (ie., if an item moves from slot 10 to slot 12, slot 10 will be set to
	// base item, and slot 12 will be set to this item ID)
	for ( int i = 0; i < nCount; i++ )
	{
		itemid_t& refLoadoutItemID = *( pLoadout + i );

		if ( i == nSlot )
		{
			// This may be an invalid slot for the item, but CTFPlayerInventory::InventoryReceived()
			// will have detected that and sent off a request already to move it. The response
			// to that will clear this loadout slot.
			refLoadoutItemID = itemID;
			bThisClassLoadoutChanged = true;
		}
		else if ( refLoadoutItemID == itemID )
		{
			refLoadoutItemID = LOADOUT_SLOT_USE_BASE_ITEM;
			bThisClassLoadoutChanged = true;
		}
	}

	return bThisClassLoadoutChanged;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::PostSOUpdate( steamIDOwner, eEvent );

	VerifyChangedLoadoutsAreValid();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
#ifdef CLIENT_DLL
	// Free offline-allocated items before the base class purges m_aInventoryItems.
	m_vecOfflineItems.PurgeAndDeleteElements();
#endif

	BaseClass::SOCacheSubscribed( steamIDOwner, eEvent );

#ifdef CLIENT_DLL
	// When fresh webapi data arrives, save it to disk then immediately reload
	// from the file. This makes cfg/local_inventory.txt the single source of
	// truth for in-memory inventory, so the live SOCache is only ever used as
	// a data source for the file — never as the live inventory itself.
	// This also ensures inventory_refresh, autoupdate, and manual edits to the
	// file all behave consistently: the file is always what the player sees.
	if ( m_bGotItemsFromSteam && m_pSOCache && GTFGCClientSystem()->ConsumePendingOfflineCacheSave() )
	{
		SaveOfflineItemCache();

		// Reload from the freshly-written file. This repopulates m_aInventoryItems
		// from the file (not the live SOCache), which is what we want:
		//  - Respects any manual edits the user made to the file
		//  - Ensures mod/loaner injection runs cleanly on a known-good inventory
		//  - Decouples in-memory state from the live SOCache entirely
		// Clear m_pSOCache AFTER saving so SaveOfflineItemCache can still read it,
		// but BEFORE LoadOfflineItemCache so it builds with no live SOCache.
		m_pSOCache = nullptr;
		LoadOfflineItemCache();
		return; // LoadOfflineItemCache runs all post-load steps; skip the ones below
	}
#endif

	UpdateRealTFLoadoutItems();
	LoadLocalLoadout();

	VerifyChangedLoadoutsAreValid();
	UpdateCachedServerLoadoutItems();

#ifdef CLIENT_DLL
	InjectModAndLoanerItems();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add a new item for a econ item
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::AddEconItem( CEconItem * pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems )
{
	if ( BaseClass::AddEconItem( pItem, bUpdateAckFile, bWriteAckFile, bCheckForNewItems ) )
	{
		if ( bCheckForNewItems && InventoryManager()->GetLocalInventory() == this )
		{
			if ( IsUnacknowledged( pItem->GetInventoryToken() ) )
			{
				OnHasNewQuest();
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::VerifyLoadoutItemsAreValid( int iClass )
{
	// Important note: currently this function walks the loadout slots in order causing slots towards
	// the beginning of the list to take priority over slots later in the list. This means that currently
	// weapons take priority over hats, hats take priority over misc slots, etc. which is all well and good.
	// If later we want the order in which slots claim their equip regions to change, we'll want to change
	// the iteration order here and also change GenerateEquipRegionMaskUpToSlot(), which is used for
	// filling out the UI.
	equip_region_mask_t unCumulativeRegionMask = 0;
	for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
	{
		CEconItemView *pEquippedItemView = GetItemInLoadout( iClass, i );
		if ( !pEquippedItemView )
			continue;

		// Does this item use the same regions as some item that we already have equipped?
		equip_region_mask_t unItemEquipMask = pEquippedItemView->GetItemDefinition()->GetEquipRegionMask();
		if ( unItemEquipMask & unCumulativeRegionMask )
		{
			// Unequip this item. This will wind up calling into ::ItemHasBeenUpdated() once the
			// unequip makes it to the GC and back.
			InventoryManager()->UpdateInventoryEquippedState( this, INVALID_ITEM_ID, iClass, pEquippedItemView->GetEquippedPositionForClass( iClass ) );
		}
		else
		{
			unCumulativeRegionMask |= unItemEquipMask;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::VerifyChangedLoadoutsAreValid()
{
	// We assume that each local client will verify their own inventory, but we don't want my client to verify that
    // your loadout is valid -- we expect that'll be done either by the server or the GC or, worst case, your local
    // client.
    //
    // A note on this: this change is being made because some malicious users are intentionally equipping invalid sets
    // of items on bots (and players?) and joining servers. Later on, this code calls InventoryManager() functions to
    // remove the problematic items, but the inventory manager is a global for the local player. This means that another
    // played connected with an invalid inventory can cause changes in your inventory because we don't distinguish
    // self/other state past this point.
    if ( GetOwner() != steamapicontext->SteamUser()->GetSteamID() )
        return;

	// We maybe changed equip state for an item and it's possible if we're sending bad messages and/or
	// hacking state that we'll now have conflicting items equipped. Walk all of our items for this class
	// to verify our state is good.
	for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( m_bLoadoutChanged[iClass] )
		{
			VerifyLoadoutItemsAreValid( iClass );
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ItemIsBeingRemoved( CEconItemView *pItem )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( pItem->IsEquippedForClass( iClass ) )
		{
			for ( int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; iSlot++ )
			{
				if ( m_LoadoutItems[iClass][iSlot] == pItem->GetItemID() )
				{
					m_LoadoutItems[iClass][iSlot] = LOADOUT_SLOT_USE_BASE_ITEM;
					m_bLoadoutChanged[iClass] = true;
				}
			}
		}
	}
}


//=======================================================================================================================
// TF PLAYER INVENTORY
//=======================================================================================================================

#ifdef CLIENT_DLL
// WTF: Declaring this inline caused a compiler bug.
CTFPlayerInventory	*CTFInventoryManager::GetLocalTFInventory( void ) 
{ 
	return &m_LocalInventory; 
}
#endif

#ifdef CLIENT_DLL
void CTFInventoryManager::UpdateInventoryEquippedState(CPlayerInventory *pInventory, uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot)
{
	BaseClass::UpdateInventoryEquippedState(pInventory, ulItemID, unClass, unSlot);

	CTFPlayerInventory* pTFInventory = dynamic_cast<CTFPlayerInventory*>(pInventory);
	if (pTFInventory) 
	{
		pTFInventory->EquipLocal(ulItemID, unClass, unSlot);
		pTFInventory->SaveLocalLoadout();
	}
}

#endif

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_dumpinv_other, "Dumps the contents of a specified client inventory. Format: item_dumpinv_other <player index>", FCVAR_NONE )
{
	if ( args.ArgC() > 1 )
	{
		C_TFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( atoi(args[1]) ) );
		if ( pOther )
		{
			pOther->Inventory()->DumpInventoryToConsole( true );
			return;
		}
	}
	Msg("Couldn't find specified player.\nFormat: item_dumpinv_other <player index>\n");
}
#endif
#endif // _DEBUG

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_dumpinv, "Dumps the contents of a specified client inventory. Format: item_dumpinv", FCVAR_CHEAT )
#else
CON_COMMAND_F( item_dumpinv_sv, "Dumps the contents of a specified server inventory. Format: item_dumpinv_sv", FCVAR_CHEAT )
#endif
{
#if defined(CLIENT_DLL)
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
#else
	CSteamID steamID;
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	pPlayer->GetSteamID( &steamID );
#endif

	CPlayerInventory *pInventory = TFInventoryManager()->GetInventoryForPlayer( steamID );
	if ( !pInventory )
	{
		Msg("No inventory for that player.\n");
		return;
	}

	pInventory->DumpInventoryToConsole( true );
}
#endif

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_deleteunknowns, "Deletes all items in your inventory that we don't have static data for. Useful for removing items that have been removed from the backend.", FCVAR_CHEAT )
{
	int iDeleted = TFInventoryManager()->DeleteUnknowns( InventoryManager()->GetLocalInventory() );
	Msg("Deleted %d unknown items.\n", iDeleted);
}
#endif
#endif


#if defined( TF_CLIENT_DLL )
CON_COMMAND( load_itempreset, "Equip all items for a given preset on the player." )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: \"load_itempreset <preset index>\" - <preset index> can be 0 to 3." );
		return;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	equipped_class_t unClass = pPlayer->GetPlayerClass()->GetClassIndex();
	equipped_preset_t unPreset = atoi( args[1] );
	if ( TFInventoryManager()->LoadPreset( unClass, unPreset ) )
	{
		// Tell the GC to tell server that we should respawn if we're in a respawn room
		extern ConVar tf_respawn_on_loadoutchanges;
		if ( tf_respawn_on_loadoutchanges.GetBool() )
		{
			GCSDK::CGCMsg< ::MsgGCEmpty_t > msg( k_EMsgGCRespawnPostLoadoutChange );
			GCClientSystem()->BSendMessage( msg );
		}
	}
}
#endif	// TF_CLIENT_DLL

#if defined( TF_CLIENT_DLL ) && INVENTORY_VIA_WEBAPI
CON_COMMAND(save_loadout, "Save local loadout.")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout();
}

CON_COMMAND(reset_loadout, "Reset local loadout to what is active on TF2.")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout(true, true);
	pInventory->LoadLocalLoadout();
}

CON_COMMAND(clear_loadout, "Clear local loadout back to defaults")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout(true, false);
	pInventory->LoadLocalLoadout();
}

CON_COMMAND(reset_loadout_ui, "Reset local loadout to what is active on TF2 (show confirmation)")
{
	ShowConfirmDialog( "#TF_SDK_ResetLoadout_Title",
		"#TF_SDK_ResetLoadout_Desc",
		"#MessageBox_OK",
		"#cancel", []( bool bConfirmed, void* pContext )
		{
			if ( bConfirmed )
				engine->ClientCmd_Unrestricted( "reset_loadout\n" );
		});
}

CON_COMMAND(clear_loadout_ui, "Clear local loadout back to stock defaults (show confirmation)")
{
	ShowConfirmDialog( "#TF_SDK_ClearLoadout_Title",
		"#TF_SDK_ClearLoadout_Desc",
		"#MessageBox_OK",
		"#cancel", []( bool bConfirmed, void* pContext )
		{
			if ( bConfirmed )
				engine->ClientCmd_Unrestricted( "clear_loadout\n" );
		} );
}
#endif	// TF_CLIENT_DLL

#if defined( TF_CLIENT_DLL ) && INVENTORY_VIA_WEBAPI
bool CTFInventoryManager::LoadPreset(equipped_class_t unClass, equipped_preset_t unPreset)
{
	if (!IsValidPlayerClass(unClass))
		return false;

	if (!IsPresetIndexValid(unPreset))
		return false;

	// NOTE: removed GetSOC() check — preset switching is purely local (local_loadout.txt)
	// and does not require a live GC connection.

	if (!steamapicontext || !steamapicontext->SteamUser())
		return false;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CTFPlayerInventory *pInv = GetInventoryForPlayer(localSteamID);
	if (!pInv)
		return false;

	if (m_flNextLoadPresetChange > gpGlobals->realtime)
	{
		Msg("Loadout change denied. Changing presets too quickly.\n");
		return false;
	}

	m_flNextLoadPresetChange = gpGlobals->realtime + 0.5f;

	return pInv->EquipLocalPreset(unClass, unPreset);
}

bool CTFPlayerInventory::EquipLocalPreset(equipped_class_t unClass, equipped_preset_t unPreset)
{
	if (!InventoryManager()->IsPresetIndexValid(unPreset))
		return false;

	m_ActivePreset[unClass] = unPreset;

	for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
	{
		itemid_t uItemId = m_PresetItems[unPreset][unClass][iSlot];
		EquipLocal(uItemId, unClass, iSlot);
	}

	SaveLocalLoadout();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Serializes the live inventory to disk so it can be restored when
//          the webapi is unreachable. Called whenever fresh GC data arrives.
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SaveOfflineItemCache()
{
	if ( InventoryManager()->GetLocalInventory() != this )
		return;
	if ( !g_pFullFileSystem || !m_pSOCache )
		return;

	CGCClientSharedObjectTypeCache *pTypeCache =
		m_pSOCache->FindTypeCache( CEconItem::k_nTypeID );
	if ( !pTypeCache )
	{
		Warning( "[OfflineInventory] SOCache has no item type cache — nothing to save\n" );
		return;
	}

	KeyValues *pRootKV = new KeyValues( "local_inventory" );
	int nSaved = 0;

	// Store the full GC-granted backpack size (base + expansion) so that offline
	// GetMaxItemCount can reconstruct the same limit without a live SOCache.
	// We store the total real-item slot count here; GetMaxItemCount adds synthetic
	// slots on top at runtime, so we don't include those in the file.
	{
		int nRealSlots = DEFAULT_NUM_BACKPACK_SLOTS;
		CEconGameAccountClient *pAcct = m_pSOCache->GetSingleton<CEconGameAccountClient>();
		if ( pAcct )
		{
			if ( pAcct->Obj().trial_account() )
				nRealSlots = DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT;
			nRealSlots += pAcct->Obj().additional_backpack_slots();
		}
		pRootKV->SetInt( "backpack_slots", nRealSlots );
	}

	// ── Real GC items ──
	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CEconItem *pItem = static_cast<CEconItem *>( pTypeCache->GetObject( i ) );
		if ( !pItem ) continue;

		char szID[32];
		V_snprintf( szID, sizeof(szID), "%llu", pItem->GetItemID() );

		KeyValues *pItemKV = new KeyValues( szID );
		pItemKV->SetInt(    "defindex", (int)pItem->GetDefinitionIndex() );
		pItemKV->SetInt(    "quality",  (int)pItem->GetQuality() );
		pItemKV->SetInt(    "level",    (int)pItem->GetItemLevel() );
		pItemKV->SetInt(    "origin",   (int)pItem->GetOrigin() );
		pItemKV->SetString( "pos",      CNumStr( pItem->GetInventoryToken() ) );

		CUtlString strEquipped;
		for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; ++iClass )
		{
			equipped_slot_t iSlot = pItem->GetEquippedPositionForClass( iClass );
			if ( iSlot != INVALID_EQUIPPED_SLOT )
			{
				if ( !strEquipped.IsEmpty() ) strEquipped += " ";
				char szPair[16];
				V_snprintf( szPair, sizeof(szPair), "%d,%d", (int)iClass, (int)iSlot );
				strEquipped += szPair;
			}
		}
		if ( !strEquipped.IsEmpty() )
			pItemKV->SetString( "equipped", strEquipped.Get() );

		pRootKV->AddSubKey( pItemKV );
		++nSaved;
	}

	// ── Mod items — write into the file so they load as real items next time ──
	// Mod items get IDs in the k_ModItemIDBase range and are always injected
	// regardless of the player's real inventory.
	CTFInventoryManager *pMgr = TFInventoryManager();
	int nNextSlot = (int)( pRootKV->GetInt( "backpack_slots", DEFAULT_NUM_BACKPACK_SLOTS ) ) + 1;

	for ( int i = 0; i < pMgr->GetModItemCount(); ++i )
	{
		CEconItemView *pSrc = pMgr->GetModItem( i );
		if ( !pSrc ) continue;

		char szID[32];
		V_snprintf( szID, sizeof(szID), "%llu", pSrc->GetItemID() );

		KeyValues *pItemKV = new KeyValues( szID );
		pItemKV->SetInt(    "defindex", (int)pSrc->GetItemDefIndex() );
		pItemKV->SetInt(    "quality",  (int)pSrc->GetItemQuality() );
		pItemKV->SetInt(    "level",    1 );
		pItemKV->SetInt(    "origin",   (int)kEconItemOrigin_Invalid );
		pItemKV->SetString( "pos",      CNumStr( (uint32)nNextSlot | 0x80000000u ) );
		++nNextSlot;

		pRootKV->AddSubKey( pItemKV );
		++nSaved;
	}

	// ── Loaner items — only for defindexes the player doesn't own a real copy of ──
	for ( int i = 0; i < pMgr->GetLoanerItemCount(); ++i )
	{
		CEconItemView *pSrc = pMgr->GetLoanerItem( i );
		if ( !pSrc ) continue;

		const item_definition_index_t nDef = pSrc->GetItemDefIndex();

		// Skip if the player already has a real item with this defindex
		bool bOwnsReal = false;
		for ( uint32 j = 0; j < pTypeCache->GetCount(); ++j )
		{
			CEconItem *pReal = static_cast<CEconItem *>( pTypeCache->GetObject( j ) );
			if ( pReal && pReal->GetDefinitionIndex() == nDef )
			{
				bOwnsReal = true;
				break;
			}
		}
		if ( bOwnsReal ) continue;

		char szID[32];
		V_snprintf( szID, sizeof(szID), "%llu", pSrc->GetItemID() );

		KeyValues *pItemKV = new KeyValues( szID );
		pItemKV->SetInt(    "defindex", (int)nDef );
		pItemKV->SetInt(    "quality",  AE_NORMAL );
		pItemKV->SetInt(    "level",    1 );
		pItemKV->SetInt(    "origin",   (int)kEconItemOrigin_QuestLoanerItem );
		pItemKV->SetString( "pos",      CNumStr( (uint32)nNextSlot | 0x80000000u ) );
		++nNextSlot;

		pRootKV->AddSubKey( pItemKV );
		++nSaved;
	}

	if ( pRootKV->SaveToFile( g_pFullFileSystem, OFFLINE_ITEM_CACHE_FILE, "MOD" ) )
		DevMsg( "[OfflineInventory] Saved %d items to %s\n", nSaved, OFFLINE_ITEM_CACHE_FILE );
	else
		Warning( "[OfflineInventory] Failed to write %s\n", OFFLINE_ITEM_CACHE_FILE );

	pRootKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Loads the disk cache and injects items directly into m_aInventoryItems,
//          bypassing the SOCache/GC path entirely. Sets m_bGotItemsFromSteam so
//          all RetrievedInventoryFromSteam() gates pass. Called from PostInitGC
//          after RequestInventory() has registered the inventory as a listener.
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::LoadOfflineItemCache()
{
	if ( InventoryManager()->GetLocalInventory() != this )
		return false;
	if ( !g_pFullFileSystem )
		return false;
	if ( !g_pFullFileSystem->FileExists( OFFLINE_ITEM_CACHE_FILE, "MOD" ) )
		return false;

	KeyValues *pRootKV = new KeyValues( "local_inventory" );
	if ( !pRootKV->LoadFromFile( g_pFullFileSystem, OFFLINE_ITEM_CACHE_FILE, "MOD" ) )
	{
		pRootKV->deleteThis();
		Warning( "[OfflineInventory] Failed to parse %s\n", OFFLINE_ITEM_CACHE_FILE );
		return false;
	}

	m_vecOfflineItems.PurgeAndDeleteElements();
	m_aInventoryItems.Purge();
	DirtyItemHandles();

	// Restore cached account data that GetMaxItemCount() needs when offline.
	m_nCachedAdditionalBackpackSlots = pRootKV->GetInt( "backpack_slots", 0 );

	int nLoaded = 0;
	FOR_EACH_TRUE_SUBKEY( pRootKV, pItemKV )
	{
		// Item ID is the subkey name
		itemid_t nItemID = (itemid_t)V_atoui64( pItemKV->GetName() );
		if ( nItemID == 0 )
		{
			Warning( "[OfflineInventory] Skipping item with invalid ID '%s'\n", pItemKV->GetName() );
			continue;
		}

		CEconItem *pEconItem = new CEconItem();
		pEconItem->SetItemID(          nItemID );
		pEconItem->SetDefinitionIndex( (item_definition_index_t)pItemKV->GetInt( "defindex", 0 ) );
		pEconItem->SetQuality(         pItemKV->GetInt( "quality", AE_UNIQUE ) );
		pEconItem->SetItemLevel(       (uint32)pItemKV->GetInt( "level", 1 ) );
		pEconItem->SetOrigin(          (eEconItemOrigin)pItemKV->GetInt( "origin", kEconItemOrigin_Invalid ) );
		pEconItem->SetInventoryToken(  (uint32)V_atoui64( pItemKV->GetString( "pos", "0" ) ) );

		// Restore equipped state from "class,slot" pairs
		const char *pszEquipped = pItemKV->GetString( "equipped", "" );
		if ( pszEquipped && pszEquipped[0] )
		{
			// Walk the space-separated "class,slot" pairs without strtok to
			// avoid the strtok_s (Windows) vs strtok_r (Linux) mismatch.
			const char *pScan = pszEquipped;
			while ( *pScan )
			{
				// Skip spaces
				while ( *pScan == ' ' ) ++pScan;
				if ( !*pScan ) break;

				int iClass = 0, iSlot = 0;
				if ( sscanf( pScan, "%d,%d", &iClass, &iSlot ) == 2 )
					pEconItem->Equip( (equipped_class_t)iClass, (equipped_slot_t)iSlot );

				// Advance past this token
				while ( *pScan && *pScan != ' ' ) ++pScan;
			}
		}

		m_vecOfflineItems.AddToTail( pEconItem );

		if ( AddEconItem( pEconItem, /*bUpdateAckFile=*/true,
		                             /*bWriteAckFile=*/false,
		                             /*bCheckForNewItems=*/false ) )
		{
			CEconItemView *pView = GetInventoryItemByItemID( nItemID );
			if ( pView )
			{
				pView->SetNonSOEconItem( pEconItem );

				// ItemHasBeenUpdated fired inside AddEconItem while GetSOCData() was
				// still null (m_pSOCache is null and m_pNonSOEconItem wasn't set yet).
				// That means UpdateEquipStateForClass saw INVALID_EQUIPPED_SLOT for
				// every class and did NOT populate m_LoadoutItems. Fix that now by
				// reading the equipped state directly from pEconItem and writing it.
				for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
				{
					equipped_slot_t iSlot = pEconItem->GetEquippedPositionForClass( iClass );
					if ( iSlot != INVALID_EQUIPPED_SLOT && iSlot < CLASS_LOADOUT_POSITION_COUNT )
					{
						m_LoadoutItems[iClass][iSlot] = nItemID;
						m_bLoadoutChanged[iClass] = true;
					}
				}
			}
		}
		++nLoaded;
	}

	pRootKV->deleteThis();

	if ( nLoaded == 0 )
	{
		m_vecOfflineItems.PurgeAndDeleteElements();
		Warning( "[OfflineInventory] No valid items found in %s\n", OFFLINE_ITEM_CACHE_FILE );
		return false;
	}

	m_bGotItemsFromSteam = true;
	CInventoryManager::SendItemSystemConnectedEvent();
	ResortInventory();
	InventoryManager()->CleanAckFile();
	InventoryManager()->SaveAckFile();

	// m_OwnerID is normally set by BaseClass::SOCacheSubscribed, which we bypass
	// in the offline path. Without it, LoadLocalLoadout and SaveLocalLoadout check
	// GetOwner() != SteamUser()->GetSteamID() and return immediately.
	// SteamUser() may be null when fully offline (Steam servers disconnected), so
	// we use GetLocalPlayerSteamID() from ClientSteamContext which caches it
	// from the last known good login and works without a live Steam connection.
	if ( m_OwnerID.IsValid() == false )
	{
		CSteamID cachedID = ClientSteamContext().GetLocalPlayerSteamID();
		if ( cachedID.IsValid() )
			m_OwnerID = cachedID;
	}

	// Mod and loaner items are now written directly into local_inventory.txt by
	// SaveOfflineItemCache, so they load as regular items above. No separate
	// injection step is needed. InjectModAndLoanerItems is still used in the
	// online SOCacheSubscribed path as a fallback, but not here.

	UpdateRealTFLoadoutItems();
	LoadLocalLoadout();

	// Run ValidateInventoryPositions AFTER the full load sequence. Running it
	// before means UpdateInventoryEquippedState can write INVALID_ITEM_ID into
	// m_LoadoutItems for legitimate entries, breaking presets.
	ValidateInventoryPositions();
	VerifyChangedLoadoutsAreValid();
	UpdateCachedServerLoadoutItems();

	DevMsg( "[OfflineInventory] Loaded %d items from %s\n", nLoaded, OFFLINE_ITEM_CACHE_FILE );
	return true;
}
#endif // CLIENT_DLL

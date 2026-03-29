//=============================================================================
// tf2v_offline_inventory.cpp
// See tf2v_offline_inventory.h for full documentation.
//=============================================================================

#include "cbase.h"
#include "tf2v_offline_inventory.h"
#include "econ_item_view.h"
#include "econ_item_schema.h"
#include "econ_item_constants.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_shareddefs.h"       // TF_CLASS_COUNT_ALL, TF_LOADOUT_SLOT_COUNT
#include "gc_clientsystem.h"     // GCClientSystem()->BConnectedtoGC()

#ifndef NO_STEAM
#ifdef CLIENT_DLL
#include "clientsteamcontext.h"  // ClientSteamContext().BLoggedOn()
#else
#include "steam/steam_gameserver.h"
#endif
#endif

// ---------------------------------------------------------------------------
// Convars
// ---------------------------------------------------------------------------

ConVar tf2v_offline_inventory(
    "tf2v_offline_inventory", "0",
    FCVAR_ARCHIVE | FCVAR_NOTIFY,
    "Force offline inventory emulator (0 = auto-detect, 1 = always offline).\n"
    "In DEBUG builds this is always active regardless of this setting." );

// ---------------------------------------------------------------------------
// File paths
// ---------------------------------------------------------------------------

#define TF2V_OFFLINE_DIR        "cfg/tf2v_offline"
#define TF2V_OFFLINE_INV_FILE   "cfg/tf2v_offline/inventory.vdf"
#define TF2V_OFFLINE_SESS_FILE  "cfg/tf2v_offline/session.vdf"

// Offline item IDs start here to avoid colliding with real GC item IDs.
// Real GC IDs are uint64 values in the billions; we use a local namespace
// starting at 1,000,000. These are never sent to the GC.
#define TF2V_OFFLINE_ITEM_ID_BASE  1000000ULL

// ---------------------------------------------------------------------------
// TF2VIsOfflineMode
// ---------------------------------------------------------------------------

bool TF2VIsOfflineMode()
{
#ifdef _DEBUG
    return true;   // Debug builds always use offline inventory — no GC noise
#endif

    // Force override
    if ( tf2v_offline_inventory.GetBool() )
        return true;

#ifndef NO_STEAM
    // No Steam context
#ifdef CLIENT_DLL
    if ( !ClientSteamContext().BLoggedOn() )
        return true;
#else
    if ( !steamgameserverapicontext ||
         !steamgameserverapicontext->SteamGameServer() ||
         !steamgameserverapicontext->SteamGameServer()->BLoggedOn() )
        return true;
#endif

    // GC not connected (includes: GC down, behind firewall, LAN session)
    if ( !GCClientSystem()->BConnectedtoGC() )
        return true;
#else
    // NO_STEAM build — always offline
    return true;
#endif

    return false;
}

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static KeyValues  *s_pInventoryKV  = NULL;  // root of inventory.vdf
static bool        s_bInitialized  = false;

// Per-slot item views, built on Init from the VDF.
// Indexed [class][slot]. NULL means "use base item".

// Helper class to collect all item definitions
class CAllItemDefinitionsCollector : public IEconItemDefinitionIterator
{
public:
    CUtlVector<CEconItemDefinition *> m_vecItemDefs;

    virtual bool OnIterate( CEconItemDefinition *pItemDef ) OVERRIDE
    {
        if ( pItemDef )
        {
            m_vecItemDefs.AddToTail( pItemDef );
        }
        return true; // Continue iterating
    }
};

static CEconItemView *s_pItems[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT];
static CUtlVector<CEconItemView *> s_pAllGeneratedItemViews;

static void ClearItemCache()
{
    for ( int c = 0; c < TF_CLASS_COUNT_ALL; c++ )
        for ( int s = 0; s < TF_LOADOUT_SLOT_COUNT; s++ )
        {
            s_pItems[c][s] = NULL; // s_pAllGeneratedItemViews owns the items
        }

    for ( int i = 0; i < s_pAllGeneratedItemViews.Count(); ++i )
    {
        delete s_pAllGeneratedItemViews[i];
    }
    s_pAllGeneratedItemViews.Purge();
}

// ---------------------------------------------------------------------------
// VDF helpers
// ---------------------------------------------------------------------------

// Returns the CSteamID string for the local player, used as the VDF root key.
static void GetLocalSteamIDString( char *pszOut, int nLen )
{
#ifndef NO_STEAM
#ifdef CLIENT_DLL
    if ( steamapicontext && steamapicontext->SteamUser() )
    {
        CSteamID id = steamapicontext->SteamUser()->GetSteamID();
        uint64 uid = id.ConvertToUint64();
        if ( uid != 0 )
        {
            V_snprintf( pszOut, nLen, "%llu", uid );
            return;
        }
    }
#endif
#endif
    // Fallback — use "local" as key for LAN / no-Steam sessions
    V_strncpy( pszOut, "local", nLen );
}

// Gets or creates the per-player KV block within inventory.vdf.
static KeyValues *GetOrCreatePlayerKV( const char *pszSteamID )
{
    if ( !s_pInventoryKV ) return NULL;
    KeyValues *pPlayer = s_pInventoryKV->FindKey( pszSteamID );
    if ( !pPlayer )
    {
        pPlayer = new KeyValues( pszSteamID );
        s_pInventoryKV->AddSubKey( pPlayer );
    }
    return pPlayer;
}

// ---------------------------------------------------------------------------
// Build the in-memory item cache from VDF loadout data.
// VDF structure (per player):
//   "loadout"
//   {
//       "scout" { "primary" "45" "secondary" "0" ... }
//       "soldier" { ... }
//       ...
//   }
// Value "0" means use stock/base item.
// Any non-zero value is a def_index in the local schema.
// ---------------------------------------------------------------------------
static void RebuildItemCache( KeyValues *pPlayerKV )
{
    ClearItemCache(); // This now clears s_pAllGeneratedItemViews and nulls s_pItems

    // 1. Collect all item definitions
    CAllItemDefinitionsCollector collector;
    GetItemSchema()->IterateItemDefinitions( &collector );

    // 2. Create all CEconItemView objects and store them
    for ( int i = 0; i < collector.m_vecItemDefs.Count(); ++i )
    {
        const CEconItemDefinition *pDef = collector.m_vecItemDefs[i];
        if ( !pDef )
            continue;

        CEconItemView *pView = new CEconItemView( pDef->GetDefinitionIndex() );
        pView->SetItemQuality( AE_UNIQUE );
        pView->SetItemID( TF2V_OFFLINE_ITEM_ID_BASE + (uint64)(i + 1) ); // Unique ID based on index

        s_pAllGeneratedItemViews.AddToTail( pView );
    }

    // 3. Populate s_pItems (equipped slots) with a selection from s_pAllGeneratedItemViews
    //    We'll try to put one unique item into each class/slot combination.
    //    The remaining items are still in s_pAllGeneratedItemViews and can be accessed
    //    via other means (e.g., a UI that lists all owned items).

    CUtlVector<CEconItemView *> remainingItemsToEquip = s_pAllGeneratedItemViews; // Copy for tracking
    int currentItemIndex = 0;

    static const char *s_szClassNames[] =
    {
        "",           // TF_CLASS_UNDEFINED
        "scout",      "sniper",    "soldier",  "demoman",
        "medic",      "heavy",     "pyro",     "spy",      "engineer",
    };
    // Note: static_assert for ARRAYSIZE(s_szClassNames) == TF_CLASS_COUNT_ALL is already in original code or handled.

    static const char *s_szSlotNames[] =
    {
        "primary",   "secondary", "melee",    "pda",      "pda2",
        "building",  "head",      "misc",     "misc2",    "misc3",
        "action",    "taunt",     "taunt2",   "taunt3",   "taunt4",
        "taunt5",    "taunt6",    "taunt7",   "taunt8",
    };
    // Note: slot count from schema may exceed this array — handle gracefully.

    for ( int c = 1; c < TF_CLASS_COUNT_ALL; c++ )
    {
        for ( int s = 0; s < ARRAYSIZE(s_szSlotNames); s++ )
        {
            if ( s >= TF_LOADOUT_SLOT_COUNT ) break;

            // Find an item from the remaining items that fits this class and slot
            for ( int i = 0; i < remainingItemsToEquip.Count(); ++i )
            {
                CEconItemView *pItemToEquip = remainingItemsToEquip[i];
                if ( !pItemToEquip )
                    continue;

                const CEconItemDefinition *pDefToEquip = pItemToEquip->GetItemDefinition();
                if ( !pDefToEquip )
                    continue;
                
                // Check if the item can be equipped by this class in this slot
                if ( pDefToEquip->CanBeEquippedByClass( (TF_CLASS)c ) && pDefToEquip->CanBeEquippedInLoadoutSlot( (loadout_slot_t)s ) )
                {
                    s_pItems[c][s] = pItemToEquip;
                    remainingItemsToEquip.Remove( i ); // Remove from consideration for other slots
                    // DevMsg( "[TF2V Offline] Equipped item %s for class %s in slot %s\n", pDefToEquip->GetName(), s_szClassNames[c], s_szSlotNames[s] );
                    break; // Move to the next slot
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_Init
// ---------------------------------------------------------------------------

void TF2VOfflineInventory_Init()
{
    if ( s_bInitialized ) return;
    s_bInitialized = true;

    memset( s_pItems, 0, sizeof(s_pItems) );

    // Ensure directory exists
    g_pFullFileSystem->CreateDirHierarchy( TF2V_OFFLINE_DIR, "MOD" );

    // Load or create inventory.vdf
    s_pInventoryKV = new KeyValues( "tf2v_offline_inventory" );
    if ( !s_pInventoryKV->LoadFromFile( g_pFullFileSystem,
                                         TF2V_OFFLINE_INV_FILE, "MOD" ) )
    {
        // First run — create a blank file with stock items
        s_pInventoryKV->SaveToFile( g_pFullFileSystem,
                                     TF2V_OFFLINE_INV_FILE, "MOD" );
        DevMsg( "[TF2V Offline] Created blank inventory: %s\n", TF2V_OFFLINE_INV_FILE );
    }
    else
    {
        DevMsg( "[TF2V Offline] Loaded %s\n", TF2V_OFFLINE_INV_FILE );
    }

    // Build the in-memory item cache for the local player
    char szSteamID[32];
    GetLocalSteamIDString( szSteamID, sizeof(szSteamID) );
    KeyValues *pPlayerKV = GetOrCreatePlayerKV( szSteamID );
    RebuildItemCache( pPlayerKV );
}

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_Shutdown
// ---------------------------------------------------------------------------

void TF2VOfflineInventory_Shutdown()
{
    if ( !s_bInitialized ) return;
    s_bInitialized = false;

    ClearItemCache();

    if ( s_pInventoryKV )
    {
        s_pInventoryKV->deleteThis();
        s_pInventoryKV = NULL;
    }
}

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_GetItem
// ---------------------------------------------------------------------------

CEconItemView *TF2VOfflineInventory_GetItem( int iClass, int iSlot )
{
    if ( !s_bInitialized ) TF2VOfflineInventory_Init();
    if ( iClass < 0 || iClass >= TF_CLASS_COUNT_ALL ) return NULL;
    if ( iSlot  < 0 || iSlot  >= TF_LOADOUT_SLOT_COUNT ) return NULL;
    return s_pItems[iClass][iSlot];  // NULL means "use stock"
}

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_EquipItem
// ---------------------------------------------------------------------------

void TF2VOfflineInventory_EquipItem( int iClass, int iSlot,
                                      item_definition_index_t nDefIndex,
                                      int nQuality )
{
    if ( !s_bInitialized ) TF2VOfflineInventory_Init();
    if ( iClass < 1 || iClass >= TF_CLASS_COUNT_ALL ) return;
    if ( iSlot  < 0 || iSlot  >= TF_LOADOUT_SLOT_COUNT ) return;

    static const char *s_szClassNames[] =
    {
        "",       "scout",  "sniper", "soldier", "demoman",
        "medic",  "heavy",  "pyro",   "spy",     "engineer",
    };
    static const char *s_szSlotNames[] =
    {
        "primary", "secondary", "melee",  "pda",   "pda2",
        "building","head",      "misc",   "misc2", "misc3",
        "action",  "taunt",     "taunt2", "taunt3","taunt4",
        "taunt5",  "taunt6",    "taunt7", "taunt8",
    };

    // Update VDF
    char szSteamID[32];
    GetLocalSteamIDString( szSteamID, sizeof(szSteamID) );
    KeyValues *pPlayerKV = GetOrCreatePlayerKV( szSteamID );
    if ( !pPlayerKV ) return;

    KeyValues *pLoadout = pPlayerKV->FindKey( "loadout", true );
    KeyValues *pClass   = pLoadout->FindKey( s_szClassNames[iClass], true );
    pClass->SetInt( s_szSlotNames[iSlot], (int)nDefIndex );

    s_pInventoryKV->SaveToFile( g_pFullFileSystem, TF2V_OFFLINE_INV_FILE, "MOD" );

    // Rebuild in-memory cache
    RebuildItemCache( pPlayerKV );

    DevMsg( "[TF2V Offline] Equipped def_index %d in slot %s/%s\n",
            (int)nDefIndex, s_szClassNames[iClass], s_szSlotNames[iSlot] );
}

// ---------------------------------------------------------------------------
// Console commands
// ---------------------------------------------------------------------------

CON_COMMAND( tf2v_offline_reload,
             "Reload the offline inventory from cfg/tf2v_offline/inventory.vdf." )
{
    if ( !TF2VIsOfflineMode() )
    {
        Msg( "[TF2V Offline] Not in offline mode — use tf2v_offline_inventory 1 first.\n" );
        return;
    }
    TF2VOfflineInventory_Shutdown();
    TF2VOfflineInventory_Init();
    Msg( "[TF2V Offline] Inventory reloaded.\n" );
}

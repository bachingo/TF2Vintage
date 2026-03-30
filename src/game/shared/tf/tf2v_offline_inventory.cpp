//=============================================================================
// tf2v_offline_inventory.cpp
// See tf2v_offline_inventory.h for documentation.
//=============================================================================

#include "cbase.h"
#include "tf2v_offline_inventory.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_shareddefs.h"
#include "tf_item_constants.h"
#include "econ_item_view.h"
#include "econ_item_schema.h"
#include "tf_item_schema.h"
#include "gc_clientsystem.h"

#ifndef NO_STEAM
#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
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
    "Force offline inventory emulator. 0=auto, 1=always offline." );

// ---------------------------------------------------------------------------
// File paths
// ---------------------------------------------------------------------------

#define TF2V_OFFLINE_DIR        "cfg/tf2v_offline"
#define TF2V_OFFLINE_INV_FILE   "cfg/tf2v_offline/inventory.vdf"

// Offline item IDs start at 1,000,000 to avoid colliding with real GC IDs.
#define TF2V_OFFLINE_ITEM_ID_BASE  1000000ULL

// ---------------------------------------------------------------------------
// TF2VIsOfflineMode
// ---------------------------------------------------------------------------

bool TF2VIsOfflineMode()
{
#ifdef _DEBUG
    return true;
#endif

    if ( tf2v_offline_inventory.GetBool() )
        return true;

#ifndef NO_STEAM
#ifdef CLIENT_DLL
    if ( !ClientSteamContext().BLoggedOn() )
        return true;
#else
    if ( !steamgameserverapicontext ||
         !steamgameserverapicontext->SteamGameServer() ||
         !steamgameserverapicontext->SteamGameServer()->BLoggedOn() )
        return true;
#endif
    if ( !GCClientSystem()->BConnectedtoGC() )
        return true;
#else
    return true;
#endif

    return false;
}

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static KeyValues  *s_pInventoryKV = NULL;
static bool        s_bInitialized = false;

// Per-slot item views indexed [class][slot]. NULL = use stock.
static CEconItemView *s_pItems[TF_CLASS_COUNT_ALL][CLASS_LOADOUT_POSITION_COUNT];

static void ClearItemCache()
{
    for ( int c = 0; c < TF_CLASS_COUNT_ALL; c++ )
        for ( int s = 0; s < CLASS_LOADOUT_POSITION_COUNT; s++ )
        {
            delete s_pItems[c][s];
            s_pItems[c][s] = NULL;
        }
}

// ---------------------------------------------------------------------------
// SteamID helper
// ---------------------------------------------------------------------------

static void GetLocalSteamIDString( char *pszOut, int nLen )
{
#ifndef NO_STEAM
#ifdef CLIENT_DLL
    if ( steamapicontext && steamapicontext->SteamUser() )
    {
        uint64 uid = steamapicontext->SteamUser()->GetSteamID().ConvertToUint64();
        if ( uid != 0 )
        {
            V_snprintf( pszOut, nLen, "%llu", uid );
            return;
        }
    }
#endif
#endif
    V_strncpy( pszOut, "local", nLen );
}

// ---------------------------------------------------------------------------
// Build item cache from VDF loadout data
// VDF structure per player:
//   "loadout" { "scout" { "primary" "45" ... } ... }
// Value 0 = stock (leave NULL).
// ---------------------------------------------------------------------------

static void RebuildItemCache( KeyValues *pPlayerKV )
{
    ClearItemCache();
    if ( !pPlayerKV ) return;

    KeyValues *pLoadout = pPlayerKV->FindKey( "loadout" );
    if ( !pLoadout ) return;

    // Class name strings (index matches TF_CLASS_* constants)
    static const char *s_szClassNames[TF_CLASS_COUNT_ALL] =
    {
        "",          // TF_CLASS_UNDEFINED
        "scout",     "sniper",   "soldier",  "demoman",
        "medic",     "heavy",    "pyro",     "spy",      "engineer",
    };

    // Slot name strings (index matches loadout_positions_t)
    static const char *s_szSlotNames[] =
    {
        "primary",  "secondary", "melee",   "pda",    "pda2",
        "building", "head",      "misc",    "misc2",  "misc3",
        "action",   "taunt",     "taunt2",  "taunt3", "taunt4",
        "taunt5",   "taunt6",    "taunt7",  "taunt8",
    };
    static_assert( ARRAYSIZE(s_szSlotNames) <= CLASS_LOADOUT_POSITION_COUNT,
                   "Slot name array larger than CLASS_LOADOUT_POSITION_COUNT" );

    GameItemSchema_t *pSchema = GetItemSchema();
    if ( !pSchema ) return;

    for ( int c = 1; c < TF_CLASS_COUNT_ALL; c++ )
    {
        KeyValues *pClass = pLoadout->FindKey( s_szClassNames[c] );
        if ( !pClass ) continue;

        for ( int s = 0; s < (int)ARRAYSIZE(s_szSlotNames); s++ )
        {
            int nDefIndex = pClass->GetInt( s_szSlotNames[s], 0 );
            if ( nDefIndex == 0 ) continue;

            const CEconItemDefinition *pDef =
                pSchema->GetItemDefinition( nDefIndex );
            if ( !pDef )
            {
                DevWarning( "[TF2V Offline] Unknown def_index %d for %s/%s — skipping.\n",
                            nDefIndex, s_szClassNames[c], s_szSlotNames[s] );
                continue;
            }

            // Verify the item can actually go in this class/slot
            int nItemSlot = pDef->GetLoadoutSlot( c );
            if ( nItemSlot != s )
            {
                DevWarning( "[TF2V Offline] def_index %d doesn't fit slot %s for %s — skipping.\n",
                            nDefIndex, s_szSlotNames[s], s_szClassNames[c] );
                continue;
            }

            CEconItemView *pView = new CEconItemView();
            pView->SetItemDefIndex( (item_definition_index_t)nDefIndex );
            pView->SetItemQuality( AE_UNIQUE );
            pView->SetItemID( TF2V_OFFLINE_ITEM_ID_BASE + (uint64)(c * 100 + s) );

            s_pItems[c][s] = pView;
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

    g_pFullFileSystem->CreateDirHierarchy( TF2V_OFFLINE_DIR, "MOD" );

    s_pInventoryKV = new KeyValues( "tf2v_offline_inventory" );
    if ( !s_pInventoryKV->LoadFromFile( g_pFullFileSystem, TF2V_OFFLINE_INV_FILE, "MOD" ) )
    {
        // First run — create a blank inventory file
        s_pInventoryKV->SaveToFile( g_pFullFileSystem, TF2V_OFFLINE_INV_FILE, "MOD" );
        DevMsg( "[TF2V Offline] Created blank inventory: %s\n", TF2V_OFFLINE_INV_FILE );
    }

    char szSteamID[32];
    GetLocalSteamIDString( szSteamID, sizeof(szSteamID) );

    KeyValues *pPlayerKV = s_pInventoryKV->FindKey( szSteamID );
    if ( !pPlayerKV )
    {
        pPlayerKV = new KeyValues( szSteamID );
        s_pInventoryKV->AddSubKey( pPlayerKV );
    }
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
    if ( iSlot  < 0 || iSlot  >= CLASS_LOADOUT_POSITION_COUNT ) return NULL;
    return s_pItems[iClass][iSlot];
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
    if ( iSlot  < 0 || iSlot  >= CLASS_LOADOUT_POSITION_COUNT ) return;

    static const char *s_szClassNames[TF_CLASS_COUNT_ALL] =
    {
        "", "scout", "sniper", "soldier", "demoman",
        "medic", "heavy", "pyro", "spy", "engineer",
    };
    static const char *s_szSlotNames[] =
    {
        "primary", "secondary", "melee", "pda", "pda2",
        "building", "head", "misc", "misc2", "misc3",
        "action", "taunt", "taunt2", "taunt3", "taunt4",
        "taunt5", "taunt6", "taunt7", "taunt8",
    };

    char szSteamID[32];
    GetLocalSteamIDString( szSteamID, sizeof(szSteamID) );

    KeyValues *pPlayerKV = s_pInventoryKV ? s_pInventoryKV->FindKey( szSteamID, true ) : NULL;
    if ( !pPlayerKV ) return;

    KeyValues *pLoadout = pPlayerKV->FindKey( "loadout", true );
    KeyValues *pClass   = pLoadout->FindKey( s_szClassNames[iClass], true );
    pClass->SetInt( s_szSlotNames[iSlot], (int)nDefIndex );

    if ( s_pInventoryKV )
        s_pInventoryKV->SaveToFile( g_pFullFileSystem, TF2V_OFFLINE_INV_FILE, "MOD" );

    RebuildItemCache( pPlayerKV );
}

// ---------------------------------------------------------------------------
// Console command
// ---------------------------------------------------------------------------

CON_COMMAND( tf2v_offline_reload, "Reload offline inventory from disk." )
{
    if ( !TF2VIsOfflineMode() )
    {
        Msg( "[TF2V Offline] Not in offline mode.\n" );
        return;
    }
    TF2VOfflineInventory_Shutdown();
    TF2VOfflineInventory_Init();
    Msg( "[TF2V Offline] Reloaded.\n" );
}

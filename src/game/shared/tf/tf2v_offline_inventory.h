//=============================================================================
// tf2v_offline_inventory.h
//
// TF2V Offline Inventory Emulator
//
// Replaces the old TF2V CTFInventory (VDF array of 8 weapon slots) with a
// system that uses the merge branch's existing local_loadout.txt infrastructure.
// The offline emulator activates when:
//   - _DEBUG build (always — test builds never need GC noise)
//   - Steam user unavailable (no Steam, LAN-only session, offline mode)
//   - GC not connected (GC down, firewall, etc.)
//   - tf2v_offline_inventory 1 (server operator or player forces offline)
//
// When GC reconnects, SOCacheSubscribed fires and LoadLocalLoadout() reloads
// from the GC-backed data automatically. No explicit "dump and reload" needed.
//
// File layout:
//   cfg/local_loadout.txt        — merge branch's existing uint64 loadout cache
//   cfg/tf2v_offline/inventory.vdf — richer offline item database (per SteamID)
//   cfg/tf2v_offline/session.vdf   — transient session data (quest progress, etc.)
//
// SmartSteamEmulator comparison:
//   SSE intercepts Steam API calls at the DLL level and spoofs them.
//   TF2V's offline emulator does not intercept Steam — it sits above the GC
//   layer, using the merge branch's own "mod mode" (EquipLocal / LoadLocalLoadout)
//   as the foundation. The GC is not spoofed; it is simply bypassed when absent.
//=============================================================================
#pragma once

#include "econ_item_view.h"

// ---------------------------------------------------------------------------
// TF2VIsOfflineMode() — the central offline check.
//
// Called from UpdateInventory and GetLoadoutItem. Returns true when the
// offline emulator should serve items instead of the GC-backed inventory.
// ---------------------------------------------------------------------------
bool TF2VIsOfflineMode();

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_Init()
// Load or create cfg/tf2v_offline/inventory.vdf for the local player.
// Called from CTFPlayer::Spawn() (server) and CHLClient::Init() (client).
// ---------------------------------------------------------------------------
void TF2VOfflineInventory_Init();

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_Shutdown()
// Flush session.vdf to disk. Called from CTFPlayer disconnect / client shutdown.
// ---------------------------------------------------------------------------
void TF2VOfflineInventory_Shutdown();

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_GetItem( iClass, iSlot )
// Returns the CEconItemView* the offline emulator has for this slot.
// Returns NULL if the offline inventory has no item here (caller uses base item).
// The returned pointer is valid until the next map load.
// ---------------------------------------------------------------------------
CEconItemView *TF2VOfflineInventory_GetItem( int iClass, int iSlot );

// ---------------------------------------------------------------------------
// TF2VOfflineInventory_EquipItem( iClass, iSlot, nDefIndex, nQuality )
// Equips an item in the offline loadout. Writes to inventory.vdf immediately.
// Called from the offline loadout UI.
// ---------------------------------------------------------------------------
void TF2VOfflineInventory_EquipItem( int iClass, int iSlot,
                                     item_definition_index_t nDefIndex,
                                     int nQuality = AE_UNIQUE );

// ---------------------------------------------------------------------------
// Convar declared here so both server and client can reference it.
// Defined in tf2v_offline_inventory.cpp.
// ---------------------------------------------------------------------------
extern ConVar tf2v_offline_inventory;

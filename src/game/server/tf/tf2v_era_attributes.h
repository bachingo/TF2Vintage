//=============================================================================
// tf2v_era_attributes.h
//
// Per-era weapon attribute overrides for TF2 Vintage.
//
// THREE SYSTEMS IN ONE HEADER:
//
// 1. TF2V_ERA_ATTRIBUTES — per-item attribute overrides by era
//    Loaded from cfg/tf2v_era_attributes.vdf (sideloaded, never touches
//    live items_game.txt). When an item is equipped at era N, any era-specific
//    attributes defined for that item override the schema defaults for that
//    era. This handles weapon behaviour changes across patches.
//
// 2. TF2VStripAnachronisticModifiers — graceful degradation
//    Instead of rejecting an item outright, strips the newest-first modifiers
//    from a copy of the CEconItemView until it complies with the active era.
//    Order of removal: War Paint (170) → Stat clock (150) → Collector's
//    quality (133) → Haunted quality (117) → Strange quality (112) →
//    Unusual quality (103) → Paint (100). Returns false only if the base
//    item itself post-dates the era.
//
// 3. TF2VApplyEraAttributeOverrides — era behaviour patch
//    After the item passes (possibly after stripping), applies era-specific
//    attribute values from tf2v_era_attributes.vdf to the live entity.
//    This is called after GiveNamedItem, targeting the weapon entity directly.
//=============================================================================
#pragma once

class CEconItemView;
class CTFWeaponBase;
class CEconEntity;

// ---------------------------------------------------------------------------
// TF2VStripAnachronisticModifiers
//
// Takes the player's loadout CEconItemView, checks it against the active era,
// and returns a modified COPY that complies (stripping modifiers newest-first).
// The caller passes this copy to GiveNamedItem instead of the original.
//
// The original item in the player's inventory is NEVER modified.
// Returns true if the copy (pOutView) is valid at the active era.
// Returns false if even the base item post-dates the era — use stock instead.
//
// pStripLog (optional): filled with what was stripped, for VGUI display.
// ---------------------------------------------------------------------------

#define TF2V_MAX_STRIP_LOG_ENTRIES 6

struct CTF2VStripLog
{
    int  nEntries;
    struct Entry_t
    {
        char szWhat[64];       // e.g. "War Paint", "Strange quality", "Paint"
        char szDate[32];       // e.g. "October 20, 2017"
        char szUpdate[48];     // e.g. "Jungle Inferno Update"
    } entries[TF2V_MAX_STRIP_LOG_ENTRIES];

    CTF2VStripLog() { memset(this, 0, sizeof(*this)); }
};

bool TF2VStripAnachronisticModifiers( const CEconItemView *pOriginal,
                                      CEconItemView       *pOutView,
                                      int                  nActiveEra,
                                      CTF2VStripLog       *pLog = NULL );

// ---------------------------------------------------------------------------
// TF2VApplyEraAttributeOverrides
//
// Called after the weapon entity has been spawned. Reads
// cfg/tf2v_era_attributes.vdf and applies era-appropriate attribute values
// to the live weapon entity, overriding the schema defaults.
//
// This implements patch-accurate weapon behaviour — e.g. the Backburner
// having +50 HP at era 20 (Pyro Update) before the Jungle Inferno rework.
//
// pEntity: the newly spawned CTFWeaponBase / CEconEntity.
// nDefIndex: item definition index (to look up in the VDF).
// nActiveEra: current server era.
// ---------------------------------------------------------------------------
void TF2VApplyEraAttributeOverrides( CEconEntity  *pEntity,
                                     int           nDefIndex,
                                     int           nActiveEra );

// Reload the era attributes VDF without restarting.
void TF2VReloadEraAttributesTable();

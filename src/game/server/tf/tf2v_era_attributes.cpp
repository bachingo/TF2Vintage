//=============================================================================
// tf2v_era_attributes.cpp
// See tf2v_era_attributes.h for full documentation.
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "econ_item_view.h"
#include "econ_item_schema.h"
#include "econ_item_constants.h"
#include "econ_entity.h"
#include "tf_weaponbase.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_gamerules.h"
#include "tf2v_era_attributes.h"
#include "tf2v_item_era_enforcement.h"

// ---------------------------------------------------------------------------
// Era attributes VDF — loaded once, reloaded on demand.
//
// File format: cfg/tf2v_era_attributes.vdf
//
//   "tf2v_era_attributes"
//   {
//       "45"    // item def index (Backburner)
//       {
//           "20"    // era integer (Pyro Update)
//           {
//               "add_crit_when_team_capped"    "1"
//               "max_health_additive_bonus"    "50"
//               "airblast_disabled"            "1"
//           }
//           "105"   // era 105 — Backburner gains airblast back
//           {
//               "airblast_disabled"            "0"
//               "max_health_additive_bonus"    "50"
//           }
//           "170"   // era 170 — Jungle Inferno rework
//           {
//               "mult_flame_ammopersec"        "0.5"
//               "max_health_additive_bonus"    "0"
//               "airblast_disabled"            "0"
//           }
//       }
//   }
//
// Eras are applied using "highest era <= active era" logic.
// ---------------------------------------------------------------------------

static bool       s_bAttrTableLoaded = false;
static KeyValues *s_pAttrTable       = NULL;

static void TF2VEnsureAttrTableLoaded()
{
    if ( s_bAttrTableLoaded ) return;
    s_bAttrTableLoaded = true;

    KeyValues *pKV = new KeyValues( "tf2v_era_attributes" );
    if ( !pKV->LoadFromFile( filesystem, "cfg/tf2v_era_attributes.vdf", "MOD" ) )
    {
        DevMsg( "[TF2V] cfg/tf2v_era_attributes.vdf not found — "
                "per-era attribute overrides disabled.\n" );
        pKV->deleteThis();
        return;
    }
    s_pAttrTable = pKV;
    DevMsg( "[TF2V] Loaded era attribute overrides from "
            "cfg/tf2v_era_attributes.vdf.\n" );
}

void TF2VReloadEraAttributesTable()
{
    if ( s_pAttrTable ) { s_pAttrTable->deleteThis(); s_pAttrTable = NULL; }
    s_bAttrTableLoaded = false;
    TF2VEnsureAttrTableLoaded();
    Msg( "[TF2V] Era attribute overrides reloaded.\n" );
}

CON_COMMAND( tf2v_reload_era_attributes,
    "Reload cfg/tf2v_era_attributes.vdf without restarting the server." )
{
    TF2VReloadEraAttributesTable();
}

// ---------------------------------------------------------------------------
// StripLog helper
// ---------------------------------------------------------------------------
static void StripLog( CTF2VStripLog *pLog,
                      const char *pszWhat, const char *pszDate,
                      const char *pszUpdate )
{
    if ( !pLog || pLog->nEntries >= TF2V_MAX_STRIP_LOG_ENTRIES ) return;
    int i = pLog->nEntries++;
    V_strncpy( pLog->entries[i].szWhat,   pszWhat,   sizeof(pLog->entries[i].szWhat) );
    V_strncpy( pLog->entries[i].szDate,   pszDate,   sizeof(pLog->entries[i].szDate) );
    V_strncpy( pLog->entries[i].szUpdate, pszUpdate, sizeof(pLog->entries[i].szUpdate) );
}

// ---------------------------------------------------------------------------
// System 2: TF2VStripAnachronisticModifiers
//
// Goal: given a player's loadout item and the active era, produce a copy
// of that item with only the modifiers that existed in the active era.
//
// Holy Mackerel example (def era 100, Mann-Conomy):
//   Server era 2025 (201+) — Strange + Decoration: nothing stripped, spawn as-is.
//   Server era 2015 (150)  — Strange OK (112), Decoration (170) stripped.
//   Server era 2010 (100)  — Strange (112) and Decoration (170) both stripped;
//                            item spawns as plain Unique Holy Mackerel.
//   Server era 2009 (70)   — base item era 100 > active era 70: return false,
//                            caller falls back to stock (bat).
//
// All era thresholds are read from the VDF via TF2VGetBaseItemEra(),
// TF2VGetQualityEra(), and TF2VGetAttributeEra() — the same functions used
// by TF2VIsItemEraAllowed(). The two systems cannot drift apart.
//
// Returns true  — pOutView is valid; spawn it (modifiers may have been stripped).
// Returns false — base item post-dates active era; caller must use stock.
// ---------------------------------------------------------------------------

bool TF2VStripAnachronisticModifiers( const CEconItemView *pOriginal,
                                      CEconItemView       *pOutView,
                                      int                  nActiveEra,
                                      CTF2VStripLog       *pLog )
{
    Assert( pOriginal && pOutView );
    if ( !pOriginal || !pOutView ) return false;

    // Deep-copy the original. We mutate pOutView only.
    *pOutView = *pOriginal;

    const CEconItemDefinition *pDef = pOutView->GetItemDefinition();
    if ( !pDef ) return false;

    // --- Gate 1: is the base item itself too new? ---
    // Uses TF2VGetBaseItemEra so VDF overrides are respected.
    int nBaseEra = TF2VGetBaseItemEra( pDef );
    if ( nBaseEra > nActiveEra )
        return false;  // base item post-dates era — caller gives stock

    // Helper: remove attribute by schema name, returns true if it existed.
    auto RemoveAttr = [&]( const char *pszAttr ) -> bool
    {
        static CSchemaAttributeDefHandle hAttr( pszAttr );
        if ( hAttr && pOutView->FindAttribute( hAttr ) )
        {
            pOutView->GetAttributeList()->RemoveAttribute( hAttr );
            return true;
        }
        return false;
    };

    // Helper: get era threshold from VDF for a quality, with a sane fallback
    // if the VDF key is absent (should never happen with a complete VDF, but
    // we don't want a missing key to disable stripping entirely).
    auto QualEra = [&]( int nQuality, int nFallback ) -> int
    {
        int n = TF2VGetQualityEra( nQuality, nullptr );
        return ( n > 0 ) ? n : nFallback;
    };

    // Helper: get era threshold from VDF for an attribute floor key.
    auto AttrEra = [&]( const char *pszVDFKey, int nFallback ) -> int
    {
        int n = TF2VGetAttributeEra( pszVDFKey );
        return ( n > 0 ) ? n : nFallback;
    };

    // --- Strip in newest-first order ---
    // Each block only fires if the active era is below that modifier's
    // introduction era. The era threshold is always read from the VDF.

    // 1. War Paint / weapon skin  (VDF: _q_paintkitweapon, _a_set_item_texture_wear)
    {
        int nEra = QualEra( AE_PAINTKITWEAPON, 170 );
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_PAINTKITWEAPON )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                RemoveAttr( "set item texture wear" );
                RemoveAttr( "set item texture seed" );
                RemoveAttr( "paintkit_proto_def_index" );
                StripLog( pLog, "War Paint / weapon skin",
                          "October 20, 2017", "Jungle Inferno Update" );
            }
        }
    }

    // 2. Stat clock / kill eater  (VDF: _a_kill_eater)
    {
        int nEra = AttrEra( "_a_kill_eater", 150 );
        if ( nActiveEra < nEra )
        {
            if ( RemoveAttr( "kill eater" ) )
            {
                RemoveAttr( "kill eater score type" );
                RemoveAttr( "kill eater 2" );
                RemoveAttr( "kill eater score type 2" );
                RemoveAttr( "kill eater 3" );
                RemoveAttr( "kill eater score type 3" );
                // If the item is Strange because of the kill eater, drop to Unique.
                // Check this AFTER stripping the kill eater, not before, so we
                // don't accidentally downgrade items that are Strange for other
                // reasons (handled by the Strange quality block below).
                if ( pOutView->GetItemQuality() == AE_STRANGE )
                {
                    int nStrangeEra = QualEra( AE_STRANGE, 112 );
                    if ( nActiveEra < nStrangeEra )
                        pOutView->SetItemQuality( AE_UNIQUE );
                }
                StripLog( pLog, "Stat clock (kill eater)",
                          "December 17, 2015", "Tough Break Update" );
            }
        }
    }

    // 3. Collector's quality  (VDF: _q_collectors)
    {
        int nEra = QualEra( AE_COLLECTORS, 133 );
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_COLLECTORS )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                StripLog( pLog, "Collector's quality",
                          "December 22, 2014", "Smissmas 2014" );
            }
        }
    }

    // 4. Haunted quality  (VDF: _q_haunted)
    {
        int nEra = QualEra( AE_HAUNTED, 117 );
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_HAUNTED )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                StripLog( pLog, "Haunted quality",
                          "October 2012", "Scream Fortress 2012" );
            }
        }
    }

    // 5. Strange quality  (VDF: _q_strange)
    {
        int nEra = QualEra( AE_STRANGE, 112 );
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_STRANGE )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                // Kill eater was already removed in step 2 if applicable,
                // but strip again defensively for items that have Strange
                // quality without a kill eater (e.g. legacy Strange items).
                RemoveAttr( "kill eater" );
                RemoveAttr( "kill eater score type" );
                StripLog( pLog, "Strange quality",
                          "December 15, 2011", "Australian Christmas 2011" );
            }
        }
    }

    // 6. Halloween spells  (VDF: _a_spell_type)
    {
        int nEra = AttrEra( "_a_spell_type", 117 );
        if ( nActiveEra < nEra )
        {
            if ( RemoveAttr( "halloween spell type" ) )
            {
                StripLog( pLog, "Halloween spell",
                          "October 2012", "Scream Fortress 2012" );
            }
        }
    }

    // 7. Unusual quality  (VDF: _q_unusual)
    {
        int nEra = QualEra( AE_UNUSUAL, 103 );
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_UNUSUAL )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                RemoveAttr( "attach particle effect" );
                StripLog( pLog, "Unusual quality",
                          "December 17, 2010", "Australian Christmas 2010" );
            }
        }
    }

    // 8. Paint  (VDF: _a_paint_color — add to VDF if you need server-side tuning;
    //            falls back to era 100 / Mann-Conomy if key absent)
    {
        int nEra = AttrEra( "_a_paint_color", 100 );
        if ( nActiveEra < nEra )
        {
            bool bHadPaint = RemoveAttr( "paint color" );
            bHadPaint     |= RemoveAttr( "paint color 2" );
            if ( bHadPaint )
                StripLog( pLog, "Paint",
                          "September 30, 2010", "Mann-Conomy Update" );
        }
    }

    // 9. Genuine quality  (VDF: _q_genuine — add to VDF to override;
    //                      falls back to era 100 / Mann-Conomy if key absent)
    {
        int nEra = QualEra( AE_RARITY1, 100 );  // AE_RARITY1 == Genuine
        if ( nActiveEra < nEra )
        {
            if ( pOutView->GetItemQuality() == AE_RARITY1 )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
                StripLog( pLog, "Genuine quality",
                          "September 30, 2010", "Mann-Conomy Update" );
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// System 3: TF2VApplyEraAttributeOverrides
//
// Finds the correct era block in tf2v_era_attributes.vdf for this item
// and applies attribute values to the live entity via SetRuntimeAttributeValue.
// "Highest era block <= active era" — same cascading logic as mapcycles.
// ---------------------------------------------------------------------------

void TF2VApplyEraAttributeOverrides( CEconEntity *pEntity,
                                     int          nDefIndex,
                                     int          nActiveEra )
{
    TF2VEnsureAttrTableLoaded();
    if ( !s_pAttrTable || !pEntity ) return;

    char szDefKey[16];
    V_snprintf( szDefKey, sizeof(szDefKey), "%d", nDefIndex );
    KeyValues *pItemKV = s_pAttrTable->FindKey( szDefKey );
    if ( !pItemKV ) return;

    // Walk era blocks to find highest era <= active era
    KeyValues *pBestBlock = NULL;
    int        nBestEra   = -1;

    for ( KeyValues *pEraKV = pItemKV->GetFirstSubKey();
          pEraKV; pEraKV = pEraKV->GetNextKey() )
    {
        int nBlockEra = Q_atoi( pEraKV->GetName() );
        if ( nBlockEra <= nActiveEra && nBlockEra > nBestEra )
        {
            nBestEra   = nBlockEra;
            pBestBlock = pEraKV;
        }
    }

    if ( !pBestBlock ) return;

    CAttributeList *pAttrList = pEntity->GetAttributeList();
    if ( !pAttrList ) return;

    for ( KeyValues *pAttr = pBestBlock->GetFirstValue();
          pAttr; pAttr = pAttr->GetNextValue() )
    {
        const char *pszAttrName = pAttr->GetName();
        float flValue = pAttr->GetFloat();

        const CEconItemAttributeDefinition *pAttrDef =
            GetItemSchema()->GetAttributeDefinitionByName( pszAttrName );
        if ( !pAttrDef )
        {
            DevWarning( "[TF2V] tf2v_era_attributes: unknown attribute '%s' "
                        "on item %d\n", pszAttrName, nDefIndex );
            continue;
        }

        pAttrList->SetRuntimeAttributeValue( pAttrDef, flValue );
    }

    DevMsg( "[TF2V] Applied era %d attribute overrides to item %d.\n",
            nBestEra, nDefIndex );
}

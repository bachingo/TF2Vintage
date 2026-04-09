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
//       // etc.
//   }
//
// Eras are applied using "highest era <= active era" logic, identical to the
// mapcycle routing. If no era block exists for an item, the schema defaults
// are used unchanged.
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
        // Not an error — servers running latest era don't need the file.
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
// System 2: TF2VStripAnachronisticModifiers
//
// Strips newest-first modifiers from a copy of the item view until it
// complies with nActiveEra. Each modifier has a known introduction era.
// Stripping order (newest to oldest):
//   1. War paint / weapon skin     (era 170 — Jungle Inferno)
//   2. Stat clock / kill eater     (era 150 — Tough Break)
//   3. Collector's quality         (era 133 — Smissmas 2014)
//   4. Haunted quality             (era 117 — Scream Fortress 2012)
//   5. Strange quality             (era 112 — Atribute Dec 2011)
//   6. Unusual quality             (era 103 — Australian Christmas 2010)
//   7. Paint                       (era 100 — Mannconomy Sep 2010)
//   8. Genuine quality             (era 100 — Mannconomy)
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

bool TF2VStripAnachronisticModifiers( const CEconItemView *pOriginal,
                                      CEconItemView       *pOutView,
                                      int                  nActiveEra,
                                      CTF2VStripLog       *pLog )
{
    Assert( pOriginal && pOutView );
    if ( !pOriginal || !pOutView ) return false;

    // Deep-copy the original. We will mutate pOutView only.
    *pOutView = *pOriginal;

    const CEconItemDefinition *pDef = pOutView->GetItemDefinition();
    if ( !pDef ) return false;

    // Actually check just the item's own date:
    int nItemOnlyEra = TF2VDateStringToEra_Public( pDef->GetFirstSaleDate() );
    if ( nItemOnlyEra > nActiveEra )
        return false;  // base item post-dates era — use stock

    // Helper: attribute removal by schema name
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

    // --- Strip newest-first ---

    // 1. War Paint (era 170)
    if ( nActiveEra < 170 )
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

    // 2. Stat clock / kill eater (era 150)
    if ( nActiveEra < 150 )
    {
        if ( RemoveAttr( "kill eater" ) )
        {
            RemoveAttr( "kill eater score type" );
            RemoveAttr( "kill eater 2" );
            RemoveAttr( "kill eater score type 2" );
            RemoveAttr( "kill eater 3" );
            RemoveAttr( "kill eater score type 3" );
            // If quality was Strange-for-kill-eater, drop to Unique
            if ( pOutView->GetItemQuality() == AE_STRANGE )
            {
                pOutView->SetItemQuality( AE_UNIQUE );
            }
            StripLog( pLog, "Stat clock (kill eater)",
                      "December 17, 2015", "Tough Break Update" );
        }
    }

    // 3. Collector's quality (era 133)
    if ( nActiveEra < 133 )
    {
        if ( pOutView->GetItemQuality() == AE_COLLECTORS )
        {
            pOutView->SetItemQuality( AE_UNIQUE );
            StripLog( pLog, "Collector's quality",
                      "December 22, 2014", "Smissmas 2014" );
        }
    }

    // 4. Haunted quality (era 117)
    if ( nActiveEra < 117 )
    {
        if ( pOutView->GetItemQuality() == AE_HAUNTED )
        {
            pOutView->SetItemQuality( AE_UNIQUE );
            StripLog( pLog, "Haunted quality",
                      "October 2012", "Scream Fortress 2012" );
        }
    }

    // 5. Strange quality (era 112)
    if ( nActiveEra < 112 )
    {
        if ( pOutView->GetItemQuality() == AE_STRANGE )
        {
            pOutView->SetItemQuality( AE_UNIQUE );
            RemoveAttr( "kill eater" );
            RemoveAttr( "kill eater score type" );
            StripLog( pLog, "Strange quality",
                      "December 15, 2011", "Australian Christmas 2011" );
        }
    }

    // 6. Unusual quality (era 103)
    if ( nActiveEra < 103 )
    {
        if ( pOutView->GetItemQuality() == AE_UNUSUAL )
        {
            pOutView->SetItemQuality( AE_UNIQUE );
            RemoveAttr( "attach particle effect" );
            StripLog( pLog, "Unusual quality",
                      "December 17, 2010", "Australian Christmas 2010" );
        }
    }

    // 7. Paint (era 100)
    if ( nActiveEra < 100 )
    {
        bool bHadPaint = RemoveAttr( "paint color" );
        bHadPaint     |= RemoveAttr( "paint color 2" );
        if ( bHadPaint )
            StripLog( pLog, "Paint",
                      "September 30, 2010", "Mann-Conomy Update" );
    }

    // 8. Genuine quality (era 100 — promo items)
    if ( nActiveEra < 100 )
    {
        if ( pOutView->GetItemQuality() == AE_RARITY1 )
        {
            pOutView->SetItemQuality( AE_UNIQUE );
            StripLog( pLog, "Genuine quality",
                      "September 30, 2010", "Mann-Conomy Update" );
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// System 3: TF2VApplyEraAttributeOverrides
//
// Finds the correct era block in tf2v_era_attributes.vdf for this item
// and applies attribute values to the live entity via SetRuntimeAttributeValue.
//
// "Highest era block <= active era" — same cascading logic as mapcycles.
// If no file or no matching entry, this is a no-op.
// ---------------------------------------------------------------------------

void TF2VApplyEraAttributeOverrides( CEconEntity *pEntity,
                                     int          nDefIndex,
                                     int          nActiveEra )
{
    TF2VEnsureAttrTableLoaded();
    if ( !s_pAttrTable || !pEntity ) return;

    // Find this item's entry
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

    if ( !pBestBlock ) return;  // no era block applies

    CAttributeList *pAttrList = pEntity->GetAttributeList();
    if ( !pAttrList ) return;

    // Apply each attribute in the era block
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

//=============================================================================
// tf2v_item_era_enforcement.cpp
//
// Era-accurate item enforcement for TF2 Vintage.
// Hooks into CTFPlayer::ItemIsAllowed() in tf_player.cpp.
//
// Architecture:
//   - Loads cfg/tf2v_item_eras.vdf at first use (lazy, cached).
//   - TF2VGetItemEra( CEconItemView* ) returns the earliest era at which
//     that specific item instance could legitimately exist, accounting for:
//       1. The base item's first_sale_date era
//       2. The item's quality (Strange, Unusual, Collector's, etc.)
//       3. Applied attributes (stat clock / kill eater, war paint / texture wear)
//   - ItemIsAllowed() calls TF2VIsItemEraAllowed() which returns false and
//     sets a reason string if the item post-dates the active era.
//   - The reason string feeds the existing ban-reason display path used by
//     competitive whitelisting, with a new code:
//     ITEM_BAN_ANACHRONISTIC (distinct from ITEM_BAN_COMPETITIVE).
//
// File format: cfg/tf2v_item_eras.vdf
//   "tf2v_item_eras"
//   {
//       "45"    "10"      // item def index -> min era
//       "_q_strange"  "112"  // quality floor
//       "_a_kill_eater" "150" // attribute floor
//   }
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "econ_item_view.h"
#include "econ_item_schema.h"
#include "econ_item_constants.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_gamerules.h"

// ----------------------------------------------------------------------------
// Era table — loaded once from cfg/tf2v_item_eras.vdf, then cached.
// Thread safety: server-only, single-threaded frame loop.
// ----------------------------------------------------------------------------

static bool           s_bEraTableLoaded  = false;
static KeyValues     *s_pEraTable        = NULL;  // "tf2v_item_eras" root KV

static void TF2VEnsureEraTableLoaded()
{
    if ( s_bEraTableLoaded )
        return;

    s_bEraTableLoaded = true;  // set first to avoid re-entry on error

    KeyValues *pKV = new KeyValues( "tf2v_item_eras" );
    if ( !pKV->LoadFromFile( filesystem, "cfg/tf2v_item_eras.vdf", "MOD" ) )
    {
        Warning( "[TF2V] Could not load cfg/tf2v_item_eras.vdf — "
                 "era item enforcement disabled.\n" );
        pKV->deleteThis();
        return;
    }

    s_pEraTable = pKV;
    DevMsg( "[TF2V] Loaded era table from cfg/tf2v_item_eras.vdf.\n" );
}

// Reload the era table (useful for live server config changes).
CON_COMMAND( tf2v_reload_item_eras,
             "Reload cfg/tf2v_item_eras.vdf without restarting the server." )
{
    if ( s_pEraTable )
    {
        s_pEraTable->deleteThis();
        s_pEraTable = NULL;
    }
    s_bEraTableLoaded = false;
    TF2VEnsureEraTableLoaded();
    Msg( "[TF2V] Item era table reloaded.\n" );
}

// ----------------------------------------------------------------------------
// Lookup helpers
// ----------------------------------------------------------------------------

// Returns the min era for an item def index, or 0 if not in table.
static int TF2VGetBaseItemEra( item_definition_index_t nDefIndex )
{
    if ( !s_pEraTable )
        return 0;

    char szKey[16];
    V_snprintf( szKey, sizeof(szKey), "%d", (int)nDefIndex );
    const char *pszEra = s_pEraTable->GetString( szKey, NULL );
    return pszEra ? Q_atoi( pszEra ) : 0;
}

// Returns the era floor imposed by an item's quality, or 0 if none.
static int TF2VGetQualityEraFloor( int nQuality )
{
    if ( !s_pEraTable )
        return 0;

    const char *pszKey = NULL;
    switch ( nQuality )
    {
    case AE_UNUSUAL:        pszKey = "_q_unusual";        break;
    case AE_STRANGE:        pszKey = "_q_strange";        break;
    case AE_HAUNTED:        pszKey = "_q_haunted";        break;
    case AE_COLLECTORS:     pszKey = "_q_collectors";     break;
    case AE_PAINTKITWEAPON: pszKey = "_q_paintkitweapon"; break;
    default:                return 0;
    }

    const char *pszEra = s_pEraTable->GetString( pszKey, NULL );
    return pszEra ? Q_atoi( pszEra ) : 0;
}

// Returns the highest attribute era floor on this item, or 0 if none.
static int TF2VGetAttributeEraFloor( CEconItemView *pItem )
{
    if ( !s_pEraTable || !pItem )
        return 0;

    static const struct { const char *pszAttr; const char *pszKey; }
    s_AttrChecks[] =
    {
        { "kill eater",             "_a_kill_eater"            },  // stat clock
        { "set item texture wear",  "_a_set_item_texture_wear" },  // war paint / wear
        { "halloween spell type",   "_a_spell_type"            },  // halloween spells
    };

    int nFloor = 0;
    for ( int i = 0; i < ARRAYSIZE( s_AttrChecks ); i++ )
    {
        static CSchemaAttributeDefHandle pAttrDef( s_AttrChecks[i].pszAttr );
        if ( pAttrDef && pItem->FindAttribute( pAttrDef ) )
        {
            const char *pszEra = s_pEraTable->GetString( s_AttrChecks[i].pszKey, NULL );
            if ( pszEra )
                nFloor = MAX( nFloor, Q_atoi( pszEra ) );
        }
    }
    return nFloor;
}

// ----------------------------------------------------------------------------
// TF2VGetItemEra — the unified earliest-possible era for this item instance.
//
// Result = MAX( base_item_era, quality_floor, attribute_floor )
//
// Example: stock Rocket Launcher (era 0) with Strange quality (era 112)
//          and a stat clock (era 150) -> earliest valid era is 150.
// ----------------------------------------------------------------------------

int TF2VGetItemEra( CEconItemView *pItem )
{
    TF2VEnsureEraTableLoaded();

    if ( !pItem || !pItem->IsValid() )
        return 0;

    const CEconItemDefinition *pDef = pItem->GetItemDefinition();
    if ( !pDef )
        return 0;

    int nBaseEra  = TF2VGetBaseItemEra( pDef->GetDefinitionIndex() );
    int nQualEra  = TF2VGetQualityEraFloor( pItem->GetItemQuality() );
    int nAttrEra  = TF2VGetAttributeEraFloor( pItem );

    return MAX( nBaseEra, MAX( nQualEra, nAttrEra ) );
}

// ----------------------------------------------------------------------------
// TF2VIsItemEraAllowed — called from CTFPlayer::ItemIsAllowed().
//
// Returns true if the item is valid at the current era.
// Returns false and fills pszReason if the item post-dates the active era.
// pszReason is a static string — do not free.
//
// Only active when tf2v_enforcement >= 2 (weapon gate enforced).
// ----------------------------------------------------------------------------

bool TF2VIsItemEraAllowed( CEconItemView *pItem, const char **pszReason )
{
    // Only enforce at level 2+ (weapon gate)
    if ( !TFGameRules() || tf2v_enforcement.GetInt() < 2 )
        return true;

    if ( !s_pEraTable )
        return true;  // table failed to load — fail open

    int nActiveEra = tf2v_era.GetInt();
    int nItemEra   = TF2VGetItemEra( pItem );

    if ( nItemEra <= nActiveEra )
        return true;

    // Build a human-readable reason for the rejection.
    // This feeds the same rejection path as competitive bans,
    // but with a distinct code so the HUD can show a different message.
    if ( pszReason )
    {
        static char s_szReason[128];
        V_snprintf( s_szReason, sizeof(s_szReason),
                    "#TF2V_Item_Anachronistic_%d",   // localised string per era
                    nItemEra );
        *pszReason = s_szReason;
    }

    DevMsg( "[TF2V] Blocked anachronistic item %d ('%s') — "
            "requires era %d, server is era %d.\n",
            pItem->GetItemDefinition()
                ? (int)pItem->GetItemDefinition()->GetDefinitionIndex() : -1,
            pItem->GetItemDefinition()
                ? pItem->GetItemDefinition()->GetItemBaseName() : "unknown",
            nItemEra, nActiveEra );

    return false;
}

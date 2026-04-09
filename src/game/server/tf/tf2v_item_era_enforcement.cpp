//=============================================================================
// tf2v_item_era_enforcement.cpp
//
// Era-accurate item enforcement for TF2 Vintage.
// Hooks into CTFPlayer::ItemIsAllowed() via TF2VIsItemEraAllowed().
//
// DESIGN:
//   cfg/tf2v_item_eras.vdf maps item def indices to era integers, plus
//   modifier floors prefixed with "_q_" (quality) and "_a_" (attribute).
//
//   TF2VGetItemEra() computes MAX(base_era, quality_floor, attribute_floors)
//   for any item instance — the earliest era where that specific combination
//   of item + modifiers could legitimately exist.
//
//   TF2VIsItemEraAllowed() returns false and fills a CTF2VEraViolation struct
//   explaining which component caused the rejection and when that component
//   was actually introduced, expressed as a human-readable date string rather
//   than an internal era number. This feeds the VGUI ban popup.
//
// SINGLE SOURCE OF TRUTH:
//   All era thresholds for both the allowed-check AND the strip logic read
//   from the same place: VDF override > first_sale_date > 0 (stock).
//   TF2VGetBaseItemEra(), TF2VGetQualityEra(), TF2VGetAttributeEra() are
//   the public accessors that tf2v_era_attributes.cpp uses for stripping,
//   so the two systems can never drift apart.
//
// VIOLATION DISPLAY (CTF2VEraViolation):
//   "Your Strange Rocket Launcher cannot appear in era 50 (Feb 2009).
//    — Base item: Rocket Launcher (Oct 2007) ✓
//    — Quality: Strange quality (Dec 2011) ✗  first appeared Dec 15, 2011"
//
// ITEMS_GAME INTEGRATION:
//   The merge branch's CEconItemDefinition has GetFirstSaleDate() which
//   returns a YYYY/MM/DD string from items_game.txt. The VDF supplements it
//   for items missing first_sale_date and provides modifier floors that
//   items_game doesn't track.
//
//   Priority: VDF era override > first_sale_date derived era > 0 (stock)
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "econ_item_view.h"
#include "econ_item_schema.h"
#include "econ_item_constants.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "tf_gamerules.h"
#include "tf_gamerules_convars.h"
#include "tf2v_item_era_enforcement.h"


// ---------------------------------------------------------------------------
// Era human-readable date table.
// Maps era integer -> display string shown to the player.
// ---------------------------------------------------------------------------
struct TF2VEraInfo_t
{
    int         nEra;
    const char *pszDate;
    const char *pszUpdateName;
};

static const TF2VEraInfo_t s_EraInfoTable[] =
{
    {   0, "October 10, 2007",       "Launch"                    },
    {   1, "October 10, 2007",       "Launch"                    },
    {   7, "January 25, 2008",       "January 2008 patch"        },
    {   8, "February 14, 2008",      "February 2008 patch"       },
    {  10, "April 29, 2008",         "Gold Rush Update"          },
    {  20, "June 19, 2008",          "Pyro Update"               },
    {  30, "August 19, 2008",        "Heavy Update"              },
    {  31, "December 11, 2008",      "December 2008 patch"       },
    {  50, "February 24, 2009",      "Scout Update"              },
    {  60, "May 21, 2009",           "Sniper vs. Spy Update"     },
    {  70, "August 13, 2009",        "Classless Update"          },
    {  71, "September 15, 2009",     "September 2009 patch"      },
    {  80, "December 17, 2009",      "WAR! Update"               },
    {  82, "April 28, 2010",         "April 2010 patch"          },
    {  90, "July 8, 2010",           "Engineer Update"           },
    {  91, "October 27, 2010",       "Scream Fortress 2010"      },
    { 100, "September 30, 2010",     "Mann-Conomy Update"        },
    { 103, "December 17, 2010",      "Australian Christmas 2010" },
    { 104, "January 19, 2011",       "January 2011 patch"        },
    { 105, "April 14, 2011",         "April 2011 patch"          },
    { 110, "June 23, 2011",          "Über Update"               },
    { 111, "August 31, 2011",        "August 2011 patch"         },
    { 112, "December 15, 2011",      "Australian Christmas 2011" },
    { 115, "July 10, 2013",          "July 2013 patch"           },
    { 117, "October–November 2013",  "Scream Fortress 2013"      },
    { 120, "June 27, 2012",          "Pyromania Update"          },
    { 121, "August 15, 2012",        "Mann vs. Machine"          },
    { 130, "June 18, 2014",          "Love & War Update"         },
    { 133, "December 22, 2014",      "Smissmas 2014"             },
    { 140, "July 2, 2015",           "Gun Mettle Update"         },
    { 150, "December 17, 2015",      "Tough Break Update"        },
    { 160, "July 7, 2016",           "Meet Your Match Update"    },
    { 170, "October 20, 2017",       "Jungle Inferno Update"     },
    { 180, "March 28, 2018",         "March 2018 update"         },
    { 181, "October 19, 2018",       "Scream Fortress X"         },
    { 182, "October 10, 2019",       "Scream Fortress XI"        },
    { 185, "October 1, 2020",        "Scream Fortress XII"       },
    { 186, "December 3, 2020",       "Smissmas 2020"             },
    { 187, "October 5, 2021",        "Scream Fortress XIII"      },
    { 188, "December 2, 2021",       "Smissmas 2021"             },
    { 189, "October 5, 2022",        "Scream Fortress XIV"       },
    { 190, "December 1–5, 2022",     "VScript / Smissmas 2022"   },
    { 191, "October 9, 2023",        "Scream Fortress XV"        },
    { 192, "December 7, 2023",       "Smissmas 2023"             },
    { 193, "October 10, 2024",       "Scream Fortress XVI"       },
    { 194, "December 11, 2024",      "Smissmas 2024"             },
    { 200, "February 18, 2025",      "TF2 SDK Release"           },
    { 201, "July 24, 2025",          "Summer 2025"               },
};

// NOTE: The table above contains some out-of-order era numbers (e.g. 120/121
// appear after 117 but are chronologically earlier). We do a full linear scan
// instead of breaking early so the correct best-fit entry is always found
// regardless of insertion order.
static const TF2VEraInfo_t *TF2VGetEraInfo( int nEra )
{
    const TF2VEraInfo_t *pBest = &s_EraInfoTable[0];
    for ( int i = 0; i < ARRAYSIZE(s_EraInfoTable); i++ )
    {
        if ( s_EraInfoTable[i].nEra <= nEra )
            pBest = &s_EraInfoTable[i];
        // No early break — table has non-monotone entries.
    }
    return pBest;
}

// ---------------------------------------------------------------------------
// Era table from VDF
// ---------------------------------------------------------------------------
static bool       s_bEraTableLoaded = false;
static KeyValues *s_pEraTable       = NULL;

static void TF2VEnsureEraTableLoaded()
{
    if ( s_bEraTableLoaded ) return;
    s_bEraTableLoaded = true;

    KeyValues *pKV = new KeyValues( "tf2v_item_eras" );
    if ( !pKV->LoadFromFile( filesystem, "cfg/tf2v_item_eras.vdf", "MOD" ) )
    {
        Warning( "[TF2V] cfg/tf2v_item_eras.vdf not found — "
                 "era item enforcement disabled.\n" );
        pKV->deleteThis();
        return;
    }
    s_pEraTable = pKV;
}

void TF2VReloadItemEraTable()
{
    if ( s_pEraTable ) { s_pEraTable->deleteThis(); s_pEraTable = NULL; }
    s_bEraTableLoaded = false;
    TF2VEnsureEraTableLoaded();
    Msg( "[TF2V] Item era table reloaded.\n" );
}

CON_COMMAND( tf2v_reload_item_eras,
    "Reload cfg/tf2v_item_eras.vdf without restarting the server." )
{
    TF2VReloadItemEraTable();
}

// ---------------------------------------------------------------------------
// Parse first_sale_date from items_game into an era integer.
// first_sale_date format: "YYYY/MM/DD" or "1960/00/00" if absent.
// ---------------------------------------------------------------------------
static int TF2VDateStringToEra( const char *pszDate )
{
    if ( !pszDate || *pszDate == '\0' || V_strcmp( pszDate, "1960/00/00" ) == 0 )
        return 0;  // no date = stock item = era 0

    int y = 0, m = 1, d = 1;
    if ( sscanf( pszDate, "%d/%d/%d", &y, &m, &d ) < 1 ) return 0;
    if ( m <= 0 ) m = 1;
    if ( d <= 0 ) d = 1;

    int nDateInt = y * 10000 + m * 100 + d;

    static const struct { int nDate; int nEra; } s_DateBounds[] =
    {
        { 20071010,  1 }, { 20080125,  7 }, { 20080214,  8 },
        { 20080429, 10 }, { 20080619, 20 }, { 20080819, 30 },
        { 20081211, 31 }, { 20090224, 50 }, { 20090521, 60 },
        { 20090813, 70 }, { 20090915, 71 }, { 20091217, 80 },
        { 20100428, 82 }, { 20100708, 90 }, { 20101027, 91 },
        { 20100930,100 }, { 20101217,103 }, { 20110119,104 },
        { 20110414,105 }, { 20110623,110 }, { 20110831,111 },
        { 20111215,112 }, { 20130710,115 }, { 20131129,117 },
        { 20140618,130 }, { 20141222,133 }, { 20150702,140 },
        { 20151217,150 }, { 20160707,160 }, { 20171020,170 },
        { 20180328,180 }, { 20181019,181 }, { 20191010,182 },
        { 20191216,183 }, { 20201001,185 }, { 20201203,186 },
        { 20211005,187 }, { 20211202,188 }, { 20221005,189 },
        { 20221205,190 }, { 20230712,191 }, { 20231009,191 },
        { 20231207,192 }, { 20241010,193 }, { 20241211,194 },
        { 20250218,200 }, { 20250724,201 }, { 20251009,202 },
    };

    int nEra = 0;
    for ( int i = 0; i < ARRAYSIZE(s_DateBounds); i++ )
    {
        if ( nDateInt >= s_DateBounds[i].nDate )
            nEra = s_DateBounds[i].nEra;
        else
            break;
    }
    return nEra;
}

// Public wrapper — used by tf2v_era_attributes.cpp
int TF2VDateStringToEra_Public( const char *pszDate )
{
    return TF2VDateStringToEra( pszDate );
}

// ---------------------------------------------------------------------------
// Component lookups — these are now PUBLIC so TF2VStripAnachronisticModifiers
// in tf2v_era_attributes.cpp reads from the same source of truth.
// ---------------------------------------------------------------------------

int TF2VGetBaseItemEra( const CEconItemDefinition *pDef )
{
    if ( !pDef ) return 0;

    TF2VEnsureEraTableLoaded();

    // 1. VDF era override takes highest priority
    if ( s_pEraTable )
    {
        char szKey[16];
        V_snprintf( szKey, sizeof(szKey), "%d", (int)pDef->GetDefinitionIndex() );
        const char *pszEra = s_pEraTable->GetString( szKey, NULL );
        if ( pszEra ) return Q_atoi( pszEra );
    }

    // 2. Parse first_sale_date from items_game
    return TF2VDateStringToEra( pDef->GetFirstSaleDate() );
}

int TF2VGetQualityEra( int nQuality, const char **pszNameOut )
{
    TF2VEnsureEraTableLoaded();
    if ( !s_pEraTable ) return 0;

    struct { int nQuality; const char *pszKey; const char *pszName; } s_QualityMap[] =
    {
        { AE_UNUSUAL,        "_q_unusual",        "Unusual quality"     },
        { AE_STRANGE,        "_q_strange",        "Strange quality"     },
        { AE_HAUNTED,        "_q_haunted",        "Haunted quality"     },
        { AE_COLLECTORS,     "_q_collectors",     "Collector's quality" },
        { AE_PAINTKITWEAPON, "_q_paintkitweapon", "War Paint"           },
    };

    for ( int i = 0; i < ARRAYSIZE(s_QualityMap); i++ )
    {
        if ( s_QualityMap[i].nQuality == nQuality )
        {
            const char *pszEra = s_pEraTable->GetString( s_QualityMap[i].pszKey, NULL );
            if ( pszEra )
            {
                if ( pszNameOut ) *pszNameOut = s_QualityMap[i].pszName;
                return Q_atoi( pszEra );
            }
        }
    }
    return 0;
}

int TF2VGetAttributeEra( const char *pszVDFKey )
{
    TF2VEnsureEraTableLoaded();
    if ( !s_pEraTable || !pszVDFKey ) return 0;
    const char *pszEra = s_pEraTable->GetString( pszVDFKey, NULL );
    return pszEra ? Q_atoi( pszEra ) : 0;
}


// ---------------------------------------------------------------------------
// Attribute floor table — used by both TF2VGetItemEra and TF2VIsItemEraAllowed
// ---------------------------------------------------------------------------
struct AttributeFloor_t
{
    const char *pszAttrName;
    const char *pszVDFKey;
    const char *pszDisplayName;
};

static const AttributeFloor_t s_AttributeFloors[] =
{
    { "kill eater",            "_a_kill_eater",            "Stat clock (kill eater)"  },
    { "set item texture wear", "_a_set_item_texture_wear", "Weapon skin / war paint"  },
    { "halloween spell type",  "_a_spell_type",            "Halloween spell"          },
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int TF2VGetItemEra( CEconItemView *pItem )
{
    TF2VEnsureEraTableLoaded();
    if ( !pItem || !pItem->IsValid() ) return 0;
    const CEconItemDefinition *pDef = pItem->GetItemDefinition();
    if ( !pDef ) return 0;

    int nBase = TF2VGetBaseItemEra( pDef );
    int nQual = TF2VGetQualityEra( pItem->GetItemQuality(), NULL );
    int nAttr = 0;

    if ( s_pEraTable )
    {
        static CSchemaAttributeDefHandle s_pAttrHandles[ARRAYSIZE(s_AttributeFloors)] = {
            CSchemaAttributeDefHandle("kill eater"),
            CSchemaAttributeDefHandle("set item texture wear"),
            CSchemaAttributeDefHandle("halloween spell type"),
        };
        for ( int i = 0; i < ARRAYSIZE(s_AttributeFloors); i++ )
        {
            if ( s_pAttrHandles[i] && pItem->FindAttribute( s_pAttrHandles[i] ) )
            {
                const char *p = s_pEraTable->GetString( s_AttributeFloors[i].pszVDFKey, NULL );
                if ( p ) nAttr = MAX( nAttr, Q_atoi( p ) );
            }
        }
    }

    return MAX( nBase, MAX( nQual, nAttr ) );
}

bool TF2VIsItemEraAllowed( CEconItemView *pItem, CTF2VEraViolation *pViolation )
{
    if ( !TFGameRules() || tf2v_enforcement.GetInt() < 2 )
        return true;

    TF2VEnsureEraTableLoaded();
    if ( !s_pEraTable ) return true;  // fail open

    const CEconItemDefinition *pDef = pItem ? pItem->GetItemDefinition() : NULL;
    if ( !pDef ) return true;

    int nActiveEra = tf2v_era.GetInt();

    int nBaseEra = TF2VGetBaseItemEra( pDef );
    const char *pszQualName = NULL;
    int nQualEra  = TF2VGetQualityEra( pItem->GetItemQuality(), &pszQualName );
    int nWorstEra = MAX( nBaseEra, nQualEra );

    int nWorstAttrEra = 0;
    const char *pszWorstAttrName = NULL;

    if ( s_pEraTable )
    {
        static CSchemaAttributeDefHandle s_pAttrHandles[ARRAYSIZE(s_AttributeFloors)] = {
            CSchemaAttributeDefHandle("kill eater"),
            CSchemaAttributeDefHandle("set item texture wear"),
            CSchemaAttributeDefHandle("halloween spell type"),
        };
        for ( int i = 0; i < ARRAYSIZE(s_AttributeFloors); i++ )
        {
            if ( s_pAttrHandles[i] && pItem->FindAttribute( s_pAttrHandles[i] ) )
            {
                const char *p = s_pEraTable->GetString( s_AttributeFloors[i].pszVDFKey, NULL );
                if ( p )
                {
                    int nFloor = Q_atoi( p );
                    if ( nFloor > nWorstAttrEra )
                    {
                        nWorstAttrEra    = nFloor;
                        pszWorstAttrName = s_AttributeFloors[i].pszDisplayName;
                    }
                }
            }
        }
    }

    nWorstEra = MAX( nWorstEra, nWorstAttrEra );

    if ( nWorstEra <= nActiveEra )
        return true;  // item is valid in this era

    // --- Build violation report ---
    if ( pViolation )
    {
        pViolation->bViolation   = true;
        pViolation->nRequiredEra = nWorstEra;
        pViolation->nActiveEra   = nActiveEra;

        const TF2VEraInfo_t *pActiveInfo = TF2VGetEraInfo( nActiveEra );

        V_snprintf( pViolation->szItemName, sizeof(pViolation->szItemName),
                    "%s", pDef->GetItemBaseName() );
        V_snprintf( pViolation->szActiveEraDate, sizeof(pViolation->szActiveEraDate),
                    "%s", pActiveInfo ? pActiveInfo->pszDate : "?" );

        pViolation->nViolatingComponents = 0;

        auto AddComp = [&]( const char *pszLabel, int nEra, bool bFailing )
        {
            if ( pViolation->nViolatingComponents >= TF2V_MAX_VIOLATION_COMPONENTS )
                return;
            int idx = pViolation->nViolatingComponents++;
            const TF2VEraInfo_t *pInfo = TF2VGetEraInfo( nEra );
            V_snprintf( pViolation->components[idx].szLabel,
                        sizeof(pViolation->components[idx].szLabel), "%s", pszLabel );
            V_snprintf( pViolation->components[idx].szDate,
                        sizeof(pViolation->components[idx].szDate),
                        "%s", pInfo ? pInfo->pszDate : "?" );
            V_snprintf( pViolation->components[idx].szUpdateName,
                        sizeof(pViolation->components[idx].szUpdateName),
                        "%s", pInfo ? pInfo->pszUpdateName : "?" );
            pViolation->components[idx].bFailing = bFailing;
            pViolation->components[idx].nEra     = nEra;
        };

        AddComp( pDef->GetItemBaseName(), nBaseEra, nBaseEra > nActiveEra );
        if ( nQualEra > 0 && pszQualName )
            AddComp( pszQualName, nQualEra, nQualEra > nActiveEra );
        if ( nWorstAttrEra > 0 && pszWorstAttrName )
            AddComp( pszWorstAttrName, nWorstAttrEra, nWorstAttrEra > nActiveEra );
    }

    DevMsg( "[TF2V] Blocked '%s' — requires era %d (%s), server is era %d (%s).\n",
            pDef->GetItemBaseName(), nWorstEra,
            TF2VGetEraInfo(nWorstEra) ? TF2VGetEraInfo(nWorstEra)->pszDate : "?",
            nActiveEra,
            TF2VGetEraInfo(nActiveEra) ? TF2VGetEraInfo(nActiveEra)->pszDate : "?" );

    return false;
}

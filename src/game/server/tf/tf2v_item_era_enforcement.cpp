//=============================================================================
// tf2v_item_era_enforcement.cpp
//
// Era-accurate item enforcement for TF2 Vintage.
//
// ERA SYSTEM — DAY-BASED EPOCH
// ============================================================
// The era integer is now "days since September 17 2007" with Day 1 = that
// date.  Conversion formula:
//
//   day = (date - 2007-09-17) + 1
//
// Examples verified against the request spec:
//   "2007/10/10"  ->  day 24   (PC / Xbox retail launch)
//   "2010/09/30"  ->  day 1110 (Mann-Conomy Update)
//
// TF2VDayFromDate()  — runtime "YYYY/MM/DD" -> day integer (defined here
//                      and declared in tf_gamerules_era_internal.h for use
//                      by ApplyEra and any other TU that needs it).
//
// The static TF2VDateStringToEra() used internally by item enforcement now
// calls TF2VDayFromDate() directly — there is no longer a lookup table.
// An item whose first_sale_date is "2010/09/30" resolves to day 1110;
// an item with "2007/09/17" resolves to day 1; a stock item ("1960/00/00"
// or empty) resolves to 0 (always allowed).
//
// All other logic (VDF override table, quality era floors, attribute era
// floors, violation struct, strip log) is unchanged.
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
#include "tf_gamerules_era_internal.h"
#include "tf2v_item_era_enforcement.h"


// ---------------------------------------------------------------------------
// Era human-readable label table.
//
// Maps a day integer to the nearest named update milestone, used only for
// display text in the VGUI violation popup. The table is "best-fit lower"
// (highest entry <= nDay wins).
//
// Dates are shown to the player as the real calendar date, not the day
// integer, so they remain human-readable.
// ---------------------------------------------------------------------------
struct TF2VEraInfo_t
{
    int         nDay;           // day-since-epoch threshold
    const char *pszDate;        // human-readable display date
    const char *pszUpdateName;  // update/patch name
};

static const TF2VEraInfo_t s_EraInfoTable[] =
{
    {    1, "September 17, 2007",  "Beta Launch"                },
    {   24, "October 10, 2007",   "PC / Xbox Retail Launch"    },
    {   39, "October 25, 2007",   "October 2007 patch"         },
    {   95, "December 20, 2007",  "December 2007 patch"        },
    {  131, "January 25, 2008",   "January 2008 patch"         },
    {  151, "February 14, 2008",  "February 2008 patch"        },
    {  226, "April 29, 2008",     "Gold Rush Update"           },
    {  277, "June 19, 2008",      "Pyro Update"                },
    {  338, "August 19, 2008",    "Heavy Update"               },
    {  452, "December 11, 2008",  "December 2008 patch"        },
    {  500, "January 28, 2009",   "January 2009 patch"         },
    {  505, "February 2, 2009",   "Crit rework patch"          },
    {  527, "February 24, 2009",  "Scout Update"               },
    {  537, "March 6, 2009",      "March 2009 patch"           },
    {  613, "May 21, 2009",       "Sniper vs. Spy Update"      },
    {  631, "June 8, 2009",       "June 2009 patch"            },
    {  697, "August 13, 2009",    "Classless Update"           },
    {  730, "September 15, 2009", "September 2009 patch"       },
    {  823, "December 17, 2009",  "WAR! Update"                },
    {  914, "March 18, 2010",     "Community Update #1"        },
    {  956, "April 29, 2010",     "April 2010 patch"           },
    {  977, "May 20, 2010",       "Community Update #2"        },
    { 1026, "July 8, 2010",       "Engineer Update"            },
    { 1110, "September 30, 2010", "Mann-Conomy Update"         },
    { 1116, "October 6, 2010",    "Polycount Pack"             },
    { 1123, "October 13, 2010",   "Manniversary 2010"          },
    { 1137, "October 27, 2010",   "Scream Fortress 2010"       },
    { 1188, "December 17, 2010",  "Australian Christmas 2010"  },
    { 1221, "January 19, 2011",   "January 2011 patch"         },
    { 1306, "April 14, 2011",     "April 2011 patch"           },
    { 1376, "June 23, 2011",      "Über Update / F2P"          },
    { 1445, "August 31, 2011",    "August 2011 patch"          },
    { 1488, "October 13, 2011",   "Manniversary 2011"          },
    { 1502, "October 27, 2011",   "Halloween 2011"             },
    { 1551, "December 15, 2011",  "Australian Christmas 2011"  },
    { 1628, "March 1, 2012",      "March 2012 patch"           },
    { 1659, "April 1, 2012",      "April 2012 patch"           },
    { 1746, "June 27, 2012",      "Pyromania Update"           },
    { 1795, "August 15, 2012",    "Mann vs. Machine Launch"    },
    { 1867, "October 26, 2012",   "October 2012 patch"         },
    { 2124, "July 10, 2013",      "July 2013 patch"            },
    { 2266, "November 29, 2013",  "Scream Fortress 2013"       },
    { 2467, "June 18, 2014",      "Love & War Update"          },
    { 2654, "December 22, 2014",  "Smissmas 2014"              },
    { 2846, "July 2, 2015",       "Gun Mettle Update"          },
    { 3014, "December 17, 2015",  "Tough Break Update"         },
    { 3217, "July 7, 2016",       "Meet Your Match Update"     },
    { 3687, "October 20, 2017",   "Jungle Inferno Update"      },
    { 3846, "March 28, 2018",     "March 2018 Update"          },
    { 4051, "October 19, 2018",   "Scream Fortress X"          },
    { 4407, "October 10, 2019",   "Scream Fortress XI"         },
    { 4764, "October 1, 2020",    "Scream Fortress XII"        },
    { 4827, "December 3, 2020",   "Smissmas 2020"              },
    { 5133, "October 5, 2021",    "Scream Fortress XIII"       },
    { 5191, "December 2, 2021",   "Smissmas 2021"              },
    { 5498, "October 5, 2022",    "Scream Fortress XIV"        },
    { 5559, "December 5, 2022",   "VScript / Smissmas 2022"    },
    { 5778, "July 12, 2023",      "Summer 2023 (VSH official)" },
    { 5867, "October 9, 2023",    "Scream Fortress XV"         },
    { 5926, "December 7, 2023",   "Smissmas 2023"              },
    { 6234, "October 10, 2024",   "Scream Fortress XVI"        },
    { 6296, "December 11, 2024",  "Smissmas 2024"              },
    { 6365, "February 18, 2025",  "TF2 SDK Release"            },
    { 6521, "July 24, 2025",      "Summer 2025"                },
    { 6598, "October 9, 2025",    "Scream Fortress XVII"       },
};

static const TF2VEraInfo_t *TF2VGetEraInfo( int nDay )
{
    const TF2VEraInfo_t *pBest = &s_EraInfoTable[0];
    for ( int i = 0; i < ARRAYSIZE(s_EraInfoTable); i++ )
    {
        if ( s_EraInfoTable[i].nDay <= nDay )
            pBest = &s_EraInfoTable[i];
        else
            break;
    }
    return pBest;
}


// ---------------------------------------------------------------------------
// CTF2VEraViolation::GetSummary
// ---------------------------------------------------------------------------
const char *CTF2VEraViolation::GetSummary() const
{
    static char s_szBuf[256];
    const TF2VEraInfo_t *pInfo = TF2VGetEraInfo( nActiveEra );
    V_snprintf( s_szBuf, sizeof(s_szBuf),
        "Item not allowed in the active era (day %d — %s).",
        nActiveEra,
        pInfo ? pInfo->pszDate : "unknown date" );
    return s_szBuf;
}


// ---------------------------------------------------------------------------
// Day-based date conversion
//
// TF2VDayFromDate — the authoritative runtime "YYYY/MM/DD" -> day integer
// function.  Uses the proleptic Gregorian calendar.  Returns 0 on error.
//
// This is also declared in tf_gamerules_era_internal.h so that ApplyEra
// and other TUs can call it without pulling in this whole TU.
//
// Algorithm: convert both the epoch anchor (2007/09/17) and the target date
// to a Julian Day Number, subtract, then add 1 so the epoch = Day 1.
// The JDN formula used here (Tondering's) is correct for all Gregorian dates.
// ---------------------------------------------------------------------------

// Internal: Gregorian date -> Julian Day Number (no TF2-specific assumptions)
static int JulianDayNumber( int y, int m, int d )
{
    // Algorithm from Tondering's Calendar FAQ.
    // Valid for all Gregorian dates; overflow safe for years 2000-2100.
    int a = (14 - m) / 12;
    int yy = y + 4800 - a;
    int mm = m + 12 * a - 3;
    return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}

// Epoch anchor as a JDN, computed once.
static const int s_nEpochJDN = JulianDayNumber(
    TF2V_EPOCH_YEAR, TF2V_EPOCH_MONTH, TF2V_EPOCH_DAY );

// Public runtime conversion (definition; declaration in tf_gamerules_era_internal.h)
int TF2VDayFromDate( const char *pszDate )
{
    if ( !pszDate || *pszDate == '\0' ) return 0;

    int y = 0, m = 1, d = 1;
    if ( sscanf( pszDate, "%d/%d/%d", &y, &m, &d ) < 1 ) return 0;
    if ( y < TF2V_EPOCH_YEAR ) return 0;   // pre-TF2 date — treat as stock
    if ( m <= 0 ) m = 1;
    if ( d <= 0 ) d = 1;

    int nTargetJDN = JulianDayNumber( y, m, d );
    int nDay = nTargetJDN - s_nEpochJDN + 1;
    return ( nDay >= 1 ) ? nDay : 0;
}

// Private wrapper used within this TU (keeps call sites identical to before)
static int TF2VDateStringToEra( const char *pszDate )
{
    if ( !pszDate || *pszDate == '\0' )                     return 0;
    if ( V_strcmp( pszDate, "1960/00/00" ) == 0 )           return 0;
    return TF2VDayFromDate( pszDate );
}

// Public wrapper — used by tf2v_era_attributes.cpp
int TF2VDateStringToEra_Public( const char *pszDate )
{
    return TF2VDateStringToEra( pszDate );
}


// ---------------------------------------------------------------------------
// Era table from VDF
//
// cfg/tf2v_item_eras.vdf now stores day integers as era values.
// The format is identical to before — only the integer values change.
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
// Component lookups (public — used by tf2v_era_attributes.cpp so both
// the enforcement check and the strip logic share the same source of truth)
// ---------------------------------------------------------------------------

int TF2VGetBaseItemEra( const CEconItemDefinition *pDef )
{
    if ( !pDef ) return 0;

    TF2VEnsureEraTableLoaded();

    // 1. VDF era override (values must be day integers in the VDF)
    if ( s_pEraTable )
    {
        char szKey[16];
        V_snprintf( szKey, sizeof(szKey), "%d", (int)pDef->GetDefinitionIndex() );
        const char *pszEra = s_pEraTable->GetString( szKey, NULL );
        if ( pszEra ) return Q_atoi( pszEra );
    }

    // 2. Convert first_sale_date from items_game.txt
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
// Attribute floor table — used by TF2VGetItemEra and TF2VIsItemEraAllowed
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
    if ( !pItem ) return 0;

    const CEconItemDefinition *pDef = pItem->GetItemDefinition();
    int nEra = TF2VGetBaseItemEra( pDef );

    // Quality era floor
    {
        const char *pszQualName = NULL;
        int nQualEra = TF2VGetQualityEra( pItem->GetItemQuality(), &pszQualName );
        if ( nQualEra > nEra ) nEra = nQualEra;
    }

    // Attribute era floors
    for ( int i = 0; i < ARRAYSIZE(s_AttributeFloors); i++ )
    {
        static CSchemaAttributeDefHandle hAttr( s_AttributeFloors[i].pszAttrName );
        if ( hAttr && pItem->FindAttribute( hAttr ) )
        {
            int nAttrEra = TF2VGetAttributeEra( s_AttributeFloors[i].pszVDFKey );
            if ( nAttrEra > nEra ) nEra = nAttrEra;
        }
    }

    return nEra;
}


bool TF2VIsItemEraAllowed( CEconItemView *pItem, CTF2VEraViolation *pViolation )
{
    if ( !pItem ) return true;

    int nActiveEra = TFGameRules() ? TFGameRules()->EraState().nAllowedWeaponEra : 0;
    if ( nActiveEra <= 0 ) return true;  // no era gate active

    bool bViolation = false;
    int  nHighestRequired = 0;

    if ( pViolation )
    {
        memset( pViolation, 0, sizeof(*pViolation) );
        pViolation->nActiveEra = nActiveEra;

        const TF2VEraInfo_t *pActiveInfo = TF2VGetEraInfo( nActiveEra );
        if ( pActiveInfo )
            V_strncpy( pViolation->szActiveEraDate,
                       pActiveInfo->pszDate,
                       sizeof(pViolation->szActiveEraDate) );

        const CEconItemDefinition *pDef = pItem->GetItemDefinition();
        if ( pDef )
            V_strncpy( pViolation->szItemName,
                       pDef->GetItemBaseName(),
                       sizeof(pViolation->szItemName) );
    }

    // --- Base item ---
    {
        const CEconItemDefinition *pDef = pItem->GetItemDefinition();
        int nBase = TF2VGetBaseItemEra( pDef );
        if ( nBase > nActiveEra )
        {
            bViolation = true;
            if ( nBase > nHighestRequired ) nHighestRequired = nBase;
            if ( pViolation && pViolation->nViolatingComponents < TF2V_MAX_VIOLATION_COMPONENTS )
            {
                const TF2VEraInfo_t *pInfo = TF2VGetEraInfo( nBase );
                int n = pViolation->nViolatingComponents++;
                V_strncpy( pViolation->components[n].szLabel,
                           "Base item", sizeof(pViolation->components[n].szLabel) );
                V_strncpy( pViolation->components[n].szDate,
                           pInfo ? pInfo->pszDate : "unknown",
                           sizeof(pViolation->components[n].szDate) );
                V_strncpy( pViolation->components[n].szUpdateName,
                           pInfo ? pInfo->pszUpdateName : "",
                           sizeof(pViolation->components[n].szUpdateName) );
                pViolation->components[n].nEra    = nBase;
                pViolation->components[n].bFailing = true;
            }
        }
    }

    // --- Quality ---
    {
        const char *pszQualName = NULL;
        int nQualEra = TF2VGetQualityEra( pItem->GetItemQuality(), &pszQualName );
        if ( nQualEra > nActiveEra )
        {
            bViolation = true;
            if ( nQualEra > nHighestRequired ) nHighestRequired = nQualEra;
            if ( pViolation && pViolation->nViolatingComponents < TF2V_MAX_VIOLATION_COMPONENTS )
            {
                const TF2VEraInfo_t *pInfo = TF2VGetEraInfo( nQualEra );
                int n = pViolation->nViolatingComponents++;
                V_strncpy( pViolation->components[n].szLabel,
                           pszQualName ? pszQualName : "Quality",
                           sizeof(pViolation->components[n].szLabel) );
                V_strncpy( pViolation->components[n].szDate,
                           pInfo ? pInfo->pszDate : "unknown",
                           sizeof(pViolation->components[n].szDate) );
                V_strncpy( pViolation->components[n].szUpdateName,
                           pInfo ? pInfo->pszUpdateName : "",
                           sizeof(pViolation->components[n].szUpdateName) );
                pViolation->components[n].nEra    = nQualEra;
                pViolation->components[n].bFailing = true;
            }
        }
    }

    // --- Attributes ---
    for ( int i = 0; i < ARRAYSIZE(s_AttributeFloors); i++ )
    {
        static CSchemaAttributeDefHandle hAttr( s_AttributeFloors[i].pszAttrName );
        if ( !hAttr || !pItem->FindAttribute( hAttr ) ) continue;

        int nAttrEra = TF2VGetAttributeEra( s_AttributeFloors[i].pszVDFKey );
        if ( nAttrEra > nActiveEra )
        {
            bViolation = true;
            if ( nAttrEra > nHighestRequired ) nHighestRequired = nAttrEra;
            if ( pViolation && pViolation->nViolatingComponents < TF2V_MAX_VIOLATION_COMPONENTS )
            {
                const TF2VEraInfo_t *pInfo = TF2VGetEraInfo( nAttrEra );
                int n = pViolation->nViolatingComponents++;
                V_strncpy( pViolation->components[n].szLabel,
                           s_AttributeFloors[i].pszDisplayName,
                           sizeof(pViolation->components[n].szLabel) );
                V_strncpy( pViolation->components[n].szDate,
                           pInfo ? pInfo->pszDate : "unknown",
                           sizeof(pViolation->components[n].szDate) );
                V_strncpy( pViolation->components[n].szUpdateName,
                           pInfo ? pInfo->pszUpdateName : "",
                           sizeof(pViolation->components[n].szUpdateName) );
                pViolation->components[n].nEra    = nAttrEra;
                pViolation->components[n].bFailing = true;
            }
        }
    }

    if ( pViolation )
    {
        pViolation->bViolation   = bViolation;
        pViolation->nRequiredEra = nHighestRequired;
    }

    return !bViolation;
}

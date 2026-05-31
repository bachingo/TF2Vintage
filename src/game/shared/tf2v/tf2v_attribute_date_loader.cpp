//-----------------------------------------------------------------------------
// Purpose: TF2V: Attribute date loader - CORRECTED FOR DAYS SINCE LAUNCH
// Converts YYYY/MM/DD dates to days since September 16, 2007
//-----------------------------------------------------------------------------

#include "cbase.h"
#include "tf_item_inventory.h"
#include "tf_gamerules.h"
#include "tf2v_attribute_date_loader.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar tf2v_war_result( "tf2v_war_result", "0", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_CHEAT, "Distribution of the Gunboats reward. 0: Soldier, 1: Demoman, 2: Both.", true, 0, true, 2 );

// Global instance
CTF2VAttributeDateManager *g_pTF2VAttributeDateManager = NULL;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTF2VAttributeDateManager::CTF2VAttributeDateManager()
{
	m_bInitialized = false;
	
	m_ItemDates.SetLessFunc( DefLessFunc( int ) );
	m_PaintDates.SetLessFunc( DefLessFunc( int ) );
	m_UnusualEffectDates.SetLessFunc( DefLessFunc( int ) );
	m_WarPaintDates.SetLessFunc( DefLessFunc( int ) );
	m_WeaponAttributeVersions.SetLessFunc( DefLessFunc( int ) );
	m_CommonDefIndex.SetLessFunc( DefLessFunc( int ) );
	m_ItemSetAttributeVersions.SetLessFunc( []( const CUtlString &a, const CUtlString &b ) {
    return V_stricmp( a.Get(), b.Get() ) < 0; });
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CTF2VAttributeDateManager::~CTF2VAttributeDateManager()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Initialize
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::Init()
{
	if ( m_bInitialized )
		return;

	Msg( "[TF2V] Loading attribute introduction dates...\n" );

	bool bItemSuccess = LoadItemDates( "scripts/items/tf2v_item_dates.txt" );
	bool bPaintSuccess = LoadPaintDates( "scripts/items/tf2v_paint_dates.txt" );
	bool bUnusualSuccess = LoadUnusualDates( "scripts/items/tf2v_unusual_dates.txt" );
	bool bWarPaintSuccess = LoadWarPaintDates( "scripts/items/tf2v_warpaint_dates.txt" );
	bool bWeaponSuccess = LoadWeaponAttributeVersions( "scripts/items/tf2v_weapon_attributes.txt" );
	bool bCommonDefSuccess = LoadCommonDefIndex( "scripts/items/tf2v_common_defindex.txt" );
	bool bItemSetSuccess = LoadItemSetAttributeVersions( "scripts/items/tf2v_item_set_attributes.txt" );
	
	if ( bItemSuccess )
		Msg( "[TF2V] Loaded %d items\n", m_ItemDates.Count() );
	else
		Warning( "[TF2V] Failed to load item dates!\n" );

	if ( bPaintSuccess )
		Msg( "[TF2V] Loaded %d paint colors\n", m_PaintDates.Count() );
	else
		Warning( "[TF2V] Failed to load paint dates!\n" );

	if ( bUnusualSuccess )
		Msg( "[TF2V] Loaded %d unusual effects\n", m_UnusualEffectDates.Count() );
	else
		Warning( "[TF2V] Failed to load unusual effect dates!\n" );

	if ( bWarPaintSuccess )
		Msg( "[TF2V] Loaded %d war paints\n", m_WarPaintDates.Count() );
	else
		Warning( "[TF2V] Failed to load war paint dates!\n" );

	if ( bWeaponSuccess )
	{
		Msg( "[TF2V] Loaded attribute versions for %d weapons\n", m_WeaponAttributeVersions.Count() );
	}
	else
	{
		Warning( "[TF2V] Failed to load weapon attribute versions!\n" );
	}
	
	if ( bCommonDefSuccess )
		Msg( "[TF2V] Loaded %d common defindex mappings\n", m_CommonDefIndex.Count() );
	else
		Warning( "[TF2V] Failed to load common defindex mappings!\n" );

	if ( bItemSetSuccess )
    	Msg( "[TF2V] Loaded attribute versions for %d item sets\n", m_ItemSetAttributeVersions.Count() );
	else
    	Warning( "[TF2V] Failed to load item set attribute versions!\n" );

	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::Shutdown()
{
	m_ItemDates.Purge();
	m_PaintDates.Purge();
	m_UnusualEffectDates.Purge();
	m_WarPaintDates.Purge();
	FOR_EACH_MAP_FAST( m_WeaponAttributeVersions, i )
	{
		delete m_WeaponAttributeVersions[i];
	}
	m_WeaponAttributeVersions.Purge();
	m_CommonDefIndex.Purge();
	FOR_EACH_MAP_FAST( m_ItemSetAttributeVersions, i )
	{
		delete m_ItemSetAttributeVersions[i];
	}
	m_ItemSetAttributeVersions.Purge();
	
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Reload
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::ReloadAllDates()
{
	Msg( "[TF2V] Reloading introduction dates...\n" );
	Shutdown();
	Init();
}

//-----------------------------------------------------------------------------
// Convert date string "YYYY/MM/DD" to days since beta
// TF2 Beta: September 17, 2007 (Use September 16th so the 17th is Day 1)
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::ParseDateString( const char *pszDate )
{
	if ( !pszDate || !pszDate[0] )
		return TF2V_DAY_UNKNOWN;

	int iYear = 0, iMonth = 0, iDay = 0;

	// Try format: "YYYY/MM/DD"
	if ( sscanf( pszDate, "%d/%d/%d", &iYear, &iMonth, &iDay ) == 3 )
	{
		return ConvertDateToDaysSinceLaunch( iYear, iMonth, iDay );
	}

	// Try format: "YYYY-MM-DD"
	if ( sscanf( pszDate, "%d-%d-%d", &iYear, &iMonth, &iDay ) == 3 )
	{
		return ConvertDateToDaysSinceLaunch( iYear, iMonth, iDay );
	}

	// Try direct day number
	int iDays = atoi( pszDate );
	if ( iDays >= 0 && iDays < TF2V_DAY_UNKNOWN )
	{
		return iDays;
	}

	Warning( "[TF2V] Failed to parse date: %s\n", pszDate );
	return TF2V_DAY_UNKNOWN;
}

//-----------------------------------------------------------------------------
// Convert calendar date to days since September 16, 2007
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::ConvertDateToDaysSinceLaunch( int iYear, int iMonth, int iDay )
{
	// TF2 launch date
	const int kLaunchYear = 2007;
	const int kLaunchMonth = 9;
	const int kLaunchDay = 16;
	
	static const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	
	// Helper: Is leap year?
	auto isLeapYear = []( int year ) -> bool {
		return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	};
	
	// Helper: Get total days since epoch
	auto getDaysSinceEpoch = [&]( int year, int month, int day ) -> int {
		int totalDays = 0;
		
		// Add days for complete years
		for ( int y = 1; y < year; y++ )
		{
			totalDays += 365;
			if ( isLeapYear( y ) )
				totalDays++;
		}
		
		// Add days for complete months
		for ( int m = 1; m < month; m++ )
		{
			totalDays += daysInMonth[m];
			if ( m == 2 && isLeapYear( year ) )
				totalDays++;
		}
		
		// Add days in current month
		totalDays += day;
		
		return totalDays;
	};
	
	int targetDays = getDaysSinceEpoch( iYear, iMonth, iDay );
	int launchDays = getDaysSinceEpoch( kLaunchYear, kLaunchMonth, kLaunchDay );
	
	return targetDays - launchDays;
}

//-----------------------------------------------------------------------------
// Load item dates
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadItemDates( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_item_dates" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}

	m_ItemDates.Purge();

	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		int iItemDefIndex = atoi( pSub->GetName() );
		int iDays = ParseDateString( pSub->GetString() );

		if ( iItemDefIndex >= 0 && iDays >= 0 )
		{
			m_ItemDates.Insert( iItemDefIndex, iDays );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load paint dates
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadPaintDates( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_paint_dates" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}

	m_PaintDates.Purge();

	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		const char *pszRGBKey = pSub->GetName();
		const char *pszDate = pSub->GetString();

		// Parse RGB (decimal or hex)
		int iRGB = 0;
		if ( V_strnicmp( pszRGBKey, "0x", 2 ) == 0 )
		{
			sscanf( pszRGBKey + 2, "%x", &iRGB );
		}
		else
		{
			iRGB = atoi( pszRGBKey );
		}

		// Convert date to days since launch
		int iDays = ParseDateString( pszDate );

		if ( iRGB > 0 && iDays >= 0 )
		{
			m_PaintDates.Insert( iRGB, iDays );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load unusual dates
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadUnusualDates( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_unusual_dates" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}

	m_UnusualEffectDates.Purge();

	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		int iEffectIndex = atoi( pSub->GetName() );
		int iDays = ParseDateString( pSub->GetString() );

		if ( iEffectIndex >= 0 && iDays >= 0 )
		{
			m_UnusualEffectDates.Insert( iEffectIndex, iDays );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load war paint dates
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadWarPaintDates( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_warpaint_dates" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}

	m_WarPaintDates.Purge();

	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		int iProtoDefIndex = atoi( pSub->GetName() );
		int iDays = ParseDateString( pSub->GetString() );

		if ( iProtoDefIndex >= 0 && iDays >= 0 )
		{
			m_WarPaintDates.Insert( iProtoDefIndex, iDays );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load weapon attribute versions
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadWeaponAttributeVersions( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_weapon_attributes" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}

	// Clear existing data
	FOR_EACH_MAP_FAST( m_WeaponAttributeVersions, i )
	{
		delete m_WeaponAttributeVersions[i];
	}
	m_WeaponAttributeVersions.Purge();

	// Parse each weapon entry
	for ( KeyValues *pWeapon = pKV->GetFirstSubKey(); pWeapon; pWeapon = pWeapon->GetNextKey() )
	{
		int iItemDef = atoi( pWeapon->GetName() );
		
		if ( iItemDef <= 0 )
		{
			Warning( "[TF2V] Invalid item def: %s\n", pWeapon->GetName() );
			continue;
		}

		CUtlVector<WeaponAttributeVersion_t> *pVersions = new CUtlVector<WeaponAttributeVersion_t>();

		// Parse each date block
		for ( KeyValues *pDateBlock = pWeapon->GetFirstSubKey(); pDateBlock; pDateBlock = pDateBlock->GetNextKey() )
		{
			int iDate = ParseDateString( pDateBlock->GetName() );
			
			if ( iDate < 0 )
			{
				Warning( "[TF2V] Invalid date '%s' for item %d\n", pDateBlock->GetName(), iItemDef );
				continue;
			}

			WeaponAttributeVersion_t version;
			version.iStartDate = iDate;
			
			if ( !ParseAttributeBlock( pDateBlock, version.attributes ) )
			{
				Warning( "[TF2V] Failed to parse attributes for item %d date %s\n", iItemDef, pDateBlock->GetName() );
				continue;
			}

			pVersions->AddToTail( version );
		}

		// Sort by date
		pVersions->Sort( []( const WeaponAttributeVersion_t *a, const WeaponAttributeVersion_t *b ) -> int {
			return a->iStartDate - b->iStartDate;
		});

		// After building pVersions, before inserting:
		if ( pVersions->Count() == 0 )
		{
			// Bare entry with no date blocks — not useful, skip it.
			// (These are probably placeholder comments in the data file.)
			Warning( "[TF2V] Item %d has no date blocks, skipping\n", iItemDef );
			delete pVersions;
			continue;
		}

		m_WeaponAttributeVersions.Insert( iItemDef, pVersions );
		
		DevMsg( "[TF2V] Loaded %d versions for item %d\n", pVersions->Count(), iItemDef );
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load item set attribute versions from file
// Format mirrors tf2v_weapon_attributes.txt but keyed by set name string
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadItemSetAttributeVersions( const char *pszFilename )
{
    KeyValues *pKV = new KeyValues( "tf2v_item_set_attributes" );
    if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
    {
        Warning( "[TF2V] Failed to load %s\n", pszFilename );
        pKV->deleteThis();
        return false;
    }

    // Clear existing
    FOR_EACH_MAP_FAST( m_ItemSetAttributeVersions, i )
    {
        delete m_ItemSetAttributeVersions[i];
    }
    m_ItemSetAttributeVersions.Purge();

    for ( KeyValues *pSetKV = pKV->GetFirstSubKey(); pSetKV; pSetKV = pSetKV->GetNextKey() )
    {
        const char *pszSetName = pSetKV->GetName();
        if ( !pszSetName || !pszSetName[0] )
            continue;

        // Validate that this set actually exists in the schema
		const CEconItemSchema::ItemSetMap_t &itemSets = GetItemSchema()->GetItemSets();
		if ( itemSets.Find( pszSetName ) == itemSets.InvalidIndex() )
		{
			Warning( "[TF2V] Item set '%s' not found in schema, skipping\n", pszSetName );
			continue;
		}

        CUtlVector<WeaponAttributeVersion_t> *pVersions = new CUtlVector<WeaponAttributeVersion_t>();

        for ( KeyValues *pDateBlock = pSetKV->GetFirstSubKey(); pDateBlock; pDateBlock = pDateBlock->GetNextKey() )
        {
            int iDate = ParseDateString( pDateBlock->GetName() );
            if ( iDate < 0 )
            {
                Warning( "[TF2V] Invalid date '%s' for item set '%s'\n", pDateBlock->GetName(), pszSetName );
                continue;
            }

            WeaponAttributeVersion_t version;
            version.iStartDate = iDate;

            // Empty blocks are valid — means "no set bonus this era"
            // ParseAttributeBlock returns false for empty blocks, so handle that case:
            KeyValues *pFirstAttrib = pDateBlock->GetFirstSubKey();
            if ( pFirstAttrib )
            {
                if ( !ParseAttributeBlock( pDateBlock, version.attributes ) )
                {
                    Warning( "[TF2V] Failed to parse attributes for set '%s' date '%s'\n",
                             pszSetName, pDateBlock->GetName() );
                    // Still add the version with empty attributes rather than skipping,
                    // so the "no bonus in this era" intent is preserved.
                }
            }
            // else: no subkeys = intentionally empty block, version.attributes stays empty

            pVersions->AddToTail( version );
        }

        // Sort ascending by date so binary/linear search works correctly
        pVersions->Sort( []( const WeaponAttributeVersion_t *a, const WeaponAttributeVersion_t *b ) -> int {
            return a->iStartDate - b->iStartDate;
        });

        m_ItemSetAttributeVersions.Insert( CUtlString( pszSetName ), pVersions );

        DevMsg( "[TF2V] Loaded %d versions for item set '%s'\n", pVersions->Count(), pszSetName );
    }

    pKV->deleteThis();
    return true;
}

//-----------------------------------------------------------------------------
// Returns the era-appropriate attribute list for a named item set.
// Returns NULL if:
//   - No versioned data exists for this set (caller should use schema default)
//   - No version predates the current era (set didn't exist yet; caller blocks bonus)
// Returns a pointer to an EMPTY vector if the set existed but had no bonus at
// this era — caller must treat that as "no bonus", not "use schema default".
//-----------------------------------------------------------------------------
const CUtlVector<CEconItemAttribute> *CTF2VAttributeDateManager::GetItemSetAttributesForEra( const char *pszSetName )
{
    if ( !m_bInitialized || !pszSetName || !TFGameRules() )
        return NULL;

    int iMapIndex = m_ItemSetAttributeVersions.Find( CUtlString( pszSetName ) );
    if ( iMapIndex == m_ItemSetAttributeVersions.InvalidIndex() )
        return NULL; // No override — caller uses schema default

    CUtlVector<WeaponAttributeVersion_t> *pVersionList = m_ItemSetAttributeVersions[iMapIndex];
    if ( !pVersionList || pVersionList->Count() == 0 )
        return NULL;

    // Walk backwards: find latest version that has started by now
    for ( int i = pVersionList->Count() - 1; i >= 0; i-- )
    {
        if ( TF2VIsContemporary( pVersionList->Element(i).iStartDate ) )
        {
            return &pVersionList->Element(i).attributes;
        }
    }

    // All versions post-date current era — set bonus didn't exist yet.
    // Return NULL so caller knows to suppress the bonus entirely.
    return NULL;
}

bool CTF2VAttributeDateManager::ItemSetHasVersionedAttributes( const char *pszSetName )
{
    if ( !m_bInitialized || !pszSetName )
        return false;
    return m_ItemSetAttributeVersions.Find( CUtlString( pszSetName ) ) != m_ItemSetAttributeVersions.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Parse attribute block
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ParseAttributeBlock( KeyValues *pKV, CUtlVector<CEconItemAttribute> &attributes )
{
	if ( !pKV )
		return false;

	attributes.Purge();

	for ( KeyValues *pAttrib = pKV->GetFirstSubKey(); pAttrib; pAttrib = pAttrib->GetNextKey() )
	{
		const char *pszAttributeClass = pAttrib->GetString( "attribute_class", NULL );
		const char *pszValue = pAttrib->GetString( "value", NULL );

		if ( !pszAttributeClass || !pszValue )
		{
			Warning( "[TF2V] Attribute '%s' missing attribute_class or value\n", pAttrib->GetName() );
			continue;
		}

		CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttributeClass );
		if ( !pAttrDef )
		{
			Warning( "[TF2V] Unknown attribute class: %s\n", pszAttributeClass );
			continue;
		}

		// Create attribute using the constructor that takes index and value
		float flValue = atof( pszValue );
		CEconItemAttribute attribute( pAttrDef->GetDefinitionIndex(), flValue );
		
		attributes.AddToTail( attribute );
	}

	return attributes.Count() > 0;
}

//-----------------------------------------------------------------------------
// Load common defindex mappings
// Maps variant item definitions to their base/common definition
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::LoadCommonDefIndex( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "tf2v_common_defindex" );
	if ( !pKV->LoadFromFile( filesystem, pszFilename, "MOD" ) )
	{
		Warning( "[TF2V] Failed to load %s\n", pszFilename );
		pKV->deleteThis();
		return false;
	}
 
	m_CommonDefIndex.Purge();
 
	// Iterate through all key-value pairs
	// Format: "variant_defindex" "base_defindex"
	// Example: "200" "13"  (Festive Scattergun -> Scattergun)
	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		int iVariantDefIndex = atoi( pSub->GetName() );
		int iBaseDefIndex = atoi( pSub->GetString() );
 
		if ( iVariantDefIndex >= 0 && iBaseDefIndex >= 0 )
		{
			m_CommonDefIndex.Insert( iVariantDefIndex, iBaseDefIndex );
		}
	}
 
	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Query functions - return Days since TF2V Epoch (2007/09/16)
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::GetItemIntroductionDate( int iDefindex )
{
	if ( !m_bInitialized )
		return TF2V_DAY_UNKNOWN;

	int idx = m_ItemDates.Find( iDefindex );
	if ( m_ItemDates.IsValidIndex( idx ) )
		return m_ItemDates[idx];

	return TF2V_DAY_UNKNOWN;
}

int CTF2VAttributeDateManager::GetPaintIntroductionDate( int iRGB )
{
	if ( !m_bInitialized )
		return TF2V_DAY_UNKNOWN;

	int idx = m_PaintDates.Find( iRGB );
	if ( m_PaintDates.IsValidIndex( idx ) )
		return m_PaintDates[idx];

	return TF2V_DAY_UNKNOWN; // Unknown = blocked
}

int CTF2VAttributeDateManager::GetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( !m_bInitialized )
		return TF2V_DAY_UNKNOWN;

	int idx = m_UnusualEffectDates.Find( iEffectIndex );
	if ( m_UnusualEffectDates.IsValidIndex( idx ) )
		return m_UnusualEffectDates[idx];

	return TF2V_DAY_UNKNOWN;
}

int CTF2VAttributeDateManager::GetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( !m_bInitialized )
		return TF2V_DAY_UNKNOWN;

	int idx = m_WarPaintDates.Find( iProtoDefIndex );
	if ( m_WarPaintDates.IsValidIndex( idx ) )
		return m_WarPaintDates[idx];

	return TF2V_DAY_UNKNOWN;
}


//-----------------------------------------------------------------------------
// Purpose: TF2V: Simplified time period filtering
// Strips: Paint, Stat Tracking, Killstreaks, Quality (if too new)
// Only replaces entire item if base item is anachronistic
// Defined in sharreddefs.h
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Check medals.
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::IsItemMedal( const CEconItemView *pItem )
{
	if ( !pItem || !pItem->IsValid() )
		return false;
 
	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return false;
 
	// Check if item is in tournament medal range
	if ( pItemDef->GetEquipRegionMask() & GetItemSchema()->GetEquipRegionBitMaskByName( "medal" ) )
	{
		return true;
	}
 
	return false;
}

//-----------------------------------------------------------------------------
// Distinguish schema tints from player-applied tints
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::IsPaintPlayerApplied( const CEconItemView *pItem, const CEconItemAttribute *pPaintAttrib )
{
    if ( !pItem || !pPaintAttrib || !pItem->GetStaticData() )
        return false;

    // Check schema static attributes for a built-in tint — if found, this is
    // a schema-defined color (e.g. stock item with built-in tint), not player-applied.
    const CEconItemDefinition *pDef = pItem->GetStaticData();
    for ( int i = 0; i < pDef->GetStaticAttributes().Count(); i++ )
    {
        const static_attrib_t &schemaAttr = pDef->GetStaticAttributes()[i];
        const CEconItemAttributeDefinition *pAttrDef =
            GetItemSchema()->GetAttributeDefinition( schemaAttr.iDefIndex );
        if ( !pAttrDef )
            continue;

        const char *pszName = pAttrDef->GetDefinitionName();
        if ( V_stristr( pszName, "paint" ) ||
             V_stristr( pszName, "set_item_tint_rgb" ) ||
             V_stristr( pszName, "item_tint_rgb" ) )
            return false; // schema-defined tint, not player-applied
    }

    return true; // no schema tint found, must be player-applied
}

//-----------------------------------------------------------------------------
// Checks to see if item passes base item check
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ItemIsAllowedTimePeriod( const CEconItemView *pItem, int iClass, int iSlot )
{
	if ( !pItem || !pItem->GetStaticData() || !m_bInitialized || !TFGameRules() )
		return false;
	
	// Check if this item is in a slot that doesn't exist yet.
	
	// Cosmetics as a whole did not exist before Sniper vs. Spy
	if ( ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_SNIPER_SPY ) ) && IsWearableSlot( iSlot ) )
	{
		return false;
	}
	
	// Misc slots did not exist prior to Classless
	if ( ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_CLASSLESS ) ) && ( iSlot == LOADOUT_POSITION_MISC ) )
	{
		return false;
	}
	
	// Misc2 did not exist prior to Engineer Update
	if ( ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_ENGINEER ) ) && ( iSlot == LOADOUT_POSITION_MISC2 ) )
	{
		return false;
	}
	
	// Action slots were not used prior to Mannconomy
	if ( ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNCONOMY ) ) && ( iSlot == LOADOUT_POSITION_ACTION ) )
	{
		return false;
	}
	
	// Taunts were not an item slot prior to the Replay Update
	if ( ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_REPLAY ) ) && IsTauntSlot( iSlot ) )
	{
		return false;
	}
	
	int iDefIndex = pItem->GetItemDefIndex();
	// TF2V: Special condition for the Gunboats.
	if ( iDefIndex == 133 )
	{
		if ( iClass == TF_CLASS_DEMOMAN && !tf2v_war_result.GetInt() )
		{
			// Canon timeline: Demoman did not win the war.
			// ClientPrint( this, HUD_PRINTNOTIFY, "#Item_WARResultCanon" );
			return false;
		}
		if ( iClass == TF_CLASS_SOLDIER && tf2v_war_result.GetInt() == 1 )
		{
			// Alternative timeline: Soldier did not win the war.
			// ClientPrint( this, HUD_PRINTNOTIFY, "#Item_WARResultAlternate" );
			return false;
		}
		// On tf2v_war_result == 2, both Soldier and Demoman get it.
	}

	// TF2V: Edge case for the Reserve Shooter.
	if ( iDefIndex == 415 )
	{
		// Pyro didn't get the Reserve Shooter until Manniversary.
		if ( iClass == TF_CLASS_PYRO && TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNIVERSARY ) )
			return false;
	}
	
	// Get the specific date it released as an integer.
	int iItemDate = GetItemIntroductionDate( pItem->GetItemDefIndex() );
	
	// True when the current date is equal or later than its introduction date.
	return TF2VIsContemporary( iItemDate );

}

//-----------------------------------------------------------------------------
// Check if item quality is allowed in current era
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ItemQualityIsAllowedTimePeriod( int iQuality )
{
	if ( !TFGameRules() || !m_bInitialized  )
		return false;

	// Map qualities to their introduction eras
	switch ( iQuality )
	{
		case AE_NORMAL:
			return true; // Stock items (Always available)
		case AE_UNIQUE:
			return TF2VIsContemporary( TF2V_DAY_MAJOR_GOLDRUSH ); // Regular items (If this errors, we have a problem)
		case AE_COMMUNITY:
			return TF2VIsContemporary( TF2V_DAY_MAJOR_WAR ); // Community items
		case AE_SELFMADE:
			return TF2VIsContemporary( TF2V_DAY_CONTENT_FIRST_CONTENT ); // Self Made
		case AE_VINTAGE:
		case AE_DEVELOPER:
		case AE_UNUSUAL: // Unusuals		
			return TF2VIsContemporary( TF2V_DAY_MAJOR_MANNCONOMY ); // Mann-Conomy Update
		case AE_RARITY1:
			return TF2VIsContemporary( TF2V_DAY_PROMO_RIFT ); // Promotional items
		case AE_STRANGE:
			return TF2VIsContemporary( TF2V_DAY_MAJOR_UBER ); // Strange weapons
		case AE_HAUNTED:
			return TF2VIsContemporary( TF2V_DAY_HALLOWEEN_2011 ); // Halloween items
		case AE_COLLECTORS:
			return TF2VIsContemporary( 2249 ); // 9 days before TF2V_DAY_MAJOR_TWOCITIES
		case AE_PAINTKITWEAPON:
		case AE_RARITY_DEFAULT:
		case AE_RARITY_COMMON:
		case AE_RARITY_UNCOMMON:
		case AE_RARITY_RARE:
		case AE_RARITY_MYTHICAL:
		case AE_RARITY_LEGENDARY:
		case AE_RARITY_ANCIENT:
			return TF2VIsContemporary( TF2V_DAY_MAJOR_GUN_METTLE ); // Warpaint items	
		default:
			return false; // Unknown qualities blocked by default
	}
}

//-----------------------------------------------------------------------------
// Strip time-inappropriate attributes from item
// Returns: true if any attributes were removed
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::StripAnachronisticAttributes( CEconItemView *pItem )
{
	if ( !pItem || !pItem->IsValid() || !m_bInitialized || !TFGameRules() )
		return false;

	CAttributeList *pAttribList = pItem->GetAttributeList();
	if ( !pAttribList )
		return false;

	bool bModified = false;

	// Tournament medal check
	bool bIsMedal = IsItemMedal( pItem );
	
	// Iterate backwards so we can safely remove attributes
	for ( int i = pAttribList->GetNumAttributes() - 1; i >= 0; i-- )
	{
		const CEconItemAttribute *pAttrib = pAttribList->GetAttribute( i );
		if ( !pAttrib )
			continue;

		const CEconItemAttributeDefinition *pAttrDef = pAttrib->GetStaticData();
		if ( !pAttrDef )
			continue;

		const char *pszAttrName = pAttrDef->GetDefinitionName();
		bool bShouldRemove = false;
		
		// Paint attributes - introduced with Mann-Conomy
		if ( V_stristr( pszAttrName, "paint" ) || 
			 V_stristr( pszAttrName, "set item tint" ) ||
			 V_stristr( pszAttrName, "item_tint_rgb" ) ||
			 V_stristr( pszAttrName, "item_tint_rgb_2" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNCONOMY ) )
			{
				bShouldRemove = true;
			}
			else if ( !bIsMedal && IsPaintPlayerApplied( pItem, pAttrib ) )
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iRGB = (int)flAttribValue;
				if ( TF2VIsAnachronistic( GetPaintIntroductionDate( iRGB ) ) )
					bShouldRemove = true;
			}
		}
		
		// Unusual effects - introduced with Mann-Conomy
		else if ( V_stristr( pszAttrName, "attach particle effect" ) ||
				  V_stristr( pszAttrName, "unusual_effect" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNCONOMY ) )
			{
				bShouldRemove = true;
			}
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iEffectIndex = (int)flAttribValue;
				if ( TF2VIsAnachronistic( GetUnusualEffectIntroductionDate( iEffectIndex ) ) )
					bShouldRemove = true;
			}
		}
		
		// Stat tracking (Strange counters) - introduced with Mann-Conomy
		else if ( V_stristr( pszAttrName, "kill eater" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_UBER ) )
			{
				bShouldRemove = true;
			}
		}
		
		// Halloween/Haunted effects
		else if ( V_stristr( pszAttrName, "halloween" ) ||
				  V_stristr( pszAttrName, "haunted" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_HALLOWEEN_2011 ) )
			{
				bShouldRemove = true;
			}
		}
		
		// Festive effects - introduced with Australian Christmas 2011
		else if ( V_stristr( pszAttrName, "festive" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_SMISSMAS_2011 ) )
			{
				bShouldRemove = true;
			}
		}
		
		// Killstreak effects - introduced with Two Cities
		else if ( V_stristr( pszAttrName, "killstreak" ) ||
				  V_stristr( pszAttrName, "kill streak" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_TWOCITIES ) )
			{
				bShouldRemove = true;
			}
		}
		
		// Australium - introduced with Two Cities
		else if ( V_stristr( pszAttrName, "australium" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_TWOCITIES ) )
			{
				bShouldRemove = true;
			}
		}
		
		else if ( V_stristr( pszAttrName, "paintkit_proto_def_index" ) ||
				  V_stristr( pszAttrName, "paint_kit_proto_def_index" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_GUN_METTLE ) )
			{
				bShouldRemove = true;
			}
			else
			{
				float flAttribValue = pAttrib->GetValue();
			
				int iProtoDefIndex = (int)flAttribValue;
				int iWarPaintIntroDate = GetWarPaintIntroductionDate( iProtoDefIndex );
			
				if ( TF2VIsAnachronistic( iWarPaintIntroDate ) )
				{
					bShouldRemove = true;
				}
			}
		}
		
		// Stat clock: February 29 2016 (post Tough Break)
		else if ( V_stristr( pszAttrName, "stat_" ) )
		{
			if ( TF2VIsAnachronistic( 3088 ) )
			{
				bShouldRemove = true;
			}
		}
		

		if ( bShouldRemove )
		{
			pAttribList->RemoveAttributeByIndex( i );
			bModified = true;
		}
	}

	return bModified;
}

//-----------------------------------------------------------------------------
// Apply era-appropriate weapon attributes
// This OVERRIDES static_attrs by applying runtime attributes
// Returns if any attributes were stripped
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ApplyWeaponAttributesToItem( CEconItemView *pItem )
{
    if ( !pItem || !pItem->IsValid() || !TFGameRules() || !m_bInitialized )
        return false;

    int iDefIndex = GetCommonItemDef( pItem->GetItemDefIndex() );

    int iMapIndex = m_WeaponAttributeVersions.Find( iDefIndex );
    if ( iMapIndex == m_WeaponAttributeVersions.InvalidIndex() )
        return false;

    CUtlVector<WeaponAttributeVersion_t> *pVersionList = m_WeaponAttributeVersions[iMapIndex];
    if ( !pVersionList || pVersionList->Count() == 0 )
        return false;

    int iCurrentDay = TF2VGetEra();
    int iVersionIndex = -1;
    for ( int i = pVersionList->Count() - 1; i >= 0; i-- )
    {
        if ( pVersionList->Element(i).iStartDate <= iCurrentDay )
        {
            iVersionIndex = i;
            break;
        }
    }
    if ( iVersionIndex == -1 )
        return false;

    WeaponAttributeVersion_t *pCorrectVersion = &pVersionList->Element( iVersionIndex );
    CAttributeList *pAttribList = pItem->GetAttributeList();
    if ( !pAttribList )
        return false;

    // Strategy: write every schema static attribute into the instance list
    // with its era-correct value. The deduplication wrapper in
    // CEconItemView::IterateAttributes will then block the schema from
    // re-emitting any of these, since they now exist in the instance list.
    // Cosmetic instance attributes (paint, unusual, etc.) are never touched.

    const CEconItemDefinition *pDef = pItem->GetStaticData();
    if ( !pDef )
        return false;

    for ( int i = 0; i < pDef->GetStaticAttributes().Count(); i++ )
    {
        const static_attrib_t &schemaAttr = pDef->GetStaticAttributes()[i];
        const CEconItemAttributeDefinition *pAttrDef =
            GetItemSchema()->GetAttributeDefinition( schemaAttr.iDefIndex );
        if ( !pAttrDef )
            continue;

        // Find the era-correct value for this attribute
        float flEraValue = schemaAttr.m_value.asFloat; // default: schema value
        for ( int j = 0; j < pCorrectVersion->attributes.Count(); j++ )
        {
            const CEconItemAttribute &eraAttr = pCorrectVersion->attributes[j];
            if ( eraAttr.GetStaticData() == pAttrDef )
            {
                flEraValue = eraAttr.GetValue();
                break;
            }
        }

        // Write into instance list — this shadows the schema value via
        // the deduplication wrapper. Only adds/updates, never destroys.
        pAttribList->SetRuntimeAttributeValue( pAttrDef, flEraValue );
    }

    return true;
}

//-----------------------------------------------------------------------------
// Quick check if item needs modification for current era
// Returns true if item needs modification (without creating a copy)
// This is a fast validation path to avoid unnecessary item copies
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ItemNeedsModification( const CEconItemView *pItem, int iSlot )
{
	if ( !pItem || !pItem->IsValid() || !TFGameRules() || !m_bInitialized )
		return false;

	// Check 1: Quality too new?
	if ( !ItemQualityIsAllowedTimePeriod( pItem->GetItemQuality() ) )
		return true;
	
	// Check 2: Cosmetic attributes too new?
	if ( HasAnachronisticAttributes( pItem ) )
		return true;
	
	// Check 3: Weapon attributes need era adjustment?
	// Weapons need attribute versioning if we're before the last balance patch
	if ( IsWeaponSlot( iSlot ) && ( TF2VIsAnachronistic( TF2V_DAY_LAST_WEAPON_BALANCE ) ) )
	{
		return true;
	}
	
	// Item is compliant - no modification needed
	return false;
}

//-----------------------------------------------------------------------------
// Get time-period compliant version of item
// Returns true if the item needed modification (pOutItem is populated).
// Returns false if the item is compliant as-is (pOutItem is untouched).
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::GetTimePeriodCompliantItem( 
    const CEconItemView *pOriginalItem, 
    CEconItemView *pOutItem,        // caller-owned, pre-constructed copy
    int iClass, int iSlot )
{
    if ( !pOriginalItem || !pOriginalItem->IsValid() || !TFGameRules() || !m_bInitialized )
        return false;

    if ( !ItemIsAllowedTimePeriod( pOriginalItem, iClass, iSlot ) )
        return false; // Caller handles stock fallback — not our job

    bool bQualityModify   = !ItemQualityIsAllowedTimePeriod( pOriginalItem->GetItemQuality() );
    bool bCosmeticsModify = HasAnachronisticAttributes( pOriginalItem );
    bool bWeaponModify    = IsWeaponSlot( iSlot ) && TF2VIsAnachronistic( TF2V_DAY_LAST_WEAPON_BALANCE );

    if ( !bQualityModify && !bCosmeticsModify && !bWeaponModify )
        return false; // Compliant as-is

    // Populate the caller's copy
    *pOutItem = *pOriginalItem;

    if ( bQualityModify )
        pOutItem->SetItemQuality( AE_UNIQUE );

    if ( bCosmeticsModify )
        StripAnachronisticAttributes( pOutItem );

    if ( bWeaponModify )
        ApplyWeaponAttributesToItem( pOutItem );

    return true;
}

//-----------------------------------------------------------------------------
// Quick check if item has attributes that would be stripped
// Used to avoid unnecessary copying
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::HasAnachronisticAttributes( const CEconItemView *pItem )
{
	if ( !pItem || !pItem->IsValid() || !TFGameRules() || !m_bInitialized )
		return false;

	const CAttributeList *pAttribList = pItem->GetAttributeList();
	if ( !pAttribList )
		return false;

	for ( int i = 0; i < pAttribList->GetNumAttributes(); i++ )
	{
		const CEconItemAttribute *pAttrib = pAttribList->GetAttribute( i );
		if ( !pAttrib )
			continue;

		const CEconItemAttributeDefinition *pAttrDef = pAttrib->GetStaticData();
		if ( !pAttrDef )
			continue;

		const char *pszAttrName = pAttrDef->GetDefinitionName();

		// Check each category
		if ( V_stristr( pszAttrName, "paint" ) || 
			 V_stristr( pszAttrName, "set item tint" ) ||
			 V_stristr( pszAttrName, "item_tint_rgb" ) ||
			 V_stristr( pszAttrName, "item_tint_rgb_2" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNCONOMY ) )
				return true;
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iRGB = (int)flAttribValue;
				if ( TF2VIsAnachronistic( GetPaintIntroductionDate( iRGB ) ) )
					return true;
			}
		}
		else if ( V_stristr( pszAttrName, "attach particle effect" ) ||
				  V_stristr( pszAttrName, "unusual_effect" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNCONOMY ) )
			{
				return true;
			}
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iEffectIndex = (int)flAttribValue;
				if ( TF2VIsAnachronistic( GetUnusualEffectIntroductionDate( iEffectIndex ) ) )
					return true;
			}
		}
		else if ( V_stristr( pszAttrName, "kill eater" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_UBER ) )
				return true;
		}
		else if ( V_stristr( pszAttrName, "halloween" ) ||
				  V_stristr( pszAttrName, "haunted" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_HALLOWEEN_2011 ) )
				return true;
		}
		else if ( V_stristr( pszAttrName, "festive" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_SMISSMAS_2011 ) )
				return true;
		}
		else if ( V_stristr( pszAttrName, "killstreak" ) || V_stristr( pszAttrName, "kill streak" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_TWOCITIES ) )
				return true;
		}
		else if ( V_stristr( pszAttrName, "australium" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_TWOCITIES ) )
				return true;
		}
		else if ( V_stristr( pszAttrName, "paintkit_proto_def_index" ) ||
				  V_stristr( pszAttrName, "paint_kit_proto_def_index" ) )
		{
			if ( TF2VIsAnachronistic( TF2V_DAY_MAJOR_GUN_METTLE ) )
			{
				return true;
			}
			else
			{
				float flAttribValue = pAttrib->GetValue();
			
				int iProtoDefIndex = (int)flAttribValue;
				int iWarPaintIntroDate = GetWarPaintIntroductionDate( iProtoDefIndex );
			
				if ( TF2VIsAnachronistic( iWarPaintIntroDate ) )
				{
					return true;
				}
			}
		}
		else if ( V_stristr( pszAttrName, "stat_" ) )
		{
			if ( TF2VIsAnachronistic( 3088 ) )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Checks if we're a weird variant of an item definition (such as Botkiller)
// And points our attribute table at the common item.
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::GetCommonItemDef( int iDefIndex )
{
	// Look up in the map
	int iIndex = m_CommonDefIndex.Find( iDefIndex );
	
	// If found, return the mapped base defindex
	if ( iIndex != m_CommonDefIndex.InvalidIndex() )
	{
		return m_CommonDefIndex[iIndex];
	}
	
	// If not found in map, this item has no variants - return as-is
	return iDefIndex;
}
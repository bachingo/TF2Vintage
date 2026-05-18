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


ConVar tf2v_war_result( "tf2v_war_result", "0", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Distribution of the Gunboats reward. 0: Soldier, 1: Demoman, 2: Both.", true, 0, true, 2 );

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
		return 0;

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
	if ( iDays >= 0 && iDays <= 99999 )
	{
		return iDays;
	}

	Warning( "[TF2V] Failed to parse date: %s\n", pszDate );
	return 0;
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

		m_WeaponAttributeVersions.Insert( iItemDef, pVersions );
		
		DevMsg( "[TF2V] Loaded %d versions for item %d\n", pVersions->Count(), iItemDef );
	}

	pKV->deleteThis();
	return true;
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

		CEconItemAttribute attribute;
		attribute.SetAttributeDefinition( pAttrDef );
		
		attribute_data_union_t value;
		value.asFloat = atof( pszValue );
		attribute.SetValue( value );
		
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
bool CTF2VAttributeDateManager::IsItemMedal( CEconItemView *pItem )
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
bool CTF2VAttributeDateManager::IsPaintPlayerApplied( CEconItemView *pItem, const CEconItemAttribute *pPaintAttrib )
{
	if ( !pItem || !pPaintAttrib )
		return false;
 
	// Check if this item definition has tint in its schema
	const CAttributeList *pSchemaAttribs = pItem->GetAttributeList();
	if ( pSchemaAttribs )
	{
		for ( int i = 0; i < pSchemaAttribs->GetNumAttributes(); i++ )
		{
			const CEconItemAttribute *pSchemaAttrib = pSchemaAttribs->GetAttribute( i );
			if ( !pSchemaAttrib )
				continue;
 
			const CEconItemAttributeDefinition *pSchemaAttrDef = pSchemaAttrib->GetStaticData();
			if ( !pSchemaAttrDef )
				continue;
 
			// If schema has tint attribute, this is built-in, not player-applied
			if ( V_stristr( pSchemaAttrDef->GetDefinitionName(), "paint" ) ||
			V_stristr( pSchemaAttrDef->GetDefinitionName(), "item" ) ||
			V_stristr( pSchemaAttrDef->GetDefinitionName(), "set_item_tint_rgb" ) ||
			V_stristr( pSchemaAttrDef->GetDefinitionName(), "item_tint_rgb_2" ) )
			{
				return false; // Schema tint - don't filter!
			}
		}
	}
 
	return true; // No schema tint found - must be player-applied
}

//-----------------------------------------------------------------------------
// Attribute category helper
//-----------------------------------------------------------------------------
enum AttributeCategory_t
{
	ATTRIB_CAT_WEAPON_STAT,		// Replace with era version
	ATTRIB_CAT_MODIFIER,		// Strip if anachronistic (done elsewhere)
	ATTRIB_CAT_COSMETIC,		// Always preserve
};

static AttributeCategory_t GetAttributeCategory( const char *pszAttrClass )
{
	if ( !pszAttrClass )
		return ATTRIB_CAT_WEAPON_STAT;

	// Cosmetic attributes - ALWAYS preserve
	if ( V_stristr( pszAttrClass, "min_viewmodel" ) ||
		 V_stristr( pszAttrClass, "custom_projectile_model" ) ||
		 V_stristr( pszAttrClass, "custom_paintjob" ) ||
		 V_stristr( pszAttrClass, "unusual_description" ) ||
		 V_stristr( pszAttrClass, "tournament_item" ) ||
		 V_stristr( pszAttrClass, "item_name" ) ||
		 V_stristr( pszAttrClass, "item_description" ) )
	{
		return ATTRIB_CAT_COSMETIC;
	}

	// Modifier attributes - preserve for now, will be filtered later
	if ( V_stristr( pszAttrClass, "tint_rgb" ) ||
		 V_stristr( pszAttrClass, "particle" ) ||
		 V_stristr( pszAttrClass, "killstreak" ) ||
		 V_stristr( pszAttrClass, "kill_eater" ) ||
		 V_stristr( pszAttrClass, "paintkit" ) ||
		 V_stristr( pszAttrClass, "australium" ) ||
		 V_stristr( pszAttrClass, "festive" ) ||
		 V_stristr( pszAttrClass, "halloween" ) )
	{
		return ATTRIB_CAT_MODIFIER;
	}

	// Everything else is a weapon stat
	return ATTRIB_CAT_WEAPON_STAT;
}

//-----------------------------------------------------------------------------
// Checks to see if item passes base item check
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ItemIsAllowedTimePeriod( CEconItemView *pItem, int iClass, int iSlot )
{
	if ( !pItem || !pItem->GetStaticData() || !TFGameRules() )
		return false;
	
	// TF2V: Special condition for the Gunboats.
	if ( pItem->GetItemDefIndex() == 133 )
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
	if ( pItem->GetItemDefIndex() == 415 )
	{
		// Pyro didn't get the Reserve Shooter until Manniversary.
		if ( iClass == TF_CLASS_PYRO && TF2VIsAnachronistic( TF2V_DAY_MAJOR_MANNIVERSARY ) )
			return false;
	}
	
	return GetItemIntroductionDate(pItem->GetItemDefIndex()) < TFGameRules()->GetTF2VEra();
	
	return false;
}

//-----------------------------------------------------------------------------
// Check if item quality is allowed in current era
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::ItemQualityIsAllowedTimePeriod( int iQuality )
{
	if ( !TFGameRules() )
		return false;

	int iCurrentEra = TFGameRules()->GetTF2VEra();
	
	// Map qualities to their introduction eras
	switch ( iQuality )
	{
		case AE_NORMAL:
			return true; // Stock items (Always available)
		case AE_UNIQUE:
			return iCurrentEra >= TF2V_DAY_MAJOR_GOLDRUSH; // Regular items (If this errors, we have a problem)
		case AE_COMMUNITY:
			return iCurrentEra >= TF2V_DAY_MAJOR_WAR; // Community items
		case AE_SELFMADE:
			return iCurrentEra >= TF2V_DAY_CONTENT_FIRST_CONTENT; // Self Made
		case AE_VINTAGE:
		case AE_DEVELOPER:
		case AE_UNUSUAL: // Unusuals		
			return iCurrentEra >= TF2V_DAY_MAJOR_MANNCONOMY; // Mann-Conomy Update
		case AE_RARITY1:
			return iCurrentEra >= TF2V_DAY_PROMO_RIFT; // Promotional items
		case AE_STRANGE:
			return iCurrentEra >= TF2V_DAY_MAJOR_UBER; // Strange weapons
		case AE_HAUNTED:
			return iCurrentEra >= TF2V_DAY_HALLOWEEN_2011; // Halloween items
		case AE_COLLECTORS:
			return iCurrentEra >= 2249; // 9 days before TF2V_DAY_MAJOR_TWOCITIES
		case AE_PAINTKITWEAPON:
		case AE_RARITY_DEFAULT:
		case AE_RARITY_COMMON:
		case AE_RARITY_UNCOMMON:
		case AE_RARITY_RARE:
		case AE_RARITY_MYTHICAL:
		case AE_RARITY_LEGENDARY:
		case AE_RARITY_ANCIENT:
			return iCurrentEra >= TF2V_DAY_MAJOR_GUN_METTLE; // Warpaint items	
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
	if ( !pItem || !pItem->IsValid() || !TFGameRules() )
		return false;

	CAttributeList *pAttribList = pItem->GetAttributeList();
	if ( !pAttribList )
		return false;

	bool bModified = false;
	int iCurrentEra = TFGameRules()->GetTF2VEra();

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
			if ( iCurrentEra < TF2V_DAY_MAJOR_MANNCONOMY )
			{
				bShouldRemove = true;
			}
			else if ( !bIsMedal && IsPaintPlayerApplied( pItem, pAttrib ) )
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iRGB = (int)flAttribValue;
				if ( iCurrentEra < GetPaintIntroductionDate( iRGB ) )
					bShouldRemove = true;
			}
		}
		
		// Unusual effects - introduced with Mann-Conomy
		else if ( V_stristr( pszAttrName, "attach particle effect" ) ||
				  V_stristr( pszAttrName, "unusual_effect" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_MANNCONOMY )
			{
				bShouldRemove = true;
			}
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iEffectIndex = (int)flAttribValue;
				if ( iCurrentEra < GetUnusualEffectIntroductionDate( iEffectIndex ) )
					bShouldRemove = true;
			}
		}
		
		// Stat tracking (Strange counters) - introduced with Mann-Conomy
		else if ( V_stristr( pszAttrName, "kill eater" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_UBER )
			{
				bShouldRemove = true;
			}
		}
		
		// Halloween/Haunted effects
		else if ( V_stristr( pszAttrName, "halloween" ) ||
				  V_stristr( pszAttrName, "haunted" ) )
		{
			if ( iCurrentEra < TF2V_DAY_HALLOWEEN_2011 )
			{
				bShouldRemove = true;
			}
		}
		
		// Festive effects - introduced with Australian Christmas 2011
		else if ( V_stristr( pszAttrName, "festive" ) )
		{
			if ( iCurrentEra < TF2V_DAY_SMISSMAS_2011 )
			{
				bShouldRemove = true;
			}
		}
		
		// Killstreak effects - introduced with Two Cities
		else if ( V_stristr( pszAttrName, "killstreak" ) ||
				  V_stristr( pszAttrName, "kill streak" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_TWOCITIES )
			{
				bShouldRemove = true;
			}
		}
		
		// Australium - introduced with Two Cities
		else if ( V_stristr( pszAttrName, "australium" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_TWOCITIES )
			{
				bShouldRemove = true;
			}
		}
		
		else if ( V_stristr( pszAttrName, "paintkit_proto_def_index" ) ||
				  V_stristr( pszAttrName, "paint_kit_proto_def_index" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_GUN_METTLE )
			{
				bShouldRemove = true;
			}
			else
			{
				float flAttribValue = pAttrib->GetValue();
			
				int iProtoDefIndex = (int)flAttribValue;
				int iWarPaintIntroDate = GetWarPaintIntroductionDate( iProtoDefIndex );
			
				if ( iCurrentEra < iWarPaintIntroDate )
				{
					bShouldRemove = true;
				}
			}
		}
		
		// Stat clock: February 29 2016 (post Tough Break)
		else if ( V_stristr( pszAttrName, "stat_" ) )
		{
			if ( iCurrentEra < 3088 ) 
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
bool CTF2VAttributeDateManager::ApplyWeaponAttributesToItem( CEconItemView *pOriginalItem, int iCurrentEra )
{
	if ( !pOriginalItem || !pOriginalItem->IsValid() || !m_bInitialized )
		return false;

	// Get the Itemdef for this weapon, along with its base variant.
	int iItemDef = pOriginalItem->GetItemDefIndex();
	
	// Check if we have versioned attributes for this item
	int idx = m_WeaponAttributeVersions.Find( GetCommonItemDef(iItemDef) );
	if ( !m_WeaponAttributeVersions.IsValidIndex( idx ) )
	{
		// No versioned attributes - keep default behavior
		return false;
	}

	CUtlVector<WeaponAttributeVersion_t> *pVersions = m_WeaponAttributeVersions[idx];
	if ( !pVersions || pVersions->Count() == 0 )
		return false;

	// Find correct version for current era
	WeaponAttributeVersion_t *pCorrectVersion = NULL;
	
	for ( int i = pVersions->Count() - 1; i >= 0; i-- )
	{
		if ( (*pVersions)[i].iStartDate <= iCurrentEra )
		{
			pCorrectVersion = &(*pVersions)[i];
			break;
		}
	}

	if ( !pCorrectVersion )
	{
		pCorrectVersion = &(*pVersions)[0];
	}

	CAttributeList *pAttribList = pOriginalItem->GetAttributeList();
	if ( !pAttribList )
		return false;

	// CRITICAL: Save modifier and cosmetic attributes
	CUtlVector<CEconItemAttribute> preservedAttributes;
	
	for ( int i = 0; i < pAttribList->GetNumAttributes(); i++ )
	{
		const CEconItemAttribute *pAttrib = pAttribList->GetAttribute( i );
		if ( !pAttrib )
			continue;

		const CEconItemAttributeDefinition *pAttrDef = pAttrib->GetStaticData();
		if ( !pAttrDef )
			continue;

		const char *pszAttrClass = pAttrDef->GetDefinitionName();
		AttributeCategory_t category = GetAttributeCategory( pszAttrClass );
		
		// Preserve modifiers and cosmetics
		if ( category == ATTRIB_CAT_MODIFIER || category == ATTRIB_CAT_COSMETIC )
		{
			preservedAttributes.AddToTail( *pAttrib );
		}
	}

	// Clear all attributes (this clears weapon stats but we saved modifiers)
	pAttribList->RemoveAllAttributes();

	// Apply era-appropriate weapon stats
	for ( int i = 0; i < pCorrectVersion->attributes.Count(); i++ )
	{
		pAttribList->AddAttribute( &pCorrectVersion->attributes[i] );
	}

	// Restore preserved modifiers and cosmetics
	for ( int i = 0; i < preservedAttributes.Count(); i++ )
	{
		pAttribList->AddAttribute( &preservedAttributes[i] );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Get time-period compliant version of item
// Returns: Modified item, base item if too new, or original if compliant
//-----------------------------------------------------------------------------
CEconItemView *CTF2VAttributeDateManager::GetTimePeriodCompliantItem( CEconItemView *pOriginalItem, int iClass /*= -1*/, int iSlot /*= -1*/)
{
	if ( !pOriginalItem || !pOriginalItem->IsValid() )
		return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
	
	int iCurrentEra = TFGameRules()->GetTF2VEra();
	
	// Check if this item is in a slot that doesn't exist yet.
	
	// Cosmetics as a whole did not exist before Sniper vs. Spy
	if ( ( iCurrentEra < TF2V_DAY_MAJOR_SNIPER_SPY ) && IsWearableSlot(iSlot) )
	{
		return nullptr;
	}
	
	// Misc slots did not exist prior to Classless
	if ( ( iCurrentEra < TF2V_DAY_MAJOR_CLASSLESS ) && ( iSlot == LOADOUT_POSITION_MISC ) )
	{
		return nullptr;
	}
	
	// Misc2 did not exist prior to Engineer Update
	if ( ( iCurrentEra < TF2V_DAY_MAJOR_ENGINEER ) && ( iSlot == LOADOUT_POSITION_MISC2 ) )
	{
		return nullptr;
	}
	
	// Action slots were not used prior to Mannconomy
	if ( ( iCurrentEra < TF2V_DAY_MAJOR_MANNCONOMY ) && ( iSlot == LOADOUT_POSITION_ACTION ) )
	{
		return nullptr;
	}
	
	// Taunts were not an item slot prior to the Replay Update
	if ( ( iCurrentEra < TF2V_DAY_MAJOR_REPLAY ) && IsTauntSlot(iSlot) )
	{
		return nullptr;
	}
	
	// Passes the slot check. Now we have to look into the item's details.
	// STEP 1: Check if base item is allowed at all by comparing the release date to the ingame date
	if ( !ItemIsAllowedTimePeriod( pOriginalItem, iClass, iSlot ) )
	{
		// Base item is too new - replace entirely with stock
		if ( IsWearableSlot( iSlot ) )
			return nullptr;
		else
			return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
	}

	// Base item is allowed, now check for modifications needed
	bool bNeedsModification = false;
	
	// STEP 2: Check quality
	int iOriginalQuality = pOriginalItem->GetItemQuality();
	bool bQualityTooNew = !ItemQualityIsAllowedTimePeriod( iOriginalQuality );
	if ( bQualityTooNew )
	{
		bNeedsModification = true;
	}

	// Check if any of our cosmetic applications (paints, killstreaks, war paints) are too new.
	bool bCosmeticAttributesTooNew = HasAnachronisticAttributes( pOriginalItem );
	if ( bCosmeticAttributesTooNew )
	{
		bNeedsModification = true;
	}
	
	// Check if our weapons need adjusting.
	// We do this based on whether or not the date is after the most recent balance patch.
	bool bWeaponAttributesTooNew = IsValidWeaponSlot( iSlot ) && ( iCurrentEra < TF2V_DAY_LAST_WEAPON_BALANCE );
	if ( bWeaponAttributesTooNew )
	{
		bNeedsModification = true;
	}	

	// STEP 3: Check if any attributes need stripping
	// We'll check this on the copy to avoid modifying original
	if ( !bNeedsModification )
	{
		// Item is completely compliant - return as-is
		return pOriginalItem;
	}

	// STEP 4: Create a modified copy
	// Note: You may want to cache this instead of creating new every time
	CEconItemView *pModifiedItem = new CEconItemView( *pOriginalItem );
	
	// STEP 5: Downgrade quality if needed
	if ( bQualityTooNew )
	{
		pModifiedItem->SetItemQuality( AE_UNIQUE );
	}

	// STEP 6: Strip anachronistic cosmetic attributes
	if ( bCosmeticAttributesTooNew )
	{
		StripAnachronisticAttributes( pModifiedItem );
	}
	
	// STEP 7: Weapons get their attributes changed.
	// Preserve cosmetic and essential attributes.
	if ( bWeaponAttributesTooNew )
	{
		ApplyWeaponAttributesToItem( pModifiedItem, iCurrentEra );
	}

	return pModifiedItem;
}

//-----------------------------------------------------------------------------
// Quick check if item has attributes that would be stripped
// Used to avoid unnecessary copying
//-----------------------------------------------------------------------------
bool CTF2VAttributeDateManager::HasAnachronisticAttributes( CEconItemView *pItem )
{
	if ( !pItem || !pItem->IsValid() || !TFGameRules() )
		return false;

	CAttributeList *pAttribList = pItem->GetAttributeList();
	if ( !pAttribList )
		return false;

	int iCurrentEra = TFGameRules()->GetTF2VEra();
	
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
			if ( iCurrentEra < TF2V_DAY_MAJOR_MANNCONOMY )
				return true;
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iRGB = (int)flAttribValue;
				if ( iCurrentEra < GetPaintIntroductionDate( iRGB ) )
					return true;
			}
		}
		else if ( V_stristr( pszAttrName, "attach particle effect" ) ||
				  V_stristr( pszAttrName, "unusual_effect" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_MANNCONOMY )
			{
				return true;
			}
			else
			{
				// Investigate the permutation further.
				float flAttribValue = pAttrib->GetValue();
				int iEffectIndex = (int)flAttribValue;
				if ( iCurrentEra < GetUnusualEffectIntroductionDate( iEffectIndex ) )
					return true;
			}
		}
		else if ( V_stristr( pszAttrName, "kill eater" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_UBER )
				return true;
		}
		else if ( V_stristr( pszAttrName, "halloween" ) ||
				  V_stristr( pszAttrName, "haunted" ) )
		{
			if ( iCurrentEra < TF2V_DAY_HALLOWEEN_2011 )
				return true;
		}
		else if ( V_stristr( pszAttrName, "festive" ) )
		{
			if ( iCurrentEra < TF2V_DAY_SMISSMAS_2011 )
				return true;
		}
		else if ( V_stristr( pszAttrName, "killstreak" ) || V_stristr( pszAttrName, "kill streak" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_TWOCITIES )
				return true;
		}
		else if ( V_stristr( pszAttrName, "australium" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_TWOCITIES )
				return true;
		}
		else if ( V_stristr( pszAttrName, "paintkit_proto_def_index" ) ||
				  V_stristr( pszAttrName, "paint_kit_proto_def_index" ) )
		{
			if ( iCurrentEra < TF2V_DAY_MAJOR_GUN_METTLE )
			{
				return true;
			}
			else
			{
				float flAttribValue = pAttrib->GetValue();
			
				int iProtoDefIndex = (int)flAttribValue;
				int iWarPaintIntroDate = GetWarPaintIntroductionDate( iProtoDefIndex );
			
				if ( iCurrentEra < iWarPaintIntroDate )
				{
					return true;
				}
			}
		}
		else if ( V_stristr( pszAttrName, "stat_" ) )
		{
			if ( iCurrentEra < 3088 )
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
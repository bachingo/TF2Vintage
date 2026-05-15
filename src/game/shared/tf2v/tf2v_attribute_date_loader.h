//-----------------------------------------------------------------------------
// Purpose: TF2V: Load attribute introduction dates from external files
// Replaces hardcoded static arrays with KeyValues-based file loading
//-----------------------------------------------------------------------------

#ifndef TF2V_ATTRIBUTE_DATE_LOADER_H
#define TF2V_ATTRIBUTE_DATE_LOADER_H

#pragma once

#include "tier1/utlmap.h"
#include "tier1/KeyValues.h"
#include "filesystem.h"

// Forward declarations
class CEconItemView;
class CEconItemAttribute;
class CEconItemAttributeDefinition;

//-----------------------------------------------------------------------------
// Structure to hold a set of attributes for a specific time period
//-----------------------------------------------------------------------------
struct WeaponAttributeVersion_t
{
	int iStartDate;									// Start date (days since launch)
	CUtlVector<CEconItemAttribute> attributes;		// Attributes active during this period
	
	WeaponAttributeVersion_t() { iStartDate = 0; }
};

//-----------------------------------------------------------------------------
// Attribute categories for filtering
//-----------------------------------------------------------------------------
enum AttributeCategory_t
{
	ATTRIB_CAT_WEAPON_STAT,		// Gameplay stats - replace with era version
	ATTRIB_CAT_MODIFIER,		// Player mods - era filter
	ATTRIB_CAT_COSMETIC,		// Visual only - always preserve
	ATTRIB_CAT_CLERICAL,		// System metadata - NEVER touch
};

//-----------------------------------------------------------------------------
// Class to manage attribute introduction dates loaded from files
//-----------------------------------------------------------------------------
class CTF2VAttributeDateManager
{
public:
	CTF2VAttributeDateManager();
	~CTF2VAttributeDateManager();

	// Load all date files on init
	void Init();
	void Shutdown();

	// Query functions - return introduction date in YYYYMMDD format
	// Returns 99999999 if not found (future date = blocked by default)
	
	int GetItemIntroductionDate( int iDefindex );
	int GetPaintIntroductionDate( int iRGB );
	int GetUnusualEffectIntroductionDate( int iEffectIndex );
	int GetWarPaintIntroductionDate( int iProtoDefIndex );

	// Reload files (useful for testing/updates)
	void ReloadAllDates();

private:
	// Load individual files
	bool LoadItemDates( const char *pszFilename );
	bool LoadPaintDates( const char *pszFilename );
	bool LoadUnusualDates( const char *pszFilename );
	bool LoadWarPaintDates( const char *pszFilename );
	bool LoadWeaponAttributeVersions( const char *pszFilename );

	// Helper: Convert date string "YYYY/MM/DD" to integer YYYYMMDD
	int ParseDateString( const char *pszDate );
	
	int ConvertDateToDaysSinceLaunch( int iYear, int iMonth, int iDay );

public:
	// TF2V Item checks
	// Tournament medals first
	bool 				IsItemMedal( CEconItemView *pItem );
	bool 				IsPaintPlayerApplied( CEconItemView *pItem, const CEconItemAttribute *pPaintAttrib );
	
	// Anachronistic modifiers
	bool 				ItemIsAllowedTimePeriod( CEconItemView *pItem, int iClass = -1, int iSlot = -1);
	bool 				ItemQualityIsAllowedTimePeriod( int iQuality );
	bool 				HasAnachronisticAttributes( CEconItemView *pItem );

	bool 				StripAnachronisticAttributes( CEconItemView *pItem );
	
	bool 				ApplyWeaponAttributesToItem( CEconItemView *pOriginalItem, int iCurrentEra );
	
	
	// Item is allowed
	CEconItemView 		*GetTimePeriodCompliantItem( CEconItemView *pOriginalItem, int iClass, int iSlot );

private:
	// Storage maps: key -> date
	CUtlMap<int, int> m_ItemDates;			// Definition Index -> Era Date
	CUtlMap<int, int> m_PaintDates;			// RGB -> Era Date
	CUtlMap<int, int> m_UnusualEffectDates;	// Effect Index -> Era Date
	CUtlMap<int, int> m_WarPaintDates;		// Proto Def Index -> Era Date


	bool m_bInitialized;
};

// Global instance
extern CTF2VAttributeDateManager *g_pTF2VAttributeDateManager;

// Accessor functions (for backwards compatibility with existing code)
inline CEconItemView *TV2VGetTimePeriodCompliantItem( CEconItemView *pOriginalItem, int iClass, int iSlot )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetTimePeriodCompliantItem( pOriginalItem, iClass, iSlot );
	return pOriginalItem;
}

// Accessor functions (for backwards compatibility with existing code)
inline bool TV2VItemIsAllowedTimePeriod( CEconItemView *pItem, int iClass = -1, int iSlot = -1 )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->ItemIsAllowedTimePeriod( pItem, iClass, iSlot );
	return false;
}

// Accessor functions (for backwards compatibility with existing code)
inline bool TV2VItemAttributesAllowedTimePeriod( CEconItemView *pOriginalItem, int iClass, int iSlot )
{
	if ( g_pTF2VAttributeDateManager )
	{
		return ( ItemQualityIsAllowedTimePeriod( pOriginalItem->GetItemQuality() ) && !HasAnachronisticAttributes( pOriginalItem ) );
	}
	return false;
}

// Accessor functions (for backwards compatibility with existing code)
inline int TV2VGetItemIntroductionDate( int iDefindex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetItemIntroductionDate( iDefindex );
	return TF2V_DAY_UNKNOWN;
}

// Accessor functions (for backwards compatibility with existing code)
inline int TV2VGetPaintIntroductionDate( int iRGB )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetPaintIntroductionDate( iRGB );
	return TF2V_DAY_UNKNOWN;
}

inline int TV2VGetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetUnusualEffectIntroductionDate( iEffectIndex );
	return TF2V_DAY_UNKNOWN;
}

inline int TV2VGetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetWarPaintIntroductionDate( iProtoDefIndex );
	return TF2V_DAY_UNKNOWN;
}

#endif // TF2V_ATTRIBUTE_DATE_LOADER_H

//-----------------------------------------------------------------------------
// Purpose: TF2V: Load attribute introduction dates from external files
// Replaces hardcoded static arrays with KeyValues-based file loading
//-----------------------------------------------------------------------------

#ifndef TF2V_ATTRIBUTE_DATE_LOADER_H
#define TF2V_ATTRIBUTE_DATE_LOADER_H

#pragma once

#include "tier1/utlmap.h"
#include "tier1/utlvector.h"
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
	
	WeaponAttributeVersion_t() 
	{ 
		iStartDate = 0; 
	}

	// Explicit copy constructor using a manual element copy loop
	WeaponAttributeVersion_t( const WeaponAttributeVersion_t &other )
	{
		iStartDate = other.iStartDate;
		
		// Clear existing items and copy over the new ones manually
		attributes.Purge();
		for ( int i = 0; i < other.attributes.Count(); ++i )
		{
			attributes.AddToTail( other.attributes[i] );
		}
	}

	// Explicit assignment operator using the same manual copy loop
	WeaponAttributeVersion_t& operator=( const WeaponAttributeVersion_t &other )
	{
		if ( this != &other )
		{
			iStartDate = other.iStartDate;
			
			attributes.Purge();
			for ( int i = 0; i < other.attributes.Count(); ++i )
			{
				attributes.AddToTail( other.attributes[i] );
			}
		}
		return *this;
	}
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

	// Query functions - return introduction date as days since Sept 16, 2007
	// Returns TF2V_DAY_UNKNOWN if not found (future date = blocked by default)
	
	int GetItemIntroductionDate( int iDefindex );
	int GetPaintIntroductionDate( int iRGB );
	int GetUnusualEffectIntroductionDate( int iEffectIndex );
	int GetWarPaintIntroductionDate( int iProtoDefIndex );

	// Reload files (useful for testing/updates)
	void ReloadAllDates();

	// Helper: Convert date string "YYYY/MM/DD" to days since launch
	int ParseDateString( const char *pszDate );
	int ConvertDateToDaysSinceLaunch( int iYear, int iMonth, int iDay );

	// TF2V Item checks
	// Tournament medals first
	bool IsItemMedal( CEconItemView *pItem );
	bool IsPaintPlayerApplied( CEconItemView *pItem, const CEconItemAttribute *pPaintAttrib );
	
	// Anachronistic modifiers
	bool ItemIsAllowedTimePeriod( const CEconItemView *pItem, int iClass = -1, int iSlot = -1 );
	bool ItemQualityIsAllowedTimePeriod( int iQuality );
	bool HasAnachronisticAttributes( CEconItemView *pItem );
	bool StripAnachronisticAttributes( CEconItemView *pItem );
	
	// Quick validation check - returns true if item needs modification
	bool ItemNeedsModification( CEconItemView *pItem, int iSlot );
	
	// Weapon attribute versioning
	bool ApplyWeaponAttributesToItem( CEconItemView *pItem );
	
	// Common item def lookup
	int GetCommonItemDef( int iDefIndex );
	
	// Item is allowed - main entry point
	bool GetTimePeriodCompliantItem( const CEconItemView *pOriginalItem, CEconItemView *pOutItem, int iClass, int iSlot );

	// Item Sets
	const CUtlVector<CEconItemAttribute> *GetItemSetAttributesForEra( const char *pszSetName );
	bool ItemSetHasVersionedAttributes( const char *pszSetName );

private:
	// Load individual files
	bool LoadItemDates( const char *pszFilename );
	bool LoadPaintDates( const char *pszFilename );
	bool LoadUnusualDates( const char *pszFilename );
	bool LoadWarPaintDates( const char *pszFilename );
	bool LoadWeaponAttributeVersions( const char *pszFilename );
	bool ParseAttributeBlock( KeyValues *pKV, CUtlVector<CEconItemAttribute> &attributes );
	bool LoadCommonDefIndex( const char *pszFilename );
	bool LoadItemSetAttributeVersions( const char *pszFilename );

	// Storage maps: key -> date (days since Sept 16, 2007)
	CUtlMap<int, int> m_ItemDates;			// Definition Index -> Era Date
	CUtlMap<int, int> m_PaintDates;			// RGB -> Era Date
	CUtlMap<int, int> m_UnusualEffectDates;	// Effect Index -> Era Date
	CUtlMap<int, int> m_WarPaintDates;		// Proto Def Index -> Era Date
	CUtlMap<int, int> m_CommonDefIndex;		// Def Index Variant -> Common Denominator
	CUtlMap<int, CUtlVector<WeaponAttributeVersion_t>*> m_WeaponAttributeVersions;
	CUtlMap<CUtlString, CUtlVector<WeaponAttributeVersion_t>*> m_ItemSetAttributeVersions;

	bool m_bInitialized;
};

// Global instance
extern CTF2VAttributeDateManager *g_pTF2VAttributeDateManager;

// Accessor functions (for backwards compatibility with existing code)
inline bool TF2VItemNeedsModification( CEconItemView *pItem, int iSlot )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->ItemNeedsModification( pItem, iSlot );
	return false;
}

inline bool TF2VItemIsAllowedTimePeriod( const CEconItemView *pItem, int iClass = -1, int iSlot = -1 )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->ItemIsAllowedTimePeriod( pItem, iClass, iSlot );
	return false;
}

inline bool TF2VItemAttributesAllowedTimePeriod( CEconItemView *pOriginalItem, int iClass, int iSlot )
{
	if ( g_pTF2VAttributeDateManager )
	{
		return ( g_pTF2VAttributeDateManager->ItemQualityIsAllowedTimePeriod( pOriginalItem->GetItemQuality() ) 
				 && !g_pTF2VAttributeDateManager->HasAnachronisticAttributes( pOriginalItem ) );
	}
	return false;
}

inline const CUtlVector<CEconItemAttribute> *TF2VGetItemSetAttributesForEra( const char *pszSetName )
{
    if ( g_pTF2VAttributeDateManager )
        return g_pTF2VAttributeDateManager->GetItemSetAttributesForEra( pszSetName );
    return NULL;
}

inline int TF2VGetItemIntroductionDate( int iDefindex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetItemIntroductionDate( iDefindex );
	return TF2V_DAY_UNKNOWN;
}

inline int TF2VGetPaintIntroductionDate( int iRGB )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetPaintIntroductionDate( iRGB );
	return TF2V_DAY_UNKNOWN;
}

inline int TF2VGetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetUnusualEffectIntroductionDate( iEffectIndex );
	return TF2V_DAY_UNKNOWN;
}

inline int TF2VGetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetWarPaintIntroductionDate( iProtoDefIndex );
	return TF2V_DAY_UNKNOWN;
}

inline int TF2VParseDateString( const char *pszDate )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->ParseDateString( pszDate );
	return TF2V_DAY_UNKNOWN;
}

inline int TF2VConvertDateToDaysSinceLaunch( int iYear, int iMonth, int iDay )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->ConvertDateToDaysSinceLaunch( iYear, iMonth, iDay );
	return TF2V_DAY_UNKNOWN;
}

#endif // TF2V_ATTRIBUTE_DATE_LOADER_H

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

	// Helper: Convert date string "YYYY/MM/DD" to integer YYYYMMDD
	int ParseDateString( const char *pszDate );
	
	int ConvertDateToDaysSinceLaunch( int iYear, int iMonth, int iDay );

public:
	// TF2V Item checks
	// Tournament medals first
	bool 				IsItemMedal( CEconItemView *pItem );
	bool 				IsPaintPlayerApplied( CEconItemView *pItem, const CEconItemAttribute *pPaintAttrib );
	
	// Anachronistic modifiers
	bool 				ItemIsAllowedTimePeriod( CEconItemView *pItem );
	bool 				ItemQualityIsAllowedTimePeriod( int iQuality );
	bool 				HasAnachronisticAttributes( CEconItemView *pItem );

	bool 				StripAnachronisticAttributes( CEconItemView *pItem );
	
	
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
inline CEconItemView GetTimePeriodCompliantItem( CEconItemView *pOriginalItem, int iClass, int iSlot )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetTimePeriodCompliantItem( CEconItemView *pOriginalItem, int iClass, int iSlot );
	return pOriginalItem;
}

// Accessor functions (for backwards compatibility with existing code)
inline int GetItemIntroductionDate( int iRGB )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetItemIntroductionDate( iRGB );
	return 99999999;
}

// Accessor functions (for backwards compatibility with existing code)
inline int GetPaintIntroductionDate( int iRGB )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetPaintIntroductionDate( iRGB );
	return 99999999;
}

inline int GetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetUnusualEffectIntroductionDate( iEffectIndex );
	return 99999999;
}

inline int GetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( g_pTF2VAttributeDateManager )
		return g_pTF2VAttributeDateManager->GetWarPaintIntroductionDate( iProtoDefIndex );
	return 99999999;
}

#endif // TF2V_ATTRIBUTE_DATE_LOADER_H

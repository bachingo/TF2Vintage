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
	int GetPaintIntroductionDate( int iRGB );
	int GetUnusualEffectIntroductionDate( int iEffectIndex );
	int GetWarPaintIntroductionDate( int iProtoDefIndex );

	// Reload files (useful for testing/updates)
	void ReloadAllDates();

private:
	// Load individual files
	bool LoadPaintDates( const char *pszFilename );
	bool LoadUnusualDates( const char *pszFilename );
	bool LoadWarPaintDates( const char *pszFilename );

	// Helper: Convert date string "YYYY/MM/DD" to integer YYYYMMDD
	int ParseDateString( const char *pszDate );

	// Storage maps: key -> date
	CUtlMap<int, int> m_PaintDates;			// RGB -> Date
	CUtlMap<int, int> m_UnusualEffectDates;	// Effect Index -> Date
	CUtlMap<int, int> m_WarPaintDates;		// Proto Def Index -> Date

	bool m_bInitialized;
};

// Global instance
extern CTF2VAttributeDateManager *g_pTF2VAttributeDateManager;

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

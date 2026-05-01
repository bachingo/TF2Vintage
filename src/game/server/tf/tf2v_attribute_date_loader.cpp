//-----------------------------------------------------------------------------
// Purpose: TF2V: Attribute date loader - CORRECTED FOR DAYS SINCE LAUNCH
// Converts YYYY/MM/DD dates to days since September 16, 2007
//-----------------------------------------------------------------------------

#include "cbase.h"
#include "tf2v_attribute_date_loader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CTF2VAttributeDateManager *g_pTF2VAttributeDateManager = NULL;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTF2VAttributeDateManager::CTF2VAttributeDateManager()
{
	m_bInitialized = false;
	
	m_PaintDates.SetLessFunc( DefLessFunc( int ) );
	m_UnusualEffectDates.SetLessFunc( DefLessFunc( int ) );
	m_WarPaintDates.SetLessFunc( DefLessFunc( int ) );
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

	bool bPaintSuccess = LoadPaintDates( "scripts/items/tf2v_paint_dates.txt" );
	bool bUnusualSuccess = LoadUnusualDates( "scripts/items/tf2v_unusual_dates.txt" );
	bool bWarPaintSuccess = LoadWarPaintDates( "scripts/items/tf2v_warpaint_dates.txt" );

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

	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::Shutdown()
{
	m_PaintDates.Purge();
	m_UnusualEffectDates.Purge();
	m_WarPaintDates.Purge();
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Reload
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::ReloadAllDates()
{
	Msg( "[TF2V] Reloading attribute introduction dates...\n" );
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
// Query functions - return DAYS since launch (not YYYYMMDD!)
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::GetPaintIntroductionDate( int iRGB )
{
	if ( !m_bInitialized )
		return 99999;

	int idx = m_PaintDates.Find( iRGB );
	if ( m_PaintDates.IsValidIndex( idx ) )
		return m_PaintDates[idx];

	return 99999; // Unknown = blocked
}

int CTF2VAttributeDateManager::GetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( !m_bInitialized )
		return 99999;

	int idx = m_UnusualEffectDates.Find( iEffectIndex );
	if ( m_UnusualEffectDates.IsValidIndex( idx ) )
		return m_UnusualEffectDates[idx];

	return 99999;
}

int CTF2VAttributeDateManager::GetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( !m_bInitialized )
		return 99999;

	int idx = m_WarPaintDates.Find( iProtoDefIndex );
	if ( m_WarPaintDates.IsValidIndex( idx ) )
		return m_WarPaintDates[idx];

	return 99999;
}

#endif // GAME_DLL

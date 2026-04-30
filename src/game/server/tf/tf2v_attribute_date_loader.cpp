//-----------------------------------------------------------------------------
// Purpose: TF2V: Implementation of attribute date loading from files
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
	
	// Initialize maps with default less function
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
// Initialize and load all date files
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::Init()
{
	if ( m_bInitialized )
		return;

	Msg( "[TF2V] Loading attribute introduction dates...\n" );

	// Load each file
	// These files should be in scripts/items/ alongside items_game.txt
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
// Shutdown and clear all data
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::Shutdown()
{
	m_PaintDates.Purge();
	m_UnusualEffectDates.Purge();
	m_WarPaintDates.Purge();
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Reload all date files
//-----------------------------------------------------------------------------
void CTF2VAttributeDateManager::ReloadAllDates()
{
	Msg( "[TF2V] Reloading attribute introduction dates...\n" );
	Shutdown();
	Init();
}

//-----------------------------------------------------------------------------
// Load paint dates from file
// Format:
// "tf2v_paint_dates"
// {
//     "0x7D4071"    "2010/09/30"    // Optional comment
// }
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

	// Clear existing data
	m_PaintDates.Purge();

	// Parse each paint entry
	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		const char *pszRGBHex = pSub->GetName();	// e.g. "0x7D4071"
		const char *pszDate = pSub->GetString();	// e.g. "2010/09/30"

		// Convert hex RGB string to integer
		int iRGB = 0;
		if ( V_strnicmp( pszRGBHex, "0x", 2 ) == 0 )
		{
			// Parse as hex (skip "0x" prefix)
			sscanf( pszRGBHex + 2, "%x", &iRGB );
		}
		else
		{
			// Try parsing as decimal
			iRGB = atoi( pszRGBHex );
		}

		// Convert date string to integer
		int iDate = ParseDateString( pszDate );

		if ( iRGB > 0 && iDate > 0 )
		{
			m_PaintDates.Insert( iRGB, iDate );
		}
		else
		{
			Warning( "[TF2V] Invalid paint entry: %s = %s\n", pszRGBHex, pszDate );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load unusual effect dates from file
// Format:
// "tf2v_unusual_dates"
// {
//     "1"    "2010/09/30"    // Optional comment
// }
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

	// Clear existing data
	m_UnusualEffectDates.Purge();

	// Parse each unusual effect entry
	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		const char *pszEffectIndex = pSub->GetName();	// e.g. "1", "13", "70"
		const char *pszDate = pSub->GetString();		// e.g. "2010/09/30"

		int iEffectIndex = atoi( pszEffectIndex );
		int iDate = ParseDateString( pszDate );

		if ( iEffectIndex >= 0 && iDate > 0 )
		{
			m_UnusualEffectDates.Insert( iEffectIndex, iDate );
		}
		else
		{
			Warning( "[TF2V] Invalid unusual entry: %s = %s\n", pszEffectIndex, pszDate );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Load war paint dates from file
// Format:
// "tf2v_warpaint_dates"
// {
//     "1"    "2015/07/02"    // Optional comment
// }
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

	// Clear existing data
	m_WarPaintDates.Purge();

	// Parse each war paint entry
	for ( KeyValues *pSub = pKV->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
	{
		const char *pszProtoDefIndex = pSub->GetName();	// e.g. "1", "100", "300"
		const char *pszDate = pSub->GetString();		// e.g. "2015/07/02"

		int iProtoDefIndex = atoi( pszProtoDefIndex );
		int iDate = ParseDateString( pszDate );

		if ( iProtoDefIndex >= 0 && iDate > 0 )
		{
			m_WarPaintDates.Insert( iProtoDefIndex, iDate );
		}
		else
		{
			Warning( "[TF2V] Invalid war paint entry: %s = %s\n", pszProtoDefIndex, pszDate );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Parse a date string "YYYY/MM/DD" into integer YYYYMMDD
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::ParseDateString( const char *pszDate )
{
	if ( !pszDate || !pszDate[0] )
		return 0;

	int iYear = 0, iMonth = 0, iDay = 0;

	// Try format: "YYYY/MM/DD"
	if ( sscanf( pszDate, "%d/%d/%d", &iYear, &iMonth, &iDay ) == 3 )
	{
		return ( iYear * 10000 ) + ( iMonth * 100 ) + iDay;
	}

	// Try format: "YYYY-MM-DD"
	if ( sscanf( pszDate, "%d-%d-%d", &iYear, &iMonth, &iDay ) == 3 )
	{
		return ( iYear * 10000 ) + ( iMonth * 100 ) + iDay;
	}

	// Try format: "YYYYMMDD" (already formatted)
	int iDateInt = atoi( pszDate );
	if ( iDateInt >= 19700101 && iDateInt <= 99991231 )
	{
		return iDateInt;
	}

	Warning( "[TF2V] Failed to parse date: %s\n", pszDate );
	return 0;
}

//-----------------------------------------------------------------------------
// Query functions
//-----------------------------------------------------------------------------
int CTF2VAttributeDateManager::GetPaintIntroductionDate( int iRGB )
{
	if ( !m_bInitialized )
		return 99999999;

	int idx = m_PaintDates.Find( iRGB );
	if ( m_PaintDates.IsValidIndex( idx ) )
	{
		return m_PaintDates[idx];
	}

	// Not found - return future date (will be blocked)
	return 99999999;
}

int CTF2VAttributeDateManager::GetUnusualEffectIntroductionDate( int iEffectIndex )
{
	if ( !m_bInitialized )
		return 99999999;

	int idx = m_UnusualEffectDates.Find( iEffectIndex );
	if ( m_UnusualEffectDates.IsValidIndex( idx ) )
	{
		return m_UnusualEffectDates[idx];
	}

	// Not found - return future date (will be blocked)
	return 99999999;
}

int CTF2VAttributeDateManager::GetWarPaintIntroductionDate( int iProtoDefIndex )
{
	if ( !m_bInitialized )
		return 99999999;

	int idx = m_WarPaintDates.Find( iProtoDefIndex );
	if ( m_WarPaintDates.IsValidIndex( idx ) )
	{
		return m_WarPaintDates[idx];
	}

	// Not found - return future date (will be blocked)
	return 99999999;
}

//-----------------------------------------------------------------------------
// Console commands for debugging
//-----------------------------------------------------------------------------
#ifdef GAME_DLL

CON_COMMAND( tf2v_reload_attribute_dates, "Reload all attribute introduction date files" )
{
	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	g_pTF2VAttributeDateManager->ReloadAllDates();
	Msg( "Attribute dates reloaded.\n" );
}

CON_COMMAND( tf2v_list_paint_dates, "List all loaded paint colors and dates" )
{
	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	Msg( "=== Loaded Paint Colors (%d total) ===\n", g_pTF2VAttributeDateManager->m_PaintDates.Count() );
	
	FOR_EACH_MAP_FAST( g_pTF2VAttributeDateManager->m_PaintDates, i )
	{
		int iRGB = g_pTF2VAttributeDateManager->m_PaintDates.Key( i );
		int iDate = g_pTF2VAttributeDateManager->m_PaintDates[i];
		
		Msg( "  0x%06X -> %d\n", iRGB, iDate );
	}
	
	Msg( "=====================================\n" );
}

CON_COMMAND( tf2v_list_unusual_dates, "List all loaded unusual effects and dates" )
{
	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	Msg( "=== Loaded Unusual Effects (%d total) ===\n", g_pTF2VAttributeDateManager->m_UnusualEffectDates.Count() );
	
	FOR_EACH_MAP_FAST( g_pTF2VAttributeDateManager->m_UnusualEffectDates, i )
	{
		int iEffect = g_pTF2VAttributeDateManager->m_UnusualEffectDates.Key( i );
		int iDate = g_pTF2VAttributeDateManager->m_UnusualEffectDates[i];
		
		Msg( "  Effect #%d -> %d\n", iEffect, iDate );
	}
	
	Msg( "=========================================\n" );
}

CON_COMMAND( tf2v_list_warpaint_dates, "List all loaded war paints and dates" )
{
	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	Msg( "=== Loaded War Paints (%d total) ===\n", g_pTF2VAttributeDateManager->m_WarPaintDates.Count() );
	
	FOR_EACH_MAP_FAST( g_pTF2VAttributeDateManager->m_WarPaintDates, i )
	{
		int iPaint = g_pTF2VAttributeDateManager->m_WarPaintDates.Key( i );
		int iDate = g_pTF2VAttributeDateManager->m_WarPaintDates[i];
		
		Msg( "  War Paint #%d -> %d\n", iPaint, iDate );
	}
	
	Msg( "====================================\n" );
}

CON_COMMAND( tf2v_check_paint, "Check introduction date for a specific paint RGB" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: tf2v_check_paint <RGB_hex>\n" );
		Msg( "Example: tf2v_check_paint 0x7D4071\n" );
		return;
	}

	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	const char *pszRGB = args[1];
	int iRGB = 0;
	
	if ( V_strnicmp( pszRGB, "0x", 2 ) == 0 )
	{
		sscanf( pszRGB + 2, "%x", &iRGB );
	}
	else
	{
		iRGB = atoi( pszRGB );
	}

	int iDate = g_pTF2VAttributeDateManager->GetPaintIntroductionDate( iRGB );
	
	if ( iDate == 99999999 )
	{
		Msg( "Paint color 0x%06X: NOT FOUND (will be blocked)\n", iRGB );
	}
	else
	{
		Msg( "Paint color 0x%06X: Introduced %d\n", iRGB, iDate );
	}
}

CON_COMMAND( tf2v_check_unusual, "Check introduction date for a specific unusual effect" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: tf2v_check_unusual <effect_index>\n" );
		Msg( "Example: tf2v_check_unusual 13\n" );
		return;
	}

	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	int iEffect = atoi( args[1] );
	int iDate = g_pTF2VAttributeDateManager->GetUnusualEffectIntroductionDate( iEffect );
	
	if ( iDate == 99999999 )
	{
		Msg( "Unusual effect #%d: NOT FOUND (will be blocked)\n", iEffect );
	}
	else
	{
		Msg( "Unusual effect #%d: Introduced %d\n", iEffect, iDate );
	}
}

CON_COMMAND( tf2v_check_warpaint, "Check introduction date for a specific war paint" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: tf2v_check_warpaint <proto_def_index>\n" );
		Msg( "Example: tf2v_check_warpaint 1\n" );
		return;
	}

	if ( !g_pTF2VAttributeDateManager )
	{
		Warning( "Attribute date manager not initialized!\n" );
		return;
	}

	int iPaint = atoi( args[1] );
	int iDate = g_pTF2VAttributeDateManager->GetWarPaintIntroductionDate( iPaint );
	
	if ( iDate == 99999999 )
	{
		Msg( "War paint #%d: NOT FOUND (will be blocked)\n", iPaint );
	}
	else
	{
		Msg( "War paint #%d: Introduced %d\n", iPaint, iDate );
	}
}

#endif // GAME_DLL

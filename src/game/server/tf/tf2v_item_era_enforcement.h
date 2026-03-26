//=============================================================================
// tf2v_item_era_enforcement.h
// Include in tf_player.cpp (server-side only, GAME_DLL).
//=============================================================================
#pragma once

class CEconItemView;

// ---------------------------------------------------------------------------
// Violation report — one struct per rejected item.
// Each component explains what part of the item post-dates the active era.
// Populated by TF2VIsItemEraAllowed() when returning false.
// ---------------------------------------------------------------------------
#define TF2V_MAX_VIOLATION_COMPONENTS 4

struct CTF2VEraViolation
{
	bool  bViolation;
	int   nRequiredEra;   // earliest era this item can exist
	int   nActiveEra;     // current server era

	char  szItemName[64];         // base item name (e.g. "Rocket Launcher")
	char  szActiveEraDate[32];    // e.g. "February 24, 2009"

	int   nViolatingComponents;
	struct Component_t
	{
		char szLabel[64];      // e.g. "Rocket Launcher" / "Strange quality" / "Stat clock"
		char szDate[32];       // e.g. "December 15, 2011"
		char szUpdateName[48]; // e.g. "Australian Christmas 2011"
		bool bFailing;         // true if this component post-dates active era
		int  nEra;
	} components[TF2V_MAX_VIOLATION_COMPONENTS];

	CTF2VEraViolation() { memset( this, 0, sizeof(*this) ); }

	// Returns a formatted summary string for the chat/console log.
	// For VGUI use the components array directly.
	const char *GetSummary() const
	{
		static char s_szBuf[512];
		V_snprintf( s_szBuf, sizeof(s_szBuf),
		            "[TF2V] %s cannot appear before %s.",
		            szItemName, components[0].szDate );
		for ( int i = 0; i < nViolatingComponents; i++ )
		{
			if ( components[i].bFailing )
			{
				char szLine[128];
				V_snprintf( szLine, sizeof(szLine),
				            " %s first appeared %s (%s).",
				            components[i].szLabel,
				            components[i].szDate,
				            components[i].szUpdateName );
				V_strcat( s_szBuf, szLine, sizeof(s_szBuf) );
			}
		}
		return s_szBuf;
	}
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Returns the earliest era at which this item instance could exist,
// accounting for base item first_sale_date, quality, and attributes.
int  TF2VGetItemEra( CEconItemView *pItem );

// Returns true if the item is valid at the current server era.
// Fills *pViolation with a detailed breakdown if returning false.
// Only enforces when tf2v_enforcement >= 2. Fails open if VDF not loaded.
bool TF2VIsItemEraAllowed( CEconItemView *pItem,
                           CTF2VEraViolation *pViolation = NULL );


// Expose date->era conversion for use by tf2v_era_attributes.cpp.
// Parses "YYYY/MM/DD" from items_game first_sale_date into an era integer.
// Returns 0 for stock items (no date) or unparseable strings.
int TF2VDateStringToEra_Public( const char *pszDate );
// Reload cfg/tf2v_item_eras.vdf without restarting.
void TF2VReloadItemEraTable();

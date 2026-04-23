//=============================================================================
// tf2v_item_era_enforcement.h
//=============================================================================
#pragma once

#include "econ_item_view.h"

// Maximum violation components shown in the VGUI popup
#define TF2V_MAX_VIOLATION_COMPONENTS 8

struct CTF2VEraViolationComponent
{
    char szLabel[64];
    char szDate[64];
    char szUpdateName[64];
    int  nEra;
    bool bFailing;
};

struct CTF2VEraViolation
{
    bool   bViolation        = false;
    int    nRequiredEra      = 0;
    int    nActiveEra        = 0;
    char   szItemName[64]    = {};
    char   szActiveEraDate[64] = {};
    int    nViolatingComponents = 0;
    CTF2VEraViolationComponent components[TF2V_MAX_VIOLATION_COMPONENTS];

    const char *GetSummary() const;
};

// Reload the era table without restarting the server
void TF2VReloadItemEraTable();

// Compute the earliest era at which pItem (with all its modifiers) is valid.
int  TF2VGetItemEra( CEconItemView *pItem );

// Returns false and fills pViolation if pItem is not legal in the active era.
bool TF2VIsItemEraAllowed( CEconItemView *pItem, CTF2VEraViolation *pViolation );

// Public helper — converts a "YYYY/MM/DD" first_sale_date string to an era int.
int  TF2VDateStringToEra_Public( const char *pszDate );

// Public helper — returns the base era for a definition, respecting VDF overrides.
// Used by TF2VStripAnachronisticModifiers so both systems share the same source.
int  TF2VGetBaseItemEra( const CEconItemDefinition *pDef );

// Public helper — returns the era floor for a quality integer.
// Pass nullptr for pszNameOut if you don't need the display name.
int  TF2VGetQualityEra( int nQuality, const char **pszNameOut = nullptr );

// Public helper — returns the era floor for a named attribute VDF key (e.g. "_a_kill_eater").
// Returns 0 if the key is not in the era table.
int  TF2VGetAttributeEra( const char *pszVDFKey );

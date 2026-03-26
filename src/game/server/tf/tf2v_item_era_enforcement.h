//=============================================================================
// tf2v_item_era_enforcement.h
// Include in tf_player.cpp (server-side only).
//=============================================================================
#pragma once

class CEconItemView;

// Returns the earliest era at which this item instance could exist,
// accounting for base item date, quality, and applied attributes.
int  TF2VGetItemEra( CEconItemView *pItem );

// Returns true if the item is valid at the current server era.
// Fills *pszReason with a localisation key on rejection.
// Only enforces when tf2v_enforcement >= 2.
bool TF2VIsItemEraAllowed( CEconItemView *pItem, const char **pszReason = NULL );

// Reload the era table from cfg/tf2v_item_eras.vdf.
// Exposed for the tf2v_reload_item_eras concommand.
void TF2VReloadItemEraTable();

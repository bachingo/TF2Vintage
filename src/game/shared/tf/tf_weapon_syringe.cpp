//====== Copyright ? 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_syringe.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Syringe tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFSyringe, DT_TFWeaponSyringe)

BEGIN_NETWORK_TABLE(CTFSyringe, DT_TFWeaponSyringe)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFSyringe)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_syringe, CTFSyringe);
PRECACHE_WEAPON_REGISTER(tf_weapon_syringe);

//=============================================================================
//
// Weapon Syringe functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFSyringe::CTFSyringe()
{
}
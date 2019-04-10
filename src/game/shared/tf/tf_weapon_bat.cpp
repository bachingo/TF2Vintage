//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bat.h"
#include "decals.h"
#include "tf_viewmodel.h"
#include "tf_projectile_stunball.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "in_buttons.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#define TF_BAT_DAMAGE 15.0f
#define TF_BAT_VEL 2990.0f
#define TF_BAT_GRAV 1.0f
#define TF_STUNBALL_VIEWMODEL "models/weapons/v_models/v_baseball.mdl"
#define TF_STUNBALL_ADDON 2

//=============================================================================
//
// Weapon Bat 
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBat, DT_TFWeaponBat )

BEGIN_NETWORK_TABLE( CTFBat, DT_TFWeaponBat )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat, CTFBat );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBat::CTFBat()
{
}
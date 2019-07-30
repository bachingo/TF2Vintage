//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shovel.h"
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
// Weapon Shovel tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFShovel, DT_TFWeaponShovel )

BEGIN_NETWORK_TABLE( CTFShovel, DT_TFWeaponShovel )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFShovel )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_shovel, CTFShovel );
PRECACHE_WEAPON_REGISTER( tf_weapon_shovel );

//=============================================================================
//
// Weapon Shovel functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShovel::CTFShovel()
{
}

int CTFShovel::GetCustomDamageType() const
{
	int nShovelWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT( nShovelWeaponMode, set_weapon_mode );
	if ( nShovelWeaponMode == 1 || nShovelWeaponMode == 2 )
		return TF_DMG_CUSTOM_PICKAXE;

	return TF_DMG_CUSTOM_NONE;
}

float CTFShovel::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flDmg = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );

	int nShovelDamageBoost = 0;
	CALL_ATTRIB_HOOK_INT( nShovelDamageBoost, set_weapon_mode );
	if ( nShovelDamageBoost != 1 )
		return flDmg;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0.0f;

	float flFraction = Clamp( (float)pOwner->GetHealth() / pOwner->GetMaxHealth(), 0.0f, 1.0f );
	flDmg *= flFraction * -1.15 + 1.65;

	return flDmg;
}

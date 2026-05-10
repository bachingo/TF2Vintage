//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_fireaxe.h"
#include "tf_gamerules.h"

//=============================================================================
//
// Weapon FireAxe tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFireAxe, DT_TFWeaponFireAxe )

BEGIN_NETWORK_TABLE( CTFFireAxe, DT_TFWeaponFireAxe )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFireAxe )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_fireaxe, CTFFireAxe );
PRECACHE_WEAPON_REGISTER( tf_weapon_fireaxe );

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFireAxe::GetInitialAfterburnDuration() const 
{ 
	int iAddBurningDamageType = 0;
	CALL_ATTRIB_HOOK_INT( iAddBurningDamageType, set_dmgtype_ignite );
	if ( iAddBurningDamageType )
	{
		// TF2V: These were changed in Jungle Inferno.
		return TFGameRules->IsAnachronistic(TF2V_DAY_MAJOR_JUNGLE_INFERNO) ? TF_AFTERBURN_BASE_DURATION_OLD : TF_AFTERBURN_BASE_DURATION_NEW;
	}

	return BaseClass::GetInitialAfterburnDuration();
}
#endif


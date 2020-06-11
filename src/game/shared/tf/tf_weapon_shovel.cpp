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

ConVar tf2v_use_new_split_equalizer("tf2v_use_new_split_equalizer", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Splits the Equalizer and Escape Plan into their modern versions.", true, 0, true, 1);
ConVar tf2v_use_new_equalizer_damage("tf2v_use_new_equalizer_damage", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Makes the Equalizer's damage boost use the newer formula.", true, 0, true, 1);

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

float CTFShovel::GetSpeedMod( void ) const
{
	if ( m_bLowered )
		return 1.0f;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 1.0f;

	int nShovelSpeedBoost = 0;
	CALL_ATTRIB_HOOK_INT( nShovelSpeedBoost, set_weapon_mode );
	if ( !nShovelSpeedBoost || ( nShovelSpeedBoost != 2 && tf2v_use_new_split_equalizer.GetBool() ) )
		return 1.0f;

	float flFraction = (float)pOwner->GetHealth() / pOwner->GetMaxHealth();
	if ( flFraction > 0.8f )
		return 1.0f;
	else if ( flFraction > 0.6f )
		return 1.1f;
	else if ( flFraction > 0.4f )
		return 1.2f;
	else if ( flFraction > 0.2f )
		return 1.4f;
	else
		return 1.6f;
}

float CTFShovel::GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage )
{
	float flDmg = BaseClass::GetMeleeDamage( pTarget, iDamageType, iCustomDamage );

	int nShovelDamageBoost = 0;
	CALL_ATTRIB_HOOK_INT( nShovelDamageBoost, set_weapon_mode );
	if ( !nShovelDamageBoost || ( nShovelDamageBoost != 1 && tf2v_use_new_split_equalizer.GetBool() ) )
		return flDmg;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0.0f;

	float flFraction = Clamp( (float)pOwner->GetHealth() / pOwner->GetMaxHealth(), 0.0f, 1.0f );
	
	// Get the damage output.
	float flDamageOutput = 0;
	if (tf2v_use_new_equalizer_damage.GetBool()) // New algorithm [107.25 - 0.37295 * HP ] converted to ratio output and %HP input.
		flDamageOutput = Clamp( flFraction, 1.65f, 0.5025f );
	else 										 // Old algorithm [162.5 - 0.65 * HP ] converted to ratio output and %HP input.
		flDamageOutput = Clamp( flFraction, 2.5f, 0.5f );
	
	flDmg *= flDamageOutput;

	return flDmg;
}

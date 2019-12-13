//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_syringegun.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Syringe Gun tables.
//

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFSyringeGun )
END_DATADESC()
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFSyringeGun, tf_weapon_syringegun_medic )
CREATE_SIMPLE_WEAPON_TABLE( TFCrossbow, tf_weapon_crossbow )

//=============================================================================
//
// Weapon SyringeGun functions.
//
void CTFSyringeGun::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheTeamParticles("nailtrails_medic_%s");
	PrecacheTeamParticles("nailtrails_medic_%s_crit");
#endif
}

float CTFSyringeGun::GetSpeedMod( void ) const
{
	if ( m_bLowered )
		return 1.0f;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 1.0f;

	float flSyringeGunSpeedBoost = 1.0f;
	CALL_ATTRIB_HOOK_INT( flSyringeGunSpeedBoost, mult_player_movespeed_resource_level );
	if ( ( flSyringeGunSpeedBoost != 1.0f ) && ( pOwner->MedicGetChargeLevel() != 0 ) )
	{
		return ( ( pOwner->MedicGetChargeLevel() / 100.0f ) * flSyringeGunSpeedBoost );
	}
	
	return 1.0f;
}
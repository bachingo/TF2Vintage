//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_shotgun.h"

#if defined CLIENT_DLL
#define CTFShotgun_BuildingRescue C_TFShotgun_BuildingRescue
#endif

class CTFShotgun_BuildingRescue : public CTFShotgun
{
	DECLARE_CLASS( CTFShotgun_BuildingRescue, CTFShotgun )
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual float		GetProjectileSpeed( void );
	virtual float		GetProjectileGravity( void );

	virtual int			GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_BUILDING_RESCUE; }
};

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_BuildingRescue, tf_weapon_shotgun_building_rescue );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFShotgun_BuildingRescue::GetProjectileSpeed( void )
{
	return 2400.f;
	// They perform the below, which doesn't make sense to do when dealing with constants
	/*return RemapValClamped( 0.75f,
	                          0.0f,
	                          TF_BOW_MAX_CHARGE_TIME,
	                          TF_BOW_MIN_CHARGE_VEL,
	                          TF_BOW_MAX_CHARGE_VEL );*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFShotgun_BuildingRescue::GetProjectileGravity( void )
{
	return 0.2f;
	// They perform the below, which doesn't make sense to do when dealing with constants
	/*return RemapValClamped( 0.75f,
	                          0.0f,
	                          TF_BOW_MAX_CHARGE_TIME,
	                          0.5f,
	                          0.1f );*/
}

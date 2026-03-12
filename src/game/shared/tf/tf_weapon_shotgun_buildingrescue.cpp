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

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "functionproxy.h"
#include "toolframework_client.h"

class CProxyBuildingRescueLevel : public CResultProxy
{
public:
	virtual void OnBind( void *pArg );
};


void CProxyBuildingRescueLevel::OnBind( void *pArg )
{
	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer == nullptr )
		return;

	int nTeleportBuildings = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nTeleportBuildings, building_teleporting_pickup );
	if ( nTeleportBuildings == 0 )
		return;

	float flScale = 10.f;
	const int nMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

	if ( nMetal >= nTeleportBuildings )
		flScale = ( 3.f - ( ( ( nMetal - nTeleportBuildings ) / ( pPlayer->GetMaxAmmo( TF_AMMO_METAL ) - nTeleportBuildings ) ) * 3.f ) ) + 1.f;

	VMatrix matrix, output;
	MatrixBuildTranslation( output, -0.5, -0.5, 0.0 );
	MatrixBuildScale( matrix, 1.0, flScale, 1.0 );
	MatrixMultiply( matrix, output, output );
	MatrixBuildTranslation( matrix, 0.5, 0.5, 0.0 );
	MatrixMultiply( matrix, output, output );

	m_pResult->SetMatrixValue( output );

	if ( ToolsEnabled() )
		ToolFramework_RecordMaterialParams( GetMaterial() );
}

EXPOSE_INTERFACE( CProxyBuildingRescueLevel, IMaterialProxy, "BuildingRescueLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif

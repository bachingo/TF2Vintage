//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bonesaw.h"

#ifdef CLIENT_DLL
#include "tf_weapon_medigun.h"
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "c_tf_viewmodeladdon.h"
#endif

//=============================================================================
//
// Weapon Bonesaw tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBonesaw, DT_TFWeaponBonesaw )

BEGIN_NETWORK_TABLE( CTFBonesaw, DT_TFWeaponBonesaw )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBonesaw )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bonesaw, CTFBonesaw );
PRECACHE_WEAPON_REGISTER( tf_weapon_bonesaw );

#ifdef CLIENT_DLL
void C_TFBonesaw::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	UpdateChargePoseParam();
}

bool C_TFBonesaw::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		UpdateChargePoseParam();
		return true;
	}

	return false;
}

void C_TFBonesaw::UpdateChargePoseParam( void )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	C_WeaponMedigun *pMedigun = pOwner->GetMedigun();
	if ( pMedigun )
	{
		SetPoseParameter( "syringe_charge_level", pMedigun->GetChargeLevel() );

		C_ViewmodelAttachmentModel *pAttachment = GetViewmodelAddon();
		if ( pAttachment )
		{
			pAttachment->SetPoseParameter( "syringe_charge_level", pMedigun->GetChargeLevel() );
		}
	}
}
#endif

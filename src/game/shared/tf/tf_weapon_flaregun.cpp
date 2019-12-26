//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED(TFFlareGun, DT_WeaponFlareGun)

BEGIN_NETWORK_TABLE(CTFFlareGun, DT_WeaponFlareGun)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFFlareGun)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_flaregun, CTFFlareGun);
PRECACHE_WEAPON_REGISTER(tf_weapon_flaregun);

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC(CTFFlareGun)
END_DATADESC()
#endif

#define TF_FLARE_MIN_VEL 1200

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFlareGun::CTFFlareGun()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::Spawn(void)
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::PrimaryAttack(void)
{
	//Short and simple attack.
	BaseClass::PrimaryAttack();
		
	//Check if we do anything else, like keep track of flares.
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		// Update our active flares.
		#if 0
		CBaseEntity *pFlare = 

		if (pFlare)
		{
		#ifdef GAME_DLL

			CTFGrenadePipebombProjectile *pFlare = (CTFGrenadePipebombProjectile*)pProjectile;
			pFlare->SetLauncher(this);

			FlareHandle hHandle;
			hHandle = pFlare;
			m_Flares.AddToTail(hHandle);
		#endif
		}
		#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Detonator - Detonate flares in midair
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
#ifdef GAME_DLL
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		if ( !CanAttack() )
			return;

		// Get a valid player.
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return;
		for ( int i = 0; i < m_Flares.Count(); i++ )
		{
			CTFProjectile_Flare *pFlare = m_Flares[ i ];
			pFlare->Detonate();
			m_Flares.Remove(i);
		}
	}
	else
		BaseClass::SecondaryAttack();
#endif
	return;
}


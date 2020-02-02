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
// Purpose: Detonator - Detonate flares in midair
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
	int nWeaponMode = 0;
	//CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		if ( !CanAttack() )
			return;

		// Get a valid player.
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
			return;
#ifdef GAME_DLL
		for ( int i = 0; i < m_Flares.Count(); i++ )
		{
			CTFProjectile_Flare *pFlare = m_Flares[ i ];
			if (pFlare)
			{
				pFlare->Detonate();
				m_Flares.Remove(i);
			}
		}
#endif
	}
	else
		BaseClass::SecondaryAttack();
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Add pipebombs to our list as they're fired
//-----------------------------------------------------------------------------
CBaseEntity *CTFFlareGun::FireProjectile( CTFPlayer *pPlayer )
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
		if ( pProjectile )
		{
		#ifdef GAME_DLL
			FlareHandle hHandle;
			hHandle = (CTFProjectile_Flare *)pProjectile;
			m_Flares.AddToTail( hHandle );
		#endif
		}

		return pProjectile;
	}
	else
		return BaseClass::FireProjectile( pPlayer );
}


//-----------------------------------------------------------------------------
// Purpose: If a flare is exploded, remove from list.
//-----------------------------------------------------------------------------
void CTFFlareGun::DeathNotice(CBaseEntity *pVictim)
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		Assert( dynamic_cast<CTFProjectile_Flare *>( pVictim ) );

		FlareHandle hHandle;
		hHandle = (CTFProjectile_Flare *)pVictim;
		m_Flares.FindAndRemove( hHandle );
		
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGun::HasKnockback() const
{
	int nFlaresHaveKnockback = 0;
	CALL_ATTRIB_HOOK_INT( nFlaresHaveKnockback, set_weapon_mode );
	return nFlaresHaveKnockback == 3;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::AddFlare( CTFProjectile_Flare *pFlare )
{
	FlareHandle hHandle;
	hHandle = pFlare;
	m_Flares.AddToTail( hHandle );
}

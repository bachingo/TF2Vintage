#include "cbase.h"
#include "tf_weapon_laser_pointer.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_baseobject.h"
// Server specific.
#else
#include "tf_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFLaser_Pointer, DT_WeaponLaser_Pointer )

BEGIN_NETWORK_TABLE( CTFLaser_Pointer, DT_WeaponLaser_Pointer )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLaser_Pointer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_laser_pointer, CTFLaser_Pointer );
PRECACHE_WEAPON_REGISTER( tf_weapon_laser_pointer );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFLaser_Pointer )
END_DATADESC()
#endif

CTFLaser_Pointer::CTFLaser_Pointer()
{
#ifdef GAME_DLL
	pGun = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFLaser_Pointer::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		for ( int i = 0; i < pOwner->GetObjectCount(); i++ )
		{

			CBaseObject *pObject = pOwner->GetObject( i );


			if ( pObject->GetType() == OBJ_SENTRYGUN )
			{
				pGun = dynamic_cast< CObjectSentrygun * > ( pObject );
			}
		}
	}
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFLaser_Pointer::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	if ( pGun )
	{

		if ( pGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			pGun->OnStopWrangling();
			pGun->SetShouldFire( false );
		}
	}
	pGun = NULL;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::WeaponReset( void )
{
	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	if ( !pGun )
		return;

	if ( m_flNextPrimaryAttack < gpGlobals->curtime && pGun->GetState() == SENTRY_STATE_WRANGLED )
	{
		pGun->SetShouldFire( true );

		// input buffer
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.05f;
	}
#endif
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::SecondaryAttack( void )
{
	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	if ( !pGun )
		return;

	if ( m_flNextSecondaryAttack <= gpGlobals->curtime && pGun->GetState() == SENTRY_STATE_WRANGLED )
	{
		int iUpgradeLevel = pGun->GetUpgradeLevel();

		if ( iUpgradeLevel == 3 )
		{
			pGun->FireRockets();

			// Rockets fire slightly faster wrangled
			m_flNextSecondaryAttack = gpGlobals->curtime + 2.5;
		}
	}
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::ItemPostFrame( void )
{
#ifdef GAME_DLL
	if ( pGun )
	{
		//TODO: Find a better way to determine if we can wrangle
		if ( !pGun->IsRedeploying() && !pGun->IsBuilding() && !pGun->IsUpgrading() && !pGun->HasSapper() )
		{
			pGun->SetState( SENTRY_STATE_WRANGLED );
		}
	}
#endif

	BaseClass::ItemPostFrame();
}
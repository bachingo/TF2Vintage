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
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vecEnd ) ),
#else
	SendPropVector( SENDINFO ( m_vecEnd ) ),
#endif
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

/*acttable_t CTFLaser_Pointer::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_SECONDARY2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SECONDARY2, false },
	{ ACT_MP_RUN, ACT_MP_RUN_SECONDARY2, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_SECONDARY2, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_SECONDARY2, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_SECONDARY2, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_SECONDARY2, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_SECONDARY2, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_SECONDARY2, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_SECONDARY2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_SECONDARY2, false },
};*/

CTFLaser_Pointer::CTFLaser_Pointer()
{
	pGun = NULL;
#ifndef GAME_DLL
	pLaser = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFLaser_Pointer::Deploy( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		for ( int i = 0; i < pOwner->GetObjectCount(); i++ )
		{
#ifdef GAME_DLL
			CBaseObject *pObject = pOwner->GetObject( i );
#else
			C_BaseObject *pObject = pOwner->GetObject( i );
#endif

			if ( pObject->GetType() == OBJ_SENTRYGUN )
			{
#ifdef GAME_DLL
				pGun = dynamic_cast< CObjectSentrygun * > ( pObject );
#else
				pGun = dynamic_cast< C_ObjectSentrygun * > ( pObject );
#endif
			}
		}
	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFLaser_Pointer::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( pGun )
	{
#ifdef GAME_DLL
		if ( pGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			pGun->OnStopWrangling();
			pGun->SetShouldFire( false );
		}
	}
#else
		if ( pLaser )
		{
			pGun->DestroyLaserBeam();		
		}
	}
	pLaser = NULL;
#endif

	pGun = NULL;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::WeaponReset( void )
{
#ifndef GAME_DLL
	pLaser = NULL;
#endif
	pGun = NULL;
	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::PrimaryAttack( void )
{
	if ( !CanAttack() || !pGun )
		return;

#ifdef GAME_DLL
	if ( m_flNextPrimaryAttack < gpGlobals->curtime && pGun->GetState() == SENTRY_STATE_WRANGLED )
	{
		pGun->SetShouldFire( true );

		// input buffer
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.05f;
	}
#endif
	SendWeaponAnim( ACT_SECONDARY_VM_PRIMARYATTACK_2 );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaser_Pointer::SecondaryAttack( void )
{
	if ( !CanAttack() || !pGun )
		return;

#ifdef GAME_DLL
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
	if ( pGun )
	{
#ifdef GAME_DLL
		//TODO: Find a better way to determine if we can wrangle
		if ( !pGun->IsRedeploying() && !pGun->IsBuilding() && !pGun->IsUpgrading() && !pGun->HasSapper() )
		{
			pGun->SetState( SENTRY_STATE_WRANGLED );
			m_vecEnd = pGun->m_vecEnd;
		}
#else
		if ( !pLaser && pGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			// crappy hack for team color vector
			Vector vecColor;
			int iTeam = GetTeamNumber();

			switch ( iTeam )
			{
				case TF_TEAM_RED:
					vecColor.Init( 255, -255, -255 );
					break;

				case TF_TEAM_BLUE:
					vecColor.Init( -255, -255, 255 );
					break;

				default:
					vecColor.Init();
					break;
			}

			// create pLaser
			pLaser = pGun->CreateLaserBeam();
			pLaser->SetControlPoint( 2, vecColor );
		}

		if ( pGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			pLaser->SetControlPoint( 1, m_vecEnd );
		}
#endif
	}

	BaseClass::ItemPostFrame();
}
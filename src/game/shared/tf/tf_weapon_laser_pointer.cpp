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

ConVar tf_debug_wrangler( "tf_wrangler_debug", "0", FCVAR_CHEAT );

class CTraceFilterIgnoreTeammatesAndTeamObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesAndTeamObjects, CTraceFilterSimple );

	CTraceFilterIgnoreTeammatesAndTeamObjects( const IHandleEntity *passentity, int collisionGroup, int teamNumber )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		m_iTeamNumber = teamNumber;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity && pEntity->GetTeamNumber() == m_iTeamNumber )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	int m_iTeamNumber;
};


IMPLEMENT_NETWORKCLASS_ALIASED( TFLaserPointer, DT_WeaponLaserPointer )

BEGIN_NETWORK_TABLE( CTFLaserPointer, DT_WeaponLaserPointer )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLaserPointer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_laser_pointer, CTFLaserPointer );
PRECACHE_WEAPON_REGISTER( tf_weapon_laser_pointer );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFLaserPointer )
END_DATADESC()
#endif

CTFLaserPointer::CTFLaserPointer()
{
#ifdef GAME_DLL
	pGun = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFLaserPointer::Deploy( void )
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
bool CTFLaserPointer::Holster( CBaseCombatWeapon *pSwitchingTo )
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
void CTFLaserPointer::WeaponReset( void )
{
	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLaserPointer::PrimaryAttack( void )
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
	SendWeaponAnim( ACT_ITEM3_VM_CHARGE );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLaserPointer::SecondaryAttack( void )
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
void CTFLaserPointer::ItemPostFrame( void )
{
#ifdef GAME_DLL
	if ( pGun )
	{
		//TODO: Find a better way to determine if we can wrangle
		if ( !pGun->IsRedeploying() && !pGun->IsBuilding() && !pGun->IsUpgrading() && !pGun->HasSapper() )
		{
			pGun->SetState( SENTRY_STATE_WRANGLED );
		}

		if ( pGun->GetState() == SENTRY_STATE_WRANGLED )
		{
			UpdateLaserDot();
		}
	}
#endif

	BaseClass::ItemPostFrame();
}

#ifdef GAME_DLL
void CTFLaserPointer::UpdateLaserDot( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	pGun->StudioFrameAdvance( );

	if ( !pOwner || !pOwner->IsAlive() )
	{
		pGun->OnStopWrangling();
		pGun->SetShouldFire( false );
		return;
	}

	trace_t tr;
	Vector vecStart, vecEnd, vecForward;
	pOwner->EyeVectors( &vecForward );

	vecStart = pOwner->EyePosition();
	vecEnd = vecStart + ( vecForward * MAX_TRACE_LENGTH);

	CTraceFilterIgnoreTeammatesAndTeamObjects *pFilter = new CTraceFilterIgnoreTeammatesAndTeamObjects( this, COLLISION_GROUP_NONE, GetTeamNumber() );

	// First pass to find where we are looking
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pFilter, &tr );

	vecStart = pGun->EyePosition();
	vecEnd = tr.endpos;

	// Second pass to find what we actually see
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pFilter, &tr );

	// If we're looking at a player fix our position to the centermass
	if ( tr.DidHitNonWorldEntity() )
	{
		pGun->SetEnemy( tr.m_pEnt );
		vecEnd = pGun->GetEnemyAimPosition( tr.m_pEnt );
	}
	else
	{
		pGun->SetEnemy( NULL );
		vecEnd = tr.endpos;
	}

	pGun->SetEndVector( vecEnd );

	// Adjust sentry angles 
	vecForward = vecEnd - vecStart;
	pGun->UpdateSentryAngles( vecForward ); 

	if ( tf_debug_wrangler.GetBool() ) 
	{
		NDebugOverlay::Line( vecStart, vecEnd, 0, 255, 0, true, 0.25f );
	}
}
#endif
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_crossbow.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFCrossbow, DT_TFCrossbow )
BEGIN_NETWORK_TABLE( CTFCrossbow, DT_TFCrossbow )
#ifndef CLIENT_DLL
	SendPropFloat( SENDINFO(m_flRegenerateDuration) ),
#else
	RecvPropFloat( RECVINFO(m_flRegenerateDuration) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCrossbow )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_crossbow, CTFCrossbow );
PRECACHE_WEAPON_REGISTER( tf_weapon_crossbow );


//-----------------------------------------------------------------------------
// Purpose: Reload while holstered
//-----------------------------------------------------------------------------
bool CTFCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if( !m_bReloadedThroughAnimEvent )
	{
		float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );

		float flReloadTime = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReload;
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time_hidden );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, fast_reload );

		float flDelay = flReloadTime + flFireDelay + m_flRegenerateDuration;
		if ( flDelay > GetWeaponIdleTime() )
		{
			SetWeaponIdleTime( flDelay );
			m_flNextPrimaryAttack = flDelay;
		}

		IncrementAmmo();
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCrossbow::GetProjectileSpeed( void )
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
float CTFCrossbow::GetProjectileGravity( void )
{
	return 0.2f;
	// They perform the below, which doesn't make sense to do when dealing with constants
	/*return RemapValClamped( 0.75f,
							  0.0f,
							  TF_BOW_MAX_CHARGE_TIME,
							  0.5f,
							  0.1f );*/
}

//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_revolver.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#if defined( CLIENT_DLL )
ConVar tf2v_revolver_scale_crosshair( "tf2v_revolver_scale_crosshair", "1", FCVAR_ARCHIVE, "Toggle the crosshair size scaling on the ambassador" );
ConVar tf2v_new_revolver_reload("tf2v_new_revolver_reload", "0", FCVAR_ARCHIVE, "Toggles between the old and new revolver reload animation.");
#endif

//=============================================================================
//
// Weapon Revolver tables.
//
#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );

CREATE_SIMPLE_WEAPON_TABLE( TFRevolver, tf_weapon_revolver )
CREATE_SIMPLE_WEAPON_TABLE( TFRevolver_Secondary, tf_weapon_revolver_secondary )

//=============================================================================
//
// Weapon Revolver functions.
//

bool CTFRevolver::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			return false;
		}
	}

	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRevolver::HasSapperCrits()
{
	int iSapperCrits = 0;
	CALL_ATTRIB_HOOK_INT(iSapperCrits, sapper_kills_collect_crits);

	return (iSapperCrits > 0);
}

#if defined( CLIENT_DLL )
void CTFRevolver::GetWeaponCrosshairScale( float &flScale )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner == nullptr )
		return;

	int iMode = 0;
	CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode );
	if ( iMode == 1 && tf2v_revolver_scale_crosshair.GetBool() )
	{
		/*const float flTimeBase = pOwner->GetFinalPredictedTime();
		const float flFireInterval = ( ( gpGlobals->interpolation_amount * gpGlobals->interpolation_amount ) + flTimeBase ) - GetLastFireTime();
		flScale = ( Clamp( ( flFireInterval + -1.0f ) * -2.0f, 0.0f, 1.0f ) * 1.75f ) + 0.75f;*/
		float flFireInterval = Min( gpGlobals->curtime - GetLastFireTime(), 1.25f );
		flScale = Clamp( ( flFireInterval / 1.25f ), 0.334f, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFRevolver::TranslateViewmodelHandActivity( int iActivity )
{
	int iTranslation = iActivity;

	if ( iActivity == ACT_SECONDARY_VM_RELOAD && !tf2v_new_revolver_reload.GetBool() )
		iTranslation = ACT_SECONDARY_VM_RELOAD2;

	return BaseClass::TranslateViewmodelHandActivity( iTranslation );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRevolver::PrimaryAttack(void)
{
	BaseClass::PrimaryAttack();
	if (HasSapperCrits())
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if (pOwner && pOwner->IsAlive())
		{
			pOwner->m_Shared.DeductSapperKillCount();

			if (pOwner->m_Shared.GetSapperKillCount() < 1)
				pOwner->m_Shared.RemoveCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRevolver::ItemPostFrame( void )
{
	if (HasSapperCrits())
	{
		CritThink();
	}
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Used for checking if we are critboosted or not.
//-----------------------------------------------------------------------------
void CTFRevolver::CritThink( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		if ( pOwner->m_Shared.GetSapperKillCount() > 0 )
		{
			if ( !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON ) )
				pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );
		}
		else
		{
			if ( pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON ) )
				pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRevolver::Deploy( void )
{
	if (HasSapperCrits())
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if (pOwner && BaseClass::Deploy())
		{
			if (pOwner->m_Shared.GetSapperKillCount() > 0)
				pOwner->m_Shared.AddCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
			return true;
		}
	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRevolver::Holster( CBaseCombatWeapon *pSwitchTo )
{
	if (HasSapperCrits())
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if (pOwner && BaseClass::Holster(pSwitchTo))
		{
			if (pOwner->m_Shared.InCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON))
				pOwner->m_Shared.RemoveCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
			return true;
		}
	}

	return BaseClass::Holster(pSwitchTo);
}


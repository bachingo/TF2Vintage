//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_smg.h"
#include "in_buttons.h"

#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon SMG tables.
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


CREATE_SIMPLE_WEAPON_TABLE( TFSMG, tf_weapon_smg )
CREATE_SIMPLE_WEAPON_TABLE( TFSMG_Primary, tf_weapon_smg_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFSMG_Charged, tf_weapon_charged_smg )

// Server specific.
//#ifndef CLIENT_DLL
//BEGIN_DATADESC( CTFSMG )
//END_DATADESC()
//#endif

//=============================================================================
//
// Weapon SMG functions.
//

extern ConVar tf2v_use_new_cleaners;

//=============================================================================
//
// Weapon SMG_Charged functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSMG_Charged::HasChargeBar(void)
{
	if (tf2v_use_new_cleaners.GetBool())
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFSMG_Charged::GetEffectBarProgress(void)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return pOwner->m_Shared.GetCrikeyMeter() / 100.0f;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Activate special ability
//-----------------------------------------------------------------------------
void CTFSMG_Charged::SecondaryAttack(void)
{
	CTFPlayer *pOwner = ToTFPlayer(GetOwner());
	if ( !pOwner )
		return;

	if ( ( pOwner->m_Shared.GetCrikeyMeter() >= 100.0f ) && tf2v_use_new_cleaners.GetBool() )
			pOwner->m_Shared.AddCond( TF_COND_MINICRITBOOSTED_RAGE_BUFF );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSMG_Charged::ItemBusyFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( ( pOwner->m_nButtons & IN_ATTACK2 ) )
		SecondaryAttack();

	BaseClass::ItemBusyFrame();
}

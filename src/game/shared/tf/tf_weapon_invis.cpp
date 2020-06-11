//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_invis.h"
#include "in_buttons.h"

#if !defined( CLIENT_DLL )
	#include "vguiscreen.h"
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
#endif

extern ConVar tf_spy_invis_unstealth_time;
extern ConVar tf_spy_cloak_consume_rate;
extern ConVar tf_spy_cloak_regen_rate;

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponInvis, DT_TFWeaponInvis )

BEGIN_NETWORK_TABLE( CTFWeaponInvis, DT_TFWeaponInvis )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponInvis )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_invis, CTFWeaponInvis );
PRECACHE_WEAPON_REGISTER( tf_weapon_invis );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFWeaponInvis )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose: Use the offhand view model
//-----------------------------------------------------------------------------
void CTFWeaponInvis::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle state
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::ActivateInvisibility( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	if ( pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		pOwner->m_Shared.FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
		return true;
	}

	float flConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flConsumeRate, mult_cloak_meter_consume_rate );

	float flRegenRate = tf_spy_cloak_regen_rate.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flRegenRate, mult_cloak_meter_regen_rate );

	pOwner->m_Shared.SetHasMotionCloak( HasMotionCloak() );
	pOwner->m_Shared.SetCloakDrainRate( flConsumeRate );
	pOwner->m_Shared.SetCloakRegenRate( flRegenRate );

	if ( HasFeignDeath() )
	{
		if ( pOwner->m_Shared.IsFeignDeathReady() )
		{
			if ( !pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
			{
				pOwner->HolsterOffHandWeapon();

				CBaseCombatWeapon *pWeapon = pOwner->GetActiveWeapon();
				if ( pWeapon )
					pWeapon->m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;

				pOwner->m_Shared.SetFeignReady( false );
				return true;
			}
		}
		else if ( pOwner->m_Shared.GetSpyCloakMeter() == 100.0f )
		{
			pOwner->m_Shared.SetFeignReady( true );
			pOwner->SetOffHandWeapon( this );
			return true;
		}

		return false;
	}

	if ( pOwner->CanGoInvisible() && ( pOwner->m_Shared.GetSpyCloakMeter() > 8.0f ) )
	{
		pOwner->m_Shared.AddCond( TF_COND_STEALTHED );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::CleanUpInvisibility( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->m_Shared.IsFeignDeathReady() )
		pOwner->m_Shared.SetFeignReady( false );

	if ( pOwner->m_Shared.IsStealthed() )
		pOwner->m_Shared.FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );

	pOwner->HolsterOffHandWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::HasFeignDeath( void ) const
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT( nWeaponMode, set_weapon_mode );
	return nWeaponMode == 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::HasMotionCloak( void ) const
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT( nWeaponMode, set_weapon_mode );
	return nWeaponMode == 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFWeaponInvis::GetEffectLabelText(void)
{
	if (HasFeignDeath())
		return "#TF_Feign";
	else if (HasMotionCloak())
		return "#TF_CloakDagger";
	
	return "#TF_Cloak";
	
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the view model when we hide
//-----------------------------------------------------------------------------
void CTFWeaponInvis::HideThink( void )
{ 
	SetWeaponVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
		if ( vm )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( vm )
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Deploy( void )
{
	bool b = BaseClass::Deploy();

	SetWeaponIdleTime( gpGlobals->curtime + 1.5 );

	return b;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponInvis::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	bool bHolster = BaseClass::Holster( pSwitchingTo );

	// far in the future
	SetWeaponIdleTime( gpGlobals->curtime + 10 );

	return bHolster;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::SecondaryAttack( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::ItemBusyFrame( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponInvis::GetEffectBarProgress( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		return ( pOwner->m_Shared.GetSpyCloakMeter() / 100.0f );
	}

	return 1.0f;
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponInvis::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// Get the name of the viewmodel.
	char* cViewModel = nullptr;
	cViewModel = (char*)GetViewModel(0);

	// Assume it's the default and return.
	if (!cViewModel)
	{
		pPanelName = "pda_panel_spy_invis";
		return;
	}
	
	// Search the viewmodels for the following strings.
	if (Q_stristr(cViewModel, "v_watch_pocket_spy")) // Dead Ringer
	{
		pPanelName = "pda_panel_spy_invis_pocket";
		return;
	}
	else if (Q_stristr(cViewModel, "v_ttg_watch_spy")) // TTG Watch [Enthusiast's Timepiece]
	{
		pPanelName = "pda_panel_spy_invis_pocket_ttg";
		return;
	}
	else if (Q_stristr(cViewModel, "v_hm_watch"))	// Quackenbirdt
	{
		pPanelName = "pda_panel_spy_invis_pocket_hm";
		return;
	}
	else // v_watch_spy [Invis Watch], v_watch_leather_spy [Cloak and Dagger]
		pPanelName = "pda_panel_spy_invis";
		
	return;
}

#endif
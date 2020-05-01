//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Cow Mangler (Particle Cannon)
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_particlecannon.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

#define TF_CANNON_CHARGEUP_TIME 2.0f

//=============================================================================
//
// Particle Cannon Table.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFParticleCannon, DT_WeaponParticleCannon )

BEGIN_NETWORK_TABLE( CTFParticleCannon, DT_WeaponParticleCannon )
#ifndef CLIENT_DLL
	//SendPropBool( SENDINFO(m_bLockedOn) ),
	SendPropFloat(SENDINFO(m_flChargeUpTime), 0, SPROP_NOSCALE),
#else
	//RecvPropInt( RECVINFO(m_bLockedOn) ),
	RecvPropFloat( RECVINFO( m_flChargeUpTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFParticleCannon )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_particle_cannon, CTFParticleCannon );
PRECACHE_WEAPON_REGISTER( tf_weapon_particle_cannon );

// Server specific.

#ifdef GAME_DLL
BEGIN_DATADESC(CTFParticleCannon)
END_DATADESC()
#endif


//-----------------------------------------------------------------------------
// Purpose: No holstering while charging a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::CanHolster( void )
{
	if (m_flChargeUpTime != 0)
		return false;
	
	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();

#ifdef GAME_DLL

	/*
	Vector forward;
	AngleVectors( pOwner->EyeAngles(), &forward );
	trace_t tr;
	CTraceFilterSimple filter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceLine( pOwner->EyePosition(), pOwner->EyePosition() + forward * 2000, MASK_SOLID, &filter, &tr );

	if ( tr.m_pEnt &&
		tr.m_pEnt->IsPlayer() &&
		tr.m_pEnt->IsAlive() &&
		tr.m_pEnt->GetTeamNumber() != pOwner->GetTeamNumber() )
	{
		m_bLockedOn = true;
	}
	else
	{
		m_bLockedOn = false;
	}
	*/
	
#endif

	if ( ( m_flChargeUpTime > 0 ) && gpGlobals->curtime >= m_flChargeUpTime )
	{
		m_flChargeUpTime = 0;
		ChargeAttack();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Starts the charge up on the Cow Mangler.
//-----------------------------------------------------------------------------
void CTFParticleCannon::SecondaryAttack( void )
{
	// Need to have the attribute to use this.
	if ( !HasChargeUp() )
		return;
	
	// Are we capable of firing again?
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;
	
	// Do we have full energy to launch a charged shot?
	if ( m_iClip1 < GetMaxClip1() )
	{
		Reload();
		return;
	}
	
	// Set our charging time, and prevent us attacking until it is done.
	m_flChargeUpTime = gpGlobals->curtime + TF_CANNON_CHARGEUP_TIME;
	m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flChargeUpTime + m_pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay;
	
	// Slow down the player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer)
	{
		if (!pPlayer->m_Shared.InCond(TF_COND_AIMING))
			pPlayer->m_Shared.AddCond(TF_COND_AIMING);
	}
	
	WeaponSound(SPECIAL1);
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we're allowed to charge up a shot.
//-----------------------------------------------------------------------------
bool CTFParticleCannon::HasChargeUp( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer)
		return false;
	
	int nHasEnergyChargeUp = 0;
	CALL_ATTRIB_HOOK_INT(nHasEnergyChargeUp,energy_weapon_charged_shot);
	
	return (nHasEnergyChargeUp != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Launches a powerful energy ball that can stun buildings.
//-----------------------------------------------------------------------------
void CTFParticleCannon::ChargeAttack( void )
{
	// Remove the slowdown.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer)
	{
		if (pPlayer->m_Shared.InCond(TF_COND_AIMING))
			pPlayer->m_Shared.RemoveCond(TF_COND_AIMING);
	}
	
	// We use a special Energy Ball call here to do the charged variant.
	BaseClass::FireEnergyBall( pPlayer, true );
	
	// Clear our ammo.
	m_iClip1 = 0;
	
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	// Call the muzzle flash, but don't use the rocket launcher's backblast version.
	CTFWeaponBase::CreateMuzzleFlashEffects( pAttachEnt, nIndex );
}

/*
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFParticleCannon::DrawCrosshair( void )
{
	BaseClass::DrawCrosshair();

	if ( m_bLockedOn )
	{
		int iXpos = XRES(340);
		int iYpos = YRES(260);
		int iWide = XRES(8);
		int iTall = YRES(8);

		Color col( 0, 255, 0, 255 );
		vgui::surface()->DrawSetColor( col );

		vgui::surface()->DrawFilledRect( iXpos, iYpos, iXpos + iWide, iYpos + iTall );

		// Draw the charge level onscreen
		vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
		vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
		vgui::surface()->DrawSetTextFont( hFont );
		vgui::surface()->DrawSetTextColor( col );
		vgui::surface()->DrawSetTextPos(iXpos + XRES(12), iYpos );
		vgui::surface()->DrawPrintText(L"Lock", wcslen(L"Lock"));

		vgui::surface()->DrawLine( XRES(320), YRES(240), iXpos, iYpos );
	}
}
*/

#endif
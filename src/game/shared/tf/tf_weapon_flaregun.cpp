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
	//Check if we do anything else, like keep track of flares.
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	
	if (nWeaponMode != 1)
	{
		//Short and simple attack.
		BaseClass::PrimaryAttack();
	}
	else	// This is just BaseClass:PrimaryAttack() but with the added flare tracking.
	{
				// Check for ammunition.
		if ( m_iClip1 <= 0 && UsesClipsForAmmo1() )
			return;

		// Are we capable of firing again?
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;

		// Get the player owning the weapon.
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		if ( !CanAttack() )
			return;

		CalcIsAttackCritical();

#ifndef CLIENT_DLL
		pPlayer->RemoveInvisibility();
		pPlayer->RemoveDisguise();

		// Minigun has custom handling
		if ( GetWeaponID() != TF_WEAPON_MINIGUN )
		{
			pPlayer->SpeakWeaponFire();
		}
		CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		SendWeaponAnim( ACT_VM_PRIMARYATTACK );

		pPlayer->SetAnimation( PLAYER_ATTACK1 );

		// Update our active flares.
		CBaseEntity *pFlareProjectile = static_cast<CTFProjectile_Flare *>(FireProjectile(ToTFPlayer(GetOwner())));

		if (pFlareProjectile)
		{
#ifdef GAME_DLL
			CTFProjectile_Flare *pFlare = (CTFProjectile_Flare*)pFlareProjectile;
			pFlare->SetLauncher(this);

			FlareHandle hHandle;
			hHandle = pFlare;
			m_Flares.AddToTail(hHandle);
#endif
		}

		m_flLastFireTime  = gpGlobals->curtime;

		// Set next attack times.
		float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
		CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );
		
		if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
				CALL_ATTRIB_HOOK_FLOAT( flFireDelay, rocketjump_attackrate_bonus );
			
		float flHealthModFireBonus = 1;
		CALL_ATTRIB_HOOK_FLOAT( flHealthModFireBonus, mult_postfiredelay_with_reduced_health );
		if (flHealthModFireBonus != 1)
		{
			flFireDelay *= RemapValClamped( pPlayer->GetHealth() / pPlayer->GetMaxHealth(), 0.2, 0.9, flHealthModFireBonus, 1.0 );
		}

		m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

		// Don't push out secondary attack, because our secondary fire
		// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
		//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

		// Set the idle animation times based on the sequence duration, so that we play full fire animations
		// that last longer than the refire rate may allow.
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

		AbortReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Detonator - Detonate flares in midair
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
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
#ifdef GAME_DLL
		for ( int i = 0; i < m_Flares.Count(); i++ )
		{
			CTFProjectile_Flare *pFlare = m_Flares[ i ];
			pFlare->Detonate();
			m_Flares.Remove(i);
		}
#endif
	}
	else
		BaseClass::SecondaryAttack();
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

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun::ApplyPostOnHitAttributes( CTakeDamageInfo const &info, CTFPlayer *pVictim )
{
	BaseClass::ApplyPostOnHitAttributes( info, pVictim );

	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pAttacker == NULL || pVictim == NULL )
		return;

	if ( !HasKnockback() || pVictim->m_Shared.InCond( TF_COND_MEGAHEAL ) )
		return;

	if ( pVictim->m_Shared.GetKnockbackWeaponID() >= 0 )
		return;

	Vector vecToVictim = pAttacker->WorldSpaceCenter() - pVictim->WorldSpaceCenter();

	float flKnockbackMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flKnockbackMult, scattergun_knockback_mult );

	// This check is a bit open ended and broad
	if ( ( info.GetDamage() <= 30.0f || vecToVictim.LengthSqr() > Square( 400.0f ) ) && flKnockbackMult < 1.0f )
		return;

	vecToVictim.NormalizeInPlace();
	
	const float flSizeMag = pVictim->WorldAlignSize().x * pVictim->WorldAlignSize().y * pVictim->WorldAlignSize().z;
	const float flHullMag = 48 * 48 * 82.0;

	const float flDamageForce = info.GetDamage() * ( flHullMag / flSizeMag ) * flKnockbackMult;

	float flDmgForce = Min( flDamageForce, 1000.0f );
	Vector vecVelocityImpulse = vecToVictim * abs( flDmgForce );

	pVictim->ApplyAirBlastImpulse( vecVelocityImpulse );
	pVictim->m_Shared.StunPlayer( 0.3f, 1.0f, 1.0f, TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_LIMITMOVEMENT, pAttacker );

	pVictim->m_Shared.SetKnockbackWeaponID( pAttacker->GetUserID() );
}
#endif

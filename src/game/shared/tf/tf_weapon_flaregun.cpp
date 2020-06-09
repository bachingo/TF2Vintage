//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"
#include "tf_weapon_flamethrower.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

extern ConVar tf2v_use_extinguish_heal;
extern ConVar tf2v_debug_airblast;

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
			if (pFlare)
			{
				pFlare->Detonate();
			}
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add flares to our list as they're fired
//-----------------------------------------------------------------------------
void CTFFlareGun::AddFlare( CTFProjectile_Flare *pFlare )
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT(nWeaponMode, set_weapon_mode);
	if (nWeaponMode == 1)
	{
		#ifdef GAME_DLL
		FlareHandle hHandle;
		hHandle = pFlare;
		m_Flares.AddToTail( hHandle );
		#endif
	}
	return;
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
		#ifdef GAME_DLL
		Assert( dynamic_cast<CTFProjectile_Flare *>( pVictim ) );

		FlareHandle hHandle;
		hHandle = (CTFProjectile_Flare *)pVictim;
		m_Flares.FindAndRemove( hHandle );
		#endif
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


// Manmelter Start



IMPLEMENT_NETWORKCLASS_ALIASED(TFFlareGunRevenge, DT_WeaponFlareGunRevenge)

BEGIN_NETWORK_TABLE(CTFFlareGunRevenge, DT_WeaponFlareGunRevenge)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFFlareGunRevenge)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_flaregun_revenge, CTFFlareGunRevenge);
PRECACHE_WEAPON_REGISTER(tf_weapon_flaregun_revenge);

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC(CTFFlareGunRevenge)
END_DATADESC()
#endif

#define TF_MANMELTER_FIRE_RATE 2.0f
#define TF_MANMELTER_AIRBLAST_RATE 0.75f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGunRevenge::Deploy( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && CTFWeaponBaseGun::Deploy())
	{
		if ( pOwner->m_Shared.HasAirblastCrits() )
			pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGunRevenge::Holster( CBaseCombatWeapon *pSwitchTo )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && CTFWeaponBaseGun::Holster(pSwitchTo))
	{
		pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Does the fancy weapon effects.
//-----------------------------------------------------------------------------
void CTFFlareGunRevenge::ItemPostFrame( void )
{
	if (gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		// Add the sparks when we're ready to shoot.
#ifdef CLIENT_DLL
		C_BaseEntity *pModel = GetWeaponForEffect();
		if ( pModel )
		{
			pModel->ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
				
			CNewParticleEffect* pMuzzle = pModel->ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
			if ( pMuzzle )
			{
				pMuzzle->SetControlPoint(CUSTOM_COLOR_CP1, GetEnergyWeaponColor(false));
				pMuzzle->SetControlPoint(CUSTOM_COLOR_CP2, GetEnergyWeaponColor(true));
			}
			
			pModel->ParticleProp()->Create( "drg_manmelter_idle", PATTACH_POINT_FOLLOW, "muzzle" );
		}
#endif

		// Play a ding when we're ready to shoot.
		if ( bWaitingtoFire )
		{
			bWaitingtoFire = false;
			EmitSound( "Weapon_SniperRailgun.NonScoped" );
		}
	}
	
	CTFWeaponBaseGun::ItemPostFrame();
}	

//-----------------------------------------------------------------------------
// Purpose: Fire a laser beam.
//-----------------------------------------------------------------------------
void CTFFlareGunRevenge::PrimaryAttack(void)
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack() )
	{
		return;
	}

	CalcIsAttackCritical();

#ifndef CLIENT_DLL

	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();

	// Minigun has custom handling
	if (GetWeaponID() != TF_WEAPON_MINIGUN)
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	WeaponSound(SPECIAL2);

	pPlayer->SetAnimation(PLAYER_ATTACK1);

	FireProjectile(pPlayer);

	m_flLastFireTime = gpGlobals->curtime;

	// Set next attack times.
	float flFireDelay = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT(flFireDelay, mult_postfiredelay);

	if (pPlayer->m_Shared.InCond(TF_COND_BLASTJUMPING))
		CALL_ATTRIB_HOOK_FLOAT(flFireDelay, rocketjump_attackrate_bonus);

	float flHealthModFireBonus = 1;
	CALL_ATTRIB_HOOK_FLOAT(flHealthModFireBonus, mult_postfiredelay_with_reduced_health);
	if (flHealthModFireBonus != 1)
	{
		flFireDelay *= RemapValClamped(pPlayer->GetHealth() / pPlayer->GetMaxHealth(), 0.2, 0.9, flHealthModFireBonus, 1.0);
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + TF_MANMELTER_FIRE_RATE;
	bWaitingtoFire = true;
	
	if (pPlayer && pPlayer->IsAlive())
	{
		pPlayer->m_Shared.DeductAirblastCrit();

		if (!pPlayer->m_Shared.HasAirblastCrits())
			pPlayer->m_Shared.RemoveCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
	}

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	AbortReload();
}

//-----------------------------------------------------------------------------
// Purpose: Airblast players.
//-----------------------------------------------------------------------------
void CTFFlareGunRevenge::SecondaryAttack( void )
{

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer(GetPlayerOwner());
	if (!pOwner)
		return;

	// Are we capable of firing again?
	if (m_flNextSecondaryAttack > gpGlobals->curtime)
		return;

	if ( !CanAttack() )
	{
		return;
	}

	// Save our airblast counter for later to see if we extinguished someone.
#ifdef CLIENT_DLL
	int nExtinguished = pOwner->m_Shared.GetAirblastCritCount();
#endif

#ifdef GAME_DLL

	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pOwner, false);

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation(pOwner, pOwner->GetCurrentCommand());

	Vector vecDir;
	QAngle angDir = pOwner->EyeAngles();
	AngleVectors(angDir, &vecDir);

	const Vector vecBlastSize = Vector(128, 128, 64);

	// Picking max out of length, width, height for airblast distance.
	float flBlastDist = Max(Max(vecBlastSize.x, vecBlastSize.y), vecBlastSize.z);

	Vector vecOrigin = pOwner->Weapon_ShootPosition() + vecDir * flBlastDist;

	CBaseEntity *pList[64];

	int count = UTIL_EntitiesInBox(pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0);

	if (tf2v_debug_airblast.GetBool())
	{
		NDebugOverlay::Box(vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0);
	}

	for (int i = 0; i < count; i++)
	{
		CBaseEntity *pEntity = pList[i];

		if (!pEntity)
			continue;

		if (pEntity == pOwner)
			continue;

		if (!pEntity->IsDeflectable())
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		trace_t tr;
		UTIL_TraceLine(pOwner->Weapon_ShootPosition(), pEntity->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr);
		if (tr.fraction != 1.0f)
			continue;

		if (pEntity->IsPlayer() && pEntity->IsAlive() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);

			if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING) && pTFPlayer->InSameTeam(pOwner))
			{
				// Extinguish teammates.
				pTFPlayer->m_Shared.RemoveCond(TF_COND_BURNING);
				if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING_PYRO))
					pTFPlayer->m_Shared.RemoveCond(TF_COND_BURNING_PYRO);
			
				pTFPlayer->EmitSound("TFPlayer.FlameOut");

				// Give us revenge crits as well.
				if (CanGetAirblastCrits())
					pOwner->m_Shared.StoreAirblastCrit();

				if (pOwner && pOwner->GetActiveWeapon() == this)
				{
					if (pOwner->m_Shared.HasAirblastCrits())
					{
						if (!pOwner->m_Shared.InCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON))
							pOwner->m_Shared.AddCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
					}
					else
					{
						if (pOwner->m_Shared.InCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON))
							pOwner->m_Shared.RemoveCond(TF_COND_CRITBOOSTED_ACTIVEWEAPON);
					}
				}

				// If we got the convar turned on, heal us.
				if (tf2v_use_extinguish_heal.GetBool())
				{
					int iHPtoHeal = 20;
					int iHealthRestored = TakeHealth(iHPtoHeal, DMG_GENERIC);
					if (iHealthRestored)
					{
						IGameEvent *event_healonhit = gameeventmanager->CreateEvent("player_healonhit");

						if (event_healonhit)
						{
							event_healonhit->SetInt("amount", iHealthRestored);
							event_healonhit->SetInt("entindex", pOwner->entindex());

							gameeventmanager->FireEvent(event_healonhit);
						}
					}
				}
			}
		}
	}

#endif

#ifdef CLIENT_DLL
	C_BaseEntity *pModel = GetWeaponForEffect();
	if (pModel)
	{
		CNewParticleEffect* pVacuum;
		if ( nExtinguished != pOwner->m_Shared.GetAirblastCritCount() ) // We extinguished someone in between, play the extinguished variant.
		{	
			// We play our sound here for extinguishing feedback.
			WeaponSound(SPECIAL1);
			pVacuum = pModel->ParticleProp()->Create("drg_manmelter_vacuum_flames", PATTACH_POINT_FOLLOW, "muzzle");
		}
		else // No extinguish, play the standard variant.
			pVacuum = pModel->ParticleProp()->Create("drg_manmelter_vacuum", PATTACH_POINT_FOLLOW, "muzzle");
		
		if ( pVacuum )
		{
			pVacuum->SetControlPoint(CUSTOM_COLOR_CP1, GetEnergyWeaponColor(false));
			pVacuum->SetControlPoint(CUSTOM_COLOR_CP2, GetEnergyWeaponColor(true));
		}
	}
#endif

	// Don't allow firing immediately after airblasting.
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + TF_MANMELTER_AIRBLAST_RATE;
	bWaitingtoFire = true;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFFlareGunRevenge::GetCount(void)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return pOwner->m_Shared.GetAirblastCritCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Displays the Charge bar.
//-----------------------------------------------------------------------------
bool CTFFlareGunRevenge::HasChargeBar(void)
{
	if ( CanGetAirblastCrits() )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlareGunRevenge::CanGetAirblastCrits(void) const
{
	int nAirblastRevenge = 0;
	CALL_ATTRIB_HOOK_INT( nAirblastRevenge, sentry_killed_revenge );
	return nAirblastRevenge == 1;
}

//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_dragons_fury.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "tf_projectile_dragons_fury.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "c_tf_viewmodeladdon.h"
// Server specific.
#else
#include "tf_player.h"
#include "soundent.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "props.h"
#endif


#ifdef GAME_DLL
ConVar  tf_fireball_airblast_recharge_penalty("tf_fireball_airblast_recharge_penalty", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
ConVar  tf_fireball_hit_recharge_boost("tf_fireball_hit_recharge_boost", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "" );
#endif

#define TF_FLAMEBALL_AMMO_PER_SECONDARY_ATTACK 4

//CREATE_SIMPLE_WEAPON_TABLE( TFWeaponFlameBall, tf_weapon_rocketlauncher_fireball ) // DRAGON'S FURY
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponFlameBall, DT_WeaponFlameBall )

BEGIN_NETWORK_TABLE( CTFWeaponFlameBall, DT_WeaponFlameBall )
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flRechargeScale ) ),
#else
	RecvPropTime( RECVINFO( m_flRechargeScale ) ),
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFWeaponFlameBall )
	DEFINE_PRED_FIELD_TOL( m_flRechargeScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher_fireball, CTFWeaponFlameBall );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher_fireball );

BEGIN_DATADESC(CTFWeaponFlameBall)
DEFINE_FIELD(m_flRechargeScale, FIELD_TIME),
END_DATADESC()

extern ConVar tf2v_airblast;
extern ConVar tf2v_debug_airblast;
extern ConVar tf2v_use_extinguish_heal;
extern ConVar tf2v_airblast_players;


//=============================================================================
//
// Weapon Dragon's Fury i guess?
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponFlameBall::CTFWeaponFlameBall()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFWeaponFlameBall::~CTFWeaponFlameBall()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::Precache( void )
{
	BaseClass::Precache();
	PrecacheParticleSystem( "pyro_blast" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	PrecacheScriptSound( "TFPlayer.FlameOut" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheScriptSound( "Weapon_DragonsFury.Single" );
	PrecacheScriptSound( "Weapon_DragonsFury.SingleCrit" );
	PrecacheScriptSound( "Weapon_DragonsFury.PressureBuild" );
	PrecacheScriptSound( "Weapon_DragonsFury.PressureBuildStop" );
	PrecacheParticleSystem( "deflect_fx" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponFlameBall::FireProjectile( CTFPlayer *pPlayer )
{
	if ( IsCurrentAttackACrit() )
	{
		EmitSound( "Weapon_DragonsFury.SingleCrit" );
	}
	else
	{
		EmitSound( "Weapon_DragonsFury.Single" );
	}

	// Server only - create the fireball.
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( -23.5f, 10.0f, -23.5f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 12.5f;
	}

	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, false );
		
	CTFProjectile_BallOfFire *pProjectile = CTFProjectile_BallOfFire::Create( this, vecSrc, angForward, pPlayer, pPlayer );
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
	}

#endif

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	
	RemoveAmmo( pPlayer );

	DoFireEffects();

	UpdatePunchAngles( pPlayer );

#ifdef GAME_DLL
	return pProjectile;
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponFlameBall::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::PrimaryAttack()
{
	m_bReloadedThroughAnimEvent = false;

	BaseClass::PrimaryAttack();

#ifdef GAME_DLL
	StartPressureSound();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::SecondaryAttack()
{
	if (tf2v_airblast.GetInt() != 1 && tf2v_airblast.GetInt() != 2)
		return;

	int iNoAirblast = 0;
	CALL_ATTRIB_HOOK_FLOAT( iNoAirblast, set_flamethrower_push_disabled );
	if ( iNoAirblast )
		return;

	if (tf2v_airblast.GetInt() != 1 && tf2v_airblast.GetInt() != 2)
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer(GetPlayerOwner());
	if (!pOwner)
		return;

	if (!CanAttack() )
	{
		return;
	}

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	WeaponSound(WPN_DOUBLE);

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


		if (pEntity->IsPlayer() && pEntity->IsAlive())
		{
			CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);

			Vector vecPushDir;
			QAngle angPushDir = angDir;

			// assume that shooter is looking at least 45 degrees up.
			if (tf2v_airblast.GetInt() > 0)
				angPushDir[PITCH] = Min(-45.f, angPushDir[PITCH]);

			AngleVectors(angPushDir, &vecPushDir);

			DeflectPlayer(pTFPlayer, pOwner, vecPushDir);
		}
		else
		{
			// Deflect projectile to the point that we're aiming at, similar to rockets.
			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecDeflect;
			GetProjectileReflectSetup(GetTFPlayerOwner(), vecPos, &vecDeflect, false);

			DeflectEntity(pEntity, pOwner, vecDeflect);
		}
	}

	lagcompensation->FinishLagCompensation(pOwner);
#endif

	float flAmmoPerSecondaryAttack = TF_FLAMEBALL_AMMO_PER_SECONDARY_ATTACK;
	CALL_ATTRIB_HOOK_FLOAT(flAmmoPerSecondaryAttack, mult_airblast_cost);

	if ( !BaseClass::IsEnergyWeapon() )
		pOwner->RemoveAmmo( flAmmoPerSecondaryAttack, m_iPrimaryAmmoType );

	// Don't allow firing immediately after airblasting.
	m_flRechargeScale = m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.75f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWeaponFlameBall::GetEffectBarProgress( void )
{
	return m_flRechargeScale / 0.8f;
}

#ifdef GAME_DLL
void CTFWeaponFlameBall::StartPressureSound( void )
{
	EmitSound( "Weapon_DragonsFury.PressureBuild" );
}

void CTFWeaponFlameBall::OnResourceMeterFilled( void )
{
	EmitSound( "Weapon_DragonsFury.PressureBuildStop" );
}

void CTFWeaponFlameBall::DeflectEntity(CBaseEntity *pEntity, CTFPlayer *pAttacker, Vector &vecDir)
{
	if (!TFGameRules() || !CanAirBlastDeflectProjectile())
		return;

	if ((pEntity->GetTeamNumber() == pAttacker->GetTeamNumber()))
		return;

	pEntity->Deflected(pAttacker, vecDir);
	pEntity->EmitSound("Weapon_FlameThrower.AirBurstAttackDeflect");
}

void CTFWeaponFlameBall::DeflectPlayer(CTFPlayer *pVictim, CTFPlayer *pAttacker, Vector &vecDir)
{
	if (!pVictim)
		return;

	if (!pVictim->InSameTeam(pAttacker) && CanAirBlastPushPlayers() && !pVictim->m_Shared.InCond(TF_COND_MEGAHEAL))
	{
		// Don't push players if they're too far off to the side. Ignore Z.
		Vector vecVictimDir = pVictim->WorldSpaceCenter() - pAttacker->WorldSpaceCenter();

		Vector vecVictimDir2D(vecVictimDir.x, vecVictimDir.y, 0.0f);
		VectorNormalize(vecVictimDir2D);

		Vector vecDir2D(vecDir.x, vecDir.y, 0.0f);
		VectorNormalize(vecDir2D);

		float flDot = DotProduct(vecDir2D, vecVictimDir2D);
		if (flDot >= 0.8)
		{
			// Push enemy players.
			pVictim->SetGroundEntity(NULL);

			// Pushes players based on the airblast type chosen.
			pVictim->EmitSound("TFPlayer.AirBlastImpact");
			if (tf2v_airblast.GetInt() == 1)
			{
				pVictim->SetAbsVelocity(vecDir * 500);
				pVictim->m_Shared.AddCond(TF_COND_NO_MOVE, 0.5f);
			}
			else if (tf2v_airblast.GetInt() == 2)
			{
				pVictim->ApplyAbsVelocityImpulse(vecDir * 750);
				pVictim->m_Shared.AddCond(TF_COND_REDUCED_MOVE, 0.5f);
			}
			// Add pusher as recent damager we he can get a kill credit for pushing a player to his death.
			pVictim->AddDamagerToHistory(pAttacker);
		}
	}
	else if (pVictim->InSameTeam(pAttacker) && CanAirBlastPutOutTeammate())
	{
		if (pVictim->m_Shared.InCond(TF_COND_BURNING))
		{
			// Extinguish teammates.
			pVictim->m_Shared.RemoveCond(TF_COND_BURNING);
			if (pVictim->m_Shared.InCond(TF_COND_BURNING_PYRO))
					pVictim->m_Shared.RemoveCond(TF_COND_BURNING_PYRO);
			pVictim->EmitSound("TFPlayer.FlameOut");

			// Bonus points.
			IGameEvent *event_bonus = gameeventmanager->CreateEvent("player_bonuspoints");
			if (event_bonus)
			{
				event_bonus->SetInt("player_entindex", pVictim->entindex());
				event_bonus->SetInt("source_entindex", pAttacker->entindex());
				event_bonus->SetInt("points", 1);

				gameeventmanager->FireEvent(event_bonus);
			}

			CTF_GameStats.Event_PlayerAwardBonusPoints(pAttacker, pVictim, 1);

			// If we got the convar turned on, heal us .
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
						event_healonhit->SetInt("entindex", pAttacker->entindex());

						gameeventmanager->FireEvent(event_healonhit);
					}
				}

			}


		}
	}
}

bool CTFWeaponFlameBall::CanAirBlast(void)
{
	if (!GetTFPlayerOwner())
		return false;

	if (tf2v_airblast.GetInt() == 0)
		return false;

	int iAirblastDisabled = 0;
	CALL_ATTRIB_HOOK_INT(iAirblastDisabled, airblast_disabled);
	return iAirblastDisabled == 0;
}

bool CTFWeaponFlameBall::CanAirBlastDeflectProjectile(void)
{
	if (!CanAirBlast())
		return false;

	int iDeflectProjDisabled = 0;
	CALL_ATTRIB_HOOK_INT(iDeflectProjDisabled, airblast_deflect_projectiles_disabled);
	return iDeflectProjDisabled == 0;
}

bool CTFWeaponFlameBall::CanAirBlastPushPlayers(void)
{
	if (!CanAirBlast())
		return false;

	if (!tf2v_airblast_players.GetBool())
		return false;

	int iPushForceDisabled = 0;
	CALL_ATTRIB_HOOK_INT(iPushForceDisabled, airblast_pushback_disabled);
	return iPushForceDisabled == 0;
}

bool CTFWeaponFlameBall::CanAirBlastPutOutTeammate(void)
{
	if (!CanAirBlast())
		return false;

	int iExtinguishDisabled = 0;
	CALL_ATTRIB_HOOK_INT(iExtinguishDisabled, airblast_put_out_teammate_disabled);
	return iExtinguishDisabled == 0;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponFlameBall::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates, bool bUseHitboxes )
{
	BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, bUseHitboxes );
}

#ifdef CLIENT_DLL
void CTFWeaponFlameBall::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	UpdatePoseParam();
}

bool CTFWeaponFlameBall::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		UpdatePoseParam();
		return true;
	}

	return false;
}

void CTFWeaponFlameBall::UpdatePoseParam( void )
{
	SetPoseParameter( "reload", m_flRechargeScale = m_flNextSecondaryAttack );
	SetPoseParameter( "charge_level", m_flRechargeScale = m_flNextSecondaryAttack );

	C_ViewmodelAttachmentModel *pAttachment = GetViewmodelAddon();
	if ( pAttachment )
 	{
		pAttachment->SetPoseParameter( "reload", m_flRechargeScale = m_flNextSecondaryAttack );
		pAttachment->SetPoseParameter( "charge_level", m_flRechargeScale = m_flNextSecondaryAttack );
	}
}
#endif
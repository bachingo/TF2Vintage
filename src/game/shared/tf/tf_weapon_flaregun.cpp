//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flaregun.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

extern ConVar tf2v_use_extinguish_heal;
extern ConVar tf2v_debug_airblast;

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlareGun, DT_WeaponFlareGun )

BEGIN_NETWORK_TABLE( CTFFlareGun, DT_WeaponFlareGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlareGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flaregun, CTFFlareGun );
PRECACHE_WEAPON_REGISTER( tf_weapon_flaregun );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFFlareGun )
END_DATADESC()
#endif

#define TF_FLARE_MIN_VEL 1200

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFFlareGun::CTFFlareGun()
{
	m_flLastDenySoundTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		BaseClass::PrimaryAttack();
	}
	else
	{
		if ( gpGlobals->curtime > m_flLastDenySoundTime )
		{
			WeaponSound( SPECIAL2 );
			m_flLastDenySoundTime = gpGlobals->curtime + 1.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Detonator - Detonate flares in midair
//-----------------------------------------------------------------------------
void CTFFlareGun::SecondaryAttack( void )
{
	if ( GetFlareGunMode() != TF_FLARE_MODE_DETONATE )
		return;

	if ( !CanAttack() )
		return;

		// Get a valid player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	for ( int i = 0; i < m_Flares.Count(); i++ )
	{
		CTFProjectile_Flare *pFlare = m_Flares[i];
		if ( pFlare )
		{
			pFlare->Detonate();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Add flares to our list as they're fired
//-----------------------------------------------------------------------------
void CTFFlareGun::AddFlare( CTFProjectile_Flare *pFlare )
{
#ifdef GAME_DLL
	FlareHandle hHandle;
	hHandle = pFlare;
	m_Flares.AddToTail( hHandle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: If a flare is exploded, remove from list.
//-----------------------------------------------------------------------------
void CTFFlareGun::DeathNotice( CBaseEntity *pVictim )
{
#ifdef GAME_DLL
	Assert( dynamic_cast<CTFProjectile_Flare *>( pVictim ) );

	FlareHandle hHandle;
	hHandle = (CTFProjectile_Flare *)pVictim;
	m_Flares.FindAndRemove( hHandle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFlareGun::GetFlareGunMode() const
{
	int nWeaponMode = 0;
	CALL_ATTRIB_HOOK_INT( nWeaponMode, set_weapon_mode );
	return nWeaponMode;
}


// Manmelter Start



IMPLEMENT_NETWORKCLASS_ALIASED( TFFlareGun_Revenge, DT_WeaponFlareGun_Revenge )

BEGIN_NETWORK_TABLE( CTFFlareGun_Revenge, DT_WeaponFlareGun_Revenge )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
	RecvPropFloat( RECVINFO( m_fLastExtinguishTime ) ),
#else
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
	SendPropFloat( SENDINFO( m_fLastExtinguishTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlareGun_Revenge )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flaregun_revenge, CTFFlareGun_Revenge );
PRECACHE_WEAPON_REGISTER( tf_weapon_flaregun_revenge );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFFlareGun_Revenge )
END_DATADESC()
#endif

#define TF_MANMELTER_FIRE_RATE 2.0f
#define TF_MANMELTER_AIRBLAST_RATE 0.75f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlareGun_Revenge::CTFFlareGun_Revenge()
{
	m_fLastExtinguishTime = 0.0f;

#ifdef CLIENT_DLL
	m_bReadyToFire = false;
	m_nOldRevengeCrits = 0;
#endif

	StopCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlareGun_Revenge::~CTFFlareGun_Revenge()
{
	StopCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_manmelter_vacuum" );
	PrecacheParticleSystem( "drg_manmelter_vacuum_flames" );
	PrecacheParticleSystem( "drg_manmelter_muzzleflash" );
}

//-----------------------------------------------------------------------------
// Purpose: Fire a laser beam.
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	if ( m_flChargeBeginTime > 0.0f )
		return;

	BaseClass::PrimaryAttack();

	// Lower the reveng crit count
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		pOwner->m_Shared.DeductAirblastCrit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Airblast players.
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::SecondaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Are we capable of firing again?
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
		return;

	if ( GetChargeBeginTime() == 0.0f )
	{
		StartCharge();
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + TF_MANMELTER_AIRBLAST_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFlareGun_Revenge::GetCustomDamageType( void ) const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->m_Shared.GetAirblastCritCount() > 0 )
		return TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT;

	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlareGun_Revenge::Deploy( void )
{
#ifdef CLIENT_DLL
	SetContextThink( &CTFFlareGun_Revenge::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	m_bReadyToFire = false;
#endif

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && BaseClass::Deploy() )
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
bool CTFFlareGun_Revenge::Holster( CBaseCombatWeapon *pSwitchTo )
{
#ifdef CLIENT_DLL
	m_bReadyToFire = false;
	StopCharge();
#endif

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && BaseClass::Holster( pSwitchTo ) )
	{
		pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );

		return true;
	}

	return false;
}

void CTFFlareGun_Revenge::WeaponReset( void )
{
	BaseClass::WeaponReset();

#if defined( CLIENT_DLL )
	StopCharge();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Does the fancy weapon effects.
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_flChargeBeginTime > 0.0f )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		// If we're not holding down the attack button
		if ( !(pPlayer->m_nButtons & IN_ATTACK2) )
			StopCharge();
		else
			ChargePostFrame();
	}

#ifdef CLIENT_DLL
	if ( WeaponState() == WEAPON_IS_ACTIVE )
	{
		if ( GetIndexForThinkContext( "EFFECTS_THINK" ) == NO_THINK_CONTEXT )
			SetContextThink( &CTFFlareGun_Revenge::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );
	}
#endif
}

void CTFFlareGun_Revenge::ChargePostFrame( void )
{
	if ( gpGlobals->curtime > m_fLastExtinguishTime + 0.5f )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			Vector vecEye = pOwner->EyePosition();
			Vector vecForward, vecRight, vecUp;
			AngleVectors( pOwner->EyeAngles(), &vecForward, NULL, NULL );

			const Vector vHull = Vector( 16.0f, 16.0f, 16.0f );

			trace_t tr;
			UTIL_TraceHull( vecEye, vecEye + vecForward * 256.0f, -vHull, vHull, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );

			CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
			if ( pTarget )
			{
			#ifdef GAME_DLL
				CTFPlayer *pBurner = pTarget->m_Shared.GetBurnAttacker();
			#endif
				// Extinguish friends
				if ( ExtinguishPlayerInternal( pTarget, pOwner ) )
				{
					m_fLastExtinguishTime = gpGlobals->curtime;

				#ifdef GAME_DLL
					// Make sure the team isn't burning themselves to earn crits
					if ( pBurner && !pBurner->InSameTeam( pOwner ) )
					{
						if( CanGetAirblastCrits() )
						{
							pOwner->m_Shared.StoreAirblastCrit();


							if ( !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON ) )
								pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED_ACTIVEWEAPON );
						}

						int nRestoreHealthOnExtinguish = 0;
						CALL_ATTRIB_HOOK_INT( nRestoreHealthOnExtinguish, extinguish_restores_health );
						if ( nRestoreHealthOnExtinguish > 0 || tf2v_use_extinguish_heal.GetBool()  )
						{
							const int nHPToRestore = 20;
							int iHealthTaken = pOwner->TakeHealth( nHPToRestore, DMG_GENERIC );

							IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
							if ( event )
							{
								event->SetInt( "amount", iHealthTaken );
								event->SetInt( "entindex", pOwner->entindex() );

								gameeventmanager->FireEvent( event );
							}
						}
					}
				#endif
				}
			}
		}
	}
}

#if defined(CLIENT_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( m_nOldRevengeCrits < pOwner->m_Shared.GetAirblastCritCount() )
		{
			WeaponSound( SPECIAL1 );
			DispatchParticleEffect( "drg_manmelter_vacuum_flames", PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
		}

		m_nOldRevengeCrits = pOwner->m_Shared.GetAirblastCritCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::StartChargeEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		DispatchParticleEffect( "drg_manmelter_vacuum", PATTACH_POINT_FOLLOW, GetAppropriateWorldOrViewModel(), "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::StopChargeEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		GetAppropriateWorldOrViewModel()->ParticleProp()->StopParticlesNamed( "drg_manmelter_vacuum", false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( WeaponState() != WEAPON_IS_ACTIVE )
		return;

	SetContextThink( &CTFFlareGun_Revenge::ClientEffectsThink, gpGlobals->curtime + 0.25f, "EFFECTS_THINK" );

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	if ( GetFlareGunMode() == TF_FLARE_MODE_REVENGE && m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		ParticleProp()->Init( this );
		CNewParticleEffect *pEffect = ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
		if ( pEffect )
		{
			pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetEnergyWeaponColor( false ) );
			pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetEnergyWeaponColor( true ) );
		}

		ParticleProp()->Create( "drg_manmelter_idle", PATTACH_POINT_FOLLOW, "muzzle" );

		if ( !m_bReadyToFire )
		{
			m_bReadyToFire = true;

			EmitSound( "Weapon_SniperRailgun.NonScoped" );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFFlareGun_Revenge::GetCount( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		return pOwner->m_Shared.GetAirblastCritCount();
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Displays the Charge bar.
//-----------------------------------------------------------------------------
bool CTFFlareGun_Revenge::HasChargeBar( void )
{
	if ( CanGetAirblastCrits() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlareGun_Revenge::CanGetAirblastCrits( void ) const
{
	int nAirblastRevenge = 0;
	CALL_ATTRIB_HOOK_INT( nAirblastRevenge, sentry_killed_revenge );
	return nAirblastRevenge == 1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::StartCharge( void )
{
	m_flChargeBeginTime = gpGlobals->curtime;

#ifdef CLIENT_DLL
	if ( !m_pChargeLoop )
	{
		CLocalPlayerFilter filter;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pChargeLoop = controller.SoundCreate( filter, entindex(), GetShootSound( WPN_DOUBLE ) );
		controller.Play( m_pChargeLoop, 1.0, 100 );
	}

	StartChargeEffects();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlareGun_Revenge::StopCharge( void )
{
	m_flChargeBeginTime = 0.0f;

#ifdef CLIENT_DLL
	if ( m_pChargeLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargeLoop );
	}

	m_pChargeLoop = NULL;

	StopChargeEffects();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlareGun_Revenge::ExtinguishPlayerInternal( CTFPlayer *pTarget, CTFPlayer *pOwner )
{
	if ( pOwner->InSameTeam( pTarget ) && pTarget->m_Shared.InCond( TF_COND_BURNING ) )
	{
	#ifdef GAME_DLL
		pTarget->EmitSound( "TFPlayer.FlameOut" );
		pTarget->m_Shared.RemoveCond( TF_COND_BURNING );

		CRecipientFilter filter;
		filter.AddRecipient( pOwner );
		filter.AddRecipient( pTarget );

		UserMessageBegin( filter, "PlayerExtinguished" );
			WRITE_BYTE( pOwner->entindex() );
			WRITE_BYTE( pTarget->entindex() );
		MessageEnd();

		IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
		if ( event )
		{
			event->SetInt( "victim", pTarget->entindex() );
			event->SetInt( "healer", pOwner->entindex() );

			gameeventmanager->FireEvent( event, true );
		}
	#endif // GAME_DLL

		return true;
	}

	return false;
}

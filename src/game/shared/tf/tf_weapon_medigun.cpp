//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:			The Medic's Medikit weapon
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "particles_simple.h"
#include "c_tf_player.h"
#include "soundenvelope.h"
#else
#include "ndebugoverlay.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

#include "tf_weapon_medigun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *g_pszMedigunHealSounds[TF_MEDIGUN_COUNT] =
{
	"WeaponMedigun.Healing",
	"WeaponMedigun.Healing",
	"Weapon_Quick_Fix.Healing",
	"WeaponMedigun_Vaccinator.Healing",
	"WeaponMedigun.Healing"
};

// Buff ranges
ConVar weapon_medigun_damage_modifier( "weapon_medigun_damage_modifier", "1.5", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Scales the damage a player does while being healed with the medigun." );
ConVar weapon_medigun_construction_rate( "weapon_medigun_construction_rate", "10", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Constructing object health healed per second by the medigun." );
ConVar weapon_medigun_charge_rate( "weapon_medigun_charge_rate", "40", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time healing it takes to fully charge the medigun." );
ConVar weapon_medigun_chargerelease_rate( "weapon_medigun_chargerelease_rate", "8", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time it takes the a full charge of the medigun to be released." );


ConVar tf2v_setup_uber_rate("tf2v_setup_uber_rate", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Affects how Uber is built during Setup.", true, 0, true, 2);
ConVar tf2v_uber_juggle_penalty("tf2v_uber_juggle_penalty", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Adds Ubercharge Drain penalty when juggling multiple Uber targets.", true, 0, true, 1);

extern ConVar tf2v_use_medic_speed_match;

#if defined (CLIENT_DLL)
ConVar tf_medigun_autoheal( "tf_medigun_autoheal", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will cause the Medigun's primary attack to be a toggle instead of needing to be held down." );
ConVar hud_medichealtargetmarker("hud_medichealtargetmarker", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO);
#endif

#if !defined (CLIENT_DLL)
ConVar tf_medigun_lagcomp(  "tf_medigun_lagcomp", "1", FCVAR_DEVELOPMENTONLY );
#endif

extern ConVar tf_invuln_time;

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RecvProxy_HealingTarget( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponMedigun *pMedigun = ((CWeaponMedigun*)(pStruct));
	if ( pMedigun != NULL )
	{
		pMedigun->ForceHealingTargetUpdate();
	}

	RecvProxy_IntToEHandle( pData, pStruct, pOut );
}
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_medigun, CWeaponMedigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_medigun );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMedigun, DT_WeaponMedigun )

BEGIN_NETWORK_TABLE( CWeaponMedigun, DT_WeaponMedigun )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flChargeLevel ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hHealingTarget ) ),
	SendPropBool( SENDINFO( m_bHealing ) ),
	SendPropBool( SENDINFO( m_bAttacking ) ),
	SendPropBool( SENDINFO( m_bChargeRelease ) ),
	SendPropBool( SENDINFO( m_bHolstered ) ),
	SendPropFloat( SENDINFO( m_flFinalUberLevel ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargeLevel) ),
	RecvPropEHandle( RECVINFO( m_hHealingTarget ), RecvProxy_HealingTarget ),
	RecvPropBool( RECVINFO( m_bHealing ) ),
	RecvPropBool( RECVINFO( m_bAttacking ) ),
	RecvPropBool( RECVINFO( m_bChargeRelease ) ),
	RecvPropBool( RECVINFO( m_bHolstered ) ),
	RecvPropFloat( RECVINFO(m_flFinalUberLevel) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMedigun  )

	DEFINE_PRED_FIELD( m_bHealing, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAttacking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bHolstered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hHealingTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( m_flHealEffectLifetime, FIELD_FLOAT ),

	DEFINE_PRED_FIELD( m_flChargeLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bChargeRelease, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

//	DEFINE_PRED_FIELD( m_bPlayingSound, FIELD_BOOLEAN ),
//	DEFINE_PRED_FIELD( m_bUpdateHealingTargets, FIELD_BOOLEAN ),

END_PREDICTION_DATA()
#endif

#define PARTICLE_PATH_VEL				140.0
#define NUM_PATH_PARTICLES_PER_SEC		300.0f
#define NUM_MEDIGUN_PATH_POINTS		8
#define TF_MEGAHEAL_BOOST 3.0
#define VACCINATOR_UBER_COUNT 4

extern ConVar tf_max_health_boost;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMedigun::CWeaponMedigun( void )
{
	WeaponReset();

	SetPredictionEligible( true );
}

CWeaponMedigun::~CWeaponMedigun()
{
#ifdef CLIENT_DLL
	if ( m_pChargedSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_flHealEffectLifetime = 0;

	m_bHealing = false;
	m_bAttacking = false;
	m_bHolstered = true;
	m_bChargeRelease = false;

	m_bCanChangeTarget = true;

	m_flNextBuzzTime = 0;
	m_flReleaseStartedAt = 0;
	
	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		int iPreserveUbercharge = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetTFPlayerOwner(), iPreserveUbercharge, preserve_ubercharge );
		m_flChargeLevel = Min( iPreserveUbercharge / 100.0f, m_flChargeLevel.Get() );
	}
	else
	{
		m_flChargeLevel = 0.0f;
	}
	m_flFinalUberLevel = 0.0f;

	RemoveHealingTarget( true );
	m_ExtraInvurns.Purge();

#if defined( CLIENT_DLL )
	m_bPlayingSound = false;
	m_bUpdateHealingTargets = false;
	m_bOldChargeRelease = false;

	UpdateEffects();
	ManageChargeEffect();

	m_pChargeEffect = NULL;
	m_pChargedSound = NULL;
#else
	m_nVaccinatorUberChunks = 0;
#endif

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::Precache()
{
	BaseClass::Precache();

	int iType = GetMedigunType();

	PrecacheScriptSound( g_pszMedigunHealSounds[iType] );

	PrecacheScriptSound( "WeaponMedigun.NoTarget" );
	PrecacheScriptSound( "WeaponMedigun.Charged" );
	PrecacheScriptSound( "WeaponMedigun.HealingDetach" );

	PrecacheTeamParticles( "medicgun_invulnstatus_fullcharge_%s" );
	PrecacheTeamParticles( "medicgun_beam_%s" );
	PrecacheTeamParticles( "medicgun_beam_%s_invun" );

	PrecacheTeamParticles( "vaccinator_%s_beam1" );
	PrecacheTeamParticles( "vaccinator_%s_beam2" );	
	PrecacheTeamParticles( "vaccinator_%s_beam3" );
	
	PrecacheTeamParticles( "vaccinator_%s_buff1" );
	PrecacheTeamParticles( "vaccinator_%s_buff2" );
	PrecacheTeamParticles( "vaccinator_%s_buff3" );

	// Precache charge sounds.
	for ( int i = 0; i < TF_CHARGE_COUNT; i++ )
	{
		PrecacheScriptSound( g_MedigunEffects[i].sound_enable );
		PrecacheScriptSound( g_MedigunEffects[i].sound_disable );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bHolstered = false;

#ifdef GAME_DLL
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( m_bChargeRelease && pOwner )
		{
			pOwner->m_Shared.RecalculateChargeEffects();
		}
#endif

#ifdef CLIENT_DLL
		ManageChargeEffect();
#endif

		m_flNextTargetCheckTime = gpGlobals->curtime;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bHolstered = true;



#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		pOwner->m_Shared.RecalculateChargeEffects( true );
	}
#endif

#ifdef CLIENT_DLL
	UpdateEffects();
	ManageChargeEffect();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateOnRemove( void )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bChargeRelease = false;
	m_bHolstered = true;

#ifndef CLIENT_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		pOwner->m_Shared.RecalculateChargeEffects( true );
	}
#else
	if ( m_bPlayingSound )
	{
		m_bPlayingSound = false;
		StopHealSound();
	}

	UpdateEffects();
	ManageChargeEffect();
#endif

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetTargetRange( void )
{
	return (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetStickRange( void )
{
	return (GetTargetRange() * 1.2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetHealRate( void )
{
	float flHealRate = (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flHealRate, mult_medigun_healrate );
	return flHealRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWeaponMedigun::GetMedigunType( void )
{
	int iType = 0;
	CALL_ATTRIB_HOOK_INT( iType, set_weapon_mode );

	if ( iType >= 0 && iType < TF_MEDIGUN_COUNT )
		return iType;

	AssertMsg( 0, "Invalid medigun type!\n" );
	return TF_MEDIGUN_STOCK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
medigun_charge_types CWeaponMedigun::GetChargeType( void )
{
	int iChargeType = TF_CHARGE_INVULNERABLE;
	CALL_ATTRIB_HOOK_INT( iChargeType, set_charge_type );
	
	// Vaccinator: Swap between the charge types.
	if (iChargeType == 3)
		return (medigun_charge_types)GetCurrentResistanceType();

	if ( iChargeType > TF_CHARGE_NONE && iChargeType < TF_CHARGE_COUNT )
		return (medigun_charge_types)iChargeType;

	AssertMsg( 0, "Invalid charge type!\n" );
	return TF_CHARGE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if this is a medigun that has swappable modes.
//-----------------------------------------------------------------------------
bool CWeaponMedigun::HasMultipleHealingModes(void)
{
	int iChargeType = 0;
	CALL_ATTRIB_HOOK_INT( iChargeType, set_charge_type );
	
	return (iChargeType == 3);
}

//-----------------------------------------------------------------------------
// Purpose: Swaps between Bullet, Explosive, and Fire Resist types.
//-----------------------------------------------------------------------------
void CWeaponMedigun::SwapResistanceType(void)
{
	// If swapping from fire, scroll back to bullet
	if (m_iResistanceType == 5)
		m_iResistanceType = 3;
	else	// Increase from bullet to explosive to fire.
		m_iResistanceType += 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponMedigun::GetHealSound( void )
{
	return g_pszMedigunHealSounds[GetMedigunType()];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::HealingTarget( CBaseEntity *pTarget )
{
	if ( pTarget == m_hHealingTarget )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::AllowedToHealTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
	if ( !pTFPlayer )
		return false;

	int nWeaponBlocksHealing = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, nWeaponBlocksHealing, weapon_blocks_healing );
	if ( nWeaponBlocksHealing == 1 )
		return false;

	bool bStealthed = pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED );
	bool bDisguised = pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED );

	// We can heal teammates and enemies that are disguised as teammates
	if ( !bStealthed &&
		( pTFPlayer->InSameTeam( pOwner ) ||
		( bDisguised && pTFPlayer->m_Shared.GetDisguiseTeam() == pOwner->GetTeamNumber() ) ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::CouldHealTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	if ( pTarget->IsPlayer() && pTarget->IsAlive() && !HealingTarget(pTarget) )
		return AllowedToHealTarget( pTarget );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::MaintainTargetInSlot()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	CBaseEntity *pTarget = m_hHealingTarget;
	Assert( pTarget );

	// Make sure the guy didn't go out of range.
	bool bLostTarget = true;
	Vector vecSrc = pOwner->Weapon_ShootPosition( );
	Vector vecTargetPoint = pTarget->WorldSpaceCenter();
	Vector vecPoint;

	// If it's brush built, use absmins/absmaxs
	pTarget->CollisionProp()->CalcNearestPoint( vecSrc, &vecPoint );

	float flDistance = (vecPoint - vecSrc).Length();
	if ( flDistance < GetStickRange() )
	{
		if ( m_flNextTargetCheckTime > gpGlobals->curtime )
			return;

		m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;

		trace_t tr;
		CMedigunFilter drainFilter( pOwner );

		Vector vecAiming;
		pOwner->EyeVectors( &vecAiming );

		Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
		UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );

		// Still visible?
		if ( tr.m_pEnt == pTarget )
			return;

		UTIL_TraceLine( vecSrc, vecTargetPoint, MASK_SHOT, &drainFilter, &tr );

		// Still visible?
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;

		// If we failed, try the target's eye point as well
		UTIL_TraceLine( vecSrc, pTarget->EyePosition(), MASK_SHOT, &drainFilter, &tr );
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;
	}

	// We've lost this guy
	if ( bLostTarget )
	{
		RemoveHealingTarget();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::FindNewTargetForSlot()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition( );
	if ( m_hHealingTarget )
	{
		RemoveHealingTarget();
	}

	// In Normal mode, we heal players under our crosshair
	Vector vecAiming;
	pOwner->EyeVectors( &vecAiming );

	// Find a player in range of this player, and make sure they're healable.
	Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );
	if ( tr.fraction != 1.0 && tr.m_pEnt )
	{
		if ( CouldHealTarget( tr.m_pEnt ) )
		{
#ifdef GAME_DLL
			pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STARTEDHEALING );
			if ( tr.m_pEnt->IsPlayer() )
			{
				CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
				pTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STARTEDHEALING );
				if ( ShouldUpdateSpeed(pTarget) )
					pOwner->SetMaxSpeed(pTarget->MaxSpeed());
			}

			// Start the heal target thinking.
			SetContextThink( &CWeaponMedigun::HealTargetThink, gpGlobals->curtime, s_pszMedigunHealTargetThink );
#endif

			m_hHealingTarget.Set( tr.m_pEnt );
			m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;
		}			
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::HealTargetThink( void )
{	
	// Verify that we still have a valid heal target.
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( !pTarget || !pTarget->IsAlive() )
	{
		SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	float flTime = gpGlobals->curtime - pOwner->GetTimeBase();
	if ( flTime > 5.0f || !AllowedToHealTarget(pTarget) )
	{
		RemoveHealingTarget( false );
		if (ShouldUpdateSpeed(ToTFPlayer(pTarget)))
			pOwner->TeamFortress_SetSpeed();
	}

	SetNextThink( gpGlobals->curtime + 0.2f, s_pszMedigunHealTargetThink );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponMedigun::IsAttachedToBuilding( void ) const
{
	if ( m_hHealingTarget )
		return m_hHealingTarget->IsBaseObject();

	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to a healable target
//-----------------------------------------------------------------------------
bool CWeaponMedigun::FindAndHealTargets( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	bool bFound = false;

	// Maintaining beam to existing target?
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( pTarget && pTarget->IsAlive() )
	{
		MaintainTargetInSlot();
	}
	else
	{	
		FindNewTargetForSlot();
	}

	CBaseEntity *pNewTarget = m_hHealingTarget;
	if ( pNewTarget && pNewTarget->IsAlive() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pNewTarget );

#ifdef GAME_DLL
		// HACK: For now, just deal with players
		if ( pTFPlayer )
		{
			if ( pTarget != pNewTarget && pNewTarget->IsPlayer() )
			{
				float flHealRate = GetHealRate();
				
				// Heal 3x faster if using the megaheal.
				if ( pTFPlayer->m_Shared.InCond(TF_COND_MEGAHEAL) )
					flHealRate *= TF_MEGAHEAL_BOOST;

				float flOverhealDecayRate = 1.0;
				CALL_ATTRIB_HOOK_FLOAT( flOverhealDecayRate, mult_medigun_overheal_decay );

				// This is a headache....
				float flOverhealBonus = tf_max_health_boost.GetFloat() - 1.0f;

				float flOverhealMult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT( flOverhealMult, mult_medigun_overheal_amount );
				if ( flOverhealMult >= 1.0f )
				{
					flOverhealBonus += flOverhealMult;
				}
				else if ( flOverhealMult < 1.0f && flOverhealBonus > 0.0f )
				{
					flOverhealBonus *= flOverhealMult;
					flOverhealBonus += 1.0f;
				}

				if ( flOverhealBonus <= 1.0f )
					flOverhealBonus = 1.0f;
				
				pTFPlayer->m_Shared.Heal( pOwner, flHealRate, flOverhealBonus, flOverhealDecayRate );
				
			}

			pTFPlayer->m_Shared.RecalculateChargeEffects( false );
		}

		if ( m_flReleaseStartedAt && m_flReleaseStartedAt < (gpGlobals->curtime + 0.2) )
		{
			// When we start the release, everyone we heal rockets to full health
			pNewTarget->TakeHealth( pNewTarget->GetMaxHealth(), DMG_GENERIC );
		}
#endif
	
		bFound = true;

		// Charge up our power if we're not releasing it, and our target
		// isn't receiving any benefit from our healing.
		if ( !m_bChargeRelease )
		{
			if ( pTFPlayer )
			{
				int iBoostMax = floor( pTFPlayer->m_Shared.GetMaxBuffedHealth() * 0.95);

				if ( weapon_medigun_charge_rate.GetFloat() )
				{
					float flChargeAmount = gpGlobals->frametime / weapon_medigun_charge_rate.GetFloat();

					CALL_ATTRIB_HOOK_FLOAT( flChargeAmount, mult_medigun_uberchargerate );

					if ( ( TFGameRules() && TFGameRules()->InSetup() ) && ( tf2v_setup_uber_rate.GetInt() == 2 ) )
					{
						// Build charge at triple rate during setup
						flChargeAmount *= 3.0f;
					}
					else if  ( pNewTarget->GetHealth() >= iBoostMax )
					{
						if ( TFGameRules() && ( !TFGameRules()->InSetup() || ( TFGameRules()->InSetup() && ( tf2v_setup_uber_rate.GetInt() == 0 ) ) ) )
						{
							// Reduced charge for healing fully healed guys
							flChargeAmount *= 0.5f;
						}
					}

					int iTotalHealers = pTFPlayer->m_Shared.GetNumHealers();
					if ( iTotalHealers > 1 )
					{
						flChargeAmount /= (float)iTotalHealers;
					}

					float flNewLevel = min( m_flChargeLevel + flChargeAmount, 1.0 );
#ifdef GAME_DLL
					if ( flNewLevel >= 1.0 && m_flChargeLevel < 1.0 )
					{
						pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );

						if ( pTFPlayer )
						{
							pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
						}
					}
#endif
					m_flChargeLevel = flNewLevel;
				}
			}
		}
	}

	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::AddCharge( float flAmount )
{
	float flChargeRate = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_medigun_uberchargerate );
	if ( !flChargeRate ) // Can't earn uber.
		return;

	float flNewLevel = min( m_flChargeLevel + flAmount, 1.0 );

#ifdef GAME_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	CTFPlayer *pHealingTarget = ToTFPlayer( m_hHealingTarget );

	if ( !m_bChargeRelease && flNewLevel >= 1.0 && m_flChargeLevel < 1.0 )
	{
		if ( pPlayer )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );
		}

		if ( pHealingTarget )
		{
			pHealingTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
		}
	}
#endif

	m_flChargeLevel = flNewLevel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::RemoveCharge( float flAmount, bool bDrainSound /* = false */ )
{
	float flChargeRate = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flChargeRate, mult_medigun_uberchargerate );
	if ( !flChargeRate ) // Can't earn uber, don't bother.
		return;

	// Never let us fall under 0%.
	if ( m_flChargeLevel > 0 )
	{
		float flNewLevel = max( m_flChargeLevel - ( flAmount / 100), 0.0 );
		
#ifdef GAME_DLL
		if (bDrainSound)
		{
			// Emit a sound, to warn us that our charge was drained.
			CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
			if (pOwner)
			{
				CSingleUserRecipientFilter filter( pOwner );
				pOwner->EmitSound( filter, pOwner->entindex(), "Weapon_Pomson.DrainedVictim" );
			}
		}
#endif

		m_flChargeLevel = flNewLevel;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	DrainCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::DrainCharge( void )
{
	// If we're in charge release mode, drain our charge
	if ( m_bChargeRelease )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		float flChargeAmount = gpGlobals->frametime / weapon_medigun_chargerelease_rate.GetFloat();
		
		// Targets other than our own Uber suffer drain penalties.
		if ( tf2v_uber_juggle_penalty.GetBool() )
		{
			int nUberedPlayers = 1;
			// Check who is ubered by us and who isn't.
			if (m_ExtraInvurns.Count() != NULL )
			{
				for (int i = 0; i < m_ExtraInvurns.Count(); i++)
				{
					if (m_ExtraInvurns[i].hPlayer && m_ExtraInvurns[i].hPlayer->IsAlive())
					{
						if (m_ExtraInvurns[i].hPlayer.Get() != m_hHealingTarget.Get() &&	// Not our current target
							((m_ExtraInvurns[i].flTime + tf_invuln_time.GetFloat()) > gpGlobals->curtime))	// Affected by Uber
						{
							// Add to our count.
							nUberedPlayers++;
							continue;
						}
					}
					// Remove from our list.
					m_ExtraInvurns.Remove(i);

				}
				// Adjust our drain rate for the extra players.
				if (nUberedPlayers > 1)
					flChargeAmount += ((flChargeAmount * 0.5) * (nUberedPlayers - 1));
			}
		}
		
		m_flChargeLevel = max( m_flChargeLevel - flChargeAmount, m_flFinalUberLevel );
		if ( m_flChargeLevel <= m_flFinalUberLevel )
		{
			m_bChargeRelease = false;
			m_flReleaseStartedAt = 0;
			m_flFinalUberLevel = 0;
			m_ExtraInvurns.Purge();

#ifdef GAME_DLL

			if ( m_bHealingSelf )
			{
				m_bHealingSelf = false;
				pOwner->m_Shared.StopHealing( pOwner );
			}

			pOwner->m_Shared.RecalculateChargeEffects();
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the hold-down healing
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	// If we're lowered, we're not allowed to fire
	if ( CanAttack() == false )
	{
		RemoveHealingTarget( true );
		return;
	}

#if !defined( CLIENT_DLL )
	if ( AppliesModifier() )
	{
		m_DamageModifier.SetModifier( weapon_medigun_damage_modifier.GetFloat() );
	}
#endif

	// Try to start healing
	m_bAttacking = false;
	if ( pOwner->GetMedigunAutoHeal() )
	{
		if ( pOwner->m_nButtons & IN_ATTACK )
		{
			if ( m_bCanChangeTarget )
			{
				RemoveHealingTarget();
#if defined( CLIENT_DLL )
				m_bPlayingSound = false;
				StopHealSound();
#endif
				// can't change again until we release the attack button
				m_bCanChangeTarget = false;
			}
		}
		else
		{
			m_bCanChangeTarget = true;
		}

		if ( m_bHealing || ( pOwner->m_nButtons & IN_ATTACK ) )
		{
			PrimaryAttack();
			m_bAttacking = true;
		}
	}
	else
	{
		if ( /*m_bChargeRelease || */ pOwner->m_nButtons & IN_ATTACK )
		{
			PrimaryAttack();
			m_bAttacking = true;
		}
 		else if ( m_bHealing )
 		{
 			// Detach from the player if they release the attack button.
 			RemoveHealingTarget();
 		}
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		SecondaryAttack();
	}
	
	if ( pOwner->m_nButtons & IN_RELOAD )
	{
		if ( HasMultipleHealingModes() )
		{
			// Swap our current resistance type, and play a sound.
			SwapResistanceType();
			EmitSound( "WeaponMedigun_Vaccinator.Toggle" );
		}
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Lower( void )
{
	// Stop healing if we are
	if ( m_bHealing )
	{
		RemoveHealingTarget( true );
		m_bAttacking = false;

#ifdef CLIENT_DLL
		UpdateEffects();
#endif
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::RemoveHealingTarget( bool bSilent )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	// If this guy is already in our detached target list, update the time. Otherwise, add him.
	if ( m_bChargeRelease )
	{
		int i = 0;
		for ( i = 0; i < m_ExtraInvurns.Count(); i++ )
		{
			if ( m_ExtraInvurns[i].hPlayer == m_hHealingTarget )
			{
				m_ExtraInvurns[i].flTime = gpGlobals->curtime;
				break;
			}
		}
		if ( i == m_ExtraInvurns.Count() )
		{
			int iIdx = m_ExtraInvurns.AddToTail();
			m_ExtraInvurns[iIdx].hPlayer = m_hHealingTarget;
			m_ExtraInvurns[iIdx].flTime = gpGlobals->curtime;
		}
	}

#ifdef GAME_DLL
	if ( m_hHealingTarget )
	{
		// HACK: For now, just deal with players
		if ( m_hHealingTarget->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
			pTFPlayer->m_Shared.StopHealing( pOwner );
			pTFPlayer->m_Shared.RecalculateChargeEffects( false );

			if ( !bSilent )
			{
				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STOPPEDHEALING, pTFPlayer->IsAlive() ? "healtarget:alive" : "healtarget:dead" );
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STOPPEDHEALING );
			}
		}
	}

	// Stop thinking - we no longer have a heal target.
	SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
#endif

	m_hHealingTarget.Set( NULL );

	// Stop the welding animation
	if ( m_bHealing && !bSilent )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
	}

#ifndef CLIENT_DLL
	m_DamageModifier.RemoveModifier();
#endif
	m_bHealing = false;

}


//-----------------------------------------------------------------------------
// Purpose: Attempt to heal any player within range of the medikit
//-----------------------------------------------------------------------------
void CWeaponMedigun::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;


	if ( !CanAttack() )
		return;

#ifdef GAME_DLL

	int iChargeType = TF_CHARGE_INVULNERABLE;
	CALL_ATTRIB_HOOK_INT(iChargeType, set_charge_type);

	// Start boosting ourself if we're not
	if (m_bChargeRelease && !m_bHealingSelf && iChargeType == TF_CHARGE_MEGAHEAL)
	{
		float flHealRate = GetHealRate() * TF_MEGAHEAL_BOOST;
		pOwner->m_Shared.Heal( pOwner, flHealRate, 1.0f, 0.0f );
		m_bHealingSelf = true;
	}

#endif

#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
#endif

	if ( FindAndHealTargets() )
	{
		// Start the animation
		if ( !m_bHealing )
		{
#ifdef GAME_DLL
			pOwner->SpeakWeaponFire();
#endif

			SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
		}

		m_bHealing = true;
	}
	else
	{
		RemoveHealingTarget();
	}
	
#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->FinishLagCompensation( pOwner );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Burn charge level to generate invulnerability
//-----------------------------------------------------------------------------
void CWeaponMedigun::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;
	
	bool bHasFullCharge = (m_flChargeLevel < 1.0);
	if ( HasMultipleHealingModes() )
		bHasFullCharge = m_flChargeLevel < (1.0 / VACCINATOR_UBER_COUNT );

	// Ensure they have a full charge and are not already in charge release mode
	if ( bHasFullCharge || m_bChargeRelease )
	{
#ifdef CLIENT_DLL
		// Deny, flash
		if ( !m_bChargeRelease && m_flFlashCharge <= 0 )
		{
			m_flFlashCharge = 10;
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
		}
#endif
		return;
	}

	if ( pOwner->HasTheFlag() )
	{
#ifdef GAME_DLL
		CSingleUserRecipientFilter filter( pOwner );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_INVULN_WITH_FLAG );
#endif
		pOwner->EmitSound( "Player.DenyWeaponSelection" );
		return;
	}

	// Start super charge
	m_bChargeRelease = true;
	m_flReleaseStartedAt = 0;//gpGlobals->curtime;
	
	// Log our Uber level so we stop there.
	if ( HasMultipleHealingModes() )
		m_flFinalUberLevel = m_flChargeLevel - ( m_flChargeLevel / VACCINATOR_UBER_COUNT );
	else	// Stop our uber when it drains completely.
		m_flFinalUberLevel = 0;

#ifdef GAME_DLL
	// Award invurn point for every full Ubercharge bar deployed.
	if ( HasMultipleHealingModes() )
	{
		m_nVaccinatorUberChunks += 1;
		if (m_nVaccinatorUberChunks == VACCINATOR_UBER_COUNT)
		{
			// Award a point and reset our counter.
			CTF_GameStats.Event_PlayerInvulnerable( pOwner );
			m_nVaccinatorUberChunks = 0;
		}
	}
	else
		CTF_GameStats.Event_PlayerInvulnerable( pOwner );
	
	pOwner->m_Shared.RecalculateChargeEffects();

	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEDEPLOYED );

	if ( m_hHealingTarget && m_hHealingTarget->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
		pTFPlayer->m_Shared.RecalculateChargeEffects();
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED );
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_chargedeployed" );
	if ( event )
	{
		event->SetInt( "userid", pOwner->GetUserID() );

		gameeventmanager->FireEvent( event, true );	// don't send to clients
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Idle tests to see if we're facing a valid target for the medikit
//			If so, move into the "heal-able" animation. 
//			Otherwise, move into the "not-heal-able" animation.
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		// Loop the welding animation
		if ( m_bHealing )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			return;
		}

		return BaseClass::WeaponIdle();
	}
}

bool CWeaponMedigun::ShouldUpdateSpeed(CTFPlayer *pTarget)
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;
	
	int nCanFollowCharges = 0;
	CALL_ATTRIB_HOOK_INT( nCanFollowCharges, set_weapon_mode);
	// Beta Quick Fix: Always match speed.
	if ( nCanFollowCharges == 4 ) 
		return true;
	// Quick-Fix: Follow shield charges, but only when we are allowed to match speed.
	else if ( nCanFollowCharges == 2 && tf2v_use_medic_speed_match.GetBool() )
		return true;
	// Other mediguns can match speed, but only when allowed to and not on shield charges.
	else if ( tf2v_use_medic_speed_match.GetBool() && !pTarget->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return true;
	
	return false;
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::StopHealSound( bool bStopHealingSound, bool bStopNoTargetSound )
{
	if ( bStopHealingSound )
	{
		StopSound( GetHealSound() );

		CLocalPlayerFilter filter;
		EmitSound( filter, entindex(), "WeaponMedigun.HealingDetach" );
	}

	if ( bStopNoTargetSound )
	{
		StopSound( "WeaponMedigun.NoTarget" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ManageChargeEffect( void )
{
	bool bOwnerTaunting = false;

	if ( GetTFPlayerOwner() && ( GetTFPlayerOwner()->m_Shared.InCond( TF_COND_TAUNTING ) == true  || GetTFPlayerOwner()->m_Shared.InCond( TF_COND_STUNNED ) == true) )
	{
		bOwnerTaunting = true;
	}
	
	// Run uber effects on a full bar, unless we use Vaccinator chunks.
	bool bAtUberLevel = (m_flChargeLevel >= 1.0f);
	if ( HasMultipleHealingModes() )
		bAtUberLevel = m_flChargeLevel >= (1.0f / VACCINATOR_UBER_COUNT );
	

	if ( GetTFPlayerOwner() && bOwnerTaunting == false && m_bHolstered == false && ( bAtUberLevel || m_bChargeRelease == true ) )
	{
		C_BaseEntity *pEffectOwner = GetWeaponForEffect();

		if ( pEffectOwner && m_pChargeEffect == NULL )
		{
			const char *pszEffectName = ConstructTeamParticle( "medicgun_invulnstatus_fullcharge_%s", GetTFPlayerOwner()->GetTeamNumber() );

			m_pChargeEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
			m_hChargeEffectHost = pEffectOwner;
		}

		if ( m_pChargedSound == NULL )
		{
			CLocalPlayerFilter filter;

			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			
			if ( HasMultipleHealingModes() )
			{
				// Vaccinator has multiple Ubercharge level sounds.
				switch ((int)floor( m_flChargeLevel / (1.0f / VACCINATOR_UBER_COUNT ) ) )
				{
					case 1:	// Bar at 25%
					m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun_Vaccinator.Charged_tier_01" );
					break;
					case 2:	// Bar at 50%
					m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun_Vaccinator.Charged_tier_02" );
					break;					
					case 3:	// Bar at 75%
					m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun_Vaccinator.Charged_tier_03" );
					break;					
					case 4:	// Bar at 100%
					m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun_Vaccinator.Charged_tier_04" );
					break;					
				}
			}
			else
				m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun.Charged" );
			
			controller.Play( m_pChargedSound, 1.0, 100 );
		}
	}
	else
	{
		C_BaseEntity *pEffectOwner = m_hChargeEffectHost.Get();

		if ( m_pChargeEffect != NULL )
		{
			if ( pEffectOwner )
			{
				pEffectOwner->ParticleProp()->StopEmission( m_pChargeEffect );
				m_hChargeEffectHost = NULL;
			}

			m_pChargeEffect = NULL;
		}

		if ( m_pChargedSound != NULL )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
			m_pChargedSound = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CWeaponMedigun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}

	// Think?
	if ( m_bHealing )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}
	else
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		if ( m_bPlayingSound )
		{
			m_bPlayingSound = false;
			StopHealSound( true, false );
		}

		// Are they holding the attack button but not healing anyone? Give feedback.
		if ( IsActiveByLocalPlayer() && GetOwner() && GetOwner()->IsAlive() && m_bAttacking && GetOwner() == C_BasePlayer::GetLocalPlayer() && CanAttack() == true )
		{
			if ( gpGlobals->curtime >= m_flNextBuzzTime )
			{
				CLocalPlayerFilter filter;
				EmitSound( filter, entindex(), "WeaponMedigun.NoTarget" );
				m_flNextBuzzTime = gpGlobals->curtime + 0.5f;	// only buzz every so often.
			}
		}
		else
		{
			StopHealSound( false, true );	// Stop the "no target" sound.
		}
	}

	ManageChargeEffect();
	

	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ClientThink()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	// Don't show it while the player is dead. Ideally, we'd respond to m_bHealing in OnDataChanged,
	// but it stops sending the weapon when it's holstered, and it gets holstered when the player dies.
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer || pFiringPlayer->IsPlayerDead() || pFiringPlayer->IsDormant() )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );

		if ( m_bPlayingSound )
		{
			m_bPlayingSound = false;
			StopHealSound();
		}
		return;
	}
		
	// If the local player is the guy getting healed, let him know 
	// who's healing him, and their charge level.
	if( m_hHealingTarget != NULL )
	{
		if ( pLocalPlayer == m_hHealingTarget )
		{
			pLocalPlayer->SetHealer( pFiringPlayer, m_flChargeLevel );
		}

		if ( !m_bPlayingSound )
		{
			m_bPlayingSound = true;
			CLocalPlayerFilter filter;
			EmitSound( filter, entindex(), GetHealSound() );
		}
	}

	if ( m_bOldChargeRelease != m_bChargeRelease )
	{
		m_bOldChargeRelease = m_bChargeRelease;
		ForceHealingTargetUpdate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateEffects( void )
{
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer )
		return;

	C_BaseEntity *pEffectOwner = m_hHealingTargetEffect.hOwner.Get();

	// Remove all the effects
	if ( m_hHealingTargetEffect.pEffect && pEffectOwner )
	{
		pEffectOwner->ParticleProp()->StopEmission( m_hHealingTargetEffect.pEffect );
		pEffectOwner->ParticleProp()->StopEmission( m_hHealingTargetEffect.pCustomEffect );
	}

	m_hHealingTargetEffect.hOwner = NULL;
	m_hHealingTargetEffect.pTarget = NULL;
	m_hHealingTargetEffect.pEffect = NULL;
	m_hHealingTargetEffect.pCustomEffect = NULL;

	pEffectOwner = GetWeaponForEffect();

	// Don't add targets if the medic is dead
	if ( !pEffectOwner || pFiringPlayer->IsPlayerDead() )
		return;

	// Add our targets
	// Loops through the healing targets, and make sure we have an effect for each of them
	if ( m_hHealingTarget )
	{
		if ( m_hHealingTargetEffect.pTarget == m_hHealingTarget )
			return;

		const char *pszFormat = 0;
		// Unused asset from TF2:
		// Vaccinator's different beams have different particle effects.
		if ( HasMultipleHealingModes() )
		{
			switch (GetCurrentResistanceType())
			{
				case 3:	// Bullet
					pszFormat = "vaccinator_%s_beam1";
				break;
				case 4: // Explosive
					pszFormat = "vaccinator_%s_beam2";
				break;
				case 5: // Fire
					pszFormat = "vaccinator_%s_beam3";
				break;
			}
		}
		else
		{
			if ( hud_medichealtargetmarker.GetBool() && !IsReleasingCharge() )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if (pLocalPlayer && ( pLocalPlayer == pFiringPlayer ) )
					pszFormat = "medicgun_beam_%s_targeted" ;
				
				pszFormat = "medicgun_beam_%s";
			}
			else
				pszFormat = IsReleasingCharge() ? "medicgun_beam_%s_invun" : "medicgun_beam_%s";
		}
		
		const char *pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber() );

		CNewParticleEffect *pEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
		pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );

		CEconItemDefinition *pStatic = GetItem()->GetStaticData();
		if ( pStatic )
		{
			const char *pszCustomEffectName = NULL;
			PerTeamVisuals_t *pVisuals = pStatic->GetVisuals( TEAM_UNASSIGNED );
			if ( pVisuals )
			{
				pszCustomEffectName = pVisuals->GetCustomParticleSystem();
			}

			pVisuals = pStatic->GetVisuals( GetTeamNumber() );
			if ( pVisuals )
			{
				pszCustomEffectName = pVisuals->GetCustomParticleSystem();
			}

			if ( pszCustomEffectName && pszCustomEffectName[0] != '\0' )
			{
				CNewParticleEffect *pCustomEffect = pEffectOwner->ParticleProp()->Create( pszCustomEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
				pEffectOwner->ParticleProp()->AddControlPoint( pCustomEffect, 1, m_hHealingTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0,0,50) );

				m_hHealingTargetEffect.pCustomEffect = pCustomEffect;
			}
		}

		m_hHealingTargetEffect.hOwner = pEffectOwner;
		m_hHealingTargetEffect.pTarget = m_hHealingTarget;
		m_hHealingTargetEffect.pEffect = pEffect;
	}
}

#endif

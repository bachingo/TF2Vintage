//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_weapon_knife.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "ilagcompensationmanager.h"
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Knife tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFKnife, DT_TFWeaponKnife )

BEGIN_NETWORK_TABLE( CTFKnife, DT_TFWeaponKnife )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bReadyToBackstab ) ),
	RecvPropFloat( RECVINFO( m_flKnifeRegenTime ) ),
	RecvPropBool( RECVINFO( m_bForcedSwap ) ),
	RecvPropFloat( RECVINFO( m_flSwapBlocked ) ),
#else
	SendPropBool( SENDINFO( m_bReadyToBackstab ) ),
	SendPropFloat( SENDINFO( m_flKnifeRegenTime ), 0, SPROP_NOSCALE ),
	SendPropBool( SENDINFO( m_bForcedSwap ) ),
	SendPropFloat( SENDINFO( m_flSwapBlocked), 0, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKnife )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bReadyToBackstab, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
PRECACHE_WEAPON_REGISTER( tf_weapon_knife );


ConVar tf2v_use_new_backstabs( "tf2v_use_new_backstabs", "2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes knife backstab behavior.", true, 0, true, 2 );

//=============================================================================
//
// Weapon Knife functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFKnife::CTFKnife()
{
	m_flSwapBlocked = false;
	m_bForcedSwap = false;
	m_flKnifeRegenTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to backstab.
//-----------------------------------------------------------------------------
bool CTFKnife::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bReadyToBackstab = false;
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::ItemPostFrame( void )
{
	BackstabVMThink();
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Set stealth attack bool
//-----------------------------------------------------------------------------
void CTFKnife::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	if ( !pPlayer || !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	int iVictimHealth = 0;
	m_hBackstabVictim = NULL;

	trace_t trace;
	if ( DoSwingTrace( trace ) == true )
	{
		// we will hit something with the attack
		if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
		{
			CTFPlayer *pTarget = ToTFPlayer( trace.m_pEnt );

			if ( pTarget && pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() )
			{
				// Check to see if we can backstab.
				bool bCanBackstab = IsBehindTarget(pTarget);
				if ( tf2v_use_new_backstabs.GetInt() == 2 ) // Must be readied before allowed to backstab
					bCanBackstab = ( bCanBackstab && m_bReadyToBackstab );
				// Deal extra damage to players when stabbing them from behind
				if ( bCanBackstab )
				{
					// this will be a backstab, do the strong anim
					m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

					// store the victim to compare when we do the damage
					m_hBackstabVictim = pTarget;
					iVictimHealth = pTarget->GetHealth();
				}
			}
		}
	}

#if !defined(CLIENT_DLL)
	pPlayer->RemoveInvisibility();

	lagcompensation->FinishLagCompensation( pPlayer );

	// Reset "backstab ready" state after each attack.
	m_bReadyToBackstab = false;
#endif

	// Swing the weapon.
	Swing( pPlayer );
	
	if (tf2v_use_new_backstabs.GetInt() == 2 )
	{
		// And hit instantly.
		Smack();
		m_flSmackTime = -1.0f;
	}

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );

	int nDisguiseOnBackstab = 0;
	CALL_ATTRIB_HOOK_INT( nDisguiseOnBackstab, set_disguise_on_backstab );

	float flDisguiseSpeed = 0.1;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flDisguiseSpeed, disguise_speed_penalty );
	
	if( nDisguiseOnBackstab == 0 || !m_hBackstabVictim || m_hBackstabVictim->IsAlive() || pPlayer->HasTheFlag() )
	{
		pPlayer->RemoveDisguise();
	}
	else
	{
		if( flDisguiseSpeed > 0.1f )
			pPlayer->RemoveDisguise();

		SetContextThink( &CTFKnife::DisguiseOnKill, gpGlobals->curtime + flDisguiseSpeed, "DisguiseOnKill" );
	}

	int nSanguisuge = 0;
	CALL_ATTRIB_HOOK_INT( nSanguisuge, sanguisuge );
	if ( nSanguisuge != 0 )
	{
		if ( !m_hBackstabVictim || m_hBackstabVictim->IsAlive() )
			return;

		int nHealthToSteal = Max( 5, iVictimHealth );
		int nHealthToAdd = clamp(nHealthToSteal, 0, ((pPlayer->m_Shared.GetMaxBuffedHealth() * 2) - pPlayer->GetHealth()));
		if ( nHealthToAdd > 0 )
		{
			pPlayer->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH );
			pPlayer->m_Shared.HealthKitPickupEffects( nHealthToAdd );
			pPlayer->m_Shared.SetNextSanguisugeDecay();
			pPlayer->m_Shared.ChangeSanguisugeHealth(nHealthToAdd);
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFKnife::GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iDamageType, iCustomDamage );

	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner && pTarget->IsPlayer() )
	{

		bool bIsBackstab = ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget ); // Since Swing and Smack are done in the same frame now we don't need to run additional checks anymore.
		if (tf2v_use_new_backstabs.GetInt() != 2) // Unless we use old backstabs.
		bIsBackstab = IsBehindTarget(pTarget) || bIsBackstab; 

		if (bIsBackstab)
		{
			// this will be a backstab, do the strong anim.
			if (!TFGameRules()->IsBossClass(pTarget))
			{
				// Do twice the target's health so that random modification will still kill him.
				flBaseDamage = pTarget->GetHealth() * 2;
			}
			else
			{
				// If we're backstabbing a boss class, we don't do an instant kill but rather based on the total health.
				int iBossMaxHealth = pTarget->GetMaxHealth();
				float flBossDamageModifier = (0.784314 + 512.0);
				flBaseDamage = pow(float(iBossMaxHealth), flBossDamageModifier);
			}
			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;
		}
	}

	return flBaseDamage;
}


//-----------------------------------------------------------------------------
// Purpose: Newer backstab check to prevent facestabs.
//-----------------------------------------------------------------------------
bool CTFKnife::IsBehindAndFacingTarget( CBaseEntity *pTarget )
{
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	// Get a forward vector of the attacker.
	Vector vecOwnerForward;
	AngleVectors( GetOwner()->EyeAngles(), &vecOwnerForward );
	vecOwnerForward.z = 0.0f;
	vecOwnerForward.NormalizeInPlace();

	float flDotOwner = DotProduct( vecOwnerForward, vecToTarget );
	float flDotVictim = DotProduct( vecVictimForward, vecToTarget );

	// Make sure they're actually facing the target.
	// This needs to be done because lag compensation can place target slightly behind the attacker.
	if ( flDotOwner > 0.5 )
		return ( flDotVictim > -0.1 );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: The old, original backstab check.
//-----------------------------------------------------------------------------
bool CTFKnife::IsBehindTarget( CBaseEntity *pTarget )
{
	// If using newer backstabs, use the better backstab algorithm instead.
	if ( tf2v_use_new_backstabs.GetInt() == 2 )
		return IsBehindAndFacingTarget(pTarget);
	
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	float flDot = DotProduct( vecVictimForward, vecToTarget );

	return ( flDot > -0.1 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFKnife::CalcIsAttackCriticalHelper( void )
{
	// Always crit from behind, never from front
	return ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if we raise or lower our knife.
// 			This allows our knife to be much more responsive.
//-----------------------------------------------------------------------------
void CTFKnife::WeaponIdle( void )
{
	BackstabVMThink();
	return BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFKnife::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKnife::DoViewModelAnimation( void )
{
	// Overriding so it doesn't do backstab animation on crit.
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to backstab.
//-----------------------------------------------------------------------------
bool CTFKnife::SendWeaponAnim( int iActivity )
{
	switch( iActivity )
	{
	case ACT_VM_IDLE:
	case ACT_MELEE_VM_IDLE:
	case ACT_ITEM1_VM_IDLE:
	case ACT_ITEM2_VM_IDLE:
		if ( m_bReadyToBackstab )
			iActivity = ACT_BACKSTAB_VM_IDLE;
		break;
	case ACT_BACKSTAB_VM_UP:
	case ACT_ITEM1_BACKSTAB_VM_UP:
	case ACT_ITEM2_BACKSTAB_VM_UP:
		m_bReadyToBackstab = true;
		break;
	case ACT_BACKSTAB_VM_DOWN:
	case ACT_ITEM1_BACKSTAB_VM_DOWN:
	case ACT_ITEM2_BACKSTAB_VM_DOWN:
	default:
		m_bReadyToBackstab = false;
		break;
	}
	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Check for knife raise conditions.
//-----------------------------------------------------------------------------
void CTFKnife::BackstabVMThink( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;
	
	// Don't raise the knife when cloaked.
	if ( pOwner->m_Shared.InCond(TF_COND_STEALTHED) || pOwner->m_Shared.InCond(TF_COND_STEALTHED_BLINK) )
	{
		if ( m_bReadyToBackstab )
		{
			SendWeaponAnim( ACT_BACKSTAB_VM_DOWN );
			m_bReadyToBackstab = false;
		}
		return;
	}

	
	if ( GetActivity() == ACT_VM_IDLE ||
		GetActivity() == ACT_MELEE_VM_IDLE ||
		GetActivity() == ACT_BACKSTAB_VM_IDLE ||
		GetActivity() == ACT_ITEM1_VM_IDLE  ||
		GetActivity() == ACT_ITEM1_BACKSTAB_VM_IDLE ||
		GetActivity() == ACT_ITEM2_VM_IDLE  ||
		GetActivity() == ACT_ITEM2_BACKSTAB_VM_IDLE )
	{
		trace_t tr;
		if ( CanAttack() && DoSwingTrace( tr ) &&
			tr.m_pEnt->IsPlayer() && tr.m_pEnt->GetTeamNumber() != pOwner->GetTeamNumber() &&
			IsBehindTarget(tr.m_pEnt))
		{
			if ( !m_bReadyToBackstab )
			{
				SendWeaponAnim( ACT_BACKSTAB_VM_UP );
				m_bReadyToBackstab = true;
			}
		}
		else
		{
			if ( m_bReadyToBackstab )
			{
				SendWeaponAnim( ACT_BACKSTAB_VM_DOWN );
				m_bReadyToBackstab = false;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKnife::DisguiseOnKill( void )
{
	if ( !m_hBackstabVictim )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	int iTeamNum = m_hBackstabVictim->GetTeamNumber();
	int iClassIdx = m_hBackstabVictim->GetPlayerClass()->GetClassIndex();
	pOwner->m_Shared.Disguise( iTeamNum, iClassIdx, m_hBackstabVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKnife::BackstabBlocked( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	m_flNextPrimaryAttack = gpGlobals->curtime + 2.0f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 2.0f;
	m_flSwapBlocked = gpGlobals->curtime + 2.0f;

	SendWeaponAnim( ACT_MELEE_VM_PULLBACK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFKnife::CanDeploy( void )
{
	// Haven't regenerated yet, can't use.
	if ( gpGlobals->curtime < m_flKnifeRegenTime && m_flKnifeRegenTime > 0 )
	{
		return false;
	}
	
	if (BaseClass::CanDeploy() && m_flKnifeRegenTime != 0)
	{
		// Reset our regen time.
		m_flKnifeRegenTime = 0;
		return true;
	}

	return BaseClass::CanDeploy();
}


// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFKnife::CanHolster( void )
{
	// Forced holster
	if ( m_bForcedSwap )
		return true;
	
	// Got our backstab blocked? Don't let us swap.
	if ( m_flSwapBlocked > gpGlobals->curtime )
		return false;

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose: Checks if we can extinguish ourselves.
// -----------------------------------------------------------------------------
bool CTFKnife::CanExtinguish( void )
{
	int nMeltsinFire = 0;
	CALL_ATTRIB_HOOK_INT( nMeltsinFire, melts_in_fire );
	if (nMeltsinFire)
	{
		// If we have a knife right now, we can extinguish ourselves.
		if ( gpGlobals->curtime > m_flKnifeRegenTime )
			return true;
	}
	return false;
}

#define TF_KNIFE_REGEN_TIME 15
// -----------------------------------------------------------------------------
// Purpose: Processess the extinguish effects.
// -----------------------------------------------------------------------------
void CTFKnife::Extinguish( void )
{
	// Set our knife regeneration time.
	m_flKnifeRegenTime = gpGlobals->curtime + TF_KNIFE_REGEN_TIME;
	
	CTFPlayer *pTFOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pTFOwner )
	{
		// Make the melting sound.
		pTFOwner->EmitSound( "Icicle.Melt" );
		
		// Force ourselves to swap weapon.
		pTFOwner->SwitchToNextBestWeapon(this);
	}
}


// -----------------------------------------------------------------------------
// Purpose: Displays if we have a charge bar or not.
// -----------------------------------------------------------------------------
bool CTFKnife::HasChargeBar( void )
{
	int nMeltsinFire = 0;
	CALL_ATTRIB_HOOK_INT( nMeltsinFire, melts_in_fire );
	if (nMeltsinFire)
		return true;
	
	return false;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFKnife::GetEffectBarProgress(void)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return ( Min( gpGlobals->curtime - m_flKnifeRegenTime, (float) TF_KNIFE_REGEN_TIME) / TF_KNIFE_REGEN_TIME );
	}

	return 0.0f;
}

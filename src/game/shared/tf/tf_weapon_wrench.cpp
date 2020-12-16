//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "decals.h"
#include "baseobject_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "variant_t.h"
#endif

//=============================================================================
//
// Weapon Wrench tables.
//
CREATE_SIMPLE_WEAPON_TABLE( TFWrench, tf_weapon_wrench )
CREATE_SIMPLE_WEAPON_TABLE( TFRobotArm, tf_weapon_robot_arm )

//=============================================================================
//
// Weapon Wrench functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWrench::CTFWrench()
{
}

#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector vecHitPos )
{
	// Did this object hit do any work? repair or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, vecHitPos );
	
	CDisablePredictionFiltering disabler;

	if ( pObject->IsDisposableBuilding() )
	{
		CSingleUserRecipientFilter singleFilter( pPlayer );
		EmitSound( singleFilter, pObject->entindex(), "Player.UseDeny" );
	}
	else
	{
		if ( bUsefulHit )
		{
			// play success sound
			WeaponSound( SPECIAL1 );
		}
		else
		{
			// play failure sound
			WeaponSound( SPECIAL2 );
		}
	}
}
#endif

void CTFWrench::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ), pPlayer, trace.endpos );
#endif
	}
	else
	{
		// if we cannot, Smack as usual for player hits
		BaseClass::Smack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if our wrench is the Eureka Effect variant.
//-----------------------------------------------------------------------------
bool CTFWrench::IsEurekaEffect( void )
{
	int nEureka = 0;
	CALL_ATTRIB_HOOK_INT(nEureka, alt_fire_teleport_to_spawn);
	
	return (nEureka != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Checks if our wrench is the Eureka Effect variant.
//-----------------------------------------------------------------------------
void CTFWrench::ItemPostFrame( void )
{
#ifdef GAME_DLL
	if (IsEurekaEffect())
	{
		// Eureka Effect checks if we pressed the reload key.
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner->m_nButtons & IN_RELOAD )
		EurekaTeleport();
		
	}
#endif
	BaseClass::ItemPostFrame();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWrench::ApplyBuildingHealthUpgrade( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	for ( int i = pPlayer->GetObjectCount(); --i >= 0; )
	{
		CBaseObject *pObj = pPlayer->GetObject( i );
		if ( pObj )
		{
			pObj->ApplyHealthUpgrade();
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets up our teleporting.
//-----------------------------------------------------------------------------
void CTFWrench::EurekaTeleport( void )
{
	// Get our owner.
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if (!pOwner)
		return;
	
	if ( pOwner->IsAllowedToTaunt() )
	{
		pOwner->StartEurekaTeleport();
		pOwner->SetEurekaTeleportTime();
		pOwner->Taunt( TAUNT_EUREKA, MP_CONCEPT_TAUNT_EUREKA_EFFECT_TELEPORT );
	}
	
}
#endif

//=============================================================================
//
// Weapon Robot Arm functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::Smack( void )
{
	//TODO: check if this needs lag compensation

	trace_t tr;

	// Did we hit an enemy player?
	if ( DoSwingTrace( tr ) && tr.DidHitNonWorldEntity() && tr.m_pEnt && tr.m_pEnt->IsPlayer() && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() )
	{
		m_iConsecutivePunches++;
		m_flComboDecayTime = gpGlobals->curtime;

		if ( m_iConsecutivePunches > 2 )
			m_bComboKill = true;
	}
	else
	{
		m_bComboKill = false;
		m_iConsecutivePunches = 0;
	}

	BaseClass::Smack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::PrimaryAttack( void )
{
	// reset the combo if we've already hit 3 times or exceeded the decay time
	if ( gpGlobals->curtime - m_flComboDecayTime > 1.0f || m_iConsecutivePunches > 2  )
	{
		m_iConsecutivePunches = 0;
		m_bComboKill = false;
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::WeaponIdle( void )
{
	if ( m_bComboKill )
	{
		SendWeaponAnim( ACT_ITEM2_VM_IDLE_2 );
		m_bComboKill = false;
	}

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFRobotArm::GetCustomDamageType( void ) const
{
	if ( m_iConsecutivePunches == 3 )
		return TF_DMG_CUSTOM_COMBO_PUNCH;

	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRobotArm::CalcIsAttackCriticalHelper( void )
{
	// punch after 2 consecutive hits always crits
	return ( m_iConsecutivePunches == 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::DoViewModelAnimation( void )
{
	if ( m_iConsecutivePunches == 2 )
		SendWeaponAnim( ACT_ITEM2_VM_SWINGHARD );
	else
		SendWeaponAnim( ACT_ITEM2_VM_HITCENTER );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRobotArm::GetForceScale( void )
{
	if ( m_iConsecutivePunches == 3 )
	{
		return 500.0f;
	}
	
	return BaseClass::GetForceScale();
}
#endif
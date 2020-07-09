//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_obj.h"
#include "ilagcompensationmanager.h"
// Client specific.
#else
#include "c_tf_player.h"
#include "c_baseobject.h"
#endif

#define TF_SPEED_BUFF_DURATION_LEGACY 3.0f	// Values used before mid-2016.
#define TF_SPEED_BUFF_DURATION_MODERN 2.0f	// Values used after mid-2016.

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMelee, DT_TFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTFWeaponBaseMelee, DT_TFWeaponBaseMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseMelee )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_melee, CTFWeaponBaseMelee );

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseMelee )
	DEFINE_THINKFUNC( Smack )
END_DATADESC()
#endif

#ifndef CLIENT_DLL
ConVar tf_meleeattackforcescale( "tf_meleeattackforcescale", "80.0", FCVAR_CHEAT | FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY );
#endif

ConVar tf2v_speed_buff_duration( "tf2v_new_speed_buff_duration", "2.0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Swaps between using old (3s) and new (2s) speed buffing times." );

ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Controls random crits for melee weapons.\n0 - Melee weapons do not randomly crit. \n1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. \n2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting.", true, 0, true, 2 );
extern ConVar tf_weapon_criticals;
extern ConVar tf2v_critchance_melee;
extern ConVar tf2v_use_new_jag;

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBaseMelee::CTFWeaponBaseMelee()
{
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	m_bConnected = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Precache()
{
	BaseClass::Precache();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Spawn()
{
	Precache();

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast< CTFWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in melee weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;
	Assert( m_pWeaponInfo );

	// No ammo.
	m_iClip1 = -1;

	BaseClass::Spawn();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanHolster( void ) const
{
	if ( GetTFPlayerOwner()->m_Shared.InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) || GetTFPlayerOwner()->m_Shared.InCond( TF_COND_BERSERK ) )
		return false;

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;

		GetTFPlayerOwner()->m_Shared.SetNextMeleeCrit( kCritType_None );
	}


		
	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

#ifdef GAME_DLL
	pPlayer->EndClassSpecialSkill();
#endif

	// Swing the weapon.
	Swing( pPlayer );

	m_bCurrentAttackIsMiniCrit = pPlayer->m_Shared.GetNextMeleeCrit() != kCritType_None;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
	CalcIsAttackCritical();
	CalcIsAttackMiniCritical();

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();

	// Set next attack times.
	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );
	
	if ( tf2v_use_new_jag.GetInt() > 0 )
			CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay_jag );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );
	
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}

	m_flSmackTime = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::DoViewModelAnimation( void )
{
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	if ( IsCurrentAttackACritical() )
		act = ACT_VM_SWINGHARD;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseMelee::GetSwingRange( void ) const
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer == nullptr )
		return 48;

	float flSwingRangeMult = 1.0f;
	if ( pPlayer->GetModelScale() > 1.0f )
		flSwingRangeMult *= pPlayer->GetModelScale();

	CALL_ATTRIB_HOOK_FLOAT( flSwingRangeMult, melee_range_multiplier );
	return 48 * flSwingRangeMult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}

	BaseClass::ItemPostFrame();
}

bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &tr )
{
	return DoSwingTraceInternal( tr, false, NULL );
}

bool CTFWeaponBaseMelee::DoSwingTraceInternal( trace_t &trace, bool bCleave, MeleePartitionVector *enumResults )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	float flBoundsMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flBoundsMult, melee_bounds_multiplier );

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();
	CTraceFilterIgnoreTeammates filterFriendlies( this, COLLISION_GROUP_NONE, GetTeamNumber() );

	if ( bCleave )
	{
		Ray_t ray; CBaseEntity *pList[256];
		ray.Init( vecSwingStart, vecSwingEnd, vecSwingMins * flBoundsMult, vecSwingMaxs * flBoundsMult );

		int nCount = UTIL_EntitiesAlongRay( pList, sizeof pList, ray, FL_CLIENT|FL_OBJECT );
		if ( nCount > 0 )
		{
			int nHit = 0;
			for ( int i=0; i < nCount; ++i )
			{
				if ( pList[ i ] == pPlayer )
					continue;

				if ( pList[ i ]->GetTeamNumber() == pPlayer->GetTeamNumber() )
					continue;

				if ( enumResults )
				{
					trace_t trace;
					UTIL_TraceModel( vecSwingStart, vecSwingEnd, 
									 vecSwingMins * flBoundsMult, 
									 vecSwingMaxs * flBoundsMult, 
									 pList[i], COLLISION_GROUP_NONE, &trace );

					enumResults->Element( enumResults->AddToTail() ) = trace;
				}
			}

			return nHit > 0;
		}

		return false;
	}
	else
	{
		// This takes priority over anything else we hit
		int nDamagesSappers = 0;
		CALL_ATTRIB_HOOK_INT( nDamagesSappers, set_dmg_apply_to_sapper );
		if ( nDamagesSappers != 0 )
		{
			CTraceFilterIgnorePlayers filterPlayers( NULL, COLLISION_GROUP_NONE );
			UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filterPlayers, &trace );
			if ( trace.fraction >= 1.0 )
				UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins * flBoundsMult, vecSwingMaxs * flBoundsMult, MASK_SOLID, &filterPlayers, &trace );

			// Ensure valid target
			if ( trace.m_pEnt && trace.m_pEnt->IsBaseObject() )
			{
				CBaseObject *pObject = static_cast<CBaseObject *>( trace.m_pEnt );
				if ( pObject->GetTeamNumber() == pPlayer->GetTeamNumber() && pObject->HasSapper() )
					return ( trace.fraction < 1.0 );
			}
		}

		// See if we hit anything.
		CTraceFilterIgnoreFriendlyCombatItems filterCombatItem( this, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filterCombatItem, &trace );
		if ( trace.fraction >= 1.0 )
		{
			UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins * flBoundsMult, vecSwingMaxs * flBoundsMult, MASK_SOLID, &filterCombatItem, &trace );
			if ( trace.fraction < 1.0 )
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity *pHit = trace.m_pEnt;
				if ( !pHit || pHit->IsBSPModel() )
				{
					// Why duck hull min/max?
					FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
				}

				// This is the point on the actual surface (the hull could have hit space)
				vecSwingEnd = trace.endpos;	
			}
		}

		return ( trace.fraction < 1.0f );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	CUtlVector<trace_t> results;
	trace_t trace;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer == nullptr )
		return;

#if !defined( CLIENT_DLL )
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	int nMeleeCleaves = 0;
	CALL_ATTRIB_HOOK_INT( nMeleeCleaves, melee_cleave_attack );

	// We hit, setup the smack.
	if ( DoSwingTraceInternal( trace, nMeleeCleaves > 0, &results ) )
	{
		// Hit sound - immediate.
		if( trace.m_pEnt->IsPlayer()  )
		{
			WeaponSound( MELEE_HIT );
		}
		else
		{
			WeaponSound( MELEE_HIT_WORLD );
		}
		
		if ( nMeleeCleaves )
		{
			for ( int i=0; i<results.Count(); ++i )
				OnSwingHit( results[i] );
		}
		else
		{
			OnSwingHit( trace );
		}
	}
	else
	{
		int nSelfHarm = 0;
		CALL_ATTRIB_HOOK_INT( nSelfHarm, hit_self_on_miss );
		if ( nSelfHarm != 0 ) // Hit ourselves, dummy!
		{
			// Do Damage.
			DoMeleeDamage( pPlayer, trace );
		}
	}

#if !defined( CLIENT_DLL )
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

void CTFWeaponBaseMelee::DoMeleeDamage( CBaseEntity *pTarget, trace_t &trace )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

#if !defined( CLIENT_DLL )
	// Do Damage.
	int iCustomDamage = GetCustomDamageType();
	
	int nSetDmgType = 0;
	CALL_ATTRIB_HOOK_INT( nSetDmgType, set_dmgtype_ignite );

	
	int iDmgType = DMG_MELEE | DMG_CLUB;
	if ( IsCurrentAttackACrit() )
	{
		// TODO: Not removing the old critical path yet, but the new custom damage is marking criticals as well for melee now.
		iDmgType |= DMG_CRITICAL;
	}

	if ( IsCurrentAttackAMiniCrit() )
	{
		iDmgType |= DMG_MINICRITICAL;
	}
	
	if ( nSetDmgType )
	{
		iDmgType |= DMG_IGNITE;
	}

	float flDamage = GetMeleeDamage( pTarget, iDmgType, iCustomDamage );
	
	CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );

	if ( pTarget == pPlayer )
		info.SetDamageForce( vec3_origin );
	else
		CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * GetForceScale() );

	pTarget->DispatchTraceAttack( info, vecForward, &trace ); 
	ApplyMultiDamage();

	OnEntityHit( pTarget );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, int &iDamageTyoe, int &iCustomDamage )
{
	float flDamage = (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer == nullptr || !pPlayer->IsAlive() )
		return flDamage;

	float flHealthMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flHealthMult, mult_dmg_with_reduced_health );
	if ( flHealthMult != 1.0f )
	{
		float flFraction = pPlayer->HealthFraction();
		flDamage *= RemapValClamped( flFraction, 0.0f, 1.0f, flHealthMult, 1.0f );
	}
	
	return flDamage;
}

void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity )
{
#ifdef GAME_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && TFGameRules()->GetIT() && ToBasePlayer( pEntity ) )
	{
		if ( TFGameRules()->GetIT() == pPlayer )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "tagged_player_as_it" );
			if ( event )
			{
				event->SetInt( "player", pPlayer->GetUserID() );

				gameeventmanager->FireEvent( event );
			}

			UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_ANNOUNCE_TAG", pPlayer->GetPlayerName(), ToBasePlayer( pEntity )->GetPlayerName() );

			CSingleUserRecipientFilter filter( pPlayer );
			CBaseEntity::EmitSound( filter, pPlayer->entindex(), "Player.TaggedOtherIT" );

			TFGameRules()->SetIT( pEntity );
		}
	}
#endif
}

void CTFWeaponBaseMelee::OnSwingHit( trace_t &trace )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer == nullptr )
		return;

#if defined( GAME_DLL )
	// Ally buff calculations.
	int nCanBuffAllies = 0;
	CALL_ATTRIB_HOOK_INT( nCanBuffAllies, speed_buff_ally );

	int nTradeHealthPool = 0;
	CALL_ATTRIB_HOOK_INT( nTradeHealthPool, add_give_health_to_teammate_on_hit );

	// Check to see if they can be buffed.
	CTFPlayer *pTFPlayer = ToTFPlayer( trace.m_pEnt );
	if ( pTFPlayer )
	{
		// We can buff our team, and spies disguised as teammates.
		if ( pTFPlayer->InSameTeam( pPlayer ) || pTFPlayer->m_Shared.GetDisguiseTeam() == pPlayer->GetTeamNumber() )
		{
			if( nCanBuffAllies == 1 )
			{
				const float flBuffDuration = tf2v_speed_buff_duration.GetFloat();
				pTFPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, flBuffDuration );
				// We buff ourselves a little bit longer
				pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, (flBuffDuration * 1.25) );
			}

			if ( nTradeHealthPool != 0 )
			{
				nTradeHealthPool = Min( nTradeHealthPool, pPlayer->GetHealth() - 1 );

				if ( pTFPlayer->TakeHealth( nTradeHealthPool, DMG_GENERIC ) )
				{
					CTakeDamageInfo info( pPlayer, pPlayer, this, nTradeHealthPool, DMG_PREVENT_PHYSICS_FORCE );
					pPlayer->TakeDamage( info );
				}
			}
		}
	}
#endif

	// Do Damage.
	DoMeleeDamage( trace.m_pEnt, trace );

	// Don't impact trace friendly players or objects
	if ( trace.m_pEnt && trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
	#if defined( CLIENT_DLL )
		UTIL_ImpactTrace( &trace, DMG_CLUB );
	#endif
		m_bConnected = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelper( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	int nCvarValue = tf_weapon_criticals_melee.GetInt();

	if ( nCvarValue == 0 )
		return false;

	if ( nCvarValue == 1 && !tf_weapon_criticals.GetBool() )
		return false;

	m_bCurrentAttackIsMiniCrit = pPlayer->m_Shared.GetNextMeleeCrit() != kCritType_None;
	if ( pPlayer->m_Shared.GetNextMeleeCrit() == kCritType_Crit )
		return true;

	float flPlayerCritMult = pPlayer->GetCritMult();

	float flCritChance = ( ( tf2v_critchance_melee.GetFloat() / 100 ) * flPlayerCritMult );
	CALL_ATTRIB_HOOK_FLOAT( flCritChance, mult_crit_chance );

	// If the chance is 0, just bail.
	if ( flCritChance == 0.0f )
		return false;

	return ( RandomInt( 0, WEAPON_RANDOM_RANGE-1 ) <= flCritChance * WEAPON_RANDOM_RANGE );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetForceScale( void )
{
	return tf_meleeattackforcescale.GetFloat();
}
#endif

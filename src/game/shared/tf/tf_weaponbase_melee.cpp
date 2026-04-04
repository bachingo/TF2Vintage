//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "tf_weapon_bat.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"
#include "debugoverlay_shared.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "tf_passtime_logic.h"
// Client specific.
#else
#include "c_tf_gamestats.h"
#include "c_tf_player.h"
// NVNT haptics system interface
#include "haptics/ihaptics.h"
// VR hand support
#include "tfvr/c_tfvr_hand.h"
#endif

ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Controls random crits for melee weapons. 0 - Melee weapons do not randomly crit. 1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. 2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting." );

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

#ifdef _DEBUG
extern ConVar tf_weapon_criticals_force_random;
#endif // _DEBUG

// VR Physical Melee ConVars
ConVar tfvr_melee_range_mult( "tfvr_melee_range_mult", "1.5", FCVAR_REPLICATED, "VR melee range as fraction of base range" );
ConVar tfvr_melee_swing_speed( "tfvr_melee_swing_speed", "150.0", FCVAR_REPLICATED, "Min grip speed (units/sec) to register a VR melee hit" );
ConVar tfvr_melee_hull_width( "tfvr_melee_hull_width", "3.0", FCVAR_REPLICATED, "Half-width of VR melee damage hull trace" );
ConVar tfvr_melee_debug( "tfvr_melee_debug", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Draw VR melee collision debug. 1=hull, 2=hull+velocity" );
ConVar tfvr_melee_bone_axis( "tfvr_melee_bone_axis", "-1", FCVAR_ARCHIVE, "Weapon bone axis for melee direction. -1=auto, 0=X(red), 1=Y(green), 2=Z(blue) in HLMV" );

// Hard minimum time between VR melee hits. Just under the fastest vanilla melee
// fire rate (Eviction Notice w/ double fire rate power up @ 0.24s) to prevent double-hits from controller
// jitter or rapid swing reversals. Per-weapon damage scaling still applies above this.
#define VR_MELEE_MIN_COOLDOWN 0.24f

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
	m_bMiniCrit = false;

	m_flVRGripSpeed = 0.0f;
	m_flVRLastHitTime = 0.0f;
	m_flVRHitDamageMod = 1.0f;
	m_flVRLastHeadAddTime = 0.0f;
	m_bVRSwingActive = false;
	m_bVRSwingHit = false;
	m_bVRSwingActiveLeft = false;
	m_bVRSwingHitLeft = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanHolster( void ) const
{
	// For fist users, energy buffs come from steak sandviches which lock us into attacking with melee.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) )
		return false;

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Precache()
{
	BaseClass::Precache();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		char szMeleeSoundStr[128] = "MVM_";
		const char *shootsound = GetShootSound( MELEE_HIT );
		if ( shootsound && shootsound[0] )
		{
			V_strcat(szMeleeSoundStr, shootsound, sizeof( szMeleeSoundStr ));
			CBaseEntity::PrecacheScriptSound( szMeleeSoundStr );
		}
	}
	CBaseEntity::PrecacheScriptSound("MVM_Weapon_Default.HitFlesh");
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
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	}

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
	
		int iSelfMark = 0;
		CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
		if ( iSelfMark )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

int	CTFWeaponBaseMelee::GetSwingRange( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		return 128;
	}
	else
	{
		int iIsSword = 0;
		CALL_ATTRIB_HOOK_INT( iIsSword, is_a_sword )
		if ( iIsSword )
		{
			return 72; // swords are typically 72
		}
		return 48;
	}
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

	// VR: physical melee replaces button-press attacks for actual melee weapons.
	// Some non-melee items (banners, flags, rocket pack) inherit from this class
	// but should keep their normal PrimaryAttack behavior.
	if ( pPlayer->IsInVRMode() && IsVRPhysicalMeleeWeapon() )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

	pPlayer->EndClassSpecialSkill();

	// Swing the weapon.
	Swing( pPlayer );

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_MINICRIT )
	{
		m_bMiniCrit = true;
	}
	else
	{
		m_bMiniCrit = false;
	}


#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
#endif

	pPlayer->m_Shared.OnAttack();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;


	m_flNextSecondaryAttack = gpGlobals->curtime + GetNextSecondaryAttackDelay(); // default: 0.5f
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PlaySwingSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
	CalcIsAttackCritical();

#ifdef GAME_DLL
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();

#ifdef CLIENT_DLL
	// VR: Trigger swing animation on the VR hand (only for trigger-based melee, not physical)
	if ( m_bHeldByVRHand && !IsOwnerInVR() )
	{
		C_TFVRHand *pRightHand = GetLocalPlayerRightHand();
		if ( pRightHand && pRightHand->GetHeldWeapon() == this )
		{
			pRightHand->PlayWeaponFireAnimation();
		}
	}
#endif

	// Set next attack times.
	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	m_flNextSecondaryAttack = gpGlobals->curtime + flFireDelay;
	pPlayer->m_Shared.SetNextStealthTime( m_flNextSecondaryAttack );

	SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	PlaySwingSound();

#ifdef GAME_DLL
	// Remember if there are potential targets when we start our swing.
	// If there are, the player is exempt from taking "hurt self on miss" damage
	// if ALL of these players have died when our swing has finished, and we didn't hit.
	// This guards against me performing a "good" swing and being punished by a friend
	// killing my target "out from under me".
	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( pPlayer->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	m_potentialVictimVector.RemoveAll();
	const float looseSwingRange = 1.2f * GetSwingRange();

	for( int i=0; i<enemyVector.Count(); ++i )
	{
		Vector toVictim = enemyVector[i]->WorldSpaceCenter() - pPlayer->Weapon_ShootPosition();

		if ( toVictim.IsLengthLessThan( looseSwingRange ) )
		{
			m_potentialVictimVector.AddToTail( enemyVector[i] );
		}
	}
#endif

	m_flSmackTime = GetSmackTime( m_iWeaponMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::DoViewModelAnimation( void )
{
	if ( IsCurrentAttackACrit() )
	{
		if ( SendWeaponAnim( ACT_VM_SWINGHARD ) )
		{
			// check that weapon has the activity
			return;
		}
	}

	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

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

// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPreFrame( void )
{
	int iSelfMark = 0;
	CALL_ATTRIB_HOOK_INT( iSelfMark, self_mark_for_death );
	if ( iSelfMark )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, iSelfMark );
		}
	}

	return BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	// VR: use velocity-based physical melee instead of button+smack.
	// Only for actual melee weapons -- items like banners inherit from this
	// class but shouldn't trigger physical melee.
	if ( IsOwnerInVR() && IsVRPhysicalMeleeWeapon() )
	{
		VRPhysicalMeleeUpdate();
		BaseClass::ItemPostFrame();
		return;
	}

	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		m_flSmackTime = -1.0f;
		Smack();
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
		}
	}

	BaseClass::ItemPostFrame();
}


bool CTFWeaponBaseMelee::DoSwingTraceInternal( trace_t &trace, bool bCleave, CUtlVector< trace_t >* pTargetTraceVector )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMinsBase( -18, -18, -18 );
	static Vector vecSwingMaxsBase( 18, 18, 18 );

	float fBoundsScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( fBoundsScale, melee_bounds_multiplier );
	Vector vecSwingMins = vecSwingMinsBase * fBoundsScale;
	Vector vecSwingMaxs = vecSwingMaxsBase * fBoundsScale;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	float fSwingRange = GetSwingRange();

	// Scale the range and bounds by the model scale if they're larger
	// Not scaling down the range for smaller models because midgets need all the help they can get
	if ( pPlayer->GetModelScale() > 1.0f )
	{
		fSwingRange *= pPlayer->GetModelScale();
		vecSwingMins *= pPlayer->GetModelScale();
		vecSwingMaxs *= pPlayer->GetModelScale();
	}

	CALL_ATTRIB_HOOK_FLOAT( fSwingRange, melee_range_multiplier );

	Vector vecForward; 
	AngleVectors( pPlayer->Weapon_ShootAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * fSwingRange;

	// In MvM, melee hits from the robot team wont hit teammates to ensure mobs of melee bots don't 
	// swarm so tightly they hit each other and no-one else
	bool bDontHitTeammates = pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS && TFGameRules()->IsMannVsMachineMode();
	CTraceFilterIgnoreTeammates ignoreTeammatesFilter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );

	if ( bCleave )
	{
		Ray_t ray;
		ray.Init( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs );
		CBaseEntity *pList[256];
		int nTargetCount = UTIL_EntitiesAlongRay( pList, ARRAYSIZE( pList ), ray, FL_CLIENT|FL_OBJECT );
		
		int nHitCount = 0;
		for ( int i=0; i<nTargetCount; ++i )
		{
			CBaseEntity *pTarget = pList[i];
			if ( pTarget == pPlayer )
			{
				// don't hit yourself
				continue;
			}

			if ( bDontHitTeammates && pTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
			{
				// don't hit teammate
				continue;
			}

			if ( pTargetTraceVector )
			{
				trace_t tr;
				UTIL_TraceModel( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, pTarget, COLLISION_GROUP_NONE, &tr );
				pTargetTraceVector->AddToTail();
				pTargetTraceVector->Tail() = tr;
			}
			nHitCount++;
		}

		return nHitCount > 0;
	}
	else
	{
		bool bSapperHit = false;

		// if this weapon can damage sappers, do that trace first
		int iDmgSappers = 0;
		CALL_ATTRIB_HOOK_INT( iDmgSappers, set_dmg_apply_to_sapper );
		if ( iDmgSappers != 0 )
		{
			CTraceFilterIgnorePlayers ignorePlayersFilter( NULL, COLLISION_GROUP_NONE );
			UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &ignorePlayersFilter, &trace );
			if ( trace.fraction >= 1.0 )
			{
				UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &ignorePlayersFilter, &trace );
			}

			if ( trace.fraction < 1.0f &&
				 trace.m_pEnt &&
				 trace.m_pEnt->IsBaseObject() &&
				 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
			{
				CBaseObject *pObject = static_cast< CBaseObject* >( trace.m_pEnt );
				if ( pObject->HasSapper() )
				{
					bSapperHit = true;
				}
			}
		}

		if ( !bSapperHit )
		{
			// See if we hit anything.
			if ( bDontHitTeammates )
			{
				UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &ignoreTeammatesFilter, &trace );
			}
			else
			{
				CTraceFilterIgnoreFriendlyCombatItems filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
				UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filter, &trace );
			}

			if ( trace.fraction >= 1.0 )
			{
				if ( bDontHitTeammates )
				{
					UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &ignoreTeammatesFilter, &trace );
				}
				else
				{
					CTraceFilterIgnoreFriendlyCombatItems filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
					UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &filter, &trace );
				}

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
		}

		return ( trace.fraction < 1.0f );
	}
}


bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &trace )
{
	return DoSwingTraceInternal( trace, false, NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::OnSwingHit( trace_t &trace, float flDamageMod )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// NVNT if this is the client dll and the owner is the local player
	//	Notify the haptics system the local player just hit something.
#ifdef CLIENT_DLL
	if(pPlayer==C_TFPlayer::GetLocalTFPlayer() && haptics)
		haptics->ProcessHapticEvent(2,"Weapons","meleehit");
#endif

	bool bHitEnemyPlayer = false;

	// Hit sound - immediate.
	if( trace.m_pEnt->IsPlayer() )
	{
		CTFPlayer *pTargetPlayer = ToTFPlayer( trace.m_pEnt );

		bool bPlayMvMHitOnly = false;
		// handle hitting a robot	
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( pTargetPlayer  && pTargetPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS && !pTargetPlayer->IsPlayer() )
			{
				bPlayMvMHitOnly = true;

				CBroadcastRecipientFilter filter;
				// 					CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );
				// 					if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
				// 					{
				// 						filter.UsePredictionRules();
				// 					}

				char szMeleeSoundStr[128] = "MVM_";
				const char *shootsound = GetShootSound( MELEE_HIT );
				if ( shootsound && shootsound[0] )
				{
					V_strcat(szMeleeSoundStr, shootsound, sizeof( szMeleeSoundStr ));
					CSoundParameters params;
					if ( CBaseEntity::GetParametersForSound( szMeleeSoundStr, params, NULL ) )
					{
						EmitSound( filter, GetOwner()->entindex(), szMeleeSoundStr, NULL );
					}
					else
					{
						EmitSound( filter, GetOwner()->entindex(), "MVM_Weapon_Default.HitFlesh", NULL );
					}
				}
				else
				{
					EmitSound( filter, GetOwner()->entindex(), "MVM_Weapon_Default.HitFlesh", NULL );
				}
			}
		} 
		if(! bPlayMvMHitOnly )
		{
			WeaponSound( MELEE_HIT );
		}

#if !defined (CLIENT_DLL)

		if ( pTargetPlayer->m_Shared.HasPasstimeBall() && g_pPasstimeLogic ) 
		{
			// This handles stealing the ball from teammates since there's no damage involved
			// TODO find a better place for this
			g_pPasstimeLogic->OnBallCarrierMeleeHit( pTargetPlayer, pPlayer );
		}

		if ( pPlayer->GetTeamNumber() != pTargetPlayer->GetTeamNumber() )
		{
			bHitEnemyPlayer = true;

			if ( TFGameRules()->IsIT( pPlayer ) )
			{
				IGameEvent *pEvent = gameeventmanager->CreateEvent( "tagged_player_as_it" );
				if ( pEvent )
				{
					pEvent->SetInt( "player", pPlayer->GetUserID() );
					gameeventmanager->FireEvent( pEvent, true );
				}

				// Tag! You're IT!
				TFGameRules()->SetIT( pTargetPlayer );

				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_YES );

				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_ANNOUNCE_TAG", pPlayer->GetPlayerName(), pTargetPlayer->GetPlayerName() );

				CSingleUserReliableRecipientFilter filter( pPlayer );
				pPlayer->EmitSound( filter, pPlayer->entindex(), "Player.TaggedOtherIT" );
			}
		}

		if ( pTargetPlayer->InSameTeam( pPlayer ) || pTargetPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() )
		{
			int iSpeedBuffOnHit = 0;
			CALL_ATTRIB_HOOK_INT( iSpeedBuffOnHit, speed_buff_ally );
			if ( iSpeedBuffOnHit > 0 && trace.m_pEnt )
			{
				pTargetPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 2.f );
				pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 3.6f );		// give the soldier a bit of additional time to allow them to keep up better with faster classes

				EconEntity_OnOwnerKillEaterEvent( this, pPlayer, pTargetPlayer, kKillEaterEvent_TeammatesWhipped );	// Strange
			}

			// Give health to teammates on hit
			int nGiveHealthOnHit = 0;
			CALL_ATTRIB_HOOK_INT( nGiveHealthOnHit, add_give_health_to_teammate_on_hit );
			if ( nGiveHealthOnHit != 0 )
			{
				// Always keep at least 1 health for ourselves
				nGiveHealthOnHit = Min( pPlayer->GetHealth() - 1, nGiveHealthOnHit );
				int nHealthGiven = pTargetPlayer->TakeHealth( nGiveHealthOnHit, DMG_GENERIC );

				if ( nHealthGiven > 0 )
				{
					// Subtract health given from my own
					CTakeDamageInfo info( pPlayer, pPlayer, this, nHealthGiven, DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
					pPlayer->TakeDamage( info );
				}
			}
		}
		else
		{
			float flSpeedBoostOnHitEnemy = 0.f;
			CALL_ATTRIB_HOOK_FLOAT( flSpeedBoostOnHitEnemy, speed_boost_on_hit_enemy );
			if ( flSpeedBoostOnHitEnemy > 0 && trace.m_pEnt )
			{
				pPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, flSpeedBoostOnHitEnemy );
			}
		}
#endif
	}
	else
	{
		WeaponSound( MELEE_HIT_WORLD );
	}

	DoMeleeDamage( trace.m_pEnt, trace, flDamageMod );

	return bHitEnemyPlayer;
}


// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	trace_t trace;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	bool bHitEnemyPlayer = false;

	int nCleaveAttack = 0;
	CALL_ATTRIB_HOOK_INT( nCleaveAttack, melee_cleave_attack );
	bool bCleave = nCleaveAttack > 0;

	// We hit, setup the smack.
	CUtlVector<trace_t> targetTraceVector;
	if ( DoSwingTraceInternal( trace, bCleave, &targetTraceVector ) )
	{
		if ( bCleave )
		{
			for ( int i=0; i<targetTraceVector.Count(); ++i )
			{
				bHitEnemyPlayer |= OnSwingHit( targetTraceVector[i] );
			}
		}
		else
		{
			bHitEnemyPlayer = OnSwingHit( trace );
		}
	}
	else
	{
		// if ALL of my potential targets have been killed by someone else between the 
		// time I started my swing and the time my swing would have landed, don't
		// punish me for it.
		bool bIsCleanMiss = true;

#ifdef GAME_DLL
		for( int i=0; i<m_potentialVictimVector.Count(); ++i )
		{
			if ( m_potentialVictimVector[i] != NULL && m_potentialVictimVector[i]->IsAlive() )
			{
				bIsCleanMiss = false;
				break;
			}
		}
#endif

		if ( bIsCleanMiss )
		{
			int iHitSelf = 0;
			CALL_ATTRIB_HOOK_INT( iHitSelf, hit_self_on_miss );
			if ( iHitSelf == 1 )
			{
				DoMeleeDamage( GetTFPlayerOwner(), trace, 0.5f );
			}
		}
	}

#if !defined (CLIENT_DLL)

	// ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES
	if ( GetWeaponID() == TF_WEAPON_BONESAW )
	{
		int iCount = pPlayer->GetPerLifeCounterKV( "medic_bonesaw_hits" );

		if ( bHitEnemyPlayer )
		{
			if ( ++iCount >= 5 )
			{
				pPlayer->AwardAchievement( ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES );
			}
		}
		else
		{
			iCount = 0;
		}

		pPlayer->SetPerLifeCounterKV( "medic_bonesaw_hits", iCount );
	}

	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

float CTFWeaponBaseMelee::GetSmackTime( int iWeaponMode )
{
	return gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( iWeaponMode ).m_flSmackDelay;
}

void CTFWeaponBaseMelee::DoMeleeDamage( CBaseEntity* ent, trace_t& trace )
{
	DoMeleeDamage( ent, trace, 1.f );
}

void CTFWeaponBaseMelee::DoMeleeDamage( CBaseEntity* ent, trace_t& trace, float flDamageMod )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	Vector vecForward; 
	AngleVectors( pPlayer->Weapon_ShootAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 48;

#ifndef CLIENT_DLL
	// Do Damage.
	int iCustomDamage = GetDamageCustom();
	int iDmgType = DMG_MELEE | DMG_NEVERGIB | DMG_CLUB;

	int iCritFromBehind = 0;
	CALL_ATTRIB_HOOK_INT( iCritFromBehind, crit_from_behind );
	if ( iCritFromBehind > 0 )
	{
		Vector entForward; 
		AngleVectors( ent->EyeAngles(), &entForward );

		Vector toEnt = ent->GetAbsOrigin() - pPlayer->GetAbsOrigin();
		toEnt.NormalizeInPlace();

		if ( DotProduct( toEnt, entForward ) > 0.7071f )
		{
			iDmgType |= DMG_CRITICAL;
		}
	}

	float flDamage = GetMeleeDamage( ent, &iDmgType, &iCustomDamage ) * flDamageMod;

	// Base melee damage increased because we disallow random crits in this mode. Without random crits, melee is underpowered
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		if ( !IsCurrentAttackACrit() ) // Don't multiply base damage if attack is a crit
		{
			if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
			{
				flDamage *= ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) ? 1.4f : 1.9f );
			}
			// Strength powerup multiplies damage later and we only want double regular damage
			// Shields are a source of increased melee damage (charge crit) so they don't need a base boost
			else if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() != RUNE_STRENGTH && !pPlayer->m_Shared.IsShieldEquipped() )
			{
				flDamage *= 1.3f;
			}
		}
	}

	if ( IsCurrentAttackACrit() )
	{
		// TODO: Not removing the old critical path yet, but the new custom damage is marking criticals as well for melee now.
		iDmgType |= DMG_CRITICAL;
	}
	else if ( m_bMiniCrit )
	{
		iDmgType |= DMG_RADIUS_MAX; // Unused for melee, indicates this should be a minicrit.
	}

	CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );

	if ( fabs( flDamage ) >= 1.0f )
	{
		CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * GetForceScale() );
	}
	else
	{
		info.SetDamageForce( vec3_origin );
	}
	
	ent->DispatchTraceAttack( info, vecForward, &trace ); 
	ApplyMultiDamage();

	OnEntityHit( ent, &info );

	bool bTruce = TFGameRules() && TFGameRules()->IsTruceActive() && pPlayer->IsTruceValidForEnt();
	if ( !bTruce )
	{
		int iCritsForceVictimToLaugh = 0;
		CALL_ATTRIB_HOOK_INT( iCritsForceVictimToLaugh, crit_forces_victim_to_laugh );
		if ( iCritsForceVictimToLaugh > 0 && ( IsCurrentAttackACrit() || iDmgType & DMG_CRITICAL ) )
		{
			CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

			if ( pVictimPlayer && pVictimPlayer->CanBeForcedToLaugh() && ( pPlayer->GetTeamNumber() != pVictimPlayer->GetTeamNumber() ) )
			{
				// force victim to laugh!
				pVictimPlayer->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );

				// strange stat tracking
				EconEntity_OnOwnerKillEaterEvent( this,
												  ToTFPlayer( GetOwner() ),
												  pVictimPlayer,
												  kKillEaterEvent_PlayerTickle );
			}
		}

		int iTickleEnemiesWieldingSameWeapon = 0;
		CALL_ATTRIB_HOOK_INT( iTickleEnemiesWieldingSameWeapon, tickle_enemies_wielding_same_weapon );
		if ( iTickleEnemiesWieldingSameWeapon > 0 )
		{
			CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

			if ( pVictimPlayer && pVictimPlayer->CanBeForcedToLaugh() && ( pPlayer->GetTeamNumber() != pVictimPlayer->GetTeamNumber() ) )
			{
				CTFWeaponBase *myWeapon = pPlayer->GetActiveTFWeapon();
				CTFWeaponBase *theirWeapon = pVictimPlayer->GetActiveTFWeapon();

				if ( myWeapon && theirWeapon )
				{
					CEconItemView *myItem = myWeapon->GetAttributeContainer()->GetItem();
					CEconItemView *theirItem = theirWeapon->GetAttributeContainer()->GetItem();

					if ( myItem && theirItem && myItem->GetItemDefIndex() == theirItem->GetItemDefIndex() )
					{
						// force victim to laugh!
						pVictimPlayer->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );
					}
				}
			}
		}
	}
	if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
	{
		CTFPlayer *pVictimPlayer = ToTFPlayer( ent );

		if ( pVictimPlayer && !pVictimPlayer->InSameTeam( pPlayer ) )
		{
			CPASAttenuationFilter filter( pPlayer );
			Vector origin = pPlayer->GetAbsOrigin();
			Vector vecDir = pVictimPlayer->GetAbsOrigin() - origin;
			VectorNormalize( vecDir );
				
			if ( !pVictimPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) &&
				!pVictimPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
			{
				if ( pVictimPlayer->m_Shared.IsCarryingRune() ) 
				{
					pVictimPlayer->DropRune();
					ClientPrint( pVictimPlayer, HUD_PRINTCENTER, "#TF_Powerup_Knocked_Out" );
				}
				else if ( pVictimPlayer->HasTheFlag() )	
				{
					pVictimPlayer->DropFlag();
					ClientPrint( pVictimPlayer, HUD_PRINTCENTER, "#TF_CTF_PlayerDrop" );
				}
			}
			EmitSound( filter, entindex(), "Powerup.Knockout_Melee_Hit" );
			pVictimPlayer->ApplyGenericPushbackImpulse( vecDir * 400.0f, pPlayer );
		}
	}

#endif
	// Don't impact trace friendly players or objects
	if ( ent && ent->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
#ifdef CLIENT_DLL
		UTIL_ImpactTrace( &trace, DMG_CLUB );
#endif
		m_bConnected = true;
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetForceScale( void )
{
	return tf_meleeattackforcescale.GetFloat();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage )
{
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

	int iCritDoesNoDamage = 0;
	CALL_ATTRIB_HOOK_INT( iCritDoesNoDamage, crit_does_no_damage );
	if ( iCritDoesNoDamage > 0 )
	{
		if ( IsCurrentAttackACrit() )
		{
			return 0.0f;	
		}

		if ( piDamageType && *piDamageType & DMG_CRITICAL )
		{
			return 0.0f;
		}
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		float flHalfHealth = pPlayer->GetMaxHealth() * 0.5f;
		if ( pPlayer->GetHealth() < flHalfHealth )
		{
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_bonus_while_half_dead );
		}
		else
		{
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg_penalty_while_half_alive );
		}

		// Some weapons change damage based on player's health
		float flReducedHealthBonus = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flReducedHealthBonus, mult_dmg_with_reduced_health );
		if ( flReducedHealthBonus != 1.0f )
		{
			float flHealthFraction = clamp( pPlayer->HealthFraction(), 0.0f, 1.0f );
			flReducedHealthBonus = Lerp( flHealthFraction, flReducedHealthBonus, 1.0f );

			flDamage *= flReducedHealthBonus;
		}
	}

	return flDamage;
}

void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info )
{
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelperNoCrits( void )
{
	// This function was called because the tf_weapon_criticals ConVar is off, but if
	// melee crits are set to be forced on, then call the regular crit helper function.
	if ( tf_weapon_criticals_melee.GetInt() > 1 )
	{
		return CalcIsAttackCriticalHelper();
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_CRIT )
	{
		return true;
	}
	else
	{
		return BaseClass::CalcIsAttackCriticalHelperNoCrits();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelper( void )
{
	// If melee crits are off, then check the NoCrits helper.
	if ( tf_weapon_criticals_melee.GetInt() == 0 )
	{
		return CalcIsAttackCriticalHelperNoCrits();
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	if ( !CanFireCriticalShot() )
		return false;

	// Crit boosted players fire all crits
	if ( pPlayer->m_Shared.IsCritBoosted() )
		return true;

	float flPlayerCritMult = pPlayer->GetCritMult();
	float flCritChance = TF_DAMAGE_CRIT_CHANCE_MELEE * flPlayerCritMult;
	CALL_ATTRIB_HOOK_FLOAT( flCritChance, mult_crit_chance );

	// mess with the crit chance seed so it's not based solely on the prediction seed
	int iMask = ( entindex() << 16 ) | ( pPlayer->entindex() << 8 );
	int iSeed = CBaseEntity::GetPredictionRandomSeed() ^ iMask;
	if ( iSeed != m_iCurrentSeed )
	{
		m_iCurrentSeed = iSeed;
		RandomSeed( m_iCurrentSeed );
	}

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_CRIT )
	{
		return true;
	}

	// Regulate crit frequency to reduce client-side seed hacking
	float flDamage = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );
	AddToCritBucket( flDamage );

	// Track each request
	m_nCritChecks++;

	bool bCrit = ( RandomInt( 0, WEAPON_RANDOM_RANGE-1 ) < ( flCritChance ) * WEAPON_RANDOM_RANGE );

#ifdef _DEBUG
	// Force seed to always say yes
	if ( tf_weapon_criticals_force_random.GetInt() )
	{
		bCrit = true;
	}
#endif // _DEBUG

	if ( bCrit )
	{
		// Seed says crit.  Run it by the manager.
		bCrit = IsAllowedToWithdrawFromCritBucket( flDamage );
	}

	return bCrit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFWeaponBaseMelee::GetShootSound( int iIndex ) const
{
	// Custom Melee weapons may override their hit effects
	if ( iIndex == MELEE_HIT )
	{
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			const char *pszSound = pItem->GetStaticData()->GetCustomSound( GetTeamNumber(), 1 );
			if ( pszSound )
				return pszSound;
		}
	}

	return BaseClass::GetShootSound(iIndex);
}

//=============================================================================
//
// VR Physical Melee System
//
//=============================================================================

//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::IsVRPhysicalMeleeWeapon()
{
	int id = GetWeaponID();
	return ( id != TF_WEAPON_BUFF_ITEM &&
			 id != TF_WEAPON_ROCKETPACK );
}

//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::IsOwnerInVR()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	return ( pPlayer && pPlayer->IsInVRMode() );
}

//-----------------------------------------------------------------------------
// Weapon bone world transform from the usercmd (single source of truth for
// both client prediction and server simulation). The client packs weapon_bone
// position + Y axis direction into rightControllerOrigin/Angles.
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::GetVRWeaponBoneTransform( Vector &outPos, QAngle &outAng )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	const CUserCmd *pCmd = pPlayer->GetCurrentUserCommand();
	if ( !pCmd || pCmd->rightControllerOrigin == vec3_origin )
		return false;

	outPos = pCmd->rightControllerOrigin;
	outAng = pCmd->rightControllerAngles;
	return true;
}

bool CTFWeaponBaseMelee::GetVRWeaponBoneTransformLeft( Vector &outPos, QAngle &outAng )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	const CUserCmd *pCmd = pPlayer->GetCurrentUserCommand();
	if ( !pCmd || pCmd->leftControllerOrigin == vec3_origin )
		return false;

	outPos = pCmd->leftControllerOrigin;
	outAng = pCmd->leftControllerAngles;
	return true;
}

//-----------------------------------------------------------------------------
// Dynamic VR melee range: total reach (arm extension + trace) approximates
// base melee range * multiplier. When the hand is extended, the trace
// shortens; when tucked in, it lengthens. A minimum trace length is enforced
// so there's always a hitbox beyond the weapon model.
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetVRSwingRange( const Vector *pGripPos )
{
	float flMaxRange = GetSwingRange() * tfvr_melee_range_mult.GetFloat();

	if ( !pGripPos )
		return flMaxRange;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return flMaxRange;

	// Use the usercmd's eye position so both grip and body center come from the
	// same client frame. pPlayer->EyePosition() is evaluated at prediction tick
	// time and drifts relative to the usercmd grip position during locomotion.
	const CUserCmd *pCmd = pPlayer->GetCurrentUserCommand();
	Vector vecBody;
	if ( pCmd && pCmd->clientEyePosition != vec3_origin )
		vecBody = pCmd->clientEyePosition;
	else
		vecBody = pPlayer->EyePosition();

	Vector vecToGrip = *pGripPos - vecBody;
	vecToGrip.z = 0.0f;
	float flArmExtension = vecToGrip.Length();

	float flMinTrace = flMaxRange * 0.3f;
	float flRange = flMaxRange - flArmExtension;
	return MAX( flRange, flMinTrace );
}

//-----------------------------------------------------------------------------
// Proportional damage when hitting faster than the weapon's fire rate.
// Keeps DPS capped at base TF2 levels regardless of swing speed.
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::CalcVRCooldownDamageMod()
{
	if ( m_flVRLastHitTime <= 0.0f )
		return 1.0f;

	float flElapsed = gpGlobals->curtime - m_flVRLastHitTime;
	float flAttackInterval = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	flAttackInterval = ApplyFireDelay( flAttackInterval );

	if ( flAttackInterval <= 0.0f )
		return 1.0f;

	if ( flElapsed >= flAttackInterval )
		return 1.0f;

	return flElapsed / flAttackInterval;
}

//-----------------------------------------------------------------------------
// Returns false if the weapon's attack interval hasn't elapsed since the
// last head/organ was granted.  Non-VR players are never gated.
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CanVRAddHead()
{
	if ( !IsOwnerInVR() )
		return true;

	if ( m_flVRLastHeadAddTime <= 0.0f )
		return true;

	float flElapsed = gpGlobals->curtime - m_flVRLastHeadAddTime;
	float flAttackInterval = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	flAttackInterval = ApplyFireDelay( flAttackInterval );

	return ( flAttackInterval <= 0.0f || flElapsed >= flAttackInterval );
}

//-----------------------------------------------------------------------------
// Called when a VR swing ends without registering a hit.
// Applies self-damage for weapons with the hit_self_on_miss attribute
// (e.g. Boston Basher), matching vanilla Smack() miss behavior.
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::OnVRSwingMiss()
{
#if !defined( CLIENT_DLL )
	int iHitSelf = 0;
	CALL_ATTRIB_HOOK_INT( iHitSelf, hit_self_on_miss );
	if ( iHitSelf != 1 )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	trace_t trace;
	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = pPlayer->WorldSpaceCenter();

	DoMeleeDamage( pPlayer, trace, 0.5f );
#endif
}

//-----------------------------------------------------------------------------
// Shared hull trace from an arbitrary grip position/orientation.
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::DoVRSwingTraceFromHand( trace_t &trace, const Vector &vecStart, const QAngle &angBone )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	float fHullHalf = tfvr_melee_hull_width.GetFloat();
	float fSwingRange = GetVRSwingRange( &vecStart );

	Vector vecWeaponFwd;
	AngleVectors( angBone, &vecWeaponFwd );
	Vector vecEnd = vecStart + vecWeaponFwd * fSwingRange;

	Vector vecMins( -fHullHalf, -fHullHalf, -fHullHalf );
	Vector vecMaxs( fHullHalf, fHullHalf, fHullHalf );

	CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );

	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, &filter, &trace );

	if ( trace.fraction >= 1.0f )
	{
		UTIL_TraceHull( vecStart, vecEnd, vecMins, vecMaxs, MASK_SOLID, &filter, &trace );
	}

	return ( trace.fraction < 1.0f && trace.m_pEnt );
}

//-----------------------------------------------------------------------------
// Hull trace from right-hand grip along weapon axis for VR melee collision.
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::DoVRSwingTrace( trace_t &trace )
{
	Vector vecStart;
	QAngle angBone;
	if ( !GetVRWeaponBoneTransform( vecStart, angBone ) )
		return false;

	return DoVRSwingTraceFromHand( trace, vecStart, angBone );
}

//-----------------------------------------------------------------------------
// Per-frame VR melee update. Grip speed is pre-computed on the client in
// tracking space and delivered via usercmd, so both sides always agree and
// player locomotion is inherently excluded from the velocity measurement.
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::VRPhysicalMeleeUpdate()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	const CUserCmd *pCmd = pPlayer->GetCurrentUserCommand();
	if ( !pCmd )
		return;

	// Block melee while cloaked or uncloaking to preserve game balance.
	// IsStealthed covers the cloaked state; the visibility check also
	// blocks during the fade-in transition that vanilla prevents via
	// the normal attack lockout.
	if ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.GetPercentInvisible() > 0.0f )
		return;

	if ( IsVRMeleeBlocked() )
		return;

	bool bIsFists = ( GetWeaponID() == TF_WEAPON_FISTS );

	float flRightGripSpeed = pCmd->vrMeleeGripSpeed;
	float flLeftGripSpeed  = bIsFists ? pCmd->vrMeleeGripSpeedLeft : 0.0f;
	m_flVRGripSpeed = MAX( flRightGripSpeed, flLeftGripSpeed );

	float flThreshold = tfvr_melee_swing_speed.GetFloat();
	float flResetThreshold = flThreshold * 0.25f;

	// --- Right-hand swing state ---
	if ( m_bVRSwingActive )
	{
		if ( flRightGripSpeed < flResetThreshold )
		{
			if ( !m_bVRSwingHit )
				OnVRSwingMiss();
			m_bVRSwingActive = false;
			m_bVRSwingHit = false;
		}
	}
	else
	{
		if ( flRightGripSpeed >= flThreshold )
		{
			// VR ball aim: a swing while the offhand trigger is held launches
			// the ball from the offhand aim point instead of doing a melee hit.
			if ( pCmd->vrBallAimActive )
			{
				CTFBat_Wood *pBatWood = dynamic_cast<CTFBat_Wood *>( this );
				if ( pBatWood )
				{
#if !defined( CLIENT_DLL )
					pBatWood->VRBallAimLaunch();
#else
					WeaponSound( SPECIAL2 );
#endif
					m_bVRSwingActive = true;
					m_bVRSwingHit = true;
					return;
				}
			}

			m_bVRSwingActive = true;
			m_bVRSwingHit = false;
			OnVRSwingStart();
			WeaponSound( MELEE_MISS );
		}
	}

	// --- Left-hand swing state (fists only) ---
	if ( bIsFists )
	{
		if ( m_bVRSwingActiveLeft )
		{
			if ( flLeftGripSpeed < flResetThreshold )
			{
				if ( !m_bVRSwingHitLeft )
					OnVRSwingMiss();
				m_bVRSwingActiveLeft = false;
				m_bVRSwingHitLeft = false;
			}
		}
		else
		{
			if ( flLeftGripSpeed >= flThreshold )
			{
				m_bVRSwingActiveLeft = true;
				m_bVRSwingHitLeft = false;
				WeaponSound( MELEE_MISS );
			}
		}
	}

	// Debug visualization
	if ( tfvr_melee_debug.GetInt() > 0 )
	{
		float fHullHalf = tfvr_melee_hull_width.GetFloat();
		Vector vecMins( -fHullHalf, -fHullHalf, -fHullHalf );
		Vector vecMaxs( fHullHalf, fHullHalf, fHullHalf );

		// Right hand debug
		{
			int r, g, b, a;
			if ( m_bVRSwingHit )             { r = 64;  g = 64;  b = 255; a = 60; }
			else if ( m_bVRSwingActive )      { r = 255; g = 255; b = 0;   a = 80; }
			else                              { r = 128; g = 128; b = 128; a = 40; }

			Vector vecBonePos;
			QAngle angBone;
			if ( GetVRWeaponBoneTransform( vecBonePos, angBone ) )
			{
				float fRange = GetVRSwingRange( &vecBonePos );
				Vector vecFwd;
				AngleVectors( angBone, &vecFwd );
				Vector vecEnd = vecBonePos + vecFwd * fRange;

				NDebugOverlay::SweptBox( vecBonePos, vecEnd, vecMins, vecMaxs, angBone, r, g, b, a, 0.0f );
				NDebugOverlay::Line( vecBonePos, vecEnd, r, g, b, false, 0.0f );
				NDebugOverlay::Cross3D( vecBonePos, 2.0f, 0, 255, 0, false, 0.0f );
				NDebugOverlay::Cross3D( vecEnd, 2.0f, 255, 0, 0, false, 0.0f );

				if ( tfvr_melee_debug.GetInt() >= 2 )
				{
					char sz[64];
					V_snprintf( sz, sizeof(sz), "R: %.0f / %.0f%s",
						flRightGripSpeed, flThreshold,
						m_bVRSwingHit ? " [HIT]" : ( m_bVRSwingActive ? " [SWING]" : "" ) );
					NDebugOverlay::EntityTextAtPosition( vecEnd, 0, sz, 0.0f, r, g, b, 255 );
				}
			}
		}

		// Left hand debug (fists only)
		if ( bIsFists )
		{
			int r, g, b, a;
			if ( m_bVRSwingHitLeft )          { r = 64;  g = 64;  b = 255; a = 60; }
			else if ( m_bVRSwingActiveLeft )   { r = 255; g = 255; b = 0;   a = 80; }
			else                               { r = 128; g = 128; b = 128; a = 40; }

			Vector vecLeftPos;
			QAngle angLeft;
			if ( GetVRWeaponBoneTransformLeft( vecLeftPos, angLeft ) )
			{
				float fRangeL = GetVRSwingRange( &vecLeftPos );
				Vector vecLeftFwd;
				AngleVectors( angLeft, &vecLeftFwd );
				Vector vecEndL = vecLeftPos + vecLeftFwd * fRangeL;

				NDebugOverlay::SweptBox( vecLeftPos, vecEndL, vecMins, vecMaxs, angLeft, r, g, b, a, 0.0f );
				NDebugOverlay::Line( vecLeftPos, vecEndL, r, g, b, false, 0.0f );
				NDebugOverlay::Cross3D( vecLeftPos, 2.0f, 0, 255, 0, false, 0.0f );
				NDebugOverlay::Cross3D( vecEndL, 2.0f, 255, 0, 0, false, 0.0f );

				if ( tfvr_melee_debug.GetInt() >= 2 )
				{
					char sz[64];
					V_snprintf( sz, sizeof(sz), "L: %.0f / %.0f%s",
						flLeftGripSpeed, flThreshold,
						m_bVRSwingHitLeft ? " [HIT]" : ( m_bVRSwingActiveLeft ? " [SWING]" : "" ) );
					NDebugOverlay::EntityTextAtPosition( vecEndL, 0, sz, 0.0f, r, g, b, 255 );
				}
			}
		}
	}

	// Hard cooldown: suppress hit detection entirely until the minimum
	// interval has elapsed since the last registered hit.
	bool bOnCooldown = ( m_flVRLastHitTime > 0.0f &&
		( gpGlobals->curtime - m_flVRLastHitTime ) < VR_MELEE_MIN_COOLDOWN );

	bool bRightCanHit = m_bVRSwingActive && !m_bVRSwingHit && !bOnCooldown;
	bool bLeftCanHit  = bIsFists && m_bVRSwingActiveLeft && !m_bVRSwingHitLeft && !bOnCooldown;

	if ( !bRightCanHit && !bLeftCanHit )
		return;

#if !defined( CLIENT_DLL )
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	trace_t trace;
	bool bHit = false;
	bool bHitFromLeft = false;

	if ( bRightCanHit )
	{
		bHit = DoVRSwingTrace( trace );
	}

	if ( !bHit && bLeftCanHit )
	{
		Vector vecLeftStart;
		QAngle angLeftBone;
		if ( GetVRWeaponBoneTransformLeft( vecLeftStart, angLeftBone ) )
		{
			bHit = DoVRSwingTraceFromHand( trace, vecLeftStart, angLeftBone );
			if ( bHit )
				bHitFromLeft = true;
		}
	}

	if ( bHit )
	{
		if ( bHitFromLeft )
			m_bVRSwingHitLeft = true;
		else
			m_bVRSwingHit = true;

		float flDamageMod = CalcVRCooldownDamageMod();
		m_flVRLastHitTime = gpGlobals->curtime;
		m_flVRHitDamageMod = flDamageMod;

		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
		m_bConnected = true;

		OnVRPreMeleeHit( trace );

		CalcIsAttackCritical();

		m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;
		m_bMiniCrit = ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_MINICRIT );

		SendPlayerAnimEvent( pPlayer );

#if !defined( CLIENT_DLL )
		pPlayer->SpeakWeaponFire();
		CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

		if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
		{
			pPlayer->RemoveInvisibility();
		}
#else
		C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

		pPlayer->m_Shared.OnAttack();

		if ( !HandleVRBuildingHit( trace, flDamageMod ) )
		{
			OnSwingHit( trace, flDamageMod );
		}

		OnVRPostMeleeHit( trace );
		m_flVRHitDamageMod = 1.0f;

		if ( tfvr_melee_debug.GetInt() > 0 )
		{
			NDebugOverlay::Cross3D( trace.endpos, 8.0f, 255, 0, 0, false, 0.5f );
			if ( tfvr_melee_debug.GetInt() >= 2 )
			{
				char szHit[64];
				V_snprintf( szHit, sizeof(szHit), "HIT dmg:%.0f%%", flDamageMod * 100.0f );
				NDebugOverlay::EntityTextAtPosition( trace.endpos, 1, szHit, 0.5f, 255, 64, 64, 255 );
			}
		}

		pPlayer->m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
	}

#if !defined( CLIENT_DLL )
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

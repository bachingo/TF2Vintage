//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_grapplinghook.h"
#include "decals.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_team.h"
#include "soundent.h"
#include "tf_gamestats.h"
#endif

ConVar	tf_grapplinghook_max_distance( "tf_grapplinghook_max_distance", "2000", FCVAR_REPLICATED | FCVAR_CHEAT, "Valid distance for grappling hook to travel" );
ConVar	tf_grapplinghook_fire_delay( "tf_grapplinghook_fire_delay", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar	tf_grapplinghook_projectile_speed( "tf_grapplinghook_projectile_speed", "1500", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast does the grappliing hook projectile travel" );

//=============================================================================
//
// Weapon Flag tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrapplingHook, DT_GrapplingHook )

BEGIN_NETWORK_TABLE( CTFGrapplingHook, DT_GrapplingHook )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrapplingHook )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grapplinghook, CTFGrapplingHook );
PRECACHE_WEAPON_REGISTER( tf_weapon_grapplinghook );

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrapplingHook::CTFGrapplingHook()
{
#ifdef CLIENT_DLL
	m_pPullingLoop = NULL;
#endif
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrapplingHook::~CTFGrapplingHook()
{
#ifdef CLIENT_DLL
	if ( m_pPullingLoop )
	{
		( CSoundEnvelopeController::GetController() ).SoundDestroy( m_pPullingLoop );
		m_pPullingLoop = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "WeaponGrapplingHook.ReelStart" );
	PrecacheScriptSound( "WeaponGrapplingHook.ReelStop" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();
	// Get the player owning the weapon.
	if ( pPlayer )
	{
#ifdef GAME_DLL
		if ( ShouldRemoveInvisibilityOnPrimaryAttack() )
			pPlayer->RemoveInvisibility();

		if ( ShouldRemoveDisguiseOnPrimaryAttack() )
			pPlayer->RemoveDisguise();

		CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
	}

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#if GAME_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );
#endif

	SendWeaponAnim( ACT_GRAPPLE_FIRE_START );

	if ( pPlayer )
	{
		FireProjectile( pPlayer );
		WeaponSound( SINGLE );
	}

	m_flLastFireTime = gpGlobals->curtime;

	// Set next attack times.
	float flFireDelay = 0.5f;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );

	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_RUNE_HASTE ) )
			flFireDelay /= 2;

		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_SPEED_BOOST ) )
			flFireDelay /= 1.5;

		if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
			CALL_ATTRIB_HOOK_FLOAT( flFireDelay, rocketjump_attackrate_bonus );
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::SecondaryAttack( void )
{
	BaseClass::SecondaryAttack();

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::Deploy( void )
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !pOwner->IsAlive() )
	{
		RemoveHookProjectile( true );
		return;
	}

	if ( pOwner->m_nButtons & IN_ATTACK )
	{
		PrimaryAttack();
	}
 	else
 	{
		RemoveHookProjectile( true );
 	}

	if ( pOwner->m_nButtons & IN_JUMP )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
			RemoveHookProjectile( true );
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
		SecondaryAttack();

	if ( pOwner->GetGrapplingHookTarget() && !pOwner->GetGrapplingHookTarget()->IsAlive() )
	{
		RemoveHookProjectile( true );
		return;
	}

#ifdef CLIENT_DLL
	if ( pOwner->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) && !m_pPullingLoop )
	{
		CLocalPlayerFilter filter;
		m_pPullingLoop = ( CSoundEnvelopeController::GetController() ).SoundCreate( filter, entindex(), "WeaponGrapplingHook.ReelStart" );
		( CSoundEnvelopeController::GetController() ).Play( m_pPullingLoop, 1.0, 100 );
	}
	else
	{
		if ( m_pPullingLoop )
		{
			( CSoundEnvelopeController::GetController() ).SoundDestroy( m_pPullingLoop );
			m_pPullingLoop = NULL;
		}
	}
#endif

	WeaponIdle();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFGrapplingHook::CanAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	Vector vecForward; 
	QAngle angShot = pOwner->EyeAngles();
	AngleVectors( angShot, &vecForward );
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecEnd = vecSrc + vecForward * tf_grapplinghook_max_distance.GetFloat();

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID | CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr);
	if ( tr.fraction > 1.0f )
		return false;

	if ( tr.surface.flags & SURF_SKY )
		return false;

	if ( m_hProjectile.Get() )
		return false;

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	return BaseClass::CanAttack();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFGrapplingHook::GetProjectileSpeed( void )
{
	float flSpeed = 2600.0f;
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer->m_Shared.InCond( TF_COND_RUNE_AGILITY ) && !pPlayer->HasTheFlag() )
		flSpeed *= 1.2f;

	return flSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFGrapplingHook::FireProjectile( CTFPlayer *pPlayer )
{
	m_hProjectile = BaseClass::FireProjectile( pPlayer );
	return m_hProjectile;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::RemoveHookProjectile( bool bSomething )
{
#ifdef GAME_DLL
	if ( m_hProjectile )
	{
		if ( GetTFPlayerOwner() )
			GetTFPlayerOwner()->m_nButtons &= ~IN_ATTACK;

		UTIL_Remove( m_hProjectile );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::PlayWeaponShootSound( void )
{
	//WeaponSound( SINGLE );
}

acttable_t CTFGrapplingHook::s_grapplinghook_normal_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE_ALLCLASS, false },
	{ ACT_MP_WALK, ACT_MP_WALK_MELEE_ALLCLASS, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_MELEE_ALLCLASS, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_MELEE_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_MELEE_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_MELEE_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_MELEE_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_MELEE_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_MELEE_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_MELEE, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_MELEE, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_MELEE, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_MELEE, false },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
acttable_t *CTFGrapplingHook::ActivityList( int &iActivityCount )
{
	acttable_t *pTable = s_grapplinghook_normal_acttable;
	iActivityCount = Q_ARRAYSIZE( s_grapplinghook_normal_acttable );

	return pTable;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrapplingHook::TranslateViewmodelHandActivity( int iActivity )
{
	int iTranslation = iActivity;

	if ( GetTFPlayerOwner() )
	{
		if ( ( !GetItem() || GetItem()->GetAnimationSlot() != TF_WPN_TYPE_MELEE )/* && DWORD( pOwner + 9236 ) == 4*/)
		{
			switch (iActivity)
			{
				case ACT_VM_DRAW:
					iTranslation = ACT_GRAPPLE_DRAW;
					break;
				case ACT_VM_IDLE:
					if ( GetTFPlayerOwner()->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
						iTranslation = ACT_GRAPPLE_FIRE_IDLE;
					else
						iTranslation = ACT_GRAPPLE_IDLE;
					break;
				case ACT_GRAPPLE_FIRE_START:
					if ( GetTFPlayerOwner()->m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
						iTranslation = ACT_GRAPPLE_FIRE_IDLE;
					break;
				default:
					return BaseClass::TranslateViewmodelHandActivity( iTranslation );
			}
		}
	}

	return BaseClass::TranslateViewmodelHandActivity( iTranslation );
}
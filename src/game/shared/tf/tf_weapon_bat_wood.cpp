//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bat_wood.h"
#include "decals.h"
#include "tf_viewmodel.h"
#include "tf_projectile_stunball.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "in_buttons.h"
// Server specific.
#else
#include "tf_gamestats.h"
#include "tf_player.h"
#endif

ConVar sv_proj_stunball_damage( "sv_proj_stunball_damage", "15", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies stunball damage." );
ConVar tf_scout_stunball_regen_rate( "tf_scout_stunball_regen_rate", "15.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies stunball regen rate." );
ConVar tf_scout_stunball_base_speed( "tf_scout_stunball_base_speed", "3000.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies stunball base speed." );
ConVar tf_scout_bat_launch_delay( "tf_scout_bat_launch_delay", "0.1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Modifies delay for stunball launch" );

ConVar tf2v_use_new_ball_regen( "tf2v_use_new_ball_regen", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Increases ball regeneration by 33%. Best used when tf_scout_stunball_regen_rate set at 15." );

IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Wood, DT_TFWeaponBat_Wood )

BEGIN_NETWORK_TABLE( CTFBat_Wood, DT_TFWeaponBat_Wood )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flNextFireTime ) ),
	RecvPropBool( RECVINFO( m_bFiring ) ),
#else
	SendPropTime( SENDINFO( m_flNextFireTime ) ),
	SendPropBool( SENDINFO( m_bFiring ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat_Wood )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_wood, CTFBat_Wood );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_wood );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFBat_Wood::CTFBat_Wood()
{
#ifdef CLIENT_DLL
	// Assume true in case the player is switching to this weapon at resupply
	m_bHasBall = true;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Wood::Precache( void )
{
	PrecacheModel( TF_STUNBALL_VIEWMODEL );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Check if we should have the stunball out
//-----------------------------------------------------------------------------
bool CTFBat_Wood::Deploy( void )
{
#ifdef CLIENT_DLL
	// This is here in case the ammo state changes while the weapon is holstered
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		// Do we currently have ammo?
		m_bHasBall = pOwner->GetAmmoCount( m_iPrimaryAmmoType ) ? true : false;
	}
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Check if we can currently create a new ball
//-----------------------------------------------------------------------------
bool CTFBat_Wood::CanCreateBall( CTFPlayer *pPlayer )
{
	// We need ammo to fire
	if ( !pPlayer )
		return false;

	int iType = 0;
	CALL_ATTRIB_HOOK_INT( iType, set_weapon_mode );

	// Only weapon mode 1 can fire balls
	if ( !iType )
		return false;

	trace_t tr;
	Vector vecStart, vecEnd, vecDir;
	AngleVectors( pPlayer->EyeAngles(), &vecDir );

	vecStart = pPlayer->EyePosition();
	vecEnd = vecStart + (vecDir * 48);

	CTraceFilterIgnorePlayers *pFilter = new CTraceFilterIgnorePlayers( this, COLLISION_GROUP_NONE );

	UTIL_TraceLine( vecStart, vecEnd, MASK_ALL, pFilter, &tr );

#ifdef GAME_DLL
	//NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, true, 5.0f );
#endif

	// A wall is stopping our fire
	if ( tr.DidHitWorld() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Pick up a ball on the ground
//-----------------------------------------------------------------------------
bool CTFBat_Wood::PickedUpBall( CTFPlayer *pPlayer )
{
	// Only one ball at a time
	if ( !pPlayer || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) || GetEffectBarProgress() == 1.0f )
		return false;

#ifdef GAME_DLL
	// Pick up the ball
	pPlayer->GiveAmmo( 1, m_iPrimaryAmmoType, true, TF_AMMO_SOURCE_RESUPPLY );
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_GRAB_BALL );
	m_flEffectBarRegenTime = gpGlobals->curtime;
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Swing the bat
//-----------------------------------------------------------------------------
void CTFBat_Wood::PrimaryAttack( void )
{
	if ( m_flNextFireTime > gpGlobals->curtime )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: Alt. Fire
//-----------------------------------------------------------------------------
void CTFBat_Wood::SecondaryAttack( void )
{
	if ( m_flNextFireTime > gpGlobals->curtime || m_bFiring )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer || !pPlayer->CanAttack() || !CanCreateBall( pPlayer ) || !pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) )
		return;

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	m_flNextFireTime = gpGlobals->curtime + 0.25f;
	SetWeaponIdleTime( m_flNextFireTime );

	SetContextThink( &CTFBat_Wood::LaunchBallThink, gpGlobals->curtime + tf_scout_bat_launch_delay.GetFloat(), "LAUNCH_BALL_THINK" );
	m_bFiring = true;

#ifdef GAME_DLL
	if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
		pPlayer->RemoveInvisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::ItemPostFrame( void )
{
#ifdef CLIENT_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner )
	{
		// Do we currently have ammo?
		m_bHasBall = pOwner->GetAmmoCount( m_iPrimaryAmmoType ) ? true : false;
	}
#endif

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::LaunchBallThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
#ifdef GAME_DLL
		LaunchBall( GetTFPlayerOwner() );
		pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

		pPlayer->SpeakWeaponFire( MP_CONCEPT_BAT_BALL );
		CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
		StartEffectBarRegen();
	}

	m_bFiring = false;
}

//-----------------------------------------------------------------------------
// Purpose: Launch a ball
//-----------------------------------------------------------------------------
CBaseEntity *CTFBat_Wood::LaunchBall( CTFPlayer *pPlayer )
{
	WeaponSound( SPECIAL2 );

#ifdef GAME_DLL
	AngularImpulse spin = AngularImpulse( random->RandomInt( -1200, 1200 ), 0, 0 );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// The ball always launches at the player's crouched eye position
	// so that it always passes through the bbox
	Vector vecSrc = pPlayer->GetAbsOrigin() + VEC_DUCK_VIEW_SCALED( pPlayer );
	vecSrc +=  vecForward * -64.0f;


	//NDebugOverlay::Line( pPlayer->EyePosition(), vecSrc, 0, 255, 0, true, 5.0f );
	
	Vector vecVelocity = ( vecForward * tf_scout_stunball_base_speed.GetFloat() ) + ( vecUp * 200.0f );

	//GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, false );

	CTFStunBall* pProjectile = CTFStunBall::Create(this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
	if ( pProjectile )
	{
		CalcIsAttackCritical();
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( sv_proj_stunball_damage.GetFloat() );
	}

	return pProjectile;
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update animations depending on if we do or do not have the ball
//-----------------------------------------------------------------------------
bool CTFBat_Wood::SendWeaponAnim( int iActivity )
{
	int iNewActivity = iActivity;
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// if we have the ball show it on our viewmodel 
	if ( pPlayer && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) )
	{
		switch ( iActivity )
		{
			case ACT_VM_DRAW:
				iNewActivity = ACT_VM_DRAW_SPECIAL;
				break;
			case ACT_VM_HOLSTER:
				iNewActivity = ACT_VM_HOLSTER_SPECIAL;
				break;
			case ACT_VM_IDLE:
				iNewActivity = ACT_VM_IDLE_SPECIAL;
				break;
			case ACT_VM_PULLBACK:
				iNewActivity = ACT_VM_PULLBACK_SPECIAL;
				break;
			case ACT_VM_PRIMARYATTACK:
			case ACT_VM_SECONDARYATTACK:
				iNewActivity = ACT_VM_PRIMARYATTACK_SPECIAL;
				break;
			case ACT_VM_HITCENTER:
				iNewActivity = ACT_VM_HITCENTER_SPECIAL;
				break;
			case ACT_VM_SWINGHARD:
				iNewActivity = ACT_VM_SWINGHARD_SPECIAL;
				break;
			case ACT_VM_IDLE_TO_LOWERED:
				iNewActivity = ACT_VM_IDLE_TO_LOWERED_SPECIAL;
				break;
			case ACT_VM_IDLE_LOWERED:
				iNewActivity = ACT_VM_IDLE_LOWERED_SPECIAL;
				break;
			case ACT_VM_LOWERED_TO_IDLE:
				iNewActivity = ACT_VM_LOWERED_TO_IDLE_SPECIAL;
				break;
		}
	}

	return BaseClass::SendWeaponAnim( iNewActivity );
}
float CTFBat_Wood::InternalGetEffectBarRechargeTime( void )
{
	float flRegenTime = tf_scout_stunball_regen_rate.GetFloat();
	if (tf2v_use_new_ball_regen.GetBool())
		flRegenTime *= (10/15);
	
	return flRegenTime;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFBat_Wood::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	// Really hacky workaround for primary/secondary interactions
	if ( m_flNextFireTime > gpGlobals->curtime )
	{
		pCmd->buttons &= ~IN_ATTACK;
		pCmd->buttons &= ~IN_ATTACK2;
	}

	if ( pCmd->buttons & IN_ATTACK2 )
	{	
		if ( !CanCreateBall( GetTFPlayerOwner() ) )
		{
			pCmd->buttons &= ~IN_ATTACK2;
		}
		else
		{
			pCmd->buttons &= ~IN_ATTACK;
		}
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Giftwrap, DT_TFWeaponBat_Giftwrap )

BEGIN_NETWORK_TABLE( CTFBat_Giftwrap, DT_TFWeaponBat_Giftwrap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBat_Giftwrap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bat_giftwrap, CTFBat_Giftwrap );
PRECACHE_WEAPON_REGISTER( tf_weapon_bat_giftwrap );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBat_Giftwrap::Precache( void )
{
	PrecacheModel( TF_BAUBLE_VIEWMODEL );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Giftwrap::LaunchBallThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
#ifdef GAME_DLL
		LaunchBall( GetTFPlayerOwner() );
		pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

		CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
		StartEffectBarRegen();
	}

	m_bFiring = false;
}

//-----------------------------------------------------------------------------
// Purpose: Launch a ball
//-----------------------------------------------------------------------------
CBaseEntity *CTFBat_Giftwrap::LaunchBall( CTFPlayer *pPlayer )
{
	WeaponSound( SPECIAL2 );

#ifdef GAME_DLL
	AngularImpulse spin = AngularImpulse( random->RandomInt( -1200, 1200 ), 0, 0 );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// The ball always launches at the player's crouched eye position
	// so that it always passes through the bbox
	Vector vecSrc = pPlayer->GetAbsOrigin() + VEC_DUCK_VIEW_SCALED( pPlayer );
	vecSrc +=  vecForward * -64.0f;


	//NDebugOverlay::Line( pPlayer->EyePosition(), vecSrc, 0, 255, 0, true, 5.0f );
	
	Vector vecVelocity = ( vecForward * tf_scout_stunball_base_speed.GetFloat() ) + ( vecUp * 200.0f );

	//GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, false );

	CTFBauble* pProjectile = CTFBauble::Create(this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
	if ( pProjectile )
	{
		CalcIsAttackCritical();
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( sv_proj_stunball_damage.GetFloat() );
	}

	return pProjectile;
#endif
	return NULL;
}
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
#include "tf_player.h"
#endif

#define TF_BAT_DAMAGE 15.0f
#define TF_BAT_VEL 2990.0f
#define TF_BAT_GRAV 1.0f
#define TF_STUNBALL_VIEWMODEL "models/weapons/v_models/v_baseball.mdl"
#define TF_STUNBALL_ADDON 2

IMPLEMENT_NETWORKCLASS_ALIASED( TFBat_Wood, DT_TFWeaponBat_Wood )

BEGIN_NETWORK_TABLE( CTFBat_Wood, DT_TFWeaponBat_Wood )
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
	m_flNextFireTime = -1;
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
// Purpose: Reset the fire time when we holster
//-----------------------------------------------------------------------------
bool CTFBat_Wood::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	UpdateViewmodelBall( GetTFPlayerOwner(), true );
#endif

	m_flNextFireTime = -1;
	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: Reset the fire time when we deploy
//-----------------------------------------------------------------------------
bool CTFBat_Wood::Deploy( void )
{
#ifdef CLIENT_DLL
	UpdateViewmodelBall( GetTFPlayerOwner() );
#endif

	m_flNextFireTime = -1;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the viewmodel 
//-----------------------------------------------------------------------------
void CTFBat_Wood::WeaponReset( void )
{
#ifdef CLIENT_DLL
	UpdateViewmodelBall( GetTFPlayerOwner(), true );
#endif

	m_flNextFireTime = -1;
	BaseClass::WeaponReset();
}
//-----------------------------------------------------------------------------
// Purpose: Check if we can currently create a new ball
//-----------------------------------------------------------------------------
bool CTFBat_Wood::CanCreateBall( CTFPlayer *pPlayer )
{
	// We need ammo to fire
	if ( !pPlayer || !pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) )
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
	if ( !pPlayer || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) )
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
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer || !pPlayer->CanAttack() || !CanCreateBall( pPlayer ) )
		return;

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );

	m_flNextFireTime = gpGlobals->curtime + 0.1f;
	m_flNextPrimaryAttack = m_flNextFireTime;

	SetWeaponIdleTime( gpGlobals->curtime + 0.35f + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );

	FireBall( pPlayer );
	pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
	StartEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::WeaponIdle( void )
{
	if ( m_flNextFireTime > 0 && gpGlobals->curtime > m_flNextFireTime )
	{
		WeaponSound( SPECIAL2 );
		m_flNextFireTime = -1;
	}

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::ItemPostFrame( void )
{
#ifdef CLIENT_DLL
	UpdateViewmodelBall( GetTFPlayerOwner() );
#endif

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Fire a ball
//-----------------------------------------------------------------------------
CBaseEntity *CTFBat_Wood::FireBall( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	AngularImpulse spin = AngularImpulse( random->RandomInt( -1200, 1200 ), 0, 0 );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	// Set the starting position a bit behind the player so the projectile
	// launches out of the players view
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	vecSrc +=  vecForward * -64.0f + vecRight * 8.0f + vecUp * -6.0f;

	
	Vector vecVelocity = ( vecForward * TF_BAT_VEL ) + ( vecUp * 200.0f );

	//GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false, false );

	CTFStunBall *pProjectile = CTFStunBall::Create(this, vecSrc, pPlayer->EyeAngles(), vecVelocity, pPlayer, pPlayer, spin, GetTFWpnData() );
	if ( pProjectile )
	{
		CalcIsAttackCritical();
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( TF_BAT_DAMAGE );
	}

	return pProjectile;
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Remove the ball
//-----------------------------------------------------------------------------
void CTFBat_Wood::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	UpdateViewmodelBall( GetTFPlayerOwner(), true );
#endif

	BaseClass::UpdateOnRemove();
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

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBat_Wood::UpdateViewmodelBall( C_TFPlayer *pOwner, bool bHolster /*= false*/ )
{
	if ( pOwner )
	{
		C_TFViewModel *vm = dynamic_cast < C_TFViewModel *  >( pOwner->GetViewModel( m_nViewModelIndex ) );
		if ( vm )
		{
			// Update the ball
			if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 && !bHolster )
			{
				vm->UpdateViewmodelAddon( TF_STUNBALL_VIEWMODEL, TF_STUNBALL_ADDON );
			}
			else
			{
				vm->UpdateViewmodelAddon( "", TF_STUNBALL_ADDON );
			}
		}
	}
}
#endif

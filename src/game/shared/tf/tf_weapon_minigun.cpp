//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_minigun.h"
#include "decals.h"
#include "in_buttons.h"
#include "tf_fx_shared.h"


// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "bone_setup.h"
#include "c_te_legacytempents.h"

// Server specific.
#else
#include "tf_player.h"
#include "tf_gamerules.h"
#include "takedamageinfo.h"
#endif

#ifdef CLIENT_DLL
	#include "c_te_effect_dispatch.h"
#else
	#include "te_effect_dispatch.h"
#endif

#define MAX_BARREL_SPIN_VELOCITY	20
#define TF_MINIGUN_PENALTY_TIME 1

//=============================================================================
//
// Weapon Minigun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigun, DT_WeaponMinigun )

BEGIN_NETWORK_TABLE( CTFMinigun, DT_WeaponMinigun )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) )
// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritShot ) )
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFMinigun )
	DEFINE_FIELD(  m_iWeaponState, FIELD_INTEGER ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_minigun, CTFMinigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_minigun );


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFMinigun )
END_DATADESC()
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFMinigun_Real, tf_weapon_minigun_real )


#ifdef CLIENT_DLL
extern ConVar tf2v_model_muzzleflash;
extern ConVar cl_ejectbrass;
ConVar tf2v_minigun_ejectbrass( "tf2v_minigun_ejectbrass", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Use real shells instead of sprites?");
#endif

ConVar tf2v_use_new_minigun_spinup("tf2v_use_new_minigun_spinup", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Makes winding and unwinding the minigun 25% faster." );
ConVar tf2v_use_new_minigun_rampup("tf2v_use_new_minigun_rampup", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes the accuracy and damage of the minigun based on fire time.", true, 0, true, 3 );


//=============================================================================
//
// Weapon Minigun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFMinigun::CTFMinigun()
{

#ifdef CLIENT_DLL
	m_pSoundCur = NULL;
#endif

#ifdef CLIENT_DLL
	m_pEjectBrassEffect = NULL;
	m_iEjectBrassAttachment = -1;

	m_pMuzzleEffect = NULL;
	m_iMuzzleAttachment = -1;
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFMinigun::~CTFMinigun()
{
	WeaponReset();
}

void CTFMinigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = AC_STATE_IDLE;
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bCritShot = false;
	m_flStartedFiringAt = -1;
	m_flStartedWindingAt = -1;
	m_flNextFiringSpeech = 0;

	m_flBarrelAngle = 0;

	m_flBarrelCurrentVelocity = 0;
	m_flBarrelTargetVelocity = 0;

#ifdef CLIENT_DLL
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	m_iMinigunSoundCur = -1;

	StopMuzzleEffect();
	StopBrassEffect();
#endif
}

#ifdef GAME_DLL
int CTFMinigun::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::Precache( void )
{
	BaseClass::Precache();
}

void CTFMinigun::Spawn(void)
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::PrimaryAttack()
{
	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SharedAttack()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		WeaponIdle();
		return;
	}


	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	}
	else if ( pPlayer->m_nButtons & IN_ATTACK2 )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}
	
	// Recalculate our thinking times based on fire delays.
	float flFireDelay = m_pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );
	
	// We drain ammo when spinning or firing, but only when we have the trait.
	int iDrainTimeInterval = 0;
	int iAmmoDrain = 0;
	CALL_ATTRIB_HOOK_FLOAT(iAmmoDrain, uses_ammo_while_aiming);
	if (iAmmoDrain != 0)
	iDrainTimeInterval = 1/iAmmoDrain; // Time to deduct ammo, in seconds.

	switch ( m_iWeaponState )
	{
	default:
	case AC_STATE_IDLE:
		{
			// Removed the need for cells to powerup the AC
			WindUp();

			float flSpinupTime = GetSpinUpLength();

			if (pPlayer->GetViewModel( m_nViewModelIndex ))
				pPlayer->GetViewModel( m_nViewModelIndex )->SetPlaybackRate( 0.75 / Max( flSpinupTime, FLT_EPSILON) );

			m_flNextPrimaryAttack = gpGlobals->curtime + flSpinupTime;
			m_flNextSecondaryAttack = gpGlobals->curtime + flSpinupTime;
			m_flTimeWeaponIdle = gpGlobals->curtime + flSpinupTime;
			m_flStartedFiringAt = -1;
			m_flStartedWindingAt = -1;
			m_flDrainTime = -1;
			m_flNextFireAttack = -1;
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
			break;
		}
	case AC_STATE_STARTFIRING:
		{
			// Start playing the looping fire sound
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
			{
				if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
				{
					m_iWeaponState = AC_STATE_SPINNING;
#ifdef GAME_DLL
					pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				}
				else
				{
					m_iWeaponState = AC_STATE_FIRING;
#ifdef GAME_DLL
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
				}

				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + flFireDelay;
			}
			if ( m_flStartedWindingAt < 0 )	// We started winding, clock the time.
			{
				m_flStartedWindingAt = gpGlobals->curtime;
				if (iAmmoDrain)	// Set our drain time if we have the attribute.
					m_flDrainTime = m_flStartedWindingAt + iDrainTimeInterval;
			}
			break;
		}
	case AC_STATE_FIRING:
		{
			if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
#ifdef GAME_DLL
				pPlayer->ClearWeaponFireScene();
				pPlayer->SpeakWeaponFire( MP_CONCEPT_WINDMINIGUN );
#endif
				m_iWeaponState = AC_STATE_SPINNING;
				
				if ( m_flNextPrimaryAttack > gpGlobals->curtime )
				return;
	
				m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + flFireDelay;
			}
			else if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
			{
				m_iWeaponState = AC_STATE_DRYFIRE;
			}
			else
			{
				if ( m_flStartedFiringAt < 0 )
				{
					m_flStartedFiringAt = gpGlobals->curtime;
				}

#ifdef GAME_DLL
				if ( m_flNextFiringSpeech < gpGlobals->curtime )
				{
					m_flNextFiringSpeech = gpGlobals->curtime + 5.0;
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MINIGUN_FIREWEAPON );
				}
#endif

				// Only fire if we're actually shooting
				UseRealMinigunBrassEject();
				BaseClass::PrimaryAttack();		// fire and do timers
				CalcIsAttackCritical();
				m_bCritShot = IsCurrentAttackACrit();
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flTimeWeaponIdle = gpGlobals->curtime + (flFireDelay * 2);
			}
			break;
		}
	case AC_STATE_DRYFIRE:
		{
			m_flStartedFiringAt = -1;
			if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
			{
				m_iWeaponState = AC_STATE_FIRING;
			}
			else if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
			{
				m_iWeaponState = AC_STATE_SPINNING;
			}
			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	case AC_STATE_SPINNING:
		{
			m_flStartedFiringAt = -1;
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				if ( pPlayer->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
				{
#ifdef GAME_DLL
					pPlayer->ClearWeaponFireScene();
					pPlayer->SpeakWeaponFire( MP_CONCEPT_FIREMINIGUN );
#endif
					m_iWeaponState = AC_STATE_FIRING;
				}
				else
				{
					m_iWeaponState = AC_STATE_DRYFIRE;
				}
			}

			SendWeaponAnim( ACT_VM_SECONDARYATTACK );
			break;
		}
	}
	

	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		// Drain ammo when winding, if we have the trait.
		if ( iAmmoDrain != 0 && gpGlobals->curtime > m_flDrainTime )
		{
			// If we're above the drain time, take bullets away.
			m_iClip1 -= iAmmoDrain;
			m_flDrainTime = gpGlobals->curtime + iDrainTimeInterval;
		}
		
		// Check if we should do an AOE fire attack.
		int nFireAttack = 0;
		CALL_ATTRIB_HOOK_INT( nFireAttack, ring_of_fire_while_aiming );
		if ( nFireAttack )
			FireAttack(nFireAttack);
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Fall through to Primary Attack
//-----------------------------------------------------------------------------
void CTFMinigun::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	SharedAttack();
}

//-----------------------------------------------------------------------------
// Purpose: Causes a fire puff attack.
//-----------------------------------------------------------------------------
void CTFMinigun::FireAttack( int nDamageAmount )
{
	if ( m_flNextFireAttack > gpGlobals->curtime )
		return;
	
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;
	
#ifdef CLIENT_DLL
	DispatchParticleEffect( "heavy_ring_of_fire_fp", pOwner->GetAbsOrigin(), vec3_angle );
#else
	DispatchParticleEffect("heavy_ring_of_fire", pOwner->GetAbsOrigin(), vec3_angle);
	// We explode in a small radius, set us up as an explosion.
	CTakeDamageInfo newInfo(pOwner, pOwner, this, vec3_origin, pOwner->GetAbsOrigin(), nDamageAmount, DMG_IGNITE);
	CTFRadiusDamageInfo radiusInfo(&newInfo, pOwner->GetAbsOrigin(), 135.0f);
	TFGameRules()->RadiusDamage( radiusInfo );
#endif

	// Update the time for the next fire attack.
	m_flNextFireAttack = gpGlobals->curtime + 0.5;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindUp( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_STARTFIRING;
	pPlayer->m_Shared.AddCond( TF_COND_AIMING );

#ifndef CLIENT_DLL
	pPlayer->StopRandomExpressions();
#endif

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif


	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::CanHolster( void ) const
{
	if ( m_iWeaponState > AC_STATE_IDLE )
		return false;

	if ( GetActivity() == ACT_MP_ATTACK_STAND_POSTFIRE ||
		GetActivity() == ACT_PRIMARY_ATTACK_STAND_POSTFIRE ||
		GetActivity() == ACT_SECONDARY_ATTACK_STAND_POSTFIRE ||
		GetActivity() == ACT_MELEE_ATTACK_STAND_POSTFIRE ||
		GetActivity() == ACT_ITEM1_ATTACK_STAND_POSTFIRE ||
		GetActivity() == ACT_ITEM2_ATTACK_STAND_POSTFIRE )
	{
		if ( !IsViewModelSequenceFinished() )
			return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}
	m_flBarrelCurrentVelocity = 0.0f;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMinigun::Lower( void )
{
	if ( m_iWeaponState > AC_STATE_IDLE )
	{
		WindDown();
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WindDown( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	float flSpinDownTime = 2.0f;
	if (!tf2v_use_new_minigun_spinup.GetBool())
	{
		flSpinDownTime *= (4/3);
		if (pPlayer->GetViewModel( m_nViewModelIndex ))
			pPlayer->GetViewModel( m_nViewModelIndex )->SetPlaybackRate( 2.0 / Max( flSpinDownTime, FLT_EPSILON) );
	}
	SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );

	// Set the appropriate firing state.
	m_iWeaponState = AC_STATE_IDLE;
	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#else
	pPlayer->ClearWeaponFireScene();
#endif

	// Time to weapon idle.
	m_flTimeWeaponIdle = gpGlobals->curtime + flSpinDownTime;

	// Update player's speed
	pPlayer->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_flBarrelTargetVelocity = 0;
#endif
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponIdle()
{
	if ( gpGlobals->curtime < m_flTimeWeaponIdle )
		return;

	// Always wind down if we've hit here, because it only happens when the player has stopped firing/spinning
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		}

		WindDown();
		return;
	}

	BaseClass::WeaponIdle();

	m_flTimeWeaponIdle = gpGlobals->curtime + 12.5;// how long till we do this again.
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMinigun::SendWeaponAnim( int iActivity )
{
#ifdef CLIENT_DLL
	// Client procedurally animates the barrel bone
	if ( iActivity == ACT_MP_ATTACK_STAND_PRIMARYFIRE || iActivity == ACT_MP_ATTACK_STAND_PREFIRE )
	{
		m_flBarrelTargetVelocity = MAX_BARREL_SPIN_VELOCITY;
	}
	else if ( iActivity == ACT_MP_ATTACK_STAND_POSTFIRE )
	{
		m_flBarrelTargetVelocity = 0;
	}

#endif


	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
			return true;

		// Otherwise, play the start it
		return BaseClass::SendWeaponAnim( ACT_VM_PRIMARYATTACK );		
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: This will force the minigun to turn off the firing sound and play the spinning sound
//-----------------------------------------------------------------------------
void CTFMinigun::HandleFireOnEmpty( void )
{
	if ( m_iWeaponState == AC_STATE_FIRING || m_iWeaponState == AC_STATE_SPINNING )
	{
		 m_iWeaponState = AC_STATE_DRYFIRE;

		 SendWeaponAnim( ACT_VM_SECONDARYATTACK );

		 if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
		 {
			m_iWeaponState = AC_STATE_SPINNING;
		 }
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetProjectileDamage( void )
{
	float flDamage = BaseClass::GetProjectileDamage();

	if ( tf2v_use_new_minigun_rampup.GetInt() != 0)
	{
		float flDamageMod = 1.0f;
		switch (tf2v_use_new_minigun_rampup.GetInt())
		{
			case 1:	// Rampup based on firing time.
			case 2:
			if ( GetFiringTime() < TF_MINIGUN_PENALTY_TIME )
				flDamageMod = RemapValClamped( GetFiringTime(), 0.2, TF_MINIGUN_PENALTY_TIME, 0.5, 1 );			
			break;
			case 3:	// Rampup based on spinning time.
			if ( GetWindingTime() < TF_MINIGUN_PENALTY_TIME + GetSpinUpLength() )
				flDamageMod = RemapValClamped( GetWindingTime(), 0.2, (TF_MINIGUN_PENALTY_TIME + GetSpinUpLength()), 0.5, 1 );				
			break;
		}
		
		if ( flDamageMod != 1.0f )	// If damage modified, adjust.
			flDamage *= flDamageMod;
	}
	
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetWeaponSpread( void )
{
	float flSpread = BaseClass::GetWeaponSpread();

	if ( tf2v_use_new_minigun_rampup.GetInt() != 0)
	{
		float flSpreadMod = 1.0f;
		switch (tf2v_use_new_minigun_rampup.GetInt())
		{
			case 1:	// Rampup based on firing time.
			case 2:
			if ( GetFiringTime() < TF_MINIGUN_PENALTY_TIME )
				flSpreadMod = RemapValClamped( GetFiringTime(), 0.2, TF_MINIGUN_PENALTY_TIME, 0.5, 1 );			
			break;
			case 3:	// Rampup based on spinning time.
			if ( GetWindingTime() < TF_MINIGUN_PENALTY_TIME + GetSpinUpLength() )
				flSpreadMod = RemapValClamped( GetWindingTime(), 0.2, (TF_MINIGUN_PENALTY_TIME + GetSpinUpLength()), 0.5, 1 );				
			break;
		}
		
		if ( flSpreadMod != 1.0f )	// If damage modified, adjust.
			flSpread *= flSpreadMod;
	}
	
	return flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFMinigun::GetSpinUpLength( void )
{
	float flSpinupTime = 0.75f;
	if (!tf2v_use_new_minigun_spinup.GetBool())
	flSpinupTime *= (4/3);
			
	CALL_ATTRIB_HOOK_FLOAT( flSpinupTime, mult_minigun_spinup_time );
	flSpinupTime = Max( flSpinupTime, FLT_EPSILON ); // Don't divide by 0
	return flSpinupTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::UseRealMinigunBrassEject( void )
{
#ifdef CLIENT_DLL
	// If minigun shells or shells in general aren't enabled, bail.
	if (tf2v_minigun_ejectbrass.GetBool() && cl_ejectbrass.GetBool() )
	{
		// If it's time to fire, then run the calculation.
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;
		
		
		C_BaseEntity *pEffectOwner = GetWeaponForEffect();
		if ( !pEffectOwner )
		return;
		
		int iEjectBrassAttachmentReal = pEffectOwner->LookupAttachment("eject_brass");
		
		CEffectData brassejectdata;
		if (iEjectBrassAttachmentReal != -1)
		{
			pEffectOwner->GetAttachment(iEjectBrassAttachmentReal, brassejectdata.m_vOrigin, brassejectdata.m_vAngles);
			brassejectdata.m_nHitBox = TF_WEAPON_MINIGUN;
			DispatchEffect("TF_EjectBrass", brassejectdata);
		}
	}
#endif
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFMinigun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iBarrelBone = LookupBone( "barrel" );

	// skip resetting this while recording in the tool
	// we change the weapon to the worldmodel and back to the viewmodel when recording
	// which causes the minigun to not spin while recording
	if ( !IsToolRecording() )
	{
		m_flBarrelAngle = 0;

		m_flBarrelCurrentVelocity = 0;
		m_flBarrelTargetVelocity = 0;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if (m_iBarrelBone != -1)
	{
		UpdateBarrelMovement();

		// Weapon happens to be aligned to (0,0,0)
		// If that changes, use this code block instead to
		// modify the angles

		/*
		RadianEuler a;
		QuaternionAngles( q[iBarrelBone], a );

		a.x = m_flBarrelAngle;

		AngleQuaternion( a, q[iBarrelBone] );
		*/

		AngleQuaternion( RadianEuler( 0, 0, m_flBarrelAngle ), q[m_iBarrelBone] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	int iBarrelBone = Studio_BoneIndexByName( hdr, "barrel" );
	if  ( iBarrelBone != -1 && ( hdr->boneFlags( iBarrelBone ) & boneMask ) )
	{
		RadianEuler a;
		QuaternionAngles( q[ iBarrelBone ], a );
		a.z = GetBarrelRotation();
		AngleQuaternion( a, q[ iBarrelBone ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the velocity and position of the rotating barrel
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateBarrelMovement()
{
	if ( m_flBarrelCurrentVelocity != m_flBarrelTargetVelocity )
	{
		float flSpinupTime = GetSpinUpLength();

		// update barrel velocity to bring it up to speed or to rest
		m_flBarrelCurrentVelocity = Approach( m_flBarrelTargetVelocity, m_flBarrelCurrentVelocity, 0.1 / flSpinupTime );

		if ( 0 == m_flBarrelCurrentVelocity )
		{	
			// if we've stopped rotating, turn off the wind-down sound
			WeaponSoundUpdate();
		}
	}

	// update the barrel rotation based on current velocity
	m_flBarrelAngle += m_flBarrelCurrentVelocity * gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::OnDataChanged( DataUpdateType_t updateType )
{
	// Brass ejection and muzzle flash.
	HandleBrassEffect();
	
//	if (!ShouldMuzzleFlash())
	if (!tf2v_model_muzzleflash.GetBool())
	{
		HandleMuzzleEffect();
	}

	BaseClass::OnDataChanged( updateType );

	WeaponSoundUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::UpdateOnRemove( void )
{
	if ( m_pSoundCur )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}

	// Force the particle system off.
	StopMuzzleEffect();
	StopBrassEffect();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		// Am I firing? Stop the firing sound.
		if ( !IsDormant() && bDormant && m_iWeaponState >= AC_STATE_FIRING )
		{
			WeaponSoundUpdate();
		}

		// If firing and going dormant - stop the brass effect.
		if ( !IsDormant() && bDormant && m_iWeaponState != AC_STATE_IDLE )
		{
			StopMuzzleEffect();
			StopBrassEffect();
		}
	}

	// Deliberately skip base combat weapon
	//C_BaseEntity::SetDormant( bDormant );
	BaseClass::SetDormant(bDormant);
}


//-----------------------------------------------------------------------------
// Purpose: 
// won't be called for w_ version of the model, so this isn't getting updated twice
//-----------------------------------------------------------------------------
void CTFMinigun::ItemPreFrame( void )
{
	UpdateBarrelMovement();
	BaseClass::ItemPreFrame();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartBrassEffect()
{
	StopBrassEffect();

	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iEjectBrassAttachment == -1 )
	{
		m_iEjectBrassAttachment = pEffectOwner->LookupAttachment( "eject_brass" );
	}

	// Start the brass ejection, if a system hasn't already been started.
	if ( m_iEjectBrassAttachment != -1 && m_pEjectBrassEffect == NULL )
	{
		if (!cl_ejectbrass.GetBool() || !tf2v_minigun_ejectbrass.GetBool() )
			m_pEjectBrassEffect = pEffectOwner->ParticleProp()->Create( "eject_minigunbrass", PATTACH_POINT_FOLLOW, m_iEjectBrassAttachment );
		m_hBrassEffectHost = pEffectOwner;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StartMuzzleEffect()
{
	StopMuzzleEffect();

	C_BaseEntity *pEffectOwner = GetWeaponForEffect();
	if ( !pEffectOwner )
		return;

	// Try and setup the attachment point if it doesn't already exist.
	// This caching will mess up if we go third person from first - we only do this in taunts and don't fire so we should
	// be okay for now.
	if ( m_iMuzzleAttachment == -1 )
	{
		m_iMuzzleAttachment = pEffectOwner->LookupAttachment( "muzzle" );
	}

	// Start the muzzle flash, if a system hasn't already been started.
	if ( m_iMuzzleAttachment != -1 && m_pMuzzleEffect == NULL )
	{
		m_pMuzzleEffect = pEffectOwner->ParticleProp()->Create( "muzzle_minigun_constant", PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
		m_hMuzzleEffectHost = pEffectOwner;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopBrassEffect()
{
	C_BaseEntity *pEffectOwner = m_hBrassEffectHost.Get();

	// Stop the brass ejection.
	if ( m_pEjectBrassEffect )
	{
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_pEjectBrassEffect );
			m_hBrassEffectHost = NULL;
		}

		m_pEjectBrassEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::StopMuzzleEffect()
{
	C_BaseEntity *pEffectOwner = m_hMuzzleEffectHost.Get();

	// Stop the muzzle flash.
	if ( m_pMuzzleEffect )
	{
		if ( pEffectOwner )
		{
			pEffectOwner->ParticleProp()->StopEmission( m_pMuzzleEffect );
			m_hMuzzleEffectHost = NULL;
		}

		m_pMuzzleEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleBrassEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pEjectBrassEffect == NULL )
	{
		StartBrassEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pEjectBrassEffect )
	{
		StopBrassEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMinigun::HandleMuzzleEffect()
{
	if ( m_iWeaponState == AC_STATE_FIRING && m_pMuzzleEffect == NULL )
	{	
		StartMuzzleEffect();
	}
	else if ( m_iWeaponState != AC_STATE_FIRING && m_pMuzzleEffect )
	{
		StopMuzzleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: View model barrel rotation angle. Calculated here, implemented in 
// tf_viewmodel.cpp
//-----------------------------------------------------------------------------
float CTFMinigun::GetBarrelRotation( void )
{
	return m_flBarrelAngle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMinigun::CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles )
{
	// Prevent jumping while firing
	if ( m_iWeaponState != AC_STATE_IDLE )
	{
		pCmd->buttons &= ~IN_JUMP;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd, vecOldViewAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CTFMinigun::WeaponSoundUpdate()
{
	// determine the desired sound for our current state
	int iSound = -1;
	switch ( m_iWeaponState )
	{
	case AC_STATE_IDLE:
		if ( m_flBarrelCurrentVelocity > 0 )
		{
			iSound = SPECIAL2;	// wind down sound
			if ( m_flBarrelTargetVelocity > 0 )
			{
				m_flBarrelTargetVelocity = 0;
			}
		}
		else
			iSound = -1;
		break;
	case AC_STATE_STARTFIRING:
		iSound = SPECIAL1;	// wind up sound
		break;
	case AC_STATE_FIRING:
		{
			if ( m_bCritShot == true ) 
			{
				iSound = BURST;	// Crit sound
			}
			else
			{
				iSound = WPN_DOUBLE; // firing sound
			}
		}
		break;
	case AC_STATE_SPINNING:
		if ( CAttributeManager::AttribHookValue<int>( 0, "minigun_no_spin_sounds", this ) == 0 )
			iSound = SPECIAL3;	// spinning sound
		break;
	case AC_STATE_DRYFIRE:
		iSound = EMPTY;		// out of ammo, still trying to fire
		break;
	default:
		Assert( false );
		break;
	}

	// if we're already playing the desired sound, nothing to do
	if ( m_iMinigunSoundCur == iSound )
		return;

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iMinigunSoundCur = iSound;
	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	// play the appropriate sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
	controller.SoundChangeVolume( m_pSoundCur, 1.0, 0.1 );
}
#endif

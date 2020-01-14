//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "props_shared.h"
#include "tf_weapon_shotgun.h"
#include "decals.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"

// Client specific.
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#endif

#if defined( CLIENT_DLL )
extern ConVar cl_autoreload;
#endif

float AirBurstDamageForce( Vector const &vecSize, float damage, float scale )
{
	const float flSizeMag = vecSize.x * vecSize.y * vecSize.z;
	const float flHullMag = 48 * 48 * 82.0;

	const float flDamageForce = damage * ( flHullMag / flSizeMag ) * scale;

	return Min( flDamageForce, 1000.0f );
}

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Soldier, tf_weapon_shotgun_soldier )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_HWG, tf_weapon_shotgun_hwg )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Pyro, tf_weapon_shotgun_pyro )
CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )
CREATE_SIMPLE_WEAPON_TABLE( TFPepBrawlBlaster, tf_weapon_pep_brawler_blaster)
CREATE_SIMPLE_WEAPON_TABLE( TFSodaPopper, tf_weapon_soda_popper)

//=============================================================================
//
// Weapon Shotgun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShotgun::CTFShotgun()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
	pPlayer->SetPunchAngle( angle );
}

//=============================================================================
//
// Weapon Scatter Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScatterGun::FireBullet( CTFPlayer *pPlayer )
{
	if ( !HasKnockback() || ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_PREROUND ) )
	{
		BaseClass::FireBullet( pPlayer );
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !( ( pOwner->GetFlags() & FL_ONGROUND ) || pOwner->m_Shared.HasRecoiled() ) )
	{
		pOwner->m_Shared.SetHasRecoiled( true );

		pOwner->m_Shared.StunPlayer( 0.3f, 1.0f, 1.0f, TF_STUNFLAG_LIMITMOVEMENT | TF_STUNFLAG_SLOWDOWN, NULL );

	#if defined( GAME_DLL )
		EntityMatrix matrix;
		matrix.InitFromEntity( pOwner );

		Vector vecLocalTranslation = pOwner->GetAbsOrigin() + pOwner->GetAbsVelocity();

		Vector vecLocal = matrix.WorldToLocal( vecLocalTranslation );
		vecLocal.x = -300.0f;

		Vector vecVelocity = matrix.LocalToWorld( vecLocal );
		vecVelocity -= pOwner->GetAbsOrigin();

		pOwner->SetAbsVelocity( vecVelocity );

		pOwner->ApplyAbsVelocityImpulse( Vector( 0, 0, 50 ) );
		pOwner->RemoveFlag( FL_ONGROUND );
	#endif
	}

	BaseClass::FireBullet( pPlayer );
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScatterGun::ApplyPostOnHitAttributes( CTakeDamageInfo const &info, CTFPlayer *pVictim )
{
	BaseClass::ApplyPostOnHitAttributes( info, pVictim );

	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pAttacker == NULL || pVictim == NULL )
		return;

	if ( !HasKnockback() || pVictim->m_Shared.InCond( TF_COND_MEGAHEAL ) )
		return;

	if ( pVictim->m_Shared.GetKnockbackWeaponID() >= 0 )
		return;

	Vector vecToVictim = pAttacker->WorldSpaceCenter() - pVictim->WorldSpaceCenter();

	float flKnockbackMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flKnockbackMult, scattergun_knockback_mult );

	// This check is a bit open ended and broad
	if ( ( info.GetDamage() <= 30.0f || vecToVictim.LengthSqr() > Square( 400.0f ) ) && flKnockbackMult < 1.0f )
		return;

	vecToVictim.NormalizeInPlace();

	float flDmgForce = AirBurstDamageForce( pVictim->WorldAlignSize(), info.GetDamage(), flKnockbackMult );
	Vector vecVelocityImpulse = vecToVictim * abs( flDmgForce );

	pVictim->ApplyAirBlastImpulse( vecVelocityImpulse );
	pVictim->m_Shared.StunPlayer( 0.3f, 1.0f, 1.0f, TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_LIMITMOVEMENT, pAttacker );

	pVictim->m_Shared.SetKnockbackWeaponID( pAttacker->GetUserID() );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScatterGun::Equip( CBaseCombatCharacter *pEquipTo )
{
	if ( pEquipTo )
	{
		CTFPlayer *pOwner = ToTFPlayer( pEquipTo );
		if ( pOwner )
		{
			// CTFPlayerShared::SetScoutHypeMeter
		#if defined( CLIENT_DLL )
			m_bAutoReload = cl_autoreload.GetBool();
		#else
			m_bAutoReload = pOwner->ShouldAutoReload();
		#endif
		}
	}

	int nScatterGunNoReloadSingle = 0;
	CALL_ATTRIB_HOOK_INT( nScatterGunNoReloadSingle, set_scattergun_no_reload_single );
	if ( nScatterGunNoReloadSingle == 1 )
		m_bReloadsSingly = false;

	BaseClass::Equip( pEquipTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFScatterGun::Deploy()
{
	if ( !ReloadsSingly() )
	{
	#if defined( CLIENT_DLL )
		cl_autoreload.SetValue( 0 );
	#else
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( pOwner )
			pOwner->SetAutoReload( false );
	#endif
	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFScatterGun::Holster(CBaseCombatWeapon *pSwitchTo)
{
	if ( !ReloadsSingly() )
	{
	#if defined( CLIENT_DLL )
		cl_autoreload.SetValue( m_bAutoReload );
	#else
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( pOwner )
			pOwner->SetAutoReload( m_bAutoReload );
	#endif
	}

	return BaseClass::Holster( pSwitchTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScatterGun::FinishReload()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !UsesClipsForAmmo1() )
		return;

	if ( ReloadsSingly() )
		return;

	m_iClip1 += Min( GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount( m_iPrimaryAmmoType ) );

	if ( !BaseClass::IsEnergyWeapon() )
		pOwner->RemoveAmmo( GetMaxClip1(), m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFScatterGun::SendWeaponAnim( int iActivity )
{
	if ( GetTFPlayerOwner() && HasKnockback() )
	{
		switch ( iActivity )
		{
			case ACT_VM_DRAW:
				iActivity = ACT_ITEM2_VM_DRAW;
				break;
			case ACT_VM_HOLSTER:
				iActivity = ACT_ITEM2_VM_HOLSTER;
				break;
			case ACT_VM_IDLE:
				iActivity = ACT_ITEM2_VM_IDLE;
				break;
			case ACT_VM_PULLBACK:
				iActivity = ACT_ITEM2_VM_PULLBACK;
				break;
			case ACT_VM_PRIMARYATTACK:
				iActivity = ACT_ITEM2_VM_PRIMARYATTACK;
				break;
			case ACT_VM_SECONDARYATTACK:
				iActivity = ACT_ITEM2_VM_SECONDARYATTACK;
				break;
			case ACT_VM_RELOAD:
				iActivity = ACT_ITEM2_VM_RELOAD;
				break;
			case ACT_VM_DRYFIRE:
				iActivity = ACT_ITEM2_VM_DRYFIRE;
				break;
			case ACT_VM_IDLE_TO_LOWERED:
				iActivity = ACT_ITEM2_VM_IDLE_TO_LOWERED;
				break;
			case ACT_VM_IDLE_LOWERED:
				iActivity = ACT_ITEM2_VM_IDLE_LOWERED;
				break;
			case ACT_VM_LOWERED_TO_IDLE:
				iActivity = ACT_ITEM2_VM_LOWERED_TO_IDLE;
				break;
			default:
				return BaseClass::SendWeaponAnim( iActivity );
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFScatterGun::HasKnockback() const
{
	int nScatterGunHasKnockback = 0;
	CALL_ATTRIB_HOOK_INT( nScatterGunHasKnockback, set_scattergun_has_knockback );
	return nScatterGunHasKnockback == 1;
}


IMPLEMENT_NETWORKCLASS_ALIASED( TFShotgun_Revenge, DT_TFShotgun_Revenge )

BEGIN_NETWORK_TABLE( CTFShotgun_Revenge, DT_TFShotgun_Revenge )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_iRevengeCrits ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO( m_iRevengeCrits ) ),
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFShotgun_Revenge )
	DEFINE_PRED_FIELD( m_iRevengeCrits, FIELD_INTEGER, FTYPEDESC_INSENDTABLE )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_sentry_revenge, CTFShotgun_Revenge );
PRECACHE_WEAPON_REGISTER( tf_weapon_sentry_revenge );


CTFShotgun_Revenge::CTFShotgun_Revenge()
{
	m_bReloadsSingly = true;
	m_iRevengeCrits = 0;
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetWorldModelIndex( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->IsAlive() )
	{
		if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) && pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
			return modelinfo->GetModelIndex( "models/player/items/engineer/guitar.mdl" );
	}

	return BaseClass::GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::SetWeaponVisible( bool visible )
{
	if ( !visible )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner && pOwner->IsAlive() )
		{
			if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) && pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
			{
				const int iModelIndex = modelinfo->GetModelIndex( "models/player/items/engineer/guitar.mdl" );

				CUtlVector<breakmodel_t> list;

				BuildGibList( list, iModelIndex, 1.0f, COLLISION_GROUP_NONE );
				if ( !list.IsEmpty() )
				{
					QAngle vecAngles = CollisionProp()->GetCollisionAngles();

					Vector vecFwd, vecRight, vecUp;
					AngleVectors( vecAngles, &vecFwd, &vecRight, &vecUp );

					Vector vecOrigin = CollisionProp()->GetCollisionOrigin();
					vecOrigin = vecOrigin + vecFwd * 70.0f + vecUp * 10.0f;

					AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

					breakablepropparams_t params( vecOrigin, vecAngles, Vector( 0.0f, 0.0f, 200.0f ), angularImpulse );

					CreateGibsFromList( list, iModelIndex, NULL, params, NULL, -1, false, true );
				}
			}

		}
	}
	BaseClass::SetWeaponVisible( visible );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	m_iRevengeCrits = Max( m_iRevengeCrits - 1, 0 );

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->IsAlive() )
	{
		if ( m_iRevengeCrits == 0 )
			pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetCount( void ) const
{
	return m_iRevengeCrits;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetCustomDamageType( void ) const
{
	if ( m_iRevengeCrits > 0 )
		return TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT;

	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::Deploy( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && BaseClass::Deploy() )
	{
		if ( m_iRevengeCrits > 0 )
			pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::Holster( CBaseCombatWeapon *pSwitchTo )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && BaseClass::Holster( pSwitchTo ) )
	{
		pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::Detach( void )
{
	m_iRevengeCrits = 0;
	BaseClass::Detach();
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::OnSentryKilled( CObjectSentrygun *pSentry )
{
	if ( CanGetRevengeCrits() )
	{
		m_iRevengeCrits = Min( m_iRevengeCrits + pSentry->GetAssists() + ( pSentry->GetKills() * 2 ), TF_WEAPON_MAX_REVENGE );

		CTFPlayer *pOwner = GetTFPlayerOwner();

		if ( pOwner && pOwner->GetActiveWeapon() == this )
		{
			if ( m_iRevengeCrits > 0 )
			{
				if ( !pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED ) )
					pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
			}
			else
			{
				if ( pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED ) )
					pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::Precache( void )
{
	int iMdlIndex = PrecacheModel( "models/player/items/engineer/guitar.mdl" );
	PrecacheGibsForModel( iMdlIndex );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::CanGetRevengeCrits( void ) const
{
	int nSentryRevenge = 0;
	CALL_ATTRIB_HOOK_INT( nSentryRevenge, sentry_killed_revenge );
	return nSentryRevenge == 1;
}


float CTFSodaPopper::GetEffectBarProgress(void)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return pOwner->m_Shared.GetHypeMeter() / 100.0f;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPepBrawlBlaster::GetEffectBarProgress(void)
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner)
	{
		return pOwner->m_Shared.GetHypeMeter() / 100.0f;
	}

	return 0.0f;
}

float CTFPepBrawlBlaster::GetSpeedMod(void) const
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 1.0f;

	// Check if we get boost on damage.
	int nBoostOnDamage = 0;
	CALL_ATTRIB_HOOK_INT( nBoostOnDamage, boost_on_damage );
	if ( nBoostOnDamage == 0 )
		return 1.0f;	// No boost bails.

	// Use a linear relationship between boost level and added speed.
	// Original speed calculation ranged from 260HU/s at 0%, to 520HU/s at 100%.
	// This is Output = (260/100)x + 260, and then divided by input 260 to get a ratio.
	// The max ratio is 2, so we can simplify even further to get the result below.
	return ((pOwner->m_Shared.GetHypeMeter() / 100) + 1);
}


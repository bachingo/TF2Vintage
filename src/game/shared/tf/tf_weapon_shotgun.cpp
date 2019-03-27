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

// Client specific.
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#endif
	

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Soldier, tf_weapon_shotgun_soldier )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_HWG, tf_weapon_shotgun_hwg )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Pyro, tf_weapon_shotgun_pyro )
CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )

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


IMPLEMENT_NETWORKCLASS_ALIASED( TFShotgun_Revenge, DT_TFShotgun_Revenge )

BEGIN_NETWORK_TABLE( CTFShotgun_Revenge, DT_TFShotgun_Revenge )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_iRevengeCrits ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO( m_iRevengeCrits ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFShotgun_Revenge )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sentry_revenge, CTFShotgun_Revenge );
PRECACHE_WEAPON_REGISTER( tf_weapon_sentry_revenge );


CTFShotgun_Revenge::CTFShotgun_Revenge()
{
	m_bReloadsSingly = true;
	m_iRevengeCrits = 0;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetWorldModelIndex( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && pOwner->IsAlive())
	{
		if (pOwner->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER && pOwner->m_Shared.InCond( TF_COND_TAUNTING ))
		{
			int iMdlIdx = modelinfo->GetModelIndex( "models/player/items/engineer/guitar.mdl" );
			return iMdlIdx;
		}
	}

	return BaseClass::GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::SetWeaponVisible( bool visible )
{
	if (!visible)
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if (pOwner && pOwner->IsAlive())
		{
			if (pOwner->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER && pOwner->m_Shared.InCond( TF_COND_TAUNTING ))
			{
				int iModelIndex = modelinfo->GetModelIndex( "models/player/items/engineer/guitar.mdl" );

				CUtlVector<breakmodel_t> list;

				BuildGibList( list, iModelIndex, 1.0f, COLLISION_GROUP_NONE );
				if (!list.IsEmpty())
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
	if (!CanAttack())
		return;

	BaseClass::PrimaryAttack();

	m_iRevengeCrits = Max( m_iRevengeCrits - 1, 0 );
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
	if (m_iRevengeCrits > 0)
		return TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::Deploy( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && BaseClass::Deploy())
	{
		if (m_iRevengeCrits > 0)
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
	if (pOwner && BaseClass::Holster( pSwitchTo ))
	{
		if (m_iRevengeCrits > 0)
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

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::OnSentryKilled( CObjectSentrygun *pSentry )
{
	if (CanGetRevengeCrits())
	{
		m_iRevengeCrits = Min( m_iRevengeCrits + pSentry->GetAssists() + ( pSentry->GetKills() * 2 ), TF_WEAPON_MAX_REVENGE );

		CTFPlayer *pOwner = GetTFPlayerOwner();
		if (pOwner)
		{
			if (m_iRevengeCrits > 0)
			{
				if (!pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED ))
					pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
			}
			else
			{
				if (pOwner->m_Shared.InCond( TF_COND_CRITBOOSTED ))
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
	return CAttributeManager::AttribHookValue<int>( 0, "sentry_killed_revenge", this ) == 1;
}

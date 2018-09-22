//=========== Copyright © 2018, LFE-Team, Not All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_buff_item.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "effect_dispatch_data.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
// Server specific.
#else
#include "tf_player.h"
#include "ai_basenpc.h"
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
#endif

#define TF_BUFF_RADIUS 450.0f

enum
{
	TF_BUFF_OFFENSE = 1,
	TF_BUFF_DEFENSE,
	TF_BUFF_REGENONDAMAGE,
};

//=============================================================================
//
// Weapon BUFF item tables.
//


IMPLEMENT_NETWORKCLASS_ALIASED( TFBuffItem, DT_WeaponBuffItem )

BEGIN_NETWORK_TABLE( CTFBuffItem, DT_WeaponBuffItem )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flEffectBarProgress ) ),
#else
	SendPropFloat( SENDINFO( m_flEffectBarProgress ), 11, 0, 0.0f, 1.0f ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBuffItem )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flEffectBarRegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_buff_item, CTFBuffItem );
PRECACHE_WEAPON_REGISTER( tf_weapon_buff_item );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFBuffItem )
END_DATADESC()
#endif

CTFBuffItem::CTFBuffItem()
{
	m_flEffectBarRegenTime = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::Precache( void )
{
	BaseClass::Precache();
	PrecacheTeamParticles( "soldierbuff_%s_buffed" );
	PrecacheParticleSystem( "soldierbuff_mvm" );

	PrecacheScriptSound( "Weapon_BuffBanner.HornRed" );
	PrecacheScriptSound( "Weapon_BuffBanner.HornBlue" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornRed" );
	PrecacheScriptSound( "Weapon_BattalionsBackup.HornBlue" );
	PrecacheScriptSound( "Samurai.Conch" );
	PrecacheScriptSound( "Weapon_BuffBanner.Flag" );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge meter after use
//-----------------------------------------------------------------------------
bool CTFBuffItem::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( bBuffUsed )
	{
		m_flEffectBarProgress = 0.0f;
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge meter 
//-----------------------------------------------------------------------------
void CTFBuffItem::WeaponReset( void )
{
	m_flEffectBarRegenTime = 0.0f;
	m_flEffectBarProgress = 0.0f;
	bBuffUsed = false;

	BaseClass::WeaponReset();
}


// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	//if ( m_flEffectBarRegenTime != InternalGetEffectBarRechargeTime() )
	//	return;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	if ( GetBuffType() == TF_BUFF_OFFENSE )
	{
		if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
			pOwner->EmitSound( "Weapon_BuffBanner.HornRed" );
		else if ( pOwner->GetTeamNumber() == TF_TEAM_BLUE )
			pOwner->EmitSound( "Weapon_BuffBanner.HornBlue" );
	}
	else if ( GetBuffType() == TF_BUFF_DEFENSE )
	{
		if ( pOwner->GetTeamNumber() == TF_TEAM_RED )
			pOwner->EmitSound( "Weapon_BattalionsBackup.HornRed" );
		else if ( pOwner->GetTeamNumber() == TF_TEAM_BLUE )
			pOwner->EmitSound( "Weapon_BattalionsBackup.HornBlue" );
	}
	else if ( GetBuffType() == TF_BUFF_REGENONDAMAGE )
	{
		pOwner->EmitSound( "Samurai.Conch" );
	}

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
	{
#ifdef GAME_DLL 
		pOwner->SpeakWeaponFire( MP_CONCEPT_PLAYER_BATTLECRY );
		CTF_GameStats.Event_PlayerFiredWeapon( pOwner, false );
#endif
		pOwner->SwitchToNextBestWeapon( this );
		pOwner->EmitSound( "Weapon_BuffBanner.Flag" );

		IGameEvent *event_buff = gameeventmanager->CreateEvent( "deploy_buff_banner" );
		if ( event_buff )
		{
			event_buff->SetInt( "buff_type", GetBuffType() );
			event_buff->SetInt( "buff_owner", pOwner->entindex() );
 			gameeventmanager->FireEvent( event_buff );
		}
#ifdef GAME_DLL 
		CBaseEntity *pEntity = NULL;
		Vector vecOrigin = pOwner->GetAbsOrigin();

		for ( CEntitySphereQuery sphere( vecOrigin, TF_BUFF_RADIUS ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if ( !pEntity )
				continue;

			Vector vecHitPoint;
			pEntity->CollisionProp()->CalcNearestPoint( vecOrigin, &vecHitPoint );
			Vector vecDir = vecHitPoint - vecOrigin;

			CTFPlayer *pPlayer = ToTFPlayer( pEntity );

			if ( vecDir.LengthSqr() < ( TF_BUFF_RADIUS * TF_BUFF_RADIUS ) )
			{
				if ( pPlayer )
				{
					if ( pPlayer->InSameTeam( pOwner ) )
					{
						if ( GetBuffType() == TF_BUFF_OFFENSE )
							pPlayer->m_Shared.AddCond( TF_COND_OFFENSEBUFF );
						else if ( GetBuffType() == TF_BUFF_DEFENSE )
							pPlayer->m_Shared.AddCond( TF_COND_DEFENSEBUFF );
						else if ( GetBuffType() == TF_BUFF_REGENONDAMAGE )
							pPlayer->m_Shared.AddCond( TF_COND_REGENONDAMAGEBUFF );
					}
				}
			}
			else
			{
				if ( pPlayer )
				{
					if ( pPlayer->InSameTeam( pOwner ) )
					{
						if ( GetBuffType() == TF_BUFF_OFFENSE )
							pPlayer->m_Shared.RemoveCond( TF_COND_OFFENSEBUFF );
						else if ( GetBuffType() == TF_BUFF_DEFENSE )
							pPlayer->m_Shared.RemoveCond( TF_COND_DEFENSEBUFF );
						else if ( GetBuffType() == TF_BUFF_REGENONDAMAGE )
							pPlayer->m_Shared.RemoveCond( TF_COND_REGENONDAMAGEBUFF );
					}
				}
			}
		}
#endif
	}

	//m_flEffectBarRegenTime = gpGlobals->curtime + InternalGetEffectBarRechargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBuffItem::CheckEffectBarRegen( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_flEffectBarRegenTime != 0.0f )
	{
		if ( gpGlobals->curtime >= m_flEffectBarRegenTime )
		{
			m_flEffectBarRegenTime = 0.0f;
			EffectBarRegenFinished();
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CTFBuffItem::GetBuffType( void )
{
	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );
	return iBuffType;
}
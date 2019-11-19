//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_halloween_boss.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#endif


CHalloweenBaseBoss::CHalloweenBaseBoss()
{
}


CHalloweenBaseBoss::~CHalloweenBaseBoss()
{
}

#ifdef GAME_DLL
CHalloweenBaseBoss *CHalloweenBaseBoss::SpawnBossAtPos( HalloweenBossType type, Vector const& pos, int teamNum, CBaseEntity *pOwner )
{
	CHalloweenBaseBoss *pBoss = NULL;
	switch (type)
	{
		case HEADLESS_HATMAN:
			pBoss = dynamic_cast<CHalloweenBaseBoss *>( CreateEntityByName( "headless_hatman" ) );
			break;
		case EYEBALL_BOSS:
			pBoss = dynamic_cast<CHalloweenBaseBoss *>( CreateEntityByName( "eyeball_boss" ) );
			break;
		case MERASMUS:
			pBoss = dynamic_cast<CHalloweenBaseBoss *>( CreateEntityByName( "merasmus" ) );
			break;
		default:
			return NULL;
	}

	if ( pBoss )
	{
		pBoss->SetAbsOrigin( pos + Vector( 0, 0, 10 ) );
		pBoss->ChangeTeam( teamNum );
		pBoss->SetOwnerEntity( pOwner );
		DispatchSpawn( pBoss );

		return pBoss;
	}

	return NULL;
}


void CHalloweenBaseBoss::Spawn( void )
{
	BaseClass::Spawn();

	TFGameRules()->RegisterBoss( this );

	ConVarRef sv_cheats( "sv_cheats" );
	if (sv_cheats.IsValid() && sv_cheats.GetBool())
		m_bSpawnedWithCheats = true;

	m_flDPSCounter = 0;
	m_flDPSMax = 0;
}

void CHalloweenBaseBoss::UpdateOnRemove( void )
{
	TFGameRules()->RemoveBoss( this );
	BaseClass::UpdateOnRemove();
}

int CHalloweenBaseBoss::OnTakeDamage( const CTakeDamageInfo& info )
{
	if (info.GetDamage() != 0.0f)
	{
		if (info.GetAttacker() != this && info.GetAttacker()->IsPlayer())
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
			if (pWeapon)
				pWeapon->ApplyOnHitAttributes( this, ToTFPlayer( info.GetAttacker() ), info );
		}
	}

	return BaseClass::OnTakeDamage( info );
}

int CHalloweenBaseBoss::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
	CTakeDamageInfo newInfo( info );
	if (!info.GetAttacker())
	{
		if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN)
			newInfo.SetDamage( 0.0f );

		if (info.GetDamageType() & DMG_CRITICAL)
			newInfo.ScaleDamage( this->GetCritInjuryMultiplier() );

		IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
		if (event)
		{
			event->SetInt( "entindex", entindex() );
			event->SetInt( "health", GetHealth() );
			event->SetInt( "damageamount", newInfo.GetDamage() );
			event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) == DMG_CRITICAL );
			event->SetInt( "boss", GetBossType() );
			event->SetInt( "attacker_player", -1 );
			event->SetInt( "weaponid", 0 );

			gameeventmanager->FireEvent( event );
		}

		return BaseClass::OnTakeDamage_Alive( newInfo );
	}

	if (info.GetAttacker()->GetTeamNumber() == GetTeamNumber())
		return 0;

	if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN)
		newInfo.SetDamage( 0.0f );
	
	if (info.GetDamageType() & DMG_CRITICAL)
		newInfo.ScaleDamage( this->GetCritInjuryMultiplier() );

	CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
	if (pPlayer)
	{
		bool isMelee = false;
		CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon *)info.GetWeapon();
		if (pWeapon)
			isMelee = pWeapon->IsMeleeWeapon();

		RememberAttacker( pPlayer, isMelee, newInfo.GetDamage() );
		for (int i=0; i<pPlayer->m_Shared.GetNumHealers(); ++i)
		{
			CTFPlayer *pHealer = ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) );
			if (pHealer)
				RememberAttacker( pHealer, isMelee, 0.0f );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if (event)
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", GetHealth() );
		event->SetInt( "damageamount", newInfo.GetDamage() );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) == DMG_CRITICAL );
		event->SetInt( "boss", GetBossType() );

		CTFPlayer *pPlayer = ToTFPlayer( info.GetAttacker() );
		if (pPlayer)
		{
			event->SetInt( "attacker_player", engine->GetPlayerUserId( pPlayer->edict() ) );
			event->SetInt( "weaponid", pPlayer->GetActiveTFWeapon() ? pPlayer->GetActiveTFWeapon()->GetWeaponID() : 0 );
		}
		else
		{
			event->SetInt( "attacker_player", -1 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	return BaseClass::OnTakeDamage_Alive( newInfo );
}

void CHalloweenBaseBoss::Event_Killed( const CTakeDamageInfo& info )
{
	for (int i=0; i<m_lastAttackers.Count(); ++i)
	{
		if (m_lastAttackers[i].m_hPlayer)
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "halloween_boss_killed" );
			if (event)
			{
				event->SetInt( "boss", GetBossType() );
				event->SetInt( "killer", engine->GetPlayerUserId( m_lastAttackers[i].m_hPlayer->edict() ) );

				gameeventmanager->FireEvent( event );
			}

			//TFGameRules()->DropHalloweenSoulPack( 25, WorldSpaceCenter(), m_lastAttackers[i].m_hPlayer, true );
		}
	}

	BaseClass::Event_Killed( info );
}
#endif

int CHalloweenBaseBoss::GetBossType( void ) const
{
	return 0;
}

float CHalloweenBaseBoss::GetCritInjuryMultiplier( void ) const
{
	return 3.0f;
}

int CHalloweenBaseBoss::GetLevel( void ) const
{
	return 0;
}

#ifdef GAME_DLL
void CHalloweenBaseBoss::RememberAttacker( CTFPlayer *pAttacker, bool bMelee, float flDmgAmount )
{
	m_damageInfos.AddToTail( { flDmgAmount, gpGlobals->curtime } );

	for (int i=0; i<m_lastAttackers.Count(); ++i)
	{
		AttackerInfo *info = &m_lastAttackers[i];
		if (ENTINDEX( info->m_hPlayer ) == ENTINDEX( pAttacker ))
		{
			info->m_flTimeDamaged = gpGlobals->curtime;
			info->m_wasMelee = bMelee;
			return;
		}
	}
	m_lastAttackers.AddToHead( { pAttacker, gpGlobals->curtime, bMelee } );
}

void CHalloweenBaseBoss::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}

void CHalloweenBaseBoss::UpdateDamagePerSecond( void )
{
	float flAvgDPS = 0.0f;
	m_flDPSCounter = 0;

	for (int i=0; i<m_damageInfos.Count(); ++i)
	{
		DamageRateInfo *info = &m_damageInfos[i];
		if (( gpGlobals->curtime - info->m_flTimeDealt ) <= 5.0f)
		{
			flAvgDPS += info->m_flDamage;
			m_flDPSCounter = flAvgDPS;
		}
	}
	flAvgDPS *= 0.2f;

	m_flDPSCounter = flAvgDPS;
	if (flAvgDPS > m_flDPSMax)
		m_flDPSMax = flAvgDPS;
}


void CHalloweenBaseBoss::Update( void )
{
	BaseClass::Update();
	UpdateDamagePerSecond();
}
#endif